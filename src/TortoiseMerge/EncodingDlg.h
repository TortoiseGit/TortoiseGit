// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2020, 2023, 2025 - TortoiseGit
// Copyright (C) 2013, 2020 - TortoiseSVN

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
#include "FileTextLines.h"
#include "StandAloneDlg.h"

extern const CFileTextLines::UnicodeType uctArray[9];
extern const EOL eolArray[10];

/**
 * \ingroup TortoiseMerge
 * Encoding dialog used in TortoiseMerge.
 */
class CEncodingDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CEncodingDlg)

public:
	CEncodingDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CEncodingDlg();
	void Create(CWnd* pParent = nullptr) { __super::Create(IDD, pParent); ShowWindow(SW_SHOW); UpdateWindow(); }
// Dialog Data
	enum { IDD = IDD_ENCODING };

	CFileTextLines::UnicodeType texttype = CFileTextLines::UnicodeType::ASCII;
	EOL lineendings = EOL::AutoLine;
	CString view;
protected:
	void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	void OnCancel() override;
	void OnOK() override;
	BOOL OnInitDialog() override;
	CComboBox m_Encoding;
	CComboBox m_EOL;
};
