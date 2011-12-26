// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit

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

// SshAskPass.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "SshAskPass.h"
#include <stdio.h>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);

TCHAR g_Promptphrase[] = _T("Enter your OpenSSH passphrase:");
TCHAR *g_Prompt = NULL;

TCHAR g_PassWord[MAX_LOADSTRING];

int APIENTRY _tWinMain(HINSTANCE	hInstance,
					 HINSTANCE		hPrevInstance,
					 LPTSTR			lpCmdLine,
					 int			nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);

	SetDllDirectory(L"");

	if( _tcslen(lpCmdLine) == 0 )
	{
		g_Prompt = g_Promptphrase;
	}
	else
	{
		g_Prompt = lpCmdLine;
	}

	TCHAR *yesno=_T("(yes/no)");
	size_t lens = _tcslen(yesno);
	TCHAR *p = lpCmdLine;
	BOOL bYesNo=FALSE;

	while(*p)
	{
		if (_tcsncicmp(p, yesno, lens) == 0)
		{
			bYesNo = TRUE;
			break;
		}
		p++;
	}

	if(bYesNo)
	{
		if (::MessageBox(NULL, lpCmdLine, _T("TortoiseGit - git CLI stdin wrapper"), MB_YESNO|MB_ICONQUESTION) == IDYES)
		{
			_tprintf(_T("yes"));
		}
		else
		{
			_tprintf(_T("no"));
		}
		return 0;
	}
	else
	{
		if(DialogBox(hInst, MAKEINTRESOURCE(IDD_ASK_PASSWORD), NULL, About) == IDOK)
		{
			_tprintf(_T("%s"), g_PassWord);
			return 0;
		}
		return -1;
	}
	return (int) 0;
}

// Message handler for password box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		{
			RECT rect;
			::GetWindowRect(hDlg,&rect);
			DWORD dwWidth = GetSystemMetrics(SM_CXSCREEN);
			DWORD dwHeight = GetSystemMetrics(SM_CYSCREEN);

			DWORD x,y;
			x=(dwWidth - (rect.right-rect.left))/2;
			y=(dwHeight - (rect.bottom-rect.top))/2;

			::MoveWindow(hDlg,x,y,rect.right-rect.left,rect.bottom-rect.top,TRUE);
			HWND title=::GetDlgItem(hDlg,IDC_STATIC_TITLE);
			if(g_Prompt)
				::SetWindowText(title,g_Prompt);
		}
		return (INT_PTR)TRUE;

	case WM_COMMAND:

		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			if( LOWORD(wParam) == IDOK )
			{
				HWND password = ::GetDlgItem(hDlg,IDC_PASSWORD);
				::GetWindowText(password,g_PassWord,MAX_LOADSTRING);
			}
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}
