#pragma once

#include "Git.h"
#include "TGitPath.h"
#include "GitRev.h"

#define LOG_INDEX_MAGIC		0x88445566
#define LOG_DATA_MAGIC		0x99aa00FF
#define LOG_DATA_ITEM_MAGIC 0x0F8899CC
#define LOG_DATA_FILE_MAGIC 0x19999FFF
#define LOG_INDEX_VERSION 0x1

struct SLogCacheIndexHeader 
{
	DWORD m_Magic;
	DWORD m_Version;
	DWORD m_ItemCount;
};

struct SLogCacheItem
{
	BYTE  m_Hash[40];
	ULONGLONG m_Offset;
};

struct SLogCacheRevFileHeader
{
	DWORD m_Magic;
	DWORD m_Version;
};

struct SLogCacheRevItemHeader
{
	DWORD m_Magic;
	DWORD m_Version;
	DWORD m_FileCount;
};

#define INDEX_FILE_NAME _T("tortoisegit.index")
#define DATA_FILE_NAME _T("tortoisegit.data")
#define LOCK_FILE_NAME _T("tortoisegit.lock")

class CLogCache
{
protected:
	CFile m_IndexFile;
	CFile m_DataFile;
	CFile m_LockFile;

	BOOL CheckHeader(SLogCacheIndexHeader &header)
	{
		if(header.m_Magic != LOG_INDEX_MAGIC)
			return FALSE;

		if(header.m_Version != LOG_INDEX_VERSION)
			return FALSE;

		return TRUE;
	}

	BOOL CheckHeader(SLogCacheRevFileHeader &header)
	{
		if(header.m_Magic != LOG_DATA_MAGIC)
			return FALSE;

		if(header.m_Version != LOG_INDEX_VERSION)
			return FALSE;

		return TRUE;
	}

	BOOL CheckHeader(SLogCacheRevItemHeader &header)
	{
		if(header.m_Magic != LOG_DATA_ITEM_MAGIC)
			return FALSE;

		if(header.m_Version != LOG_INDEX_VERSION)
			return FALSE;
		
		return TRUE;
	}

	int SaveOneItem(GitRev &Rev,ULONGLONG offset);
	int LoadOneItem(GitRev &Rev,ULONGLONG offset);
	CString m_GitDir;
	int RebuildCacheFile();

public:
	CLogCache();
	~CLogCache();
	int FetchCacheIndex(CString GitDir);
	std::vector<GitRev> m_NewCacheEntry;
	std::map<CString, ULONGLONG> m_HashMapIndex;
	int GetCacheData(GitRev &Rev);
	int AddCacheEntry(GitRev &Rev);
	int SaveCache();


};