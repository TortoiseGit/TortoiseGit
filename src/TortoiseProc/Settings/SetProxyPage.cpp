// TortoiseSVN - a Windows shell extension for easy version control

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
	, m_timeout(0)
	, m_isEnabled(FALSE)
	, m_SSHClient(_T(""))
	, m_Exceptions(_T(""))
{
	m_regServeraddress = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-host"), _T(""));
	m_regServerport = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-port"), _T(""));
	m_regUsername = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-username"), _T(""));
	m_regPassword = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-password"), _T(""));
	m_regTimeout = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-timeout"), _T(""));
	m_regSSHClient = CRegString(_T("Software\\TortoiseGit\\SSH"));
	m_SSHClient = m_regSSHClient;
	m_regExceptions = CRegString(_T("Software\\Tigris.org\\Subversion\\Servers\\global\\http-proxy-exceptions"), _T(""));

	m_regServeraddress_copy = CRegString(_T("Software\\TortoiseGit\\Servers\\global\\http-proxy-host"), _T(""));
	m_regServerport_copy = CRegString(_T("Software\\TortoiseGit\\Servers\\global\\http-proxy-port"), _T(""));
	m_regUsername_copy = CRegString(_T("Software\\TortoiseGit\\Servers\\global\\http-proxy-username"), _T(""));
	m_regPassword_copy = CRegString(_T("Software\\TortoiseGit\\Servers\\global\\http-proxy-password"), _T(""));
	m_regTimeout_copy = CRegString(_T("Software\\TortoiseGit\\Servers\\global\\http-proxy-timeout"), _T(""));
	m_regExceptions_copy = CRegString(_T("Software\\TortoiseGit\\Servers\\global\\http-proxy-exceptions"), _T(""));
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
	DDX_Text(pDX, IDC_TIMEOUT, m_timeout);
	DDX_Text(pDX, IDC_EXCEPTIONS, m_Exceptions);
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



BOOL CSetProxyPage::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_tooltips.Create(this);
	m_tooltips.AddTool(IDC_SERVERADDRESS, IDS_SETTINGS_PROXYSERVER_TT);
	m_tooltips.AddTool(IDC_EXCEPTIONS, IDS_SETTINGS_PROXYEXCEPTIONS_TT);

	m_SSHClient = m_regSSHClient;
	m_serveraddress = m_regServeraddress;
	m_serverport = _ttoi((LPCTSTR)(CString)m_regServerport);
	m_username = m_regUsername;
	m_password = m_regPassword;
	m_Exceptions = m_regExceptions;
	m_timeout = _ttoi((LPCTSTR)(CString)m_regTimeout);

	if (m_serveraddress.IsEmpty())
	{
		m_isEnabled = FALSE;
		EnableGroup(FALSE);
		// now since we already created our registry entries
		// we delete them here again...
		m_regServeraddress.removeValue();
		m_regServerport.removeValue();
		m_regUsername.removeValue();
		m_regPassword.removeValue();
		m_regTimeout.removeValue();
		m_regExceptions.removeValue();
	}
	else
	{
		m_isEnabled = TRUE;
		EnableGroup(TRUE);
	}
	if (m_serveraddress.IsEmpty())
		m_serveraddress = m_regServeraddress_copy;
	if (m_serverport==0)
		m_serverport = _ttoi((LPCTSTR)(CString)m_regServerport_copy);
	if (m_username.IsEmpty())
		m_username = m_regUsername_copy;
	if (m_password.IsEmpty())
		m_password = m_regPassword_copy;
	if (m_Exceptions.IsEmpty())
		m_Exceptions = m_regExceptions_copy;
	if (m_timeout == 0)
		m_timeout = _ttoi((LPCTSTR)(CString)m_regTimeout_copy);

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
	GetDlgItem(IDC_TIMEOUT)->EnableWindow(b);
	GetDlgItem(IDC_EXCEPTIONS)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL1)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL2)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL3)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL4)->EnableWindow(b);
	GetDlgItem(IDC_PROXYLABEL5)->EnableWindow(b);
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
	if (m_isEnabled)
	{
		CString temp;
		Store (m_serveraddress, m_regServeraddress);
		m_regServeraddress_copy = m_serveraddress;
		temp.Format(_T("%d"), m_serverport);
		Store (temp, m_regServerport);
		m_regServerport_copy = temp;
		Store (m_username, m_regUsername);
		m_regUsername_copy = m_username;
		Store (m_password, m_regPassword);
		m_regPassword_copy = m_password;
		temp.Format(_T("%d"), m_timeout);
		Store (temp, m_regTimeout);
		m_regTimeout_copy = temp;
		Store (m_Exceptions, m_regExceptions);
		m_regExceptions_copy = m_Exceptions;
	}
	else
	{
		m_regServeraddress.removeValue();
		m_regServerport.removeValue();
		m_regUsername.removeValue();
		m_regPassword.removeValue();
		m_regTimeout.removeValue();
		m_regExceptions.removeValue();

		CString temp;
		m_regServeraddress_copy = m_serveraddress;
		temp.Format(_T("%d"), m_serverport);
		m_regServerport_copy = temp;
		m_regUsername_copy = m_username;
		m_regPassword_copy = m_password;
		temp.Format(_T("%d"), m_timeout);
		m_regTimeout_copy = temp;
		m_regExceptions_copy = m_Exceptions;
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
