// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016 - TortoiseGit

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
#include "UnicodeUtils.h"
#include "IconMenu.h"
#include "LoglistCommonResource.h"
#include <Tlhelp32.h>
#include "AppUtils.h"
#include "SmartHandle.h"
#include "../TGitCache/CacheInterface.h"
#include "LoglistUtils.h"
#include "MessageBox.h"
#include "LogFile.h"
#include "CmdLineParser.h"
#include "StringUtils.h"

// CProgressDlg dialog

IMPLEMENT_DYNAMIC(CProgressDlg, CResizableStandAloneDialog)

CProgressDlg::CProgressDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CProgressDlg::IDD, pParent)
	, m_bShowCommand(true)
	, m_bAbort(false)
	, m_bDone(false)
	, m_startTick(GetTickCount64())
	, m_BufStart(0)
	, m_Git(&g_Git)
	, m_hAccel(nullptr)
{
	m_pThread = nullptr;
	m_bBufferAll=false;
	m_GitStatus = (DWORD)-1;
	int autoClose = CRegDWORD(_T("Software\\TortoiseGit\\AutoCloseGitProgress"), 0);
	CCmdLineParser parser(AfxGetApp()->m_lpCmdLine);
	if (parser.HasKey(_T("closeonend")))
		autoClose = parser.GetLongVal(_T("closeonend"));
	switch (autoClose)
	{
	case 1:
		m_AutoClose = AUTOCLOSE_IF_NO_OPTIONS;
		break;
	case 2:
		m_AutoClose = AUTOCLOSE_IF_NO_ERRORS;
		break;
	default:
		m_AutoClose = AUTOCLOSE_NO;
		break;
	}
}

CProgressDlg::~CProgressDlg()
{
	if (m_hAccel)
		DestroyAcceleratorTable(m_hAccel);
	delete m_pThread;
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
	ON_REGISTERED_MESSAGE(TaskBarButtonCreated, OnTaskbarBtnCreated)
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
	CAutoLibrary hUser = AtlLoadSystemLibraryUsingFullPath(_T("user32.dll"));
	if (hUser)
	{
		ChangeWindowMessageFilterExDFN *pfnChangeWindowMessageFilterEx = (ChangeWindowMessageFilterExDFN*)GetProcAddress(hUser, "ChangeWindowMessageFilterEx");
		if (pfnChangeWindowMessageFilterEx)
			pfnChangeWindowMessageFilterEx(m_hWnd, TaskBarButtonCreated, MSGFLT_ALLOW, &cfs);
	}
	m_pTaskbarList.Release();
	if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
		m_pTaskbarList = nullptr;

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
		InitialText = m_PreText + _T("\r\n");
#if 0
	if (m_bShowCommand && (!m_GitCmd.IsEmpty() ))
		InitialText += m_GitCmd+_T("\r\n\r\n");
#endif
	m_Log.SetWindowTextW(InitialText);
	m_CurrentWork.SetWindowTextW(_T(""));

	if (!m_PreFailText.IsEmpty())
		InsertColorText(this->m_Log, m_PreFailText, RGB(255, 0, 0));

	EnableSaveRestore(_T("ProgressDlg"));

	m_pThread = AfxBeginThread(ProgressThreadEntry, this, THREAD_PRIORITY_NORMAL,0,CREATE_SUSPENDED);
	if (!m_pThread)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		DialogEnableWindow(IDCANCEL, TRUE);
	}
	else
	{
		m_pThread->m_bAutoDelete = FALSE;
		m_pThread->ResumeThread();
	}

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, m_Git->m_CurrentDir, sWindowTitle);

	// Make sure this dialog is shown in foreground (see issue #1536)
	SetForegroundWindow();

	return TRUE;
}

static void EnsurePostMessage(CWnd *pWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
redo:
	if (!pWnd->PostMessage(Msg, wParam, lParam))
	{	
		if (GetLastError() == ERROR_NOT_ENOUGH_QUOTA)
		{
			Sleep(20);
			goto redo;
		}
		else
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": Message %d-%d could not be sent (error %d; %s)\n"), wParam, lParam, GetLastError(), (LPCTSTR)CFormatMessageWrapper());
	}
}

UINT CProgressDlg::ProgressThreadEntry(LPVOID pVoid)
{
	return reinterpret_cast<CProgressDlg*>(pVoid)->ProgressThread();
}

//static function, Share with SyncDialog
UINT CProgressDlg::RunCmdList(CWnd* pWnd, STRING_VECTOR& cmdlist, STRING_VECTOR& dirlist, bool bShowCommand, CString* pfilename, volatile bool* bAbort, CGitGuardedByteArray* pdata, CGit* git)
{
	UINT ret=0;

	std::vector<std::unique_ptr<CBlockCacheForPath>> cacheBlockList;
	std::vector<std::unique_ptr<CGit>> gitList;
	if (dirlist.empty())
		cacheBlockList.push_back(std::make_unique<CBlockCacheForPath>(git->m_CurrentDir));
	else
	{
		for (const auto& dir : dirlist)
		{
			auto pGit = std::make_unique<CGit>();
			pGit->m_CurrentDir = dir;
			gitList.push_back(std::move(pGit));
			cacheBlockList.push_back(std::make_unique<CBlockCacheForPath>(dir));
		}
	}

	EnsurePostMessage(pWnd, MSG_PROGRESSDLG_UPDATE_UI, MSG_PROGRESSDLG_START, 0);

	if(pdata)
		pdata->clear();

	for (size_t i = 0; i < cmdlist.size(); ++i)
	{
		if(cmdlist[i].IsEmpty())
			continue;

		if (bShowCommand)
		{
			CStringA str;
			if (gitList.empty() || gitList.size() == 1 && gitList[0]->m_CurrentDir == git->m_CurrentDir)
				str = CUnicodeUtils::GetMulti(cmdlist[i].Trim() + _T("\r\n\r\n"), CP_UTF8);
			else
				str = CUnicodeUtils::GetMulti((i > 0 ? _T("\r\n") : _T("")) + gitList[i]->m_CurrentDir + _T("\r\n") + cmdlist[i].Trim() + _T("\r\n\r\n"), CP_UTF8);
			for (int j = 0; j < str.GetLength(); ++j)
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

		PROCESS_INFORMATION pi;
		CAutoGeneralHandle hRead;
		int runAsyncRet = -1;
		if (gitList.empty())
			runAsyncRet = git->RunAsync(cmdlist[i].Trim(), &pi, hRead.GetPointer(), nullptr, pfilename);
		else
			runAsyncRet = gitList[i]->RunAsync(cmdlist[i].Trim(), &pi, hRead.GetPointer(), nullptr, pfilename);
		if (runAsyncRet)
		{
			EnsurePostMessage(pWnd, MSG_PROGRESSDLG_UPDATE_UI, MSG_PROGRESSDLG_FAILED, -1 * runAsyncRet);
			return runAsyncRet;
		}

		CAutoGeneralHandle piProcess(pi.hProcess);
		CAutoGeneralHandle piThread(pi.hThread);
		DWORD readnumber;
		char lastByte = '\0';
		char byte;
		CString output;
		while (ReadFile(hRead, &byte, 1, &readnumber, nullptr))
		{
			if(pdata)
			{
				if(byte == 0)
					byte = '\n';

				pdata->m_critSec.Lock();
				if (byte == '\n' && lastByte != '\r')
					pdata->push_back('\r');
				pdata->push_back( byte);
				lastByte = byte;
				pdata->m_critSec.Unlock();

				if(byte == '\r' || byte == '\n')
					pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,0);
			}
			else
				pWnd->PostMessage(MSG_PROGRESSDLG_UPDATE_UI,MSG_PROGRESSDLG_RUN,byte);
		}
		if (pdata)
		{
			pdata->m_critSec.Lock();
			bool post = !pdata->empty();
			pdata->m_critSec.Unlock();
			if (post)
				EnsurePostMessage(pWnd, MSG_PROGRESSDLG_UPDATE_UI, MSG_PROGRESSDLG_RUN, 0);
		}

		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": waiting for process to finish (%s), aborted: %d\n"), (LPCTSTR)cmdlist[i], *bAbort);

		WaitForSingleObject(pi.hProcess, INFINITE);

		DWORD status=0;
		if(!GetExitCodeProcess(pi.hProcess,&status) || *bAbort)
		{
			CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": process %s finished, status code could not be fetched, (error %d; %s), aborted: %d\n"), (LPCTSTR)cmdlist[i], GetLastError(), (LPCTSTR)CFormatMessageWrapper(), *bAbort);

			EnsurePostMessage(pWnd, MSG_PROGRESSDLG_UPDATE_UI, MSG_PROGRESSDLG_FAILED, status);
			return TGIT_GIT_ERROR_GET_EXIT_CODE;
		}
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": process %s finished with code %d\n"), (LPCTSTR)cmdlist[i], status);
		ret |= status;
	}

	EnsurePostMessage(pWnd, MSG_PROGRESSDLG_UPDATE_UI, MSG_PROGRESSDLG_END, ret);

	return ret;
}

UINT CProgressDlg::ProgressThread()
{
	if (!m_GitCmd.IsEmpty())
		m_GitCmdList.push_back(m_GitCmd);

	CString *pfilename;

	if(m_LogFile.IsEmpty())
		pfilename = nullptr;
	else
		pfilename=&m_LogFile;

	m_startTick = GetTickCount64();
	m_GitStatus = RunCmdList(this, m_GitCmdList, m_GitDirList, m_bShowCommand, pfilename, &m_bAbort, &this->m_Databuf, m_Git);
	return 0;
}

LRESULT CProgressDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam)
{
	if(wParam == MSG_PROGRESSDLG_START)
	{
		m_BufStart = 0 ;
		m_Animate.Play(0, INT_MAX, INT_MAX);
		DialogEnableWindow(IDCANCEL, TRUE);
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, 0, 100);
		}
	}
	if(wParam == MSG_PROGRESSDLG_END || wParam == MSG_PROGRESSDLG_FAILED)
	{
		CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": got message: %d\n"), wParam);
		ULONGLONG tickSpent = GetTickCount64() - m_startTick;
		CString strEndTime = CLoglistUtils::FormatDateAndTime(CTime::GetCurrentTime(), DATE_SHORTDATE, true, false);

		if(m_bBufferAll)
		{
			m_Databuf.m_critSec.Lock();
			m_Databuf.push_back(0);
			m_Log.SetWindowText(Convert2UnionCode((char*)&m_Databuf[0]));
			m_Databuf.m_critSec.Unlock();
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
		if (m_GitCmd.IsEmpty() && m_GitCmdList.empty())
			m_GitStatus = (DWORD)-1;

		// detect crashes of perl when performing git svn actions
		if (m_GitStatus == 0 && m_GitCmd.Find(_T(" svn ")) > 1)
		{
			CString log;
			m_Log.GetWindowText(log);
			if (log.GetLength() > 18 && log.Mid(log.GetLength() - 18) == _T("perl.exe.stackdump"))
				m_GitStatus = (DWORD)-1;
		}

		if (m_PostExecCallback)
		{
			CString extraMsg;
			m_PostExecCallback(m_GitStatus, extraMsg);
			if (!extraMsg.IsEmpty())
			{
				int start = m_Log.GetTextLength();
				m_Log.SetSel(start, start);
				m_Log.ReplaceSel(extraMsg);
			}
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
			if (CRegDWORD(_T("Software\\TortoiseGit\\ShowGitexeTimings"), TRUE))
				err.Format(_T("\r\n\r\n%s (%I64u ms @ %s)\r\n"), (LPCTSTR)log, tickSpent, (LPCTSTR)strEndTime);
			else
				err.Format(_T("\r\n\r\n%s\r\n"), (LPCTSTR)log);
			if (!m_GitCmd.IsEmpty() || !m_GitCmdList.empty())
				InsertColorText(this->m_Log, err, RGB(255,0,0));
			if (CRegDWORD(_T("Software\\TortoiseGit\\NoSounds"), FALSE) == FALSE)
				PlaySound((LPCTSTR)SND_ALIAS_SYSTEMEXCLAMATION, nullptr, SND_ALIAS_ID | SND_ASYNC);
		}
		else {
			if (m_pTaskbarList)
				m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NOPROGRESS);
			CString temp;
			temp.LoadString(IDS_SUCCESS);
			CString log;
			if (CRegDWORD(_T("Software\\TortoiseGit\\ShowGitexeTimings"), TRUE))
				log.Format(_T("\r\n%s (%I64u ms @ %s)\r\n"), (LPCTSTR)temp, tickSpent, (LPCTSTR)strEndTime);
			else
				log.Format(_T("\r\n%s\r\n"), (LPCTSTR)temp);
			InsertColorText(this->m_Log, log, RGB(0,0,255));
			this->DialogEnableWindow(IDCANCEL,FALSE);
		}

		m_Log.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);

		if (wParam == MSG_PROGRESSDLG_END)
		{
			if (m_PostCmdCallback)	// new handling method using callback
			{
				m_PostCmdCallback(m_GitStatus, m_PostCmdList);

				if (!m_PostCmdList.empty())
				{
					int i = 0;
					for (const auto& entry : m_PostCmdList)
					{
						++i;
						m_ctrlPostCmd.AddEntry(entry.icon, entry.label);
						TCHAR accellerator = CStringUtils::GetAccellerator(entry.label);
						if (accellerator == L'\0')
							continue;
						++m_accellerators[accellerator].cnt;
						if (m_accellerators[accellerator].cnt > 1)
							m_accellerators[accellerator].id = -1;
						else
							m_accellerators[accellerator].id = i - 1;
					}

					if (m_accellerators.size())
					{
						LPACCEL lpaccelNew = (LPACCEL)LocalAlloc(LPTR, m_accellerators.size() * sizeof(ACCEL));
						SCOPE_EXIT { LocalFree(lpaccelNew); };
						i = 0;
						for (auto& entry : m_accellerators)
						{
							lpaccelNew[i].cmd = (WORD)(WM_USER + 1 + entry.second.id);
							lpaccelNew[i].fVirt = FVIRTKEY | FALT;
							lpaccelNew[i].key = entry.first;
							entry.second.wmid = lpaccelNew[i].cmd;
							++i;
						}
						m_hAccel = CreateAcceleratorTable(lpaccelNew, (int)m_accellerators.size());
					}
					GetDlgItem(IDC_PROGRESS_BUTTON1)->ShowWindow(SW_SHOW);
				}
			}
		}

		if(wParam == MSG_PROGRESSDLG_END && m_GitStatus == 0)
		{
			if (m_AutoClose == AUTOCLOSE_IF_NO_OPTIONS && m_PostCmdList.empty() || m_AutoClose == AUTOCLOSE_IF_NO_ERRORS)
				PostMessage(WM_COMMAND, 1, (LPARAM)GetDlgItem(IDOK)->m_hWnd);
		}
	}

	if(!m_bBufferAll)
	{
		if(lParam == 0)
		{
			m_Databuf.m_critSec.Lock();
			for (size_t i = this->m_BufStart; i < this->m_Databuf.size(); ++i)
			{
				char c = this->m_Databuf[m_BufStart];
				++m_BufStart;
				m_Databuf.m_critSec.Unlock();
				ParserCmdOutput(c);

				m_Databuf.m_critSec.Lock();
			}

			if(m_BufStart>1000)
			{
				m_Databuf.erase(m_Databuf.cbegin(), m_Databuf.cbegin() + m_BufStart);
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
					for (int i = escapePosition + 2; i < colorEnd; ++i)
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
		CString str = CUnicodeUtils::GetUnicode(oneline);

//		TRACE(_T("End Char %s \r\n"),ch==_T('\r')?_T("lf"):_T(""));
//		TRACE(_T("End Char %s \r\n"),ch==_T('\n')?_T("cr"):_T(""));

		int lines = log.GetLineCount();
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
		log.PostMessage(WM_VSCROLL, SB_BOTTOM, 0);

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

		oneline.Empty();
	}
	else
		oneline+=ch;
}
void CProgressDlg::RemoveLastLine(CString &str)
{
	int start;
	start=str.ReverseFind(_T('\n'));
	if(start>0)
		str=str.Left(start);
}
// CProgressDlg message handlers

void CProgressDlg::WriteLog() const
{
	CLogFile logfile(g_Git.m_CurrentDir);
	if (logfile.Open())
	{
		logfile.AddTimeLine();
		CString text = GetLogText();
		LPCTSTR psz_string = text;
		while (*psz_string)
		{
			size_t i_len = wcscspn(psz_string, L"\r\n");
			logfile.AddLine(CString(psz_string, (int)i_len));
			psz_string += i_len;
			if (*psz_string == '\r')
			{
				++psz_string;
				if (*psz_string == '\n')
					++psz_string;
			}
			else if (*psz_string == '\n')
				++psz_string;
		}
		if (m_bAbort)
		{
			CString canceled;
			canceled.LoadString(IDS_USERCANCELLED);
			logfile.AddLine(canceled);
		}
		logfile.Close();
	}
}

void CProgressDlg::OnBnClickedOk()
{
	if (m_pThread) // added here because Close-button is "called" from thread by PostMessage
		::WaitForSingleObject(m_pThread->m_hThread, 5000);
	m_Log.GetWindowText(this->m_LogText);
	WriteLog();
	OnOK();
}

void CProgressDlg::OnBnClickedButton1()
{
	WriteLog();
	ShowWindow(SW_HIDE);
	m_PostCmdList.at(m_ctrlPostCmd.GetCurrentEntry()).action();
	EndDialog(IDOK);
} 

void CProgressDlg::OnClose()
{
	DialogEnableWindow(IDCANCEL, TRUE);
	__super::OnClose();
}

void CProgressDlg::OnCancel()
{
	CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) _T(": User canceled\n"));
	m_bAbort = true;
	if(m_bDone)
	{
		WriteLog();
		CResizableStandAloneDialog::OnCancel();
		return;
	}

	if( m_Git->m_CurrentGitPi.hProcess )
	{
		DWORD dwConfirmKillProcess = CRegDWORD(_T("Software\\TortoiseGit\\ConfirmKillProcess"));
		if (dwConfirmKillProcess && CMessageBox::Show(m_hWnd, IDS_PROC_CONFIRMKILLPROCESS, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
		if(::GenerateConsoleCtrlEvent(CTRL_C_EVENT,0))
		{
			::WaitForSingleObject(m_Git->m_CurrentGitPi.hProcess ,10000);
		}

		KillProcessTree(m_Git->m_CurrentGitPi.dwProcessId);
	}

	::WaitForSingleObject(m_Git->m_CurrentGitPi.hProcess ,10000);
	if (m_pThread)
	{
		if (::WaitForSingleObject(m_pThread->m_hThread, 5000) == WAIT_TIMEOUT)
			g_Git.KillRelatedThreads(m_pThread);
	}

	WriteLog();
	CResizableStandAloneDialog::OnCancel();
}

void CProgressDlg::KillProcessTree(DWORD dwProcessId, unsigned int depth)
{
	// recursively kills a process tree
	// This is not optimized, but works and isn't called very often ;)

	if (!dwProcessId || depth > 20)
		return;

	PROCESSENTRY32 pe = { 0 };
	pe.dwSize = sizeof(PROCESSENTRY32);

	CAutoGeneralHandle hSnap = ::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (::Process32First(hSnap, &pe))
	{
		do
		{
			if (pe.th32ParentProcessID == dwProcessId)
				KillProcessTree(pe.th32ProcessID, depth + 1);
		} while (::Process32Next(hSnap, &pe));

		CAutoGeneralHandle hProc = ::OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
		if (hProc)
			::TerminateProcess(hProc, 1);
	}
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
}

CString CCommitProgressDlg::Convert2UnionCode(char *buff, int size)
{
	int start=0;
	if(size == -1)
		size = (int)strlen(buff);

	for (int i = 0; i < size; ++i)
	{
		if(buff[i] == ']')
			start = i;
		if( start >0 && buff[i] =='\n' )
		{
			start =i;
			break;
		}
	}

	CString str;
	CGit::StringAppend(&str, (BYTE*)buff, g_Git.m_LogEncode, start);
	CGit::StringAppend(&str, (BYTE*)buff + start, CP_UTF8, size - start);

	ClearESC(str);

	return str;
}

LRESULT CProgressDlg::OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return __super::OnTaskbarButtonCreated(wParam, lParam);
}

BOOL CProgressDlg::PreTranslateMessage(MSG* pMsg)
{
	if (m_hAccel && TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
		return TRUE;
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
				long start = -1, end = -1;
				auto pEdit = (CRichEditCtrl *)GetDlgItem(IDC_LOG);
				pEdit->GetSel(start, end);
				popup.AppendMenuIcon(WM_COPY, IDS_SCIEDIT_COPY, IDI_COPYCLIP);
				if (start >= end)
					popup.EnableMenuItem(WM_COPY, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
				popup.AppendMenu(MF_SEPARATOR);
				popup.AppendMenuIcon(EM_SETSEL, IDS_STATUSLIST_CONTEXT_COPYEXT, IDI_COPYCLIP);
				int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, pMsg->pt.x, pMsg->pt.y, this);
				switch (cmd)
				{
					case 0: // no command selected
						break;
					case EM_SETSEL:
						{
							pEdit->SetRedraw(FALSE);
							int oldLine = pEdit->GetFirstVisibleLine();
							pEdit->SetSel(0, -1);
							pEdit->Copy();
							pEdit->SetSel(start, end);
							int newLine = pEdit->GetFirstVisibleLine();
							pEdit->LineScroll(oldLine - newLine);
							pEdit->SetRedraw(TRUE);
							pEdit->RedrawWindow();
						}
						break;
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

LRESULT CProgressDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	if (m_hAccel && message == WM_COMMAND && LOWORD(wParam) >= WM_USER && LOWORD(wParam) <= WM_USER + m_accellerators.size())
	{
		for (const auto& entry : m_accellerators)
		{
			if (entry.second.wmid != LOWORD(wParam))
				continue;
			if (entry.second.id == -1)
				m_ctrlPostCmd.PostMessage(WM_KEYDOWN, VK_F4, NULL);
			else
			{
				m_ctrlPostCmd.SetCurrentEntry(entry.second.id);
				OnBnClickedButton1();
			}
			return 0;
		}
	}

	return __super::DefWindowProc(message, wParam, lParam);
}
