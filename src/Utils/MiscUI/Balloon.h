// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

#include "gradient.h"
#include "htmlformatter.h"
#include "cursor.h"

//The styles
#define BALLOON_ANCHOR					0x0001
#define BALLOON_SHADOW					0x0002
#define BALLOON_ROUNDED					0x0004
#define BALLOON_RSA						0x0007
#define BALLOON_VCENTER_ALIGN			0x0008
#define BALLOON_BOTTOM_ALIGN			0x0010
#define BALLOON_CLOSEBUTTON				0x0020

//The behaviors
#define BALLOON_MULTIPLE_SHOW			0x0001	//Multiple show for single control
#define	BALLOON_TRACK_MOUSE				0x0002	//ToolTip follows the mouse cursor
#define BALLOON_DIALOG					0x0004	//Shown as a dialog instead of a tooltip
#define BALLOON_DIALOG_DESTROY			0x0008	//delete the object after window is destroyed. Use carefully!

//The masks
#define BALLOON_MASK_STYLES				0x0001	//The styles for the tooltip gets from the structures
#define BALLOON_MASK_EFFECT				0x0002	//The background's type for the tooltip gets from the structures
#define BALLOON_MASK_COLORS				0x0004	//The background's colors for the tooltip gets from the structures
#define BALLOON_MASK_DIRECTION			0x0008  //The align for the tooltip gets from the structures
#define BALLOON_MASK_BEHAVIOUR			0x0010  //The behavior for the tooltip gets from the structures

/**
 * \ingroup Utils
 * BALLOON_INFO structure.
 */
typedef struct tagBALLOON_INFO
{
	HICON		hIcon;			///<The icon of the tooltip
	CString		sBalloonTip;	///<The string of the tooltip
	UINT        nMask;			///<The mask 
	UINT		nStyles;		///<The tool tip's styles
	UINT        nDirection;		///<Direction display the tooltip relate cursor point
	UINT		nEffect;		///<The color's type or effects
	UINT        nBehaviour;		///<The tool tip's behavior
	COLORREF	crBegin;		///<Begin Color
	COLORREF    crMid;			///<Mid Color
	COLORREF	crEnd;			///<End Color

    tagBALLOON_INFO();          ///<proper initialization of all members
} BALLOON_INFO;

/**
 * \ingroup Utils
 * This structure is sent to with the notify messages.
 */
typedef struct tagNM_BALLOON_DISPLAY {
    NMHDR hdr;
	CPoint * pt;
	CWnd * pWnd;
	BALLOON_INFO * bi;
} NM_BALLOON_DISPLAY;

#define BALLOON_CLASSNAME    _T("CBalloon")  // Window class name
#define UDM_TOOLTIP_FIRST		   (WM_USER + 100)
#define UDM_TOOLTIP_DISPLAY		   (UDM_TOOLTIP_FIRST) //User has changed the data

/**
 * \ingroup Utils
 * Shows Balloons with info text in it. Either as Tooltips or as modeless dialog boxes.
 * Several options are available to customize the look and behavior of the balloons.
 * Since this class inherits CHTMLFormatter you can use all the tags CHTMLFormatter
 * provides to format the text.
 * Please refer to the documentation of the methods for details.\n
 * \image html "balloon_box.jpg"
 * \image html "balloon_tooltip.jpg"
 * 
 * To use the dialog balloons just call the static methods:
 * \code
 * CWnd* ctrl = GetDlgItem(IDC_EDITBOX);
 * CRect rt;
 * ctrl->GetWindowRect(rt);
 * CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
 * CBalloon::ShowBalloon(NULL, point, 
 *                 "this is a <b>Message Balloon</b>\n<hr=100%>\n<ct=0x0000FF>Warning! Warning!</ct>\nSomething unexpected happened",
 *                 TRUE, IDI_EXCLAMATION);
 * \endcode
 * 
 * To use the tooltips, declare an object of CBalloon as a member of your dialog class:
 * \code
 * CBalloon m_tooltips;
 * \code
 * In your OnInitDialog() method add the tooltips and modify them as you like:
 * \code
 * m_tooltips.Create(this);		//initializes the tooltips
 * m_tooltips.AddTool(IDC_BUTTON, "this button does nothing");
 * m_tooltips.AddTool(IDC_EDITBOX, "enter a value here", IDI_ICON);
 * m_tooltips.SetEffectBk(GetDlgItem(IDC_EDITBOX), CBalloon::BALLOON_EFFECT_HGRADIENT);		//only affects the edit box tooltip
 * m_tooltips.SetGradientColors(0x80ffff, 0x000000, 0xffff80);
 * \endcode
 * and last you have to override the PreTranslateMessage() method of your dialog box:
 * \code
 * BOOL CMyDialog::PreTranslateMessage(MSG* pMsg)
 * {
 *  m_tooltips.RelayEvent(pMsg);
 *  return CDialog::PreTranslateMessage(pMsg);
 * }
 * \endcode
 */
class CBalloon : public CWnd, public CHTMLFormatter
{
// Construction
public:
	virtual BOOL Create(CWnd* pParentWnd);
	CBalloon();
	virtual ~CBalloon();

// Attributes
public:
	enum {	XBLSZ_ROUNDED_CX = 0,
			XBLSZ_ROUNDED_CY,
			XBLSZ_MARGIN_CX,
			XBLSZ_MARGIN_CY,
			XBLSZ_SHADOW_CX,
			XBLSZ_SHADOW_CY,
			XBLSZ_WIDTH_ANCHOR,
			XBLSZ_HEIGHT_ANCHOR,
			XBLSZ_MARGIN_ANCHOR,
			XBLSZ_BORDER_CX,
			XBLSZ_BORDER_CY,
			XBLSZ_BUTTON_MARGIN_CX,
			XBLSZ_BUTTON_MARGIN_CY,

			XBLSZ_MAX_SIZES
		};

	enum {	BALLOON_COLOR_FG = 0,
			BALLOON_COLOR_BK_BEGIN,
			BALLOON_COLOR_BK_MID,
			BALLOON_COLOR_BK_END,
			BALLOON_COLOR_SHADOW,		// Color for the shadow
			BALLOON_COLOR_BORDER,		// Color for border of the tooltip

			BALLOON_MAX_COLORS
		};

	enum {	BALLOON_LEFT_TOP = 0,
			BALLOON_RIGHT_TOP,
			BALLOON_LEFT_BOTTOM,
			BALLOON_RIGHT_BOTTOM,

			BALLOON_MAX_DIRECTIONS
		};

	enum {	BALLOON_EFFECT_SOLID = 0,
			BALLOON_EFFECT_HGRADIENT,
			BALLOON_EFFECT_VGRADIENT,
			BALLOON_EFFECT_HCGRADIENT,
			BALLOON_EFFECT_VCGRADIENT,
			BALLOON_EFFECT_3HGRADIENT,
			BALLOON_EFFECT_3VGRADIENT,

			BALLOON_MAX_EFFECTS
		};


// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(XToolTip)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	/** \name Balloon Dialogs 
	 * static methods to show a balloon like a modeless dialog
	 */
	//@{
	/**
	 * Pops up a balloon like a modeless dialog to inform the user in
	 * a non disturbing way like with a normal MessageBox().
	 * \image html "balloon_box.jpg"
	 * An example of when to use such balloons is in a dialog box where
	 * the user can enter values. If one or several values are outside
	 * of valid ranges then just pop up a balloon. That way the user 
	 * knows exactly \b where the wrong value is (if the balloon is
	 * placed so that the anchor points to the edit box) and also
	 * doesn't have to press "OK" to close the box.
	 *
	 * \param pWnd the parent window. Or NULL if no parent window is available.
	 * \param pt the point where the anchor should point to. For example if you
	 * want to point to an edit box the point would be:
	 * \code
	 * CWnd* ctrl = GetDlgItem(IDC_EDITBOX);
	 * CRect rt;
	 * ctrl->GetWindowRect(rt);
	 * CPoint point = CPoint((rt.left+rt.right)/2, (rt.top+rt.bottom)/2);
	 * \endcode
	 * \param nIdText the string ID of the text to show. The ID has to be in your resources.
	 * \param sText the string to show.
	 * \param showCloseButton If TRUE, then the balloon has a close button in the upper right corner. That also
	 * makes the balloon to show up until the user presses the close button.\n
	 * If FALSE, then the balloon will be closed after a timeout of 5 seconds or as soon as the user clicks anywhere
	 * on the balloon.
	 * \param hIcon a handle of an icon.
	 * \param nIdIcon an ID of an icon which has to be in your resources.
	 * \param szIcon a name of an icon. Either a path to an icon file or one of
	 * the following system icons:\n
	 * - IDI_APPLICATION
	 * - IDI_ERROR
	 * - IDI_HAND
	 * - IDI_EXCLAMATION
	 * - IDI_WARNING
	 * - IDI_QUESTION
	 * - IDI_WINLOGO
	 * - IDI_INFORMATION
	 * - IDI_ASTERISK
	 * - IDI_QUESTION
	 * 
	 * \param nDirection the direction to where the dialog should be drawn. Defaults to BALLOON_RIGHT_TOP.
	 * - BALLOON_LEFT_TOP
	 * - BALLOON_RIGHT_TOP
	 * - BALLOON_LEFT_BOTTOM
	 * - BALLOON_RIGHT_BOTTOM
	 *
	 * \param nEffect specifies how to draw the background. Defaults to BALLOON_EFFECT_SOLID.
	 * - BALLOON_EFFECT_SOLID		one color for the background. The default is the standard windows color for tooltip backgrounds.
	 * - BALLOON_EFFECT_HGRADIENT	draws a horizontal gradient from crStart to crEnd
	 * - BALLOON_EFFECT_VGRADIENT	draws a vertical gradient from crStart to crEnd
	 * - BALLOON_EFFECT_HCGRADIENT	draws a horizontal gradient from crStart to crEnd to crStart
	 * - BALLOON_EFFECT_VCGRADIENT	draws a vertical gradient from crStart to crEnd to crStart
	 * - BALLOON_EFFECT_3HGRADIENT	draws a horizontal gradient from crStart to crMid to crEnd
	 * - BALLOON_EFFECT_3VGRADIENT	draws a vertical gradient from crStart to crMid to crEnd
	 *
	 * \param crStart the starting color for gradients
	 * \param crMid the middle color for three colored gradients
	 * \param crEnd the end color for gradients
	 */
	/**
	 * \overload ShowBalloon(CWnd * pWnd, CPoint pt, UINT nIdText, BOOL showCloseButton, UINT nIdIcon, UINT nDirection = BALLOON_RIGHT_TOP, UINT nEffect = BALLOON_EFFECT_SOLID, COLORREF crStart = NULL, COLORREF crMid = NULL, COLORREF crEnd = NULL);
	 */
	static void ShowBalloon(
		CWnd * pWnd, CPoint pt, const CString& sText, BOOL showCloseButton, HICON hIcon,
		UINT nDirection = BALLOON_RIGHT_TOP, UINT nEffect = BALLOON_EFFECT_SOLID,
		COLORREF crStart = NULL, COLORREF crMid = NULL, COLORREF crEnd = NULL);
	/**
	 * \overload ShowBalloon(CWnd * pWnd, CPoint pt, UINT nIdText, BOOL showCloseButton, LPCTSTR szIcon);
	 */
	static void ShowBalloon(CWnd * pWnd, CPoint pt, UINT nIdText, BOOL showCloseButton, LPCTSTR szIcon);
	//@}

	/** 
	 * Helper function to return the center point of a dialog control 
	 * Useful for passing to ShowBalloon
	 */
	static CPoint GetCtrlCentre(CWnd* pDlgWnd, UINT ctrlId);

	/** \name ToolTips 
	 * handling of tooltips.
	 */
	//@{
	//@{
	/**
	 * Adds a tooltip for a windows element to the internal list.
	 * \param pWnd pointer to a windows element.
	 * \param nIdWnd an ID of a dialog resource.
	 * \param nIdText an ID of a string dialog resource to use as the tooltip text.
	 * \param sBalloontipText string for the tooltip.
	 * \param hIcon handle for an icon to show on the tooltip.
	 * \param nIdIcon a resource ID for an icon to show on the tooltip.
	 * \param bi pointer to a BALLOON_IFNO structure.
	 */
	void	AddTool(CWnd * pWnd, UINT nIdText, HICON hIcon = NULL); //Adds tool
	void	AddTool(CWnd * pWnd, UINT nIdText, UINT nIdIcon); //Adds tool
	void	AddTool(CWnd * pWnd, const CString& sBalloonTipText, HICON hIcon = NULL); //Adds tool
	void	AddTool(CWnd * pWnd, const CString& sBalloonTipText, UINT nIdIcon); //Adds tool
	void	AddTool(int nIdWnd, UINT nIdText, HICON hIcon = NULL); //Adds tool
	void	AddTool(int nIdWnd, UINT nIdText, UINT nIdIcon); //Adds tool
	void	AddTool(int nIdWnd, const CString& sBalloonTipText, HICON hIcon = NULL); //Adds tool
	void	AddTool(int nIdWnd, const CString& sBalloonTipText, UINT nIdIcon); //Adds tool
	void	AddTool(CWnd * pWnd, BALLOON_INFO & bi); //Adds tool
	//@}

	/**
	 * Gets the text and the icon handle of a specific tooltip.
	 * \param pWnd pointer to the tooltip window
	 * \param sBalloonTipText the returned tooltip text
	 * \param hIcon the returned icon handle
	 * \param bi pointer to the returned BALLOON_INFO structure.
	 * \return TRUE if the tooltip exists.
	 */
	BOOL	GetTool(CWnd * pWnd, CString & sBalloonTipText, HICON & hIcon) const; //Gets the tool tip's text
	BOOL	GetTool(CWnd * pWnd, BALLOON_INFO & bi) const; //Gets tool

	/**
	 * Removes a specific tooltip from the internal list.
	 * \param pWnd pointer to the tooltip window
	 */
	void	RemoveTool(CWnd * pWnd);  //Removes specified tool

	/**
	 * Removes all tooltips from the internal list. 
	 */
	void	RemoveAllTools(); // Removes all tools
	//@}

	/** \name Styles 
	 * handling of tooltip appearance styles.
	 * The following styles are available:
	 * - BALLOON_ANCHOR			the balloon is drawn with an anchor
	 * - BALLOON_SHADOW			the balloon is drawn with a SE shadow
	 * - BALLOON_ROUNDED		the balloon has round corners. For tooltips like the standard windows ones disable this style.
	 * - BALLOON_RSA			combines BALLOON_ANCHOR, BALLOON_SHADOW and BALLOON_ROUNDED. This is the default.
	 * - BALLOON_VCENTER_ALIGN
	 * - BALLOON_BOTTOM_ALIGN
	 * - BALLOON_CLOSEBUTTON	the balloon has a close button in the upper right corner.
	 */
	//@{
	/**
	 * sets styles for either all tooltips or specific ones.
	 * \param nStyles the styles to set.
	 * \param pWnd pointer to the tooltip window or NULL if the styles should affect all tooltips.
	 */
	void	SetStyles(DWORD nStyles, CWnd * pWnd = NULL); //Sets New Style
	/**
	 * Modifies existing styles.
	 * \param nAddStyles the styles to add.
	 * \param nRemoveStyles the styles to remove
	 * \param pWnd pointer to the tooltip window or NULL if the styles should affect all tooltips.
	 */
	void	ModifyStyles(DWORD nAddStyles, DWORD nRemoveStyles, CWnd * pWnd = NULL); //Modifies styles
	/**
	 * returns the current styles for the tooltip.
	 * \param pWnd pointer to the tooltip window or NULL if the global styles are needed.
	 */
	DWORD	GetStyles(CWnd * pWnd = NULL) const; //Gets current Styles
	/**
	 * Resets the styles to the default values.
	 * \param pWnd pointer to the tooltip window or NULL if the styles should affect all tooltips.
	 */
	void	SetDefaultStyles(CWnd * pWnd = NULL); //Sets default styles
	//@}

	/** \name Colors
	 * different color settings. The following elements have colors:
	 * - BALLOON_COLOR_FG				the foreground text color. Default is black.
	 * - BALLOON_COLOR_BK_BEGIN			the background color and the first color in gradients.
	 * - BALLOON_COLOR_BK_MID			the middle color for gradients.
	 * - BALLOON_COLOR_BK_END			the end color for gradients.
	 * - BALLOON_COLOR_SHADOW			the color of the shadow
	 * - BALLOON_COLOR_BORDER			the color for the balloon border
	 */
	//@{
	/**
	 * Sets the color for a balloon element.
	 * \param nIndex the element to set the color.
	 * \param crColor the color.
	 */
	void	SetColor(int nIndex, COLORREF crColor); //Sets the color
	/**
	 * Returns the color of a balloon element.
	 */
	COLORREF	GetColor(int nIndex) const; //Gets the color
	/**
	 * Resets all colors to default values.
	 */
	void	SetDefaultColors(); //Sets default colors
	/**
	 * Sets the colors used in the background gradients.
	 * \param crBegin first color
	 * \param crMid middle color
	 * \param crEnd end color
	 * \param pWnd pointer to the tooltip window or NULL if the settings are global.
	 */
	void	SetGradientColors(COLORREF crBegin, COLORREF crMid, COLORREF crEnd, CWnd * pWnd = NULL); //Sets the gradient's colors
	/**
	 * Returns the colors used in the background gradients.
	 * \param pWnd pointer to the tooltip window or NULL if the global settings are needed.
	 */
	void	GetGradientColors(COLORREF & crBegin, COLORREF & crMid, COLORREF & crEnd, CWnd * pWnd = NULL) const; //Gets the gradient's colors
	//@}


	/** \name Masks 
	 * Manipulate masks of tooltips. Masks are used to define styles, effects, colors and the like for single
	 * tooltips and not only for all tooltips.
	 * Whatever mask is set for a specific tooltip means that this tooltip has its own version of those settings
	 * and ignores the global settings.\n
	 * The following masks are available:
	 * - BALLOON_MASK_STYLES		masks out the styles
	 * - BALLOON_MASK_EFFECT		masks out the effects
	 * - BALLOON_MASK_COLORS		masks out the colors
	 * - BALLOON_MASK_DIRECTION		masks out the direction
	 * - BALLOON_MASK_BEHAVIOUR		masks out the behavior
	 * 
	 * The functions either set, modify or read out the masks for specific tooltip windows.
	 */
	//@{
	void	SetMaskTool(CWnd * pWnd, UINT nMask = 0);
	void	ModifyMaskTool(CWnd * pWnd, UINT nAddMask, UINT nRemoveMask);
	UINT	GetMaskTool(CWnd * pWnd) const;
	//@}

	/** \name Effects
	 * Use these methods to manipulate background effects of the tooltip. The following
	 * effects are available.
	 * - BALLOON_EFFECT_SOLID		one color for the background. The default is the standard windows color for tooltip backgrounds.
	 * - BALLOON_EFFECT_HGRADIENT	draws a horizontal gradient from crStart to crEnd
	 * - BALLOON_EFFECT_VGRADIENT	draws a vertical gradient from crStart to crEnd
	 * - BALLOON_EFFECT_HCGRADIENT	draws a horizontal gradient from crStart to crEnd to crStart
	 * - BALLOON_EFFECT_VCGRADIENT	draws a vertical gradient from crStart to crEnd to crStart
	 * - BALLOON_EFFECT_3HGRADIENT	draws a horizontal gradient from crStart to crMid to crEnd
	 * - BALLOON_EFFECT_3VGRADIENT	draws a vertical gradient from crStart to crMid to crEnd
	 */
	//@{
	void	SetEffectBk(UINT nEffect, CWnd * pWnd = NULL);
	UINT	GetEffectBk(CWnd * pWnd = NULL) const;
	//@}

	/** \name Notification 
	 * Gets or sets if the parent or any other window should get notification messages from
	 * the tooltips.
	 */
	//@{
	void	SetNotify(HWND hWnd);
	void	SetNotify(BOOL bParentNotify = TRUE);
	BOOL	GetNotify() const; //Is enabled notification
	//@}

	/** \name Delaytimes
	 * Gets or sets the delay times for the tooltips.
	 * - TTDT_AUTOPOP time in milliseconds until the tooltip automatically closes.
	 * - TTDT_INITIAL time in milliseconds until the tooltip appears when the mouse pointer is over a control.
	 */
	//@{
	void	SetDelayTime(DWORD dwDuration, UINT nTime);
	UINT	GetDelayTime(DWORD dwDuration) const;
	//@}


	/** \name Direction 
	 * Gets or sets the direction of the balloons.
	 * - BALLOON_LEFT_TOP
	 * - BALLOON_RIGHT_TOP
	 * - BALLOON_LEFT_BOTTOM
	 * - BALLOON_RIGHT_BOTTOM
	 */
	//@{
	void	SetDirection(UINT nDirection = BALLOON_RIGHT_TOP, CWnd * pWnd = NULL);
	UINT	GetDirection(CWnd * pWnd = NULL) const;
	//@}

	/** \name Behavior
	 * Gets or sets the behavior of the balloons.
	 * - BALLOON_MULTIPLE_SHOW		if this is set then the tooltip will appear again if the mouse pointer is still over the same control.
	 * - BALLOON_TRACK_MOUSE		if set then the tooltip will follow the mouse pointer
	 * - BALLOON_DIALOG				the balloon is shown as a dialog instead of a tooltip, i.e. it won't close when the mouse pointer leaves the control.
	 * - BALLOON_DIALOG_DESTROY		the object itself is destroyed when the balloon is closed. Use this \b very carefully!
	 */
	//@{
	void	SetBehaviour(UINT nBehaviour = 0, CWnd * pWnd = NULL);
	UINT	GetBehaviour(CWnd * pWnd = NULL) const;
	//@}

	/** \name Fonts 
	 * Font settings for the balloon text.
	 */
	//@{
	BOOL	SetFont(CFont & font); //set font
	BOOL	SetFont(LPLOGFONT lf); //set font
	BOOL	SetFont(LPCTSTR lpszFaceName, int nSizePoints = 8,
									BOOL bUnderline = FALSE, BOOL bBold = FALSE,
									BOOL bStrikeOut = FALSE, BOOL bItalic = FALSE); //set font
	void	SetDefaultFont(); //set default fonts
	void	GetFont(CFont & font) const;
	void	GetFont(LPLOGFONT lf) const;
	//@}

	/**
	 * Call this method from CDialog::PreTranslateMessage(pMsg).
	 */
	void	RelayEvent(MSG* pMsg);
	
	/**
	 * Hide tooltip immediately.
	 */
	void	Pop();

	/**
	 * Shows a tooltip immediately.
	 */
	void	DisplayToolTip(CPoint * pt = NULL);
	void	DisplayToolTip(CPoint * pt, CRect * rect);

	// Generated message map functions
protected:
	void	SetSize(int nSizeIndex, UINT nValue);
	UINT	GetSize(int nSizeIndex) const;
	void	SetDefaultSizes();

	void	Redraw(BOOL bRedraw = TRUE);
	void	KillTimers(UINT nIDTimer = NULL);
		
	void	SetNewToolTip(CWnd * pWnd);
	void	GetMonitorWorkArea(const CPoint& sourcePoint, CRect& monitorRect) const;

	/**
	 * Finds the child window to which the point belongs
	 * \param point the point to look for the child window
	 * \return the pointer to the child window, or NULL if there is now window
	 */
	HWND	GetChildWindowFromPoint(CPoint & point) const;
	BOOL	IsCursorInToolTip() const;
    inline	BOOL IsVisible() const { return ((GetStyle() & WS_VISIBLE) == WS_VISIBLE); }

	CSize	GetTooltipSize(const CString& str); //Gets max rectangle for display tooltip text
	CSize	GetSizeIcon(HICON hIcon) const;
	void	CalculateInfoBoxRect(CPoint * pt, CRect * rect);

	LPLOGFONT	GetSystemToolTipFont() const;

	int		GetNextHorizDirection(int nDirection) const;
	int		GetNextVertDirection(int nDirection) const;
	BOOL	TestHorizDirection(int x, int cx, const CRect& monitorRect, int nDirection, LPRECT rect);
	BOOL	TestVertDirection(int y, int cy, const CRect& monitorRect, int nDirection, LPRECT rect);

	CRect	GetWindowRegion(CRgn * rgn, CSize sz, CPoint pt) const;

	LRESULT	SendNotify(CWnd * pWnd, CPoint * pt, BALLOON_INFO & bi);

	void	OnDraw(CDC * pDC, CRect rect);
	void	OnDrawBackground(CDC * pDC, CRect * pRect);

	virtual void PostNcDestroy();
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnDestroy();
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
protected:
	enum {	BALLOON_SHOW = 0x100, //the identifier of the timer for show the tooltip
			BALLOON_HIDE = 0x101  //the identifier of the timer for hide the tooltip
		};

	CMap<HWND, HWND, BALLOON_INFO, BALLOON_INFO> m_ToolMap; //Tool Maps

	HWND	m_hNotifyWnd; // Handle to window for notification about change data
	CWnd *	m_pParentWnd; // The pointer to the parent window
	HWND	m_hCurrentWnd;
	HWND	m_hDisplayedWnd;
	UINT	m_nLastDirection;
	

    LOGFONT	m_LogFont;                  // Current font in use
	
	//Default setting
	COLORREF	m_crColor [BALLOON_MAX_COLORS]; //The indexing colors
	UINT	m_nSizes [XBLSZ_MAX_SIZES]; //All sizes 
	UINT	m_nStyles;
	UINT	m_nDirection;
	UINT	m_nEffect;
	UINT	m_nBehaviour;	 //The tool tip's behavior 

	UINT	m_nTimeAutoPop; 
	UINT	m_nTimeInitial;

	//The properties of the current tooltip
	CPoint  m_ptOriginal;

	CRgn	m_rgnBalloon;
	CRgn    m_rgnShadow;

	CSize	m_szBalloonIcon; //the size of the current icon
	CSize	m_szBalloonText; //the size of the tool tip's text
	CSize	m_szCloseButton;

	CRect	m_rtCloseButton;	//the rect for the close button
	BOOL	m_bButtonPushed;

	CCursor	m_Cursor;

	BALLOON_INFO	m_pToolInfo; //info of the current tooltip


};


















