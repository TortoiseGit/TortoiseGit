// TortoiseGitBlame - a Viewer for Git Blames

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2014 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "MenuEncode.h"
#include "gitdll.h"
#include "StringUtils.h"
#include "BlameIndexColors.h"
#include "BlameDetectMovedOrCopiedLines.h"
#include "TGitPath.h"
#include "IconMenu.h"
#include "DPIAware.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

UINT CTortoiseGitBlameView::m_FindDialogMessage;

// CTortoiseGitBlameView
IMPLEMENT_DYNAMIC(CSciEditBlame,CSciEdit)

IMPLEMENT_DYNCREATE(CTortoiseGitBlameView, CView)

BEGIN_MESSAGE_MAP(CTortoiseGitBlameView, CView)
	// Standard printing commands
	ON_COMMAND(ID_EDIT_FIND,OnEditFind)
	ON_COMMAND(ID_EDIT_GOTO,OnEditGoto)
	ON_COMMAND(ID_EDIT_COPY, CopyToClipboard)
	ON_UPDATE_COMMAND_UI(ID_EDIT_COPY, OnUpdateViewCopyToClipboard)
	ON_COMMAND(ID_VIEW_NEXT,OnViewNext)
	ON_COMMAND(ID_VIEW_PREV,OnViewPrev)
	ON_COMMAND(ID_FIND_NEXT, OnFindNext)
	ON_COMMAND(ID_FIND_PREV, OnFindPrev)
	ON_COMMAND(ID_VIEW_SHOWAUTHOR, OnViewToggleAuthor)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWAUTHOR, OnUpdateViewToggleAuthor)
	ON_COMMAND(ID_VIEW_SHOWDATE, OnViewToggleDate)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWDATE, OnUpdateViewToggleDate)
	ON_COMMAND(ID_VIEW_SHOWFILENAME, OnViewToggleShowFilename)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWFILENAME, OnUpdateViewToggleShowFilename)
	ON_COMMAND(ID_VIEW_SHOWORIGINALLINENUMBER, OnViewToggleShowOriginalLineNumber)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWORIGINALLINENUMBER, OnUpdateViewToggleShowOriginalLineNumber)
	ON_COMMAND(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_DISABLED, OnViewDetectMovedOrCopiedLinesToggleDisabled)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_DISABLED, OnUpdateViewDetectMovedOrCopiedLinesToggleDisabled)
	ON_COMMAND(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE, OnViewDetectMovedOrCopiedLinesToggleWithinFile)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE, OnUpdateViewDetectMovedOrCopiedLinesToggleWithinFile)
	ON_COMMAND(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES, OnViewDetectMovedOrCopiedLinesToggleFromModifiedFiles)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES, OnUpdateViewDetectMovedOrCopiedLinesToggleFromModifiedFiles)
	ON_COMMAND(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION, OnViewDetectMovedOrCopiedLinesToggleFromExistingFilesAtFileCreation)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION, OnUpdateViewDetectMovedOrCopiedLinesToggleFromExistingFilesAtFileCreation)
	ON_COMMAND(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES, OnViewDetectMovedOrCopiedLinesToggleFromExistingFiles)
	ON_UPDATE_COMMAND_UI(ID_VIEW_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES, OnUpdateViewDetectMovedOrCopiedLinesToggleFromExistingFiles)
	ON_COMMAND(ID_VIEW_IGNORE_WHITESPACE, OnViewToggleIgnoreWhitespace)
	ON_UPDATE_COMMAND_UI(ID_VIEW_IGNORE_WHITESPACE, OnUpdateViewToggleIgnoreWhitespace)
	ON_COMMAND(ID_VIEW_SHOWCOMPLETELOG, OnViewToggleShowCompleteLog)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SHOWCOMPLETELOG, OnUpdateViewToggleShowCompleteLog)
	ON_COMMAND(ID_VIEW_ONLYCONSIDERFIRSTPARENTS, OnViewToggleOnlyFirstParent)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ONLYCONSIDERFIRSTPARENTS, OnUpdateViewToggleOnlyFirstParent)
	ON_COMMAND(ID_VIEW_FOLLOWRENAMES, OnViewToggleFollowRenames)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FOLLOWRENAMES, OnUpdateViewToggleFollowRenames)
	ON_COMMAND(ID_VIEW_COLORBYAGE, OnViewToggleColorByAge)
	ON_UPDATE_COMMAND_UI(ID_VIEW_COLORBYAGE, OnUpdateViewToggleColorByAge)
	ON_COMMAND(ID_VIEW_ENABLELEXER, OnViewToggleLexer)
	ON_UPDATE_COMMAND_UI(ID_VIEW_ENABLELEXER, OnUpdateViewToggleLexer)
	ON_COMMAND(ID_VIEW_WRAPLONGLINES, OnViewWrapLongLines)
	ON_UPDATE_COMMAND_UI(ID_VIEW_WRAPLONGLINES, OnUpdateViewWrapLongLines)
	ON_COMMAND_RANGE(IDM_FORMAT_ENCODE, IDM_FORMAT_ENCODE_END, OnChangeEncode)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEHOVER()
	ON_WM_MOUSELEAVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_ERASEBKGND()
	ON_NOTIFY(SCN_PAINTED, IDC_SCINTILLA, OnSciPainted)
	ON_NOTIFY(SCN_GETBKCOLOR, IDC_SCINTILLA, OnSciGetBkColor)
	ON_REGISTERED_MESSAGE(m_FindDialogMessage, OnFindDialogMessage)
END_MESSAGE_MAP()


// CTortoiseGitBlameView construction/destruction

CTortoiseGitBlameView::CTortoiseGitBlameView()
	: wBlame(0)
	, wHeader(0)
	, hwndTT(0)
	, bIgnoreEOL(false)
	, bIgnoreSpaces(false)
	, bIgnoreAllSpaces(false)
	, m_MouseLine(-1)
	, m_bMatchCase(false)
	, hInstance(nullptr)
	, hResource(nullptr)
	, currentDialog(nullptr)
	, wMain(nullptr)
	, wLocator(nullptr)
	, m_blamewidth(0)
	, m_revwidth(0)
	, m_datewidth(0)
	, m_authorwidth(0)
	, m_filenameWidth(0)
	, m_originalLineNumberWidth(0)
	, m_linewidth(0)
	, m_SelectedLine(-1)
	, m_bShowLine(true)
	, m_pFindDialog(nullptr)
#ifdef USE_TEMPFILENAME
	, m_Buffer(nullptr)
#endif
{
	m_windowcolor = ::GetSysColor(COLOR_WINDOW);
	m_textcolor = ::GetSysColor(COLOR_WINDOWTEXT);
	m_texthighlightcolor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_mouserevcolor = InterColor(m_windowcolor, m_textcolor, 20);
	m_mouseauthorcolor = InterColor(m_windowcolor, m_textcolor, 10);
	m_selectedrevcolor = ::GetSysColor(COLOR_HIGHLIGHT);
	m_selectedauthorcolor = InterColor(m_selectedrevcolor, m_texthighlightcolor, 35);

	HIGHCONTRAST highContrast = { 0 };
	highContrast.cbSize = sizeof(HIGHCONTRAST);
	BOOL highContrastModeEnabled = SystemParametersInfo(SPI_GETHIGHCONTRAST, 0, &highContrast, 0) == TRUE && (highContrast.dwFlags & HCF_HIGHCONTRASTON);
	m_colorage = !!theApp.GetInt(L"ColorAge", !highContrastModeEnabled);
	m_bLexer = !!theApp.GetInt(L"EnableLexer", !highContrastModeEnabled);

	m_bShowAuthor = (theApp.GetInt(L"ShowAuthor", 1) == 1);
	m_bShowDate = (theApp.GetInt(L"ShowDate", 0) == 1);
	m_bShowFilename = (theApp.GetInt(L"ShowFilename", 0) == 1);
	m_bShowOriginalLineNumber = (theApp.GetInt(L"ShowOriginalLineNumber", 0) == 1);
	m_dwDetectMovedOrCopiedLines = theApp.GetInt(L"DetectMovedOrCopiedLines", 0);
	m_bIgnoreWhitespace = (theApp.GetInt(L"IgnoreWhitespace", 0) == 1);
	m_bShowCompleteLog = (theApp.GetInt(L"ShowCompleteLog", 1) == 1);
	m_bOnlyFirstParent = (theApp.GetInt(L"OnlyFirstParent", 0) == 1);
	m_bFollowRenames = (theApp.GetInt(L"FollowRenames", 0) == 1);
	m_bBlameOutputContainsOtherFilenames = FALSE;
	m_bWrapLongLines = !!theApp.GetInt(L"WrapLongLines", 0);
	m_sFindText = theApp.GetString(L"FindString");

	m_FindDialogMessage = ::RegisterWindowMessage(FINDMSGSTRING);
	// get short/long datetime setting from registry
	DWORD RegUseShortDateFormat = CRegDWORD(L"Software\\TortoiseGit\\LogDateFormat", TRUE);
	if ( RegUseShortDateFormat )
	{
		m_DateFormat = DATE_SHORTDATE;
	}
	else
	{
		m_DateFormat = DATE_LONGDATE;
	}
	// get relative time display setting from registry
	DWORD regRelativeTimes = CRegDWORD(L"Software\\TortoiseGit\\RelativeTimes", FALSE);
	m_bRelativeTimes = (regRelativeTimes != 0);

	m_sRev.LoadString(IDS_LOG_REVISION);
	m_sFileName.LoadString(IDS_FILENAME);
	m_sAuthor.LoadString(IDS_LOG_AUTHOR);
	m_sDate.LoadString(IDS_LOG_DATE);
	m_sMessage.LoadString(IDS_LOG_MESSAGE);
}

CTortoiseGitBlameView::~CTortoiseGitBlameView()
{
#ifdef USE_TEMPFILENAME
	delete m_Buffer;
	m_Buffer = nullptr;
#endif
}
struct EncodingUnit
{
	int id;
	char *name;
};

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
		{20866,	"koi8_r	csKOI8R"},																	//IDM_FORMAT_KOI8R_CYRILLIC
		{65001,	"UTF-8"},																			//IDM_FORMAT_UTF8
		{1200,	"UTF-16 LE"},																		//IDM_FORMAT_UTF16LE
		{1201,	"UTF-16 BE"},																		//IDM_FORMAT_UTF16BE
};
void CTortoiseGitBlameView::OnChangeEncode(UINT nId)
{
	if(nId >= IDM_FORMAT_ENCODE && nId <= IDM_FORMAT_ENCODE_END)
		this->UpdateInfo(encodings[nId - IDM_FORMAT_ENCODE].id);
}
int CTortoiseGitBlameView::OnCreate(LPCREATESTRUCT lpcs)
{
	CRect rect,rect1;
	this->GetWindowRect(&rect1);
	rect.left = m_blamewidth + CDPIAware::Instance().ScaleX(LOCATOR_WIDTH);
	rect.right=rect.Width();
	rect.top=0;
	rect.bottom=rect.Height();
	if (!m_TextView.Create(L"Scintilla", L"source", 0, rect, this, IDC_SCINTILLA, 0))
	{
		TRACE0("Failed to create view\n");
		return -1; // fail to create
	}
	m_TextView.Init(-1);
	m_TextView.ShowWindow( SW_SHOW);
	CreateFont();
	SendEditor(SCI_SETREADONLY, TRUE);
	m_ToolTip.Create(this->GetParent());

	::AfxGetApp()->GetMainWnd();
	return CView::OnCreate(lpcs);
}

void CTortoiseGitBlameView::OnSize(UINT /*nType*/, int cx, int cy)
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

BOOL CTortoiseGitBlameView::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CTortoiseGitBlameView::OnDraw(CDC* pDC)
{
	CTortoiseGitBlameDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CMemDC myDC(*pDC, this);
	RECT rc;
	GetClientRect(&rc);
	myDC.GetDC().FillSolidRect(&rc, m_windowcolor);
	DrawBlame(myDC.GetDC());
	DrawLocatorBar(myDC.GetDC());
}

int CTortoiseGitBlameView::GetLineUnderCursor(CPoint point)
{
	auto firstvisibleline = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));
	auto line = static_cast<int>(SendEditor(SCI_DOCLINEFROMVISIBLE, firstvisibleline));
	auto linesonscreen = static_cast<int>(SendEditor(SCI_LINESONSCREEN)) + 1;
	auto height = static_cast<int>(SendEditor(SCI_TEXTHEIGHT));

	int i = 0, y = 0;
	for (i = line; y <= point.y && i < (line + linesonscreen); ++i)
	{
		auto wrapcount = static_cast<int>(SendEditor(SCI_WRAPCOUNT, i));
		if (wrapcount > 1)
		{
			if (i == line)
				wrapcount -= static_cast<int>(SendEditor(SCI_DOCLINEFROMVISIBLE, firstvisibleline + wrapcount - 1)) - static_cast<int>(SendEditor(SCI_DOCLINEFROMVISIBLE, firstvisibleline));
			linesonscreen -= wrapcount - 1;
		}
		y += height * wrapcount;
	}
	return i - 1;
}

void CTortoiseGitBlameView::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
	int line = GetLineUnderCursor(point);
	if (m_data.IsValidLine(line))
	{
		m_MouseLine = line;
		ClientToScreen(&point);

		CGitHash hash = m_data.GetHash(line);
		CString hashStr = hash.ToString();

		GitRevLoglist* pRev = nullptr;
		int logIndex = m_lineToLogIndex[line];
		if (logIndex >= 0)
			pRev = &GetLogData()->GetGitRevAt(logIndex);
		else
		{
			pRev = m_data.GetRev(line, GetLogData()->m_pLogCache->m_HashMap);
			if (pRev && pRev->m_ParentHash.empty())
			{
				if (pRev->GetParentFromHash(pRev->m_CommitHash))
					MessageBox(pRev->GetLastErr(), L"TortoiseGit", MB_ICONERROR);
			}
		}

		if (!pRev)
			return;

		CIconMenu popup;
		CIconMenu blamemenu, diffmenu;

		if (!popup.CreatePopupMenu())
			return;

		// Now find the relevant parent commits, they must contain the file which is blamed to be the source of the selected line,
		// otherwise there is no previous file to compare to (only another previous revision).

		GIT_REV_LIST parentHashWithFile;
		std::vector<CString> parentFilename;
		try
		{
			CTGitPath path(m_data.GetFilename(line));
			const CTGitPathList& files = pRev->GetFiles(nullptr);
			for (int j = 0, j_size = files.GetCount(); j < j_size; ++j)
			{
				const CTGitPath &file =  files[j];
				if (file.IsEquivalentTo(path))
				{
					if (!(file.m_ParentNo & MERGE_MASK))
					{
						int action = file.m_Action;
						// ignore (action & CTGitPath::LOGACTIONS_ADDED), as then there is nothing to blame/diff
						// ignore (action & CTGitPath::LOGACTIONS_DELETED), should never happen as the file must exist
						if (action & (CTGitPath::LOGACTIONS_MODIFIED | CTGitPath::LOGACTIONS_REPLACED))
						{
							int parentNo = file.m_ParentNo & PARENT_MASK;
							if (parentNo >= 0 && static_cast<size_t>(parentNo) < pRev->m_ParentHash.size())
							{
								parentHashWithFile.push_back(pRev->m_ParentHash[parentNo]);
								parentFilename.push_back((action & CTGitPath::LOGACTIONS_REPLACED) ? file.GetGitOldPathString() : file.GetGitPathString());
							}
						}
					}
				}
			}
		}
		catch (const char* msg)
		{
			MessageBox(L"Could not get files of parents.\nlibgit reports:\n" + CString(msg), L"TortoiseGit", MB_ICONERROR);
		}

		// blame previous
		if (!parentHashWithFile.empty())
		{
			if (parentHashWithFile.size() == 1)
			{
				popup.AppendMenuIcon(ID_BLAMEPREVIOUS, IDS_BLAME_POPUP_BLAME, IDI_BLAME_POPUP_BLAME);
			}
			else
			{
				blamemenu.CreatePopupMenu();
				popup.AppendMenuIcon(ID_BLAMEPREVIOUS, IDS_BLAME_POPUP_BLAME, IDI_BLAME_POPUP_BLAME, blamemenu.m_hMenu);

				for (size_t i = 0; i < parentHashWithFile.size(); ++i)
				{
					CString str;
					str.Format(IDS_PARENT, i + 1);
					blamemenu.AppendMenuIcon(ID_BLAMEPREVIOUS + ((i + 1) << 16), str);
				}
			}
		}

		// compare with previous
		if (!parentHashWithFile.empty())
		{
			if (parentHashWithFile.size() == 1)
			{
				popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_BLAME_POPUP_COMPARE, IDI_BLAME_POPUP_COMPARE);
				if (CRegDWORD(L"Software\\TortoiseGit\\DiffByDoubleClickInLog", FALSE))
					popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
			}
			else
			{
				diffmenu.CreatePopupMenu();
				popup.AppendMenuIcon(ID_COMPAREWITHPREVIOUS, IDS_BLAME_POPUP_COMPARE, IDI_BLAME_POPUP_COMPARE, diffmenu.m_hMenu);
				for (size_t i = 0; i < parentHashWithFile.size(); ++i)
				{
					CString str;
					str.Format(IDS_BLAME_POPUP_PARENT, i + 1);
					diffmenu.AppendMenuIcon(static_cast<UINT>(ID_COMPAREWITHPREVIOUS + ((i + 1) << 16)),str);
					if (i == 0 && CRegDWORD(L"Software\\TortoiseGit\\DiffByDoubleClickInLog", FALSE))
					{
						popup.SetDefaultItem(ID_COMPAREWITHPREVIOUS, FALSE);
						diffmenu.SetDefaultItem(static_cast<UINT>(ID_COMPAREWITHPREVIOUS + ((i + 1) << 16)), FALSE);
					}
				}
			}
		}

		popup.AppendMenuIcon(ID_SHOWLOG, IDS_BLAME_POPUP_LOG, IDI_BLAME_POPUP_LOG);
		popup.AppendMenu(MF_SEPARATOR, NULL);
		popup.AppendMenuIcon(ID_COPYHASHTOCLIPBOARD, IDS_BLAME_POPUP_COPYHASHTOCLIPBOARD, IDI_BLAME_POPUP_COPY);
		popup.AppendMenuIcon(ID_COPYLOGTOCLIPBOARD, IDS_BLAME_POPUP_COPYLOGTOCLIPBOARD, IDI_BLAME_POPUP_COPY);

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
		if (!cmd)
			return;
		this->ContextMenuAction(cmd, pRev, parentHashWithFile, parentFilename, line);
	}
}

void CTortoiseGitBlameView::ContextMenuAction(int cmd, GitRev *pRev, GIT_REV_LIST& parentHashWithFile, const std::vector<CString>& parentFilename, int selectedLine)
{
	switch (cmd & 0xFFFF)
	{
	case ID_BLAMEPREVIOUS:
		{
			int index = (cmd>>16) & 0xFFFF;
			if (index > 0)
				index -= 1;

			CString path = ResolveCommitFile(parentFilename[index]);
			CString endrev = parentHashWithFile[index].ToString();
			int line = m_data.GetOriginalLineNumber(selectedLine);

			CString procCmd = L"/path:\"" + path + L"\" ";
			procCmd += L" /command:blame";
			procCmd += L" /endrev:" + endrev;
			procCmd += L" /line:";
			procCmd.AppendFormat(L"%d", line);

			CCommonAppUtils::RunTortoiseGitProc(procCmd);
		}
		break;

	case ID_COMPAREWITHPREVIOUS:
		{
			int index = (cmd >> 16) & 0xFFFF;
			if (index > 0)
				index -= 1;

			CString path = ResolveCommitFile(parentFilename[index]);
			CString startrev = parentHashWithFile[index].ToString();
			CString endrev = pRev->m_CommitHash.ToString();

			CString procCmd = L"/path:\"" + path + L"\" ";
			procCmd += L" /command:diff";
			procCmd += L" /startrev:" + startrev;
			procCmd += L" /endrev:" + endrev;
			if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
				procCmd += L" /alternative";

			CCommonAppUtils::RunTortoiseGitProc(procCmd);
		}
		break;

	case ID_SHOWLOG:
		{
			CString path = ResolveCommitFile(selectedLine);
			CString rev = m_data.GetHash(selectedLine).ToString();

			CString procCmd = L"/path:\"" + path + L"\" ";
			procCmd += L" /command:log";
			procCmd += L" /rev:" + rev;
			procCmd += L" /endrev:" + rev;

			CCommonAppUtils::RunTortoiseGitProc(procCmd);
		}
		break;

	case ID_COPYHASHTOCLIPBOARD:
		this->GetLogList()->CopySelectionToClipBoard(CGitLogListBase::ID_COPYCLIPBOARDHASH);
		break;

	case ID_COPYLOGTOCLIPBOARD:
		this->GetLogList()->CopySelectionToClipBoard(CGitLogListBase::ID_COPYCLIPBOARDFULL);
		break;
	}
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
	return static_cast<CTortoiseGitBlameDoc*>(m_pDocument);
}
#endif //_DEBUG


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
	return m_TextView.Call(Msg, wParam, lParam);
}

void CTortoiseGitBlameView::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face)
{
	if (fore == back && fore == m_windowcolor)
		fore = m_textcolor;
	m_TextView.SetAStyle(style, fore, back, size, face);
}

void CTortoiseGitBlameView::InitialiseEditor()
{
	SendEditor(SCI_STYLERESETDEFAULT);
	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	std::string fontName = CUnicodeUtils::StdGetUTF8(CRegStdString(L"Software\\TortoiseGit\\BlameFontName", L"Consolas"));
	SetAStyle(STYLE_DEFAULT,
			::GetSysColor(COLOR_WINDOWTEXT),
			::GetSysColor(COLOR_WINDOW),
			static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\BlameFontSize", 10)),
			fontName.c_str()
			);
	SendEditor(SCI_SETTABWIDTH, static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\BlameTabSize", 4)));
	SendEditor(SCI_SETREADONLY, TRUE);
	auto numberOfLines = m_data.GetNumberOfLines();
	int numDigits = 2;
	while (numberOfLines)
	{
		numberOfLines /= 10;
		++numDigits;
	}
	if (m_bShowLine)
		SendEditor(SCI_SETMARGINWIDTHN, 0, numDigits * static_cast<int>(SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, reinterpret_cast<LPARAM>("8"))));
	else
		SendEditor(SCI_SETMARGINWIDTHN, 0);
	SendEditor(SCI_SETMARGINWIDTHN, 1);
	SendEditor(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	SendEditor(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	SendEditor(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	SendEditor(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));
	m_regOldLinesColor = CRegStdDWORD(L"Software\\TortoiseGit\\BlameOldColor", BLAMEOLDCOLOR);
	m_regNewLinesColor = CRegStdDWORD(L"Software\\TortoiseGit\\BlameNewColor", BLAMENEWCOLOR);
	if (CRegStdDWORD(L"Software\\TortoiseGit\\ScintillaDirect2D", FALSE) != FALSE)
	{
		SendEditor(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITERETAIN);
		SendEditor(SCI_SETBUFFEREDDRAW, 0);
	}

	if (m_bWrapLongLines)
		SendEditor(SCI_SETWRAPMODE, SC_WRAP_WORD);
	else
		SendEditor(SCI_SETWRAPMODE, SC_WRAP_NONE);
	SendEditor(SCI_STYLECLEARALL);
}

bool CTortoiseGitBlameView::DoSearch(CTortoiseGitBlameData::SearchDirection direction)
{
	auto pos = static_cast<Sci_Position>(SendEditor(SCI_GETCURRENTPOS));
	auto line = static_cast<int>(SendEditor(SCI_LINEFROMPOSITION, pos));

	int i = m_data.FindFirstLineWrapAround(direction, m_sFindText, line, m_bMatchCase, [hWnd = m_pFindDialog->GetSafeHwnd()]{ FLASHWINFO flags = { sizeof(FLASHWINFO), hWnd, FLASHW_ALL, 2, 100 }; ::FlashWindowEx(&flags); });
	if (i >= 0)
	{
		GotoLine(i + 1);
		auto selstart = static_cast<int>(static_cast<Sci_Position>(SendEditor(SCI_GETCURRENTPOS)));
		auto selend = static_cast<int>(static_cast<Sci_Position>(SendEditor(SCI_POSITIONFROMLINE, i + 1)));
		SendEditor(SCI_SETSELECTIONSTART, selstart);
		SendEditor(SCI_SETSELECTIONEND, selend);
		m_SelectedLine = i;
	}
	else
		::MessageBox(m_pFindDialog && m_pFindDialog->GetSafeHwnd() ? m_pFindDialog->GetSafeHwnd() : wMain, L"\"" + m_sFindText + L"\" " + CString(MAKEINTRESOURCE(IDS_NOTFOUND)), L"TortoiseGitBlame", MB_ICONINFORMATION);

	return true;
}

void CTortoiseGitBlameView::OnFindPrev()
{
	if (m_sFindText.IsEmpty())
		return;
	DoSearch(CTortoiseGitBlameData::SearchPrevious);
}

void CTortoiseGitBlameView::OnFindNext()
{
	if (m_sFindText.IsEmpty())
		return;
	DoSearch(CTortoiseGitBlameData::SearchNext);
}

bool CTortoiseGitBlameView::GotoLine(int line)
{
	--line;
	int numberOfLines = static_cast<int>(m_data.GetNumberOfLines());
	if (line < 0 || numberOfLines == 0)
		return false;
	if (line >= numberOfLines)
	{
		line = numberOfLines - 1;
	}

	auto nCurrentPos = static_cast<Sci_Position>(SendEditor(SCI_GETCURRENTPOS));
	int nCurrentLine = static_cast<int>(SendEditor(SCI_LINEFROMPOSITION, nCurrentPos));
	int nFirstVisibleLine = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));
	int nLinesOnScreen = static_cast<int>(SendEditor(SCI_LINESONSCREEN));

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
			SendEditor(SCI_GOTOLINE, static_cast<WPARAM>(line + nLinesOnScreen * (2 / 3.0)));
		}
		else
		{
			SendEditor(SCI_GOTOLINE, static_cast<WPARAM>(line - nLinesOnScreen * (1 / 3.0)));
		}
	}

	// Highlight the line
	int nPosStart = static_cast<int>(SendEditor(SCI_POSITIONFROMLINE, line));
	int nPosEnd = static_cast<int>(SendEditor(SCI_GETLINEENDPOSITION, line));
	SendEditor(SCI_SETSEL,nPosEnd,nPosStart);

	return true;
}

bool CTortoiseGitBlameView::ScrollToLine(long line)
{
	if (line < 0)
		return false;

	int nCurrentLine = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));

	int scrolldelta = line - nCurrentLine;
	SendEditor(SCI_LINESCROLL, 0, scrolldelta);

	return true;
}

void CTortoiseGitBlameView::CopyToClipboard()
{
	CWnd * wnd = GetFocus();
	if (wnd == this->GetLogList())
		GetLogList()->CopySelectionToClipBoard();
	else if (wnd)
	{
		if (CString(wnd->GetRuntimeClass()->m_lpszClassName) == L"CMFCPropertyGridCtrl")
		{
			auto grid = static_cast<CMFCPropertyGridCtrl*>(wnd);
			if (grid->GetCurSel() && !grid->GetCurSel()->IsGroup())
				CStringUtils::WriteAsciiStringToClipboard(grid->GetCurSel()->GetValue(), GetSafeHwnd());
		}
		else
			m_TextView.Call(SCI_COPY);
	}
}

LONG CTortoiseGitBlameView::GetBlameWidth()
{
	LONG blamewidth = 0;
	SIZE width;
	CreateFont();
	HDC hDC = this->GetDC()->m_hDC;
	HFONT oldfont = static_cast<HFONT>(::SelectObject(hDC, m_font.GetSafeHandle()));

	CString shortHash('f', g_Git.GetShortHASHLength() + 1);
	::GetTextExtentPoint32(hDC, shortHash, g_Git.GetShortHASHLength() + 1, &width);
	m_revwidth = width.cx + CDPIAware::Instance().ScaleX(BLAMESPACE);
	blamewidth += m_revwidth;

	if (m_bShowDate)
	{
		SIZE maxwidth = {0};

		auto numberOfLines = m_data.GetNumberOfLines();
		for (size_t i = 0; i < numberOfLines; ++i)
		{
			::GetTextExtentPoint32(hDC, m_data.GetDate(i), m_data.GetDate(i).GetLength(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_datewidth = maxwidth.cx + CDPIAware::Instance().ScaleX(BLAMESPACE);
		blamewidth += m_datewidth;
	}
	if ( m_bShowAuthor)
	{
		SIZE maxwidth = {0};

		size_t numberOfLines = m_data.GetNumberOfLines();
		for (size_t i = 0; i < numberOfLines; ++i)
		{
			::GetTextExtentPoint32(hDC,m_data.GetAuthor(i) , m_data.GetAuthor(i).GetLength(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_authorwidth = maxwidth.cx + CDPIAware::Instance().ScaleX(BLAMESPACE);
		blamewidth += m_authorwidth;
	}
	if (m_bShowFilename)
	{
		SIZE maxwidth = {0};

		size_t numberOfLines = m_data.GetNumberOfLines();
		for (size_t i = 0; i < numberOfLines; ++i)
		{
			::GetTextExtentPoint32(hDC, m_data.GetFilename(i), m_data.GetFilename(i).GetLength(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_filenameWidth = maxwidth.cx + CDPIAware::Instance().ScaleX(BLAMESPACE);
		blamewidth += m_filenameWidth;
	}
	if (m_bShowOriginalLineNumber)
	{
		SIZE maxwidth = {0};

		size_t numberOfLines = m_data.GetNumberOfLines();
		CString str;
		for (size_t i = 0; i < numberOfLines; ++i)
		{
			str.Format(L"%5d", m_data.GetOriginalLineNumber(i));
			::GetTextExtentPoint32(hDC, str, str.GetLength(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_originalLineNumberWidth = maxwidth.cx + CDPIAware::Instance().ScaleX(BLAMESPACE);
		blamewidth += m_originalLineNumberWidth;
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
	if (m_font.GetSafeHandle())
		return;
	LOGFONT lf = {0};
	lf.lfWeight = 400;
	lf.lfHeight = -CDPIAware::Instance().PointsToPixelsY(static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\BlameFontSize", 10)));
	lf.lfCharSet = DEFAULT_CHARSET;
	CRegStdString fontname = CRegStdString(L"Software\\TortoiseGit\\BlameFontName", L"Consolas");
	wcsncpy_s(lf.lfFaceName, static_cast<std::wstring>(fontname).c_str(), _TRUNCATE);
	m_font.CreateFontIndirect(&lf);

	lf.lfItalic = TRUE;
	m_italicfont.CreateFontIndirect(&lf);
}

void CTortoiseGitBlameView::DrawBlame(HDC hDC)
{
	if (!hDC || m_data.GetNumberOfLines() == 0)
		return;
	if (!m_font.GetSafeHandle())
		return;

	HFONT oldfont = nullptr;
	int firstvisibleline = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));
	int line = static_cast<int>(SendEditor(SCI_DOCLINEFROMVISIBLE, firstvisibleline));
	int linesonscreen = static_cast<int>(SendEditor(SCI_LINESONSCREEN)) + 1;
	int height = static_cast<int>(SendEditor(SCI_TEXTHEIGHT));
	int Y = 0;
	TCHAR buf[MAX_PATH] = {0};
	std::fill_n(buf, _countof(buf) - 1, L' ');
	RECT rc;
	CGitHash oldHash;
	CString oldFile;

	for (int i = line; i < (line + linesonscreen) && static_cast<size_t>(i) < m_data.GetNumberOfLines(); ++i)
	{
		auto wrapcount = static_cast<int>(SendEditor(SCI_WRAPCOUNT, i));
		if (wrapcount > 1)
		{
			if (i == line)
				wrapcount -= static_cast<int>(SendEditor(SCI_DOCLINEFROMVISIBLE, firstvisibleline + wrapcount - 1)) - static_cast<int>(SendEditor(SCI_DOCLINEFROMVISIBLE, firstvisibleline));
			linesonscreen -= wrapcount - 1;
		}
		CGitHash hash(m_data.GetHash(i));
		oldfont = static_cast<HFONT>(::SelectObject(hDC, m_font.GetSafeHandle()));
		::SetBkColor(hDC, m_windowcolor);
		::SetTextColor(hDC, m_textcolor);
		if (!hash.IsEmpty() && hash == m_SelectedHash)
		{
			::SetBkColor(hDC, m_selectedauthorcolor);
			::SetTextColor(hDC, m_texthighlightcolor);
		}

		if (m_MouseLine == i)
			::SetBkColor(hDC, m_mouserevcolor);

		if ((!hash.IsEmpty() && hash == m_SelectedHash) || m_MouseLine == i)
		{
			auto old = ::GetTextColor(hDC);
			::SetTextColor(hDC, ::GetBkColor(hDC));
			RECT rc2 = { CDPIAware::Instance().ScaleX(LOCATOR_WIDTH), Y, m_blamewidth + CDPIAware::Instance().ScaleX(LOCATOR_WIDTH), Y + (wrapcount * height) };
			for (int j = 0; j < wrapcount; ++j)
				::ExtTextOut(hDC, 0, Y + (j * height), ETO_CLIPPED, &rc2, buf, _countof(buf) - 1, 0);
			::SetTextColor(hDC, old);
		}

		CString file = m_data.GetFilename(i);
		if (oldHash != hash || (m_bShowFilename && oldFile != file) || m_bShowOriginalLineNumber)
		{
			rc.top = static_cast<LONG>(Y);
			rc.left = CDPIAware::Instance().ScaleX(LOCATOR_WIDTH);
			rc.bottom = static_cast<LONG>(Y + height);
			rc.right = rc.left + m_blamewidth;
			if (oldHash != hash)
			{
				CString shortHashStr = hash.ToString(g_Git.GetShortHASHLength());
				::ExtTextOut(hDC, CDPIAware::Instance().ScaleX(LOCATOR_WIDTH), Y, ETO_CLIPPED, &rc, shortHashStr, shortHashStr.GetLength(), 0);
			}
			int Left = m_revwidth;

			if (m_bShowAuthor)
			{
				rc.right = rc.left + Left + m_authorwidth;
				if (oldHash != hash)
					::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, m_data.GetAuthor(i), m_data.GetAuthor(i).GetLength(), 0);
				Left += m_authorwidth;
			}
			if (m_bShowDate)
			{
				rc.right = rc.left + Left + m_datewidth;
				if (oldHash != hash)
					::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, m_data.GetDate(i), m_data.GetDate(i).GetLength(), 0);
				Left += m_datewidth;
			}
			if (m_bShowFilename)
			{
				rc.right = rc.left + Left + m_filenameWidth;
				if (oldFile != file)
					::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, m_data.GetFilename(i), m_data.GetFilename(i).GetLength(), 0);
				Left += m_filenameWidth;
			}
			if (m_bShowOriginalLineNumber)
			{
				rc.right = rc.left + Left + m_originalLineNumberWidth;
				CString str;
				str.Format(L"%5d", m_data.GetOriginalLineNumber(i));
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, str, str.GetLength(), 0);
				Left += m_originalLineNumberWidth;
			}
			oldHash = hash;
			oldFile = file;
		}
		if (i == m_SelectedLine && m_pFindDialog)
		{
			LOGBRUSH brush;
			brush.lbColor = m_textcolor;
			brush.lbHatch = 0;
			brush.lbStyle = BS_SOLID;
			HPEN pen = ExtCreatePen(PS_SOLID | PS_GEOMETRIC, 2, &brush, 0, nullptr);
			HGDIOBJ hPenOld = SelectObject(hDC, pen);
			RECT rc2 = { CDPIAware::Instance().ScaleX(LOCATOR_WIDTH), Y + 1, m_blamewidth, Y + (wrapcount * height) - 1};
			::MoveToEx(hDC, rc2.left, rc2.top, nullptr);
			::LineTo(hDC, rc2.right, rc2.top);
			::LineTo(hDC, rc2.right, rc2.bottom);
			::LineTo(hDC, rc2.left, rc2.bottom);
			::LineTo(hDC, rc2.left, rc2.top);
			SelectObject(hDC, hPenOld);
			DeleteObject(pen);
		}
		Y += wrapcount * height;
		::SelectObject(hDC, oldfont);
	}
}

void CTortoiseGitBlameView::DrawLocatorBar(HDC hDC)
{
	if (!hDC)
		return;

	int line = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));
	int linesonscreen = static_cast<int>(SendEditor(SCI_LINESONSCREEN));
	int Y = 0;
	COLORREF blackColor = GetSysColor(COLOR_WINDOWTEXT);

	RECT rc;
	//::GetClientRect(wLocator, &rc);
	this->GetClientRect(&rc);

	rc.right = CDPIAware::Instance().ScaleX(LOCATOR_WIDTH);

	RECT lineRect = rc;
	LONG height = rc.bottom-rc.top;

	auto numberOfLines = static_cast<int>(m_data.GetNumberOfLines());
	// draw the colored bar
	for (int currentLine = 0; currentLine < numberOfLines; ++currentLine)
	{
		COLORREF cr = GetLineColor(currentLine);
		// get the line color
		if ((currentLine >= line)&&(currentLine < (line + linesonscreen)))
		{
			cr = InterColor(cr, blackColor, 10);
		}
		SetBkColor(hDC, cr);
		lineRect.top = static_cast<LONG>(Y);
		lineRect.bottom = ((currentLine + 1) * height / numberOfLines);
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, nullptr, 0, nullptr);
		Y = lineRect.bottom;
	}

	if (numberOfLines > 0)
	{
		// now draw two lines indicating the scroll position of the source view
		SetBkColor(hDC, blackColor);
		lineRect.top = static_cast<LONG>(line) * height / numberOfLines;
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, nullptr, 0, nullptr);
		lineRect.top = static_cast<LONG>(line + linesonscreen) * height / numberOfLines;
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, nullptr, 0, nullptr);
	}
}

void CTortoiseGitBlameView::SetupLexer(CString filename)
{
	int start = filename.ReverseFind(L'.');
	SendEditor(SCI_SETLEXER, SCLEX_NULL);
	if (!m_bLexer)
		return;
	if (start > 0)
	{
		//wcscpy_s(line, 20, lineptr+1);
		//_wcslwr_s(line, 20);
		CString ext=filename.Right(filename.GetLength()-start-1);
		const TCHAR* line = ext;

		if ((wcscmp(line, L"py") == 0) ||
			(wcscmp(line, L"pyw") == 0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PYTHON);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield").GetBuffer()));
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
		if ((wcscmp(line, L"c") == 0) ||
			(wcscmp(line, L"cc") == 0) ||
			(wcscmp(line, L"cpp") == 0) ||
			(wcscmp(line, L"cxx") == 0) ||
			(wcscmp(line, L"h") == 0) ||
			(wcscmp(line, L"hh") == 0) ||
			(wcscmp(line, L"hpp") == 0) ||
			(wcscmp(line, L"hxx") == 0) ||
			(wcscmp(line, L"dlg") == 0) ||
			(wcscmp(line, L"mak") == 0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"and and_eq asm auto bitand bitor bool break \
case catch char class compl const const_cast continue \
default delete do double dynamic_cast else enum explicit export extern false float for \
friend goto if inline int long mutable namespace new not not_eq \
operator or or_eq private protected public \
register reinterpret_cast return short signed sizeof static static_cast struct switch \
template this throw true try typedef typeid typename union unsigned using \
virtual void volatile wchar_t while xor xor_eq").GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"a addindex addtogroup anchor arg attention \
author b brief bug c class code date def defgroup deprecated dontinclude \
e em endcode endhtmlonly endif endlatexonly endlink endverbatim enum example exception \
f$ f[ f] file fn hideinitializer htmlinclude htmlonly \
if image include ingroup internal invariant interface latexonly li line link \
mainpage name namespace nosubgrouping note overload \
p page par param post pre ref relates remarks return retval \
sa section see showinitializer since skip skipline struct subsection \
test throw todo typedef union until \
var verbatim verbinclude version warning weakgroup $ @ \\ & < > # { }").GetBuffer()));
			SetupCppLexer();
		}
		if (wcscmp(line, L"cs") == 0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"abstract as base bool break byte case catch char checked class \
const continue decimal default delegate do double else enum \
event explicit extern false finally fixed float for foreach goto if \
implicit in int interface internal is lock long namespace new null \
object operator out override params private protected public \
readonly ref return sbyte sealed short sizeof stackalloc static \
string struct switch this throw true try typeof uint ulong \
unchecked unsafe ushort using virtual void while").GetBuffer()));
			SetupCppLexer();
		}
		if ((wcscmp(line, L"rc") == 0) ||
			(wcscmp(line, L"rc2") == 0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"ACCELERATORS ALT AUTO3STATE AUTOCHECKBOX AUTORADIOBUTTON \
BEGIN BITMAP BLOCK BUTTON CAPTION CHARACTERISTICS CHECKBOX CLASS \
COMBOBOX CONTROL CTEXT CURSOR DEFPUSHBUTTON DIALOG DIALOGEX DISCARDABLE \
EDITTEXT END EXSTYLE FONT GROUPBOX ICON LANGUAGE LISTBOX LTEXT \
MENU MENUEX MENUITEM MESSAGETABLE POPUP \
PUSHBUTTON RADIOBUTTON RCDATA RTEXT SCROLLBAR SEPARATOR SHIFT STATE3 \
STRINGTABLE STYLE TEXTINCLUDE VALUE VERSION VERSIONINFO VIRTKEY").GetBuffer()));
			SetupCppLexer();
		}
		if ((wcscmp(line, L"idl") == 0) ||
			(wcscmp(line, L"odl") == 0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"aggregatable allocate appobject arrays async async_uuid \
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
v1_enum vararg version void wchar_t wire_marshal").GetBuffer()));
			SetupCppLexer();
		}
		if (wcscmp(line, L"java") == 0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"abstract assert boolean break byte case catch char class \
const continue default do double else extends final finally float for future \
generic goto if implements import inner instanceof int interface long \
native new null outer package private protected public rest \
return short static super switch synchronized this throw throws \
transient try var void volatile while").GetBuffer()));
			SetupCppLexer();
		}
		if (wcscmp(line, L"js") == 0)
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"abstract boolean break byte case catch char class \
const continue debugger default delete do double else enum export extends \
final finally float for function goto if implements import in instanceof \
int interface long native new package private protected public \
return short static super switch synchronized this throw throws \
transient try typeof var void volatile while with").GetBuffer()));
			SetupCppLexer();
		}
		if ((wcscmp(line, L"pas") == 0) ||
			(wcscmp(line, L"dpr") == 0) ||
			(wcscmp(line, L"pp") == 0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_PASCAL);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"and array as begin case class const constructor \
destructor div do downto else end except file finally \
for function goto if implementation in inherited \
interface is mod not object of on or packed \
procedure program property raise record repeat \
set shl shr then threadvar to try type unit \
until uses var while with xor").GetBuffer()));
			SetupCppLexer();
		}
		if ((wcscmp(line, L"as") == 0) ||
			(wcscmp(line, L"asc") == 0) ||
			(wcscmp(line, L"jsfl") == 0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_CPP);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"add and break case catch class continue default delete do \
dynamic else eq extends false finally for function ge get gt if implements import in \
instanceof interface intrinsic le lt ne new not null or private public return \
set static super switch this throw true try typeof undefined var void while with").GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"Array Arguments Accessibility Boolean Button Camera Color \
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
targetPath tellTarget toggleHighQuality trace unescape unloadMovie unLoadMovieNum updateAfterEvent").GetBuffer()));
			SetupCppLexer();
		}
		if ((wcscmp(line, L"html") == 0) ||
			(wcscmp(line, L"htm") == 0) ||
			(wcscmp(line, L"shtml") == 0) ||
			(wcscmp(line, L"htt") == 0) ||
			(wcscmp(line, L"xml") == 0) ||
			(wcscmp(line, L"asp") == 0) ||
			(wcscmp(line, L"xsl") == 0) ||
			(wcscmp(line, L"php") == 0) ||
			(wcscmp(line, L"xhtml") == 0) ||
			(wcscmp(line, L"phtml") == 0) ||
			(wcscmp(line, L"cfm") == 0) ||
			(wcscmp(line, L"tpl") == 0) ||
			(wcscmp(line, L"dtd") == 0) ||
			(wcscmp(line, L"hta") == 0) ||
			(wcscmp(line, L"htd") == 0) ||
			(wcscmp(line, L"wxs") == 0))
		{
			SendEditor(SCI_SETLEXER, SCLEX_HTML);
			SendEditor(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"a abbr acronym address applet area b base basefont \
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
file hidden image").GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 1, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"assign audio block break catch choice clear disconnect else elseif \
emphasis enumerate error exit field filled form goto grammar help \
if initial link log menu meta noinput nomatch object option p paragraph \
param phoneme prompt property prosody record reprompt return s say-as \
script sentence subdialog submit throw transfer value var voice vxml").GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 2, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"accept age alphabet anchor application base beep bridge category charset \
classid cond connecttimeout content contour count dest destexpr dtmf dtmfterm \
duration enctype event eventexpr expr expritem fetchtimeout finalsilence \
gender http-equiv id level maxage maxstale maxtime message messageexpr \
method mime modal mode name namelist next nextitem ph pitch range rate \
scope size sizeexpr skiplist slot src srcexpr sub time timeexpr timeout \
transferaudio type value variant version volume xml:lang").GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 3, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"and assert break class continue def del elif \
else except exec finally for from global if import in is lambda None \
not or pass print raise return try while yield").GetBuffer()));
			SendEditor(SCI_SETKEYWORDS, 4, reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"and argv as argc break case cfunction class continue declare default do \
die echo else elseif empty enddeclare endfor endforeach endif endswitch \
endwhile e_all e_parse e_error e_warning eval exit extends false for \
foreach function global http_cookie_vars http_get_vars http_post_vars \
http_post_files http_env_vars http_server_vars if include include_once \
list new not null old_function or parent php_os php_self php_version \
print require require_once return static switch stdclass this true var \
xor virtual while __file__ __line__ __sleep __wakeup").GetBuffer()));

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
			for (int bstyle = SCE_HB_DEFAULT; bstyle <= SCE_HB_STRINGEOL; ++bstyle) {
				SendEditor(SCI_STYLESETFONT, bstyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"Lucida Console").GetBuffer()));
				SendEditor(SCI_STYLESETBACK, bstyle, lightBlue);
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HB_STRINGEOL, RGB(0x7F,0x7F,0xFF));
			SendEditor(SCI_STYLESETFONT, SCE_HB_COMMENTLINE,
				reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"Lucida Console").GetBuffer()));

			SetAStyle(SCE_HBA_DEFAULT, black);
			SetAStyle(SCE_HBA_COMMENTLINE, darkGreen);
			SetAStyle(SCE_HBA_NUMBER, RGB(0,0x80,0x80));
			SetAStyle(SCE_HBA_WORD, darkBlue);
			SendEditor(SCI_STYLESETBOLD, SCE_HBA_WORD, 1);
			SetAStyle(SCE_HBA_STRING, RGB(0x80,0,0x80));
			SetAStyle(SCE_HBA_IDENTIFIER, black);

			// Show the whole section of ASP VBScript with bright yellow background
			for (int bastyle = SCE_HBA_DEFAULT; bastyle <= SCE_HBA_STRINGEOL; ++bastyle) {
				SendEditor(SCI_STYLESETFONT, bastyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"Lucida Console").GetBuffer()));
				SendEditor(SCI_STYLESETBACK, bastyle, RGB(0xFF, 0xFF, 0));
				// This call extends the backround colour of the last style on the line to the edge of the window
				SendEditor(SCI_STYLESETEOLFILLED, bastyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HBA_STRINGEOL, RGB(0xCF,0xCF,0x7F));
			SendEditor(SCI_STYLESETFONT, SCE_HBA_COMMENTLINE,
				reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"Lucida Console").GetBuffer()));

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
			for (int jstyle = SCE_HJ_DEFAULT; jstyle <= SCE_HJ_SYMBOLS; ++jstyle) {
				SendEditor(SCI_STYLESETFONT, jstyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"Lucida Console").GetBuffer()));
				SendEditor(SCI_STYLESETBACK, jstyle, offWhite);
				SendEditor(SCI_STYLESETEOLFILLED, jstyle, 1);
			}
			SendEditor(SCI_STYLESETBACK, SCE_HJ_STRINGEOL, RGB(0xDF, 0xDF, 0x7F));
			SendEditor(SCI_STYLESETEOLFILLED, SCE_HJ_STRINGEOL, 1);

			// Show the whole section of Javascript with brown background
			for (int jastyle = SCE_HJA_DEFAULT; jastyle <= SCE_HJA_SYMBOLS; ++jastyle) {
				SendEditor(SCI_STYLESETFONT, jastyle,
					reinterpret_cast<LPARAM>(m_TextView.StringForControl(L"Lucida Console").GetBuffer()));
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
	SendEditor(SCI_SETPROPERTY, reinterpret_cast<WPARAM>("lexer.cpp.track.preprocessor"), reinterpret_cast<LPARAM>("0"));
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

	if(type == CFileTextLines::UTF16_LE)
		return 1200;
	if (type == CFileTextLines::UTF16_LEBOM)
	{
		*bomoffset = 2;
		return 1200;
	}

	if(type == CFileTextLines::UTF16_BE)
		return 1201;
	if (type == CFileTextLines::UTF16_BEBOM)
	{
		*bomoffset = 2;
		return 1201;
	}

	return GetACP();
}

void CTortoiseGitBlameView::ParseBlame()
{
	m_data.ParseBlameOutput(GetDocument()->m_BlameData, GetLogData()->m_pLogCache->m_HashMap, m_DateFormat, m_bRelativeTimes);
	CString filename = GetDocument()->m_GitPath.GetGitPathString();
	m_bBlameOutputContainsOtherFilenames = m_data.ContainsOnlyFilename(filename) ? FALSE : TRUE;
}

void CTortoiseGitBlameView::MapLineToLogIndex()
{
	std::vector<int> lineToLogIndex;


	size_t numberOfLines = m_data.GetNumberOfLines();
	lineToLogIndex.reserve(numberOfLines);
	size_t logSize = this->GetLogData()->size();
	for (size_t j = 0; j < numberOfLines; ++j)
	{
		CGitHash& hash = m_data.GetHash(j);

		int index = -2;
		for (size_t i = 0; i < logSize; ++i)
		{
			if (hash == (*GetLogData())[i])
			{
				index = static_cast<int>(i);
				break;
			}
		}
		lineToLogIndex.push_back(index);
	}
	this->m_lineToLogIndex.swap(lineToLogIndex);
}

void CTortoiseGitBlameView::UpdateInfo(int Encode)
{
	CreateFont();

	InitialiseEditor();
	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);

	SendEditor(SCI_SETCODEPAGE, SC_CP_UTF8);

	int encoding = m_data.UpdateEncoding(Encode);

	auto numberOfLines = static_cast<int>(m_data.GetNumberOfLines());
	if (numberOfLines > 0)
	{
		CStringA text;
		for (int i = 0; i < numberOfLines; ++i)
		{
			text += m_data.GetUtf8Line(i);
			text += '\n';
		}
		text.TrimRight("\r\n");
		SendEditor(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(text)));
	}

	{
		UINT nID;
		UINT nStyle;
		int cxWidth;
		int nIndex = static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->m_wndStatusBar.CommandToIndex(ID_INDICATOR_ENCODING);
		static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->m_wndStatusBar.GetPaneInfo(nIndex, nID, nStyle, cxWidth);
		CString sBarText;
		for (int i = 0; i < _countof(encodings); ++i)
		{
			if (encodings[i].id == encoding)
			{
				sBarText = CString(encodings[i].name);
				break;
			}
		}
		//calculate the width of the text
		auto pDC = static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->m_wndStatusBar.GetDC();
		if (pDC)
		{
			CSize size = pDC->GetTextExtent(sBarText);
			static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->m_wndStatusBar.SetPaneInfo(nIndex, nID, nStyle, size.cx + 2);
			ReleaseDC(pDC);
		}
		static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->m_wndStatusBar.SetPaneText(nIndex, sBarText);
	}

#ifdef USE_TEMPFILENAME
	delete m_Buffer;
	m_Buffer = nullptr;

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

	GetBlameWidth();
	CRect rect;
	this->GetClientRect(rect);
	//this->m_TextView.GetWindowRect(rect);
	//this->m_TextView.ScreenToClient(rect);
	rect.left=this->m_blamewidth;
	this->m_TextView.MoveWindow(rect);

	this->Invalidate();
}

CString CTortoiseGitBlameView::ResolveCommitFile(int line)
{
	return ResolveCommitFile(m_data.GetFilename(line));
}

CString CTortoiseGitBlameView::ResolveCommitFile(const CString& path)
{
	if (path.IsEmpty())
	{
		return static_cast<CMainFrame*>(::AfxGetApp()->GetMainWnd())->GetActiveView()->GetDocument()->GetPathName();
	}
	else
	{
		return g_Git.CombinePath(path);
	}
}

COLORREF CTortoiseGitBlameView::GetLineColor(size_t line)
{
	if (m_colorage && line < m_lineToLogIndex.size())
	{
		int logIndex = m_lineToLogIndex[line];
		if (logIndex >= 0)
		{
			int slider = static_cast<int>((GetLogData()->size() - logIndex) * 100 / (GetLogData()->size() + 1));
			return InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), slider);
		}
	}
	return m_windowcolor;
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
	int line = GetLineUnderCursor(point);
	if (static_cast<size_t>(line) < m_data.GetNumberOfLines())
	{
		SetSelectedLine(line);
		if (m_data.GetHash(line) != m_SelectedHash)
		{
			m_SelectedHash = m_data.GetHash(line);

			int logIndex = m_lineToLogIndex[line];
			if (logIndex >= 0)
			{
				this->GetLogList()->SetItemState(logIndex, LVIS_SELECTED, LVIS_SELECTED);
				this->GetLogList()->EnsureVisible(logIndex, FALSE);
			}
			else
			{
				GitRevLoglist* pRev = m_data.GetRev(line, GetLogData()->m_pLogCache->m_HashMap);
				this->GetDocument()->GetMainFrame()->m_wndProperties.UpdateProperties(pRev);
			}
		}
		else
		{
			m_SelectedHash.Empty();
		}
		this->Invalidate();
		this->m_TextView.Invalidate();

	}
	else
	{
		SetSelectedLine(-1);
	}

	CView::OnLButtonDown(nFlags,point);
}

void CTortoiseGitBlameView::OnSciGetBkColor(NMHDR* hdr, LRESULT* /*result*/)
{
	auto notification = reinterpret_cast<SCNotification*>(hdr);

	if (notification->line < static_cast<Sci_Position>(m_data.GetNumberOfLines()))
	{
		if (m_data.GetHash(notification->line) == this->m_SelectedHash)
			notification->lParam = m_selectedauthorcolor;
		else
			notification->lParam = GetLineColor(notification->line);
	}
}

void CTortoiseGitBlameView::FocusOn(GitRevLoglist* pRev)
{
	this->GetDocument()->GetMainFrame()->m_wndProperties.UpdateProperties(pRev);

	this->Invalidate();

	if (m_SelectedHash != pRev->m_CommitHash) {
		m_SelectedHash = pRev->m_CommitHash;
		int line = m_data.FindFirstLine(m_SelectedHash, 0);
		if (line >= 0)
		{
			GotoLine(line + 1);
			m_TextView.Invalidate();
			return;
		}
		SendEditor(SCI_SETSEL, INT_MAX, -1);
	}
}

void CTortoiseGitBlameView::OnMouseHover(UINT /*nFlags*/, CPoint point)
{
	int line = GetLineUnderCursor(point);
	if (m_data.IsValidLine(line))
	{
		if (line != m_MouseLine)
		{
			m_MouseLine = line;
			GitRev *pRev = nullptr;
			int logIndex = m_lineToLogIndex[line];
			if (logIndex >= 0)
				pRev = &GetLogData()->GetGitRevAt(logIndex);
			else
				pRev = m_data.GetRev(line, GetLogData()->m_pLogCache->m_HashMap);

			if (!pRev)
				return;

			CString body = pRev->GetBody();
			int maxLine = 15;
			int iline = 0;
			int pos = 0;
			while (iline++ < maxLine)
			{
				int pos2 = body.Find(L'\n', pos);
				if (pos2 < 0)
					break;
				int lineLength = pos2 - pos - 1;
				pos = pos2 + 1;
				iline += lineLength / 70;
			}

			CString filename;
			if ((m_bShowCompleteLog && m_bFollowRenames && !m_bOnlyFirstParent) || !BlameIsLimitedToOneFilename(m_dwDetectMovedOrCopiedLines) || m_bBlameOutputContainsOtherFilenames)
				filename.Format(L"%s: %s\n", static_cast<LPCTSTR>(m_sFileName), static_cast<LPCTSTR>(m_data.GetFilename(line)));

			CString str;
			str.Format(L"%s: %s\n%s%s: %s <%s>\n%s: %s\n%s:\n%s\n%s",	static_cast<LPCTSTR>(m_sRev), static_cast<LPCTSTR>(pRev->m_CommitHash.ToString()), static_cast<LPCTSTR>(filename),
																	static_cast<LPCTSTR>(m_sAuthor), static_cast<LPCTSTR>(pRev->GetAuthorName()), static_cast<LPCTSTR>(pRev->GetAuthorEmail()),
																	static_cast<LPCTSTR>(m_sDate), static_cast<LPCTSTR>(CLoglistUtils::FormatDateAndTime(pRev->GetAuthorDate(), m_DateFormat, true, m_bRelativeTimes)),
																	static_cast<LPCTSTR>(m_sMessage), static_cast<LPCTSTR>(pRev->GetSubject()),
																	iline <= maxLine ? static_cast<LPCTSTR>(body) : (body.Left(pos) + L"\n...................."));

			m_ToolTip.Pop();
			m_ToolTip.AddTool(this, str);

			Invalidate();
		}
	}
}

void CTortoiseGitBlameView::OnMouseMove(UINT /*nFlags*/, CPoint /*point*/)
{
	TRACKMOUSEEVENT tme;
	tme.cbSize=sizeof(TRACKMOUSEEVENT);
	tme.dwFlags=TME_HOVER|TME_LEAVE;
	tme.hwndTrack=this->m_hWnd;
	tme.dwHoverTime=1;
	TrackMouseEvent(&tme);
	Invalidate();
}

void CTortoiseGitBlameView::OnMouseLeave()
{
	if (m_MouseLine == -1)
		return;

	m_MouseLine = -1;
	Invalidate();
}

BOOL CTortoiseGitBlameView::PreTranslateMessage(MSG* pMsg)
{
	if (m_ToolTip.GetSafeHwnd())
		m_ToolTip.RelayEvent(pMsg);
	return CView::PreTranslateMessage(pMsg);
}

void CTortoiseGitBlameView::OnEditFind()
{
	if (m_pFindDialog)
		return;

	m_pFindDialog=new CFindReplaceDialog();

	CString oneline = m_sFindText;
	if (m_TextView.Call(SCI_GETSELECTIONSTART) != m_TextView.Call(SCI_GETSELECTIONEND))
	{
		LRESULT bufsize = m_TextView.Call(SCI_GETSELECTIONEND) - m_TextView.Call(SCI_GETSELECTIONSTART);
		auto linebuf = std::make_unique<char[]>(bufsize + 1);
		SecureZeroMemory(linebuf.get(), bufsize + 1);
		SendEditor(SCI_GETSELTEXT, 0, reinterpret_cast<LPARAM>(linebuf.get()));
		oneline = m_TextView.StringFromControl(linebuf.get());
	}

	DWORD flags = FR_DOWN | FR_HIDEWHOLEWORD;
	if (theApp.GetInt(L"FindMatchCase"))
		flags |= FR_MATCHCASE;

	m_pFindDialog->Create(TRUE, oneline, nullptr, flags, this);
}

void CTortoiseGitBlameView::OnEditGoto()
{
	CEditGotoDlg dlg;
	if(dlg.DoModal()==IDOK)
	{
		this->GotoLine(dlg.m_LineNumber);
	}
}

LRESULT CTortoiseGitBlameView::OnFindDialogMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	ASSERT(m_pFindDialog);

	// If the FR_DIALOGTERM flag is set,
	// invalidate the handle identifying the dialog box.
	if (m_pFindDialog->IsTerminating())
	{
			m_pFindDialog = nullptr;
			return 0;
	}

	// If the FR_FINDNEXT flag is set,
	// call the application-defined search routine
	// to search for the requested string.
	if(m_pFindDialog->FindNext())
	{
		m_bMatchCase = !!(m_pFindDialog->MatchCase());
		m_sFindText = m_pFindDialog->GetFindString();

		theApp.WriteInt(L"FindMatchCase", m_bMatchCase ? 1 : 0);
		theApp.WriteString(L"FindString", m_sFindText);

		DoSearch(m_pFindDialog->SearchDown() ?  CTortoiseGitBlameData::SearchNext : CTortoiseGitBlameData::SearchPrevious);
	}

	return 0;
}

void CTortoiseGitBlameView::OnViewNext()
{
	int startline = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));
	int line = m_data.FindNextLine(this->m_SelectedHash, static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE), false));
	if(line >= 0)
		SendEditor(SCI_LINESCROLL, 0, line - startline - 2);
}
void CTortoiseGitBlameView::OnViewPrev()
{
	int startline = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE));
	int line = m_data.FindNextLine(this->m_SelectedHash, static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE)), true);
	if(line >= 0)
		SendEditor(SCI_LINESCROLL, 0, line - startline - 2);
}

void CTortoiseGitBlameView::OnViewToggleAuthor()
{
	m_bShowAuthor = ! m_bShowAuthor;

	theApp.WriteInt(L"ShowAuthor", m_bShowAuthor);

	CRect rect;
	this->GetClientRect(&rect);
	rect.left=GetBlameWidth();

	m_TextView.MoveWindow(&rect);
}

void CTortoiseGitBlameView::OnUpdateViewToggleAuthor(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bShowAuthor);
}

void CTortoiseGitBlameView::OnViewToggleDate()
{
	m_bShowDate = ! m_bShowDate;

	theApp.WriteInt(L"ShowDate", m_bShowDate);

	CRect rect;
	this->GetClientRect(&rect);
	rect.left=GetBlameWidth();

	m_TextView.MoveWindow(&rect);
}

void CTortoiseGitBlameView::OnUpdateViewToggleDate(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bShowDate);
}

void CTortoiseGitBlameView::OnViewToggleShowFilename()
{
	m_bShowFilename = ! m_bShowFilename;

	theApp.WriteInt(L"ShowFilename", m_bShowFilename);

	CRect rect;
	this->GetClientRect(&rect);
	rect.left = GetBlameWidth();

	m_TextView.MoveWindow(&rect);
}

void CTortoiseGitBlameView::OnUpdateViewToggleShowFilename(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bShowFilename);
}

void CTortoiseGitBlameView::OnViewToggleShowOriginalLineNumber()
{
	m_bShowOriginalLineNumber = ! m_bShowOriginalLineNumber;

	theApp.WriteInt(L"ShowOriginalLineNumber", m_bShowOriginalLineNumber);

	CRect rect;
	this->GetClientRect(&rect);
	rect.left = GetBlameWidth();

	m_TextView.MoveWindow(&rect);
}

void CTortoiseGitBlameView::OnUpdateViewToggleShowOriginalLineNumber(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bShowOriginalLineNumber);
}

void CTortoiseGitBlameView::OnViewDetectMovedOrCopiedLines(DWORD dwDetectMovedOrCopiedLines)
{
	m_dwDetectMovedOrCopiedLines = dwDetectMovedOrCopiedLines;

	theApp.DoWaitCursor(1);

	theApp.WriteInt(L"DetectMovedOrCopiedLines", m_dwDetectMovedOrCopiedLines);

	auto document = static_cast<CTortoiseGitBlameDoc*>(m_pDocument);
	if (!document->m_CurrentFileName.IsEmpty())
	{
		document->m_lLine = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE)) + 1;
		theApp.m_pDocManager->OnFileNew();
		document->OnOpenDocument(document->m_CurrentFileName, document->m_Rev);
	}
	theApp.DoWaitCursor(-1);
}

void CTortoiseGitBlameView::OnViewDetectMovedOrCopiedLinesToggleDisabled()
{
	OnViewDetectMovedOrCopiedLines(BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED);
}

void CTortoiseGitBlameView::OnUpdateViewDetectMovedOrCopiedLinesToggleDisabled(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_DISABLED);
}

void CTortoiseGitBlameView::OnViewDetectMovedOrCopiedLinesToggleWithinFile()
{
	OnViewDetectMovedOrCopiedLines(BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE);
}

void CTortoiseGitBlameView::OnUpdateViewDetectMovedOrCopiedLinesToggleWithinFile(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_WITHIN_FILE);
}

void CTortoiseGitBlameView::OnViewDetectMovedOrCopiedLinesToggleFromModifiedFiles()
{
	OnViewDetectMovedOrCopiedLines(BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES);
}

void CTortoiseGitBlameView::OnUpdateViewDetectMovedOrCopiedLinesToggleFromModifiedFiles(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_MODIFIED_FILES);
}

void CTortoiseGitBlameView::OnViewDetectMovedOrCopiedLinesToggleFromExistingFilesAtFileCreation()
{
	OnViewDetectMovedOrCopiedLines(BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION);
}

void CTortoiseGitBlameView::OnUpdateViewDetectMovedOrCopiedLinesToggleFromExistingFilesAtFileCreation(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES_AT_FILE_CREATION);
}

void CTortoiseGitBlameView::OnViewDetectMovedOrCopiedLinesToggleFromExistingFiles()
{
	OnViewDetectMovedOrCopiedLines(BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES);
}

void CTortoiseGitBlameView::OnUpdateViewDetectMovedOrCopiedLinesToggleFromExistingFiles(CCmdUI *pCmdUI)
{
	pCmdUI->SetRadio(m_dwDetectMovedOrCopiedLines == BLAME_DETECT_MOVED_OR_COPIED_LINES_FROM_EXISTING_FILES);
}

void CTortoiseGitBlameView::ReloadDocument()
{
	theApp.DoWaitCursor(1);
	auto document = static_cast<CTortoiseGitBlameDoc*>(m_pDocument);
	if (!document->m_CurrentFileName.IsEmpty())
	{
		document->m_lLine = static_cast<int>(SendEditor(SCI_GETFIRSTVISIBLELINE)) + 1;
		theApp.m_pDocManager->OnFileNew();
		document->OnOpenDocument(document->m_CurrentFileName, document->m_Rev);
	}
	theApp.DoWaitCursor(-1);
}

void CTortoiseGitBlameView::OnViewToggleIgnoreWhitespace()
{
	m_bIgnoreWhitespace = ! m_bIgnoreWhitespace;

	theApp.WriteInt(L"IgnoreWhitespace", m_bIgnoreWhitespace ? 1 : 0);

	ReloadDocument();
}

void CTortoiseGitBlameView::OnUpdateViewToggleIgnoreWhitespace(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bIgnoreWhitespace);
}

void CTortoiseGitBlameView::OnViewToggleShowCompleteLog()
{
	m_bShowCompleteLog = ! m_bShowCompleteLog;

	theApp.WriteInt(L"ShowCompleteLog", m_bShowCompleteLog ? 1 : 0);

	ReloadDocument();
}

void CTortoiseGitBlameView::OnUpdateViewToggleOnlyFirstParent(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bOnlyFirstParent);
}

void CTortoiseGitBlameView::OnViewToggleOnlyFirstParent()
{
	m_bOnlyFirstParent = !m_bOnlyFirstParent;

	theApp.WriteInt(L"OnlyFirstParent", m_bOnlyFirstParent ? 1 : 0);

	ReloadDocument();
}

void CTortoiseGitBlameView::OnUpdateViewToggleShowCompleteLog(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(BlameIsLimitedToOneFilename(m_dwDetectMovedOrCopiedLines) && !m_bOnlyFirstParent);
	pCmdUI->SetCheck(m_bShowCompleteLog && BlameIsLimitedToOneFilename(m_dwDetectMovedOrCopiedLines) && !m_bOnlyFirstParent);
}

void CTortoiseGitBlameView::OnViewToggleFollowRenames()
{
	m_bFollowRenames = ! m_bFollowRenames;

	theApp.WriteInt(L"FollowRenames", m_bFollowRenames ? 1 : 0);

	ReloadDocument();
}

void CTortoiseGitBlameView::OnUpdateViewToggleFollowRenames(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(m_bShowCompleteLog && BlameIsLimitedToOneFilename(m_dwDetectMovedOrCopiedLines) && !m_bOnlyFirstParent);
	pCmdUI->SetCheck(m_bFollowRenames && m_bShowCompleteLog && BlameIsLimitedToOneFilename(m_dwDetectMovedOrCopiedLines) && !m_bOnlyFirstParent);
}

void CTortoiseGitBlameView::OnViewToggleColorByAge()
{
	m_colorage = ! m_colorage;

	theApp.WriteInt(L"ColorAge", m_colorage);

	m_TextView.Invalidate();
}

void CTortoiseGitBlameView::OnUpdateViewToggleColorByAge(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_colorage);
}

void CTortoiseGitBlameView::OnViewToggleLexer()
{
	m_bLexer = !m_bLexer;

	theApp.WriteInt(L"EnableLexer", m_bLexer);

	InitialiseEditor();
	SetupLexer(GetDocument()->m_CurrentFileName);
	this->Invalidate();
}

void CTortoiseGitBlameView::OnUpdateViewToggleLexer(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bLexer);
}

void CTortoiseGitBlameView::OnViewWrapLongLines()
{
	m_bWrapLongLines = !m_bWrapLongLines;

	theApp.WriteInt(L"WrapLongLines", m_bWrapLongLines);

	if (m_bWrapLongLines)
		SendEditor(SCI_SETWRAPMODE, SC_WRAP_WORD);
	else
		SendEditor(SCI_SETWRAPMODE, SC_WRAP_NONE);
}

void CTortoiseGitBlameView::OnUpdateViewWrapLongLines(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_bWrapLongLines);
}

void CTortoiseGitBlameView::OnUpdateViewCopyToClipboard(CCmdUI *pCmdUI)
{
	CWnd * wnd = GetFocus();
	if (wnd == GetLogList())
		pCmdUI->Enable(GetLogList()->GetSelectedCount() > 0);
	else if (wnd)
	{
		if (CString(wnd->GetRuntimeClass()->m_lpszClassName) == L"CMFCPropertyGridCtrl")
		{
			auto grid = static_cast<CMFCPropertyGridCtrl*>(wnd);
			pCmdUI->Enable(grid->GetCurSel() && !grid->GetCurSel()->IsGroup() && !CString(grid->GetCurSel()->GetValue()).IsEmpty());
		}
		else
			pCmdUI->Enable(m_TextView.Call(SCI_GETSELECTIONSTART) != m_TextView.Call(SCI_GETSELECTIONEND));
	}
	else
		pCmdUI->Enable(FALSE);
}

void CTortoiseGitBlameView::OnSysColorChange()
{
	__super::OnSysColorChange();
	m_windowcolor = ::GetSysColor(COLOR_WINDOW);
	m_textcolor = ::GetSysColor(COLOR_WINDOWTEXT);
	m_texthighlightcolor = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
	m_mouserevcolor = InterColor(m_windowcolor, m_textcolor, 20);
	m_mouseauthorcolor = InterColor(m_windowcolor, m_textcolor, 10);
	m_selectedrevcolor = ::GetSysColor(COLOR_HIGHLIGHT);
	m_selectedauthorcolor = InterColor(m_selectedrevcolor, m_texthighlightcolor, 35);
	InitialiseEditor();
	SetupLexer(GetDocument()->m_CurrentFileName);
}

ULONG CTortoiseGitBlameView::GetGestureStatus(CPoint ptTouch)
{
	int line = GetLineUnderCursor(ptTouch);
	if (m_data.IsValidLine(line))
		return 0;
	return __super::GetGestureStatus(ptTouch);
}
