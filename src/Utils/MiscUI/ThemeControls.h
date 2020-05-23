// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit
// Copyright (C) 2020 - TortoiseSVN

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

/// a CMFCButton which draws properly in dark mode
class CThemeMFCButton : public CMFCButton
{
public:
	CThemeMFCButton()
		: CMFCButton()
	{
	}

	virtual void OnDraw(CDC* pDC, const CRect& rect, UINT uiState) override;
	virtual void OnFillBackground(CDC* pDC, const CRect& rectClient) override;
	virtual void OnDrawBorder(CDC* pDC, CRect& rectClient, UINT uiState) override;
};

/// a CThemeMFCMenuButton which draws properly in dark mode
class CThemeMFCMenuButton : public CMFCMenuButton
{
public:
	CThemeMFCMenuButton()
		: CMFCMenuButton()
	{
	}
	virtual void OnDraw(CDC* pDC, const CRect& rect, UINT uiState) override;
	virtual void OnDrawFocusRect(CDC* pDC, const CRect& rectClient) override;
	virtual void OnDrawBorder(CDC* pDC, CRect& rectClient, UINT uiState) override;

	virtual BOOL IsPressed() { return __super::IsPressed(); }

	void OnButtonDraw(CDC* pDC, const CRect& rect, UINT uiState);

	bool m_bNoArrow = false;
};
