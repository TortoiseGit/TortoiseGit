// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017 - TortoiseGit
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

#include "StandAloneDlg.h"
#include "GitProgressList.h"
#include "MenuButton.h"

/**
 * \ingroup TortoiseProc
 * Handles different git commands and shows the notify messages
 * in a listbox. Since several git commands have similar notify
 * messages they are grouped together in this single class.
 */
class CGitProgressDlg : public CResizableStandAloneDialog
{
public:

	DECLARE_DYNAMIC(CGitProgressDlg)

public:

	CGitProgressDlg(CWnd* pParent = nullptr);
	virtual ~CGitProgressDlg();


	void SetCommand(ProgressCommand* cmd) { m_ProgList.SetCommand(cmd); }
	void SetOptions(DWORD opts) {m_ProgList.SetOptions(opts);}

	/**
	 * If the number of items for which the operation is done on is known
	 * beforehand, that number can be set here. It is then used to show a more
	 * accurate progress bar during the operation.
	 */
	void SetItemCount(long count) {if(count) m_ProgList.SetItemCountTotal(count);}

	bool DidErrorsOccur() {return m_ProgList.m_bErrorsOccurred;}

	enum { IDD = IDD_SVNPROGRESS };


protected:

	virtual BOOL						OnInitDialog() override;
	virtual void						OnCancel() override;
	virtual BOOL						PreTranslateMessage(MSG* pMsg) override;
	virtual void						DoDataExchange(CDataExchange* pDX) override;

	afx_msg void	OnBnClickedLogbutton();
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnClose();
	afx_msg void	OnEnSetfocusInfotext();
	afx_msg LRESULT	OnCtlColorStatic(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH	OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg LRESULT	OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	LRESULT			OnCmdEnd(WPARAM wParam, LPARAM lParam);
	LRESULT			OnCmdStart(WPARAM wParam, LPARAM lParam);

	DECLARE_MESSAGE_MAP()

private:
	virtual void OnOK() override;

	CAnimateCtrl			m_Animate;
	CProgressCtrl			m_ProgCtrl;
	CGitProgressList		m_ProgList;
	CEdit					m_InfoCtrl;
	CStatic					m_ProgLableCtrl;
	CMenuButton				m_cMenuButton;
	PostCmdList				m_PostCmdList;

	CBrush					m_background_brush;
	DWORD					m_AutoClose;

	typedef struct {
		int id;
		int cnt;
		int wmid;
	} ACCELLERATOR;
	std::map<TCHAR, ACCELLERATOR>	m_accellerators;
	HACCEL							m_hAccel;
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};
