// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016 - TortoiseGit

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
	static int Advanced;
	static int None;
	static int LocalWincred;
	static int LocalWinstore;
	static int LocalGCM;
	static int GlobalWincred;
	static int GlobalWinstore;
	static int GlobalGCM;
	static int SystemWincred;
	static int SystemGCM;

	static void Init()
	{
		Advanced = -1;
		None = -1;
		LocalWincred = -1;
		LocalWinstore = -1;
		LocalGCM = -1;
		GlobalWincred = -1;
		GlobalWinstore = -1;
		GlobalGCM = -1;
		SystemWincred = -1;
		SystemGCM = -1;
	}
}

namespace ConfigType
{
	static int Local;
	static int Global;
	static int System;

	static void Init()
	{
		Local = -1;
		Global = -1;
		System = -1;
	}
}

// CSettingGitCredential dialog

IMPLEMENT_DYNAMIC(CSettingGitCredential, ISettingsPropPage)

CSettingGitCredential::CSettingGitCredential()
	: ISettingsPropPage(CSettingGitCredential::IDD)
	, m_bUseHttpPath(FALSE)
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
END_MESSAGE_MAP()

static bool RunUAC()
{
	CString sCmd;
	sCmd.Format(L"/command:settings /page:gitcredential /path:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir);
	return CAppUtils::RunTortoiseGitProc(sCmd, true);
}

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

	ConfigType::Init();
	AddConfigType(ConfigType::Local, CString(MAKEINTRESOURCE(IDS_SETTINGS_LOCAL)), hasLocal);
	AddConfigType(ConfigType::Global, CString(MAKEINTRESOURCE(IDS_SETTINGS_GLOBAL)));
	AddConfigType(ConfigType::System, CString(MAKEINTRESOURCE(IDS_SETTINGS_SYSTEM)));
	m_ctrlConfigType.SetCurSel(0);

	if (WincredExists())
		((CComboBox*) GetDlgItem(IDC_COMBO_HELPER))->AddString(L"wincred");
	if (WinstoreExists())
		((CComboBox*) GetDlgItem(IDC_COMBO_HELPER))->AddString(GetWinstorePath());
	if (GCMExists())
		((CComboBox*)GetDlgItem(IDC_COMBO_HELPER))->AddString(L"manager");

	SimpleCredentialType::Init();
	AddSimpleCredential(SimpleCredentialType::Advanced, CString(MAKEINTRESOURCE(IDS_ADVANCED)));
	AddSimpleCredential(SimpleCredentialType::None, CString(MAKEINTRESOURCE(IDS_NONE)));
	AddSimpleCredential(SimpleCredentialType::LocalWincred, CString(MAKEINTRESOURCE(IDS_LOCAL_WINCRED)), hasLocal && WincredExists());
	AddSimpleCredential(SimpleCredentialType::LocalWinstore, CString(MAKEINTRESOURCE(IDS_LOCAL_WINSTORE)), hasLocal && WinstoreExists());
	AddSimpleCredential(SimpleCredentialType::LocalGCM, CString(MAKEINTRESOURCE(IDS_LOCAL_GCM)), hasLocal && GCMExists());
	AddSimpleCredential(SimpleCredentialType::GlobalWincred, CString(MAKEINTRESOURCE(IDS_GLOBAL_WINCRED)), WincredExists());
	AddSimpleCredential(SimpleCredentialType::GlobalWinstore, CString(MAKEINTRESOURCE(IDS_GLOBAL_WINSTORE)), WinstoreExists());
	AddSimpleCredential(SimpleCredentialType::GlobalGCM, CString(MAKEINTRESOURCE(IDS_GLOBAL_GCM)), GCMExists());
	AddSimpleCredential(SimpleCredentialType::SystemWincred, CString(MAKEINTRESOURCE(IDS_SYSTEM_WINCRED)), WincredExists());
	AddSimpleCredential(SimpleCredentialType::SystemGCM, CString(MAKEINTRESOURCE(IDS_SYSTEM_GCM)), GCMExists());

	LoadList();

	EnableAdvancedOptions();

	UpdateData(FALSE);
	return TRUE;
}
// CSettingGitCredential message handlers

void CSettingGitCredential::OnCbnSelchangeComboSimplecredential()
{
	EnableAdvancedOptions();
	SetModified();
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
		text.Format(L"%s:%s", (LPCTSTR)prefix, (LPCTSTR)m_strUrl);
	else
		text = prefix;
	if (IsUrlExist(text))
	{
		CString msg;
		msg.Format(IDS_GITCREDENTIAL_OVERWRITEHELPER, (LPCTSTR)m_strUrl);
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

	OnApply();
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
	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(enable);
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(enable);
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

	if (m_ChangedMask)
	{
		if (CMessageBox::Show(GetSafeHwnd(), IDS_GITCREDENTIAL_SAVEHELPER, IDS_APPNAME, 1, IDI_QUESTION, IDS_SAVEBUTTON, IDS_DISCARDBUTTON) == 1)
			OnApply();
	}
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

	m_ChangedMask = 0;

	GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(TRUE);
	this->UpdateData(FALSE);
}

void CSettingGitCredential::OnCbnSelchangeComboConfigType()
{
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
	else
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

void CSettingGitCredential::LoadList()
{
	CAutoConfig config(true);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, FALSE);
	if (!g_Git.ms_bCygwinGit && !g_Git.ms_bMsys2Git)
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitProgramDataConfig()), GIT_CONFIG_LEVEL_PROGRAMDATA, FALSE);

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
		m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::None);
		return;
	}
	if (anyList.size() > 1)
	{
		m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::Advanced);
		return;
	}

	int pos = 0;
	CString prefix = anyList[0].Tokenize(L"\n", pos);
	CString key = anyList[0].Tokenize(L"\n", pos);
	CString value = anyList[0].Tokenize(L"\n", pos);
	if (key != L"credential.helper")
	{
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
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::SystemWincred);
			return;
		}
		else if (value == L"manager")
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::SystemGCM);
			return;
		}
	}

	m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::Advanced);
}

CString CSettingGitCredential::Load(CString key)
{
	CString cmd;

	if (m_strUrl.IsEmpty())
		cmd.Format(L"credential.%s", (LPCTSTR)key);
	else
		cmd.Format(L"credential.%s.%s", (LPCTSTR)m_strUrl, (LPCTSTR)key);

	CAutoConfig config(true);
	int sel = m_ctrlConfigType.GetCurSel();
	if (sel == ConfigType::Local)
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, FALSE);
	else if (sel == ConfigType::Global)
	{
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, FALSE);
	}
	else if (sel == ConfigType::System)
		git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, FALSE);

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
		cmd.Format(L"credential.%s", (LPCTSTR)key);
	else
		cmd.Format(L"credential.%s.%s", (LPCTSTR)m_strUrl, (LPCTSTR)key);

	int sel = m_ctrlConfigType.GetCurSel();
	CONFIG_TYPE configLevel = sel == ConfigType::System ? CONFIG_SYSTEM : sel == ConfigType::Global ? CONFIG_GLOBAL : CONFIG_LOCAL;

	bool old = g_Git.m_IsUseGitDLL;
	// workaround gitdll bug
	// TODO: switch to libgit2
	g_Git.m_IsUseGitDLL = false;
	if (g_Git.SetConfigValue(cmd, value, configLevel))
	{
		CString msg;
		msg.Format(IDS_PROC_SAVECONFIGFAILED, (LPCTSTR)cmd, (LPCTSTR)value);
		CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
	}
	if (value.IsEmpty())
	{
		if (g_Git.UnsetConfigValue(cmd, configLevel))
		{
			CString msg;
			msg.Format(IDS_PROC_SAVECONFIGFAILED, (LPCTSTR)cmd, (LPCTSTR)value);
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
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, FALSE);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, FALSE);

	STRING_VECTOR list;
	git_config_foreach_match(config, "credential\\..*", GetCredentialAnyEntryCallback, &list);
	for (size_t i = 0; i < list.size(); ++i)
	{
		int pos = 0;
		CString prefix = list[i].Tokenize(L"\n", pos);
		if ((prefix == L"S" || prefix == L"P") && !CAppUtils::IsAdminLogin())
		{
			RunUAC();
			return -1;
		}
	}

	int result = 0;
	// workaround gitdll bug
	// TODO: switch to libgit2
	bool old = g_Git.m_IsUseGitDLL;
	g_Git.m_IsUseGitDLL = false;
	for (size_t i = 0; i < list.size(); ++i)
	{
		if (list[i] != match)
		{
			int pos = 0;
			CString prefix = list[i].Tokenize(L"\n", pos);
			CString key = list[i].Tokenize(L"\n", pos);
			CONFIG_TYPE configLevel = (prefix == L"S" || prefix == L"P") ? CONFIG_SYSTEM : prefix == L"G" || prefix == L"X" ? CONFIG_GLOBAL : CONFIG_LOCAL;
			if (g_Git.UnsetConfigValue(key, configLevel))
			{
				CString msg;
				msg.Format(IDS_PROC_SAVECONFIGFAILED, (LPCTSTR)key, L"");
				CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
				result = 1;
				break;
			}
		}
	}
	g_Git.m_IsUseGitDLL = old;

	return result;
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
		msg.Format(IDS_PROC_SAVECONFIGFAILED, L"credential.helper", (LPCTSTR)value);
		CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return false;
	}
	g_Git.m_IsUseGitDLL = old;
	return true;
}

BOOL CSettingGitCredential::OnApply()
{
	CWaitCursor wait;
	UpdateData();

	int type = m_ctrlSimpleCredential.GetCurSel();
	if (type == SimpleCredentialType::SystemWincred && !CAppUtils::IsAdminLogin())
	{
		RunUAC();
		EndDialog(0);
		return FALSE;
	}
	if (type == SimpleCredentialType::SystemGCM && !CAppUtils::IsAdminLogin())
	{
		RunUAC();
		EndDialog(0);
		return FALSE;
	}
	if (type != SimpleCredentialType::Advanced)
	{
		if (!SaveSimpleCredential(type))
			return FALSE;

		int ret = DeleteOtherKeys(type);
		if (ret < 0)
			EndDialog(0);
		if (ret)
			return FALSE;
		SetModified(FALSE);
		return ISettingsPropPage::OnApply();
	}

	int sel = m_ctrlConfigType.GetCurSel();
	if (sel == ConfigType::System && !CAppUtils::IsAdminLogin())
	{
		RunUAC();
		EndDialog(0);
		return FALSE;
	}

	if (m_ChangedMask & CREDENTIAL_URL)
	{
		//Add Helper
		if (m_strHelper.IsEmpty())
		{
			CMessageBox::Show(GetSafeHwnd(), IDS_GITCREDENTIAL_HELPEREMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return FALSE;
		}
		m_strUrl.Replace(L'\\', L'/');
		m_strHelper.Replace(L'\\', L'/');
		
		Save(L"helper", m_strHelper);
		m_ChangedMask &= ~CREDENTIAL_HELPER;
		
		sel = m_ctrlConfigType.GetCurSel();
		CString prefix = sel == ConfigType::System ? L"S" : sel == ConfigType::Global ? L"G" : L"L";
		CString text;
		if (!m_strUrl.IsEmpty())
			text.Format(L"%s:%s", (LPCTSTR)prefix, (LPCTSTR)m_strUrl);
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

	SetModified(FALSE);

	m_ChangedMask = 0;
	return ISettingsPropPage::OnApply();
}

void CSettingGitCredential::OnBnClickedButtonRemove()
{
	int index = m_ctrlUrlList.GetCurSel();
	if (index >= 0)
	{
		CString str;
		m_ctrlUrlList.GetText(index, str);
		CString msg;
		msg.Format(IDS_WARN_REMOVE, (LPCTSTR)str);
		if (CMessageBox::Show(GetSafeHwnd(), msg, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			CAutoConfig config(true);
			int pos = str.Find(L':');
			CString prefix = pos >= 0 ? str.Left(pos) : str;
			CString url = pos >= 0 ? str.Mid(pos + 1) : L"";
			CONFIG_TYPE configLevel = CONFIG_LOCAL;
			switch (prefix[0])
			{
			case L'L':
				{
				configLevel = CONFIG_LOCAL;
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitLocalConfig()), GIT_CONFIG_LEVEL_LOCAL, FALSE);
				break;
				}
			case L'G':
			case L'X':
				{
				configLevel = CONFIG_GLOBAL;
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, FALSE);
				break;
				}
			case L'P':
			case L'S':
				{
				if (!CAppUtils::IsAdminLogin())
				{
					RunUAC();
					EndDialog(0);
					return;
				}
				configLevel = CONFIG_SYSTEM;
				git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, FALSE);
				if (!g_Git.ms_bCygwinGit && !g_Git.ms_bMsys2Git)
					git_config_add_file_ondisk(config, CGit::GetGitPathStringA(g_Git.GetGitProgramDataConfig()), GIT_CONFIG_LEVEL_PROGRAMDATA, FALSE);
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
