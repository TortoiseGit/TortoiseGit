// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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


// TortoiseGitBlameView.h : interface of the CTortoiseGitBlameView class
//


#pragma once

#include "Scintilla.h"
#include "SciLexer.h"
#include "registry.h"
#include "SciEdit.h"

#include "GitBlameLogList.h"
#include "Tooltip.h"

const COLORREF black = RGB(0,0,0);
const COLORREF white = RGB(0xff,0xff,0xff);
const COLORREF red = RGB(0xFF, 0, 0);
const COLORREF offWhite = RGB(0xFF, 0xFB, 0xF0);
const COLORREF darkGreen = RGB(0, 0x80, 0);
const COLORREF darkBlue = RGB(0, 0, 0x80);
const COLORREF lightBlue = RGB(0xA6, 0xCA, 0xF0);
const int blockSize = 128 * 1024;

#define BLAMESPACE 5
#define HEADER_HEIGHT 18
#define LOCATOR_WIDTH 10

#define MAX_LOG_LENGTH 2000


#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp)                        ((int)(short)HIWORD(lp))
#endif

class CSciEditBlame: public CSciEdit
{
	DECLARE_DYNAMIC(CSciEditBlame)
public:
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
	{
		switch (nChar)
		{
			case (VK_ESCAPE):
			{
				if ((Call(SCI_AUTOCACTIVE)==0)&&(Call(SCI_CALLTIPACTIVE)==0))
				{
					::SendMessage(::AfxGetApp()->GetMainWnd()->m_hWnd, WM_CLOSE, 0, 0);
					return;
				}
			}
			break;
		}
		CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
	}
};

class CTortoiseGitBlameView : public CView
{
protected: // create from serialization only
	CTortoiseGitBlameView();
	DECLARE_DYNCREATE(CTortoiseGitBlameView)

// Attributes
public:
	CTortoiseGitBlameDoc* GetDocument() const;
	int GetEncode(unsigned char * buffer, int size, int *bomoffset);
// Operations
public:

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~CTortoiseGitBlameView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnChangeEncode(UINT nID);
	afx_msg void OnEditFind();
	afx_msg void OnEditGoto();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSciPainted(NMHDR*, LRESULT*);
	afx_msg void OnLButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags,CPoint point){OnLButtonDown(nFlags,point);CView::OnRButtonDown(nFlags,point);};
	afx_msg void OnSciGetBkColor(NMHDR*, LRESULT*);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg LRESULT OnFindDialogMessage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnViewNext();
	afx_msg void OnViewPrev();
	afx_msg void OnViewToggleAuthor();
	afx_msg void OnUpdateViewToggleAuthor(CCmdUI *pCmdUI);
	afx_msg void OnViewToggleFollowRenames();
	afx_msg void OnUpdateViewToggleFollowRenames(CCmdUI *pCmdUI);
	afx_msg void CopyHashToClipboard();
	afx_msg void OnUpdateBlamePopupBlamePrevious(CCmdUI *pCmdUI);
	afx_msg void OnUpdateBlamePopupDiffPrevious(CCmdUI *pCmdUI);
	afx_msg void OnUpdateViewCopyToClipboard(CCmdUI *pCmdUI);

	int FindNextLine(CGitHash commithash, bool bUpOrDown=false);
	int FindFirstLine(CGitHash commithash, int line)
	{
		while(line>=0)
		{
			if( m_CommitHash[line] != commithash )
			{
				return line++;
			}
			line--;
		}
		return line;
	}

	DECLARE_MESSAGE_MAP()

	static UINT m_FindDialogMessage;
public:

	void UpdateInfo(int encode = 0);
	void FocusOn(GitRev *pRev);

	CSciEditBlame		m_TextView;
	CToolTips			m_ToolTip;

	HINSTANCE hInstance;
	HINSTANCE hResource;
	HWND currentDialog;
	HWND wMain;
	HWND m_wEditor;
	HWND wBlame;
	HWND wHeader;
	HWND wLocator;
	HWND hwndTT;

	BOOL bIgnoreEOL;
	BOOL bIgnoreSpaces;
	BOOL bIgnoreAllSpaces;

	BOOL m_bShowAuthor;
	BOOL m_bShowDate;
	BOOL m_bFollowRenames;

	LRESULT SendEditor(UINT Msg, WPARAM wParam=0, LPARAM lParam=0);

	void SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW), int size=-1, CString *face=0);

	void InitialiseEditor();
	LONG GetBlameWidth();
	void DrawBlame(HDC hDC);
	void DrawLocatorBar(HDC hDC);
	void StartSearch();
	void CopyToClipboard();
	void CopySelectedLogToClipboard();
	void BlamePreviousRevision();
	void DiffPreviousRevision();
	void ShowLog();
	bool DoSearch(CString what, DWORD flags);
	bool GotoLine(long line);
	bool ScrollToLine(long line);
	void GotoLineDlg();
	static INT_PTR CALLBACK GotoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

	void SetSelectedLine(LONG line) { m_SelectedLine=line;};

	LONG					m_mouserev;
	LONG					m_MouseLine;
	LONG					m_selectedrev;
	LONG					m_selectedorigrev;
	CGitHash				m_SelectedHash;
	CGitHash				m_selecteddate;
	static long				m_gotoline;
	long					m_lowestrev;
	long					m_highestrev;
	bool					m_colorage;

	std::vector<LONG>		m_ID;
	std::vector<LONG>		m_LineNum;
	std::vector<CString>	m_Dates;
	std::vector<CString>	m_Authors;
	std::vector<CGitHash>	m_CommitHash;

	std::map<CString,GitRev> m_NoListCommit;

	char					m_szTip[MAX_LOG_LENGTH*2+6];
	wchar_t					m_wszTip[MAX_LOG_LENGTH*2+6];
	void StringExpand(LPSTR str);
	void StringExpand(LPWSTR str);
	BOOL					ttVisible;

	CLogDataVector *		GetLogData();

	BOOL m_bShowLine;

protected:
	void CreateFont();
	void SetupLexer(CString filename);
	void SetupCppLexer();
	COLORREF InterColor(COLORREF c1, COLORREF c2, int Slider);
	CString GetAppDirectory();
	std::vector<COLORREF>	colors;
	HFONT					m_font;
	HFONT					m_italicfont;
	LONG					m_blamewidth;
	LONG					m_revwidth;
	LONG					m_datewidth;
	LONG					m_authorwidth;
	LONG					m_linewidth;
	LONG					m_SelectedLine; ///< zero-based

	COLORREF				m_mouserevcolor;
	COLORREF				m_mouseauthorcolor;
	COLORREF				m_selectedrevcolor;
	COLORREF				m_selectedauthorcolor;
	COLORREF				m_windowcolor;
	COLORREF				m_textcolor;
	COLORREF				m_texthighlightcolor;

	LRESULT					m_directFunction;
	LRESULT					m_directPointer;
	FINDREPLACE				fr;
	TCHAR					szFindWhat[80];

	CRegStdDWORD				m_regOldLinesColor;
	CRegStdDWORD				m_regNewLinesColor;

	CGitBlameLogList * GetLogList();

	CFindReplaceDialog		*m_pFindDialog;

	char					*m_Buffer;

	DWORD					m_DateFormat;	// DATE_SHORTDATE or DATE_LONGDATE
	bool					m_bRelativeTimes;	// Show relative times

	CString					m_sRev;
	CString					m_sAuthor;
	CString					m_sDate;
	CString					m_sMessage;
};

#ifndef _DEBUG  // debug version in TortoiseGitBlameView.cpp
inline CTortoiseGitBlameDoc* CTortoiseGitBlameView::GetDocument() const
   { return reinterpret_cast<CTortoiseGitBlameDoc*>(m_pDocument); }
#endif

