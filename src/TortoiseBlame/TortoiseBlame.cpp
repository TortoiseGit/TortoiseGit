// TortoiseBlame - a Viewer for Subversion Blames

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

#include "stdafx.h"
#include "CmdLineParser.h"
#include "TortoiseBlame.h"
#include "registry.h"
#include "LangDll.h"

#define MAX_LOADSTRING 1000

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma warning(push)
#pragma warning(disable:4127)		// conditional expression is constant

// Global Variables:
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
TCHAR szViewtitle[MAX_PATH];
TCHAR szOrigPath[MAX_PATH];
TCHAR searchstringnotfound[MAX_LOADSTRING];

const bool ShowDate = false;
const bool ShowAuthor = true;
const bool ShowLine = true;
bool ShowPath = false;

static TortoiseBlame app;
long TortoiseBlame::m_gotoline = 0;

TortoiseBlame::TortoiseBlame()
{
	hInstance = 0;
	hResource = 0;
	currentDialog = 0;
	wMain = 0;
	wEditor = 0;
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
}

TortoiseBlame::~TortoiseBlame()
{
	if (m_font)
		DeleteObject(m_font);
	if (m_italicfont)
		DeleteObject(m_italicfont);
}

std::string TortoiseBlame::GetAppDirectory()
{
	std::string path;
	DWORD len = 0;
	DWORD bufferlen = MAX_PATH;		// MAX_PATH is not the limit here!
	do
	{
		bufferlen += MAX_PATH;		// MAX_PATH is not the limit here!
		TCHAR * pBuf = new TCHAR[bufferlen];
		len = GetModuleFileName(NULL, pBuf, bufferlen);
		path = std::string(pBuf, len);
		delete [] pBuf;
	} while(len == bufferlen);
	path = path.substr(0, path.rfind('\\') + 1);

	return path;
}

// Return a color which is interpolated between c1 and c2.
// Slider controls the relative proportions as a percentage:
// Slider = 0 	represents pure c1
// Slider = 50	represents equal mixture
// Slider = 100	represents pure c2
COLORREF TortoiseBlame::InterColor(COLORREF c1, COLORREF c2, int Slider)
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

LRESULT TortoiseBlame::SendEditor(UINT Msg, WPARAM wParam, LPARAM lParam)
{
	if (m_directFunction)
	{
		return ((SciFnDirect) m_directFunction)(m_directPointer, Msg, wParam, lParam);
	}
	return ::SendMessage(wEditor, Msg, wParam, lParam);
}

void TortoiseBlame::GetRange(int start, int end, char *text)
{
	TEXTRANGE tr;
	tr.chrg.cpMin = start;
	tr.chrg.cpMax = end;
	tr.lpstrText = text;
	SendMessage(wEditor, EM_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&tr));
}

void TortoiseBlame::SetTitle()
{
	char title[MAX_PATH + 100];
	strcpy_s(title, MAX_PATH + 100, szTitle);
	strcat_s(title, MAX_PATH + 100, " - ");
	strcat_s(title, MAX_PATH + 100, szViewtitle);
	::SetWindowText(wMain, title);
}

BOOL TortoiseBlame::OpenLogFile(const char *fileName)
{
	char logmsgbuf[10000+1];
	FILE * File;
	fopen_s(&File, fileName, "rb");
	if (File == 0)
	{
		return FALSE;
	}
	LONG rev = 0;
	std::string msg;
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
		msg = std::string(logmsgbuf, slength);
		if (reallength)
		{
			fseek(File, reallength-MAX_LOG_LENGTH, SEEK_CUR);
			msg = msg + _T("\n...");
		}
		int len2 = ::MultiByteToWideChar(CP_UTF8, NULL, msg.c_str(), min(msg.size(), MAX_LOG_LENGTH+5), wbuf, MAX_LOG_LENGTH+5);
		wbuf[len2] = 0;
		len2 = ::WideCharToMultiByte(CP_ACP, NULL, wbuf, len2, logmsgbuf, MAX_LOG_LENGTH+5, NULL, NULL);
		logmsgbuf[len2] = 0;
		msg = std::string(logmsgbuf);
		logmessages[rev] = msg;
	}
}

BOOL TortoiseBlame::OpenFile(const char *fileName)
{
	SendEditor(SCI_SETREADONLY, FALSE);
	SendEditor(SCI_CLEARALL);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SetTitle();
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_CANCEL);
	SendEditor(SCI_SETUNDOCOLLECTION, 0);
	::ShowWindow(wEditor, SW_HIDE);
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
	File.getline(line, sizeof(line)/sizeof(char));
	File.getline(line, sizeof(line)/sizeof(char));
	m_lowestrev = LONG_MAX;
	m_highestrev = 0;
	bool bUTF8 = true;
	do
	{
		File.getline(line, sizeof(line)/sizeof(TCHAR));
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
			dates.push_back(std::string(lineptr, 30));
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
			paths.push_back(std::string(lineptr));
			if (trimptr+1 < lineptr+61)
				lineptr +=61;
			else
				lineptr = (trimptr+1);
			trimptr = lineptr+30;
			while ((*trimptr == ' ')&&(trimptr > lineptr))
				trimptr--;
			*(trimptr+1) = 0;
			authors.push_back(std::string(lineptr));
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
	::SetFocus(wEditor);
	SendEditor(EM_EMPTYUNDOBUFFER);
	SendEditor(SCI_SETSAVEPOINT);
	SendEditor(SCI_GOTOPOS, 0);
	SendEditor(SCI_SETSCROLLWIDTHTRACKING, TRUE);
	SendEditor(SCI_SETREADONLY, TRUE);

	//check which lexer to use, depending on the filetype
	SetupLexer(fileName);
	::ShowWindow(wEditor, SW_SHOW);
	m_blamewidth = 0;
	::InvalidateRect(wMain, NULL, TRUE);
	RECT rc;
	GetWindowRect(wMain, &rc);
	SetWindowPos(wMain, 0, rc.left, rc.top, rc.right-rc.left-1, rc.bottom - rc.top, 0);
	return TRUE;
}

void TortoiseBlame::SetAStyle(int style, COLORREF fore, COLORREF back, int size, const char *face)
{
	SendEditor(SCI_STYLESETFORE, style, fore);
	SendEditor(SCI_STYLESETBACK, style, back);
	if (size >= 1)
		SendEditor(SCI_STYLESETSIZE, style, size);
	if (face)
		SendEditor(SCI_STYLESETFONT, style, reinterpret_cast<LPARAM>(face));
}

void TortoiseBlame::InitialiseEditor()
{
	m_directFunction = SendMessage(wEditor, SCI_GETDIRECTFUNCTION, 0, 0);
	m_directPointer = SendMessage(wEditor, SCI_GETDIRECTPOINTER, 0, 0);
	// Set up the global default style. These attributes are used wherever no explicit choices are made.
	SetAStyle(STYLE_DEFAULT, black, white, (DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10),
		((stdstring)(CRegStdString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New")))).c_str());
	SendEditor(SCI_SETTABWIDTH, (DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\BlameTabSize"), 4));
	SendEditor(SCI_SETREADONLY, TRUE);
	LRESULT pix = SendEditor(SCI_TEXTWIDTH, STYLE_LINENUMBER, (LPARAM)_T("_99999"));
	if (ShowLine)
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
	m_regOldLinesColor = CRegStdWORD(_T("Software\\TortoiseGit\\BlameOldColor"), RGB(230, 230, 255));
	m_regNewLinesColor = CRegStdWORD(_T("Software\\TortoiseGit\\BlameNewColor"), RGB(255, 230, 230));
}

void TortoiseBlame::StartSearch()
{
	if (currentDialog)
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

bool TortoiseBlame::DoSearch(LPSTR what, DWORD flags)
{
	TCHAR szWhat[80];
	int pos = SendEditor(SCI_GETCURRENTPOS);
	int line = SendEditor(SCI_LINEFROMPOSITION, pos);
	bool bFound = false;
	bool bCaseSensitive = !!(flags & FR_MATCHCASE);

	strcpy_s(szWhat, sizeof(szWhat), what);

	if(!bCaseSensitive)
	{
		char *p;
		size_t len = strlen(szWhat);
		for (p = szWhat; p < szWhat + len; p++)
		{
			if (isupper(*p)&&__isascii(*p))
				*p = _tolower(*p);
		}
	}

	std::string sWhat = std::string(szWhat);

	char buf[20];
	int i=0;
	for (i=line; (i<(int)authors.size())&&(!bFound); ++i)
	{
		int bufsize = SendEditor(SCI_GETLINE, i);
		char * linebuf = new char[bufsize+1];
		SecureZeroMemory(linebuf, bufsize+1);
		SendEditor(SCI_GETLINE, i, (LPARAM)linebuf);
		if (!bCaseSensitive)
		{
			char *p;
			for (p = linebuf; p < linebuf + bufsize; p++)
			{
				if (isupper(*p)&&__isascii(*p))
					*p = _tolower(*p);
			}
		}
		_stprintf_s(buf, 20, _T("%ld"), revs[i]);
		if (authors[i].compare(sWhat)==0)
			bFound = true;
		else if ((!bCaseSensitive)&&(_stricmp(authors[i].c_str(), szWhat)==0))
			bFound = true;
		else if (strcmp(buf, szWhat) == 0)
			bFound = true;
		else if (strstr(linebuf, szWhat))
			bFound = true;
		delete [] linebuf;
	}
	if (!bFound)
	{
		for (i=0; (i<line)&&(!bFound); ++i)
		{
			int bufsize = SendEditor(SCI_GETLINE, i);
			char * linebuf = new char[bufsize+1];
			SecureZeroMemory(linebuf, bufsize+1);
			SendEditor(SCI_GETLINE, i, (LPARAM)linebuf);
			if (!bCaseSensitive)
			{
				char *p;
				for (p = linebuf; p < linebuf + bufsize; p++)
				{
					if (isupper(*p)&&__isascii(*p))
						*p = _tolower(*p);
				}
			}
			_stprintf_s(buf, 20, _T("%ld"), revs[i]);
			if (authors[i].compare(sWhat)==0)
				bFound = true;
			else if ((!bCaseSensitive)&&(_stricmp(authors[i].c_str(), szWhat)==0))
				bFound = true;
			else if (strcmp(buf, szWhat) == 0)
				bFound = true;
			else if (strstr(linebuf, szWhat))
				bFound = true;
			delete [] linebuf;
		}
	}
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
		::MessageBox(wMain, searchstringnotfound, "TortoiseBlame", MB_ICONINFORMATION);
	}
	return true;
}

bool TortoiseBlame::GotoLine(long line)
{
	--line;
	if (line < 0)
		return false;
	if ((unsigned long)line >= authors.size())
	{
		line = authors.size()-1;
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

bool TortoiseBlame::ScrollToLine(long line)
{
	if (line < 0)
		return false;

	int nCurrentLine = SendEditor(SCI_GETFIRSTVISIBLELINE);

	int scrolldelta = line - nCurrentLine;
	SendEditor(SCI_LINESCROLL, 0, scrolldelta);

	return true;
}

void TortoiseBlame::CopySelectedLogToClipboard()
{
	if (m_selectedrev <= 0)
		return;
	std::map<LONG, std::string>::iterator iter;
	if ((iter = app.logmessages.find(m_selectedrev)) != app.logmessages.end())
	{
		std::string msg;
		msg += m_selectedauthor;
		msg += "  ";
		msg += app.m_selecteddate;
		msg += '\n';
		msg += iter->second;
		msg += _T("\n");
		if (OpenClipboard(app.wBlame))
		{
			EmptyClipboard();
			HGLOBAL hClipboardData;
			hClipboardData = GlobalAlloc(GMEM_DDESHARE, msg.size()+1);
			char * pchData;
			pchData = (char*)GlobalLock(hClipboardData);
			strcpy_s(pchData, msg.size()+1, msg.c_str());
			GlobalUnlock(hClipboardData);
			SetClipboardData(CF_TEXT,hClipboardData);
			CloseClipboard();
		}
	}
}

void TortoiseBlame::BlamePreviousRevision()
{
	LONG nRevisionTo = m_selectedorigrev - 1;
	if ( nRevisionTo<1 )
	{
		return;
	}

	// We now determine the smallest revision number in the blame file (but ignore "-1")
	// We do this for two reasons:
	// 1. we respect the "From revision" which the user entered
	// 2. we speed up the call of "svn blame" because previous smaller revision numbers don't have any effect on the result
	LONG nSmallestRevision = -1;
	for (LONG line=0;line<(LONG)app.revs.size();line++)
	{
		const LONG nRevision = app.revs[line];
		if ( nRevision > 0 )
		{
			if ( nSmallestRevision < 1 )
			{
				nSmallestRevision = nRevision;
			}
			else
			{
				nSmallestRevision = min(nSmallestRevision,nRevision);
			}
		}
	}

	char bufStartRev[20];
	_stprintf_s(bufStartRev, 20, _T("%d"), nSmallestRevision);

	char bufEndRev[20];
	_stprintf_s(bufEndRev, 20, _T("%d"), nRevisionTo);

	char bufLine[20];
	_stprintf_s(bufLine, 20, _T("%d"), m_SelectedLine+1); //using the current line is a good guess.

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	stdstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
	stdstring svnCmd = _T(" /command:blame ");
	svnCmd += _T(" /path:\"");
	svnCmd += szOrigPath;
	svnCmd += _T("\"");
	svnCmd += _T(" /startrev:");
	svnCmd += bufStartRev;
	svnCmd += _T(" /endrev:");
	svnCmd += bufEndRev;
	svnCmd += _T(" /line:");
	svnCmd += bufLine;
	if (bIgnoreEOL)
		svnCmd += _T(" /ignoreeol");
	if (bIgnoreSpaces)
		svnCmd += _T(" /ignorespaces");
	if (bIgnoreAllSpaces)
		svnCmd += _T(" /ignoreallspaces");
	if (CreateProcess(tortoiseProcPath.c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
	{
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
	}
}

void TortoiseBlame::DiffPreviousRevision()
{
	LONG nRevisionTo = m_selectedorigrev;
	if ( nRevisionTo<1 )
	{
		return;
	}

	LONG nRevisionFrom = nRevisionTo-1;

	char bufStartRev[20];
	_stprintf_s(bufStartRev, 20, _T("%d"), nRevisionFrom);

	char bufEndRev[20];
	_stprintf_s(bufEndRev, 20, _T("%d"), nRevisionTo);

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	stdstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
	stdstring svnCmd = _T(" /command:diff ");
	svnCmd += _T(" /path:\"");
	svnCmd += szOrigPath;
	svnCmd += _T("\"");
	svnCmd += _T(" /startrev:");
	svnCmd += bufStartRev;
	svnCmd += _T(" /endrev:");
	svnCmd += bufEndRev;
	if (CreateProcess(tortoiseProcPath.c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
	{
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
	}
}

void TortoiseBlame::ShowLog()
{
	char bufRev[20];
	_stprintf_s(bufRev, 20, _T("%d"), m_selectedorigrev);

	STARTUPINFO startup;
	PROCESS_INFORMATION process;
	memset(&startup, 0, sizeof(startup));
	startup.cb = sizeof(startup);
	memset(&process, 0, sizeof(process));
	stdstring tortoiseProcPath = GetAppDirectory() + _T("TortoiseProc.exe");
	stdstring svnCmd = _T(" /command:log ");
	svnCmd += _T(" /path:\"");
	svnCmd += szOrigPath;
	svnCmd += _T("\"");
	svnCmd += _T(" /startrev:");
	svnCmd += bufRev;
	svnCmd += _T(" /pegrev:");
	svnCmd += bufRev;
	if (CreateProcess(tortoiseProcPath.c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process))
	{
		CloseHandle(process.hThread);
		CloseHandle(process.hProcess);
	}
}

void TortoiseBlame::Notify(SCNotification *notification)
{
	switch (notification->nmhdr.code)
	{
	case SCN_SAVEPOINTREACHED:
		break;

	case SCN_SAVEPOINTLEFT:
		break;
	case SCN_PAINTED:
		InvalidateRect(wBlame, NULL, FALSE);
		InvalidateRect(wLocator, NULL, FALSE);
		break;
	case SCN_GETBKCOLOR:
		if ((m_colorage)&&(notification->line < (int)revs.size()))
		{
			notification->lParam = InterColor(DWORD(m_regOldLinesColor), DWORD(m_regNewLinesColor), (revs[notification->line]-m_lowestrev)*100/((m_highestrev-m_lowestrev)+1));
		}
		break;
	}
}

void TortoiseBlame::Command(int id)
{
	switch (id)
	{
	case IDM_EXIT:
		::PostQuitMessage(0);
		break;
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
}

void TortoiseBlame::GotoLineDlg()
{
	if (DialogBox(hResource, MAKEINTRESOURCE(IDD_GOTODLG), wMain, GotoDlgProc)==IDOK)
	{
		GotoLine(m_gotoline);
	}
}

INT_PTR CALLBACK TortoiseBlame::GotoDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM /*lParam*/)
{
	switch (uMsg)
	{
	case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
			case IDOK:
				{
					HWND hEditCtrl = GetDlgItem(hwndDlg, IDC_LINENUMBER);
					if (hEditCtrl)
					{
						TCHAR buf[MAX_PATH];
						if (::GetWindowText(hEditCtrl, buf, MAX_PATH))
						{
							m_gotoline = _ttol(buf);
						}

					}
				}
			// fall through
			case IDCANCEL:
				EndDialog(hwndDlg, wParam);
				break;
			}
		}
		break;
	}
	return FALSE;
}

LONG TortoiseBlame::GetBlameWidth()
{
	if (m_blamewidth)
		return m_blamewidth;
	LONG blamewidth = 0;
	SIZE width;
	CreateFont();
	HDC hDC = ::GetDC(wBlame);
	HFONT oldfont = (HFONT)::SelectObject(hDC, m_font);
	TCHAR buf[MAX_PATH];
	_stprintf_s(buf, MAX_PATH, _T("%8ld "), 88888888);
	::GetTextExtentPoint(hDC, buf, _tcslen(buf), &width);
	m_revwidth = width.cx + BLAMESPACE;
	blamewidth += m_revwidth;
	if (ShowDate)
	{
		_stprintf_s(buf, MAX_PATH, _T("%30s"), _T("31.08.2001 06:24:14"));
		::GetTextExtentPoint32(hDC, buf, _tcslen(buf), &width);
		m_datewidth = width.cx + BLAMESPACE;
		blamewidth += m_datewidth;
	}
	if (ShowAuthor)
	{
		SIZE maxwidth = {0};
		for (std::vector<std::string>::iterator I = authors.begin(); I != authors.end(); ++I)
		{
			::GetTextExtentPoint32(hDC, I->c_str(), I->size(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_authorwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_authorwidth;
	}
	if (ShowPath)
	{
		SIZE maxwidth = {0};
		for (std::vector<std::string>::iterator I = paths.begin(); I != paths.end(); ++I)
		{
			::GetTextExtentPoint32(hDC, I->c_str(), I->size(), &width);
			if (width.cx > maxwidth.cx)
				maxwidth = width;
		}
		m_pathwidth = maxwidth.cx + BLAMESPACE;
		blamewidth += m_pathwidth;
	}
	::SelectObject(hDC, oldfont);
	POINT pt = {blamewidth, 0};
	LPtoDP(hDC, &pt, 1);
	m_blamewidth = pt.x;
	ReleaseDC(wBlame, hDC);
	return m_blamewidth;
}

void TortoiseBlame::CreateFont()
{
	if (m_font)
		return;
	LOGFONT lf = {0};
	lf.lfWeight = 400;
	HDC hDC = ::GetDC(wBlame);
	lf.lfHeight = -MulDiv((DWORD)CRegStdWORD(_T("Software\\TortoiseGit\\BlameFontSize"), 10), GetDeviceCaps(hDC, LOGPIXELSY), 72);
	lf.lfCharSet = DEFAULT_CHARSET;
	CRegStdString fontname = CRegStdString(_T("Software\\TortoiseGit\\BlameFontName"), _T("Courier New"));
	_tcscpy_s(lf.lfFaceName, 32, ((stdstring)fontname).c_str());
	m_font = ::CreateFontIndirect(&lf);

	lf.lfItalic = TRUE;
	m_italicfont = ::CreateFontIndirect(&lf);

	ReleaseDC(wBlame, hDC);
}

void TortoiseBlame::DrawBlame(HDC hDC)
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
	GetClientRect(wBlame, &rc);
	for (LRESULT i=line; i<(line+linesonscreen); ++i)
	{
		sel = FALSE;
		if (i < (int)revs.size())
		{
			if (mergelines[i])
				oldfont = (HFONT)::SelectObject(hDC, m_italicfont);
			else
				oldfont = (HFONT)::SelectObject(hDC, m_font);
			::SetBkColor(hDC, m_windowcolor);
			::SetTextColor(hDC, m_textcolor);
			if (authors[i].size()>0)
			{
				if (authors[i].compare(m_mouseauthor)==0)
					::SetBkColor(hDC, m_mouseauthorcolor);
				if (authors[i].compare(m_selectedauthor)==0)
				{
					::SetBkColor(hDC, m_selectedauthorcolor);
					::SetTextColor(hDC, m_texthighlightcolor);
					sel = TRUE;
				}
			}
			if ((revs[i] == m_mouserev)&&(!sel))
				::SetBkColor(hDC, m_mouserevcolor);
			if (revs[i] == m_selectedrev)
			{
				::SetBkColor(hDC, m_selectedrevcolor);
				::SetTextColor(hDC, m_texthighlightcolor);
			}
			_stprintf_s(buf, MAX_PATH, _T("%8ld       "), revs[i]);
			rc.right = rc.left + m_revwidth;
			::ExtTextOut(hDC, 0, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
			int Left = m_revwidth;
			if (ShowDate)
			{
				rc.right = rc.left + Left + m_datewidth;
				_stprintf_s(buf, MAX_PATH, _T("%30s            "), dates[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_datewidth;
			}
			if (ShowAuthor)
			{
				rc.right = rc.left + Left + m_authorwidth;
				_stprintf_s(buf, MAX_PATH, _T("%-30s            "), authors[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_authorwidth;
			}
			if (ShowPath)
			{
				rc.right = rc.left + Left + m_pathwidth;
				_stprintf_s(buf, MAX_PATH, _T("%-60s            "), paths[i].c_str());
				::ExtTextOut(hDC, Left, Y, ETO_CLIPPED, &rc, buf, _tcslen(buf), 0);
				Left += m_authorwidth;
			}
			if ((i==m_SelectedLine)&&(currentDialog))
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

void TortoiseBlame::DrawHeader(HDC hDC)
{
	if (hDC == NULL)
		return;

	RECT rc;
	HFONT oldfont = (HFONT)::SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
	GetClientRect(wHeader, &rc);

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
}

void TortoiseBlame::DrawLocatorBar(HDC hDC)
{
	if (hDC == NULL)
		return;

	LONG_PTR line = SendEditor(SCI_GETFIRSTVISIBLELINE);
	LONG_PTR linesonscreen = SendEditor(SCI_LINESONSCREEN);
	LONG_PTR Y = 0;
	COLORREF blackColor = GetSysColor(COLOR_WINDOWTEXT);

	RECT rc;
	GetClientRect(wLocator, &rc);
	RECT lineRect = rc;
	LONG height = rc.bottom-rc.top;
	LONG currentLine = 0;

	// draw the colored bar
	for (std::vector<LONG>::const_iterator it = revs.begin(); it != revs.end(); ++it)
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
		lineRect.bottom = (currentLine * height / revs.size());
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		Y = lineRect.bottom;
	}

	if (revs.size())
	{
		// now draw two lines indicating the scroll position of the source view
		SetBkColor(hDC, blackColor);
		lineRect.top = line * height / revs.size();
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
		lineRect.top = (line + linesonscreen) * height / revs.size();
		lineRect.bottom = lineRect.top+1;
		::ExtTextOut(hDC, 0, 0, ETO_OPAQUE, &lineRect, NULL, 0, NULL);
	}
}

void TortoiseBlame::StringExpand(LPSTR str)
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
void TortoiseBlame::StringExpand(LPWSTR str)
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

	CRegStdWORD loc = CRegStdWORD(_T("Software\\TortoiseGit\\LanguageID"), 1033);
	long langId = loc;

	CLangDll langDLL;
	app.hResource = langDLL.Init(_T("TortoiseBlame"), langId);
	if (app.hResource == NULL)
		app.hResource = app.hInstance;

	// Initialize global strings
	LoadString(app.hResource, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(app.hResource, IDC_TORTOISEBLAME, szWindowClass, MAX_LOADSTRING);
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
		MessageBox(NULL, szInfo, _T("TortoiseBlame"), MB_ICONERROR);
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


	hAccelTable = LoadAccelerators(app.hResource, (LPCTSTR)IDC_TORTOISEBLAME);

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
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= (LPCTSTR)IDC_TORTOISEBLAME;
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
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseBlameBlame");
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
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseBlameHeader");
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
	wcex.hIcon			= LoadIcon(hResource, (LPCTSTR)IDI_TORTOISEBLAME);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= 0;
	wcex.lpszClassName	= _T("TortoiseBlameLocator");
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

	CRegStdWORD pos(_T("Software\\TortoiseGit\\TBlamePos"), 0);
	CRegStdWORD width(_T("Software\\TortoiseGit\\TBlameSize"), 0);
	CRegStdWORD state(_T("Software\\TortoiseGit\\TBlameState"), 0);
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

void TortoiseBlame::InitSize()
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
	InvalidateRect(wMain, NULL, FALSE);
	::SetWindowPos(wEditor, 0, sourcerc.left, sourcerc.top, sourcerc.right - sourcerc.left, sourcerc.bottom - sourcerc.top, 0);
	::SetWindowPos(wBlame, 0, blamerc.left, blamerc.top, blamerc.right - blamerc.left, blamerc.bottom - blamerc.top, 0);
	if (m_colorage)
		::SetWindowPos(wLocator, 0, 0, blamerc.top, LOCATOR_WIDTH, blamerc.bottom - blamerc.top, SWP_SHOWWINDOW);
	else
		::ShowWindow(wLocator, SW_HIDE);
}

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
		app.wEditor = ::CreateWindow(
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
		::ShowWindow(app.wEditor, SW_SHOW);
		::SetFocus(app.wEditor);
		app.wBlame = ::CreateWindow(
			_T("TortoiseBlameBlame"),
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
			_T("TortoiseBlameHeader"),
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
			_T("TortoiseBlameLocator"),
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
			CRegStdWORD pos(_T("Software\\TortoiseGit\\TBlamePos"), 0);
			CRegStdWORD width(_T("Software\\TortoiseGit\\TBlameSize"), 0);
			CRegStdWORD state(_T("Software\\TortoiseGit\\TBlameState"), 0);
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
			::DestroyWindow(app.wEditor);
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
				std::map<LONG, std::string>::iterator iter;
				if ((iter = app.logmessages.find(rev)) != app.logmessages.end())
				{
					std::string msg;
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
			int y = ((int)(short)HIWORD(lParam));
			LONG_PTR line = app.SendEditor(SCI_GETFIRSTVISIBLELINE);
			LONG_PTR height = app.SendEditor(SCI_TEXTHEIGHT);
			line = line + (y/height);
			if (line < (LONG)app.revs.size())
			{
				app.SetSelectedLine(line);
				if (app.revs[line] != app.m_selectedrev)
				{
					app.m_selectedrev = app.revs[line];
					app.m_selectedorigrev = app.origrevs[line];
					app.m_selectedauthor = app.authors[line];
					app.m_selecteddate = app.dates[line];
				}
				else
				{
					app.m_selectedauthor.clear();
					app.m_selecteddate.clear();
					app.m_selectedrev = -2;
					app.m_selectedorigrev = -2;
				}
				::InvalidateRect(app.wBlame, NULL, FALSE);
			}
			else
			{
				app.SetSelectedLine(-1);
			}
		}
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

#pragma warning(pop)
