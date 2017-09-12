// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit

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
#include "FirstStartWizardBasePage.h"
#include "HyperLink.h"

/**
 * First page in the first start wizard
 */
class CFirstStartWizardLanguage : public CFirstStartWizardBasePage
{
	DECLARE_DYNAMIC(CFirstStartWizardLanguage)

public:
	CFirstStartWizardLanguage();
	virtual ~CFirstStartWizardLanguage();

	enum { IDD = IDD_FIRSTSTARTWIZARD_LANGUAGE };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL	OnInitDialog() override;
	virtual BOOL	OnSetActive() override;
	virtual LRESULT	OnWizardNext() override;
	afx_msg LRESULT	OnDialogDisplayed(WPARAM wParam, LPARAM lParam);
	afx_msg void	OnClickedLink(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void	OnBnClickedRefresh();

	DECLARE_MESSAGE_MAP()

	CComboBox		m_LanguageCombo;
	CRegDWORD		m_regLanguage;
	DWORD			m_dwLanguage;

	CHyperLink		m_link;
};
