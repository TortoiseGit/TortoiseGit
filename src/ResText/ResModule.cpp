// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2010-2012 - TortoiseSVN

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
#include "StdAfx.h"
#include "Utils.h"
#include ".\resmodule.h"
#include <memory>
#include "..\Utils\SysInfo.h"

#define MYERROR {CUtils::Error(); return FALSE;}

CResModule::CResModule(void)
	: m_bTranslatedStrings(0)
	, m_bDefaultStrings(0)
	, m_bTranslatedDialogStrings(0)
	, m_bDefaultDialogStrings(0)
	, m_bTranslatedMenuStrings(0)
	, m_bDefaultMenuStrings(0)
	, m_bTranslatedAcceleratorStrings(0)
	, m_bDefaultAcceleratorStrings(0)
	, m_wTargetLang(0)
	, m_hResDll(NULL)
	, m_hUpdateRes(NULL)
	, m_bQuiet(false)
	, m_bRTL(false)
	, m_bAdjustEOLs(false)
{
}

CResModule::~CResModule(void)
{
}

BOOL CResModule::ExtractResources(std::vector<std::wstring> filelist, LPCTSTR lpszPOFilePath, BOOL bNoUpdate, LPCTSTR lpszHeaderFile)
{
	for (std::vector<std::wstring>::iterator I = filelist.begin(); I != filelist.end(); ++I)
	{
		m_hResDll = LoadLibraryEx(I->c_str(), NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE|LOAD_LIBRARY_AS_DATAFILE);
		if (m_hResDll == NULL)
			MYERROR;

		size_t nEntries = m_StringEntries.size();
		// fill in the std::map with all translatable entries

		if (!m_bQuiet)
			_ftprintf(stdout, _T("Extracting StringTable...."));
		EnumResourceNames(m_hResDll, RT_STRING,  EnumResNameCallback, (LONG_PTR)this);
		if (!m_bQuiet)
			_ftprintf(stdout, _T("%4d Strings\n"), m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		if (!m_bQuiet)
			_ftprintf(stdout, _T("Extracting Dialogs........"));
		EnumResourceNames(m_hResDll, RT_DIALOG,  EnumResNameCallback, (LONG_PTR)this);
		if (!m_bQuiet)
			_ftprintf(stdout, _T("%4d Strings\n"), m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();

		if (!m_bQuiet)
			_ftprintf(stdout, _T("Extracting Menus.........."));
		EnumResourceNames(m_hResDll, RT_MENU,    EnumResNameCallback, (LONG_PTR)this);
		if (!m_bQuiet)
			_ftprintf(stdout, _T("%4d Strings\n"), m_StringEntries.size()-nEntries);
		nEntries = m_StringEntries.size();
		if (!m_bQuiet)
			_ftprintf(stdout, _T("Extracting Accelerators..."));
		EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameCallback, (LONG_PTR)this);
		if (!m_bQuiet)
			_ftprintf(stdout, _T("%4d Accelerators\n"), m_StringEntries.size()-nEntries);
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
	m_hResDll = LoadLibraryEx(lpszSrcLangDllPath, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE|LOAD_LIBRARY_AS_DATAFILE);
	if (m_hResDll == NULL)
		MYERROR;

	size_t nEntries = 0;
	// fill in the std::map with all translatable entries

	if (!m_bQuiet)
		_ftprintf(stdout, _T("Extracting StringTable...."));
	EnumResourceNames(m_hResDll, RT_STRING,  EnumResNameCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d Strings\n"), m_StringEntries.size());
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_ftprintf(stdout, _T("Extracting Dialogs........"));
	EnumResourceNames(m_hResDll, RT_DIALOG,  EnumResNameCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d Strings\n"), m_StringEntries.size()-nEntries);
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_ftprintf(stdout, _T("Extracting Menus.........."));
	EnumResourceNames(m_hResDll, RT_MENU,    EnumResNameCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d Strings\n"), m_StringEntries.size()-nEntries);
	nEntries = m_StringEntries.size();

	if (!m_bQuiet)
		_ftprintf(stdout, _T("Extracting Accelerators..."));
	EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d Accelerators\n"), m_StringEntries.size()-nEntries);
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

BOOL CResModule::CreateTranslatedResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszDestLangDllPath, LPCTSTR lpszPOFilePath)
{
	if (!CopyFile(lpszSrcLangDllPath, lpszDestLangDllPath, FALSE))
		MYERROR;

	int count = 0;
	do
	{
		if (SysInfo::Instance().IsVistaOrLater())
			m_hResDll = LoadLibraryEx (lpszSrcLangDllPath, NULL, LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE|LOAD_LIBRARY_AS_IMAGE_RESOURCE|LOAD_IGNORE_CODE_AUTHZ_LEVEL);
		else
			m_hResDll = LoadLibraryEx (lpszSrcLangDllPath, NULL, LOAD_LIBRARY_AS_IMAGE_RESOURCE|LOAD_IGNORE_CODE_AUTHZ_LEVEL);
		if (m_hResDll == NULL)
			Sleep(100);
		count++;
	} while ((m_hResDll == NULL)&&(count < 10));

	if (m_hResDll == NULL)
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

	BOOL bRes = FALSE;
	count = 0;
	do
	{
		m_hUpdateRes = BeginUpdateResource(sDestFile.c_str(), FALSE);
		if (m_hUpdateRes == NULL)
			Sleep(100);
		count++;
	} while ((m_hUpdateRes == NULL)&&(count < 10));

	if (m_hUpdateRes == NULL)
		MYERROR;


	if (!m_bQuiet)
		_ftprintf(stdout, _T("Translating StringTable..."));
	bRes = EnumResourceNames(m_hResDll, RT_STRING, EnumResNameWriteCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d translated, %4d not translated\n"), m_bTranslatedStrings, m_bDefaultStrings);

	if (!m_bQuiet)
		_ftprintf(stdout, _T("Translating Dialogs......."));
	bRes = EnumResourceNames(m_hResDll, RT_DIALOG, EnumResNameWriteCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d translated, %4d not translated\n"), m_bTranslatedDialogStrings, m_bDefaultDialogStrings);

	if (!m_bQuiet)
		_ftprintf(stdout, _T("Translating Menus........."));
	bRes = EnumResourceNames(m_hResDll, RT_MENU, EnumResNameWriteCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d translated, %4d not translated\n"), m_bTranslatedMenuStrings, m_bDefaultMenuStrings);

	if (!m_bQuiet)
		_ftprintf(stdout, _T("Translating Accelerators.."));
	bRes = EnumResourceNames(m_hResDll, RT_ACCELERATOR, EnumResNameWriteCallback, (LONG_PTR)this);
	if (!m_bQuiet)
		_ftprintf(stdout, _T("%4d translated, %4d not translated\n"), m_bTranslatedAcceleratorStrings, m_bDefaultAcceleratorStrings);

	bRes = TRUE;
	if (!EndUpdateResource(m_hUpdateRes, !bRes))
		MYERROR;

	FreeLibrary(m_hResDll);
	return TRUE;
DONE_ERROR:
	if (m_hResDll)
		FreeLibrary(m_hResDll);
	return FALSE;
}

BOOL CResModule::ExtractString(UINT nID)
{
	HRSRC		hrsrc = FindResource(m_hResDll, MAKEINTRESOURCE(nID), RT_STRING);
	HGLOBAL		hglStringTable;
	LPWSTR		p;

	if (!hrsrc)
		MYERROR;
	hglStringTable = LoadResource(m_hResDll, hrsrc);

	if (!hglStringTable)
		goto DONE_ERROR;
	p = (LPWSTR)LockResource(hglStringTable);

	if (p == NULL)
		goto DONE_ERROR;
	/*	[Block of 16 strings.  The strings are Pascal style with a WORD
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
		WCHAR * pBuf = new WCHAR[MAX_STRING_LENGTH*2];
		SecureZeroMemory(pBuf, MAX_STRING_LENGTH*2*sizeof(WCHAR));
		wcscpy(pBuf, msgid.c_str());
		CUtils::StringExtend(pBuf);

		if (wcslen(pBuf))
		{
			std::wstring str = std::wstring(pBuf);
			RESOURCEENTRY entry = m_StringEntries[str];
			entry.resourceIDs.insert(nID);
			if (wcschr(str.c_str(), '%'))
				entry.flag = _T("#, c-format");
			m_StringEntries[str] = entry;
		}
		delete [] pBuf;
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

BOOL CResModule::ReplaceString(UINT nID, WORD wLanguage)
{
	HRSRC		hrsrc = FindResourceEx(m_hResDll, RT_STRING, MAKEINTRESOURCE(nID), wLanguage);
	HGLOBAL		hglStringTable;
	LPWSTR		p;

	if (!hrsrc)
		MYERROR;
	hglStringTable = LoadResource(m_hResDll, hrsrc);

	if (!hglStringTable)
		goto DONE_ERROR;
	p = (LPWSTR)LockResource(hglStringTable);

	if (p == NULL)
		goto DONE_ERROR;
/*	[Block of 16 strings.  The strings are Pascal style with a WORD
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
		WCHAR * pBuf = new WCHAR[MAX_STRING_LENGTH*2];
		SecureZeroMemory(pBuf, MAX_STRING_LENGTH*2*sizeof(WCHAR));
		wcscpy(pBuf, msgid.c_str());
		CUtils::StringExtend(pBuf);
		msgid = std::wstring(pBuf);

		RESOURCEENTRY resEntry;
		resEntry = m_StringEntries[msgid];
		wcscpy(pBuf, resEntry.msgstr.c_str());
		CUtils::StringCollapse(pBuf);
		size_t newlen = wcslen(pBuf);
		if (newlen)
			nMem += newlen;
		else
			nMem += len;
		pp += len;
		delete [] pBuf;
	}

	WORD * newTable = new WORD[nMem + (nMem % 2)];
	SecureZeroMemory(newTable, (nMem + (nMem % 2))*2);

	size_t index = 0;
	for (int i=0; i<16; ++i)
	{
		int len = GET_WORD(p);
		p++;
		std::wstring msgid = std::wstring(p, len);
		WCHAR * pBuf = new WCHAR[MAX_STRING_LENGTH*2];
		SecureZeroMemory(pBuf, MAX_STRING_LENGTH*2*sizeof(WCHAR));
		wcscpy(pBuf, msgid.c_str());
		CUtils::StringExtend(pBuf);
		msgid = std::wstring(pBuf);

		RESOURCEENTRY resEntry;
		resEntry = m_StringEntries[msgid];
		wcscpy(pBuf, resEntry.msgstr.c_str());
		CUtils::StringCollapse(pBuf);
		size_t newlen = wcslen(pBuf);
		if (newlen)
		{
			newTable[index++] = (WORD)newlen;
			wcsncpy((wchar_t *)&newTable[index], pBuf, newlen);
			index += newlen;
			m_bTranslatedStrings++;
		}
		else
		{
			newTable[index++] = (WORD)len;
			if (len)
				wcsncpy((wchar_t *)&newTable[index], p, len);
			index += len;
			if (len)
				m_bDefaultStrings++;
		}
		p += len;
		delete [] pBuf;
	}

	if (!UpdateResource(m_hUpdateRes, RT_STRING, MAKEINTRESOURCE(nID), (m_wTargetLang ? m_wTargetLang : wLanguage), newTable, (DWORD)(nMem + (nMem % 2))*2))
	{
		delete [] newTable;
		goto DONE_ERROR;
	}

	if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_STRING, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
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

BOOL CResModule::ExtractMenu(UINT nID)
{
	HRSRC		hrsrc = FindResource(m_hResDll, MAKEINTRESOURCE(nID), RT_MENU);
	HGLOBAL		hglMenuTemplate;
	WORD		version, offset;
	DWORD dwHelpId;
	const WORD *p, *p0;

	if (!hrsrc)
		MYERROR;

	hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

	if (!hglMenuTemplate)
		MYERROR;

	p = (const WORD*)LockResource(hglMenuTemplate);

	if (p == NULL)
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
			dwHelpId = GET_DWORD(p);
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

BOOL CResModule::ReplaceMenu(UINT nID, WORD wLanguage)
{
	HRSRC		hrsrc = FindResourceEx(m_hResDll, RT_MENU, MAKEINTRESOURCE(nID), wLanguage);
	HGLOBAL		hglMenuTemplate;
	WORD		version, offset;
	LPWSTR		p;
	WORD *p0;
	DWORD dwHelpId;

	if (!hrsrc)
		MYERROR;	//just the language wasn't found

	hglMenuTemplate = LoadResource(m_hResDll, hrsrc);

	if (!hglMenuTemplate)
		MYERROR;

	p = (LPWSTR)LockResource(hglMenuTemplate);

	if (p == NULL)
		MYERROR;

	//struct MenuHeader {
	//	WORD   wVersion;           // Currently zero
	//	WORD   cbHeaderSize;       // Also zero
	//};

	// MENUEX resource
	//struct MenuExHeader {
	//    WORD wVersion;           // One
	//    WORD wOffset;
	//    DWORD dwHelpId;
	//};
	p0 = (WORD *)p;
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
			if (!CountMemReplaceMenuResource((WORD *)p, &nMem, NULL))
				goto DONE_ERROR;
			WORD * newMenu = new WORD[nMem + (nMem % 2)+2];
			SecureZeroMemory(newMenu, (nMem + (nMem % 2)+2)*2);
			size_t index = 2;		// MenuHeader has 2 WORDs zero
			if (!CountMemReplaceMenuResource((WORD *)p, &index, newMenu))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if (!UpdateResource(m_hUpdateRes, RT_MENU, MAKEINTRESOURCE(nID), (m_wTargetLang ? m_wTargetLang : wLanguage), newMenu, (DWORD)(nMem + (nMem % 2)+2)*2))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_MENU, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
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
			dwHelpId = GET_DWORD(p);
			size_t nMem = 0;
			if (!CountMemReplaceMenuExResource((WORD *)(p0 + offset), &nMem, NULL))
				goto DONE_ERROR;
			WORD * newMenu = new WORD[nMem + (nMem % 2) + 4];
			SecureZeroMemory(newMenu, (nMem + (nMem % 2) + 4) * 2);
			CopyMemory(newMenu, p0, 2 * sizeof(WORD) + sizeof(DWORD));
			size_t index = 4;		// MenuExHeader has 2 x WORD + 1 x DWORD
			if (!CountMemReplaceMenuExResource((WORD *)(p0 + offset), &index, newMenu))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if (!UpdateResource(m_hUpdateRes, RT_MENU, MAKEINTRESOURCE(nID), (m_wTargetLang ? m_wTargetLang : wLanguage), newMenu, (DWORD)(nMem + (nMem % 2) + 4) * 2))
			{
				delete [] newMenu;
				goto DONE_ERROR;
			}

			if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_MENU, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
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
	LPCWSTR     str;

	//struct PopupMenuItem {
	//	WORD   fItemFlags;
	//	WCHAR  szItemText[];
	//};
	//struct NormalMenuItem {
	//	WORD   fItemFlags;
	//	WORD   wMenuID;
	//	WCHAR  szItemText[];
	//};

	do
	{
		flags = GET_WORD(res);
		res++;
		if (!(flags & MF_POPUP))
		{
			id = GET_WORD(res);	//normal menu item
			res++;
		}
		else
			id = (WORD)-1;			//popup menu item

		str = (LPCWSTR)res;
		size_t l = wcslen(str)+1;
		res += l;

		if (flags & MF_POPUP)
		{
			TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
			SecureZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
			_tcscpy(pBuf, str);
			CUtils::StringExtend(pBuf);

			std::wstring wstr = std::wstring(pBuf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			if (id)
				entry.resourceIDs.insert(id);

			m_StringEntries[wstr] = entry;
			delete [] pBuf;

			if ((res = ParseMenuResource(res))==0)
				return NULL;
		}
		else if (id != 0)
		{
			TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
			SecureZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
			_tcscpy(pBuf, str);
			CUtils::StringExtend(pBuf);

			std::wstring wstr = std::wstring(pBuf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			entry.resourceIDs.insert(id);

			TCHAR szTempBuf[1024];
			_stprintf(szTempBuf, _T("#: MenuEntry; ID:%d"), id);
			MENUENTRY menu_entry;
			menu_entry.wID = id;
			menu_entry.reference = szTempBuf;
			menu_entry.msgstr = wstr;

			m_StringEntries[wstr] = entry;
			m_MenuEntries[id] = menu_entry;
			delete [] pBuf;
		}
	} while (!(flags & MF_END));
	return res;
}

const WORD* CResModule::CountMemReplaceMenuResource(const WORD * res, size_t * wordcount, WORD * newMenu)
{
	WORD		flags;
	WORD		id = 0;

	//struct PopupMenuItem {
	//	WORD   fItemFlags;
	//	WCHAR  szItemText[];
	//};
	//struct NormalMenuItem {
	//	WORD   fItemFlags;
	//	WORD   wMenuID;
	//	WCHAR  szItemText[];
	//};

	do
	{
		flags = GET_WORD(res);
		res++;
		if (newMenu == NULL)
			(*wordcount)++;
		else
			newMenu[(*wordcount)++] = flags;
		if (!(flags & MF_POPUP))
		{
			id = GET_WORD(res);	//normal menu item
			res++;
			if (newMenu == NULL)
				(*wordcount)++;
			else
				newMenu[(*wordcount)++] = id;
		}
		else
			id = (WORD)-1;			//popup menu item

		if (flags & MF_POPUP)
		{
			ReplaceStr((LPCWSTR)res, newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen((LPCWSTR)res) + 1;

			if ((res = CountMemReplaceMenuResource(res, wordcount, newMenu))==0)
				return NULL;
		}
		else if (id != 0)
		{
			ReplaceStr((LPCWSTR)res, newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen((LPCWSTR)res) + 1;
		}
		else
		{
			if (newMenu)
				wcscpy((wchar_t *)&newMenu[(*wordcount)], (LPCWSTR)res);
			(*wordcount) += wcslen((LPCWSTR)res) + 1;
			res += wcslen((LPCWSTR)res) + 1;
		}
	} while (!(flags & MF_END));
	return res;
}

const WORD* CResModule::ParseMenuExResource(const WORD * res)
{
	DWORD dwType, dwState, menuId;
	WORD bResInfo;
	LPCWSTR		str;

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
		dwType = GET_DWORD(res);
		res += 2;
		dwState = GET_DWORD(res);
		res += 2;
		menuId = GET_DWORD(res);
		res += 2;
		bResInfo = GET_WORD(res);
		res++;

		str = (LPCWSTR)res;
		size_t l = wcslen(str)+1;
		res += l;
		// Align to DWORD boundary
		res += ((((WORD)res + 3) & ~3) - (WORD)res)/sizeof(WORD);

		if (dwType & MFT_SEPARATOR)
			continue;

		if (bResInfo & 0x01)
		{
			// Popup menu - note this can also have a non-zero ID
			if (menuId == 0)
				menuId = (WORD)-1;
			TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
			SecureZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
			_tcscpy(pBuf, str);
			CUtils::StringExtend(pBuf);

			std::wstring wstr = std::wstring(pBuf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			// Popup has a DWORD help entry on a DWORD boundary - skip over it
			res += 2;

			entry.resourceIDs.insert(menuId);
			TCHAR szTempBuf[1024];
			_stprintf(szTempBuf, _T("#: MenuExPopupEntry; ID:%d"), menuId);
			MENUENTRY menu_entry;
			menu_entry.wID = (WORD)menuId;
			menu_entry.reference = szTempBuf;
			menu_entry.msgstr = wstr;
			m_StringEntries[wstr] = entry;
			m_MenuEntries[(WORD)menuId] = menu_entry;
			delete [] pBuf;

			if ((res = ParseMenuExResource(res)) == 0)
				return NULL;
		} else if (menuId != 0)
		{
			TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
			SecureZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
			_tcscpy(pBuf, str);
			CUtils::StringExtend(pBuf);

			std::wstring wstr = std::wstring(pBuf);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			entry.resourceIDs.insert(menuId);

			TCHAR szTempBuf[1024];
			_stprintf(szTempBuf, _T("#: MenuExEntry; ID:%d"), menuId);
			MENUENTRY menu_entry;
			menu_entry.wID = (WORD)menuId;
			menu_entry.reference = szTempBuf;
			menu_entry.msgstr = wstr;
			m_StringEntries[wstr] = entry;
			m_MenuEntries[(WORD)menuId] = menu_entry;
			delete [] pBuf;
		}
	} while (!(bResInfo & 0x80));
	return res;
}

const WORD* CResModule::CountMemReplaceMenuExResource(const WORD * res, size_t * wordcount, WORD * newMenu)
{
	DWORD dwType, dwState, menuId;
	WORD bResInfo;
	WORD *p0;

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
		p0 = (WORD *)res;
		dwType = GET_DWORD(res);
		res += 2;
		dwState = GET_DWORD(res);
		res += 2;
		menuId = GET_DWORD(res);
		res += 2;
		bResInfo = GET_WORD(res);
		res++;

		if (newMenu != NULL) {
			CopyMemory(&newMenu[*wordcount], p0, 7 * sizeof(WORD));
		}
		(*wordcount) += 7;

		if (dwType & MFT_SEPARATOR) {
			// Align to DWORD
			(*wordcount)++;
			res++;
			continue;
		}

		if (bResInfo & 0x01)
		{
			ReplaceStr((LPCWSTR)res, newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen((LPCWSTR)res) + 1;
			// Align to DWORD
			res += ((((WORD)res + 3) & ~3) - (WORD)res)/sizeof(WORD);
			if ((*wordcount) & 0x01)
				(*wordcount)++;

			if (newMenu != NULL)
				CopyMemory(&newMenu[*wordcount], res, sizeof(DWORD));  // Copy Help ID

			res += 2;
			(*wordcount) += 2;

			if ((res = CountMemReplaceMenuExResource(res, wordcount, newMenu)) == 0)
				return NULL;
		}
		else if (menuId != 0)
		{
			ReplaceStr((LPCWSTR)res, newMenu, wordcount, &m_bTranslatedMenuStrings, &m_bDefaultMenuStrings);
			res += wcslen((LPCWSTR)res) + 1;
		}
		else
		{
			if (newMenu)
				wcscpy((wchar_t *)&newMenu[(*wordcount)], (LPCWSTR)res);
			(*wordcount) += wcslen((LPCWSTR)res) + 1;
			res += wcslen((LPCWSTR)res) + 1;
		}
		// Align to DWORD
		res += ((((WORD)res + 3) & ~3) - (WORD)res)/sizeof(WORD);
		if ((*wordcount) & 0x01)
			(*wordcount)++;
	} while (!(bResInfo & 0x80));
	return res;
}

BOOL CResModule::ExtractAccelerator(UINT nID)
{
	HRSRC		hrsrc = FindResource(m_hResDll, MAKEINTRESOURCE(nID), RT_ACCELERATOR);
	HGLOBAL		hglAccTable;
	WORD		fFlags, wAnsi, wID;
	const WORD*	p;
	bool		bEnd(false);

	if (!hrsrc)
		MYERROR;

	hglAccTable = LoadResource(m_hResDll, hrsrc);

	if (!hglAccTable)
		goto DONE_ERROR;

	p = (const WORD*)LockResource(hglAccTable);

	if (p == NULL)
		MYERROR;

	/*
	struct ACCELTABLEENTRY
	{
		WORD fFlags;		FVIRTKEY, FSHIFT, FCONTROL, FALT, 0x80 - Last in a table
		WORD wAnsi;			ANSI character
		WORD wId;			Keyboard accelerator passed to windows
		WORD padding;		# bytes added to ensure aligned to DWORD boundary
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
		{				// 0x80
			bEnd = true;
		}

		if ((wAnsi < 0x30) ||
			(wAnsi > 0x5A) ||
			(wAnsi >= 0x3A && wAnsi <= 0x40))
			continue;

		TCHAR * pBuf = new TCHAR[1024];
		SecureZeroMemory(pBuf, 1024 * sizeof(TCHAR));

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
		_stprintf(pBuf, _T("ID:%d:"), wID);

		// EXACTLY 5 characters long "ACS+X"
		// V = Virtual key (or blank if not used)
		// A = Alt key     (or blank if not used)
		// C = Ctrl key    (or blank if not used)
		// S = Shift key   (or blank if not used)
		// X = upper case character
		// e.g. "V CS+Q" == Ctrl + Shift + 'Q'
		if ((fFlags & FVIRTKEY) == FVIRTKEY) 		// 0x01
			_tcscat(pBuf, _T("V"));
		else
			_tcscat(pBuf, _T(" "));

		if ((fFlags & FALT) == FALT) 				// 0x10
			_tcscat(pBuf, _T("A"));
		else
			_tcscat(pBuf, _T(" "));

		if ((fFlags & FCONTROL) == FCONTROL) 		// 0x08
			_tcscat(pBuf, _T("C"));
		else
			_tcscat(pBuf, _T(" "));

		if ((fFlags & FSHIFT) == FSHIFT) 			// 0x04
			_tcscat(pBuf, _T("S"));
		else
			_tcscat(pBuf, _T(" "));

		_stprintf(pBuf, _T("%s+%c"), pBuf, wAnsi);

		std::wstring wstr = std::wstring(pBuf);
		RESOURCEENTRY AKey_entry = m_StringEntries[wstr];

		TCHAR szTempBuf[1024];
		SecureZeroMemory(szTempBuf, sizeof (szTempBuf));
		std::wstring wmenu = _T("");
		pME_iter = m_MenuEntries.find(wID);
		if (pME_iter != m_MenuEntries.end())
		{
			wmenu = pME_iter->second.msgstr;
		}
		_stprintf(szTempBuf, _T("#. Accelerator Entry for Menu ID:%d; '%s'"), wID, wmenu.c_str());
		AKey_entry.automaticcomments.push_back(std::wstring(szTempBuf));

		m_StringEntries[wstr] = AKey_entry;
		delete [] pBuf;
	} while (!bEnd);

	UnlockResource(hglAccTable);
	FreeResource(hglAccTable);
	return TRUE;

DONE_ERROR:
	UnlockResource(hglAccTable);
	FreeResource(hglAccTable);
	MYERROR;
}

BOOL CResModule::ReplaceAccelerator(UINT nID, WORD wLanguage)
{
	LPACCEL		lpaccelNew;			// pointer to new accelerator table
	HACCEL		haccelOld;			// handle to old accelerator table
	int			cAccelerators;		// number of accelerators in table
	HGLOBAL		hglAccTableNew;
	const WORD*	p;
	int			i;

	haccelOld = LoadAccelerators(m_hResDll, MAKEINTRESOURCE(nID));

	if (haccelOld == NULL)
		MYERROR;

	cAccelerators = CopyAcceleratorTable(haccelOld, NULL, 0);

	lpaccelNew = (LPACCEL) LocalAlloc(LPTR, cAccelerators * sizeof(ACCEL));

	if (lpaccelNew == NULL)
		MYERROR;

	CopyAcceleratorTable(haccelOld, lpaccelNew, cAccelerators);

	// Find the accelerator that the user modified
	// and change its flags and virtual-key code
	// as appropriate.

	BYTE xfVirt;
	WORD xkey;
	static const size_t BufferSize = 1024;
	TCHAR * pBuf = new TCHAR[BufferSize];
	for (i = 0; i < cAccelerators; i++)
	{
		if ((lpaccelNew[i].key < 0x30) ||
			(lpaccelNew[i].key > 0x5A) ||
			(lpaccelNew[i].key >= 0x3A && lpaccelNew[i].key <= 0x40))
			continue;

		SecureZeroMemory(pBuf, BufferSize * sizeof(TCHAR));

		_stprintf(pBuf, _T("ID:%d:"), lpaccelNew[i].cmd);

		// get original key combination
		if ((lpaccelNew[i].fVirt & FVIRTKEY) == FVIRTKEY) 		// 0x01
			_tcscat(pBuf, _T("V"));
		else
			_tcscat(pBuf, _T(" "));

		if ((lpaccelNew[i].fVirt & FALT) == FALT) 				// 0x10
			_tcscat(pBuf, _T("A"));
		else
			_tcscat(pBuf, _T(" "));

		if ((lpaccelNew[i].fVirt & FCONTROL) == FCONTROL) 		// 0x08
			_tcscat(pBuf, _T("C"));
		else
			_tcscat(pBuf, _T(" "));

		if ((lpaccelNew[i].fVirt & FSHIFT) == FSHIFT) 			// 0x04
			_tcscat(pBuf, _T("S"));
		else
			_tcscat(pBuf, _T(" "));

		_stprintf(pBuf, _T("%s+%c"), pBuf, lpaccelNew[i].key);

		// Is it there?
		std::map<std::wstring, RESOURCEENTRY>::iterator pAK_iter = m_StringEntries.find(pBuf);
		if (pAK_iter != m_StringEntries.end())
		{
			m_bTranslatedAcceleratorStrings++;
			xfVirt = 0;
			xkey = 0;
			std::wstring wtemp = pAK_iter->second.msgstr;
			wtemp = wtemp.substr(wtemp.find_last_of(':')+1);
			if (wtemp.size() != 6)
				continue;
			if (wtemp.compare(0, 1, _T("V")) == 0)
				xfVirt |= FVIRTKEY;
			else if (wtemp.compare(0, 1, _T(" ")) != 0)
				continue;	// not a space - user must have made a mistake when translating
			if (wtemp.compare(1, 1, _T("A")) == 0)
				xfVirt |= FALT;
			else if (wtemp.compare(1, 1, _T(" ")) != 0)
				continue;	// not a space - user must have made a mistake when translating
			if (wtemp.compare(2, 1, _T("C")) == 0)
				xfVirt |= FCONTROL;
			else if (wtemp.compare(2, 1, _T(" ")) != 0)
				continue;	// not a space - user must have made a mistake when translating
			if (wtemp.compare(3, 1, _T("S")) == 0)
				xfVirt |= FSHIFT;
			else if (wtemp.compare(3, 1, _T(" ")) != 0)
				continue;	// not a space - user must have made a mistake when translating
			if (wtemp.compare(4, 1, _T("+")) == 0)
			{
				_stscanf(wtemp.substr(5, 1).c_str(), _T("%c"), &xkey);
				lpaccelNew[i].fVirt = xfVirt;
				lpaccelNew[i].key = xkey;
			}
		}
		else
			m_bDefaultAcceleratorStrings++;
	}

	delete [] pBuf;

	// Create the new accelerator table
	hglAccTableNew = LocalAlloc(LPTR, cAccelerators * 4 * sizeof(WORD));
	p = (WORD *)hglAccTableNew;
	lpaccelNew[cAccelerators-1].fVirt |= 0x80;
	for (i = 0; i < cAccelerators; i++)
	{
		memcpy((void *)p, &lpaccelNew[i].fVirt, 1);
		p++;
		memcpy((void *)p, &lpaccelNew[i].key, sizeof(WORD));
		p++;
		memcpy((void *)p, &lpaccelNew[i].cmd, sizeof(WORD));
		p++;
		p++;
	}

	if (!UpdateResource(m_hUpdateRes, RT_ACCELERATOR, MAKEINTRESOURCE(nID),
		(m_wTargetLang ? m_wTargetLang : wLanguage), hglAccTableNew /* haccelNew*/, cAccelerators * 4 * sizeof(WORD)))
	{
		goto DONE_ERROR;
	}

	if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_ACCELERATOR, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
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

BOOL CResModule::ExtractDialog(UINT nID)
{
	const WORD*	lpDlg;
	const WORD*	lpDlgItem;
	DIALOGINFO	dlg;
	DLGITEMINFO	dlgItem;
	WORD		bNumControls;
	HRSRC		hrsrc;
	HGLOBAL		hGlblDlgTemplate;

	hrsrc = FindResource(m_hResDll, MAKEINTRESOURCE(nID), RT_DIALOG);

	if (hrsrc == NULL)
		MYERROR;

	hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);
	if (hGlblDlgTemplate == NULL)
		MYERROR;

	lpDlg = (const WORD*) LockResource(hGlblDlgTemplate);

	if (lpDlg == NULL)
		MYERROR;

	lpDlgItem = (const WORD*) GetDialogInfo(lpDlg, &dlg);
	bNumControls = dlg.nbItems;

	if (dlg.caption)
	{
		TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
		SecureZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
		_tcscpy(pBuf, dlg.caption);
		CUtils::StringExtend(pBuf);

		std::wstring wstr = std::wstring(pBuf);
		RESOURCEENTRY entry = m_StringEntries[wstr];
		entry.resourceIDs.insert(nID);

		m_StringEntries[wstr] = entry;
		delete [] pBuf;
	}

	while (bNumControls-- != 0)
	{
		TCHAR szTitle[500];
		SecureZeroMemory(szTitle, sizeof(szTitle));
		BOOL  bCode;

		lpDlgItem = GetControlInfo((WORD *) lpDlgItem, &dlgItem, dlg.dialogEx, &bCode);

		if (bCode == FALSE)
			_tcscpy(szTitle, dlgItem.windowName);

		if (_tcslen(szTitle) > 0)
		{
			CUtils::StringExtend(szTitle);

			std::wstring wstr = std::wstring(szTitle);
			RESOURCEENTRY entry = m_StringEntries[wstr];
			entry.resourceIDs.insert(dlgItem.id);

			m_StringEntries[wstr] = entry;
		}
	}

	UnlockResource(hGlblDlgTemplate);
	FreeResource(hGlblDlgTemplate);
	return (TRUE);
}

BOOL CResModule::ReplaceDialog(UINT nID, WORD wLanguage)
{
	const WORD*	lpDlg;
	HRSRC		hrsrc;
	HGLOBAL		hGlblDlgTemplate;

	hrsrc = FindResourceEx(m_hResDll, RT_DIALOG, MAKEINTRESOURCE(nID), wLanguage);

	if (hrsrc == NULL)
		MYERROR;

	hGlblDlgTemplate = LoadResource(m_hResDll, hrsrc);

	if (hGlblDlgTemplate == NULL)
		MYERROR;

	lpDlg = (WORD *) LockResource(hGlblDlgTemplate);

	if (lpDlg == NULL)
		MYERROR;

	size_t nMem = 0;
	const WORD * p = lpDlg;
	if (!CountMemReplaceDialogResource(p, &nMem, NULL))
		goto DONE_ERROR;
	WORD * newDialog = new WORD[nMem + (nMem % 2)];
	SecureZeroMemory(newDialog, (nMem + (nMem % 2))*2);

	size_t index = 0;
	if (!CountMemReplaceDialogResource(lpDlg, &index, newDialog))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}

	if (!UpdateResource(m_hUpdateRes, RT_DIALOG, MAKEINTRESOURCE(nID), (m_wTargetLang ? m_wTargetLang : wLanguage), newDialog, (DWORD)(nMem + (nMem % 2))*2))
	{
		delete [] newDialog;
		goto DONE_ERROR;
	}

	if ((m_wTargetLang)&&(!UpdateResource(m_hUpdateRes, RT_DIALOG, MAKEINTRESOURCE(nID), wLanguage, NULL, 0)))
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

const WORD* CResModule::GetDialogInfo(const WORD * pTemplate, LPDIALOGINFO lpDlgInfo)
{
	const WORD* p = (const WORD *)pTemplate;

	lpDlgInfo->style = GET_DWORD(p);
	p += 2;

	if (lpDlgInfo->style == 0xffff0001)	// DIALOGEX resource
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
		lpDlgInfo->menuName = NULL;
		p++;
		break;
	case 0xffff:
		lpDlgInfo->menuName = (LPCTSTR) (WORD) GET_WORD(p + 1);
		p += 2;
		break;
	default:
		lpDlgInfo->menuName = (LPCTSTR) p;
		p += wcslen((LPCWSTR) p) + 1;
		break;
	}

	// Get the class name

	switch (GET_WORD(p))
	{
	case 0x0000:
		lpDlgInfo->className = (LPCTSTR)MAKEINTATOM(32770);
		p++;
		break;
	case 0xffff:
		lpDlgInfo->className = (LPCTSTR) (WORD) GET_WORD(p + 1);
		p += 2;
		break;
	default:
		lpDlgInfo->className = (LPCTSTR) p;
		p += wcslen((LPCTSTR)p) + 1;
		break;
	}

	// Get the window caption

	lpDlgInfo->caption = (LPCTSTR)p;
	p += wcslen((LPCWSTR) p) + 1;

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

		lpDlgInfo->faceName = (LPCTSTR)p;
		p += wcslen((LPCWSTR) p) + 1;
	}
	// First control is on DWORD boundary
	p += ((((WORD)p + 3) & ~3) - (WORD)p)/sizeof(WORD);

	return p;
}

const WORD* CResModule::GetControlInfo(const WORD* p, LPDLGITEMINFO lpDlgItemInfo, BOOL dialogEx, LPBOOL bIsID)
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
		lpDlgItemInfo->id = (WORD) GET_DWORD(p);
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
		lpDlgItemInfo->className = (LPCTSTR) p;
		p += wcslen((LPCWSTR) p) + 1;
	}

	if (GET_WORD(p) == 0xffff)	// an integer ID?
	{
		*bIsID = TRUE;
		lpDlgItemInfo->windowName = (LPCTSTR) (DWORD) GET_WORD(p + 1);
		p += 2;
	}
	else
	{
		*bIsID = FALSE;
		lpDlgItemInfo->windowName = (LPCTSTR) p;
		p += wcslen((LPCWSTR) p) + 1;
	}

	if (GET_WORD(p))
	{
		lpDlgItemInfo->data = (LPVOID) (p + 1);
		p += GET_WORD(p) / sizeof(WORD);
	}
	else
		lpDlgItemInfo->data = NULL;

	p++;
	// Next control is on DWORD boundary
	p += ((((WORD)p + 3) & ~3) - (WORD)p)/sizeof(WORD);
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

	if (style == 0xffff0001)	// DIALOGEX resource
	{
		bEx = TRUE;
		if (newDialog)
		{
			newDialog[(*wordcount)++] = GET_WORD(res++);	//help id
			newDialog[(*wordcount)++] = GET_WORD(res++);	//help id
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			style = GET_DWORD(res);
			newDialog[(*wordcount)++] = GET_WORD(res++);	//style
			newDialog[(*wordcount)++] = GET_WORD(res++);	//style
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
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
			//style = GET_DWORD(res);
			//newDialog[(*wordcount)++] = GET_WORD(res++);	//style
			//newDialog[(*wordcount)++] = GET_WORD(res++);	//style
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
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		}
		(*wordcount) += wcslen((LPCWSTR) res) + 1;
		res += wcslen((LPCWSTR) res) + 1;
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
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		}
		(*wordcount) += wcslen((LPCWSTR) res) + 1;
		res += wcslen((LPCWSTR) res) + 1;
		break;
	}

	// Get the window caption

	ReplaceStr((LPCWSTR)res, newDialog, wordcount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
	res += wcslen((LPCWSTR)res) + 1;

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
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		(*wordcount) += wcslen((LPCWSTR)res) + 1;
		res += wcslen((LPCWSTR)res) + 1;
	}
	// First control is on DWORD boundary
	while ((*wordcount)%2)
		(*wordcount)++;
	while ((ULONG)res % 4)
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
			newDialog[(*wordcount)++] = GET_WORD(res++);	//helpid
			newDialog[(*wordcount)++] = GET_WORD(res++);	//helpid
		}
		else
		{
			res += 2;
			(*wordcount) += 2;
		}
	}
	if (newDialog)
	{
		LONG * exStyle = (LONG*)&newDialog[(*wordcount)];
		newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
		newDialog[(*wordcount)++] = GET_WORD(res++);	//exStyle
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
		newDialog[(*wordcount)++] = GET_WORD(res++);	//style
		newDialog[(*wordcount)++] = GET_WORD(res++);	//style
	}
	else
	{
		res += 2;
		(*wordcount) += 2;
	}

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//x
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//y
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//cx
	res++;
	(*wordcount)++;

	if (newDialog)
		newDialog[(*wordcount)] = GET_WORD(res);	//cy
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

	if (GET_WORD(res) == 0xffff)	//classID
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
			wcscpy((LPWSTR)&newDialog[(*wordcount)], (LPCWSTR)res);
		(*wordcount) += wcslen((LPCWSTR) res) + 1;
		res += wcslen((LPCWSTR) res) + 1;
	}

	if (GET_WORD(res) == 0xffff)	// an integer ID?
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
		ReplaceStr((LPCWSTR)res, newDialog, wordcount, &m_bTranslatedDialogStrings, &m_bDefaultDialogStrings);
		res += wcslen((LPCWSTR)res) + 1;
	}

	if (newDialog)
		memcpy(&newDialog[(*wordcount)], res, (GET_WORD(res)+1)*sizeof(WORD));
	(*wordcount) += (GET_WORD(res)+1);
	res += (GET_WORD(res)+1);
	// Next control is on DWORD boundary
	while ((*wordcount) % 2)
		(*wordcount)++;
	res += ((((WORD)res + 3) & ~3) - (WORD)res)/sizeof(WORD);

	return res;
}

BOOL CALLBACK CResModule::EnumResNameCallback(HMODULE /*hModule*/, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	CResModule* lpResModule = (CResModule*)lParam;

	if (lpszType == RT_STRING)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractString(LOWORD(lpszName)))
				return FALSE;
		}
	}
	else if (lpszType == RT_MENU)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractMenu(LOWORD(lpszName)))
				return FALSE;
		}
	}
	else if (lpszType == RT_DIALOG)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractDialog(LOWORD(lpszName)))
				return FALSE;
		}
	}
	else if (lpszType == RT_ACCELERATOR)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			if (!lpResModule->ExtractAccelerator(LOWORD(lpszName)))
				return FALSE;
		}
	}

	return TRUE;
}

#pragma warning(push)
#pragma warning(disable: 4189)
BOOL CALLBACK CResModule::EnumResNameWriteCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam)
{
	CResModule* lpResModule = (CResModule*)lParam;
	return EnumResourceLanguages(hModule, lpszType, lpszName, (ENUMRESLANGPROC)&lpResModule->EnumResWriteLangCallback, lParam);
}
#pragma warning(pop)

BOOL CALLBACK CResModule::EnumResWriteLangCallback(HMODULE /*hModule*/, LPCTSTR lpszType, LPTSTR lpszName, WORD wLanguage, LONG_PTR lParam)
{
	BOOL bRes = FALSE;
	CResModule* lpResModule = (CResModule*)lParam;

	if (lpszType == RT_STRING)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			bRes = lpResModule->ReplaceString(LOWORD(lpszName), wLanguage);
		}
	}
	else if (lpszType == RT_MENU)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			bRes = lpResModule->ReplaceMenu(LOWORD(lpszName), wLanguage);
		}
	}
	else if (lpszType == RT_DIALOG)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			bRes = lpResModule->ReplaceDialog(LOWORD(lpszName), wLanguage);
		}
	}
	else if (lpszType == RT_ACCELERATOR)
	{
		if (IS_INTRESOURCE(lpszName))
		{
			bRes = lpResModule->ReplaceAccelerator(LOWORD(lpszName), wLanguage);
		}
	}

	return bRes;

}

void CResModule::ReplaceStr(LPCWSTR src, WORD * dest, size_t * count, int * translated, int * def)
{
	TCHAR * pBuf = new TCHAR[MAX_STRING_LENGTH];
	SecureZeroMemory(pBuf, MAX_STRING_LENGTH * sizeof(TCHAR));
	wcscpy(pBuf, src);
	CUtils::StringExtend(pBuf);

	std::wstring wstr = std::wstring(pBuf);
	RESOURCEENTRY entry = m_StringEntries[wstr];
	if (entry.msgstr.size())
	{
		wcscpy(pBuf, entry.msgstr.c_str());
		CUtils::StringCollapse(pBuf);
		if (dest)
			wcscpy((wchar_t *)&dest[(*count)], pBuf);
		(*count) += wcslen(pBuf)+1;
		(*translated)++;
	}
	else
	{
		if (dest)
			wcscpy((wchar_t *)&dest[(*count)], src);
		(*count) += wcslen(src) + 1;
		if (wcslen(src))
			(*def)++;
	}
	delete [] pBuf;
}
