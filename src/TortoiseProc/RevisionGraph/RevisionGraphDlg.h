// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2009, 2011 - TortoiseSVN
// Copyright (C) 2012 - TortoiseGit

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
//#include "RevisionGraph/AllGraphOptions.h"
#include "ProgressDlg.h"
#include "Colors.h"
#include "RevisionGraphWnd.h"
#include "StandAloneDlg.h"
#include "Win7.h"

/**
 * \ingroup TortoiseProc
 * Helper class extending CToolBar, needed only to have the toolbar include
 * a combobox.
 */
class CRevGraphToolBar : public CToolBar
{
public:
	CComboBoxEx		m_ZoomCombo;
};

/**
 * \ingroup TortoiseProc
 * A dialog showing a revision graph.
 *
 * The analyzation of the log data is done in the child class CRevisionGraph,
 * the drawing is done in the member class CRevisionGraphWnd
 * Here, we handle window messages.
 */
class CRevisionGraphDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CRevisionGraphDlg)
public:
	CRevisionGraphDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CRevisionGraphDlg();
	enum { IDD = IDD_REVISIONGRAPH };


	void			SetPath(const CString& sPath) {m_Graph.m_sPath = sPath;}
//	void			SetPegRevision(SVNRev revision) {m_Graph.m_pegRev = revision;}
	void			DoZoom (float factor);

	void			UpdateFullHistory();
	void			StartWorkerThread();

	void			StartHidden() { m_bVisible = false; }
	void			SetOutputFile(const CString& path) { m_outputPath = path; }
	CString		 	GetOutputFile() const { return m_outputPath; }
//	void			SetOptions(DWORD options) { m_Graph.m_state.GetOptions()->SetRegistryFlags(options, 0x407fbf); }

protected:
	bool			m_bFetchLogs;
	char			m_szTip[MAX_TT_LENGTH+1];
	wchar_t		 	m_wszTip[MAX_TT_LENGTH+1];

	CString		 	m_sFilter;

	HACCEL			m_hAccel;

	BOOL			InitializeToolbar();

	virtual void	DoDataExchange(CDataExchange* pDX);		// DDX/DDV support
	virtual BOOL	OnInitDialog();
	virtual void	OnCancel();
	virtual void	OnOK();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg void	OnViewFilter();
	afx_msg void	OnViewZoomin();
	afx_msg void	OnViewZoomout();
	afx_msg void	OnViewZoom100();
	afx_msg void	OnViewZoomHeight();
	afx_msg void	OnViewZoomWidth();
	afx_msg void	OnViewZoomAll();
	afx_msg void	OnViewCompareheadrevisions();
	afx_msg void	OnViewComparerevisions();
	afx_msg void	OnViewUnifieddiff();
	afx_msg void	OnViewUnifieddiffofheadrevisions();
	afx_msg void	OnViewShowoverview();
	afx_msg void	OnFileSavegraphas();
	afx_msg void	OnMenuexit();
	afx_msg void	OnMenuhelp();
	afx_msg void	OnChangeZoom();
	afx_msg BOOL	OnToggleOption (UINT controlID);
	afx_msg BOOL	OnToggleReloadOption (UINT controlID);
	afx_msg BOOL	OnToggleRedrawOption (UINT controlID);
	afx_msg BOOL	OnToolTipNotify (UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void	OnWindowPosChanging(WINDOWPOS* lpwndpos);

	DECLARE_MESSAGE_MAP()

	BOOL			ToggleOption (UINT controlID);
	void			SetOption (UINT controlID);

	CRect			GetGraphRect();
	void			UpdateStatusBar();

private:
	void			UpdateZoomBox();
	void			UpdateOptionAvailability (UINT id, bool available);
	void			UpdateOptionAvailability();

	bool			UpdateData();

	float						m_fZoomFactor;
	CRevisionGraphWnd			m_Graph;
	CStatusBarCtrl				m_StatusBar;
	CRevGraphToolBar			m_ToolBar;
	bool						m_bVisible;
	CString						m_outputPath;
	ULONG_PTR					m_gdiPlusToken;
	CComPtr<ITaskbarList3>		m_pTaskbarList;
};
