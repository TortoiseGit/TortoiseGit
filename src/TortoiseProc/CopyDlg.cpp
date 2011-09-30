// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "stdafx.h"
#include "TortoiseProc.h"
#include "CopyDlg.h"
#include "MessageBox.h"
#include "UnicodeUtils.h"
#include "RepositoryBrowser.h"
#include "BrowseFolder.h"
#include "Registry.h"
#include "TSVNPath.h"
#include "AppUtils.h"
#include "HistoryDlg.h"

IMPLEMENT_DYNAMIC(CCopyDlg, CResizableStandAloneDialog)
CCopyDlg::CCopyDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCopyDlg::IDD, pParent)
	, m_URL(_T(""))
	, m_sLogMessage(_T(""))
	, m_sBugID(_T(""))
	, m_CopyRev(SVNRev::REV_HEAD)
	, m_bDoSwitch(false)
	, m_bSettingChanged(false)
	, m_bCancelled(false)
	, m_pThread(NULL)
	, m_pLogDlg(NULL)
	, m_bThreadRunning(0)
{
}

CCopyDlg::~CCopyDlg()
{
	if (m_pLogDlg)
		delete m_pLogDlg;
}

void CCopyDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_URLCOMBO, m_URLCombo);
	DDX_Control(pDX, IDC_BROWSE, m_butBrowse);
	DDX_Text(pDX, IDC_BUGID, m_sBugID);
	DDX_Control(pDX, IDC_LOGMESSAGE, m_cLogMessage);
	DDX_Check(pDX, IDC_DOSWITCH, m_bDoSwitch);
}


BEGIN_MESSAGE_MAP(CCopyDlg, CResizableStandAloneDialog)
	ON_REGISTERED_MESSAGE(WM_REVSELECTED, OnRevSelected)
	ON_MESSAGE(WM_TSVN_MAXREVFOUND, OnRevFound)
	ON_BN_CLICKED(IDC_BROWSE, OnBnClickedBrowse)
	ON_BN_CLICKED(IDHELP, OnBnClickedHelp)
	ON_BN_CLICKED(IDC_BROWSEFROM, OnBnClickedBrowsefrom)
	ON_BN_CLICKED(IDC_COPYHEAD, OnBnClickedCopyhead)
	ON_BN_CLICKED(IDC_COPYREV, OnBnClickedCopyrev)
	ON_BN_CLICKED(IDC_COPYWC, OnBnClickedCopywc)
	ON_BN_CLICKED(IDC_HISTORY, OnBnClickedHistory)
	ON_EN_CHANGE(IDC_LOGMESSAGE, OnEnChangeLogmessage)
	ON_CBN_EDITCHANGE(IDC_URLCOMBO, &CCopyDlg::OnCbnEditchangeUrlcombo)
END_MESSAGE_MAP()


BOOL CCopyDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AdjustControlSize(IDC_COPYHEAD);
	AdjustControlSize(IDC_COPYREV);
	AdjustControlSize(IDC_COPYWC);
	AdjustControlSize(IDC_DOSWITCH);

	CTSVNPath path(m_path);

	m_History.SetMaxHistoryItems((LONG)CRegDWORD(_T("Software\\TortoiseGit\\MaxHistoryItems"), 25));

	SetRevision(m_CopyRev);

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_HISTORY, IDS_COMMITDLG_HISTORY_TT);

	if (SVN::PathIsURL(path))
	{
		DialogEnableWindow(IDC_COPYWC, FALSE);
		DialogEnableWindow(IDC_DOSWITCH, FALSE);
		SetDlgItemText(IDC_COPYSTARTLABEL, CString(MAKEINTRESOURCE(IDS_COPYDLG_FROMURL)));
	}

	m_bFile = !path.IsDirectory();
	SVN svn;
	m_wcURL = svn.GetURLFromPath(path);
	CString sUUID = svn.GetUUIDFromPath(path);
	if (m_wcURL.IsEmpty())
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_NOURLOFFILE, IDS_APPNAME, MB_ICONERROR);
		TRACE(_T("could not retrieve the URL of the file!\n"));
		this->EndDialog(IDCANCEL);		//exit
	}
	m_URLCombo.SetURLHistory(TRUE);
	m_URLCombo.LoadHistory(_T("Software\\TortoiseGit\\History\\repoURLS\\")+sUUID, _T("url"));
	m_URLCombo.AddString(CTSVNPath(m_wcURL).GetUIPathString(), 0);
	m_URLCombo.SelectString(-1, CTSVNPath(m_wcURL).GetUIPathString());
	SetDlgItemText(IDC_FROMURL, m_wcURL);
	if (!m_URL.IsEmpty())
		m_URLCombo.SetWindowText(m_URL);

	CString reg;
	reg.Format(_T("Software\\TortoiseGit\\History\\commit%s"), (LPCTSTR)sUUID);
	m_History.Load(reg, _T("logmsgs"));

	m_ProjectProperties.ReadProps(m_path);
	m_cLogMessage.Init(m_ProjectProperties);
	m_cLogMessage.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));
	if (m_ProjectProperties.sMessage.IsEmpty())
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_LOGMESSAGE)->SetFocus();
	}
	else
	{
		GetDlgItem(IDC_BUGID)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_BUGIDLABEL)->ShowWindow(SW_SHOW);
		if (!m_ProjectProperties.sLabel.IsEmpty())
			SetDlgItemText(IDC_BUGIDLABEL, m_ProjectProperties.sLabel);
		GetDlgItem(IDC_BUGID)->SetFocus();
	}
	if (!m_sLogMessage.IsEmpty())
		m_cLogMessage.SetText(m_sLogMessage);

	AddAnchor(IDC_REPOGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COPYSTARTLABEL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_FROMURL, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_TOURLLABEL, TOP_LEFT);
	AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BROWSE, TOP_RIGHT);
	AddAnchor(IDC_FROMGROUP, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COPYHEAD, TOP_LEFT);
	AddAnchor(IDC_COPYREV, TOP_LEFT);
	AddAnchor(IDC_COPYREVTEXT, TOP_LEFT);
	AddAnchor(IDC_BROWSEFROM, TOP_LEFT);
	AddAnchor(IDC_COPYWC, TOP_LEFT);
	AddAnchor(IDC_MSGGROUP, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_HISTORY, TOP_LEFT);
	AddAnchor(IDC_BUGIDLABEL, TOP_RIGHT);
	AddAnchor(IDC_BUGID, TOP_RIGHT);
	AddAnchor(IDC_LOGMESSAGE, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_DOSWITCH, BOTTOM_LEFT);
	AddAnchor(IDOK, BOTTOM_RIGHT);
	AddAnchor(IDCANCEL, BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	if ((m_pParentWnd==NULL)&&(hWndExplorer))
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	EnableSaveRestore(_T("CopyDlg"));

	m_bSettingChanged = false;
	// start a thread to obtain the highest revision number of the working copy
	// without blocking the dialog
	if ((m_pThread = AfxBeginThread(FindRevThreadEntry, this))==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}

	return TRUE;
}

UINT CCopyDlg::FindRevThreadEntry(LPVOID pVoid)
{
	return ((CCopyDlg*)pVoid)->FindRevThread();
}

UINT CCopyDlg::FindRevThread()
{
	InterlockedExchange(&m_bThreadRunning, TRUE);
	if (GetWCRevisionStatus(m_path, true, m_minrev, m_maxrev, m_bswitched, m_bmodified, m_bSparse))
	{
		if (!m_bCancelled)
			SendMessage(WM_TSVN_MAXREVFOUND);
	}
	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

void CCopyDlg::OnOK()
{
	m_bCancelled = true;
	// check if the status thread has already finished
	if (m_pThread)
	{
		WaitForSingleObject(m_pThread->m_hThread, 1000);
		if (m_bThreadRunning)
		{
			// we gave the thread a chance to quit. Since the thread didn't
			// listen to us we have to kill it.
			TerminateThread(m_pThread->m_hThread, (DWORD)-1);
			InterlockedExchange(&m_bThreadRunning, FALSE);
		}
	}

	CString id;
	GetDlgItemText(IDC_BUGID, id);
	CString sRevText;
	GetDlgItemText(IDC_COPYREVTEXT, sRevText);
	if (!m_ProjectProperties.CheckBugID(id))
	{
		ShowBalloon(IDC_BUGID, IDS_COMMITDLG_ONLYNUMBERS);
		return;
	}
	m_sLogMessage = m_cLogMessage.GetText();
	if ((m_ProjectProperties.bWarnIfNoIssue) && (id.IsEmpty() && !m_ProjectProperties.HasBugID(m_sLogMessage)))
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_COMMITDLG_NOISSUEWARNING, IDS_APPNAME, MB_YESNO | MB_ICONWARNING)!=IDYES)
			return;
	}
	UpdateData(TRUE);

	if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
		m_CopyRev = SVNRev(SVNRev::REV_HEAD);
	else if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYWC)
		m_CopyRev = SVNRev(SVNRev::REV_WC);
	else
		m_CopyRev = SVNRev(sRevText);

	if (!m_CopyRev.IsValid())
	{
		ShowBalloon(IDC_COPYREVTEXT, IDS_ERR_INVALIDREV);
		return;
	}

	CString combourl;
	m_URLCombo.GetWindowText(combourl);
	if ((m_wcURL.CompareNoCase(combourl)==0)&&(m_CopyRev.IsHead()))
	{
		CString temp;
		temp.Format(IDS_ERR_COPYITSELF, (LPCTSTR)m_wcURL, (LPCTSTR)m_URLCombo.GetString());
		CMessageBox::Show(this->m_hWnd, temp, _T("TortoiseSVN"), MB_ICONERROR);
		return;
	}

	m_URLCombo.SaveHistory();
	m_URL = m_URLCombo.GetString();
	if (!CTSVNPath(m_URL).IsValidOnWindows())
	{
		if (CMessageBox::Show(this->m_hWnd, IDS_WARN_NOVALIDPATH, IDS_APPNAME, MB_ICONINFORMATION|MB_YESNO) != IDYES)
			return;
	}

	m_History.AddEntry(m_sLogMessage);
	m_History.Save();

	m_sBugID.Trim();
	if (!m_sBugID.IsEmpty())
	{
		m_sBugID.Replace(_T(", "), _T(","));
		m_sBugID.Replace(_T(" ,"), _T(","));
		CString sBugID = m_ProjectProperties.sMessage;
		sBugID.Replace(_T("%BUGID%"), m_sBugID);
		if (m_ProjectProperties.bAppend)
			m_sLogMessage += _T("\n") + sBugID + _T("\n");
		else
			m_sLogMessage = sBugID + _T("\n") + m_sLogMessage;
		UpdateData(FALSE);
	}
	CResizableStandAloneDialog::OnOK();
}

void CCopyDlg::OnBnClickedBrowse()
{
	m_tooltips.Pop();	// hide the tooltips
	SVNRev rev = SVNRev::REV_HEAD;

	CAppUtils::BrowseRepository(m_URLCombo, this, rev);
}

void CCopyDlg::OnBnClickedHelp()
{
	m_tooltips.Pop();	// hide the tooltips
	OnHelp();
}

void CCopyDlg::OnCancel()
{
	m_bCancelled = true;
	// check if the status thread has already finished
	if (m_pThread)
	{
		WaitForSingleObject(m_pThread->m_hThread, INFINITE);
	}
	if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText()) != 0)
		m_History.AddEntry(m_cLogMessage.GetText());
	m_History.Save();
	CResizableStandAloneDialog::OnCancel();
}

BOOL CCopyDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);

	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_RETURN:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					if ( GetDlgItem(IDOK)->IsWindowEnabled() )
					{
						PostMessage(WM_COMMAND, IDOK);
					}
				}
			}
		}
	}

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CCopyDlg::OnBnClickedBrowsefrom()
{
	m_tooltips.Pop();	// hide the tooltips
	UpdateData(TRUE);
	if (::IsWindow(m_pLogDlg->GetSafeHwnd())&&(m_pLogDlg->IsWindowVisible()))
		return;
	AfxGetApp()->DoWaitCursor(1);
	//now show the log dialog
	if (!m_wcURL.IsEmpty())
	{
		delete m_pLogDlg;
		m_pLogDlg = new CLogDlg();
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\NumberOfLogs"), 100);
		int limit = (int)(LONG)reg;
		m_pLogDlg->SetParams(CTSVNPath(m_wcURL), SVNRev::REV_HEAD, SVNRev::REV_HEAD, 1, limit, TRUE);
		m_pLogDlg->SetSelect(true);
		m_pLogDlg->Create(IDD_LOGMESSAGE, this);
		m_pLogDlg->ShowWindow(SW_SHOW);
		m_pLogDlg->m_wParam = 0;
		m_pLogDlg->m_pNotifyWindow = this;
	}
	AfxGetApp()->DoWaitCursor(-1);
}

void CCopyDlg::OnEnChangeLogmessage()
{
    CString sTemp = m_cLogMessage.GetText();
	DialogEnableWindow(IDOK, sTemp.GetLength() >= m_ProjectProperties.nMinLogSize);
}

LPARAM CCopyDlg::OnRevSelected(WPARAM /*wParam*/, LPARAM lParam)
{
	CString temp;
	temp.Format(_T("%ld"), lParam);
	SetDlgItemText(IDC_COPYREVTEXT, temp);
	CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
	DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
	return 0;
}

void CCopyDlg::OnBnClickedCopyhead()
{
	m_tooltips.Pop();	// hide the tooltips
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
}

void CCopyDlg::OnBnClickedCopyrev()
{
	m_tooltips.Pop();	// hide the tooltips
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
}

void CCopyDlg::OnBnClickedCopywc()
{
	m_tooltips.Pop();	// hide the tooltips
	m_bSettingChanged = true;
	DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
}

void CCopyDlg::OnBnClickedHistory()
{
	m_tooltips.Pop();	// hide the tooltips
	SVN svn;
	CHistoryDlg historyDlg;
	historyDlg.SetHistory(m_History);
	if (historyDlg.DoModal()==IDOK)
	{
		if (historyDlg.GetSelectedText().Compare(m_cLogMessage.GetText().Left(historyDlg.GetSelectedText().GetLength()))!=0)
		{
			if (m_ProjectProperties.sLogTemplate.Compare(m_cLogMessage.GetText())!=0)
				m_cLogMessage.InsertText(historyDlg.GetSelectedText(), !m_cLogMessage.GetText().IsEmpty());
			else
				m_cLogMessage.SetText(historyDlg.GetSelectedText());
		}
		DialogEnableWindow(IDOK, m_ProjectProperties.nMinLogSize <= m_cLogMessage.GetText().GetLength());
	}
}

LPARAM CCopyDlg::OnRevFound(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	// we have found the highest last-committed revision
	// in the working copy
	if ((!m_bSettingChanged)&&(m_maxrev != 0)&&(!m_bCancelled))
	{
		// we only change the setting automatically if the user hasn't done so
		// already him/herself, if the highest revision is valid. And of course,
		// if the thread hasn't been stopped forcefully.
		if (GetCheckedRadioButton(IDC_COPYHEAD, IDC_COPYREV) == IDC_COPYHEAD)
		{
			if (m_bmodified)
			{
				// the working copy has local modifications.
				// show a warning balloon if the user has selected HEAD as the
				// source revision
				ShowBalloon(IDC_COPYHEAD, IDS_WARN_COPYHEADWITHLOCALMODS);
			}
			else
			{
				// and of course, we only change it if the radio button for a REPO-to-REPO copy
				// is enabled for HEAD and if there are no local modifications
				CString temp;
				temp.Format(_T("%ld"), m_maxrev);
				SetDlgItemText(IDC_COPYREVTEXT, temp);
				CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
				DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
			}
		}
	}
	return 0;
}

void CCopyDlg::SetRevision(const SVNRev& rev)
{
	if (rev.IsHead())
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYHEAD);
		DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
	}
	else if (rev.IsWorking())
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYWC);
		DialogEnableWindow(IDC_COPYREVTEXT, FALSE);
	}
	else
	{
		CheckRadioButton(IDC_COPYHEAD, IDC_COPYREV, IDC_COPYREV);
		DialogEnableWindow(IDC_COPYREVTEXT, TRUE);
		CString temp;
		temp.Format(_T("%ld"), (LONG)rev);
		SetDlgItemText(IDC_COPYREVTEXT, temp);
	}
}

void CCopyDlg::OnCbnEditchangeUrlcombo()
{
	m_bSettingChanged = true;
}
