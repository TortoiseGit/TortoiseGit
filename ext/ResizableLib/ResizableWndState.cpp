/////////////////////////////////////////////////////////////////////////////
//
// This file is part of ResizableLib
// http://sourceforge.net/projects/resizablelib
//
// Copyright (C) 2000-2004,2008 by Paolo Messina
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
 *  @brief Implementation of the CResizableWndState class.
 */

#include "stdafx.h"
#include "ResizableWndState.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResizableWndState::CResizableWndState()
{

}

CResizableWndState::~CResizableWndState()
{

}

// used to save/restore window's size and position
// either in the registry or a private .INI file
// depending on your application settings

#define PLACEMENT_ENT	_T("WindowPlacement")
#define PLACEMENT_FMT 	_T("%d,%d,%d,%d,%d,%d,%d,%d")

/*!
 *  This function saves the current window position and size using the base
 *  class persist method. Minimized and maximized state is also optionally
 *  preserved.
 *  @sa CResizableState::WriteState
 *  @note Window coordinates are in the form used by the system functions
 *  GetWindowPlacement and SetWindowPlacement.
 *  
 *  @param pszName String that identifies stored settings
 *  @param bRectOnly Flag that specifies wether to ignore min/max state
 *  
 *  @return Returns @a TRUE if successful, @a FALSE otherwise
 */
BOOL CResizableWndState::SaveWindowRect(LPCTSTR pszName, BOOL bRectOnly)
{
	CString data, id;
	WINDOWPLACEMENT wp;

	SecureZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);
	if (!GetResizableWnd()->GetWindowPlacement(&wp))
		return FALSE;
	
	// use workspace coordinates
	RECT& rc = wp.rcNormalPosition;

	if (bRectOnly)	// save size/pos only (normal state)
	{
		data.Format(PLACEMENT_FMT, rc.left, rc.top,
			rc.right, rc.bottom, SW_SHOWNORMAL, 0, 0, 0);
	}
	else	// save also min/max state
	{
		data.Format(PLACEMENT_FMT, rc.left, rc.top,
			rc.right, rc.bottom, wp.showCmd, wp.flags,
			wp.ptMinPosition.x, wp.ptMinPosition.y);
	}

	id = CString(pszName) + PLACEMENT_ENT;
	return WriteState(id, data);
}

/*!
 *  This function loads and set the current window position and size using
 *  the base class persist method. Minimized and maximized state is also
 *  optionally preserved.
 *  @sa CResizableState::WriteState
 *  @note Window coordinates are in the form used by the system functions
 *  GetWindowPlacement and SetWindowPlacement.
 *  
 *  @param pszName String that identifies stored settings
 *  @param bRectOnly Flag that specifies wether to ignore min/max state
 *  
 *  @return Returns @a TRUE if successful, @a FALSE otherwise
 */
BOOL CResizableWndState::LoadWindowRect(LPCTSTR pszName, BOOL bRectOnly, BOOL bHorzResize, BOOL bVertResize)
{
	CString data, id;
	WINDOWPLACEMENT wp;

	id = CString(pszName) + PLACEMENT_ENT;
	if (!ReadState(id, data))	// never saved before
		return FALSE;
	
	SecureZeroMemory(&wp, sizeof(WINDOWPLACEMENT));
	wp.length = sizeof(WINDOWPLACEMENT);
	if (!GetResizableWnd()->GetWindowPlacement(&wp))
		return FALSE;

	// use workspace coordinates
	RECT& rc = wp.rcNormalPosition;

	long min_width = rc.right - rc.left;
	long min_height = rc.bottom - rc.top;

	if (_stscanf(data, PLACEMENT_FMT, &rc.left, &rc.top,
		&rc.right, &rc.bottom, &wp.showCmd, &wp.flags,
		&wp.ptMinPosition.x, &wp.ptMinPosition.y) == 8)
	{
		if ((!bVertResize) || (rc.bottom - rc.top < min_height))
			rc.bottom = rc.top + min_height;
		if ((!bHorzResize) || (rc.right - rc.left < min_width))
			rc.right = rc.left + min_width;
		if (bRectOnly)	// restore size/pos only
		{
			wp.showCmd = SW_SHOWNORMAL;
			wp.flags = 0;
			return GetResizableWnd()->SetWindowPlacement(&wp);
		}
		else	// restore minimized window to normal or maximized state
		{
			if (wp.showCmd == SW_SHOWMINIMIZED && wp.flags == 0)
				wp.showCmd = SW_SHOWNORMAL;
			else if (wp.showCmd == SW_SHOWMINIMIZED && wp.flags == WPF_RESTORETOMAXIMIZED)
				wp.showCmd = SW_SHOWMAXIMIZED;
			return GetResizableWnd()->SetWindowPlacement(&wp);
		}
	}
	return FALSE;
}
