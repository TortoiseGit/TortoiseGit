// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2024 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "GitHash.h"
#include "gitdll.h"
#include "GitStatus.h"
#include "UnicodeUtils.h"
#include "ReaderWriterLock.h"
#include "GitAdminDir.h"
#include "StringUtils.h"
#include "PathUtils.h"

#ifndef S_IFLNK
#define S_IFLNK 0120000
#undef _S_IFLNK
#define _S_IFLNK S_IFLNK
#endif
#ifndef S_ISLNK
#define S_ISLNK(m) (((m) & _S_IFMT) == _S_IFLNK)
#endif

struct CGitIndex
{
	/* m_Size and m_ModifyTime are only uint32_t in libgit2, cf. https://github.com/libgit2/libgit2/blob/8535fdb9cbad8fcd15ee4022ed29c4138547e22d/include/git2/index.h#L48-L51 and https://tortoisegit.org/issue/4108 */
	CString    m_FileName;
	mutable int32_t	m_ModifyTime;
	mutable uint32_t	m_ModifyTimeNanos;
	uint16_t	m_Flags;
	uint16_t	m_FlagsExtended;
	CGitHash	m_IndexHash;
	uint32_t	m_Size;
	uint32_t	m_Mode;

	int Print();
};

class CGitIndexList : private std::vector<CGitIndex>
{
public:
	BOOL		m_bHasConflicts = FALSE;
	CString		m_branch;
	size_t		m_incoming = static_cast<size_t>(-1);
	size_t		m_outgoing = static_cast<size_t>(-1);
	size_t		m_stashCount = 0;
	inline bool IsIgnoreCase() const { return m_iIndexCaps & GIT_INDEX_CAPABILITY_IGNORE_CASE; }

	CGitIndexList();
	~CGitIndexList();

	bool HasIndexChangedOnDisk(const CString& gitdir) const;
	int ReadIndex(const CString& dotgitdir);
	int ReadIncomingOutgoing(git_repository* repo);
	int GetFileStatus(const CString& gitdir, const CString& path, git_wc_status2_t& status, CGitHash* pHash = nullptr) const;
	int GetFileStatus(CAutoRepository& repository, const CString& gitdir, const CGitIndex& entry, git_wc_status2_t& status, __int64 time, __int64 filesize, bool isSymlink) const;

	using std::vector<CGitIndex>::begin;
	using std::vector<CGitIndex>::end;
	using std::vector<CGitIndex>::cbegin;
	using std::vector<CGitIndex>::cend;
	using std::vector<CGitIndex>::empty;
	using std::vector<CGitIndex>::size;
	using std::vector<CGitIndex>::operator[];

#ifdef GOOGLETEST_INCLUDE_GTEST_GTEST_H_
	FRIEND_TEST(GitIndexCBasicGitWithTestRepoFixture, GetFileStatus);
#endif
private:
	__time64_t m_LastModifyTime = 0;
	__int64 m_LastFileSize = -1;

	int		m_iIndexCaps = GIT_INDEX_CAPABILITY_IGNORE_CASE | GIT_INDEX_CAPABILITY_NO_SYMLINKS;
	__int64 m_iMaxCheckSize = 10 * 1024 * 1024;
	bool	m_bCalculateIncomingOutgoing = true;
	CAutoConfig config;
	int GetFileStatus(const CString& gitdir, const CString& path, git_wc_status2_t& status, __int64 time, __int64 filesize, bool isSymlink, CGitHash* pHash = nullptr) const;
};

using SHARED_INDEX_PTR = std::shared_ptr<const CGitIndexList>;
using CAutoLocker = CComCritSecLock<CComCriticalSection>;

template<typename SharedPtr>
class SharedPtrMapTmpl : private std::map<CString, SharedPtr>
{
public:
	[[nodiscard]] SharedPtr SafeGet(const CString& path)
	{
		CString thePath(CPathUtils::NormalizePath(path));
		CAutoLocker lock(m_critSec);
		auto lookup = this->find(thePath);
		if (lookup == this->cend())
			return {};
		return lookup->second;
	}

	bool SafeClear(const CString& path)
	{
		CString thePath(CPathUtils::NormalizePath(path));
		CAutoLocker lock(m_critSec);
		auto lookup = this->find(thePath);
		if (lookup == this->cend())
			return false;
		this->erase(lookup);
		return true;
	}

	bool SafeClearRecursively(const CString& path)
	{
		CString thePath(CPathUtils::NormalizePath(path));
		CAutoLocker lock(m_critSec);
		std::vector<CString> toRemove;
		for (auto it = this->cbegin(); it != this->cend(); ++it)
		{
			if (CStringUtils::StartsWith((*it).first, thePath))
				toRemove.push_back((*it).first);
		}
		for (auto it = toRemove.cbegin(); it != toRemove.cend(); ++it)
			this->erase(*it);
		return !toRemove.empty();
	}

protected:
	void SafeSet(const CString& path, SharedPtr ptr)
	{
		CString thePath(CPathUtils::NormalizePath(path));
		CAutoLocker lock(m_critSec);
		(*this)[thePath] = ptr;
	}

private:
	CComAutoCriticalSection m_critSec;
};

class CGitIndexFileMap : protected SharedPtrMapTmpl<SHARED_INDEX_PTR>
{
public:
	[[nodiscard]] SHARED_INDEX_PTR CheckAndUpdate(const CString& gitdir)
	{
		if (auto pIndex = SafeGet(gitdir); pIndex && !pIndex->HasIndexChangedOnDisk(gitdir))
			return pIndex;

		auto newIndex = std::make_shared<CGitIndexList>();
		if (newIndex->ReadIndex(gitdir))
		{
			SafeClear(gitdir);
			return {};
		}

		SafeSet(gitdir, newIndex);

		return newIndex;
	}

	using SharedPtrMapTmpl<SHARED_INDEX_PTR>::SafeClear;
	using SharedPtrMapTmpl<SHARED_INDEX_PTR>::SafeClearRecursively;
	using SharedPtrMapTmpl<SHARED_INDEX_PTR>::SafeGet;
};

struct CGitTreeItem
{
	CString	m_FileName;
	CGitHash	m_Hash;
	int			m_Flags;
};

/* After object create, never change field against
 * that needn't lock to get field
*/
class CGitHeadFileList : private std::vector<CGitTreeItem>
{
private:
	int GetPackRef(const CString &gitdir);

	__time64_t	m_LastModifyTimeHead = 0;
	__time64_t	m_LastModifyTimeRef = 0;
	__time64_t	m_LastModifyTimePackRef = 0;

	__int64		m_LastFileSizeHead = -1;
	__int64		m_LastFileSizePackRef = -1;

	CString		m_HeadRefFile;
	CGitHash	m_Head;
	CString		m_HeadFile;
	CString		m_Gitdir;
	CString		m_PackRefFile;
	bool		m_bRefFromPackRefFile = false;

	std::map<CString,CGitHash> m_PackRefMap;

public:
	CGitHeadFileList() = default;

	int ReadTree(bool ignoreCase);
	int ReadHeadHash(const CString& gitdir);
	bool CheckHeadUpdate() const;

	using std::vector<CGitTreeItem>::begin;
	using std::vector<CGitTreeItem>::end;
	using std::vector<CGitTreeItem>::cbegin;
	using std::vector<CGitTreeItem>::cend;
	using std::vector<CGitTreeItem>::empty;
	using std::vector<CGitTreeItem>::size;
	using std::vector<CGitTreeItem>::operator[];

private:
	int ReadTreeRecursive(git_repository& repo, const git_tree* tree, const CString& base);
};

using SHARED_TREE_PTR = std::shared_ptr<const CGitHeadFileList>;
class CGitHeadFileMap : protected SharedPtrMapTmpl<SHARED_TREE_PTR>
{
public:
	[[nodiscard]] SHARED_TREE_PTR CheckHeadAndUpdate(const CString& gitdir, bool ignoreCase);

	using SharedPtrMapTmpl<SHARED_TREE_PTR>::SafeClear;
	using SharedPtrMapTmpl<SHARED_TREE_PTR>::SafeClearRecursively;
	using SharedPtrMapTmpl<SHARED_TREE_PTR>::SafeGet;
};

struct CGitFileName
{
	CGitFileName() = default;
	CGitFileName(LPCWSTR filename, __int64 size, __int64 lastmodified)
	: m_FileName(filename)
	, m_Size(size)
	, m_LastModified(lastmodified)
	{
	}
	CString m_FileName;
	__int64 m_Size = -1;
	__int64 m_LastModified = 0;
	bool	m_bSymlink = false;
};

class CGitIgnoreItem
{
public:
	~CGitIgnoreItem()
	{
		if(m_pExcludeList)
			git_free_exclude_list(m_pExcludeList);
	}

	__time64_t	m_LastModifyTime = 0;
	__int64		m_LastFileSize = -1;
	CStringA m_BaseDir;
	std::unique_ptr<char[]> m_buffer;
	EXCLUDE_LIST m_pExcludeList = nullptr;
	int* m_iIgnoreCase = nullptr;

	int FetchIgnoreList(const CString& projectroot, const CString& file, bool isGlobal, int* ignoreCase);

	/**
	* patha: the filename to be checked whether it is ignored or not
	* base: must be a pointer to the beginning of the base filename WITHIN patha
	* type: DT_DIR or DT_REG
	*/
	int IsPathIgnored(const CStringA& patha, const char* base, int& type);
#ifdef GOOGLETEST_INCLUDE_GTEST_GTEST_H_
	int IsPathIgnored(const CStringA& patha, int& type);
#endif
};

class CGitIgnoreList
{
private:
	bool CheckFileChanged(const CString &path);
	int FetchIgnoreFile(const CString &gitdir, const CString &gitignore, bool isGlobal);

	int CheckIgnore(const CString& path, const CString& root, bool isDir, const CString& adminDir);
	int CheckFileAgainstIgnoreList(const CString &ignorefile, const CStringA &patha, const char * base, int &type);

	// core.excludesfile stuff
	std::map<CString, CString> m_CoreExcludesfiles;
	std::map<CString, int> m_IgnoreCase;
	CString m_sGitSystemConfigPath;
	ULONGLONG m_dGitSystemConfigPathLastChecked = 0LL;
	CReaderWriterLock	m_coreExcludefilesSharedMutex;
	// checks if the msysgit path has changed and return true/false
	// if the path changed, the cache is update
	// force is only ised in constructor
	bool CheckAndUpdateGitSystemConfigPath(bool force = true);
	bool CheckAndUpdateCoreExcludefile(const CString &adminDir);
	const CString GetWindowsHome();

public:
	CReaderWriterLock		m_SharedMutex;

	CGitIgnoreList(){ CheckAndUpdateGitSystemConfigPath(true); }

	std::map<CString, CGitIgnoreItem> m_Map;

	bool CheckAndUpdateIgnoreFiles(const CString& gitdir, const CString& path, bool isDir, std::set<CString>* lastChecked = nullptr);
	bool IsIgnore(CString path, const CString& root, bool isDir, const CString& adminDir);
};

template<class T>
inline void DoSortFilenametSortVector(T& vector, bool ignoreCase)
{
	if (ignoreCase)
		std::sort(vector.begin(), vector.end(), [](const auto& e1, const auto& e2) { return e1.m_FileName.CompareNoCase(e2.m_FileName) < 0; });
	else
		std::sort(vector.begin(), vector.end(), [](const auto& e1, const auto& e2) { return e1.m_FileName.Compare(e2.m_FileName) < 0; });
}

static const size_t NPOS = static_cast<size_t>(-1); // bad/missing length/position
static_assert(MAXSIZE_T == NPOS, "NPOS must equal MAXSIZE_T");
static_assert(-1 == static_cast<int>(NPOS), "NPOS must equal -1");

template<class T>
inline int GetRangeInSortVector(const T& vector, LPCWSTR pstr, size_t len, bool ignoreCase, size_t* start, size_t* end, size_t pos)
{
	if (ignoreCase)
		return GetRangeInSortVector_int(vector, pstr, len, _wcsnicmp, start, end, pos);

	return GetRangeInSortVector_int(vector, pstr, len, wcsncmp, start, end, pos);
}

template<class T, class V>
int GetRangeInSortVector_int(const T& vector, LPCWSTR pstr, size_t len, V compare, size_t* start, size_t* end, size_t pos)
{
	static_assert(std::is_convertible_v<V, std::function<int(const wchar_t*, const wchar_t*, size_t)>>, "Wrong signature for the compare method, needs to be a wcsncmp equivalent");
	if (pos == NPOS)
		return -1;
	if (!start || !end)
		return -1;

	*start = *end = NPOS;

	if (vector.empty())
		return -1;

	if (pos >= vector.size())
		return -1;

	if (compare(vector[pos].m_FileName, pstr, len) != 0)
		return -1;

	*start = 0;
	*end = vector.size() - 1;

	// shortcut, if all entries are going match
	if (!len)
		return 0;

	for (size_t i = pos; i < vector.size(); ++i)
	{
		if (compare(vector[i].m_FileName, pstr, len) != 0)
			break;

		*end = i;
	}
	for (size_t i = pos + 1; i-- > 0;)
	{
		if (compare(vector[i].m_FileName, pstr, len) != 0)
			break;

		*start = i;
	}

	return 0;
}

template<class T>
inline size_t SearchInSortVector(const T& vector, LPCWSTR pstr, int len, bool ignoreCase)
{
	if (ignoreCase)
	{
		if (len < 0)
			return SearchInSortVector_int(vector, pstr, _wcsicmp);

		return SearchInSortVector_int(vector, pstr, [len](const auto& s1, const auto& s2) { return _wcsnicmp(s1, s2, len); });
	}

	if (len < 0)
		return SearchInSortVector_int(vector, pstr, wcscmp);

	return SearchInSortVector_int(vector, pstr, [len](const auto& s1, const auto& s2) { return wcsncmp(s1, s2, len); });
}

template<class T, class V>
size_t SearchInSortVector_int(const T& vector, LPCWSTR pstr, V compare)
{
	static_assert(std::is_convertible_v<V, std::function<int(const wchar_t*, const wchar_t*)>>, "Wrong signature for the compare method, needs to be a wcscmp equivalent");
	size_t end = vector.size() - 1;
	size_t start = 0;
	size_t mid = (start + end) / 2;

	if (vector.empty())
		return NPOS;

	while(!( start == end && start==mid))
	{
		const int cmp = compare(vector[mid].m_FileName, pstr);
		if (cmp == 0)
			return mid;
		else if (cmp < 0)
			start = mid + 1;
		else // (cmp > 0)
			end = mid;

		mid=(start +end ) /2;

	}

	if (compare(vector[mid].m_FileName, pstr) == 0)
		return mid;

	return NPOS;
};

class CGitAdminDirMap : private std::map<CString, CString>
{
public:
	CComAutoCriticalSection		m_critIndexSec;
	std::map<CString, CString>	m_reverseLookup;
	std::map<CString, CString>	m_WorktreeAdminDirLookup;

	CString GetAdminDir(const CString &path)
	{
		CString thePath(CPathUtils::NormalizePath(path));
		CAutoLocker lock(m_critIndexSec);
		auto lookup = find(thePath);
		if (lookup == cend())
		{
			CString adminDir;
			bool isWorktree = false;
			if (GitAdminDir::GetAdminDirPath(path, adminDir, &isWorktree) && PathIsDirectory(adminDir))
			{
				(*this)[thePath] = adminDir;
				if (!isWorktree) // GitAdminDir::GetAdminDirPath returns the commongit dir ("parent/.git") and this would override the lookup path for the main repo
					m_reverseLookup[CPathUtils::BuildPathWithPathDelimiter(CPathUtils::NormalizePath(adminDir))] = path;
				return (*this)[thePath];
			}
			ATLASSERT(false);
			return CPathUtils::BuildPathWithPathDelimiter(path) + L".git\\"; // in case of an error stick to old behavior
		}

		return lookup->second;
	}

	CString GetAdminDirConcat(const CString& path, const CString& subpath)
	{
		CString result(GetAdminDir(path));
		result += subpath;
		return result;
	}

	CString GetWorktreeAdminDir(const CString& path)
	{
		CString thePath(CPathUtils::NormalizePath(path));
		CAutoLocker lock(m_critIndexSec);
		auto lookup = m_WorktreeAdminDirLookup.find(thePath);
		if (lookup == m_WorktreeAdminDirLookup.cend())
		{
			CString wtadmindir;
			if (GitAdminDir::GetWorktreeAdminDirPath(path, wtadmindir) && PathIsDirectory(wtadmindir))
			{
				m_WorktreeAdminDirLookup[thePath] = wtadmindir;
				m_reverseLookup[CPathUtils::BuildPathWithPathDelimiter(CPathUtils::NormalizePath(wtadmindir))] = path;
				return m_WorktreeAdminDirLookup[thePath];
			}
			ATLASSERT(false);
			return CPathUtils::BuildPathWithPathDelimiter(path) + L".git\\"; // we should never get here
		}
		return lookup->second;
	}

	CString GetWorktreeAdminDirConcat(const CString& path, const CString& subpath)
	{
		CString result(GetWorktreeAdminDir(path));
		result += subpath;
		return result;
	}

	CString GetWorkingCopy(const CString &gitDir)
	{
		CString path(CPathUtils::BuildPathWithPathDelimiter(CPathUtils::NormalizePath(gitDir)));
		CAutoLocker lock(m_critIndexSec);
		auto lookup = m_reverseLookup.find(path);
		if (lookup == m_reverseLookup.cend())
			return gitDir;
		return lookup->second;
	}

#ifdef GOOGLETEST_INCLUDE_GTEST_GTEST_H_
	using std::map<CString, CString>::clear;
#endif
};
