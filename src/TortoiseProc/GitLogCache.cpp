#include "stdafx.h"
#include "GitLogCache.h"

CLogCache::CLogCache()
{

}

CLogCache::~CLogCache()
{
	this->m_IndexFile.Close();
	this->m_DataFile.Close();
}

int CLogCache::FetchCache(CString GitDir)
{
	if(this->m_IndexFile.m_hFile == CFile::hFileNull)
	{
		BOOL b=m_IndexFile.Open(GitDir+_T("\\.git\\")+INDEX_FILE_NAME,
						CFile::modeRead|CFile::shareDenyNone|
						CFile::modeNoTruncate|CFile::modeCreate);
		if(!b)
			return -1;
	}

	SLogCacheIndexHeader header;
	UINT count=m_IndexFile.Read(&header,sizeof(SLogCacheIndexHeader));
	if(count != sizeof(SLogCacheIndexHeader))
		return -2;

	if( header.m_Magic != LOG_INDEX_MAGIC )
		return -3;

	if( header.m_Version != LOG_INDEX_VERSION )
		return -4;


	SLogCacheItem Item;

	for(int i=0;i<header.m_ItemCount;i++)
	{
		count=m_IndexFile.Read(&Item,sizeof(SLogCacheItem));	
		if(count != sizeof(SLogCacheItem)
			break;

		CString str;
		g_Git.StringAppend(&str,Item.m_Hash,CP_UTF8,40);
		this->m_HashMapIndex[str]=Item.m_Offset;
	}

	return 0;
}

int CLogCache::SaveOneItem(GitRev &Rev)
{
	
}

int CLogCache::LoadOneItem(GitRev &Rev,UINT offset)
{
	SLogCacheRevItemHeader header;
	m_DataFile.Seek(offset,CFile::begin);

	if(this->m_DataFile.m_hFile == CFile::hFileNull)
	{
		BOOL b=m_DataFile.Open(GitDir+_T("\\.git\\")+INDEX_FILE_NAME,
						CFile::modeRead|CFile::shareDenyNone|
						CFile::modeNoTruncate|CFile::modeCreate);
		if(!b)
			return -1;
	}

	UINT count=m_DataFile.Read(&header,sizeof(SLogCacheRevItemHeader));
	if( count != sizeof(SLogCacheRevItemHeader))
		return -1;
	if( !CheckHeader(header))
		return -2;
	
	CGitByteArray stream;

	stream.resize(header.m_RevSize-sizeof(SLogCacheRevItemHeader));

	count=m_DataFile.Read(&stream[0],stream.size());
	if( count != stream.size)
		return;

	


	
}