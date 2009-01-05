#include "StdAfx.h"
#include "Git.h"
#include "atlconv.h"
#include "GitRev.h"

#define MAX_DIRBUFFER 1000
CGit g_Git;
CGit::CGit(void)
{
	GetCurrentDirectory(MAX_DIRBUFFER,m_CurrentDir.GetBuffer(MAX_DIRBUFFER));
}

CGit::~CGit(void)
{
}

static char g_Buffer[4096];

int CGit::RunAsync(CString cmd,PROCESS_INFORMATION *piOut,HANDLE *hReadOut,CString *StdioFile)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;
	HANDLE hStdioFile;

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
void CGit::StringAppend(CString *str,char *p)
{
       USES_CONVERSION;
       str->Append(A2W(p));

}	
BOOL CGit::IsInitRepos()
{
	CString cmdout;
	cmdout.Empty();
	if(g_Git.Run(_T("git.exe rev-parse --revs-only HEAD"),&cmdout))
	{
	//	CMessageBox::Show(NULL,cmdout,_T("TortoiseGit"),MB_OK);
		return TRUE;
	}
	if(cmdout.IsEmpty())
		return TRUE;

	return FALSE;
}
int CGit::Run(CString cmd, CString* output)
{
	PROCESS_INFORMATION pi;
	HANDLE hRead;
	if(RunAsync(cmd,&pi,&hRead))
		return GIT_ERROR_CREATE_PROCESS;

	DWORD readnumber;
	while(ReadFile(hRead,g_Buffer,1023,&readnumber,NULL))
	{
		g_Buffer[readnumber]=0;
		StringAppend(output,g_Buffer);
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

CString CGit::GetUserName(void)
{
	CString UserName;
	Run(_T("git.exe config user.name"),&UserName);
	return UserName;
}
CString CGit::GetUserEmail(void)
{
	CString UserName;
	Run(_T("git.exe config user.email"),&UserName);
	return UserName;
}

CString CGit::GetCurrentBranch(void)
{
	CString output;
	//Run(_T("git.exe branch"),&branch);

	int ret=g_Git.Run(_T("git.exe branch"),&output);
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
	log.Format(_T("#<%c>%%n"),LOG_REV_ITEM_BEGIN);
	format += log;
	if(IsFull)
	{
		log.Format(_T("#<%c>%%an%%n"),LOG_REV_AUTHOR_NAME);
		format += log;
		log.Format(_T("#<%c>%%ae%%n"),LOG_REV_AUTHOR_EMAIL);
		format += log;
		log.Format(_T("#<%c>%%ai%%n"),LOG_REV_AUTHOR_DATE);
		format += log;
		log.Format(_T("#<%c>%%cn%%n"),LOG_REV_COMMIT_NAME);
		format += log;
		log.Format(_T("#<%c>%%ce%%n"),LOG_REV_COMMIT_EMAIL);
		format += log;
		log.Format(_T("#<%c>%%ci%%n"),LOG_REV_COMMIT_DATE);
		format += log;
		log.Format(_T("#<%c>%%s%%n"),LOG_REV_COMMIT_SUBJECT);
		format += log;
		log.Format(_T("#<%c>%%b%%n"),LOG_REV_COMMIT_BODY);
		format += log;
	}
	log.Format(_T("#<%c>%%m%%H%%n"),LOG_REV_COMMIT_HASH);
	format += log;
	log.Format(_T("#<%c>%%P%%n"),LOG_REV_COMMIT_PARENT);
	format += log;

	if(IsFull)
	{
		log.Format(_T("#<%c>%%n"),LOG_REV_COMMIT_FILE);
		format += log;
	}
	return 0;
}

int CGit::GetLog(CString& logOut, CString &hash, int count)
{

	CString cmd;
	CString log;
	CString num;
	CString since;
	if(count>0)
		num.Format(_T("-n%d"),count);

	cmd.Format(_T("git.exe log %s --left-right --boundary -C --numstat --raw --pretty=format:\""),
				num);
	BuildOutputFormat(log);
	cmd += log;
	cmd += CString(_T("\"  "))+hash;
	return Run(cmd,&logOut);
}


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
	HANDLE hRead, hWrite;

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
	CString cmd;
	CString out;
	cmd.Format(_T("git.exe rev-parse %s" ),friendname);
	Run(cmd,&out);
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
	ret=g_Git.Run(cmd,&output);
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
	ret=g_Git.Run(cmd,&output);
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
	ret=g_Git.Run(cmd,&output);
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
	cmd=_T("git show-ref");
	ret=g_Git.Run(cmd,&output);
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