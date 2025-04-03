// TortoiseGit - a Windows shell extension for easy version control

// Copyright (c) 2003 by Andreas Kapust <info@akinstaller.de>; <http://www.codeproject.com/Articles/2607/AutoComplete-without-IAutoComplete>
// Copyright (C) 2009, 2012-2013, 2015, 2018, 2023, 2025 - TortoiseGit

// Licensed under: The Code Project Open License (CPOL); <http://www.codeproject.com/info/cpol10.aspx>

#if !defined(AFX_ACWND_H__5CED9BF8_C1CB_4A74_B022_ABA25680CC42__INCLUDED_)
#define AFX_ACWND_H__5CED9BF8_C1CB_4A74_B022_ABA25680CC42__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ACWnd.h : Header-Datei
//

/*********************************************************************
*
* CACListWnd
* Copyright (c) 200 by Andreas Kapust
* All rights reserved.
* info@akinstaller.de
*
*********************************************************************/

#define ENAC_UPDATE        WM_USER + 1200
/////////////////////////////////////////////////////////////////////////////
// Fenster CACListWnd
#define IDTimerInstall 10
class CACListWnd : public CWnd
{
	// Konstruktion
public:
	CACListWnd();
	void Init(CWnd *pWnd);
	bool EnsureVisible(int item,bool m_bWait);
	bool SelectItem(int item);
	int FindString(int nStartAfter, LPCWSTR lpszString, bool m_bDisplayOnly = false);
	int FindStringExact(int nIndexStart, LPCWSTR lpszFind);
	int SelectString(LPCWSTR lpszString);
	bool GetText(int item, CString& m_Text);
	void AddSearchString(LPCWSTR lpszString){ m_SearchList.Add(lpszString); }
	void RemoveAll(){m_SearchList.RemoveAll(); m_DisplayList.RemoveAll();}
	CString GetString();
	CString GetNextString(int m_iChar);

	void CopyList();
	void SortSearchList(){SortList(m_SearchList);}
	// Attribute
public:
	CListCtrl m_List;
	CString m_DisplayStr;
	wchar_t m_PrefixChar = L'\0';
	long m_lMode = 0;
	// Operationen
public:
	CStringArray m_SearchList;
	// Überschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktionsüberschreibungen
	//{{AFX_VIRTUAL(CACListWnd)
	//}}AFX_VIRTUAL

	// Implementierung
public:
	virtual ~CACListWnd();
	void DrawItem(CDC* pDC,long m_lItem,long width);

	// Generierte Nachrichtenzuordnungsfunktionen
protected:
	//{{AFX_MSG(CACListWnd)
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnNcPaint();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	CStringArray m_DisplayList;
	CScrollBar m_VertBar, m_HoriBar;
	CRect m_LastSize, m_ParentRect;
	CFont* pFontDC = nullptr;
	CFont fontDC, boldFontDC;
	CEdit* m_pEditParent = nullptr;
	CFont m_uiFont;
	LOGFONT logfont{};

	int m_nIDTimer = 0;
	long m_lTopIndex = 0;
	long m_lCount = 0;
	long m_ItemHeight = 16;
	long m_VisibleItems = 0;
	long m_lSelItem = -1;

	int HitTest(CPoint point);
	void SetScroller();
	void SetProp();
	long ScrollBarWidth();
	void InvalidateAndScroll();
	void SortList(CStringArray& list);
	static int CompareString(const void* p1, const void* p2);

public:
	int GetSelectedItem() const { return m_lSelItem; }
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ fügt unmittelbar vor der vorhergehenden Zeile zusätzliche Deklarationen ein.

#endif // AFX_ACWND_H__5CED9BF8_C1CB_4A74_B022_ABA25680CC42__INCLUDED_
