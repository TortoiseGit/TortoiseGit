// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2017 - TortoiseGit

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
// SendMailDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SendMailDlg.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PatchListCtrl.h"
#include "SendMailPatch.h"

// CSendMailDlg dialog

IMPLEMENT_DYNAMIC(CSendMailDlg, CResizableStandAloneDialog)

CSendMailDlg::CSendMailDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CSendMailDlg::IDD, pParent)
	, m_bCustomSubject(FALSE)
	, m_regAttach(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Attach", 0)
	, m_regCombine(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Combine", 0)
{
	m_bAttachment  = m_regAttach;
	m_bCombine =     m_regCombine;
	this->m_ctrlList.m_ContextMenuMask &=~ m_ctrlList.GetMenuMask(CPatchListCtrl::MENU_SENDMAIL);
}

CSendMailDlg::~CSendMailDlg()
{
}

void CSendMailDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_SENDMAIL_TO, m_To);
	DDX_Text(pDX, IDC_SENDMAIL_CC, m_CC);
	DDX_Check(pDX, IDC_SENDMAIL_ATTACHMENT, m_bAttachment);
	DDX_Check(pDX, IDC_SENDMAIL_COMBINE, m_bCombine);
	DDX_Control(pDX, IDC_SENDMAIL_PATCHS, m_ctrlList);
	DDX_Control(pDX,IDC_SENDMAIL_SETUP, this->m_SmtpSetup);
	DDX_Control(pDX,IDC_SENDMAIL_TO,m_ctrlTO);
	DDX_Control(pDX,IDC_SENDMAIL_CC,m_ctrlCC);
}


BEGIN_MESSAGE_MAP(CSendMailDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SENDMAIL_COMBINE, &CSendMailDlg::OnBnClickedSendmailCombine)
	ON_BN_CLICKED(IDOK, &CSendMailDlg::OnBnClickedOk)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_SENDMAIL_PATCHS, &CSendMailDlg::OnLvnItemchangedSendmailPatchs)
	ON_NOTIFY(NM_DBLCLK, IDC_SENDMAIL_PATCHS, &CSendMailDlg::OnNMDblclkSendmailPatchs)
	ON_EN_CHANGE(IDC_SENDMAIL_SUBJECT, &CSendMailDlg::OnEnChangeSendmailSubject)
	ON_STN_CLICKED(IDC_SENDMAIL_SETUP, &CSendMailDlg::OnStnClickedSendmailSetup)
END_MESSAGE_MAP()

// CSendMailDlg message handlers

BOOL CSendMailDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	AdjustControlSize(IDC_SENDMAIL_ATTACHMENT);
	AdjustControlSize(IDC_SENDMAIL_COMBINE);
	AdjustControlSize(IDC_SENDMAIL_SETUP);

	AddAnchor(IDC_SENDMAIL_GROUP,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_TO,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_CC,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_SUBJECT,TOP_LEFT,TOP_RIGHT);
	AddAnchor(IDC_SENDMAIL_SETUP,TOP_RIGHT);

	AddAnchor(IDC_SENDMAIL_PATCHS,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	EnableSaveRestore(L"SendMailDlg");

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, m_PathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

	m_ctrlCC.Init();
	m_ctrlTO.Init();

	m_ctrlCC.SetSeparator(L";");
	m_ctrlTO.SetSeparator(L";");

	m_AddressReg.SetMaxHistoryItems(0xFFFF);

	m_AddressReg.Load(L"Software\\TortoiseGit\\TortoiseProc\\EmailAddress\\", L"email");
	for (size_t i = 0; i < m_AddressReg.GetCount(); ++i)
	{
		m_ctrlCC.AddSearchString(m_AddressReg.GetEntry(i));
		m_ctrlTO.AddSearchString(m_AddressReg.GetEntry(i));
	}

	m_ctrlList.SetExtendedStyle( m_ctrlList.GetExtendedStyle()| LVS_EX_CHECKBOXES );

	for (int i = 0; i < m_PathList.GetCount(); ++i)
	{
		m_ctrlList.InsertItem(i,m_PathList[i].GetWinPathString());
		m_ctrlList.SetCheck(i,true);
	}

	if (m_PathList.GetCount() == 1)
		m_ctrlList.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);

	this->UpdateData(FALSE);
	OnBnClickedSendmailCombine();

	m_SmtpSetup.SetURL(CString());

	return TRUE;
}
void CSendMailDlg::OnBnClickedSendmailCombine()
{
	this->UpdateData();

	if (m_bCustomSubject)
		return;

	this->GetDlgItem(IDC_SENDMAIL_SUBJECT)->EnableWindow(this->m_bCombine);
	if(m_bCombine)
		GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(this->m_Subject);

	UpdateSubject();
}

void CSendMailDlg::OnBnClickedOk()
{
	this->UpdateData();

	if (m_To.IsEmpty() && m_CC.IsEmpty() && CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\DeliveryType", SEND_MAIL_MAPI) != SEND_MAIL_MAPI)
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_ADDRESS_NO_EMPTY, IDS_APPNAME, MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	int start =0;
	CString Address;
	while(start>=0)
	{
		Address = m_CC.Tokenize(L";", start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}
	start =0;
	while(start>=0)
	{
		Address = m_To.Tokenize(L";", start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}

	this->m_PathList.Clear();
	for (int i = 0; i < m_ctrlList.GetItemCount(); ++i)
	{
		CTGitPath path;
		if(m_ctrlList.GetCheck(i))
		{
			path.SetFromWin(m_ctrlList.GetItemText(i,0));
			this->m_PathList.AddPath(path);
		}
	}

	m_regAttach=m_bAttachment;
	m_regCombine=m_bCombine;

	OnOK();
}

void CSendMailDlg::UpdateSubject()
{
	this->UpdateData();

	if(!this->m_bCombine)
	{
		if(m_ctrlList.GetSelectedCount()==1)
		{
			POSITION pos=m_ctrlList.GetFirstSelectedItemPosition();
			int index=m_ctrlList.GetNextSelectedItem(pos);
			if(this->m_MapPatch.find(index) == m_MapPatch.end() )
			{
				CString pathfile=m_ctrlList.GetItemText(index,0);
				m_MapPatch[index].Parse(pathfile, false);
			}
			GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(m_MapPatch[index].m_Subject);
		}
		else
			GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(L"");
	}
}

void CSendMailDlg::OnLvnItemchangedSendmailPatchs(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	if (!m_bCustomSubject)
		UpdateSubject();

	*pResult = 0;
}

void CSendMailDlg::OnNMDblclkSendmailPatchs(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	CString path=this->m_ctrlList.GetItemText(pNMItemActivate->iItem,0);
	CTGitPath gitpath;
	gitpath.SetFromWin(path);

	CAppUtils::StartUnifiedDiffViewer(path,gitpath.GetFilename());

	*pResult = 0;
}

void CSendMailDlg::OnEnChangeSendmailSubject()
{
	this->UpdateData();
	if (m_bCombine || m_bCustomSubject)
		GetDlgItem(IDC_SENDMAIL_SUBJECT)->GetWindowText(this->m_Subject);
}

void CSendMailDlg::OnStnClickedSendmailSetup()
{
	CCommonAppUtils::RunTortoiseGitProc(L"/command:settings /page:smtp");
}
