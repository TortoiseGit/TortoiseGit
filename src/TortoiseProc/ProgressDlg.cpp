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
// ProgressDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ProgressDlg.h"
#include "Git.h"
#include "atlconv.h"
#include "UnicodeUtils.h"
#include "IconMenu.h"
#include "LoglistCommonResource.h"
#include "Tlhelp32.h"
#include "AppUtils.h"
#include "SmartHandle.h"
#include "../TGitCache/CacheInterface.h"
#include "LoglistUtils.h"

// CProgressDlg dialog

IMPLEMENT_DYNAMIC(CProgressDlg, CResizableStandAloneDialog)

CProgressDlg::CProgressDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CProgressDlg::IDD, pParent), m_bShowCommand(true), m_bAutoCloseOnSuccess(false), m_bAbort(false), m_bDone(false), m_startTick(GetTickCount())
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
	ON_WM_CLOSE()
	ON_MESSAGE(MSG_PROGRESSDLG_UPDATE_UI, OnProgressUpdateUI)
	ON_BN_CLICKED(IDOK, &CProgressDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_PROGRESS_BUTTON1,&CProgressDlg::OnBnClickedButton1)
	ON_REGISTERED_MESSAGE(WM_TASKBARBTNCREATED, OnTaskbarBtnCreated)
END_MESSAGE_MAP()

BOOL CProgressDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// Let the TaskbarButtonCreated message through the UIPI filter. If we don't
	// do this, Explorer would be unable to send that message to our window if we
	// were running elevated. It's OK to make the call all the time, since if we're
	// not elevated, this is a no-op.
	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
	CAutoLibrary hUser = ::LoadLibrary(_T("user32.dll"));
	if (hUser)
	{
		ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
		if (pfnChangeWindowMessageFilterEx)
		{
			pfnChangeWindowMessageFilterEx(m_hWnd, WM_TASKBARBTNCREATED, MSGFLT_ALLOW, &cfs);
		}
	}
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);

	AddAnchor(IDC_TITLE_ANIMATE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_RUN_PROGRESS, TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_LOG, TOP_LEFT,BOTTOM_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDC_PROGRESS_BUTTON1,BOTTOM_LEFT);
	AddAnchor(IDC_CURRENT,TOP_LEFT);

	this->GetDlgItem(IDC_PROGRESS_BUTTON1)->ShowWindow(SW_HIDE);
	m_Animate.Open(IDR_DOWNLOAD);

	CFont m_logFont;
	CAppUtils::CreateFontForLogs(m_logFont);
	//GetDlgItem(IDC_CMD_LOG)->SetFont(&m_logFont);
	m_Log.SetFont(&m_logFont);

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

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

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

	CBlockCacheForPath cacheBlock(g_Git.m_CurrentDir);

	pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_START,0);

	if(pdata)
		pdata->clear();

	for(int i=0;i<cmdlist.size();i++)
	{
		if(cmdlist[i].IsEmpty())
			continue;

		if (bShowCommand)
		{
			CStringA str = CUnicodeUtils::GetMulti(cmdlist[i].Trim() + _T("\n\n"), CP_UTF8);
			for(int j=0;j<str.GetLength();j++)
			{
				if(pdata)
				{
					pdata->m_critSec.Lock();
					pdata->push_back(str[j]);
					pdata->m_critSec.Unlock();
				}
				else
					pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,str[j]);
			}
			if(pdata)
				pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,0);
		}

		g_Git.RunAsync(cmdlist[i].Trim(),&pi, &hRead, NULL, pfilename);

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
			}
			else
				pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,byte);
		}

		CloseHandle(pi.hThread);

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD status=0;
		if(!GetExitCodeProcess(pi.hProcess,&status) || *bAbort)
		{
			CloseHandle(pi.hProcess);

			CloseHandle(hRead);

			pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI, MSG_PROGRESSDLG_FAILED, status);
			return TGIT_GIT_ERROR_GET_EXIT_CODE;
		}
		ret |= status;
	}

	CloseHandle(pi.hProcess);

	CloseHandle(hRead);

	pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI, MSG_PROGRESSDLG_END, ret);

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

	m_startTick = GetTickCount();
	m_GitStatus = RunCmdList(this,m_GitCmdList,m_bShowCommand,pfilename,&m_bAbort,&this->m_Databuf);;
	return 0;
}

LRESULT CProgressDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam)
{
	if(wParam == MSG_PROGRESSDLG_START)
	{
		m_BufStart = 0 ;
		m_Animate.Play(0, INT_MAX, INT_MAX);
		this->DialogEnableWindow(IDOK,FALSE);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, 0, 100);
		}
	}
	if(wParam == MSG_PROGRESSDLG_END || wParam == MSG_PROGRESSDLG_FAILED)
	{
		DWORD tickSpent = GetTickCount() - m_startTick;
		CString strEndTime = CLoglistUtils::FormatDateAndTime(CTime::GetCurrentTime(), DATE_SHORTDATE, true, false);

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

		m_GitStatus = (DWORD)lParam;

		// detect crashes of perl when performing git svn actions
		if (m_GitStatus == 0 && m_GitCmd.Find(_T(" svn ")) > 1)
		{
			CString log;
			m_Log.GetWindowText(log);
			if (log.GetLength() > 18 && log.Mid(log.GetLength() - 18) == _T("perl.exe.stackdump"))
				m_GitStatus = (DWORD)-1;
		}

		if(this->m_GitStatus)
		{
			if (m_pTaskbarList)
			{
				m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
				m_pTaskbarList->SetProgressValue(m_hWnd, 100, 100);
			}
			CString log;
			log.Format(IDS_PROC_PROGRESS_GITUNCLEANEXIT, m_GitStatus);
			CString err;
			err.Format(_T("\r\n\r\n%s (%d ms @ %s)\r\n"), log, tickSpent, strEndTime);
			InsertColorText(this->m_Log, err, RGB(255,0,0));
		}
		else {
			if (m_pTaskbarList)
				m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
			CString temp;
			temp.LoadString(IDS_SUCCESS);
			CString log;
			log.Format(_T("\r\n%s (%d ms @ %s)\r\n"), temp, tickSpent, strEndTime);
			InsertColorText(this->m_Log, log, RGB(0,0,255));
			this->DialogEnableWindow(IDCANCEL,FALSE);
		}

		if(wParam == MSG_PROGRESSDLG_END && m_GitStatus == 0)
		{
			if(m_bAutoCloseOnSuccess)
			{
				m_Log.GetWindowText(this->m_LogText);
				EndDialog(IDOK);
			}

			if(m_PostCmdList.GetCount() > 0)
				GetDlgItem(IDC_PROGRESS_BUTTON1)->ShowWindow(SW_SHOW);
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

		}
		else
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

void CProgressDlg::ParserCmdOutput(char ch)
{
	ParserCmdOutput(this->m_Log,this->m_Progress,this->m_hWnd,this->m_pTaskbarList,this->m_LogTextA,ch,&this->m_CurrentWork);
}
void CProgressDlg::ClearESC(CString &str)
{
	// see http://ascii-table.com/ansi-escape-sequences.php and http://tldp.org/HOWTO/Bash-Prompt-HOWTO/c327.html
	str.Replace(_T("\033[K"), _T("")); // erase until end of line; no need to care for this, because we always clear the whole line

	// drop colors
	while (true)
	{
		int escapePosition = str.Find(_T('\033'));
		if (escapePosition >= 0 && str.GetLength() >= escapePosition + 3)
		{
			if (str.Mid(escapePosition, 2) == _T("\033["))
			{
				int colorEnd = str.Find(_T('m'), escapePosition + 2);
				if (colorEnd > 0)
				{
					bool found = true;
					for (int i = escapePosition + 2; i < colorEnd; i++)
					{
						if (str[i] != _T(';') && (str[i] < _T('0') && str[i] > _T('9')))
						{
							found = false;
							break;
						}
					}
					if (found)
					{
						if (escapePosition > 0)
							str = str.Left(escapePosition) + str.Mid(colorEnd + 1);
						else
							str = str.Mid(colorEnd);
						continue;
					}
				}
			}
		}
		break;
	}
}
void CProgressDlg::ParserCmdOutput(CRichEditCtrl &log,CProgressCtrl &progressctrl,HWND m_hWnd,CComPtr<ITaskbarList3> m_pTaskbarList,CStringA &oneline, char ch, CWnd *CurrentWork)
{
	//TRACE(_T("%c"),ch);
	if( ch == ('\r') || ch == ('\n'))
	{
		CString str;

//		TRACE(_T("End Char %s \r\n"),ch==_T('\r')?_T("lf"):_T(""));
//		TRACE(_T("End Char %s \r\n"),ch==_T('\n')?_T("cr"):_T(""));

		int lines = log.GetLineCount();
		g_Git.StringAppend(&str, (BYTE*)oneline.GetBuffer(), CP_UTF8);
		str.Trim();
//		TRACE(_T("%s"), str);

		ClearESC(str);

		if(ch == ('\r'))
		{
			int start=log.LineIndex(lines-1);
			log.SetSel(start, log.GetTextLength());
			log.ReplaceSel(str);
		}
		else
		{
			int length = log.GetWindowTextLength();
			log.SetSel(length, length);
			if (length > 0)
				log.ReplaceSel(_T("\r\n") + str);
			else
				log.ReplaceSel(str);
		}

		if (lines > 500) //limited log length
		{
			int end=log.LineIndex(1);
			log.SetSel(0,end);
			log.ReplaceSel(_T(""));
		}
		log.LineScroll(log.GetLineCount() - log.GetFirstVisibleLine() - 4);

		int s1=oneline.ReverseFind(_T(':'));
		int s2=oneline.Find(_T('%'));
		if (s1 > 0 && s2 > 0)
		{
			if(CurrentWork)
				CurrentWork->SetWindowTextW(str.Left(s1));

			int pos=FindPercentage(str);
			TRACE(_T("Pos %d\r\n"),pos);
			if(pos>0)
			{
				progressctrl.SetPos(pos);
				if (m_pTaskbarList)
				{
					m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
					m_pTaskbarList->SetProgressValue(m_hWnd, pos, 100);
				}
			}
		}

		oneline="";

	}
	else
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
	this->EndDialog((int)(IDC_PROGRESS_BUTTON1 + this->m_ctrlPostCmd.GetCurrentEntry()));
}

void CProgressDlg::OnClose()
{
	DialogEnableWindow(IDCANCEL, TRUE);
	__super::OnClose();
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
		}
		else
		{
			GetLastError();
		}

		KillProcessTree(g_Git.m_CurrentGitPi.dwProcessId);
	}

	::WaitForSingleObject(g_Git.m_CurrentGitPi.hProcess ,10000);
	CResizableStandAloneDialog::OnCancel();
}

void CProgressDlg::KillProcessTree(DWORD dwProcessId, unsigned int depth)
{
	// recursively kills a process tree
	// This is not optimized, but works and isn't called very often ;)

	if (!dwProcessId || depth > 20)
		return;

	PROCESSENTRY32 pe;
	memset(&pe, 0, sizeof(PROCESSENTRY32));
	pe.dwSize = sizeof(PROCESSENTRY32);

	CAutoGeneralHandle hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (::Process32First(hSnap, &pe))
	{
		do
		{
			if (pe.th32ParentProcessID == dwProcessId)
				KillProcessTree(pe.th32ProcessID, depth + 1);
		} while (::Process32Next(hSnap, &pe));

		HANDLE hProc = ::OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
		if (hProc)
			::TerminateProcess(hProc, 1);
	}
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

	CString cmd, output;
	int cp=CP_UTF8;

	cmd=_T("git.exe config i18n.logOutputEncoding");
	if (g_Git.Run(cmd, &output, NULL, CP_UTF8))
		cp=CP_UTF8;

	int start=0;
	output=output.Tokenize(_T("\n"),start);
	cp=CUnicodeUtils::GetCPCode(output);

	start =0;
	if(size == -1)
		size = (int)strlen(buff);

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
	g_Git.StringAppend(&str, (BYTE*)buff + start, CP_UTF8, size - start);

	ClearESC(str);

	return str;
}

LRESULT CProgressDlg::OnTaskbarBtnCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return 0;
}

BOOL CProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		if (pMsg->wParam == VK_ESCAPE)
		{
			// pressing the ESC key should close the dialog. But since we disabled the escape
			// key (so the user doesn't get the idea that he could simply undo an e.g. update)
			// this won't work.
			// So if the user presses the ESC key, change it to VK_RETURN so the dialog gets
			// the impression that the OK button was pressed.
			if ((!GetDlgItem(IDCANCEL)->IsWindowEnabled())
				&&(GetDlgItem(IDOK)->IsWindowEnabled())&&(GetDlgItem(IDOK)->IsWindowVisible()))
			{
				// since we convert ESC to RETURN, make sure the OK button has the focus.
				GetDlgItem(IDOK)->SetFocus();
				pMsg->wParam = VK_RETURN;
			}
		}
	}
	else if (pMsg->message == WM_CONTEXTMENU || pMsg->message == WM_RBUTTONDOWN)
	{
		CWnd * pWnd = (CWnd*) GetDlgItem(IDC_LOG);
		if (pWnd == GetFocus())
		{
			CIconMenu popup;
			if (popup.CreatePopupMenu())
			{
				popup.AppendMenuIcon(WM_COPY, IDS_SCIEDIT_COPY, IDI_COPYCLIP);
				if (m_Log.GetSelText().IsEmpty())
					popup.EnableMenuItem(WM_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				popup.AppendMenu(MF_SEPARATOR);
				popup.AppendMenuIcon(EM_SETSEL, IDS_SCIEDIT_SELECTALL);
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pMsg->pt.x, pMsg->pt.y, this);
				switch (cmd)
				{
					case 0: // no command selected
						break;
					case EM_SETSEL:
					case WM_COPY:
						::SendMessage(GetDlgItem(IDC_LOG)->GetSafeHwnd(), cmd, 0, -1);
						break;
				}
				return TRUE;
			}
		}
	}
	return __super::PreTranslateMessage(pMsg);
}
