// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007 - TortoiseSVN

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


/**
 * First page in the merge wizard for selecting the merge type
 */
class CMergeWizardStart : public CMergeWizardBasePage
{
	DECLARE_DYNAMIC(CMergeWizardStart)

public:
	CMergeWizardStart();
	virtual ~CMergeWizardStart();

	enum { IDD = IDD_MERGEWIZARD_START };
	
protected:
	virtual void		DoDataExchange(CDataExchange* pDX);
	virtual LRESULT		OnWizardNext();
	virtual BOOL		OnInitDialog();
	virtual BOOL		OnSetActive();

	DECLARE_MESSAGE_MAP()
};
