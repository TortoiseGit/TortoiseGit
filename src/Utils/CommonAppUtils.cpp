// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
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
#include "stdafx.h"
#include "../Resources/LoglistCommonResource.h"
#include "CommonAppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "FormatMessageWrapper.h"
#include "registry.h"
#include "SelectFileFilter.h"
#include "DPIAware.h"
#include "LoadIconEx.h"

extern CString sOrigCWD;
extern CString g_sGroupingUUID;

bool CCommonAppUtils::LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup, CString *cwd, bool uac)
{
	CString theCWD = sOrigCWD;
	if (cwd)
		theCWD = *cwd;

	if (uac)
	{
		CString file, param;
		SHELLEXECUTEINFO shellinfo = { 0 };
		shellinfo.cbSize = sizeof(shellinfo);
		shellinfo.lpVerb = L"runas";
		shellinfo.nShow = SW_SHOWNORMAL;
		shellinfo.fMask = SEE_MASK_NOCLOSEPROCESS;
		shellinfo.lpDirectory = theCWD;

		int pos = sCommandLine.Find('"');
		if (pos == 0)
		{
			pos = sCommandLine.Find('"', 2);
			if (pos > 1)
			{
				file = sCommandLine.Mid(1, pos - 1);
				param = sCommandLine.Mid(pos + 1);
			}
			else
			{
				if (idErrMessageFormat != 0)
				{
					CString temp;
					temp.Format(idErrMessageFormat, static_cast<LPCTSTR>(CFormatMessageWrapper()));
					MessageBox(nullptr, temp, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
				}
				return false;
			}
		}
		else
		{
			pos = sCommandLine.Find(' ', 1);
			if (pos > 0)
			{
				file = sCommandLine.Left(pos);
				param = sCommandLine.Mid(pos + 1);
			}
			else
				file = sCommandLine;
		}

		shellinfo.lpFile = file;
		shellinfo.lpParameters = param;

		if (!ShellExecuteEx(&shellinfo))
		{
			if (idErrMessageFormat != 0)
			{
				CString temp;
				temp.Format(idErrMessageFormat, static_cast<LPCTSTR>(CFormatMessageWrapper()));
				MessageBox(nullptr, temp, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
			}
			return false;
		}

		if (shellinfo.hProcess)
		{
			if (bWaitForStartup)
				WaitForInputIdle(shellinfo.hProcess, 10000);

			CloseHandle(shellinfo.hProcess);
		}

		return true;
	}

	STARTUPINFO startup = { 0 };
	PROCESS_INFORMATION process = { 0 };
	startup.cb = sizeof(startup);

	CString cleanCommandLine(sCommandLine);
	if (CreateProcess(nullptr, cleanCommandLine.GetBuffer(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, theCWD, &startup, &process) == 0)
	{
		if (idErrMessageFormat)
		{
			CString temp;
			temp.Format(idErrMessageFormat, static_cast<LPCTSTR>(CFormatMessageWrapper()));
			MessageBox(nullptr, temp, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
		}
		return false;
	}

	AllowSetForegroundWindow(process.dwProcessId);

	if (bWaitForStartup)
		WaitForInputIdle(process.hProcess, 10000);

	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);

	return true;
}

bool CCommonAppUtils::RunTortoiseGitProc(const CString& sCommandLine, bool uac, bool includeGroupingUUID)
{
	CString pathToExecutable = CPathUtils::GetAppDirectory() + L"TortoiseGitProc.exe";
	CString sCmd;
	sCmd.Format(L"\"%s\" %s", static_cast<LPCTSTR>(pathToExecutable), static_cast<LPCTSTR>(sCommandLine));
	if (AfxGetMainWnd()->GetSafeHwnd() && (sCommandLine.Find(L"/hwnd:") < 0))
		sCmd.AppendFormat(L" /hwnd:%p", static_cast<void*>(AfxGetMainWnd()->GetSafeHwnd()));
	if (!g_sGroupingUUID.IsEmpty() && includeGroupingUUID)
	{
		sCmd += L" /groupuuid:\"";
		sCmd += g_sGroupingUUID;
		sCmd += L'"';
	}

	return LaunchApplication(sCmd, NULL, false, nullptr, uac);
}

bool CCommonAppUtils::IsAdminLogin()
{
	SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
	PSID administratorsGroup;
	// Initialize SID.
	if (!AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &administratorsGroup))
		return false;
	SCOPE_EXIT { FreeSid(administratorsGroup); };

	// Check whether the token is present in admin group.
	BOOL isInAdminGroup = FALSE;
	if (!CheckTokenMembership(nullptr, administratorsGroup, &isInAdminGroup))
		return false;

	return !!isInAdminGroup;
}

bool CCommonAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID)
{
	return SetListCtrlBackgroundImage(hListCtrl, nID, CDPIAware::Instance().ScaleX(128), CDPIAware::Instance().ScaleY(128));
}

bool CCommonAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width, int height)
{
	if (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\ShowListBackgroundImage", TRUE)) == FALSE)
		return false;
	ListView_SetTextBkColor(hListCtrl, CLR_NONE);
	COLORREF bkColor = ListView_GetBkColor(hListCtrl);
	// create a bitmap from the icon
	auto hIcon = ::LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), width, height);
	if (!hIcon)
		return false;
	SCOPE_EXIT { DestroyIcon(hIcon); };

	RECT rect = {0};
	rect.right = width;
	rect.bottom = height;

	HWND desktop = ::GetDesktopWindow();
	if (!desktop)
		return false;

	HDC screen_dev = ::GetDC(desktop);
	if (!screen_dev)
		return false;
	SCOPE_EXIT { ::ReleaseDC(desktop, screen_dev); };

	// Create a compatible DC
	HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
	if (!dst_hdc)
		return false;
	SCOPE_EXIT { ::DeleteDC(dst_hdc); };

	// Create a new bitmap of icon size
	HBITMAP bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
	if (!bmp)
		return false;

	// Select it into the compatible DC
	auto old_dst_bmp = static_cast<HBITMAP>(::SelectObject(dst_hdc, bmp));
	// Fill the background of the compatible DC with the given color
	::SetBkColor(dst_hdc, bkColor);
	::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);

	// Draw the icon into the compatible DC
	::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, nullptr, DI_NORMAL);
	::SelectObject(dst_hdc, old_dst_bmp);

	LVBKIMAGE lv;
	lv.ulFlags = LVBKIF_TYPE_WATERMARK;
	lv.hbm = bmp;
	lv.xOffsetPercent = 100;
	lv.yOffsetPercent = 100;
	ListView_SetBkImage(hListCtrl, &lv);
	return true;
}

bool CCommonAppUtils::FileOpenSave(CString& path, int* filterindex, UINT title, UINT filterId, bool bOpen, HWND hwndOwner, LPCTSTR defaultExt, bool handleAsFile)
{
	// Create a new common save file dialog
	CComPtr<IFileDialog> pfd;

	if (!SUCCEEDED(pfd.CoCreateInstance(bOpen ? CLSID_FileOpenDialog : CLSID_FileSaveDialog, nullptr, CLSCTX_INPROC_SERVER)))
		return false;

	// Set the dialog options
	DWORD dwOptions;
	if (!SUCCEEDED(pfd->GetOptions(&dwOptions)))
		return false;

	if (bOpen)
	{
		if (!SUCCEEDED(pfd->SetOptions(dwOptions | FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST)))
			return false;
	}
	else
	{
		if (!SUCCEEDED(pfd->SetOptions(dwOptions | FOS_OVERWRITEPROMPT | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST)))
			return false;
	}
	if ((!PathIsDirectory(path) || handleAsFile) && !SUCCEEDED(pfd->SetFileName(CPathUtils::GetFileNameFromPath(path))))
		return false;

	// Set a title
	if (title)
	{
		CString temp;
		temp.LoadString(title);
		CStringUtils::RemoveAccelerators(temp);
		pfd->SetTitle(temp);
	}
	CComPtr<IShellItem> psiDefaultFolder;
	if (filterId)
	{
		CSelectFileFilter fileFilter(filterId);
		if (!SUCCEEDED(pfd->SetFileTypes(fileFilter.GetCount(), fileFilter)))
			return false;
		if (filterId == 1602 || filterId == 2501) // IDS_GITEXEFILEFILTER || IDS_PROGRAMSFILEFILTER
		{
			pfd->SetClientGuid({ 0x323ca4b0, 0x62df, 0x4a08, { 0xa5, 0x5, 0x58, 0xde, 0xa2, 0xb9, 0x2d, 0xcd } });
			if (SUCCEEDED(SHCreateItemFromParsingName(CPathUtils::GetProgramsDirectory(), nullptr, IID_PPV_ARGS(&psiDefaultFolder))))
				pfd->SetDefaultFolder(psiDefaultFolder);
		}
		else if (filterId == 1120) // IDS_PUTTYKEYFILEFILTER
		{
			pfd->SetClientGuid({ 0x271dbd3b, 0x50da, 0x4148, { 0x95, 0xfd, 0x64, 0x73, 0x69, 0xd1, 0x74, 0x2 } });
			if (SUCCEEDED(SHCreateItemFromParsingName(CPathUtils::GetDocumentsDirectory(), nullptr, IID_PPV_ARGS(&psiDefaultFolder))))
				pfd->SetDefaultFolder(psiDefaultFolder);
		}
	}

	if (defaultExt && !SUCCEEDED(pfd->SetDefaultExtension(defaultExt)))
			return false;

	// set the default folder
	CComPtr<IShellItem> psiFolder;
	if (CStringUtils::StartsWith(path, L"\\") || path.Mid(1, 2) == L":\\")
	{
		CString dir = path;
		if (!PathIsDirectory(dir) || handleAsFile)
		{
			if (PathRemoveFileSpec(dir.GetBuffer()))
				dir.ReleaseBuffer();
			else
				dir.Empty();
		}
		if (!dir.IsEmpty() && SUCCEEDED(SHCreateItemFromParsingName(dir, nullptr, IID_PPV_ARGS(&psiFolder))))
			pfd->SetFolder(psiFolder);
	}

	// Show the save/open file dialog
	if (!SUCCEEDED(pfd->Show(hwndOwner)))
		return false;

	// Get the selection from the user
	CComPtr<IShellItem> psiResult;
	if (!SUCCEEDED(pfd->GetResult(&psiResult)))
		return false;

	CComHeapPtr<WCHAR> pszPath;
	if (!SUCCEEDED(psiResult->GetDisplayName(SIGDN_FILESYSPATH, &pszPath)))
		return false;

	path = CString(pszPath);
	if (filterindex)
	{
		UINT fi = 0;
		pfd->GetFileTypeIndex(&fi);
		*filterindex = fi;
	}
	return true;
}

HICON CCommonAppUtils::LoadIconEx(UINT resourceId, UINT cx, UINT cy)
{
	return ::LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(resourceId), cx, cy);
}

void CCommonAppUtils::SetCharFormat(CWnd* window, DWORD mask , DWORD effects, const std::vector<CHARRANGE>& positions)
{
	CHARFORMAT2 format = {};
	format.cbSize = sizeof(CHARFORMAT2);
	format.dwMask = mask;
	format.dwEffects = effects;
	format.crTextColor = effects;

	for (const auto& range : positions)
	{
		window->SendMessage(EM_EXSETSEL, 0, reinterpret_cast<LPARAM>(&range));
		window->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
	}
}

void CCommonAppUtils::SetCharFormat(CWnd* window, DWORD mask, DWORD effects )
{
	CHARFORMAT2 format = {};
	format.cbSize = sizeof(CHARFORMAT2);
	format.dwMask = mask;
	format.dwEffects = effects;
	window->SendMessage(EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format));
}
