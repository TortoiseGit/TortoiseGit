// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit
// Copyright (C) 2003-2007 - Stefan Kueng

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
#include "BaseDialog.h"
#include "LoadIconEx.h"

INT_PTR CDialog::DoModal(HINSTANCE hInstance, int resID, HWND hWndParent)
{
	hResource = hInstance;
	return DialogBoxParam(hInstance, MAKEINTRESOURCE(resID), hWndParent, &CDialog::stDlgFunc, reinterpret_cast<LPARAM>(this));
}

HWND CDialog::Create(HINSTANCE hInstance, int resID, HWND hWndParent)
{
	hResource = hInstance;
	m_hwnd = CreateDialogParam(hInstance, MAKEINTRESOURCE(resID), hWndParent, &CDialog::stDlgFunc, reinterpret_cast<LPARAM>(this));
	return m_hwnd;
}

void CDialog::InitDialog(HWND hwndDlg, UINT iconID)
{
	HWND hwndOwner;
	RECT rc, rcDlg, rcOwner;

	hwndOwner = ::GetParent(hwndDlg);
	if (!hwndOwner)
		hwndOwner = ::GetDesktopWindow();

	GetWindowRect(hwndOwner, &rcOwner);
	GetWindowRect(hwndDlg, &rcDlg);
	CopyRect(&rc, &rcOwner);

	OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
	OffsetRect(&rc, -rc.left, -rc.top);
	OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

	SetWindowPos(hwndDlg, HWND_TOP, rcOwner.left + (rc.right / 2), rcOwner.top + (rc.bottom / 2), 0, 0,	SWP_NOSIZE);
	auto hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(iconID), ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON));
	::SendMessage(hwndDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(hIcon));
	hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(iconID), ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON));
	::SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(hIcon));
}

INT_PTR CALLBACK CDialog::stDlgFunc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CDialog* pWnd;
	if (uMsg == WM_INITDIALOG)
	{
		// get the pointer to the window from lpCreateParams
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		pWnd = reinterpret_cast<CDialog*>(lParam);
		pWnd->m_hwnd = hwndDlg;
	}
	// get the pointer to the window
	pWnd = GetObjectFromWindow(hwndDlg);

	// if we have the pointer, go to the message handler of the window
	// else, use DefWindowProc
	if (pWnd)
	{
		LRESULT lRes = pWnd->DlgFunc(hwndDlg, uMsg, wParam, lParam);
		SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, lRes);
		return lRes;
	}
	else
		return DefWindowProc(hwndDlg, uMsg, wParam, lParam);
}
