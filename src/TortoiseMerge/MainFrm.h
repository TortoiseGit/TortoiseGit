// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseGit
// Copyright (C) 2006-2015, 2017 - TortoiseSVN

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
#pragma once

#include "DiffData.h"
#include "LocatorBar.h"
#include "LineDiffBar.h"
#include "FilePatchesDlg.h"
#include "TempFile.h"
#include "XSplitter.h"
#include "GitPatch.h"
#include "../../ext/SimpleIni/SimpleIni.h"
#include "CustomMFCRibbonStatusBar.h"
#include <tuple>
#include "NativeRibbonApp.h"

class CLeftView;
class CRightView;
class CBottomView;
#define MOVESTOIGNORE 3

#define TABMODE_NONE		0x00
#define TABMODE_USESPACES	0x01
#define TABMODE_SMARTINDENT	0x02

#define TABSIZEBUTTON1 3
#define TABSIZEBUTTON2 4
#define TABSIZEBUTTON4 5
#define TABSIZEBUTTON8 6
#define ENABLEEDITORCONFIG 8


/**
 * \ingroup TortoiseMerge
 * The main frame of TortoiseMerge. Handles all the menu and toolbar commands.
 */
class CMainFrame : public CFrameWndEx, public CPatchFilesDlgCallBack //CFrameWndEx
{
public:
	CMainFrame();
	virtual ~CMainFrame();

	void			ShowDiffBar(bool bShow);
	void			DiffLeftToBase();
	void			DiffRightToBase();

#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif
protected:
	DECLARE_DYNCREATE(CMainFrame)

	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL	OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual void	ActivateFrame(int nCmdShow = -1);
	virtual BOOL	OnShowPopupMenu(CMFCPopupMenu* pMenuPopup);
	/// line = -1 means keep the current position,
	/// line >= 0 means scroll to that line,
	/// and line == -2 means do nothing or scroll to first diff depending on registry setting
	bool			LoadViews(int line = -2);
	void			ClearViewNamesAndPaths();
	void			SetWindowTitle();
	void			RecalcLayout(BOOL bNotify = TRUE) override;

	afx_msg LRESULT	OnTaskbarButtonCreated(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT	OnIdleUpdateCmdUI(WPARAM wParam, LPARAM);

	afx_msg void	OnFileSave();
	afx_msg void	OnFileSaveAs();
	afx_msg void	OnFileOpen();
	afx_msg void	OnFileOpen(bool fillyours);

	afx_msg void	OnFileReload();
	afx_msg void	OnClose();
	afx_msg void	OnActivate(UINT, CWnd*, BOOL);
	afx_msg void	OnViewWhitespaces();
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void	OnDestroy();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void	OnUpdateFileSave(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateFileSaveAs(CCmdUI *pCmdUI);
	afx_msg void	OnViewOnewaydiff();
	afx_msg void	OnUpdateViewOnewaydiff(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateViewWhitespaces(CCmdUI *pCmdUI);
	afx_msg void	OnViewOptions();
	afx_msg void	OnViewLinedown();
	afx_msg void	OnViewLineup();
	afx_msg void	OnViewLineleft();
	afx_msg void	OnViewLineright();
	afx_msg void	OnEditUseTheirs();
	afx_msg void	OnEditUseMine();
	afx_msg void	OnEditUseTheirsThenMine();
	afx_msg void	OnEditUseMineThenTheirs();
	afx_msg void	OnUpdateEditUseminethentheirblock(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditUsemyblock(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditUsetheirblock(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditUsetheirthenmyblock(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateMergeMarkasresolved(CCmdUI *pCmdUI);
	afx_msg void	OnMergeMarkasresolved();
	afx_msg void	OnUpdateMergeNextconflict(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateMergePreviousconflict(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditCopy(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditPaste(CCmdUI *pCmdUI);
	afx_msg void	OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void	OnViewSwitchleft();
	afx_msg void	OnUpdateViewSwitchleft(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateViewShowfilelist(CCmdUI *pCmdUI);
	afx_msg void	OnViewShowfilelist();
	afx_msg void	OnEditUndo();
	afx_msg void	OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void	OnEditRedo();
	afx_msg void	OnUpdateEditRedo(CCmdUI *pCmdUI);
	afx_msg void	OnEditEnable();
	afx_msg void	OnUpdateEditEnable(CCmdUI *pCmdUI);
	afx_msg void	OnViewInlinediffword();
	afx_msg void	OnUpdateViewInlinediffword(CCmdUI *pCmdUI);
	afx_msg void	OnViewInlinediff();
	afx_msg void	OnUpdateViewInlinediff(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditCreateunifieddifffile(CCmdUI *pCmdUI);
	afx_msg void	OnEditCreateunifieddifffile();
	afx_msg void	OnUpdateViewLinediffbar(CCmdUI *pCmdUI);
	afx_msg void	OnViewLinediffbar();
	afx_msg void	OnUpdateViewLocatorbar(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateViewBars(CCmdUI *pCmdUI);
	afx_msg void	OnViewLocatorbar();
	afx_msg void	OnEditUseleftblock();
	afx_msg void	OnUpdateUseBlock(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditUseleftblock(CCmdUI *pCmdUI);
	afx_msg void	OnEditUseleftfile();
	afx_msg void	OnUpdateEditUseleftfile(CCmdUI *pCmdUI);
	afx_msg void	OnEditUseblockfromleftbeforeright();
	afx_msg void	OnUpdateEditUseblockfromleftbeforeright(CCmdUI *pCmdUI);
	afx_msg void	OnEditUseblockfromrightbeforeleft();
	afx_msg void	OnUpdateEditUseblockfromrightbeforeleft(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateNavigateNextdifference(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateNavigatePreviousdifference(CCmdUI *pCmdUI);
	afx_msg void	OnViewCollapsed();
	afx_msg void	OnUpdateViewCollapsed(CCmdUI *pCmdUI);
	afx_msg void	OnViewComparewhitespaces();
	afx_msg void	OnUpdateViewComparewhitespaces(CCmdUI *pCmdUI);
	afx_msg void	OnViewIgnorewhitespacechanges();
	afx_msg void	OnUpdateViewIgnorewhitespacechanges(CCmdUI *pCmdUI);
	afx_msg void	OnViewIgnoreallwhitespacechanges();
	afx_msg void	OnUpdateViewIgnoreallwhitespacechanges(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateNavigateNextinlinediff(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateNavigatePrevinlinediff(CCmdUI *pCmdUI);
	afx_msg void	OnViewMovedBlocks();
	afx_msg void	OnUpdateViewMovedBlocks(CCmdUI *pCmdUI);
	afx_msg void	OnViewWraplonglines();
	afx_msg void	OnUpdateViewWraplonglines(CCmdUI *pCmdUI);
	afx_msg void	OnIndicatorLeftview();
	afx_msg void	OnIndicatorRightview();
	afx_msg void	OnIndicatorBottomview();
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnViewIgnorecomments();
	afx_msg void	OnUpdateViewIgnorecomments(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateViewRegexFilter(CCmdUI *pCmdUI);
	afx_msg void	OnRegexfilter(UINT cmd);
	afx_msg void	OnDummyEnabled() {};
	afx_msg void	OnEncodingLeft(UINT cmd);
	afx_msg void	OnEncodingRight(UINT cmd);
	afx_msg void	OnEncodingBottom(UINT cmd);
	afx_msg void	OnEOLLeft(UINT cmd);
	afx_msg void	OnEOLRight(UINT cmd);
	afx_msg void	OnEOLBottom(UINT cmd);
	afx_msg void	OnTabModeLeft(UINT cmd);
	afx_msg void	OnTabModeRight(UINT cmd);
	afx_msg void	OnTabModeBottom(UINT cmd);
	afx_msg void	OnUpdateEncodingLeft(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEncodingRight(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEncodingBottom(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEOLLeft(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEOLRight(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEOLBottom(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateTabModeLeft(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateTabModeRight(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateTabModeBottom(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateThreeWayActions(CCmdUI* pCmdUI);
	afx_msg	void	OnRegexNoFilter();
	afx_msg void	OnUpdateRegexNoFilter(CCmdUI* pCmdUI);

	DECLARE_MESSAGE_MAP()
protected:
	void			UpdateLayout();
	virtual	BOOL	PatchFile(CString sFilePath, bool bContentMods, bool bPropMods, CString sVersion, BOOL bAutoPatch) override;
	virtual BOOL	DiffFiles(CString sURL1, CString sRev1, CString sURL2, CString sRev2) override;
	int				CheckResolved();
	BOOL			MarkAsResolved();
	int				SaveFile(const CString& sFilePath);
	void			WriteWindowPlacement(WINDOWPLACEMENT * pwp);
	BOOL			ReadWindowPlacement(WINDOWPLACEMENT * pwp);
	bool			FileSave(bool bCheckResolved=true);
	void			PatchSave();
	bool			FileSaveAs(bool bCheckResolved=true);
	void			LoadIgnoreCommentData();
	/// checks if there are modifications and asks the user to save them first
	/// IDCANCEL is returned if the user wants to cancel.
	/// If the user wanted to save the modifications, this method does the saving
	/// itself.
	int				CheckForReload();
	enum ECheckForSaveReason {
		CHFSR_CLOSE, ///< closing apps
		CHFSR_SWITCH, ///< switching views
		CHFSR_RELOAD, ///< reload views also switching between 1 and 2 way diff
		CHFSR_OPTIONS, ///< white space change, options
		CHFSR_OPEN, ///< open open dialog
	};
	/// checks if there are modifications and asks the user to save them first
	/// IDCANCEL is returned if the user wants to cancel.
	/// If the user wanted to save the modifications, this method does the saving
	/// itself.
	int				CheckForSave(ECheckForSaveReason eReason/* = CHFSR_SWITCH*/);
	void			DeleteBaseTheirsMineOnClose();
	void			OnViewLineUpDown(int direction);
	void			OnViewLineLeftRight(int direction);
	static void		OnTabMode(CBaseView *view, int cmd);
	static void		OnUpdateTabMode(CBaseView *view, CCmdUI *pCmdUI, int startid);
	bool			HasConflictsWontKeep();
	bool			TryGetFileName(CString& result);
	CBaseView*		GetActiveBaseView() const;
	void			OnViewTextFoldUnfold();
	void			OnViewTextFoldUnfold(CBaseView* view);
	bool			HasUnsavedEdits() const;
	static bool		HasUnsavedEdits(const CBaseView* view);
	bool			HasMarkedBlocks() const;
	static bool		IsViewGood(const CBaseView* view);
	static bool		HasPrevConflict(CBaseView* view);
	static bool		HasNextConflict(CBaseView* view);
	static bool		HasPrevInlineDiff(CBaseView* view);
	static bool		HasNextInlineDiff(CBaseView* view);
	void			BuildRegexSubitems(CMFCPopupMenu* pMenuPopup = nullptr);
	bool			AdjustUnicodeTypeForLoad(CFileTextLines::UnicodeType& type);
	void			DiffTwo(const CWorkingFile& file1, const CWorkingFile& file2);

protected:
	CMFCStatusBar	m_wndStatusBar;
	CCustomMFCRibbonStatusBar	m_wndRibbonStatusBar;
	CLocatorBar		m_wndLocatorBar;
	CLineDiffBar	m_wndLineDiffBar;
	CXSplitter		m_wndSplitter;
	CXSplitter		m_wndSplitter2;
	CFilePatchesDlg	m_dlgFilePatches;

	GitPatch		m_Patch;
	BOOL			m_bInitSplitter;
	bool			m_bCheckReload;

	bool			m_bHasConflicts;

	bool			m_bInlineWordDiff;
	bool			m_bInlineDiff;
	bool			m_bLineDiff;
	bool			m_bLocatorBar;
	bool			m_bUseRibbons;

	CRegDWORD		m_regWrapLines;
	CRegDWORD		m_regViewModedBlocks;
	CRegDWORD		m_regOneWay;
	CRegDWORD		m_regCollapsed;
	CRegDWORD		m_regInlineDiff;
	CRegDWORD		m_regUseRibbons;
	CRegDWORD		m_regIgnoreComments;

	std::map<CString, std::tuple<CString, CString, CString>>	m_IgnoreCommentsMap;
	CSimpleIni		m_regexIni;
	int				m_regexIndex;
public:
	CLeftView *		m_pwndLeftView;
	CRightView *	m_pwndRightView;
	CBottomView *	m_pwndBottomView;
	BOOL			m_bOneWay;
	BOOL			m_bReversedPatch;
	CDiffData		m_Data;
	bool			m_bReadOnly;
	bool			m_bBlame;
	int				m_nMoveMovesToIgnore;
	bool			m_bCollapsed;
	bool			m_bViewMovedBlocks;
	bool			m_bWrapLines;
	bool			m_bSaveRequired;
	bool			m_bSaveRequiredOnConflicts;
	bool			m_bDeleteBaseTheirsMineOnClose;
	HWND			resolveMsgWnd;
	WPARAM			resolveMsgWParam;
	LPARAM			resolveMsgLParam;

	const CMFCToolBar *   GetToolbar() const { return &m_wndToolBar; }
	void			FillEncodingButton( CMFCRibbonButton * pButton, int start );
	void			FillEOLButton( CMFCRibbonButton * pButton, int start );
	void			FillTabModeButton(CMFCRibbonButton * pButton, int start);
	CMFCMenuBar		m_wndMenuBar;
	CMFCToolBar		m_wndToolBar;

	std::unique_ptr<CNativeRibbonApp> m_pRibbonApp;
	CComPtr<IUIFramework> m_pRibbonFramework;
};