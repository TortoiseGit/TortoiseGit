#include "StdAfx.h"
#include "Git.h"
#include "atlconv.h"
#include "GitRev.h"
#include "registry.h"
#include "GitConfig.h"


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
			return TRUE;
		}
	}

	return FALSE;
}


#define MAX_DIRBUFFER 1000
CString CGit::ms_LastMsysGitDir;
CGit g_Git;
CGit::CGit(void)
{
	GetCurrentDirectory(MAX_DIRBUFFER,m_CurrentDir.GetBuffer(MAX_DIRBUFFER));

	// make sure git/bin is in PATH before wingit.dll gets (delay) loaded by wgInit()
	if ( !CheckMsysGitDir() )
	{
		// TODO
	}

	if ( !wgInit() )
	{
		// TODO
	}
}

CGit::~CGit(void)
{
}

static char g_Buffer[4096];

int CGit::RunAsync(CString cmd,PROCESS_INFORMATION *piOut,HANDLE *hReadOut,CString *StdioFile)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;
	HANDLE hStdioFile = NULL;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor=NULL;
	sa.bInheritHandle=TRUE;
	if(!CreatePipe(&hRead,&hWrite,&sa,0))
	{
		return GIT_ERROR_OPEN_PIP;
	}
	
	if(StdioFile)
	{
		hStdioFile=CreateFile(*StdioFile,GENERIC_WRITE,FILE_SHARE_READ   |   FILE_SHARE_WRITE,   
			&sa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);  
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb=sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	si.hStdError=hWrite;
	if(StdioFile)
		si.hStdOutput=hStdioFile;
	else
		si.hStdOutput=hWrite;

	si.wShowWindow=SW_HIDE;
	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;

	if(!CreateProcess(NULL,(LPWSTR)cmd.GetString(), NULL,NULL,TRUE,NULL,NULL,(LPWSTR)m_CurrentDir.GetString(),&si,&pi))
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,NULL);
		return GIT_ERROR_CREATE_PROCESS;
	}
	
	CloseHandle(hWrite);
	if(piOut)
		*piOut=pi;
	if(hReadOut)
		*hReadOut=hRead;
	
	return 0;

}
//Must use sperate function to convert ANSI str to union code string
//Becuase A2W use stack as internal convert buffer. 
void CGit::StringAppend(CString *str,BYTE *p,int code,int length)
{
     //USES_CONVERSION;
	 //str->Append(A2W_CP((LPCSTR)p,code));
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
int CGit::Run(CString cmd,BYTE_VECTOR *vector)
{
	PROCESS_INFORMATION pi;
	HANDLE hRead;
	if(RunAsync(cmd,&pi,&hRead))
		return GIT_ERROR_CREATE_PROCESS;

	DWORD readnumber;
	BYTE data;
	while(ReadFile(hRead,&data,1,&readnumber,NULL))
	{
		//g_Buffer[readnumber]=0;
		vector->push_back(data);
//		StringAppend(output,g_Buffer,codes);
	}

	
	CloseHandle(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD exitcode =0;

	if(!GetExitCodeProcess(pi.hProcess,&exitcode))
	{
		return GIT_ERROR_GET_EXIT_CODE;
	}

	CloseHandle(pi.hProcess);

	CloseHandle(hRead);
	return exitcode;

}
int CGit::Run(CString cmd, CString* output,int code)
{
	BYTE_VECTOR vector;
	int ret;
	ret=Run(cmd,&vector);

	if(ret)
		return ret;
	
	vector.push_back(0);
	
	StringAppend(output,&(vector[0]),code);
	return 0;
}

CString CGit::GetUserName(void)
{
	CString UserName;
	Run(_T("git.exe config user.name"),&UserName,CP_UTF8);
	return UserName;
}
CString CGit::GetUserEmail(void)
{
	CString UserName;
	Run(_T("git.exe config user.email"),&UserName,CP_UTF8);
	return UserName;
}

CString CGit::GetCurrentBranch(void)
{
	CString output;
	//Run(_T("git.exe branch"),&branch);

	int ret=g_Git.Run(_T("git.exe branch"),&output,CP_UTF8);
	if(!ret)
	{		
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			//i++;
			one=output.Tokenize(_T("\n"),pos);
			//list.push_back(one.Right(one.GetLength()-2));
			if(one[0] == _T('*'))
				return one.Right(one.GetLength()-2);
		}
	}
	return CString("");
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
		log.Format(_T("#<%c>%%s%%x00"),LOG_REV_COMMIT_SUBJECT);
		format += log;
		log.Format(_T("#<%c>%%b%%x00"),LOG_REV_COMMIT_BODY);
		format += log;
	}
	log.Format(_T("#<%c>%%m%%H%%x00"),LOG_REV_COMMIT_HASH);
	format += log;
	log.Format(_T("#<%c>%%P%%x00"),LOG_REV_COMMIT_PARENT);
	format += log;

	if(IsFull)
	{
		log.Format(_T("#<%c>%%x00"),LOG_REV_COMMIT_FILE);
		format += log;
	}
	return 0;
}

int CGit::GetLog(BYTE_VECTOR& logOut, CString &hash,  CTGitPath *path ,int count,int mask)
{

	CString cmd;
	CString log;
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

	param+=hash;

	cmd.Format(_T("git.exe log %s -z --topo-order --parents %s --pretty=format:\""),
				num,param);

	BuildOutputFormat(log,!(mask&CGit::LOG_INFO_ONLY_HASH));

	cmd += log;
	cmd += CString(_T("\"  "))+hash+file;

	return Run(cmd,&logOut);
}

#if 0
int CGit::GetShortLog(CString &logOut,CTGitPath * path, int count)
{
	CString cmd;
	CString log;
	int n;
	if(count<0)
		n=100;
	else
		n=count;
	cmd.Format(_T("git.exe log --left-right --boundary --topo-order -n%d --pretty=format:\""),n);
	BuildOutputFormat(log,false);
	cmd += log+_T("\"");
	if (path)
		cmd+= _T("  -- \"")+path->GetGitPathString()+_T("\"");
	//cmd += CString(_T("\" HEAD~40..HEAD"));
	return Run(cmd,&logOut);
}
#endif

#define BUFSIZE 512
void GetTempPath(CString &path)
{
	TCHAR lpPathBuffer[BUFSIZE];
	DWORD dwRetVal;
	DWORD dwBufSize=BUFSIZE;
	dwRetVal = GetTempPath(dwBufSize,     // length of the buffer
                           lpPathBuffer); // buffer for path 
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

	dwRetVal = GetTempPath(dwBufSize,     // length of the buffer
                           lpPathBuffer); // buffer for path 
    if (dwRetVal > dwBufSize || (dwRetVal == 0))
    {
        return _T("");
    }
	 // Create a temporary file. 
    uRetVal = GetTempFileName(lpPathBuffer, // directory for tmp files
                              TEXT("Patch"),  // temp file name prefix 
                              0,            // create unique name 
                              szTempName);  // buffer for name 


    if (uRetVal == 0)
    {
        return _T("");
    }

	return CString(szTempName);

}

int CGit::RunLogFile(CString cmd,CString &filename)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb=sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	SECURITY_ATTRIBUTES   psa={sizeof(psa),NULL,TRUE};;   
	psa.bInheritHandle=TRUE;   
    
	HANDLE   houtfile=CreateFile(filename,GENERIC_WRITE,FILE_SHARE_READ   |   FILE_SHARE_WRITE,   
			&psa,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);   


	si.wShowWindow=SW_HIDE;
	si.dwFlags=STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW;
	si.hStdOutput   =   houtfile; 
	
	if(!CreateProcess(NULL,(LPWSTR)cmd.GetString(), NULL,NULL,TRUE,NULL,NULL,(LPWSTR)m_CurrentDir.GetString(),&si,&pi))
	{
		LPVOID lpMsgBuf;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,GetLastError(),MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&lpMsgBuf,
			0,NULL);
		return GIT_ERROR_CREATE_PROCESS;
	}
	
	WaitForSingleObject(pi.hProcess,INFINITE);   
  	
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(houtfile);
	return GIT_SUCCESS;
	return 0;
}

git_revnum_t CGit::GetHash(CString &friendname)
{
	// NOTE: could replace this with wgGetRevisionID call

	CString cmd;
	CString out;
	cmd.Format(_T("git.exe rev-parse %s" ),friendname);
	Run(cmd,&out,CP_UTF8);
	int pos=out.ReverseFind(_T('\n'));
	if(pos>0)
		return out.Left(pos);
	return out;
}

int CGit::GetTagList(STRING_VECTOR &list)
{
	int ret;
	CString cmd,output;
	cmd=_T("git.exe tag -l");
	int i=0;
	ret=g_Git.Run(cmd,&output,CP_UTF8);
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
	return ret;
}

int CGit::GetBranchList(STRING_VECTOR &list,int *current,BRANCH_TYPE type)
{
	int ret;
	CString cmd,output;
	cmd=_T("git.exe branch");

	if(type==(BRANCH_LOCAL|BRANCH_REMOTE))
		cmd+=_T(" -a");
	else if(type==BRANCH_REMOTE)
		cmd+=_T(" -r");

	int i=0;
	ret=g_Git.Run(cmd,&output,CP_UTF8);
	if(!ret)
	{		
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			i++;
			one=output.Tokenize(_T("\n"),pos);
			list.push_back(one.Right(one.GetLength()-2));
			if(one[0] == _T('*'))
				if(current)
					*current=i;
		}
	}
	return ret;
}

int CGit::GetRemoteList(STRING_VECTOR &list)
{
	int ret;
	CString cmd,output;
	cmd=_T("git.exe config  --get-regexp remote.*.url");
	ret=g_Git.Run(cmd,&output,CP_UTF8);
	if(!ret)
	{
		int pos=0;
		CString one;
		while( pos>=0 )
		{
			one=output.Tokenize(_T("\n"),pos);
			int start=one.Find(_T("."),0);
			if(start>0)
			{
				CString url;
				url=one.Right(one.GetLength()-start-1);
				one=url;
				one=one.Left(one.Find(_T("."),0));
				list.push_back(one);
			}
		}
	}
	return ret;
}

int CGit::GetMapHashToFriendName(MAP_HASH_NAME &map)
{
	int ret;
	CString cmd,output;
	cmd=_T("git show-ref -d");
	ret=g_Git.Run(cmd,&output,CP_UTF8);
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

				map[hash].push_back(name);
			}
		}
	}
	return ret;
}

BOOL CGit::CheckMsysGitDir()
{
	static BOOL bInitialized = FALSE;

	if (bInitialized)
	{
		return TRUE;
	}

	TCHAR *oldpath,*home;
	size_t size;

	// set HOME if not set already
	_tgetenv_s(&size, NULL, 0, _T("HOME"));
	if (!size)
	{
		_tdupenv_s(&home,&size,_T("USERPROFILE")); 
		_tputenv_s(_T("HOME"),home);
		free(home);
	}

	// search PATH if git/bin directory is alredy present
	if ( FindGitPath() )
	{
		bInitialized = TRUE;
		return TRUE;
	}

	// add git/bin path to PATH

	CRegString msysdir=CRegString(REG_MSYSGIT_PATH,_T(""),FALSE,HKEY_LOCAL_MACHINE);
	CString str=msysdir;
	if(str.IsEmpty())
	{
		CRegString msysinstalldir=CRegString(REG_MSYSGIT_INSTALL,_T(""),FALSE,HKEY_LOCAL_MACHINE);
		str=msysinstalldir;
		if ( !str.IsEmpty() )
		{
			str += (str[str.GetLength()-1] != '\\') ? "\\bin" : "bin";
			msysdir=str;
			msysdir.write();
		}
		else
		{
			return false;
		}
	}
	//CGit::m_MsysGitPath=str;

	//set path

	_tdupenv_s(&oldpath,&size,_T("PATH")); 

	CString path;
	path.Format(_T("%s;%s"),oldpath,str);

	_tputenv_s(_T("PATH"),path);

	free(oldpath);

	if( !FindGitPath() )
	{
		return false;
	}
	else
	{
		bInitialized = TRUE;
		return true;
	}
}