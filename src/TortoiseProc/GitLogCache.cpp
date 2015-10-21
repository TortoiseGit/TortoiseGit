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
#include "stdafx.h"
#include "GitAdminDir.h"
#include "GitLogCache.h"
#include "registry.h"

int static Compare(const void *p1, const void*p2)
{
	return memcmp(p1, p2, GIT_HASH_SIZE);
}

CLogCache::CLogCache()
{
	m_IndexFile = INVALID_HANDLE_VALUE;
	m_IndexFileMap = INVALID_HANDLE_VALUE;
	m_pCacheIndex = nullptr;

	m_DataFile = INVALID_HANDLE_VALUE;
	m_DataFileMap = INVALID_HANDLE_VALUE;
	m_pCacheData = nullptr;
	m_DataFileLength = 0;
	m_bEnabled = CRegDWORD(L"Software\\TortoiseGit\\EnableLogCache", TRUE);
}

void CLogCache::CloseDataHandles()
{
	if(m_pCacheData)
	{
		UnmapViewOfFile(m_pCacheData);
		m_pCacheData = nullptr;
	}
	if (m_DataFileMap != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_DataFileMap);
		m_DataFileMap=INVALID_HANDLE_VALUE;
	}
	if (m_DataFile != INVALID_HANDLE_VALUE)
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
		m_pCacheIndex = nullptr;
	}

	if (m_IndexFileMap != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_IndexFileMap);
		m_IndexFileMap = INVALID_HANDLE_VALUE;
	}

	//this->m_IndexFile.Close();
	//this->m_DataFile.Close();
	if (m_IndexFile != INVALID_HANDLE_VALUE)
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

GitRevLoglist* CLogCache::GetCacheData(const CGitHash& hash)
{
	m_HashMap[hash].m_CommitHash=hash;
	return &m_HashMap[hash];
}

ULONGLONG CLogCache::GetOffset(const CGitHash& hash, SLogCacheIndexFile* pData)
{
	if (!pData)
		pData = m_pCacheIndex;

	if (!pData)
		return 0;

	SLogCacheIndexItem* p=reinterpret_cast<SLogCacheIndexItem*>(bsearch(hash.m_hash, pData->m_Item,
			pData->m_Header.m_ItemCount,
			sizeof(SLogCacheIndexItem),
			Compare));

	if(p)
		return p->m_Offset;
	else
		return 0;
}

int CLogCache::FetchCacheIndex(CString GitDir)
{
	if (!m_bEnabled)
		return 0;

	if (!GitAdminDir::GetWorktreeAdminDirPath(GitDir, m_GitDir))
		return -1;

	int ret = -1;
	do
	{
		if( m_IndexFile == INVALID_HANDLE_VALUE)
		{
			CString file = m_GitDir + INDEX_FILE_NAME;
			m_IndexFile = CreateFile(file,
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_DELETE,
						nullptr,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						nullptr);

			if( m_IndexFile == INVALID_HANDLE_VALUE)
				break;
		}

		if( m_IndexFileMap == INVALID_HANDLE_VALUE)
		{
			m_IndexFileMap = CreateFileMapping(m_IndexFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if( m_IndexFileMap == INVALID_HANDLE_VALUE)
				break;
		}

		if (!m_pCacheIndex)
		{
			m_pCacheIndex = reinterpret_cast<SLogCacheIndexFile*>(MapViewOfFile(m_IndexFileMap, FILE_MAP_READ, 0, 0, 0));
			if (!m_pCacheIndex)
				break;
		}

		DWORD indexFileLength = GetFileSize(m_IndexFile, nullptr);
		if (indexFileLength == INVALID_FILE_SIZE || indexFileLength < sizeof(SLogCacheIndexHeader))
			break;

		if( !CheckHeader(&m_pCacheIndex->m_Header))
			break;

		if (indexFileLength < sizeof(SLogCacheIndexHeader) + m_pCacheIndex->m_Header.m_ItemCount * sizeof(SLogCacheIndexItem))
			break;

		if(	m_DataFile == INVALID_HANDLE_VALUE )
		{
			CString file = m_GitDir + DATA_FILE_NAME;
			m_DataFile = CreateFile(file,
						GENERIC_READ,
						FILE_SHARE_READ | FILE_SHARE_DELETE,
						nullptr,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						nullptr);

			if(m_DataFile == INVALID_HANDLE_VALUE)
				break;
		}

		if( m_DataFileMap == INVALID_HANDLE_VALUE)
		{
			m_DataFileMap = CreateFileMapping(m_DataFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if( m_DataFileMap == INVALID_HANDLE_VALUE)
				break;
		}
		m_DataFileLength = GetFileSize(m_DataFile, nullptr);
		if (m_DataFileLength == INVALID_FILE_SIZE || m_DataFileLength < sizeof(SLogCacheDataFileHeader))
			break;

		if (!m_pCacheData)
		{
			m_pCacheData = (BYTE*)MapViewOfFile(m_DataFileMap,FILE_MAP_READ,0,0,0);
			if (!m_pCacheData)
				break;
		}

		if (!CheckHeader(reinterpret_cast<SLogCacheDataFileHeader*>(m_pCacheData)))
			break;

		if (m_DataFileLength < sizeof(SLogCacheDataFileHeader) + m_pCacheIndex->m_Header.m_ItemCount * sizeof(SLogCacheDataFileHeader))
			break;

		ret = 0;
	}while(0);

	if(ret)
	{
		CloseIndexHandles();
		CloseDataHandles();
		::DeleteFile(m_GitDir + INDEX_FILE_NAME);
		::DeleteFile(m_GitDir + DATA_FILE_NAME);
	}
	return ret;
}

int CLogCache::SaveOneItem(const GitRevLoglist& Rev, LONG offset)
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
	for (int i = 0; i < Rev.m_Files.GetCount(); ++i)
	{
		SLogCacheRevFileHeader revfileheader;
		revfileheader.m_Magic = LOG_DATA_FILE_MAGIC;
		revfileheader.m_Version = LOG_INDEX_VERSION;
		revfileheader.m_IsSubmodule = Rev.m_Files[i].IsDirectory() ? 1 : 0;
		revfileheader.m_Action = Rev.m_Files[i].m_Action;
		revfileheader.m_Stage = Rev.m_Files[i].m_Stage;
		revfileheader.m_ParentNo = Rev.m_Files[i].m_ParentNo;
		name =  Rev.m_Files[i].GetGitPathString();
		revfileheader.m_FileNameSize = name.GetLength();
		oldname = Rev.m_Files[i].GetGitOldPathString();
		revfileheader.m_OldFileNameSize = oldname.GetLength();

		stat = Rev.m_Files[i].m_StatAdd;
		revfileheader.m_Add = (stat == L"-") ? 0xFFFFFFFF : _wtol(stat);
		stat = Rev.m_Files[i].m_StatDel;
		revfileheader.m_Del = (stat == L"-") ? 0xFFFFFFFF : _wtol(stat);

		if(!WriteFile(this->m_DataFile, &revfileheader, sizeof(revfileheader) - sizeof(TCHAR), &num, 0))
			return -1;

		if(!name.IsEmpty())
		{
			if (!WriteFile(this->m_DataFile, name, name.GetLength() * sizeof(TCHAR), &num, 0))
				return -1;
		}
		if(!oldname.IsEmpty())
		{
			if (!WriteFile(this->m_DataFile, oldname, oldname.GetLength() * sizeof(TCHAR), &num, 0))
				return -1;
		}

	}
	return 0;
}

int CLogCache::LoadOneItem(GitRevLoglist& Rev,ULONGLONG offset)
{
	if (!m_pCacheData)
		return -1;

	if (offset + sizeof(SLogCacheRevItemHeader) > m_DataFileLength)
		return -2;

	SLogCacheRevItemHeader* header = reinterpret_cast<SLogCacheRevItemHeader*>(m_pCacheData + offset);

	if( !CheckHeader(header))
		return -2;

	Rev.m_Action = 0;
	SLogCacheRevFileHeader *fileheader;

	offset += sizeof(SLogCacheRevItemHeader);
	fileheader = reinterpret_cast<SLogCacheRevFileHeader*>(m_pCacheData + offset);

	for (DWORD i = 0; i < header->m_FileCount; ++i)
	{
		CTGitPath path;
		CString oldfile;

		if (offset + sizeof(SLogCacheRevFileHeader) > m_DataFileLength)
		{
			Rev.m_Action = 0;
			Rev.m_Files.Clear();
			return -2;
		}

		if(!CheckHeader(fileheader))
		{
			Rev.m_Action = 0;
			Rev.m_Files.Clear();
			return -2;
		}

		CString file(fileheader->m_FileName, fileheader->m_FileNameSize);
		if(fileheader->m_OldFileNameSize)
			oldfile = CString(fileheader->m_FileName + fileheader->m_FileNameSize, fileheader->m_OldFileNameSize);
		path.SetFromGit(file, &oldfile, (int*)&fileheader->m_IsSubmodule);

		path.m_ParentNo = fileheader ->m_ParentNo;
		path.m_Stage = fileheader ->m_Stage;
		path.m_Action = fileheader->m_Action & ~(CTGitPath::LOGACTIONS_HIDE | CTGitPath::LOGACTIONS_GRAY);
		Rev.m_Action |= path.m_Action;

		if(fileheader->m_Add == 0xFFFFFFFF)
			path.m_StatAdd = L"-";
		else
			path.m_StatAdd.Format(L"%d", fileheader->m_Add);

		if(fileheader->m_Del == 0xFFFFFFFF)
			path.m_StatDel = L"-";
		else
			path.m_StatDel.Format(L"%d", fileheader->m_Del);

		Rev.m_Files.AddPath(path);

		offset += sizeof(*fileheader) + fileheader->m_OldFileNameSize*sizeof(TCHAR) + fileheader->m_FileNameSize*sizeof(TCHAR) - sizeof(TCHAR);
		fileheader = reinterpret_cast<SLogCacheRevFileHeader*>(m_pCacheData + offset);
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
	if (!m_bEnabled)
		return 0;

	BOOL bIsRebuild=false;

	if (this->m_HashMap.empty()) // is not sufficient, because "working copy changes" are always included
		return 0;

	if( this->m_GitDir.IsEmpty())
		return 0;

	if (this->m_pCacheIndex && m_pCacheIndex->m_Header.m_ItemCount == 0) // check for empty log list (issue #915)
		return 0;

	SLogCacheIndexFile* pIndex = nullptr;
	if(this->m_pCacheIndex)
	{
		pIndex = reinterpret_cast<SLogCacheIndexFile*>(malloc(sizeof(SLogCacheIndexFile) + sizeof(SLogCacheIndexItem) * (m_pCacheIndex->m_Header.m_ItemCount)));
		if (!pIndex)
			return -1;

		memcpy(pIndex,this->m_pCacheIndex,
			sizeof(SLogCacheIndexFile) + sizeof(SLogCacheIndexItem) *( m_pCacheIndex->m_Header.m_ItemCount-1)
			);
	}

	this->CloseDataHandles();
	this->CloseIndexHandles();

	SLogCacheIndexHeader header;
	CString file = this->m_GitDir + INDEX_FILE_NAME;
	int ret = -1;
	do
	{
		m_IndexFile = CreateFile(file,
						GENERIC_READ|GENERIC_WRITE,
						0,
						nullptr,
						OPEN_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						nullptr);

		if(m_IndexFile == INVALID_HANDLE_VALUE)
			break;

		file = m_GitDir + DATA_FILE_NAME;

		m_DataFile = CreateFile(file,
						GENERIC_READ|GENERIC_WRITE,
						0,
						nullptr,
						OPEN_ALWAYS,
						FILE_ATTRIBUTE_NORMAL,
						nullptr);

		if(m_DataFile == INVALID_HANDLE_VALUE)
			break;

		{
			memset(&header,0,sizeof(SLogCacheIndexHeader));
			DWORD num=0;
			if ((!ReadFile(m_IndexFile, &header, sizeof(SLogCacheIndexHeader), &num, 0)) || num != sizeof(SLogCacheIndexHeader) ||
				!CheckHeader(&header)
				)
			{
				RebuildCacheFile();
				bIsRebuild =true;
			}
		}
		if(!bIsRebuild)
		{
			SLogCacheDataFileHeader datafileheader;
			DWORD num=0;
			if ((!ReadFile(m_DataFile, &datafileheader, sizeof(SLogCacheDataFileHeader), &num, 0) || num != sizeof(SLogCacheDataFileHeader) ||
				!CheckHeader(&datafileheader)))
			{
				RebuildCacheFile();
				bIsRebuild=true;
			}
		}

		if(bIsRebuild)
			header.m_ItemCount=0;

		SetFilePointer(m_DataFile,0,0,2);
		SetFilePointer(m_IndexFile,0,0,2);

		for (auto i = m_HashMap.cbegin(); i != m_HashMap.cend(); ++i)
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
						TRACE(L"Save one item error");
						SetFilePointerEx(this->m_DataFile,offset, &offset,0);
						continue;
					}

					SLogCacheIndexItem item;
					item.m_Hash = (*i).second.m_CommitHash;
					item.m_Offset=offset.QuadPart;

					DWORD num;
					WriteFile(m_IndexFile,&item,sizeof(SLogCacheIndexItem),&num,0);
					++header.m_ItemCount;
				}
			}
		}
		FlushFileBuffers(m_IndexFile);

		m_IndexFileMap = CreateFileMapping(m_IndexFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
		if(m_IndexFileMap == INVALID_HANDLE_VALUE)
			break;

		m_pCacheIndex = reinterpret_cast<SLogCacheIndexFile*>(MapViewOfFile(m_IndexFileMap, FILE_MAP_WRITE, 0, 0, 0));
		if (!m_pCacheIndex)
			break;

		m_pCacheIndex->m_Header.m_ItemCount = header.m_ItemCount;
		Sort();
		FlushViewOfFile(m_pCacheIndex,0);
		ret = 0;
	}while(0);

	this->CloseDataHandles();
	this->CloseIndexHandles();

	free(pIndex);
	return ret;
}

void CLogCache::Sort()
{
	if (this->m_pCacheIndex)
		qsort(m_pCacheIndex->m_Item, m_pCacheIndex->m_Header.m_ItemCount, sizeof(SLogCacheIndexItem), Compare);
}

int CLogCache::ClearAllParent()
{
	for (auto i = m_HashMap.begin(); i != m_HashMap.end(); ++i)
	{
		(*i).second.m_ParentHash.clear();
		(*i).second.m_Lanes.clear();
	}
	return 0;
}

void CLogCache::ClearAllLanes()
{
	for (auto i = m_HashMap.begin(); i != m_HashMap.end(); ++i)
		(*i).second.m_Lanes.clear();
}
