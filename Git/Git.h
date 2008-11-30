#pragma once
#include "GitRev.h"
#include "GitStatus.h"

enum
{
	GIT_SUCCESS=0,
	GIT_ERROR_OPEN_PIP,
	GIT_ERROR_CREATE_PROCESS
};
class CGit
{
public:

	CGit(void);
	~CGit(void);
	int Run(CString cmd, CString* output);
	int RunLogFile(CString cmd, CString &filename);
	CString GetUserName(void);
	CString GetUserEmail(void);
	CString GetCurrentBranch(void);
	CString m_CurrentDir;
	int GetLog(CString& logOut);
	git_revnum_t GetHash(CString &friendname);
	
};
extern void GetTempPath(CString &path);
extern CString GetTempFile();


extern CGit g_Git;