// TortoiseMerge - a Diff/Patch program

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
#include "afxcmn.h"
#include "../../ext/SimpleIni/SimpleIni.h"

// CRegexFiltersDlg dialog

class CRegexFiltersDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CRegexFiltersDlg)

public:
	CRegexFiltersDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CRegexFiltersDlg();

// Dialog Data
	enum { IDD = IDD_REGEXFILTERS };

	void			SetIniFile(CSimpleIni * pIni) { m_pIni = pIni; }
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedAdd();
	afx_msg void OnBnClickedEdit();
	afx_msg void OnBnClickedRemove();
	afx_msg void OnNMDblclkRegexlist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedRegexlist(NMHDR *pNMHDR, LRESULT *pResult);

	DECLARE_MESSAGE_MAP()

	void		SetupListControl();
private:
	CListCtrl		m_RegexList;
	CSimpleIni *	m_pIni;
};
