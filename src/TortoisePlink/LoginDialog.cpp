// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003 - Tim Kemp and Stefan Kueng

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

HINSTANCE g_hmodThisDll;
HWND g_hwndMain;

class LoginDialog
{
public:
   LoginDialog(const std::string& prompt);
   
   static bool DoLoginDialog(std::string& password, const std::string& prompt);

private:
   bool myOK;
   HWND _hdlg;

   std::string  myPassword;
   std::string  myPrompt;
   
   void CreateModule(void);
   void RetrieveValues();
   
   std::string GetPassword();

   friend BOOL CALLBACK LoginDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
};


BOOL DoLoginDialog(char* password, int maxlen, const char* prompt)
{
   g_hmodThisDll = GetModuleHandle(0);
   g_hwndMain = GetParentHwnd();
   std::string passwordstr;
   BOOL res = LoginDialog::DoLoginDialog(passwordstr, prompt);
   if (res)
      strncpy(password, passwordstr.c_str(), maxlen);
   return res;
}


BOOL CALLBACK LoginDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   if (uMsg == WM_INITDIALOG)
   {
      LoginDialog* pDlg = (LoginDialog*) lParam;
      pDlg->_hdlg = hwndDlg;
      SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
      // Set prompt text
      SendDlgItemMessage(hwndDlg, IDC_LOGIN_PROMPT, WM_SETTEXT,
                         pDlg->myPrompt.length(), (LPARAM) pDlg->myPrompt.c_str());
      // Make sure edit control has the focus
      //SendDlgItemMessage(hwndDlg, IDC_LOGIN_PASSWORD, WM_SETFOCUS, 0, 0);
      if (GetDlgCtrlID((HWND) wParam) != IDC_LOGIN_PASSWORD)
      { 
         SetFocus(GetDlgItem(hwndDlg, IDC_LOGIN_PASSWORD));
         return FALSE; 
      } 
      return TRUE; 
   }
   else if (uMsg == WM_COMMAND && LOWORD(wParam) == IDCANCEL && HIWORD(wParam) == BN_CLICKED)
   {
      LoginDialog* pDlg = (LoginDialog*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
      pDlg->myOK = false;
      EndDialog(hwndDlg, IDCANCEL);
      return 1;
   }
   else if (uMsg == WM_COMMAND && LOWORD(wParam) == IDOK && HIWORD(wParam) == BN_CLICKED)
   {
      LoginDialog* pDlg = (LoginDialog*) GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
      pDlg->myOK = true;
      pDlg->RetrieveValues();
      EndDialog(hwndDlg, IDOK);
      return 1;
   }
   return 0;
}

LoginDialog::LoginDialog(const std::string& prompt)
{
   myPrompt = prompt;
}

void LoginDialog::CreateModule(void)
{
   DialogBoxParam(g_hmodThisDll, MAKEINTRESOURCE(IDD_LOGIN), g_hwndMain,
                  (DLGPROC)(LoginDialogProc), (long)this);
}


bool LoginDialog::DoLoginDialog(std::string& password, const std::string& prompt)
{
   LoginDialog *pDlg = new LoginDialog(prompt);

   pDlg->CreateModule();

   bool ret = pDlg->myOK;
   password = pDlg->myPassword;
   
   delete pDlg;

   return ret;
}


std::string LoginDialog::GetPassword()
{
   char szTxt[256];
   SendDlgItemMessage(_hdlg, IDC_LOGIN_PASSWORD, WM_GETTEXT, sizeof(szTxt), (LPARAM)szTxt);
   std::string strText = szTxt;
   return strText;
}

void LoginDialog::RetrieveValues()
{
   myPassword = GetPassword();
}


BOOL IsWinNT()
{
   OSVERSIONINFO vi;
   vi.dwOSVersionInfoSize = sizeof(vi);
   if (GetVersionEx(&vi))
   {
      if (vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
      {
         return TRUE;
      }
   }
   return FALSE;
}

HWND GetParentHwnd()
{
   if (IsWinNT())
   {
      return GetDesktopWindow();
   }
   else
   {
      return GetForegroundWindow();
   }
}
