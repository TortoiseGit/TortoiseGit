// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
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
#include "GitLogListBase.h"
#include "IconMenu.h"
#include "GitProgressDlg.h"
#include "ProgressDlg.h"
#include "MessageBox.h"
#include "LoglistUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "../TortoiseShell/Resource.h"
#include "CommonAppUtils.h"
#include "DPIAware.h"

const UINT CGitLogListBase::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);
const UINT CGitLogListBase::m_ScrollToMessage = RegisterWindowMessage(L"TORTOISEGIT_LOG_SCROLLTO");
const UINT CGitLogListBase::m_ScrollToRef = RegisterWindowMessage(L"TORTOISEGIT_LOG_SCROLLTOREF");
const UINT CGitLogListBase::m_RebaseActionMessage = RegisterWindowMessage(L"TORTOISEGIT_LOG_REBASEACTION");
const UINT CGitLogListBase::LOGLIST_RESET_WCREV = RegisterWindowMessage(L"TORTOISEGIT_LOG_RESET_WCREV");

IMPLEMENT_DYNAMIC(CGitLogListBase, CHintCtrl<CResizableColumnsListCtrl<CListCtrl>>)

CGitLogListBase::CGitLogListBase() : CHintCtrl<CResizableColumnsListCtrl<CListCtrl>>()
	,m_regMaxBugIDColWidth(L"Software\\TortoiseGit\\MaxBugIDColWidth", 200)
	,m_nSearchIndex(0)
	,m_bNoDispUpdates(FALSE)
	, m_bThreadRunning(FALSE)
	, m_ShowFilter(FILTERSHOW_ALL)
	, m_bShowWC(false)
	, m_logEntries(&m_LogCache)
	, m_pFindDialog(nullptr)
	, m_dwDefaultColumns(0)
	, m_arShownList(&m_critSec)
	, m_hasWC(true)
	, m_bNoHightlightHead(FALSE)
	, m_ShowRefMask(LOGLIST_SHOWALLREFS)
	, m_bFullCommitMessageOnLogLine(false)
	, m_OldTopIndex(-1)
	, m_AsyncThreadRunning(FALSE)
	, m_AsyncThreadExit(FALSE)
	, m_DiffingThread(nullptr)
	, m_bIsCherryPick(false)
	, m_pMailmap(nullptr)
	, m_bShowBugtraqColumn(false)
	, m_IsIDReplaceAction(FALSE)
	, m_ShowMask(0)
	, m_LoadingThread(nullptr)
	, m_bExitThread(FALSE)
	, m_IsOldFirst(FALSE)
	, m_IsRebaseReplaceGraph(FALSE)
	, m_ContextMenuMask(0xFFFFFFFFFFFFFFFF)
	, m_bDragndropEnabled(false)
	, m_bDragging(FALSE)
	, m_nDropIndex(-1)
	, m_nDropMarkerLast(-1)
	, m_nDropMarkerLastHot(-1)
	, m_LogFilter(std::make_shared<CLogDlgFilter>())
	, m_HashMap(std::make_shared<MAP_HASH_NAME>())
{
	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)

	ResetWcRev(false);

	int cx = GetSystemMetrics(SM_CXSMICON);
	int cy = GetSystemMetrics(SM_CYSMICON);
	m_hModifiedIcon = CCommonAppUtils::LoadIconEx(IDI_ACTIONMODIFIED, cx, cy);
	m_hReplacedIcon = CCommonAppUtils::LoadIconEx(IDI_ACTIONREPLACED, cx, cy);
	m_hConflictedIcon = CCommonAppUtils::LoadIconEx(IDI_ACTIONCONFLICTED, cx, cy);
	m_hAddedIcon = CCommonAppUtils::LoadIconEx(IDI_ACTIONADDED, cx, cy);
	m_hDeletedIcon = CCommonAppUtils::LoadIconEx(IDI_ACTIONDELETED, cx, cy);
	m_hFetchIcon = CCommonAppUtils::LoadIconEx(IDI_ACTIONFETCHING, cx, cy);

	m_Filter.m_NumberOfLogsScale = CRegDWORD(L"Software\\TortoiseGit\\LogDialog\\NumberOfLogsScale", CFilterData::SHOW_NO_LIMIT);
	if (m_Filter.m_NumberOfLogsScale == CFilterData::SHOW_LAST_SEL_DATE)
	{
		CString key;
		key.Format(L"Software\\TortoiseGit\\History\\LogDlg_Limits\\%s\\FromDate", static_cast<LPCTSTR>(g_Git.m_CurrentDir));
		key.Replace(L':', L'_');
		CString lastSelFromDate = CRegString(key);
		if (lastSelFromDate.GetLength() == 10)
		{
			CTime time = CTime(_wtoi(static_cast<LPCTSTR>(lastSelFromDate.Left(4))), _wtoi(static_cast<LPCTSTR>(lastSelFromDate.Mid(5, 2))), _wtoi(static_cast<LPCTSTR>(lastSelFromDate.Mid(8, 2))), 0, 0, 0);
			m_Filter.m_From = static_cast<DWORD>(time.GetTime());
		}
	}
	m_Filter.m_NumberOfLogs = CRegDWORD(L"Software\\TortoiseGit\\LogDialog\\NumberOfLogs", 1);

	for (int i = 0; i < Lanes::COLORS_NUM; ++i)
	{
		m_LineColors[i] = m_Colors.GetColor((CColors::Colors)(CColors::BranchLine1+i));
	}
	// get short/long datetime setting from registry
	DWORD RegUseShortDateFormat = CRegDWORD(L"Software\\TortoiseGit\\LogDateFormat", TRUE);
	if ( RegUseShortDateFormat )
	{
		m_DateFormat = DATE_SHORTDATE;
	}
	else
	{
		m_DateFormat = DATE_LONGDATE;
	}
	// get relative time display setting from registry
	DWORD regRelativeTimes = CRegDWORD(L"Software\\TortoiseGit\\RelativeTimes", FALSE);
	m_bRelativeTimes = (regRelativeTimes != 0);

	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_PICK);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_SQUASH);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_EDIT);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_REBASE_SKIP);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_LOG);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_BLAME);
	m_ContextMenuMask &= ~GetContextMenuBit(ID_BLAMEPREVIOUS);

	m_ColumnRegKey = L"log";

	m_bTagsBranchesOnRightSide = !!CRegDWORD(L"Software\\TortoiseGit\\DrawTagsBranchesOnRightSide", FALSE);
	m_bSymbolizeRefNames = !!CRegDWORD(L"Software\\TortoiseGit\\SymbolizeRefNames", FALSE);
	m_bIncludeBoundaryCommits = !!CRegDWORD(L"Software\\TortoiseGit\\LogIncludeBoundaryCommits", FALSE);
	m_bFullCommitMessageOnLogLine = !!CRegDWORD(L"Software\\TortoiseGit\\FullCommitMessageOnLogLine", FALSE);

	m_LineWidth = max(1, static_cast<int>(CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\Graph\\LogLineWidth", 2)));
	m_NodeSize = max(1, static_cast<int>(CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\Graph\\LogNodeSize", 10)));

	if (CRegDWORD(L"Software\\TortoiseGit\\LogDialog\\UseMailmap", FALSE) == TRUE)
		git_read_mailmap(&m_pMailmap);

	m_AsyncDiffEvent = ::CreateEvent(nullptr, FALSE, TRUE, nullptr);
	StartAsyncDiffThread();
}

HWND CGitLogListBase::GetParentHWND()
{
	auto owner = GetSafeOwner();
	if (!owner)
		return GetSafeHwnd();
	return owner->GetSafeHwnd();
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
				g_Git.RefreshGitIndex();
				g_Git.GetWorkingTreeChanges(files, false, nullptr, true); // filtering is done in LogDlg.cpp
				auto& action = pRev->GetAction(this);
				action = 0;
				for (int j = 0; j < files.GetCount(); ++j)
					action |= files[j].m_Action;

				if (CString err; pRev->GetUnRevFiles().FillUnRev(CTGitPath::LOGACTIONS_UNVER, nullptr, &err))
				{
					MessageBox(L"Failed to get UnRev file list\n" + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
					InterlockedExchange(&m_AsyncThreadRunning, FALSE);
					return -1;
				}

				InterlockedExchange(&pRev->m_IsDiffFiles, TRUE);
				InterlockedExchange(&pRev->m_IsFull, TRUE);

				CString body = L"\n";
				body.AppendFormat(IDS_FILESCHANGES, files.GetCount());
				pRev->GetBody() = body;
				::PostMessage(m_hWnd, MSG_LOADED, 0, 0);
				this->GetParent()->PostMessage(WM_COMMAND, MSG_FETCHED_DIFF, 0);
			}

			if (!pRev->CheckAndDiff())
			{	// fetch change file list
				for (int i = GetTopIndex(); !m_AsyncThreadExit && i <= GetTopIndex() + GetCountPerPage(); ++i)
				{
					if (i < static_cast<int>(m_arShownList.size()))
					{
						GitRevLoglist* data = m_arShownList.SafeGetAt(i);
						if(data->m_CommitHash == pRev->m_CommitHash)
						{
							::PostMessage(m_hWnd, MSG_LOADED, i, 0);
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

	git_free_mailmap(m_pMailmap);

	SafeTerminateThread();
	SafeTerminateAsyncDiffThread();

	if(m_AsyncDiffEvent)
		CloseHandle(m_AsyncDiffEvent);
}


BEGIN_MESSAGE_MAP(CGitLogListBase, CHintCtrl<CResizableColumnsListCtrl<CListCtrl>>)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
	ON_REGISTERED_MESSAGE(m_ScrollToMessage, OnScrollToMessage)
	ON_REGISTERED_MESSAGE(m_ScrollToRef, OnScrollToRef)
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
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, &OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, &OnToolTipText)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
END_MESSAGE_MAP()

void CGitLogListBase::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	//if (m_nRowHeight>0)
		lpMeasureItemStruct->itemHeight = CDPIAware::Instance().ScaleY(50);
}

int CGitLogListBase:: OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	PreSubclassWindow();
	return __super::OnCreate(lpCreateStruct);
}

void CGitLogListBase::SetStyle()
{
	SetExtendedStyle(LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES | LVS_EX_FULLROWSELECT);
}

void CGitLogListBase::PreSubclassWindow()
{
	SetStyle();
	// load the icons for the action columns
//	m_Theme.Open(m_hWnd, L"ListView");
	SetWindowTheme(m_hWnd, L"Explorer", nullptr);
	__super::PreSubclassWindow();
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

	Init();

	// use the default font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	LOGFONT lf = { 0 };
	GetFont()->GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont.CreateFontIndirect(&lf);
	lf.lfWeight = FW_DONTCARE;
	lf.lfItalic = TRUE;
	m_FontItalics.CreateFontIndirect(&lf);
	lf.lfWeight = FW_BOLD;
	m_boldItalicsFont.CreateFontIndirect(&lf);

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

	auto iconItemBorder = CDPIAware::Instance().ScaleX(ICONITEMBORDER);
	auto columnWidth = CDPIAware::Instance().ScaleX(ICONITEMBORDER + 16 * 4);
	static int with[] =
	{
		columnWidth,
		columnWidth,
		columnWidth,
		columnWidth,
		2 * iconItemBorder + GetSystemMetrics(SM_CXSMICON) * 5,
		CDPIAware::Instance().ScaleX(LOGLIST_MESSAGE_MIN),
		columnWidth,
		columnWidth,
		columnWidth,
		columnWidth,
		columnWidth,
		columnWidth,
		columnWidth,
		columnWidth,
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
	m_ColumnManager.ReadSettings(m_dwDefaultColumns, hideColumns, m_ColumnRegKey + L"loglist", _countof(normal), with);
	m_ColumnManager.SetRightAlign(LOGLIST_ID);

	if (!(hideColumns & GIT_LOG_ACTIONS))
	{
		// Configure fake a imagelist for LogList with 1px width and height = GetSystemMetrics(SM_CYSMICON)
		// to set the minimum item height: we draw icons in the actions column, but on High-DPI the
		// display's font height may be less than small icon height.
		ASSERT((GetStyle() & LVS_SHAREIMAGELISTS) == 0);
		HIMAGELIST hImageList = ImageList_Create(1, GetSystemMetrics(SM_CYSMICON), 0, 1, 0);
		ListView_SetImageList(GetSafeHwnd(), hImageList, LVSIL_SMALL);
	}

	SetRedraw(true);
}

void CGitLogListBase::FillBackGround(HDC hdc, DWORD_PTR Index, CRect &rect)
{
	LVITEM rItem = { 0 };
	rItem.mask  = LVIF_STATE;
	rItem.iItem = static_cast<int>(Index);
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
	HPEN oldpen = static_cast<HPEN>(::SelectObject(hdc, nullPen));
	HBRUSH darkBrush = static_cast<HBRUSH>(::CreateSolidBrush(darkColor));
	HBRUSH oldbrush = static_cast<HBRUSH>(::SelectObject(hdc, darkBrush));
	::RoundRect(hdc, rt2.left, rt2.top, rt2.right, rt2.bottom, point.x, point.y);

	::SelectObject(hdc, brush);
	rt2.OffsetRect(-2, -2);
	::RoundRect(hdc, rt2.left, rt2.top, rt2.right, rt2.bottom, point.x, point.y);
	::SelectObject(hdc, oldbrush);
	::SelectObject(hdc, oldpen);
	::DeleteObject(nullPen);
	::DeleteObject(darkBrush);
}

void DrawUpstream(HDC hdc, CRect rect, COLORREF color, int bold)
{
	HPEN pen = ::CreatePen(PS_SOLID, bold, color);
	HPEN oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));
	::MoveToEx(hdc, rect.left + 2 + bold, rect.top + 2 - bold, nullptr);
	::LineTo(hdc, rect.left + 2 + bold, rect.bottom + 1 - bold);
	::MoveToEx(hdc, rect.left + 3, rect.top + 1, nullptr);
	::LineTo(hdc, rect.left, rect.top + 4);
	::MoveToEx(hdc, rect.left + 2 + bold, rect.top + 1, nullptr);
	::LineTo(hdc, rect.right + 1 + bold, rect.top + 4);
	::MoveToEx(hdc, rect.left + 1, rect.top + 2 + bold, nullptr);
	::LineTo(hdc, rect.right + 1 + bold, rect.top + 2 + bold);
	::SelectObject(hdc, oldpen);
	::DeleteObject(pen);
}

void CGitLogListBase::DrawTagBranchMessage(NMLVCUSTOMDRAW* pLVCD, CRect& rect, INT_PTR index, std::vector<REFLABEL>& refList)
{
	GitRevLoglist* data = m_arShownList.SafeGetAt(index);
	CRect rt=rect;
	LVITEM rItem = { 0 };
	rItem.mask  = LVIF_STATE;
	rItem.iItem = static_cast<int>(index);
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	CDC W_Dc;
	W_Dc.Attach(pLVCD->nmcd.hdc);

	HTHEME hTheme = nullptr;
	if (IsAppThemed())
		hTheme = OpenThemeData(m_hWnd, L"Explorer::ListView;ListView");

	SIZE oneSpaceSize;
	if (m_bTagsBranchesOnRightSide)
	{
		HFONT oldFont = static_cast<HFONT>(SelectObject(pLVCD->nmcd.hdc, GetStockObject(DEFAULT_GUI_FONT)));
		GetTextExtentPoint32(pLVCD->nmcd.hdc, L" ", 1, &oneSpaceSize);
		SelectObject(pLVCD->nmcd.hdc, oldFont);
		rt.left += oneSpaceSize.cx * 2;
	}
	else
	{
		GetTextExtentPoint32(pLVCD->nmcd.hdc, L" ", 1, &oneSpaceSize);
		DrawTagBranch(pLVCD->nmcd.hdc, W_Dc, hTheme, rect, rt, rItem, data, refList);
		rt.left += oneSpaceSize.cx;
	}

	CString msg = MessageDisplayStr(data);
	int action = data->GetRebaseAction();
	bool skip = !!(action & (LOGACTIONS_REBASE_DONE | LOGACTIONS_REBASE_SKIP));
	std::vector<CHARRANGE> ranges;
	auto filter = m_LogFilter;
	if ((filter->GetSelectedFilters() & (LOGFILTER_SUBJECT | (m_bFullCommitMessageOnLogLine ? LOGFILTER_MESSAGES : 0))) && filter->IsFilterActive())
		filter->GetMatchRanges(ranges, msg, 0);
	if (hTheme)
	{
		int txtState = LISS_NORMAL;
		if (rItem.state & LVIS_SELECTED)
			txtState = LISS_SELECTED;

		if (!ranges.empty())
			DrawListItemWithMatchesRect(pLVCD, ranges, rt, msg, m_Colors, hTheme, txtState);
		else
		{
			DTTOPTS opts = { 0 };
			opts.dwSize = sizeof(opts);
			opts.crText = skip ? RGB(128, 128, 128) : ::GetSysColor(COLOR_WINDOWTEXT);
			opts.dwFlags = DTT_TEXTCOLOR;
			DrawThemeTextEx(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, msg, -1, DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS, &rt, &opts);
		}
	}
	else
	{
		if ((rItem.state & LVIS_SELECTED) && ::GetFocus() == m_hWnd)
			pLVCD->clrText = skip ? RGB(128, 128, 128) : ::GetSysColor(COLOR_HIGHLIGHTTEXT);
		else
			pLVCD->clrText = skip ? RGB(128, 128, 128) : ::GetSysColor(COLOR_WINDOWTEXT);
		if (!ranges.empty())
			DrawListItemWithMatchesRect(pLVCD, ranges, rt, msg, m_Colors);
		else
		{
			COLORREF clrOld = ::SetTextColor(pLVCD->nmcd.hdc, pLVCD->clrText);
			::DrawText(pLVCD->nmcd.hdc, msg, msg.GetLength(), &rt, DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);
			::SetTextColor(pLVCD->nmcd.hdc, clrOld);
		}
	}

	if (m_bTagsBranchesOnRightSide)
	{
		SIZE size;
		GetTextExtentPoint32(pLVCD->nmcd.hdc, msg, msg.GetLength(), &size);

		rt.left += oneSpaceSize.cx + size.cx;

		DrawTagBranch(pLVCD->nmcd.hdc, W_Dc, hTheme, rect, rt, rItem, data, refList);
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
				rt.right += 5;
				textRect.OffsetRect(5, 0);
			}

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
				HPEN oldpen = static_cast<HPEN>(SelectObject(hdc, pen = ::CreatePen(PS_SOLID, 2, m_Colors.Lighten(colRef, 50))));
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
				newRect.SetRect(rt.left + 2, rt.top + 4, rt.left + 6, rt.bottom - 4);
				DrawUpstream(hdc, newRect, color, bold);
			}

			if (!refList[i].fullName.IsEmpty())
				m_RefLabelPosMap[refList[i].fullName] = rt;

			rt.left = rt.right + 1;
		}
		if (brush)
			::DeleteObject(brush);
	}
	rt.right = rect.right;
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


		Gdiplus::Pen mypen(&gradient, static_cast<Gdiplus::REAL>(m_LineWidth));
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


		Gdiplus::Pen mypen(&gradient, static_cast<Gdiplus::REAL>(m_LineWidth));
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

		Gdiplus::Pen mypen(&gradient, static_cast<Gdiplus::REAL>(m_LineWidth));

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
	HPEN oldpen = static_cast<HPEN>(::SelectObject(hdc, pen));

	Gdiplus::Pen myPen(GetGdiColor(col), static_cast<Gdiplus::REAL>(m_LineWidth));

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
	case Lanes::MERGE_FORK_L_INITIAL:
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
	case Lanes::MERGE_FORK_L_INITIAL:
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
	HBRUSH oldbrush = static_cast<HBRUSH>(::SelectObject(hdc, brush));

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
	case Lanes::MERGE_FORK_L_INITIAL:
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
	rItem.iItem = static_cast<int>(index);
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
			// Store the color back in the NMLVCUSTOMDRAW struct.
			pLVCD->clrText = crText;
			return;
		}
		break;

	case CDDS_ITEMPREPAINT | CDDS_ITEM | CDDS_SUBITEM:
	{
		switch (pLVCD->iSubItem)
		{
		case LOGLIST_GRAPH:
			if ((m_ShowFilter & FILTERSHOW_MERGEPOINTS) && !m_LogFilter->IsFilterActive())
			{
				if (m_arShownList.size() > pLVCD->nmcd.dwItemSpec && !this->m_IsRebaseReplaceGraph)
				{
					CRect rect;
					GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_LABEL, rect);

					//TRACE(L"A Graphic left %d right %d\r\n", rect.left, rect.right);
					FillBackGround(pLVCD->nmcd.hdc, pLVCD->nmcd.dwItemSpec,rect);

					GitRevLoglist* data = m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
					if( !data ->m_CommitHash.IsEmpty())
						DrawGraph(pLVCD->nmcd.hdc,rect,pLVCD->nmcd.dwItemSpec);

					*pResult = CDRF_SKIPDEFAULT;
					return;
				}
			}
			break;

		case LOGLIST_MESSAGE:
			// If the top index of list is changed, the position map of reference label is outdated.
			if (m_OldTopIndex != GetTopIndex())
			{
				m_OldTopIndex = GetTopIndex();
				m_RefLabelPosMap.clear();
			}

			if (m_arShownList.size() > pLVCD->nmcd.dwItemSpec)
			{
				GitRevLoglist* data = m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);

				auto hashMapSharedPtr = m_HashMap;
				auto hashMap = *hashMapSharedPtr.get();
				if ((hashMap.find(data->m_CommitHash) != hashMap.cend() || (!m_superProjectHash.IsEmpty() && data->m_CommitHash == m_superProjectHash)) && !(data->GetRebaseAction() & LOGACTIONS_REBASE_DONE))
				{
					CRect rect;
					GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_BOUNDS, rect);

					// BEGIN: extended redraw, HACK for issue #1618 and #2014
					// not in FillBackGround method, because this only affected the message subitem
					if (0 != pLVCD->iStateId) // don't know why, but this helps against loosing the focus rect
						return;

					int index = static_cast<int>(pLVCD->nmcd.dwItemSpec);
					int state = GetItemState(index, LVIS_SELECTED);
					int txtState = LISS_NORMAL;
					if (IsAppThemed() && GetHotItem() == static_cast<int>(index))
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
					{
						hTheme = OpenThemeData(m_hWnd, L"Explorer::ListView;ListView");

						// make sure the column separator/border is not overpainted
						int borderWidth = 0;
						GetThemeMetric(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, LISS_NORMAL, TMT_BORDERSIZE, &borderWidth);
						InflateRect(&rect, -(2 * borderWidth), 0);
					}

					if (hTheme && IsThemeBackgroundPartiallyTransparent(hTheme, LVP_LISTDETAIL, txtState))
						DrawThemeParentBackground(m_hWnd, pLVCD->nmcd.hdc, &rect);
					else
					{
						HBRUSH brush = ::CreateSolidBrush(pLVCD->clrTextBk);
						::FillRect(pLVCD->nmcd.hdc, rect, brush);
						::DeleteObject(brush);
					}
					if (hTheme && txtState != LISS_NORMAL)
					{
						CRect rt;
						// get rect of whole line
						GetItemRect(index, rt, LVIR_BOUNDS);
						CRect rect2 = rect;

						// calculate background for rect of whole line, but limit redrawing to SubItem rect
						DrawThemeBackground(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, rt, rect2);

						CloseThemeData(hTheme);
					}
					// END: extended redraw

					FillBackGround(pLVCD->nmcd.hdc, pLVCD->nmcd.dwItemSpec, rect);

					std::vector<REFLABEL> refsToShow;
					STRING_VECTOR remoteTrackingList;
					std::vector<CString>::const_iterator refListIt;
					std::vector<CString>::const_iterator refListItEnd;
					auto commitRefsIt = hashMap.find(data->m_CommitHash);
					if (commitRefsIt != hashMap.cend())
					{
						refListIt = (*commitRefsIt).second.cbegin();
						refListItEnd = (*commitRefsIt).second.cend();
					}
					for (; refListIt != refListItEnd; ++refListIt)
					{
						REFLABEL refLabel;
						refLabel.color = RGB(255, 255, 255);
						refLabel.singleRemote = false;
						refLabel.hasTracking = false;
						refLabel.sameName = false;
						refLabel.name = CGit::GetShortName(*refListIt, &refLabel.refType);
						refLabel.fullName = *refListIt;

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
								defaultUpstream.Format(L"refs/remotes/%s/%s", static_cast<LPCTSTR>(pullRemote), static_cast<LPCTSTR>(pullBranch));
								refLabel.hasTracking = true;
								if (m_ShowRefMask & LOGLIST_SHOWREMOTEBRANCHES)
								{
									bool found = false;
									for (auto it2 = refListIt + 1; it2 != refListItEnd; ++it2)
									{
										if (*it2 == defaultUpstream)
										{
											found = true;
											break;
										}
									}

									if (found)
									{
										bool sameName = pullBranch == refLabel.name;
										refsToShow.push_back(refLabel);
										CGit::GetShortName(defaultUpstream, refLabel.name, L"refs/remotes/");
										refLabel.color = m_Colors.GetColor(CColors::RemoteBranch);
										if (m_bSymbolizeRefNames)
										{
											if (!m_SingleRemote.IsEmpty() && m_SingleRemote == pullRemote)
											{
												refLabel.simplifiedName = L'/';
												if (sameName)
													refLabel.simplifiedName += L'≡';
												else
													refLabel.simplifiedName += pullBranch;
												refLabel.singleRemote = true;
											}
											else if (sameName)
												refLabel.simplifiedName = pullRemote + L"/≡";
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
								if (remoteTrackingList[j] == *refListIt)
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
								if (!m_SingleRemote.IsEmpty() && CStringUtils::StartsWith(refLabel.name, m_SingleRemote + L"/"))
								{
									refLabel.simplifiedName = L'/' + refLabel.name.Mid(m_SingleRemote.GetLength() + 1);
									refLabel.singleRemote = true;
								}
							}
							break;
						}
						case CGit::REF_TYPE::ANNOTATED_TAG: // fallthrough
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

						case CGit::REF_TYPE::BISECT_GOOD: // fallthrough
						case CGit::REF_TYPE::BISECT_BAD: // fallthrough
						case CGit::REF_TYPE::BISECT_SKIP:
							if (!(m_ShowRefMask & LOGLIST_SHOWBISECT))
								continue;
							refLabel.color = (refLabel.refType == CGit::REF_TYPE::BISECT_GOOD) ? m_Colors.GetColor(CColors::BisectGood) : ((refLabel.refType == CGit::REF_TYPE::BISECT_SKIP) ? m_Colors.GetColor(CColors::BisectSkip) : m_Colors.GetColor(CColors::BisectBad));
							break;

						case CGit::REF_TYPE::NOTES:
							if (!(m_ShowRefMask & LOGLIST_SHOWOTHERREFS))
								continue;
							refLabel.color = m_Colors.GetColor(CColors::NoteNode);
							break;

						default:
							if (!(m_ShowRefMask & LOGLIST_SHOWOTHERREFS))
								continue;
							refLabel.color = m_Colors.GetColor(CColors::OtherRef);
							break;
						}
						refsToShow.push_back(refLabel);
					}
					if (!m_superProjectHash.IsEmpty() && data->m_CommitHash == m_superProjectHash)
					{
						REFLABEL refLabel;
						refLabel.color = RGB(246, 153, 253);
						refLabel.singleRemote = false;
						refLabel.hasTracking = false;
						refLabel.sameName = false;
						refLabel.name = L"super-project-pointer";
						refLabel.fullName = "";
						refsToShow.push_back(refLabel);
					}

					if (refsToShow.empty())
					{
						*pResult = CDRF_DODEFAULT;
						return;
					}

					DrawTagBranchMessage(pLVCD, rect, pLVCD->nmcd.dwItemSpec, refsToShow);

					*pResult = CDRF_SKIPDEFAULT;
					return;
				}
				else if (DrawListItemWithMatchesIfEnabled(m_LogFilter, LOGFILTER_SUBJECT | LOGFILTER_MESSAGES, pLVCD, pResult))
					return;
			}
			break;

		case LOGLIST_ACTION:
		{
			if (m_IsIDReplaceAction || !m_ColumnManager.IsVisible(LOGLIST_ACTION))
			{
				*pResult = CDRF_DODEFAULT;
				return;
			}
			*pResult = CDRF_DODEFAULT;

			if (m_arShownList.size() <= pLVCD->nmcd.dwItemSpec)
				return;

			int nIcons = 0;
			int iconwidth = ::GetSystemMetrics(SM_CXSMICON);
			int iconheight = ::GetSystemMetrics(SM_CYSMICON);

			GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
			CRect rect;
			GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_BOUNDS, rect);
			//TRACE(L"Action left %d right %d\r\n", rect.left, rect.right);
			// Get the selected state of the
			// item being drawn.

			CMemDC myDC(*CDC::FromHandle(pLVCD->nmcd.hdc), rect);
			BitBlt(myDC.GetDC(), rect.left, rect.top, rect.Width(), rect.Height(), pLVCD->nmcd.hdc, rect.left, rect.top, SRCCOPY);

			// Fill the background if necessary
			FillBackGround(myDC.GetDC(), pLVCD->nmcd.dwItemSpec, rect);

			// Draw the icon(s) into the compatible DC
			int action = pLogEntry->GetAction(this);
			auto iconItemBorder = CDPIAware::Instance().ScaleX(ICONITEMBORDER);
			if (!pLogEntry->m_IsDiffFiles)
			{
				::DrawIconEx(myDC.GetDC(), rect.left + iconItemBorder, rect.top, m_hFetchIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
				*pResult = CDRF_SKIPDEFAULT;
				return;
			}

			if (action & CTGitPath::LOGACTIONS_MODIFIED)
				::DrawIconEx(myDC.GetDC(), rect.left + iconItemBorder, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
			++nIcons;

			if (action & (CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_COPY))
				::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconwidth + iconItemBorder, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
			++nIcons;

			if (action & CTGitPath::LOGACTIONS_DELETED)
				::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconwidth + iconItemBorder, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
			++nIcons;

			if (action & CTGitPath::LOGACTIONS_REPLACED)
				::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconwidth + iconItemBorder, rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
			++nIcons;

			if (action & CTGitPath::LOGACTIONS_UNMERGED)
				::DrawIconEx(myDC.GetDC(), rect.left + nIcons * iconwidth + iconItemBorder, rect.top, m_hConflictedIcon, iconwidth, iconheight, 0, nullptr, DI_NORMAL);
			++nIcons;

			*pResult = CDRF_SKIPDEFAULT;
			return;
		}

		case LOGLIST_HASH:
			if (DrawListItemWithMatchesIfEnabled(m_LogFilter, LOGFILTER_REVS, pLVCD, pResult))
				return;
			break;

		case LOGLIST_AUTHOR:
		case LOGLIST_COMMIT_NAME:
			if (DrawListItemWithMatchesIfEnabled(m_LogFilter, LOGFILTER_AUTHORS, pLVCD, pResult))
				return;
			break;

		case LOGLIST_EMAIL:
		case LOGLIST_COMMIT_EMAIL:
			if (DrawListItemWithMatchesIfEnabled(m_LogFilter, LOGFILTER_EMAILS, pLVCD, pResult))
				return;
			break;

		case LOGLIST_BUG:
			if (DrawListItemWithMatchesIfEnabled(m_LogFilter, LOGFILTER_BUGID, pLVCD, pResult))
				return;
			break;
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
		const std::wsregex_iterator end;
		std::wstring s = msg;
		std::wregex regex1(L"^\\s*git-svn-id:\\s+(.*)\\@(\\d+)\\s([a-f\\d\\-]+)$");
		for (std::wsregex_iterator it(s.cbegin(), s.cend(), regex1); it != end; ++it)
		{
			const std::wsmatch match = *it;
			if (match.size() == 4)
			{
				ATLTRACE(L"matched rev: %s\n", std::wstring(match[2]).c_str());
				return std::wstring(match[2]).c_str();
			}
		}
		std::wregex regex2(L"^\\s*git-svn-id:\\s(\\d+)\\@([a-f\\d\\-]+)$");
		for (std::wsregex_iterator it(s.cbegin(), s.cend(), regex2); it != end; ++it)
		{
			const std::wsmatch match = *it;
			if (match.size() == 3)
			{
				ATLTRACE(L"matched rev: %s\n", std::wstring(match[1]).c_str());
				return std::wstring(match[1]).c_str();
			}
		}
	}
	catch (std::exception&) {}

	return L"";
}

CString CGitLogListBase::MessageDisplayStr(GitRev* pLogEntry)
{
	if (!m_bFullCommitMessageOnLogLine || pLogEntry->GetBody().IsEmpty())
		return pLogEntry->GetSubject();

	CString txt(pLogEntry->GetSubject());
	txt += L' ';
	txt += pLogEntry->GetBody();

	// Deal with CRLF
	txt.Replace(L'\n', L' ');
	txt.Replace(L'\r', L' ');

	return txt;
}

// CGitLogListBase message handlers

static const char* GetMailmapMapping(GIT_MAILMAP mailmap, const CString& email, const CString& name, bool returnEmail)
{
	struct payload_struct { const CString* name; const char* authorName; };
	payload_struct payload = { &name, nullptr };//check
	const char* author1 = nullptr;
	const char* email1 = nullptr;
	git_lookup_mailmap(mailmap, &email1, &author1, CUnicodeUtils::GetUTF8(email), &payload,
		[](void* payload) -> const char* { return reinterpret_cast<payload_struct*>(payload)->authorName = _strdup(CUnicodeUtils::GetUTF8(*reinterpret_cast<payload_struct*>(payload)->name)); });
	free((void *)payload.authorName);
	if (returnEmail)
		return email1;
	return author1;
}

static void CopyMailmapProcessedData(GIT_MAILMAP mailmap, LV_ITEM* pItem, const CString& email, const CString& name, bool returnEmail)
{
	if (mailmap)
	{
		const char* translated = GetMailmapMapping(mailmap, email, name, returnEmail);
		if (translated)
		{
			lstrcpyn(pItem->pszText, CUnicodeUtils::GetUnicode(translated), pItem->cchTextMax - 1);
			return;
		}
	}
	if (returnEmail)
		lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(email), pItem->cchTextMax - 1);
	else
		lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(name), pItem->cchTextMax - 1);
}

void CGitLogListBase::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	// Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	// Do the list need text information?
	if (!(pItem->mask & LVIF_TEXT))
		return;

	// By default, clear text buffer.
	lstrcpyn(pItem->pszText, L"", pItem->cchTextMax - 1);

	bool bOutOfRange = pItem->iItem >= static_cast<int>(m_arShownList.size());

	*pResult = 0;
	if (m_bNoDispUpdates || bOutOfRange)
		return;

	// Which item number?
	GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(pItem->iItem);

	CString temp;
	if(m_IsOldFirst)
		temp.Format(L"%d", pItem->iItem + 1);
	else
		temp.Format(L"%zu", m_arShownList.size() - pItem->iItem);

	if (!pLogEntry)
		return;

	// Which column?
	switch (pItem->iSubItem)
	{
	case LOGLIST_GRAPH:	//Graphic
		break;
	case LOGLIST_REBASE:
		if (m_IsRebaseReplaceGraph)
			lstrcpyn(pItem->pszText, GetRebaseActionName(pLogEntry->GetRebaseAction() & LOGACTIONS_REBASE_MODE_MASK), pItem->cchTextMax - 1);
	break;
	case LOGLIST_ACTION: //action -- no text in the column
		break;
	case LOGLIST_HASH:
		lstrcpyn(pItem->pszText, pLogEntry->m_CommitHash.ToString(), pItem->cchTextMax - 1);
		break;
	case LOGLIST_ID:
		if (this->m_IsIDReplaceAction)
			lstrcpyn(pItem->pszText, temp, pItem->cchTextMax - 1);
		break;
	case LOGLIST_MESSAGE: //Message
		lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(MessageDisplayStr(pLogEntry)), pItem->cchTextMax - 1);
		break;
	case LOGLIST_AUTHOR: //Author
		CopyMailmapProcessedData(m_pMailmap, pItem, pLogEntry->GetAuthorEmail(), pLogEntry->GetAuthorName(), false);
		break;
	case LOGLIST_DATE: //Date
		if (!pLogEntry->m_CommitHash.IsEmpty())
			lstrcpyn(pItem->pszText,
				CLoglistUtils::FormatDateAndTime(pLogEntry->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes),
				pItem->cchTextMax - 1);
		break;

	case LOGLIST_EMAIL:
		CopyMailmapProcessedData(m_pMailmap, pItem, pLogEntry->GetAuthorEmail(), pLogEntry->GetAuthorName(), true);
		break;

	case LOGLIST_COMMIT_NAME: //Commit
		CopyMailmapProcessedData(m_pMailmap, pItem, pLogEntry->GetCommitterEmail(), pLogEntry->GetCommitterName(), false);
		break;

	case LOGLIST_COMMIT_EMAIL: //Commit Email
		CopyMailmapProcessedData(m_pMailmap, pItem, pLogEntry->GetCommitterEmail(), pLogEntry->GetCommitterName(), true);
		break;

	case LOGLIST_COMMIT_DATE: //Commit Date
		if (!pLogEntry->m_CommitHash.IsEmpty())
			lstrcpyn(pItem->pszText,
				CLoglistUtils::FormatDateAndTime(pLogEntry->GetCommitterDate(), m_DateFormat, true, m_bRelativeTimes),
				pItem->cchTextMax - 1);
		break;
	case LOGLIST_BUG: //Bug ID
		lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(this->m_ProjectProperties.FindBugID(pLogEntry->GetSubjectBody())), pItem->cchTextMax - 1);
		break;
	case LOGLIST_SVNREV: //SVN revision
		lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(FindSVNRev(pLogEntry->GetSubjectBody())), pItem->cchTextMax - 1);
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
	auto hashMap = m_HashMap;
	auto refList = hashMap.get()->find(pSelLogEntry->m_CommitHash);
	if (refList == hashMap.get()->cend())
		return false;
	return find_if((*refList).second, [](const auto& ref) { return ref == L"refs/stash"; }) != (*refList).second.cend();
}

bool CGitLogListBase::IsBisect(const GitRev * pSelLogEntry)
{
	auto hashMap = m_HashMap;
	auto refList = hashMap->find(pSelLogEntry->m_CommitHash);
	if (refList == hashMap->cend())
		return false;
	return find_if((*refList).second, [](const auto& ref) { return CStringUtils::StartsWith(ref, L"refs/bisect/"); }) != (*refList).second.cend();
}

void CGitLogListBase::GetParentHashes(GitRev *pRev, GIT_REV_LIST &parentHash)
{
	if (pRev->m_ParentHash.empty())
	{
		if (pRev->GetParentFromHash(pRev->m_CommitHash))
			MessageBox(pRev->GetLastErr(), L"TortoiseGit", MB_ICONERROR);
	}
	parentHash = pRev->m_ParentHash;
}

void CGitLogListBase::OnContextMenu(CWnd* pWnd, CPoint point)
{
	__super::OnContextMenu(pWnd, point);

	if (pWnd != this)
		return;

	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return; // nothing selected, nothing to do with a context menu

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

	bool showExtendedMenu = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

	POSITION pos = GetFirstSelectedItemPosition();
	int FirstSelect = GetNextSelectedItem(pos);
	if (FirstSelect < 0)
		return;

	GitRevLoglist* pSelLogEntry = m_arShownList.SafeGetAt(FirstSelect);
	if (pSelLogEntry == nullptr)
		return;

	int LastSelect = -1;
	UINT selectedCount = 1;
	while (pos)
	{
		LastSelect = GetNextSelectedItem(pos);
		++selectedCount;
	}

	ASSERT(GetSelectedCount() == selectedCount);

	//entry is selected, now show the popup menu
	CIconMenu popup;
	CIconMenu subbranchmenu, submenu, gnudiffmenu, diffmenu, blamemenu, revertmenu;

	if (popup.CreatePopupMenu())
	{
		CGitHash headHash;
		if (g_Git.GetHash(headHash, L"HEAD"))
		{
			MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
			return;
		}
		auto hashMapSharedPtr = m_HashMap;
		auto hashMap = *hashMapSharedPtr.get();
		bool isHeadCommit = (pSelLogEntry->m_CommitHash == headHash);
		CString currentBranch = L"refs/heads/" + g_Git.GetCurrentBranch();
		CTGitPath workingTree(g_Git.m_CurrentDir);
		bool isMergeActive = workingTree.IsMergeActive();
		bool isBisectActive = workingTree.IsBisectActive();
		bool isStash = IsOnStash(FirstSelect);
		GIT_REV_LIST parentHash;
		GetParentHashes(pSelLogEntry, parentHash);
		STRING_VECTOR parentInfo;
		for (size_t i = 0; i < parentHash.size(); ++i)
		{
			CString str;
			str.Format(IDS_PARENT, i + 1);
			GitRev rev;
			if (rev.GetCommit(parentHash[i].ToString()) == 0)
			{
				CString commitTitle = rev.GetSubject();
				if (commitTitle.GetLength() > 20)
				{
					commitTitle.Truncate(20);
					commitTitle += L"...";
				}
				str.AppendFormat(L": \"%s\" (%s)", static_cast<LPCTSTR>(commitTitle), static_cast<LPCTSTR>(parentHash[i].ToString(g_Git.GetShortHASHLength())));
			}
			else
				str.AppendFormat(L" (%s)", static_cast<LPCTSTR>(parentHash[i].ToString(g_Git.GetShortHASHLength())));
			parentInfo.push_back(str);
		}

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

		if (selectedCount == 1)
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
						for (size_t i = 0; i < parentInfo.size(); ++i)
						{
							blamemenu.AppendMenuIcon(ID_BLAMEPREVIOUS + ((i + 1) << 16), parentInfo[i]);
						}
						requiresSeparator = true;
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF1) && m_hasWC) // compare with WC, unified
				{
					if (parentHash.size() == 1)
					{
						popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
						requiresSeparator = true;
					}
					else if (parentHash.size() > 1)
					{
						gnudiffmenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_GNUDIFF1,IDS_LOG_POPUP_GNUDIFF_PARENT, IDI_DIFF, gnudiffmenu.m_hMenu);

						gnudiffmenu.AppendMenuIcon(static_cast<UINT_PTR>(ID_GNUDIFF1 + (0xFFFF << 16)), CString(MAKEINTRESOURCE(IDS_ALLPARENTS)));
						gnudiffmenu.AppendMenuIcon(static_cast<UINT_PTR>(ID_GNUDIFF1 + (0xFFFE << 16)), CString(MAKEINTRESOURCE(IDS_ONLYMERGEDFILES)));
						gnudiffmenu.AppendMenuIcon(static_cast<UINT_PTR>(ID_GNUDIFF1 + (0xFFFD << 16)), CString(MAKEINTRESOURCE(IDS_DIFFWITHMERGE)));

						for (size_t i = 0; i < parentInfo.size(); ++i)
						{
							gnudiffmenu.AppendMenuIcon(ID_GNUDIFF1 + ((i + 1) << 16), parentInfo[i]);
						}
						requiresSeparator = true;
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_COMPAREWITHPREVIOUS))
				{
					if (parentHash.size() == 1)
					{
						popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
						if (CRegDWORD(L"Software\\TortoiseGit\\DiffByDoubleClickInLog", FALSE) && m_ColumnRegKey != L"reflog")
							popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
						requiresSeparator = true;
					}
					else if (parentHash.size() > 1)
					{
						diffmenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF, diffmenu.m_hMenu);
						for (size_t i = 0; i < parentInfo.size(); ++i)
						{
							diffmenu.AppendMenuIcon(ID_COMPAREWITHPREVIOUS + ((i + 1) << 16), parentInfo[i]);
							if (i == 0 && CRegDWORD(L"Software\\TortoiseGit\\DiffByDoubleClickInLog", FALSE) && m_ColumnRegKey != L"reflog")
							{
								popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
								diffmenu.SetDefaultItem(static_cast<UINT>(ID_COMPAREWITHPREVIOUS + ((i + 1) << 16)), FALSE);
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
						popup.AppendMenuIcon(ID_STASH_SAVE, IDS_MENUSTASHSAVE, IDI_SHELVE);
						requiresSeparator = true;
					}
				}

				if ((pSelLogEntry->m_CommitHash.IsEmpty() || isStash) && workingTree.HasStashDir())
				{
					if (m_ContextMenuMask&GetContextMenuBit(ID_STASH_POP))
					{
						popup.AppendMenuIcon(ID_STASH_POP, IDS_MENUSTASHPOP, IDI_UNSHELVE);
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

				if (isBisectActive)
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
					if (m_ContextMenuMask&GetContextMenuBit(ID_BISECTSKIP) && !IsBisect(pFirstEntry))
					{
						popup.AppendMenuIcon(ID_BISECTSKIP, IDS_MENUBISECTSKIP, IDI_BISECT);
						requiresSeparator = true;
					}
				}

				if (pSelLogEntry->m_CommitHash.IsEmpty() && isBisectActive)
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
						popup.AppendMenuIcon(ID_FETCH, IDS_MENUFETCH, IDI_UPDATE);

					if ((m_ContextMenuMask & GetContextMenuBit(ID_SUBMODULE_UPDATE)) && workingTree.HasSubmodules())
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
				if (m_ContextMenuMask & GetContextMenuBit(ID_LOG))
				{
					popup.AppendMenuIcon(ID_LOG, IDS_LOG_POPUP_LOG, IDI_LOG);
					if (m_ColumnRegKey == L"reflog")
						popup.SetDefaultItem(ID_LOG, FALSE);
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_REPOBROWSE))
					popup.AppendMenuIcon(ID_REPOBROWSE, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);

				str.Format(IDS_LOG_POPUP_MERGEREV, static_cast<LPCTSTR>(g_Git.GetCurrentBranch()));

				if (m_ContextMenuMask&GetContextMenuBit(ID_MERGEREV) && !isHeadCommit && m_hasWC && !isMergeActive && !isStash)
				{
					popup.AppendMenuIcon(ID_MERGEREV, str, IDI_MERGE);

					size_t index = static_cast<size_t>(-1);
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, hashMap, nullptr, &index))
						popup.SetMenuItemData(ID_MERGEREV, reinterpret_cast<LONG_PTR>(&hashMap[pSelLogEntry->m_CommitHash][index]));
				}

				str.Format(IDS_RESET_TO_THIS_FORMAT, static_cast<LPCTSTR>(g_Git.GetCurrentBranch()));

				if (m_ContextMenuMask&GetContextMenuBit(ID_RESET) && m_hasWC && !isStash)
					popup.AppendMenuIcon(ID_RESET, str, IDI_RESET);


				// Add Switch Branch express Menu
				if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.end()
					&& (m_ContextMenuMask&GetContextMenuBit(ID_SWITCHBRANCH) && m_hasWC && !isStash)
					)
				{
					std::vector<const CString*> branchs;
					auto addCheck = [&](const CString& ref)
					{
						if (!CStringUtils::StartsWith(ref, L"refs/heads/") || ref == currentBranch)
							return;
						branchs.push_back(&ref);
					};
					size_t index = static_cast<size_t>(-1);
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, hashMap, nullptr, &index))
						addCheck(hashMap[pSelLogEntry->m_CommitHash][index]);
					else
						for_each(hashMap[pSelLogEntry->m_CommitHash], addCheck);

					CString str2;
					str2.LoadString(IDS_SWITCH_BRANCH);

					if(branchs.size() == 1)
					{
						str2 += L' ';
						str2 += L'"' + branchs[0]->Mid(static_cast<int>(wcslen(L"refs/heads/"))) + L'"';
						popup.AppendMenuIcon(ID_SWITCHBRANCH, str2, IDI_SWITCH);

						popup.SetMenuItemData(ID_SWITCHBRANCH, reinterpret_cast<LONG_PTR>(branchs[0]));

					}
					else if(branchs.size() > 1)
					{
						subbranchmenu.CreatePopupMenu();
						for (size_t i = 0 ; i < branchs.size(); ++i)
						{
							if (*branchs[i] != currentBranch)
							{
								subbranchmenu.AppendMenuIcon(ID_SWITCHBRANCH + (i << 16), branchs[i]->Mid(static_cast<int>(wcslen(L"refs/heads/"))));
								subbranchmenu.SetMenuItemData(ID_SWITCHBRANCH+(i<<16), reinterpret_cast<LONG_PTR>(branchs[i]));
							}
						}

						popup.AppendMenuIcon(ID_SWITCHBRANCH, str2, IDI_SWITCH, subbranchmenu.m_hMenu);
					}
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_SWITCHTOREV) && !isHeadCommit && m_hasWC && !isStash)
				{
					popup.AppendMenuIcon(ID_SWITCHTOREV, IDS_SWITCH_TO_THIS, IDI_SWITCH);
					size_t index = static_cast<size_t>(-1);
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, hashMap, nullptr, &index))
						popup.SetMenuItemData(ID_SWITCHTOREV, reinterpret_cast<LONG_PTR>(&hashMap[pSelLogEntry->m_CommitHash][index]));
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_CREATE_BRANCH) && !isStash)
				{
					popup.AppendMenuIcon(ID_CREATE_BRANCH, IDS_CREATE_BRANCH_AT_THIS, IDI_COPY);

					size_t index = static_cast<size_t>(-1);
					CGit::REF_TYPE type = CGit::REF_TYPE::REMOTE_BRANCH;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, hashMap, nullptr, &index))
						popup.SetMenuItemData(ID_CREATE_BRANCH, reinterpret_cast<LONG_PTR>(&hashMap[pSelLogEntry->m_CommitHash][index]));
				}

				if (m_ContextMenuMask&GetContextMenuBit(ID_CREATE_TAG) && !isStash)
					popup.AppendMenuIcon(ID_CREATE_TAG,IDS_CREATE_TAG_AT_THIS , IDI_TAG);

				str.Format(IDS_REBASE_THIS_FORMAT, static_cast<LPCTSTR>(g_Git.GetCurrentBranch()));

				if (pSelLogEntry->m_CommitHash != headHash && m_hasWC && !isMergeActive && !isStash)
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

						for (size_t i = 0; i < parentInfo.size(); ++i)
						{
							revertmenu.AppendMenuIcon(ID_REVERTREV + ((i + 1) << 16), parentInfo[i]);
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
			if (selectedCount == 1 && CStringUtils::StartsWith(pSelLogEntry->m_Ref, L"refs/stash"))
				popup.AppendMenuIcon(ID_REFLOG_STASH_APPLY, IDS_MENUSTASHAPPLY, IDI_UNSHELVE);
			if (selectedCount <= 2)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (selectedCount >= 2)
		{
			bool bAddSeparator = false;
			if ((selectedCount == 2) || IsSelectionContinuous())
			{
				if (m_ContextMenuMask&GetContextMenuBit(ID_COMPARETWO)) // compare two revisions
				{
					popup.AppendMenuIcon(ID_COMPARETWO, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
					bAddSeparator = true;
				}
			}

			if (selectedCount == 2)
			{
				if (m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF2) && m_hasWC) // compare two revisions, unified
				{
					popup.AppendMenuIcon(ID_GNUDIFF2, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
					bAddSeparator = true;
				}

				if (!pSelLogEntry->m_CommitHash.IsEmpty())
				{
					CString firstSelHash = pSelLogEntry->m_CommitHash.ToString(g_Git.GetShortHASHLength());
					GitRevLoglist* pLastEntry = m_arShownList.SafeGetAt(LastSelect);
					CString lastSelHash = pLastEntry->m_CommitHash.ToString(g_Git.GetShortHASHLength());
					CString menu;
					menu.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(lastSelHash + L".." + firstSelHash));
					popup.AppendMenuIcon(ID_LOG_VIEWRANGE, menu, IDI_LOG);
					menu.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(lastSelHash + L"..." + firstSelHash));
					popup.AppendMenuIcon(ID_LOG_VIEWRANGE_REACHABLEFROMONLYONE, menu, IDI_LOG);
					bAddSeparator = true;
				}
			}

			if ((m_ContextMenuMask & GetContextMenuBit(ID_COMPARETWOCOMMITCHANGES)) && selectedCount == 2 && !IsSelectionContinuous())
			{
				bAddSeparator = true;
				popup.AppendMenuIcon(ID_COMPARETWOCOMMITCHANGES, IDS_LOG_POPUP_COMPARECHANGESET, IDI_DIFF);
			}

			if (bAddSeparator)
			{
				popup.AppendMenu(MF_SEPARATOR, NULL);
				bAddSeparator = false;
			}

			if (m_ContextMenuMask&GetContextMenuBit(ID_REVERTREV) && m_hasWC && !isMergeActive)
					popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREVS, IDI_REVERT);

			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (selectedCount > 1 && isBisectActive && (m_ContextMenuMask & GetContextMenuBit(ID_BISECTSKIP)) && !IsBisect(pSelLogEntry))
		{
			popup.AppendMenuIcon(ID_BISECTSKIP, IDS_MENUBISECTSKIP, IDI_BISECT);
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (!pSelLogEntry->m_CommitHash.IsEmpty())
		{
			bool bAddSeparator = false;
			if (selectedCount >= 2 && IsSelectionContinuous())
			{
				if (m_ContextMenuMask&GetContextMenuBit(ID_COMBINE_COMMIT) && m_hasWC && !isMergeActive)
				{
					int headindex = this->GetHeadIndex();
					if(headindex>=0 && LastSelect >= headindex && FirstSelect >= headindex)
					{
						CString head;
						head.Format(L"HEAD~%d", FirstSelect - headindex);
						CGitHash hashFirst;
						int ret = g_Git.GetHash(hashFirst, head);
						head.Format(L"HEAD~%d",LastSelect-headindex);
						CGitHash hash;
						ret = ret || g_Git.GetHash(hash, head);
						GitRevLoglist* pFirstEntry = m_arShownList.SafeGetAt(FirstSelect);
						GitRevLoglist* pLastEntry = m_arShownList.SafeGetAt(LastSelect);
						if (!ret && pFirstEntry->m_CommitHash == hashFirst && pLastEntry->m_CommitHash == hash)
						{
							popup.AppendMenuIcon(ID_COMBINE_COMMIT,IDS_COMBINE_TO_ONE,IDI_COMBINE);
							bAddSeparator = true;
						}
					}
				}
			}
			if (m_ContextMenuMask&GetContextMenuBit(ID_CHERRY_PICK) && !isHeadCommit && m_hasWC && !isMergeActive) {
				if (selectedCount >= 2)
					popup.AppendMenuIcon(ID_CHERRY_PICK, IDS_CHERRY_PICK_VERSIONS, IDI_PICK);
				else
					popup.AppendMenuIcon(ID_CHERRY_PICK, IDS_CHERRY_PICK_VERSION, IDI_PICK);
				bAddSeparator = true;
			}

			if (selectedCount <= 2 || (IsSelectionContinuous() && !isStash))
				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_PATCH)) {
					popup.AppendMenuIcon(ID_CREATE_PATCH, IDS_CREATE_PATCH, IDI_PATCH);
					bAddSeparator = true;
				}

			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (m_hasWC && !isMergeActive && !isStash && (m_ContextMenuMask & GetContextMenuBit(ID_BISECTSTART)) && selectedCount == 2 && !m_arShownList.SafeGetAt(FirstSelect)->m_CommitHash.IsEmpty() && !isBisectActive)
		{
			popup.AppendMenuIcon(ID_BISECTSTART, IDS_MENUBISECTSTART, IDI_BISECT);
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if (selectedCount == 1)
		{
			bool bAddSeparator = false;
			if ((m_ContextMenuMask & GetContextMenuBit(ID_PUSH)) && ((!isStash && hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend()) || showExtendedMenu))
			{
				// show the push-option only if the log entry has an associated local branch
				bool isLocal = hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend() && find_if(hashMap[pSelLogEntry->m_CommitHash], [](const CString& ref) { return CStringUtils::StartsWith(ref, L"refs/heads/") || CStringUtils::StartsWith(ref, L"refs/tags/"); }) != hashMap[pSelLogEntry->m_CommitHash].cend();
				if (isLocal || showExtendedMenu)
				{
					CString str;
					str.LoadString(IDS_MENUPUSH);

					CString branch;
					size_t index = static_cast<size_t>(-1);
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, hashMap, &branch, &index))
						if (type == CGit::REF_TYPE::LOCAL_BRANCH || type == CGit::REF_TYPE::ANNOTATED_TAG || type == CGit::REF_TYPE::TAG)
							str.Insert(str.Find(L'.'), L" \"" + branch + L'"');

					popup.AppendMenuIcon(ID_PUSH, str, IDI_PUSH);

					if (index != static_cast<size_t>(-1) && index < hashMap[pSelLogEntry->m_CommitHash].size())
						popup.SetMenuItemData(ID_PUSH, reinterpret_cast<LONG_PTR>(&hashMap[pSelLogEntry->m_CommitHash][index]));

					if (m_ContextMenuMask & GetContextMenuBit(ID_SVNDCOMMIT) && workingTree.HasGitSVNDir())
						popup.AppendMenuIcon(ID_SVNDCOMMIT, IDS_MENUSVNDCOMMIT, IDI_COMMIT);

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
				if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.end() )
				{
					std::vector<const CString*> branchs;
					auto addCheck = [&](const CString& ref)
					{
						if (ref == currentBranch)
							return;
						branchs.push_back(&ref);
					};
					size_t index = static_cast<size_t>(-1);
					CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
					if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, hashMap, nullptr, &index))
						addCheck(hashMap[pSelLogEntry->m_CommitHash][index]);
					else
						for_each(hashMap[pSelLogEntry->m_CommitHash], addCheck);

					CString str;
					if (branchs.size() == 1)
					{
						str.LoadString(IDS_DELETE_BRANCHTAG_SHORT);
						str += L' ';
						str += *branchs[0];
						popup.AppendMenuIcon(ID_DELETE, str, IDI_DELETE);
						popup.SetMenuItemData(ID_DELETE, reinterpret_cast<LONG_PTR>(branchs[0]));
						bAddSeparator = true;
					}
					else if (branchs.size() > 1)
					{
						str.LoadString(IDS_DELETE_BRANCHTAG);
						submenu.CreatePopupMenu();
						for (size_t i = 0; i < branchs.size(); ++i)
						{
							submenu.AppendMenuIcon(ID_DELETE + (i << 16), *branchs[i]);
							submenu.SetMenuItemData(ID_DELETE + (i << 16), reinterpret_cast<LONG_PTR>(branchs[i]));
						}
						submenu.AppendMenuIcon(ID_DELETE + (branchs.size() << 16), IDS_ALL);
						submenu.SetMenuItemData(ID_DELETE + (branchs.size() << 16), reinterpret_cast<LONG_PTR>(MAKEINTRESOURCE(IDS_ALL)));

						popup.AppendMenuIcon(ID_DELETE,str, IDI_DELETE, submenu.m_hMenu);
						bAddSeparator = true;
					}
				}
			} // m_ContextMenuMask &GetContextMenuBit(ID_DELETE)
			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		} // selectedCount == 1

		CIconMenu clipSubMenu;
		if (!clipSubMenu.CreatePopupMenu())
			return;
		if (m_ContextMenuMask & GetContextMenuBit(ID_COPYCLIPBOARD))
		{
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFULL, IDS_LOG_POPUP_CLIPBOARD_FULL, IDI_COPYCLIP);
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDFULLNOPATHS, IDS_LOG_POPUP_CLIPBOARD_FULLNOPATHS, IDI_COPYCLIP);
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDHASH, IDS_LOG_HASH, IDI_COPYCLIP);
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDAUTHORSFULL, IDS_LOG_POPUP_CLIPBOARD_AUTHORSFULL, IDI_COPYCLIP);
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDAUTHORSNAME, IDS_LOG_POPUP_CLIPBOARD_AUTHORSNAME, IDI_COPYCLIP);
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDAUTHORSEMAIL, IDS_LOG_POPUP_CLIPBOARD_AUTHORSEMAIL, IDI_COPYCLIP);
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDSUBJECTS, IDS_LOG_POPUP_CLIPBOARD_SUBJECTS, IDI_COPYCLIP);
			clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDMESSAGES, IDS_LOG_POPUP_CLIPBOARD_MSGS, IDI_COPYCLIP);
			if (hashMap.find(pSelLogEntry->m_CommitHash) != hashMap.cend() && selectedCount == 1)
			{
				clipSubMenu.AppendMenuIcon(ID_COPYCLIPBOARDBRANCHTAG, IDS_LOG_POPUP_CLIPBOARD_TAGBRANCHES, IDI_COPYCLIP);
				size_t index = static_cast<size_t>(-1);
				CGit::REF_TYPE type = CGit::REF_TYPE::UNKNOWN;
				if (IsMouseOnRefLabelFromPopupMenu(pSelLogEntry, point, type, hashMap, nullptr, &index))
					clipSubMenu.SetMenuItemData(ID_COPYCLIPBOARDBRANCHTAG, reinterpret_cast<LONG_PTR>(&hashMap[pSelLogEntry->m_CommitHash][index]));
			}

			CString temp;
			temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
			popup.InsertMenu(static_cast<UINT>(-1), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(clipSubMenu.m_hMenu), temp);
		}

		if(m_ContextMenuMask&GetContextMenuBit(ID_FINDENTRY))
			popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND, IDI_FILTEREDIT);

		if (selectedCount == 1 && (m_ContextMenuMask & GetContextMenuBit(ID_SHOWBRANCHES)) && !pSelLogEntry->m_CommitHash.IsEmpty())
			popup.AppendMenuIcon(ID_SHOWBRANCHES, IDS_LOG_POPUP_SHOWBRANCHES, IDI_SHOWBRANCHES);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
//		DialogEnableWindow(IDOK, FALSE);
//		SetPromptApp(&theApp);

		this->ContextMenuAction(cmd, FirstSelect, LastSelect, &popup, hashMap);

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
		CString from(MAKEINTRESOURCE(IDS_STATUSLIST_FROM));
		bool first = true;
		while (pos)
		{
			CString sLogCopyText;
			CString sPaths;
			GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(GetNextSelectedItem(pos));

			if (toCopy == ID_COPYCLIPBOARDFULL)
			{
				sPaths = L"----\r\n";
				for (int cpPathIndex = 0; cpPathIndex<pLogEntry->GetFiles(this).GetCount(); ++cpPathIndex)
				{
					sPaths += ((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).GetActionName() + L": " + pLogEntry->GetFiles(this)[cpPathIndex].GetGitPathString();
					if (((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).m_Action & (CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_COPY) && !((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).GetGitOldPathString().IsEmpty())
					{
						sPaths += L' ';
						sPaths.AppendFormat(from, static_cast<LPCTSTR>(pLogEntry->GetFiles(this)[cpPathIndex].GetGitOldPathString()));
					}
					sPaths += L"\r\n";
				}
				sPaths.Trim();
				sPaths += L"\r\n";
			}

			if (toCopy == ID_COPYCLIPBOARDFULL || toCopy == ID_COPYCLIPBOARDFULLNOPATHS)
			{
				CString sNotesTags;
				if (!pLogEntry->m_Notes.IsEmpty())
				{
					sNotesTags = L"----\n" + CString(MAKEINTRESOURCE(IDS_NOTES));
					sNotesTags += L":\n";
					sNotesTags += pLogEntry->m_Notes;
					sNotesTags.Replace(L"\n", L"\r\n");
				}
				CString tagInfo = GetTagInfo(pLogEntry);
				if (!tagInfo.IsEmpty())
				{
					sNotesTags += L"----\r\n" + CString(MAKEINTRESOURCE(IDS_PROC_LOG_TAGINFO)) + L":\r\n";
					tagInfo.Replace(L"\n", L"\r\n");
					sNotesTags += tagInfo;
				}

				sLogCopyText.Format(L"%s: %s\r\n%s: %s <%s>\r\n%s: %s\r\n%s:\r\n%s\r\n%s%s\r\n",
					static_cast<LPCTSTR>(sRev), static_cast<LPCTSTR>(pLogEntry->m_CommitHash.ToString()),
					static_cast<LPCTSTR>(sAuthor), static_cast<LPCTSTR>(pLogEntry->GetAuthorName()), static_cast<LPCTSTR>(pLogEntry->GetAuthorEmail()),
					static_cast<LPCTSTR>(sDate),
					static_cast<LPCTSTR>(CLoglistUtils::FormatDateAndTime(pLogEntry->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes)),
					static_cast<LPCTSTR>(sMessage), static_cast<LPCTSTR>(pLogEntry->GetSubjectBody(true)),
					static_cast<LPCTSTR>(sNotesTags),
					static_cast<LPCTSTR>(sPaths));
				sClipdata +=  sLogCopyText;
			}
			else if (toCopy == ID_COPYCLIPBOARDAUTHORSFULL)
			{
				if (!first)
					sClipdata += L"\r\n";
				sClipdata += pLogEntry->GetAuthorName();
				sClipdata += L" <";
				sClipdata += pLogEntry->GetAuthorEmail();
				sClipdata += L">";
			}
			else if (toCopy == ID_COPYCLIPBOARDAUTHORSNAME)
			{
				if (!first)
					sClipdata += L"\r\n";
				sClipdata += pLogEntry->GetAuthorName();
			}
			else if (toCopy == ID_COPYCLIPBOARDAUTHORSEMAIL)
			{
				if (!first)
					sClipdata += L"\r\n";
				sClipdata += pLogEntry->GetAuthorEmail();
			}

			else if (toCopy == ID_COPYCLIPBOARDMESSAGES)
			{
				sClipdata += L"* ";
				sClipdata += pLogEntry->GetSubjectBody(true);
				sClipdata += L"\r\n\r\n";
			}
			else if (toCopy == ID_COPYCLIPBOARDSUBJECTS)
			{
				sClipdata += L"* ";
				sClipdata += pLogEntry->GetSubject().Trim();
				sClipdata += L"\r\n\r\n";
			}
			else
			{
				if (!first)
					sClipdata += L"\r\n";
				sClipdata += pLogEntry->m_CommitHash.ToString();
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

	POSITION pos = GetFirstSelectedItemPosition();
	auto FirstSelect = GetNextSelectedItem(pos);
	int LastSelect = -1;
	while (pos)
		LastSelect = GetNextSelectedItem(pos);

	auto hashMap = m_HashMap;
	ContextMenuAction(ID_COMPAREWITHPREVIOUS, FirstSelect, LastSelect, nullptr, *hashMap.get());
}

void CGitLogListBase::OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	*pResult = -1;

	if (pFindInfo->lvfi.flags & LVFI_PARAM)
		return;
	if (pFindInfo->iStart < 0 || pFindInfo->iStart >= static_cast<int>(m_arShownList.size()))
		return;
	if (!pFindInfo->lvfi.psz)
		return;
#if 0
	CString sCmp = pFindInfo->lvfi.psz;
	CString sRev;
	for (int i=pFindInfo->iStart; i<m_arShownList.GetCount(); ++i)
	{
		GitRev * pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.SafeGetAt(i));
		sRev.Format(L"%ld", pLogEntry->Rev);
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
			sRev.Format(L"%ld", pLogEntry->Rev);
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

	SetItemCountEx(static_cast<int>(m_logEntries.size()));

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

int CGitLogListBase::FillGitLog(std::unordered_set<CGitHash>& hashes)
{
	ClearText();

	m_arShownList.SafeRemoveAll();

	m_logEntries.ClearAll();
	if (m_logEntries.Fill(hashes))
		return -1;

	SetItemCountEx(static_cast<int>(m_logEntries.size()));

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

	int mask = CGit::LOG_INFO_ONLY_HASH;
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

	CString range;
	{
		Locker lock(m_critSec);
		range = m_sRange;
	}
	if (range.IsEmpty())
		range = L"HEAD";

	// follow does not work for directories
	if (!path || path->IsDirectory())
		mask &= ~CGit::LOG_INFO_FOLLOW;
	// follow does not work with all branches 8at least in TGit)
	if (mask & CGit::LOG_INFO_FOLLOW)
		mask &= ~(CGit::LOG_INFO_ALL_BRANCH | CGit::LOG_INFO_BASIC_REFS | CGit::LOG_INFO_LOCAL_BRANCHES);

	CString cmd = g_Git.GetLogCmd(range, path, mask, &m_Filter, CRegDWORD(L"Software\\TortoiseGit\\LogOrderBy", CGit::LOG_ORDER_TOPOORDER));

	//this->m_logEntries.ParserFromLog();
	if(IsInWorkingThread())
	{
		PostMessage(LVM_SETITEMCOUNT, this->m_logEntries.size(), LVSICF_NOINVALIDATEALL);
	}
	else
	{
		SetItemCountEx(static_cast<int>(m_logEntries.size()));
	}

	try
	{
		g_Git.CheckAndInitDll();
	}
	catch (char* msg)
	{
		CString err(msg);
		MessageBox(L"Could not initialize libgit.\nlibgit reports:\n" + err, L"TortoiseGit", MB_ICONERROR);
		return -1;
	}

	if (!g_Git.CanParseRev(range))
	{
		if (!(mask & CGit::LOG_INFO_ALL_BRANCH) && !(mask & CGit::LOG_INFO_BASIC_REFS) && !(mask & CGit::LOG_INFO_LOCAL_BRANCHES))
			return 0;

		// if show all branches, pick any ref as dummy entry ref
		STRING_VECTOR list;
		if (g_Git.GetRefList(list))
			MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);
		if (list.empty())
			return 0;

		cmd = g_Git.GetLogCmd(list[0], path, mask, &m_Filter, CRegDWORD(L"Software\\TortoiseGit\\LogOrderBy", CGit::LOG_ORDER_TOPOORDER));
	}

	g_Git.m_critGitDllSec.Lock();
	try {
		if (git_open_log(&m_DllGitLog, CUnicodeUtils::GetMulti(cmd, CP_UTF8)))
		{
			g_Git.m_critGitDllSec.Unlock();
			return -1;
		}
	}
	catch (char* msg)
	{
		g_Git.m_critGitDllSec.Unlock();
		CString err(msg);
		MessageBox(L"Could not open log.\nlibgit reports:\n" + err, L"TortoiseGit", MB_ICONERROR);
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
			if (CRegDWORD(L"Software\\TortoiseGit\\DiffByDoubleClickInLog", FALSE))
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

	if (CRegDWORD(L"Software\\TortoiseGit\\DiffByDoubleClickInLog", FALSE))
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
	return static_cast<CGitLogListBase*>(pVoid)->LogThread();
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
	::PostMessage(this->GetParent()->m_hWnd, MSG_LOAD_PERCENTAGE, GITLOG_START, 0);

	ULONGLONG  t1,t2;

	if(BeginFetchLog())
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);

		return 1;
	}

	// create a copy we can safely work on in this thread
	auto shared_filter(m_LogFilter);
	auto& filter = *shared_filter;

	TRACE(L"\n===Begin===\n");
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
	CString range;
	{
		Locker lock(m_critSec);
		range = m_sRange;
	}
	if (!g_Git.CanParseRev(range))
	{
		// walk revisions if show all branches and there exists any ref
		if (!(m_ShowMask & CGit::LOG_INFO_ALL_BRANCH) && !(m_ShowMask & CGit::LOG_INFO_BASIC_REFS) && !(m_ShowMask & CGit::LOG_INFO_LOCAL_BRANCHES))
			shouldWalk = false;
		else
		{
			STRING_VECTOR list;
			if (g_Git.GetRefList(list))
				MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);
			if (list.empty())
				shouldWalk = false;
		}
	}

	if (shouldWalk)
	{
		g_Git.m_critGitDllSec.Lock();
		if (!m_DllGitLog)
		{
			MessageBox(L"Opening log failed.", L"TortoiseGit", MB_ICONERROR);
			g_Git.m_critGitDllSec.Unlock();
			InterlockedExchange(&m_bThreadRunning, FALSE);
			InterlockedExchange(&m_bNoDispUpdates, FALSE);
			return 1;
		}
		int total = 0;
		try
		{
			[&] {git_get_log_firstcommit(m_DllGitLog);}();
			total = git_get_log_estimate_commit_count(m_DllGitLog);
		}
		catch (char* msg)
		{
			CString err(msg);
			MessageBox(L"Could not get first commit.\nlibgit reports:\n" + err, L"TortoiseGit", MB_ICONERROR);
			ret = -1;
		}
		g_Git.m_critGitDllSec.Unlock();

		auto hashMapSharedPtr = m_HashMap;
		auto hashMap = *hashMapSharedPtr.get();

		GIT_COMMIT commit;
		t2 = t1 = GetTickCount64();
		int oldprecentage = 0;
		size_t oldsize = m_logEntries.size();
		std::unordered_map<CGitHash, std::unordered_set<CGitHash>> commitChildren;
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
				MessageBox(L"Could not get next commit.\nlibgit reports:\n" + err, L"TortoiseGit", MB_ICONERROR);
				break;
			}
			g_Git.m_critGitDllSec.Unlock();

			if(ret)
			{
				if (ret != -2) // other than end of revision walking
					MessageBox((L"Could not get next commit.\nlibgit returns:" + std::to_wstring(ret)).c_str(), L"TortoiseGit", MB_ICONERROR);
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

			CGitHash hash = CGitHash::FromRaw(commit.m_hash);

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
				MessageBox(L"Could not get commit notes.\nlibgit reports:\n" + err, L"TortoiseGit", MB_ICONERROR);
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
						it = commitChildren.insert(make_pair(parentHash, std::unordered_set<CGitHash>())).first;
					it->second.insert(pRev->m_CommitHash);
				}
			}

#ifdef DEBUG
			pRev->DbgPrint();
			TRACE(L"\n");
#endif

			bool visible = filter(pRev, this, hashMap);
			if (visible && !ShouldShowFilter(pRev, commitChildren, hashMap))
				visible = false;
			this->m_critSec.Lock();
			m_logEntries.append(hash, visible);
			if (visible)
				m_arShownList.SafeAdd(pRev);
			this->m_critSec.Unlock();

			if (!visible)
				continue;

			if (lastSelectedHashNItem == -1 && hash == m_lastSelectedHash)
				lastSelectedHashNItem = static_cast<int>(m_arShownList.size()) - 1;

			t2 = GetTickCount64();

			if (t2 - t1 > 500UL || (m_logEntries.size() - oldsize > 100))
			{
				//update UI
				int percent = static_cast<int>(m_logEntries.size() * 100 / total + GITLOG_START + 1);
				if(percent > 99)
					percent =99;
				if(percent < GITLOG_START)
					percent = GITLOG_START +1;

				oldsize = m_logEntries.size();
				PostMessage(LVM_SETITEMCOUNT, this->m_logEntries.size(), LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);

				//if( percent > oldprecentage )
				{
					::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE, percent, 0);
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
	PostMessage(LVM_SETITEMCOUNT, this->m_logEntries.size(), LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);

	if (lastSelectedHashNItem >= 0)
		PostMessage(m_ScrollToMessage, lastSelectedHashNItem);

	if (this->m_hWnd)
		::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE, GITLOG_END, 0);

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
	auto hashMap = m_HashMap;
	for (auto it = hashMap->cbegin(); it != hashMap->cend(); ++it)
	{
		for (const auto& ref : it->second)
		{
			CString branchName;
			if (CGit::GetShortName(ref, branchName, L"refs/heads/"))
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

	if (m_pMailmap)
	{
		git_free_mailmap(m_pMailmap);
		git_read_mailmap(&m_pMailmap);
	}

	//Update branch and Tag info
	ReloadHashMap();
	if (m_pFindDialog)
		m_pFindDialog->RefreshList();
	//Assume Thread have exited
	//if(!m_bThreadRunning)
	{
		m_logEntries.clear();

		if (IsCleanFilter)
			m_LogFilter = std::make_shared<CLogDlgFilter>();

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
	m_DiffingThread = AfxBeginThread(AsyncThread, this, THREAD_PRIORITY_BELOW_NORMAL, 0, CREATE_SUSPENDED);
	if (!m_DiffingThread)
	{
		InterlockedExchange(&m_AsyncThreadRunning, FALSE);
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}
	m_DiffingThread->m_bAutoDelete = FALSE;
	m_DiffingThread->ResumeThread();
}

void CGitLogListBase::StartLoadingThread()
{
	if (InterlockedExchange(&m_bThreadRunning, TRUE) != FALSE)
		return;
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	InterlockedExchange(&m_bExitThread, FALSE);
	m_LoadingThread = AfxBeginThread(LogThreadEntry, this, THREAD_PRIORITY_LOWEST, 0, CREATE_SUSPENDED);
	if (!m_LoadingThread)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}
	m_LoadingThread->m_bAutoDelete = FALSE;
	m_LoadingThread->ResumeThread();
}

bool CGitLogListBase::ShouldShowFilter(GitRevLoglist* pRev, const std::unordered_map<CGitHash, std::unordered_set<CGitHash>>& commitChildren, MAP_HASH_NAME& hashMap)
{
	if (m_ShowFilter & FILTERSHOW_ANYCOMMIT)
		return true;

	if ((m_ShowFilter & FILTERSHOW_REFS) && hashMap.find(pRev->m_CommitHash) != hashMap.cend())
	{
		// Keep all refs.
		const STRING_VECTOR& refList = hashMap[pRev->m_CommitHash];
		for (size_t i = 0; i < refList.size(); ++i)
		{
			const CString &str = refList[i];
			if (CStringUtils::StartsWith(str, L"refs/heads/"))
			{
				if (m_ShowRefMask & LOGLIST_SHOWLOCALBRANCHES)
					return true;
			}
			else if (CStringUtils::StartsWith(str, L"refs/remotes/"))
			{
				if (m_ShowRefMask & LOGLIST_SHOWREMOTEBRANCHES)
					return true;
			}
			else if (CStringUtils::StartsWith(str, L"refs/tags/"))
			{
				if (m_ShowRefMask & LOGLIST_SHOWTAGS)
					return true;
			}
			else if (CStringUtils::StartsWith(str, L"refs/stash"))
			{
				if (m_ShowRefMask & LOGLIST_SHOWSTASH)
					return true;
			}
			else if (CStringUtils::StartsWith(str, L"refs/bisect/"))
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
			const std::unordered_set<CGitHash> &children = childrenIt->second;
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

CString CGitLogListBase::GetTagInfo(GitRev* pLogEntry) const
{
	auto hashMap = m_HashMap;
	auto refs = hashMap->find(pLogEntry->m_CommitHash);
	if (refs == hashMap->end())
		return L"";

	return GetTagInfo(refs->second);
}

CString CGitLogListBase::GetTagInfo(const STRING_VECTOR& refs) const
{
	CString tagInfo;
	for (auto it = refs.cbegin(); it != refs.cend(); ++it)
	{
		if (!CStringUtils::StartsWith((*it), L"refs/tags/"))
			continue;
		if (!CStringUtils::EndsWith((*it), L"^{}"))
			continue;

		CString cmd;
		cmd.Format(L"git.exe cat-file tag %s", static_cast<LPCTSTR>((*it).Left((*it).GetLength() - static_cast<int>(wcslen(L"^{}")))));
		CString output;
		if (g_Git.Run(cmd, &output, nullptr, CP_UTF8) != 0)
			continue;

		// parse tag date
		do
		{
			// this assumes that in the header of the tag there is no ">" before the "tagger " header entry
			int pos1 = output.Find(L'>');
			if (pos1 < 0)
				break;
			++pos1;
			if (output[pos1] == L' ')
				++pos1;
			int pos2 = output.Find(L'\n', pos1);
			if (pos2 < 0)
				break;

			CString str = output.Mid(pos1, pos2 - pos1);
			wchar_t* pEnd = nullptr;
			errno = 0;
			auto number = wcstoumax(str.GetBuffer(), &pEnd, 10);
			if (str.GetBuffer() == pEnd)
				break;
			if (errno == ERANGE)
				break;

			output.Delete(pos1, pos2 - pos1);
			output.Insert(pos1, static_cast<LPCWSTR>(CLoglistUtils::FormatDateAndTime(CTime(number), m_DateFormat, true, m_bRelativeTimes)));
		} while (0);
		output.Trim().AppendChar(L'\n');
		tagInfo += output;
	}
	return tagInfo;
}

void CGitLogListBase::Clear()
{
	m_arShownList.SafeRemoveAll();
	DeleteAllItems();

	m_logEntries.ClearAll();
}

void CGitLogListBase::OnDestroy()
{
	SafeTerminateThread();
	SafeTerminateAsyncDiffThread();

	int retry = 0;
	while(m_LogCache.SaveCache())
	{
		if(retry > 5)
			break;
		Sleep(1000);

		++retry;

		//if(CMessageBox::Show(nullptr, L"Cannot Save Log Cache to Disk. To retry click yes. To give up click no.", L"TortoiseGit",
		//					MB_YESNO) == IDNO)
		//					break;
	}

	__super::OnDestroy();
}

LRESULT CGitLogListBase::OnLoad(WPARAM wParam,LPARAM /*lParam*/)
{
	CRect rect;
	int i = static_cast<int>(wParam);
	this->GetItemRect(i,&rect,LVIR_BOUNDS);
	this->InvalidateRect(rect);

	return 0;
}

/**
 * Save column widths to the registry
 */
void CGitLogListBase::SaveColumnWidths()
{
	// HACK that graph column is always shown
	SetColumnWidth(0, m_ColumnManager.GetWidth(0, false));

	__super::SaveColumnWidths();
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
			if (pRev->m_CommitHash == m_HeadHash)
				return static_cast<int>(i);
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

LRESULT CGitLogListBase::OnScrollToMessage(WPARAM itemToSelect, LPARAM /*lParam*/)
{
	if (GetSelectedCount() != 0)
		return 0;

	CGitHash theSelectedHash = m_lastSelectedHash;
	SetItemState(static_cast<int>(itemToSelect), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
	m_lastSelectedHash = theSelectedHash;

	int countPerPage = GetCountPerPage();
	EnsureVisible(max(0, static_cast<int>(itemToSelect) - countPerPage / 2), FALSE);
	EnsureVisible(min(GetItemCount(), static_cast<int>(itemToSelect) + countPerPage / 2), FALSE);
	EnsureVisible(static_cast<int>(itemToSelect), FALSE);
	return 0;
}

LRESULT CGitLogListBase::OnScrollToRef(WPARAM wParam, LPARAM /*lParam*/)
{
	CString* ref = reinterpret_cast<CString*>(wParam);
	if (!ref || ref->IsEmpty())
		return 1;

	bool bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

	CGitHash hash;
	if (g_Git.GetHash(hash, *ref + L"^{}")) // add ^{} in order to get the correct SHA-1 (especially for signed tags)
		MessageBox(g_Git.GetGitLastErr(L"Could not get hash of ref \"" + *ref + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);

	if (hash.IsEmpty())
		return 1;

	bool bFound = false;
	int cnt = static_cast<int>(m_arShownList.size());
	int i;
	for (i = 0; i < cnt; ++i)
	{
		GitRev* pLogEntry = m_arShownList.SafeGetAt(i);
		if (pLogEntry && pLogEntry->m_CommitHash == hash)
		{
			bFound = true;
			break;
		}
	}
	if (!bFound)
		return 1;

	EnsureVisible(i, FALSE);
	if (!bShift)
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
	UpdateData(FALSE);

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

	int cnt = static_cast<int>(m_arShownList.size());
	bool bShift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

	if(m_pFindDialog->IsRef())
	{
		CString str;
		str=m_pFindDialog->GetFindString();

		CGitHash hash;

		if(!str.IsEmpty())
		{
			if (g_Git.GetHash(hash, str + L"^{}")) // add ^{} in order to get the correct SHA-1 (especially for signed tags)
				MessageBox(g_Git.GetGitLastErr(L"Could not get hash of ref \"" + str + L"^{}\"."), L"TortoiseGit", MB_ICONERROR);
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
		CLogDlgFilter filter { m_pFindDialog->GetFindString(), m_pFindDialog->Regex(), LOGFILTER_ALL, m_pFindDialog->MatchCase() == TRUE };

		auto hashMapSharedPtr = m_HashMap;
		auto hashMap = *hashMapSharedPtr.get();

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

			if (filter(m_arShownList.SafeGetAt(i), this, hashMap))
			{
				bFound = true;
				break;
			}
		}
	} // if(m_pFindDialog->FindNext())

	if (bFound)
	{
		m_nSearchIndex = i;
		EnsureVisible(i, FALSE);
		if (!bShift)
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
		pTI->uId = static_cast<UINT>((nItem << 10) + (nSubItem & 0x3ff) + 1);
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
	auto pTTTA = reinterpret_cast<TOOLTIPTEXTA*>(pNMHDR);
	auto pTTTW = reinterpret_cast<TOOLTIPTEXTW*>(pNMHDR);

	*pResult = 0;

	// Ignore messages from the built in tooltip, we are processing them internally
	if ((pNMHDR->idFrom == reinterpret_cast<UINT_PTR>(m_hWnd)) &&
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
			auto hashMap = m_HashMap;
			if (IsMouseOnRefLabel(m_arShownList.SafeGetAt(nItem), lvhitTestInfo.pt, type, *hashMap.get(), &branch))
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
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);

		if (strTipText.GetLength() >= _countof(m_wszTip))
		{
			strTipText.Truncate(_countof(m_wszTip) - 1 - 3);
			strTipText += L"...";
		}
		wcsncpy_s(m_wszTip, strTipText, _TRUNCATE);
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
		auto hashMap = m_HashMap;
		if (hashMap->find(pLogEntry->m_CommitHash) == hashMap->cend() && (m_superProjectHash.IsEmpty() || pLogEntry->m_CommitHash != m_superProjectHash))
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

bool CGitLogListBase::IsMouseOnRefLabelFromPopupMenu(const GitRevLoglist* pLogEntry, const CPoint& point, CGit::REF_TYPE& type, MAP_HASH_NAME& hashMap, CString* pShortname /*nullptr*/, size_t* pIndex /*nullptr*/)
{
	POINT pt = point;
	ScreenToClient(&pt);
	return IsMouseOnRefLabel(pLogEntry, pt, type, hashMap, pShortname, pIndex);
}

bool CGitLogListBase::IsMouseOnRefLabel(const GitRevLoglist* pLogEntry, const POINT& pt, CGit::REF_TYPE& type, MAP_HASH_NAME& hashMap, CString* pShortname /*nullptr*/, size_t* pIndex /*nullptr*/)
{
	if (!pLogEntry)
		return false;

	auto refList = hashMap.find(pLogEntry->m_CommitHash);
	if (refList == hashMap.cend())
		return false;

	for (size_t i = 0; i < hashMap[pLogEntry->m_CommitHash].size(); ++i)
	{
		const auto labelpos = m_RefLabelPosMap.find(hashMap[pLogEntry->m_CommitHash][i]);
		if (labelpos == m_RefLabelPosMap.cend() || !labelpos->second.PtInRect(pt))
			continue;

		CGit::REF_TYPE foundType;
		if (pShortname)
			*pShortname = CGit::GetShortName(hashMap[pLogEntry->m_CommitHash][i], &foundType);
		else
			CGit::GetShortName(hashMap[pLogEntry->m_CommitHash][i], &foundType);
		if (foundType != type && type != CGit::REF_TYPE::UNKNOWN)
			return false;

		type = foundType;
		if (pIndex)
			*pIndex = i;
		return true;
	}
	return false;
}

void CGitLogListBase::OnBeginDrag(NMHDR* /*pnmhdr*/, LRESULT* pResult)
{
	*pResult = 0;

	if (!m_bDragndropEnabled || GetSelectedCount() == 0 || !IsSelectionContinuous())
		return;

	m_bDragging = TRUE;
	m_nDropIndex = -1;
	m_nDropMarkerLast = -1;
	m_nDropMarkerLastHot = GetHotItem();
	SetCapture();
}

void CGitLogListBase::OnMouseMove(UINT nFlags, CPoint point)
{
	__super::OnMouseMove(nFlags, point);

	if (!m_bDragging)
		return;

	CPoint dropPoint = point;
	ClientToScreen(&dropPoint);

	if (WindowFromPoint(dropPoint) != this)
	{
		SetCursor(LoadCursor(nullptr, IDC_NO));
		m_nDropIndex = -1;
		DrawDropInsertMarker(m_nDropIndex);
		return;
	}

	SetCursor(LoadCursor(nullptr, IDC_ARROW));
	ScreenToClient(&dropPoint);

	dropPoint.y += 10;
	m_nDropIndex = HitTest(dropPoint);

	if (m_nDropIndex == -1) // might be last item, allow to move past last item
	{
		dropPoint.y -= 10;
		m_nDropIndex = HitTest(dropPoint);
		if (m_nDropIndex != -1)
			m_nDropIndex = GetItemCount();
	}

	POSITION pos = GetFirstSelectedItemPosition();
	int first = GetNextSelectedItem(pos);
	int last = first;
	while (pos)
		last = GetNextSelectedItem(pos);
	if (m_nDropIndex == -1 || (m_nDropIndex >= first && m_nDropIndex - 1 <= last))
	{
		SetCursor(LoadCursor(nullptr, IDC_NO));
		m_nDropIndex = -1;
	}

	// handle auto scrolling
	int hotItem = GetHotItem();
	int topindex = GetTopIndex();
	if (hotItem == topindex && hotItem != 0)
		EnsureVisible(hotItem - 1, FALSE);
	else if (hotItem >= topindex + GetCountPerPage() - 1 && hotItem + 1 < GetItemCount())
		EnsureVisible(hotItem + 1, FALSE);

	DrawDropInsertMarker(m_nDropIndex);
}

void CGitLogListBase::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_bDragging)
	{
		::ReleaseCapture();
		SetCursor(LoadCursor(nullptr, IDC_HAND));
		m_bDragging = FALSE;

		CRect rect;
		GetItemRect(m_nDropMarkerLast, &rect, 0);
		rect.bottom = rect.top + 2;
		rect.top -= 2;
		InvalidateRect(&rect, 0);

		CPoint pt(point);
		ClientToScreen(&pt);
		if (WindowFromPoint(pt) == this && m_nDropIndex != -1)
			GetParent()->PostMessage(MSG_COMMITS_REORDERED, m_nDropIndex, 0);
	}

	__super::OnLButtonUp(nFlags, point);
}

void CGitLogListBase::DrawDropInsertMarker(int nIndex)
{
	if (m_nDropMarkerLast != nIndex)
	{
		CRect rect;
		if (GetItemRect(m_nDropMarkerLast, &rect, 0))
		{
			rect.bottom = rect.top + 2;
			rect.top -= 2;
			InvalidateRect(&rect, 0);
		}
		else if (m_nDropMarkerLast == GetItemCount())
			DrawDropInsertMarkerLine(m_nDropMarkerLast); // double painting = removal
		m_nDropMarkerLast = nIndex;

		if (nIndex < 0)
			return;

		DrawDropInsertMarkerLine(m_nDropMarkerLast);
	}
	else if (m_nDropMarkerLastHot != GetHotItem())
	{
		m_nDropMarkerLastHot = GetHotItem();
		m_nDropMarkerLast = -1;
	}
}

void CGitLogListBase::DrawDropInsertMarkerLine(int nIndex)
{
	CBrush* pBrush = CDC::GetHalftoneBrush();
	CDC* pDC = GetDC();

	CRect rect;
	if (nIndex < GetItemCount())
	{
		GetItemRect(nIndex, &rect, 0);
		rect.bottom = rect.top + 2;
		rect.top -= 2;
	}
	else
	{
		GetItemRect(nIndex - 1, &rect, 0);
		rect.top = rect.bottom - 2;
		rect.bottom += 2;
	}

	CBrush* pBrushOld = pDC->SelectObject(pBrush);
	pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(), PATINVERT);
	pDC->SelectObject(pBrushOld);

	ReleaseDC(pDC);
}

ULONG CGitLogListBase::GetGestureStatus(CPoint /*ptTouch*/)
{
	return 0;
}

void CGitLogListBase::DrawListItemWithMatchesRect(NMLVCUSTOMDRAW* pLVCD, const std::vector<CHARRANGE>& ranges, CRect rect, const CString& text, CColors& colors, HTHEME hTheme /*= nullptr*/, int txtState /*= 0*/)
{
	int drawPos = 0;
	COLORREF textColor = pLVCD->clrText;
	RECT rc = rect;
	if (!hTheme)
	{
		::SetTextColor(pLVCD->nmcd.hdc, textColor);
		SetBkMode(pLVCD->nmcd.hdc, TRANSPARENT);
	}
	DTTOPTS opts = { 0 };
	opts.dwSize = sizeof(opts);
	opts.crText = textColor;
	opts.dwFlags = DTT_TEXTCOLOR;

	for (auto it = ranges.cbegin(); it != ranges.cend(); ++it)
	{
		rc = rect;
		if (it->cpMin - drawPos)
		{
			if (!hTheme)
			{
				DrawText(pLVCD->nmcd.hdc, text.Mid(drawPos), it->cpMin - drawPos, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				DrawText(pLVCD->nmcd.hdc, text.Mid(drawPos), it->cpMin - drawPos, &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
			}
			else
			{
				DrawThemeTextEx(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, text.Mid(drawPos), it->cpMin - drawPos, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS, &rc, &opts);
				GetThemeTextExtent(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, text.Mid(drawPos), it->cpMin - drawPos, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS, &rect, &rc);
			}
			rect.left = rc.right;
		}
		rc = rect;
		drawPos = it->cpMin;
		if (it->cpMax - drawPos)
		{
			if (!hTheme)
			{
				::SetTextColor(pLVCD->nmcd.hdc, colors.GetColor(CColors::FilterMatch));
				DrawText(pLVCD->nmcd.hdc, text.Mid(drawPos), it->cpMax - drawPos, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				DrawText(pLVCD->nmcd.hdc, text.Mid(drawPos), it->cpMax - drawPos, &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
				::SetTextColor(pLVCD->nmcd.hdc, textColor);
			}
			else
			{
				opts.crText = colors.GetColor(CColors::FilterMatch);
				DrawThemeTextEx(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, text.Mid(drawPos), it->cpMax - drawPos, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS, &rc, &opts);
				GetThemeTextExtent(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, text.Mid(drawPos), it->cpMax - drawPos, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS, &rect, &rc);
				opts.crText = textColor;
			}
			rect.left = rc.right;
		}
		rc = rect;
		drawPos = it->cpMax;
	}
	if (!hTheme)
		DrawText(pLVCD->nmcd.hdc, text.Mid(drawPos), -1, &rc, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
	else
		DrawThemeTextEx(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, text.Mid(drawPos), -1, DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS, &rc, &opts);
}

bool CGitLogListBase::DrawListItemWithMatchesIfEnabled(std::shared_ptr<CLogDlgFilter> filter, DWORD selectedFilter, NMLVCUSTOMDRAW* pLVCD, LRESULT* pResult)
{
	if ((filter->GetSelectedFilters() & selectedFilter) && filter->IsFilterActive())
	{
		*pResult = DrawListItemWithMatches(filter.get(), *this, pLVCD, m_Colors);
		return true;
	}
	return false;
}

LRESULT CGitLogListBase::DrawListItemWithMatches(CFilterHelper* filter, CListCtrl& listCtrl, NMLVCUSTOMDRAW* pLVCD, CColors& colors)
{
	CString text = static_cast<LPCTSTR>(listCtrl.GetItemText(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem));
	if (text.IsEmpty())
		return CDRF_DODEFAULT;

	std::vector<CHARRANGE> ranges;
	filter->GetMatchRanges(ranges, text, 0);
	if (ranges.empty())
		return CDRF_DODEFAULT;

	// even though we initialize the 'rect' here with nmcd.rc,
	// we must not use it but use the rects from GetItemRect()
	// and GetSubItemRect(). Because on XP, the nmcd.rc has
	// bogus data in it.
	CRect rect = pLVCD->nmcd.rc;

	// find the margin where the text label starts
	CRect labelRC, boundsRC, iconRC;
	listCtrl.GetItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), &labelRC, LVIR_LABEL);
	listCtrl.GetItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), &iconRC, LVIR_ICON);
	listCtrl.GetItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), &boundsRC, LVIR_BOUNDS);

	int leftmargin = labelRC.left - boundsRC.left;
	if (pLVCD->iSubItem)
		leftmargin -= iconRC.Width();

	if (pLVCD->iSubItem != 0)
		listCtrl.GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_BOUNDS, rect);

	int borderWidth = 0;
	HTHEME hTheme = nullptr;
	if (IsAppThemed())
	{
		hTheme = OpenThemeData(listCtrl.m_hWnd, L"Explorer::ListView;ListView");
		GetThemeMetric(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, LISS_NORMAL, TMT_BORDERSIZE, &borderWidth);
	}
	else
		borderWidth = GetSystemMetrics(SM_CXBORDER);

	if (listCtrl.GetExtendedStyle() & LVS_EX_CHECKBOXES)
	{
		// I'm not very happy about this fixed margin here
		// but I haven't found a way to ask the system what
		// the margin really is.
		// At least it works on XP/Vista/win7/win8, and even with
		// increased font sizes
		leftmargin = 4;
	}

	LVITEM item = { 0 };
	item.iItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
	item.iSubItem = 0;
	item.mask = LVIF_IMAGE | LVIF_STATE;
	item.stateMask = static_cast<UINT>(-1);
	listCtrl.GetItem(&item);

	// fill background
	int txtState = LISS_NORMAL;
	if (!hTheme)
	{
		HBRUSH brush = nullptr;
		if (item.state & LVIS_SELECTED)
		{
			if (::GetFocus() == listCtrl.GetSafeHwnd())
			{
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
				pLVCD->clrText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
			}
			else
			{
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
				pLVCD->clrText = ::GetSysColor(COLOR_WINDOWTEXT);
			}
		}
		else
			brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
		CRect my;
		listCtrl.GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_LABEL, my);
		::FillRect(pLVCD->nmcd.hdc, my, brush);
		::DeleteObject(brush);
	}
	else
	{
		if (listCtrl.GetHotItem() == static_cast<int>(pLVCD->nmcd.dwItemSpec))
		{
			if (item.state & LVIS_SELECTED)
				txtState = LISS_HOTSELECTED;
			else
				txtState = LISS_HOT;
		}
		else if (item.state & LVIS_SELECTED)
		{
			if (::GetFocus() == listCtrl.GetSafeHwnd())
				txtState = LISS_SELECTED;
			else
				txtState = LISS_SELECTEDNOTFOCUS;
		}

		if (IsThemeBackgroundPartiallyTransparent(hTheme, LVP_LISTDETAIL, txtState))
			DrawThemeParentBackground(listCtrl.m_hWnd, pLVCD->nmcd.hdc, &rect);
		else
		{
			HBRUSH brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
			::FillRect(pLVCD->nmcd.hdc, rect, brush);
			::DeleteObject(brush);
		}
		if (txtState != LISS_NORMAL)
		{
			CRect my;
			listCtrl.GetSubItemRect(static_cast<int>(pLVCD->nmcd.dwItemSpec), pLVCD->iSubItem, LVIR_LABEL, my);
			if (pLVCD->iSubItem == 0)
			{
				// also fill the icon part of the line
				my.top = 0;
				my.left = 0;
			}

			// calculate background for rect of whole line, but limit redrawing to SubItem rect
			DrawThemeBackground(hTheme, pLVCD->nmcd.hdc, LVP_LISTITEM, txtState, boundsRC, my);
		}
	}

	// draw the icon for the first column
	if (pLVCD->iSubItem == 0)
	{
		rect = boundsRC;
		rect.right = rect.left + listCtrl.GetColumnWidth(0);
		rect.left = iconRC.left;

		if (item.iImage >= 0)
		{
			POINT pt;
			pt.x = rect.left;
			pt.y = rect.top;
			CDC dc;
			dc.Attach(pLVCD->nmcd.hdc);
			int style = ILD_TRANSPARENT;
			if (!hTheme)
			{
				auto whitebrush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
				::FillRect(dc, iconRC, whitebrush);
				::DeleteObject(whitebrush);
				if (item.state & LVIS_SELECTED)
				{
					if (::GetFocus() == listCtrl.GetSafeHwnd())
						style = ILD_SELECTED;
					else
						style = ILD_FOCUS;
				}
			}
			listCtrl.GetImageList(LVSIL_SMALL)->Draw(&dc, item.iImage, pt, style);
			dc.Detach();
			leftmargin -= iconRC.left;
		}
		else
		{
			RECT irc = boundsRC;
			irc.left += borderWidth;
			irc.right = iconRC.left;

			int state = 0;
			if (item.state & LVIS_SELECTED)
			{
				if (listCtrl.GetHotItem() == item.iItem)
					state = CBS_CHECKEDHOT;
				else
					state = CBS_CHECKEDNORMAL;
			}
			else
			{
				if (listCtrl.GetHotItem() == item.iItem)
					state = CBS_UNCHECKEDHOT;
			}
			if ((state) && (listCtrl.GetExtendedStyle() & LVS_EX_CHECKBOXES))
			{
				HTHEME hTheme2 = OpenThemeData(listCtrl.m_hWnd, L"BUTTON");
				DrawThemeBackground(hTheme2, pLVCD->nmcd.hdc, BP_CHECKBOX, state, &irc, NULL);
				CloseThemeData(hTheme2);
			}
		}
	}
	InflateRect(&rect, -(2 * borderWidth), 0);

	rect.left += leftmargin;
	RECT rc = rect;

	// is the column left- or right-aligned? (we don't handle centered (yet))
	LVCOLUMN Column;
	Column.mask = LVCF_FMT;
	listCtrl.GetColumn(pLVCD->iSubItem, &Column);
	if (Column.fmt & LVCFMT_RIGHT)
	{
		DrawText(pLVCD->nmcd.hdc, text, -1, &rc, DT_CALCRECT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
		rect.left = rect.right - (rc.right - rc.left);
		if (!hTheme)
		{
			rect.left += 2 * borderWidth;
			rect.right += 2 * borderWidth;
		}
	}

	DrawListItemWithMatchesRect(pLVCD, ranges, rect, text, colors, hTheme, txtState);
	if (hTheme)
		CloseThemeData(hTheme);

	return CDRF_SKIPDEFAULT;
}
