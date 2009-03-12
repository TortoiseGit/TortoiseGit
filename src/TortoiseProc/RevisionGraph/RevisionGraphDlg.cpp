// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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

	DWORD dwOpts = CRegStdDWORD(_T("Software\\TortoiseSVN\\RevisionGraphOptions"), 0x1ff199);
    m_Graph.m_state.GetOptions()->SetRegistryFlags (dwOpts, 0x7fbf);
}

CRevisionGraphDlg::~CRevisionGraphDlg()
{
    // save option state

	CRegStdDWORD regOpts = CRegStdDWORD(_T("Software\\TortoiseSVN\\RevisionGraphOptions"), 1);
    regOpts = m_Graph.m_state.GetOptions()->GetRegistryFlags();

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
	ON_COMMAND(ID_VIEW_ZOOMHEIGHT, OnViewZoomHeight)
	ON_COMMAND(ID_VIEW_ZOOMWIDTH, OnViewZoomWidth)
	ON_COMMAND(ID_VIEW_ZOOMALL, OnViewZoomAll)
	ON_COMMAND(ID_MENUEXIT, OnMenuexit)
	ON_COMMAND(ID_MENUHELP, OnMenuhelp)
	ON_COMMAND(ID_VIEW_COMPAREHEADREVISIONS, OnViewCompareheadrevisions)
	ON_COMMAND(ID_VIEW_COMPAREREVISIONS, OnViewComparerevisions)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFF, OnViewUnifieddiff)
	ON_COMMAND(ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, OnViewUnifieddiffofheadrevisions)
	ON_COMMAND(ID_FILE_SAVEGRAPHAS, OnFileSavegraphas)
	ON_COMMAND_EX(ID_VIEW_SHOWALLREVISIONS, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_GROUPBRANCHES, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_TOPDOWN, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_TOPALIGNTREES, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_SHOWHEAD, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_EXACTCOPYSOURCE, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_FOLDTAGS, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_REDUCECROSSLINES, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_REMOVEDELETEDONES, OnToggleOption)
	ON_COMMAND_EX(ID_VIEW_SHOWWCREV, OnToggleReloadOption)
	ON_COMMAND_EX(ID_VIEW_REMOVEUNCHANGEDBRANCHES, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_SHOWWCMODIFICATION, OnToggleReloadOption)
    ON_COMMAND_EX(ID_VIEW_SHOWDIFFPATHS, OnToggleOption)
    ON_COMMAND_EX(ID_VIEW_SHOWTREESTRIPES, OnToggleRedrawOption)
	ON_CBN_SELCHANGE(ID_REVGRAPH_ZOOMCOMBO, OnChangeZoom)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_COMMAND(ID_VIEW_FILTER, OnViewFilter)
	ON_COMMAND(ID_VIEW_SHOWOVERVIEW, OnViewShowoverview)
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
	int zoomComboIndex = 0;
	while (m_ToolBar.GetItemID(zoomComboIndex) != ID_REVGRAPH_ZOOMCOMBO) zoomComboIndex++;

	// next convert that button to a separator and get its position
	m_ToolBar.SetButtonInfo(zoomComboIndex, ID_REVGRAPH_ZOOMCOMBO, TBBS_SEPARATOR,
		SNAP_WIDTH);
	RECT rect;
	m_ToolBar.GetItemRect(zoomComboIndex, &rect);

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

    UpdateOptionAvailability();

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

    CSyncPointer<CAllRevisionGraphOptions> 
        options (m_Graph.m_state.GetOptions());

    for (size_t i = 0; i < options->count(); ++i)
        if ((*options)[i]->CommandID() != 0)
        	SetOption ((*options)[i]->CommandID());

	CMenu * pMenu = GetMenu();
	if (pMenu)
	{
		CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowRevGraphOverview"), FALSE);
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

    assert (m_Graph.m_bThreadRunning == TRUE);
	if (AfxBeginThread(WorkerThread, this)== NULL)
	{
        InterlockedExchange (&m_Graph.m_bThreadRunning, FALSE);
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
	assert (pDlg->m_Graph.m_bThreadRunning == TRUE);
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

	    if (!pDlg->m_Graph.FetchRevisionData (pDlg->m_Graph.m_sPath, pegRev))
		    CMessageBox::Show ( pDlg->m_hWnd
                              , pDlg->m_Graph.m_state.GetLastErrorMessage()
                              , _T("TortoiseSVN")
                              , MB_ICONERROR);

        pDlg->m_Graph.m_pProgress->Stop();
        delete pDlg->m_Graph.m_pProgress;
        pDlg->m_Graph.m_pProgress = NULL;

    	pDlg->m_bFetchLogs = false;	// we've got the logs, no need to fetch them a second time
    }

    // standard plus user settings

    if (pDlg->m_Graph.AnalyzeRevisionData())
    {
        pDlg->UpdateStatusBar();
        pDlg->UpdateOptionAvailability();
    }

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
            UpdateFullHistory();
			break;
		}
	}
	if ((m_hAccel)&&(pMsg->message >= WM_KEYFIRST && pMsg->message <= WM_KEYLAST))
	{
		if (pMsg->wParam == VK_ESCAPE)
			if (m_Graph.CancelMouseZoom())
				return TRUE;
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

void CRevisionGraphDlg::OnViewZoomHeight()
{
	CRect graphRect = m_Graph.GetGraphRect();
    CRect windowRect = m_Graph.GetWindowRect();

	float horzfact = (windowRect.Width() - 4.0f)/(4.0f + graphRect.Width());
	float vertfact = (windowRect.Height() - 4.0f)/(4.0f + graphRect.Height());
    if ((horzfact < vertfact) && (horzfact < 2.0f))
    	vertfact = (windowRect.Height() - 20.0f)/(4.0f + graphRect.Height());

    DoZoom (min (2.0f, vertfact));
}

void CRevisionGraphDlg::OnViewZoomWidth()
{
	// zoom the graph so that it is completely visible in the window
	CRect graphRect = m_Graph.GetGraphRect();
    CRect windowRect = m_Graph.GetWindowRect();

	float horzfact = (windowRect.Width() - 4.0f)/(4.0f + graphRect.Width());
	float vertfact = (windowRect.Height() - 4.0f)/(4.0f + graphRect.Height());
    if ((vertfact < horzfact) && (vertfact < 2.0f))
    	horzfact = (windowRect.Width() - 20.0f)/(4.0f + graphRect.Width());

    DoZoom (min (2.0f, horzfact));
}

void CRevisionGraphDlg::OnViewZoomAll()
{
	// zoom the graph so that it is completely visible in the window
	CRect graphRect = m_Graph.GetGraphRect();
    CRect windowRect = m_Graph.GetWindowRect();

	float horzfact = (windowRect.Width() - 4.0f)/(4.0f + graphRect.Width());
	float vertfact = (windowRect.Height() - 4.0f)/(4.0f + graphRect.Height());

    DoZoom (min (2.0f, min(horzfact, vertfact)));
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

void CRevisionGraphDlg::UpdateFullHistory()
{
    m_Graph.SetDlgTitle (false);

    SVN svn;
	LogCache::CRepositoryInfo& cachedProperties 
        = svn.GetLogCachePool()->GetRepositoryInfo();
    CString root = m_Graph.m_state.GetRepositoryRoot();
    CString uuid = m_Graph.m_state.GetRepositoryUUID();

    cachedProperties.ResetHeadRevision (uuid, root);

    m_bFetchLogs = true;
    StartWorkerThread();
}

void CRevisionGraphDlg::SetOption (UINT controlID)
{
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;

	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(controlID);
    if (tbstate != -1)
    {
        if (m_Graph.m_state.GetOptions()->IsSelected (controlID))
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

BOOL CRevisionGraphDlg::ToggleOption (UINT controlID)
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

    CSyncPointer<CAllRevisionGraphOptions> 
        options (m_Graph.m_state.GetOptions());
    if (((state & MF_CHECKED) != 0) == options->IsSelected (controlID))
        options->ToggleSelection (controlID);

    return TRUE;
}

BOOL CRevisionGraphDlg::OnToggleOption (UINT controlID)
{
    if (!ToggleOption (controlID))
        return FALSE;

    // re-process the data

    StartWorkerThread();

    return TRUE;
}

BOOL CRevisionGraphDlg::OnToggleReloadOption (UINT controlID)
{
    if (!m_Graph.m_state.GetFetchedWCState())
        m_bFetchLogs = true;

    return OnToggleOption (controlID);
}

BOOL CRevisionGraphDlg::OnToggleRedrawOption (UINT controlID)
{
    if (!ToggleOption (controlID))
        return FALSE;

    m_Graph.BuildPreview();
    Invalidate();

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
	sFormat.Format(IDS_REVGRAPH_STATUSBARNUMNODES, m_Graph.m_state.GetNodeCount());
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
    CSyncPointer<CAllRevisionGraphOptions> 
        options (m_Graph.m_state.GetOptions());

    CRevisionInRange* revisionRange = options->GetOption<CRevisionInRange>();
    svn_revnum_t head = m_Graph.m_state.GetHeadRevision();
    svn_revnum_t lowerLimit = revisionRange->GetLowerLimit();
    svn_revnum_t upperLimit = revisionRange->GetUpperLimit();

    CRemovePathsBySubString* pathFilter = options->GetOption<CRemovePathsBySubString>();

	CRevGraphFilterDlg dlg;
	dlg.SetMaxRevision (head);
	dlg.SetFilterString (m_sFilter);
    dlg.SetRemoveSubTrees (pathFilter->GetRemoveSubTrees());
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
        revisionRange->SetUpperLimit (maxrev == head ? revision_t (NO_REVISION) : maxrev);

        pathFilter->SetRemoveSubTrees (dlg.GetRemoveSubTrees());
        std::set<std::string>& filterPaths = pathFilter->GetFilterPaths();
        int index = 0;
        filterPaths.clear();

        CString path = m_sFilter.Tokenize (_T("*"),  index);
        while (!path.IsEmpty())
        {
            filterPaths.insert (CUnicodeUtils::StdGetUTF8 ((LPCTSTR)path));
            path = m_sFilter.Tokenize (_T("*"),  index);
        }

        // update menu & toolbar

	    CMenu * pMenu = GetMenu();
    	int tbstate = m_ToolBar.GetToolBarCtrl().GetState(ID_VIEW_FILTER);
        if (revisionRange->IsActive() || pathFilter->IsActive())
	    {
    	    if (pMenu != NULL)
    		    pMenu->CheckMenuItem(ID_VIEW_FILTER, MF_BYCOMMAND | MF_CHECKED);
		    m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_FILTER, tbstate | TBSTATE_CHECKED);
	    }
	    else
	    {
    	    if (pMenu != NULL)
		        pMenu->CheckMenuItem(ID_VIEW_FILTER, MF_BYCOMMAND | MF_UNCHECKED);
		    m_ToolBar.GetToolBarCtrl().SetState(ID_VIEW_FILTER, tbstate & (~TBSTATE_CHECKED));
	    }

        // re-run query

        StartWorkerThread();
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

	CRegDWORD reg = CRegDWORD(_T("Software\\TortoiseSVN\\ShowRevGraphOverview"), FALSE);
	reg = m_Graph.GetShowOverview();
	m_Graph.Invalidate(FALSE);
}

void CRevisionGraphDlg::UpdateOptionAvailability (UINT id, bool available)
{
	CMenu * pMenu = GetMenu();
	if (pMenu == NULL)
		return;

    pMenu->EnableMenuItem (id, available ? MF_ENABLED : MF_GRAYED);

    int tbstate = m_ToolBar.GetToolBarCtrl().GetState(id);
    int newTbstate = available
        ? tbstate | TBSTATE_ENABLED
        : tbstate & ~TBSTATE_ENABLED;

    if (tbstate != newTbstate)
        m_ToolBar.GetToolBarCtrl().SetState(id, newTbstate);
}

void CRevisionGraphDlg::UpdateOptionAvailability()
{
    bool multipleTrees = m_Graph.m_state.GetTreeCount() > 1;
    bool isWCPath = !CTSVNPath (m_Graph.m_sPath).IsUrl();

    UpdateOptionAvailability (ID_VIEW_TOPALIGNTREES, multipleTrees);
    UpdateOptionAvailability (ID_VIEW_SHOWTREESTRIPES, multipleTrees);
    UpdateOptionAvailability (ID_VIEW_SHOWWCREV, isWCPath);
    UpdateOptionAvailability (ID_VIEW_SHOWWCMODIFICATION, isWCPath);
}

