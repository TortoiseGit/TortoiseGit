// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016 - TortoiseGit
// Copyright (C) 2005-2007 Marco Costalba

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
// GitLogList.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "GitLogListBase.h"
#include "IconMenu.h"
#include "cursor.h"
#include "GitProgressDlg.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "registry.h"
#include "LoglistUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "IconMenu.h"
#include "GitStatus.h"
#include "..\TortoiseShell\Resource.h"
#include "FindDlg.h"

template < typename Cont, typename Pred>
void for_each(Cont& c, Pred p)
{
	std::for_each(cbegin(c), cend(c), p);
}

template<typename T>
using const_iterator = typename T::const_iterator;

template <typename Cont, typename Pred>
const_iterator<Cont> find_if(Cont& c, Pred p)
{
	return std::find_if(cbegin(c), cend(c), p);
}

const UINT CGitLogListBase::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);
const UINT CGitLogListBase::m_ScrollToMessage = RegisterWindowMessage(_T("TORTOISEGIT_LOG_SCROLLTO"));
const UINT CGitLogListBase::m_RebaseActionMessage = RegisterWindowMessage(_T("TORTOISEGIT_LOG_REBASEACTION"));

IMPLEMENT_DYNAMIC(CGitLogListBase, CHintListCtrl)

CGitLogListBase::CGitLogListBase():CHintListCtrl()
	,m_regMaxBugIDColWidth(_T("Software\\TortoiseGit\\MaxBugIDColWidth"), 200)
	,m_nSearchIndex(0)
	,m_bNoDispUpdates(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bStrictStopped(false)
	, m_SelectedFilters(LOGFILTER_ALL)
	, m_ShowFilter(FILTERSHOW_ALL)
	, m_bShowWC(false)
	, m_logEntries(&m_LogCache)
	, m_pFindDialog(nullptr)
	, m_ColumnManager(this)
	, m_dwDefaultColumns(0)
	, m_arShownList(&m_critSec)
	, m_hasWC(true)
	, m_bNoHightlightHead(FALSE)
	, m_ShowRefMask(LOGLIST_SHOWALLREFS)
	, m_bFullCommitMessageOnLogLine(false)
	, m_OldTopIndex(-1)
	, m_AsyncThreadRunning(FALSE)
	, m_AsyncThreadExit(FALSE)
	, m_bIsCherryPick(false)
{
	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont.CreateFontIndirect(&lf);
	lf.lfWeight = FW_DONTCARE;
	lf.lfItalic = TRUE;
	m_FontItalics.CreateFontIndirect(&lf);
	lf.lfWeight = FW_BOLD;
	m_boldItalicsFont.CreateFontIndirect(&lf);

	m_bShowBugtraqColumn=false;

	m_IsIDReplaceAction=FALSE;

	this->m_critSec.Init();
	ResetWcRev(false);

	m_hModifiedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hConflictedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONCONFLICTED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hFetchIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONFETCHING), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), FALSE);
	m_bFilterCaseSensitively = !!CRegDWORD(_T("Software\\TortoiseGit\\FilterCaseSensitively"), FALSE);

	m_Filter.m_NumberOfLogsScale = (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogDialog\\NumberOfLogsScale"), CFilterData::SHOW_NO_LIMIT);
	if (m_Filter.m_NumberOfLogsScale == CFilterData::SHOW_LAST_SEL_DATE)
	{
		CString key;
		key.Format(_T("Software\\TortoiseGit\\History\\LogDlg_Limits\\%s\\FromDate"), (LPCTSTR)g_Git.m_CurrentDir);
		key.Replace(_T(':'), _T('_'));
		CString lastSelFromDate = CRegString(key);
		if (lastSelFromDate.GetLength() == 10)
		{
			CTime time = CTime(_wtoi((LPCTSTR)lastSelFromDate.Mid(0, 4)), _wtoi((LPCTSTR)lastSelFromDate.Mid(5, 2)), _wtoi((LPCTSTR)lastSelFromDate.Mid(8, 2)), 0, 0, 0);
			m_Filter.m_From = (DWORD)time.GetTime();
		}
	}
	m_Filter.m_NumberOfLogs = (DWORD)CRegDWORD(_T("Software\\TortoiseGit\\LogDialog\\NumberOfLogs"), 1);

	m_ShowMask = 0;
	m_LoadingThread = nullptr;

	InterlockedExchange(&m_bExitThread,FALSE);
	m_IsOldFirst = FALSE;
	m_IsRebaseReplaceGraph = FALSE;

	for (int i = 0; i < Lanes::COLORS_NUM; ++i)
	{
		m_LineColors[i] = m_Colors.GetColor((CColors::Colors)(CColors::BranchLine1+i));
	}
	// get short/long datetime setting from registry
	DWORD RegUseShortDateFormat = CRegDWORD(_T("Software\\TortoiseGit\\LogDateFormat"), TRUE);
	if ( RegUseShortDateFormat )
	{
		m_DateFormat = DATE_SHORTDATE;
	}
	else
	{
		m_DateFormat = DATE_LONGDATE;
	}
	// get relative time display setting from registry
	DWORD regRelativeTimes = CRegDWORD(_T("Software\\TortoiseGit\\RelativeTimes"), FALSE);
	m_bRelativeTimes = (regRelativeTimes != 0);
	m_ContextMenuMask = 0xFFFFFFFFFFFFFFFF;

	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_PICK);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_SQUASH);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_EDIT);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_SKIP);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_LOG);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_BLAME);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_BLAMEPREVIOUS);

	m_ColumnRegKey=_T("log");

	m_bTagsBranchesOnRightSide = !!CRegDWORD(_T("Software\\TortoiseGit\\DrawTagsBranchesOnRightSide"), FALSE);
	m_bSymbolizeRefNames = !!CRegDWORD(_T("Software\\TortoiseGit\\SymbolizeRefNames"), FALSE);
	m_bIncludeBoundaryCommits = !!CRegDWORD(_T("Software\\TortoiseGit\\LogIncludeBoundaryCommits"), FALSE);
	m_bFullCommitMessageOnLogLine = !!CRegDWORD(_T("Software\\TortoiseGit\\FullCommitMessageOnLogLine"), FALSE);

	m_LineWidth = max(1, CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\Graph\\LogLineWidth"), 2));
	m_NodeSize = max(1, CRegDWORD(_T("Software\\TortoiseGit\\TortoiseProc\\Graph\\LogNodeSize"), 10));

	m_AsyncDiffEvent = ::CreateEvent(nullptr, FALSE, TRUE, nullptr);
	m_AsynDiffListLock.Init();
	StartAsyncDiffThread();
}

int CGitLogListBase::AsyncDiffThread()
{
	while(!m_AsyncThreadExit)
	{
		::WaitForSingleObject(m_AsyncDiffEvent, INFINITE);

		GitRevLoglist* pRev = nullptr;
		while(!m_AsyncThreadExit && !m_AsynDiffList.empty())
		{
			m_AsynDiffListLock.Lock();
			pRev = m_AsynDiffList.back();
			m_AsynDiffList.pop_back();
			m_AsynDiffListLock.Unlock();

			if( pRev->m_CommitHash.IsEmpty() )
			{
				if(pRev->m_IsDiffFiles)
					continue;

				CTGitPathList& files = pRev->GetFiles(this);
				files.Clear();
				pRev->m_ParentHash.clear();
				pRev->m_ParentHash.push_back(m_HeadHash);
				g_Git.GetWorkingTreeChanges(files);
				int& action = pRev->GetAction(this);
				action = 0;
				for (int j = 0; j < files.GetCount(); ++j)
					action |= files[j].m_Action;

				CString err;
				if (pRev->GetUnRevFiles().FillUnRev(CTGitPath::LOGACTIONS_UNVER, nullptr, &err))
				{
					MessageBox(_T("Failed to get UnRev file list\n") + err, _T("TortoiseGit"), MB_OK | MB_ICONERROR);
					InterlockedExchange(&m_AsyncThreadRunning, FALSE);
					return -1;
				}

				InterlockedExchange(&pRev->m_IsDiffFiles, TRUE);
				InterlockedExchange(&pRev->m_IsFull, TRUE);

				pRev->GetBody().Format(IDS_FILESCHANGES, files.GetCount());
				::PostMessage(m_hWnd,MSG_LOADED,(WPARAM)0,0);
				this->GetParent()->PostMessage(WM_COMMAND, MSG_FETCHED_DIFF, 0);
			}

			if (!pRev->CheckAndDiff())
			{	// fetch change file list
				for (int i = GetTopIndex(); !m_AsyncThreadExit && i <= GetTopIndex() + GetCountPerPage(); ++i)
				{
					if (i < (int)m_arShownList.size())
					{
						GitRevLoglist* data = m_arShownList.SafeGetAt(i);
						if(data->m_CommitHash == pRev->m_CommitHash)
						{
							::PostMessage(m_hWnd,MSG_LOADED,(WPARAM)i,0);
							break;
						}
					}
				}

				if(!m_AsyncThreadExit && GetSelectedCount() == 1)
				{
					POSITION pos = GetFirstSelectedItemPosition();
					int nItem = GetNextSelectedItem(pos);

					if(nItem>=0)
					{
						GitRevLoglist* data = m_arShownList.SafeGetAt(nItem);
						if(data)
							if(data->m_CommitHash == pRev->m_CommitHash)
								this->GetParent()->PostMessage(WM_COMMAND, MSG_FETCHED_DIFF, 0);
					}
				}
			}
		}
	}
	InterlockedExchange(&m_AsyncThreadRunning, FALSE);
	return 0;
}
void CGitLogListBase::hideFromContextMenu(unsigned __int64 hideMask, bool exclusivelyShow)
{
	if (exclusivelyShow)
		m_ContextMenuMask &= hideMask;
	else
		m_ContextMenuMask &= ~hideMask;
}

CGitLogListBase::~CGitLogListBase()
{
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	this->m_arShownList.SafeRemoveAll();

	DestroyIcon(m_hModifiedIcon);
	DestroyIcon(m_hReplacedIcon);
	DestroyIcon(m_hConflictedIcon);
	DestroyIcon(m_hAddedIcon);
	DestroyIcon(m_hDeletedIcon);
	m_logEntries.ClearAll();

	SafeTerminateThread();
	SafeTerminateAsyncDiffThread();

	if(m_AsyncDiffEvent)
		CloseHandle(m_AsyncDiffEvent);
}


BEGIN_MESSAGE_MAP(CGitLogListBase, CHintListCtrl)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
	ON_REGISTERED_MESSAGE(m_ScrollToMessage, OnScrollToMessage)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdrawLoglist)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetdispinfoLoglist)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkLoglist)
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM,OnLvnOdfinditemLoglist)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_MESSAGE(MSG_LOADED,OnLoad)
	ON_WM_MEASUREITEM()
	ON_WM_MEASUREITEM_REFLECT()
	ON_NOTIFY(HDN_BEGINTRACKA, 0, OnHdnBegintrack)
	ON_NOTIFY(HDN_BEGINTRACKW, 0, OnHdnBegintrack)
	ON_NOTIFY(HDN_ITEMCHANGINGA, 0, OnHdnItemchanging)
	ON_NOTIFY(HDN_ITEMCHANGINGW, 0, OnHdnItemchanging)
	ON_NOTIFY(HDN_ENDTRACK, 0, OnColumnResized)
	ON_NOTIFY(HDN_ENDDRAG, 0, OnColumnMoved)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, &OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, &OnToolTipText)
END_MESSAGE_MAP()

void CGitLogListBase::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	//if (m_nRowHeight>0)
		lpMeasureItemStruct->itemHeight = 50;
}

int CGitLogListBase:: OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	PreSubclassWindow();
	return CHintListCtrl::OnCreate(lpCreateStruct);
}

void CGitLogListBase::PreSubclassWindow()
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);
	// load the icons for the action columns
//	m_Theme.Open(m_hWnd, L"ListView");
	SetWindowTheme(m_hWnd, L"Explorer", nullptr);
	CHintListCtrl::PreSubclassWindow();
}

CString CGitLogListBase::GetRebaseActionName(int action)
{
	if (action & LOGACTIONS_REBASE_EDIT)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_EDIT);
	if (action & LOGACTIONS_REBASE_SQUASH)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_SQUASH);
	if (action & LOGACTIONS_REBASE_PICK)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_PICK);
	if (action & LOGACTIONS_REBASE_SKIP)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_SKIP);

	return MAKEINTRESOURCE(IDS_PATHACTIONS_UNKNOWN);
}

void CGitLogListBase::InsertGitColumn()
{
	CString temp;

	CRegDWORD regFullRowSelect(_T("Software\\TortoiseGit\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	SetExtendedStyle(exStyle);

	// only load properties if we have a repository
	if (GitAdminDir::IsWorkingTreeOrBareRepo(g_Git.m_CurrentDir))
		UpdateProjectProperties();

	static UINT normal[] =
	{
		IDS_LOG_GRAPH,
		IDS_LOG_REBASE,
		IDS_LOG_ID,
		IDS_LOG_HASH,
		IDS_LOG_ACTIONS,
		IDS_LOG_MESSAGE,
		IDS_LOG_AUTHOR,
		IDS_LOG_DATE,
		IDS_LOG_EMAIL,
		IDS_LOG_COMMIT_NAME,
		IDS_LOG_COMMIT_EMAIL,
		IDS_LOG_COMMIT_DATE,
		IDS_LOG_BUGIDS,
		IDS_LOG_SVNREV,
	};

	static int with[] =
	{
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		LOGLIST_MESSAGE_MIN,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
	};
	m_dwDefaultColumns = GIT_LOG_GRAPH|GIT_LOG_ACTIONS|GIT_LOG_MESSAGE|GIT_LOG_AUTHOR|GIT_LOG_DATE;

	DWORD hideColumns = 0;
	if(this->m_IsRebaseReplaceGraph)
	{
		hideColumns |= GIT_LOG_GRAPH;
		m_dwDefaultColumns |= GIT_LOG_REBASE;
	}
	else
		hideColumns |= GIT_LOG_REBASE;

	if(this->m_IsIDReplaceAction)
	{
		hideColumns |= GIT_LOG_ACTIONS;
		m_dwDefaultColumns |= GIT_LOG_ID;
		m_dwDefaultColumns |= GIT_LOG_HASH;
	}
	else
		hideColumns |= GIT_LOG_ID;
	if(this->m_bShowBugtraqColumn)
		m_dwDefaultColumns |= GIT_LOGLIST_BUG;
	else
		hideColumns |= GIT_LOGLIST_BUG;
	if (CTGitPath(g_Git.m_CurrentDir).HasGitSVNDir())
		m_dwDefaultColumns |= GIT_LOGLIST_SVNREV;
	else
		hideColumns |= GIT_LOGLIST_SVNREV;
	SetRedraw(false);

	m_ColumnManager.SetNames(normal, _countof(normal));
	m_ColumnManager.ReadSettings(m_dwDefaultColumns, hideColumns, m_ColumnRegKey+_T("loglist"), _countof(normal), with);

	SetRedraw(true);
}

/**
 * Resizes all columns in a list control to values in registry.
 */
void CGitLogListBase::ResizeAllListCtrlCols()
{
	// column max and min widths to allow
	static const int nMinimumWidth = 10;
	static const int nMaximumWidth = 1000;
	CHeaderCtrl* pHdrCtrl = (CHeaderCtrl*)(GetDlgItem(0));
	if (pHdrCtrl)
	{
		int numcols = pHdrCtrl->GetItemCount();
		for (int col = 0; col < numcols; ++col)
		{
			// get width for this col last time from registry
			CString regentry;
			regentry.Format( _T("Software\\TortoiseGit\\%s\\ColWidth%d"), (LPCTSTR)m_ColumnRegKey, col);
			CRegDWORD regwidth(regentry, 0);
			int cx = regwidth;
			if ( cx == 0 )
			{
				// no saved value, setup sensible defaults
				if (col == this->LOGLIST_MESSAGE)
					cx = LOGLIST_MESSAGE_MIN;
				else
					cx = ICONITEMBORDER+16*4;
			}
			if (cx < nMinimumWidth)
				cx = nMinimumWidth;
			else if (cx > nMaximumWidth)
				cx = nMaximumWidth;

			SetColumnWidth(col, cx);
		}
	}
}

void CGitLogListBase::FillBackGround(HDC hdc, DWORD_PTR Index, CRect &rect)
{
	LVITEM rItem = { 0 };
	rItem.mask  = LVIF_STATE;
	rItem.iItem = (int)Index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(Index);
	HBRUSH brush = nullptr;

	if (!(rItem.state & LVIS_SELECTED))
	{
		int action = pLogEntry->GetRebaseAction();
		if (action & LOGACTIONS_REBASE_SQUASH)
			brush = ::CreateSolidBrush(RGB(156,156,156));
		else if (action & LOGACTIONS_REBASE_EDIT)
			brush = ::CreateSolidBrush(RGB(200,200,128));
	}
	else if (!IsAppThemed())
	{
		if (rItem.state & LVIS_SELECTED)
		{
			if (::GetFocus() == m_hWnd)
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
			else
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
		}
	}
	if (brush)
	{
		::FillRect(hdc, &rect, brush);
		::DeleteObject(brush);
	}
}

void DrawTrackingRoundRect(HDC hdc, CRect rect, HBRUSH brush, COLORREF darkColor)
{
	POINT point = { 4, 4 };
	CRect rt2 = rect;
	rt2.DeflateRect(1, 1);
	rt2.OffsetRect(2, 2);

	HPEN nullPen = ::CreatePen(PS_NULL, 0, 0);
	HPEN oldpen = (HPEN)::SelectObject(hdc, nullPen);
	HBRUSH darkBrush = (HBRUSH)::CreateSolidBrush(darkColor);
	HBRUSH oldbrush = (HBRUSH)::SelectObject(hdc, darkBrush);
	::RoundRect(hdc, rt2.left, rt2.top, rt2.right, rt2.bottom, point.x, point.y);

	::SelectObject(hdc, brush);
	rt2.OffsetRect(-2, -2);
	::RoundRect(hdc, rt2.left, rt2.top, rt2.right, rt2.bottom, point.x, point.y);
	::SelectObject(hdc, oldbrush);
	::SelectObject(hdc, oldpen);
	::DeleteObject(nullPen);
	::DeleteObject(darkBrush);
}

void DrawLightning(HDC hdc, CRect rect, COLORREF color, int bold)
{
	HPEN pen = ::CreatePen(PS_SOLID, bold, color);
	HPEN oldpen = (HPEN)::SelectObject(hdc, pen);
	::MoveToEx(hdc, rect.left + 7, rect.top, nullptr);
	::LineTo(hdc, rect.left + 1, (rect.top + rect.bottom) / 2);
	::LineTo(hdc, rect.left + 6, (rect.top + rect.bottom) / 2);
	::LineTo(hdc, rect.left, rect.bottom);
	::SelectObject(hdc, oldpen);
	::DeleteObject(pen);
}

void DrawUpTriangle(HDC hdc, CRect rect, COLORREF color, int bold)
{
	HPEN pen = ::CreatePen(PS_SOLID, bold, color);
	HPEN oldpen = (HPEN)::SelectObject(hdc, pen);
	::MoveToEx(hdc, (rect.left + rect.right) / 2, rect.top, nullptr);
	::LineTo(hdc, rect.left, rect.bottom);
	::LineTo(hdc, rect.right, rect.bottom);
	::LineTo(hdc, (rect.left + rect.right) / 2, rect.top);
	::SelectObject(hdc, oldpen);
	::DeleteObject(pen);
}

void CGitLogListBase::DrawTagBranchMessage(HDC hdc, CRect &rect, INT_PTR index, std::vector<REFLABEL> &refList)
{
	GitRevLoglist* data = m_arShownList.SafeGetAt(index);
	CRect rt=rect;
	LVITEM rItem = { 0 };
	rItem.mask  = LVIF_STATE;
	rItem.iItem = (int)index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	CDC W_Dc;
	W_Dc.Attach(hdc);

	HTHEME hTheme = nullptr;
	if (IsAppThemed())
		hTheme = OpenThemeData(m_hWnd, L"Explorer::ListView;ListView");

	SIZE oneSpaceSize;
	if (m_bTagsBranchesOnRightSide)
	{
		HFONT oldFont = (HFONT)SelectObject(hdc, (HFONT)GetStockObject(DEFAULT_GUI_FONT));
		GetTextExtentPoint32(hdc, L" ", 1, &oneSpaceSize);
		SelectObject(hdc, oldFont);
		rt.left += oneSpaceSize.cx * 2;
	}
	else
	{
		GetTextExtentPoint32(hdc, L" ", 1, &oneSpaceSize);
		DrawTagBranch(hdc, W_Dc, hTheme, rect, rt, rItem, data, refList);
		rt.left += oneSpaceSize.cx;
	}

	CString msg = MessageDisplayStr(data);
	int action = data->GetRebaseAction();
	bool skip = !!(action & (LOGACTIONS_REBASE_DONE | LOGACTIONS_REBASE_SKIP));
	if (IsAppThemed())
	{
		int txtState = LISS_NORMAL;
		if (rItem.state & LVIS_SELECTED)
			txtState = LISS_SELECTED;

		DTTOPTS opts = { 0 };
		opts.dwSize = sizeof(opts);
		opts.crText = skip ? RGB(128,128,128) : ::GetSysColor(COLOR_WINDOWTEXT);
		opts.dwFlags = DTT_TEXTCOLOR;
		DrawThemeTextEx(hTheme, hdc, LVP_LISTITEM, txtState, msg, -1, DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, &rt, &opts);
	}
	else
	{
		if ((rItem.state & LVIS_SELECTED) && (::GetFocus() == m_hWnd))
		{
			COLORREF clrOld = ::SetTextColor(hdc, skip ? RGB(128,128,128) : ::GetSysColor(COLOR_HIGHLIGHTTEXT));
			::DrawText(hdc,msg, msg.GetLength(), &rt, DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
			::SetTextColor(hdc, clrOld);
		}
		else
		{
			COLORREF clrOld = ::SetTextColor(hdc, skip ? RGB(128,128,128) : ::GetSysColor(COLOR_WINDOWTEXT));
			::DrawText(hdc, msg, msg.GetLength(), &rt, DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
			::SetTextColor(hdc, clrOld);
		}
	}

	if (m_bTagsBranchesOnRightSide)
	{
		SIZE size;
		GetTextExtentPoint32(hdc, msg, msg.GetLength(), &size);

		rt.left += oneSpaceSize.cx + size.cx;

		DrawTagBranch(hdc, W_Dc, hTheme, rect, rt, rItem, data, refList);
	}

	if (hTheme)
		CloseThemeData(hTheme);

	W_Dc.Detach();
}

void CGitLogListBase::DrawTagBranch(HDC hdc, CDC& W_Dc, HTHEME hTheme, CRect& rect, CRect& rt, LVITEM& rItem, GitRevLoglist* data, std::vector<REFLABEL>& refList)
{
	for (unsigned int i = 0; i < refList.size(); ++i)
	{
		CString shortname = !refList[i].simplifiedName.IsEmpty() ? refList[i].simplifiedName : refList[i].name;
		HBRUSH brush = 0;
		COLORREF colRef = refList[i].color;
		bool singleRemote = refList[i].singleRemote;
		bool hasTracking = refList[i].hasTracking;
		bool sameName = refList[i].sameName;
		CGit::REF_TYPE refType = refList[i].refType;

		//When row selected, ajust label color
		if (!IsAppThemed())
		{
			if (rItem.state & LVIS_SELECTED)
				colRef = CColors::MixColors(colRef, ::GetSysColor(COLOR_HIGHLIGHT), 150);
		}

		brush = ::CreateSolidBrush(colRef);

		if (!shortname.IsEmpty() && (rt.left < rect.right))
		{
			SIZE size = { 0 };
			GetTextExtentPoint32(hdc, shortname, shortname.GetLength(), &size);

			rt.SetRect(rt.left, rt.top, rt.left + size.cx, rt.bottom);
			rt.right += 8;

			int textpos = DT_CENTER;

			if (rt.right > rect.right)
			{
				rt.right = rect.right;
				textpos = 0;
			}

			CRect textRect = rt;

			if (singleRemote)
			{
				rt.right += 8;
				textRect.OffsetRect(8, 0);
			}

			if (sameName)
				rt.right += 8;

			if (hasTracking)
				DrawTrackingRoundRect(hdc, rt, brush, m_Colors.Darken(colRef, 100));
			else
			{
				//Fill interior of ref label
				::FillRect(hdc, &rt, brush);
			}

			//Draw edge of label
			CRect rectEdge = rt;

			if (!hasTracking)
			{
				W_Dc.Draw3dRect(rectEdge, m_Colors.Lighten(colRef, 100), m_Colors.Darken(colRef, 100));
				rectEdge.DeflateRect(1, 1);
				W_Dc.Draw3dRect(rectEdge, m_Colors.Lighten(colRef, 50), m_Colors.Darken(colRef, 50));
			}

			if (refType == CGit::REF_TYPE::ANNOTATED_TAG)
			{
				rt.right += 8;
				POINT trianglept[3] = { { rt.right - 8, rt.top }, { rt.right, (rt.top + rt.bottom) / 2 }, { rt.right - 8, rt.bottom } };
				HRGN hrgn = ::CreatePolygonRgn(trianglept, 3, ALTERNATE);
				::FillRgn(hdc, hrgn, brush);
				::DeleteObject(hrgn);
				::MoveToEx(hdc, trianglept[0].x - 1, trianglept[0].y, nullptr);
				HPEN pen;
				HPEN oldpen = (HPEN)SelectObject(hdc, pen = ::CreatePen(PS_SOLID, 2, m_Colors.Lighten(colRef, 50)));
				::LineTo(hdc, trianglept[1].x - 1, trianglept[1].y - 1);
				::DeleteObject(pen);
				SelectObject(hdc, pen = ::CreatePen(PS_SOLID, 2, m_Colors.Darken(colRef, 50)));
				::LineTo(hdc, trianglept[2].x - 1, trianglept[2].y - 1);
				::DeleteObject(pen);
				SelectObject(hdc, pen = ::CreatePen(PS_SOLID, 2, colRef));
				::MoveToEx(hdc, trianglept[0].x - 1, trianglept[2].y - 3, nullptr);
				::LineTo(hdc, trianglept[0].x - 1, trianglept[0].y);
				::DeleteObject(pen);
				SelectObject(hdc, oldpen);
			}

			//Draw text inside label
			bool customColor = (colRef & 0xff) * 30 + ((colRef >> 8) & 0xff) * 59 + ((colRef >> 16) & 0xff) * 11 <= 12800;	// check if dark background
			if (!customColor && IsAppThemed())
			{
				int txtState = LISS_NORMAL;
				if (rItem.state & LVIS_SELECTED)
					txtState = LISS_SELECTED;

				DTTOPTS opts = { 0 };
				opts.dwSize = sizeof(opts);
				opts.crText = ::GetSysColor(COLOR_WINDOWTEXT);
				opts.dwFlags = DTT_TEXTCOLOR;
				DrawThemeTextEx(hTheme, hdc, LVP_LISTITEM, txtState, shortname, -1, textpos | DT_SINGLELINE | DT_VCENTER, &textRect, &opts);
			}
			else
			{
				W_Dc.SetBkMode(TRANSPARENT);
				if (customColor || (rItem.state & LVIS_SELECTED))
				{
					COLORREF clrNew = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
					COLORREF clrOld = ::SetTextColor(hdc,clrNew);
					::DrawText(hdc, shortname, shortname.GetLength(), &textRect, textpos | DT_SINGLELINE | DT_VCENTER);
					::SetTextColor(hdc,clrOld);
				}
				else
				{
					COLORREF clrOld = ::SetTextColor(hdc, ::GetSysColor(COLOR_WINDOWTEXT));
					::DrawText(hdc, shortname, shortname.GetLength(), &textRect, textpos | DT_SINGLELINE | DT_VCENTER);
					::SetTextColor(hdc, clrOld);
				}
			}

			if (singleRemote)
			{
				COLORREF color = ::GetSysColor(customColor ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
				int bold = data->m_CommitHash == m_HeadHash ? 2 : 1;
				CRect newRect;
				newRect.SetRect(rt.left + 4, rt.top + 4, rt.left + 8, rt.bottom - 4);
				DrawLightning(hdc, newRect, color, bold);
			}

			if (sameName)
			{
				COLORREF color = ::GetSysColor(customColor ? COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT);
				int bold = data->m_CommitHash == m_HeadHash ? 2 : 1;
				CRect newRect;
				newRect.SetRect(rt.right - 12, rt.top + 4, rt.right - 4, rt.bottom - 4);
				DrawUpTriangle(hdc, newRect, color, bold);
			}

			m_RefLabelPosMap[refList[i].fullName] = rt;

			rt.left = rt.right + 1;
		}
		if (brush)
			::DeleteObject(brush);
	}
	rt.right = rect.right;
}

static COLORREF blend(const COLORREF& col1, const COLORREF& col2, int amount = 128) {
	// Returns ((256 - amount)*col1 + amount*col2) / 256;
	return RGB(((256 - amount)*GetRValue(col1)   + amount*GetRValue(col2)  ) / 256,
					((256 - amount)*GetGValue(col1) + amount*GetGValue(col2) ) / 256,
					((256 - amount)*GetBValue(col1)  + amount*GetBValue(col2) ) / 256);
}

Gdiplus::Color GetGdiColor(COLORREF col)
{
	return Gdiplus::Color(GetRValue(col),GetGValue(col),GetBValue(col));
}
void CGitLogListBase::paintGraphLane(HDC hdc, int laneHeight,int type, int x1, int x2,
										const COLORREF& col,const COLORREF& activeColor, int top
										)
{
	int h = laneHeight / 2;
	int m = (x1 + x2) / 2;
	int r = (x2 - x1) * m_NodeSize / 30;
	int d =  2 * r;

	#define P_CENTER	m , h+top
	#define P_0			x2, h+top
	#define P_90		m , 0+top-1
	#define P_180		x1, h+top
	#define P_270		m , 2 * h+top +1
	#define R_CENTER	m - r, h - r+top, d, d


	#define DELTA_UR_B 2*(x1 - m), 2*h +top
	#define DELTA_UR_E 0*16, 90*16 +top  // -,

	#define DELTA_DR_B 2*(x1 - m), 2*-h +top
	#define DELTA_DR_E 270*16, 90*16 +top  // -'

	#define DELTA_UL_B 2*(x2 - m), 2*h +top
	#define DELTA_UL_E 90*16, 90*16 +top //  ,-

	#define DELTA_DL_B 2*(x2 - m),2*-h +top
	#define DELTA_DL_E 180*16, 90*16  //  '-

	#define CENTER_UR x1, 2*h, 225
	#define CENTER_DR x1, 0  , 135
	#define CENTER_UL x2, 2*h, 315
	#define CENTER_DL x2, 0  ,  45


	Gdiplus::Graphics graphics( hdc );

	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	// arc
	switch (type) {
	case Lanes::JOIN:
	case Lanes::JOIN_R:
	case Lanes::HEAD:
	case Lanes::HEAD_R:
	{
		Gdiplus::LinearGradientBrush gradient(
								Gdiplus::Point(x1-2, h+top-2),
								Gdiplus::Point(P_270),
								GetGdiColor(activeColor),GetGdiColor(col));


		Gdiplus::Pen mypen(&gradient, (Gdiplus::REAL)m_LineWidth);
		//Gdiplus::Pen mypen(Gdiplus::Color(0,0,0),2);

		//graphics.DrawRectangle(&mypen,x1-(x2-x1)/2,top+h, x2-x1,laneHeight);
		graphics.DrawArc(&mypen,x1-(x2-x1)/2-1,top+h-1, x2-x1,laneHeight,270,90);
		//graphics.DrawLine(&mypen,x1-1,h+top,P_270);

		break;
	}
	case Lanes::JOIN_L:
	{
		Gdiplus::LinearGradientBrush gradient(
								Gdiplus::Point(P_270),
								Gdiplus::Point(x2+1, h+top-1),
								GetGdiColor(col),GetGdiColor(activeColor));


		Gdiplus::Pen mypen(&gradient, (Gdiplus::REAL)m_LineWidth);
		//Gdiplus::Pen mypen(Gdiplus::Color(0,0,0),2);

		//graphics.DrawRectangle(&mypen,x1-(x2-x1)/2,top+h, x2-x1,laneHeight);
		graphics.DrawArc(&mypen,x1+(x2-x1)/2,top+h-1, x2-x1,laneHeight,180,90);
		//graphics.DrawLine(&mypen,x1-1,h+top,P_270);


		break;
	}
	case Lanes::TAIL:
	case Lanes::TAIL_R:
	{
		Gdiplus::LinearGradientBrush gradient(
								Gdiplus::Point(x1-2, h+top-2),
								Gdiplus::Point(P_90),
								GetGdiColor(activeColor),GetGdiColor(col));

		Gdiplus::Pen mypen(&gradient, (Gdiplus::REAL)m_LineWidth);

		graphics.DrawArc(&mypen,x1-(x2-x1)/2-1,top-h-1, x2-x1,laneHeight,0,90);

#if 0
		QConicalGradient gradient(CENTER_DR);
		gradient.setColorAt(0.375, activeCol);
		gradient.setColorAt(0.625, col);
		myPen.setBrush(gradient);
		p->setPen(myPen);
		p->drawArc(P_CENTER, DELTA_DR);
#endif
		break;
	}
	default:
		break;
	}


	//static QPen myPen(Qt::black, 2); // fast path here
	CPen pen;
	pen.CreatePen(PS_SOLID,2,col);
	//myPen.setColor(col);
	HPEN oldpen=(HPEN)::SelectObject(hdc,(HPEN)pen);

	Gdiplus::Pen myPen(GetGdiColor(col), (Gdiplus::REAL)m_LineWidth);

	graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);

	//p->setPen(myPen);

	// vertical line
	switch (type) {
	case Lanes::ACTIVE:
	case Lanes::NOT_ACTIVE:
	case Lanes::MERGE_FORK:
	case Lanes::MERGE_FORK_R:
	case Lanes::MERGE_FORK_L:
	case Lanes::JOIN:
	case Lanes::JOIN_R:
	case Lanes::JOIN_L:
	case Lanes::CROSS:
		//DrawLine(hdc,P_90,P_270);
		graphics.DrawLine(&myPen,P_90,P_270);
		//p->drawLine(P_90, P_270);
		break;
	case Lanes::HEAD_L:
	case Lanes::BRANCH:
		//DrawLine(hdc,P_CENTER,P_270);
		graphics.DrawLine(&myPen,P_CENTER,P_270);
		//p->drawLine(P_CENTER, P_270);
		break;
	case Lanes::TAIL_L:
	case Lanes::INITIAL:
	case Lanes::BOUNDARY:
	case Lanes::BOUNDARY_C:
	case Lanes::BOUNDARY_R:
	case Lanes::BOUNDARY_L:
		//DrawLine(hdc,P_90, P_CENTER);
		graphics.DrawLine(&myPen,P_90,P_CENTER);
		//p->drawLine(P_90, P_CENTER);
		break;
	default:
		break;
	}

	myPen.SetColor(GetGdiColor(activeColor));

	// horizontal line
	switch (type) {
	case Lanes::MERGE_FORK:
	case Lanes::JOIN:
	case Lanes::HEAD:
	case Lanes::TAIL:
	case Lanes::CROSS:
	case Lanes::CROSS_EMPTY:
	case Lanes::BOUNDARY_C:
		//DrawLine(hdc,P_180,P_0);
		graphics.DrawLine(&myPen,P_180,P_0);
		//p->drawLine(P_180, P_0);
		break;
	case Lanes::MERGE_FORK_R:
	case Lanes::BOUNDARY_R:
		//DrawLine(hdc,P_180,P_CENTER);
		graphics.DrawLine(&myPen,P_180,P_CENTER);
		//p->drawLine(P_180, P_CENTER);
		break;
	case Lanes::MERGE_FORK_L:
	case Lanes::HEAD_L:
	case Lanes::TAIL_L:
	case Lanes::BOUNDARY_L:
		//DrawLine(hdc,P_CENTER,P_0);
		graphics.DrawLine(&myPen,P_CENTER,P_0);
		//p->drawLine(P_CENTER, P_0);
		break;
	default:
		break;
	}

	graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	CBrush brush;
	brush.CreateSolidBrush(col);
	HBRUSH oldbrush=(HBRUSH)::SelectObject(hdc,(HBRUSH)brush);

	Gdiplus::SolidBrush myBrush(GetGdiColor(col));
	// center symbol, e.g. rect or ellipse
	switch (type) {
	case Lanes::ACTIVE:
	case Lanes::INITIAL:
	case Lanes::BRANCH:

		//p->setPen(Qt::NoPen);
		//p->setBrush(col);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		graphics.FillEllipse(&myBrush, R_CENTER);
		//p->drawEllipse(R_CENTER);
		break;
	case Lanes::MERGE_FORK:
	case Lanes::MERGE_FORK_R:
	case Lanes::MERGE_FORK_L:
		//p->setPen(Qt::NoPen);
		//p->setBrush(col);
		//p->drawRect(R_CENTER);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
		graphics.FillRectangle(&myBrush, R_CENTER);
		break;
	case Lanes::UNAPPLIED:
		// Red minus sign
		//p->setPen(Qt::NoPen);
		//p->setBrush(Qt::red);
		//p->drawRect(m - r, h - 1, d, 2);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
		graphics.FillRectangle(&myBrush,m-r,h-1,d,2);
		break;
	case Lanes::APPLIED:
		// Green plus sign
		//p->setPen(Qt::NoPen);
		//p->setBrush(DARK_GREEN);
		//p->drawRect(m - r, h - 1, d, 2);
		//p->drawRect(m - 1, h - r, 2, d);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
		graphics.FillRectangle(&myBrush,m-r,h-1,d,2);
		graphics.FillRectangle(&myBrush,m-1,h-r,2,d);
		break;
	case Lanes::BOUNDARY:
		//p->setBrush(back);
		//p->drawEllipse(R_CENTER);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
		graphics.DrawEllipse(&myPen, R_CENTER);
		break;
	case Lanes::BOUNDARY_C:
	case Lanes::BOUNDARY_R:
	case Lanes::BOUNDARY_L:
		//p->setBrush(back);
		//p->drawRect(R_CENTER);
		graphics.SetSmoothingMode(Gdiplus::SmoothingModeNone);
		graphics.FillRectangle(&myBrush,R_CENTER);
		break;
	default:
		break;
	}

	::SelectObject(hdc,oldpen);
	::SelectObject(hdc,oldbrush);
	#undef P_CENTER
	#undef P_0
	#undef P_90
	#undef P_180
	#undef P_270
	#undef R_CENTER
}

void CGitLogListBase::DrawGraph(HDC hdc,CRect &rect,INT_PTR index)
{
	// TODO: unfinished
//	return;
	GitRevLoglist* data = m_arShownList.SafeGetAt(index);
	if(data->m_CommitHash.IsEmpty())
		return;

	CRect rt=rect;
	LVITEM rItem = { 0 };
	rItem.mask  = LVIF_STATE;
	rItem.iItem = (int)index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

//	p->translate(QPoint(opt.rect.left(), opt.rect.top()));

	if (data->m_Lanes.empty())
		m_logEntries.setLane(data->m_CommitHash);

	std::vector<int>& lanes=data->m_Lanes;
	size_t laneNum = lanes.size();
	UINT activeLane = 0;
	for (UINT i = 0; i < laneNum; ++i)
		if (Lanes::isMerge(lanes[i])) {
			activeLane = i;
			break;
		}

	int x2 = 0;
	int maxWidth = rect.Width();
	int lw = 3 * rect.Height() / 4; //laneWidth()

	COLORREF activeColor = m_LineColors[activeLane % Lanes::COLORS_NUM];
	//if (opt.state & QStyle::State_Selected)
	//	activeColor = blend(activeColor, opt.palette.highlightedText().color(), 208);

	for (unsigned int i = 0; i < laneNum && x2 < maxWidth; ++i)
	{
		int x1 = x2;
		x2 += lw;

		int ln = lanes[i];
		if (ln == Lanes::EMPTY)
			continue;

		COLORREF color = i == activeLane ? activeColor : m_LineColors[i % Lanes::COLORS_NUM];
		paintGraphLane(hdc, rect.Height(),ln, x1+rect.left, x2+rect.left, color,activeColor, rect.top);
	}

#if 0
	for (UINT i = 0; i < laneNum && x2 < maxWidth; ++i) {
		int x1 = x2;
		x2 += lw;

		int ln = lanes[i];
		if (ln == Lanes::EMPTY)
			continue;

		UINT col = (  Lanes:: isHead(ln) ||Lanes:: isTail(ln) || Lanes::isJoin(ln)
					|| ln ==Lanes:: CROSS_EMPTY) ? activeLane : i;

		if (ln == Lanes::CROSS)
		{
			paintGraphLane(hdc, rect.Height(),Lanes::NOT_ACTIVE, x1, x2, m_LineColors[col % Lanes::COLORS_NUM],rect.top);
			paintGraphLane(hdc, rect.Height(),Lanes::CROSS, x1, x2, m_LineColors[activeLane % Lanes::COLORS_NUM],rect.top);
		}
		else
			paintGraphLane(hdc, rect.Height(),ln, x1, x2, m_LineColors[col % Lanes::COLORS_NUM],rect.top);
	}
#endif
}

void CGitLogListBase::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	if (m_bNoDispUpdates)
		return;

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color.

			// Tell Windows to send draw notifications for each subitem.
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			COLORREF crText = GetSysColor(COLOR_WINDOWTEXT);

			if (m_arShownList.size() > pLVCD->nmcd.dwItemSpec)
			{
				GitRevLoglist* data = m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
				if (data)
				{
					HGDIOBJ hGdiObj = nullptr;
					int action = data->GetRebaseAction();
					if (action & (LOGACTIONS_REBASE_DONE | LOGACTIONS_REBASE_SKIP))
						crText = RGB(128,128,128);

					if (action & LOGACTIONS_REBASE_SQUASH)
						pLVCD->clrTextBk = RGB(156,156,156);
					else if (action & LOGACTIONS_REBASE_EDIT)
						pLVCD->clrTextBk  = RGB(200,200,128);
					else
						pLVCD->clrTextBk  = ::GetSysColor(COLOR_WINDOW);

					if (action & LOGACTIONS_REBASE_CURRENT)
						hGdiObj = m_boldFont.GetSafeHandle();

					BOOL isHeadHash = data->m_CommitHash == m_HeadHash && m_bNoHightlightHead == FALSE;
					BOOL isHighlight = data->m_CommitHash == m_highlight && !m_highlight.IsEmpty();
					if (isHeadHash && isHighlight)
						hGdiObj = m_boldItalicsFont.GetSafeHandle();
					else if (isHeadHash)
						hGdiObj = m_boldFont.GetSafeHandle();
					else if (isHighlight)
						hGdiObj = m_FontItalics.GetSafeHandle();

					if (hGdiObj)
					{
						SelectObject(pLVCD->nmcd.hdc, hGdiObj);
						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					}

//					if ((data->childStackDepth)||(m_mergedRevs.find(data->Rev) != m_mergedRevs.end()))
//						crText = GetSysColor(COLOR_GRAYTEXT);

					if (data->m_CommitHash.IsEmpty())
					{
						//crText = GetSysColor(RGB(200,200,0));
						//SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						// We changed the font, so we're returning CDRF_NEWFONT. This
						// tells the control to recalculate the extent of the text.
						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					}
				}
			}
			if (m_arShownList.size() == pLVCD->nmcd.dwItemSpec)
			{
				if (m_bStrictStopped)
					crText = GetSysColor(COLOR_GRAYTEXT);
			}
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT|CDDS_ITEM|CDDS_SUBITEM:
		{
			if (m_bStrictStopped && m_arShownList.size() == pLVCD->nmcd.dwItemSpec)
			{
				pLVCD->nmcd.uItemState &= ~(CDIS_SELECTED|CDIS_FOCUS);
			}

			if (pLVCD->iSubItem == LOGLIST_GRAPH && !HasFilterText() && (m_ShowFilter & FILTERSHOW_MERGEPOINTS))
			{
				if (m_arShownList.size() > pLVCD->nmcd.dwItemSpec && !this->m_IsRebaseReplaceGraph)
				{
					CRect rect;
					GetSubItemRect((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_LABEL, rect);

					//TRACE(_T("A Graphic left %d right %d\r\n"),rect.left,rect.right);
					FillBackGround(pLVCD->nmcd.hdc, pLVCD->nmcd.dwItemSpec,rect);

					GitRevLoglist* data = m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
					if( !data ->m_CommitHash.IsEmpty())
						DrawGraph(pLVCD->nmcd.hdc,rect,pLVCD->nmcd.dwItemSpec);

					*pResult = CDRF_SKIPDEFAULT;
					return;
				}
			}

			if (pLVCD->iSubItem == LOGLIST_MESSAGE)
			{
				// If the top index of list is changed, the position map of reference label is outdated.
				if (m_OldTopIndex != GetTopIndex())
				{
					m_OldTopIndex = GetTopIndex();
					m_RefLabelPosMap.clear();
				}

				if (m_arShownList.size() > pLVCD->nmcd.dwItemSpec)
				{
					GitRevLoglist* data = m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);

					if (!m_HashMap[data->m_CommitHash].empty() && !(data->GetRebaseAction() & LOGACTIONS_REBASE_DONE))
					{
						CRect rect;
						GetSubItemRect((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);

						// BEGIN: extended redraw, HACK for issue #1618 and #2014
						// not in FillBackGround method, because this only affected the message subitem
						if (0 != pLVCD->iStateId) // don't know why, but this helps against loosing the focus rect
							return;

						int index = (int)pLVCD->nmcd.dwItemSpec;
						int state = GetItemState(index, LVIS_SELECTED);
						int txtState = LISS_NORMAL;
						if (IsAppThemed() && GetHotItem() == (int)index)
						{
							if (state & LVIS_SELECTED)
								txtState = LISS_HOTSELECTED;
							else
								txtState = LISS_HOT;
						}
						else if (state & LVIS_SELECTED)
						{
							if (::GetFocus() == m_hWnd)	
								txtState = LISS_SELECTED;
							else
								txtState = LISS_SELECTEDNOTFOCUS;
						}

						HTHEME hTheme = nullptr;
						if (IsAppThemed())
							hTheme = OpenThemeData(m_hWnd, L"Explorer::ListView;ListView");

						if (hTheme && IsThemeBackgroundPartiallyTransparent(hTheme, LVP_LISTDETAIL, txtState))
							DrawThemeParentBackground(m_hWnd, pLVCD->nmcd.hdc, &rect);
						else
						{
							HBRUSH brush = ::CreateSolidBrush(pLVCD->clrTextBk);
							::FillRect(pLVCD->nmcd.hdc, rect, brush);
							::DeleteObject(brush);
						}
						if (hTheme)
						{
							CRect rt;
							// get rect of whole line
							GetItemRect(index, rt, LVIR_BOUNDS);
							CRect rect2 = rect;
							if (txtState == LISS_NORMAL) // avoid drawing of grey borders
								rect2.DeflateRect(1, 1, 1, 1);

							// calculate background for rect of whole line, but limit redrawing to SubItem rect
							DrawThemeBackground(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, rt, rect2);

							CloseThemeData(hTheme);
						}
						// END: extended redraw

						FillBackGround(pLVCD->nmcd.hdc, pLVCD->nmcd.dwItemSpec,rect);

						std::vector<REFLABEL> refsToShow;
						STRING_VECTOR remoteTrackingList;
						STRING_VECTOR refList = m_HashMap[data->m_CommitHash];
						for (unsigned int i = 0; i < refList.size(); ++i)
						{
							CString str = refList[i];

							REFLABEL refLabel;
							refLabel.color = RGB(255, 255, 255);
							refLabel.singleRemote = false;
							refLabel.hasTracking = false;
							refLabel.sameName = false;
							refLabel.name = CGit::GetShortName(str, &refLabel.refType);
							refLabel.fullName = str;

							switch (refLabel.refType)
							{
							case CGit::REF_TYPE::LOCAL_BRANCH:
							{
								if (!(m_ShowRefMask & LOGLIST_SHOWLOCALBRANCHES))
									continue;
								if (refLabel.name == m_CurrentBranch)
									refLabel.color = m_Colors.GetColor(CColors::CurrentBranch);
								else
									refLabel.color = m_Colors.GetColor(CColors::LocalBranch);

								std::pair<CString, CString> trackingEntry = m_TrackingMap[refLabel.name];
								CString pullRemote = trackingEntry.first;
								CString pullBranch = trackingEntry.second;
								if (!pullRemote.IsEmpty() && !pullBranch.IsEmpty())
								{
									CString defaultUpstream;
									defaultUpstream.Format(_T("refs/remotes/%s/%s"), (LPCTSTR)pullRemote, (LPCTSTR)pullBranch);
									refLabel.hasTracking = true;
									if (m_ShowRefMask & LOGLIST_SHOWREMOTEBRANCHES)
									{
										bool found = false;
										for (size_t j = i + 1; j < refList.size(); ++j)
										{
											if (refList[j] == defaultUpstream)
											{
												found = true;
												break;
											}
										}

										if (found)
										{
											bool sameName = pullBranch == refLabel.name;
											refsToShow.push_back(refLabel);
											CGit::GetShortName(defaultUpstream, refLabel.name, _T("refs/remotes/"));
											refLabel.color = m_Colors.GetColor(CColors::RemoteBranch);
											if (m_bSymbolizeRefNames)
											{
												if (!m_SingleRemote.IsEmpty() && m_SingleRemote == pullRemote)
												{
													refLabel.simplifiedName = _T("/") + (sameName ? CString() : pullBranch);
													refLabel.singleRemote = true;
												}
												else if (sameName)
													refLabel.simplifiedName = pullRemote + _T("/");
												refLabel.sameName = sameName;
											}
											refLabel.fullName = defaultUpstream;
											refsToShow.push_back(refLabel);
											remoteTrackingList.push_back(defaultUpstream);
											continue;
										}
									}
								}
								break;
							}
							case CGit::REF_TYPE::REMOTE_BRANCH:
							{
								if (!(m_ShowRefMask & LOGLIST_SHOWREMOTEBRANCHES))
									continue;
								bool found = false;
								for (size_t j = 0; j < remoteTrackingList.size(); ++j)
								{
									if (remoteTrackingList[j] == str)
									{
										found = true;
										break;
									}
								}
								if (found)
									continue;

								refLabel.color = m_Colors.GetColor(CColors::RemoteBranch);
								if (m_bSymbolizeRefNames)
								{
									if (!m_SingleRemote.IsEmpty() && refLabel.name.Left(m_SingleRemote.GetLength() + 1) == m_SingleRemote + _T("/"))
									{
										refLabel.simplifiedName = _T("/") + refLabel.name.Mid(m_SingleRemote.GetLength() + 1);
										refLabel.singleRemote = true;
									}
								}
								break;
							}
							case CGit::REF_TYPE::ANNOTATED_TAG:
							case CGit::REF_TYPE::TAG:
								if (!(m_ShowRefMask & LOGLIST_SHOWTAGS))
									continue;
								refLabel.color = m_Colors.GetColor(CColors::Tag);
								break;
							case CGit::REF_TYPE::STASH:
								if (!(m_ShowRefMask & LOGLIST_SHOWSTASH))
									continue;
								refLabel.color = m_Colors.GetColor(CColors::Stash);
								break;
							case CGit::REF_TYPE::BISECT_GOOD:
							case CGit::REF_TYPE::BISECT_BAD:
								if (!(m_ShowRefMask & LOGLIST_SHOWBISECT))
									continue;
								refLabel.color = (refLabel.refType == CGit::REF_TYPE::BISECT_GOOD) ? m_Colors.GetColor(CColors::BisectGood): m_Colors.GetColor(CColors::BisectBad);
								break;
							default:
								continue;
							}
							refsToShow.push_back(refLabel);
						}

						if (refsToShow.empty())
						{
							*pResult = CDRF_DODEFAULT;
							return;
						}

						DrawTagBranchMessage(pLVCD->nmcd.hdc, rect, pLVCD->nmcd.dwItemSpec, refsToShow);

						*pResult = CDRF_SKIPDEFAULT;
						return;

					}

				}
			}

			if (pLVCD->iSubItem == LOGLIST_ACTION)
			{
				if(this->m_IsIDReplaceAction)
				{
					*pResult = CDRF_DODEFAULT;
					return;
				}
				*pResult = CDRF_DODEFAULT;

				if (m_arShownList.size() <= pLVCD->nmcd.dwItemSpec)
					return;

				int		nIcons = 0;
				int		iconwidth = ::GetSystemMetrics(SM_CXSMICON);
				int		iconheight = ::GetSystemMetrics(SM_CYSMICON);

				GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
				CRect rect;
				GetSubItemRect((int)pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
				//TRACE(_T("Action left %d right %d\r\n"),rect.left,rect.right);
				// Get the selected state of the
				// item being drawn.

				// Fill the background if necessary
				FillBackGround(pLVCD->nmcd.hdc, pLVCD->nmcd.dwItemSpec, rect);

				// Draw the icon(s) into the compatible DC
				int action = pLogEntry->GetAction(this);
				if (!pLogEntry->m_IsDiffFiles)
				{
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hFetchIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
					*pResult = CDRF_SKIPDEFAULT;
					return;
				}

				if (action & CTGitPath::LOGACTIONS_MODIFIED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
				++nIcons;

				if (action & (CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_COPY))
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + nIcons * iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
				++nIcons;

				if (action & CTGitPath::LOGACTIONS_DELETED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + nIcons * iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
				++nIcons;

				if (action & CTGitPath::LOGACTIONS_REPLACED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + nIcons * iconwidth + ICONITEMBORDER, rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
				++nIcons;

				if (action & CTGitPath::LOGACTIONS_UNMERGED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + nIcons * iconwidth + ICONITEMBORDER, rect.top, m_hConflictedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
				++nIcons;

				*pResult = CDRF_SKIPDEFAULT;
				return;
			}
		}
		break;
	}
	*pResult = CDRF_DODEFAULT;
}

CString FindSVNRev(const CString& msg)
{
	try
	{
		const std::tr1::wsregex_iterator end;
		std::wstring s = msg;
		std::tr1::wregex regex1(_T("^\\s*git-svn-id:\\s+(.*)\\@(\\d+)\\s([a-f\\d\\-]+)$"));
		for (std::tr1::wsregex_iterator it(s.cbegin(), s.cend(), regex1); it != end; ++it)
		{
			const std::tr1::wsmatch match = *it;
			if (match.size() == 4)
			{
				ATLTRACE(_T("matched rev: %s\n"), std::wstring(match[2]).c_str());
				return std::wstring(match[2]).c_str();
			}
		}
		std::tr1::wregex regex2(_T("^\\s*git-svn-id:\\s(\\d+)\\@([a-f\\d\\-]+)$"));
		for (std::tr1::wsregex_iterator it(s.cbegin(), s.cend(), regex2); it != end; ++it)
		{
			const std::tr1::wsmatch match = *it;
			if (match.size() == 3)
			{
				ATLTRACE(_T("matched rev: %s\n"), std::wstring(match[1]).c_str());
				return std::wstring(match[1]).c_str();
			}
		}
	}
	catch (std::exception) {}

	return _T("");
}

CString CGitLogListBase::MessageDisplayStr(GitRev* pLogEntry)
{
	if (!m_bFullCommitMessageOnLogLine || pLogEntry->GetBody().IsEmpty())
		return pLogEntry->GetSubject();

	CString txt(pLogEntry->GetSubject());
	txt += _T(' ');
	txt += pLogEntry->GetBody();

	// Deal with CRLF
	txt.Replace(_T('\n'), _T(' '));
	txt.Replace(_T('\r'), _T(' '));

	return txt;
}

// CGitLogListBase message handlers

void CGitLogListBase::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	// Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	// Do the list need text information?
	if (!(pItem->mask & LVIF_TEXT))
		return;

	// By default, clear text buffer.
	lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax - 1);

	bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();

	*pResult = 0;
	if (m_bNoDispUpdates || bOutOfRange)
		return;

	// Which item number?
	GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(pItem->iItem);

	CString temp;
	if(m_IsOldFirst)
		temp.Format(_T("%d"),pItem->iItem+1);
	else
		temp.Format(_T("%d"), m_arShownList.size() - pItem->iItem);

	// Which column?
	switch (pItem->iSubItem)
	{
	case LOGLIST_GRAPH:	//Graphic
		break;
	case LOGLIST_REBASE:
		{
			if (this->m_IsRebaseReplaceGraph && pLogEntry)
				lstrcpyn(pItem->pszText, GetRebaseActionName(pLogEntry->GetRebaseAction() & LOGACTIONS_REBASE_MODE_MASK), pItem->cchTextMax - 1);
		}
		break;
	case LOGLIST_ACTION: //action -- no text in the column
		break;
	case LOGLIST_HASH:
		if(pLogEntry)
			lstrcpyn(pItem->pszText, pLogEntry->m_CommitHash.ToString(), pItem->cchTextMax - 1);
		break;
	case LOGLIST_ID:
		if(this->m_IsIDReplaceAction)
			lstrcpyn(pItem->pszText, temp, pItem->cchTextMax - 1);
		break;
	case LOGLIST_MESSAGE: //Message
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)MessageDisplayStr(pLogEntry), pItem->cchTextMax - 1);
		break;
	case LOGLIST_AUTHOR: //Author
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetAuthorName(), pItem->cchTextMax - 1);
		break;
	case LOGLIST_DATE: //Date
		if ( pLogEntry && (!pLogEntry->m_CommitHash.IsEmpty()) )
			lstrcpyn(pItem->pszText,
				CLoglistUtils::FormatDateAndTime(pLogEntry->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes),
				pItem->cchTextMax - 1);
		break;

	case LOGLIST_EMAIL:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetAuthorEmail(), pItem->cchTextMax - 1);
		break;

	case LOGLIST_COMMIT_NAME: //Commit
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetCommitterName(), pItem->cchTextMax - 1);
		break;

	case LOGLIST_COMMIT_EMAIL: //Commit Email
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetCommitterEmail(), pItem->cchTextMax - 1);
		break;

	case LOGLIST_COMMIT_DATE: //Commit Date
		if (pLogEntry && (!pLogEntry->m_CommitHash.IsEmpty()))
			lstrcpyn(pItem->pszText,
				CLoglistUtils::FormatDateAndTime(pLogEntry->GetCommitterDate(), m_DateFormat, true, m_bRelativeTimes),
				pItem->cchTextMax - 1);
		break;
	case LOGLIST_BUG: //Bug ID
		if(pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)this->m_ProjectProperties.FindBugID(pLogEntry->GetSubjectBody()), pItem->cchTextMax - 1);
		break;
	case LOGLIST_SVNREV: //SVN revision
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)FindSVNRev(pLogEntry->GetSubjectBody()), pItem->cchTextMax - 1);
		break;

	default:
		ASSERT(false);
	}
}

bool CGitLogListBase::IsOnStash(int index)
{
	GitRevLoglist* rev = m_arShownList.SafeGetAt(index);
	if (IsStash(rev))
		return true;
	if (index > 0)
	{
		GitRevLoglist* preRev = m_arShownList.SafeGetAt(index - 1);
		if (IsStash(preRev))
			return preRev->m_ParentHash.size() == 2 && preRev->m_ParentHash[1] == rev->m_CommitHash;
	}
	return false;
}

bool CGitLogListBase::IsStash(const GitRev * pSelLogEntry)
{
	for (size_t i = 0; i < m_HashMap[pSelLogEntry->m_CommitHash].size(); ++i)
	{
		if (m_HashMap[pSelLogEntry->m_CommitHash][i] == _T("refs/stash"))
			return true;
	}
	return false;
}

bool CGitLogListBase::IsBisect(const GitRev * pSelLogEntry)
{
	for (size_t i = 0; i < m_HashMap[pSelLogEntry->m_CommitHash].size(); ++i)
	{
		if (m_HashMap[pSelLogEntry->m_CommitHash][i].Left(12) == _T("refs/bisect/"))
			return true;
	}
	return false;
}

void CGitLogListBase::GetParentHashes(GitRev *pRev, GIT_REV_LIST &parentHash)
{
	if (pRev->m_ParentHash.empty())
	{
		if (pRev->GetParentFromHash(pRev->m_CommitHash))
			MessageBox(pRev->GetLastErr(), _T("TortoiseGit"), MB_ICONERROR);
	}
	parentHash = pRev->m_ParentHash;
}

void CGitLogListBase::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (pWnd == GetHeaderCtrl())
		return m_ColumnManager.OnContextMenuHeader(pWnd,point,!!IsGroupViewEnabled());

	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return; // nothing selected, nothing to do with a context menu

	// if the user selected the info text telling about not all revisions shown due to
	// the "stop on copy/rename" option, we also don't show the context menu
	if (m_bStrictStopped && selIndex == (int)m_arShownList.size())
		return;

	// if the context menu is invoked through the keyboard, we have to use
	// a calculated position on where to anchor the menu on
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		GetItemRect(selIndex, &rect, LVIR_LABEL);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	m_nSearchIndex = selIndex;
	m_bCancelled = FALSE;

	// calculate some information the context menu commands can use
//	CString pathURL = GetURLFromPath(m_path);

	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	GitRevLoglist* pSelLogEntry = m_arShownList.SafeGetAt(indexNext);
	if (pSelLogEntry == nullptr)
		return;
#if 0
	GitRev revSelected = pSelLogEntry->Rev;
	GitRev revPrevious = git_revnum_t(revSelected)-1;
	if ((pSelLogEntry->pArChangedPaths)&&(pSelLogEntry->pArChangedPaths->GetCount() <= 2))
	{
		for (int i=0; i<pSelLogEntry->pArChangedPaths->GetCount(); ++i)
		{
			LogChangedPath * changedpath = (LogChangedPath *)pSelLogEntry->pArChangedPaths->SafeGetAt(i);
			if (changedpath->lCopyFromRev)
				revPrevious = changedpath->lCopyFromRev;
		}
	}
	GitRev revSelected2;
	if (pos)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(GetNextSelectedItem(pos)));
		revSelected2 = pLogEntry->Rev;
	}
	bool bAllFromTheSameAuthor = true;
	CString firstAuthor;
	CLogDataVector selEntries;
	GitRev revLowest, revHighest;
	GitRevRangeArray revisionRanges;
	{
		POSITION pos = GetFirstSelectedItemPosition();
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(GetNextSelectedItem(pos)));
		revisionRanges.AddRevision(pLogEntry->Rev);
		selEntries.push_back(pLogEntry);
		firstAuthor = pLogEntry->sAuthor;
		revLowest = pLogEntry->Rev;
		revHighest = pLogEntry->Rev;
		while (pos)
		{
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(GetNextSelectedItem(pos)));
			revisionRanges.AddRevision(pLogEntry->Rev);
			selEntries.push_back(pLogEntry);
			if (firstAuthor.Compare(pLogEntry->sAuthor))
				bAllFromTheSameAuthor = false;
			revLowest = (git_revnum_t(pLogEntry->Rev) > git_revnum_t(revLowest) ? revLowest : pLogEntry->Rev);
			revHighest = (git_revnum_t(pLogEntry->Rev) < git_revnum_t(revHighest) ? revHighest : pLogEntry->Rev);
		}
	}

#endif

	bool showExtendedMenu = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

	int FirstSelect=-1, LastSelect=-1;
	pos = GetFirstSelectedItemPosition();
	FirstSelect = GetNextSelectedItem(pos);
	while(pos)
	{
		LastSelect = GetNextSelectedItem(pos);
	}
	//entry is selected, now show the popup menu
	CIconMenu popup;
	CIconMenu subbranchmenu, submenu, gnudiffmenu, diffmenu, blamemenu, revertmenu;

	if (popup.CreatePopupMenu())
	{
		bool isHeadCommit = (pSelLogEntry->m_CommitHash == m_HeadHash);
		CString currentBranch = _T("refs/heads/") + g_Git.GetCurrentBranch();
		bool isMergeActive = CTGitPath(g_Git.m_CurrentDir).IsMergeActive();
		bool isStash = IsOnStash(indexNext);
		GIT_REV_LIST parentHash;
		GetParentHashes(pSelLogEntry, parentHash);

		if (m_ContextMenuMask & GetContextMenuBit(ID_REBASE_PICK) && !(pSelLogEntry->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE)))
			popup.AppendMenuIcon(ID_REBASE_PICK, IDS_REBASE_PICK, IDI_PICK);

		if (m_ContextMenuMask & GetContextMenuBit(ID_REBASE_SQUASH) && !(pSelLogEntry->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE)) && FirstSelect != GetItemCount() - 1 && LastSelect != GetItemCount() - 1 && (m_bIsCherryPick || pSelLogEntry->ParentsCount() == 1))
			popup.AppendMenuIcon(ID_REBASE_SQUASH, IDS_REBASE_SQUASH, IDI_SQUASH);

		if (m_ContextMenuMask & GetContextMenuBit(ID_REBASE_EDIT) && !(pSelLogEntry->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE)))
			popup.AppendMenuIcon(ID_REBASE_EDIT, IDS_REBASE_EDIT, IDI_EDIT);

		if (m_ContextMenuMask & GetContextMenuBit(ID_REBASE_SKIP) && !(pSelLogEntry->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE)))
			popup.AppendMenuIcon(ID_REBASE_SKIP, IDS_REBASE_SKIP, IDI_SKIP);

		if (m_ContextMenuMask & (GetContextMenuBit(ID_REBASE_SKIP) | GetContextMenuBit(ID_REBASE_EDIT) | GetContextMenuBit(ID_REBASE_SQUASH) | GetContextMenuBit(ID_REBASE_PICK)) && !(pSelLogEntry->GetRebaseAction() & (LOGACTIONS_REBASE_CURRENT | LOGACTIONS_REBASE_DONE)))
			popup.AppendMenu(MF_SEPARATOR, NULL);

		if (GetSelectedCount() == 1)
		{
			{
				bool requiresSeparator = false;
				if( !pSelLogEntry->m_CommitHash.IsEmpty())
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_COMPARE) && m_hasWC) // compare revision with WC
					{
						popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
						requiresSeparator = true;
					}
				}
				else
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_COMMIT))
					{
						popup.AppendMenuIcon(ID_COMMIT, IDS_LOG_POPUP_COMMIT, IDI_COMMIT);
						requiresSeparator = true;
					}
					if (isMergeActive && (m_ContextMenuMask & GetContextMenuBit(ID_MERGE_ABORT)))
					{
						popup.AppendMenuIcon(ID_MERGE_ABORT, IDS_MENUMERGEABORT, IDI_MERGEABORT);
						requiresSeparator = true;
					}
				}

				if (m_ContextMenuMask & GetContextMenuBit(ID_BLAMEPREVIOUS))
				{
					if (parentHash.size() == 1)
					{
						popup.AppendMenuIcon(ID_BLAMEPREVIOUS, IDS_LOG_POPUP_BLAMEPREVIOUS, IDI_BLAME);
						requiresSeparator = true;
					}
					else if (parentHash.size() > 1)
					{
						blamemenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_BLAMEPREVIOUS, IDS_LOG_POPUP_BLAMEPREVIOUS, IDI_BLAME, blamemenu.m_hMenu);
						for (size_t i = 0; i < parentHash.size(); ++i)
						{
							CString str;
							str.Format(IDS_PARENT, i + 1);
							blamemenu.AppendMenuIcon(ID_BLAMEPREVIOUS +((i + 1) << 16), str);
						}
						requiresSeparator = true;
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF1) && m_hasWC) // compare with WC, unified
				{
					if (parentHash.size() == 1)
					{
						popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
						requiresSeparator = true;
					}
					else if (parentHash.size() > 1)
					{
						gnudiffmenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_GNUDIFF1,IDS_LOG_POPUP_GNUDIFF_PARENT, IDI_DIFF, gnudiffmenu.m_hMenu);

						gnudiffmenu.AppendMenuIcon((UINT_PTR)(ID_GNUDIFF1 + (0xFFFF << 16)), CString(MAKEINTRESOURCE(IDS_ALLPARENTS)));
						gnudiffmenu.AppendMenuIcon((UINT_PTR)(ID_GNUDIFF1 + (0xFFFE << 16)), CString(MAKEINTRESOURCE(IDS_ONLYMERGEDFILES)));
						gnudiffmenu.AppendMenuIcon((UINT_PTR)(ID_GNUDIFF1 + (0xFFFD << 16)), CString(MAKEINTRESOURCE(IDS_DIFFWITHMERGE)));

						for (size_t i = 0; i < parentHash.size(); ++i)
						{
							CString str;
							str.Format(IDS_PARENT, i + 1);
							gnudiffmenu.AppendMenuIcon(ID_GNUDIFF1+((i+1)<<16),str);
						}
						requiresSeparator = true;
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_COMPAREWITHPREVIOUS))
				{
					if (parentHash.size() == 1)
					{
						popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
						if (CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
							popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
						requiresSeparator = true;
					}
					else if (parentHash.size() > 1)
					{
						diffmenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF, diffmenu.m_hMenu);
						for (size_t i = 0; i < parentHash.size(); ++i)
						{
							CString str;
							str.Format(IDS_PARENT, i + 1);
							diffmenu.AppendMenuIcon(ID_COMPAREWITHPREVIOUS +((i+1)<<16),str);
							if (i == 0 && CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
							{
								popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
								diffmenu.SetDefaultItem((UINT)(ID_COMPAREWITHPREVIOUS + ((i + 1) << 16)), FALSE);
							}
						}
						requiresSeparator = true;
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_BLAME))
				{
					popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
					requiresSeparator = true;
				}

				if (requiresSeparator)
				{
					popup.AppendMenu(MF_SEPARATOR, NULL);
					requiresSeparator = false;
				}

				if (pSelLogEntry->m_CommitHash.IsEmpty() && !isMergeActive)
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_STASH_SAVE))
					{
						popup.AppendMenuIcon(ID_STASH_SAVE, IDS_MENUSTASHSAVE, IDI_COMMIT);
						requiresSeparator = true;
					}
				}

				if ((pSelLogEntry->m_CommitHash.IsEmpty() || isStash) && CTGitPath(g_Git.m_CurrentDir).HasStashDir())
				{
					if (m_ContextMenuMask&GetContextMenuBit(ID_STASH_POP))
					{
						popup.AppendMenuIcon(ID_STASH_POP, IDS_MENUSTASHPOP, IDI_RELOCATE);
						requiresSeparator = true;
					}

					if (m_ContextMenuMask&GetContextMenuBit(ID_STASH_LIST))
					{
						popup.AppendMenuIcon(ID_STASH_LIST, IDS_MENUSTASHLIST, IDI_LOG);
						requiresSeparator = true;
					}
				}

				if (requiresSeparator)
				{
					popup.AppendMenu(MF_SEPARATOR, NULL);
					requiresSeparator = false;
				}

				if (CTGitPath(g_Git.m_CurrentDir).IsBisectActive())
				{
					GitRevLoglist* pFirstEntry = m_arShownList.SafeGetAt(FirstSelect);
					if (m_ContextMenuMask&GetContextMenuBit(ID_BISECTGOOD) && !IsBisect(pFirstEntry))
					{
						popup.AppendMenuIcon(ID_BISECTGOOD, IDS_MENUBISECTGOOD, IDI_THUMB_UP);
						requiresSeparator = true;
					}

					if (m_ContextMenuMask&GetContextMenuBit(ID_BISECTBAD) && !IsBisect(pFirstEntry))
					{
						popup.AppendMenuIcon(ID_BISECTBAD, IDS_MENUBISECTBAD, IDI_THUMB_DOWN);
						requiresSeparator = true;
					}
				}

				if (pSelLogEntry->m_CommitHash.IsEmpty() && CTGitPath(g_Git.m_CurrentDir).IsBisectActive())
				{
					if (m_ContextMenuMask&GetContextMenuBit(ID_BISECTRESET))
					{
						popup.AppendMenuIcon(ID_BISECTRESET, IDS_MENUBISECTRESET, IDI_BISECT_RESET);
						requiresSeparator = true;
					}
				}

				if (requiresSeparator)
				{
					popup.AppendMenu(MF_SEPARATOR, NULL);
					requiresSeparator = false;
				}

				if (pSelLogEntry->m_CommitHash.IsEmpty())
				{
					if (m_ContextMenuMask & GetContextMenuBit(ID_PULL) && !isMergeActive)
						popup.AppendMenuIcon(ID_PULL, IDS_MENUPULL, IDI_PULL);

					if(m_ContextMenuMask&GetContextMenuBit(ID_FETCH))
						popup.AppendMenuIcon(ID_FETCH, IDS_MENUFETCH, IDI_PULL);

					if (CTGitPath(g_Git.m_CurrentDir).HasSubmodules() && m_ContextMenuMask & GetContextMenuBit(ID_SUBMODULE_UPDATE))
						popup.AppendMenuIcon(ID_SUBMODULE_UPDATE, IDS_PROC_SYNC_SUBKODULEUPDATE, IDI_UPDATE);

					popup.AppendMenu(MF_SEPARATOR, NULL);

					if (m_ContextMenuMask & GetContextMenuBit(ID_CLEANUP))
						popup.AppendMenuIcon(ID_CLEANUP, IDS_MENUCLEANUP, IDI_CLEANUP);

					popup.AppendMenu(MF_SEPARATOR, NULL);
				}
			}

//			if (!m_ProjectProperties.sWebViewerRev.IsEmpty())
//			{
//				popup.AppendMenuIcon(ID_VIEWREV, IDS_LOG_POPUP_VIEWREV);
//			}
//			if (!m_ProjectProperties.sWebViewerPathRev.IsEmpty())
//			{
//				popup.AppendMenuIcon(ID_VIEWPATHREV, IDS_LOG_POPUP_VIEWPATHREV);
//			}
//			if ((!m_ProjectProperties.sWebViewerPathRev.IsEmpty())||
//				(!m_ProjectProperties.sWebViewerRev.IsEmpty()))
//			{
//				popup.AppendMenu(MF_SEPARATOR, NULL);
//			}

			CString str;
			//if (m_hasWC)
			//	popup.AppendMenuIcon(ID_REVERTTOREV, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);

			if(!pSelLogEntry->m_CommitHash.IsEmpty())
			{
				if((m_ContextMenuMask&GetContextMenuBit(ID_LOG)) &&
					GetSelectedCount() == 1)
						popup.AppendMenuIcon(ID_LOG, IDS_LOG_POPUP_LOG, IDI_LOG);

				if (m_ContextMenuMask&GetContextMenuBit(ID_REPOBROWSE))
					popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);

				str.Format(IDS_LOG_POPUP_MERGEREV, (LPCTSTR)g_Git.GetCurrentBranch());

				if (m_ContextMenuMask&GetContextMenuBit(ID_MERGEREV) && !isHeadCommit && m_hasWC && !isMergeActive && !isStash)
				{
					popup.AppendMenuIcon(ID_MERGEREV, str, IDI_MERGE);

					size_t index = (size_t)-1;
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, nullptr, &index))
						popup.SetMenuItemData(ID_MERGEREV, (ULONG_PTR)&m_HashMap[pSelLogEntry->m_CommitHash][index]);
				}

				str.Format(IDS_RESET_TO_THIS_FORMAT, (LPCTSTR)g_Git.GetCurrentBranch());

				if (m_ContextMenuMask&GetContextMenuBit(ID_RESET) && m_hasWC && !isStash)
					popup.AppendMenuIcon(ID_RESET,str,IDI_REVERT);


				// Add Switch Branch express Menu
				if( this->m_HashMap.find(pSelLogEntry->m_CommitHash) != m_HashMap.end()
					&& (m_ContextMenuMask&GetContextMenuBit(ID_SWITCHBRANCH) && m_hasWC && !isStash)
					)
				{
					std::vector<const CString*> branchs;
					auto addCheck = [&](const CString& ref)
					{
						if (ref.Find(_T("refs/heads/")) != 0 || ref == currentBranch)
							return;
						branchs.push_back(&ref);
					};
					size_t index = (size_t)-1;
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, nullptr, &index))
						addCheck(m_HashMap[pSelLogEntry->m_CommitHash][index]);
					else
						for_each(m_HashMap[pSelLogEntry->m_CommitHash], addCheck);

					CString str2;
					str2.LoadString(IDS_SWITCH_BRANCH);

					if(branchs.size() == 1)
					{
						str2 += _T(' ');
						str2 += _T('"') + branchs[0]->Mid(11) + _T('"');
						popup.AppendMenuIcon(ID_SWITCHBRANCH, str2, IDI_SWITCH);

						popup.SetMenuItemData(ID_SWITCHBRANCH,(ULONG_PTR)branchs[0]);

					}
					else if(branchs.size() > 1)
					{
						subbranchmenu.CreatePopupMenu();
						for (size_t i = 0 ; i < branchs.size(); ++i)
						{
							if (*branchs[i] != currentBranch)
							{
								subbranchmenu.AppendMenuIcon(ID_SWITCHBRANCH+(i<<16), branchs[i]->Mid(11));
								subbranchmenu.SetMenuItemData(ID_SWITCHBRANCH+(i<<16), (ULONG_PTR) branchs[i]);
							}
						}

						popup.AppendMenuIcon(ID_SWITCHBRANCH, str2, IDI_SWITCH, subbranchmenu.m_hMenu);
					}
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_SWITCHTOREV) && !isHeadCommit && m_hasWC && !isStash)
				{
					popup.AppendMenuIcon(ID_SWITCHTOREV, IDS_SWITCH_TO_THIS, IDI_SWITCH);
					size_t index = (size_t)-1;
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, nullptr, &index))
						popup.SetMenuItemData(ID_SWITCHTOREV, (ULONG_PTR)&m_HashMap[pSelLogEntry->m_CommitHash][index]);
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_CREATE_BRANCH) && !isStash)
				{
					popup.AppendMenuIcon(ID_CREATE_BRANCH, IDS_CREATE_BRANCH_AT_THIS, IDI_COPY);

					size_t index = (size_t)-1;
					CGit::REF_TYPE type = CGit::REF_TYPE::REMOTE_BRANCH;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, nullptr, &index))
						popup.SetMenuItemData(ID_CREATE_BRANCH, (ULONG_PTR)&m_HashMap[pSelLogEntry->m_CommitHash][index]);
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_CREATE_TAG) && !isStash)
					popup.AppendMenuIcon(ID_CREATE_TAG,IDS_CREATE_TAG_AT_THIS , IDI_TAG);

				str.Format(IDS_REBASE_THIS_FORMAT, (LPCTSTR)g_Git.GetCurrentBranch());

				if (pSelLogEntry->m_CommitHash != m_HeadHash && m_hasWC && !isMergeActive && !isStash)
					if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_TO_VERSION))
						popup.AppendMenuIcon(ID_REBASE_TO_VERSION, str , IDI_REBASE);

				if(m_ContextMenuMask&GetContextMenuBit(ID_EXPORT))
					popup.AppendMenuIcon(ID_EXPORT,IDS_EXPORT_TO_THIS, IDI_EXPORT);

				if (m_ContextMenuMask&GetContextMenuBit(ID_REVERTREV) && m_hasWC && !isMergeActive && !isStash)
				{
					if (parentHash.size() == 1)
						popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
					else if (parentHash.size() > 1)
					{
						revertmenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT, revertmenu.m_hMenu);

						for (size_t i = 0; i < parentHash.size(); ++i)
						{
							CString str2;
							str2.Format(IDS_PARENT, i + 1);
							revertmenu.AppendMenuIcon(ID_REVERTREV + ((i + 1) << 16), str2);
						}
					}
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_EDITNOTE) && !isStash)
					popup.AppendMenuIcon(ID_EDITNOTE, IDS_EDIT_NOTES, IDI_EDIT);

				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
		}

		if(!pSelLogEntry->m_Ref.IsEmpty())
		{
			popup.AppendMenuIcon(ID_REFLOG_DEL, IDS_REFLOG_DEL, IDI_DELETE);
			if (GetSelectedCount() == 1 && pSelLogEntry->m_Ref.Find(_T("refs/stash")) == 0)
				popup.AppendMenuIcon(ID_REFLOG_STASH_APPLY, IDS_MENUSTASHAPPLY, IDI_RELOCATE);
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (GetSelectedCount() >= 2)
		{
			bool bAddSeparator = false;
			if (IsSelectionContinuous() || (GetSelectedCount() == 2))
			{
				if(m_ContextMenuMask&GetContextMenuBit(ID_COMPARETWO)) // compare two revisions
					popup.AppendMenuIcon(ID_COMPARETWO, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
			}

			if (GetSelectedCount() == 2)
			{
				if(m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF2) && m_hasWC) // compare two revisions, unified
					popup.AppendMenuIcon(ID_GNUDIFF2, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);

				if (!pSelLogEntry->m_CommitHash.IsEmpty())
				{
					CString firstSelHash = pSelLogEntry->m_CommitHash.ToString().Left(g_Git.GetShortHASHLength());
					GitRevLoglist* pLastEntry = m_arShownList.SafeGetAt(LastSelect);
					CString lastSelHash = pLastEntry->m_CommitHash.ToString().Left(g_Git.GetShortHASHLength());
					CString menu;
					menu.Format(IDS_SHOWLOG_OF, (LPCTSTR)(lastSelHash + _T("..") + firstSelHash));
					popup.AppendMenuIcon(ID_LOG_VIEWRANGE, menu, IDI_LOG);
					menu.Format(IDS_SHOWLOG_OF, (LPCTSTR)(lastSelHash + _T("...") + firstSelHash));
					popup.AppendMenuIcon(ID_LOG_VIEWRANGE_REACHABLEFROMONLYONE, menu, IDI_LOG);
				}

				bAddSeparator = true;
			}

			if (m_hasWC)
				bAddSeparator = true;

			if (m_ContextMenuMask&GetContextMenuBit(ID_REVERTREV) && m_hasWC && !isMergeActive)
					popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREVS, IDI_REVERT);

			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if ( GetSelectedCount() >0 && (!pSelLogEntry->m_CommitHash.IsEmpty()))
		{
			bool bAddSeparator = false;
			if ( IsSelectionContinuous() && GetSelectedCount() >= 2 )
			{
				if (m_ContextMenuMask&GetContextMenuBit(ID_COMBINE_COMMIT) && m_hasWC && !isMergeActive)
				{
					CString head;
					int headindex;
					headindex = this->GetHeadIndex();
					if(headindex>=0 && LastSelect >= headindex && FirstSelect >= headindex)
					{
						head.Format(_T("HEAD~%d"), FirstSelect - headindex);
						CGitHash hashFirst;
						if (g_Git.GetHash(hashFirst, head))
							MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of ") + head + _T(".")), _T("TortoiseGit"), MB_ICONERROR);
						head.Format(_T("HEAD~%d"),LastSelect-headindex);
						CGitHash hash;
						if (g_Git.GetHash(hash, head))
							MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of ") + head + _T(".")), _T("TortoiseGit"), MB_ICONERROR);
						GitRevLoglist* pFirstEntry = m_arShownList.SafeGetAt(FirstSelect);
						GitRevLoglist* pLastEntry = m_arShownList.SafeGetAt(LastSelect);
						if (pFirstEntry->m_CommitHash == hashFirst && pLastEntry->m_CommitHash == hash) {
							popup.AppendMenuIcon(ID_COMBINE_COMMIT,IDS_COMBINE_TO_ONE,IDI_COMBINE);
							bAddSeparator = true;
						}
					}
				}
			}
			if (m_ContextMenuMask&GetContextMenuBit(ID_CHERRY_PICK) && !isHeadCommit && m_hasWC && !isMergeActive) {
				if (GetSelectedCount() >= 2)
					popup.AppendMenuIcon(ID_CHERRY_PICK, IDS_CHERRY_PICK_VERSIONS, IDI_EXPORT);
				else
					popup.AppendMenuIcon(ID_CHERRY_PICK, IDS_CHERRY_PICK_VERSION, IDI_EXPORT);
				bAddSeparator = true;
			}

			if (GetSelectedCount() <= 2 || (IsSelectionContinuous() && GetSelectedCount() > 0 && !isStash))
				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_PATCH)) {
					popup.AppendMenuIcon(ID_CREATE_PATCH, IDS_CREATE_PATCH, IDI_PATCH);
					bAddSeparator = true;
				}

			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (m_hasWC && !isMergeActive && !isStash && (m_ContextMenuMask & GetContextMenuBit(ID_BISECTSTART)) && GetSelectedCount() == 2 && !m_arShownList.SafeGetAt(FirstSelect)->m_CommitHash.IsEmpty() && !CTGitPath(g_Git.m_CurrentDir).IsBisectActive())
		{
			popup.AppendMenuIcon(ID_BISECTSTART, IDS_MENUBISECTSTART, IDI_BISECT);
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (GetSelectedCount() == 1)
		{
			bool bAddSeparator = false;
			if (m_ContextMenuMask&GetContextMenuBit(ID_PUSH) && ((!isStash && !m_HashMap[pSelLogEntry->m_CommitHash].empty()) || showExtendedMenu))
			{
				// show the push-option only if the log entry has an associated local branch
				bool isLocal = find_if(m_HashMap[pSelLogEntry->m_CommitHash], [](const CString& ref) { return ref.Find(_T("refs/heads/")) == 0; }) != m_HashMap[pSelLogEntry->m_CommitHash].cend();
				if (isLocal || showExtendedMenu)
				{
					CString str;
					str.LoadString(IDS_LOG_PUSH);

					CString branch;
					size_t index = (size_t)-1;
					CGit::REF_TYPE type = CGit::REF_TYPE::LOCAL_BRANCH;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, &branch, &index))
						str.Insert(str.Find(L'.'), L" \"" + branch + L'"');

					popup.AppendMenuIcon(ID_PUSH, str, IDI_PUSH);

					if (index != (size_t)-1 && index < m_HashMap[pSelLogEntry->m_CommitHash].size())
						popup.SetMenuItemData(ID_PUSH, (ULONG_PTR)&m_HashMap[pSelLogEntry->m_CommitHash][index]);

					bAddSeparator = true;
				}
			}
			if (m_ContextMenuMask & GetContextMenuBit(ID_PULL) && isHeadCommit && !isMergeActive && m_hasWC)
			{
				popup.AppendMenuIcon(ID_PULL, IDS_MENUPULL, IDI_PULL);
				bAddSeparator = true;
			}


			if(m_ContextMenuMask &GetContextMenuBit(ID_DELETE))
			{
				if( this->m_HashMap.find(pSelLogEntry->m_CommitHash) != m_HashMap.end() )
				{
					std::vector<const CString*> branchs;
					auto addCheck = [&](const CString& ref)
					{
						if (ref == currentBranch)
							return;
						branchs.push_back(&ref);
					};
					size_t index = (size_t)-1;
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, nullptr, &index))
						addCheck(m_HashMap[pSelLogEntry->m_CommitHash][index]);
					else
						for_each(m_HashMap[pSelLogEntry->m_CommitHash], addCheck);

					CString str;
					if (branchs.size() == 1)
					{
						str.LoadString(IDS_DELETE_BRANCHTAG_SHORT);
						str+=_T(' ');
						str += *branchs[0];
						popup.AppendMenuIcon(ID_DELETE, str, IDI_DELETE);
						popup.SetMenuItemData(ID_DELETE, (ULONG_PTR)branchs[0]);
						bAddSeparator = true;
					}
					else if (branchs.size() > 1)
					{
						str.LoadString(IDS_DELETE_BRANCHTAG);
						submenu.CreatePopupMenu();
						for (size_t i = 0; i < branchs.size(); ++i)
						{
							submenu.AppendMenuIcon(ID_DELETE + (i << 16), *branchs[i]);
							submenu.SetMenuItemData(ID_DELETE + (i << 16), (ULONG_PTR)branchs[i]);
						}
						submenu.AppendMenuIcon(ID_DELETE + (branchs.size() << 16), IDS_ALL);
						submenu.SetMenuItemData(ID_DELETE + (branchs.size() << 16), (ULONG_PTR)MAKEINTRESOURCE(IDS_ALL));

						popup.AppendMenuIcon(ID_DELETE,str, IDI_DELETE, submenu.m_hMenu);
						bAddSeparator = true;
					}
				}
			} // m_ContextMenuMask &GetContextMenuBit(ID_DELETE)
			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		} // GetSelectedCount() == 1

		if (GetSelectedCount() != 0)
		{
			if(m_ContextMenuMask&GetContextMenuBit(ID_COPYHASH))
				popup.AppendMenuIcon(ID_COPYHASH, IDS_COPY_COMMIT_HASH, IDI_COPYCLIP);
			if(m_ContextMenuMask&GetContextMenuBit(ID_COPYCLIPBOARD))
				popup.AppendMenuIcon(ID_COPYCLIPBOARD, IDS_LOG_POPUP_COPYTOCLIPBOARD, IDI_COPYCLIP);
			if(m_ContextMenuMask&GetContextMenuBit(ID_COPYCLIPBOARDMESSAGES))
				popup.AppendMenuIcon(ID_COPYCLIPBOARDMESSAGES, IDS_LOG_POPUP_COPYTOCLIPBOARDMESSAGES, IDI_COPYCLIP);
		}

		if(m_ContextMenuMask&GetContextMenuBit(ID_FINDENTRY))
			popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND, IDI_FILTEREDIT);

		if (GetSelectedCount() == 1 && m_ContextMenuMask & GetContextMenuBit(ID_SHOWBRANCHES) && !pSelLogEntry->m_CommitHash.IsEmpty())
			popup.AppendMenuIcon(ID_SHOWBRANCHES, IDS_LOG_POPUP_SHOWBRANCHES, IDI_SHOWBRANCHES);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
//		DialogEnableWindow(IDOK, FALSE);
//		SetPromptApp(&theApp);

		this->ContextMenuAction(cmd, FirstSelect, LastSelect, &popup);

//		EnableOKButton();
	} // if (popup.CreatePopupMenu())
}

bool CGitLogListBase::IsSelectionContinuous()
{
	if ( GetSelectedCount()==1 )
	{
		// if only one revision is selected, the selection is of course
		// continuous
		return true;
	}

	POSITION pos = GetFirstSelectedItemPosition();
	bool bContinuous = (m_arShownList.size() == m_logEntries.size());
	if (bContinuous)
	{
		int itemindex = GetNextSelectedItem(pos);
		while (pos)
		{
			int nextindex = GetNextSelectedItem(pos);
			if (nextindex - itemindex > 1)
			{
				bContinuous = false;
				break;
			}
			itemindex = nextindex;
		}
	}
	return bContinuous;
}

void CGitLogListBase::CopySelectionToClipBoard(int toCopy)
{
	CString sClipdata;
	POSITION pos = GetFirstSelectedItemPosition();
	if (pos)
	{
		CString sRev;
		sRev.LoadString(IDS_LOG_REVISION);
		CString sAuthor;
		sAuthor.LoadString(IDS_LOG_AUTHOR);
		CString sDate;
		sDate.LoadString(IDS_LOG_DATE);
		CString sMessage;
		sMessage.LoadString(IDS_LOG_MESSAGE);
		bool first = true;
		while (pos)
		{
			CString sLogCopyText;
			CString sPaths;
			GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(GetNextSelectedItem(pos));

			if (toCopy == ID_COPY_ALL)
			{
				//pLogEntry->GetFiles(this)
				//LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;

				CString from(MAKEINTRESOURCE(IDS_STATUSLIST_FROM));
				for (int cpPathIndex = 0; cpPathIndex<pLogEntry->GetFiles(this).GetCount(); ++cpPathIndex)
				{
					sPaths += ((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).GetActionName() + _T(": ") + pLogEntry->GetFiles(this)[cpPathIndex].GetGitPathString();
					if (((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).m_Action & (CTGitPath::LOGACTIONS_REPLACED|CTGitPath::LOGACTIONS_COPY) && !((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).GetGitOldPathString().IsEmpty())
					{
						sPaths += L' ';
						sPaths.AppendFormat(from, (LPCTSTR)((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).GetGitOldPathString());
					}
					sPaths += _T("\r\n");
				}
				sPaths.Trim();
				CString body = pLogEntry->GetBody();
				body.Replace(_T("\n"), _T("\r\n"));
				sLogCopyText.Format(_T("%s: %s\r\n%s: %s <%s>\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
					(LPCTSTR)sRev, (LPCTSTR)pLogEntry->m_CommitHash.ToString(),
					(LPCTSTR)sAuthor, (LPCTSTR)pLogEntry->GetAuthorName(), (LPCTSTR)pLogEntry->GetAuthorEmail(),
					(LPCTSTR)sDate,
					(LPCTSTR)CLoglistUtils::FormatDateAndTime(pLogEntry->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes),
					(LPCTSTR)sMessage, (LPCTSTR)pLogEntry->GetSubjectBody(true),
					(LPCTSTR)sPaths);
				sClipdata +=  sLogCopyText;
			}
			else if (toCopy == ID_COPY_MESSAGE)
			{
				sClipdata += _T("* ");
				sClipdata += pLogEntry->GetSubjectBody(true);
				sClipdata += _T("\r\n\r\n");
			}
			else if (toCopy == ID_COPY_SUBJECT)
			{
				sClipdata += _T("* ");
				sClipdata += pLogEntry->GetSubject().Trim();
				sClipdata += _T("\r\n\r\n");
			}
			else
			{
				if (!first)
					sClipdata += _T("\r\n");
				sClipdata += pLogEntry->m_CommitHash;
			}

			first = false;
		}
		CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
	}
}

void CGitLogListBase::DiffSelectedRevWithPrevious()
{
	if (m_bThreadRunning)
		return;

	int FirstSelect=-1, LastSelect=-1;
	POSITION pos = GetFirstSelectedItemPosition();
	FirstSelect = GetNextSelectedItem(pos);
	while(pos)
		LastSelect = GetNextSelectedItem(pos);

	ContextMenuAction(ID_COMPAREWITHPREVIOUS, FirstSelect, LastSelect, nullptr);
}

void CGitLogListBase::OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	*pResult = -1;

	if (pFindInfo->lvfi.flags & LVFI_PARAM)
		return;
	if (pFindInfo->iStart < 0 || pFindInfo->iStart >= (int)m_arShownList.size())
		return;
	if (!pFindInfo->lvfi.psz)
		return;
#if 0
	CString sCmp = pFindInfo->lvfi.psz;
	CString sRev;
	for (int i=pFindInfo->iStart; i<m_arShownList.GetCount(); ++i)
	{
		GitRev * pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.SafeGetAt(i));
		sRev.Format(_T("%ld"), pLogEntry->Rev);
		if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
		{
			if (sCmp.Compare(sRev.Left(sCmp.GetLength()))==0)
			{
				*pResult = i;
				return;
			}
		}
		else
		{
			if (sCmp.Compare(sRev)==0)
			{
				*pResult = i;
				return;
			}
		}
	}
	if (pFindInfo->lvfi.flags & LVFI_WRAP)
	{
		for (int i=0; i<pFindInfo->iStart; ++i)
		{
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(i));
			sRev.Format(_T("%ld"), pLogEntry->Rev);
			if (pFindInfo->lvfi.flags & LVFI_PARTIAL)
			{
				if (sCmp.Compare(sRev.Left(sCmp.GetLength()))==0)
				{
					*pResult = i;
					return;
				}
			}
			else
			{
				if (sCmp.Compare(sRev)==0)
				{
					*pResult = i;
					return;
				}
			}
		}
	}
#endif
	*pResult = -1;
}

int CGitLogListBase::FillGitLog(CTGitPath *path, CString *range, int info)
{
	ClearText();

	this->m_arShownList.SafeRemoveAll();

	this->m_logEntries.ClearAll();
	if (this->m_logEntries.ParserFromLog(path, 0, info, range))
		return -1;

	SetItemCountEx((int)m_logEntries.size());

	for (unsigned int i = 0; i < m_logEntries.size(); ++i)
	{
		if(m_IsOldFirst)
		{
			m_logEntries.GetGitRevAt(m_logEntries.size()-i-1).m_IsFull=TRUE;
			this->m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(m_logEntries.size()-i-1));
		}
		else
		{
			m_logEntries.GetGitRevAt(i).m_IsFull=TRUE;
			this->m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(i));
		}
	}

	ReloadHashMap();

	if(path)
		m_Path=*path;
	return 0;
}

int CGitLogListBase::FillGitLog(std::set<CGitHash>& hashes)
{
	ClearText();

	m_arShownList.SafeRemoveAll();

	m_logEntries.ClearAll();
	if (m_logEntries.Fill(hashes))
		return -1;

	SetItemCountEx((int)m_logEntries.size());

	for (unsigned int i = 0; i < m_logEntries.size(); ++i)
	{
		if (m_IsOldFirst)
		{
			m_logEntries.GetGitRevAt(m_logEntries.size() - i - 1).m_IsFull = TRUE;
			m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(m_logEntries.size() - i - 1));
		}
		else
		{
			m_logEntries.GetGitRevAt(i).m_IsFull = TRUE;
			m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(i));
		}
	}

	ReloadHashMap();

	return 0;
}

int CGitLogListBase::BeginFetchLog()
{
	ClearText();

	this->m_arShownList.SafeRemoveAll();

	this->m_logEntries.ClearAll();

	this->m_LogCache.ClearAllParent();

	m_LogCache.FetchCacheIndex(g_Git.m_CurrentDir);

	CTGitPath *path;
	if(this->m_Path.IsEmpty())
		path = nullptr;
	else
		path=&this->m_Path;

	int mask;
	mask = CGit::LOG_INFO_ONLY_HASH;
	if (m_bIncludeBoundaryCommits)
		mask |= CGit::LOG_INFO_BOUNDARY;
//	if(this->m_bAllBranch)
	mask |= m_ShowMask ;

	if(m_bShowWC)
	{
		this->m_logEntries.insert(m_logEntries.cbegin(), m_wcRev.m_CommitHash);
		ResetWcRev();
		this->m_LogCache.m_HashMap[m_wcRev.m_CommitHash]=m_wcRev;
	}

	if (m_sRange.IsEmpty())
		m_sRange = _T("HEAD");

#if 0 /* use tortoiegit filter */
	if (this->m_nSelectedFilter == LOGFILTER_ALL || m_nSelectedFilter == LOGFILTER_AUTHORS)
		data.m_Author = this->m_sFilterText;

	if(this->m_nSelectedFilter == LOGFILTER_ALL || m_nSelectedFilter == LOGFILTER_MESSAGES)
		data.m_MessageFilter = this->m_sFilterText;

	data.m_IsRegex = m_bFilterWithRegex;
#endif

	// follow does not work for directories
	if (!path || path->IsDirectory())
		mask &= ~CGit::LOG_INFO_FOLLOW;
	// follow does not work with all branches 8at least in TGit)
	if (mask & CGit::LOG_INFO_FOLLOW)
		mask &= ~(CGit::LOG_INFO_ALL_BRANCH | CGit::LOG_INFO_BASIC_REFS | CGit::LOG_INFO_LOCAL_BRANCHES);

	CString cmd = g_Git.GetLogCmd(m_sRange, path, mask, &m_Filter, CRegDWORD(L"Software\\TortoiseGit\\LogOrderBy", CGit::LOG_ORDER_TOPOORDER));

	//this->m_logEntries.ParserFromLog();
	if(IsInWorkingThread())
	{
		PostMessage(LVM_SETITEMCOUNT, (WPARAM) this->m_logEntries.size(),(LPARAM) LVSICF_NOINVALIDATEALL);
	}
	else
	{
		SetItemCountEx((int)m_logEntries.size());
	}

	try
	{
		[] { git_init(); } ();
	}
	catch (char* msg)
	{
		CString err(msg);
		MessageBox(_T("Could not initialize libgit.\nlibgit reports:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
		return -1;
	}

	if (!g_Git.CanParseRev(m_sRange))
	{
		if (!(mask & CGit::LOG_INFO_ALL_BRANCH) && !(mask & CGit::LOG_INFO_BASIC_REFS) && !(mask & CGit::LOG_INFO_LOCAL_BRANCHES))
			return 0;

		// if show all branches, pick any ref as dummy entry ref
		STRING_VECTOR list;
		if (g_Git.GetRefList(list))
			MessageBox(g_Git.GetGitLastErr(_T("Could not get all refs.")), _T("TortoiseGit"), MB_ICONERROR);
		if (list.empty())
			return 0;

		cmd = g_Git.GetLogCmd(list[0], path, mask, &m_Filter, CRegDWORD(L"Software\\TortoiseGit\\LogOrderBy", CGit::LOG_ORDER_TOPOORDER));
	}

	g_Git.m_critGitDllSec.Lock();
	try {
		if (git_open_log(&m_DllGitLog, CUnicodeUtils::GetMulti(cmd, CP_UTF8).GetBuffer()))
		{
			g_Git.m_critGitDllSec.Unlock();
			return -1;
		}
	}
	catch (char* msg)
	{
		g_Git.m_critGitDllSec.Unlock();
		CString err(msg);
		MessageBox(_T("Could not open log.\nlibgit reports:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
		return -1;
	}
	g_Git.m_critGitDllSec.Unlock();

	return 0;
}

BOOL CGitLogListBase::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam=='\r')
	{
		//if (GetFocus()==GetDlgItem(IDC_LOGLIST))
		{
			if (CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
			{
				DiffSelectedRevWithPrevious();
				return TRUE;
			}
		}
#if 0
		if (GetFocus()==GetDlgItem(IDC_LOGMSG))
		{
			DiffSelectedFile();
			return TRUE;
		}
#endif
	}
	else if (pMsg->message == WM_KEYDOWN && pMsg->wParam == 'A' && GetAsyncKeyState(VK_CONTROL)&0x8000)
	{
		// select all entries
		for (int i=0; i<GetItemCount(); ++i)
			SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		return TRUE;
	}

#if 0
	if (m_hAccel && !bSkipAccelerator)
	{
		int ret = TranslateAccelerator(m_hWnd, m_hAccel, pMsg);
		if (ret)
			return TRUE;
	}

#endif
	//m_tooltips.RelayEvent(pMsg);
	return __super::PreTranslateMessage(pMsg);
}

void CGitLogListBase::OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// a double click on an entry in the revision list has happened
	*pResult = 0;

	if (CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
		DiffSelectedRevWithPrevious();
}

void CGitLogListBase::FetchLogAsync(void* data)
{
	ReloadHashMap();
	m_ProcData=data;
	StartLoadingThread();
}

UINT CGitLogListBase::LogThreadEntry(LPVOID pVoid)
{
	return ((CGitLogListBase*)pVoid)->LogThread();
}

void CGitLogListBase::GetTimeRange(CTime &oldest, CTime &latest)
{
	//CTime time;
	oldest=CTime::GetCurrentTime();
	latest=CTime(1971,1,2,0,0,0);
	for (unsigned int i = 0; i < m_logEntries.size(); ++i)
	{
		if(m_logEntries[i].IsEmpty())
			continue;

		if (m_logEntries.GetGitRevAt(i).GetCommitterDate().GetTime() < oldest.GetTime())
			oldest = m_logEntries.GetGitRevAt(i).GetCommitterDate().GetTime();

		if (m_logEntries.GetGitRevAt(i).GetCommitterDate().GetTime() > latest.GetTime())
			latest = m_logEntries.GetGitRevAt(i).GetCommitterDate().GetTime();

	}

	if(latest<oldest)
		latest=oldest;
}

UINT CGitLogListBase::LogThread()
{
	::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) GITLOG_START,0);

	ULONGLONG  t1,t2;

	if(BeginFetchLog())
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);

		return 1;
	}

	std::tr1::wregex pat;//(_T("Remove"), tr1::regex_constants::icase);
	bool bRegex = false;
	if (m_bFilterWithRegex)
		bRegex = ValidateRegexp(m_sFilterText, pat, false);

	TRACE(_T("\n===Begin===\n"));
	//Update work copy item;

	if (!m_logEntries.empty())
	{
		GitRevLoglist* pRev = &m_logEntries.GetGitRevAt(0);

		m_arShownList.SafeAdd(pRev);
	}


	InterlockedExchange(&m_bNoDispUpdates, FALSE);

	// store commit number of the last selected commit/line before the refresh or -1
	int lastSelectedHashNItem = -1;
	if (m_lastSelectedHash.IsEmpty())
		lastSelectedHashNItem = 0;

	int ret = 0;

	bool shouldWalk = true;
	if (!g_Git.CanParseRev(m_sRange))
	{
		// walk revisions if show all branches and there exists any ref
		if (!(m_ShowMask & CGit::LOG_INFO_ALL_BRANCH) && !(m_ShowMask & CGit::LOG_INFO_BASIC_REFS) && !(m_ShowMask & CGit::LOG_INFO_LOCAL_BRANCHES))
			shouldWalk = false;
		else
		{
			STRING_VECTOR list;
			if (g_Git.GetRefList(list))
				MessageBox(g_Git.GetGitLastErr(_T("Could not get all refs.")), _T("TortoiseGit"), MB_ICONERROR);
			if (list.empty())
				shouldWalk = false;
		}
	}

	if (shouldWalk)
	{
		g_Git.m_critGitDllSec.Lock();
		int total = 0;
		try
		{
			[&] {git_get_log_firstcommit(m_DllGitLog);}();
			total = git_get_log_estimate_commit_count(m_DllGitLog);
		}
		catch (char* msg)
		{
			CString err(msg);
			MessageBox(_T("Could not get first commit.\nlibgit reports:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
			ret = -1;
		}
		g_Git.m_critGitDllSec.Unlock();

		GIT_COMMIT commit;
		t2 = t1 = GetTickCount64();
		int oldprecentage = 0;
		size_t oldsize = m_logEntries.size();
		std::map<CGitHash, std::set<CGitHash>> commitChildren;
		while (ret== 0 && !m_bExitThread)
		{
			g_Git.m_critGitDllSec.Lock();
			try
			{
				[&] { ret = git_get_log_nextcommit(this->m_DllGitLog, &commit, m_ShowMask & CGit::LOG_INFO_FOLLOW); } ();
			}
			catch (char* msg)
			{
				g_Git.m_critGitDllSec.Unlock();
				CString err(msg);
				MessageBox(_T("Could not get next commit.\nlibgit reports:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
				break;
			}
			g_Git.m_critGitDllSec.Unlock();

			if(ret)
			{
				if (ret != -2) // other than end of revision walking
					MessageBox((_T("Could not get next commit.\nlibgit returns:") + std::to_wstring(ret)).c_str(), _T("TortoiseGit"), MB_ICONERROR);
				break;
			}

			if (commit.m_ignore == 1)
			{
				git_free_commit(&commit);
				continue;
			}

			//printf("%s\r\n",commit.GetSubject());
			if(m_bExitThread)
				break;

			CGitHash hash = (char*)commit.m_hash ;

			GitRevLoglist* pRev = m_LogCache.GetCacheData(hash);
			pRev->m_GitCommit = commit;
			InterlockedExchange(&pRev->m_IsCommitParsed, FALSE);

			char* note = nullptr;
			g_Git.m_critGitDllSec.Lock();
			try
			{
				git_get_notes(commit.m_hash, &note);
			}
			catch (char* msg)
			{
				g_Git.m_critGitDllSec.Unlock();
				CString err(msg);
				MessageBox(_T("Could not get commit notes.\nlibgit reports:\n") + err, _T("TortoiseGit"), MB_ICONERROR);
				break;
			}
			g_Git.m_critGitDllSec.Unlock();

			if(note)
			{
				pRev->m_Notes = CUnicodeUtils::GetUnicode(note);
				free(note);
				note = nullptr;
			}

			if(!pRev->m_IsDiffFiles)
			{
				pRev->m_CallDiffAsync = DiffAsync;
			}

			pRev->ParserParentFromCommit(&commit);
			if (m_ShowFilter & FILTERSHOW_MERGEPOINTS) // See also ShouldShowFilter()
			{
				for (size_t i = 0; i < pRev->m_ParentHash.size(); ++i)
				{
					const CGitHash &parentHash = pRev->m_ParentHash[i];
					auto it = commitChildren.find(parentHash);
					if (it == commitChildren.end())
						it = commitChildren.insert(make_pair(parentHash, std::set<CGitHash>())).first;
					it->second.insert(pRev->m_CommitHash);
				}
			}

#ifdef DEBUG
			pRev->DbgPrint();
			TRACE(_T("\n"));
#endif

			bool visible = true;
			if (HasFilterText())
			{
				if(!IsMatchFilter(bRegex,pRev,pat))
					visible = false;
			}
			if (visible && !ShouldShowFilter(pRev, commitChildren))
				visible = false;
			this->m_critSec.Lock();
			m_logEntries.append(hash, visible);
			if (visible)
				m_arShownList.SafeAdd(pRev);
			this->m_critSec.Unlock();

			if (!visible)
				continue;

			if (lastSelectedHashNItem == -1 && hash == m_lastSelectedHash)
				lastSelectedHashNItem = (int)m_arShownList.size() - 1;

			t2 = GetTickCount64();

			if (t2 - t1 > 500UL || (m_logEntries.size() - oldsize > 100))
			{
				//update UI
				int percent = (int)m_logEntries.size() * 100 / total + GITLOG_START + 1;
				if(percent > 99)
					percent =99;
				if(percent < GITLOG_START)
					percent = GITLOG_START +1;

				oldsize = m_logEntries.size();
				PostMessage(LVM_SETITEMCOUNT, (WPARAM) this->m_logEntries.size(),(LPARAM) LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);

				//if( percent > oldprecentage )
				{
					::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) percent,0);
					oldprecentage = percent;
				}

				if (lastSelectedHashNItem >= 0)
					PostMessage(m_ScrollToMessage, lastSelectedHashNItem);

				t1 = t2;
			}
		}
		g_Git.m_critGitDllSec.Lock();
		git_close_log(m_DllGitLog);
		g_Git.m_critGitDllSec.Unlock();

	}

	if (m_bExitThread)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		return 0;
	}

	//Update UI;
	PostMessage(LVM_SETITEMCOUNT, (WPARAM) this->m_logEntries.size(),(LPARAM) LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);

	if (lastSelectedHashNItem >= 0)
		PostMessage(m_ScrollToMessage, lastSelectedHashNItem);

	if (this->m_hWnd)
		::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) GITLOG_END,0);

	InterlockedExchange(&m_bThreadRunning, FALSE);

	return 0;
}

void CGitLogListBase::FetchRemoteList()
{
	STRING_VECTOR remoteList;
	m_SingleRemote.Empty();
	if (!g_Git.GetRemoteList(remoteList) && remoteList.size() == 1)
		m_SingleRemote = remoteList[0];
}

void CGitLogListBase::FetchTrackingBranchList()
{
	m_TrackingMap.clear();
	for (auto it = m_HashMap.cbegin(); it != m_HashMap.cend(); ++it)
	{
		for (const auto& ref : it->second)
		{
			CString branchName;
			if (CGit::GetShortName(ref, branchName, _T("refs/heads/")))
			{
				CString pullRemote, pullBranch;
				g_Git.GetRemoteTrackedBranch(branchName, pullRemote, pullBranch);
				if (!pullRemote.IsEmpty() && !pullBranch.IsEmpty())
					m_TrackingMap[branchName] = std::make_pair(pullRemote, pullBranch);
			}
		}
	}
}

void CGitLogListBase::Refresh(BOOL IsCleanFilter)
{
	SafeTerminateThread();

	this->SetItemCountEx(0);
	this->Clear();

	ResetWcRev();

	ShowGraphColumn((m_ShowMask & CGit::LOG_INFO_FOLLOW) ? false : true);

	//Update branch and Tag info
	ReloadHashMap();
	if (m_pFindDialog)
		m_pFindDialog->RefreshList();
	//Assume Thread have exited
	//if(!m_bThreadRunning)
	{
		m_logEntries.clear();

		if(IsCleanFilter)
		{
			m_sFilterText.Empty();
		}

		SafeTerminateAsyncDiffThread();
		m_AsynDiffListLock.Lock();
		m_AsynDiffList.clear();
		m_AsynDiffListLock.Unlock();
		StartAsyncDiffThread();
		
		StartLoadingThread();
	}
}

void CGitLogListBase::StartAsyncDiffThread()
{
	if (m_AsyncThreadExit)
		return;
	if (InterlockedExchange(&m_AsyncThreadRunning, TRUE) != FALSE)
		return;
	m_DiffingThread = AfxBeginThread(AsyncThread, this, THREAD_PRIORITY_BELOW_NORMAL);
	if (!m_DiffingThread)
	{
		InterlockedExchange(&m_AsyncThreadRunning, FALSE);
		CMessageBox::Show(nullptr, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CGitLogListBase::StartLoadingThread()
{
	if (InterlockedExchange(&m_bThreadRunning, TRUE) != FALSE)
		return;
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	InterlockedExchange(&m_bExitThread, FALSE);
	m_LoadingThread = AfxBeginThread(LogThreadEntry, this, THREAD_PRIORITY_LOWEST);
	if (!m_LoadingThread)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		CMessageBox::Show(nullptr, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

bool CGitLogListBase::ValidateRegexp(LPCTSTR regexp_str, std::tr1::wregex& pat, bool bMatchCase /* = false */)
{
	try
	{
		std::tr1::regex_constants::syntax_option_type type = std::tr1::regex_constants::ECMAScript;
		if (!bMatchCase)
			type |= std::tr1::regex_constants::icase;
		pat = std::tr1::wregex(regexp_str, type);
		return true;
	}
	catch (std::exception) {}
	return false;
}
BOOL CGitLogListBase::IsMatchFilter(bool bRegex, GitRevLoglist* pRev, std::tr1::wregex& pat)
{
	BOOL result = TRUE;
	std::tr1::regex_constants::match_flag_type flags = std::tr1::regex_constants::match_any;
	CString sRev;

	if ((bRegex)&&(m_bFilterWithRegex))
	{
		if (m_SelectedFilters & LOGFILTER_BUGID)
		{
			if(this->m_bShowBugtraqColumn)
			{
				CString sBugIds = m_ProjectProperties.FindBugID(pRev->GetSubjectBody());

				ATLTRACE(_T("bugID = \"%s\"\n"), (LPCTSTR)sBugIds);
				if (std::regex_search(std::wstring(sBugIds), pat, flags))
					return TRUE;
			}
		}

		if ((m_SelectedFilters & LOGFILTER_SUBJECT) || (m_SelectedFilters & LOGFILTER_MESSAGES))
		{
			ATLTRACE(_T("messge = \"%s\"\n"), (LPCTSTR)pRev->GetSubject());
			if (std::regex_search(std::wstring((LPCTSTR)pRev->GetSubject()), pat, flags))
				return TRUE;
		}

		if (m_SelectedFilters & LOGFILTER_MESSAGES)
		{
			ATLTRACE(_T("messge = \"%s\"\n"), (LPCTSTR)pRev->GetBody());
			if (std::regex_search(std::wstring((LPCTSTR)pRev->GetBody()), pat, flags))
					return TRUE;
		}

		if (m_SelectedFilters & LOGFILTER_AUTHORS)
		{
			if (std::regex_search(std::wstring(pRev->GetAuthorName()), pat, flags))
				return TRUE;

			if (std::regex_search(std::wstring(pRev->GetCommitterName()), pat, flags))
				return TRUE;
		}

		if (m_SelectedFilters & LOGFILTER_EMAILS)
		{
			if (std::regex_search(std::wstring(pRev->GetAuthorEmail()), pat, flags))
				return TRUE;

			if (std::regex_search(std::wstring(pRev->GetCommitterEmail()), pat, flags))
				return TRUE;
		}

		if (m_SelectedFilters & LOGFILTER_REVS)
		{
			sRev = pRev->m_CommitHash.ToString();
			if (std::regex_search(std::wstring((LPCTSTR)sRev), pat, flags))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_NOTES)
		{
			if (std::regex_search(std::wstring(pRev->m_Notes), pat, flags))
				return TRUE;
		}

		if (m_SelectedFilters & LOGFILTER_REFNAME)
		{
			STRING_VECTOR refs = m_HashMap[pRev->m_CommitHash];
			for (const auto& ref : refs)
			{
				if (std::regex_search(std::wstring(ref), pat, flags))
					return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_ANNOTATEDTAG)
		{
			if (std::regex_search(std::wstring(GetTagInfo(pRev)), pat, flags))
				return TRUE;
		}

		if (m_SelectedFilters & LOGFILTER_PATHS)
		{
			CTGitPathList* pathList = nullptr;
			if( pRev->m_IsDiffFiles)
				pathList = &pRev->GetFiles(this);
			else
			{
				if(!pRev->m_IsSimpleListReady)
					pRev->SafeGetSimpleList(&g_Git);
			}

			if(pathList)
				for (INT_PTR cpPathIndex = 0; cpPathIndex < pathList->GetCount(); ++cpPathIndex)
				{
					if (std::regex_search(std::wstring((LPCTSTR)pathList->m_paths.at(cpPathIndex).GetGitOldPathString()), pat, flags))
						return true;
					if (std::regex_search(std::wstring((LPCTSTR)pathList->m_paths.at(cpPathIndex).GetGitPathString()), pat, flags))
						return true;
				}

			for (size_t i = 0; i < pRev->m_SimpleFileList.size(); ++i)
			{
				if (std::regex_search(std::wstring((LPCTSTR)pRev->m_SimpleFileList[i]), pat, flags))
					return true;
			}
		}
	}
	else
	{
		CString find = m_sFilterText;
		if (!m_bFilterCaseSensitively)
			find.MakeLower();
		result = find[0] == '!' ? FALSE : TRUE;
		if (!result)
			find = find.Mid(1);

		if (m_SelectedFilters & LOGFILTER_BUGID)
		{
			if(this->m_bShowBugtraqColumn)
			{
				CString sBugIds = m_ProjectProperties.FindBugID(pRev->GetSubjectBody());

				if (!m_bFilterCaseSensitively)
					sBugIds.MakeLower();
				if ((sBugIds.Find(find) >= 0))
					return result;
			}
		}

		if ((m_SelectedFilters & LOGFILTER_SUBJECT) || (m_SelectedFilters & LOGFILTER_MESSAGES))
		{
			CString msg = pRev->GetSubject();

			if (!m_bFilterCaseSensitively)
				msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
				return result;
		}

		if (m_SelectedFilters & LOGFILTER_MESSAGES)
		{
			CString msg = pRev->GetBody();

			if (!m_bFilterCaseSensitively)
				msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
				return result;
		}

		if (m_SelectedFilters & LOGFILTER_AUTHORS)
		{
			CString msg = pRev->GetAuthorName();
			if (!m_bFilterCaseSensitively)
				msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
				return result;
		}

		if (m_SelectedFilters & LOGFILTER_EMAILS)
		{
			CString msg = pRev->GetAuthorEmail();
			if (!m_bFilterCaseSensitively)
				msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
				return result;
		}

		if (m_SelectedFilters & LOGFILTER_NOTES)
		{
			CString msg = pRev->m_Notes;
			if (!m_bFilterCaseSensitively)
				msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
				return result;
		}

		if (m_SelectedFilters & LOGFILTER_REVS)
		{
			sRev = pRev->m_CommitHash.ToString();
			if ((sRev.Find(find) >= 0))
				return result;
		}

		if (m_SelectedFilters & LOGFILTER_ANNOTATEDTAG)
		{
			CString msg = GetTagInfo(pRev);
			if (!m_bFilterCaseSensitively)
				msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
				return result;
		}

		if (m_SelectedFilters & LOGFILTER_REFNAME)
		{
			STRING_VECTOR refs = m_HashMap[pRev->m_CommitHash];
			for (auto it = refs.cbegin(); it != refs.cend(); ++it)
			{
				if (it->Find(find) >= 0)
					return result;
			}
		}

		if (m_SelectedFilters & LOGFILTER_PATHS)
		{
			CTGitPathList* pathList = nullptr;
			if( pRev->m_IsDiffFiles)
				pathList = &pRev->GetFiles(this);
			else
			{
				if(!pRev->m_IsSimpleListReady)
					pRev->SafeGetSimpleList(&g_Git);
			}
			if(pathList)
				for (INT_PTR cpPathIndex = 0; cpPathIndex < pathList->GetCount() ; ++cpPathIndex)
				{
					CTGitPath *cpath = &pathList->m_paths.at(cpPathIndex);
					CString path = cpath->GetGitOldPathString();
					if (!m_bFilterCaseSensitively)
						path.MakeLower();
					if ((path.Find(find)>=0))
						return result;
					path = cpath->GetGitPathString();
					if (!m_bFilterCaseSensitively)
						path.MakeLower();
					if ((path.Find(find)>=0))
						return result;
				}

			for (size_t i = 0; i < pRev->m_SimpleFileList.size(); ++i)
			{
				CString path = pRev->m_SimpleFileList[i];
				if (!m_bFilterCaseSensitively)
					path.MakeLower();
				if ((path.Find(find)>=0))
					return result;
			}
		}
	} // else (from if (bRegex))
	return !result;
}

static bool CStringStartsWith(const CString &str, const CString &prefix)
{
	return str.Left(prefix.GetLength()) == prefix;
}
bool CGitLogListBase::ShouldShowFilter(GitRevLoglist* pRev, const std::map<CGitHash, std::set<CGitHash>>& commitChildren)
{
	if (m_ShowFilter & FILTERSHOW_ANYCOMMIT)
		return true;

	if (m_ShowFilter & FILTERSHOW_REFS)
	{
		// Keep all refs.
		const STRING_VECTOR &refList = m_HashMap[pRev->m_CommitHash];
		for (size_t i = 0; i < refList.size(); ++i)
		{
			const CString &str = refList[i];
			if (CStringStartsWith(str, _T("refs/heads/")))
			{
				if (m_ShowRefMask & LOGLIST_SHOWLOCALBRANCHES)
					return true;
			}
			else if (CStringStartsWith(str, _T("refs/remotes/")))
			{
				if (m_ShowRefMask & LOGLIST_SHOWREMOTEBRANCHES)
					return true;
			}
			else if (CStringStartsWith(str, _T("refs/tags/")))
			{
				if (m_ShowRefMask & LOGLIST_SHOWTAGS)
					return true;
			}
			else if (CStringStartsWith(str, _T("refs/stash")))
			{
				if (m_ShowRefMask & LOGLIST_SHOWSTASH)
					return true;
			}
			else if (CStringStartsWith(str, _T("refs/bisect/")))
			{
				if (m_ShowRefMask & LOGLIST_SHOWBISECT)
					return true;
			}
		}
		// Keep the head too.
		if (pRev->m_CommitHash == m_HeadHash)
			return true;
	}

	if (m_ShowFilter & FILTERSHOW_MERGEPOINTS)
	{
		if (pRev->ParentsCount() > 1)
			return true;
		auto childrenIt = commitChildren.find(pRev->m_CommitHash);
		if (childrenIt != commitChildren.end())
		{
			const std::set<CGitHash> &children = childrenIt->second;
			if (children.size() > 1)
				return true;
		}
	}
	return false;
}

void CGitLogListBase::ShowGraphColumn(bool bShow)
{
	// HACK to hide graph column
	if (bShow)
		SetColumnWidth(0, m_ColumnManager.GetWidth(0, false));
	else
		SetColumnWidth(0, 0);
}

CString CGitLogListBase::GetTagInfo(GitRev* pLogEntry)
{
	CString cmd;
	CString output;

	if (m_HashMap.find(pLogEntry->m_CommitHash) != m_HashMap.end())
	{
		STRING_VECTOR &vector = m_HashMap[pLogEntry->m_CommitHash];
		for (size_t i = 0; i < vector.size(); ++i)
		{
			if (vector[i].Find(_T("refs/tags/")) == 0)
			{
				CString tag = vector[i];
				int start = vector[i].Find(_T("^{}"));
				if (start > 0)
					tag = tag.Left(start);
				else
					continue;

				cmd.Format(_T("git.exe cat-file	tag %s"), (LPCTSTR)tag);
				if (g_Git.Run(cmd, &output, nullptr, CP_UTF8) == 0)
					output.AppendChar(_T('\n'));
			}
		}
	}

	return output;
}

void CGitLogListBase::RecalculateShownList(CThreadSafePtrArray * pShownlist)
{
	pShownlist->SafeRemoveAll();

	std::tr1::wregex pat;//(_T("Remove"), tr1::regex_constants::icase);
	bool bRegex = false;
	if (m_bFilterWithRegex)
		bRegex = ValidateRegexp(m_sFilterText, pat, false);

	std::tr1::regex_constants::match_flag_type flags = std::tr1::regex_constants::match_any;
	CString sRev;
	for (DWORD i=0; i<m_logEntries.size(); ++i)
	{
		if ((bRegex)&&(m_bFilterWithRegex))
		{
#if 0
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_BUGID))
			{
				ATLTRACE(_T("bugID = \"%s\"\n"), (LPCTSTR)m_logEntries[i]->sBugIDs);
				if (std::regex_search(std::wstring((LPCTSTR)m_logEntries[i]->sBugIDs), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(m_logEntries[i]);
					continue;
				}
			}
#endif
			if ((m_SelectedFilters & LOGFILTER_SUBJECT) || (m_SelectedFilters & LOGFILTER_MESSAGES))
			{
				ATLTRACE(_T("messge = \"%s\"\n"),m_logEntries.GetGitRevAt(i).GetSubject());
				if (std::regex_search(std::wstring((LPCTSTR)m_logEntries.GetGitRevAt(i).GetSubject()), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_MESSAGES)
			{
				ATLTRACE(_T("messge = \"%s\"\n"),m_logEntries.GetGitRevAt(i).GetBody());
				if (std::regex_search(std::wstring((LPCTSTR)m_logEntries.GetGitRevAt(i).GetBody()), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_PATHS)
			{
				CTGitPathList pathList = m_logEntries.GetGitRevAt(i).GetFiles(this);

				bool bGoing = true;
				for (INT_PTR cpPathIndex = 0; cpPathIndex < pathList.GetCount() && bGoing; ++cpPathIndex)
				{
					CTGitPath cpath = pathList[cpPathIndex];
					if (std::regex_search(std::wstring((LPCTSTR)cpath.GetGitOldPathString()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					if (std::regex_search(std::wstring((LPCTSTR)cpath.GetGitPathString()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					if (std::regex_search(std::wstring((LPCTSTR)cpath.GetActionName()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
				}
			}
			if (m_SelectedFilters & LOGFILTER_AUTHORS)
			{
				if (std::regex_search(std::wstring((LPCTSTR)m_logEntries.GetGitRevAt(i).GetAuthorName()), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_EMAILS)
			{
				if (std::regex_search(std::wstring((LPCTSTR)m_logEntries.GetGitRevAt(i).GetAuthorEmail()), pat, flags) && IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_REVS)
			{
				sRev = m_logEntries.GetGitRevAt(i).m_CommitHash.ToString();
				if (std::regex_search(std::wstring((LPCTSTR)sRev), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_REFNAME)
			{
				STRING_VECTOR refs = m_HashMap[m_logEntries.GetGitRevAt(i).m_CommitHash];
				for (auto it = refs.cbegin(); it != refs.cend(); ++it)
				{
					if (std::regex_search(std::wstring((LPCTSTR)*it), pat, flags) && IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						continue;
					}
				}
			}
		} // if (bRegex)
		else
		{
			CString find = m_sFilterText;
			if (!m_bFilterCaseSensitively)
				find.MakeLower();
#if 0
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_BUGID))
			{
				CString sBugIDs = m_logEntries[i]->sBugIDs;

				if (!m_bFilterCaseSensitively)
					sBugIDs = sBugIDs.MakeLower();
				if ((sBugIDs.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(m_logEntries[i]);
					continue;
				}
			}
#endif
			if ((m_SelectedFilters & LOGFILTER_SUBJECT) || (m_SelectedFilters & LOGFILTER_MESSAGES))
			{
				CString msg = m_logEntries.GetGitRevAt(i).GetSubject();

				if (!m_bFilterCaseSensitively)
					msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_MESSAGES)
			{
				CString msg = m_logEntries.GetGitRevAt(i).GetBody();

				if (!m_bFilterCaseSensitively)
					msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_PATHS)
			{
				CTGitPathList pathList = m_logEntries.GetGitRevAt(i).GetFiles(this);

				bool bGoing = true;
				for (INT_PTR cpPathIndex = 0; cpPathIndex < pathList.GetCount() && bGoing; ++cpPathIndex)
				{
					CTGitPath cpath = pathList[cpPathIndex];
					CString path = cpath.GetGitOldPathString();
					if (!m_bFilterCaseSensitively)
						path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					path = cpath.GetGitPathString();
					if (!m_bFilterCaseSensitively)
						path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					path = cpath.GetActionName();
					if (!m_bFilterCaseSensitively)
						path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
				}
			}
			if (m_SelectedFilters & LOGFILTER_AUTHORS)
			{
				CString msg = m_logEntries.GetGitRevAt(i).GetAuthorName();
				if (!m_bFilterCaseSensitively)
					msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_EMAILS)
			{
				CString msg = m_logEntries.GetGitRevAt(i).GetAuthorEmail();
				if (!m_bFilterCaseSensitively)
					msg = msg.MakeLower();
				if ((msg.Find(find) >= 0) && (IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_REVS)
			{
				sRev = m_logEntries.GetGitRevAt(i).m_CommitHash.ToString();
				if ((sRev.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_REFNAME)
			{
				STRING_VECTOR refs = m_HashMap[m_logEntries.GetGitRevAt(i).m_CommitHash];
				for (auto it = refs.cbegin(); it != refs.cend(); ++it)
				{
					if (it->Find(find) >= 0 && IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						continue;
					}
				}
			}
		} // else (from if (bRegex))
	} // for (DWORD i=0; i<m_logEntries.size(); ++i)
}

BOOL CGitLogListBase::IsEntryInDateRange(int /*i*/)
{
	/*
	__time64_t time = m_logEntries.GetGitRevAt(i).GetAuthorDate().GetTime();

	if(m_From == -1)
		if(m_To == -1)
			return true;
		else
			return time <= m_To;
	else
		if(m_To == -1)
			return time >= m_From;
		else
			return ((time >= m_From)&&(time <= m_To));
	*/
	return TRUE; /* git dll will filter time range */

//	return TRUE;
}

void CGitLogListBase::StartFilter()
{
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	RecalculateShownList(&m_arShownList);
	InterlockedExchange(&m_bNoDispUpdates, FALSE);


	DeleteAllItems();
	SetItemCountEx(ShownCountWithStopped());
	RedrawItems(0, ShownCountWithStopped());
	Invalidate();
}

void CGitLogListBase::RemoveFilter()
{
	InterlockedExchange(&m_bNoDispUpdates, TRUE);

	m_arShownList.SafeRemoveAll();

	// reset the time filter too
#if 0
	m_timFrom = (__time64_t(m_tFrom));
	m_timTo = (__time64_t(m_tTo));
	m_DateFrom.SetTime(&m_timFrom);
	m_DateTo.SetTime(&m_timTo);
	m_DateFrom.SetRange(&m_timFrom, &m_timTo);
	m_DateTo.SetRange(&m_timFrom, &m_timTo);
#endif

	for (DWORD i=0; i<m_logEntries.size(); ++i)
	{
		if(this->m_IsOldFirst)
			m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(m_logEntries.size()-i-1));
		else
			m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(i));
	}
//	InterlockedExchange(&m_bNoDispUpdates, FALSE);
	DeleteAllItems();
	SetItemCountEx(ShownCountWithStopped());
	RedrawItems(0, ShownCountWithStopped());

	InterlockedExchange(&m_bNoDispUpdates, FALSE);
}

void CGitLogListBase::Clear()
{
	m_arShownList.SafeRemoveAll();
	DeleteAllItems();

	m_logEntries.ClearAll();
}

void CGitLogListBase::OnDestroy()
{
	// save the column widths to the registry
	SaveColumnWidths();

	SafeTerminateThread();
	SafeTerminateAsyncDiffThread();

	int retry = 0;
	while(m_LogCache.SaveCache())
	{
		if(retry > 5)
			break;
		Sleep(1000);

		++retry;

		//if(CMessageBox::Show(nullptr, _T("Cannot Save Log Cache to Disk. To retry click yes. To give up click no."), _T("TortoiseGit"),
		//					MB_YESNO) == IDNO)
		//					break;
	}

	CHintListCtrl::OnDestroy();
}

LRESULT CGitLogListBase::OnLoad(WPARAM wParam,LPARAM /*lParam*/)
{
	CRect rect;
	int i=(int)wParam;
	this->GetItemRect(i,&rect,LVIR_BOUNDS);
	this->InvalidateRect(rect);

	return 0;
}

/**
 * Save column widths to the registry
 */
void CGitLogListBase::SaveColumnWidths()
{
	int maxcol = m_ColumnManager.GetColumnCount();

	// HACK that graph column is always shown
	SetColumnWidth(0, m_ColumnManager.GetWidth(0, false));

	for (int col = 0; col < maxcol; ++col)
		if (m_ColumnManager.IsVisible (col))
			m_ColumnManager.ColumnResized (col);

	m_ColumnManager.WriteSettings();
}

int CGitLogListBase::GetHeadIndex()
{
	if(m_HeadHash.IsEmpty())
		return -1;

	for (size_t i = 0; i < m_arShownList.size(); ++i)
	{
		GitRev* pRev = m_arShownList.SafeGetAt(i);
		if(pRev)
		{
			if(pRev->m_CommitHash.ToString() == m_HeadHash )
				return (int)i;
		}
	}
	return -1;
}
void CGitLogListBase::OnFind()
{
	if (!m_pFindDialog)
	{
		m_pFindDialog = new CFindDlg(this);
		m_pFindDialog->Create(this);
	}
}
void CGitLogListBase::OnHdnBegintrack(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_ColumnManager.OnHdnBegintrack(pNMHDR, pResult);
}
void CGitLogListBase::OnHdnItemchanging(NMHDR *pNMHDR, LRESULT *pResult)
{
	if(!m_ColumnManager.OnHdnItemchanging(pNMHDR, pResult))
		Default();
}
LRESULT CGitLogListBase::OnScrollToMessage(WPARAM itemToSelect, LPARAM /*lParam*/)
{
	if (GetSelectedCount() != 0)
		return 0;

	CGitHash theSelectedHash = m_lastSelectedHash;
	SetItemState((int)itemToSelect, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_lastSelectedHash = theSelectedHash;

	int countPerPage = GetCountPerPage();
	EnsureVisible(max(0, (int)itemToSelect-countPerPage/2), FALSE);
	EnsureVisible(min(GetItemCount(), (int)itemToSelect+countPerPage/2), FALSE);
	EnsureVisible((int)itemToSelect, FALSE);
	return 0;
}
LRESULT CGitLogListBase::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	ASSERT(m_pFindDialog);
	bool bFound = false;
	int i=0;

	if (m_pFindDialog->IsTerminating())
	{
		// invalidate the handle identifying the dialog box.
		m_pFindDialog = nullptr;
		return 0;
	}

	int cnt = (int)m_arShownList.size();

	if(m_pFindDialog->IsRef())
	{
		CString str;
		str=m_pFindDialog->GetFindString();

		CGitHash hash;

		if(!str.IsEmpty())
		{
			if (g_Git.GetHash(hash, str + _T("^{}"))) // add ^{} in order to get the correct SHA-1 (especially for signed tags)
				MessageBox(g_Git.GetGitLastErr(_T("Could not get hash of ref \"") + str + _T("^{}\".")), _T("TortoiseGit"), MB_ICONERROR);
		}

		if(!hash.IsEmpty())
		{
			for (i = 0; i < cnt; ++i)
			{
				GitRev* pLogEntry = m_arShownList.SafeGetAt(i);
				if(pLogEntry && pLogEntry->m_CommitHash == hash)
				{
					bFound = true;
					break;
				}
			}
		}
		if (!bFound)
		{
			m_pFindDialog->FlashWindowEx(FLASHW_ALL, 2, 100);
			return 0;
		}
	}

	if (m_pFindDialog->FindNext() && !bFound)
	{
		//read data from dialog
		CString findText = m_pFindDialog->GetFindString();
		bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);

		std::tr1::wregex pat;
		bool bRegex = false;
		if (m_pFindDialog->Regex())
			bRegex = ValidateRegexp(findText, pat, bMatchCase);

		std::tr1::regex_constants::match_flag_type flags = std::tr1::regex_constants::match_not_null;

		for (i = m_nSearchIndex + 1; ; ++i)
		{
			if (i >= cnt)
			{
				i = 0;
				m_pFindDialog->FlashWindowEx(FLASHW_ALL, 2, 100);
			}
			if (m_nSearchIndex >= 0)
			{
				if (i == m_nSearchIndex)
				{
					::MessageBeep(0xFFFFFFFF);
					m_pFindDialog->FlashWindowEx(FLASHW_ALL, 3, 100);
					break;
				}
			}

			GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(i);

			CString str;
			str+=pLogEntry->m_CommitHash.ToString();
			str += _T('\n');

			for (size_t j = 0; j < this->m_HashMap[pLogEntry->m_CommitHash].size(); ++j)
			{
				str+=m_HashMap[pLogEntry->m_CommitHash][j];
				str += _T('\n');
			}

			str+=pLogEntry->GetAuthorEmail();
			str += _T('\n');
			str+=pLogEntry->GetAuthorName();
			str += _T('\n');
			str+=pLogEntry->GetBody();
			str += _T('\n');
			str+=pLogEntry->GetCommitterEmail();
			str += _T('\n');
			str+=pLogEntry->GetCommitterName();
			str += _T('\n');
			str+=pLogEntry->GetSubject();
			str += _T('\n');
			str+=pLogEntry->m_Notes;
			str += _T('\n');
			str+=GetTagInfo(pLogEntry);
			str += _T('\n');


			/*Because changed files list is loaded on demand when gui show,
			  files will empty when files have not fetched.

			  we can add it back by using one-way diff(with outnumber changed and rename detect.
			  here just need changed filename list. one-way is much quicker.
			*/
			if(pLogEntry->m_IsFull)
			{
				for (int j = 0; j < pLogEntry->GetFiles(this).GetCount(); ++j)
				{
					str += pLogEntry->GetFiles(this)[j].GetWinPath();
					str += _T('\n');
					str += pLogEntry->GetFiles(this)[j].GetGitOldPathString();
					str += _T('\n');
				}
			}
			else
			{
				if(!pLogEntry->m_IsSimpleListReady)
					pLogEntry->SafeGetSimpleList(&g_Git);

				for (size_t j = 0; j < pLogEntry->m_SimpleFileList.size(); ++j)
				{
					str += pLogEntry->m_SimpleFileList[j];
					str += _T('\n');
				}
			}

			if (bRegex)
			{
				if (std::regex_search(std::wstring(str), pat, flags))
				{
					bFound = true;
					break;
				}
			}
			else
			{
				if (bMatchCase)
				{
					if (str.Find(findText) >= 0)
					{
						bFound = true;
						break;
					}
				}
				else
				{
					CString msg = str;
					msg = msg.MakeLower();
					CString find = findText.MakeLower();
					if (msg.Find(find) >= 0)
					{
						bFound = TRUE;
						break;
					}
				}
			}
		} // for (i = this->m_nSearchIndex; i<m_arShownList.GetItemCount()&&!bFound; ++i)

	} // if(m_pFindDialog->FindNext())
	//UpdateLogInfoLabel();

	if (bFound)
	{
		m_nSearchIndex = i;
		EnsureVisible(i, FALSE);
		if ((GetAsyncKeyState(VK_SHIFT) & 0x8000) == 0)
		{
			SetItemState(GetSelectionMark(), 0, LVIS_SELECTED);
			SetItemState(i, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			SetSelectionMark(i);
		}
		else
		{
			GitRev* pLogEntry = m_arShownList.SafeGetAt(i);
			if (pLogEntry)
				m_highlight = pLogEntry->m_CommitHash;
		}
		Invalidate();
		//FillLogMessageCtrl();
		UpdateData(FALSE);
	}

	return 0;
}

void CGitLogListBase::OnColumnResized(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_ColumnManager.OnColumnResized(pNMHDR,pResult);

	*pResult = FALSE;
}

void CGitLogListBase::OnColumnMoved(NMHDR *pNMHDR, LRESULT *pResult)
{
	m_ColumnManager.OnColumnMoved(pNMHDR, pResult);

	Invalidate(FALSE);
}

INT_PTR CGitLogListBase::OnToolHitTest(CPoint point, TOOLINFO * pTI) const
{
	LVHITTESTINFO lvhitTestInfo;

	lvhitTestInfo.pt = point;

	int nItem = ListView_SubItemHitTest(m_hWnd, &lvhitTestInfo);
	int nSubItem = lvhitTestInfo.iSubItem;

	UINT nFlags = lvhitTestInfo.flags;

	// nFlags is 0 if the SubItemHitTest fails
	// Therefore, 0 & <anything> will equal false
	if (nFlags & LVHT_ONITEM)
	{
		// Get the client area occupied by this control
		RECT rcClient;
		GetClientRect(&rcClient);

		// Fill in the TOOLINFO structure
		pTI->hwnd = m_hWnd;
		pTI->uId = (UINT)((nItem<<10)+(nSubItem&0x3ff)+1);
		pTI->lpszText = LPSTR_TEXTCALLBACK;
		pTI->rect = rcClient;

		return pTI->uId; // By returning a unique value per listItem,
		// we ensure that when the mouse moves over another list item,
		// the tooltip will change
	}
	else
	{
		// Otherwise, we aren't interested, so let the message propagate
		return -1;
	}
}

BOOL CGitLogListBase::OnToolTipText(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult)
{
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

	*pResult = 0;

	// Ignore messages from the built in tooltip, we are processing them internally
	if ((pNMHDR->idFrom == (UINT_PTR)m_hWnd) &&
		(((pNMHDR->code == TTN_NEEDTEXTA) && (pTTTA->uFlags & TTF_IDISHWND)) ||
		((pNMHDR->code == TTN_NEEDTEXTW) && (pTTTW->uFlags & TTF_IDISHWND))))
		return FALSE;

	// Get the mouse position
	const MSG* pMessage = GetCurrentMessage();

	CPoint pt;
	pt = pMessage->pt;
	ScreenToClient(&pt);

	// Check if the point falls onto a list item
	LVHITTESTINFO lvhitTestInfo;
	lvhitTestInfo.pt = pt;

	int nItem = SubItemHitTest(&lvhitTestInfo);

	if (lvhitTestInfo.flags & LVHT_ONITEM)
	{
		// Get branch description first
		CString strTipText;
		if (lvhitTestInfo.iSubItem == LOGLIST_MESSAGE)
		{
			CString branch;
			CGit::REF_TYPE type = CGit::REF_TYPE::LOCAL_BRANCH;
			if (IsMouseOnRefLabel(m_arShownList.SafeGetAt(nItem), lvhitTestInfo.pt, type, &branch))
			{
				MAP_STRING_STRING descriptions;
				g_Git.GetBranchDescriptions(descriptions);
				if (descriptions.find(branch) != descriptions.cend())
				{
					strTipText.LoadString(IDS_DESCRIPTION);
					strTipText += L":\n";
					strTipText += descriptions[branch];
				}
			}
		}

		bool followMousePos = false;
		if (!strTipText.IsEmpty())
			followMousePos = true;
		else
			strTipText = GetToolTipText(nItem, lvhitTestInfo.iSubItem);
		if (strTipText.IsEmpty())
			return FALSE;

		// we want multiline tooltips
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, INT_MAX);

		wcscpy_s(m_wszTip, strTipText);
		// handle Unicode as well as non-Unicode requests
		if (pNMHDR->code == TTN_NEEDTEXTA)
		{
			pTTTA->hinst = nullptr;
			pTTTA->lpszText = m_szTip;
			::WideCharToMultiByte(CP_ACP, 0, m_wszTip, -1, m_szTip, 8192, nullptr, nullptr);
		}
		else
		{
			pTTTW->hinst = nullptr;
			pTTTW->lpszText = m_wszTip;
		}

		CRect rect;
		GetSubItemRect(nItem, lvhitTestInfo.iSubItem, LVIR_LABEL, rect);
		if (followMousePos)
			rect.MoveToXY(pt.x, pt.y + 18); // 18: to act like a normal tooltip
		ClientToScreen(rect);
		::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, rect.left, rect.top, 0, 0, SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER);

		return TRUE; // We found a tool tip,
		// tell the framework this message has been handled
	}

	return FALSE; // We didn't handle the message,
	// let the framework continue propagating the message
}

CString CGitLogListBase::GetToolTipText(int nItem, int nSubItem)
{
	if (nSubItem == LOGLIST_MESSAGE && !m_bTagsBranchesOnRightSide)
	{
		GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(nItem);
		if (pLogEntry == nullptr)
			return CString();
		if (m_HashMap[pLogEntry->m_CommitHash].empty())
			return CString();
		return pLogEntry->GetSubject();
	}
	else if (nSubItem == LOGLIST_DATE && m_bRelativeTimes)
	{
		GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(nItem);
		if (pLogEntry == nullptr)
			return CString();
		return CLoglistUtils::FormatDateAndTime(pLogEntry->GetAuthorDate(), m_DateFormat, true, false);
	}
	else if (nSubItem == LOGLIST_COMMIT_DATE && m_bRelativeTimes)
	{
		GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(nItem);
		if (pLogEntry == nullptr)
			return CString();
		return CLoglistUtils::FormatDateAndTime(pLogEntry->GetCommitterDate(), m_DateFormat, true, false);
	}
	else if (nSubItem == LOGLIST_ACTION)
	{
		GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(nItem);
		if (pLogEntry == nullptr)
			return CString();

		if (!pLogEntry->m_IsDiffFiles)
			return CString(MAKEINTRESOURCE(IDS_PROC_LOG_FETCHINGFILES));

		int actions = pLogEntry->GetAction(this);
		CString sToolTipText;

		CString actionText;
		if (actions & CTGitPath::LOGACTIONS_MODIFIED)
			actionText += CTGitPath::GetActionName(CTGitPath::LOGACTIONS_MODIFIED);

		if (actions & CTGitPath::LOGACTIONS_ADDED)
		{
			if (!actionText.IsEmpty())
				actionText += L"\r\n";
			actionText += CTGitPath::GetActionName(CTGitPath::LOGACTIONS_ADDED);
		}

		if (actions & CTGitPath::LOGACTIONS_DELETED)
		{
			if (!actionText.IsEmpty())
				actionText += L"\r\n";
			actionText += CTGitPath::GetActionName(CTGitPath::LOGACTIONS_DELETED);
		}

		if (actions & CTGitPath::LOGACTIONS_REPLACED)
		{
			if (!actionText.IsEmpty())
				actionText += L"\r\n";
			actionText += CTGitPath::GetActionName(CTGitPath::LOGACTIONS_REPLACED);
		}

		if (actions & CTGitPath::LOGACTIONS_UNMERGED)
		{
			if (!actionText.IsEmpty())
				actionText += L"\r\n";
			actionText += CTGitPath::GetActionName(CTGitPath::LOGACTIONS_UNMERGED);
		}

		if (!actionText.IsEmpty())
		{
			CString sTitle(MAKEINTRESOURCE(IDS_LOG_ACTIONS));
			sToolTipText = sTitle + L":\r\n" + actionText;
		}
		return sToolTipText;
	}
	return CString();
}

bool CGitLogListBase::IsMouseOnRefLabelFromPopupMenu(const GitRevLoglist* pLogEntry, const CPoint& point, CGit::REF_TYPE& type, CString* pShortname /*nullptr*/, size_t* pIndex /*nullptr*/)
{
	POINT pt = point;
	ScreenToClient(&pt);
	return IsMouseOnRefLabel(pLogEntry, pt, type, pShortname, pIndex);
}

bool CGitLogListBase::IsMouseOnRefLabel(const GitRevLoglist* pLogEntry, const POINT& pt, CGit::REF_TYPE& type, CString* pShortname /*nullptr*/, size_t* pIndex /*nullptr*/)
{
	if (!pLogEntry)
		return false;

	for (size_t i = 0; i < m_HashMap[pLogEntry->m_CommitHash].size(); ++i)
	{
		const auto labelpos = m_RefLabelPosMap.find(m_HashMap[pLogEntry->m_CommitHash][i]);
		if (labelpos == m_RefLabelPosMap.cend() || !labelpos->second.PtInRect(pt))
			continue;

		CGit::REF_TYPE foundType;
		if (pShortname)
			*pShortname = CGit::GetShortName(m_HashMap[pLogEntry->m_CommitHash][i], &foundType);
		else
			CGit::GetShortName(m_HashMap[pLogEntry->m_CommitHash][i], &foundType);
		if (foundType != type && type != CGit::REF_TYPE::UNKNOWN)
			return false;

		type = foundType;
		if (pIndex)
			*pIndex = i;
		return true;
	}
	return false;
}
