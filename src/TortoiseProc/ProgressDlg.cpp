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
	m_bAltAbortPress=false;
	m_bBufferAll=false;
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
	ON_BN_CLICKED(IDC_PROGRESS_BUTTON1,&CProgressDlg::OnBnClickedButton1)
END_MESSAGE_MAP()

BOOL CProgressDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_TITLE_ANIMATE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RUN_PROGRESS, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_LOG, TOP_LEFT,BOTTOM_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS_BUTTON1,BOTTOM_RIGHT);

	this->GetDlgItem(IDC_PROGRESS_BUTTON1)->ShowWindow(SW_HIDE);
	m_Animate.Open(IDR_DOWNLOAD);
	
	CString InitialText;
	if ( !m_PreText.IsEmpty() )
	{
		InitialText = m_PreText + _T("\r\n");
	}
#if 0
	if (m_bShowCommand && (!m_GitCmd.IsEmpty() ))
	{
		InitialText += m_GitCmd+_T("\r\n\r\n");
	}
#endif
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

//static function, Share with SyncDialog
UINT CProgressDlg::RunCmdList(CWnd *pWnd,std::vector<CString> &cmdlist,bool bShowCommand,CString *pfilename,bool *bAbort,std::vector<TCHAR>*pdata)
{
	UINT ret=0;

	PROCESS_INFORMATION pi;
	HANDLE hRead;

	pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_START,0);

	if(pdata)
		pdata->clear();
		 
	for(int i=0;i<cmdlist.size();i++)
	{
		if(cmdlist[i].IsEmpty())
			continue;

		if (bShowCommand)
		{
			CString str;
			str+= cmdlist[i]+_T("\n\n");
			for(int j=0;j<str.GetLength();j++)
			{
				if(pdata)
					pdata->push_back(str[j]);
				else
					pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,str[j]);
			}
			if(pdata)
				pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,0);
		}

		g_Git.RunAsync(cmdlist[i],&pi, &hRead,pfilename);

		DWORD readnumber;
		char buffer[2];
		CString output;
		while(ReadFile(hRead,buffer,1,&readnumber,NULL))
		{
			buffer[readnumber]=0;
			
			if(pdata)
			{
				pdata->push_back((TCHAR)buffer[0]);

				if(buffer[0] == '\r' || buffer[0] == '\n')
					pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,0);
			}else
				pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,buffer[0]);
		}
	
		CloseHandle(pi.hThread);

		WaitForSingleObject(pi.hProcess, INFINITE);
		
		DWORD status=0;
		if(!GetExitCodeProcess(pi.hProcess,&status) || *bAbort)
		{
			CloseHandle(pi.hProcess);

			CloseHandle(hRead);

			pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_FAILED,0);
			return GIT_ERROR_GET_EXIT_CODE;
		}
		ret |= status;
	}

	CloseHandle(pi.hProcess);

	CloseHandle(hRead);

	pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_END,0);

	return ret;

}

UINT CProgressDlg::ProgressThread()
{
	
	m_GitCmdList.push_back(m_GitCmd);

	CString *pfilename;

	if(m_LogFile.IsEmpty())
		pfilename=NULL;
	else
		pfilename=&m_LogFile;	

	m_GitStatus = RunCmdList(this,m_GitCmdList,m_bShowCommand,pfilename,&m_bAbort,&this->m_Databuf);;
	return 0;
}

LRESULT CProgressDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam)
{
	if(wParam == MSG_PROGRESSDLG_START)
	{
		m_BufStart = 0 ;
		m_Animate.Play(0,-1,-1);
		this->DialogEnableWindow(IDOK,FALSE);
	}
	if(wParam == MSG_PROGRESSDLG_END || wParam == MSG_PROGRESSDLG_FAILED)
	{
		if(m_bBufferAll)
		{
			m_Databuf.push_back(0);
			InsertCRLF();
			m_Log.SetWindowText(&m_Databuf[0]);
		}
		m_BufStart=0;
		this->m_Databuf.clear();

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
				GetDlgItem(IDC_PROGRESS_BUTTON1)->SetWindowText(m_changeAbortButtonOnSuccessTo);
				GetDlgItem(IDC_PROGRESS_BUTTON1)->ShowWindow(SW_SHOW);
				GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
				//Set default button is "close" rather than "push"
				this->SendMessage(WM_NEXTDLGCTL, (WPARAM)GetDlgItem(IDOK)->m_hWnd, TRUE);
			}
			else
				DialogEnableWindow(IDCANCEL, FALSE);
		}
		else
			DialogEnableWindow(IDCANCEL, FALSE);
	}

	if(!m_bBufferAll)
	{
		if(lParam == 0)
		{
			for(int i=this->m_BufStart;i<this->m_Databuf.size();i++)
			{
				ParserCmdOutput(this->m_Databuf[m_BufStart]);
				m_BufStart++;
			}
		}else
			ParserCmdOutput((TCHAR)lParam);
	}
	return 0;
}

//static function, Share with SyncDialog
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

#if 0
void CProgressDlg::ParserCmdOutput(TCHAR ch)
{
	TRACE(_T("%c"),ch);
	if( ch == _T('\r') || ch == _T('\n'))
	{
		TRACE(_T("End Char %s \r\n"),ch==_T('\r')?_T("lf"):_T(""));
		TRACE(_T("End Char %s \r\n"),ch==_T('\n')?_T("cr"):_T(""));

		int lines=m_Log.GetLineCount();

		if(ch == _T('\r'))
		{	
			int start=m_Log.LineIndex(lines-1);
			int length=m_Log.LineLength(lines-1);
			m_Log.SetSel( start,start+length);
			m_Log.ReplaceSel(m_LogText);

		}else
		{
			m_Log.SetSel(m_Log.GetWindowTextLength(),
					     m_Log.GetWindowTextLength());
			m_Log.ReplaceSel(CString(_T("\r\n"))+m_LogText);
		}
		
		if( lines > 500 ) //limited log length
		{
			int end=m_Log.LineIndex(1);
			m_Log.SetSel(0,end);
			m_Log.ReplaceSel(_T(""));
		}
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
#endif

void CProgressDlg::ParserCmdOutput(TCHAR ch)
{
	ParserCmdOutput(this->m_Log,this->m_Progress,this->m_LogText,ch,&this->m_CurrentWork);
}
void CProgressDlg::ParserCmdOutput(CRichEditCtrl &log,CProgressCtrl &progressctrl,CString &oneline, TCHAR ch, CWnd *CurrentWork)
{
	//TRACE(_T("%c"),ch);
	TRACE(_T("%c"),ch);
	if( ch == _T('\r') || ch == _T('\n'))
	{
		TRACE(_T("End Char %s \r\n"),ch==_T('\r')?_T("lf"):_T(""));
		TRACE(_T("End Char %s \r\n"),ch==_T('\n')?_T("cr"):_T(""));

		int lines=log.GetLineCount();

		if(ch == _T('\r'))
		{	
			int start=log.LineIndex(lines-1);
			int length=log.LineLength(lines-1);
			log.SetSel( start,start+length);
			log.ReplaceSel(oneline);

		}else
		{
			log.SetSel(log.GetWindowTextLength(),
					     log.GetWindowTextLength());
			log.ReplaceSel(CString(_T("\r\n"))+oneline);
		}
		
		if( lines > 500 ) //limited log length
		{
			int end=log.LineIndex(1);
			log.SetSel(0,end);
			log.ReplaceSel(_T(""));
		}
		log.LineScroll(log.GetLineCount());

		int s1=oneline.Find(_T(':'));
		int s2=oneline.Find(_T('%'));
		if(s1>0 && s2>0)
		{
			if(CurrentWork)
				CurrentWork->SetWindowTextW(oneline.Left(s1));

			int pos=FindPercentage(oneline);
			TRACE(_T("Pos %d\r\n"),pos);
			if(pos>0)
				progressctrl.SetPos(pos);
		}

		oneline=_T("");

	}else
	{
		oneline+=ch;
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

void CProgressDlg::OnBnClickedButton1()
{
	this->EndDialog(IDC_PROGRESS_BUTTON1);
	
}
void CProgressDlg::OnCancel()
{
	m_bAbort = true;
	if(m_bDone)
	{
		CResizableStandAloneDialog::OnCancel();
		return;
	}

	if( g_Git.m_CurrentGitPi.hProcess )
	{
		if(::GenerateConsoleCtrlEvent(CTRL_C_EVENT,0))
		{
			::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess ,10000);
		}else
		{
			int error=::GetLastError();
		}

		HANDLE	hProcessHandle = ::OpenProcess( PROCESS_TERMINATE, FALSE,g_Git.m_CurrentGitPi.dwProcessId);
		if( hProcessHandle )
			if(!::TerminateProcess(hProcessHandle,-1) )
			{
				int error =::GetLastError();
			}
	}
	
	::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess ,10000);
	CResizableStandAloneDialog::OnCancel();

}

void CProgressDlg::InsertCRLF()
{
	for(int i=0;i<m_Databuf.size();i++)
	{
		if(m_Databuf[i]==_T('\n'))
		{
			if(i==0 || m_Databuf[i-1]!= _T('\r'))
			{
				m_Databuf.insert(m_Databuf.begin()+i,_T('\r'));
				i++;
			}
		}
	}
}