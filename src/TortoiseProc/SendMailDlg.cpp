// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#include "MailMsg.h"
// CSendMailDlg dialog

IMPLEMENT_DYNAMIC(CSendMailDlg, CResizableStandAloneDialog)

CSendMailDlg::CSendMailDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSendMailDlg::IDD, pParent)
	, m_To(_T(""))
	, m_CC(_T(""))
	, m_Subject(_T(""))

	, m_regAttach(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Attach"),0)
	, m_regCombine(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\Combine"),0)
	, m_regUseMAPI(_T("Software\\TortoiseGit\\TortoiseProc\\SendMail\\UseMAPI"),0)
{
	m_bAttachment  = m_regAttach;
	m_bCombine =     m_regCombine;
	m_bUseMAPI = m_regUseMAPI;
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
	DDX_Check(pDX, IDC_SENDMAIL_MAPI, m_bUseMAPI);
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
	ON_BN_CLICKED(IDC_SENDMAIL_MAPI, &CSendMailDlg::OnBnClickedSendmailMapi)
END_MESSAGE_MAP()

// CSendMailDlg message handlers

BOOL CSendMailDlg::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);

	return CDialog::PreTranslateMessage(pMsg);
}

BOOL CSendMailDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

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

	AdjustControlSize(IDC_SENDMAIL_ATTACHMENT);
	AdjustControlSize(IDC_SENDMAIL_COMBINE);
	AdjustControlSize(IDC_SENDMAIL_MAPI);
	AdjustControlSize(IDC_SENDMAIL_SETUP);

	EnableSaveRestore(_T("SendMailDlg"));

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, m_PathList.GetCommonRoot().GetUIPathString(), sWindowTitle);

	CString mailCient;
	CMailMsg::DetectMailClient(mailCient);
	if (mailCient.IsEmpty()) {
		m_bUseMAPI = false;
		GetDlgItem(IDC_SENDMAIL_MAPI)->EnableWindow(false);
		GetDlgItem(IDC_SENDMAIL_MAPI)->SendMessage(BM_SETCHECK, BST_UNCHECKED);
	}

	m_ctrlCC.Init();
	m_ctrlTO.Init();

	m_ctrlCC.SetSeparator(_T(";"));
	m_ctrlTO.SetSeparator(_T(";"));

	m_AddressReg.SetMaxHistoryItems(0xFFFF);

	m_AddressReg.Load(_T("Software\\TortoiseGit\\TortoiseProc\\EmailAddress\\"),_T("email"));
	for(int i=0;i<m_AddressReg.GetCount();i++)
	{
		m_ctrlCC.AddSearchString(m_AddressReg.GetEntry(i));
		m_ctrlTO.AddSearchString(m_AddressReg.GetEntry(i));
	}

	m_ctrlList.SetExtendedStyle( m_ctrlList.GetExtendedStyle()| LVS_EX_CHECKBOXES );

	for(int i=0;i<m_PathList.GetCount();i++)
	{
		m_ctrlList.InsertItem(i,m_PathList[i].GetWinPathString());
		m_ctrlList.SetCheck(i,true);
	}

//	m_ctrlCC.AddSearchString(_T("Tortoisegit-dev@google.com"));
//	m_ctrlTO.AddSearchString(_T("Tortoisegit-dev@google.com"));
	this->UpdateData(FALSE);
	OnBnClickedSendmailCombine();

	//Create the ToolTip control
	if( !m_ToolTip.Create(this))
	{
		TRACE0("Unable to create the ToolTip!");
	}
	else
	{
		m_ToolTip.SetMaxTipWidth(1024*8); // make multiline tooltips possible
		m_ToolTip.AddTool(GetDlgItem(IDC_SENDMAIL_MAPI), CString(MAKEINTRESOURCE(IDS_PROC_SENDMAIL_WRAPTOOLTIP)));
		m_ToolTip.Activate(TRUE);
	}

	return TRUE;
}
void CSendMailDlg::OnBnClickedSendmailCombine()
{
	this->UpdateData();
	this->GetDlgItem(IDC_SENDMAIL_SUBJECT)->EnableWindow(this->m_bCombine);
	if(m_bCombine)
		GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(this->m_Subject);

	UpdateSubject();
}

void CSendMailDlg::OnBnClickedOk()
{
	this->UpdateData();

	if(this->m_To.IsEmpty() && this->m_CC.IsEmpty())
	{
		CMessageBox::Show(NULL,IDS_ERR_ADDRESS_NO_EMPTY,IDS_APPNAME,MB_OK|MB_ICONERROR);
		return;
	}
	int start =0;
	CString Address;
	while(start>=0)
	{
		Address=this->m_CC.Tokenize(_T(";"),start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}
	start =0;
	while(start>=0)
	{
		Address=this->m_To.Tokenize(_T(";"),start);
		m_AddressReg.AddEntry(Address);
		m_AddressReg.Save();
	}

	this->m_PathList.Clear();
	for(int i=0;i<m_ctrlList.GetItemCount();i++)
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
	m_regUseMAPI=m_bUseMAPI;

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
				m_MapPatch[index].Parser(pathfile);
			}
			GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(m_MapPatch[index].m_Subject);
		}
		else
		{
			GetDlgItem(IDC_SENDMAIL_SUBJECT)->SetWindowText(_T(""));
		}
	}
}

void CSendMailDlg::OnLvnItemchangedSendmailPatchs(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
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
	if(this->m_bCombine)
		GetDlgItem(IDC_SENDMAIL_SUBJECT)->GetWindowText(this->m_Subject);
}

void CSendMailDlg::OnBnClickedSendmailMapi()
{
	this->UpdateData();
}
