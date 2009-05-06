// ConfirmDelRefDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ConfirmDelRefDlg.h"
#include "Git.h"


// CConfirmDelRefDlg dialog

IMPLEMENT_DYNAMIC(CConfirmDelRefDlg, CDialog)

CConfirmDelRefDlg::CConfirmDelRefDlg(CString completeRefName, CWnd* pParent /*=NULL*/)
:	CDialog(CConfirmDelRefDlg::IDD, pParent),
	m_completeRefName(completeRefName)
{

}

CConfirmDelRefDlg::~CConfirmDelRefDlg()
{
}

void CConfirmDelRefDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_FORCE, m_butForce);
	DDX_Control(pDX, IDC_STATIC_MESSAGE, m_statMessage);
	DDX_Control(pDX, IDOK, m_butOK);
}


BEGIN_MESSAGE_MAP(CConfirmDelRefDlg, CDialog)
	ON_BN_CLICKED(IDC_CHECK_FORCE, OnBnForce)
END_MESSAGE_MAP()


// CConfirmDelRefDlg message handlers

BOOL CConfirmDelRefDlg::OnInitDialog()
{
	CDialog::OnInitDialog();
	CString csMessage;

	csMessage=L"Are you sure you want to delete the ";
	if(wcsncmp(m_completeRefName,L"refs/heads",10)==0)
	{
		SetWindowText(L"Confirm deletion of branch " + m_completeRefName);
		csMessage += "branch '";
		csMessage += m_completeRefName;
		csMessage += "'?";

		//Check if branch is fully merged in HEAD
		CString branchHash = g_Git.GetHash(m_completeRefName);
		CString commonAncestor;
		CString cmd;
		cmd.Format(L"git.exe merge-base HEAD %s",m_completeRefName);
		g_Git.Run(cmd,&commonAncestor,CP_UTF8);

		branchHash=branchHash.Left(40);
		commonAncestor=commonAncestor.Left(40);
		
		if(commonAncestor != branchHash)
		{
			m_butForce.ShowWindow(SW_SHOW);
			m_butOK.EnableWindow(FALSE);
			csMessage += L"\r\n\r\nWarning: this branch is not fully merged into HEAD. If you realy want to delete this branch, check the force flag.";
		}
	}
	else if(wcsncmp(m_completeRefName,L"refs/tags",9)==0)
	{
		SetWindowText(L"Confirm deletion of tag " + m_completeRefName);
		csMessage += "tag '";
		csMessage += m_completeRefName;
		csMessage += "'?";
	}

	m_statMessage.SetWindowText(csMessage);

	return TRUE;
}

void CConfirmDelRefDlg::OnOK()
{
	bool bForce = m_butForce.GetCheck()!=0;

	if(wcsncmp(m_completeRefName,L"refs/heads",10)==0)
	{
		CString branchToDelete = m_completeRefName.Mid(11);
		CString cmd;
		cmd.Format(L"git.exe branch -%c %s",bForce?L'D':L'd',branchToDelete);
		CString resultDummy;
		g_Git.Run(cmd,&resultDummy,CP_UTF8);
	}
	else if(wcsncmp(m_completeRefName,L"refs/tags",9)==0)
	{
		CString tagToDelete = m_completeRefName.Mid(10);
		CString cmd;
		cmd.Format(L"git.exe tag -d %s",tagToDelete);
		CString resultDummy;
		g_Git.Run(cmd,&resultDummy,CP_UTF8);
	}

	CDialog::OnOK();
}

void CConfirmDelRefDlg::OnBnForce()
{
	if(m_butForce.GetCheck()!=0)
		m_butOK.EnableWindow(TRUE);
	else if(wcsncmp(m_completeRefName,L"refs/heads",10)==0)
		m_butOK.EnableWindow(FALSE);
}