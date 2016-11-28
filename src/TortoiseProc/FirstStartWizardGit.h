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
#include "Tooltip.h"
#include "ConfigureGitExe.h"
#include "HyperLink.h"

/**
 * Git page in the first start wizard
 */
class CFirstStartWizardGit : public CFirstStartWizardBasePage, public CConfigureGitExe
{
	DECLARE_DYNAMIC(CFirstStartWizardGit)

public:
	CFirstStartWizardGit();
	virtual ~CFirstStartWizardGit();

	enum { IDD = IDD_FIRSTSTARTWIZARD_GIT };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX);
	virtual LRESULT	OnWizardNext();
	virtual BOOL	OnInitDialog();
	virtual BOOL	OnSetActive();
	virtual BOOL	PreTranslateMessage(MSG* pMsg);

	DECLARE_MESSAGE_MAP()

	afx_msg void	OnMsysGitPathModify();
	afx_msg void	OnBrowseDir();
	afx_msg void	OnCheck();

	CString			m_sMsysGitPath;
	CString			m_sMsysGitExtranPath;

	CToolTips		m_tooltips;
	CHyperLink		m_link;
};
