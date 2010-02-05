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

void CPatchViewDlg::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face) 
{
	m_ctrlPatchView.Call(SCI_STYLESETFORE, style, fore);
	m_ctrlPatchView.Call(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		m_ctrlPatchView.Call(SCI_STYLESETSIZE, style, size);
	if (face) 
		m_ctrlPatchView.Call(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}


// CPatchViewDlg message handlers

BOOL CPatchViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  Add extra initialization here
	m_ctrlPatchView.Init(*m_pProjectProperties);
	m_ctrlPatchView.SetFont((CString)CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New")), (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogFontSize"), 8));

	SetAStyle(STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT), ::GetSysColor(COLOR_WINDOW),
		// Reusing TortoiseBlame's setting which already have an user friendly
		// pane in TortoiseSVN's Settings dialog, while there is no such
		// pane for TortoiseUDiff.
		CRegStdDWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10),
		WideToMultibyte(CRegStdString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New"))).c_str());

	m_ctrlPatchView.Call(SCI_SETTABWIDTH, 4);
	m_ctrlPatchView.Call(SCI_SETREADONLY, TRUE);
	//LRESULT pix = m_ctrlPatchView.Call(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)"_99999");
	//m_ctrlPatchView.Call(SCI_SETMARGINWIDTHN, 0, pix);
	//m_ctrlPatchView.Call(SCI_SETMARGINWIDTHN, 1);
	//m_ctrlPatchView.Call(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	m_ctrlPatchView.Call(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	m_ctrlPatchView.Call(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	m_ctrlPatchView.Call(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	m_ctrlPatchView.Call(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	m_ctrlPatchView.Call(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));

	//SendEditor(SCI_SETREADONLY, FALSE);
	m_ctrlPatchView.Call(SCI_CLEARALL);
	m_ctrlPatchView.Call(EM_EMPTYUNDOBUFFER);
	m_ctrlPatchView.Call(SCI_SETSAVEPOINT);
	m_ctrlPatchView.Call(SCI_CANCEL);
	m_ctrlPatchView.Call(SCI_SETUNDOCOLLECTION, 0);

	m_ctrlPatchView.Call(SCI_SETUNDOCOLLECTION, 1);
	m_ctrlPatchView.Call(SCI_SETWRAPMODE,SC_WRAP_NONE);
	
	//::SetFocus(m_hWndEdit);
	m_ctrlPatchView.Call(EM_EMPTYUNDOBUFFER);
	m_ctrlPatchView.Call(SCI_SETSAVEPOINT);
	m_ctrlPatchView.Call(SCI_GOTOPOS, 0);

	m_ctrlPatchView.Call(SCI_CLEARDOCUMENTSTYLE, 0, 0);
	m_ctrlPatchView.Call(SCI_SETSTYLEBITS, 5, 0);

	//SetAStyle(SCE_DIFF_DEFAULT, RGB(0, 0, 0));
	SetAStyle(SCE_DIFF_COMMAND, RGB(0x0A, 0x24, 0x36));
	SetAStyle(SCE_DIFF_POSITION, RGB(0xFF, 0, 0));
	SetAStyle(SCE_DIFF_HEADER, RGB(0x80, 0, 0), RGB(0xFF, 0xFF, 0x80));
	SetAStyle(SCE_DIFF_COMMENT, RGB(0, 0x80, 0));
	m_ctrlPatchView.Call(SCI_STYLESETBOLD, SCE_DIFF_COMMENT, TRUE);
	SetAStyle(SCE_DIFF_DELETED, ::GetSysColor(COLOR_WINDOWTEXT), RGB(0xFF, 0x80, 0x80));
	SetAStyle(SCE_DIFF_ADDED, ::GetSysColor(COLOR_WINDOWTEXT), RGB(0x80, 0xFF, 0x80));

	m_ctrlPatchView.Call(SCI_SETLEXER, SCLEX_DIFF);
	m_ctrlPatchView.Call(SCI_SETKEYWORDS, 0, (LPARAM)"revision");
	m_ctrlPatchView.Call(SCI_COLOURISE, 0, -1);

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
