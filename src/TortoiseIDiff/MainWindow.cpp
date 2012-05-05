// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2012 - TortoiseSVN

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
#include "commctrl.h"
#include "Commdlg.h"
#include "TortoiseIDiff.h"
#include "MainWindow.h"
#include "AboutDlg.h"
#include "TaskbarUUID.h"

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
    wcx.hCursor = LoadCursor(NULL, IDC_SIZEWE);
    wcx.lpszClassName = ResString(hResource, IDS_APP_TITLE);
    wcx.hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
    wcx.hbrBackground = (HBRUSH)(COLOR_3DFACE+1);
    wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
    wcx.hIconSm = LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
    if (RegisterWindow(&wcx))
    {
        if (Create(WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_VISIBLE, NULL))
        {
            UpdateWindow(m_hwnd);
            return true;
        }
    }
    return false;
}

void CMainWindow::PositionChildren(RECT * clientrect /* = NULL */)
{
    RECT tbRect;
    if (clientrect == NULL)
        return;
    SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
    GetWindowRect(hwndTB, &tbRect);
    LONG tbHeight = tbRect.bottom-tbRect.top-1;
    HDWP hdwp = BeginDeferWindowPos(2);
    if (bOverlap)
    {
        SetWindowPos(picWindow1, NULL, clientrect->left, clientrect->top+tbHeight, clientrect->right-clientrect->left, clientrect->bottom-clientrect->top-tbHeight, SWP_SHOWWINDOW);
    }
    else
    {
        if (bVertical)
        {
            RECT child;
            child.left = clientrect->left;
            child.top = clientrect->top+tbHeight;
            child.right = clientrect->right;
            child.bottom = nSplitterPos-(SPLITTER_BORDER/2);
            if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
            child.top = nSplitterPos+(SPLITTER_BORDER/2);
            child.bottom = clientrect->bottom;
            if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        }
        else
        {
            RECT child;
            child.left = clientrect->left;
            child.top = clientrect->top+tbHeight;
            child.right = nSplitterPos-(SPLITTER_BORDER/2);
            child.bottom = clientrect->bottom;
            if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow1, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
            child.left = nSplitterPos+(SPLITTER_BORDER/2);
            child.right = clientrect->right;
            if (hdwp) hdwp = DeferWindowPos(hdwp, picWindow2, NULL, child.left, child.top, child.right-child.left, child.bottom-child.top, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        }
    }
    if (hdwp) EndDeferWindowPos(hdwp);
    picWindow1.SetTransparentColor(transparentColor);
    picWindow2.SetTransparentColor(transparentColor);
    InvalidateRect(*this, NULL, FALSE);
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
            picWindow1.SetPic(leftpicpath, leftpictitle, true);
            picWindow2.RegisterAndCreateWindow(hwnd);
            picWindow2.SetPic(rightpicpath, rightpictitle, false);

            picWindow1.SetOtherPicWindow(&picWindow2);
            picWindow2.SetOtherPicWindow(&picWindow1);
            // center the splitter
            RECT rect;
            GetClientRect(hwnd, &rect);
            nSplitterPos = (rect.right-rect.left-SPLITTER_BORDER)/2;
            CreateToolbar();
            PositionChildren(&rect);
            picWindow1.FitImageInWindow();
            picWindow2.FitImageInWindow();
        }
        break;
    case WM_COMMAND:
        {
            return DoCommand(LOWORD(wParam));
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
            ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
            EndPaint(hwnd, &ps);
        }
        break;
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO * mmi = (MINMAXINFO*)lParam;
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
                nSplitterPos = (rect.bottom-rect.top-SPLITTER_BORDER+tbHeight)/2;
            }
            else
                nSplitterPos = (rect.right-rect.left-SPLITTER_BORDER)/2;
            PositionChildren(&rect);
        }
        break;
    case WM_SETCURSOR:
        {
            if ((HWND)wParam == *this)
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
                        HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZENS));
                        SetCursor(hCur);
                    }
                    else
                    {
                        HCURSOR hCur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));
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
            }
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR pNMHDR = (LPNMHDR)lParam;
            if (pNMHDR->code == TTN_GETDISPINFO)
            {
                LPTOOLTIPTEXT lpttt;

                lpttt = (LPTOOLTIPTEXT) lParam;
                lpttt->hinst = hResource;

                // Specify the resource identifier of the descriptive
                // text for the given button.
                TCHAR stringbuf[MAX_PATH] = {0};
                MENUITEMINFO mii;
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_TYPE;
                mii.dwTypeData = stringbuf;
                mii.cch = _countof(stringbuf);
                GetMenuItemInfo(GetMenu(*this), (UINT)lpttt->hdr.idFrom, FALSE, &mii);
                _tcscpy_s(lpttt->lpszText, 80, stringbuf);
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

LRESULT CMainWindow::DoCommand(int id)
{
    switch (id)
    {
    case ID_FILE_OPEN:
        {
            if (OpenDialog())
            {
                picWindow1.SetPic(leftpicpath, _T(""), true);
                picWindow2.SetPic(rightpicpath, _T(""), false);
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

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bShowInfo ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_IMAGEINFO, (LPARAM)&tbi);
        }
        break;
    case ID_VIEW_OVERLAPIMAGES:
        {
            bOverlap = !bOverlap;
            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bOverlap ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_OVERLAPIMAGES, uCheck);
            uCheck |= (m_BlendType == CPicWindow::BLEND_ALPHA) ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_BLENDALPHA, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bOverlap ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_OVERLAPIMAGES, (LPARAM)&tbi);

            tbi.fsState = (m_BlendType == CPicWindow::BLEND_ALPHA) ? TBSTATE_CHECKED : 0;
            if (bOverlap)
                tbi.fsState |= TBSTATE_ENABLED;
            else
                tbi.fsState = 0;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_BLENDALPHA, (LPARAM)&tbi);

            if (bOverlap)
                tbi.fsState = 0;
            else
                tbi.fsState = bVertical ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_ARRANGEVERTICAL, (LPARAM)&tbi);

            if (bOverlap)
            {
                bLinkedPositions = true;
                picWindow1.LinkPositions(bLinkedPositions);
                picWindow2.LinkPositions(bLinkedPositions);
                tbi.fsState = TBSTATE_CHECKED;
            }
            else
                tbi.fsState = bLinkedPositions ? TBSTATE_ENABLED | TBSTATE_CHECKED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_LINKIMAGESTOGETHER, (LPARAM)&tbi);

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
            uCheck |= (m_BlendType == CPicWindow::BLEND_ALPHA) ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_BLENDALPHA, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = (m_BlendType == CPicWindow::BLEND_ALPHA) ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_BLENDALPHA, (LPARAM)&tbi);
            picWindow1.SetBlendAlpha(m_BlendType, picWindow1.GetBlendAlpha());
            PositionChildren();
        }
        break;
    case ID_VIEW_TRANSPARENTCOLOR:
        {
            static COLORREF customColors[16] = {0};
            CHOOSECOLOR ccDlg;
            memset(&ccDlg, 0, sizeof(ccDlg));
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
                // The color picker takes the focus and we don't get it back.
                ::SetFocus(picWindow1);
            }
        }
        break;
    case ID_VIEW_FITTOGETHER:
        {
            bFitSizes = !bFitSizes;
            picWindow1.FitSizes(bFitSizes);
            picWindow2.FitSizes(bFitSizes);

            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bFitSizes ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_FITTOGETHER, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bFitSizes ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_FITTOGETHER, (LPARAM)&tbi);
        }
        break;
    case ID_VIEW_LINKIMAGESTOGETHER:
        {
            bLinkedPositions = !bLinkedPositions;
            picWindow1.LinkPositions(bLinkedPositions);
            picWindow2.LinkPositions(bLinkedPositions);

            HMENU hMenu = GetMenu(*this);
            UINT uCheck = MF_BYCOMMAND;
            uCheck |= bLinkedPositions ? MF_CHECKED : MF_UNCHECKED;
            CheckMenuItem(hMenu, ID_VIEW_LINKIMAGESTOGETHER, uCheck);

            // change the state of the toolbar button
            TBBUTTONINFO tbi;
            tbi.cbSize = sizeof(TBBUTTONINFO);
            tbi.dwMask = TBIF_STATE;
            tbi.fsState = bLinkedPositions ? TBSTATE_CHECKED | TBSTATE_ENABLED : TBSTATE_ENABLED;
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_LINKIMAGESTOGETHER, (LPARAM)&tbi);
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
            picWindow2.FitImageInWindow();
            picWindow1.FitImageInWindow();
        }
        break;
    case ID_VIEW_ORININALSIZE:
        {
            picWindow2.SetZoom(1.0, false);
            picWindow1.SetZoom(1.0, false);
        }
        break;
    case ID_VIEW_ZOOMIN:
        {
            picWindow1.Zoom(true, false);
            if ((!bFitSizes)&&(!bOverlap))
                picWindow2.Zoom(true, false);
        }
        break;
    case ID_VIEW_ZOOMOUT:
        {
            picWindow1.Zoom(false, false);
            if ((!bFitSizes)&&(!bOverlap))
                picWindow2.Zoom(false, false);
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
                nSplitterPos = (rect.bottom-rect.top-SPLITTER_BORDER+tbHeight)/2;
            }
            else
            {
                nSplitterPos = (rect.right-rect.left-SPLITTER_BORDER)/2;
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
            SendMessage(hwndTB, TB_SETBUTTONINFO, ID_VIEW_ARRANGEVERTICAL, (LPARAM)&tbi);

            PositionChildren(&rect);
        }
        break;
    case ID_ABOUT:
        {
            CAboutDlg dlg(*this);
            dlg.DoModal(hInst, IDD_ABOUT, *this);
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
    hbrushOld = (HBRUSH)SelectObject(hdc, hbr);

    PatBlt(hdc, x1, y1, width, height, PATINVERT);

    SelectObject(hdc, hbrushOld);

    DeleteObject(hbr);
    DeleteObject(hbm);
}

LRESULT CMainWindow::Splitter_OnLButtonDown(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    HDC hdc;
    RECT rect;
    RECT clientrect;

    pt.x = (short)LOWORD(lParam);  // horizontal position of cursor
    pt.y = (short)HIWORD(lParam);

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

LRESULT CMainWindow::Splitter_OnLButtonUp(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    RECT rect;
    RECT clientrect;

    POINT pt;
    pt.x = (short)LOWORD(lParam);  // horizontal position of cursor
    pt.y = (short)HIWORD(lParam);

    if (bDragMode == FALSE)
        return 0;

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
    if (pt.x > rect.right-4)
        pt.x = rect.right-4;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom-4)
        pt.y = rect.bottom-4;

    hdc = GetWindowDC(hwnd);
    if (bVertical)
        DrawXorBar(hdc, clientrect.left, oldy+2, clientrect.right-clientrect.left-2, 4);
    else
        DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
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
    if (bVertical)
        nSplitterPos = pt.y;
    else
        nSplitterPos = pt.x;

    ReleaseCapture();

    //position the child controls
    PositionChildren(&rect);
    return 0;
}

LRESULT CMainWindow::Splitter_OnMouseMove(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    RECT rect;
    RECT clientrect;

    POINT pt;

    if (bDragMode == FALSE)
        return 0;

    pt.x = (short)LOWORD(lParam);  // horizontal position of cursor
    pt.y = (short)HIWORD(lParam);

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
    if (pt.x > rect.right-4)
        pt.x = rect.right-4;
    if (pt.y < 0)
        pt.y = 0;
    if (pt.y > rect.bottom-4)
        pt.y = rect.bottom-4;

    if ((wParam & MK_LBUTTON) && ((bVertical && (pt.y != oldy)) || (!bVertical && (pt.x != oldx))))
    {
        hdc = GetWindowDC(hwnd);

        if (bVertical)
        {
            DrawXorBar(hdc, clientrect.left, oldy+2, clientrect.right-clientrect.left-2, 4);
            DrawXorBar(hdc, clientrect.left, pt.y+2, clientrect.right-clientrect.left-2, 4);
        }
        else
        {
            DrawXorBar(hdc, oldx+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
            DrawXorBar(hdc, pt.x+2, clientrect.top, 4, clientrect.bottom-clientrect.top-2);
        }

        ReleaseDC(hwnd, hdc);

        oldx = pt.x;
        oldy = pt.y;
    }

    return 0;
}

bool CMainWindow::OpenDialog()
{
    return (DialogBox(hResource, MAKEINTRESOURCE(IDD_OPEN), *this, (DLGPROC)OpenDlgProc)==IDOK);
}

BOOL CALLBACK CMainWindow::OpenDlgProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
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
            SetWindowPos(hwndDlg, NULL, centeredrect.left, centeredrect.top, centeredrect.right-centeredrect.left, centeredrect.bottom-centeredrect.top, SWP_SHOWWINDOW);

            if (leftpicpath.size())
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
                TCHAR path[MAX_PATH];
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
    ofn.lpstrTitle = ResString(::hResource, IDS_OPENIMAGEFILE);
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST | OFN_EXPLORER;
    ofn.hInstance = ::hResource;
    TCHAR filters[] = _T("Images\0*.wmf;*.jpg;*jpeg;*.bmp;*.gif;*.png;*.ico;*.dib;*.emf\0All (*.*)\0*.*\0\0");
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

    hwndTB = CreateWindowEx(0,
                            TOOLBARCLASSNAME,
                            (LPCTSTR)NULL,
                            WS_CHILD | WS_BORDER | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS,
                            0, 0, 0, 0,
                            *this,
                            (HMENU)IDC_TORTOISEIDIFF,
                            hResource,
                            NULL);
    if (hwndTB == INVALID_HANDLE_VALUE)
        return false;

    SendMessage(hwndTB, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);

    TBBUTTON tbb[12];
    // create an imagelist containing the icons for the toolbar
    hToolbarImgList = ImageList_Create(24, 24, ILC_COLOR32 | ILC_MASK, 12, 4);
    if (hToolbarImgList == NULL)
        return false;
    int index = 0;
    HICON hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_OVERLAP));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_OVERLAPIMAGES;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_BLEND));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_BLENDALPHA;
    tbb[index].fsState = 0;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_LINK));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_LINKIMAGESTOGETHER;
    tbb[index].fsState = TBSTATE_ENABLED | TBSTATE_CHECKED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_FITTOGETHER));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_FITTOGETHER;
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

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_VERTICAL));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ARRANGEVERTICAL;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_FITINWINDOW));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_FITIMAGESINWINDOW;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_ORIGSIZE));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ORININALSIZE;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_ZOOMIN));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_ZOOMIN;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_ZOOMOUT));
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

    hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_IMGINFO));
    tbb[index].iBitmap = ImageList_AddIcon(hToolbarImgList, hIcon);
    tbb[index].idCommand = ID_VIEW_IMAGEINFO;
    tbb[index].fsState = TBSTATE_ENABLED;
    tbb[index].fsStyle = BTNS_BUTTON;
    tbb[index].dwData = 0;
    tbb[index++].iString = 0;

    SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)hToolbarImgList);
    SendMessage(hwndTB, TB_ADDBUTTONS, (WPARAM)index, (LPARAM) (LPTBBUTTON) &tbb);
    SendMessage(hwndTB, TB_AUTOSIZE, 0, 0);
    ShowWindow(hwndTB, SW_SHOW);
    return true;

}
