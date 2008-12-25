// MergeDlg.cpp : implementation file
//

#include "stdafx.h"

#include "Git.h"
#include "TortoiseProc.h"
#include "MergeDlg.h"


#include "Messagebox.h"
// CMergeDlg dialog

IMPLEMENT_DYNAMIC(CMergeDlg, CResizableStandAloneDialog)

CMergeDlg::CMergeDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CMergeDlg::IDD, pParent),
	CChooseVersion(this)
{

}

CMergeDlg::~CMergeDlg()
{
}

void CMergeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	CHOOSE_VERSION_DDX;

	DDX_Check(pDX,IDC_CHECK_NOFF,this->m_bNoFF);
	DDX_Check(pDX,IDC_CHECK_SQUASH,this->m_bSquash);
}


BEGIN_MESSAGE_MAP(CMergeDlg, CResizableStandAloneDialog)
	CHOOSE_VERSION_EVENT
	ON_BN_CLICKED(IDOK, &CMergeDlg::OnBnClickedOk)
END_MESSAGE_MAP()


BOOL CMergeDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();


	CHOOSE_VERSION_ADDANCHOR;

	AddAnchor(IDC_GROUP_OPTION, TOP_LEFT, TOP_RIGHT);

	
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);


	CheckRadioButton(IDC_RADIO_BRANCH,IDC_RADIO_VERSION,IDC_RADIO_BRANCH);

	Init();
	
	this->SetDefaultChoose(IDC_RADIO_BRANCH);

	return TRUE;
}

// CMergeDlg message handlers


void CMergeDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	this->UpdateData(TRUE);
	
	this->UpdateRevsionName();

	OnOK();
}
