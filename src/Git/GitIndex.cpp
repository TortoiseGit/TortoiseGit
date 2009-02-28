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

#define FILL_DATA() \
	m_FileName.Empty();\
	g_Git.StringAppend(&m_FileName,(BYTE*)entry->name,CP_ACP,Big2lit(entry->flags)&CE_NAMEMASK);\
	m_FileName.Replace(_T('/'),_T('\\'));\
	this->m_Flags=Big2lit(entry->flags);\
	this->m_ModifyTime=Big2lit(entry->mtime.sec);
	
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

CGitIndexList::CGitIndexList()
{
	this->m_LastModifyTime=0;
}

int CGitIndexList::ReadIndex(CString IndexFile)
{
	HANDLE hfile=0;
	int ret=0;
	BYTE *buffer=NULL,*p;
	CGitIndex GitIndex;

	try
	{
		do
		{
			this->clear();
			this->m_Map.clear();

			hfile = CreateFile(IndexFile,
									GENERIC_READ,
									FILE_SHARE_READ,
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

int CGitIndexList::GetFileStatus(CString &gitdir,CString &path,git_wc_status_kind *status,struct __stat64 &buf,FIll_STATUS_CALLBACK callback,void *pData)
{

	if(status)
	{
		if(m_Map.find(path) == m_Map.end() )
		{
			*status = git_wc_status_unversioned;
		}else
		{
 			int index = m_Map[path];
			if(index <0)
				return -1;
			if(index >= size() )
				return -1;
		
			if( buf.st_mtime ==  at(index).m_ModifyTime )
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

		}
		if(callback)
			callback(gitdir+_T("\\")+path,*status,pData);
	}
	return 0;
}

int CGitIndexList::GetStatus(CString &gitdir,CString &path, git_wc_status_kind *status,
							 BOOL IsFull, BOOL IsRecursive,
							 FIll_STATUS_CALLBACK callback,void *pData)
{
	int result;
	struct __stat64 buf;
	git_wc_status_kind dirstatus = git_wc_status_none;
	if(status)
	{
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

int CGitIndexFileMap::GetFileStatus(CString &gitdir, CString &path, git_wc_status_kind *status,BOOL IsFull, BOOL IsRecursive,
									FIll_STATUS_CALLBACK callback,void *pData)
{
	struct __stat64 buf;
	int result;
	try
	{
		CString IndexFile;
		IndexFile=gitdir+_T("\\.git\\index");
		/* Get data associated with "crt_stat.c": */
		result = _tstat64( IndexFile, &buf );

//		WIN32_FILE_ATTRIBUTE_DATA FileInfo;
//		GetFileAttributesEx(_T("D:\\tortoisegit\\src\\gpl.txt"),GetFileExInfoStandard,&FileInfo);
//		result = _tstat64( _T("D:\\tortoisegit\\src\\gpl.txt"), &buf );

		if(result)
		   return result;

		if((*this)[IndexFile].m_LastModifyTime != buf.st_mtime )
		{
			if((*this)[IndexFile].ReadIndex(IndexFile))
				return -1;
		}
		(*this)[IndexFile].m_LastModifyTime = buf.st_mtime;

		(*this)[IndexFile].GetStatus(gitdir,path,status,IsFull,IsRecursive,callback,pData);
				
	}catch(...)
	{
		return -1;
	}
	return 0;
}