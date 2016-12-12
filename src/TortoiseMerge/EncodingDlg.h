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

extern const CFileTextLines::UnicodeType uctArray[9];
extern const EOL eolArray[10];

/**
 * \ingroup TortoiseMerge
 * Encoding dialog used in TortoiseMerge.
 */
class CEncodingDlg : public CDialog
{
	DECLARE_DYNAMIC(CEncodingDlg)

public:
	CEncodingDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CEncodingDlg();
	void Create(CWnd* pParent = nullptr) { CDialog::Create(IDD, pParent); ShowWindow(SW_SHOW); UpdateWindow(); }
// Dialog Data
	enum { IDD = IDD_ENCODING };

	CFileTextLines::UnicodeType texttype;
	EOL lineendings;
	CString view;
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
	virtual void OnCancel();
	virtual void OnOK();
	virtual BOOL OnInitDialog();
	CComboBox m_Encoding;
	CComboBox m_EOL;
};
