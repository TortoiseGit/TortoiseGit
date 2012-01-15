// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011 - TortoiseSVN

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
#include "StdAfx.h"
#include "TaskbarUUID.h"
#include "registry.h"
#include "CmdLineParser.h"

#include <Shobjidl.h>
#include "Win7.h"
#include "SmartHandle.h"

#define APPID (_T("TGIT.TGIT.1"))


void SetTaskIDPerUUID()
{
    typedef HRESULT STDAPICALLTYPE SetCurrentProcessExplicitAppUserModelIDFN(PCWSTR AppID);
    CAutoLibrary hShell = ::LoadLibrary(_T("shell32.dll"));
    if (hShell)
    {
        SetCurrentProcessExplicitAppUserModelIDFN *pfnSetCurrentProcessExplicitAppUserModelID = (SetCurrentProcessExplicitAppUserModelIDFN*)GetProcAddress(hShell, "SetCurrentProcessExplicitAppUserModelID");
        if (pfnSetCurrentProcessExplicitAppUserModelID)
        {
            std::wstring id = GetTaskIDPerUUID();
            pfnSetCurrentProcessExplicitAppUserModelID(id.c_str());
        }
    }
}

std::wstring GetTaskIDPerUUID(LPCTSTR uuid /*= NULL */)
{
    CRegStdDWORD r = CRegStdDWORD(_T("Software\\TortoiseGit\\GroupTaskbarIconsPerRepo"), 0);
    std::wstring id = APPID;
    if ((r < 2)||(r == 3))
    {
        wchar_t buf[MAX_PATH] = {0};
        GetModuleFileName(NULL, buf, MAX_PATH);
        std::wstring n = buf;
        n = n.substr(n.find_last_of('\\'));
        id += n;
    }

    if (r)
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

#ifdef _MFC_VER
extern CString g_sGroupingUUID;
#endif

void SetUUIDOverlayIcon( HWND hWnd )
{
    if (CRegStdDWORD(_T("Software\\TortoiseGit\\GroupTaskbarIconsPerRepo"), 0))
    {
        if (CRegStdDWORD(_T("Software\\TortoiseGit\\GroupTaskbarIconsPerRepoOverlay"), FALSE))
        {
            std::wstring uuid;
#ifdef _MFC_VER
            uuid = g_sGroupingUUID;
#else
            CCmdLineParser parser(GetCommandLine());
            if (parser.HasVal(L"groupuuid"))
                uuid = parser.GetVal(L"groupuuid");
#endif
            if (!uuid.empty())
            {
                ITaskbarList3 * pTaskbarInterface = NULL;
                HRESULT hr = CoCreateInstance(CLSID_TaskbarList, NULL, CLSCTX_INPROC_SERVER, IID_ITaskbarList3, reinterpret_cast<void**> (&(pTaskbarInterface)));

                if (SUCCEEDED(hr))
                {
                    int foundUUIDIndex = 0;
                    do
                    {
                        wchar_t buf[MAX_PATH];
                        swprintf_s(buf, _countof(buf), L"%s%d", L"Software\\TortoiseGit\\LastUsedUUIDsForGrouping\\", foundUUIDIndex);
                        CRegStdString r = CRegStdString(buf);
                        std::wstring sr = r;
                        if (sr.empty() || (sr.compare(uuid)==0))
                        {
                            r = uuid;
                            break;
                        }
                        foundUUIDIndex++;
                    } while (foundUUIDIndex < 20);
                    if (foundUUIDIndex >= 20)
                    {
                        CRegStdString r = CRegStdString(L"Software\\TortoiseGit\\LastUsedUUIDsForGrouping\\1");
                        r.removeKey();
                    }

                    DWORD colors[6] = {0x80FF0000, 0x80FFFF00, 0x8000FF00, 0x800000FF, 0x80000000, 0x8000FFFF};

                    // AND mask - monochrome - determines which pixels get drawn
                    BYTE AND[32];
                    for( int i=0; i<32; i++ )
                    {
                        AND[i] = 0xFF;
                    }

                    // XOR mask - 32bpp ARGB - determines the pixel values
                    DWORD XOR[256];
                    for( int i=0; i<256; i++ )
                    {
                        XOR[i] = colors[foundUUIDIndex % 6];
                    }

                    HICON icon = ::CreateIcon(NULL,16,16,1,32,AND,(BYTE*)XOR);
                    pTaskbarInterface->SetOverlayIcon(hWnd, icon, uuid.c_str());
                    pTaskbarInterface->Release();
                    DestroyIcon(icon);
                }
            }
        }
    }
}
