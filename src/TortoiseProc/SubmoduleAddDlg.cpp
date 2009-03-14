// SubmoduleAddDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "SubmoduleAddDlg.h"


// CSubmoduleAddDlg dialog

IMPLEMENT_DYNAMIC(CSubmoduleAddDlg, CDialog)

CSubmoduleAddDlg::CSubmoduleAddDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CSubmoduleAddDlg::IDD, pParent)
	, m_bBranch(FALSE)
	, m_strBranch(_T(""))
{

}

CSubmoduleAddDlg::~CSubmoduleAddDlg()
{
}

void CSubmoduleAddDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_REPOSITORY, m_Repository);
	DDX_Control(pDX, IDC_COMBOBOXEX_PATH, m_PathCtrl);
	DDX_Check(pDX, IDC_BRANCH_CHECK, m_bBranch);
	DDX_Text(pDX, IDC_SUBMODULE_BRANCH, m_strBranch);
}


BEGIN_MESSAGE_MAP(CSubmoduleAddDlg, CDialog)
END_MESSAGE_MAP()


// CSubmoduleAddDlg message handlers
