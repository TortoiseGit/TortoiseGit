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
	this->m_HashMap[Rev.m_CommitHash] = Rev;
	return 0;
}

GitRev * CLogCache::GetCacheData(CGitHash &hash)
{
	if(this->m_HashMapIndex.find(hash)==m_HashMapIndex.end())
	{
		if(this->m_HashMap.IsExist(hash))
		{
			return &m_HashMap[hash];
		}
		m_HashMap[hash].m_CommitHash=hash;
		return &m_HashMap[hash];
	}
	else
	{
		GitRev rev;
		if(!LoadOneItem(rev,m_HashMapIndex[hash]))
		{
			m_HashMap[hash].CopyFrom(rev);
			m_HashMap[hash].m_IsFull=true;
			
			return &m_HashMap[hash];
		}
	}
	m_HashMap[hash].m_CommitHash=hash;
	return &m_HashMap[hash];
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

		this->m_HashMapIndex[Item.m_Hash]=Item.m_Offset;
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
	header.m_FileCount=Rev.GetFiles(this).GetCount();

	m_DataFile.Write(&header,sizeof(SLogCacheRevItemHeader));
	
	CArchive ar(&m_DataFile, CArchive::store);

	ar<<Rev.GetAuthorName();
	ar<<Rev.GetAuthorEmail();
	ar<<Rev.GetAuthorDate();
	ar<<Rev.GetCommitterName();
	ar<<Rev.GetCommitterEmail();
	ar<<Rev.GetCommitterDate();
	ar<<Rev.GetSubject();
	ar<<Rev.GetBody();
	ar<<Rev.m_CommitHash;
	ar<<Rev.GetAction(this);

	for(int i=0;i<Rev.GetFiles(this).GetCount();i++)
	{
		SLogCacheRevFileHeader header;
		header.m_Magic=LOG_DATA_FILE_MAGIC;
		header.m_Version=LOG_INDEX_VERSION;

		ar<<header.m_Magic;
		ar<<header.m_Version;
		ar<<Rev.GetFiles(this)[i].GetGitPathString();
		ar<<Rev.GetFiles(this)[i].GetGitOldPathString();
		ar<<Rev.GetFiles(this)[i].m_Action;
		ar<<Rev.GetFiles(this)[i].m_Stage;
		ar<<Rev.GetFiles(this)[i].m_StatAdd;
		ar<<Rev.GetFiles(this)[i].m_StatDel;
		ar<<Rev.GetFiles(this)[i].m_ParentNo;
		
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

	ar>>Rev.GetAuthorName();
	ar>>Rev.GetAuthorEmail();
	ar>>Rev.GetAuthorDate();
	ar>>Rev.GetCommitterName();
	ar>>Rev.GetCommitterEmail();
	ar>>Rev.GetCommitterDate();
	ar>>Rev.GetSubject();
	ar>>Rev.GetBody();
	ar>>Rev.m_CommitHash;
	ar>>Rev.GetAction(this);

	Rev.GetFiles(this).Clear();

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
		ar>>path.m_ParentNo;
		
		Rev.GetFiles(this).AddPath(path);
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
	if( this->m_HashMap.size() == 0 )
		return 0;

	if( this->m_GitDir.IsEmpty())
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
	CGitHashMap::iterator i;
	for(i=m_HashMap.begin();i!=m_HashMap.end();i++)
	{
		if(this->m_HashMapIndex.find((*i).second.m_CommitHash) == m_HashMapIndex.end() || bIsRebuild)
		{
			if((*i).second.m_IsFull && !(*i).second.m_CommitHash.IsEmpty())
			{
				ULONGLONG offset = m_DataFile.GetPosition();
				this->SaveOneItem((*i).second,offset);

				SLogCacheItem item;
				item.m_Hash = (*i).second.m_CommitHash;
				item.m_Offset=offset;

				m_IndexFile.Write(&item,sizeof(SLogCacheItem));
			}
		}
	}
	m_IndexFile.Close();
	m_DataFile.Close();
	return 0;
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