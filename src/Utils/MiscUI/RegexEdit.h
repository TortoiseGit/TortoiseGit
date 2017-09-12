// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017 - TortoiseGit
// Copyright (C) 2011 - TortoiseSVN

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

/**
 * \ingroup Utils
 * edit control for entering regular expressions.
 * if the entered regex is invalid, the background is drawn in red.
 */
class CRegexEdit : public CEdit
{
	DECLARE_DYNAMIC(CRegexEdit)

public:
	CRegexEdit();
	virtual ~CRegexEdit();

	bool IsValidRegex() const { return m_bValid; }

protected:
	DECLARE_MESSAGE_MAP()
	afx_msg HBRUSH CtlColor(CDC* /*pDC*/, UINT /*nCtlColor*/);
	virtual ULONG GetGestureStatus(CPoint ptTouch) override;

private:
	CBrush  m_invalidBkgnd;
	bool    m_bValid;
};
