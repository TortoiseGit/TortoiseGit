// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit
// Copyright (C) 2007 - TortoiseSVN

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
#include "basedialog.h"


/**
 * \ingroup TortoiseUDiff
 * FindBar.
 * A search bar similar to the one found in FireFox
 */
class CFindBar : public CDialog
{
public:
	CFindBar();
	~CFindBar(void);

	void					SetParent(HWND hParent) {m_hParent = hParent;}
	void					SetSearchString(LPCTSTR findStr);
	void					SelectSearchString();

protected:
	LRESULT CALLBACK		DlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	LRESULT					DoCommand(int id, int msg);

	void					DoFind(bool bFindPrev);
	void					SetTheme(bool bDark);

private:
	HWND					m_hParent;
	CAutoIcon m_hIcon;
	int m_themeCallbackId;
};
