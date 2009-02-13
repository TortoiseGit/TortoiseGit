// RebaseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RebaseDlg.h"


// CRebaseDlg dialog

IMPLEMENT_DYNAMIC(CRebaseDlg, CResizableStandAloneDialog)

CRebaseDlg::CRebaseDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRebaseDlg::IDD, pParent)
    , m_bPickAll(FALSE)
    , m_bSquashAll(FALSE)
    , m_bEditAll(FALSE)
{

}

CRebaseDlg::~CRebaseDlg()
{
}

void CRebaseDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_REBASE_PROGRESS, m_ProgressBar);
    DDX_Control(pDX, IDC_STATUS_STATIC, m_CtrlStatusText);
    DDX_Check(pDX, IDC_PICK_ALL, m_bPickAll);
    DDX_Check(pDX, IDC_SQUASH_ALL, m_bSquashAll);
    DDX_Check(pDX, IDC_EDIT_ALL, m_bEditAll);
}


BEGIN_MESSAGE_MAP(CRebaseDlg, CResizableStandAloneDialog)
    ON_BN_CLICKED(IDC_PICK_ALL, &CRebaseDlg::OnBnClickedPickAll)
    ON_BN_CLICKED(IDC_SQUASH_ALL, &CRebaseDlg::OnBnClickedSquashAll)
    ON_BN_CLICKED(IDC_EDIT_ALL, &CRebaseDlg::OnBnClickedEditAll)
    ON_BN_CLICKED(IDC_REBASE_SPLIT, &CRebaseDlg::OnBnClickedRebaseSplit)
END_MESSAGE_MAP()

BOOL CRebaseDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	CRect rectDummy;
	//IDC_REBASE_DUMY_TAB
	
	CWnd *pwnd=this->GetDlgItem(IDC_REBASE_DUMY_TAB);
	pwnd->GetWindowRect(&rectDummy);
	
	if (!m_ctrlTabCtrl.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 0))
	{
		TRACE0("Failed to create output tab window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.SetResizeMode(CMFCTabCtrl::RESIZE_NO);
	// Create output panes:
	//const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
	const DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if (! this->m_FileListCtrl.Create(dwStyle,rectDummy,&this->m_ctrlTabCtrl,0) )
	{
		TRACE0("Failed to create output windows\n");
		return FALSE;      // fail to create
	}

	if( ! this->m_LogMessageCtrl.Create(_T("Scintilla"),_T("source"),0,rectDummy,&m_ctrlTabCtrl,0,0) )
	{
		TRACE0("Failed to create log message control");
		return FALSE;
	}

	m_ctrlTabCtrl.AddTab(&m_FileListCtrl,_T("Modified File"));
	m_ctrlTabCtrl.AddTab(&m_LogMessageCtrl,_T("Log Message"),1);

	return TRUE;
}
// CRebaseDlg message handlers

void CRebaseDlg::OnBnClickedPickAll()
{
    // TODO: Add your control notification handler code here
}

void CRebaseDlg::OnBnClickedSquashAll()
{
    // TODO: Add your control notification handler code here
}

void CRebaseDlg::OnBnClickedEditAll()
{
    // TODO: Add your control notification handler code here
}

void CRebaseDlg::OnBnClickedRebaseSplit()
{
    // TODO: Add your control notification handler code here
}
