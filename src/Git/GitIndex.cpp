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

typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

#define FILL_DATA() \
	m_FileName.Empty();\
	g_Git.StringAppend(&m_FileName,(BYTE*)entry->name,CP_ACP,Big2lit(entry->flags)&CE_NAMEMASK);\
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
	this->m_LastModifyTime=0;
}

int CGitIndexList::ReadIndex(CString IndexFile)
{
	HANDLE hfile=INVALID_HANDLE_VALUE;
	int ret=0;
	BYTE *buffer=NULL,*p;
	CGitIndex GitIndex;

	try
	{
		do
		{
			{
				CAutoWriteLock lock(&this->m_SharedMutex);
				this->clear();
				this->m_Map.clear();
			}

			hfile = CreateFile(IndexFile,
									GENERIC_READ,
									FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
									NULL,
									OPEN_EXISTING,
									FILE_ATTRIBUTE_NORMAL,
									NULL);


			if(hfile == INVALID_HANDLE_VALUE)
			{
				ret = -1 ;
				break;
			}

			cache_header *header;
			DWORD size=0,filesize=0;
			
			filesize=GetFileSize(hfile,NULL);

			if(filesize == INVALID_FILE_SIZE )
			{
				ret =-1;
				break;
			}

			buffer = new BYTE[filesize];
			p=buffer;
			if(buffer == NULL)
			{
				ret = -1;
				break;
			}
			if(! ReadFile( hfile, buffer,filesize,&size,NULL) )
			{
				ret = GetLastError();
				break;
			}
			
			CloseHandle(hfile);
			hfile = INVALID_HANDLE_VALUE; /* close early to above lock time long*/

			if (size != filesize)
			{
				ret = -1;
				break;
			}
			header = (cache_header *) buffer;
			if( Big2lit(header->hdr_signature) != CACHE_SIGNATURE )
			{
				ret = -1;
				break;
			}
			p+= sizeof(cache_header);

			int entries = Big2lit(header->hdr_entries);

			{
				CAutoWriteLock lock(&this->m_SharedMutex);

				for(int i=0;i<entries;i++)
				{
					ondisk_cache_entry *entry;
					ondisk_cache_entry_extended *entryex;
					entry=(ondisk_cache_entry*)p;
					entryex=(ondisk_cache_entry_extended*)p;
					int flags=Big2lit(entry->flags);
					if( flags & CE_EXTENDED)
					{
						GitIndex.FillData(entryex);
						p+=ondisk_ce_size(entryex);

					}else
					{
						GitIndex.FillData(entry);
						p+=ondisk_ce_size(entry);
					}

					if(p>buffer+filesize)
					{
						ret = -1;
						break;
					}

					this->push_back(GitIndex);
					this->m_Map[GitIndex.m_FileName]=this->size()-1;
				}
			}
		}while(0);
	}catch(...)
	{
		ret= -1;
	}

	if(hfile != INVALID_HANDLE_VALUE)
		CloseHandle(hfile);
	if(buffer)
		delete buffer;
	return ret;
}

int CGitIndexList::GetFileStatus(CString &gitdir,CString &path,git_wc_status_kind *status,__int64 time,FIll_STATUS_CALLBACK callback,void *pData, CGitHash *pHash)
{

	if(status)
	{
		CAutoReadLock lock(&this->m_SharedMutex);

		if(m_Map.find(path) == m_Map.end() )
		{
			*status = git_wc_status_unversioned;

			if(pHash)
				pHash->Empty();

		}else
		{
 			int index = m_Map[path];
			if(index <0)
				return -1;
			if(index >= size() )
				return -1;
		
			if( time ==  at(index).m_ModifyTime )
			{
				*status = git_wc_status_normal;
			}else
			{
				*status = git_wc_status_modified;
			}

			if(at(index).m_Flags & CE_STAGEMASK )
				*status = git_wc_status_conflicted;
			else if(at(index).m_Flags & CE_INTENT_TO_ADD)
				*status = git_wc_status_added;

			if(pHash)
				*pHash = at(index).m_IndexHash;
		}
		
	}

	if(callback && status)
			callback(gitdir+_T("\\")+path,*status,false, pData);
	return 0;
}

int CGitIndexList::GetStatus(CString &gitdir,CString &path, git_wc_status_kind *status,
							 BOOL IsFull, BOOL IsRecursive,
							 FIll_STATUS_CALLBACK callback,void *pData,
							 CGitHash *pHash)
{
	int result;
	git_wc_status_kind dirstatus = git_wc_status_none;
	__int64 time;
	bool isDir=false;

	if(status)
	{
		if(path.IsEmpty())
			result = g_Git.GetFileModifyTime(gitdir,&time,&isDir);
		else
			result = g_Git.GetFileModifyTime( gitdir+_T("\\")+path, &time, &isDir );

		if(result)
		{
			*status = git_wc_status_deleted;
			if(callback)
				callback(gitdir+_T("\\")+path,git_wc_status_deleted,false, pData);

			return 0;
		}
		if(isDir)
		{
			if(!path.IsEmpty())
			{
				if( path.Right(1) != _T("\\"))
					path+=_T("\\");
			}
			int len =path.GetLength();

			{
				CAutoReadLock lock(&this->m_SharedMutex);

				for(int i=0;i<size();i++)
				{
					if( at(i).m_FileName.GetLength() > len )
					{
						if(at(i).m_FileName.Left(len) == path)
						{
							if( !IsFull )
							{
								*status = git_wc_status_normal; 
								if(callback)
									callback(gitdir+_T("\\")+path,*status,false, pData);
								return 0;

							}else
							{	
								result = g_Git.GetFileModifyTime( gitdir+_T("\\")+at(i).m_FileName , &time);
								if(result)
									continue;

								*status = git_wc_status_none;
								GetFileStatus(gitdir,at(i).m_FileName,status,time,callback,pData);
								if( *status != git_wc_status_none )
								{
									if( dirstatus == git_wc_status_none)
									{
										dirstatus = git_wc_status_normal;
									}
									if( *status != git_wc_status_normal )
									{
										dirstatus = git_wc_status_modified;
									}
								}

							}
						}
					}
				} /* End For */

			} /* End Auto Lock */

			if( dirstatus != git_wc_status_none )
			{
				*status = dirstatus;
			}
			else
			{
				*status = git_wc_status_unversioned;
			}
			if(callback)
				callback(gitdir+_T("\\")+path,*status,false, pData);
							
			return 0;

		}else
		{
			GetFileStatus(gitdir,path,status,time,callback,pData,pHash);
		}
	}	
	return 0;
}

int CGitIndexFileMap::CheckAndUpdateIndex(CString &gitdir,bool *loaded)
{
	__int64 time;
	int result;

	try
	{
		CString IndexFile;
		IndexFile=gitdir+_T("\\.git\\index");
		/* Get data associated with "crt_stat.c": */
		result = g_Git.GetFileModifyTime( IndexFile, &time );

//		WIN32_FILE_ATTRIBUTE_DATA FileInfo;
//		GetFileAttributesEx(_T("D:\\tortoisegit\\src\\gpl.txt"),GetFileExInfoStandard,&FileInfo);
//		result = _tstat64( _T("D:\\tortoisegit\\src\\gpl.txt"), &buf );
		
		if(loaded)
			*loaded = false;

		if(result)
		   return result;

		{
			this->m_SharedMutex.AcquireShared();
			if( this->find(gitdir) == this->end() )
			{

				m_SharedMutex.ReleaseShared();

				m_SharedMutex.AcquireExclusive();
				(*this)[gitdir].m_LastModifyTime = 0; /* insert new item*/
				(*this)[gitdir].m_SharedMutex.Init();
				m_SharedMutex.ReleaseExclusive();
				m_SharedMutex.AcquireShared();
			}

			if((*this)[gitdir].m_LastModifyTime != time )
			{
				if((*this)[gitdir].ReadIndex(IndexFile))
				{
					m_SharedMutex.ReleaseShared();
					return -1;
				}
				if(loaded)
					*loaded =true;
			}

			(*this)[gitdir].m_LastModifyTime = time;

			m_SharedMutex.ReleaseShared();
		}
				
	}catch(...)
	{
		return -1;
	}
	return 0;
}

int CGitIndexFileMap::GetFileStatus(CString &gitdir, CString &path, git_wc_status_kind *status,BOOL IsFull, BOOL IsRecursive,
									FIll_STATUS_CALLBACK callback,void *pData,
									CGitHash *pHash)
{
	int result;
	try
	{
		CheckAndUpdateIndex(gitdir);

		{
			CAutoReadLock lock(&this->m_SharedMutex);
			(*this)[gitdir].GetStatus(gitdir,path,status,IsFull,IsRecursive,callback,pData,pHash);
		}
				
	}catch(...)
	{
		return -1;
	}
	return 0;
}

int CGitIndexFileMap::IsUnderVersionControl(CString &gitdir, CString &path, bool isDir,bool *isVersion)
{
	try
	{	
		if(path.IsEmpty())
		{
			*isVersion =true;
			return 0;
		}
		CString subpath=path;
		subpath.Replace(_T('\\'), _T('/'));
		if(isDir)
			subpath+=_T('/');
		
		CheckAndUpdateIndex(gitdir);
		if(isDir)
		{
			CAutoReadLock lock(&this->m_SharedMutex);
			CAutoReadLock lock1(&(*this)[gitdir].m_SharedMutex);

			*isVersion = (SearchInSortVector((*this)[gitdir], subpath.GetBuffer(), subpath.GetLength()) >= 0);
		}
		else
		{
			CAutoReadLock lock(&this->m_SharedMutex);
			CAutoReadLock lock1(&(*this)[gitdir].m_SharedMutex);

			*isVersion = ((*this)[gitdir].m_Map.find(subpath) != (*this)[gitdir].m_Map.end());
		}

	}catch(...)
	{
		return -1;
	}
	return 0;
}

int CGitHeadFileList::GetPackRef(CString &gitdir)
{
	CString PackRef =  gitdir;
	PackRef += _T("\\.git\\packed-refs");

	__int64 mtime;
	if( g_Git.GetFileModifyTime(PackRef, &mtime))
	{
		//packed refs is not existed
		this->m_PackRefFile.Empty();
		this->m_PackRefMap.clear();
		return 0;

	}else if(mtime == m_LastModifyTimePackRef)
	{
		return 0;

	}else
	{
		this->m_PackRefFile = PackRef;
		this->m_LastModifyTimePackRef = mtime;
	}

	this->m_PackRefMap.clear();

	int ret =0;
	HANDLE hfile = CreateFile(PackRef,
				GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
	do
	{
		if(hfile == INVALID_HANDLE_VALUE)
		{
			ret = -1;
			break;
		}
		
		int filesize = GetFileSize(hfile,NULL);
		DWORD size =0;
		char *buff;
		buff = new char[filesize];

		ReadFile(hfile,buff,filesize,&size,NULL);

		if(size != filesize)
		{
			ret = -1;
			break;
		}

		CString hash;
		CString ref;
		
		for(int i=0;i<filesize;i++)
		{
			hash.Empty();
			ref.Empty();
			if(buff[i] == '#')
			{
				while(buff[i] != '\n')
				{	i++;
					if( i==filesize)
						break;
				}
				i++;
			}

			if( i== filesize)
				break;

			while(buff[i] != ' ')
			{
				hash.AppendChar(buff[i]);
				i++;
				if(i==filesize)
					break;
			}

			i++;
			if( i== filesize)
				break;

			while(buff[i] != '\n')
			{
				ref.AppendChar(buff[i]);
				i++;
				if( i== filesize)
					break;
			}
			
			i++;

			if( !ref.IsEmpty() )
			{
				this->m_PackRefMap[ref] = hash;
			}

			
		}

		delete buff;

	}while(0);

	if( hfile != INVALID_HANDLE_VALUE)
		CloseHandle(hfile);

	return ret;

}
int CGitHeadFileList::ReadHeadHash(CString gitdir)
{
	CString HeadFile = gitdir;
	HeadFile += _T("\\.git\\HEAD");

	
	HANDLE hfile=INVALID_HANDLE_VALUE;
	HANDLE href = INVALID_HANDLE_VALUE;

	int ret = 0;
	m_Gitdir = gitdir;

	m_HeadFile = HeadFile;

	if( g_Git.GetFileModifyTime(m_HeadFile,&m_LastModifyTimeHead))
		return -1;

	try
	{
		CAutoWriteLock lock(&this->m_SharedMutex);

		do
		{
			hfile = CreateFile(HeadFile,
				GENERIC_READ,
				FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
				NULL,
				OPEN_EXISTING,
				FILE_ATTRIBUTE_NORMAL,
				NULL);

			if(hfile == INVALID_HANDLE_VALUE)
			{
				ret = -1;
				break;
			}

			DWORD size=0,filesize=0;
			unsigned char buffer[40] ;		
			ReadFile(hfile,buffer,4,&size,NULL);
			if(size !=4)
			{
				ret = -1;
				break;
			}
			buffer[4]=0;
			if(strcmp((const char*)buffer,"ref:") == 0)
			{
				filesize = GetFileSize(hfile,NULL);

				unsigned char *p = (unsigned char*)malloc(filesize -4);

				ReadFile(hfile,p,filesize-4,&size,NULL);

				m_HeadRefFile.Empty();
				g_Git.StringAppend(&this->m_HeadRefFile,p,CP_ACP,filesize-4);
				CString ref = this->m_HeadRefFile;
				ref=ref.Trim();
				int start =0;
				ref=ref.Tokenize(_T("\n"),start);
				free(p);
				m_HeadRefFile=gitdir+_T("\\.git\\")+m_HeadRefFile.Trim();
				m_HeadRefFile.Replace(_T('/'),_T('\\'));

				__int64 time;
				if(g_Git.GetFileModifyTime(m_HeadRefFile,&time,NULL))
				{
					m_HeadRefFile.Empty();
					if( GetPackRef(gitdir))
					{
						ret = -1;
						break;
					}
					if(this->m_PackRefMap.find(ref) == m_PackRefMap.end())
					{
						ret = -1;
						break;
					}
					this ->m_Head = m_PackRefMap[ref];
					ret =0;
					break;
				}

				href = CreateFile(m_HeadRefFile,
					GENERIC_READ,
					FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					FILE_ATTRIBUTE_NORMAL,
					NULL);

				if(href == INVALID_HANDLE_VALUE)
				{
					m_HeadRefFile.Empty();

					if( GetPackRef(gitdir))
					{
						ret = -1;
						break;
					}

					if(this->m_PackRefMap.find(ref) == m_PackRefMap.end())
					{
						ret = -1;
						break;
					}
					this ->m_Head = m_PackRefMap[ref];
					ret =0;
					break;
				}
				ReadFile(href,buffer,40,&size,NULL);
				if(size != 40)
				{
					ret =-1;
					break;
				}
				this->m_Head.ConvertFromStrA((char*)buffer);

				this->m_LastModifyTimeRef = time;

			}else
			{
				ReadFile(hfile,buffer+4,40-4,&size,NULL);
				if(size !=36)
				{
					ret =-1;
					break;
				}
				m_HeadRefFile.Empty();

				this->m_Head.ConvertFromStrA((char*)buffer);
			}
		}while(0);
	}catch(...)
	{
		ret = -1;
	}

	if(hfile != INVALID_HANDLE_VALUE)
		CloseHandle(hfile);		
	if(href != INVALID_HANDLE_VALUE)
		CloseHandle(href);

	return ret;
}

bool CGitHeadFileList::CheckHeadUpdate()
{
	if(this->m_HeadFile.IsEmpty())
		return true;

	__int64 mtime=0;
	 
	if( g_Git.GetFileModifyTime(m_HeadFile,&mtime))
		return true;

	if(mtime != this->m_LastModifyTimeHead)
		return true;

	if(!this->m_HeadRefFile.IsEmpty())
	{
		if(g_Git.GetFileModifyTime(m_HeadRefFile,&mtime))
			return true;
		
		if(mtime != this->m_LastModifyTimeRef)
			return true;
	}

	if(!this->m_PackRefFile.IsEmpty())
	{
		if(g_Git.GetFileModifyTime(m_PackRefFile,&mtime))
			return true;

		if(mtime != this->m_LastModifyTimePackRef)
			return true;
	}
	return false;
}

int CGitHeadFileList::ReadTree()
{
	int ret;
	if( this->m_Head.IsEmpty())
		return -1;

	CAutoLocker lock(g_Git.m_critGitDllSec);
	CAutoWriteLock lock1(&this->m_SharedMutex);

	if(m_Gitdir != g_Git.m_CurrentDir)
	{
		g_Git.SetCurrentDir(m_Gitdir);
		SetCurrentDirectory(g_Git.m_CurrentDir);
		git_init();	
	}
	
	this->m_Map.clear();
	this->clear();

	ret = git_read_tree(this->m_Head.m_hash,CGitHeadFileList::CallBack,this);
	if(!ret)
		m_TreeHash = m_Head;

	return ret;
}

int CGitHeadFileList::CallBack(const unsigned char *sha1, const char *base, int baselen,
		const char *pathname, unsigned mode, int stage, void *context)
{
#define S_IFGITLINK	0160000

	CGitHeadFileList *p = (CGitHeadFileList*)context;
	if( mode&S_IFDIR )
	{
		if( (mode&S_IFMT) != S_IFGITLINK)
			return READ_TREE_RECURSIVE;
	}

	unsigned int cur = p->size();
	p->resize(p->size()+1);
	p->at(cur).m_Hash = (char*)sha1;
	p->at(cur).m_FileName.Empty();
	
	if(base)
		g_Git.StringAppend(&p->at(cur).m_FileName,(BYTE*)base,CP_ACP,baselen);

	g_Git.StringAppend(&p->at(cur).m_FileName,(BYTE*)pathname,CP_ACP);

	//p->at(cur).m_FileName.Replace(_T('/'),_T('\\'));

	p->m_Map[p->at(cur).m_FileName]=cur;

	if( (mode&S_IFMT) == S_IFGITLINK)
		return 0;

	return READ_TREE_RECURSIVE;
}

int CGitIgnoreItem::FetchIgnoreList(CString &projectroot, CString &file)
{
	if(this->m_pExcludeList)
	{
		free(m_pExcludeList);
		m_pExcludeList=NULL;
	}

	this->m_BaseDir.Empty();
	if( projectroot.GetLength() < file.GetLength())
	{
		CString base = file.Mid(projectroot.GetLength()+1);
		base.Replace(_T('\\'), _T('/'));
		if(base != _T(".git/info/exclude"))
		{
			int start=base.ReverseFind(_T('/'));
			if(start>=0)
			{
				base=base.Left(start);
				this->m_BaseDir = CUnicodeUtils::GetMulti(base,CP_ACP) ;
			}
		}
	}
	{

		if(g_Git.GetFileModifyTime(file,&m_LastModifyTime))
			return -1;

		if(git_create_exclude_list(&this->m_pExcludeList))
			return -1;


		HANDLE hfile = CreateFile(file,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);


		if(hfile == INVALID_HANDLE_VALUE)
		{
			return -1 ;
		}

		DWORD size=0,filesize=0;

		filesize=GetFileSize(hfile,NULL);

		if(filesize == INVALID_FILE_SIZE )
		{
			CloseHandle(hfile);
			return -1;
		}

		BYTE *buffer = new BYTE[filesize+1];

		if(buffer == NULL)
		{
			CloseHandle(hfile);
			return -1;
		}

		if(! ReadFile( hfile, buffer,filesize,&size,NULL) )
		{
			CloseHandle(hfile);
			return GetLastError();
		}

		CloseHandle(hfile);

		BYTE *p = buffer;
		for(int i=0;i<size;i++)
		{
			if( buffer[i] == '\n' || buffer[i] =='\r' || i==(size-1) )
			{
				if (buffer[i] == '\n' || buffer[i] =='\r')
					buffer[i]=0;
				if( i== size-1)
					buffer[size]=0;
				
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
		//buffer=NULL;
	}
	return 0;
}

bool CGitIgnoreList::CheckFileChanged(CString &path)
{
	__int64 time=0;

	int ret=g_Git.GetFileModifyTime(path, &time);

	this->m_SharedMutex.AcquireShared();
	bool cacheExist = (m_Map.find(path) != m_Map.end());
	this->m_SharedMutex.ReleaseShared();

	// both cache and file is not exist  
	if( (ret != 0) && (!cacheExist))
		return false;

	// file exist but cache miss
	if( (ret == 0) && (!cacheExist))
		return true;

	// file not exist but cache exist
	if( (ret != 0) && (cacheExist))
	{
		return true;
	}
	// file exist and cache exist
	if (! cacheExist )
	{
		CAutoWriteLock lock(&this->m_SharedMutex);
		m_Map[path].m_LastModifyTime = 0;
	}
	{
		CAutoReadLock lock(&this->m_SharedMutex);
		if( m_Map[path].m_LastModifyTime == time )
			return false;
	}
	return true;

}

bool CGitIgnoreList::CheckIgnoreChanged(CString &gitdir,CString &path)
{
	CString temp;
	temp=gitdir;
	temp+=_T("\\");
	temp+=path;

	temp.Replace(_T('/'), _T('\\'));

	while(!temp.IsEmpty())
	{
		temp+=_T("\\.git");

		if(CGit::GitPathFileExists(temp))
		{
			CString gitignore=temp;
			gitignore += _T("ignore");
			if( CheckFileChanged(gitignore) )
				return true;

			temp+=_T("\\info\\exclude");

			if( CheckFileChanged(temp) )
				return true;
			else
				return false;
		}else
		{
			temp+=_T("ignore");
			if( CheckFileChanged(temp) )
				return true;
		}

		int found=0;
		int i;
		for( i=temp.GetLength() -1;i>=0;i--)
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

int CGitIgnoreList::LoadAllIgnoreFile(CString &gitdir,CString &path)
{
	CString temp;
	
	temp=gitdir;
	temp+=_T("\\");
	temp+=path;

	temp.Replace(_T('/'), _T('\\'));

	while(!temp.IsEmpty())
	{
		temp+=_T("\\.git");

		if(CGit::GitPathFileExists(temp))
		{
			CString gitignore = temp;
			gitignore += _T("ignore");
			if( CheckFileChanged(gitignore) )
			{
				if(CGit::GitPathFileExists(temp)) //if .gitignore remove, we need remote cache
				{
					CAutoWriteLock lock(&this->m_SharedMutex);
					m_Map[gitignore].FetchIgnoreList(gitdir,gitignore);
				}
				else
				{
					CAutoWriteLock lock(&this->m_SharedMutex);
					m_Map.erase(gitignore);
				}
			}

			temp+=_T("\\info\\exclude");

			if( CheckFileChanged(temp) )
			{
				if(CGit::GitPathFileExists(temp)) //if .gitignore remove, we need remote cache
				{
					CAutoWriteLock lock(&this->m_SharedMutex);
					return m_Map[temp].FetchIgnoreList(gitdir,temp);
				}
				else
				{
					CAutoWriteLock lock(&this->m_SharedMutex);
					m_Map.erase(temp);
					return 0;
				}
			}

			return 0;

		}else
		{
			temp+=_T("ignore");
			if( CheckFileChanged(temp) )
			{
				if(CGit::GitPathFileExists(temp)) //if .gitignore remove, we need remote cache
				{
					CAutoWriteLock lock(&this->m_SharedMutex);
					m_Map[temp].FetchIgnoreList(gitdir,temp);
				}
				else
				{
					CAutoWriteLock lock(&this->m_SharedMutex);
					m_Map.erase(temp);
				}
			}
		}

		int found=0;
		int i;
		for( i=temp.GetLength() -1;i>=0;i--)
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
int CGitIgnoreList::GetIgnoreFileChangeTimeList(CString &path, std::vector<__int64> &timelist)
{
	CString temp=path;
	CString ignore=temp;
	int start =0;

	do{
		CAutoReadLock lock(&this->m_SharedMutex);

		ignore=temp;
		ignore+=_T("\\.gitignore");
		std::map<CString, CGitIgnoreItem>::iterator itMap;
		itMap = m_Map.find(ignore);
		if(itMap == m_Map.end())
		{
			timelist.push_back(0);

		}else
		{
			timelist.push_back(itMap->second.m_LastModifyTime);
		}

		ignore =temp;
		ignore += _T("\\.git\\info\\exclude");
		
		itMap = m_Map.find(ignore);
		if(itMap == m_Map.end())
		{

		}else
		{
			timelist.push_back(itMap->second.m_LastModifyTime);
			return 0;
		}

		ignore=temp;
		ignore+=_T("\\.git");

		if(CGit::GitPathFileExists(ignore))
			return 0;

		start = temp.ReverseFind(_T('\\'));
		if(start>0)
			temp=temp.Left(start);

	}while(start>0);

	return -1;

}
bool CGitIgnoreList::IsIgnore(CString &path,CString &projectroot)
{
	CString str=path;
	
	str.Replace(_T('\\'),_T('/'));

	int ret;
	ret = CheckIgnore(path, projectroot);
	while(ret < 0)
	{
		int start=str.ReverseFind(_T('/'));
		if(start<0)
			return (ret == 1);
		
		str=str.Left(start);
		ret = CheckIgnore(str, projectroot);
	}

	return (ret == 1);
}
int CGitIgnoreList::CheckIgnore(CString &path,CString &projectroot)
{
	__int64 time=0;
	bool dir=0;
	CString temp=projectroot+_T("\\")+path;
	temp.Replace(_T('/'), _T('\\'));

	CStringA patha;

	patha = CUnicodeUtils::GetMulti(path,CP_ACP) ;
	patha.Replace('\\','/');

	if(g_Git.GetFileModifyTime(temp,&time,&dir))
		return -1;

	int type=0;
	if( dir )
		type = DT_DIR;
	else
		type = DT_REG;

	while(!temp.IsEmpty())
	{
		int x;
		x=temp.ReverseFind(_T('\\'));
		if(x<0)	x=0;
			temp=temp.Left(x);

		temp+=_T("\\.gitignore");

		char *base;
			
		patha.Replace('\\', '/');
		int pos=patha.ReverseFind('/');
		base = pos>=0? patha.GetBuffer()+pos+1:patha.GetBuffer();

		CAutoReadLock lock(&this->m_SharedMutex);

		if(this->m_Map.find(temp) == m_Map.end() )
		{

		}else
		{
			int ret=-1;

			if(m_Map[temp].m_pExcludeList)
				ret = git_check_excluded_1( patha, patha.GetLength(), base, &type, m_Map[temp].m_pExcludeList);
			
			if(ret == 1)
				return 1;
			if(ret == 0)
				return 0;
		}

		temp = temp.Left(temp.GetLength()-11);
		temp +=_T("\\.git\\info\\exclude");

		if(this->m_Map.find(temp) == m_Map.end() )
		{

		}else
		{
			int ret=-1;
			
			if(m_Map[temp].m_pExcludeList)
				ret = git_check_excluded_1( patha, patha.GetLength(), base, &type, m_Map[temp].m_pExcludeList);

			if(ret == 1)
				return 1;
			if(ret == 0)
				return 0;

			return -1;
		}
		temp = temp.Left(temp.GetLength()-18);
	}
	
	return -1;
}

int CGitHeadFileMap::CheckHeadUpdate(CString &gitdir)
{
	m_SharedMutex.AcquireShared();
	if( find(gitdir) == end())
	{
		m_SharedMutex.ReleaseShared();
		m_SharedMutex.AcquireExclusive();
		(*this)[gitdir].m_LastModifyTimeRef = 0; /* Insert new item*/
		(*this)[gitdir].m_SharedMutex.Init();
		m_SharedMutex.ReleaseExclusive();
	}else
	{
		m_SharedMutex.ReleaseShared();
	}

	m_SharedMutex.AcquireShared();
	int ret= (*this)[gitdir].CheckHeadUpdate();
	m_SharedMutex.ReleaseShared();

	return ret;
}

int CGitHeadFileMap::GetHeadHash(CString &gitdir, CGitHash &hash)
{
	if(CheckHeadUpdate(gitdir))
	{
		CAutoReadLock(&this->m_SharedMutex);
		(*this)[gitdir].ReadHeadHash(gitdir);
	
	}else
	{
		CAutoReadLock(&this->m_SharedMutex);
		hash = (*this)[gitdir].m_Head;
	}
	return 0;
}
#if 0

int CGitStatus::GetStatus(CString &gitdir, CString &path, git_wc_status_kind *status, BOOL IsFull, BOOL IsRecursive , FIll_STATUS_CALLBACK callback , void *pData)
{
	int result;
	__int64 time;
	bool	dir;

	git_wc_status_kind dirstatus = git_wc_status_none;
	if(status)
	{
		g_Git.GetFileModifyTime(path,&time,&dir);
		if( dir)
		{
		}else
		{
			
		}
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
					path+=_T("\\");
			}
			int len =path.GetLength();

			for(int i=0;i<size();i++)
			{
				if( at(i).m_FileName.GetLength() > len )
				{
					if(at(i).m_FileName.Left(len) == path)
					{
						if( !IsFull )
						{
							*status = git_wc_status_normal; 
							if(callback)
								callback(gitdir+_T("\\")+path,*status,pData);
							return 0;

						}else
						{	
							result = _tstat64( gitdir+_T("\\")+at(i).m_FileName, &buf );
							if(result)
								continue;
							
							*status = git_wc_status_none;
							GetFileStatus(gitdir,at(i).m_FileName,status,buf,callback,pData);
							if( *status != git_wc_status_none )
							{
								if( dirstatus == git_wc_status_none)
								{
									dirstatus = git_wc_status_normal;
								}
								if( *status != git_wc_status_normal )
								{
									dirstatus = git_wc_status_modified;
								}
							}
							
						}
					}
				}
			}

			if( dirstatus != git_wc_status_none )
			{
				*status = dirstatus;
			}
			else
			{
				*status = git_wc_status_unversioned;
			}
			if(callback)
				callback(gitdir+_T("\\")+path,*status,pData);
							
			return 0;

		}else
		{
			GetFileStatus(gitdir,path,status,buf,callback,pData);
		}
	}	
	return 0;

}
#endif