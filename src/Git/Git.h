// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013 - TortoiseGit

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
#include "GitType.h"
#include "GitRev.h"
#include "GitStatus.h"
#include "GitAdminDir.h"
#include "gitdll.h"

class CFilterData
{
public:
	CFilterData()
	{
		m_From=m_To=-1;
		m_IsRegex=1;
	}
	__time64_t m_From;
	__time64_t m_To;
	CString m_Author;
	CString m_Committer;
	CString m_MessageFilter;
	BOOL m_IsRegex;
};

class CGitCall
{
public:
	CGitCall(){}
	CGitCall(CString cmd):m_Cmd(cmd){}

	CString			GetCmd()const{return m_Cmd;}
	void			SetCmd(CString cmd){m_Cmd=cmd;}

	//This function is called when command output data is available.
	//When this function returns 'true' the git command should be aborted.
	//This behavior is not implemented yet.
	virtual bool	OnOutputData(const BYTE* data, size_t size)=0;
	virtual bool	OnOutputErrData(const BYTE* data, size_t size)=0;
	virtual void	OnEnd(){}

private:
	CString m_Cmd;

};

class CTGitPath;
class CEnvironment:public std::vector<TCHAR>
{
public:
	void CopyProcessEnvironment();
	CString GetEnv(TCHAR *name);
	void SetEnv(TCHAR* name, TCHAR* value);
};
class CGit
{
private:
	GitAdminDir m_GitDir;
	CString		gitLastErr;
protected:
	bool m_IsGitDllInited;
	GIT_DIFF m_GitDiff;
	GIT_DIFF m_GitSimpleListDiff;
public:
	CComCriticalSection			m_critGitDllSec;
	bool	m_IsUseGitDLL;
	bool	m_IsUseLibGit2;
	DWORD	m_IsUseLibGit2_Mask1;

	CEnvironment m_Environment;

	static BOOL GitPathFileExists(const CString &path)
	{
		if(path[0] == _T('\\') && path[1] == _T('\\'))
		//it is netshare \\server\sharefoldername
		// \\server\.git will create smb error log.
		{
			int length = path.GetLength();

			if(length<2)
				return false;

			int start = path.Find(_T('\\'),2);
			if(start<0)
				return false;

			start = path.Find(_T('\\'),start+1);
			if(start<0)
				return false;

			return PathFileExists(path);

		}
		else
			return PathFileExists(path);
	}
	void CheckAndInitDll()
	{
		if(!m_IsGitDllInited)
		{
			git_init();
			m_IsGitDllInited=true;
		}
	}

	GIT_DIFF GetGitDiff()
	{
		if(m_GitDiff)
			return m_GitDiff;
		else
		{
			git_open_diff(&m_GitDiff,"-C -M -r");
			return m_GitDiff;
		}
	}

	GIT_DIFF GetGitSimpleListDiff()
	{
		if(m_GitSimpleListDiff)
			return m_GitSimpleListDiff;
		else
		{
			git_open_diff(&m_GitSimpleListDiff,"-r -r");
			return m_GitSimpleListDiff;
		}
	}

	BOOL CheckMsysGitDir();
	BOOL m_bInitialized;

	CString GetHomeDirectory();
	CString GetGitLocalConfig();
	CString GetGitGlobalConfig();
	CString GetGitGlobalXDGConfigPath();
	CString GetGitGlobalXDGConfig();
	CString GetGitSystemConfig();
	static CString ms_LastMsysGitDir;	// the last msysgitdir added to the path, blank if none
	static int m_LogEncode;
	static bool IsBranchNameValid(CString branchname);
	bool IsBranchTagNameUnique(const CString& name);
	bool BranchTagExists(const CString& name, bool isBranch = true);
	unsigned int Hash2int(CGitHash &hash);
//	static CString m_MsysGitPath;

	PROCESS_INFORMATION m_CurrentGitPi;

	CGit(void);
	~CGit(void);

	int Run(CString cmd, CString* output, int code);
	int Run(CString cmd, CString* output, CString* outputErr, int code);
	int Run(CString cmd, BYTE_VECTOR *byte_array, BYTE_VECTOR *byte_arrayErr = NULL);
	int Run(CGitCall* pcall);

private:
	static DWORD WINAPI AsyncReadStdErrThread(LPVOID lpParam);
	typedef struct AsyncReadStdErrThreadArguments
	{
		HANDLE fileHandle;
		CGitCall* pcall;
	} ASYNCREADSTDERRTHREADARGS, *PASYNCREADSTDERRTHREADARGS;
	CString GetUnifiedDiffCmd(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, bool bMerge, bool bCombine);

public:
	int RunAsync(CString cmd, PROCESS_INFORMATION *pi, HANDLE* hRead, HANDLE *hErrReadOut, CString *StdioFile = NULL);
	int RunLogFile(CString cmd, const CString &filename);

	int GetDiffPath(CTGitPathList *PathList, CGitHash *hash1, CGitHash *hash2, char *arg=NULL);

	int GetGitEncode(TCHAR* configkey);

	bool IsFastForward(const CString &from, const CString &to, CGitHash * commonAncestor = NULL);
	CString GetConfigValue(CString name, int encoding=CP_UTF8, CString *GitPath=NULL,BOOL RemoveCR=TRUE);

	int SetConfigValue(CString key, CString value, CONFIG_TYPE type=CONFIG_LOCAL, int encoding=CP_UTF8, CString *GitPath=NULL);
	int UnsetConfigValue(CString key, CONFIG_TYPE type=CONFIG_LOCAL, int encoding=CP_UTF8, CString *GitPath=NULL);

	CString GetUserName(void);
	CString GetUserEmail(void);
	CString GetCurrentBranch(void);
	CString GetSymbolicRef(const wchar_t* symbolicRefName = L"HEAD", bool bStripRefsHeads = true);
	// read current branch name from HEAD file, returns 0 on success, -1 on failure, 1 detached (branch name "HEAD" returned)
	int GetCurrentBranchFromFile(const CString &sProjectRoot, CString &sBranchOut);
	BOOL CheckCleanWorkTree();
	int Revert(CString commit, CTGitPathList &list, bool keep=true);
	int Revert(CString commit, CTGitPath &path);
	CString GetGitLastErr(CString msg);
	bool SetCurrentDir(CString path, bool submodule = false)
	{
		bool b = m_GitDir.HasAdminDir(path, submodule ? false : !!PathIsDirectory(path), &m_CurrentDir);
		if (!b && g_GitAdminDir.IsBareRepo(path))
		{
			m_CurrentDir = path;
			b = true;
		}
		if(m_CurrentDir.GetLength() == 2 && m_CurrentDir[1] == _T(':')) //C: D:
		{
			m_CurrentDir += _T('\\');
		}
		return b;
	}
	CString m_CurrentDir;

	typedef enum
	{
		BRANCH_LOCAL		= 0x1,
		BRANCH_REMOTE		= 0x2,
		BRANCH_FETCH_HEAD	= 0x4,
		BRANCH_LOCAL_F		= BRANCH_LOCAL	| BRANCH_FETCH_HEAD,
		BRANCH_ALL			= BRANCH_LOCAL	| BRANCH_REMOTE,
		BRANCH_ALL_F		= BRANCH_ALL	| BRANCH_FETCH_HEAD,
	}BRANCH_TYPE;

	typedef enum
	{
		LOG_INFO_STAT=0x1,
		LOG_INFO_FILESTATE=0x2,
		LOG_INFO_PATCH=0x4,
		LOG_INFO_FULLHISTORY=0x8,
		LOG_INFO_BOUNDARY=0x10,
        LOG_INFO_ALL_BRANCH=0x20,
		LOG_INFO_ONLY_HASH=0x40,
		LOG_INFO_DETECT_RENAME=0x80,
		LOG_INFO_DETECT_COPYRENAME=0x100,
		LOG_INFO_FIRST_PARENT = 0x200,
		LOG_INFO_NO_MERGE = 0x400,
		LOG_INFO_FOLLOW = 0x800,
		LOG_INFO_SHOW_MERGEDFILE=0x1000,
		LOG_INFO_FULL_DIFF = 0x2000,
		LOG_INFO_SIMPILFY_BY_DECORATION = 0x4000, 
	}LOG_INFO_MASK;

	typedef enum
	{
		LOCAL_BRANCH,
		REMOTE_BRANCH,
		TAG,
		STASH,
		BISECT_GOOD,
		BISECT_BAD,
		NOTES,
		UNKNOWN,

	}REF_TYPE;

	int GetRemoteList(STRING_VECTOR &list);
	int GetBranchList(STRING_VECTOR &list, int *Current,BRANCH_TYPE type=BRANCH_LOCAL);
	int GetTagList(STRING_VECTOR &list);
	int GetRemoteTags(CString remote, STRING_VECTOR &list);
	int GetMapHashToFriendName(MAP_HASH_NAME &map);

	CString DerefFetchHead();

	// FixBranchName():
	// When branchName == FETCH_HEAD, derefrence it.
	// A selected branch name got from GetBranchList(), with flag BRANCH_FETCH_HEAD enabled,
	// should go through this function before it is used.
	CString	FixBranchName_Mod(CString& branchName);
	CString	FixBranchName(const CString& branchName);

	//hash is empty means all. -1 means all

	int GetLog(CGitCall* pgitCall, const CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_FULL_DIFF|LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME|LOG_INFO_SHOW_MERGEDFILE,
								CString *from=NULL,CString *to=NULL);
	int GetLog(BYTE_VECTOR& logOut,const CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_FULL_DIFF|LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME|LOG_INFO_SHOW_MERGEDFILE,
								CString *from=NULL,CString *to=NULL);

	CString GetLogCmd(const CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_FULL_DIFF|LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME|LOG_INFO_SHOW_MERGEDFILE,
					  CString *from=NULL,CString *to=NULL, bool paramonly=false,
					  CFilterData * filter =NULL);

	int GetHash(CGitHash &hash, TCHAR* friendname);
	int GetHash(CGitHash &hash, CString ref) { return GetHash(hash, ref.GetBuffer()); }

	int BuildOutputFormat(CString &format,bool IsFull=TRUE);
	//int GetShortLog(const CString &log,CTGitPath * path=NULL, int count =-1);
	static void StringAppend(CString *str,BYTE *p,int code=CP_UTF8,int length=-1);

	BOOL IsOrphanBranch(CString ref);
	BOOL IsInitRepos();
	int ListConflictFile(CTGitPathList &list,CTGitPath *path=NULL);
	int GetRefList(STRING_VECTOR &list);

	int RefreshGitIndex();
	int GetOneFile(CString Refname, CTGitPath &path, const CString &outputfile);

	//Example: master -> refs/heads/master
	CString GetFullRefName(CString shortRefName);
	//Removes 'refs/heads/' or just 'refs'. Example: refs/heads/master -> master
	static CString StripRefName(CString refName);

	int GetCommitDiffList(const CString &rev1,const CString &rev2,CTGitPathList &outpathlist);
	int GetInitAddList(CTGitPathList &outpathlist);

	__int64 filetime_to_time_t(const FILETIME *ft)
	{
		long long winTime = ((long long)ft->dwHighDateTime << 32) + ft->dwLowDateTime;
		winTime -= 116444736000000000LL; /* Windows to Unix Epoch conversion */
		winTime /= 10000000;		 /* Nano to seconds resolution */
		return (time_t)winTime;
	}

	int GetFileModifyTime(LPCTSTR filename, __int64 *time, bool * isDir=NULL)
	{
		WIN32_FILE_ATTRIBUTE_DATA fdata;
		if (GetFileAttributesEx(filename, GetFileExInfoStandard, &fdata))
		{
			if(time)
				*time = filetime_to_time_t(&fdata.ftLastWriteTime);

			if(isDir)
				*isDir = !!( fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);

			return 0;
		}
		return -1;
	}

	int GetShortHASHLength();

	static BOOL GetShortName(CString ref, CString &shortname, CString prefix)
	{
		//TRACE(_T("%s %s\r\n"),ref,prefix);
		if (ref.Left(prefix.GetLength()) ==  prefix)
		{
			shortname = ref.Right(ref.GetLength() - prefix.GetLength());
			if (shortname.Right(3) == _T("^{}"))
				shortname=shortname.Left(shortname.GetLength() - 3);
			return TRUE;
		}
		return FALSE;
	}

	static CString GetShortName(CString ref, REF_TYPE *type);

	int GetUnifiedDiff(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, CString patchfile, bool bMerge, bool bCombine);
	int GetUnifiedDiff(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, CStringA * buffer, bool bMerge, bool bCombine);

	enum
	{
		GIT_CMD_INIT,
		GIT_CMD_CLONE,
		GIT_CMD_DIFF,
	};
	BOOL UsingLibGit2(int cmd);

};
extern void GetTempPath(CString &path);
extern CString GetTempFile();
extern DWORD GetTortoiseGitTempPath(DWORD nBufferLength, LPTSTR lpBuffer);

extern CGit g_Git;
