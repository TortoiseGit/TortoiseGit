// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2020, 2023-2025 - TortoiseGit
// Copyright (C) 2003-2008, 2010, 2020 - TortoiseSVN

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
#include "CommonAppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "FormatMessageWrapper.h"
#include "registry.h"
#include "SelectFileFilter.h"
#include "DPIAware.h"
#include "LoadIconEx.h"
#include "IconBitmapUtils.h"
#include "CreateProcessHelper.h"

extern CString g_sGroupingUUID;

bool CCommonAppUtils::LaunchApplication(const CString& sCommandLine, const LaunchApplicationFlags& flags)
{
	LPCWSTR theCWD = nullptr;
	if (flags.psCWD)
		theCWD = flags.psCWD;

	if (flags.bUAC)
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
				if (flags.uiIDErrMessageFormat != 0)
				{
					CString temp;
					temp.Format(flags.uiIDErrMessageFormat, static_cast<LPCWSTR>(CFormatMessageWrapper()));
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
			if (flags.uiIDErrMessageFormat != 0)
			{
				CString temp;
				temp.Format(flags.uiIDErrMessageFormat, static_cast<LPCWSTR>(CFormatMessageWrapper()));
				MessageBox(nullptr, temp, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
			}
			return false;
		}

		CAutoGeneralHandle piProcess(std::move(shellinfo.hProcess));
		if (piProcess)
		{
			if (flags.bWaitForStartup)
				WaitForInputIdle(piProcess, 10000);

			if (flags.bWaitForExit)
			{
				DWORD count = 1;
				HANDLE handles[2];
				handles[0] = piProcess;
				if (flags.hWaitHandle)
				{
					count = 2;
					handles[1] = flags.hWaitHandle;
				}
				WaitForMultipleObjects(count, handles, FALSE, INFINITE);
				if (flags.hWaitHandle)
					CloseHandle(flags.hWaitHandle);

				if (flags.pdwExitCode)
				{
					if (!GetExitCodeProcess(piProcess, flags.pdwExitCode))
						return false;
				}
			}
		}

		return true;
	}

	STARTUPINFO startup = { 0 };
	PROCESS_INFORMATION process = { 0 };
	startup.cb = sizeof(startup);

	CString cleanCommandLine(sCommandLine);
	if (CreateProcess(nullptr, cleanCommandLine.GetBuffer(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, theCWD, &startup, &process) == 0)
	{
		if (flags.uiIDErrMessageFormat)
		{
			CString temp;
			temp.Format(flags.uiIDErrMessageFormat, static_cast<LPCWSTR>(CFormatMessageWrapper()));
			MessageBox(nullptr, temp, L"TortoiseGit", MB_OK | MB_ICONINFORMATION);
		}
		return false;
	}

	CAutoGeneralHandle piThread(std::move(process.hThread));
	CAutoGeneralHandle piProcess(std::move(process.hProcess));

	AllowSetForegroundWindow(process.dwProcessId);

	if (flags.bWaitForStartup)
		WaitForInputIdle(piProcess, 10000);

	if (flags.bWaitForExit)
	{
		DWORD count = 1;
		HANDLE handles[2];
		handles[0] = piProcess;
		if (flags.hWaitHandle)
		{
			count = 2;
			handles[1] = flags.hWaitHandle;
		}
		WaitForMultipleObjects(count, handles, FALSE, INFINITE);
		if (flags.hWaitHandle)
			CloseHandle(flags.hWaitHandle);

		if (flags.pdwExitCode)
		{
			if (!GetExitCodeProcess(piProcess, flags.pdwExitCode))
				return false;
		}
	}

	return true;
}

bool CCommonAppUtils::RunTortoiseGitProc(const CString& sCommandLine, bool uac, bool includeGroupingUUID)
{
	CString pathToExecutable = CPathUtils::GetAppDirectory() + L"TortoiseGitProc.exe";
	CString sCmd;
	sCmd.Format(L"\"%s\" %s", static_cast<LPCWSTR>(pathToExecutable), static_cast<LPCWSTR>(sCommandLine));
	if (AfxGetMainWnd()->GetSafeHwnd() && (sCommandLine.Find(L"/hwnd:") < 0))
		sCmd.AppendFormat(L" /hwnd:%p", static_cast<void*>(AfxGetMainWnd()->GetSafeHwnd()));
	if (!g_sGroupingUUID.IsEmpty() && includeGroupingUUID)
	{
		sCmd += L" /groupuuid:\"";
		sCmd += g_sGroupingUUID;
		sCmd += L'"';
	}

	return LaunchApplication(sCmd, LaunchApplicationFlags().UAC(uac));
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
	return SetListCtrlBackgroundImage(hListCtrl, nID, CDPIAware::Instance().ScaleX(hListCtrl, 128), CDPIAware::Instance().ScaleY(hListCtrl, 128));
}

bool CCommonAppUtils::SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width, int height)
{
	if (static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\ShowListBackgroundImage", TRUE)) == FALSE)
		return false;
	// create a bitmap from the icon
	CAutoIcon hIcon = ::LoadIconEx(AfxGetResourceHandle(), MAKEINTRESOURCE(nID), width, height);
	if (!hIcon)
		return false;

	LVBKIMAGE lv;
	lv.ulFlags = LVBKIF_TYPE_WATERMARK | LVBKIF_FLAG_ALPHABLEND;
	lv.hbm = IconBitmapUtils::IconToBitmapPARGB32(hIcon, width, height);
	lv.xOffsetPercent = 100;
	lv.yOffsetPercent = 100;
	ListView_SetBkImage(hListCtrl, &lv);
	return true;
}

bool CCommonAppUtils::FileOpenSave(CString& path, int* filterindex, UINT title, UINT filterId, bool bOpen, HWND hwndOwner, LPCWSTR defaultExt, bool handleAsFile)
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

CString CCommonAppUtils::GetLogFontName()
{
	return CRegString(L"Software\\TortoiseGit\\LogFontName", L"Consolas");
}

DWORD CCommonAppUtils::GetLogFontSize()
{
	return CRegDWORD(L"Software\\TortoiseGit\\LogFontSize", 9);
}

void CCommonAppUtils::CreateFontForLogs(HWND hWnd, CFont& fontToCreate)
{
	LOGFONT logFont;
	HDC hScreenDC = ::GetDC(nullptr);
	logFont.lfHeight = -CDPIAware::Instance().PointsToPixelsY(hWnd, GetLogFontSize());
	::ReleaseDC(nullptr, hScreenDC);
	logFont.lfWidth = 0;
	logFont.lfEscapement = 0;
	logFont.lfOrientation = 0;
	logFont.lfWeight = FW_NORMAL;
	logFont.lfItalic = 0;
	logFont.lfUnderline = 0;
	logFont.lfStrikeOut = 0;
	logFont.lfCharSet = DEFAULT_CHARSET;
	logFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	logFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	logFont.lfQuality = DRAFT_QUALITY;
	logFont.lfPitchAndFamily = FF_DONTCARE | FIXED_PITCH;
	wcsncpy_s(logFont.lfFaceName, static_cast<LPCWSTR>(GetLogFontName()), _TRUNCATE);
	VERIFY(fontToCreate.CreateFontIndirect(&logFont));
}

const char* CCommonAppUtils::GetResourceData(const wchar_t* resName, int id, DWORD& resLen)
{
	resLen = 0;
	auto hResource = FindResource(nullptr, MAKEINTRESOURCE(id), resName);
	if (!hResource)
		return nullptr;
	auto hResourceLoaded = LoadResource(nullptr, hResource);
	if (!hResourceLoaded)
		return nullptr;
	auto lpResLock = static_cast<const char*>(LockResource(hResourceLoaded));
	resLen = SizeofResource(nullptr, hResource);
	return lpResLock;
}

bool CCommonAppUtils::StartHtmlHelp(DWORD_PTR id, CString page /* = L"index.html" */)
{
	ATLASSERT(page == L"index.html" || id == 0);

#if !defined(IDR_HELPMAPPING) || !defined(IDS_APPNAME)
	UNREFERENCED_PARAMETER(id);
	UNREFERENCED_PARAMETER(page);
	return false;
#else
	static std::map<DWORD_PTR, std::wstring> idMap;

	if (idMap.empty())
	{
		DWORD resSize = 0;
		const char* resData = GetResourceData(L"help", IDR_HELPMAPPING, resSize);
		if (resData)
		{
			auto resString = std::string(resData, resSize);
			std::vector<std::string> lines;
			stringtok(lines, resString, true, "\r\n");
			for (const auto& line : lines)
			{
				if (line.empty())
					continue;
				std::vector<std::string> lineParts;
				stringtok(lineParts, line, true, "=");
				if (lineParts.size() == 2)
					idMap[std::stoi(lineParts[0], nullptr, 0)] = CUnicodeUtils::StdGetUnicode(lineParts[1]);
			}
		}
	}

	if (idMap.find(id) != idMap.end())
	{
		page = idMap[id].c_str();
		page.Replace(L"@", L"%40");
	}

	CString appName(MAKEINTRESOURCE(IDS_APPNAME));
	if (CString appHelp = CPathUtils::GetAppDirectory() + appName + L"_en\\"; PathFileExists(appHelp))
	{
		// We have to find the default browser, ourselves, because ShellExecute cannot handle anchors on local HTML files
		DWORD dwszBuffPathLen = MAX_PATH;
		if (CString application; SUCCEEDED(::AssocQueryStringW(ASSOCF_IS_PROTOCOL, ASSOCSTR_COMMAND, L"http", L"open", CStrBuf(application, dwszBuffPathLen), &dwszBuffPathLen)))
		{
			if (application.Find(L"%1") < 0)
				application += L" %1";

			if (application.Find(L"\"%1\"") >= 0)
				application.Replace(L"\"%1\"", L"%1");

			appHelp.Replace(L" ", L"%20");
			appHelp.Replace(L'\\', L'/');
			application.Replace(L"%1", L"file:///" + appHelp + page); // if the path is not prefixed with file:///, the anchor separator may not be detected correctly
			return CCreateProcessHelper::CreateProcessDetached(nullptr, application);
		}
	}
	CString url;
	url.Format(L"https://tortoisegit.org/docs/%s/%s", static_cast<LPCWSTR>(appName.MakeLower()), static_cast<LPCWSTR>(page));
	return reinterpret_cast<INT_PTR>(ShellExecute(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL)) > 32;
#endif
}

int CCommonAppUtils::ExploreTo(HWND hwnd, CString path)
{
	if (PathFileExists(path))
	{
		HRESULT ret = E_FAIL;
		ITEMIDLIST __unaligned* pidl = ILCreateFromPath(path);
		if (pidl)
		{
			ret = SHOpenFolderAndSelectItems(pidl, 0, 0, 0);
			ILFree(pidl);
		}
		return SUCCEEDED(ret) ? 0 : -1;
	}
	// if filepath does not exist any more, navigate to closest matching folder
	do
	{
		const int pos = path.ReverseFind(L'\\');
		if (pos <= 3)
			break;
		path.Truncate(pos);
	} while (!PathFileExists(path));
	return reinterpret_cast<INT_PTR>(ShellExecute(hwnd, L"explore", path, nullptr, nullptr, SW_SHOW)) > 32 ? 0 : -1;
}

