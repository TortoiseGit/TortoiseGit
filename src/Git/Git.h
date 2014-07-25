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

#pragma once
#include "GitType.h"
#include "GitRev.h"
#include "GitStatus.h"
#include "GitAdminDir.h"
#include "gitdll.h"
#include <functional>

#define REG_MSYSGIT_PATH _T("Software\\TortoiseGit\\MSysGit")
#define REG_MSYSGIT_EXTRA_PATH _T("Software\\TortoiseGit\\MSysGitExtra")

struct git_repository;

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

typedef std::function<void (const CStringA&)> GitReceiverFunc;

class CTGitPath;
class CEnvironment:public std::vector<TCHAR>
{
public:
	void CopyProcessEnvironment();
	CString GetEnv(const TCHAR *name);
	void SetEnv(const TCHAR* name, const TCHAR* value);
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
	DWORD	m_IsUseLibGit2_mask;

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

	BOOL CheckMsysGitDir(BOOL bFallback = TRUE);
	BOOL m_bInitialized;

	typedef enum
	{
		GIT_CMD_CLONE,
		GIT_CMD_FETCH,
		GIT_CMD_COMMIT_UPDATE_INDEX,
		GIT_CMD_DIFF,
		GIT_CMD_RESET,
		GIT_CMD_REVERT,
		GIT_CMD_MERGE_BASE,
		GIT_CMD_DELETETAGBRANCH,
		GIT_CMD_GETONEFILE,
		GIT_CMD_ADD,
	} LIBGIT2_CMD;
	bool UsingLibGit2(LIBGIT2_CMD cmd) const;

	CString GetHomeDirectory() const;
	CString GetGitLocalConfig() const;
	CString GetGitGlobalConfig() const;
	CString GetGitGlobalXDGConfigPath() const;
	CString GetGitGlobalXDGConfig() const;
	CString GetGitSystemConfig() const;
	git_repository * GetGitRepository() const;
	static CStringA GetGitPathStringA(const CString &path);
	static CString ms_LastMsysGitDir;	// the last msysgitdir added to the path, blank if none
	static int ms_LastMsysGitVersion;
	static int m_LogEncode;
	static bool IsBranchNameValid(const CString& branchname);
	bool IsBranchTagNameUnique(const CString& name);
	/**
	* Checks if a branch or tag with the given name exists
	*isBranch is true -> branch, tag otherwise
	*/
	bool BranchTagExists(const CString& name, bool isBranch = true);
	unsigned int Hash2int(const CGitHash &hash);

	PROCESS_INFORMATION m_CurrentGitPi;

	CGit(void);
	~CGit(void);

	int Run(CString cmd, CString* output, int code);
	int Run(CString cmd, CString* output, CString* outputErr, int code);
	int Run(CString cmd, BYTE_VECTOR *byte_array, BYTE_VECTOR *byte_arrayErr = NULL);
	int Run(CGitCall* pcall);
	int Run(CString cmd, const GitReceiverFunc& recv);

private:
	static DWORD WINAPI AsyncReadStdErrThread(LPVOID lpParam);
	typedef struct AsyncReadStdErrThreadArguments
	{
		HANDLE fileHandle;
		CGitCall* pcall;
	} ASYNCREADSTDERRTHREADARGS, *PASYNCREADSTDERRTHREADARGS;
	CString GetUnifiedDiffCmd(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, bool bMerge, bool bCombine, int diffContext);

public:
	int RunAsync(CString cmd, PROCESS_INFORMATION *pi, HANDLE* hRead, HANDLE *hErrReadOut, CString *StdioFile = NULL);
	int RunLogFile(CString cmd, const CString &filename, CString *stdErr);

	int GetDiffPath(CTGitPathList *PathList, CGitHash *hash1, CGitHash *hash2, char *arg=NULL);

	int GetGitEncode(TCHAR* configkey);

	bool IsFastForward(const CString &from, const CString &to, CGitHash * commonAncestor = NULL);
	CString GetConfigValue(const CString& name);
	bool GetConfigValueBool(const CString& name);
	int GetConfigValueInt32(const CString& name, int def = 0);

	int SetConfigValue(const CString& key, const CString& value, CONFIG_TYPE type = CONFIG_LOCAL);
	int UnsetConfigValue(const CString& key, CONFIG_TYPE type = CONFIG_LOCAL);

	CString GetUserName(void);
	CString GetUserEmail(void);
	CString GetCurrentBranch(bool fallback = false);
	void GetRemoteTrackedBranch(const CString& localBranch, CString& remote, CString& branch);
	void GetRemoteTrackedBranchForHEAD(CString& remote, CString& branch);
	// read current branch name from HEAD file, returns 0 on success, -1 on failure, 1 detached (branch name "HEAD" returned)
	static int GetCurrentBranchFromFile(const CString &sProjectRoot, CString &sBranchOut, bool fallback = false);
	BOOL CheckCleanWorkTree();
	int Revert(const CString& commit, const CTGitPathList &list, bool keep=true);
	int Revert(const CString& commit, const CTGitPath &path);
	int DeleteRef(const CString& reference);
	/**
	Use this method only if m_IsUseLibGit2 is used for fallbacks.
	If you directly use libgit2 methods, use GetLibGit2LastErr instead.
	*/
	CString GetGitLastErr(const CString& msg);
	CString GetGitLastErr(const CString& msg, LIBGIT2_CMD cmd);
	static CString GetLibGit2LastErr();
	static CString GetLibGit2LastErr(const CString& msg);
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

	enum
	{
		LOG_ORDER_CHRONOLOGIALREVERSED,
		LOG_ORDER_TOPOORDER,
		LOG_ORDER_DATEORDER,
	};

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
		LOG_INFO_LOCAL_BRANCHES = 0x8000,
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
	int GetRemoteTags(const CString& remote, STRING_VECTOR &list);
	int GetMapHashToFriendName(MAP_HASH_NAME &map);
	static int GetMapHashToFriendName(git_repository* repo, MAP_HASH_NAME &map);

	CString DerefFetchHead();

	// FixBranchName():
	// When branchName == FETCH_HEAD, derefrence it.
	// A selected branch name got from GetBranchList(), with flag BRANCH_FETCH_HEAD enabled,
	// should go through this function before it is used.
	CString	FixBranchName_Mod(CString& branchName);
	CString	FixBranchName(const CString& branchName);

	CString GetLogCmd(const CString &range, const CTGitPath *path = NULL, int count=-1, int InfoMask = LOG_INFO_FULL_DIFF|LOG_INFO_STAT|LOG_INFO_FILESTATE|LOG_INFO_BOUNDARY|LOG_INFO_DETECT_COPYRENAME|LOG_INFO_SHOW_MERGEDFILE, bool paramonly=false, CFilterData * filter =NULL);

	int GetHash(CGitHash &hash, const CString& friendname);
	static int GetHash(git_repository * repo, CGitHash &hash, const CString& friendname, bool skipFastCheck = false);

	int BuildOutputFormat(CString &format,bool IsFull=TRUE);
	static void StringAppend(CString *str, const BYTE *p, int code = CP_UTF8, int length = -1);

	BOOL CanParseRev(CString ref);
	BOOL IsInitRepos();
	int ListConflictFile(CTGitPathList &list, const CTGitPath *path = nullptr);
	int GetRefList(STRING_VECTOR &list);

	int RefreshGitIndex();
	int GetOneFile(const CString &Refname, const CTGitPath &path, const CString &outputfile);

	//Example: master -> refs/heads/master
	CString GetFullRefName(const CString& shortRefName);
	//Removes 'refs/heads/' or just 'refs'. Example: refs/heads/master -> master
	static CString StripRefName(CString refName);

	int GetCommitDiffList(const CString &rev1, const CString &rev2, CTGitPathList &outpathlist, bool ignoreSpaceAtEol = false, bool ignoreSpaceChange = false, bool ignoreAllSpace = false, bool ignoreBlankLines = false);
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

	int GetShortHASHLength() const;

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

	static CString GetShortName(const CString& ref, REF_TYPE *type);

	static bool LoadTextFile(const CString &filename, CString &msg);

	int GetUnifiedDiff(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, CString patchfile, bool bMerge, bool bCombine, int diffContext);
	int GetUnifiedDiff(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, CStringA * buffer, bool bMerge, bool bCombine, int diffContext);

	int GitRevert(int parent, const CGitHash &hash);

	CString CombinePath(const CString &path) const
	{
		if (path.IsEmpty())
			return m_CurrentDir;
		if (m_CurrentDir.IsEmpty())
			return path;
		return m_CurrentDir + (m_CurrentDir.Right(1) == _T("\\") ? _T("") : _T("\\")) + path;
	}

	CString CombinePath(const CTGitPath &path) const
	{
		return CombinePath(path.GetWinPath());
	}

	CString CombinePath(const CTGitPath *path) const
	{
		ATLASSERT(path);
		return CombinePath(path->GetWinPath());
	}
};
extern void GetTempPath(CString &path);
extern CString GetTempFile();
extern DWORD GetTortoiseGitTempPath(DWORD nBufferLength, LPTSTR lpBuffer);

extern CGit g_Git;
