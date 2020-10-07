// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2014-2017, 2019-2020 - TortoiseGit

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
#include "StandAloneDlg.h"
#include "SciEdit.h"
#include "FindBar.h"
#include "DiffLinesForStaging.h"
#include "StagingOperations.h"
#include "EnableStagingTypes.h"

class IHasPatchView
{
public:
	virtual CWnd *GetPatchViewParentWnd() = 0;
	virtual void TogglePatchView() = 0;
};

// CPatchViewDlg dialog
class CPatchViewDlg : public CStandAloneDialog, public CSciEditContextMenuInterface
{
	DECLARE_DYNAMIC(CPatchViewDlg)

public:
	CPatchViewDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CPatchViewDlg();
	IHasPatchView	*m_ParentDlg;
	void SetText(const CString& text);
	void ClearView();
	void ShowAndAlignToParent();
	void ParentOnMoving(HWND parentHWND, LPRECT pRect);
	void ParentOnSizing(HWND parentHWND, LPRECT pRect);
	void EnableStaging(EnableStagingTypes enableStagingType);

// Dialog Data
	enum { IDD = IDD_PATCH_VIEW };

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

public:
	CSciEdit			m_ctrlPatchView;

protected:
	DECLARE_MESSAGE_MAP()

	virtual BOOL OnInitDialog() override;
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnClose();
	afx_msg void OnDestroy();

	afx_msg void OnShowFindBar();
	afx_msg void OnFindNext();
	afx_msg void OnFindPrev();
	afx_msg void OnFindReset();
	afx_msg void OnFindExit();
	afx_msg void OnEscape();
	afx_msg void OnStageLines();
	afx_msg void OnStageHunks();
	afx_msg void OnUnstageLines();
	afx_msg void OnUnstageHunks();
	LRESULT OnFindNextMessage(WPARAM, LPARAM);
	LRESULT OnFindPrevMessage(WPARAM, LPARAM);
	LRESULT OnFindResetMessage(WPARAM, LPARAM);
	LRESULT OnFindExitMessage(WPARAM, LPARAM);
	static UINT WM_PARTIALSTAGINGREFRESHPATCHVIEW;

	void				DoSearch(bool reverse);
	CFindBar            m_FindBar;
	bool                m_bShowFindBar;

	HACCEL				m_hAccel;

	EnableStagingTypes	m_nEnableStagingType;

	// CSciEditContextMenuInterface
	virtual void		InsertMenuItems(CMenu& mPopup, int& nCmd) override;
	virtual bool		HandleMenuItemClick(int cmd, CSciEdit* pSciEdit) override;
	int					m_nPopupSave;
	int					m_nStageHunks;
	int					m_nStageLines;
	int					m_nUnstageHunks;
	int					m_nUnstageLines;

	int GetFirstLineNumberSelected();
	int GetLastLineNumberSelected();
	std::unique_ptr<char[]> GetFullLineByLineNumber(int line);
	void StageOrUnstageSelectedLinesOrHunks(StagingType stagingType);
};
