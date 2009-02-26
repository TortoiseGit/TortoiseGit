/*
 * Copyright (c) 2001-2002 Paolo Messina and Jerzy Kaczorowski
 * 
 * The contents of this file are subject to the Artistic License (the "License").
 * You may not use this file except in compliance with the License. 
 * You may obtain a copy of the License at:
 * http://www.opensource.org/licenses/artistic-license.html
 * 
 * THIS PACKAGE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR IMPLIED 
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF 
 * MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 * 
 */

// OddButton.cpp : implementation file
//

#include "stdafx.h"
#include "OddButton.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COddButton

COddButton::COddButton()
{
	m_bDefault = FALSE;
	m_bCanBeDefault = FALSE;
	
	// invalid value, since type still unknown
	m_nTypeStyle = ODDBTN_BS_TYPEMASK;
}

COddButton::~COddButton()
{
}

IMPLEMENT_DYNAMIC(COddButton, CButton)

BEGIN_MESSAGE_MAP(COddButton, CButton)
	//{{AFX_MSG_MAP(COddButton)
	ON_WM_GETDLGCODE()
	//}}AFX_MSG_MAP
	ON_MESSAGE(BM_SETSTYLE, OnSetStyle)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COddButton message handlers

/*!
	PreSubclassWindow virtual override

  	Initialize internal data members and verify that the BS_OWNERDRAW style is set
	\note COddButton::PreSubclassWindow should be called if overrided in a derived class
*/
void COddButton::PreSubclassWindow() 
{
	// set initial control type
	m_nTypeStyle = GetStyle() & ODDBTN_BS_TYPEMASK;

	// set initial default state flag
	if (m_nTypeStyle == BS_DEFPUSHBUTTON)
	{
		// enable default state handling for push buttons
		m_bCanBeDefault = TRUE;

		// set default state for a default button
		m_bDefault = TRUE;

		// adjust style for default button
		m_nTypeStyle = BS_PUSHBUTTON;
	}
	else if (m_nTypeStyle == BS_PUSHBUTTON)
	{
		// enable default state handling for push buttons
		m_bCanBeDefault = TRUE;
	}

	// you should not set the Owner Draw before this call
	// (don't use the resource editor "Owner Draw" or
	// ModifyStyle(0, BS_OWNERDRAW) before calling PreSubclassWindow() )
	ASSERT(m_nTypeStyle != BS_OWNERDRAW);

	// switch to owner-draw
	ModifyStyle(ODDBTN_BS_TYPEMASK, BS_OWNERDRAW, SWP_FRAMECHANGED);

	CButton::PreSubclassWindow();
}

/// WM_GETDLGCODE message handler, indicate to the system whether we want to handle default state
UINT COddButton::OnGetDlgCode() 
{
	UINT nCode = CButton::OnGetDlgCode();

	// handle standard control types
	switch (GetControlType())
	{
	case BS_RADIOBUTTON:
	case BS_AUTORADIOBUTTON:
		nCode |= DLGC_RADIOBUTTON;
		break;

	case BS_GROUPBOX:
		nCode = DLGC_STATIC;
		break;
	}

	// tell the system if we want default state handling
	// (losing default state always allowed)
	if (m_bCanBeDefault || m_bDefault)
		nCode |= (m_bDefault ? DLGC_DEFPUSHBUTTON : DLGC_UNDEFPUSHBUTTON);

	return nCode;
}

/// BM_SETSTYLE message handler, update internal default state data member
LRESULT COddButton::OnSetStyle(WPARAM wParam, LPARAM lParam)
{
	UINT_PTR nNewType = (wParam & ODDBTN_BS_TYPEMASK);

	// update default state flag
	if (nNewType == BS_DEFPUSHBUTTON)
	{
		// we must like default state at this point
		ASSERT(m_bCanBeDefault);

		m_bDefault = TRUE;
	}
	else if (nNewType == BS_PUSHBUTTON)
	{
		// losing default state always allowed
		m_bDefault = FALSE;
	}

	// can't change control type after owner-draw is set.
	// let the system process changes to other style bits
	// and redrawing, while keeping owner-draw style
	return DefWindowProc(BM_SETSTYLE,
		(wParam & ~ODDBTN_BS_TYPEMASK) | BS_OWNERDRAW, lParam);
}

/////////////////////////////////////////////////////////////////////////////
// COddButton methods

/*!
	Get the type of control to draw
	\return The of control
*/
UINT COddButton::GetControlType() const
{
	return m_nTypeStyle;
}

/*!
	Get the control's default state
	\return TRUE if control has a default state, FALSE otherwise
*/
BOOL COddButton::IsDefault() const
{
	// if we have default state, we must like it!
	ASSERT((m_bCanBeDefault && m_bDefault) == m_bDefault);

	return m_bDefault;
}

/*!
	Enable default state handling 
	\param bEnable TRUE to enable default state handling, FALSE to disable
*/
void COddButton::EnableDefault(BOOL bEnable)
{
	m_bCanBeDefault = bEnable;

	// disabling default when control has default state
	// needs removing the default state
	if (!bEnable && m_bDefault)
	{
		// remove default state
		SendMessage(BM_SETSTYLE, (GetStyle() & ~ODDBTN_BS_TYPEMASK) | BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
		ASSERT(m_bDefault == FALSE);

		// update default button
		CWnd* pParent = GetParent();
		if( pParent )
		{
			LRESULT lRes = pParent->SendMessage(DM_GETDEFID);
			if (HIWORD(lRes) == DC_HASDEFID)
			{
				pParent->SendMessage(DM_SETDEFID, LOWORD(lRes));
			}
		}
	}
}

/*!
	Set the dialog's default pushbutton
	\param pDialog Dialog to set the default pushbutton
	\param nID Button ID to make default
	\note It fixes the drawing problem that sometimes occurs when using plain CDialog::SetDefID,
	see also MS kb Q67655 (<A HREF="http://support.microsoft.com/default.aspx?scid=KB;EN-US;Q67655&">
	HOWTO: Change or Set the Default Push Button in a Dialog Box</A>)
*/
void COddButton::SetDefID(CDialog* pDialog, const UINT nID)
{
	if( !pDialog || !::IsWindow(pDialog->m_hWnd) )
	{
		ASSERT(FALSE); // Bad pointer or dialog is not a window
		return;
	}

	// get the current default button
	const DWORD dwPrevDefID = pDialog->GetDefID();
	const UINT nPrevID = (HIWORD(dwPrevDefID) == DC_HASDEFID) ? LOWORD(dwPrevDefID) : 0;
		
	/*
	 *	Set the new default ID in the dialog
	 */
	pDialog->SetDefID(nID);

	/*
	 *	Make sure the previous default button doesn't have the default state anymore
	 */

	// check previous ID is a default-compatible button
	// and it has the default state
	LRESULT lRes = (nPrevID == 0) ? 0 : pDialog->SendDlgItemMessage(nPrevID, WM_GETDLGCODE);
	if( (lRes & DLGC_BUTTON) && 
		(lRes & DLGC_DEFPUSHBUTTON) )
	{
		pDialog->SendDlgItemMessage(nPrevID, BM_SETSTYLE,
			BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
	}
	
	/*
	 *	Make sure the new default button receives the default state
	 */

	// check new ID is a button
	lRes = (nID == 0) ? 0 : pDialog->SendDlgItemMessage(nID, WM_GETDLGCODE);
	if( lRes & DLGC_BUTTON )
	{
		// exception: current focused button should keep its default state (IMHO)
		CWnd* pFocusWnd = GetFocus();
		LRESULT lResFocus = (pFocusWnd == NULL) ? 0 : pFocusWnd->SendMessage(WM_GETDLGCODE);

		// check focused control is a default-compatible button
		if( (lResFocus & DLGC_BUTTON) && 
			(lResFocus & (DLGC_DEFPUSHBUTTON | DLGC_UNDEFPUSHBUTTON)) )
		{
			// remove default state (if needed)
			if( (lRes & DLGC_DEFPUSHBUTTON) && 
				(nID != (UINT)pFocusWnd->GetDlgCtrlID()) )
			{
				pDialog->SendDlgItemMessage(nID, BM_SETSTYLE,
					BS_PUSHBUTTON, MAKELPARAM(TRUE, 0));
			}

			// set default state (if needed)
			if( lResFocus & DLGC_UNDEFPUSHBUTTON )
			{
				pFocusWnd->SendMessage(BM_SETSTYLE,
					BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));
			}
		}
		else if( lRes & DLGC_UNDEFPUSHBUTTON )
		{
			// not default-compatible button has the focus
			// set default state
			pDialog->SendDlgItemMessage(nID, BM_SETSTYLE,
				BS_DEFPUSHBUTTON, MAKELPARAM(TRUE, 0));
		}
	}
}
