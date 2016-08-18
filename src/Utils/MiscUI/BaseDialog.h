// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit
// Copyright (C) 2003-2007, 2010, 2012-2014 - TortoiseSVN

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
#include <string>


/**
 * \ingroup Utils
 * A base window class.
 * Provides separate window message handlers for every window object based on
 * this class.
 */
class CDialog
{
public:
	INT_PTR DoModal(HINSTANCE hInstance, int resID, HWND hWndParent);
	HWND	Create(HINSTANCE hInstance, int resID, HWND hWndParent);

	virtual LRESULT CALLBACK DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	operator HWND() const {return m_hwnd;}
protected:
	HINSTANCE hResource;
	HWND m_hwnd;

	void InitDialog(HWND hwndDlg, UINT iconID);

	// the real message handler
	static INT_PTR CALLBACK stDlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// returns a pointer the dialog (stored as the WindowLong)
	inline static CDialog * GetObjectFromWindow(HWND hWnd)
	{
		return reinterpret_cast<CDialog*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}
};

