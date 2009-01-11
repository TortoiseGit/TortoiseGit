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
	m_wcRev.m_Subject=_T("Working Copy");

	m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon    =  (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

	m_bFilterWithRegex = !!CRegDWORD(_T("Software\\TortoiseGit\\UseRegexFilter"), TRUE);

	g_Git.GetMapHashToFriendName(m_HashMap);

	m_From=CTime(1970,1,2,0,0,0);
	m_To=CTime::GetCurrentTime();
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
}


BEGIN_MESSAGE_MAP(CGitLogListBase, CHintListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdrawLoglist)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetdispinfoLoglist)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkLoglist)
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM,OnLvnOdfinditemLoglist)
	ON_WM_CREATE()
END_MESSAGE_MAP()

int CGitLogListBase:: OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	PreSubclassWindow();
	return CHintListCtrl::OnCreate(lpCreateStruct);
}

void CGitLogListBase::PreSubclassWindow()
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES);
	// load the icons for the action columns
	m_Theme.SetWindowTheme(GetSafeHwnd(), L"Explorer", NULL);
	CHintListCtrl::PreSubclassWindow();
}

void CGitLogListBase::InsertGitColumn()
{
	CString temp;

	int c = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	
	while (c>=0)
		DeleteColumn(c--);
	temp.LoadString(IDS_LOG_GRAPH);

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

void CGitLogListBase::ResizeAllListCtrlCols()
{

	const int nMinimumWidth = ICONITEMBORDER+16*4;
	int maxcol = ((CHeaderCtrl*)(GetDlgItem(0)))->GetItemCount()-1;
	int nItemCount = GetItemCount();
	TCHAR textbuf[MAX_PATH];
	CHeaderCtrl * pHdrCtrl = (CHeaderCtrl*)(GetDlgItem(0));
	if (pHdrCtrl)
	{
		for (int col = 0; col <= maxcol; col++)
		{
			HDITEM hdi = {0};
			hdi.mask = HDI_TEXT;
			hdi.pszText = textbuf;
			hdi.cchTextMax = sizeof(textbuf);
			pHdrCtrl->GetItem(col, &hdi);
			int cx = GetStringWidth(hdi.pszText)+20; // 20 pixels for col separator and margin
			for (int index = 0; index<nItemCount; ++index)
			{
				// get the width of the string and add 14 pixels for the column separator and margins
				int linewidth = GetStringWidth(GetItemText(index, col)) + 14;
				if (index < m_arShownList.GetCount())
				{
					GitRev * pCurLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(index));
					if ((pCurLogEntry)&&(pCurLogEntry->m_CommitHash == m_wcRev.m_CommitHash))
					{
						// set the bold font and ask for the string width again
						SendMessage(WM_SETFONT, (WPARAM)m_boldFont, NULL);
						linewidth = GetStringWidth(GetItemText(index, col)) + 14;
						// restore the system font
						SendMessage(WM_SETFONT, NULL, NULL);
					}
				}
				if (index == 0)
				{
					// add the image size
					CImageList * pImgList = GetImageList(LVSIL_SMALL);
					if ((pImgList)&&(pImgList->GetImageCount()))
					{
						IMAGEINFO imginfo;
						pImgList->GetImageInfo(0, &imginfo);
						linewidth += (imginfo.rcImage.right - imginfo.rcImage.left);
						linewidth += 3;	// 3 pixels between icon and text
					}
				}
				if (cx < linewidth)
					cx = linewidth;
			}
			// Adjust columns "Actions" containing icons
			if (col == this->LOGLIST_ACTION)
			{
				if (cx < nMinimumWidth)
				{
					cx = nMinimumWidth;
				}
			}
			
			if (col == this->LOGLIST_MESSAGE)
			{
				if (cx > LOGLIST_MESSAGE_MAX)
				{
					cx = LOGLIST_MESSAGE_MAX;
				}
				if (cx < LOGLIST_MESSAGE_MIN)
				{
					cx = LOGLIST_MESSAGE_MIN;
				}

			}
			// keep the bug id column small
			if ((col == 4)&&(m_bShowBugtraqColumn))
			{
				if (cx > (int)(DWORD)m_regMaxBugIDColWidth)
				{
					cx = (int)(DWORD)m_regMaxBugIDColWidth;
				}
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

	if (m_Theme.IsAppThemed() && m_bVista)
	{
		m_Theme.Open(m_hWnd, L"Explorer");
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
#if 0
			if (pLogEntry->bCopiedSelf)
			{
				// unfortunately, the pLVCD->nmcd.uItemState does not contain valid
				// information at this drawing stage. But we can check the whether the
				// previous stage changed the background color of the item
				if (pLVCD->clrTextBk == GetSysColor(COLOR_MENU))
				{
					HBRUSH brush;
					brush = ::CreateSolidBrush(::GetSysColor(COLOR_MENU));
					if (brush)
					{
						::FillRect(pLVCD->nmcd.hdc, &rect, brush);
						::DeleteObject(brush);
					}
				}
			}
#endif
		}

		if (m_Theme.IsBackgroundPartiallyTransparent(LVP_LISTDETAIL, state))
			m_Theme.DrawParentBackground(m_hWnd, hdc, &rect);

			m_Theme.DrawBackground(hdc, LVP_LISTDETAIL, state, &rect, NULL);
	}
	else
	{
		HBRUSH brush;
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

	for(int i=0;i<m_HashMap[data->m_CommitHash].size();i++)
	{
		CString str;
		str=m_HashMap[data->m_CommitHash][i];
		
		CString shortname;
		HBRUSH brush=0;
		shortname=_T("");
		if(GetShortName(str,shortname,_T("refs/heads/")))
		{
			brush = ::CreateSolidBrush(RGB(0xff, 0, 0));
		}else if(GetShortName(str,shortname,_T("refs/remotes/")))
		{
			brush = ::CreateSolidBrush(RGB(0xff, 0xff, 0));
		}
		else if(GetShortName(str,shortname,_T("refs/tags/")))
		{
			brush = ::CreateSolidBrush(RGB(0, 0, 0xff));
		}

		if(!shortname.IsEmpty())
		{
			SIZE size;
			memset(&size,0,sizeof(SIZE));
			GetTextExtentPoint32(hdc, shortname,shortname.GetLength(),&size);
		
			rt.SetRect(rt.left,rt.top,rt.left+size.cx,rt.bottom);
			rt.right+=4;
			::FillRect(hdc, &rt, brush);
			if (rItem.state & LVIS_SELECTED)
			{
				COLORREF   clrOld   = ::SetTextColor(hdc,::GetSysColor(COLOR_HIGHLIGHTTEXT));   
				::DrawText(hdc,shortname,shortname.GetLength(),&rt,DT_CENTER);
				::SetTextColor(hdc,clrOld);   
			}else
			{
				::DrawText(hdc,shortname,shortname.GetLength(),&rt,DT_CENTER);
			}

			
			::MoveToEx(hdc,rt.left,rt.top,NULL);
			::LineTo(hdc,rt.right,rt.top);
			::LineTo(hdc,rt.right,rt.bottom);
			::LineTo(hdc,rt.left,rt.bottom);
			::LineTo(hdc,rt.left,rt.top);
				
			rt.left=rt.right+3;
		}
		if(brush)
			::DeleteObject(brush);
	}		
	rt.right=rect.right;

	if (rItem.state & LVIS_SELECTED)
	{
		COLORREF   clrOld   = ::SetTextColor(hdc,::GetSysColor(COLOR_HIGHLIGHTTEXT));   
		::DrawText(hdc,data->m_Subject,data->m_Subject.GetLength(),&rt,DT_LEFT);
		::SetTextColor(hdc,clrOld);   
	}else
	{
		::DrawText(hdc,data->m_Subject,data->m_Subject.GetLength(),&rt,DT_LEFT);
	}
	
}

void CGitLogListBase::paintGraphLane(HDC hdc, int laneHeight,int type, int x1, int x2,
                                      const COLORREF& col,int top
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

	//static QPen myPen(Qt::black, 2); // fast path here
	CPen pen;
	pen.CreatePen(PS_SOLID,2,col);
	//myPen.setColor(col);
	HPEN oldpen=(HPEN)::SelectObject(hdc,(HPEN)pen);

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
		DrawLine(hdc,P_90,P_270);
		//p->drawLine(P_90, P_270);
		break;
	case Lanes::HEAD:
	case Lanes::HEAD_R:
	case Lanes::HEAD_L:
	case Lanes::BRANCH:
		DrawLine(hdc,P_CENTER,P_270);
		//p->drawLine(P_CENTER, P_270);
		break;
	case Lanes::TAIL:
	case Lanes::TAIL_R:
	case Lanes::TAIL_L:
	case Lanes::INITIAL:
	case Lanes::BOUNDARY:
	case Lanes::BOUNDARY_C:
	case Lanes::BOUNDARY_R:
	case Lanes::BOUNDARY_L:
		DrawLine(hdc,P_90, P_CENTER);
		//p->drawLine(P_90, P_CENTER);
		break;
	default:
		break;
	}

	// horizontal line
	switch (type) {
	case Lanes::MERGE_FORK:
	case Lanes::JOIN:
	case Lanes::HEAD:
	case Lanes::TAIL:
	case Lanes::CROSS:
	case Lanes::CROSS_EMPTY:
	case Lanes::BOUNDARY_C:
		DrawLine(hdc,P_180,P_0);
		//p->drawLine(P_180, P_0);
		break;
	case Lanes::MERGE_FORK_R:
	case Lanes::JOIN_R:
	case Lanes::HEAD_R:
	case Lanes::TAIL_R:
	case Lanes::BOUNDARY_R:
		DrawLine(hdc,P_180,P_CENTER);
		//p->drawLine(P_180, P_CENTER);
		break;
	case Lanes::MERGE_FORK_L:
	case Lanes::JOIN_L:
	case Lanes::HEAD_L:
	case Lanes::TAIL_L:
	case Lanes::BOUNDARY_L:
		DrawLine(hdc,P_CENTER,P_0);
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
	CRect rt=rect;
	LVITEM   rItem;
	SecureZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask  = LVIF_STATE;
	rItem.iItem = index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	static const COLORREF colors[Lanes::COLORS_NUM] = { RGB(0,0,0), RGB(0xFF,0,0), RGB(0,0xFF,0),
	                                           RGB(0,0,0xFF), RGB(128,128,128), RGB(128,128,0),
	                                           RGB(0,128,128), RGB(128,0,128) };


//	p->translate(QPoint(opt.rect.left(), opt.rect.top()));



	if (data->m_Lanes.size() == 0)
		m_logEntries.setLane(data->m_CommitHash);

	std::vector<int>& lanes=data->m_Lanes;
	UINT laneNum = lanes.size();
	UINT mergeLane = 0;
	for (UINT i = 0; i < laneNum; i++)
		if (Lanes::isMerge(lanes[i])) {
			mergeLane = i;
			break;
		}

	int x1 = 0, x2 = 0;
	int maxWidth = rect.Width();
	int lw = 3 * rect.Height() / 4; //laneWidth() 
	for (UINT i = 0; i < laneNum && x2 < maxWidth; i++) {

		x1 = x2;
		x2 += lw;

		int ln = lanes[i];
		if (ln == Lanes::EMPTY)
			continue;

		UINT col = (  Lanes:: isHead(ln) ||Lanes:: isTail(ln) || Lanes::isJoin(ln)
		            || ln ==Lanes:: CROSS_EMPTY) ? mergeLane : i;

		if (ln == Lanes::CROSS) {
			paintGraphLane(hdc, rect.Height(),Lanes::NOT_ACTIVE, x1, x2, colors[col % Lanes::COLORS_NUM],rect.top);
			paintGraphLane(hdc, rect.Height(),Lanes::CROSS, x1, x2, colors[mergeLane % Lanes::COLORS_NUM],rect.top);
		} else
			paintGraphLane(hdc, rect.Height(),ln, x1, x2, colors[col % Lanes::COLORS_NUM],rect.top);
	}

	TRACE(_T("index %d %d\r\n"),index,data->m_Lanes.size());
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
//					if ((data->childStackDepth)||(m_mergedRevs.find(data->Rev) != m_mergedRevs.end()))
//						crText = GetSysColor(COLOR_GRAYTEXT);
//					if (data->Rev == m_wcRev)
//					{
//						SelectObject(pLVCD->nmcd.hdc, m_boldFont);
						// We changed the font, so we're returning CDRF_NEWFONT. This
						// tells the control to recalculate the extent of the text.
//						*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
//					}
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
				if (m_arShownList.GetCount() > (INT_PTR)pLVCD->nmcd.dwItemSpec)
				{
					CRect rect;
					GetSubItemRect(pLVCD->nmcd.dwItemSpec, pLVCD->iSubItem, LVIR_BOUNDS, rect);
					
					FillBackGround(pLVCD->nmcd.hdc, (INT_PTR)pLVCD->nmcd.dwItemSpec,rect);
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
					if(!data->m_IsFull)
					{
						if(data->SafeFetchFullInfo(&g_Git))
							this->Invalidate();
						TRACE(_T("Update ... %d\r\n"),pLVCD->nmcd.dwItemSpec);
					}

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
				// Get the selected state of the
				// item being drawn.							

				// Fill the background
				FillBackGround(pLVCD->nmcd.hdc, (INT_PTR)pLVCD->nmcd.dwItemSpec,rect);
				
				// Draw the icon(s) into the compatible DC
				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_MODIFIED)
					::DrawIconEx(pLVCD->nmcd.hdc, rect.left + ICONITEMBORDER, rect.top, m_hModifiedIcon, iconwidth, iconheight, 0, NULL, DI_NORMAL);
				nIcons++;

				if (pLogEntry->m_Action & CTGitPath::LOGACTIONS_ADDED)
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
	temp.Format(_T("%d"),m_arShownList.GetCount()-pItem->iItem);
	    
	// Which column?
	switch (pItem->iSubItem)
	{
	case this->LOGLIST_GRAPH:	//Graphic
		if (pLogEntry)
		{
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
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->m_AuthorDate.Format(_T("%Y-%m-%d %H:%M")), pItem->cchTextMax);
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
	if (popup.CreatePopupMenu())
	{
		if (GetSelectedCount() == 1)
		{
#if 0
			if (!m_path.IsDirectory())
			{
				if (m_hasWC)
				{
					popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
					popup.AppendMenuIcon(ID_BLAMECOMPARE, IDS_LOG_POPUP_BLAMECOMPARE, IDI_BLAME);
				}
				popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
				popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
				popup.AppendMenu(MF_SEPARATOR, NULL);
				popup.AppendMenuIcon(ID_SAVEAS, IDS_LOG_POPUP_SAVE, IDI_SAVEAS);
				popup.AppendMenuIcon(ID_OPEN, IDS_LOG_POPUP_OPEN, IDI_OPEN);
				popup.AppendMenuIcon(ID_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
				popup.AppendMenuIcon(ID_BLAME, IDS_LOG_POPUP_BLAME, IDI_BLAME);
				popup.AppendMenu(MF_SEPARATOR, NULL);
			}
			else
#endif 
			{
				if (m_hasWC)
				{
					popup.AppendMenuIcon(ID_COMPARE, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
					// TODO:
					// TortoiseMerge could be improved to take a /blame switch
					// and then not 'cat' the files from a unified diff but
					// blame then.
					// But until that's implemented, the context menu entry for
					// this feature is commented out.
					//popup.AppendMenu(ID_BLAMECOMPARE, IDS_LOG_POPUP_BLAMECOMPARE, IDI_BLAME);
				}
				popup.AppendMenuIcon(ID_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF_CH, IDI_DIFF);
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
			
			popup.AppendMenuIcon(ID_SWITCHTOREV, _T("Switch/Checkout to this") , IDI_SWITCH);
			popup.AppendMenuIcon(ID_CREATE_BRANCH, _T("Create Branch at this version") , IDI_COPY);
			popup.AppendMenuIcon(ID_CREATE_TAG, _T("Create Tag at this version"), IDI_COPY);
			popup.AppendMenuIcon(ID_CHERRY_PICK, _T("Cherry Pick this version"), IDI_EXPORT);
			popup.AppendMenuIcon(ID_EXPORT, _T("Export this version"), IDI_EXPORT);
			

			popup.AppendMenu(MF_SEPARATOR, NULL);
		}
		else if (GetSelectedCount() >= 2)
		{
			bool bAddSeparator = false;
			if (IsSelectionContinuous() || (GetSelectedCount() == 2))
			{
				popup.AppendMenuIcon(ID_COMPARETWO, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
			}
			if (GetSelectedCount() == 2)
			{
				//popup.AppendMenuIcon(ID_BLAMETWO, IDS_LOG_POPUP_BLAMEREVS, IDI_BLAME);
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
			popup.AppendMenuIcon(ID_COPYHASH, _T("Copy Commit Hash"));
		}
		if (GetSelectedCount() != 0)
		{
			popup.AppendMenuIcon(ID_COPYCLIPBOARD, IDS_LOG_POPUP_COPYTOCLIPBOARD);
		}
		popup.AppendMenuIcon(ID_FINDENTRY, IDS_LOG_POPUP_FIND);

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
					(LPCTSTR)sDate, (LPCTSTR)pLogEntry->m_AuthorDate.Format(_T("%Y-%m-%d %H:%M")),
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
#if 0
	if (m_bThreadRunning)
		return;
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

int CGitLogListBase::FillGitLog(CTGitPath *path,int info)
{
	ClearText();

	this->m_logEntries.ClearAll();
	this->m_logEntries.ParserFromLog(path,-1,info);

	//this->m_logEntries.ParserFromLog();
	SetItemCountEx(this->m_logEntries.size());

	this->m_arShownList.RemoveAll();

	for(int i=0;i<m_logEntries.size();i++)
	{
		m_logEntries[i].m_IsFull=TRUE;
		this->m_arShownList.Add(&m_logEntries[i]);
	}
	return 0;

}

int CGitLogListBase::FillGitShortLog()
{
	ClearText();

	this->m_logEntries.ClearAll();
	this->m_logEntries.ParserShortLog();

	//this->m_logEntries.ParserFromLog();
	SetItemCountEx(this->m_logEntries.size());

	this->m_arShownList.RemoveAll();

	for(int i=0;i<m_logEntries.size();i++)
		this->m_arShownList.Add(&m_logEntries[i]);

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

int CGitLogListBase::FetchLogAsync(CALLBACK_PROCESS *proc,void * data)
{
	m_ProcCallBack=proc;
	m_ProcData=data;

	InterlockedExchange(&m_bThreadRunning, TRUE);
	InterlockedExchange(&m_bNoDispUpdates, TRUE);
	if (AfxBeginThread(LogThreadEntry, this)==NULL)
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
	for(int i=0;i<m_logEntries.size();i++)
	{
		if(m_logEntries[i].m_AuthorDate.GetTime() < oldest.GetTime())
			oldest = m_logEntries[i].m_AuthorDate.GetTime();

		if(m_logEntries[i].m_AuthorDate.GetTime() > latest.GetTime())
			latest = m_logEntries[i].m_AuthorDate.GetTime();

	}
}

UINT CGitLogListBase::LogThread()
{

	if(m_ProcCallBack)
		m_ProcCallBack(m_ProcData,GITLOG_START);

	InterlockedExchange(&m_bThreadRunning, TRUE);
	InterlockedExchange(&m_bNoDispUpdates, TRUE);

    //does the user force the cache to refresh (shift or control key down)?
    bool refresh =    (GetKeyState (VK_CONTROL) < 0) 
                   || (GetKeyState (VK_SHIFT) < 0);

	//disable the "Get All" button while we're receiving
	//log messages.

	CString temp;
	temp.LoadString(IDS_PROGRESSWAIT);
	ShowText(temp, true);

	FillGitShortLog();
	
	

	RedrawItems(0, m_arShownList.GetCount());
	SetRedraw(false);
	ResizeAllListCtrlCols();
	SetRedraw(true);

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
	InterlockedExchange(&m_bNoDispUpdates, FALSE);

	int index=0;
	int updated=0;
	int percent=0;
	while(1)
	{
		for(int i=0;i<m_logEntries.size();i++)
		{
			if(!m_logEntries.FetchFullInfo(i))
			{
				updated++;
			}
			
			percent=updated*98/m_logEntries.size() + GITLOG_START+1;
			if(percent == GITLOG_END)
				percent == GITLOG_END -1;
			
			if(m_ProcCallBack)
				m_ProcCallBack(m_ProcData,percent);
		}
		if(updated==m_logEntries.size())
			break;
	}

	//RefreshCursor();
	// make sure the filter is applied (if any) now, after we refreshed/fetched
	// the log messages

	

	if(m_ProcCallBack)
		m_ProcCallBack(m_ProcData,GITLOG_END);

	InterlockedExchange(&m_bThreadRunning, FALSE);

	return 0;
}

void CGitLogListBase::Refresh()
{
	if(!m_bThreadRunning)
	{
		this->SetItemCountEx(0);
		m_logEntries.clear();
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
				sRev.Format(_T("%ld"), m_logEntries[i].m_CommitHash);
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
				sRev.Format(_T("%ld"), m_logEntries[i].m_CommitHash);
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
		m_arShownList.Add(&m_logEntries[i]);
	}
//	InterlockedExchange(&m_bNoDispUpdates, FALSE);
	DeleteAllItems();
	SetItemCountEx(ShownCountWithStopped());
	RedrawItems(0, ShownCountWithStopped());
	SetRedraw(false);
	ResizeAllListCtrlCols();
	SetRedraw(true);

	InterlockedExchange(&m_bNoDispUpdates, FALSE);
}