// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2005-2007 Marco Costalba
// Copyright (C) 2011-2012 - Sven Strickroth <email@cs-ware.de>

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
#include "GitRev.h"
//#include "VssStyle.h"
#include "IconMenu.h"
// CGitLogListBase
#include "cursor.h"
#include "InputDlg.h"
#include "GITProgressDlg.h"
#include "ProgressDlg.h"
//#include "RepositoryBrowser.h"
//#include "CopyDlg.h"
//#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "LoglistUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
//#include "GitInfo.h"
//#include "GitDiff.h"
#include "IconMenu.h"
//#include "RevisionRangeDlg.h"
//#include "BrowseFolder.h"
//#include "BlameDlg.h"
//#include "Blame.h"
//#include "GitHelpers.h"
#include "GitStatus.h"
//#include "LogDlgHelper.h"
//#include "CachedLogInfo.h"
//#include "RepositoryInfo.h"
//#include "EditPropertiesDlg.h"
#include "FileDiffDlg.h"
#include "..\\TortoiseShell\\Resource.h"
#include "FindDlg.h"
#include "SysInfo.h"

const UINT CGitLogListBase::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

IMPLEMENT_DYNAMIC(CGitLogListBase, CHintListCtrl)

CGitLogListBase::CGitLogListBase():CHintListCtrl()
	,m_regMaxBugIDColWidth(_T("Software\\TortoiseGit\\MaxBugIDColWidth"), 200)
	,m_nSearchIndex(0)
	,m_bNoDispUpdates(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bStrictStopped(false)
	, m_pStoreSelection(NULL)
	, m_SelectedFilters(LOGFILTER_ALL)
	, m_bShowWC(false)
	, m_logEntries(&m_LogCache)
	, m_pFindDialog(NULL)
	, m_ColumnManager(this)
	, m_dwDefaultColumns(0)
	, m_arShownList(&m_critSec)
	, m_hasWC(true)
{
	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);

	m_bShowBugtraqColumn=false;

	m_IsIDReplaceAction=FALSE;

	this->m_critSec.Init();
	m_wcRev.m_CommitHash.Empty();
	m_wcRev.GetSubject() = CString(MAKEINTRESOURCE(IDS_LOG_WORKINGDIRCHANGES));
	m_wcRev.m_ParentHash.clear();
	m_wcRev.m_Mark=_T('-');
	m_wcRev.m_IsUpdateing=FALSE;
	m_wcRev.m_IsFull = TRUE;

	m_hModifiedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hFetchIcon	= (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONFETCHING), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), TRUE);

	g_Git.GetMapHashToFriendName(m_HashMap);
	m_CurrentBranch=g_Git.GetCurrentBranch();
	this->m_HeadHash=g_Git.GetHash(_T("HEAD"));

	m_From=-1;;
	m_To=-1;

	m_ShowMask = 0;
	m_LoadingThread = NULL;

	InterlockedExchange(&m_bExitThread,FALSE);
	m_IsOldFirst = FALSE;
	m_IsRebaseReplaceGraph = FALSE;


	for(int i=0;i<Lanes::COLORS_NUM;i++)
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

	m_ColumnRegKey=_T("log");

	m_AsyncThreadExit = FALSE;
	m_AsyncDiffEvent = ::CreateEvent(NULL,FALSE,TRUE,NULL);
	m_AsynDiffListLock.Init();

	m_DiffingThread = AfxBeginThread(AsyncThread, this, THREAD_PRIORITY_BELOW_NORMAL);
	if (m_DiffingThread ==NULL)
	{
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

}

int CGitLogListBase::AsyncDiffThread()
{
	m_AsyncThreadExited = false;
	while(!m_AsyncThreadExit)
	{
		::WaitForSingleObject(m_AsyncDiffEvent, INFINITE);

		GitRev *pRev = NULL;
		while(!m_AsyncThreadExit && m_AsynDiffList.size() > 0)
		{
			m_AsynDiffListLock.Lock();
			pRev = m_AsynDiffList.back();
			m_AsynDiffList.pop_back();
			m_AsynDiffListLock.Unlock();

			if( pRev->m_CommitHash.IsEmpty() )
			{
				if(pRev->m_IsDiffFiles)
					continue;

				pRev->GetFiles(this).Clear();
				pRev->m_ParentHash.clear();
				pRev->m_ParentHash.push_back(m_HeadHash);
				if(g_Git.IsInitRepos())
				{
					g_Git.GetInitAddList(pRev->GetFiles(this));

				}
				else
				{
					g_Git.GetCommitDiffList(pRev->m_CommitHash.ToString(),this->m_HeadHash.ToString(), pRev->GetFiles(this));
				}
				pRev->GetAction(this) = 0;

				for(int j=0;j< pRev->GetFiles(this).GetCount();j++)
					pRev->GetAction(this) |= pRev->GetFiles(this)[j].m_Action;

				InterlockedExchange(&pRev->m_IsDiffFiles, TRUE);
				InterlockedExchange(&pRev->m_IsFull, TRUE);

				pRev->GetBody().Format(IDS_FILESCHANGES, pRev->GetFiles(this).GetCount());
				::PostMessage(m_hWnd,MSG_LOADED,(WPARAM)0,0);
				this->GetParent()->PostMessage(WM_COMMAND, MSG_FETCHED_DIFF, 0);
			}

			if(!pRev->CheckAndDiff())
			{	// fetch change file list
				for(int i=GetTopIndex(); !m_AsyncThreadExit && i <= GetTopIndex()+GetCountPerPage(); i++)
				{
					if(i < m_arShownList.GetCount())
					{
						GitRev* data = (GitRev*)m_arShownList.SafeGetAt(i);
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
						GitRev* data = (GitRev*)m_arShownList[nItem];
						if(data)
							if(data->m_CommitHash == pRev->m_CommitHash)
							{
								this->GetParent()->PostMessage(WM_COMMAND, MSG_FETCHED_DIFF, 0);
							}
					}
				}
			}
		}
	}
	m_AsyncThreadExited = true;
	return 0;
}
void CGitLogListBase::hideFromContextMenu(unsigned __int64 hideMask, bool exclusivelyShow)
{
	if (exclusivelyShow)
	{
		m_ContextMenuMask &= hideMask;
	}
	else
	{
		m_ContextMenuMask &= ~hideMask;
	}
}

CGitLogListBase::~CGitLogListBase()
{
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	this->m_arShownList.SafeRemoveAll();

	DestroyIcon(m_hModifiedIcon);
	DestroyIcon(m_hReplacedIcon);
	DestroyIcon(m_hAddedIcon);
	DestroyIcon(m_hDeletedIcon);
	m_logEntries.ClearAll();

	if (m_boldFont)
		DeleteObject(m_boldFont);

	if ( m_pStoreSelection )
	{
		delete m_pStoreSelection;
		m_pStoreSelection = NULL;
	}

	SafeTerminateThread();
	SafeTerminateAsyncDiffThread();

	if(m_AsyncDiffEvent)
		CloseHandle(m_AsyncDiffEvent);
}


BEGIN_MESSAGE_MAP(CGitLogListBase, CHintListCtrl)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
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
END_MESSAGE_MAP()

void CGitLogListBase::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	//if (m_nRowHeight>0)
	{
		lpMeasureItemStruct->itemHeight = 50;
	}
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
	SetWindowTheme(m_hWnd, L"Explorer", NULL);
	CHintListCtrl::PreSubclassWindow();
}

void CGitLogListBase::InsertGitColumn()
{
	CString temp;

	CRegDWORD regFullRowSelect(_T("Software\\TortoiseGit\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	SetExtendedStyle(exStyle);

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
	};
	m_dwDefaultColumns = GIT_LOG_GRAPH|GIT_LOG_ACTIONS|GIT_LOG_MESSAGE|GIT_LOG_AUTHOR|GIT_LOG_DATE;

	DWORD hideColumns = 0;
	if(this->m_IsRebaseReplaceGraph)
	{
		hideColumns |= GIT_LOG_GRAPH;
		m_dwDefaultColumns |= GIT_LOG_REBASE;
	}
	else
	{
		hideColumns |= GIT_LOG_REBASE;
	}

	if(this->m_IsIDReplaceAction)
	{
		hideColumns |= GIT_LOG_ACTIONS;
		m_dwDefaultColumns |= GIT_LOG_ID;
		m_dwDefaultColumns |= GIT_LOG_HASH;
	}
	else
	{
		hideColumns |= GIT_LOG_ID;
	}
	if(this->m_bShowBugtraqColumn)
	{
		m_dwDefaultColumns |= GIT_LOGLIST_BUG;
	}
	else
	{
		hideColumns |= GIT_LOGLIST_BUG;
	}
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
		for (int col = 0; col < numcols; col++)
		{
			// get width for this col last time from registry
			CString regentry;
			regentry.Format( _T("Software\\TortoiseGit\\%s\\ColWidth%d"),m_ColumnRegKey, col);
			CRegDWORD regwidth(regentry, 0);
			int cx = regwidth;
			if ( cx == 0 )
			{
				// no saved value, setup sensible defaults
				if (col == this->LOGLIST_MESSAGE)
				{
					cx = LOGLIST_MESSAGE_MIN;
				}
				else
				{
					cx = ICONITEMBORDER+16*4;
				}
			}
			if (cx < nMinimumWidth)
			{
				cx = nMinimumWidth;
			}
			else if (cx > nMaximumWidth)
			{
				cx = nMaximumWidth;
			}

			SetColumnWidth(col, cx);
		}
	}

}


BOOL CGitLogListBase::GetShortName(CString ref, CString &shortname,CString prefix)
{
	//TRACE(_T("%s %s\r\n"),ref,prefix);
	if(ref.Left(prefix.GetLength()) ==  prefix)
	{
		shortname = ref.Right(ref.GetLength()-prefix.GetLength());
		if(shortname.Right(3)==_T("^{}"))
			shortname=shortname.Left(shortname.GetLength()-3);
		return TRUE;
	}
	return FALSE;
}

void CGitLogListBase::FillBackGround(HDC hdc, int Index,CRect &rect)
{
	LVITEM rItem;
	SecureZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask  = LVIF_STATE;
	rItem.iItem = Index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	GitRev* pLogEntry = (GitRev*)m_arShownList.SafeGetAt(Index);
	HBRUSH brush = NULL;

	if (!(rItem.state & LVIS_SELECTED))
	{
		if(pLogEntry->GetAction(this)&CTGitPath::LOGACTIONS_REBASE_SQUASH)
			brush = ::CreateSolidBrush(RGB(156,156,156));
		else if(pLogEntry->GetAction(this)&CTGitPath::LOGACTIONS_REBASE_EDIT)
			brush = ::CreateSolidBrush(RGB(200,200,128));
	}
	else if (!(IsAppThemed() && SysInfo::Instance().IsVistaOrLater()))
	{
		if (rItem.state & LVIS_SELECTED)
		{
			if (::GetFocus() == m_hWnd)
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
			else
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
		}
	}

	if (brush != NULL)
	{
		::FillRect(hdc, &rect, brush);
		::DeleteObject(brush);
	}
}

void CGitLogListBase::DrawTagBranch(HDC hdc,CRect &rect,INT_PTR index)
{
	GitRev* data = (GitRev*)m_arShownList.SafeGetAt(index);
	CRect rt=rect;
	LVITEM rItem;
	SecureZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask  = LVIF_STATE;
	rItem.iItem = index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	CDC W_Dc;
	W_Dc.Attach(hdc);

	HTHEME hTheme = NULL;
	if (IsAppThemed() && SysInfo::Instance().IsVistaOrLater())
		hTheme = OpenThemeData(m_hWnd, L"Explorer::ListView;ListView");

	for(unsigned int i=0;i<m_HashMap[data->m_CommitHash].size();i++)
	{
		CString str;
		str=m_HashMap[data->m_CommitHash][i];

		CString shortname;
		HBRUSH brush = 0;
		shortname = _T("");
		COLORREF colRef = 0;

		//Determine label color
		if(GetShortName(str,shortname,_T("refs/heads/")))
		{
			if( shortname == m_CurrentBranch )
				colRef = m_Colors.GetColor(CColors::CurrentBranch);
			else
				colRef = m_Colors.GetColor(CColors::LocalBranch);

		}
		else if(GetShortName(str,shortname,_T("refs/remotes/")))
		{
			colRef = m_Colors.GetColor(CColors::RemoteBranch);
		}
		else if(GetShortName(str,shortname,_T("refs/tags/")))
		{
			colRef = m_Colors.GetColor(CColors::Tag);
		}
		else if(GetShortName(str,shortname,_T("refs/stash")))
		{
			colRef = m_Colors.GetColor(CColors::Stash);
			shortname=_T("stash");
		}
		else if(GetShortName(str,shortname,_T("refs/bisect/")))
		{
			if(shortname.Find(_T("good")) == 0)
			{
				colRef = m_Colors.GetColor(CColors::BisectGood);
				shortname = _T("good");
			}

			if(shortname.Find(_T("bad")) == 0)
			{
				colRef = m_Colors.GetColor(CColors::BisectBad);
				shortname = _T("bad");
			}
		}

		//When row selected, ajust label color
		if (!(IsAppThemed() && SysInfo::Instance().IsVistaOrLater()))
			if (rItem.state & LVIS_SELECTED)
				colRef = CColors::MixColors(colRef, ::GetSysColor(COLOR_HIGHLIGHT), 150);

		brush = ::CreateSolidBrush(colRef);

		if(!shortname.IsEmpty() && (rt.left<rect.right) )
		{
			SIZE size;
			memset(&size,0,sizeof(SIZE));
			GetTextExtentPoint32(hdc, shortname,shortname.GetLength(),&size);

			rt.SetRect(rt.left,rt.top,rt.left+size.cx,rt.bottom);
			rt.right+=8;

			int textpos = DT_CENTER;

			if(rt.right > rect.right)
			{
				rt.right = rect.right;
				textpos =0;
			}

			//Fill interior of ref label
			::FillRect(hdc, &rt, brush);

			//Draw edge of label

			CRect rectEdge = rt;

			W_Dc.Draw3dRect(rectEdge, m_Colors.Lighten(colRef,100), m_Colors.Darken(colRef,100));
			rectEdge.DeflateRect(1,1);
			W_Dc.Draw3dRect(rectEdge, m_Colors.Lighten(colRef,50), m_Colors.Darken(colRef,50));

			//Draw text inside label
			if (IsAppThemed() && SysInfo::Instance().IsVistaOrLater())
			{
				int txtState = LISS_NORMAL;
				if (rItem.state & LVIS_SELECTED)
					txtState = LISS_SELECTED;

				DrawThemeText(hTheme,hdc, LVP_LISTITEM, txtState, shortname, -1, textpos | DT_SINGLELINE | DT_VCENTER, 0, &rt);
			}
			else
			{
				W_Dc.SetBkMode(TRANSPARENT);
				if (rItem.state & LVIS_SELECTED)
				{
					COLORREF clrNew = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
					COLORREF clrOld = ::SetTextColor(hdc,clrNew);
					::DrawText(hdc,shortname,shortname.GetLength(),&rt,textpos | DT_SINGLELINE | DT_VCENTER);
					::SetTextColor(hdc,clrOld);
				}
				else
				{
					::DrawText(hdc,shortname,shortname.GetLength(),&rt,textpos | DT_SINGLELINE | DT_VCENTER);
				}
			}

			//::MoveToEx(hdc,rt.left,rt.top,NULL);
			//::LineTo(hdc,rt.right,rt.top);
			//::LineTo(hdc,rt.right,rt.bottom);
			//::LineTo(hdc,rt.left,rt.bottom);
			//::LineTo(hdc,rt.left,rt.top);

			rt.left=rt.right+1;
		}
		if(brush)
			::DeleteObject(brush);
	}
	rt.right=rect.right;

	if (IsAppThemed() && SysInfo::Instance().IsVistaOrLater())
	{
		int txtState = LISS_NORMAL;
		if (rItem.state & LVIS_SELECTED)
			txtState = LISS_SELECTED;

		DrawThemeText(hTheme,hdc, LVP_LISTITEM, txtState, data->GetSubject(), -1, DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER, 0, &rt);
	}
	else
	{
		if (rItem.state & LVIS_SELECTED)
		{
			COLORREF clrOld = ::SetTextColor(hdc,::GetSysColor(COLOR_HIGHLIGHTTEXT));
			::DrawText(hdc,data->GetSubject(),data->GetSubject().GetLength(),&rt,DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			::SetTextColor(hdc,clrOld);
		}
		else
		{
			::DrawText(hdc,data->GetSubject(),data->GetSubject().GetLength(),&rt,DT_NOPREFIX | DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		}
	}

	if (hTheme)
		CloseThemeData(hTheme);

	W_Dc.Detach();
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
	int r = (x2 - x1) / 3;
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


		Gdiplus::Pen mypen(&gradient,2);
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


		Gdiplus::Pen mypen(&gradient,2);
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

		Gdiplus::Pen mypen(&gradient,2);

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

	Gdiplus::Pen myPen(GetGdiColor(col),2);

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
	GitRev* data = (GitRev*)m_arShownList.SafeGetAt(index);
	if(data->m_CommitHash.IsEmpty())
		return;

	CRect rt=rect;
	LVITEM rItem;
	SecureZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask  = LVIF_STATE;
	rItem.iItem = index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

//	p->translate(QPoint(opt.rect.left(), opt.rect.top()));

	if (data->m_Lanes.size() == 0)
		m_logEntries.setLane(data->m_CommitHash);

	std::vector<int>& lanes=data->m_Lanes;
	UINT laneNum = lanes.size();
	UINT activeLane = 0;
	for (UINT i = 0; i < laneNum; i++)
		if (Lanes::isMerge(lanes[i])) {
			activeLane = i;
			break;
		}

	int x1 = 0, x2 = 0;
	int maxWidth = rect.Width();
	int lw = 3 * rect.Height() / 4; //laneWidth()

	COLORREF activeColor = m_LineColors[activeLane % Lanes::COLORS_NUM];
	//if (opt.state & QStyle::State_Selected)
	//	activeColor = blend(activeColor, opt.palette.highlightedText().color(), 208);

	for (unsigned int i = 0; i < laneNum && x2 < maxWidth; i++)
	{

		x1 = x2;
		x2 += lw;

		int ln = lanes[i];
		if (ln == Lanes::EMPTY)
			continue;

		COLORREF color = i == activeLane ? activeColor : m_LineColors[i % Lanes::COLORS_NUM];
		paintGraphLane(hdc, rect.Height(),ln, x1+rect.left, x2+rect.left, color,activeColor, rect.top);
	}

#if 0
	for (UINT i = 0; i < laneNum && x2 < maxWidth; i++) {

		x1 = x2;
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

			if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
			{
				GitRev* data = (GitRev*)m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
				if (data)
				{
					if (data->GetAction(this)& (CTGitPath::LOGACTIONS_REBASE_DONE| CTGitPath::LOGACTIONS_REBASE_SKIP) )
						crText = RGB(128,128,128);

					if(data->GetAction(this)&CTGitPath::LOGACTIONS_REBASE_SQUASH)
						pLVCD->clrTextBk = RGB(156,156,156);
					else if(data->GetAction(this)&CTGitPath::LOGACTIONS_REBASE_EDIT)
						pLVCD->clrTextBk  = RGB(200,200,128);
					else
						pLVCD->clrTextBk  = ::GetSysColor(COLOR_WINDOW);

					if(data->GetAction(this)&CTGitPath::LOGACTIONS_REBASE_CURRENT)
					{
						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					}

					if(data->m_CommitHash.ToString() == m_HeadHash)
					{
						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
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
			if (m_arShownList.GetCount() == (INT_PTR)pLVCD->nmcd.dwItemSpec)
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
			if ((m_bStrictStopped)&&(m_arShownList.GetCount() == (INT_PTR)pLVCD->nmcd.dwItemSpec))
			{
				pLVCD->nmcd.uItemState &= ~(CDIS_SELECTED|CDIS_FOCUS);
			}

			if (pLVCD->iSubItem == LOGLIST_GRAPH && m_sFilterText.IsEmpty())
			{
				if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec && (!this->m_IsRebaseReplaceGraph) )
				{
					CRect rect;
					GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_LABEL, rect);

					//TRACE(_T("A Graphic left %d right %d\r\n"),rect.left,rect.right);
					FillBackGround(pLVCD->nmcd.hdc, (INT_PTR)pLVCD->nmcd.dwItemSpec,rect);

					GitRev* data = (GitRev*)m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
					if( !data ->m_CommitHash.IsEmpty())
						DrawGraph(pLVCD->nmcd.hdc,rect,pLVCD->nmcd.dwItemSpec);

					*pResult = CDRF_SKIPDEFAULT;
					return;
				}
			}

			if (pLVCD->iSubItem == LOGLIST_MESSAGE)
			{
				if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
				{
					GitRev* data = (GitRev*)m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec);
					//if(!data->m_IsFull)
					//{
						//if(data->SafeFetchFullInfo(&g_Git))
						//	this->Invalidate();
						//TRACE(_T("Update ... %d\r\n"),pLVCD->nmcd.dwItemSpec);
					//}

					if(m_HashMap[data->m_CommitHash].size()!=0)
					{
						CRect rect;

						GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);

						FillBackGround(pLVCD->nmcd.hdc, (INT_PTR)pLVCD->nmcd.dwItemSpec,rect);
						DrawTagBranch(pLVCD->nmcd.hdc,rect,pLVCD->nmcd.dwItemSpec);

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

				if (m_arShownList.GetCount() <= (INT_PTR)pLVCD->nmcd.dwItemSpec)
					return;

				int		nIcons = 0;
				int		iconwidth = ::GetSystemMetrics(SM_CXSMICON);
				int		iconheight = ::GetSystemMetrics(SM_CYSMICON);

				GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_arShownList.SafeGetAt(pLVCD->nmcd.dwItemSpec));
				CRect rect;
				GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
				//TRACE(_T("Action left %d right %d\r\n"),rect.left,rect.right);
				// Get the selected state of the
				// item being drawn.

				// Fill the background if necessary
				FillBackGround(pLVCD->nmcd.hdc, (INT_PTR)pLVCD->nmcd.dwItemSpec,rect);

				// Draw the icon(s) into the compatible DC
				pLogEntry->GetAction(this);

				if (!pLogEntry->m_IsDiffFiles)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hFetchIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);

				if (pLogEntry->GetAction(this) & CTGitPath::LOGACTIONS_MODIFIED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->GetAction(this) & (CTGitPath::LOGACTIONS_ADDED|CTGitPath::LOGACTIONS_COPY) )
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->GetAction(this) & CTGitPath::LOGACTIONS_DELETED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->GetAction(this) & CTGitPath::LOGACTIONS_REPLACED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hReplacedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;
				*pResult = CDRF_SKIPDEFAULT;
				return;
			}
		}
		break;
	}
	*pResult = CDRF_DODEFAULT;
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
	lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);

	bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();

	*pResult = 0;
	if (m_bNoDispUpdates || bOutOfRange)
		return;

	// Which item number?
	int itemid = pItem->iItem;
	GitRev * pLogEntry = NULL;
	if (itemid < m_arShownList.GetCount())
		pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.SafeGetAt(pItem->iItem));

	CString temp;
	if(m_IsOldFirst)
	{
		temp.Format(_T("%d"),pItem->iItem+1);

	}
	else
	{
		temp.Format(_T("%d"),m_arShownList.GetCount()-pItem->iItem);
	}

	// Which column?
	switch (pItem->iSubItem)
	{
	case this->LOGLIST_GRAPH:	//Graphic
		break;
	case this->LOGLIST_REBASE:
		{
			if(this->m_IsRebaseReplaceGraph)
			{
				CTGitPath path;
				path.m_Action=pLogEntry->GetAction(this)&CTGitPath::LOGACTIONS_REBASE_MODE_MASK;
				lstrcpyn(pItem->pszText,path.GetActionName(), pItem->cchTextMax);
			}
		}
		break;
	case this->LOGLIST_ACTION: //action -- no text in the column
		break;
	case this->LOGLIST_HASH:
		if(pLogEntry)
			lstrcpyn(pItem->pszText, pLogEntry->m_CommitHash.ToString(), pItem->cchTextMax);
		break;
	case this->LOGLIST_ID:
		if(this->m_IsIDReplaceAction)
			lstrcpyn(pItem->pszText, temp, pItem->cchTextMax);
		break;
	case this->LOGLIST_MESSAGE: //Message
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetSubject(), pItem->cchTextMax);
		break;
	case this->LOGLIST_AUTHOR: //Author
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetAuthorName(), pItem->cchTextMax);
		break;
	case this->LOGLIST_DATE: //Date
		if ( pLogEntry && (!pLogEntry->m_CommitHash.IsEmpty()) )
			lstrcpyn(pItem->pszText,
				CLoglistUtils::FormatDateAndTime(pLogEntry->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes),
				pItem->cchTextMax);
		break;

	case this->LOGLIST_EMAIL:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetAuthorEmail(), pItem->cchTextMax);
		break;

	case this->LOGLIST_COMMIT_NAME: //Commit
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetCommitterName(), pItem->cchTextMax);
		break;

	case this->LOGLIST_COMMIT_EMAIL: //Commit Email
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetCommitterEmail(), pItem->cchTextMax);
		break;

	case this->LOGLIST_COMMIT_DATE: //Commit Date
		if (pLogEntry && (!pLogEntry->m_CommitHash.IsEmpty()))
			lstrcpyn(pItem->pszText,
				CLoglistUtils::FormatDateAndTime(pLogEntry->GetCommitterDate(), m_DateFormat, true, m_bRelativeTimes),
				pItem->cchTextMax);
		break;
	case this->LOGLIST_BUG: //Bug ID
		if(pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)this->m_ProjectProperties.FindBugID(pLogEntry->GetSubject() + _T("\r\n\r\n") + pLogEntry->GetBody()), pItem->cchTextMax);
		break;

	default:
		ASSERT(false);
	}
}

void CGitLogListBase::OnContextMenu(CWnd* pWnd, CPoint point)
{

	if (pWnd == GetHeaderCtrl())
	{
		return m_ColumnManager.OnContextMenuHeader(pWnd,point,!!IsGroupViewEnabled());
	}

	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return; // nothing selected, nothing to do with a context menu

	// if the user selected the info text telling about not all revisions shown due to
	// the "stop on copy/rename" option, we also don't show the context menu
	if ((m_bStrictStopped)&&(selIndex == m_arShownList.GetCount()))
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

	GitRev* pSelLogEntry = reinterpret_cast<GitRev*>(m_arShownList.SafeGetAt(indexNext));
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

	int FirstSelect=-1, LastSelect=-1;
	pos = GetFirstSelectedItemPosition();
	FirstSelect = GetNextSelectedItem(pos);
	while(pos)
	{
		LastSelect = GetNextSelectedItem(pos);
	}
	//entry is selected, now show the popup menu
	CIconMenu popup;
	CIconMenu subbranchmenu, submenu, gnudiffmenu,diffmenu;

	if (popup.CreatePopupMenu())
	{
		bool isHeadCommit = (pSelLogEntry->m_CommitHash == m_HeadHash);
		CString currentBranch = _T("refs/heads/") + g_Git.GetCurrentBranch();

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_PICK))
			popup.AppendMenuIcon(ID_REBASE_PICK, IDS_REBASE_PICK, IDI_PICK);

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_SQUASH))
			popup.AppendMenuIcon(ID_REBASE_SQUASH, IDS_REBASE_SQUASH, IDI_SQUASH);

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_EDIT))
			popup.AppendMenuIcon(ID_REBASE_EDIT, IDS_REBASE_EDIT, IDI_EDIT);

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_SKIP))
			popup.AppendMenuIcon(ID_REBASE_SKIP, IDS_REBASE_SKIP, IDI_SKIP);

		if(m_ContextMenuMask&(GetContextMenuBit(ID_REBASE_SKIP)|GetContextMenuBit(ID_REBASE_EDIT)|
			GetContextMenuBit(ID_REBASE_SQUASH)|GetContextMenuBit(ID_REBASE_PICK)))
			popup.AppendMenu(MF_SEPARATOR, NULL);

		if (GetSelectedCount() == 1)
		{

			{
				if( !pSelLogEntry->m_CommitHash.IsEmpty())
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_COMPARE) && m_hasWC) // compare revision with WC
						popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
				}
				else
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_COMMIT))
						popup.AppendMenuIcon(ID_COMMIT, IDS_LOG_POPUP_COMMIT, IDI_COMMIT);
				}
				if(m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF1) && m_hasWC) // compare with WC, unified
				{
					GitRev *pRev=pSelLogEntry;
					if(pSelLogEntry->m_ParentHash.size()==0)
					{
						pRev->GetParentFromHash(pRev->m_CommitHash);
					}
					if(pRev->m_ParentHash.size()<=1)
					{
						popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);

					}
					else
					{
						gnudiffmenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_GNUDIFF1,IDS_LOG_POPUP_GNUDIFF_PARENT, IDI_DIFF, gnudiffmenu.m_hMenu);

						gnudiffmenu.AppendMenuIcon(ID_GNUDIFF1 + (0xFFFF << 16), CString(MAKEINTRESOURCE(IDS_ALLPARENTS)));
						gnudiffmenu.AppendMenuIcon(ID_GNUDIFF1 + (0xFFFE << 16), CString(MAKEINTRESOURCE(IDS_ONLYMERGEDFILES)));

						for(int i=0;i<pRev->m_ParentHash.size();i++)
						{
							CString str;
							str.Format(IDS_PARENT, i + 1);
							gnudiffmenu.AppendMenuIcon(ID_GNUDIFF1+((i+1)<<16),str);
						}
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_COMPAREWITHPREVIOUS))
				{

					GitRev *pRev=pSelLogEntry;
					if(pSelLogEntry->m_ParentHash.size()==0)
					{
						pRev->GetParentFromHash(pRev->m_CommitHash);
					}
					if(pRev->m_ParentHash.size()<=1)
					{
						popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
						if (CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
							popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
					}
					else
					{
						diffmenu.CreatePopupMenu();
						popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF, diffmenu.m_hMenu);
						for(int i=0;i<pRev->m_ParentHash.size();i++)
						{
							CString str;
							str.Format(IDS_PARENT, i + 1);
							diffmenu.AppendMenuIcon(ID_COMPAREWITHPREVIOUS +((i+1)<<16),str);
							if (i == 0 && CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
							{
								popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
								diffmenu.SetDefaultItem(ID_COMPAREWITHPREVIOUS +((i+1)<<16), FALSE);
							}
						}
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_BLAME))
					popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);

				//popup.AppendMenuIcon(ID_BLAMEWITHPREVIOUS, IDS_LOG_POPUP_BLAMEWITHPREVIOUS, IDI_BLAME);
				popup.AppendMenu(MF_SEPARATOR, NULL);

				if (pSelLogEntry->m_CommitHash.IsEmpty())
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_STASH_SAVE))
						popup.AppendMenuIcon(ID_STASH_SAVE, IDS_MENUSTASHSAVE, IDI_COMMIT);

					if (CTGitPath(g_Git.m_CurrentDir).HasStashDir())
					{
						if(m_ContextMenuMask&GetContextMenuBit(ID_STASH_POP))
							popup.AppendMenuIcon(ID_STASH_POP, IDS_MENUSTASHPOP, IDI_RELOCATE);

						if(m_ContextMenuMask&GetContextMenuBit(ID_STASH_LIST))
							popup.AppendMenuIcon(ID_STASH_LIST, IDS_MENUSTASHLIST, IDI_LOG);
					}

					popup.AppendMenu(MF_SEPARATOR, NULL);

					if(m_ContextMenuMask&GetContextMenuBit(ID_FETCH))
						popup.AppendMenuIcon(ID_FETCH, IDS_MENUFETCH, IDI_PULL);

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

			CString str,format;
			//if (m_hasWC)
			//	popup.AppendMenuIcon(ID_REVERTTOREV, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);

			if(!pSelLogEntry->m_CommitHash.IsEmpty())
			{
				if((m_ContextMenuMask&GetContextMenuBit(ID_LOG)) &&
					GetSelectedCount() == 1)
						popup.AppendMenuIcon(ID_LOG, IDS_LOG_POPUP_LOG, IDI_LOG);

				format.LoadString(IDS_LOG_POPUP_MERGEREV);
				str.Format(format,g_Git.GetCurrentBranch());

				if (m_ContextMenuMask&GetContextMenuBit(ID_MERGEREV) && !isHeadCommit && m_hasWC)
					popup.AppendMenuIcon(ID_MERGEREV, str, IDI_MERGE);

				format.LoadString(IDS_RESET_TO_THIS_FORMAT);
				str.Format(format,g_Git.GetCurrentBranch());

				if(m_ContextMenuMask&GetContextMenuBit(ID_RESET) && m_hasWC)
					popup.AppendMenuIcon(ID_RESET,str,IDI_REVERT);


				// Add Switch Branch express Menu
				if( this->m_HashMap.find(pSelLogEntry->m_CommitHash) != m_HashMap.end()
					&& (m_ContextMenuMask&GetContextMenuBit(ID_SWITCHBRANCH) && m_hasWC)
					)
				{
					std::vector<CString *> branchs;
					for(int i=0;i<m_HashMap[pSelLogEntry->m_CommitHash].size();i++)
					{
						CString ref = m_HashMap[pSelLogEntry->m_CommitHash][i];
						if(ref.Find(_T("refs/heads/")) == 0 && ref != currentBranch)
						{
							branchs.push_back(&m_HashMap[pSelLogEntry->m_CommitHash][i]);
						}
					}

					CString str;
					str.LoadString(IDS_SWITCH_BRANCH);

					if(branchs.size() == 1)
					{
						str+=_T(" ");
						str+= _T('"') + branchs[0]->Mid(11) + _T('"');
						popup.AppendMenuIcon(ID_SWITCHBRANCH,str,IDI_SWITCH);

						popup.SetMenuItemData(ID_SWITCHBRANCH,(ULONG_PTR)branchs[0]);

					}
					else if(branchs.size() > 1)
					{
						subbranchmenu.CreatePopupMenu();
						for(int i=0;i<branchs.size();i++)
						{
							if (*branchs[i] != currentBranch)
							{
								subbranchmenu.AppendMenuIcon(ID_SWITCHBRANCH+(i<<16), branchs[i]->Mid(11));
								subbranchmenu.SetMenuItemData(ID_SWITCHBRANCH+(i<<16), (ULONG_PTR) branchs[i]);
							}
						}

						popup.AppendMenuIcon(ID_SWITCHBRANCH, str, IDI_SWITCH, subbranchmenu.m_hMenu);
					}
				}

				if(m_ContextMenuMask&GetContextMenuBit(ID_SWITCHTOREV) && !isHeadCommit && m_hasWC)
					popup.AppendMenuIcon(ID_SWITCHTOREV, IDS_SWITCH_TO_THIS , IDI_SWITCH);

				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_BRANCH))
					popup.AppendMenuIcon(ID_CREATE_BRANCH, IDS_CREATE_BRANCH_AT_THIS , IDI_COPY);

				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_TAG))
					popup.AppendMenuIcon(ID_CREATE_TAG,IDS_CREATE_TAG_AT_THIS , IDI_TAG);

				format.LoadString(IDS_REBASE_THIS_FORMAT);
				str.Format(format,g_Git.GetCurrentBranch());

				if(pSelLogEntry->m_CommitHash != m_HeadHash && m_hasWC)
					if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_TO_VERSION))
						popup.AppendMenuIcon(ID_REBASE_TO_VERSION, str , IDI_REBASE);

				if(m_ContextMenuMask&GetContextMenuBit(ID_EXPORT))
					popup.AppendMenuIcon(ID_EXPORT,IDS_EXPORT_TO_THIS, IDI_EXPORT);

				if (m_ContextMenuMask&GetContextMenuBit(ID_REVERTREV) && m_hasWC)
					popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);

				if (m_ContextMenuMask&GetContextMenuBit(ID_EDITNOTE))
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
				//popup.AppendMenuIcon(ID_BLAMETWO, IDS_LOG_POPUP_BLAMEREVS, IDI_BLAME);
				if(m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF2) && m_hasWC) // compare two revisions, unified
					popup.AppendMenuIcon(ID_GNUDIFF2, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
				bAddSeparator = true;
			}

			if (m_hasWC)
			{
				//popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREVS, IDI_REVERT);
//				if (m_hasWC)
//					popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREVS, IDI_MERGE);
				bAddSeparator = true;
			}

			if (m_ContextMenuMask&GetContextMenuBit(ID_REVERTREV) && m_hasWC)
					popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREVS, IDI_REVERT);

			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if ( GetSelectedCount() >0 && (!pSelLogEntry->m_CommitHash.IsEmpty()))
		{
			bool bAddSeparator = false;
			if ( IsSelectionContinuous() && GetSelectedCount() >= 2 )
			{
				if(m_ContextMenuMask&GetContextMenuBit(ID_COMBINE_COMMIT) && m_hasWC)
				{
					CString head;
					int headindex;
					headindex = this->GetHeadIndex();
					if(headindex>=0)
					{
						head.Format(_T("HEAD~%d"),LastSelect-headindex);
						CGitHash hash=g_Git.GetHash(head);
						GitRev* pLastEntry = reinterpret_cast<GitRev*>(m_arShownList.SafeGetAt(LastSelect));
						if(pLastEntry->m_CommitHash == hash) {
							popup.AppendMenuIcon(ID_COMBINE_COMMIT,IDS_COMBINE_TO_ONE,IDI_COMBINE);
							bAddSeparator = true;
						}
					}
				}
			}
			if(m_ContextMenuMask&GetContextMenuBit(ID_CHERRY_PICK) && !isHeadCommit && m_hasWC) {
				if (GetSelectedCount() >= 2)
					popup.AppendMenuIcon(ID_CHERRY_PICK, IDS_CHERRY_PICK_VERSIONS, IDI_EXPORT);
				else
					popup.AppendMenuIcon(ID_CHERRY_PICK, IDS_CHERRY_PICK_VERSION, IDI_EXPORT);
				bAddSeparator = true;
			}

			if(GetSelectedCount()<=2 || (IsSelectionContinuous() && GetSelectedCount() > 0))
				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_PATCH)) {
					popup.AppendMenuIcon(ID_CREATE_PATCH, IDS_CREATE_PATCH, IDI_PATCH);
					bAddSeparator = true;
				}

			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

#if 0
//		if ((selEntries.size() > 0)&&(bAllFromTheSameAuthor))
//		{
//			popup.AppendMenuIcon(ID_EDITAUTHOR, IDS_LOG_POPUP_EDITAUTHOR);
//		}
//		if (GetSelectedCount() == 1)
//		{
//			popup.AppendMenuIcon(ID_EDITLOG, IDS_LOG_POPUP_EDITLOG);
//			popup.AppendMenuIcon(ID_REVPROPS, IDS_REPOBROWSE_SHOWREVPROP, IDI_PROPERTIES); // "Show Revision Properties"
//			popup.AppendMenu(MF_SEPARATOR, NULL);
//		}
#endif

		if (GetSelectedCount() == 1)
		{
			bool bAddSeparator = false;
			if(m_ContextMenuMask&GetContextMenuBit(ID_PUSH) && m_HashMap[pSelLogEntry->m_CommitHash].size() >= 1)
			{
				// show the push-option only if the log entry has an associated local branch
				bool isLocal = false;
				for(int i=0; isLocal == false && i < m_HashMap[pSelLogEntry->m_CommitHash].size(); i++)
				{
					if (m_HashMap[pSelLogEntry->m_CommitHash][i].Find(_T("refs/heads/")) == 0)
						isLocal = true;
				}
				if (isLocal)
				{
					popup.AppendMenuIcon(ID_PUSH, IDS_LOG_PUSH, IDI_PUSH);
					bAddSeparator = true;
				}
			}

			if(m_ContextMenuMask &GetContextMenuBit(ID_DELETE))
			{
				if( this->m_HashMap.find(pSelLogEntry->m_CommitHash) != m_HashMap.end() )
				{
					std::vector<CString *> branchs;
					for (int i = 0; i < m_HashMap[pSelLogEntry->m_CommitHash].size(); i++)
					{
						if(m_HashMap[pSelLogEntry->m_CommitHash][i] != currentBranch)
							branchs.push_back(&m_HashMap[pSelLogEntry->m_CommitHash][i]);
					}
					CString str;
					if (branchs.size() == 1)
					{
						str.LoadString(IDS_DELETE_BRANCHTAG_SHORT);
						str+=_T(" ");
						str += *branchs[0];
						popup.AppendMenuIcon(ID_DELETE, str, IDI_DELETE);
						popup.SetMenuItemData(ID_DELETE, (ULONG_PTR)branchs[0]);
						bAddSeparator = true;
					}
					else if (branchs.size() > 1)
					{
						str.LoadString(IDS_DELETE_BRANCHTAG);
						submenu.CreatePopupMenu();
						for (int i = 0; i < branchs.size(); i++)
						{
							submenu.AppendMenuIcon(ID_DELETE + (i << 16), *branchs[i]);
							submenu.SetMenuItemData(ID_DELETE + (i << 16), (ULONG_PTR)branchs[i]);
						}

						popup.AppendMenuIcon(ID_DELETE,str, IDI_DELETE, submenu.m_hMenu);
						bAddSeparator = true;
					}
				}
			} // m_ContextMenuMask &GetContextMenuBit(ID_DELETE)
			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		} // GetSelectedCount() == 1

		if (GetSelectedCount() == 1)
		{
			if(m_ContextMenuMask&GetContextMenuBit(ID_COPYHASH))
				popup.AppendMenuIcon(ID_COPYHASH, IDS_COPY_COMMIT_HASH, IDI_COPYCLIP);
		}
		if (GetSelectedCount() != 0)
		{
			if(m_ContextMenuMask&GetContextMenuBit(ID_COPYCLIPBOARD))
				popup.AppendMenuIcon(ID_COPYCLIPBOARD, IDS_LOG_POPUP_COPYTOCLIPBOARD, IDI_COPYCLIP);
		}

		if(m_ContextMenuMask&GetContextMenuBit(ID_FINDENTRY))
			popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND, IDI_FILTEREDIT);

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
	bool bContinuous = (m_arShownList.GetCount() == (INT_PTR)m_logEntries.size());
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

void CGitLogListBase::CopySelectionToClipBoard(bool HashOnly)
{

	CString sClipdata;
	POSITION pos = GetFirstSelectedItemPosition();
	if (pos != NULL)
	{
		CString sRev;
		sRev.LoadString(IDS_LOG_REVISION);
		CString sAuthor;
		sAuthor.LoadString(IDS_LOG_AUTHOR);
		CString sDate;
		sDate.LoadString(IDS_LOG_DATE);
		CString sMessage;
		sMessage.LoadString(IDS_LOG_MESSAGE);
		while (pos)
		{
			CString sLogCopyText;
			CString sPaths;
			GitRev * pLogEntry = reinterpret_cast<GitRev *>(m_arShownList.SafeGetAt(GetNextSelectedItem(pos)));

			if(!HashOnly)
			{
				//pLogEntry->GetFiles(this)
				//LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;

				for (int cpPathIndex = 0; cpPathIndex<pLogEntry->GetFiles(this).GetCount(); ++cpPathIndex)
				{
					sPaths += ((CTGitPath&)pLogEntry->GetFiles(this)[cpPathIndex]).GetActionName() + _T(" : ") + pLogEntry->GetFiles(this)[cpPathIndex].GetGitPathString();
					sPaths += _T("\r\n");
				}
				sPaths.Trim();
				CString body = pLogEntry->GetBody();
				body.Replace(_T("\n"), _T("\r\n"));
				sLogCopyText.Format(_T("%s: %s\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
					(LPCTSTR)sRev, pLogEntry->m_CommitHash.ToString(),
					(LPCTSTR)sAuthor, (LPCTSTR)pLogEntry->GetAuthorName(),
					(LPCTSTR)sDate,
					(LPCTSTR)CLoglistUtils::FormatDateAndTime(pLogEntry->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes),
					(LPCTSTR)sMessage, pLogEntry->GetSubject().Trim() + _T("\r\n\r\n") + body.Trim(),
					(LPCTSTR)sPaths);
				sClipdata +=  sLogCopyText;
			}
			else
			{
				sClipdata += pLogEntry->m_CommitHash;
				break;
			}

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
	{
		LastSelect = GetNextSelectedItem(pos);
	}

	ContextMenuAction(ID_COMPAREWITHPREVIOUS,FirstSelect,LastSelect, NULL);

#if 0
	UpdateLogInfoLabel();
	int selIndex = m_LogList.GetSelectionMark();
	if (selIndex < 0)
		return;
	int selCount = m_LogList.GetSelectedCount();
	if (selCount != 1)
		return;

	// Find selected entry in the log list
	POSITION pos = m_LogList.GetFirstSelectedItemPosition();
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.SafeGetAt(m_LogList.GetNextSelectedItem(pos)));
	long rev1 = pLogEntry->Rev;
	long rev2 = rev1-1;
	CTGitPath path = m_path;

	// See how many files under the relative root were changed in selected revision
	int nChanged = 0;
	LogChangedPath * changed = NULL;
	for (INT_PTR c = 0; c < pLogEntry->pArChangedPaths->GetCount(); ++c)
	{
		LogChangedPath * cpath = pLogEntry->pArChangedPaths->SafeGetAt(c);
		if (cpath  &&  cpath -> sPath.Left(m_sRelativeRoot.GetLength()).Compare(m_sRelativeRoot)==0)
		{
			++nChanged;
			changed = cpath;
		}
	}

	if (m_path.IsDirectory() && nChanged == 1)
	{
		// We're looking at the log for a directory and only one file under dir was changed in the revision
		// Do diff on that file instead of whole directory
		path.AppendPathString(changed->sPath.Mid(m_sRelativeRoot.GetLength()));
	}

	m_bCancelled = FALSE;
	DialogEnableWindow(IDOK, FALSE);
	SetPromptApp(&theApp);
	theApp.DoWaitCursor(1);

	if (PromptShown())
	{
		GitDiff diff(this, m_hWnd, true);
		diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		diff.SetHEADPeg(m_LogRevision);
		diff.ShowCompare(path, rev2, path, rev1);
	}
	else
	{
		CAppUtils::StartShowCompare(m_hWnd, path, rev2, path, rev1, GitRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	}

	theApp.DoWaitCursor(-1);
	EnableOKButton();
#endif
}

void CGitLogListBase::OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVFINDITEM pFindInfo = reinterpret_cast<LPNMLVFINDITEM>(pNMHDR);
	*pResult = -1;

	if (pFindInfo->lvfi.flags & LVFI_PARAM)
		return;
	if ((pFindInfo->iStart < 0)||(pFindInfo->iStart >= m_arShownList.GetCount()))
		return;
	if (pFindInfo->lvfi.psz == 0)
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

int CGitLogListBase::FillGitLog(CTGitPath *path,int info,CString *from,CString *to)
{
	ClearText();


	this->m_arShownList.SafeRemoveAll();

	this->m_logEntries.ClearAll();
	this->m_logEntries.ParserFromLog(path,-1,info,from,to);

	//this->m_logEntries.ParserFromLog();
	SetItemCountEx(this->m_logEntries.size());

	for(unsigned int i=0;i<m_logEntries.size();i++)
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

	if(path)
		m_Path=*path;
	return 0;

}

int CGitLogListBase::BeginFetchLog()
{
	ClearText();

	this->m_arShownList.SafeRemoveAll();

	this->m_logEntries.ClearAll();
	git_init();

	this->m_LogCache.ClearAllParent();

	m_LogCache.FetchCacheIndex(g_Git.m_CurrentDir);

	CTGitPath *path;
	if(this->m_Path.IsEmpty())
		path=NULL;
	else
		path=&this->m_Path;

	CString hash;
	int mask;
	mask = CGit::LOG_INFO_ONLY_HASH | CGit::LOG_INFO_BOUNDARY;
//	if(this->m_bAllBranch)
	mask |= m_ShowMask ;

	if(m_bShowWC)
	{
		this->m_logEntries.insert(m_logEntries.begin(),this->m_wcRev.m_CommitHash);
		ResetWcRev();
		this->m_LogCache.m_HashMap[m_wcRev.m_CommitHash]=m_wcRev;
	}

	CString *pFrom, *pTo;
	pFrom = pTo = NULL;
	CString head(_T("HEAD"));
	if(!this->m_startrev.IsEmpty())
	{
		pFrom = &this->m_startrev;
		if(!this->m_endrev.IsEmpty())
			pTo = &this->m_endrev;
		else
			pTo = &head;
	}

	CFilterData data;
	data.m_From = m_From;
	data.m_To =m_To;

#if 0 /* use tortoiegit filter */
 	if(this->m_nSelectedFilter == LOGFILTER_ALL || m_nSelectedFilter == LOGFILTER_AUTHORS)
		data.m_Author = this->m_sFilterText;

	if(this->m_nSelectedFilter == LOGFILTER_ALL || m_nSelectedFilter == LOGFILTER_MESSAGES)
		data.m_MessageFilter = this->m_sFilterText;

	data.m_IsRegex = m_bFilterWithRegex;
#endif

	CString cmd=g_Git.GetLogCmd(m_StartRef,path,-1,mask,pFrom,pTo,true,&data);

	//this->m_logEntries.ParserFromLog();
	if(IsInWorkingThread())
	{
		PostMessage(LVM_SETITEMCOUNT, (WPARAM) this->m_logEntries.size(),(LPARAM) LVSICF_NOINVALIDATEALL);
	}
	else
	{
		SetItemCountEx(this->m_logEntries.size());
	}

	git_init();

	if(g_Git.IsInitRepos())
		return 0;

	if (git_open_log(&m_DllGitLog, CUnicodeUtils::GetMulti(cmd, CP_UTF8).GetBuffer()))
	{
		return -1;
	}

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
		{
			SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		}
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

int CGitLogListBase::FetchLogAsync(void * data)
{
	m_ProcData=data;
	m_bExitThread=FALSE;
	InterlockedExchange(&m_bThreadRunning, TRUE);
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	m_LoadingThread = AfxBeginThread(LogThreadEntry, this, THREAD_PRIORITY_LOWEST);
	if (m_LoadingThread ==NULL)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return -1;
	}
	return 0;
}

//this is the thread function which calls the subversion function
UINT CGitLogListBase::LogThreadEntry(LPVOID pVoid)
{
	return ((CGitLogListBase*)pVoid)->LogThread();
}

void CGitLogListBase::GetTimeRange(CTime &oldest, CTime &latest)
{
	//CTime time;
	oldest=CTime::GetCurrentTime();
	latest=CTime(1971,1,2,0,0,0);
	for(unsigned int i=0;i<m_logEntries.size();i++)
	{
		if(m_logEntries[i].IsEmpty())
			continue;

		if(m_logEntries.GetGitRevAt(i).GetAuthorDate().GetTime() < oldest.GetTime())
			oldest = m_logEntries.GetGitRevAt(i).GetAuthorDate().GetTime();

		if(m_logEntries.GetGitRevAt(i).GetAuthorDate().GetTime() > latest.GetTime())
			latest = m_logEntries.GetGitRevAt(i).GetAuthorDate().GetTime();

	}

	if(latest<oldest)
		latest=oldest;
}

UINT CGitLogListBase::LogThread()
{
	::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) GITLOG_START,0);

	InterlockedExchange(&m_bThreadRunning, TRUE);
	InterlockedExchange(&m_bNoDispUpdates, TRUE);

	ULONGLONG  t1,t2;

	if(BeginFetchLog())
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);

		return -1;
	}

	tr1::wregex pat;//(_T("Remove"), tr1::regex_constants::icase);
	bool bRegex = false;
	if (m_bFilterWithRegex)
		bRegex = ValidateRegexp(m_sFilterText, pat, false);

	TRACE(_T("\n===Begin===\n"));
	//Update work copy item;

	if( m_logEntries.size() > 0)
	{
		GitRev *pRev = &m_logEntries.GetGitRevAt(0);

		m_arShownList.SafeAdd(pRev);
	}


	InterlockedExchange(&m_bNoDispUpdates, FALSE);

	// store commit number of the last selected commit/line before the refresh or -1
	int lastSelectedHashNItem = -1;

	if(!g_Git.IsInitRepos())
	{
		g_Git.m_critGitDllSec.Lock();
		git_get_log_firstcommit(m_DllGitLog);
		int total = git_get_log_estimate_commit_count(m_DllGitLog);
		g_Git.m_critGitDllSec.Unlock();

		GIT_COMMIT commit;
		t2=t1=GetTickCount();
		int oldprecentage = 0;
		int oldsize=m_logEntries.size();
		int ret=0;
		while( ret== 0)
		{
			g_Git.m_critGitDllSec.Lock();
			ret = git_get_log_nextcommit(this->m_DllGitLog, &commit, 0);
			g_Git.m_critGitDllSec.Unlock();

			if(ret)
				break;

			//printf("%s\r\n",commit.GetSubject());
			if(m_bExitThread)
				break;

			CGitHash hash = (char*)commit.m_hash ;

			GitRev *pRev = m_LogCache.GetCacheData(hash);
			pRev->m_GitCommit = commit;
			InterlockedExchange(&pRev->m_IsCommitParsed, FALSE);

			char *note=NULL;
			g_Git.m_critGitDllSec.Lock();
			git_get_notes(commit.m_hash,&note);
			g_Git.m_critGitDllSec.Unlock();

			if(note)
			{
				pRev->m_Notes.Empty();
				g_Git.StringAppend(&pRev->m_Notes,(BYTE*)note);
			}

			if(!pRev->m_IsDiffFiles)
			{
				pRev->m_CallDiffAsync = DiffAsync;
			}

			pRev->ParserParentFromCommit(&commit);

#ifdef DEBUG
			pRev->DbgPrint();
			TRACE(_T("\n"));
#endif

			if(!m_sFilterText.IsEmpty())
			{
				if(!IsMatchFilter(bRegex,pRev,pat))
					continue;
			}
			this->m_critSec.Lock();
			m_logEntries.push_back(hash);
			m_arShownList.SafeAdd(pRev);
			this->m_critSec.Unlock();

			if (lastSelectedHashNItem == -1 && hash == m_lastSelectedHash)
				lastSelectedHashNItem = m_arShownList.GetCount() - 1;

			t2=GetTickCount();

			if(t2-t1>500 || (m_logEntries.size()-oldsize >100))
			{
				//update UI
				int percent=m_logEntries.size()*100/total + GITLOG_START+1;
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
				t1 = t2;
			}
		}
		g_Git.m_critGitDllSec.Lock();
		git_close_log(m_DllGitLog);
		g_Git.m_critGitDllSec.Unlock();

	}

	// restore last selected item
	if (lastSelectedHashNItem >= 0)
	{
		SetItemState(lastSelectedHashNItem, LVIS_SELECTED, LVIS_SELECTED);
		EnsureVisible(lastSelectedHashNItem, FALSE);
	}

	//Update UI;
	PostMessage(LVM_SETITEMCOUNT, (WPARAM) this->m_logEntries.size(),(LPARAM) LVSICF_NOINVALIDATEALL|LVSICF_NOSCROLL);
	::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) GITLOG_END,0);

	InterlockedExchange(&m_bThreadRunning, FALSE);

	return 0;
}

void CGitLogListBase::Refresh(BOOL IsCleanFilter)
{
	SafeTerminateThread();

	this->SetItemCountEx(0);
	this->Clear();

	ResetWcRev();

	//Update branch and Tag info
	ReloadHashMap();
	//Assume Thread have exited
	//if(!m_bThreadRunning)
	{
		m_logEntries.clear();

		if(IsCleanFilter)
		{
			m_sFilterText.Empty();
			m_From=-1;
			m_To=-1;
		}

		InterlockedExchange(&m_bExitThread,FALSE);

		InterlockedExchange(&m_bThreadRunning, TRUE);
		InterlockedExchange(&m_bNoDispUpdates, TRUE);
		if ( (m_LoadingThread=AfxBeginThread(LogThreadEntry, this)) ==NULL)
		{
			InterlockedExchange(&m_bThreadRunning, FALSE);
			InterlockedExchange(&m_bNoDispUpdates, FALSE);
			CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
	}
}
bool CGitLogListBase::ValidateRegexp(LPCTSTR regexp_str, tr1::wregex& pat, bool bMatchCase /* = false */)
{
	try
	{
		tr1::regex_constants::syntax_option_type type = tr1::regex_constants::ECMAScript;
		if (!bMatchCase)
			type |= tr1::regex_constants::icase;
		pat = tr1::wregex(regexp_str, type);
		return true;
	}
	catch (exception) {}
	return false;
}
BOOL CGitLogListBase::IsMatchFilter(bool bRegex, GitRev *pRev, tr1::wregex &pat)
{

	tr1::regex_constants::match_flag_type flags = tr1::regex_constants::match_any;
	CString sRev;

	if ((bRegex)&&(m_bFilterWithRegex))
	{
		if (m_SelectedFilters & LOGFILTER_BUGID)
		{
			if(this->m_bShowBugtraqColumn)
			{
				CString sBugIds = m_ProjectProperties.FindBugID(pRev->GetSubject() + _T("\r\n\r\n") + pRev->GetBody());

				ATLTRACE(_T("bugID = \"%s\"\n"), sBugIds);
				if (regex_search(wstring(sBugIds), pat, flags))
				{
					return TRUE;
				}
			}
		}

		if ((m_SelectedFilters & LOGFILTER_SUBJECT) || (m_SelectedFilters & LOGFILTER_MESSAGES))
		{
			ATLTRACE(_T("messge = \"%s\"\n"), pRev->GetSubject());
			if (regex_search(wstring((LPCTSTR)pRev->GetSubject()), pat, flags))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_MESSAGES)
		{
			ATLTRACE(_T("messge = \"%s\"\n"),pRev->GetBody());
			if (regex_search(wstring((LPCTSTR)pRev->GetBody()), pat, flags))
			{
					return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_AUTHORS)
		{
			if (regex_search(wstring(pRev->GetAuthorName()), pat, flags))
			{
				return TRUE;
			}

			if (regex_search(wstring(pRev->GetCommitterName()), pat, flags))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_REVS)
		{
			sRev.Format(_T("%s"), pRev->m_CommitHash.ToString());
			if (regex_search(wstring((LPCTSTR)sRev), pat, flags))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_PATHS)
		{
			CTGitPathList *pathList=NULL;
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
					if (regex_search(wstring((LPCTSTR)pathList->m_paths.at(cpPathIndex).GetGitOldPathString()), pat, flags))
					{
						return true;
					}
					if (regex_search(wstring((LPCTSTR)pathList->m_paths.at(cpPathIndex).GetGitPathString()), pat, flags))
					{
						return true;
					}
				}

			for(INT_PTR i=0;i<pRev->m_SimpleFileList.size();i++)
			{
				if (regex_search(wstring((LPCTSTR)pRev->m_SimpleFileList[i]), pat, flags))
				{
					return true;
				}
			}
		}
	}
	else
	{
		CString find = m_sFilterText;
		find.MakeLower();

		if (m_SelectedFilters & LOGFILTER_BUGID)
		{
			if(this->m_bShowBugtraqColumn)
			{
				CString sBugIds = m_ProjectProperties.FindBugID(pRev->GetSubject() + _T("\r\n\r\n") + pRev->GetBody());

				sBugIds.MakeLower();
				if ((sBugIds.Find(find) >= 0))
				{
					return TRUE;
				}
			}
		}

		if ((m_SelectedFilters & LOGFILTER_SUBJECT) || (m_SelectedFilters & LOGFILTER_MESSAGES))
		{
			CString msg = pRev->GetSubject();

			msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_MESSAGES)
		{
			CString msg = pRev->GetBody();

			msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_AUTHORS)
		{
			CString msg = pRev->GetAuthorName();
			msg = msg.MakeLower();
			if ((msg.Find(find) >= 0))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_REVS)
		{
			sRev.Format(_T("%s"), pRev->m_CommitHash.ToString());
			if ((sRev.Find(find) >= 0))
			{
				return TRUE;
			}
		}

		if (m_SelectedFilters & LOGFILTER_PATHS)
		{
			CTGitPathList *pathList=NULL;
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
					path.MakeLower();
					if ((path.Find(find)>=0))
					{
						return true;
					}
					path = cpath->GetGitPathString();
					path.MakeLower();
					if ((path.Find(find)>=0))
					{
						return true;
					}
				}

			for (INT_PTR i=0;i<pRev->m_SimpleFileList.size();i++)
			{
				CString path = pRev->m_SimpleFileList[i];
				path.MakeLower();
				if ((path.Find(find)>=0))
				{
					return true;
				}
			}
		}
	} // else (from if (bRegex))
	return FALSE;
}


void CGitLogListBase::RecalculateShownList(CThreadSafePtrArray * pShownlist)
{

	pShownlist->SafeRemoveAll();

	tr1::wregex pat;//(_T("Remove"), tr1::regex_constants::icase);
	bool bRegex = false;
	if (m_bFilterWithRegex)
		bRegex = ValidateRegexp(m_sFilterText, pat, false);

	tr1::regex_constants::match_flag_type flags = tr1::regex_constants::match_any;
	CString sRev;
	for (DWORD i=0; i<m_logEntries.size(); ++i)
	{
		if ((bRegex)&&(m_bFilterWithRegex))
		{
#if 0
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_BUGID))
			{
				ATLTRACE(_T("bugID = \"%s\"\n"), (LPCTSTR)m_logEntries[i]->sBugIDs);
				if (regex_search(wstring((LPCTSTR)m_logEntries[i]->sBugIDs), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(m_logEntries[i]);
					continue;
				}
			}
#endif
			if ((m_SelectedFilters & LOGFILTER_SUBJECT) || (m_SelectedFilters & LOGFILTER_MESSAGES))
			{
				ATLTRACE(_T("messge = \"%s\"\n"),m_logEntries.GetGitRevAt(i).GetSubject());
				if (regex_search(wstring((LPCTSTR)m_logEntries.GetGitRevAt(i).GetSubject()), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_MESSAGES)
			{
				ATLTRACE(_T("messge = \"%s\"\n"),m_logEntries.GetGitRevAt(i).GetBody());
				if (regex_search(wstring((LPCTSTR)m_logEntries.GetGitRevAt(i).GetBody()), pat, flags)&&IsEntryInDateRange(i))
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
					if (regex_search(wstring((LPCTSTR)cpath.GetGitOldPathString()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					if (regex_search(wstring((LPCTSTR)cpath.GetGitPathString()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					if (regex_search(wstring((LPCTSTR)cpath.GetActionName()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
				}
			}
			if (m_SelectedFilters & LOGFILTER_AUTHORS)
			{
				if (regex_search(wstring((LPCTSTR)m_logEntries.GetGitRevAt(i).GetAuthorName()), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_REVS)
			{
				sRev.Format(_T("%s"), m_logEntries.GetGitRevAt(i).m_CommitHash.ToString());
				if (regex_search(wstring((LPCTSTR)sRev), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
		} // if (bRegex)
		else
		{
			CString find = m_sFilterText;
			find.MakeLower();
#if 0
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_BUGID))
			{
				CString sBugIDs = m_logEntries[i]->sBugIDs;

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
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					path = cpath.GetGitPathString();
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
						bGoing = false;
						continue;
					}
					path = cpath.GetActionName();
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
				msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
				}
			}
			if (m_SelectedFilters & LOGFILTER_REVS)
			{
				sRev.Format(_T("%s"), m_logEntries.GetGitRevAt(i).m_CommitHash.ToString());
				if ((sRev.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->SafeAdd(&m_logEntries.GetGitRevAt(i));
					continue;
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
		{
			m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(m_logEntries.size()-i-1));
		}
		else
		{
			m_arShownList.SafeAdd(&m_logEntries.GetGitRevAt(i));
		}
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

		retry++;

		//if(CMessageBox::Show(NULL,_T("Cannot Save Log Cache to Disk. To retry click yes. To give up click no."),_T("TortoiseGit"),
		//					MB_YESNO) == IDNO)
		//					break;
	}

	CHintListCtrl::OnDestroy();
}

LRESULT CGitLogListBase::OnLoad(WPARAM wParam,LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
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

	for (int col = 0; col < maxcol; col++)
		if (m_ColumnManager.IsVisible (col))
			m_ColumnManager.ColumnResized (col);

	m_ColumnManager.WriteSettings();
}

int CGitLogListBase::GetHeadIndex()
{
	if(m_HeadHash.IsEmpty())
		return -1;

	for(int i=0;i<m_arShownList.GetCount();i++)
	{
		GitRev *pRev = (GitRev*)m_arShownList[i];
		if(pRev)
		{
			if(pRev->m_CommitHash.ToString() == m_HeadHash )
				return i;
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
LRESULT CGitLogListBase::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{

	ASSERT(m_pFindDialog != NULL);
	bool bFound = false;
	int i=0;

	if (m_pFindDialog->IsTerminating())
	{
		// invalidate the handle identifying the dialog box.
		m_pFindDialog = NULL;
		return 0;
	}

	if(m_pFindDialog->IsRef())
	{
		CString str;
		str=m_pFindDialog->GetFindString();

		CGitHash hash;

		if(!str.IsEmpty())
			hash = g_Git.GetHash(str);

		if(!hash.IsEmpty())
		{
			for (i = 0; i<m_arShownList.GetCount(); i++)
			{
				GitRev* pLogEntry = (GitRev*)m_arShownList.SafeGetAt(i);
				if(pLogEntry && pLogEntry->m_CommitHash == hash)
				{
					bFound = true;
					break;
				}
			}
		}

	}

	if(m_pFindDialog->FindNext())
	{
		//read data from dialog
		CString FindText = m_pFindDialog->GetFindString();
		bool bMatchCase = (m_pFindDialog->MatchCase() == TRUE);

		tr1::wregex pat;
		bool bRegex = ValidateRegexp(FindText, pat, bMatchCase);

		tr1::regex_constants::match_flag_type flags = tr1::regex_constants::match_not_null;


		for (i = this->m_nSearchIndex; i<m_arShownList.GetCount()&&!bFound; i++)
		{
			GitRev* pLogEntry = (GitRev*)m_arShownList.SafeGetAt(i);

			CString str;
			str+=pLogEntry->m_CommitHash.ToString();
			str+=_T("\n");

			for(int j=0;j<this->m_HashMap[pLogEntry->m_CommitHash].size();j++)
			{
				str+=m_HashMap[pLogEntry->m_CommitHash][j];
				str+=_T("\n");
			}

			str+=pLogEntry->GetAuthorEmail();
			str+=_T("\n");
			str+=pLogEntry->GetAuthorName();
			str+=_T("\n");
			str+=pLogEntry->GetBody();
			str+=_T("\n");
			str+=pLogEntry->GetCommitterEmail();
			str+=_T("\n");
			str+=pLogEntry->GetCommitterName();
			str+=_T("\n");
			str+=pLogEntry->GetSubject();
			str+=_T("\n");


			/*Because changed files list is loaded on demand when gui show,
			  files will empty when files have not fetched.

			  we can add it back by using one-way diff(with outnumber changed and rename detect.
			  here just need changed filename list. one-way is much quicker.
			*/
			if(pLogEntry->m_IsFull)
			{
				for(int i=0;i<pLogEntry->GetFiles(this).GetCount();i++)
				{
					str+=pLogEntry->GetFiles(this)[i].GetWinPath();
					str+=_T("\n");
					str+=pLogEntry->GetFiles(this)[i].GetGitOldPathString();
					str+=_T("\n");
				}
			}
			else
			{
				if(!pLogEntry->m_IsSimpleListReady)
					pLogEntry->SafeGetSimpleList(&g_Git);

				for(int i=0;i<pLogEntry->m_SimpleFileList.size();i++)
				{
					str+=pLogEntry->m_SimpleFileList[i];
					str+=_T("\n");
				}

			}


			if (bRegex)
			{
				if (regex_search(wstring(str), pat, flags))
				{
					bFound = true;
					break;
				}
			}
			else
			{
				if (bMatchCase)
				{
					if (str.Find(FindText) >= 0)
					{
						bFound = true;
						break;
					}

				}
				else
				{
					CString msg = str;
					msg = msg.MakeLower();
					CString find = FindText.MakeLower();
					if (msg.Find(find) >= 0)
					{
						bFound = TRUE;
						break;
					}
				}
			}
		} // for (i = this->m_nSearchIndex; i<m_arShownList.GetItemCount()&&!bFound; i++)

	} // if(m_pFindDialog->FindNext())
	//UpdateLogInfoLabel();

	if (bFound)
	{
		this->m_nSearchIndex = i;
		EnsureVisible(i, FALSE);
		SetItemState(GetSelectionMark(), 0, LVIS_SELECTED);
		SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
		SetSelectionMark(i);
		//FillLogMessageCtrl();
		UpdateData(FALSE);
		m_nSearchIndex++;
		if (m_nSearchIndex >= m_arShownList.GetCount())
			m_nSearchIndex = (int)m_arShownList.GetCount()-1;
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
