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
{
	m_dwDeliveryType = m_regDeliveryType;
}

CSettingSMTP::~CSettingSMTP()
{
}

void CSettingSMTP::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SMTPDELIVERYCOMBO, m_SMTPDeliveryTypeCombo);
}

BEGIN_MESSAGE_MAP(CSettingSMTP, CPropertyPage)
	ON_CBN_SELCHANGE(IDC_SMTPDELIVERYCOMBO, OnModifiedDeliveryCombo)
END_MESSAGE_MAP()

BOOL CSettingSMTP::OnInitDialog()
{
	ISettingsPropPage::OnInitDialog();

	m_SMTPDeliveryTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_SMTP_DIRECTLY)));
	CString mailCient;
	CMailMsg::DetectMailClient(mailCient);
	if (!mailCient.IsEmpty())
		m_SMTPDeliveryTypeCombo.AddString(CString(MAKEINTRESOURCE(IDS_SMTP_MAPI)));

	if ((int)m_dwDeliveryType >= m_SMTPDeliveryTypeCombo.GetCount())
		m_dwDeliveryType = 0;

	m_SMTPDeliveryTypeCombo.SetCurSel(m_dwDeliveryType);

	this->UpdateData(FALSE);

	return TRUE;
}

void CSettingSMTP::OnModifiedDeliveryCombo()
{
	m_dwDeliveryType = m_SMTPDeliveryTypeCombo.GetCurSel();
	SetModified();
}

BOOL CSettingSMTP::OnApply()
{
	CWaitCursor wait;
	this->UpdateData();

	m_regDeliveryType = m_dwDeliveryType;

	SetModified(FALSE);

	return ISettingsPropPage::OnApply();
}
