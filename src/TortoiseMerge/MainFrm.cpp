// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2004-2018 - TortoiseSVN
// Copyright (C) 2012-2014 - Sven Strickroth <email@cs-ware.de>

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
#include "TortoiseMerge.h"
#include "OpenDlg.h"
#include "SysProgressDlg.h"
#include "Settings.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "MainFrm.h"
#include "LeftView.h"
#include "RightView.h"
#include "BottomView.h"
#include "DiffColors.h"
#include "SelectFileFilter.h"
#include "FormatMessageWrapper.h"
#include "TaskbarUUID.h"
#include "RegexFiltersDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame
#define IDT_RELOADCHECKTIMER 123

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	// Global help commands
	ON_COMMAND(ID_HELP_FINDER, CFrameWndEx::OnHelpFinder)
	ON_COMMAND(ID_HELP, CFrameWndEx::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CFrameWndEx::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CFrameWndEx::OnHelpFinder)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_VIEW_WHITESPACES, OnViewWhitespaces)
	ON_WM_SIZE()
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
	ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
	ON_COMMAND(ID_VIEW_ONEWAYDIFF, OnViewOnewaydiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ONEWAYDIFF, OnUpdateViewOnewaydiff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WHITESPACES, OnUpdateViewWhitespaces)
	ON_COMMAND(ID_VIEW_OPTIONS, OnViewOptions)
	ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
	ON_WM_CLOSE()
	ON_WM_ACTIVATE()
	ON_COMMAND(ID_FILE_RELOAD, OnFileReload)
	ON_COMMAND(ID_VIEW_LINEDOWN, OnViewLinedown)
	ON_COMMAND(ID_VIEW_LINEUP, OnViewLineup)
	ON_COMMAND(ID_VIEW_MOVEDBLOCKS, OnViewMovedBlocks)
	ON_UPDATE_COMMAND_UI(ID_VIEW_MOVEDBLOCKS, OnUpdateViewMovedBlocks)
	ON_UPDATE_COMMAND_UI(ID_EDIT_MARKASRESOLVED, OnUpdateMergeMarkasresolved)
	ON_COMMAND(ID_EDIT_MARKASRESOLVED, OnMergeMarkasresolved)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTCONFLICT, OnUpdateMergeNextconflict)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVIOUSCONFLICT, OnUpdateMergePreviousconflict)
	ON_WM_MOVE()
	ON_WM_MOVING()
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateEditCopy)
	ON_COMMAND(ID_VIEW_SWITCHLEFT, OnViewSwitchleft)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SWITCHLEFT, OnUpdateViewSwitchleft)
	ON_COMMAND(ID_VIEW_LINELEFT, &CMainFrame::OnViewLineleft)
	ON_COMMAND(ID_VIEW_LINERIGHT, &CMainFrame::OnViewLineright)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWFILELIST, &CMainFrame::OnUpdateViewShowfilelist)
	ON_COMMAND(ID_VIEW_SHOWFILELIST, &CMainFrame::OnViewShowfilelist)
	ON_COMMAND(ID_EDIT_USETHEIRBLOCK, &CMainFrame::OnEditUseTheirs)
	ON_COMMAND(ID_EDIT_USEMYBLOCK, &CMainFrame::OnEditUseMine)
	ON_COMMAND(ID_EDIT_USETHEIRTHENMYBLOCK, &CMainFrame::OnEditUseTheirsThenMine)
	ON_COMMAND(ID_EDIT_USEMINETHENTHEIRBLOCK, &CMainFrame::OnEditUseMineThenTheirs)
	ON_COMMAND(ID_EDIT_UNDO, &CMainFrame::OnEditUndo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_UNDO, &CMainFrame::OnUpdateEditUndo)
	ON_COMMAND(ID_EDIT_REDO, &CMainFrame::OnEditRedo)
	ON_UPDATE_COMMAND_UI(ID_EDIT_REDO, &CMainFrame::OnUpdateEditRedo)
	ON_COMMAND(ID_EDIT_ENABLE, &CMainFrame::OnEditEnable)
	ON_UPDATE_COMMAND_UI(ID_EDIT_ENABLE, &CMainFrame::OnUpdateEditEnable)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USEMINETHENTHEIRBLOCK, &CMainFrame::OnUpdateEditUseminethentheirblock)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USEMYBLOCK, &CMainFrame::OnUpdateEditUsemyblock)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USETHEIRBLOCK, &CMainFrame::OnUpdateEditUsetheirblock)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USETHEIRTHENMYBLOCK, &CMainFrame::OnUpdateEditUsetheirthenmyblock)
	ON_COMMAND(ID_VIEW_INLINEDIFFWORD, &CMainFrame::OnViewInlinediffword)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INLINEDIFFWORD, &CMainFrame::OnUpdateViewInlinediffword)
	ON_COMMAND(ID_VIEW_INLINEDIFF, &CMainFrame::OnViewInlinediff)
	ON_UPDATE_COMMAND_UI(ID_VIEW_INLINEDIFF, &CMainFrame::OnUpdateViewInlinediff)
	ON_UPDATE_COMMAND_UI(ID_EDIT_CREATEUNIFIEDDIFFFILE, &CMainFrame::OnUpdateEditCreateunifieddifffile)
	ON_COMMAND(ID_EDIT_CREATEUNIFIEDDIFFFILE, &CMainFrame::OnEditCreateunifieddifffile)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LINEDIFFBAR, &CMainFrame::OnUpdateViewLinediffbar)
	ON_COMMAND(ID_VIEW_LINEDIFFBAR, &CMainFrame::OnViewLinediffbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BARS, &CMainFrame::OnUpdateViewBars)
	ON_UPDATE_COMMAND_UI(ID_VIEW_LOCATORBAR, &CMainFrame::OnUpdateViewLocatorbar)
	ON_COMMAND(ID_VIEW_LOCATORBAR, &CMainFrame::OnViewLocatorbar)
	ON_COMMAND(ID_EDIT_USELEFTBLOCK, &CMainFrame::OnEditUseleftblock)
	ON_UPDATE_COMMAND_UI(ID_USEBLOCKS, &CMainFrame::OnUpdateUseBlock)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USELEFTBLOCK, &CMainFrame::OnUpdateEditUseleftblock)
	ON_COMMAND(ID_EDIT_USELEFTFILE, &CMainFrame::OnEditUseleftfile)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USELEFTFILE, &CMainFrame::OnUpdateEditUseleftfile)
	ON_COMMAND(ID_EDIT_USEBLOCKFROMLEFTBEFORERIGHT, &CMainFrame::OnEditUseblockfromleftbeforeright)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USEBLOCKFROMLEFTBEFORERIGHT, &CMainFrame::OnUpdateEditUseblockfromleftbeforeright)
	ON_COMMAND(ID_EDIT_USEBLOCKFROMRIGHTBEFORELEFT, &CMainFrame::OnEditUseblockfromrightbeforeleft)
	ON_UPDATE_COMMAND_UI(ID_EDIT_USEBLOCKFROMRIGHTBEFORELEFT, &CMainFrame::OnUpdateEditUseblockfromrightbeforeleft)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTDIFFERENCE, &CMainFrame::OnUpdateNavigateNextdifference)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVIOUSDIFFERENCE, &CMainFrame::OnUpdateNavigatePreviousdifference)
	ON_COMMAND(ID_VIEW_COLLAPSED, &CMainFrame::OnViewCollapsed)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COLLAPSED, &CMainFrame::OnUpdateViewCollapsed)
	ON_COMMAND(ID_VIEW_COMPAREWHITESPACES, &CMainFrame::OnViewComparewhitespaces)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COMPAREWHITESPACES, &CMainFrame::OnUpdateViewComparewhitespaces)
	ON_COMMAND(ID_VIEW_IGNOREWHITESPACECHANGES, &CMainFrame::OnViewIgnorewhitespacechanges)
	ON_UPDATE_COMMAND_UI(ID_VIEW_IGNOREWHITESPACECHANGES, &CMainFrame::OnUpdateViewIgnorewhitespacechanges)
	ON_COMMAND(ID_VIEW_IGNOREALLWHITESPACECHANGES, &CMainFrame::OnViewIgnoreallwhitespacechanges)
	ON_UPDATE_COMMAND_UI(ID_VIEW_IGNOREALLWHITESPACECHANGES, &CMainFrame::OnUpdateViewIgnoreallwhitespacechanges)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_NEXTINLINEDIFF, &CMainFrame::OnUpdateNavigateNextinlinediff)
	ON_UPDATE_COMMAND_UI(ID_NAVIGATE_PREVINLINEDIFF, &CMainFrame::OnUpdateNavigatePrevinlinediff)
	ON_COMMAND(ID_VIEW_WRAPLONGLINES, &CMainFrame::OnViewWraplonglines)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WRAPLONGLINES, &CMainFrame::OnUpdateViewWraplonglines)
	ON_REGISTERED_MESSAGE( TaskBarButtonCreated, CMainFrame::OnTaskbarButtonCreated )
	ON_UPDATE_COMMAND_UI(ID_EDIT_PASTE, &CMainFrame::OnUpdateEditPaste)
	ON_COMMAND(ID_INDICATOR_LEFTVIEW, &CMainFrame::OnIndicatorLeftview)
	ON_COMMAND(ID_INDICATOR_RIGHTVIEW, &CMainFrame::OnIndicatorRightview)
	ON_COMMAND(ID_INDICATOR_BOTTOMVIEW, &CMainFrame::OnIndicatorBottomview)
	ON_WM_TIMER()
	ON_COMMAND(ID_VIEW_IGNORECOMMENTS, &CMainFrame::OnViewIgnorecomments)
	ON_UPDATE_COMMAND_UI(ID_VIEW_IGNORECOMMENTS, &CMainFrame::OnUpdateViewIgnorecomments)
	ON_COMMAND_RANGE(ID_REGEXFILTER, ID_REGEXFILTER+400, &CMainFrame::OnRegexfilter)
	ON_UPDATE_COMMAND_UI_RANGE(ID_REGEXFILTER, ID_REGEXFILTER+400, &CMainFrame::OnUpdateViewRegexFilter)
	ON_COMMAND(ID_REGEX_NO_FILTER, &CMainFrame::OnRegexNoFilter)
	ON_UPDATE_COMMAND_UI(ID_REGEX_NO_FILTER, &CMainFrame::OnUpdateRegexNoFilter)
	ON_COMMAND(ID_INDICATOR_LEFTVIEWCOMBOENCODING, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_RIGHTVIEWCOMBOENCODING, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_BOTTOMVIEWCOMBOENCODING, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_LEFTVIEWCOMBOEOL, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_RIGHTVIEWCOMBOEOL, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_BOTTOMVIEWCOMBOEOL, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_LEFTVIEWCOMBOTABMODE, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_RIGHTVIEWCOMBOTABMODE, &CMainFrame::OnDummyEnabled)
	ON_COMMAND(ID_INDICATOR_BOTTOMVIEWCOMBOTABMODE, &CMainFrame::OnDummyEnabled)
	ON_UPDATE_COMMAND_UI(ID_EDIT_THREEWAY_ACTIONS, &CMainFrame::OnUpdateThreeWayActions)
	ON_COMMAND_RANGE(ID_INDICATOR_LEFTENCODINGSTART, ID_INDICATOR_LEFTENCODINGSTART+19, &CMainFrame::OnEncodingLeft)
	ON_COMMAND_RANGE(ID_INDICATOR_RIGHTENCODINGSTART, ID_INDICATOR_RIGHTENCODINGSTART+19, &CMainFrame::OnEncodingRight)
	ON_COMMAND_RANGE(ID_INDICATOR_BOTTOMENCODINGSTART, ID_INDICATOR_BOTTOMENCODINGSTART+19, &CMainFrame::OnEncodingBottom)
	ON_COMMAND_RANGE(ID_INDICATOR_LEFTEOLSTART, ID_INDICATOR_LEFTEOLSTART+19, &CMainFrame::OnEOLLeft)
	ON_COMMAND_RANGE(ID_INDICATOR_RIGHTEOLSTART, ID_INDICATOR_RIGHTEOLSTART+19, &CMainFrame::OnEOLRight)
	ON_COMMAND_RANGE(ID_INDICATOR_BOTTOMEOLSTART, ID_INDICATOR_BOTTOMEOLSTART+19, &CMainFrame::OnEOLBottom)
	ON_COMMAND_RANGE(ID_INDICATOR_LEFTTABMODESTART, ID_INDICATOR_LEFTTABMODESTART+19, &CMainFrame::OnTabModeLeft)
	ON_COMMAND_RANGE(ID_INDICATOR_RIGHTTABMODESTART, ID_INDICATOR_RIGHTTABMODESTART+19, &CMainFrame::OnTabModeRight)
	ON_COMMAND_RANGE(ID_INDICATOR_BOTTOMTABMODESTART, ID_INDICATOR_BOTTOMTABMODESTART+19, &CMainFrame::OnTabModeBottom)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_LEFTENCODINGSTART, ID_INDICATOR_LEFTENCODINGSTART+19, &CMainFrame::OnUpdateEncodingLeft)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_RIGHTENCODINGSTART, ID_INDICATOR_RIGHTENCODINGSTART+19, &CMainFrame::OnUpdateEncodingRight)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_BOTTOMENCODINGSTART, ID_INDICATOR_BOTTOMENCODINGSTART+19, &CMainFrame::OnUpdateEncodingBottom)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_LEFTEOLSTART, ID_INDICATOR_LEFTEOLSTART+19, &CMainFrame::OnUpdateEOLLeft)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_RIGHTEOLSTART, ID_INDICATOR_RIGHTEOLSTART+19, &CMainFrame::OnUpdateEOLRight)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_BOTTOMEOLSTART, ID_INDICATOR_BOTTOMEOLSTART+19, &CMainFrame::OnUpdateEOLBottom)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_LEFTTABMODESTART, ID_INDICATOR_LEFTTABMODESTART+19, &CMainFrame::OnUpdateTabModeLeft)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_RIGHTTABMODESTART, ID_INDICATOR_RIGHTTABMODESTART+19, &CMainFrame::OnUpdateTabModeRight)
	ON_UPDATE_COMMAND_UI_RANGE(ID_INDICATOR_BOTTOMTABMODESTART, ID_INDICATOR_BOTTOMTABMODESTART+19, &CMainFrame::OnUpdateTabModeBottom)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_LEFTVIEW,
	ID_INDICATOR_RIGHTVIEW,
	ID_INDICATOR_BOTTOMVIEW,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL
};


// CMainFrame construction/destruction

CMainFrame::CMainFrame()
	: m_bInitSplitter(FALSE)
	, m_bReversedPatch(FALSE)
	, m_bHasConflicts(false)
	, m_bInlineWordDiff(true)
	, m_bLineDiff(true)
	, m_bLocatorBar(true)
	, m_nMoveMovesToIgnore(0)
	, m_pwndLeftView(nullptr)
	, m_pwndRightView(nullptr)
	, m_pwndBottomView(nullptr)
	, m_bReadOnly(false)
	, m_bBlame(false)
	, m_bCheckReload(false)
	, m_bSaveRequired(false)
	, m_bSaveRequiredOnConflicts(false)
	, m_bDeleteBaseTheirsMineOnClose(false)
	, resolveMsgWnd(0)
	, resolveMsgWParam(0)
	, resolveMsgLParam(0)
	, m_regWrapLines(L"Software\\TortoiseGitMerge\\WrapLines", 0)
	, m_regViewModedBlocks(L"Software\\TortoiseGitMerge\\ViewMovedBlocks", TRUE)
	, m_regOneWay(L"Software\\TortoiseGitMerge\\OnePane")
	, m_regCollapsed(L"Software\\TortoiseGitMerge\\Collapsed", 0)
	, m_regInlineDiff(L"Software\\TortoiseGitMerge\\DisplayBinDiff", TRUE)
	, m_regUseRibbons(L"Software\\TortoiseGitMerge\\UseRibbons", TRUE)
	, m_regIgnoreComments(L"Software\\TortoiseGitMerge\\IgnoreComments", FALSE)
	, m_regexIndex(-1)
{
	m_bOneWay = (0 != (static_cast<DWORD>(m_regOneWay)));
	m_bCollapsed = !!static_cast<DWORD>(m_regCollapsed);
	m_bViewMovedBlocks = !!static_cast<DWORD>(m_regViewModedBlocks);
	m_bWrapLines = !!static_cast<DWORD>(m_regWrapLines);
	m_bInlineDiff = !!m_regInlineDiff;
	m_bUseRibbons = !!m_regUseRibbons;
}

CMainFrame::~CMainFrame()
{
}

LRESULT CMainFrame::OnTaskbarButtonCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	SetUUIDOverlayIcon(m_hWnd);
	return 0;
}


int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_bUseRibbons)
	{
		HRESULT hr;
		hr = m_pRibbonFramework.CoCreateInstance(__uuidof(UIRibbonFramework));
		if (FAILED(hr))
		{
			TRACE(L"Failed to create ribbon framework (%08x)\n", hr);
			return -1; // fail to create
		}

		m_pRibbonApp.reset(new CNativeRibbonApp(this, m_pRibbonFramework));
		m_pRibbonApp->SetSettingsFileName(CPathUtils::GetAppDataDirectory() + L"TortoiseGitMerge-RibbonSettings");

		hr = m_pRibbonFramework->Initialize(m_hWnd, m_pRibbonApp.get());
		if (FAILED(hr))
		{
			TRACE(L"Failed to initialize ribbon framework (%08x)\n", hr);
			return -1; // fail to create
		}

		hr = m_pRibbonFramework->LoadUI(AfxGetResourceHandle(), L"TORTOISEGITMERGERIBBON_RIBBON");
		if (FAILED(hr))
		{
			TRACE(L"Failed to load ribbon UI (%08x)\n", hr);
			return -1; // fail to create
		}
		BuildRegexSubitems();
		if (!m_wndRibbonStatusBar.Create(this))
		{
			TRACE0("Failed to create ribbon status bar\n");
			return -1; // fail to create
		}
		m_wndRibbonStatusBar.AddElement(new CMFCRibbonStatusBarPane(ID_SEPARATOR, CString(MAKEINTRESOURCE(AFX_IDS_IDLEMESSAGE)), TRUE), L"");

		CString sTooltip(MAKEINTRESOURCE(IDS_ENCODING_COMBO_TOOLTIP));
		auto apBtnGroupLeft = std::make_unique<CMFCRibbonButtonsGroup>();
		apBtnGroupLeft->SetID(ID_INDICATOR_LEFTVIEW);
		apBtnGroupLeft->AddButton(new CMFCRibbonStatusBarPane(ID_SEPARATOR,   CString(MAKEINTRESOURCE(IDS_STATUSBAR_LEFTVIEW)), TRUE));
		CMFCRibbonButton * pButton = new CMFCRibbonButton(ID_INDICATOR_LEFTVIEWCOMBOENCODING, L"");
		pButton->SetToolTipText(sTooltip);
		FillEncodingButton(pButton, ID_INDICATOR_LEFTENCODINGSTART);
		apBtnGroupLeft->AddButton(pButton);
		pButton = new CMFCRibbonButton(ID_INDICATOR_LEFTVIEWCOMBOEOL, L"");
		FillEOLButton(pButton, ID_INDICATOR_LEFTEOLSTART);
		apBtnGroupLeft->AddButton(pButton);
		pButton = new CMFCRibbonButton(ID_INDICATOR_LEFTVIEWCOMBOTABMODE, L"");
		FillTabModeButton(pButton, ID_INDICATOR_LEFTTABMODESTART);
		apBtnGroupLeft->AddButton(pButton);
		apBtnGroupLeft->AddButton(new CMFCRibbonStatusBarPane(ID_INDICATOR_LEFTVIEW,   L"", TRUE));
		m_wndRibbonStatusBar.AddExtendedElement(apBtnGroupLeft.release(), L"");

		auto apBtnGroupRight = std::make_unique<CMFCRibbonButtonsGroup>();
		apBtnGroupRight->SetID(ID_INDICATOR_RIGHTVIEW);
		apBtnGroupRight->AddButton(new CMFCRibbonStatusBarPane(ID_SEPARATOR,   CString(MAKEINTRESOURCE(IDS_STATUSBAR_RIGHTVIEW)), TRUE));
		pButton = new CMFCRibbonButton(ID_INDICATOR_RIGHTVIEWCOMBOENCODING, L"");
		pButton->SetToolTipText(sTooltip);
		FillEncodingButton(pButton, ID_INDICATOR_RIGHTENCODINGSTART);
		apBtnGroupRight->AddButton(pButton);
		pButton = new CMFCRibbonButton(ID_INDICATOR_RIGHTVIEWCOMBOEOL, L"");
		FillEOLButton(pButton, ID_INDICATOR_RIGHTEOLSTART);
		apBtnGroupRight->AddButton(pButton);
		pButton = new CMFCRibbonButton(ID_INDICATOR_RIGHTVIEWCOMBOTABMODE, L"");
		FillTabModeButton(pButton, ID_INDICATOR_RIGHTTABMODESTART);
		apBtnGroupRight->AddButton(pButton);
		apBtnGroupRight->AddButton(new CMFCRibbonStatusBarPane(ID_INDICATOR_RIGHTVIEW,  L"", TRUE));
		m_wndRibbonStatusBar.AddExtendedElement(apBtnGroupRight.release(), L"");
	}
	else
	{
		if (!m_wndMenuBar.Create(this))
		{
			TRACE0("Failed to create menubar\n");
			return -1; // fail to create
		}
		m_wndMenuBar.SetPaneStyle(m_wndMenuBar.GetPaneStyle() | CBRS_SIZE_DYNAMIC | CBRS_TOOLTIPS | CBRS_FLYBY);

		// prevent the menu bar from taking the focus on activation
		CMFCPopupMenu::SetForceMenuFocus(FALSE);
		if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_ALIGN_TOP | CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY) || !m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
		{
			TRACE0("Failed to create toolbar\n");
			return -1; // fail to create
		}
		m_wndToolBar.SetWindowText(L"Main");
		if (!m_wndStatusBar.Create(this) ||
			!m_wndStatusBar.SetIndicators(indicators,
			_countof(indicators)))
		{
			TRACE0("Failed to create status bar\n");
			return -1;		// fail to create
		}
		m_wndStatusBar.EnablePaneDoubleClick();
	}

	if (!m_wndLocatorBar.Create(this, IDD_DIFFLOCATOR,
		CBRS_ALIGN_LEFT | CBRS_SIZE_FIXED, ID_VIEW_LOCATORBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	if (!m_wndLineDiffBar.Create(this, IDD_LINEDIFF,
		CBRS_ALIGN_BOTTOM | CBRS_SIZE_FIXED, ID_VIEW_LINEDIFFBAR))
	{
		TRACE0("Failed to create dialogbar\n");
		return -1;		// fail to create
	}
	m_wndLocatorBar.m_pMainFrm = this;
	m_wndLineDiffBar.m_pMainFrm = this;

	EnableDocking(CBRS_ALIGN_ANY);
	if (!m_bUseRibbons)
	{
		m_wndMenuBar.EnableDocking(CBRS_ALIGN_TOP);
		m_wndToolBar.EnableDocking(CBRS_ALIGN_TOP);
		DockPane(&m_wndMenuBar);
		DockPane(&m_wndToolBar);
	}
	DockPane(&m_wndLocatorBar);
	DockPane(&m_wndLineDiffBar);
	ShowPane(&m_wndLocatorBar, true, false, true);
	ShowPane(&m_wndLineDiffBar, true, false, true);

	m_wndLocatorBar.EnableGripper(FALSE);
	m_wndLineDiffBar.EnableGripper(FALSE);

	return 0;
}

void CMainFrame::OnDestroy()
{
	if (m_pRibbonFramework)
		m_pRibbonFramework->Destroy();

	CFrameWndEx::OnDestroy();
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWndEx::PreCreateWindow(cs) )
		return FALSE;
	return TRUE;
}

// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWndEx::Dump(dc);
}

#endif //_DEBUG


// CMainFrame message handlers


BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT /*lpcs*/, CCreateContext* pContext)
{
	CRect cr;
	GetClientRect( &cr);


	// split into three panes:
	//        -------------
	//        |     |     |
	//        |     |     |
	//        |------------
	//        |           |
	//        |           |
	//        |------------

	// create a splitter with 2 rows, 1 column
	if (!m_wndSplitter.CreateStatic(this, 2, 1))
	{
		TRACE0("Failed to CreateStaticSplitter\n");
		return FALSE;
	}

	// add the second splitter pane - which is a nested splitter with 2 columns
	if (!m_wndSplitter2.CreateStatic(
		&m_wndSplitter, // our parent window is the first splitter
		1, 2, // the new splitter is 1 row, 2 columns
		WS_CHILD | WS_VISIBLE | WS_BORDER, // style, WS_BORDER is needed
		m_wndSplitter.IdFromRowCol(0, 0)
		// new splitter is in the first row, 1st column of first splitter
		))
	{
		TRACE0("Failed to create nested splitter\n");
		return FALSE;
	}
	// add the first splitter pane - the default view in row 0
	if (!m_wndSplitter.CreateView(1, 0,
		RUNTIME_CLASS(CBottomView), CSize(cr.Width(), cr.Height()), pContext))
	{
		TRACE0("Failed to create first pane\n");
		return FALSE;
	}
	m_pwndBottomView = static_cast<CBottomView*>(m_wndSplitter.GetPane(1, 0));
	m_pwndBottomView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndBottomView->m_pwndLineDiffBar = &m_wndLineDiffBar;
	if (m_bUseRibbons)
		m_pwndBottomView->m_pwndRibbonStatusBar = &m_wndRibbonStatusBar;
	else
		m_pwndBottomView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndBottomView->m_pMainFrame = this;

	// now create the two views inside the nested splitter

	if (!m_wndSplitter2.CreateView(0, 0,
		RUNTIME_CLASS(CLeftView), CSize(cr.Width()/2, cr.Height()/2), pContext))
	{
		TRACE0("Failed to create second pane\n");
		return FALSE;
	}
	m_pwndLeftView = static_cast<CLeftView*>(m_wndSplitter2.GetPane(0, 0));
	m_pwndLeftView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndLeftView->m_pwndLineDiffBar = &m_wndLineDiffBar;
	if (m_bUseRibbons)
		m_pwndLeftView->m_pwndRibbonStatusBar = &m_wndRibbonStatusBar;
	else
		m_pwndLeftView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndLeftView->m_pMainFrame = this;

	if (!m_wndSplitter2.CreateView(0, 1,
		RUNTIME_CLASS(CRightView), CSize(cr.Width()/2, cr.Height()/2), pContext))
	{
		TRACE0("Failed to create third pane\n");
		return FALSE;
	}
	m_pwndRightView = static_cast<CRightView*>(m_wndSplitter2.GetPane(0, 1));
	m_pwndRightView->m_pwndLocator = &m_wndLocatorBar;
	m_pwndRightView->m_pwndLineDiffBar = &m_wndLineDiffBar;
	if (m_bUseRibbons)
		m_pwndRightView->m_pwndRibbonStatusBar = &m_wndRibbonStatusBar;
	else
		m_pwndRightView->m_pwndStatusBar = &m_wndStatusBar;
	m_pwndRightView->m_pMainFrame = this;
	m_bInitSplitter = TRUE;

	m_dlgFilePatches.Create(IDD_FILEPATCHES, this);
	UpdateLayout();
	return TRUE;
}

// Callback function
BOOL CMainFrame::PatchFile(CString sFilePath, bool /*bContentMods*/, bool bPropMods, CString sVersion, BOOL bAutoPatch)
{
	//"dry run" was successful, so save the patched file somewhere...
	CString sTempFile = CTempFiles::Instance().GetTempFilePathString();
	CString sRejectedFile, sBasePath;
	if (m_Patch.GetPatchResult(sFilePath, sTempFile, sRejectedFile, sBasePath) < 0)
	{
		MessageBox(m_Patch.GetErrorMessage(), nullptr, MB_ICONERROR);
		return FALSE;
	}
	sFilePath = m_Patch.GetTargetPath() + L'\\' + sFilePath;
	sFilePath.Replace('/', '\\');
	if (sBasePath.IsEmpty())
		sBasePath = sFilePath;
	if (m_bReversedPatch)
	{
		m_Data.m_baseFile.SetFileName(sTempFile);
		CString temp;
		temp.Format(L"%s %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCTSTR>(m_Data.m_sPatchPatched));
		m_Data.m_baseFile.SetDescriptiveName(temp);
		m_Data.m_yourFile.SetFileName(sFilePath);
		temp.Format(L"%s %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCTSTR>(m_Data.m_sPatchOriginal));
		m_Data.m_yourFile.SetDescriptiveName(temp);
		m_Data.m_theirFile.SetOutOfUse();
		m_Data.m_mergedFile.SetOutOfUse();
	}
	else
	{
		if ((!PathFileExists(sBasePath))||(PathIsDirectory(sBasePath)))
		{
			m_Data.m_baseFile.SetFileName(CTempFiles::Instance().GetTempFilePathString());
			m_Data.m_baseFile.CreateEmptyFile();
		}
		else
		{
			m_Data.m_baseFile.SetFileName(sBasePath);
		}
		CString sDescription;
		sDescription.Format(L"%s %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sBasePath)), static_cast<LPCTSTR>(m_Data.m_sPatchOriginal));
		m_Data.m_baseFile.SetDescriptiveName(sDescription);
		if (sBasePath == sFilePath)
		{
			m_Data.m_yourFile.SetFileName(sTempFile);
			CString temp;
			temp.Format(L"%s %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sBasePath)), static_cast<LPCTSTR>(m_Data.m_sPatchPatched));
			m_Data.m_yourFile.SetDescriptiveName(temp);
			m_Data.m_theirFile.SetOutOfUse();
		}
		else
		{
			if (!PathFileExists(sFilePath) || PathIsDirectory(sFilePath))
			{
				m_Data.m_yourFile.SetFileName(CTempFiles::Instance().GetTempFilePathString());
				m_Data.m_yourFile.CreateEmptyFile();
				CString temp;
				temp.Format(L"%s %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_NOTFOUNDVIEWTITLEINDICATOR))));
				m_Data.m_yourFile.SetDescriptiveName(temp);
			}
			else
				m_Data.m_yourFile.SetFileName(sFilePath);
			m_Data.m_theirFile.SetFileName(sTempFile);
			CString temp;
			temp.Format(L"%s %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sFilePath)), static_cast<LPCTSTR>(m_Data.m_sPatchPatched));
			m_Data.m_theirFile.SetDescriptiveName(temp);
		}
		m_Data.m_mergedFile.SetFileName(sFilePath);
		m_Data.m_bPatchRequired = bPropMods;
	}
	TRACE(L"comparing %s\nwith the patched result %s\n", static_cast<LPCTSTR>(sFilePath), static_cast<LPCTSTR>(sTempFile));

	LoadViews();
	if (!sRejectedFile.IsEmpty())
	{
#if 0 // TGIT TODO
		// start TortoiseUDiff with the rejected hunks
		CString sTitle;
		sTitle.Format(IDS_TITLE_REJECTEDHUNKS, static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sFilePath)));
		CAppUtils::StartUnifiedDiffViewer(sRejectedFile, sTitle);
#endif
	}
	if (bAutoPatch)
	{
		if (sBasePath != sFilePath && HasConflictsWontKeep())
			return FALSE;

		PatchSave();
	}
	return TRUE;
}

// Callback function
BOOL CMainFrame::DiffFiles(CString sURL1, CString sRev1, CString sURL2, CString sRev2)
{
	CString tempfile1 = CTempFiles::Instance().GetTempFilePathString();
	CString tempfile2 = CTempFiles::Instance().GetTempFilePathString();

	ASSERT(tempfile1.Compare(tempfile2));

	CString sTemp;
	CSysProgressDlg progDlg;
	sTemp.Format(IDS_GETVERSIONOFFILE, static_cast<LPCTSTR>(sRev1));
	progDlg.SetLine(1, sTemp, true);
	progDlg.SetLine(2, sURL1, true);
	sTemp.LoadString(IDS_GETVERSIONOFFILETITLE);
	progDlg.SetTitle(sTemp);
	progDlg.SetShowProgressBar(true);
	progDlg.SetTime(FALSE);
	progDlg.SetProgress(1,100);
	progDlg.ShowModeless(this);
	if (!CAppUtils::GetVersionedFile(sURL1, sRev1, tempfile1, &progDlg, m_hWnd))
	{
		progDlg.Stop();
		CString sErrMsg;
		sErrMsg.FormatMessage(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, static_cast<LPCTSTR>(sRev1), static_cast<LPCTSTR>(sURL1));
		MessageBox(sErrMsg, nullptr, MB_ICONERROR);
		return FALSE;
	}
	sTemp.Format(IDS_GETVERSIONOFFILE, static_cast<LPCTSTR>(sRev2));
	progDlg.SetLine(1, sTemp, true);
	progDlg.SetLine(2, sURL2, true);
	progDlg.SetProgress(50, 100);
	if (!CAppUtils::GetVersionedFile(sURL2, sRev2, tempfile2, &progDlg, m_hWnd))
	{
		progDlg.Stop();
		CString sErrMsg;
		sErrMsg.FormatMessage(IDS_ERR_MAINFRAME_FILEVERSIONNOTFOUND, static_cast<LPCTSTR>(sRev2), static_cast<LPCTSTR>(sURL2));
		MessageBox(sErrMsg, nullptr, MB_ICONERROR);
		return FALSE;
	}
	progDlg.SetProgress(100,100);
	progDlg.Stop();
	CString temp;
	temp.Format(L"%s Revision %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sURL1)), static_cast<LPCTSTR>(sRev1));
	m_Data.m_baseFile.SetFileName(tempfile1);
	m_Data.m_baseFile.SetDescriptiveName(temp);
	temp.Format(L"%s Revision %s", static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(sURL2)), static_cast<LPCTSTR>(sRev2));
	m_Data.m_yourFile.SetFileName(tempfile2);
	m_Data.m_yourFile.SetDescriptiveName(temp);

	LoadViews();

	return TRUE;
}

void CMainFrame::OnFileOpen()
{
	if (CheckForSave(CHFSR_OPEN)==IDCANCEL)
		return;
	return OnFileOpen(false);
}

void CMainFrame::OnFileOpen(bool fillyours)
{
	COpenDlg dlg;
	if (fillyours)
		dlg.m_sBaseFile = m_Data.m_yourFile.GetFilename();
	if (dlg.DoModal() != IDOK)
	{
		return;
	}
	m_dlgFilePatches.ShowWindow(SW_HIDE);
	m_dlgFilePatches.Init(nullptr, nullptr, CString(), nullptr);
	TRACE(L"got the files:\n   %s\n   %s\n   %s\n   %s\n   %s\n", static_cast<LPCTSTR>(dlg.m_sBaseFile), static_cast<LPCTSTR>(dlg.m_sTheirFile), static_cast<LPCTSTR>(dlg.m_sYourFile),
		static_cast<LPCTSTR>(dlg.m_sUnifiedDiffFile), static_cast<LPCTSTR>(dlg.m_sPatchDirectory));
	m_Data.m_baseFile.SetFileName(dlg.m_sBaseFile);
	m_Data.m_theirFile.SetFileName(dlg.m_sTheirFile);
	m_Data.m_yourFile.SetFileName(dlg.m_sYourFile);
	m_Data.m_sDiffFile = dlg.m_sUnifiedDiffFile;
	m_Data.m_sPatchPath = dlg.m_sPatchDirectory;
	m_Data.m_mergedFile.SetOutOfUse();
	CCrashReport::Instance().AddFile2(dlg.m_sBaseFile, nullptr, L"Basefile", CR_AF_MAKE_FILE_COPY);
	CCrashReport::Instance().AddFile2(dlg.m_sTheirFile, nullptr, L"Theirfile", CR_AF_MAKE_FILE_COPY);
	CCrashReport::Instance().AddFile2(dlg.m_sYourFile, nullptr, L"Yourfile", CR_AF_MAKE_FILE_COPY);
	CCrashReport::Instance().AddFile2(dlg.m_sUnifiedDiffFile, nullptr, L"Difffile", CR_AF_MAKE_FILE_COPY);

	if (!m_Data.IsBaseFileInUse() && m_Data.IsTheirFileInUse() && m_Data.IsYourFileInUse())
	{
		// a diff between two files means "Yours" against "Base", not "Theirs" against "Yours"
		m_Data.m_baseFile.TransferDetailsFrom(m_Data.m_theirFile);
	}
	if (m_Data.IsBaseFileInUse() && m_Data.IsTheirFileInUse() && !m_Data.IsYourFileInUse())
	{
		// a diff between two files means "Yours" against "Base", not "Theirs" against "Base"
		m_Data.m_yourFile.TransferDetailsFrom(m_Data.m_theirFile);
	}
	m_bSaveRequired = false;

	LoadViews();
}

void CMainFrame::ClearViewNamesAndPaths()
{
	m_pwndLeftView->m_sWindowName.Empty();
	m_pwndLeftView->m_sFullFilePath.Empty();
	m_pwndLeftView->m_sReflectedName.Empty();
	m_pwndRightView->m_sWindowName.Empty();
	m_pwndRightView->m_sFullFilePath.Empty();
	m_pwndRightView->m_sReflectedName.Empty();
	m_pwndBottomView->m_sWindowName.Empty();
	m_pwndBottomView->m_sFullFilePath.Empty();
	m_pwndBottomView->m_sReflectedName.Empty();
}

bool CMainFrame::LoadViews(int line)
{
	LoadIgnoreCommentData();
	m_Data.SetBlame(m_bBlame);
	m_Data.SetMovedBlocks(m_bViewMovedBlocks);
	m_bHasConflicts = false;
	CBaseView* pwndActiveView = m_pwndLeftView;
	int nOldLine = m_pwndRightView ? m_pwndRightView->m_nTopLine : -1;
	int nOldLineNumber =
		m_pwndRightView && m_pwndRightView->m_pViewData ?
		m_pwndRightView->m_pViewData->GetLineNumber(m_pwndRightView->m_nTopLine) : -1;
	POINT ptOldCaretPos = {-1, -1};
	if (m_pwndRightView && m_pwndRightView->IsTarget())
		ptOldCaretPos = m_pwndRightView->GetCaretPosition();
	if (m_pwndBottomView && m_pwndBottomView->IsTarget())
		ptOldCaretPos = m_pwndBottomView->GetCaretPosition();
	CString sExt = CPathUtils::GetFileExtFromPath(m_Data.m_baseFile.GetFilename()).MakeLower();
	sExt.TrimLeft(L".");
	auto sC = m_IgnoreCommentsMap.find(sExt);
	if (sC == m_IgnoreCommentsMap.end())
	{
		sExt = CPathUtils::GetFileExtFromPath(m_Data.m_yourFile.GetFilename()).MakeLower();
		sC = m_IgnoreCommentsMap.find(sExt);
		if (sC == m_IgnoreCommentsMap.end())
		{
			sExt = CPathUtils::GetFileExtFromPath(m_Data.m_theirFile.GetFilename()).MakeLower();
			sC = m_IgnoreCommentsMap.find(sExt);
		}
	}
	if (sC != m_IgnoreCommentsMap.end())
		m_Data.SetCommentTokens(std::get<0>(sC->second), std::get<1>(sC->second), std::get<2>(sC->second));
	else
		m_Data.SetCommentTokens(L"", L"", L"");
	if (!m_Data.Load())
	{
		m_pwndLeftView->BuildAllScreen2ViewVector();
		m_pwndLeftView->DocumentUpdated();
		m_pwndRightView->DocumentUpdated();
		m_pwndBottomView->DocumentUpdated();
		m_wndLocatorBar.DocumentUpdated();
		m_wndLineDiffBar.DocumentUpdated();
		::MessageBox(m_hWnd, m_Data.GetError(), L"TortoiseGitMerge", MB_ICONERROR);
		m_Data.m_mergedFile.SetOutOfUse();
		m_bSaveRequired = false;
		return false;
	}
	SetWindowTitle();
	m_pwndLeftView->BuildAllScreen2ViewVector();
	m_pwndLeftView->DocumentUpdated();
	m_pwndRightView->DocumentUpdated();
	m_pwndBottomView->DocumentUpdated();
	m_wndLocatorBar.DocumentUpdated();
	m_wndLineDiffBar.DocumentUpdated();

	m_pwndLeftView->SetWritable(false);
	m_pwndLeftView->SetWritableIsChangable(false);
	m_pwndLeftView->SetTarget(false);
	m_pwndRightView->SetWritable(false);
	m_pwndRightView->SetWritableIsChangable(false);
	m_pwndRightView->SetTarget(false);
	m_pwndBottomView->SetWritable(false);
	m_pwndBottomView->SetWritableIsChangable(false);
	m_pwndBottomView->SetTarget(false);

	if (!m_Data.IsBaseFileInUse())
	{
		CSysProgressDlg progDlg;
		if (m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
		{
			m_Data.m_baseFile.TransferDetailsFrom(m_Data.m_theirFile);
		}
		else if ((!m_Data.m_sDiffFile.IsEmpty())&&(!m_Patch.Init(m_Data.m_sDiffFile, m_Data.m_sPatchPath, &progDlg)))
		{
			progDlg.Stop();
			ClearViewNamesAndPaths();
			MessageBox(m_Patch.GetErrorMessage(), nullptr, MB_ICONERROR);
			m_bSaveRequired = false;
			return false;
		}
		progDlg.Stop();
		if (m_Patch.GetNumberOfFiles() > 0)
		{
			CString betterpatchpath = m_Patch.CheckPatchPath(m_Data.m_sPatchPath);
			if (betterpatchpath.CompareNoCase(m_Data.m_sPatchPath)!=0)
			{
				CString msg;
				msg.FormatMessage(IDS_WARNBETTERPATCHPATHFOUND, static_cast<LPCTSTR>(m_Data.m_sPatchPath), static_cast<LPCTSTR>(betterpatchpath));
				CTaskDialog taskdlg(msg,
									CString(MAKEINTRESOURCE(IDS_WARNBETTERPATCHPATHFOUND_TASK2)),
									L"TortoiseGitMerge",
									0,
									TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
				CString task3;
				WCHAR t3[MAX_PATH] = { 0 };
				CString cp = betterpatchpath.Left(MAX_PATH - 1);
				PathCompactPathEx(t3, cp, 50, 0);
				task3.Format(IDS_WARNBETTERPATCHPATHFOUND_TASK3, t3);
				taskdlg.AddCommandControl(1, task3);
				CString task4;
				WCHAR t4[MAX_PATH] = { 0 };
				cp = m_Data.m_sPatchPath.Left(MAX_PATH - 1);
				PathCompactPathEx(t4, cp, 50, 0);
				task4.Format(IDS_WARNBETTERPATCHPATHFOUND_TASK4, t4);
				taskdlg.AddCommandControl(2, task4);
				taskdlg.SetDefaultCommandControl(1);
				taskdlg.SetMainIcon(TD_INFORMATION_ICON);
				taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
				if (taskdlg.DoModal(m_hWnd) == 1)
				{
					m_Data.m_sPatchPath = betterpatchpath;
					m_Patch.Init(m_Data.m_sDiffFile, m_Data.m_sPatchPath, &progDlg);
				}
			}
			m_dlgFilePatches.Init(&m_Patch, this, m_Data.m_sPatchPath, this);
			m_dlgFilePatches.ShowWindow(SW_SHOW);
			ClearViewNamesAndPaths();
			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			m_pwndLeftView->SetHidden(FALSE);
			m_pwndRightView->SetHidden(FALSE);
			m_pwndBottomView->SetHidden(TRUE);
		}
	}
	if (m_Data.IsBaseFileInUse() && !m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
	{
		m_Data.m_yourFile.TransferDetailsFrom(m_Data.m_theirFile);
	}
	if (m_Data.IsBaseFileInUse() && m_Data.IsYourFileInUse() && !m_Data.IsTheirFileInUse())
	{
		//diff between YOUR and BASE
		if (m_bOneWay)
		{
			pwndActiveView = m_pwndLeftView;
			if (!m_wndSplitter2.IsColumnHidden(1))
				m_wndSplitter2.HideColumn(1);

			m_pwndLeftView->m_pViewData = &m_Data.m_YourBaseBoth;
			m_pwndLeftView->SetTextType(m_Data.m_arYourFile.GetUnicodeType());
			m_pwndLeftView->SetLineEndingStyle(m_Data.m_arYourFile.GetLineEndings());
			m_pwndLeftView->m_sWindowName = m_Data.m_baseFile.GetWindowName() + L" - " + m_Data.m_yourFile.GetWindowName();
			m_pwndLeftView->m_sFullFilePath = m_Data.m_baseFile.GetFilename() + L" - " + m_Data.m_yourFile.GetFilename();
			m_pwndLeftView->m_sReflectedName = m_Data.m_yourFile.GetReflectedName();
			m_pwndLeftView->m_pWorkingFile = &m_Data.m_yourFile;
			m_pwndLeftView->SetTarget();
			m_pwndLeftView->SetWritableIsChangable(true);

			m_pwndRightView->m_pViewData = nullptr;
			m_pwndRightView->m_pWorkingFile = nullptr;
			m_pwndBottomView->m_pViewData = nullptr;
			m_pwndBottomView->m_pWorkingFile = nullptr;

			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			m_pwndLeftView->SetHidden(FALSE);
			m_pwndRightView->SetHidden(TRUE);
			m_pwndBottomView->SetHidden(TRUE);
			::SetWindowPos(m_pwndLeftView->m_hWnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
		}
		else
		{
			pwndActiveView = m_pwndRightView;
			if (m_wndSplitter2.IsColumnHidden(1))
				m_wndSplitter2.ShowColumn();

			m_pwndLeftView->m_pViewData = &m_Data.m_YourBaseLeft;
			m_pwndLeftView->SetTextType(m_Data.m_arBaseFile.GetUnicodeType());
			m_pwndLeftView->SetLineEndingStyle(m_Data.m_arBaseFile.GetLineEndings());
			m_pwndLeftView->m_sWindowName = m_Data.m_baseFile.GetWindowName();
			m_pwndLeftView->m_sFullFilePath = m_Data.m_baseFile.GetFilename();
			m_pwndLeftView->m_sConvertedFilePath = m_Data.m_baseFile.GetConvertedFileName();
			m_pwndLeftView->m_sReflectedName = m_Data.m_baseFile.GetReflectedName();
			m_pwndLeftView->m_pWorkingFile = &m_Data.m_baseFile;
			m_pwndLeftView->SetWritableIsChangable(true);

			m_pwndRightView->m_pViewData = &m_Data.m_YourBaseRight;
			m_pwndRightView->SetTextType(m_Data.m_arYourFile.GetUnicodeType());
			m_pwndRightView->SetLineEndingStyle(m_Data.m_arYourFile.GetLineEndings());
			m_pwndRightView->m_sWindowName = m_Data.m_yourFile.GetWindowName();
			m_pwndRightView->m_sFullFilePath = m_Data.m_yourFile.GetFilename();
			m_pwndRightView->m_sConvertedFilePath = m_Data.m_yourFile.GetConvertedFileName();
			m_pwndRightView->m_sReflectedName = m_Data.m_yourFile.GetReflectedName();
			m_pwndRightView->m_pWorkingFile = &m_Data.m_yourFile;
			m_pwndRightView->SetWritable();
			m_pwndRightView->SetTarget();

			m_pwndBottomView->m_pViewData = nullptr;
			m_pwndBottomView->m_pWorkingFile = nullptr;

			if (!m_wndSplitter.IsRowHidden(1))
				m_wndSplitter.HideRow(1);
			m_pwndLeftView->SetHidden(FALSE);
			m_pwndRightView->SetHidden(FALSE);
			m_pwndBottomView->SetHidden(TRUE);
		}
		bool hasMods, hasConflicts, hasWhitespaceMods, hasFilteredMods;
		pwndActiveView->CheckModifications(hasMods, hasConflicts, hasWhitespaceMods, hasFilteredMods);
		if (!hasMods && !hasConflicts)
		{
			// files appear identical, show a dialog informing the user that there are or might
			// be other differences
			bool hasEncodingDiff = m_Data.m_arBaseFile.GetUnicodeType() != m_Data.m_arYourFile.GetUnicodeType();
			bool hasEOLDiff = m_Data.m_arBaseFile.GetLineEndings() != m_Data.m_arYourFile.GetLineEndings();
			if (hasWhitespaceMods || hasEncodingDiff || hasEOLDiff)
			{
				// text is identical, but the files do not match
				CString sWarning(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_MAIN));
				CString sWhitespace(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_WHITESPACE));
				CString sEncoding(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_ENCODING));
				CString sEOL(MAKEINTRESOURCE(IDS_TEXTIDENTICAL_EOL));
				if (hasWhitespaceMods)
				{
					sWarning += L"\r\n";
					sWarning += sWhitespace;
				}
				if (hasEncodingDiff)
				{
					sWarning += L"\r\n";
					sWarning += sEncoding;
					sWarning += L" (";
					sWarning += m_Data.m_arBaseFile.GetEncodingName(m_Data.m_arBaseFile.GetUnicodeType());
					sWarning += L", ";
					sWarning += m_Data.m_arYourFile.GetEncodingName(m_Data.m_arYourFile.GetUnicodeType());
					sWarning += L")";
				}
				if (hasEOLDiff)
				{
					sWarning += L"\r\n";
					sWarning += sEOL;
				}
				AfxMessageBox(sWarning, MB_ICONINFORMATION);
			}
		}
	}
	else if (m_Data.IsBaseFileInUse() && m_Data.IsYourFileInUse() && m_Data.IsTheirFileInUse())
	{
		//diff between THEIR, YOUR and BASE
		m_pwndBottomView->SetWritable();
		m_pwndBottomView->SetTarget();
		pwndActiveView = m_pwndBottomView;

		m_pwndLeftView->m_pViewData = &m_Data.m_TheirBaseBoth;
		m_pwndLeftView->SetTextType(m_Data.m_arTheirFile.GetUnicodeType());
		m_pwndLeftView->SetLineEndingStyle(m_Data.m_arTheirFile.GetLineEndings());
		m_pwndLeftView->m_sWindowName = m_Data.m_theirFile.GetWindowName(IDS_VIEWTITLE_THEIRS);
		m_pwndLeftView->m_sFullFilePath = m_Data.m_theirFile.GetFilename();
		m_pwndLeftView->m_sConvertedFilePath = m_Data.m_theirFile.GetConvertedFileName();
		m_pwndLeftView->m_sReflectedName = m_Data.m_theirFile.GetReflectedName();
		m_pwndLeftView->m_pWorkingFile = &m_Data.m_theirFile;

		m_pwndRightView->m_pViewData = &m_Data.m_YourBaseBoth;
		m_pwndRightView->SetTextType(m_Data.m_arYourFile.GetUnicodeType());
		m_pwndRightView->SetLineEndingStyle(m_Data.m_arYourFile.GetLineEndings());
		m_pwndRightView->m_sWindowName = m_Data.m_yourFile.GetWindowName(IDS_VIEWTITLE_MINE);
		m_pwndRightView->m_sFullFilePath = m_Data.m_yourFile.GetFilename();
		m_pwndRightView->m_sConvertedFilePath = m_Data.m_yourFile.GetConvertedFileName();
		m_pwndRightView->m_sReflectedName = m_Data.m_yourFile.GetReflectedName();
		m_pwndRightView->m_pWorkingFile = &m_Data.m_yourFile;

		m_pwndBottomView->m_pViewData = &m_Data.m_Diff3;
		m_pwndBottomView->SetTextType(m_Data.m_arTheirFile.GetUnicodeType());
		m_pwndBottomView->SetLineEndingStyle(m_Data.m_arTheirFile.GetLineEndings());
		m_pwndBottomView->m_sWindowName.LoadString(IDS_VIEWTITLE_MERGED);
		m_pwndBottomView->m_sWindowName += L" - " + m_Data.m_mergedFile.GetWindowName();
		m_pwndBottomView->m_sFullFilePath = m_Data.m_mergedFile.GetFilename();
		m_pwndBottomView->m_sConvertedFilePath = m_Data.m_mergedFile.GetConvertedFileName();
		m_pwndBottomView->m_sReflectedName = m_Data.m_mergedFile.GetReflectedName();
		m_pwndBottomView->m_pWorkingFile = &m_Data.m_mergedFile;

		if (m_wndSplitter2.IsColumnHidden(1))
			m_wndSplitter2.ShowColumn();
		if (m_wndSplitter.IsRowHidden(1))
			m_wndSplitter.ShowRow();
		m_pwndLeftView->SetHidden(FALSE);
		m_pwndRightView->SetHidden(FALSE);
		m_pwndBottomView->SetHidden(FALSE);
		// in three pane view, hide the line diff bar
		m_wndLineDiffBar.ShowPane(false, false, true);
		m_wndLineDiffBar.DocumentUpdated();
	}
	if (!m_Data.m_mergedFile.InUse())
	{
		m_Data.m_mergedFile.SetFileName(m_Data.m_yourFile.GetFilename());
	}
	m_pwndLeftView->BuildAllScreen2ViewVector();
	m_pwndLeftView->DocumentUpdated();
	m_pwndRightView->DocumentUpdated();
	m_pwndBottomView->DocumentUpdated();
	m_wndLocatorBar.DocumentUpdated();
	m_wndLineDiffBar.DocumentUpdated();
	UpdateLayout();
	SetActiveView(pwndActiveView);

	if ((line >= -1) && m_pwndRightView->m_pViewData)
	{
		int n = line == -1 ? min( nOldLineNumber, nOldLine ) : line;
		if (n >= 0)
		{
			n = m_pwndRightView->m_pViewData->FindLineNumber(n);
			if (m_bCollapsed)
			{
				// adjust the goto-line position if we're collapsed
				int step = m_pwndRightView->m_nTopLine > n ? -1 : 1;
				int skip = 0;
				for (int i = m_pwndRightView->m_nTopLine; i != n; i += step)
				{
					if (m_pwndRightView->m_pViewData->GetHideState(i) == HIDESTATE_HIDDEN)
						++skip;
				}
				if (m_pwndRightView->m_pViewData->GetHideState(n) == HIDESTATE_HIDDEN)
					OnViewTextFoldUnfold();
				else
					n = n + (skip * step * -1);
			}
		}
		if (n < 0)
			n = nOldLine;

		m_pwndRightView->ScrollAllToLine(n);
		POINT p;
		p.x = 0;
		p.y = n;
		if ((ptOldCaretPos.x >= 0) || (ptOldCaretPos.y >= 0))
			p = ptOldCaretPos;
		m_pwndLeftView->SetCaretPosition(p);
		m_pwndRightView->SetCaretPosition(p);
		m_pwndBottomView->SetCaretPosition(p);
		m_pwndBottomView->ScrollToChar(0);
		m_pwndLeftView->ScrollToChar(0);
		m_pwndRightView->ScrollToChar(0);
	}
	else
	{
		CRegDWORD regFirstDiff = CRegDWORD(L"Software\\TortoiseGitMerge\\FirstDiffOnLoad", TRUE);
		CRegDWORD regFirstConflict = CRegDWORD(L"Software\\TortoiseGitMerge\\FirstConflictOnLoad", TRUE);
		bool bGoFirstDiff = (0 != static_cast<DWORD>(regFirstDiff));
		bool bGoFirstConflict = (0 != static_cast<DWORD>(regFirstConflict));
		if (bGoFirstConflict && (CheckResolved()>=0))
		{
			pwndActiveView->GoToFirstConflict();
			// Ignore the first few Mouse Move messages, so that the line diff stays on
			// the first diff line until the user actually moves the mouse
			m_nMoveMovesToIgnore = MOVESTOIGNORE;
		}
		else if (bGoFirstDiff)
		{
			pwndActiveView->GoToFirstDifference();
			// Ignore the first few Mouse Move messages, so that the line diff stays on
			// the first diff line until the user actually moves the mouse
			m_nMoveMovesToIgnore = MOVESTOIGNORE;
		}
		else
		{
			// Avoid incorrect rendering of active pane.
			m_pwndBottomView->ScrollToChar(0);
			m_pwndLeftView->ScrollToChar(0);
			m_pwndRightView->ScrollToChar(0);
		}
	}
	CheckResolved();
	if (m_bHasConflicts && !m_bSaveRequiredOnConflicts)
		m_bSaveRequired = false;
	CUndo::GetInstance().Clear();
	return true;
}

void CMainFrame::UpdateLayout()
{
	if (m_bInitSplitter)
	{
		m_wndSplitter.CenterSplitter();
		m_wndSplitter2.CenterSplitter();
	}
}

void CMainFrame::RecalcLayout(BOOL bNotify)
{
	GetClientRect(&m_dockManager.m_rectInPlace);
	if (m_pRibbonApp)
		m_dockManager.m_rectInPlace.top += m_pRibbonApp->GetRibbonHeight();
	CFrameWndEx::RecalcLayout(bNotify);
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWndEx::OnSize(nType, cx, cy);
	if (m_bInitSplitter && nType != SIZE_MINIMIZED)
	{
		if (m_wndSplitter.GetSafeHwnd())
		{
			if (m_wndSplitter.HasOldRowSize() && (m_wndSplitter.GetOldRowCount() == 2))
			{
				int oldTotal = m_wndSplitter.GetOldRowSize(0) + m_wndSplitter.GetOldRowSize(1);
				if (oldTotal)
				{
					int cxCur0, cxCur1, cxMin0, cxMin1;
					m_wndSplitter.GetRowInfo(0, cxCur0, cxMin0);
					m_wndSplitter.GetRowInfo(1, cxCur1, cxMin1);
					cxCur0 = m_wndSplitter.GetOldRowSize(0) * (cxCur0 + cxCur1) / oldTotal;
					cxCur1 = m_wndSplitter.GetOldRowSize(1) * (cxCur0 + cxCur1) / oldTotal;
					m_wndSplitter.SetRowInfo(0, cxCur0, 0);
					m_wndSplitter.SetRowInfo(1, cxCur1, 0);
					m_wndSplitter.RecalcLayout();
				}
			}

			if (m_wndSplitter2.HasOldColSize() && (m_wndSplitter2.GetOldColCount() == 2))
			{
				int oldTotal = m_wndSplitter2.GetOldColSize(0) + m_wndSplitter2.GetOldColSize(1);
				if (oldTotal)
				{
					int cyCur0, cyCur1, cyMin0, cyMin1;
					m_wndSplitter2.GetColumnInfo(0, cyCur0, cyMin0);
					m_wndSplitter2.GetColumnInfo(1, cyCur1, cyMin1);
					cyCur0 = m_wndSplitter2.GetOldColSize(0) * (cyCur0 + cyCur1) / oldTotal;
					cyCur1 = m_wndSplitter2.GetOldColSize(1) * (cyCur0 + cyCur1) / oldTotal;
					m_wndSplitter2.SetColumnInfo(0, cyCur0, 0);
					m_wndSplitter2.SetColumnInfo(1, cyCur1, 0);
					m_wndSplitter2.RecalcLayout();
				}
			}
		}
	}
	if ((nType == SIZE_RESTORED)&&m_bCheckReload)
	{
		m_bCheckReload = false;
		CheckForReload();
	}
}

void CMainFrame::OnViewWhitespaces()
{
	CRegDWORD regViewWhitespaces = CRegDWORD(L"Software\\TortoiseGitMerge\\ViewWhitespaces", 1);
	BOOL bViewWhitespaces = regViewWhitespaces;
	if (m_pwndLeftView)
		bViewWhitespaces = m_pwndLeftView->m_bViewWhitespace;

	bViewWhitespaces = !bViewWhitespaces;
	regViewWhitespaces = bViewWhitespaces;
	if (m_pwndLeftView)
	{
		m_pwndLeftView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndLeftView->Invalidate();
	}
	if (m_pwndRightView)
	{
		m_pwndRightView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndRightView->Invalidate();
	}
	if (m_pwndBottomView)
	{
		m_pwndBottomView->m_bViewWhitespace = bViewWhitespaces;
		m_pwndBottomView->Invalidate();
	}
}

void CMainFrame::OnUpdateViewWhitespaces(CCmdUI *pCmdUI)
{
	if (m_pwndLeftView)
		pCmdUI->SetCheck(m_pwndLeftView->m_bViewWhitespace);
}

void CMainFrame::OnViewCollapsed()
{
	m_regCollapsed = !static_cast<DWORD>(m_regCollapsed);
	m_bCollapsed = !!static_cast<DWORD>(m_regCollapsed);

	OnViewTextFoldUnfold();
	m_wndLocatorBar.Invalidate();
}

void CMainFrame::OnUpdateViewCollapsed(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bCollapsed);
}

void CMainFrame::OnViewWraplonglines()
{
	m_bWrapLines = !static_cast<DWORD>(m_regWrapLines);
	m_regWrapLines = m_bWrapLines;

	if (m_pwndLeftView)   m_pwndLeftView  ->WrapChanged();
	if (m_pwndRightView)  m_pwndRightView ->WrapChanged();
	if (m_pwndBottomView) m_pwndBottomView->WrapChanged();
	OnViewTextFoldUnfold();
	m_wndLocatorBar.DocumentUpdated();
}

void CMainFrame::OnViewTextFoldUnfold()
{
	OnViewTextFoldUnfold(m_pwndLeftView);
	OnViewTextFoldUnfold(m_pwndRightView);
	OnViewTextFoldUnfold(m_pwndBottomView);
}

void CMainFrame::OnViewTextFoldUnfold(CBaseView* view)
{
	if (view == 0)
		return;
	view->BuildAllScreen2ViewVector();
	view->UpdateCaret();
	view->Invalidate();
	view->EnsureCaretVisible();
}

void CMainFrame::OnUpdateViewWraplonglines(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bWrapLines);
}

void CMainFrame::OnViewOnewaydiff()
{
	if (CheckForSave(CHFSR_RELOAD)==IDCANCEL)
		return;
	m_bOneWay = !m_bOneWay;
	m_regOneWay = m_bOneWay;
	ShowDiffBar(!m_bOneWay);
	LoadViews(-1);
}

void CMainFrame::DiffLeftToBase()
{
	DiffTwo(m_Data.m_baseFile, m_Data.m_theirFile);
}

void CMainFrame::DiffRightToBase()
{
	DiffTwo(m_Data.m_baseFile, m_Data.m_yourFile);
}

void CMainFrame::DiffTwo(const CWorkingFile& file1, const CWorkingFile& file2)
{
	wchar_t ownpath[MAX_PATH] = { 0 };
	GetModuleFileName(nullptr, ownpath, MAX_PATH);
	CString sCmd;
	sCmd.Format(L"\"%s\"", ownpath);
	sCmd += L" /base:\"";
	sCmd += file1.GetFilename();
	sCmd += L"\" /mine:\"";
	sCmd += file2.GetFilename();
	sCmd += L'"';
	if (!file1.GetWindowName().IsEmpty())
		sCmd += L" /basename:\"" + file1.GetWindowName() + L"\"";
	if (!file2.GetWindowName().IsEmpty())
		sCmd += L" /yourname:\"" + file2.GetWindowName() + L"\"";

	CAppUtils::LaunchApplication(sCmd, 0, false);
}

void CMainFrame::ShowDiffBar(bool bShow)
{
	if (bShow)
	{
		// restore the line diff bar
		m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
		m_wndLineDiffBar.DocumentUpdated();
		m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
		m_wndLocatorBar.DocumentUpdated();
	}
	else
	{
		// in one way view, hide the line diff bar
		m_wndLineDiffBar.ShowPane(false, false, true);
		m_wndLineDiffBar.DocumentUpdated();
	}
}

int CMainFrame::CheckResolved()
{
	//only in three way diffs can be conflicts!
	m_bHasConflicts = true;
	if (IsViewGood(m_pwndBottomView))
	{
		CViewData* viewdata = m_pwndBottomView->m_pViewData;
		if (viewdata)
		{
			for (int i=0; i<viewdata->GetCount(); i++)
			{
				const DiffStates state = viewdata->GetState(i);
				if ((DIFFSTATE_CONFLICTED == state)||(DIFFSTATE_CONFLICTED_IGNORED == state))
					return i;
			}
		}
	}
	m_bHasConflicts = false;
	return -1;
}

int CMainFrame::SaveFile(const CString& sFilePath)
{
	CBaseView* pView = nullptr;
	CViewData* pViewData = nullptr;
	CFileTextLines * pOriginFile = &m_Data.m_arBaseFile;
	if (IsViewGood(m_pwndBottomView))
	{
		pView = m_pwndBottomView;
		pViewData = m_pwndBottomView->m_pViewData;
	}
	else if (IsViewGood(m_pwndRightView))
	{
		pView = m_pwndRightView;
		pViewData = m_pwndRightView->m_pViewData;
		if (m_Data.IsYourFileInUse())
			pOriginFile = &m_Data.m_arYourFile;
		else if (m_Data.IsTheirFileInUse())
			pOriginFile = &m_Data.m_arTheirFile;
	}
	else
	{
		// nothing to save!
		return 1;
	}
	Invalidate();
	if ((pViewData)&&(pOriginFile))
	{
		CFileTextLines file;
		pOriginFile->CopySettings(&file);
		CFileTextLines::SaveParams saveParams;
		saveParams.m_LineEndings = pView->GetLineEndings();
		saveParams.m_UnicodeType = pView->GetTextType();
		file.SetSaveParams(saveParams);
		for (int i=0; i<pViewData->GetCount(); i++)
		{
			//only copy non-removed lines
			DiffStates state = pViewData->GetState(i);
			switch (state)
			{
			case DIFFSTATE_CONFLICTED:
			case DIFFSTATE_CONFLICTED_IGNORED:
				{
					int first = i;
					int last = i;
					do
					{
						last++;
					} while((last<pViewData->GetCount()) && ((pViewData->GetState(last)==DIFFSTATE_CONFLICTED)||(pViewData->GetState(last)==DIFFSTATE_CONFLICTED_IGNORED)));
					// TortoiseGitMerge changes here
					file.Add(L"<<<<<<< .mine", m_pwndRightView->GetLineEndings());
					for (int j=first; j<last; j++)
					{
						EOL lineending = m_pwndRightView->m_pViewData->GetLineEnding(j);
						if (lineending == EOL_NOENDING)
							lineending = m_pwndRightView->GetLineEndings();
						file.Add(m_pwndRightView->m_pViewData->GetLine(j), lineending);
					}
					file.Add(L"=======", m_pwndRightView->GetLineEndings());
					for (int j=first; j<last; j++)
					{
						EOL lineending = m_pwndLeftView->m_pViewData->GetLineEnding(j);
						if (lineending == EOL_NOENDING)
							lineending = m_pwndLeftView->GetLineEndings();
						file.Add(m_pwndLeftView->m_pViewData->GetLine(j), lineending);
					}
					file.Add(L">>>>>>> .theirs", m_pwndRightView->GetLineEndings());
					i = last-1;
				}
				break;
			case DIFFSTATE_EMPTY:
			case DIFFSTATE_CONFLICTEMPTY:
			case DIFFSTATE_IDENTICALREMOVED:
			case DIFFSTATE_REMOVED:
			case DIFFSTATE_THEIRSREMOVED:
			case DIFFSTATE_YOURSREMOVED:
			case DIFFSTATE_CONFLICTRESOLVEDEMPTY:
				// do not save removed lines
				break;
			default:
				file.Add(pViewData->GetLine(i), pViewData->GetLineEnding(i));
				break;
			}
		}
		if (!file.Save(sFilePath, false, false))
		{
			CMessageBox::Show(m_hWnd, file.GetErrorString(), L"TortoiseGitMerge", MB_ICONERROR);
			return -1;
		}
		if (sFilePath == m_Data.m_baseFile.GetFilename())
		{
			m_Data.m_baseFile.StoreFileAttributes();
		}
		if (sFilePath == m_Data.m_theirFile.GetFilename())
		{
			m_Data.m_theirFile.StoreFileAttributes();
		}
		if (sFilePath == m_Data.m_yourFile.GetFilename())
		{
			m_Data.m_yourFile.StoreFileAttributes();
		}
		/*if (sFilePath == m_Data.m_mergedFile.GetFilename())
		{
			m_Data.m_mergedFile.StoreFileAttributes();
		}//*/
		m_dlgFilePatches.SetFileStatusAsPatched(sFilePath);
		if (IsViewGood(m_pwndBottomView))
			m_pwndBottomView->SetModified(FALSE);
		else if (IsViewGood(m_pwndRightView))
			m_pwndRightView->SetModified(FALSE);
		CUndo::GetInstance().MarkAsOriginalState(
				false,
				IsViewGood(m_pwndRightView) && (pViewData == m_pwndRightView->m_pViewData),
				IsViewGood(m_pwndBottomView) && (pViewData == m_pwndBottomView->m_pViewData));
		if (file.GetCount() == 1 && file.GetAt(0).IsEmpty() && file.GetLineEnding(0) == EOL_NOENDING)
			return 0;
		return file.GetCount();
	}
	return 1;
}

void CMainFrame::OnFileSave()
{
	// when multiple files are set as writable we have to ask what file to save
	int nEditableViewCount =
			(CBaseView::IsViewGood(m_pwndLeftView) && m_pwndLeftView->IsWritable() ? 1 : 0)
			+ (CBaseView::IsViewGood(m_pwndRightView) && m_pwndRightView->IsWritable() ? 1 : 0)
			+ (CBaseView::IsViewGood(m_pwndBottomView) && m_pwndBottomView->IsWritable() ? 1 : 0);
	bool bLeftIsModified = CBaseView::IsViewGood(m_pwndLeftView) && m_pwndLeftView->IsModified();
	bool bRightIsModified = CBaseView::IsViewGood(m_pwndRightView) && m_pwndRightView->IsModified();
	bool bBottomIsModified = CBaseView::IsViewGood(m_pwndBottomView) && m_pwndBottomView->IsModified();
	int nModifiedViewCount =
			(bLeftIsModified ? 1 : 0)
			+ (bRightIsModified ? 1 : 0)
			+ (bBottomIsModified ? 1 : 0);
	if (nEditableViewCount>1)
	{
		if (nModifiedViewCount == 1)
		{
			if (bLeftIsModified)
				m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
			else
				FileSave();
		}
		else if (nModifiedViewCount>0)
		{
			// both views
			CTaskDialog taskdlg(CString(MAKEINTRESOURCE(IDS_SAVE_MORE)),
				CString(MAKEINTRESOURCE(IDS_SAVE)),
				CString(MAKEINTRESOURCE(IDS_APPNAME)),
				0,
				TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
			CString sTaskTemp;
			if (m_pwndLeftView->m_pWorkingFile->InUse() && !m_pwndLeftView->m_pWorkingFile->IsReadonly())
				sTaskTemp.Format(IDS_ASKFORSAVE_SAVELEFT, static_cast<LPCTSTR>(m_pwndLeftView->m_pWorkingFile->GetFilename()));
			else
				sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVELEFTAS));
			taskdlg.AddCommandControl(201, sTaskTemp, bLeftIsModified);// left
			if (bLeftIsModified)
				taskdlg.SetDefaultCommandControl(201);
			if (m_pwndRightView->m_pWorkingFile->InUse() && !m_pwndRightView->m_pWorkingFile->IsReadonly())
				sTaskTemp.Format(IDS_ASKFORSAVE_SAVERIGHT, static_cast<LPCTSTR>(m_pwndRightView->m_pWorkingFile->GetFilename()));
			else
				sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVERIGHTAS));
			taskdlg.AddCommandControl(202, sTaskTemp, bRightIsModified); // right
			if (bRightIsModified)
				taskdlg.SetDefaultCommandControl(202);
			taskdlg.AddCommandControl(203, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVEALL2)), nModifiedViewCount>1); // both
			if (nModifiedViewCount > 1)
				taskdlg.SetDefaultCommandControl(203);
			taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
			taskdlg.SetMainIcon(TD_WARNING_ICON);
			UINT ret = static_cast<UINT>(taskdlg.DoModal(m_hWnd));
			switch (ret)
			{
			case 201: // left
				m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
				break;
			case 203: // both
				m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
			case 202: // right
				m_pwndRightView->SaveFile();
				break;
			}
		}
	}
	else
	{
		// only target view was modified
		FileSave();
	}
}

void CMainFrame::PatchSave()
{
	bool bDoesNotExist = !PathFileExists(m_Data.m_mergedFile.GetFilename());
	if (m_Data.m_bPatchRequired)
	{
		m_Patch.PatchPath(m_Data.m_mergedFile.GetFilename());
	}
	if (!PathIsDirectory(m_Data.m_mergedFile.GetFilename()))
	{
		int saveret = SaveFile(m_Data.m_mergedFile.GetFilename());
		if (saveret==0)
		{
			// file was saved with 0 lines, remove it.
			m_Patch.RemoveFile(m_Data.m_mergedFile.GetFilename());
			// just in case
			DeleteFile(m_Data.m_mergedFile.GetFilename());
		}
		m_Data.m_mergedFile.StoreFileAttributes();
		if (m_Data.m_mergedFile.GetFilename() == m_Data.m_yourFile.GetFilename())
			m_Data.m_yourFile.StoreFileAttributes();
		if (bDoesNotExist && (DWORD(CRegDWORD(L"Software\\TortoiseGitMerge\\AutoAdd", TRUE))))
		{
			// call TortoiseProc to add the new file to version control
			CString cmd = L"/command:add /noui /path:\"";
			cmd += m_Data.m_mergedFile.GetFilename() + L'"';
			CAppUtils::RunTortoiseGitProc(cmd);
		}
	}
}

bool CMainFrame::FileSave(bool bCheckResolved /*=true*/)
{
	if (HasMarkedBlocks())
	{
		CString sTitle(MAKEINTRESOURCE(IDS_WARNMARKEDBLOCKS));
		CString sSubTitle(MAKEINTRESOURCE(IDS_ASKFORSAVE_MARKEDBLOCKS));
		CString sAppName(MAKEINTRESOURCE(IDS_APPNAME));
		CTaskDialog taskdlg(sTitle,
			sSubTitle,
			sAppName,
			0,
			TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
		taskdlg.AddCommandControl(10, CString(MAKEINTRESOURCE(IDS_MARKEDBLOCKSSAVEINCLUDE)));
		taskdlg.AddCommandControl(11, CString(MAKEINTRESOURCE(IDS_MARKEDBLOCKSSAVEEXCLUDE)));
		taskdlg.AddCommandControl(12, CString(MAKEINTRESOURCE(IDS_MARKEDBLCOKSSAVEIGNORE)));
		taskdlg.AddCommandControl(IDCANCEL, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_CANCEL_OPEN)));
		taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
		taskdlg.SetDefaultCommandControl(10);
		taskdlg.SetMainIcon(TD_WARNING_ICON);
		UINT ret = static_cast<UINT>(taskdlg.DoModal(m_hWnd));
		if (ret == 10)
			m_pwndRightView->LeaveOnlyMarkedBlocks();
		else if (ret == 11)
			m_pwndRightView->UseViewFileOfMarked();
		else if (ret == 12)
			m_pwndRightView->UseViewFileExceptEdited();
		else
			return false;
	}

	if (!m_Data.m_mergedFile.InUse())
		return FileSaveAs(bCheckResolved);
	// check if the file has the readonly attribute set
	bool bDoesNotExist = false;
	DWORD fAttribs = GetFileAttributes(m_Data.m_mergedFile.GetFilename());
	if ((fAttribs != INVALID_FILE_ATTRIBUTES)&&(fAttribs & FILE_ATTRIBUTE_READONLY))
		return FileSaveAs(bCheckResolved);
	if (fAttribs == INVALID_FILE_ATTRIBUTES)
	{
		bDoesNotExist = (GetLastError() == ERROR_FILE_NOT_FOUND);
	}
	if (bCheckResolved && HasConflictsWontKeep())
		return false;

	if ((static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGitMerge\\Backup"))) != 0)
	{
		MoveFileEx(m_Data.m_mergedFile.GetFilename(), m_Data.m_mergedFile.GetFilename() + L".bak", MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
	}
	if (m_Data.m_bPatchRequired)
	{
		m_Patch.PatchPath(m_Data.m_mergedFile.GetFilename());
	}
	int saveret = SaveFile(m_Data.m_mergedFile.GetFilename());
	if (saveret==0)
	{
		// file was saved with 0 lines!
		// ask the user if the file should be deleted
		CString msg;
		msg.Format(IDS_DELETEWHENEMPTY, static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(m_Data.m_mergedFile.GetFilename())));
		CTaskDialog taskdlg(msg,
							CString(MAKEINTRESOURCE(IDS_DELETEWHENEMPTY_TASK2)),
							CString(MAKEINTRESOURCE(IDS_APPNAME)),
							0,
							TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
		taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_DELETEWHENEMPTY_TASK3)));
		taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_DELETEWHENEMPTY_TASK4)));
		taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
		taskdlg.SetDefaultCommandControl(1);
		taskdlg.SetMainIcon(TD_WARNING_ICON);
		if (taskdlg.DoModal(m_hWnd) == 1)
		{
			m_Patch.RemoveFile(m_Data.m_mergedFile.GetFilename());
			DeleteFile(m_Data.m_mergedFile.GetFilename());
		}
	}
	else if (saveret < 0)
	{
		// error while saving the file
		return false;
	}

	// if we're in conflict resolve mode (three pane view), check if there are no more conflicts
	// and if there aren't, ask to mark the file as resolved
	if (IsViewGood(m_pwndBottomView) && !m_bHasConflicts && bCheckResolved)
	{
		CString projectRoot;
		if (GitAdminDir::HasAdminDir(m_Data.m_mergedFile.GetFilename(), false, &projectRoot))
		{
			CString subpath = m_Data.m_mergedFile.GetFilename();
			subpath.Replace(L'\\', L'/');
			if (subpath.GetLength() >= projectRoot.GetLength())
			{
				if (subpath[projectRoot.GetLength()] == L'/')
					subpath = subpath.Right(subpath.GetLength() - projectRoot.GetLength() - 1);
				else
					subpath = subpath.Right(subpath.GetLength() - projectRoot.GetLength());
			}

			CAutoRepository repository(projectRoot);
			bool hasConflictInIndex = false;
			do
			{
				if (!repository)
					break;

				CAutoIndex index;
				if (git_repository_index(index.GetPointer(), repository))
					break;

				CStringA path = CUnicodeUtils::GetMulti(subpath, CP_UTF8);
				hasConflictInIndex = git_index_get_bypath(index, path, 1) || git_index_get_bypath(index, path, 2);
			} while(0);

			if (hasConflictInIndex)
			{
				CString msg;
				msg.Format(IDS_MARKASRESOLVED, static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(m_Data.m_mergedFile.GetFilename())));
				CTaskDialog taskdlg(msg,
					CString(MAKEINTRESOURCE(IDS_MARKASRESOLVED_TASK2)),
					CString(MAKEINTRESOURCE(IDS_APPNAME)),
					0,
					TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
				taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_MARKASRESOLVED_TASK3)));
				taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_MARKASRESOLVED_TASK4)));
				taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
				taskdlg.SetDefaultCommandControl(1);
				taskdlg.SetMainIcon(TD_WARNING_ICON);
				if (taskdlg.DoModal(m_hWnd) == 1)
					MarkAsResolved();
			}
		}
	}

	m_bSaveRequired = false;
	m_Data.m_mergedFile.StoreFileAttributes();

	if (bDoesNotExist && (DWORD(CRegDWORD(L"Software\\TortoiseGitMerge\\AutoAdd", TRUE))))
	{
		// call TortoiseProc to add the new file to version control
		CString cmd = L"/command:add /noui /path:\"";
		cmd += m_Data.m_mergedFile.GetFilename() + L'"';
		if(!CAppUtils::RunTortoiseGitProc(cmd))
			return false;
	}
	return true;
}

void CMainFrame::OnFileSaveAs()
{
	// ask what file to save as
	bool bHaveConflict = (CheckResolved() >= 0);
	CTaskDialog taskdlg(
			CString(MAKEINTRESOURCE(bHaveConflict ? IDS_ASKFORSAVEAS_MORECONFLICT : IDS_ASKFORSAVEAS_MORE)),
			CString(MAKEINTRESOURCE(IDS_ASKFORSAVEAS)),
			CString(MAKEINTRESOURCE(IDS_APPNAME)),
			0,
			TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
	// default can be last view (target) as was in 1.7 or actual (where is cursor) as is in most text editor
	if (IsViewGood(m_pwndLeftView))
	{
		taskdlg.AddCommandControl(201, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVELEFTAS))); // left
		taskdlg.SetDefaultCommandControl(201);
	}
	if (IsViewGood(m_pwndRightView))
	{
		taskdlg.AddCommandControl(202, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVERIGHTAS))); // right
		taskdlg.SetDefaultCommandControl(202);
	}
	if (IsViewGood(m_pwndBottomView))
	{
		taskdlg.AddCommandControl(203, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVEBOTTOMAS))); // bottom
		taskdlg.SetDefaultCommandControl(203);
	}
	if (bHaveConflict)
	{
		taskdlg.AddCommandControl(204, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_NEEDRESOLVE))); // resolve
		taskdlg.SetDefaultCommandControl(204);
	}
	taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
	taskdlg.SetMainIcon(bHaveConflict ? TD_WARNING_ICON : TD_INFORMATION_ICON);
	int nCommand = static_cast<int>(taskdlg.DoModal(m_hWnd));
	CString sFileName;
	switch (nCommand)
	{
	case 201: // left
		if (TryGetFileName(sFileName))
		{
			// in 2, 3 view display we want to keep removed lines
			m_pwndLeftView->SaveFileTo(sFileName, IsViewGood(m_pwndRightView) ? SAVE_REMOVEDLINES : 0);
		}
		break;
	case 202: // right
		if (TryGetFileName(sFileName))
		{
			m_pwndRightView->SaveFileTo(sFileName);
		}
		break;
	case 203: // bottom
		FileSaveAs();
		break;
	case 204: // continue resolving
		if (IsViewGood(m_pwndBottomView))
		{
			m_pwndBottomView->GoToLine(CheckResolved());
		}
		break;
	}
}

bool CMainFrame::FileSaveAs(bool bCheckResolved /*=true*/)
{
	if (bCheckResolved && HasConflictsWontKeep())
		return false;

	CString fileName;
	if(!TryGetFileName(fileName))
		return false;

	SaveFile(fileName);
	return true;
}

void CMainFrame::OnUpdateFileSave(CCmdUI *pCmdUI)
{
	BOOL bEnable = FALSE;
	if (m_Data.m_mergedFile.InUse())
	{
		if (IsViewGood(m_pwndBottomView)&&(m_pwndBottomView->m_pViewData))
			bEnable = TRUE;
		else if ( (IsViewGood(m_pwndRightView)&&(m_pwndRightView->m_pViewData)) &&
				  (m_pwndRightView->IsModified() || (m_Data.m_yourFile.GetWindowName().Right(static_cast<int>(wcslen(L": patched"))).Compare(L": patched") == 0)))
			bEnable = TRUE;
		else if (IsViewGood(m_pwndLeftView)
				&& (m_pwndLeftView->m_pViewData)
				&& (m_pwndLeftView->IsModified()))
			bEnable = TRUE;
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateFileSaveAs(CCmdUI *pCmdUI)
{
	// any file is open we can save it as
	BOOL bEnable = FALSE;
	if (IsViewGood(m_pwndBottomView)&&(m_pwndBottomView->m_pViewData))
		bEnable = TRUE;
	else if (IsViewGood(m_pwndRightView)&&(m_pwndRightView->m_pViewData))
		bEnable = TRUE;
	else if (IsViewGood(m_pwndLeftView)&&(m_pwndLeftView->m_pViewData))
		bEnable = TRUE;
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateViewOnewaydiff(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(!m_bOneWay);
	BOOL bEnable = TRUE;
	if (IsViewGood(m_pwndBottomView))
	{
		bEnable = FALSE;
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnViewOptions()
{
	CString sTemp;
	sTemp.LoadString(IDS_SETTINGSTITLE);
	CSettings dlg(sTemp);
	dlg.DoModal();
	if (dlg.IsReloadNeeded())
	{
		if (CheckForSave(CHFSR_OPTIONS)==IDCANCEL)
			return;
		CDiffColors::GetInstance().LoadRegistry();
		LoadViews();
		return;
	}
	CDiffColors::GetInstance().LoadRegistry();
	if (m_pwndBottomView)
		m_pwndBottomView->DocumentUpdated();
	if (m_pwndLeftView)
		m_pwndLeftView->DocumentUpdated();
	if (m_pwndRightView)
		m_pwndRightView->DocumentUpdated();
}

void CMainFrame::OnClose()
{
	if (!IsWindowEnabled())
		return; // just in case someone sends a WM_CLOSE to the main window while another window (dialog,...) is open
	if (CheckForSave(CHFSR_CLOSE)!=IDCANCEL)
	{
		WINDOWPLACEMENT wp;

		// before it is destroyed, save the position of the window
		wp.length = sizeof wp;

		if (GetWindowPlacement(&wp))
		{

			if (IsIconic())
				// never restore to Iconic state
				wp.showCmd = SW_SHOW ;

			if ((wp.flags & WPF_RESTORETOMAXIMIZED) != 0)
				// if maximized and maybe iconic restore maximized state
				wp.showCmd = SW_SHOWMAXIMIZED ;

			// and write it
			WriteWindowPlacement(&wp);
		}
		__super::OnClose();
	}
}

void CMainFrame::OnActivate(UINT nValue, CWnd* /*pwnd*/, BOOL /*bActivated?*/)
{
	if (nValue != 0) // activated
	{
		if (IsIconic())
		{
			m_bCheckReload = TRUE;
		}
		else
		{
			// use a timer to give other messages time to get processed
			// first, like e.g. the WM_CLOSE message in case the user
			// clicked the close button and that brought the window
			// to the front - in that case checking for reload wouldn't
			// do any good.
			SetTimer(IDT_RELOADCHECKTIMER, 300, nullptr);
		}
	}
}

void CMainFrame::OnViewLinedown()
{
	OnViewLineUpDown(1);
}

void CMainFrame::OnViewLineup()
{
	OnViewLineUpDown(-1);
}

void CMainFrame::OnViewLineUpDown(int direction)
{
	if (m_pwndLeftView)
		m_pwndLeftView->ScrollToLine(m_pwndLeftView->m_nTopLine+direction);
	if (m_pwndRightView)
		m_pwndRightView->ScrollToLine(m_pwndRightView->m_nTopLine+direction);
	if (m_pwndBottomView)
		m_pwndBottomView->ScrollToLine(m_pwndBottomView->m_nTopLine+direction);
	m_wndLocatorBar.Invalidate();
	m_nMoveMovesToIgnore = MOVESTOIGNORE;
}

void CMainFrame::OnViewLineleft()
{
	OnViewLineLeftRight(-1);
}

void CMainFrame::OnViewLineright()
{
	OnViewLineLeftRight(1);
}

void CMainFrame::OnViewLineLeftRight(int direction)
{
	if (m_pwndLeftView)
		m_pwndLeftView->ScrollSide(direction);
	if (m_pwndRightView)
		m_pwndRightView->ScrollSide(direction);
	if (m_pwndBottomView)
		m_pwndBottomView->ScrollSide(direction);
}

void CMainFrame::OnEditUseTheirs()
{
	if (m_pwndBottomView)
		m_pwndBottomView->UseTheirTextBlock();
}
void CMainFrame::OnUpdateEditUsetheirblock(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_pwndBottomView && m_pwndBottomView->HasSelection());
}

void CMainFrame::OnEditUseMine()
{
	if (m_pwndBottomView)
		m_pwndBottomView->UseMyTextBlock();
}
void CMainFrame::OnUpdateEditUsemyblock(CCmdUI *pCmdUI)
{
	OnUpdateEditUsetheirblock(pCmdUI);
}

void CMainFrame::OnEditUseTheirsThenMine()
{
	if (m_pwndBottomView)
		m_pwndBottomView->UseTheirAndYourBlock();
}

void CMainFrame::OnUpdateEditUsetheirthenmyblock(CCmdUI *pCmdUI)
{
	OnUpdateEditUsetheirblock(pCmdUI);
}

void CMainFrame::OnEditUseMineThenTheirs()
{
	if (m_pwndBottomView)
		m_pwndBottomView->UseYourAndTheirBlock();
}

void CMainFrame::OnUpdateEditUseminethentheirblock(CCmdUI *pCmdUI)
{
	OnUpdateEditUsetheirblock(pCmdUI);
}

void CMainFrame::OnEditUseleftblock()
{
	if (m_pwndBottomView->IsWindowVisible())
		m_pwndBottomView->UseRightBlock();
	else
		m_pwndRightView->UseLeftBlock();
}

void CMainFrame::OnUpdateEditUseleftblock(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsViewGood(m_pwndRightView) && m_pwndRightView->IsTarget() && m_pwndRightView->HasSelection());
}

void CMainFrame::OnUpdateUseBlock(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(TRUE);
}

void CMainFrame::OnEditUseleftfile()
{
	if (m_pwndBottomView->IsWindowVisible())
		m_pwndBottomView->UseRightFile();
	else
		m_pwndRightView->UseLeftFile();
}

void CMainFrame::OnUpdateEditUseleftfile(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsViewGood(m_pwndRightView) && m_pwndRightView->IsTarget());
}

void CMainFrame::OnEditUseblockfromleftbeforeright()
{
	if (m_pwndRightView)
		m_pwndRightView->UseBothLeftFirst();
}

void CMainFrame::OnUpdateEditUseblockfromleftbeforeright(CCmdUI *pCmdUI)
{
	OnUpdateEditUseleftblock(pCmdUI);
}

void CMainFrame::OnEditUseblockfromrightbeforeleft()
{
	if (m_pwndRightView)
		m_pwndRightView->UseBothRightFirst();
}

void CMainFrame::OnUpdateEditUseblockfromrightbeforeleft(CCmdUI *pCmdUI)
{
	OnUpdateEditUseleftblock(pCmdUI);
}

void CMainFrame::OnFileReload()
{
	if (CheckForSave(CHFSR_RELOAD)==IDCANCEL)
		return;
	CDiffColors::GetInstance().LoadRegistry();
	LoadViews(-1);
}

void CMainFrame::ActivateFrame(int nCmdShow)
{
	// nCmdShow is the normal show mode this frame should be in
	// translate default nCmdShow (-1)
	if (nCmdShow == -1)
	{
		if (!IsWindowVisible())
			nCmdShow = SW_SHOWNORMAL;
		else if (IsIconic())
			nCmdShow = SW_RESTORE;
	}

	// bring to top before showing
	BringToTop(nCmdShow);

	if (nCmdShow != -1)
	{
		// show the window as specified
		WINDOWPLACEMENT wp;

		if ( !ReadWindowPlacement(&wp) )
		{
			ShowWindow(nCmdShow);
		}
		else
		{
			if ( nCmdShow != SW_SHOWNORMAL )
				wp.showCmd = nCmdShow;

			SetWindowPlacement(&wp);
		}

		// and finally, bring to top after showing
		BringToTop(nCmdShow);
	}
}

BOOL CMainFrame::ReadWindowPlacement(WINDOWPLACEMENT * pwp)
{
	CRegString placement = CRegString(L"Software\\TortoiseGitMerge\\WindowPos");
	CString sPlacement = placement;
	if (sPlacement.IsEmpty())
		return FALSE;
	int nRead = swscanf_s(sPlacement, L"%u,%u,%d,%d,%d,%d,%d,%d,%d,%d",
				&pwp->flags, &pwp->showCmd,
				&pwp->ptMinPosition.x, &pwp->ptMinPosition.y,
				&pwp->ptMaxPosition.x, &pwp->ptMaxPosition.y,
				&pwp->rcNormalPosition.left,  &pwp->rcNormalPosition.top,
				&pwp->rcNormalPosition.right, &pwp->rcNormalPosition.bottom);
	if ( nRead != 10 )
		return FALSE;
	pwp->length = sizeof(WINDOWPLACEMENT);

	return TRUE;
}

void CMainFrame::WriteWindowPlacement(WINDOWPLACEMENT * pwp)
{
	CRegString placement(L"Software\\TortoiseGitMerge\\WindowPos");
	TCHAR szBuffer[_countof("-32767")*8 + sizeof("65535")*2];

	swprintf_s(szBuffer, L"%u,%u,%d,%d,%d,%d,%d,%d,%d,%d",
			pwp->flags, pwp->showCmd,
			pwp->ptMinPosition.x, pwp->ptMinPosition.y,
			pwp->ptMaxPosition.x, pwp->ptMaxPosition.y,
			pwp->rcNormalPosition.left, pwp->rcNormalPosition.top,
			pwp->rcNormalPosition.right, pwp->rcNormalPosition.bottom);
	placement = szBuffer;
}

void CMainFrame::OnUpdateMergeMarkasresolved(CCmdUI *pCmdUI)
{
	if (!pCmdUI)
		return;
	BOOL bEnable = FALSE;
	if ((!m_bReadOnly)&&(m_Data.m_mergedFile.InUse()))
	{
		if (IsViewGood(m_pwndBottomView)&&(m_pwndBottomView->m_pViewData))
		{
			bEnable = TRUE;
		}
	}
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnMergeMarkasresolved()
{
	if(HasConflictsWontKeep())
		return;

	// now check if the file has already been saved and if not, save it.
	if (m_Data.m_mergedFile.InUse())
	{
		if (IsViewGood(m_pwndBottomView)&&(m_pwndBottomView->m_pViewData))
		{
			if (!FileSave(false))
				return;
			m_bSaveRequired = false;
		}
	}
	MarkAsResolved();
}

BOOL CMainFrame::MarkAsResolved()
{
	if (m_bReadOnly)
		return FALSE;
	if (!IsViewGood(m_pwndBottomView))
		return FALSE;

	CString cmd = L"/command:resolve /path:\"";
	cmd += m_Data.m_mergedFile.GetFilename();
	cmd += L"\" /closeonend:1 /noquestion /skipcheck /silent";
	if (resolveMsgWnd)
	{
		CString s;
		s.Format(L" /resolvemsghwnd:%I64d /resolvemsgwparam:%I64d /resolvemsglparam:%I64d", reinterpret_cast<__int64>(resolveMsgWnd), static_cast<__int64>(resolveMsgWParam), static_cast<__int64>(resolveMsgLParam));
		cmd += s;
	}
	if(!CAppUtils::RunTortoiseGitProc(cmd))
		return FALSE;
	m_bSaveRequired = false;
	return TRUE;
}

void CMainFrame::OnUpdateMergeNextconflict(CCmdUI *pCmdUI)
{
	BOOL bShow = FALSE;
	if (HasNextConflict(m_pwndBottomView))
		bShow = TRUE;
	else if (HasNextConflict(m_pwndRightView))
		bShow = TRUE;
	else if (HasNextConflict(m_pwndLeftView))
		bShow = TRUE;
	pCmdUI->Enable(bShow);
}

bool CMainFrame::HasNextConflict(CBaseView* view)
{
	if (view == 0)
		return false;
	if (!view->IsTarget())
		return false;
	return view->HasNextConflict();
}

void CMainFrame::OnUpdateMergePreviousconflict(CCmdUI *pCmdUI)
{
	BOOL bShow = FALSE;
	if (HasPrevConflict(m_pwndBottomView))
		bShow = TRUE;
	else if (HasPrevConflict(m_pwndRightView))
		bShow = TRUE;
	else if (HasPrevConflict(m_pwndLeftView))
		bShow = TRUE;
	pCmdUI->Enable(bShow);
}

bool CMainFrame::HasPrevConflict(CBaseView* view)
{
	if (view == 0)
		return false;
	if (!view->IsTarget())
		return false;
	return view->HasPrevConflict();
}

void CMainFrame::OnUpdateNavigateNextdifference(CCmdUI *pCmdUI)
{
	CBaseView* baseView = GetActiveBaseView();
	BOOL bShow = FALSE;
	if (baseView != 0)
		bShow = baseView->HasNextDiff();
	pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateNavigatePreviousdifference(CCmdUI *pCmdUI)
{
	CBaseView* baseView = GetActiveBaseView();
	BOOL bShow = FALSE;
	if (baseView != 0)
		bShow = baseView->HasPrevDiff();
	pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateNavigateNextinlinediff(CCmdUI *pCmdUI)
{
	BOOL bShow = FALSE;
	if (HasNextInlineDiff(m_pwndBottomView))
		bShow = TRUE;
	else if (HasNextInlineDiff(m_pwndRightView))
		bShow = TRUE;
	else if (HasNextInlineDiff(m_pwndLeftView))
		bShow = TRUE;
	pCmdUI->Enable(bShow);
}

bool CMainFrame::HasNextInlineDiff(CBaseView* view)
{
	if (view == 0)
		return false;
	if (!view->IsTarget())
		return false;
	return view->HasNextInlineDiff();
}

void CMainFrame::OnUpdateNavigatePrevinlinediff(CCmdUI *pCmdUI)
{
	BOOL bShow = FALSE;
	if (HasPrevInlineDiff(m_pwndBottomView))
		bShow = TRUE;
	else if (HasPrevInlineDiff(m_pwndRightView))
		bShow = TRUE;
	else if (HasPrevInlineDiff(m_pwndLeftView))
		bShow = TRUE;
	pCmdUI->Enable(bShow);
}

bool CMainFrame::HasPrevInlineDiff(CBaseView* view)
{
	if (view == 0)
		return false;
	if (!view->IsTarget())
		return false;
	return view->HasPrevInlineDiff();
}

void CMainFrame::OnMoving(UINT fwSide, LPRECT pRect)
{
	// if the pathfilelist dialog is attached to the mainframe,
	// move it along with the mainframe
	if (::IsWindow(m_dlgFilePatches.m_hWnd))
	{
		RECT patchrect;
		m_dlgFilePatches.GetWindowRect(&patchrect);
		if (::IsWindow(m_hWnd))
		{
			RECT thisrect;
			GetWindowRect(&thisrect);
			if (patchrect.right == thisrect.left)
			{
				m_dlgFilePatches.SetWindowPos(nullptr, patchrect.left - (thisrect.left - pRect->left), patchrect.top - (thisrect.top - pRect->top),
					0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
			}
		}
	}
	__super::OnMoving(fwSide, pRect);
}

void CMainFrame::OnUpdateEditCopy(CCmdUI *pCmdUI)
{
	BOOL bShow = FALSE;
	if ((m_pwndBottomView)&&(m_pwndBottomView->HasSelection()))
		bShow = TRUE;
	else if ((m_pwndRightView)&&(m_pwndRightView->HasSelection()))
		bShow = TRUE;
	else if ((m_pwndLeftView)&&(m_pwndLeftView->HasSelection()))
		bShow = TRUE;
	pCmdUI->Enable(bShow);
}

void CMainFrame::OnUpdateEditPaste(CCmdUI *pCmdUI)
{
	BOOL bWritable = FALSE;
	if ((m_pwndBottomView)&&(m_pwndBottomView->IsWritable()))
		bWritable = TRUE;
	else if ((m_pwndRightView)&&(m_pwndRightView->IsWritable()))
		bWritable = TRUE;
	else if ((m_pwndLeftView)&&(m_pwndLeftView->IsWritable()))
		bWritable = TRUE;
	pCmdUI->Enable(bWritable && ::IsClipboardFormatAvailable(CF_TEXT));
}

void CMainFrame::OnViewSwitchleft()
{
	if (CheckForSave(CHFSR_SWITCH)!=IDCANCEL)
	{
		CWorkingFile file = m_Data.m_baseFile;
		m_Data.m_baseFile = m_Data.m_yourFile;
		m_Data.m_yourFile = file;
		if (m_Data.m_mergedFile.GetFilename().CompareNoCase(m_Data.m_yourFile.GetFilename())==0)
		{
			m_Data.m_mergedFile = m_Data.m_baseFile;
		}
		else if (m_Data.m_mergedFile.GetFilename().CompareNoCase(m_Data.m_baseFile.GetFilename())==0)
		{
			m_Data.m_mergedFile = m_Data.m_yourFile;
		}
		LoadViews();
	}
}

void CMainFrame::OnUpdateViewSwitchleft(CCmdUI *pCmdUI)
{
	BOOL bEnable = !IsViewGood(m_pwndBottomView);
	pCmdUI->Enable(bEnable);
}

void CMainFrame::OnUpdateViewShowfilelist(CCmdUI *pCmdUI)
{
	BOOL bEnable = m_dlgFilePatches.HasFiles();
	pCmdUI->Enable(bEnable);
	pCmdUI->SetCheck(m_dlgFilePatches.IsWindowVisible());
}

void CMainFrame::OnViewShowfilelist()
{
	m_dlgFilePatches.ShowWindow(m_dlgFilePatches.IsWindowVisible() ? SW_HIDE : SW_SHOW);
}

void CMainFrame::OnEditUndo()
{
	if (CUndo::GetInstance().CanUndo())
	{
		CUndo::GetInstance().Undo(m_pwndLeftView, m_pwndRightView, m_pwndBottomView);
	}
}

void CMainFrame::OnUpdateEditUndo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(CUndo::GetInstance().CanUndo());
}

void CMainFrame::OnEditRedo()
{
	if (CUndo::GetInstance().CanRedo())
	{
		CUndo::GetInstance().Redo(m_pwndLeftView, m_pwndRightView, m_pwndBottomView);
	}
}

void CMainFrame::OnUpdateEditRedo(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(CUndo::GetInstance().CanRedo());
}

void CMainFrame::OnEditEnable()
{
	CBaseView * pView = GetActiveBaseView();
	if (pView && pView->IsReadonlyChangable())
	{
		bool isReadOnly = pView->IsReadonly();
		pView->SetReadonly(!isReadOnly);
	}
}

void CMainFrame::OnUpdateEditEnable(CCmdUI *pCmdUI)
{
	CBaseView * pView = GetActiveBaseView();
	if (pView)
	{
		pCmdUI->Enable(pView->IsReadonlyChangable() || !pView->IsReadonly());
		pCmdUI->SetCheck(!pView->IsReadonly());
	}
	else
	{
		pCmdUI->Enable(FALSE);
	}
}

void CMainFrame::OnIndicatorLeftview()
{
	if (m_bUseRibbons)
		return;
	if (IsViewGood(m_pwndLeftView))
	{
		m_pwndLeftView->AskUserForNewLineEndingsAndTextType(IDS_STATUSBAR_LEFTVIEW);
	}
}

void CMainFrame::OnIndicatorRightview()
{
	if (m_bUseRibbons)
		return;
	if (IsViewGood(m_pwndRightView))
	{
		m_pwndRightView->AskUserForNewLineEndingsAndTextType(IDS_STATUSBAR_RIGHTVIEW);
	}
}

void CMainFrame::OnIndicatorBottomview()
{
	if (m_bUseRibbons)
		return;
	if (IsViewGood(m_pwndBottomView))
	{
		m_pwndBottomView->AskUserForNewLineEndingsAndTextType(IDS_STATUSBAR_BOTTOMVIEW);
	}
}

int CMainFrame::CheckForReload()
{
	static bool bLock = false; //we don't want to check when activated after MessageBox we just created ... this is simple, but we don't need multithread lock
	if (bLock)
	{
		return IDNO;
	}
	bLock = true;
	bool bSourceChanged =
			m_Data.m_baseFile.HasSourceFileChanged()
			|| m_Data.m_yourFile.HasSourceFileChanged()
			|| m_Data.m_theirFile.HasSourceFileChanged()
			/*|| m_Data.m_mergedFile.HasSourceFileChanged()*/;
	if (!bSourceChanged)
	{
		bLock = false;
		return IDNO;
	}

	CString msg = HasUnsavedEdits() ? CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDELOOSECHANGES)) : CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDE));
	CTaskDialog taskdlg(msg,
						CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDE_TASK2)),
						L"TortoiseGitMerge",
						0,
						TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
	CString sTask3;
	if (HasUnsavedEdits())
		sTask3.LoadString(IDS_WARNMODIFIEDOUTSIDE_TASK3);
	else
		sTask3.LoadString(IDS_WARNMODIFIEDOUTSIDE_TASK4);
	taskdlg.AddCommandControl(IDYES, sTask3);
	taskdlg.AddCommandControl(IDNO, CString(MAKEINTRESOURCE(IDS_WARNMODIFIEDOUTSIDE_TASK5)));
	taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
	taskdlg.SetDefaultCommandControl(IDYES);
	taskdlg.SetMainIcon(TD_WARNING_ICON);
	UINT ret = static_cast<UINT>(taskdlg.DoModal(m_hWnd));
	if (ret == IDYES)
	{
		CDiffColors::GetInstance().LoadRegistry();
		LoadViews(-1);
	}
	else
	{
		if (IsViewGood(m_pwndBottomView)) // three pane view
		{
			/*if (m_Data.m_sourceFile.HasSourceFileChanged())
				m_pwndBottomView->SetModified();
			if (m_Data.m_mergedFile.HasSourceFileChanged())
				m_pwndBottomView->SetModified();//*/
			if (m_Data.m_yourFile.HasSourceFileChanged())
				m_pwndRightView->SetModified();
			if (m_Data.m_theirFile.HasSourceFileChanged())
				m_pwndLeftView->SetModified();
		}
		else if (IsViewGood(m_pwndRightView)) // two pane view
		{
			if (m_Data.m_baseFile.HasSourceFileChanged())
				m_pwndLeftView->SetModified();
			if (m_Data.m_yourFile.HasSourceFileChanged())
				m_pwndRightView->SetModified();
		}
		else
		{
			if (m_Data.m_yourFile.HasSourceFileChanged())
				m_pwndLeftView->SetModified();
		}

		// no reload just store updated file time
		m_Data.m_baseFile.StoreFileAttributes();
		m_Data.m_theirFile.StoreFileAttributes();
		m_Data.m_yourFile.StoreFileAttributes();
		//m_Data.m_mergedFile.StoreFileAttributes();
	}
	bLock = false;
	return ret;
}

int CMainFrame::CheckForSave(ECheckForSaveReason eReason)
{
	int idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
	int idNoSave = IDS_ASKFORSAVE_TASK7;
	int idCancelAction = IDS_ASKFORSAVE_CANCEL_OPEN;
	switch (eReason)
	{
	case CHFSR_CLOSE:
		//idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
		idNoSave = IDS_ASKFORSAVE_TASK4;
		idCancelAction = IDS_ASKFORSAVE_TASK5;
		break;
	case CHFSR_SWITCH:
		//idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
		//idNoSave = IDS_ASKFORSAVE_TASK7;
		idCancelAction = IDS_ASKFORSAVE_TASK8;
		break;
	case CHFSR_RELOAD:
		//idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
		//idNoSave = IDS_ASKFORSAVE_TASK7;
		idCancelAction = IDS_ASKFORSAVE_CANCEL_OPEN;
		break;
	case CHFSR_OPTIONS:
		idTitle = IDS_WARNMODIFIEDLOOSECHANGESOPTIONS;
		//idNoSave = IDS_ASKFORSAVE_TASK7;
		idCancelAction = IDS_ASKFORSAVE_CANCEL_OPTIONS;
		break;
	case CHFSR_OPEN:
		//idTitle = IDS_WARNMODIFIEDLOOSECHANGES;
		idNoSave = IDS_ASKFORSAVE_NOSAVE_OPEN;
		idCancelAction = IDS_ASKFORSAVE_CANCEL_OPEN;
		break;
	}

	CString sTitle(MAKEINTRESOURCE(idTitle));
	CString sSubTitle(MAKEINTRESOURCE(IDS_ASKFORSAVE_TASK2));
	CString sNoSave(MAKEINTRESOURCE(idNoSave));
	CString sCancelAction(MAKEINTRESOURCE(idCancelAction));
	CString sAppName(MAKEINTRESOURCE(IDS_APPNAME));

	// TODO simplify logic, reduce code duplication
	if (CBaseView::IsViewGood(m_pwndBottomView))
	{
		// three-way diff - by design only bottom can be changed
		// use 1.7 way to do that
	}
	else if (CBaseView::IsViewGood(m_pwndRightView))
	{
		// two-way diff -
		// in 1.7 version only right was saved, now left and/or right can be save, so we need to indicate what we are asking to save
		if (HasUnsavedEdits(m_pwndLeftView))
		{
			// both views
			CTaskDialog taskdlg(sTitle,
								sSubTitle,
								sAppName,
								0,
								TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
			CString sTaskTemp;
			if (m_pwndLeftView->m_pWorkingFile->InUse() && !m_pwndLeftView->m_pWorkingFile->IsReadonly())
				sTaskTemp.Format(IDS_ASKFORSAVE_SAVELEFT, static_cast<LPCTSTR>(m_pwndLeftView->m_pWorkingFile->GetFilename()));
			else
				sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVELEFTAS));
			taskdlg.AddCommandControl(201, sTaskTemp); // left
			taskdlg.SetDefaultCommandControl(201);
			if (HasUnsavedEdits(m_pwndRightView))
			{
				if (m_pwndRightView->m_pWorkingFile->InUse() && !m_pwndRightView->m_pWorkingFile->IsReadonly())
					sTaskTemp.Format(IDS_ASKFORSAVE_SAVERIGHT, static_cast<LPCTSTR>(m_pwndRightView->m_pWorkingFile->GetFilename()));
				else
					sTaskTemp = CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVERIGHTAS));
				taskdlg.AddCommandControl(202, sTaskTemp); // right
				taskdlg.AddCommandControl(203, CString(MAKEINTRESOURCE(IDS_ASKFORSAVE_SAVEALL2))); // both
				taskdlg.SetDefaultCommandControl(203);
			}
			taskdlg.AddCommandControl(IDNO, sNoSave); // none
			taskdlg.AddCommandControl(IDCANCEL, sCancelAction); // cancel
			taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
			taskdlg.SetMainIcon(TD_WARNING_ICON);
			UINT ret = static_cast<UINT>(taskdlg.DoModal(m_hWnd));
			switch (ret)
			{
			case 201: // left
				m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
				break;
			case 203: // both
				m_pwndLeftView->SaveFile(SAVE_REMOVEDLINES);
			case 202: // right
				m_pwndRightView->SaveFile();
				break;
			}
			if (ret != IDCANCEL && (eReason == CHFSR_CLOSE || eReason == CHFSR_OPEN))
				DeleteBaseTheirsMineOnClose();
			return ret;
		}
		else
		{
			// only secondary (left) view
		}
		// only right view - 1.7 implementation is used
	}
	else if (CBaseView::IsViewGood(m_pwndLeftView))
	{
		// only one view - only one to save
		// 1.7 FileSave don't support this mode
		if (HasUnsavedEdits(m_pwndLeftView))
		{
			CTaskDialog taskdlg(sTitle,
								sSubTitle,
								sAppName,
								0,
								TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
			CString sTask3;
			if (m_Data.m_mergedFile.InUse())
				sTask3.Format(IDS_ASKFORSAVE_TASK3, static_cast<LPCTSTR>(m_Data.m_mergedFile.GetFilename()));
			else
				sTask3.LoadString(IDS_ASKFORSAVE_TASK6);
			taskdlg.AddCommandControl(IDYES, sTask3);
			taskdlg.AddCommandControl(IDNO, sNoSave);
			taskdlg.AddCommandControl(IDCANCEL, sCancelAction);
			taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
			taskdlg.SetDefaultCommandControl(IDYES);
			taskdlg.SetMainIcon(TD_WARNING_ICON);
			if (static_cast<UINT>(taskdlg.DoModal(m_hWnd)) == IDYES)
			{
				if (m_pwndLeftView->SaveFile()<0)
					return IDCANCEL;
			}
		}
		if (eReason == CHFSR_CLOSE || eReason == CHFSR_OPEN)
			DeleteBaseTheirsMineOnClose();
		return IDNO;
	}
	else
	{
		if (eReason == CHFSR_CLOSE || eReason == CHFSR_OPEN)
			DeleteBaseTheirsMineOnClose();
		return IDNO; // nothing to save
	}

	// 1.7 implementation
	UINT ret = IDNO;
	if (HasUnsavedEdits())
	{
		CTaskDialog taskdlg(sTitle,
							sSubTitle,
							sAppName,
							0,
							TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
		CString sTask3;
		if (m_Data.m_mergedFile.InUse())
			sTask3.Format(IDS_ASKFORSAVE_TASK3, static_cast<LPCTSTR>(m_Data.m_mergedFile.GetFilename()));
		else
			sTask3.LoadString(IDS_ASKFORSAVE_TASK6);
		taskdlg.AddCommandControl(IDYES, sTask3);
		taskdlg.AddCommandControl(IDNO, sNoSave);
		taskdlg.AddCommandControl(IDCANCEL, sCancelAction);
		taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
		taskdlg.SetDefaultCommandControl(IDYES);
		taskdlg.SetMainIcon(TD_WARNING_ICON);
		ret = static_cast<UINT>(taskdlg.DoModal(m_hWnd));

		if (ret == IDYES)
		{
			if (!FileSave())
				ret = IDCANCEL;
		}
	}

	if (ret != IDCANCEL && (eReason == CHFSR_CLOSE || eReason == CHFSR_OPEN))
		DeleteBaseTheirsMineOnClose();

	return ret;
}

void CMainFrame::DeleteBaseTheirsMineOnClose()
{
	if (!m_bDeleteBaseTheirsMineOnClose)
		return;

	m_bDeleteBaseTheirsMineOnClose = false;

	DeleteFile(m_Data.m_baseFile.GetFilename());
	DeleteFile(m_Data.m_theirFile.GetFilename());
	DeleteFile(m_Data.m_yourFile.GetFilename());
}

bool CMainFrame::HasUnsavedEdits() const
{
	return HasUnsavedEdits(m_pwndBottomView) || HasUnsavedEdits(m_pwndRightView) || m_bSaveRequired;
}

bool CMainFrame::HasUnsavedEdits(const CBaseView* view)
{
	if (!CBaseView::IsViewGood(view))
		return false;
	return view->IsModified();
}

bool CMainFrame::HasMarkedBlocks() const
{
	return CBaseView::IsViewGood(m_pwndRightView) && m_pwndRightView->HasMarkedBlocks();
}

bool CMainFrame::IsViewGood(const CBaseView* view)
{
	return CBaseView::IsViewGood(view);
}

void CMainFrame::OnViewInlinediffword()
{
	m_bInlineWordDiff = !m_bInlineWordDiff;
	if (m_pwndLeftView)
	{
		m_pwndLeftView->SetInlineWordDiff(m_bInlineWordDiff);
		m_pwndLeftView->BuildAllScreen2ViewVector();
		m_pwndLeftView->DocumentUpdated();
	}
	if (m_pwndRightView)
	{
		m_pwndRightView->SetInlineWordDiff(m_bInlineWordDiff);
		m_pwndRightView->BuildAllScreen2ViewVector();
		m_pwndRightView->DocumentUpdated();
	}
	if (m_pwndBottomView)
	{
		m_pwndBottomView->SetInlineWordDiff(m_bInlineWordDiff);
		m_pwndBottomView->BuildAllScreen2ViewVector();
		m_pwndBottomView->DocumentUpdated();
	}
	m_wndLineDiffBar.DocumentUpdated();
}

void CMainFrame::OnUpdateViewInlinediffword(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bInlineDiff && IsViewGood(m_pwndLeftView) && IsViewGood(m_pwndRightView));
	pCmdUI->SetCheck(m_bInlineWordDiff);
}

void CMainFrame::OnViewInlinediff()
{
	m_bInlineDiff = !m_bInlineDiff;
	if (m_pwndLeftView)
	{
		m_pwndLeftView->SetInlineDiff(m_bInlineDiff);
		m_pwndLeftView->BuildAllScreen2ViewVector();
		m_pwndLeftView->DocumentUpdated();
	}
	if (m_pwndRightView)
	{
		m_pwndRightView->SetInlineDiff(m_bInlineDiff);
		m_pwndRightView->BuildAllScreen2ViewVector();
		m_pwndRightView->DocumentUpdated();
	}
	if (m_pwndBottomView)
	{
		m_pwndBottomView->SetInlineDiff(m_bInlineDiff);
		m_pwndBottomView->BuildAllScreen2ViewVector();
		m_pwndBottomView->DocumentUpdated();
	}
	m_wndLineDiffBar.DocumentUpdated();
}

void CMainFrame::OnUpdateViewInlinediff(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(IsViewGood(m_pwndLeftView) && IsViewGood(m_pwndRightView));
	pCmdUI->SetCheck(m_bInlineDiff);
}

void CMainFrame::OnUpdateEditCreateunifieddifffile(CCmdUI *pCmdUI)
{
	// "create unified diff file" is only available if two files
	// are diffed, not three.
	bool bEnabled = true;
	if (!IsViewGood(m_pwndLeftView))
		bEnabled = false;
	else if (!IsViewGood(m_pwndRightView))
		bEnabled = false;
	else if (IsViewGood(m_pwndBottomView)) //no negation here
		bEnabled = false;
	pCmdUI->Enable(bEnabled);
}

void CMainFrame::OnEditCreateunifieddifffile()
{
	CString origFile, modifiedFile;
	CString origReflected, modifiedReflected;
	// the original file is the one on the left
	if (m_pwndLeftView)
	{
		origFile = m_pwndLeftView->m_sFullFilePath;
		origReflected = m_pwndLeftView->m_sReflectedName;
	}
	if (m_pwndRightView)
	{
		modifiedFile = m_pwndRightView->m_sFullFilePath;
		modifiedReflected = m_pwndRightView->m_sReflectedName;
	}
	if (origFile.IsEmpty() || modifiedFile.IsEmpty())
		return;

	CString outputFile;
	if(!TryGetFileName(outputFile))
		return;

	CRegStdDWORD regContextLines(L"Software\\TortoiseGitMerge\\ContextLines", static_cast<DWORD>(-1));
	CAppUtils::CreateUnifiedDiff(origFile, modifiedFile, outputFile, regContextLines, true);
	// from here a hacky solution exchanges the paths included in the patch, see issue #2541
	if (origReflected.IsEmpty() || modifiedReflected.IsEmpty())
		return;
	CString projectDir1, projectDir2;
	if (!GitAdminDir::HasAdminDir(origReflected, &projectDir1) || !GitAdminDir::HasAdminDir(modifiedReflected, &projectDir2) || projectDir1 != projectDir2)
		return;
	CStringA origReflectedA = CUnicodeUtils::GetUTF8(origReflected.Mid(projectDir1.GetLength() + 1));
	CStringA modifiedReflectedA = CUnicodeUtils::GetUTF8(modifiedReflected.Mid(projectDir1.GetLength() + 1));
	origReflectedA.Replace('\\', '/');
	modifiedReflectedA.Replace('\\', '/');
	try
	{
		CStdioFile file;
		// w/o typeBinary for some files \r gets dropped
		if (!file.Open(outputFile, CFile::typeBinary | CFile::modeReadWrite | CFile::shareExclusive))
			return;

		CStringA filecontent;
		UINT filelength = static_cast<UINT>(file.GetLength());
		int bytesread = static_cast<int>(file.Read(filecontent.GetBuffer(filelength), filelength));
		filecontent.ReleaseBuffer(bytesread);

		if (!CStringUtils::StartsWith(filecontent, "diff --git "))
			return;
		int lineend = filecontent.Find("\n");
		if (lineend <= static_cast<int>(strlen("diff --git ")))
			return;
		CStringA newStart = "diff --git \"a/" + origReflectedA + "\" \"b/" + modifiedReflectedA + "\"\n";
		if (!CStringUtils::StartsWith(filecontent.GetBuffer() + lineend, "\nindex "))
			return;
		int nextlineend = filecontent.Find("\n", lineend + 1);
		if (nextlineend <= lineend)
			return;
		newStart += filecontent.Mid(lineend + 1, nextlineend - lineend);
		lineend = filecontent.Find("\n--- ", nextlineend);
		nextlineend = filecontent.Find("\n@@ ", lineend);
		if (nextlineend <= lineend)
			return;
		newStart += "--- \"a/" + origReflectedA + "\"\n+++ \"b/" + modifiedReflectedA + "\"";
		filecontent = newStart + filecontent.Mid(nextlineend);
		file.SeekToBegin();
		file.Write(filecontent, static_cast<UINT>(filecontent.GetLength()));
		file.SetLength(file.GetPosition());
		file.Close();
	}
	catch (CFileException*)
	{
	}
}

void CMainFrame::OnUpdateViewLinediffbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bLineDiff);
	pCmdUI->Enable();
}

void CMainFrame::OnViewLinediffbar()
{
	m_bLineDiff = !m_bLineDiff;
	m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
	m_wndLineDiffBar.DocumentUpdated();
	m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
	m_wndLocatorBar.DocumentUpdated();
}

void CMainFrame::OnUpdateViewLocatorbar(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bLocatorBar);
	pCmdUI->Enable();
}

void CMainFrame::OnUpdateViewBars(CCmdUI * pCmdUI)
{
	pCmdUI->Enable();
}

void CMainFrame::OnViewLocatorbar()
{
	m_bLocatorBar = !m_bLocatorBar;
	m_wndLocatorBar.ShowPane(m_bLocatorBar, false, true);
	m_wndLocatorBar.DocumentUpdated();
	m_wndLineDiffBar.ShowPane(m_bLineDiff, false, true);
	m_wndLineDiffBar.DocumentUpdated();
}

void CMainFrame::OnViewComparewhitespaces()
{
	if (CheckForSave(CHFSR_OPTIONS)==IDCANCEL)
		return;
	CRegDWORD regIgnoreWS(L"Software\\TortoiseGitMerge\\IgnoreWS");
	regIgnoreWS = 0;
	LoadViews(-1);
}

void CMainFrame::OnUpdateViewComparewhitespaces(CCmdUI *pCmdUI)
{
	CRegDWORD regIgnoreWS(L"Software\\TortoiseGitMerge\\IgnoreWS");
	DWORD dwIgnoreWS = regIgnoreWS;
	pCmdUI->SetCheck(dwIgnoreWS == 0);
}

void CMainFrame::OnViewIgnorewhitespacechanges()
{
	if (CheckForSave(CHFSR_OPTIONS)==IDCANCEL)
		return;
	CRegDWORD regIgnoreWS(L"Software\\TortoiseGitMerge\\IgnoreWS");
	regIgnoreWS = 2;
	LoadViews(-1);
}

void CMainFrame::OnUpdateViewIgnorewhitespacechanges(CCmdUI *pCmdUI)
{
	CRegDWORD regIgnoreWS(L"Software\\TortoiseGitMerge\\IgnoreWS");
	DWORD dwIgnoreWS = regIgnoreWS;
	pCmdUI->SetCheck(dwIgnoreWS == 2);
}

void CMainFrame::OnViewIgnoreallwhitespacechanges()
{
	if (CheckForSave(CHFSR_OPTIONS)==IDCANCEL)
		return;
	CRegDWORD regIgnoreWS(L"Software\\TortoiseGitMerge\\IgnoreWS");
	regIgnoreWS = 1;
	LoadViews(-1);
}

void CMainFrame::OnUpdateViewIgnoreallwhitespacechanges(CCmdUI *pCmdUI)
{
	CRegDWORD regIgnoreWS(L"Software\\TortoiseGitMerge\\IgnoreWS");
	DWORD dwIgnoreWS = regIgnoreWS;
	pCmdUI->SetCheck(dwIgnoreWS == 1);
}

void CMainFrame::OnViewMovedBlocks()
{
	m_bViewMovedBlocks = !static_cast<DWORD>(m_regViewModedBlocks);
	m_regViewModedBlocks = m_bViewMovedBlocks;
	LoadViews(-1);
}

void CMainFrame::OnUpdateViewMovedBlocks(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bViewMovedBlocks);
	BOOL bEnable = TRUE;
	if (IsViewGood(m_pwndBottomView))
	{
		bEnable = FALSE;
	}
	pCmdUI->Enable(bEnable);
}

bool CMainFrame::HasConflictsWontKeep()
{
	const int nConflictLine = CheckResolved();
	if (nConflictLine < 0)
		return false;
	if (!m_pwndBottomView)
		return false;

	CString sTemp;
	sTemp.Format(IDS_ERR_MAINFRAME_FILEHASCONFLICTS, m_pwndBottomView->m_pViewData->GetLineNumber(nConflictLine)+1);
	CTaskDialog taskdlg(sTemp,
						CString(MAKEINTRESOURCE(IDS_ERR_MAINFRAME_FILEHASCONFLICTS_TASK2)),
						L"TortoiseGitMerge",
						0,
						TDF_ENABLE_HYPERLINKS | TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW);
	taskdlg.AddCommandControl(1, CString(MAKEINTRESOURCE(IDS_ERR_MAINFRAME_FILEHASCONFLICTS_TASK3)));
	taskdlg.AddCommandControl(2, CString(MAKEINTRESOURCE(IDS_ERR_MAINFRAME_FILEHASCONFLICTS_TASK4)));
	taskdlg.SetCommonButtons(TDCBF_CANCEL_BUTTON);
	taskdlg.SetDefaultCommandControl(2);
	taskdlg.SetMainIcon(TD_ERROR_ICON);
	if (taskdlg.DoModal(m_hWnd) == 1)
		return false;

	m_pwndBottomView->GoToLine(nConflictLine);
	return true;
}

bool CMainFrame::TryGetFileName(CString& result)
{
	return CCommonAppUtils::FileOpenSave(result, nullptr, IDS_SAVEASTITLE, IDS_COMMONFILEFILTER, false, m_hWnd);
}

CBaseView* CMainFrame::GetActiveBaseView() const
{
	CView* activeView = GetActiveView();
	CBaseView* activeBase = dynamic_cast<CBaseView*>( activeView );
	return activeBase;
}

void CMainFrame::SetWindowTitle()
{
	// try to find a suitable window title
	CString sYour = m_Data.m_yourFile.GetDescriptiveName();
	if (sYour.Find(L" - ") >= 0)
		sYour = sYour.Left(sYour.Find(L" - "));
	if (sYour.Find(L" : ") >= 0)
		sYour = sYour.Left(sYour.Find(L" : "));
	CString sTheir = m_Data.m_theirFile.GetDescriptiveName();
	if (sTheir.IsEmpty())
		sTheir = m_Data.m_baseFile.GetDescriptiveName();
	if (sTheir.Find(L" - ") >= 0)
		sTheir = sTheir.Left(sTheir.Find(L" - "));
	if (sTheir.Find(L" : ") >= 0)
		sTheir = sTheir.Left(sTheir.Find(L" : "));

	if (!sYour.IsEmpty() && !sTheir.IsEmpty())
	{
		if (sYour.CompareNoCase(sTheir)==0)
			SetWindowText(sYour + L" - TortoiseGitMerge");
		else if ((sYour.GetLength() < 10) &&
			(sTheir.GetLength() < 10))
			SetWindowText(sYour + L" - " + sTheir + L" - TortoiseGitMerge");
		else
		{
			// we have two very long descriptive texts here, which
			// means we have to find a way to use them as a window
			// title in a shorter way.
			// for simplicity, we just use the one from "yourfile"
			SetWindowText(sYour + L" - TortoiseGitMerge");
		}
	}
	else if (!sYour.IsEmpty())
		SetWindowText(sYour + L" - TortoiseGitMerge");
	else if (!sTheir.IsEmpty())
		SetWindowText(sTheir + L" - TortoiseGitMerge");
	else
		SetWindowText(L"TortoiseGitMerge");
}

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case IDT_RELOADCHECKTIMER:
		KillTimer(nIDEvent);
		CheckForReload();
		break;
	}

	__super::OnTimer(nIDEvent);
}

void CMainFrame::LoadIgnoreCommentData()
{
	static bool bLoaded = false;
	if (bLoaded)
		return;
	CString sPath = CPathUtils::GetAppDataDirectory() + L"ignorecomments.txt";
	if (!PathFileExists(sPath))
	{
		// ignore comments file does not exist (yet), so create a default one
		HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDR_IGNORECOMMENTSTXT), L"config");
		if (hRes)
		{
			HGLOBAL hResourceLoaded = LoadResource(nullptr, hRes);
			if (hResourceLoaded)
			{
				auto lpResLock = static_cast<char*>(LockResource(hResourceLoaded));
				DWORD dwSizeRes = SizeofResource(nullptr, hRes);
				if (lpResLock)
				{
					HANDLE hFile = CreateFile(sPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
					if (hFile != INVALID_HANDLE_VALUE)
					{
						DWORD dwWritten = 0;
						WriteFile(hFile, lpResLock, dwSizeRes, &dwWritten, nullptr);
						CloseHandle(hFile);
					}
				}
			}
		}
	}

	try
	{
		CStdioFile file;
		if (file.Open(sPath, CFile::modeRead))
		{
			CString sLine;
			while (file.ReadString(sLine))
			{
				int eqpos = sLine.Find('=');
				if (eqpos >= 0)
				{
					CString sExts = sLine.Left(eqpos);
					CString sComments = sLine.Mid(eqpos+1);

					int pos = sComments.Find(',');
					CString sLineStart = sComments.Left(pos);
					pos = sComments.Find(',', pos);
					int pos2 = sComments.Find(',', pos+1);
					CString sBlockStart = sComments.Mid(pos+1, pos2-pos-1);
					CString sBlockEnd = sComments.Mid(pos2+1);

					auto commentTuple = std::make_tuple(sLineStart, sBlockStart, sBlockEnd);

					pos = 0;
					CString temp;
					for (;;)
					{
						temp = sExts.Tokenize(L",", pos);
						if (temp.IsEmpty())
						{
							break;
						}
						ASSERT(m_IgnoreCommentsMap.find(temp) == m_IgnoreCommentsMap.end());
						m_IgnoreCommentsMap[temp] = commentTuple;
					}
				}
			}
		}
	}
	catch (CFileException* e)
	{
		e->Delete();
	}
	bLoaded = true;
}

void CMainFrame::OnViewIgnorecomments()
{
	if (CheckForSave(CHFSR_OPTIONS)==IDCANCEL)
		return;
	m_regIgnoreComments = !DWORD(m_regIgnoreComments);
	LoadViews(-1);
}

void CMainFrame::OnUpdateViewIgnorecomments(CCmdUI *pCmdUI)
{
	// only enable if we have comments defined for this file extension
	CString sExt = CPathUtils::GetFileExtFromPath(m_Data.m_baseFile.GetFilename()).MakeLower();
	sExt.TrimLeft(L".");
	auto sC = m_IgnoreCommentsMap.find(sExt);
	if (sC == m_IgnoreCommentsMap.end())
	{
		sExt = CPathUtils::GetFileExtFromPath(m_Data.m_yourFile.GetFilename()).MakeLower();
		sExt.TrimLeft(L".");
		sC = m_IgnoreCommentsMap.find(sExt);
		if (sC == m_IgnoreCommentsMap.end())
		{
			sExt = CPathUtils::GetFileExtFromPath(m_Data.m_theirFile.GetFilename()).MakeLower();
			sExt.TrimLeft(L".");
			sC = m_IgnoreCommentsMap.find(sExt);
		}
	}
	pCmdUI->Enable(sC != m_IgnoreCommentsMap.end());

	pCmdUI->SetCheck(DWORD(m_regIgnoreComments) != 0);
}


void CMainFrame::OnRegexfilter(UINT cmd)
{
	if ((cmd == ID_REGEXFILTER)||(cmd == (ID_REGEXFILTER+1)))
	{
		CRegexFiltersDlg dlg(this);
		dlg.SetIniFile(&m_regexIni);
		if (dlg.DoModal() == IDOK)
		{
			FILE* pFile = nullptr;
			_wfopen_s(&pFile, CPathUtils::GetAppDataDirectory() + L"regexfilters.ini", L"wb");
			m_regexIni.SaveFile(pFile);
			fclose(pFile);
		}
		BuildRegexSubitems();
	}
	else
	{
		if (cmd == static_cast<UINT>(m_regexIndex) && !m_bUseRibbons)
		{
			if (CheckForSave(CHFSR_OPTIONS)==IDCANCEL)
				return;
			m_Data.SetRegexTokens(std::wregex(), L"");
			m_regexIndex = -1;
			LoadViews(-1);
		}
		else if (cmd != static_cast<UINT>(m_regexIndex))
		{
			CSimpleIni::TNamesDepend sections;
			m_regexIni.GetAllSections(sections);
			int index = ID_REGEXFILTER + 2;
			m_regexIndex = -1;
			for (const auto& section : sections)
			{
				if (cmd == static_cast<UINT>(index))
				{
					if (CheckForSave(CHFSR_OPTIONS)==IDCANCEL)
						break;
					try
					{
						std::wregex rx(m_regexIni.GetValue(section.pItem, L"regex", L""));
						m_Data.SetRegexTokens(rx, m_regexIni.GetValue(section.pItem, L"replace", L""));
					}
					catch (std::exception &ex)
					{
						MessageBox(L"Regex is invalid!\r\n" + CString(ex.what()));
					}
					m_regexIndex = index;
					try
					{
						LoadViews(-1);
					}
					catch (const std::regex_error& ex)
					{
						MessageBox(L"Regexp error caught:\r\n" + CString(ex.what()) + L"\r\nTrying to recover by unsetting it again.");
						m_Data.SetRegexTokens(std::wregex(), L"");
						m_regexIndex = -1;
						LoadViews(-1);
					}
					break;
				}
				++index;
			}
		}
	}
}

void CMainFrame::OnUpdateViewRegexFilter( CCmdUI *pCmdUI )
{
	pCmdUI->Enable();
	pCmdUI->SetCheck(pCmdUI->m_nID == static_cast<UINT>(m_regexIndex));
}

void CMainFrame::BuildRegexSubitems(CMFCPopupMenu* pMenuPopup)
{
	CString sIniPath = CPathUtils::GetAppDataDirectory() + L"regexfilters.ini";
	if (!PathFileExists(sIniPath))
	{
		// ini file does not exist (yet), so create a default one
		HRSRC hRes = FindResource(nullptr, MAKEINTRESOURCE(IDR_REGEXFILTERINI), L"config");
		if (hRes)
		{
			HGLOBAL hResourceLoaded = LoadResource(nullptr, hRes);
			if (hResourceLoaded)
			{
				auto lpResLock = static_cast<char*>(LockResource(hResourceLoaded));
				DWORD dwSizeRes = SizeofResource(nullptr, hRes);
				if (lpResLock)
				{
					HANDLE hFile = CreateFile(sIniPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
					if (hFile != INVALID_HANDLE_VALUE)
					{
						DWORD dwWritten = 0;
						WriteFile(hFile, lpResLock, dwSizeRes, &dwWritten, nullptr);
						CloseHandle(hFile);
					}
				}
			}
		}
	}

	m_regexIni.LoadFile(sIniPath);
	CSimpleIni::TNamesDepend sections;
	m_regexIni.GetAllSections(sections);

	if (m_bUseRibbons)
	{
		std::list<CNativeRibbonDynamicItemInfo> items;
		int cmdIndex = 2;
		items.push_back(CNativeRibbonDynamicItemInfo(ID_REGEX_NO_FILTER, CString(MAKEINTRESOURCE(ID_REGEX_NO_FILTER)), IDB_REGEX_FILTER));
		for (const auto& section : sections)
		{
			items.emplace_back(ID_REGEXFILTER + cmdIndex, section.pItem, IDB_REGEX_FILTER);
			cmdIndex++;
		}

		m_pRibbonApp->SetItems(ID_REGEXFILTER, items);
	}
	else if (pMenuPopup)
	{
		int iIndex = -1;
		if (!CMFCToolBar::IsCustomizeMode() &&
			(iIndex = pMenuPopup->GetMenuBar()->CommandToIndex(ID_REGEXFILTER)) >= 0)
		{
			if (!sections.empty())
				pMenuPopup->InsertSeparator(iIndex + 1); // insert the separator at the end
			int cmdIndex = 2;
			for (const auto& section : sections)
			{
				pMenuPopup->InsertItem(CMFCToolBarMenuButton(ID_REGEXFILTER + cmdIndex, nullptr, -1, section.pItem), iIndex + cmdIndex);
				cmdIndex++;
			}
		}
	}
}

void CMainFrame::FillEncodingButton( CMFCRibbonButton * pButton, int start )
{
	pButton->SetDefaultCommand(FALSE);
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::ASCII,       L"ASCII"       ));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::BINARY,      L"BINARY"      ));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_LE,    L"UTF-16LE"    ));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_LEBOM, L"UTF-16LE BOM"));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_BE,    L"UTF-16BE"    ));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF16_BEBOM, L"UTF-16BE BOM"));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF32_LE,    L"UTF-32LE"    ));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF32_BE,    L"UTF-32BE"    ));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF8,        L"UTF-8"       ));
	pButton->AddSubItem(new CMFCRibbonButton(start + CFileTextLines::UnicodeType::UTF8BOM,     L"UTF-8 BOM"   ));
}

void CMainFrame::FillEOLButton( CMFCRibbonButton * pButton, int start )
{
	pButton->SetDefaultCommand(FALSE);
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_LF  , L"LF"  ));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_CRLF, L"CRLF"));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_LFCR, L"LRCR"));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_CR  , L"CR"  ));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_VT  , L"VT"  ));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_FF  , L"FF"  ));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_NEL , L"NEL" ));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_LS  , L"LS"  ));
	pButton->AddSubItem(new CMFCRibbonButton(start + EOL::EOL_PS  , L"PS"  ));
}

void CMainFrame::FillTabModeButton(CMFCRibbonButton * pButton, int start)
{
	pButton->SetDefaultCommand(FALSE);
	pButton->AddSubItem(new CMFCRibbonButton(start + TABMODE_NONE        , L"Tab"));
	pButton->AddSubItem(new CMFCRibbonButton(start + TABMODE_USESPACES   , L"Space"));
	pButton->AddSubItem(new CMFCRibbonSeparator(TRUE));
	pButton->AddSubItem(new CMFCRibbonButton(start + TABMODE_SMARTINDENT , L"Smart tab char"));
	pButton->AddSubItem(new CMFCRibbonSeparator(TRUE));
	pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON1, L"1"));
	pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON2, L"2"));
	pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON4, L"4"));
	pButton->AddSubItem(new CMFCRibbonButton(start + TABSIZEBUTTON8, L"8"));
	pButton->AddSubItem(new CMFCRibbonSeparator(TRUE));
	pButton->AddSubItem(new CMFCRibbonButton(start + ENABLEEDITORCONFIG, L"EditorConfig"));
}

bool CMainFrame::AdjustUnicodeTypeForLoad(CFileTextLines::UnicodeType& type)
{
	switch (type)
	{
		case CFileTextLines::UnicodeType::AUTOTYPE:
		case CFileTextLines::UnicodeType::BINARY:
		return false;

		case CFileTextLines::UnicodeType::ASCII:
		case CFileTextLines::UnicodeType::UTF16_LE:
		case CFileTextLines::UnicodeType::UTF16_BE:
		case CFileTextLines::UnicodeType::UTF32_LE:
		case CFileTextLines::UnicodeType::UTF32_BE:
		case CFileTextLines::UnicodeType::UTF8:
		return true;

		case CFileTextLines::UnicodeType::UTF16_LEBOM:
		type = CFileTextLines::UnicodeType::UTF16_LE;
		return true;

		case CFileTextLines::UnicodeType::UTF16_BEBOM:
		type = CFileTextLines::UnicodeType::UTF16_BE;
		return true;

		case CFileTextLines::UnicodeType::UTF8BOM:
		type = CFileTextLines::UnicodeType::UTF8;
		return true;
	}
	return false;
}

void CMainFrame::OnEncodingLeft( UINT cmd )
{
	if (m_pwndLeftView)
	{
		if (GetKeyState(VK_CONTROL) & 0x8000)
		{
			// reload with selected encoding
			auto saveparams = m_Data.m_arBaseFile.GetSaveParams();
			saveparams.m_UnicodeType = CFileTextLines::UnicodeType(cmd - ID_INDICATOR_LEFTENCODINGSTART);
			if (AdjustUnicodeTypeForLoad(saveparams.m_UnicodeType))
			{
				m_Data.m_arBaseFile.SetSaveParams(saveparams);
				m_Data.m_arBaseFile.KeepEncoding();
				LoadViews();
			}
		}
		else
		{
			m_pwndLeftView->SetTextType(CFileTextLines::UnicodeType(cmd - ID_INDICATOR_LEFTENCODINGSTART));
			m_pwndLeftView->RefreshViews();
		}
	}
}

void CMainFrame::OnEncodingRight( UINT cmd )
{
	if (m_pwndRightView)
	{
		if (GetKeyState(VK_CONTROL) & 0x8000)
		{
			// reload with selected encoding
			auto saveparams = m_Data.m_arYourFile.GetSaveParams();
			saveparams.m_UnicodeType = CFileTextLines::UnicodeType(cmd - ID_INDICATOR_RIGHTENCODINGSTART);
			if (AdjustUnicodeTypeForLoad(saveparams.m_UnicodeType))
			{
				m_Data.m_arYourFile.SetSaveParams(saveparams);
				m_Data.m_arYourFile.KeepEncoding();
				LoadViews();
			}
		}
		else
		{
			m_pwndRightView->SetTextType(CFileTextLines::UnicodeType(cmd - ID_INDICATOR_RIGHTENCODINGSTART));
			m_pwndRightView->RefreshViews();
		}
	}
}

void CMainFrame::OnEncodingBottom( UINT cmd )
{
	if (m_pwndBottomView)
	{
		if (GetKeyState(VK_CONTROL) & 0x8000)
		{
			// reload with selected encoding
			auto saveparams = m_Data.m_arTheirFile.GetSaveParams();
			saveparams.m_UnicodeType = CFileTextLines::UnicodeType(cmd - ID_INDICATOR_BOTTOMENCODINGSTART);
			if (AdjustUnicodeTypeForLoad(saveparams.m_UnicodeType))
			{
				m_Data.m_arTheirFile.SetSaveParams(saveparams);
				m_Data.m_arTheirFile.KeepEncoding();
				LoadViews();
			}
		}
		else
		{
			m_pwndBottomView->SetTextType(CFileTextLines::UnicodeType(cmd - ID_INDICATOR_BOTTOMENCODINGSTART));
			m_pwndBottomView->RefreshViews();
		}
	}
}

void CMainFrame::OnEOLLeft( UINT cmd )
{
	if (m_pwndLeftView)
	{
		m_pwndLeftView->ReplaceLineEndings(EOL(cmd-ID_INDICATOR_LEFTEOLSTART));
		m_pwndLeftView->RefreshViews();
	}
}

void CMainFrame::OnEOLRight( UINT cmd )
{
	if (m_pwndRightView)
	{
		m_pwndRightView->ReplaceLineEndings(EOL(cmd-ID_INDICATOR_RIGHTEOLSTART));
		m_pwndRightView->RefreshViews();
	}
}

void CMainFrame::OnEOLBottom( UINT cmd )
{
	if (m_pwndBottomView)
	{
		m_pwndBottomView->ReplaceLineEndings(EOL(cmd-ID_INDICATOR_BOTTOMEOLSTART));
		m_pwndBottomView->RefreshViews();
	}
}

void CMainFrame::OnTabModeLeft( UINT cmd )
{
	OnTabMode(m_pwndLeftView, static_cast<int>(cmd) - ID_INDICATOR_LEFTTABMODESTART);
}

void CMainFrame::OnTabModeRight( UINT cmd )
{
	OnTabMode(m_pwndRightView, static_cast<int>(cmd) - ID_INDICATOR_RIGHTTABMODESTART);
}

void CMainFrame::OnTabModeBottom( UINT cmd )
{
	OnTabMode(m_pwndBottomView, static_cast<int>(cmd) - ID_INDICATOR_BOTTOMTABMODESTART);
}

void CMainFrame::OnTabMode(CBaseView *view, int cmd)
{
	if (!view)
		return;
	int nTabMode = view->GetTabMode();
	if (cmd == TABMODE_NONE || cmd == TABMODE_USESPACES)
		view->SetTabMode((nTabMode & (~TABMODE_USESPACES)) | (cmd & TABMODE_USESPACES));
	else if (cmd == TABMODE_SMARTINDENT) // Toggle
		view->SetTabMode((nTabMode & (~TABMODE_SMARTINDENT)) | ((nTabMode & TABMODE_SMARTINDENT) ? 0 : TABMODE_SMARTINDENT));
	else if (cmd == TABSIZEBUTTON1)
		view->SetTabSize(1);
	else if (cmd == TABSIZEBUTTON2)
		view->SetTabSize(2);
	else if (cmd == TABSIZEBUTTON4)
		view->SetTabSize(4);
	else if (cmd == TABSIZEBUTTON8)
		view->SetTabSize(8);
	else if (cmd == ENABLEEDITORCONFIG)
		view->SetEditorConfigEnabled(!view->GetEditorConfigEnabled());
	view->RefreshViews();
}

void CMainFrame::OnUpdateEncodingLeft( CCmdUI *pCmdUI )
{
	if (m_pwndLeftView)
	{
		pCmdUI->SetCheck(CFileTextLines::UnicodeType(pCmdUI->m_nID - ID_INDICATOR_LEFTENCODINGSTART) == m_pwndLeftView->GetTextType());
		pCmdUI->Enable(m_pwndLeftView->IsWritable() || (GetKeyState(VK_CONTROL)&0x8000));
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateEncodingRight( CCmdUI *pCmdUI )
{
	if (m_pwndRightView)
	{
		pCmdUI->SetCheck(CFileTextLines::UnicodeType(pCmdUI->m_nID - ID_INDICATOR_RIGHTENCODINGSTART) == m_pwndRightView->GetTextType());
		pCmdUI->Enable(m_pwndRightView->IsWritable() || (GetKeyState(VK_CONTROL) & 0x8000));
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateEncodingBottom( CCmdUI *pCmdUI )
{
	if (m_pwndBottomView)
	{
		pCmdUI->SetCheck(CFileTextLines::UnicodeType(pCmdUI->m_nID - ID_INDICATOR_BOTTOMENCODINGSTART) == m_pwndBottomView->GetTextType());
		pCmdUI->Enable(m_pwndBottomView->IsWritable() || (GetKeyState(VK_CONTROL) & 0x8000));
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateEOLLeft( CCmdUI *pCmdUI )
{
	if (m_pwndLeftView)
	{
		pCmdUI->SetCheck(EOL(pCmdUI->m_nID - ID_INDICATOR_LEFTEOLSTART) == m_pwndLeftView->GetLineEndings());
		pCmdUI->Enable(m_pwndLeftView->IsWritable());
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateEOLRight( CCmdUI *pCmdUI )
{
	if (m_pwndRightView)
	{
		pCmdUI->SetCheck(EOL(pCmdUI->m_nID - ID_INDICATOR_RIGHTEOLSTART) == m_pwndRightView->GetLineEndings());
		pCmdUI->Enable(m_pwndRightView->IsWritable());
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateEOLBottom( CCmdUI *pCmdUI )
{
	if (m_pwndBottomView)
	{
		pCmdUI->SetCheck(EOL(pCmdUI->m_nID - ID_INDICATOR_BOTTOMEOLSTART) == m_pwndBottomView->GetLineEndings());
		pCmdUI->Enable(m_pwndBottomView->IsWritable());
	}
	else
		pCmdUI->Enable(FALSE);
}

void CMainFrame::OnUpdateTabModeLeft(CCmdUI *pCmdUI)
{
	OnUpdateTabMode(m_pwndLeftView, pCmdUI, ID_INDICATOR_LEFTTABMODESTART);
}

void CMainFrame::OnUpdateTabModeRight(CCmdUI *pCmdUI)
{
	OnUpdateTabMode(m_pwndRightView, pCmdUI, ID_INDICATOR_RIGHTTABMODESTART);
}

void CMainFrame::OnUpdateTabModeBottom(CCmdUI *pCmdUI)
{
	OnUpdateTabMode(m_pwndBottomView, pCmdUI, ID_INDICATOR_BOTTOMTABMODESTART);
}

void CMainFrame::OnUpdateTabMode(CBaseView *view, CCmdUI *pCmdUI, int startid)
{
	if (view)
	{
		int cmd = static_cast<int>(pCmdUI->m_nID) - startid;
		if (cmd == TABMODE_NONE)
			pCmdUI->SetCheck((view->GetTabMode() & TABMODE_USESPACES) == TABMODE_NONE);
		else if (cmd == TABMODE_USESPACES)
			pCmdUI->SetCheck(view->GetTabMode() & TABMODE_USESPACES);
		else if (cmd == TABMODE_SMARTINDENT)
			pCmdUI->SetCheck(view->GetTabMode() & TABMODE_SMARTINDENT);
		else if (cmd == TABSIZEBUTTON1)
			pCmdUI->SetCheck(view->GetTabSize() == 1);
		else if (cmd == TABSIZEBUTTON2)
			pCmdUI->SetCheck(view->GetTabSize() == 2);
		else if (cmd == TABSIZEBUTTON4)
			pCmdUI->SetCheck(view->GetTabSize() == 4);
		else if (cmd == TABSIZEBUTTON8)
			pCmdUI->SetCheck(view->GetTabSize() == 8);
		else if (cmd == ENABLEEDITORCONFIG)
			pCmdUI->SetCheck(view->GetEditorConfigEnabled());
		pCmdUI->Enable(view->IsWritable());
		if (cmd == ENABLEEDITORCONFIG)
			pCmdUI->Enable(view->IsWritable() && view->GetEditorConfigLoaded());
	}
	else
		pCmdUI->Enable(FALSE);
}

BOOL CMainFrame::OnShowPopupMenu(CMFCPopupMenu* pMenuPopup)
{
	__super::OnShowPopupMenu(pMenuPopup);

	if (!pMenuPopup)
		return TRUE;

	if (!CMFCToolBar::IsCustomizeMode() &&
		pMenuPopup->GetMenuBar()->CommandToIndex(ID_REGEXFILTER) >= 0)
	{
		BuildRegexSubitems(pMenuPopup);
	}

	return TRUE;
}

LRESULT CMainFrame::OnIdleUpdateCmdUI(WPARAM wParam, LPARAM lParam)
{
	__super::OnIdleUpdateCmdUI(wParam, lParam);

	if (m_pRibbonApp)
	{
		auto bDisableIfNoHandler = static_cast<BOOL>(wParam);
		m_pRibbonApp->UpdateCmdUI(bDisableIfNoHandler);
	}
	return 0;
}

void CMainFrame::OnUpdateThreeWayActions(CCmdUI * pCmdUI)
{
	pCmdUI->Enable();
}

void CMainFrame::OnRegexNoFilter()
{
	if (CheckForSave(CHFSR_OPTIONS) == IDCANCEL)
		return;
	m_Data.SetRegexTokens(std::wregex(), L"");
	m_regexIndex = -1;
	LoadViews(-1);
}

void CMainFrame::OnUpdateRegexNoFilter(CCmdUI * pCmdUI)
{
	pCmdUI->SetCheck(m_regexIndex < 0);
}
