// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#include "gitstatus.h"
#include "SharedMutex.h"

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

	int ReadIndex(CString file);
	int GetStatus(const CString &gitdir, const CString &path, git_wc_status_kind * status, BOOL IsFull=false, BOOL IsRecursive=false, FIll_STATUS_CALLBACK callback = NULL, void *pData = NULL,CGitHash *pHash=NULL, bool * assumeValid = NULL, bool * skipWorktree = NULL);
protected:
	bool m_bCheckContent;
	int GetFileStatus(const CString &gitdir, const CString &path, git_wc_status_kind * status, __int64 time, FIll_STATUS_CALLBACK callback = NULL, void *pData = NULL, CGitHash *pHash = NULL, bool * assumeValid = NULL, bool * skipWorktree = NULL);
	int GetDirStatus(const CString &gitdir,const CString &path, git_wc_status_kind * status,__int64 time,FIll_STATUS_CALLBACK callback=NULL,void *pData=NULL,CGitHash *pHash=NULL);
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
							FIll_STATUS_CALLBACK callback=NULL,
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
	SharedMutex	m_SharedMutex;

public:
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

	CGitHeadFileList()
	{
		m_LastModifyTimeHead=0;
		m_LastModifyTimeRef=0;
		m_LastModifyTimePackRef = 0;
		m_SharedMutex.Init();
	}

	~CGitHeadFileList()
	{
		m_SharedMutex.Release();
	}

	int ReadTree();
	int ReadHeadHash(CString gitdir);
	bool CheckHeadUpdate();
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
						FIll_STATUS_CALLBACK callback=NULL,void *pData=NULL,
						bool isLoaded=false);
	bool CheckHeadUpdate(const CString &gitdir);
	int GetHeadHash(const CString &gitdir, CGitHash &hash);
	int IsUnderVersionControl(const CString &gitdir, const CString &path, bool isDir, bool *isVersion);

	bool IsHashChanged(const CString &gitdir)
	{
		SHARED_TREE_PTR ptr = SafeGet(gitdir);

		if( ptr.get() == NULL)
			return false;

		return ptr->m_Head != ptr->m_TreeHash;
	}
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
	SharedMutex  m_SharedMutex;

	CGitIgnoreItem()
	{
		m_LastModifyTime =0;
		m_pExcludeList =NULL;
	}
	~CGitIgnoreItem()
	{
		if(m_pExcludeList)
			git_free_exclude_list(m_pExcludeList);
		m_pExcludeList=NULL;
	}
	__time64_t  m_LastModifyTime;
	CStringA m_BaseDir;
	EXCLUDE_LIST m_pExcludeList;
	int FetchIgnoreList(const CString &projectroot, const CString &file, bool isGlobal);
};

class CGitIgnoreList
{
private:
	bool CheckFileChanged(const CString &path);
	int FetchIgnoreFile(const CString &gitdir, const CString &gitignore, bool isGlobal);

	int  CheckIgnore(const CString &path,const CString &root);
	int CheckFileAgainstIgnoreList(const CString &ignorefile, const CStringA &patha, const char * base, int &type);

	// core.excludesfile stuff
	std::map<CString, CString> m_CoreExcludesfiles;
	CString m_sMsysGitBinPath;
	DWORD m_dMsysGitBinPathLastChecked;
	SharedMutex	m_coreExcludefilesSharedMutex;
	// checks if the msysgit path has changed and return true/false
	// if the path changed, the cache is update
	// force is only ised in constructor
	bool CheckAndUpdateMsysGitBinpath(bool force = true);
	bool CheckAndUpdateCoreExcludefile(const CString &adminDir);
	const CString GetWindowsHome();

public:
	SharedMutex		m_SharedMutex;

	CGitIgnoreList(){ m_SharedMutex.Init(); m_coreExcludefilesSharedMutex.Init(); CheckAndUpdateMsysGitBinpath(true); }
	~CGitIgnoreList() { m_SharedMutex.Release(); m_coreExcludefilesSharedMutex.Release(); }

	std::map<CString, CGitIgnoreItem> m_Map;

	bool CheckIgnoreChanged(const CString &gitdir,const CString &path);
	int  LoadAllIgnoreFile(const CString &gitdir,const CString &path);
	bool IsIgnore(const CString &path,const CString &root);
};

template<class T>
int GetRangeInSortVector(T &vector,LPTSTR pstr,int len, int *start, int *end, int pos)
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
		for (int i = 0; i < (int)vector.size(); i++)
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

		for (int i = pos; i < (int)vector.size(); i++)
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
int SearchInSortVector(T &vector, LPTSTR pstr, int len)
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

		if(cmp ==0)
			return mid;

		if(cmp < 0)
		{
			start = mid+1;
		}

		if(cmp > 0)
		{
			end=mid;
		}
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
			if (PathIsDirectory(thePath + _T("\\.git")))
			{
				(*this)[thePath] = thePath + _T("\\.git\\");
				m_reverseLookup[thePath + _T("\\.git")] = thePath;
				return (*this)[thePath];
			}
			else
			{
				FILE * pFile = _tfsopen(thePath + _T("\\.git"), _T("r"), SH_DENYWR);
				if (pFile)
				{
					int size = 65536;
					auto_buffer<char> buffer(size);
					if (fread(buffer, sizeof(char), size, pFile))
					{
						fclose(pFile);
						CString str = CString(buffer);
						if (str.Left(8) == _T("gitdir: "))
						{
							str = str.TrimRight().Mid(8);
							str.Replace(_T("/"), _T("\\"));
							str.TrimRight(_T("\\"));
							if (str.GetLength() > 0 && str[0] == _T('.'))
							{
								str = thePath + _T("\\") + str;
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
					fclose(pFile);
				}
				return thePath + _T("\\.git\\"); // in case of an error stick to old behavior
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
