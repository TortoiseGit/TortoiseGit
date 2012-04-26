// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2012 - TortoiseGit
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
#include "git.h"
#include ".\setproxypage.h"
#include "MessageBox.h"

IMPLEMENT_DYNAMIC(CSetProxyPage, ISettingsPropPage)
CSetProxyPage::CSetProxyPage()
	: ISettingsPropPage(CSetProxyPage::IDD)
	, m_serveraddress(_T(""))
	, m_serverport(0)
	, m_username(_T(""))
	, m_password(_T(""))
	, m_isEnabled(FALSE)
	, m_SSHClient(_T(""))
{
	m_regServeraddress = CRegString(_T("Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-host"), _T(""));
	m_regServerport = CRegString(_T("Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-port"), _T(""));
	m_regUsername = CRegString(_T("Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-username"), _T(""));
	m_regPassword = CRegString(_T("Software\\TortoiseGit\\Git\\Servers\\global\\http-proxy-password"), _T(""));
	m_regSSHClient = CRegString(_T("Software\\TortoiseGit\\SSH"));
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
	ON_EN_CHANGE(IDC_TIMEOUT, OnChange)
	ON_EN_CHANGE(IDC_SSHCLIENT, OnChange)
	ON_EN_CHANGE(IDC_EXCEPTIONS, OnChange)
	ON_BN_CLICKED(IDC_ENABLE, OnBnClickedEnable)
	ON_BN_CLICKED(IDC_SSHBROWSE, OnBnClickedSshbrowse)
END_MESSAGE_MAP()

HRESULT StringEscape(const CString& str_in, CString* escaped_string) {
	// http://tools.ietf.org/html/rfc3986#section-3.2.1
	if (!escaped_string)
		return E_INVALIDARG;

	DWORD buf_len = INTERNET_MAX_URL_LENGTH + 1;
	HRESULT hr = ::UrlEscape(str_in, escaped_string->GetBufferSetLength(buf_len), &buf_len, URL_ESCAPE_PERCENT | URL_ESCAPE_SEGMENT_ONLY);
	if (SUCCEEDED(hr)) {
		escaped_string->ReleaseBuffer();
	}

	escaped_string->Replace(_T("@"), _T("%40"));
	escaped_string->Replace(_T(":"), _T("%3a"));

	return hr;
}

HRESULT StringUnescape(const CString& str_in, CString* unescaped_string) {
	if (!unescaped_string)
		return E_INVALIDARG;

	DWORD buf_len = INTERNET_MAX_URL_LENGTH + 1;
	HRESULT hr = ::UrlUnescape(str_in.AllocSysString(), unescaped_string->GetBufferSetLength(buf_len), &buf_len, 0);
	if (SUCCEEDED(hr)) {
		unescaped_string->ReleaseBuffer();
	}

	return hr;
}

BOOL CSetProxyPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_SERVERADDRESS, IDS_SETTINGS_PROXYSERVER_TT);

	CString proxy = g_Git.GetConfigValue(_T("http.proxy"), CP_UTF8);

	m_SSHClient = m_regSSHClient;
	m_serveraddress = m_regServeraddress;
	m_serverport = _ttoi((LPCTSTR)(CString)m_regServerport);
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
		start = proxy.Find(_T("://"),start);
		if(start<0)
			start =0;
		else
			start+=3;

		int at = proxy.Find(_T("@"), 0);
		int port;

		if(at<0)
		{
			m_username=_T("");
			m_password=_T("");
			port=proxy.Find(_T(":"),start);
			if(port<0)
				m_serveraddress = proxy.Mid(start);
			else
				m_serveraddress = proxy.Mid(start, port-start);

		}
		else
		{
			int username;
			username = proxy.Find(_T(":"),start);
			if(username<=0 || username >at)
			{
				StringUnescape(proxy.Mid(start, at - start), &m_username);
				m_password=_T("");
			}
			else if(username < at)
			{
				StringUnescape(proxy.Mid(start, username - start), &m_username);
				StringUnescape(proxy.Mid(username + 1, at - username - 1), &m_password);
			}

			port=proxy.Find(_T(":"),at);
			if(port<0)
				m_serveraddress = proxy.Mid(at+1);
			else
				m_serveraddress = proxy.Mid(at+1, port-at-1);
		}

		if(port<0)
		{
			m_serverport= 0;
		}
		else
			m_serverport = _ttoi(proxy.Mid(port+1));

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
	{
		EnableGroup(TRUE);
	}
	else
	{
		EnableGroup(FALSE);
	}
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

BOOL CSetProxyPage::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);
	return ISettingsPropPage::PreTranslateMessage(pMsg);
}

void CSetProxyPage::OnChange()
{
	SetModified();
}

BOOL CSetProxyPage::OnApply()
{
	UpdateData();

	CString temp;
	Store (m_serveraddress, m_regServeraddress);
	temp.Format(_T("%d"), m_serverport);
	Store (temp, m_regServerport);
	Store (m_username, m_regUsername);
	Store (m_password, m_regPassword);


	CString http_proxy;
	if(!m_serveraddress.IsEmpty())
	{
		if(m_serveraddress.Left(5) != _T("http:"))
			http_proxy=_T("http://");

		if(!m_username.IsEmpty())
		{
			CString escapedUsername;

			if (StringEscape(m_username, &escapedUsername))
			{
				::MessageBox(NULL, _T("Could not encode username."), _T("TortoiseGit"), MB_ICONERROR);
				return FALSE;
			}

			http_proxy += escapedUsername;

			if(!m_password.IsEmpty())
			{
				CString escapedPassword;
				if (StringEscape(m_password, &escapedPassword))
				{
					::MessageBox(NULL, _T("Could not encode password."), _T("TortoiseGit"), MB_ICONERROR);
					return FALSE;
				}
				http_proxy += _T(":") + escapedPassword;
			}

			http_proxy += _T("@");
		}
		http_proxy+=m_serveraddress;

		if(m_serverport)
		{
			temp.Format(_T("%d"), m_serverport);
			http_proxy  += _T(":")+temp;
		}
	}

	if (m_isEnabled)
	{
		g_Git.SetConfigValue(_T("http.proxy"),http_proxy,CONFIG_GLOBAL);
	}
	else
	{
		g_Git.UnsetConfigValue(_T("http.proxy"), CONFIG_GLOBAL);
	}
	m_regSSHClient = m_SSHClient;
	SetModified(FALSE);
	return ISettingsPropPage::OnApply();
}

void CSetProxyPage::OnBnClickedSshbrowse()
{
	CString openPath;
	if (CAppUtils::FileOpenSave(openPath, NULL, IDS_SETTINGS_SELECTSSH, IDS_PROGRAMSFILEFILTER, true, m_hWnd))
	{
		UpdateData();
		m_SSHClient = openPath;
		UpdateData(FALSE);
		SetModified();
	}
}
