// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2017, 2019 - TortoiseGit
// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "SetProxyPage.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "Git.h"
#include "SetProxyPage.h"

IMPLEMENT_DYNAMIC(CSetProxyPage, ISettingsPropPage)
CSetProxyPage::CSetProxyPage()
	: ISettingsPropPage(CSetProxyPage::IDD)
	, m_serverport(0)
	, m_isEnabled(FALSE)
{
	m_regServeraddress = CRegString(L"Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-host", L"");
	m_regServerport = CRegString(L"Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-port", L"");
	m_regUsername = CRegString(L"Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-username", L"");
	m_regPassword = CRegString(L"Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-password", L"");
	m_regSSHClient = CRegString(L"Software\\TortoiseGit\\SSH");
	m_SSHClient = m_regSSHClient;
}

CSetProxyPage::~CSetProxyPage()
{
}

void CSetProxyPage::DoDataExchange(CDataExchange* pDX)
{
	ISettingsPropPage::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SERVERADDRESS, m_serveraddress);
	DDX_Text(pDX, IDC_SERVERPORT, m_serverport);
	DDX_Text(pDX, IDC_USERNAME, m_username);
	DDX_Text(pDX, IDC_PASSWORD, m_password);
	DDX_Check(pDX, IDC_ENABLE, m_isEnabled);
	DDX_Text(pDX, IDC_SSHCLIENT, m_SSHClient);
	DDX_Control(pDX, IDC_SSHCLIENT, m_cSSHClientEdit);
}


BEGIN_MESSAGE_MAP(CSetProxyPage, ISettingsPropPage)
	ON_EN_CHANGE(IDC_SERVERADDRESS, OnChange)
	ON_EN_CHANGE(IDC_SERVERPORT, OnChange)
	ON_EN_CHANGE(IDC_USERNAME, OnChange)
	ON_EN_CHANGE(IDC_PASSWORD, OnChange)
	ON_EN_CHANGE(IDC_SSHCLIENT, OnChange)
	ON_BN_CLICKED(IDC_ENABLE, OnBnClickedEnable)
	ON_BN_CLICKED(IDC_SSHBROWSE, OnBnClickedSshbrowse)
END_MESSAGE_MAP()

HRESULT StringEscape(const CString& str_in, CString* escaped_string) {
	// http://tools.ietf.org/html/rfc3986#section-3.2.1
	if (!escaped_string)
		return E_INVALIDARG;

	DWORD buf_len = INTERNET_MAX_URL_LENGTH + 1;
	HRESULT hr = ::UrlEscape(str_in, CStrBuf(*escaped_string, buf_len), &buf_len, URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY);

	escaped_string->Replace(L"@", L"%40");
	escaped_string->Replace(L":", L"%3a");

	return hr;
}

HRESULT StringUnescape(const CString& str_in, CString* unescaped_string) {
	if (!unescaped_string)
		return E_INVALIDARG;

	DWORD buf_len = INTERNET_MAX_URL_LENGTH + 1;
	ATL::CComBSTR temp(str_in);
	return ::UrlUnescape(temp, CStrBuf(*unescaped_string, buf_len), &buf_len, 0);
}

BOOL CSetProxyPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_ENABLE);

	m_tooltips.AddTool(IDC_SERVERADDRESS, IDS_SETTINGS_PROXYSERVER_TT);

	CString proxy = g_Git.GetConfigValue(L"http.proxy");

	m_SSHClient = m_regSSHClient;
	if (m_SSHClient.IsEmpty())
		m_SSHClient = CRegString(L"Software\\TortoiseGit\\SSH", L"", FALSE, HKEY_LOCAL_MACHINE);
	if (m_SSHClient.IsEmpty())
	{
		TCHAR sPlink[MAX_PATH] = {0};
		GetModuleFileName(nullptr, sPlink, _countof(sPlink));
		LPTSTR ptr = wcsrchr(sPlink, L'\\');
		if (ptr)
		{
			wcscpy_s(ptr + 1, _countof(sPlink) - (ptr - sPlink + 1), L"TortoiseGitPlink.exe");
			m_SSHClient = CString(sPlink);
		}
	}
	m_serveraddress = m_regServeraddress;
	m_serverport = _wtoi(static_cast<LPCTSTR>(static_cast<CString>(m_regServerport)));
	m_username = m_regUsername;
	m_password = m_regPassword;

	if (proxy.IsEmpty())
	{
		m_isEnabled = FALSE;
		EnableGroup(FALSE);
	}
	else
	{
		int start=0;
		start = proxy.Find(L"://", start);
		if(start<0)
			start =0;
		else
			start+=3;

		int at = proxy.Find(L'@');
		int port;

		if(at<0)
		{
			m_username.Empty();
			m_password.Empty();
			port = proxy.Find(L':', start);
			if(port<0)
				m_serveraddress = proxy.Mid(start);
			else
				m_serveraddress = proxy.Mid(start, port-start);

		}
		else
		{
			int username = proxy.Find(L':', start);
			if(username<=0 || username >at)
			{
				StringUnescape(proxy.Mid(start, at - start), &m_username);
				m_password.Empty();
			}
			else if(username < at)
			{
				StringUnescape(proxy.Mid(start, username - start), &m_username);
				StringUnescape(proxy.Mid(username + 1, at - username - 1), &m_password);
			}

			port = proxy.Find(L':', at);
			if(port<0)
				m_serveraddress = proxy.Mid(at+1);
			else
				m_serveraddress = proxy.Mid(at+1, port-at-1);
		}

		if(port<0)
			m_serverport= 0;
		else
			m_serverport = _wtoi(proxy.Mid(port + 1));

		m_isEnabled = TRUE;
		EnableGroup(TRUE);
	}

	SHAutoComplete(::GetDlgItem(m_hWnd, IDC_SSHCLIENT), SHACF_FILESYSTEM | SHACF_FILESYS_ONLY);

	UpdateData(FALSE);

	return TRUE;
}

void CSetProxyPage::OnBnClickedEnable()
{
	UpdateData();
	if (m_isEnabled)
		EnableGroup(TRUE);
	else
		EnableGroup(FALSE);
	SetModified();
}

void CSetProxyPage::EnableGroup(BOOL b)
{
	GetDlgItem(IDC_SERVERADDRESS)->EnableWindow(b);
	GetDlgItem(IDC_SERVERPORT)->EnableWindow(b);
	GetDlgItem(IDC_USERNAME)->EnableWindow(b);
	GetDlgItem(IDC_PASSWORD)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL1)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL2)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL3)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL6)->EnableWindow(b);
}

void CSetProxyPage::OnChange()
{
	SetModified();
}

BOOL CSetProxyPage::OnApply()
{
	UpdateData();

	CString temp;
	Store(m_serveraddress, m_regServeraddress);
	temp.Format(L"%u", m_serverport);
	Store(temp, m_regServerport);
	Store(m_username, m_regUsername);
	Store(m_password, m_regPassword);


	CString http_proxy;
	if(!m_serveraddress.IsEmpty())
	{
		if (m_serveraddress.Find(L"://") == -1)
			http_proxy = L"http://";

		if(!m_username.IsEmpty())
		{
			CString escapedUsername;

			if (FAILED(StringEscape(m_username, &escapedUsername)))
			{
				::MessageBox(nullptr, L"Could not encode username.", L"TortoiseGit", MB_ICONERROR);
				return FALSE;
			}

			http_proxy += escapedUsername;

			if(!m_password.IsEmpty())
			{
				CString escapedPassword;
				if (FAILED(StringEscape(m_password, &escapedPassword)))
				{
					::MessageBox(nullptr, L"Could not encode password.", L"TortoiseGit", MB_ICONERROR);
					return FALSE;
				}
				http_proxy += L':' + escapedPassword;
			}

			http_proxy += L'@';
		}
		http_proxy+=m_serveraddress;

		if(m_serverport)
		{
			http_proxy += L':';
			http_proxy.AppendFormat(L"%u", m_serverport);
		}
	}

	if (m_isEnabled)
	{
		if (g_Git.SetConfigValue(L"http.proxy",http_proxy,CONFIG_GLOBAL))
			return FALSE;
	}
	else
		g_Git.UnsetConfigValue(L"http.proxy", CONFIG_GLOBAL);
	m_regSSHClient = m_SSHClient;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetProxyPage::OnBnClickedSshbrowse()
{
	UpdateData();
	CString filename = m_SSHClient;
	if (!PathFileExists(filename))
		filename.Empty();
	if (CAppUtils::FileOpenSave(filename, nullptr, IDS_SETTINGS_SELECTSSH, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		m_SSHClient = filename;
		UpdateData(FALSE);
		SetModified();
	}
}
