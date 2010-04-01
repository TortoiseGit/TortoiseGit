#pragma once
#include "GitType.h"
#include "GitRev.h"
#include "GitStatus.h"
#include "GitAdminDir.h"
#include "gitdll.h"

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
	virtual void	OnEnd(){}

private:
	CString m_Cmd;

};

class CTGitPath;

class CGit
{
private:
	GitAdminDir m_GitDir;
protected:
	bool m_IsGitDllInited;
	GIT_DIFF m_GitDiff;
public:
	CComCriticalSection			m_critGitDllSec;

	static bool GitPathFileExists(CString &path)
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

	static BOOL CheckMsysGitDir();
	static CString ms_LastMsysGitDir;	// the last msysgitdir added to the path, blank if none
	static int m_LogEncode;
	unsigned int Hash2int(CString &hash);
//	static CString m_MsysGitPath;
	
	PROCESS_INFORMATION m_CurrentGitPi;

	CGit(void);
	~CGit(void);
	
	int Run(CString cmd, CString* output,int code);
	int Run(CString cmd, BYTE_VECTOR *byte_array);
	int Run(CGitCall* pcall);

	int RunAsync(CString cmd,PROCESS_INFORMATION *pi, HANDLE* hRead, CString *StdioFile=NULL);
	int RunLogFile(CString cmd, CString &filename);

	bool IsFastForward(CString &from, CString &to);
	CString GetConfigValue(CString name);
	CString GetUserName(void);
	CString GetUserEmail(void);
	CString GetCurrentBranch(void);
	CString GetSymbolicRef(const wchar_t* symbolicRefName = L"HEAD", bool bStripRefsHeads = true);
	// read current branch name from HEAD file, returns 0 on success, -1 on failure, 1 detached (branch name "HEAD" returned)
	int GetCurrentBranchFromFile(const CString &sProjectRoot, CString &sBranchOut);
	BOOL CheckCleanWorkTree();
	int Revert(CTGitPath &path,bool keep=true);
	int Revert(CTGitPathList &list,bool keep=true);

	bool SetCurrentDir(CString path)
	{
		bool b = m_GitDir.HasAdminDir(path,&m_CurrentDir);
		if(m_CurrentDir.GetLength() == 2 && m_CurrentDir[1] == _T(':')) //C: D:
		{
			m_CurrentDir+=_T('\\');
		}
		return b;
	}
	CString m_CurrentDir;

	typedef enum
	{
		BRANCH_LOCAL=0x1,
		BRANCH_REMOTE=0x2,
		BRANCH_ALL=BRANCH_LOCAL|BRANCH_REMOTE,
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
	}LOG_INFO_MASK;

	int GetRemoteList(STRING_VECTOR &list);
	int GetBranchList(STRING_VECTOR &list, int *Current,BRANCH_TYPE type=BRANCH_LOCAL);
	int GetTagList(STRING_VECTOR &list);
	int GetMapHashToFriendName(MAP_HASH_NAME &map);
	
	//hash is empty means all. -1 means all

	int GetLog(CGitCall* pgitCall, CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_FULL_DIFF|LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME|LOG_INFO_SHOW_MERGEDFILE,
								CString *from=NULL,CString *to=NULL);
	int GetLog(BYTE_VECTOR& logOut,CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_FULL_DIFF|LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME|LOG_INFO_SHOW_MERGEDFILE,
								CString *from=NULL,CString *to=NULL);

	CString GetLogCmd(CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_FULL_DIFF|LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME|LOG_INFO_SHOW_MERGEDFILE,
								CString *from=NULL,CString *to=NULL, bool paramonly=false);

	BOOL EnumFiles(const TCHAR *pszProjectPath, const TCHAR *pszSubPath, unsigned int nFlags, WGENUMFILECB *pEnumCb, void *pUserData);

	git_revnum_t GetHash(const CString &friendname);

	int BuildOutputFormat(CString &format,bool IsFull=TRUE);
	//int GetShortLog(CString &log,CTGitPath * path=NULL, int count =-1);
	static void StringAppend(CString *str,BYTE *p,int code=CP_UTF8,int length=-1);

	BOOL IsInitRepos();
	int ListConflictFile(CTGitPathList &list,CTGitPath *path=NULL);
	int GetRefList(STRING_VECTOR &list);

	int RefreshGitIndex();

	//Example: master -> refs/heads/master
	CString GetFullRefName(CString shortRefName);
	//Removes 'refs/heads/' or just 'refs'. Example: refs/heads/master -> master
	static CString StripRefName(CString refName);

	int GetCommitDiffList(CString &rev1,CString &rev2,CTGitPathList &outpathlist);
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
};
extern void GetTempPath(CString &path);
extern CString GetTempFile();


extern CGit g_Git;

#if 0
inline static BOOL wgEnumFiles(const TCHAR *pszProjectPath, const TCHAR *pszSubPath, unsigned int nFlags, WGENUMFILECB *pEnumCb, void *pUserData) 
{
	return g_Git.EnumFiles(pszProjectPath, pszSubPath, nFlags, pEnumCb, pUserData); 
}
#endif
