
// TortoiseGitBlameView.h : interface of the CTortoiseGitBlameView class
//


#pragma once

#include "Scintilla.h"
#include "SciLexer.h"
#include "registry.h"
#include "SciEdit.h"

#include "GitBlameLogList.h"
#include "Balloon.h"

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


class CTortoiseGitBlameView : public CView
{
protected: // create from serialization only
	CTortoiseGitBlameView();
	DECLARE_DYNCREATE(CTortoiseGitBlameView)

// Attributes
public:
	CTortoiseGitBlameDoc* GetDocument() const;

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
	afx_msg void OnEditFind();
	afx_msg void OnEditGoto();
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSciPainted(NMHDR*, LRESULT*);
	afx_msg void OnLButtonDown(UINT nFlags,CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags,CPoint point){OnLButtonDown(nFlags,point);CView::OnRButtonDown(nFlags,point);};
	afx_msg void OnSciGetBkColor(NMHDR*, LRESULT*);
	afx_msg void OnMouseHover(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
    afx_msg LRESULT OnFindDialogMessage(WPARAM   wParam,   LPARAM   lParam);
	DECLARE_MESSAGE_MAP()

    static UINT m_FindDialogMessage;
public:

	void UpdateInfo();
	void FocusOn(GitRev *pRev);

	CSciEdit			m_TextView;
	CBalloon			m_ToolTip;

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


	LRESULT SendEditor(UINT Msg, WPARAM wParam=0, LPARAM lParam=0);

	void GetRange(int start, int end, char *text);

	void SetTitle();
	BOOL OpenFile(const char *fileName);
	BOOL OpenLogFile(const char *fileName);

	void Command(int id);
	void Notify(SCNotification *notification);

	void SetAStyle(int style, COLORREF fore, COLORREF back=::GetSysColor(COLOR_WINDOW), int size=-1, CString *face=0);

	void InitialiseEditor();
    void InitSize();
	LONG GetBlameWidth();
	void DrawBlame(HDC hDC);
	void DrawHeader(HDC hDC);
	void DrawLocatorBar(HDC hDC);
	void StartSearch();
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

	LONG						m_mouserev;
	LONG						m_MouseLine;
	LONG						m_selectedrev;
	LONG						m_selectedorigrev;
	CString						m_SelectedHash;
	CString						m_selecteddate;
	static long					m_gotoline;
	long						m_lowestrev;
	long						m_highestrev;
	bool						m_colorage;

//	std::vector<bool>		m_Mergelines;
	std::vector<LONG>		m_ID;
	std::vector<LONG>		m_LineNum;
//	std::vector<LONG>		m_Origrevs;
	std::vector<CString>	m_Dates;
	std::vector<CString>	m_Authors;
	std::vector<CString>	m_CommitHash;

//	std::vector<CString>	m_Paths;
//	std::map<LONG, CString>	logmessages;
	char						m_szTip[MAX_LOG_LENGTH*2+6];
	wchar_t						m_wszTip[MAX_LOG_LENGTH*2+6];
	void StringExpand(LPSTR str);
	void StringExpand(LPWSTR str);
	BOOL						ttVisible;

	CLogDataVector *		GetLogData();

	BOOL m_bShowLine;

protected:
	void CreateFont();
	void SetupLexer(CString filename);
	void SetupCppLexer();
	COLORREF InterColor(COLORREF c1, COLORREF c2, int Slider);
	CString GetAppDirectory();
	std::vector<COLORREF>		colors;
	HFONT						m_font;
	HFONT						m_italicfont;
	LONG						m_blamewidth;
	LONG						m_revwidth;
	LONG						m_datewidth;
	LONG						m_authorwidth;
	LONG						m_pathwidth;
	LONG						m_linewidth;
	LONG						m_SelectedLine; ///< zero-based

	COLORREF					m_mouserevcolor;
	COLORREF					m_mouseauthorcolor;
	COLORREF					m_selectedrevcolor;
	COLORREF					m_selectedauthorcolor;
	COLORREF					m_windowcolor;
	COLORREF					m_textcolor;
	COLORREF					m_texthighlightcolor;

	LRESULT						m_directFunction;
	LRESULT						m_directPointer;
	FINDREPLACE					fr;
	TCHAR						szFindWhat[80];

	CRegStdWORD					m_regOldLinesColor;
	CRegStdWORD					m_regNewLinesColor;

	CGitBlameLogList * GetLogList();

    CFindReplaceDialog          *m_pFindDialog;

	DWORD						m_DateFormat;	// DATE_SHORTDATE or DATE_LONGDATE
};

#ifndef _DEBUG  // debug version in TortoiseGitBlameView.cpp
inline CTortoiseGitBlameDoc* CTortoiseGitBlameView::GetDocument() const
   { return reinterpret_cast<CTortoiseGitBlameDoc*>(m_pDocument); }
#endif

