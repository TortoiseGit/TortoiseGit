// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018-2019 - TortoiseGit
// Copyright (C) 2003-2012, 2014 - TortoiseSVN

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
#include "RemoteCacheLink.h"
#include "GitStatus.h"
#include "GitFolderStatus.h"
#include "IconBitmapUtils.h"
#include "MenuInfo.h"

extern	volatile LONG		g_cRefThisDll;			// Reference count of this DLL.
extern	HINSTANCE			g_hmodThisDll;			// Instance handle for this DLL
extern	ShellCache			g_ShellCache;			// caching of registry entries, ...
extern	DWORD				g_langid;
extern	ULONGLONG			g_langTimeout;
extern	HINSTANCE			g_hResInst;
extern	std::wstring		g_filepath;
extern	git_wc_status_kind	g_filestatus;			///< holds the corresponding status to the file/dir above
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

// The actual OLE Shell context menu handler
/**
 * \ingroup TortoiseShell
 * The main class of our COM object / Shell Extension.
 * It contains all Interfaces we implement for the shell to use.
 * \remark The implementations of the different interfaces are
 * split into several *.cpp files to keep them in a reasonable size.
 */
class CShellExt : public IContextMenu3,
							IPersistFile,
							IShellExtInit,
							IShellIconOverlayIdentifier,
							IShellPropSheetExt,
							ICopyHookW

{
protected:

	FileState m_State;
	volatile ULONG	m_cRef;
	//std::map<int,std::string> verbMap;
	std::map<UINT_PTR, UINT_PTR>	myIDMap;
	std::map<UINT_PTR, UINT_PTR>	mySubMenuMap;
	std::map<std::wstring, UINT_PTR> myVerbsMap;
	std::map<UINT_PTR, std::wstring> myVerbsIDMap;
	std::wstring folder_;
	std::vector<std::wstring> files_;
	DWORD itemStates;				///< see the globals.h file for the ITEMIS_* defines
	DWORD itemStatesFolder;			///< used for states of the folder_ (folder background and/or drop target folder)
	std::wstring uuidSource;
	std::wstring uuidTarget;
	int space;
	TCHAR stringtablebuffer[255];
	std::wstring ignoredprops;
	CRegStdString		regDiffLater;

	GitFolderStatus		m_CachedStatus;		// status cache
	CRemoteCacheLink	m_remoteCacheLink;
	IconBitmapUtils		m_iconBitmapUtils;

#define MAKESTRING(ID) LoadStringEx(g_hResInst, ID, stringtablebuffer, _countof(stringtablebuffer), static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT))))
private:
	void			InsertGitMenu(BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, GitCommands com, UINT uFlags);
	bool			InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT &indexMenu, int &indexSubMenu, unsigned __int64 topmenu, bool bShowIcons, UINT uFlags);
	std::wstring	WriteFileListToTempFile();
	bool			WriteClipboardPathsToTempFile(std::wstring& tempfile);
	LPCTSTR			GetMenuTextFromResource(int id);
	bool			ShouldInsertItem(const MenuInfo& pair) const;
	bool			ShouldEnableMenu(const YesNoPair& pair) const;
	void			TweakMenu(HMENU menu);
	void			AddPathCommand(tstring& gitCmd, LPCTSTR command, bool bFilesAllowed);
	void			AddPathFileCommand(tstring& gitCmd, LPCTSTR command);
	void			AddPathFileDropCommand(tstring& gitCmd, LPCTSTR command);
	STDMETHODIMP	QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT &indexMenu);
	bool			IsIllegalFolder(const std::wstring& folder, int* cslidarray);
	static void		RunCommand(const tstring& path, const tstring& command, LPCTSTR errorMessage);

public:
	CShellExt(FileState state);
	virtual ~CShellExt();

	/** \name IUnknown
	 * IUnknown members
	 */
	//@{
	STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *) override;
	STDMETHODIMP_(ULONG) AddRef() override;
	STDMETHODIMP_(ULONG) Release() override;
	//@}

	/** \name IContextMenu2
	 * IContextMenu2 members
	 */
	//@{
	STDMETHODIMP	QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) override;
	STDMETHODIMP	InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi) override;
	STDMETHODIMP	GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR *reserved, LPSTR pszName, UINT cchMax) override;
	STDMETHODIMP	HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) override;
	//@}

	/** \name IContextMenu3
	 * IContextMenu3 members
	 */
	//@{
	STDMETHODIMP	HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult) override;
	//@}

	/** \name IShellExtInit
	 * IShellExtInit methods
	 */
	//@{
	STDMETHODIMP	Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY hKeyID) override;
	//@}

	/** \name IPersistFile
	 * IPersistFile methods
	 */
	//@{
	STDMETHODIMP	GetClassID(CLSID *pclsid) override;
	STDMETHODIMP	Load(LPCOLESTR pszFileName, DWORD dwMode) override;
	STDMETHODIMP	IsDirty(void) override { return S_OK; };
	STDMETHODIMP	Save(LPCOLESTR /*pszFileName*/, BOOL /*fRemember*/)  override { return S_OK; };
	STDMETHODIMP	SaveCompleted(LPCOLESTR /*pszFileName*/) override { return S_OK; };
	STDMETHODIMP	GetCurFile(LPOLESTR * /*ppszFileName*/) override { return S_OK; };
	//@}

	/** \name IShellIconOverlayIdentifier
	 * IShellIconOverlayIdentifier methods
	 */
	//@{
	STDMETHODIMP	GetOverlayInfo(LPWSTR pwszIconFile, int cchMax, int *pIndex, DWORD *pdwFlags) override;
	STDMETHODIMP	GetPriority(int *pPriority) override;
	STDMETHODIMP	IsMemberOf(LPCWSTR pwszPath, DWORD dwAttrib) override;
	//@}

	/** \name IShellPropSheetExt
	 * IShellPropSheetExt methods
	 */
	//@{
	STDMETHODIMP	AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam) override;
	STDMETHODIMP	ReplacePage (UINT, LPFNADDPROPSHEETPAGE, LPARAM) override;
	//@}

	/** \name ICopyHook
	 * ICopyHook members
	 */
	//@{
	STDMETHODIMP_(UINT) CopyCallback(HWND hWnd, UINT wFunc, UINT wFlags, LPCTSTR pszSrcFile, DWORD dwSrcAttribs, LPCTSTR pszDestFile, DWORD dwDestAttribs) override;
	//@}
};
