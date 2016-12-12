// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2011, 2013 - TortoiseSVN

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
#include <afxwin.h>


// CGotoLineDlg dialog

class CGotoLineDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CGotoLineDlg)

public:
	CGotoLineDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CGotoLineDlg();

	int			GetLineNumber() const {return m_nLine;}
	void		SetLabel(const CString& label) { m_sLabel = label; }
	void		SetLimits(int low, int high) { m_nLow = low; m_nHigh = high; }

	// Dialog Data
	enum { IDD = IDD_GOTO };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();

	DECLARE_MESSAGE_MAP()
private:
	int			m_nLine;
	int			m_nLow;
	int			m_nHigh;
	CString		m_sLabel;
	CEdit		m_cNumber;
};
