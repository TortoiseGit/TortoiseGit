// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ProgressDlg.h"
#include "Git.h"
#include "atlconv.h"
// CProgressDlg dialog

IMPLEMENT_DYNAMIC(CProgressDlg, CResizableStandAloneDialog)

CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CProgressDlg::IDD, pParent), m_bShowCommand(true)
{

}

CProgressDlg::~CProgressDlg()
{
	if(m_pThread != NULL)
	{
		delete m_pThread;
	}
}

void CProgressDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CURRENT, this->m_CurrentWork);
	DDX_Control(pDX, IDC_TITLE_ANIMATE, this->m_Animate);
	DDX_Control(pDX, IDC_RUN_PROGRESS, this->m_Progress);
	DDX_Control(pDX, IDC_LOG, this->m_Log);
}


BEGIN_MESSAGE_MAP(CProgressDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()

BOOL CProgressDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_TITLE_ANIMATE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RUN_PROGRESS, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_LOG, TOP_LEFT,BOTTOM_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	m_Animate.Open(IDR_DOWNLOAD);
	
	CString InitialText;
	if ( !m_PreText.IsEmpty() )
	{
		InitialText = m_PreText + _T("\r\n");
	}
	if (m_bShowCommand)
	{
		InitialText += m_GitCmd+_T("\r\n\r\n");
	}
	m_Log.SetWindowTextW(InitialText);
	m_CurrentWork.SetWindowTextW(_T(""));

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (m_pThread==NULL)
	{
//		ReportError(CString(MAKEINTRESOURCE(IDS_ERR_THREADSTARTFAILED)));
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}
	return TRUE;
}

UINT CProgressDlg::ProgressThreadEntry(LPVOID pVoid)
{
	return ((CProgressDlg*)pVoid)->ProgressThread();
}

UINT CProgressDlg::ProgressThread()
{
	PROCESS_INFORMATION pi;
	HANDLE hRead;

	m_Animate.Play(0,-1,-1);

	CString *pfilename;
	if(m_LogFile.IsEmpty())
		pfilename=NULL;
	else
		pfilename=&m_LogFile;

	g_Git.RunAsync(this->m_GitCmd,&pi, &hRead,pfilename);
	this->DialogEnableWindow(IDOK,FALSE);

	DWORD readnumber;
	char buffer[2];
	CString output;
	while(ReadFile(hRead,buffer,1,&readnumber,NULL))
	{
		buffer[readnumber]=0;
		ParserCmdOutput((TCHAR)buffer[0]);
	}

	CloseHandle(pi.hThread);

	WaitForSingleObject(pi.hProcess, INFINITE);
	m_GitStatus =0;

	if(!GetExitCodeProcess(pi.hProcess,&m_GitStatus))
	{
		return GIT_ERROR_GET_EXIT_CODE;
	}

	CloseHandle(pi.hProcess);

	CloseHandle(hRead);

	m_Progress.SetPos(100);
	this->DialogEnableWindow(IDOK,TRUE);

	m_Animate.Stop();
	return 0;
}

int CProgressDlg::FindPercentage(CString &log)
{
	int s1=log.Find(_T('%'));
	if(s1<0)
		return -1;

	int s2=s1-1;
	for(int i=s1-1;i>=0;i--)
	{
		if(log[i]>=_T('0') && log[i]<=_T('9'))
			s2=i;
		else
			break;
	}
	return _ttol(log.Mid(s2,s1-s2));
}

void CProgressDlg::ParserCmdOutput(TCHAR ch)
{
	TRACE(_T("%c"),ch);
	if( ch == _T('\r') || ch == _T('\n'))
	{
		TRACE(_T("End Char %s \r\n"),ch==_T('\r')?_T("lf"):_T(""));
		TRACE(_T("End Char %s \r\n"),ch==_T('\n')?_T("cr"):_T(""));

		CString text;
		m_Log.GetWindowTextW(text);
		if(ch == _T('\r'))
		{
			RemoveLastLine(text);
		}
		text+=_T("\r\n")+m_LogText;
		m_Log.SetWindowTextW(text);
		
		m_Log.LineScroll(m_Log.GetLineCount());

		int s1=m_LogText.Find(_T(':'));
		int s2=m_LogText.Find(_T('%'));
		if(s1>0 && s2>0)
		{
			this->m_CurrentWork.SetWindowTextW(m_LogText.Left(s1));
			int pos=FindPercentage(m_LogText);
			TRACE(_T("Pos %d\r\n"),pos);
			if(pos>0)
				this->m_Progress.SetPos(pos);
		}

		m_LogText=_T("");

	}else
	{
		m_LogText+=ch;
	}

}
void CProgressDlg::RemoveLastLine(CString &str)
{
	int start;
	start=str.ReverseFind(_T('\n'));
	if(start>0)
		str=str.Left(start);
	return;
}
// CProgressDlg message handlers
