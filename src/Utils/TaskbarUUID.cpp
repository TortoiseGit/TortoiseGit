// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2019 - TortoiseGit
// Copyright (C) 2011-2012, 2016 - TortoiseSVN

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
#pragma once
#include "stdafx.h"
#include "TaskbarUUID.h"
#include "registry.h"
#include "CmdLineParser.h"
#include "LoadIconEx.h"
#include <Shobjidl.h>
#include "SmartHandle.h"
#include <atlbase.h>
#pragma warning(push)
#pragma warning(disable: 4458)
#include <GdiPlus.h>
#pragma warning(pop)
#pragma comment(lib, "gdiplus.lib")

#define APPID (L"TGIT.TGIT.1")

void SetTaskIDPerUUID()
{
	typedef HRESULT STDAPICALLTYPE SetCurrentProcessExplicitAppUserModelIDFN(PCWSTR AppID);
	CAutoLibrary hShell = AtlLoadSystemLibraryUsingFullPath(L"shell32.dll");
	if (hShell)
	{
		auto pfnSetCurrentProcessExplicitAppUserModelID = reinterpret_cast<SetCurrentProcessExplicitAppUserModelIDFN*>(GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID"));
		if (pfnSetCurrentProcessExplicitAppUserModelID)
		{
			std::wstring id = GetTaskIDPerUUID();
			pfnSetCurrentProcessExplicitAppUserModelID(id.c_str());
		}
	}
}

std::wstring GetTaskIDPerUUID(LPCTSTR uuid /*= nullptr */)
{
	CRegStdDWORD r = CRegStdDWORD(L"Software\\TortoiseGit\\GroupTaskbarIconsPerRepo", 3);
	std::wstring id = APPID;
	if ((r < 2)||(r == 3))
	{
		wchar_t buf[MAX_PATH] = {0};
		GetModuleFileName(nullptr, buf, _countof(buf));
		std::wstring n = buf;
		n = n.substr(n.find_last_of('\\'));
		id += n;
	}

	if (r >= 3)
	{
		if (uuid)
		{
			id += uuid;
		}
		else
		{
			CCmdLineParser parser(GetCommandLine());
			if (parser.HasVal(L"groupuuid"))
			{
				id += parser.GetVal(L"groupuuid");
			}
		}
	}
	return id;
}

#ifdef __AFXWIN_H__
extern CString g_sGroupingUUID;
extern CString g_sGroupingIcon;
extern bool g_bGroupingRemoveIcon;
#endif

void SetUUIDOverlayIcon( HWND hWnd )
{
	if (!CRegStdDWORD(L"Software\\TortoiseGit\\GroupTaskbarIconsPerRepo", 3))
		return;

	if (!CRegStdDWORD(L"Software\\TortoiseGit\\GroupTaskbarIconsPerRepoOverlay", TRUE))
		return;

	std::wstring uuid;
	std::wstring sicon;
	bool bRemoveicon = false;
#ifdef __AFXWIN_H__
	uuid = g_sGroupingUUID;
	sicon = g_sGroupingIcon;
	bRemoveicon = g_bGroupingRemoveIcon;
#else
	CCmdLineParser parser(GetCommandLine());
	if (parser.HasVal(L"groupuuid"))
		uuid = parser.GetVal(L"groupuuid");
#endif
	if (uuid.empty())
		return;

	CComPtr<ITaskbarList3> pTaskbarInterface;
	if (FAILED(pTaskbarInterface.CoCreateInstance(CLSID_TaskbarList, nullptr, CLSCTX_INPROC_SERVER)))
		return;

	int foundUUIDIndex = 0;
	do
	{
		wchar_t buf[MAX_PATH] = { 0 };
		swprintf_s(buf, L"%s%d", L"Software\\TortoiseGit\\LastUsedUUIDsForGrouping\\", foundUUIDIndex);
		CRegStdString r = CRegStdString(buf);
		std::wstring sr = r;
		if (sr.empty())
		{
			r = uuid + (sicon.empty() ? L"" : (L";" + sicon));
			break;
		}
		size_t sep = sr.find(L';');
		std::wstring olduuid = sep != std::wstring::npos ? sr.substr(0, sep) : sr;
		if (olduuid.compare(uuid) == 0)
		{
			if (bRemoveicon)
				r = uuid; // reset icon path in registry
			else if (!sicon.empty())
				r = uuid + (sicon.empty() ? L"" : (L";" + sicon));
			else
				sicon = sep != std::wstring::npos ? sr.substr(sep + 1) : L"";
			break;
		}
		foundUUIDIndex++;
	} while (foundUUIDIndex < 20);
	if (foundUUIDIndex >= 20)
	{
		CRegStdString r = CRegStdString(L"Software\\TortoiseGit\\LastUsedUUIDsForGrouping\\1");
		r.removeKey();
	}

	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);

	HICON icon = nullptr;
	if (!sicon.empty())
	{
		if (sicon.size() >= 4 && !_wcsicmp(sicon.substr(sicon.size() - 4).c_str(), L".ico"))
			icon = LoadIconEx(nullptr, sicon.c_str(), iconWidth, iconHeight);
		else
		{
			ULONG_PTR gdiplusToken = 0;
			Gdiplus::GdiplusStartupInput gdiplusStartupInput;
			GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
			if (gdiplusToken)
			{
				{
				auto pBitmap = std::make_unique<Gdiplus::Bitmap>(sicon.c_str(), FALSE);
				if (pBitmap->GetLastStatus() == Gdiplus::Status::Ok)
					pBitmap->GetHICON(&icon);
				}
				Gdiplus::GdiplusShutdown(gdiplusToken);
			}
		}
	}

	if (!icon)
	{
		DWORD colors[6] = { 0x80FF0000, 0x80FFFF00, 0x8000FF00, 0x800000FF, 0x80000000, 0x8000FFFF };

		// AND mask - monochrome - determines which pixels get drawn
		auto AND = std::make_unique<BYTE[]>(iconWidth * iconWidth);
		for (int i = 0; i < iconWidth * iconWidth; ++i)
			AND[i] = 0xff;

		// XOR mask - 32bpp ARGB - determines the pixel values
		auto XOR = std::make_unique<DWORD[]>(iconWidth * iconWidth);
		for (int i = 0; i < iconWidth * iconWidth; ++i)
			XOR[i] = colors[foundUUIDIndex % 6];

		icon = ::CreateIcon(nullptr, iconWidth, iconHeight, 1, 32, AND.get(), reinterpret_cast<BYTE*>(XOR.get()));
	}
	pTaskbarInterface->SetOverlayIcon(hWnd, icon, uuid.c_str());
	DestroyIcon(icon);
}
