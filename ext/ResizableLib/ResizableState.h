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
 *  @brief Interface for the CResizableState class.
 */

#if !defined(AFX_RESIZABLESTATE_H__INCLUDED_)
#define AFX_RESIZABLESTATE_H__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*! @addtogroup CoreComponents
 *  @{
 */

//! @brief Provides basic persisting capabilities
/*!
 *  Derive from this class to persist user interface settings, or anything
 *  suitable. The base implementation uses the application profile, which can
 *  be set to either the Registry or an INI File. Other storing methods
 *  can be implemented in derived classes.
 */
class CResizableState  
{
	static CString m_sDefaultStorePath;
	CString m_sStorePath;

protected:
	
	//! @brief Get default path where state is stored
	static LPCTSTR GetDefaultStateStore();
	
	//! @brief Set default path where state is stored
	static void SetDefaultStateStore(LPCTSTR szPath);

	//! @brief Get current path where state is stored
	LPCTSTR GetStateStore();
	
	//! @brief Set current path where state is stored
	void SetStateStore(LPCTSTR szPath);

	//! @name Overridables
	//@{
	
	//! @brief Read state information
	virtual BOOL ReadState(LPCTSTR szId, CString& rsState);
	
	//! @brief Write state information
	virtual BOOL WriteState(LPCTSTR szId, LPCTSTR szState);

	//@}

public:
	CResizableState();
	virtual ~CResizableState();
};

// @}
#endif // !defined(AFX_RESIZABLESTATE_H__INCLUDED_)
