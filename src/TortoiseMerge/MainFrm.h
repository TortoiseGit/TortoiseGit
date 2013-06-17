// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseGit
// Copyright (C) 2006-2012 - TortoiseSVN

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

class CLeftView;
class CRightView;
class CBottomView;
#define MOVESTOIGNORE 3

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
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif
protected:
	DECLARE_DYNCREATE(CMainFrame)

	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL	OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual void	ActivateFrame(int nCmdShow = -1);
	/// line = -1 means keep the current position,
	/// line >= 0 means scroll to that line,
	/// and line == -2 means do nothing or scroll to first diff depending on registry setting
	bool			LoadViews(int line = -2);
	void			ClearViewNamesAndPaths();
	void			SetWindowTitle();

	afx_msg LRESULT	OnTaskbarButtonCreated(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnApplicationLook(UINT id);
	afx_msg void	OnUpdateApplicationLook(CCmdUI* pCmdUI);

	afx_msg void	OnFileSave();
	afx_msg void	OnFileSaveAs();
	afx_msg void	OnFileOpen();
	afx_msg void	OnFileReload();
	afx_msg void	OnClose();
	afx_msg void	OnActivate(UINT, CWnd*, BOOL);
	afx_msg void	OnViewWhitespaces();
	afx_msg int		OnCreate(LPCREATESTRUCT lpCreateStruct);
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
	void			OnViewLineUpDown(int direction);
	void			OnViewLineLeftRight(int direction);
	bool			HasConflictsWontKeep();
	bool			TryGetFileName(CString& result);
	CBaseView*		GetActiveBaseView() const;
	void			OnViewTextFoldUnfold();
	void			OnViewTextFoldUnfold(CBaseView* view);
	bool			HasUnsavedEdits() const;
	static bool		HasUnsavedEdits(const CBaseView* view);
	static bool		IsViewGood(const CBaseView* view);
	static bool		HasPrevConflict(CBaseView* view);
	static bool		HasNextConflict(CBaseView* view);
	static bool		HasPrevInlineDiff(CBaseView* view);
	static bool		HasNextInlineDiff(CBaseView* view);

protected:
	CMFCStatusBar	m_wndStatusBar;
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
	bool			m_bUseTaskDialog;

	CMFCRibbonBar				m_wndRibbonBar;
	CMFCRibbonApplicationButton	m_MainButton;

	CRegDWORD		m_regWrapLines;
	CRegDWORD		m_regViewModedBlocks;
	CRegDWORD		m_regOneWay;
	CRegDWORD		m_regCollapsed;
	CRegDWORD		m_regInlineDiff;
	CRegDWORD		m_regUseRibbons;
	CRegDWORD		m_regUseTaskDialog;
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
	HWND			resolveMsgWnd;
	WPARAM			resolveMsgWParam;
	LPARAM			resolveMsgLParam;

	const CMFCToolBar *   GetToolbar() const { return &m_wndToolBar; }
	CMFCMenuBar		m_wndMenuBar;
	CMFCToolBar		m_wndToolBar;
};