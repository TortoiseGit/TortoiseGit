// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2013 - TortoiseGit

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
// SettingSMTP.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SettingSMTP.h"
#include "MailMsg.h"

// CSettingSMTP dialog

IMPLEMENT_DYNAMIC(CSettingSMTP, ISettingsPropPage)

CSettingSMTP::CSettingSMTP()
	: ISettingsPropPage(CSettingSMTP::IDD)
	, m_regDeliveryType(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\DeliveryType"), 0)
	, m_regServer(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Address"), _T(""))
	, m_regPort(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Port"), 25)
	, m_regAuthenticate(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\AuthenticationRequired"), FALSE)
	, m_regUsername(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Username"), _T(""))
	, m_regPassword(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Password"), _T(""))
{
	m_dwDeliveryType = m_regDeliveryType;
	m_Server = m_regServer;
	m_Port = m_regPort;
	m_bAuth = m_regAuthenticate;
	m_User = m_regUsername;
	m_Password = m_regPassword;
}

CSettingSMTP::~CSettingSMTP()
{
}

void CSettingSMTP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SMTPDELIVERYCOMBO, m_SMTPDeliveryTypeCombo);
	DDX_Check(pDX, IDC_SMTP_AUTH, m_bAuth);
	DDX_Text(pDX, IDC_SMTP_SERVER, m_Server);
	DDX_Text(pDX, IDC_SMTP_PORT, m_Port);
	DDX_Text(pDX, IDC_SMTP_USER, m_User);
	DDX_Text(pDX, IDC_SMTP_PASSWORD, m_Password);
}

BEGIN_MESSAGE_MAP(CSettingSMTP, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_SMTPDELIVERYCOMBO, OnModifiedDeliveryCombo)
	ON_BN_CLICKED(IDC_SMTP_AUTH, OnBnClickedSmtpAuth)
	ON_CBN_SELCHANGE(IDC_SMTPENCRYPTIONCOMBO, OnModified)
	ON_EN_CHANGE(IDC_SMTP_SERVER, OnModified)
	ON_EN_CHANGE(IDC_SMTP_PORT, OnModified)
	ON_EN_CHANGE(IDC_SEND_ADDRESS, OnModified)
	ON_EN_CHANGE(IDC_SMTP_USER, OnModified)
	ON_EN_CHANGE(IDC_SMTP_PASSWORD, OnModified)
END_MESSAGE_MAP()

BOOL CSettingSMTP::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_SMTPDeliveryTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_SMTP_DIRECTLY)));
	CString mailCient;
	CMailMsg::DetectMailClient(mailCient);
	if (!mailCient.IsEmpty())
		m_SMTPDeliveryTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_SMTP_MAPI)));
	m_SMTPDeliveryTypeCombo.AddString(_T("Use configured server"));

	if ((int)m_dwDeliveryType >= m_SMTPDeliveryTypeCombo.GetCount())
		m_dwDeliveryType = 0;

	m_SMTPDeliveryTypeCombo.SetCurSel(m_dwDeliveryType);

	this->UpdateData(FALSE);

	OnModifiedDeliveryCombo();
	OnBnClickedSmtpAuth();

	return TRUE;
}

void CSettingSMTP::OnModified()
{
	SetModified();
}

void CSettingSMTP::OnModifiedDeliveryCombo()
{
	m_dwDeliveryType = m_SMTPDeliveryTypeCombo.GetCurSel();

	GetDlgItem(IDC_SMTP_PASSWORD)->EnableWindow(m_dwDeliveryType >= 2 && m_bAuth);
	GetDlgItem(IDC_SMTP_USER)->EnableWindow(m_dwDeliveryType >= 2 && m_bAuth);
	GetDlgItem(IDC_SMTP_AUTH)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_SMTP_PORT)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_SMTP_SERVER)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_STATIC_SMTPLOGIN)->EnableWindow(m_dwDeliveryType >= 2 && m_bAuth);
	GetDlgItem(IDC_STATIC_SMTPPASSWORD)->EnableWindow(m_dwDeliveryType >= 2 && m_bAuth);
	GetDlgItem(IDC_STATIC_SMTPPORT)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_STATIC_SMTPSERVER)->EnableWindow(m_dwDeliveryType >= 2);

	SetModified();
}

BOOL CSettingSMTP::OnApply()
{
	CWaitCursor wait;
	this->UpdateData();

	m_regDeliveryType = m_dwDeliveryType;
	m_regServer = m_Server;
	m_regPort = m_Port;
	m_regAuthenticate = m_bAuth;
	m_regUsername = m_User;
	m_regPassword = m_Password;

	SetModified(FALSE);

	return ISettingsPropPage::OnApply();
}

// CSettingSMTP message handlers
void CSettingSMTP::OnBnClickedSmtpAuth()
{
	UpdateData();
	GetDlgItem(IDC_SMTP_USER)->EnableWindow(m_bAuth);
	GetDlgItem(IDC_SMTP_PASSWORD)->EnableWindow(m_bAuth);
	GetDlgItem(IDC_STATIC_SMTPLOGIN)->EnableWindow(m_bAuth);
	GetDlgItem(IDC_STATIC_SMTPPASSWORD)->EnableWindow(m_bAuth);
	SetModified();
}
