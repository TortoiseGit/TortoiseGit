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

#include "StdAfx.h"
#include "Git.h"
#include "atlconv.h"
#include "GitRev.h"
#include "registry.h"
#include "GitConfig.h"
#include <map>
#include "UnicodeUtils.h"
#include "gitdll.h"
#include <fstream>

int CGit::m_LogEncode=CP_UTF8;
typedef CComCritSecLock<CComCriticalSection> CAutoLocker;

static LPTSTR nextpath(LPCTSTR src, LPTSTR dst, UINT maxlen)
{
	LPCTSTR orgsrc;

	while (*src == _T(';'))
		src++;

	orgsrc = src;

	if (!--maxlen)
		goto nullterm;

	while (*src && *src != _T(';'))
	{
		if (*src != _T('"'))
		{
			*dst++ = *src++;
			if (!--maxlen)
			{
				orgsrc = src;
				goto nullterm;
			}
		}
		else
		{
			src++;
			while (*src && *src != _T('"'))
			{
				*dst++ = *src++;
				if (!--maxlen)
				{
					orgsrc = src;
					goto nullterm;
				}
			}

			if (*src)
				src++;
		}
	}

	while (*src == _T(';'))
		src++;

nullterm:

	*dst = 0;

	return (orgsrc != src) ? (LPTSTR)src : NULL;
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

	TCHAR buf[_MAX_PATH];

	// search in all paths defined in PATH
	while ((env = nextpath(env, buf, _MAX_PATH-1)) && *buf)
	{
		TCHAR *pfin = buf + _tcslen(buf)-1;

		// ensure trailing slash
		if (*pfin != _T('/') && *pfin != _T('\\'))
			_tcscpy(++pfin, _T("\\"));

		const int len = _tcslen(buf);

		if ((len + 7) < _MAX_PATH)
			_tcscpy(pfin+1, _T("git.exe"));
		else
			break;

		if ( FileExists(buf) )
		{
			// dir found
			pfin[1] = 0;
			CGit::ms_LastMsysGitDir = buf;
			return TRUE;
		}
	}

	return FALSE;
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

	LPTSTR pEnv = m_Environment.size()? &m_Environment[0]: NULL;
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
		len= strlen((const char*)p);
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
BOOL CGit::IsInitRepos()
{
	CString cmdout;
	cmdout.Empty();
	if(g_Git.Run(_T("git.exe rev-parse --revs-only HEAD"),&cmdout,CP_UTF8))
	{
	//	CMessageBox::Show(NULL,cmdout,_T("TortoiseGit"),MB_OK);
		return TRUE;
	}
	if(cmdout.IsEmpty())
		return TRUE;

	return FALSE;
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
		size_t oldsize=m_pvector->size();
		m_pvector->resize(m_pvector->size()+size);
		memcpy(&*(m_pvector->begin()+oldsize),data,size);
		return false;
	}
	virtual bool OnOutputErrData(const BYTE* data, size_t size)
	{
		if (!m_pvectorErr)
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
			g_Git.StringAppend(&configValue,(BYTE*)value.GetBuffer(),encoding);
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

		const char *refs_heads_master = git_resolve_ref(CUnicodeUtils::GetUTF8(CString(symbolicRefName)), sha1, 0, &flag);
		if(refs_heads_master && (flag&REF_ISSYMREF))
		{
			g_Git.StringAppend(&refName,(BYTE*)refs_heads_master);
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
		sBranchOut = s + len;
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

	if(from != NULL && to != NULL)
	{
		CString range;
		range.Format(_T(" %s..%s "),FixBranchName(*from),FixBranchName(*to));
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
	dwRetVal = GetTempPath(dwBufSize,		// length of the buffer
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

	dwRetVal = GetTempPath(dwBufSize,		// length of the buffer
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

	LPTSTR pEnv = m_Environment.size()? &m_Environment[0]: NULL;
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

CGitHash CGit::GetHash(TCHAR* friendname)
{
	if(this->m_IsUseGitDLL)
	{
		this->CheckAndInitDll();

		CGitHash hash;
		CStringA ref;
		ref = CUnicodeUtils::GetMulti(FixBranchName(friendname), CP_UTF8);
		try
		{
			git_get_sha1(ref, hash.m_hash);

		}catch(...)
		{
		}
		return hash;

	}
	else
	{
		CString cmd;
		CString out;
		cmd.Format(_T("git.exe rev-parse %s" ),FixBranchName(friendname));
		Run(cmd, &out, NULL, CP_UTF8);
	//	int pos=out.ReverseFind(_T('\n'));
		out.FindOneOf(_T("\r\n"));
		return CGitHash(out);
	}
}

int CGit::GetInitAddList(CTGitPathList &outputlist)
{
	CString cmd;
	BYTE_VECTOR cmdout;

	cmd=_T("git.exe ls-files -s -t -z");
	outputlist.Clear();
	if (g_Git.Run(cmd, &cmdout))
		return -1;

	outputlist.ParserFromLsFile(cmdout);
	for(int i=0;i<outputlist.GetCount();i++)
		((unsigned int)outputlist[i].m_Action) = CTGitPath::LOGACTIONS_ADDED;

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
	if (g_Git.Run(cmd, &out))
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
	int ret;

	if(this->m_IsUseGitDLL)
	{
		return git_for_each_ref_in("refs/tags/",addto_list_each_ref_fn, &list);

	}
	else
	{
		CString cmd, output;
		cmd=_T("git.exe tag -l");
		int i=0;
		ret = g_Git.Run(cmd, &output, NULL, CP_UTF8);
		if(!ret)
		{
			int pos=0;
			CString one;
			while( pos>=0 )
			{
				i++;
				one=output.Tokenize(_T("\n"),pos);
				list.push_back(one);
			}
		}
	}
	return ret;
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
	CString cmd, output;
	cmd = _T("git.exe branch --no-color");

	if((type&BRANCH_ALL) == BRANCH_ALL)
		cmd += _T(" -a");
	else if(type&BRANCH_REMOTE)
		cmd += _T(" -r");

	int i=0;
	ret = g_Git.Run(cmd, &output, NULL, CP_UTF8);
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
				if(current)
					*current=i;
				one = one.Mid(2);
			}
			if (one != _T("(no branch)"))
				list.push_back(one);
			i++;
		}
	}

	if(type & BRANCH_FETCH_HEAD && !DerefFetchHead().IsEmpty())
		list.push_back(L"FETCH_HEAD");

	return ret;
}

int CGit::GetRemoteList(STRING_VECTOR &list)
{
	int ret;
	CString cmd, output;
	cmd=_T("git.exe remote");
	ret = g_Git.Run(cmd, &output, NULL, CP_UTF8);
	if(!ret)
	{
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			one=output.Tokenize(_T("\n"),pos);
			list.push_back(one);
		}
	}
	return ret;
}

int CGit::GetRefList(STRING_VECTOR &list)
{
	int ret;
	if(this->m_IsUseGitDLL)
	{
		return git_for_each_ref_in("",addto_list_each_ref_fn, &list);

	}
	else
	{
		CString cmd, output;
		cmd=_T("git.exe show-ref -d");
		ret = g_Git.Run(cmd, &output, NULL, CP_UTF8);
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
		}
	}
	return ret;
}

int addto_map_each_ref_fn(const char *refname, const unsigned char *sha1, int /*flags*/, void *cb_data)
{
	MAP_HASH_NAME *map = (MAP_HASH_NAME*)cb_data;
	CString str;
	g_Git.StringAppend(&str, (BYTE*)refname, CP_UTF8);
	CGitHash hash((char*)sha1);

	(*map)[hash].push_back(str);

	if(strncmp(refname, "refs/tags", 9) == 0)
	{
		GIT_HASH refhash;
		if(!git_deref_tag(sha1, refhash))
		{
			(*map)[(char*)refhash].push_back(str+_T("^{}"));
		}
	}
	return 0;
}

int CGit::GetMapHashToFriendName(MAP_HASH_NAME &map)
{
	int ret;
	if(this->m_IsUseGitDLL)
	{
		return git_for_each_ref_in("",addto_map_each_ref_fn, &map);

	}
	else
	{
		CString cmd, output;
		cmd=_T("git.exe show-ref -d");
		ret = g_Git.Run(cmd, &output, NULL, CP_UTF8);
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
	}
	return ret;
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
		char charBuf[MAX_PATH];
		TCHAR buf[MAX_PATH];
		strcpy_s(charBuf, MAX_PATH, get_windows_home_directory());
		_tcscpy_s(buf, MAX_PATH, CA2CT(charBuf));
		m_Environment.SetEnv(_T("HOME"), buf);
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
			_tcscpy(ptr + 1, _T("TortoisePlink.exe"));
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
			_tcscpy(ptr + 1, _T("SshAskPass.exe"));
			m_Environment.SetEnv(_T("DISPLAY"),_T(":9999"));
			m_Environment.SetEnv(_T("SSH_ASKPASS"),sAskPass);
			m_Environment.SetEnv(_T("GIT_ASKPASS"),sAskPass);
		}
	}

	// add git/bin path to PATH

	CRegString msysdir=CRegString(REG_MSYSGIT_PATH,_T(""),FALSE);
	str=msysdir;
	if(str.IsEmpty())
	{
		CRegString msysinstalldir=CRegString(REG_MSYSGIT_INSTALL,_T(""),FALSE,HKEY_LOCAL_MACHINE);
		str=msysinstalldir;
		if ( !str.IsEmpty() )
		{
			str += (str[str.GetLength()-1] != '\\') ? "\\bin" : "bin";
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

BOOL CGit::CheckCleanWorkTree()
{
	CString out;
	CString cmd;
	cmd=_T("git.exe rev-parse --verify HEAD");

	if(g_Git.Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git.exe update-index --ignore-submodules --refresh");
	if(g_Git.Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git.exe diff-files --quiet --ignore-submodules");
	if(g_Git.Run(cmd,&out,CP_UTF8))
		return FALSE;

	cmd=_T("git diff-index --cached --quiet HEAD --ignore-submodules");
	if(g_Git.Run(cmd,&out,CP_UTF8))
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
		cmd.Format(_T("git.exe mv -- \"%s\" \"%s\""),path.GetGitPathString(),path.GetGitOldPathString());
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}

		cmd.Format(_T("git.exe checkout %s -f -- \"%s\""), commit, path.GetGitOldPathString());
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}

	}
	else if(path.m_Action & CTGitPath::LOGACTIONS_ADDED)
	{	//To init git repository, there are not HEAD, so we can use git reset command
		cmd.Format(_T("git.exe rm -f --cached -- \"%s\""),path.GetGitPathString());

		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			::MessageBox(NULL, out, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			return -1;
		}
	}
	else
	{
		cmd.Format(_T("git.exe checkout %s -f -- \"%s\""), commit, path.GetGitPathString());
		if (g_Git.Run(cmd, &out, CP_UTF8))
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

	if (g_Git.Run(cmd, &vector))
	{
		return -1;
	}

	list.ParserFromLsFile(vector);

	return 0;
}

bool CGit::IsFastForward(const CString &from, const CString &to)
{
	CString base;
	CGitHash basehash,hash;
	CString cmd, err;
	cmd.Format(_T("git.exe merge-base %s %s"), FixBranchName(to), FixBranchName(from));

	if (g_Git.Run(cmd, &base, &err, CP_UTF8))
	{
		//CMessageBox::Show(NULL, base + _T("\n") + err, _T("TortoiseGit"), MB_OK|MB_ICONERROR);
		return false;
	}
	basehash = base;

	hash=g_Git.GetHash(from);

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
		return g_Git.RunLogFile(cmd,outputfile);
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
	for(int i=0;i<size();i++)
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
