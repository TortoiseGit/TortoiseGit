// TortoiseIDiff - an image diff viewer in TortoiseSVN and TortoiseGit

// Copyright (C) 2006 - 2007, 2010-2011 - TortoiseSVN

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
#include "mainWindow.h"
#include "CmdLineParser.h"
#include "registry.h"
#include "LangDll.h"
#include "TortoiseIDiff.h"
#include "TaskbarUUID.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Global Variables:
HINSTANCE hInst;                                // current instance
HINSTANCE hResource;                            // the resource dll
HCURSOR   curHand;
HCURSOR   curHandDown;

int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);

    SetDllDirectory(L"");
    SetTaskIDPerUUID();
    CRegStdDWORD loc = CRegStdDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
    long langId = loc;

    CLangDll langDLL;
    hResource = langDLL.Init(_T("TortoiseIDiff"), langId);
    if (hResource == NULL)
        hResource = hInstance;

    CCmdLineParser parser(lpCmdLine);

    if (parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
    {
        TCHAR buf[1024];
        LoadString(hResource, IDS_COMMANDLINEHELP, buf, _countof(buf));
        MessageBox(NULL, buf, _T("TortoiseIDiff"), MB_ICONINFORMATION);
        langDLL.Close();
        return 0;
    }


    MSG msg;
    HACCEL hAccelTable;

    hInst = hInstance;

    INITCOMMONCONTROLSEX used = {
        sizeof(INITCOMMONCONTROLSEX),
        ICC_STANDARD_CLASSES | ICC_BAR_CLASSES | ICC_WIN95_CLASSES
    };
    InitCommonControlsEx(&used);

    // load the cursors we need
    curHand = (HCURSOR)LoadImage(hInst, MAKEINTRESOURCE(IDC_PANCUR), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);
    curHandDown = (HCURSOR)LoadImage(hInst, MAKEINTRESOURCE(IDC_PANDOWNCUR), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);

    CMainWindow mainWindow(hResource);
    mainWindow.SetRegistryPath(_T("Software\\TortoiseGit\\TortoiseIDiffWindowPos"));
    std::wstring leftfile = parser.HasVal(_T("left")) ? parser.GetVal(_T("left")) : _T("");
    if ((leftfile.size() == 0)&&(lpCmdLine[0] != 0))
    {
        leftfile = lpCmdLine;
    }
    mainWindow.SetLeft(leftfile.c_str(), parser.HasVal(_T("lefttitle")) ? parser.GetVal(_T("lefttitle")) : _T(""));
    mainWindow.SetRight(parser.HasVal(_T("right")) ? parser.GetVal(_T("right")) : _T(""), parser.HasVal(_T("righttitle")) ? parser.GetVal(_T("righttitle")) : _T(""));
    if (mainWindow.RegisterAndCreateWindow())
    {
        hAccelTable = LoadAccelerators(hResource, MAKEINTRESOURCE(IDR_TORTOISEIDIFF));
        if (!parser.HasVal(_T("left")))
        {
            PostMessage(mainWindow, WM_COMMAND, ID_FILE_OPEN, 0);
        }
        if (parser.HasKey(_T("overlay")))
        {
            PostMessage(mainWindow, WM_COMMAND, ID_VIEW_OVERLAPIMAGES, 0);
        }
        if (parser.HasKey(_T("fit")))
        {
            PostMessage(mainWindow, WM_COMMAND, ID_VIEW_FITTOGETHER, 0);
        }
        if (parser.HasKey(_T("showinfo")))
        {
            PostMessage(mainWindow, WM_COMMAND, ID_VIEW_IMAGEINFO, 0);
        }
        // Main message loop:
        while (GetMessage(&msg, NULL, 0, 0))
        {
            if (!TranslateAccelerator(mainWindow, hAccelTable, &msg))
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
    return 1;
}







