// RefLogDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "RefLogDlg.h"
#include "git.h"

// CRefLogDlg dialog

IMPLEMENT_DYNAMIC(CRefLogDlg, CResizableStandAloneDialog)

CRefLogDlg::CRefLogDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRefLogDlg::IDD, pParent)
{

}

CRefLogDlg::~CRefLogDlg()
{
}

void CRefLogDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_REF, m_ChooseRef);
	DDX_Control(pDX, IDC_REFLOG_LIST, m_RefList);
}


BEGIN_MESSAGE_MAP(CRefLogDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDOK, &CRefLogDlg::OnBnClickedOk)
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_REF,   &CRefLogDlg::OnCbnSelchangeRef)
END_MESSAGE_MAP()

BOOL CRefLogDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	
	AddAnchor(IDC_REFLOG_LIST,TOP_LEFT,BOTTOM_RIGHT);

	AddOthersToAnchor();
	this->EnableSaveRestore(_T("RefLogDlg"));

	STRING_VECTOR list;
	g_Git.GetRefList(list);

	this->m_ChooseRef.AddString(list);
	m_ChooseRef.SetMaxHistoryItems(0x7FFFFFFF);

	this->m_RefList.InsertRefLogColumn();
	m_RefList.m_logEntries.ParserFromRefLog(_T("master"));
	return TRUE;
}
// CRefLogDlg message handlers

void CRefLogDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}

void CRefLogDlg::OnCbnSelchangeRef()
{
	
}