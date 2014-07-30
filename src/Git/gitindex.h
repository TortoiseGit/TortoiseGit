// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit

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

#include "GitHash.h"
#include "gitdll.h"
#include "GitStatus.h"
#include "UnicodeUtils.h"
#include "ReaderWriterLock.h"

class CGitIndex
{
public:
	CString    m_FileName;
	__time64_t	m_ModifyTime;
	unsigned short m_Flags;
	CGitHash	m_IndexHash;

	int Print();
};

class CGitIndexList:public std::vector<CGitIndex>
{
protected:

public:
	__time64_t  m_LastModifyTime;

	CGitIndexList();
	~CGitIndexList();

	int ReadIndex(CString file);
	int GetStatus(const CString &gitdir, const CString &path, git_wc_status_kind * status, BOOL IsFull = false, BOOL IsRecursive = false, FILL_STATUS_CALLBACK callback = nullptr, void *pData = nullptr, CGitHash *pHash = nullptr, bool * assumeValid = nullptr, bool * skipWorktree = nullptr);
protected:
	bool m_bCheckContent;
	CComCriticalSection m_critRepoSec;
	CAutoRepository repository;
	int GetFileStatus(const CString &gitdir, const CString &path, git_wc_status_kind * status, __int64 time, FILL_STATUS_CALLBACK callback = nullptr, void *pData = nullptr, CGitHash *pHash = nullptr, bool * assumeValid = nullptr, bool * skipWorktree = nullptr);
	int GetDirStatus(const CString &gitdir, const CString &path, git_wc_status_kind * status,__int64 time, FILL_STATUS_CALLBACK callback = nullptr, void *pData = nullptr,CGitHash *pHash = nullptr);
};

typedef std::tr1::shared_ptr<CGitIndexList> SHARED_INDEX_PTR;
typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

class CGitIndexFileMap:public std::map<CString, SHARED_INDEX_PTR>
{
public:
	CComCriticalSection			m_critIndexSec;

	CGitIndexFileMap() { m_critIndexSec.Init(); }
	~CGitIndexFileMap() { m_critIndexSec.Term(); }

	SHARED_INDEX_PTR SafeGet(const CString &path)
	{
		CString thePath = path;
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		if(this->find(thePath) == end())
			return SHARED_INDEX_PTR();
		else
			return (*this)[thePath];
	}

	void SafeSet(const CString &path, SHARED_INDEX_PTR ptr)
	{
		CString thePath = path;
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		(*this)[thePath] = ptr;
	}

	bool SafeClear(const CString &path)
	{
		CString thePath = path;
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		if (this->find(thePath) == end())
			return false;
		(*this)[thePath] = nullptr;
		return true;
	}

	bool SafeClearRecursively(const CString &rootpath)
	{
		CString thePath = rootpath;
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		for (auto it = this->begin(); it != this->end(); ++it)
		{
			if ((*it).first.Find(thePath) == 0)
				(*this)[(*it).first] = nullptr;
		}
		return true;
	}

	int Check(const CString &gitdir, bool *isChanged);
	int LoadIndex(const CString &gitdir);

	bool CheckAndUpdate(const CString &gitdir,bool isLoadUpdatedIndex)
	{
		bool isChanged=false;
		if(isLoadUpdatedIndex && Check(gitdir,&isChanged))
			return false;

		if(isChanged && isLoadUpdatedIndex)
		{
			LoadIndex(gitdir);
			return true;
		}

		return false;
	}
	int GetFileStatus(const CString &gitdir,const CString &path,git_wc_status_kind * status,
							BOOL IsFull=false, BOOL IsRecursive=false,
							FILL_STATUS_CALLBACK callback = nullptr,
							void *pData=NULL,CGitHash *pHash=NULL,
							bool isLoadUpdatedIndex = true, bool * assumeValid = NULL, bool * skipWorktree = NULL);

	int IsUnderVersionControl(const CString &gitdir,
							  const CString &path,
							  bool isDir,
							  bool *isVersion,
							  bool isLoadUpdateIndex=true);
};


class CGitTreeItem
{
public:
	CString	m_FileName;
	CGitHash	m_Hash;
	int			m_Flags;
};

/* After object create, never change field agains
 * that needn't lock to get field
*/
class CGitHeadFileList:public std::vector<CGitTreeItem>
{
private:
	int GetPackRef(const CString &gitdir);
	CReaderWriterLock m_SharedMutex;

	__time64_t  m_LastModifyTimeHead;
	__time64_t  m_LastModifyTimeRef;
	__time64_t	m_LastModifyTimePackRef;

	CString		m_HeadRefFile;
	CGitHash	m_Head;
	CString		m_HeadFile;
	CString		m_Gitdir;
	CString		m_PackRefFile;

	CGitHash	m_TreeHash; /* buffered tree hash value */

	std::map<CString,CGitHash> m_PackRefMap;

public:
	CGitHeadFileList()
	{
		m_LastModifyTimeHead=0;
		m_LastModifyTimeRef=0;
		m_LastModifyTimePackRef = 0;
	}

	int ReadTree();
	int ReadHeadHash(CString gitdir);
	bool CheckHeadUpdate();
	bool HeadHashEqualsTreeHash();
	bool HeadFileIsEmpty();
	bool HeadIsEmpty();
	static int CallBack(const unsigned char *, const char *, int, const char *, unsigned int, int, void *);
};

typedef std::tr1::shared_ptr<CGitHeadFileList> SHARED_TREE_PTR;
class CGitHeadFileMap:public std::map<CString,SHARED_TREE_PTR>
{
public:

	CComCriticalSection			m_critTreeSec;

	CGitHeadFileMap() { m_critTreeSec.Init(); }
	~CGitHeadFileMap() { m_critTreeSec.Term(); }

	SHARED_TREE_PTR SafeGet(const CString &path)
	{
		CString thePath = path;
		thePath.MakeLower();
		CAutoLocker lock(m_critTreeSec);
		if(this->find(thePath) == end())
			return SHARED_TREE_PTR();
		else
			return (*this)[thePath];
	}

	void SafeSet(const CString &path, SHARED_TREE_PTR ptr)
	{
		CString thePath = path;
		thePath.MakeLower();
		CAutoLocker lock(m_critTreeSec);
		(*this)[thePath] = ptr;
	}

	int GetFileStatus(const CString &gitdir,const CString &path,git_wc_status_kind * status,BOOL IsFull=false, BOOL IsRecursive=false,
						FILL_STATUS_CALLBACK callback = nullptr, void *pData = nullptr,
						bool isLoaded=false);
	bool CheckHeadAndUpdate(const CString &gitdir, bool readTree = true);
	int IsUnderVersionControl(const CString &gitdir, const CString &path, bool isDir, bool *isVersion);
};

class CGitFileName
{
public:
	CString m_FileName;
	CString m_CaseFileName;
};

class CGitIgnoreItem
{
public:
	CReaderWriterLock  m_SharedMutex;

	CGitIgnoreItem()
	{
		m_LastModifyTime =0;
		m_pExcludeList =NULL;
		m_buffer = NULL;
	}
	~CGitIgnoreItem()
	{
		if(m_pExcludeList)
			git_free_exclude_list(m_pExcludeList);
		if (m_buffer)
			free(m_buffer);
		m_pExcludeList=NULL;
		m_buffer = NULL;
	}
	__time64_t  m_LastModifyTime;
	CStringA m_BaseDir;
	BYTE *m_buffer;
	EXCLUDE_LIST m_pExcludeList;

	int FetchIgnoreList(const CString &projectroot, const CString &file, bool isGlobal);
};

class CGitIgnoreList
{
private:
	bool CheckFileChanged(const CString &path);
	int FetchIgnoreFile(const CString &gitdir, const CString &gitignore, bool isGlobal);

	int  CheckIgnore(const CString &path, const CString &root, bool isDir);
	int CheckFileAgainstIgnoreList(const CString &ignorefile, const CStringA &patha, const char * base, int &type);

	// core.excludesfile stuff
	std::map<CString, CString> m_CoreExcludesfiles;
	CString m_sMsysGitBinPath;
	DWORD m_dMsysGitBinPathLastChecked;
	CReaderWriterLock	m_coreExcludefilesSharedMutex;
	// checks if the msysgit path has changed and return true/false
	// if the path changed, the cache is update
	// force is only ised in constructor
	bool CheckAndUpdateMsysGitBinpath(bool force = true);
	bool CheckAndUpdateCoreExcludefile(const CString &adminDir);
	const CString GetWindowsHome();

public:
	CReaderWriterLock		m_SharedMutex;

	CGitIgnoreList(){ CheckAndUpdateMsysGitBinpath(true); }

	std::map<CString, CGitIgnoreItem> m_Map;

	bool CheckIgnoreChanged(const CString &gitdir,const CString &path, bool isDir);
	int  LoadAllIgnoreFile(const CString &gitdir, const CString &path, bool isDir);
	bool IsIgnore(const CString &path, const CString &root, bool isDir);
};

template<class T>
int GetRangeInSortVector(const T &vector, LPCTSTR pstr, int len, int *start, int *end, int pos)
{
	if( pos < 0)
	{
		return -1;
	}
	if(start == 0 || end == NULL)
		return -1;

	*start=*end=-1;

	if (vector.empty())
		return -1;

	if (pos >= (int)vector.size())
		return -1;

	if( _tcsnccmp(vector[pos].m_FileName, pstr,len) != 0)
	{
		for (int i = 0; i < (int)vector.size(); ++i)
		{
			if( _tcsnccmp(vector[i].m_FileName, pstr,len) == 0 )
			{
				if(*start<0)
					*start =i;
				*end =i;
			}
		}
		return -1;
	}
	else
	{
		*start =0;
		*end = (int)vector.size();

		for (int i = pos; i < (int)vector.size(); ++i)
		{
			if( _tcsnccmp(vector[i].m_FileName, pstr,len) == 0 )
			{
				*end=i;
			}
			else
			{
				break;
			}
		}
		for(int i=pos;i>=0;i--)
		{
			if( _tcsnccmp(vector[i].m_FileName, pstr,len) == 0 )
			{
				*start=i;
			}
			else
			{
				break;
			}
		}
	}
	return 0;
}

template<class T>
int SearchInSortVector(const T &vector, LPCTSTR pstr, int len)
{
	int end = (int)vector.size() - 1;
	int start = 0;
	int mid = (start+end)/2;

	if (vector.empty())
		return -1;

	while(!( start == end && start==mid))
	{
		int cmp;
		if(len < 0)
			cmp = _tcscmp(vector[mid].m_FileName,pstr);
		else
			cmp = _tcsnccmp( vector[mid].m_FileName,pstr,len );

		if (cmp == 0)
			return mid;
		else if (cmp < 0)
			start = mid + 1;
		else // (cmp > 0)
			end = mid;

		mid=(start +end ) /2;

	}
	if(len <0)
	{
		if(_tcscmp(vector[mid].m_FileName,pstr) == 0)
			return mid;
	}
	else
	{
		if(_tcsnccmp( vector[mid].m_FileName,pstr,len ) == 0)
			return mid;
	}
	return -1;
};

class CGitAdminDirMap:public std::map<CString, CString>
{
public:
	CComCriticalSection			m_critIndexSec;
	std::map<CString, CString>	m_reverseLookup;

	CGitAdminDirMap() { m_critIndexSec.Init(); }
	~CGitAdminDirMap() { m_critIndexSec.Term(); }

	CString GetAdminDir(const CString &path)
	{
		CString thePath = path;
		thePath.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		if(this->find(thePath) == end())
		{
			if (PathIsDirectory(path + _T("\\.git")))
			{
				(*this)[thePath] = path + _T("\\.git\\");
				m_reverseLookup[thePath + _T("\\.git")] = path;
				return (*this)[thePath];
			}
			else
			{
				FILE * pFile = _tfsopen(path + _T("\\.git"), _T("r"), SH_DENYWR);
				if (pFile)
				{
					int size = 65536;
					std::unique_ptr<char[]> buffer(new char[size]);
					int length = 0;
					if ((length = (int)fread(buffer.get(), sizeof(char), size, pFile)) >= 8)
					{
						fclose(pFile);
						CStringA strA(buffer.get(), length);
						if (strA.Left(8) == "gitdir: ")
						{
							CString str = CUnicodeUtils::GetUnicode(strA.Trim().Mid(8)); // 8 = len("gitdir: ")
							str.Replace(_T("/"), _T("\\"));
							str.TrimRight(_T("\\"));
							if (str.GetLength() > 0 && str[0] == _T('.'))
							{
								str = path + _T("\\") + str;
								CString newPath;
								PathCanonicalize(newPath.GetBuffer(MAX_PATH), str.GetBuffer());
								newPath.ReleaseBuffer();
								str.ReleaseBuffer();
								str = newPath;
							}
							(*this)[thePath] = str + _T("\\");
							m_reverseLookup[str.MakeLower()] = path;
							return (*this)[thePath];
						}
					}
					else
						fclose(pFile);
				}
				return path + _T("\\.git\\"); // in case of an error stick to old behavior
			}
		}
		else
			return (*this)[thePath];
	}

	CString GetWorkingCopy(const CString &gitDir)
	{
		CString path = gitDir;
		path.MakeLower();
		CAutoLocker lock(m_critIndexSec);
		if (m_reverseLookup.find(path) == m_reverseLookup.end())
			return gitDir;
		else
			return m_reverseLookup[path];
	}
};
