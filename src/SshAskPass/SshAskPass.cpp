// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2018-2020 - TortoiseGit

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
#include "SmartHandle.h"
#include <memory>
#include "DarkModeHelper.h"
#include "registry.h"

#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 256

// Global Variables:
HINSTANCE hInst;								// current instance
bool g_darkmode = false;

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

	if (StrStrI(lpCmdLine, L"(yes/no"))
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
		auto len = static_cast<int>(_tcslen(g_PassWord));
		auto size = WideCharToMultiByte(CP_UTF8, 0, g_PassWord, len, nullptr, 0, nullptr, nullptr);
		auto buf = std::make_unique<char[]>(size + 1);
		auto ret = WideCharToMultiByte(CP_UTF8, 0, g_PassWord, len, buf.get(), size, nullptr, nullptr);
		buf[ret] = '\0';
		printf("%s\n", buf.get());
		SecureZeroMemory(buf.get(), size + 1);
		SecureZeroMemory(&g_PassWord, sizeof(g_PassWord));
		return 0;
	}
	wprintf(L"\n");
	return -1;
}

void MarkWindowAsUnpinnable(HWND hWnd)
{
	typedef HRESULT (WINAPI *SHGPSFW) (HWND hwnd,REFIID riid,void** ppv);

	CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(L"Shell32.dll");

	if (hShell) {
		auto pfnSHGPSFW = reinterpret_cast<SHGPSFW>(::GetProcAddress(hShell, "SHGetPropertyStoreForWindow"));
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
	}
}

static SIZE GetTextSize(HWND hWnd, const TCHAR* str)
{
	HDC hDC = ::GetWindowDC(hWnd);
	HFONT font = reinterpret_cast<HFONT>(::SendMessage(hWnd, WM_GETFONT, 0, 0));
	HFONT oldFont = reinterpret_cast<HFONT>(::SelectObject(hDC, font));
	RECT r = { 0 };
	::DrawText(hDC, str, -1, &r, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_CALCRECT);
	::SelectObject(hDC, oldFont);

	return SIZE{ r.right, r.bottom };
}

static void MoveButton(HWND hDlg, DWORD id, const POINT& diff)
{
	RECT rect = { 0 };
	HWND button = ::GetDlgItem(hDlg, id);
	::GetWindowRect(button, &rect);
	::MapWindowPoints(nullptr, hDlg, reinterpret_cast<LPPOINT>(&rect), 2);
	::MoveWindow(button, rect.left + diff.x / 2, rect.top + diff.y, rect.right - rect.left, rect.bottom - rect.top, TRUE);
}

void SetTheme(HWND hWnd)
{
	HIGHCONTRAST hc = { sizeof(HIGHCONTRAST) };
	SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(HIGHCONTRAST), &hc, FALSE);
	bool isHighContrastMode = ((hc.dwFlags & HCF_HIGHCONTRASTON) != 0);

	g_darkmode = !isHighContrastMode && DarkModeHelper::Instance().CanHaveDarkMode() && DarkModeHelper::Instance().ShouldAppsUseDarkMode() && CRegStdDWORD(L"Software\\TortoiseGit\\DarkTheme", FALSE) == TRUE;
	if (g_darkmode)
	{
		DarkModeHelper::Instance().AllowDarkModeForApp(TRUE);
		DarkModeHelper::Instance().AllowDarkModeForWindow(hWnd, TRUE);
		SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetStockObject(BLACK_BRUSH)));
		if (FAILED(SetWindowTheme(hWnd, L"DarkMode_Explorer", nullptr)))
			SetWindowTheme(hWnd, L"Explorer", nullptr);
		BOOL darkFlag = TRUE;
		DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data = { DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag) };
		DarkModeHelper::Instance().SetWindowCompositionAttribute(hWnd, &data);
		DarkModeHelper::Instance().FlushMenuThemes();
		DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
		for (UINT id : { IDOK, IDCANCEL })
		{
			HWND ctrl = ::GetDlgItem(hWnd, id);
			if (FAILED(SetWindowTheme(ctrl, L"DarkMode_Explorer", nullptr)))
				SetWindowTheme(ctrl, L"Explorer", nullptr);
		}
	}
	else
	{
		DarkModeHelper::Instance().AllowDarkModeForApp(FALSE);
		DarkModeHelper::Instance().AllowDarkModeForWindow(hWnd, FALSE);
		SetWindowTheme(hWnd, L"Explorer", nullptr);
		BOOL darkFlag = FALSE;
		DarkModeHelper::WINDOWCOMPOSITIONATTRIBDATA data = { DarkModeHelper::WINDOWCOMPOSITIONATTRIB::WCA_USEDARKMODECOLORS, &darkFlag, sizeof(darkFlag) };
		DarkModeHelper::Instance().SetWindowCompositionAttribute(hWnd, &data);
		DarkModeHelper::Instance().FlushMenuThemes();
		DarkModeHelper::Instance().RefreshImmersiveColorPolicyState();
		DarkModeHelper::Instance().AllowDarkModeForApp(FALSE);
		SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, reinterpret_cast<LONG_PTR>(GetSysColorBrush(COLOR_3DFACE)));
		for (UINT id : { IDOK, IDCANCEL })
			SetWindowTheme(::GetDlgItem(hWnd, id), L"Explorer", nullptr);
	}

	::RedrawWindow(hWnd, nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_ERASE | RDW_INTERNALPAINT | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

// Message handler for password box.
INT_PTR CALLBACK PasswdDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
	static HBRUSH hbrBkgnd = nullptr;
	switch (message)
	{
	case WM_INITDIALOG:
		{
			MarkWindowAsUnpinnable(hDlg);
			SetTheme(hDlg);

			RECT rect;
			::GetWindowRect(hDlg,&rect);
			DWORD dwWidth = GetSystemMetrics(SM_CXSCREEN);
			DWORD dwHeight = GetSystemMetrics(SM_CYSCREEN);

			HWND title = ::GetDlgItem(hDlg, IDC_STATIC_TITLE);
			::SetWindowText(title, g_Prompt);
			RECT titleRect = { 0 };
			::GetClientRect(title, &titleRect);
			auto promptRect = GetTextSize(title, g_Prompt);
			POINT diff = { 0 };
			diff.y = max(0, promptRect.cy - titleRect.bottom);
			diff.x = max(0, promptRect.cx - titleRect.right);
			::SetWindowPos(title, nullptr, 0, 0, titleRect.right + diff.x, titleRect.bottom + diff.y, SWP_NOMOVE);

			HWND textfield = ::GetDlgItem(hDlg, IDC_PASSWORD);
			RECT textfieldRect = { 0 };
			::GetWindowRect(textfield, &textfieldRect);
			::MapWindowPoints(nullptr, hDlg, reinterpret_cast<LPPOINT>(&textfieldRect), 2);
			::MoveWindow(textfield, textfieldRect.left, textfieldRect.top + diff.y, textfieldRect.right - textfieldRect.left + diff.x, textfieldRect.bottom - textfieldRect.top, TRUE);

			MoveButton(hDlg, IDOK, diff);
			MoveButton(hDlg, IDCANCEL, diff);

			DWORD x = (dwWidth - (rect.right - rect.left + diff.x)) / 2;
			DWORD y = (dwHeight - (rect.bottom - rect.top + diff.y)) / 2;
			::MoveWindow(hDlg, x, y, rect.right - rect.left + diff.x, rect.bottom - rect.top + diff.y, TRUE);
			::SendMessage(textfield, EM_SETLIMITTEXT, MAX_LOADSTRING - 1, 0);
			if (!StrStrI(g_Prompt, L"pass"))
				::SendMessage(textfield, EM_SETPASSWORDCHAR, 0, 0);
			::FlashWindow(hDlg, TRUE);
		}
		return TRUE;

	case WM_CTLCOLORDLG:
	case WM_CTLCOLOREDIT:
	case WM_CTLCOLORSTATIC:
		if (g_darkmode)
		{
			constexpr COLORREF darkBkColor = 0x202020; // cf. THeme.h
			constexpr COLORREF darkTextColor = 0xDDDDDD;

			HDC hdc = reinterpret_cast<HDC>(wParam);
			SetTextColor(hdc, darkTextColor);
			SetBkColor(hdc, darkBkColor);
			if (!hbrBkgnd)
				hbrBkgnd = CreateSolidBrush(darkBkColor);
			return reinterpret_cast<INT_PTR>(hbrBkgnd);
		}
		break;

	case WM_DESTROY:
		if (hbrBkgnd)
		{
			::DeleteObject(hbrBkgnd);
			hbrBkgnd = nullptr;
		}
		break;

	case WM_SYSCOLORCHANGE:
		SetTheme(hDlg);
		break;

	case WM_COMMAND:

		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			HWND password = ::GetDlgItem(hDlg, IDC_PASSWORD);
			if (LOWORD(wParam) == IDOK)
				::GetWindowText(password, g_PassWord, MAX_LOADSTRING);

			// overwrite textfield contents with garbage in order to wipe the cache
			TCHAR gargabe[MAX_LOADSTRING];
			wmemset(gargabe, L'*', _countof(gargabe));
			gargabe[_countof(gargabe) - 1] = L'\0';
			::SetWindowText(password, gargabe);
			gargabe[0] = L'\0';
			::SetWindowText(password, gargabe);

			EndDialog(hDlg, LOWORD(wParam));
			return TRUE;
		}
		break;
	}
	return FALSE;
}
