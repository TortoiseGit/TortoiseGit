// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018-2019 - TortoiseGit
// Copyright (C) 2003, 2013, 2018 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

#include "LoginDialog.h"
#include "TortoisePlinkRes.h"
#include <string>
#include <memory>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

HINSTANCE g_hmodThisDll;
HWND g_hwndMain;

class LoginDialog {
public:
	LoginDialog(const std::string& prompt);
	~LoginDialog();

	static bool DoLoginDialog(char* password, int maxlen, const char* prompt);

private:
	bool myOK;
	HWND _hdlg;

	char myPassword[MAX_LENGTH_PASSWORD];
	std::string myPrompt;

	void CreateModule(void);
	void RetrieveValues();
	void PurgeValues();

	friend BOOL CALLBACK LoginDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

BOOL DoLoginDialog(char* password, int maxlen, const char* prompt)
{
	g_hmodThisDll = GetModuleHandle(0);
	g_hwndMain = GetParentHwnd();
	return LoginDialog::DoLoginDialog(password, maxlen, prompt);
}

BOOL CALLBACK LoginDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		auto pDlg = reinterpret_cast<LoginDialog*>(lParam);
		pDlg->_hdlg = hwndDlg;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		// Set prompt text
		SendDlgItemMessage(hwndDlg, IDC_LOGIN_PROMPT, WM_SETTEXT, pDlg->myPrompt.length(), reinterpret_cast<LPARAM>(pDlg->myPrompt.c_str()));
		SendDlgItemMessage(hwndDlg, IDC_LOGIN_PASSWORD, EM_SETLIMITTEXT, MAX_LENGTH_PASSWORD - 1, 0);
		// Make sure edit control has the focus
		//SendDlgItemMessage(hwndDlg, IDC_LOGIN_PASSWORD, WM_SETFOCUS, 0, 0);
		if (GetDlgCtrlID(reinterpret_cast<HWND>(wParam)) != IDC_LOGIN_PASSWORD)
		{
			SetFocus(GetDlgItem(hwndDlg, IDC_LOGIN_PASSWORD));
			return FALSE;
		}
		return TRUE;
	}
	else if (uMsg == WM_COMMAND && LOWORD(wParam) == IDCANCEL && HIWORD(wParam) == BN_CLICKED)
	{
		auto pDlg = reinterpret_cast<LoginDialog*>(GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
		pDlg->myOK = false;
		pDlg->PurgeValues();
		EndDialog(hwndDlg, IDCANCEL);
		return 1;
	}
	else if (uMsg == WM_COMMAND && LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED)
	{
		auto pDlg = reinterpret_cast<LoginDialog*>(GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
		pDlg->myOK = true;
		pDlg->RetrieveValues();
		pDlg->PurgeValues();
		EndDialog(hwndDlg, IDOK);
		return 1;
	}
	return 0;
}

LoginDialog::LoginDialog(const std::string& prompt)
{
	myPrompt = prompt;
	SecureZeroMemory(&myPassword, sizeof(myPassword));
}

LoginDialog::~LoginDialog()
{
	SecureZeroMemory(&myPassword, sizeof(myPassword));
}

void LoginDialog::CreateModule(void)
{
	DialogBoxParam(g_hmodThisDll, MAKEINTRESOURCE(IDD_LOGIN), g_hwndMain, (DLGPROC)(LoginDialogProc), reinterpret_cast<LPARAM>(this));
}

bool LoginDialog::DoLoginDialog(char* password, int maxlen, const char* prompt)
{
	auto pDlg = std::make_unique<LoginDialog>(prompt);

	pDlg->CreateModule();

	bool ret = pDlg->myOK;

	if (ret)
		strncpy_s(password, maxlen, pDlg->myPassword, sizeof(pDlg->myPassword));

	return ret;
}

void LoginDialog::RetrieveValues()
{
	SendDlgItemMessage(_hdlg, IDC_LOGIN_PASSWORD, WM_GETTEXT, sizeof(myPassword), reinterpret_cast<LPARAM>(myPassword));
}

void LoginDialog::PurgeValues()
{
	// overwrite textfield contents with garbage in order to wipe the cache
	char gargabe[MAX_LENGTH_PASSWORD];
	memset(gargabe, L'*', sizeof(gargabe));
	gargabe[sizeof(gargabe) - 1] = '\0';
	SendDlgItemMessage(_hdlg, IDC_LOGIN_PASSWORD, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(gargabe));
	gargabe[0] = '\0';
	SendDlgItemMessage(_hdlg, IDC_LOGIN_PASSWORD, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(gargabe));
}

HWND GetParentHwnd()
{
	return GetDesktopWindow();
}
