// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2003-2013 - TortoiseSVN
// Copyright (C) 2011-2012 Sven Strickroth <email@cs-ware.de>

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
#include "AppUtils.h"
#include "GotoLineDlg.h"

// Note about lines:
// We use three different kind of lines here:
// 1. The real lines of the original files.
//    These are shown in the view margin and are not used elsewhere, they're only for user information.
// 2. Screen lines.
//    The lines actually shown on screen. All methods use screen lines as parameters/outputs if not explicitly specified otherwise.
// 3. View lines.
//    These are the lines of the diff data. If unmodified sections are collapsed, not all of those lines are shown.
//
// Basically view lines are the line data, while screen lines are shown lines.


#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MARGINWIDTH 20
#define HEADERHEIGHT 10

#define IDT_SCROLLTIMER 101

CBaseView * CBaseView::m_pwndLeft = NULL;
CBaseView * CBaseView::m_pwndRight = NULL;
CBaseView * CBaseView::m_pwndBottom = NULL;
CLocatorBar * CBaseView::m_pwndLocator = NULL;
CLineDiffBar * CBaseView::m_pwndLineDiffBar = NULL;
CMFCStatusBar * CBaseView::m_pwndStatusBar = NULL;
CMainFrame * CBaseView::m_pMainFrame = NULL;
CBaseView::Screen2View CBaseView::m_Screen2View;
const UINT CBaseView::m_FindDialogMessage = RegisterWindowMessage(FINDMSGSTRING);

allviewstate CBaseView::m_AllState;

IMPLEMENT_DYNCREATE(CBaseView, CView)

CBaseView::CBaseView()
	: m_pCacheBitmap(NULL)
	, m_pViewData(NULL)
	, m_pOtherViewData(NULL)
	, m_pOtherView(NULL)
	, m_nLineHeight(-1)
	, m_nCharWidth(-1)
	, m_nScreenChars(-1)
	, m_nLastScreenChars(-1)
	, m_nMaxLineLength(-1)
	, m_nScreenLines(-1)
	, m_nTopLine(0)
	, m_nOffsetChar(0)
	, m_nDigits(0)
	, m_nMouseLine(-1)
	, m_mouseInMargin(false)
	, m_bIsHidden(FALSE)
	, lineendings(EOL_AUTOLINE)
	, m_bReadonly(true)
	, m_bReadonlyIsChangable(false)
	, m_bTarget(false)
	, m_nCaretGoalPos(0)
	, m_nSelViewBlockStart(-1)
	, m_nSelViewBlockEnd(-1)
	, m_bFocused(FALSE)
	, m_bShowSelection(true)
	, texttype(CFileTextLines::AUTOTYPE)
	, m_bModified(FALSE)
	, m_bOtherDiffChecked(false)
	, m_bInlineWordDiff(true)
	, m_bWhitespaceInlineDiffs(false)
	, m_pState(NULL)
	, m_pFindDialog(NULL)
	, m_nStatusBarID(0)
	, m_bMatchCase(false)
	, m_bLimitToDiff(true)
	, m_bWholeWord(false)
	, m_pDC(NULL)
	, m_pWorkingFile(NULL)
{
	m_ptCaretViewPos.x = 0;
	m_ptCaretViewPos.y = 0;
	m_ptSelectionViewPosStart = m_ptCaretViewPos;
	m_ptSelectionViewPosEnd = m_ptSelectionViewPosStart;
	m_ptSelectionViewPosOrigin = m_ptSelectionViewPosEnd;
	m_bViewWhitespace = CRegDWORD(_T("Software\\TortoiseGitMerge\\ViewWhitespaces"), 1);
	m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseGitMerge\\ViewLinenumbers"), 1);
	m_bShowInlineDiff = CRegDWORD(_T("Software\\TortoiseGitMerge\\DisplayBinDiff"), TRUE);
	m_nInlineDiffMaxLineLength = CRegDWORD(_T("Software\\TortoiseGitMerge\\InlineDiffMaxLineLength"), 3000);
	m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseGitMerge\\InlineAdded"), INLINEADDED_COLOR);
	m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseGitMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
	m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorModifiedB"), MODIFIED_COLOR);
	m_WhiteSpaceFg = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\Whitespace"), GetSysColor(COLOR_GRAYTEXT));
	m_sWordSeparators = CRegString(_T("Software\\TortoiseGitMerge\\WordSeparators"), _T("[]();:.,{}!@#$%^&*-+=|/\\<>'`~\""));
	m_bIconLFs = CRegDWORD(_T("Software\\TortoiseGitMerge\\IconLFs"), 0);
	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseGitMerge\\TabSize"), 4);
	std::fill_n(m_apFonts, fontsCount, (CFont*)NULL);
	m_hConflictedIcon = LoadIcon(IDI_CONFLICTEDLINE);
	m_hConflictedIgnoredIcon = LoadIcon(IDI_CONFLICTEDIGNOREDLINE);
	m_hRemovedIcon = LoadIcon(IDI_REMOVEDLINE);
	m_hAddedIcon = LoadIcon(IDI_ADDEDLINE);
	m_hWhitespaceBlockIcon = LoadIcon(IDI_WHITESPACELINE);
	m_hEqualIcon = LoadIcon(IDI_EQUALLINE);
	m_hLineEndingCR = LoadIcon(IDI_LINEENDINGCR);
	m_hLineEndingCRLF = LoadIcon(IDI_LINEENDINGCRLF);
	m_hLineEndingLF = LoadIcon(IDI_LINEENDINGLF);
	m_hEditedIcon = LoadIcon(IDI_LINEEDITED);
	m_hMovedIcon = LoadIcon(IDI_MOVEDLINE);
	m_margincursor = (HCURSOR)LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDC_MARGINCURSOR), IMAGE_CURSOR, 0, 0, LR_DEFAULTSIZE);

	for (int i=0; i<1024; ++i)
		m_sConflictedText += _T("??");
	m_sNoLineNr.LoadString(IDS_EMPTYLINETT);

	m_szTip[0]  = 0;
	m_wszTip[0] = 0;
	SecureZeroMemory(&m_lfBaseFont, sizeof(m_lfBaseFont));
	EnableToolTips();
}

CBaseView::~CBaseView()
{
	ReleaseBitmap();
	DeleteFonts();
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
	DestroyIcon(m_hMovedIcon);
	DestroyCursor(m_margincursor);
}

BEGIN_MESSAGE_MAP(CBaseView, CView)
	ON_WM_VSCROLL()
	ON_WM_HSCROLL()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHWHEEL()
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
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_COMMAND(ID_NAVIGATE_NEXTINLINEDIFF, &CBaseView::OnNavigateNextinlinediff)
	ON_COMMAND(ID_NAVIGATE_PREVINLINEDIFF, &CBaseView::OnNavigatePrevinlinediff)
	ON_COMMAND(ID_EDIT_SELECTALL, &CBaseView::OnEditSelectall)
	ON_COMMAND(ID_EDIT_FIND, OnEditFind)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
	ON_COMMAND(ID_EDIT_FINDNEXT, OnEditFindnext)
	ON_COMMAND(ID_EDIT_FINDPREV, OnEditFindprev)
	ON_COMMAND(ID_EDIT_FINDNEXTSTART, OnEditFindnextStart)
	ON_COMMAND(ID_EDIT_FINDPREVSTART, OnEditFindprevStart)
	ON_COMMAND(ID_EDIT_GOTOLINE, &CBaseView::OnEditGotoline)
	ON_WM_LBUTTONUP()
END_MESSAGE_MAP()


void CBaseView::DocumentUpdated()
{
	ReleaseBitmap();
	m_nLineHeight = -1;
	m_nCharWidth = -1;
	m_nScreenChars = -1;
	m_nLastScreenChars = -1;
	m_nMaxLineLength = -1;
	m_nScreenLines = -1;
	m_nTopLine = 0;
	m_bModified = FALSE;
	m_bOtherDiffChecked = false;
	m_nDigits = 0;
	m_nMouseLine = -1;
	m_nTabSize = (int)(DWORD)CRegDWORD(_T("Software\\TortoiseGitMerge\\TabSize"), 4);
	m_bViewLinenumbers = CRegDWORD(_T("Software\\TortoiseGitMerge\\ViewLinenumbers"), 1);
	m_InlineAddedBk = CRegDWORD(_T("Software\\TortoiseGitMerge\\InlineAdded"), INLINEADDED_COLOR);
	m_InlineRemovedBk = CRegDWORD(_T("Software\\TortoiseGitMerge\\InlineRemoved"), INLINEREMOVED_COLOR);
	m_ModifiedBk = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\ColorModifiedB"), MODIFIED_COLOR);
	m_WhiteSpaceFg = CRegDWORD(_T("Software\\TortoiseGitMerge\\Colors\\Whitespace"), GetSysColor(COLOR_GRAYTEXT));
	m_bIconLFs = CRegDWORD(_T("Software\\TortoiseGitMerge\\IconLFs"), 0);
	m_nInlineDiffMaxLineLength = CRegDWORD(_T("Software\\TortoiseGitMerge\\InlineDiffMaxLineLength"), 3000);
	DeleteFonts();
	ClearCurrentSelection();
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
			case DIFFSTATE_MOVED_TO:
			case DIFFSTATE_THEIRSADDED:
			case DIFFSTATE_YOURSADDED:
			case DIFFSTATE_CONFLICTADDED:
				nAddedLines++;
				break;
			case DIFFSTATE_IDENTICALREMOVED:
			case DIFFSTATE_REMOVED:
			case DIFFSTATE_MOVED_FROM:
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
	case CFileTextLines::UTF16_LE:
		sBarText = _T("UTF-16LE ");
		break;
	case CFileTextLines::UTF16_BE:
		sBarText = _T("UTF-16BE ");
		break;
	case CFileTextLines::UTF32_LE:
		sBarText = _T("UTF-32LE ");
		break;
	case CFileTextLines::UTF32_BE:
		sBarText = _T("UTF-32BE ");
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
	case EOL_VT:
		sBarText += _T("VT ");
		break;
	case EOL_FF:
		sBarText += _T("FF ");
		break;
	case EOL_NEL:
		sBarText += _T("NEL ");
		break;
	case EOL_LS:
		sBarText += _T("LS ");
		break;
	case EOL_PS:
		sBarText += _T("PS ");
		break;
#ifdef _DEBUG
	case EOL_AUTOLINE:
		sBarText += _T("AEOL ");
		break;
#endif
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
		//  View must always create its own scrollbars,
		//  if only it's not used within splitter
		cs.style |= (WS_HSCROLL | WS_VSCROLL);
	}
	cs.lpszClass = AfxRegisterWndClass(CS_DBLCLKS);
	return TRUE;
}

CFont* CBaseView::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/)
{
	int nIndex = 0;
	if (bBold)
		nIndex |= 1;
	if (bItalic)
		nIndex |= 2;
	if (m_apFonts[nIndex] == NULL)
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = (BYTE) bItalic;
		CDC * pDC = GetDC();
		if (pDC)
		{
			m_lfBaseFont.lfHeight = -MulDiv((DWORD)CRegDWORD(_T("Software\\TortoiseGitMerge\\LogFontSize"), 10), GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
			ReleaseDC(pDC);
		}
		_tcsncpy_s(m_lfBaseFont.lfFaceName, (LPCTSTR)(CString)CRegString(_T("Software\\TortoiseGitMerge\\LogFontName"), _T("Courier New")), 32);
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
	if (pDC == nullptr)
		return;
	CFont *pOldFont = pDC->SelectObject(GetFont());
	const CSize szCharExt = pDC->GetTextExtent(_T("X"));
	pDC->SelectObject(pOldFont);
	ReleaseDC(pDC);

	m_nLineHeight = szCharExt.cy;
	if (m_nLineHeight <= 0)
		m_nLineHeight = -1;
	m_nCharWidth = szCharExt.cx;
	if (m_nCharWidth <= 0)
		m_nCharWidth = -1;
}

int CBaseView::GetScreenChars()
{
	if (m_nScreenChars == -1)
	{
		CRect rect;
		GetClientRect(&rect);
		m_nScreenChars = (rect.Width() - GetMarginWidth() - GetSystemMetrics(SM_CXVSCROLL)) / GetCharWidth();
		if (m_nScreenChars < 0)
			m_nScreenChars = 0;
	}
	return m_nScreenChars;
}

int CBaseView::GetAllMinScreenChars() const
{
	int nChars = INT_MAX;
	if (IsLeftViewGood())
		nChars = std::min<int>(nChars, m_pwndLeft->GetScreenChars());
	if (IsRightViewGood())
		nChars = std::min<int>(nChars, m_pwndRight->GetScreenChars());
	if (IsBottomViewGood())
		nChars = std::min<int>(nChars, m_pwndBottom->GetScreenChars());
	return (nChars==INT_MAX) ? 0 : nChars;
}

int CBaseView::GetAllMaxLineLength() const
{
	int nLength = 0;
	if (IsLeftViewGood())
		nLength = std::max<int>(nLength, m_pwndLeft->GetMaxLineLength());
	if (IsRightViewGood())
		nLength = std::max<int>(nLength, m_pwndRight->GetMaxLineLength());
	if (IsBottomViewGood())
		nLength = std::max<int>(nLength, m_pwndBottom->GetMaxLineLength());
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
			int nActualLength = GetLineLength(i);
			if (m_nMaxLineLength < nActualLength)
				m_nMaxLineLength = nActualLength;
		}
	}
	return m_nMaxLineLength;
}

int CBaseView::GetLineLength(int index)
{
	if (m_pViewData == NULL)
		return 0;
	if (m_pViewData->GetCount() == 0)
		return 0;
	if ((int)m_Screen2View.size() <= index)
		return 0;
	int viewLine = GetViewLineForScreen(index);
	if (m_pMainFrame->m_bWrapLines)
	{
		int nLineLength = GetLineChars(index).GetLength();
		ASSERT(nLineLength >= 0);
		return nLineLength;
	}
	int nLineLength = m_pViewData->GetLine(viewLine).GetLength();
	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetViewLineLength(int nViewLine) const
{
	if (m_pViewData == NULL)
		return 0;
	if (m_pViewData->GetCount() <= nViewLine)
		return 0;
	int nLineLength = m_pViewData->GetLine(nViewLine).GetLength();
	ASSERT(nLineLength >= 0);
	return nLineLength;
}

int CBaseView::GetLineCount() const
{
	if (m_pViewData == NULL)
		return 1;
	int nLineCount = (int)m_Screen2View.size();
	ASSERT(nLineCount >= 0);
	return nLineCount;
}

int CBaseView::GetSubLineOffset(int index)
{
	return m_Screen2View.GetSubLineOffset(index);
}

CString CBaseView::GetViewLineChars(int nViewLine) const
{
	if (m_pViewData == NULL)
		return 0;
	if (m_pViewData->GetCount() <= nViewLine)
		return 0;
	return m_pViewData->GetLine(nViewLine);
}

CString CBaseView::GetLineChars(int index)
{
	if (m_pViewData == NULL)
		return 0;
	if (m_pViewData->GetCount() == 0)
		return 0;
	if ((int)m_Screen2View.size() <= index)
		return 0;
	int viewLine = GetViewLineForScreen(index);
	if (m_pMainFrame->m_bWrapLines)
	{
		int subLine = GetSubLineOffset(index);
		if (subLine >= 0)
		{
			if (subLine < CountMultiLines(viewLine))
			{
				return m_ScreenedViewLine[viewLine].SubLines[subLine];
			}
			return L"";
		}
	}
	return m_pViewData->GetLine(viewLine);
}

void CBaseView::CheckOtherView()
{
	if (m_bOtherDiffChecked)
		return;
	// find out what the 'other' file is
	m_pOtherViewData = NULL;
	m_pOtherView = NULL;
	if (this == m_pwndLeft && IsRightViewGood())
	{
		m_pOtherViewData = m_pwndRight->m_pViewData;
		m_pOtherView = m_pwndRight;
	}

	if (this == m_pwndRight && IsLeftViewGood())
	{
		m_pOtherViewData = m_pwndLeft->m_pViewData;
		m_pOtherView = m_pwndLeft;
	}

	m_bOtherDiffChecked = true;
}


void CBaseView::GetWhitespaceBlock(CViewData *viewData, int nLineIndex, int & nStartBlock, int & nEndBlock)
{
	enum { MAX_WHITESPACEBLOCK_SIZE = 8 };
	ASSERT(viewData);

	DiffStates origstate = viewData->GetState(nLineIndex);

	// Go back and forward at most MAX_WHITESPACEBLOCK_SIZE lines to see where this block ends
	nStartBlock = nLineIndex;
	nEndBlock = nLineIndex;
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
}

CString CBaseView::GetWhitespaceString(CViewData *viewData, int nStartBlock, int nEndBlock)
{
	enum { MAX_WHITESPACEBLOCK_SIZE = 8 };
	int len = 0;
	for (int i = nStartBlock; i <= nEndBlock; ++i)
		len += viewData->GetLine(i).GetLength();

	CString block;
	// do not check for whitespace blocks if the line is too long, because
	// reserving a lot of memory here takes too much time (performance hog)
	if (len > MAX_WHITESPACEBLOCK_SIZE*256)
		return block;
	block.Preallocate(len+1);
	for (int i = nStartBlock; i <= nEndBlock; ++i)
		block += viewData->GetLine(i);
	return block;
}

bool CBaseView::IsBlockWhitespaceOnly(int nLineIndex, bool& bIdentical)
{
	if (m_pViewData == NULL)
		return false;
	bIdentical = false;
	CheckOtherView();
	if (!m_pOtherViewData)
		return false;
	int viewLine = GetViewLineForScreen(nLineIndex);
	if (
		(m_pViewData->GetState(viewLine) == DIFFSTATE_NORMAL) &&
		(m_pOtherViewData->GetLine(viewLine) == m_pViewData->GetLine(viewLine))
		)
	{
		bIdentical = true;
		return false;
	}
	// first check whether the line itself only has whitespace changes
	CString mine = m_pViewData->GetLine(viewLine);
	CString other = m_pOtherViewData->GetLine(min(viewLine, m_pOtherViewData->GetCount() - 1));
	if (mine.IsEmpty() && other.IsEmpty())
	{
		bIdentical = true;
		return false;
	}

	if (mine == other)
	{
		bIdentical = true;
		return true;
	}
	FilterWhitespaces(mine, other);
	if (mine == other)
		return true;

	int nStartBlock1, nEndBlock1;
	int nStartBlock2, nEndBlock2;
	GetWhitespaceBlock(m_pViewData, viewLine, nStartBlock1, nEndBlock1);
	GetWhitespaceBlock(m_pOtherViewData, min(viewLine, m_pOtherViewData->GetCount() - 1), nStartBlock2, nEndBlock2);
	mine = GetWhitespaceString(m_pViewData, nStartBlock1, nEndBlock1);
	if (mine.IsEmpty())
		bIdentical = false;
	else
	{
		other = GetWhitespaceString(m_pOtherViewData, nStartBlock2, nEndBlock2);
		bIdentical = mine == other;
		FilterWhitespaces(mine, other);
	}

	return (!mine.IsEmpty()) && (mine == other);
}

bool CBaseView::IsViewLineHidden(int nViewLine)
{
	return IsViewLineHidden(m_pViewData, nViewLine);
}

bool CBaseView::IsViewLineHidden(CViewData * pViewData, int nViewLine)
{
	return m_pMainFrame->m_bCollapsed && (pViewData->GetHideState(nViewLine)!=HIDESTATE_SHOWN);
}

int CBaseView::GetLineNumber(int index) const
{
	if (m_pViewData == NULL)
		return -1;
	int viewLine = GetViewLineForScreen(index);
	if (m_pViewData->GetLineNumber(viewLine)==DIFF_EMPTYLINENUMBER)
		return -1;
	return m_pViewData->GetLineNumber(viewLine);
}

int CBaseView::GetScreenLines()
{
	if (m_nScreenLines == -1)
	{
		SCROLLBARINFO sbi;
		sbi.cbSize = sizeof(sbi);
		int scrollBarHeight = 0;
		if (GetScrollBarInfo(OBJID_HSCROLL, &sbi))
			scrollBarHeight = sbi.rcScrollBar.bottom - sbi.rcScrollBar.top;
		if ((sbi.rgstate[0] & STATE_SYSTEM_INVISIBLE)||(sbi.rgstate[0] & STATE_SYSTEM_UNAVAILABLE))
			scrollBarHeight = 0;
		CRect rect;
		GetClientRect(&rect);
		m_nScreenLines = (rect.Height() - HEADERHEIGHT - scrollBarHeight) / GetLineHeight();
		if (m_nScreenLines < 0)
			m_nScreenLines = 0;
	}
	return m_nScreenLines;
}

int CBaseView::GetAllMinScreenLines() const
{
	int nLines = INT_MAX;
	if (IsLeftViewGood())
		nLines = m_pwndLeft->GetScreenLines();
	if (IsRightViewGood())
		nLines = std::min<int>(nLines, m_pwndRight->GetScreenLines());
	if (IsBottomViewGood())
		nLines = std::min<int>(nLines, m_pwndBottom->GetScreenLines());
	return (nLines==INT_MAX) ? 0 : nLines;
}

int CBaseView::GetAllLineCount() const
{
	int nLines = 0;
	if (IsLeftViewGood())
		nLines = m_pwndLeft->GetLineCount();
	if (IsRightViewGood())
		nLines = std::max<int>(nLines, m_pwndRight->GetLineCount());
	if (IsBottomViewGood())
		nLines = std::max<int>(nLines, m_pwndBottom->GetLineCount());
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
	//  Note we cannot use nPos because of its 16-bit nature
	SCROLLINFO si;
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	VERIFY(master->GetScrollInfo(SB_VERT, &si));

	int nPageLines = GetScreenLines();
	int nLineCount = GetLineCount();

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
			RECT thumbrect;
			GetClientRect(&thumbrect);
			ClientToScreen(&thumbrect);

			POINT thumbpoint;
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
		EnableScrollBarCtrl(SB_HORZ, !m_pMainFrame->m_bWrapLines);
		if (!m_pMainFrame->m_bWrapLines)
		{
			int minScreenChars = GetAllMinScreenChars();
			int maxLineLength = GetAllMaxLineLength();
			if (minScreenChars >= maxLineLength && m_nOffsetChar > 0)
			{
				m_nOffsetChar = 0;
				Invalidate();
			}
			si.fMask = SIF_DISABLENOSCROLL | SIF_PAGE | SIF_POS | SIF_RANGE;
			si.nMin = 0;
			si.nMax = m_pMainFrame->m_bWrapLines ? minScreenChars : maxLineLength;
			si.nMax += GetMarginWidth()/GetCharWidth();
			si.nPage = minScreenChars;
			si.nPos = m_nOffsetChar;
		}
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
		if (m_pwndLineDiffBar)
			m_pwndLineDiffBar->Invalidate();
	}
}

void CBaseView::ScrollAllToChar(int nNewOffsetChar, BOOL bTrackScrollBar /* = TRUE */)
{
	if (m_pwndLeft)
		m_pwndLeft->ScrollToChar(nNewOffsetChar, bTrackScrollBar);
	if (m_pwndRight)
		m_pwndRight->ScrollToChar(nNewOffsetChar, bTrackScrollBar);
	if (m_pwndBottom)
		m_pwndBottom->ScrollToChar(nNewOffsetChar, bTrackScrollBar);
}

void CBaseView::ScrollAllSide(int delta)
{
	int nNewOffset = m_nOffsetChar;
	nNewOffset += delta;
	int nMaxLineLength = GetMaxLineLength();
	if (nNewOffset >= nMaxLineLength)
		nNewOffset = nMaxLineLength - 1;
	if (nNewOffset < 0)
		nNewOffset = 0;
	ScrollAllToChar(nNewOffset, TRUE);
	if (m_pwndLineDiffBar)
		m_pwndLineDiffBar->Invalidate();
	UpdateCaret();
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

void CBaseView::ScrollVertical(short zDelta)
{
	const int nLineCount = GetLineCount();
	int nTopLine = m_nTopLine;
	nTopLine -= (zDelta/30);
	if (nTopLine < 0)
		nTopLine = 0;
	if (nTopLine >= nLineCount)
		nTopLine = nLineCount - 1;
	ScrollToLine(nTopLine, TRUE);
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
		int nViewLine = GetViewLineForScreen(nLineIndex);
		HICON icon = NULL;
		ASSERT(nViewLine<(int)m_ScreenedViewLine.size());
		TScreenedViewLine::EIcon eIcon = m_ScreenedViewLine[nViewLine].eIcon;
		if (eIcon==TScreenedViewLine::ICN_UNKNOWN)
		{
			DiffStates state = m_pViewData->GetState(nViewLine);
			switch (state)
			{
			case DIFFSTATE_ADDED:
			case DIFFSTATE_THEIRSADDED:
			case DIFFSTATE_YOURSADDED:
			case DIFFSTATE_IDENTICALADDED:
			case DIFFSTATE_CONFLICTADDED:
				eIcon = TScreenedViewLine::ICN_ADD;
				break;
			case DIFFSTATE_REMOVED:
			case DIFFSTATE_THEIRSREMOVED:
			case DIFFSTATE_YOURSREMOVED:
			case DIFFSTATE_IDENTICALREMOVED:
				eIcon = TScreenedViewLine::ICN_REMOVED;
				break;
			case DIFFSTATE_CONFLICTED:
				eIcon = TScreenedViewLine::ICN_CONFLICT;
				break;
			case DIFFSTATE_CONFLICTED_IGNORED:
				eIcon = TScreenedViewLine::ICN_CONFLICTIGNORED;
				break;
			case DIFFSTATE_EDITED:
				eIcon = TScreenedViewLine::ICN_EDIT;
				break;
			case DIFFSTATE_MOVED_TO:
			case DIFFSTATE_MOVED_FROM:
				eIcon = TScreenedViewLine::ICN_MOVED;
				break;
			default:
				break;
			}
			bool bIdentical = false;
			if ((state != DIFFSTATE_EDITED)&&(IsBlockWhitespaceOnly(nLineIndex, bIdentical)))
			{
				if (bIdentical)
					eIcon = TScreenedViewLine::ICN_SAME;
				else
					eIcon = TScreenedViewLine::ICN_WHITESPACEDIFF;
			}
			m_ScreenedViewLine[nViewLine].eIcon = eIcon;
		}
		switch (eIcon)
		{
		case TScreenedViewLine::ICN_UNKNOWN:
		case TScreenedViewLine::ICN_NONE:
			 break;
		case TScreenedViewLine::ICN_SAME:
			icon = m_hEqualIcon;
			break;
		case TScreenedViewLine::ICN_EDIT:
			icon = m_hEditedIcon;
			break;
		case TScreenedViewLine::ICN_WHITESPACEDIFF:
			icon = m_hWhitespaceBlockIcon;
			break;
		case TScreenedViewLine::ICN_ADD:
			icon = m_hAddedIcon;
			break;
		case TScreenedViewLine::ICN_CONFLICT:
			icon = m_hConflictedIcon;
			break;
		case TScreenedViewLine::ICN_CONFLICTIGNORED:
			icon = m_hConflictedIgnoredIcon;
			break;
		case TScreenedViewLine::ICN_REMOVED:
			icon = m_hRemovedIcon;
			break;
		case TScreenedViewLine::ICN_MOVED:
			icon = m_hMovedIcon;
			break;
		}


		if (icon)
		{
			::DrawIconEx(pdc->m_hDC, rect.left + 2, rect.top + (rect.Height()-16)/2, icon, 16, 16, NULL, NULL, DI_NORMAL);
		}
		if ((m_bViewLinenumbers)&&(m_nDigits))
		{
			int nSubLine = GetSubLineOffset(nLineIndex);
			bool bIsFirstSubline = (nSubLine == 0) || (nSubLine == -1);
			CString sLinenumber;
			if (bIsFirstSubline)
			{
				CString sLinenumberFormat;
				int nLineNumber = GetLineNumber(nLineIndex);
				if (IsViewLineHidden(GetViewLineForScreen(nLineIndex)))
				{
					// TODO: do not show if there is no number hidden
					// TODO: show number if there is only one
					sLinenumberFormat.Format(_T("%%%ds"), m_nDigits);
					sLinenumber.Format(sLinenumberFormat, (m_nDigits>1) ? _T("↕⁞") : _T("⁞")); // alternative …
				}
				else if (nLineNumber >= 0)
				{
					sLinenumberFormat.Format(_T("%%%dd"), m_nDigits);
					sLinenumber.Format(sLinenumberFormat, nLineNumber+1);
				}
				else if (m_pMainFrame->m_bWrapLines)
				{
					sLinenumberFormat.Format(_T("%%%ds"), m_nDigits);
					sLinenumber.Format(sLinenumberFormat, _T("·"));
				}
				if (!sLinenumber.IsEmpty())
				{
					pdc->SetBkColor(::GetSysColor(COLOR_SCROLLBAR));
					pdc->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));

					pdc->SelectObject(GetFont());
					pdc->ExtTextOut(rect.left + 18, rect.top, ETO_CLIPPED, &rect, sLinenumber, NULL);
				}
			}
		}
	}
}

int CBaseView::GetMarginWidth()
{
	if ((m_bViewLinenumbers)&&(m_pViewData)&&(m_pViewData->GetCount()))
	{
		if (m_nDigits <= 0)
		{
			int nLength = (int)m_pViewData->GetCount();
			// find out how many digits are needed to show the highest line number
			CString sMax;
			sMax.Format(_T("%d"), nLength);
			m_nDigits = sMax.GetLength();
		}
		int nWidth = GetCharWidth();
		return (MARGINWIDTH + (m_nDigits * nWidth) + 2);
	}
	return MARGINWIDTH;
}

void CBaseView::DrawHeader(CDC *pdc, const CRect &rect)
{
	CRect textrect(rect.left, rect.top, rect.Width(), GetLineHeight()+HEADERHEIGHT);
	COLORREF crBk, crFg;
	if (IsBottomViewGood())
	{
		CDiffColors::GetInstance().GetColors(DIFFSTATE_NORMAL, crBk, crFg);
		crBk = ::GetSysColor(COLOR_SCROLLBAR);
	}
	else
	{
		DiffStates state = DIFFSTATE_REMOVED;
		if (this == m_pwndRight)
		{
			state = DIFFSTATE_ADDED;
		}
		CDiffColors::GetInstance().GetColors(state, crBk, crFg);
	}
	pdc->SetBkColor(crBk);
	pdc->FillSolidRect(textrect, crBk);

	pdc->SetTextColor(crFg);

	pdc->SelectObject(GetFont(FALSE, TRUE));

	CString sViewTitle;
	if (IsModified())
	{
		 sViewTitle = _T("* ") + m_sWindowName;
	}
	else
	{
		sViewTitle = m_sWindowName;
	}
	int nStringLength = (GetCharWidth()*m_sWindowName.GetLength());
	if (nStringLength > rect.Width())
	{
		int offset = std::min<int>(m_nOffsetChar, (nStringLength-rect.Width())/GetCharWidth()+1);
		sViewTitle = m_sWindowName.Mid(offset);
	}
	pdc->ExtTextOut(std::max<int>(rect.left + (rect.Width()-nStringLength)/2, 1),
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
	bool bBeyondFileLineCached = false;
	while (rcLine.top < rcClient.bottom)
	{
		if (nCurrentLine < nLineCount)
		{
			DrawMargin(&cacheDC, rcCacheMargin, nCurrentLine);
			DrawSingleLine(&cacheDC, rcCacheLine, nCurrentLine);
			bBeyondFileLineCached = false;
		}
		else if (!bBeyondFileLineCached)
		{
			DrawMargin(&cacheDC, rcCacheMargin, -1);
			DrawSingleLine(&cacheDC, rcCacheLine, -1);
			bBeyondFileLineCached = true;
		}

		VERIFY(pDC->BitBlt(rcLine.left, rcLine.top, rcLine.Width(), rcLine.Height(), &cacheDC, 0, 0, SRCCOPY));

		nCurrentLine ++;
		rcLine.OffsetRect(0, nLineHeight);
	}

	cacheDC.SelectObject(pOldBitmap);
	cacheDC.DeleteDC();
}

bool CBaseView::IsStateConflicted(DiffStates state)
{
	switch (state)
	{
	 case DIFFSTATE_CONFLICTED:
	 case DIFFSTATE_CONFLICTED_IGNORED:
	 case DIFFSTATE_CONFLICTEMPTY:
	 case DIFFSTATE_CONFLICTADDED:
		return true;
	}
	return false;
}

bool CBaseView::IsStateEmpty(DiffStates state)
{
	switch (state)
	{
	 case DIFFSTATE_CONFLICTEMPTY:
	 case DIFFSTATE_UNKNOWN:
	 case DIFFSTATE_EMPTY:
		return true;
	}
	return false;
}

bool CBaseView::IsStateRemoved(DiffStates state)
{
	switch (state)
	{
	 case DIFFSTATE_REMOVED:
	 case DIFFSTATE_MOVED_FROM:
	 case DIFFSTATE_THEIRSREMOVED:
	 case DIFFSTATE_YOURSREMOVED:
	 case DIFFSTATE_IDENTICALREMOVED:
		return true;
	}
	return false;
}

DiffStates CBaseView::ResolveState(DiffStates state)
{
	if (IsStateConflicted(state))
	{
		if (state == DIFFSTATE_CONFLICTEMPTY)
			return DIFFSTATE_CONFLICTRESOLVEDEMPTY;
		else
			return DIFFSTATE_CONFLICTRESOLVED;
	}
	return state;
}


bool CBaseView::IsLineEmpty(int nLineIndex)
{
	if (m_pViewData == 0)
		return FALSE;
	int nViewLine = GetViewLineForScreen(nLineIndex);
	return IsViewLineEmpty(nViewLine);
}

bool CBaseView::IsViewLineEmpty(int nViewLine)
{
	if (m_pViewData == 0)
		return FALSE;
	const DiffStates state = m_pViewData->GetState(nViewLine);
	return IsStateEmpty(state);
}

bool CBaseView::IsLineRemoved(int nLineIndex)
{
	if (m_pViewData == 0)
		return FALSE;
	int nViewLine = GetViewLineForScreen(nLineIndex);
	return IsViewLineRemoved(nViewLine);
}

bool CBaseView::IsViewLineRemoved(int nViewLine)
{
	if (m_pViewData == 0)
		return FALSE;
	const DiffStates state = m_pViewData->GetState(nViewLine);
	return IsStateRemoved(state);
}

bool CBaseView::IsViewLineConflicted(int nLineIndex)
{
	if (m_pViewData == 0)
		return false;
	const DiffStates state = m_pViewData->GetState(nLineIndex);
	return IsStateConflicted(state);
}

COLORREF CBaseView::InlineDiffColor(int nLineIndex)
{
	return IsLineRemoved(nLineIndex) ? m_InlineRemovedBk : m_InlineAddedBk;
}

COLORREF CBaseView::InlineViewLineDiffColor(int nViewLine)
{
	return IsViewLineRemoved(nViewLine) ? m_InlineRemovedBk : m_InlineAddedBk;
}

void CBaseView::DrawLineEnding(CDC *pDC, const CRect &rc, int nLineIndex, const CPoint& origin)
{
	if (!(m_bViewWhitespace && m_pViewData && (nLineIndex >= 0) && (nLineIndex < GetLineCount())))
		return;
	int viewLine = GetViewLineForScreen(nLineIndex);
	EOL ending = m_pViewData->GetLineEnding(viewLine);
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
			(viewLine < m_pOtherViewData->GetCount()) &&
			(ending != EOL_NOENDING) &&
			(ending != m_pOtherViewData->GetLineEnding(viewLine) &&
			(m_pOtherViewData->GetLineEnding(viewLine) != EOL_NOENDING))
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
		bool bMultiline = false;
		if (((int)m_Screen2View.size() > nLineIndex+1) && (GetViewLineForScreen(nLineIndex+1) == viewLine))
		{
			if (GetLineLength(nLineIndex+1))
			{
				// multiline
				bMultiline = true;
				pDC->MoveTo(origin.x, yMiddle-2);
				pDC->LineTo(origin.x+GetCharWidth()-1, yMiddle-2);
				pDC->LineTo(origin.x+GetCharWidth()-1, yMiddle+2);
				pDC->LineTo(origin.x, yMiddle+2);
			}
			else if (GetLineLength(nLineIndex) == 0)
				bMultiline = true;
		}
		else if ((nLineIndex > 0) && (GetViewLineForScreen(nLineIndex-1) == viewLine) && (GetLineLength(nLineIndex) == 0))
			bMultiline = true;

		if (!bMultiline)
		{
			switch (ending)
			{
			case EOL_AUTOLINE:
			case EOL_CRLF:
				// arrow from top to middle+2, then left
				pDC->MoveTo(origin.x+GetCharWidth()-1, rc.top+1);
				pDC->LineTo(origin.x+GetCharWidth()-1, yMiddle);
			case EOL_CR:
				// arrow from right to left
				pDC->MoveTo(origin.x+GetCharWidth()-1, yMiddle);
				pDC->LineTo(origin.x, yMiddle);
				pDC->LineTo(origin.x+4, yMiddle+4);
				pDC->MoveTo(origin.x, yMiddle);
				pDC->LineTo(origin.x+4, yMiddle-4);
				break;
			case EOL_LFCR:
				// from right-upper to left then down
				pDC->MoveTo(origin.x+GetCharWidth()-1, yMiddle-2);
				pDC->LineTo(xMiddle, yMiddle-2);
				pDC->LineTo(xMiddle, rc.bottom-1);
				pDC->LineTo(xMiddle+4, rc.bottom-5);
				pDC->MoveTo(xMiddle, rc.bottom-1);
				pDC->LineTo(xMiddle-4, rc.bottom-5);
				break;
			case EOL_LF:
				// arrow from top to bottom
				pDC->MoveTo(xMiddle, rc.top);
				pDC->LineTo(xMiddle, rc.bottom-1);
				pDC->LineTo(xMiddle+4, rc.bottom-5);
				pDC->MoveTo(xMiddle, rc.bottom-1);
				pDC->LineTo(xMiddle-4, rc.bottom-5);
				break;
			case EOL_FF:    // Form Feed, U+000C
			case EOL_NEL:   // Next Line, U+0085
			case EOL_LS:    // Line Separator, U+2028
			case EOL_PS:    // Paragraph Separator, U+2029
				// draw a horizontal line at the bottom of this line
				pDC->FillSolidRect(rc.left, rc.bottom-1, rc.right, rc.bottom, GetSysColor(COLOR_WINDOWTEXT));
				pDC->MoveTo(origin.x+GetCharWidth()-1, rc.bottom-GetCharWidth()-2);
				pDC->LineTo(origin.x, rc.bottom-2);
				pDC->LineTo(origin.x+5, rc.bottom-2);
				pDC->MoveTo(origin.x, rc.bottom-2);
				pDC->LineTo(origin.x+1, rc.bottom-6);
				break;
			default: // other EOLs
				// arrow from top right to bottom left
				pDC->MoveTo(origin.x+GetCharWidth()-1, rc.bottom-GetCharWidth());
				pDC->LineTo(origin.x, rc.bottom-1);
				pDC->LineTo(origin.x+5, rc.bottom-2);
				pDC->MoveTo(origin.x, rc.bottom-1);
				pDC->LineTo(origin.x+1, rc.bottom-6);
				break;
			case EOL_NOENDING:
				break;
			}
		}
		pDC->SelectObject(oldpen);
	}
}

void CBaseView::DrawBlockLine(CDC *pDC, const CRect &rc, int nLineIndex)
{
	if (!m_bShowSelection)
		return;

	int nSelBlockStart;
	int nSelBlockEnd;
	if (!GetViewSelection(nSelBlockStart, nSelBlockEnd))
		return;

	const int THICKNESS = 2;
	COLORREF rectcol = GetSysColor(m_bFocused ? COLOR_WINDOWTEXT : COLOR_GRAYTEXT);

	int nViewLineIndex = GetViewLineForScreen(nLineIndex);
	int nSubLine = GetSubLineOffset(nLineIndex);
	bool bFirstLineOfViewLine = (nSubLine==0 || nSubLine==-1);
	if ((nViewLineIndex == nSelBlockStart) && bFirstLineOfViewLine)
	{
		pDC->FillSolidRect(rc.left, rc.top, rc.Width(), THICKNESS, rectcol);
	}

	bool bLastLineOfViewLine = (nLineIndex+1 == m_Screen2View.size()) || (GetViewLineForScreen(nLineIndex) != GetViewLineForScreen(nLineIndex+1));
	if ((nViewLineIndex == nSelBlockEnd) && bLastLineOfViewLine)
	{
		pDC->FillSolidRect(rc.left, rc.bottom - THICKNESS, rc.Width(), THICKNESS, rectcol);
	}
}

void CBaseView::DrawTextLine(
	CDC * pDC, const CRect &rc, int nLineIndex, POINT& coords)
 {
	ASSERT(nLineIndex < GetLineCount());
	int nViewLine = GetViewLineForScreen(nLineIndex);
	ASSERT(m_pViewData && (nViewLine < m_pViewData->GetCount()));

	LineColors lineCols = GetLineColors(nViewLine);

	CString sViewLine = GetViewLineChars(nViewLine);
	// mark selection
	if (m_bShowSelection && HasTextSelection())
	{
		// has this line selection ?
		if ((m_ptSelectionViewPosStart.y <= nViewLine) && (nViewLine <= m_ptSelectionViewPosEnd.y))
		{
			int nViewLineLength = sViewLine.GetLength();

			// first suppose the whole line is selected
			int selectedStart = 0;
			int selectedEnd = nViewLineLength;

			// the view line is partially selected
			if (m_ptSelectionViewPosStart.y == nViewLine)
			{
				selectedStart = m_ptSelectionViewPosStart.x;
			}

			if (m_ptSelectionViewPosEnd.y == nViewLine)
			{
				selectedEnd =  m_ptSelectionViewPosEnd.x;
			}
			// apply selection coloring
			// First enforce start and end point
			lineCols.SplitBlock(selectedStart);
			lineCols.SplitBlock(selectedEnd);
			// change color of affected parts
			long intenseColorScale = m_bFocused ? 70 : 30;
			std::map<int, linecolors_t>::iterator it = lineCols.lower_bound(selectedStart);
			for ( ; it != lineCols.end() && it->first < selectedEnd; ++it)
			{
				COLORREF crBk = CAppUtils::IntenseColor(intenseColorScale, it->second.background);
				if (it->second.shot == it->second.background)
				{
					it->second.shot = crBk;
				}
				it->second.background = crBk;
				it->second.text = CAppUtils::IntenseColor(intenseColorScale, it->second.text);
			}
		}
	}

	// TODO: remove duplicate from selection and mark
	if (!m_sMarkedWord.IsEmpty())
	{
		int nMarkLength = m_sMarkedWord.GetLength();
		//int nViewLineLength = sViewLine.GetLength();
		const TCHAR * text = sViewLine;
		const TCHAR * findText = text;
		while ((findText = _tcsstr(findText, (LPCTSTR)m_sMarkedWord))!=0)
		{
			int nMarkStart = static_cast<int>(findText - text);
			int nMarkEnd = nMarkStart + nMarkLength;
			// First enforce start and end point
			lineCols.SplitBlock(nMarkStart);
			lineCols.SplitBlock(nMarkEnd);
			// change color of affected parts
			const long int nIntenseColorScale = 200;
			std::map<int, linecolors_t>::iterator it = lineCols.lower_bound(nMarkStart);
			for ( ; it != lineCols.end() && it->first < nMarkEnd; ++it)
			{
				COLORREF crBk = CAppUtils::IntenseColor(nIntenseColorScale, it->second.background);
				if (it->second.shot == it->second.background)
				{
					it->second.shot = crBk;
				}
				it->second.background = crBk;
				it->second.text = CAppUtils::IntenseColor(nIntenseColorScale, it->second.text);
			}
			findText += nMarkLength;
		}
	}
	if (!m_sFindText.IsEmpty())
	{
		int nMarkStart = 0;
		int nMarkEnd = 0;
		int nStringPos = nMarkStart;
		CString searchLine = sViewLine;
		if (!m_bMatchCase)
			searchLine.MakeLower();
		while (StringFound(searchLine, SearchNext, nMarkStart, nMarkEnd)!=0)
		{
			// First enforce start and end point
			lineCols.SplitBlock(nMarkStart+nStringPos);
			lineCols.SplitBlock(nMarkEnd+nStringPos);
			// change color of affected parts
			const long int nIntenseColorScale = 30;
			std::map<int, linecolors_t>::iterator it = lineCols.lower_bound(nMarkStart+nStringPos);
			for ( ; it != lineCols.end() && it->first < nMarkEnd; ++it)
			{
				COLORREF crBk = CAppUtils::IntenseColor(nIntenseColorScale, it->second.background);
				if (it->second.shot == it->second.background)
				{
					it->second.shot = crBk;
				}
				it->second.background = crBk;
				it->second.text = CAppUtils::IntenseColor(nIntenseColorScale, it->second.text);
			}
			searchLine = searchLine.Mid(nMarkEnd);
			nStringPos = nMarkEnd;
		}
	}

	// @ this point we may cache data for next line which may be same in wrapped mode

	int nTextOffset = 0;
	int nSubline = GetSubLineOffset(nLineIndex);
	for (int n=0; n<nSubline; n++)
	{
		CString sLine = m_ScreenedViewLine[nViewLine].SubLines[n];
		nTextOffset += sLine.GetLength();
	}

	CString sLine = GetLineChars(nLineIndex);
	int nLineLength = sLine.GetLength();
	CString sLineExp = ExpandChars(sLine);
	LPCTSTR textExp = sLineExp;
	//int nLineLengthExp = sLineExp.GetLength();
	int nStartExp = 0;
	int nLeft = coords.x;
	for (std::map<int, linecolors_t>::const_iterator itStart = lineCols.begin(); itStart != lineCols.end(); ++itStart)
	{
		std::map<int, linecolors_t>::const_iterator itEnd = itStart;
		++itEnd;
		int nStart = std::max<int>(0, itStart->first - nTextOffset);
		int nEnd = nLineLength;
		if (itEnd != lineCols.end())
		{
			 nEnd = std::min<int>(nEnd, itEnd->first - nTextOffset);
		}
		int nBlockLength = nEnd - nStart;
		if (nBlockLength > 0 && nEnd>=0)
		{
			pDC->SetBkColor(itStart->second.background);
			pDC->SetTextColor(itStart->second.text);
			int nEndExp = CountExpandedChars(sLine, nEnd);
			int nTextLength = nEndExp - nStartExp;
			LPCTSTR p_zBlockText = textExp + nStartExp;
			SIZE Size;
			GetTextExtentPoint32(pDC->GetSafeHdc(), p_zBlockText, nTextLength, &Size); // falls time-2-tme
			int nRight = nLeft + Size.cx;
			if ((nRight > rc.left) && (nLeft < rc.right))
			{
				// note: ExtTextOut has a limit for the length of the string. That limit is supposed
				// to be 8192, but that's not really true: I found that the limit (at least on my machine and a few others)
				// is 4094 (4095 doesn't work anymore).
				// So we limit the length here to that 4094 chars.
				// In case we're scrolled to the right, there's no need to draw the string
				// from way outside our window, so we also offset the drawing to the start of the window.
				// This reduces the string length as well.
				int offset = 0;
				int leftcoord = nLeft;
				if (nLeft < 0)
				{
					offset = (-nLeft/GetCharWidth());
					nTextLength -= offset;
					leftcoord = nLeft % GetCharWidth();
				}

				pDC->ExtTextOut(leftcoord, coords.y, ETO_CLIPPED, &rc, p_zBlockText+offset, min(nTextLength, 4094), NULL);
				if ((itStart->second.shot != itStart->second.background) && (itStart->first == nStart + nTextOffset))
				{
					pDC->FillSolidRect(nLeft-1, rc.top, 1, rc.Height(), itStart->second.shot);
				}
			}
			nLeft = nRight;
			coords.x = nRight;
			nStartExp = nEndExp;
		}
	}
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

	int viewLine = GetViewLineForScreen(nLineIndex);
	if (m_pMainFrame->m_bCollapsed)
	{
		if (m_pViewData->GetHideState(viewLine) == HIDESTATE_MARKER)
		{
			COLORREF crBkgnd, crText;
			CDiffColors::GetInstance().GetColors(DIFFSTATE_UNKNOWN, crBkgnd, crText);
			pDC->FillSolidRect(rc, crBkgnd);

			const int THICKNESS = 2;
			COLORREF rectcol = GetSysColor(COLOR_WINDOWTEXT);
			pDC->FillSolidRect(rc.left, rc.top + (rc.Height()/2), rc.Width(), THICKNESS, rectcol);
			pDC->SetTextColor(GetSysColor(COLOR_GRAYTEXT));
			pDC->SetBkColor(crBkgnd);
			CRect rect = rc;
			pDC->DrawText(_T("{...}"), &rect, DT_NOPREFIX|DT_SINGLELINE|DT_CENTER);
			return;
		}
	}

	DiffStates diffState = m_pViewData->GetState(viewLine);
	COLORREF crBkgnd, crText;
	CDiffColors::GetInstance().GetColors(diffState, crBkgnd, crText);

	if (diffState == DIFFSTATE_CONFLICTED)
	{
		// conflicted lines are shown without 'text' on them
		CRect rect = rc;
		pDC->FillSolidRect(rc, crBkgnd);
		// now draw some faint text patterns
		pDC->SetTextColor(CAppUtils::IntenseColor(130, crBkgnd));
		pDC->DrawText(m_sConflictedText, rect, DT_LEFT|DT_NOPREFIX|DT_SINGLELINE);
		DrawBlockLine(pDC, rc, nLineIndex);
		return;
	}

	CPoint origin(rc.left - m_nOffsetChar * GetCharWidth(), rc.top);
	CString sLine = GetLineChars(nLineIndex);
	if (sLine.IsEmpty())
	{
		pDC->FillSolidRect(rc, crBkgnd);
		DrawBlockLine(pDC, rc, nLineIndex);
		DrawLineEnding(pDC, rc, nLineIndex, origin);
		return;
	}

	CheckOtherView();

	// Draw the line

	pDC->SelectObject(GetFont(FALSE, FALSE));

	DrawTextLine(pDC, rc, nLineIndex, origin);

	// draw white space after the end of line
	CRect frect = rc;
	if (origin.x > frect.left)
		frect.left = origin.x;
	if (frect.right > frect.left)
		pDC->FillSolidRect(frect, crBkgnd);

	// draw the whitespace chars
	LPCTSTR pszChars = (LPCWSTR)sLine;
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
	if (origin.x >= rc.left)
		DrawLineEnding(pDC, rc, nLineIndex, origin);
}

void CBaseView::ExpandChars(const CString &sLine, int nOffset, int nCount, CString &line)
{
	if (nCount <= 0)
	{
		line = _T("");
		return;
	}

	int nTabSize = GetTabSize();

	int nActualOffset = CountExpandedChars(sLine, nOffset);

	LPCTSTR pszChars = (LPCWSTR)sLine;
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

CString CBaseView::ExpandChars(const CString &sLine, int nOffset)
{
	CString sRet;
	int nLength = sLine.GetLength();
	ExpandChars(sLine, nOffset, nLength, sRet);
	return sRet;
}

int CBaseView::CountExpandedChars(const CString &sLine, int nLength)
{
	int nTabSize = GetTabSize();

	int nActualOffset = 0;
	for (int i=0; i<nLength; i++)
	{
		if (sLine[i] == _T('\t'))
			nActualOffset += (nTabSize - nActualOffset % nTabSize);
		else
			nActualOffset ++;
	}
	return nActualOffset;
}

void CBaseView::ScrollAllToLine(int nNewTopLine, BOOL bTrackScrollBar)
{
	if (m_pwndLeft)
		m_pwndLeft->ScrollToLine(nNewTopLine, bTrackScrollBar);
	if (m_pwndRight)
		m_pwndRight->ScrollToLine(nNewTopLine, bTrackScrollBar);
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
	if (nNewTopLine >= (int)m_Screen2View.size())
		nNewTopLine = (int)m_Screen2View.size()-1;
	if (bAll)
		ScrollAllToLine(nNewTopLine);
	else
		ScrollToLine(nNewTopLine);
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
	if ((m_pFindDialog)&&(!m_pFindDialog->IsTerminating()))
	{
		m_pFindDialog->SendMessage(WM_CLOSE);
		return;
	}
	CView::OnDestroy();
	DeleteFonts();
	ReleaseBitmap();
}

void CBaseView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	ReleaseBitmap();

	m_nScreenLines = -1;
	m_nScreenChars = -1;
	if (m_nLastScreenChars != GetScreenChars())
	{
		BuildAllScreen2ViewVector();
		m_nLastScreenChars = m_nScreenChars;
		if (m_pMainFrame && m_pMainFrame->m_bWrapLines)
		{
			// if we're in wrap mode, the line wrapping most likely changed
			// and that means we have to redraw the whole window, not just the
			// scrolled part.
			Invalidate(FALSE);
		}
		else
		{
			// make sure the view header is redrawn
			CRect rcScroll;
			GetClientRect(&rcScroll);
			rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
			InvalidateRect(&rcScroll, FALSE);
		}
	}
	else
	{
		// make sure the view header is redrawn
		CRect rcScroll;
		GetClientRect(&rcScroll);
		rcScroll.bottom = GetLineHeight()+HEADERHEIGHT;
		InvalidateRect(&rcScroll, FALSE);

		UpdateLocator();
		RecalcVertScrollBar();
		RecalcHorzScrollBar();
	}
	UpdateCaret();
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

void CBaseView::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (m_pwndLeft)
		m_pwndLeft->OnDoMouseHWheel(nFlags, zDelta, pt);
	if (m_pwndRight)
		m_pwndRight->OnDoMouseHWheel(nFlags, zDelta, pt);
	if (m_pwndBottom)
		m_pwndBottom->OnDoMouseHWheel(nFlags, zDelta, pt);
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::OnDoMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
	bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
	bool bShift = !!(GetKeyState(VK_SHIFT)&0x8000);

	if (bControl || bShift)
	{
		if (m_pMainFrame->m_bWrapLines)
			return;
		// Ctrl-Wheel scrolls sideways
		ScrollSide(-zDelta/30);
	}
	else
	{
		ScrollVertical(zDelta);
	}
}

void CBaseView::OnDoMouseHWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
	bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
	bool bShift = !!(GetKeyState(VK_SHIFT)&0x8000);

	if (bControl || bShift)
	{
		ScrollVertical(zDelta);
	}
	else
	{
		if (m_pMainFrame->m_bWrapLines)
			return;
		// Ctrl-Wheel scrolls sideways
		ScrollSide(-zDelta/30);
	}
}

BOOL CBaseView::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (nHitTest == HTCLIENT)
	{
		if ((m_pViewData)&&(m_pMainFrame->m_bCollapsed))
		{
			if (m_nMouseLine < (int)m_Screen2View.size())
			{
				if (m_nMouseLine >= 0)
				{
					int viewLine = GetViewLineForScreen(m_nMouseLine);
					if (viewLine < m_pViewData->GetCount())
					{
						if (m_pViewData->GetHideState(viewLine) == HIDESTATE_MARKER)
						{
							::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
							return TRUE;
						}
					}
				}
			}
		}
		if (m_mouseInMargin)
		{
			::SetCursor(m_margincursor);
			return TRUE;
		}
		if (m_nMouseLine >= 0)
		{
			::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM)));    // Set To Edit Cursor
			return TRUE;
		}

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

void CBaseView::OnContextMenu(CPoint point, DiffStates state)
{
	if (!this->IsWindowVisible())
		return;

	CIconMenu popup;
	if (!popup.CreatePopupMenu())
		return;

	AddContextItems(popup, state);

	CompensateForKeyboard(point);

	int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);
	ResetUndoStep();
	switch (cmd)
	{
	// 2-pane view commands; target is right view
	case POPUPCOMMAND_USELEFTBLOCK:
		m_pwndRight->UseLeftBlock();
		break;
	case POPUPCOMMAND_USELEFTFILE:
		m_pwndRight->UseLeftFile();
		break;
	case POPUPCOMMAND_USEBOTHLEFTFIRST:
		m_pwndRight->UseBothLeftFirst();
		break;
	case POPUPCOMMAND_USEBOTHRIGHTFIRST:
		m_pwndRight->UseBothRightFirst();
		break;
	// 3-pane view commands; target is bottom view
	case POPUPCOMMAND_USEYOURANDTHEIRBLOCK:
		m_pwndBottom->UseBothRightFirst();
		break;
	case POPUPCOMMAND_USETHEIRANDYOURBLOCK:
		m_pwndBottom->UseBothLeftFirst();
		break;
	case POPUPCOMMAND_USEYOURBLOCK:
		m_pwndBottom->UseRightBlock();
		break;
	case POPUPCOMMAND_USEYOURFILE:
		m_pwndBottom->UseRightFile();
		break;
	case POPUPCOMMAND_USETHEIRBLOCK:
		m_pwndBottom->UseLeftBlock();
		break;
	case POPUPCOMMAND_USETHEIRFILE:
		m_pwndBottom->UseLeftFile();
		break;
	// copy, cut and paste commands
	case ID_EDIT_COPY:
		OnEditCopy();
		break;
	case ID_EDIT_CUT:
		OnEditCut();
		break;
	case ID_EDIT_PASTE:
		OnEditPaste();
		break;
	default:
		return;
	} // switch (cmd)
	SaveUndoStep(); // all except copy, cut  paste save undo step, but this should not be harmful as step is empty already and thus not saved
	return;
}

void CBaseView::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if (!m_pViewData)
		return;

	int nViewBlockStart = -1;
	int nViewBlockEnd = -1;
	GetViewSelection(nViewBlockStart, nViewBlockEnd);
	if ((point.x >= 0) && (point.y >= 0))
	{
		int nLine = GetLineFromPoint(point)-1;
		if ((nLine >= 0) && (nLine < m_Screen2View.size()))
		{
			int nViewLine = GetViewLineForScreen(nLine);
			if (((nViewLine < nViewBlockStart) || (nViewBlockEnd < nViewLine)))
			{
				ClearSelection(); // Clear text-copy selection

				nViewBlockStart = nViewLine;
				nViewBlockEnd = nViewLine;
				DiffStates state = m_pViewData->GetState(nViewLine);
				while (nViewBlockStart > 0)
				{
					const DiffStates lineState = m_pViewData->GetState(nViewBlockStart-1);
					if (!LinesInOneChange(-1, state, lineState))
						break;
					nViewBlockStart--;
				}

				while (nViewBlockEnd < (m_pViewData->GetCount()-1))
				{
					const DiffStates lineState = m_pViewData->GetState(nViewBlockEnd+1);
					if (!LinesInOneChange(1, state, lineState))
						break;
					nViewBlockEnd++;
				}

				SetupAllViewSelection(nViewBlockStart, nViewBlockEnd);
				UpdateCaretPosition(point);
			}
		}
	}

	// FixSelection(); fix selection range
	/*if (m_nSelBlockEnd >= m_pViewData->GetCount())
		m_nSelBlockEnd = m_pViewData->GetCount()-1;//*/

	DiffStates state = DIFFSTATE_UNKNOWN;
	if (GetViewSelection(nViewBlockStart, nViewBlockEnd))
	{
		// find a more 'relevant' state in the selection
		for (int i=nViewBlockStart; i<=nViewBlockEnd; ++i)
		{
			state = m_pViewData->GetState(i);
			if ((state != DIFFSTATE_NORMAL) && (state != DIFFSTATE_UNKNOWN))
				break;
		}
	}
	OnContextMenu(point, state);
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
	SetCaretToFirstViewLine();
	SelectNextBlock(1, false, false);
}

void CBaseView::GoToFirstConflict()
{
	SetCaretToFirstViewLine();
	SelectNextBlock(1, true, false);
}

void CBaseView::HighlightLines(int nStart, int nEnd /* = -1 */)
{
	ClearSelection();
	SetupAllSelection(nStart, max(nStart, nEnd));

	UpdateCaretPosition(SetupPoint(0, nStart));
	Invalidate();
}

void CBaseView::HighlightViewLines(int nStart, int nEnd /* = -1 */)
{
	ClearSelection();
	SetupAllViewSelection(nStart, max(nStart, nEnd));

	UpdateCaretViewPosition(SetupPoint(0, nStart));
	Invalidate();
}

void CBaseView::SetupAllViewSelection(int start, int end)
{
	SetupViewSelection(m_pwndBottom, start, end);
	SetupViewSelection(m_pwndLeft, start, end);
	SetupViewSelection(m_pwndRight, start, end);
}

void CBaseView::SetupAllSelection(int start, int end)
{
	SetupAllViewSelection(GetViewLineForScreen(start), GetViewLineForScreen(end));
}

//void CBaseView::SetupSelection(CBaseView* view, int start, int end) { }

void CBaseView::SetupSelection(int start, int end)
{
	SetupViewSelection(GetViewLineForScreen(start), GetViewLineForScreen(end));
}

void CBaseView::SetupViewSelection(CBaseView* view, int start, int end)
{
	if (!IsViewGood(view))
		return;
	view->SetupViewSelection(start, end);
}

void CBaseView::SetupViewSelection(int start, int end)
{
	// clear text selection before setting line selection ?
	m_nSelViewBlockStart = start;
	m_nSelViewBlockEnd = end;
	Invalidate();
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

bool CBaseView::HasNextConflict()
{
	return SelectNextBlock(1, true, true, true);
}

bool CBaseView::HasPrevConflict()
{
	return SelectNextBlock(-1, true, true, true);
}

bool CBaseView::HasNextDiff()
{
	return SelectNextBlock(1, false, true, true);
}

bool CBaseView::HasPrevDiff()
{
	return SelectNextBlock(-1, false, true, true);
}

bool CBaseView::LinesInOneChange(int direction,
	 DiffStates initialLineState, DiffStates currentLineState)
{
	// Checks whether all the adjacent lines starting from the initial line
	// and up to the current line form the single change

	// Do not distinguish between moved and added/removed lines
	if (initialLineState == DIFFSTATE_MOVED_TO)
		initialLineState = DIFFSTATE_ADDED;
	if (initialLineState == DIFFSTATE_MOVED_FROM)
		initialLineState = DIFFSTATE_REMOVED;
	if (currentLineState == DIFFSTATE_MOVED_TO)
		currentLineState = DIFFSTATE_ADDED;
	if (currentLineState == DIFFSTATE_MOVED_FROM)
		currentLineState = DIFFSTATE_REMOVED;

	// First of all, if the two lines have identical states, they surely
	// belong to one change.
	if (initialLineState == currentLineState)
		return true;

	// Either we move down and initial line state is "added" or "removed" and
	// current line state is "empty"...
	if (direction > 0)
	{
		if (currentLineState == DIFFSTATE_EMPTY)
		{
			if (initialLineState == DIFFSTATE_ADDED || initialLineState == DIFFSTATE_REMOVED)
				return true;
		}
		if (initialLineState == DIFFSTATE_CONFLICTADDED && currentLineState == DIFFSTATE_CONFLICTEMPTY)
			return true;
	}
	// ...or we move up and initial line state is "empty" and current line
	// state is "added" or "removed".
	if (direction < 0)
	{
		if (initialLineState == DIFFSTATE_EMPTY)
		{
			if (currentLineState == DIFFSTATE_ADDED || currentLineState == DIFFSTATE_REMOVED)
				return true;
		}
		if (initialLineState == DIFFSTATE_CONFLICTEMPTY && currentLineState == DIFFSTATE_CONFLICTADDED)
			return true;
	}
	return false;
}

bool CBaseView::SelectNextBlock(int nDirection, bool bConflict, bool bSkipEndOfCurrentBlock /* = true */, bool dryrun /* = false */)
{
	if (! m_pViewData)
		return false;

	const int linesCount = (int)m_Screen2View.size();
	if(linesCount == 0)
		return false;

	int nCenterPos = GetCaretPosition().y;
	int nLimit = -1;
	if (nDirection > 0)
		nLimit = linesCount;

	if (nCenterPos >= linesCount)
		nCenterPos = linesCount-1;

	if (bSkipEndOfCurrentBlock)
	{
		// Find end of current block
		const DiffStates state = m_pViewData->GetState(GetViewLineForScreen(nCenterPos));
		while (nCenterPos != nLimit)
		{
			const DiffStates lineState = m_pViewData->GetState(GetViewLineForScreen(nCenterPos));
			if (!LinesInOneChange(nDirection, state, lineState))
				break;
			nCenterPos += nDirection;
		}
	}

	// Find next diff/conflict block
	while (nCenterPos != nLimit)
	{
		DiffStates linestate = m_pViewData->GetState(GetViewLineForScreen(nCenterPos));
		if (!bConflict &&
			(linestate != DIFFSTATE_NORMAL) &&
			(linestate != DIFFSTATE_UNKNOWN))
		{
			break;
		}
		if (bConflict &&
			((linestate == DIFFSTATE_CONFLICTADDED) ||
			 (linestate == DIFFSTATE_CONFLICTED_IGNORED) ||
			 (linestate == DIFFSTATE_CONFLICTED) ||
			 (linestate == DIFFSTATE_CONFLICTEMPTY)))
		{
			break;
		}

		nCenterPos += nDirection;
	}
	if (nCenterPos == nLimit)
		return false;
	if (dryrun)
		return (nCenterPos != nLimit);

	// Find end of new block
	DiffStates state = m_pViewData->GetState(GetViewLineForScreen(nCenterPos));
	int nBlockEnd = nCenterPos;
	const int maxAllowedLine = nLimit-nDirection;
	while (nBlockEnd != maxAllowedLine)
	{
		const int lineIndex = nBlockEnd + nDirection;
		if (lineIndex >= linesCount)
			break;
		DiffStates lineState = m_pViewData->GetState(GetViewLineForScreen(lineIndex));
		if (!LinesInOneChange(nDirection, state, lineState))
			break;
		nBlockEnd += nDirection;
	}

	int nTopPos = nCenterPos - (GetScreenLines()/2);
	if (nTopPos < 0)
		nTopPos = 0;

	POINT ptCaretPos = {0, nCenterPos};
	SetCaretPosition(ptCaretPos);
	ClearSelection();
	if (nDirection > 0)
		SetupAllSelection(nCenterPos, nBlockEnd);
	else
		SetupAllSelection(nBlockEnd, nCenterPos);

	ScrollAllToLine(nTopPos, FALSE);
	RecalcAllVertScrollBars(TRUE);
	SetCaretToLineStart();
	EnsureCaretVisible();
	OnNavigateNextinlinediff();

	UpdateViewsCaretPosition();
	UpdateCaret();
	ShowDiffLines(nCenterPos);
	return true;
}

BOOL CBaseView::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	if (pNMHDR->idFrom != (UINT)m_hWnd)
		return FALSE;

	CString strTipText;
	strTipText = m_sWindowName + _T("\r\n") + m_sFullFilePath;

	DWORD pos = GetMessagePos();
	CPoint point(GET_X_LPARAM(pos), GET_Y_LPARAM(pos));
	ScreenToClient(&point);
	const int nLine = GetButtonEventLineIndex(point);

	if (nLine >= 0)
	{
		int nViewLine = GetViewLineForScreen(nLine);
		if((m_pViewData)&&(nViewLine < m_pViewData->GetCount()))
		{
			if (m_pViewData->GetState(nViewLine)==DIFFSTATE_MOVED_FROM)
			{
				strTipText.Format(IDS_MOVED_TO_TT, m_pViewData->GetMovedIndex(nViewLine)+1);
			}
			if (m_pViewData->GetState(nViewLine)==DIFFSTATE_MOVED_TO)
			{
				strTipText.Format(IDS_MOVED_FROM_TT, m_pViewData->GetMovedIndex(nViewLine)+1);
			}
		}
	}


	*pResult = 0;
	if (strTipText.IsEmpty())
		return TRUE;

	// need to handle both ANSI and UNICODE versions of the message
	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
	}
	else
	{
		TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
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
	int marginwidth = MARGINWIDTH;
	if ((m_bViewLinenumbers)&&(m_pViewData)&&(m_pViewData->GetCount())&&(m_nDigits > 0))
	{
		marginwidth = (MARGINWIDTH + (m_nDigits * m_nCharWidth) + 2);
	}
	CRect borderrect(rcClient.left, rcClient.top+m_nLineHeight+HEADERHEIGHT, marginwidth, rcClient.bottom);

	if (textrect.PtInRect(point) || borderrect.PtInRect(point))
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

		return (textrect.PtInRect(point) ? 1 : 2);
	}

	return -1;
}

void CBaseView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
	bool bShift = !!(GetKeyState(VK_SHIFT)&0x8000);

	switch (nChar)
	{
	case VK_TAB:
		if ((nChar == '\t') && bControl)
		{
			if (this==m_pwndLeft)
			{
				if (IsViewGood(m_pwndRight))
				{
					m_pwndRight->SetFocus();
				}
				else if (IsViewGood(m_pwndBottom))
				{
					m_pwndBottom->SetFocus();
				}
			}
			else if (this==m_pwndRight)
			{
				if (IsViewGood(m_pwndBottom))
				{
					m_pwndBottom->SetFocus();
				}
				else if (IsViewGood(m_pwndLeft))
				{
					m_pwndLeft->SetFocus();
				}
			}
			else if (this==m_pwndBottom)
			{
				if (IsViewGood(m_pwndLeft))
				{
					m_pwndLeft->SetFocus();
				}
				else if (IsViewGood(m_pwndRight))
				{
					m_pwndRight->SetFocus();
				}
			}
		}
		break;
	case VK_PRIOR:
		{
			POINT ptCaretPos = GetCaretPosition();
			ptCaretPos.y -= GetScreenLines();
			ptCaretPos.y = max(ptCaretPos.y, 0);
			ptCaretPos.x = CalculateCharIndex(ptCaretPos.y, m_nCaretGoalPos);
			SetCaretPosition(ptCaretPos);
			OnCaretMove(MOVELEFT, bShift);
			ShowDiffLines(ptCaretPos.y);
		}
		break;
	case VK_NEXT:
		{
			POINT ptCaretPos = GetCaretPosition();
			ptCaretPos.y += GetScreenLines();
			if (ptCaretPos.y >= GetLineCount())
				ptCaretPos.y = GetLineCount()-1;
			ptCaretPos.x = CalculateCharIndex(ptCaretPos.y, m_nCaretGoalPos);
			SetCaretPosition(ptCaretPos);
			OnCaretMove(MOVERIGHT, bShift);
			ShowDiffLines(ptCaretPos.y);
		}
		break;
	case VK_HOME:
		{
			if (bControl)
			{
				ScrollAllToLine(0);
				SetCaretToViewStart();
				m_nCaretGoalPos = 0;
				if (bShift)
					AdjustSelection(MOVELEFT);
				else
					ClearSelection();
				UpdateCaret();
			}
			else
			{
				SetCaretToLineStart();
				m_nCaretGoalPos = 0;
				OnCaretMove(MOVERIGHT, bShift);
				ScrollAllToChar(0);
			}
		}
		break;
	case VK_END:
		{
			if (bControl)
			{
				ScrollAllToLine(GetLineCount()-GetAllMinScreenLines());
				POINT ptCaretPos;
				ptCaretPos.y = GetLineCount()-1;
				ptCaretPos.x = GetLineLength(ptCaretPos.y);
				SetCaretAndGoalPosition(ptCaretPos);
				if (bShift)
					AdjustSelection(MOVERIGHT);
				else
					ClearSelection();
			}
			else
			{
				POINT ptCaretPos = GetCaretPosition();
				ptCaretPos.x = GetLineLength(ptCaretPos.y);
				if ((GetSubLineOffset(ptCaretPos.y) != -1) && (GetSubLineOffset(ptCaretPos.y) != CountMultiLines(GetViewLineForScreen(ptCaretPos.y))-1)) // not last screen line of view line
				{
					ptCaretPos.x--;
				}
				SetCaretAndGoalPosition(ptCaretPos);
				OnCaretMove(bShift);
			}
		}
		break;
	case VK_BACK:
		if (IsWritable())
		{
			if (! HasTextSelection())
			{
				POINT ptCaretPos = GetCaretPosition();
				if (ptCaretPos.y == 0 && ptCaretPos.x == 0)
					break;
				m_ptSelectionViewPosEnd = GetCaretViewPosition();
				if (bControl)
					MoveCaretWordLeft();
				else
				{
					while (MoveCaretLeft() && IsViewLineEmpty(GetCaretViewPosition().y))
					{
					}
				}
				m_ptSelectionViewPosStart = GetCaretViewPosition();
			}
			RemoveSelectedText();
		}
		break;
	case VK_DELETE:
		if (IsWritable())
		{
			if (! HasTextSelection())
			{
				if (bControl)
				{
					m_ptSelectionViewPosStart = GetCaretViewPosition();
					MoveCaretWordRight();
					m_ptSelectionViewPosEnd = GetCaretViewPosition();
				}
				else
				{
					if (! MoveCaretRight())
						break;
					m_ptSelectionViewPosEnd = GetCaretViewPosition();
					MoveCaretLeft();
					m_ptSelectionViewPosStart = GetCaretViewPosition();
				}
			}
			RemoveSelectedText();
		}
		break;
	}
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CBaseView::OnLButtonDown(UINT nFlags, CPoint point)
{
	const int nClickedLine = GetButtonEventLineIndex(point);
	if ((nClickedLine >= m_nTopLine)&&(nClickedLine < GetLineCount()))
	{
		POINT ptCaretPos;
		ptCaretPos.y = nClickedLine;
		LONG xpos = point.x - GetMarginWidth();
		LONG xpos2 = xpos / GetCharWidth();
		xpos2 += m_nOffsetChar;
		if ((xpos % GetCharWidth()) >= (GetCharWidth()/2))
			xpos2++;
		ptCaretPos.x = CalculateCharIndex(ptCaretPos.y, xpos2);
		SetCaretAndGoalPosition(ptCaretPos);

		if (nFlags & MK_SHIFT)
			AdjustSelection(MOVERIGHT);
		else
		{
			ClearSelection();
			SetupAllSelection(ptCaretPos.y, ptCaretPos.y);
			if (point.x < GetMarginWidth())
			{
				// select the whole line
				m_ptSelectionViewPosStart = m_ptSelectionViewPosEnd = GetCaretViewPosition();
				m_ptSelectionViewPosEnd.x = GetViewLineLength(m_ptSelectionViewPosEnd.y);
			}
		}

		UpdateViewsCaretPosition();
		Invalidate();
	}

	CView::OnLButtonDown(nFlags, point);
}

enum ECharGroup { // ordered by priority low-to-hi
	CHG_UNKNOWN,
	CHG_CONTROL, // x00-x08, x0a-x1f
	CHG_WHITESPACE, // space tab
	CHG_PUNCTUATION, // 0x21-2f, x3a-x40, x5b-x60, x7b-x7f .,:;!?(){}[]/\<> ...
	CHG_WORDLETTER, // alpha num _ (others)
};

ECharGroup GetCharGroup(wchar_t zChar)
{
	if (zChar == ' ' || zChar == '\t' )
	{
		return CHG_WHITESPACE;
	}
	if (zChar < 0x20)
	{
		return CHG_CONTROL;
	}
	if ((zChar >= 0x21 && zChar <= 0x2f)
			|| (zChar >= 0x3a && zChar <= 0x40)
			|| (zChar >= 0x5b && zChar <= 0x5e)
			|| (zChar == 0x60)
			|| (zChar >= 0x7b && zChar <= 0x7f))
	{
		return CHG_PUNCTUATION;
	}
	return CHG_WORDLETTER;
}

void CBaseView::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	if (m_pViewData == 0) {
		CView::OnLButtonDblClk(nFlags, point);
		return;
	}

	const int nClickedLine = GetButtonEventLineIndex(point);
	if ( nClickedLine < 0)
		return;
	int nViewLine = GetViewLineForScreen(nClickedLine);
	if (point.x < GetMarginWidth())  // only if double clicked on the margin
	{
		if((nViewLine < m_pViewData->GetCount())) // a double click on moved line scrolls to corresponding line
		{
			if((m_pViewData->GetState(nViewLine)==DIFFSTATE_MOVED_FROM)||
				(m_pViewData->GetState(nViewLine)==DIFFSTATE_MOVED_TO))
			{
				int movedindex = m_pViewData->GetMovedIndex(nViewLine);
				int screenLine = FindViewLineNumber(movedindex);
				int nTop = screenLine - GetScreenLines()/2;
				if (nTop < 0)
					nTop = 0;
				ScrollAllToLine(nTop);
				// find and select the whole moved block
				int startSel = movedindex;
				int endSel = movedindex;
				while ((startSel > 0) && ((m_pOtherViewData->GetState(startSel) == DIFFSTATE_MOVED_FROM) || (m_pOtherViewData->GetState(startSel) == DIFFSTATE_MOVED_TO)))
					startSel--;
				startSel++;
				while ((endSel < GetLineCount()) && ((m_pOtherViewData->GetState(endSel) == DIFFSTATE_MOVED_FROM) || (m_pOtherViewData->GetState(endSel) == DIFFSTATE_MOVED_TO)))
					endSel++;
				endSel--;
				m_pOtherView->SetupSelection(startSel, endSel);
				return CView::OnLButtonDblClk(nFlags, point);
			}
		}
	}
	if ((m_pMainFrame->m_bCollapsed)&&(m_pViewData->GetHideState(nViewLine) == HIDESTATE_MARKER))
	{
		// a double click on a marker expands the hidden text
		int i = nViewLine;
		while ((i < m_pViewData->GetCount())&&(m_pViewData->GetHideState(i) != HIDESTATE_SHOWN))
		{
			if ((m_pwndLeft)&&(m_pwndLeft->m_pViewData))
				m_pwndLeft->m_pViewData->SetLineHideState(i, HIDESTATE_SHOWN);
			if ((m_pwndRight)&&(m_pwndRight->m_pViewData))
				m_pwndRight->m_pViewData->SetLineHideState(i, HIDESTATE_SHOWN);
			if ((m_pwndBottom)&&(m_pwndBottom->m_pViewData))
				m_pwndBottom->m_pViewData->SetLineHideState(i, HIDESTATE_SHOWN);
			i++;
		}
		BuildAllScreen2ViewVector();
		if (m_pwndLeft)
			m_pwndLeft->Invalidate();
		if (m_pwndRight)
			m_pwndRight->Invalidate();
		if (m_pwndBottom)
			m_pwndBottom->Invalidate();
	}
	else
	{
		POINT ptCaretPos;
		ptCaretPos.y = nClickedLine;
		ptCaretPos.x = CalculateCharIndex(ptCaretPos.y, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
		SetCaretPosition(ptCaretPos);
		ClearSelection();

		POINT ptViewCarret = GetCaretViewPosition();
		nViewLine = ptViewCarret.y;
		if (nViewLine >= GetViewCount())
			return;
		CString sLine = GetViewLine(nViewLine);
		int nLineLength = sLine.GetLength();
		int nBasePos = ptViewCarret.x;
		// get target char group
		ECharGroup eLeft = CHG_UNKNOWN;
		if (nBasePos > 0)
		{
			eLeft = GetCharGroup(sLine[nBasePos-1]);
		}
		ECharGroup eRight = CHG_UNKNOWN;
		if (nBasePos < nLineLength)
		{
			eRight = GetCharGroup(sLine[nBasePos]);
		}
		ECharGroup eTarget = max(eRight, eLeft);
		// find left margin
		int nLeft = nBasePos;
		while (nLeft > 0 && GetCharGroup(sLine[nLeft-1]) == eTarget)
		{
			nLeft--;
		}
		// get right margin
		int nRight = nBasePos;
		while (nRight < nLineLength && GetCharGroup(sLine[nRight]) == eTarget)
		{
			nRight++;
		}
		// set selection
		m_ptSelectionViewPosStart.x = nLeft;
		m_ptSelectionViewPosStart.y = nViewLine;
		m_ptSelectionViewPosEnd.x = nRight;
		m_ptSelectionViewPosEnd.y = nViewLine;
		m_ptSelectionViewPosOrigin = m_ptSelectionViewPosStart;
		SetupAllViewSelection(nViewLine, nViewLine);
		// set caret
		ptCaretPos = ConvertViewPosToScreen(m_ptSelectionViewPosEnd);
		UpdateViewsCaretPosition();
		UpdateGoalPos();

		// set mark word
		m_sPreviousMarkedWord = m_sMarkedWord; // store marked word to recall in case of triple click
		int nMarkWidth = max(nRight - nLeft, 0);
		m_sMarkedWord = sLine.Mid(m_ptSelectionViewPosStart.x, nMarkWidth).Trim();
		if (m_sMarkedWord.Compare(m_sPreviousMarkedWord) == 0)
		{
			m_sMarkedWord.Empty();
		}

		if (m_pwndLeft)
			m_pwndLeft->SetMarkedWord(m_sMarkedWord);
		if (m_pwndRight)
			m_pwndRight->SetMarkedWord(m_sMarkedWord);
		if (m_pwndBottom)
			m_pwndBottom->SetMarkedWord(m_sMarkedWord);

		Invalidate();
		if (m_pwndLocator)
			m_pwndLocator->Invalidate();
	}

	CView::OnLButtonDblClk(nFlags, point);
}

void CBaseView::OnLButtonTrippleClick( UINT /*nFlags*/, CPoint point )
{
	const int nClickedLine = GetButtonEventLineIndex(point);
	if (((point.y - HEADERHEIGHT) / GetLineHeight()) <= 0)
	{
		if (!m_sConvertedFilePath.IsEmpty() && (GetKeyState(VK_CONTROL)&0x8000))
		{
			PCIDLIST_ABSOLUTE __unaligned pidl = ILCreateFromPath((LPCTSTR)m_sConvertedFilePath);
			if (pidl)
			{
				SHOpenFolderAndSelectItems(pidl,0,0,0);
				CoTaskMemFree((LPVOID)pidl);
			}
		}
		return;
	}
	POINT ptCaretPos;
	ptCaretPos.y = nClickedLine;
	ptCaretPos.x = CalculateCharIndex(ptCaretPos.y, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
	SetCaretAndGoalPosition(ptCaretPos);
	m_sMarkedWord = m_sPreviousMarkedWord;     // recall previous Marked word
	if (m_pwndLeft)
		m_pwndLeft->SetMarkedWord(m_sMarkedWord);
	if (m_pwndRight)
		m_pwndRight->SetMarkedWord(m_sMarkedWord);
	if (m_pwndBottom)
		m_pwndBottom->SetMarkedWord(m_sMarkedWord);
	ClearSelection();
	m_ptSelectionViewPosStart.x = 0;
	m_ptSelectionViewPosStart.y = nClickedLine;
	m_ptSelectionViewPosEnd.x = GetLineLength(nClickedLine);
	m_ptSelectionViewPosEnd.y = nClickedLine;
	SetupSelection(m_ptSelectionViewPosStart.y, m_ptSelectionViewPosEnd.y);
	UpdateViewsCaretPosition();
	Invalidate();
	if (m_pwndLocator)
		m_pwndLocator->Invalidate();
}

void CBaseView::OnEditCopy()
{
	CString sCopyData = GetSelectedText();

	if (!sCopyData.IsEmpty())
	{
		CStringUtils::WriteAsciiStringToClipboard(sCopyData, m_hWnd);
	}
}

void CBaseView::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_pMainFrame->m_nMoveMovesToIgnore > 0)
	{
		--m_pMainFrame->m_nMoveMovesToIgnore;
		CView::OnMouseMove(nFlags, point);
		return;
	}
	int nMouseLine = GetButtonEventLineIndex(point);
	if (nMouseLine < -1)
		nMouseLine = -1;
	m_mouseInMargin = point.x < GetMarginWidth();

	ShowDiffLines(nMouseLine);

	KillTimer(IDT_SCROLLTIMER);
	if (nFlags & MK_LBUTTON)
	{
		int saveMouseLine = nMouseLine >= 0 ? nMouseLine : 0;
		saveMouseLine = saveMouseLine < GetLineCount() ? saveMouseLine : GetLineCount() - 1;
		if (saveMouseLine < 0)
			return;
		int charIndex = CalculateCharIndex(saveMouseLine, m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth());
		if (HasSelection() &&
			((nMouseLine >= m_nTopLine)&&(nMouseLine < GetLineCount())))
		{
			POINT ptCaretPos = {charIndex, nMouseLine};
			SetCaretAndGoalPosition(ptCaretPos);
			AdjustSelection(MOVERIGHT);
			Invalidate();
			UpdateWindow();
		}
		if (nMouseLine < m_nTopLine)
		{
			ScrollAllToLine(m_nTopLine-1, TRUE);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
		if (nMouseLine >= m_nTopLine + GetScreenLines() - 2)
		{
			ScrollAllToLine(m_nTopLine+1, TRUE);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
		if (!m_pMainFrame->m_bWrapLines && ((m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth()) <= m_nOffsetChar))
		{
			ScrollAllSide(-1);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
		if (!m_pMainFrame->m_bWrapLines && (charIndex >= (GetScreenChars()+m_nOffsetChar-4)))
		{
			ScrollAllSide(1);
			SetTimer(IDT_SCROLLTIMER, 20, NULL);
		}
		SetCapture();
	}


	CView::OnMouseMove(nFlags, point);
}

void CBaseView::OnLButtonUp(UINT nFlags, CPoint point)
{
	ShowDiffLines(-1);
	ReleaseCapture();
	KillTimer(IDT_SCROLLTIMER);

	__super::OnLButtonUp(nFlags, point);
}

void CBaseView::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_SCROLLTIMER)
	{
		POINT point;
		GetCursorPos(&point);
		ScreenToClient(&point);
		int nMouseLine = GetButtonEventLineIndex(point);
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
				ScrollAllToLine(m_nTopLine-1, TRUE);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
			if (nMouseLine >= m_nTopLine + GetScreenLines() - 2)
			{
				ScrollAllToLine(m_nTopLine+1, TRUE);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
			if (!m_pMainFrame->m_bWrapLines && ((m_nOffsetChar + (point.x - GetMarginWidth()) / GetCharWidth()) <= m_nOffsetChar))
			{
				ScrollAllSide(-1);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
			if (!m_pMainFrame->m_bWrapLines && (charIndex >= (GetScreenChars()+m_nOffsetChar-4)))
			{
				ScrollAllSide(1);
				SetTimer(IDT_SCROLLTIMER, 20, NULL);
			}
		}

	}

	CView::OnTimer(nIDEvent);
}

void CBaseView::ShowDiffLines(int nLine)
{
	if ((nLine < m_nTopLine)||(nLine >= GetLineCount()))
	{
		m_pwndLineDiffBar->ShowLines(nLine);
		nLine = -1;
		m_nMouseLine = nLine;
		return;
	}

	if ((!m_pwndRight)||(!m_pwndLeft))
		return;
	if(m_pMainFrame->m_bOneWay)
		return;

	nLine = (nLine > (int)m_pwndRight->m_Screen2View.size() ? -1 : nLine);
	nLine = (nLine > (int)m_pwndLeft->m_Screen2View.size() ? -1 : nLine);

	if (nLine < 0)
		return;

	if (nLine != m_nMouseLine)
	{
		if (nLine >= GetLineCount())
			nLine = -1;
		m_nMouseLine = nLine;
		m_pwndLineDiffBar->ShowLines(nLine);
	}
	m_pMainFrame->m_nMoveMovesToIgnore = MOVESTOIGNORE;
}

const viewdata& CBaseView::GetEmptyLineData()
{
	static const viewdata emptyLine(_T(""), DIFFSTATE_EMPTY, -1, EOL_NOENDING, HIDESTATE_SHOWN, -1);
	return emptyLine;
}

void CBaseView::InsertViewEmptyLines(int nFirstView, int nCount)
{
	for (int i = 0; i < nCount; i++)
	{
		InsertViewData(nFirstView, GetEmptyLineData());
	}
}


void CBaseView::UpdateCaret()
{
	POINT ptCaretPos = GetCaretPosition();
	ptCaretPos.y = std::max<int>(std::min<int>(ptCaretPos.y, GetLineCount()-1), 0);
	ptCaretPos.x = std::max<int>(std::min<int>(ptCaretPos.x, GetLineLength(ptCaretPos.y)), 0);
	SetCaretPosition(ptCaretPos);

	int nCaretOffset = CalculateActualOffset(ptCaretPos);

	if (m_bFocused &&
		ptCaretPos.y >= m_nTopLine &&
		ptCaretPos.y < (m_nTopLine+GetScreenLines()) &&
		nCaretOffset >= m_nOffsetChar &&
		nCaretOffset < (m_nOffsetChar+GetScreenChars()))
	{
		CreateSolidCaret(2, GetLineHeight());
		SetCaretPos(TextToClient(ptCaretPos));
		ShowCaret();
	}
	else
	{
		HideCaret();
	}
}

POINT CBaseView::ConvertScreenPosToView(const POINT& pt)
{
	POINT ptViewPos;
	ptViewPos.x = pt.x;

	int nSubLine = GetSubLineOffset(pt.y);
	if (nSubLine > 0)
	{
		for (int nScreenLine = pt.y-1; nScreenLine >= pt.y-nSubLine; nScreenLine--)
		{
			ptViewPos.x += GetLineChars(nScreenLine).GetLength();
		}
	}

	ptViewPos.y = GetViewLineForScreen(pt.y);
	return ptViewPos;
}

POINT CBaseView::ConvertViewPosToScreen(const POINT& pt)
{
	POINT ptPos;
	int nViewLineLenLeft = GetViewLineLength(pt.y);
	ptPos.x = min(nViewLineLenLeft, pt.x);
	ptPos.y = FindScreenLineForViewLine(pt.y);
	if (GetViewLineForScreen(ptPos.y) != pt.y )
	{
		ptPos.x = 0;
	}
	else if (GetSubLineOffset(ptPos.y) >= 0) // sublined
	{
		int nSubLineLength = GetLineChars(ptPos.y).GetLength();
		while (nSubLineLength < ptPos.x)
		{
			ptPos.x -= nSubLineLength;
			nViewLineLenLeft -= nSubLineLength;
			ptPos.y++;
			nSubLineLength = GetLineChars(ptPos.y).GetLength();
		}
		// last pos of non last sub-line go to start of next screen line
		// Note: while this works correctly, it's not what a user might expect:
		// cursor-right when the caret is before the last char of a wrapped line
		// now moves the caret to the next line. But users expect the caret to
		// move to the right of the last char instead, and with another cursor-right
		// keystroke to move the caret to the next line.
		// Basically, this would require to handle two caret positions for the same
		// logical position in the line string (one on the last position of the first line,
		// one on the first position of the new line. For non-wrapped lines this works
		// because there's an 'invisible' newline char at the end of the first line.
		if (nSubLineLength == ptPos.x && nViewLineLenLeft > nSubLineLength)
		{
			ptPos.x = 0;
			ptPos.y++;
		}
	}

	return ptPos;
}


void CBaseView::EnsureCaretVisible()
{
	POINT ptCaretPos = GetCaretPosition();
	int nCaretOffset = CalculateActualOffset(ptCaretPos);

	if (ptCaretPos.y < m_nTopLine)
		ScrollAllToLine(ptCaretPos.y);
	int screnLines = GetScreenLines();
	if (screnLines)
	{
		if (ptCaretPos.y >= (m_nTopLine+screnLines)-1)
			ScrollAllToLine(ptCaretPos.y-screnLines+2);
		if (nCaretOffset < m_nOffsetChar)
			ScrollAllToChar(nCaretOffset);
		if (nCaretOffset > (m_nOffsetChar+GetScreenChars()-1))
			ScrollAllToChar(nCaretOffset-GetScreenChars()+1);
	}
}

int CBaseView::CalculateActualOffset(const POINT& point)
{
	int nLineIndex = point.y;
	int nCharIndex = point.x;
	ASSERT(nCharIndex >= 0);
	CString sLine = GetLineChars(nLineIndex);
	int nLineLength = sLine.GetLength();
	return CountExpandedChars(sLine, min(nCharIndex, nLineLength));
}

int CBaseView::CalculateCharIndex(int nLineIndex, int nActualOffset)
{
	int nLength = GetLineLength(nLineIndex);
	int nSubLine = GetSubLineOffset(nLineIndex);
	if (nSubLine>=0)
	{
		int nViewLine = GetViewLineForScreen(nLineIndex);
		if ((nViewLine>=0)&&(nViewLine < (int)m_ScreenedViewLine.size()))
		{
			int nMultilineCount = CountMultiLines(nViewLine);
			if ((nMultilineCount>0) && (nSubLine<nMultilineCount-1))
			{
				nLength--;
			}
		}
	}
	CString Line = GetLineChars(nLineIndex);
	int nIndex = 0;
	int nOffset = 0;
	int nTabSize = GetTabSize();
	while (nOffset < nActualOffset && nIndex < nLength)
	{
		if (Line.GetAt(nIndex) == _T('\t'))
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
	int nOffsetScreenLine = max(0, (point.y - m_nTopLine));
	pt.y = nOffsetScreenLine * GetLineHeight();
	pt.x = CalculateActualOffset(point);

	int nLeft = GetMarginWidth() - m_nOffsetChar * GetCharWidth();
	CDC * pDC = GetDC();
	if (pDC)
	{
		pDC->SelectObject(GetFont()); // is this right font ?
		int nScreenLine = nOffsetScreenLine + m_nTopLine;
		CString sLine = GetLineChars(nScreenLine);
		ExpandChars(sLine, 0, std::min<int>(pt.x, sLine.GetLength()), sLine);
		nLeft += pDC->GetTextExtent(sLine, pt.x).cx;
		ReleaseDC(pDC);
	} else {
		nLeft += pt.x * GetCharWidth();
	}

	pt.x =  nLeft;
	pt.y = (pt.y + GetLineHeight() + HEADERHEIGHT);
	return pt;
}

void CBaseView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CView::OnChar(nChar, nRepCnt, nFlags);

	if (IsReadonly())
		return;

	if ((::GetKeyState(VK_LBUTTON) & 0x8000) != 0 ||
		(::GetKeyState(VK_RBUTTON) & 0x8000) != 0)
	{
		return;
	}

	if (!m_pViewData) // no data - nothing to do
		return;

	if (nChar == VK_F16)
	{
		// generated by a ctrl+backspace - ignore.
	}
	else if ((nChar > 31)||(nChar == VK_TAB))
	{
		ResetUndoStep();
		RemoveSelectedText();
		POINT ptCaretViewPos = GetCaretViewPosition();
		int nViewLine = ptCaretViewPos.y;
		if ((nViewLine==0)&&(GetViewCount()==0))
			OnChar(VK_RETURN, 0, 0);
		viewdata lineData = GetViewData(nViewLine);
		lineData.sLine.Insert(ptCaretViewPos.x, (wchar_t)nChar);
		if (IsStateEmpty(lineData.state))
		{
			// if not last line set EOL
			for (int nCheckViewLine = nViewLine+1; nCheckViewLine < GetViewCount(); nCheckViewLine++)
			{
				if (!IsViewLineEmpty(nCheckViewLine))
				{
					lineData.ending = lineendings;
					break;
				}
			}
			// make sure previous (non empty) line have EOL set
			for (int nCheckViewLine = nViewLine-1; nCheckViewLine > 0; nCheckViewLine--)
			{
				if (!IsViewLineEmpty(nCheckViewLine))
				{
					if (GetViewLineEnding(nCheckViewLine) == EOL_NOENDING)
					{
						SetViewLineEnding(nCheckViewLine, lineendings);
					}
					break;
				}
			}
		}
		lineData.state = DIFFSTATE_EDITED;
		bool bNeedRenumber = false;
		if (lineData.linenumber == -1)
		{
			lineData.linenumber = 0;
			bNeedRenumber = true;
		}
		SetViewData(nViewLine, lineData);
		SaveUndoStep();
		BuildAllScreen2ViewVector(nViewLine);
		if (bNeedRenumber)
		{
			UpdateViewLineNumbers();
		}
		MoveCaretRight();
		UpdateGoalPos();
	}
	else if (nChar == 10)
	{
		int nViewLine = GetViewLineForScreen(GetCaretPosition().y);
		EOL eol = m_pViewData->GetLineEnding(nViewLine);
		EOL newEOL = EOL_CRLF;
		switch (eol)
		{
			case EOL_CRLF:
				newEOL = EOL_CR;
				break;
			case EOL_CR:
				newEOL = EOL_LF;
				break;
			case EOL_LF:
				newEOL = EOL_CRLF;
				break;
		}
		if (eol==EOL_NOENDING || eol==newEOL)
			// don't allow to change enter on empty line, or last text line (lines with EOL_NOENDING)
			// to add EOL on newly edited empty line hit enter
			// don't store into UNDO if no change happened
			// and don't mark file as modified
			return;
		AddUndoViewLine(nViewLine);
		m_pViewData->SetLineEnding(nViewLine, newEOL);
		m_pViewData->SetState(nViewLine, DIFFSTATE_EDITED);
		UpdateGoalPos();
	}
	else if (nChar == VK_RETURN)
	{
		// insert a new, fresh and empty line below the cursor
		RemoveSelectedText();

		CUndo::GetInstance().BeginGrouping();

		POINT ptCaretViewPos = GetCaretViewPosition();
		int nViewLine = ptCaretViewPos.y;
		int nLeft = ptCaretViewPos.x;
		CString sLine = GetViewLineChars(nViewLine);
		CString sLineLeft = sLine.Left(nLeft);
		CString sLineRight = sLine.Right(sLine.GetLength() - nLeft);
		EOL eOriginalEnding = EOL_AUTOLINE;
		if (m_pViewData->GetCount() > nViewLine)
			eOriginalEnding = GetViewLineEnding(nViewLine);

		if (!sLineRight.IsEmpty() || (eOriginalEnding!=lineendings))
		{
			viewdata newFirstLine(sLineLeft, DIFFSTATE_EDITED, 1, lineendings, HIDESTATE_SHOWN, -1);
			SetViewData(nViewLine, newFirstLine);
		}

		int nInsertLine = (m_pViewData->GetCount()==0) ? 0 : nViewLine + 1;
		viewdata newLastLine(sLineRight, DIFFSTATE_EDITED, 1, eOriginalEnding, HIDESTATE_SHOWN, -1);
		InsertViewData(nInsertLine, newLastLine);
		SaveUndoStep();

		// adds new line everywhere except me
		if (IsViewGood(m_pwndLeft) && m_pwndLeft!=this)
		{
			m_pwndLeft->InsertViewEmptyLines(nInsertLine, 1);
		}
		if (IsViewGood(m_pwndRight) && m_pwndRight!=this)
		{
			m_pwndRight->InsertViewEmptyLines(nInsertLine, 1);
		}
		if (IsViewGood(m_pwndBottom) && m_pwndBottom!=this)
		{
			m_pwndBottom->InsertViewEmptyLines(nInsertLine, 1);
		}
		SaveUndoStep();

		UpdateViewLineNumbers();
		SaveUndoStep();
		CUndo::GetInstance().EndGrouping();

		BuildAllScreen2ViewVector();
		// move the cursor to the new line
		ptCaretViewPos = SetupPoint(0, nViewLine+1);
		SetCaretAndGoalViewPosition(ptCaretViewPos);
	}
	else
		return; // Unknown control character -- ignore it.
	ClearSelection();
	EnsureCaretVisible();
	UpdateCaret();
	SetModified(true);
	Invalidate(FALSE);
}

void CBaseView::AddUndoViewLine(int nViewLine, bool bAddEmptyLine)
{
	ResetUndoStep();
	m_AllState.left.AddViewLineFromView(m_pwndLeft, nViewLine, bAddEmptyLine);
	m_AllState.right.AddViewLineFromView(m_pwndRight, nViewLine, bAddEmptyLine);
	m_AllState.bottom.AddViewLineFromView(m_pwndBottom, nViewLine, bAddEmptyLine);
	SaveUndoStep();
	RecalcAllVertScrollBars();
	Invalidate(FALSE);
}

void CBaseView::AddEmptyViewLine(int nViewLineIndex)
{
	if (m_pViewData == NULL)
		return;
	int viewLine = nViewLineIndex;
	EOL ending = m_pViewData->GetLineEnding(viewLine);
	if (ending == EOL_NOENDING)
	{
		 ending = lineendings;
	}
	viewdata newLine(_T(""), DIFFSTATE_EDITED, -1, ending, HIDESTATE_SHOWN, -1);
	if (IsTarget()) // TODO: once more wievs will writable this is not correct anymore
	{
		CString sPartLine = GetViewLineChars(nViewLineIndex);
		int nPosx = GetCaretPosition().x; // should be view pos ?
		m_pViewData->SetLine(viewLine, sPartLine.Left(nPosx));
		sPartLine = sPartLine.Mid(nPosx);
		newLine.sLine = sPartLine;
	}
	m_pViewData->InsertData(viewLine+1, newLine);
	BuildAllScreen2ViewVector();
}

void CBaseView::RemoveSelectedText()
{
	if (m_pViewData == NULL)
		return;
	if (!HasTextSelection())
		return;

	// fix selection if starts or ends on empty line
	SetCaretViewPosition(m_ptSelectionViewPosEnd);
	while (IsViewLineEmpty(GetCaretViewPosition().y) && MoveCaretRight())
	{
	}
	m_ptSelectionViewPosEnd = GetCaretViewPosition();
	SetCaretViewPosition(m_ptSelectionViewPosStart);
	while (IsViewLineEmpty(GetCaretViewPosition().y) && MoveCaretRight())
	{
	}
	m_ptSelectionViewPosStart = GetCaretViewPosition();
	if (!HasTextSelection())
	{
		ClearSelection();
		return;
	}

	// We want to undo the insertion in a single step.
	ResetUndoStep();
	CUndo::GetInstance().BeginGrouping();

	// combine first and last line
	viewdata oFirstLine = GetViewData(m_ptSelectionViewPosStart.y);
	viewdata oLastLine = GetViewData(m_ptSelectionViewPosEnd.y);
	oFirstLine.sLine = oFirstLine.sLine.Left(m_ptSelectionViewPosStart.x) + oLastLine.sLine.Mid(m_ptSelectionViewPosEnd.x);
	oFirstLine.ending = oLastLine.ending;
	oFirstLine.state = DIFFSTATE_EDITED;
	SetViewData(m_ptSelectionViewPosStart.y, oFirstLine);

	// clean up middle lines if any
	if (m_ptSelectionViewPosStart.y != m_ptSelectionViewPosEnd.y)
	{
		viewdata oEmptyLine = GetEmptyLineData();
		for (int nViewLine = m_ptSelectionViewPosStart.y+1; nViewLine <= m_ptSelectionViewPosEnd.y; nViewLine++)
		{
			 SetViewData(nViewLine, oEmptyLine);
		}
		SaveUndoStep();

		if (CleanEmptyLines())
		{
			BuildAllScreen2ViewVector(); // schedule full rebuild
		}
		SaveUndoStep();
		UpdateViewLineNumbers();
	}

	SaveUndoStep();
	CUndo::GetInstance().EndGrouping();

	SetModified();
	BuildAllScreen2ViewVector(m_ptSelectionViewPosStart.y, m_ptSelectionViewPosEnd.y);
	SetCaretViewPosition(m_ptSelectionViewPosStart);
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

	ResetUndoStep();

	POINT ptCaretViewPos = GetCaretViewPosition();
	int nLeft = ptCaretViewPos.x;
	int nViewLine = ptCaretViewPos.y;

	if ((nViewLine==0)&&(GetViewCount()==0))
		OnChar(VK_RETURN, 0, 0);

	std::vector<CString> lines;
	int nStart = 0;
	int nEolPos = 0;
	while ((nEolPos = sClipboardText.Find('\r', nEolPos))>=0)
	{
		CString sLine = sClipboardText.Mid(nStart, nEolPos-nStart);
		lines.push_back(sLine);
		nEolPos++;
		nStart = nEolPos;
	}
	CString sLine = sClipboardText.Mid(nStart);
	lines.push_back(sLine);

	int nLinesToPaste = (int)lines.size();
	if (nLinesToPaste > 1)
	{
		// multiline text

		// We want to undo the multiline insertion in a single step.
		CUndo::GetInstance().BeginGrouping();

		sLine = GetViewLineChars(nViewLine);
		CString sLineLeft = sLine.Left(nLeft);
		CString sLineRight = sLine.Right(sLine.GetLength() - nLeft);
		EOL eOriginalEnding = GetViewLineEnding(nViewLine);
		viewdata newLine(_T(""), DIFFSTATE_EDITED, 1, lineendings, HIDESTATE_SHOWN, -1);
		if (!lines[0].IsEmpty() || !sLineRight.IsEmpty() || (eOriginalEnding!=lineendings))
		{
			newLine.sLine = sLineLeft + lines[0];
			SetViewData(nViewLine, newLine);
		}

		int nInsertLine = nViewLine;
		for (int i = 1; i < nLinesToPaste-1; i++)
		{
			newLine.sLine = lines[i];
			InsertViewData(++nInsertLine, newLine);
		}
		newLine.sLine = lines[nLinesToPaste-1] + sLineRight;
		newLine.ending = eOriginalEnding;
		InsertViewData(++nInsertLine, newLine);

		SaveUndoStep();

		// adds new lines everywhere except me
		if (IsViewGood(m_pwndLeft) && m_pwndLeft!=this)
		{
			m_pwndLeft->InsertViewEmptyLines(nViewLine+1, nLinesToPaste-1);
		}
		if (IsViewGood(m_pwndRight) && m_pwndRight!=this)
		{
			m_pwndRight->InsertViewEmptyLines(nViewLine+1, nLinesToPaste-1);
		}
		if (IsViewGood(m_pwndBottom) && m_pwndBottom!=this)
		{
			m_pwndBottom->InsertViewEmptyLines(nViewLine+1, nLinesToPaste-1);
		}
		SaveUndoStep();

		UpdateViewLineNumbers();
		CUndo::GetInstance().EndGrouping();

		ptCaretViewPos = SetupPoint(lines[nLinesToPaste-1].GetLength(), nInsertLine);
	}
	else
	{
		 // single line text - just insert it
		sLine = GetViewLineChars(nViewLine);
		sLine.Insert(nLeft, sClipboardText);
		ptCaretViewPos = SetupPoint(nLeft + sClipboardText.GetLength(), nViewLine);
		SetViewLine(nViewLine, sLine);
		SetViewState(nViewLine, DIFFSTATE_EDITED);
		SaveUndoStep();
	}

	SetModified();
	RefreshViews();
	BuildAllScreen2ViewVector();
	UpdateCaretViewPosition(ptCaretViewPos);
}

void CBaseView::OnCaretDown()
{
	POINT ptCaretPos = GetCaretPosition();
	int nLine = ptCaretPos.y;
	int nNextLine = nLine + 1;
	if (nNextLine >= GetLineCount()) // already at last line
	{
		return;
	}

	POINT ptCaretViewPos = GetCaretViewPosition();
	int nViewLine = ptCaretViewPos.y;
	int nNextViewLine = GetViewLineForScreen(nNextLine);
	if (!((nNextViewLine == nViewLine) && (GetSubLineOffset(nNextLine)<CountMultiLines(nNextViewLine)))) // not on same view line
	{
		// find next suitable screen line
		while ((nNextViewLine == nViewLine) || IsViewLineHidden(nNextViewLine))
		{
			nNextLine++;
			if (nNextLine >= GetLineCount())
			{
				return;
			}
			nNextViewLine = GetViewLineForScreen(nNextLine);
		}
	}
	ptCaretPos.y = nNextLine;
	ptCaretPos.x = CalculateCharIndex(ptCaretPos.y, m_nCaretGoalPos);
	SetCaretPosition(ptCaretPos);
	OnCaretMove(MOVELEFT);
	ShowDiffLines(ptCaretPos.y);
}

bool CBaseView::MoveCaretLeft()
{
	POINT ptCaretViewPos = GetCaretViewPosition();

	//int nViewLine = ptCaretViewPos.y;
	if (ptCaretViewPos.x == 0)
	{
		int nPrevLine = GetCaretPosition().y;
		int nPrevViewLine;
		do {
			nPrevLine--;
			if (nPrevLine < 0)
			{
				return false;
			}
			nPrevViewLine = GetViewLineForScreen(nPrevLine);
		} while ((GetSubLineOffset(nPrevLine) >= CountMultiLines(nPrevViewLine)) || IsViewLineHidden(nPrevViewLine));
		ptCaretViewPos = ConvertScreenPosToView(SetupPoint(GetLineLength(nPrevLine), nPrevLine));
		ShowDiffLines(nPrevLine);
	}
	else
		--ptCaretViewPos.x;

	SetCaretAndGoalViewPosition(ptCaretViewPos);
	return true;
}

bool CBaseView::MoveCaretRight()
{
	POINT ptCaretViewPos = GetCaretViewPosition();

	int nViewLine = ptCaretViewPos.y;
	int nViewLineLen = GetViewLineLength(nViewLine);
	if (ptCaretViewPos.x >= nViewLineLen)
	{
		int nNextLine = GetCaretPosition().y;
		int nNextViewLine;
		do {
			nNextLine++;
			if (nNextLine >= GetLineCount())
			{
				return false;
			}
			nNextViewLine = GetViewLineForScreen(nNextLine);
		} while (nNextViewLine == nViewLine || IsViewLineHidden(nNextViewLine));
		ptCaretViewPos.y = nNextViewLine;
		ptCaretViewPos.x = 0;
		ShowDiffLines(nNextLine);
	}
	else
		++ptCaretViewPos.x;

	SetCaretAndGoalViewPosition(ptCaretViewPos);
	return true;
}

void CBaseView::UpdateGoalPos()
{
	m_nCaretGoalPos = CalculateActualOffset(GetCaretPosition());
}

void CBaseView::OnCaretLeft()
{
	MoveCaretLeft();
	OnCaretMove(MOVELEFT);
}

void CBaseView::OnCaretRight()
{
	MoveCaretRight();
	OnCaretMove(MOVERIGHT);
}

void CBaseView::OnCaretUp()
{
	POINT ptCaretPos = GetCaretPosition();
	int nLine = ptCaretPos.y;
	if (nLine <= 0) // already at first line
	{
		return;
	}
	int nPrevLine = nLine - 1;

	POINT ptCaretViewPos = GetCaretViewPosition();
	int nViewLine = ptCaretViewPos.y;
	int nPrevViewLine = GetViewLineForScreen(nPrevLine);
	if (nPrevViewLine != nViewLine) // not on same view line
	{
		// find previous suitable screen line
		while ((GetSubLineOffset(nPrevLine) >= CountMultiLines(nPrevViewLine)) || IsViewLineHidden(nPrevViewLine))
		{
			if (nPrevLine <= 0)
			{
				return;
			}
			nPrevLine--;
			nPrevViewLine = GetViewLineForScreen(nPrevLine);
		}
	}
	ptCaretPos.y = nPrevLine;
	ptCaretPos.x = CalculateCharIndex(ptCaretPos.y, m_nCaretGoalPos);
	SetCaretPosition(ptCaretPos);
	OnCaretMove(MOVELEFT);
	ShowDiffLines(ptCaretPos.y);
}

bool CBaseView::IsWordSeparator(const wchar_t ch) const
{
	return ch == ' ' || ch == '\t' || (m_sWordSeparators.Find(ch) >= 0);
}

bool CBaseView::IsCaretAtWordBoundary()
{
	POINT ptViewCaret = GetCaretViewPosition();
	CString line = GetViewLineChars(ptViewCaret.y);
	if (line.IsEmpty())
		return false; // no boundary at the empty lines
	if (ptViewCaret.x == 0)
		return !IsWordSeparator(line.GetAt(ptViewCaret.x));
	if (ptViewCaret.x >= GetViewLineLength(ptViewCaret.y))
		return !IsWordSeparator(line.GetAt(ptViewCaret.x - 1));
	return
		IsWordSeparator(line.GetAt(ptViewCaret.x)) !=
		IsWordSeparator(line.GetAt(ptViewCaret.x - 1));
}

void CBaseView::UpdateViewsCaretPosition()
{
	POINT ptCaretPos = GetCaretPosition();
	if (m_pwndBottom && m_pwndBottom!=this)
		m_pwndBottom->UpdateCaretPosition(ptCaretPos);
	if (m_pwndLeft && m_pwndLeft!=this)
		m_pwndLeft->UpdateCaretPosition(ptCaretPos);
	if (m_pwndRight && m_pwndRight!=this)
		m_pwndRight->UpdateCaretPosition(ptCaretPos);
}

void CBaseView::OnCaretWordleft()
{
	MoveCaretWordLeft();
	OnCaretMove(MOVELEFT);
}

void CBaseView::OnCaretWordright()
{
	MoveCaretWordRight();
	OnCaretMove(MOVERIGHT);
}

void CBaseView::MoveCaretWordLeft()
{
	while (MoveCaretLeft() && !IsCaretAtWordBoundary())
	{
	}
}

void CBaseView::MoveCaretWordRight()
{
	while (MoveCaretRight() && !IsCaretAtWordBoundary())
	{
	}
}

void CBaseView::ClearCurrentSelection()
{
	m_ptSelectionViewPosStart = GetCaretViewPosition();
	m_ptSelectionViewPosEnd = m_ptSelectionViewPosStart;
	m_ptSelectionViewPosOrigin = m_ptSelectionViewPosStart;
	m_nSelViewBlockStart = -1;
	m_nSelViewBlockEnd = -1;
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

void CBaseView::AdjustSelection(bool bMoveLeft)
{
	POINT ptCaretViewPos = GetCaretViewPosition();
	if (ArePointsSame(m_ptSelectionViewPosOrigin, SetupPoint(-1, -1)))
	{
		// select all have been used recently update origin
		m_ptSelectionViewPosOrigin = bMoveLeft ? m_ptSelectionViewPosEnd :  m_ptSelectionViewPosStart;
	}
	if ((ptCaretViewPos.y < m_ptSelectionViewPosOrigin.y) ||
		(ptCaretViewPos.y == m_ptSelectionViewPosOrigin.y && ptCaretViewPos.x <= m_ptSelectionViewPosOrigin.x))
	{
		m_ptSelectionViewPosStart = ptCaretViewPos;
		m_ptSelectionViewPosEnd = m_ptSelectionViewPosOrigin;
	}
	else
	{
		m_ptSelectionViewPosStart = m_ptSelectionViewPosOrigin;
		m_ptSelectionViewPosEnd = ptCaretViewPos;
	}

	SetupAllViewSelection(m_ptSelectionViewPosStart.y, m_ptSelectionViewPosEnd.y);

	Invalidate(FALSE);
}

void CBaseView::OnEditCut()
{
	if (IsWritable())
	{
		OnEditCopy();
		RemoveSelectedText();
	}
}

void CBaseView::OnEditPaste()
{
	if (IsWritable())
	{
		CUndo::GetInstance().BeginGrouping();
		RemoveSelectedText();
		PasteText();
		CUndo::GetInstance().EndGrouping();
	}
}

void CBaseView::DeleteFonts()
{
	for (int i=0; i<fontsCount; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
			m_apFonts[i] = NULL;
		}
	}
}

void CBaseView::OnCaretMove(bool bMoveLeft)
{
	bool bShift = !!(GetKeyState(VK_SHIFT)&0x8000);
	OnCaretMove(bMoveLeft, bShift);
}

void CBaseView::OnCaretMove(bool bMoveLeft, bool isShiftPressed)
{
	if(isShiftPressed)
		AdjustSelection(bMoveLeft);
	else
		ClearSelection();
	EnsureCaretVisible();
	UpdateCaret();
}

void CBaseView::AddContextItems(CIconMenu& popup, DiffStates /*state*/)
{
	AddCutCopyAndPaste(popup);
}

void CBaseView::AddCutCopyAndPaste(CIconMenu& popup)
{
	popup.AppendMenu(MF_SEPARATOR, NULL);
	CString temp;
	temp.LoadString(IDS_EDIT_COPY);
	popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_COPY, temp);
	if (IsWritable())
	{
		temp.LoadString(IDS_EDIT_CUT);
		popup.AppendMenu(MF_STRING | (HasTextSelection() ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_CUT, temp);
		temp.LoadString(IDS_EDIT_PASTE);
		popup.AppendMenu(MF_STRING | (CAppUtils::HasClipboardFormat(CF_UNICODETEXT)||CAppUtils::HasClipboardFormat(CF_TEXT) ? MF_ENABLED : MF_DISABLED|MF_GRAYED), ID_EDIT_PASTE, temp);
	}
}

void CBaseView::CompensateForKeyboard(CPoint& point)
{
	// if the context menu is invoked through the keyboard, we have to use
	// a calculated position on where to anchor the menu on
	if (ArePointsSame(point, SetupPoint(-1, -1)))
	{
		CRect rect;
		GetWindowRect(&rect);
		point = rect.CenterPoint();
	}
}

HICON CBaseView::LoadIcon(WORD iconId)
{
	HANDLE icon = ::LoadImage( AfxGetResourceHandle(), MAKEINTRESOURCE(iconId),
						IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	return (HICON)icon;
}

void CBaseView::ReleaseBitmap()
{
	if (m_pCacheBitmap != NULL)
	{
		m_pCacheBitmap->DeleteObject();
		delete m_pCacheBitmap;
		m_pCacheBitmap = NULL;
	}
}

void CBaseView::BuildMarkedWordArray()
{
	int lineCount = GetLineCount();
	m_arMarkedWordLines.clear();
	m_arMarkedWordLines.reserve(lineCount);
	bool bDoit = !m_sMarkedWord.IsEmpty();
	for (int i = 0; i < lineCount; ++i)
	{
		if (bDoit)
		{
			CString line = GetLineChars(i);

			if (!line.IsEmpty())
			{
				m_arMarkedWordLines.push_back(line.Find(m_sMarkedWord) != -1);
			}
			else
				m_arMarkedWordLines.push_back(0);
		}
		else
			m_arMarkedWordLines.push_back(0);
	}
}

void CBaseView::BuildFindStringArray()
{
	int lineCount = GetLineCount();
	m_arFindStringLines.clear();
	m_arFindStringLines.reserve(lineCount);
	bool bDoit = !m_sFindText.IsEmpty();
	int s = 0;
	int e = 0;
	for (int i = 0; i < lineCount; ++i)
	{
		if (bDoit)
		{
			CString line = GetLineChars(i);

			if (!line.IsEmpty())
			{
				line = line.MakeLower();
				m_arFindStringLines.push_back(StringFound(line, SearchNext, s, e));
			}
			else
				m_arFindStringLines.push_back(0);
		}
		else
			m_arFindStringLines.push_back(0);
	}
	UpdateLocator();
}

bool CBaseView::GetInlineDiffPositions(int nViewLine, std::vector<inlineDiffPos>& positions)
{
	if (!m_bShowInlineDiff)
		return false;
	if ((m_pwndBottom != NULL) && !(m_pwndBottom->IsHidden()))
		return false;

	CString sLine = GetViewLineChars(nViewLine);
	if (sLine.IsEmpty())
		return false;

	CheckOtherView();
	if (!m_pOtherViewData)
		return false;

	CString sDiffLine = m_pOtherViewData->GetLine(nViewLine);
	if (sDiffLine.IsEmpty())
		return false;

	CString sLineExp = ExpandChars(sLine);
	CString sDiffLineExp = ExpandChars(sDiffLine);
	svn_diff_t * diff = NULL;
	m_svnlinediff.Diff(&diff, sLineExp, sLineExp.GetLength(), sDiffLineExp, sDiffLineExp.GetLength(), m_bInlineWordDiff);
	if (!diff || !SVNLineDiff::ShowInlineDiff(diff))
		return false;

	size_t lineoffset = 0;
	size_t position = 0;
	while (diff)
	{
		apr_off_t len = diff->original_length;
		size_t oldpos = position;

		for (apr_off_t i = 0; i < len; ++i)
		{
			position += m_svnlinediff.m_line1tokens[lineoffset].size();
			lineoffset++;
		}

		if (diff->type == svn_diff__type_diff_modified)
		{
			inlineDiffPos p;
			p.start = oldpos;
			p.end = position;
			positions.push_back(p);
		}

		diff = diff->next;
	}

	return !positions.empty();
}

void CBaseView::OnNavigateNextinlinediff()
{
	int nX;
	if (GetNextInlineDiff(nX))
	{
		POINT ptCaretViewPos = GetCaretViewPosition();
		ptCaretViewPos.x = nX;
		SetCaretAndGoalViewPosition(ptCaretViewPos);
		m_ptSelectionViewPosOrigin = ptCaretViewPos;
		EnsureCaretVisible();
	}
}

void CBaseView::OnNavigatePrevinlinediff()
{
	int nX;
	if (GetPrevInlineDiff(nX))
	{
		POINT ptCaretViewPos = GetCaretViewPosition();
		ptCaretViewPos.x = nX;
		SetCaretAndGoalViewPosition(ptCaretViewPos);
		m_ptSelectionViewPosOrigin = ptCaretViewPos;
		EnsureCaretVisible();
	}
}

bool CBaseView::HasNextInlineDiff()
{
	int nPos;
	return GetNextInlineDiff(nPos);
}

bool CBaseView::GetNextInlineDiff(int & nPos)
{
	POINT ptCaretViewPos = GetCaretViewPosition();
	std::vector<inlineDiffPos> positions;
	if (GetInlineDiffPositions(ptCaretViewPos.y, positions))
	{
		for (auto it = positions.cbegin(); it != positions.cend(); ++it)
		{
			if (it->start > ptCaretViewPos.x)
			{
				nPos = (LONG)it->start;
				return true;
			}
			if (it->end > ptCaretViewPos.x)
			{
				nPos = (LONG)it->end;
				return true;
			}
		}
	}
	return false;
}

bool CBaseView::HasPrevInlineDiff()
{
	int nPos;
	return GetPrevInlineDiff(nPos);
}

bool CBaseView::GetPrevInlineDiff(int & nPos)
{
	POINT ptCaretViewPos = GetCaretViewPosition();
	std::vector<inlineDiffPos> positions;
	if (GetInlineDiffPositions(ptCaretViewPos.y, positions))
	{
		for (auto it = positions.crbegin(); it != positions.crend(); ++it)
		{
			if ( it->end < ptCaretViewPos.x)
			{
				nPos = (LONG)it->end;
				return true;
			}
			if ( it->start < ptCaretViewPos.x)
			{
				nPos = (LONG)it->start;
				return true;
			}
		}
	}
	return false;
}

CBaseView * CBaseView::GetFirstGoodView()
{
	if (IsViewGood(m_pwndLeft))
		return m_pwndLeft;
	if (IsViewGood(m_pwndRight))
		return m_pwndRight;
	if (IsViewGood(m_pwndBottom))
		return m_pwndBottom;
	return NULL;
}

void CBaseView::BuildAllScreen2ViewVector()
{
	CBaseView * p_pwndView = GetFirstGoodView();
	if (p_pwndView)
	{
		m_Screen2View.ScheduleFullRebuild(p_pwndView->m_pViewData);
	}
}

void CBaseView::BuildAllScreen2ViewVector(int nViewLine)
{
	BuildAllScreen2ViewVector(nViewLine, nViewLine);
}

void CBaseView::BuildAllScreen2ViewVector(int nFirstViewLine, int nLastViewLine)
{
	CBaseView * p_pwndView = GetFirstGoodView();
	if (p_pwndView)
	{
		m_Screen2View.ScheduleRangeRebuild(p_pwndView->m_pViewData, nFirstViewLine, nLastViewLine);
	}
}

void CBaseView::UpdateViewLineNumbers()
{
	int nLineNumber = 0;
	int nViewLineCount = GetViewCount();
	for (int nViewLine = 0; nViewLine < nViewLineCount; nViewLine++)
	{
		int oldLine = (int)GetViewLineNumber(nViewLine);
		if (oldLine >= 0)
			SetViewLineNumber(nViewLine, nLineNumber++);
	}
	m_nDigits = 0;
}

int CBaseView::CleanEmptyLines()
{
	int nRemovedCount = 0;
	int nViewLineCount = GetViewCount();
	bool bCheckLeft = IsViewGood(m_pwndLeft);
	bool bCheckRight = IsViewGood(m_pwndRight);
	bool bCheckBottom = IsViewGood(m_pwndBottom);
	for (int nViewLine = 0; nViewLine < nViewLineCount; )
	{
		bool bAllEmpty = true;
		bAllEmpty &= !bCheckLeft || IsStateEmpty(m_pwndLeft->GetViewState(nViewLine));
		bAllEmpty &= !bCheckRight || IsStateEmpty(m_pwndRight->GetViewState(nViewLine));
		bAllEmpty &= !bCheckBottom || IsStateEmpty(m_pwndBottom->GetViewState(nViewLine));
		if (bAllEmpty)
		{
			if (bCheckLeft)
			{
				m_pwndLeft->RemoveViewData(nViewLine);
			}
			if (bCheckRight)
			{
				m_pwndRight->RemoveViewData(nViewLine);
			}
			if (bCheckBottom)
			{
				m_pwndBottom->RemoveViewData(nViewLine);
			}
			if (CUndo::GetInstance().IsGrouping()) // if use group undo -> ensure back adding goes in right (reversed) order
			{
				SaveUndoStep();
			}
			nViewLineCount--;
			nRemovedCount++;
			continue;
		}
		nViewLine++;
	}
	return nRemovedCount;
}

int CBaseView::FindScreenLineForViewLine( int viewLine )
{
	return m_Screen2View.FindScreenLineForViewLine(viewLine);
}

int CBaseView::CountMultiLines( int nViewLine )
{
	if (m_ScreenedViewLine.empty())
		return 0;   // in case the view is completely empty

	ASSERT(nViewLine < (int)m_ScreenedViewLine.size());

	if (m_ScreenedViewLine[nViewLine].bSublinesSet)
	{
		return (int)m_ScreenedViewLine[nViewLine].SubLines.size();
	}

	CString multiline = CStringUtils::WordWrap(m_pViewData->GetLine(nViewLine), GetScreenChars()-1, false, true, GetTabSize()); // GetMultiLine(nLine);

	TScreenedViewLine oScreenedLine;
	// tokenize string
	int prevpos = 0;
	int pos = 0;
	while ((pos = multiline.Find('\n', pos)) >= 0)
	{
		oScreenedLine.SubLines.push_back(multiline.Mid(prevpos, pos-prevpos)); // WordWrap could return vector/list of lines instead of string
		pos++;
		prevpos = pos;
	}
	oScreenedLine.SubLines.push_back(multiline.Mid(prevpos));
	oScreenedLine.bSublinesSet = true;
	m_ScreenedViewLine[nViewLine] = oScreenedLine;

	return CountMultiLines(nViewLine);
}

/// prepare inline diff cache
LineColors & CBaseView::GetLineColors(int nViewLine)
{
	ASSERT(nViewLine < (int)m_ScreenedViewLine.size());

	if (m_bWhitespaceInlineDiffs)
	{
		if (m_ScreenedViewLine[nViewLine].bLineColorsSetWhiteSpace)
			return m_ScreenedViewLine[nViewLine].lineColorsWhiteSpace;
	}
	else
	{
		if (m_ScreenedViewLine[nViewLine].bLineColorsSet)
			return m_ScreenedViewLine[nViewLine].lineColors;
	}

	LineColors oLineColors;
	// set main line color
	COLORREF crBkgnd, crText;
	DiffStates diffState = m_pViewData->GetState(nViewLine);
	CDiffColors::GetInstance().GetColors(diffState, crBkgnd, crText);
	oLineColors.SetColor(0, crText, crBkgnd);

	do {
		if (!m_bShowInlineDiff)
			break;

		if ((diffState == DIFFSTATE_NORMAL)&&(!m_bWhitespaceInlineDiffs))
			break;

		CString sLine = GetViewLineChars(nViewLine);
		if (sLine.IsEmpty())
			break;
		if (!m_pOtherView)
			break;

		CString sDiffLine = m_pOtherView->GetViewLineChars(nViewLine);
		if (sDiffLine.IsEmpty())
			break;

		svn_diff_t * diff = NULL;
		if (sLine.GetLength() > (int)m_nInlineDiffMaxLineLength)
			break;
		m_svnlinediff.Diff(&diff, sLine, sLine.GetLength(), sDiffLine, sDiffLine.GetLength(), m_bInlineWordDiff);
		if (!diff || !SVNLineDiff::ShowInlineDiff(diff) || !diff->next)
			break;

		int lineoffset = 0;
		int nTextStartOffset = 0;
		std::map<int, COLORREF> removedPositions;
		while (diff)
		{
			apr_off_t len = diff->original_length;

			CString s;
			for (int i = 0; i < len; ++i)
			{
				s += m_svnlinediff.m_line1tokens[lineoffset].c_str();
				lineoffset++;
			}
			bool bInlineDiff = (diff->type == svn_diff__type_diff_modified);
			int nTextLength = s.GetLength();

			CDiffColors::GetInstance().GetColors(diffState, crBkgnd, crText);
			if ((m_bShowInlineDiff)&&(bInlineDiff))
			{
				crBkgnd = InlineViewLineDiffColor(nViewLine);
			}
			else
			{
				crBkgnd = m_ModifiedBk;
			}

			if (len < diff->modified_length)
			{
				removedPositions[nTextStartOffset] = m_InlineRemovedBk;
			}
			oLineColors.SetColor(nTextStartOffset, crText, crBkgnd);

			nTextStartOffset += nTextLength;
			diff = diff->next;
		}
		for (std::map<int, COLORREF>::const_iterator it = removedPositions.begin(); it != removedPositions.end(); ++it)
		{
			oLineColors.AddShotColor(it->first, it->second);
		}
	} while (false); // error catch

	if (!m_bWhitespaceInlineDiffs)
	{
		m_ScreenedViewLine[nViewLine].lineColors = oLineColors;
		m_ScreenedViewLine[nViewLine].bLineColorsSet = true;
	}
	else
	{
		m_ScreenedViewLine[nViewLine].lineColorsWhiteSpace = oLineColors;
		m_ScreenedViewLine[nViewLine].bLineColorsSetWhiteSpace = true;
	}

	return GetLineColors(nViewLine);
}

void CBaseView::OnEditSelectall()
{
	if (m_pViewData == nullptr)
		return;
	int nLastViewLine = m_pViewData->GetCount()-1;
	if (nLastViewLine < 0)
		return;
	SetupAllViewSelection(0, nLastViewLine);

	CString sLine = GetViewLineChars(nLastViewLine);
	m_ptSelectionViewPosStart = SetupPoint(0, 0);
	m_ptSelectionViewPosEnd = SetupPoint(sLine.GetLength(), nLastViewLine);
	m_ptSelectionViewPosOrigin = SetupPoint(-1, -1);

	UpdateWindow();
}

void CBaseView::FilterWhitespaces(CString& first, CString& second)
{
	FilterWhitespaces(first);
	FilterWhitespaces(second);
}

void CBaseView::FilterWhitespaces(CString& line)
{
	line.Remove(' ');
	line.Remove('\t');
	line.Remove('\r');
	line.Remove('\n');
}

int CBaseView::GetButtonEventLineIndex(const POINT& point)
{
	const int nLineFromTop = (point.y - HEADERHEIGHT) / GetLineHeight();
	int nEventLine = nLineFromTop + m_nTopLine;
	nEventLine--;     //we need the index
	return nEventLine;
}


BOOL CBaseView::PreTranslateMessage(MSG* pMsg)
{
	if (RelayTrippleClick(pMsg))
		return TRUE;
	return CView::PreTranslateMessage(pMsg);
}


void CBaseView::ResetUndoStep()
{
	m_AllState.Clear();
}

void CBaseView::SaveUndoStep()
{
	if (!m_AllState.IsEmpty())
	{
		CUndo::GetInstance().AddState(m_AllState, GetCaretViewPosition());
	}
	ResetUndoStep();
}

void CBaseView::InsertViewData( int index, const CString& sLine, DiffStates state, int linenumber, EOL ending, HIDESTATE hide, int movedline )
{
	m_pState->addedlines.push_back(index);
	m_pViewData->InsertData(index, sLine, state, linenumber, ending, hide, movedline);
}

void CBaseView::InsertViewData( int index, const viewdata& data )
{
	m_pState->addedlines.push_back(index);
	m_pViewData->InsertData(index, data);
}

void CBaseView::RemoveViewData( int index )
{
	m_pState->removedlines[index] = m_pViewData->GetData(index);
	m_pViewData->RemoveData(index);
}

void CBaseView::SetViewData( int index, const viewdata& data )
{
	m_pState->replacedlines[index] = m_pViewData->GetData(index);
	m_pViewData->SetData(index, data);
}

void CBaseView::SetViewState( int index, DiffStates state )
{
	m_pState->linestates[index] = m_pViewData->GetState(index);
	m_pViewData->SetState(index, state);
}

void CBaseView::SetViewLine( int index, const CString& sLine )
{
	m_pState->difflines[index] = m_pViewData->GetLine(index);
	m_pViewData->SetLine(index, sLine);
}

void CBaseView::SetViewLineNumber( int index, int linenumber )
{
	int oldLineNumber = m_pViewData->GetLineNumber(index);
	if (oldLineNumber != linenumber) {
		m_pState->linelines[index] = oldLineNumber;
		m_pViewData->SetLineNumber(index, linenumber);
	}
}

void CBaseView::SetViewLineEnding( int index, EOL ending )
{
	m_pState->linesEOL[index] = m_pViewData->GetLineEnding(index);
	m_pViewData->SetLineEnding(index, ending);
}


BOOL CBaseView::GetViewSelection( int& start, int& end ) const
{
	if (HasSelection())
	{
		start = m_nSelViewBlockStart;
		end = m_nSelViewBlockEnd;
		return true;
	}
	return false;
}

int CBaseView::Screen2View::GetViewLineForScreen( int screenLine )
{
	RebuildIfNecessary();
	if (size() <= screenLine)
		return 0;
	return m_Screen2View[screenLine].nViewLine;
}

int CBaseView::Screen2View::size()
{
	RebuildIfNecessary();
	return (int)m_Screen2View.size();
}

int CBaseView::Screen2View::GetSubLineOffset( int screenLine )
{
	RebuildIfNecessary();
	if (size() <= screenLine)
		return 0;
	return m_Screen2View[screenLine].nViewSubLine;
}

CBaseView::TScreenLineInfo CBaseView::Screen2View::GetScreenLineInfo( int screenLine )
{
	RebuildIfNecessary();
	return m_Screen2View[screenLine];
}

/**
	doing partial rebuild, whole screen2view vector is built, but uses ScreenedViewLine cache to do it faster
*/
void CBaseView::Screen2View::RebuildIfNecessary()
{
	if (!m_pViewData)
		return; // rebuild not necessary

	FixScreenedCacheSize(m_pwndLeft);
	FixScreenedCacheSize(m_pwndRight);
	FixScreenedCacheSize(m_pwndBottom);
	if (!m_bFull)
	{
		for (auto it = m_RebuildRanges.cbegin(); it != m_RebuildRanges.cend(); ++it)
		{
			ResetScreenedViewLineCache(m_pwndLeft, *it);
			ResetScreenedViewLineCache(m_pwndRight, *it);
			ResetScreenedViewLineCache(m_pwndBottom, *it);
		}
	}
	else
	{
		ResetScreenedViewLineCache(m_pwndLeft);
		ResetScreenedViewLineCache(m_pwndRight);
		ResetScreenedViewLineCache(m_pwndBottom);
	}
	m_RebuildRanges.clear();
	m_bFull = false;

	size_t OldSize = m_Screen2View.size();
	m_Screen2View.clear();
	m_Screen2View.reserve(OldSize); // guess same size
	for (int i = 0; i < m_pViewData->GetCount(); ++i)
	{
		if (m_pMainFrame->m_bCollapsed)
		{
			while ((i < m_pViewData->GetCount())&&(m_pViewData->GetHideState(i) == HIDESTATE_HIDDEN))
				++i;
			if (!(i < m_pViewData->GetCount()))
				break;
		}
		TScreenLineInfo oLineInfo;
		oLineInfo.nViewLine = i;
		oLineInfo.nViewSubLine = -1; // no wrap
		if (m_pMainFrame->m_bWrapLines && !IsViewLineHidden(m_pViewData, i))
		{
			int nMaxLines = 0;
			if (IsLeftViewGood())
				nMaxLines = std::max<int>(nMaxLines, m_pwndLeft->CountMultiLines(i));
			if (IsRightViewGood())
				nMaxLines = std::max<int>(nMaxLines, m_pwndRight->CountMultiLines(i));
			if (IsBottomViewGood())
				nMaxLines = std::max<int>(nMaxLines, m_pwndBottom->CountMultiLines(i));
			for (int l = 0; l < (nMaxLines-1); ++l)
			{
				oLineInfo.nViewSubLine++;
				m_Screen2View.push_back(oLineInfo);
			}
			oLineInfo.nViewSubLine++;
		}
		m_Screen2View.push_back(oLineInfo);
	}
	m_pViewData = NULL;

	if (IsLeftViewGood())
		 m_pwndLeft->BuildMarkedWordArray();
	if (IsRightViewGood())
		m_pwndRight->BuildMarkedWordArray();
	if (IsBottomViewGood())
		m_pwndBottom->BuildMarkedWordArray();
	UpdateLocator();
	RecalcAllVertScrollBars();
	RecalcAllHorzScrollBars();
}

int CBaseView::Screen2View::FindScreenLineForViewLine( int viewLine )
{
	RebuildIfNecessary();

	int nScreenLineCount = (int)m_Screen2View.size();

	int nPos = 0;
	if (nScreenLineCount>16)
	{
		// for enough long data search for last screen
		// with viewline less than one we are looking for
		// use approximate method (based on) binary search using asymmetric start point
		// in form 2**n (determined as MSB of length) to go around division and rounding;
		// this effectively looks for bit values from MSB to LSB

		int nTestBit;
		//GetMostSignificantBitValue
		// note _BitScanReverse(&nTestBit, nScreenLineCount); can be used instead
		nTestBit = nScreenLineCount;
		nTestBit |= nTestBit>>1;
		nTestBit |= nTestBit>>2;
		nTestBit |= nTestBit>>4;
		nTestBit |= nTestBit>>8;
		nTestBit |= nTestBit>>16;
		nTestBit ^= (nTestBit>>1);

		while (nTestBit)
		{
			int nTestPos = nPos | nTestBit;
			if (nTestPos < nScreenLineCount && m_Screen2View[nTestPos].nViewLine < viewLine)
			{
				nPos = nTestPos;
			}
			nTestBit >>= 1;
		}
	}
	while (nPos < nScreenLineCount && m_Screen2View[nPos].nViewLine < viewLine)
	{
		nPos++;
	}

	return nPos;
}

void CBaseView::Screen2View::ScheduleFullRebuild(CViewData * pViewData) {
	m_bFull = true;

	m_pViewData = pViewData;
}

void CBaseView::Screen2View::ScheduleRangeRebuild(CViewData * pViewData, int nFirstViewLine, int nLastViewLine)
{
	if (m_bFull)
		return;

	m_pViewData = pViewData;

	TRebuildRange Range;
	Range.FirstViewLine=nFirstViewLine;
	Range.LastViewLine=nLastViewLine;
	m_RebuildRanges.push_back(Range);
}

bool CBaseView::Screen2View::FixScreenedCacheSize(CBaseView* pwndView)
{
	if (!IsViewGood(pwndView))
	{
		return false;
	}
	const int nOldSize = (int)pwndView->m_ScreenedViewLine.size();
	const int nViewCount = std::max<int>(pwndView->GetViewCount(), 0);
	if (nOldSize == nViewCount)
	{
		return false;
	}
	pwndView->m_ScreenedViewLine.resize(nViewCount);
	return true;
}

bool CBaseView::Screen2View::ResetScreenedViewLineCache(CBaseView* pwndView)
{
	if (!IsViewGood(pwndView))
	{
		return false;
	}
	TRebuildRange Range={0, pwndView->GetViewCount()-1};
	ResetScreenedViewLineCache(pwndView, Range);
	return true;
}

bool CBaseView::Screen2View::ResetScreenedViewLineCache(CBaseView* pwndView, const TRebuildRange& Range)
{
	if (!IsViewGood(pwndView))
	{
		return false;
	}
	if (Range.LastViewLine == -1)
	{
		return false;
	}
	ASSERT(Range.FirstViewLine >= 0);
	ASSERT(Range.LastViewLine < pwndView->GetViewCount());
	for (int i = Range.FirstViewLine; i <= Range.LastViewLine; i++)
	{
		pwndView->m_ScreenedViewLine[i].Clear();
	}
	return false;
}

void CBaseView::WrapChanged()
{
	m_nMaxLineLength = -1;
	m_nOffsetChar = 0;
}

void CBaseView::OnEditFind()
{
	if (m_pFindDialog)
		return;

	m_pFindDialog = new CFindDlg(this);
	m_pFindDialog->Create(this);

	m_pFindDialog->SetFindString(HasTextSelection() ? GetSelectedText() : L"");
}

LRESULT CBaseView::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	ASSERT(m_pFindDialog != NULL);

	if (m_pFindDialog->IsTerminating())
	{
		// invalidate the handle identifying the dialog box.
		m_pFindDialog = NULL;
		return 0;
	}

	if(m_pFindDialog->FindNext())
	{
		//read data from dialog
		m_sFindText = m_pFindDialog->GetFindString();
		m_bMatchCase = (m_pFindDialog->MatchCase() == TRUE);
		m_bLimitToDiff = m_pFindDialog->LimitToDiffs();
		m_bWholeWord = m_pFindDialog->WholeWord();

		if (!m_bMatchCase)
			m_sFindText = m_sFindText.MakeLower();

		BuildFindStringArray();
		OnEditFindnext();
	}

	return 0;
}

void CBaseView::OnEditFindnextStart()
{
	if (m_pViewData == nullptr)
		return;
	if (HasTextSelection())
	{
		m_sFindText = GetSelectedText();
		m_bMatchCase = false;
		m_bLimitToDiff = false;
		m_bWholeWord = false;
		m_sFindText = m_sFindText.MakeLower();

		BuildFindStringArray();
		OnEditFindnext();
	}
	else
	{
		m_sFindText.Empty();
		BuildFindStringArray();
	}
}

void CBaseView::OnEditFindprevStart()
{
	if (m_pViewData == nullptr)
		return;
	if (HasTextSelection())
	{
		m_sFindText = GetSelectedText();
		m_bMatchCase = false;
		m_bLimitToDiff = false;
		m_bWholeWord = false;
		m_sFindText = m_sFindText.MakeLower();

		BuildFindStringArray();
		OnEditFindprev();
	}
	else
	{
		m_sFindText.Empty();
		BuildFindStringArray();
	}
}

bool CBaseView::StringFound(const CString& str, SearchDirection srchDir, int& start, int& end) const
{
	start = str.Find(m_sFindText);
	if ((srchDir==SearchPrevious)&&(start>=0))
	{
		int laststart = start;
		do
		{
			start = laststart;
			laststart = str.Find(m_sFindText, laststart+1);
		} while (laststart >= 0);
	}
	end = start + m_sFindText.GetLength();
	bool bStringFound = (start >= 0);
	if (bStringFound && m_bWholeWord)
	{
		if (start)
			bStringFound = IsWordSeparator(str.Mid(start-1,1).GetAt(0));

		if (bStringFound)
		{
			if (str.GetLength() > end)
				bStringFound = IsWordSeparator(str.Mid(end, 1).GetAt(0));
		}
	}
	return bStringFound;
}

void CBaseView::OnEditFindprev()
{
	Search(SearchPrevious);
}

void CBaseView::OnEditFindnext()
{
	Search(SearchNext);
}

void CBaseView::Search(SearchDirection srchDir)
{
	if (m_sFindText.IsEmpty())
		return;
	if(!m_pViewData)
		return;

	POINT start = m_ptSelectionViewPosEnd;
	POINT end;
	end.y = m_pViewData->GetCount()-1;
	if (end.y < 0)
		return;

	if (srchDir==SearchNext)
		end.x = GetViewLineLength(end.y);
	else
	{
		end.x = m_ptSelectionViewPosStart.x;
		start.x = 0;
	}

	if (!HasTextSelection())
	{
		start.y = m_ptCaretViewPos.y;
		if (srchDir==SearchNext)
			start.x = m_ptCaretViewPos.x;
		else
		{
			start.x = 0;
			end.x = m_ptCaretViewPos.x;
		}
	}
	CString sSelectedText;
	int startline = -1;
	for (int nViewLine=start.y; ;srchDir==SearchNext ? nViewLine++ : nViewLine--)
	{
		if (nViewLine < 0)
		{
			nViewLine = m_pViewData->GetCount()-1;
			startline = start.y;
		}
		if (nViewLine > end.y)
		{
			nViewLine = 0;
			startline = start.y;
		}
		if (startline >= 0)
		{
			if (nViewLine == startline)
				break;
		}
		switch (m_pViewData->GetState(nViewLine))
		{
		case DIFFSTATE_EMPTY:
			break;
		case DIFFSTATE_UNKNOWN:
		case DIFFSTATE_NORMAL:
			if (m_bLimitToDiff)
				break;
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
		case DIFFSTATE_MOVED_FROM:
		case DIFFSTATE_MOVED_TO:
		case DIFFSTATE_YOURSREMOVED:
		case DIFFSTATE_YOURSADDED:
		case DIFFSTATE_EDITED:
			{
				sSelectedText = GetViewLineChars(nViewLine);
				if (nViewLine==start.y)
					sSelectedText = srchDir==SearchNext ? sSelectedText.Mid(start.x) : sSelectedText.Left(start.x);
				if (!m_bMatchCase)
					sSelectedText = sSelectedText.MakeLower();
				int startfound = -1;
				int endfound = -1;
				if (StringFound(sSelectedText, srchDir, startfound, endfound))
				{
					HighlightViewLines(nViewLine, nViewLine);
					m_ptSelectionViewPosStart.x = startfound;
					m_ptSelectionViewPosEnd.x = endfound;
					if (nViewLine==start.y)
					{
						m_ptSelectionViewPosStart.x += start.x;
						m_ptSelectionViewPosEnd.x += start.x;
					}
					m_ptSelectionViewPosEnd.x = m_ptSelectionViewPosStart.x + m_sFindText.GetLength();
					m_ptSelectionViewPosStart.y = nViewLine;
					m_ptSelectionViewPosEnd.y = nViewLine;
					m_ptCaretViewPos = m_ptSelectionViewPosStart;
					UpdateViewsCaretPosition();
					EnsureCaretVisible();
					Invalidate();
					return;
				}
			}
			break;
		}
	}
	m_pMainFrame->m_nMoveMovesToIgnore = MOVESTOIGNORE;
}

CString CBaseView::GetSelectedText() const
{
	CString sSelectedText;
	POINT start = m_ptSelectionViewPosStart;
	POINT end = m_ptSelectionViewPosEnd;
	if (!HasTextSelection())
	{
		if (!HasSelection())
			return sSelectedText;
		start.y = m_nSelViewBlockStart;
		start.x = 0;
		end.y = m_nSelViewBlockEnd;
		end.x = GetViewLineLength(m_nSelViewBlockEnd);
	}
	// first store the selected lines in one CString
	for (int nViewLine=start.y; nViewLine<=end.y; nViewLine++)
	{
		switch (m_pViewData->GetState(nViewLine))
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
		case DIFFSTATE_MOVED_FROM:
		case DIFFSTATE_MOVED_TO:
		case DIFFSTATE_YOURSREMOVED:
		case DIFFSTATE_YOURSADDED:
		case DIFFSTATE_EDITED:
			sSelectedText += GetViewLineChars(nViewLine);
			sSelectedText += _T("\r\n");
			break;
		}
	}
	// remove the non-selected chars from the first line, last line and last \r\n
	int nLeftCut = start.x;
	int nRightCut = GetViewLineChars(end.y).GetLength() - end.x + 2;
	sSelectedText = sSelectedText.Mid(nLeftCut, sSelectedText.GetLength()-nLeftCut-nRightCut);
	return sSelectedText;
}

void CBaseView::OnEditGotoline()
{
	if (m_pViewData == NULL)
		return;
	// find the last and first line number
	int nViewLineCount = m_pViewData->GetCount();

	int nLastLineNumber = DIFF_EMPTYLINENUMBER;
	for (int nViewLine=nViewLineCount-1; nViewLine>=0; --nViewLine)
	{
		 nLastLineNumber = m_pViewData->GetLineNumber(nViewLine);
		 if (nLastLineNumber!=DIFF_EMPTYLINENUMBER)
		 {
			 break;
		 }
	}
	if (nLastLineNumber==DIFF_EMPTYLINENUMBER || nLastLineNumber==0) // not numbered line foud or last one is first
	{
		return;
	}
	nLastLineNumber++;
	int nFirstLineNumber=1; // first is always 1

	CString sText;
	sText.Format(IDS_GOTOLINE, nFirstLineNumber, nLastLineNumber);

	CGotoLineDlg dlg(this);
	dlg.SetLabel(sText);
	dlg.SetLimits(nFirstLineNumber, nLastLineNumber);
	if (dlg.DoModal() == IDOK)
	{
		for (int nViewLine = 0; nViewLine < nViewLineCount; ++nViewLine)
		{
			if ((m_pViewData->GetLineNumber(nViewLine)+1) == dlg.GetLineNumber())
			{
				HighlightViewLines(nViewLine, nViewLine);
				return;
			}
		}
	}
}

void CBaseView::OnToggleReadonly()
{
	if (IsReadonlyChangable()) {
		SetWritable(IsReadonly());
	}
}

int CBaseView::SaveFile(int nFlags) 
{
	Invalidate();
	if (m_pViewData!=NULL && m_pWorkingFile!=NULL)
	{
		CFileTextLines file;
		//file.SetSaveParams(m_SaveParams);

		for (int i=0; i<m_pViewData->GetCount(); i++)
		{
			//only copy non-removed lines
			DiffStates state = m_pViewData->GetState(i);
			switch (state)
			{
			case DIFFSTATE_CONFLICTED:
			case DIFFSTATE_CONFLICTED_IGNORED:
				{
					int first = i;
					int last = i;
					do
					{
						last++;
					} while((last<m_pViewData->GetCount()) && ((m_pViewData->GetState(last)==DIFFSTATE_CONFLICTED)||(m_pViewData->GetState(last)==DIFFSTATE_CONFLICTED_IGNORED)));
					file.Add(_T("<<<<<<< .mine"), EOL_NOENDING);
					for (int j=first; j<last; j++)
					{
						file.Add(m_pwndRight->m_pViewData->GetLine(j), m_pwndRight->m_pViewData->GetLineEnding(j));
					}
					file.Add(_T("======="), EOL_NOENDING);
					for (int j=first; j<last; j++)
					{
						file.Add(m_pwndLeft->m_pViewData->GetLine(j), m_pwndLeft->m_pViewData->GetLineEnding(j));
					}
					file.Add(_T(">>>>>>> .theirs"), EOL_NOENDING);
					i = last-1;
				}
				break;
			case DIFFSTATE_EMPTY:
				break;
			case DIFFSTATE_CONFLICTEMPTY:
			case DIFFSTATE_IDENTICALREMOVED:
			case DIFFSTATE_REMOVED:
			case DIFFSTATE_THEIRSREMOVED:
			case DIFFSTATE_YOURSREMOVED:
			case DIFFSTATE_CONFLICTRESOLVEDEMPTY:
				if ((nFlags&SAVE_REMOVED) == 0)
				{
					// do not save removed lines
					break;
				}
			default:
				file.Add(m_pViewData->GetLine(i), m_pViewData->GetLineEnding(i));
				break;
			}
		}
		if (!file.Save(m_pWorkingFile->GetFilename()))
		{
			::MessageBox(m_hWnd, file.GetErrorString(), _T("TortoiseGitMerge"), MB_ICONERROR);
			return -1;
		}
		m_pWorkingFile->StoreFileAttributes();
		// m_dlgFilePatches.SetFileStatusAsPatched(sFilePath);
		SetModified(FALSE);
		CUndo::GetInstance().MarkAsOriginalState();
		if (file.GetCount() == 1 && file.GetAt(0).IsEmpty() && file.GetLineEnding(0) == EOL_NOENDING)
			return 0;
		return file.GetCount();
	}
	return 1;
}


int CBaseView::SaveFileTo(CString sFileName)
{
	if (m_pWorkingFile)
	{
		m_pWorkingFile->SetFileName(sFileName);
		return SaveFile();
	}
	return -1;
}
