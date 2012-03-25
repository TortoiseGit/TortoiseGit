// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2009 - TortoiseSVN

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
#include "TempFiles.h"
#include "XSplitter.h"
#include "Patch.h"
#include "FindDlg.h"

class CLeftView;
class CRightView;
class CBottomView;

/**
 * \ingroup TortoiseMerge
 * The main frame of TortoiseMerge. Handles all the menu and toolbar commands.
 */
class CMainFrame : public CFrameWndEx, public CPatchFilesDlgCallBack //CFrameWndEx
{
	
public:
	CMainFrame();
	virtual ~CMainFrame();

#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif
protected: 
	DECLARE_DYNCREATE(CMainFrame)

	virtual BOOL	PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL	OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	virtual void	ActivateFrame(int nCmdShow = -1);
	bool			LoadViews(bool bRetainPosition = false);
	void			ClearViewNamesAndPaths();
	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnApplicationLook(UINT id);
	afx_msg void	OnUpdateApplicationLook(CCmdUI* pCmdUI);

	afx_msg void	OnFileSave();
	afx_msg void	OnFileSaveAs();
	afx_msg void	OnFileOpen();
	afx_msg void	OnFileReload();
	afx_msg void	OnClose();
	afx_msg void	OnEditFind();
	afx_msg void	OnEditFindnext();
	afx_msg void	OnEditFindprev();
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
	afx_msg void	OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void	OnViewSwitchleft();
	afx_msg void	OnUpdateViewSwitchleft(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateViewShowfilelist(CCmdUI *pCmdUI);
	afx_msg void	OnViewShowfilelist();
	afx_msg void	OnEditUndo();
	afx_msg void	OnUpdateEditUndo(CCmdUI *pCmdUI);
	afx_msg void	OnViewInlinediffword();
	afx_msg void	OnUpdateViewInlinediffword(CCmdUI *pCmdUI);
	afx_msg void	OnUpdateEditCreateunifieddifffile(CCmdUI *pCmdUI);
	afx_msg void	OnEditCreateunifieddifffile();
	afx_msg void	OnUpdateViewLinediffbar(CCmdUI *pCmdUI);
	afx_msg void	OnViewLinediffbar();
	afx_msg void	OnUpdateViewLocatorbar(CCmdUI *pCmdUI);
	afx_msg void	OnViewLocatorbar();
	afx_msg void	OnEditUseleftblock();
	afx_msg void	OnUpdateEditUseleftblock(CCmdUI *pCmdUI);
	afx_msg void	OnEditUseleftfile();
	afx_msg void	OnUpdateEditUseleftfile(CCmdUI *pCmdUI);
	afx_msg void	OnEditUseblockfromleftbeforeright();
	afx_msg void	OnUpdateEditUseblockfromleftbeforeright(CCmdUI *pCmdUI);
	afx_msg void	OnEditUseblockfromrightbeforeleft();
	afx_msg void	OnUpdateEditUseblockfromrightbeforeleft(CCmdUI *pCmdUI);

	DECLARE_MESSAGE_MAP()
protected:
	void			UpdateLayout();
	virtual BOOL	PatchFile(const int nIndex, bool bAutoPatch, bool bIsReview);
	virtual BOOL	DiffFiles(CString sURL1, CString sRev1, CString sURL2, CString sRev2);
	int				CheckResolved();
	BOOL			MarkAsResolved();
	int				SaveFile(const CString& sFilePath);
	void			WriteWindowPlacement(WINDOWPLACEMENT * pwp);
	BOOL			ReadWindowPlacement(WINDOWPLACEMENT * pwp);
	bool			FileSave(bool bCheckResolved=true);
	bool			FileSaveAs(bool bCheckResolved=true);
	bool 			StringFound(const CString&)const;
	enum SearchDirection{SearchNext=0, SearchPrevious=1};	
	void 			Search(SearchDirection);
	int				FindSearchStart(int nDefault);
	/// checks if there are modifications and asks the user to save them first
	/// IDCANCEL is returned if the user wants to cancel.
	/// If the user wanted to save the modifications, this method does the saving
	/// itself.
	int				CheckForSave();

protected: 
	CMFCMenuBar     m_wndMenuBar;
	CMFCStatusBar	m_wndStatusBar;
	CMFCToolBar		m_wndToolBar;
	CLocatorBar		m_wndLocatorBar;
	CLineDiffBar	m_wndLineDiffBar;
	CXSplitter		m_wndSplitter;
	CXSplitter		m_wndSplitter2;
	CFilePatchesDlg m_dlgFilePatches;

	CPatch			m_Patch;
	BOOL			m_bInitSplitter;
	CTempFiles		m_TempFiles;

	int				m_nSearchIndex;
	CString			m_sFindText;
	BOOL			m_bMatchCase;
	bool			m_bLimitToDiff;
	bool			m_bWholeWord;
	static const UINT m_FindDialogMessage;
	CFindDlg *		m_pFindDialog;
	bool			m_bHasConflicts;

	bool			m_bInlineWordDiff;
	bool			m_bLineDiff;
	bool			m_bLocatorBar;

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

	void			ShowDiffBar(bool bShow);
};






