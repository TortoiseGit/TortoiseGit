#pragma once

#include "Git.h"
#include "TGitPath.h"
#include "GitRev.h"

#define LOG_INDEX_MAGIC		0x88445566
#define LOG_DATA_MAGIC		0x99aa00FF
#define LOG_DATA_ITEM_MAGIC 0x0F8899CC
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
	DWORD m_Offset;
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
	DWORD m_RevSize;
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

	BOOL CheckHeader(SLogCacheRevFileHeader &header)
	{
		if(header.m_Magic == LOG_DATA_MAGIC)
			return TRUE;
		else
			return FALSE;

		if(header.m_Version == LOG_INDEX_VERSION)
			return TRUE;
		else
			return FALSE;
	}

	BOOL CheckHeader(SLogCacheRevItemHeader &header)
	{
		if(header.m_Magic == LOG_DATA_ITEM_MAGIC)
			return TRUE;
		else
			return FALSE;

		if(header.m_Version == LOG_INDEX_VERSION)
			return TRUE;
		else
			return FALSE;

	}

	int SaveOneItem(GitRev &Rev);
	int LoadOneItem(GitRev &Rev,UINT offset);

public:
	CLogCache();
	~CLogCache();
	int FetchCache(CString GitDir);
	std::vector<GitRev> m_NewCacheEntry;
	std::map<CString, DWORD> m_HashMapIndex;
	int GetCacheData(GitRev &Rev);
	int AddCacheEntry(GitRev &Rev);

};