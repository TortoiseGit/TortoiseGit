// RebaseDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "RebaseDlg.h"


// CRebaseDlg dialog

IMPLEMENT_DYNAMIC(CRebaseDlg, CDialog)

CRebaseDlg::CRebaseDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRebaseDlg::IDD, pParent)
    , m_bPickupAll(false)
    , m_bSquashALL(false)
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


BEGIN_MESSAGE_MAP(CRebaseDlg, CDialog)
    ON_BN_CLICKED(IDC_PICK_ALL, &CRebaseDlg::OnBnClickedPickAll)
    ON_BN_CLICKED(IDC_SQUASH_ALL, &CRebaseDlg::OnBnClickedSquashAll)
    ON_BN_CLICKED(IDC_EDIT_ALL, &CRebaseDlg::OnBnClickedEditAll)
    ON_BN_CLICKED(IDC_REBASE_SPLIT, &CRebaseDlg::OnBnClickedRebaseSplit)
END_MESSAGE_MAP()


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
