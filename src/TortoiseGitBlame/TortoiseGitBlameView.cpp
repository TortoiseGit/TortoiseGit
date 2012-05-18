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

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

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
	ON_COMMAND(ID_EDIT_COPY,CopySelectedLogToClipboard)
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
	m_pathwidth = 0;
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
			m_MouseLine = line;
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
	this->GetLogList()->CopySelectionToClipBoard(TRUE);
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

void CTortoiseGitBlameView::GetRange(int start, int end, char *text)
{
#if 0
	TEXTRANGE tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;

	SendMessage(m_wEditor, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
#endif
}

void CTortoiseGitBlameView::SetTitle()
{
#if 0
	char title[MAX_PATH + 100];
	strcpy_s(title, MAX_PATH + 100, szTitle);
	strcat_s(title, MAX_PATH + 100, " - ");
	strcat_s(title, MAX_PATH + 100, szViewtitle);
	::SetWindowText(wMain, title);
#endif
}

BOOL CTortoiseGitBlameView::OpenLogFile(const char *fileName)
{
#if 0
	char logmsgbuf[10000+1];
	FILE * File;
	fopen_s(&File, fileName, "rb");
	if (File == 0)
	{
		return FALSE;
	}
	LONG rev = 0;
	CString msg;
	int slength = 0;
	int reallength = 0;
	size_t len = 0;
	wchar_t wbuf[MAX_LOG_LENGTH+6];
	for (;;)
	{
		len = fread(&rev, sizeof(LONG), 1, File);
		if (len == 0)
		{
			fclose(File);
			InitSize();
			return TRUE;
		}
		len = fread(&slength, sizeof(int), 1, File);
		if (len == 0)
		{
			fclose(File);
			InitSize();
			return FALSE;
		}
		if (slength > MAX_LOG_LENGTH)
		{
			reallength = slength;
			slength = MAX_LOG_LENGTH;
		}
		else
			reallength = 0;
		len = fread(logmsgbuf, sizeof(char), slength, File);
		if (len < (size_t)slength)
		{
			fclose(File);
			InitSize();
			return FALSE;
		}
		msg = CString(logmsgbuf, slength);
		if (reallength)
		{
			fseek(File, reallength-MAX_LOG_LENGTH, SEEK_CUR);
			msg = msg + _T("\n...");
		}
		int len2 = ::MultiByteToWideChar(CP_UTF8, NULL, msg.c_str(), min(msg.size(), MAX_LOG_LENGTH+5), wbuf, MAX_LOG_LENGTH+5);
		wbuf[len2] = 0;
		len2 = ::WideCharToMultiByte(CP_ACP, NULL, wbuf, len2, logmsgbuf, MAX_LOG_LENGTH+5, NULL, NULL);
		logmsgbuf[len2] = 0;
		msg = CString(logmsgbuf);
		logmessages[rev] = msg;
	}
#endif
	return TRUE;
}

BOOL CTortoiseGitBlameView::OpenFile(const char *fileName)
{
#if 0
	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SetTitle();
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);
	::ShowWindow(m_wEditor, SW_HIDE);
	std::ifstream File;
	File.open(fileName);
	if (!File.good())
	{
		return FALSE;
	}
	char line[100*1024];
	char * lineptr = NULL;
	char * trimptr = NULL;
	//ignore the first two lines, they're of no interest to us
	File.getline(line, _countof(line));
	File.getline(line, _countof(line));
	m_lowestrev = LONG_MAX;
	m_highestrev = 0;
	bool bUTF8 = true;
	do
	{
		File.getline(line, _countof(line));
		if (File.gcount()>139)
		{
			mergelines.push_back((line[0] != ' '));
			lineptr = &line[9];
			long rev = _ttol(lineptr);
			revs.push_back(rev);
			m_lowestrev = min(m_lowestrev, rev);
			m_highestrev = max(m_highestrev, rev);
			lineptr += 7;
			rev = _ttol(lineptr);
			origrevs.push_back(rev);
			lineptr += 7;
			dates.push_back(CString(lineptr, 30));
			lineptr += 31;
			// unfortunately, the 'path' entry can be longer than the 60 chars
			// we made the column. We therefore have to step through the path
			// string until we find a space
			trimptr = lineptr;
			do
			{
				// TODO: how can we deal with the situation where the path has
				// a space in it, but the space is after the 60 chars reserved
				// for it?
				// The only way to deal with that would be to use a custom
				// binary format for the blame file.
				trimptr++;
				trimptr = _tcschr(trimptr, ' ');
			} while ((trimptr)&&(trimptr+1 < lineptr+61));
			if (trimptr)
				*trimptr = 0;
			else
				trimptr = lineptr;
			paths.push_back(CString(lineptr));
			if (trimptr+1 < lineptr+61)
				lineptr +=61;
			else
				lineptr = (trimptr+1);
			trimptr = lineptr+30;
			while ((*trimptr == ' ')&&(trimptr > lineptr))
				trimptr--;
			*(trimptr+1) = 0;
			authors.push_back(CString(lineptr));
			lineptr += 31;
			// in case we find an UTF8 BOM at the beginning of the line, we remove it
			if (((unsigned char)lineptr[0] == 0xEF)&&((unsigned char)lineptr[1] == 0xBB)&&((unsigned char)lineptr[2] == 0xBF))
			{
				lineptr += 3;
			}
			if (((unsigned char)lineptr[0] == 0xBB)&&((unsigned char)lineptr[1] == 0xEF)&&((unsigned char)lineptr[2] == 0xBF))
			{
				lineptr += 3;
			}
			// check each line for illegal utf8 sequences. If one is found, we treat
			// the file as ASCII, otherwise we assume an UTF8 file.
			char * utf8CheckBuf = lineptr;
			while ((bUTF8)&&(*utf8CheckBuf))
			{
				if ((*utf8CheckBuf == 0xC0)||(*utf8CheckBuf == 0xC1)||(*utf8CheckBuf >= 0xF5))
				{
					bUTF8 = false;
					break;
				}
				if ((*utf8CheckBuf & 0xE0)==0xC0)
				{
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
				}
				if ((*utf8CheckBuf & 0xF0)==0xE0)
				{
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
				}
				if ((*utf8CheckBuf & 0xF8)==0xF0)
				{
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
					utf8CheckBuf++;
					if (*utf8CheckBuf == 0)
						break;
					if ((*utf8CheckBuf & 0xC0)!=0x80)
					{
						bUTF8 = false;
						break;
					}
				}

				utf8CheckBuf++;
			}
			SendEditor(SCI_ADDTEXT, _tcslen(lineptr), reinterpret_cast<LPARAM>(static_cast<char *>(lineptr)));
			SendEditor(SCI_ADDTEXT, 2, (LPARAM)_T("\r\n"));
		}
	} while (File.gcount() > 0);

	if (bUTF8)
		SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);

	SendEditor(SCI_SETUNDOCOLLECTION, 1);
	::SetFocus(m_wEditor);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETSCROLLWIDTHTRACKING, TRUE);
	SendEditor(SCI_SETREADONLY, TRUE);

	//check which lexer to use, depending on the filetype
	SetupLexer(fileName);
	::ShowWindow(m_wEditor, SW_SHOW);
	m_blamewidth = 0;
	::InvalidateRect(wMain, NULL, TRUE);
	RECT rc;
	GetWindowRect(wMain, &rc);
	SetWindowPos(wMain, 0, rc.left, rc.top, rc.right-rc.left-1, rc.bottom - rc.top, 0);
#endif
	return TRUE;
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
	if (SysInfo::Instance().IsWin7OrLater())
	{
		SendEditor(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITE);
		SendEditor(SCI_SETBUFFEREDDRAW, 0);
	}
	SendEditor(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);

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
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int line = SendEditor(SCI_LINEFROMPOSITION, pos);
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
		int bufsize = SendEditor(SCI_GETLINE, i);
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
		int selstart = SendEditor(SCI_GETCURRENTPOS);
		int selend = SendEditor(SCI_POSITIONFROMLINE, i);
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
		line = m_CommitHash.size()-1;
	}

	int nCurrentPos = SendEditor(SCI_GETCURRENTPOS);
	int nCurrentLine = SendEditor(SCI_LINEFROMPOSITION,nCurrentPos);
	int nFirstVisibleLine = SendEditor(SCI_GETFIRSTVISIBLELINE);
	int nLinesOnScreen = SendEditor(SCI_LINESONSCREEN);

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
	int nPosStart = SendEditor(SCI_POSITIONFROMLINE,line);
	int nPosEnd = SendEditor(SCI_GETLINEENDPOSITION,line);
	SendEditor(SCI_SETSEL,nPosEnd,nPosStart);

	return true;
}

bool CTortoiseGitBlameView::ScrollToLine(long line)
{
	if (line < 0)
		return false;

	int nCurrentLine = SendEditor(SCI_GETFIRSTVISIBLELINE);

	int scrolldelta = line - nCurrentLine;
	SendEditor(SCI_LINESCROLL, 0, scrolldelta);

	return true;
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

void CTortoiseGitBlameView::Notify(SCNotification *notification)
{
	switch (notification->nmhdr.code)
	{
	case SCN_SAVEPOINTREACHED:
		break;

	case SCN_SAVEPOINTLEFT:
		break;
	case SCN_PAINTED:
//		InvalidateRect(wBlame, NULL, FALSE);
//		InvalidateRect(wLocator, NULL, FALSE);
		break;
	case SCN_GETBKCOLOR:
//		if ((m_colorage)&&(notification->line < (int)revs.size()))
//		{
//			notification->lParam = InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (revs[notification->line]-m_lowestrev)*100/((m_highestrev-m_lowestrev)+1));
//		}
		break;
	}
}

void CTortoiseGitBlameView::Command(int id)
{
#if 0
	switch (id)
	{
//	case IDM_EXIT:
//		::PostQuitMessage(0);
//		break;
	case ID_EDIT_FIND:
		StartSearch();
		break;
	case ID_COPYTOCLIPBOARD:
		CopySelectedLogToClipboard();
		break;
	case ID_BLAME_PREVIOUS_REVISION:
		BlamePreviousRevision();
		break;
	case ID_DIFF_PREVIOUS_REVISION:
		DiffPreviousRevision();
		break;
	case ID_SHOWLOG:
		ShowLog();
		break;
	case ID_EDIT_GOTOLINE:
		GotoLineDlg();
		break;
	case ID_VIEW_COLORAGEOFLINES:
		{
			m_colorage = !m_colorage;
			HMENU hMenu = GetMenu(wMain);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= m_colorage ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_COLORAGEOFLINES, uCheck);
			m_blamewidth = 0;
			InitSize();
		}
		break;
	case ID_VIEW_MERGEPATH:
		{
			ShowPath = !ShowPath;
			HMENU hMenu = GetMenu(wMain);
			UINT uCheck = MF_BYCOMMAND;
			uCheck |= ShowPath ? MF_CHECKED : MF_UNCHECKED;
			CheckMenuItem(hMenu, ID_VIEW_MERGEPATH, uCheck);
			m_blamewidth = 0;
			InitSize();
		}
	default:
		break;
	};
#endif
}


LONG CTortoiseGitBlameView::GetBlameWidth()
{
	LONG blamewidth = 0;
	SIZE width;
	CreateFont();
	HDC hDC = this->GetDC()->m_hDC;
	HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);

	TCHAR buf[MAX_PATH];

	::GetTextExtentPoint32(hDC, _T("fffffff"), 7, &width);
	m_revwidth = width.cx + BLAMESPACE;
	blamewidth += m_revwidth;

#if 0
	_stprintf_s(buf, MAX_PATH, _T("%d"), m_CommitHash.size());
	::GetTextExtentPoint(hDC, buf, _tcslen(buf), &width);
	m_linewidth = width.cx + BLAMESPACE;
	blamewidth += m_revwidth;
#endif

	if (m_bShowDate)
	{
		_stprintf_s(buf, MAX_PATH, _T("%30s"), _T("31.08.2001 06:24:14"));
		::GetTextExtentPoint32(hDC, buf, _tcslen(buf), &width);
		m_datewidth = width.cx + BLAMESPACE;
		blamewidth += m_datewidth;
	}
	if ( m_bShowAuthor)
	{
		SIZE maxwidth = {0};

		for (unsigned int i=0;i<this->m_Authors.size();i++)
		//for (std::vector<CString>::iterator I = authors.begin(); I != authors.end(); ++I)
		{
			::GetTextExtentPoint32(hDC,m_Authors[i] , m_Authors[i].GetLength(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_authorwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_authorwidth;
	}
#if 0
	if (ShowPath)
	{
		SIZE maxwidth = {0};
		for (std::vector<CString>::iterator I = paths.begin(); I != paths.end(); ++I)
		{
			::GetTextExtentPoint32(hDC, I->c_str(), I->size(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_pathwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_pathwidth;
	}
#endif
	::SelectObject(hDC, oldfont);
	POINT pt = {blamewidth, 0};
	LPtoDP(hDC, &pt, 1);
	m_blamewidth = pt.x;
	//::ReleaseDC(wBlame, hDC);

	//return m_blamewidth;
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

	//m_TextView.SetFont(lf.lfFaceName,((DWORD)CRegStdDWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10)));
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
			rc.top=Y;
			rc.left=LOCATOR_WIDTH;
			rc.bottom=Y+height;
			rc.right = rc.left + m_blamewidth;
			::ExtTextOut(hDC, LOCATOR_WIDTH, Y, ETO_CLIPPED, &rc, str, str.GetLength(), 0);
			int Left = m_revwidth;

			if (m_bShowAuthor)
			{
				rc.right = rc.left + Left + m_authorwidth;
				//_stprintf_s(buf, MAX_PATH, _T("%-30s            "), authors[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, m_Authors[i], m_Authors[i].GetLength(), 0);
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
#if 0
			if (ShowPath)
			{
				rc.right = rc.left + Left + m_pathwidth;
				_stprintf_s(buf, MAX_PATH, _T("%-60s            "), paths[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_authorwidth;
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
				rc2.top = Y;
				rc2.bottom = Y + height;
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
			::ExtTextOut(hDC, 0, Y, ETO_CLIPPED, &rc, buf, MAX_PATH-1, 0);
			Y += height;
		}
	}
}

void CTortoiseGitBlameView::DrawHeader(HDC hDC)
{
#if 0
	if (hDC == NULL)
		return;

	RECT rc;
	HFONT oldfont = (HFONT)::SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	::GetClientRect(wHeader, &rc);

	::SetBkColor(hDC, ::GetSysColor(COLOR_BTNFACE));

	TCHAR szText[MAX_LOADSTRING];
	LoadString(app.hResource, IDS_HEADER_REVISION, szText, MAX_LOADSTRING);
	::ExtTextOut(hDC, LOCATOR_WIDTH, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
	int Left = m_revwidth+LOCATOR_WIDTH;
	if (ShowDate)
	{
		LoadString(app.hResource, IDS_HEADER_DATE, szText, MAX_LOADSTRING);
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
		Left += m_datewidth;
	}
	if (ShowAuthor)
	{
		LoadString(app.hResource, IDS_HEADER_AUTHOR, szText, MAX_LOADSTRING);
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
		Left += m_authorwidth;
	}
	if (ShowPath)
	{
		LoadString(app.hResource, IDS_HEADER_PATH, szText, MAX_LOADSTRING);
		::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);
		Left += m_pathwidth;
	}
	LoadString(app.hResource, IDS_HEADER_LINE, szText, MAX_LOADSTRING);
	::ExtTextOut(hDC, Left, 0, ETO_CLIPPED, &rc, szText, _tcslen(szText), 0);

	::SelectObject(hDC, oldfont);
#endif
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
		lineRect.top = Y;
		lineRect.bottom = (currentLine * height / m_ID.size());
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		Y = lineRect.bottom;
	}

	if (m_ID.size())
	{
		// now draw two lines indicating the scroll position of the source view
		SetBkColor(hDC, blackColor);
		lineRect.top = line * height / m_ID.size();
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		lineRect.top = (line + linesonscreen) * height / m_ID.size();
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

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hResource);
ATOM				MyRegisterBlameClass(HINSTANCE hResource);
ATOM				MyRegisterHeaderClass(HINSTANCE hResource);
ATOM				MyRegisterLocatorClass(HINSTANCE hResource);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndBlameProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndHeaderProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK	WndLocatorProc(HWND, UINT, WPARAM, LPARAM);
UINT				uFindReplaceMsg;

#if 0
int APIENTRY _tWinMain(HINSTANCE	hInstance,
					 HINSTANCE		/*hPrevInstance*/,
					 LPTSTR			lpCmdLine,
					 int			nCmdShow)
{
	app.hInstance = hInstance;
	MSG msg;
	HACCEL hAccelTable;

	if (::LoadLibrary("SciLexer.DLL") == NULL)
		return FALSE;

	CRegStdDWORD loc = CRegStdDWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;

	CLangDll langDLL;
	app.hResource = langDLL.Init(_T("CTortoiseGitBlameView"), langId);
	if (app.hResource == NULL)
		app.hResource = app.hInstance;

	// Initialize global strings
	LoadString(app.hResource, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(app.hResource, IDC_TortoiseGitBlameView, szWindowClass, MAX_LOADSTRING);
	LoadString(app.hResource, IDS_SEARCHNOTFOUND, searchstringnotfound, MAX_LOADSTRING);
	MyRegisterClass(app.hResource);
	MyRegisterBlameClass(app.hResource);
	MyRegisterHeaderClass(app.hResource);
	MyRegisterLocatorClass(app.hResource);

	// Perform application initialization:
	if (!InitInstance (app.hResource, nCmdShow))
	{
		langDLL.Close();
		return FALSE;
	}

	SecureZeroMemory(szViewtitle, MAX_PATH);
	SecureZeroMemory(szOrigPath, MAX_PATH);
	char blamefile[MAX_PATH] = {0};
	char logfile[MAX_PATH] = {0};

	CCmdLineParser parser(lpCmdLine);


	if (__argc > 1)
	{
		_tcscpy_s(blamefile, MAX_PATH, __argv[1]);
	}
	if (__argc > 2)
	{
		_tcscpy_s(logfile, MAX_PATH, __argv[2]);
	}
	if (__argc > 3)
	{
		_tcscpy_s(szViewtitle, MAX_PATH, __argv[3]);
		if (parser.HasVal(_T("revrange")))
		{
			_tcscat_s(szViewtitle, MAX_PATH, _T(" : "));
			_tcscat_s(szViewtitle, MAX_PATH, parser.GetVal(_T("revrange")));
		}
	}
	if ((_tcslen(blamefile)==0) || parser.HasKey(_T("?")) || parser.HasKey(_T("help")))
	{
		TCHAR szInfo[MAX_LOADSTRING];
		LoadString(app.hResource, IDS_COMMANDLINE_INFO, szInfo, MAX_LOADSTRING);
		MessageBox(NULL, szInfo, _T("CTortoiseGitBlameView"), MB_ICONERROR);
		langDLL.Close();
		return 0;
	}

	if ( parser.HasKey(_T("path")) )
	{
		_tcscpy_s(szOrigPath, MAX_PATH, parser.GetVal(_T("path")));
	}
	app.bIgnoreEOL = parser.HasKey(_T("ignoreeol"));
	app.bIgnoreSpaces = parser.HasKey(_T("ignorespaces"));
	app.bIgnoreAllSpaces = parser.HasKey(_T("ignoreallspaces"));

	app.SendEditor(SCI_SETCODEPAGE, GetACP());
	app.OpenFile(blamefile);
	if (_tcslen(logfile)>0)
		app.OpenLogFile(logfile);

	if (parser.HasKey(_T("line")))
	{
		app.GotoLine(parser.GetLongVal(_T("line")));
	}

	CheckMenuItem(GetMenu(app.wMain), ID_VIEW_COLORAGEOFLINES, MF_CHECKED|MF_BYCOMMAND);


	hAccelTable = LoadAccelerators(app.hResource, (LPCTSTR)IDC_TortoiseGitBlameView);

	BOOL going = TRUE;
	msg.wParam = 0;
	while (going)
	{
		going = GetMessage(&msg, NULL, 0, 0);
		if (app.currentDialog && going)
		{
			if (!IsDialogMessage(app.currentDialog, &msg))
			{
				if (TranslateAccelerator(msg.hwnd, hAccelTable, &msg) == 0)
				{
					TranslateMessage(&msg);
					DispatchMessage(&msg);
				}
			}
		}
		else if (going)
		{
			if (TranslateAccelerator(app.wMain, hAccelTable, &msg) == 0)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}
	langDLL.Close();
	return msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_TortoiseGitBlameView;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterBlameClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndBlameProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseGitBlameViewBlame");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterHeaderClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndHeaderProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseGitBlameViewHeader");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

ATOM MyRegisterLocatorClass(HINSTANCE hResource)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndLocatorProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hResource;
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TortoiseGitBlameView);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseGitBlameViewLocator");
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

BOOL InitInstance(HINSTANCE hResource, int nCmdShow)
{
	app.wMain = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
							CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hResource, NULL);

	if (!app.wMain)
	{
		return FALSE;
	}

	CRegStdDWORD pos(_T("Software\\TortoiseGit\\TBlamePos"), 0);
	CRegStdDWORD width(_T("Software\\TortoiseGit\\TBlameSize"), 0);
	CRegStdDWORD state(_T("Software\\TortoiseGit\\TBlameState"), 0);
	if (DWORD(pos) && DWORD(width))
	{
		RECT rc;
		rc.left = LOWORD(DWORD(pos));
		rc.top = HIWORD(DWORD(pos));
		rc.right = rc.left + LOWORD(DWORD(width));
		rc.bottom = rc.top + HIWORD(DWORD(width));
		HMONITOR hMon = MonitorFromRect(&rc, MONITOR_DEFAULTTONULL);
		if (hMon)
		{
			// only restore the window position if the monitor is valid
			MoveWindow(app.wMain, LOWORD(DWORD(pos)), HIWORD(DWORD(pos)),
						LOWORD(DWORD(width)), HIWORD(DWORD(width)), FALSE);
		}
	}
	if (DWORD(state) == SW_MAXIMIZE)
		ShowWindow(app.wMain, SW_MAXIMIZE);
	else
		ShowWindow(app.wMain, nCmdShow);
	UpdateWindow(app.wMain);

	//Create the tooltips

	INITCOMMONCONTROLSEX iccex;
	app.hwndTT; // handle to the ToolTip control
	TOOLINFO ti;
	RECT rect; // for client area coordinates
	iccex.dwICC = ICC_WIN95_CLASSES;
	iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
	InitCommonControlsEx(&iccex);

	/* CREATE A TOOLTIP WINDOW */
	app.hwndTT = CreateWindowEx(WS_EX_TOPMOST,
								TOOLTIPS_CLASS,
								NULL,
								WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								CW_USEDEFAULT,
								app.wBlame,
								NULL,
								app.hResource,
								NULL
								);

	SetWindowPos(app.hwndTT,
				HWND_TOPMOST,
				0,
				0,
				0,
				0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

	/* GET COORDINATES OF THE MAIN CLIENT AREA */
	GetClientRect (app.wBlame, &rect);

	/* INITIALIZE MEMBERS OF THE TOOLINFO STRUCTURE */
	ti.cbSize = sizeof(TOOLINFO);
	ti.uFlags = TTF_TRACK | TTF_ABSOLUTE;//TTF_SUBCLASS | TTF_PARSELINKS;
	ti.hwnd = app.wBlame;
	ti.hinst = app.hResource;
	ti.uId = 0;
	ti.lpszText = LPSTR_TEXTCALLBACK;
	// ToolTip control will cover the whole window
	ti.rect.left = rect.left;
	ti.rect.top = rect.top;
	ti.rect.right = rect.right;
	ti.rect.bottom = rect.bottom;

	/* SEND AN ADDTOOL MESSAGE TO THE TOOLTIP CONTROL WINDOW */
	SendMessage(app.hwndTT, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &ti);
	SendMessage(app.hwndTT, TTM_SETMAXTIPWIDTH, 0, 600);
	//SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELONG(50000, 0));
	//SendMessage(app.hwndTT, TTM_SETDELAYTIME, TTDT_RESHOW, MAKELONG(1000, 0));

	uFindReplaceMsg = RegisterWindowMessage(FINDMSGSTRING);

	return TRUE;
}
#endif
void CTortoiseGitBlameView::InitSize()
{
	RECT rc;
	RECT blamerc;
	RECT sourcerc;
	::GetClientRect(wMain, &rc);
	::SetWindowPos(wHeader, 0, rc.left, rc.top, rc.right-rc.left, HEADER_HEIGHT, 0);
	rc.top += HEADER_HEIGHT;
	blamerc.left = rc.left;
	blamerc.top = rc.top;
	LONG w = GetBlameWidth();
	blamerc.right = w > abs(rc.right - rc.left) ? rc.right : w + rc.left;
	blamerc.bottom = rc.bottom;
	sourcerc.left = blamerc.right;
	sourcerc.top = rc.top;
	sourcerc.bottom = rc.bottom;
	sourcerc.right = rc.right;
	if (m_colorage)
	{
		::OffsetRect(&blamerc, LOCATOR_WIDTH, 0);
		::OffsetRect(&sourcerc, LOCATOR_WIDTH, 0);
		sourcerc.right -= LOCATOR_WIDTH;
	}
	::InvalidateRect(wMain, NULL, FALSE);
	::SetWindowPos(m_wEditor, 0, sourcerc.left, sourcerc.top, sourcerc.right - sourcerc.left, sourcerc.bottom - sourcerc.top, 0);
	::SetWindowPos(wBlame, 0, blamerc.left, blamerc.top, blamerc.right - blamerc.left, blamerc.bottom - blamerc.top, 0);
	if (m_colorage)
		::SetWindowPos(wLocator, 0, 0, blamerc.top, LOCATOR_WIDTH, blamerc.bottom - blamerc.top, SWP_SHOWWINDOW);
	else
		::ShowWindow(wLocator, SW_HIDE);
}

#if 0
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == uFindReplaceMsg)
	{
		LPFINDREPLACE lpfr = (LPFINDREPLACE)lParam;

		// If the FR_DIALOGTERM flag is set,
		// invalidate the handle identifying the dialog box.
		if (lpfr->Flags & FR_DIALOGTERM)
		{
			app.currentDialog = NULL;
			return 0;
		}
		if (lpfr->Flags & FR_FINDNEXT)
		{
			app.DoSearch(lpfr->lpstrFindWhat, lpfr->Flags);
		}
		return 0;
	}
	switch (message)
	{
	case WM_CREATE:
		app.m_wEditor = ::CreateWindow(
			"Scintilla",
			"Source",
			WS_CHILD | WS_VSCROLL | WS_HSCROLL | WS_CLIPCHILDREN,
			0, 0,
			100, 100,
			hWnd,
			0,
			app.hResource,
			0);
		app.InitialiseEditor();
		::ShowWindow(app.m_wEditor, SW_SHOW);
		::SetFocus(app.m_wEditor);
		app.wBlame = ::CreateWindow(
			_T("TortoiseGitBlameViewBlame"),
			_T("blame"),
			WS_CHILD | WS_CLIPCHILDREN,
			CW_USEDEFAULT, 0,
			CW_USEDEFAULT, 0,
			hWnd,
			NULL,
			app.hResource,
			NULL);
		::ShowWindow(app.wBlame, SW_SHOW);
		app.wHeader = ::CreateWindow(
			_T("TortoiseGitBlameViewHeader"),
			_T("header"),
			WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
			CW_USEDEFAULT, 0,
			CW_USEDEFAULT, 0,
			hWnd,
			NULL,
			app.hResource,
			NULL);
		::ShowWindow(app.wHeader, SW_SHOW);
		app.wLocator = ::CreateWindow(
			_T("TortoiseGitBlameViewLocator"),
			_T("locator"),
			WS_CHILD | WS_CLIPCHILDREN | WS_BORDER,
			CW_USEDEFAULT, 0,
			CW_USEDEFAULT, 0,
			hWnd,
			NULL,
			app.hResource,
			NULL);
		::ShowWindow(app.wLocator, SW_SHOW);
		return 0;

	case WM_SIZE:
		if (wParam != 1)
		{
			app.InitSize();
		}
		return 0;

	case WM_COMMAND:
		app.Command(LOWORD(wParam));
		break;
	case WM_NOTIFY:
		app.Notify(reinterpret_cast<SCNotification *>(lParam));
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		{
			CRegStdDWORD pos(_T("Software\\TortoiseGit\\TBlamePos"), 0);
			CRegStdDWORD width(_T("Software\\TortoiseGit\\TBlameSize"), 0);
			CRegStdDWORD state(_T("Software\\TortoiseGit\\TBlameState"), 0);
			RECT rc;
			GetWindowRect(app.wMain, &rc);
			if ((rc.left >= 0)&&(rc.top >= 0))
			{
				pos = MAKELONG(rc.left, rc.top);
				width = MAKELONG(rc.right-rc.left, rc.bottom-rc.top);
			}
			WINDOWPLACEMENT wp = {0};
			wp.length = sizeof(WINDOWPLACEMENT);
			GetWindowPlacement(app.wMain, &wp);
			state = wp.showCmd;
			::DestroyWindow(app.m_wEditor);
			::PostQuitMessage(0);
		}
		return 0;
	case WM_SETFOCUS:
		::SetFocus(app.wBlame);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndBlameProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	TRACKMOUSEEVENT mevt;
	HDC hDC;
	switch (message)
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(app.wBlame, &ps);
		app.DrawBlame(hDC);
		EndPaint(app.wBlame, &ps);
		break;
	case WM_COMMAND:
		app.Command(LOWORD(wParam));
		break;
	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->code)
		{
		case TTN_GETDISPINFO:
			{
				LPNMHDR pNMHDR = (LPNMHDR)lParam;
				NMTTDISPINFOA* pTTTA = (NMTTDISPINFOA*)pNMHDR;
				NMTTDISPINFOW* pTTTW = (NMTTDISPINFOW*)pNMHDR;
				POINT point;
				DWORD ptW = GetMessagePos();
				point.x = GET_X_LPARAM(ptW);
				point.y = GET_Y_LPARAM(ptW);
				::ScreenToClient(app.wBlame, &point);
				LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
				LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
				line = line + (point.y/height);
				if (line >= (LONG)app.revs.size())
					break;
				if (line < 0)
					break;
				LONG rev = app.revs[line];
				if (line >= (LONG)app.revs.size())
					break;

				SecureZeroMemory(app.m_szTip, sizeof(app.m_szTip));
				SecureZeroMemory(app.m_wszTip, sizeof(app.m_wszTip));
				std::map<LONG, CString>::iterator iter;
				if ((iter = app.logmessages.find(rev)) != app.logmessages.end())
				{
					CString msg;
					if (!ShowAuthor)
					{
						msg += app.authors[line];
					}
					if (!ShowDate)
					{
						if (!ShowAuthor) msg += "  ";
						msg += app.dates[line];
					}
					if (!ShowAuthor || !ShowDate)
						msg += '\n';
					msg += iter->second;
					// an empty tooltip string will deactivate the tooltips,
					// which means we must make sure that the tooltip won't
					// be empty.
					if (msg.empty())
						msg = _T(" ");
					if (pNMHDR->code == TTN_NEEDTEXTA)
					{
						lstrcpyn(app.m_szTip, msg.c_str(), MAX_LOG_LENGTH*2);
						app.StringExpand(app.m_szTip);
						pTTTA->lpszText = app.m_szTip;
					}
					else
					{
						pTTTW->lpszText = app.m_wszTip;
						::MultiByteToWideChar( CP_ACP , 0, msg.c_str(), min(msg.size(), MAX_LOG_LENGTH*2), app.m_wszTip, MAX_LOG_LENGTH*2);
						app.StringExpand(app.m_wszTip);
					}
				}
			}
			break;
		}
		return 0;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		return 0;
	case WM_MOUSELEAVE:
		app.m_mouserev = -2;
		app.m_mouseauthor.clear();
		app.ttVisible = FALSE;
		SendMessage(app.hwndTT, TTM_TRACKACTIVATE, FALSE, 0);
		::InvalidateRect(app.wBlame, NULL, FALSE);
		break;
	case WM_MOUSEMOVE:
		{
			mevt.cbSize = sizeof(TRACKMOUSEEVENT);
			mevt.dwFlags = TME_LEAVE;
			mevt.dwHoverTime = HOVER_DEFAULT;
			mevt.hwndTrack = app.wBlame;
			::TrackMouseEvent(&mevt);
			POINT pt = {((int)(short)LOWORD(lParam)), ((int)(short)HIWORD(lParam))};
			ClientToScreen(app.wBlame, &pt);
			pt.x += 15;
			pt.y += 15;
			SendMessage(app.hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(pt.x, pt.y));
			if (!app.ttVisible)
			{
				TOOLINFO ti;
				ti.cbSize = sizeof(TOOLINFO);
				ti.hwnd = app.wBlame;
				ti.uId = 0;
				SendMessage(app.hwndTT, TTM_TRACKACTIVATE, TRUE, (LPARAM)&ti);
			}
			int y = ((int)(short)HIWORD(lParam));
			LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
			LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
			line = line + (y/height);
			app.ttVisible = (line < (LONG)app.revs.size());
			if ( app.ttVisible )
			{
				if (app.authors[line].compare(app.m_mouseauthor) != 0)
				{
					app.m_mouseauthor = app.authors[line];
				}
				if (app.revs[line] != app.m_mouserev)
				{
					app.m_mouserev = app.revs[line];
					::InvalidateRect(app.wBlame, NULL, FALSE);
					SendMessage(app.hwndTT, TTM_UPDATE, 0, 0);
				}
			}
		}
		break;
	case WM_RBUTTONDOWN:
		// fall through
	case WM_LBUTTONDOWN:
		{
		break;
	case WM_SETFOCUS:
		::SetFocus(app.wBlame);
		app.SendEditor(SCI_GRABFOCUS);
		break;
	case WM_CONTEXTMENU:
		{
			if (app.m_selectedrev <= 0)
				break;;
			int xPos = GET_X_LPARAM(lParam);
			int yPos = GET_Y_LPARAM(lParam);
			if ((xPos < 0)||(yPos < 0))
			{
				// requested from keyboard, not mouse pointer
				// use the center of the window
				RECT rect;
				GetClientRect(app.wBlame, &rect);
				xPos = rect.right-rect.left;
				yPos = rect.bottom-rect.top;
			}
			HMENU hMenu = LoadMenu(app.hResource, MAKEINTRESOURCE(IDR_BLAMEPOPUP));
			HMENU hPopMenu = GetSubMenu(hMenu, 0);

			if ( _tcslen(szOrigPath)==0 )
			{
				// Without knowing the original path we cannot blame the previous revision
				// because we don't know which filename to pass to tortoiseproc.
				EnableMenuItem(hPopMenu,ID_BLAME_PREVIOUS_REVISION, MF_DISABLED|MF_GRAYED);
			}

			TrackPopupMenu(hPopMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, xPos, yPos, 0, app.wBlame, NULL);
			DestroyMenu(hMenu);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndHeaderProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	switch (message)
	{
	case WM_CREATE:
		return 0;
	case WM_PAINT:
		hDC = BeginPaint(app.wHeader, &ps);
		app.DrawHeader(hDC);
		EndPaint(app.wHeader, &ps);
		break;
	case WM_COMMAND:
		break;
	case WM_DESTROY:
		break;
	case WM_CLOSE:
		return 0;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK WndLocatorProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC;
	switch (message)
	{
	case WM_PAINT:
		hDC = BeginPaint(app.wLocator, &ps);
		app.DrawLocatorBar(hDC);
		EndPaint(app.wLocator, &ps);
		break;
	case WM_LBUTTONDOWN:
	case WM_MOUSEMOVE:
		if (wParam & MK_LBUTTON)
		{
			RECT rect;
			::GetClientRect(hWnd, &rect);
			int nLine = HIWORD(lParam)*app.revs.size()/(rect.bottom-rect.top);

			if (nLine < 0)
				nLine = 0;
			app.ScrollToLine(nLine);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
#endif

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
				encoding = GetEncode( &data[start+2], data.size() - start -2, &bomoffset);
			}
			{
				if(encoding == 1200)
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
	m_highestrev=this->GetLogData()->size()+m_NoListCommit.size();

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

	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR height = SendEditor(SCI_TEXTHEIGHT);
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
						m_ID[line] = this->GetLogData()->size()-i;
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
			m_MouseLine = line;//m_CommitHash[line];
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
			str.Format(_T("%s: %s\n%s: %s\n%s: %s\n%s:\n%s\n%s"),	m_sRev, pRev->m_CommitHash.ToString(),
																	m_sAuthor, pRev->GetAuthorName(),
																	m_sDate, CLoglistUtils::FormatDateAndTime(pRev->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes),
																	m_sMessage, pRev->GetSubject(),
																	pRev->GetBody());

			m_ToolTip.Pop();
			m_ToolTip.AddTool(this, str);

			CRect rect;
			rect.left=LOCATOR_WIDTH;
			rect.right=this->m_blamewidth+rect.left;
			rect.top=point.y-height;
			rect.bottom=point.y+height;
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

int CTortoiseGitBlameView::FindNextLine(CGitHash CommitHash,bool bUpOrDown)
{
	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR startline =line;
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
