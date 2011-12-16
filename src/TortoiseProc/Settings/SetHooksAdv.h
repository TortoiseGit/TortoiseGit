// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008,2010 - TortoiseSVN

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
#include "Hooks.h"
#include "StandAloneDlg.h"
#include "Tooltip.h"

/**
 * \ingroup TortoiseProc
 * helper dialog to configure client side hook scripts.
 */
class CSetHooksAdv : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CSetHooksAdv)

public:
	CSetHooksAdv(CWnd* pParent = NULL);   // standard constructor
	virtual ~CSetHooksAdv();

// Dialog Data
	enum { IDD = IDD_SETTINGSHOOKCONFIG };

	hookkey key;
	hookcmd cmd;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();
	afx_msg void OnBnClickedHookbrowse();
	afx_msg void OnBnClickedHookcommandbrowse();
	afx_msg void OnBnClickedHelp();

	DECLARE_MESSAGE_MAP()

	CString			m_sPath;
	CString			m_sCommandLine;
	BOOL			m_bWait;
	BOOL			m_bHide;
	CComboBox		m_cHookTypeCombo;
	CToolTips		m_tooltips;
};
