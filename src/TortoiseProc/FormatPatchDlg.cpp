// FormatPatch.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "FormatPatchDlg.h"
#include "git.h"
#include "BrowseFolder.h"
#include "LogDlg.h"
#include "BrowseRefsDlg.h"
// CFormatPatchDlg dialog

IMPLEMENT_DYNAMIC(CFormatPatchDlg, CResizableStandAloneDialog)

CFormatPatchDlg::CFormatPatchDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CFormatPatchDlg::IDD, pParent),
	m_regSendMail(_T("Software\\TortoiseGit\\TortoiseProc\\FormatPatch\\SendMail"),0)
{
	m_Num=1;
	this->m_bSendMail = m_regSendMail;
	this->m_Radio = IDC_RADIO_SINCE;
}

CFormatPatchDlg::~CFormatPatchDlg()
{
}

void CFormatPatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_DIR,	m_cDir);
	DDX_Control(pDX, IDC_COMBOBOXEX_SINCE,	m_cSince);
	DDX_Control(pDX, IDC_COMBOBOXEX_FROM,	m_cFrom);
	DDX_Control(pDX, IDC_COMBOBOXEX_TO,		m_cTo);
	DDX_Control(pDX, IDC_EDIT_NUM,			m_cNum);

	DDX_Text(pDX,IDC_EDIT_NUM,m_Num);

	DDX_Text(pDX, IDC_COMBOBOXEX_DIR,	m_Dir);
	DDX_Text(pDX, IDC_COMBOBOXEX_SINCE,	m_Since);
	DDX_Text(pDX, IDC_COMBOBOXEX_FROM,	m_From);
	DDX_Text(pDX, IDC_COMBOBOXEX_TO,	m_To);
	
	DDX_Check(pDX, IDC_CHECK_SENDMAIL, m_bSendMail);
}


BEGIN_MESSAGE_MAP(CFormatPatchDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_BUTTON_DIR, &CFormatPatchDlg::OnBnClickedButtonDir)
	ON_BN_CLICKED(IDC_BUTTON_FROM, &CFormatPatchDlg::OnBnClickedButtonFrom)
	ON_BN_CLICKED(IDC_BUTTON_TO, &CFormatPatchDlg::OnBnClickedButtonTo)
	ON_BN_CLICKED(IDOK, &CFormatPatchDlg::OnBnClickedOk)
	ON_BN_CLICKED(IDC_RADIO_SINCE, &CFormatPatchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_NUM, &CFormatPatchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_RADIO_RANGE, &CFormatPatchDlg::OnBnClickedRadio)
	ON_BN_CLICKED(IDC_BUTTON_REF, &CFormatPatchDlg::OnBnClickedButtonRef)
END_MESSAGE_MAP()

BOOL CFormatPatchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	
	AddAnchor(IDC_GROUP_DIR, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_DIR,TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_DIR, TOP_RIGHT);

	AddAnchor(IDC_GROUP_VERSION, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_SINCE, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_EDIT_NUM, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_SPIN_NUM, TOP_RIGHT);

	AddAnchor(IDC_COMBOBOXEX_FROM, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_COMBOBOXEX_TO, TOP_LEFT, TOP_RIGHT);

	AddAnchor(IDC_BUTTON_FROM,  TOP_RIGHT);
	AddAnchor(IDC_BUTTON_TO,	TOP_RIGHT);
	AddAnchor(IDC_CHECK_SENDMAIL,BOTTOM_LEFT);
	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	this->AddOthersToAnchor();

	m_cDir.SetPathHistory(TRUE);
	m_cDir.LoadHistory(_T("Software\\TortoiseGit\\History\\FormatPatchURLS"), _T("path"));
	m_cDir.SetCurSel(0);

	STRING_VECTOR list;
	g_Git.GetBranchList(list,NULL,CGit::BRANCH_ALL);
	m_cSince.SetMaxHistoryItems(list.size());
	m_cSince.AddString(list);

	if(!m_Since.IsEmpty())
		m_cSince.SetWindowText(m_Since);

	m_cFrom.LoadHistory(_T("Software\\TortoiseGit\\History\\FormatPatchFromURLS"), _T("ver"));
	m_cFrom.SetCurSel(0);

	if(!m_From.IsEmpty())
		m_cFrom.SetWindowText(m_From);

	m_cTo.LoadHistory(_T("Software\\TortoiseGit\\History\\FormatPatchToURLS"), _T("ver"));
	m_cTo.SetCurSel(0);

	if(!m_To.IsEmpty())
		m_cTo.SetWindowText(m_To);

	this->CheckRadioButton(IDC_RADIO_SINCE,IDC_RADIO_RANGE,this->m_Radio);
	
	OnBnClickedRadio();

	EnableSaveRestore(_T("FormatPatchDlg"));
	return TRUE;
}
// CFormatPatchDlg message handlers

void CFormatPatchDlg::OnBnClickedButtonDir()
{
	// TODO: Add your control notification handler code here
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	CString strCloneDirectory;
	this->UpdateData(TRUE);
	strCloneDirectory=m_Dir;
	if (browseFolder.Show(GetSafeHwnd(), strCloneDirectory) == CBrowseFolder::OK) 
	{
		m_Dir=strCloneDirectory;
		this->UpdateData(FALSE);
	}
}

void CFormatPatchDlg::OnBnClickedButtonFrom()
{
	CLogDlg dlg;
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if ( dlg.DoModal() == IDOK )
	{
		// get selected hash if any
		CString selectedHash = dlg.GetSelectedHash();
		// load into window, do this even if empty so that it is clear that nothing has been selected
		m_cFrom.AddString(selectedHash);
		CheckRadioButton(IDC_RADIO_SINCE, IDC_RADIO_RANGE, IDC_RADIO_RANGE);
		OnBnClickedRadio();
	}
}

void CFormatPatchDlg::OnBnClickedButtonTo()
{
	CLogDlg dlg;
	// tell the dialog to use mode for selecting revisions
	dlg.SetSelect(true);
	// only one revision must be selected however
	dlg.SingleSelection(true);
	if ( dlg.DoModal() == IDOK )
	{
		// get selected hash if any
		CString selectedHash = dlg.GetSelectedHash();
		// load into window, do this even if empty so that it is clear that nothing has been selected
		m_cTo.AddString(selectedHash);
		CheckRadioButton(IDC_RADIO_SINCE, IDC_RADIO_RANGE, IDC_RADIO_RANGE);
		OnBnClickedRadio();
	}
}

void CFormatPatchDlg::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here

	m_cDir.SaveHistory();
	m_cFrom.SaveHistory();
	m_cTo.SaveHistory();
	this->UpdateData(TRUE);
	this->m_Radio=GetCheckedRadioButton(IDC_RADIO_SINCE,IDC_RADIO_RANGE);

	m_regSendMail=this->m_bSendMail;
	OnOK();
}

void CFormatPatchDlg::OnBnClickedRadio()
{
	// TODO: Add your control notification handler code here
	int radio=this->GetCheckedRadioButton(IDC_RADIO_SINCE,IDC_RADIO_RANGE);
	m_cSince.EnableWindow(FALSE);
	m_cNum.EnableWindow(FALSE);
	m_cFrom.EnableWindow(FALSE);
	m_cTo.EnableWindow(FALSE);
	switch(radio)
	{
	case IDC_RADIO_SINCE:
		m_cSince.EnableWindow(TRUE);
		break;
	case IDC_RADIO_NUM:
		m_cNum.EnableWindow(TRUE);
		break;
	case IDC_RADIO_RANGE:
		m_cFrom.EnableWindow(TRUE);
		m_cTo.EnableWindow(TRUE);
	}
}

void CFormatPatchDlg::OnBnClickedButtonRef()
{
	if(CBrowseRefsDlg::PickRefForCombo(&m_cSince, gPickRef_NoTag))
	{
		CheckRadioButton(IDC_RADIO_SINCE, IDC_RADIO_RANGE, IDC_RADIO_SINCE);
		OnBnClickedRadio();
	}
}
