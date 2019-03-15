// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2015-2016 - TortoiseGit
// Copyright (C) 2006-2007, 2009, 2011-2013, 2015 - TortoiseSVN

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
#include "BaseWindow.h"
#include "PicWindow.h"
#include "TortoiseIDiff.h"

#include <map>
#include <CommCtrl.h>

#define SPLITTER_BORDER 2

#define WINDOW_MINHEIGHT 200
#define WINDOW_MINWIDTH 200

enum FileType
{
    FileTypeMine        = 1,
    FileTypeTheirs      = 2,
    FileTypeBase        = 3,
};

/**
 * \ingroup TortoiseIDiff
 * The main window of TortoiseIDiff.
 * Hosts the two image views, the menu, toolbar, slider, ...
 */
class CMainWindow : public CWindow
{
public:
    CMainWindow(HINSTANCE hInstance, const WNDCLASSEX* wcx = nullptr) : CWindow(hInstance, wcx)
        , picWindow1(hInstance)
        , picWindow2(hInstance)
        , picWindow3(hInstance)
        , oldx(-4)
        , oldy(-4)
        , bMoved(false)
        , bDragMode(false)
        , bDrag2(false)
        , nSplitterPos(100)
        , nSplitterPos2(200)
        , bOverlap(false)
        , bShowInfo(false)
        , bVertical(false)
        , bLinkedPositions(true)
        , bFitWidths(false)
        , bFitHeights(false)
        , transparentColor(::GetSysColor(COLOR_WINDOW))
        , m_BlendType(CPicWindow::BLEND_ALPHA)
        , hwndTB(0)
        , hToolbarImgList(nullptr)
        , bSelectionMode(false)
        , resolveMsgWnd(nullptr)
        , resolveMsgLParam(0)
        , resolveMsgWParam(0)
    {
        SetWindowTitle(static_cast<LPCTSTR>(ResString(hResource, IDS_APP_TITLE)));
    };

    /**
     * Registers the window class and creates the window.
     */
    bool RegisterAndCreateWindow();

    /**
     * Sets the image path and title for the left image view.
     */
    void SetLeft(const tstring& leftpath, const tstring& lefttitle) { leftpicpath = leftpath; leftpictitle = lefttitle; }
    /**
     * Sets the image path and the title for the right image view.
     */
    void SetRight(const tstring& rightpath, const tstring& righttitle) { rightpicpath = rightpath; rightpictitle = righttitle; }

    /**
     * Sets the image path and title for selection mode. In selection mode, the images
     * are shown side-by-side for the user to chose one of them. The chosen image is
     * saved at the path for \b FileTypeResult (if that path has been set) and the
     * process return value is the chosen FileType.
     */
    void SetSelectionImage(FileType ft, const std::wstring& path, const std::wstring& title);
    void SetSelectionResult(const std::wstring& path) { selectionResult = path; }

protected:
    /// the message handler for this window
    LRESULT CALLBACK                    WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    /// Handles all the WM_COMMAND window messages (e.g. menu commands)
    LRESULT                             DoCommand(int id, LPARAM lParam);

    /// Positions the child windows. Call this after the window sizes/positions have changed.
    void                                PositionChildren(RECT* clientrect = nullptr);
    /// Shows the "Open images" dialog where the user can select the images to diff
    bool                                OpenDialog();
    static BOOL CALLBACK                OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static bool                         AskForFile(HWND owner, TCHAR * path);

    // splitter methods
    void                                DrawXorBar(HDC hdc, int x1, int y1, int width, int height);
    LRESULT                             Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                             Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    LRESULT                             Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
    void                                Splitter_CaptureChanged();

    // toolbar
    bool                                CreateToolbar();
    HWND                                hwndTB;
    HIMAGELIST                          hToolbarImgList;

    // command line params
    static tstring                      leftpicpath;
    static tstring                      leftpictitle;

    static tstring                      rightpicpath;
    static tstring                      rightpictitle;

    // image data
    CPicWindow                          picWindow1;
    CPicWindow                          picWindow2;
    CPicWindow                          picWindow3;
    bool                                bShowInfo;
    COLORREF                            transparentColor;

    // splitter data
    int                                 oldx;
    int                                 oldy;
    bool                                bMoved;
    bool                                bDragMode;
    bool                                bDrag2;
    int                                 nSplitterPos;
    int                                 nSplitterPos2;

    // one/two pane view
    bool                                bSelectionMode;
    bool                                bOverlap;
    bool                                bVertical;
    bool                                bLinkedPositions;
    bool                                bFitWidths;
    bool                                bFitHeights;
    CPicWindow::BlendType               m_BlendType;

    // selection mode data
    std::map<FileType, std::wstring>    selectionPaths;
    std::map<FileType, std::wstring>    selectionTitles;
    std::wstring                        selectionResult;

public:
    HWND			resolveMsgWnd;
    WPARAM			resolveMsgWParam;
    LPARAM			resolveMsgLParam;
};

