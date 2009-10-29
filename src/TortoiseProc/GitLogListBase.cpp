// GitLogList.cpp : implementation file
//
/*
	Description: qgit revision list view

	Author: Marco Costalba (C) 2005-2007

	Copyright: See COPYING file that comes with this distribution

*/
#include "stdafx.h"
#include "GitLogListBase.h"
#include "GitRev.h"
//#include "VssStyle.h"
#include "IconMenu.h"
// CGitLogListBase
#include "cursor.h"
#include "InputDlg.h"
#include "PropDlg.h"
#include "SVNProgressDlg.h"
#include "ProgressDlg.h"
//#include "RepositoryBrowser.h"
//#include "CopyDlg.h"
//#include "StatGraphDlg.h"
#include "Logdlg.h"
#include "MessageBox.h"
#include "Registry.h"
#include "AppUtils.h"
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


IMPLEMENT_DYNAMIC(CGitLogListBase, CHintListCtrl)

CGitLogListBase::CGitLogListBase():CHintListCtrl()
	,m_regMaxBugIDColWidth(_T("Software\\TortoiseGit\\MaxBugIDColWidth"), 200)
	,m_nSearchIndex(0)
	,m_bNoDispUpdates(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bStrictStopped(false)
	, m_pStoreSelection(NULL)
	, m_nSelectedFilter(LOGFILTER_ALL)
	, m_bVista(false)
	, m_bShowWC(false)
{
	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);
	
	m_bShowBugtraqColumn=0;

	m_IsIDReplaceAction=FALSE;

	m_wcRev.m_CommitHash=GIT_REV_ZERO;
	m_wcRev.m_Subject=_T("Working dir changes");
	m_wcRev.m_ParentHash.clear();
	m_wcRev.m_Mark=_T('-');
	m_wcRev.m_IsUpdateing=FALSE;
	m_wcRev.m_IsFull = TRUE;

	m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon    =  (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), TRUE);

	g_Git.GetMapHashToFriendName(m_HashMap);
	m_CurrentBranch=g_Git.GetCurrentBranch();
	this->m_HeadHash=g_Git.GetHash(CString(_T("HEAD"))).Left(40);

	m_From=CTime(1970,1,2,0,0,0);
	m_To=CTime::GetCurrentTime();
    m_ShowMask = 0;
	m_LoadingThread = NULL;

	m_bExitThread=FALSE;
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

	OSVERSIONINFOEX inf;
	SecureZeroMemory(&inf, sizeof(OSVERSIONINFOEX));
	inf.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	GetVersionEx((OSVERSIONINFO *)&inf);
	WORD fullver = MAKEWORD(inf.dwMinorVersion, inf.dwMajorVersion);
	m_bVista = (fullver >= 0x0600);

	m_ColumnRegKey=_T("log");
}

CGitLogListBase::~CGitLogListBase()
{
	InterlockedExchange(&m_bNoDispUpdates, TRUE);

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

	if(this->m_bThreadRunning)
	{
		m_bExitThread=true;
		WaitForSingleObject(m_LoadingThread->m_hThread,1000);
		TerminateThread();
	}
}


BEGIN_MESSAGE_MAP(CGitLogListBase, CHintListCtrl)
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
END_MESSAGE_MAP()

void CGitLogListBase::OnMeasureItem(int nIDCtl, LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CListCtrl::OnMeasureItem(nIDCtl, lpMeasureItemStruct);
}

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
	m_Theme.Open(m_hWnd, L"Explorer::ListView;ListView");
	m_Theme.SetWindowTheme(m_hWnd, L"Explorer", NULL);
	CHintListCtrl::PreSubclassWindow();
}

void CGitLogListBase::InsertGitColumn()
{
	CString temp;

	int c = GetHeaderCtrl()->GetItemCount()-1;
	
	while (c>=0)
		DeleteColumn(c--);
	temp.LoadString(IDS_LOG_GRAPH);

	if(m_IsRebaseReplaceGraph)
	{
		temp=_T("Rebase");
	}
	else
	{
		temp.LoadString(IDS_LOG_GRAPH);
	}
	InsertColumn(this->LOGLIST_GRAPH, temp);
	
#if 0	
	// make the revision column right aligned
	LVCOLUMN Column;
	Column.mask = LVCF_FMT;
	Column.fmt = LVCFMT_RIGHT;
	SetColumn(0, &Column); 
#endif	
//	CString log;
//	g_Git.GetLog(log);

	if(m_IsIDReplaceAction)
	{
		temp.LoadString(IDS_LOG_ID);
		InsertColumn(this->LOGLIST_ACTION, temp);
	}
	else
	{
		temp.LoadString(IDS_LOG_ACTIONS);
		InsertColumn(this->LOGLIST_ACTION, temp);
	}
	temp.LoadString(IDS_LOG_MESSAGE);
	InsertColumn(this->LOGLIST_MESSAGE, temp);
	
	temp.LoadString(IDS_LOG_AUTHOR);
	InsertColumn(this->LOGLIST_AUTHOR, temp);
	
	temp.LoadString(IDS_LOG_DATE);
	InsertColumn(this->LOGLIST_DATE, temp);
	

	if (m_bShowBugtraqColumn)
	{
//		temp = m_ProjectProperties.sLabel;
		if (temp.IsEmpty())
			temp.LoadString(IDS_LOG_BUGIDS);
		InsertColumn(this->LOGLIST_BUG, temp);

	}
	
	SetRedraw(false);
	ResizeAllListCtrlCols();
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
			} else if (cx > nMaximumWidth)
			{
				cx = nMaximumWidth;
			}

			SetColumnWidth(col, cx);
		}
	}

}


BOOL CGitLogListBase::GetShortName(CString ref, CString &shortname,CString prefix)
{
	TRACE(_T("%s %s\r\n"),ref,prefix);
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
//	HBRUSH brush;
	LVITEM   rItem;
	SecureZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask  = LVIF_STATE;
	rItem.iItem = Index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	GitRev* pLogEntry = (GitRev*)m_arShownList.GetAt(Index);
	HBRUSH brush = NULL;

	
	if (m_Theme.IsAppThemed() && m_bVista)
	{
		int state = LISS_NORMAL;
		if (rItem.state & LVIS_SELECTED)
		{
			if (::GetFocus() == m_hWnd)
				state |= LISS_SELECTED;
			else
				state |= LISS_SELECTEDNOTFOCUS;
		}
		else
		{
			if(pLogEntry->m_Action&CTGitPath::LOGACTIONS_REBASE_SQUASH)
				brush = ::CreateSolidBrush(RGB(156,156,156));
			else if(pLogEntry->m_Action&CTGitPath::LOGACTIONS_REBASE_EDIT)
				brush = ::CreateSolidBrush(RGB(200,200,128));
		}

		if (brush != NULL)
		{
			::FillRect(hdc, &rect, brush);
			::DeleteObject(brush);
		}
		else
		{
			if (m_Theme.IsBackgroundPartiallyTransparent(LVP_LISTITEM, state))
				m_Theme.DrawParentBackground(m_hWnd, hdc, &rect);

			CRect rectDraw = rect;
			if(rItem.state & LVIS_SELECTED)
				rectDraw.InflateRect(1,0);
			else
				rectDraw.InflateRect(1,1);

			m_Theme.DrawBackground(hdc, LVP_LISTITEM, state, rectDraw, &rect);
		}
	}
	else
	{
		
		if (rItem.state & LVIS_SELECTED)
		{
			if (::GetFocus() == m_hWnd)
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_HIGHLIGHT));
			else
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_BTNFACE));
		}
		else
		{
			//if (pLogEntry->bCopiedSelf)
			//	brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
			//else
			if(pLogEntry->m_Action&CTGitPath::LOGACTIONS_REBASE_SQUASH)
				brush = ::CreateSolidBrush(RGB(156,156,156));
			else if(pLogEntry->m_Action&CTGitPath::LOGACTIONS_REBASE_EDIT)
				brush = ::CreateSolidBrush(RGB(200,200,128));
			else
				brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
		}
		if (brush == NULL)
			return;

		::FillRect(hdc, &rect, brush);
		::DeleteObject(brush);
		
	}
}

void CGitLogListBase::DrawTagBranch(HDC hdc,CRect &rect,INT_PTR index)
{
	GitRev* data = (GitRev*)m_arShownList.GetAt(index);
	CRect rt=rect;
	LVITEM   rItem;
	SecureZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask  = LVIF_STATE;
	rItem.iItem = index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

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

		}else if(GetShortName(str,shortname,_T("refs/remotes/")))
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

		//When row selected, ajust label color
		if (!(m_Theme.IsAppThemed() && m_bVista))
			if (rItem.state & LVIS_SELECTED)
				colRef = CColors::MixColors(colRef, ::GetSysColor(COLOR_HIGHLIGHT), 150);

		brush = ::CreateSolidBrush(colRef);
		

		if(!shortname.IsEmpty())
		{
			SIZE size;
			memset(&size,0,sizeof(SIZE));
			GetTextExtentPoint32(hdc, shortname,shortname.GetLength(),&size);
		
			rt.SetRect(rt.left,rt.top,rt.left+size.cx,rt.bottom);
			rt.right+=8;

			//Fill interior of ref label
			::FillRect(hdc, &rt, brush);

			//Draw edge of label
			CDC W_Dc;
			W_Dc.Attach(hdc);

			CRect rectEdge = rt;

			W_Dc.Draw3dRect(rectEdge, m_Colors.Lighten(colRef,100), m_Colors.Darken(colRef,100));
			rectEdge.DeflateRect(1,1);
			W_Dc.Draw3dRect(rectEdge, m_Colors.Lighten(colRef,50), m_Colors.Darken(colRef,50));

			W_Dc.Detach();

			//Draw text inside label
			if (m_Theme.IsAppThemed() && m_bVista)
			{
				int txtState = LISS_NORMAL;
				if (rItem.state & LVIS_SELECTED)
					txtState = LISS_SELECTED;

				m_Theme.DrawText(hdc, LVP_LISTITEM, txtState, shortname, -1, DT_CENTER | DT_SINGLELINE | DT_VCENTER, 0, &rt);
			}
			else
			{
				if (rItem.state & LVIS_SELECTED)
				{
					COLORREF clrNew = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
					COLORREF   clrOld   = ::SetTextColor(hdc,clrNew);   
					::DrawText(hdc,shortname,shortname.GetLength(),&rt,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
					::SetTextColor(hdc,clrOld);
				}else
				{
					::DrawText(hdc,shortname,shortname.GetLength(),&rt,DT_CENTER | DT_SINGLELINE | DT_VCENTER);
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

	if (m_Theme.IsAppThemed() && m_bVista)
	{
		int txtState = LISS_NORMAL;
		if (rItem.state & LVIS_SELECTED)
			txtState = LISS_SELECTED;

		m_Theme.DrawText(hdc, LVP_LISTITEM, txtState, data->m_Subject, -1, DT_LEFT | DT_SINGLELINE | DT_VCENTER, 0, &rt);
	}
	else
	{
		if (rItem.state & LVIS_SELECTED)
		{
			COLORREF   clrOld   = ::SetTextColor(hdc,::GetSysColor(COLOR_HIGHLIGHTTEXT));   
			::DrawText(hdc,data->m_Subject,data->m_Subject.GetLength(),&rt,DT_LEFT | DT_SINGLELINE | DT_VCENTER);
			::SetTextColor(hdc,clrOld);   
		}else
		{
			::DrawText(hdc,data->m_Subject,data->m_Subject.GetLength(),&rt,DT_LEFT | DT_SINGLELINE | DT_VCENTER);
		}
	}
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

	#define P_CENTER m , h+top
	#define P_0      x2, h+top
	#define P_90     m , 0+top
	#define P_180    x1, h+top
	#define P_270    m , 2 * h+top
	#define R_CENTER m - r, h - r+top, m - r+d, h - r+top+d


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
		graphics.DrawArc(&mypen,x1-(x2-x1)/2-1,top+h-1, x2-x1+1,laneHeight+1,270,90);
		//graphics.DrawLine(&mypen,x1-1,h+top,P_270);

		break;
	}
	case Lanes::JOIN_L: 
	{
	
		Gdiplus::Pen mypen(Gdiplus::Color(0,0,0),2);

		
		graphics.DrawArc(&mypen,x1,top+h, x2-x1,laneHeight,270,90);

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

		graphics.DrawArc(&mypen,x1-(x2-x1)/2-1,top-h-1, x2-x1+1,laneHeight+1,0,90);


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

	CBrush brush;
	brush.CreateSolidBrush(col);
	HBRUSH oldbrush=(HBRUSH)::SelectObject(hdc,(HBRUSH)brush);
	// center symbol, e.g. rect or ellipse
	switch (type) {
	case Lanes::ACTIVE:
	case Lanes::INITIAL:
	case Lanes::BRANCH:

		//p->setPen(Qt::NoPen);
		//p->setBrush(col);
		::Ellipse(hdc, R_CENTER);
		//p->drawEllipse(R_CENTER);
		break;
	case Lanes::MERGE_FORK:
	case Lanes::MERGE_FORK_R:
	case Lanes::MERGE_FORK_L:
		//p->setPen(Qt::NoPen);
		//p->setBrush(col);
		//p->drawRect(R_CENTER);
		Rectangle(hdc,R_CENTER);
		break;
	case Lanes::UNAPPLIED:
		// Red minus sign
		//p->setPen(Qt::NoPen);
		//p->setBrush(Qt::red);
		//p->drawRect(m - r, h - 1, d, 2);
		::Rectangle(hdc,m-r,h-1,d,2);
		break;
	case Lanes::APPLIED:
		// Green plus sign
		//p->setPen(Qt::NoPen);
		//p->setBrush(DARK_GREEN);
		//p->drawRect(m - r, h - 1, d, 2);
		//p->drawRect(m - 1, h - r, 2, d);
		::Rectangle(hdc,m-r,h-1,d,2);
		::Rectangle(hdc,m-1,h-r,2,d);
		break;
	case Lanes::BOUNDARY:
		//p->setBrush(back);
		//p->drawEllipse(R_CENTER);
		::Ellipse(hdc, R_CENTER);
		break;
	case Lanes::BOUNDARY_C:
	case Lanes::BOUNDARY_R:
	case Lanes::BOUNDARY_L:
		//p->setBrush(back);
		//p->drawRect(R_CENTER);
		::Rectangle(hdc,R_CENTER);
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
	//todo unfinished
//	return;
	GitRev* data = (GitRev*)m_arShownList.GetAt(index);
	if(data->m_CommitHash.IsEmpty())
		return;

	CRect rt=rect;
	LVITEM   rItem;
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

		if (ln == Lanes::CROSS) {
			paintGraphLane(hdc, rect.Height(),Lanes::NOT_ACTIVE, x1, x2, m_LineColors[col % Lanes::COLORS_NUM],rect.top);
			paintGraphLane(hdc, rect.Height(),Lanes::CROSS, x1, x2, m_LineColors[activeLane % Lanes::COLORS_NUM],rect.top);
		} else
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
				GitRev* data = (GitRev*)m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec);
				if (data)
				{
#if 0
					if (data->bCopiedSelf)
					{
						// only change the background color if the item is not 'hot' (on vista with m_Themes enabled)
						if (!m_Theme.IsAppm_Themed() || !m_bVista || ((pLVCD->nmcd.uItemState & CDIS_HOT)==0))
							pLVCD->clrTextBk = GetSysColor(COLOR_MENU);
					}

					if (data->bCopies)
						crText = m_Colors.GetColor(CColors::Modified);
#endif
					if (data->m_Action& (CTGitPath::LOGACTIONS_REBASE_DONE| CTGitPath::LOGACTIONS_REBASE_SKIP) ) 
						crText = RGB(128,128,128);

					if(data->m_Action&CTGitPath::LOGACTIONS_REBASE_SQUASH)
						pLVCD->clrTextBk = RGB(156,156,156);
					else if(data->m_Action&CTGitPath::LOGACTIONS_REBASE_EDIT)
						pLVCD->clrTextBk  = RGB(200,200,128);
					else
						pLVCD->clrTextBk  = ::GetSysColor(COLOR_WINDOW);

					if(data->m_Action&CTGitPath::LOGACTIONS_REBASE_CURRENT)
					{
						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					}

					if(data->m_CommitHash == m_HeadHash)
					{
						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
					}

//					if ((data->childStackDepth)||(m_mergedRevs.find(data->Rev) != m_mergedRevs.end()))
//						crText = GetSysColor(COLOR_GRAYTEXT);
//					
					if (data->m_CommitHash == GIT_REV_ZERO)
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

			if (pLVCD->iSubItem == LOGLIST_GRAPH)
			{
				if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec && (!this->m_IsRebaseReplaceGraph) )
				{
					CRect rect;
					GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
					if(pLVCD->iSubItem == 0)
					{
						CRect second;
						GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem+1, LVIR_BOUNDS, second);
						rect.right=second.left;
					}
					
					TRACE(_T("A Graphic left %d right %d\r\n"),rect.left,rect.right);
					FillBackGround(pLVCD->nmcd.hdc, (INT_PTR)pLVCD->nmcd.dwItemSpec,rect);
					
					GitRev* data = (GitRev*)m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec);
					if( data ->m_CommitHash != GIT_REV_ZERO)
						DrawGraph(pLVCD->nmcd.hdc,rect,pLVCD->nmcd.dwItemSpec);

					*pResult = CDRF_SKIPDEFAULT;
					return;
				
				}
			}

			if (pLVCD->iSubItem == LOGLIST_MESSAGE)
			{
				if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
				{
					GitRev* data = (GitRev*)m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec);
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
			
			if (pLVCD->iSubItem == 1)
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

				GitRev* pLogEntry = reinterpret_cast<GitRev *>(m_arShownList.GetAt(pLVCD->nmcd.dwItemSpec));
				CRect rect;
				GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
				TRACE(_T("Action left %d right %d\r\n"),rect.left,rect.right);
				// Get the selected state of the
				// item being drawn.							

				// Fill the background
				FillBackGround(pLVCD->nmcd.hdc, (INT_PTR)pLVCD->nmcd.dwItemSpec,rect);
				
				// Draw the icon(s) into the compatible DC
				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_MODIFIED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->m_Action & (CTGitPath::LOGACTIONS_ADDED|CTGitPath::LOGACTIONS_COPY) )
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hAddedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_DELETED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left+nIcons*iconwidth + ICONITEMBORDER, rect.top, m_hDeletedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
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
		pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(pItem->iItem));

	CString temp;
	if(m_IsOldFirst)
	{
		temp.Format(_T("%d"),pItem->iItem+1);

	}else
	{
		temp.Format(_T("%d"),m_arShownList.GetCount()-pItem->iItem);
	}
	    
	// Which column?
	switch (pItem->iSubItem)
	{
	case this->LOGLIST_GRAPH:	//Graphic
		if (pLogEntry)
		{
			if(this->m_IsRebaseReplaceGraph)
			{
				CTGitPath path;
				path.m_Action=pLogEntry->m_Action&CTGitPath::LOGACTIONS_REBASE_MODE_MASK;

				lstrcpyn(pItem->pszText,path.GetActionName(), pItem->cchTextMax);
			}
		}
		break;
	case this->LOGLIST_ACTION: //action -- no text in the column
		if(this->m_IsIDReplaceAction)
			lstrcpyn(pItem->pszText, temp, pItem->cchTextMax);
		break;
	case this->LOGLIST_MESSAGE: //Message
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->m_Subject, pItem->cchTextMax);
		break;
	case this->LOGLIST_AUTHOR: //Author
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->m_AuthorName, pItem->cchTextMax);
		break;
	case this->LOGLIST_DATE: //Date
		if (pLogEntry && pLogEntry->m_CommitHash != GIT_REV_ZERO)
			lstrcpyn(pItem->pszText,
				CAppUtils::FormatDateAndTime( pLogEntry->m_AuthorDate, m_DateFormat, true, m_bRelativeTimes ), 
				pItem->cchTextMax);
		break;
		
	case 5:

		break;
	default:
		ASSERT(false);
	}
}

void CGitLogListBase::OnContextMenu(CWnd* pWnd, CPoint point)
{

	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return;	// nothing selected, nothing to do with a context menu

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

	GitRev* pSelLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(indexNext));
#if 0
	GitRev revSelected = pSelLogEntry->Rev;
	GitRev revPrevious = git_revnum_t(revSelected)-1;
	if ((pSelLogEntry->pArChangedPaths)&&(pSelLogEntry->pArChangedPaths->GetCount() <= 2))
	{
		for (int i=0; i<pSelLogEntry->pArChangedPaths->GetCount(); ++i)
		{
			LogChangedPath * changedpath = (LogChangedPath *)pSelLogEntry->pArChangedPaths->GetAt(i);
			if (changedpath->lCopyFromRev)
				revPrevious = changedpath->lCopyFromRev;
		}
	}
	GitRev revSelected2;
	if (pos)
	{
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(GetNextSelectedItem(pos)));
		revSelected2 = pLogEntry->Rev;
	}
	bool bAllFromTheSameAuthor = true;
	CString firstAuthor;
	CLogDataVector selEntries;
	GitRev revLowest, revHighest;
	GitRevRangeArray revisionRanges;
	{
		POSITION pos = GetFirstSelectedItemPosition();
		PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(GetNextSelectedItem(pos)));
		revisionRanges.AddRevision(pLogEntry->Rev);
		selEntries.push_back(pLogEntry);
		firstAuthor = pLogEntry->sAuthor;
		revLowest = pLogEntry->Rev;
		revHighest = pLogEntry->Rev;
		while (pos)
		{
			pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(GetNextSelectedItem(pos)));
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
	CIconMenu submenu;
	if (popup.CreatePopupMenu())
	{

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_PICK))
			popup.AppendMenuIcon(ID_REBASE_PICK,  IDS_REBASE_PICK,   IDI_PICK);

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_SQUASH))
			popup.AppendMenuIcon(ID_REBASE_SQUASH,IDS_REBASE_SQUASH, IDI_SQUASH);

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_EDIT))
			popup.AppendMenuIcon(ID_REBASE_EDIT,  IDS_REBASE_EDIT,   IDI_EDIT);

		if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_SKIP))
			popup.AppendMenuIcon(ID_REBASE_SKIP,  IDS_REBASE_SKIP,   IDI_SKIP);
		
		if(m_ContextMenuMask&(GetContextMenuBit(ID_REBASE_SKIP)|GetContextMenuBit(ID_REBASE_EDIT)|
			      GetContextMenuBit(ID_REBASE_SQUASH)|GetContextMenuBit(ID_REBASE_PICK)))
			popup.AppendMenu(MF_SEPARATOR, NULL);

		if (GetSelectedCount() == 1)
		{
			
			{
				if(pSelLogEntry->m_CommitHash != GIT_REV_ZERO)
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_COMPARE))
						popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
					// TODO:
					// TortoiseMerge could be improved to take a /blame switch
					// and then not 'cat' the files from a unified diff but
					// blame then.
					// But until that's implemented, the context menu entry for
					// this feature is commented out.
					//popup.AppendMenu(ID_BLAMECOMPARE, IDS_LOG_POPUP_BLAMECOMPARE, IDI_BLAME);
				}else
				{
					if(m_ContextMenuMask&GetContextMenuBit(ID_COMMIT))
						popup.AppendMenuIcon(ID_COMMIT, IDS_LOG_POPUP_COMMIT, IDI_COMMIT);
				}
				if(m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF1))
					popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);

				if(m_ContextMenuMask&GetContextMenuBit(ID_COMPAREWITHPREVIOUS))
					popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
				//popup.AppendMenuIcon(ID_BLAMEWITHPREVIOUS, IDS_LOG_POPUP_BLAMEWITHPREVIOUS, IDI_BLAME);
				popup.AppendMenu(MF_SEPARATOR, NULL);
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

			//if (m_hasWC)
			//	popup.AppendMenuIcon(ID_REVERTTOREV, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);
			//if (m_hasWC)
			//	popup.AppendMenuIcon(ID_REVERTREV, IDS_LOG_POPUP_REVERTREV, IDI_REVERT);
			//if (m_hasWC)
			//	popup.AppendMenuIcon(ID_MERGEREV, IDS_LOG_POPUP_MERGEREV, IDI_MERGE);
			
			CString str,format;
			format.LoadString(IDS_RESET_TO_THIS_FORMAT);
			str.Format(format,g_Git.GetCurrentBranch());

			if(pSelLogEntry->m_CommitHash != GIT_REV_ZERO)
			{
				if(m_ContextMenuMask&GetContextMenuBit(ID_RESET))
					popup.AppendMenuIcon(ID_RESET,str,IDI_REVERT);

				if(m_ContextMenuMask&GetContextMenuBit(ID_SWITCHTOREV))
					popup.AppendMenuIcon(ID_SWITCHTOREV, IDS_SWITCH_TO_THIS , IDI_SWITCH);

				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_BRANCH))
					popup.AppendMenuIcon(ID_CREATE_BRANCH, IDS_CREATE_BRANCH_AT_THIS , IDI_COPY);

				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_TAG))
					popup.AppendMenuIcon(ID_CREATE_TAG,IDS_CREATE_TAG_AT_THIS , IDI_COPY);
			
				format.LoadString(IDS_REBASE_THIS_FORMAT);
				str.Format(format,g_Git.GetCurrentBranch());

				if(pSelLogEntry->m_CommitHash != m_HeadHash)
					if(m_ContextMenuMask&GetContextMenuBit(ID_REBASE_TO_VERSION))
						popup.AppendMenuIcon(ID_REBASE_TO_VERSION, str , IDI_REBASE);			

				if(m_ContextMenuMask&GetContextMenuBit(ID_EXPORT))
					popup.AppendMenuIcon(ID_EXPORT,IDS_EXPORT_TO_THIS, IDI_EXPORT);	
			

				popup.AppendMenu(MF_SEPARATOR, NULL);
			}

		}

		if(!pSelLogEntry->m_Ref.IsEmpty() && GetSelectedCount() == 1)
		{
			popup.AppendMenuIcon(ID_REFLOG_DEL, IDS_REFLOG_DEL,     IDI_DELETE);	
			popup.AppendMenuIcon(ID_STASH_APPLY,IDS_MENUSTASHAPPLY, IDI_RELOCATE);	
			popup.AppendMenu(MF_SEPARATOR, NULL);
		}
	
		if (GetSelectedCount() >= 2)
		{
			bool bAddSeparator = false;
			if (IsSelectionContinuous() || (GetSelectedCount() == 2))
			{
				if(m_ContextMenuMask&GetContextMenuBit(ID_COMPARETWO))
					popup.AppendMenuIcon(ID_COMPARETWO, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
			}

			if (GetSelectedCount() == 2)
			{
				//popup.AppendMenuIcon(ID_BLAMETWO, IDS_LOG_POPUP_BLAMEREVS, IDI_BLAME);
				if(m_ContextMenuMask&GetContextMenuBit(ID_GNUDIFF2))
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
			if (bAddSeparator)
				popup.AppendMenu(MF_SEPARATOR, NULL);
		}

		if ( GetSelectedCount() >0 && pSelLogEntry->m_CommitHash != GIT_REV_ZERO)
		{
			if ( IsSelectionContinuous() && GetSelectedCount() >= 2 )
			{
				if(m_ContextMenuMask&GetContextMenuBit(ID_COMBINE_COMMIT))
				{
					CString head;
					int headindex;
					headindex = this->GetHeadIndex();
					if(headindex>=0)
					{
						head.Format(_T("HEAD~%d"),LastSelect-headindex);
						CString hash=g_Git.GetHash(head);
						hash=hash.Left(40);
						GitRev* pLastEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
						if(pLastEntry->m_CommitHash == hash)
							popup.AppendMenuIcon(ID_COMBINE_COMMIT,IDS_COMBINE_TO_ONE,IDI_COMBINE);
					}
				}
			}
			if(m_ContextMenuMask&GetContextMenuBit(ID_CHERRY_PICK))
				popup.AppendMenuIcon(ID_CHERRY_PICK, IDS_CHERRY_PICK_VERSION, IDI_EXPORT);

			if(GetSelectedCount()<=2 || (IsSelectionContinuous() && GetSelectedCount() > 0))
				if(m_ContextMenuMask&GetContextMenuBit(ID_CREATE_PATCH))
					popup.AppendMenuIcon(ID_CREATE_PATCH, IDS_CREATE_PATCH, IDI_PATCH);
			
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
			if(m_ContextMenuMask&GetContextMenuBit(ID_COPYHASH))
				popup.AppendMenuIcon(ID_COPYHASH, IDS_COPY_COMMIT_HASH);
		}
		if (GetSelectedCount() != 0)
		{
			if(m_ContextMenuMask&GetContextMenuBit(ID_COPYCLIPBOARD))
				popup.AppendMenuIcon(ID_COPYCLIPBOARD, IDS_LOG_POPUP_COPYTOCLIPBOARD);
		}

		if(m_ContextMenuMask&GetContextMenuBit(ID_FINDENTRY))
			popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND);


		if (GetSelectedCount() == 1)
		{
			if(m_ContextMenuMask &GetContextMenuBit(ID_DELETE))
			{
				if( this->m_HashMap.find(pSelLogEntry->m_CommitHash) != m_HashMap.end() )
				{
					CString str;
					str.LoadString(IDS_DELETE_BRANCHTAG);
					if( m_HashMap[pSelLogEntry->m_CommitHash].size() == 1 )
					{
						str+=_T(" ");
						str+=m_HashMap[pSelLogEntry->m_CommitHash].at(0);
						popup.AppendMenuIcon(ID_DELETE,str+_T("..."),IDI_DELETE);
					}
					else if( m_HashMap[pSelLogEntry->m_CommitHash].size() > 1 )
					{
						
						submenu.CreatePopupMenu();
						for(int i=0;i<m_HashMap[pSelLogEntry->m_CommitHash].size();i++)
						{
							submenu.AppendMenuIcon(ID_DELETE+(i<<16),m_HashMap[pSelLogEntry->m_CommitHash][i]+_T("..."));
						}

						popup.AppendMenu(MF_BYPOSITION|MF_POPUP|MF_STRING,(UINT) submenu.m_hMenu,str); 

					}
					
				}
			}
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this, 0);
//		DialogEnableWindow(IDOK, FALSE);
//		SetPromptApp(&theApp);
	
		this->ContextMenuAction(cmd, FirstSelect, LastSelect);
		
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
			GitRev * pLogEntry = reinterpret_cast<GitRev *>(m_arShownList.GetAt(GetNextSelectedItem(pos)));

			if(!HashOnly)
			{
				//pLogEntry->m_Files
				//LogChangedPathArray * cpatharray = pLogEntry->pArChangedPaths;
			
				for (int cpPathIndex = 0; cpPathIndex<pLogEntry->m_Files.GetCount(); ++cpPathIndex)
				{
					sPaths += ((CTGitPath&)pLogEntry->m_Files[cpPathIndex]).GetActionName() + _T(" : ") + pLogEntry->m_Files[cpPathIndex].GetGitPathString();
					sPaths += _T("\r\n");
				}
				sPaths.Trim();
				sLogCopyText.Format(_T("%s: %s\r\n%s: %s\r\n%s: %s\r\n%s:\r\n%s\r\n----\r\n%s\r\n\r\n"),
					(LPCTSTR)sRev, pLogEntry->m_CommitHash,
					(LPCTSTR)sAuthor, (LPCTSTR)pLogEntry->m_AuthorName,
					(LPCTSTR)sDate, 
					(LPCTSTR)CAppUtils::FormatDateAndTime( pLogEntry->m_AuthorDate, m_DateFormat, true, m_bRelativeTimes ),
					(LPCTSTR)sMessage, pLogEntry->m_Subject+_T("\r\n")+pLogEntry->m_Body,
					(LPCTSTR)sPaths);
				sClipdata +=  sLogCopyText;
			}else
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

	ContextMenuAction(ID_COMPAREWITHPREVIOUS,FirstSelect,LastSelect);

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
	PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(m_LogList.GetNextSelectedItem(pos)));
	long rev1 = pLogEntry->Rev;
	long rev2 = rev1-1;
	CTGitPath path = m_path;

	// See how many files under the relative root were changed in selected revision
	int nChanged = 0;
	LogChangedPath * changed = NULL;
	for (INT_PTR c = 0; c < pLogEntry->pArChangedPaths->GetCount(); ++c)
	{
		LogChangedPath * cpath = pLogEntry->pArChangedPaths->GetAt(c);
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
		GitRev * pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(i));
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
			PLOGENTRYDATA pLogEntry = reinterpret_cast<PLOGENTRYDATA>(m_arShownList.GetAt(i));
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

	this->m_logEntries.ClearAll();
	this->m_logEntries.ParserFromLog(path,-1,info,from,to);

	//this->m_logEntries.ParserFromLog();
	SetItemCountEx(this->m_logEntries.size());

	this->m_arShownList.RemoveAll();

	for(unsigned int i=0;i<m_logEntries.size();i++)
	{
		if(m_IsOldFirst)
		{
			m_logEntries[m_logEntries.size()-i-1].m_IsFull=TRUE;
			this->m_arShownList.Add(&m_logEntries[m_logEntries.size()-i-1]);
		
		}else
		{
			m_logEntries[i].m_IsFull=TRUE;
			this->m_arShownList.Add(&m_logEntries[i]);
		}
	}

    if(path)
        m_Path=*path;
	return 0;

}

int CGitLogListBase::FillGitShortLog()
{
	ClearText();

	this->m_logEntries.ClearAll();

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
	mask |= m_ShowMask;
	
	if(m_bShowWC)
		this->m_logEntries.insert(m_logEntries.begin(),this->m_wcRev);

	this->m_logEntries.FetchShortLog(path,m_StartRef,-1,mask);

	//this->m_logEntries.ParserFromLog();
	if(IsInWorkingThread())
	{
		PostMessage(LVM_SETITEMCOUNT, (WPARAM) this->m_logEntries.size(),(LPARAM) LVSICF_NOINVALIDATEALL);
	}
	else
	{
		SetItemCountEx(this->m_logEntries.size());
	}

	this->m_arShownList.RemoveAll();

	for(unsigned int i=0;i<m_logEntries.size();i++)
	{
		if(i>0 || m_logEntries[i].m_CommitHash != GIT_REV_ZERO)
			m_logEntries[i].m_Subject=_T("parser...");

		if(this->m_IsOldFirst)
		{
			this->m_arShownList.Add(&m_logEntries[m_logEntries.size()-1-i]);

		}else
		{
			this->m_arShownList.Add(&m_logEntries[i]);
		}
	}
	return 0;
}

BOOL CGitLogListBase::PreTranslateMessage(MSG* pMsg)
{
	// Skip Ctrl-C when copying text out of the log message or search filter
	BOOL bSkipAccelerator = ( pMsg->message == WM_KEYDOWN && pMsg->wParam=='C' && (GetFocus()==GetDlgItem(IDC_MSGVIEW) || GetFocus()==GetDlgItem(IDC_SEARCHEDIT) ) && GetKeyState(VK_CONTROL)&0x8000 );
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
	m_LoadingThread = AfxBeginThread(LogThreadEntry, this);
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
		if(m_logEntries[i].m_AuthorDate.GetTime() < oldest.GetTime())
			oldest = m_logEntries[i].m_AuthorDate.GetTime();

		if(m_logEntries[i].m_AuthorDate.GetTime() > latest.GetTime())
			latest = m_logEntries[i].m_AuthorDate.GetTime();

	}
}

//Helper class for FetchFullLogInfo()
class CGitCall_FetchFullLogInfo : public CGitCall
{
public:
	CGitCall_FetchFullLogInfo(CGitLogListBase* ploglist):m_ploglist(ploglist),m_CollectedCount(0){}
	virtual bool OnOutputData(const BYTE* data, size_t size)
	{
		if(size==0)
			return m_ploglist->m_bExitThread;
		//Add received data to byte collector
		m_ByteCollector.append(data,size);

		//Find loginfo endmarker
		static const BYTE dataToFind[]={0,0,'#','<'};
		int found=m_ByteCollector.findData(dataToFind,4);
		if(found<0)
			return m_ploglist->m_bExitThread;//Not found
		found+=2;//Position after loginfo end-marker

		//Prepare data for OnLogInfo and call it
		BYTE_VECTOR logInfo;
		logInfo.append(&*m_ByteCollector.begin(),found);
		OnLogInfo(logInfo);

		//Remove loginfo from bytecollector
		m_ByteCollector.erase(m_ByteCollector.begin(),m_ByteCollector.begin()+found);

		return m_ploglist->m_bExitThread;
	}
	virtual void OnEnd()
	{
		//Rest should be a complete log line.
		if(!m_ByteCollector.empty())
			OnLogInfo(m_ByteCollector);
	}


	void OnLogInfo(BYTE_VECTOR& logInfo)
	{
		GitRev fullRev;
		fullRev.ParserFromLog(logInfo);
		MAP_HASH_REV::iterator itRev=m_ploglist->m_logEntries.m_HashMap.find(fullRev.m_CommitHash);
		if(itRev==m_ploglist->m_logEntries.m_HashMap.end())
		{
			//Should not occur, only when Git-tree updated in the mean time. (Race condition)
			return;//Ignore
		}
		//Set updating
		int rev=itRev->second;
		GitRev* revInVector=&m_ploglist->m_logEntries[rev];


		if(revInVector->m_IsFull)
			return;

		if(!m_ploglist->m_LogCache.GetCacheData(m_ploglist->m_logEntries[rev]))
		{
			++m_CollectedCount;
			InterlockedExchange(&m_ploglist->m_logEntries[rev].m_IsUpdateing,FALSE);
			InterlockedExchange(&m_ploglist->m_logEntries[rev].m_IsFull,TRUE);
			::PostMessage(m_ploglist->m_hWnd,MSG_LOADED,(WPARAM)rev,0);
			return;
		}

//		fullRev.m_IsUpdateing=TRUE;
//		fullRev.m_IsFull=TRUE;
	

		if(InterlockedExchange(&revInVector->m_IsUpdateing,TRUE))
			return;//Cannot update this row now. Ignore.
		TCHAR oldmark=revInVector->m_Mark;
		GIT_REV_LIST oldlist=revInVector->m_ParentHash;
//		CString oldhash=m_CommitHash;

		//Parse new rev info
		revInVector->ParserFromLog(logInfo);

		if(oldmark!=0)
			revInVector->m_Mark=oldmark;  //parser full log will cause old mark overwrited. 
							       //So we need keep old bound mark.
		revInVector->m_ParentHash=oldlist;

		//update cache
		m_ploglist->m_LogCache.AddCacheEntry(*revInVector);

		//Reset updating
		InterlockedExchange(&revInVector->m_IsFull,TRUE);
		InterlockedExchange(&revInVector->m_IsUpdateing,FALSE);

		//Notify listcontrol and update progress bar
		++m_CollectedCount;

		::PostMessage(m_ploglist->m_hWnd,MSG_LOADED,(WPARAM)rev,0);

		DWORD percent=m_CollectedCount*68/m_ploglist->m_logEntries.size() + GITLOG_START+1+30;
		if(percent == GITLOG_END)
			percent = GITLOG_END -1;
		
		::PostMessage(m_ploglist->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) percent,0);
	}

	CGitLogListBase*	m_ploglist;
	BYTE_VECTOR			m_ByteCollector;
	int					m_CollectedCount;

};

void CGitLogListBase::FetchFullLogInfo(CString &from, CString &to)
{
	CGitCall_FetchFullLogInfo fetcher(this);
	int mask=
		CGit::LOG_INFO_FULL_DIFF|
		CGit::LOG_INFO_STAT|
		CGit::LOG_INFO_FILESTATE|
		CGit::LOG_INFO_DETECT_COPYRENAME|
		CGit::LOG_INFO_SHOW_MERGEDFILE |
		m_ShowMask;

	CTGitPath *path;
    if(this->m_Path.IsEmpty())
        path=NULL;
    else
        path=&this->m_Path;

	g_Git.GetLog(&fetcher,CString(),path,-1,mask,&from,&to);
}

void CGitLogListBase::FetchLastLogInfo()
{
	unsigned int updated=0;
	int percent=0;
	CRect rect;
	{
		for(unsigned int i=0;i<m_logEntries.size();i++)
		{
			if(m_logEntries[i].m_IsFull)
				continue;

			if(m_LogCache.GetCacheData(m_logEntries[i]))
			{
				if(!m_logEntries.FetchFullInfo(i))
				{
					updated++;
  				}
				m_LogCache.AddCacheEntry(m_logEntries[i]);

			}else
			{
				updated++;
				InterlockedExchange(&m_logEntries[i].m_IsUpdateing,FALSE);
				InterlockedExchange(&m_logEntries[i].m_IsFull,TRUE);
			}
			
			::PostMessage(m_hWnd,MSG_LOADED,(WPARAM)i,0);

			if(m_bExitThread)
			{
				InterlockedExchange(&m_bThreadRunning, FALSE);
				InterlockedExchange(&m_bNoDispUpdates, FALSE);
				return;
			}			
		}
	}
}

UINT CGitLogListBase::LogThread()
{

//	if(m_ProcCallBack)
//		m_ProcCallBack(m_ProcData,GITLOG_START);
	::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) GITLOG_START,0);

	InterlockedExchange(&m_bThreadRunning, TRUE);
	InterlockedExchange(&m_bNoDispUpdates, TRUE);

    //does the user force the cache to refresh (shift or control key down)?
    bool refresh =    (GetKeyState (VK_CONTROL) < 0) 
                   || (GetKeyState (VK_SHIFT) < 0);

	//disable the "Get All" button while we're receiving
	//log messages.

	FillGitShortLog();
	
	if(this->m_bExitThread)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		InterlockedExchange(&m_bNoDispUpdates, FALSE);
		return 0;
	}
	InterlockedExchange(&m_bNoDispUpdates, FALSE);
	::PostMessage(GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) GITLOG_START_ALL, 0);

	int start=0; CString firstcommit,lastcommit;
	int update=0;
	for(int i=0;i<m_logEntries.size();i++)
	{
		if( i==0 && m_logEntries[i].m_CommitHash == GIT_REV_ZERO)
		{
			m_logEntries[i].m_Files.Clear();
			m_logEntries[i].m_ParentHash.clear();
			m_logEntries[i].m_ParentHash.push_back(m_HeadHash);
			g_Git.GetCommitDiffList(m_logEntries[i].m_CommitHash,this->m_HeadHash,m_logEntries[i].m_Files);
			m_logEntries[i].m_Action =0;
			for(int j=0;j< m_logEntries[i].m_Files.GetCount();j++)
				m_logEntries[i].m_Action |= m_logEntries[i].m_Files[j].m_Action;
			
			m_logEntries[i].m_Body.Format(_T("%d files changed"),m_logEntries[i].m_Files.GetCount());
			::PostMessage(m_hWnd,MSG_LOADED,(WPARAM)0,0);
			continue;
		}

		start=this->m_logEntries[i].ParserFromLog(m_logEntries.m_RawlogData,start);
		m_logEntries.m_HashMap[m_logEntries[i].m_CommitHash]=i;

		if(m_LogCache.GetCacheData(m_logEntries[i]))
		{
			if(firstcommit.IsEmpty())
				firstcommit=m_logEntries[i].m_CommitHash;
			lastcommit=m_logEntries[i].m_CommitHash;

		}else
		{
			InterlockedExchange(&m_logEntries[i].m_IsUpdateing,FALSE);
			InterlockedExchange(&m_logEntries[i].m_IsFull,TRUE);
			update++;
		}
		if(start<0)
			break;
		if(start>=m_logEntries.m_RawlogData.size())
			break;

		int percent=i*30/m_logEntries.size() + GITLOG_START+1;

		::PostMessage(GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) percent, 0);
		::PostMessage(m_hWnd,MSG_LOADED,(WPARAM) i, 0);

		if(this->m_bExitThread)
		{	
			InterlockedExchange(&m_bThreadRunning, FALSE);
			InterlockedExchange(&m_bNoDispUpdates, FALSE);
			return 0;
		}
	}
	if(!lastcommit.IsEmpty())
		FetchFullLogInfo(lastcommit,firstcommit);
	
	this->FetchLastLogInfo();
	
#if 0
	RedrawItems(0, m_arShownList.GetCount());
//	SetRedraw(false);
//	ResizeAllListCtrlCols();
//	SetRedraw(true);

	if ( m_pStoreSelection )
	{
		// Deleting the instance will restore the
		// selection of the CLogDlg.
		delete m_pStoreSelection;
		m_pStoreSelection = NULL;
	}
	else
	{
		// If no selection has been set then this must be the first time
		// the revisions are shown. Let's preselect the topmost revision.
		if ( GetItemCount()>0 )
		{
			SetSelectionMark(0);
			SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
#endif



	//FetchFullLogInfo();
	//FetchFullLogInfoOrig();
	//RefreshCursor();
	// make sure the filter is applied (if any) now, after we refreshed/fetched
	// the log messages

	::PostMessage(this->GetParent()->m_hWnd,MSG_LOAD_PERCENTAGE,(WPARAM) GITLOG_END,0);

	InterlockedExchange(&m_bThreadRunning, FALSE);

	return 0;
}

void CGitLogListBase::Refresh()
{	
	m_bExitThread=TRUE;
	if(m_LoadingThread!=NULL)
	{
		DWORD ret =::WaitForSingleObject(m_LoadingThread->m_hThread,20000);
		if(ret == WAIT_TIMEOUT)
			TerminateThread();
	}

	this->Clear();

	//Update branch and Tag info
	ReloadHashMap();
	//Assume Thread have exited
	//if(!m_bThreadRunning)
	{
		this->SetItemCountEx(0);
		m_logEntries.clear();
		m_bExitThread=FALSE;
		InterlockedExchange(&m_bThreadRunning, TRUE);
		InterlockedExchange(&m_bNoDispUpdates, TRUE);
		if (AfxBeginThread(LogThreadEntry, this)==NULL)
		{
			InterlockedExchange(&m_bThreadRunning, FALSE);
			InterlockedExchange(&m_bNoDispUpdates, FALSE);
			CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
		m_sFilterText.Empty();
		m_From=CTime(1970,1,2,0,0,0);
		m_To=CTime::GetCurrentTime();
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

void CGitLogListBase::RecalculateShownList(CPtrArray * pShownlist)
{

	pShownlist->RemoveAll();
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
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
#endif
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
			{
				ATLTRACE(_T("messge = \"%s\"\n"),m_logEntries[i].m_Subject);
				if (regex_search(wstring((LPCTSTR)m_logEntries[i].m_Subject), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(&m_logEntries[i]);
					continue;
				}

				ATLTRACE(_T("messge = \"%s\"\n"),m_logEntries[i].m_Body);
				if (regex_search(wstring((LPCTSTR)m_logEntries[i].m_Body), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(&m_logEntries[i]);
					continue;
				}
			}
#if 0
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
			{
				LogChangedPathArray * cpatharray = m_logEntries[i]->pArChangedPaths;

				bool bGoing = true;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					if (regex_search(wstring((LPCTSTR)cpath->sCopyFromPath), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					if (regex_search(wstring((LPCTSTR)cpath->sPath), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					if (regex_search(wstring((LPCTSTR)cpath->GetAction()), pat, flags)&&IsEntryInDateRange(i))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
				}
				if (!bGoing)
					continue;
			}
#endif
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
			{
				if (regex_search(wstring((LPCTSTR)m_logEntries[i].m_AuthorName), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(&m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_REVS))
			{
				sRev.Format(_T("%s"), m_logEntries[i].m_CommitHash);
				if (regex_search(wstring((LPCTSTR)sRev), pat, flags)&&IsEntryInDateRange(i))
				{
					pShownlist->Add(&m_logEntries[i]);
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
					pShownlist->Add(m_logEntries[i]);
					continue;
				}
			}
#endif
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_MESSAGES))
			{
				CString msg = m_logEntries[i].m_Subject;

				msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(&m_logEntries[i]);
					continue;
				}
				msg = m_logEntries[i].m_Body;

				msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(&m_logEntries[i]);
					continue;
				}
			}
#if 0
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_PATHS))
			{
				LogChangedPathArray * cpatharray = m_logEntries[i]->pArChangedPaths;

				bool bGoing = true;
				for (INT_PTR cpPathIndex = 0; cpPathIndex<cpatharray->GetCount() && bGoing; ++cpPathIndex)
				{
					LogChangedPath * cpath = cpatharray->GetAt(cpPathIndex);
					CString path = cpath->sCopyFromPath;
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					path = cpath->sPath;
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
					path = cpath->GetAction();
					path.MakeLower();
					if ((path.Find(find)>=0)&&(IsEntryInDateRange(i)))
					{
						pShownlist->Add(m_logEntries[i]);
						bGoing = false;
						continue;
					}
				}
			}
#endif
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_AUTHORS))
			{
				CString msg = m_logEntries[i].m_AuthorName;
				msg = msg.MakeLower();
				if ((msg.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(&m_logEntries[i]);
					continue;
				}
			}
			if ((m_nSelectedFilter == LOGFILTER_ALL)||(m_nSelectedFilter == LOGFILTER_REVS))
			{
				sRev.Format(_T("%s"), m_logEntries[i].m_CommitHash);
				if ((sRev.Find(find) >= 0)&&(IsEntryInDateRange(i)))
				{
					pShownlist->Add(&m_logEntries[i]);
					continue;
				}
			}
		} // else (from if (bRegex))	
	} // for (DWORD i=0; i<m_logEntries.size(); ++i) 

}

BOOL CGitLogListBase::IsEntryInDateRange(int i)
{
	__time64_t time = m_logEntries[i].m_AuthorDate.GetTime();
	if ((time >= m_From.GetTime())&&(time <= m_To.GetTime()))
		return TRUE;

	return FALSE;

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
	SetRedraw(false);
	//ResizeAllListCtrlCols();
	SetRedraw(true);
	Invalidate();

}
void CGitLogListBase::RemoveFilter()
{

	InterlockedExchange(&m_bNoDispUpdates, TRUE);

	m_arShownList.RemoveAll();

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
			m_arShownList.Add(&m_logEntries[m_logEntries.size()-i-1]);
		}else
		{
			m_arShownList.Add(&m_logEntries[i]);
		}
	}
//	InterlockedExchange(&m_bNoDispUpdates, FALSE);
	DeleteAllItems();
	SetItemCountEx(ShownCountWithStopped());
	RedrawItems(0, ShownCountWithStopped());
//	SetRedraw(false);
//	ResizeAllListCtrlCols();
//	SetRedraw(true);

	InterlockedExchange(&m_bNoDispUpdates, FALSE);
}

void CGitLogListBase::Clear()
{
	m_arShownList.RemoveAll();
	DeleteAllItems();

	m_logEntries.ClearAll();

}

void CGitLogListBase::OnDestroy()
{
	// save the column widths to the registry
	SaveColumnWidths();

	if(this->m_bThreadRunning)
	{
		this->m_bExitThread=true;
		DWORD ret =::WaitForSingleObject(m_LoadingThread->m_hThread,20000);
		if(ret == WAIT_TIMEOUT)
			TerminateThread();
	}
	while(m_LogCache.SaveCache())
	{
		if(CMessageBox::Show(NULL,_T("Cannot Save Log Cache to Disk. To retry click yes. To give up click no."),_T("TortoiseGit"),
							MB_YESNO) == IDNO)
							break;
	}
	CHintListCtrl::OnDestroy();
}

LRESULT CGitLogListBase::OnLoad(WPARAM wParam,LPARAM lParam)
{
	CRect rect;
	int i=(int)wParam;
	this->GetItemRect(i,&rect,LVIR_BOUNDS);
	this->InvalidateRect(rect);

	if(this->GetItemState(i,LVIF_STATE) & LVIS_SELECTED)
	{
		int i=0;
	}
	return 0;
}

/**
 * Save column widths to the registry
 */
void CGitLogListBase::SaveColumnWidths()
{
	CHeaderCtrl* pHdrCtrl = (CHeaderCtrl*)(GetDlgItem(0));
	if (pHdrCtrl)
	{
		int numcols = pHdrCtrl->GetItemCount();
		for (int col = 0; col < numcols; col++)
		{
			int width = GetColumnWidth( col );
			CString regentry;
			regentry.Format( _T("Software\\TortoiseGit\\%s\\ColWidth%d"),m_ColumnRegKey, col);
			CRegDWORD regwidth(regentry, 0);
			regwidth = width;	// this writes it to reg
		}
	}
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
			if(pRev->m_CommitHash == m_HeadHash )
				return i;
		}
	}
	return -1;
}