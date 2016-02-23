// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2012, 2015-2016 - TortoiseSVN

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
#include <Windows.h>

class CLinkControl : public CStatic
{
public:
	CLinkControl(void);
	virtual ~CLinkControl(void);

	static const UINT LK_LINKITEMCLICKED;

protected:
	virtual void PreSubclassWindow();
	virtual BOOL OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult);

private:
	HCURSOR         m_hLinkCursor;                  // Cursor for hyperlink
	CFont           m_UnderlineFont;                // Font for underline display
	CFont           m_NormalFont;                   // Font for default display
	bool            m_bOverControl;                 // cursor over control?

	void DrawFocusRect();
	void ClearFocusRect();
	void NotifyParent(UINT msg);

protected:
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnCaptureChanged(CWnd *pWnd);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg UINT OnGetDlgCode();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnClicked();
	afx_msg void OnEnable(BOOL enabled);

	DECLARE_MESSAGE_MAP()
};

