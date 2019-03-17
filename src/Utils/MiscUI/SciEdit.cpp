// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2012-2018 - TortoiseSVN

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
#include "LoglistCommonResource.h"
#include "PathUtils.h"
#include "UnicodeUtils.h"
#include <string>
#include "registry.h"
#include "SciEdit.h"
#include "SmartHandle.h"
#include "../../TortoiseUDiff/UDiffColors.h"
#include "LoadIconEx.h"

void CSciEditContextMenuInterface::InsertMenuItems(CMenu&, int&) {return;}
bool CSciEditContextMenuInterface::HandleMenuItemClick(int, CSciEdit *) {return false;}
void CSciEditContextMenuInterface::HandleSnippet(int, const CString &, CSciEdit *) { return; }


#define STYLE_ISSUEBOLD			11
#define STYLE_ISSUEBOLDITALIC	12
#define STYLE_BOLD				14
#define STYLE_ITALIC			15
#define STYLE_UNDERLINED		16
#define STYLE_URL				17
#define INDIC_MISSPELLED		18

#define STYLE_MASK 0x1f

#define SCI_ADDWORD			2000

struct loc_map {
	const char * cp;
	const char * def_enc;
};

struct loc_map enc2locale[] = {
	{"28591","ISO8859-1"},
	{"28592","ISO8859-2"},
	{"28593","ISO8859-3"},
	{"28594","ISO8859-4"},
	{"28595","ISO8859-5"},
	{"28596","ISO8859-6"},
	{"28597","ISO8859-7"},
	{"28598","ISO8859-8"},
	{"28599","ISO8859-9"},
	{"28605","ISO8859-15"},
	{"20866","KOI8-R"},
	{"21866","KOI8-U"},
	{"1251","microsoft-cp1251"},
	{"65001","UTF-8"},
	};


IMPLEMENT_DYNAMIC(CSciEdit, CWnd)

CSciEdit::CSciEdit(void) : m_DirectFunction(NULL)
	, m_DirectPointer(NULL)
	, m_spellcodepage(0)
	, m_separator(0)
	, m_typeSeparator(1)
	, m_bDoStyle(false)
	, m_nAutoCompleteMinChars(3)
	, m_SpellingCache(2000)
	, m_blockModifiedHandler(false)
{
	m_hModule = ::LoadLibrary(L"SciLexer_tgit.dll");
}

CSciEdit::~CSciEdit(void)
{
	m_personalDict.Save();
}

static std::unique_ptr<UINT[]> Icon2Image(HICON hIcon)
{
	if (hIcon == nullptr)
		return nullptr;

	ICONINFO iconInfo;
	if (!GetIconInfo(hIcon, &iconInfo))
		return nullptr;

	BITMAP bm;
	if (!GetObject(iconInfo.hbmColor, sizeof(BITMAP), &bm))
		return nullptr;

	int width = bm.bmWidth;
	int height = bm.bmHeight;
	int bytesPerScanLine = (width * 3 + 3) & 0xFFFFFFFC;
	int size = bytesPerScanLine * height;
	BITMAPINFO infoheader;
	infoheader.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	infoheader.bmiHeader.biWidth = width;
	infoheader.bmiHeader.biHeight = height;
	infoheader.bmiHeader.biPlanes = 1;
	infoheader.bmiHeader.biBitCount = 24;
	infoheader.bmiHeader.biCompression = BI_RGB;
	infoheader.bmiHeader.biSizeImage = size;

	auto ptrb = std::make_unique<BYTE[]>(size * 2 + height * width * 4);
	LPBYTE pixelsIconRGB = ptrb.get();
	LPBYTE alphaPixels = pixelsIconRGB + size;
	HDC hDC = CreateCompatibleDC(nullptr);
	SCOPE_EXIT { DeleteDC(hDC); };
	HBITMAP hBmpOld = static_cast<HBITMAP>(SelectObject(hDC, iconInfo.hbmColor));
	if (!GetDIBits(hDC, iconInfo.hbmColor, 0, height, static_cast<LPVOID>(pixelsIconRGB), &infoheader, DIB_RGB_COLORS))
		return nullptr;

	SelectObject(hDC, hBmpOld);
	if (!GetDIBits(hDC, iconInfo.hbmMask, 0, height, static_cast<LPVOID>(alphaPixels), &infoheader, DIB_RGB_COLORS))
		return nullptr;

	auto imagePixels = std::make_unique<UINT[]>(height * width);
	int lsSrc = width * 3;
	int vsDest = height - 1;
	for (int y = 0; y < height; y++)
	{
		int linePosSrc = (vsDest - y) * lsSrc;
		int linePosDest = y * width;
		for (int x = 0; x < width; x++)
		{
			int currentDestPos = linePosDest + x;
			int currentSrcPos = linePosSrc + x * 3;
			imagePixels[currentDestPos] = ((static_cast<UINT>(
				(
					((pixelsIconRGB[currentSrcPos + 2]  /*Red*/)
					| (pixelsIconRGB[currentSrcPos + 1] << 8 /*Green*/))
					| pixelsIconRGB[currentSrcPos] << 16 /*Blue*/
				)
				| ((alphaPixels[currentSrcPos] ? 0 : 0xff) << 24))) & 0xffffffff);
		}
	}
	return imagePixels;
}

void CSciEdit::SetColors(bool recolorize)
{
	Call(SCI_STYLESETFORE, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT));
	Call(SCI_STYLESETBACK, STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOW));
	Call(SCI_SETSELFORE, TRUE, ::GetSysColor(COLOR_HIGHLIGHTTEXT));
	Call(SCI_SETSELBACK, TRUE, ::GetSysColor(COLOR_HIGHLIGHT));
	Call(SCI_SETCARETFORE, ::GetSysColor(COLOR_WINDOWTEXT));

	if (recolorize)
		Call(SCI_COLOURISE, 0, -1);
}

void CSciEdit::Init(LONG lLanguage)
{
	//Setup the direct access data
	m_DirectFunction = SendMessage(SCI_GETDIRECTFUNCTION, 0, 0);
	m_DirectPointer = SendMessage(SCI_GETDIRECTPOINTER, 0, 0);
	Call(SCI_SETMARGINWIDTHN, 1, 0);
	Call(SCI_SETUSETABS, 0);		//pressing TAB inserts spaces
	Call(SCI_SETWRAPVISUALFLAGS, SC_WRAPVISUALFLAG_END);
	Call(SCI_AUTOCSETIGNORECASE, 1);
	Call(SCI_SETLEXER, SCLEX_CONTAINER);
	Call(SCI_SETCODEPAGE, SC_CP_UTF8);
	Call(SCI_AUTOCSETFILLUPS, 0, reinterpret_cast<LPARAM>("\t(["));
	Call(SCI_AUTOCSETMAXWIDTH, 0);
	//Set the default windows colors for edit controls
	SetColors(false);
	Call(SCI_SETMODEVENTMASK, SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT | SC_PERFORMED_UNDO | SC_PERFORMED_REDO);
	Call(SCI_INDICSETSTYLE, INDIC_MISSPELLED, INDIC_SQUIGGLE);
	Call(SCI_INDICSETFORE, INDIC_MISSPELLED, RGB(255,0,0));
	CStringA sWordChars;
	CStringA sWhiteSpace;
	for (int i=0; i<255; ++i)
	{
		if (i == '\r' || i == '\n')
			continue;
		else if (i < 0x20 || i == ' ')
			sWhiteSpace += static_cast<char>(i);
		else if (isalnum(i) || i == '\'' || i == '_' || i == '-')
			sWordChars += static_cast<char>(i);
	}
	Call(SCI_SETWORDCHARS, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sWordChars)));
	Call(SCI_SETWHITESPACECHARS, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sWhiteSpace)));
	m_bDoStyle = static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\StyleCommitMessages", TRUE)) == TRUE;
	m_nAutoCompleteMinChars = static_cast<int>(CRegStdDWORD(L"Software\\TortoiseGit\\AutoCompleteMinChars", 3));
	// look for dictionary files and use them if found
	if (lLanguage >= 0 && static_cast<DWORD>(CRegStdDWORD(L"Software\\TortoiseGit\\Spellchecker", TRUE)) == TRUE)
	{
		if (!lLanguage || (lLanguage && !LoadDictionaries(lLanguage)))
		{
			long langId = GetUserDefaultLCID();
			do
			{
				LoadDictionaries(langId);
				DWORD lid = SUBLANGID(langId);
				lid--;
				if (lid > 0)
					langId = MAKELANGID(PRIMARYLANGID(langId), lid);
				else if (langId == 1033)
					langId = 0;
				else
					langId = 1033;
			} while (langId && (!pChecker || !pThesaur));
		}
	}

	Call(SCI_SETEDGEMODE, EDGE_NONE);
	Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
	Call(SCI_ASSIGNCMDKEY, SCK_END, SCI_LINEENDWRAP);
	Call(SCI_ASSIGNCMDKEY, SCK_END + (SCMOD_SHIFT << 16), SCI_LINEENDWRAPEXTEND);
	Call(SCI_ASSIGNCMDKEY, SCK_HOME, SCI_HOMEWRAP);
	Call(SCI_ASSIGNCMDKEY, SCK_HOME + (SCMOD_SHIFT << 16), SCI_HOMEWRAPEXTEND);
	if (CRegStdDWORD(L"Software\\TortoiseGit\\ScintillaDirect2D", FALSE) != FALSE)
	{
		// set font quality for the popup window, since that window does not use D2D
		Call(SCI_SETFONTQUALITY, SC_EFF_QUALITY_LCD_OPTIMIZED);
		// now enable D2D
		Call(SCI_SETTECHNOLOGY, SC_TECHNOLOGY_DIRECTWRITERETAIN);
		Call(SCI_SETBUFFEREDDRAW, 0);
	}
}


void CSciEdit::Init(const ProjectProperties& props)
{
	Init(props.lProjectLanguage);
	m_sCommand = CUnicodeUtils::GetUTF8(props.GetCheckRe());
	m_sBugID = CUnicodeUtils::GetUTF8(props.GetBugIDRe());
	m_sUrl = CUnicodeUtils::GetUTF8(props.sUrl);

	Call(SCI_SETMOUSEDWELLTIME, 333);

	if (props.nLogWidthMarker)
	{
		Call(SCI_SETWRAPMODE, SC_WRAP_NONE);
		Call(SCI_SETEDGEMODE, EDGE_LINE);
		Call(SCI_SETEDGECOLUMN, props.nLogWidthMarker);
		Call(SCI_SETSCROLLWIDTHTRACKING, TRUE);
		Call(SCI_SETSCROLLWIDTH, 1);
	}
	else
	{
		Call(SCI_SETEDGEMODE, EDGE_NONE);
		Call(SCI_SETWRAPMODE, SC_WRAP_WORD);
	}
}

void CSciEdit::SetIcon(const std::map<int, UINT> &icons)
{
	int iconWidth = GetSystemMetrics(SM_CXSMICON);
	int iconHeight = GetSystemMetrics(SM_CYSMICON);
	Call(SCI_RGBAIMAGESETWIDTH, iconWidth);
	Call(SCI_RGBAIMAGESETHEIGHT, iconHeight);
	for (auto icon : icons)
	{
		auto hIcon = LoadIconEx(AfxGetInstanceHandle(), MAKEINTRESOURCE(icon.second), iconWidth, iconHeight);
		auto bytes = Icon2Image(hIcon);
		DestroyIcon(hIcon);
		Call(SCI_REGISTERRGBAIMAGE, icon.first, reinterpret_cast<LPARAM>(bytes.get()));
	}
}

BOOL CSciEdit::LoadDictionaries(LONG lLanguageID)
{
	//Setup the spell checker and thesaurus
	TCHAR buf[6] = { 0 };
	CString sFolderUp = CPathUtils::GetAppParentDirectory();
	CString sFolderAppData = CPathUtils::GetAppDataDirectory();
	CString sFile;

	GetLocaleInfo(MAKELCID(lLanguageID, SORT_DEFAULT), LOCALE_SISO639LANGNAME, buf, _countof(buf));
	sFile = buf;
	if (lLanguageID == 2074)
		sFile += L"-Latn";
	sFile += L'_';
	GetLocaleInfo(MAKELCID(lLanguageID, SORT_DEFAULT), LOCALE_SISO3166CTRYNAME, buf, _countof(buf));
	sFile += buf;
	if (!pChecker)
	{
		if ((PathFileExists(sFolderAppData + L"dic\\" + sFile + L".aff")) &&
			(PathFileExists(sFolderAppData + L"dic\\" + sFile + L".dic")))
		{
			pChecker = std::make_unique<Hunspell>(CStringA(sFolderAppData + L"dic\\" + sFile + L".aff"), CStringA(sFolderAppData + L"dic\\" + sFile + L".dic"));
		}
		else if ((PathFileExists(sFolderUp + L"Languages\\" + sFile + L".aff")) &&
			(PathFileExists(sFolderUp + L"Languages\\" + sFile + L".dic")))
		{
			pChecker = std::make_unique<Hunspell>(CStringA(sFolderUp + L"Languages\\" + sFile + L".aff"), CStringA(sFolderUp + L"Languages\\" + sFile + L".dic"));
		}
		if (pChecker)
		{
			const char* encoding = pChecker->get_dic_encoding();
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": %s\n", encoding);
			m_spellcodepage = 0;
			for (int i = 0; i < _countof(enc2locale); ++i)
			{
				if (strcmp(encoding, enc2locale[i].def_enc) == 0)
					m_spellcodepage = atoi(enc2locale[i].cp);
			}
			m_personalDict.Init(lLanguageID);
		}
	}
#if THESAURUS
	if (!pThesaur)
	{
		if ((PathFileExists(sFolderAppData + L"dic\\th_" + sFile + L"_v2.idx")) &&
			(PathFileExists(sFolderAppData + L"dic\\th_" + sFile + L"_v2.dat")))
		{
			pThesaur = std::make_unique<MyThes>(CStringA(sFolderAppData + L"dic\\th_" + sFile + L"_v2.idx"), CStringA(sFolderAppData + L"dic\\th_" + sFile + L"_v2.dat"));
		}
		else if ((PathFileExists(sFolderUp + L"Languages\\th_" + sFile + L"_v2.idx")) &&
			(PathFileExists(sFolderUp + L"Languages\\th_" + sFile + L"_v2.dat")))
		{
			pThesaur = std::make_unique<MyThes>(CStringA(sFolderUp + L"Languages\\th_" + sFile + L"_v2.idx"), CStringA(sFolderUp + L"Languages\\th_" + sFile + L"_v2.dat"));
		}
	}
#endif
	if ((pThesaur)||(pChecker))
		return TRUE;
	return FALSE;
}

LRESULT CSciEdit::Call(UINT message, WPARAM wParam, LPARAM lParam)
{
	ASSERT(::IsWindow(m_hWnd)); //Window must be valid
	ASSERT(m_DirectFunction); //Direct function must be valid
	return reinterpret_cast<SciFnDirect>(m_DirectFunction)(m_DirectPointer, message, wParam, lParam);
}

CString CSciEdit::StringFromControl(const CStringA& text)
{
	CString sText;
#ifdef UNICODE
	int codepage = static_cast<int>(Call(SCI_GETCODEPAGE));
	int reslen = MultiByteToWideChar(codepage, 0, text, text.GetLength(), 0, 0);
	MultiByteToWideChar(codepage, 0, text, text.GetLength(), sText.GetBuffer(reslen+1), reslen+1);
	sText.ReleaseBuffer(reslen);
#else
	sText = text;
#endif
	return sText;
}

CStringA CSciEdit::StringForControl(const CString& text)
{
	CStringA sTextA;
#ifdef UNICODE
	int codepage = static_cast<int>(SendMessage(SCI_GETCODEPAGE));
	int reslen = WideCharToMultiByte(codepage, 0, text, text.GetLength(), 0, 0, 0, 0);
	WideCharToMultiByte(codepage, 0, text, text.GetLength(), sTextA.GetBuffer(reslen), reslen, 0, 0);
	sTextA.ReleaseBuffer(reslen);
#else
	sTextA = text;
#endif
	ATLTRACE("string length %d\n", sTextA.GetLength());
	return sTextA;
}

void CSciEdit::SetText(const CString& sText)
{
	CStringA sTextA = StringForControl(sText);
	Call(SCI_SETTEXT, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));

	if (Call(SCI_GETSCROLLWIDTHTRACKING) != 0)
		Call(SCI_SETSCROLLWIDTH, 1);

	// Scintilla seems to have problems with strings that
	// aren't terminated by a newline char. Once that char
	// is there, it can be removed without problems.
	// So we add here a newline, then remove it again.
	Call(SCI_DOCUMENTEND);
	Call(SCI_NEWLINE);
	Call(SCI_DELETEBACK);
}

void CSciEdit::InsertText(const CString& sText, bool bNewLine)
{
	CStringA sTextA = StringForControl(sText);
	Call(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));
	if (bNewLine)
		Call(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>("\n"));
}

CString CSciEdit::GetText()
{
	auto len = static_cast<int>(Call(SCI_GETTEXT, 0, 0));
	CStringA sTextA;
	Call(SCI_GETTEXT, len + 1, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(CStrBufA(sTextA, len + 1))));
	return StringFromControl(sTextA);
}

CString CSciEdit::GetWordUnderCursor(bool bSelectWord, bool allchars)
{
	Sci_TextRange textrange;
	auto pos = static_cast<Sci_Position>(Call(SCI_GETCURRENTPOS));
	textrange.chrg.cpMin = static_cast<int>(Call(SCI_WORDSTARTPOSITION, pos, TRUE));
	if ((pos == textrange.chrg.cpMin)||(textrange.chrg.cpMin < 0))
		return CString();
	textrange.chrg.cpMax = static_cast<int>(Call(SCI_WORDENDPOSITION, textrange.chrg.cpMin, TRUE));

	auto textbuffer = std::make_unique<char[]>(textrange.chrg.cpMax - textrange.chrg.cpMin + 1);
	textrange.lpstrText = textbuffer.get();
	Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textrange));
	CString sRet = StringFromControl(textbuffer.get());
	if (m_bDoStyle && !allchars)
	{
		for (const auto styleindicator : { '*', '_', '^' })
		{
			if (sRet.IsEmpty())
				break;
			if (sRet[sRet.GetLength() - 1] == styleindicator)
			{
				--textrange.chrg.cpMax;
				sRet.Truncate(sRet.GetLength() - 1);
			}
			if (sRet.IsEmpty())
				break;
			if (sRet[0] == styleindicator)
			{
				++textrange.chrg.cpMin;
				sRet = sRet.Right(sRet.GetLength() - 1);
			}
		}
	}
	if (bSelectWord)
		Call(SCI_SETSEL, textrange.chrg.cpMin, textrange.chrg.cpMax);
	return sRet;
}

void CSciEdit::SetFont(CString sFontName, int iFontSizeInPoints)
{
	Call(SCI_STYLESETFONT, STYLE_DEFAULT, reinterpret_cast<LPARAM>(CUnicodeUtils::GetUTF8(sFontName).GetBuffer()));
	Call(SCI_STYLESETSIZE, STYLE_DEFAULT, iFontSizeInPoints);
	Call(SCI_STYLECLEARALL);

	LPARAM color = GetSysColor(COLOR_HOTLIGHT);
	// set the styles for the bug ID strings
	Call(SCI_STYLESETBOLD, STYLE_ISSUEBOLD, TRUE);
	Call(SCI_STYLESETFORE, STYLE_ISSUEBOLD, color);
	Call(SCI_STYLESETBOLD, STYLE_ISSUEBOLDITALIC, TRUE);
	Call(SCI_STYLESETITALIC, STYLE_ISSUEBOLDITALIC, TRUE);
	Call(SCI_STYLESETFORE, STYLE_ISSUEBOLDITALIC, color);
	Call(SCI_STYLESETHOTSPOT, STYLE_ISSUEBOLDITALIC, TRUE);

	// set the formatted text styles
	Call(SCI_STYLESETBOLD, STYLE_BOLD, TRUE);
	Call(SCI_STYLESETITALIC, STYLE_ITALIC, TRUE);
	Call(SCI_STYLESETUNDERLINE, STYLE_UNDERLINED, TRUE);

	// set the style for URLs
	Call(SCI_STYLESETFORE, STYLE_URL, color);
	Call(SCI_STYLESETHOTSPOT, STYLE_URL, TRUE);

	Call(SCI_SETHOTSPOTACTIVEUNDERLINE, TRUE);
}

void CSciEdit::SetAutoCompletionList(std::map<CString, int>&& list, TCHAR separator, TCHAR typeSeparator)
{
	//copy the auto completion list.

	//SK: instead of creating a copy of that list, we could accept a pointer
	//to the list and use that instead. But then the caller would have to make
	//sure that the list persists over the lifetime of the control!
	m_autolist.clear();
	m_autolist = std::move(list);
	m_separator = separator;
	m_typeSeparator = typeSeparator;
}

BOOL CSciEdit::IsMisspelled(const CString& sWord)
{
	// convert the string from the control to the encoding of the spell checker module.
	CStringA sWordA = GetWordForSpellChecker(sWord);

	// words starting with a digit are treated as correctly spelled
	if (_istdigit(sWord.GetAt(0)))
		return FALSE;
	// words in the personal dictionary are correct too
	if (m_personalDict.FindWord(sWord))
		return FALSE;

	// Check spell checking cache first.
	const BOOL *cacheResult = m_SpellingCache.try_get(std::wstring(sWord, sWord.GetLength()));
	if (cacheResult)
		return *cacheResult;

	// now we actually check the spelling...
	BOOL misspelled = !pChecker->spell(sWordA);
	if (misspelled)
	{
		// the word is marked as misspelled, we now check whether the word
		// is maybe a composite identifier
		// a composite identifier consists of multiple words, with each word
		// separated by a change in lower to uppercase letters
		misspelled = FALSE;
		if (sWord.GetLength() > 1)
		{
			int wordstart = 0;
			int wordend = 1;
			while (wordend < sWord.GetLength())
			{
				while ((wordend < sWord.GetLength())&&(!_istupper(sWord[wordend])))
					wordend++;
				if ((wordstart == 0)&&(wordend == sWord.GetLength()))
				{
					// words in the auto list are also assumed correctly spelled
					if (m_autolist.find(sWord) != m_autolist.end())
						misspelled = FALSE;
					else
						misspelled = TRUE;
					break;
				}
				sWordA = GetWordForSpellChecker(sWord.Mid(wordstart, wordend - wordstart));
				if ((sWordA.GetLength() > 2) && (!pChecker->spell(sWordA)))
				{
					misspelled = TRUE;
					break;
				}
				wordstart = wordend;
				wordend++;
			}
		}
	}

	// Update cache.
	m_SpellingCache.insert_or_assign(std::wstring(sWord, sWord.GetLength()), misspelled);
	return misspelled;
}

void CSciEdit::CheckSpelling(Sci_Position startpos, Sci_Position endpos)
{
	if (!pChecker)
		return;

	Sci_TextRange textrange;
	textrange.chrg.cpMin = static_cast<Sci_PositionCR>(startpos);
	textrange.chrg.cpMax = textrange.chrg.cpMin;
	auto lastpos = endpos;
	if (lastpos < 0)
		lastpos = static_cast<int>(Call(SCI_GETLENGTH)) - textrange.chrg.cpMin;
	Call(SCI_SETINDICATORCURRENT, INDIC_MISSPELLED);
	while (textrange.chrg.cpMax < lastpos)
	{
		textrange.chrg.cpMin = static_cast<int>(Call(SCI_WORDSTARTPOSITION, textrange.chrg.cpMax + 1, TRUE));
		if (textrange.chrg.cpMin < textrange.chrg.cpMax)
			break;
		textrange.chrg.cpMax = static_cast<int>(Call(SCI_WORDENDPOSITION, textrange.chrg.cpMin, TRUE));
		if (textrange.chrg.cpMin == textrange.chrg.cpMax)
		{
			textrange.chrg.cpMax++;
			// since Scintilla squiggles to the end of the text even if told to stop one char before it,
			// we have to clear here the squiggly lines to the end.
			if (textrange.chrg.cpMin)
				Call(SCI_INDICATORCLEARRANGE, textrange.chrg.cpMin-1, textrange.chrg.cpMax - textrange.chrg.cpMin + 1);
			continue;
		}
		ATLASSERT(textrange.chrg.cpMax >= textrange.chrg.cpMin);
		auto textbuffer = std::make_unique<char[]>(textrange.chrg.cpMax - textrange.chrg.cpMin + 2);
		SecureZeroMemory(textbuffer.get(), textrange.chrg.cpMax - textrange.chrg.cpMin + 2);
		textrange.lpstrText = textbuffer.get();
		textrange.chrg.cpMax++;
		Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textrange));
		auto len = static_cast<int>(strlen(textrange.lpstrText));
		if (len == 0)
		{
			textrange.chrg.cpMax--;
			Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textrange));
			len = static_cast<int>(strlen(textrange.lpstrText));
			textrange.chrg.cpMax++;
			len++;
		}
		if (len && textrange.lpstrText[len - 1] == '.')
		{
			// Try to ignore file names from the auto list.
			// Do do this, for each word ending with '.' we extract next word and check
			// whether the combined string is present in auto list.
			Sci_TextRange twoWords;
			twoWords.chrg.cpMin = textrange.chrg.cpMin;
			twoWords.chrg.cpMax = static_cast<int>(Call(SCI_WORDENDPOSITION, textrange.chrg.cpMax + 1, TRUE));
			auto twoWordsBuffer = std::make_unique<char[]>(twoWords.chrg.cpMax - twoWords.chrg.cpMin + 1);
			twoWords.lpstrText = twoWordsBuffer.get();
			SecureZeroMemory(twoWords.lpstrText, twoWords.chrg.cpMax - twoWords.chrg.cpMin + 1);
			Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&twoWords));
			CString sWord = StringFromControl(twoWords.lpstrText);
			if (m_autolist.find(sWord) != m_autolist.end())
			{
				//mark word as correct (remove the squiggle line)
				Call(SCI_INDICATORCLEARRANGE, twoWords.chrg.cpMin, twoWords.chrg.cpMax - twoWords.chrg.cpMin);
				textrange.chrg.cpMax = twoWords.chrg.cpMax;
				continue;
			}
		}
		if (len)
			textrange.lpstrText[len - 1] = '\0';
		textrange.chrg.cpMax--;
		if (textrange.lpstrText[0])
		{
			CString sWord = StringFromControl(textrange.lpstrText);
			if ((GetStyleAt(textrange.chrg.cpMin) != STYLE_URL) && IsMisspelled(sWord))
			{
				//mark word as misspelled
				Call(SCI_INDICATORFILLRANGE, textrange.chrg.cpMin, textrange.chrg.cpMax - textrange.chrg.cpMin);
			}
			else
			{
				//mark word as correct (remove the squiggle line)
				Call(SCI_INDICATORCLEARRANGE, textrange.chrg.cpMin, textrange.chrg.cpMax - textrange.chrg.cpMin);
				Call(SCI_INDICATORCLEARRANGE, textrange.chrg.cpMin, textrange.chrg.cpMax - textrange.chrg.cpMin + 1);
			}
		}
	}
}

void CSciEdit::SuggestSpellingAlternatives()
{
	if (!pChecker)
		return;
	CString word = GetWordUnderCursor(true);
	Call(SCI_SETCURRENTPOS, Call(SCI_WORDSTARTPOSITION, Call(SCI_GETCURRENTPOS), TRUE));
	if (word.IsEmpty())
		return;
	char ** wlst = nullptr;
	int ns = pChecker->suggest(&wlst, GetWordForSpellChecker(word));
	if (ns > 0)
	{
		CString suggestions;
		for (int i=0; i < ns; i++)
		{
			suggestions.AppendFormat(L"%s%c%d%c", static_cast<LPCTSTR>(GetWordFromSpellChecker(wlst[i])), m_typeSeparator, AUTOCOMPLETE_SPELLING, m_separator);
			free(wlst[i]);
		}
		free(wlst);
		suggestions.TrimRight(m_separator);
		if (suggestions.IsEmpty())
			return;
		Call(SCI_AUTOCSETSEPARATOR, CStringA(m_separator).GetAt(0));
		Call(SCI_AUTOCSETTYPESEPARATOR, m_typeSeparator);
		Call(SCI_AUTOCSETDROPRESTOFWORD, 1);
		Call(SCI_AUTOCSHOW, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(StringForControl(suggestions))));
		return;
	}
	free(wlst);
}

void CSciEdit::DoAutoCompletion(Sci_Position nMinPrefixLength)
{
	if (m_autolist.empty())
		return;
	auto pos = static_cast<int>(static_cast<Sci_Position>(Call(SCI_GETCURRENTPOS)));
	if (pos != static_cast<int>(Call(SCI_WORDENDPOSITION, pos, TRUE)))
		return;	// don't auto complete if we're not at the end of a word
	CString word = GetWordUnderCursor();
	if (word.GetLength() < nMinPrefixLength)
	{
		word = GetWordUnderCursor(false, true);
		if (word.GetLength() < nMinPrefixLength)
			return;		// don't auto complete yet, word is too short
	}
	CString sAutoCompleteList;

	for (int i = 0; i < 2; ++i)
	{
		std::vector<CString> words;

		pos = word.Find('-');

		CString wordLower = word;
		wordLower.MakeLower();
		CString wordHigher = word;
		wordHigher.MakeUpper();

		words.push_back(word);
		words.push_back(wordLower);
		words.push_back(wordHigher);

		if (pos >= 0)
		{
			CString s = wordLower.Left(pos);
			if (s.GetLength() >= nMinPrefixLength)
				words.push_back(s);
			s = wordLower.Mid(pos + 1);
			if (s.GetLength() >= nMinPrefixLength)
				words.push_back(s);
			s = wordHigher.Left(pos);
			if (s.GetLength() >= nMinPrefixLength)
				words.push_back(wordHigher.Left(pos));
			s = wordHigher.Mid(pos + 1);
			if (s.GetLength() >= nMinPrefixLength)
				words.push_back(wordHigher.Mid(pos+1));
		}

		// note: the m_autolist is case-sensitive because
		// its contents are also used to mark words in it
		// as correctly spelled. If it would be case-insensitive,
		// case spelling mistakes would not show up as misspelled.
		std::map<CString, int> wordset;
		for (const auto& w : words)
		{
			for (auto lowerit = m_autolist.lower_bound(w);
				 lowerit != m_autolist.end(); ++lowerit)
			{
				int compare = w.CompareNoCase(lowerit->first.Left(w.GetLength()));
				if (compare > 0)
					continue;
				else if (compare == 0)
					wordset.emplace(lowerit->first, lowerit->second);
				else
					break;
			}
		}

		for (const auto& w : wordset)
			sAutoCompleteList.AppendFormat(L"%s%c%d%c", static_cast<LPCTSTR>(w.first), m_typeSeparator, w.second, m_separator);

		sAutoCompleteList.TrimRight(m_separator);

		if (i == 0)
		{
			if (sAutoCompleteList.IsEmpty())
			{
				// retry with all chars
				word = GetWordUnderCursor(false, true);
			}
			else
				break;
		}
		if (i == 1)
		{
			if (sAutoCompleteList.IsEmpty())
				return;
		}
	}

	Call(SCI_AUTOCSETSEPARATOR, CStringA(m_separator).GetAt(0));
	Call(SCI_AUTOCSETTYPESEPARATOR, (m_typeSeparator));
	auto sForControl = StringForControl(sAutoCompleteList);
	Call(SCI_AUTOCSHOW, StringForControl(word).GetLength(), reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sForControl)));
}

BOOL CSciEdit::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	if (message != WM_NOTIFY)
		return CWnd::OnChildNotify(message, wParam, lParam, pLResult);

	auto lpnmhdr = reinterpret_cast<LPNMHDR>(lParam);
	auto lpSCN = reinterpret_cast<SCNotification*>(lParam);

	if(lpnmhdr->hwndFrom==m_hWnd)
	{
		switch(lpnmhdr->code)
		{
		case SCN_CHARADDED:
			{
				if ((lpSCN->ch < 32)&&(lpSCN->ch != 13)&&(lpSCN->ch != 10))
					Call(SCI_DELETEBACK);
				else
					DoAutoCompletion(m_nAutoCompleteMinChars);
				return TRUE;
			}
			break;
		case SCN_AUTOCSELECTION:
			{
				CString text = StringFromControl(lpSCN->text);
				if (m_autolist[text] == AUTOCOMPLETE_SNIPPET)
				{
					Call(SCI_AUTOCCANCEL);
					for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
					{
						CSciEditContextMenuInterface * pHandler = m_arContextHandlers.GetAt(handlerindex);
						pHandler->HandleSnippet(m_autolist[text], text, this);
					}
				}
				return TRUE;
			}
		case SCN_STYLENEEDED:
		{
			auto startpos = static_cast<Sci_Position>(Call(SCI_GETENDSTYLED));
			auto endpos = reinterpret_cast<SCNotification*>(lpnmhdr)->position;

			auto startwordpos = static_cast<int>(Call(SCI_WORDSTARTPOSITION, startpos, true));
			auto endwordpos = static_cast<int>(Call(SCI_WORDENDPOSITION, endpos, true));

			MarkEnteredBugID(startwordpos, endwordpos);
			if (m_bDoStyle)
				StyleEnteredText(startwordpos, endwordpos);

			StyleURLs(startwordpos, endwordpos);
			CheckSpelling(startwordpos, endwordpos);

			// Tell scintilla editor that we styled all requested range.
			Call(SCI_STARTSTYLING, endwordpos);
			Call(SCI_SETSTYLING, 0, 0);
		}
		break;
		case SCN_MODIFIED:
		{
			if (!m_blockModifiedHandler && (lpSCN->modificationType & (SC_MOD_INSERTTEXT | SC_MOD_DELETETEXT)))
			{
				auto firstline = static_cast<int>(Call(SCI_GETFIRSTVISIBLELINE));
				auto lastline = firstline + static_cast<int>(Call(SCI_LINESONSCREEN));
				auto firstpos = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, firstline));
				auto lastpos = static_cast<Sci_Position>(Call(SCI_GETLINEENDPOSITION, lastline));
				auto pos1 = lpSCN->position;
				auto pos2 = pos1 + lpSCN->length;
				// always use the bigger range
				firstpos = min(firstpos, pos1);
				lastpos = max(lastpos, pos2);

				WrapLines(firstpos, lastpos);
			}
			break;
		}
		case SCN_DWELLSTART:
		case SCN_HOTSPOTRELEASECLICK:
			{
				Sci_TextRange textrange;
				textrange.chrg.cpMin = static_cast<Sci_PositionCR>(lpSCN->position);
				textrange.chrg.cpMax = static_cast<Sci_PositionCR>(lpSCN->position);
				auto style = GetStyleAt(lpSCN->position);
				if (style != STYLE_ISSUEBOLDITALIC && style != STYLE_URL)
					break;
				while (GetStyleAt(textrange.chrg.cpMin - 1) == style)
					--textrange.chrg.cpMin;
				while (GetStyleAt(textrange.chrg.cpMax + 1) == style)
					++textrange.chrg.cpMax;
				++textrange.chrg.cpMax;
				auto textbuffer = std::make_unique<char[]>(textrange.chrg.cpMax - textrange.chrg.cpMin + 1);
				textrange.lpstrText = textbuffer.get();
				Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textrange));
				CString url;
				if (style == STYLE_URL)
				{
					url = StringFromControl(textbuffer.get());
					if (url.Find(L'@') > 0 && !PathIsURL(url))
						url = L"mailto:" + url;
				}
				else
				{
					url = m_sUrl;
					url.Replace(L"%BUGID%", StringFromControl(textbuffer.get()));
				}
				if (!url.IsEmpty())
				{
					if (lpnmhdr->code == SCN_HOTSPOTRELEASECLICK)
						ShellExecute(GetParent()->GetSafeHwnd(), L"open", url, nullptr, nullptr, SW_SHOWDEFAULT);
					else
					{
						CStringA sTextA = StringForControl(url);
						Call(SCI_CALLTIPSHOW, lpSCN->position + 3, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));
					}
				}
			}
			break;
		case SCN_DWELLEND:
			Call(SCI_CALLTIPCANCEL);
			break;
		}
	}
	return CWnd::OnChildNotify(message, wParam, lParam, pLResult);
}

BEGIN_MESSAGE_MAP(CSciEdit, CWnd)
	ON_WM_KEYDOWN()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()

void CSciEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	switch (nChar)
	{
	case (VK_ESCAPE):
		{
			if ((Call(SCI_AUTOCACTIVE)==0)&&(Call(SCI_CALLTIPACTIVE)==0))
				::SendMessage(GetParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
		}
		break;
	}
	CWnd::OnKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CSciEdit::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case VK_SPACE:
			{
				if ((GetKeyState(VK_CONTROL) & 0x8000) && ((GetKeyState(VK_MENU) & 0x8000) == 0))
				{
					DoAutoCompletion(1);
					return TRUE;
				}
			}
			break;
		case VK_TAB:
			// The TAB cannot be handled in OnKeyDown because it is too late by then.
			{
				if ((GetKeyState(VK_CONTROL) & 0x8000) && ((GetKeyState(VK_MENU) & 0x8000) == 0))
				{
					//Ctrl-Tab was pressed, this means we should provide the user with
					//a list of possible spell checking alternatives to the word under
					//the cursor
					SuggestSpellingAlternatives();
					return TRUE;
				}
				else if (!Call(SCI_AUTOCACTIVE))
				{
					::PostMessage(GetParent()->GetSafeHwnd(), WM_NEXTDLGCTL, GetKeyState(VK_SHIFT)&0x8000, 0);
					return TRUE;
				}
			}
			break;
		}
	}
	return CWnd::PreTranslateMessage(pMsg);
}

void CSciEdit::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	auto anchor = static_cast<Sci_Position>(Call(SCI_GETANCHOR));
	auto currentpos = static_cast<Sci_Position>(Call(SCI_GETCURRENTPOS));
	auto selstart = static_cast<int>(static_cast<Sci_Position>(Call(SCI_GETSELECTIONSTART)));
	auto selend = static_cast<int>(static_cast<Sci_Position>(Call(SCI_GETSELECTIONEND)));
	Sci_Position pointpos = 0;
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		GetClientRect(&rect);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
		pointpos = static_cast<Sci_Position>(Call(SCI_GETCURRENTPOS));
	}
	else
	{
		// change the cursor position to the point where the user
		// right-clicked.
		CPoint clientpoint = point;
		ScreenToClient(&clientpoint);
		pointpos = static_cast<Sci_Position>(Call(SCI_POSITIONFROMPOINT, clientpoint.x, clientpoint.y));
	}
	CString sMenuItemText;
	CMenu popup;
	bool bRestoreCursor = true;
	if (popup.CreatePopupMenu())
	{
		bool bCanUndo = !!Call(SCI_CANUNDO);
		bool bCanRedo = !!Call(SCI_CANREDO);
		bool bHasSelection = (selend-selstart > 0);
		bool bCanPaste = !!Call(SCI_CANPASTE);
		bool bIsReadOnly = !!Call(SCI_GETREADONLY);
		UINT uEnabledMenu = MF_STRING | MF_ENABLED;
		UINT uDisabledMenu = MF_STRING | MF_GRAYED;

		// find the word under the cursor
		CString sWord;
		if (pointpos)
		{
			// setting the cursor clears the selection
			Call(SCI_SETANCHOR, pointpos);
			Call(SCI_SETCURRENTPOS, pointpos);
			sWord = GetWordUnderCursor();
			// restore the selection
			Call(SCI_SETSELECTIONSTART, selstart);
			Call(SCI_SETSELECTIONEND, selend);
		}
		else
			sWord = GetWordUnderCursor();
		CStringA worda = GetWordForSpellChecker(sWord);

		int nCorrections = 1;
		bool bSpellAdded = false;
		// check if the word under the cursor is spelled wrong
		if ((pChecker)&&(!worda.IsEmpty()) && !bIsReadOnly)
		{
			char ** wlst = nullptr;
			// get the spell suggestions
			int ns = pChecker->suggest(&wlst,worda);
			if (ns > 0)
			{
				// add the suggestions to the context menu
				for (int i=0; i < ns; i++)
				{
					bSpellAdded = true;
					CString sug = GetWordFromSpellChecker(wlst[i]);
					popup.InsertMenu(static_cast<UINT>(-1), 0, nCorrections++, sug);
					free(wlst[i]);
				}
				free(wlst);
			}
			else
				free(wlst);
		}
		// only add a separator if spelling correction suggestions were added
		if (bSpellAdded)
			popup.AppendMenu(MF_SEPARATOR);

		// also allow the user to add the word to the custom dictionary so
		// it won't show up as misspelled anymore
		if ((sWord.GetLength()<PDICT_MAX_WORD_LENGTH)&&((pChecker)&&(m_autolist.find(sWord) == m_autolist.end())&&(!pChecker->spell(worda)))&&
			(!_istdigit(sWord.GetAt(0)))&&(!m_personalDict.FindWord(sWord)) && !bIsReadOnly)
		{
			sMenuItemText.Format(IDS_SCIEDIT_ADDWORD, static_cast<LPCTSTR>(sWord));
			popup.AppendMenu(uEnabledMenu, SCI_ADDWORD, sMenuItemText);
			// another separator
			popup.AppendMenu(MF_SEPARATOR);
		}

		// add the 'default' entries
		sMenuItemText.LoadString(IDS_SCIEDIT_UNDO);
		popup.AppendMenu(bCanUndo ? uEnabledMenu : uDisabledMenu, SCI_UNDO, sMenuItemText);
		sMenuItemText.LoadString(IDS_SCIEDIT_REDO);
		popup.AppendMenu(bCanRedo ? uEnabledMenu : uDisabledMenu, SCI_REDO, sMenuItemText);

		popup.AppendMenu(MF_SEPARATOR);

		sMenuItemText.LoadString(IDS_SCIEDIT_CUT);
		popup.AppendMenu(!bIsReadOnly && bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_CUT, sMenuItemText);
		sMenuItemText.LoadString(IDS_SCIEDIT_COPY);
		popup.AppendMenu(bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_COPY, sMenuItemText);
		sMenuItemText.LoadString(IDS_SCIEDIT_PASTE);
		popup.AppendMenu(bCanPaste ? uEnabledMenu : uDisabledMenu, SCI_PASTE, sMenuItemText);

		popup.AppendMenu(MF_SEPARATOR);

		sMenuItemText.LoadString(IDS_SCIEDIT_SELECTALL);
		popup.AppendMenu(uEnabledMenu, SCI_SELECTALL, sMenuItemText);

		if (!bIsReadOnly && Call(SCI_GETEDGECOLUMN))
		{
			popup.AppendMenu(MF_SEPARATOR);

			sMenuItemText.LoadString(IDS_SCIEDIT_SPLITLINES);
			popup.AppendMenu(bHasSelection ? uEnabledMenu : uDisabledMenu, SCI_LINESSPLIT, sMenuItemText);
		}

		if (m_arContextHandlers.GetCount() > 0)
			popup.AppendMenu(MF_SEPARATOR);

		int nCustoms = nCorrections;
		// now add any custom context menus
		for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
		{
			CSciEditContextMenuInterface * pHandler = m_arContextHandlers.GetAt(handlerindex);
			pHandler->InsertMenuItems(popup, nCustoms);
		}
#if THESAURUS
		// add found thesauri to sub menu's
		CMenu thesaurs;
		int nThesaurs = 0;
		CPtrArray menuArray;
		if (thesaurs.CreatePopupMenu())
		{
			if ((nCustoms > nCorrections || m_arContextHandlers.IsEmpty()) && !bIsReadOnly)
				popup.AppendMenu(MF_SEPARATOR);
			if (pThesaur && !worda.IsEmpty() && !bIsReadOnly)
			{
				mentry * pmean;
				worda.MakeLower();
				int count = pThesaur->Lookup(worda, worda.GetLength(),&pmean);
				if (count)
				{
					mentry * pm = pmean;
					for (int  i=0; i < count; i++)
					{
						CMenu * submenu = new CMenu();
						menuArray.Add(submenu);
						submenu->CreateMenu();
						for (int j=0; j < pm->count; j++)
						{
							CString sug = CString(pm->psyns[j]);
							submenu->InsertMenu(UINT(-1), 0, nCorrections + nCustoms + (nThesaurs++), sug);
						}
						thesaurs.InsertMenu(UINT(-1), MF_POPUP, reinterpret_cast<UINT_PTR>(submenu->m_hMenu), CString(pm->defn));
						pm++;
					}
				}
				if ((count > 0)&&(point.x >= 0))
				{
#ifdef IDS_SPELLEDIT_THESAURUS
					sMenuItemText.LoadString(IDS_SPELLEDIT_THESAURUS);
					popup.InsertMenu(UINT(-1), MF_POPUP, reinterpret_cast<UINT_PTR>(thesaurs.m_hMenu), sMenuItemText);
#else
					popup.InsertMenu(UINT(-1), MF_POPUP, reinterpret_cast<UINT_PTR>(thesaurs.m_hMenu), L"Thesaurus");
#endif
					nThesaurs = nCustoms;
				}
				else
				{
					sMenuItemText.LoadString(IDS_SPELLEDIT_NOTHESAURUS);
					popup.AppendMenu(MF_DISABLED | MF_GRAYED | MF_STRING, 0, sMenuItemText);
				}

				pThesaur->CleanUpAfterLookup(&pmean, count);
			}
			else if (!bIsReadOnly)
			{
				sMenuItemText.LoadString(IDS_SPELLEDIT_NOTHESAURUS);
				popup.AppendMenu(MF_DISABLED | MF_GRAYED | MF_STRING, 0, sMenuItemText);
			}
		}
#endif
		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
		switch (cmd)
		{
		case 0:
			break;	// no command selected
		case SCI_SELECTALL:
			bRestoreCursor = false;
			// fall through
		case SCI_UNDO:
		case SCI_REDO:
		case SCI_CUT:
		case SCI_COPY:
		case SCI_PASTE:
			Call(cmd);
			break;
		case SCI_ADDWORD:
			m_personalDict.AddWord(sWord);
			CheckSpelling(static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, static_cast<int>(Call(SCI_GETFIRSTVISIBLELINE)))), static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, static_cast<int>(Call(SCI_GETFIRSTVISIBLELINE)) + static_cast<int>(Call(SCI_LINESONSCREEN)))));
			break;
		case SCI_LINESSPLIT:
			{
				auto marker = static_cast<int>(Call(SCI_GETEDGECOLUMN) * static_cast<int>(Call(SCI_TEXTWIDTH, 0, reinterpret_cast<LPARAM>(" "))));
				if (marker)
				{
					m_blockModifiedHandler = true;
					SCOPE_EXIT{ m_blockModifiedHandler = false; };
					Call(SCI_TARGETFROMSELECTION);
					Call(SCI_LINESJOIN);
					Call(SCI_LINESSPLIT, marker);
				}
			}
			break;
		default:
			if (cmd < nCorrections)
			{
				Call(SCI_SETANCHOR, pointpos);
				Call(SCI_SETCURRENTPOS, pointpos);
				GetWordUnderCursor(true);
				CString temp;
				popup.GetMenuString(cmd, temp, 0);
				// setting the cursor clears the selection
				Call(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(StringForControl(temp))));
			}
			else if (cmd < (nCorrections+nCustoms))
			{
				for (INT_PTR handlerindex = 0; handlerindex < m_arContextHandlers.GetCount(); ++handlerindex)
				{
					CSciEditContextMenuInterface * pHandler = m_arContextHandlers.GetAt(handlerindex);
					if (pHandler->HandleMenuItemClick(cmd, this))
						break;
				}
			}
#if THESAURUS
			else if (cmd <= (nThesaurs+nCorrections+nCustoms))
			{
				Call(SCI_SETANCHOR, pointpos);
				Call(SCI_SETCURRENTPOS, pointpos);
				GetWordUnderCursor(true);
				CString temp;
				thesaurs.GetMenuString(cmd, temp, 0);
				Call(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(StringForControl(temp))));
			}
#endif
		}
#ifdef THESAURUS
		for (INT_PTR index = 0; index < menuArray.GetCount(); ++index)
		{
			auto pMenu = static_cast<CMenu*>(menuArray[index]);
			delete pMenu;
		}
#endif
	}
	if (bRestoreCursor)
	{
		// restore the anchor and cursor position
		Call(SCI_SETCURRENTPOS, currentpos);
		Call(SCI_SETANCHOR, anchor);
	}
}

bool CSciEdit::StyleEnteredText(Sci_Position startstylepos, Sci_Position endstylepos)
{
	bool bStyled = false;
	const auto line = static_cast<int>(Call(SCI_LINEFROMPOSITION, startstylepos));
	const auto line_number_end = static_cast<int>(Call(SCI_LINEFROMPOSITION, endstylepos));
	for (auto line_number = line; line_number <= line_number_end; ++line_number)
	{
		auto offset = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, line_number));
		auto line_len = static_cast<int>(Call(SCI_LINELENGTH, line_number));
		auto linebuffer = std::make_unique<char[]>(line_len + 1);
		Call(SCI_GETLINE, line_number, reinterpret_cast<LPARAM>(linebuffer.get()));
		linebuffer[line_len] = '\0';
		Sci_Position start = 0;
		Sci_Position end = 0;
		while (FindStyleChars(linebuffer.get(), '*', start, end))
		{
			Call(SCI_STARTSTYLING, start + offset, STYLE_BOLD);
			Call(SCI_SETSTYLING, end-start, STYLE_BOLD);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(linebuffer.get(), '^', start, end))
		{
			Call(SCI_STARTSTYLING, start + offset, STYLE_ITALIC);
			Call(SCI_SETSTYLING, end-start, STYLE_ITALIC);
			bStyled = true;
			start = end;
		}
		start = 0;
		end = 0;
		while (FindStyleChars(linebuffer.get(), '_', start, end))
		{
			Call(SCI_STARTSTYLING, start + offset, STYLE_UNDERLINED);
			Call(SCI_SETSTYLING, end-start, STYLE_UNDERLINED);
			bStyled = true;
			start = end;
		}
	}
	return bStyled;
}

bool CSciEdit::WrapLines(Sci_Position startpos, Sci_Position endpos)
{
	auto markerX = static_cast<Sci_Position>(Call(SCI_GETEDGECOLUMN) * static_cast<int>(Call(SCI_TEXTWIDTH, 0, reinterpret_cast<LPARAM>(" "))));
	if (markerX)
	{
		Call(SCI_SETTARGETSTART, startpos);
		Call(SCI_SETTARGETEND, endpos);
		Call(SCI_LINESSPLIT, markerX);
		return true;
	}
	return false;
}

void CSciEdit::AdvanceUTF8(const char * str, int& pos)
{
	if ((str[pos] & 0xE0)==0xC0)
	{
		// utf8 2-byte sequence
		pos += 2;
	}
	else if ((str[pos] & 0xF0)==0xE0)
	{
		// utf8 3-byte sequence
		pos += 3;
	}
	else if ((str[pos] & 0xF8)==0xF0)
	{
		// utf8 4-byte sequence
		pos += 4;
	}
	else
		pos++;
}

bool CSciEdit::FindStyleChars(const char* line, char styler, Sci_Position& start, Sci_Position& end)
{
	int i=0;
	int u=0;
	while (i < start)
	{
		AdvanceUTF8(line, i);
		u++;
	}

	bool bFoundMarker = false;
	CString sULine = CUnicodeUtils::GetUnicode(line);
	// find a starting marker
	while (line[i] != 0)
	{
		if (line[i] == styler)
		{
			if ((line[i+1]!=0)&&(IsCharAlphaNumeric(sULine[u+1]))&&
				(((u>0)&&(!IsCharAlphaNumeric(sULine[u-1]))) || (u==0)))
			{
				start = i+1;
				AdvanceUTF8(line, i);
				u++;
				bFoundMarker = true;
				break;
			}
		}
		AdvanceUTF8(line, i);
		u++;
	}
	if (!bFoundMarker)
		return false;
	// find ending marker
	bFoundMarker = false;
	while (line[i])
	{
		if (line[i] == styler)
		{
			if ((IsCharAlphaNumeric(sULine[u-1]))&&
				((((u+1)<sULine.GetLength())&&(!IsCharAlphaNumeric(sULine[u+1]))) || ((u+1) == sULine.GetLength()))
				)
			{
				end = i;
				i++;
				bFoundMarker = true;
				break;
			}
		}
		AdvanceUTF8(line, i);
		u++;
	}
	return bFoundMarker;
}

BOOL CSciEdit::MarkEnteredBugID(Sci_Position startstylepos, Sci_Position endstylepos)
{
	if (m_sCommand.IsEmpty())
		return FALSE;
	// get the text between the start and end position we have to style
	const auto line_number = static_cast<int>(Call(SCI_LINEFROMPOSITION, startstylepos));
	auto start_pos = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, line_number));
	auto end_pos = endstylepos;

	if (start_pos == end_pos)
		return FALSE;
	if (start_pos > end_pos)
	{
		auto switchtemp = start_pos;
		start_pos = end_pos;
		end_pos = switchtemp;
	}

	auto textbuffer = std::make_unique<char[]>(end_pos - start_pos + 2);
	Sci_TextRange textrange;
	textrange.lpstrText = textbuffer.get();
	textrange.chrg.cpMin = static_cast<Sci_PositionCR>(start_pos);
	textrange.chrg.cpMax = static_cast<Sci_PositionCR>(end_pos);
	Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textrange));
	CStringA msg = CStringA(textbuffer.get());

	Call(SCI_STARTSTYLING, start_pos, STYLE_MASK);

	try
	{
		if (!m_sBugID.IsEmpty())
		{
			// match with two regex strings (without grouping!)
			const std::regex regCheck(m_sCommand);
			const std::regex regBugID(m_sBugID);
			const std::sregex_iterator end;
			std::string s = msg;
			LONG pos = 0;
			// note:
			// if start_pos is 0, we're styling from the beginning and let the ^ char match the beginning of the line
			// that way, the ^ matches the very beginning of the log message and not the beginning of further lines.
			// problem is: this only works *while* entering log messages. If a log message is pasted in whole or
			// multiple lines are pasted, start_pos can be 0 and styling goes over multiple lines. In that case, those
			// additional line starts also match ^
			for (std::sregex_iterator it(s.cbegin(), s.cend(), regCheck, start_pos != 0 ? std::regex_constants::match_not_bol : std::regex_constants::match_default); it != end; ++it)
			{
				// clear the styles up to the match position
				Call(SCI_SETSTYLING, it->position(0)-pos, STYLE_DEFAULT);

				// (*it)[0] is the matched string
				std::string matchedString = (*it)[0];
				LONG matchedpos = 0;
				for (std::sregex_iterator it2(matchedString.cbegin(), matchedString.cend(), regBugID); it2 != end; ++it2)
				{
					ATLTRACE("matched id : %s\n", std::string((*it2)[0]).c_str());

					// bold style up to the id match
					ATLTRACE("position = %ld\n", it2->position(0));
					if (it2->position(0))
						Call(SCI_SETSTYLING, it2->position(0) - matchedpos, STYLE_ISSUEBOLD);
					// bold and recursive style for the bug ID itself
					if ((*it2)[0].str().size())
						Call(SCI_SETSTYLING, (*it2)[0].str().size(), STYLE_ISSUEBOLDITALIC);
					matchedpos = static_cast<LONG>(it2->position(0) + (*it2)[0].str().size());
				}
				if (matchedpos&& matchedpos < static_cast<LONG>(matchedString.size()))
				{
					Call(SCI_SETSTYLING, matchedString.size() - matchedpos, STYLE_ISSUEBOLD);
				}
				pos = static_cast<LONG>(it->position(0) + matchedString.size());
			}
			// bold style for the rest of the string which isn't matched
			if (s.size()-pos)
				Call(SCI_SETSTYLING, s.size()-pos, STYLE_DEFAULT);
		}
		else
		{
			const std::regex regCheck(m_sCommand);
			const std::sregex_iterator end;
			std::string s = msg;
			LONG pos = 0;
			for (std::sregex_iterator it(s.cbegin(), s.cend(), regCheck); it != end; ++it)
			{
				// clear the styles up to the match position
				if (it->position(0) - pos >= 0)
					Call(SCI_SETSTYLING, it->position(0) - pos, STYLE_DEFAULT);
				pos = static_cast<LONG>(it->position(0));

				const std::smatch match = *it;
				// we define group 1 as the whole issue text and
				// group 2 as the bug ID
				if (match.size() >= 2)
				{
					ATLTRACE("matched id : %s\n", std::string(match[1]).c_str());
					if (match[1].first - s.cbegin() - pos >= 0)
						Call(SCI_SETSTYLING, match[1].first - s.cbegin() - pos, STYLE_ISSUEBOLD);
					Call(SCI_SETSTYLING, std::string(match[1]).size(), STYLE_ISSUEBOLDITALIC);
					pos = static_cast<LONG>(match[1].second - s.cbegin());
				}
			}
		}
	}
	catch (std::exception&) {}

	return FALSE;
}

//similar code in AppUtils.cpp
bool CSciEdit::IsValidURLChar(unsigned char ch)
{
	return isalnum(ch) ||
		ch == '_' || ch == '/' || ch == ';' || ch == '?' || ch == '&' || ch == '=' ||
		ch == '%' || ch == ':' || ch == '.' || ch == '#' || ch == '-' || ch == '+' ||
		ch == '|' || ch == '>' || ch == '<' || ch == '!' || ch == '@' || ch == '~';
}

//similar code in AppUtils.cpp
void CSciEdit::StyleURLs(Sci_Position startstylepos, Sci_Position endstylepos)
{
	const auto line_number = static_cast<int>(Call(SCI_LINEFROMPOSITION, startstylepos));
	startstylepos = static_cast<Sci_Position>(Call(SCI_POSITIONFROMLINE, line_number));

	auto len = endstylepos - startstylepos + 1;
	auto textbuffer = std::make_unique<char[]>(len + 1);
	Sci_TextRange textrange;
	textrange.lpstrText = textbuffer.get();
	textrange.chrg.cpMin = static_cast<Sci_PositionCR>(startstylepos);
	textrange.chrg.cpMax = static_cast<Sci_PositionCR>(endstylepos);
	Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&textrange));
	// we're dealing with utf8 encoded text here, which means one glyph is
	// not necessarily one byte/wchar_t
	// that's why we use CStringA to still get a correct char index
	CStringA msg = textbuffer.get();

	int starturl = -1;
	for (int i = 0; i <= msg.GetLength(); AdvanceUTF8(msg, i))
	{
		if ((i < len) && IsValidURLChar(msg[i]))
		{
			if (starturl < 0)
				starturl = i;
		}
		else
		{
			if (starturl >= 0)
			{
				bool strip = true;
				if (msg[starturl] == '<' && i < len) // try to detect and do not strip URLs put within <>
				{
					while (starturl <= i && msg[starturl] == '<') // strip leading '<'
						++starturl;
					strip = false;
					i = starturl;
					while (i < len && msg[i] != '\r' && msg[i] != '\n' && msg[i] != '>') // find first '>' or new line after resetting i to start position
						AdvanceUTF8(msg, i);
				}

				int skipTrailing = 0;
				while (strip && i - skipTrailing - 1 > starturl && (msg[i - skipTrailing - 1] == '.' || msg[i - skipTrailing - 1] == '-' || msg[i - skipTrailing - 1] == '?' || msg[i - skipTrailing - 1] == ';' || msg[i - skipTrailing - 1] == ':' || msg[i - skipTrailing - 1] == '>' || msg[i - skipTrailing - 1] == '<' || msg[i - skipTrailing - 1] == '!'))
					++skipTrailing;

				if (!IsUrlOrEmail(msg.Mid(starturl, i - starturl - skipTrailing)))
				{
					starturl = -1;
					continue;
				}

				ASSERT(startstylepos + i - skipTrailing <= endstylepos);
				Call(SCI_STARTSTYLING, startstylepos + starturl, STYLE_URL);
				Call(SCI_SETSTYLING, i - starturl - skipTrailing, STYLE_URL);
			}
			starturl = -1;
		}
	}
}

bool CSciEdit::IsUrlOrEmail(const CStringA& sText)
{
	if (!PathIsURLA(sText))
	{
		auto atpos = sText.Find('@');
		if (atpos <= 0)
			return false;
		if (sText.Find('.', atpos) <= atpos + 1) // a dot must follow after the @, but not directly after it
			return false;
		if (sText.Find(':', atpos) < 0) // do not detect git@example.com:something as an email address
			return true;
		return false;
	}
	for (const CStringA& prefix : { "http://", "https://", "git://", "ftp://", "file://", "mailto:" })
	{
		if (strncmp(sText, prefix, prefix.GetLength()) == 0 && sText.GetLength() != prefix.GetLength())
			return true;
	}
	return false;
}

CStringA CSciEdit::GetWordForSpellChecker(const CString& sWord)
{
	// convert the string from the control to the encoding of the spell checker module.
	CStringA sWordA;
	if (m_spellcodepage)
	{
		char * buf = sWordA.GetBuffer(sWord.GetLength() * 4 + 1);
		int lengthIncTerminator = WideCharToMultiByte(m_spellcodepage, 0, sWord, -1, buf, sWord.GetLength() * 4, nullptr, nullptr);
		if (lengthIncTerminator == 0)
			return ""; // converting to the codepage failed
		sWordA.ReleaseBuffer(lengthIncTerminator - 1);
	}
	else
		sWordA = CStringA(sWord);

	sWordA.Trim("\'\".,");

	if (m_bDoStyle)
	{
		for (const auto styleindicator : { '*', '_', '^' })
		{
			if (sWordA.IsEmpty())
				break;
			if (sWordA[sWordA.GetLength() - 1] == styleindicator)
				sWordA.Truncate(sWordA.GetLength() - 1);
			if (sWordA.IsEmpty())
				break;
			if (sWordA[0] == styleindicator)
				sWordA = sWordA.Right(sWordA.GetLength() - 1);
		}
	}

	return sWordA;
}

CString CSciEdit::GetWordFromSpellChecker(const CStringA& sWordA)
{
	CString sWord;
	if (m_spellcodepage)
	{
		wchar_t * buf = sWord.GetBuffer(sWordA.GetLength() * 2);
		int lengthIncTerminator = MultiByteToWideChar(m_spellcodepage, 0, sWordA, -1, buf, sWordA.GetLength() * 2);
		if (lengthIncTerminator == 0)
			return L"";
		sWord.ReleaseBuffer(lengthIncTerminator - 1);
	}
	else
		sWord = CString(sWordA);

	sWord.Trim(L"\'\".,");

	return sWord;
}

bool CSciEdit::IsUTF8(LPVOID pBuffer, size_t cb)
{
	if (cb < 2)
		return true;
	auto pVal = static_cast<UINT16*>(pBuffer);
	auto pVal2 = reinterpret_cast<UINT8*>(pVal + 1);
	// scan the whole buffer for a 0x0000 sequence
	// if found, we assume a binary file
	for (size_t i=0; i<(cb-2); i=i+2)
	{
		if (0x0000 == *pVal++)
			return false;
	}
	pVal = static_cast<UINT16*>(pBuffer);
	if (*pVal == 0xFEFF)
		return false;
	if (cb < 3)
		return false;
	if (*pVal == 0xBBEF)
	{
		if (*pVal2 == 0xBF)
			return true;
	}
	// check for illegal UTF8 chars
	pVal2 = static_cast<UINT8*>(pBuffer);
	for (size_t i=0; i<cb; ++i)
	{
		if ((*pVal2 == 0xC0)||(*pVal2 == 0xC1)||(*pVal2 >= 0xF5))
			return false;
		pVal2++;
	}
	pVal2 = static_cast<UINT8*>(pBuffer);
	bool bUTF8 = false;
	for (size_t i=0; i<(cb-3); ++i)
	{
		if ((*pVal2 & 0xE0)==0xC0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			bUTF8 = true;
		}
		if ((*pVal2 & 0xF0)==0xE0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			bUTF8 = true;
		}
		if ((*pVal2 & 0xF8)==0xF0)
		{
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			pVal2++;i++;
			if ((*pVal2 & 0xC0)!=0x80)
				return false;
			bUTF8 = true;
		}
		pVal2++;
	}
	if (bUTF8)
		return true;
	return false;
}

void CSciEdit::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face)
{
	Call(SCI_STYLESETFORE, style, fore);
	Call(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		Call(SCI_STYLESETSIZE, style, size);
	if (face)
		Call(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

void CSciEdit::SetUDiffStyle()
{
	m_bDoStyle = false;
	SetAStyle(STYLE_DEFAULT, ::GetSysColor(COLOR_WINDOWTEXT), ::GetSysColor(COLOR_WINDOW),
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffFontSize", 10),
		CUnicodeUtils::StdGetUTF8(CRegStdString(L"Software\\TortoiseGit\\UDiffFontName", L"Consolas")).c_str());
	Call(SCI_SETTABWIDTH, CRegStdDWORD(L"Software\\TortoiseGit\\UDiffTabSize", 4));

	Call(SCI_SETREADONLY, TRUE);
	//LRESULT pix = Call(SCI_TEXTWIDTH, STYLE_LINENUMBER, reinterpret_cast<LPARAM>("_99999"));
	//Call(SCI_SETMARGINWIDTHN, 0, pix);
	//Call(SCI_SETMARGINWIDTHN, 1);
	//Call(SCI_SETMARGINWIDTHN, 2);
	//Set the default windows colors for edit controls
	SetColors(false);

	//SendEditor(SCI_SETREADONLY, FALSE);
	Call(SCI_CLEARALL);
	Call(EM_EMPTYUNDOBUFFER);
	Call(SCI_SETSAVEPOINT);
	Call(SCI_CANCEL);
	Call(SCI_SETUNDOCOLLECTION, 0);

	Call(SCI_SETUNDOCOLLECTION, 1);
	Call(SCI_SETWRAPMODE,SC_WRAP_NONE);

	//::SetFocus(m_hWndEdit);
	Call(EM_EMPTYUNDOBUFFER);
	Call(SCI_SETSAVEPOINT);
	Call(SCI_GOTOPOS, 0);

	Call(SCI_SETVIEWWS, 1);
	Call(SCI_SETWHITESPACESIZE, 2);
	Call(SCI_SETWHITESPACEFORE, true, ::GetSysColor(COLOR_3DSHADOW));
	Call(SCI_STYLESETVISIBLE, STYLE_CONTROLCHAR, TRUE);

	Call(SCI_CLEARDOCUMENTSTYLE, 0, 0);

	HIGHCONTRAST highContrast = { 0 };
	highContrast.cbSize = sizeof(HIGHCONTRAST);
	if (SystemParametersInfo(SPI_GETHIGHCONTRAST, 0, &highContrast, 0) == TRUE && (highContrast.dwFlags & HCF_HIGHCONTRASTON))
	{
		Call(SCI_SETLEXER, SCLEX_NULL);
		return;
	}

	//SetAStyle(SCE_DIFF_DEFAULT, RGB(0, 0, 0));
	SetAStyle(SCE_DIFF_COMMAND,
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffForeCommandColor", UDIFF_COLORFORECOMMAND),
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffBackCommandColor", UDIFF_COLORBACKCOMMAND));
	SetAStyle(SCE_DIFF_POSITION,
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffForePositionColor", UDIFF_COLORFOREPOSITION),
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffBackPositionColor", UDIFF_COLORBACKPOSITION));
	SetAStyle(SCE_DIFF_HEADER,
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffForeHeaderColor", UDIFF_COLORFOREHEADER),
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffBackHeaderColor", UDIFF_COLORBACKHEADER));
	SetAStyle(SCE_DIFF_COMMENT,
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffForeCommentColor", UDIFF_COLORFORECOMMENT),
		CRegStdDWORD(L"Software\\TortoiseGit\\UDiffBackCommentColor", UDIFF_COLORBACKCOMMENT));
	Call(SCI_STYLESETBOLD, SCE_DIFF_COMMENT, TRUE);
	for (int style : { SCE_DIFF_ADDED, SCE_DIFF_PATCH_ADD, SCE_DIFF_PATCH_DELETE })
	{
		SetAStyle(style,
			CRegStdDWORD(L"Software\\TortoiseGit\\UDiffForeAddedColor", UDIFF_COLORFOREADDED),
			CRegStdDWORD(L"Software\\TortoiseGit\\UDiffBackAddedColor", UDIFF_COLORBACKADDED));
	}
	for (int style : { SCE_DIFF_DELETED, SCE_DIFF_REMOVED_PATCH_ADD, SCE_DIFF_REMOVED_PATCH_DELETE })
	{
		SetAStyle(style,
			CRegStdDWORD(L"Software\\TortoiseGit\\UDiffForeRemovedColor", UDIFF_COLORFOREREMOVED),
			CRegStdDWORD(L"Software\\TortoiseGit\\UDiffBackRemovedColor", UDIFF_COLORBACKREMOVED));
	}

	Call(SCI_SETLEXER, SCLEX_DIFF);
	Call(SCI_SETKEYWORDS, 0, reinterpret_cast<LPARAM>("revision"));
	Call(SCI_COLOURISE, 0, -1);
}

int CSciEdit::LoadFromFile(CString &filename)
{
	CAutoFILE fp = _wfsopen(filename, L"rb", _SH_DENYWR);
	if (!fp)
		return -1;

	char data[4096] = { 0 };
	size_t lenFile = fread(data, 1, sizeof(data), fp);
	bool bUTF8 = IsUTF8(data, lenFile);
	while (lenFile > 0)
	{
		Call(SCI_ADDTEXT, lenFile, reinterpret_cast<LPARAM>(static_cast<char *>(data)));
		lenFile = fread(data, 1, sizeof(data), fp);
	}
	Call(SCI_SETCODEPAGE, bUTF8 ? SC_CP_UTF8 : GetACP());
	return 0;
}

void CSciEdit::RestyleBugIDs()
{
	auto endstylepos = static_cast<int>(Call(SCI_GETLENGTH));
	// clear all styles
	Call(SCI_STARTSTYLING, 0, STYLE_MASK);
	Call(SCI_SETSTYLING, endstylepos, STYLE_DEFAULT);
	// style the bug IDs
	MarkEnteredBugID(0, endstylepos);
}

ULONG CSciEdit::GetGestureStatus(CPoint /*ptTouch*/)
{
	return 0;
}
