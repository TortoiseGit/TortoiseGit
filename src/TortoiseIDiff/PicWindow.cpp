// TortoiseGitIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006-2013, 2018-2019 - TortoiseSVN
// Copyright (C) 2016, 2018-2019 - TortoiseGit

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
#include <shellapi.h>
#include <CommCtrl.h>
#include "PicWindow.h"
#include <math.h>
#include <memory>
#include "../Utils/DPIAware.h"
#include "../Utils/LoadIconEx.h"

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "shell32.lib")

bool CPicWindow::RegisterAndCreateWindow(HWND hParent)
{
    WNDCLASSEX wcx;

    // Fill in the window class structure with default parameters
    wcx.cbSize = sizeof(WNDCLASSEX);
    wcx.style = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc = CWindow::stWinMsgHandler;
    wcx.cbClsExtra = 0;
    wcx.cbWndExtra = 0;
    wcx.hInstance = hResource;
    wcx.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcx.lpszClassName = L"TortoiseGitIDiffPicWindow";
    wcx.hIcon = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_TORTOISEIDIFF), GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
    wcx.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
    wcx.hIconSm = LoadIconEx(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
    RegisterWindow(&wcx);
    if (CreateEx(WS_EX_ACCEPTFILES | WS_EX_CLIENTEDGE, WS_CHILD | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, hParent))
    {
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        CreateButtons();
        return true;
    }
    return false;
}

void CPicWindow::PositionTrackBar()
{
    const auto slider_width = CDPIAware::Instance().ScaleX(SLIDER_WIDTH);
    RECT rc;
    GetClientRect(&rc);
    HWND slider = m_AlphaSlider.GetWindow();
    if ((pSecondPic)&&(m_blend == BLEND_ALPHA))
    {
        MoveWindow(slider, 0, rc.top - CDPIAware::Instance().ScaleY(4) + slider_width, slider_width, rc.bottom - rc.top - slider_width + CDPIAware::Instance().ScaleX(8), true);
        ShowWindow(slider, SW_SHOW);
        MoveWindow(hwndAlphaToggleBtn, 0, rc.top - CDPIAware::Instance().ScaleY(4), slider_width, slider_width, true);
        ShowWindow(hwndAlphaToggleBtn, SW_SHOW);
    }
    else
    {
        ShowWindow(slider, SW_HIDE);
        ShowWindow(hwndAlphaToggleBtn, SW_HIDE);
    }
}

LRESULT CALLBACK CPicWindow::WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    TRACKMOUSEEVENT mevt;
    switch (uMsg)
    {
    case WM_CREATE:
        {
            // create a slider control
            CreateTrackbar(hwnd);
            ShowWindow(m_AlphaSlider.GetWindow(), SW_HIDE);
            //Create the tooltips
            TOOLINFO ti;
            RECT rect;                  // for client area coordinates

            hwndTT = CreateWindowEx(0,
                TOOLTIPS_CLASS,
                nullptr,
                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                hwnd,
                nullptr,
                hResource,
                nullptr
                );

            SetWindowPos(hwndTT,
                HWND_TOP,
                0,
                0,
                0,
                0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            ::GetClientRect(hwnd, &rect);

            ti.cbSize = sizeof(TOOLINFO);
            ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;
            ti.hwnd = hwnd;
            ti.hinst = hResource;
            ti.uId = 0;
            ti.lpszText = LPSTR_TEXTCALLBACK;
            // ToolTip control will cover the whole window
            ti.rect.left = rect.left;
            ti.rect.top = rect.top;
            ti.rect.right = rect.right;
            ti.rect.bottom = rect.bottom;

            SendMessage(hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
            SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
            nHSecondScrollPos = 0;
            nVSecondScrollPos = 0;
        }
        break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        InvalidateRect(*this, nullptr, FALSE);
        break;
    case WM_ERASEBKGND:
        return 1;
        break;
    case WM_PAINT:
        Paint(hwnd);
        break;
    case WM_SIZE:
        PositionTrackBar();
        SetupScrollBars();
        break;
    case WM_VSCROLL:
        if (pSecondPic && (reinterpret_cast<HWND>(lParam) == m_AlphaSlider.GetWindow()))
        {
            if (LOWORD(wParam) == TB_THUMBTRACK)
            {
                // while tracking, only redraw after 50 milliseconds
                ::SetTimer(*this, TIMER_ALPHASLIDER, 50, nullptr);
            }
            else
                SetBlendAlpha(m_blend, SendMessage(m_AlphaSlider.GetWindow(), TBM_GETPOS, 0, 0) / 16.0f);
        }
        else
        {
            UINT nPos = HIWORD(wParam);
            bool bForceUpdate = false;
            if (LOWORD(wParam) == SB_THUMBTRACK || LOWORD(wParam) == SB_THUMBPOSITION)
            {
                // Get true 32-bit scroll position
                SCROLLINFO si;
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_TRACKPOS;
                GetScrollInfo(*this, SB_VERT, &si);
                nPos = si.nTrackPos;
                bForceUpdate = true;
            }

            OnVScroll(LOWORD(wParam), nPos);
            if (bLinkedPositions && pTheOtherPic)
            {
                pTheOtherPic->OnVScroll(LOWORD(wParam), nPos);
                if (bForceUpdate)
                    ::UpdateWindow(*pTheOtherPic);
            }
        }
        break;
    case WM_HSCROLL:
        {
            UINT nPos = HIWORD(wParam);
            bool bForceUpdate = false;
            if (LOWORD(wParam) == SB_THUMBTRACK || LOWORD(wParam) == SB_THUMBPOSITION)
            {
                // Get true 32-bit scroll position
                SCROLLINFO si;
                si.cbSize = sizeof(SCROLLINFO);
                si.fMask = SIF_TRACKPOS;
                GetScrollInfo(*this, SB_VERT, &si);
                nPos = si.nTrackPos;
                bForceUpdate = true;
            }

            OnHScroll(LOWORD(wParam), nPos);
            if (bLinkedPositions && pTheOtherPic)
            {
                pTheOtherPic->OnHScroll(LOWORD(wParam), nPos);
                if (bForceUpdate)
                    ::UpdateWindow(*pTheOtherPic);
            }
        }
        break;
    case WM_MOUSEWHEEL:
        {
            OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
        }
        break;
    case WM_MOUSEHWHEEL:
        {
            OnMouseWheel(GET_KEYSTATE_WPARAM(wParam)|MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(*this);
        ptPanStart.x = GET_X_LPARAM(lParam);
        ptPanStart.y = GET_Y_LPARAM(lParam);
        startVScrollPos = nVScrollPos;
        startHScrollPos = nHScrollPos;
        startVSecondScrollPos = nVSecondScrollPos;
        startHSecondScrollPos = nHSecondScrollPos;
        bDragging = true;
        SetCapture(*this);
        break;
    case WM_LBUTTONUP:
        bDragging = false;
        ReleaseCapture();
        InvalidateRect(*this, nullptr, FALSE);
        break;
    case WM_MOUSELEAVE:
        ptPanStart.x = -1;
        ptPanStart.y = -1;
        m_lastTTPos.x = 0;
        m_lastTTPos.y = 0;
        SendMessage(hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
        break;
    case WM_MOUSEMOVE:
        {
            mevt.cbSize = sizeof(TRACKMOUSEEVENT);
            mevt.dwFlags = TME_LEAVE;
            mevt.dwHoverTime = HOVER_DEFAULT;
            mevt.hwndTrack = *this;
            ::TrackMouseEvent(&mevt);
            POINT pt = { static_cast<int>(static_cast<short>(LOWORD(lParam))), static_cast<int>(static_cast<short>(HIWORD(lParam))) };
            if (pt.y < CDPIAware::Instance().ScaleY(HEADER_HEIGHT))
            {
                ClientToScreen(*this, &pt);
                if ((abs(m_lastTTPos.x - pt.x) > 20)||(abs(m_lastTTPos.y - pt.y) > 10))
                {
                    m_lastTTPos = pt;
                    pt.x += 15;
                    pt.y += 15;
                    SendMessage(hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
                    TOOLINFO ti = {0};
                    ti.cbSize = sizeof(TOOLINFO);
                    ti.hwnd = *this;
                    ti.uId = 0;
                    SendMessage(hwndTT, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));
                }
            }
            else
            {
                SendMessage(hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
                m_lastTTPos.x = 0;
                m_lastTTPos.y = 0;
            }
            if ((wParam & MK_LBUTTON) &&
                (ptPanStart.x >= 0) &&
                (ptPanStart.y >= 0))
            {
                // pan the image
                int xPos = GET_X_LPARAM(lParam);
                int yPos = GET_Y_LPARAM(lParam);

                if (wParam & MK_CONTROL)
                {
                    nHSecondScrollPos = startHSecondScrollPos + (ptPanStart.x - xPos);
                    nVSecondScrollPos = startVSecondScrollPos + (ptPanStart.y - yPos);
                }
                else if (wParam & MK_SHIFT)
                {
                    nHScrollPos = startHScrollPos + (ptPanStart.x - xPos);
                    nVScrollPos = startVScrollPos + (ptPanStart.y - yPos);
                }
                else
                {
                    nHSecondScrollPos = startHSecondScrollPos + (ptPanStart.x - xPos);
                    nVSecondScrollPos = startVSecondScrollPos + (ptPanStart.y - yPos);
                    nHScrollPos = startHScrollPos + (ptPanStart.x - xPos);
                    nVScrollPos = startVScrollPos + (ptPanStart.y - yPos);
                    if (!bLinkedPositions && pTheOtherPic)
                    {
                        // snap to the other picture borders
                        if (abs(nVScrollPos-pTheOtherPic->nVScrollPos) < 10)
                            nVScrollPos = pTheOtherPic->nVScrollPos;
                        if (abs(nHScrollPos-pTheOtherPic->nHScrollPos) < 10)
                            nHScrollPos = pTheOtherPic->nHScrollPos;
                    }
                }
                SetupScrollBars();
                InvalidateRect(*this, nullptr, TRUE);
                UpdateWindow(*this);
                if (pTheOtherPic && (bLinkedPositions) && ((wParam & MK_SHIFT)==0))
                {
                    pTheOtherPic->nHScrollPos = nHScrollPos;
                    pTheOtherPic->nVScrollPos = nVScrollPos;
                    pTheOtherPic->SetupScrollBars();
                    InvalidateRect(*pTheOtherPic, nullptr, TRUE);
                    UpdateWindow(*pTheOtherPic);
                }
            }
        }
        break;
    case WM_SETCURSOR:
        {
            // we show a hand cursor if the image can be dragged,
            // and a hand-down cursor if the image is currently dragged
            if (*this == reinterpret_cast<HWND>(wParam) && LOWORD(lParam) == HTCLIENT)
            {
                RECT rect;
                GetClientRect(&rect);
                LONG width = picture.m_Width;
                LONG height = picture.m_Height;
                if (pSecondPic)
                {
                    width = max(width, pSecondPic->m_Width);
                    height = max(height, pSecondPic->m_Height);
                }

                if ((GetKeyState(VK_LBUTTON)&0x8000)||(HIWORD(lParam) == WM_LBUTTONDOWN))
                {
                    SetCursor(curHandDown);
                }
                else
                {
                    SetCursor(curHand);
                }
                return TRUE;
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        break;
    case WM_DROPFILES:
        {
            auto hDrop = reinterpret_cast<HDROP>(wParam);
            TCHAR szFileName[MAX_PATH] = {0};
            // we only use the first file dropped (if multiple files are dropped)
            if (DragQueryFile(hDrop, 0, szFileName, _countof(szFileName)))
            {
                SetPic(szFileName, L"", bMainPic);
                FitImageInWindow();
                InvalidateRect(*this, nullptr, TRUE);
            }
        }
        break;
    case WM_COMMAND:
        {
            switch (LOWORD(wParam))
            {
            case LEFTBUTTON_ID:
                {
                    PrevImage();
                    if (bLinkedPositions && pTheOtherPic)
                        pTheOtherPic->PrevImage();
                    return 0;
                }
                break;
            case RIGHTBUTTON_ID:
                {
                    NextImage();
                    if (bLinkedPositions && pTheOtherPic)
                        pTheOtherPic->NextImage();
                    return 0;
                }
                break;
            case PLAYBUTTON_ID:
                {
                    bPlaying = !bPlaying;
                    Animate(bPlaying);
                    if (bLinkedPositions && pTheOtherPic)
                        pTheOtherPic->Animate(bPlaying);
                    return 0;
                }
                break;
            case ALPHATOGGLEBUTTON_ID:
                {
                    WORD msg = HIWORD(wParam);
                    switch (msg)
                    {
                    case BN_DOUBLECLICKED:
                        {
                            SendMessage(hwndAlphaToggleBtn, BM_SETSTATE, 1, 0);
                            SetTimer(*this, ID_ALPHATOGGLETIMER, 1000, nullptr);
                        }
                        break;
                    case BN_CLICKED:
                        KillTimer(*this, ID_ALPHATOGGLETIMER);
                        ToggleAlpha();
                        break;
                    }
                    return 0;
                }
                break;
            case BLENDALPHA_ID:
                {
                    m_blend = BLEND_ALPHA;
                    PositionTrackBar();
                    InvalidateRect(*this, nullptr, TRUE);
                }
                break;
            case BLENDXOR_ID:
                {
                    m_blend = BLEND_XOR;
                    PositionTrackBar();
                    InvalidateRect(*this, nullptr, TRUE);
                }
                break;
            case SELECTBUTTON_ID:
                {
                    SendMessage(GetParent(m_hwnd), WM_COMMAND, MAKEWPARAM(SELECTBUTTON_ID, SELECTBUTTON_ID), reinterpret_cast<LPARAM>(m_hwnd));
                }
                break;
            }
        }
        break;
    case WM_TIMER:
        {
            switch (wParam)
            {
            case ID_ANIMATIONTIMER:
                {
                    nCurrentFrame++;
                    if (nCurrentFrame > picture.GetNumberOfFrames(0))
                        nCurrentFrame = 1;
                    long delay = picture.SetActiveFrame(nCurrentFrame);
                    delay = max(100l, delay);
                    SetTimer(*this, ID_ANIMATIONTIMER, delay, nullptr);
                    InvalidateRect(*this, nullptr, FALSE);
                }
                break;
            case TIMER_ALPHASLIDER:
                {
                    SetBlendAlpha(m_blend, SendMessage(m_AlphaSlider.GetWindow(), TBM_GETPOS, 0, 0)/16.0f);
                    KillTimer(*this, TIMER_ALPHASLIDER);
                }
                break;
            case ID_ALPHATOGGLETIMER:
                {
                    ToggleAlpha();
                }
                break;
            }
        }
        break;
    case WM_NOTIFY:
        {
            auto pNMHDR = reinterpret_cast<LPNMHDR>(lParam);
            if (pNMHDR->code == TTN_GETDISPINFO)
            {
                if (pNMHDR->hwndFrom == m_AlphaSlider.GetWindow())
                {
                    auto lpttt = reinterpret_cast<LPTOOLTIPTEXT>(lParam);
                    lpttt->hinst = hResource;
                    TCHAR stringbuf[MAX_PATH] = {0};
                    swprintf_s(stringbuf, L"%i%% alpha", static_cast<int>(SendMessage(m_AlphaSlider.GetWindow(),TBM_GETPOS, 0, 0) / 16.0f * 100.0f));
                    wcscpy_s(lpttt->lpszText, 80, stringbuf);
                }
                else if (pNMHDR->idFrom == reinterpret_cast<UINT_PTR>(hwndAlphaToggleBtn))
                {
                    swprintf_s(m_wszTip, static_cast<const TCHAR*>(ResString(hResource, IDS_ALPHABUTTONTT)), static_cast<int>(SendMessage(m_AlphaSlider.GetWindow(),TBM_GETPOS, 0, 0) / 16.0f * 100.0f));
                    if (pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        auto pTTTW = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);
                        pTTTW->lpszText = m_wszTip;
                    }
                    else
                    {
                        auto pTTTA = reinterpret_cast<NMTTDISPINFOA*>(pNMHDR);
                        pTTTA->lpszText = m_szTip;
                        ::WideCharToMultiByte(CP_ACP, 0, m_wszTip, -1, m_szTip, 8192, nullptr, nullptr);
                    }
                }
                else
                {
                    BuildInfoString(m_wszTip, _countof(m_wszTip), true);
                    if (pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        auto pTTTW = reinterpret_cast<NMTTDISPINFOW*>(pNMHDR);
                        pTTTW->lpszText = m_wszTip;
                    }
                    else
                    {
                        auto pTTTA = reinterpret_cast<NMTTDISPINFOA*>(pNMHDR);
                        pTTTA->lpszText = m_szTip;
                        ::WideCharToMultiByte(CP_ACP, 0, m_wszTip, -1, m_szTip, 8192, nullptr, nullptr);
                    }
                }
            }
        }
        break;
    case WM_DESTROY:
        DestroyIcon(hLeft);
        DestroyIcon(hRight);
        DestroyIcon(hPlay);
        DestroyIcon(hStop);
        bWindowClosed = TRUE;
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
};

void CPicWindow::NextImage()
{
    nCurrentDimension++;
    if (nCurrentDimension > picture.GetNumberOfDimensions())
        nCurrentDimension = picture.GetNumberOfDimensions();
    nCurrentFrame++;
    if (nCurrentFrame > picture.GetNumberOfFrames(0))
        nCurrentFrame = picture.GetNumberOfFrames(0);
    picture.SetActiveFrame(nCurrentFrame >= nCurrentDimension ? nCurrentFrame : nCurrentDimension);
    InvalidateRect(*this, nullptr, FALSE);
    PositionChildren();
}

void CPicWindow::PrevImage()
{
    nCurrentDimension--;
    if (nCurrentDimension < 1)
        nCurrentDimension = 1;
    nCurrentFrame--;
    if (nCurrentFrame < 1)
        nCurrentFrame = 1;
    picture.SetActiveFrame(nCurrentFrame >= nCurrentDimension ? nCurrentFrame : nCurrentDimension);
    InvalidateRect(*this, nullptr, FALSE);
    PositionChildren();
}

void CPicWindow::Animate(bool bStart)
{
    if (bStart)
    {
        SendMessage(hwndPlayBtn, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hStop));
        SetTimer(*this, ID_ANIMATIONTIMER, 0, nullptr);
    }
    else
    {
        SendMessage(hwndPlayBtn, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hPlay));
        KillTimer(*this, ID_ANIMATIONTIMER);
    }
}

void CPicWindow::SetPic(const tstring& path, const tstring& title, bool bFirst)
{
    bMainPic = bFirst;
    picpath=path;pictitle=title;
    picture.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    bValid = picture.Load(picpath);
    nDimensions = picture.GetNumberOfDimensions();
    if (nDimensions)
        nFrames = picture.GetNumberOfFrames(0);
    if (bValid)
    {
        picscale = 100;
        PositionChildren();
        InvalidateRect(*this, nullptr, FALSE);
    }
}

void CPicWindow::DrawViewTitle(HDC hDC, RECT * rect)
{
    const auto header_height = CDPIAware::Instance().ScaleY(HEADER_HEIGHT);
    auto hFont = CreateFont(-CDPIAware::Instance().PointsToPixelsY(pSecondPic ? 8 : 10), 0, 0, 0, FW_DONTCARE, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"MS Shell Dlg");
    auto hFontOld = static_cast<HFONT>(SelectObject(hDC, hFont));

    RECT textrect;
    textrect.left = rect->left;
    textrect.top = rect->top;
    textrect.right = rect->right;
    textrect.bottom = rect->top + header_height;
    if (HasMultipleImages())
        textrect.bottom += header_height;

    COLORREF crBk, crFg;
    crBk = ::GetSysColor(COLOR_SCROLLBAR);
    crFg = ::GetSysColor(COLOR_WINDOWTEXT);
    SetBkColor(hDC, crBk);
    ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &textrect, nullptr, 0, nullptr);

    if (GetFocus() == *this)
        DrawEdge(hDC, &textrect, EDGE_BUMP, BF_RECT);
    else
        DrawEdge(hDC, &textrect, EDGE_ETCHED, BF_RECT);

    SetTextColor(hDC, crFg);

    // use the path if no title is set.
    tstring * title = pictitle.empty() ? &picpath : &pictitle;

    tstring realtitle = *title;
    tstring imgnumstring;

    if (HasMultipleImages())
    {
        TCHAR buf[MAX_PATH] = {0};
        if (nFrames > 1)
            swprintf_s(buf, static_cast<const TCHAR*>(ResString(hResource, IDS_DIMENSIONSANDFRAMES)), nCurrentFrame, nFrames);
        else
            swprintf_s(buf, static_cast<const TCHAR*>(ResString(hResource, IDS_DIMENSIONSANDFRAMES)), nCurrentDimension, nDimensions);
        imgnumstring = buf;
    }

    SIZE stringsize;
    if (GetTextExtentPoint32(hDC, realtitle.c_str(), static_cast<int>(realtitle.size()), &stringsize))
    {
        int nStringLength = stringsize.cx;
        int texttop = pSecondPic ? textrect.top + (header_height /2) - stringsize.cy : textrect.top + (header_height /2) - stringsize.cy/2;
        ExtTextOut(hDC,
            max(textrect.left + ((textrect.right - textrect.left) - nStringLength) / 2, 1l),
            texttop,
            ETO_CLIPPED,
            &textrect,
            realtitle.c_str(),
            static_cast<UINT>(realtitle.size()),
            nullptr);
        if (pSecondPic)
        {
            realtitle = (pictitle2.empty() ? picpath2 : pictitle2);
            ExtTextOut(hDC,
                max(textrect.left + ((textrect.right - textrect.left) - nStringLength) / 2, 1l),
                texttop + stringsize.cy,
                ETO_CLIPPED,
                &textrect,
                realtitle.c_str(),
                static_cast<UINT>(realtitle.size()),
                nullptr);
        }
    }
    if (HasMultipleImages())
    {
        if (GetTextExtentPoint32(hDC, imgnumstring.c_str(), static_cast<int>(imgnumstring.size()), &stringsize))
        {
            int nStringLength = stringsize.cx;

            ExtTextOut(hDC,
                max(textrect.left + ((textrect.right - textrect.left) - nStringLength) / 2, 1l),
                textrect.top + header_height + (header_height /2) - stringsize.cy/2,
                ETO_CLIPPED,
                &textrect,
                imgnumstring.c_str(),
                static_cast<UINT>(imgnumstring.size()),
                nullptr);
        }
    }
    SelectObject(hDC, hFontOld);
    DeleteObject(hFont);
}

void CPicWindow::SetupScrollBars()
{
    RECT rect;
    GetClientRect(&rect);

    SCROLLINFO si = {sizeof(si)};

    si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;

    long width = picture.m_Width*picscale/100;
    long height = picture.m_Height*picscale/100;
    if (pSecondPic && pTheOtherPic)
    {
        width = max(width, pSecondPic->m_Width*pTheOtherPic->GetZoom()/100);
        height = max(height, pSecondPic->m_Height*pTheOtherPic->GetZoom()/100);
    }

    bool bShowHScrollBar = (nHScrollPos > 0); // left of pic is left of window
    bShowHScrollBar      = bShowHScrollBar || (width-nHScrollPos > rect.right); // right of pic is outside right of window
    bShowHScrollBar      = bShowHScrollBar || (width+nHScrollPos > rect.right); // right of pic is outside right of window
    bool bShowVScrollBar = (nVScrollPos > 0); // top of pic is above window
    bShowVScrollBar      = bShowVScrollBar || (height-nVScrollPos+rect.top > rect.bottom); // bottom of pic is below window
    bShowVScrollBar      = bShowVScrollBar || (height+nVScrollPos+rect.top > rect.bottom); // bottom of pic is below window

    // if the image is smaller than the window, we don't need the scrollbars
    ShowScrollBar(*this, SB_HORZ, bShowHScrollBar);
    ShowScrollBar(*this, SB_VERT, bShowVScrollBar);

    if (bShowVScrollBar)
    {
        si.nPos  = nVScrollPos;
        si.nPage = rect.bottom-rect.top;
        if (height < rect.bottom-rect.top)
        {
            if (nVScrollPos > 0)
            {
                si.nMin  = 0;
                si.nMax  = rect.bottom+nVScrollPos-rect.top;
            }
            else
            {
                si.nMin  = nVScrollPos;
                si.nMax  = int(height);
            }
        }
        else
        {
            if (nVScrollPos > 0)
            {
                si.nMin  = 0;
                si.nMax  = int(max(height, rect.bottom+nVScrollPos-rect.top));
            }
            else
            {
                si.nMin  = 0;
                si.nMax  = int(height-nVScrollPos);
            }
        }
        SetScrollInfo(*this, SB_VERT, &si, TRUE);
    }

    if (bShowHScrollBar)
    {
        si.nPos  = nHScrollPos;
        si.nPage = rect.right-rect.left;
        if (width < rect.right-rect.left)
        {
            if (nHScrollPos > 0)
            {
                si.nMin  = 0;
                si.nMax  = rect.right+nHScrollPos-rect.left;
            }
            else
            {
                si.nMin  = nHScrollPos;
                si.nMax  = int(width);
            }
        }
        else
        {
            if (nHScrollPos > 0)
            {
                si.nMin  = 0;
                si.nMax  = int(max(width, rect.right+nHScrollPos-rect.left));
            }
            else
            {
                si.nMin  = 0;
                si.nMax  = int(width-nHScrollPos);
            }
        }
        SetScrollInfo(*this, SB_HORZ, &si, TRUE);
    }

    PositionChildren();
}

void CPicWindow::OnVScroll(UINT nSBCode, UINT nPos)
{
    RECT rect;
    GetClientRect(&rect);

    switch (nSBCode)
    {
    case SB_BOTTOM:
        nVScrollPos = LONG(picture.GetHeight()*picscale/100);
        break;
    case SB_TOP:
        nVScrollPos = 0;
        break;
    case SB_LINEDOWN:
        nVScrollPos++;
        break;
    case SB_LINEUP:
        nVScrollPos--;
        break;
    case SB_PAGEDOWN:
        nVScrollPos += (rect.bottom-rect.top);
        break;
    case SB_PAGEUP:
        nVScrollPos -= (rect.bottom-rect.top);
        break;
    case SB_THUMBPOSITION:
        nVScrollPos = nPos;
        break;
    case SB_THUMBTRACK:
        nVScrollPos = nPos;
        break;
    default:
        return;
    }
    LONG height = LONG(picture.GetHeight()*picscale/100);
    if (pSecondPic)
    {
        height = max(height, LONG(pSecondPic->GetHeight()*picscale/100));
        nVSecondScrollPos = nVScrollPos;
    }
    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::OnHScroll(UINT nSBCode, UINT nPos)
{
    RECT rect;
    GetClientRect(&rect);

    switch (nSBCode)
    {
    case SB_RIGHT:
        nHScrollPos = LONG(picture.GetWidth()*picscale/100);
        break;
    case SB_LEFT:
        nHScrollPos = 0;
        break;
    case SB_LINERIGHT:
        nHScrollPos++;
        break;
    case SB_LINELEFT:
        nHScrollPos--;
        break;
    case SB_PAGERIGHT:
        nHScrollPos += (rect.right-rect.left);
        break;
    case SB_PAGELEFT:
        nHScrollPos -= (rect.right-rect.left);
        break;
    case SB_THUMBPOSITION:
        nHScrollPos = nPos;
        break;
    case SB_THUMBTRACK:
        nHScrollPos = nPos;
        break;
    default:
        return;
    }
    LONG width = LONG(picture.GetWidth()*picscale/100);
    if (pSecondPic)
    {
        width = max(width, LONG(pSecondPic->GetWidth()*picscale/100));
        nHSecondScrollPos = nHScrollPos;
    }
    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::OnMouseWheel(short fwKeys, short zDelta)
{
    RECT rect;
    GetClientRect(&rect);
    LONG width = long(picture.m_Width*picscale/100);
    LONG height = long(picture.m_Height*picscale/100);
    if (pSecondPic)
    {
        width = max(width, long(pSecondPic->m_Width*picscale/100));
        height = max(height, long(pSecondPic->m_Height*picscale/100));
    }
    if ((fwKeys & MK_SHIFT)&&(fwKeys & MK_CONTROL)&&(pSecondPic))
    {
        // ctrl+shift+wheel: change the alpha channel
        float a = blendAlpha;
        a -= float(zDelta)/120.0f/4.0f;
        if (a < 0.0f)
            a = 0.0f;
        else if (a > 1.0f)
            a = 1.0f;
        SetBlendAlpha(m_blend, a);
    }
    else if (fwKeys & MK_SHIFT)
    {
        // shift means scrolling sideways
        OnHScroll(SB_THUMBPOSITION, GetHPos()-zDelta);
        if ((bLinkedPositions)&&(pTheOtherPic))
        {
            pTheOtherPic->OnHScroll(SB_THUMBPOSITION, pTheOtherPic->GetHPos()-zDelta);
        }
    }
    else if (fwKeys & MK_CONTROL)
    {
        // control means adjusting the scale factor
        Zoom(zDelta>0, true);
        PositionChildren();
        InvalidateRect(*this, nullptr, FALSE);
        SetWindowPos(*this, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOREPOSITION | SWP_NOMOVE);
        UpdateWindow(*this);
        if ((bLinkedPositions || bOverlap) && pTheOtherPic)
        {
            pTheOtherPic->nHScrollPos = nHScrollPos;
            pTheOtherPic->nVScrollPos = nVScrollPos;
            pTheOtherPic->SetupScrollBars();
            InvalidateRect(*pTheOtherPic, nullptr, TRUE);
            UpdateWindow(*pTheOtherPic);
        }
    }
    else
    {
        OnVScroll(SB_THUMBPOSITION, GetVPos()-zDelta);
        if ((bLinkedPositions)&&(pTheOtherPic))
        {
            pTheOtherPic->OnVScroll(SB_THUMBPOSITION, pTheOtherPic->GetVPos()-zDelta);
        }
    }
}

void CPicWindow::GetClientRect(RECT * pRect)
{
    ::GetClientRect(*this, pRect);
    pRect->top += CDPIAware::Instance().ScaleY(HEADER_HEIGHT);
    if (HasMultipleImages())
    {
        pRect->top += CDPIAware::Instance().ScaleY(HEADER_HEIGHT);
    }
    if (pSecondPic)
        pRect->left += CDPIAware::Instance().ScaleX(SLIDER_WIDTH);
}

void CPicWindow::GetClientRectWithScrollbars(RECT * pRect)
{
    GetClientRect(pRect);
    ::GetWindowRect(*this, pRect);
    pRect->right = pRect->right-pRect->left;
    pRect->bottom = pRect->bottom-pRect->top;
    pRect->top = 0;
    pRect->left = 0;
    pRect->top += CDPIAware::Instance().ScaleY(HEADER_HEIGHT);
    if (HasMultipleImages())
    {
        pRect->top += CDPIAware::Instance().ScaleY(HEADER_HEIGHT);
    }
    if (pSecondPic)
        pRect->left += CDPIAware::Instance().ScaleX(SLIDER_WIDTH);
};


void CPicWindow::SetZoom(int Zoom, bool centermouse, bool inzoom)
{
    // Set the interpolation mode depending on zoom
    int oldPicscale = picscale;
    int oldOtherPicscale = picscale;

    picture.SetInterpolationMode(InterpolationModeNearestNeighbor);
    if (pSecondPic)
        pSecondPic->SetInterpolationMode(InterpolationModeNearestNeighbor);

    if ((oldPicscale == 0) || (Zoom == 0))
        return;

    picscale = Zoom;

    if (pTheOtherPic && !inzoom)
    {
        if (bOverlap)
        {
            pTheOtherPic->SetZoom(Zoom, centermouse, true);
        }
        if (bFitHeights)
        {
            m_linkedHeight = 0;
            pTheOtherPic->SetZoomToHeight(picture.m_Height*Zoom/100);
        }
        if (bFitWidths)
        {
            m_linkedWidth = 0;
            pTheOtherPic->SetZoomToWidth(picture.m_Width*Zoom/100);
        }
    }

    // adjust the scrollbar positions according to the new zoom and the
    // mouse position: if possible, keep the pixel where the mouse pointer
    // is at the same position after the zoom
    if (!inzoom)
    {
        POINT cpos;
        DWORD ptW = GetMessagePos();
        cpos.x = GET_X_LPARAM(ptW);
        cpos.y = GET_Y_LPARAM(ptW);
        ScreenToClient(*this, &cpos);
        RECT clientrect;
        GetClientRect(&clientrect);
        if ((PtInRect(&clientrect, cpos))&&(centermouse))
        {
            // the mouse pointer is over our window
            nHScrollPos = (nHScrollPos + cpos.x)*Zoom/oldPicscale-cpos.x;
            nVScrollPos = (nVScrollPos + cpos.y)*Zoom/oldPicscale-cpos.y;
            if (pTheOtherPic && bMainPic)
            {
                int otherzoom = pTheOtherPic->GetZoom();
                nHSecondScrollPos = (nHSecondScrollPos + cpos.x)*otherzoom/oldOtherPicscale-cpos.x;
                nVSecondScrollPos = (nVSecondScrollPos + cpos.y)*otherzoom/oldOtherPicscale-cpos.y;
            }
        }
        else
        {
            nHScrollPos = (nHScrollPos + ((clientrect.right-clientrect.left)/2))*Zoom/oldPicscale-((clientrect.right-clientrect.left)/2);
            nVScrollPos = (nVScrollPos + ((clientrect.bottom-clientrect.top)/2))*Zoom/oldPicscale-((clientrect.bottom-clientrect.top)/2);
            if (pTheOtherPic && bMainPic)
            {
                int otherzoom = pTheOtherPic->GetZoom();
                nHSecondScrollPos = (nHSecondScrollPos + ((clientrect.right-clientrect.left)/2))*otherzoom/oldOtherPicscale-((clientrect.right-clientrect.left)/2);
                nVSecondScrollPos = (nVSecondScrollPos + ((clientrect.bottom-clientrect.top)/2))*otherzoom/oldOtherPicscale-((clientrect.bottom-clientrect.top)/2);
            }
        }
    }

    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::Zoom(bool in, bool centermouse)
{
    int zoomFactor;

    // Find correct zoom factor and quantize picscale
    if (picscale % 10)
    {
        picscale /= 10;
        picscale *= 10;
        if (!in)
            picscale += 10;
    }

    if (!in && picscale <= 20)
    {
        picscale = 10;
        zoomFactor = 0;
    }
    else if ((in && picscale < 100) || (!in && picscale <= 100))
    {
        zoomFactor = 10;
    }
    else if ((in && picscale < 200) || (!in && picscale <= 200))
    {
        zoomFactor = 20;
    }
    else
    {
        zoomFactor = 10;
    }

    // Set zoom
    if (in)
    {
        SetZoom(picscale+zoomFactor, centermouse);
    }
    else
    {
        SetZoom(picscale-zoomFactor, centermouse);
    }
}

void CPicWindow::FitImageInWindow()
{
    RECT rect;

    GetClientRectWithScrollbars(&rect);

    const auto border = CDPIAware::Instance().ScaleX(2);
    if (rect.right-rect.left)
    {
        int Zoom = 100;
        if (((rect.right - rect.left) > picture.m_Width + border) && ((rect.bottom - rect.top) > picture.m_Height + border))
        {
            // image is smaller than the window
            Zoom = 100;
        }
        else
        {
            // image is bigger than the window
            if (picture.m_Width > 0 && picture.m_Height > 0)
            {
                int xscale = (rect.right - rect.left - border) * 100 / picture.m_Width;
                int yscale = (rect.bottom - rect.top - border) * 100 / picture.m_Height;
                Zoom = min(yscale, xscale);
            }
        }
        if (pSecondPic)
        {
            if (((rect.right - rect.left) > pSecondPic->m_Width + border) && ((rect.bottom - rect.top) > pSecondPic->m_Height + border))
            {
                // image is smaller than the window
                if (pTheOtherPic)
                    pTheOtherPic->SetZoom(min(100, Zoom), false);
            }
            else
            {
                // image is bigger than the window
                int xscale = (rect.right - rect.left - border) * 100 / pSecondPic->m_Width;
                int yscale = (rect.bottom - rect.top - border) * 100 / pSecondPic->m_Height;
                if (pTheOtherPic)
                    pTheOtherPic->SetZoom(min(yscale, xscale), false);
            }
            nHSecondScrollPos = 0;
            nVSecondScrollPos = 0;
        }
        SetZoom(Zoom, false);
    }
    CenterImage();
    PositionChildren();
    InvalidateRect(*this, nullptr, TRUE);
}

void CPicWindow::CenterImage()
{
    RECT rect;
    GetClientRectWithScrollbars(&rect);
    const auto border = CDPIAware::Instance().ScaleX(2);
    long width = picture.m_Width*picscale / 100 + border;
    long height = picture.m_Height*picscale / 100 + border;
    if (pSecondPic && pTheOtherPic)
    {
        width = max(width, pSecondPic->m_Width*pTheOtherPic->GetZoom() / 100 + border);
        height = max(height, pSecondPic->m_Height*pTheOtherPic->GetZoom() / 100 + border);
    }

    bool bPicWidthBigger = (int(width) > (rect.right-rect.left));
    bool bPicHeightBigger = (int(height) > (rect.bottom-rect.top));
    // set the scroll position so that the image is drawn centered in the window
    // if the window is bigger than the image
    if (!bPicWidthBigger)
    {
        nHScrollPos = -((rect.right-rect.left+4)-int(width))/2;
        nHSecondScrollPos = nHScrollPos;
    }
    if (!bPicHeightBigger)
    {
        nVScrollPos = -((rect.bottom-rect.top+4)-int(height))/2;
        nVSecondScrollPos = nVScrollPos;
    }
    SetupScrollBars();
}

void CPicWindow::FitWidths(bool bFit)
{
    bFitWidths = bFit;

    SetZoom(GetZoom(), false);
}

void CPicWindow::FitHeights(bool bFit)
{
    bFitHeights = bFit;

    SetZoom(GetZoom(), false);
}

void CPicWindow::ShowPicWithBorder(HDC hdc, const RECT &bounds, CPicture &pic, int scale)
{
    ::SetBkColor(hdc, transparentColor);
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &bounds, nullptr, 0, nullptr);

    RECT picrect;
    picrect.left =  bounds.left - nHScrollPos;
    picrect.top = bounds.top - nVScrollPos;
    if ((!bLinkedPositions || bOverlap) && (pTheOtherPic) && (&pic != &picture))
    {
        picrect.left = bounds.left - nHSecondScrollPos;
        picrect.top  = bounds.top - nVSecondScrollPos;
    }
    picrect.right = (picrect.left + pic.m_Width * scale / 100);
    picrect.bottom = (picrect.top + pic.m_Height * scale / 100);

    if (bFitWidths && m_linkedWidth)
        picrect.right = picrect.left + m_linkedWidth;
    if (bFitHeights && m_linkedHeight)
        picrect.bottom = picrect.top + m_linkedHeight;

    pic.Show(hdc, picrect);

    const auto bordersize = CDPIAware::Instance().ScaleX(1);

    RECT border;
    border.left = picrect.left - bordersize;
    border.top = picrect.top - bordersize;
    border.right = picrect.right + bordersize;
    border.bottom = picrect.bottom + bordersize;

    HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
    HPEN hOldPen = static_cast<HPEN>(SelectObject(hdc, hPen));
    MoveToEx(hdc, border.left, border.top, nullptr);
    LineTo(hdc, border.left, border.bottom);
    LineTo(hdc, border.right, border.bottom);
    LineTo(hdc, border.right, border.top);
    LineTo(hdc, border.left, border.top);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void CPicWindow::Paint(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rect, fullrect;

    GetUpdateRect(hwnd, &rect, FALSE);
    if (IsRectEmpty(&rect))
        return;

    const auto slider_width = CDPIAware::Instance().ScaleX(SLIDER_WIDTH);
    const auto border = CDPIAware::Instance().ScaleX(4);
    ::GetClientRect(*this, &fullrect);
    hdc = BeginPaint(hwnd, &ps);
    {
        // Exclude the alpha control and button
        if ((pSecondPic)&&(m_blend == BLEND_ALPHA))
            ExcludeClipRect(hdc, 0, m_inforect.top - border, slider_width, m_inforect.bottom + border);

        CMyMemDC memDC(hdc);
        if ((pSecondPic)&&(m_blend != BLEND_ALPHA))
        {
            // erase the place where the alpha slider would be
            ::SetBkColor(memDC, transparentColor);
            RECT bounds = { 0, m_inforect.top - border, slider_width, m_inforect.bottom + border };
            ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &bounds, nullptr, 0, nullptr);
        }

        GetClientRect(&rect);
        if (bValid)
        {
            ShowPicWithBorder(memDC, rect, picture, picscale);
            if (pSecondPic)
            {
                HDC secondhdc = CreateCompatibleDC(hdc);
                HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
                HBITMAP hOldBitmap = static_cast<HBITMAP>(SelectObject(secondhdc, hBitmap));
                SetWindowOrgEx(secondhdc, rect.left, rect.top, nullptr);

                if ((pSecondPic)&&(m_blend != BLEND_ALPHA))
                {
                    // erase the place where the alpha slider would be
                    ::SetBkColor(secondhdc, transparentColor);
                    RECT bounds = { 0, m_inforect.top - border, slider_width, m_inforect.bottom + border };
                    ::ExtTextOut(secondhdc, 0, 0, ETO_OPAQUE, &bounds, nullptr, 0, nullptr);
                }
                if (pTheOtherPic)
                    ShowPicWithBorder(secondhdc, rect, *pSecondPic, pTheOtherPic->GetZoom());

                if (m_blend == BLEND_ALPHA)
                {
                    BLENDFUNCTION blender;
                    blender.AlphaFormat = 0;
                    blender.BlendFlags = 0;
                    blender.BlendOp = AC_SRC_OVER;
                    blender.SourceConstantAlpha = static_cast<BYTE>(blendAlpha * 255);
                    AlphaBlend(memDC,
                        rect.left,
                        rect.top,
                        rect.right-rect.left,
                        rect.bottom-rect.top,
                        secondhdc,
                        rect.left,
                        rect.top,
                        rect.right-rect.left,
                        rect.bottom-rect.top,
                        blender);
                }
                else if (m_blend == BLEND_XOR)
                {
                    BitBlt(memDC,
                        rect.left,
                        rect.top,
                        rect.right-rect.left,
                        rect.bottom-rect.top,
                        secondhdc,
                        rect.left,
                        rect.top,
                        //rect.right-rect.left,
                        //rect.bottom-rect.top,
                        SRCINVERT);
                    InvertRect(memDC, &rect);
                }
                SelectObject(secondhdc, hOldBitmap);
                DeleteObject(hBitmap);
                DeleteDC(secondhdc);
            }
            else if (bDragging && pTheOtherPic && !bLinkedPositions)
            {
                // when dragging, show lines indicating the position of the other image
                HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(/*COLOR_ACTIVEBORDER*/COLOR_HIGHLIGHT));
                HPEN hOldPen = static_cast<HPEN>(SelectObject(memDC, hPen));
                int xpos = rect.left - pTheOtherPic->nHScrollPos - 1;
                MoveToEx(memDC, xpos, rect.top, nullptr);
                LineTo(memDC, xpos, rect.bottom);
                xpos = rect.left - pTheOtherPic->nHScrollPos + pTheOtherPic->picture.m_Width*pTheOtherPic->GetZoom()/100 + 1;
                if (bFitWidths && m_linkedWidth)
                    xpos = rect.left + pTheOtherPic->m_linkedWidth + 1;
                MoveToEx(memDC, xpos, rect.top, nullptr);
                LineTo(memDC, xpos, rect.bottom);

                int ypos = rect.top - pTheOtherPic->nVScrollPos - 1;
                MoveToEx(memDC, rect.left, ypos, nullptr);
                LineTo(memDC, rect.right, ypos);
                ypos = rect.top - pTheOtherPic->nVScrollPos + pTheOtherPic->picture.m_Height*pTheOtherPic->GetZoom()/100 + 1;
                if (bFitHeights && m_linkedHeight)
                    ypos = rect.top - pTheOtherPic->m_linkedHeight + 1;
                MoveToEx(memDC, rect.left, ypos, nullptr);
                LineTo(memDC, rect.right, ypos);

                SelectObject(memDC, hOldPen);
                DeleteObject(hPen);
            }

            int sliderwidth = 0;
            if ((pSecondPic)&&(m_blend == BLEND_ALPHA))
                sliderwidth = slider_width;
            m_inforect.left = rect.left + border + sliderwidth;
            m_inforect.top = rect.top;
            m_inforect.right = rect.right+sliderwidth;
            m_inforect.bottom = rect.bottom;

            SetBkColor(memDC, transparentColor);
            if (bShowInfo)
            {
                auto infostring = std::make_unique<TCHAR[]>(8192);
                BuildInfoString(infostring.get(), 8192, false);
                // set the font
                NONCLIENTMETRICS metrics = {0};
                metrics.cbSize = sizeof(NONCLIENTMETRICS);
                SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
                HFONT hFont = CreateFontIndirect(&metrics.lfStatusFont);
                HFONT hFontOld = static_cast<HFONT>(SelectObject(memDC, hFont));
                // find out how big the rectangle for the text has to be
                DrawText(memDC, infostring.get(), -1, &m_inforect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER | DT_CALCRECT);

                // the text should be drawn with a four pixel offset to the window borders
                m_inforect.top = rect.bottom - (m_inforect.bottom-m_inforect.top) - border;
                m_inforect.bottom = rect.bottom-4;

                // first draw an edge rectangle
                RECT edgerect;
                edgerect.left = m_inforect.left - border;
                edgerect.top = m_inforect.top - border;
                edgerect.right = m_inforect.right + border;
                edgerect.bottom = m_inforect.bottom + border;
                ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &edgerect, nullptr, 0, nullptr);
                DrawEdge(memDC, &edgerect, EDGE_BUMP, BF_RECT | BF_SOFT);

                SetTextColor(memDC, GetSysColor(COLOR_WINDOWTEXT));
                DrawText(memDC, infostring.get(), -1, &m_inforect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER);
                SelectObject(memDC, hFontOld);
                DeleteObject(hFont);
            }
        }
        else
        {
            SetBkColor(memDC, ::GetSysColor(COLOR_WINDOW));
            SetTextColor(memDC, ::GetSysColor(COLOR_WINDOWTEXT));
            ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, nullptr, 0, nullptr);
            SIZE stringsize;
            ResString str = ResString(hResource, IDS_INVALIDIMAGEINFO);

            // set the font
            NONCLIENTMETRICS metrics = {0};
            metrics.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
            HFONT hFont = CreateFontIndirect(&metrics.lfStatusFont);
            HFONT hFontOld = static_cast<HFONT>(SelectObject(memDC, hFont));

            if (GetTextExtentPoint32(memDC, str, static_cast<int>(wcslen(str)), &stringsize))
            {
                int nStringLength = stringsize.cx;

                ExtTextOut(memDC,
                    max(rect.left + ((rect.right - rect.left) - nStringLength) / 2, 1l),
                    rect.top + ((rect.bottom-rect.top) - stringsize.cy)/2,
                    ETO_CLIPPED,
                    &rect,
                    str,
                    static_cast<UINT>(wcslen(str)),
                    nullptr);
            }
            SelectObject(memDC, hFontOld);
            DeleteObject(hFont);
        }
        DrawViewTitle(memDC, &fullrect);
    }
    EndPaint(hwnd, &ps);
}

bool CPicWindow::CreateButtons()
{
    // Ensure that the common control DLL is loaded.
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC  = ICC_BAR_CLASSES | ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icex);

    hwndLeftBtn = CreateWindowEx(0,
                                L"BUTTON",
                                static_cast<LPCTSTR>(nullptr),
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                0, 0, 0, 0,
                                *this,
                                reinterpret_cast<HMENU>(LEFTBUTTON_ID),
                                hResource,
                                nullptr);
    if (hwndLeftBtn == INVALID_HANDLE_VALUE)
        return false;
    int iconWidth = GetSystemMetrics(SM_CXSMICON);
    int iconHeight = GetSystemMetrics(SM_CYSMICON);
    hLeft = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_BACKWARD), iconWidth, iconHeight);
    SendMessage(hwndLeftBtn, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hLeft));
    hwndRightBtn = CreateWindowEx(0,
                                L"BUTTON",
                                static_cast<LPCTSTR>(nullptr),
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                0, 0, 0, 0,
                                *this,
                                reinterpret_cast<HMENU>(RIGHTBUTTON_ID),
                                hResource,
                                nullptr);
    if (hwndRightBtn == INVALID_HANDLE_VALUE)
        return false;
    hRight = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_FORWARD), iconWidth, iconHeight);
    SendMessage(hwndRightBtn, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hRight));
    hwndPlayBtn = CreateWindowEx(0,
                                L"BUTTON",
                                static_cast<LPCTSTR>(nullptr),
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                0, 0, 0, 0,
                                *this,
                                reinterpret_cast<HMENU>(PLAYBUTTON_ID),
                                hResource,
                                nullptr);
    if (hwndPlayBtn == INVALID_HANDLE_VALUE)
        return false;
    hPlay = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_START), iconWidth, iconHeight);
    hStop = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_STOP), iconWidth, iconHeight);
    SendMessage(hwndPlayBtn, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hPlay));
    hwndAlphaToggleBtn = CreateWindowEx(0,
                                L"BUTTON",
                                static_cast<LPCTSTR>(nullptr),
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT | BS_NOTIFY | BS_PUSHLIKE,
                                0, 0, 0, 0,
                                *this,
                                reinterpret_cast<HMENU>(ALPHATOGGLEBUTTON_ID),
                                hResource,
                                nullptr);
    if (hwndAlphaToggleBtn == INVALID_HANDLE_VALUE)
        return false;
    hAlphaToggle = LoadIconEx(hResource, MAKEINTRESOURCE(IDI_ALPHATOGGLE), iconWidth, iconHeight);
    SendMessage(hwndAlphaToggleBtn, BM_SETIMAGE, IMAGE_ICON, reinterpret_cast<LPARAM>(hAlphaToggle));

    TOOLINFO ti = {0};
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND|TTF_SUBCLASS;
    ti.hwnd = *this;
    ti.hinst = hResource;
    ti.uId = reinterpret_cast<UINT_PTR>(hwndAlphaToggleBtn);
    ti.lpszText = LPSTR_TEXTCALLBACK;
    // ToolTip control will cover the whole window
    ti.rect.left = 0;
    ti.rect.top = 0;
    ti.rect.right = 0;
    ti.rect.bottom = 0;
    SendMessage(hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
    ResString sSelect(hResource, IDS_SELECT);
    hwndSelectBtn = CreateWindowEx(0,
                                   L"BUTTON",
                                   sSelect,
                                   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                   0, 0, 0, 0,
                                   *this,
                                   reinterpret_cast<HMENU>(SELECTBUTTON_ID),
                                   hResource,
                                   nullptr);
    if (hwndPlayBtn == INVALID_HANDLE_VALUE)
        return false;

    return true;
}

void CPicWindow::PositionChildren()
{
    const auto header_height = CDPIAware::Instance().ScaleY(HEADER_HEIGHT);
    const auto selBorder = CDPIAware::Instance().ScaleX(100);
    RECT rect;
    ::GetClientRect(*this, &rect);
    if (HasMultipleImages())
    {
        int iconWidth = GetSystemMetrics(SM_CXSMICON);
        int iconHeight = GetSystemMetrics(SM_CYSMICON);
        SetWindowPos(hwndLeftBtn, HWND_TOP, rect.left + iconWidth / 4, rect.top + header_height + (header_height -iconHeight)/2, iconWidth, iconHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        SetWindowPos(hwndRightBtn, HWND_TOP, rect.left + iconWidth + iconWidth / 2, rect.top + header_height + (header_height - iconHeight) / 2, iconWidth, iconHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        if (nFrames > 1)
            SetWindowPos(hwndPlayBtn, HWND_TOP, rect.left + iconWidth * 2 + iconWidth / 2, rect.top + header_height + (header_height - iconHeight) / 2, iconWidth, iconHeight, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        else
            ShowWindow(hwndPlayBtn, SW_HIDE);
    }
    else
    {
        ShowWindow(hwndLeftBtn, SW_HIDE);
        ShowWindow(hwndRightBtn, SW_HIDE);
        ShowWindow(hwndPlayBtn, SW_HIDE);
    }
    if (bSelectionMode)
        SetWindowPos(hwndSelectBtn, HWND_TOP, rect.right - selBorder, rect.bottom - header_height, selBorder, header_height, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    else
        ShowWindow(hwndSelectBtn, SW_HIDE);
    PositionTrackBar();
}

bool CPicWindow::HasMultipleImages()
{
    return (((nDimensions > 1) || (nFrames > 1)) && (!pSecondPic));
}

void CPicWindow::CreateTrackbar(HWND hwndParent)
{
    HWND hwndTrack = CreateWindowEx(
        0,                                  // no extended styles
        TRACKBAR_CLASS,                     // class name
        L"Trackbar Control",             // title (caption)
        WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_TOOLTIPS | TBS_AUTOTICKS,                // style
        10, 10,                             // position
        200, 30,                            // size
        hwndParent,                         // parent window
        reinterpret_cast<HMENU>(TRACKBAR_ID),// control identifier
        hInst,                              // instance
        nullptr                             // no WM_CREATE parameter
        );

    SendMessage(hwndTrack, TBM_SETRANGE,
        TRUE,                  // redraw flag
        MAKELONG(0, 16));      // min. & max. positions
    SendMessage(hwndTrack, TBM_SETTIPSIDE,
        TBTS_TOP,
        0);

    m_AlphaSlider.ConvertTrackbarToNice(hwndTrack);
}

void CPicWindow::BuildInfoString(TCHAR * buf, int size, bool bTooltip)
{
    // Unfortunately, we need two different strings for the tooltip
    // and the info box. Because the tooltips use a different tab size
    // than ExtTextOut(), and to keep the output aligned we therefore
    // need two different strings.
    // Note: some translations could end up with two identical strings, but
    // in English we need two - even if we wouldn't need two in English, some
    // translation might then need two again.
    if (pSecondPic && pTheOtherPic)
    {
        swprintf_s(buf, size,
            static_cast<const TCHAR*>(ResString(hResource, bTooltip ? IDS_DUALIMAGEINFOTT : IDS_DUALIMAGEINFO)),
            picture.GetFileSizeAsText().c_str(), picture.GetFileSizeAsText(false).c_str(),
            picture.m_Width, picture.m_Height,
            picture.GetHorizontalResolution(), picture.GetVerticalResolution(),
            picture.m_ColorDepth,
            static_cast<UINT>(GetZoom()),
            pSecondPic->GetFileSizeAsText().c_str(), pSecondPic->GetFileSizeAsText(false).c_str(),
            pSecondPic->m_Width, pSecondPic->m_Height,
            pSecondPic->GetHorizontalResolution(), pSecondPic->GetVerticalResolution(),
            pSecondPic->m_ColorDepth,
            static_cast<UINT>(pTheOtherPic->GetZoom()));
    }
    else
    {
        swprintf_s(buf, size,
            static_cast<const TCHAR*>(ResString(hResource, bTooltip ? IDS_IMAGEINFOTT : IDS_IMAGEINFO)),
            picture.GetFileSizeAsText().c_str(), picture.GetFileSizeAsText(false).c_str(),
            picture.m_Width, picture.m_Height,
            picture.GetHorizontalResolution(), picture.GetVerticalResolution(),
            picture.m_ColorDepth,
            static_cast<UINT>(GetZoom()));
    }
}

void CPicWindow::SetZoomToWidth( long width )
{
    m_linkedWidth = width;
    if (picture.m_Width)
    {
        int zoom = width*100/picture.m_Width;
        SetZoom(zoom, false, true);
    }
}

void CPicWindow::SetZoomToHeight( long height )
{
    m_linkedHeight = height;
    if (picture.m_Height)
    {
        int zoom = height*100/picture.m_Height;
        SetZoom(zoom, false, true);
    }
}
