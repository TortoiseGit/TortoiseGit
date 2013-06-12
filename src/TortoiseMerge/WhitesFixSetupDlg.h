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

/**
 * \ingroup TortoiseMerge
 * WhitesFix dialog used in TortoiseMerge.
 */
class CWhitesFixSetupDlg : public CDialog
{
	DECLARE_DYNAMIC(CWhitesFixSetupDlg)

public:
	CWhitesFixSetupDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CWhitesFixSetupDlg();

	// Dialog Data
	enum { IDD = IDD_WHITESFIXSETUP };

	// in out: values for Fix Mode
	BOOL convertSpaces;
	BOOL trimRight;
	BOOL fixEols;

protected:

	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();

	CButton m_useSpaces;
	CButton m_trimRight;
	CButton m_fixEols;
};
