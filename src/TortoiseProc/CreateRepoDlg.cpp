// CreateRepoDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CreateRepoDlg.h"
#include "BrowseFolder.h"
#include "MessageBox.h"
#include "AppUtils.h"
// CCreateRepoDlg dialog

IMPLEMENT_DYNCREATE(CCreateRepoDlg, CResizableStandAloneDialog)

CCreateRepoDlg::CCreateRepoDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCreateRepoDlg::IDD, pParent)
{

	m_bBare = FALSE;

}

CCreateRepoDlg::~CCreateRepoDlg()
{
}

void CCreateRepoDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);

	DDX_Check(pDX,IDC_CHECK_BARE, m_bBare);

}

BOOL CCreateRepoDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	//AddAnchor(IDC_URLCOMBO, TOP_LEFT, TOP_RIGHT);
	//AddAnchor(IDC_CLONE_BROWSE_URL, TOP_RIGHT);
	//AddAnchor(IDC_CLONE_DIR, TOP_LEFT,TOP_RIGHT);
	//AddAnchor(IDC_CLONE_DIR_BROWSE, TOP_RIGHT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

    //AddAnchor(IDC_GROUP_CLONE,TOP_LEFT,TOP_RIGHT);
    //AddAnchor(IDC_PUTTYKEYFILE_BROWSE,TOP_RIGHT);
    //AddAnchor(IDC_PUTTYKEY_AUTOLOAD,TOP_LEFT);
    //AddAnchor(IDC_PUTTYKEYFILE,TOP_LEFT,TOP_RIGHT);
	//AddAnchor(IDC_CLONE_GROUP_SVN,TOP_LEFT,TOP_RIGHT);
	//AddAnchor(IDHELP, BOTTOM_RIGHT);

	m_tooltips.Create(this);
	CString tt;
	tt.LoadString(IDS_CLONE_DEPTH_TT);
	m_tooltips.AddTool(IDC_EDIT_DEPTH,tt);
	m_tooltips.AddTool(IDC_CHECK_DEPTH,tt);

	this->AddOthersToAnchor();

/*
	CWnd *window=this->GetDlgItem(IDC_CLONE_DIR);
	if(window)
		SHAutoComplete(window->m_hWnd, SHACF_FILESYSTEM);
  */     
    EnableSaveRestore(_T("CreateRepoDlg"));
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

BEGIN_MESSAGE_MAP(CCreateRepoDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_CHECK_BARE, &CCreateRepoDlg::OnBnClickedCheckBare)
END_MESSAGE_MAP()



// CCloneDlg message handlers

void CCreateRepoDlg::OnOK()
{
	UpdateData(TRUE);

	CResizableDialog::OnOK();
}

void CCreateRepoDlg::OnCancel()
{
	CResizableDialog::OnCancel();
}

void CCreateRepoDlg::OnBnClickedCheckBare()
{
	this->UpdateData();
}
BOOL CCreateRepoDlg::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	m_tooltips.RelayEvent(pMsg);

	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

