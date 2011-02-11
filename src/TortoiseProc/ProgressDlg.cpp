// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ProgressDlg.h"
#include "Git.h"
#include "atlconv.h"
#include "UnicodeUtils.h"
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
	DDX_Control(pDX, IDC_PROGRESS_BUTTON1, this->m_ctrlPostCmd);
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
	AddAnchor(IDC_PROGRESS_BUTTON1,BOTTOM_LEFT);
	AddAnchor(IDC_CURRENT,TOP_LEFT);

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

	EnableSaveRestore(_T("ProgressDlg"));

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

	if(m_PostCmdList.GetCount()>0)
		m_ctrlPostCmd.AddEntries(m_PostCmdList);

	return TRUE;
}

UINT CProgressDlg::ProgressThreadEntry(LPVOID pVoid)
{
	return ((CProgressDlg*)pVoid)->ProgressThread();
}

//static function, Share with SyncDialog
UINT CProgressDlg::RunCmdList(CWnd *pWnd,std::vector<CString> &cmdlist,bool bShowCommand,CString *pfilename,bool *bAbort,CGitByteArray *pdata)
{
	UINT ret=0;

	PROCESS_INFORMATION pi;
	HANDLE hRead = 0;

	memset(&pi,0,sizeof(PROCESS_INFORMATION));

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
			str+= cmdlist[i].Trim()+_T("\n\n");
			for(int j=0;j<str.GetLength();j++)
			{
				if(pdata)
				{
					pdata->m_critSec.Lock();
					pdata->push_back(str[j]&0xFF);
					pdata->m_critSec.Unlock();
				}
				else
					pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,str[j]);
			}
			if(pdata)
				pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,0);
		}

		g_Git.RunAsync(cmdlist[i].Trim(),&pi, &hRead,pfilename);

		DWORD readnumber;
		char byte;
		CString output;
		while(ReadFile(hRead,&byte,1,&readnumber,NULL))
		{
			if(pdata)
			{
				if(byte == 0)
					byte = '\n';

				pdata->m_critSec.Lock();
				pdata->push_back( byte);
				pdata->m_critSec.Unlock();

				if(byte == '\r' || byte == '\n')
					pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,0);
			}else
				pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,byte);
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
			m_Databuf.m_critSec.Lock();
			m_Databuf.push_back(0);
			m_Databuf.m_critSec.Unlock();
			InsertCRLF();
			m_Databuf.m_critSec.Lock();
			m_Log.SetWindowText(Convert2UnionCode((char*)&m_Databuf[0]));
			m_Databuf.m_critSec.Unlock();
			m_Log.LineScroll(m_Log.GetLineCount() - m_Log.GetFirstVisibleLine() - 4);
		}
		m_BufStart=0;
		m_Databuf.m_critSec.Lock();
		this->m_Databuf.clear();
		m_Databuf.m_critSec.Unlock();

		m_bDone = true;
		m_Animate.Stop();
		m_Progress.SetPos(100);
		this->DialogEnableWindow(IDOK,TRUE);

		CString err;
		err.Format(_T("\r\nFailed 0x%x(git return wrong reture code at sometime )\r\n"),m_GitStatus);
		if(this->m_GitStatus)
		{
			//InsertColorText(this->m_Log,err,RGB(255,0,0));
		}
		else {
			InsertColorText(this->m_Log,_T("\r\nSuccess\r\n"),RGB(0,0,255));
			this->DialogEnableWindow(IDCANCEL,FALSE);
		}

		if(wParam == MSG_PROGRESSDLG_END && m_GitStatus == 0)
		{
			if(m_bAutoCloseOnSuccess)
				EndDialog(IDOK);

			if(m_PostCmdList.GetCount() > 0)
			{
				//GetDlgItem(IDC_PROGRESS_BUTTON1)->SetWindowText(m_changeAbortButtonOnSuccessTo);
				GetDlgItem(IDC_PROGRESS_BUTTON1)->ShowWindow(SW_SHOW);
				//GetDlgItem(IDCANCEL)->ShowWindow(SW_HIDE);
				//Set default button is "close" rather than "push"
				this->SendMessage(WM_NEXTDLGCTL, (WPARAM)GetDlgItem(IDOK)->m_hWnd, TRUE);
			}
		}
	}

	if(!m_bBufferAll)
	{
		if(lParam == 0)
		{
			m_Databuf.m_critSec.Lock();
			for(int i=this->m_BufStart;i<this->m_Databuf.size();i++)
			{
				char c = this->m_Databuf[m_BufStart];
				m_BufStart++;
				m_Databuf.m_critSec.Unlock();
				ParserCmdOutput(c);

				m_Databuf.m_critSec.Lock();
			}

			if(m_BufStart>1000)
			{
				m_Databuf.erase(m_Databuf.begin(), m_Databuf.begin()+m_BufStart);
				m_BufStart =0;
			}
			m_Databuf.m_critSec.Unlock();

		}else
			ParserCmdOutput((char)lParam);
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

void CProgressDlg::ParserCmdOutput(char ch)
{
	ParserCmdOutput(this->m_Log,this->m_Progress,this->m_LogTextA,ch,&this->m_CurrentWork);
}
int CProgressDlg::ClearESC(CStringA &str)
{
	return str.Replace("\033[K","");
}
void CProgressDlg::ParserCmdOutput(CRichEditCtrl &log,CProgressCtrl &progressctrl,CStringA &oneline, char ch, CWnd *CurrentWork)
{
	//TRACE(_T("%c"),ch);
	if( ch == ('\r') || ch == ('\n'))
	{
		CString str;

//		TRACE(_T("End Char %s \r\n"),ch==_T('\r')?_T("lf"):_T(""));
//		TRACE(_T("End Char %s \r\n"),ch==_T('\n')?_T("cr"):_T(""));
		
		if(ClearESC(oneline))
		{
			ch = ('\r');
		}
		
		int lines=log.GetLineCount();
		g_Git.StringAppend(&str,(BYTE*)oneline.GetBuffer(),CP_ACP);
//		TRACE(_T("%s"), str);

		if(ch == ('\r'))
		{	
			int start=log.LineIndex(lines-1);
			int length=log.LineLength(lines-1);
			log.SetSel( start,start+length);			
			log.ReplaceSel(str);

		}else
		{
			log.SetSel(log.GetWindowTextLength(),
					     log.GetWindowTextLength());
			log.ReplaceSel(CString(_T("\r\n"))+str);
		}
		
		if( lines > 500 ) //limited log length
		{
			int end=log.LineIndex(1);
			log.SetSel(0,end);
			log.ReplaceSel(_T(""));
		}
		log.LineScroll(log.GetLineCount() - log.GetFirstVisibleLine() - 4);

		int s1=oneline.ReverseFind(_T(':'));
		int s2=oneline.Find(_T('%'));
		if(s1>0 && s2>0)
		{
			if(CurrentWork)
				CurrentWork->SetWindowTextW(str.Left(s1));

			int pos=FindPercentage(str);
			TRACE(_T("Pos %d\r\n"),pos);
			if(pos>0)
				progressctrl.SetPos(pos);
		}

		oneline="";

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
	m_Log.GetWindowText(this->m_LogText);
	OnOK();
}

void CProgressDlg::OnBnClickedButton1()
{
	this->EndDialog(IDC_PROGRESS_BUTTON1 + this->m_ctrlPostCmd.GetCurrentEntry());
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
			GetLastError();
		}

		HANDLE	hProcessHandle = ::OpenProcess( PROCESS_TERMINATE, FALSE,g_Git.m_CurrentGitPi.dwProcessId);
		if( hProcessHandle )
			if(!::TerminateProcess(hProcessHandle,-1) )
			{
				GetLastError();
			}
	}
	
	::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess ,10000);
	CResizableStandAloneDialog::OnCancel();

}

void CProgressDlg::InsertCRLF()
{
	m_Databuf.m_critSec.Lock();
	for(int i=0;i<m_Databuf.size();i++)
	{
		if(m_Databuf[i]==('\n'))
		{
			if(i==0 || m_Databuf[i-1]!= ('\r'))
			{
				m_Databuf.insert(m_Databuf.begin()+i,('\r'));
				i++;
			}
		}
	}
	m_Databuf.m_critSec.Unlock();
}

void CProgressDlg::InsertColorText(CRichEditCtrl &edit,CString text,COLORREF rgb)
{
	CHARFORMAT old,cf;
	edit.GetDefaultCharFormat(cf);
	old=cf;
	cf.dwMask|=CFM_COLOR;
	cf.crTextColor=rgb;
	cf.dwEffects|=CFE_BOLD;
	cf.dwEffects &= ~CFE_AUTOCOLOR ;
	edit.SetSel(edit.GetTextLength()-1,edit.GetTextLength());
	edit.ReplaceSel(text);
	edit.SetSel(edit.LineIndex(edit.GetLineCount()-2),edit.GetTextLength());
	edit.SetSelectionCharFormat(cf);
	edit.SetSel(edit.GetTextLength(),edit.GetTextLength());
	edit.SetDefaultCharFormat(old);
	edit.LineScroll(edit.GetLineCount() - edit.GetFirstVisibleLine() - 4);
}

CString CCommitProgressDlg::Convert2UnionCode(char *buff, int size)
{
	CString str;

	CString cmd,output;
	int cp=CP_UTF8;

	cmd=_T("git.exe config i18n.logOutputEncoding");
	if(g_Git.Run(cmd,&output,CP_ACP))
		cp=CP_UTF8;

	int start=0;
	output=output.Tokenize(_T("\n"),start);
	cp=CUnicodeUtils::GetCPCode(output);

	start =0;
	if(size == -1)
		size=strlen(buff);

	for(int i=0;i<size;i++)
	{
		if(buff[i] == ']')
			start = i;
		if( start >0 && buff[i] =='\n' )
		{
			start =i;
			break;
		}
	}

	str.Empty();
	g_Git.StringAppend(&str, (BYTE*)buff, cp, start);
	g_Git.StringAppend(&str, (BYTE*)buff+start, CP_ACP,size - start);

	return str;
}
