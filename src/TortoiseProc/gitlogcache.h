// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2017 - TortoiseGit

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
#pragma once

#include "GitRevLoglist.h"
#include "GitHash.h"

#define LOG_INDEX_MAGIC		0x88AA5566
#define LOG_DATA_MAGIC		0x99BB0FFF
#define LOG_DATA_ITEM_MAGIC 0x0FCC9ACC
#define LOG_DATA_FILE_MAGIC 0x19EE9DFF
#define LOG_INDEX_VERSION	0x11

#pragma pack (1)
struct SLogCacheIndexHeader
{
	DWORD m_Magic;
	DWORD m_Version;
	DWORD m_ItemCount;
};

struct SLogCacheIndexItem
{
	CGitHash  m_Hash;
	ULONGLONG m_Offset;
};

struct SLogCacheIndexFile
{
	struct SLogCacheIndexHeader m_Header;
	struct SLogCacheIndexItem	m_Item[1]; //dynmatic size
};

struct SLogCacheRevFileHeader
{
	DWORD m_Magic;
	DWORD m_Action;
	DWORD m_Stage;
	DWORD m_ParentNo;
	DWORD m_Add;
	DWORD m_Del;
	DWORD m_IsSubmodule;
	DWORD m_FileNameSize;
	DWORD m_OldFileNameSize;
	TCHAR m_FileName[1];
};

struct SLogCacheRevItemHeader
{
	DWORD m_Magic;
	DWORD m_FileCount;
};

struct SLogCacheDataFileHeader
{
	DWORD m_Magic;
	DWORD m_Version;
};
# pragma pack ()

class CGitHashMap : public std::unordered_map<CGitHash, GitRevLoglist>
{
public:
	bool IsExist(CGitHash &hash)
	{
		return find(hash) != end();
	}
};

#define INDEX_FILE_NAME L"tortoisegit.index"
#define DATA_FILE_NAME L"tortoisegit.data"

class CLogCache
{
public:

protected:
	BOOL m_bEnabled;

	HANDLE m_IndexFile;
	HANDLE m_IndexFileMap;
	SLogCacheIndexFile *m_pCacheIndex;



	HANDLE m_DataFile;
	HANDLE m_DataFileMap;
	BYTE  *m_pCacheData;
	DWORD m_DataFileLength;

	void CloseDataHandles();
	void CloseIndexHandles();

	BOOL CheckHeader(SLogCacheIndexHeader *header)
	{
		if (header->m_Magic != LOG_INDEX_MAGIC)
			return FALSE;

		if (header->m_Version != LOG_INDEX_VERSION)
			return FALSE;

		return TRUE;
	}

	BOOL CheckHeader(SLogCacheRevFileHeader *header)
	{
		if (header->m_Magic != LOG_DATA_FILE_MAGIC)
			return FALSE;

		return TRUE;
	}

	BOOL CheckHeader(SLogCacheRevItemHeader *header)
	{
		if (header->m_Magic != LOG_DATA_ITEM_MAGIC)
			return FALSE;

		return TRUE;
	}

	BOOL CheckHeader(SLogCacheDataFileHeader *header)
	{
		if (header->m_Magic != LOG_DATA_MAGIC)
			return FALSE;

		if (header->m_Version != LOG_INDEX_VERSION)
			return FALSE;

		return TRUE;
	}

	int SaveOneItem(const GitRevLoglist& Rev, LONG offset);

	CString m_GitDir;
	int RebuildCacheFile();

public:
	CLogCache();
	~CLogCache();
	int FetchCacheIndex(CString GitDir);
	int LoadOneItem(GitRevLoglist& Rev, ULONGLONG offset);
	ULONGLONG GetOffset(const CGitHash& hash, SLogCacheIndexFile* pData = nullptr);

	CGitHashMap m_HashMap;

	GitRevLoglist* GetCacheData(const CGitHash& hash);
	int SaveCache();

	int ClearAllParent();
	void ClearAllLanes();
};
