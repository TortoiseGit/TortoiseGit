// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016 - TortoiseGit
// Copyright (C) 2003-2011, 2015 - TortoiseSVN

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
#include "UnicodeUtils.h"
#include "StringUtils.h"
#include "ClipboardHelper.h"
#include "SmartHandle.h"

int strwildcmp(const char *wild, const char *string)
{
	const char *cp = NULL;
	const char *mp = NULL;
	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
		{
			return 0;
		}
		++wild;
		++string;
	}
	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)
			{
				return 1;
			}
			mp = wild;
			cp = string+1;
		}
		else if ((*wild == *string) || (*wild == '?'))
		{
			++wild;
			++string;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
	{
		++wild;
	}
	return !*wild;
}

int wcswildcmp(const wchar_t *wild, const wchar_t *string)
{
	const wchar_t *cp = NULL;
	const wchar_t *mp = NULL;
	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
		{
			return 0;
		}
		++wild;
		++string;
	}
	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)
			{
				return 1;
			}
			mp = wild;
			cp = string+1;
		}
		else if ((*wild == *string) || (*wild == '?'))
		{
			++wild;
			++string;
		}
		else
		{
			wild = mp;
			string = cp++;
		}
	}

	while (*wild == '*')
	{
		++wild;
	}
	return !*wild;
}

#ifdef _MFC_VER

void CStringUtils::RemoveAccelerators(CString& text)
{
	int pos = 0;
	while ((pos=text.Find('&',pos))>=0)
	{
		if (text.GetLength() > (pos-1))
		{
			if (text.GetAt(pos+1)!=' ')
				text.Delete(pos);
		}
		++pos;
	}
}

TCHAR CStringUtils::GetAccellerator(const CString& text)
{
	int pos = 0;
	while ((pos = text.Find('&', pos)) >= 0)
	{
		if (text.GetLength() > (pos - 1))
		{
			if (text.GetAt(pos + 1) != ' ' && text.GetAt(pos + 1) != '&')
				return towupper(text.GetAt(pos + 1));
		}
		++pos;
	}
	return L'\0';
}

bool CStringUtils::WriteAsciiStringToClipboard(const CStringA& sClipdata, LCID lcid, HWND hOwningWnd)
{
	CClipboardHelper clipboardHelper;
	if (!clipboardHelper.Open(hOwningWnd))
		return false;

	EmptyClipboard();
	HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc(sClipdata.GetLength() + 1);
	if (!hClipboardData)
		return false;

	char* pchData = (char*)GlobalLock(hClipboardData);
	if (!pchData)
		return false;

	strcpy_s(pchData, sClipdata.GetLength() + 1, (LPCSTR)sClipdata);
	GlobalUnlock(hClipboardData);
	if (!SetClipboardData(CF_TEXT, hClipboardData))
		return false;

	HANDLE hlocmem = CClipboardHelper::GlobalAlloc(sizeof(LCID));
	if (!hlocmem)
		return false;

	PLCID plcid = (PLCID)GlobalLock(hlocmem);
	if (plcid)
	{
		*plcid = lcid;
		SetClipboardData(CF_LOCALE, static_cast<HANDLE>(plcid));
	}
	GlobalUnlock(hlocmem);

	return true;
}

bool CStringUtils::WriteAsciiStringToClipboard(const CStringW& sClipdata, HWND hOwningWnd)
{
	CClipboardHelper clipboardHelper;
	if (!clipboardHelper.Open(hOwningWnd))
		return false;

	EmptyClipboard();
	HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc((sClipdata.GetLength() + 1) * sizeof(WCHAR));
	if (!hClipboardData)
		return false;

	WCHAR* pchData = (WCHAR*)GlobalLock(hClipboardData);
	if (!pchData)
		return false;

	_tcscpy_s(pchData, sClipdata.GetLength()+1, (LPCWSTR)sClipdata);
	GlobalUnlock(hClipboardData);
	if (!SetClipboardData(CF_UNICODETEXT, hClipboardData))
		return false;

	// no need to also set CF_TEXT : the OS does this
	// automatically.
	return true;
}

bool CStringUtils::WriteDiffToClipboard(const CStringA& sClipdata, HWND hOwningWnd)
{
	UINT cFormat = RegisterClipboardFormat(_T("TGIT_UNIFIEDDIFF"));
	if (cFormat == 0)
		return false;
	CClipboardHelper clipboardHelper;
	if (!clipboardHelper.Open(hOwningWnd))
		return false;

	EmptyClipboard();
	HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc(sClipdata.GetLength() + 1);
	if (!hClipboardData)
		return false;

	char* pchData = (char*)GlobalLock(hClipboardData);
	if (!pchData)
		return false;

	strcpy_s(pchData, sClipdata.GetLength()+1, (LPCSTR)sClipdata);
	GlobalUnlock(hClipboardData);
	if (!SetClipboardData(cFormat,hClipboardData))
		return false;
	if (!SetClipboardData(CF_TEXT, hClipboardData))
		return false;

	CString sClipdataW = CUnicodeUtils::GetUnicode(sClipdata);
	auto hClipboardDataW = CClipboardHelper::GlobalAlloc(sClipdataW.GetLength()*sizeof(wchar_t) + 1);
	if (!hClipboardDataW)
		return false;

	wchar_t* pchDataW = (wchar_t*)GlobalLock(hClipboardDataW);
	if (!pchDataW)
		return false;

	wcscpy_s(pchDataW, sClipdataW.GetLength() + 1, (LPCWSTR)sClipdataW);
	GlobalUnlock(hClipboardDataW);
	if (!SetClipboardData(CF_UNICODETEXT, hClipboardDataW))
		return false;

	return true;
}

bool CStringUtils::ReadStringFromTextFile(const CString& path, CString& text)
{
	if (!PathFileExists(path))
		return false;
	try
	{
		CStdioFile file;
		// w/o typeBinary for some files \r gets dropped
		if (!file.Open(path, CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite))
			return false;

		CStringA filecontent;
		UINT filelength = (UINT)file.GetLength();
		int bytesread = (int)file.Read(filecontent.GetBuffer(filelength), filelength);
		filecontent.ReleaseBuffer(bytesread);
		text = CUnicodeUtils::GetUnicode(filecontent);
		file.Close();
	}
	catch (CFileException* pE)
	{
		text.Empty();
		pE->Delete();
	}
	return true;
}

#endif // #ifdef _MFC_VER

#if defined(CSTRING_AVAILABLE) || defined(_MFC_VER)
BOOL CStringUtils::WildCardMatch(const CString& wildcard, const CString& string)
{
	return _tcswildcmp(wildcard, string);
}

CString CStringUtils::LinesWrap(const CString& longstring, int limit /* = 80 */, bool bCompactPaths /* = true */)
{
	CString retString;
	if ((longstring.GetLength() < limit) || (limit == 0))
		return longstring;  // no wrapping needed.
	// now start breaking the string into lines

	int linepos = 0;
	int lineposold = 0;
	CString temp;
	while ((linepos = longstring.Find('\n', linepos)) >= 0)
	{
		temp = longstring.Mid(lineposold, linepos-lineposold);
		if ((linepos+1)<longstring.GetLength())
			++linepos;
		else
			break;
		lineposold = linepos;
		if (!retString.IsEmpty())
			retString += _T("\n");
		retString += WordWrap(temp, limit, bCompactPaths, false, 4);
	}
	temp = longstring.Mid(lineposold);
	if (!temp.IsEmpty())
		retString += _T("\n");
	retString += WordWrap(temp, limit, bCompactPaths, false, 4);
	retString.Trim();
	return retString;
}

CString CStringUtils::WordWrap(const CString& longstring, int limit, bool bCompactPaths, bool bForceWrap, int tabSize)
{
	int nLength = longstring.GetLength();
	CString retString;

	if (limit < 0)
		limit = 0;

	int nLineStart = 0;
	int nLineEnd = 0;
	int tabOffset = 0;
	for (int i = 0; i < nLength; ++i)
	{
		if (i-nLineStart+tabOffset >= limit)
		{
			if (nLineEnd == nLineStart)
			{
				if (bForceWrap)
					nLineEnd = i;
				else
				{
					while ((i < nLength) && (longstring[i] != ' ') && (longstring[i] != '\t'))
						++i;
					nLineEnd = i;
				}
			}
			if (bCompactPaths)
			{
				CString longline = longstring.Mid(nLineStart, nLineEnd-nLineStart).Left(MAX_PATH-1);
				if ((bCompactPaths)&&(longline.GetLength() < MAX_PATH))
				{
					if (((!PathIsFileSpec(longline))&&longline.Find(':')<3)||(PathIsURL(longline)))
					{
						TCHAR buf[MAX_PATH] = {0};
						PathCompactPathEx(buf, longline, limit+1, 0);
						longline = buf;
					}
				}
				retString += longline;
			}
			else
				retString += longstring.Mid(nLineStart, nLineEnd-nLineStart);
			retString += L"\n";
			tabOffset = 0;
			nLineStart = nLineEnd;
		}
		if (longstring[i] == ' ')
			nLineEnd = i;
		if (longstring[i] == '\t')
		{
			tabOffset += (tabSize - i % tabSize);
			nLineEnd = i;
		}
	}
	if (bCompactPaths)
	{
		CString longline = longstring.Mid(nLineStart).Left(MAX_PATH-1);
		if ((bCompactPaths)&&(longline.GetLength() < MAX_PATH))
		{
			if (((!PathIsFileSpec(longline))&&longline.Find(':')<3)||(PathIsURL(longline)))
			{
				TCHAR buf[MAX_PATH] = {0};
				PathCompactPathEx(buf, longline, limit+1, 0);
				longline = buf;
			}
		}
		retString += longline;
	}
	else
		retString += longstring.Mid(nLineStart);

	return retString;
}
int CStringUtils::GetMatchingLength (const CString& lhs, const CString& rhs)
{
	int lhsLength = lhs.GetLength();
	int rhsLength = rhs.GetLength();
	int maxResult = min (lhsLength, rhsLength);

	LPCTSTR pLhs = lhs;
	LPCTSTR pRhs = rhs;

	for (int i = 0; i < maxResult; ++i)
		if (pLhs[i] != pRhs[i])
			return i;

	return maxResult;
}

int CStringUtils::FastCompareNoCase (const CStringW& lhs, const CStringW& rhs)
{
	// attempt latin-only comparison

	INT_PTR count = min (lhs.GetLength(), rhs.GetLength()+1);
	const wchar_t* left = lhs;
	const wchar_t* right = rhs;
	for (const wchar_t* last = left + count+1; left < last; ++left, ++right)
	{
		int leftChar = *left;
		int rightChar = *right;

		int diff = leftChar - rightChar;
		if (diff != 0)
		{
			// case-sensitive comparison found a difference

			if ((leftChar | rightChar) >= 0x80)
			{
				// non-latin char -> fall back to CRT code
				// (full comparison required as we might have
				// skipped special chars / UTF plane selectors)

				return _wcsicmp (lhs, rhs);
			}

			// normalize to lower case

			if ((leftChar >= 'A') && (leftChar <= 'Z'))
				leftChar += 'a' - 'A';
			if ((rightChar >= 'A') && (rightChar <= 'Z'))
				rightChar += 'a' - 'A';

			// compare again

			diff = leftChar - rightChar;
			if (diff != 0)
				return diff;
		}
	}

	// must be equal (both ended with a 0)

	return 0;
}

static void cleanup_space(CString& string)
{
	for (int pos = 0; pos < string.GetLength(); ++pos)
	{
		if (_istspace(string[pos])) {
			string.SetAt(pos ,_T(' '));
			int cnt;
			for (cnt = 0; _istspace(string[pos + cnt + 1]); ++cnt);
			string.Delete(pos + 1, cnt);
		}
	}
}

static void get_sane_name(CString* out, const CString* name, const CString& email)
{
	const CString* src = name;
	if (name->GetLength() < 3 || 60 < name->GetLength() || _tcschr(*name, _T('@')) || _tcschr(*name, _T('<')) || _tcschr(*name, _T('>')))
		src = &email;
	else if (name == out)
		return;
	*out = *src;
}

static void parse_bogus_from(const CString& mailaddress, CString& parsedAddress, CString* parsedName)
{
	/* John Doe <johndoe> */

	int bra = mailaddress.Find(L"<");
	if (bra < 0)
		return;
	int ket = mailaddress.Find(L">");
	if (ket < 0)
		return;

	parsedAddress = mailaddress.Mid(bra + 1, ket - bra - 1);

	if (parsedName)
	{
		*parsedName = mailaddress.Left(bra).Trim();
		get_sane_name(parsedName, parsedName, parsedAddress);
	}
}

void CStringUtils::ParseEmailAddress(CString mailaddress, CString& parsedAddress, CString* parsedName)
{
	auto buf = mailaddress.GetBuffer();
	auto at = _tcschr(buf, _T('@'));
	if (!at)
	{
		parse_bogus_from(mailaddress, parsedAddress, parsedName);
		return;
	}

	/* Pick up the string around '@', possibly delimited with <>
	 * pair; that is the email part.
	 */
	while (at > buf)
	{
		auto c = at[-1];
		if (_istspace(c))
			break;
		if (c == _T('<')) {
			at[-1] = _T(' ');
			break;
		}
		at--;
	}

	mailaddress.ReleaseBuffer();
	size_t el = _tcscspn(at, _T(" \n\t\r\v\f>"));
	parsedAddress = mailaddress.Mid((int)(at - buf), (int)el);
	mailaddress.Delete((int)(at - buf), (int)(el + (at[el] ? 1 : 0)));

	/* The remainder is name.  It could be
	 *
	 * - "John Doe <john.doe@xz>"			(a), or
	 * - "john.doe@xz (John Doe)"			(b), or
	 * - "John (zzz) Doe <john.doe@xz> (Comment)"	(c)
	 *
	 * but we have removed the email part, so
	 *
	 * - remove extra spaces which could stay after email (case 'c'), and
	 * - trim from both ends, possibly removing the () pair at the end
	 *   (cases 'b' and 'c').
	 */
	cleanup_space(mailaddress);
	mailaddress.Trim();
	if (!mailaddress.IsEmpty() && ((mailaddress[0] == _T('(') && mailaddress[mailaddress.GetLength() - 1] == _T(')')) || (mailaddress[0] == _T('"') && mailaddress[mailaddress.GetLength() - 1] == _T('"'))))
		mailaddress = mailaddress.Mid(1, mailaddress.GetLength() - 2);

	if (parsedName)
		get_sane_name(parsedName, &mailaddress, parsedAddress);
}
#endif // #if defined(CSTRING_AVAILABLE) || defined(_MFC_VER)

bool CStringUtils::WriteStringToTextFile(const std::wstring& path, const std::wstring& text, bool bUTF8 /* = true */)
{
	DWORD dwWritten = 0;
	CAutoFile hFile = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (!hFile)
		return false;

	if (bUTF8)
	{
		std::string buf = CUnicodeUtils::StdGetUTF8(text);
		if (!WriteFile(hFile, buf.c_str(), (DWORD)buf.length(), &dwWritten, NULL))
		{
			return false;
		}
	}
	else
	{
		if (!WriteFile(hFile, text.c_str(), (DWORD)text.length(), &dwWritten, NULL))
		{
			return false;
		}
	}
	return true;
}

inline static void PipeToNull(TCHAR* ptr)
{
	if (*ptr == '|')
		*ptr = '\0';
}

void CStringUtils::PipesToNulls(TCHAR* buffer, size_t length)
{
	TCHAR* ptr = buffer + length;
	while (ptr != buffer)
	{
		PipeToNull(ptr);
		ptr--;
	}
}

void CStringUtils::PipesToNulls(TCHAR* buffer)
{
	TCHAR* ptr = buffer;
	while (*ptr != 0)
	{
		PipeToNull(ptr);
		++ptr;
	}
}
