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

#include "stdafx.h"
#include "Git.h"
#include "atlconv.h"
#include "GitRev.h"
#include "registry.h"
#include "GitConfig.h"
#include <map>
#include "UnicodeUtils.h"
#include "gitdll.h"
#include <fstream>
#include "git2.h"

int CGit::m_LogEncode=CP_UTF8;
typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

static LPTSTR nextpath(wchar_t *path, wchar_t *buf, size_t buflen)
{
	wchar_t term, *base = path;

	if (path == NULL || buf == NULL || buflen == 0)
		return NULL;

	term = (*path == L'"') ? *path++ : L';';

	for (buflen--; *path && *path != term && buflen; buflen--)
		*buf++ = *path++;

	*buf = L'\0'; /* reserved a byte via initial subtract */

	while (*path == term || *path == L';')
		path++;

	return (path != base) ? path : NULL;
}

static inline BOOL FileExists(LPCTSTR lpszFileName)
{
	struct _stat st;
	return _tstat(lpszFileName, &st) == 0;
}

static BOOL FindGitPath()
{
	size_t size;
	_tgetenv_s(&size, NULL, 0, _T("PATH"));

	if (!size)
	{
		return FALSE;
	}

	TCHAR *env = (TCHAR*)alloca(size * sizeof(TCHAR));
	_tgetenv_s(&size, env, size, _T("PATH"));

	TCHAR buf[MAX_PATH];

	// search in all paths defined in PATH
	while ((env = nextpath(env, buf, MAX_PATH - 1)) != NULL && *buf)
	{
		TCHAR *pfin = buf + _tcslen(buf)-1;

		// ensure trailing slash
		if (*pfin != _T('/') && *pfin != _T('\\'))
			_tcscpy_s(++pfin, 2, _T("\\")); // we have enough space left, MAX_PATH-1 is used in nextpath above

		const size_t len = _tcslen(buf);

		if ((len + 7) < MAX_PATH)
			_tcscpy_s(pfin + 1, MAX_PATH - len, _T("git.exe"));
		else
			break;

		if ( FileExists(buf) )
		{
			// dir found
			pfin[1] = 0;
			CGit::ms_LastMsysGitDir = buf;
			CGit::ms_LastMsysGitDir.TrimRight(_T("\\"));
			return TRUE;
		}
	}

	return FALSE;
}

static bool g_bSortLogical;
static bool g_bSortLocalBranchesFirst;

static void GetSortOptions()
{
	g_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
	if (g_bSortLogical)
		g_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
	g_bSortLocalBranchesFirst = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoSortLocalBranchesFirst", 0, false, HKEY_CURRENT_USER);
	if (g_bSortLocalBranchesFirst)
		g_bSortLocalBranchesFirst = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoSortLocalBranchesFirst", 0, false, HKEY_LOCAL_MACHINE);
}

static int LogicalComparePredicate(CString &left, CString &right)
{
	if (g_bSortLogical)
		return StrCmpLogicalW(left, right) < 0;
	return StrCmpI(left, right) < 0;
}

static int LogicalCompareBranchesPredicate(CString &left, CString &right)
{
	if (g_bSortLocalBranchesFirst)
	{
		int leftIsRemote = left.Find(_T("remotes/"));
		int rightIsRemote = right.Find(_T("remotes/"));

		if (leftIsRemote == 0 && rightIsRemote < 0)
			return false;
		else if (leftIsRemote < 0 && rightIsRemote == 0)
			return true;
	}
	if (g_bSortLogical)
		return StrCmpLogicalW(left, right) < 0;
	return StrCmpI(left, right) < 0;
}

#define MAX_DIRBUFFER 1000
#define CALL_OUTPUT_READ_CHUNK_SIZE 1024

CString CGit::ms_LastMsysGitDir;
CGit g_Git;


CGit::CGit(void)
{
	GetCurrentDirectory(MAX_DIRBUFFER,m_CurrentDir.GetBuffer(MAX_DIRBUFFER));
	m_CurrentDir.ReleaseBuffer();
	m_IsGitDllInited = false;
	m_GitDiff=0;
	m_GitSimpleListDiff=0;
	m_IsUseGitDLL = !!CRegDWORD(_T("Software\\TortoiseGit\\UsingGitDLL"),1);
	m_IsUseLibGit2 = !!CRegDWORD(_T("Software\\TortoiseGit\\UseLibgit2"), TRUE);
	m_IsUseLibGit2_Mask1 = CRegDWORD(_T("Software\\TortoiseGit\\UseLibgit2_mask1"), 0);

	GetSortOptions();
	this->m_bInitialized =false;
	CheckMsysGitDir();
	m_critGitDllSec.Init();
}

CGit::~CGit(void)
{
	if(this->m_GitDiff)
	{
		git_close_diff(m_GitDiff);
		m_GitDiff=0;
	}
	if(this->m_GitSimpleListDiff)
	{
		git_close_diff(m_GitSimpleListDiff);
		m_GitSimpleListDiff=0;
	}
}

bool CGit::IsBranchNameValid(CString branchname)
{
	if (branchname.IsEmpty())
		return false;

	for(int i=0; i < branchname.GetLength(); i++)
	{
		TCHAR c = branchname.GetAt(i);
		if (c <= ' ' || c == '~' || c == '^' || c == ':' || c == '\\' || c == '?' || c == '[')
			return false;
	}

	if (branchname.Find(L".") == 0 || branchname.Find(L"/.") >= 0 || branchname.Find(L"..") >= 0 || branchname.Find(L"@{") >= 0 || branchname.ReverseFind('*') >= 0)
		return false;

	CString reverseBranchname = branchname.MakeReverse();
	if (branchname.Find(L'.') == 0 || branchname.Find(L'/') == 0 || reverseBranchname.Find(L"kcol.") >= 0)
		return false;

	return true;
}

static char g_Buffer[4096];

int CGit::RunAsync(CString cmd, PROCESS_INFORMATION *piOut, HANDLE *hReadOut, HANDLE *hErrReadOut, CString *StdioFile)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite, hReadErr = NULL, hWriteErr = NULL;
	HANDLE hStdioFile = NULL;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor=NULL;
	sa.bInheritHandle=TRUE;
	if(!CreatePipe(&hRead,&hWrite,&sa,0))
	{
		return TGIT_GIT_ERROR_OPEN_PIP;
	}
	if (hErrReadOut && !CreatePipe(&hReadErr, &hWriteErr, &sa, 0))
		return TGIT_GIT_ERROR_OPEN_PIP;

	if(StdioFile)
	{
		hStdioFile=CreateFile(*StdioFile,GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,
								&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb=sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	if (hErrReadOut)
		si.hStdError = hWriteErr;
	else
		si.hStdError = hWrite;
	if(StdioFile)
		si.hStdOutput=hStdioFile;
	else
		si.hStdOutput=hWrite;

	si.wShowWindow=SW_HIDE;
	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;

	LPTSTR pEnv = (!m_Environment.empty()) ? &m_Environment[0]: NULL;
	DWORD dwFlags = pEnv ? CREATE_UNICODE_ENVIRONMENT : 0;

	//DETACHED_PROCESS make ssh recognize that it has no console to launch askpass to input password.
	dwFlags |= DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP;

	memset(&this->m_CurrentGitPi,0,sizeof(PROCESS_INFORMATION));
	memset(&pi, 0, sizeof(PROCESS_INFORMATION));

	if(cmd.Find(_T("git")) == 0)
	{
		int firstSpace = cmd.Find(_T(" "));
		if (firstSpace > 0)
			cmd = _T('"')+CGit::ms_LastMsysGitDir+_T("\\")+ cmd.Left(firstSpace) + _T('"')+ cmd.Mid(firstSpace);
		else
			cmd=_T('"')+CGit::ms_LastMsysGitDir+_T("\\")+cmd+_T('"');
	}

	if(!CreateProcess(NULL,(LPWSTR)cmd.GetString(), NULL,NULL,TRUE,dwFlags,pEnv,(LPWSTR)m_CurrentDir.GetString(),&si,&pi))
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR)&lpMsgBuf,
						0,NULL);
		return TGIT_GIT_ERROR_CREATE_PROCESS;
	}

	m_CurrentGitPi = pi;

	CloseHandle(hWrite);
	if (hErrReadOut)
		CloseHandle(hWriteErr);
	if(piOut)
		*piOut=pi;
	if(hReadOut)
		*hReadOut=hRead;
	if(hErrReadOut)
		*hErrReadOut = hReadErr;
	return 0;

}
//Must use sperate function to convert ANSI str to union code string
//Becuase A2W use stack as internal convert buffer.
void CGit::StringAppend(CString *str,BYTE *p,int code,int length)
{
	//USES_CONVERSION;
	//str->Append(A2W_CP((LPCSTR)p,code));
	if(str == NULL)
		return ;

	WCHAR * buf;

	int len ;
	if(length<0)
		len = (int)strlen((const char*)p);
	else
		len=length;
	//if (len==0)
	//	return ;
	//buf = new WCHAR[len*4 + 1];
	buf = str->GetBuffer(len*4+1+str->GetLength())+str->GetLength();
	SecureZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(code, 0, (LPCSTR)p, len, buf, len*4);
	str->ReleaseBuffer();
	//str->Append(buf);
	//delete buf;
}

BOOL CGit::IsOrphanBranch(CString ref)
{
	if (ref.IsEmpty())
		ref = _T("HEAD");

	CString cmdout;
	if (Run(_T("git.exe rev-parse --revs-only ") + ref, &cmdout, CP_UTF8))
	{
		return TRUE;
	}
	if(cmdout.IsEmpty())
		return TRUE;

	return FALSE;
}

BOOL CGit::IsInitRepos()
{
	return IsOrphanBranch(_T("HEAD"));
}

DWORD WINAPI CGit::AsyncReadStdErrThread(LPVOID lpParam)
{
	PASYNCREADSTDERRTHREADARGS pDataArray;
	pDataArray = (PASYNCREADSTDERRTHREADARGS)lpParam;

	DWORD readnumber;
	BYTE data[CALL_OUTPUT_READ_CHUNK_SIZE];
	while (ReadFile(pDataArray->fileHandle, data, CALL_OUTPUT_READ_CHUNK_SIZE, &readnumber, NULL))
	{
		if (pDataArray->pcall->OnOutputErrData(data,readnumber))
			break;
	}

	return 0;
}

int CGit::Run(CGitCall* pcall)
{
	PROCESS_INFORMATION pi;
	HANDLE hRead, hReadErr;
	if(RunAsync(pcall->GetCmd(),&pi,&hRead, &hReadErr))
		return TGIT_GIT_ERROR_CREATE_PROCESS;

	HANDLE thread;
	ASYNCREADSTDERRTHREADARGS threadArguments;
	threadArguments.fileHandle = hReadErr;
	threadArguments.pcall = pcall;
	thread = CreateThread(NULL, 0, AsyncReadStdErrThread, &threadArguments, 0, NULL);

	DWORD readnumber;
	BYTE data[CALL_OUTPUT_READ_CHUNK_SIZE];
	bool bAborted=false;
	while(ReadFile(hRead,data,CALL_OUTPUT_READ_CHUNK_SIZE,&readnumber,NULL))
	{
		// TODO: when OnOutputData() returns 'true', abort git-command. Send CTRL-C signal?
		if(!bAborted)//For now, flush output when command aborted.
			if(pcall->OnOutputData(data,readnumber))
				bAborted=true;
	}
	if(!bAborted)
		pcall->OnEnd();

	if (thread)
		WaitForSingleObject(thread, INFINITE);

	CloseHandle(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitcode =0;

	if(!GetExitCodeProcess(pi.hProcess,&exitcode))
	{
		return TGIT_GIT_ERROR_GET_EXIT_CODE;
	}

	CloseHandle(pi.hProcess);

	CloseHandle(hRead);
	CloseHandle(hReadErr);
	return exitcode;
}
class CGitCall_ByteVector : public CGitCall
{
public:
	CGitCall_ByteVector(CString cmd,BYTE_VECTOR* pvector, BYTE_VECTOR* pvectorErr = NULL):CGitCall(cmd),m_pvector(pvector),m_pvectorErr(pvectorErr){}
	virtual bool OnOutputData(const BYTE* data, size_t size)
	{
		if (size == 0)
			return false;
		size_t oldsize=m_pvector->size();
		m_pvector->resize(m_pvector->size()+size);
		memcpy(&*(m_pvector->begin()+oldsize),data,size);
		return false;
	}
	virtual bool OnOutputErrData(const BYTE* data, size_t size)
	{
		if (!m_pvectorErr || size == 0)
			return false;
		size_t oldsize = m_pvectorErr->size();
		m_pvectorErr->resize(m_pvectorErr->size() + size);
		memcpy(&*(m_pvectorErr->begin() + oldsize), data, size);
		return false;
	}
	BYTE_VECTOR* m_pvector;
	BYTE_VECTOR* m_pvectorErr;

};
int CGit::Run(CString cmd,BYTE_VECTOR *vector, BYTE_VECTOR *vectorErr)
{
	CGitCall_ByteVector call(cmd, vector, vectorErr);
	return Run(&call);
}
int CGit::Run(CString cmd, CString* output, int code)
{
	CString err;
	int ret;
	ret = Run(cmd, output, &err, code);

	if (output && !err.IsEmpty())
	{
		if (!output->IsEmpty())
			*output += _T("\n");
		*output += err;
	}

	return ret;
}
int CGit::Run(CString cmd, CString* output, CString* outputErr, int code)
{
	BYTE_VECTOR vector, vectorErr;
	int ret;
	if (outputErr)
		ret = Run(cmd, &vector, &vectorErr);
	else
		ret = Run(cmd, &vector);

	vector.push_back(0);
	StringAppend(output, &(vector[0]), code);

	if (outputErr)
	{
		vectorErr.push_back(0);
		StringAppend(outputErr, &(vectorErr[0]), code);
	}

	return ret;
}

CString CGit::GetUserName(void)
{
	CEnvironment env;
	env.CopyProcessEnvironment();
	CString envname = env.GetEnv(_T("GIT_AUTHOR_NAME"));
	if (!envname.IsEmpty())
		return envname;
	return GetConfigValue(L"user.name", this->GetGitEncode(L"i18n.commitencoding"));
}
CString CGit::GetUserEmail(void)
{
	CEnvironment env;
	env.CopyProcessEnvironment();
	CString envmail = env.GetEnv(_T("GIT_AUTHOR_EMAIL"));
	if (!envmail.IsEmpty())
		return envmail;

	return GetConfigValue(L"user.email");
}

CString CGit::GetConfigValue(CString name,int encoding, CString *GitPath, BOOL RemoveCR)
{
	CString configValue;
	int start = 0;
	if(this->m_IsUseGitDLL)
	{
		CString *git_path=NULL;

		CAutoLocker lock(g_Git.m_critGitDllSec);

		try
		{
			CTGitPath path;

			CheckAndInitDll();
			git_path = GitPath;

		}catch(...)
		{
		}
		CStringA key, value;
		key =  CUnicodeUtils::GetMulti(name, encoding);
		CStringA p;
		if(git_path)
			p = CUnicodeUtils::GetMulti(*GitPath, CP_UTF8);

		if(git_get_config(key.GetBuffer(), value.GetBufferSetLength(4096), 4096, p.GetBuffer()))
			return CString();
		else
		{
			StringAppend(&configValue,(BYTE*)value.GetBuffer(),encoding);
			if(RemoveCR)
				return configValue.Tokenize(_T("\n"),start);
			return configValue;
		}

	}
	else
	{
		CString cmd;
		cmd.Format(L"git.exe config %s", name);
		Run(cmd, &configValue, NULL, encoding);
		if(RemoveCR)
			return configValue.Tokenize(_T("\n"),start);
		return configValue;
	}
}

int CGit::SetConfigValue(CString key, CString value, CONFIG_TYPE type, int encoding, CString *GitPath)
{
	if(this->m_IsUseGitDLL)
	{
		CAutoLocker lock(g_Git.m_critGitDllSec);

		try
		{
			CheckAndInitDll();

		}catch(...)
		{
		}
		CStringA keya, valuea;
		keya = CUnicodeUtils::GetMulti(key, CP_UTF8);
		valuea = CUnicodeUtils::GetMulti(value, encoding);
		CStringA p;
		if(GitPath)
			p = CUnicodeUtils::GetMulti(*GitPath, CP_UTF8);

		return get_set_config(keya.GetBuffer(), valuea.GetBuffer(), type, p.GetBuffer());

	}
	else
	{
		CString cmd;
		CString option;
		switch(type)
		{
		case CONFIG_GLOBAL:
			option = _T("--global");
			break;
		case CONFIG_SYSTEM:
			option = _T("--system");
			break;
		default:
			break;
		}
		cmd.Format(_T("git.exe config %s %s \"%s\""), option, key, value);
		CString out;
		if (Run(cmd, &out, NULL, encoding))
		{
			return -1;
		}
	}
	return 0;
}

int CGit::UnsetConfigValue(CString key, CONFIG_TYPE type, int encoding, CString *GitPath)
{
	if(this->m_IsUseGitDLL)
	{
		CAutoLocker lock(g_Git.m_critGitDllSec);

		try
		{
			CheckAndInitDll();
		}catch(...)
		{
		}
		CStringA keya;
		keya = CUnicodeUtils::GetMulti(key, CP_UTF8);
		CStringA p;
		if(GitPath)
			p=CUnicodeUtils::GetMulti(*GitPath,CP_ACP);

		return get_set_config(keya.GetBuffer(), NULL, type, p.GetBuffer());
	}
	else
	{
		CString cmd;
		CString option;
		switch(type)
		{
		case CONFIG_GLOBAL:
			option = _T("--global");
			break;
		case CONFIG_SYSTEM:
			option = _T("--system");
			break;
		default:
			break;
		}
		cmd.Format(_T("git.exe config %s --unset %s"), option, key);
		CString out;
		if (Run(cmd, &out, NULL, encoding))
		{
			return -1;
		}
	}
	return 0;
}

CString CGit::GetCurrentBranch(void)
{
	CString output;
	//Run(_T("git.exe branch"),&branch);

	if(this->GetCurrentBranchFromFile(this->m_CurrentDir,output))
	{
		return _T("(no branch)");
	}
	else
		return output;

}

CString CGit::GetSymbolicRef(const wchar_t* symbolicRefName, bool bStripRefsHeads)
{
	CString refName;
	if(this->m_IsUseGitDLL)
	{
		unsigned char sha1[20];
		int flag;

		CAutoLocker lock(g_Git.m_critGitDllSec);
		const char *refs_heads_master = git_resolve_ref(CUnicodeUtils::GetUTF8(CString(symbolicRefName)), sha1, 0, &flag);
		if(refs_heads_master && (flag&REF_ISSYMREF))
		{
			StringAppend(&refName,(BYTE*)refs_heads_master);
			if(bStripRefsHeads)
				refName = StripRefName(refName);
		}

	}
	else
	{
		CString cmd;
		cmd.Format(L"git symbolic-ref %s", symbolicRefName);
		if (Run(cmd, &refName, NULL, CP_UTF8) != 0)
			return CString();//Error
		int iStart = 0;
		refName = refName.Tokenize(L"\n", iStart);
		if(bStripRefsHeads)
			refName = StripRefName(refName);
	}
	return refName;
}

CString CGit::GetFullRefName(CString shortRefName)
{
	CString refName;
	CString cmd;
	cmd.Format(L"git rev-parse --symbolic-full-name %s", shortRefName);
	if (Run(cmd, &refName, NULL, CP_UTF8) != 0)
		return CString();//Error
	int iStart = 0;
	return refName.Tokenize(L"\n", iStart);
}

CString CGit::StripRefName(CString refName)
{
	if(wcsncmp(refName, L"refs/heads/", 11) == 0)
		refName = refName.Mid(11);
	else if(wcsncmp(refName, L"refs/", 5) == 0)
		refName = refName.Mid(5);
	int start =0;
	return refName.Tokenize(_T("\n"),start);
}

int CGit::GetCurrentBranchFromFile(const CString &sProjectRoot, CString &sBranchOut)
{
	// read current branch name like git-gui does, by parsing the .git/HEAD file directly

	if ( sProjectRoot.IsEmpty() )
		return -1;

	CString sDotGitPath;
	if (!g_GitAdminDir.GetAdminDirPath(sProjectRoot, sDotGitPath))
		return -1;

	CString sHeadFile = sDotGitPath + _T("HEAD");

	FILE *pFile;
	_tfopen_s(&pFile, sHeadFile.GetString(), _T("r"));

	if (!pFile)
	{
		return -1;
	}

	char s[256] = {0};
	fgets(s, sizeof(s), pFile);

	fclose(pFile);

	const char *pfx = "ref: refs/heads/";
	const int len = 16;//strlen(pfx)

	if ( !strncmp(s, pfx, len) )
	{
		//# We're on a branch.  It might not exist.  But
		//# HEAD looks good enough to be a branch.
		CStringA utf8Branch(s + len);
		sBranchOut = CUnicodeUtils::GetUnicode(utf8Branch);
		sBranchOut.TrimRight(_T(" \r\n\t"));

		if ( sBranchOut.IsEmpty() )
			return -1;
	}
	else
	{
		//# Assume this is a detached head.
		sBranchOut = "HEAD";

		return 1;
	}

	return 0;
}

int CGit::BuildOutputFormat(CString &format,bool IsFull)
{
	CString log;
	log.Format(_T("#<%c>%%x00"),LOG_REV_ITEM_BEGIN);
	format += log;
	if(IsFull)
	{
		log.Format(_T("#<%c>%%an%%x00"),LOG_REV_AUTHOR_NAME);
		format += log;
		log.Format(_T("#<%c>%%ae%%x00"),LOG_REV_AUTHOR_EMAIL);
		format += log;
		log.Format(_T("#<%c>%%ai%%x00"),LOG_REV_AUTHOR_DATE);
		format += log;
		log.Format(_T("#<%c>%%cn%%x00"),LOG_REV_COMMIT_NAME);
		format += log;
		log.Format(_T("#<%c>%%ce%%x00"),LOG_REV_COMMIT_EMAIL);
		format += log;
		log.Format(_T("#<%c>%%ci%%x00"),LOG_REV_COMMIT_DATE);
		format += log;
		log.Format(_T("#<%c>%%b%%x00"),LOG_REV_COMMIT_BODY);
		format += log;
	}

	log.Format(_T("#<%c>%%m%%H%%x00"),LOG_REV_COMMIT_HASH);
	format += log;
	log.Format(_T("#<%c>%%P%%x00"),LOG_REV_COMMIT_PARENT);
	format += log;
	log.Format(_T("#<%c>%%s%%x00"),LOG_REV_COMMIT_SUBJECT);
	format += log;

	if(IsFull)
	{
		log.Format(_T("#<%c>%%x00"),LOG_REV_COMMIT_FILE);
		format += log;
	}
	return 0;
}

int CGit::GetLog(BYTE_VECTOR& logOut, const CString &hash,  CTGitPath *path ,int count,int mask,CString *from,CString *to)
{
	CGitCall_ByteVector gitCall(CString(), &logOut, NULL);
	return GetLog(&gitCall,hash,path,count,mask,from,to);
}

CString CGit::GetLogCmd( const CString &hash, CTGitPath *path, int count, int mask,CString *from,CString *to,bool paramonly,
						CFilterData *Filter)
{
	CString cmd;
	CString num;
	CString since;

	CString file;

	if(path)
		file.Format(_T(" -- \"%s\""),path->GetGitPathString());

	if(count>0)
		num.Format(_T("-n%d"),count);

	CString param;

	if(mask& LOG_INFO_STAT )
		param += _T(" --numstat ");
	if(mask& LOG_INFO_FILESTATE)
		param += _T(" --raw ");

	if(mask& LOG_INFO_FULLHISTORY)
		param += _T(" --full-history ");

	if(mask& LOG_INFO_BOUNDARY)
		param += _T(" --left-right --boundary ");

	if(mask& CGit::LOG_INFO_ALL_BRANCH)
		param += _T(" --all ");

	if(mask& CGit::LOG_INFO_DETECT_COPYRENAME)
		param += _T(" -C ");

	if(mask& CGit::LOG_INFO_DETECT_RENAME )
		param += _T(" -M ");

	if(mask& CGit::LOG_INFO_FIRST_PARENT )
		param += _T(" --first-parent ");

	if(mask& CGit::LOG_INFO_NO_MERGE )
		param += _T(" --no-merges ");

	if(mask& CGit::LOG_INFO_FOLLOW)
		param += _T(" --follow ");

	if(mask& CGit::LOG_INFO_SHOW_MERGEDFILE)
		param += _T(" -c ");

	if(mask& CGit::LOG_INFO_FULL_DIFF)
		param += _T(" --full-diff ");

	if(mask& CGit::LOG_INFO_SIMPILFY_BY_DECORATION)
		param += _T(" --simplify-by-decoration ");

	if(from != NULL && to != NULL)
	{
		CString range;
		range.Format(_T(" %s..%s "),FixBranchName(*from),FixBranchName(*to));
		param += range;
	}
	else if(to != NULL && hash.IsEmpty())
	{
		CString range;
		range.Format(_T(" %s "), FixBranchName(*to));
		param += range;
	}
	param+=hash;

	CString st1,st2;

	if( Filter && (Filter->m_From != -1))
	{
		st1.Format(_T(" --max-age=%I64u "), Filter->m_From);
		param += st1;
	}

	if( Filter && (Filter->m_To != -1))
	{
		st2.Format(_T(" --min-age=%I64u "), Filter->m_To);
		param += st2;
	}

	bool isgrep = false;
	if( Filter && (!Filter->m_Author.IsEmpty()))
	{
		st1.Format(_T(" --author=\"%s\"" ),Filter->m_Author);
		param += st1;
		isgrep = true;
	}

	if( Filter && (!Filter->m_Committer.IsEmpty()))
	{
		st1.Format(_T(" --committer=\"%s\"" ),Filter->m_Author);
		param += st1;
		isgrep = true;
	}

	if( Filter && (!Filter->m_MessageFilter.IsEmpty()))
	{
		st1.Format(_T(" --grep=\"%s\"" ),Filter->m_MessageFilter);
		param += st1;
		isgrep = true;
	}

	if(Filter && isgrep)
	{
		if(!Filter->m_IsRegex)
			param += _T(" --fixed-strings ");

		param += _T(" --regexp-ignore-case --extended-regexp ");
	}

	if (CRegDWORD(_T("Software\\TortoiseGit\\LogTopoOrder"), TRUE))
		param += _T(" --topo-order");

	if(paramonly) //tgit.dll.Git.cpp:setup_revisions() only looks at args[1] and greater.  To account for this, pass a dummy parameter in the 0th place
		cmd.Format(_T("--ignore-this-parameter %s -z %s --parents "), num, param);
	else
	{
		CString log;
		BuildOutputFormat(log,!(mask&CGit::LOG_INFO_ONLY_HASH));
		cmd.Format(_T("git.exe log %s -z %s --parents --pretty=format:\"%s\""),
				num,param,log);
	}

	cmd += file;

	return cmd;
}
//int CGit::GetLog(CGitCall* pgitCall, const CString &hash,  CTGitPath *path ,int count,int mask)
int CGit::GetLog(CGitCall* pgitCall, const CString &hash, CTGitPath *path, int count, int mask,CString *from,CString *to)
{
	pgitCall->SetCmd( GetLogCmd(hash,path,count,mask,from,to) );

	return Run(pgitCall);
//	return Run(cmd,&logOut);
}

#define BUFSIZE 512
void GetTempPath(CString &path)
{
	TCHAR lpPathBuffer[BUFSIZE];
	DWORD dwRetVal;
	DWORD dwBufSize=BUFSIZE;
	dwRetVal = GetTortoiseGitTempPath(dwBufSize,		// length of the buffer
							lpPathBuffer);	// buffer for path
	if (dwRetVal > dwBufSize || (dwRetVal == 0))
	{
		path=_T("");
	}
	path.Format(_T("%s"),lpPathBuffer);
}
CString GetTempFile()
{
	TCHAR lpPathBuffer[BUFSIZE];
	DWORD dwRetVal;
	DWORD dwBufSize=BUFSIZE;
	TCHAR szTempName[BUFSIZE];
	UINT uRetVal;

	dwRetVal = GetTortoiseGitTempPath(dwBufSize,		// length of the buffer
							lpPathBuffer);	// buffer for path
	if (dwRetVal > dwBufSize || (dwRetVal == 0))
	{
		return _T("");
	}
	 // Create a temporary file.
	uRetVal = GetTempFileName(lpPathBuffer,		// directory for tmp files
								TEXT("Patch"),	// temp file name prefix
								0,				// create unique name
								szTempName);	// buffer for name


	if (uRetVal == 0)
	{
		return _T("");
	}

	return CString(szTempName);

}

DWORD GetTortoiseGitTempPath(DWORD nBufferLength, LPTSTR lpBuffer)
{
	DWORD result = ::GetTempPath(nBufferLength, lpBuffer);
	if (result == 0) return 0;
	if (lpBuffer == NULL || (result + 13 > nBufferLength))
	{
		if (lpBuffer)
			lpBuffer[0] = '\0';
		return result + 13;
	}

	_tcscat(lpBuffer, _T("TortoiseGit\\"));
	CreateDirectory(lpBuffer, NULL);

	return result + 13;
}

int CGit::RunLogFile(CString cmd,const CString &filename)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb=sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	SECURITY_ATTRIBUTES   psa={sizeof(psa),NULL,TRUE};;
	psa.bInheritHandle=TRUE;

	HANDLE houtfile=CreateFile(filename,GENERIC_WRITE,FILE_SHARE_READ | FILE_SHARE_WRITE,
			&psa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);

	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdOutput = houtfile;

	LPTSTR pEnv = (!m_Environment.empty()) ? &m_Environment[0]: NULL;
	DWORD dwFlags = pEnv ? CREATE_UNICODE_ENVIRONMENT : 0;

	if(cmd.Find(_T("git")) == 0)
		cmd=CGit::ms_LastMsysGitDir+_T("\\")+cmd;

	if(!CreateProcess(NULL,(LPWSTR)cmd.GetString(), NULL,NULL,TRUE,dwFlags,pEnv,(LPWSTR)m_CurrentDir.GetString(),&si,&pi))
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
						NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						(LPTSTR)&lpMsgBuf,
						0,NULL);
		return TGIT_GIT_ERROR_CREATE_PROCESS;
	}

	WaitForSingleObject(pi.hProcess,INFINITE);

	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(houtfile);
	return TGIT_GIT_SUCCESS;
//	return 0;
}

int CGit::GetHash(CGitHash &hash, TCHAR* friendname)
{
	// no need to parse a ref if it's already a 40-byte hash
	if (CGitHash::IsValidSHA1(friendname))
	{
		CString sHash(friendname);
		hash = CGitHash(sHash);
		return 0;
	}

	if (m_IsUseLibGit2)
	{
		git_repository *repo = NULL;
		CStringA gitdirA = CUnicodeUtils::GetMulti(CTGitPath(m_CurrentDir).GetGitPathString(), CP_UTF8);
		if (git_repository_open(&repo, gitdirA.GetBuffer()))
		{
			gitdirA.ReleaseBuffer();
			return -1;
		}
		gitdirA.ReleaseBuffer();

		int isHeadOrphan = git_repository_head_orphan(repo);
		if (isHeadOrphan != 0)
		{
			git_repository_free(repo);
			hash.Empty();
			if (isHeadOrphan == 1)
				return 0;
			else
				return -1;
		}

		CStringA refnameA = CUnicodeUtils::GetMulti(friendname, CP_UTF8);

		git_object * gitObject = NULL;
		if (git_revparse_single(&gitObject, repo, refnameA.GetBuffer()))
		{
			refnameA.ReleaseBuffer();
			git_repository_free(repo);
			return -1;
		}
		refnameA.ReleaseBuffer();

		const git_oid * oid = git_object_id(gitObject);
		if (oid == NULL)
		{
			git_object_free(gitObject);
			git_repository_free(repo);
			return -1;
		}

		hash = CGitHash((char *)oid->id);

		git_object_free(gitObject); // also frees oid
		git_repository_free(repo);

		return 0;
	}
	else
	{
		CString cmd;
		cmd.Format(_T("git.exe rev-parse %s" ),FixBranchName(friendname));
		int ret = Run(cmd, &gitLastErr, NULL, CP_UTF8);
		hash = CGitHash(gitLastErr.Trim());
		return ret;
	}
}

int CGit::GetInitAddList(CTGitPathList &outputlist)
{
	CString cmd;
	BYTE_VECTOR cmdout;

	cmd=_T("git.exe ls-files -s -t -z");
	outputlist.Clear();
	if (Run(cmd, &cmdout))
		return -1;

	outputlist.ParserFromLsFile(cmdout);
	for(int i=0;i<outputlist.GetCount();i++)
		const_cast<CTGitPath&>(outputlist[i]).m_Action = CTGitPath::LOGACTIONS_ADDED;

	return 0;
}
int CGit::GetCommitDiffList(const CString &rev1,const CString &rev2,CTGitPathList &outputlist)
{
	CString cmd;

	if(rev1 == GIT_REV_ZERO || rev2 == GIT_REV_ZERO)
	{
		//rev1=+_T("");
		if(rev1 == GIT_REV_ZERO)
			cmd.Format(_T("git.exe diff -r --raw -C -M --numstat -z %s"),rev2);
		else
			cmd.Format(_T("git.exe diff -r -R --raw -C -M --numstat -z %s"),rev1);
	}
	else
	{
		cmd.Format(_T("git.exe diff-tree -r --raw -C -M --numstat -z %s %s"),rev2,rev1);
	}

	BYTE_VECTOR out;
	if (Run(cmd, &out))
		return -1;

	outputlist.ParserFromLog(out);

	return 0;
}

int addto_list_each_ref_fn(const char *refname, const unsigned char * /*sha1*/, int /*flags*/, void *cb_data)
{
	STRING_VECTOR *list = (STRING_VECTOR*)cb_data;
	CString str;
	g_Git.StringAppend(&str, (BYTE*)refname, CP_UTF8);
	list->push_back(str);
	return 0;
}

int CGit::GetTagList(STRING_VECTOR &list)
{
	if (this->m_IsUseLibGit2)
	{
		git_repository *repo = NULL;

		CStringA gitdir = CUnicodeUtils::GetMulti(CTGitPath(m_CurrentDir).GetGitPathString(), CP_UTF8);
		if (git_repository_open(&repo, gitdir.GetBuffer()))
		{
			gitdir.ReleaseBuffer();
			return -1;
		}
		gitdir.ReleaseBuffer();

		git_strarray tag_names;

		if (git_tag_list(&tag_names, repo))
		{
			git_repository_free(repo);
			return -1;
		}

		for (size_t i = 0; i < tag_names.count; i++)
		{
			CStringA tagName(tag_names.strings[i]);
			list.push_back(CUnicodeUtils::GetUnicode(tagName));
		}

		git_strarray_free(&tag_names);

		git_repository_free(repo);

		std::sort(list.begin(), list.end(), LogicalComparePredicate);

		return 0;
	}
	else
	{
		CString cmd, output;
		cmd=_T("git.exe tag -l");
		int ret = Run(cmd, &output, NULL, CP_UTF8);
		if(!ret)
		{
			int pos=0;
			CString one;
			while( pos>=0 )
			{
				one=output.Tokenize(_T("\n"),pos);
				if (!one.IsEmpty())
					list.push_back(one);
			}
			std::sort(list.begin(), list.end(), LogicalComparePredicate);
		}
		return ret;
	}
}

CString CGit::GetGitLastErr(CString msg)
{
	if (this->m_IsUseLibGit2)
	{
		const git_error *libgit2err = giterr_last();
		if (libgit2err)
		{
			CString lastError = CUnicodeUtils::GetUnicode(CStringA(libgit2err->message));
			giterr_clear();
			return msg + _T("\nlibgit2 returned: ") + lastError;
		}
		else
		{
			return msg + _T("\nUnknown libgit2 error.");
		}
	}
	else if (gitLastErr.IsEmpty())
		return msg + _T("\nUnknown git.exe error.");
	else
	{
		CString lastError = gitLastErr;
		gitLastErr.Empty();
		return msg + _T("\n") + lastError;
	}
}

CString CGit::FixBranchName_Mod(CString& branchName)
{
	if(branchName == "FETCH_HEAD")
		branchName = DerefFetchHead();
	return branchName;
}

CString	CGit::FixBranchName(const CString& branchName)
{
	CString tempBranchName = branchName;
	FixBranchName_Mod(tempBranchName);
	return tempBranchName;
}

bool CGit::IsBranchTagNameUnique(const CString& name)
{
	CString output;

	CString cmd;
	cmd.Format(_T("git show-ref --tags --heads refs/heads/%s refs/tags/%s"), name, name);
	int ret = Run(cmd, &output, NULL, CP_UTF8);
	if (!ret)
	{
		int i = 0, pos = 0;
		while (pos >= 0)
		{
			if (!output.Tokenize(_T("\n"), pos).IsEmpty())
				i++;
		}
		if (i >= 2)
			return false;
	}

	return true;
}

/*
Checks if a branch or tag with the given name exists
isBranch is true -> branch, tag otherwise
*/
bool CGit::BranchTagExists(const CString& name, bool isBranch /*= true*/)
{
	CString cmd, output;

	cmd = _T("git show-ref ");
	if (isBranch)
		cmd += _T("--heads ");
	else
		cmd += _T("--tags ");

	cmd += _T("refs/heads/") + name;
	cmd += _T(" refs/tags/") + name;

	int ret = Run(cmd, &output, NULL, CP_UTF8);
	if (!ret)
	{
		if (!output.IsEmpty())
			return true;
	}

	return false;
}

CString CGit::DerefFetchHead()
{
	using namespace std;
	CString dotGitPath;
	g_GitAdminDir.GetAdminDirPath(m_CurrentDir, dotGitPath);
	ifstream fetchHeadFile((dotGitPath + L"FETCH_HEAD").GetString(), ios::in | ios::binary);
	int forMergeLineCount = 0;
	string line;
	string hashToReturn;
	while(getline(fetchHeadFile, line))
	{
		//Tokenize this line
		string::size_type prevPos = 0;
		string::size_type pos = line.find('\t');
		if(pos == string::npos)	continue; //invalid line

		string hash = line.substr(0, pos);
		++pos; prevPos = pos; pos = line.find('\t', pos); if(pos == string::npos) continue;

		bool forMerge = pos == prevPos;
		++pos; prevPos = pos; pos = line.size(); if(pos == string::npos) continue;

		string remoteBranch = line.substr(prevPos, pos - prevPos);

		//Process this line
		if(forMerge)
		{
			hashToReturn = hash;
			++forMergeLineCount;
			if(forMergeLineCount > 1)
				return L""; //More then 1 branch to merge found. Octopus merge needed. Cannot pick single ref from FETCH_HEAD
		}
	}

	return CUnicodeUtils::GetUnicode(hashToReturn.c_str());
}

int CGit::GetBranchList(STRING_VECTOR &list,int *current,BRANCH_TYPE type)
{
	int ret;
	CString cmd, output, cur;
	cmd = _T("git.exe branch --no-color");

	if((type&BRANCH_ALL) == BRANCH_ALL)
		cmd += _T(" -a");
	else if(type&BRANCH_REMOTE)
		cmd += _T(" -r");

	ret = Run(cmd, &output, NULL, CP_UTF8);
	if(!ret)
	{
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			one=output.Tokenize(_T("\n"),pos);
			one.Trim(L" \r\n\t");
			if(one.Find(L" -> ") >= 0 || one.IsEmpty())
				continue; // skip something like: refs/origin/HEAD -> refs/origin/master
			if(one[0] == _T('*'))
			{
				one = one.Mid(2);
				cur = one;
			}
			if (one != _T("(no branch)"))
				list.push_back(one);
		}
	}

	if(type & BRANCH_FETCH_HEAD && !DerefFetchHead().IsEmpty())
		list.push_back(L"FETCH_HEAD");

	std::sort(list.begin(), list.end(), LogicalCompareBranchesPredicate);

	if (current && cur != _T("(no branch)"))
	{
		for (unsigned int i = 0; i < list.size(); i++)
		{
			if (list[i] == cur)
			{
				*current = i;
				break;
			}
		}
	}

	return ret;
}

int CGit::GetRemoteList(STRING_VECTOR &list)
{
	if (this->m_IsUseLibGit2)
	{
		git_repository *repo = NULL;

		CStringA gitdir = CUnicodeUtils::GetMulti(CTGitPath(m_CurrentDir).GetGitPathString(), CP_UTF8);
		if (git_repository_open(&repo, gitdir.GetBuffer()))
		{
			gitdir.ReleaseBuffer();
			return -1;
		}
		gitdir.ReleaseBuffer();

		git_strarray remotes;

		if (git_remote_list(&remotes, repo))
		{
			git_repository_free(repo);
			return -1;
		}

		for (size_t i = 0; i < remotes.count; i++)
		{
			CStringA remote(remotes.strings[i]);
			list.push_back(CUnicodeUtils::GetUnicode(remote));
		}

		git_strarray_free(&remotes);

		git_repository_free(repo);

		std::sort(list.begin(), list.end());

		return 0;
	}
	else
	{
		int ret;
		CString cmd, output;
		cmd=_T("git.exe remote");
		ret = Run(cmd, &output, NULL, CP_UTF8);
		if(!ret)
		{
			int pos=0;
			CString one;
			while( pos>=0 )
			{
				one=output.Tokenize(_T("\n"),pos);
				if (!one.IsEmpty())
					list.push_back(one);
			}
		}
		return ret;
	}
}

int CGit::GetRemoteTags(CString remote, STRING_VECTOR &list)
{
	CString cmd, out, err;
	cmd.Format(_T("git.exe ls-remote -t \"%s\""), remote);
	if (Run(cmd, &out, &err, CP_UTF8))
	{
		MessageBox(NULL, err, _T("TortoiseGit"), MB_ICONERROR);
		return -1;
	}

	int pos = 0;
	while (pos >= 0)
	{
		CString one = out.Tokenize(_T("\n"), pos).Mid(51).Trim(); // sha1, tab + refs/tags/
		// dot not include annotated tags twice; this works, because an annotated tag appears twice (one normal tag and one with ^{} at the end)
		if (one.Find(_T("^{}")) >= 1)
			continue;
		if (!one.IsEmpty())
			list.push_back(one);
	}
	std::sort(list.begin(), list.end(), LogicalComparePredicate);
	return 0;
}

int libgit2_addto_list_each_ref_fn(const char *refname, void *payload)
{
	STRING_VECTOR *list = (STRING_VECTOR*)payload;
	CString str;
	g_Git.StringAppend(&str, (BYTE*)refname, CP_UTF8);
	list->push_back(str);
	return 0;
}

int CGit::GetRefList(STRING_VECTOR &list)
{
	if (this->m_IsUseLibGit2)
	{
		git_repository *repo = NULL;

		CStringA gitdir = CUnicodeUtils::GetMulti(CTGitPath(m_CurrentDir).GetGitPathString(), CP_UTF8);
		if (git_repository_open(&repo, gitdir.GetBuffer()))
		{
			gitdir.ReleaseBuffer();
			return -1;
		}
		gitdir.ReleaseBuffer();

		if (git_reference_foreach(repo, GIT_REF_LISTALL, libgit2_addto_list_each_ref_fn, &list))
		{
			git_repository_free(repo);
			return -1;
		}

		git_repository_free(repo);

		std::sort(list.begin(), list.end(), LogicalComparePredicate);

		return 0;
	}
	else
	{
		CString cmd, output;
		cmd=_T("git.exe show-ref -d");
		int ret = Run(cmd, &output, NULL, CP_UTF8);
		if(!ret)
		{
			int pos=0;
			CString one;
			while( pos>=0 )
			{
				one=output.Tokenize(_T("\n"),pos);
				int start=one.Find(_T(" "),0);
				if(start>0)
				{
					CString name;
					name=one.Right(one.GetLength()-start-1);
					list.push_back(name);
				}
			}
			std::sort(list.begin(), list.end(), LogicalComparePredicate);
		}
		return ret;
	}
}

typedef struct map_each_ref_payload {
	git_repository * repo;
	MAP_HASH_NAME * map;
} map_each_ref_payload;

int libgit2_addto_map_each_ref_fn(const char *refname, void *payload)
{
	map_each_ref_payload *payloadContent = (map_each_ref_payload*)payload;

	CString str;
	g_Git.StringAppend(&str, (BYTE*)refname, CP_UTF8);

	git_object * gitObject = NULL;

	do
	{
		if (git_revparse_single(&gitObject, payloadContent->repo, refname))
		{
			break;
		}

		if (git_object_type(gitObject) == GIT_OBJ_TAG)
		{
			str += _T("^{}"); // deref tag
			git_object * derefedTag = NULL;
			if (git_object_peel(&derefedTag, gitObject, GIT_OBJ_ANY))
			{
				break;
			}
			git_object_free(gitObject);
			gitObject = derefedTag;
		}

		const git_oid * oid = git_object_id(gitObject);
		if (oid == NULL)
		{
			break;
		}

		CGitHash hash((char *)oid->id);
		(*payloadContent->map)[hash].push_back(str);
	} while (false);

	if (gitObject)
	{
		git_object_free(gitObject);
		return 0;
	}

	return 1;
}

int CGit::GetMapHashToFriendName(MAP_HASH_NAME &map)
{
	if (this->m_IsUseLibGit2)
	{
		git_repository *repo = NULL;

		CStringA gitdir = CUnicodeUtils::GetMulti(CTGitPath(m_CurrentDir).GetGitPathString(), CP_UTF8);
		if (git_repository_open(&repo, gitdir.GetBuffer()))
		{
			gitdir.ReleaseBuffer();
			return -1;
		}
		gitdir.ReleaseBuffer();

		map_each_ref_payload payloadContent = { repo, &map };

		if (git_reference_foreach(repo, GIT_REF_LISTALL, libgit2_addto_map_each_ref_fn, &payloadContent))
		{
			git_repository_free(repo);
			return -1;
		}

		git_repository_free(repo);

		for (auto it = map.begin(); it != map.end(); ++it)
		{
			std::sort(it->second.begin(), it->second.end());
		}

		return 0;
	}
	else
	{
		CString cmd, output;
		cmd=_T("git.exe show-ref -d");
		int ret = Run(cmd, &output, NULL, CP_UTF8);
		if(!ret)
		{
			int pos=0;
			CString one;
			while( pos>=0 )
			{
				one=output.Tokenize(_T("\n"),pos);
				int start=one.Find(_T(" "),0);
				if(start>0)
				{
					CString name;
					name=one.Right(one.GetLength()-start-1);

					CString hash;
					hash=one.Left(start);

					map[CGitHash(hash)].push_back(name);
				}
			}
		}
		return ret;
	}
}

BOOL CGit::CheckMsysGitDir()
{
	if (m_bInitialized)
	{
		return TRUE;
	}

	this->m_Environment.clear();
	m_Environment.CopyProcessEnvironment();

	TCHAR *oldpath;
	size_t homesize,size;

	// set HOME if not set already
	_tgetenv_s(&homesize, NULL, 0, _T("HOME"));
	if (!homesize)
	{
		CString home = GetHomeDirectory();
		m_Environment.SetEnv(_T("HOME"), home.GetBuffer());
		home.ReleaseBuffer();
	}
	CString str;

	//setup ssh client
	CString sshclient=CRegString(_T("Software\\TortoiseGit\\SSH"));

	if(!sshclient.IsEmpty())
	{
		m_Environment.SetEnv(_T("GIT_SSH"), sshclient.GetBuffer());
		m_Environment.SetEnv(_T("SVN_SSH"), sshclient.GetBuffer());
	}
	else
	{
		TCHAR sPlink[MAX_PATH];
		GetModuleFileName(NULL, sPlink, _countof(sPlink));
		LPTSTR ptr = _tcsrchr(sPlink, _T('\\'));
		if (ptr) {
			_tcscpy_s(ptr + 1, MAX_PATH - (ptr - sPlink + 1), _T("TortoiseGitPLink.exe"));
			m_Environment.SetEnv(_T("GIT_SSH"), sPlink);
			m_Environment.SetEnv(_T("SVN_SSH"), sPlink);
		}
	}

	{
		TCHAR sAskPass[MAX_PATH];
		GetModuleFileName(NULL, sAskPass, _countof(sAskPass));
		LPTSTR ptr = _tcsrchr(sAskPass, _T('\\'));
		if (ptr)
		{
			_tcscpy_s(ptr + 1, MAX_PATH - (ptr - sAskPass + 1), _T("SshAskPass.exe"));
			m_Environment.SetEnv(_T("DISPLAY"),_T(":9999"));
			m_Environment.SetEnv(_T("SSH_ASKPASS"),sAskPass);
			m_Environment.SetEnv(_T("GIT_ASKPASS"),sAskPass);
		}
	}

	// add git/bin path to PATH

	CRegString msysdir=CRegString(REG_MSYSGIT_PATH,_T(""),FALSE);
	str=msysdir;
	if(str.IsEmpty() || !FileExists(str + _T("\\git.exe")))
	{
		CRegString msysinstalldir=CRegString(REG_MSYSGIT_INSTALL,_T(""),FALSE,HKEY_LOCAL_MACHINE);
		str=msysinstalldir;
		str.TrimRight(_T("\\"));
		if ( !str.IsEmpty() )
		{
			str += "\\bin";
			msysdir=str;
			CGit::ms_LastMsysGitDir = str;
			msysdir.write();
		}
		else
		{
			// search PATH if git/bin directory is already present
			if ( FindGitPath() )
			{
				m_bInitialized = TRUE;
				msysdir = CGit::ms_LastMsysGitDir;
				msysdir.write();
				return TRUE;
			}

			return FALSE;
		}
	}
	else
	{
		CGit::ms_LastMsysGitDir = str;
	}

	// check for git.exe existance (maybe it was deinstalled in the meantime)
	if (!FileExists(CGit::ms_LastMsysGitDir + _T("\\git.exe")))
		return FALSE;

	//set path
	_tdupenv_s(&oldpath,&size,_T("PATH"));

	CString path;
	path.Format(_T("%s;%s"),oldpath,str + _T(";")+ (CString)CRegString(REG_MSYSGIT_EXTRA_PATH,_T(""),FALSE));

	m_Environment.SetEnv(_T("PATH"),path.GetBuffer());

	CString str1 = m_Environment.GetEnv(_T("PATH"));

	CString sOldPath = oldpath;
	free(oldpath);

	m_bInitialized = TRUE;
	return true;
}

CString CGit::GetHomeDirectory()
{
	const wchar_t * homeDir = wget_windows_home_directory();
	return CString(homeDir, (int)wcslen(homeDir));
}

CString CGit::GetGitLocalConfig()
{
	CString path;
	g_GitAdminDir.GetAdminDirPath(g_Git.m_CurrentDir, path);
	path += _T("config");
	return path;
}

CString CGit::GetGitGlobalConfig()
{
	return g_Git.GetHomeDirectory() + _T("\\.gitconfig");
}

CString CGit::GetGitGlobalXDGConfigPath()
{
	return g_Git.GetHomeDirectory() + _T("\\.config\\git");
}

CString CGit::GetGitGlobalXDGConfig()
{
	return g_Git.GetGitGlobalXDGConfigPath() + _T("\\config");
}

CString CGit::GetGitSystemConfig()
{
	const wchar_t * systemConfig = wget_msysgit_etc();
	return CString(systemConfig, (int)wcslen(systemConfig));
}

BOOL CGit::CheckCleanWorkTree()
{
	CString out;
	CString cmd;
	cmd=_T("git.exe rev-parse --verify HEAD");

	if(Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git.exe update-index --ignore-submodules --refresh");
	if(Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git.exe diff-files --quiet --ignore-submodules");
	if(Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git diff-index --cached --quiet HEAD --ignore-submodules");
	if(Run(cmd,&out,CP_UTF8))
		return FALSE;

	return TRUE;
}
int CGit::Revert(CString commit, CTGitPathList &list, bool)
{
	int ret;
	for(int i=0;i<list.GetCount();i++)
	{
		ret = Revert(commit, (CTGitPath&)list[i]);
		if(ret)
			return ret;
	}
	return 0;
}
int CGit::Revert(CString commit, CTGitPath &path)
{
	CString cmd, out;

	if(path.m_Action & CTGitPath::LOGACTIONS_REPLACED && !path.GetGitOldPathString().IsEmpty())
	{
		if (CTGitPath(path.GetGitOldPathString()).IsDirectory())
		{
			CString err;
			err.Format(_T("Cannot revert renaming of \"%s\". A directory with the old name \"%s\" exists."), path.GetGitPathString(), path.GetGitOldPathString());
			::MessageBox(NULL, err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}
		CString force;
		// if the filenames only differ in case, we have to pass "-f"
		if (path.GetGitPathString().CompareNoCase(path.GetGitOldPathString()) == 0)
			force = _T("-f ");
		cmd.Format(_T("git.exe mv %s-- \"%s\" \"%s\""), force, path.GetGitPathString(), path.GetGitOldPathString());
		if (Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}

		cmd.Format(_T("git.exe checkout %s -f -- \"%s\""), commit, path.GetGitOldPathString());
		if (Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}

	}
	else if(path.m_Action & CTGitPath::LOGACTIONS_ADDED)
	{	//To init git repository, there are not HEAD, so we can use git reset command
		cmd.Format(_T("git.exe rm -f --cached -- \"%s\""),path.GetGitPathString());

		if (Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}
	}
	else
	{
		cmd.Format(_T("git.exe checkout %s -f -- \"%s\""), commit, path.GetGitPathString());
		if (Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}
	}

	if (path.m_Action & CTGitPath::LOGACTIONS_DELETED)
	{
		cmd.Format(_T("git.exe add -f -- \"%s\""), path.GetGitPathString());
		if (Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}
	}

	return 0;
}

int CGit::ListConflictFile(CTGitPathList &list,CTGitPath *path)
{
	BYTE_VECTOR vector;

	CString cmd;
	if(path)
		cmd.Format(_T("git.exe ls-files -u -t -z -- \"%s\""),path->GetGitPathString());
	else
		cmd=_T("git.exe ls-files -u -t -z");

	if (Run(cmd, &vector))
	{
		return -1;
	}

	list.ParserFromLsFile(vector);

	return 0;
}

bool CGit::IsFastForward(const CString &from, const CString &to, CGitHash * commonAncestor)
{
	CString base;
	CGitHash basehash,hash;
	CString cmd, err;
	cmd.Format(_T("git.exe merge-base %s %s"), FixBranchName(to), FixBranchName(from));

	if (Run(cmd, &base, &err, CP_UTF8))
	{
		return false;
	}
	basehash = base.Trim();

	GetHash(hash, from);

	if (commonAncestor)
		*commonAncestor = basehash;

	return hash == basehash;
}

unsigned int CGit::Hash2int(CGitHash &hash)
{
	int ret=0;
	for(int i=0;i<4;i++)
	{
		ret = ret << 8;
		ret |= hash.m_hash[i];
	}
	return ret;
}

int CGit::RefreshGitIndex()
{
	if(g_Git.m_IsUseGitDLL)
	{
		CAutoLocker lock(g_Git.m_critGitDllSec);
		try
		{
			return git_run_cmd("update-index","update-index -q --refresh");

		}catch(...)
		{
			return -1;
		}

	}
	else
	{
		CString cmd,output;
		cmd=_T("git.exe update-index --refresh");
		return Run(cmd, &output, CP_UTF8);
	}
}

int CGit::GetOneFile(CString Refname, CTGitPath &path, const CString &outputfile)
{
	if(g_Git.m_IsUseGitDLL)
	{
		CAutoLocker lock(g_Git.m_critGitDllSec);
		try
		{
			g_Git.CheckAndInitDll();
			CStringA ref, patha, outa;
			ref = CUnicodeUtils::GetMulti(Refname, CP_UTF8);
			patha = CUnicodeUtils::GetMulti(path.GetGitPathString(), CP_UTF8);
			outa = CUnicodeUtils::GetMulti(outputfile, CP_UTF8);
			::DeleteFile(outputfile);
			return git_checkout_file((const char*)ref.GetBuffer(),(const char*)patha.GetBuffer(),(const char*)outa.GetBuffer());

		}catch(...)
		{
			return -1;
		}
	}
	else
	{
		CString cmd;
		cmd.Format(_T("git.exe cat-file -p %s:\"%s\""), Refname, path.GetGitPathString());
		return RunLogFile(cmd,outputfile);
	}
}
void CEnvironment::CopyProcessEnvironment()
{
	TCHAR *p = GetEnvironmentStrings();
	while(*p !=0 || *(p+1) !=0)
		this->push_back(*p++);

	push_back(_T('\0'));
	push_back(_T('\0'));
}

CString CEnvironment::GetEnv(TCHAR *name)
{
	CString str;
	for (size_t i = 0; i < size(); i++)
	{
		str = &(*this)[i];
		int start =0;
		CString sname = str.Tokenize(_T("="),start);
		if(sname.CompareNoCase(name) == 0)
		{
			return &(*this)[i+start];
		}
		i+=str.GetLength();
	}
	return _T("");
}

void CEnvironment::SetEnv(TCHAR *name, TCHAR* value)
{
	unsigned int i;
	for( i=0;i<size();i++)
	{
		CString str = &(*this)[i];
		int start =0;
		CString sname = str.Tokenize(_T("="),start);
		if(sname.CompareNoCase(name) == 0)
		{
			break;
		}
		i+=str.GetLength();
	}

	if(i == size())
	{
		i -= 1; // roll back terminate \0\0
		this->push_back(_T('\0'));
	}

	CEnvironment::iterator it;
	it=this->begin();
	it += i;

	while(*it && i<size())
	{
		this->erase(it);
		it=this->begin();
		it += i;
	}

	while(*name)
	{
		this->insert(it,*name++);
		i++;
		it= begin()+i;
	}

	this->insert(it, _T('='));
	i++;
	it= begin()+i;

	while(*value)
	{
		this->insert(it,*value++);
		i++;
		it= begin()+i;
	}

}

int CGit::GetGitEncode(TCHAR* configkey)
{
	CString str=GetConfigValue(configkey);

	if(str.IsEmpty())
		return CP_UTF8;

	return CUnicodeUtils::GetCPCode(str);
}


int CGit::GetDiffPath(CTGitPathList *PathList, CGitHash *hash1, CGitHash *hash2, char *arg)
{
	GIT_FILE file=0;
	int ret=0;
	GIT_DIFF diff=0;

	CAutoLocker lock(g_Git.m_critGitDllSec);

	if(arg == NULL)
		diff = GetGitDiff();
	else
		git_open_diff(&diff, arg);

	if(diff ==NULL)
		return -1;

	bool isStat = 0;
	if(arg == NULL)
		isStat = true;
	else
		isStat = !!strstr(arg, "stat");

	int count=0;

	if(hash2 == NULL)
		ret = git_root_diff(diff, hash1->m_hash, &file, &count,isStat);
	else
		ret = git_diff(diff,hash2->m_hash,hash1->m_hash,&file,&count,isStat);

	if(ret)
		return -1;

	CTGitPath path;
	CString strnewname;
	CString stroldname;

	for(int j=0;j<count;j++)
	{
		path.Reset();
		char *newname;
		char *oldname;

		strnewname.Empty();
		stroldname.Empty();

		int mode=0,IsBin=0,inc=0,dec=0;
		git_get_diff_file(diff,file,j,&newname,&oldname,
					&mode,&IsBin,&inc,&dec);

		StringAppend(&strnewname, (BYTE*)newname, CP_UTF8);
		StringAppend(&stroldname, (BYTE*)oldname, CP_UTF8);

		path.SetFromGit(strnewname,&stroldname);
		path.ParserAction((BYTE)mode);

		if(IsBin)
		{
			path.m_StatAdd=_T("-");
			path.m_StatDel=_T("-");
		}
		else
		{
			path.m_StatAdd.Format(_T("%d"),inc);
			path.m_StatDel.Format(_T("%d"),dec);
		}
		PathList->AddPath(path);
	}
	git_diff_flush(diff);

	if(arg)
		git_close_diff(diff);

	return 0;
}

int CGit::GetShortHASHLength()
{
	return 7;
}

CString CGit::GetShortName(CString ref, REF_TYPE *out_type)
{
	CString str=ref;
	CString shortname;
	REF_TYPE type = CGit::UNKNOWN;

	if (CGit::GetShortName(str, shortname, _T("refs/heads/")))
	{
		type = CGit::LOCAL_BRANCH;

	}
	else if (CGit::GetShortName(str, shortname, _T("refs/remotes/")))
	{
		type = CGit::REMOTE_BRANCH;
	}
	else if (CGit::GetShortName(str, shortname, _T("refs/tags/")))
	{
		type = CGit::TAG;
	}
	else if (CGit::GetShortName(str, shortname, _T("refs/stash")))
	{
		type = CGit::STASH;
		shortname=_T("stash");
	}
	else if (CGit::GetShortName(str, shortname, _T("refs/bisect/")))
	{
		if (shortname.Find(_T("good")) == 0)
		{
			type = CGit::BISECT_GOOD;
			shortname = _T("good");
		}

		if (shortname.Find(_T("bad")) == 0)
		{
			type = CGit::BISECT_BAD;
			shortname = _T("bad");
		}
	}
	else if (CGit::GetShortName(str, shortname, _T("refs/notes/")))
	{
		type = CGit::NOTES;
	}
	else
	{
		type = CGit::UNKNOWN;
		shortname = _T("Unknown");
	}

	if(out_type)
		*out_type = type;

	return shortname;
}

CString CGit::GetUnifiedDiffCmd(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, bool bMerge, bool bCombine)
{
	CString cmd;
	CString output;
/*
	if(!Patchfile.IsEmpty())
	{
		output.Format(_T("--output=\"%s\""), Patchfile);
	}
*/
	if(rev2 == GitRev::GetWorkingCopy())
	{
		cmd.Format(_T("git.exe diff --no-color --stat -p %s %s "), output, rev1);
	}
	else if (rev1 == GitRev::GetWorkingCopy())
	{
		cmd.Format(_T("git.exe diff --no-color -R --stat -p %s %s "), output, rev2);
	}
	else
	{
		CString merge;
		if(bMerge)
				merge += _T("-m ");

		if(bCombine)
				merge += _T("-c ");

		cmd.Format(_T("git.exe diff-tree -r -p %s --stat %s %s"),merge, rev1,rev2);
	}

	if( !path.IsEmpty() )
	{
		cmd += _T(" -- \"");
		cmd += path.GetGitPathString();
		cmd += _T("\" ");
	}

	return cmd;
}

static int UnifiedDiffToFile(const git_diff_delta *delta,
							 const git_diff_range *range,
							 char line_origin,
							 const char *content,
							 size_t content_len,
							 void *payload)
{
	CFile *file = (CFile*)payload;
	file->Write(content, content_len);
	return 0;
}

static int resolve_to_tree(
	git_repository *repo, const char *identifier, git_tree **tree)
{
	int err = 0;
	size_t len = strlen(identifier);
	git_oid oid;
	git_object *obj = NULL;

	/* try to resolve as OID */
	if (git_oid_fromstrn(&oid, identifier, len) == 0)
		git_object_lookup_prefix(&obj, repo, &oid, len, GIT_OBJ_ANY);

	/* try to resolve as reference */
	if (obj == NULL) {
		git_reference *ref, *resolved;
		if (git_reference_lookup(&ref, repo, identifier) == 0) {
			git_reference_resolve(&resolved, ref);
			git_reference_free(ref);
			if (resolved) {
				git_object_lookup(&obj, repo, git_reference_target(resolved), GIT_OBJ_ANY);
				git_reference_free(resolved);
			}
		}
	}

	if (obj == NULL)
		return GIT_ENOTFOUND;

	switch (git_object_type(obj)) {
	case GIT_OBJ_TREE:
		*tree = (git_tree *)obj;
		break;
	case GIT_OBJ_COMMIT:
		err = git_commit_tree(tree, (git_commit *)obj);
		git_object_free(obj);
		break;
	default:
		err = GIT_ENOTFOUND;
	}

	return err;
}

/* use libgit2 get unified diff */
static int GetUnifiedDiffLibGit2(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, git_diff_data_cb callback, void *data, bool bMerge)
{
	git_repository *repo = NULL;
	CStringA gitdirA = CUnicodeUtils::GetMulti(CTGitPath(g_Git.m_CurrentDir).GetGitPathString(), CP_UTF8);
	CStringA tree1 = CUnicodeUtils::GetMulti(rev1, CP_UTF8);
	CStringA tree2 = CUnicodeUtils::GetMulti(rev2, CP_UTF8);
	int ret = 0;

	if (git_repository_open(&repo, gitdirA.GetBuffer()))
	{
		gitdirA.ReleaseBuffer();
		return -1;
	}
	gitdirA.ReleaseBuffer();

	int isHeadOrphan = git_repository_head_orphan(repo);
	if (isHeadOrphan != 0)
	{
		git_repository_free(repo);
		if (isHeadOrphan == 1)
			return 0;
		else
			return -1;
	}

	git_diff_options opts = GIT_DIFF_OPTIONS_INIT;
	git_diff_list *diff=NULL;

	if(rev1 == GitRev::GetWorkingCopy() || rev2 == GitRev::GetWorkingCopy())
	{
		//opts.flags |= GIT_DIFF_IGNORE_WHITESPACE_EOL;

		if (rev1 == GitRev::GetWorkingCopy())
		{
			//opts.flags |= GIT_DIFF_REVERSE;
		}
		git_tree *t1 = NULL;
		git_diff_list *diff2 = NULL;

		do
		{
			if (rev1 !=  GitRev::GetWorkingCopy())
				if (resolve_to_tree(repo, tree1, &t1))
				{
					ret = -1;
					break;
				}
			if (rev2 != GitRev::GetWorkingCopy())
				if (resolve_to_tree(repo, tree2, &t1))
				{
					ret = -1;
					break;
				}

			ret = git_diff_tree_to_index(&diff, repo, t1, NULL, &opts);
			if (ret) 
				break;

			ret =  git_diff_index_to_workdir(&diff2, repo, NULL, &opts);
			if (ret) 
				break;

			ret = git_diff_merge(diff, diff2);
			if (ret) 
				break;

			ret = git_diff_print_patch(diff, callback, data);
			if (ret) 
				break;
			git_diff_list_free(diff);
			git_tree_free(t1);

		}while(0);
	}else
	{
		git_tree *t1, *t2;
		t1 = t2 = NULL;
		do
		{
			if (resolve_to_tree(repo, tree1, &t1))
			{
				ret = -1;
				break;
			}
			if (resolve_to_tree(repo, tree2, &t2))
			{
				ret = -1;
				break;
			}			
			if (git_diff_tree_to_tree(&diff, repo, t1, t2, &opts))
			{
				ret = -1;
				break;
			}
			if(git_diff_print_patch(diff, callback, data))
			{
				ret = -1;
				break;
			}
		}while(0);

		git_diff_list_free(diff);
		git_tree_free(t1);
		git_tree_free(t2);
	
	}
	git_repository_free(repo);

	return ret;
}

int CGit::GetUnifiedDiff(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, CString patchfile, bool bMerge, bool bCombine)
{

	/* The submodule of LibGit2 output is little difference with git core
	*/
	if (UsingLibGit2(GIT_CMD_DIFF))
	{
		CFile file;
		if(!file.Open(patchfile, CFile::modeCreate|CFile::modeWrite))
			return -1;
		int ret = GetUnifiedDiffLibGit2(path, rev1, rev2, UnifiedDiffToFile, &file, bMerge);
		file.Close();
		return ret;
	}
/*	//git will launch git status command to get submodule status.
	else if (m_IsUseGitDLL)
	{
		CStringA cmd = CUnicodeUtils::GetMulti(GetUnifiedDiffCmd(path, rev1, rev2, bMerge, patchfile), CP_UTF8);
		CStringA arg;
		arg = cmd = cmd.Mid(13); // cut "git.exe diff"
		if (cmd.Find("-tree") == 0)
		{
			cmd = "diff-tree";
			arg = arg.Mid(6); // cut "-tree ";
		}
		else
		{
			cmd = "diff";
		}
		return git_run_cmd(cmd.GetBuffer(), arg.GetBuffer());
	}
	*/
	else
	{
		CString cmd;
		cmd = GetUnifiedDiffCmd(path, rev1, rev2, bMerge, bCombine);
		return g_Git.RunLogFile(cmd, patchfile);
	}
}

static int UnifiedDiffToStringA(const git_diff_delta *delta,
								const git_diff_range *range,
								char line_origin,
								const char *content,
								size_t content_len,
								void *payload)
{
	CStringA *str = (CStringA*) payload;
	str->Append(content, content_len);
	return 0;
}
int CGit::GetUnifiedDiff(const CTGitPath& path, const git_revnum_t& rev1, const git_revnum_t& rev2, CStringA * buffer, bool bMerge, bool bCombine)
{
	if (m_IsUseLibGit2)
	{
		return GetUnifiedDiffLibGit2(path, rev1, rev2, UnifiedDiffToStringA, buffer, bMerge);
	}else
	{
		CString cmd;
		cmd = GetUnifiedDiffCmd(path, rev1, rev2, bMerge, bCombine);
		BYTE_VECTOR vector;
		int ret = Run(cmd, &vector);
		if(!vector.empty())
			buffer->Append((char *)&vector[0]);
		return ret;
	}
}

BOOL CGit::UsingLibGit2(int cmd)
{
	if (cmd>=0 && cmd<32)
		return !!((1<<cmd) & m_IsUseLibGit2_Mask1);
 }