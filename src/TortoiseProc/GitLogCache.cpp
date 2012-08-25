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
#include "stdafx.h"
#include "GitLogCache.h"

int static Compare(const void *p1, const void*p2)
{
	return memcmp(p1,p2,20);
}

CLogCache::CLogCache()
{
	m_IndexFile = INVALID_HANDLE_VALUE;
	m_IndexFileMap = INVALID_HANDLE_VALUE;
	m_pCacheIndex = NULL;

	m_DataFile = INVALID_HANDLE_VALUE;
	m_DataFileMap = INVALID_HANDLE_VALUE;
	m_pCacheData = NULL;
}

void CLogCache::CloseDataHandles()
{
	if(m_pCacheData)
	{
		UnmapViewOfFile(m_pCacheData);
		m_pCacheData=NULL;
	}
	if(m_DataFileMap)
	{
		CloseHandle(m_DataFileMap);
		m_DataFileMap=INVALID_HANDLE_VALUE;
	}
	if(m_DataFile)
	{
		CloseHandle(m_DataFile);
		m_DataFile = INVALID_HANDLE_VALUE;
	}
}

void CLogCache::CloseIndexHandles()
{
	if(m_pCacheIndex)
	{
		UnmapViewOfFile(m_pCacheIndex);
		m_pCacheIndex = NULL;
	}

	if(m_IndexFileMap)
	{
		CloseHandle(m_IndexFileMap);
		m_IndexFileMap = INVALID_HANDLE_VALUE;
	}

	//this->m_IndexFile.Close();
	//this->m_DataFile.Close();
	if(m_IndexFile)
	{
		CloseHandle(m_IndexFile);
		m_IndexFile=INVALID_HANDLE_VALUE;
	}

}
CLogCache::~CLogCache()
{
	CloseIndexHandles();
	CloseDataHandles();
}

int CLogCache::AddCacheEntry(GitRev &Rev)
{
	this->m_HashMap[Rev.m_CommitHash] = Rev;
	return 0;
}

GitRev * CLogCache::GetCacheData(CGitHash &hash)
{
	m_HashMap[hash].m_CommitHash=hash;
	return &m_HashMap[hash];
}

ULONGLONG CLogCache::GetOffset(CGitHash &hash,SLogCacheIndexFile *pData)
{

	if(pData==NULL)
		pData = m_pCacheIndex;

	if(pData == 0)
		return 0;

	SLogCacheIndexItem *p=(SLogCacheIndexItem *)bsearch(hash.m_hash,pData->m_Item,
			pData->m_Header.m_ItemCount,
			sizeof(SLogCacheIndexItem),
			Compare);

	if(p)
		return p->m_Offset;
	else
		return 0;
}

int CLogCache::FetchCacheIndex(CString GitDir)
{
	int ret=0;
	if (!g_GitAdminDir.GetAdminDirPath(GitDir, m_GitDir))
		return -1;

	do
	{
		if( m_IndexFile == INVALID_HANDLE_VALUE)
		{
			CString file = m_GitDir + INDEX_FILE_NAME;
			m_IndexFile = CreateFile(file,
						GENERIC_READ,
						FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

			if( m_IndexFile == INVALID_HANDLE_VALUE)
			{
				ret = -1;
				break;;
			}
		}

		if( m_IndexFileMap == INVALID_HANDLE_VALUE)
		{
			m_IndexFileMap = CreateFileMapping(m_IndexFile, NULL, PAGE_READONLY,0,0,NULL);
			if( m_IndexFileMap == INVALID_HANDLE_VALUE)
			{
				ret = -1;
				break;
			}
		}

		if( m_pCacheIndex == NULL )
		{
			m_pCacheIndex = (SLogCacheIndexFile*)MapViewOfFile(m_IndexFileMap,FILE_MAP_READ,0,0,0);
			if( m_pCacheIndex ==NULL )
			{
				ret = -1;
				break;
			}
		}

		if( !CheckHeader(&m_pCacheIndex->m_Header))
		{
			ret =-1;
			break;
		}

		if(	m_DataFile == INVALID_HANDLE_VALUE )
		{
			CString file = m_GitDir + DATA_FILE_NAME;
			m_DataFile = CreateFile(file,
						GENERIC_READ,
						FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

			if(m_DataFile == INVALID_HANDLE_VALUE)
			{
				ret =-1;
				break;
			}
		}

		if( m_DataFileMap == INVALID_HANDLE_VALUE)
		{
			m_DataFileMap = CreateFileMapping(m_DataFile, NULL, PAGE_READONLY,0,0,NULL);
			if( m_DataFileMap == INVALID_HANDLE_VALUE)
			{
				ret = -1;
				break;
			}
		}

		if(	m_pCacheData == NULL)
		{
			m_pCacheData = (BYTE*)MapViewOfFile(m_DataFileMap,FILE_MAP_READ,0,0,0);
			if( m_pCacheData ==NULL )
			{
				ret = -1;
				break;
			}
		}

		if(!CheckHeader( (SLogCacheDataFileHeader*)m_pCacheData))
		{
			ret = -1;
			break;
		}

	}while(0);

	if(ret)
	{
		CloseIndexHandles();
		::DeleteFile(m_GitDir + INDEX_FILE_NAME);
		::DeleteFile(m_GitDir + DATA_FILE_NAME);
	}
	return ret;

}

int CLogCache::SaveOneItem(GitRev &Rev, LONG offset)
{
	if(!Rev.m_IsDiffFiles)
		return -1;

	SetFilePointer(this->m_DataFile, offset,0, 0);

	SLogCacheRevItemHeader header;

	header.m_Magic=LOG_DATA_ITEM_MAGIC;
	header.m_Version=LOG_INDEX_VERSION;
	header.m_FileCount=Rev.m_Files.GetCount();

	DWORD num;

	if(!WriteFile(this->m_DataFile,&header, sizeof(header),&num,0))
		return -1;

	CString stat;
	CString name,oldname;
	for(int i=0;i<Rev.m_Files.GetCount();i++)
	{
		SLogCacheRevFileHeader header;
		header.m_Magic=LOG_DATA_FILE_MAGIC;
		header.m_Version=LOG_INDEX_VERSION;
		header.m_Action = Rev.m_Files[i].m_Action;
		header.m_Stage = Rev.m_Files[i].m_Stage;
		header.m_ParentNo = Rev.m_Files[i].m_ParentNo;
		name =  Rev.m_Files[i].GetGitPathString();
		header.m_FileNameSize = name.GetLength();
		oldname = Rev.m_Files[i].GetGitOldPathString();
		header.m_OldFileNameSize = oldname.GetLength();

		stat = Rev.m_Files[i].m_StatAdd;
		header.m_Add = (stat == _T("-"))? 0xFFFFFFFF:_tstol(stat);
		stat = Rev.m_Files[i].m_StatDel;
		header.m_Del = (stat == _T("-"))? 0xFFFFFFFF:_tstol(stat);

		if(!WriteFile(this->m_DataFile,&header, sizeof(header)-sizeof(TCHAR),&num,0))
			return -1;

		if(!name.IsEmpty())
		{
			if(!WriteFile(this->m_DataFile,name.GetBuffer(), name.GetLength()*sizeof(TCHAR),&num,0))
				return -1;
		}
		if(!oldname.IsEmpty())
		{
			if(!WriteFile(this->m_DataFile,oldname.GetBuffer(), oldname.GetLength()*sizeof(TCHAR),&num,0))
				return -1;
		}

	}
	return 0;
}

int CLogCache::LoadOneItem(GitRev &Rev,ULONGLONG offset)
{
	if(m_pCacheData == NULL)
		return -1;

	SLogCacheRevItemHeader *header;
	header = (SLogCacheRevItemHeader *)(this->m_pCacheData + offset);

	if( !CheckHeader(header))
		return -2;

	Rev.m_Action = 0;
	SLogCacheRevFileHeader *fileheader;

	offset += sizeof(SLogCacheRevItemHeader);
	fileheader =(SLogCacheRevFileHeader *)(this->m_pCacheData + offset);

	for (DWORD i = 0; i < header->m_FileCount; i++)
	{
		CTGitPath path;
		CString oldfile;
		path.Reset();

		if(!CheckHeader(fileheader))
			return -1;

		CString file(fileheader->m_FileName, fileheader->m_FileNameSize);
		if(fileheader->m_OldFileNameSize)
		{
			oldfile = CString(fileheader->m_FileName + fileheader->m_FileNameSize, fileheader->m_OldFileNameSize);
		}
		path.SetFromGit(file,&oldfile);

		path.m_ParentNo = fileheader ->m_ParentNo;
		path.m_Stage = fileheader ->m_Stage;
		path.m_Action = fileheader ->m_Action;
		Rev.m_Action |= path.m_Action;

		if(fileheader->m_Add == 0xFFFFFFFF)
			path.m_StatAdd=_T("-");
		else
			path.m_StatAdd.Format(_T("%d"),fileheader->m_Add);

		if(fileheader->m_Del == 0xFFFFFFFF)
			path.m_StatDel=_T("-");
		else
			path.m_StatDel.Format(_T("%d"), fileheader->m_Del);

		Rev.m_Files.AddPath(path);

		offset += sizeof(*fileheader) + fileheader->m_OldFileNameSize*sizeof(TCHAR) + fileheader->m_FileNameSize*sizeof(TCHAR) - sizeof(TCHAR);
		fileheader = (SLogCacheRevFileHeader *) (this->m_pCacheData + offset);
	}
	return 0;
}
int CLogCache::RebuildCacheFile()
{
	SLogCacheIndexHeader Indexheader;

	Indexheader.m_Magic = LOG_INDEX_MAGIC;
	Indexheader.m_Version = LOG_INDEX_VERSION;
	Indexheader.m_ItemCount =0;

	SLogCacheDataFileHeader dataheader;

	dataheader.m_Magic = LOG_DATA_MAGIC;
	dataheader.m_Version = LOG_INDEX_VERSION;

	::SetFilePointer(m_IndexFile,0,0,0);
	::SetFilePointer(m_DataFile,0,0,0);
	SetEndOfFile(this->m_IndexFile);
	SetEndOfFile(this->m_DataFile);

	DWORD num;
	WriteFile(this->m_IndexFile,&Indexheader,sizeof(SLogCacheIndexHeader),&num,0);
	SetEndOfFile(this->m_IndexFile);
	WriteFile(this->m_DataFile,&dataheader,sizeof(SLogCacheDataFileHeader),&num,0);
	SetEndOfFile(this->m_DataFile);
	return 0;
}
int CLogCache::SaveCache()
{
	int ret =0;
	BOOL bIsRebuild=false;

	if (this->m_HashMap.empty()) // is not sufficient, because "working copy changes" are always included
		return 0;

	if( this->m_GitDir.IsEmpty())
		return 0;

	if (this->m_pCacheIndex && m_pCacheIndex->m_Header.m_ItemCount == 0) // check for empty log list (issue #915)
		return 0;

	SLogCacheIndexFile *pIndex =  NULL;
	if(this->m_pCacheIndex)
	{
		pIndex = (SLogCacheIndexFile *)malloc(sizeof(SLogCacheIndexFile)
					+sizeof(SLogCacheIndexItem) * (m_pCacheIndex->m_Header.m_ItemCount) );
		if(pIndex ==NULL)
			return -1;

		memcpy(pIndex,this->m_pCacheIndex,
			sizeof(SLogCacheIndexFile) + sizeof(SLogCacheIndexItem) *( m_pCacheIndex->m_Header.m_ItemCount-1)
			);
	}

	this->CloseDataHandles();
	this->CloseIndexHandles();

	SLogCacheIndexHeader header;
	CString file = this->m_GitDir + INDEX_FILE_NAME;
	do
	{
		m_IndexFile = CreateFile(file,
						GENERIC_READ|GENERIC_WRITE,
						0,
						NULL,
						OPEN_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

		if(m_IndexFile == INVALID_HANDLE_VALUE)
		{
			ret = -1;
			break;
		}

		file = m_GitDir + DATA_FILE_NAME;

		m_DataFile = CreateFile(file,
						GENERIC_READ|GENERIC_WRITE,
						0,
						NULL,
						OPEN_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						NULL);

		if(m_DataFile == INVALID_HANDLE_VALUE)
		{
			ret = -1;
			break;
		}


		{

			memset(&header,0,sizeof(SLogCacheIndexHeader));
			DWORD num=0;
			if((!ReadFile(m_IndexFile,&header, sizeof(SLogCacheIndexHeader),&num,0)) ||
				!CheckHeader(&header)
				)
			{
				RebuildCacheFile();
				bIsRebuild =true;
			}
		}
		if(!bIsRebuild)
		{
			SLogCacheDataFileHeader header;
			DWORD num=0;
			if((!ReadFile(m_DataFile,&header,sizeof(SLogCacheDataFileHeader),&num,0)||
				!CheckHeader(&header)))
			{
				RebuildCacheFile();
				bIsRebuild=true;
			}
		}

		if(bIsRebuild)
			header.m_ItemCount=0;

		SetFilePointer(m_DataFile,0,0,2);
		SetFilePointer(m_IndexFile,0,0,2);

		CGitHashMap::iterator i;
		for(i=m_HashMap.begin();i!=m_HashMap.end();i++)
		{
			if(this->GetOffset((*i).second.m_CommitHash,pIndex) ==0 || bIsRebuild)
			{
				if((*i).second.m_IsDiffFiles && !(*i).second.m_CommitHash.IsEmpty())
				{
					LARGE_INTEGER offset;
					offset.LowPart=0;
					offset.HighPart=0;
					LARGE_INTEGER start;
					start.QuadPart = 0;
					SetFilePointerEx(this->m_DataFile,start,&offset,1);
					if (this->SaveOneItem((*i).second, (LONG)offset.QuadPart))
					{
						TRACE(_T("Save one item error"));
						SetFilePointerEx(this->m_DataFile,offset, &offset,0);
						continue;
					}

					SLogCacheIndexItem item;
					item.m_Hash = (*i).second.m_CommitHash;
					item.m_Offset=offset.QuadPart;

					DWORD num;
					WriteFile(m_IndexFile,&item,sizeof(SLogCacheIndexItem),&num,0);
					header.m_ItemCount ++;
				}
			}
		}
		FlushFileBuffers(m_IndexFile);

		m_IndexFileMap = CreateFileMapping(m_IndexFile, NULL, PAGE_READWRITE,0,0,NULL);
		if(m_IndexFileMap == INVALID_HANDLE_VALUE)
		{
			ret =-1;
			break;
		}

		m_pCacheIndex = (SLogCacheIndexFile*)MapViewOfFile(m_IndexFileMap,FILE_MAP_WRITE,0,0,0);
		if(m_pCacheIndex == NULL)
		{
			ret = -1;
			break;
		}

		m_pCacheIndex->m_Header.m_ItemCount = header.m_ItemCount;
		Sort();
		FlushViewOfFile(m_pCacheIndex,0);

	}while(0);

	this->CloseDataHandles();
	this->CloseIndexHandles();

	if(pIndex)
		free(pIndex);
	return ret;
}



void CLogCache::Sort()
{
	if(this->m_pCacheIndex)
	{
		qsort(m_pCacheIndex->m_Item, m_pCacheIndex->m_Header.m_ItemCount,sizeof(SLogCacheIndexItem), Compare);
	}
}

int CLogCache::ClearAllParent()
{
	CGitHashMap::iterator i;
	for(i=m_HashMap.begin();i!=m_HashMap.end();i++)
	{
		(*i).second.m_ParentHash.clear();
		(*i).second.m_Lanes.clear();
	}
	return 0;
}