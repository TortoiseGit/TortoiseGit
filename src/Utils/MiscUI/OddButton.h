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

#if !defined(AFX_ODDBUTTON_H__4CA1E42E_E3C3_4FEA_99A1_E865DEB500DA__INCLUDED_)
#define AFX_ODDBUTTON_H__4CA1E42E_E3C3_4FEA_99A1_E865DEB500DA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OddButton.h : header file
//

/// Control's type mask
#define ODDBTN_BS_TYPEMASK 0x0000000FL

/////////////////////////////////////////////////////////////////////////////
// COddButton window

/*!
	\brief Owner-Draw Default Button is a base class for owner-draw buttons that provides basic 
	support for default state handling.

	Derived class can indicate the default state when appropriate simply by calling 
	COddButton::IsDefault method to determine the default state and using 
	any visual effect (e.g. a thin black frame around the button) when it becomes default.

	\note A special static method COddButton::SetDefID can be used to set the default button 
	for the dialog to work around the problems with using a CDialog::SetDefID described in MS kb Q67655 
	(<A HREF="http://support.microsoft.com/default.aspx?scid=KB;EN-US;Q67655&">
	HOWTO: Change or Set the Default Push Button in a Dialog Box
	</A>)
*/
class COddButton : public CButton
{
// Construction
public:
	COddButton();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COddButton)
	protected:
	virtual void PreSubclassWindow();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~COddButton();

// Generated message map functions
protected:
	//{{AFX_MSG(COddButton)
	afx_msg UINT OnGetDlgCode();
	//}}AFX_MSG
	afx_msg LRESULT OnSetStyle(WPARAM wParam, LPARAM lParam);
	
	DECLARE_MESSAGE_MAP()

private:
	// Data members
	BOOL m_bCanBeDefault;	/*!< TRUE to enable default state handling */
	BOOL m_bDefault;		/*!< Set to TRUE when control has default state */
	UINT m_nTypeStyle;		/*!< Type of control */

protected:
// Interface
	
	/// Use to enable or disable default state handling	
	void EnableDefault(BOOL bEnable);

	/// Use to know whether the control has a default state
	BOOL IsDefault() const;

	/// Use to know the type of control to draw
	UINT GetControlType() const;

public:
	/// Use to set the dialog's default pushbutton
	static void SetDefID(CDialog* pDialog, const UINT nID);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ODDBUTTON_H__4CA1E42E_E3C3_4FEA_99A1_E865DEB500DA__INCLUDED_)
