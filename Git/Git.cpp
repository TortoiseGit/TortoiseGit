#include "StdAfx.h"
#include "Git.h"
#include "atlconv.h"
#include "afx.h"

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

int CGit::Run(CString cmd, CString* output)
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
	
	DWORD readnumber;
	while(ReadFile(hRead,buffer,4090,&readnumber,NULL))
	{
		buffer[readnumber]=0;
		USES_CONVERSION;
		output->Append(A2W(buffer));
		if(readnumber<4090)
			break;
	}

	
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(hWrite);
	CloseHandle(hRead);
	return GIT_SUCCESS;
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
