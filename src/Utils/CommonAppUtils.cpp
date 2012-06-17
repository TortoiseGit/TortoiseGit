// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2003-2008,2010 - TortoiseSVN

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
#include "..\Resources\LoglistCommonResource.h"
#include "CommonAppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "Registry.h"

extern CString sOrigCWD;

bool CCommonAppUtils::LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup, CString *cwd)
{
	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));

	CString cleanCommandLine(sCommandLine);

	CString theCWD = sOrigCWD;
	if (cwd != NULL)
		theCWD = *cwd;

	if (CreateProcess(NULL, const_cast<TCHAR*>((LPCTSTR)cleanCommandLine), NULL, NULL, FALSE, 0, 0, theCWD, &startup, &process)==0)
	{
		if(idErrMessageFormat != 0)
		{
			CString temp;
			temp.Format(idErrMessageFormat, CFormatMessageWrapper());
			MessageBox(NULL, temp, _T("TortoiseGit"), MB_OK | MB_ICONINFORMATION);
		}
		return false;
	}

	if (bWaitForStartup)
	{
		WaitForInputIdle(process.hProcess, 10000);
	}

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return true;
}

bool CCommonAppUtils::RunTortoiseProc(const CString& sCommandLine)
{
	CString pathToExecutable = CPathUtils::GetAppDirectory() + _T("TortoiseProc.exe");
	CString sCmd;
	sCmd.Format(_T("\"%s\" %s"), (LPCTSTR)pathToExecutable, (LPCTSTR)sCommandLine);
	if (AfxGetMainWnd()->GetSafeHwnd() && (sCommandLine.Find(L"/hwnd:") < 0))
	{
		CString sCmdLine;
		sCmdLine.Format(L"%s /hwnd:%ld", (LPCTSTR)sCommandLine, AfxGetMainWnd()->GetSafeHwnd());
		sCmd.Format(_T("\"%s\" %s"), (LPCTSTR)pathToExecutable, (LPCTSTR)sCmdLine);
	}

	return LaunchApplication(sCmd, NULL, false);
}

bool CCommonAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width /* = 128 */, int height /* = 128 */)
{
	if ((((DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\ShowListBackgroundImage"), TRUE)) == FALSE))
		return false;
	ListView_SetTextBkColor(hListCtrl, CLR_NONE);
	COLORREF bkColor = ListView_GetBkColor(hListCtrl);
	// create a bitmap from the icon
	HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), IMAGE_ICON, width, height, LR_DEFAULTCOLOR);
	if (!hIcon)
		return false;

	RECT rect = {0};
	rect.right = width;
	rect.bottom = height;
	HBITMAP bmp = NULL;

	HWND desktop = ::GetDesktopWindow();
	if (desktop)
	{
		HDC screen_dev = ::GetDC(desktop);
		if (screen_dev)
		{
			// Create a compatible DC
			HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
			if (dst_hdc)
			{
				// Create a new bitmap of icon size
				bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
				if (bmp)
				{
					// Select it into the compatible DC
					HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
					// Fill the background of the compatible DC with the given color
					::SetBkColor(dst_hdc, bkColor);
					::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

					// Draw the icon into the compatible DC
					::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);
					::SelectObject(dst_hdc, old_dst_bmp);
				}
				::DeleteDC(dst_hdc);
			}
		}
		::ReleaseDC(desktop, screen_dev);
	}

	// Restore settings
	DestroyIcon(hIcon);

	if (bmp == NULL)
		return false;

	LVBKIMAGE lv;
	lv.ulFlags = LVBKIF_TYPE_WATERMARK;
	lv.hbm = bmp;
	lv.xOffsetPercent = 100;
	lv.yOffsetPercent = 100;
	ListView_SetBkImage(hListCtrl, &lv);
	return true;
}

bool CCommonAppUtils::FileOpenSave(CString& path, int * filterindex, UINT title, UINT filter, bool bOpen, HWND hwndOwner)
{
	OPENFILENAME ofn = {0};				// common dialog box structure
	TCHAR szFile[MAX_PATH] = {0};		// buffer for file name. Explorer can't handle paths longer than MAX_PATH.
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndOwner;
	_tcscpy_s(szFile, MAX_PATH, (LPCTSTR)path);
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	CString sFilter;
	TCHAR * pszFilters = NULL;
	if (filter)
	{
		sFilter.LoadString(filter);
		pszFilters = new TCHAR[sFilter.GetLength()+4];
		_tcscpy_s (pszFilters, sFilter.GetLength()+4, sFilter);
		// Replace '|' delimiters with '\0's
		TCHAR *ptr = pszFilters + _tcslen(pszFilters);  //set ptr at the NULL
		while (ptr != pszFilters)
		{
			if (*ptr == '|')
				*ptr = '\0';
			ptr--;
		}
		ofn.lpstrFilter = pszFilters;
	}
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	CString temp;
	if (title)
	{
		temp.LoadString(title);
		CStringUtils::RemoveAccelerators(temp);
	}
	ofn.lpstrTitle = temp;
	if (bOpen)
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
	else
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;


	// Display the Open dialog box.
	bool bRet = false;
	if (bOpen)
	{
		bRet = !!GetOpenFileName(&ofn);
	}
	else
	{
		bRet = !!GetSaveFileName(&ofn);
	}
	SetCurrentDirectory(sOrigCWD.GetBuffer());
	sOrigCWD.ReleaseBuffer();
	if (bRet)
	{
		if (pszFilters)
			delete [] pszFilters;
		path = CString(ofn.lpstrFile);
		if (filterindex)
			*filterindex = ofn.nFilterIndex;
		return true;
	}
	if (pszFilters)
		delete [] pszFilters;
	return false;
}
