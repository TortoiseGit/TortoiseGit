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
	: CResizableStandAloneDialog(CProgressDlg::IDD, pParent), m_bShowCommand(true), m_bAutoCloseOnSuccess(false), m_bAbort(false), m_bDone(false)
{
	m_pThread = NULL;
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
	ON_MESSAGE(MSG_PROGRESSDLG_UPDATE_UI, OnProgressUpdateUI)
	ON_BN_CLICKED(IDOK, &CProgressDlg::OnBnClickedOk)
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
	if (m_bShowCommand && (!m_GitCmd.IsEmpty() ))
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

	if(!m_Title.IsEmpty())
		this->SetWindowText(m_Title);
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

	this->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_START,0);

	CString *pfilename;
	if(m_LogFile.IsEmpty())
		pfilename=NULL;
	else
		pfilename=&m_LogFile;

	m_GitCmdList.push_back(m_GitCmd);

	m_GitStatus =0;

	for(int i=0;i<m_GitCmdList.size();i++)
	{
		if(m_GitCmdList[i].IsEmpty())
			continue;

		if (m_bShowCommand && m_GitCmdList[i]!= m_GitCmd)
		{
			CString str;
			str+= m_GitCmdList[i]+_T("\r\n\r\n");
			for(int j=0;j<str.GetLength();j++)
				this->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,str[j]);
		}

		g_Git.RunAsync(this->m_GitCmdList[i],&pi, &hRead,pfilename);

		DWORD readnumber;
		char buffer[2];
		CString output;
		while(ReadFile(hRead,buffer,1,&readnumber,NULL))
		{
			buffer[readnumber]=0;
			this->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,(TCHAR)buffer[0]);
		}
	
		CloseHandle(pi.hThread);

		WaitForSingleObject(pi.hProcess, INFINITE);
		
		DWORD status=0;
		if(!GetExitCodeProcess(pi.hProcess,&status) || m_bAbort)
		{
			CloseHandle(pi.hProcess);

			CloseHandle(hRead);

			this->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_FAILED,0);
			return GIT_ERROR_GET_EXIT_CODE;
		}
		m_GitStatus |= status;
	}

	CloseHandle(pi.hProcess);

	CloseHandle(hRead);

	this->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_END,0);

	return 0;
}

LRESULT CProgressDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam)
{
	if(wParam == MSG_PROGRESSDLG_START)
	{
		m_Animate.Play(0,-1,-1);
		this->DialogEnableWindow(IDOK,FALSE);
	}
	if(wParam == MSG_PROGRESSDLG_END || wParam == MSG_PROGRESSDLG_FAILED)
	{
		m_bDone = true;
		m_Animate.Stop();
		m_Progress.SetPos(100);
		this->DialogEnableWindow(IDOK,TRUE);

		if(wParam == MSG_PROGRESSDLG_END && m_GitStatus == 0)
		{
			if(m_bAutoCloseOnSuccess)
				EndDialog(IDOK);

			if(!m_changeAbortButtonOnSuccessTo.IsEmpty())
			{
				GetDlgItem(IDCANCEL)->SetWindowText(m_changeAbortButtonOnSuccessTo);
				//Set default button is "close" rather than "push"
				this->SendMessage(WM_NEXTDLGCTL, (WPARAM)GetDlgItem(IDOK)->m_hWnd, TRUE);
			}
			else
				DialogEnableWindow(IDCANCEL, FALSE);
		}
		else
			DialogEnableWindow(IDCANCEL, FALSE);
	}

	if(lParam != 0)
		ParserCmdOutput((TCHAR)lParam);

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

void CProgressDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	m_Log.GetWindowText(this->m_LogText);
	OnOK();
}

void CProgressDlg::OnCancel()
{
	if(m_bDone)
	{
		CResizableStandAloneDialog::OnCancel();
		return;
	}

	m_bAbort = true;
}
