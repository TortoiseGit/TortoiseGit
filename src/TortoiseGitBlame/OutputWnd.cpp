// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//


#include "stdafx.h"

#include "OutputWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COutputBar

COutputWnd::COutputWnd()
{
}

COutputWnd::~COutputWnd()
{
}

IMPLEMENT_DYNCREATE(CGitMFCTabCtrl, CMFCTabCtrl)

BEGIN_MESSAGE_MAP(CGitMFCTabCtrl, CMFCTabCtrl)
	ON_NOTIFY(LVN_ITEMCHANGED, 0, OnLvnItemchangedLoglist)
END_MESSAGE_MAP()

void CGitMFCTabCtrl::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	COutputWnd *pWnd=DYNAMIC_DOWNCAST(COutputWnd,this->GetParent());
	pWnd->OnLvnItemchangedLoglist(pNMHDR,pResult);
}

IMPLEMENT_DYNAMIC(COutputWnd, CDockablePane)

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, 0, OnLvnItemchangedLoglist)
END_MESSAGE_MAP()

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	TRACE(_T("%u\n"),LVN_ITEMCHANGED);
	m_Font.CreateStockObject(DEFAULT_GUI_FONT);

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create tabs window:
	if (!m_wndTabs.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, 0))
	{
		TRACE0("Failed to create output tab window\n");
		return -1;      // fail to create
	}

	// Create output panes:
	//const DWORD dwStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL;
	const DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if (! m_LogList.Create(dwStyle,rectDummy,&m_wndTabs,0) )
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}
#if 0
	if (!m_wndOutputBuild.Create(dwStyle, rectDummy, &m_wndTabs, 2) ||
		!m_wndOutputDebug.Create(dwStyle, rectDummy, &m_wndTabs, 3) ||
		!m_wndOutputFind.Create(dwStyle, rectDummy, &m_wndTabs, 4))
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}
#endif

	m_LogList.SetFont(&m_Font);
	//m_wndOutputBuild.SetFont(&m_Font);
	//m_wndOutputDebug.SetFont(&m_Font);
	//m_wndOutputFind.SetFont(&m_Font);

	CString strTabName;
	BOOL bNameValid;

	// Attach list windows to tab:
	bNameValid = strTabName.LoadString(IDS_GIT_LOG_TAB);
	ASSERT(bNameValid);

	m_wndTabs.AddTab(&m_LogList, strTabName, (UINT)0);

	m_LogList.m_IsIDReplaceAction=TRUE;
	m_LogList.DeleteAllItems();
	m_LogList.m_ColumnRegKey=_T("Blame");
	m_LogList.InsertGitColumn();

	m_LogList.hideUnimplementedCommands();

#if 0
	bNameValid = strTabName.LoadString(IDS_DEBUG_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputDebug, strTabName, (UINT)1);
	bNameValid = strTabName.LoadString(IDS_FIND_TAB);
	ASSERT(bNameValid);
	m_wndTabs.AddTab(&m_wndOutputFind, strTabName, (UINT)2);
#endif;

	// Fill output tabs with some dummy text (nothing magic here)
	FillBuildWindow();
//	FillDebugWindow();
//	FillFindWindow();

	this->SetWindowTextW(_T("Git Log"));
	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// Tab control should cover the whole client area:
	m_wndTabs.SetWindowPos (NULL, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

void COutputWnd::AdjustHorzScroll(CListBox& wndListBox)
{
	CClientDC dc(this);
	CFont* pOldFont = dc.SelectObject(&m_Font);

	int cxExtentMax = 0;

	for (int i = 0; i < wndListBox.GetCount(); i ++)
	{
		CString strItem;
		wndListBox.GetText(i, strItem);

		cxExtentMax = max(cxExtentMax, dc.GetTextExtent(strItem).cx);
	}

	wndListBox.SetHorizontalExtent(cxExtentMax);
	dc.SelectObject(pOldFont);
}

void COutputWnd::FillBuildWindow()
{
//	m_wndOutputBuild.AddString(_T("Build output is being displayed here."));
//	m_wndOutputBuild.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputBuild.AddString(_T("but you can change the way it is displayed as you wish..."));
}

void COutputWnd::FillDebugWindow()
{
//	m_wndOutputDebug.AddString(_T("Debug output is being displayed here."));
//	m_wndOutputDebug.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputDebug.AddString(_T("but you can change the way it is displayed as you wish..."));
}

void COutputWnd::FillFindWindow()
{
//	m_wndOutputFind.AddString(_T("Find output is being displayed here."));
//	m_wndOutputFind.AddString(_T("The output is being displayed in rows of a list view"));
//	m_wndOutputFind.AddString(_T("but you can change the way it is displayed as you wish..."));
}

int COutputWnd::LoadHistory(CString filename, bool follow)
{
	m_LogList.ReloadHashMap();
	CTGitPath path;
	path.SetFromGit(filename);

	m_LogList.Clear();
	m_LogList.FillGitLog(&path, follow ? CGit::LOG_INFO_FOLLOW : 0);
	m_LogList.UpdateProjectProperties();
	return 0;

}
void COutputWnd::OnLvnItemchangedLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	//if (this->IsThreadRunning())
	if (pNMLV->iItem >= 0)
	{
		if (pNMLV->iSubItem != 0)
			return;

		if (pNMLV->uNewState & LVIS_SELECTED)
		{
			CMainFrame *pMain=DYNAMIC_DOWNCAST(CMainFrame,AfxGetApp()->GetMainWnd());
			POSITION pos=pMain->GetActiveDocument()->GetFirstViewPosition();
			CTortoiseGitBlameView *pView=DYNAMIC_DOWNCAST(CTortoiseGitBlameView,pMain->GetActiveDocument()->GetNextView(pos));
			pView->FocusOn(&this->m_LogList.m_logEntries.GetGitRevAt(pNMLV->iItem));
		}
	}
}
/////////////////////////////////////////////////////////////////////////////
// COutputList1

COutputList::COutputList()
{
}

COutputList::~COutputList()
{
}

BEGIN_MESSAGE_MAP(COutputList, CListBox)
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_COMMAND(ID_EDIT_CLEAR, OnEditClear)
	ON_COMMAND(ID_VIEW_OUTPUTWND, OnViewOutput)
	ON_WM_WINDOWPOSCHANGING()
END_MESSAGE_MAP()
/////////////////////////////////////////////////////////////////////////////
// COutputList message handlers

void COutputList::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	CMenu menu;
	menu.LoadMenu(IDR_OUTPUT_POPUP);

	CMenu* pSumMenu = menu.GetSubMenu(0);

	if (AfxGetMainWnd()->IsKindOf(RUNTIME_CLASS(CMDIFrameWndEx)))
	{
		CMFCPopupMenu* pPopupMenu = new CMFCPopupMenu;

		if (!pPopupMenu->Create(this, point.x, point.y, (HMENU)pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		((CMDIFrameWndEx*)AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void COutputList::OnEditCopy()
{
	MessageBox(_T("Copy output"));
}

void COutputList::OnEditClear()
{
	MessageBox(_T("Clear output"));
}

void COutputList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame != NULL && pParentBar != NULL)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();

	}
}

