// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2010-2017, 2019 - TortoiseSVN

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
#include "Utils.h"
#include "UnicodeUtils.h"
#include "ResModule.h"
#include <regex>
#include <memory>
#include <fstream>
#include <string>
#include <algorithm>
#include <functional>
#include <locale>
#include <codecvt>

#pragma warning(push)
#pragma warning(disable: 4091) // 'typedef ': ignored on left of '' when no variable is declared
#include <Imagehlp.h>
#pragma warning(pop)

#pragma comment(lib, "Imagehlp.lib")

#ifndef RT_RIBBON
#define RT_RIBBON MAKEINTRESOURCE(28)
#endif


#define MYERROR {CUtils::Error(); return FALSE;}

static const WORD * AlignWORD(const WORD * pWord)
{
	const WORD * res = pWord;
	res += (((reinterpret_cast<UINT_PTR>(pWord) + 3) & ~3) - reinterpret_cast<UINT_PTR>(pWord)) / sizeof(WORD);
	return res;
}

std::wstring NumToStr(INT_PTR num)
{
	wchar_t buf[100];
	swprintf_s(buf, L"%Id", num);
	return buf;
}

CResModule::CResModule(void)
	: m_bTranslatedStrings(0)
	, m_bDefaultStrings(0)
	, m_bTranslatedDialogStrings(0)
	, m_bDefaultDialogStrings(0)
	, m_bTranslatedMenuStrings(0)
	, m_bDefaultMenuStrings(0)
	, m_bTranslatedAcceleratorStrings(0)
	, m_bDefaultAcceleratorStrings(0)
	, m_bTranslatedRibbonTexts(0)
	, m_bDefaultRibbonTexts(0)
	, m_wTargetLang(0)
	, m_hResDll(nullptr)
	, m_hUpdateRes(nullptr)
	, m_bQuiet(false)
	, m_bRTL(false)
	, m_bAdjustEOLs(false)
{
}

CResModule::~CResModule(void)
{
}

BOOL CResModule::ExtractResources(const std::vector<std::wstring>& filelist, LPCTSTR lpszPOFilePath, BOOL bNoUpdate, LPCTSTR lpszHeaderFile)
{
	for (auto I = filelist.cbegin(); I != filelist.cend(); ++I)
	{
		std::wstring filepath = *I;
		m_currentHeaderDataDialogs.clear();
		m_currentHeaderDataMenus.clear();
		m_currentHeaderDataStrings.clear();
		auto starpos = I->find('*');
		if (starpos != std::wstring::npos)
			filepath = I->substr(0, starpos);
		while (starpos != std::wstring::npos)
		{
			auto starposnext = I->find('*', starpos + 1);
			std::wstring headerfile = I->substr(starpos + 1, starposnext - starpos - 1);
			ScanHeaderFile(headerfile);
			starpos = starposnext;
		}
		m_hResDll = LoadLibraryEx(filepath.c_str(), nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
		if (!m_hResDll)
			MYERROR;

		size_t nEntries = m_StringEntries.size();
		// fill in the std::map with all translatable entries

		if (!m_bQuiet)
			_ftprintf(stdout, L"Extracting StringTable....");
		EnumResourceNames(m_hResDll, RT_STRING, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
		if (!m_bQuiet)
			_ftprintf(stdout, L"%4Iu Strings\n", m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		if (!m_bQuiet)
			_ftprintf(stdout, L"Extracting Dialogs........");
		EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
		if (!m_bQuiet)
			_ftprintf(stdout, L"%4Iu Strings\n", m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		if (!m_bQuiet)
			_ftprintf(stdout, L"Extracting Menus..........");
		EnumResourceNames(m_hResDll, RT_MENU, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
		if (!m_bQuiet)
			_ftprintf(stdout, L"%4Iu Strings\n", m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();
		if (!m_bQuiet)
			_ftprintf(stdout, L"Extracting Accelerators...");
		EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
		if (!m_bQuiet)
			_ftprintf(stdout, L"%4Iu Accelerators\n", m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();
		if (!m_bQuiet)
			_ftprintf(stdout, L"Extracting Ribbons........");
		EnumResourceNames(m_hResDll, RT_RIBBON, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
		if (!m_bQuiet)
			_ftprintf(stdout, L"%4Iu Strings\n", m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		// parse a probably existing file and update the translations which are
		// already done
		m_StringEntries.ParseFile(lpszPOFilePath, !bNoUpdate, m_bAdjustEOLs);

		FreeLibrary(m_hResDll);
	}

	// at last, save the new file
	return m_StringEntries.SaveFile(lpszPOFilePath, lpszHeaderFile);
}

BOOL CResModule::ExtractResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszPoFilePath, BOOL bNoUpdate, LPCTSTR lpszHeaderFile)
{
	m_hResDll = LoadLibraryEx(lpszSrcLangDllPath, nullptr, LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE);
	if (!m_hResDll)
		MYERROR;

	size_t nEntries = 0;
	// fill in the std::map with all translatable entries

	if (!m_bQuiet)
		_ftprintf(stdout, L"Extracting StringTable....");
	EnumResourceNames(m_hResDll, RT_STRING, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4Iu Strings\n", m_StringEntries.size());
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_ftprintf(stdout, L"Extracting Dialogs........");
	EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4Iu Strings\n", m_StringEntries.size()-nEntries);
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_ftprintf(stdout, L"Extracting Menus..........");
	EnumResourceNames(m_hResDll, RT_MENU, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4Iu Strings\n", m_StringEntries.size()-nEntries);
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_ftprintf(stdout, L"Extracting Accelerators...");
	EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4Iu Accelerators\n", m_StringEntries.size()-nEntries);
	nEntries = m_StringEntries.size();

	// parse a probably existing file and update the translations which are
	// already done
	m_StringEntries.ParseFile(lpszPoFilePath, !bNoUpdate, m_bAdjustEOLs);

	// at last, save the new file
	if (!m_StringEntries.SaveFile(lpszPoFilePath, lpszHeaderFile))
		goto DONE_ERROR;

	FreeLibrary(m_hResDll);

	return TRUE;

DONE_ERROR:
	if (m_hResDll)
		FreeLibrary(m_hResDll);
	return FALSE;
}

void CResModule::RemoveSignatures(LPCTSTR lpszDestLangDllPath)
{
	// Remove any signatures in the file:
	// if we don't remove it here, the signature will be invalid after
	// we modify this file, and the signtool.exe will throw an error and refuse to sign it again.
	auto hFile = CreateFile(lpszDestLangDllPath, FILE_READ_DATA | FILE_WRITE_DATA, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		CUtils::Error();
		return;
	}

	DWORD certcount = 0;
	DWORD indices[100];
	ImageEnumerateCertificates(hFile, CERT_SECTION_TYPE_ANY, &certcount, indices, _countof(indices));

	for (DWORD i = 0; i < certcount; ++i)
		ImageRemoveCertificate(hFile, i);

	CloseHandle(hFile);
}

BOOL CResModule::CreateTranslatedResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszDestLangDllPath, LPCTSTR lpszPOFilePath)
{
	if (!CopyFile(lpszSrcLangDllPath, lpszDestLangDllPath, FALSE))
		MYERROR;

	RemoveSignatures(lpszDestLangDllPath);

	int count = 0;
	do
	{
		m_hResDll = LoadLibraryEx(lpszSrcLangDllPath, nullptr, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_IGNORE_CODE_AUTHZ_LEVEL);
		if (!m_hResDll)
			Sleep(100);
		count++;
	} while (!m_hResDll && (count < 10));

	if (!m_hResDll)
		MYERROR;

	sDestFile = std::wstring(lpszDestLangDllPath);

	// get all translated strings
	if (!m_StringEntries.ParseFile(lpszPOFilePath, FALSE, m_bAdjustEOLs))
		goto DONE_ERROR;
	m_bTranslatedStrings = 0;
	m_bDefaultStrings = 0;
	m_bTranslatedDialogStrings = 0;
	m_bDefaultDialogStrings = 0;
	m_bTranslatedMenuStrings = 0;
	m_bDefaultMenuStrings = 0;
	m_bTranslatedAcceleratorStrings = 0;
	m_bDefaultAcceleratorStrings = 0;

	count = 0;
	do
	{
		m_hUpdateRes = BeginUpdateResource(sDestFile.c_str(), FALSE);
		if (!m_hUpdateRes)
			Sleep(100);
		count++;
	} while (!m_hUpdateRes && (count < 10));

	if (!m_hUpdateRes)
		MYERROR;


	if (!m_bQuiet)
		_ftprintf(stdout, L"Translating StringTable...");
	EnumResourceNames(m_hResDll, RT_STRING, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedStrings, m_bDefaultStrings);

	if (!m_bQuiet)
		_ftprintf(stdout, L"Translating Dialogs.......");
	EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedDialogStrings, m_bDefaultDialogStrings);

	if (!m_bQuiet)
		_ftprintf(stdout, L"Translating Menus.........");
	EnumResourceNames(m_hResDll, RT_MENU, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedMenuStrings, m_bDefaultMenuStrings);

	if (!m_bQuiet)
		_ftprintf(stdout, L"Translating Accelerators..");
	EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedAcceleratorStrings, m_bDefaultAcceleratorStrings);

	if (!m_bQuiet)
		_ftprintf(stdout, L"Translating Ribbons.......");
	EnumResourceNames(m_hResDll, RT_RIBBON, EnumResNameWriteCallback, reinterpret_cast<LONG_PTR>(this));
	if (!m_bQuiet)
		_ftprintf(stdout, L"%4d translated, %4d not translated\n", m_bTranslatedRibbonTexts, m_bDefaultRibbonTexts);
	BOOL bRes = TRUE;
	if (!EndUpdateResource(m_hUpdateRes, !bRes))
		MYERROR;

	AdjustCheckSum(sDestFile);

	FreeLibrary(m_hResDll);
	return TRUE;
DONE_ERROR:
	if (m_hResDll)
		FreeLibrary(m_hResDll);
	return FALSE;
}

BOOL CResModule::ExtractString(LPCTSTR lpszType)
{
	HRSRC       hrsrc = FindResource(m_hResDll, lpszType, RT_STRING);
	HGLOBAL     hglStringTable;
	LPWSTR      p;

	if (!hrsrc)
		MYERROR;
	hglStringTable = LoadResource(m_hResDll, hrsrc);

	if (!hglStringTable)
		goto DONE_ERROR;
	p = static_cast<LPWSTR>(LockResource(hglStringTable));

	if (!p)
		goto DONE_ERROR;
	/*  [Block of 16 strings.  The strings are Pascal style with a WORD
	length preceding the string.  16 strings are always written, even
	if not all slots are full.  Any slots in the block with no string
	have a zero WORD for the length.]
	*/

	//first check how much memory we need
	LPWSTR pp = p;
	for (int i=0; i<16; ++i)
	{
		int len = GET_WORD(pp);
		pp++;
		std::wstring msgid = std::wstring(pp, len);
		wchar_t buf[MAX_STRING_LENGTH * 2] = { 0 };
		wcscpy_s(buf, msgid.c_str());
		CUtils::StringExtend(buf);

		if (buf[0])
		{
			std::wstring str = std::wstring(buf);
			RESOURCEENTRY entry = m_StringEntries[str];
			InsertResourceIDs(RT_STRING, 0, entry, (reinterpret_cast<INT_PTR>(lpszType) - 1) * 16 + i, L"");
			if (wcschr(str.c_str(), '%'))
				entry.flag = L"#, c-format";
			m_StringEntries[str] = entry;
		}
		pp += len;
	}
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	return TRUE;
DONE_ERROR:
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	MYERROR;
}

BOOL CResModule::ReplaceString(LPCTSTR lpszType, WORD wLanguage)
{
	HRSRC       hrsrc = FindResourceEx(m_hResDll, RT_STRING, lpszType, wLanguage);
	HGLOBAL     hglStringTable;
	LPWSTR      p;

	if (!hrsrc)
		MYERROR;
	hglStringTable = LoadResource(m_hResDll, hrsrc);

	if (!hglStringTable)
		goto DONE_ERROR;
	p = static_cast<LPWSTR>(LockResource(hglStringTable));

	if (!p)
		goto DONE_ERROR;
/*  [Block of 16 strings.  The strings are Pascal style with a WORD
	length preceding the string.  16 strings are always written, even
	if not all slots are full.  Any slots in the block with no string
	have a zero WORD for the length.]
*/

	//first check how much memory we need
	size_t nMem = 0;
	LPWSTR pp = p;
	for (int i=0; i<16; ++i)
	{
		nMem++;
		size_t len = GET_WORD(pp);
		pp++;
		std::wstring msgid = std::wstring(pp, len);
		wchar_t buf[MAX_STRING_LENGTH * 2] = { 0 };
		wcscpy_s(buf, msgid.c_str());
		CUtils::StringExtend(buf);
		msgid = std::wstring(buf);

		RESOURCEENTRY resEntry;
		resEntry = m_StringEntries[msgid];
		wcscpy_s(buf, resEntry.msgstr.empty() ? msgid.c_str() : resEntry.msgstr.c_str());
		ReplaceWithRegex(buf);
		CUtils::StringCollapse(buf);
		size_t newlen = wcslen(buf);
		if (newlen)
			nMem += newlen;
		else
			nMem += len;
		pp += len;
	}

	WORD * newTable = new WORD[nMem + (nMem % 2)];
	SecureZeroMemory(newTable, (nMem + (nMem % 2))*2);

	size_t index = 0;
	for (int i=0; i<16; ++i)
	{
		int len = GET_WORD(p);
		p++;
		std::wstring msgid = std::wstring(p, len);
		wchar_t buf[MAX_STRING_LENGTH * 2] = { 0 };
		wcscpy_s(buf, msgid.c_str());
		CUtils::StringExtend(buf);
		msgid = std::wstring(buf);

		RESOURCEENTRY resEntry;
		resEntry = m_StringEntries[msgid];
		wcscpy_s(buf, resEntry.msgstr.empty() ? msgid.c_str() : resEntry.msgstr.c_str());
		ReplaceWithRegex(buf);
		CUtils::StringCollapse(buf);
		size_t newlen = wcslen(buf);
		if (newlen)
		{
			newTable[index++] = static_cast<WORD>(newlen);
			wcsncpy(reinterpret_cast<wchar_t*>(&newTable[index]), buf, newlen);
			index += newlen;
			m_bTranslatedStrings++;
		}
		else
		{
			newTable[index++] = static_cast<WORD>(len);
			if (len)
				wcsncpy(reinterpret_cast<wchar_t*>(&newTable[index]), p, len);
			index += len;
			if (len)
				m_bDefaultStrings++;
		}
		p += len;
	}

	if (!UpdateResource(m_hUpdateRes, RT_STRING, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newTable, static_cast<DWORD>(nMem + (nMem % 2))*2))
	{
		delete [] newTable;
		goto DONE_ERROR;
	}

	if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_STRING, lpszType, wLanguage, nullptr, 0)))
	{
		delete [] newTable;
		goto DONE_ERROR;
	}
	delete [] newTable;
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	return TRUE;
DONE_ERROR:
	UnlockResource(hglStringTable);
	FreeResource(hglStringTable);
	MYERROR;
}

BOOL CResModule::ExtractMenu(LPCTSTR lpszType)
{
	HRSRC       hrsrc = FindResource(m_hResDll, lpszType, RT_MENU);
	HGLOBAL     hglMenuTemplate;
	WORD        version, offset;
	const WORD *p, *p0;

	if (!hrsrc)
		MYERROR;

	hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

	if (!hglMenuTemplate)
		MYERROR;

	p = static_cast<const WORD*>(LockResource(hglMenuTemplate));

	if (!p)
		MYERROR;

	// Standard MENU resource
	//struct MenuHeader {
	//  WORD   wVersion;           // Currently zero
	//  WORD   cbHeaderSize;       // Also zero
	//};

	// MENUEX resource
	//struct MenuExHeader {
	//    WORD wVersion;           // One
	//    WORD wOffset;
	//    DWORD dwHelpId;
	//};
	p0 = p;
	version = GET_WORD(p);

	p++;

	switch (version)
	{
	case 0:
		{
			offset = GET_WORD(p);
			p += offset;
			p++;
			if (!ParseMenuResource(p))
				goto DONE_ERROR;
		}
		break;
	case 1:
		{
			offset = GET_WORD(p);
			p++;
			//dwHelpId = GET_DWORD(p);
			if (!ParseMenuExResource(p0 + offset))
				goto DONE_ERROR;
		}
		break;
	default:
		goto DONE_ERROR;
	}

	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	return TRUE;

DONE_ERROR:
	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	MYERROR;
}

BOOL CResModule::ReplaceMenu(LPCTSTR lpszType, WORD wLanguage)
{
	HRSRC       hrsrc = FindResourceEx(m_hResDll, RT_MENU, lpszType, wLanguage);
	HGLOBAL     hglMenuTemplate;
	WORD        version, offset;
	LPWSTR      p;
	WORD *p0;

	if (!hrsrc)
		MYERROR;    //just the language wasn't found

	hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

	if (!hglMenuTemplate)
		MYERROR;

	p = static_cast<LPWSTR>(LockResource(hglMenuTemplate));

	if (!p)
		MYERROR;

	//struct MenuHeader {
	//  WORD   wVersion;           // Currently zero
	//  WORD   cbHeaderSize;       // Also zero
	//};

	// MENUEX resource
	//struct MenuExHeader {
	//    WORD wVersion;           // One
	//    WORD wOffset;
	//    DWORD dwHelpId;
	//};
	p0 = reinterpret_cast<WORD*>(p);
	version = GET_WORD(p);

	p++;

	switch (version)
	{
	case 0:
		{
			offset = GET_WORD(p);
			p += offset;
			p++;
			size_t nMem = 0;
			if (!CountMemReplaceMenuResource(reinterpret_cast<WORD*>(p), &nMem, nullptr))
				goto DONE_ERROR;
			WORD * newMenu = new WORD[nMem + (nMem % 2)+2];
			SecureZeroMemory(newMenu, (nMem + (nMem % 2)+2)*2);
			size_t index = 2;       // MenuHeader has 2 WORDs zero
			if (!CountMemReplaceMenuResource(reinterpret_cast<WORD*>(p), &index, newMenu))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newMenu, static_cast<DWORD>(nMem + (nMem % 2)+2)*2))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, wLanguage, nullptr, 0)))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}
			delete [] newMenu;
		}
		break;
	case 1:
		{
			offset = GET_WORD(p);
			p++;
			//dwHelpId = GET_DWORD(p);
			size_t nMem = 0;
			if (!CountMemReplaceMenuExResource(reinterpret_cast<WORD*>(p0 + offset), &nMem, nullptr))
				goto DONE_ERROR;
			WORD * newMenu = new WORD[nMem + (nMem % 2) + 4];
			SecureZeroMemory(newMenu, (nMem + (nMem % 2) + 4) * 2);
			CopyMemory(newMenu, p0, 2 * sizeof(WORD) + sizeof(DWORD));
			size_t index = 4;       // MenuExHeader has 2 x WORD + 1 x DWORD
			if (!CountMemReplaceMenuExResource(reinterpret_cast<WORD*>(p0 + offset), &index, newMenu))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newMenu, static_cast<DWORD>(nMem + (nMem % 2) + 4) * 2))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_MENU, lpszType, wLanguage, nullptr, 0)))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}
			delete [] newMenu;
		}
		break;
	default:
		goto DONE_ERROR;
	}

	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	return TRUE;

DONE_ERROR:
	UnlockResource(hglMenuTemplate);
	FreeResource(hglMenuTemplate);
	MYERROR;
}

const WORD* CResModule::ParseMenuResource(const WORD * res)
{
	WORD        flags;
	WORD        id = 0;

	//struct PopupMenuItem {
	//  WORD   fItemFlags;
	//  WCHAR  szItemText[];
	//};
	//struct NormalMenuItem {
	//  WORD   fItemFlags;
	//  WORD   wMenuID;
	//  WCHAR  szItemText[];
	//};

	do
	{
		flags = GET_WORD(res);
		res++;
		if (!(flags & MF_POPUP))
		{
			id = GET_WORD(res); //normal menu item
			res++;
		}
		else
			id = static_cast<WORD>(-1); //popup menu item

		auto str = reinterpret_cast<LPCWSTR>(res);
		size_t l = wcslen(str)+1;
		res += l;

		if (flags & MF_POPUP)
		{
			wchar_t buf[MAX_STRING_LENGTH] = { 0 };
			wcscpy_s(buf, str);
			CUtils::StringExtend(buf);

			std::wstring wstr = std::wstring(buf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			if (id)
				InsertResourceIDs(RT_MENU, 0, entry, id, L" - PopupMenu");

			m_StringEntries[wstr] = entry;

			if ((res = ParseMenuResource(res))==0)
				return nullptr;
		}
		else if (id != 0)
		{
			wchar_t buf[MAX_STRING_LENGTH] = { 0 };
			wcscpy_s(buf, str);
			CUtils::StringExtend(buf);

			std::wstring wstr = std::wstring(buf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			InsertResourceIDs(RT_MENU, 0, entry, id, L" - Menu");

			TCHAR szTempBuf[1024] = { 0 };
			swprintf_s(szTempBuf, L"#: MenuEntry; ID:%u", id);
			MENUENTRY menu_entry;
			menu_entry.wID = id;
			menu_entry.reference = szTempBuf;
			menu_entry.msgstr = wstr;

			m_StringEntries[wstr] = entry;
			m_MenuEntries[id] = menu_entry;
		}
	} while (!(flags & MF_END));
	return res;
}

const WORD* CResModule::CountMemReplaceMenuResource(const WORD * res, size_t * wordcount, WORD * newMenu)
{
	WORD        flags;
	WORD        id = 0;

	//struct PopupMenuItem {
	//  WORD   fItemFlags;
	//  WCHAR  szItemText[];
	//};
	//struct NormalMenuItem {
	//  WORD   fItemFlags;
	//  WORD   wMenuID;
	//  WCHAR  szItemText[];
	//};

	do
	{
		flags = GET_WORD(res);
		res++;
		if (!newMenu)
			(*wordcount)++;
		else
			newMenu[(*wordcount)++] = flags;
		if (!(flags & MF_POPUP))
		{
			id = GET_WORD(res); //normal menu item
			res++;
			if (!newMenu)
				(*wordcount)++;
			else
				newMenu[(*wordcount)++] = id;
		}
		else
			id = static_cast<WORD>(-1); //popup menu item

		if (flags & MF_POPUP)
		{
			ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;

			if ((res = CountMemReplaceMenuResource(res, wordcount, newMenu))==0)
				return nullptr;
		}
		else if (id != 0)
		{
			ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		}
		else
		{
			if (newMenu)
				wcscpy(reinterpret_cast<wchar_t*>(&newMenu[(*wordcount)]), reinterpret_cast<LPCWSTR>(res));
			(*wordcount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
			res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		}
	} while (!(flags & MF_END));
	return res;
}

const WORD* CResModule::ParseMenuExResource(const WORD * res)
{
	WORD bResInfo;

	//struct MenuExItem {
	//    DWORD dwType;
	//    DWORD dwState;
	//    DWORD menuId;
	//    WORD bResInfo;
	//    WCHAR szText[];
	//    DWORD dwHelpId; - Popup menu only
	//};

	do
	{
		DWORD dwType = GET_DWORD(res);
		res += 2;
		//dwState = GET_DWORD(res);
		res += 2;
		DWORD menuId = GET_DWORD(res);
		res += 2;
		bResInfo = GET_WORD(res);
		res++;

		auto str = reinterpret_cast<LPCWSTR>(res);
		size_t l = wcslen(str)+1;
		res += l;
		// Align to DWORD boundary
		res = AlignWORD(res);

		if (dwType & MFT_SEPARATOR)
			continue;

		if (bResInfo & 0x01)
		{
			// Popup menu - note this can also have a non-zero ID
			if (menuId == 0)
				menuId = static_cast<WORD>(-1);
			wchar_t buf[MAX_STRING_LENGTH] = { 0 };
			wcscpy_s(buf, str);
			CUtils::StringExtend(buf);

			std::wstring wstr = std::wstring(buf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			// Popup has a DWORD help entry on a DWORD boundary - skip over it
			res += 2;

			InsertResourceIDs(RT_MENU, 0, entry, menuId, L" - PopupMenuEx");
			TCHAR szTempBuf[1024] = { 0 };
			swprintf_s(szTempBuf, L"#: MenuExPopupEntry; ID:%lu", menuId);
			MENUENTRY menu_entry;
			menu_entry.wID = static_cast<WORD>(menuId);
			menu_entry.reference = szTempBuf;
			menu_entry.msgstr = wstr;
			m_StringEntries[wstr] = entry;
			m_MenuEntries[static_cast<WORD>(menuId)] = menu_entry;

			if ((res = ParseMenuExResource(res)) == 0)
				return nullptr;
		} else if (menuId != 0)
		{
			wchar_t buf[MAX_STRING_LENGTH] = { 0 };
			wcscpy_s(buf, str);
			CUtils::StringExtend(buf);

			std::wstring wstr = std::wstring(buf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			InsertResourceIDs(RT_MENU, 0, entry, menuId, L" - MenuEx");

			TCHAR szTempBuf[1024] = { 0 };
			swprintf_s(szTempBuf, L"#: MenuExEntry; ID:%lu", menuId);
			MENUENTRY menu_entry;
			menu_entry.wID = static_cast<WORD>(menuId);
			menu_entry.reference = szTempBuf;
			menu_entry.msgstr = wstr;
			m_StringEntries[wstr] = entry;
			m_MenuEntries[static_cast<WORD>(menuId)] = menu_entry;
		}
	} while (!(bResInfo & 0x80));
	return res;
}

const WORD* CResModule::CountMemReplaceMenuExResource(const WORD * res, size_t * wordcount, WORD * newMenu)
{
	WORD bResInfo;

	//struct MenuExItem {
	//    DWORD dwType;
	//    DWORD dwState;
	//    DWORD menuId;
	//    WORD bResInfo;
	//    WCHAR szText[];
	//    DWORD dwHelpId; - Popup menu only
	//};

	do
	{
		auto p0 = const_cast<WORD*>(res);
		DWORD dwType = GET_DWORD(res);
		res += 2;
		//dwState = GET_DWORD(res);
		res += 2;
		DWORD menuId = GET_DWORD(res);
		res += 2;
		bResInfo = GET_WORD(res);
		res++;

		if (newMenu)
			CopyMemory(&newMenu[*wordcount], p0, 7 * sizeof(WORD));

		(*wordcount) += 7;

		if (dwType & MFT_SEPARATOR) {
			// Align to DWORD
			(*wordcount)++;
			res++;
			continue;
		}

		if (bResInfo & 0x01)
		{
			ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
			// Align to DWORD
			res = AlignWORD(res);
			if ((*wordcount) & 0x01)
				(*wordcount)++;

			if (newMenu)
				CopyMemory(&newMenu[*wordcount], res, sizeof(DWORD));  // Copy Help ID

			res += 2;
			(*wordcount) += 2;

			if ((res = CountMemReplaceMenuExResource(res, wordcount, newMenu)) == 0)
				return nullptr;
		}
		else if (menuId != 0)
		{
			ReplaceStr(reinterpret_cast<LPCWSTR>(res), newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		}
		else
		{
			if (newMenu)
				wcscpy(reinterpret_cast<wchar_t*>(&newMenu[(*wordcount)]), reinterpret_cast<LPCWSTR>(res));
			(*wordcount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
			res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		}
		// Align to DWORD
		res = AlignWORD(res);
		if ((*wordcount) & 0x01)
			(*wordcount)++;
	} while (!(bResInfo & 0x80));
	return res;
}

BOOL CResModule::ExtractAccelerator(LPCTSTR lpszType)
{
	HRSRC       hrsrc = FindResource(m_hResDll, lpszType, RT_ACCELERATOR);
	HGLOBAL     hglAccTable;
	WORD        fFlags, wAnsi, wID;
	const WORD* p;
	bool        bEnd(false);

	if (!hrsrc)
		MYERROR;

	hglAccTable = LoadResource(m_hResDll, hrsrc);

	if (!hglAccTable)
		goto DONE_ERROR;

	p = static_cast<const WORD*>(LockResource(hglAccTable));

	if (!p)
		MYERROR;

	/*
	struct ACCELTABLEENTRY
	{
		WORD fFlags;        FVIRTKEY, FSHIFT, FCONTROL, FALT, 0x80 - Last in a table
		WORD wAnsi;         ANSI character
		WORD wId;           Keyboard accelerator passed to windows
		WORD padding;       # bytes added to ensure aligned to DWORD boundary
	};
	*/

	do
	{
		fFlags = GET_WORD(p);
		p++;
		wAnsi = GET_WORD(p);
		p++;
		wID = GET_WORD(p);
		p++;
		p++;  // Skip over padding

		if ((fFlags & 0x80) == 0x80)
		{               // 0x80
			bEnd = true;
		}

		if ((wAnsi < 0x30) ||
			(wAnsi > 0x5A) ||
			(wAnsi >= 0x3A && wAnsi <= 0x40))
			continue;

		wchar_t buf[1024] = { 0 };
		wchar_t buf2[1024] = { 0 };

		// include the menu ID in the msgid to make sure that 'duplicate'
		// accelerator keys are listed in the po-file.
		// without this, we would get entries like this:
		//#. Accelerator Entry for Menu ID:32809; '&Filter'
		//#. Accelerator Entry for Menu ID:57636; '&Find'
		//#: Corresponding Menu ID:32771; '&Find'
		//msgid "V C +F"
		//msgstr ""
		//
		// Since "filter" and "find" are most likely translated to words starting
		// with different letters, we need to have a separate accelerator entry
		// for each of those
		swprintf_s(buf, L"ID:%u:", wID);

		// EXACTLY 5 characters long "ACS+X"
		// V = Virtual key (or blank if not used)
		// A = Alt key     (or blank if not used)
		// C = Ctrl key    (or blank if not used)
		// S = Shift key   (or blank if not used)
		// X = upper case character
		// e.g. "V CS+Q" == Ctrl + Shift + 'Q'
		if ((fFlags & FVIRTKEY) == FVIRTKEY)        // 0x01
			wcscat_s(buf, L"V");
		else
			wcscat_s(buf, L" ");

		if ((fFlags & FALT) == FALT)                // 0x10
			wcscat_s(buf, L"A");
		else
			wcscat_s(buf, L" ");

		if ((fFlags & FCONTROL) == FCONTROL)        // 0x08
			wcscat_s(buf, L"C");
		else
			wcscat_s(buf, L" ");

		if ((fFlags & FSHIFT) == FSHIFT)            // 0x04
			wcscat_s(buf, L"S");
		else
			wcscat_s(buf, L" ");

		swprintf_s(buf2, L"%s+%c", buf, wAnsi);

		std::wstring wstr = std::wstring(buf2);
		RESOURCEENTRY AKey_entry = m_StringEntries[wstr];

		TCHAR szTempBuf[1024] = { 0 };
		std::wstring wmenu;
		pME_iter = m_MenuEntries.find(wID);
		if (pME_iter != m_MenuEntries.end())
		{
			wmenu = pME_iter->second.msgstr;
		}
		swprintf_s(szTempBuf, L"#. Accelerator Entry for Menu ID:%u; '%s'", wID, wmenu.c_str());
		AKey_entry.automaticcomments.push_back(std::wstring(szTempBuf));

		m_StringEntries[wstr] = AKey_entry;
	} while (!bEnd);

	UnlockResource(hglAccTable);
	FreeResource(hglAccTable);
	return TRUE;

DONE_ERROR:
	UnlockResource(hglAccTable);
	FreeResource(hglAccTable);
	MYERROR;
}

BOOL CResModule::ReplaceAccelerator(LPCTSTR lpszType, WORD wLanguage)
{
	LPACCEL     lpaccelNew;         // pointer to new accelerator table
	HACCEL      haccelOld;          // handle to old accelerator table
	int         cAccelerators;      // number of accelerators in table
	HGLOBAL     hglAccTableNew;
	const WORD* p;
	int         i;

	haccelOld = LoadAccelerators(m_hResDll, lpszType);

	if (!haccelOld)
		MYERROR;

	cAccelerators = CopyAcceleratorTable(haccelOld, nullptr, 0);

	lpaccelNew = static_cast<LPACCEL>(LocalAlloc(LPTR, cAccelerators * sizeof(ACCEL)));

	if (!lpaccelNew)
		MYERROR;

	CopyAcceleratorTable(haccelOld, lpaccelNew, cAccelerators);

	// Find the accelerator that the user modified
	// and change its flags and virtual-key code
	// as appropriate.

	for (i = 0; i < cAccelerators; i++)
	{
		if ((lpaccelNew[i].key < 0x30) ||
			(lpaccelNew[i].key > 0x5A) ||
			(lpaccelNew[i].key >= 0x3A && lpaccelNew[i].key <= 0x40))
			continue;

		BYTE xfVirt;
		wchar_t xkey = { 0 };
		wchar_t buf[1024] = { 0 };
		wchar_t buf2[1024] = { 0 };

		swprintf_s(buf, L"ID:%d:", lpaccelNew[i].cmd);

		// get original key combination
		if ((lpaccelNew[i].fVirt & FVIRTKEY) == FVIRTKEY)       // 0x01
			wcscat_s(buf, L"V");
		else
			wcscat_s(buf, L" ");

		if ((lpaccelNew[i].fVirt & FALT) == FALT)               // 0x10
			wcscat_s(buf, L"A");
		else
			wcscat_s(buf, L" ");

		if ((lpaccelNew[i].fVirt & FCONTROL) == FCONTROL)       // 0x08
			wcscat_s(buf, L"C");
		else
			wcscat_s(buf, L" ");

		if ((lpaccelNew[i].fVirt & FSHIFT) == FSHIFT)           // 0x04
			wcscat_s(buf, L"S");
		else
			wcscat_s(buf, L" ");

		swprintf_s(buf2, L"%s+%c", buf, lpaccelNew[i].key);

		// Is it there?
		std::map<std::wstring, RESOURCEENTRY>::iterator pAK_iter = m_StringEntries.find(buf2);
		if (pAK_iter != m_StringEntries.end())
		{
			m_bTranslatedAcceleratorStrings++;
			xfVirt = 0;
			xkey = 0;
			std::wstring wtemp = pAK_iter->second.msgstr;
			wtemp = wtemp.substr(wtemp.find_last_of(':')+1);
			if (wtemp.size() != 6)
				continue;
			if (wtemp.compare(0, 1, L"V") == 0)
				xfVirt |= FVIRTKEY;
			else if (wtemp.compare(0, 1, L" ") != 0)
				continue;   // not a space - user must have made a mistake when translating
			if (wtemp.compare(1, 1, L"A") == 0)
				xfVirt |= FALT;
			else if (wtemp.compare(1, 1, L" ") != 0)
				continue;   // not a space - user must have made a mistake when translating
			if (wtemp.compare(2, 1, L"C") == 0)
				xfVirt |= FCONTROL;
			else if (wtemp.compare(2, 1, L" ") != 0)
				continue;   // not a space - user must have made a mistake when translating
			if (wtemp.compare(3, 1, L"S") == 0)
				xfVirt |= FSHIFT;
			else if (wtemp.compare(3, 1, L" ") != 0)
				continue;   // not a space - user must have made a mistake when translating
			if (wtemp.compare(4, 1, L"+") == 0)
			{
				swscanf_s(wtemp.substr(5, 1).c_str(), L"%c", &xkey, 1);
				lpaccelNew[i].fVirt = xfVirt;
				lpaccelNew[i].key = static_cast<DWORD>(xkey);
			}
		}
		else
			m_bDefaultAcceleratorStrings++;
	}


	// Create the new accelerator table
	hglAccTableNew = LocalAlloc(LPTR, cAccelerators * 4 * sizeof(WORD));
	if (!hglAccTableNew)
		goto DONE_ERROR;
	p = static_cast<WORD*>(hglAccTableNew);
	lpaccelNew[cAccelerators-1].fVirt |= 0x80;
	for (i = 0; i < cAccelerators; i++)
	{
		memcpy(reinterpret_cast<void*>(const_cast<WORD*>(p)), &lpaccelNew[i].fVirt, 1);
		p++;
		memcpy(reinterpret_cast<void*>(const_cast<WORD*>(p)), &lpaccelNew[i].key, sizeof(WORD));
		p++;
		memcpy(reinterpret_cast<void*>(const_cast<WORD*>(p)), &lpaccelNew[i].cmd, sizeof(WORD));
		p++;
		p++;
	}

	if (!UpdateResource(m_hUpdateRes, RT_ACCELERATOR, lpszType,
		(m_wTargetLang ? m_wTargetLang : wLanguage), hglAccTableNew /* haccelNew*/, cAccelerators * 4 * sizeof(WORD)))
	{
		goto DONE_ERROR;
	}

	if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_ACCELERATOR, lpszType, wLanguage, nullptr, 0)))
	{
		goto DONE_ERROR;
	}

	LocalFree(hglAccTableNew);
	LocalFree(lpaccelNew);
	return TRUE;

DONE_ERROR:
	LocalFree(hglAccTableNew);
	LocalFree(lpaccelNew);
	MYERROR;
}

BOOL CResModule::ExtractDialog(LPCTSTR lpszType)
{
	const WORD* lpDlg;
	const WORD* lpDlgItem;
	DIALOGINFO  dlg;
	DLGITEMINFO dlgItem;
	WORD        bNumControls;
	HRSRC       hrsrc;
	HGLOBAL     hGlblDlgTemplate;

	hrsrc = FindResource(m_hResDll, lpszType, RT_DIALOG);

	if (!hrsrc)
		MYERROR;

	hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);
	if (!hGlblDlgTemplate)
		MYERROR;

	lpDlg = static_cast<const WORD*>(LockResource(hGlblDlgTemplate));

	if (!lpDlg)
		MYERROR;

	lpDlgItem = GetDialogInfo(lpDlg, &dlg);
	bNumControls = dlg.nbItems;

	if (dlg.caption)
	{
		wchar_t buf[MAX_STRING_LENGTH] = { 0 };
		wcscpy_s(buf, dlg.caption);
		CUtils::StringExtend(buf);

		std::wstring wstr = std::wstring(buf);
		RESOURCEENTRY entry = m_StringEntries[wstr];
		InsertResourceIDs(RT_DIALOG, reinterpret_cast<INT_PTR>(lpszType), entry, reinterpret_cast<INT_PTR>(lpszType), L"");

		m_StringEntries[wstr] = entry;
	}

	while (bNumControls-- != 0)
	{
		TCHAR szTitle[500] = { 0 };
		BOOL  bCode;

		lpDlgItem = GetControlInfo(lpDlgItem, &dlgItem, dlg.dialogEx, &bCode);

		if (bCode == FALSE)
			wcsncpy_s(szTitle, dlgItem.windowName, _countof(szTitle) - 1);

		if (szTitle[0])
		{
			CUtils::StringExtend(szTitle);

			std::wstring wstr = std::wstring(szTitle);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			InsertResourceIDs(RT_DIALOG, reinterpret_cast<INT_PTR>(lpszType), entry, dlgItem.id, L"");

			m_StringEntries[wstr] = entry;
		}
	}

	UnlockResource(hGlblDlgTemplate);
	FreeResource(hGlblDlgTemplate);
	return (TRUE);
}

BOOL CResModule::ReplaceDialog(LPCTSTR lpszType, WORD wLanguage)
{
	const WORD* lpDlg;
	HRSRC       hrsrc;
	HGLOBAL     hGlblDlgTemplate;

	hrsrc = FindResourceEx(m_hResDll, RT_DIALOG, lpszType, wLanguage);

	if (!hrsrc)
		MYERROR;

	hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);

	if (!hGlblDlgTemplate)
		MYERROR;

	lpDlg = static_cast<WORD*>(LockResource(hGlblDlgTemplate));

	if (!lpDlg)
		MYERROR;

	size_t nMem = 0;
	const WORD * p = lpDlg;
	if (!CountMemReplaceDialogResource(p, &nMem, nullptr))
		goto DONE_ERROR;
	WORD * newDialog = new WORD[nMem + (nMem % 2)];
	SecureZeroMemory(newDialog, (nMem + (nMem % 2))*2);

	size_t index = 0;
	if (!CountMemReplaceDialogResource(lpDlg, &index, newDialog))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}

	if (!UpdateResource(m_hUpdateRes, RT_DIALOG, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), newDialog, static_cast<DWORD>(nMem + (nMem % 2))*2))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}

	if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_DIALOG, lpszType, wLanguage, nullptr, 0)))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}

	delete [] newDialog;
	UnlockResource(hGlblDlgTemplate);
	FreeResource(hGlblDlgTemplate);
	return TRUE;

DONE_ERROR:
	UnlockResource(hGlblDlgTemplate);
	FreeResource(hGlblDlgTemplate);
	MYERROR;
}

const WORD* CResModule::GetDialogInfo(const WORD * pTemplate, LPDIALOGINFO lpDlgInfo) const
{
	const WORD* p = pTemplate;

	lpDlgInfo->style = GET_DWORD(p);
	p += 2;

	if (lpDlgInfo->style == 0xffff0001) // DIALOGEX resource
	{
		lpDlgInfo->dialogEx = TRUE;
		lpDlgInfo->helpId   = GET_DWORD(p);
		p += 2;
		lpDlgInfo->exStyle  = GET_DWORD(p);
		p += 2;
		lpDlgInfo->style    = GET_DWORD(p);
		p += 2;
	}
	else
	{
		lpDlgInfo->dialogEx = FALSE;
		lpDlgInfo->helpId   = 0;
		lpDlgInfo->exStyle  = GET_DWORD(p);
		p += 2;
	}

	lpDlgInfo->nbItems = GET_WORD(p);
	p++;

	lpDlgInfo->x = GET_WORD(p);
	p++;

	lpDlgInfo->y = GET_WORD(p);
	p++;

	lpDlgInfo->cx = GET_WORD(p);
	p++;

	lpDlgInfo->cy = GET_WORD(p);
	p++;

	// Get the menu name

	switch (GET_WORD(p))
	{
	case 0x0000:
		lpDlgInfo->menuName = nullptr;
		p++;
		break;
	case 0xffff:
		lpDlgInfo->menuName = reinterpret_cast<LPCTSTR>(static_cast<WORD>(GET_WORD(p + 1)));
		p += 2;
		break;
	default:
		lpDlgInfo->menuName = reinterpret_cast<LPCTSTR>(p);
		p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
		break;
	}

	// Get the class name

	switch (GET_WORD(p))
	{
	case 0x0000:
		lpDlgInfo->className = static_cast<LPCTSTR>(MAKEINTATOM(32770));
		p++;
		break;
	case 0xffff:
		lpDlgInfo->className = reinterpret_cast<LPCTSTR>(static_cast<WORD>(GET_WORD(p + 1)));
		p += 2;
		break;
	default:
		lpDlgInfo->className = reinterpret_cast<LPCTSTR>(p);
		p += wcslen(reinterpret_cast<LPCTSTR>(p)) + 1;
		break;
	}

	// Get the window caption

	lpDlgInfo->caption = reinterpret_cast<LPCTSTR>(p);
	p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;

	// Get the font name

	if (lpDlgInfo->style & DS_SETFONT)
	{
		lpDlgInfo->pointSize = GET_WORD(p);
		p++;

		if (lpDlgInfo->dialogEx)
		{
			lpDlgInfo->weight = GET_WORD(p);
			p++;
			lpDlgInfo->italic = LOBYTE(GET_WORD(p));
			p++;
		}
		else
		{
			lpDlgInfo->weight = FW_DONTCARE;
			lpDlgInfo->italic = FALSE;
		}

		lpDlgInfo->faceName = reinterpret_cast<LPCTSTR>(p);
		p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
	}
	// First control is on DWORD boundary
	p = AlignWORD(p);

	return p;
}

const WORD* CResModule::GetControlInfo(const WORD* p, LPDLGITEMINFO lpDlgItemInfo, BOOL dialogEx, LPBOOL bIsID) const
{
	if (dialogEx)
	{
		lpDlgItemInfo->helpId = GET_DWORD(p);
		p += 2;
		lpDlgItemInfo->exStyle = GET_DWORD(p);
		p += 2;
		lpDlgItemInfo->style = GET_DWORD(p);
		p += 2;
	}
	else
	{
		lpDlgItemInfo->helpId = 0;
		lpDlgItemInfo->style = GET_DWORD(p);
		p += 2;
		lpDlgItemInfo->exStyle = GET_DWORD(p);
		p += 2;
	}

	lpDlgItemInfo->x = GET_WORD(p);
	p++;

	lpDlgItemInfo->y = GET_WORD(p);
	p++;

	lpDlgItemInfo->cx = GET_WORD(p);
	p++;

	lpDlgItemInfo->cy = GET_WORD(p);
	p++;

	if (dialogEx)
	{
		// ID is a DWORD for DIALOGEX
		lpDlgItemInfo->id = static_cast<WORD>(GET_DWORD(p));
		p += 2;
	}
	else
	{
		lpDlgItemInfo->id = GET_WORD(p);
		p++;
	}

	if (GET_WORD(p) == 0xffff)
	{
		GET_WORD(p + 1);

		p += 2;
	}
	else
	{
		lpDlgItemInfo->className = reinterpret_cast<LPCTSTR>(p);
		p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
	}

	if (GET_WORD(p) == 0xffff)  // an integer ID?
	{
		*bIsID = TRUE;
		lpDlgItemInfo->windowName = reinterpret_cast<LPCTSTR>(GET_WORD(p + 1));
		p += 2;
	}
	else
	{
		*bIsID = FALSE;
		lpDlgItemInfo->windowName = reinterpret_cast<LPCTSTR>(p);
		p += wcslen(reinterpret_cast<LPCWSTR>(p)) + 1;
	}

	if (GET_WORD(p))
	{
		lpDlgItemInfo->data = const_cast<WORD*>(p + 1);
		p += GET_WORD(p) / sizeof(WORD);
	}
	else
		lpDlgItemInfo->data = nullptr;

	p++;
	// Next control is on DWORD boundary
	p = AlignWORD(p);
	return p;
}

const WORD * CResModule::CountMemReplaceDialogResource(const WORD * res, size_t * wordcount, WORD * newDialog)
{
	BOOL bEx = FALSE;
	DWORD style = GET_DWORD(res);
	if (newDialog)
	{
		newDialog[(*wordcount)++] = GET_WORD(res++);
		newDialog[(*wordcount)++] = GET_WORD(res++);
	}
	else
	{
		res += 2;
		(*wordcount) += 2;
	}

	if (style == 0xffff0001)    // DIALOGEX resource
	{
		bEx = TRUE;
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);    //help id
			newDialog[(*wordcount)++] = GET_WORD(res++);    //help id
			newDialog[(*wordcount)++] = GET_WORD(res++);    //exStyle
			newDialog[(*wordcount)++] = GET_WORD(res++);    //exStyle
			style = GET_DWORD(res);
			newDialog[(*wordcount)++] = GET_WORD(res++);    //style
			newDialog[(*wordcount)++] = GET_WORD(res++);    //style
		}
		else
		{
			res += 4;
			style = GET_DWORD(res);
			res += 2;
			(*wordcount) += 6;
		}
	}
	else
	{
		bEx = FALSE;
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);    //exStyle
			newDialog[(*wordcount)++] = GET_WORD(res++);    //exStyle
			//style = GET_DWORD(res);
			//newDialog[(*wordcount)++] = GET_WORD(res++);  //style
			//newDialog[(*wordcount)++] = GET_WORD(res++);  //style
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);
	WORD nbItems = GET_WORD(res);
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //x
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //y
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //cx
	(*wordcount)++;
	res++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res); //cy
	(*wordcount)++;
	res++;

	// Get the menu name

	switch (GET_WORD(res))
	{
	case 0x0000:
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		(*wordcount)++;
		res++;
		break;
	case 0xffff:
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			(*wordcount) += 2;
			res += 2;
		}
		break;
	default:
		if (newDialog)
		{
			wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordcount)]), reinterpret_cast<LPCWSTR>(res));
		}
		(*wordcount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		break;
	}

	// Get the class name

	switch (GET_WORD(res))
	{
	case 0x0000:
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		(*wordcount)++;
		res++;
		break;
	case 0xffff:
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			(*wordcount) += 2;
			res += 2;
		}
		break;
	default:
		if (newDialog)
		{
			wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordcount)]), reinterpret_cast<LPCWSTR>(res));
		}
		(*wordcount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		break;
	}

	// Get the window caption

	ReplaceStr(reinterpret_cast<LPCWSTR>(res), newDialog, wordcount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
	res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;

	// Get the font name

	if (style & DS_SETFONT)
	{
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		res++;
		(*wordcount)++;

		if (bEx)
		{
			if (newDialog)
			{
				newDialog[(*wordcount)++] = GET_WORD(res++);
				newDialog[(*wordcount)++] = GET_WORD(res++);
			}
			else
			{
				res += 2;
				(*wordcount) += 2;
			}
		}

		if (newDialog)
			wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordcount)]), reinterpret_cast<LPCWSTR>(res));
		(*wordcount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
	}
	// First control is on DWORD boundary
	while ((*wordcount)%2)
		(*wordcount)++;
	while (reinterpret_cast<UINT_PTR>(res) % 4)
		res++;

	while (nbItems--)
	{
		res = ReplaceControlInfo(res, wordcount, newDialog, bEx);
	}
	return res;
}

const WORD* CResModule::ReplaceControlInfo(const WORD * res, size_t * wordcount, WORD * newDialog, BOOL bEx)
{
	if (bEx)
	{
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);    //helpid
			newDialog[(*wordcount)++] = GET_WORD(res++);    //helpid
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	if (newDialog)
	{
		auto exStyle = reinterpret_cast<LONG*>(&newDialog[(*wordcount)]);
		newDialog[(*wordcount)++] = GET_WORD(res++);    //exStyle
		newDialog[(*wordcount)++] = GET_WORD(res++);    //exStyle
		if (m_bRTL)
			*exStyle |= WS_EX_RTLREADING;
	}
	else
	{
		res += 2;
		(*wordcount) += 2;
	}

	if (newDialog)
	{
		newDialog[(*wordcount)++] = GET_WORD(res++);    //style
		newDialog[(*wordcount)++] = GET_WORD(res++);    //style
	}
	else
	{
		res += 2;
		(*wordcount) += 2;
	}

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);    //x
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);    //y
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);    //cx
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);    //cy
	res++;
	(*wordcount)++;

	if (bEx)
	{
		// ID is a DWORD for DIALOGEX
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	else
	{
		if (newDialog)
			newDialog[(*wordcount)] = GET_WORD(res);
		res++;
		(*wordcount)++;
	}

	if (GET_WORD(res) == 0xffff)    //classID
	{
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	else
	{
		if (newDialog)
			wcscpy(reinterpret_cast<LPWSTR>(&newDialog[(*wordcount)]), reinterpret_cast<LPCWSTR>(res));
		(*wordcount) += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
		res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
	}

	if (GET_WORD(res) == 0xffff)    // an integer ID?
	{
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);
			newDialog[(*wordcount)++] = GET_WORD(res++);
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	else
	{
		ReplaceStr(reinterpret_cast<LPCWSTR>(res), newDialog, wordcount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
		res += wcslen(reinterpret_cast<LPCWSTR>(res)) + 1;
	}

	if (newDialog)
		memcpy(&newDialog[(*wordcount)], res, (GET_WORD(res)+1)*sizeof(WORD));
	(*wordcount) += (GET_WORD(res)+1);
	res += (GET_WORD(res)+1);
	// Next control is on DWORD boundary
	while ((*wordcount) % 2)
		(*wordcount)++;
	res = AlignWORD(res);

	return res;
}

BOOL CResModule::ExtractRibbon(LPCTSTR lpszType)
{
	HRSRC       hrsrc = FindResource(m_hResDll, lpszType, RT_RIBBON);
	HGLOBAL     hglRibbonTemplate;
	const BYTE *p;

	if (!hrsrc)
		MYERROR;

	hglRibbonTemplate = LoadResource(m_hResDll, hrsrc);

	DWORD sizeres = SizeofResource(m_hResDll, hrsrc);

	if (!hglRibbonTemplate)
		MYERROR;

	p = static_cast<const BYTE*>(LockResource(hglRibbonTemplate));

	if (!p)
		MYERROR;

	// Resource consists of one single string
	// that is XML.

	// extract all <id><name>blah1</name><value>blah2</value></id><text>blah</text> elements

	const std::regex regRevMatch("<ID><NAME>([^<]+)</NAME><VALUE>([^<]+)</VALUE></ID><TEXT>([^<]+)</TEXT>");
	std::string ss = std::string(reinterpret_cast<const char*>(p), sizeres);
	const std::sregex_iterator end;
	for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatch); it != end; ++it)
	{
		size_t len;

		std::string str1 = (*it)[1];
		len = str1.size();
		auto bufw1 = std::make_unique<wchar_t[]>(len * 4 + 1);
		SecureZeroMemory(bufw1.get(), (len * 4 + 1) * sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, str1.c_str(), -1, bufw1.get(), static_cast<int>(len) * 4);
		std::wstring strIdNameVal = bufw1.get();
		strIdNameVal += L" - Ribbon name";

		std::string str2 = (*it)[2];
		len = str2.size();
		auto bufw2 = std::make_unique<wchar_t[]>(len * 4 + 1);
		SecureZeroMemory(bufw2.get(), (len * 4 + 1)*sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, str2.c_str(), -1, bufw2.get(), static_cast<int>(len) * 4);
		std::wstring strIdVal = bufw2.get();

		std::string str3 = (*it)[3];
		len = str3.size();
		auto bufw3 = std::make_unique<wchar_t[]>(len * 4 + 1);
		SecureZeroMemory(bufw3.get(), (len * 4 + 1)*sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, str3.c_str(), -1, bufw3.get(), static_cast<int>(len) * 4);
		std::wstring str = bufw3.get();

		RESOURCEENTRY entry = m_StringEntries[str];
		InsertResourceIDs(RT_RIBBON, 0, entry, std::stoi(strIdVal), strIdNameVal.c_str());
		if (wcschr(str.c_str(), '%'))
			entry.flag = L"#, c-format";
		m_StringEntries[str] = entry;
		m_bDefaultRibbonTexts++;
	}

	// extract all </ELEMENT_NAME><NAME>blahblah</NAME> elements

	const std::regex regRevMatchName("</ELEMENT_NAME><NAME>([^<]+)</NAME>");
	for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatchName); it != end; ++it)
	{
		std::string str = (*it)[1];
		size_t len = str.size();
		auto bufw = std::make_unique<wchar_t[]>(len * 4 + 1);
		SecureZeroMemory(bufw.get(), (len*4 + 1)*sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufw.get(), static_cast<int>(len) * 4);
		std::wstring ret = bufw.get();
		RESOURCEENTRY entry = m_StringEntries[ret];
		InsertResourceIDs(RT_RIBBON, 0, entry, reinterpret_cast<INT_PTR>(lpszType), L" - Ribbon element");
		if (wcschr(ret.c_str(), '%'))
			entry.flag = L"#, c-format";
		m_StringEntries[ret] = entry;
		m_bDefaultRibbonTexts++;
	}

	UnlockResource(hglRibbonTemplate);
	FreeResource(hglRibbonTemplate);
	return TRUE;
}

BOOL CResModule::ReplaceRibbon(LPCTSTR lpszType, WORD wLanguage)
{
	HRSRC       hrsrc = FindResource(m_hResDll, lpszType, RT_RIBBON);
	HGLOBAL     hglRibbonTemplate;
	const BYTE *p;

	if (!hrsrc)
		MYERROR;

	hglRibbonTemplate = LoadResource(m_hResDll, hrsrc);

	DWORD sizeres = SizeofResource(m_hResDll, hrsrc);

	if (!hglRibbonTemplate)
		MYERROR;

	p = static_cast<const BYTE*>(LockResource(hglRibbonTemplate));

	if (!p)
		MYERROR;

	std::string ss = std::string(reinterpret_cast<const char*>(p), sizeres);
	size_t len = ss.size();
	auto bufw = std::make_unique<wchar_t[]>(len * 4 + 1);
	SecureZeroMemory(bufw.get(), (len*4 + 1)*sizeof(wchar_t));
	MultiByteToWideChar(CP_UTF8, 0, ss.c_str(), -1, bufw.get(), static_cast<int>(len) * 4);
	std::wstring ssw = bufw.get();


	const std::regex regRevMatch("<TEXT>([^<]+)</TEXT>");
	const std::sregex_iterator end;
	for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatch); it != end; ++it)
	{
		std::string str = (*it)[1];
		size_t slen = str.size();
		auto bufw2 = std::make_unique<wchar_t[]>(slen * 4 + 1);
		SecureZeroMemory(bufw2.get(), (slen*4 + 1)*sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufw2.get(), static_cast<int>(slen) * 4);
		std::wstring ret = bufw2.get();

		RESOURCEENTRY entry = m_StringEntries[ret];
		ret = L"<TEXT>" + ret + L"</TEXT>";

		if (entry.msgstr.size())
		{
			size_t buflen = entry.msgstr.size() + 10;
			auto sbuf = std::make_unique<wchar_t[]>(buflen);
			wcscpy_s(sbuf.get(), buflen, entry.msgstr.c_str());
			CUtils::StringCollapse(sbuf.get());
			ReplaceWithRegex(sbuf.get(), buflen);
			std::wstring sreplace = L"<TEXT>";
			sreplace += sbuf.get();
			sreplace += L"</TEXT>";
			CUtils::SearchReplace(ssw, ret, sreplace);
			m_bTranslatedRibbonTexts++;
		}
		else
			m_bDefaultRibbonTexts++;
	}

	const std::regex regRevMatchName("</ELEMENT_NAME><NAME>([^<]+)</NAME>");
	for (std::sregex_iterator it(ss.cbegin(), ss.cend(), regRevMatchName); it != end; ++it)
	{
		std::string str = (*it)[1];
		size_t slen = str.size();
		auto bufw2 = std::make_unique<wchar_t[]>(slen * 4 + 1);
		SecureZeroMemory(bufw2.get(), (slen*4 + 1)*sizeof(wchar_t));
		MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, bufw2.get(), static_cast<int>(slen) * 4);
		std::wstring ret = bufw2.get();

		RESOURCEENTRY entry = m_StringEntries[ret];
		ret = L"</ELEMENT_NAME><NAME>" + ret + L"</NAME>";

		if (entry.msgstr.size())
		{
			size_t buflen = entry.msgstr.size() + 10;
			auto sbuf = std::make_unique<wchar_t[]>(buflen);
			wcscpy_s(sbuf.get(), buflen, entry.msgstr.c_str());
			CUtils::StringCollapse(sbuf.get());
			ReplaceWithRegex(sbuf.get(), buflen);
			std::wstring sreplace = L"</ELEMENT_NAME><NAME>";
			sreplace += sbuf.get();
			sreplace += L"</NAME>";
			CUtils::SearchReplace(ssw, ret, sreplace);
			m_bTranslatedRibbonTexts++;
		}
		else
			m_bDefaultRibbonTexts++;
	}

	auto buf = std::make_unique<char[]>(ssw.size() * 4 + 1);
	int lengthIncTerminator = WideCharToMultiByte(CP_UTF8, 0, ssw.c_str(), -1, buf.get(), static_cast<int>(len) * 4, nullptr, nullptr);


	if (!UpdateResource(m_hUpdateRes, RT_RIBBON, lpszType, (m_wTargetLang ? m_wTargetLang : wLanguage), buf.get(), lengthIncTerminator-1))
	{
		goto DONE_ERROR;
	}

	if (m_wTargetLang && (!UpdateResource(m_hUpdateRes, RT_RIBBON, lpszType, wLanguage, nullptr, 0)))
	{
		goto DONE_ERROR;
	}


	UnlockResource(hglRibbonTemplate);
	FreeResource(hglRibbonTemplate);
	return TRUE;

DONE_ERROR:
	UnlockResource(hglRibbonTemplate);
	FreeResource(hglRibbonTemplate);
	MYERROR;
}

std::wstring CResModule::ReplaceWithRegex(WCHAR* pBuf, size_t bufferSize)
{
	for (const auto& t : m_StringEntries.m_regexes)
	{
		try
		{
			std::wregex e(std::get<0>(t), std::regex_constants::icase);
			auto replaced = std::regex_replace(pBuf, e, std::get<1>(t));
			wcscpy_s(pBuf, bufferSize, replaced.c_str());
		}
		catch (std::exception&)
		{
		}
	}
	return pBuf;
}

std::wstring CResModule::ReplaceWithRegex(std::wstring& s)
{
	for (const auto& t : m_StringEntries.m_regexes)
	{
		try
		{
			std::wregex e(std::get<0>(t), std::regex_constants::icase);
			auto replaced = std::regex_replace(s, e, std::get<1>(t));
			s = replaced;
		}
		catch (std::exception&)
		{
		}
	}
	return s;
}

BOOL CALLBACK CResModule::EnumResNameCallback(HMODULE /*hModule*/, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	auto lpResModule = reinterpret_cast<CResModule*>(lParam);

	if (lpszType == RT_STRING)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractString(lpszName))
				return FALSE;
		}
	}
	else if (lpszType == RT_MENU)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractMenu(lpszName))
				return FALSE;
		}
	}
	else if (lpszType == RT_DIALOG)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractDialog(lpszName))
				return FALSE;
		}
	}
	else if (lpszType == RT_ACCELERATOR)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractAccelerator(lpszName))
				return FALSE;
		}
	}
	else if (lpszType == RT_RIBBON)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractRibbon(lpszName))
				return FALSE;
		}
	}

	return TRUE;
}

BOOL CALLBACK CResModule::EnumResNameWriteCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	auto lpResModule = reinterpret_cast<CResModule*>(lParam);
	return EnumResourceLanguages(hModule, lpszType, lpszName, reinterpret_cast<ENUMRESLANGPROC>(&lpResModule->EnumResWriteLangCallback), lParam);
}

BOOL CALLBACK CResModule::EnumResWriteLangCallback(HMODULE /*hModule*/, LPCTSTR lpszType, LPTSTR lpszName, WORD wLanguage, LONG_PTR lParam)
{
	BOOL bRes = FALSE;
	auto lpResModule = reinterpret_cast<CResModule*>(lParam);

	if (lpszType == RT_STRING)
	{
		bRes = lpResModule->ReplaceString(lpszName, wLanguage);
	}
	else if (lpszType == RT_MENU)
	{
		bRes = lpResModule->ReplaceMenu(lpszName, wLanguage);
	}
	else if (lpszType == RT_DIALOG)
	{
		bRes = lpResModule->ReplaceDialog(lpszName, wLanguage);
	}
	else if (lpszType == RT_ACCELERATOR)
	{
		bRes = lpResModule->ReplaceAccelerator(lpszName, wLanguage);
	}
	else if (lpszType == RT_RIBBON)
	{
		bRes = lpResModule->ReplaceRibbon(lpszName, wLanguage);
	}

	return bRes;

}

void CResModule::ReplaceStr(LPCWSTR src, WORD * dest, size_t * count, int * translated, int * def)
{
	wchar_t buf[MAX_STRING_LENGTH] = { 0 };
	wcscpy_s(buf, src);
	CUtils::StringExtend(buf);

	std::wstring wstr = std::wstring(buf);
	ReplaceWithRegex(buf);
	RESOURCEENTRY entry = m_StringEntries[wstr];
	if (!entry.msgstr.empty())
	{
		wcscpy_s(buf, entry.msgstr.c_str());
		ReplaceWithRegex(buf);
		CUtils::StringCollapse(buf);
		if (dest)
			wcscpy(reinterpret_cast<wchar_t*>(&dest[(*count)]), buf);
		(*count) += wcslen(buf) + 1;
		(*translated)++;
	}
	else
	{
		if (wcscmp(buf, wstr.c_str()))
		{
			if (dest)
				wcscpy(reinterpret_cast<wchar_t*>(&dest[(*count)]), buf);
			(*count) += wcslen(buf) + 1;
			(*translated)++;
		}
		else
		{
			if (dest)
				wcscpy(reinterpret_cast<wchar_t*>(&dest[(*count)]), src);
			(*count) += wcslen(src) + 1;
			if (wcslen(src))
				(*def)++;
		}
	}
}

static bool StartsWith(const std::string& heystacl, const char* needle)
{
	return heystacl.compare(0, strlen(needle), needle) == 0;
}

static bool StartsWith(const std::wstring& heystacl, const wchar_t* needle)
{
	return heystacl.compare(0, wcslen(needle), needle) == 0;
}

size_t CResModule::ScanHeaderFile(const std::wstring & filepath)
{
	size_t count = 0;

	// open the file and read the contents
	DWORD reqLen = GetFullPathName(filepath.c_str(), 0, nullptr, nullptr);
	auto wcfullPath = std::make_unique<TCHAR[]>(reqLen + 1);
	GetFullPathName(filepath.c_str(), reqLen, wcfullPath.get(), nullptr);
	std::wstring fullpath = wcfullPath.get();


	// first treat the file as ASCII and try to get the defines
	{
		std::ifstream  fin(fullpath);
		std::string    file_line;
		while (std::getline(fin, file_line))
		{
			auto defpos = file_line.find("#define");
			if (defpos != std::string::npos)
			{
				std::string text = file_line.substr(defpos + 7);
				trim(text);
				auto spacepos = text.find(' ');
				if (spacepos == std::string::npos)
					spacepos = text.find('\t');
				if (spacepos != std::string::npos)
				{
					auto value = atol(text.substr(spacepos).c_str());
					if (value == 0 && text.substr(spacepos).find("0x") != std::string::npos)
						value = std::stoul(text.substr(spacepos), nullptr, 16);
					text = text.substr(0, spacepos);
					trim(text);
					if (StartsWith(text, "IDS_") || StartsWith(text, "AFX_IDS_") || StartsWith(text, "AFX_IDP_"))
					{
						m_currentHeaderDataStrings[value] = CUnicodeUtils::StdGetUnicode(text);
						++count;
					}
					else if (StartsWith(text, "IDD_") || StartsWith(text, "AFX_IDD_"))
					{
						m_currentHeaderDataDialogs[value] = CUnicodeUtils::StdGetUnicode(text);
						++count;
					}
					else if (StartsWith(text, "ID_") || StartsWith(text, "AFX_ID_"))
					{
						m_currentHeaderDataMenus[value] = CUnicodeUtils::StdGetUnicode(text);
						++count;
					}
					else if (StartsWith(text, "cmd"))
					{
						m_currentHeaderDataStrings[value] = CUnicodeUtils::StdGetUnicode(text);
						++count;
					}
					else if (text.find("_RESID") != std::string::npos)
					{
						m_currentHeaderDataStrings[value] = CUnicodeUtils::StdGetUnicode(text);
						++count;
					}
				}

			}
		}
	}

	// now try the same with the file treated as utf16
	{
		// open as a byte stream
		std::ifstream wfin(fullpath, std::ios::binary);
		// apply BOM-sensitive UTF-16 facet
		//std::wifstream wfin(fullpath);
		std::string file_line;
		while (std::getline(wfin, file_line))
		{
			auto defpos = file_line.find("#define");
			if (defpos != std::wstring::npos)
			{
				std::wstring text = CUnicodeUtils::StdGetUnicode(file_line.substr(defpos + 7));
				trim(text);
				auto spacepos = text.find(' ');
				if (spacepos == std::wstring::npos)
					spacepos = text.find('\t');
				if (spacepos != std::wstring::npos)
				{
					auto value = _wtol(text.substr(spacepos).c_str());
					if (value == 0 && text.substr(spacepos).find(L"0x") != std::wstring::npos)
						value = std::stoul(text.substr(spacepos), nullptr, 16);
					text = text.substr(0, spacepos);
					trim(text);
					if (StartsWith(text, L"IDS_") || StartsWith(text, L"AFX_IDS_") || StartsWith(text, L"AFX_IDP_"))
					{
						m_currentHeaderDataStrings[value] = text;
						++count;
					}
					else if (StartsWith(text, L"IDD_") || StartsWith(text, L"AFX_IDD_"))
					{
						m_currentHeaderDataDialogs[value] = text;
						++count;
					}
					else if (StartsWith(text, L"ID_") || StartsWith(text, L"AFX_ID_"))
					{
						m_currentHeaderDataMenus[value] = text;
						++count;
					}
					else if (StartsWith(text, L"cmd"))
					{
						m_currentHeaderDataStrings[value] = text;
						++count;
					}
					else if (text.find(L"_RESID") != std::string::npos)
					{
						m_currentHeaderDataStrings[value] = text;
						++count;
					}
				}
			}
		}
	}

	return count;
}

void CResModule::InsertResourceIDs(LPCWSTR lpType, INT_PTR mainId, RESOURCEENTRY& entry, INT_PTR id, LPCWSTR infotext)
{
	if (lpType == RT_DIALOG)
	{
		auto foundIt = m_currentHeaderDataDialogs.find(mainId);
		if (foundIt != m_currentHeaderDataDialogs.end())
			entry.resourceIDs.insert(L"Dialog " + foundIt->second + L": Control id " + NumToStr(id) + infotext);
		else
			entry.resourceIDs.insert(NumToStr(id) + infotext);
	}
	else if (lpType == RT_STRING)
	{
		auto foundIt = m_currentHeaderDataStrings.find(id);
		if (foundIt != m_currentHeaderDataStrings.end())
			entry.resourceIDs.insert(foundIt->second + infotext);
		else
			entry.resourceIDs.insert(NumToStr(id) + infotext);
	}
	else if (lpType == RT_MENU)
	{
		auto foundIt = m_currentHeaderDataMenus.find(id);
		if (foundIt != m_currentHeaderDataMenus.end())
			entry.resourceIDs.insert(foundIt->second + infotext);
		else
			entry.resourceIDs.insert(NumToStr(id) + infotext);
	}
	else if (lpType == RT_RIBBON && infotext && wcsstr(infotext, L"ID") == infotext)
			entry.resourceIDs.insert(infotext);
	else
		entry.resourceIDs.insert(NumToStr(id) + infotext);
}

bool CResModule::AdjustCheckSum(const std::wstring& resFile)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	HANDLE hFileMapping = nullptr;
	PVOID pBaseAddress = nullptr;

	try
	{
		hFile = CreateFile(resFile.c_str(), FILE_READ_DATA | FILE_WRITE_DATA, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (hFile == INVALID_HANDLE_VALUE)
			throw GetLastError();

		hFileMapping = CreateFileMapping(hFile, nullptr, PAGE_READWRITE, 0, 0, nullptr);
		if (!hFileMapping)
			throw GetLastError();

		pBaseAddress = MapViewOfFile(hFileMapping, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (!pBaseAddress)
			throw GetLastError();

		auto fileSize = GetFileSize(hFile, nullptr);
		if (fileSize == INVALID_FILE_SIZE)
			throw GetLastError();
		DWORD dwChecksum = 0;
		DWORD dwHeaderSum = 0;
		CheckSumMappedFile(pBaseAddress, fileSize, &dwHeaderSum, &dwChecksum);

		PIMAGE_DOS_HEADER pDOSHeader = static_cast<PIMAGE_DOS_HEADER>(pBaseAddress);
		if (pDOSHeader->e_magic != IMAGE_DOS_SIGNATURE)
			throw GetLastError();

		PIMAGE_NT_HEADERS pNTHeader = reinterpret_cast<PIMAGE_NT_HEADERS>(static_cast<PBYTE>(pBaseAddress) + pDOSHeader->e_lfanew);
		if (pNTHeader->Signature != IMAGE_NT_SIGNATURE)
			throw GetLastError();

		SetLastError(ERROR_SUCCESS);

		DWORD* pChecksum = &(pNTHeader->OptionalHeader.CheckSum);
		*pChecksum = dwChecksum;

		UnmapViewOfFile(pBaseAddress);
		CloseHandle(hFileMapping);
		CloseHandle(hFile);
	}
	catch (...)
	{
		if (pBaseAddress)
			UnmapViewOfFile(pBaseAddress);
		if (hFileMapping)
			CloseHandle(hFileMapping);
		if (hFile != INVALID_HANDLE_VALUE)
			CloseHandle(hFile);
		return false;
	}

	return true;
}
