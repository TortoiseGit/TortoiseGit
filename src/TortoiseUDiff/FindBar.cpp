// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2013, 2015-2016, 2019-2020 - TortoiseGit
// Copyright (C) 2003-2007, 2012-2013, 2018 - TortoiseSVN

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
#include "resource.h"
#include "FindBar.h"
#include "registry.h"
#include <string>
#include <Commdlg.h>
#include "LoadIconEx.h"
#include "Theme.h"

CFindBar::CFindBar()
	: m_hParent(nullptr)
	, m_themeCallbackId(0)
{
}

CFindBar::~CFindBar(void)
{
	CTheme::Instance().RemoveRegisteredCallback(m_themeCallbackId);
}

LRESULT CFindBar::DlgFunc(HWND /*hwndDlg*/, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			m_hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_CANCELNORMAL));
			SendMessage(GetDlgItem(*this, IDC_FINDEXIT), BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(static_cast<HICON>(m_hIcon)));
			m_themeCallbackId = CTheme::Instance().RegisterThemeChangeCallback([this]() { SetTheme(CTheme::Instance().IsDarkTheme()); });
			SetTheme(CTheme::Instance().IsDarkTheme());
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
	auto findtext = std::make_unique<TCHAR[]>(len + 1);
	if (!::GetWindowText(GetDlgItem(*this, IDC_FINDTEXT), findtext.get(), len + 1))
		return;
	std::wstring ft = std::wstring(findtext.get());
	const bool bCaseSensitive = !!SendMessage(GetDlgItem(*this, IDC_MATCHCASECHECK), BM_GETCHECK, 0, 0);
	const UINT message = bFindPrev ? COMMITMONITOR_FINDMSGPREV : COMMITMONITOR_FINDMSGNEXT;
	::SendMessage(m_hParent, message, bCaseSensitive, reinterpret_cast<LPARAM>(ft.c_str()));
}

void CFindBar::SelectSearchString()
{
	SendMessage(GetDlgItem(*this, IDC_FINDTEXT), EM_SETSEL, 0, -1);
}

void CFindBar::SetSearchString(LPCTSTR findStr)
{
	::SetWindowText(GetDlgItem(*this, IDC_FINDTEXT), findStr);
}

void CFindBar::SetTheme(bool bDark)
{
	CTheme::Instance().SetThemeForDialog(*this, bDark);
}
