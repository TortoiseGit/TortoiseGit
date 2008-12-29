// GitLogList.cpp : implementation file
//
/*
	Description: qgit revision list view

	Author: Marco Costalba (C) 2005-2007

	Copyright: See COPYING file that comes with this distribution

*/
#include "stdafx.h"
#include "TortoiseProc.h"
#include "GitLogList.h"
#include "GitRev.h"
//#include "VssStyle.h"
#include "IconMenu.h"
// CGitLogList
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




IMPLEMENT_DYNAMIC(CGitLogList, CHintListCtrl)

CGitLogList::CGitLogList():CHintListCtrl()
	,m_regMaxBugIDColWidth(_T("Software\\TortoiseGit\\MaxBugIDColWidth"), 200)
	,m_nSearchIndex(0)
	,m_bNoDispUpdates(FALSE)
	, m_bThreadRunning(FALSE)
	, m_bStrictStopped(false)
	, m_pStoreSelection(NULL)
{
	// use the default GUI font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);

	m_wcRev.m_CommitHash=GIT_REV_ZERO;
	m_wcRev.m_Subject=_T("Working Copy");

	m_hModifiedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONMODIFIED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hReplacedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONREPLACED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hAddedIcon    =  (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONADDED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);
	m_hDeletedIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ACTIONDELETED), IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);

	g_Git.GetMapHashToFriendName(m_HashMap);
}

CGitLogList::~CGitLogList()
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


BEGIN_MESSAGE_MAP(CGitLogList, CHintListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnNMCustomdrawLoglist)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetdispinfoLoglist)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclkLoglist)
	ON_NOTIFY_REFLECT(LVN_ODFINDITEM,OnLvnOdfinditemLoglist)
	ON_WM_CREATE()
END_MESSAGE_MAP()

int CGitLogList:: OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	PreSubclassWindow();
	return CHintListCtrl::OnCreate(lpCreateStruct);
}

void CGitLogList::PreSubclassWindow()
{
	SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_SUBITEMIMAGES);
	// load the icons for the action columns
	m_Theme.SetWindowTheme(GetSafeHwnd(), L"Explorer", NULL);
	CHintListCtrl::PreSubclassWindow();
}

void CGitLogList::InsertGitColumn()
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

	temp.LoadString(IDS_LOG_ACTIONS);
	InsertColumn(this->LOGLIST_ACTION, temp);
	
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

void CGitLogList::ResizeAllListCtrlCols()
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
BOOL CGitLogList::GetShortName(CString ref, CString &shortname,CString prefix)
{
	if(ref.Left(prefix.GetLength()) ==  prefix)
	{
		shortname = ref.Right(ref.GetLength()-prefix.GetLength());
		return TRUE;
	}
	return FALSE;
}
void CGitLogList::FillBackGround(HDC hdc, int Index,CRect &rect)
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

void CGitLogList::DrawTagBranch(HDC hdc,CRect &rect,INT_PTR index)
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

void CGitLogList::paintGraphLane(HDC hdc, int laneHeight,int type, int x1, int x2,
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

void CGitLogList::DrawGraph(HDC hdc,CRect &rect,INT_PTR index)
{
	//todo unfinished
	return;
	GitRev* data = (GitRev*)m_arShownList.GetAt(index);
	CRect rt=rect;
	LVITEM   rItem;
	SecureZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask  = LVIF_STATE;
	rItem.iItem = index;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	static const COLORREF colors[Lanes::COLORS_NUM] = { RGB(0,0,0), RGB(0xFF,0,0), RGB(0,0x1F,0),
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

void CGitLogList::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
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

// CGitLogList message handlers

void CGitLogList::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
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
	if (m_bNoDispUpdates || m_bThreadRunning || bOutOfRange)
		return;

	// Which item number?
	int itemid = pItem->iItem;
	GitRev * pLogEntry = NULL;
	if (itemid < m_arShownList.GetCount())
		pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(pItem->iItem));

	    
	// Which column?
	switch (pItem->iSubItem)
	{
	case this->LOGLIST_GRAPH:	//Graphic
		if (pLogEntry)
		{
		}
		break;
	case this->LOGLIST_ACTION: //action -- no text in the column
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

void CGitLogList::OnContextMenu(CWnd* pWnd, CPoint point)
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
		theApp.DoWaitCursor(1);
		bool bOpenWith = false;

		switch (cmd)
		{
			case ID_GNUDIFF1:
			{
				CString tempfile=GetTempFile();
				CString cmd;
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				cmd.Format(_T("git.exe diff-tree -r -p --stat %s"),r1->m_CommitHash);
				g_Git.RunLogFile(cmd,tempfile);
				CAppUtils::StartUnifiedDiffViewer(tempfile,r1->m_CommitHash.Left(6)+_T(":")+r1->m_Subject);
			}
			break;

			case ID_GNUDIFF2:
			{
				CString tempfile=GetTempFile();
				CString cmd;
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				cmd.Format(_T("git.exe diff-tree -r -p --stat %s %s"),r1->m_CommitHash,r2->m_CommitHash);
				g_Git.RunLogFile(cmd,tempfile);
				CAppUtils::StartUnifiedDiffViewer(tempfile,r1->m_CommitHash.Left(6)+_T(":")+r2->m_CommitHash.Left(6));

			}
			break;

		case ID_COMPARETWO:
			{
				GitRev * r1 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(FirstSelect));
				GitRev * r2 = reinterpret_cast<GitRev*>(m_arShownList.GetAt(LastSelect));
				CFileDiffDlg dlg;
				dlg.SetDiff(NULL,*r1,*r2);
				dlg.DoModal();
				
			}
			break;
		

		case ID_COMPARE:
			{
				GitRev * r1 = &m_wcRev;
				GitRev * r2 = pSelLogEntry;
				CFileDiffDlg dlg;
				dlg.SetDiff(NULL,*r1,*r2);
				dlg.DoModal();

				//user clicked on the menu item "compare with working copy"
				//if (PromptShown())
				//{
				//	GitDiff diff(this, m_hWnd, true);
				//	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
				//	diff.SetHEADPeg(m_LogRevision);
				//	diff.ShowCompare(m_path, GitRev::REV_WC, m_path, revSelected);
				//}
				//else
				//	CAppUtils::StartShowCompare(m_hWnd, m_path, GitRev::REV_WC, m_path, revSelected, GitRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			}
			break;

		case ID_COMPAREWITHPREVIOUS:
			{

				CFileDiffDlg dlg;
				
				if(pSelLogEntry->m_ParentHash.size()>0)
				//if(m_logEntries.m_HashMap[pSelLogEntry->m_ParentHash[0]]>=0)
				{
					dlg.SetDiff(NULL,pSelLogEntry->m_CommitHash,pSelLogEntry->m_ParentHash[0]);
					dlg.DoModal();
				}else
				{
					CMessageBox::Show(NULL,_T("No previous version"),_T("TortoiseGit"),MB_OK);	
				}
				//if (PromptShown())
				//{
				//	GitDiff diff(this, m_hWnd, true);
				//	diff.SetAlternativeTool(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
				//	diff.SetHEADPeg(m_LogRevision);
				//	diff.ShowCompare(CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected);
				//}
				//else
				//	CAppUtils::StartShowCompare(m_hWnd, CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected, GitRev(), m_LogRevision, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			}
			break;
		case ID_COPYCLIPBOARD:
			{
				CopySelectionToClipBoard();
			}
			break;
		case ID_COPYHASH:
			{
				CopySelectionToClipBoard(TRUE);
			}
			break;
		case ID_EXPORT:
			CAppUtils::Export(&pSelLogEntry->m_CommitHash);
			break;
		case ID_CREATE_BRANCH:
			CAppUtils::CreateBranchTag(FALSE,&pSelLogEntry->m_CommitHash);
			m_HashMap.clear();
			g_Git.GetMapHashToFriendName(m_HashMap);
			Invalidate();			
			break;
		case ID_CREATE_TAG:
			CAppUtils::CreateBranchTag(TRUE,&pSelLogEntry->m_CommitHash);
			m_HashMap.clear();
			g_Git.GetMapHashToFriendName(m_HashMap);
			Invalidate();
			break;
		case ID_SWITCHTOREV:
			CAppUtils::Switch(&pSelLogEntry->m_CommitHash);
			break;

		default:
			//CMessageBox::Show(NULL,_T("Have not implemented"),_T("TortoiseGit"),MB_OK);
			break;
#if 0
	
		case ID_REVERTREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseGit"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}
				CString msg;
				msg.Format(IDS_LOG_REVERT_CONFIRM, m_path.GetWinPath());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CGitProgressDlg dlg;
					dlg.SetCommand(CGitProgressDlg::GitProgress_Merge);
					dlg.SetPathList(CTGitPathList(m_path));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					revisionRanges.AdjustForMerge(true);
					dlg.SetRevisionRanges(revisionRanges);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
		case ID_MERGEREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseGit"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}

				CString path = m_path.GetWinPathString();
				bool bGotSavePath = false;
				if ((GetSelectedCount() == 1)&&(!m_path.IsDirectory()))
				{
					bGotSavePath = CAppUtils::FileOpenSave(path, NULL, IDS_LOG_MERGETO, IDS_COMMONFILEFILTER, true, GetSafeHwnd());
				}
				else
				{
					CBrowseFolder folderBrowser;
					folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
					bGotSavePath = (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK);
				}
				if (bGotSavePath)
				{
					CGitProgressDlg dlg;
					dlg.SetCommand(CGitProgressDlg::GitProgress_Merge);
					dlg.SetPathList(CTGitPathList(CTGitPath(path)));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					revisionRanges.AdjustForMerge(false);
					dlg.SetRevisionRanges(revisionRanges);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
		case ID_REVERTTOREV:
			{
				// we need an URL to complete this command, so error out if we can't get an URL
				if (pathURL.IsEmpty())
				{
					CString strMessage;
					strMessage.Format(IDS_ERR_NOURLOFFILE, (LPCTSTR)(m_path.GetUIPathString()));
					CMessageBox::Show(this->m_hWnd, strMessage, _T("TortoiseGit"), MB_ICONERROR);
					TRACE(_T("could not retrieve the URL of the folder!\n"));
					break;		//exit
				}

				CString msg;
				msg.Format(IDS_LOG_REVERTTOREV_CONFIRM, m_path.GetWinPath());
				if (CMessageBox::Show(this->m_hWnd, msg, _T("TortoiseGit"), MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					CGitProgressDlg dlg;
					dlg.SetCommand(CGitProgressDlg::GitProgress_Merge);
					dlg.SetPathList(CTGitPathList(m_path));
					dlg.SetUrl(pathURL);
					dlg.SetSecondUrl(pathURL);
					GitRevRangeArray revarray;
					revarray.AddRevRange(GitRev::REV_HEAD, revSelected);
					dlg.SetRevisionRanges(revarray);
					dlg.SetPegRevision(m_LogRevision);
					dlg.DoModal();
				}
			}
			break;
	

	
		case ID_BLAMECOMPARE:
			{
				//user clicked on the menu item "compare with working copy"
				//now first get the revision which is selected
				if (PromptShown())
				{
					GitDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(m_path, GitRev::REV_BASE, m_path, revSelected, GitRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, m_path, GitRev::REV_BASE, m_path, revSelected, GitRev(), m_LogRevision, false, false, true);
			}
			break;
		case ID_BLAMETWO:
			{
				//user clicked on the menu item "compare and blame revisions"
				if (PromptShown())
				{
					GitDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTGitPath(pathURL), revSelected2, CTGitPath(pathURL), revSelected, GitRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTGitPath(pathURL), revSelected2, CTGitPath(pathURL), revSelected, GitRev(), m_LogRevision, false, false, true);
			}
			break;
		case ID_BLAMEWITHPREVIOUS:
			{
				//user clicked on the menu item "Compare and Blame with previous revision"
				if (PromptShown())
				{
					GitDiff diff(this, this->m_hWnd, true);
					diff.SetHEADPeg(m_LogRevision);
					diff.ShowCompare(CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected, GitRev(), false, true);
				}
				else
					CAppUtils::StartShowCompare(m_hWnd, CTGitPath(pathURL), revPrevious, CTGitPath(pathURL), revSelected, GitRev(), m_LogRevision, false, false, true);
			}
			break;
		
		case ID_OPENWITH:
			bOpenWith = true;
		case ID_OPEN:
			{
				CProgressDlg progDlg;
				progDlg.SetTitle(IDS_APPNAME);
				progDlg.SetAnimation(IDR_DOWNLOAD);
				CString sInfoLine;
				sInfoLine.Format(IDS_PROGRESSGETFILEREVISION, m_path.GetWinPath(), (LPCTSTR)revSelected.ToString());
				progDlg.SetLine(1, sInfoLine, true);
				SetAndClearProgressInfo(&progDlg);
				progDlg.ShowModeless(m_hWnd);
				CTGitPath tempfile = CTempFiles::Instance().GetTempFilePath(false, m_path, revSelected);
				bool bSuccess = true;
				if (!Cat(m_path, GitRev(GitRev::REV_HEAD), revSelected, tempfile))
				{
					bSuccess = false;
					// try again, but with the selected revision as the peg revision
					if (!Cat(m_path, revSelected, revSelected, tempfile))
					{
						progDlg.Stop();
						SetAndClearProgressInfo((HWND)NULL);
						CMessageBox::Show(this->m_hWnd, GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
						EnableOKButton();
						break;
					}
					bSuccess = true;
				}
				if (bSuccess)
				{
					progDlg.Stop();
					SetAndClearProgressInfo((HWND)NULL);
					SetFileAttributes(tempfile.GetWinPath(), FILE_ATTRIBUTE_READONLY);
					int ret = 0;
					if (!bOpenWith)
						ret = (int)ShellExecute(this->m_hWnd, NULL, tempfile.GetWinPath(), NULL, NULL, SW_SHOWNORMAL);
					if ((ret <= HINSTANCE_ERROR)||bOpenWith)
					{
						CString cmd = _T("RUNDLL32 Shell32,OpenAs_RunDLL ");
						cmd += tempfile.GetWinPathString() + _T(" ");
						CAppUtils::LaunchApplication(cmd, NULL, false);
					}
				}
			}
			break;
		case ID_BLAME:
			{
				CBlameDlg dlg;
				dlg.EndRev = revSelected;
				if (dlg.DoModal() == IDOK)
				{
					CBlame blame;
					CString tempfile;
					CString logfile;
					tempfile = blame.BlameToTempFile(m_path, dlg.StartRev, dlg.EndRev, dlg.EndRev, logfile, _T(""), dlg.m_bIncludeMerge, TRUE, TRUE);
					if (!tempfile.IsEmpty())
					{
						if (dlg.m_bTextView)
						{
							//open the default text editor for the result file
							CAppUtils::StartTextViewer(tempfile);
						}
						else
						{
							CString sParams = _T("/path:\"") + m_path.GetGitPathString() + _T("\" ");
							if(!CAppUtils::LaunchTortoiseBlame(tempfile, logfile, CPathUtils::GetFileNameFromPath(m_path.GetFileOrDirectoryName()),sParams))
							{
								break;
							}
						}
					}
					else
					{
						CMessageBox::Show(this->m_hWnd, blame.GetLastErrorMessage(), _T("TortoiseGit"), MB_ICONERROR);
					}
				}
			}
			break;
		case ID_UPDATE:
			{
				CString sCmd;
				CString url = _T("tgit:")+pathURL;
				sCmd.Format(_T("%s /command:update /path:\"%s\" /rev:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)m_path.GetWinPath(), (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_FINDENTRY:
			{
				m_nSearchIndex = GetSelectionMark();
				if (m_nSearchIndex < 0)
					m_nSearchIndex = 0;
				if (m_pFindDialog)
				{
					break;
				}
				else
				{
					m_pFindDialog = new CFindReplaceDialog();
					m_pFindDialog->Create(TRUE, NULL, NULL, FR_HIDEUPDOWN | FR_HIDEWHOLEWORD, this);									
				}
			}
			break;
		case ID_REPOBROWSE:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:repobrowser /path:\"%s\" /rev:%s"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)pathURL, (LPCTSTR)revSelected.ToString());

				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_EDITLOG:
			{
				EditLogMessage(selIndex);
			}
			break;
		case ID_EDITAUTHOR:
			{
				EditAuthor(selEntries);
			}
			break;
		case ID_REVPROPS:
			{
				CEditPropertiesDlg dlg;
				dlg.SetProjectProperties(&m_ProjectProperties);
				CTGitPathList escapedlist;
				dlg.SetPathList(CTGitPathList(CTGitPath(pathURL)));
				dlg.SetRevision(revSelected);
				dlg.RevProps(true);
				dlg.DoModal();
			}
			break;
		
		case ID_EXPORT:
			{
				CString sCmd;
				sCmd.Format(_T("%s /command:export /path:\"%s\" /revision:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)pathURL, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_CHECKOUT:
			{
				CString sCmd;
				CString url = _T("tgit:")+pathURL;
				sCmd.Format(_T("%s /command:checkout /url:\"%s\" /revision:%ld"),
					(LPCTSTR)(CPathUtils::GetAppDirectory()+_T("TortoiseProc.exe")),
					(LPCTSTR)url, (LONG)revSelected);
				CAppUtils::LaunchApplication(sCmd, NULL, false);
			}
			break;
		case ID_VIEWREV:
			{
				CString url = m_ProjectProperties.sWebViewerRev;
				url = GetAbsoluteUrlFromRelativeUrl(url);
				url.Replace(_T("%REVISION%"), revSelected.ToString());
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
		case ID_VIEWPATHREV:
			{
				CString relurl = pathURL;
				CString sRoot = GetRepositoryRoot(CTGitPath(relurl));
				relurl = relurl.Mid(sRoot.GetLength());
				CString url = m_ProjectProperties.sWebViewerPathRev;
				url = GetAbsoluteUrlFromRelativeUrl(url);
				url.Replace(_T("%REVISION%"), revSelected.ToString());
				url.Replace(_T("%PATH%"), relurl);
				if (!url.IsEmpty())
					ShellExecute(this->m_hWnd, _T("open"), url, NULL, NULL, SW_SHOWDEFAULT);					
			}
			break;
#endif
		
		} // switch (cmd)
		theApp.DoWaitCursor(-1);
//		EnableOKButton();
	} // if (popup.CreatePopupMenu())

}

bool CGitLogList::IsSelectionContinuous()
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

void CGitLogList::CopySelectionToClipBoard(bool HashOnly)
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

void CGitLogList::DiffSelectedRevWithPrevious()
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

void CGitLogList::OnLvnOdfinditemLoglist(NMHDR *pNMHDR, LRESULT *pResult)
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

int CGitLogList::FillGitShortLog()
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

BOOL CGitLogList::PreTranslateMessage(MSG* pMsg)
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

void CGitLogList::OnNMDblclkLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// a double click on an entry in the revision list has happened
	*pResult = 0;

  if (CRegDWORD(_T("Software\\TortoiseGit\\DiffByDoubleClickInLog"), FALSE))
	  DiffSelectedRevWithPrevious();
}

int CGitLogList::FetchLogAsync(CALLBACK_PROCESS *proc,void * data)
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
UINT CGitLogList::LogThreadEntry(LPVOID pVoid)
{
	return ((CGitLogList*)pVoid)->LogThread();
}


UINT CGitLogList::LogThread()
{

	if(m_ProcCallBack)
		m_ProcCallBack(m_ProcData,GITLOG_START);

	InterlockedExchange(&m_bThreadRunning, TRUE);

    //does the user force the cache to refresh (shift or control key down)?
    bool refresh =    (GetKeyState (VK_CONTROL) < 0) 
                   || (GetKeyState (VK_SHIFT) < 0);

	//disable the "Get All" button while we're receiving
	//log messages.

	CString temp;
	temp.LoadString(IDS_PROGRESSWAIT);
	ShowText(temp, true);

	FillGitShortLog();
	
	InterlockedExchange(&m_bThreadRunning, FALSE);

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

	return 0;
}

