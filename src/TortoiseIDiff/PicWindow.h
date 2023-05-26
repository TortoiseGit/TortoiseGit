// TortoiseIDiff - an image diff viewer in TortoiseSVN

// Copyright (C) 2020, 2023 - TortoiseGit
// Copyright (C) 2006-2010, 2012-2016, 2020 - TortoiseSVN

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
#include <CommCtrl.h>
#include "BaseWindow.h"
#include "TortoiseIDiff.h"
#include "Picture.h"
#include "NiceTrackbar.h"

#define HEADER_HEIGHT 30

#define ID_ANIMATIONTIMER 100
#define TIMER_ALPHASLIDER 101
#define ID_ALPHATOGGLETIMER 102

#define LEFTBUTTON_ID           101
#define RIGHTBUTTON_ID          102
#define PLAYBUTTON_ID           103
#define ALPHATOGGLEBUTTON_ID    104
#define BLENDALPHA_ID           105
#define BLENDXOR_ID             106
#define SELECTBUTTON_ID         107

#define TRACKBAR_ID 101
#define SLIDER_HEIGHT 30
#define SLIDER_WIDTH 30


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        static_cast<int>(static_cast<short>(LOWORD(lp)))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        static_cast<int>(static_cast<short>(HIWORD(lp)))
#endif

/**
 * \ingroup TortoiseIDiff
 * The image view window.
 * Shows an image and provides methods to scale the image or alpha blend it
 * over another image.
 */
class CPicWindow : public CWindow
{
private:
    CPicWindow() : CWindow(nullptr)
        , transparentColor(::GetSysColor(COLOR_WINDOW))
    {
        m_wszTip[0] = 0;
        m_szTip[0] = 0;
    }
public:
    CPicWindow(HINSTANCE hInstance, const WNDCLASSEX* wcx = nullptr) : CWindow(hInstance, wcx)
        , transparentColor(::GetSysColor(COLOR_WINDOW))
    {
        SetWindowTitle(L"Picture Window");
        m_wszTip[0]   = 0;
        m_szTip[0]    = 0;
    };

    enum class BlendType
    {
        Alpha,
        Xor,
    };
    /// Registers the window class and creates the window
    bool RegisterAndCreateWindow(HWND hParent);

    /// Sets the image path and title to show
    void SetPic(const std::wstring& path, const std::wstring& title, bool bFirst);
    /// Returns the CPicture image object. Used to get an already loaded image
    /// object without having to load it again.
    CPicture * GetPic() {return &picture;}
    /// Sets the path and title of the second image which is alpha blended over the original
    void SetSecondPic(CPicture* pPicture = nullptr, const std::wstring& sectit = L"", const std::wstring& secpath = L"", int hpos = 0, int vpos = 0)
    {
        pSecondPic = pPicture;
        pictitle2 = sectit;
        picpath2 = secpath;
        nVSecondScrollPos = vpos;
        nHSecondScrollPos = hpos;
    }

    void StopTimer() {KillTimer(*this, ID_ANIMATIONTIMER);}

    /// Returns the currently used alpha blending value (0.0-1.0)
    float GetBlendAlpha() const { return blendAlpha; }
    /// Sets the alpha blending value
    void SetBlendAlpha(BlendType type, float a)
    {
        m_blend = type;
        blendAlpha = a;
        if (m_AlphaSlider.IsValid())
            SendMessage(m_AlphaSlider.GetWindow(), TBM_SETPOS, 1, static_cast<LPARAM>(a * 16.0f));
        PositionTrackBar();
        InvalidateRect(*this, nullptr, FALSE);
    }
    /// Toggle the alpha blending value
    void ToggleAlpha()
    {
        if( 0.0f != GetBlendAlpha() )
            SetBlendAlpha(m_blend, 0.0f);
        else
            SetBlendAlpha(m_blend, 1.0f);
    }

    /// Set the color that this PicWindow will display behind transparent images.
    void SetTransparentColor(COLORREF back) { transparentColor = back; InvalidateRect(*this, nullptr, false); }

    /// Resizes the image to fit into the window. Small images are not enlarged.
    void FitImageInWindow();
    /// center the image in the view
    void CenterImage();
    /// forces the widths of the images to be the same
    void FitWidths(bool bFit);
    /// forces the heights of the images to be the same
    void FitHeights(bool bFit);
    /// Sets the zoom factor of the image
    void SetZoom(int Zoom, bool centermouse, bool inzoom = false);
    /// Returns the currently used zoom factor in which the image is shown.
    int GetZoom() const { return picscale; }
    /// Zooms in (true) or out (false) in nice steps
    void Zoom(bool in, bool centermouse);
    /// Sets the 'Other' pic window
    void SetOtherPicWindow(CPicWindow * pWnd) {pTheOtherPic = pWnd;}
    /// Links/Unlinks the two pic windows
    void LinkPositions(bool bLink) {bLinkedPositions = bLink;}
    /// Sets the overlay mode info
    void SetOverlapMode(bool b) {bOverlap = b;}

    void ShowInfo(bool bShow = true) { bShowInfo = bShow; InvalidateRect(*this, nullptr, false); }
    /// Sets up the scrollbars as needed
    void SetupScrollBars();

    bool HasMultipleImages();

    int GetHPos() const { return nHScrollPos; }
    int GetVPos() const { return nVScrollPos; }
    void SetZoomValue(int z) { picscale = z; InvalidateRect(*this, nullptr, FALSE); }

    void SetSelectionMode(bool bSelect = true) { bSelectionMode = bSelect; }
    /// Handles the mouse wheel
    void                OnMouseWheel(short fwKeys, short zDelta);
protected:
    /// the message handler for this window
    LRESULT CALLBACK    WinMsgHandler(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) override;
    /// Draws the view title bar
    void                DrawViewTitle(HDC hDC, RECT * rect);
    /// Creates the image buttons
    bool                CreateButtons();
    /// Handles vertical scrolling
    void                OnVScroll(UINT nSBCode, UINT nPos);
    /// Handles horizontal scrolling
    void                OnHScroll(UINT nSBCode, UINT nPos);
    /// Returns the client rectangle, without the scrollbars and the view title.
    /// Basically the rectangle the image can use.
    void                GetClientRect(RECT * pRect);
    /// Returns the client rectangle, without the view title but with the scrollbars
    void                GetClientRectWithScrollbars(RECT * pRect);
    /// the WM_PAINT function
    void                Paint(HWND hwnd);
    /// Draw pic to hdc, with a border, scaled by scale.
    void                ShowPicWithBorder(HDC hdc, const RECT &bounds, CPicture &pic, int scale);
    /// Positions the buttons
    void                PositionChildren();
    /// advance to the next image in the file
    void                NextImage();
    /// go back to the previous image in the file
    void                PrevImage();
    /// starts/stops the animation
    void                Animate(bool bStart);
    /// Creates the trackbar (the alpha blending slider control)
    void                CreateTrackbar(HWND hwndParent);
    /// Moves the alpha slider trackbar to the correct position
    void                PositionTrackBar();
    /// creates the info string used in the info box and the tooltips
    void                BuildInfoString(wchar_t* buf, int size, bool bTooltip);
    /// adjusts the zoom to fit the specified width
    void                SetZoomToWidth(long width);
    /// adjusts the zoom to fit the specified height
    void                SetZoomToHeight(long height);
    /// sets the dark mode
    void                SetTheme(bool bDark);
    /// returns the transparent color, adjusted for theme
    COLORREF            GetTransparentThemedColor();

    std::wstring        picpath;            ///< the path to the image we show
    std::wstring        pictitle;           ///< the string to show in the image view as a title
    CPicture            picture;            ///< the picture object of the image
    bool                bValid = false;     ///< true if the picture object is valid, i.e. if the image could be loaded and can be shown
    int                 picscale = 100;     ///< the scale factor of the image in percent
    COLORREF            transparentColor;   ///< the color to draw under the images
    bool                bFirstpaint = false;     ///< true if the image is painted the first time. Used to initialize some stuff when the window is valid for sure.
    CPicture*           pSecondPic = nullptr;    ///< if set, this is the picture to draw transparently above the original
    CPicWindow*         pTheOtherPic = nullptr;  ///< pointer to the other picture window. Used for "linking" the two windows when scrolling/zooming/...
    bool                bMainPic = false;        ///< if true, this is the first image
    bool                bLinkedPositions = true; ///< if true, the two image windows are linked together for scrolling/zooming/...
    bool                bFitWidths = false;      ///< if true, the two image windows are shown with the same width
    bool                bFitHeights = false;     ///< if true, the two image windows are shown with the same height
    bool                bOverlap = false;        ///< true if the overlay mode is active
    bool                bDragging = false;      ///< indicates an ongoing dragging operation
    BlendType           m_blend = BlendType::Alpha; ///< type of blending to use
    std::wstring        pictitle2;          ///< the title of the second picture
    std::wstring        picpath2;           ///< the path of the second picture
    float               blendAlpha = 0.5f;  ///<the alpha value for transparency blending
    bool                bShowInfo = false;  ///< true if the info rectangle of the image should be shown
    wchar_t             m_wszTip[8192];
    char                m_szTip[8192];
    POINT               m_lastTTPos{};
    HWND                hwndTT = nullptr;
    HWND                hwndTrack = nullptr;
    bool                bSelectionMode = false;     ///< true if TortoiseIDiff is in selection mode, used to resolve conflicts
    int                 m_themeCallbackId = 0;
    // scrollbar info
    int                 nVScrollPos = 0;        ///< vertical scroll position
    int                 nHScrollPos = 0;        ///< horizontal scroll position
    int                 nVSecondScrollPos = 0;  ///< vertical scroll position of second pic at the moment of enabling overlap mode
    int                 nHSecondScrollPos = 0;  ///< horizontal scroll position of second pic at the moment of enabling overlap mode
    POINT               ptPanStart{ -1, -1 }; ///< the point of the last mouse click
    int                 startVScrollPos = 0;  ///< the vertical scroll position when panning starts
    int                 startHScrollPos = 0;  ///< the horizontal scroll position when panning starts
    int                 startVSecondScrollPos = 0;  ///< the vertical scroll position of the second pic when panning starts
    int                 startHSecondScrollPos = 0;  ///< the horizontal scroll position of the second pic when panning starts
    // image frames/dimensions
    UINT                nDimensions = 0;
    UINT                nCurrentDimension = 1;
    UINT                nFrames = 0;
    UINT                nCurrentFrame = 1;

    // controls
    HWND                hwndLeftBtn = nullptr;
    HWND                hwndRightBtn = nullptr;
    HWND                hwndPlayBtn = nullptr;
    HWND                hwndSelectBtn = nullptr;
    CNiceTrackbar       m_AlphaSlider;
    HWND                hwndAlphaToggleBtn = nullptr;
    CAutoIcon           hLeft;
    CAutoIcon           hRight;
    CAutoIcon           hPlay;
    CAutoIcon           hStop;
    CAutoIcon           hAlphaToggle;
    bool                bPlaying = false;
    RECT                m_inforect{};

    // linked image sizes/positions
    long                m_linkedWidth = 0;
    long                m_linkedHeight = 0;
};
