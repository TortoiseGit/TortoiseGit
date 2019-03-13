/////////////////////////////////////////////////////////////////////////////
//
// This file is part of ResizableLib
// http://sourceforge.net/projects/resizablelib
//
// Copyright (C) 2000-2004 by Paolo Messina
// http://www.geocities.com/ppescher - mailto:ppescher@hotmail.com
//
// The contents of this file are subject to the Artistic License (the "License").
// You may not use this file except in compliance with the License. 
// You may obtain a copy of the License at:
// http://www.opensource.org/licenses/artistic-license.html
//
// If you find this code useful, credits would be nice!
//
/////////////////////////////////////////////////////////////////////////////

/*!
 *  @file
 *  @brief Interface for the CResizableSheetState class.
 */

#if !defined(AFX_RESIZABLESHEETSTATE_H__INCLUDED_)
#define AFX_RESIZABLESHEETSTATE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "ResizableWndState.h"

/*! @addtogroup CoreComponents
 *  @{
 */

//! @brief Persists active page in property sheets or wizard dialogs
/*!
 *  Derive from this class when you want to persist the active page
 *  in property sheets or wizard dialogs.
 *  This class is used in the provided resizable counterparts of
 *  the standard MFC property sheet classes.
 */
class CResizableSheetState : public CResizableWndState  
{
protected:

	//! @brief Load and set the active property page 
	BOOL LoadPage(LPCTSTR pszName);

	//! @brief Save the current active property page 
	BOOL SavePage(LPCTSTR pszName);

	//! @brief Override to provide the parent window
	virtual CWnd* GetResizableWnd() const override = 0;

public:
	CResizableSheetState();
	virtual ~CResizableSheetState();
};

// @}
#endif // !defined(AFX_RESIZABLESHEETSTATE_H__INCLUDED_)
