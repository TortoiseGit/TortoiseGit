// SVNIgnoreTypeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "SVNIgnoreTypeDlg.h"


// CSVNIgnoreTypeDlg dialog

IMPLEMENT_DYNAMIC(CSVNIgnoreTypeDlg, CResizableStandAloneDialog)

CSVNIgnoreTypeDlg::CSVNIgnoreTypeDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CSVNIgnoreTypeDlg::IDD, pParent)
	, m_SVNIgnoreType(0)
{

}

CSVNIgnoreTypeDlg::~CSVNIgnoreTypeDlg()
{
}

void CSVNIgnoreTypeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Radio(pDX, IDC_RADIO_EXCLUDE, m_SVNIgnoreType);
}


BEGIN_MESSAGE_MAP(CSVNIgnoreTypeDlg, CResizableStandAloneDialog)
END_MESSAGE_MAP()


// CSVNIgnoreTypeDlg message handlers

BOOL CSVNIgnoreTypeDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	AddAnchor(IDC_RADIO_EXCLUDE,  TOP_LEFT);
	AddAnchor(IDC_RADIO_GITIGNORE,TOP_LEFT);
	AddAnchor(IDC_GROUP_TYPE,TOP_LEFT, BOTTOM_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);


	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
