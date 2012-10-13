// TortoiseGitBlame - a Viewer for Git Blames

// Copyright (C) 2008-2012 - TortoiseGit
// Copyright (C) 2010-2012 Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2003-2008 - TortoiseSVN

// Copyright (C)2003 Don HO <donho@altern.org>

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

// CTortoiseGitBlameView.cpp : implementation of the CTortoiseGitBlameView class
//

#include "stdafx.h"
#include "TortoiseGitBlame.h"
#include "CommonAppUtils.h"
#include "TortoiseGitBlameDoc.h"
#include "TortoiseGitBlameView.h"
#include "MainFrm.h"
#include "EditGotoDlg.h"
#include "LoglistUtils.h"
#include "FileTextLines.h"
#include "UniCodeUtils.h"
#include "MenuEncode.h"
#include "gitdll.h"
#include "SysInfo.h"
#include "StringUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

wchar_t WideCharSwap2(wchar_t nValue)
{
	return (((nValue>> 8)) | (nValue << 8));
}

UINT CTortoiseGitBlameView::m_FindDialogMessage;

// CTortoiseGitBlameView
IMPLEMENT_DYNAMIC(CSciEditBlame,CSciEdit)

IMPLEMENT_DYNCREATE(CTortoiseGitBlameView, CView)

BEGIN_MESSAGE_MAP(CTortoiseGitBlameView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &CTortoiseGitBlameView::OnFilePrintPreview)
	ON_COMMAND(ID_EDIT_FIND,OnEditFind)
	ON_COMMAND(ID_EDIT_GOTO,OnEditGoto)
	ON_COMMAND(ID_EDIT_COPY, CopyToClipboard)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateViewCopyToClipboard)
	ON_COMMAND(ID_VIEW_NEXT,OnViewNext)
	ON_COMMAND(ID_VIEW_PREV,OnViewPrev)
	ON_COMMAND(ID_VIEW_SHOWAUTHOR, OnViewToggleAuthor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWAUTHOR, OnUpdateViewToggleAuthor)
	ON_COMMAND(ID_VIEW_FOLLOWRENAMES, OnViewToggleFollowRenames)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FOLLOWRENAMES, OnUpdateViewToggleFollowRenames)
	ON_COMMAND(ID_BLAMEPOPUP_COPYHASHTOCLIPBOARD, CopyHashToClipboard)
	ON_COMMAND(ID_BLAMEPOPUP_COPYLOGTOCLIPBOARD, CopySelectedLogToClipboard)
	ON_COMMAND(ID_BLAMEPOPUP_BLAMEPREVIOUSREVISION, BlamePreviousRevision)
	ON_COMMAND(ID_BLAMEPOPUP_DIFFPREVIOUS, DiffPreviousRevision)
	ON_COMMAND(ID_BLAMEPOPUP_SHOWLOG, ShowLog)
	ON_UPDATE_COMMAND_UI(ID_BLAMEPOPUP_BLAMEPREVIOUSREVISION, OnUpdateBlamePopupBlamePrevious)
	ON_UPDATE_COMMAND_UI(ID_BLAMEPOPUP_DIFFPREVIOUS, OnUpdateBlamePopupDiffPrevious)
	ON_COMMAND_RANGE(IDM_FORMAT_ENCODE, IDM_FORMAT_ENCODE_END, OnChangeEncode)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_NOTIFY(SCN_PAINTED,0,OnSciPainted)
	ON_NOTIFY(SCN_GETBKCOLOR,0,OnSciGetBkColor)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
END_MESSAGE_MAP()


// CTortoiseGitBlameView construction/destruction

CTortoiseGitBlameView::CTortoiseGitBlameView()
{
	hInstance = 0;
	hResource = 0;
	currentDialog = 0;
	wMain = 0;
	m_wEditor = 0;
	wLocator = 0;

	m_font = 0;
	m_italicfont = 0;
	m_blamewidth = 0;
	m_revwidth = 0;
	m_datewidth = 0;
	m_authorwidth = 0;
	m_linewidth = 0;

	m_windowcolor = ::GetSysColor(COLOR_WINDOW);
	m_textcolor = ::GetSysColor(COLOR_WINDOWTEXT);
	m_texthighlightcolor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_mouserevcolor = InterColor(m_windowcolor, m_textcolor, 20);
	m_mouseauthorcolor = InterColor(m_windowcolor, m_textcolor, 10);
	m_selectedrevcolor = ::GetSysColor(COLOR_HIGHLIGHT);
	m_selectedauthorcolor = InterColor(m_selectedrevcolor, m_texthighlightcolor, 35);
	m_mouserev = -2;

	m_selectedrev = -1;
	m_selectedorigrev = -1;
	m_SelectedLine = -1;
	m_directPointer = 0;
	m_directFunction = 0;

	m_lowestrev = LONG_MAX;
	m_highestrev = 0;
	m_colorage = true;

	m_bShowLine=true;

	m_bShowAuthor = (theApp.GetInt(_T("ShowAuthor"), 1) == 1);
	m_bShowDate=false;
	m_bFollowRenames = (theApp.GetInt(_T("FollowRenames"), 0) == 1);

	m_FindDialogMessage = ::RegisterWindowMessage(FINDMSGSTRING);
	m_pFindDialog = NULL;
	// get short/long datetime setting from registry
	DWORD RegUseShortDateFormat = CRegDWORD(_T("Software\\TortoiseGit\\LogDateFormat"), TRUE);
	if ( RegUseShortDateFormat )
	{
		m_DateFormat = DATE_SHORTDATE;
	}
	else
	{
		m_DateFormat = DATE_LONGDATE;
	}
	// get relative time display setting from registry
	DWORD regRelativeTimes = CRegDWORD(_T("Software\\TortoiseGit\\RelativeTimes"), FALSE);
	m_bRelativeTimes = (regRelativeTimes != 0);

	m_sRev.LoadString(IDS_LOG_REVISION);
	m_sAuthor.LoadString(IDS_LOG_AUTHOR);
	m_sDate.LoadString(IDS_LOG_DATE);
	m_sMessage.LoadString(IDS_LOG_MESSAGE);

	m_Buffer = NULL;
}

CTortoiseGitBlameView::~CTortoiseGitBlameView()
{
	if (m_font)
		DeleteObject(m_font);
	if (m_italicfont)
		DeleteObject(m_italicfont);

	if(m_Buffer)
	{
		delete m_Buffer;
		m_Buffer=NULL;
	}
}
struct EncodingUnit
{
	int id;
	char *name;
};

void CTortoiseGitBlameView::OnChangeEncode(UINT nId)
{

	static EncodingUnit	encodings[]	= {
		{1250,	"windows-1250"},																	//IDM_FORMAT_WIN_1250
		{1251,	"windows-1251"},																	//IDM_FORMAT_WIN_1251
		{1252,	"windows-1252"},																	//IDM_FORMAT_WIN_1252
		{1253,	"windows-1253"},																	//IDM_FORMAT_WIN_1253
		{1254,	"windows-1254"},																	//IDM_FORMAT_WIN_1254
		{1255,	"windows-1255"},																	//IDM_FORMAT_WIN_1255
		{1256,	"windows-1256"},																	//IDM_FORMAT_WIN_1256
		{1257,	"windows-1257"},																	//IDM_FORMAT_WIN_1257
		{1258,	"windows-1258"},																	//IDM_FORMAT_WIN_1258
		{28591,	"latin1	ISO_8859-1 ISO-8859-1 CP819	IBM819 csISOLatin1 iso-ir-100 l1"},				//IDM_FORMAT_ISO_8859_1
		{28592,	"latin2	ISO_8859-2 ISO-8859-2 csISOLatin2 iso-ir-101 l2"},							//IDM_FORMAT_ISO_8859_2
		{28593,	"latin3	ISO_8859-3 ISO-8859-3 csISOLatin3 iso-ir-109 l3"},							//IDM_FORMAT_ISO_8859_3
		{28594,	"latin4	ISO_8859-4 ISO-8859-4 csISOLatin4 iso-ir-110 l4"},							//IDM_FORMAT_ISO_8859_4
		{28595,	"cyrillic ISO_8859-5 ISO-8859-5	csISOLatinCyrillic iso-ir-144"},					//IDM_FORMAT_ISO_8859_5
		{28596,	"arabic	ISO_8859-6 ISO-8859-6 csISOLatinArabic iso-ir-127 ASMO-708 ECMA-114"},		//IDM_FORMAT_ISO_8859_6
		{28597,	"greek ISO_8859-7 ISO-8859-7 csISOLatinGreek greek8	iso-ir-126 ELOT_928	ECMA-118"},	//IDM_FORMAT_ISO_8859_7
		{28598,	"hebrew	ISO_8859-8 ISO-8859-8 csISOLatinHebrew iso-ir-138"},						//IDM_FORMAT_ISO_8859_8
		{28599,	"latin5	ISO_8859-9 ISO-8859-9 csISOLatin5 iso-ir-148 l5"},							//IDM_FORMAT_ISO_8859_9
		{28600,	"latin6	ISO_8859-10	ISO-8859-10	csISOLatin6	iso-ir-157 l6"},						//IDM_FORMAT_ISO_8859_10
		{28601,	"ISO_8859-11 ISO-8859-11"},															//IDM_FORMAT_ISO_8859_11
		{28603,	"ISO_8859-13 ISO-8859-13"},															//IDM_FORMAT_ISO_8859_13
		{28604,	"iso-celtic	latin8 ISO_8859-14 ISO-8859-14 18 iso-ir-199"},							//IDM_FORMAT_ISO_8859_14
		{28605,	"Latin-9 ISO_8859-15 ISO-8859-15"},													//IDM_FORMAT_ISO_8859_15
		{28606,	"latin10 ISO_8859-16 ISO-8859-16 110 iso-ir-226"},									//IDM_FORMAT_ISO_8859_16
		{437,	"IBM437	cp437 437 csPC8CodePage437"},												//IDM_FORMAT_DOS_437
		{720,	"IBM720	cp720 oem720 720"},															//IDM_FORMAT_DOS_720
		{737,	"IBM737	cp737 oem737 737"},															//IDM_FORMAT_DOS_737
		{775,	"IBM775	cp775 oem775 775"},															//IDM_FORMAT_DOS_775
		{850,	"IBM850	cp850 oem850 850"},															//IDM_FORMAT_DOS_850
		{852,	"IBM852	cp852 oem852 852"},															//IDM_FORMAT_DOS_852
		{855,	"IBM855	cp855 oem855 855 csIBM855"},												//IDM_FORMAT_DOS_855
		{857,	"IBM857	cp857 oem857 857"},															//IDM_FORMAT_DOS_857
		{858,	"IBM858	cp858 oem858 858"},															//IDM_FORMAT_DOS_858
		{860,	"IBM860	cp860 oem860 860"},															//IDM_FORMAT_DOS_860
		{861,	"IBM861	cp861 oem861 861"},															//IDM_FORMAT_DOS_861
		{862,	"IBM862	cp862 oem862 862"},															//IDM_FORMAT_DOS_862
		{863,	"IBM863	cp863 oem863 863"},															//IDM_FORMAT_DOS_863
		{865,	"IBM865	cp865 oem865 865"},															//IDM_FORMAT_DOS_865
		{866,	"IBM866	cp866 oem866 866"},															//IDM_FORMAT_DOS_866
		{869,	"IBM869	cp869 oem869 869"},															//IDM_FORMAT_DOS_869
		{950,	"big5 csBig5"},																		//IDM_FORMAT_BIG5
		{936,	"gb2312	gbk	csGB2312"},																//IDM_FORMAT_GB2312
		{932,	"Shift_JIS MS_Kanji	csShiftJIS csWindows31J"},										//IDM_FORMAT_SHIFT_JIS
		{949,	"windows-949 korean"},																//IDM_FORMAT_KOREAN_WIN
		{51949,	"euc-kr	csEUCKR"},																	//IDM_FORMAT_EUC_KR
		{874,	"tis-620"},																			//IDM_FORMAT_TIS_620
		{10007,	"x-mac-cyrillic	xmaccyrillic"},														//IDM_FORMAT_MAC_CYRILLIC
		{21866,	"koi8_u"},																			//IDM_FORMAT_KOI8U_CYRILLIC
		{20866,	"koi8_r	csKOI8R"}																	//IDM_FORMAT_KOI8R_CYRILLIC
};
	if(nId >= IDM_FORMAT_ENCODE && nId <= IDM_FORMAT_ENCODE_END)
		this->UpdateInfo(encodings[nId - IDM_FORMAT_ENCODE].id);
}
int CTortoiseGitBlameView::OnCreate(LPCREATESTRUCT lpcs)
{

	CRect rect,rect1;
	this->GetWindowRect(&rect1);
	rect.left=m_blamewidth+LOCATOR_WIDTH;
	rect.right=rect.Width();
	rect.top=0;
	rect.bottom=rect.Height();
	BOOL b=m_TextView.Create(_T("Scintilla"),_T("source"),0,rect,this,0,0);
	m_TextView.Init(0,FALSE);
	m_TextView.ShowWindow( SW_SHOW);
	//m_TextView.InsertText(_T("Abdadfasdf"));
	m_wEditor = m_TextView.m_hWnd;
	CreateFont();
	InitialiseEditor();
	m_ToolTip.Create(this->GetParent());

	::AfxGetApp()->GetMainWnd();
	return CView::OnCreate(lpcs);

}

void CTortoiseGitBlameView::OnSize(UINT nType,int cx, int cy)
{

	CRect rect;
	rect.left=m_blamewidth;
	rect.right=cx;
	rect.top=0;
	rect.bottom=cy;

	m_TextView.MoveWindow(&rect);

}
BOOL CTortoiseGitBlameView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

// CTortoiseGitBlameView drawing

void CTortoiseGitBlameView::OnDraw(CDC* /*pDC*/)
{
	CTortoiseGitBlameDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	DrawBlame(this->GetDC()->m_hDC);
	DrawLocatorBar(this->GetDC()->m_hDC);
	// TODO: add draw code for native data here
}


// CTortoiseGitBlameView printing


void CTortoiseGitBlameView::OnFilePrintPreview()
{
	AFXPrintPreview(this);
}

BOOL CTortoiseGitBlameView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CTortoiseGitBlameView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CTortoiseGitBlameView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void CTortoiseGitBlameView::OnRButtonUp(UINT nFlags, CPoint point)
{
	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR height = SendEditor(SCI_TEXTHEIGHT);
	line = line + (point.y/height);
	if (line < (LONG)m_CommitHash.size())
	{
		if(m_ID[line] >= 0) // only show context menu if we have log data for it
		{
			m_MouseLine = (LONG)line;
			ClientToScreen(&point);
			theApp.GetContextMenuManager()->ShowPopupMenu(IDR_BLAME_POPUP, point.x, point.y, this, TRUE);
		}
	}
}

void CTortoiseGitBlameView::OnUpdateBlamePopupBlamePrevious(CCmdUI *pCmdUI)
{
	if (m_ID[m_MouseLine] <= 1)
	{
		pCmdUI->Enable(false);
	}
	else
	{
		pCmdUI->Enable(true);
	}
}

void CTortoiseGitBlameView::OnUpdateBlamePopupDiffPrevious(CCmdUI *pCmdUI)
{
	if (m_ID[m_MouseLine] <= 1)
	{
		pCmdUI->Enable(false);
	}
	else
	{
		pCmdUI->Enable(true);
	}
}

void CTortoiseGitBlameView::CopyHashToClipboard()
{
	this->GetLogList()->CopySelectionToClipBoard(CGitLogListBase::ID_COPY_HASH);
}

// CTortoiseGitBlameView diagnostics

#ifdef _DEBUG
void CTortoiseGitBlameView::AssertValid() const
{
	CView::AssertValid();
}

void CTortoiseGitBlameView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CTortoiseGitBlameDoc* CTortoiseGitBlameView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CTortoiseGitBlameDoc)));
	return (CTortoiseGitBlameDoc*)m_pDocument;
}
#endif //_DEBUG


// CTortoiseGitBlameView message handlers
CString CTortoiseGitBlameView::GetAppDirectory()
{
	CString path;
	DWORD len = 0;
	DWORD bufferlen = MAX_PATH;		// MAX_PATH is not the limit here!
	do
	{
		bufferlen += MAX_PATH;		// MAX_PATH is not the limit here!
		TCHAR * pBuf = new TCHAR[bufferlen];
		len = GetModuleFileName(NULL, pBuf, bufferlen);
		path = CString(pBuf, len);
		delete [] pBuf;
	} while(len == bufferlen);

	path = path.Left(path.ReverseFind(_T('\\')));
	//path = path.substr(0, path.rfind('\\') + 1);

	return path;
}

// Return a color which is interpolated between c1 and c2.
// Slider controls the relative proportions as a percentage:
// Slider = 0 	represents pure c1
// Slider = 50	represents equal mixture
// Slider = 100	represents pure c2
COLORREF CTortoiseGitBlameView::InterColor(COLORREF c1, COLORREF c2, int Slider)
{
	int r, g, b;

	// Limit Slider to 0..100% range
	if (Slider < 0)
		Slider = 0;
	if (Slider > 100)
		Slider = 100;

	// The color components have to be treated individually.
	r = (GetRValue(c2) * Slider + GetRValue(c1) * (100 - Slider)) / 100;
	g = (GetGValue(c2) * Slider + GetGValue(c1) * (100 - Slider)) / 100;
	b = (GetBValue(c2) * Slider + GetBValue(c1) * (100 - Slider)) / 100;

	return RGB(r, g, b);
}

LRESULT CTortoiseGitBlameView::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_directFunction)
	{
		return ((SciFnDirect) m_directFunction)(m_directPointer, Msg, wParam, lParam);
	}
	return ::SendMessage(m_wEditor, Msg, wParam, lParam);
}

void CTortoiseGitBlameView::SetAStyle(int style, COLORREF fore, COLORREF back, int size, CString *face)
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face)
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(this->m_TextView.StringForControl(*face).GetBuffer()));
}

void CTortoiseGitBlameView::InitialiseEditor()
{

	m_directFunction = ::SendMessage(m_wEditor, SCI_GETDIRECTFUNCTION, 0, 0);
	m_directPointer = ::SendMessage(m_wEditor, SCI_GETDIRECTPOINTER, 0, 0);
	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT,
			  black,
			  white,
			(DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10),
			&CString(((stdstring)CRegStdString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New"))).c_str())
			);
	SendEditor(SCI_SETTABWIDTH, (DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\BlameTabSize"), 4));
	SendEditor(SCI_SETREADONLY, TRUE);
	LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)this->m_TextView.StringForControl(_T("_99999")).GetBuffer());
	if (m_bShowLine)
		SendEditor(SCI_SETMARGINWIDTHN, 0, pix);
	else
		SendEditor(SCI_SETMARGINWIDTHN, 0);
	SendEditor(SCI_SETMARGINWIDTHN, 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	SendEditor(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	SendEditor(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	SendEditor(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	SendEditor(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	SendEditor(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));
	m_regOldLinesColor = CRegStdDWORD(_T("Software\\TortoiseGit\\BlameOldColor"), RGB(230, 230, 255));
	m_regNewLinesColor = CRegStdDWORD(_T("Software\\TortoiseGit\\BlameNewColor"), RGB(255, 230, 230));
	CRegStdDWORD used2d(L"Software\\TortoiseGit\\ScintillaDirect2D", FALSE);
	if (SysInfo::Instance().IsWin7OrLater() && DWORD(used2d))
	{
		SendEditor(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE);
		SendEditor(SCI_SETBUFFEREDDRAW, 0);
	}
	SendEditor(SCI_SETFONTQUALITY, SC_EFF_QUALITY_DEFAULT);

	this->m_TextView.Call(SCI_SETWRAPMODE, SC_WRAP_NONE);

}

void CTortoiseGitBlameView::StartSearch()
{
	if (m_pFindDialog)
		return;
	bool bCase = false;
	// Initialize FINDREPLACE
	if (fr.Flags & FR_MATCHCASE)
		bCase = true;
	SecureZeroMemory(&fr, sizeof(fr));
	fr.lStructSize = sizeof(fr);
	fr.hwndOwner = wMain;
	fr.lpstrFindWhat = szFindWhat;
	fr.wFindWhatLen = 80;
	fr.Flags = FR_HIDEUPDOWN | FR_HIDEWHOLEWORD;
	fr.Flags |= bCase ? FR_MATCHCASE : 0;

	currentDialog = FindText(&fr);
}

bool CTortoiseGitBlameView::DoSearch(CString what, DWORD flags)
{

	//char szWhat[80];
	int pos = (int)SendEditor(SCI_GETCURRENTPOS);
	int line = (int)SendEditor(SCI_LINEFROMPOSITION, pos);
	bool bFound = false;
	bool bCaseSensitive = !!(flags & FR_MATCHCASE);

	//strcpy_s(szWhat, sizeof(szWhat), what);

	if(!bCaseSensitive)
	{
		what=what.MakeLower();
	}

	//CString sWhat = CString(szWhat);

	//char buf[20];
	//int i=0;
	int i=line;
	do
	{
		int bufsize = (int)SendEditor(SCI_GETLINE, i);
		char * linebuf = new char[bufsize+1];
		SecureZeroMemory(linebuf, bufsize+1);
		SendEditor(SCI_GETLINE, i, (LPARAM)linebuf);
		CString oneline=this->m_TextView.StringFromControl(linebuf);
		if (!bCaseSensitive)
		{
			oneline=oneline.MakeLower();
		}
		//_stprintf_s(buf, 20, _T("%ld"), revs[i]);
		if (this->m_Authors[i].Find(what)>=0)
			bFound = true;
		else if ((!bCaseSensitive)&&(this->m_Authors[i].MakeLower().Find(what)>=0))
			bFound = true;
		else if (oneline.Find(what) >=0)
			bFound = true;

		delete [] linebuf;

		i++;
		if(i>=(signed int)m_CommitHash.size())
			i=0;
	}while(i!=line &&(!bFound));

	if (bFound)
	{
		GotoLine(i);
		int selstart = (int)SendEditor(SCI_GETCURRENTPOS);
		int selend = (int)SendEditor(SCI_POSITIONFROMLINE, i);
		SendEditor(SCI_SETSELECTIONSTART, selstart);
		SendEditor(SCI_SETSELECTIONEND, selend);
		m_SelectedLine = i-1;
	}
	else
	{
		::MessageBox(wMain, _T("\"") + what + _T("\" ") + CString(MAKEINTRESOURCE(IDS_NOTFOUND)), _T("TortoiseGitBlame"), MB_ICONINFORMATION);
	}

	return true;
}

bool CTortoiseGitBlameView::GotoLine(long line)
{
	--line;
	if (line < 0)
		return false;
	if ((unsigned long)line >= m_CommitHash.size())
	{
		line = (long)m_CommitHash.size()-1;
	}

	int nCurrentPos = (int)SendEditor(SCI_GETCURRENTPOS);
	int nCurrentLine = (int)SendEditor(SCI_LINEFROMPOSITION,nCurrentPos);
	int nFirstVisibleLine = (int)SendEditor(SCI_GETFIRSTVISIBLELINE);
	int nLinesOnScreen = (int)SendEditor(SCI_LINESONSCREEN);

	if ( line>=nFirstVisibleLine && line<=nFirstVisibleLine+nLinesOnScreen)
	{
		// no need to scroll
		SendEditor(SCI_GOTOLINE, line);
	}
	else
	{
		// Place the requested line one third from the top
		if ( line > nCurrentLine )
		{
			SendEditor(SCI_GOTOLINE, (WPARAM)(line+(int)nLinesOnScreen*(2/3.0)));
		}
		else
		{
			SendEditor(SCI_GOTOLINE, (WPARAM)(line-(int)nLinesOnScreen*(1/3.0)));
		}
	}

	// Highlight the line
	int nPosStart = (int)SendEditor(SCI_POSITIONFROMLINE,line);
	int nPosEnd = (int)SendEditor(SCI_GETLINEENDPOSITION,line);
	SendEditor(SCI_SETSEL,nPosEnd,nPosStart);

	return true;
}

bool CTortoiseGitBlameView::ScrollToLine(long line)
{
	if (line < 0)
		return false;

	int nCurrentLine = (int)SendEditor(SCI_GETFIRSTVISIBLELINE);

	int scrolldelta = line - nCurrentLine;
	SendEditor(SCI_LINESCROLL, 0, scrolldelta);

	return true;
}

void CTortoiseGitBlameView::CopyToClipboard()
{
	CWnd * wnd = GetFocus();
	if (wnd == this->GetLogList())
		CopySelectedLogToClipboard();
	else if (wnd)
	{
		if (CString(wnd->GetRuntimeClass()->m_lpszClassName) == _T("CMFCPropertyGridCtrl"))
		{
			CMFCPropertyGridCtrl *grid = (CMFCPropertyGridCtrl *)wnd;
			if (grid->GetCurSel() && !grid->GetCurSel()->IsGroup())
				CStringUtils::WriteAsciiStringToClipboard(grid->GetCurSel()->GetValue(), GetSafeHwnd());
		}
		else
			m_TextView.Call(SCI_COPY);
	}
}

void CTortoiseGitBlameView::CopySelectedLogToClipboard()
{
	this->GetLogList()->CopySelectionToClipBoard(FALSE);
}

void CTortoiseGitBlameView::BlamePreviousRevision()
{
	CString procCmd = _T("/path:\"");
	procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
	procCmd += _T("\" ");
	procCmd += _T(" /command:blame");
	procCmd += _T(" /endrev:") + this->GetLogData()->GetGitRevAt(this->GetLogData()->size()-m_ID[m_MouseLine]+1).m_CommitHash.ToString();

	CCommonAppUtils::RunTortoiseProc(procCmd);
}

void CTortoiseGitBlameView::DiffPreviousRevision()
{
	CString procCmd = _T("/path:\"");
	procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
	procCmd += _T("\" ");
	procCmd += _T(" /command:diff");
	procCmd += _T(" /startrev:") + this->GetLogData()->GetGitRevAt(this->GetLogData()->size() - m_ID[m_MouseLine]).m_CommitHash.ToString();
	procCmd += _T(" /endrev:") + this->GetLogData()->GetGitRevAt(this->GetLogData()->size() - m_ID[m_MouseLine] + 1).m_CommitHash.ToString();

	CCommonAppUtils::RunTortoiseProc(procCmd);
}

void CTortoiseGitBlameView::ShowLog()
{
	CString procCmd = _T("/path:\"");
	procCmd += ((CMainFrame*)::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
	procCmd += _T("\" ");
	procCmd += _T(" /command:log");
	procCmd += _T(" /rev:") + this->GetLogData()->GetGitRevAt(this->GetLogData()->size() - m_ID[m_MouseLine]).m_CommitHash.ToString();

	CCommonAppUtils::RunTortoiseProc(procCmd);
}

LONG CTortoiseGitBlameView::GetBlameWidth()
{
	LONG blamewidth = 0;
	SIZE width;
	CreateFont();
	HDC hDC = this->GetDC()->m_hDC;
	HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);

	TCHAR buf[MAX_PATH];

	CString shortHash('f', g_Git.GetShortHASHLength() + 1);
	::GetTextExtentPoint32(hDC, shortHash, g_Git.GetShortHASHLength() + 1, &width);
	m_revwidth = width.cx + BLAMESPACE;
	blamewidth += m_revwidth;

	if (m_bShowDate)
	{
		_stprintf_s(buf, MAX_PATH, _T("%30s"), _T("31.08.2001 06:24:14"));
		::GetTextExtentPoint32(hDC, buf, (int)_tcslen(buf), &width);
		m_datewidth = width.cx + BLAMESPACE;
		blamewidth += m_datewidth;
	}
	if ( m_bShowAuthor)
	{
		SIZE maxwidth = {0};

		for (unsigned int i=0;i<this->m_Authors.size();i++)
		{
			::GetTextExtentPoint32(hDC,m_Authors[i] , m_Authors[i].GetLength(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_authorwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_authorwidth;
	}
	::SelectObject(hDC, oldfont);
	POINT pt = {blamewidth, 0};
	LPtoDP(hDC, &pt, 1);
	m_blamewidth = pt.x;
	//::ReleaseDC(wBlame, hDC);
	return blamewidth;

}

void CTortoiseGitBlameView::CreateFont()
{
	if (m_font)
		return;
	LOGFONT lf = {0};
	lf.lfWeight = 400;
	HDC hDC = ::GetDC(wBlame);
	lf.lfHeight = -MulDiv((DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	lf.lfCharSet = DEFAULT_CHARSET;
	CRegStdString fontname = CRegStdString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New"));
	_tcscpy_s(lf.lfFaceName, 32, ((stdstring)fontname).c_str());
	m_font = ::CreateFontIndirect(&lf);

	lf.lfItalic = TRUE;
	m_italicfont = ::CreateFontIndirect(&lf);

	::ReleaseDC(wBlame, hDC);
}

void CTortoiseGitBlameView::DrawBlame(HDC hDC)
{
	if (hDC == NULL)
		return;
	if (m_font == NULL)
		return;

	HFONT oldfont = NULL;
	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
	LONG_PTR height = SendEditor(SCI_TEXTHEIGHT);
	LONG_PTR Y = 0;
	TCHAR buf[MAX_PATH];
	RECT rc;
	BOOL sel = FALSE;
	//::GetClientRect(this->m_hWnd, &rc);
	for (LRESULT i=line; i<(line+linesonscreen); ++i)
	{
		sel = FALSE;
		if (i < (int)m_CommitHash.size())
		{
		//	if (mergelines[i])
		//		oldfont = (HFONT)::SelectObject(hDC, m_italicfont);
		//	else
				oldfont = (HFONT)::SelectObject(hDC, m_font);
			::SetBkColor(hDC, m_windowcolor);
			::SetTextColor(hDC, m_textcolor);
			if (!m_CommitHash[i].IsEmpty())
			{
				//if (m_CommitHash[i].Compare(m_MouseHash)==0)
				//	::SetBkColor(hDC, m_mouseauthorcolor);
				if (m_CommitHash[i] == m_SelectedHash )
				{
					::SetBkColor(hDC, m_selectedauthorcolor);
					::SetTextColor(hDC, m_texthighlightcolor);
					sel = TRUE;
				}
			}

			if(m_MouseLine == i)
				::SetBkColor(hDC, m_mouserevcolor);

			//if ((revs[i] == m_mouserev)&&(!sel))
			//	::SetBkColor(hDC, m_mouserevcolor);
			//if (revs[i] == m_selectedrev)
			//{
			//	::SetBkColor(hDC, m_selectedrevcolor);
			//	::SetTextColor(hDC, m_texthighlightcolor);
			//}

			CString str;
			str = m_CommitHash[i].ToString().Left(g_Git.GetShortHASHLength());

			//_stprintf_s(buf, MAX_PATH, _T("%8ld       "), revs[i]);
			rc.top = (LONG)Y;
			rc.left=LOCATOR_WIDTH;
			rc.bottom = (LONG)(Y + height);
			rc.right = rc.left + m_blamewidth;
			::ExtTextOut(hDC, LOCATOR_WIDTH, (int)Y, ETO_CLIPPED, &rc, str, str.GetLength(), 0);
			int Left = m_revwidth;

			if (m_bShowAuthor)
			{
				rc.right = rc.left + Left + m_authorwidth;
				//_stprintf_s(buf, MAX_PATH, _T("%-30s            "), authors[i].c_str());
				::ExtTextOut(hDC, Left, (int)Y, ETO_CLIPPED, &rc, m_Authors[i], m_Authors[i].GetLength(), 0);
				Left += m_authorwidth;
			}
#if 0
			if (ShowDate)
			{
				rc.right = rc.left + Left + m_datewidth;
				_stprintf_s(buf, MAX_PATH, _T("%30s            "), dates[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_datewidth;
			}

#endif
			if ((i==m_SelectedLine)&&(m_pFindDialog))
			{
				LOGBRUSH brush;
				brush.lbColor = m_textcolor;
				brush.lbHatch = 0;
				brush.lbStyle = BS_SOLID;
				HPEN pen = ExtCreatePen(PS_SOLID | PS_GEOMETRIC, 2, &brush, 0, NULL);
				HGDIOBJ hPenOld = SelectObject(hDC, pen);
				RECT rc2 = rc;
				rc2.top = (LONG)Y;
				rc2.bottom = (LONG)(Y + height);
				::MoveToEx(hDC, rc2.left, rc2.top, NULL);
				::LineTo(hDC, rc2.right, rc2.top);
				::LineTo(hDC, rc2.right, rc2.bottom);
				::LineTo(hDC, rc2.left, rc2.bottom);
				::LineTo(hDC, rc2.left, rc2.top);
				SelectObject(hDC, hPenOld);
				DeleteObject(pen);
			}
			Y += height;
			::SelectObject(hDC, oldfont);
		}
		else
		{
			::SetBkColor(hDC, m_windowcolor);
			for (int j=0; j< MAX_PATH; ++j)
				buf[j]=' ';
			::ExtTextOut(hDC, 0, (int)Y, ETO_CLIPPED, &rc, buf, MAX_PATH-1, 0);
			Y += height;
		}
	}
}

void CTortoiseGitBlameView::DrawLocatorBar(HDC hDC)
{
	if (hDC == NULL)
		return;

	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
	LONG_PTR Y = 0;
	COLORREF blackColor = GetSysColor(COLOR_WINDOWTEXT);

	RECT rc;
	//::GetClientRect(wLocator, &rc);
	this->GetClientRect(&rc);

	rc.right=LOCATOR_WIDTH;

	RECT lineRect = rc;
	LONG height = rc.bottom-rc.top;
	LONG currentLine = 0;

	// draw the colored bar
	for (std::vector<LONG>::const_iterator it = m_ID.begin(); it != m_ID.end(); ++it)
	{
		currentLine++;
		// get the line color
		COLORREF cr = InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (*it - m_lowestrev)*100/((m_highestrev-m_lowestrev)+1));
		if ((currentLine > line)&&(currentLine <= (line + linesonscreen)))
		{
			cr = InterColor(cr, blackColor, 10);
		}
		SetBkColor(hDC, cr);
		lineRect.top = (LONG)Y;
		lineRect.bottom = (currentLine * height / (LONG)m_ID.size());
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		Y = lineRect.bottom;
	}

	if (!m_ID.empty())
	{
		// now draw two lines indicating the scroll position of the source view
		SetBkColor(hDC, blackColor);
		lineRect.top = (LONG)line * height / (LONG)m_ID.size();
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		lineRect.top = (LONG)(line + linesonscreen) * height / (LONG)m_ID.size();
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
	}

}

void CTortoiseGitBlameView::StringExpand(LPSTR str)
{
	char * cPos = str;
	do
	{
		cPos = strchr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, strlen(cPos)*sizeof(char));
			*cPos = '\r';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}
void CTortoiseGitBlameView::StringExpand(LPWSTR str)
{
	wchar_t * cPos = str;
	do
	{
		cPos = wcschr(cPos, '\n');
		if (cPos)
		{
			memmove(cPos+1, cPos, wcslen(cPos)*sizeof(wchar_t));
			*cPos = '\r';
			cPos++;
			cPos++;
		}
	} while (cPos != NULL);
}

void CTortoiseGitBlameView::SetupLexer(CString filename)
{

	TCHAR *line;
	//const char * lineptr = _tcsrchr(filename, '.');
	int start=filename.ReverseFind(_T('.'));
	if (start>0)
	{
		//_tcscpy_s(line, 20, lineptr+1);
		//_tcslwr_s(line, 20);
		CString ext=filename.Right(filename.GetLength()-start-1);
		line=ext.GetBuffer();

		if ((_tcscmp(line, _T("py"))==0)||
			(_tcscmp(line, _T("pyw"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PYTHON);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield")).GetBuffer()));
			SetAStyle(SCE_P_DEFAULT, black);
			SetAStyle(SCE_P_COMMENTLINE, darkGreen);
			SetAStyle(SCE_P_NUMBER, RGB(0, 0x80, 0x80));
			SetAStyle(SCE_P_STRING, RGB(0, 0, 0x80));
			SetAStyle(SCE_P_CHARACTER, RGB(0, 0, 0x80));
			SetAStyle(SCE_P_WORD, RGB(0x80, 0, 0x80));
			SetAStyle(SCE_P_TRIPLE, black);
			SetAStyle(SCE_P_TRIPLEDOUBLE, black);
			SetAStyle(SCE_P_CLASSNAME, darkBlue);
			SetAStyle(SCE_P_DEFNAME, darkBlue);
			SetAStyle(SCE_P_OPERATOR, darkBlue);
			SetAStyle(SCE_P_IDENTIFIER, darkBlue);
			SetAStyle(SCE_P_COMMENTBLOCK, darkGreen);
			SetAStyle(SCE_P_STRINGEOL, red);
		}
		if ((_tcscmp(line, _T("c"))==0)||
			(_tcscmp(line, _T("cc"))==0)||
			(_tcscmp(line, _T("cpp"))==0)||
			(_tcscmp(line, _T("cxx"))==0)||
			(_tcscmp(line, _T("h"))==0)||
			(_tcscmp(line, _T("hh"))==0)||
			(_tcscmp(line, _T("hpp"))==0)||
			(_tcscmp(line, _T("hxx"))==0)||
			(_tcscmp(line, _T("dlg"))==0)||
			(_tcscmp(line, _T("mak"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("and and_eq asm auto bitand bitor bool break \
case catch char class compl const const_cast continue \
default delete do double dynamic_cast else enum explicit export extern false float for \
friend goto if inline int long mutable namespace new not not_eq \
operator or or_eq private protected public \
register reinterpret_cast return short signed sizeof static static_cast struct switch \
template this throw true try typedef typeid typename union unsigned using \
virtual void volatile wchar_t while xor xor_eq")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 3, (LPARAM)(m_TextView.StringForControl(_T("a addindex addtogroup anchor arg attention \
author b brief bug c class code date def defgroup deprecated dontinclude \
e em endcode endhtmlonly endif endlatexonly endlink endverbatim enum example exception \
f$ f[ f] file fn hideinitializer htmlinclude htmlonly \
if image include ingroup internal invariant interface latexonly li line link \
mainpage name namespace nosubgrouping note overload \
p page par param post pre ref relates remarks return retval \
sa section see showinitializer since skip skipline struct subsection \
test throw todo typedef union until \
var verbatim verbinclude version warning weakgroup $ @ \\ & < > # { }")).GetBuffer()));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("cs"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("abstract as base bool break byte case catch char checked class \
const continue decimal default delegate do double else enum \
event explicit extern false finally fixed float for foreach goto if \
implicit in int interface internal is lock long namespace new null \
object operator out override params private protected public \
readonly ref return sbyte sealed short sizeof stackalloc static \
string struct switch this throw true try typeof uint ulong \
unchecked unsafe ushort using virtual void while")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("rc"))==0)||
			(_tcscmp(line, _T("rc2"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("ACCELERATORS ALT AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON \
BEGIN BITMAP BLOCK BUTTON CAPTION CHARACTERISTICS CHECKBOX CLASS \
COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG DIALOGEX DISCARDABLE \
EDITTEXT END EXSTYLE FONT GROUPBOX ICON LANGUAGE LISTBOX LTEXT \
MENU MENUEX MENUITEM MESSAGETABLE POPUP \
PUSHBUTTON RADIOBUTTON RCDATA RTEXT SCROLLBAR SEPARATOR SHIFT STATE3 \
STRINGTABLE STYLE TEXTINCLUDE VALUE VERSION VERSIONINFO VIRTKEY")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("idl"))==0)||
			(_tcscmp(line, _T("odl"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("aggregatable allocate appobject arrays async async_uuid \
auto_handle \
bindable boolean broadcast byte byte_count \
call_as callback char coclass code comm_status \
const context_handle context_handle_noserialize \
context_handle_serialize control cpp_quote custom \
decode default defaultbind defaultcollelem \
defaultvalue defaultvtable dispinterface displaybind dllname \
double dual \
enable_allocate encode endpoint entry enum error_status_t \
explicit_handle \
fault_status first_is float \
handle_t heap helpcontext helpfile helpstring \
helpstringcontext helpstringdll hidden hyper \
id idempotent ignore iid_as iid_is immediatebind implicit_handle \
import importlib in include in_line int __int64 __int3264 interface \
last_is lcid length_is library licensed local long \
max_is maybe message methods midl_pragma \
midl_user_allocate midl_user_free min_is module ms_union \
ncacn_at_dsp ncacn_dnet_nsp ncacn_http ncacn_ip_tcp \
ncacn_nb_ipx ncacn_nb_nb ncacn_nb_tcp ncacn_np \
ncacn_spx ncacn_vns_spp ncadg_ip_udp ncadg_ipx ncadg_mq \
ncalrpc nocode nonbrowsable noncreatable nonextensible notify \
object odl oleautomation optimize optional out out_of_line \
pipe pointer_default pragma properties propget propput propputref \
ptr public \
range readonly ref represent_as requestedit restricted retval \
shape short signed size_is small source strict_context_handle \
string struct switch switch_is switch_type \
transmit_as typedef \
uidefault union unique unsigned user_marshal usesgetlasterror uuid \
v1_enum vararg version void wchar_t wire_marshal")).GetBuffer()));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("java"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("abstract assert boolean break byte case catch char class \
const continue default do double else extends final finally float for future \
generic goto if implements import inner instanceof int interface long \
native new null outer package private protected public rest \
return short static super switch synchronized this throw throws \
transient try var void volatile while")).GetBuffer()));
			SetupCppLexer();
		}
		if (_tcscmp(line, _T("js"))==0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("abstract boolean break byte case catch char class \
const continue debugger default delete do double else enum export extends \
final finally float for function goto if implements import in instanceof \
int interface long native new package private protected public \
return short static super switch synchronized this throw throws \
transient try typeof var void volatile while with")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("pas"))==0)||
			(_tcscmp(line, _T("dpr"))==0)||
			(_tcscmp(line, _T("pp"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PASCAL);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("and array as begin case class const constructor \
destructor div do downto else end except file finally \
for function goto if implementation in inherited \
interface is mod not object of on or packed \
procedure program property raise record repeat \
set shl shr then threadvar to try type unit \
until uses var while with xor")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("as"))==0)||
			(_tcscmp(line, _T("asc"))==0)||
			(_tcscmp(line, _T("jsfl"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("add and break case catch class continue default delete do \
dynamic else eq extends false finally for function ge get gt if implements import in \
instanceof interface intrinsic le lt ne new not null or private public return \
set static super switch this throw true try typeof undefined var void while with")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 1, (LPARAM)(m_TextView.StringForControl(_T("Array Arguments Accessibility Boolean Button Camera Color \
ContextMenu ContextMenuItem Date Error Function Key LoadVars LocalConnection Math \
Microphone Mouse MovieClip MovieClipLoader NetConnection NetStream Number Object \
PrintJob Selection SharedObject Sound Stage String StyleSheet System TextField \
TextFormat TextSnapshot Video Void XML XMLNode XMLSocket \
_accProps _focusrect _global _highquality _parent _quality _root _soundbuftime \
arguments asfunction call capabilities chr clearInterval duplicateMovieClip \
escape eval fscommand getProperty getTimer getURL getVersion gotoAndPlay gotoAndStop \
ifFrameLoaded Infinity -Infinity int isFinite isNaN length loadMovie loadMovieNum \
loadVariables loadVariablesNum maxscroll mbchr mblength mbord mbsubstring MMExecute \
NaN newline nextFrame nextScene on onClipEvent onUpdate ord parseFloat parseInt play \
prevFrame prevScene print printAsBitmap printAsBitmapNum printNum random removeMovieClip \
scroll set setInterval setProperty startDrag stop stopAllSounds stopDrag substring \
targetPath tellTarget toggleHighQuality trace unescape unloadMovie unLoadMovieNum updateAfterEvent")).GetBuffer()));
			SetupCppLexer();
		}
		if ((_tcscmp(line, _T("html"))==0)||
			(_tcscmp(line, _T("htm"))==0)||
			(_tcscmp(line, _T("shtml"))==0)||
			(_tcscmp(line, _T("htt"))==0)||
			(_tcscmp(line, _T("xml"))==0)||
			(_tcscmp(line, _T("asp"))==0)||
			(_tcscmp(line, _T("xsl"))==0)||
			(_tcscmp(line, _T("php"))==0)||
			(_tcscmp(line, _T("xhtml"))==0)||
			(_tcscmp(line, _T("phtml"))==0)||
			(_tcscmp(line, _T("cfm"))==0)||
			(_tcscmp(line, _T("tpl"))==0)||
			(_tcscmp(line, _T("dtd"))==0)||
			(_tcscmp(line, _T("hta"))==0)||
			(_tcscmp(line, _T("htd"))==0)||
			(_tcscmp(line, _T("wxs"))==0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_HTML);
			SendEditor(SCI_SETSTYLEBITS, 7);
			SendEditor(SCI_SETKEYWORDS, 0, (LPARAM)(m_TextView.StringForControl(_T("a abbr acronym address applet area b base basefont \
bdo big blockquote body br button caption center \
cite code col colgroup dd del dfn dir div dl dt em \
fieldset font form frame frameset h1 h2 h3 h4 h5 h6 \
head hr html i iframe img input ins isindex kbd label \
legend li link map menu meta noframes noscript \
object ol optgroup option p param pre q s samp \
script select small span strike strong style sub sup \
table tbody td textarea tfoot th thead title tr tt u ul \
var xml xmlns abbr accept-charset accept accesskey action align alink \
alt archive axis background bgcolor border \
cellpadding cellspacing char charoff charset checked cite \
class classid clear codebase codetype color cols colspan \
compact content coords \
data datafld dataformatas datapagesize datasrc datetime \
declare defer dir disabled enctype event \
face for frame frameborder \
headers height href hreflang hspace http-equiv \
id ismap label lang language leftmargin link longdesc \
marginwidth marginheight maxlength media method multiple \
name nohref noresize noshade nowrap \
object onblur onchange onclick ondblclick onfocus \
onkeydown onkeypress onkeyup onload onmousedown \
onmousemove onmouseover onmouseout onmouseup \
onreset onselect onsubmit onunload \
profile prompt readonly rel rev rows rowspan rules \
scheme scope selected shape size span src standby start style \
summary tabindex target text title topmargin type usemap \
valign value valuetype version vlink vspace width \
text password checkbox radio submit reset \
file hidden image")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 1, (LPARAM)(m_TextView.StringForControl(_T("assign audio block break catch choice clear disconnect else elseif \
emphasis enumerate error exit field filled form goto grammar help \
if initial link log menu meta noinput nomatch object option p paragraph \
param phoneme prompt property prosody record reprompt return s say-as \
script sentence subdialog submit throw transfer value var voice vxml")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 2, (LPARAM)(m_TextView.StringForControl(_T("accept age alphabet anchor application base beep bridge category charset \
classid cond connecttimeout content contour count dest destexpr dtmf dtmfterm \
duration enctype event eventexpr expr expritem fetchtimeout finalsilence \
gender http-equiv id level maxage maxstale maxtime message messageexpr \
method mime modal mode name namelist next nextitem ph pitch range rate \
scope size sizeexpr skiplist slot src srcexpr sub time timeexpr timeout \
transferaudio type value variant version volume xml:lang")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 3, (LPARAM)(m_TextView.StringForControl(_T("and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield")).GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 4, (LPARAM)(m_TextView.StringForControl(_T("and argv as argc break case cfunction class continue declare default do \
die echo else elseif empty enddeclare endfor endforeach endif endswitch \
endwhile e_all e_parse e_error e_warning eval exit extends false for \
foreach function global http_cookie_vars http_get_vars http_post_vars \
http_post_files http_env_vars http_server_vars if include include_once \
list new not null old_function or parent php_os php_self php_version \
print require require_once return static switch stdclass this true var \
xor virtual while __file__ __line__ __sleep __wakeup")).GetBuffer()));

			SetAStyle(SCE_H_TAG, darkBlue);
			SetAStyle(SCE_H_TAGUNKNOWN, red);
			SetAStyle(SCE_H_ATTRIBUTE, darkBlue);
			SetAStyle(SCE_H_ATTRIBUTEUNKNOWN, red);
			SetAStyle(SCE_H_NUMBER, RGB(0x80,0,0x80));
			SetAStyle(SCE_H_DOUBLESTRING, RGB(0,0x80,0));
			SetAStyle(SCE_H_SINGLESTRING, RGB(0,0x80,0));
			SetAStyle(SCE_H_OTHER, RGB(0x80,0,0x80));
			SetAStyle(SCE_H_COMMENT, RGB(0x80,0x80,0));
			SetAStyle(SCE_H_ENTITY, RGB(0x80,0,0x80));

			SetAStyle(SCE_H_TAGEND, darkBlue);
			SetAStyle(SCE_H_XMLSTART, darkBlue);	// <?
			SetAStyle(SCE_H_QUESTION, darkBlue);	// <?
			SetAStyle(SCE_H_XMLEND, darkBlue);		// ?>
			SetAStyle(SCE_H_SCRIPT, darkBlue);		// <script
			SetAStyle(SCE_H_ASP, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <% ... %>
			SetAStyle(SCE_H_ASPAT, RGB(0x4F, 0x4F, 0), RGB(0xFF, 0xFF, 0));	// <%@ ... %>

			SetAStyle(SCE_HB_DEFAULT, black);
			SetAStyle(SCE_HB_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HB_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HB_WORD, darkBlue);
			SendEditor(SCI_STYLESETBOLD, SCE_HB_WORD, 1);
			SetAStyle(SCE_HB_STRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HB_IDENTIFIER, black);

			// This light blue is found in the windows system palette so is safe to use even in 256 colour modes.
			// Show the whole section of VBScript with light blue background
			for (int bstyle=SCE_HB_DEFAULT; bstyle<=SCE_HB_STRINGEOL; bstyle++) {
				SendEditor(SCI_STYLESETFONT, bstyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, bstyle, lightBlue);
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HB_STRINGEOL, RGB(0x7F,0x7F,0xFF));
			SendEditor(SCI_STYLESETFONT, SCE_HB_COMMENTLINE,
				reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));

			SetAStyle(SCE_HBA_DEFAULT, black);
			SetAStyle(SCE_HBA_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HBA_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HBA_WORD, darkBlue);
			SendEditor(SCI_STYLESETBOLD, SCE_HBA_WORD, 1);
			SetAStyle(SCE_HBA_STRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HBA_IDENTIFIER, black);

			// Show the whole section of ASP VBScript with bright yellow background
			for (int bastyle=SCE_HBA_DEFAULT; bastyle<=SCE_HBA_STRINGEOL; bastyle++) {
				SendEditor(SCI_STYLESETFONT, bastyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, bastyle, RGB(0xFF, 0xFF, 0));
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bastyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HBA_STRINGEOL, RGB(0xCF,0xCF,0x7F));
			SendEditor(SCI_STYLESETFONT, SCE_HBA_COMMENTLINE,
				reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));

			// If there is no need to support embedded Javascript, the following code can be dropped.
			// Javascript will still be correctly processed but will be displayed in just the default style.

			SetAStyle(SCE_HJ_START, RGB(0x80,0x80,0));
			SetAStyle(SCE_HJ_DEFAULT, black);
			SetAStyle(SCE_HJ_COMMENT, darkGreen);
			SetAStyle(SCE_HJ_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HJ_COMMENTDOC, darkGreen);
			SetAStyle(SCE_HJ_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HJ_WORD, black);
			SetAStyle(SCE_HJ_KEYWORD, darkBlue);
			SetAStyle(SCE_HJ_DOUBLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJ_SINGLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJ_SYMBOLS, black);

			SetAStyle(SCE_HJA_START, RGB(0x80,0x80,0));
			SetAStyle(SCE_HJA_DEFAULT, black);
			SetAStyle(SCE_HJA_COMMENT, darkGreen);
			SetAStyle(SCE_HJA_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HJA_COMMENTDOC, darkGreen);
			SetAStyle(SCE_HJA_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HJA_WORD, black);
			SetAStyle(SCE_HJA_KEYWORD, darkBlue);
			SetAStyle(SCE_HJA_DOUBLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJA_SINGLESTRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HJA_SYMBOLS, black);

			SetAStyle(SCE_HPHP_DEFAULT, black);
			SetAStyle(SCE_HPHP_HSTRING,  RGB(0x80,0,0x80));
			SetAStyle(SCE_HPHP_SIMPLESTRING,  RGB(0x80,0,0x80));
			SetAStyle(SCE_HPHP_WORD, darkBlue);
			SetAStyle(SCE_HPHP_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HPHP_VARIABLE, red);
			SetAStyle(SCE_HPHP_HSTRING_VARIABLE, red);
			SetAStyle(SCE_HPHP_COMPLEX_VARIABLE, red);
			SetAStyle(SCE_HPHP_COMMENT, darkGreen);
			SetAStyle(SCE_HPHP_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HPHP_OPERATOR, darkBlue);

			// Show the whole section of Javascript with off white background
			for (int jstyle=SCE_HJ_DEFAULT; jstyle<=SCE_HJ_SYMBOLS; jstyle++) {
				SendEditor(SCI_STYLESETFONT, jstyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, jstyle, offWhite);
				SendEditor(SCI_STYLESETEOLFILLED, jstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HJ_STRINGEOL, RGB(0xDF, 0xDF, 0x7F));
			SendEditor(SCI_STYLESETEOLFILLED, SCE_HJ_STRINGEOL, 1);

			// Show the whole section of Javascript with brown background
			for (int jastyle=SCE_HJA_DEFAULT; jastyle<=SCE_HJA_SYMBOLS; jastyle++) {
				SendEditor(SCI_STYLESETFONT, jastyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(_T("Lucida Console")).GetBuffer()));
				SendEditor(SCI_STYLESETBACK, jastyle, RGB(0xDF, 0xDF, 0x7F));
				SendEditor(SCI_STYLESETEOLFILLED, jastyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HJA_STRINGEOL, RGB(0x0,0xAF,0x5F));
			SendEditor(SCI_STYLESETEOLFILLED, SCE_HJA_STRINGEOL, 1);
		}
	}
	else
	{
		SendEditor(SCI_SETLEXER, SCLEX_CPP);
		SetupCppLexer();
	}
	SendEditor(SCI_COLOURISE, 0, -1);

}

void CTortoiseGitBlameView::SetupCppLexer()
{
	SetAStyle(SCE_C_DEFAULT, RGB(0, 0, 0));
	SetAStyle(SCE_C_COMMENT, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTLINE, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOC, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTLINEDOC, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOCKEYWORD, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_COMMENTDOCKEYWORDERROR, RGB(0, 0x80, 0));
	SetAStyle(SCE_C_NUMBER, RGB(0, 0x80, 0x80));
	SetAStyle(SCE_C_WORD, RGB(0, 0, 0x80));
	SendEditor(SCE_C_WORD, 1);
	SetAStyle(SCE_C_STRING, RGB(0x80, 0, 0x80));
	SetAStyle(SCE_C_IDENTIFIER, RGB(0, 0, 0));
	SetAStyle(SCE_C_PREPROCESSOR, RGB(0x80, 0, 0));
	SetAStyle(SCE_C_OPERATOR, RGB(0x80, 0x80, 0));
}

int CTortoiseGitBlameView::GetEncode(unsigned char *buff, int size, int *bomoffset)
{
	CFileTextLines textlines;
	CFileTextLines::UnicodeType type = textlines.CheckUnicodeType(buff, size);

	if(type == CFileTextLines::UTF8BOM)
	{
		*bomoffset = 3;
		return CP_UTF8;
	}
	if(type == CFileTextLines::UTF8)
		return CP_UTF8;

	if(type == CFileTextLines::UNICODE_LE)
	{
		*bomoffset = 2;
		return 1200;
	}

	// check for UNICODE_BE does not work, because git blame produces two NUL chars in a row
	if(type == CFileTextLines::BINARY && size > 2 && buff[0] == 0xFE && buff[1] == 0xFF)
	{
		*bomoffset = 2;
		return 1201;
	}

	return GetACP();
}

void CTortoiseGitBlameView::UpdateInfo(int Encode)
{
	BYTE_VECTOR &data = GetDocument()->m_BlameData;
	CString one;
	int pos=0;

	BYTE_VECTOR vector;

	CLogDataVector * pRevs= GetLogData();

	this->m_CommitHash.clear();
	this->m_Authors.clear();
	this->m_ID.clear();
	CString line;

	CreateFont();

	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);

	SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);

	int current = 0;
	int encoding = Encode;
	while( pos>=0 && current >=0 && pos<data.size() )
	{
		current = data.findData((const BYTE*)"\n",1,pos);
		//one=data.Tokenize(_T("\n"),pos);

		bool isbound = ( data[pos] == _T('^') );

		if( (data.size() - pos) >1 && data[pos] == _T('^'))
			pos ++;

		if( data[pos] == 0)
			continue;

		CGitHash hash;
		if(isbound)
		{
			git_init();
			data[pos+39]=0;
			if(git_get_sha1((const char*)&data[pos], hash.m_hash))
			{
				::MessageBox(NULL, _T("Can't get hash"), _T("TortoiseGit"), MB_OK|MB_ICONERROR);
			}

		}
		else
			hash.ConvertFromStrA((char*)&data[pos]);


		int start=0;
		start=data.findData((const BYTE*)")",1,pos + 40);
		if(start>0)
		{

			int bomoffset = 0;
			CStringA stra;
			stra.Empty();

			if(current>=0)
				data[current] = 0;
			else
				data.push_back(0);

			if( pos <40 && encoding==0)
			{
				// first line
				encoding = GetEncode(&data[start + 2], (int)(data.size() - start - 2), &bomoffset);
			}
			{
				if(encoding == 1201)
				{
					CString strw;
					DWORD size = ((current - start -2 - bomoffset)/2);
					TCHAR *buffer = strw.GetBuffer(size);
					memcpy(buffer, &data[start + 2 + bomoffset],sizeof(TCHAR)*size);
					// swap the bytes to little-endian order to get proper strings in wchar_t format
					wchar_t * pSwapBuf = buffer;
					for (DWORD i = 0; i<size; ++i)
					{
						*pSwapBuf = WideCharSwap2(*pSwapBuf);
						++pSwapBuf;
					}
					strw.ReleaseBuffer();

					stra = CUnicodeUtils::GetUTF8(strw);
				}
				else if(encoding == 1200)
				{
					CString strw;
					// the first bomoffset is 2, after that it's 1 (see issue #920)
					if (bomoffset == 0)
						bomoffset = 1;
					int size = ((current - start -2 - bomoffset)/2);
					TCHAR *buffer = strw.GetBuffer(size);
					memcpy(buffer, &data[start + 2 + bomoffset],sizeof(TCHAR)*size);
					strw.ReleaseBuffer();

					stra = CUnicodeUtils::GetUTF8(strw);
				}
				else if(encoding == CP_UTF8)
				{
					stra =  &data[start + 2 + bomoffset ];
				}
				else
				{
					CString strw;
					strw = CUnicodeUtils::GetUnicode(CStringA(&data[start + 2 + bomoffset ]), encoding);
					stra = CUnicodeUtils::GetUTF8(strw);
				}
			}

			SendEditor(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)stra);
			SendEditor(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)"\n\0\0\0");

			if(current>=0)
				data[current] = '\n';

		}

		if(this->m_NoListCommit.find(hash) == m_NoListCommit.end() )
		{
			this->m_NoListCommit[hash].GetCommitFromHash(hash);
		}
		m_ID.push_back(-1); // m_ID is calculated lazy on demand
		m_Authors.push_back(m_NoListCommit[hash].GetAuthorName());

		m_CommitHash.push_back(hash);
		pos = current+1;
	}

#if 0
	if(m_Buffer)
	{
		delete m_Buffer;
		m_Buffer=NULL;
	}

	CFile file;
	file.Open(this->GetDocument()->m_TempFileName,CFile::modeRead);

	m_Buffer = new char[file.GetLength()+4];
	m_Buffer[file.GetLength()] =0;
	m_Buffer[file.GetLength()+1] =0;
	m_Buffer[file.GetLength()+2] =0;
	m_Buffer[file.GetLength()+3] =0;

	file.Read(m_Buffer, file.GetLength());

	int bomoffset =0;
	int encoding = GetEncode( (unsigned char *)m_Buffer, file.GetLength(), &bomoffset);

	file.Close();
	//SendEditor(SCI_SETCODEPAGE, encoding);

	//SendEditor(SCI_REPLACESEL, 0, (LPARAM)(LPCSTR)(m_Buffer + bomoffset));
#endif
	SetupLexer(GetDocument()->m_CurrentFileName);

	SendEditor(SCI_SETUNDOCOLLECTION, 1);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETSCROLLWIDTHTRACKING, TRUE);
	SendEditor(SCI_SETREADONLY, TRUE);

	m_lowestrev=0;
	m_highestrev = (long)(this->GetLogData()->size() + m_NoListCommit.size());

	GetBlameWidth();
	CRect rect;
	this->GetClientRect(rect);
	//this->m_TextView.GetWindowRect(rect);
	//this->m_TextView.ScreenToClient(rect);
	rect.left=this->m_blamewidth;
	this->m_TextView.MoveWindow(rect);

	this->Invalidate();
}

CGitBlameLogList * CTortoiseGitBlameView::GetLogList()
{
	return &(GetDocument()->GetMainFrame()->m_wndOutput.m_LogList);
}


CLogDataVector * CTortoiseGitBlameView::GetLogData()
{
	return &(GetDocument()->GetMainFrame()->m_wndOutput.m_LogList.m_logEntries);
}

void CTortoiseGitBlameView::OnSciPainted(NMHDR *,LRESULT *)
{
	this->Invalidate();
}

void CTortoiseGitBlameView::OnLButtonDown(UINT nFlags,CPoint point)
{

	LONG line = (LONG)SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG height = (LONG)SendEditor(SCI_TEXTHEIGHT);
	line = line + (point.y/height);

	if (line < (LONG)m_CommitHash.size())
	{
		SetSelectedLine(line);
		if (m_CommitHash[line] != m_SelectedHash)
		{
			m_SelectedHash = m_CommitHash[line];

			// lazy calculate m_ID
			if (m_ID[line] == -1)
			{
				m_ID[line] = -2; // don't do this lazy calculation again and again for unfindable hashes
				for(int i = 0; i<this->GetLogData()->size(); i++)
				{
					if(m_SelectedHash == this->GetLogData()->at(i))
					{
						m_ID[line] = (LONG)(this->GetLogData()->size() - i);
						break;
					}
				}
			}

			if(m_ID[line]>=0)
			{
				this->GetLogList()->SetItemState(this->GetLogList()->GetItemCount()-m_ID[line],
															LVIS_SELECTED,
															LVIS_SELECTED);
				this->GetLogList()->EnsureVisible(this->GetLogList()->GetItemCount()-m_ID[line], FALSE);
			}
			else
			{
				this->GetDocument()->GetMainFrame()->m_wndProperties.UpdateProperties(&m_NoListCommit[m_CommitHash[line]]);
			}
		}
		else
		{
			m_SelectedHash.Empty();
		}
		//::InvalidateRect( NULL, FALSE);
		this->Invalidate();
		this->m_TextView.Invalidate();

	}
	else
	{
		SetSelectedLine(-1);
	}

	CView::OnLButtonDown(nFlags,point);
}

void CTortoiseGitBlameView::OnSciGetBkColor(NMHDR* hdr, LRESULT* result)
{

	SCNotification *notification=reinterpret_cast<SCNotification *>(hdr);

	if ((m_colorage)&&(notification->line < (int)m_CommitHash.size()))
	{
		if(m_CommitHash[notification->line] == this->m_SelectedHash )
			notification->lParam = m_selectedauthorcolor;
		else
			notification->lParam = InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (m_ID[notification->line]-m_lowestrev)*100/((m_highestrev-m_lowestrev)+1));
	}

}

void CTortoiseGitBlameView::FocusOn(GitRev *pRev)
{
	this->GetDocument()->GetMainFrame()->m_wndProperties.UpdateProperties(pRev);

	this->Invalidate();

	if (m_SelectedHash != pRev->m_CommitHash) {
		m_SelectedHash = pRev->m_CommitHash;
		int i;
		for(i=0;i<m_CommitHash.size();i++)
		{
			if( pRev->m_CommitHash == m_CommitHash[i] )
				break;
		}
		this->GotoLine(i);
		this->m_TextView.Invalidate();
	}
}

void CTortoiseGitBlameView::OnMouseHover(UINT nFlags, CPoint point)
{
	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR height = SendEditor(SCI_TEXTHEIGHT);
	line = line + (point.y/height);

	if (line < (LONG)m_CommitHash.size())
	{
		if (line != m_MouseLine)
		{
			m_MouseLine = (LONG)line;//m_CommitHash[line];
			GitRev *pRev;
			if(m_ID[line]<0)
			{
				pRev=&this->m_NoListCommit[m_CommitHash[line]];

			}
			else
			{
				pRev=&this->GetLogData()->GetGitRevAt(this->GetLogList()->GetItemCount()-m_ID[line]);
			}

			CString str;
			str.Format(_T("%s: %s\n%s: %s <%s>\n%s: %s\n%s:\n%s\n%s"),	m_sRev, pRev->m_CommitHash.ToString(),
																	m_sAuthor, pRev->GetAuthorName(), pRev->GetAuthorEmail(),
																	m_sDate, CLoglistUtils::FormatDateAndTime(pRev->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes),
																	m_sMessage, pRev->GetSubject(),
																	pRev->GetBody());

			m_ToolTip.Pop();
			m_ToolTip.AddTool(this, str);

			CRect rect;
			rect.left=LOCATOR_WIDTH;
			rect.right=this->m_blamewidth+rect.left;
			rect.top = point.y - (LONG)height;
			rect.bottom = point.y + (LONG)height;
			this->InvalidateRect(rect);
		}
	}
}

void CTortoiseGitBlameView::OnMouseMove(UINT nFlags, CPoint point)
{
	TRACKMOUSEEVENT tme;
	tme.cbSize=sizeof(TRACKMOUSEEVENT);
	tme.dwFlags=TME_HOVER|TME_LEAVE;
	tme.hwndTrack=this->m_hWnd;
	tme.dwHoverTime=1;
	TrackMouseEvent(&tme);
}


BOOL CTortoiseGitBlameView::PreTranslateMessage(MSG* pMsg)
{
	m_ToolTip.RelayEvent(pMsg);
	return CView::PreTranslateMessage(pMsg);
}

void CTortoiseGitBlameView::OnEditFind()
{
	m_pFindDialog=new CFindReplaceDialog();

	m_pFindDialog->Create(TRUE,_T(""),NULL,FR_DOWN,this);
}

void CTortoiseGitBlameView::OnEditGoto()
{
	CEditGotoDlg dlg;
	if(dlg.DoModal()==IDOK)
	{
		this->GotoLine(dlg.m_LineNumber);
	}
}

LRESULT CTortoiseGitBlameView::OnFindDialogMessage(WPARAM wParam, LPARAM lParam)
{
	ASSERT(m_pFindDialog != NULL);

	// If the FR_DIALOGTERM flag is set,
	// invalidate the handle identifying the dialog box.
	if (m_pFindDialog->IsTerminating())
	{
			m_pFindDialog = NULL;
			return 0;
	}

	// If the FR_FINDNEXT flag is set,
	// call the application-defined search routine
	// to search for the requested string.
	if(m_pFindDialog->FindNext())
	{
		//read data from dialog
		CString FindName = m_pFindDialog->GetFindString();
		bool bMatchCase = m_pFindDialog->MatchCase() == TRUE;
		bool bMatchWholeWord = m_pFindDialog->MatchWholeWord() == TRUE;
		bool bSearchDown = m_pFindDialog->SearchDown() == TRUE;

		DoSearch(FindName,m_pFindDialog->m_fr.Flags);
		//with given name do search
		// *FindWhatYouNeed(FindName, bMatchCase, bMatchWholeWord, bSearchDown);
	}

	return 0;
}

void CTortoiseGitBlameView::OnViewNext()
{
	FindNextLine(this->m_SelectedHash,false);
}
void CTortoiseGitBlameView::OnViewPrev()
{
	FindNextLine(this->m_SelectedHash,true);
}

void CTortoiseGitBlameView::OnViewToggleAuthor()
{
	m_bShowAuthor = ! m_bShowAuthor;

	theApp.WriteInt(_T("ShowAuthor"), m_bShowAuthor);

	CRect rect;
	this->GetClientRect(&rect);
	rect.left=GetBlameWidth();

	m_TextView.MoveWindow(&rect);
}

void CTortoiseGitBlameView::OnUpdateViewToggleAuthor(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bShowAuthor);
}

void CTortoiseGitBlameView::OnViewToggleFollowRenames()
{
	m_bFollowRenames = ! m_bFollowRenames;

	theApp.WriteInt(_T("FollowRenames"), m_bFollowRenames);

	UINT uCheck = MF_BYCOMMAND;
	uCheck |= m_bFollowRenames ? MF_CHECKED : MF_UNCHECKED;
	CheckMenuItem(GetMenu()->m_hMenu, ID_VIEW_FOLLOWRENAMES, uCheck);

	CTortoiseGitBlameDoc *document = (CTortoiseGitBlameDoc *) m_pDocument;
	if (!document->m_CurrentFileName.IsEmpty())
	{
		theApp.m_pDocManager->OnFileNew();
		document->OnOpenDocument(document->m_CurrentFileName, document->m_Rev);
	}
}

void CTortoiseGitBlameView::OnUpdateViewToggleFollowRenames(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bFollowRenames);
}

void CTortoiseGitBlameView::OnUpdateViewCopyToClipboard(CCmdUI *pCmdUI)
{
	CWnd * wnd = GetFocus();
	if (wnd == GetLogList())
	{
		pCmdUI->Enable(GetLogList()->GetSelectedCount() > 0);
	}
	else if (wnd)
	{
		if (CString(wnd->GetRuntimeClass()->m_lpszClassName) == _T("CMFCPropertyGridCtrl"))
		{
			CMFCPropertyGridCtrl *grid = (CMFCPropertyGridCtrl *)wnd;
			pCmdUI->Enable(grid->GetCurSel() && !grid->GetCurSel()->IsGroup() && !CString(grid->GetCurSel()->GetValue()).IsEmpty());
		}
		else
			pCmdUI->Enable(m_TextView.Call(SCI_GETSELECTIONSTART) != m_TextView.Call(SCI_GETSELECTIONEND));
	}
	else
		pCmdUI->Enable(FALSE);
}

int CTortoiseGitBlameView::FindNextLine(CGitHash CommitHash,bool bUpOrDown)
{
	LONG line = (LONG)SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG startline = line;
	bool findNoMatch =false;
	while(line>=0 && line<m_CommitHash.size())
	{
		if(m_CommitHash[line]!=CommitHash)
		{
			findNoMatch=true;
		}

		if(m_CommitHash[line] == CommitHash && findNoMatch)
		{
			if( line == startline+2 )
			{
				findNoMatch=false;
			}
			else
			{
				if( bUpOrDown )
				{
					line=FindFirstLine(CommitHash,line);
				}
				SendEditor(SCI_LINESCROLL,0,line-startline-2);
				return line;
			}
		}
		if(bUpOrDown)
			line--;
		else
			line++;
	}
	return -1;
}
