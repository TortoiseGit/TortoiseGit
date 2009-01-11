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

typedef std::vector<CString> STRING_VECTOR;
typedef std::map<CString, STRING_VECTOR> MAP_HASH_NAME;


class CGit
{
private:
	GitAdminDir m_GitDir;
public:
	static BOOL CheckMsysGitDir();

//	static CString m_MsysGitPath;
	CGit(void);
	~CGit(void);
	int Run(CString cmd, CString* output);
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
	}LOG_INFO_MASK;

	int GetRemoteList(STRING_VECTOR &list);
	int GetBranchList(STRING_VECTOR &list, int *Current,BRANCH_TYPE type=BRANCH_LOCAL);
	int GetTagList(STRING_VECTOR &list);
	int GetMapHashToFriendName(MAP_HASH_NAME &map);
	
	//hash is empty means all. -1 means all
	int GetLog(CString& logOut,CString &hash, CTGitPath *path = NULL,int count=-1,int InfoMask=LOG_INFO_STAT|LOG_INFO_FILESTATE);

	git_revnum_t GetHash(CString &friendname);

	int BuildOutputFormat(CString &format,bool IsFull=TRUE);
	int GetShortLog(CString &log,CTGitPath * path=NULL, int count =-1);
	static void StringAppend(CString *str,char *p);

	BOOL IsInitRepos();
	
};
extern void GetTempPath(CString &path);
extern CString GetTempFile();


extern CGit g_Git;