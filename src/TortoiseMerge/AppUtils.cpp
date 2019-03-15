// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2010-2011, 2014-2016, 2019 - TortoiseGit
// Copyright (C) 2006-2010, 2012-2014 - TortoiseSVN

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
#include "registry.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include "SysProgressDlg.h"

#pragma warning(push)
#include "svn_pools.h"
#include "svn_io.h"
#include "svn_path.h"
#include "svn_diff.h"
#include "svn_string.h"
#include "svn_utf.h"
#pragma warning(pop)
#include "Git.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"

BOOL CAppUtils::GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CSysProgressDlg* progDlg, HWND hWnd /*=nullptr*/)
{
	CString sSCMPath = CRegString(L"Software\\TortoiseGitMerge\\SCMPath", L"");
	if (sSCMPath.IsEmpty())
	{
		// no path set, so use TortoiseGit as default
		sSCMPath = CPathUtils::GetAppDirectory() + L"TortoiseGitProc.exe";
		sSCMPath += L" /command:cat /path:\"%1\" /revision:%2 /savepath:\"%3\" /hwnd:%4";
	}
	CString sTemp;
	sTemp.Format(L"%p", static_cast<void*>(hWnd));
	sSCMPath.Replace(L"%1", sPath);
	sSCMPath.Replace(L"%2", sVersion);
	sSCMPath.Replace(L"%3", sSavePath);
	sSCMPath.Replace(L"%4", sTemp);
	// start the external SCM program to fetch the specific version of the file
	PROCESS_INFORMATION process;
	if (!CCreateProcessHelper::CreateProcess(nullptr, sSCMPath.GetBuffer(), &process))
	{
		CFormatMessageWrapper errorDetails;
		MessageBox(nullptr, errorDetails, L"TortoiseGitMerge", MB_OK | MB_ICONERROR);
		return FALSE;
	}
	DWORD ret = 0;
	do
	{
		ret = WaitForSingleObject(process.hProcess, 100);
	} while ((ret == WAIT_TIMEOUT) && (!progDlg || !progDlg->HasUserCancelled()));
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	if (progDlg && progDlg->HasUserCancelled())
	{
		return FALSE;
	}
	if (!PathFileExists(sSavePath))
		return FALSE;
	return TRUE;
}

bool CAppUtils::CreateUnifiedDiff(const CString& orig, const CString& modified, const CString& output, int contextsize, bool bShowError)
{
	CString diffContext;
	if (contextsize >= 0)
		diffContext.Format(L"--unified=%d", contextsize);
	CString cmd, err;
	cmd.Format(L"git.exe diff --no-index %s -- \"%s\" \"%s\"", static_cast<LPCTSTR>(diffContext), static_cast<LPCTSTR>(orig), static_cast<LPCTSTR>(modified));

	int result = g_Git.RunLogFile(cmd, output, &err);
	if (result != 0 && result != 1 && bShowError)
	{
		MessageBox(nullptr, L"Failed to create patch.\n" + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return false;
	}
	return true;
}

bool CAppUtils::HasClipboardFormat(UINT format)
{
	if (OpenClipboard(nullptr))
	{
		UINT enumFormat = 0;
		do
		{
			if (enumFormat == format)
			{
				CloseClipboard();
				return true;
			}
		} while((enumFormat = EnumClipboardFormats(enumFormat))!=0);
		CloseClipboard();
	}
	return false;
}

COLORREF CAppUtils::IntenseColor(long scale, COLORREF col)
{
	// if the color is already dark (gray scale below 127),
	// then lighten the color by 'scale', otherwise darken it
	int Gray = (static_cast<int>(GetRValue(col)) + GetGValue(col) + GetBValue(col)) / 3;
	if (Gray > 127)
	{
		long red   = MulDiv(GetRValue(col),(255-scale),255);
		long green = MulDiv(GetGValue(col),(255-scale),255);
		long blue  = MulDiv(GetBValue(col),(255-scale),255);

		return RGB(red, green, blue);
	}
	long R = MulDiv(255-GetRValue(col),scale,255)+GetRValue(col);
	long G = MulDiv(255-GetGValue(col),scale,255)+GetGValue(col);
	long B = MulDiv(255-GetBValue(col),scale,255)+GetBValue(col);

	return RGB(R, G, B);
}
