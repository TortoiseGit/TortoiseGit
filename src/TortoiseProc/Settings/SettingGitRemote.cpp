// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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
// SettingGitRemote.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitRemote.h"
#include "Settings.h"
#include "GitAdminDir.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "Git.h"
#include "HistoryCombo.h"

// CSettingGitRemote dialog

IMPLEMENT_DYNAMIC(CSettingGitRemote, ISettingsPropPage)

CSettingGitRemote::CSettingGitRemote()
	: ISettingsPropPage(CSettingGitRemote::IDD)
	, m_bNoFetch(false)
	, m_bPrune(2)
	, m_bPruneAll(FALSE)
	, m_bPushDefault(FALSE)
{
	m_ChangedMask = 0;
}

CSettingGitRemote::~CSettingGitRemote()
{
}

void CSettingGitRemote::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_REMOTE, m_ctrlRemoteList);
	DDX_Text(pDX, IDC_EDIT_REMOTE, m_strRemote);
	DDX_Text(pDX, IDC_EDIT_URL, m_strUrl);
	DDX_Text(pDX, IDC_EDIT_PUSHURL, m_strPushUrl);
	DDX_Text(pDX, IDC_EDIT_PUTTY_KEY, m_strPuttyKeyfile);
	DDX_Control(pDX, IDC_COMBO_TAGOPT, m_ctrlTagOpt);
	DDX_Check(pDX, IDC_CHECK_PRUNE, m_bPrune);
	DDX_Check(pDX, IDC_CHECK_PRUNEALL, m_bPruneAll);
	DDX_Check(pDX, IDC_CHECK_PUSHDEFAULT, m_bPushDefault);
}

BEGIN_MESSAGE_MAP(CSettingGitRemote, CPropertyPage)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_BROWSE, &CSettingGitRemote::OnBnClickedButtonBrowse)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CSettingGitRemote::OnBnClickedButtonAdd)
	ON_LBN_SELCHANGE(IDC_LIST_REMOTE, &CSettingGitRemote::OnLbnSelchangeListRemote)
	ON_EN_CHANGE(IDC_EDIT_REMOTE, &CSettingGitRemote::OnEnChangeEditRemote)
	ON_EN_CHANGE(IDC_EDIT_URL, &CSettingGitRemote::OnEnChangeEditUrl)
	ON_EN_CHANGE(IDC_EDIT_PUSHURL, &CSettingGitRemote::OnEnChangeEditPushUrl)
	ON_EN_CHANGE(IDC_EDIT_PUTTY_KEY, &CSettingGitRemote::OnEnChangeEditPuttyKey)
	ON_CBN_SELCHANGE(IDC_COMBO_TAGOPT, &CSettingGitRemote::OnCbnSelchangeComboTagOpt)
	ON_BN_CLICKED(IDC_CHECK_PRUNE, &CSettingGitRemote::OnBnClickedCheckprune)
	ON_BN_CLICKED(IDC_CHECK_PRUNEALL, &CSettingGitRemote::OnBnClickedCheckpruneall)
	ON_BN_CLICKED(IDC_CHECK_PUSHDEFAULT, &CSettingGitRemote::OnBnClickedCheckpushdefault)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CSettingGitRemote::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_BUTTON_RENAME_REMOTE, &CSettingGitRemote::OnBnClickedButtonRenameRemote)
END_MESSAGE_MAP()

static void ShowEditBalloon(HWND hDlg, UINT nIdControl, UINT nIdText, UINT nIdTitle, int nIcon = TTI_WARNING)
{
	CString text(MAKEINTRESOURCE(nIdText));
	CString title(MAKEINTRESOURCE(nIdTitle));
	EDITBALLOONTIP bt;
	bt.cbStruct = sizeof(bt);
	bt.pszText  = text;
	bt.pszTitle = title;
	bt.ttiIcon  = nIcon;
	SendDlgItemMessage(hDlg, nIdControl, EM_SHOWBALLOONTIP, 0, reinterpret_cast<LPARAM>(&bt));
}

#define TIMER_PREFILL 1

BOOL CSettingGitRemote::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_CHECK_PRUNE);
	AdjustControlSize(IDC_CHECK_PRUNEALL);
	AdjustControlSize(IDC_CHECK_PUSHDEFAULT);

	STRING_VECTOR remotes;
	g_Git.GetRemoteList(remotes);
	m_ctrlRemoteList.ResetContent();
	for (size_t i = 0; i < remotes.size(); i++)
		m_ctrlRemoteList.AddString(remotes[i]);

	m_ctrlTagOpt.AddString(CString(MAKEINTRESOURCE(IDS_FETCH_REACHABLE)));
	m_ctrlTagOpt.AddString(CString(MAKEINTRESOURCE(IDS_NONE)));
	m_ctrlTagOpt.AddString(CString(MAKEINTRESOURCE(IDS_ALL)));
	m_ctrlTagOpt.SetCurSel(0);

	CString pruneAll = g_Git.GetConfigValue(L"fetch.prune");
	m_bPruneAll = pruneAll == L"true" ? TRUE : FALSE;

	{
		CString tmp;
		tmp.Format(IDS_GITCONFIG_SETTING, L"remote.pushdefault");
		m_tooltips.AddTool(IDC_CHECK_PUSHDEFAULT, tmp);
		tmp.Format(IDS_GITCONFIG_SETTING, L"remote.<name>.prune");
		m_tooltips.AddTool(IDC_CHECK_PRUNE, tmp);
		tmp.Format(IDS_GITCONFIG_SETTING, L"fetch.prune");
		m_tooltips.AddTool(IDC_CHECK_PRUNEALL, tmp);
		tmp.Format(IDS_GITCONFIG_SETTING, L"remote<name>.tagopt");
		m_tooltips.AddTool(IDC_COMBO_TAGOPT, tmp);
	}

	//this->GetDlgItem(IDC_EDIT_REMOTE)->EnableWindow(FALSE);
	this->UpdateData(FALSE);

	SetTimer(TIMER_PREFILL, 1000, nullptr);
	return TRUE;
}
// CSettingGitRemote message handlers

void CSettingGitRemote::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == TIMER_PREFILL)
	{
		if (m_strRemote.IsEmpty() && m_ctrlRemoteList.GetCount() == 0)
			ShowEditBalloon(IDC_EDIT_URL, IDS_B_T_PREFILL_ORIGIN, IDS_HINT, TTI_INFO);

		KillTimer(TIMER_PREFILL);
	}
}

void CSettingGitRemote::OnBnClickedButtonBrowse()
{
	UpdateData();
	CString filename = m_strPuttyKeyfile;
	if (!PathFileExists(filename))
		filename.Empty();
	if (!CAppUtils::FileOpenSave(filename, nullptr, 0, IDS_PUTTYKEYFILEFILTER, true, GetSafeHwnd()))
		return;

	m_strPuttyKeyfile = filename;
	UpdateData(FALSE);
	OnEnChangeEditPuttyKey();
}

void CSettingGitRemote::OnBnClickedButtonAdd()
{
	this->UpdateData();

	if(m_strRemote.IsEmpty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_REMOTEEMPTY, IDS_APPNAME, MB_OK |  MB_ICONERROR);
		return;
	}
	if(m_strUrl.IsEmpty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	m_ChangedMask = REMOTE_NAME | REMOTE_URL | REMOTE_PUTTYKEY | REMOTE_TAGOPT | REMOTE_PRUNE | REMOTE_PRUNEALL | REMOTE_PUSHDEFAULT | REMOTE_PUSHURL;
	if(IsRemoteExist(m_strRemote))
	{
		CString msg;
		msg.Format(IDS_PROC_GITCONFIG_OVERWRITEREMOTE, static_cast<LPCTSTR>(m_strRemote));
		if (CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			m_ChangedMask &= ~REMOTE_NAME;
		else
			return;
	}

	this->OnApply();
}

BOOL CSettingGitRemote::IsRemoteExist(CString &remote)
{
	CString str;
	for(int i=0;i<m_ctrlRemoteList.GetCount();i++)
	{
		m_ctrlRemoteList.GetText(i,str);
		if(str == remote)
			return true;
	}
	return false;
}

struct CheckRefspecStruct
{
	CStringA remote;
	bool result;
};

static int CheckRemoteCollideWithRefspec(const git_config_entry *entry, void * payload)
{
	auto crs = reinterpret_cast<CheckRefspecStruct*>(payload);
	crs->result = false;
	if (entry->name == "remote." + crs->remote + ".fetch")
		return 0;
	CStringA value = CStringA(entry->value);
	CStringA match = ":refs/remotes/" + crs->remote;
	int pos = value.Find(match);
	if (pos < 0)
		return 0;
	if ((pos + match.GetLength() == value.GetLength()) || (value[pos + match.GetLength()] == '/'))
	{
		crs->result = true;
		return GIT_EUSER;
	}
	return 0;
}

bool CSettingGitRemote::IsRemoteCollideWithRefspec(CString remote)
{
	CheckRefspecStruct crs = { CUnicodeUtils::GetUTF8(remote), false };
	CAutoConfig config(true);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, g_Git.GetGitRepository(), FALSE);
	for (const auto pattern : { "remote\\..*\\.fetch", "svn-remote\\..*\\.fetch", "svn-remote\\..*\\.branches", "svn-remote\\..*\\.tags" })
	{
		git_config_foreach_match(config, pattern, CheckRemoteCollideWithRefspec, &crs);
		if (crs.result)
			return true;
	}
	return false;
}

void CSettingGitRemote::OnLbnSelchangeListRemote()
{
	CWaitCursor wait;

	if(m_ChangedMask)
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_SAVEREMOTE, IDS_APPNAME, 1, IDI_QUESTION, IDS_SAVEBUTTON, IDS_DISCARDBUTTON) == 1)
			OnApply();
	}
	SetModified(FALSE);

	int index = this->m_ctrlRemoteList.GetCurSel();
	if(index<0)
	{
		m_strUrl.Empty();
		m_strPushUrl.Empty();
		m_strRemote.Empty();
		m_strPuttyKeyfile.Empty();
		this->UpdateData(FALSE);
		return;
	}
	m_ctrlRemoteList.GetText(index, m_strRemote);

	CString cmd, output;
	cmd.Format(L"remote.%s.url", static_cast<LPCTSTR>(m_strRemote));
	m_strUrl = g_Git.GetConfigValue(cmd);

	cmd.Format(L"remote.%s.pushurl", static_cast<LPCTSTR>(m_strRemote));
	m_strPushUrl = g_Git.GetConfigValue(cmd);

	cmd.Format(L"remote.%s.puttykeyfile", static_cast<LPCTSTR>(m_strRemote));

	this->m_strPuttyKeyfile = g_Git.GetConfigValue(cmd);

	m_ChangedMask=0;


	cmd.Format(L"remote.%s.tagopt", static_cast<LPCTSTR>(m_strRemote));
	CString tagopt = g_Git.GetConfigValue(cmd);
	index = 0;
	if (tagopt == "--no-tags")
		index = 1;
	else if (tagopt == "--tags")
		index = 2;
	m_ctrlTagOpt.SetCurSel(index);

	CString pushDefault = g_Git.GetConfigValue(L"remote.pushdefault");
	m_bPushDefault = pushDefault == m_strRemote ? TRUE : FALSE;
	cmd.Format(L"remote.%s.prune", static_cast<LPCTSTR>(m_strRemote));
	CString prune = g_Git.GetConfigValue(cmd);
	m_bPrune = prune == L"true" ? TRUE : prune == L"false" ? FALSE : 2;
	CString pruneAll = g_Git.GetConfigValue(L"fetch.prune");
	m_bPruneAll = pruneAll == L"true" ? TRUE : FALSE;

	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_RENAME_REMOTE)->EnableWindow(TRUE);
	this->UpdateData(FALSE);
}

void CSettingGitRemote::OnEnChangeEditRemote()
{
	m_ChangedMask|=REMOTE_NAME;

	this->UpdateData();

	if (IsRemoteCollideWithRefspec(m_strRemote))
		ShowEditBalloon(IDC_EDIT_REMOTE, IDS_B_T_REMOTE_NAME_COLLIDE, IDS_HINT, TTI_WARNING);
	if( (!this->m_strRemote.IsEmpty())&&(!this->m_strUrl.IsEmpty()) )
		this->SetModified();
}

void CSettingGitRemote::OnEnChangeEditUrl()
{
	m_ChangedMask|=REMOTE_URL;

	this->UpdateData();

	if (m_strRemote.IsEmpty() && !m_strUrl.IsEmpty() && m_ctrlRemoteList.GetCount() == 0)
	{
		GetDlgItem(IDC_EDIT_REMOTE)->SetWindowText(L"origin");
		OnEnChangeEditRemote();
	}

	if( (!this->m_strRemote.IsEmpty())&&(!this->m_strUrl.IsEmpty()) )
		this->SetModified();
}

void CSettingGitRemote::OnEnChangeEditPushUrl()
{
	m_ChangedMask |= REMOTE_PUSHURL;

	this->UpdateData();

	if (!this->m_strRemote.IsEmpty())
		this->SetModified();
}

void CSettingGitRemote::OnEnChangeEditPuttyKey()
{
	m_ChangedMask|=REMOTE_PUTTYKEY;

	this->UpdateData();
	if (!this->m_strUrl.IsEmpty())
		this->SetModified();
}

void CSettingGitRemote::OnCbnSelchangeComboTagOpt()
{
	m_ChangedMask |= REMOTE_TAGOPT;

	this->UpdateData();
	this->SetModified();
}

void CSettingGitRemote::OnBnClickedCheckprune()
{
	m_ChangedMask |= REMOTE_PRUNE;

	this->UpdateData();
	this->SetModified();
}

void CSettingGitRemote::OnBnClickedCheckpruneall()
{
	m_ChangedMask |= REMOTE_PRUNEALL;
	UpdateData();
	SetModified();
}

void CSettingGitRemote::OnBnClickedCheckpushdefault()
{
	m_ChangedMask |= REMOTE_PUSHDEFAULT;
	UpdateData();
	SetModified();
}

BOOL CSettingGitRemote::Save(CString key,CString value)
{
	CString cmd,out;

	cmd.Format(L"remote.%s.%s", static_cast<LPCTSTR>(m_strRemote), static_cast<LPCTSTR>(key));
	if (value.IsEmpty())
	{
		// don't check result code. it fails if the entry not exist
		g_Git.UnsetConfigValue(cmd, CONFIG_LOCAL);
		if (!g_Git.GetConfigValue(cmd).IsEmpty())
		{
			CString msg;
			msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(cmd), static_cast<LPCTSTR>(value));
			CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		return TRUE;
	}

	if (g_Git.SetConfigValue(cmd, value, CONFIG_LOCAL))
	{
		CString msg;
		msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(cmd), static_cast<LPCTSTR>(value));
		CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	return TRUE;
}

BOOL CSettingGitRemote::SaveGeneral(CString key, CString value)
{
	if (value.IsEmpty())
	{
		// don't check result code. it fails if the entry not exist
		g_Git.UnsetConfigValue(key, CONFIG_LOCAL);
		if (!g_Git.GetConfigValue(key).IsEmpty())
		{
			CString msg;
			msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(key), static_cast<LPCTSTR>(value));
			CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		return TRUE;
	}

	if (g_Git.SetConfigValue(key, value, CONFIG_LOCAL))
	{
		CString msg;
		msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(key), static_cast<LPCTSTR>(value));
		CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return FALSE;
	}

	return TRUE;
}

BOOL CSettingGitRemote::OnApply()
{
	CWaitCursor wait;
	this->UpdateData();

	if (m_ChangedMask & REMOTE_PUSHDEFAULT)
	{
		if (!m_strRemote.Trim().IsEmpty() && m_bPushDefault)
		{
			if (!SaveGeneral(L"remote.pushdefault", m_strRemote.Trim()))
				return FALSE;
		}
		if (!m_bPushDefault)
		{
			if (!SaveGeneral(L"remote.pushdefault", L""))
				return FALSE;
		}

		m_ChangedMask &= ~REMOTE_PUSHDEFAULT;
	}

	if (m_ChangedMask & REMOTE_PRUNEALL)
	{
		if (!SaveGeneral(L"fetch.prune", m_bPruneAll == TRUE ? L"true" : L""))
			return FALSE;
		m_ChangedMask &= ~REMOTE_PRUNEALL;
	}

	if (m_ChangedMask && m_strRemote.Trim().IsEmpty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_REMOTEEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return FALSE;
	}

	if(m_ChangedMask & REMOTE_NAME)
	{
		//Add Remote
		if(m_strRemote.IsEmpty())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_REMOTEEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return FALSE;
		}
		if(m_strUrl.IsEmpty())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_PROC_GITCONFIG_URLEMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return FALSE;
		}

		if (m_ctrlRemoteList.GetCount() > 0)
		{
			// tagopt not --no-tags
			if (m_ctrlTagOpt.GetCurSel() != 1)
			{
				if (CMessageBox::ShowCheck(GetSafeHwnd(), IDS_PROC_GITCONFIG_ASKTAGOPT, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION, L"TagOptNoTagsWarning", IDS_MSGBOX_DONOTSHOWAGAIN) == IDYES)
				{
					m_ctrlTagOpt.SetCurSel(1);
					m_ChangedMask |= REMOTE_TAGOPT;
				}
			}
		}

		m_strUrl.Replace(L'\\', L'/');
		CString cmd,out;
		cmd.Format(L"git.exe remote add \"%s\" \"%s\"", static_cast<LPCTSTR>(m_strRemote), static_cast<LPCTSTR>(m_strUrl));
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			CMessageBox::Show(GetSafeHwnd(), out, L"TorotiseGit", MB_OK | MB_ICONERROR);
			return FALSE;
		}
		m_ChangedMask &= ~REMOTE_URL;

		m_ctrlRemoteList.SetCurSel(m_ctrlRemoteList.AddString(m_strRemote));
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_RENAME_REMOTE)->EnableWindow(TRUE);
		if (!m_bNoFetch && CMessageBox::Show(GetSafeHwnd(), IDS_SETTINGS_FETCH_ADDEDREMOTE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO) == IDYES)
			CCommonAppUtils::RunTortoiseGitProc(L"/command:fetch /path:\"" + g_Git.m_CurrentDir + L"\" /remote:\"" + m_strRemote + L'"');
	}
	if(m_ChangedMask & REMOTE_URL)
	{
		m_strUrl.Replace(L'\\', L'/');
		if (!Save(L"url", m_strUrl))
			return FALSE;
	}

	if(m_ChangedMask & REMOTE_PUTTYKEY)
	{
		if (!Save(L"puttykeyfile", m_strPuttyKeyfile))
			return FALSE;
	}

	if (m_ChangedMask & REMOTE_TAGOPT)
	{
		CString tagopt;
		int index = m_ctrlTagOpt.GetCurSel();
		if (index == 1)
			tagopt = "--no-tags";
		else if (index == 2)
			tagopt = "--tags";
		if (!Save(L"tagopt", tagopt))
			return FALSE;
	}

	if (m_ChangedMask & REMOTE_PRUNE)
	{
		if (!Save(L"prune", m_bPrune == TRUE ? L"true" : m_bPrune == FALSE ? L"false" : L""))
			return FALSE;
	}

	if (m_ChangedMask & REMOTE_PUSHURL)
	{
		m_strPushUrl.Replace(L'\\', L'/');
		if (!Save(L"pushurl", m_strPushUrl))
			return FALSE;
	}

	SetModified(FALSE);

	m_ChangedMask = 0;
	return ISettingsPropPage::OnApply();
}

void CleanupSyncRemotes(const CString& remote)
{
	CString workingDir = g_Git.m_CurrentDir;
	workingDir.Replace(L':', L'_');
	CHistoryCombo::RemoveEntryFromHistory(L"Software\\TortoiseGit\\History\\SyncURL\\" + workingDir, L"url", remote);
}

void CSettingGitRemote::OnBnClickedButtonRemove()
{
	int index = m_ctrlRemoteList.GetCurSel();
	if(index>=0)
	{
		CString str;
		m_ctrlRemoteList.GetText(index,str);
		CString msg;
		msg.Format(IDS_WARN_REMOVE, static_cast<LPCTSTR>(str));
		if (CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CString cmd,out;
			cmd.Format(L"git.exe remote rm %s", static_cast<LPCTSTR>(str));
			if (g_Git.Run(cmd, &out, CP_UTF8))
			{
				CMessageBox::Show(GetSafeHwnd(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
				return;
			}

			CleanupSyncRemotes(str);
			m_ctrlRemoteList.DeleteString(index);
			OnLbnSelchangeListRemote();
		}
	}
}

void CSettingGitRemote::OnBnClickedButtonRenameRemote()
{
	int sel = m_ctrlRemoteList.GetCurSel();
	if (sel >= 0)
	{
		CString oldRemote, newRemote;
		m_ctrlRemoteList.GetText(sel, oldRemote);
		GetDlgItem(IDC_EDIT_REMOTE)->GetWindowText(newRemote);
		CString cmd, out;
		cmd.Format(L"git.exe remote rename %s %s", static_cast<LPCTSTR>(oldRemote), static_cast<LPCTSTR>(newRemote));
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			CMessageBox::Show(GetSafeHwnd(), out, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return;
		}

		CleanupSyncRemotes(oldRemote);
		m_ctrlRemoteList.DeleteString(sel);
		m_ctrlRemoteList.SetCurSel(m_ctrlRemoteList.AddString(newRemote));
		m_ChangedMask &= ~REMOTE_NAME;
		if (!m_ChangedMask)
			this->SetModified(FALSE);
	}
}
