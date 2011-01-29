#pragma once

#include "Git.h"
#include "TGitPath.h"
#include "GitRev.h"
#include "GitHash.h"

#define LOG_INDEX_MAGIC		0x88445566
#define LOG_DATA_MAGIC		0x99aa0FFF
#define LOG_DATA_ITEM_MAGIC 0x0F889ACC
#define LOG_DATA_FILE_MAGIC 0x19999DFF
#define LOG_INDEX_VERSION   0xD

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
	DWORD m_Version;
	DWORD m_Action;
	DWORD m_Stage;
	DWORD m_ParentNo;
	DWORD m_Add;
	DWORD m_Del;
	DWORD m_FileNameSize;
	DWORD m_OldFileNameSize;
	TCHAR m_FileName[1];
};
# pragma pack ()

struct SLogCacheRevItemHeader
{
	DWORD m_Magic;
	DWORD m_Version;
	DWORD m_FileCount;
};

struct SLogCacheDataFileHeader
{
	DWORD m_Magic;
	DWORD m_Version;
};

class CGitHashMap:public std::map<CGitHash,GitRev>
{
public:
	bool IsExist(CGitHash &hash)
	{
		return find(hash) != end();
	}
};

#define INDEX_FILE_NAME _T("tortoisegit.index")
#define DATA_FILE_NAME _T("tortoisegit.data")
#define LOCK_FILE_NAME _T("tortoisegit.lock")


class CLogCache
{
public:
	
protected:
	HANDLE m_IndexFile;
	HANDLE m_IndexFileMap;
	SLogCacheIndexFile *m_pCacheIndex;



	HANDLE m_DataFile;
	HANDLE m_DataFileMap;
	BYTE  *m_pCacheData;

	void CloseDataHandles();
	void CloseIndexHandles();
	void Sort();

	BOOL CheckHeader(SLogCacheIndexHeader *header)
	{
		if(header->m_Magic != LOG_INDEX_MAGIC)
			return FALSE;

		if(header->m_Version != LOG_INDEX_VERSION)
			return FALSE;

		return TRUE;
	}

	BOOL CheckHeader(SLogCacheRevFileHeader *header)
	{
		if(header->m_Magic != LOG_DATA_FILE_MAGIC)
			return FALSE;

		if(header->m_Version != LOG_INDEX_VERSION)
			return FALSE;

		return TRUE;
	}

	BOOL CheckHeader(SLogCacheRevItemHeader *header)
	{
		if(header->m_Magic != LOG_DATA_ITEM_MAGIC)
			return FALSE;

		if(header->m_Version != LOG_INDEX_VERSION)
			return FALSE;
		
		return TRUE;
	}

	BOOL CheckHeader(SLogCacheDataFileHeader *header)
	{
		if(header->m_Magic != LOG_DATA_MAGIC)
			return FALSE;

		if(header->m_Version != LOG_INDEX_VERSION)
			return FALSE;
		
		return TRUE;
	}

	int SaveOneItem(GitRev &Rev,ULONGLONG offset);
	
	CString m_GitDir;
	int RebuildCacheFile();

public:
	CLogCache();
	~CLogCache();
	int FetchCacheIndex(CString GitDir);
	int LoadOneItem(GitRev &Rev,ULONGLONG offset);
	ULONGLONG GetOffset(CGitHash &hash, SLogCacheIndexFile *pData =NULL);

	CGitHashMap m_HashMap;
	
	GitRev * GetCacheData(CGitHash &Rev);
	int AddCacheEntry(GitRev &Rev);
	int SaveCache();

	int ClearAllParent();

};