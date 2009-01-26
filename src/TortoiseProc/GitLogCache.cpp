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
		if( count != sizeof(SLogCacheItem) )
			break;

		CString str;
		g_Git.StringAppend(&str,Item.m_Hash,CP_UTF8,40);
		this->m_HashMapIndex[str]=Item.m_Offset;
	}

	return 0;
}

int CLogCache::SaveOneItem(CString &GitDir,GitRev &Rev,UINT offset)
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

		ar<<Rev.m_Files[i].m_Action;
		ar<<Rev.m_Files[i].m_Stage;
		ar<<Rev.m_Files[i].m_StatAdd;
		ar<<Rev.m_Files[i].m_StatDel;
		ar<<Rev.m_Files[i].GetGitPathString();
	}
	return 0;
}

int CLogCache::LoadOneItem(CString &GitDir,GitRev &Rev,UINT offset)
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
		CString file;
		path.Reset();
		SLogCacheRevFileHeader header;

		ar>>header.m_Magic;
		ar>>header.m_Version;

		if( this->CheckHeader(header) )
			return -1;

		ar>>path.m_Action;
		ar>>path.m_Stage;
		ar>>path.m_StatAdd;
		ar>>path.m_StatDel;
		ar>>file;
		path.SetFromGit(file);
		Rev.m_Files.AddPath(path);
	}
	return 0;
}