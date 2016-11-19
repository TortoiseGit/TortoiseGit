// TortoiseIDiff - an image diff viewer in TortoiseSVN and TortoiseGit

// Copyright (C) 2015 - TortoiseGit
// Copyright (C) 2006-2007, 2010-2014 - TortoiseSVN

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
#include "MainWindow.h"
#include "CmdLineParser.h"
#include "registry.h"
#include "LangDll.h"
#include "TortoiseIDiff.h"
#include "TaskbarUUID.h"
#include <Shellapi.h>

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Global Variables:
HINSTANCE hInst;                                // current instance
HINSTANCE hResource;                            // the resource dll
HCURSOR   curHand;
HCURSOR   curHandDown;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE /*hPrevInstance*/,
                       LPTSTR    lpCmdLine,
                       int       /*nCmdShow*/)
{
    SetDllDirectory(L"");
    SetTaskIDPerUUID();
    CRegStdDWORD loc = CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", 1033);
    long langId = loc;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    CLangDll langDLL;
    hResource = langDLL.Init(L"TortoiseIDiff", langId);
    if (hResource == NULL)
        hResource = hInstance;

    git_libgit2_init();

    CCmdLineParser parser(lpCmdLine);

    if (parser.HasKey(L"?") || parser.HasKey(L"help"))
    {
        TCHAR buf[1024] = { 0 };
        LoadString(hResource, IDS_COMMANDLINEHELP, buf, _countof(buf));
        MessageBox(NULL, buf, L"TortoiseIDiff", MB_ICONINFORMATION);
        langDLL.Close();
        return 0;
    }


    MSG msg;
    hInst = hInstance;

    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_WIN95_CLASSES
    };
    InitCommonControlsEx(&used);

    // load the cursors we need
    curHand = (HCURSOR)LoadImage(hInst, MAKEINTRESOURCE(IDC_PANCUR), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
    curHandDown = (HCURSOR)LoadImage(hInst, MAKEINTRESOURCE(IDC_PANDOWNCUR), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);

    auto mainWindow = std::make_unique<CMainWindow>(hResource);
    mainWindow->SetRegistryPath(L"Software\\TortoiseGit\\TortoiseIDiffWindowPos");
    std::wstring leftfile = parser.HasVal(L"left") ? parser.GetVal(L"left") : L"";
    std::wstring rightfile = parser.HasVal(L"right") ? parser.GetVal(L"right") : L"";
    if ((leftfile.empty()) && (lpCmdLine[0] != 0))
    {
        int nArgs;
        LPWSTR * szArglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
        if (szArglist)
        {
            if (nArgs == 3)
            {
                // Four parameters:
                // [0]: Program name
                // [1]: left file
                // [2]: right file
                if (PathFileExists(szArglist[1]) && PathFileExists(szArglist[2]))
                {
                    leftfile = szArglist[1];
                    rightfile = szArglist[2];
                }
            }
        }

        // Free memory allocated for CommandLineToArgvW arguments.
        LocalFree(szArglist);
    }
    mainWindow->SetLeft(leftfile.c_str(), parser.HasVal(L"lefttitle") ? parser.GetVal(L"lefttitle") : L"");
    mainWindow->SetRight(rightfile.c_str(), parser.HasVal(L"righttitle") ? parser.GetVal(L"righttitle") : L"");
    if (parser.HasVal(L"base"))
        mainWindow->SetSelectionImage(FileTypeBase, parser.GetVal(L"base"), parser.HasVal(L"basetitle") ? parser.GetVal(L"basetitle") : L"");
    if (parser.HasVal(L"mine"))
        mainWindow->SetSelectionImage(FileTypeMine, parser.GetVal(L"mine"), parser.HasVal(L"minetitle") ? parser.GetVal(L"minetitle") : L"");
    if (parser.HasVal(L"theirs"))
        mainWindow->SetSelectionImage(FileTypeTheirs, parser.GetVal(L"theirs"), parser.HasVal(L"theirstitle") ? parser.GetVal(L"theirstitle") : L"");
    if (parser.HasVal(L"result"))
        mainWindow->SetSelectionResult(parser.GetVal(L"result"));
    mainWindow->resolveMsgWnd = parser.HasVal(L"resolvemsghwnd") ? (HWND)parser.GetLongLongVal(L"resolvemsghwnd") : 0;
    mainWindow->resolveMsgWParam = parser.HasVal(L"resolvemsgwparam") ? (WPARAM)parser.GetLongLongVal(L"resolvemsgwparam") : 0;
    mainWindow->resolveMsgLParam = parser.HasVal(L"resolvemsglparam") ? (LPARAM)parser.GetLongLongVal(L"resolvemsglparam") : 0;
    if (mainWindow->RegisterAndCreateWindow())
    {
        HACCEL hAccelTable = LoadAccelerators(hResource, MAKEINTRESOURCE(IDR_TORTOISEIDIFF));
        if (!parser.HasVal(L"left") && parser.HasVal(L"base") && !parser.HasVal(L"mine") && !parser.HasVal(L"theirs"))
        {
            PostMessage(*mainWindow, WM_COMMAND, ID_FILE_OPEN, 0);
        }
        if (parser.HasKey(L"overlay"))
        {
            PostMessage(*mainWindow, WM_COMMAND, ID_VIEW_OVERLAPIMAGES, 0);
        }
        if (parser.HasKey(L"fit"))
        {
            PostMessage(*mainWindow, WM_COMMAND, ID_VIEW_FITIMAGEHEIGHTS, 0);
            PostMessage(*mainWindow, WM_COMMAND, ID_VIEW_FITIMAGEWIDTHS, 0);
        }
        if (parser.HasKey(L"fitwidth"))
        {
            PostMessage(*mainWindow, WM_COMMAND, ID_VIEW_FITIMAGEWIDTHS, 0);
        }
        if (parser.HasKey(L"fitheight"))
        {
            PostMessage(*mainWindow, WM_COMMAND, ID_VIEW_FITIMAGEHEIGHTS, 0);
        }
        if (parser.HasKey(L"showinfo"))
        {
            PostMessage(*mainWindow, WM_COMMAND, ID_VIEW_IMAGEINFO, 0);
        }
        // Main message loop:
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!TranslateAccelerator(*mainWindow, hAccelTable, &msg))
            {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        return (int) msg.wParam;
    }
    langDLL.Close();
    DestroyCursor(curHand);
    DestroyCursor(curHandDown);
    CoUninitialize();
    git_libgit2_shutdown();
    return 1;
}
