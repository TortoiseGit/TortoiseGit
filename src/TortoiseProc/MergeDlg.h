// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2011, 2013-2016 - TortoiseGit

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
#include "ChooseVersion.h"
#include "SciEdit.h"
#include "RegHistory.h"

// CMergeDlg dialog

class CMergeDlg : public CResizableStandAloneDialog,public CChooseVersion , public CSciEditContextMenuInterface
{
	DECLARE_DYNAMIC(CMergeDlg)

public:
	CMergeDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CMergeDlg();

// Dialog Data
	enum { IDD = IDD_MERGE };

	BOOL m_bSquash;
	BOOL m_bNoFF;
	BOOL m_bFFonly;
	BOOL m_bNoCommit;
	BOOL m_bLog;
	int m_nLog;
	CString m_MergeStrategy;
	CString m_StrategyOption;
	CString m_StrategyParam;
	//CString m_Base;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;

	CSciEdit			m_cLogMessage;
	ProjectProperties	m_ProjectProperties;

	TCHAR				* m_pDefaultText;
	DECLARE_MESSAGE_MAP()
	CHOOSE_EVENT_RADIO() ;

	// CSciEditContextMenuInterface
	virtual void		InsertMenuItems(CMenu& mPopup, int& nCmd) override;
	virtual bool		HandleMenuItemClick(int cmd, CSciEdit* pSciEdit) override;

public:
	CString m_strLogMesage;

private:
	CRegHistory			m_History;
	int					m_nPopupPasteLastMessage;
	int					m_nPopupRecentMessage;

	void ReloadHistoryEntries();
	afx_msg void OnBnClickedOk();
	virtual void OnCancel() override;
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedCheckSquash();
	afx_msg void OnBnClickedCheckMergeLog();
	afx_msg void OnCbnSelchangeComboMergestrategy();
	afx_msg void OnCbnSelchangeComboStrategyoption();
	afx_msg void OnBnClickedCheckFFonlyOrNoFF();
	afx_msg void OnSysColorChange();
};
