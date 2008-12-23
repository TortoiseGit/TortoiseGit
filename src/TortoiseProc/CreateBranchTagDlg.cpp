// CreateBranchTagDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "CreateBranchTagDlg.h"


// CCreateBranchTagDlg dialog

IMPLEMENT_DYNAMIC(CCreateBranchTagDlg, CResizableStandAloneDialog)

CCreateBranchTagDlg::CCreateBranchTagDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CCreateBranchTagDlg::IDD, pParent)
{

}

CCreateBranchTagDlg::~CCreateBranchTagDlg()
{
}

void CCreateBranchTagDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_BRANCH, this->m_Branch);
	DDX_Control(pDX, IDC_COMBOBOXEX_TAGS, this->m_Tags);
	DDX_Control(pDX, IDC_COMBOBOXEX_VERSION, this->m_Version);
				
	DDX_Text(pDX, IDC_BRANCH_TAG, this->m_BranchTagName);
	DDX_Check(pDX,IDC_CHECK_FORCE,this->m_bForce);
	DDX_Check(pDX,IDC_CHECK_TRACK,this->m_bTrack);

}


BEGIN_MESSAGE_MAP(CCreateBranchTagDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()

BOOL CCreateBranchTagDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDC_COMBOBOXEX_BRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_TAGS, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_VERSION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_GROUP_BRANCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_BASEON, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);

	return TRUE;


}
// CCreateBranchTagDlg message handlers
