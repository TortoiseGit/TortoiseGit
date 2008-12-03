// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
#include "MergeWizardBasePage.h"
#include "HistoryCombo.h"
#include "LogDlg.h"

/**
 * Page in the merge wizard for selecting two urls and revisions for
 * a tree merge.
 */
class CMergeWizardReintegrate : public CMergeWizardBasePage
{
	DECLARE_DYNAMIC(CMergeWizardReintegrate)

public:
	CMergeWizardReintegrate();
	virtual ~CMergeWizardReintegrate();

	enum { IDD = IDD_MERGEWIZARD_REINTEGRATE };

protected:
	virtual void		DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL		OnInitDialog();
	virtual LRESULT		OnWizardNext();
	virtual LRESULT		OnWizardBack();
	virtual BOOL		OnSetActive();

	BOOL				CheckData(bool bShowErrors = true);

	afx_msg void		OnBnClickedShowmergelog();
	afx_msg void		OnBnClickedBrowse();


	DECLARE_MESSAGE_MAP()

	CHistoryCombo		m_URLCombo;
	CLogDlg	*			m_pLogDlg;
	CLogDlg	*			m_pLogDlg2;

public:
	CString				m_URL;
	afx_msg void OnBnClickedShowlogwc();
};
