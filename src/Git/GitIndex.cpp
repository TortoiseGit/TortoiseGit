// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016 - TortoiseGit

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
#include "TGitPath.h"
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
		if (SHGetFolderPath(nullptr, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, CStrBuf(programdataConfig, MAX_PATH)) == S_OK && programdataConfig.GetLength() < MAX_PATH - 11) /* 11 = len("\\Git\\config") */
			return programdataConfig + L"\\Git\\config";
	}
	return L"";
}

int CGitIndex::Print()
{
	_tprintf(_T("0x%08X  0x%08X %s %s\n"),
		(int)this->m_ModifyTime,
		this->m_Flags,
		(LPCTSTR)this->m_IndexHash.ToString(),
		(LPCTSTR)this->m_FileName);

	return 0;
}

CGitIndexList::CGitIndexList()
: m_bHasConflicts(FALSE)
{
	this->m_LastModifyTime = 0;
	m_critRepoSec.Init();
	m_iMaxCheckSize = (__int64)CRegDWORD(_T("Software\\TortoiseGit\\TGitCacheCheckContentMaxSize"), 10 * 1024) * 1024; // stored in KiB
}

CGitIndexList::~CGitIndexList()
{
	m_critRepoSec.Term();
}

static bool SortIndex(const CGitIndex &Item1, const CGitIndex &Item2)
{
	return Item1.m_FileName.Compare(Item2.m_FileName) < 0;
}

static bool SortTree(const CGitTreeItem &Item1, const CGitTreeItem &Item2)
{
	return Item1.m_FileName.Compare(Item2.m_FileName) < 0;
}

int CGitIndexList::ReadIndex(CString dgitdir)
{
	this->clear();

	CAutoLocker lock(m_critRepoSec);
	if (repository.Open(dgitdir))
		return -1;

	// add config files
	CAutoConfig config(true);

	CString projectConfig = dgitdir + _T("config");
	CString globalConfig = g_Git.GetGitGlobalConfig();
	CString globalXDGConfig = g_Git.GetGitGlobalXDGConfig();
	CString systemConfig(CRegString(REG_SYSTEM_GITCONFIGPATH, _T(""), FALSE));
	CString programDataConfig(GetProgramDataGitConfig());

	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(projectConfig), GIT_CONFIG_LEVEL_LOCAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalConfig), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalXDGConfig), GIT_CONFIG_LEVEL_XDG, FALSE);
	if (!systemConfig.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(systemConfig), GIT_CONFIG_LEVEL_SYSTEM, FALSE);
	if (!programDataConfig.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(programDataConfig), GIT_CONFIG_LEVEL_PROGRAMDATA, FALSE);

	git_repository_set_config(repository, config);

	CAutoIndex index;
	// load index in order to enumerate files
	if (git_repository_index(index.GetPointer(), repository))
	{
		repository.Free();
		return -1;
	}

	m_bHasConflicts = FALSE;

	size_t ecount = git_index_entrycount(index);
	try
	{
		resize(ecount);
	}
	catch (const std::bad_alloc& ex)
	{
		repository.Free();
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Could not resize index-vector: %s\n", ex.what());
		return -1;
	}
	for (size_t i = 0; i < ecount; ++i)
	{
		const git_index_entry *e = git_index_get_byindex(index, i);

		this->at(i).m_FileName = CUnicodeUtils::GetUnicode(e->path);
		this->at(i).m_FileName.MakeLower();
		this->at(i).m_ModifyTime = e->mtime.seconds;
		this->at(i).m_Flags = e->flags;
		this->at(i).m_FlagsExtended = e->flags_extended;
		this->at(i).m_IndexHash = e->id.id;
		this->at(i).m_Size = e->file_size;
		m_bHasConflicts |= GIT_IDXENTRY_STAGE(e);
	}

	CGit::GetFileModifyTime(dgitdir + _T("index"), &this->m_LastModifyTime);
	std::sort(this->begin(), this->end(), SortIndex);

	return 0;
}

int CGitIndexList::GetFileStatus(const CString &gitdir, const CString &pathorg, git_wc_status_kind *status, __int64 time, __int64 filesize, FILL_STATUS_CALLBACK callback, void *pData, CGitHash *pHash, bool * assumeValid, bool * skipWorktree)
{
	if (!status)
		return 0;

	CString path = pathorg;
	path.MakeLower();

	int index = SearchInSortVector(*this, path, -1);

	if (index < 0)
	{
		*status = git_wc_status_unversioned;
		if (pHash)
			pHash->Empty();

		if (callback && assumeValid && skipWorktree)
			callback(CombinePath(gitdir, pathorg), *status, false, pData, *assumeValid, *skipWorktree);

		return 0;
	}

	// skip-worktree has higher priority than assume-valid
	if (at(index).m_FlagsExtended & GIT_IDXENTRY_SKIP_WORKTREE)
	{
		*status = git_wc_status_normal;
		if (skipWorktree)
			*skipWorktree = true;
	}
	else if (at(index).m_Flags & GIT_IDXENTRY_VALID)
	{
		*status = git_wc_status_normal;
		if (assumeValid)
			*assumeValid = true;
	}
	else if (filesize != at(index).m_Size)
		*status = git_wc_status_modified;
	else if (time == at(index).m_ModifyTime)
		*status = git_wc_status_normal;
	else if (repository && filesize < m_iMaxCheckSize)
	{
		git_oid actual;
		CStringA fileA = CUnicodeUtils::GetMulti(pathorg, CP_UTF8);
		m_critRepoSec.Lock(); // prevent concurrent access to repository instance and especially filter-lists
		if (!git_repository_hashfile(&actual, repository, fileA, GIT_OBJ_BLOB, nullptr) && !git_oid_cmp(&actual, (const git_oid*)at(index).m_IndexHash.m_hash))
		{
			at(index).m_ModifyTime = time;
			*status = git_wc_status_normal;
		}
		else
			*status = git_wc_status_modified;
		m_critRepoSec.Unlock();
	}
	else
		*status = git_wc_status_modified;

	if (at(index).m_Flags & GIT_IDXENTRY_STAGEMASK)
		*status = git_wc_status_conflicted;
	else if (at(index).m_FlagsExtended & GIT_IDXENTRY_INTENT_TO_ADD)
		*status = git_wc_status_added;

	if (pHash)
		*pHash = at(index).m_IndexHash;

	if (callback && assumeValid && skipWorktree)
		callback(CombinePath(gitdir, pathorg), *status, false, pData, *assumeValid, *skipWorktree);

	return 0;
}

int CGitIndexList::GetStatus(const CString& gitdir, CString path, git_wc_status_kind* status,
							 BOOL IsFull, BOOL /*IsRecursive*/,
							 FILL_STATUS_CALLBACK callback, void *pData,
							 CGitHash *pHash, bool * assumeValid, bool * skipWorktree)
{
	__int64 time, filesize = 0;
	bool isDir = false;

	if (!status)
		return 0;

	git_wc_status_kind dirstatus = git_wc_status_none;
	int result;
	if (path.IsEmpty())
		result = CGit::GetFileModifyTime(gitdir, &time, &isDir);
	else
		result = CGit::GetFileModifyTime(CombinePath(gitdir, path), &time, &isDir, &filesize);

	if (result)
	{
		*status = git_wc_status_deleted;
		if (callback && assumeValid && skipWorktree)
			callback(CombinePath(gitdir, path), git_wc_status_deleted, false, pData, *assumeValid, *skipWorktree);

		return 0;
	}

	if (!isDir)
	{
		GetFileStatus(gitdir, path, status, time, filesize, callback, pData, pHash, assumeValid, skipWorktree);
		return 0;
	}

	if (!path.IsEmpty() && path.Right(1) != _T('\\'))
		path += _T('\\');

	int len = path.GetLength();

	for (auto it = cbegin(), itend = cend(); it != itend; ++it)
	{
		if (!((*it).m_FileName.GetLength() > len && wcsncmp((*it).m_FileName, path, len) == 0))
			continue;

		if (!IsFull)
		{
			*status = git_wc_status_normal;
			if (callback)
				callback(CombinePath(gitdir, path), *status, false, pData, ((*it).m_Flags & GIT_IDXENTRY_VALID) && !((*it).m_FlagsExtended & GIT_IDXENTRY_SKIP_WORKTREE), ((*it).m_FlagsExtended & GIT_IDXENTRY_SKIP_WORKTREE) != 0);

			return 0;
		}

		result = CGit::GetFileModifyTime(CombinePath(gitdir, (*it).m_FileName), &time, nullptr, &filesize);
		if (result)
			continue;

		*status = git_wc_status_none;
		if (assumeValid)
			*assumeValid = false;
		if (skipWorktree)
			*skipWorktree = false;

		GetFileStatus(gitdir, (*it).m_FileName, status, time, filesize, callback, pData, nullptr, assumeValid, skipWorktree);

		// if a file is assumed valid, we need to inform the caller, otherwise the assumevalid flag might not get to the explorer on first open of a repository
		if (callback && assumeValid && skipWorktree && (*assumeValid || *skipWorktree))
			callback(CombinePath(gitdir, path), *status, false, pData, *assumeValid, *skipWorktree);
		if (*status != git_wc_status_none)
		{
			if (dirstatus == git_wc_status_none)
				dirstatus = git_wc_status_normal;
			if (*status != git_wc_status_normal)
				dirstatus = git_wc_status_modified;
		}
	} /* End For */

	if (dirstatus != git_wc_status_none)
		*status = dirstatus;
	else
		*status = git_wc_status_unversioned;

	if (callback)
		callback(CombinePath(gitdir, path), *status, false, pData, false, false);

	return 0;
}

int CGitIndexFileMap::Check(const CString &gitdir, bool *isChanged)
{
	__int64 time;

	CString IndexFile = g_AdminDirMap.GetAdminDirConcat(gitdir, _T("index"));

	if (CGit::GetFileModifyTime(IndexFile, &time))
		return -1;

	SHARED_INDEX_PTR pIndex;
	pIndex = this->SafeGet(gitdir);

	if (!pIndex)
	{
		if(isChanged)
			*isChanged = true;
		return 0;
	}

	if (pIndex->m_LastModifyTime == time)
	{
		if (isChanged)
			*isChanged = false;
	}
	else
	{
		if (isChanged)
			*isChanged = true;
	}
	return 0;
}

int CGitIndexFileMap::LoadIndex(const CString &gitdir)
{
	SHARED_INDEX_PTR pIndex(new CGitIndexList);

	if (pIndex->ReadIndex(g_AdminDirMap.GetAdminDir(gitdir)))
		return -1;

	this->SafeSet(gitdir, pIndex);

	return 0;
}

int CGitIndexFileMap::GetFileStatus(const CString &gitdir, const CString &path, git_wc_status_kind *status,BOOL IsFull, BOOL IsRecursive,
									FILL_STATUS_CALLBACK callback, void *pData,
									CGitHash *pHash,
									bool isLoadUpdatedIndex, bool * assumeValid, bool * skipWorktree)
{
	CheckAndUpdate(gitdir, isLoadUpdatedIndex);

	SHARED_INDEX_PTR pIndex = this->SafeGet(gitdir);
	if (pIndex)
		pIndex->GetStatus(gitdir, path, status, IsFull, IsRecursive, callback, pData, pHash, assumeValid, skipWorktree);
	else
	{
		// git working tree has not index
		*status = git_wc_status_unversioned;
	}

	return 0;
}

int CGitIndexFileMap::IsUnderVersionControl(const CString& gitdir, CString subpath, bool isDir, bool* isVersion, bool isLoadUpdateIndex)
{
	if (subpath.IsEmpty())
	{
		*isVersion = true;
		return 0;
	}

	subpath.Replace(_T('\\'), _T('/'));
	if (isDir)
		subpath += _T('/');

	subpath.MakeLower();

	CheckAndUpdate(gitdir, isLoadUpdateIndex);

	SHARED_INDEX_PTR pIndex = this->SafeGet(gitdir);

	if (pIndex)
	{
		if (isDir)
			*isVersion = (SearchInSortVector(*pIndex, subpath, subpath.GetLength()) >= 0);
		else
			*isVersion = (SearchInSortVector(*pIndex, subpath, -1) >= 0);
	}

	return 0;
}

// This method is assumed to be called with m_SharedMutex locked.
int CGitHeadFileList::GetPackRef(const CString &gitdir)
{
	CString PackRef = g_AdminDirMap.GetAdminDirConcat(gitdir, _T("packed-refs"));

	__int64 mtime;
	if (CGit::GetFileModifyTime(PackRef, &mtime))
	{
		//packed refs is not existed
		this->m_PackRefFile.Empty();
		this->m_PackRefMap.clear();
		return 0;
	}
	else if(mtime == m_LastModifyTimePackRef)
		return 0;
	else
	{
		this->m_PackRefFile = PackRef;
		this->m_LastModifyTimePackRef = mtime;
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
	if (filesize == 0)
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
			m_PackRefMap[ref] = hash;

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
	CAutoWriteLock lock(m_SharedMutex);
	m_Gitdir = g_AdminDirMap.GetAdminDir(gitdir);

	m_HeadFile = m_Gitdir;
	m_HeadFile += _T("HEAD");

	if( CGit::GetFileModifyTime(m_HeadFile, &m_LastModifyTimeHead))
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
	unsigned char buffer[40];
	ReadFile(hfile, buffer, 4, &size, nullptr);
	if (size != 4)
		return -1;
	buffer[4] = 0;
	if (strcmp((const char*)buffer, "ref:") == 0)
	{
		m_HeadRefFile.Empty();
		DWORD filesize = GetFileSize(hfile, nullptr);
		if (filesize < 5)
			return -1;

		unsigned char *p = (unsigned char*)malloc(filesize - 4);
		if (!p)
			return -1;

		ReadFile(hfile, p, filesize - 4, &size, nullptr);
		CGit::StringAppend(&m_HeadRefFile, p, CP_UTF8, filesize - 4);
		free(p);

		CString ref = m_HeadRefFile.Trim();
		int start = 0;
		ref = ref.Tokenize(_T("\n"), start);
		m_HeadRefFile = m_Gitdir + m_HeadRefFile;
		m_HeadRefFile.Replace(_T('/'), _T('\\'));

		__int64 time;
		if (CGit::GetFileModifyTime(m_HeadRefFile, &time, nullptr))
		{
			m_HeadRefFile.Empty();
			if (GetPackRef(gitdir))
				return -1;
			if (m_PackRefMap.find(ref) == m_PackRefMap.end())
				return -1;

			m_Head = m_PackRefMap[ref];
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
			m_HeadRefFile.Empty();

			if (GetPackRef(gitdir))
				return -1;

			if (m_PackRefMap.find(ref) == m_PackRefMap.end())
				return -1;

			m_Head = m_PackRefMap[ref];
			return 0;
		}

		ReadFile(href, buffer, 40, &size, nullptr);
		if (size != 40)
			return -1;

		m_Head.ConvertFromStrA((char*)buffer);

		m_LastModifyTimeRef = time;

		return 0;
	}

	ReadFile(hfile, buffer + 4, 40 - 4, &size, nullptr);
	if (size != 36)
		return -1;

	m_HeadRefFile.Empty();

	m_Head.ConvertFromStrA((char*)buffer);

	return 0;
}

bool CGitHeadFileList::CheckHeadUpdate()
{
	CAutoReadLock lock(m_SharedMutex);
	if (this->m_HeadFile.IsEmpty())
		return true;

	__int64 mtime=0;

	if (CGit::GetFileModifyTime(m_HeadFile, &mtime))
		return true;

	if (mtime != this->m_LastModifyTimeHead)
		return true;

	if (!this->m_HeadRefFile.IsEmpty())
	{
		if (CGit::GetFileModifyTime(m_HeadRefFile, &mtime))
			return true;

		if (mtime != this->m_LastModifyTimeRef)
			return true;
	}

	if(!this->m_PackRefFile.IsEmpty())
	{
		if (CGit::GetFileModifyTime(m_PackRefFile, &mtime))
			return true;

		if (mtime != this->m_LastModifyTimePackRef)
			return true;
	}

	// in an empty repo HEAD points to refs/heads/master, but this ref doesn't exist.
	// So we need to retry again and again until the ref exists - otherwise we will never notice
	if (this->m_Head.IsEmpty() && this->m_HeadRefFile.IsEmpty() && this->m_PackRefFile.IsEmpty())
		return true;

	return false;
}

bool CGitHeadFileList::HeadHashEqualsTreeHash()
{
	CAutoReadLock lock(m_SharedMutex);
	return (m_Head == m_TreeHash);
}

bool CGitHeadFileList::HeadFileIsEmpty()
{
	CAutoReadLock lock(m_SharedMutex);
	return m_HeadFile.IsEmpty();
}

bool CGitHeadFileList::HeadIsEmpty()
{
	CAutoReadLock lock(m_SharedMutex);
	return m_Head.IsEmpty();
}

int CGitHeadFileList::CallBack(const unsigned char *sha1, const char *base, int baselen,
		const char *pathname, unsigned mode, int /*stage*/, void *context)
{
#define S_IFGITLINK	0160000

	CGitHeadFileList* p = reinterpret_cast<CGitHeadFileList*>(context);

	if ((mode & S_IFDIR) && (mode & S_IFMT) != S_IFGITLINK)
		return READ_TREE_RECURSIVE;

	size_t cur = p->size();
	p->resize(p->size() + 1);
	p->at(cur).m_Hash = sha1;

	CGit::StringAppend(&p->at(cur).m_FileName, (BYTE*)base, CP_UTF8, baselen);
	CGit::StringAppend(&p->at(cur).m_FileName, (BYTE*)pathname, CP_UTF8);

	p->at(cur).m_FileName.MakeLower();

	//p->at(cur).m_FileName.Replace(_T('/'), _T('\\'));

	//p->m_Map[p->at(cur).m_FileName] = cur;

	if( (mode&S_IFMT) == S_IFGITLINK)
		return 0;

	return READ_TREE_RECURSIVE;
}

int ReadTreeRecursive(git_repository &repo, const git_tree * tree, const CStringA& base, int (*CallBack) (const unsigned char *, const char *, int, const char *, unsigned int, int, void *), void *data)
{
	size_t count = git_tree_entrycount(tree);
	for (size_t i = 0; i < count; ++i)
	{
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		if (!entry)
			continue;
		int mode = git_tree_entry_filemode(entry);
		if( CallBack(git_tree_entry_id(entry)->id,
			base,
			base.GetLength(),
			git_tree_entry_name(entry),
			mode,
			0,
			data) == READ_TREE_RECURSIVE
		  )
		{
			if(mode&S_IFDIR)
			{
				git_object* object = nullptr;
				git_tree_entry_to_object(&object, &repo, entry);
				if (!object)
					continue;
				CStringA parent = base;
				parent += git_tree_entry_name(entry);
				parent += "/";
				ReadTreeRecursive(repo, (git_tree*)object, parent, CallBack, data);
				git_object_free(object);
			}
		}

	}

	return 0;
}

// ReadTree is/must only be executed on an empty list
int CGitHeadFileList::ReadTree()
{
	CAutoWriteLock lock(m_SharedMutex);
	ATLASSERT(empty());

	CAutoRepository repository(m_Gitdir);
	CAutoCommit commit;
	CAutoTree tree;
	bool ret = repository;
	ret = ret && !git_commit_lookup(commit.GetPointer(), repository, (const git_oid*)m_Head.m_hash);
	ret = ret && !git_commit_tree(tree.GetPointer(), commit);
	try
	{
		ret = ret && !ReadTreeRecursive(*repository, tree, "", CGitHeadFileList::CallBack, this);
	}
	catch (const std::bad_alloc& ex)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Catched exception inside ReadTreeRecursive: %s\n", ex.what());
		return -1;
	}
	if (!ret)
	{
		clear();
		m_LastModifyTimeHead = 0;
		return -1;
	}

	std::sort(this->begin(), this->end(), SortTree);
	m_TreeHash = git_commit_id(commit)->id;

	return 0;
}
int CGitIgnoreItem::FetchIgnoreList(const CString &projectroot, const CString &file, bool isGlobal)
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
		base.Replace(_T('\\'), _T('/'));

		int start = base.ReverseFind(_T('/'));
		if(start > 0)
		{
			base.Truncate(start);
			this->m_BaseDir = CUnicodeUtils::GetMulti(base, CP_UTF8) + "/";
		}
	}

	if (CGit::GetFileModifyTime(file, &m_LastModifyTime))
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
	m_buffer[size] = 0;

	if (git_create_exclude_list(&m_pExcludeList))
	{
		free(m_buffer);
		m_buffer = nullptr;
		return -1;
	}

	BYTE *p = m_buffer;
	int line = 0;
	for (DWORD i = 0; i < size; ++i)
	{
		if (m_buffer[i] == '\n' || m_buffer[i] == '\r' || i == (size - 1))
		{
			if (m_buffer[i] == '\n' || m_buffer[i] == '\r')
				m_buffer[i] = 0;

			if (p[0] != '#' && p[0] != 0)
				git_add_exclude((const char*)p, this->m_BaseDir, m_BaseDir.GetLength(), this->m_pExcludeList, ++line);

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
	const char* base = (pos >= 0) ? ((const char*)patha + pos + 1) : patha;

	return IsPathIgnored(patha, base, type);
}
#endif

int CGitIgnoreItem::IsPathIgnored(const CStringA& patha, const char* base, int& type)
{
	if (!m_pExcludeList)
		return -1; // error or undecided

	return git_check_excluded_1(patha, patha.GetLength(), base, &type, m_pExcludeList);
}

bool CGitIgnoreList::CheckFileChanged(const CString &path)
{
	__int64 time = 0;

	int ret = CGit::GetFileModifyTime(path, &time);

	bool cacheExist;
	{
		CAutoReadLock lock(m_SharedMutex);
		cacheExist = (m_Map.find(path) != m_Map.end());
	}

	if (!cacheExist && ret == 0)
	{
		CAutoWriteLock lock(m_SharedMutex);
		m_Map[path].m_LastModifyTime = 0;
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
		if (m_Map[path].m_LastModifyTime == time)
			return false;
	}
	return true;
}

bool CGitIgnoreList::CheckIgnoreChanged(const CString &gitdir, const CString &path, bool isDir)
{
	CString temp(gitdir);
	temp += _T('\\');
	temp += path;

	temp.Replace(_T('/'), _T('\\'));

	if (!isDir)
	{
		int x = temp.ReverseFind(_T('\\'));
		if (x >= 2)
			temp.Truncate(x);
	}

	while(!temp.IsEmpty())
	{
		CString tempOrig = temp;
		temp += _T("\\.git");

		if (CGit::GitPathFileExists(temp))
		{
			CString gitignore=temp;
			gitignore += _T("ignore");
			if (CheckFileChanged(gitignore))
				return true;

			CString adminDir = g_AdminDirMap.GetAdminDir(tempOrig);
			CString wcglobalgitignore = adminDir + _T("info\\exclude");
			if (CheckFileChanged(wcglobalgitignore))
				return true;

			if (CheckAndUpdateCoreExcludefile(adminDir))
				return true;

			return false;
		}

		temp += _T("ignore");
		if (CheckFileChanged(temp))
			return true;

		int found=0;
		int i;
		for (i = temp.GetLength() - 1; i >= 0; --i)
		{
			if(temp[i] == _T('\\'))
				++found;

			if(found == 2)
				break;
		}

		temp.Truncate(i);
	}
	return true;
}

int CGitIgnoreList::FetchIgnoreFile(const CString &gitdir, const CString &gitignore, bool isGlobal)
{
	if (CGit::GitPathFileExists(gitignore)) //if .gitignore remove, we need remote cache
	{
		CAutoWriteLock lock(m_SharedMutex);
		m_Map[gitignore].FetchIgnoreList(gitdir, gitignore, isGlobal);
	}
	else
	{
		CAutoWriteLock lock(m_SharedMutex);
		m_Map.erase(gitignore);
	}
	return 0;
}

int CGitIgnoreList::LoadAllIgnoreFile(const CString &gitdir, const CString &path, bool isDir)
{
	CString temp(gitdir);
	temp += _T('\\');
	temp += path;

	temp.Replace(_T('/'), _T('\\'));

	if (!isDir)
	{
		int x = temp.ReverseFind(_T('\\'));
		if (x >= 2)
			temp.Truncate(x);
	}

	while (!temp.IsEmpty())
	{
		CString tempOrig = temp;
		temp += _T("\\.git");

		if (CGit::GitPathFileExists(temp))
		{
			CString gitignore = temp;
			gitignore += _T("ignore");
			if (CheckFileChanged(gitignore))
				FetchIgnoreFile(gitdir, gitignore, false);

			CString adminDir = g_AdminDirMap.GetAdminDir(tempOrig);
			CString wcglobalgitignore = adminDir;
			wcglobalgitignore += _T("info\\exclude");
			if (CheckFileChanged(wcglobalgitignore))
			{
				FetchIgnoreFile(gitdir, wcglobalgitignore, true);
			}

			if (CheckAndUpdateCoreExcludefile(adminDir))
			{
				CString excludesFile;
				{
					CAutoReadLock lock(m_SharedMutex);
					excludesFile = m_CoreExcludesfiles[adminDir];
				}
				if (!excludesFile.IsEmpty())
					FetchIgnoreFile(gitdir, excludesFile, true);
			}

			return 0;
		}

		temp += _T("ignore");
		if (CheckFileChanged(temp))
			FetchIgnoreFile(gitdir, temp, false);

		int found = 0;
		int i;
		for (i = temp.GetLength() - 1; i >= 0; --i)
		{
			if(temp[i] == _T('\\'))
				++found;

			if(found == 2)
				break;
		}

		temp.Truncate(i);
	}
	return 0;
}
bool CGitIgnoreList::CheckAndUpdateGitSystemConfigPath(bool force)
{
	if (force)
		m_sGitProgramDataConfigPath = GetProgramDataGitConfig();
	// recheck every 30 seconds
	if (GetTickCount64() - m_dGitSystemConfigPathLastChecked > 30000UL || force)
	{
		m_dGitSystemConfigPathLastChecked = GetTickCount64();
		CString gitSystemConfigPath(CRegString(REG_SYSTEM_GITCONFIGPATH, _T(""), FALSE));
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
	projectConfig += _T("config");
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
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(projectConfig), GIT_CONFIG_LEVEL_LOCAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalConfig), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(globalXDGConfig), GIT_CONFIG_LEVEL_XDG, FALSE);
	if (!m_sGitSystemConfigPath.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(m_sGitSystemConfigPath), GIT_CONFIG_LEVEL_SYSTEM, FALSE);
	if (!m_sGitProgramDataConfigPath.IsEmpty())
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(m_sGitProgramDataConfigPath), GIT_CONFIG_LEVEL_PROGRAMDATA, FALSE);

	config.GetString(_T("core.excludesfile"), excludesFile);
	if (excludesFile.IsEmpty())
		excludesFile = GetWindowsHome() + _T("\\.config\\git\\ignore");
	else if (CStringUtils::StartsWith(excludesFile, L"~/"))
		excludesFile = GetWindowsHome() + excludesFile.Mid(1);

	CAutoWriteLock lockMap(m_SharedMutex);
	CGit::GetFileModifyTime(projectConfig, &m_Map[projectConfig].m_LastModifyTime);
	CGit::GetFileModifyTime(globalXDGConfig, &m_Map[globalXDGConfig].m_LastModifyTime);
	if (m_Map[globalXDGConfig].m_LastModifyTime == 0)
		m_Map.erase(globalXDGConfig);
	CGit::GetFileModifyTime(globalConfig, &m_Map[globalConfig].m_LastModifyTime);
	if (m_Map[globalConfig].m_LastModifyTime == 0)
		m_Map.erase(globalConfig);
	if (!m_sGitSystemConfigPath.IsEmpty())
		CGit::GetFileModifyTime(m_sGitSystemConfigPath, &m_Map[m_sGitSystemConfigPath].m_LastModifyTime);
	if (m_Map[m_sGitSystemConfigPath].m_LastModifyTime == 0 || m_sGitSystemConfigPath.IsEmpty())
		m_Map.erase(m_sGitSystemConfigPath);
	if (!m_sGitProgramDataConfigPath.IsEmpty())
		CGit::GetFileModifyTime(m_sGitProgramDataConfigPath, &m_Map[m_sGitProgramDataConfigPath].m_LastModifyTime);
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
bool CGitIgnoreList::IsIgnore(CString str, const CString& projectroot, bool isDir)
{
	str.Replace(_T('\\'),_T('/'));

	if (!str.IsEmpty() && str[str.GetLength() - 1] == _T('/'))
		str.Truncate(str.GetLength() - 1);

	int ret;
	ret = CheckIgnore(str, projectroot, isDir);
	while (ret < 0)
	{
		int start = str.ReverseFind(_T('/'));
		if(start < 0)
			return (ret == 1);

		str.Truncate(start);
		ret = CheckIgnore(str, projectroot, isDir);
	}

	return (ret == 1);
}
int CGitIgnoreList::CheckFileAgainstIgnoreList(const CString &ignorefile, const CStringA &patha, const char * base, int &type)
{
	if (m_Map.find(ignorefile) == m_Map.end())
		return -1; // error or undecided

	return (m_Map[ignorefile].IsPathIgnored(patha, base, type));
}
int CGitIgnoreList::CheckIgnore(const CString &path, const CString &projectroot, bool isDir)
{
	CString temp = CombinePath(projectroot, path);
	temp.Replace(_T('/'), _T('\\'));

	CStringA patha = CUnicodeUtils::GetMulti(path, CP_UTF8);
	patha.Replace('\\', '/');

	int type = 0;
	if (isDir)
	{
		type = DT_DIR;

		// strip directory name
		// we do not need to check for a .ignore file inside a directory we might ignore
		int i = temp.ReverseFind(_T('\\'));
		if (i >= 0)
			temp.Truncate(i);
	}
	else
	{
		type = DT_REG;

		int x = temp.ReverseFind(_T('\\'));
		if (x >= 2)
			temp.Truncate(x);
	}

	int pos = patha.ReverseFind('/');
	const char * base = (pos >= 0) ? ((const char*)patha + pos + 1) : patha;

	int ret = -1;

	CAutoReadLock lock(m_SharedMutex);
	while (!temp.IsEmpty())
	{
		CString tempOrig = temp;
		temp += _T("\\.git");

		if (CGit::GitPathFileExists(temp))
		{
			CString gitignore = temp;
			gitignore += _T("ignore");
			if ((ret = CheckFileAgainstIgnoreList(gitignore, patha, base, type)) != -1)
				break;

			CString adminDir = g_AdminDirMap.GetAdminDir(tempOrig);
			CString wcglobalgitignore = adminDir;
			wcglobalgitignore += _T("info\\exclude");
			if ((ret = CheckFileAgainstIgnoreList(wcglobalgitignore, patha, base, type)) != -1)
				break;

			CString excludesFile = m_CoreExcludesfiles[adminDir];
			if (!excludesFile.IsEmpty())
				ret = CheckFileAgainstIgnoreList(excludesFile, patha, base, type);

			break;
		}

		temp += _T("ignore");
		if ((ret = CheckFileAgainstIgnoreList(temp, patha, base, type)) != -1)
			break;

		int found = 0;
		int i;
		for (i = temp.GetLength() - 1; i >= 0; i--)
		{
			if (temp[i] == _T('\\'))
				++found;

			if (found == 2)
				break;
		}

		temp.Truncate(i);
	}

	return ret;
}

bool CGitHeadFileMap::CheckHeadAndUpdate(const CString &gitdir, bool readTree /* = true */)
{
	SHARED_TREE_PTR ptr = this->SafeGet(gitdir, true);

	if (ptr.get() && !ptr->CheckHeadUpdate() && (!readTree || ptr->HeadHashEqualsTreeHash()))
		return false;

	ptr = SHARED_TREE_PTR(new CGitHeadFileList);
	ptr->ReadHeadHash(gitdir);
	if (readTree)
		ptr->ReadTree();

	this->SafeSet(gitdir, ptr);

	return true;
}

int CGitHeadFileMap::IsUnderVersionControl(const CString& gitdir, CString subpath, bool isDir, bool* isVersion)
{
	if (subpath.IsEmpty())
	{
		*isVersion = true;
		return 0;
	}

	subpath.Replace(_T('\\'), _T('/'));
	if (isDir)
		subpath += _T('/');

	subpath.MakeLower();

	CheckHeadAndUpdate(gitdir);

	SHARED_TREE_PTR treeptr = SafeGet(gitdir);

	// Init Repository
	if (treeptr->HeadFileIsEmpty())
	{
		*isVersion = false;
		return 0;
	}
	if (treeptr->empty())
	{
		*isVersion = false;
		return 1;
	}

	if (isDir)
		*isVersion = (SearchInSortVector(*treeptr, subpath, subpath.GetLength()) >= 0);
	else
		*isVersion = (SearchInSortVector(*treeptr, subpath, -1) >= 0);

	return 0;
}
