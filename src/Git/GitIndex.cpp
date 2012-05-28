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
#include "git2.h"
#include "SmartHandle.h"

CGitAdminDirMap g_AdminDirMap;

#define FILL_DATA() \
	m_FileName.Empty();\
	g_Git.StringAppend(&m_FileName, (BYTE*)entry->name, CP_UTF8, Big2lit(entry->flags)&CE_NAMEMASK);\
	m_FileName.MakeLower(); \
	this->m_Flags=Big2lit(entry->flags);\
	this->m_ModifyTime=Big2lit(entry->mtime.sec);\
	this->m_IndexHash=(char*)(entry->sha1);

int CGitIndex::FillData(ondisk_cache_entry * entry)
{
	FILL_DATA();
	return 0;
}

int CGitIndex::FillData(ondisk_cache_entry_extended * entry)
{
	FILL_DATA();
	this->m_Flags |= ((int)Big2lit(entry->flags2))<<16;
	return 0;
}

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
}

static bool SortIndex(CGitIndex &Item1, CGitIndex &Item2)
{
	return Item1.m_FileName.Compare(Item2.m_FileName) < 0;
}

static bool SortTree(CGitTreeItem &Item1, CGitTreeItem &Item2)
{
	return Item1.m_FileName.Compare(Item2.m_FileName) < 0;
}

int CGitIndexList::ReadIndex(CString IndexFile)
{
	int ret=0;
	BYTE *buffer = NULL, *p;
	CGitIndex GitIndex;

#ifdef DEBUG
	m_GitFile = IndexFile;
#endif

	try
	{
		do
		{
			this->clear();

			CAutoFile hfile = CreateFile(IndexFile,
									GENERIC_READ,
									FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL);


			if (!hfile)
			{
				ret = -1 ;
				break;
			}

			CAutoFile hmap = CreateFileMapping(hfile, NULL, PAGE_READONLY, 0, 0, NULL);
			if (!hmap)
			{
				ret =-1;
				break;
			}

			p = buffer = (BYTE*)MapViewOfFile(hmap, FILE_MAP_READ, 0, 0, 0);
			if (buffer == NULL)
			{
				ret = -1;
				break;
			}

			cache_header *header;
			header = (cache_header *) buffer;

			if (Big2lit(header->hdr_signature) != CACHE_SIGNATURE )
			{
				ret = -1;
				break;
			}
			p += sizeof(cache_header);

			int entries = Big2lit(header->hdr_entries);
			resize(entries);

				for(int i = 0; i < entries; i++)
				{
					ondisk_cache_entry *entry;
					ondisk_cache_entry_extended *entryex;
					entry = (ondisk_cache_entry*)p;
					entryex = (ondisk_cache_entry_extended*)p;
					int flags=Big2lit(entry->flags);
					if (flags & CE_EXTENDED)
					{
						this->at(i).FillData(entryex);
						p += ondisk_ce_size(entryex);
					}
					else
					{
						this->at(i).FillData(entry);
						p += ondisk_ce_size(entry);
					}
				}

			std::sort(this->begin(), this->end(), SortIndex);
			g_Git.GetFileModifyTime(IndexFile, &this->m_LastModifyTime);
		} while(0);
	}
	catch(...)
	{
		ret= -1;
	}

	if (buffer)
		UnmapViewOfFile(buffer);

	return ret;
}

int CGitIndexList::GetFileStatus(const CString &gitdir,const CString &pathorg,git_wc_status_kind *status,__int64 time,FIll_STATUS_CALLBACK callback,void *pData, CGitHash *pHash)
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
			if (index >= size() )
				return -1;

			if (time == at(index).m_ModifyTime)
			{
				*status = git_wc_status_normal;
			}
			else
			{
				*status = git_wc_status_modified;
			}

			if (at(index).m_Flags & CE_STAGEMASK )
				*status = git_wc_status_conflicted;
			else if (at(index).m_Flags & CE_INTENT_TO_ADD)
				*status = git_wc_status_added;

			if(pHash)
				*pHash = at(index).m_IndexHash;
		}

	}

	if(callback && status)
			callback(gitdir + _T("\\") + pathorg, *status, false, pData);
	return 0;
}

int CGitIndexList::GetStatus(const CString &gitdir,const CString &pathParam, git_wc_status_kind *status,
							 BOOL IsFull, BOOL IsRecursive,
							 FIll_STATUS_CALLBACK callback,void *pData,
							 CGitHash *pHash)
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
				callback(gitdir + _T("\\") + path, git_wc_status_deleted, false, pData);

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

				for (int i = 0; i < size(); i++)
				{
					if (at(i).m_FileName.GetLength() > len)
					{
						if (at(i).m_FileName.Left(len) == path)
						{
							if (!IsFull)
							{
								*status = git_wc_status_normal;
								if (callback)
									callback(gitdir + _T("\\") + path, *status, false, pData);
								return 0;

							}
							else
							{
								result = g_Git.GetFileModifyTime(gitdir+_T("\\") + at(i).m_FileName, &time);
								if (result)
									continue;

								*status = git_wc_status_none;
								GetFileStatus(gitdir, at(i).m_FileName, status, time, callback, pData);
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
				callback(gitdir + _T("\\") + path, *status, false, pData);

			return 0;

		}
		else
		{
			GetFileStatus(gitdir, path, status, time, callback, pData, pHash);
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

		CString IndexFile = g_AdminDirMap.GetAdminDir(gitdir) + _T("index");

		if(pIndex->ReadIndex(IndexFile))
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
									bool isLoadUpdatedIndex)
{
	try
	{
		CheckAndUpdate(gitdir, isLoadUpdatedIndex);

		SHARED_INDEX_PTR pIndex = this->SafeGet(gitdir);
		if (pIndex.get() != NULL)
		{
			pIndex->GetStatus(gitdir, path, status, IsFull, IsRecursive, callback, pData, pHash);
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
		this->m_PackRefFile = PackRef;
		this->m_LastModifyTimePackRef = mtime;
	}

	int ret = 0;
	{
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

			delete buff;

		} while(0);
	}
	return ret;

}
int CGitHeadFileList::ReadHeadHash(CString gitdir)
{
	int ret = 0;
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
#if 0
int CGitHeadFileList::ReadTree()
{
	int ret;
	if (this->m_Head.IsEmpty())
		return -1;

	try
	{
		CAutoLocker lock(g_Git.m_critGitDllSec);
		CAutoWriteLock lock1(&this->m_SharedMutex);

		if (m_Gitdir != g_Git.m_CurrentDir)
		{
			g_Git.SetCurrentDir(m_Gitdir);
			SetCurrentDirectory(g_Git.m_CurrentDir);
			git_init();
		}

		this->m_Map.clear();
		this->clear();

		ret = git_read_tree(this->m_Head.m_hash, CGitHeadFileList::CallBack, this);
		if (!ret)
			m_TreeHash = m_Head;

	} catch(...)
	{
		return -1;
	}
	return ret;
}
#endif;

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

	unsigned int cur = p->size();
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
	for (int i = 0; i < count; i++)
	{
		const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
		if (entry == NULL)
			continue;
		int mode = git_tree_entry_attributes(entry);
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
		CString base = file.Mid(projectroot.GetLength());
		base.Replace(_T('\\'), _T('/'));

		int start = base.ReverseFind(_T('/'));
		if(start > 0)
		{
			base = base.Left(start);
			this->m_BaseDir = CUnicodeUtils::GetMulti(base, CP_UTF8);
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
		for (int i = 0; i < size; i++)
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
	CString globalConfig = GetWindowsHome() + _T("\\.gitconfig");

	CAutoWriteLock lock(&m_coreExcludefilesSharedMutex);
	hasChanged = CheckAndUpdateMsysGitBinpath();
	CString systemConfig = m_sMsysGitBinPath + _T("\\..\\etc\\gitconfig");

	hasChanged = hasChanged || CheckFileChanged(projectConfig);
	hasChanged = hasChanged || CheckFileChanged(globalConfig);
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
	git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 3);
	projectConfigA.ReleaseBuffer();
	CStringA globalConfigA = CUnicodeUtils::GetMulti(globalConfig, CP_UTF8);
	git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 2);
	globalConfigA.ReleaseBuffer();
	if (!m_sMsysGitBinPath.IsEmpty())
	{
		CStringA systemConfigA = CUnicodeUtils::GetMulti(systemConfig, CP_UTF8);
		git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1);
		systemConfigA.ReleaseBuffer();
	}
	const char * out = NULL;
	CStringA name(_T("core.excludesfile"));
	git_config_get_string(&out, config, name.GetBuffer());
	name.ReleaseBuffer();
	CStringA excludesFileA(out);
	excludesFile = CUnicodeUtils::GetUnicode(excludesFileA);
	if (excludesFile.Find(_T("~/")) == 0)
		excludesFile = GetWindowsHome() + excludesFile.Mid(1);
	git_config_free(config);

	CAutoWriteLock lockMap(&m_SharedMutex);
	g_Git.GetFileModifyTime(projectConfig, &m_Map[projectConfig].m_LastModifyTime);
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
	static CString sWindowsHome(get_windows_home_directory());
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
	return false;
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
#if 0

int CGitStatus::GetStatus(const CString &gitdir, const CString &path, git_wc_status_kind *status, BOOL IsFull, BOOL IsRecursive , FIll_STATUS_CALLBACK callback , void *pData)
{
	int result;
	__int64 time;
	bool dir;

	git_wc_status_kind dirstatus = git_wc_status_none;
	if (status)
	{
		g_Git.GetFileModifyTime(path, &time, &dir);
		if(path.IsEmpty())
			result = _tstat64( gitdir, &buf );
		else
			result = _tstat64( gitdir+_T("\\")+path, &buf );

		if(result)
			return -1;

		if(buf.st_mode & _S_IFDIR)
		{
			if(!path.IsEmpty())
			{
				if( path.Right(1) != _T("\\"))
					path += _T("\\");
			}
			int len = path.GetLength();

			for (int i = 0; i < size(); i++)
			{
				if (at(i).m_FileName.GetLength() > len)
				{
					if (at(i).m_FileName.Left(len) == path)
					{
						if(!IsFull)
						{
							*status = git_wc_status_normal;
							if(callback)
								callback(gitdir + _T("\\") + path, *status, pData);
							return 0;

						}
						else
						{
							result = _tstat64(gitdir + _T("\\") + at(i).m_FileName, &buf);
							if (result)
								continue;

							*status = git_wc_status_none;
							GetFileStatus(gitdir, at(i).m_FileName, status, buf, callback, pData);
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
			}

			if (dirstatus != git_wc_status_none)
			{
				*status = dirstatus;
			}
			else
			{
				*status = git_wc_status_unversioned;
			}
			if(callback)
				callback(gitdir + _T("\\") + path, *status, pData);

			return 0;

		}
		else
		{
			GetFileStatus(gitdir, path, status, buf, callback, pData);
		}
	}
	return 0;

}
#endif
