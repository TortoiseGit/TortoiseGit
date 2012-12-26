// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2006 - 2008, 2011 - TortoiseSVN

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
#include "NiceTrackbar.h"
#include <tchar.h>
#include <CommCtrl.h>
#include <WindowsX.h>


LRESULT CALLBACK CNiceTrackbar::NiceTrackbarProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    CNiceTrackbar* self = (CNiceTrackbar*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (message) {
    case WM_LBUTTONDOWN:
        self->m_Dragging = true;
        self->m_DragChanged = false;
        SetCapture(hwnd);
        //SetFocus(hwnd);
        if (self->SetThumb(lParam)) {
            self->m_DragChanged = true;
            self->PostMessageToParent(TB_THUMBTRACK);
        }
        return 0;
    case WM_MOUSEMOVE:
        if (self->m_Dragging)
        {
            if (self->SetThumb(lParam))
            {
                self->m_DragChanged = true;
                self->PostMessageToParent(TB_THUMBTRACK);
            }
            return 0;
        }
        break;
    case WM_LBUTTONUP:
        if (self->m_Dragging)
        {
            self->m_Dragging = false;
            ReleaseCapture();
            if (self->SetThumb(lParam))
            {
                self->PostMessageToParent(TB_ENDTRACK);
                self->m_DragChanged = true;
            }
            if (self->m_DragChanged)
            {
                self->PostMessageToParent(TB_THUMBPOSITION);
                self->m_DragChanged = false;
            }
            return 0;
        }
        break;
    case WM_CAPTURECHANGED:
        if (self->m_Dragging)
        {
            self->m_Dragging = false;
            return 0;
        }
        break;
    case WM_DESTROY:
        SetWindowLongPtr (hwnd, GWLP_WNDPROC, (LONG_PTR)self->m_OrigProc);
        break;
    }
    return CallWindowProc (self->m_OrigProc, hwnd, message, wParam, lParam);
}


void CNiceTrackbar::ConvertTrackbarToNice( HWND window )
{
    m_Window = window;

    // setup this pointer
    SetWindowLongPtr( window, GWLP_USERDATA, (LONG_PTR)this );

    // subclass it
    m_OrigProc = (WNDPROC)SetWindowLongPtr( window, GWLP_WNDPROC, (LONG_PTR)NiceTrackbarProc );
}


bool CNiceTrackbar::SetThumb (LPARAM lparamPoint)
{
    POINT point = { GET_X_LPARAM(lparamPoint), GET_Y_LPARAM(lparamPoint) };
    const int nMin = (int)SendMessage(m_Window, TBM_GETRANGEMIN, 0, 0l);
    const int nMax = (int)SendMessage(m_Window, TBM_GETRANGEMAX, 0, 0l);
    RECT rc;
    SendMessage(m_Window, TBM_GETCHANNELRECT, 0, (LPARAM)&rc);
    double ratio;
    if (GetWindowLong(m_Window, GWL_STYLE) & TBS_VERT)
    {
        // note: for vertical trackbar, it still returns the rectangle as if it was horizontal
        ratio = (double)(point.y - rc.left)/(rc.right - rc.left);
    }
    else
    {
        ratio = (double)(point.x - rc.left)/(rc.right - rc.left);
    }

    int nNewPos = (int)(nMin + (nMax-nMin)*ratio + 0.5); // round the result to go to the nearest tick mark

    const bool changed = (nNewPos != (int)SendMessage(m_Window, TBM_GETPOS, 0, 0));
    if (changed)
    {
        SendMessage(m_Window, TBM_SETPOS, TRUE, nNewPos);
    }
    return changed;
}


void CNiceTrackbar::PostMessageToParent (int tbCode) const
{
    HWND parent = GetParent(m_Window);
    if (parent) {
        int pos = (int)SendMessage(m_Window, TBM_GETPOS, 0, 0);
        bool vert = (GetWindowLong(m_Window, GWL_STYLE) & TBS_VERT) != 0;
        PostMessage( parent, vert ? WM_VSCROLL : WM_HSCROLL, (WPARAM)((pos << 16) | tbCode), (LPARAM)m_Window );
    }
}
