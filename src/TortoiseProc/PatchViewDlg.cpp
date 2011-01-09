// PatchViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PatchViewDlg.h"
#include "Registry.h"
#include "CommitDlg.h"
#include "UnicodeUtils.h"
// CPatchViewDlg dialog

IMPLEMENT_DYNAMIC(CPatchViewDlg, CDialog)

CPatchViewDlg::CPatchViewDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CPatchViewDlg::IDD, pParent)
{

}

CPatchViewDlg::~CPatchViewDlg()
{
}

void CPatchViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PATCH, m_ctrlPatchView);
}

BEGIN_MESSAGE_MAP(CPatchViewDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_MOVING()
	ON_WM_DESTROY()
	ON_WM_CLOSE()
END_MESSAGE_MAP()

// CPatchViewDlg message handlers

BOOL CPatchViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	m_ctrlPatchView.Init(*m_pProjectProperties);
	m_ctrlPatchView.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));

	m_ctrlPatchView.SetUDiffStyle();

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPatchViewDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
	if (this->IsWindowVisible())
	{
		CRect rect;
		GetClientRect(rect);
		GetDlgItem(IDC_PATCH)->MoveWindow(rect.left, rect.top, cx, cy);
	}
}

void CPatchViewDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
	// TODO: Add your message handler code here
#define STICKYSIZE 5
	RECT parentRect;
	this->m_ParentCommitDlg->GetWindowRect(&parentRect);
	if (abs(parentRect.right - pRect->left) < STICKYSIZE)
	{
		int width = pRect->right - pRect->left;
		pRect->left = parentRect.right;
		pRect->right = pRect->left + width;
	}
	CDialog::OnMoving(fwSide, pRect);
}

void CPatchViewDlg::OnDestroy()
{
	CDialog::OnDestroy();

	this->m_ParentCommitDlg->ShowViewPatchText(true);
	// TODO: Add your message handler code here
}

void CPatchViewDlg::OnClose()
{
	// TODO: Add your message handler code here and/or call default
	CDialog::OnClose();
	this->DestroyWindow();
}
