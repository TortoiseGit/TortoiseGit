// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2019 - TortoiseGit
// Copyright (C) 2003-2007, 2011-2012, 2014-2017 - TortoiseSVN

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
#pragma once
#include <vector>
#include <string>
#include <map>
#include "POFile.h"

#define GET_WORD(ptr)        (*reinterpret_cast<const WORD*>(ptr))
#define GET_DWORD(ptr)       (*reinterpret_cast<const DWORD*>(ptr))
#define ALIGN_DWORD(type, p) reinterpret_cast<type>(reinterpret_cast<DWORD>((p) + 3) & ~3)

#define MAX_STRING_LENGTH   (32*1024)

// DIALOG CONTROL INFORMATION
typedef struct tagDlgItemInfo
{
	DWORD   style;
	DWORD   exStyle;
	DWORD   helpId;
	short   x;
	short   y;
	short   cx;
	short   cy;
	WORD    id;
	LPCTSTR className;
	LPCTSTR windowName;
	LPVOID  data;
} DLGITEMINFO, * LPDLGITEMINFO;

// DIALOG TEMPLATE
typedef struct tagDialogInfo
{
	DWORD   style;
	DWORD   exStyle;
	DWORD   helpId;
	WORD    nbItems;
	short   x;
	short   y;
	short   cx;
	short   cy;
	LPCTSTR menuName;
	LPCTSTR className;
	LPCTSTR caption;
	WORD    pointSize;
	WORD    weight;
	BOOL    italic;
	LPCTSTR faceName;
	BOOL    dialogEx;
} DIALOGINFO, * LPDIALOGINFO;
// MENU resource
typedef struct tagMenuEntry
{
	WORD            wID;
	std::wstring    reference;
	std::wstring    msgstr;
} MENUENTRY, * LPMENUENTRY;

/**
 * \ingroup ResText
 * Class to handle a resource module (*.exe or *.dll file).
 *
 * Provides methods to extract and apply resource strings.
 */
class CResModule
{
public:
	CResModule(void);
	~CResModule(void);

	BOOL    ExtractResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszPOFilePath, BOOL bNoUpdate, LPCTSTR lpszHeaderFile);
	BOOL    ExtractResources(const std::vector<std::wstring>& filelist, LPCTSTR lpszPOFilePath, BOOL bNoUpdate, LPCTSTR lpszHeaderFile);
	BOOL    CreateTranslatedResources(LPCTSTR lpszSrcLangDllPath, LPCTSTR lpszDestLangDllPath, LPCTSTR lpszPOFilePath);
	void    SetQuiet(BOOL bQuiet = TRUE) {m_bQuiet = bQuiet; m_StringEntries.SetQuiet(bQuiet);}
	void    SetLanguage(WORD wLangID) {m_wTargetLang = wLangID;}
	void    SetRTL(bool bRTL = true) {m_bRTL = bRTL;}
	void    SetAdjustEOLs(bool bAdjustEOLs = true) {m_bAdjustEOLs = bAdjustEOLs;}

private:
	static  BOOL CALLBACK EnumResNameCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam);
	static  BOOL CALLBACK EnumResNameWriteCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, LONG_PTR lParam);
	static  BOOL CALLBACK EnumResWriteLangCallback(HMODULE hModule, LPCTSTR lpszType, LPTSTR lpszName, WORD wLanguage, LONG_PTR lParam);
	BOOL    ExtractString(LPCTSTR lpszType);
	BOOL    ExtractDialog(LPCTSTR lpszType);
	BOOL    ExtractMenu(LPCTSTR lpszType);
	BOOL    ExtractRibbon(LPCTSTR lpszType);
	BOOL    ReplaceString(LPCTSTR lpszType, WORD wLanguage);
	BOOL    ReplaceDialog(LPCTSTR lpszType, WORD wLanguage);
	BOOL    ReplaceMenu(LPCTSTR lpszType, WORD wLanguage);
	BOOL    ExtractAccelerator(LPCTSTR lpszType);
	BOOL    ReplaceAccelerator(LPCTSTR lpszType, WORD wLanguage);
	BOOL    ReplaceRibbon(LPCTSTR lpszType, WORD wLanguage);

	template <size_t _Size>
	inline std::wstring ReplaceWithRegex(WCHAR (&pBuf)[_Size])
	{
		return ReplaceWithRegex(pBuf, _Size);
	}
	std::wstring ReplaceWithRegex(WCHAR* pBuf, size_t bufferSize);
	std::wstring ReplaceWithRegex(std::wstring& s);

	const WORD* ParseMenuResource(const WORD * res);
	const WORD* CountMemReplaceMenuResource(const WORD * res, size_t * wordcount, WORD * newMenu);
	const WORD* ParseMenuExResource(const WORD * res);
	const WORD* CountMemReplaceMenuExResource(const WORD * res, size_t * wordcount, WORD * newMenu);
	const WORD* GetControlInfo(const WORD* p, LPDLGITEMINFO lpDlgItemInfo, BOOL dialogEx, LPBOOL bIsID) const;
	const WORD* GetDialogInfo(const WORD * pTemplate, LPDIALOGINFO lpDlgInfo) const;
	const WORD* CountMemReplaceDialogResource(const WORD * res, size_t * wordcount, WORD * newMenu);
	const WORD* ReplaceControlInfo(const WORD * res, size_t * wordcount, WORD * newDialog, BOOL bEx);

	void    ReplaceStr(LPCWSTR src, WORD * dest, size_t * count, int * translated, int * def);

	size_t  ScanHeaderFile(const std::wstring& filepath);
	void    InsertResourceIDs(LPCWSTR lpType, INT_PTR mainId, RESOURCEENTRY& entry, INT_PTR id, LPCWSTR infotext);
	bool    AdjustCheckSum(const std::wstring& resFile);
	void    RemoveSignatures(LPCTSTR lpszDestLangDllPath);

	HMODULE         m_hResDll;
	HANDLE          m_hUpdateRes;
	CPOFile         m_StringEntries;
	std::map<WORD, MENUENTRY> m_MenuEntries;
	std::map<WORD, MENUENTRY>::iterator pME_iter;
	std::wstring    sDestFile;
	std::map<INT_PTR, std::wstring> m_currentHeaderDataDialogs;
	std::map<INT_PTR, std::wstring> m_currentHeaderDataStrings;
	std::map<INT_PTR, std::wstring> m_currentHeaderDataMenus;
	BOOL            m_bQuiet;

	bool            m_bRTL;
	bool            m_bAdjustEOLs;

	int             m_bTranslatedStrings;
	int             m_bDefaultStrings;
	int             m_bTranslatedDialogStrings;
	int             m_bDefaultDialogStrings;
	int             m_bTranslatedMenuStrings;
	int             m_bDefaultMenuStrings;
	int             m_bTranslatedAcceleratorStrings;
	int             m_bDefaultAcceleratorStrings;
	int             m_bTranslatedRibbonTexts;
	int             m_bDefaultRibbonTexts;

	WORD            m_wTargetLang;
};
