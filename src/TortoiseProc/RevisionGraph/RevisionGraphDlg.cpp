// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "TortoiseProc.h"
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "AppUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include "RevGraphFilterDlg.h"
#include ".\revisiongraphdlg.h"
#include "RepositoryInfo.h"
#include "RevisionInRange.h"
#include "RemovePathsBySubString.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

IMPLEMENT_DYNAMIC(CRevisionGraphDlg, CResizableStandAloneDialog)
CRevisionGraphDlg::CRevisionGraphDlg(CWnd* pParent /*=NULL*/)
	: CResizableStandAloneDialog(CRevisionGraphDlg::IDD, pParent)
	, m_hAccel(NULL)
	, m_bFetchLogs(true)
	, m_fZoomFactor(0.5)
{
    // GDI+ initialization

    GdiplusStartupInput input;
    GdiplusStartup (&m_gdiPlusToken, &input, NULL);

    // restore option state

	DWORD dwOpts = CRegStdWORD(_T("Software\\TortoiseGit\\RevisionGraphOptions"), 0x211);
    m_options.SetRegistryFlags (dwOpts, 0x3ff);
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
    // save option state

	CRegStdWORD regOpts = CRegStdWORD(_T("Software\\TortoiseGit\\RevisionGraphOptions"), 1);
    regOpts = m_options.GetRegistryFlags();

    // GDI+ cleanup

    GdiplusShutdown (m_gdiPlusToken);
}

void CRevisionGraphDlg::DoDataExchange(CDataExchange* pDX)
{
	CResizableStandAloneDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRevisionGraphDlg, CResizableStandAloneDialog)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_VIEW_ZOOMIN, OnViewZoomin)
	ON_COMMAND(ID_VIEW_ZOOMOUT, OnViewZoomout)
	ON_COMMAND(ID_VIEW_ZOOM100, OnViewZoom100)
	ON_COMMAND(ID_VIEW_ZOOMALL, OnViewZoomAll)
	ON_COMMAND(ID_MENUEXIT, OnMenuexit)
	ON_COMMAND(ID_MENUHELP, OnMenuhelp)
	ON_COMMAND(ID_VIEW_COMPAREHEADREVISIONS, OnViewCompareheadrevisions)
	ON_COMMAND(ID_VIEW_COMPAREREVISIONS, OnViewComparerevisions)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFF, OnViewUnifieddiff)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, OnViewUnifieddiffofheadrevisions)
	ON_COMMAND(ID_FILE_SAVEGRAPHAS, &CRevisionGraphDlg::OnFileSavegraphas)
	ON_COMMAND_EX(ID_VIEW_SHOWALLREVISIONS, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_GROUPBRANCHES, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_TOPDOWN, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_SHOWHEAD, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_EXACTCOPYSOURCE, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_FOLDTAGS, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_REDUCECROSSLINES, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_REMOVEDELETEDONES, &CRevisionGraphDlg::OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_SHOWWCREV, &CRevisionGraphDlg::OnToggleOption)
	ON_CBN_SELCHANGE(ID_REVGRAPH_ZOOMCOMBO, OnChangeZoom)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_COMMAND(ID_VIEW_FILTER, &CRevisionGraphDlg::OnViewFilter)
	ON_COMMAND(ID_VIEW_SHOWOVERVIEW, &CRevisionGraphDlg::OnViewShowoverview)
END_MESSAGE_MAP()

BOOL CRevisionGraphDlg::InitializeToolbar()
{
	// set up the toolbar
	// add the tool bar to the dialog
	m_ToolBar.CreateEx(this, TBSTYLE_FLAT | TBSTYLE_WRAPABLE | TBSTYLE_TRANSPARENT | CBRS_SIZE_DYNAMIC);
	m_ToolBar.LoadToolBar(IDR_REVGRAPHBAR);
	m_ToolBar.ShowWindow(SW_SHOW);
	m_ToolBar.SetBarStyle(CBRS_ALIGN_TOP | CBRS_TOOLTIPS | CBRS_FLYBY);

	// toolbars aren't true-color without some tweaking:
	{
		CImageList	cImageList;
		CBitmap		cBitmap;
		BITMAP		bmBitmap;

		cBitmap.Attach(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHBAR),
			IMAGE_BITMAP, 0, 0,
			LR_DEFAULTSIZE|LR_CREATEDIBSECTION));
		cBitmap.GetBitmap(&bmBitmap);

		CSize		cSize(bmBitmap.bmWidth, bmBitmap.bmHeight); 
		int			nNbBtn = cSize.cx/20;
		RGBTRIPLE *	rgb	= (RGBTRIPLE*)(bmBitmap.bmBits);
		COLORREF	rgbMask	= RGB(rgb[0].rgbtRed, rgb[0].rgbtGreen, rgb[0].rgbtBlue);

		cImageList.Create(20, cSize.cy, ILC_COLOR32|ILC_MASK, nNbBtn, 0);
		cImageList.Add(&cBitmap, rgbMask);
		m_ToolBar.SendMessage(TB_SETIMAGELIST, 0, (LPARAM)cImageList.m_hImageList);
		cImageList.Detach(); 
		cBitmap.Detach();
	}
	RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);

#define SNAP_WIDTH 60 //the width of the combo box
	// set up the ComboBox control as a snap mode select box
	// First get the index of the placeholders position in the toolbar
	int index = 0;
	while (m_ToolBar.GetItemID(index) != ID_REVGRAPH_ZOOMCOMBO) index++;

	// next convert that button to a separator and get its position
	m_ToolBar.SetButtonInfo(index, ID_REVGRAPH_ZOOMCOMBO, TBBS_SEPARATOR,
		SNAP_WIDTH);
	RECT rect;
	m_ToolBar.GetItemRect(index, &rect);

	// expand the rectangle to allow the combo box room to drop down
	rect.top+=3;
	rect.bottom += 200;

	// then create the combo box and show it
	if (!m_ToolBar.m_ZoomCombo.CreateEx(WS_EX_RIGHT, WS_CHILD|WS_VISIBLE|CBS_AUTOHSCROLL|CBS_DROPDOWN,
		rect, &m_ToolBar, ID_REVGRAPH_ZOOMCOMBO))
	{
		TRACE0("Failed to create combo-box\n");
		return FALSE;
	}
	m_ToolBar.m_ZoomCombo.ShowWindow(SW_SHOW);

	// set toolbar button styles

	UINT styles[] = { TBBS_CHECKBOX|TBBS_CHECKED
					, TBBS_CHECKBOX
					, 0};

	UINT itemIDs[] = { ID_VIEW_GROUPBRANCHES 
					 , 0		// separate styles by "0"
					 , ID_VIEW_SHOWOVERVIEW
					 , ID_VIEW_TOPDOWN
					 , ID_VIEW_SHOWHEAD
					 , ID_VIEW_EXACTCOPYSOURCE
					 , ID_VIEW_FOLDTAGS
					 , ID_VIEW_REDUCECROSSLINES
                     , ID_VIEW_REMOVEDELETEDONES
                     , ID_VIEW_SHOWWCREV
					 , 0};

	for (UINT* itemID = itemIDs, *style = styles; *style != 0; ++itemID)
	{
		if (*itemID == 0)
		{
			++style;
			continue;
		}

		int index = 0;
		while (m_ToolBar.GetItemID(index) != *itemID)
			index++;
		m_ToolBar.SetButtonStyle(index, m_ToolBar.GetButtonStyle(index)|*style);
	}

	// fill the combo box

	TCHAR* texts[] = { _T("5%")
					 , _T("10%")
					 , _T("20%")
					 , _T("40%")
					 , _T("50%")
					 , _T("75%")
					 , _T("100%")
					 , _T("200%")
					 , NULL};

	COMBOBOXEXITEM cbei;
	SecureZeroMemory(&cbei, sizeof cbei);
	cbei.mask = CBEIF_TEXT;

	for (TCHAR** text = texts; *text != NULL; ++text)
	{
		cbei.pszText = *text;
		m_ToolBar.m_ZoomCombo.InsertItem(&cbei);
	}

	m_ToolBar.m_ZoomCombo.SetCurSel(1);

	return TRUE;
}

BOOL CRevisionGraphDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();

	EnableToolTips();

	// set up the status bar
	m_StatusBar.Create(WS_CHILD|WS_VISIBLE|SBT_OWNERDRAW,
		CRect(0,0,0,0), this, 1);
	int strPartDim[2]= {120, -1};
	m_StatusBar.SetParts(2, strPartDim);

	if (InitializeToolbar() != TRUE)
		return FALSE;

    for (size_t i = 0; i < m_options.count(); ++i)
        if (m_options[i]->CommandID() != 0)
        	SetOption (m_options[i]->CommandID());

	CMenu * pMenu = GetMenu();
	if (pMenu)
	{
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\ShowRevGraphOverview"), FALSE);
		m_Graph.SetShowOverview ((DWORD)reg != FALSE);
		pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | (DWORD(reg) ? MF_CHECKED : 0));
		int tbstate = m_ToolBar.GetToolBarCtrl().GetState(ID_VIEW_SHOWOVERVIEW);
		m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbstate | (DWORD(reg) ? TBSTATE_CHECKED : 0));
	}

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(),MAKEINTRESOURCE(IDR_ACC_REVISIONGRAPH));

	CRect graphrect = GetGraphRect();
	m_Graph.Init(this, &graphrect);
	m_Graph.SetOwner(this);
	m_Graph.UpdateWindow();
    DoZoom (0.75);

	EnableSaveRestore(_T("RevisionGraphDlg"));

	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
	if (hWndExplorer)
		CenterWindow(CWnd::FromHandle(hWndExplorer));
	return TRUE;  // return TRUE unless you set the focus to a control
}

UINT CRevisionGraphDlg::WorkerThread(LPVOID pVoid)
{
	CRevisionGraphDlg*	pDlg;
	pDlg = (CRevisionGraphDlg*)pVoid;
	InterlockedExchange(&pDlg->m_Graph.m_bThreadRunning, TRUE);
	CoInitialize(NULL);

    if (pDlg->m_bFetchLogs)
    {
	    pDlg->m_Graph.m_pProgress = new CProgressDlg();
	    pDlg->m_Graph.m_pProgress->SetTitle(IDS_REVGRAPH_PROGTITLE);
	    pDlg->m_Graph.m_pProgress->SetCancelMsg(IDS_REVGRAPH_PROGCANCEL);
	    pDlg->m_Graph.m_pProgress->SetTime();
	    pDlg->m_Graph.m_pProgress->SetProgress(0, 100);

        svn_revnum_t pegRev = pDlg->m_Graph.m_pegRev.IsNumber()
                            ? (svn_revnum_t)pDlg->m_Graph.m_pegRev
                            : (svn_revnum_t)-1;

	    if (!pDlg->m_Graph.FetchRevisionData (pDlg->m_Graph.m_sPath, pegRev, pDlg->m_options))
		    CMessageBox::Show (pDlg->m_hWnd, pDlg->m_Graph.GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);

        pDlg->m_Graph.m_pProgress->Stop();
        delete pDlg->m_Graph.m_pProgress;
        pDlg->m_Graph.m_pProgress = NULL;

    	pDlg->m_bFetchLogs = false;	// we've got the logs, no need to fetch them a second time
    }

    // standard plus user settings

    pDlg->m_options.Prepare();
    if (pDlg->m_Graph.AnalyzeRevisionData (pDlg->m_options))
        pDlg->UpdateStatusBar();

	CoUninitialize();
	InterlockedExchange(&pDlg->m_Graph.m_bThreadRunning, FALSE);

    pDlg->m_Graph.SendMessage (CRevisionGraphWnd::WM_WORKERTHREADDONE, 0, 0);
	return 0;
}

void CRevisionGraphDlg::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	CRect rect;
	GetClientRect(&rect);
	if (IsWindow(m_ToolBar))
	{
		RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0);
	}
	if (IsWindow(m_StatusBar))
	{
		CRect statusbarrect;
		m_StatusBar.GetClientRect(&statusbarrect);
		statusbarrect.top = rect.bottom - statusbarrect.top + statusbarrect.bottom;
		m_StatusBar.MoveWindow(&statusbarrect);
	}
	if (IsWindow(m_Graph))
	{
		m_Graph.MoveWindow (GetGraphRect());
	}
}

BOOL CRevisionGraphDlg::PreTranslateMessage(MSG* pMsg)
{
#define SCROLL_STEP  20
	if (pMsg->message == WM_KEYDOWN)
	{
		int pos = 0;
		switch (pMsg->wParam)
		{
		case VK_UP:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos - SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_DOWN:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos + SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_LEFT:
			pos = m_Graph.GetScrollPos(SB_HORZ);
			m_Graph.SetScrollPos(SB_HORZ, pos - SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_RIGHT:
			pos = m_Graph.GetScrollPos(SB_HORZ);
			m_Graph.SetScrollPos(SB_HORZ, pos + SCROLL_STEP);
			m_Graph.Invalidate();
			break;
		case VK_PRIOR:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos - GetGraphRect().Height() / 2);
			m_Graph.Invalidate();
			break;
		case VK_NEXT:
			pos = m_Graph.GetScrollPos(SB_VERT);
			m_Graph.SetScrollPos(SB_VERT, pos + GetGraphRect().Height() / 2);
			m_Graph.Invalidate();
			break;
		case VK_F5:
	        m_Graph.SetDlgTitle (false);

            SVN svn;
        	LogCache::CRepositoryInfo& cachedProperties 
                = svn.GetLogCachePool()->GetRepositoryInfo();
            CString root = m_Graph.GetRepositoryRoot();
            CString uuid = m_Graph.GetRepositoryUUID();

            cachedProperties.ResetHeadRevision (uuid, root);

            m_bFetchLogs = true;
            StartWorkerThread();

			break;
		}
	}
	if ((m_hAccel)&&(pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST))
	{
		return TranslateAccelerator(m_hWnd,m_hAccel,pMsg);
	}
	return __super::PreTranslateMessage(pMsg);
}

void CRevisionGraphDlg::DoZoom (float zoom)
{
    m_fZoomFactor = zoom;
    m_Graph.DoZoom (zoom);
    UpdateZoomBox();
}

void CRevisionGraphDlg::OnViewZoomin()
{
    DoZoom (min (2.0f, m_fZoomFactor / .9f));
}

void CRevisionGraphDlg::OnViewZoomout()
{
    DoZoom (max (0.01f, m_fZoomFactor * .9f));
}

void CRevisionGraphDlg::OnViewZoom100()
{
	DoZoom (1.0);
}

void CRevisionGraphDlg::OnViewZoomAll()
{
	// zoom the graph so that it is completely visible in the window
	CRect windowrect = GetGraphRect();
    CRect viewrect = m_Graph.GetViewRect();

	float horzfact = float(viewrect.Width())/float(windowrect.Width()-6);
	float vertfact = float(viewrect.Height())/float(windowrect.Height()-6);

    DoZoom (1.0f/(max (1.0f, max(horzfact, vertfact))));
}

void CRevisionGraphDlg::OnMenuexit()
{
	if (!m_Graph.m_bThreadRunning)
		EndDialog(IDOK);
}

void CRevisionGraphDlg::OnMenuhelp()
{
	OnHelp();
}

void CRevisionGraphDlg::OnViewCompareheadrevisions()
{
	m_Graph.CompareRevs(true);
}

void CRevisionGraphDlg::OnViewComparerevisions()
{
	m_Graph.CompareRevs(false);
}

void CRevisionGraphDlg::OnViewUnifieddiff()
{
	m_Graph.UnifiedDiffRevs(false);
}

void CRevisionGraphDlg::OnViewUnifieddiffofheadrevisions()
{
	m_Graph.UnifiedDiffRevs(true);
}

void CRevisionGraphDlg::SetOption (UINT controlID)
{
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;

	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(controlID);
    if (tbstate != -1)
    {
        if (m_options.IsSelected (controlID))
	    {
		    pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_CHECKED);
		    m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate | TBSTATE_CHECKED);
	    }
	    else
	    {
		    pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_UNCHECKED);
		    m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate & (~TBSTATE_CHECKED));
	    }
    }
}

BOOL CRevisionGraphDlg::OnToggleOption (UINT controlID)
{
    // check request for validity

	if (m_Graph.m_bThreadRunning)
	{
		int state = m_ToolBar.GetToolBarCtrl().GetState(controlID);
		if (state & TBSTATE_CHECKED)
			state &= ~TBSTATE_CHECKED;
		else
			state |= TBSTATE_CHECKED;
		m_ToolBar.GetToolBarCtrl().SetState (controlID, state);
		return FALSE;
	}

	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return FALSE;

    // actually toggle the option

	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(controlID);
	UINT state = pMenu->GetMenuState(controlID, MF_BYCOMMAND);
	if (state & MF_CHECKED)
	{
		pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_UNCHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate & (~TBSTATE_CHECKED));
	}
	else
	{
		pMenu->CheckMenuItem(controlID, MF_BYCOMMAND | MF_CHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(controlID, tbstate | TBSTATE_CHECKED);
	}

    if (((state & MF_CHECKED) != 0) == m_options.IsSelected (controlID))
        m_options.ToggleSelection (controlID);

    // re-process the data

    StartWorkerThread();

    return TRUE;
}

void CRevisionGraphDlg::StartWorkerThread()
{
	if (InterlockedExchange(&m_Graph.m_bThreadRunning, TRUE) == TRUE)
        return;

	if (AfxBeginThread(WorkerThread, this)==NULL)
	{
		CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	    InterlockedExchange(&m_Graph.m_bThreadRunning, FALSE);
	}
}

void CRevisionGraphDlg::OnCancel()
{
	if (!m_Graph.m_bThreadRunning)
		__super::OnCancel();
}

void CRevisionGraphDlg::OnOK()
{
	OnChangeZoom();
}

void CRevisionGraphDlg::OnFileSavegraphas()
{
	CString tempfile;
	int filterindex = 0;
	if (CAppUtils::FileOpenSave(tempfile, &filterindex, IDS_REVGRAPH_SAVEPIC, IDS_PICTUREFILEFILTER, false, m_hWnd))
	{
		// if the user doesn't specify a file extension, default to
		// wmf and add that extension to the filename. But only if the
		// user chose the 'pictures' filter. The filename isn't changed
		// if the 'All files' filter was chosen.
		CString extension;
		int dotPos = tempfile.ReverseFind('.');
		int slashPos = tempfile.ReverseFind('\\');
		if (dotPos > slashPos)
			extension = tempfile.Mid(dotPos);
		if ((filterindex == 1)&&(extension.IsEmpty()))
		{
			extension = _T(".wmf");
			tempfile += extension;
		}
		m_Graph.SaveGraphAs(tempfile);
	}
}

CRect CRevisionGraphDlg::GetGraphRect()
{
    CRect rect;
    GetClientRect(&rect);

    CRect statusbarrect;
    m_StatusBar.GetClientRect(&statusbarrect);
    rect.bottom -= statusbarrect.Height();

    CRect toolbarrect;
    m_ToolBar.GetClientRect(&toolbarrect);
    rect.top += toolbarrect.Height();

    return rect;
}

void CRevisionGraphDlg::UpdateStatusBar()
{
	CString sFormat;
	sFormat.Format(IDS_REVGRAPH_STATUSBARURL, (LPCTSTR)m_Graph.m_sPath);
	m_StatusBar.SetText(sFormat,1,0);
	sFormat.Format(IDS_REVGRAPH_STATUSBARNUMNODES, m_Graph.GetNodeCount());
	m_StatusBar.SetText(sFormat,0,0);
}

void CRevisionGraphDlg::OnChangeZoom()
{
	if (!IsWindow(m_Graph.GetSafeHwnd()))
		return;
	CString strText;
	CString strItem;
	CComboBoxEx* pCBox = (CComboBoxEx*)m_ToolBar.GetDlgItem(ID_REVGRAPH_ZOOMCOMBO);
	pCBox->GetWindowText(strItem);
	if (strItem.IsEmpty())
		return;
	ATLTRACE(_T("OnChangeZoom to %s\n"), strItem);

    DoZoom ((float)(_tstof(strItem)/100.0));
}

void CRevisionGraphDlg::UpdateZoomBox()
{
	CString strText;
	CString strItem;
	CComboBoxEx* pCBox = (CComboBoxEx*)m_ToolBar.GetDlgItem(ID_REVGRAPH_ZOOMCOMBO);
	pCBox->GetWindowText(strItem);
	strText.Format(_T("%.0f%%"), (m_fZoomFactor*100.0));
	if (strText.Compare(strItem) != 0)
		pCBox->SetWindowText(strText);
}

BOOL CRevisionGraphDlg::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;

	UINT_PTR nID = pNMHDR->idFrom;

	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool 
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (nID != 0) // will be zero on a separator
	{
		strTipText.LoadString (static_cast<UINT>(nID));
	}

	*pResult = 0;
	if (strTipText.IsEmpty())
		return TRUE;

	if (strTipText.GetLength() >= MAX_TT_LENGTH)
		strTipText = strTipText.Left(MAX_TT_LENGTH);

	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
	}
	else
	{
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 600);
		lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
		pTTTW->lpszText = m_wszTip;
	}
	// bring the tooltip window above other pop up windows
	::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
		SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	return TRUE;    // message was handled
}

void CRevisionGraphDlg::OnViewFilter()
{
    CRevisionInRange* revisionRange = m_options.GetOption<CRevisionInRange>();
    svn_revnum_t head = m_Graph.GetHeadRevision();
    svn_revnum_t lowerLimit = revisionRange->GetLowerLimit();
    svn_revnum_t upperLimit = revisionRange->GetUpperLimit();

	CRevGraphFilterDlg dlg;
	dlg.SetMaxRevision (head);
	dlg.SetFilterString (m_sFilter);
    dlg.SetRevisionRange ( min (head, lowerLimit == -1 ? 1 : lowerLimit)
                         , min (head, upperLimit == -1 ? head : upperLimit));

    if (dlg.DoModal()==IDOK)
	{
		// user pressed OK to dismiss the dialog, which means
		// we have to accept the new filter settings and apply them
		svn_revnum_t minrev, maxrev;
		dlg.GetRevisionRange(minrev, maxrev);
		m_sFilter = dlg.GetFilterString();

        revisionRange->SetLowerLimit (minrev);
        revisionRange->SetUpperLimit (maxrev);

        std::set<std::string>& filterPaths 
            = m_options.GetOption<CRemovePathsBySubString>()->GetFilterPaths();

        int index = 0;
        filterPaths.clear();

        CString path = m_sFilter.Tokenize (_T("*"),  index);
        while (!path.IsEmpty())
        {
            filterPaths.insert (CUnicodeUtils::StdGetUTF8 ((LPCTSTR)path));
            path = m_sFilter.Tokenize (_T("*"),  index);
        }

        InterlockedExchange(&m_Graph.m_bThreadRunning, TRUE);

		if (AfxBeginThread(WorkerThread, this)==NULL)
		{
			CMessageBox::Show(this->m_hWnd, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
	}
}

void CRevisionGraphDlg::OnViewShowoverview()
{
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;
	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(ID_VIEW_SHOWOVERVIEW);
	UINT state = pMenu->GetMenuState(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND);
	if (state & MF_CHECKED)
	{
		pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | MF_UNCHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbstate & (~TBSTATE_CHECKED));
		m_Graph.SetShowOverview (false);
	}
	else
	{
		pMenu->CheckMenuItem(ID_VIEW_SHOWOVERVIEW, MF_BYCOMMAND | MF_CHECKED);
		m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_SHOWOVERVIEW, tbstate | TBSTATE_CHECKED);
		m_Graph.SetShowOverview (true);
	}

	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseGit\\ShowRevGraphOverview"), FALSE);
	reg = m_Graph.GetShowOverview();
	m_Graph.Invalidate(FALSE);
}







