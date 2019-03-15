// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2014, 2016, 2019 - TortoiseGit
// Copyright (C) 2003-2006, 2008 - TortoiseSVN

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

#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)

struct CodeMap
{
	int m_Code;
	TCHAR * m_CodeName;
};
int CUnicodeUtils::GetCPCode(const CString &codename)
{
	static CodeMap map[]=
	{
		{ 37, L"IBM037"},// IBM EBCDIC US-Canada
		{437, L"IBM437"},// OEM United States
		{500, L"IBM500"},// IBM EBCDIC International
		{708, L"ASMO-708"},// Arabic (ASMO 708)
		{709, L"Arabic"},// (ASMO-449+, BCON V4)
		{710, L"Arabic"},// - Transparent Arabic
		{720, L"DOS-720"},// Arabic (Transparent ASMO); Arabic (DOS)
		{737, L"ibm737"},// OEM Greek (formerly 437G); Greek (DOS)
		{775, L"ibm775"},// OEM Baltic; Baltic (DOS)
		{850, L"ibm850"},// OEM Multilingual Latin 1; Western European (DOS)
		{852, L"ibm852"},// OEM Latin 2; Central European (DOS)
		{855, L"IBM855"},// OEM Cyrillic (primarily Russian)
		{857, L"ibm857"},// OEM Turkish; Turkish (DOS)
		{858, L"IBM00858"},// OEM Multilingual Latin 1 + Euro symbol
		{860, L"IBM860"},// OEM Portuguese; Portuguese (DOS)
		{861, L"ibm861"},// OEM Icelandic; Icelandic (DOS)
		{862, L"DOS-862"},// OEM Hebrew; Hebrew (DOS)
		{863, L"IBM863"},// OEM French Canadian; French Canadian (DOS)
		{864, L"IBM864"},// OEM Arabic; Arabic (864)
		{865, L"IBM865"},// OEM Nordic; Nordic (DOS)
		{866, L"cp866"},// OEM Russian; Cyrillic (DOS)
		{869, L"ibm869"},// OEM Modern Greek; Greek, Modern (DOS)
		{870, L"IBM870"},// IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2
		{874, L"windows-874"},// ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows)
		{875, L"cp875"},// IBM EBCDIC Greek Modern
		{932, L"shift_jis"},// ANSI/OEM Japanese; Japanese (Shift-JIS)
		{936, L"gb2312"},// ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312)
		{949, L"ks_c_5601-1987"},// ANSI/OEM Korean (Unified Hangul Code)
		{949, L"cp949"},// ANSI/OEM Korean (Unified Hangul Code)
		{950, L"big5"},// ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5)
		{1026,L"IBM1026"},// IBM EBCDIC Turkish (Latin 5)
		{1047,L"IBM01047"},// IBM EBCDIC Latin 1/Open System
		{1140,L"IBM01140"},// IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro)
		{1141, L"IBM01141"},// IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro)
		{1142, L"IBM01142"},// IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro)
		{1143, L"IBM01143"},// IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro)
		{1144, L"IBM01144"},// IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro)
		{1145, L"IBM01145"},// IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro)
		{1146, L"IBM01146"},// IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro)
		{1147, L"IBM01147"},// IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro)
		{1148, L"IBM01148"},// IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro)
		{1149, L"IBM01149"},// IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro)
		{1200, L"utf-16"},// Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications
		{1201, L"unicodeFFFE"},// Unicode UTF-16, big endian byte order; available only to managed applications
		{1250, L"windows-1250"},// ANSI Central European; Central European (Windows)
		{1251, L"windows-1251"},// ANSI Cyrillic; Cyrillic (Windows)
		{1251, L"cp1251"},
		{1251, L"cp-1251"},
		{1251, L"cp_1251"},
		{1252, L"windows-1252"},// ANSI Latin 1; Western European (Windows)
		{1253, L"windows-1253"},// ANSI Greek; Greek (Windows)
		{1254, L"windows-1254"},// ANSI Turkish; Turkish (Windows)
		{1255, L"windows-1255"},// ANSI Hebrew; Hebrew (Windows)
		{1256, L"windows-1256"},// ANSI Arabic; Arabic (Windows)
		{1257, L"windows-1257"},// ANSI Baltic; Baltic (Windows)
		{1258, L"windows-1258"},// ANSI/OEM Vietnamese; Vietnamese (Windows)
		{1361, L"Johab"},// Korean (Johab)
		{10000,L"macintosh"},// MAC Roman; Western European (Mac)
		{10001, L"x-mac-japanese"},// Japanese (Mac)
		{10002, L"x-mac-chinesetrad"},// MAC Traditional Chinese (Big5); Chinese Traditional (Mac)
		{10003, L"x-mac-korean"},// Korean (Mac)
		{10004, L"x-mac-arabic"},// Arabic (Mac)
		{10005, L"x-mac-hebrew"},// Hebrew (Mac)
		{10006, L"x-mac-greek"},// Greek (Mac)
		{10007, L"x-mac-cyrillic"},// Cyrillic (Mac)
		{10008, L"x-mac-chinesesimp"},// MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac)
		{10010, L"x-mac-romanian"},// Romanian (Mac)
		{10017, L"x-mac-ukrainian"},// Ukrainian (Mac)
		{10021, L"x-mac-thai"},// Thai (Mac)
		{10029, L"x-mac-ce"},// MAC Latin 2; Central European (Mac)
		{10079, L"x-mac-icelandic"},// Icelandic (Mac)
		{10081, L"x-mac-turkish"},// Turkish (Mac)
		{10082, L"x-mac-croatian"},// Croatian (Mac)
		{12000, L"utf-32"},// Unicode UTF-32, little endian byte order; available only to managed applications
		{12001, L"utf-32BE"},// Unicode UTF-32, big endian byte order; available only to managed applications
		{20000, L"x-Chinese_CNS"},// CNS Taiwan; Chinese Traditional (CNS)
		{20001, L"x-cp20001"},// TCA Taiwan
		{20002, L"x_Chinese-Eten"},// Eten Taiwan; Chinese Traditional (Eten)
		{20003, L"x-cp20003"},// IBM5550 Taiwan
		{20004, L"x-cp20004"},// TeleText Taiwan
		{20005, L"x-cp20005"},// Wang Taiwan
		{20105, L"x-IA5"},// IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5)
		{20106, L"x-IA5-German"},// IA5 German (7-bit)
		{20107, L"x-IA5-Swedish"},// IA5 Swedish (7-bit)
		{20108, L"x-IA5-Norwegian"},// IA5 Norwegian (7-bit)
		{20127, L"us-ascii"},// US-ASCII (7-bit)
		{20261, L"x-cp20261"},// T.61
		{20269, L"x-cp20269"},// ISO 6937 Non-Spacing Accent
		{20273, L"IBM273"},// IBM EBCDIC Germany
		{20277, L"IBM277"},//IBM EBCDIC Denmark-Norway
		{20278, L"IBM278"},// IBM EBCDIC Finland-Sweden
		{20280, L"IBM280"},// IBM EBCDIC Italy
		{20284, L"IBM284"},// IBM EBCDIC Latin America-Spain
		{20285, L"IBM285"},// IBM EBCDIC United Kingdom
		{20290, L"IBM290"},// IBM EBCDIC Japanese Katakana Extended
		{20297, L"IBM297"},// IBM EBCDIC France
		{20420, L"IBM420"},// IBM EBCDIC Arabic
		{20423, L"IBM423"},// IBM EBCDIC Greek
		{20424, L"IBM424"},// IBM EBCDIC Hebrew
		{20833, L"x-EBCDIC-KoreanExtended"},// IBM EBCDIC Korean Extended
		{20838, L"IBM-Thai"},// IBM EBCDIC Thai
		{20866, L"koi8-r"},// Russian (KOI8-R); Cyrillic (KOI8-R)
		{20871, L"IBM871"},// IBM EBCDIC Icelandic
		{20880, L"IBM880"},// IBM EBCDIC Cyrillic Russian
		{20905, L"IBM905"},// IBM EBCDIC Turkish
		{20924, L"IBM00924"},// IBM EBCDIC Latin 1/Open System (1047 + Euro symbol)
		{20932, L"EUC-JP"},// Japanese (JIS 0208-1990 and 0121-1990)
		{20936, L"x-cp20936"},// Simplified Chinese (GB2312); Chinese Simplified (GB2312-80)
		{20949, L"x-cp20949"},// Korean Wansung
		{21025, L"cp1025"},// IBM EBCDIC Cyrillic Serbian-Bulgarian
		{21027, L"21027"},// (deprecated)
		{21866, L"koi8-u"},// Ukrainian (KOI8-U); Cyrillic (KOI8-U)
		{28591, L"iso-8859-1"},// ISO 8859-1 Latin 1; Western European (ISO)
		{28592, L"iso-8859-2"},// ISO 8859-2 Central European; Central European (ISO)
		{28593, L"iso-8859-3"},// ISO 8859-3 Latin 3
		{28594, L"iso-8859-4"},// ISO 8859-4 Baltic
		{28595, L"iso-8859-5"},// ISO 8859-5 Cyrillic
		{28596, L"iso-8859-6"},// ISO 8859-6 Arabic
		{28597, L"iso-8859-7"},// ISO 8859-7 Greek
		{28598, L"iso-8859-8"},// ISO 8859-8 Hebrew; Hebrew (ISO-Visual)
		{28599, L"iso-8859-9"},// ISO 8859-9 Turkish
		{28603, L"iso-8859-13"},// ISO 8859-13 Estonian
		{28605, L"iso-8859-15"},// ISO 8859-15 Latin 9
		{29001, L"x-Europa"},// Europa 3
		{38598, L"iso-8859-8-i"},// ISO 8859-8 Hebrew; Hebrew (ISO-Logical)
		{50220, L"iso-2022-jp"},// ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS)
		{50221, L"csISO2022JP"},// ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana)
		{50222, L"iso-2022-jp"},// ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI)
		{50225, L"iso-2022-kr"},// ISO 2022 Korean
		{50227, L"x-cp50227"},// ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022)
		{50229, L"ISO"},// 2022 Traditional Chinese
		{50930, L"EBCDIC"},// Japanese (Katakana) Extended
		{50931, L"EBCDIC"},// US-Canada and Japanese
		{50933, L"EBCDIC"},// Korean Extended and Korean
		{50935, L"EBCDIC"},// Simplified Chinese Extended and Simplified Chinese
		{50936, L"EBCDIC"},// Simplified Chinese
		{50937, L"EBCDIC"},// US-Canada and Traditional Chinese
		{50939, L"EBCDIC"},// Japanese (Latin) Extended and Japanese
		{51932, L"euc-jp"},// EUC Japanese
		{51936, L"EUC-CN"},// EUC Simplified Chinese; Chinese Simplified (EUC)
		{51949, L"euc-kr"},// EUC Korean
		{51950, L"EUC"},// Traditional Chinese
		{52936, L"hz-gb-2312"},// HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ)
		{54936, L"GB18030"},// Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030)
		{57002, L"x-iscii-de"},// ISCII Devanagari
		{57003, L"x-iscii-be"},// ISCII Bengali
		{57004, L"x-iscii-ta"},// ISCII Tamil
		{57005, L"x-iscii-te"},// ISCII Telugu
		{57006, L"x-iscii-as"},// ISCII Assamese
		{57007, L"x-iscii-or"},// ISCII Oriya
		{57008, L"x-iscii-ka"},// ISCII Kannada
		{57009, L"x-iscii-ma"},// ISCII Malayalam
		{57010, L"x-iscii-gu"},// ISCII Gujarati
		{57011, L"x-iscii-pa"},// ISCII Punjabi
		{65000, L"utf-7"},// Unicode (UTF-7)
		{65001, L"utf-8"},// Unicode (UTF-8)
		{0, nullptr}

	};
	static CodeMap *p=map;
	if (codename.IsEmpty())
		return CP_UTF8;
	CString code(codename);
	code.MakeLower();
	while (p->m_CodeName)
	{
		CString str = p->m_CodeName;
		str=str.MakeLower();

		if (str == code)
			return p->m_Code;
		++p;
	}

	return CP_UTF8;
}

CStringA CUnicodeUtils::GetUTF8(const CStringW& string)
{
	return GetMulti(string,CP_UTF8);
}

CStringA CUnicodeUtils::GetMulti(const CStringW& string,int acp)
{
	char * buf;
	CStringA retVal;
	int len = string.GetLength();
	if (len==0)
		return retVal;
	buf = retVal.GetBuffer(len*4 + 1);
	int lengthIncTerminator = WideCharToMultiByte(acp, 0, string, -1, buf, len * 4, nullptr, nullptr);
	retVal.ReleaseBuffer(lengthIncTerminator-1);
	return retVal;
}


CStringA CUnicodeUtils::GetUTF8(const CStringA& string)
{
	WCHAR * buf;
	int len = string.GetLength();
	if (len==0)
		return CStringA();
	buf = new WCHAR[len*4 + 1];
	int lengthIncTerminator = MultiByteToWideChar(CP_ACP, 0, string, -1, buf, len * 4);
	CStringW temp = CStringW(buf, lengthIncTerminator - 1);
	delete [] buf;
	return (CUnicodeUtils::GetUTF8(temp));
}

CString CUnicodeUtils::GetUnicode(const CStringA& string, int acp)
{
	WCHAR * buf;
	CString retVal;
	int len = string.GetLength();
	if (len==0)
		return retVal;
	buf = retVal.GetBuffer(len * 4 + 1);
	int lengthIncTerminator = MultiByteToWideChar(acp, 0, string, -1, buf, len * 4);
	retVal.ReleaseBuffer(lengthIncTerminator - 1);
	return retVal;
}

#endif //_MFC_VER

#ifdef UNICODE
std::string CUnicodeUtils::StdGetUTF8(const std::wstring& wide)
{
	int len = static_cast<int>(wide.size());
	if (len==0)
		return std::string();
	int size = len*4;
	char * narrow = new char[size];
	int ret = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), len, narrow, size - 1, nullptr, nullptr);
	narrow[ret] = '\0';
	std::string sRet = std::string(narrow);
	delete [] narrow;
	return sRet;
}

std::wstring CUnicodeUtils::StdGetUnicode(const std::string& multibyte)
{
	int len = static_cast<int>(multibyte.size());
	if (len==0)
		return std::wstring();
	int size = len*4;
	wchar_t * wide = new wchar_t[size];
	int ret = MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), len, wide, size - 1);
	wide[ret] = L'\0';
	std::wstring sRet = std::wstring(wide);
	delete [] wide;
	return sRet;
}
#endif

std::string WideToMultibyte(const std::wstring& wide)
{
	char * narrow = new char[wide.length()*3+2];
	BOOL defaultCharUsed;
	int ret = WideCharToMultiByte(CP_ACP, 0, wide.c_str(), static_cast<int>(wide.size()), narrow, static_cast<int>(wide.length()) * 3 - 1, ".", &defaultCharUsed);
	narrow[ret] = '\0';
	std::string str = narrow;
	delete[] narrow;
	return str;
}

std::string WideToUTF8(const std::wstring& wide)
{
	char * narrow = new char[wide.length()*3+2];
	int ret = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), narrow, static_cast<int>(wide.length()) * 3 - 1, nullptr, nullptr);
	narrow[ret] = '\0';
	std::string str = narrow;
	delete[] narrow;
	return str;
}

std::wstring MultibyteToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return std::wstring();

	wchar_t * wide = new wchar_t[multibyte.length()*2+2];
	if (!wide)
		return std::wstring();
	int ret = MultiByteToWideChar(CP_ACP, 0, multibyte.c_str(), static_cast<int>(multibyte.size()), wide, static_cast<int>(length) * 2 - 1);
	wide[ret] = L'\0';
	std::wstring str = wide;
	delete[] wide;
	return str;
}

std::wstring UTF8ToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return std::wstring();

	wchar_t * wide = new wchar_t[length*2+2];
	if (!wide)
		return std::wstring();
	int ret = MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), static_cast<int>(multibyte.size()), wide, static_cast<int>(length) * 2 - 1);
	wide[ret] = L'\0';
	std::wstring str = wide;
	delete[] wide;
	return str;
}

#pragma warning(push)
#pragma warning(disable: 4200)
struct STRINGRESOURCEIMAGE
{
	WORD nLength;
	WCHAR achString[];
};
#pragma warning(pop)	// C4200

int LoadStringEx(HINSTANCE hInstance, UINT uID, LPTSTR lpBuffer, int nBufferMax, WORD wLanguage)
{
	const STRINGRESOURCEIMAGE* pImage;
	const STRINGRESOURCEIMAGE* pImageEnd;
	ULONG nResourceSize;
	HGLOBAL hGlobal;
	UINT iIndex;
#ifndef UNICODE
	BOOL defaultCharUsed;
#endif
	if (!lpBuffer)
		return 0;
	lpBuffer[0] = L'\0';
	HRSRC hResource =  FindResourceEx(hInstance, RT_STRING, MAKEINTRESOURCE(((uID>>4)+1)), wLanguage);
	if (!hResource)
	{
		//try the default language before giving up!
		hResource = FindResource(hInstance, MAKEINTRESOURCE(((uID>>4)+1)), RT_STRING);
		if (!hResource)
			return 0;
	}
	hGlobal = LoadResource(hInstance, hResource);
	if (!hGlobal)
		return 0;
	pImage = static_cast<const STRINGRESOURCEIMAGE*>(::LockResource(hGlobal));
	if(!pImage)
		return 0;

	nResourceSize = ::SizeofResource(hInstance, hResource);
	pImageEnd = reinterpret_cast<const STRINGRESOURCEIMAGE*>(LPBYTE(pImage) + nResourceSize);
	iIndex = uID&0x000f;

	while ((iIndex > 0) && (pImage < pImageEnd))
	{
		pImage = reinterpret_cast<const STRINGRESOURCEIMAGE*>(LPBYTE(pImage) + (sizeof(STRINGRESOURCEIMAGE) + (pImage->nLength * sizeof(WCHAR))));
		iIndex--;
	}
	if (pImage >= pImageEnd)
		return 0;
	if (pImage->nLength == 0)
		return 0;
#ifdef UNICODE
	int ret = pImage->nLength;
	if (ret >= nBufferMax)
		ret = nBufferMax - 1;
	wcsncpy_s(static_cast<wchar_t*>(lpBuffer), nBufferMax, pImage->achString, ret);
	lpBuffer[ret] = L'\0';
#else
	auto ret = WideCharToMultiByte(CP_ACP, 0, pImage->achString, pImage->nLength, static_cast<LPSTR>(lpBuffer), nBufferMax - 1, ".", &defaultCharUsed);
	lpBuffer[ret] = L'\0';
#endif
	return ret;
}

