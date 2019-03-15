// TortoiseGitIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2015-2019 - TortoiseGit
// Copyright (C) 2006-2013, 2015, 2018 - TortoiseSVN

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
#include <CommCtrl.h>
#include <Commdlg.h>
#include "TortoiseIDiff.h"
#include "MainWindow.h"
#include "AboutDlg.h"
#include "TaskbarUUID.h"
#include "PathUtils.h"
#include "DPIAware.h"
#include "LoadIconEx.h"

#pragma comment(lib, "comctl32.lib")

tstring CMainWindow::leftpicpath;
tstring CMainWindow::leftpictitle;

tstring CMainWindow::rightpicpath;
tstring CMainWindow::rightpictitle;

const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

bool CMainWindow::RegisterAndCreateWindow()
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with default parameters
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = CWindow::stWinMsgHandler;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hResource;
    wcx.hCursor = LoadCursor(nullptr, IDC_SIZEWE);
    ResString clsname(hResource, IDS_APP_TITLE);
    wcx.lpszClassName = clsname;
    wcx.hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEIDIFF), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_3DFACE + 1);
    if (selectionPaths.empty())
        wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
    else
        wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF2);
    wcx.hIconSm = LoadIconEx(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
    if (RegisterWindow(&wcx))
    {
        if (Create(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, nullptr))
        {
            UpdateWindow(m_hwnd);
            return true;
        }
    }
    return false;
}

void CMainWindow::PositionChildren(RECT * clientrect /* = nullptr */)
{
    RECT tbRect;
    if (!clientrect)
        return;
    const auto splitter_border = CDPIAware::Instance().ScaleX(SPLITTER_BORDER);
    SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
    GetWindowRect(hwndTB, &tbRect);
    LONG tbHeight = tbRect.bottom-tbRect.top-1;
    HDWP hdwp = BeginDeferWindowPos(3);
    if (bOverlap && selectionPaths.empty())
    {
        SetWindowPos(picWindow1, nullptr, clientrect->left, clientrect->top + tbHeight, clientrect->right - clientrect->left, clientrect->bottom - clientrect->top - tbHeight, SWP_SHOWWINDOW);
    }
    else
    {
        if (bVertical)
        {
            if (selectionPaths.size() != 3)
            {
                // two image windows
                RECT child;
                child.left = clientrect->left;
                child.top = clientrect->top+tbHeight;
                child.right = clientrect->right;
                child.bottom = nSplitterPos - (splitter_border / 2);
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.top = nSplitterPos + (splitter_border / 2);
                child.bottom = clientrect->bottom;
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
            else
            {
                // three image windows
                RECT child;
                child.left = clientrect->left;
                child.top = clientrect->top+tbHeight;
                child.right = clientrect->right;
                child.bottom = nSplitterPos - (splitter_border / 2);
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.top = nSplitterPos + (splitter_border / 2);
                child.bottom = nSplitterPos2 - (splitter_border / 2);
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.top = nSplitterPos2 + (splitter_border / 2);
                child.bottom = clientrect->bottom;
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow3, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
        }
        else
        {
            if (selectionPaths.size() != 3)
            {
                // two image windows
                RECT child;
                child.left = clientrect->left;
                child.top = clientrect->top+tbHeight;
                child.right = nSplitterPos - (splitter_border / 2);
                child.bottom = clientrect->bottom;
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.left = nSplitterPos + (splitter_border / 2);
                child.right = clientrect->right;
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
            else
            {
                // three image windows
                RECT child;
                child.left = clientrect->left;
                child.top = clientrect->top+tbHeight;
                child.right = nSplitterPos - (splitter_border / 2);
                child.bottom = clientrect->bottom;
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.left = nSplitterPos + (splitter_border / 2);
                child.right = nSplitterPos2 - (splitter_border / 2);
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
                child.left = nSplitterPos2 + (splitter_border / 2);
                child.right = clientrect->right;
                if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow3, nullptr, child.left, child.top, child.right - child.left, child.bottom - child.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
            }
        }
    }
    if (hdwp) EndDeferWindowPos(hdwp);
    picWindow1.SetTransparentColor(transparentColor);
    picWindow2.SetTransparentColor(transparentColor);
    picWindow3.SetTransparentColor(transparentColor);
    InvalidateRect(*this, nullptr, FALSE);
}

LRESULT CALLBACK CMainWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == TaskBarButtonCreated)
    {
        SetUUIDOverlayIcon(hwnd);
    }
    switch (uMsg)
    {
    case WM_CREATE:
        {
            m_hwnd = hwnd;
            picWindow1.RegisterAndCreateWindow(hwnd);
            picWindow2.RegisterAndCreateWindow(hwnd);
            if (selectionPaths.empty())
            {
                picWindow1.SetPic(leftpicpath, leftpictitle, true);
                picWindow2.SetPic(rightpicpath, rightpictitle, false);

                picWindow1.SetOtherPicWindow(&picWindow2);
                picWindow2.SetOtherPicWindow(&picWindow1);
            }
            else
            {
                picWindow3.RegisterAndCreateWindow(hwnd);

                picWindow1.SetPic(selectionPaths[FileTypeMine], selectionTitles[FileTypeMine], false);
                picWindow2.SetPic(selectionPaths[FileTypeBase], selectionTitles[FileTypeBase], false);
                picWindow3.SetPic(selectionPaths[FileTypeTheirs], selectionTitles[FileTypeTheirs], false);
            }

            picWindow1.SetSelectionMode(!selectionPaths.empty());
            picWindow2.SetSelectionMode(!selectionPaths.empty());
            picWindow3.SetSelectionMode(!selectionPaths.empty());

            CreateToolbar();
            // center the splitter
            RECT rect;
            GetClientRect(hwnd, &rect);
            if (selectionPaths.size() != 3)
            {
                nSplitterPos = (rect.right-rect.left)/2;
                nSplitterPos2 = 0;
            }
            else
            {
                nSplitterPos = (rect.right-rect.left)/3;
                nSplitterPos2 = (rect.right-rect.left)*2/3;
            }

            PositionChildren(&rect);
            picWindow1.FitImageInWindow();
            picWindow2.FitImageInWindow();
            picWindow3.FitImageInWindow();
        }
        break;
    case WM_COMMAND:
        {
            return DoCommand(LOWORD(wParam), lParam);
        }
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            RECT rect;

            ::GetClientRect(*this, &rect);
            hdc = BeginPaint(hwnd, &ps);
            SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
            ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
            EndPaint(hwnd, &ps);
        }
        break;
    case WM_GETMINMAXINFO:
        {
            auto mmi = reinterpret_cast<MINMAXINFO*>(lParam);
            mmi->ptMinTrackSize.x = WINDOW_MINWIDTH;
            mmi->ptMinTrackSize.y = WINDOW_MINHEIGHT;
            return 0;
        }
        break;
    case WM_SIZE:
        {
            RECT rect;
            GetClientRect(hwnd, &rect);
            if (bVertical)
            {
                RECT tbRect;
                GetWindowRect(hwndTB, &tbRect);
                LONG tbHeight = tbRect.bottom-tbRect.top-1;
                if (selectionPaths.size() != 3)
                {
                    nSplitterPos = (rect.bottom-rect.top)/2+tbHeight;
                    nSplitterPos2 = 0;
                }
                else
                {
                    nSplitterPos = (rect.bottom-rect.top)/3+tbHeight;
                    nSplitterPos2 = (rect.bottom-rect.top)*2/3+tbHeight;
                }
            }
            else
            {
                if (selectionPaths.size() != 3)
                {
                    nSplitterPos = (rect.right-rect.left)/2;
                    nSplitterPos2 = 0;
                }
                else
                {
                    nSplitterPos = (rect.right-rect.left)/3;
                    nSplitterPos2 = (rect.right-rect.left)*2/3;
                }
            }
            PositionChildren(&rect);
        }
        break;
    case WM_SETCURSOR:
        {
            if (reinterpret_cast<HWND>(wParam) == *this)
            {
                RECT rect;
                POINT pt;
                GetClientRect(*this, &rect);
                GetCursorPos(&pt);
                ScreenToClient(*this, &pt);
                if (PtInRect(&rect, pt))
                {
                    if (bVertical)
                    {
                        HCURSOR hCur = LoadCursor(nullptr, IDC_SIZENS);
                        SetCursor(hCur);
                    }
                    else
                    {
                        HCURSOR hCur = LoadCursor(nullptr, IDC_SIZEWE);
                        SetCursor(hCur);
                    }
                    return TRUE;
                }
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        break;
    case WM_LBUTTONDOWN:
        Splitter_OnLButtonDown(hwnd, uMsg, wParam, lParam);
        break;
    case WM_LBUTTONUP:
        Splitter_OnLButtonUp(hwnd, uMsg, wParam, lParam);
        break;
    case WM_CAPTURECHANGED:
        Splitter_CaptureChanged();
        break;
    case WM_MOUSEMOVE:
        Splitter_OnMouseMove(hwnd, uMsg, wParam, lParam);
        break;
    case WM_MOUSEWHEEL:
        {
            // find out if the mouse cursor is over one of the views, and if
            // it is, pass the mouse wheel message to that view
            POINT pt;
            DWORD ptW = GetMessagePos();
            pt.x = GET_X_LPARAM(ptW);
            pt.y = GET_Y_LPARAM(ptW);
            RECT rect;
            GetWindowRect(picWindow1, &rect);
            if (PtInRect(&rect, pt))
            {
                picWindow1.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
            }
            else
            {
                GetWindowRect(picWindow2, &rect);
                if (PtInRect(&rect, pt))
                {
                    picWindow2.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
                }
                else
                {
                    GetWindowRect(picWindow3, &rect);
                    if (PtInRect(&rect, pt))
                    {
                        picWindow3.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
                    }
                }
            }
        }
        break;
    case WM_MOUSEHWHEEL:
        {
            // find out if the mouse cursor is over one of the views, and if
            // it is, pass the mouse wheel message to that view
            POINT pt;
            DWORD ptW = GetMessagePos();
            pt.x = GET_X_LPARAM(ptW);
            pt.y = GET_Y_LPARAM(ptW);
            RECT rect;
            GetWindowRect(picWindow1, &rect);
            if (PtInRect(&rect, pt))
            {
                picWindow1.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam)|MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
            }
            else
            {
                GetWindowRect(picWindow2, &rect);
                if (PtInRect(&rect, pt))
                {
                    picWindow2.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam)|MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
                }
                else
                {
                    GetWindowRect(picWindow3, &rect);
                    if (PtInRect(&rect, pt))
                    {
                        picWindow3.OnMouseWheel(GET_KEYSTATE_WPARAM(wParam)|MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
                    }
                }
            }
        }
        break;
    case WM_NOTIFY:
        {
            auto pNMHDR = reinterpret_cast<LPNMHDR>(lParam);
            if (pNMHDR->code == TTN_GETDISPINFO)
            {
                auto lpttt = reinterpret_cast<LPTOOLTIPTEXT>(lParam);
                lpttt->hinst = hResource;

                // Specify the resource identifier of the descriptive
                // text for the given button.
                TCHAR stringbuf[MAX_PATH] = {0};
                MENUITEMINFO mii;
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_TYPE;
                mii.dwTypeData = stringbuf;
                mii.cch = _countof(stringbuf);
                GetMenuItemInfo(GetMenu(*this), static_cast<UINT>(lpttt->hdr.idFrom), FALSE, &mii);
                wcscpy_s(lpttt->lpszText, 80, stringbuf);
            }
        }
        break;
    case WM_DESTROY:
        bWindowClosed = TRUE;
        PostQuitMessage(0);
        break;
    case WM_CLOSE:
        ImageList_Destroy(hToolbarImgList);
        ::DestroyWindow(m_hwnd);
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
};

LRESULT CMainWindow::DoCommand(int id, LPARAM lParam)
{
    switch (id)
    {
    case ID_FILE_OPEN:
        {
            if (OpenDialog())
            {
                picWindow1.SetPic(leftpicpath, L"", true);
                picWindow2.SetPic(rightpicpath, L"", false);
                if (bOverlap)
                {
                    picWindow1.SetSecondPic(picWindow2.GetPic(), rightpictitle, rightpicpath);
                }
                else
                {
                    picWindow1.SetSecondPic();
                }
                RECT rect;
                GetClientRect(*this, &rect);
                PositionChildren(&rect);
                picWindow1.FitImageInWindow();
                picWindow2.FitImageInWindow();
            }
        }
        break;
    case ID_VIEW_IMAGEINFO:
        {
            bShowInfo = !bShowInfo;
            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bShowInfo ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_IMAGEINFO, uCheck);

            picWindow1.ShowInfo(bShowInfo);
            picWindow2.ShowInfo(bShowInfo);
            picWindow3.ShowInfo(bShowInfo);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bShowInfo ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_IMAGEINFO, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
    case ID_VIEW_OVERLAPIMAGES:
        {
            bOverlap = !bOverlap;
            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bOverlap ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_OVERLAPIMAGES, uCheck);
            uCheck |= ((m_BlendType == CPicWindow::BLEND_ALPHA) && bOverlap) ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_BLENDALPHA, uCheck);
            UINT uEnabled = MF_BYCOMMAND;
            uEnabled |= bOverlap ? MF_ENABLED : MF_DISABLED | MF_GRAYED;
            EnableMenuItem(hMenu, ID_VIEW_BLENDALPHA, uEnabled);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bOverlap ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_OVERLAPIMAGES, reinterpret_cast<LPARAM>(&tbi));

            tbi.fsState = ((m_BlendType == CPicWindow::BLEND_ALPHA) && bOverlap) ? TBSTATE_CHECKED : 0;
            if (bOverlap)
                tbi.fsState |= TBSTATE_ENABLED;
            else
                tbi.fsState = 0;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_BLENDALPHA, reinterpret_cast<LPARAM>(&tbi));

            if (bOverlap)
                tbi.fsState = 0;
            else
                tbi.fsState = bVertical ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_ARRANGEVERTICAL, reinterpret_cast<LPARAM>(&tbi));

            if (bOverlap)
            {
                bLinkedPositions = true;
                picWindow1.LinkPositions(bLinkedPositions);
                picWindow2.LinkPositions(bLinkedPositions);
                tbi.fsState = TBSTATE_CHECKED;
            }
            else
                tbi.fsState = bLinkedPositions ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_LINKIMAGESTOGETHER, reinterpret_cast<LPARAM>(&tbi));

            ShowWindow(picWindow2, bOverlap ? SW_HIDE : SW_SHOW);

            if (bOverlap)
            {
                picWindow1.StopTimer();
                picWindow2.StopTimer();
                picWindow1.SetSecondPic(picWindow2.GetPic(), rightpictitle, rightpicpath,
                    picWindow2.GetHPos(), picWindow2.GetVPos());
                picWindow1.SetBlendAlpha(m_BlendType, 0.5f);
            }
            else
            {
                picWindow1.SetSecondPic();
            }
            picWindow1.SetOverlapMode(bOverlap);
            picWindow2.SetOverlapMode(bOverlap);


            RECT rect;
            GetClientRect(*this, &rect);
            PositionChildren(&rect);

            return 0;
        }
        break;
    case ID_VIEW_BLENDALPHA:
        {
            if (m_BlendType == CPicWindow::BLEND_ALPHA)
                m_BlendType = CPicWindow::BLEND_XOR;
            else
                m_BlendType = CPicWindow::BLEND_ALPHA;

            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= ((m_BlendType == CPicWindow::BLEND_ALPHA) && bOverlap) ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_BLENDALPHA, uCheck);
            UINT uEnabled = MF_BYCOMMAND;
            uEnabled |= bOverlap ? MF_ENABLED : MF_DISABLED | MF_GRAYED;
            EnableMenuItem(hMenu, ID_VIEW_BLENDALPHA, uEnabled);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = ((m_BlendType == CPicWindow::BLEND_ALPHA) && bOverlap) ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_BLENDALPHA, reinterpret_cast<LPARAM>(&tbi));
            picWindow1.SetBlendAlpha(m_BlendType, picWindow1.GetBlendAlpha());
            PositionChildren();
        }
        break;
    case ID_VIEW_TRANSPARENTCOLOR:
        {
            static COLORREF customColors[16] = {0};
            CHOOSECOLOR ccDlg = { 0 };
            ccDlg.lStructSize = sizeof(ccDlg);
            ccDlg.hwndOwner = m_hwnd;
            ccDlg.rgbResult = transparentColor;
            ccDlg.lpCustColors = customColors;
            ccDlg.Flags = CC_RGBINIT | CC_FULLOPEN;
            if(ChooseColor(&ccDlg))
            {
                transparentColor = ccDlg.rgbResult;
                picWindow1.SetTransparentColor(transparentColor);
                picWindow2.SetTransparentColor(transparentColor);
                picWindow3.SetTransparentColor(transparentColor);
                // The color picker takes the focus and we don't get it back.
                ::SetFocus(picWindow1);
            }
        }
        break;
    case ID_VIEW_FITIMAGEWIDTHS:
        {
            bFitWidths = !bFitWidths;
            picWindow1.FitWidths(bFitWidths);
            picWindow2.FitWidths(bFitWidths);
            picWindow3.FitWidths(bFitWidths);

            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bFitWidths ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_FITIMAGEWIDTHS, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bFitWidths ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_FITIMAGEWIDTHS, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
    case ID_VIEW_FITIMAGEHEIGHTS:
        {
            bFitHeights = !bFitHeights;
            picWindow1.FitHeights(bFitHeights);
            picWindow2.FitHeights(bFitHeights);
            picWindow3.FitHeights(bFitHeights);

            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bFitHeights ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_FITIMAGEHEIGHTS, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bFitHeights ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_FITIMAGEHEIGHTS, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
    case ID_VIEW_LINKIMAGESTOGETHER:
        {
            bLinkedPositions = !bLinkedPositions;
            picWindow1.LinkPositions(bLinkedPositions);
            picWindow2.LinkPositions(bLinkedPositions);
            picWindow3.LinkPositions(bLinkedPositions);

            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bLinkedPositions ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_LINKIMAGESTOGETHER, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bLinkedPositions ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_LINKIMAGESTOGETHER, reinterpret_cast<LPARAM>(&tbi));
        }
        break;
    case ID_VIEW_ALPHA0:
        picWindow1.SetBlendAlpha(m_BlendType, 0.0f);
        break;
    case ID_VIEW_ALPHA255:
        picWindow1.SetBlendAlpha(m_BlendType, 1.0f);
        break;
    case ID_VIEW_ALPHA127:
        picWindow1.SetBlendAlpha(m_BlendType, 0.5f);
        break;
    case ID_VIEW_ALPHATOGGLE:
        picWindow1.ToggleAlpha();
        break;
    case ID_VIEW_FITIMAGESINWINDOW:
        {
            picWindow1.FitImageInWindow();
            picWindow2.FitImageInWindow();
            picWindow3.FitImageInWindow();
        }
        break;
    case ID_VIEW_ORININALSIZE:
        {
            picWindow1.SetZoom(100, false);
            picWindow2.SetZoom(100, false);
            picWindow3.SetZoom(100, false);
            picWindow1.CenterImage();
            picWindow2.CenterImage();
            picWindow3.CenterImage();
        }
        break;
    case ID_VIEW_ZOOMIN:
        {
            picWindow1.Zoom(true, false);
            if ((!(bFitWidths || bFitHeights))&&(!bOverlap))
            {
                picWindow2.Zoom(true, false);
                picWindow3.Zoom(true, false);
            }
        }
        break;
    case ID_VIEW_ZOOMOUT:
        {
            picWindow1.Zoom(false, false);
            if ((!(bFitWidths || bFitHeights))&&(!bOverlap))
            {
                picWindow2.Zoom(false, false);
                picWindow3.Zoom(false, false);
            }
        }
        break;
    case ID_VIEW_ARRANGEVERTICAL:
        {
            bVertical = !bVertical;
            RECT rect;
            GetClientRect(*this, &rect);
            if (bVertical)
            {
                RECT tbRect;
                GetWindowRect(hwndTB, &tbRect);
                LONG tbHeight = tbRect.bottom-tbRect.top-1;
                if (selectionPaths.size() != 3)
                {
                    nSplitterPos = (rect.bottom-rect.top)/2+tbHeight;
                    nSplitterPos2 = 0;
                }
                else
                {
                    nSplitterPos = (rect.bottom-rect.top)/3+tbHeight;
                    nSplitterPos2 = (rect.bottom-rect.top)*2/3+tbHeight;
                }
            }
            else
            {
                if (selectionPaths.size() != 3)
                {
                    nSplitterPos = (rect.right-rect.left)/2;
                    nSplitterPos2 = 0;
                }
                else
                {
                    nSplitterPos = (rect.right-rect.left)/3;
                    nSplitterPos2 = (rect.right-rect.left)*2/3;
                }
            }
            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bVertical ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_ARRANGEVERTICAL, uCheck);
            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bVertical ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_ARRANGEVERTICAL, reinterpret_cast<LPARAM>(&tbi));

            PositionChildren(&rect);
        }
        break;
    case ID_ABOUT:
        {
            CAboutDlg dlg(*this);
            dlg.DoModal(hInst, IDD_ABOUT, *this);
        }
        break;
    case SELECTBUTTON_ID:
        {
            auto hSource = reinterpret_cast<HWND>(lParam);
            FileType resolveWith;
            if (picWindow1 == hSource)
                resolveWith = FileTypeMine;
            else if (picWindow2 == hSource)
                resolveWith = FileTypeBase;
            else if (picWindow3 == hSource)
                resolveWith = FileTypeTheirs;
            else
                break;

            if (selectionResult.empty())
            {
                PostQuitMessage(resolveWith);
                break;
            }

            CopyFile(selectionPaths[resolveWith].c_str(), selectionResult.c_str(), FALSE);

            CAutoBuf projectRoot;
            if (git_repository_discover(projectRoot, CUnicodeUtils::GetUTF8(selectionResult.c_str()), FALSE, nullptr) < 0 && strstr(projectRoot->ptr, "/.git/"))
            {
                PostQuitMessage(resolveWith);
                break;
            }

            CAutoRepository repository(projectRoot->ptr);
            if (!repository)
            {
                PostQuitMessage(resolveWith);
                break;
            }

            CStringA subpath = CUnicodeUtils::GetUTF8(selectionResult.c_str()).Mid(static_cast<int>(strlen(git_repository_workdir(repository))));

            CAutoIndex index;
            if (git_repository_index(index.GetPointer(), repository) || (git_index_get_bypath(index, subpath, 1) == nullptr && git_index_get_bypath(index, subpath, 2) == nullptr))
            {
                PostQuitMessage(resolveWith);
                break;
            }

            CString sTemp;
            sTemp.Format(ResString(hResource, IDS_MARKASRESOLVED), static_cast<LPCTSTR>(CPathUtils::GetFileNameFromPath(selectionResult.c_str())));
            if (MessageBox(m_hwnd, sTemp, L"TortoiseGitMerge", MB_YESNO | MB_ICONQUESTION) != IDYES)
                break;

            CString cmd;
            cmd.Format(L"\"%sTortoiseGitProc.exe\" /command:resolve /path:\"%s\" /closeonend:1 /noquestion /skipcheck /silent", static_cast<LPCTSTR>(CPathUtils::GetAppDirectory()), selectionResult.c_str());
            if (resolveMsgWnd)
                cmd.AppendFormat(L" /resolvemsghwnd:%I64d /resolvemsgwparam:%I64d /resolvemsglparam:%I64d", reinterpret_cast<__int64>(resolveMsgWnd), static_cast<__int64>(resolveMsgWParam), static_cast<__int64>(resolveMsgLParam));

            STARTUPINFO startup = { 0 };
            PROCESS_INFORMATION process = { 0 };
            startup.cb = sizeof(startup);

            if (!CreateProcess(nullptr, cmd.GetBuffer(), nullptr, nullptr, FALSE, CREATE_UNICODE_ENVIRONMENT, nullptr, nullptr, &startup, &process))
            {
                cmd.ReleaseBuffer();
                PostQuitMessage(resolveWith);
                break;
            }
            cmd.ReleaseBuffer();

            AllowSetForegroundWindow(process.dwProcessId);

            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);

            PostQuitMessage(resolveWith);
        }
        break;
    case IDM_EXIT:
        ::PostQuitMessage(0);
        return 0;
        break;
    default:
        break;
    };
    return 1;
}

// splitter stuff
void CMainWindow::DrawXorBar(HDC hdc, int x1, int y1, int width, int height)
{
    static WORD _dotPatternBmp[8] =
    {
        0x0055, 0x00aa, 0x0055, 0x00aa,
        0x0055, 0x00aa, 0x0055, 0x00aa
    };

    HBITMAP hbm;
    HBRUSH  hbr, hbrushOld;

    hbm = CreateBitmap(8, 8, 1, 1, _dotPatternBmp);
    hbr = CreatePatternBrush(hbm);

    SetBrushOrgEx(hdc, x1, y1, 0);
    hbrushOld = static_cast<HBRUSH>(SelectObject(hdc, hbr));

    PatBlt(hdc, x1, y1, width, height, PATINVERT);

    SelectObject(hdc, hbrushOld);

    DeleteObject(hbr);
    DeleteObject(hbm);
}

LRESULT CMainWindow::Splitter_OnLButtonDown(HWND hwnd, UINT /*iMsg*/, WPARAM /*wParam*/, LPARAM lParam)
{
    POINT pt;
    HDC hdc;
    RECT rect;
    RECT clientrect;

    pt.x = static_cast<short>(LOWORD(lParam));  // horizontal position of cursor
    pt.y = static_cast<short>(HIWORD(lParam));

    GetClientRect(hwnd, &clientrect);
    GetWindowRect(hwnd, &rect);
    POINT zero = {0,0};
    ClientToScreen(hwnd, &zero);
    OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

    ClientToScreen(hwnd, &pt);
    // find out which drag bar is used
    bDrag2 = false;
    if (!selectionPaths.empty())
    {
        RECT pic2Rect;
        GetWindowRect(picWindow2, &pic2Rect);
        if (bVertical)
        {
            if (pic2Rect.bottom <= pt.y)
                bDrag2 = true;
        }
        else
        {
            if (pic2Rect.right <= pt.x)
                bDrag2 = true;
        }
    }

    //convert the mouse coordinates relative to the top-left of
    //the window
    pt.x -= rect.left;
    pt.y -= rect.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.x < 0)
        pt.x = 0;
    if (pt.x > rect.right-4)
        pt.x = rect.right-4;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom-4)
        pt.y = rect.bottom-4;

    bDragMode = true;

    SetCapture(hwnd);

    hdc = GetWindowDC(hwnd);
    if (bVertical)
        DrawXorBar(hdc, clientrect.left, pt.y+2, clientrect.right-clientrect.left-2, 4);
    else
        DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
    ReleaseDC(hwnd, hdc);

    oldx = pt.x;
    oldy = pt.y;

    return 0;
}

void CMainWindow::Splitter_CaptureChanged()
{
    bDragMode = false;
}

LRESULT CMainWindow::Splitter_OnLButtonUp(HWND hwnd, UINT /*iMsg*/, WPARAM /*wParam*/, LPARAM lParam)
{
    HDC hdc;
    RECT rect;
    RECT clientrect;

    POINT pt;
    pt.x = static_cast<short>(LOWORD(lParam)); // horizontal position of cursor
    pt.y = static_cast<short>(HIWORD(lParam));

    if (bDragMode == FALSE)
        return 0;

    const auto bordersm = CDPIAware::Instance().ScaleX(2);
    const auto borderl = CDPIAware::Instance().ScaleY(4);

    GetClientRect(hwnd, &clientrect);
    GetWindowRect(hwnd, &rect);
    POINT zero = {0,0};
    ClientToScreen(hwnd, &zero);
    OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

    ClientToScreen(hwnd, &pt);
    pt.x -= rect.left;
    pt.y -= rect.top;

    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.x < 0)
        pt.x = 0;
    if (pt.x > rect.right - borderl)
        pt.x = rect.right - borderl;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom - borderl)
        pt.y = rect.bottom - borderl;

    hdc = GetWindowDC(hwnd);
    if (bVertical)
        DrawXorBar(hdc, clientrect.left, oldy + bordersm, clientrect.right - clientrect.left - bordersm, borderl);
    else
        DrawXorBar(hdc, oldx + bordersm, clientrect.top, borderl, clientrect.bottom - clientrect.top - bordersm);
    ReleaseDC(hwnd, hdc);

    oldx = pt.x;
    oldy = pt.y;

    bDragMode = false;

    //convert the splitter position back to screen coords.
    GetWindowRect(hwnd, &rect);
    pt.x += rect.left;
    pt.y += rect.top;

    //now convert into CLIENT coordinates
    ScreenToClient(hwnd, &pt);
    GetClientRect(hwnd, &rect);
#define MINWINSIZE 10
    if (bVertical)
    {
        if (selectionPaths.size() != 3)
        {
            nSplitterPos = pt.y;
        }
        else
        {
            if (bDrag2)
            {
                if (pt.y < (nSplitterPos+MINWINSIZE))
                    pt.y = nSplitterPos+MINWINSIZE;
                nSplitterPos2 = pt.y;
            }
            else
            {
                if (pt.y > (nSplitterPos2-MINWINSIZE))
                    pt.y = nSplitterPos2-MINWINSIZE;
                nSplitterPos = pt.y;
            }
        }
    }
    else
    {
        if (selectionPaths.size() != 3)
        {
            nSplitterPos = pt.x;
        }
        else
        {
            if (bDrag2)
            {
                if (pt.x < (nSplitterPos+MINWINSIZE))
                    pt.x = nSplitterPos+MINWINSIZE;
                nSplitterPos2 = pt.x;
            }
            else
            {
                if (pt.x > (nSplitterPos2-MINWINSIZE))
                    pt.x = nSplitterPos2-MINWINSIZE;
                nSplitterPos = pt.x;
            }
        }
    }

    ReleaseCapture();

    //position the child controls
    PositionChildren(&rect);
    return 0;
}

LRESULT CMainWindow::Splitter_OnMouseMove(HWND hwnd, UINT /*iMsg*/, WPARAM wParam, LPARAM lParam)
{
    RECT rect;
    RECT clientrect;

    POINT pt;

    if (bDragMode == FALSE)
        return 0;

    const auto bordersm = CDPIAware::Instance().ScaleX(2);
    const auto borderl = CDPIAware::Instance().ScaleY(4);

    pt.x = static_cast<short>(LOWORD(lParam));  // horizontal position of cursor
    pt.y = static_cast<short>(HIWORD(lParam));

    GetClientRect(hwnd, &clientrect);
    GetWindowRect(hwnd, &rect);
    POINT zero = {0,0};
    ClientToScreen(hwnd, &zero);
    OffsetRect(&clientrect, zero.x-rect.left, zero.y-rect.top);

    //convert the mouse coordinates relative to the top-left of
    //the window
    ClientToScreen(hwnd, &pt);
    pt.x -= rect.left;
    pt.y -= rect.top;

    //same for the window coordinates - make them relative to 0,0
    OffsetRect(&rect, -rect.left, -rect.top);

    if (pt.x < 0)
        pt.x = 0;
    if (pt.x > rect.right - borderl)
        pt.x = rect.right - borderl;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom - borderl)
        pt.y = rect.bottom - borderl;

    if ((wParam & MK_LBUTTON) && ((bVertical && (pt.y != oldy)) || (!bVertical && (pt.x != oldx))))
    {
        HDC hdc = GetWindowDC(hwnd);

        if (bVertical)
        {
            DrawXorBar(hdc, clientrect.left, oldy + bordersm, clientrect.right - clientrect.left - bordersm, borderl);
            DrawXorBar(hdc, clientrect.left, pt.y + bordersm, clientrect.right - clientrect.left - bordersm, borderl);
        }
        else
        {
            DrawXorBar(hdc, oldx + bordersm, clientrect.top, borderl, clientrect.bottom - clientrect.top - bordersm);
            DrawXorBar(hdc, pt.x + bordersm, clientrect.top, borderl, clientrect.bottom - clientrect.top - bordersm);
        }

        ReleaseDC(hwnd, hdc);

        oldx = pt.x;
        oldy = pt.y;
    }

    return 0;
}

bool CMainWindow::OpenDialog()
{
    return (DialogBox(hResource, MAKEINTRESOURCE(IDD_OPEN), *this, reinterpret_cast<DLGPROC>(OpenDlgProc)) == IDOK);
}

BOOL CALLBACK CMainWindow::OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // center on the parent window
            HWND hParentWnd = ::GetParent(hwndDlg);
            RECT parentrect, childrect, centeredrect;
            GetWindowRect(hParentWnd, &parentrect);
            GetWindowRect(hwndDlg, &childrect);
            centeredrect.left = parentrect.left + ((parentrect.right-parentrect.left-childrect.right+childrect.left)/2);
            centeredrect.right = centeredrect.left + (childrect.right-childrect.left);
            centeredrect.top = parentrect.top + ((parentrect.bottom-parentrect.top-childrect.bottom+childrect.top)/2);
            centeredrect.bottom = centeredrect.top + (childrect.bottom-childrect.top);
            SetWindowPos(hwndDlg, nullptr, centeredrect.left, centeredrect.top, centeredrect.right - centeredrect.left, centeredrect.bottom - centeredrect.top, SWP_SHOWWINDOW);

            if (!leftpicpath.empty())
                SetDlgItemText(hwndDlg, IDC_LEFTIMAGE, leftpicpath.c_str());
            SetFocus(hwndDlg);
        }
        break;
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_LEFTBROWSE:
            {
                TCHAR path[MAX_PATH] = {0};
                if (AskForFile(hwndDlg, path))
                {
                    SetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path);
                }
            }
            break;
        case IDC_RIGHTBROWSE:
            {
                TCHAR path[MAX_PATH] = {0};
                if (AskForFile(hwndDlg, path))
                {
                    SetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path);
                }
            }
            break;
        case IDOK:
            {
                TCHAR path[MAX_PATH] = { 0 };
                if (!GetDlgItemText(hwndDlg, IDC_LEFTIMAGE, path, _countof(path)))
                    *path = 0;
                leftpicpath = path;
                if (!GetDlgItemText(hwndDlg, IDC_RIGHTIMAGE, path, _countof(path)))
                    *path = 0;
                rightpicpath = path;
            }
            // Fall through.
        case IDCANCEL:
            EndDialog(hwndDlg, wParam);
            return TRUE;
        }
    }
    return FALSE;
}

bool CMainWindow::AskForFile(HWND owner, TCHAR * path)
{
    OPENFILENAME ofn = {0};         // common dialog box structure
    // Initialize OPENFILENAME
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = path;
    ofn.nMaxFile = MAX_PATH;
    ResString sTitle(::hResource, IDS_OPENIMAGEFILE);
    ofn.lpstrTitle = sTitle;
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    ofn.hInstance = ::hResource;
    TCHAR filters[] = L"Images\0*.wmf;*.jpg;*jpeg;*.bmp;*.gif;*.png;*.ico;*.dib;*.emf;*.webp\0All (*.*)\0*.*\0\0";
    ofn.lpstrFilter = filters;
    ofn.nFilterIndex = 1;
    // Display the Open dialog box.
    if (GetOpenFileName(&ofn)==FALSE)
    {
        return false;
    }
    return true;
}

bool CMainWindow::CreateToolbar()
{
    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    hwndTB = CreateWindowEx(TBSTYLE_EX_DOUBLEBUFFER,
                            TOOLBARCLASSNAME,
                            static_cast<LPCTSTR>(nullptr),
                            WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                            0, 0, 0, 0,
                            *this,
                            reinterpret_cast<HMENU>(IDC_TORTOISEIDIFF),
                            hResource,
                            nullptr);
    if (hwndTB == INVALID_HANDLE_VALUE)
        return false;

    SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

    TBBUTTON tbb[14];
    // create an imagelist containing the icons for the toolbar
    auto imgSizeX = CDPIAware::Instance().ScaleX(24);
    auto imgSizeY = CDPIAware::Instance().ScaleY(24);
    hToolbarImgList = ImageList_Create(imgSizeX, imgSizeY, ILC_COLOR32 | ILC_MASK, 12, 4);
    if (!hToolbarImgList)
        return false;
    int index = 0;
    HICON hIcon = nullptr;
    if (selectionPaths.empty())
    {
        hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_OVERLAP), imgSizeX, imgSizeY);
        tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_OVERLAPIMAGES;
        tbb[index].fsState = TBSTATE_ENABLED;
        tbb[index].fsStyle = BTNS_BUTTON;
        tbb[index].dwData = 0;
        tbb[index++].iString = 0;

        hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_BLEND), imgSizeX, imgSizeY);
        tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_BLENDALPHA;
        tbb[index].fsState = 0;
        tbb[index].fsStyle = BTNS_BUTTON;
        tbb[index].dwData = 0;
        tbb[index++].iString = 0;

        tbb[index].iBitmap = 0;
        tbb[index].idCommand = 0;
        tbb[index].fsState = TBSTATE_ENABLED;
        tbb[index].fsStyle = BTNS_SEP;
        tbb[index].dwData = 0;
        tbb[index++].iString = 0;

        hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_LINK), imgSizeX, imgSizeY);
        tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_LINKIMAGESTOGETHER;
        tbb[index].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED;
        tbb[index].fsStyle = BTNS_BUTTON;
        tbb[index].dwData = 0;
        tbb[index++].iString = 0;

        hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FITWIDTHS), imgSizeX, imgSizeY);
        tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_FITIMAGEWIDTHS;
        tbb[index].fsState = TBSTATE_ENABLED;
        tbb[index].fsStyle = BTNS_BUTTON;
        tbb[index].dwData = 0;
        tbb[index++].iString = 0;

        hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FITHEIGHTS), imgSizeX, imgSizeY);
        tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
        tbb[index].idCommand = ID_VIEW_FITIMAGEHEIGHTS;
        tbb[index].fsState = TBSTATE_ENABLED;
        tbb[index].fsStyle = BTNS_BUTTON;
        tbb[index].dwData = 0;
        tbb[index++].iString = 0;

        tbb[index].iBitmap = 0;
        tbb[index].idCommand = 0;
        tbb[index].fsState = TBSTATE_ENABLED;
        tbb[index].fsStyle = BTNS_SEP;
        tbb[index].dwData = 0;
        tbb[index++].iString = 0;
    }

    hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FITINWINDOW), imgSizeX, imgSizeY);
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_FITIMAGESINWINDOW;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ORIGSIZE), imgSizeX, imgSizeY);
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ORININALSIZE;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ZOOMIN), imgSizeX, imgSizeY);
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ZOOMIN;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ZOOMOUT), imgSizeX, imgSizeY);
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ZOOMOUT;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    tbb[index].iBitmap = 0;
    tbb[index].idCommand = 0;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_SEP;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_IMGINFO), imgSizeX, imgSizeY);
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_IMAGEINFO;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_VERTICAL), imgSizeX, imgSizeY);
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ARRANGEVERTICAL;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    SendMessage(hwndTB, TB_SETIMAGELIST, 0, reinterpret_cast<LPARAM>(hToolbarImgList));
    SendMessage(hwndTB, TB_ADDBUTTONS, index, reinterpret_cast<LPARAM>(&tbb));
    SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
    ShowWindow(hwndTB, SW_SHOW);
    return true;
}

void CMainWindow::SetSelectionImage( FileType ft, const std::wstring& path, const std::wstring& title )
{
    selectionPaths[ft] = path;
    selectionTitles[ft] = title;
}
