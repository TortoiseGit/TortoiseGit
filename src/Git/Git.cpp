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

char buffer[4096];
int CGit::RunAsync(CString cmd,PROCESS_INFORMATION *piOut,HANDLE *hReadOut)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hRead, hWrite;

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor=NULL;
	sa.bInheritHandle=TRUE;
	if(!CreatePipe(&hRead,&hWrite,&sa,0))
	{
		return GIT_ERROR_OPEN_PIP;
	}
	
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	si.cb=sizeof(STARTUPINFO);
	GetStartupInfo(&si);

	si.hStdError=hWrite;
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
int CGit::Run(CString cmd, CString* output)
{
	PROCESS_INFORMATION pi;
	HANDLE hRead;
	if(RunAsync(cmd,&pi,&hRead))
		return GIT_ERROR_CREATE_PROCESS;

	DWORD readnumber;
	while(ReadFile(hRead,buffer,4090,&readnumber,NULL))
	{
		buffer[readnumber]=0;
		USES_CONVERSION;
		output->Append(A2W(buffer));
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
	Run(_T("git.cmd config user.name"),&UserName);
	return UserName;
}
CString CGit::GetUserEmail(void)
{
	CString UserName;
	Run(_T("git.cmd config user.email"),&UserName);
	return UserName;
}

CString CGit::GetCurrentBranch(void)
{
	CString branch;
	Run(_T("git.cmd branch"),&branch);
	if(branch.GetLength()>0)
	{
		branch.Replace(_T('*'),_T(' '));
		branch.TrimLeft();
		return branch;
	}
	return CString("");
}

int CGit::GetLog(CString& logOut)
{

	CString cmd;
	CString log;
	cmd=("git.cmd log -C --numstat --raw --pretty=format:\"");
	log.Format(_T("#<%c>%%n"),LOG_REV_ITEM_BEGIN);
	cmd += log;
	log.Format(_T("#<%c>%%an%%n"),LOG_REV_AUTHOR_NAME);
	cmd += log;
	log.Format(_T("#<%c>%%ae%%n"),LOG_REV_AUTHOR_EMAIL);
	cmd += log;
	log.Format(_T("#<%c>%%ai%%n"),LOG_REV_AUTHOR_DATE);
	cmd += log;
	log.Format(_T("#<%c>%%cn%%n"),LOG_REV_COMMIT_NAME);
	cmd += log;
	log.Format(_T("#<%c>%%ce%%n"),LOG_REV_COMMIT_EMAIL);
	cmd += log;
	log.Format(_T("#<%c>%%ci%%n"),LOG_REV_COMMIT_DATE);
	cmd += log;
	log.Format(_T("#<%c>%%s%%n"),LOG_REV_COMMIT_SUBJECT);
	cmd += log;
	log.Format(_T("#<%c>%%b%%n"),LOG_REV_COMMIT_BODY);
	cmd += log;
	log.Format(_T("#<%c>%%H%%n"),LOG_REV_COMMIT_HASH);
	cmd += log;
	log.Format(_T("#<%c>%%P%%n"),LOG_REV_COMMIT_PARENT);
	cmd += log;
	log.Format(_T("#<%c>%%n"),LOG_REV_COMMIT_FILE);
	cmd += log;
	cmd += CString(_T("\""));
	Run(cmd,&logOut);
	return 0;
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
	cmd.Format(_T("git.cmd rev-parse %s" ),friendname);
	Run(cmd,&out);
	int pos=out.ReverseFind(_T('\n'));
	if(pos>0)
		return out.Left(pos);
	return out;
}

int CGit::GetBranchList(CStringList &list,int *current)
{
	int ret;
	CString cmd,output;
	cmd=_T("git.exe branch");
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
			list.AddTail(one.Right(one.GetLength()-2));
			if(one[0] == _T('*'))
				if(current)
					*current=i;
		}
	}
	return ret;
}

int CGit::GetRemoteList(CStringList &list)
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
				list.AddTail(one);
			}
		}
	}
	return ret;
}