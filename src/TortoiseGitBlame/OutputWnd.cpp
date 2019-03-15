// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2016, 2018-2019 - TortoiseGit

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

IMPLEMENT_DYNAMIC(COutputWnd, CDockablePane)

BEGIN_MESSAGE_MAP(COutputWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LOG, OnLvnItemchangedLoglist)
END_MESSAGE_MAP()

int COutputWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create output panes:
	const DWORD dwStyle =LVS_REPORT | LVS_SHOWSELALWAYS | LVS_ALIGNLEFT | LVS_OWNERDATA | WS_BORDER | WS_TABSTOP |LVS_SINGLESEL |WS_CHILD | WS_VISIBLE;

	if (!m_LogList.Create(dwStyle, rectDummy, this, IDC_LOG))
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}

	// for some unknown reason, the SetExtendedStyle in OnCreate/PreSubclassWindow is not working here
	m_LogList.SetStyle();

	m_Gravatar.Create(L"", WS_CHILD | WS_VISIBLE, rectDummy, this);
	bool bEnableGravatar = !!CRegDWORD(L"Software\\TortoiseGit\\EnableGravatar", FALSE);
	m_Gravatar.EnableGravatar(bEnableGravatar);
	if (bEnableGravatar)
		m_Gravatar.Init();

	CString strTabName;
	BOOL bNameValid;

	// Attach list windows to tab:
	bNameValid = strTabName.LoadString(IDS_GIT_LOG_TAB);
	ASSERT(bNameValid);

	m_LogList.m_IsIDReplaceAction=TRUE;
	m_LogList.DeleteAllItems();
	m_LogList.m_ColumnRegKey = L"Blame";
	m_LogList.InsertGitColumn();

	m_LogList.hideUnimplementedCommands();

	this->SetWindowTextW(CString(MAKEINTRESOURCE(IDS_GIT_LOG_TAB)));
	return 0;
}

void COutputWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);

	// Tab control should cover the whole client area:
	if (m_Gravatar.IsGravatarEnabled())
	{
		m_LogList.SetWindowPos(nullptr, -1, -1, cx - 80, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
		m_Gravatar.SetWindowPos(nullptr, cx - 80, 0, 80, 80, SWP_NOACTIVATE | SWP_NOZORDER);
		return;
	}
	m_LogList.SetWindowPos(nullptr, -1, -1, cx, cy, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
}

int COutputWnd::LoadHistory(CString filename, CString revision, bool follow)
{
	CTGitPath path;
	path.SetFromGit(filename);

	m_LogList.Clear();
	m_LogList.ShowGraphColumn(!follow);
	if (m_LogList.FillGitLog(&path, &revision, follow ? CGit::LOG_INFO_FOLLOW : 0))
		return -1;
	m_LogList.UpdateProjectProperties();
	return 0;
}

int COutputWnd::LoadHistory(std::unordered_set<CGitHash>& hashes)
{
	m_LogList.Clear();
	m_LogList.ShowGraphColumn(false);
	if (m_LogList.FillGitLog(hashes))
		return -1;
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
			m_Gravatar.LoadGravatar(m_LogList.m_logEntries.GetGitRevAt(pNMLV->iItem).GetAuthorEmail());
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

		if (!pPopupMenu->Create(this, point.x, point.y, pSumMenu->m_hMenu, FALSE, TRUE))
			return;

		static_cast<CMDIFrameWndEx*>(AfxGetMainWnd())->OnShowPopupMenu(pPopupMenu);
		UpdateDialogControls(this, FALSE);
	}

	SetFocus();
}

void COutputList::OnViewOutput()
{
	CDockablePane* pParentBar = DYNAMIC_DOWNCAST(CDockablePane, GetOwner());
	CMDIFrameWndEx* pMainFrame = DYNAMIC_DOWNCAST(CMDIFrameWndEx, GetTopLevelFrame());

	if (pMainFrame && pParentBar)
	{
		pMainFrame->SetFocus();
		pMainFrame->ShowPane(pParentBar, FALSE, FALSE, FALSE);
		pMainFrame->RecalcLayout();
	}
}
