#include "stdafx.h"
#include "GitLogCache.h"

CLogCache::CLogCache()
{

}

CLogCache::~CLogCache()
{
	//this->m_IndexFile.Close();
	//this->m_DataFile.Close();
}
int CLogCache::AddCacheEntry(GitRev &Rev)
{
	this->m_NewCacheEntry.push_back(Rev);
	return 0;
}

int CLogCache::GetCacheData(GitRev &Rev)
{
	if(this->m_HashMapIndex.find(Rev.m_CommitHash)==m_HashMapIndex.end())
	{
		for(int i=0;i<this->m_NewCacheEntry.size();i++)
		{
			if(m_NewCacheEntry[i].m_CommitHash==Rev.m_CommitHash)
			{
				Rev.CopyFrom(m_NewCacheEntry[i],true);
				return 0;
			}
		}
		return -1;
	}
	else
	{
		return LoadOneItem(Rev,m_HashMapIndex[Rev.m_CommitHash]);
	}
	return 0;
}
int CLogCache::FetchCacheIndex(CString GitDir)
{

	if(this->m_IndexFile.m_hFile == CFile::hFileNull)
	{
		BOOL b=m_IndexFile.Open(GitDir+_T("\\.git\\")+INDEX_FILE_NAME,
			CFile::modeRead|CFile::shareDenyNone|
			CFile::modeNoTruncate|CFile::modeCreate);
		if(!b)
			return -1;
	}
	//Cache has been fetched.
	//if(m_GitDir == GitDir)
	//	return 0;

	m_GitDir=GitDir;
	m_IndexFile.SeekToBegin();

	SLogCacheIndexHeader header;
	UINT count=m_IndexFile.Read(&header,sizeof(SLogCacheIndexHeader));
	if(count != sizeof(SLogCacheIndexHeader))
		return -2;

	if( header.m_Magic != LOG_INDEX_MAGIC )
		return -3;

	if( header.m_Version != LOG_INDEX_VERSION )
		return -4;


	SLogCacheItem Item;

	//for(int i=0;i<header.m_ItemCount;i++)
	this->m_HashMapIndex.clear();

	while(1)
	{
		count=m_IndexFile.Read(&Item,sizeof(SLogCacheItem));	
		if( count != sizeof(SLogCacheItem) )
			break;

		CString str;
		g_Git.StringAppend(&str,Item.m_Hash,CP_UTF8,40);
		this->m_HashMapIndex[str]=Item.m_Offset;
	}

	return 0;
}

int CLogCache::SaveOneItem(GitRev &Rev,ULONGLONG offset)
{
	
	SLogCacheRevItemHeader header;
	m_DataFile.Seek(offset,CFile::begin);

	if(this->m_DataFile.m_hFile == CFile::hFileNull)
	{
		BOOL b=m_DataFile.Open(m_GitDir+_T("\\.git\\")+INDEX_FILE_NAME,
			CFile::modeRead|CFile::shareDenyNone|
			CFile::modeNoTruncate|CFile::modeCreate);
		if(!b)
			return -1;
	}

	header.m_Magic=LOG_DATA_ITEM_MAGIC;
	header.m_Version=LOG_INDEX_VERSION;
	header.m_FileCount=Rev.m_Files.GetCount();

	m_DataFile.Write(&header,sizeof(SLogCacheRevItemHeader));
	
	CArchive ar(&m_DataFile, CArchive::store);

	ar<<Rev.m_AuthorName;
	ar<<Rev.m_AuthorEmail;
	ar<<Rev.m_AuthorDate;
	ar<<Rev.m_CommitterName;
	ar<<Rev.m_CommitterEmail;
	ar<<Rev.m_CommitterDate;
	ar<<Rev.m_Subject;
	ar<<Rev.m_Body;
	ar<<Rev.m_CommitHash;
	ar<<Rev.m_Action;

	for(int i=0;i<Rev.m_Files.GetCount();i++)
	{
		SLogCacheRevFileHeader header;
		header.m_Magic=LOG_DATA_FILE_MAGIC;
		header.m_Version=LOG_INDEX_VERSION;

		ar<<header.m_Magic;
		ar<<header.m_Version;
		ar<<Rev.m_Files[i].GetGitPathString();
		ar<<Rev.m_Files[i].GetGitOldPathString();
		ar<<Rev.m_Files[i].m_Action;
		ar<<Rev.m_Files[i].m_Stage;
		ar<<Rev.m_Files[i].m_StatAdd;
		ar<<Rev.m_Files[i].m_StatDel;
		
	}
	return 0;
}

int CLogCache::LoadOneItem(GitRev &Rev,ULONGLONG offset)
{
	SLogCacheRevItemHeader header;

	if(this->m_DataFile.m_hFile == CFile::hFileNull)
	{
		BOOL b=m_DataFile.Open(m_GitDir+_T("\\.git\\")+DATA_FILE_NAME,
			CFile::modeRead|CFile::shareDenyNone|
			CFile::modeNoTruncate|CFile::modeCreate);
		if(!b)
			return -1;
	}
	m_DataFile.Seek(offset,CFile::begin);

	UINT count=m_DataFile.Read(&header,sizeof(SLogCacheRevItemHeader));
	if( count != sizeof(SLogCacheRevItemHeader))
		return -1;
	if( !CheckHeader(header))
		return -2;


	CArchive ar(&m_DataFile, CArchive::load);

	ar>>Rev.m_AuthorName;
	ar>>Rev.m_AuthorEmail;
	ar>>Rev.m_AuthorDate;
	ar>>Rev.m_CommitterName;
	ar>>Rev.m_CommitterEmail;
	ar>>Rev.m_CommitterDate;
	ar>>Rev.m_Subject;
	ar>>Rev.m_Body;
	ar>>Rev.m_CommitHash;
	ar>>Rev.m_Action;

	Rev.m_Files.Clear();

	for(int i=0;i<header.m_FileCount;i++)
	{
		CTGitPath path;
		CString file,oldfile;
		path.Reset();
		SLogCacheRevFileHeader header;

		ar>>header.m_Magic;
		ar>>header.m_Version;

		if( this->CheckHeader(header) )
			return -1;
		ar>>file;
		ar>>oldfile;
		path.SetFromGit(file,&oldfile);

		ar>>path.m_Action;
		ar>>path.m_Stage;
		ar>>path.m_StatAdd;
		ar>>path.m_StatDel;
		
		
		Rev.m_Files.AddPath(path);
	}
	return 0;
}
int CLogCache::RebuildCacheFile()
{
	m_IndexFile.SetLength(0);
	m_DataFile.SetLength(0);
	{
		SLogCacheIndexHeader header;
		header.m_Magic=LOG_INDEX_MAGIC;
		header.m_Version=LOG_INDEX_VERSION;
		m_IndexFile.SeekToBegin();
		m_IndexFile.Write(&header,sizeof(SLogCacheIndexHeader));
	}
	
	{
		SLogCacheRevFileHeader header;
		header.m_Magic=LOG_DATA_MAGIC;
		header.m_Version=LOG_INDEX_VERSION;

		m_DataFile.SeekToBegin();
		m_DataFile.Write(&header,sizeof(SLogCacheRevFileHeader));
	}
	return 0;
}
int CLogCache::SaveCache()
{
	if( this->m_NewCacheEntry.size() == 0 )
		return 0;

	bool bIsRebuild=false;
	if(this->m_DataFile.m_hFile != CFile::hFileNull)
		m_DataFile.Close();
	if(this->m_IndexFile.m_hFile!=CFile::hFileNull)
		m_IndexFile.Close();

	if(this->m_IndexFile.m_hFile == CFile::hFileNull)
	{
		BOOL b=m_IndexFile.Open(m_GitDir+_T("\\.git\\")+INDEX_FILE_NAME,
			CFile::modeReadWrite|CFile::shareDenyWrite|
			CFile::modeNoTruncate|CFile::modeCreate);
		if(!b)
			return -1;
	}

	if(this->m_DataFile.m_hFile == CFile::hFileNull)
	{
		BOOL b=m_DataFile.Open(m_GitDir+_T("\\.git\\")+DATA_FILE_NAME,
			CFile::modeReadWrite|CFile::shareDenyWrite|
			CFile::modeNoTruncate|CFile::modeCreate);

		
		if(!b)
		{
			m_IndexFile.Close();
			return -1;
		}
	}

	{
		SLogCacheIndexHeader header;
		memset(&header,0,sizeof(SLogCacheIndexHeader));
		UINT count=m_IndexFile.Read(&header,sizeof(SLogCacheIndexHeader));
		if(count != sizeof(SLogCacheIndexHeader) || !this->CheckHeader(header))
		{// new file
			RebuildCacheFile();
			bIsRebuild=true;
		}
	}

	{

		SLogCacheRevFileHeader header;
	
		UINT count=m_DataFile.Read(&header,sizeof(SLogCacheRevFileHeader));
		if( count != sizeof(SLogCacheRevFileHeader) || !CheckHeader(header))
		{
			RebuildCacheFile();
			bIsRebuild=true;
		}
	
	}

	m_DataFile.SeekToEnd();
	m_IndexFile.SeekToEnd();
	for(int i=0;i<this->m_NewCacheEntry.size();i++)
	{
		if(this->m_HashMapIndex.find(m_NewCacheEntry[i].m_CommitHash) == m_HashMapIndex.end() || bIsRebuild)
		{
			ULONGLONG offset = m_DataFile.GetPosition();
			this->SaveOneItem(m_NewCacheEntry[i],offset);

			SLogCacheItem item;
			for(int j=0; j<40;j++)
				item.m_Hash[j]=(BYTE)m_NewCacheEntry[i].m_CommitHash[j];
			item.m_Offset=offset;

			m_IndexFile.Write(&item,sizeof(SLogCacheItem));
		}
	}
	m_IndexFile.Close();
	m_DataFile.Close();
	return 0;
}