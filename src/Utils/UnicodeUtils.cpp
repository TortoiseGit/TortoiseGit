// TortoiseGit - a Windows shell extension for easy version control

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
#include "StdAfx.h"
#include "unicodeutils.h"

CUnicodeUtils::CUnicodeUtils(void)
{
}

CUnicodeUtils::~CUnicodeUtils(void)
{
}

#if defined(_MFC_VER) || defined(CSTRING_AVAILABLE)

struct CodeMap
{
	int m_Code;
	TCHAR * m_CodeName;
};
int CUnicodeUtils::GetCPCode(CString &codename)
{
	static CodeMap map[]=
	{	
		{037, _T("IBM037")},// IBM EBCDIC US-Canada 
		{437, _T("IBM437")},// OEM United States 
		{500, _T("IBM500")},// IBM EBCDIC International 
		{708, _T("ASMO-708")},// Arabic (ASMO 708) 
		{709, _T("Arabic")},// (ASMO-449+, BCON V4) 
		{710, _T("Arabic")},// - Transparent Arabic 
		{720, _T("DOS-720")},// Arabic (Transparent ASMO); Arabic (DOS) 
		{737, _T("ibm737")},// OEM Greek (formerly 437G); Greek (DOS) 
		{775, _T("ibm775")},// OEM Baltic; Baltic (DOS) 
		{850, _T("ibm850")},// OEM Multilingual Latin 1; Western European (DOS) 
		{852, _T("ibm852")},// OEM Latin 2; Central European (DOS) 
		{855, _T("IBM855")},// OEM Cyrillic (primarily Russian) 
		{857, _T("ibm857")},// OEM Turkish; Turkish (DOS) 
		{858, _T("IBM00858")},// OEM Multilingual Latin 1 + Euro symbol 
		{860, _T("IBM860")},// OEM Portuguese; Portuguese (DOS) 
		{861, _T("ibm861")},// OEM Icelandic; Icelandic (DOS) 
		{862, _T("DOS-862")},// OEM Hebrew; Hebrew (DOS) 
		{863, _T("IBM863")},// OEM French Canadian; French Canadian (DOS) 
		{864, _T("IBM864")},// OEM Arabic; Arabic (864) 
		{865, _T("IBM865")},// OEM Nordic; Nordic (DOS) 
		{866, _T("cp866")},// OEM Russian; Cyrillic (DOS) 
		{869, _T("ibm869")},// OEM Modern Greek; Greek, Modern (DOS) 
		{870, _T("IBM870")},// IBM EBCDIC Multilingual/ROECE (Latin 2); IBM EBCDIC Multilingual Latin 2 
		{874, _T("windows-874")},// ANSI/OEM Thai (same as 28605, ISO 8859-15); Thai (Windows) 
		{875, _T("cp875")},// IBM EBCDIC Greek Modern 
		{932, _T("shift_jis")},// ANSI/OEM Japanese; Japanese (Shift-JIS) 
		{936, _T("gb2312")},// ANSI/OEM Simplified Chinese (PRC, Singapore); Chinese Simplified (GB2312) 
		{949, _T("ks_c_5601-1987")},// ANSI/OEM Korean (Unified Hangul Code) 
		{949, _T("cp949")},// ANSI/OEM Korean (Unified Hangul Code)
		{950, _T("big5")},// ANSI/OEM Traditional Chinese (Taiwan; Hong Kong SAR, PRC); Chinese Traditional (Big5) 
		{1026,_T("IBM1026")},// IBM EBCDIC Turkish (Latin 5) 
		{1047,_T("IBM01047")},// IBM EBCDIC Latin 1/Open System 
		{1140,_T("IBM01140")},// IBM EBCDIC US-Canada (037 + Euro symbol); IBM EBCDIC (US-Canada-Euro) 
		{1141, _T("IBM01141")},// IBM EBCDIC Germany (20273 + Euro symbol); IBM EBCDIC (Germany-Euro) 
		{1142, _T("IBM01142")},// IBM EBCDIC Denmark-Norway (20277 + Euro symbol); IBM EBCDIC (Denmark-Norway-Euro) 
		{1143, _T("IBM01143")},// IBM EBCDIC Finland-Sweden (20278 + Euro symbol); IBM EBCDIC (Finland-Sweden-Euro) 
		{1144, _T("IBM01144")},// IBM EBCDIC Italy (20280 + Euro symbol); IBM EBCDIC (Italy-Euro) 
		{1145, _T("IBM01145")},// IBM EBCDIC Latin America-Spain (20284 + Euro symbol); IBM EBCDIC (Spain-Euro) 
		{1146, _T("IBM01146")},// IBM EBCDIC United Kingdom (20285 + Euro symbol); IBM EBCDIC (UK-Euro) 
		{1147, _T("IBM01147")},// IBM EBCDIC France (20297 + Euro symbol); IBM EBCDIC (France-Euro) 
		{1148, _T("IBM01148")},// IBM EBCDIC International (500 + Euro symbol); IBM EBCDIC (International-Euro) 
		{1149, _T("IBM01149")},// IBM EBCDIC Icelandic (20871 + Euro symbol); IBM EBCDIC (Icelandic-Euro) 
		{1200, _T("utf-16")},// Unicode UTF-16, little endian byte order (BMP of ISO 10646); available only to managed applications 
		{1201, _T("unicodeFFFE")},// Unicode UTF-16, big endian byte order; available only to managed applications 
		{1250, _T("windows-1250")},// ANSI Central European; Central European (Windows) 
		{1251, _T("windows-1251")},// ANSI Cyrillic; Cyrillic (Windows)
		{1251, _T("cp1251")},
		{1251, _T("cp-1251")},
		{1251, _T("cp_1251")},
		{1252, _T("windows-1252")},// ANSI Latin 1; Western European (Windows) 
		{1253, _T("windows-1253")},// ANSI Greek; Greek (Windows) 
		{1254, _T("windows-1254")},// ANSI Turkish; Turkish (Windows) 
		{1255, _T("windows-1255")},// ANSI Hebrew; Hebrew (Windows) 
		{1256, _T("windows-1256")},// ANSI Arabic; Arabic (Windows) 
		{1257, _T("windows-1257")},// ANSI Baltic; Baltic (Windows) 
		{1258, _T("windows-1258")},// ANSI/OEM Vietnamese; Vietnamese (Windows) 
		{1361, _T("Johab")},// Korean (Johab) 
		{10000,_T("macintosh")},// MAC Roman; Western European (Mac) 
		{10001, _T("x-mac-japanese")},// Japanese (Mac) 
		{10002, _T("x-mac-chinesetrad")},// MAC Traditional Chinese (Big5); Chinese Traditional (Mac) 
		{10003, _T("x-mac-korean")},// Korean (Mac) 
		{10004, _T("x-mac-arabic")},// Arabic (Mac) 
		{10005, _T("x-mac-hebrew")},// Hebrew (Mac) 
		{10006, _T("x-mac-greek")},// Greek (Mac) 
		{10007, _T("x-mac-cyrillic")},// Cyrillic (Mac) 
		{10008, _T("x-mac-chinesesimp")},// MAC Simplified Chinese (GB 2312); Chinese Simplified (Mac) 
		{10010, _T("x-mac-romanian")},// Romanian (Mac) 
		{10017, _T("x-mac-ukrainian")},// Ukrainian (Mac) 
		{10021, _T("x-mac-thai")},// Thai (Mac) 
		{10029, _T("x-mac-ce")},// MAC Latin 2; Central European (Mac) 
		{10079, _T("x-mac-icelandic")},// Icelandic (Mac) 
		{10081, _T("x-mac-turkish")},// Turkish (Mac) 
		{10082, _T("x-mac-croatian")},// Croatian (Mac) 
		{12000, _T("utf-32")},// Unicode UTF-32, little endian byte order; available only to managed applications 
		{12001, _T("utf-32BE")},// Unicode UTF-32, big endian byte order; available only to managed applications 
		{20000, _T("x-Chinese_CNS")},// CNS Taiwan; Chinese Traditional (CNS) 
		{20001, _T("x-cp20001")},// TCA Taiwan 
		{20002, _T("x_Chinese-Eten")},// Eten Taiwan; Chinese Traditional (Eten) 
		{20003, _T("x-cp20003")},// IBM5550 Taiwan 
		{20004, _T("x-cp20004")},// TeleText Taiwan 
		{20005, _T("x-cp20005")},// Wang Taiwan 
		{20105, _T("x-IA5")},// IA5 (IRV International Alphabet No. 5, 7-bit); Western European (IA5) 
		{20106, _T("x-IA5-German")},// IA5 German (7-bit) 
		{20107, _T("x-IA5-Swedish")},// IA5 Swedish (7-bit) 
		{20108, _T("x-IA5-Norwegian")},// IA5 Norwegian (7-bit) 
		{20127, _T("us-ascii")},// US-ASCII (7-bit) 
		{20261, _T("x-cp20261")},// T.61 
		{20269, _T("x-cp20269")},// ISO 6937 Non-Spacing Accent 
		{20273, _T("IBM273")},// IBM EBCDIC Germany 
		{20277, _T("IBM277")},//IBM EBCDIC Denmark-Norway 
		{20278, _T("IBM278")},// IBM EBCDIC Finland-Sweden 
		{20280, _T("IBM280")},// IBM EBCDIC Italy 
		{20284, _T("IBM284")},// IBM EBCDIC Latin America-Spain 
		{20285, _T("IBM285")},// IBM EBCDIC United Kingdom 
		{20290, _T("IBM290")},// IBM EBCDIC Japanese Katakana Extended 
		{20297, _T("IBM297")},// IBM EBCDIC France 
		{20420, _T("IBM420")},// IBM EBCDIC Arabic 
		{20423, _T("IBM423")},// IBM EBCDIC Greek 
		{20424, _T("IBM424")},// IBM EBCDIC Hebrew 
		{20833, _T("x-EBCDIC-KoreanExtended")},// IBM EBCDIC Korean Extended 
		{20838, _T("IBM-Thai")},// IBM EBCDIC Thai 
		{20866, _T("koi8-r")},// Russian (KOI8-R); Cyrillic (KOI8-R) 
		{20871, _T("IBM871")},// IBM EBCDIC Icelandic 
		{20880, _T("IBM880")},// IBM EBCDIC Cyrillic Russian 
		{20905, _T("IBM905")},// IBM EBCDIC Turkish 
		{20924, _T("IBM00924")},// IBM EBCDIC Latin 1/Open System (1047 + Euro symbol) 
		{20932, _T("EUC-JP")},// Japanese (JIS 0208-1990 and 0121-1990) 
		{20936, _T("x-cp20936")},// Simplified Chinese (GB2312); Chinese Simplified (GB2312-80) 
		{20949, _T("x-cp20949")},// Korean Wansung 
		{21025, _T("cp1025")},// IBM EBCDIC Cyrillic Serbian-Bulgarian 
		{21027, _T("21027")},// (deprecated) 
		{21866, _T("koi8-u")},// Ukrainian (KOI8-U); Cyrillic (KOI8-U) 
		{28591, _T("iso-8859-1")},// ISO 8859-1 Latin 1; Western European (ISO) 
		{28592, _T("iso-8859-2")},// ISO 8859-2 Central European; Central European (ISO) 
		{28593, _T("iso-8859-3")},// ISO 8859-3 Latin 3 
		{28594, _T("iso-8859-4")},// ISO 8859-4 Baltic 
		{28595, _T("iso-8859-5")},// ISO 8859-5 Cyrillic 
		{28596, _T("iso-8859-6")},// ISO 8859-6 Arabic 
		{28597, _T("iso-8859-7")},// ISO 8859-7 Greek 
		{28598, _T("iso-8859-8")},// ISO 8859-8 Hebrew; Hebrew (ISO-Visual) 
		{28599, _T("iso-8859-9")},// ISO 8859-9 Turkish 
		{28603, _T("iso-8859-13")},// ISO 8859-13 Estonian 
		{28605, _T("iso-8859-15")},// ISO 8859-15 Latin 9 
		{29001, _T("x-Europa")},// Europa 3 
		{38598, _T("iso-8859-8-i")},// ISO 8859-8 Hebrew; Hebrew (ISO-Logical) 
		{50220, _T("iso-2022-jp")},// ISO 2022 Japanese with no halfwidth Katakana; Japanese (JIS) 
		{50221, _T("csISO2022JP")},// ISO 2022 Japanese with halfwidth Katakana; Japanese (JIS-Allow 1 byte Kana) 
		{50222, _T("iso-2022-jp")},// ISO 2022 Japanese JIS X 0201-1989; Japanese (JIS-Allow 1 byte Kana - SO/SI) 
		{50225, _T("iso-2022-kr")},// ISO 2022 Korean 
		{50227, _T("x-cp50227")},// ISO 2022 Simplified Chinese; Chinese Simplified (ISO 2022) 
		{50229, _T("ISO")},// 2022 Traditional Chinese 
		{50930, _T("EBCDIC")},// Japanese (Katakana) Extended 
		{50931, _T("EBCDIC")},// US-Canada and Japanese 
		{50933, _T("EBCDIC")},// Korean Extended and Korean 
		{50935, _T("EBCDIC")},// Simplified Chinese Extended and Simplified Chinese 
		{50936, _T("EBCDIC")},// Simplified Chinese 
		{50937, _T("EBCDIC")},// US-Canada and Traditional Chinese 
		{50939, _T("EBCDIC")},// Japanese (Latin) Extended and Japanese 
		{51932, _T("euc-jp")},// EUC Japanese 
		{51936, _T("EUC-CN")},// EUC Simplified Chinese; Chinese Simplified (EUC) 
		{51949, _T("euc-kr")},// EUC Korean 
		{51950, _T("EUC")},// Traditional Chinese 
		{52936, _T("hz-gb-2312")},// HZ-GB2312 Simplified Chinese; Chinese Simplified (HZ) 
		{54936, _T("GB18030")},// Windows XP and later: GB18030 Simplified Chinese (4 byte); Chinese Simplified (GB18030) 
		{57002, _T("x-iscii-de")},// ISCII Devanagari 
		{57003, _T("x-iscii-be")},// ISCII Bengali 
		{57004, _T("x-iscii-ta")},// ISCII Tamil 
		{57005, _T("x-iscii-te")},// ISCII Telugu 
		{57006, _T("x-iscii-as")},// ISCII Assamese 
		{57007, _T("x-iscii-or")},// ISCII Oriya 
		{57008, _T("x-iscii-ka")},// ISCII Kannada 
		{57009, _T("x-iscii-ma")},// ISCII Malayalam 
		{57010, _T("x-iscii-gu")},// ISCII Gujarati 
		{57011, _T("x-iscii-pa")},// ISCII Punjabi 
		{65000, _T("utf-7")},// Unicode (UTF-7) 
		{65001, _T("utf-8")},// Unicode (UTF-8) 
		{0,NULL}
		
	};
	static CodeMap *p=map;
	codename=codename.MakeLower();
	while(p->m_CodeName != NULL)
	{
		CString str = p->m_CodeName;
		str=str.MakeLower();

		if( str == codename)
			return p->m_Code;
		p++;
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
//	SecureZeroMemory(buf, (string.GetLength()*4 + 1)*sizeof(char));
	int lengthIncTerminator = WideCharToMultiByte(acp, 0, string, -1, buf, len*4, NULL, NULL);
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
	SecureZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_ACP, 0, string, -1, buf, len*4);
	CStringW temp = CStringW(buf);
	delete [] buf;
	return (CUnicodeUtils::GetUTF8(temp));
}

CString CUnicodeUtils::GetUnicode(const CStringA& string, int acp)
{
	WCHAR * buf;
	int len = string.GetLength();
	if (len==0)
		return CString();
	buf = new WCHAR[len*4 + 1];
	SecureZeroMemory(buf, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(acp, 0, string, -1, buf, len*4);
	CString ret = CString(buf);
	delete [] buf;
	return ret;
}

CStringA CUnicodeUtils::ConvertWCHARStringToUTF8(const CString& string)
{
	CStringA sRet;
	char * buf;
	buf = new char[string.GetLength()+1];
	if (buf)
	{
		int i=0;
		for ( ; i<string.GetLength(); ++i)
		{
			buf[i] = (char)string.GetAt(i);
		}
		buf[i] = 0;
		sRet = CStringA(buf);
		delete [] buf;
	}
	return sRet;
}

#endif //_MFC_VER

#ifdef UNICODE
std::string CUnicodeUtils::StdGetUTF8(const wide_string& wide)
{
	int len = (int)wide.size();
	if (len==0)
		return std::string();
	int size = len*4;
	char * narrow = new char[size];
	int ret = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), len, narrow, size-1, NULL, NULL);
	narrow[ret] = 0;
	std::string sRet = std::string(narrow);
	delete [] narrow;
	return sRet;
}

wide_string CUnicodeUtils::StdGetUnicode(const std::string& multibyte)
{
	int len = (int)multibyte.size();
	if (len==0)
		return wide_string();
	int size = len*4;
	wchar_t * wide = new wchar_t[size];
	int ret = MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), len, wide, size - 1);
	wide[ret] = 0;
	wide_string sRet = wide_string(wide);
	delete [] wide;
	return sRet;
}
#endif

std::string WideToMultibyte(const wide_string& wide)
{
	char * narrow = new char[wide.length()*3+2];
	BOOL defaultCharUsed;
	int ret = (int)WideCharToMultiByte(CP_ACP, 0, wide.c_str(), (int)wide.size(), narrow, (int)wide.length()*3 - 1, ".", &defaultCharUsed);
	narrow[ret] = 0;
	std::string str = narrow;
	delete[] narrow;
	return str;
}

std::string WideToUTF8(const wide_string& wide)
{
	char * narrow = new char[wide.length()*3+2];
	int ret = (int)WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), narrow, (int)wide.length()*3 - 1, NULL, NULL);
	narrow[ret] = 0;
	std::string str = narrow;
	delete[] narrow;
	return str;
}

wide_string MultibyteToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return wide_string();

	wchar_t * wide = new wchar_t[multibyte.length()*2+2];
	if (wide == NULL)
		return wide_string();
	int ret = (int)MultiByteToWideChar(CP_ACP, 0, multibyte.c_str(), (int)multibyte.size(), wide, (int)length*2 - 1);
	wide[ret] = 0;
	wide_string str = wide;
	delete[] wide;
	return str;
}

wide_string UTF8ToWide(const std::string& multibyte)
{
	size_t length = multibyte.length();
	if (length == 0)
		return wide_string();

	wchar_t * wide = new wchar_t[length*2+2];
	if (wide == NULL)
		return wide_string();
	int ret = (int)MultiByteToWideChar(CP_UTF8, 0, multibyte.c_str(), (int)multibyte.size(), wide, (int)length*2 - 1);
	wide[ret] = 0;
	wide_string str = wide;
	delete[] wide;
	return str;
}
#ifdef UNICODE
stdstring UTF8ToString(const std::string& string) {return UTF8ToWide(string);}
std::string StringToUTF8(const stdstring& string) {return WideToUTF8(string);}
#else
stdstring UTF8ToString(const std::string& string) {return WideToMultibyte(UTF8ToWide(string));}
std::string StringToUTF8(const stdstring& string) {return WideToUTF8(MultibyteToWide(string));}
#endif


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
	int ret;

	if (lpBuffer == NULL)
		return 0;
	lpBuffer[0] = 0;
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
	pImage = (const STRINGRESOURCEIMAGE*)::LockResource(hGlobal);
	if(!pImage)
		return 0;

	nResourceSize = ::SizeofResource(hInstance, hResource);
	pImageEnd = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+nResourceSize);
	iIndex = uID&0x000f;

	while ((iIndex > 0) && (pImage < pImageEnd))
	{
		pImage = (const STRINGRESOURCEIMAGE*)(LPBYTE(pImage)+(sizeof(STRINGRESOURCEIMAGE)+(pImage->nLength*sizeof(WCHAR))));
		iIndex--;
	}
	if (pImage >= pImageEnd)
		return 0;
	if (pImage->nLength == 0)
		return 0;
#ifdef UNICODE
	ret = pImage->nLength;
	if (ret > nBufferMax)
		ret = nBufferMax;
	wcsncpy_s((wchar_t *)lpBuffer, nBufferMax, pImage->achString, ret);
	lpBuffer[ret] = 0;
#else
	ret = WideCharToMultiByte(CP_ACP, 0, pImage->achString, pImage->nLength, (LPSTR)lpBuffer, nBufferMax-1, ".", &defaultCharUsed);
	lpBuffer[ret] = 0;
#endif
	return ret;
}

