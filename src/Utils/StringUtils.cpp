// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2020 - TortoiseGit
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
#include <WinCrypt.h>
#include <scope_exit_noexcept.h>

#pragma comment(lib, "Crypt32.lib")

int strwildcmp(const char *wild, const char *string)
{
	const char* cp = nullptr;
	const char* mp = nullptr;
	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
			return 0;
		++wild;
		++string;
	}
	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)
				return 1;
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
		++wild;
	return !*wild;
}

int wcswildcmp(const wchar_t *wild, const wchar_t *string)
{
	const wchar_t* cp = nullptr;
	const wchar_t* mp = nullptr;
	while ((*string) && (*wild != '*'))
	{
		if ((*wild != *string) && (*wild != '?'))
			return 0;
		++wild;
		++string;
	}
	while (*string)
	{
		if (*wild == '*')
		{
			if (!*++wild)
				return 1;
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
		++wild;
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

	auto pchData = static_cast<char*>(GlobalLock(hClipboardData));
	if (!pchData)
		return false;

	strcpy_s(pchData, sClipdata.GetLength() + 1, static_cast<LPCSTR>(sClipdata));
	GlobalUnlock(hClipboardData);
	if (!SetClipboardData(CF_TEXT, hClipboardData))
		return false;

	HANDLE hlocmem = CClipboardHelper::GlobalAlloc(sizeof(LCID));
	if (!hlocmem)
		return false;

	auto plcid = static_cast<PLCID>(GlobalLock(hlocmem));
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

	auto pchData = static_cast<WCHAR*>(GlobalLock(hClipboardData));
	if (!pchData)
		return false;

	wcscpy_s(pchData, sClipdata.GetLength() + 1, static_cast<LPCWSTR>(sClipdata));
	GlobalUnlock(hClipboardData);
	if (!SetClipboardData(CF_UNICODETEXT, hClipboardData))
		return false;

	// no need to also set CF_TEXT : the OS does this
	// automatically.
	return true;
}

bool CStringUtils::WriteDiffToClipboard(const CStringA& sClipdata, HWND hOwningWnd)
{
	UINT cFormat = RegisterClipboardFormat(L"TGIT_UNIFIEDDIFF");
	if (cFormat == 0)
		return false;
	CClipboardHelper clipboardHelper;
	if (!clipboardHelper.Open(hOwningWnd))
		return false;

	EmptyClipboard();
	HGLOBAL hClipboardData = CClipboardHelper::GlobalAlloc(sClipdata.GetLength() + 1);
	if (!hClipboardData)
		return false;

	auto pchData = static_cast<char*>(GlobalLock(hClipboardData));
	if (!pchData)
		return false;

	strcpy_s(pchData, sClipdata.GetLength() + 1, static_cast<LPCSTR>(sClipdata));
	GlobalUnlock(hClipboardData);
	if (!SetClipboardData(cFormat,hClipboardData))
		return false;
	if (!SetClipboardData(CF_TEXT, hClipboardData))
		return false;

	CString sClipdataW = CUnicodeUtils::GetUnicode(sClipdata);
	auto hClipboardDataW = CClipboardHelper::GlobalAlloc((sClipdataW.GetLength() + 1) * sizeof(wchar_t));
	if (!hClipboardDataW)
		return false;

	auto pchDataW = static_cast<wchar_t*>(GlobalLock(hClipboardDataW));
	if (!pchDataW)
		return false;

	wcscpy_s(pchDataW, sClipdataW.GetLength() + 1, static_cast<LPCWSTR>(sClipdataW));
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
		UINT filelength = static_cast<UINT>(file.GetLength());
		int bytesread = static_cast<int>(file.Read(filecontent.GetBuffer(filelength), filelength));
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
	return wcswildcmp(wildcard, string);
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
			retString += L'\n';
		retString += WordWrap(temp, limit, bCompactPaths, false, 4);
	}
	temp = longstring.Mid(lineposold);
	if (!temp.IsEmpty())
		retString += L'\n';
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
			retString += L'\n';
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

bool CStringUtils::IsPlainReadableASCII(const CString& text)
{
	for (int i = 0; i < text.GetLength(); ++i)
	{
		if (text[i] < 32 || text[i] >= 127)
			return false;
	}
	return true;
}

static void cleanup_space(CString& string)
{
	for (int pos = 0; pos < string.GetLength(); ++pos)
	{
		if (_istspace(string[pos])) {
			string.SetAt(pos, L' ');
			int cnt;
			for (cnt = 0; _istspace(string[pos + cnt + 1]); ++cnt);
			string.Delete(pos + 1, cnt);
		}
	}
}

static void get_sane_name(CString* out, const CString* name, const CString& email)
{
	const CString* src = name;
	if (name->GetLength() < 3 || 60 < name->GetLength() || wcschr(*name, L'@') || wcschr(*name, L'<') || wcschr(*name, L'>'))
		src = &email;
	else if (name == out)
		return;
	*out = *src;
}

static void parse_bogus_from(const CString& mailaddress, CString& parsedAddress, CString* parsedName)
{
	/* John Doe <johndoe> */

	int bra = mailaddress.Find(L'<');
	if (bra < 0)
		return;
	int ket = mailaddress.Find(L'>');
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
	if (parsedName)
		parsedName->Empty();

	mailaddress = mailaddress.Trim();
	if (mailaddress.IsEmpty())
	{
		parsedAddress.Empty();
		return;
	}

	if (mailaddress.Left(1) == L'"')
	{
		mailaddress = mailaddress.TrimLeft();
		bool escaped = false;
		bool opened = true;
		for (int i = 1; i < mailaddress.GetLength(); ++i)
		{
			if (mailaddress[i] == L'"')
			{
				if (!escaped)
				{
					opened = !opened;
					if (!opened)
					{
						if (parsedName)
							*parsedName = mailaddress.Mid(1, i - 1);
						mailaddress = mailaddress.Mid(i);
						break;
					}
				}
				else
				{
					escaped = false;
					mailaddress.Delete(i - 1);
					--i;
				}
			}
			else if (mailaddress[i] == L'\\')
				escaped = !escaped;
		}
	}

	auto buf = mailaddress.GetBuffer();
	auto at = wcschr(buf, L'@');
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
		if (c == L'<')
		{
			at[-1] = L' ';
			break;
		}
		at--;
	}

	mailaddress.ReleaseBuffer();
	size_t el = wcscspn(at, L" \n\t\r\v\f>");
	parsedAddress = mailaddress.Mid(static_cast<int>(at - buf), static_cast<int>(el));
	mailaddress.Delete(static_cast<int>(at - buf), static_cast<int>(el + (at[el] ? 1 : 0)));

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
	if (!mailaddress.IsEmpty() && ((mailaddress[0] == L'(' && mailaddress[mailaddress.GetLength() - 1] == L')') || (mailaddress[0] == L'"' && mailaddress[mailaddress.GetLength() - 1] == L'"')))
		mailaddress = mailaddress.Mid(1, mailaddress.GetLength() - 2);

	if (parsedName && parsedName->IsEmpty())
		get_sane_name(parsedName, &mailaddress, parsedAddress);
}

bool CStringUtils::StartsWith(const wchar_t* heystack, const CString& needle)
{
	return wcsncmp(heystack, needle, needle.GetLength()) == 0;
}

bool CStringUtils::EndsWith(const CString& heystack, const wchar_t* needle)
{
	auto lenNeedle = wcslen(needle);
	auto lenHeystack = static_cast<size_t>(heystack.GetLength());
	if (lenNeedle > lenHeystack)
		return false;
	return wcsncmp(static_cast<LPCTSTR>(heystack) + (lenHeystack - lenNeedle), needle, lenNeedle) == 0;
}

bool CStringUtils::EndsWith(const CString& heystack, const wchar_t needle)
{
	auto lenHeystack = heystack.GetLength();
	if (!lenHeystack)
		return false;
	return *(static_cast<LPCTSTR>(heystack) + (lenHeystack - 1)) == needle;
}

bool CStringUtils::EndsWithI(const CString& heystack, const wchar_t* needle)
{
	auto lenNeedle = wcslen(needle);
	auto lenHeystack = static_cast<size_t>(heystack.GetLength());
	if (lenNeedle > lenHeystack)
		return false;
	return _wcsnicmp(static_cast<LPCTSTR>(heystack) + (lenHeystack - lenNeedle), needle, lenNeedle) == 0;
}

bool CStringUtils::StartsWithI(const wchar_t* heystack, const CString& needle)
{
	return _wcsnicmp(heystack, needle, needle.GetLength()) == 0;
}

bool CStringUtils::WriteStringToTextFile(LPCTSTR path, LPCTSTR text, bool bUTF8 /* = true */)
{
	return WriteStringToTextFile(static_cast<const std::wstring&>(path), static_cast<const std::wstring&>(text), bUTF8);
}

#endif // #if defined(CSTRING_AVAILABLE) || defined(_MFC_VER)

bool CStringUtils::StartsWith(const wchar_t* heystack, const wchar_t* needle)
{
	return wcsncmp(heystack, needle, wcslen(needle)) == 0;
}

bool CStringUtils::StartsWith(const char* heystack, const char* needle)
{
	return strncmp(heystack, needle, strlen(needle)) == 0;
}

bool CStringUtils::WriteStringToTextFile(const std::wstring& path, const std::wstring& text, bool bUTF8 /* = true */)
{
	DWORD dwWritten = 0;
	CAutoFile hFile = CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
		return false;

	if (bUTF8)
	{
		std::string buf = CUnicodeUtils::StdGetUTF8(text);
		if (!WriteFile(hFile, buf.c_str(), static_cast<DWORD>(buf.length()), &dwWritten, nullptr))
		{
			return false;
		}
	}
	else
	{
		if (!WriteFile(hFile, text.c_str(), static_cast<DWORD>(text.length()), &dwWritten, nullptr))
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
	while (*ptr)
	{
		PipeToNull(ptr);
		++ptr;
	}
}

std::unique_ptr<char[]> CStringUtils::Decrypt(const char * text)
{
	DWORD dwLen = 0;
	if (CryptStringToBinaryA(text, (DWORD)strlen(text), CRYPT_STRING_HEX, nullptr, &dwLen, nullptr, nullptr) == FALSE)
		return nullptr;

	std::unique_ptr<BYTE[]> strIn(new BYTE[dwLen + 1]);
	if (CryptStringToBinaryA(text, (DWORD)strlen(text), CRYPT_STRING_HEX, strIn.get(), &dwLen, nullptr, nullptr) == FALSE)
		return nullptr;

	DATA_BLOB blobin;
	blobin.cbData = dwLen;
	blobin.pbData = strIn.get();
	LPWSTR descr = nullptr;
	DATA_BLOB blobout = { 0 };
	if (CryptUnprotectData(&blobin, &descr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &blobout) == FALSE)
		return nullptr;
	SecureZeroMemory(blobin.pbData, blobin.cbData);

	std::unique_ptr<char[]> result(new char[blobout.cbData + 1]);
	strncpy_s(result.get(), blobout.cbData + 1, (const char*)blobout.pbData, blobout.cbData);
	SecureZeroMemory(blobout.pbData, blobout.cbData);
	LocalFree(blobout.pbData);
	LocalFree(descr);
	return result;
}

std::unique_ptr<wchar_t[]> CStringUtils::Decrypt(const wchar_t * text)
{
	DWORD dwLen = 0;
	if (CryptStringToBinaryW(text, (DWORD)wcslen(text), CRYPT_STRING_HEX, nullptr, &dwLen, nullptr, nullptr) == FALSE)
		return nullptr;

	std::unique_ptr<BYTE[]> strIn(new BYTE[dwLen + 1]);
	if (CryptStringToBinaryW(text, (DWORD)wcslen(text), CRYPT_STRING_HEX, strIn.get(), &dwLen, nullptr, nullptr) == FALSE)
		return nullptr;

	DATA_BLOB blobin;
	blobin.cbData = dwLen;
	blobin.pbData = strIn.get();
	LPWSTR descr = nullptr;
	DATA_BLOB blobout = { 0 };
	if (CryptUnprotectData(&blobin, &descr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &blobout) == FALSE)
		return nullptr;
	SecureZeroMemory(blobin.pbData, blobin.cbData);

	std::unique_ptr<wchar_t[]> result(new wchar_t[(blobout.cbData) / sizeof(wchar_t) + 1]);
	wcsncpy_s(result.get(), (blobout.cbData) / sizeof(wchar_t) + 1, (const wchar_t*)blobout.pbData, blobout.cbData / sizeof(wchar_t));
	SecureZeroMemory(blobout.pbData, blobout.cbData);
	LocalFree(blobout.pbData);
	LocalFree(descr);
	return result;
}

CStringA CStringUtils::Encrypt(const char* text)
{
	DATA_BLOB blobin = { 0 };
	DATA_BLOB blobout = { 0 };
	CStringA result;

	blobin.cbData = (DWORD)strlen(text);
	blobin.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(text));
	if (CryptProtectData(&blobin, L"TGITAuth", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &blobout) == FALSE)
		return result;
	DWORD dwLen = 0;
	if (CryptBinaryToStringA(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX | CRYPT_STRING_NOCRLF, nullptr, &dwLen) == FALSE)
		return result;
	auto strOut = std::make_unique<char[]>(dwLen + 1);
	if (CryptBinaryToStringA(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX | CRYPT_STRING_NOCRLF, strOut.get(), &dwLen) == FALSE)
		return result;
	LocalFree(blobout.pbData);

	result = strOut.get();

	return result;
}

CStringW CStringUtils::Encrypt(const wchar_t* text)
{
	DATA_BLOB blobin = { 0 };
	DATA_BLOB blobout = { 0 };
	CStringW result;

	blobin.cbData = static_cast<DWORD>(wcslen(text) * sizeof(wchar_t));
	blobin.pbData = reinterpret_cast<BYTE*>(const_cast<wchar_t*>(text));
	if (CryptProtectData(&blobin, L"TGITAuth", nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &blobout) == FALSE)
		return result;
	DWORD dwLen = 0;
	if (CryptBinaryToStringW(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX | CRYPT_STRING_NOCRLF, nullptr, &dwLen) == FALSE)
		return result;
	auto strOut = std::make_unique<wchar_t[]>(dwLen + 1);
	if (CryptBinaryToStringW(blobout.pbData, blobout.cbData, CRYPT_STRING_HEX | CRYPT_STRING_NOCRLF, strOut.get(), &dwLen) == FALSE)
		return result;
	LocalFree(blobout.pbData);

	result = strOut.get();

	return result;
}

BYTE HexLookup[513] = {
	"000102030405060708090a0b0c0d0e0f"
	"101112131415161718191a1b1c1d1e1f"
	"202122232425262728292a2b2c2d2e2f"
	"303132333435363738393a3b3c3d3e3f"
	"404142434445464748494a4b4c4d4e4f"
	"505152535455565758595a5b5c5d5e5f"
	"606162636465666768696a6b6c6d6e6f"
	"707172737475767778797a7b7c7d7e7f"
	"808182838485868788898a8b8c8d8e8f"
	"909192939495969798999a9b9c9d9e9f"
	"a0a1a2a3a4a5a6a7a8a9aaabacadaeaf"
	"b0b1b2b3b4b5b6b7b8b9babbbcbdbebf"
	"c0c1c2c3c4c5c6c7c8c9cacbcccdcecf"
	"d0d1d2d3d4d5d6d7d8d9dadbdcdddedf"
	"e0e1e2e3e4e5e6e7e8e9eaebecedeeef"
	"f0f1f2f3f4f5f6f7f8f9fafbfcfdfeff"
};
BYTE DecLookup[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // gap before first hex digit
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9,       // 0123456789
	0, 0, 0, 0, 0, 0, 0,             // :;<=>?@ (gap)
	10, 11, 12, 13, 14, 15,         // ABCDEF
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // GHIJKLMNOPQRS (gap)
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // TUVWXYZ[/]^_` (gap)
	10, 11, 12, 13, 14, 15          // abcdef
};

std::string CStringUtils::ToHexString(BYTE* pSrc, int nSrcLen)
{
	auto* pwHex = reinterpret_cast<WORD*>(HexLookup);
	auto dest = std::make_unique<char[]>((nSrcLen * 2) + 1);
	auto* pwDest = reinterpret_cast<WORD*>(dest.get());
	for (int j = 0; j < nSrcLen; ++j)
	{
		*pwDest = pwHex[*pSrc];
		++pwDest; ++pSrc;
	}
	*(reinterpret_cast<BYTE*>(pwDest)) = 0; // terminate the string
	return std::string(dest.get());
}

bool CStringUtils::FromHexString(const std::string& src, BYTE* pDest)
{
	if (src.size() % 2)
		return false;
	for (auto it = src.cbegin(); it != src.cend(); ++it)
	{
		if ((*it < '0') || (*it > 'f'))
			return false;
		int d = DecLookup[*it] << 4;
		// no bounds check necessary, since the 'if (src.size %2)' above
		// ensures that we have always one item more available
		++it;
		d |= DecLookup[*it];
		*pDest++ = static_cast<BYTE>(d);
	}
	return true;
}

std::string CStringUtils::Encrypt(const std::string& s, const std::string& password)
{
	HCRYPTPROV hProv;
	// Get handle to user default provider.
	if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
		return {};
	SCOPE_EXIT { CryptReleaseContext(hProv, 0); };

	HCRYPTHASH hHash;
	// Create hash object.
	if (!CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash))
		return {};
	SCOPE_EXIT { CryptDestroyHash(hHash); };

	// Hash password string.
	DWORD dwLength = DWORD(password.size());
	if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(password.c_str()), dwLength, 0))
		return {};

	// Create block cipher session key based on hash of the password.
	HCRYPTKEY hKey;
	if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey))
		return {};
	SCOPE_EXIT { CryptDestroyKey(hKey); };

	// Determine number of bytes to encrypt at a time.
	std::string starname = "*";
	starname += s;

	dwLength = DWORD(starname.size());
	auto buffer = std::make_unique<BYTE[]>(dwLength + 1024);
	memcpy(buffer.get(), starname.c_str(), dwLength);

	// Encrypt data
	if (!CryptEncrypt(hKey, 0, true, 0, buffer.get(), &dwLength, dwLength + 1024))
		return {};

	return CStringUtils::ToHexString(buffer.get(), dwLength);
}

std::string CStringUtils::Decrypt(const std::string& s, const std::string& password)
{
	HCRYPTPROV hProv;
	// Get handle to user default provider.
	if (!CryptAcquireContext(&hProv, nullptr, nullptr, PROV_RSA_AES, CRYPT_VERIFYCONTEXT | CRYPT_SILENT))
		return {};
	SCOPE_EXIT { CryptReleaseContext(hProv, 0); };

	HCRYPTHASH hHash;
	// Create hash object.
	if (!CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash))
		return {};
	SCOPE_EXIT { CryptDestroyHash(hHash); };

	// Hash password string.
	DWORD dwLength = DWORD(password.size());
	if (!CryptHashData(hHash, reinterpret_cast<const BYTE*>(password.c_str()), dwLength, 0))
		return {};

	// Create block cipher session key based on hash of the password.
	HCRYPTKEY hKey;
	if (!CryptDeriveKey(hProv, CALG_AES_256, hHash, 0, &hKey))
		return {};
	SCOPE_EXIT { CryptDestroyKey(hKey); };

	dwLength = DWORD(s.size() + 1024); // 1024 bytes should be enough for padding
	auto buffer = std::make_unique<BYTE[]>(dwLength);
	auto strIn = std::make_unique<BYTE[]>(s.size() + 1);
	if (!buffer || !strIn || !CStringUtils::FromHexString(s, strIn.get()))
		return {};

	// copy encrypted password to temporary buffer
	memcpy(buffer.get(), strIn.get(), s.size());
	dwLength = DWORD(s.size() / 2);
	CryptDecrypt(hKey, 0, true, 0, reinterpret_cast<BYTE*>(buffer.get()), &dwLength);
	auto decryptstring = std::string(reinterpret_cast<const char*>(buffer.get()), dwLength);
	if (decryptstring.empty() || decryptstring[0] != '*')
		return {};
	return decryptstring.substr(1);
}
