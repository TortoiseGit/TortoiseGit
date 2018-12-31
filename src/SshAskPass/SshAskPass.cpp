// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2018 - TortoiseGit

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
#include "resource.h"
#include <propsys.h>
#include <PropKey.h>
#include "UnicodeUtils.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance

const TCHAR g_Promptphrase[] = L"Enter your OpenSSH passphrase:";
const TCHAR* g_Prompt = g_Promptphrase;

TCHAR g_PassWord[MAX_LOADSTRING];

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK PasswdDlg(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE	/*hInstance*/,
					 HINSTANCE		/*hPrevInstance*/,
					 LPTSTR			lpCmdLine,
					 int			/*nCmdShow*/)
{
	SetDllDirectory(L"");

	InitCommonControls();

	size_t cmdlineLen = wcslen(lpCmdLine);
	if (lpCmdLine[0] == '"' && cmdlineLen > 1 && lpCmdLine[cmdlineLen - 1] == '"')
	{
		lpCmdLine[cmdlineLen - 1] = L'\0';
		++lpCmdLine;
	}
	if (lpCmdLine[0] != L'\0')
		g_Prompt = lpCmdLine;

	if (StrStrI(lpCmdLine, L"(yes/no)"))
	{
		if (::MessageBox(nullptr, g_Prompt, L"TortoiseGit - git CLI stdin wrapper", MB_YESNO | MB_ICONQUESTION) == IDYES)
			wprintf(L"yes");
		else
			wprintf(L"no");
		return 0;
	}

	if (StrStrI(lpCmdLine, L"Should I try again?"))
	{
		if (::MessageBox(nullptr, g_Prompt, L"TortoiseGit - git CLI yes/no wrapper", MB_YESNO | MB_ICONQUESTION) == IDYES)
			return 0;

		return 1;
	}

	if (DialogBox(hInst, MAKEINTRESOURCE(IDD_ASK_PASSWORD), nullptr, PasswdDlg) == IDOK)
	{
		printf("%s\n", CUnicodeUtils::StdGetUTF8(g_PassWord).c_str());
		return 0;
	}
	wprintf(L"\n");
	return -1;
}

void MarkWindowAsUnpinnable(HWND hWnd)
{
	typedef HRESULT (WINAPI *SHGPSFW) (HWND hwnd,REFIID riid,void** ppv);

	HMODULE hShell = AtlLoadSystemLibraryUsingFullPath(L"Shell32.dll");

	if (hShell) {
		SHGPSFW pfnSHGPSFW = (SHGPSFW)::GetProcAddress(hShell, "SHGetPropertyStoreForWindow");
		if (pfnSHGPSFW) {
			IPropertyStore *pps;
			HRESULT hr = pfnSHGPSFW(hWnd, IID_PPV_ARGS(&pps));
			if (SUCCEEDED(hr)) {
				PROPVARIANT var;
				var.vt = VT_BOOL;
				var.boolVal = VARIANT_TRUE;
				pps->SetValue(PKEY_AppUserModel_PreventPinning, var);
				pps->Release();
			}
		}
		FreeLibrary(hShell);
	}
}

// Message handler for password box.
INT_PTR CALLBACK PasswdDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (message)
	{
	case WM_INITDIALOG:
		{
			MarkWindowAsUnpinnable(hDlg);
			RECT rect;
			::GetWindowRect(hDlg,&rect);
			DWORD dwWidth = GetSystemMetrics(SM_CXSCREEN);
			DWORD dwHeight = GetSystemMetrics(SM_CYSCREEN);

			DWORD x,y;
			x=(dwWidth - (rect.right-rect.left))/2;
			y=(dwHeight - (rect.bottom-rect.top))/2;

			::MoveWindow(hDlg,x,y,rect.right-rect.left,rect.bottom-rect.top,TRUE);
			HWND title = ::GetDlgItem(hDlg, IDC_STATIC_TITLE);
			::SetWindowText(title, g_Prompt);
			SendMessage(::GetDlgItem(hDlg, IDC_PASSWORD), EM_SETLIMITTEXT, MAX_LOADSTRING - 1, 0);
			if (!StrStrI(g_Prompt, L"pass"))
				SendMessage(::GetDlgItem(hDlg, IDC_PASSWORD), EM_SETPASSWORDCHAR, 0, 0);
			::FlashWindow(hDlg, TRUE);
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
