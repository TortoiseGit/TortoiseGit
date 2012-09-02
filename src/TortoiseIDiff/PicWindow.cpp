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
#include "shellapi.h"
#include "commctrl.h"
#include "PicWindow.h"
#include "math.h"
#include "SysInfo.h"

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
    wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcx.lpszClassName = _T("TortoiseIDiffPicWindow");
    wcx.hIcon = LoadIcon(hResource, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
    wcx.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wcx.lpszMenuName = MAKEINTRESOURCE(IDC_TORTOISEIDIFF);
    wcx.hIconSm = LoadIcon(wcx.hInstance, MAKEINTRESOURCE(IDI_TORTOISEIDIFF));
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
    RECT rc;
    GetClientRect(&rc);
    HWND slider = m_AlphaSlider.GetWindow();
    if ((pSecondPic)&&(m_blend == BLEND_ALPHA))
    {
        MoveWindow(slider, 0, rc.top-4+SLIDER_WIDTH, SLIDER_WIDTH, rc.bottom-rc.top-SLIDER_WIDTH+8, true);
        ShowWindow(slider, SW_SHOW);
        MoveWindow(hwndAlphaToggleBtn, 0, rc.top-4, SLIDER_WIDTH, SLIDER_WIDTH, true);
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

            hwndTT = CreateWindowEx(WS_EX_TOPMOST,
                TOOLTIPS_CLASS,
                NULL,
                WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                hwnd,
                NULL,
                hResource,
                NULL
                );

            SetWindowPos(hwndTT,
                HWND_TOPMOST,
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

            SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
            SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
            nHSecondScrollPos = 0;
            nVSecondScrollPos = 0;
        }
        break;
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        InvalidateRect(*this, NULL, FALSE);
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
        if ((pSecondPic)&&((HWND)lParam == m_AlphaSlider.GetWindow()))
        {
            if (LOWORD(wParam) == TB_THUMBTRACK)
            {
                // while tracking, only redraw after 50 milliseconds
                ::SetTimer(*this, TIMER_ALPHASLIDER, 50, NULL);
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
            if (bLinkedPositions)
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
            if (bLinkedPositions)
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
            if (bFitSizes)
                pTheOtherPic->OnMouseWheel(GET_KEYSTATE_WPARAM(wParam), GET_WHEEL_DELTA_WPARAM(wParam));
        }
        break;
    case WM_MOUSEHWHEEL:
        {
            OnMouseWheel(GET_KEYSTATE_WPARAM(wParam)|MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
            if (bFitSizes)
                pTheOtherPic->OnMouseWheel(GET_KEYSTATE_WPARAM(wParam)|MK_SHIFT, GET_WHEEL_DELTA_WPARAM(wParam));
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
        SetCapture(*this);
        break;
    case WM_LBUTTONUP:
        ReleaseCapture();
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
            POINT pt = {((int)(short)LOWORD(lParam)), ((int)(short)HIWORD(lParam))};
            if (pt.y < HEADER_HEIGHT)
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
                    SendMessage(hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
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
                }
                SetupScrollBars();
                InvalidateRect(*this, NULL, TRUE);
                UpdateWindow(*this);
                if ((bLinkedPositions)&&((wParam & MK_SHIFT)==0))
                {
                    pTheOtherPic->nHScrollPos = nHScrollPos;
                    pTheOtherPic->nVScrollPos = nVScrollPos;
                    pTheOtherPic->SetupScrollBars();
                    InvalidateRect(*pTheOtherPic, NULL, TRUE);
                    UpdateWindow(*pTheOtherPic);
                }
            }
        }
        break;
    case WM_SETCURSOR:
        {
            // we show a hand cursor if the image can be dragged,
            // and a hand-down cursor if the image is currently dragged
            if ((*this == (HWND)wParam)&&(LOWORD(lParam)==HTCLIENT))
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
            HDROP hDrop = (HDROP)wParam;
            TCHAR szFileName[MAX_PATH];
            // we only use the first file dropped (if multiple files are dropped)
            if (DragQueryFile(hDrop, 0, szFileName, _countof(szFileName)))
            {
                SetPic(szFileName, _T(""), bMainPic);
                FitImageInWindow();
                InvalidateRect(*this, NULL, TRUE);
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
                    if (bLinkedPositions)
                        pTheOtherPic->PrevImage();
                    return 0;
                }
                break;
            case RIGHTBUTTON_ID:
                {
                    NextImage();
                    if (bLinkedPositions)
                        pTheOtherPic->NextImage();
                    return 0;
                }
                break;
            case PLAYBUTTON_ID:
                {
                    bPlaying = !bPlaying;
                    Animate(bPlaying);
                    if (bLinkedPositions)
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
                            SetTimer(*this, ID_ALPHATOGGLETIMER, 1000, NULL);
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
                    InvalidateRect(*this, NULL, TRUE);
                }
                break;
            case BLENDXOR_ID:
                {
                    m_blend = BLEND_XOR;
                    PositionTrackBar();
                    InvalidateRect(*this, NULL, TRUE);
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
                    delay = max(100, delay);
                    SetTimer(*this, ID_ANIMATIONTIMER, delay, NULL);
                    InvalidateRect(*this, NULL, FALSE);
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
            LPNMHDR pNMHDR = (LPNMHDR)lParam;
            if (pNMHDR->code == TTN_GETDISPINFO)
            {
                if (pNMHDR->hwndFrom == m_AlphaSlider.GetWindow())
                {
                    LPTOOLTIPTEXT lpttt = (LPTOOLTIPTEXT) lParam;
                    lpttt->hinst = hResource;
                    TCHAR stringbuf[MAX_PATH] = {0};
                    _stprintf_s(stringbuf, _T("%i%% alpha"), (int)(SendMessage(m_AlphaSlider.GetWindow(),TBM_GETPOS,0,0)/16.0f*100.0f));
                    _tcscpy_s(lpttt->lpszText, 80, stringbuf);
                }
                else if (pNMHDR->idFrom == (UINT_PTR)hwndAlphaToggleBtn)
                {
                    _stprintf_s(m_wszTip, (TCHAR const *)ResString(hResource, IDS_ALPHABUTTONTT), (int)(SendMessage(m_AlphaSlider.GetWindow(),TBM_GETPOS,0,0)/16.0f*100.0f));
                    if (pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        NMTTDISPINFOW* pTTTW = (NMTTDISPINFOW*)pNMHDR;
                        pTTTW->lpszText = m_wszTip;
                    }
                    else
                    {
                        NMTTDISPINFOA* pTTTA = (NMTTDISPINFOA*)pNMHDR;
                        pTTTA->lpszText = m_szTip;
                        ::WideCharToMultiByte(CP_ACP, 0, m_wszTip, -1, m_szTip, 8192, NULL, NULL);
                    }
                }
                else
                {
                    BuildInfoString(m_wszTip, _countof(m_wszTip), true);
                    if (pNMHDR->code == TTN_NEEDTEXTW)
                    {
                        NMTTDISPINFOW* pTTTW = (NMTTDISPINFOW*)pNMHDR;
                        pTTTW->lpszText = m_wszTip;
                    }
                    else
                    {
                        NMTTDISPINFOA* pTTTA = (NMTTDISPINFOA*)pNMHDR;
                        pTTTA->lpszText = m_szTip;
                        ::WideCharToMultiByte(CP_ACP, 0, m_wszTip, -1, m_szTip, 8192, NULL, NULL);
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
    InvalidateRect(*this, NULL, FALSE);
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
    InvalidateRect(*this, NULL, FALSE);
    PositionChildren();
}

void CPicWindow::Animate(bool bStart)
{
    if (bStart)
    {
        SendMessage(hwndPlayBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hStop);
        SetTimer(*this, ID_ANIMATIONTIMER, 0, NULL);
    }
    else
    {
        SendMessage(hwndPlayBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hPlay);
        KillTimer(*this, ID_ANIMATIONTIMER);
    }
}

void CPicWindow::SetPic(tstring path, tstring title, bool bFirst)
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
        picscale = 1.0;
        PositionChildren();
        InvalidateRect(*this, NULL, FALSE);
    }
}

void CPicWindow::DrawViewTitle(HDC hDC, RECT * rect)
{
    HFONT hFont = NULL;
    hFont = CreateFont(-MulDiv(pSecondPic ? 8 : 10, GetDeviceCaps(hDC, LOGPIXELSY), 72), 0, 0, 0, FW_DONTCARE, false, false, false, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, _T("MS Shell Dlg"));
    HFONT hFontOld = (HFONT)SelectObject(hDC, (HGDIOBJ)hFont);

    RECT textrect;
    textrect.left = rect->left;
    textrect.top = rect->top;
    textrect.right = rect->right;
    textrect.bottom = rect->top + HEADER_HEIGHT;
    if (HasMultipleImages())
        textrect.bottom += HEADER_HEIGHT;

    COLORREF crBk, crFg;
    crBk = ::GetSysColor(COLOR_SCROLLBAR);
    crFg = ::GetSysColor(COLOR_WINDOWTEXT);
    SetBkColor(hDC, crBk);
    ::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &textrect, NULL, 0, NULL);

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
        TCHAR buf[MAX_PATH];
        if (nFrames > 1)
            _stprintf_s(buf, (const TCHAR *)ResString(hResource, IDS_DIMENSIONSANDFRAMES), nCurrentFrame, nFrames);
        else
            _stprintf_s(buf, (const TCHAR *)ResString(hResource, IDS_DIMENSIONSANDFRAMES), nCurrentDimension, nDimensions);
        imgnumstring = buf;
    }

    SIZE stringsize;
    if (GetTextExtentPoint32(hDC, realtitle.c_str(), (int)realtitle.size(), &stringsize))
    {
        int nStringLength = stringsize.cx;
        int texttop = pSecondPic ? textrect.top + (HEADER_HEIGHT/2) - stringsize.cy : textrect.top + (HEADER_HEIGHT/2) - stringsize.cy/2;
        ExtTextOut(hDC,
            max(textrect.left + ((textrect.right-textrect.left)-nStringLength)/2, 1),
            texttop,
            ETO_CLIPPED,
            &textrect,
            realtitle.c_str(),
            (UINT)realtitle.size(),
            NULL);
        if (pSecondPic)
        {
            realtitle = (pictitle2.empty() ? picpath2 : pictitle2);
            ExtTextOut(hDC,
                max(textrect.left + ((textrect.right-textrect.left)-nStringLength)/2, 1),
                texttop + stringsize.cy,
                ETO_CLIPPED,
                &textrect,
                realtitle.c_str(),
                (UINT)realtitle.size(),
                NULL);
        }
    }
    if (HasMultipleImages())
    {
        if (GetTextExtentPoint32(hDC, imgnumstring.c_str(), (int)imgnumstring.size(), &stringsize))
        {
            int nStringLength = stringsize.cx;

            ExtTextOut(hDC,
                max(textrect.left + ((textrect.right-textrect.left)-nStringLength)/2, 1),
                textrect.top + HEADER_HEIGHT + (HEADER_HEIGHT/2) - stringsize.cy/2,
                ETO_CLIPPED,
                &textrect,
                imgnumstring.c_str(),
                (UINT)imgnumstring.size(),
                NULL);
        }
    }
    SelectObject(hDC, (HGDIOBJ)hFontOld);
    DeleteObject(hFont);
}

void CPicWindow::SetupScrollBars()
{
    RECT rect;
    GetClientRect(&rect);

    SCROLLINFO si = {sizeof(si)};

    si.fMask = SIF_POS | SIF_PAGE | SIF_RANGE | SIF_DISABLENOSCROLL;

    double width = double(picture.m_Width)*picscale;
    double height = double(picture.m_Height)*picscale;
    if (pSecondPic)
    {
        width = max(width, double(pSecondPic->m_Width)*pTheOtherPic->GetZoom());
        height = max(height, double(pSecondPic->m_Height)*pTheOtherPic->GetZoom());
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
        nVScrollPos = LONG(double(picture.GetHeight())*picscale);
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
    LONG height = LONG(double(picture.GetHeight())*picscale);
    if (pSecondPic)
    {
        height = max(height, LONG(double(pSecondPic->GetHeight())*picscale));
        nVSecondScrollPos = nVScrollPos;
    }
    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, NULL, TRUE);
}

void CPicWindow::OnHScroll(UINT nSBCode, UINT nPos)
{
    RECT rect;
    GetClientRect(&rect);

    switch (nSBCode)
    {
    case SB_RIGHT:
        nHScrollPos = LONG(double(picture.GetWidth())*picscale);
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
    LONG width = LONG(double(picture.GetWidth())*picscale);
    if (pSecondPic)
    {
        width = max(width, LONG(double(pSecondPic->GetWidth())*picscale));
        nHSecondScrollPos = nHScrollPos;
    }
    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, NULL, TRUE);
}

void CPicWindow::OnMouseWheel(short fwKeys, short zDelta)
{
    RECT rect;
    GetClientRect(&rect);
    LONG width = long(double(picture.m_Width)*picscale);
    LONG height = long(double(picture.m_Height)*picscale);
    if (pSecondPic)
    {
        width = max(width, long(double(pSecondPic->m_Width)*picscale));
        height = max(height, long(double(pSecondPic->m_Height)*picscale));
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
        if ((bFitSizes)&&(pTheOtherPic)&&(!bOverlap))
            pTheOtherPic->Zoom(zDelta>0, true);
        PositionChildren();
        InvalidateRect(*this, NULL, FALSE);
        SetWindowPos(*this, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOSIZE|SWP_NOREPOSITION|SWP_NOMOVE);
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
    pRect->top += HEADER_HEIGHT;
    if (HasMultipleImages())
    {
        pRect->top += HEADER_HEIGHT;
    }
    if (pSecondPic)
        pRect->left += SLIDER_WIDTH;
}

void CPicWindow::GetClientRectWithScrollbars(RECT * pRect)
{
    GetClientRect(pRect);
    ::GetWindowRect(*this, pRect);
    pRect->right = pRect->right-pRect->left;
    pRect->bottom = pRect->bottom-pRect->top;
    pRect->top = 0;
    pRect->left = 0;
    pRect->top += HEADER_HEIGHT;
    if (HasMultipleImages())
    {
        pRect->top += HEADER_HEIGHT;
    }
    if (pSecondPic)
        pRect->left += SLIDER_WIDTH;
};


void CPicWindow::SetZoom(double dZoom, bool centermouse)
{
    // Set the interpolation mode depending on zoom
    double oldPicscale = picscale;
    double oldOtherPicscale = picscale;
    if (dZoom < 1.0)
    {   // Zoomed out, use high quality bicubic
        picture.SetInterpolationMode(InterpolationModeHighQualityBicubic);
        if (pSecondPic)
            pSecondPic->SetInterpolationMode(InterpolationModeHighQualityBicubic);
    }
    else if (!((int)(dZoom*100.0)%100))
    {   // "Even" zoom sizes should be shown w-o any interpolation
        picture.SetInterpolationMode(InterpolationModeNearestNeighbor);
        if (pSecondPic)
            pSecondPic->SetInterpolationMode(InterpolationModeNearestNeighbor);
    }
    else
    {   // Arbitrary zoomed in, use bilinear that is semi-smoothed
        picture.SetInterpolationMode(InterpolationModeBilinear);
        if (pSecondPic)
            pSecondPic->SetInterpolationMode(InterpolationModeBilinear);
    }
    picscale = dZoom;

    if ((bFitSizes)&&(bMainPic))
    {
        double width, height;
        double zoomWidth, zoomHeight;
        width = double(picture.m_Width)*dZoom;
        height = double(picture.m_Height)*dZoom;
        zoomWidth = width/double(pTheOtherPic->GetPic()->m_Width);
        zoomHeight = height/double(pTheOtherPic->GetPic()->m_Height);
        oldOtherPicscale = pTheOtherPic->GetZoom();
        pTheOtherPic->SetZoom(min(zoomWidth, zoomHeight), false);
    }

    // adjust the scrollbar positions according to the new zoom and the
    // mouse position: if possible, keep the pixel where the mouse pointer
    // is at the same position after the zoom
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
        nHScrollPos = int(double(nHScrollPos + cpos.x)*(dZoom/oldPicscale))-cpos.x;
        nVScrollPos = int(double(nVScrollPos + cpos.y)*(dZoom/oldPicscale))-cpos.y;
        if (pTheOtherPic && bMainPic)
        {
            double otherzoom = pTheOtherPic->GetZoom();
            nHSecondScrollPos = int(double(nHSecondScrollPos + cpos.x)*(otherzoom/oldOtherPicscale))-cpos.x;
            nVSecondScrollPos = int(double(nVSecondScrollPos + cpos.y)*(otherzoom/oldOtherPicscale))-cpos.y;
        }
    }
    else
    {
        nHScrollPos = int(double(nHScrollPos + ((clientrect.right-clientrect.left)/2))*(dZoom/oldPicscale))-((clientrect.right-clientrect.left)/2);
        nVScrollPos = int(double(nVScrollPos + ((clientrect.bottom-clientrect.top)/2))*(dZoom/oldPicscale))-((clientrect.bottom-clientrect.top)/2);
        if (pTheOtherPic && bMainPic)
        {
            double otherzoom = pTheOtherPic->GetZoom();
            nHSecondScrollPos = int(double(nHSecondScrollPos + ((clientrect.right-clientrect.left)/2))*(otherzoom/oldOtherPicscale))-((clientrect.right-clientrect.left)/2);
            nVSecondScrollPos = int(double(nVSecondScrollPos + ((clientrect.bottom-clientrect.top)/2))*(otherzoom/oldOtherPicscale))-((clientrect.bottom-clientrect.top)/2);
        }
    }

    SetupScrollBars();
    PositionChildren();
    InvalidateRect(*this, NULL, TRUE);
}

void CPicWindow::Zoom(bool in, bool centermouse)
{
    double zoomFactor;

    // Find correct zoom factor and quantize picscale
    if (!in && picscale <= 0.2)
    {
        picscale = 0.1;
        zoomFactor = 0;
    }
    else if ((in && picscale < 1.0) || (!in && picscale <= 1.0))
    {
        picscale = 0.1 * RoundDouble(picscale/0.1, 0);  // Quantize to 0.1
        zoomFactor = 0.1;
    }
    else if ((in && picscale < 2.0) || (!in && picscale <= 2.0))
    {
        picscale = 0.25 * RoundDouble(picscale/0.25, 0);    // Quantize to 0.25
        zoomFactor = 0.25;
    }
    else
    {
        picscale = RoundDouble(picscale,0);
        zoomFactor = 1;
    }

    // Set zoom
    if (in)
    {
        if ((pSecondPic)&&(!bFitSizes))
            pTheOtherPic->SetZoom(pTheOtherPic->GetZoom()+zoomFactor, false);
        SetZoom(picscale+zoomFactor, centermouse);
    }
    else
    {
        if ((pSecondPic)&&(!bFitSizes))
            pTheOtherPic->SetZoom(pTheOtherPic->GetZoom()-zoomFactor, false);
        SetZoom(picscale-zoomFactor, centermouse);
    }
}

double CPicWindow::RoundDouble(double doValue, int nPrecision)
{
    static const double doBase = 10.0;
    double doComplete5, doComplete5i;

    doComplete5 = doValue * pow(doBase, (double) (nPrecision + 1));

    if (doValue < 0.0)
    {
        doComplete5 -= 5.0;
    }
    else
    {
        doComplete5 += 5.0;
    }

    doComplete5 /= doBase;
    modf(doComplete5, &doComplete5i);

    return doComplete5i / pow(doBase, (double) nPrecision);
}
void CPicWindow::FitImageInWindow()
{
    RECT rect;

    GetClientRectWithScrollbars(&rect);

    if (rect.right-rect.left)
    {
        double dZoom = 1.0;
        if (((rect.right - rect.left) > picture.m_Width+2)&&((rect.bottom - rect.top)> picture.m_Height+2))
        {
            // image is smaller than the window
            dZoom = 1.0;
        }
        else
        {
            // image is bigger than the window
            double xscale = double(rect.right-rect.left-2)/double(picture.m_Width);
            double yscale = double(rect.bottom-rect.top-2)/double(picture.m_Height);
            dZoom = min(yscale, xscale);
        }
        if (pSecondPic)
        {
            if (((rect.right - rect.left) > pSecondPic->m_Width+2)&&((rect.bottom - rect.top)> pSecondPic->m_Height+2))
            {
                // image is smaller than the window
                pTheOtherPic->SetZoom(min(1.0, dZoom), false);
            }
            else
            {
                // image is bigger than the window
                double xscale = double(rect.right-rect.left-2)/double(pSecondPic->m_Width);
                double yscale = double(rect.bottom-rect.top-2)/double(pSecondPic->m_Height);
                pTheOtherPic->SetZoom(min(yscale, xscale), false);
            }
            nHSecondScrollPos = 0;
            nVSecondScrollPos = 0;
        }
        SetZoom(dZoom, false);
    }
    CenterImage();
    PositionChildren();
    InvalidateRect(*this, NULL, TRUE);
}

void CPicWindow::CenterImage()
{
    RECT rect;
    GetClientRectWithScrollbars(&rect);
    double width = (double(picture.m_Width)*picscale) + 2.0;
    double height = (double(picture.m_Height)*picscale) + 2.0;
    if (pSecondPic)
    {
        width = max(width, (double(pSecondPic->m_Width)*pTheOtherPic->GetZoom()) + 2.0);
        height = max(height, (double(pSecondPic->m_Height)*pTheOtherPic->GetZoom()) + 2.0);
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

void CPicWindow::FitSizes(bool bFit)
{
    bFitSizes = bFit;

    SetZoom(GetZoom(), false);
}

void CPicWindow::ShowPicWithBorder(HDC hdc, const RECT &bounds, CPicture &pic, double scale)
{
    ::SetBkColor(hdc, transparentColor);
    ::ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &bounds, NULL, 0, NULL);

    RECT picrect;
    picrect.left =  bounds.left - nHScrollPos;
    picrect.top = bounds.top - nVScrollPos;
    if ((!bLinkedPositions || bOverlap) && (pTheOtherPic) && (&pic != &picture))
    {
        picrect.left = bounds.left - nHSecondScrollPos;
        picrect.top  = bounds.top - nVSecondScrollPos;
    }
    picrect.right = (picrect.left + LONG(double(pic.m_Width) * scale));
    picrect.bottom = (picrect.top + LONG(double(pic.m_Height) * scale));

    pic.Show(hdc, picrect);

    RECT border;
    border.left = picrect.left-1;
    border.top = picrect.top-1;
    border.right = picrect.right+1;
    border.bottom = picrect.bottom+1;

    HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DDKSHADOW));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, border.left, border.top, NULL);
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

    ::GetClientRect(*this, &fullrect);
    hdc = BeginPaint(hwnd, &ps);
    {
        // Exclude the alpha control and button
        if ((pSecondPic)&&(m_blend == BLEND_ALPHA))
            ExcludeClipRect(hdc, 0, m_inforect.top-4, SLIDER_WIDTH, m_inforect.bottom+4);

        CMyMemDC memDC(hdc);
        if ((pSecondPic)&&(m_blend != BLEND_ALPHA))
        {
            // erase the place where the alpha slider would be
            ::SetBkColor(memDC, transparentColor);
            RECT bounds = {0, m_inforect.top-4, SLIDER_WIDTH, m_inforect.bottom+4};
            ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &bounds, NULL, 0, NULL);
        }

        GetClientRect(&rect);
        if (bValid)
        {
            ShowPicWithBorder(memDC, rect, picture, picscale);
            if (pSecondPic)
            {
                HDC secondhdc = CreateCompatibleDC(hdc);
                HBITMAP hBitmap = CreateCompatibleBitmap(hdc, rect.right - rect.left, rect.bottom - rect.top);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(secondhdc, hBitmap);
                SetWindowOrgEx(secondhdc, rect.left, rect.top, NULL);

                if ((pSecondPic)&&(m_blend != BLEND_ALPHA))
                {
                    // erase the place where the alpha slider would be
                    ::SetBkColor(secondhdc, transparentColor);
                    RECT bounds = {0, m_inforect.top-4, SLIDER_WIDTH, m_inforect.bottom+4};
                    ::ExtTextOut(secondhdc, 0, 0, ETO_OPAQUE, &bounds, NULL, 0, NULL);
                }
                ShowPicWithBorder(secondhdc, rect, *pSecondPic, pTheOtherPic->GetZoom());

                if (m_blend == BLEND_ALPHA)
                {
                    BLENDFUNCTION blender;
                    blender.AlphaFormat = 0;
                    blender.BlendFlags = 0;
                    blender.BlendOp = AC_SRC_OVER;
                    blender.SourceConstantAlpha = (BYTE)(blendAlpha*255);
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
            int sliderwidth = 0;
            if ((pSecondPic)&&(m_blend == BLEND_ALPHA))
                sliderwidth = SLIDER_WIDTH;
            m_inforect.left = rect.left+4+sliderwidth;
            m_inforect.top = rect.top;
            m_inforect.right = rect.right+sliderwidth;
            m_inforect.bottom = rect.bottom;

            SetBkColor(memDC, transparentColor);
            if (bShowInfo)
            {
                TCHAR infostring[8192];
                BuildInfoString(infostring, _countof(infostring), false);
                // set the font
                NONCLIENTMETRICS metrics = {0};
                metrics.cbSize = sizeof(NONCLIENTMETRICS);

            #if (WINVER >= 0x600)
                if (!SysInfo::Instance().IsVistaOrLater())
                {
                    metrics.cbSize -= sizeof(int);  // subtract the size of the iPaddedBorderWidth member which is not available on XP
                }
            #endif

                SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
                HFONT hFont = CreateFontIndirect(&metrics.lfStatusFont);
                HFONT hFontOld = (HFONT)SelectObject(memDC, (HGDIOBJ)hFont);
                // find out how big the rectangle for the text has to be
                DrawText(memDC, infostring, -1, &m_inforect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER | DT_CALCRECT);

                // the text should be drawn with a four pixel offset to the window borders
                m_inforect.top = rect.bottom - (m_inforect.bottom-m_inforect.top) - 4;
                m_inforect.bottom = rect.bottom-4;

                // first draw an edge rectangle
                RECT edgerect;
                edgerect.left = m_inforect.left-4;
                edgerect.top = m_inforect.top-4;
                edgerect.right = m_inforect.right+4;
                edgerect.bottom = m_inforect.bottom+4;
                ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &edgerect, NULL, 0, NULL);
                DrawEdge(memDC, &edgerect, EDGE_BUMP, BF_RECT | BF_SOFT);

                SetTextColor(memDC, GetSysColor(COLOR_WINDOWTEXT));
                DrawText(memDC, infostring, -1, &m_inforect, DT_EDITCONTROL | DT_EXPANDTABS | DT_LEFT | DT_VCENTER);
                SelectObject(memDC, (HGDIOBJ)hFontOld);
                DeleteObject(hFont);
            }
        }
        else
        {
            SetBkColor(memDC, ::GetSysColor(COLOR_WINDOW));
            ::ExtTextOut(memDC, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
            SIZE stringsize;
            ResString str = ResString(hResource, IDS_INVALIDIMAGEINFO);

            // set the font
            NONCLIENTMETRICS metrics = {0};
            metrics.cbSize = sizeof(NONCLIENTMETRICS);
#if (WINVER >= 0x600)
            if (!SysInfo::Instance().IsVistaOrLater())
            {
                metrics.cbSize -= sizeof(int);  // subtract the size of the iPaddedBorderWidth member which is not available on XP
            }
#endif
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
            HFONT hFont = CreateFontIndirect(&metrics.lfStatusFont);
            HFONT hFontOld = (HFONT)SelectObject(memDC, (HGDIOBJ)hFont);

            if (GetTextExtentPoint32(memDC, str, (int)_tcslen(str), &stringsize))
            {
                int nStringLength = stringsize.cx;

                ExtTextOut(memDC,
                    max(rect.left + ((rect.right-rect.left)-nStringLength)/2, 1),
                    rect.top + ((rect.bottom-rect.top) - stringsize.cy)/2,
                    ETO_CLIPPED,
                    &rect,
                    str,
                    (UINT)_tcslen(str),
                    NULL);
            }
            SelectObject(memDC, (HGDIOBJ)hFontOld);
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
                                _T("BUTTON"),
                                (LPCTSTR)NULL,
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                0, 0, 0, 0,
                                (HWND)*this,
                                (HMENU)LEFTBUTTON_ID,
                                hResource,
                                NULL);
    if (hwndLeftBtn == INVALID_HANDLE_VALUE)
        return false;
    hLeft = (HICON)LoadImage(hResource, MAKEINTRESOURCE(IDI_BACKWARD), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
    SendMessage(hwndLeftBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hLeft);
    hwndRightBtn = CreateWindowEx(0,
                                _T("BUTTON"),
                                (LPCTSTR)NULL,
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                0, 0, 0, 0,
                                *this,
                                (HMENU)RIGHTBUTTON_ID,
                                hResource,
                                NULL);
    if (hwndRightBtn == INVALID_HANDLE_VALUE)
        return false;
    hRight = (HICON)LoadImage(hResource, MAKEINTRESOURCE(IDI_FORWARD), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
    SendMessage(hwndRightBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hRight);
    hwndPlayBtn = CreateWindowEx(0,
                                _T("BUTTON"),
                                (LPCTSTR)NULL,
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT,
                                0, 0, 0, 0,
                                *this,
                                (HMENU)PLAYBUTTON_ID,
                                hResource,
                                NULL);
    if (hwndPlayBtn == INVALID_HANDLE_VALUE)
        return false;
    hPlay = (HICON)LoadImage(hResource, MAKEINTRESOURCE(IDI_START), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
    hStop = (HICON)LoadImage(hResource, MAKEINTRESOURCE(IDI_STOP), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
    SendMessage(hwndPlayBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hPlay);
    hwndAlphaToggleBtn = CreateWindowEx(0,
                                _T("BUTTON"),
                                (LPCTSTR)NULL,
                                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_ICON | BS_FLAT | BS_NOTIFY | BS_PUSHLIKE,
                                0, 0, 0, 0,
                                (HWND)*this,
                                (HMENU)ALPHATOGGLEBUTTON_ID,
                                hResource,
                                NULL);
    if (hwndAlphaToggleBtn == INVALID_HANDLE_VALUE)
        return false;
    hAlphaToggle = (HICON)LoadImage(hResource, MAKEINTRESOURCE(IDI_ALPHATOGGLE), IMAGE_ICON, 16, 16, LR_LOADTRANSPARENT);
    SendMessage(hwndAlphaToggleBtn, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)hAlphaToggle);

    TOOLINFO ti = {0};
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND|TTF_SUBCLASS;
    ti.hwnd = *this;
    ti.hinst = hResource;
    ti.uId = (UINT_PTR)hwndAlphaToggleBtn;
    ti.lpszText = LPSTR_TEXTCALLBACK;
    // ToolTip control will cover the whole window
    ti.rect.left = 0;
    ti.rect.top = 0;
    ti.rect.right = 0;
    ti.rect.bottom = 0;
    SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);

    return true;
}

void CPicWindow::PositionChildren()
{
    RECT rect;
    ::GetClientRect(*this, &rect);
    if (HasMultipleImages())
    {
        SetWindowPos(hwndLeftBtn, HWND_TOP, rect.left+3, rect.top + HEADER_HEIGHT + (HEADER_HEIGHT-16)/2, 16, 16, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        SetWindowPos(hwndRightBtn, HWND_TOP, rect.left+23, rect.top + HEADER_HEIGHT + (HEADER_HEIGHT-16)/2, 16, 16, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        if (nFrames > 1)
            SetWindowPos(hwndPlayBtn, HWND_TOP, rect.left+43, rect.top + HEADER_HEIGHT + (HEADER_HEIGHT-16)/2, 16, 16, SWP_FRAMECHANGED|SWP_SHOWWINDOW);
        else
            ShowWindow(hwndPlayBtn, SW_HIDE);
    }
    else
    {
        ShowWindow(hwndLeftBtn, SW_HIDE);
        ShowWindow(hwndRightBtn, SW_HIDE);
        ShowWindow(hwndPlayBtn, SW_HIDE);
    }
    PositionTrackBar();
}

bool CPicWindow::HasMultipleImages()
{
    return (((nDimensions > 1)||(nFrames > 1))&&(pSecondPic == NULL));
}

void CPicWindow::CreateTrackbar(HWND hwndParent)
{
    HWND hwndTrack = CreateWindowEx(
        0,                                  // no extended styles
        TRACKBAR_CLASS,                     // class name
        _T("Trackbar Control"),             // title (caption)
        WS_CHILD | WS_VISIBLE | TBS_VERT | TBS_TOOLTIPS | TBS_AUTOTICKS,                // style
        10, 10,                             // position
        200, 30,                            // size
        hwndParent,                         // parent window
        (HMENU)TRACKBAR_ID,                 // control identifier
        hInst,                              // instance
        NULL                                // no WM_CREATE parameter
        );

    SendMessage(hwndTrack, TBM_SETRANGE,
        (WPARAM) TRUE,                  // redraw flag
        (LPARAM) MAKELONG(0, 16));      // min. & max. positions
    SendMessage(hwndTrack, TBM_SETTIPSIDE,
        (WPARAM) TBTS_TOP,
        (LPARAM) 0);

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
    if (pSecondPic)
    {
        _stprintf_s(buf, size,
            (TCHAR const *)ResString(hResource, bTooltip ? IDS_DUALIMAGEINFOTT : IDS_DUALIMAGEINFO),
            picture.GetFileSizeAsText().c_str(), picture.GetFileSizeAsText(false).c_str(),
            picture.m_Width, picture.m_Height,
            picture.GetHorizontalResolution(), picture.GetVerticalResolution(),
            picture.m_ColorDepth,
            (UINT)(GetZoom()*100.0),
            pSecondPic->GetFileSizeAsText().c_str(), pSecondPic->GetFileSizeAsText(false).c_str(),
            pSecondPic->m_Width, pSecondPic->m_Height,
            pSecondPic->GetHorizontalResolution(), pSecondPic->GetVerticalResolution(),
            pSecondPic->m_ColorDepth,
            (UINT)(pTheOtherPic->GetZoom()*100.0));
    }
    else
    {
        _stprintf_s(buf, size,
            (TCHAR const *)ResString(hResource, bTooltip ? IDS_IMAGEINFOTT : IDS_IMAGEINFO),
            picture.GetFileSizeAsText().c_str(), picture.GetFileSizeAsText(false).c_str(),
            picture.m_Width, picture.m_Height,
            picture.GetHorizontalResolution(), picture.GetVerticalResolution(),
            picture.m_ColorDepth,
            (UINT)(GetZoom()*100.0));
    }
}
