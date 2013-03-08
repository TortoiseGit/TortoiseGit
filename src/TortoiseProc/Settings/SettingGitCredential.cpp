// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit

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
#include "git.h"
#include "PathUtils.h"

namespace SimpleCredentialType
{
	static int Advanced;
	static int None;
	static int LocalWincred;
	static int LocalWinstore;
	static int GlobalWincred;
	static int GlobalWinstore;
	static int SystemWincred;

	static void Init()
	{
		Advanced = -1;
		None = -1;
		LocalWincred = -1;
		LocalWinstore = -1;
		GlobalWincred = -1;
		GlobalWinstore = -1;
		SystemWincred = -1;
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

CSettingGitCredential::CSettingGitCredential(CString cmdPath)
	: ISettingsPropPage(CSettingGitCredential::IDD)
	, m_strUrl(_T(""))
	, m_strHelper(_T(""))
	, m_strUsername(_T(""))
	, m_bUseHttpPath(FALSE)
	, m_cmdPath(cmdPath)
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
	sCmd.Format(_T("/command:settings /page:gitcredential /path:\"%s\""), g_Git.m_CurrentDir);
	return CAppUtils::RunTortoiseGitProc(sCmd, true);
}

static CString GetWinstorePath()
{
	TCHAR winstorebuf[MAX_PATH];
	ExpandEnvironmentStrings(_T("%AppData%\\GitCredStore\\git-credential-winstore.exe"), winstorebuf, MAX_PATH);
	CString winstore;
	winstore.Format(_T("!'%s'"), winstorebuf);
	return winstore;
}

static bool WincredExists()
{
	CString path = g_Git.ms_LastMsysGitDir;
	if (g_Git.ms_LastMsysGitDir.Right(1) != _T("\\"))
		path.AppendChar(_T('\\'));
	path.Append(_T("..\\libexec\\git-core\\git-credential-wincred.exe"));
	return !!PathFileExists(path);
}

static bool WinstoreExists()
{
	return !!PathFileExists(GetWinstorePath());
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

	CString proj;
	bool hasLocal = g_GitAdminDir.HasAdminDir(m_cmdPath, &proj);
	if (hasLocal)
	{
		CString title;
		this->GetWindowText(title);
		this->SetWindowText(title + _T(" - ") + proj);
	}

	m_ctrlUrlList.ResetContent();

	ConfigType::Init();
	AddConfigType(ConfigType::Local, CString(MAKEINTRESOURCE(IDS_SETTINGS_LOCAL)), hasLocal);
	AddConfigType(ConfigType::Global, CString(MAKEINTRESOURCE(IDS_SETTINGS_GLOBAL)));
	AddConfigType(ConfigType::System, CString(MAKEINTRESOURCE(IDS_SETTINGS_SYSTEM)));
	m_ctrlConfigType.SetCurSel(0);

	if (WincredExists())
		((CComboBox*) GetDlgItem(IDC_COMBO_HELPER))->AddString(_T("wincred"));
	if (WinstoreExists())
		((CComboBox*) GetDlgItem(IDC_COMBO_HELPER))->AddString(GetWinstorePath());

	SimpleCredentialType::Init();
	AddSimpleCredential(SimpleCredentialType::Advanced, CString(MAKEINTRESOURCE(IDS_ADVANCED)));
	AddSimpleCredential(SimpleCredentialType::None, CString(MAKEINTRESOURCE(IDS_NONE)));
	AddSimpleCredential(SimpleCredentialType::LocalWincred, CString(MAKEINTRESOURCE(IDS_LOCAL_WINCRED)), hasLocal && WincredExists());
	AddSimpleCredential(SimpleCredentialType::LocalWinstore, CString(MAKEINTRESOURCE(IDS_LOCAL_WINSTORE)), hasLocal && WinstoreExists());
	AddSimpleCredential(SimpleCredentialType::GlobalWincred, CString(MAKEINTRESOURCE(IDS_GLOBAL_WINCRED)), WincredExists());
	AddSimpleCredential(SimpleCredentialType::GlobalWinstore, CString(MAKEINTRESOURCE(IDS_GLOBAL_WINSTORE)), WinstoreExists());
	AddSimpleCredential(SimpleCredentialType::SystemWincred, CString(MAKEINTRESOURCE(IDS_SYSTEM_WINCRED)), WincredExists());

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
		CMessageBox::Show(NULL, IDS_GITCREDENTIAL_HELPEREMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	m_ChangedMask = CREDENTIAL_URL | CREDENTIAL_HELPER | CREDENTIAL_USERNAME | CREDENTIAL_USEHTTPPATH;
	int sel = m_ctrlConfigType.GetCurSel();
	CString prefix = sel == ConfigType::System ? _T("S") : sel == ConfigType::Global ? _T("G") : _T("L");
	CString text;
	if (!m_strUrl.IsEmpty())
		text.Format(_T("%s:%s"), prefix, m_strUrl);
	else
		text = prefix;
	if (IsUrlExist(text))
	{
		CString msg;
		msg.Format(IDS_GITCREDENTIAL_OVERWRITEHELPER, m_strUrl);
		if (CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) == IDYES)
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
		{
			return true;
		}
	}
	return false;
}

void CSettingGitCredential::OnLbnSelchangeListUrl()
{
	CWaitCursor wait;

	if (m_ChangedMask)
	{
		if (CMessageBox::Show(NULL, IDS_GITCREDENTIAL_SAVEHELPER, IDS_APPNAME, 1, IDI_QUESTION, IDS_SAVEBUTTON, IDS_DISCARDBUTTON) == 1)
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
	int pos = text.Find(_T(':'));
	CString prefix = pos >= 0 ? text.Left(pos) : text.Left(1);
	m_ctrlConfigType.SetCurSel(prefix == _T("S") ? ConfigType::System : prefix == _T("X") ? ConfigType::Global : prefix == _T("G") ? ConfigType::Global : ConfigType::Local);
	m_strUrl = pos >= 0 ? text.Mid(pos + 1) : _T("");

	m_strHelper = Load(_T("helper"));
	m_strUsername = Load(_T("username"));
	m_bUseHttpPath = Load(_T("useHttpPath")) == _T("true") ? TRUE : FALSE;

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

static int GetCredentialDefaultUrlCallback(const git_config_entry *entry, void *payload)
{
	CString display = entry->level == 1 ? _T("S") : entry->level == 2 ? _T("X") : entry->level == 3 ? _T("G") : _T("L");
	((STRING_VECTOR *)payload)->push_back(display);
	return 0;
}

static int GetCredentialUrlCallback(const git_config_entry *entry, void *payload)
{
	CString name = CUnicodeUtils::GetUnicode(entry->name);
	int pos1 = name.Find(_T('.'));
	int pos2 = name.ReverseFind(_T('.'));
	CString url = name.Mid(pos1 + 1, pos2 - pos1 - 1);
	CString display;
	display.Format(_T("%s:%s"), entry->level == 1 ? _T("S") : entry->level == 2 ? _T("X") : entry->level == 3 ? _T("G") : _T("L"), url);
	((STRING_VECTOR *)payload)->push_back(display);
	return 0;
}

static int GetCredentialEntryCallback(const git_config_entry *entry, void *payload)
{
	CString name = CUnicodeUtils::GetUnicode(entry->name);
	((STRING_VECTOR *)payload)->push_back(name);
	return 0;
}

static int GetCredentialAnyEntryCallback(const git_config_entry *entry, void *payload)
{
	CString name = CUnicodeUtils::GetUnicode(entry->name);
	CString value = CUnicodeUtils::GetUnicode(entry->value);
	CString display = entry->level == 1 ? _T("S") : entry->level == 2 ? _T("X") : entry->level == 3 ? _T("G") : _T("L");	
	CString text;
	text.Format(_T("%s\n%s\n%s"), display, name, value);
	((STRING_VECTOR *)payload)->push_back(text);
	return 0;
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
	git_config * config;
	git_config_new(&config);
	CStringA projectConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitLocalConfig());
	git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 4, FALSE);
	projectConfigA.ReleaseBuffer();
	CStringA globalConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalConfig());
	git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 3, FALSE);
	globalConfigA.ReleaseBuffer();
	CStringA globalXDGConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalXDGConfig());
	git_config_add_file_ondisk(config, globalXDGConfigA.GetBuffer(), 2, FALSE);
	globalXDGConfigA.ReleaseBuffer();
	CStringA systemConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitSystemConfig());
	git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1, FALSE);
	systemConfigA.ReleaseBuffer();

	STRING_VECTOR defaultList, urlList;
	git_config_foreach_match(config, "credential\\.helper", GetCredentialDefaultUrlCallback, &defaultList);
	git_config_foreach_match(config, "credential\\..*\\.helper", GetCredentialUrlCallback, &urlList);
	STRING_VECTOR anyList;
	git_config_foreach_match(config, "credential\\..*", GetCredentialAnyEntryCallback, &anyList);
	git_config_free(config);

	for (int i = 0; i < defaultList.size(); ++i)
		m_ctrlUrlList.AddString(defaultList[i]);
	for (int i = 0; i < urlList.size(); ++i)
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
	CString prefix = anyList[0].Tokenize(_T("\n"), pos);
	CString key = anyList[0].Tokenize(_T("\n"), pos);
	CString value = anyList[0].Tokenize(_T("\n"), pos);
	if (key != _T("credential.helper"))
	{
		m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::Advanced);
		return;
	}

	CString winstore = GetWinstorePath();
	if (prefix == _T("L"))
	{
		if (value == _T("wincred"))
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::LocalWincred);
			return;
		}
		else if (value == winstore)
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::LocalWinstore);
			return;
		}
	}

	if (prefix == _T("G") || prefix == _T("X"))
	{
		if (value == _T("wincred"))
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::GlobalWincred);
			return;
		}
		else if (value == winstore)
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::GlobalWinstore);
			return;
		}
	}

	if (prefix == _T("S"))
	{
		if (value == _T("wincred"))
		{
			m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::SystemWincred);
			return;
		}
	}

	m_ctrlSimpleCredential.SetCurSel(SimpleCredentialType::Advanced);
}

CString CSettingGitCredential::Load(CString key)
{
	CString cmd;

	if (m_strUrl.IsEmpty())
		cmd.Format(_T("credential.%s"), key);
	else
		cmd.Format(_T("credential.%s.%s"), m_strUrl, key);

	git_config * config;
	git_config_new(&config);
	int sel = m_ctrlConfigType.GetCurSel();
	if (sel == ConfigType::Local)
	{
		CStringA projectConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitLocalConfig());
		git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 1, FALSE);
		projectConfigA.ReleaseBuffer();
	}
	else if (sel == ConfigType::Global)
	{
		CStringA globalConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalConfig());
		git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 2, FALSE);
		globalConfigA.ReleaseBuffer();
		CStringA globalXDGConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalXDGConfig());
		git_config_add_file_ondisk(config, globalXDGConfigA.GetBuffer(), 1, FALSE);
		globalXDGConfigA.ReleaseBuffer();
	}
	else if (sel == ConfigType::System)
	{
		CStringA systemConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitSystemConfig());
		git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1, FALSE);
		systemConfigA.ReleaseBuffer();
	}
	
	CStringA cmdA = CUnicodeUtils::GetUTF8(cmd);
	CStringA valueA;
	const git_config_entry *entry;
	if (!git_config_get_entry(&entry, config, cmdA))
		valueA = CStringA(entry->value);
	cmdA.ReleaseBuffer();
	git_config_free(config);

	return CUnicodeUtils::GetUnicode(valueA);
}

void CSettingGitCredential::Save(CString key, CString value)
{
	CString cmd, out;

	if (m_strUrl.IsEmpty())
		cmd.Format(_T("credential.%s"), key);
	else
		cmd.Format(_T("credential.%s.%s"), m_strUrl, key);

	int sel = m_ctrlConfigType.GetCurSel();
	CONFIG_TYPE configLevel = sel == ConfigType::System ? CONFIG_SYSTEM : sel == ConfigType::Global ? CONFIG_GLOBAL : CONFIG_LOCAL;

	bool old = g_Git.m_IsUseGitDLL;
	// workaround gitdll bug
	// TODO: switch to libgit2
	g_Git.m_IsUseGitDLL = false;
	if (g_Git.SetConfigValue(cmd, value, configLevel, CP_UTF8, &g_Git.m_CurrentDir))
	{
		CString msg;
		msg.Format(IDS_PROC_SAVECONFIGFAILED, cmd, value);
		CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
	}
	if (value.IsEmpty())
	{
		if (g_Git.UnsetConfigValue(cmd, configLevel, CP_UTF8, &g_Git.m_CurrentDir))
		{
			CString msg;
			msg.Format(IDS_PROC_SAVECONFIGFAILED, cmd, value);
			CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		}
	}
	g_Git.m_IsUseGitDLL = old;
}

static int DeleteOtherKeys(int type)
{
	CString match;
	if (type == SimpleCredentialType::LocalWincred)
		match = _T("L\ncredential.helper\nwincred");
	else if (type == SimpleCredentialType::LocalWinstore)
		match = _T("L\ncredential.helper\n") + GetWinstorePath();
	else if (type == SimpleCredentialType::GlobalWincred)
		match = _T("G\ncredential.helper\nwincred");
	else if (type == SimpleCredentialType::GlobalWinstore)
		match = _T("G\ncredential.helper\n") + GetWinstorePath();
	else if (type == SimpleCredentialType::SystemWincred)
		match = _T("S\ncredential.helper\nwincred");

	git_config * config;
	git_config_new(&config);
	CStringA projectConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitLocalConfig());
	git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 4, FALSE);
	projectConfigA.ReleaseBuffer();
	CStringA globalConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalConfig());
	git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 3, FALSE);
	globalConfigA.ReleaseBuffer();
	CStringA globalXDGConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalXDGConfig());
	git_config_add_file_ondisk(config, globalXDGConfigA.GetBuffer(), 2, FALSE);
	globalXDGConfigA.ReleaseBuffer();
	CStringA systemConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitSystemConfig());
	git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1, FALSE);
	systemConfigA.ReleaseBuffer();

	STRING_VECTOR list;
	git_config_foreach_match(config, "credential\\..*", GetCredentialAnyEntryCallback, &list);
	for (size_t i = 0; i < list.size(); ++i)
	{
		int pos = 0;
		CString prefix = list[i].Tokenize(_T("\n"), pos);
		if (prefix == _T("S") && !CAppUtils::IsAdminLogin())
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
			CString prefix = list[i].Tokenize(_T("\n"), pos);
			CString key = list[i].Tokenize(_T("\n"), pos);
			CONFIG_TYPE configLevel = prefix == _T("S") ? CONFIG_SYSTEM : prefix == _T("G") || prefix == _T("X") ? CONFIG_GLOBAL : CONFIG_LOCAL;
			if (g_Git.UnsetConfigValue(key, configLevel, CP_UTF8, &g_Git.m_CurrentDir))
			{
				CString msg;
				msg.Format(IDS_PROC_SAVECONFIGFAILED, key, _T(""));
				CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
				result = 1;
				break;
			}
		}
	}
	g_Git.m_IsUseGitDLL = old;

	return result;
}

bool SaveSimpleCredential(int type)
{
	CONFIG_TYPE configLevel;
	CString value;
	if (type == SimpleCredentialType::LocalWincred)
	{
		configLevel = CONFIG_LOCAL;
		value = _T("wincred");
	}
	else if (type == SimpleCredentialType::LocalWinstore)
	{
		configLevel = CONFIG_LOCAL;
		value = GetWinstorePath();
	}
	else if (type == SimpleCredentialType::GlobalWincred)
	{
		configLevel = CONFIG_GLOBAL;
		value = _T("wincred");
	}
	else if (type == SimpleCredentialType::GlobalWinstore)
	{
		configLevel = CONFIG_GLOBAL;
		value = GetWinstorePath();
	}
	else if (type == SimpleCredentialType::SystemWincred)
	{
		configLevel = CONFIG_SYSTEM;
		value = _T("wincred");
	}
	else
	{
		return true;
	}

	// workaround gitdll bug
	// TODO: switch to libgit2
	bool old = g_Git.m_IsUseGitDLL;
	g_Git.m_IsUseGitDLL = false;
	if (g_Git.SetConfigValue(_T("credential.helper"), value, configLevel, CP_UTF8, &g_Git.m_CurrentDir))
	{
		CString msg;
		msg.Format(IDS_PROC_SAVECONFIGFAILED, _T("credential.helper"), value);
		CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
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
	if (type != SimpleCredentialType::Advanced)
	{
		if (!SaveSimpleCredential(type))
			return FALSE;

		if (!DeleteOtherKeys(type))
		{
			EndDialog(0);
			return FALSE;
		}
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
			CMessageBox::Show(NULL, IDS_GITCREDENTIAL_HELPEREMPTY, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return FALSE;
		}
		m_strUrl.Replace(L'\\', L'/');
		m_strHelper.Replace(L'\\', L'/');
		
		Save(_T("helper"), m_strHelper);
		m_ChangedMask &= ~CREDENTIAL_HELPER;
		
		int sel = m_ctrlConfigType.GetCurSel();
		CString prefix = sel == ConfigType::System ? _T("S") : sel == ConfigType::Global ? _T("G") : _T("L");
		CString text;
		if (!m_strUrl.IsEmpty())
			text.Format(_T("%s:%s"), prefix, m_strUrl);
		else
			text = prefix;
		int urlIndex = m_ctrlUrlList.AddString(text);
		m_ctrlUrlList.SetCurSel(urlIndex);
		GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(TRUE);
	}

	if (m_ChangedMask & CREDENTIAL_HELPER)
	{
		m_strHelper.Replace(L'\\', L'/');
		Save(_T("helper"), m_strHelper);
	}

	if (m_ChangedMask & CREDENTIAL_USERNAME)
		Save(_T("username"), m_strUsername);

	if (m_ChangedMask & CREDENTIAL_USEHTTPPATH)
		Save(_T("useHttpPath"), m_bUseHttpPath ? _T("true") : _T(""));

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
		msg.Format(IDS_GITCREDENTIAL_DELETEHELPER, str);
		if (CMessageBox::Show(NULL, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION) == IDYES)
		{
			git_config * config;
			git_config_new(&config);
			int pos = str.Find(_T(':'));
			CString prefix = pos >= 0 ? str.Left(pos) : str;
			CString url = pos >= 0 ? str.Mid(pos + 1) : _T("");
			CONFIG_TYPE configLevel = CONFIG_LOCAL;
			switch (prefix[0])
			{
			case _T('L'):
				{
				configLevel = CONFIG_LOCAL;
				CStringA projectConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitLocalConfig());
				git_config_add_file_ondisk(config, projectConfigA.GetBuffer(), 1, FALSE);
				projectConfigA.ReleaseBuffer();
				break;
				}
			case _T('G'):
			case _T('X'):
				{
				configLevel = CONFIG_GLOBAL;
				CStringA globalConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalConfig());
				git_config_add_file_ondisk(config, globalConfigA.GetBuffer(), 2, FALSE);
				globalConfigA.ReleaseBuffer();
				CStringA globalXDGConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitGlobalXDGConfig());
				git_config_add_file_ondisk(config, globalXDGConfigA.GetBuffer(), 1, FALSE);
				globalXDGConfigA.ReleaseBuffer();
				break;
				}
			case _T('S'):
				{
				if (!CAppUtils::IsAdminLogin())
				{
					RunUAC();
					EndDialog(0);
					return;
				}
				configLevel = CONFIG_SYSTEM;
				CStringA systemConfigA = CUnicodeUtils::GetUTF8(g_Git.GetGitSystemConfig());
				git_config_add_file_ondisk(config, systemConfigA.GetBuffer(), 1, FALSE);
				systemConfigA.ReleaseBuffer();
				break;
				}
			}

			STRING_VECTOR list;
			CStringA urlA = CUnicodeUtils::GetUTF8(url);
			CStringA pattern = urlA.IsEmpty() ? "^credential\\.[^.]+$" : ("credential\\." + RegexEscape(urlA) + "\\..*");
			git_config_foreach_match(config, pattern, GetCredentialEntryCallback, &list);
			for (size_t i = 0; i < list.size(); ++i)
				g_Git.UnsetConfigValue(list[i], configLevel, CP_UTF8, &g_Git.m_CurrentDir);

			m_ctrlUrlList.DeleteString(index);
			OnLbnSelchangeListUrl();
		}
	}
}
