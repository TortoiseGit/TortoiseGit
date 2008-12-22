#pragma once
#include "GitRev.h"
#include "GitStatus.h"
#include "GitAdminDir.h"
enum
{
	GIT_SUCCESS=0,
	GIT_ERROR_OPEN_PIP,
	GIT_ERROR_CREATE_PROCESS,
	GIT_ERROR_GET_EXIT_CODE
};
class CGit
{
private:
	GitAdminDir m_GitDir;
public:
	static CString m_MsysGitPath;
	CGit(void);
	~CGit(void);
	int Run(CString cmd, CString* output);
	int RunAsync(CString cmd,PROCESS_INFORMATION *pi, HANDLE* hRead);
	int RunLogFile(CString cmd, CString &filename);
	CString GetUserName(void);
	CString GetUserEmail(void);
	CString GetCurrentBranch(void);

	bool SetCurrentDir(CString path)
	{
		return m_GitDir.HasAdminDir(path,&m_CurrentDir);
	}
	CString m_CurrentDir;


	int GetRemoteList(CStringList &list);
	int GetBranchList(CStringList &list, int *Current);
	
	int GetLog(CString& logOut);
	git_revnum_t GetHash(CString &friendname);
	
};
extern void GetTempPath(CString &path);
extern CString GetTempFile();


extern CGit g_Git;