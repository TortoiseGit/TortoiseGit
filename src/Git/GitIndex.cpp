// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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

#include "StdAfx.h"
#include "Git.h"
#include "atlconv.h"
#include "GitRev.h"
#include "registry.h"
#include "GitConfig.h"
#include <map>
#include "UnicodeUtils.h"
#include "TGitPath.h"
#include "gitindex.h"
#include <sys/types.h>
#include <sys/stat.h>
#include "SmartHandle.h"

class CAutoReadLock
{
	SharedMutex *m_Lock;
public:
	CAutoReadLock(SharedMutex * lock)
	{
		m_Lock = lock;
		lock->AcquireShared();
	}
	~CAutoReadLock()
	{
		m_Lock->ReleaseShared();
	}
};

class CAutoWriteLock
{
	SharedMutex *m_Lock;
public:
	CAutoWriteLock(SharedMutex * lock)
	{
		m_Lock = lock;
		lock->AcquireExclusive();
	}
	~CAutoWriteLock()
	{
		m_Lock->ReleaseExclusive();
	}
};

CGitAdminDirMap g_AdminDirMap;

int CGitIndex::Print()
{
	_tprintf(_T("0x%08X  0x%08X %s %s\n"),
		(int)this->m_ModifyTime,
		this->m_Flags,
		this->m_IndexHash.ToString(),
		this->m_FileName);

	return 0;
}

CGitIndexList::CGitIndexList()
{
	this->m_LastModifyTime = 0;
	m_critRepoSec.Init();
	repository = NULL;
	m_bCheckContent = !!(CRegDWORD(_T("Software\\TortoiseGit\\TGitCacheCheckContent"), TRUE) == TRUE);
}

CGitIndexList::~CGitIndexList()
{
	if (repository != NULL)
	{
		git_repository_free(repository);
		m_critRepoSec.Term();
	}
}

static bool SortIndex(CGitIndex &Item1, CGitIndex &Item2)
{
	return Item1.m_FileName.Compare(Item2.m_FileName) < 0;
}

static bool SortTree(CGitTreeItem &Item1, CGitTreeItem &Item2)
{
	return Item1.m_FileName.Compare(Item2.m_FileName) < 0;
}

int CGitIndexList::ReadIndex(CString dgitdir)
{
	this->clear();

	CStringA gitdir = CUnicodeUtils::GetMulti(dgitdir, CP_UTF8);

	m_critRepoSec.Lock();
	if (repository != NULL)
	{
		git_repository_free(repository);
		repository = NULL;
	}
	git_index *index = NULL;

	int ret = git_repository_open(&repository, gitdir.GetBuffer());
	gitdir.ReleaseBuffer();
	if (ret)
		return -1;

	// add config files
	git_config * config;
	git_config_new(&config);

	CString projectConfig = dgitdir + _T("config");
	CString globalConfig = g_Git.GetGitGlobalConfig();
	CString globalXDGConfig = g_Git.GetGitGlobalXDGConfig();
	CString msysGitBinPath(CRegString(REG_MSYSGIT_PATH, _T(""), FALSE));

	CStringA projectConfigA = CUnicodeUtils::GetMulti(projectConfig, CP_UTF8);
	git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 4, FALSE);
	projectConfigA.ReleaseBuffer();
	CStringA globalConfigA = CUnicodeUtils::GetMulti(globalConfig, CP_UTF8);
	git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 3, FALSE);
	globalConfigA.ReleaseBuffer();
	CStringA globalXDGConfigA = CUnicodeUtils::GetMulti(globalXDGConfig, CP_UTF8);
	git_config_add_file_ondisk(config, globalXDGConfigA.GetBuffer(), 2, FALSE);
	globalXDGConfigA.ReleaseBuffer();
	if (!msysGitBinPath.IsEmpty())
	{
		CString systemConfig = msysGitBinPath + _T("\\..\\etc\\gitconfig");
		CStringA systemConfigA = CUnicodeUtils::GetMulti(systemConfig, CP_UTF8);
		git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1, FALSE);
		systemConfigA.ReleaseBuffer();
	}

	git_repository_set_config(repository, config);

	// load index in order to enumerate files
	if (git_repository_index(&index, repository))
	{
		repository = NULL;
		m_critRepoSec.Unlock();
		return -1;
	}

	size_t ecount = git_index_entrycount(index);
	resize(ecount);
	for (size_t i = 0; i < ecount; ++i)
	{
		const git_index_entry *e = git_index_get_byindex(index, i);

		this->at(i).m_FileName.Empty();
		g_Git.StringAppend(&this->at(i).m_FileName, (BYTE*)e->path, CP_UTF8);
		this->at(i).m_FileName.MakeLower();
		this->at(i).m_ModifyTime = e->mtime.seconds;
		this->at(i).m_Flags = e->flags | e->flags_extended;
		this->at(i).m_IndexHash = (char *) e->oid.id;
	}

	git_index_free(index);

	g_Git.GetFileModifyTime(dgitdir + _T("index"), &this->m_LastModifyTime);
	std::sort(this->begin(), this->end(), SortIndex);

	m_critRepoSec.Unlock();

	return 0;
}

int CGitIndexList::GetFileStatus(const CString &gitdir,const CString &pathorg,git_wc_status_kind *status,__int64 time,FIll_STATUS_CALLBACK callback,void *pData, CGitHash *pHash, bool * assumeValid, bool * skipWorktree)
{
	if(status)
	{
		CString path = pathorg;
		path.MakeLower();

		int start = SearchInSortVector(*this, ((CString&)path).GetBuffer(), -1);
		((CString&)path).ReleaseBuffer();

		if (start < 0)
		{
			*status = git_wc_status_unversioned;
			if (pHash)
				pHash->Empty();

		}
		else
		{
 			int index = start;
			if (index <0)
				return -1;
			if (index >= (int)size())
				return -1;

			// skip-worktree has higher priority than assume-valid
			if (at(index).m_Flags & GIT_IDXENTRY_SKIP_WORKTREE)
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
			else if (time == at(index).m_ModifyTime)
			{
				*status = git_wc_status_normal;
			}
			else if (m_bCheckContent && repository)
			{
				git_oid actual;
				CStringA fileA = CUnicodeUtils::GetMulti(pathorg, CP_UTF8);
				m_critRepoSec.Lock(); // prevent concurrent access to repository instance and especially filter-lists
				if (!git_repository_hashfile(&actual, repository, fileA.GetBuffer(), GIT_OBJ_BLOB, NULL) && !git_oid_cmp(&actual, (const git_oid*)at(index).m_IndexHash.m_hash))
				{
					at(index).m_ModifyTime = time;
					*status = git_wc_status_normal;
				}
				else
					*status = git_wc_status_modified;
				fileA.ReleaseBuffer();
				m_critRepoSec.Unlock();
			}
			else
				*status = git_wc_status_modified;

			if (at(index).m_Flags & GIT_IDXENTRY_STAGEMASK)
				*status = git_wc_status_conflicted;
			else if (at(index).m_Flags & GIT_IDXENTRY_INTENT_TO_ADD)
				*status = git_wc_status_added;

			if(pHash)
				*pHash = at(index).m_IndexHash;
		}

	}

	if(callback && status)
			callback(gitdir + _T("\\") + pathorg, *status, false, pData, *assumeValid, *skipWorktree);
	return 0;
}

int CGitIndexList::GetStatus(const CString &gitdir,const CString &pathParam, git_wc_status_kind *status,
							 BOOL IsFull, BOOL /*IsRecursive*/,
							 FIll_STATUS_CALLBACK callback,void *pData,
							 CGitHash *pHash, bool * assumeValid, bool * skipWorktree)
{
	int result;
	git_wc_status_kind dirstatus = git_wc_status_none;
	__int64 time;
	bool isDir = false;
	CString path = pathParam;

	if (status)
	{
		if (path.IsEmpty())
			result = g_Git.GetFileModifyTime(gitdir, &time, &isDir);
		else
			result = g_Git.GetFileModifyTime(gitdir + _T("\\") + path, &time, &isDir);

		if (result)
		{
			*status = git_wc_status_deleted;
			if (callback)
				callback(gitdir + _T("\\") + path, git_wc_status_deleted, false, pData, *assumeValid, *skipWorktree);

			return 0;
		}
		if (isDir)
		{
			if (!path.IsEmpty())
			{
				if (path.Right(1) != _T("\\"))
					path += _T("\\");
			}
			int len = path.GetLength();

				for (size_t i = 0; i < size(); i++)
				{
					if (at(i).m_FileName.GetLength() > len)
					{
						if (at(i).m_FileName.Left(len) == path)
						{
							if (!IsFull)
							{
								*status = git_wc_status_normal;
								if (callback)
									callback(gitdir + _T("\\") + path, *status, false, pData, (at(i).m_Flags & GIT_IDXENTRY_VALID) && !(at(i).m_Flags & GIT_IDXENTRY_SKIP_WORKTREE), (at(i).m_Flags & GIT_IDXENTRY_SKIP_WORKTREE) != 0);
								return 0;

							}
							else
							{
								result = g_Git.GetFileModifyTime(gitdir+_T("\\") + at(i).m_FileName, &time);
								if (result)
									continue;

								*status = git_wc_status_none;
								if (assumeValid)
									*assumeValid = false;
								if (skipWorktree)
									*skipWorktree = false;
								GetFileStatus(gitdir, at(i).m_FileName, status, time, callback, pData, NULL, assumeValid, skipWorktree);
								// if a file is assumed valid, we need to inform the caller, otherwise the assumevalid flag might not get to the explorer on first open of a repository
								if (callback && (assumeValid || skipWorktree))
									callback(gitdir + _T("\\") + path, *status, false, pData, *assumeValid, *skipWorktree);
								if (*status != git_wc_status_none)
								{
									if (dirstatus == git_wc_status_none)
									{
										dirstatus = git_wc_status_normal;
									}
									if (*status != git_wc_status_normal)
									{
										dirstatus = git_wc_status_modified;
									}
								}

							}
						}
					}
				} /* End For */

			if (dirstatus != git_wc_status_none)
			{
				*status = dirstatus;
			}
			else
			{
				*status = git_wc_status_unversioned;
			}
			if(callback)
				callback(gitdir + _T("\\") + path, *status, false, pData, false, false);

			return 0;

		}
		else
		{
			GetFileStatus(gitdir, path, status, time, callback, pData, pHash, assumeValid, skipWorktree);
		}
	}
	return 0;
}

int CGitIndexFileMap::Check(const CString &gitdir, bool *isChanged)
{
	__int64 time;
	int result;

	CString IndexFile = g_AdminDirMap.GetAdminDir(gitdir) + _T("index");

	/* Get data associated with "crt_stat.c": */
	result = g_Git.GetFileModifyTime(IndexFile, &time);

	if (result)
		return result;

	SHARED_INDEX_PTR pIndex;
	pIndex = this->SafeGet(gitdir);

	if (pIndex.get() == NULL)
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
	try
	{
		SHARED_INDEX_PTR pIndex(new CGitIndexList);

		if(pIndex->ReadIndex(g_AdminDirMap.GetAdminDir(gitdir)))
			return -1;

		this->SafeSet(gitdir, pIndex);

	}catch(...)
	{
		return -1;
	}
	return 0;
}

int CGitIndexFileMap::GetFileStatus(const CString &gitdir, const CString &path, git_wc_status_kind *status,BOOL IsFull, BOOL IsRecursive,
									FIll_STATUS_CALLBACK callback,void *pData,
									CGitHash *pHash,
									bool isLoadUpdatedIndex, bool * assumeValid, bool * skipWorktree)
{
	try
	{
		CheckAndUpdate(gitdir, isLoadUpdatedIndex);

		SHARED_INDEX_PTR pIndex = this->SafeGet(gitdir);
		if (pIndex.get() != NULL)
		{
			pIndex->GetStatus(gitdir, path, status, IsFull, IsRecursive, callback, pData, pHash, assumeValid, skipWorktree);
		}
		else
		{
			// git working tree has not index
			*status = git_wc_status_unversioned;
		}
	}
	catch(...)
	{
		return -1;
	}
	return 0;
}

int CGitIndexFileMap::IsUnderVersionControl(const CString &gitdir, const CString &path, bool isDir,bool *isVersion, bool isLoadUpdateIndex)
{
	try
	{
		if (path.IsEmpty())
		{
			*isVersion = true;
			return 0;
		}

		CString subpath = path;
		subpath.Replace(_T('\\'), _T('/'));
		if(isDir)
			subpath += _T('/');

		subpath.MakeLower();

		CheckAndUpdate(gitdir, isLoadUpdateIndex);

		SHARED_INDEX_PTR pIndex = this->SafeGet(gitdir);

		if(pIndex.get())
		{
			if(isDir)
				*isVersion = (SearchInSortVector(*pIndex, subpath.GetBuffer(), subpath.GetLength()) >= 0);
			else
				*isVersion = (SearchInSortVector(*pIndex, subpath.GetBuffer(), -1) >= 0);
			subpath.ReleaseBuffer();
		}

	}catch(...)
	{
		return -1;
	}
	return 0;
}

int CGitHeadFileList::GetPackRef(const CString &gitdir)
{
	CString PackRef = g_AdminDirMap.GetAdminDir(gitdir) + _T("packed-refs");

	__int64 mtime;
	if (g_Git.GetFileModifyTime(PackRef, &mtime))
	{
		CAutoWriteLock lock(&this->m_SharedMutex);
		//packed refs is not existed
		this->m_PackRefFile.Empty();
		this->m_PackRefMap.clear();
		return 0;
	}
	else if(mtime == m_LastModifyTimePackRef)
	{
		return 0;
	}
	else
	{
		CAutoWriteLock lock(&this->m_SharedMutex);
		this->m_PackRefFile = PackRef;
		this->m_LastModifyTimePackRef = mtime;
	}

	int ret = 0;
	{
		CAutoWriteLock lock(&this->m_SharedMutex);
		this->m_PackRefMap.clear();

		CAutoFile hfile = CreateFile(PackRef,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		do
		{
			if (!hfile)
			{
				ret = -1;
				break;
			}

			DWORD filesize = GetFileSize(hfile, NULL);
			DWORD size =0;
			char *buff;
			buff = new char[filesize];

			ReadFile(hfile, buff, filesize, &size, NULL);

			if (size != filesize)
			{
				ret = -1;
				break;
			}

			CString hash;
			CString ref;

			for(DWORD i=0;i<filesize;)
			{
				hash.Empty();
				ref.Empty();
				if (buff[i] == '#' || buff[i] == '^')
				{
					while (buff[i] != '\n')
					{
						i++;
						if (i == filesize)
							break;
					}
					i++;
				}

				if (i >= filesize)
					break;

				while (buff[i] != ' ')
				{
					hash.AppendChar(buff[i]);
					i++;
					if (i == filesize)
						break;
				}

				i++;
				if (i >= filesize)
					break;

				while (buff[i] != '\n')
				{
					ref.AppendChar(buff[i]);
					i++;
					if (i == filesize)
						break;
				}

				if (!ref.IsEmpty() )
				{
					this->m_PackRefMap[ref] = hash;
				}

				while (buff[i] == '\n')
				{
					i++;
					if (i == filesize)
						break;
				}
			}

			delete[] buff;

		} while(0);
	}
	return ret;

}
int CGitHeadFileList::ReadHeadHash(CString gitdir)
{
	int ret = 0;
	CAutoWriteLock lock(&this->m_SharedMutex);
	m_Gitdir = g_AdminDirMap.GetAdminDir(gitdir);

	m_HeadFile = m_Gitdir + _T("HEAD");

	if( g_Git.GetFileModifyTime(m_HeadFile, &m_LastModifyTimeHead))
		return -1;

	try
	{
		do
		{
			CAutoFile hfile = CreateFile(m_HeadFile,
				GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if (!hfile)
			{
				ret = -1;
				break;
			}

			DWORD size = 0,filesize = 0;
			unsigned char buffer[40] ;
			ReadFile(hfile, buffer, 4, &size, NULL);
			if (size != 4)
			{
				ret = -1;
				break;
			}
			buffer[4]=0;
			if (strcmp((const char*)buffer,"ref:") == 0)
			{
				filesize = GetFileSize(hfile, NULL);

				unsigned char *p = (unsigned char*)malloc(filesize -4);

				ReadFile(hfile, p, filesize - 4, &size, NULL);

				m_HeadRefFile.Empty();
				g_Git.StringAppend(&this->m_HeadRefFile, p, CP_UTF8, filesize - 4);
				CString ref = this->m_HeadRefFile;
				ref = ref.Trim();
				int start = 0;
				ref = ref.Tokenize(_T("\n"), start);
				free(p);
				m_HeadRefFile = m_Gitdir + m_HeadRefFile.Trim();
				m_HeadRefFile.Replace(_T('/'),_T('\\'));

				__int64 time;
				if (g_Git.GetFileModifyTime(m_HeadRefFile, &time, NULL))
				{
					m_HeadRefFile.Empty();
					if (GetPackRef(gitdir))
					{
						ret = -1;
						break;
					}
					if (this->m_PackRefMap.find(ref) == m_PackRefMap.end())
					{
						ret = -1;
						break;
					}
					this ->m_Head = m_PackRefMap[ref];
					ret = 0;
					break;
				}

				CAutoFile href = CreateFile(m_HeadRefFile,
					GENERIC_READ,
					FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

				if (!href)
				{
					m_HeadRefFile.Empty();

					if (GetPackRef(gitdir))
					{
						ret = -1;
						break;
					}

					if (this->m_PackRefMap.find(ref) == m_PackRefMap.end())
					{
						ret = -1;
						break;
					}
					this ->m_Head = m_PackRefMap[ref];
					ret = 0;
					break;
				}
				ReadFile(href, buffer, 40, &size, NULL);
				if (size != 40)
				{
					ret = -1;
					break;
				}
				this->m_Head.ConvertFromStrA((char*)buffer);

				this->m_LastModifyTimeRef = time;

			}
			else
			{
				ReadFile(hfile, buffer + 4, 40 - 4, &size, NULL);
				if(size != 36)
				{
					ret = -1;
					break;
				}
				m_HeadRefFile.Empty();

				this->m_Head.ConvertFromStrA((char*)buffer);
			}
		} while(0);
	}
	catch(...)
	{
		ret = -1;
	}

	return ret;
}

bool CGitHeadFileList::CheckHeadUpdate()
{
	CAutoReadLock lock(&m_SharedMutex);
	if (this->m_HeadFile.IsEmpty())
		return true;

	__int64 mtime=0;

	if (g_Git.GetFileModifyTime(m_HeadFile, &mtime))
		return true;

	if (mtime != this->m_LastModifyTimeHead)
		return true;

	if (!this->m_HeadRefFile.IsEmpty())
	{
		if (g_Git.GetFileModifyTime(m_HeadRefFile, &mtime))
			return true;

		if (mtime != this->m_LastModifyTimeRef)
			return true;
	}

	if(!this->m_PackRefFile.IsEmpty())
	{
		if (g_Git.GetFileModifyTime(m_PackRefFile, &mtime))
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

int CGitHeadFileList::CallBack(const unsigned char *sha1, const char *base, int baselen,
		const char *pathname, unsigned mode, int /*stage*/, void *context)
{
#define S_IFGITLINK	0160000

	CGitHeadFileList *p = (CGitHeadFileList*)context;
	if( mode&S_IFDIR )
	{
		if( (mode&S_IFMT) != S_IFGITLINK)
			return READ_TREE_RECURSIVE;
	}

	size_t cur = p->size();
	p->resize(p->size() + 1);
	p->at(cur).m_Hash = (char*)sha1;
	p->at(cur).m_FileName.Empty();

	if(base)
		g_Git.StringAppend(&p->at(cur).m_FileName, (BYTE*)base, CP_UTF8, baselen);

	g_Git.StringAppend(&p->at(cur).m_FileName,(BYTE*)pathname, CP_UTF8);

	p->at(cur).m_FileName.MakeLower();

	//p->at(cur).m_FileName.Replace(_T('/'), _T('\\'));

	//p->m_Map[p->at(cur).m_FileName] = cur;

	if( (mode&S_IFMT) == S_IFGITLINK)
		return 0;

	return READ_TREE_RECURSIVE;
}

int ReadTreeRecursive(git_repository &repo, git_tree * tree, CStringA base, int (*CallBack) (const unsigned char *, const char *, int, const char *, unsigned int, int, void *),void *data)
{
	size_t count = git_tree_entrycount(tree);
	for (size_t i = 0; i < count; i++)
	{
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		if (entry == NULL)
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
				git_object *object = NULL;
				git_tree_entry_to_object(&object, &repo, entry);
				if (object == NULL)
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

int CGitHeadFileList::ReadTree()
{
	CAutoWriteLock lock(&m_SharedMutex);
	CStringA gitdir = CUnicodeUtils::GetMulti(m_Gitdir, CP_UTF8);
	git_repository *repository = NULL;
	git_commit *commit = NULL;
	git_tree * tree = NULL;
	int ret = 0;
	this->clear(); // hack to avoid duplicates in the head list, which are introduced in GitStatus::GetFileStatus when this method is called
	do
	{
		ret = git_repository_open(&repository, gitdir.GetBuffer());
		gitdir.ReleaseBuffer();
		if(ret)
			break;
		ret = git_commit_lookup(&commit, repository, (const git_oid*)m_Head.m_hash);
		if(ret)
			break;

		ret = git_commit_tree(&tree, commit);
		if(ret)
			break;

		ret = ReadTreeRecursive(*repository, tree,"", CGitHeadFileList::CallBack,this);
		if(ret)
			break;

		std::sort(this->begin(), this->end(), SortTree);
		this->m_TreeHash = (char*)(git_commit_id(commit)->id);

	} while(0);

	if (tree)
		git_tree_free(tree);

	if (commit)
		git_commit_free(commit);

	if (repository)
		git_repository_free(repository);

	return ret;

}
int CGitIgnoreItem::FetchIgnoreList(const CString &projectroot, const CString &file, bool isGlobal)
{
	CAutoWriteLock lock(&this->m_SharedMutex);

	if (this->m_pExcludeList)
	{
		free(m_pExcludeList);
		m_pExcludeList=NULL;
	}

	this->m_BaseDir.Empty();
	if (!isGlobal)
	{
		CString base = file.Mid(projectroot.GetLength() + 1);
		base.Replace(_T('\\'), _T('/'));

		int start = base.ReverseFind(_T('/'));
		if(start > 0)
		{
			base = base.Left(start);
			this->m_BaseDir = CUnicodeUtils::GetMulti(base, CP_UTF8) + "/";
		}
	}
	{

		if(g_Git.GetFileModifyTime(file, &m_LastModifyTime))
			return -1;

		if(git_create_exclude_list(&this->m_pExcludeList))
			return -1;


		CAutoFile hfile = CreateFile(file,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);


		if (!hfile)
			return -1 ;

		DWORD size=0,filesize=0;

		filesize=GetFileSize(hfile, NULL);

		if(filesize == INVALID_FILE_SIZE)
			return -1;

		BYTE *buffer = new BYTE[filesize + 1];

		if(buffer == NULL)
			return -1;

		if(! ReadFile(hfile, buffer,filesize,&size,NULL))
			return GetLastError();

		BYTE *p = buffer;
		for (DWORD i = 0; i < size; i++)
		{
			if (buffer[i] == '\n' || buffer[i] == '\r' || i == (size - 1))
			{
				if (buffer[i] == '\n' || buffer[i] == '\r')
					buffer[i] = 0;
				if (i == size - 1)
					buffer[size] = 0;

				if(p[0] != '#' && p[0] != 0)
					git_add_exclude((const char*)p,
										this->m_BaseDir.GetBuffer(),
										m_BaseDir.GetLength(),
										this->m_pExcludeList);

				p=buffer+i+1;
			}
		}
		/* Can't free buffer, exluced list will use this buffer*/
		//delete buffer;
		//buffer = NULL;
	}
	return 0;
}

bool CGitIgnoreList::CheckFileChanged(const CString &path)
{
	__int64 time = 0;

	int ret = g_Git.GetFileModifyTime(path, &time);

	this->m_SharedMutex.AcquireShared();
	bool cacheExist = (m_Map.find(path) != m_Map.end());
	this->m_SharedMutex.ReleaseShared();

	if (!cacheExist && ret == 0)
	{
		CAutoWriteLock lock(&this->m_SharedMutex);
		m_Map[path].m_LastModifyTime = 0;
		m_Map[path].m_SharedMutex.Init();
	}
	// both cache and file is not exist
	if ((ret != 0) && (!cacheExist))
		return false;

	// file exist but cache miss
	if ((ret == 0) && (!cacheExist))
		return true;

	// file not exist but cache exist
	if ((ret != 0) && (cacheExist))
	{
		return true;
	}
	// file exist and cache exist

	{
		CAutoReadLock lock(&this->m_SharedMutex);
		if (m_Map[path].m_LastModifyTime == time)
			return false;
	}
	return true;
}

bool CGitIgnoreList::CheckIgnoreChanged(const CString &gitdir,const CString &path)
{
	CString temp;
	temp = gitdir;
	temp += _T("\\");
	temp += path;

	temp.Replace(_T('/'), _T('\\'));

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
		else
		{
			temp += _T("ignore");
			if (CheckFileChanged(temp))
				return true;
		}

		int found=0;
		int i;
		for (i = temp.GetLength() - 1; i >= 0; i--)
		{
			if(temp[i] == _T('\\'))
				found ++;

			if(found == 2)
				break;
		}

		temp = temp.Left(i);
	}
	return true;
}

int CGitIgnoreList::FetchIgnoreFile(const CString &gitdir, const CString &gitignore, bool isGlobal)
{
	if (CGit::GitPathFileExists(gitignore)) //if .gitignore remove, we need remote cache
	{
		CAutoWriteLock lock(&this->m_SharedMutex);
		if (m_Map.find(gitignore) == m_Map.end())
			m_Map[gitignore].m_SharedMutex.Init();

		m_Map[gitignore].FetchIgnoreList(gitdir, gitignore, isGlobal);
	}
	else
	{
		CAutoWriteLock lock(&this->m_SharedMutex);
		if (m_Map.find(gitignore) != m_Map.end())
			m_Map[gitignore].m_SharedMutex.Release();

		m_Map.erase(gitignore);
	}
	return 0;
}

int CGitIgnoreList::LoadAllIgnoreFile(const CString &gitdir,const CString &path)
{
	CString temp;

	temp = gitdir;
	temp += _T("\\");
	temp += path;

	temp.Replace(_T('/'), _T('\\'));

	while (!temp.IsEmpty())
	{
		CString tempOrig = temp;
		temp += _T("\\.git");

		if (CGit::GitPathFileExists(temp))
		{
			CString gitignore = temp;
			gitignore += _T("ignore");
			if (CheckFileChanged(gitignore))
			{
				FetchIgnoreFile(gitdir, gitignore, false);
			}

			CString adminDir = g_AdminDirMap.GetAdminDir(tempOrig);
			CString wcglobalgitignore = adminDir + _T("info\\exclude");
			if (CheckFileChanged(wcglobalgitignore))
			{
				FetchIgnoreFile(gitdir, wcglobalgitignore, true);
			}

			if (CheckAndUpdateCoreExcludefile(adminDir))
			{
				m_SharedMutex.AcquireShared();
				CString excludesFile = m_CoreExcludesfiles[adminDir];
				m_SharedMutex.ReleaseShared();
				if (!excludesFile.IsEmpty())
					FetchIgnoreFile(gitdir, excludesFile, true);
			}

			return 0;
		}
		else
		{
			temp += _T("ignore");
			if (CheckFileChanged(temp))
			{
				FetchIgnoreFile(gitdir, temp, false);
			}
		}

		int found = 0;
		int i;
		for (i = temp.GetLength() - 1; i >= 0; i--)
		{
			if(temp[i] == _T('\\'))
				found ++;

			if(found == 2)
				break;
		}

		temp = temp.Left(i);
	}
	return 0;
}
bool CGitIgnoreList::CheckAndUpdateMsysGitBinpath(bool force)
{
	// recheck every 30 seconds
	if (GetTickCount() - m_dMsysGitBinPathLastChecked > 30000 || force)
	{
		m_dMsysGitBinPathLastChecked = GetTickCount();
		CString msysGitBinPath(CRegString(REG_MSYSGIT_PATH, _T(""), FALSE));
		if (msysGitBinPath != m_sMsysGitBinPath)
		{
			m_sMsysGitBinPath = msysGitBinPath;
			return true;
		}
	}
	return false;
}
bool CGitIgnoreList::CheckAndUpdateCoreExcludefile(const CString &adminDir)
{
	bool hasChanged = false;

	CString projectConfig = adminDir + _T("config");
	CString globalConfig = g_Git.GetGitGlobalConfig();
	CString globalXDGConfig = g_Git.GetGitGlobalXDGConfig();

	CAutoWriteLock lock(&m_coreExcludefilesSharedMutex);
	hasChanged = CheckAndUpdateMsysGitBinpath();
	CString systemConfig = m_sMsysGitBinPath + _T("\\..\\etc\\gitconfig");

	hasChanged = hasChanged || CheckFileChanged(projectConfig);
	hasChanged = hasChanged || CheckFileChanged(globalConfig);
	hasChanged = hasChanged || CheckFileChanged(globalXDGConfig);
	if (!m_sMsysGitBinPath.IsEmpty())
		hasChanged = hasChanged || CheckFileChanged(systemConfig);

	m_SharedMutex.AcquireShared();
	CString excludesFile = m_CoreExcludesfiles[adminDir];
	m_SharedMutex.ReleaseShared();
	if (!excludesFile.IsEmpty())
		hasChanged = hasChanged || CheckFileChanged(excludesFile);

	if (!hasChanged)
		return false;

	git_config * config;
	git_config_new(&config);
	CStringA projectConfigA = CUnicodeUtils::GetMulti(projectConfig, CP_UTF8);
	git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 4, FALSE);
	projectConfigA.ReleaseBuffer();
	CStringA globalConfigA = CUnicodeUtils::GetMulti(globalConfig, CP_UTF8);
	git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 3, FALSE);
	globalConfigA.ReleaseBuffer();
	CStringA globalXDGConfigA = CUnicodeUtils::GetMulti(globalXDGConfig, CP_UTF8);
	git_config_add_file_ondisk(config, globalXDGConfigA.GetBuffer(), 2, FALSE);
	globalXDGConfigA.ReleaseBuffer();
	if (!m_sMsysGitBinPath.IsEmpty())
	{
		CStringA systemConfigA = CUnicodeUtils::GetMulti(systemConfig, CP_UTF8);
		git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1, FALSE);
		systemConfigA.ReleaseBuffer();
	}
	const char * out = NULL;
	CStringA name(_T("core.excludesfile"));
	git_config_get_string(&out, config, name.GetBuffer());
	name.ReleaseBuffer();
	CStringA excludesFileA(out);
	excludesFile = CUnicodeUtils::GetUnicode(excludesFileA);
	if (excludesFile.IsEmpty())
		excludesFile = GetWindowsHome() + _T("\\.config\\git\\ignore");
	else if (excludesFile.Find(_T("~/")) == 0)
		excludesFile = GetWindowsHome() + excludesFile.Mid(1);
	git_config_free(config);

	CAutoWriteLock lockMap(&m_SharedMutex);
	g_Git.GetFileModifyTime(projectConfig, &m_Map[projectConfig].m_LastModifyTime);
	g_Git.GetFileModifyTime(globalXDGConfig, &m_Map[globalXDGConfig].m_LastModifyTime);
	if (m_Map[globalXDGConfig].m_LastModifyTime == 0)
	{
		m_Map[globalXDGConfig].m_SharedMutex.Release();
		m_Map.erase(globalXDGConfig);
	}
	g_Git.GetFileModifyTime(globalConfig, &m_Map[globalConfig].m_LastModifyTime);
	if (m_Map[globalConfig].m_LastModifyTime == 0)
	{
		m_Map[globalConfig].m_SharedMutex.Release();
		m_Map.erase(globalConfig);
	}
	if (!m_sMsysGitBinPath.IsEmpty())
		g_Git.GetFileModifyTime(systemConfig, &m_Map[systemConfig].m_LastModifyTime);
	if (m_Map[systemConfig].m_LastModifyTime == 0 || m_sMsysGitBinPath.IsEmpty())
	{
		m_Map[systemConfig].m_SharedMutex.Release();
		m_Map.erase(systemConfig);
	}
	m_CoreExcludesfiles[adminDir] = excludesFile;

	return true;
}
const CString CGitIgnoreList::GetWindowsHome()
{
	static CString sWindowsHome(g_Git.GetHomeDirectory());
	return sWindowsHome;
}
bool CGitIgnoreList::IsIgnore(const CString &path,const CString &projectroot)
{
	CString str=path;

	str.Replace(_T('\\'),_T('/'));

	if (str.GetLength()>0)
		if (str[str.GetLength()-1] == _T('/'))
			str = str.Left(str.GetLength() - 1);

	int ret;
	ret = CheckIgnore(str, projectroot);
	while (ret < 0)
	{
		int start = str.ReverseFind(_T('/'));
		if(start < 0)
			return (ret == 1);

		str = str.Left(start);
		ret = CheckIgnore(str, projectroot);
	}

	return (ret == 1);
}
int CGitIgnoreList::CheckFileAgainstIgnoreList(const CString &ignorefile, const CStringA &patha, const char * base, int &type)
{
	if (m_Map.find(ignorefile) != m_Map.end())
	{
		int ret = -1;
		if(m_Map[ignorefile].m_pExcludeList)
			ret = git_check_excluded_1(patha, patha.GetLength(), base, &type, m_Map[ignorefile].m_pExcludeList);
		if (ret == 0 || ret == 1)
			return ret;
	}
	return -1;
}
int CGitIgnoreList::CheckIgnore(const CString &path,const CString &projectroot)
{
	__int64 time = 0;
	bool dir = 0;
	CString temp = projectroot + _T("\\") + path;
	temp.Replace(_T('/'), _T('\\'));

	CStringA patha = CUnicodeUtils::GetMulti(path, CP_UTF8);
	patha.Replace('\\', '/');

	if(g_Git.GetFileModifyTime(temp, &time, &dir))
		return -1;

	int type = 0;
	if (dir)
	{
		type = DT_DIR;

		// strip directory name
		// we do not need to check for a .ignore file inside a directory we might ignore
		int i = temp.ReverseFind(_T('\\'));
		if (i >= 0)
			temp = temp.Left(i);
	}
	else
		type = DT_REG;

	char * base = NULL;
	int pos = patha.ReverseFind('/');
	base = pos >= 0 ? patha.GetBuffer() + pos + 1 : patha.GetBuffer();

	int ret = -1;

	CAutoReadLock lock(&this->m_SharedMutex);
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
			CString wcglobalgitignore = adminDir + _T("info\\exclude");
			if ((ret = CheckFileAgainstIgnoreList(wcglobalgitignore, patha, base, type)) != -1)
				break;

			m_SharedMutex.AcquireShared();
			CString excludesFile = m_CoreExcludesfiles[adminDir];
			m_SharedMutex.ReleaseShared();
			if (!excludesFile.IsEmpty())
				ret = CheckFileAgainstIgnoreList(excludesFile, patha, base, type);

			break;
		}
		else
		{
			temp += _T("ignore");
			if ((ret = CheckFileAgainstIgnoreList(temp, patha, base, type)) != -1)
				break;
		}

		int found = 0;
		int i;
		for (i = temp.GetLength() - 1; i >= 0; i--)
		{
			if (temp[i] == _T('\\'))
				found++;

			if (found == 2)
				break;
		}

		temp = temp.Left(i);
	}

	patha.ReleaseBuffer();

	return ret;
}

bool CGitHeadFileMap::CheckHeadUpdate(const CString &gitdir)
{
	SHARED_TREE_PTR ptr;
	ptr = this->SafeGet(gitdir);

	if( ptr.get())
	{
		return ptr->CheckHeadUpdate();
	}
	else
	{
		SHARED_TREE_PTR ptr1(new CGitHeadFileList);
		ptr1->ReadHeadHash(gitdir);

		this->SafeSet(gitdir, ptr1);
		return true;
	}
}

int CGitHeadFileMap::IsUnderVersionControl(const CString &gitdir, const CString &path, bool isDir, bool *isVersion)
{
	try
	{
		if (path.IsEmpty())
		{
			*isVersion = true;
			return 0;
		}

		CString subpath = path;
		subpath.Replace(_T('\\'), _T('/'));
		if(isDir)
			subpath += _T('/');

		subpath.MakeLower();

		CheckHeadUpdate(gitdir);

		SHARED_TREE_PTR treeptr;
		treeptr = SafeGet(gitdir);

		if (treeptr->m_Head != treeptr->m_TreeHash)
		{
			treeptr->ReadHeadHash(gitdir);

			// Init Repository
			if (treeptr->m_HeadFile.IsEmpty())
			{
				*isVersion = false;
				return 0;
			}
			else if (treeptr->ReadTree())
			{
				treeptr->m_LastModifyTimeHead = 0;
				*isVersion = false;
				return 1;
			}
			SafeSet(gitdir, treeptr);
		}

		if(isDir)
			*isVersion = (SearchInSortVector(*treeptr, subpath.GetBuffer(), subpath.GetLength()) >= 0);
		else
			*isVersion = (SearchInSortVector(*treeptr, subpath.GetBuffer(), -1) >= 0);
		subpath.ReleaseBuffer();
	}
	catch(...)
	{
		return -1;
	}

	return 0;
}

int CGitHeadFileMap::GetHeadHash(const CString &gitdir, CGitHash &hash)
{
	SHARED_TREE_PTR ptr;
	ptr = this->SafeGet(gitdir);

	if(ptr.get() == NULL)
	{
		SHARED_TREE_PTR ptr1(new CGitHeadFileList());
		ptr1->ReadHeadHash(gitdir);

		hash = ptr1->m_Head;

		this->SafeSet(gitdir, ptr1);

	}
	else
	{
		if(ptr->CheckHeadUpdate())
		{
			SHARED_TREE_PTR ptr1(new CGitHeadFileList());
			ptr1->ReadHeadHash(gitdir);

			hash = ptr1->m_Head;
			this->SafeSet(gitdir, ptr1);
	}

		hash = ptr->m_Head;
	}
	return 0;
}
