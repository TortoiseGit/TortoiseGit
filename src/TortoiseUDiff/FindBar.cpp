// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007, 2012 - TortoiseSVN

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
#include "StdAfx.h"
#include "Resource.h"
#include "FindBar.h"
#include "Registry.h"
#include <string>
#include <Commdlg.h>

using namespace std;

CFindBar::CFindBar()
{
}

CFindBar::~CFindBar(void)
{
	DestroyIcon(m_hIcon);
}

LRESULT CFindBar::DlgFunc(HWND /*hwndDlg*/, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			m_hIcon = (HICON)::LoadImage(hResource, MAKEINTRESOURCE(IDI_CANCELNORMAL), IMAGE_ICON, 0, 0, LR_DEFAULTCOLOR);
			SendMessage(GetDlgItem(*this, IDC_FINDEXIT), BM_SETIMAGE, IMAGE_ICON, (LPARAM)m_hIcon);
		}
		return TRUE;
	case WM_COMMAND:
		return DoCommand(LOWORD(wParam), HIWORD(wParam));
	default:
		return FALSE;
	}
}

LRESULT CFindBar::DoCommand(int id, int msg)
{
	bool bFindPrev = false;
	switch (id)
	{
	case IDC_FINDPREV:
		bFindPrev = true;
		// fallthrough
	case IDC_FINDNEXT:
		{
			DoFind(bFindPrev);
		}
		break;
	case IDC_FINDEXIT:
		{
			::SendMessage(m_hParent, COMMITMONITOR_FINDEXIT, 0, 0);
		}
		break;
	case IDC_FINDTEXT:
		{
			if (msg == EN_CHANGE)
			{
				SendMessage(m_hParent, COMMITMONITOR_FINDRESET, 0, 0);
				DoFind(false);
			}
		}
		break;
	}
	return 1;
}

void CFindBar::DoFind(bool bFindPrev)
{
	int len = ::GetWindowTextLength(GetDlgItem(*this, IDC_FINDTEXT));
	TCHAR * findtext = new TCHAR[len+1];
	::GetWindowText(GetDlgItem(*this, IDC_FINDTEXT), findtext, len+1);
	wstring ft = wstring(findtext);
	delete [] findtext;
	const bool bCaseSensitive = !!SendMessage(GetDlgItem(*this, IDC_MATCHCASECHECK), BM_GETCHECK, 0, NULL);
	const UINT message = bFindPrev ? COMMITMONITOR_FINDMSGPREV : COMMITMONITOR_FINDMSGNEXT;
	::SendMessage(m_hParent, message, (WPARAM)bCaseSensitive, (LPARAM)ft.c_str());
}