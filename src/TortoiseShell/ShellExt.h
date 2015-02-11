// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI
// Copyright (C) 2003-2012 - TortoiseSVN

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
#pragma once

#include "Globals.h"
#include "registry.h"
#include "resource.h"
#include "ShellCache.h"
#include "IconBitmapUtils.h"
#include "MenuInfo.h"
#include "CrashReport.h"
#include "FileStatus.h"
#include "../version.h"
#include "DebugEventLog.h"

extern	volatile LONG		g_cRefThisDll;			// Reference count of this DLL.
extern	HINSTANCE			g_hmodThisDll;			// Instance handle for this DLL
extern	ShellCache			g_ShellCache;			// caching of registry entries, ...
extern	DWORD				g_langid;
extern	DWORD				g_langTimeout;
extern	HINSTANCE			g_hResInst;
extern	bool				g_readonlyoverlay;		///< whether to show the read only overlay or not
extern	bool				g_lockedoverlay;		///< whether to show the locked overlay or not

extern bool					g_normalovlloaded;
extern bool					g_modifiedovlloaded;
extern bool					g_conflictedovlloaded;
extern bool					g_readonlyovlloaded;
extern bool					g_deletedovlloaded;
extern bool					g_lockedovlloaded;
extern bool					g_addedovlloaded;
extern bool					g_ignoredovlloaded;
extern bool					g_unversionedovlloaded;
extern LPCTSTR				g_MenuIDString;

extern	void				LoadLangDll();
extern  CComCriticalSection	g_csGlobalCOMGuard;
typedef CComCritSecLock<CComCriticalSection> AutoLocker;

extern std::wstring getTortoiseSIString(DWORD stringID);

// The actual OLE Shell context menu handler
/**
 * \ingroup TortoiseShell
 * The main class of our COM object / Shell Extension.
 * It contains all Interfaces we implement for the shell to use.
 * \remark The implementations of the different interfaces are
 * split into several *.cpp files to keep them in a reasonable size.
 */
class CShellExt : public IContextMenu,
							IShellExtInit,
							IShellIconOverlayIdentifier,
							IShellPropSheetExt

{
protected:
	// used by IShellIconOverlayIdentifier to ask which icon we want
	FileState m_State;
	ULONG	m_cRef;
	// used by IContextMenu
	std::map<UINT_PTR, MenuInfo*> myIDMap;
	std::map<std::wstring, UINT_PTR> myVerbsMap;
	std::map<UINT_PTR, std::wstring> myVerbsIDMap;

	std::vector<std::wstring> selectedItems;
	FileStatusFlags	selectedItemsStatus;

	std::wstring currentExplorerWindowFolder;
	bool currentFolderIsControlled;

	stdstring ignoredprops;

	IconBitmapUtils		m_iconBitmapUtils;
public:
	const std::vector<std::wstring>& getSelectedItems() { return selectedItems; }
	const std::wstring& getCurrentFolder() { return currentExplorerWindowFolder; }
	bool isCurrentFolderControlled() { return currentFolderIsControlled; }
	FileStatusFlags getSelectedItemsStatus() { return selectedItemsStatus; }

private:
	void			InsertSIMenu(HMENU menu, UINT pos, UINT_PTR id, UINT idCmdFirst, MenuInfo& menuInfo);
	bool			InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT &indexMenu, int &indexSubMenu, unsigned __int64 topmenu, bool bShowIcons, UINT uFlags);
	void			TweakMenu(HMENU menu);
	STDMETHODIMP	QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT &indexMenu);
	bool			IsIllegalFolder(std::wstring folder);
	HRESULT			doesStatusMatch(FileStatusFlags fileStatusFlags);
	bool			IsPathAllowed(std::wstring folder){ return g_ShellCache.IsPathAllowed(folder); };
	FileStatusFlags	getPathStatus(std::wstring path);

	/** \name IContextMenu wrappers
	 * IContextMenu wrapper functions to catch exceptions and send crash reports
	 */
	//@{
	STDMETHODIMP	QueryContextMenu_Wrap(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHODIMP	InvokeCommand_Wrap(LPCMINVOKECOMMANDINFO lpcmi);
	STDMETHODIMP	GetCommandString_Wrap(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
	//@}

	/** \name IShellExtInit wrappers
	 * IShellExtInit wrapper functions to catch exceptions and send crash reports
	 */
	//@{
	STDMETHODIMP	Initialize_Wrap(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
	//@}

	/** \name IShellIconOverlayIdentifier wrappers
	 * IShellIconOverlayIdentifier wrapper functions to catch exceptions and send crash reports
	 */
	//@{
	STDMETHODIMP	GetOverlayInfo_Wrap(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
	STDMETHODIMP	GetPriority_Wrap(int *pPriority);
	STDMETHODIMP	IsMemberOf_Wrap(LPCWSTR pwszPath, DWORD dwAttrib);
	//@}

	/** \name IShellPropSheetExt wrappers
	 * IShellPropSheetExt wrapper functions to catch exceptions and send crash reports
	 */
	//@{
	STDMETHODIMP	AddPages_Wrap(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
	//STDMETHODIMP	ReplacePage_Wrap(UINT, LPFNADDPROPSHEETPAGE, LPARAM);
	//@}

public:
	CShellExt(FileState state);
	virtual ~CShellExt();

	/** \name IUnknown
	 * IUnknown members
	 */
	//@{
	STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG) AddRef();
	STDMETHODIMP_(ULONG) Release();
	//@}

	/** \name IContextMenu
	 * IContextMenu members
	 */
	//@{
	STDMETHODIMP	QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
	STDMETHODIMP	InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi);
	STDMETHODIMP	GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax);
	//@}


	/** \name IShellExtInit
	 * IShellExtInit methods
	 */
	//@{
	STDMETHODIMP	Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID);
	//@}

	/** \name IShellIconOverlayIdentifier
	 * IShellIconOverlayIdentifier methods
	 */
	//@{
	STDMETHODIMP	GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags);
	STDMETHODIMP	GetPriority(int *pPriority);
	STDMETHODIMP	IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib);
	//@}

	/** \name IShellPropSheetExt
	 * IShellPropSheetExt methods
	 */
	//@{
	STDMETHODIMP	AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam);
	STDMETHODIMP	ReplacePage (UINT, LPFNADDPROPSHEETPAGE, LPARAM);
	//@}
};
