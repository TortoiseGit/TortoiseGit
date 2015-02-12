// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014-2015 - TortoiseSVN
// Copyright (C) 2008-2014 - TortoiseGit
// Copyright (C) 2015 - TortoiseSI

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
#include "ShellExt.h"
#include "ItemIDList.h"
#include "PreserveChdir.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "resource.h"
#include "StatusCache.h"

#define GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

int g_shellidlist=RegisterClipboardFormat(CFSTR_SHELLIDLIST);

extern std::vector<MenuInfo> menuInfo;
static int g_syncSeq = 0;

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY  hRegKey)
{
	__try
	{
		return Initialize_Wrap(pIDFolder, pDataObj, hRegKey);
	}
	__except (HandleException(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::Initialize_Wrap(LPCITEMIDLIST pIDFolder,
                                        LPDATAOBJECT pDataObj,
                                        HKEY /* hRegKey */)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: Initialize\n");
	PreserveChdir preserveChdir;
	selectedItems.clear();
	currentExplorerWindowFolder.clear();
	selectedItemsStatus = (FileStatusFlags)FileStatus::None;
	currentFolderIsControlled = false;
	// get selected files/folders

	if (pDataObj)
	{
		STGMEDIUM medium;
		FORMATETC fmte = {(CLIPFORMAT)g_shellidlist,
			(DVTARGETDEVICE FAR *)NULL,
			DVASPECT_CONTENT,
			-1,
			TYMED_HGLOBAL};
		HRESULT hres = pDataObj->GetData(&fmte, &medium);

		if (SUCCEEDED(hres) && medium.hGlobal)
		{
			//Enumerate PIDLs which the user has selected
			CIDA* cida = (CIDA*)GlobalLock(medium.hGlobal);
			ItemIDList parent(GetPIDLFolder(cida));

			int count = cida->cidl;
			for (int i = 0; i < count; ++i)
			{
				ItemIDList child(GetPIDLItem(cida, i), &parent);
				stdstring str = child.toString();
				if (!str.empty() && IsPathAllowed(str))
				{
					selectedItems.push_back(str);
					// TODO batch?
					selectedItemsStatus |= getPathStatus(str);
				}
			}
			ItemIDList child(GetPIDLItem(cida, 0), &parent);

			GlobalUnlock(medium.hGlobal);
		}

		ReleaseStgMedium(&medium);
		if (medium.pUnkForRelease)
		{
			IUnknown* relInterface = (IUnknown*)medium.pUnkForRelease;
			relInterface->Release();
		}
	}

	// get folder background
	if (pIDFolder)
	{
		ItemIDList list(pIDFolder);
		currentExplorerWindowFolder = list.toString();
		if (IsPathAllowed(currentExplorerWindowFolder))
		{
			if (IStatusCache::getInstance().getRootFolderCache().isPathControlled(currentExplorerWindowFolder))
			{
				currentFolderIsControlled = true;
			}
		}
		else
		{
			currentExplorerWindowFolder.clear();
		}
	}	
	return S_OK;
}

void CShellExt::InsertSIMenu(HMENU menu, UINT pos, UINT_PTR id, UINT idCmdFirst, MenuInfo& menuInfo)
{
	std::wstring menutext = getTortoiseSIString(menuInfo.menuTextID);
	TCHAR verbsbuffer[255] = {0};

	MENUITEMINFO menuiteminfo;
	SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
	menuiteminfo.fType = MFT_STRING;
	menuiteminfo.dwTypeData = (LPWSTR)menutext.c_str();
	if (menuInfo.iconID)
	{
		menuiteminfo.fMask |= MIIM_BITMAP;
		menuiteminfo.hbmpItem = SysInfo::Instance().IsVistaOrLater() ? m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, menuInfo.iconID) : HBMMENU_CALLBACK;
	}
	menuiteminfo.wID = (UINT)id;
	InsertMenuItem(menu, pos, TRUE, &menuiteminfo);

	LoadString(g_hResInst, menuInfo.menuTextID, verbsbuffer, _countof(verbsbuffer));
	std::wstring verb = verbsbuffer;
	if (verb.find('&') != -1)
	{
		verb.erase(verb.find('&'),1);
	}
	myVerbsMap[verb] = id - idCmdFirst;
	myVerbsMap[verb] = id;
	myVerbsIDMap[id - idCmdFirst] = verb;
	myVerbsIDMap[id] = verb;
	myIDMap[id - idCmdFirst] = &menuInfo;
	myIDMap[id] = &menuInfo;
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT idCmdLast,
                                         UINT uFlags)
{
	__try
	{
		return QueryContextMenu_Wrap(hMenu, indexMenu, idCmdFirst, idCmdLast, uFlags);
	}
	__except (HandleException(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

STDMETHODIMP CShellExt::QueryContextMenu_Wrap(HMENU hMenu,
                                              UINT indexMenu,
                                              UINT idCmdFirst,
                                              UINT /*idCmdLast*/,
                                              UINT uFlags)
{
	PreserveChdir preserveChdir;

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return S_OK;					//we don't change the default action

	if (selectedItems.empty() && currentExplorerWindowFolder.empty())
		return S_OK;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return S_OK;

	if (IsIllegalFolder(currentExplorerWindowFolder))
		return S_OK;

	if (currentExplorerWindowFolder.empty())
	{
		// folder is empty, but maybe files are selected
		if (selectedItems.empty())
			return S_OK;	// nothing selected - we don't have a menu to show
		// check whether a selected entry is an UID - those are namespace extensions
		// which we can't handle
		for (const std::wstring& path : selectedItems)
		{
			if (_tcsncmp(path.c_str(), _T("::{"), 3) == 0)
				return S_OK;
		}
	}
	else
	{
		if (_tcsncmp(currentExplorerWindowFolder.c_str(), _T("::{"), 3) == 0)
			return S_OK;
	}

	//check if we already added our menu entry for a folder.
	//we check that by iterating through all menu entries and check if
	//the dwItemData member points to our global ID string. That string is set
	//by our shell extension when the folder menu is inserted.
	TCHAR menubuf[MAX_PATH] = {0};
	int count = GetMenuItemCount(hMenu);
	for (int i=0; i<count; ++i)
	{
		MENUITEMINFO miif;
		SecureZeroMemory(&miif, sizeof(MENUITEMINFO));
		miif.cbSize = sizeof(MENUITEMINFO);
		miif.fMask = MIIM_DATA;
		miif.dwTypeData = menubuf;
		miif.cch = _countof(menubuf);
		GetMenuItemInfo(hMenu, i, TRUE, &miif);
		if (miif.dwItemData == (ULONG_PTR)g_MenuIDString)
			return S_OK;
	}

	LoadLangDll();
	UINT idCmd = idCmdFirst;

	//create the sub menu
	HMENU subMenu = CreateMenu();
	int indexSubMenu = 0;

	bool bAddSeparator = false;
	bool bMenuEntryAdded = false;
	bool bMenuEmpty = true;
	// insert separator at start
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	std::vector<std::wstring> itemsForMenuAction = getItemsForMenuAction();
	FileStatusFlags itemStatusForMenuAction = getItemsStatusForMenuAction();

	for (MenuInfo& menu : menuInfo)
	{
		if (menu.menuID == MenuItem::Seperator)
		{
			// we don't add a separator immediately. Because there might not be
			// another 'normal' menu entry after we insert a separator.
			// we simply set a flag here, indicating that before the next
			// 'normal' menu entry, a separator should be added.
			if (!bMenuEmpty)
				bAddSeparator = true;
			if (bMenuEntryAdded)
				bAddSeparator = true;
		}
		else
		{
			// check the conditions whether to show the menu entry or not
			bool bInsertMenu = menu.enable(itemsForMenuAction, itemStatusForMenuAction);
			if (bInsertMenu)
			{
				// insert a separator
				if ((bMenuEntryAdded) && (bAddSeparator))
				{
					bAddSeparator = false;
					bMenuEntryAdded = false;
					InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, NULL);
					idCmd++;
				}

				// handle special cases (sub menus)
				if ((menu.menuID == MenuItem::IgnoreSubMenu) || (menu.menuID == MenuItem::UnIgnoreSubMenu))
				{
					if (InsertIgnoreSubmenus(idCmd, idCmdFirst, hMenu, subMenu, indexMenu, indexSubMenu, 0, TRUE, uFlags))
						bMenuEntryAdded = true;
				}
				else
				{
					// insert the menu entry
					InsertSIMenu(subMenu,
						indexSubMenu++,
						idCmd++,
						idCmdFirst,
						menu);

					bMenuEntryAdded = true;
					bMenuEmpty = false;
					bAddSeparator = false;
				}
			}
		}
	}

	// do not show TortoiseSI menu if it's empty
	if (bMenuEmpty)
	{
		if (idCmd - idCmdFirst > 0)
		{
			//separator after
			InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;
		}
		TweakMenu(hMenu);

		//return number of menu items added
		return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
	}

	//add sub menu to main context menu
	//don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
	//see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
	std::wstring menuName = getTortoiseSIString(IDS_MENU);

	MENUITEMINFO menuiteminfo;
	SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	menuiteminfo.fType = MFT_STRING;
	menuiteminfo.dwTypeData = (LPWSTR)menuName.c_str();

	menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING;
	menuiteminfo.fMask |= MIIM_BITMAP;
	menuiteminfo.hbmpItem = SysInfo::Instance().IsVistaOrLater() ? m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, IDI_APP) : HBMMENU_CALLBACK;
	menuiteminfo.hSubMenu = subMenu;
	menuiteminfo.wID = idCmd++;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);

	//separator after
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	TweakMenu(hMenu);

	//return number of menu items added
	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
}

void CShellExt::TweakMenu(HMENU hMenu)
{
	MENUINFO MenuInfo = {};
	MenuInfo.cbSize  = sizeof(MenuInfo);
	MenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
	MenuInfo.dwStyle = MNS_CHECKORBMP;
	SetMenuInfo(hMenu, &MenuInfo);
}

STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	__try
	{
		return InvokeCommand_Wrap(lpcmi);
	}
	__except (HandleException(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi)
{
	PreserveChdir preserveChdir;
	HRESULT hr = E_INVALIDARG;
	if (lpcmi == NULL)
		return hr;

	if (!selectedItems.empty() || !currentExplorerWindowFolder.empty())
	{
		UINT_PTR idCmd = LOWORD(lpcmi->lpVerb);

		if (HIWORD(lpcmi->lpVerb))
		{
			stdstring verb = stdstring(MultibyteToWide(lpcmi->lpVerb));
			std::map<stdstring, UINT_PTR>::const_iterator verb_it = myVerbsMap.lower_bound(verb);
			if (verb_it != myVerbsMap.end() && verb_it->first == verb)
				idCmd = verb_it->second;
			else
				return hr;
		}

		// See if we have a handler interface for this id
		std::map<UINT_PTR, MenuInfo*>::const_iterator id_it = myIDMap.lower_bound(idCmd);
		if (id_it != myIDMap.end() && id_it->first == idCmd)
		{
			id_it->second->siCommand(getItemsForMenuAction(), lpcmi->hwnd);
			hr = S_OK;
		} // if (id_it != myIDMap.end() && id_it->first == idCmd)
	} // if (files_.empty() || folder_.empty())
	return hr;

}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT uFlags,
                                         UINT FAR * reserved,
                                         LPSTR pszName,
                                         UINT cchMax)
{
	__try
	{
		return GetCommandString_Wrap(idCmd, uFlags, reserved, pszName, cchMax);
	}
	__except (HandleException(GetExceptionInformation()))
	{
	}
	return E_FAIL;
}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString_Wrap(UINT_PTR idCmd,
                                              UINT uFlags,
                                              UINT FAR * /*reserved*/,
                                              LPSTR pszName,
                                              UINT cchMax)
{
	PreserveChdir preserveChdir;
	//do we know the id?
	std::map<UINT_PTR, MenuInfo*>::const_iterator id_it = myIDMap.lower_bound(idCmd);
	if (id_it == myIDMap.end() || id_it->first != idCmd)
	{
		return E_INVALIDARG;		//no, we don't
	}

	LoadLangDll();
	HRESULT hr = E_INVALIDARG;

	std::wstring menuDescription = getTortoiseSIString(id_it->second->menuDescID);

	switch(uFlags)
	{
	case GCS_HELPTEXTA:
		{
			std::string help = WideToMultibyte(menuDescription.c_str());
			lstrcpynA(pszName, help.c_str(), cchMax);
			hr = S_OK;
			break;
		}
	case GCS_HELPTEXTW:
		{
			lstrcpynW((LPWSTR)pszName, menuDescription.c_str(), cchMax);
			hr = S_OK;
			break;
		}
	case GCS_VERBA:
		{
			std::map<UINT_PTR, stdstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
			if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
			{
				std::string help = WideToMultibyte(verb_id_it->second);
				lstrcpynA(pszName, help.c_str(), cchMax);
				hr = S_OK;
			}
		}
		break;
	case GCS_VERBW:
		{
			std::map<UINT_PTR, stdstring>::const_iterator verb_id_it = myVerbsIDMap.lower_bound(idCmd);
			if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
			{
				wide_string help = verb_id_it->second;
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": verb : %ws\n", help.c_str());
				lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax);
				hr = S_OK;
			}
		}
		break;
	}
	return hr;
}

bool CShellExt::IsIllegalFolder(std::wstring folder)
{
	int csidlarray[] =
	{
		CSIDL_BITBUCKET,
		CSIDL_CDBURN_AREA,
		CSIDL_COMMON_FAVORITES,
		CSIDL_COMMON_STARTMENU,
		CSIDL_COMPUTERSNEARME,
		CSIDL_CONNECTIONS,
		CSIDL_CONTROLS,
		CSIDL_COOKIES,
		CSIDL_FAVORITES,
		CSIDL_FONTS,
		CSIDL_HISTORY,
		CSIDL_INTERNET,
		CSIDL_INTERNET_CACHE,
		CSIDL_NETHOOD,
		CSIDL_NETWORK,
		CSIDL_PRINTERS,
		CSIDL_PRINTHOOD,
		CSIDL_RECENT,
		CSIDL_SENDTO,
		CSIDL_STARTMENU,
		0
	};

	int i=0;
	TCHAR buf[MAX_PATH] = {0};	//MAX_PATH ok, since SHGetSpecialFolderPath doesn't return the required buffer length!
	LPITEMIDLIST pidl = NULL;
	while (csidlarray[i])
	{
		++i;
		pidl = NULL;
		if (SHGetFolderLocation(NULL, csidlarray[i - 1], NULL, 0, &pidl) != S_OK)
			continue;
		if (!SHGetPathFromIDList(pidl, buf))
		{
			// not a file system path, definitely illegal for our use
			CoTaskMemFree(pidl);
			continue;
		}
		CoTaskMemFree(pidl);
		if (_tcslen(buf)==0)
			continue;
		if (_tcscmp(buf, folder.c_str())==0)
			return true;
	}
	return false;
}

bool CShellExt::InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT &indexMenu, int &indexSubMenu, unsigned __int64 topmenu, bool bShowIcons, UINT /*uFlags*/)
{
#if 0
	HMENU ignoresubmenu = NULL;
	int indexignoresub = 0;
	bool bShowIgnoreMenu = false;
	TCHAR maskbuf[MAX_PATH] = {0};		// MAX_PATH is ok, since this only holds a filename
	TCHAR ignorepath[MAX_PATH] = {0};		// MAX_PATH is ok, since this only holds a filename
	if (files_.empty())
		return false;
	UINT icon = bShowIcons ? IDI_IGNORE : 0;

	std::vector<stdstring>::iterator I = files_.begin();
	if (_tcsrchr(I->c_str(), '\\'))
		_tcscpy_s(ignorepath, MAX_PATH, _tcsrchr(I->c_str(), '\\')+1);
	else
		_tcscpy_s(ignorepath, MAX_PATH, I->c_str());
	if ((itemStates & ITEMIS_IGNORED) && (!ignoredprops.empty()))
	{
		// check if the item name is ignored or the mask
		size_t p = 0;
		while ( (p=ignoredprops.find( ignorepath,p )) != -1 )
		{
			if ( (p==0 || ignoredprops[p-1]==TCHAR('\n'))
				&& (p+_tcslen(ignorepath)==ignoredprops.length() || ignoredprops[p+_tcslen(ignorepath)+1]==TCHAR('\n')) )
			{
				break;
			}
			p++;
		}
		if (p!=-1)
		{
			ignoresubmenu = CreateMenu();
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING, idCmd, ignorepath);
			stdstring verb = stdstring(ignorepath);
			myVerbsMap[verb] = idCmd - idCmdFirst;
			myVerbsMap[verb] = idCmd;
			myVerbsIDMap[idCmd - idCmdFirst] = verb;
			myVerbsIDMap[idCmd] = verb;
			myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnore;
			myIDMap[idCmd++] = ShellMenuUnIgnore;
			bShowIgnoreMenu = true;
		}
		_tcscpy_s(maskbuf, MAX_PATH, _T("*"));
		if (_tcsrchr(ignorepath, '.'))
		{
			_tcscat_s(maskbuf, MAX_PATH, _tcsrchr(ignorepath, '.'));
			p = ignoredprops.find(maskbuf);
			if ((p!=-1) &&
				((ignoredprops.compare(maskbuf)==0) || (ignoredprops.find('\n', p)==p+_tcslen(maskbuf)+1) || (ignoredprops.rfind('\n', p)==p-1)))
			{
				if (ignoresubmenu==NULL)
					ignoresubmenu = CreateMenu();

				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
				stdstring verb = stdstring(maskbuf);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreCaseSensitive;
				myIDMap[idCmd++] = ShellMenuUnIgnoreCaseSensitive;
				bShowIgnoreMenu = true;
			}
		}
	}
	else if ((itemStates & ITEMIS_IGNORED) == 0)
	{
		bShowIgnoreMenu = true;
		ignoresubmenu = CreateMenu();
		if (itemStates & ITEMIS_ONLYONE)
		{
			if (itemStates & ITEMIS_INGIT)
			{
			}
			else
			{
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING, idCmd, ignorepath);
				myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
				myIDMap[idCmd++] = ShellMenuIgnore;

				_tcscpy_s(maskbuf, MAX_PATH, _T("*"));
				if (!(itemStates & ITEMIS_FOLDER) && _tcsrchr(ignorepath, '.'))
				{
					_tcscat_s(maskbuf, MAX_PATH, _tcsrchr(ignorepath, '.'));
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING, idCmd, maskbuf);
					stdstring verb = stdstring(maskbuf);
					myVerbsMap[verb] = idCmd - idCmdFirst;
					myVerbsMap[verb] = idCmd;
					myVerbsIDMap[idCmd - idCmdFirst] = verb;
					myVerbsIDMap[idCmd] = verb;
					myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitive;
					myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitive;
				}
			}
		}
		else
		{
			if (itemStates & ITEMIS_INGIT)
			{
			}
			else
			{
				MAKESTRING(IDS_MENUIGNOREMULTIPLE);
				_stprintf_s(ignorepath, MAX_PATH, stringtablebuffer, files_.size());
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING, idCmd, ignorepath);
				stdstring verb = stdstring(ignorepath);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
				myIDMap[idCmd++] = ShellMenuIgnore;

				MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK);
				_stprintf_s(ignorepath, MAX_PATH, stringtablebuffer, files_.size());
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING, idCmd, ignorepath);
				verb = stdstring(ignorepath);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitive;
				myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitive;
			}
		}
	}

	if (bShowIgnoreMenu)
	{
		MENUITEMINFO menuiteminfo;
		SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
		menuiteminfo.cbSize = sizeof(menuiteminfo);
		menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING;
		if (icon)
		{
			menuiteminfo.fMask |= MIIM_BITMAP;
			menuiteminfo.hbmpItem = SysInfo::Instance().IsVistaOrLater() ? m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, icon) : m_iconBitmapUtils.IconToBitmap(g_hResInst, icon);
		}
		menuiteminfo.fType = MFT_STRING;
		menuiteminfo.hSubMenu = ignoresubmenu;
		menuiteminfo.wID = idCmd;
		SecureZeroMemory(stringtablebuffer, sizeof(stringtablebuffer));
		if (itemStates & ITEMIS_IGNORED)
			GetMenuTextFromResource(ShellMenuUnIgnoreSub);
		else if (itemStates & ITEMIS_INGIT)
			GetMenuTextFromResource(ShellMenuDeleteIgnoreSub);
		else
			GetMenuTextFromResource(ShellMenuIgnoreSub);
		menuiteminfo.dwTypeData = stringtablebuffer;
		menuiteminfo.cch = (UINT)min(_tcslen(menuiteminfo.dwTypeData), UINT_MAX);

		InsertMenuItem((topmenu & MENUIGNORE) ? hMenu : subMenu, (topmenu & MENUIGNORE) ? indexMenu++ : indexSubMenu++, TRUE, &menuiteminfo);
		if (itemStates & ITEMIS_IGNORED)
		{
			myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnoreSub;
			myIDMap[idCmd++] = ShellMenuUnIgnoreSub;
		}
		else
		{
			myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreSub;
			myIDMap[idCmd++] = ShellMenuIgnoreSub;
		}
	}
	return bShowIgnoreMenu;
#endif 
	return false;
}

std::vector<std::wstring> CShellExt::getItemsForMenuAction()
{
	if (selectedItems.size() > 0) {
		return selectedItems;
	} else if (currentExplorerWindowFolder.size() > 0){
		return { currentExplorerWindowFolder };
	} else {
		return {};
	}
};

FileStatusFlags CShellExt::getItemsStatusForMenuAction()
{
	if (selectedItems.size() > 0) {
		return selectedItemsStatus;
	} else if (currentExplorerWindowFolder.size() > 0){
		if (currentFolderIsControlled) {
			return FileStatus::Folder | FileStatus::Member;
		} else {
			return (FileStatusFlags)FileStatus::Folder;
		}
	} else {
		return 0;
	}
};
