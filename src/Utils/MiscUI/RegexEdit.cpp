// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012, 2017, 2019 - TortoiseGit
// Copyright (C) 2011, 2016 - TortoiseSVN

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
#include "stdafx.h"
#include "RegexEdit.h"
#include <regex>

// CRegexEdit

IMPLEMENT_DYNAMIC(CRegexEdit, CEdit)
CRegexEdit::CRegexEdit()
: m_bValid(true)
{
}

CRegexEdit::~CRegexEdit()
{
}

BEGIN_MESSAGE_MAP(CRegexEdit, CEdit)
	ON_WM_CTLCOLOR_REFLECT()
END_MESSAGE_MAP()

HBRUSH CRegexEdit::CtlColor(CDC* pDC, UINT /*nCtlColor*/)
{
	bool oldState = m_bValid;
	// check if the regex is valid
	m_bValid = true;
	try
	{
		CString sRegex;
		GetWindowText(sRegex);
		const std::wregex regMatch(sRegex, std::regex_constants::icase | std::regex_constants::ECMAScript);
	}
	catch (std::exception&)
	{
		m_bValid = false;
	}

	if (!m_bValid)
	{
		pDC->SetBkColor(GetSysColor(COLOR_3DFACE) - RGB(0,20,20));
		if (!m_invalidBkgnd.GetSafeHandle())
			m_invalidBkgnd.CreateSolidBrush(GetSysColor(COLOR_3DFACE) - RGB(0,20,20));
		return static_cast<HBRUSH>(m_invalidBkgnd.GetSafeHandle());
	}
	else if (!oldState)
		this->Invalidate();

	return nullptr;
}

ULONG CRegexEdit::GetGestureStatus(CPoint /*ptTouch*/)
{
	return 0;
}
