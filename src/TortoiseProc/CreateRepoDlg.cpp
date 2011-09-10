// CreateRepoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CreateRepoDlg.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "AppUtils.h"

// CCreateRepoDlg dialog

IMPLEMENT_DYNCREATE(CCreateRepoDlg, CStandAloneDialog)

CCreateRepoDlg::CCreateRepoDlg(CWnd* pParent /*=NULL*/)
	: CStandAloneDialog(CCreateRepoDlg::IDD, pParent)
{
	m_bBare = FALSE;
}

CCreateRepoDlg::~CCreateRepoDlg()
{
}

void CCreateRepoDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);

	DDX_Check(pDX,IDC_CHECK_BARE, m_bBare);
}

BOOL CCreateRepoDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	m_tooltips.Create(this);
	CString tt;
	tt.LoadString(IDS_CLONE_DEPTH_TT);
	m_tooltips.AddTool(IDC_EDIT_DEPTH,tt);
	m_tooltips.AddTool(IDC_CHECK_DEPTH,tt);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCreateRepoDlg, CStandAloneDialog)
	ON_BN_CLICKED(IDC_CHECK_BARE, &CCreateRepoDlg::OnBnClickedCheckBare)
END_MESSAGE_MAP()

// CCloneDlg message handlers

void CCreateRepoDlg::OnOK()
{
	UpdateData(TRUE);

	CStandAloneDialog::OnOK();
}

void CCreateRepoDlg::OnCancel()
{
	CStandAloneDialog::OnCancel();
}

void CCreateRepoDlg::OnBnClickedCheckBare()
{
	this->UpdateData();
}
BOOL CCreateRepoDlg::PreTranslateMessage(MSG* pMsg)
{
	m_tooltips.RelayEvent(pMsg);

	return CStandAloneDialog::PreTranslateMessage(pMsg);
}
