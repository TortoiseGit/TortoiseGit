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
#pragma once

#include "SVNRev.h"
#include "HistoryCombo.h"
#include "Tooltip.h"
#include "XPImageButton.h"

class CRepositoryTree;

/**
 * \ingroup TortoiseProc
 * Interface definition
 */
class IRepo
{
public:
	virtual bool ChangeToUrl(CString& url, SVNRev& rev, bool bAlreadyChecked) = 0;
	virtual CString GetRepoRoot() = 0;
};

/**
 * \ingroup TortoiseProc
 * Provides a CReBarCtrl which can be used as a "control center" for the
 * repository browser. To associate the repository bar with any repository
 * tree control, call AssocTree() once after both objects are created. As
 * long as they are associated, the bar and the tree form a "team" of
 * controls that work together.
 */
class CRepositoryBar : public CReBarCtrl
{
	DECLARE_DYNAMIC(CRepositoryBar)

public:
	CRepositoryBar();
	virtual ~CRepositoryBar();

public:
	/**
	 * Creates the repository bar. Set \a in_dialog to TRUE when the bar
	 * is placed inside of a dialog. Otherwise it is assumed that the
	 * bar is placed in a frame window.
	 */
	bool Create(CWnd* parent, UINT id, bool in_dialog = true);

	/**
	 * Show the given \a svn_url in the URL combo and the revision button.
	 */
	void ShowUrl(const CString& url, SVNRev rev);

	/**
	 * Show the given \a svn_url in the URL combo and the revision button,
	 * and select it in the associated repository tree. If no \a svn_url
	 * is given, the current values are used (which effectively refreshes
	 * the tree).
	 */
	void GotoUrl(const CString& url = CString(), SVNRev rev = SVNRev(), bool bAlreadyChecked = false);

	/**
	 * Returns the current URL.
	 */
	CString GetCurrentUrl() const;

	/**
	 * Returns the current revision
	 */
	SVNRev GetCurrentRev() const;

	/**
	 * Saves the URL history of the HistoryCombo.
	 */
	void SaveHistory();
	
	/**
	 * Set the revision
	 */
	void SetRevision(SVNRev rev);

	void SetFocusToURL();

	void SetIRepo(IRepo * pRepo) {m_pRepo = pRepo;}

	void SetHeadRevision(const SVNRev& rev) {m_headRev = rev;}
	afx_msg void OnGoUp();
protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnCbnSelChange();
	afx_msg void OnBnClicked();
	afx_msg void OnDestroy();

	DECLARE_MESSAGE_MAP()

private:
	CString m_url;
	SVNRev m_rev;

	IRepo * m_pRepo;

	class CRepositoryCombo : public CHistoryCombo 
	{
		CRepositoryBar *m_bar;
	public:
		CRepositoryCombo(CRepositoryBar *bar) : m_bar(bar) {}
		virtual bool OnReturnKeyPressed();
	} m_cbxUrl;

	CButton m_btnRevision;
	CXPImageButton m_btnUp;

	SVNRev	m_headRev;
	CToolTips m_tooltips;
	HICON	m_UpIcon;
};


/**
 * \ingroup TortoiseProc
 * Implements a visual container for a CRepositoryBar which may be added to a
 * dialog. A CRepositoryBarCnr is not needed if the CRepositoryBar is attached
 * to a frame window.
 */
class CRepositoryBarCnr : public CStatic
{
	DECLARE_DYNAMIC(CRepositoryBarCnr)

public:
	CRepositoryBarCnr(CRepositoryBar *repository_bar);
	~CRepositoryBarCnr();

	// Generated message map functions
protected:
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);

	virtual void DrawItem(LPDRAWITEMSTRUCT);

	DECLARE_MESSAGE_MAP()

private:
	CRepositoryBar *m_pbarRepository;

};


