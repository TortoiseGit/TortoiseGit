// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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

#include "stdafx.h"
#include "Git.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "gitindex.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "SmartHandle.h"
#include "git2/sys/repository.h"

CGitAdminDirMap g_AdminDirMap;

static CString GetProgramDataGitConfig()
{
	if (!((CRegDWORD(L"Software\\TortoiseGit\\CygwinHack", FALSE) == TRUE) || (CRegDWORD(L"Software\\TortoiseGit\\Msys2Hack", FALSE) == TRUE)))
	{
		CString programdataConfig;
		if (SHGetFolderPath(nullptr, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, CStrBuf(programdataConfig, MAX_PATH)) == S_OK && programdataConfig.GetLength() < MAX_PATH - static_cast<int>(wcslen(L"\\Git\\config")))
			return programdataConfig + L"\\Git\\config";
	}
	return L"";
}

int CGitIndex::Print()
{
	wprintf(L"0x%08X  0x%08X %s %s\n",
		static_cast<int>(this->m_ModifyTime),
		this->m_Flags,
		static_cast<LPCTSTR>(this->m_IndexHash.ToString()),
		static_cast<LPCTSTR>(this->m_FileName));

	return 0;
}

CGitIndexList::CGitIndexList()
: m_bHasConflicts(FALSE)
, m_LastModifyTime(0)
, m_LastFileSize(-1)
, m_iIndexCaps(GIT_INDEX_CAPABILITY_IGNORE_CASE | GIT_INDEX_CAPABILITY_NO_SYMLINKS)
{
	m_iMaxCheckSize = static_cast<__int64>(CRegDWORD(L"Software\\TortoiseGit\\TGitCacheCheckContentMaxSize", 10 * 1024)) * 1024; // stored in KiB
}

CGitIndexList::~CGitIndexList()
{
}

int CGitIndexList::ReadIndex(CString dgitdir)
{
#ifdef GTEST_INCLUDE_GTEST_GTEST_H_
	clear(); // HACK to make tests work, until we use CGitIndexList
#endif
	ATLASSERT(empty());

	CAutoRepository repository(dgitdir);
	if (!repository)
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Could not open git repository in %s: %s\n", static_cast<LPCTSTR>(dgitdir), static_cast<LPCTSTR>(CGit::GetLibGit2LastErr()));
		return -1;
	}

	// add config files
	config.New();

	CString projectConfig = g_AdminDirMap.GetAdminDir(dgitdir) + L"config";
	CString globalConfig = g_Git.GetGitGlobalConfig();
	CString globalXDGConfig = g_Git.GetGitGlobalXDGConfig();
	CString systemConfig(CRegString(REG_SYSTEM_GITCONFIGPATH, L"", FALSE));
	CString programDataConfig(GetProgramDataGitConfig());

	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(projectConfig), GIT_CONFIG_LEVEL_LOCAL, repository, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalConfig), GIT_CONFIG_LEVEL_GLOBAL, repository, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalXDGConfig), GIT_CONFIG_LEVEL_XDG, repository, FALSE);
	if (!systemConfig.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(systemConfig), GIT_CONFIG_LEVEL_SYSTEM, repository, FALSE);
	if (!programDataConfig.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(programDataConfig), GIT_CONFIG_LEVEL_PROGRAMDATA, repository, FALSE);

	git_repository_set_config(repository, config);

	CGit::GetFileModifyTime(g_AdminDirMap.GetWorktreeAdminDir(dgitdir) + L"index", &m_LastModifyTime, nullptr, &m_LastFileSize);

	CAutoIndex index;
	// load index in order to enumerate files
	if (git_repository_index(index.GetPointer(), repository))
	{
		config.Free();
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Could not get index of git repository in %s: %s\n", static_cast<LPCTSTR>(dgitdir), static_cast<LPCTSTR>(CGit::GetLibGit2LastErr()));
		return -1;
	}

	m_bHasConflicts = FALSE;
	m_iIndexCaps = git_index_caps(index);
	if (CRegDWORD(L"Software\\TortoiseGit\\OverlaysCaseSensitive", TRUE) != FALSE)
		m_iIndexCaps &= ~GIT_INDEX_CAPABILITY_IGNORE_CASE;

	size_t ecount = git_index_entrycount(index);
	try
	{
		resize(ecount);
	}
	catch (const std::bad_alloc& ex)
	{
		config.Free();
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Could not resize index-vector: %s\n", ex.what());
		return -1;
	}
	for (size_t i = 0; i < ecount; ++i)
	{
		const git_index_entry *e = git_index_get_byindex(index, i);

		auto& item = (*this)[i];
		item.m_FileName = CUnicodeUtils::GetUnicode(e->path);
		if (e->mode & S_IFDIR)
			item.m_FileName += L'/';
		item.m_ModifyTime = e->mtime.seconds;
		item.m_Flags = e->flags;
		item.m_FlagsExtended = e->flags_extended;
		item.m_IndexHash = e->id;
		item.m_Size = e->file_size;
		item.m_Mode = e->mode;
		m_bHasConflicts |= GIT_INDEX_ENTRY_STAGE(e);
	}

	DoSortFilenametSortVector(*this, IsIgnoreCase());

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Reloaded index for repo: %s\n", static_cast<LPCTSTR>(dgitdir));

	return 0;
}

int CGitIndexList::GetFileStatus(const CString& gitdir, const CString& pathorg, git_wc_status2_t& status, __int64 time, __int64 filesize, bool isSymlink, CGitHash* pHash)
{
	size_t index = SearchInSortVector(*this, pathorg, -1, IsIgnoreCase());

	if (index == NPOS)
	{
		status.status = git_wc_status_unversioned;
		if (pHash)
			pHash->Empty();

		return 0;
	}

	auto& entry = (*this)[index];
	if (pHash)
		*pHash = entry.m_IndexHash;
	ATLASSERT(IsIgnoreCase() ? pathorg.CompareNoCase(entry.m_FileName) == 0 : pathorg.Compare(entry.m_FileName) == 0);
	CAutoRepository repository;
	return GetFileStatus(repository, gitdir, entry, status, time, filesize, isSymlink);
}

int CGitIndexList::GetFileStatus(CAutoRepository& repository, const CString& gitdir, CGitIndex& entry, git_wc_status2_t& status, __int64 time, __int64 filesize, bool isSymlink)
{
	ATLASSERT(!status.assumeValid && !status.skipWorktree);

	// skip-worktree has higher priority than assume-valid
	if (entry.m_FlagsExtended & GIT_INDEX_ENTRY_SKIP_WORKTREE)
	{
		status.status = git_wc_status_normal;
		status.skipWorktree = true;
	}
	else if (entry.m_Flags & GIT_INDEX_ENTRY_VALID)
	{
		status.status = git_wc_status_normal;
		status.assumeValid = true;
	}
	else if (filesize == -1)
		status.status = git_wc_status_deleted;
	else if ((isSymlink && !S_ISLNK(entry.m_Mode)) || ((m_iIndexCaps & GIT_INDEX_CAPABILITY_NO_SYMLINKS) != GIT_INDEX_CAPABILITY_NO_SYMLINKS && isSymlink != S_ISLNK(entry.m_Mode)))
		status.status = git_wc_status_modified;
	else if (!isSymlink && filesize != entry.m_Size)
		status.status = git_wc_status_modified;
	else if (CGit::filetime_to_time_t(time) == entry.m_ModifyTime)
		status.status = git_wc_status_normal;
	else if (config && filesize < m_iMaxCheckSize)
	{
		/*
		 * Opening a new repository each time is not yet optimal, however, there is no API to clear the pack-cache
		 * When a shared repository is used, we might need a mutex to prevent concurrent access to repository instance and especially filter-lists
		 */
		if (!repository)
		{
			if (repository.Open(gitdir))
			{
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Could not open git repository in %s for checking file: %s\n", static_cast<LPCTSTR>(gitdir), static_cast<LPCTSTR>(CGit::GetLibGit2LastErr()));
				return -1;
			}
			git_repository_set_config(repository, config);
		}

		git_oid actual;
		CStringA fileA = CUnicodeUtils::GetMulti(entry.m_FileName, CP_UTF8);
		if (isSymlink && S_ISLNK(entry.m_Mode))
		{
			CStringA linkDestination;
			if (!CPathUtils::ReadLink(CombinePath(gitdir, entry.m_FileName), &linkDestination) && !git_odb_hash(&actual, static_cast<LPCSTR>(linkDestination), linkDestination.GetLength(), GIT_OBJECT_BLOB) && !git_oid_cmp(&actual, entry.m_IndexHash))
			{
				entry.m_ModifyTime = time;
				status.status = git_wc_status_normal;
			}
			else
				status.status = git_wc_status_modified;
		}
		else if (!git_repository_hashfile(&actual, repository, fileA, GIT_OBJECT_BLOB, nullptr) && !git_oid_cmp(&actual, entry.m_IndexHash))
		{
			entry.m_ModifyTime = time;
			status.status = git_wc_status_normal;
		}
		else
			status.status = git_wc_status_modified;
	}
	else
		status.status = git_wc_status_modified;

	if (entry.m_Flags & GIT_INDEX_ENTRY_STAGEMASK)
		status.status = git_wc_status_conflicted;
	else if (entry.m_FlagsExtended & GIT_INDEX_ENTRY_INTENT_TO_ADD)
		status.status = git_wc_status_added;

	return 0;
}

int CGitIndexList::GetFileStatus(const CString& gitdir, const CString& path, git_wc_status2_t& status, CGitHash* pHash)
{
	ATLASSERT(!status.assumeValid && !status.skipWorktree);

	__int64 time, filesize = 0;
	bool isDir = false;
	bool isSymlink = false;

	int result;
	if (path.IsEmpty())
		result = CGit::GetFileModifyTime(gitdir, &time, &isDir);
	else
		result = CGit::GetFileModifyTime(CombinePath(gitdir, path), &time, &isDir, &filesize, &isSymlink);

	if (result)
		filesize = -1;

	if (!isDir || (isSymlink && (m_iIndexCaps & GIT_INDEX_CAPABILITY_NO_SYMLINKS) != GIT_INDEX_CAPABILITY_NO_SYMLINKS))
		return GetFileStatus(gitdir, path, status, time, filesize, isSymlink, pHash);

	if (CStringUtils::EndsWith(path, L'/'))
	{
		size_t index = SearchInSortVector(*this, path, -1, IsIgnoreCase());
		if (index == NPOS)
		{
			status.status = git_wc_status_unversioned;
			if (pHash)
				pHash->Empty();

			return 0;
		}

		if (pHash)
			*pHash = (*this)[index].m_IndexHash;

		if (!result)
			status.status = git_wc_status_normal;
		else
			status.status = git_wc_status_deleted;
		return 0;
	}

	// we get here for symlinks which are handled as files inside the git index
	if ((m_iIndexCaps & GIT_INDEX_CAPABILITY_NO_SYMLINKS) != GIT_INDEX_CAPABILITY_NO_SYMLINKS)
		return GetFileStatus(gitdir, path, status, time, filesize, isSymlink, pHash);

	// we should never get here
	status.status = git_wc_status_unversioned;

	return -1;
}

bool CGitIndexFileMap::HasIndexChangedOnDisk(const CString& gitdir)
{
	__int64 time = -1, size = -1;

	auto pIndex = SafeGet(gitdir);

	if (!pIndex)
		return true;

	CString IndexFile = g_AdminDirMap.GetWorktreeAdminDirConcat(gitdir, L"index");
	// no need to refresh if there is no index right now and the current index is empty, but otherwise or lastmodified time differs
	return (CGit::GetFileModifyTime(IndexFile, &time, nullptr, &size) && !pIndex->empty()) || pIndex->m_LastModifyTime != time || pIndex->m_LastFileSize != size;
}

int CGitIndexFileMap::LoadIndex(const CString &gitdir)
{
	SHARED_INDEX_PTR pIndex = std::make_shared<CGitIndexList>();

	if (pIndex->ReadIndex(gitdir))
	{
		SafeClear(gitdir);
		return -1;
	}

	this->SafeSet(gitdir, pIndex);

	return 0;
}

// This method is assumed to be called with m_SharedMutex locked.
int CGitHeadFileList::GetPackRef(const CString &gitdir)
{
	CString PackRef = g_AdminDirMap.GetAdminDirConcat(gitdir, L"packed-refs");

	__int64 mtime = 0, packsize = -1;
	if (CGit::GetFileModifyTime(PackRef, &mtime, nullptr, &packsize))
	{
		//packed refs is not existed
		this->m_PackRefFile.Empty();
		this->m_PackRefMap.clear();
		return 0;
	}
	else if (mtime == m_LastModifyTimePackRef && packsize == m_LastFileSizePackRef)
		return 0;
	else
	{
		this->m_PackRefFile = PackRef;
		this->m_LastModifyTimePackRef = mtime;
		this->m_LastFileSizePackRef = packsize;
	}

	m_PackRefMap.clear();

	CAutoFile hfile = CreateFile(PackRef,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (!hfile)
		return -1;

	DWORD filesize = GetFileSize(hfile, nullptr);
	if (filesize == 0 || filesize == INVALID_FILE_SIZE)
		return -1;

	DWORD size = 0;
	auto buff = std::make_unique<char[]>(filesize);
	ReadFile(hfile, buff.get(), filesize, &size, nullptr);

	if (size != filesize)
		return -1;

	for (DWORD i = 0; i < filesize;)
	{
		CString hash;
		CString ref;
		if (buff[i] == '#' || buff[i] == '^')
		{
			while (buff[i] != '\n')
			{
				++i;
				if (i == filesize)
					break;
			}
			++i;
		}

		if (i >= filesize)
			break;

		while (buff[i] != ' ')
		{
			hash.AppendChar(buff[i]);
			++i;
			if (i == filesize)
				break;
		}

		++i;
		if (i >= filesize)
			break;

		while (buff[i] != '\n')
		{
			ref.AppendChar(buff[i]);
			++i;
			if (i == filesize)
				break;
		}

		if (!ref.IsEmpty())
			m_PackRefMap[ref] = CGitHash::FromHexStrTry(hash);

		while (buff[i] == '\n')
		{
			++i;
			if (i == filesize)
				break;
		}
	}
	return 0;
}
int CGitHeadFileList::ReadHeadHash(const CString& gitdir)
{
	ATLASSERT(m_Gitdir.IsEmpty() && m_HeadFile.IsEmpty() && m_Head.IsEmpty());

	m_Gitdir = g_AdminDirMap.GetWorktreeAdminDir(gitdir);

	m_HeadFile = m_Gitdir;
	m_HeadFile += L"HEAD";

	if (CGit::GetFileModifyTime(m_HeadFile, &m_LastModifyTimeHead, nullptr, &m_LastFileSizeHead))
		return -1;

	CAutoFile hfile = CreateFile(m_HeadFile,
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr);

	if (!hfile)
		return -1;

	DWORD size = 0;
	unsigned char buffer[2 * GIT_HASH_SIZE];
	ReadFile(hfile, buffer, static_cast<DWORD>(strlen("ref:")), &size, nullptr);
	if (size != strlen("ref:"))
		return -1;
	buffer[4] = '\0';
	if (strcmp(reinterpret_cast<const char*>(buffer), "ref:") == 0)
	{
		m_HeadRefFile.Empty();
		DWORD filesize = GetFileSize(hfile, nullptr);
		if (filesize < 5 || filesize == INVALID_FILE_SIZE)
			return -1;

		auto p = static_cast<unsigned char*>(malloc(filesize - strlen("ref:")));
		if (!p)
			return -1;

		ReadFile(hfile, p, filesize - static_cast<DWORD>(strlen("ref:")), &size, nullptr);
		CGit::StringAppend(&m_HeadRefFile, p, CP_UTF8, filesize - static_cast<int>(strlen("ref:")));
		free(p);

		CString ref = m_HeadRefFile.Trim();
		int start = 0;
		ref = ref.Tokenize(L"\n", start);
		m_HeadRefFile = g_AdminDirMap.GetAdminDir(gitdir) + m_HeadRefFile;
		m_HeadRefFile.Replace(L'/', L'\\');

		__int64 time;
		if (CGit::GetFileModifyTime(m_HeadRefFile, &time, nullptr))
		{
			if (GetPackRef(gitdir))
				return -1;
			if (m_PackRefMap.find(ref) != m_PackRefMap.end())
			{
				m_bRefFromPackRefFile = true;
				m_Head = m_PackRefMap[ref];
				return 0;
			}

			// unborn branch
			m_Head.Empty();

			return 0;
		}

		CAutoFile href = CreateFile(m_HeadRefFile,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

		if (!href)
		{
			if (GetPackRef(gitdir))
				return -1;

			if (m_PackRefMap.find(ref) == m_PackRefMap.end())
				return -1;

			m_bRefFromPackRefFile = true;
			m_Head = m_PackRefMap[ref];
			return 0;
		}

		ReadFile(href, buffer, 2 * GIT_HASH_SIZE, &size, nullptr);
		if (size != 2 * GIT_HASH_SIZE)
			return -1;

		m_Head = CGitHash::FromHexStr(reinterpret_cast<const char*>(buffer));

		m_LastModifyTimeRef = time;

		return 0;
	}

	ReadFile(hfile, buffer + static_cast<DWORD>(strlen("ref:")), 2 * GIT_HASH_SIZE - static_cast<DWORD>(strlen("ref:")), &size, nullptr);
	if (size != 2 * GIT_HASH_SIZE - static_cast<DWORD>(strlen("ref:")))
		return -1;

	m_HeadRefFile.Empty();

	m_Head = CGitHash::FromHexStr(reinterpret_cast<const char*>(buffer));

	return 0;
}

bool CGitHeadFileList::CheckHeadUpdate()
{
	if (this->m_HeadFile.IsEmpty())
		return true;

	__int64 mtime = 0, size = -1;

	if (CGit::GetFileModifyTime(m_HeadFile, &mtime, nullptr, &size))
		return true;

	if (mtime != m_LastModifyTimeHead || size != m_LastFileSizeHead)
		return true;

	if (!this->m_HeadRefFile.IsEmpty())
	{
		// we need to check for the HEAD ref file here, because the original ref might have come from packedrefs and now is a ref-file
		if (CGit::GetFileModifyTime(m_HeadRefFile, &mtime))
		{
			if (!m_bRefFromPackRefFile)
				return true;

		} else if (mtime != this->m_LastModifyTimeRef)
			return true;
	}

	if (m_bRefFromPackRefFile && !m_PackRefFile.IsEmpty())
	{
		size = -1;
		if (CGit::GetFileModifyTime(m_PackRefFile, &mtime, nullptr, &size))
			return true;

		if (mtime != m_LastModifyTimePackRef || size != m_LastFileSizePackRef)
			return true;
	}

	// in an empty repo HEAD points to refs/heads/master, but this ref doesn't exist.
	// So we need to retry again and again until the ref exists - otherwise we will never notice
	if (this->m_Head.IsEmpty() && this->m_HeadRefFile.IsEmpty() && this->m_PackRefFile.IsEmpty())
		return true;

	return false;
}

int CGitHeadFileList::ReadTreeRecursive(git_repository& repo, const git_tree* tree, const CStringA& base)
{
#define S_IFGITLINK	0160000
	size_t count = git_tree_entrycount(tree);
	for (size_t i = 0; i < count; ++i)
	{
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		if (!entry)
			continue;
		int mode = git_tree_entry_filemode(entry);
		bool isDir = (mode & S_IFDIR) == S_IFDIR;
		bool isSubmodule = (mode & S_IFMT) == S_IFGITLINK;
		if (!isDir || isSubmodule)
		{
			CGitTreeItem item;
			item.m_Hash = git_tree_entry_id(entry);
			CGit::StringAppend(&item.m_FileName, base, CP_UTF8, base.GetLength());
			CGit::StringAppend(&item.m_FileName, git_tree_entry_name(entry), CP_UTF8);
			if (isSubmodule)
				item.m_FileName += L'/';
			push_back(item);
			continue;
		}

		CAutoObject object;
		git_tree_entry_to_object(object.GetPointer(), &repo, entry);
		if (!object)
			continue;
		CStringA parent = base;
		parent += git_tree_entry_name(entry);
		parent += "/";
		ReadTreeRecursive(repo, reinterpret_cast<git_tree*>(static_cast<git_object*>(object)), parent);
	}

	return 0;
}

// ReadTree is/must only be executed on an empty list
int CGitHeadFileList::ReadTree(bool ignoreCase)
{
	ATLASSERT(empty());

	// unborn branch
	if (m_Head.IsEmpty())
		return 0;

	CAutoRepository repository(m_Gitdir);
	CAutoCommit commit;
	CAutoTree tree;
	bool ret = repository;
	ret = ret && !git_commit_lookup(commit.GetPointer(), repository, m_Head);
	ret = ret && !git_commit_tree(tree.GetPointer(), commit);
	try
	{
		ret = ret && !ReadTreeRecursive(*repository, tree, "");
	}
	catch (const std::bad_alloc& ex)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Catched exception inside ReadTreeRecursive: %s\n", ex.what());
		return -1;
	}
	if (!ret)
	{
		clear();
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Could not open git repository in %s and read HEAD commit %s: %s\n", static_cast<LPCTSTR>(m_Gitdir), static_cast<LPCTSTR>(m_Head.ToString()), static_cast<LPCTSTR>(CGit::GetLibGit2LastErr()));
		m_LastModifyTimeHead = 0;
		m_LastFileSizeHead = -1;
		return -1;
	}

	DoSortFilenametSortVector(*this, ignoreCase);

	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Reloaded HEAD tree (commit is %s) for repo: %s\n", static_cast<LPCTSTR>(m_Head.ToString()), static_cast<LPCTSTR>(m_Gitdir));

	return 0;
}
int CGitIgnoreItem::FetchIgnoreList(const CString& projectroot, const CString& file, bool isGlobal, int* ignoreCase)
{
	if (this->m_pExcludeList)
	{
		git_free_exclude_list(m_pExcludeList);
		m_pExcludeList = nullptr;
	}
	free(m_buffer);
	m_buffer = nullptr;

	this->m_BaseDir.Empty();
	if (!isGlobal)
	{
		CString base = file.Mid(projectroot.GetLength() + 1);
		base.Replace(L'\\', L'/');

		int start = base.ReverseFind(L'/');
		if(start > 0)
		{
			base.Truncate(start);
			this->m_BaseDir = CUnicodeUtils::GetMulti(base, CP_UTF8) + "/";
		}
	}

	if (CGit::GetFileModifyTime(file, &m_LastModifyTime, nullptr, &m_LastFileSize))
		return -1;

	CAutoFile hfile = CreateFile(file,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE,
			nullptr,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			nullptr);

	if (!hfile)
		return -1 ;

	DWORD filesize = GetFileSize(hfile, nullptr);
	if (filesize == INVALID_FILE_SIZE)
		return -1;

	m_buffer = new BYTE[filesize + 1];
	if (!m_buffer)
		return -1;

	DWORD size = 0;
	if (!ReadFile(hfile, m_buffer, filesize, &size, nullptr))
	{
		free(m_buffer);
		m_buffer = nullptr;
		return -1;
	}
	m_buffer[size] = '\0';

	if (git_create_exclude_list(&m_pExcludeList))
	{
		free(m_buffer);
		m_buffer = nullptr;
		return -1;
	}

	m_iIgnoreCase = ignoreCase;

	BYTE *p = m_buffer;
	int line = 0;
	for (DWORD i = 0; i < size; ++i)
	{
		if (m_buffer[i] == '\n' || m_buffer[i] == '\r' || i == (size - 1))
		{
			if (m_buffer[i] == '\n' || m_buffer[i] == '\r')
				m_buffer[i] = '\0';

			if (p[0] != '#' && p[0])
				git_add_exclude(reinterpret_cast<const char*>(p), this->m_BaseDir, m_BaseDir.GetLength(), this->m_pExcludeList, ++line);

			p = m_buffer + i + 1;
		}
	}

	if (!line)
	{
		git_free_exclude_list(m_pExcludeList);
		m_pExcludeList = nullptr;
		free(m_buffer);
		m_buffer = nullptr;
	}

	return 0;
}

#ifdef GTEST_INCLUDE_GTEST_GTEST_H_
int CGitIgnoreItem::IsPathIgnored(const CStringA& patha, int& type)
{
	int pos = patha.ReverseFind('/');
	const char* base = (pos >= 0) ? (static_cast<const char*>(patha) + pos + 1) : patha;

	return IsPathIgnored(patha, base, type);
}
#endif

int CGitIgnoreItem::IsPathIgnored(const CStringA& patha, const char* base, int& type)
{
	if (!m_pExcludeList)
		return -1; // error or undecided

	return git_check_excluded_1(patha, patha.GetLength(), base, &type, m_pExcludeList, m_iIgnoreCase ? *m_iIgnoreCase : 1);
}

bool CGitIgnoreList::CheckFileChanged(const CString &path)
{
	__int64 time = 0, size = -1;

	int ret = CGit::GetFileModifyTime(path, &time, nullptr, &size);

	bool cacheExist;
	{
		CAutoReadLock lock(m_SharedMutex);
		cacheExist = (m_Map.find(path) != m_Map.end());
	}

	if (!cacheExist && ret == 0)
	{
		CAutoWriteLock lock(m_SharedMutex);
		m_Map[path].m_LastModifyTime = 0;
		m_Map[path].m_LastFileSize = -1;
	}
	// both cache and file is not exist
	if ((ret != 0) && (!cacheExist))
		return false;

	// file exist but cache miss
	if ((ret == 0) && (!cacheExist))
		return true;

	// file not exist but cache exist
	if ((ret != 0) && (cacheExist))
		return true;
	// file exist and cache exist

	{
		CAutoReadLock lock(m_SharedMutex);
		if (m_Map[path].m_LastModifyTime == time && m_Map[path].m_LastFileSize == size)
			return false;
	}
	return true;
}

int CGitIgnoreList::FetchIgnoreFile(const CString &gitdir, const CString &gitignore, bool isGlobal)
{
	if (CGit::GitPathFileExists(gitignore)) //if .gitignore remove, we need remote cache
	{
		CAutoWriteLock lock(m_SharedMutex);
		m_Map[gitignore].FetchIgnoreList(gitdir, gitignore, isGlobal, &m_IgnoreCase[g_AdminDirMap.GetAdminDir(gitdir)]);
	}
	else
	{
		CAutoWriteLock lock(m_SharedMutex);
		m_Map.erase(gitignore);
	}
	return 0;
}

bool CGitIgnoreList::CheckAndUpdateIgnoreFiles(const CString& gitdir, const CString& path, bool isDir, std::set<CString>* lastChecked)
{
	CString temp(gitdir);
	temp += L'\\';
	temp += path;

	temp.Replace(L'/', L'\\');

	if (!isDir)
	{
		int x = temp.ReverseFind(L'\\');
		if (x >= 2)
			temp.Truncate(x);
	}

	bool updated = false;
	while (!temp.IsEmpty())
	{
		if (lastChecked)
		{
			if (lastChecked->find(temp) != lastChecked->end())
				return updated;
			lastChecked->insert(temp);
		}

		temp += L"\\.gitignore";

		if (CheckFileChanged(temp))
		{
			FetchIgnoreFile(gitdir, temp, false);
			updated = true;
		}

		temp.Truncate(temp.GetLength() - static_cast<int>(wcslen(L"\\.gitignore")));
		if (CPathUtils::ArePathStringsEqual(temp, gitdir))
		{
			CString adminDir = g_AdminDirMap.GetAdminDir(temp);
			CString wcglobalgitignore = adminDir + L"info\\exclude";
			if (CheckFileChanged(wcglobalgitignore))
			{
				FetchIgnoreFile(gitdir, wcglobalgitignore, true);
				updated = true;
			}

			if (CheckAndUpdateCoreExcludefile(adminDir))
			{
				CString excludesFile;
				{
					CAutoReadLock lock(m_SharedMutex);
					excludesFile = m_CoreExcludesfiles[adminDir];
				}
				if (!excludesFile.IsEmpty())
				{
					FetchIgnoreFile(gitdir, excludesFile, true);
					updated = true;
				}
			}

			return updated;
		}

		int i = temp.ReverseFind(L'\\');
		temp.Truncate(max(0, i));
	}
	return updated;
}

bool CGitIgnoreList::CheckAndUpdateGitSystemConfigPath(bool force)
{
	if (force)
		m_sGitProgramDataConfigPath = GetProgramDataGitConfig();
	// recheck every 30 seconds
	if (GetTickCount64() - m_dGitSystemConfigPathLastChecked > 30000UL || force)
	{
		m_dGitSystemConfigPathLastChecked = GetTickCount64();
		CString gitSystemConfigPath(CRegString(REG_SYSTEM_GITCONFIGPATH, L"", FALSE));
		if (gitSystemConfigPath != m_sGitSystemConfigPath)
		{
			m_sGitSystemConfigPath = gitSystemConfigPath;
			return true;
		}
	}
	return false;
}
bool CGitIgnoreList::CheckAndUpdateCoreExcludefile(const CString &adminDir)
{
	CString projectConfig(adminDir);
	projectConfig += L"config";
	CString globalConfig = g_Git.GetGitGlobalConfig();
	CString globalXDGConfig = g_Git.GetGitGlobalXDGConfig();

	CAutoWriteLock lock(m_coreExcludefilesSharedMutex);
	bool hasChanged = CheckAndUpdateGitSystemConfigPath();
	hasChanged = hasChanged || CheckFileChanged(projectConfig);
	hasChanged = hasChanged || CheckFileChanged(globalConfig);
	hasChanged = hasChanged || CheckFileChanged(globalXDGConfig);
	if (!m_sGitProgramDataConfigPath.IsEmpty())
		hasChanged = hasChanged || CheckFileChanged(m_sGitProgramDataConfigPath);
	if (!m_sGitSystemConfigPath.IsEmpty())
		hasChanged = hasChanged || CheckFileChanged(m_sGitSystemConfigPath);

	CString excludesFile;
	{
		CAutoReadLock lock2(m_SharedMutex);
		excludesFile = m_CoreExcludesfiles[adminDir];
	}
	if (!excludesFile.IsEmpty())
		hasChanged = hasChanged || CheckFileChanged(excludesFile);

	if (!hasChanged)
		return false;

	CAutoConfig config(true);
	CAutoRepository repo(adminDir);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(projectConfig), GIT_CONFIG_LEVEL_LOCAL, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalConfig), GIT_CONFIG_LEVEL_GLOBAL, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalXDGConfig), GIT_CONFIG_LEVEL_XDG, repo, FALSE);
	if (!m_sGitSystemConfigPath.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(m_sGitSystemConfigPath), GIT_CONFIG_LEVEL_SYSTEM, repo, FALSE);
	if (!m_sGitProgramDataConfigPath.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(m_sGitProgramDataConfigPath), GIT_CONFIG_LEVEL_PROGRAMDATA, repo, FALSE);

	config.GetString(L"core.excludesfile", excludesFile);
	if (excludesFile.IsEmpty())
		excludesFile = GetWindowsHome() + L"\\.config\\git\\ignore";
	else if (CStringUtils::StartsWith(excludesFile, L"~/"))
		excludesFile = GetWindowsHome() + excludesFile.Mid(static_cast<int>(wcslen(L"~")));

	CAutoWriteLock lockMap(m_SharedMutex);
	m_IgnoreCase[adminDir] = 1;
	config.GetBOOL(L"core.ignorecase", m_IgnoreCase[adminDir]);
	CGit::GetFileModifyTime(projectConfig, &m_Map[projectConfig].m_LastModifyTime, nullptr, &m_Map[projectConfig].m_LastFileSize);
	CGit::GetFileModifyTime(globalXDGConfig, &m_Map[globalXDGConfig].m_LastModifyTime, nullptr, &m_Map[globalXDGConfig].m_LastFileSize);
	if (m_Map[globalXDGConfig].m_LastModifyTime == 0)
		m_Map.erase(globalXDGConfig);
	CGit::GetFileModifyTime(globalConfig, &m_Map[globalConfig].m_LastModifyTime, nullptr, &m_Map[globalConfig].m_LastFileSize);
	if (m_Map[globalConfig].m_LastModifyTime == 0)
		m_Map.erase(globalConfig);
	if (!m_sGitSystemConfigPath.IsEmpty())
		CGit::GetFileModifyTime(m_sGitSystemConfigPath, &m_Map[m_sGitSystemConfigPath].m_LastModifyTime, nullptr, &m_Map[m_sGitSystemConfigPath].m_LastFileSize);
	if (m_Map[m_sGitSystemConfigPath].m_LastModifyTime == 0 || m_sGitSystemConfigPath.IsEmpty())
		m_Map.erase(m_sGitSystemConfigPath);
	if (!m_sGitProgramDataConfigPath.IsEmpty())
		CGit::GetFileModifyTime(m_sGitProgramDataConfigPath, &m_Map[m_sGitProgramDataConfigPath].m_LastModifyTime, nullptr, &m_Map[m_sGitProgramDataConfigPath].m_LastFileSize);
	if (m_Map[m_sGitProgramDataConfigPath].m_LastModifyTime == 0 || m_sGitProgramDataConfigPath.IsEmpty())
		m_Map.erase(m_sGitProgramDataConfigPath);
	m_CoreExcludesfiles[adminDir] = excludesFile;

	return true;
}
const CString CGitIgnoreList::GetWindowsHome()
{
	static CString sWindowsHome(g_Git.GetHomeDirectory());
	return sWindowsHome;
}
bool CGitIgnoreList::IsIgnore(CString str, const CString& projectroot, bool isDir, const CString& adminDir)
{
	str.Replace(L'\\', L'/');

	if (!str.IsEmpty() && str[str.GetLength() - 1] == L'/')
		str.Truncate(str.GetLength() - 1);

	int ret = CheckIgnore(str, projectroot, isDir, adminDir);
	while (ret < 0)
	{
		int start = str.ReverseFind(L'/');
		if(start < 0)
			return (ret == 1);

		str.Truncate(start);
		ret = CheckIgnore(str, projectroot, TRUE, adminDir);
	}

	return (ret == 1);
}
int CGitIgnoreList::CheckFileAgainstIgnoreList(const CString &ignorefile, const CStringA &patha, const char * base, int &type)
{
	if (m_Map.find(ignorefile) == m_Map.end())
		return -1; // error or undecided

	return (m_Map[ignorefile].IsPathIgnored(patha, base, type));
}
int CGitIgnoreList::CheckIgnore(const CString &path, const CString &projectroot, bool isDir, const CString& adminDir)
{
	CString temp = CombinePath(projectroot, path);
	temp.Replace(L'/', L'\\');

	CStringA patha = CUnicodeUtils::GetMulti(path, CP_UTF8);
	patha.Replace('\\', '/');

	int type = 0;
	if (isDir)
	{
		type = DT_DIR;

		// strip directory name
		// we do not need to check for a .ignore file inside a directory we might ignore
		int i = temp.ReverseFind(L'\\');
		if (i >= 0)
			temp.Truncate(i);
	}
	else
	{
		type = DT_REG;

		int x = temp.ReverseFind(L'\\');
		if (x >= 2)
			temp.Truncate(x);
	}

	int pos = patha.ReverseFind('/');
	const char* base = (pos >= 0) ? (static_cast<const char*>(patha) + pos + 1) : patha;


	CAutoReadLock lock(m_SharedMutex);
	while (!temp.IsEmpty())
	{
		temp += L"\\.gitignore";

		if (auto ret = CheckFileAgainstIgnoreList(temp, patha, base, type); ret != -1)
			return ret;

		temp.Truncate(temp.GetLength() - static_cast<int>(wcslen(L"\\.gitignore")));

		if (CPathUtils::ArePathStringsEqual(temp, projectroot))
		{
			CString wcglobalgitignore = adminDir;
			wcglobalgitignore += L"info\\exclude";
			if (auto ret = CheckFileAgainstIgnoreList(wcglobalgitignore, patha, base, type); ret != -1)
				return ret;

			CString excludesFile = m_CoreExcludesfiles[adminDir];
			if (!excludesFile.IsEmpty())
				return CheckFileAgainstIgnoreList(excludesFile, patha, base, type);

			return -1;
		}

		int i = temp.ReverseFind(L'\\');
		temp.Truncate(max(0, i));
	}

	return -1;
}

void CGitHeadFileMap::CheckHeadAndUpdate(const CString& gitdir, bool ignoreCase)
{
	SHARED_TREE_PTR ptr = this->SafeGet(gitdir);

	if (ptr.get() && !ptr->CheckHeadUpdate())
		return;

	ptr = std::make_shared<CGitHeadFileList>();
	if (ptr->ReadHeadHash(gitdir) || ptr->ReadTree(ignoreCase))
	{
		SafeClear(gitdir);
		return;
	}

	this->SafeSet(gitdir, ptr);

	return;
}
