// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2019 - TortoiseGit

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
// SettingGitCredential.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingGitCredential.h"
#include "Settings.h"
#include "GitAdminDir.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "Git.h"
#include "PathUtils.h"

namespace SimpleCredentialType
{
	static int Advanced = -1;
	static int None = -1;
	static int LocalWincred = -1;
	static int LocalWinstore = -1;
	static int LocalGCM = -1;
	static int GlobalWincred = -1;
	static int GlobalWinstore = -1;
	static int GlobalGCM = -1;
	static int SystemWincred = -1;
	static int SystemGCM = -1;
}

namespace ConfigType
{
	static int Local = -1;
	static int Global = -1;
	static int System = -1;
}

// CSettingGitCredential dialog

IMPLEMENT_DYNAMIC(CSettingGitCredential, ISettingsPropPage)

CSettingGitCredential::CSettingGitCredential()
	: ISettingsPropPage(CSettingGitCredential::IDD)
	, m_bUseHttpPath(FALSE)
	, m_iSimpleStoredValue(-2)
{
	m_ChangedMask = 0;
}

CSettingGitCredential::~CSettingGitCredential()
{
}

void CSettingGitCredential::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_SIMPLECREDENTIAL, m_ctrlSimpleCredential);
	DDX_Control(pDX, IDC_LIST_REMOTE, m_ctrlUrlList);
	DDX_Text(pDX, IDC_EDIT_URL, m_strUrl);
	DDX_Text(pDX, IDC_COMBO_HELPER, m_strHelper);
	DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUsername);
	DDX_Check(pDX, IDC_CHECK_USEHTTPPATH, m_bUseHttpPath);
	DDX_Control(pDX, IDC_COMBO_CONFIGTYPE, m_ctrlConfigType);
}


BEGIN_MESSAGE_MAP(CSettingGitCredential, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_COMBO_SIMPLECREDENTIAL, &CSettingGitCredential::OnCbnSelchangeComboSimplecredential)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CSettingGitCredential::OnBnClickedButtonAdd)
	ON_LBN_SELCHANGE(IDC_LIST_REMOTE, &CSettingGitCredential::OnLbnSelchangeListUrl)
	ON_CBN_SELCHANGE(IDC_COMBO_CONFIGTYPE, &CSettingGitCredential::OnCbnSelchangeComboConfigType)
	ON_EN_CHANGE(IDC_EDIT_URL, &CSettingGitCredential::OnEnChangeEditUrl)
	ON_CBN_EDITCHANGE(IDC_COMBO_HELPER, &CSettingGitCredential::OnEnChangeEditHelper)
	ON_CBN_SELCHANGE(IDC_COMBO_HELPER, &CSettingGitCredential::OnEnChangeEditHelper)
	ON_EN_CHANGE(IDC_EDIT_USERNAME, &CSettingGitCredential::OnEnChangeEditUsername)
	ON_BN_CLICKED(IDC_CHECK_USEHTTPPATH, &CSettingGitCredential::OnBnClickedCheckUsehttppath)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CSettingGitCredential::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDC_OPENSETTINGSELEVATED, &CSettingGitCredential::OnBnClickedOpensettingselevated)
END_MESSAGE_MAP()

static CStringA RegexEscape(CStringA str)
{
	CStringA result;
	for (int i = 0; i < str.GetLength(); ++i)
	{
		char c = str[i];
		switch (c)
		{
			case '\\': case '*': case '+': case '?': case '|':
			case '{': case '[': case '(': case ')': case '^':
			case '$': case '.': case '#': case ' ':
				result.AppendChar('\\');
				result.AppendChar(c);
				break;
			case '\t': result.Append("\\t"); break;
			case '\n': result.Append("\\n"); break;
			case '\r': result.Append("\\r"); break;
			case '\f': result.Append("\\f"); break;
			default: result.AppendChar(c); break;
		}
	}

	return result;
}

BOOL CSettingGitCredential::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_CHECK_USEHTTPPATH);

	bool hasLocal = GitAdminDir::HasAdminDir(g_Git.m_CurrentDir);

	m_ctrlUrlList.ResetContent();

	AddConfigType(ConfigType::Local, CString(MAKEINTRESOURCE(IDS_SETTINGS_LOCAL)), hasLocal);
	AddConfigType(ConfigType::Global, CString(MAKEINTRESOURCE(IDS_SETTINGS_GLOBAL)));
	AddConfigType(ConfigType::System, CString(MAKEINTRESOURCE(IDS_SETTINGS_SYSTEM)));
	m_ctrlConfigType.SetCurSel(0);

	if (WincredExists())
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_HELPER))->AddString(L"wincred");
	if (WinstoreExists())
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_HELPER))->AddString(GetWinstorePath());
	if (GCMExists())
		static_cast<CComboBox*>(GetDlgItem(IDC_COMBO_HELPER))->AddString(L"manager");

	LoadList();
	m_iSimpleStoredValue = m_ctrlSimpleCredential.GetCurSel();

	if (!CAppUtils::IsAdminLogin())
	{
		reinterpret_cast<CButton*>(GetDlgItem(IDC_OPENSETTINGSELEVATED))->SetShield(TRUE);
		GetDlgItem(IDC_OPENSETTINGSELEVATED)->ShowWindow(SW_SHOW);
		GetDlgItem(IDC_STATICELEVATIONNEEDED)->ShowWindow(SW_SHOW);
	}

	EnableAdvancedOptions();

	UpdateData(FALSE);
	return TRUE;
}
// CSettingGitCredential message handlers

void CSettingGitCredential::OnCbnSelchangeComboSimplecredential()
{
	EnableAdvancedOptions();
	SetModified();
	m_ChangedMask |= CREDENTIAL_SIMPLE;
}

void CSettingGitCredential::OnBnClickedButtonAdd()
{
	UpdateData();

	if (m_strHelper.Trim().IsEmpty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_GITCREDENTIAL_HELPEREMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	m_ChangedMask = CREDENTIAL_URL | CREDENTIAL_HELPER | CREDENTIAL_USERNAME | CREDENTIAL_USEHTTPPATH;
	int sel = m_ctrlConfigType.GetCurSel();
	CString prefix = sel == ConfigType::System ? L"S" : sel == ConfigType::Global ? L"G" : L"L";
	CString text;
	if (!m_strUrl.IsEmpty())
		text.Format(L"%s:%s", static_cast<LPCTSTR>(prefix), static_cast<LPCTSTR>(m_strUrl));
	else
		text = prefix;
	if (IsUrlExist(text))
	{
		CString msg;
		msg.Format(IDS_GITCREDENTIAL_OVERWRITEHELPER, static_cast<LPCTSTR>(m_strUrl));
		if (CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
			m_ChangedMask &= ~CREDENTIAL_URL;
		else
			return;
	}
	else
	{
		if (m_strUsername.IsEmpty())
			m_ChangedMask &= ~CREDENTIAL_USERNAME;
		if (!m_bUseHttpPath)
			m_ChangedMask &= ~CREDENTIAL_USEHTTPPATH;
	}

	SaveSettings();
}

void CSettingGitCredential::EnableAdvancedOptions()
{
	BOOL enable = m_ctrlSimpleCredential.GetCurSel() == SimpleCredentialType::Advanced ? TRUE : FALSE;
	GetDlgItem(IDC_LIST_REMOTE)->EnableWindow(enable);
	GetDlgItem(IDC_EDIT_URL)->EnableWindow(enable);
	GetDlgItem(IDC_COMBO_HELPER)->EnableWindow(enable);
	GetDlgItem(IDC_EDIT_USERNAME)->EnableWindow(enable);
	GetDlgItem(IDC_COMBO_CONFIGTYPE)->EnableWindow(enable);
	GetDlgItem(IDC_CHECK_USEHTTPPATH)->EnableWindow(enable);

	bool canModifySystem = CAppUtils::IsAdminLogin() || (m_ctrlConfigType.GetCurSel() != ConfigType::System);
	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(enable && canModifySystem);
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(enable && canModifySystem);
}

BOOL CSettingGitCredential::IsUrlExist(CString &text)
{
	CString str;
	for(int i = 0; i < m_ctrlUrlList.GetCount();i++)
	{
		m_ctrlUrlList.GetText(i, str);
		if (str == text)
			return true;
	}
	return false;
}

void CSettingGitCredential::OnLbnSelchangeListUrl()
{
	CWaitCursor wait;

	if (m_ChangedMask & CREDENTIAL_ADVANCED_MASK)
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_GITCREDENTIAL_SAVEHELPER, IDS_APPNAME, 1, IDI_QUESTION, IDS_SAVEBUTTON, IDS_DISCARDBUTTON) == 1)
			SaveSettings();
	}
	m_ChangedMask &= ~CREDENTIAL_ADVANCED_MASK;
	if (!m_ChangedMask)
		SetModified(FALSE);

	CString cmd, output;
	int index = m_ctrlUrlList.GetCurSel();
	if (index < 0)
	{
		m_strHelper.Empty();
		m_strUrl.Empty();
		m_strHelper.Empty();
		m_bUseHttpPath = FALSE;
		UpdateData(FALSE);
		return;
	}

	CString text;
	m_ctrlUrlList.GetText(index, text);
	int pos = text.Find(L':');
	CString prefix = pos >= 0 ? text.Left(pos) : text.Left(1);
	m_ctrlConfigType.SetCurSel((prefix == L"S" || prefix == L"P") ? ConfigType::System : prefix == L"X" ? ConfigType::Global : prefix == L"G" ? ConfigType::Global : ConfigType::Local);
	m_strUrl = pos >= 0 ? text.Mid(pos + 1) : L"";

	m_strHelper = Load(L"helper");
	m_strUsername = Load(L"username");
	m_bUseHttpPath = Load(L"useHttpPath") == L"true" ? TRUE : FALSE;

	bool canModifySystem = CAppUtils::IsAdminLogin() || (m_ctrlConfigType.GetCurSel() != ConfigType::System);
	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(canModifySystem ? TRUE : FALSE);
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(canModifySystem ? TRUE : FALSE);
	this->UpdateData(FALSE);
}

void CSettingGitCredential::OnCbnSelchangeComboConfigType()
{
	if (!CAppUtils::IsAdminLogin())
	{
		int sel = m_ctrlConfigType.GetCurSel();
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(sel != ConfigType::System);
		GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(sel != ConfigType::System);
		if (sel == ConfigType::System)
			return;
	}
	SetModified();
}

void CSettingGitCredential::OnEnChangeEditUrl()
{
	m_ChangedMask |= CREDENTIAL_URL;
	SetModified();
}

void CSettingGitCredential::OnEnChangeEditHelper()
{
	m_ChangedMask |= CREDENTIAL_HELPER;

	UpdateData();
	if (!m_strHelper.IsEmpty())
		SetModified();
	else if (!m_ChangedMask)
		SetModified(0);
}

void CSettingGitCredential::OnEnChangeEditUsername()
{
	m_ChangedMask |= CREDENTIAL_USERNAME;
	SetModified();
}

void CSettingGitCredential::OnBnClickedCheckUsehttppath()
{
	m_ChangedMask |= CREDENTIAL_USEHTTPPATH;
	SetModified();
}

void CSettingGitCredential::AddConfigType(int &index, CString text, bool add)
{
	if (add)
		index = m_ctrlConfigType.AddString(text);
}

void CSettingGitCredential::AddSimpleCredential(int &index, CString text, bool add)
{
	if (add)
		index = m_ctrlSimpleCredential.AddString(text);
}

void CSettingGitCredential::FillSimpleList(bool anySystem, bool systemWincred, bool systemGCM)
{
	ATLASSERT(SimpleCredentialType::Advanced == -1);

	bool hasLocal = GitAdminDir::HasAdminDir(g_Git.m_CurrentDir);
	bool isAdmin = CAppUtils::IsAdminLogin();

	AddSimpleCredential(SimpleCredentialType::Advanced, CString(MAKEINTRESOURCE(IDS_ADVANCED)));
	if (isAdmin || !anySystem)
		AddSimpleCredential(SimpleCredentialType::None, CString(MAKEINTRESOURCE(IDS_NONE)));
	if (isAdmin || !(systemWincred || systemGCM || anySystem))
	{
		AddSimpleCredential(SimpleCredentialType::LocalWincred, CString(MAKEINTRESOURCE(IDS_LOCAL_WINCRED)), hasLocal && WincredExists());
		AddSimpleCredential(SimpleCredentialType::LocalWinstore, CString(MAKEINTRESOURCE(IDS_LOCAL_WINSTORE)), hasLocal && WinstoreExists());
		AddSimpleCredential(SimpleCredentialType::LocalGCM, CString(MAKEINTRESOURCE(IDS_LOCAL_GCM)), hasLocal && GCMExists());
		AddSimpleCredential(SimpleCredentialType::GlobalWincred, CString(MAKEINTRESOURCE(IDS_GLOBAL_WINCRED)), WincredExists());
		AddSimpleCredential(SimpleCredentialType::GlobalWinstore, CString(MAKEINTRESOURCE(IDS_GLOBAL_WINSTORE)), WinstoreExists());
		AddSimpleCredential(SimpleCredentialType::GlobalGCM, CString(MAKEINTRESOURCE(IDS_GLOBAL_GCM)), GCMExists());
	}
	if (isAdmin || systemWincred)
		AddSimpleCredential(SimpleCredentialType::SystemWincred, CString(MAKEINTRESOURCE(IDS_SYSTEM_WINCRED)), WincredExists());
	if (isAdmin || systemGCM)
		AddSimpleCredential(SimpleCredentialType::SystemGCM, CString(MAKEINTRESOURCE(IDS_SYSTEM_GCM)), GCMExists());
}

void CSettingGitCredential::LoadList()
{
	CAutoConfig config(true);
	CAutoRepository repo(g_Git.GetGitRepository());
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, repo, FALSE);
	if (!g_Git.ms_bCygwinGit && !g_Git.ms_bMsys2Git)
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitProgramDataConfig()), GIT_CONFIG_LEVEL_PROGRAMDATA, repo, FALSE);

	STRING_VECTOR defaultList, urlList;
	git_config_foreach_match(config, "credential\\.helper", GetCredentialDefaultUrlCallback, &defaultList);
	git_config_foreach_match(config, "credential\\..*\\.helper", GetCredentialUrlCallback, &urlList);
	STRING_VECTOR anyList;
	git_config_foreach_match(config, "credential\\..*", GetCredentialAnyEntryCallback, &anyList);

	for (size_t i = 0; i < defaultList.size(); ++i)
		m_ctrlUrlList.AddString(defaultList[i]);
	for (size_t i = 0; i < urlList.size(); ++i)
		m_ctrlUrlList.AddString(urlList[i]);

	if (anyList.empty())
	{
		FillSimpleList(false, false, false);
		m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::None);
		return;
	}
	if (anyList.size() > 1)
	{
		bool hasSystemType = false;
		for (size_t i = 0; i < anyList.size(); ++i)
		{
			int pos = 0;
			CString prefix = anyList[i].Tokenize(L"\n", pos);
			if (prefix == L"S" || prefix == L"P")
			{
				hasSystemType = true;
				break;
			}
		}
		FillSimpleList(hasSystemType, false, false);
		m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::Advanced);
		return;
	}

	int pos = 0;
	CString prefix = anyList[0].Tokenize(L"\n", pos);
	CString key = anyList[0].Tokenize(L"\n", pos);
	CString value = anyList[0].Tokenize(L"\n", pos);
	if (key != L"credential.helper")
	{
		FillSimpleList(true, false, false);
		m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::Advanced);
		return;
	}

	CString winstore = GetWinstorePath();
	if (prefix == L"L")
	{
		if (value == L"wincred")
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::LocalWincred);
			return;
		}
		else if (value == winstore)
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::LocalWinstore);
			return;
		}
		else if (value == L"manager")
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::LocalGCM);
			return;
		}
	}

	if (prefix == L"G" || prefix == L"X")
	{
		if (value == L"wincred")
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::GlobalWincred);
			return;
		}
		else if (value == winstore)
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::GlobalWinstore);
			return;
		}
		else if (value == L"manager")
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::GlobalGCM);
			return;
		}
	}

	if (prefix == L"S" || prefix == L"P")
	{
		if (value == L"wincred")
		{
			FillSimpleList(true, true, false);
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::SystemWincred);
			return;
		}
		else if (value == L"manager")
		{
			FillSimpleList(true, false, true);
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::SystemGCM);
			return;
		}
	}

	FillSimpleList((prefix == L"S" || prefix == L"P"), false, false);
	m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::Advanced);
}

CString CSettingGitCredential::Load(CString key)
{
	CString cmd;

	if (m_strUrl.IsEmpty())
		cmd.Format(L"credential.%s", static_cast<LPCTSTR>(key));
	else
		cmd.Format(L"credential.%s.%s", static_cast<LPCTSTR>(m_strUrl), static_cast<LPCTSTR>(key));

	CAutoConfig config(true);
	CAutoRepository repo(g_Git.GetGitRepository());
	int sel = m_ctrlConfigType.GetCurSel();
	if (sel == ConfigType::Local)
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, repo, FALSE);
	else if (sel == ConfigType::Global)
	{
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, repo, FALSE);
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, repo, FALSE);
	}
	else if (sel == ConfigType::System)
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, repo, FALSE);

	CStringA cmdA = CUnicodeUtils::GetUTF8(cmd);
	CStringA valueA;
	git_config_entry* entry;
	if (!git_config_get_entry(&entry, config, cmdA))
		valueA = CStringA(entry->value);
	git_config_entry_free(entry);

	return CUnicodeUtils::GetUnicode(valueA);
}

void CSettingGitCredential::Save(CString key, CString value)
{
	CString cmd, out;

	if (m_strUrl.IsEmpty())
		cmd.Format(L"credential.%s", static_cast<LPCTSTR>(key));
	else
		cmd.Format(L"credential.%s.%s", static_cast<LPCTSTR>(m_strUrl), static_cast<LPCTSTR>(key));

	int sel = m_ctrlConfigType.GetCurSel();
	CONFIG_TYPE configLevel = sel == ConfigType::System ? CONFIG_SYSTEM : sel == ConfigType::Global ? CONFIG_GLOBAL : CONFIG_LOCAL;

	bool old = g_Git.m_IsUseGitDLL;
	// workaround gitdll bug
	// TODO: switch to libgit2
	g_Git.m_IsUseGitDLL = false;
	if (g_Git.SetConfigValue(cmd, value, configLevel))
	{
		CString msg;
		msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(cmd), static_cast<LPCTSTR>(value));
		CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
	}
	if (value.IsEmpty())
	{
		if (g_Git.UnsetConfigValue(cmd, configLevel))
		{
			CString msg;
			msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(cmd), static_cast<LPCTSTR>(value));
			CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
		}
	}
	g_Git.m_IsUseGitDLL = old;
}

int CSettingGitCredential::DeleteOtherKeys(int type)
{
	CString match;
	if (type == SimpleCredentialType::LocalWincred)
		match = L"L\ncredential.helper\nwincred";
	else if (type == SimpleCredentialType::LocalWinstore)
		match = L"L\ncredential.helper\n" + GetWinstorePath();
	else if (type == SimpleCredentialType::LocalGCM)
		match = L"L\ncredential.helper\nmanager";
	else if (type == SimpleCredentialType::GlobalWincred)
		match = L"G\ncredential.helper\nwincred";
	else if (type == SimpleCredentialType::GlobalWinstore)
		match = L"G\ncredential.helper\n" + GetWinstorePath();
	else if (type == SimpleCredentialType::GlobalGCM)
		match = L"G\ncredential.helper\nmanager";
	else if (type == SimpleCredentialType::SystemWincred)
		match = L"S\ncredential.helper\nwincred";
	else if (type == SimpleCredentialType::SystemGCM)
		match = L"S\ncredential.helper\nmanager";

	CAutoConfig config(true);
	CAutoRepository repo = g_Git.GetGitRepository();
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, repo, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, repo, FALSE);

	STRING_VECTOR list;
	git_config_foreach_match(config, "credential\\..*", GetCredentialAnyEntryCallback, &list);
	for (size_t i = 0; i < list.size(); ++i)
	{
		int pos = 0;
		CString prefix = list[i].Tokenize(L"\n", pos);
		if ((prefix == L"S" || prefix == L"P") && list[i] != match && !CAppUtils::IsAdminLogin())
		{
			if (MessageBox(L"Cannot modify a system config without proper rights.\nDiscard changes?", L"TortoiseGit", MB_ICONERROR | MB_YESNO) == IDYES)
				return 0;
			return -1;
		}
	}

	// workaround gitdll bug
	// TODO: switch to libgit2
	bool old = g_Git.m_IsUseGitDLL;
	g_Git.m_IsUseGitDLL = false;
	SCOPE_EXIT { g_Git.m_IsUseGitDLL = old; };
	for (size_t i = 0; i < list.size(); ++i)
	{
		if (list[i] == match)
			continue;

		int pos = 0;
		CString prefix = list[i].Tokenize(L"\n", pos);
		CString key = list[i].Tokenize(L"\n", pos);
		CONFIG_TYPE configLevel = (prefix == L"S" || prefix == L"P") ? CONFIG_SYSTEM : prefix == L"G" || prefix == L"X" ? CONFIG_GLOBAL : CONFIG_LOCAL;
		if (g_Git.UnsetConfigValue(key, configLevel))
		{
			CString msg;
			msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, static_cast<LPCTSTR>(key), L"");
			CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
			return 1;
		}
	}

	return 0;
}

bool CSettingGitCredential::SaveSimpleCredential(int type)
{
	CONFIG_TYPE configLevel;
	CString value;
	if (type == SimpleCredentialType::LocalWincred)
	{
		configLevel = CONFIG_LOCAL;
		value = L"wincred";
	}
	else if (type == SimpleCredentialType::LocalWinstore)
	{
		configLevel = CONFIG_LOCAL;
		value = GetWinstorePath();
	}
	else if (type == SimpleCredentialType::LocalGCM)
	{
		configLevel = CONFIG_LOCAL;
		value = L"manager";
	}
	else if (type == SimpleCredentialType::GlobalWincred)
	{
		configLevel = CONFIG_GLOBAL;
		value = L"wincred";
	}
	else if (type == SimpleCredentialType::GlobalWinstore)
	{
		configLevel = CONFIG_GLOBAL;
		value = GetWinstorePath();
	}
	else if (type == SimpleCredentialType::GlobalGCM)
	{
		configLevel = CONFIG_GLOBAL;
		value = L"manager";
	}
	else if (type == SimpleCredentialType::SystemWincred)
	{
		configLevel = CONFIG_SYSTEM;
		value = L"wincred";
	}
	else if (type == SimpleCredentialType::SystemGCM)
	{
		configLevel = CONFIG_SYSTEM;
		value = L"manager";
	}
	else
		return true;

	// workaround gitdll bug
	// TODO: switch to libgit2
	bool old = g_Git.m_IsUseGitDLL;
	g_Git.m_IsUseGitDLL = false;
	if (g_Git.SetConfigValue(L"credential.helper", value, configLevel))
	{
		CString msg;
		msg.FormatMessage(IDS_PROC_SAVECONFIGFAILED, L"credential.helper", static_cast<LPCTSTR>(value));
		CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return false;
	}
	g_Git.m_IsUseGitDLL = old;
	return true;
}

BOOL CSettingGitCredential::OnApply()
{
	if (!m_ChangedMask)
		return ISettingsPropPage::OnApply();

	if (SaveSettings())
		return ISettingsPropPage::OnApply();
	else
		return FALSE;
}

bool CSettingGitCredential::SaveSettings()
{
	CWaitCursor wait;
	UpdateData();

	int type = m_ctrlSimpleCredential.GetCurSel();
	if ((type == SimpleCredentialType::SystemWincred || type == SimpleCredentialType::SystemGCM) && !CAppUtils::IsAdminLogin())
	{
		if (m_iSimpleStoredValue == type)
		{
			if (DeleteOtherKeys(type) > 0)
				return false;
			m_ChangedMask = 0;
			return true;
		}

		if (MessageBox(L"Cannot modify a system config without proper rights.\nDiscard changes?", L"TortoiseGit", MB_ICONERROR | MB_YESNO) == IDYES)
			return true;
		return false;
	}
	if (type != SimpleCredentialType::Advanced)
	{
		if (!SaveSimpleCredential(type))
			return false;

		if (DeleteOtherKeys(type) > 0)
			return false;
		m_ChangedMask = 0;
		return true;
	}

	int sel = m_ctrlConfigType.GetCurSel();
	if (sel == ConfigType::System && !CAppUtils::IsAdminLogin())
	{
		if (MessageBox(L"Cannot modify a system config without proper rights.\nDiscard changes?", L"TortoiseGit", MB_ICONERROR | MB_YESNO) == IDYES)
			return true;
		return false;
	}

	if (m_ChangedMask & CREDENTIAL_URL)
	{
		//Add Helper
		if (m_strHelper.IsEmpty())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_GITCREDENTIAL_HELPEREMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return false;
		}
		m_strUrl.Replace(L'\\', L'/');
		m_strHelper.Replace(L'\\', L'/');

		Save(L"helper", m_strHelper);
		m_ChangedMask &= ~CREDENTIAL_HELPER;

		sel = m_ctrlConfigType.GetCurSel();
		CString prefix = sel == ConfigType::System ? L"S" : sel == ConfigType::Global ? L"G" : L"L";
		CString text;
		if (!m_strUrl.IsEmpty())
			text.Format(L"%s:%s", static_cast<LPCTSTR>(prefix), static_cast<LPCTSTR>(m_strUrl));
		else
			text = prefix;
		int urlIndex = m_ctrlUrlList.AddString(text);
		m_ctrlUrlList.SetCurSel(urlIndex);
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	}

	if (m_ChangedMask & CREDENTIAL_HELPER)
	{
		m_strHelper.Replace(L'\\', L'/');
		Save(L"helper", m_strHelper);
	}

	if (m_ChangedMask & CREDENTIAL_USERNAME)
		Save(L"username", m_strUsername);

	if (m_ChangedMask & CREDENTIAL_USEHTTPPATH)
		Save(L"useHttpPath", m_bUseHttpPath ? L"true" : L"");

	m_ChangedMask &= ~CREDENTIAL_ADVANCED_MASK;
	if (!m_ChangedMask)
		SetModified(FALSE);

	return true;
}

void CSettingGitCredential::OnBnClickedButtonRemove()
{
	int index = m_ctrlUrlList.GetCurSel();
	if (index >= 0)
	{
		CString str;
		m_ctrlUrlList.GetText(index, str);
		CString msg;
		msg.Format(IDS_WARN_REMOVE, static_cast<LPCTSTR>(str));
		if (CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CAutoConfig config(true);
			CAutoRepository repo(g_Git.GetGitRepository());
			int pos = str.Find(L':');
			CString prefix = pos >= 0 ? str.Left(pos) : str;
			CString url = pos >= 0 ? str.Mid(pos + 1) : L"";
			CONFIG_TYPE configLevel = CONFIG_LOCAL;
			switch (prefix[0])
			{
			case L'L':
				{
				configLevel = CONFIG_LOCAL;
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, repo, FALSE);
				break;
				}
			case L'G':
			case L'X':
				{
				configLevel = CONFIG_GLOBAL;
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, repo, FALSE);
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, repo, FALSE);
				break;
				}
			case L'P':
			case L'S':
				{
				if (!CAppUtils::IsAdminLogin())
				{
					MessageBox(L"Cannot modify a system config without proper rights.", L"TortoiseGit", MB_ICONERROR);
					return;
				}
				configLevel = CONFIG_SYSTEM;
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, repo, FALSE);
				if (!g_Git.ms_bCygwinGit && !g_Git.ms_bMsys2Git)
					git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitProgramDataConfig()), GIT_CONFIG_LEVEL_PROGRAMDATA, repo, FALSE);
				break;
				}
			}

			STRING_VECTOR list;
			CStringA urlA = CUnicodeUtils::GetUTF8(url);
			CStringA pattern = urlA.IsEmpty() ? "^credential\\.[^.]+$" : ("credential\\." + RegexEscape(urlA) + "\\..*");
			git_config_foreach_match(config, pattern, GetCredentialEntryCallback, &list);
			for (size_t i = 0; i < list.size(); ++i)
				g_Git.UnsetConfigValue(list[i], configLevel);
			m_ctrlUrlList.DeleteString(index);
			OnLbnSelchangeListUrl();
		}
	}
}

void CSettingGitCredential::OnBnClickedOpensettingselevated()
{
	CString sCmd;
	sCmd.Format(L"/command:settings /page:gitcredential /path:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir));
	CAppUtils::RunTortoiseGitProc(sCmd, true);
}
