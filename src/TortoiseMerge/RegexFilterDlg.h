// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2013-2014 - TortoiseSVN

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


// CRegexFilterDlg dialog

class CRegexFilterDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRegexFilterDlg)

public:
	CRegexFilterDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRegexFilterDlg();

// Dialog Data
	enum { IDD = IDD_REGEXFILTER };

	CString			m_sName;
	CString			m_sRegex;
	CString			m_sReplace;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void ShowEditBalloon(UINT nIdControl, UINT nIdText, UINT nIdTitle, int nIcon = TTI_WARNING);

	DECLARE_MESSAGE_MAP()
};
