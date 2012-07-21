// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2012 - TortoiseSVN

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
#include "windows.h"

class CLinkControl
{
public:
	CLinkControl(void);
	virtual ~CLinkControl(void);

	bool ConvertStaticToLink(HWND hwndCtl);
	bool ConvertStaticToLink(HWND hwndParent, UINT uiCtlId);

	static const UINT LK_LINKITEMCLICKED;

protected:
private:
	static HCURSOR  g_hLinkCursor;                  // Cursor for hyperlink
	static HFONT    g_UnderlineFont;                // Font for underline display
	static HFONT    g_NormalFont;                   // Font for default display
	static int      g_counter;                      // Global resources user counter
	bool            m_bOverControl;                 // cursor over control?
	bool            m_bMouseDownPressed;            // was WM_LBUTTONDOWN received?
	HFONT           m_StdFont;                      // Standard font
	WNDPROC         m_pfnOrigCtlProc;

	void createUnderlineFont(void);
	static void createLinkCursor(void);
	void createGlobalResources(void)
	{
		createUnderlineFont();
		createLinkCursor();
	}
	static void destroyGlobalResources(void)
	{
		/*
		 * No need to call DestroyCursor() for cursors acquired through
		 * LoadCursor().
		 */
		g_hLinkCursor   = NULL;
		DeleteObject(g_UnderlineFont);
		g_UnderlineFont = NULL;
	}

	static void DrawFocusRect(HWND hwnd);
	static LRESULT CALLBACK _HyperlinkParentProc(HWND hwnd, UINT message,
												 WPARAM wParam, LPARAM lParam);
	static LRESULT CALLBACK _HyperlinkProc(HWND hwnd, UINT message,
										   WPARAM wParam, LPARAM lParam);
};

