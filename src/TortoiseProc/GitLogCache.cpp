// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2020, 2023-2025 - TortoiseGit

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
#include <intsafe.h>
#include "Git.h"

int static Compare(const void *p1, const void*p2)
{
	return memcmp(p1, p2, GIT_HASH_MAX_SIZE);
}

CLogCache::CLogCache()
{
	m_bEnabled = CRegDWORD(L"Software\\TortoiseGit\\EnableLogCache", TRUE);
}

void CLogCache::CloseDataHandles()
{
	if(m_pCacheData)
	{
		UnmapViewOfFile(m_pCacheData);
		m_pCacheData = nullptr;
	}
	if (m_DataFileMap)
	{
		CloseHandle(m_DataFileMap);
		m_DataFileMap = nullptr;
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

	if (m_IndexFileMap)
	{
		CloseHandle(m_IndexFileMap);
		m_IndexFileMap = nullptr;
	}

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

	SLogCacheIndexItem* p = reinterpret_cast<SLogCacheIndexItem*>(bsearch(hash.ToRaw(), pData->m_Item,
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

	if (PathFileExists(m_GitDir + L"\\shallow"))
	{
		try
		{
			CString strLine;
			CStdioFile file(m_GitDir + L"\\shallow", CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
			const auto hashType = g_Git.GetCurrentRepoHashType();
			while (file.ReadString(strLine))
			{
				CGitHash hash = CGitHash::FromHexStr(strLine, hashType);
				if (hash.IsEmpty())
					continue;

				m_shallowAnchors.insert(hash);
			}
		}
		catch (CFileException* pE)
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading shallow file commits list\n");
			pE->Delete();
		}
	}

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

		if (!m_IndexFileMap)
		{
			m_IndexFileMap = CreateFileMapping(m_IndexFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if (!m_IndexFileMap)
				break;
		}

		if (!m_pCacheIndex)
		{
			m_pCacheIndex = reinterpret_cast<SLogCacheIndexFile*>(MapViewOfFile(m_IndexFileMap, FILE_MAP_READ, 0, 0, 0));
			if (!m_pCacheIndex)
				break;
		}

		LARGE_INTEGER indexFileSize;
		if (!GetFileSizeEx(m_IndexFile, &indexFileSize))
			break;
		if (indexFileSize.QuadPart >= SIZE_T_MAX)
		{
			CloseIndexHandles();
			CloseDataHandles();
			return -1;
		}
		if (indexFileSize.QuadPart < sizeof(SLogCacheIndexHeader))
			break;

		if( !CheckHeader(&m_pCacheIndex->m_Header))
			break;

		if (m_pCacheIndex->m_Header.m_DiffPercentage != static_cast<unsigned int>(CGit::ms_iSimilarityIndexThreshold))
			break;

		if (size_t len; SizeTMult(sizeof(SLogCacheIndexItem), m_pCacheIndex->m_Header.m_ItemCount, &len) != S_OK || SizeTAdd(len, sizeof(SLogCacheIndexHeader), &len) != S_OK || static_cast<size_t>(indexFileSize.QuadPart) != len)
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

		if (!m_DataFileMap)
		{
			m_DataFileMap = CreateFileMapping(m_DataFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
			if (!m_DataFileMap)
				break;
		}
		if (LARGE_INTEGER fileLength; !GetFileSizeEx(m_DataFile, &fileLength) || fileLength.QuadPart < sizeof(SLogCacheDataFileHeader))
			break;
		else if (fileLength.QuadPart >= SIZE_T_MAX)
		{
			CloseIndexHandles();
			CloseDataHandles();
			return -1;
		}
		else
			m_DataFileLength = static_cast<size_t>(fileLength.QuadPart);

		if (!m_pCacheData)
		{
			m_pCacheData = static_cast<BYTE*>(MapViewOfFile(m_DataFileMap, FILE_MAP_READ, 0, 0, 0));
			if (!m_pCacheData)
				break;
		}

		if (!CheckHeader(reinterpret_cast<SLogCacheDataFileHeader*>(m_pCacheData)))
			break;

		if (size_t length; SizeTMult(m_pCacheIndex->m_Header.m_ItemCount, sizeof(SLogCacheRevItemHeader), &length) != S_OK || SizeTAdd(sizeof(SLogCacheDataFileHeader), length, &length) != S_OK || m_DataFileLength < length)
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

int CLogCache::SaveOneItem(const GitRevLoglist& Rev, LARGE_INTEGER offset)
{
	if (!Rev.m_IsDiffFiles || Rev.m_IsDiffFiles == 2)
		return -1;

	if (!SetFilePointerEx(m_DataFile, offset, nullptr, FILE_BEGIN))
		return -1;

	SLogCacheRevItemHeader header;

	header.m_Magic=LOG_DATA_ITEM_MAGIC;
	header.m_FileCount=Rev.m_Files.GetCount();

	DWORD dwWritten = 0;
	if (!WriteFile(this->m_DataFile, &header, sizeof(header), &dwWritten, 0))
		return -1;

	CString stat;
	CString name,oldname;
	for (int i = 0; i < Rev.m_Files.GetCount(); ++i)
	{
		SLogCacheRevFileHeader revfileheader;
		revfileheader.m_Magic = LOG_DATA_FILE_MAGIC;
		revfileheader.m_IsSubmodule = Rev.m_Files[i].IsDirectory() ? 1 : 0;
		revfileheader.m_Action = Rev.m_Files[i].m_Action;
		revfileheader.m_ParentNo = Rev.m_Files[i].m_ParentNo;
		name =  Rev.m_Files[i].GetGitPathString();
		revfileheader.m_FileNameSize = name.GetLength();
		oldname = Rev.m_Files[i].GetGitOldPathString();
		revfileheader.m_OldFileNameSize = oldname.GetLength();

		stat = Rev.m_Files[i].m_StatAdd;
		revfileheader.m_Add = (stat == L"-") ? 0xFFFFFFFF : _wtol(stat);
		stat = Rev.m_Files[i].m_StatDel;
		revfileheader.m_Del = (stat == L"-") ? 0xFFFFFFFF : _wtol(stat);

		if (!WriteFile(this->m_DataFile, &revfileheader, sizeof(revfileheader) - sizeof(wchar_t), &dwWritten, 0))
			return -1;

		if(!name.IsEmpty())
		{
			if (!WriteFile(this->m_DataFile, name, name.GetLength() * sizeof(wchar_t), &dwWritten, 0))
				return -1;
		}
		if(!oldname.IsEmpty())
		{
			if (!WriteFile(this->m_DataFile, oldname, oldname.GetLength() * sizeof(wchar_t), &dwWritten, 0))
				return -1;
		}

	}
	return 0;
}

int CLogCache::LoadOneItem(GitRevLoglist& Rev, ULONGLONG ullOffset)
{
	if (!m_pCacheData)
		return -1;

	if (ullOffset >= m_DataFileLength || ullOffset < sizeof(SLogCacheDataFileHeader))
		return -2;

	auto offset = static_cast<size_t>(ullOffset);
	SLogCacheRevItemHeader* header = reinterpret_cast<SLogCacheRevItemHeader*>(m_pCacheData + offset);
	if (SizeTAdd(offset, sizeof(SLogCacheRevItemHeader), &offset) != S_OK || offset > m_DataFileLength)
		return -2;

	if( !CheckHeader(header))
		return -2;

	Rev.m_Action = 0;

	for (DWORD i = 0; i < header->m_FileCount; ++i)
	{
		auto fileheader = reinterpret_cast<SLogCacheRevFileHeader*>(m_pCacheData + offset);
		if (SizeTAdd(offset, sizeof(SLogCacheRevFileHeader) - sizeof(wchar_t), &offset) != S_OK || offset >= m_DataFileLength)
		{
			Rev.m_Action = 0;
			Rev.m_Files.Clear();
			return -2;
		}

		if (!CheckHeader(fileheader) || !fileheader->m_FileNameSize || fileheader->m_FileNameSize >= INT_MAX || fileheader->m_OldFileNameSize >= INT_MAX)
		{
			Rev.m_Action = 0;
			Rev.m_Files.Clear();
			return -2;
		}

		if (size_t recordLength; SizeTAdd(fileheader->m_FileNameSize, fileheader->m_OldFileNameSize, &recordLength) != S_OK
			|| SizeTMult(recordLength, sizeof(wchar_t), &recordLength) != S_OK
			|| SizeTAdd(offset, recordLength, &offset) != S_OK
			|| offset > m_DataFileLength)
		{
			Rev.m_Action = 0;
			Rev.m_Files.Clear();
			return -2;
		}

		CString oldfile;
		CString file(fileheader->m_FileName, fileheader->m_FileNameSize);
		if(fileheader->m_OldFileNameSize)
			oldfile = CString(fileheader->m_FileName + fileheader->m_FileNameSize, fileheader->m_OldFileNameSize);
		CTGitPath path;
		int isSubmodule = fileheader->m_IsSubmodule;
		path.SetFromGit(file, oldfile.IsEmpty() ? nullptr : &oldfile, static_cast<int*>(&isSubmodule));

		path.m_ParentNo = fileheader ->m_ParentNo;
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
	}
	return 0;
}
int CLogCache::RebuildCacheFile()
{
	SLogCacheIndexHeader Indexheader;

	Indexheader.m_Magic = LOG_INDEX_MAGIC;
	Indexheader.m_Version = LOG_INDEX_VERSION;
	Indexheader.m_ItemCount =0;
	Indexheader.m_DiffPercentage = CGit::ms_iSimilarityIndexThreshold;

	SLogCacheDataFileHeader dataheader;

	dataheader.m_Magic = LOG_DATA_MAGIC;
	dataheader.m_Version = LOG_INDEX_VERSION;

	LARGE_INTEGER start{};
	SetFilePointerEx(m_DataFile, start, nullptr, FILE_BEGIN);
	SetFilePointerEx(m_IndexFile, start, nullptr, FILE_BEGIN);

	DWORD dwWritten = 0;
	WriteFile(m_IndexFile, &Indexheader, sizeof(SLogCacheIndexHeader), &dwWritten, 0);
	SetEndOfFile(this->m_IndexFile);
	WriteFile(m_DataFile, &dataheader, sizeof(SLogCacheDataFileHeader), &dwWritten, 0);
	SetEndOfFile(this->m_DataFile);
	return 0;
}
int CLogCache::SaveCache()
{
	if (!m_bEnabled)
		return 0;

	BOOL bIsRebuild=false;

	if (m_HashMap.empty() || (m_HashMap.size() == 1 && m_HashMap.find(CGitHash()) != m_HashMap.cend()))
		return 0;

	if( this->m_GitDir.IsEmpty())
		return 0;

	SLogCacheIndexFile* pIndex = nullptr;
	if (m_pCacheIndex && m_pCacheIndex->m_Header.m_ItemCount > 0)
	{
		size_t len;
		if (SizeTMult(sizeof(SLogCacheIndexItem), m_pCacheIndex->m_Header.m_ItemCount - 1, &len) != S_OK || SizeTAdd(len, sizeof(SLogCacheIndexFile), &len) != S_OK)
			return -1;
		pIndex = reinterpret_cast<SLogCacheIndexFile*>(malloc(len));
		if (!pIndex)
			return -1;

		memcpy(pIndex, m_pCacheIndex, len);
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

		{
			LARGE_INTEGER start{};
			SetFilePointerEx(m_DataFile, start, nullptr, FILE_END);
			SetFilePointerEx(m_IndexFile, start, nullptr, FILE_END);
		}

		bool isShallow = !m_shallowAnchors.empty();
		for (auto i = m_HashMap.cbegin(); i != m_HashMap.cend(); ++i)
		{
			if (!bIsRebuild && GetOffset((*i).second.m_CommitHash, pIndex) != 0)
				continue;

			if (!(*i).second.m_IsDiffFiles || (*i).second.m_IsDiffFiles == 2 || (*i).second.m_CommitHash.IsEmpty() || (isShallow && m_shallowAnchors.contains((*i).second.m_CommitHash)))
				continue;

			LARGE_INTEGER offset{};
			LARGE_INTEGER start{};
			SetFilePointerEx(m_DataFile, start, &offset, FILE_CURRENT);
			if (SaveOneItem((*i).second, offset))
			{
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Save one item error\n");
				if (!SetFilePointerEx(m_DataFile, offset, &offset, FILE_BEGIN))
					break;
				continue;
			}

			SLogCacheIndexItem item;
			item.m_oid = (*i).second.m_CommitHash;
			item.m_Offset = offset.QuadPart;

			DWORD dwWritten = 0;
			if (!WriteFile(m_IndexFile, &item, sizeof(SLogCacheIndexItem), &dwWritten, 0))
				break;
			++header.m_ItemCount;
		}
		FlushFileBuffers(m_IndexFile);

		m_IndexFileMap = CreateFileMapping(m_IndexFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
		if (!m_IndexFileMap)
			break;

		m_pCacheIndex = reinterpret_cast<SLogCacheIndexFile*>(MapViewOfFile(m_IndexFileMap, FILE_MAP_WRITE, 0, 0, 0));
		if (!m_pCacheIndex)
			break;

		m_pCacheIndex->m_Header.m_DiffPercentage = CGit::ms_iSimilarityIndexThreshold;
		m_pCacheIndex->m_Header.m_ItemCount = header.m_ItemCount;

		std::qsort(m_pCacheIndex->m_Item, m_pCacheIndex->m_Header.m_ItemCount, sizeof(SLogCacheIndexItem), Compare);

		FlushViewOfFile(m_pCacheIndex,0);
		ret = 0;
	}while(0);

	this->CloseDataHandles();
	this->CloseIndexHandles();
	if (ret)
	{
		::DeleteFile(m_GitDir + INDEX_FILE_NAME);
		::DeleteFile(m_GitDir + DATA_FILE_NAME);
	}

	free(pIndex);
	return ret;
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
