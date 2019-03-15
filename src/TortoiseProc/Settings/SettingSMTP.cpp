// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2013-2016, 2018-2019 - TortoiseGit

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
#include "SendMail.h"
#include "MailMsg.h"
#include "WindowsCredentialsStore.h"
#include "UserPassword.h"

// CSettingSMTP dialog

IMPLEMENT_DYNAMIC(CSettingSMTP, ISettingsPropPage)

CSettingSMTP::CSettingSMTP()
	: ISettingsPropPage(CSettingSMTP::IDD)
	, m_regDeliveryType(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\DeliveryType", SEND_MAIL_SMTP_DIRECT)
	, m_regServer(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Address", L"")
	, m_regPort(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Port", 25)
	, m_regEncryption(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Encryption", 0)
	, m_regAuthenticate(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\AuthenticationRequired", FALSE)
{
	m_dwDeliveryType = m_regDeliveryType;
	m_Server = m_regServer;
	m_Port = m_regPort;
	m_dwSMTPEnrcyption = m_regEncryption;
	m_bAuth = m_regAuthenticate;
}

CSettingSMTP::~CSettingSMTP()
{
}

void CSettingSMTP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SMTPDELIVERYCOMBO, m_SMTPDeliveryTypeCombo);
	DDX_Control(pDX, IDC_SMTPENCRYPTIONCOMBO, m_SMTPEncryptionCombo);
	DDX_Check(pDX, IDC_SMTP_AUTH, m_bAuth);
	DDX_Text(pDX, IDC_SMTP_SERVER, m_Server);
	DDX_Text(pDX, IDC_SMTP_PORT, m_Port);
}

BEGIN_MESSAGE_MAP(CSettingSMTP, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_SMTPDELIVERYCOMBO, OnModifiedDeliveryCombo)
	ON_CBN_SELCHANGE(IDC_SMTPENCRYPTIONCOMBO, OnModifiedEncryptionCombo)
	ON_BN_CLICKED(IDC_SMTP_AUTH, OnBnClickedSmtpAuth)
	ON_CBN_SELCHANGE(IDC_SMTPENCRYPTIONCOMBO, OnModified)
	ON_EN_CHANGE(IDC_SMTP_SERVER, OnModified)
	ON_EN_CHANGE(IDC_SMTP_PORT, OnModified)
	ON_EN_CHANGE(IDC_SEND_ADDRESS, OnModified)
	ON_BN_CLICKED(IDC_STORECREDENTIALS, OnBnClickedStoreCredentials)
	ON_BN_CLICKED(IDC_CLEARCREDENTIALS, OnBnClickedClearCredentials)
END_MESSAGE_MAP()

BOOL CSettingSMTP::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	AdjustControlSize(IDC_SMTP_AUTH);

	int idx = m_SMTPDeliveryTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_SMTP_DIRECTLY)));
	m_SMTPDeliveryTypeCombo.SetItemData(idx, SEND_MAIL_SMTP_DIRECT);
	CString mailCient;
	CMailMsg::DetectMailClient(mailCient);
	if (!mailCient.IsEmpty())
	{
		idx = m_SMTPDeliveryTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_SMTP_MAPI)));
		m_SMTPDeliveryTypeCombo.SetItemData(idx, SEND_MAIL_MAPI);
	}
	idx = m_SMTPDeliveryTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_SMTP_CONFIGURED)));
	m_SMTPDeliveryTypeCombo.SetItemData(idx, SEND_MAIL_SMTP_CONFIGURED);

	if (static_cast<int>(m_dwDeliveryType) >= m_SMTPDeliveryTypeCombo.GetCount())
		m_dwDeliveryType = 0;

	m_SMTPDeliveryTypeCombo.SetCurSel(m_dwDeliveryType);

	m_SMTPEncryptionCombo.AddString(CString(MAKEINTRESOURCE(IDS_ENCRYPT_NONE)));
	m_SMTPEncryptionCombo.AddString(CString(MAKEINTRESOURCE(IDS_ENCRYPT_STARTTLS)));
	m_SMTPEncryptionCombo.AddString(CString(MAKEINTRESOURCE(IDS_ENCRYPT_SSL)));

	if (static_cast<int>(m_dwSMTPEnrcyption) >= m_SMTPEncryptionCombo.GetCount())
		m_dwSMTPEnrcyption = 0;

	m_SMTPEncryptionCombo.SetCurSel(m_dwSMTPEnrcyption);

	this->UpdateData(FALSE);

	OnModifiedDeliveryCombo();
	UpdateCredentialsButtons();

	return TRUE;
}

void CSettingSMTP::OnModified()
{
	SetModified();
}

void CSettingSMTP::OnModifiedEncryptionCombo()
{
	m_dwSMTPEnrcyption = m_SMTPEncryptionCombo.GetCurSel();
	SetModified();
}

void CSettingSMTP::OnModifiedDeliveryCombo()
{
	m_dwDeliveryType = static_cast<DWORD>(m_SMTPDeliveryTypeCombo.GetItemData(m_SMTPDeliveryTypeCombo.GetCurSel()));

	GetDlgItem(IDC_SMTP_USER)->EnableWindow(m_dwDeliveryType >= 2 && m_bAuth);
	GetDlgItem(IDC_STORECREDENTIALS)->EnableWindow(m_dwDeliveryType >= 2 && m_bAuth);
	GetDlgItem(IDC_SMTP_AUTH)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_SMTPENCRYPTIONCOMBO)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_SMTP_PORT)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_SMTP_SERVER)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_STATIC_SMTPENCRYPTION)->EnableWindow(m_dwDeliveryType >= 2);
	GetDlgItem(IDC_STATIC_SMTPLOGIN)->EnableWindow(m_dwDeliveryType >= 2 && m_bAuth);
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
	m_regEncryption = m_dwSMTPEnrcyption;

	SetModified(FALSE);

	return ISettingsPropPage::OnApply();
}

// CSettingSMTP message handlers
void CSettingSMTP::OnBnClickedSmtpAuth()
{
	UpdateData();
	GetDlgItem(IDC_SMTP_USER)->EnableWindow(m_bAuth);
	GetDlgItem(IDC_STATIC_SMTPLOGIN)->EnableWindow(m_bAuth);
	GetDlgItem(IDC_STORECREDENTIALS)->EnableWindow(m_bAuth);
	SetModified();
}

void CSettingSMTP::OnBnClickedStoreCredentials()
{
	UpdateData();
	CUserPassword dlg;
	GetDlgItem(IDC_SMTP_USER)->GetWindowText(dlg.m_UserName);
	if (dlg.DoModal() == IDOK)
		CWindowsCredentialsStore::SaveCredential(L"TortoiseGit:SMTP-Credentials", dlg.m_UserName, dlg.m_password);

	UpdateCredentialsButtons();
	UpdateData(FALSE);
}

void CSettingSMTP::OnBnClickedClearCredentials()
{
	CWindowsCredentialsStore::DeleteCredential(L"TortoiseGit:SMTP-Credentials");
	GetDlgItem(IDC_SMTP_USER)->SetWindowText(L"");
	GetDlgItem(IDC_CLEARCREDENTIALS)->EnableWindow(FALSE);
}

void CSettingSMTP::UpdateCredentialsButtons()
{
	CCredentials credentials;
	CWindowsCredentialsStore::GetCredential(L"TortoiseGit:SMTP-Credentials", credentials);
	GetDlgItem(IDC_SMTP_USER)->SetWindowText(credentials.m_username);
	GetDlgItem(IDC_CLEARCREDENTIALS)->EnableWindow(!credentials.m_username.IsEmpty());
}
