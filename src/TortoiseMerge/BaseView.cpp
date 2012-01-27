// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2003-2009,2011 - TortoiseSVN
// Copyright (C) 2011 Sven Strickroth <email@cs-ware.de>

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

#include "stdafx.h"
#include "registry.h"
#include "TortoiseMerge.h"
#include "MainFrm.h"
#include "BaseView.h"
#include "DiffColors.h"
#include "StringUtils.h"

#include <deque>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MARGINWIDTH 20
#define HEADERHEIGHT 10

#define MAXFONTS 8

#define INLINEADDED_COLOR			RGB(255, 255, 150)
#define INLINEREMOVED_COLOR			RGB(200, 100, 100)
#define MODIFIED_COLOR				RGB(220, 220, 255)

#define IDT_SCROLLTIMER 101

CBaseView * CBaseView::m_pwndLeft = NULL;
CBaseView * CBaseView::m_pwndRight = NULL;
CBaseView * CBaseView::m_pwndBottom = NULL;
CLocatorBar * CBaseView::m_pwndLocator = NULL;
CLineDiffBar * CBaseView::m_pwndLineDiffBar = NULL;
CMFCStatusBar * CBaseView::m_pwndStatusBar = NULL;
CMainFrame * CBaseView::m_pMainFrame = NULL;

IMPLEMENT_DYNCREATE(CBaseView, CView)

CBaseView::CBaseView()
{
	m_pCacheBitmap = NULL;
	m_pViewData = NULL;
	m_pOtherViewData = NULL;
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nScreenChars = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = -1;
	m_nTopLine = 0;
	m_nOffsetChar = 0;
	m_nDigits = 0;
	m_nMouseLine = -1;
	m_bMouseWithin = FALSE;
	m_bIsHidden = FALSE;
	lineendings = EOL_AUTOLINE;
	m_bCaretHidden = true;
	m_ptCaretPos.x = 0;
	m_ptCaretPos.y = 0;
	m_nCaretGoalPos = 0;
	m_ptSelectionStartPos = m_ptCaretPos;
	m_ptSelectionEndPos = m_ptCaretPos;
	m_ptSelectionOrigin = m_ptCaretPos;
	m_bFocused = FALSE;
	m_bShowSelection = true;
	texttype = CFileTextLines::AUTOTYPE;
	m_bViewWhitespace = CRegDWORD(_T("Software\\TortoiseMerge\\ViewWhitespaces"), 1);
	m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
	m_bShowInlineDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
	m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineAdded"), INLINEADDED_COLOR);
	m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
	m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorModifiedB"), MODIFIED_COLOR);
	m_WhiteSpaceFg = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\Whitespace"), GetSysColor(COLOR_GRAYTEXT));
	m_sWordSeparators = CRegString(_T("Software\\TortoiseMerge\\WordSeparators"), _T("[]();.,{}!@#$%^&*-+=|/\\<>'`~"));;
	m_bIconLFs = CRegDWORD(_T("Software\\TortoiseMerge\\IconLFs"), 0);
	m_nSelBlockStart = -1;
	m_nSelBlockEnd = -1;
	m_bModified = FALSE;
	m_bOtherDiffChecked = false;
	m_bInlineWordDiff = true;
	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	for (int i=0; i<MAXFONTS; i++)
	{
		m_apFonts[i] = NULL;
	}
	m_hConflictedIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CONFLICTEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hConflictedIgnoredIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_CONFLICTEDIGNOREDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hRemovedIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_REMOVEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hAddedIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_ADDEDLINE), 
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hWhitespaceBlockIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_WHITESPACELINE),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hEqualIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_EQUALLINE),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hLineEndingCR = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_LINEENDINGCR),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hLineEndingCRLF = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_LINEENDINGCRLF),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hLineEndingLF = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_LINEENDINGLF),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	m_hEditedIcon = (HICON)::LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDI_LINEEDITED),
									IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	for (int i=0; i<1024; ++i)
		m_sConflictedText += _T("??");
	m_sNoLineNr.LoadString(IDS_EMPTYLINETT);
	EnableToolTips();
}

CBaseView::~CBaseView()
{
	if (m_pCacheBitmap)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
	}
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	DestroyIcon(m_hAddedIcon);
	DestroyIcon(m_hRemovedIcon);
	DestroyIcon(m_hConflictedIcon);
	DestroyIcon(m_hConflictedIgnoredIcon);
	DestroyIcon(m_hWhitespaceBlockIcon);
	DestroyIcon(m_hEqualIcon);
	DestroyIcon(m_hLineEndingCR);
	DestroyIcon(m_hLineEndingCRLF);
	DestroyIcon(m_hLineEndingLF);
	DestroyIcon(m_hEditedIcon);
}

BEGIN_MESSAGE_MAP(CBaseView, CView)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_SETCURSOR()
	ON_WM_KILLFOCUS()
	ON_WM_SETFOCUS()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(ID_NAVIGATE_NEXTDIFFERENCE, OnMergeNextdifference)
	ON_COMMAND(ID_NAVIGATE_PREVIOUSDIFFERENCE, OnMergePreviousdifference)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_COMMAND(ID_EDIT_COPY, OnEditCopy)
	ON_WM_MOUSEMOVE()
	ON_COMMAND(ID_NAVIGATE_PREVIOUSCONFLICT, OnMergePreviousconflict)
	ON_COMMAND(ID_NAVIGATE_NEXTCONFLICT, OnMergeNextconflict)
	ON_WM_CHAR()
	ON_COMMAND(ID_CARET_DOWN, &CBaseView::OnCaretDown)
	ON_COMMAND(ID_CARET_LEFT, &CBaseView::OnCaretLeft)
	ON_COMMAND(ID_CARET_RIGHT, &CBaseView::OnCaretRight)
	ON_COMMAND(ID_CARET_UP, &CBaseView::OnCaretUp)
	ON_COMMAND(ID_CARET_WORDLEFT, &CBaseView::OnCaretWordleft)
	ON_COMMAND(ID_CARET_WORDRIGHT, &CBaseView::OnCaretWordright)
	ON_COMMAND(ID_EDIT_CUT, &CBaseView::OnEditCut)
	ON_COMMAND(ID_EDIT_PASTE, &CBaseView::OnEditPaste)
	ON_WM_MOUSELEAVE()
	ON_WM_TIMER()
	ON_COMMAND(ID_EDIT_SELECTALL, &CBaseView::OnEditSelectall)
END_MESSAGE_MAP()


void CBaseView::DocumentUpdated()
{
	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nScreenChars = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = -1;
	m_nTopLine = 0;
	m_bModified = FALSE;
	m_bOtherDiffChecked = false;
	m_nDigits = 0;
	m_nMouseLine = -1;

	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\TabSize"), 4);
	m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseMerge\\ViewLinenumbers"), 1);
	m_bShowInlineDiff = CRegDWORD(_T("Software\\TortoiseMerge\\DisplayBinDiff"), TRUE);
	m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineAdded"), INLINEADDED_COLOR);
	m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
	m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\ColorModifiedB"), MODIFIED_COLOR);
	m_WhiteSpaceFg = CRegDWORD(_T("Software\\TortoiseMerge\\Colors\\Whitespace"), GetSysColor(COLOR_GRAYTEXT));
	m_bIconLFs = CRegDWORD(_T("Software\\TortoiseMerge\\IconLFs"), 0);
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	m_nSelBlockStart = -1;
	m_nSelBlockEnd = -1;
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
	UpdateStatusBar();
	Invalidate();
}

void CBaseView::UpdateStatusBar()
{
	int nRemovedLines = 0;
	int nAddedLines = 0;
	int nConflictedLines = 0;

	if (m_pViewData)
	{
		for (int i=0; i<m_pViewData->GetCount(); i++)
		{
			DiffStates state = m_pViewData->GetState(i);
			switch (state)
			{
			case DIFFSTATE_ADDED:
			case DIFFSTATE_IDENTICALADDED:
			case DIFFSTATE_THEIRSADDED:
			case DIFFSTATE_YOURSADDED:
			case DIFFSTATE_CONFLICTADDED:
				nAddedLines++;
				break;
			case DIFFSTATE_IDENTICALREMOVED:
			case DIFFSTATE_REMOVED:
			case DIFFSTATE_THEIRSREMOVED:
			case DIFFSTATE_YOURSREMOVED:
				nRemovedLines++;
				break;
			case DIFFSTATE_CONFLICTED:
			case DIFFSTATE_CONFLICTED_IGNORED:
				nConflictedLines++;
				break;
			}
		}
	}

	CString sBarText;
	CString sTemp;

	switch (texttype)
	{
	case CFileTextLines::ASCII:
		sBarText = _T("ASCII ");
		break;
	case CFileTextLines::BINARY:
		sBarText = _T("BINARY ");
		break;
	case CFileTextLines::UNICODE_LE:
		sBarText = _T("UTF-16LE ");
		break;
	case CFileTextLines::UTF8:
		sBarText = _T("UTF8 ");
		break;
	case CFileTextLines::UTF8BOM:
		sBarText = _T("UTF8 BOM ");
		break;
	}

	switch(lineendings)
	{
	case EOL_LF:
		sBarText += _T("LF ");
		break;
	case EOL_CRLF:
		sBarText += _T("CRLF ");
		break;
	case EOL_LFCR:
		sBarText += _T("LFCR ");
		break;
	case EOL_CR:
		sBarText += _T("CR ");
		break;
	}

	if (sBarText.IsEmpty())
		sBarText  += _T(" / ");

	if (nRemovedLines)
	{
		sTemp.Format(IDS_STATUSBAR_REMOVEDLINES, nRemovedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	}
	if (nAddedLines)
	{
		sTemp.Format(IDS_STATUSBAR_ADDEDLINES, nAddedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	}
	if (nConflictedLines)
	{
		sTemp.Format(IDS_STATUSBAR_CONFLICTEDLINES, nConflictedLines);
		if (!sBarText.IsEmpty())
			sBarText += _T(" / ");
		sBarText += sTemp;
	}
	if (m_pwndStatusBar)
	{
		UINT nID;
		UINT nStyle;
		int cxWidth;
		int nIndex = m_pwndStatusBar->CommandToIndex(m_nStatusBarID);
		if (m_nStatusBarID == ID_INDICATOR_BOTTOMVIEW)
		{
			sBarText.Format(IDS_STATUSBAR_CONFLICTS, nConflictedLines);
		}
		if (m_nStatusBarID == ID_INDICATOR_LEFTVIEW)
		{
			sTemp.LoadString(IDS_STATUSBAR_LEFTVIEW);
			sBarText = sTemp+sBarText;
		}
		if (m_nStatusBarID == ID_INDICATOR_RIGHTVIEW)
		{
			sTemp.LoadString(IDS_STATUSBAR_RIGHTVIEW);
			sBarText = sTemp+sBarText;
		}
		m_pwndStatusBar->GetPaneInfo(nIndex, nID, nStyle, cxWidth);
		//calculate the width of the text
		CDC * pDC = m_pwndStatusBar->GetDC();
		if (pDC)
		{
			CSize size = pDC->GetTextExtent(sBarText);
			m_pwndStatusBar->SetPaneInfo(nIndex, nID, nStyle, size.cx+2);
			ReleaseDC(pDC);
		}
		m_pwndStatusBar->SetPaneText(nIndex, sBarText);
	}
}

BOOL CBaseView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CView::PreCreateWindow(cs))
		return FALSE;

	cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;
	cs.lpszClass = AfxRegisterWndClass(CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS, 
		::LoadCursor(NULL, IDC_ARROW), reinterpret_cast<HBRUSH>(COLOR_WINDOW+1), NULL);

	CWnd *pParentWnd = CWnd::FromHandlePermanent(cs.hwndParent);
	if (pParentWnd == NULL || ! pParentWnd->IsKindOf(RUNTIME_CLASS(CSplitterWnd)))
	{
		//	View must always create its own scrollbars,
		//	if only it's not used within splitter
		cs.style |= (WS_HSCROLL | WS_VSCROLL);
	}
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS);
	return TRUE;
}

CFont* CBaseView::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/, BOOL bStrikeOut /*= FALSE*/)
{
	int nIndex = 0;
	if (bBold)
		nIndex |= 1;
	if (bItalic)
		nIndex |= 2;
	if (bStrikeOut)
		nIndex |= 4;
	if (m_apFonts[nIndex] == NULL)
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = (BYTE) bItalic;
		m_lfBaseFont.lfStrikeOut = (BYTE) bStrikeOut;
		if (bStrikeOut)
			m_lfBaseFont.lfStrikeOut = (BYTE)(DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\StrikeOut"), TRUE);
		CDC * pDC = GetDC();
		if (pDC)
		{
			m_lfBaseFont.lfHeight = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseMerge\\LogFontSize"), 10), GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
			ReleaseDC(pDC);
		}
		_tcsncpy_s(m_lfBaseFont.lfFaceName, 32, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseMerge\\LogFontName"), _T("Courier New")), 32);
		if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
		{
			delete m_apFonts[nIndex];
			m_apFonts[nIndex] = NULL;
			return CView::GetFont();
		}
	}
	return m_apFonts[nIndex];
}

void CBaseView::CalcLineCharDim()
{
	CDC *pDC = GetDC();
	CFont *pOldFont = pDC->SelectObject(GetFont());
	CSize szCharExt = pDC->GetTextExtent(_T("X"));
	m_nLineHeight = szCharExt.cy;
	if (m_nLineHeight <= 0)
		m_nLineHeight = -1;
	m_nCharWidth = szCharExt.cx;
	if (m_nCharWidth <= 0)
		m_nCharWidth = -1;
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);
}

int CBaseView::GetScreenChars()
{
	if (m_nScreenChars == -1)
	{
		CRect rect;
		GetClientRect(&rect);
		m_nScreenChars = (rect.Width() - GetMarginWidth()) / GetCharWidth();
	}
	return m_nScreenChars;
}

int CBaseView::GetAllMinScreenChars() const 
{
	int nChars = 0;
	if (IsLeftViewGood())
		nChars = m_pwndLeft->GetScreenChars();
	if (IsRightViewGood())
		nChars = (nChars < m_pwndRight->GetScreenChars() ? nChars : m_pwndRight->GetScreenChars());
	if (IsBottomViewGood())
		nChars = (nChars < m_pwndBottom->GetScreenChars() ? nChars : m_pwndBottom->GetScreenChars());
	return nChars;
}

int CBaseView::GetAllMaxLineLength() const 
{
	int nLength = 0;
	if (IsLeftViewGood())
		nLength = m_pwndLeft->GetMaxLineLength();
	if (IsRightViewGood())
		nLength = (nLength > m_pwndRight->GetMaxLineLength() ? nLength : m_pwndRight->GetMaxLineLength());
	if (IsBottomViewGood())
		nLength = (nLength > m_pwndBottom->GetMaxLineLength() ? nLength : m_pwndBottom->GetMaxLineLength());
	return nLength;
}

int CBaseView::GetLineHeight()
{
	if (m_nLineHeight == -1)
		CalcLineCharDim();
	if (m_nLineHeight <= 0)
		return 1;
	return m_nLineHeight;
}

int CBaseView::GetCharWidth()
{
	if (m_nCharWidth == -1)
		CalcLineCharDim();
	if (m_nCharWidth <= 0)
		return 1;
	return m_nCharWidth;
}

int CBaseView::GetMaxLineLength()
{
	if (m_nMaxLineLength == -1)
	{
		m_nMaxLineLength = 0;
		int nLineCount = GetLineCount();
		for (int i=0; i<nLineCount; i++)
		{
			int nActualLength = GetLineActualLength(i);
			if (m_nMaxLineLength < nActualLength)
				m_nMaxLineLength = nActualLength;
		}
	}
	return m_nMaxLineLength;
}

int CBaseView::GetLineActualLength(int index) const
{
	if (m_pViewData == NULL)
		return 0;

	return CalculateActualOffset(index, GetLineLength(index));
}

int CBaseView::GetLineLength(int index) const
{
	if (m_pViewData == NULL)
		return 0;
	if (m_pViewData->GetCount() == 0)
		return 0;
	int nLineLength = m_pViewData->GetLine(index).GetLength();
	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetLineCount() const
{
	if (m_pViewData == NULL)
		return 1;
	int nLineCount = m_pViewData->GetCount();
	ASSERT(nLineCount >= 0);
	return nLineCount;
}

LPCTSTR CBaseView::GetLineChars(int index) const
{
	if (m_pViewData == NULL)
		return 0;
	if (m_pViewData->GetCount() == 0)
		return 0;
	return m_pViewData->GetLine(index);
}

void CBaseView::CheckOtherView()
{
	if (m_bOtherDiffChecked)
		return;
	// find out what the 'other' file is
	m_pOtherViewData = NULL;
	if (this == m_pwndLeft && IsRightViewGood())
		m_pOtherViewData = m_pwndRight->m_pViewData;

	if (this == m_pwndRight && IsLeftViewGood())
		m_pOtherViewData = m_pwndLeft->m_pViewData;

	m_bOtherDiffChecked = true;
}

CString CBaseView::GetWhitespaceBlock(CViewData *viewData, int nLineIndex)
{
	enum { MAX_WHITESPACEBLOCK_SIZE	= 8 };
	ASSERT(viewData);
	
	DiffStates origstate = viewData->GetState(nLineIndex);

	// Go back and forward at most MAX_WHITESPACEBLOCK_SIZE lines to see where this block ends
	int nStartBlock = nLineIndex;
	int nEndBlock = nLineIndex;
	while ((nStartBlock > 0) && (nStartBlock > (nLineIndex - MAX_WHITESPACEBLOCK_SIZE)))
	{
		DiffStates state = viewData->GetState(nStartBlock - 1);
		if ((origstate == DIFFSTATE_EMPTY) && (state != DIFFSTATE_NORMAL))
			origstate = state;
		if ((origstate == state) || (state == DIFFSTATE_EMPTY))
			nStartBlock--;
		else
			break;
	}
	while ((nEndBlock < (viewData->GetCount() - 1)) && (nEndBlock < (nLineIndex + MAX_WHITESPACEBLOCK_SIZE)))
	{
		DiffStates state = viewData->GetState(nEndBlock + 1);
		if ((origstate == DIFFSTATE_EMPTY) && (state != DIFFSTATE_NORMAL))
			origstate = state;
		if ((origstate == state) || (state == DIFFSTATE_EMPTY))
			nEndBlock++;
		else
			break;
	}
	
	CString block;
	for (int i = nStartBlock; i <= nEndBlock; ++i)
		block += viewData->GetLine(i);
	return block;
}

bool CBaseView::IsBlockWhitespaceOnly(int nLineIndex, bool& bIdentical)
{
	enum { MAX_WHITESPACEBLOCK_SIZE	= 8 };
	CheckOtherView();
	if (!m_pOtherViewData)
		return false;
	if (
		(m_pViewData->GetState(nLineIndex) == DIFFSTATE_NORMAL) &&
		(m_pOtherViewData->GetLine(nLineIndex) == m_pViewData->GetLine(nLineIndex))
	)
		return false;

	CString mine = GetWhitespaceBlock(m_pViewData, nLineIndex);
	CString other = GetWhitespaceBlock(m_pOtherViewData, min(nLineIndex, m_pOtherViewData->GetCount() - 1));
	bIdentical = mine == other;
	
	mine.Remove(' ');
	mine.Remove('\t');
	mine.Remove('\r');
	mine.Remove('\n');
	other.Remove(' ');
	other.Remove('\t');
	other.Remove('\r');
	other.Remove('\n');
	
	return (mine == other) && (!mine.IsEmpty());
}

int CBaseView::GetLineNumber(int index) const
{
	if (m_pViewData == NULL)
		return -1;
	if (m_pViewData->GetLineNumber(index)==DIFF_EMPTYLINENUMBER)
		return -1;
	return m_pViewData->GetLineNumber(index);
}

int CBaseView::GetScreenLines()
{
	if (m_nScreenLines == -1)
	{
		SCROLLBARINFO sbi;
		sbi.cbSize = sizeof(sbi);
		GetScrollBarInfo(OBJID_HSCROLL, &sbi);
		int scrollBarHeight = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;

		CRect rect;
		GetClientRect(&rect);
		m_nScreenLines = (rect.Height() - HEADERHEIGHT - scrollBarHeight) / GetLineHeight();
	}
	return m_nScreenLines;
}

int CBaseView::GetAllMinScreenLines() const
{
	int nLines = 0;
	if (IsLeftViewGood())
		nLines = m_pwndLeft->GetScreenLines();
	if (IsRightViewGood())
		nLines = (nLines < m_pwndRight->GetScreenLines() ? nLines : m_pwndRight->GetScreenLines());
	if (IsBottomViewGood())
		nLines = (nLines < m_pwndBottom->GetScreenLines() ? nLines : m_pwndBottom->GetScreenLines());
	return nLines;
}

int CBaseView::GetAllLineCount() const
{
	int nLines = 0;
	if (IsLeftViewGood())
		nLines = m_pwndLeft->GetLineCount();
	if (IsRightViewGood())
		nLines = (nLines > m_pwndRight->GetLineCount() ? nLines : m_pwndRight->GetLineCount());
	if (IsBottomViewGood())
		nLines = (nLines > m_pwndBottom->GetLineCount() ? nLines : m_pwndBottom->GetLineCount());
	return nLines;
}

void CBaseView::RecalcAllVertScrollBars(BOOL bPositionOnly /*= FALSE*/)
{
	if (IsLeftViewGood())
		m_pwndLeft->RecalcVertScrollBar(bPositionOnly);
	if (IsRightViewGood())
		m_pwndRight->RecalcVertScrollBar(bPositionOnly);
	if (IsBottomViewGood())
		m_pwndBottom->RecalcVertScrollBar(bPositionOnly);
}

void CBaseView::RecalcVertScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
		si.nPos = m_nTopLine;
	}
	else
	{
		EnableScrollBarCtrl(SB_VERT, TRUE);
		if (GetAllMinScreenLines() >= GetAllLineCount() && m_nTopLine > 0)
		{
			m_nTopLine = 0;
			Invalidate();
		}
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = GetAllLineCount();
		si.nPage = GetAllMinScreenLines();
		si.nPos = m_nTopLine;
	}
	VERIFY(SetScrollInfo(SB_VERT, &si));
}

void CBaseView::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CView::OnVScroll(nSBCode, nPos, pScrollBar);
	if (m_pwndLeft)
		m_pwndLeft->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndRight)
		m_pwndRight->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndBottom)
		m_pwndBottom->OnDoVScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::OnDoVScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar* /*pScrollBar*/, CBaseView * master)
{
	//	Note we cannot use nPos because of its 16-bit nature
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	VERIFY(master->GetScrollInfo(SB_VERT, &si));

	int nPageLines = GetScreenLines();
	int nLineCount = GetLineCount();

	RECT thumbrect;
	POINT thumbpoint;
	int nNewTopLine;

	static LONG textwidth = 0;
	static CString sFormat(MAKEINTRESOURCE(IDS_VIEWSCROLLTIPTEXT));
	switch (nSBCode)
	{
	case SB_TOP:
		nNewTopLine = 0;
		break;
	case SB_BOTTOM:
		nNewTopLine = nLineCount - nPageLines + 1;
		break;
	case SB_LINEUP:
		nNewTopLine = m_nTopLine - 1;
		break;
	case SB_LINEDOWN:
		nNewTopLine = m_nTopLine + 1;
		break;
	case SB_PAGEUP:
		nNewTopLine = m_nTopLine - si.nPage + 1;
		break;
	case SB_PAGEDOWN:
		nNewTopLine = m_nTopLine + si.nPage - 1;
		break;
	case SB_THUMBPOSITION:
		m_ScrollTool.Clear();
		nNewTopLine = si.nTrackPos;
		textwidth = 0;
		break;
	case SB_THUMBTRACK:
		nNewTopLine = si.nTrackPos;
		if (GetFocus() == this)
		{
			GetClientRect(&thumbrect);
			ClientToScreen(&thumbrect);
			thumbpoint.x = thumbrect.right;
			thumbpoint.y = thumbrect.top + ((thumbrect.bottom-thumbrect.top)*si.nTrackPos)/(si.nMax-si.nMin);
			m_ScrollTool.Init(&thumbpoint);
			if (textwidth == 0)
			{
				CString sTemp = sFormat;
				sTemp.Format(sFormat, m_nDigits, 10*m_nDigits-1);
				textwidth = m_ScrollTool.GetTextWidth(sTemp);
			}
			thumbpoint.x -= textwidth;
			int line = GetLineNumber(nNewTopLine);
			if (line >= 0)
				m_ScrollTool.SetText(&thumbpoint, sFormat, m_nDigits, GetLineNumber(nNewTopLine)+1);
			else
				m_ScrollTool.SetText(&thumbpoint, m_sNoLineNr);
		}
		break;
	default:
		return;
	}

	if (nNewTopLine < 0)
		nNewTopLine = 0;
	if (nNewTopLine >= nLineCount)
		nNewTopLine = nLineCount - 1;
	ScrollToLine(nNewTopLine);
}

void CBaseView::RecalcAllHorzScrollBars(BOOL bPositionOnly /*= FALSE*/)
{
	if (IsLeftViewGood())
		m_pwndLeft->RecalcHorzScrollBar(bPositionOnly);
	if (IsRightViewGood())
		m_pwndRight->RecalcHorzScrollBar(bPositionOnly);
	if (IsBottomViewGood())
		m_pwndBottom->RecalcHorzScrollBar(bPositionOnly);
}

void CBaseView::RecalcHorzScrollBar(BOOL bPositionOnly /*= FALSE*/)
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	if (bPositionOnly)
	{
		si.fMask = SIF_POS;
		si.nPos = m_nOffsetChar;
	}
	else
	{
		EnableScrollBarCtrl(SB_HORZ, TRUE);
		if (GetAllMinScreenChars() >= GetAllMaxLineLength() && m_nOffsetChar > 0)
		{
			m_nOffsetChar = 0;
			Invalidate();
		}
		si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
		si.nMin = 0;
		si.nMax = GetAllMaxLineLength() + GetMarginWidth()/GetCharWidth();
		si.nPage = GetAllMinScreenChars();
		si.nPos = m_nOffsetChar;
	}
	VERIFY(SetScrollInfo(SB_HORZ, &si));
}

void CBaseView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar) 
{
	CView::OnHScroll(nSBCode, nPos, pScrollBar);
	if (m_pwndLeft)
		m_pwndLeft->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndRight)
		m_pwndRight->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndBottom)
		m_pwndBottom->OnDoHScroll(nSBCode,  nPos, pScrollBar, this);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::OnDoHScroll(UINT nSBCode, UINT /*nPos*/, CScrollBar* /*pScrollBar*/, CBaseView * master) 
{
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	VERIFY(master->GetScrollInfo(SB_HORZ, &si));

	int nPageChars = GetScreenChars();
	int nMaxLineLength = GetMaxLineLength();

	int nNewOffset;
	switch (nSBCode)
	{
	case SB_LEFT:
		nNewOffset = 0;
		break;
	case SB_BOTTOM:
		nNewOffset = nMaxLineLength - nPageChars + 1;
		break;
	case SB_LINEUP:
		nNewOffset = m_nOffsetChar - 1;
		break;
	case SB_LINEDOWN:
		nNewOffset = m_nOffsetChar + 1;
		break;
	case SB_PAGEUP:
		nNewOffset = m_nOffsetChar - si.nPage + 1;
		break;
	case SB_PAGEDOWN:
		nNewOffset = m_nOffsetChar + si.nPage - 1;
		break;
	case SB_THUMBPOSITION:
	case SB_THUMBTRACK:
		nNewOffset = si.nTrackPos;
		break;
	default:
		return;
	}

	if (nNewOffset >= nMaxLineLength)
		nNewOffset = nMaxLineLength - 1;
	if (nNewOffset < 0)
		nNewOffset = 0;
	ScrollToChar(nNewOffset, TRUE);
}

void CBaseView::ScrollToChar(int nNewOffsetChar, BOOL bTrackScrollBar /*= TRUE*/)
{
	if (m_nOffsetChar != nNewOffsetChar)
	{
		int nScrollChars = m_nOffsetChar - nNewOffsetChar;
		m_nOffsetChar = nNewOffsetChar;
		CRect rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.left += GetMarginWidth();
		rcScroll.top += GetLineHeight()+HEADERHEIGHT;
		ScrollWindow(nScrollChars * GetCharWidth(), 0, &rcScroll, &rcScroll);
		// update the view header
		rcScroll.left = 0;
		rcScroll.top = 0;
		rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
		InvalidateRect(&rcScroll, FALSE);
		UpdateWindow();
		if (bTrackScrollBar)
			RecalcHorzScrollBar(TRUE);
		UpdateCaret();
	}
}

void CBaseView::ScrollSide(int delta)
{
	int nNewOffset = m_nOffsetChar;
	nNewOffset += delta;
	int nMaxLineLength = GetMaxLineLength();
	if (nNewOffset >= nMaxLineLength)
		nNewOffset = nMaxLineLength - 1;
	if (nNewOffset < 0)
		nNewOffset = 0;
	ScrollToChar(nNewOffset, TRUE);
	if (m_pwndLineDiffBar)
		m_pwndLineDiffBar->Invalidate();
	UpdateCaret();
}

void CBaseView::ScrollToLine(int nNewTopLine, BOOL bTrackScrollBar /*= TRUE*/)
{
	if (m_nTopLine != nNewTopLine)
	{
		if (nNewTopLine < 0)
			nNewTopLine = 0;
		int nScrollLines = m_nTopLine - nNewTopLine;
		m_nTopLine = nNewTopLine;
		CRect rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.top += GetLineHeight()+HEADERHEIGHT;
		ScrollWindow(0, nScrollLines * GetLineHeight(), &rcScroll, &rcScroll);
		UpdateWindow();
		if (bTrackScrollBar)
			RecalcVertScrollBar(TRUE);
		UpdateCaret();
	}
}


void CBaseView::DrawMargin(CDC *pdc, const CRect &rect, int nLineIndex)
{
	pdc->FillSolidRect(rect, ::GetSysColor(COLOR_SCROLLBAR));

	if ((nLineIndex >= 0)&&(m_pViewData)&&(m_pViewData->GetCount()))
	{
		DiffStates state = m_pViewData->GetState(nLineIndex);
		HICON icon = NULL;
		switch (state)
		{
		case DIFFSTATE_ADDED:
		case DIFFSTATE_THEIRSADDED:
		case DIFFSTATE_YOURSADDED:
		case DIFFSTATE_IDENTICALADDED:
		case DIFFSTATE_CONFLICTADDED:
			icon = m_hAddedIcon;
			break;
		case DIFFSTATE_REMOVED:
		case DIFFSTATE_THEIRSREMOVED:
		case DIFFSTATE_YOURSREMOVED:
		case DIFFSTATE_IDENTICALREMOVED:
			icon = m_hRemovedIcon;
			break;
		case DIFFSTATE_CONFLICTED:
			icon = m_hConflictedIcon;
			break;
		case DIFFSTATE_CONFLICTED_IGNORED:
			icon = m_hConflictedIgnoredIcon;
			break;
		case DIFFSTATE_EDITED:
			icon = m_hEditedIcon;
			break;
		default:
			break;
		}
		bool bIdentical = false;
		if ((state != DIFFSTATE_EDITED)&&(IsBlockWhitespaceOnly(nLineIndex, bIdentical)))
		{
			if (bIdentical)
				icon = m_hEqualIcon;
			else
				icon = m_hWhitespaceBlockIcon;
		}

		if (icon)
		{
			::DrawIconEx(pdc->m_hDC, rect.left + 2, rect.top + (rect.Height()-16)/2, icon, 16, 16, NULL, NULL, DI_NORMAL);
		}
		if ((m_bViewLinenumbers)&&(m_nDigits))
		{
			int nLineNumber = GetLineNumber(nLineIndex);
			if (nLineNumber >= 0)
			{
				CString sLinenumberFormat;
				CString sLinenumber;
				sLinenumberFormat.Format(_T("%%%dd"), m_nDigits);
				sLinenumber.Format(sLinenumberFormat, nLineNumber+1);
				pdc->SetBkColor(::GetSysColor(COLOR_SCROLLBAR));
				pdc->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

				pdc->SelectObject(GetFont());
				pdc->ExtTextOut(rect.left + 18, rect.top, ETO_CLIPPED, &rect, sLinenumber, NULL);
			}
		}
	}
}

int CBaseView::GetMarginWidth()
{
	if ((m_bViewLinenumbers)&&(m_pViewData)&&(m_pViewData->GetCount()))
	{
		int nWidth = GetCharWidth();
		if (m_nDigits <= 0)
		{
			int nLength = (int)m_pViewData->GetCount();
			// find out how many digits are needed to show the highest line number
			int nDigits = 1;
			while (nLength / 10)
			{
				nDigits++;
				nLength /= 10;
			}
			m_nDigits = nDigits;
		}
		return (MARGINWIDTH + (m_nDigits * nWidth) + 2);
	}
	return MARGINWIDTH;
}

void CBaseView::DrawHeader(CDC *pdc, const CRect &rect)
{
	CRect textrect(rect.left, rect.top, rect.Width(), GetLineHeight()+HEADERHEIGHT);
	COLORREF crBk, crFg;
	CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, crBk, crFg);
	crBk = ::GetSysColor(COLOR_SCROLLBAR);
	if (IsBottomViewGood())
	{
		pdc->SetBkColor(crBk);
	}
	else
	{

		if (this == m_pwndRight)
		{
			CDiffColors::GetInstance().GetColors(DIFFSTATE_ADDED, crBk, crFg);
			pdc->SetBkColor(crBk);
		}
		else
		{
			CDiffColors::GetInstance().GetColors(DIFFSTATE_REMOVED, crBk, crFg);
			pdc->SetBkColor(crBk);
		}
	}
	pdc->FillSolidRect(textrect, crBk);

	pdc->SetTextColor(crFg);

	pdc->SelectObject(GetFont(FALSE, TRUE, FALSE));
	if (IsModified())
	{
		if (m_sWindowName.Left(2).Compare(_T("* "))!=0)
			m_sWindowName = _T("* ") + m_sWindowName;
	}
	else
	{
		if (m_sWindowName.Left(2).Compare(_T("* "))==0)
			m_sWindowName = m_sWindowName.Mid(2);
	}
	CString sViewTitle = m_sWindowName;
	int nStringLength = (GetCharWidth()*m_sWindowName.GetLength());
	if (nStringLength > rect.Width())
	{
		int offset = min(m_nOffsetChar, (nStringLength-rect.Width())/GetCharWidth()+1);

		sViewTitle = m_sWindowName.Mid(offset);
	}
	pdc->ExtTextOut(max(rect.left + (rect.Width()-nStringLength)/2, 1), 
		rect.top+(HEADERHEIGHT/2), ETO_CLIPPED, textrect, sViewTitle, NULL);
	if (this->GetFocus() == this)
		pdc->DrawEdge(textrect, EDGE_BUMP, BF_RECT);
	else
		pdc->DrawEdge(textrect, EDGE_ETCHED, BF_RECT);
}

void CBaseView::OnDraw(CDC * pDC)
{
	CRect rcClient;
	GetClientRect(rcClient);
	
	int nLineCount = GetLineCount();
	int nLineHeight = GetLineHeight();

	CDC cacheDC;
	VERIFY(cacheDC.CreateCompatibleDC(pDC));
	if (m_pCacheBitmap == NULL)
	{
		m_pCacheBitmap = new CBitmap;
		VERIFY(m_pCacheBitmap->CreateCompatibleBitmap(pDC, rcClient.Width(), nLineHeight));
	}
	CBitmap *pOldBitmap = cacheDC.SelectObject(m_pCacheBitmap);

	DrawHeader(pDC, rcClient);
	
	CRect rcLine;
	rcLine = rcClient;
	rcLine.top += nLineHeight+HEADERHEIGHT;
	rcLine.bottom = rcLine.top + nLineHeight;
	CRect rcCacheMargin(0, 0, GetMarginWidth(), nLineHeight);
	CRect rcCacheLine(GetMarginWidth(), 0, rcLine.Width(), nLineHeight);

	int nCurrentLine = m_nTopLine;
	while (rcLine.top < rcClient.bottom)
	{
		if (nCurrentLine < nLineCount)
		{
			DrawMargin(&cacheDC, rcCacheMargin, nCurrentLine);
			DrawSingleLine(&cacheDC, rcCacheLine, nCurrentLine);
		}
		else
		{
			DrawMargin(&cacheDC, rcCacheMargin, -1);
			DrawSingleLine(&cacheDC, rcCacheLine, -1);
		}

		VERIFY(pDC->BitBlt(rcLine.left, rcLine.top, rcLine.Width(), rcLine.Height(), &cacheDC, 0, 0, SRCCOPY));

		nCurrentLine ++;
		rcLine.OffsetRect(0, nLineHeight);
	}

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

BOOL CBaseView::IsLineRemoved(int nLineIndex)
{
	DiffStates state = DIFFSTATE_UNKNOWN;
	if (m_pViewData)
		state = m_pViewData->GetState(nLineIndex);
	BOOL ret = FALSE;
	switch (state)
	{
	case DIFFSTATE_REMOVED:
	case DIFFSTATE_THEIRSREMOVED:
	case DIFFSTATE_YOURSREMOVED:
	case DIFFSTATE_IDENTICALREMOVED:
		ret = TRUE;
		break;
	default:
		ret = FALSE;
		break;
	}
	return ret;
}

bool CBaseView::IsLineConflicted(int nLineIndex)
{
	DiffStates state = DIFFSTATE_UNKNOWN;
	if (m_pViewData)
		state = m_pViewData->GetState(nLineIndex);
	bool ret = false;
	switch (state)
	{
	case DIFFSTATE_CONFLICTED:
	case DIFFSTATE_CONFLICTED_IGNORED:
	case DIFFSTATE_CONFLICTEMPTY:
	case DIFFSTATE_CONFLICTADDED:
		ret = true;
		break;
	default:
		ret = false;
		break;
	}
	return ret;
}

COLORREF CBaseView::IntenseColor(long scale, COLORREF col)
{
	// if the color is already dark (gray scale below 127),
	// then lighten the color by 'scale', otherwise darken it
	int Gray  = (((int)GetRValue(col)) + GetGValue(col) + GetBValue(col))/3;
	if (Gray > 127)
	{
		long red   = MulDiv(GetRValue(col),(255-scale),255);
		long green = MulDiv(GetGValue(col),(255-scale),255);
		long blue  = MulDiv(GetBValue(col),(255-scale),255);

		return RGB(red, green, blue);
	}
	long R = MulDiv(255-GetRValue(col),scale,255)+GetRValue(col);
	long G = MulDiv(255-GetGValue(col),scale,255)+GetGValue(col);
	long B = MulDiv(255-GetBValue(col),scale,255)+GetBValue(col);

	return RGB(R, G, B);
}

COLORREF CBaseView::InlineDiffColor(int nLineIndex)
{
	return IsLineRemoved(nLineIndex) ? m_InlineRemovedBk : m_InlineAddedBk;
}

void CBaseView::DrawLineEnding(CDC *pDC, const CRect &rc, int nLineIndex, const CPoint& origin)
{
	if (!(m_bViewWhitespace && m_pViewData && (nLineIndex >= 0) && (nLineIndex < m_pViewData->GetCount())))
		return;

	EOL ending = m_pViewData->GetLineEnding(nLineIndex);
	if (m_bIconLFs)
	{
		HICON hEndingIcon = NULL;
		switch (ending)
		{
		case EOL_CR:	hEndingIcon = m_hLineEndingCR;		break;
		case EOL_CRLF:	hEndingIcon = m_hLineEndingCRLF;	break;
		case EOL_LF:	hEndingIcon = m_hLineEndingLF;		break;
		default: return;
		}
		if (origin.x < (rc.left-GetCharWidth()))
			return;
		// If EOL style has changed, color end-of-line markers as inline differences.
		if(
			m_bShowInlineDiff && m_pOtherViewData &&
			(nLineIndex < m_pOtherViewData->GetCount()) &&
			(ending != EOL_NOENDING) &&
			(ending != m_pOtherViewData->GetLineEnding(nLineIndex) &&
			(m_pOtherViewData->GetLineEnding(nLineIndex) != EOL_NOENDING))
			)
		{
			pDC->FillSolidRect(origin.x, origin.y, rc.Height(), rc.Height(), InlineDiffColor(nLineIndex));
		}

		DrawIconEx(pDC->GetSafeHdc(), origin.x, origin.y, hEndingIcon, rc.Height(), rc.Height(), NULL, NULL, DI_NORMAL);
	}
	else
	{
		CPen pen(PS_SOLID, 0, m_WhiteSpaceFg);
		CPen * oldpen = pDC->SelectObject(&pen);
		int yMiddle = origin.y + rc.Height()/2;
		int xMiddle = origin.x+GetCharWidth()/2;
		switch (ending)
		{
		case EOL_CR:
			// arrow from right to left
			pDC->MoveTo(origin.x+GetCharWidth(), yMiddle);
			pDC->LineTo(origin.x, yMiddle);
			pDC->LineTo(origin.x+4, yMiddle+4);
			pDC->MoveTo(origin.x, yMiddle);
			pDC->LineTo(origin.x+4, yMiddle-4);
			break;
		case EOL_CRLF:
			// arrow from top to middle+2, then left
			pDC->MoveTo(origin.x+GetCharWidth(), rc.top);
			pDC->LineTo(origin.x+GetCharWidth(), yMiddle);
			pDC->LineTo(origin.x, yMiddle);
			pDC->LineTo(origin.x+4, yMiddle+4);
			pDC->MoveTo(origin.x, yMiddle);
			pDC->LineTo(origin.x+4, yMiddle-4);
			break;
		case EOL_LF:
			// arrow from top to bottom
			pDC->MoveTo(xMiddle, rc.top);
			pDC->LineTo(xMiddle, rc.bottom-1);
			pDC->LineTo(xMiddle+4, rc.bottom-5);
			pDC->MoveTo(xMiddle, rc.bottom-1);
			pDC->LineTo(xMiddle-4, rc.bottom-5);
			break;
		}
		pDC->SelectObject(oldpen);
	}	
}

void CBaseView::DrawBlockLine(CDC *pDC, const CRect &rc, int nLineIndex)
{
	const int THICKNESS = 2;
	COLORREF rectcol = GetSysColor(m_bFocused ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT);
	if ((nLineIndex == m_nSelBlockStart) && m_bShowSelection)
	{
		pDC->FillSolidRect(rc.left, rc.top, rc.Width(), THICKNESS, rectcol);
	}		
	if ((nLineIndex == m_nSelBlockEnd) && m_bShowSelection)
	{
		pDC->FillSolidRect(rc.left, rc.bottom - THICKNESS, rc.Width(), THICKNESS, rectcol);
	}
}

void CBaseView::DrawText(
	CDC * pDC, const CRect &rc, LPCTSTR text, int textlength, int nLineIndex, POINT coords, bool bModified, bool bInlineDiff)
{
	ASSERT(m_pViewData && (nLineIndex < m_pViewData->GetCount()));
	DiffStates diffState = m_pViewData->GetState(nLineIndex);
	
	// first suppose the whole line is selected
	int selectedStart = 0, selectedEnd = textlength;
	
	if ((m_ptSelectionStartPos.y > nLineIndex) || (m_ptSelectionEndPos.y < nLineIndex)
		|| ! m_bShowSelection)
	{
		// this line has no selected text
		selectedStart = textlength;
	}
	else if ((m_ptSelectionStartPos.y == nLineIndex) || (m_ptSelectionEndPos.y == nLineIndex))
	{
		// the line is partially selected
		int xoffs = m_nOffsetChar + (coords.x - GetMarginWidth()) / GetCharWidth();
		if (m_ptSelectionStartPos.y == nLineIndex)
		{
			// the first line of selection
			int nSelectionStartOffset = CalculateActualOffset(m_ptSelectionStartPos.y, m_ptSelectionStartPos.x);
			selectedStart = max(min(nSelectionStartOffset - xoffs, textlength), 0);
		}

		if (m_ptSelectionEndPos.y == nLineIndex)
		{
			// the last line of selection
			int nSelectionEndOffset = CalculateActualOffset(m_ptSelectionEndPos.y, m_ptSelectionEndPos.x);
			selectedEnd = max(min(nSelectionEndOffset - xoffs, textlength), 0);
		}
	}

	COLORREF crBkgnd, crText;
	CDiffColors::GetInstance().GetColors(diffState, crBkgnd, crText);
	if (bModified || (diffState == DIFFSTATE_EDITED))
		crBkgnd = m_ModifiedBk;
	if (bInlineDiff)
		crBkgnd = InlineDiffColor(nLineIndex);

	pDC->SetBkColor(crBkgnd);
	pDC->SetTextColor(crText);
	if (selectedStart>=0)
		VERIFY(pDC->ExtTextOut(coords.x, coords.y, ETO_CLIPPED, &rc, text, selectedStart, NULL));

	long intenseColorScale = m_bFocused ? 70 : 30;
	pDC->SetBkColor(IntenseColor(intenseColorScale, crBkgnd));
	pDC->SetTextColor(IntenseColor(intenseColorScale, crText));
	VERIFY(pDC->ExtTextOut(
		coords.x + selectedStart * GetCharWidth(), coords.y, ETO_CLIPPED, &rc,
		text + selectedStart, selectedEnd - selectedStart, NULL));

	pDC->SetBkColor(crBkgnd);
	pDC->SetTextColor(crText);
	if (textlength - selectedEnd >= 0)
		VERIFY(pDC->ExtTextOut(
					coords.x + selectedEnd * GetCharWidth(), coords.y, ETO_CLIPPED, &rc,
					text + selectedEnd, textlength - selectedEnd, NULL));
}

bool CBaseView::DrawInlineDiff(CDC *pDC, const CRect &rc, int nLineIndex, const CString &line, CPoint &origin)
{
	if (!m_bShowInlineDiff || line.IsEmpty())
		return false;
	if ((m_pwndBottom != NULL) && !(m_pwndBottom->IsHidden()))
		return false;

	LPCTSTR pszDiffChars = NULL;
	int nDiffLength = 0;
	if (m_pOtherViewData)
	{
		int index = min(nLineIndex, m_pOtherViewData->GetCount() - 1);
		pszDiffChars = m_pOtherViewData->GetLine(index);
		nDiffLength = m_pOtherViewData->GetLine(index).GetLength();
	}

	if (!pszDiffChars || !*pszDiffChars)
		return false;

	CString diffline;
	ExpandChars(pszDiffChars, 0, nDiffLength, diffline);
	svn_diff_t * diff = NULL;
	m_svnlinediff.Diff(&diff, line, line.GetLength(), diffline, diffline.GetLength(), m_bInlineWordDiff);
	if (!diff || !SVNLineDiff::ShowInlineDiff(diff))
		return false;

	int lineoffset = 0;
	std::deque<int> removedPositions;
	while (diff)
	{
		apr_off_t len = diff->original_length;

		CString s;
		for (int i = 0; i < len; ++i)
		{
			s += m_svnlinediff.m_line1tokens[lineoffset].c_str();
			lineoffset++;
		}
		bool isModified = diff->type == svn_diff__type_diff_modified;
		DrawText(pDC, rc, (LPCTSTR)s, s.GetLength(), nLineIndex, origin, true, isModified);
		origin.x += pDC->GetTextExtent(s).cx;

		if (isModified && (len < diff->modified_length))
			removedPositions.push_back(origin.x - 1);

		diff = diff->next;
	}
	// Draw vertical bars at removed chunks' positions.
	for (std::deque<int>::iterator it = removedPositions.begin(); it != removedPositions.end(); ++it)
		pDC->FillSolidRect(*it, rc.top, 1, rc.Height(), m_InlineRemovedBk);
	return true;
}

void CBaseView::DrawSingleLine(CDC *pDC, const CRect &rc, int nLineIndex)
{
	if (nLineIndex >= GetLineCount())
		nLineIndex = -1;
	ASSERT(nLineIndex >= -1);

	if ((nLineIndex == -1) || !m_pViewData)
	{
		// Draw line beyond the text
		COLORREF crBkgnd, crText;
		CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, crBkgnd, crText);
		pDC->FillSolidRect(rc, crBkgnd);
		return;
	}

	DiffStates diffState = m_pViewData->GetState(nLineIndex);
	COLORREF crBkgnd, crText;
	CDiffColors::GetInstance().GetColors(diffState, crBkgnd, crText);

	if (diffState == DIFFSTATE_CONFLICTED)
	{
		// conflicted lines are shown without 'text' on them
		CRect rect = rc;
		pDC->FillSolidRect(rc, crBkgnd);
		// now draw some faint text patterns
		pDC->SetTextColor(IntenseColor(130, crBkgnd));
		pDC->DrawText(m_sConflictedText, rect, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE);
		DrawBlockLine(pDC, rc, nLineIndex);
		return;
	}

	CPoint origin(rc.left - m_nOffsetChar * GetCharWidth(), rc.top);
	int nLength = GetLineLength(nLineIndex);
	if (nLength == 0)
	{
		// Draw the empty line
		pDC->FillSolidRect(rc, crBkgnd);
		DrawBlockLine(pDC, rc, nLineIndex);
		DrawLineEnding(pDC, rc, nLineIndex, origin);
		return;
	}
	LPCTSTR pszChars = GetLineChars(nLineIndex);
	if (pszChars == NULL)
		return;

	CheckOtherView();

	// Draw the line

	pDC->SelectObject(GetFont(FALSE, FALSE, IsLineRemoved(nLineIndex)));
	CString line;
	ExpandChars(pszChars, 0, nLength, line);

	int nWidth = rc.right - origin.x;
	int savedx = origin.x;
	bool bInlineDiffDrawn =
		nWidth > 0 && diffState != DIFFSTATE_NORMAL &&
		DrawInlineDiff(pDC, rc, nLineIndex, line, origin);

	if (!bInlineDiffDrawn)
	{
		int nCount = min(line.GetLength(), nWidth / GetCharWidth() + 1);
		DrawText(pDC, rc, line, nCount, nLineIndex, origin, false, false);
	}

	origin.x = savedx + pDC->GetTextExtent(line).cx;

	// draw white space after the end of line
	CRect frect = rc;
	if (origin.x > frect.left)
		frect.left = origin.x;
	if (bInlineDiffDrawn)
		CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, crBkgnd, crText);
	if (frect.right > frect.left)
		pDC->FillSolidRect(frect, crBkgnd);
	// draw the whitespace chars
	if (m_bViewWhitespace)
	{
		int xpos = 0;
		int y = rc.top + (rc.bottom-rc.top)/2;

		int nActualOffset = 0;
		while ((nActualOffset < m_nOffsetChar) && (*pszChars))
		{
			if (*pszChars == _T('\t'))
				nActualOffset += (GetTabSize() - nActualOffset % GetTabSize());
			else
				nActualOffset++;
			pszChars++;
		}
		if (nActualOffset > m_nOffsetChar)
			pszChars--;

		CPen pen(PS_SOLID, 0, m_WhiteSpaceFg);
		CPen pen2(PS_SOLID, 2, m_WhiteSpaceFg);
		while (*pszChars)
		{
			switch (*pszChars)
			{
			case _T('\t'):
				{
					// draw an arrow
					CPen * oldPen = pDC->SelectObject(&pen);
					int nSpaces = GetTabSize() - (m_nOffsetChar + xpos) % GetTabSize();
					pDC->MoveTo(xpos * GetCharWidth() + rc.left, y);
					pDC->LineTo((xpos + nSpaces) * GetCharWidth() + rc.left-2, y);
					pDC->LineTo((xpos + nSpaces) * GetCharWidth() + rc.left-6, y-4);
					pDC->MoveTo((xpos + nSpaces) * GetCharWidth() + rc.left-2, y);
					pDC->LineTo((xpos + nSpaces) * GetCharWidth() + rc.left-6, y+4);
					xpos += nSpaces;
					pDC->SelectObject(oldPen);
				}
				break;
			case _T(' '):
				{
					// draw a small dot
					CPen * oldPen = pDC->SelectObject(&pen2);
					pDC->MoveTo(xpos * GetCharWidth() + rc.left + GetCharWidth()/2-1, y);
					pDC->LineTo(xpos * GetCharWidth() + rc.left + GetCharWidth()/2+1, y);
					xpos++;
					pDC->SelectObject(oldPen);
				}
				break;
			default:
				xpos++;
				break;
			}
			pszChars++;
		}
	}
	DrawBlockLine(pDC, rc, nLineIndex);
	DrawLineEnding(pDC, rc, nLineIndex, origin);
}

void CBaseView::ExpandChars(LPCTSTR pszChars, int nOffset, int nCount, CString &line)
{
	if (nCount <= 0)
	{
		line = _T("");
		return;
	}

	int nTabSize = GetTabSize();

	int nActualOffset = 0;
	for (int i=0; i<nOffset; i++)
	{
		if (pszChars[i] == _T('\t'))
			nActualOffset += (nTabSize - nActualOffset % nTabSize);
		else
			nActualOffset ++;
	}

	pszChars += nOffset;
	int nLength = nCount;

	int nTabCount = 0;
	for (int i=0; i<nLength; i++)
	{
		if (pszChars[i] == _T('\t'))
			nTabCount ++;
	}

	LPTSTR pszBuf = line.GetBuffer(nLength + nTabCount * (nTabSize - 1) + 1);
	int nCurPos = 0;
	if (nTabCount > 0 || m_bViewWhitespace)
	{
		for (int i=0; i<nLength; i++)
		{
			if (pszChars[i] == _T('\t'))
			{
				int nSpaces = nTabSize - (nActualOffset + nCurPos) % nTabSize;
				while (nSpaces > 0)
				{
					pszBuf[nCurPos ++] = _T(' ');
					nSpaces --;
				}
			}
			else
			{
				pszBuf[nCurPos] = pszChars[i];
				nCurPos ++;
			}
		}
	}
	else
	{
		memcpy(pszBuf, pszChars, sizeof(TCHAR) * nLength);
		nCurPos = nLength;
	}
	pszBuf[nCurPos] = 0;
	line.ReleaseBuffer();
}

void CBaseView::ScrollAllToLine(int nNewTopLine, BOOL bTrackScrollBar)
{
	if ((m_pwndLeft)&&(m_pwndRight))
	{
		m_pwndLeft->ScrollToLine(nNewTopLine, bTrackScrollBar);
		m_pwndRight->ScrollToLine(nNewTopLine, bTrackScrollBar);
	}
	else
	{
		if (m_pwndLeft)
			m_pwndLeft->ScrollToLine(nNewTopLine, bTrackScrollBar);
		if (m_pwndRight)
			m_pwndRight->ScrollToLine(nNewTopLine, bTrackScrollBar);
	}
	if (m_pwndBottom)
		m_pwndBottom->ScrollToLine(nNewTopLine, bTrackScrollBar);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::GoToLine(int nNewLine, BOOL bAll)
{
	//almost the same as ScrollAllToLine, but try to put the line in the
	//middle of the view, not on top
	int nNewTopLine = nNewLine - GetScreenLines()/2;
	if (nNewTopLine < 0)
		nNewTopLine = 0;
	if (m_pViewData)
	{
		if (nNewTopLine >= m_pViewData->GetCount())
			nNewTopLine = m_pViewData->GetCount()-1;
		if (bAll)
			ScrollAllToLine(nNewTopLine);
		else
			ScrollToLine(nNewTopLine);
	}
}

BOOL CBaseView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

int CBaseView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	memset(&m_lfBaseFont, 0, sizeof(m_lfBaseFont));
	//lstrcpy(m_lfBaseFont.lfFaceName, _T("Courier New"));
	//lstrcpy(m_lfBaseFont.lfFaceName, _T("FixedSys"));
	m_lfBaseFont.lfHeight = 0;
	m_lfBaseFont.lfWeight = FW_NORMAL;
	m_lfBaseFont.lfItalic = FALSE;
	m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
	m_lfBaseFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	m_lfBaseFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_lfBaseFont.lfQuality = DEFAULT_QUALITY;
	m_lfBaseFont.lfPitchAndFamily = DEFAULT_PITCH;

	return 0;
}

void CBaseView::OnDestroy()
{
	CView::OnDestroy();
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
			m_apFonts[i] = NULL;
		}
	}
	if (m_pCacheBitmap != NULL)
	{
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
}

void CBaseView::OnSize(UINT nType, int cx, int cy)
{
	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
	// make sure the view header is redrawn
	CRect rcScroll;
	GetClientRect(&rcScroll);
	rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
	InvalidateRect(&rcScroll, FALSE);

	m_nScreenLines = -1;
	m_nScreenChars = -1;
	RecalcVertScrollBar();
	RecalcHorzScrollBar();
	CView::OnSize(nType, cx, cy);
}

BOOL CBaseView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_pwndLeft)
		m_pwndLeft->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndRight)
		m_pwndRight->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndBottom)
		m_pwndBottom->OnDoMouseWheel(nFlags, zDelta, pt);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

void CBaseView::OnDoMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
	if (GetKeyState(VK_CONTROL)&0x8000)
	{
		// Ctrl-Wheel scrolls sideways
		ScrollSide(-zDelta/30);
	}
	else
	{
		int nLineCount = GetLineCount();
		int nTopLine = m_nTopLine;
		nTopLine -= (zDelta/30);
		if (nTopLine < 0)
			nTopLine = 0;
		if (nTopLine >= nLineCount)
			nTopLine = nLineCount - 1;
		ScrollToLine(nTopLine, TRUE);
	}
}

BOOL CBaseView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (nHitTest == HTCLIENT)
	{
		::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));	// Set To Arrow Cursor
		return TRUE;
	}
	return CView::OnSetCursor(pWnd, nHitTest, message);
}

void CBaseView::OnKillFocus(CWnd* pNewWnd)
{
	CView::OnKillFocus(pNewWnd);
	m_bFocused = FALSE;
	UpdateCaret();
	Invalidate();
}

void CBaseView::OnSetFocus(CWnd* pOldWnd)
{
	CView::OnSetFocus(pOldWnd);
	m_bFocused = TRUE;
	UpdateCaret();
	Invalidate();
}

int CBaseView::GetLineFromPoint(CPoint point)
{
	ScreenToClient(&point);
	return (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
}

bool CBaseView::OnContextMenu(CPoint /*point*/, int /*nLine*/, DiffStates /*state*/)
{
	return false;
}

void CBaseView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int nLine = GetLineFromPoint(point);

	if (!m_pViewData)
		return;
	if (m_nSelBlockEnd >= GetLineCount())
		m_nSelBlockEnd = GetLineCount()-1;
	if ((nLine <= m_pViewData->GetCount())&&(nLine > m_nTopLine))
	{
		int nIndex = nLine - 1;
		DiffStates state = m_pViewData->GetState(nIndex);
		if ((state != DIFFSTATE_NORMAL) && (state != DIFFSTATE_UNKNOWN))
		{
			// if there's nothing selected, or if the selection is outside the window then
			// select the diff block under the cursor.
			if (((m_nSelBlockStart<0)&&(m_nSelBlockEnd<0))||
				((m_nSelBlockEnd < m_nTopLine)||(m_nSelBlockStart > m_nTopLine+m_nScreenLines)))
			{
				while (nIndex >= 0)
				{
					if (nIndex == 0)
					{
						nIndex--;
						break;
					}
					if (state != m_pViewData->GetState(--nIndex))
						break;
				}
				m_nSelBlockStart = nIndex+1;
				while (nIndex < (m_pViewData->GetCount()-1))
				{
					if (state != m_pViewData->GetState(++nIndex))
						break;
				}
				if ((nIndex == (m_pViewData->GetCount()-1))&&(state == m_pViewData->GetState(nIndex)))
					m_nSelBlockEnd = nIndex;
				else
					m_nSelBlockEnd = nIndex-1;
				SetupSelection(m_nSelBlockStart, m_nSelBlockEnd);
				m_ptCaretPos.x = 0;
				m_ptCaretPos.y = nLine - 1;
				UpdateCaret();
			}
		}
		if (((state == DIFFSTATE_NORMAL)||(state == DIFFSTATE_UNKNOWN)) &&
			(m_nSelBlockStart >= 0)&&(m_nSelBlockEnd >= 0))
		{
			// find a more 'relevant' state in the selection
			for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; ++i)
			{
				state = m_pViewData->GetState(i);
				if ((state != DIFFSTATE_NORMAL) && (state != DIFFSTATE_UNKNOWN))
					break;
			}
		}
		bool bKeepSelection = OnContextMenu(point, nLine, state);
		if (! bKeepSelection)
			ClearSelection();
		RefreshViews();
	}
}

void CBaseView::RefreshViews()
{
	if (m_pwndLeft)
	{
		m_pwndLeft->UpdateStatusBar();
		m_pwndLeft->Invalidate();
	}
	if (m_pwndRight)
	{
		m_pwndRight->UpdateStatusBar();
		m_pwndRight->Invalidate();
	}
	if (m_pwndBottom)
	{
		m_pwndBottom->UpdateStatusBar();
		m_pwndBottom->Invalidate();
	}
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::GoToFirstDifference()
{
	m_ptCaretPos.y = 0;
	SelectNextBlock(1, false, false);
}

void CBaseView::HiglightLines(int start, int end /* = -1 */)
{
	ClearSelection();
	m_nSelBlockStart = start;
	if (end < 0)
		end = start;
	m_nSelBlockEnd = end;
	m_ptCaretPos.x = 0;
	m_ptCaretPos.y = start;
	UpdateCaret();
	Invalidate();
}

void CBaseView::SetupSelection(int start, int end)
{
	if (IsBottomViewGood())
	{
		m_pwndBottom->m_nSelBlockStart = start;
		m_pwndBottom->m_nSelBlockEnd = end;
		m_pwndBottom->Invalidate();
	}
	if (IsLeftViewGood())
	{
		m_pwndLeft->m_nSelBlockStart = start;
		m_pwndLeft->m_nSelBlockEnd = end;
		m_pwndLeft->Invalidate();
	}
	if (IsRightViewGood())
	{
		m_pwndRight->m_nSelBlockStart = start;
		m_pwndRight->m_nSelBlockEnd = end;
		m_pwndRight->Invalidate();
	}
}

void CBaseView::OnMergePreviousconflict()
{
	SelectNextBlock(-1, true);
}

void CBaseView::OnMergeNextconflict()
{
	SelectNextBlock(1, true);
}

void CBaseView::OnMergeNextdifference()
{
	SelectNextBlock(1, false);
}

void CBaseView::OnMergePreviousdifference()
{
	SelectNextBlock(-1, false);
}

void CBaseView::SelectNextBlock(int nDirection, bool bConflict, bool bSkipEndOfCurrentBlock /* = true */)
{
	if (! m_pViewData)
		return;

	if (m_pViewData->GetCount() == 0)
		return;

	int nCenterPos = m_ptCaretPos.y;
	int nLimit = 0;
	if (nDirection > 0)
		nLimit = m_pViewData->GetCount() - 1;

	if (nCenterPos >= m_pViewData->GetCount())
		nCenterPos = m_pViewData->GetCount()-1;

	if (bSkipEndOfCurrentBlock) 
	{
		// Find end of current block
		DiffStates state = m_pViewData->GetState(nCenterPos);
		while ((nCenterPos != nLimit) && 
		       (m_pViewData->GetState(nCenterPos)==state))
			nCenterPos += nDirection;
	}

	// Find next diff/conflict block
	while (nCenterPos != nLimit)
	{
		DiffStates linestate = m_pViewData->GetState(nCenterPos);
		if (!bConflict &&
			(linestate != DIFFSTATE_NORMAL) &&
			(linestate != DIFFSTATE_UNKNOWN))
			break;
		if (bConflict &&
			((linestate == DIFFSTATE_CONFLICTADDED) ||
			 (linestate == DIFFSTATE_CONFLICTED_IGNORED) ||
			 (linestate == DIFFSTATE_CONFLICTED) ||
			 (linestate == DIFFSTATE_CONFLICTEMPTY)))
			break;

		nCenterPos += nDirection;
	}

	// Find end of new block
	DiffStates state = m_pViewData->GetState(nCenterPos);
	int nBlockEnd = nCenterPos;
	while ((nBlockEnd != nLimit) &&  
			 (state == m_pViewData->GetState(nBlockEnd + nDirection)))
		nBlockEnd += nDirection;

	int nTopPos = nCenterPos - (GetScreenLines()/2);
	if (nTopPos < 0)
		nTopPos = 0;

	m_ptCaretPos.x = 0;
	m_ptCaretPos.y = nCenterPos;
	ClearSelection();
	if (nDirection > 0)
		SetupSelection(nCenterPos, nBlockEnd);
	else
		SetupSelection(nBlockEnd, nCenterPos);

	ScrollAllToLine(nTopPos, FALSE);
	RecalcAllVertScrollBars(TRUE);
	m_nCaretGoalPos = 0;
	UpdateCaret();
	ShowDiffLines(nCenterPos);
}

BOOL CBaseView::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	// need to handle both ANSI and UNICODE versions of the message
	TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
	TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
	CString strTipText;
	UINT nID = (UINT)pNMHDR->idFrom;
	if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
		pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
	{
		// idFrom is actually the HWND of the tool
		nID = ::GetDlgCtrlID((HWND)nID);
	}

	if (pNMHDR->idFrom == (UINT)m_hWnd)
	{
		if (m_sWindowName.Left(2).Compare(_T("* "))==0)
		{
			strTipText = m_sWindowName.Mid(2) + _T("\r\n") + m_sFullFilePath;
		}
		else
		{
			strTipText = m_sWindowName + _T("\r\n") + m_sFullFilePath;
		}
	}
	else
		return FALSE;
	
	*pResult = 0;
	if (strTipText.IsEmpty())
		return TRUE;

	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
	}
	else
	{
		lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
		pTTTW->lpszText = m_wszTip;
	}

	return TRUE;    // message was handled
}


INT_PTR CBaseView::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	CRect rcClient;
	GetClientRect(rcClient);
	CRect textrect(rcClient.left, rcClient.top, rcClient.Width(), m_nLineHeight+HEADERHEIGHT);
	if (textrect.PtInRect(point))
	{
		// inside the header part of the view (showing the filename)
		pTI->hwnd = this->m_hWnd;
		this->GetClientRect(&pTI->rect);
		pTI->uFlags  |= TTF_ALWAYSTIP | TTF_IDISHWND;
		pTI->uId = (UINT)m_hWnd;
		pTI->lpszText = LPSTR_TEXTCALLBACK;

		// we want multi line tooltips
		CToolTipCtrl* pToolTip = AfxGetModuleThreadState()->m_pToolTip;
		if (pToolTip->GetSafeHwnd() != NULL)
		{
			pToolTip->SetMaxTipWidth(INT_MAX);
		}

		return 1;
	}
	return -1;
}

void CBaseView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
	bool bShift = !!(GetKeyState(VK_SHIFT)&0x8000);
	switch (nChar)
	{
	case VK_PRIOR:
		{
			m_ptCaretPos.y -= GetScreenLines();
			m_ptCaretPos.y = max(m_ptCaretPos.y, 0);
			m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
			if (bShift)
				AdjustSelection();
			else
				ClearSelection();
			UpdateCaret();
			EnsureCaretVisible();
			ShowDiffLines(m_ptCaretPos.y);
		}
		break;
	case VK_NEXT:
		{
			m_ptCaretPos.y += GetScreenLines();
			if (m_ptCaretPos.y >= GetLineCount())
				m_ptCaretPos.y = GetLineCount()-1;
			m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
			if (bShift)
				AdjustSelection();
			else
				ClearSelection();
			UpdateCaret();
			EnsureCaretVisible();
			ShowDiffLines(m_ptCaretPos.y);
		}
		break;
	case VK_HOME:
		{
			if (bControl)
			{
				ScrollAllToLine(0);
				m_ptCaretPos.x = 0;
				m_ptCaretPos.y = 0;
				m_nCaretGoalPos = 0;
				if (bShift)
					AdjustSelection();
				else
					ClearSelection();
				UpdateCaret();
			}
			else
			{
				m_ptCaretPos.x = 0;
				m_nCaretGoalPos = 0;
				if (bShift)
					AdjustSelection();
				else
					ClearSelection();
				EnsureCaretVisible();
				UpdateCaret();
			}
		}
		break;
	case VK_END:
		{
			if (bControl)
			{
				ScrollAllToLine(GetLineCount()-GetAllMinScreenLines());
				m_ptCaretPos.y = GetLineCount()-1;
				m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
				UpdateGoalPos();
				if (bShift)
					AdjustSelection();
				else
					ClearSelection();
				UpdateCaret();
			}
			else
			{
				m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
				UpdateGoalPos();
				if (bShift)
					AdjustSelection();
				else
					ClearSelection();
				EnsureCaretVisible();
				UpdateCaret();
			}
		}
		break;
	case VK_BACK:
		{
			if (m_bCaretHidden)
				break;

			if (! HasTextSelection()) {
				if (m_ptCaretPos.y == 0 && m_ptCaretPos.x == 0)
					break;
				m_ptSelectionEndPos = m_ptCaretPos;
				MoveCaretLeft();
				m_ptSelectionStartPos = m_ptCaretPos;
			}
			RemoveSelectedText();
		}
		break;
	case VK_DELETE:
		{
			if (m_bCaretHidden)
				break;

			if (! HasTextSelection()) {
				if (! MoveCaretRight())
					break;
				m_ptSelectionEndPos = m_ptCaretPos;
				MoveCaretLeft();
				m_ptSelectionStartPos = m_ptCaretPos;
			}
			RemoveSelectedText();
		}
		break;
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CBaseView::OnLButtonDown(UINT nFlags, CPoint point)
{
	int nClickedLine = (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
	nClickedLine--;		//we need the index
	if ((nClickedLine >= m_nTopLine)&&(nClickedLine < GetLineCount()))
	{
		m_ptCaretPos.y = nClickedLine;
		m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
		UpdateGoalPos();

		if (nFlags & MK_SHIFT)
			AdjustSelection();
		else
		{
			ClearSelection();
			SetupSelection(m_ptCaretPos.y, m_ptCaretPos.y);
		}

		UpdateCaret();

		Invalidate();
	}

	CView::OnLButtonDown(nFlags, point);
}

void CBaseView::OnEditCopy()
{
	if ((m_ptSelectionStartPos.x == m_ptSelectionEndPos.x)&&(m_ptSelectionStartPos.y == m_ptSelectionEndPos.y))
		return;
	// first store the selected lines in one CString
	CString sCopyData;
	for (int i=m_ptSelectionStartPos.y; i<=m_ptSelectionEndPos.y; i++)
	{
		switch (m_pViewData->GetState(i))
		{
		case DIFFSTATE_EMPTY:
			break;
		case DIFFSTATE_UNKNOWN:
		case DIFFSTATE_NORMAL:
		case DIFFSTATE_REMOVED:
		case DIFFSTATE_REMOVEDWHITESPACE:
		case DIFFSTATE_ADDED:
		case DIFFSTATE_ADDEDWHITESPACE:
		case DIFFSTATE_WHITESPACE:
		case DIFFSTATE_WHITESPACE_DIFF:
		case DIFFSTATE_CONFLICTED:
		case DIFFSTATE_CONFLICTED_IGNORED:
		case DIFFSTATE_CONFLICTADDED:
		case DIFFSTATE_CONFLICTEMPTY:
		case DIFFSTATE_CONFLICTRESOLVED:
		case DIFFSTATE_IDENTICALREMOVED:
		case DIFFSTATE_IDENTICALADDED:
		case DIFFSTATE_THEIRSREMOVED:
		case DIFFSTATE_THEIRSADDED:
		case DIFFSTATE_YOURSREMOVED:
		case DIFFSTATE_YOURSADDED:
		case DIFFSTATE_EDITED:
			sCopyData += m_pViewData->GetLine(i);
			sCopyData += _T("\r\n");
			break;
		}
	}
	// remove the last \r\n
	sCopyData = sCopyData.Left(sCopyData.GetLength()-2);
	// remove the non-selected chars from the first line
	sCopyData = sCopyData.Mid(m_ptSelectionStartPos.x);
	// remove the non-selected chars from the last line
	int lastLinePos = sCopyData.ReverseFind('\n');
	lastLinePos += 1;
	if (lastLinePos == 0)
		lastLinePos -= m_ptSelectionStartPos.x;
	sCopyData = sCopyData.Left(lastLinePos+m_ptSelectionEndPos.x);
	if (!sCopyData.IsEmpty())
	{
		CStringUtils::WriteAsciiStringToClipboard(sCopyData, m_hWnd);
	}
}

void CBaseView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_pMainFrame->m_nMoveMovesToIgnore > 0) {
		--m_pMainFrame->m_nMoveMovesToIgnore;
		CView::OnMouseMove(nFlags, point);
		return;
	}

	int nMouseLine = (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
	nMouseLine--;		//we need the index
	if (nMouseLine < -1)
	{
		nMouseLine = -1;
	}
	ShowDiffLines(nMouseLine);

	KillTimer(IDT_SCROLLTIMER);
	if (nFlags & MK_LBUTTON)
	{
		int saveMouseLine = nMouseLine >= 0 ? nMouseLine : 0;
		saveMouseLine = saveMouseLine < GetLineCount() ? saveMouseLine : GetLineCount() - 1;
		int charIndex = CalculateCharIndex(saveMouseLine, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
		if (((m_nSelBlockStart >= 0)&&(m_nSelBlockEnd >= 0))&&
			((nMouseLine >= m_nTopLine)&&(nMouseLine < GetLineCount())))
		{
			m_ptCaretPos.y = nMouseLine;
			m_ptCaretPos.x = charIndex;
			UpdateGoalPos();
			AdjustSelection();
			UpdateCaret();
			Invalidate();
			UpdateWindow();
		}
		if (nMouseLine < m_nTopLine)
		{
			ScrollToLine(m_nTopLine-1, TRUE);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
		if (nMouseLine >= m_nTopLine + GetScreenLines())
		{
			ScrollToLine(m_nTopLine+1, TRUE);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
		if (charIndex <= m_nOffsetChar)
		{
			ScrollSide(-1);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
		if (charIndex >= (GetScreenChars()+m_nOffsetChar))
		{
			ScrollSide(1);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
	}

	if (!m_bMouseWithin)
	{ 
		m_bMouseWithin = TRUE;
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_LEAVE;
		tme.hwndTrack = m_hWnd;
		_TrackMouseEvent(&tme);
	}

	CView::OnMouseMove(nFlags, point);
}

void CBaseView::OnMouseLeave()
{
	ShowDiffLines(-1);
	m_bMouseWithin = FALSE;
	KillTimer(IDT_SCROLLTIMER);
	CView::OnMouseLeave();
}

void CBaseView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_SCROLLTIMER)
	{
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(&point);
		int nMouseLine = (((point.y - HEADERHEIGHT) / GetLineHeight()) + m_nTopLine);
		nMouseLine--;		//we need the index
		if (nMouseLine < -1)
		{
			nMouseLine = -1;
		}
		if (GetKeyState(VK_LBUTTON)&0x8000)
		{
			int saveMouseLine = nMouseLine >= 0 ? nMouseLine : 0;
			saveMouseLine = saveMouseLine < GetLineCount() ? saveMouseLine : GetLineCount() - 1;
			int charIndex = CalculateCharIndex(saveMouseLine, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
			if (nMouseLine < m_nTopLine)
			{
				ScrollToLine(m_nTopLine-1, TRUE);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
			if (nMouseLine >= m_nTopLine + GetScreenLines())
			{
				ScrollToLine(m_nTopLine+1, TRUE);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
			if (charIndex <= m_nOffsetChar)
			{
				ScrollSide(-1);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
			if (charIndex >= GetScreenChars())
			{
				ScrollSide(1);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
		}

	}

	CView::OnTimer(nIDEvent);
}

void CBaseView::SelectLines(int nLine1, int nLine2)
{
	if (nLine2 == -1)
		nLine2 = nLine1;
	m_nSelBlockStart = nLine1;
	m_nSelBlockEnd = nLine2;
	Invalidate();
}

void CBaseView::ShowDiffLines(int nLine)
{
	if ((nLine >= m_nTopLine)&&(nLine < GetLineCount()))
	{
		if ((m_pwndRight)&&(m_pwndRight->m_pViewData)&&(m_pwndLeft)&&(m_pwndLeft->m_pViewData)&&(!m_pMainFrame->m_bOneWay))
		{
			nLine = (nLine > m_pwndRight->m_pViewData->GetCount() ? -1 : nLine);
			nLine = (nLine > m_pwndLeft->m_pViewData->GetCount() ? -1 : nLine);

			if (nLine >= 0)
			{
				if (nLine != m_nMouseLine)
				{
					m_nMouseLine = nLine;
					if (nLine >= GetLineCount())
						nLine = -1;
					m_pwndLineDiffBar->ShowLines(nLine);
				}
			}
		}
	}
	else
	{
		m_pwndLineDiffBar->ShowLines(nLine);
	}
}

void CBaseView::UseTheirAndYourBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate)
{
	if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
		return;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
		m_pwndBottom->m_pViewData->SetLine(i, m_pwndLeft->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
		m_pwndBottom->m_pViewData->SetState(i, m_pwndLeft->m_pViewData->GetState(i));
		m_pwndBottom->m_pViewData->SetLineEnding(i, m_pwndBottom->lineendings);
		if (m_pwndBottom->IsLineConflicted(i))
		{
			if (m_pwndLeft->m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
				m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
			else
				m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
		}
		m_pwndLeft->m_pViewData->SetState(i, DIFFSTATE_YOURSADDED);
	}

	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pwndBottom->m_pViewData->InsertData(index, m_pwndRight->m_pViewData->GetData(i));
		if (m_pwndBottom->IsLineConflicted(index))
		{
			if (m_pwndRight->m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
				m_pwndBottom->m_pViewData->SetState(index, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
			else
				m_pwndBottom->m_pViewData->SetState(index, DIFFSTATE_CONFLICTRESOLVED);
		}
		m_pwndRight->m_pViewData->SetState(i, DIFFSTATE_THEIRSADDED);
		index++;
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndBottom->m_pViewData->GetLineNumber(i);
		if (oldline >= 0)
			m_pwndBottom->m_pViewData->SetLineNumber(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
	{
		rightstate.addedlines.push_back(m_nSelBlockStart);
		m_pwndRight->m_pViewData->InsertData(m_nSelBlockStart, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		m_pwndLeft->m_pViewData->InsertData(m_nSelBlockEnd+1, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		leftstate.addedlines.push_back(m_nSelBlockEnd+1);
	}
	RecalcAllVertScrollBars();
	m_pwndBottom->SetModified();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}

void CBaseView::UseYourAndTheirBlock(viewstate &rightstate, viewstate &bottomstate, viewstate &leftstate)
{
	if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
		return;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
		m_pwndBottom->m_pViewData->SetLine(i, m_pwndRight->m_pViewData->GetLine(i));
		bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
		m_pwndBottom->m_pViewData->SetState(i, m_pwndRight->m_pViewData->GetState(i));
		rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
		m_pwndBottom->m_pViewData->SetLineEnding(i, m_pwndBottom->lineendings);
		if (m_pwndBottom->IsLineConflicted(i))
		{
			if (m_pwndRight->m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
				m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
			else
				m_pwndBottom->m_pViewData->SetState(i, DIFFSTATE_CONFLICTRESOLVED);
		}
		m_pwndRight->m_pViewData->SetState(i, DIFFSTATE_YOURSADDED);
	}

	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		bottomstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pwndBottom->m_pViewData->InsertData(index, m_pwndLeft->m_pViewData->GetData(i));
		leftstate.linestates[i] = m_pwndLeft->m_pViewData->GetState(i);
		if (m_pwndBottom->IsLineConflicted(index))
		{
			if (m_pwndLeft->m_pViewData->GetState(i) == DIFFSTATE_CONFLICTEMPTY)
				m_pwndBottom->m_pViewData->SetState(index, DIFFSTATE_CONFLICTRESOLVEDEMPTY);
			else
				m_pwndBottom->m_pViewData->SetState(index, DIFFSTATE_CONFLICTRESOLVED);
		}
		m_pwndLeft->m_pViewData->SetState(i, DIFFSTATE_THEIRSADDED);
		index++;
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<m_pwndBottom->GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndBottom->m_pViewData->GetLineNumber(i);
		if (oldline >= 0)
			m_pwndBottom->m_pViewData->SetLineNumber(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in both yours and theirs
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
	{
		leftstate.addedlines.push_back(m_nSelBlockStart);
		m_pwndLeft->m_pViewData->InsertData(m_nSelBlockStart, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		m_pwndRight->m_pViewData->InsertData(m_nSelBlockEnd+1, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
	}

	RecalcAllVertScrollBars();
	m_pwndBottom->SetModified();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}

void CBaseView::UseBothRightFirst(viewstate &rightstate, viewstate &leftstate)
{
	if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
		return;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
		m_pwndRight->m_pViewData->SetState(i, DIFFSTATE_YOURSADDED);
	}

	// your block is done, now insert their block
	int index = m_nSelBlockEnd+1;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		rightstate.addedlines.push_back(m_nSelBlockEnd+1);
		m_pwndRight->m_pViewData->InsertData(index, m_pwndLeft->m_pViewData->GetData(i));
		m_pwndRight->m_pViewData->SetState(index++, DIFFSTATE_THEIRSADDED);
	}
	// adjust line numbers
	index--;
	for (int i=m_nSelBlockEnd+1; i<m_pwndRight->GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndRight->m_pViewData->GetLineNumber(i);
		if (oldline >= 0)
			m_pwndRight->m_pViewData->SetLineNumber(i, oldline+(index-m_nSelBlockEnd));
	}

	// now insert an empty block in the left view
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
	{
		leftstate.addedlines.push_back(m_nSelBlockStart);
		m_pwndLeft->m_pViewData->InsertData(m_nSelBlockStart, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
	}
	RecalcAllVertScrollBars();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}

void CBaseView::UseBothLeftFirst(viewstate &rightstate, viewstate &leftstate)
{
	if ((m_nSelBlockStart == -1)||(m_nSelBlockEnd == -1))
		return;
	// get line number from just before the block
	long linenumber = 0;
	if (m_nSelBlockStart > 0)
		linenumber = m_pwndRight->m_pViewData->GetLineNumber(m_nSelBlockStart-1);
	linenumber++;
	for (int i=m_nSelBlockStart; i<=m_nSelBlockEnd; i++)
	{
		rightstate.addedlines.push_back(m_nSelBlockStart);
		m_pwndRight->m_pViewData->InsertData(i, m_pwndLeft->m_pViewData->GetLine(i), DIFFSTATE_THEIRSADDED, linenumber++, m_pwndLeft->m_pViewData->GetLineEnding(i));
	}
	// adjust line numbers
	for (int i=m_nSelBlockEnd+1; i<m_pwndRight->GetLineCount(); ++i)
	{
		long oldline = (long)m_pwndRight->m_pViewData->GetLineNumber(i);
		if (oldline >= 0)
			m_pwndRight->m_pViewData->SetLineNumber(i, oldline+(m_nSelBlockEnd-m_nSelBlockStart)+1);
	}

	// now insert an empty block in left view
	for (int emptyblocks=0; emptyblocks < m_nSelBlockEnd-m_nSelBlockStart+1; ++emptyblocks)
	{
		leftstate.addedlines.push_back(m_nSelBlockEnd + 1);
		m_pwndLeft->m_pViewData->InsertData(m_nSelBlockEnd + 1, _T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING);
	}
	RecalcAllVertScrollBars();
	m_pwndLeft->SetModified();
	m_pwndRight->SetModified();
}

void CBaseView::UpdateCaret()
{
	if (m_ptCaretPos.y >= GetLineCount())
		m_ptCaretPos.y = GetLineCount()-1;
	if (m_ptCaretPos.y < 0)
		m_ptCaretPos.y = 0;
	if (m_ptCaretPos.x > GetLineLength(m_ptCaretPos.y))
		m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
	if (m_ptCaretPos.x < 0)
		m_ptCaretPos.x = 0;

	int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);

	if (m_bFocused && !m_bCaretHidden &&
		m_ptCaretPos.y >= m_nTopLine &&
		m_ptCaretPos.y < (m_nTopLine+GetScreenLines()) &&
		nCaretOffset >= m_nOffsetChar &&
		nCaretOffset < (m_nOffsetChar+GetScreenChars()))
	{
		CreateSolidCaret(2, GetLineHeight());
		SetCaretPos(TextToClient(m_ptCaretPos));
		ShowCaret();
	}
	else
	{
		HideCaret();
	}
}

void CBaseView::EnsureCaretVisible()
{
	int nCaretOffset = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);

	if (m_ptCaretPos.y < m_nTopLine)
		ScrollAllToLine(m_ptCaretPos.y);
	if (m_ptCaretPos.y >= (m_nTopLine+GetScreenLines()))
		ScrollAllToLine(m_ptCaretPos.y-GetScreenLines()+1);
	if (nCaretOffset < m_nOffsetChar)
		ScrollToChar(nCaretOffset);
	if (nCaretOffset > (m_nOffsetChar+GetScreenChars()-1))
		ScrollToChar(nCaretOffset-GetScreenChars()+1);
}

int CBaseView::CalculateActualOffset(int nLineIndex, int nCharIndex) const
{
	int nLength = GetLineLength(nLineIndex);
	ASSERT(nCharIndex >= 0);
	if (nCharIndex > nLength)
		nCharIndex = nLength;
	LPCTSTR pszChars = GetLineChars(nLineIndex);
	int nOffset = 0;
	int nTabSize = GetTabSize();
	for (int I = 0; I < nCharIndex; I ++)
	{
		if (pszChars[I] == _T('\t'))
			nOffset += (nTabSize - nOffset % nTabSize);
		else
			nOffset++;
	}
	return nOffset;
}

int	CBaseView::CalculateCharIndex(int nLineIndex, int nActualOffset) const
{
	int nLength = GetLineLength(nLineIndex);
	LPCTSTR pszLine = GetLineChars(nLineIndex);
	int nIndex = 0;
	int nOffset = 0;
	int nTabSize = GetTabSize();
	while (nOffset < nActualOffset && nIndex < nLength)
	{
		if (pszLine[nIndex] == _T('\t'))
			nOffset += (nTabSize - nOffset % nTabSize);
		else
			++nOffset;
		++nIndex;
	}
	return nIndex;
}

POINT CBaseView::TextToClient(const POINT& point)
{
	POINT pt;
	pt.y = max(0, (point.y - m_nTopLine) * GetLineHeight());
	pt.x = CalculateActualOffset(point.y, point.x);

	pt.x = (pt.x - m_nOffsetChar) * GetCharWidth() + GetMarginWidth();
	pt.y = (pt.y + GetLineHeight() + HEADERHEIGHT);
	return pt;
}

void CBaseView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CView::OnChar(nChar, nRepCnt, nFlags);

	if (m_bCaretHidden)
		return;

	if ((::GetKeyState(VK_LBUTTON) & 0x8000) != 0 ||
		(::GetKeyState(VK_RBUTTON) & 0x8000) != 0)
		return;

	if ((nChar > 31)||(nChar == VK_TAB))
	{
		RemoveSelectedText();
		AddUndoLine(m_ptCaretPos.y);
		CString sLine = GetLineChars(m_ptCaretPos.y);
		sLine.Insert(m_ptCaretPos.x, (wchar_t)nChar);
		m_pViewData->SetLine(m_ptCaretPos.y, sLine);
		m_pViewData->SetState(m_ptCaretPos.y, DIFFSTATE_EDITED);
		if (m_pViewData->GetLineEnding(m_ptCaretPos.y) == EOL_NOENDING && m_pViewData->GetCount() - 1 != m_ptCaretPos.y)
			m_pViewData->SetLineEnding(m_ptCaretPos.y, lineendings);
		m_ptCaretPos.x++;
		UpdateGoalPos();
	}
	else if (nChar == 10)
	{
		AddUndoLine(m_ptCaretPos.y);
		EOL eol = m_pViewData->GetLineEnding(m_ptCaretPos.y);
		EOL newEOL = EOL_CRLF;
		switch (eol) {
			case EOL_CRLF:
				newEOL = EOL_CR;
				break;
			case EOL_CR:
				newEOL = EOL_LF;
				break;
			case EOL_LF:
				newEOL = (m_pViewData->GetCount() - 1 == m_ptCaretPos.y && !m_pViewData->GetLine(m_ptCaretPos.y).IsEmpty()) ? EOL_NOENDING : EOL_CRLF;
				break;
		}
		m_pViewData->SetLineEnding(m_ptCaretPos.y, newEOL);
		m_pViewData->SetState(m_ptCaretPos.y, DIFFSTATE_EDITED);
		UpdateGoalPos();
	}
	else if (nChar == VK_RETURN)
	{
		// insert a new, fresh and empty line below the cursor
		RemoveSelectedText();
		AddUndoLine(m_ptCaretPos.y, true);
		EOL eOriginalEnding = m_pViewData->GetLineEnding(m_ptCaretPos.y);
		if (m_pViewData->GetCount() - 2 == m_ptCaretPos.y && eOriginalEnding == EOL_NOENDING)
			m_pViewData->SetLineEnding(m_ptCaretPos.y, lineendings);
		// move the cursor to the new line
		m_ptCaretPos.y++;
		m_ptCaretPos.x = 0;
		if (m_pViewData->GetCount() - 1 == m_ptCaretPos.y)
			m_pViewData->SetLineEnding(m_ptCaretPos.y, eOriginalEnding);
		UpdateGoalPos();
	}
	else
		return; // Unknown control character -- ignore it.
	ClearSelection();
	EnsureCaretVisible();
	UpdateCaret();
	SetModified(true);
	Invalidate(FALSE);
}

void CBaseView::AddUndoLine(int nLine, bool bAddEmptyLine)
{
	viewstate leftstate;
	viewstate rightstate;
	viewstate bottomstate;
	leftstate.AddLineFormView(m_pwndLeft, nLine, bAddEmptyLine);
	rightstate.AddLineFormView(m_pwndRight, nLine, bAddEmptyLine);
	bottomstate.AddLineFormView(m_pwndBottom, nLine, bAddEmptyLine);
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
}

void CBaseView::AddEmptyLine(int nLineIndex)
{
	if (m_pViewData == NULL)
		return;
	if (!m_bCaretHidden)
	{
		CString sPartLine = GetLineChars(nLineIndex);
		m_pViewData->SetLine(nLineIndex, sPartLine.Left(m_ptCaretPos.x));
		sPartLine = sPartLine.Mid(m_ptCaretPos.x);
		m_pViewData->InsertData(nLineIndex+1, sPartLine, DIFFSTATE_EDITED, -1, lineendings);
	}
	else
		m_pViewData->InsertData(nLineIndex+1, _T(""), DIFFSTATE_EDITED, -1, lineendings);
	Invalidate(FALSE);
}

void CBaseView::RemoveLine(int nLineIndex)
{
	if (m_pViewData == NULL)
		return;
	m_pViewData->RemoveData(nLineIndex);
	if (m_ptCaretPos.y >= GetLineCount())
		m_ptCaretPos.y = GetLineCount()-1;
	Invalidate(FALSE);
}

void CBaseView::RemoveSelectedText()
{
	if (m_pViewData == NULL)
		return;
	if (!HasTextSelection())
		return;

	viewstate rightstate;
	viewstate bottomstate;
	viewstate leftstate;
	std::vector<LONG> linestoremove;
	for (LONG i = m_ptSelectionStartPos.y; i <= m_ptSelectionEndPos.y; ++i)
	{
		if (i == m_ptSelectionStartPos.y)
		{
			CString sLine = GetLineChars(m_ptSelectionStartPos.y);
			CString newLine;
			if (i == m_ptSelectionStartPos.y)
			{
				if ((m_pwndLeft)&&(m_pwndLeft->m_pViewData))
				{
					leftstate.difflines[i] = m_pwndLeft->m_pViewData->GetLine(i);
					leftstate.linestates[i] = m_pwndLeft->m_pViewData->GetState(i);
				}
				if ((m_pwndRight)&&(m_pwndRight->m_pViewData))
				{
					rightstate.difflines[i] = m_pwndRight->m_pViewData->GetLine(i);
					rightstate.linestates[i] = m_pwndRight->m_pViewData->GetState(i);
				}
				if ((m_pwndBottom)&&(m_pwndBottom->m_pViewData))
				{
					bottomstate.difflines[i] = m_pwndBottom->m_pViewData->GetLine(i);
					bottomstate.linestates[i] = m_pwndBottom->m_pViewData->GetState(i);
				}
				newLine = sLine.Left(m_ptSelectionStartPos.x);
				sLine = GetLineChars(m_ptSelectionEndPos.y);
				newLine = newLine + sLine.Mid(m_ptSelectionEndPos.x);
			}
			m_pViewData->SetLine(i, newLine);
			m_pViewData->SetState(i, DIFFSTATE_EDITED);
			SetModified();
		}
		else
		{
			if ((m_pwndLeft)&&(m_pwndLeft->m_pViewData))
			{
				leftstate.removedlines[i] = m_pwndLeft->m_pViewData->GetData(i);
			}
			if ((m_pwndRight)&&(m_pwndRight->m_pViewData))
			{
				rightstate.removedlines[i] = m_pwndRight->m_pViewData->GetData(i);
			}
			if ((m_pwndBottom)&&(m_pwndBottom->m_pViewData))
			{
				bottomstate.removedlines[i] = m_pwndBottom->m_pViewData->GetData(i);
			}
			linestoremove.push_back(i);
		}
	}
	CUndo::GetInstance().AddState(leftstate, rightstate, bottomstate, m_ptCaretPos);
	// remove the lines at the end, to avoid problems with line indexes
	if (linestoremove.size())
	{
		std::vector<LONG>::const_iterator it = linestoremove.begin();
		int nLineToRemove = *it;
		for ( ; it != linestoremove.end(); ++it)
		{
			if (m_pwndLeft)
				m_pwndLeft->RemoveLine(nLineToRemove);
			if (m_pwndRight)
				m_pwndRight->RemoveLine(nLineToRemove);
			if (m_pwndBottom)
				m_pwndBottom->RemoveLine(nLineToRemove);
			SetModified();
		}
	}
	m_ptCaretPos = m_ptSelectionStartPos;
	UpdateGoalPos();
	ClearSelection();
	UpdateCaret();
	EnsureCaretVisible();
	Invalidate(FALSE);
}

void CBaseView::PasteText()
{
	if (!OpenClipboard())
		return;

	CString sClipboardText;
	HGLOBAL hglb = GetClipboardData(CF_TEXT);
	if (hglb)
	{
		LPCSTR lpstr = (LPCSTR)GlobalLock(hglb);
		sClipboardText = CString(lpstr);
		GlobalUnlock(hglb); 
	}
	hglb = GetClipboardData(CF_UNICODETEXT);
	if (hglb)
	{
		LPCTSTR lpstr = (LPCTSTR)GlobalLock(hglb);
		sClipboardText = lpstr;
		GlobalUnlock(hglb); 
	}
	CloseClipboard();

	if (sClipboardText.IsEmpty())
		return;

	sClipboardText.Replace(_T("\r\n"), _T("\r"));
	sClipboardText.Replace('\n', '\r');
	// We want to undo the insertion in a single step.
	CUndo::GetInstance().BeginGrouping();
	// use the easy way to insert text:
	// insert char by char, using the OnChar() method
	for (int i=0; i<sClipboardText.GetLength(); ++i)
	{
		OnChar(sClipboardText[i], 0, 0);
	}
	CUndo::GetInstance().EndGrouping();
}

void CBaseView::OnCaretDown()
{
	m_ptCaretPos.y++;
	m_ptCaretPos.y = min(m_ptCaretPos.y, GetLineCount()-1);
	m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
	if (GetKeyState(VK_SHIFT)&0x8000)
		AdjustSelection();
	else
		ClearSelection();
	UpdateCaret();
	EnsureCaretVisible();
	ShowDiffLines(m_ptCaretPos.y);
}

bool CBaseView::MoveCaretLeft()
{
	if (m_ptCaretPos.x == 0)
	{
		if (m_ptCaretPos.y > 0)
		{
			--m_ptCaretPos.y;
			m_ptCaretPos.x = GetLineLength(m_ptCaretPos.y);
		}
		else
			return false;
	}
	else
		--m_ptCaretPos.x;

	UpdateGoalPos();
	return true;
}

bool CBaseView::MoveCaretRight()
{
	if (m_ptCaretPos.x >= GetLineLength(m_ptCaretPos.y))
	{
		if (m_ptCaretPos.y < (GetLineCount() - 1))
		{
			++m_ptCaretPos.y;
			m_ptCaretPos.x = 0;
		}
		else
			return false;
	}
	else
		++m_ptCaretPos.x;

	UpdateGoalPos();
	return true;
}

void CBaseView::UpdateGoalPos()
{
	m_nCaretGoalPos = CalculateActualOffset(m_ptCaretPos.y, m_ptCaretPos.x);
}

void CBaseView::OnCaretLeft()
{
	MoveCaretLeft();
	if (GetKeyState(VK_SHIFT)&0x8000)
		AdjustSelection();
	else
		ClearSelection();
	EnsureCaretVisible();
	UpdateCaret();
}

void CBaseView::OnCaretRight()
{
	MoveCaretRight();
	if (GetKeyState(VK_SHIFT)&0x8000)
		AdjustSelection();
	else
		ClearSelection();
	EnsureCaretVisible();
	UpdateCaret();
}

void CBaseView::OnCaretUp()
{
	m_ptCaretPos.y--;
	m_ptCaretPos.y = max(0, m_ptCaretPos.y);
	m_ptCaretPos.x = CalculateCharIndex(m_ptCaretPos.y, m_nCaretGoalPos);
	if (GetKeyState(VK_SHIFT)&0x8000)
		AdjustSelection();
	else
		ClearSelection();
	UpdateCaret();
	EnsureCaretVisible();
	ShowDiffLines(m_ptCaretPos.y);
}

bool CBaseView::IsWordSeparator(wchar_t ch) const
{
	return ch == ' ' || ch == '\t' || (m_sWordSeparators.Find(ch) >= 0);
}

bool CBaseView::IsCaretAtWordBoundary() const
{
	LPCTSTR line = GetLineChars(m_ptCaretPos.y);
	if (!*line)
		return false; // no boundary at the empty lines
	if (m_ptCaretPos.x == 0)
		return !IsWordSeparator(line[m_ptCaretPos.x]);
	if (m_ptCaretPos.x >= GetLineLength(m_ptCaretPos.y))
		return !IsWordSeparator(line[m_ptCaretPos.x - 1]);
	return
		IsWordSeparator(line[m_ptCaretPos.x]) !=
		IsWordSeparator(line[m_ptCaretPos.x - 1]);
}

void CBaseView::OnCaretWordleft()
{
	while (MoveCaretLeft() && !IsCaretAtWordBoundary())
	{
	}
	if (GetKeyState(VK_SHIFT)&0x8000)
		AdjustSelection();
	else
		ClearSelection();
	EnsureCaretVisible();
	UpdateCaret();
}

void CBaseView::OnCaretWordright()
{
	while (MoveCaretRight() && !IsCaretAtWordBoundary())
	{
	}
	if (GetKeyState(VK_SHIFT)&0x8000)
		AdjustSelection();
	else
		ClearSelection();
	EnsureCaretVisible();
	UpdateCaret();
}

void CBaseView::ClearCurrentSelection()
{
	m_ptSelectionStartPos = m_ptCaretPos;
	m_ptSelectionEndPos = m_ptCaretPos;
	m_ptSelectionOrigin = m_ptCaretPos;
	m_nSelBlockStart = -1;
	m_nSelBlockEnd = -1;
	Invalidate(FALSE);
}

void CBaseView::ClearSelection()
{
	if (m_pwndLeft)
		m_pwndLeft->ClearCurrentSelection();
	if (m_pwndRight)
		m_pwndRight->ClearCurrentSelection();
	if (m_pwndBottom)
		m_pwndBottom->ClearCurrentSelection();
}

void CBaseView::AdjustSelection()
{
	if ((m_ptCaretPos.y < m_ptSelectionOrigin.y) || 
		(m_ptCaretPos.y == m_ptSelectionOrigin.y && m_ptCaretPos.x <= m_ptSelectionOrigin.x))
	{
		m_ptSelectionStartPos = m_ptCaretPos;
		m_ptSelectionEndPos = m_ptSelectionOrigin;
	}

	if ((m_ptCaretPos.y > m_ptSelectionOrigin.y) || 
		(m_ptCaretPos.y == m_ptSelectionOrigin.y && m_ptCaretPos.x >= m_ptSelectionOrigin.x))
	{
		m_ptSelectionStartPos = m_ptSelectionOrigin;
		m_ptSelectionEndPos = m_ptCaretPos;
	}

	SetupSelection(min(m_ptSelectionStartPos.y, m_ptSelectionEndPos.y), max(m_ptSelectionStartPos.y, m_ptSelectionEndPos.y));

	Invalidate(FALSE);
}

void CBaseView::OnEditCut()
{
	if (!m_bCaretHidden)
	{
		OnEditCopy();
		RemoveSelectedText();
	}
}

void CBaseView::OnEditPaste()
{
	if (!m_bCaretHidden)
	{
		PasteText();
	}
}

void CBaseView::OnEditSelectall()
{
	int nCount = GetLineCount();
	SetupSelection(0, nCount);
	m_ptSelectionStartPos.x = 0;
	m_ptSelectionStartPos.y = 0;

	m_ptSelectionEndPos.y = nCount-1;
	CString sLine = GetLineChars(nCount-1);
	m_ptSelectionEndPos.x = sLine.GetLength();

	UpdateWindow();
}
