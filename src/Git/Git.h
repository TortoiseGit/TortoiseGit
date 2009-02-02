#pragma once
#include "GitType.h"
#include "GitRev.h"
#include "GitStatus.h"
#include "GitAdminDir.h"

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

private:
	CString m_Cmd;

};

class CTGitPath;

class CGit
{
private:
	GitAdminDir m_GitDir;
public:
	static BOOL CheckMsysGitDir();
	static CString ms_LastMsysGitDir;	// the last msysgitdir added to the path, blank if none

//	static CString m_MsysGitPath;
	CGit(void);
	~CGit(void);
	
	int Run(CString cmd, CString* output,int code);
	int Run(CString cmd, BYTE_VECTOR *byte_array);
	int Run(CGitCall* pcall);

	int RunAsync(CString cmd,PROCESS_INFORMATION *pi, HANDLE* hRead, CString *StdioFile=NULL);
	int RunLogFile(CString cmd, CString &filename);
	CString GetUserName(void);
	CString GetUserEmail(void);
	CString GetCurrentBranch(void);

	bool SetCurrentDir(CString path)
	{
		return m_GitDir.HasAdminDir(path,&m_CurrentDir);
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
		LOG_INFO_FOLLOW = 0x800
	}LOG_INFO_MASK;

	int GetRemoteList(STRING_VECTOR &list);
	int GetBranchList(STRING_VECTOR &list, int *Current,BRANCH_TYPE type=BRANCH_LOCAL);
	int GetTagList(STRING_VECTOR &list);
	int GetMapHashToFriendName(MAP_HASH_NAME &map);
	
	//hash is empty means all. -1 means all

	int GetLog(CGitCall* pgitCall, CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME);
	int GetLog(BYTE_VECTOR& logOut,CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME);

	git_revnum_t GetHash(CString &friendname);

	int BuildOutputFormat(CString &format,bool IsFull=TRUE);
	//int GetShortLog(CString &log,CTGitPath * path=NULL, int count =-1);
	static void StringAppend(CString *str,BYTE *p,int code=CP_UTF8,int length=-1);

	BOOL IsInitRepos();
	static BOOL IsVista();
	
};
extern void GetTempPath(CString &path);
extern CString GetTempFile();


extern CGit g_Git;

extern LPBYTE wgGetRevisionID_safe(const char *pszProjectPath, const char *pszName);
extern BOOL wgEnumFiles_safe(const char *pszProjectPath, const char *pszSubPath, unsigned int nFlags, WGENUMFILECB *pEnumCb, void *pUserData);
