// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017 - TortoiseGit

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

class CGitIndex
{
public:
	CString    m_FileName;
	__time64_t	m_ModifyTime;
	uint16_t	m_Flags;
	uint16_t	m_FlagsExtended;
	CGitHash	m_IndexHash;
	__int64		m_Size;

	int Print();
};

class CGitIndexList:public std::vector<CGitIndex>
{
public:
	__time64_t  m_LastModifyTime;
	BOOL		m_bHasConflicts;

	CGitIndexList();
	~CGitIndexList();

	int ReadIndex(CString file);
	int GetStatus(const CString& gitdir, CString path, git_wc_status_kind* status, BOOL IsFull = FALSE, BOOL IsRecursive = FALSE, FILL_STATUS_CALLBACK callback = nullptr, void* pData = nullptr, CGitHash* pHash = nullptr, bool* assumeValid = nullptr, bool* skipWorktree = nullptr);
#ifdef GTEST_INCLUDE_GTEST_GTEST_H_
	FRIEND_TEST(GitIndexCBasicGitWithTestRepoFixture, GetFileStatus);
#endif
protected:
	__int64 m_iMaxCheckSize;
	CAutoConfig config;
	int GetFileStatus(const CString &gitdir, const CString &path, git_wc_status_kind * status, __int64 time, __int64 filesize, FILL_STATUS_CALLBACK callback = nullptr, void *pData = nullptr, CGitHash *pHash = nullptr, bool * assumeValid = nullptr, bool * skipWorktree = nullptr);
};

typedef std::shared_ptr<CGitIndexList> SHARED_INDEX_PTR;
typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

class CGitIndexFileMap:public std::map<CString, SHARED_INDEX_PTR>
{
public:
	CComCriticalSection			m_critIndexSec;

	CGitIndexFileMap() { m_critIndexSec.Init(); }
	~CGitIndexFileMap() { m_critIndexSec.Term(); }

	SHARED_INDEX_PTR SafeGet(CString thePath)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		auto lookup = find(thePath);
		if (lookup == cend())
			return SHARED_INDEX_PTR();
		return lookup->second;
	}

	void SafeSet(CString thePath, SHARED_INDEX_PTR ptr)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		(*this)[thePath] = ptr;
	}

	bool SafeClear(CString thePath)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		auto lookup = find(thePath);
		if (lookup == cend())
			return false;
		erase(lookup);
		return true;
	}

	bool SafeClearRecursively(CString thePath)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
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

	int Check(const CString &gitdir, bool *isChanged);
	int LoadIndex(const CString &gitdir);

	bool CheckAndUpdate(const CString& gitdir)
	{
		bool isChanged=false;
		if (Check(gitdir, &isChanged))
			return false;

		if (isChanged)
		{
			LoadIndex(gitdir);
			return true;
		}

		return false;
	}
	int GetFileStatus(const CString &gitdir,const CString &path,git_wc_status_kind * status,
							BOOL IsFull=false, BOOL IsRecursive=false,
							FILL_STATUS_CALLBACK callback = nullptr,
							void* pData = nullptr, CGitHash* pHash = nullptr,
							bool* assumeValid = nullptr, bool* skipWorktree = nullptr);

	int IsUnderVersionControl(const CString &gitdir,
							  CString path,
							  bool isDir,
							  bool *isVersion);
};


class CGitTreeItem
{
public:
	CString	m_FileName;
	CGitHash	m_Hash;
	int			m_Flags;
};

/* After object create, never change field agains
 * that needn't lock to get field
*/
class CGitHeadFileList:public std::vector<CGitTreeItem>
{
private:
	int GetPackRef(const CString &gitdir);

	__time64_t  m_LastModifyTimeHead;
	__time64_t  m_LastModifyTimeRef;
	__time64_t	m_LastModifyTimePackRef;

	CString		m_HeadRefFile;
	CGitHash	m_Head;
	CString		m_HeadFile;
	CString		m_Gitdir;
	CString		m_PackRefFile;

	CGitHash	m_TreeHash; /* buffered tree hash value */

	std::map<CString,CGitHash> m_PackRefMap;

public:
	CGitHeadFileList()
	: m_LastModifyTimeHead(0)
	, m_LastModifyTimeRef(0)
	, m_LastModifyTimePackRef(0)
	{
	}

	int ReadTree();
	int ReadHeadHash(const CString& gitdir);
	bool CheckHeadUpdate();
	bool HeadHashEqualsTreeHash();
	bool HeadFileIsEmpty();
	bool HeadIsEmpty();
	static int CallBack(const unsigned char *, const char *, int, const char *, unsigned int, int, void *);
};

typedef std::shared_ptr<CGitHeadFileList> SHARED_TREE_PTR;
class CGitHeadFileMap:public std::map<CString,SHARED_TREE_PTR>
{
public:

	CComCriticalSection			m_critTreeSec;

	CGitHeadFileMap() { m_critTreeSec.Init(); }
	~CGitHeadFileMap() { m_critTreeSec.Term(); }

	SHARED_TREE_PTR SafeGet(CString thePath, bool allowEmpty = false)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critTreeSec);
		auto lookup = find(thePath);
		if (lookup == cend())
		{
			if (allowEmpty)
				return SHARED_TREE_PTR();
			return std::make_shared<CGitHeadFileList>();
		}
		return lookup->second;
	}

	void SafeSet(CString thePath, SHARED_TREE_PTR ptr)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critTreeSec);
		(*this)[thePath] = ptr;
	}

	bool SafeClear(CString thePath)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critTreeSec);
		auto lookup = find(thePath);
		if (lookup == cend())
			return false;
		erase(lookup);
		return true;
	}

	bool SafeClearRecursively(CString thePath)
	{
		thePath.MakeLower();
		CAutoLocker lock(m_critTreeSec);
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

	int GetFileStatus(const CString &gitdir,const CString &path,git_wc_status_kind * status,BOOL IsFull=false, BOOL IsRecursive=false,
						FILL_STATUS_CALLBACK callback = nullptr, void *pData = nullptr,
						bool isLoaded=false);
	bool CheckHeadAndUpdate(const CString& gitdir);
	int IsUnderVersionControl(const CString& gitdir, CString path, bool isDir, bool* isVersion);
};

class CGitFileName
{
public:
	CGitFileName() {}
	CGitFileName(LPCTSTR filename)
	: m_FileName(filename)
	{
	}
	CString m_FileName;
};

static bool SortCGitFileName(const CGitFileName& item1, const CGitFileName& item2)
{
	return item1.m_FileName.Compare(item2.m_FileName) < 0;
}

class CGitIgnoreItem
{
public:
	CGitIgnoreItem()
	: m_LastModifyTime(0)
	, m_pExcludeList(nullptr)
	, m_buffer(nullptr)
	{
	}

	~CGitIgnoreItem()
	{
		if(m_pExcludeList)
			git_free_exclude_list(m_pExcludeList);
		free(m_buffer);
	}

	__time64_t  m_LastModifyTime;
	CStringA m_BaseDir;
	BYTE *m_buffer;
	EXCLUDE_LIST m_pExcludeList;

	int FetchIgnoreList(const CString &projectroot, const CString &file, bool isGlobal);

	/**
	* patha: the filename to be checked whether is is ignored or not
	* base: must be a pointer to the beginning of the base filename WITHIN patha
	* type: DT_DIR or DT_REG
	*/
	int IsPathIgnored(const CStringA& patha, const char* base, int& type);
#ifdef GTEST_INCLUDE_GTEST_GTEST_H_
	int IsPathIgnored(const CStringA& patha, int& type);
#endif
};

class CGitIgnoreList
{
private:
	bool CheckFileChanged(const CString &path);
	int FetchIgnoreFile(const CString &gitdir, const CString &gitignore, bool isGlobal);

	int  CheckIgnore(const CString &path, const CString &root, bool isDir);
	int CheckFileAgainstIgnoreList(const CString &ignorefile, const CStringA &patha, const char * base, int &type);

	// core.excludesfile stuff
	std::map<CString, CString> m_CoreExcludesfiles;
	CString m_sGitSystemConfigPath;
	CString m_sGitProgramDataConfigPath;
	ULONGLONG m_dGitSystemConfigPathLastChecked;
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

	bool CheckAndUpdateIgnoreFiles(const CString& gitdir, const CString& path, bool isDir);
	bool IsIgnore(CString path, const CString& root, bool isDir);
};

static const size_t NPOS = (size_t)-1; // bad/missing length/position
static_assert(MAXSIZE_T == NPOS, "NPOS must equal MAXSIZE_T");
#pragma warning(push)
#pragma warning(disable: 4310)
static_assert(-1 == (int)NPOS, "NPOS must equal -1");
#pragma warning(pop)

template<class T>
int GetRangeInSortVector(const T& vector, LPCTSTR pstr, size_t len, size_t* start, size_t* end, size_t pos)
{
	if (pos == NPOS)
		return -1;
	if (!start || !end)
		return -1;

	*start = *end = NPOS;

	if (vector.empty())
		return -1;

	if (pos >= vector.size())
		return -1;

	if (wcsncmp(vector[pos].m_FileName, pstr, len) != 0)
		return -1;

	*start = 0;
	*end = vector.size() - 1;

	// shortcut, if all entries are going match
	if (!len)
		return 0;

	for (size_t i = pos; i < vector.size(); ++i)
	{
		if (wcsncmp(vector[i].m_FileName, pstr, len) != 0)
			break;

		*end = i;
	}
	for (size_t i = pos + 1; i-- > 0;)
	{
		if (wcsncmp(vector[i].m_FileName, pstr, len) != 0)
			break;

		*start = i;
	}

	return 0;
}

template<class T>
size_t SearchInSortVector(const T& vector, LPCTSTR pstr, int len)
{
	size_t end = vector.size() - 1;
	size_t start = 0;
	size_t mid = (start + end) / 2;

	if (vector.empty())
		return NPOS;

	while(!( start == end && start==mid))
	{
		int cmp;
		if(len < 0)
			cmp = wcscmp(vector[mid].m_FileName, pstr);
		else
			cmp = wcsncmp(vector[mid].m_FileName, pstr, len);

		if (cmp == 0)
			return mid;
		else if (cmp < 0)
			start = mid + 1;
		else // (cmp > 0)
			end = mid;

		mid=(start +end ) /2;

	}
	if(len <0)
	{
		if (wcscmp(vector[mid].m_FileName, pstr) == 0)
			return mid;
	}
	else
	{
		if (wcsncmp(vector[mid].m_FileName, pstr, len) == 0)
			return mid;
	}
	return NPOS;
};

class CGitAdminDirMap:public std::map<CString, CString>
{
public:
	CComCriticalSection			m_critIndexSec;
	std::map<CString, CString>	m_reverseLookup;

	CGitAdminDirMap() { m_critIndexSec.Init(); }
	~CGitAdminDirMap() { m_critIndexSec.Term(); }

	CString GetAdminDir(const CString &path)
	{
		CString thePath(path);
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		auto lookup = find(thePath);
		if (lookup == cend())
		{
			if (PathIsDirectory(path + L"\\.git"))
			{
				(*this)[thePath] = path + L"\\.git\\";
				m_reverseLookup[thePath + L"\\.git"] = path;
				return (*this)[thePath];
			}

			CString result = GitAdminDir::ReadGitLink(path, path + L"\\.git");
			if (!result.IsEmpty())
			{
				(*this)[thePath] = result + L'\\';
				m_reverseLookup[result.MakeLower()] = path;
				return (*this)[thePath];
			}

			return path + L"\\.git\\"; // in case of an error stick to old behavior
		}

		return lookup->second;
	}

	CString GetAdminDirConcat(const CString& path, const CString& subpath)
	{
		CString result(GetAdminDir(path));
		result += subpath;
		return result;
	}

	CString GetWorkingCopy(const CString &gitDir)
	{
		CString path(gitDir);
		path.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		auto lookup = m_reverseLookup.find(path);
		if (lookup == m_reverseLookup.cend())
			return gitDir;
		return lookup->second;
	}
};
