// TortoiseGit - a Windows shell extension for easy version control

// Copyright (c) 2003 by Andreas Kapust <info@akinstaller.de>; <http://www.codeproject.com/Articles/2607/AutoComplete-without-IAutoComplete>
// Copyright (C) 2009,2012-2013 - TortoiseGit

// Licensed under: The Code Project Open License (CPOL); <http://www.codeproject.com/info/cpol10.aspx>

#if !defined(AFX_ACEDIT_H__56D21C13_ECEA_41DF_AADF_55980E861AC2__INCLUDED_)
#define AFX_ACEDIT_H__56D21C13_ECEA_41DF_AADF_55980E861AC2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ACEdit.h : Header-Datei
//

/*********************************************************************
*
* CACEdit
* Copyright (c) 2003 by Andreas Kapust
* All rights reserved.
* info@akinstaller.de
*
*********************************************************************/


#define _MODE_ONLY_FILES		(1L << 16)
#define _MODE_ONLY_DIRS			(1L << 17)

#define _MODE_STANDARD_			(1L << 0)
#define _MODE_SEPARATION_		(1L << 1)
#define _MODE_FILESYSTEM_		(1L << 2)
#define _MODE_FS_START_DIR_		(1L << 3)
#define _MODE_CURSOR_O_LIST_	(1L << 4)
#define _MODE_FIND_ALL_			(1L << 5)

#define _MODE_FS_ONLY_FILE_	(_MODE_FILESYSTEM_|_MODE_ONLY_FILES)
#define _MODE_FS_ONLY_DIR_	(_MODE_FILESYSTEM_|_MODE_ONLY_DIRS)
#define _MODE_SD_ONLY_FILE_	(_MODE_FS_START_DIR_|_MODE_ONLY_FILES)
#define _MODE_SD_ONLY_DIR_	(_MODE_FS_START_DIR_|_MODE_ONLY_DIRS)  //Fix 1.2

/////////////////////////////////////////////////////////////////////////////
// Fenster CACEdit
#include "ACListWnd.h"


class CACEdit : public CWnd //CEdit
{
	// Konstruktion
public:
	CACEdit();
	void SetMode(int iMode=_MODE_STANDARD_);
	void SetSeparator(LPCTSTR lpszString, TCHAR lpszPrefixChar = L'\0')
	{
		m_SeparationStr = lpszString;
		m_Liste.m_PrefixChar = m_PrefixChar = lpszPrefixChar;
		SetMode(_MODE_SEPARATION_);
	}

	// CComboBox
	int AddString( LPCTSTR lpszString);
	int GetLBText( int nIndex, LPTSTR lpszText );
	void GetLBText( int nIndex, CString& rString );
	int SetDroppedWidth(UINT nWidth);
	int FindString( int nStartAfter, LPCTSTR lpszString );
	int SelectString( int nStartAfter, LPCTSTR lpszString );
	void ShowDropDown(BOOL bShowIt = TRUE );
	void ResetContent();
	int GetCurSel();
	// Attribute
public:
	void Init();
	void AddSearchString(LPCTSTR lpszString);
	void AddSearchStrings(LPCTSTR Strings[]);
	void RemoveSearchAll();
	void SetStartDirectory(LPCTSTR lpszString);
	// Operationen
public:

	// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CACEdit)
public:
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;
	virtual ULONG GetGestureStatus(CPoint /*ptTouch*/) override { return 0; }
	//}}AFX_VIRTUAL

	// Implementierung
public:
	virtual ~CACEdit();
	CACListWnd m_Liste;
	// Generierte Nachrichtenzuordnungsfunktionen
protected:
	CString m_EditText, m_SeparationStr,m_LastDirectory;
	TCHAR m_PrefixChar;
	int m_iMode;
	//{{AFX_MSG(CACEdit)
	afx_msg void OnKillfocus();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChange();
	afx_msg void OnCloseList();
	//}}AFX_MSG

	afx_msg LRESULT OnUpdateFromList(WPARAM lParam, LPARAM wParam);
	DECLARE_MESSAGE_MAP()


	void ReadDirectory(CString m_Dir);
	int FindSepLeftPos(int pos, bool FindSepLeftPos = false);
	int FindSepLeftPos2(int pos);
	int FindSepRightPos(int pos);
	bool HandleKey(UINT nChar, bool m_bFromChild);

	bool m_CursorMode;
	int m_iType;
	CEdit *m_pEdit;

	TCHAR m_szDrive[_MAX_DRIVE], m_szDir[_MAX_DIR],m_szFname[_MAX_FNAME], m_szExt[_MAX_EXT];
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_ACEDIT_H__56D21C13_ECEA_41DF_AADF_55980E861AC2__INCLUDED_
