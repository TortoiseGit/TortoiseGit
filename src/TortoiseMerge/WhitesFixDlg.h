// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2013 - TortoiseSVN

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
#include "resource.h"
#include <afxcmn.h>
#include "HistoryCombo.h"
#include "FileTextLines.h"
#include "registry.h"

/**
 * \ingroup TortoiseMerge
 * WhitesFix dialog used in TortoiseMerge.
 */
class CWhitesFixDlg : public CDialog
{
	DECLARE_DYNAMIC(CWhitesFixDlg)

public:
	CWhitesFixDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CWhitesFixDlg();
	void Create(CWnd * pParent = NULL);

	INT_PTR DoModalConfirmMode();
	INT_PTR DoModalSetupMode();

	static void Enable(bool bEnable = true);
	static bool IsEnabled();

	// Dialog Data
	enum { IDD = IDD_WHITESFIX };

	// in out: values for Fix Mode
	BOOL convertSpaces;
	BOOL convertTabs;
	BOOL trimRight;
	BOOL fixEols;
	EOL lineendings;
	BOOL stopAsking;

protected:
	// typedefs
	enum EMode {
		FIX,
		SETUP,
	};

	afx_msg void	OnUseEolsClick() { m_EOL.EnableWindow(m_fixEols.GetCheck()); }
	afx_msg void	OnUseSpacesClick() { m_useTabs.SetCheck(0); }
	afx_msg void	OnUseTabsClick() { m_useSpaces.SetCheck(0); }
	afx_msg void	OnSetupClick() { CWhitesFixDlg().DoModalSetupMode(); }
	afx_msg void	OnStopAskingClick();

	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();

	static DWORD GetSettingsMap() { return CRegDWORD(_T("Software\\TortoiseGitMerge\\FixBeforeSave"), (DWORD)0xAAA9); }
	static void SetSettingsMap(DWORD nNewMap) { CRegDWORD(_T("Software\\TortoiseGitMerge\\FixBeforeSave"), (DWORD)0xAAA9) = nNewMap; }

	CButton m_titleFix;
	CButton m_titleSetup;
	CButton m_useSpaces;
	CButton m_useTabs;
	CButton m_trimRight;
	CButton m_fixEols;
	CComboBox m_EOL;
	CButton m_stopAsking;
	CButton m_setup;

	EMode m_eMode;

private:
	virtual INT_PTR DoModal() { return CDialog::DoModal(); }
};
