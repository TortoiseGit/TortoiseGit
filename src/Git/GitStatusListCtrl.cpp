// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2013-2015 - TortoiseSVN

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
#include "resource.h"
#include "../TortoiseShell/resource.h"
#include "GitStatusListCtrl.h"
#include "MessageBox.h"
#include "MyMemDC.h"
#include "UnicodeUtils.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "TempFile.h"
#include "StringUtils.h"
#include "LoglistUtils.h"
#include "Git.h"
#include "GitRev.h"
#include "GitDiff.h"
#include "GitProgressDlg.h"
#include "SysImageList.h"
#include "TGitPath.h"
#include "registry.h"
#include "InputDlg.h"
#include "GitAdminDir.h"
#include "GitDataObject.h"
#include "ProgressCommands/AddProgressCommand.h"
#include "IconMenu.h"
#include "FormatMessageWrapper.h"
#include "BrowseFolder.h"
#include "SysInfo.h"
#include "SysProgressDlg.h"
#include "CreateChangelistDlg.h"
#include "GitAdminDir.h"

const UINT CGitStatusListCtrl::GITSLNM_ITEMCOUNTCHANGED
					= ::RegisterWindowMessage(L"GITSLNM_ITEMCOUNTCHANGED");
const UINT CGitStatusListCtrl::GITSLNM_NEEDSREFRESH
					= ::RegisterWindowMessage(L"GITSLNM_NEEDSREFRESH");
const UINT CGitStatusListCtrl::GITSLNM_ADDFILE
					= ::RegisterWindowMessage(L"GITSLNM_ADDFILE");
const UINT CGitStatusListCtrl::GITSLNM_CHECKCHANGED
					= ::RegisterWindowMessage(L"GITSLNM_CHECKCHANGED");
const UINT CGitStatusListCtrl::GITSLNM_ITEMCHANGED
					= ::RegisterWindowMessage(L"GITSLNM_ITEMCHANGED");

struct icompare
{
	bool operator() (const std::wstring& lhs, const std::wstring& rhs) const
	{
		// no logical comparison here: we need this sorted strictly
		return _wcsicmp(lhs.c_str(), rhs.c_str()) < 0;
	}
};

class CIShellFolderHook : public IShellFolder
{
public:
	CIShellFolderHook(LPSHELLFOLDER sf, const CTGitPathList& pathlist)
	{
		sf->AddRef();
		m_iSF = sf;
		// it seems the paths in the HDROP need to be sorted, otherwise
		// it might not work properly or even crash.
		// to get the items sorted, we just add them to a set
		for (int i = 0; i < pathlist.GetCount(); ++i)
			sortedpaths.insert(static_cast<LPCTSTR>(g_Git.CombinePath(pathlist[i].GetWinPath())));
	}

	~CIShellFolderHook() { m_iSF->Release(); }

	// IUnknown methods --------
	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, __RPC__deref_out void** ppvObject) override { return m_iSF->QueryInterface(riid, ppvObject); }
	virtual ULONG STDMETHODCALLTYPE AddRef(void) override { return m_iSF->AddRef(); }
	virtual ULONG STDMETHODCALLTYPE Release(void) override { return m_iSF->Release(); }

	// IShellFolder methods ----
	virtual HRESULT STDMETHODCALLTYPE GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT* rgfReserved, void** ppv) override;

	virtual HRESULT STDMETHODCALLTYPE CompareIDs(LPARAM lParam, __RPC__in PCUIDLIST_RELATIVE pidl1, __RPC__in PCUIDLIST_RELATIVE pidl2) override { return m_iSF->CompareIDs(lParam, pidl1, pidl2); }
	virtual HRESULT STDMETHODCALLTYPE GetDisplayNameOf(__RPC__in_opt PCUITEMID_CHILD pidl, SHGDNF uFlags, __RPC__out STRRET* pName) override { return m_iSF->GetDisplayNameOf(pidl, uFlags, pName); }
	virtual HRESULT STDMETHODCALLTYPE CreateViewObject(__RPC__in_opt HWND hwndOwner, __RPC__in REFIID riid, __RPC__deref_out_opt void** ppv) override { return m_iSF->CreateViewObject(hwndOwner, riid, ppv); }
	virtual HRESULT STDMETHODCALLTYPE EnumObjects(__RPC__in_opt HWND hwndOwner, SHCONTF grfFlags, __RPC__deref_out_opt IEnumIDList** ppenumIDList) override { return m_iSF->EnumObjects(hwndOwner, grfFlags, ppenumIDList); }
	virtual HRESULT STDMETHODCALLTYPE BindToObject(__RPC__in PCUIDLIST_RELATIVE pidl, __RPC__in_opt IBindCtx* pbc, __RPC__in REFIID riid, __RPC__deref_out_opt void** ppv) override { return m_iSF->BindToObject(pidl, pbc, riid, ppv); }
	virtual HRESULT STDMETHODCALLTYPE ParseDisplayName(__RPC__in_opt HWND hwnd, __RPC__in_opt IBindCtx* pbc, __RPC__in_string LPWSTR pszDisplayName, __reserved ULONG* pchEaten, __RPC__deref_out_opt PIDLIST_RELATIVE* ppidl, __RPC__inout_opt ULONG* pdwAttributes) override { return m_iSF->ParseDisplayName(hwnd, pbc, pszDisplayName, pchEaten, ppidl, pdwAttributes); }
	virtual HRESULT STDMETHODCALLTYPE GetAttributesOf(UINT cidl, __RPC__in_ecount_full_opt(cidl) PCUITEMID_CHILD_ARRAY apidl, __RPC__inout SFGAOF* rgfInOut) override { return m_iSF->GetAttributesOf(cidl, apidl, rgfInOut); }
	virtual HRESULT STDMETHODCALLTYPE BindToStorage(__RPC__in PCUIDLIST_RELATIVE pidl, __RPC__in_opt IBindCtx* pbc, __RPC__in REFIID riid, __RPC__deref_out_opt void** ppv) override { return m_iSF->BindToStorage(pidl, pbc, riid, ppv); }
	virtual HRESULT STDMETHODCALLTYPE SetNameOf(__in_opt HWND hwnd, __in PCUITEMID_CHILD pidl, __in LPCWSTR pszName, __in SHGDNF uFlags, __deref_opt_out PITEMID_CHILD* ppidlOut) override { return m_iSF->SetNameOf(hwnd, pidl, pszName, uFlags, ppidlOut); }

protected:
	LPSHELLFOLDER m_iSF;
	std::set<std::wstring, icompare> sortedpaths;
};

HRESULT STDMETHODCALLTYPE CIShellFolderHook::GetUIObjectOf(HWND hwndOwner, UINT cidl, PCUITEMID_CHILD_ARRAY apidl, REFIID riid, UINT* rgfReserved, void** ppv)
{
	if (InlineIsEqualGUID(riid, IID_IDataObject))
	{
		HRESULT hres = m_iSF->GetUIObjectOf(hwndOwner, cidl, apidl, IID_IDataObject, nullptr, ppv);
		if (FAILED(hres))
			return hres;

		auto idata = static_cast<LPDATAOBJECT>(*ppv);
		// the IDataObject returned here doesn't have a HDROP, so we create one ourselves and add it to the IDataObject
		// the HDROP is necessary for most context menu handlers
		int nLength = 0;
		for (auto it = sortedpaths.cbegin(); it != sortedpaths.cend(); ++it)
		{
			nLength += static_cast<int>(it->size());
			nLength += 1; // '\0' separator
		}
		int nBufferSize = sizeof(DROPFILES) + ((nLength + 5)*sizeof(TCHAR));
		auto pBuffer = std::make_unique<char[]>(nBufferSize);
		SecureZeroMemory(pBuffer.get(), nBufferSize);
		auto df = reinterpret_cast<DROPFILES*>(pBuffer.get());
		df->pFiles = sizeof(DROPFILES);
		df->fWide = 1;
		auto pFilenames = reinterpret_cast<TCHAR*>(reinterpret_cast<BYTE*>(pBuffer.get()) + sizeof(DROPFILES));
		TCHAR* pCurrentFilename = pFilenames;

		for (auto it = sortedpaths.cbegin(); it != sortedpaths.cend(); ++it)
		{
			wcscpy_s(pCurrentFilename, it->size() + 1, it->c_str());
			pCurrentFilename += it->size();
			*pCurrentFilename = '\0'; // separator between file names
			pCurrentFilename++;
		}
		*pCurrentFilename = '\0'; // terminate array
		pCurrentFilename++;
		*pCurrentFilename = '\0'; // terminate array
		STGMEDIUM medium = { 0 };
		medium.tymed = TYMED_HGLOBAL;
		medium.hGlobal = GlobalAlloc(GMEM_ZEROINIT | GMEM_MOVEABLE, nBufferSize + 20);
		if (medium.hGlobal)
		{
			LPVOID pMem = ::GlobalLock(medium.hGlobal);
			if (pMem)
			{
				memcpy(pMem, pBuffer.get(), nBufferSize);
				GlobalUnlock(medium.hGlobal);
				FORMATETC formatetc = { 0 };
				formatetc.cfFormat = CF_HDROP;
				formatetc.dwAspect = DVASPECT_CONTENT;
				formatetc.lindex = -1;
				formatetc.tymed = TYMED_HGLOBAL;
				medium.pUnkForRelease = nullptr;
				hres = idata->SetData(&formatetc, &medium, TRUE);
				return hres;
			}
		}
		return E_OUTOFMEMORY;
	}
	else
	{
		// just pass it on to the base object
		return m_iSF->GetUIObjectOf(hwndOwner, cidl, apidl, riid, rgfReserved, ppv);
	}
}

IContextMenu2 *         g_IContext2 = nullptr;
IContextMenu3 *         g_IContext3 = nullptr;
CIShellFolderHook *     g_pFolderhook = nullptr;
IShellFolder *          g_psfDesktopFolder = nullptr;
LPITEMIDLIST *          g_pidlArray = nullptr;
int                     g_pidlArrayItems = 0;

#define SHELL_MIN_CMD   10000
#define SHELL_MAX_CMD   20000

HRESULT CALLBACK dfmCallback(IShellFolder * /*psf*/, HWND /*hwnd*/, IDataObject * /*pdtobj*/, UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	switch (uMsg)
	{
	case DFM_MERGECONTEXTMENU:
		return S_OK;
	case DFM_INVOKECOMMAND:
	case DFM_INVOKECOMMANDEX:
	case DFM_GETDEFSTATICID: // Required for Windows 7 to pick a default
		return S_FALSE;
	}
	return E_NOTIMPL;
}

BEGIN_MESSAGE_MAP(CGitStatusListCtrl, CResizableColumnsListCtrl<CListCtrl>)
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
	ON_NOTIFY_REFLECT_EX(LVN_ITEMCHANGED, OnLvnItemchanged)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_DBLCLK, OnNMDblclk)
	ON_NOTIFY_REFLECT(LVN_GETINFOTIP, OnLvnGetInfoTip)
	ON_NOTIFY_REFLECT_EX(NM_CUSTOMDRAW, OnNMCustomdraw)
	ON_NOTIFY_REFLECT(LVN_GETDISPINFO, OnLvnGetdispinfo)
	ON_WM_SETCURSOR()
	ON_WM_GETDLGCODE()
	ON_NOTIFY_REFLECT(NM_RETURN, OnNMReturn)
	ON_WM_KEYDOWN()
	ON_WM_PAINT()
	ON_WM_SYSCOLORCHANGE()
	ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
END_MESSAGE_MAP()

CGitStatusListCtrl::CGitStatusListCtrl() : CResizableColumnsListCtrl<CListCtrl>()
	, m_pbCanceled(nullptr)
	, m_pStatLabel(nullptr)
	, m_pSelectButton(nullptr)
	, m_pConfirmButton(nullptr)
	, m_bBusy(false)
	, m_bWaitCursor(false)
	, m_bEmpty(false)
	, m_bShowIgnores(false)
	, m_bIgnoreRemoveOnly(false)
	, m_bCheckChildrenWithParent(false)
	, m_bUnversionedLast(true)
	, m_bHasChangeLists(false)
	, m_bHasCheckboxes(false)
	, m_bCheckIfGroupsExist(true)
	, m_bFileDropsEnabled(false)
	, m_bOwnDrag(false)
	, m_dwDefaultColumns(0)
	, m_bAscending(false)
	, m_nSortedColumn(-1)
	, m_amend(false)
	, m_bDoNotAutoselectSubmodules(false)
	, m_bNoAutoselectMissing(false)
	, m_bHasWC(true)
	, m_hwndLogicalParent(nullptr)
	, m_bHasUnversionedItems(FALSE)
	, m_nTargetCount(0)
	, m_bHasIgnoreGroup(false)
	, m_nUnversioned(0)
	, m_nNormal(0)
	, m_nModified(0)
	, m_nAdded(0)
	, m_nDeleted(0)
	, m_nConflicted(0)
	, m_nTotal(0)
	, m_nSelected(0)
	, m_nRenamed(0)
	, m_nShownUnversioned(0)
	, m_nShownModified(0)
	, m_nShownAdded(0)
	, m_nShownDeleted(0)
	, m_nShownConflicted(0)
	, m_nShownFiles(0)
	, m_nShownSubmodules(0)
	, m_dwShow(0)
	, m_bShowFolders(false)
	, m_bUpdate(false)
	, m_dwContextMenus(0)
	, m_nIconFolder(0)
	, m_nRestoreOvl(0)
	, m_pContextMenu(nullptr)
	, m_hShellMenu(nullptr)
	, m_nBackgroundImageID(0)
	, m_FileLoaded(0)
	, m_bIsRevertTheirMy(false)
	, m_nLineAdded(0)
	, m_nLineDeleted(0)
	, m_nBlockItemChangeHandler(0)
	, m_uiFont(nullptr)
	, m_bIncludedStaged(false)
{
	m_bNoAutoselectMissing = CRegDWORD(L"Software\\TortoiseGit\\AutoselectMissingFiles", FALSE) == TRUE;

	NONCLIENTMETRICS metrics = { 0 };
	metrics.cbSize = sizeof(NONCLIENTMETRICS);
	SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &metrics, FALSE);
	m_uiFont = CreateFontIndirect(&metrics.lfMessageFont);
}

CGitStatusListCtrl::~CGitStatusListCtrl()
{
	if (m_uiFont)
		DeleteObject(m_uiFont);
	ClearStatusArray();
}

HWND CGitStatusListCtrl::GetParentHWND()
{
	if (m_hwndLogicalParent)
		return m_hwndLogicalParent->GetSafeHwnd();
	auto owner = GetSafeOwner();
	if (!owner)
		return GetSafeHwnd();
	return owner->GetSafeHwnd();
}

void CGitStatusListCtrl::ClearStatusArray()
{
#if 0
	CAutoWriteLock locker(m_guard);
	for (size_t i = 0; i < m_arStatusArray.size(); ++i)
	{
		delete m_arStatusArray[i];
	}
	m_arStatusArray.clear();
#endif
}

void CGitStatusListCtrl::Init(DWORD dwColumns, const CString& sColumnInfoContainer, unsigned __int64 dwContextMenus /* = GitSLC_POPALL */, bool bHasCheckboxes /* = true */, bool bHasWC /* = true */, DWORD allowedColumns /* = 0xffffffff */)
{
	CAutoWriteLock locker(m_guard);

	m_dwDefaultColumns = dwColumns | 1;
	m_dwContextMenus = dwContextMenus;
	m_bHasCheckboxes = bHasCheckboxes;
	m_bHasWC = bHasWC;
	m_bWaitCursor = true;

	// set the extended style of the listcontrol
	DWORD exStyle = LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	exStyle |= (bHasCheckboxes ? LVS_EX_CHECKBOXES : 0);
	SetRedraw(false);
	SetExtendedStyle(exStyle);
	CResizableColumnsListCtrl::Init();

	SetWindowTheme(m_hWnd, L"Explorer", nullptr);

	m_nIconFolder = SYS_IMAGE_LIST().GetDirIconIndex();
	m_nRestoreOvl = SYS_IMAGE_LIST().AddIcon(CCommonAppUtils::LoadIconEx(IDI_RESTOREOVL, 0, 0));
	SYS_IMAGE_LIST().SetOverlayImage(m_nRestoreOvl, OVL_RESTORE);
	SetImageList(&SYS_IMAGE_LIST(), LVSIL_SMALL);

	// keep CSorter::operator() in sync!!
	static UINT standardColumnNames[GITSLC_NUMCOLUMNS]
			= { IDS_STATUSLIST_COLFILE
			, IDS_STATUSLIST_COLFILENAME
			, IDS_STATUSLIST_COLEXT
			, IDS_STATUSLIST_COLSTATUS
			, IDS_STATUSLIST_COLADD
			, IDS_STATUSLIST_COLDEL
			, IDS_STATUSLIST_COLLASTMODIFIED
			, IDS_STATUSLIST_COLSIZE
			};

	m_ColumnManager.SetNames(standardColumnNames,GITSLC_NUMCOLUMNS);
	m_ColumnManager.ReadSettings(m_dwDefaultColumns, 0xffffffff & ~(allowedColumns | m_dwDefaultColumns), sColumnInfoContainer, GITSLC_NUMCOLUMNS);
	m_ColumnManager.SetRightAlign(4);
	m_ColumnManager.SetRightAlign(5);
	m_ColumnManager.SetRightAlign(7);

	// enable file drops
	if (!m_pDropTarget)
	{
		m_pDropTarget = std::make_unique<CGitStatusListCtrlDropTarget>(this);
		RegisterDragDrop(m_hWnd, m_pDropTarget.get());
		// create the supported formats:
		FORMATETC ftetc = { 0 };
		ftetc.dwAspect = DVASPECT_CONTENT;
		ftetc.lindex = -1;
		ftetc.tymed = TYMED_HGLOBAL;
		ftetc.cfFormat = CF_HDROP;
		m_pDropTarget->AddSuportedFormat(ftetc);
	}

	SetRedraw(true);
	m_bWaitCursor = false;
}

bool CGitStatusListCtrl::SetBackgroundImage(UINT nID)
{
	m_nBackgroundImageID = nID;
	return CAppUtils::SetListCtrlBackgroundImage(GetSafeHwnd(), nID);
}

BOOL CGitStatusListCtrl::GetStatus ( const CTGitPathList* pathList
									, bool bUpdate /* = FALSE */
									, bool bShowIgnores /* = false */
									, bool bShowUnRev /* = false */
									, bool bShowLocalChangesIgnored /* = false */)
{
	CAutoWriteLock locker(m_guard);

	m_bEmpty = false;
	m_bBusy = true;
	m_bWaitCursor = true;
	Invalidate();

	m_bIsRevertTheirMy = g_Git.IsRebaseRunning() > 0;

	int mask= CGitStatusListCtrl::FILELIST_MODIFY;
	if(bShowIgnores)
		mask|= CGitStatusListCtrl::FILELIST_IGNORE;
	if(bShowUnRev)
		mask|= CGitStatusListCtrl::FILELIST_UNVER;
	if (bShowLocalChangesIgnored)
		mask |= CGitStatusListCtrl::FILELIST_LOCALCHANGESIGNORED;
	this->UpdateFileList(mask, bUpdate, pathList);

	if (pathList && m_setDirectFiles.empty())
	{
		// remember files which are selected by users so that those can be preselected
		for (int i = 0; i < pathList->GetCount(); ++i)
			if (!(*pathList)[i].IsDirectory())
				m_setDirectFiles.insert((*pathList)[i].GetGitPathString());
	}
	LoadChangelists();

#if 0
	int refetchcounter = 0;
	BOOL bRet = TRUE;
	Invalidate();
	// force the cursor to change
	POINT pt;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);

	m_mapFilenameToChecked.clear();
	//m_StatusUrlList.Clear();
	bool bHasChangelists = (m_changelists.size() > 1 || (!m_changelists.empty() && !m_bHasIgnoreGroup));
	m_changelists.clear();
	for (size_t i=0; i < m_arStatusArray.size(); i++)
	{
		FileEntry * entry = m_arStatusArray[i];
		if ( bHasChangelists && entry->checked)
		{
			// If change lists are present, remember all checked entries
			CString path = entry->GetPath().GetGitPathString();
			m_mapFilenameToChecked[path] = true;
		}
		if ( (entry->status==git_wc_status_unversioned || entry->status==git_wc_status_missing ) && entry->checked )
		{
			// The user manually selected an unversioned or missing file. We remember
			// this so that the selection can be restored when refreshing.
			CString path = entry->GetPath().GetGitPathString();
			m_mapFilenameToChecked[path] = true;
		}
		else if ( entry->status > git_wc_status_normal && !entry->checked )
		{
			// The user manually deselected a versioned file. We remember
			// this so that the deselection can be restored when refreshing.
			CString path = entry->GetPath().GetGitPathString();
			m_mapFilenameToChecked[path] = false;
		}
	}

	// use a sorted path list to make sure we fetch the status of
	// parent items first, *then* the child items (if any)
	CTGitPathList sortedPathList = pathList;
	sortedPathList.SortByPathname();
	do
	{
		bRet = TRUE;
		m_nTargetCount = 0;
		m_bHasUnversionedItems = FALSE;
		m_bHasChangeLists = false;
		m_bShowIgnores = bShowIgnores;
		m_nSortedColumn = 0;
		m_bBlock = TRUE;
		m_bBusy = true;
		m_bEmpty = false;
		Invalidate();

		// first clear possible status data left from
		// previous GetStatus() calls
		ClearStatusArray();

		m_StatusFileList = sortedPathList;

		// Since Git_client_status() returns all files, even those in
		// folders included with Git:externals we need to check if all
		// files we get here belongs to the same repository.
		// It is possible to commit changes in an external folder, as long
		// as the folder belongs to the same repository (but another path),
		// but it is not possible to commit all files if the externals are
		// from a different repository.
		//
		// To check if all files belong to the same repository, we compare the
		// UUID's - if they're identical then the files belong to the same
		// repository and can be committed. But if they're different, then
		// tell the user to commit all changes in the external folders
		// first and exit.
		CStringA sUUID;					// holds the repo UUID
		CTGitPathList arExtPaths;		// list of Git:external paths

		GitConfig config;

		m_sURL.Empty();

		m_nTargetCount = sortedPathList.GetCount();

		GitStatus status(m_pbCanceled);
		if(m_nTargetCount > 1 && sortedPathList.AreAllPathsFilesInOneDirectory())
		{
			// This is a special case, where we've been given a list of files
			// all from one folder
			// We handle them by setting a status filter, then requesting the Git status of
			// all the files in the directory, filtering for the ones we're interested in
			status.SetFilter(sortedPathList);

			// if all selected entries are files, we don't do a recursive status
			// fetching. But if only one is a directory, we have to recurse.
			git_depth_t depth = git_depth_files;
			// We have set a filter. If all selected items were files or we fetch
			// the status not recursively, then we have to include
			// ignored items too - the user has selected them
			bool bShowIgnoresRight = true;
			for (int fcindex=0; fcindex<sortedPathList.GetCount(); ++fcindex)
			{
				if (sortedPathList[fcindex].IsDirectory())
				{
					depth = git_depth_infinity;
					bShowIgnoresRight = false;
					break;
				}
			}
			if(!FetchStatusForSingleTarget(config, status, sortedPathList.GetCommonDirectory(), bUpdate, sUUID, arExtPaths, true, depth, bShowIgnoresRight))
				bRet = FALSE;
		}
		else
		{
			for(int nTarget = 0; nTarget < m_nTargetCount; nTarget++)
			{
				// check whether the path we want the status for is already fetched due to status-fetching
				// of a parent path.
				// this check is only done for file paths, because folder paths could be included already
				// but not recursively
				if (sortedPathList[nTarget].IsDirectory() || !GetListEntry(sortedPathList[nTarget]))
				{
					if(!FetchStatusForSingleTarget(config, status, sortedPathList[nTarget], bUpdate, sUUID,
						arExtPaths, false, git_depth_infinity, bShowIgnores))
					{
						bRet = FALSE;
					}
				}
			}
		}

		// remove the 'helper' files of conflicted items from the list.
		// otherwise they would appear as unversioned files.
		for (INT_PTR cind = 0; cind < m_ConflictFileList.GetCount(); ++cind)
		{
			for (size_t i=0; i < m_arStatusArray.size(); i++)
			{
				if (m_arStatusArray[i]->GetPath().IsEquivalentTo(m_ConflictFileList[cind]))
				{
					delete m_arStatusArray[i];
					m_arStatusArray.erase(m_arStatusArray.cbegin() + i);
					break;
				}
			}
		}
		refetchcounter++;
	} while(!BuildStatistics() && (refetchcounter < 2) && (*m_pbCanceled==false));

	m_bBlock = FALSE;
	m_bBusy = false;
	GetCursorPos(&pt);
	SetCursorPos(pt.x, pt.y);
	return bRet;
#endif
	m_bBusy = false;
	m_bWaitCursor = false;
	BuildStatistics();
	return TRUE;
}

// Get the show-flags bitmap value which corresponds to a particular Git status
DWORD CGitStatusListCtrl::GetShowFlagsFromGitStatus(git_wc_status_kind status)
{
	switch (status)
	{
	case git_wc_status_none:
	case git_wc_status_unversioned:
		return GITSLC_SHOWUNVERSIONED;
	case git_wc_status_ignored:
		if (!m_bShowIgnores)
			return GITSLC_SHOWDIRECTS;
		return GITSLC_SHOWDIRECTS|GITSLC_SHOWIGNORED;
	case git_wc_status_normal:
		return GITSLC_SHOWNORMAL;
	case git_wc_status_added:
		return GITSLC_SHOWADDED;
	case git_wc_status_deleted:
		return GITSLC_SHOWREMOVED;
	case git_wc_status_modified:
		return GITSLC_SHOWMODIFIED;
	case git_wc_status_conflicted:
		return GITSLC_SHOWCONFLICTED;
	default:
		// we should NEVER get here!
		ASSERT(FALSE);
		break;
	}
	return 0;
}

void CGitStatusListCtrl::Show(unsigned int dwShow, unsigned int dwCheck /*=0*/, bool /*bShowFolders*/ /* = true */,BOOL UpdateStatusList,bool UseStoredCheckStatus)
{
	m_bWaitCursor = true;
	m_bBusy = true;
	m_bEmpty = false;
	Invalidate();

	WORD langID = static_cast<WORD>(CRegStdDWORD(L"Software\\TortoiseGit\\LanguageID", GetUserDefaultLangID()));
	SetRedraw(FALSE);
	{
		CAutoWriteLock locker(m_guard);
		DeleteAllItems();
		m_nSelected = 0;

		m_nShownUnversioned = 0;
		m_nShownModified = 0;
		m_nShownAdded = 0;
		m_nShownDeleted = 0;
		m_nShownConflicted = 0;
		m_nShownFiles = 0;
		m_nShownSubmodules = 0;

		m_dwShow = dwShow;

		if (UpdateStatusList)
		{
			m_arStatusArray.clear();
			for (int i = 0; i < m_StatusFileList.GetCount(); ++i)
				m_arStatusArray.push_back(&m_StatusFileList[i]);

			for (int i = 0; i < m_UnRevFileList.GetCount(); ++i)
				m_arStatusArray.push_back(&m_UnRevFileList[i]);

			for (int i = 0; i < m_IgnoreFileList.GetCount(); ++i)
				m_arStatusArray.push_back(&m_IgnoreFileList[i]);

			for (int i = 0; i < m_LocalChangesIgnoredFileList.GetCount(); ++i)
				m_arStatusArray.push_back(&m_LocalChangesIgnoredFileList[i]);
		}
		PrepareGroups();
		m_arListArray.clear();
		m_arListArray.reserve(m_arStatusArray.size());
		if (m_nSortedColumn >= 0)
		{
			CSorter predicate(&m_ColumnManager, m_nSortedColumn, m_bAscending);
			std::stable_sort(m_arStatusArray.begin(), m_arStatusArray.end(), predicate);
		}

		int index = 0;
		for (size_t i = 0; i < m_arStatusArray.size(); ++i)
		{
			//set default checkbox status
			auto entry = const_cast<CTGitPath*>(m_arStatusArray[i]);
			CString path = entry->GetGitPathString();
			if (!m_mapFilenameToChecked.empty() && m_mapFilenameToChecked.find(path) != m_mapFilenameToChecked.end())
				entry->m_Checked = m_mapFilenameToChecked[path];
			else if (!UseStoredCheckStatus)
			{
				bool autoSelectSubmodules = !(entry->IsDirectory() && m_bDoNotAutoselectSubmodules);
				if (((entry->m_Action & dwCheck) &&
					!(m_bNoAutoselectMissing && entry->m_Action & CTGitPath::LOGACTIONS_MISSING) ||
					dwShow & GITSLC_SHOWDIRECTFILES && m_setDirectFiles.find(path) != m_setDirectFiles.end()
					) && autoSelectSubmodules)
				{
					auto it = m_pathToChangelist.find(path);
					if ((it != m_pathToChangelist.end()) && (it->second.Compare(GITSLC_IGNORECHANGELIST) == 0))
						entry->m_Checked = false; // is in ignore-on-commit
					else 
						entry->m_Checked = true;
				}
				else
					entry->m_Checked = false;
				m_mapFilenameToChecked[path] = entry->m_Checked;
			}

			if (entry->m_Action & dwShow)
			{
				AddEntry(i, entry, langID, index);
				index++;
			}
		}
	}

	AdjustColumnWidths();

	SetRedraw(TRUE);
	GetStatisticsString();

	CHeaderCtrl * pHeader = GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (m_nSortedColumn >= 0)
	{
		pHeader->GetItem(m_nSortedColumn, &HeaderItem);
		HeaderItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(m_nSortedColumn, &HeaderItem);
	}

	RestoreScrollPos();

	m_bWaitCursor = false;
	m_bBusy = false;
	m_bEmpty = (GetItemCount() == 0);
	Invalidate();

	this->BuildStatistics();

#if 0

	m_bShowFolders = bShowFolders;

	int nTopIndex = GetTopIndex();
	POSITION posSelectedEntry = GetFirstSelectedItemPosition();
	int nSelectedEntry = 0;
	if (posSelectedEntry)
		nSelectedEntry = GetNextSelectedItem(posSelectedEntry);
	SetRedraw(FALSE);
	DeleteAllItems();

	PrepareGroups();

	m_arListArray.clear();

	m_arListArray.reserve(m_arStatusArray.size());
	SetItemCount (static_cast<int>(m_arStatusArray.size()));

	int listIndex = 0;
	for (size_t i=0; i < m_arStatusArray.size(); ++i)
	{
		FileEntry * entry = m_arStatusArray[i];
		if ((entry->inexternal) && (!(dwShow & SVNSLC_SHOWINEXTERNALS)))
			continue;
		if ((entry->differentrepo || entry->isNested) && (! (dwShow & SVNSLC_SHOWEXTERNALFROMDIFFERENTREPO)))
			continue;
		if (entry->IsFolder() && (!bShowFolders))
			continue;	// don't show folders if they're not wanted.

#if 0
		git_wc_status_kind status = GitStatus::GetMoreImportant(entry->status, entry->remotestatus);
		DWORD showFlags = GetShowFlagsFromGitStatus(status);
		if (entry->switched)
			showFlags |= SVNSLC_SHOWSWITCHED;
		if (!entry->changelist.IsEmpty())
			showFlags |= SVNSLC_SHOWINCHANGELIST;
#endif
		bool bAllowCheck = ((entry->changelist.Compare(GITSLC_IGNORECHANGELIST) != 0)
			&& (m_bCheckIfGroupsExist || (m_changelists.empty() || (m_changelists.size() == 1 && m_bHasIgnoreGroup))));

		// status_ignored is a special case - we must have the 'direct' flag set to add a status_ignored item
#if 0
		if (status != Git_wc_status_ignored || (entry->direct) || (dwShow & GitSLC_SHOWIGNORED))
		{
			if ((!entry->IsFolder()) && (status == Git_wc_status_deleted) && (dwShow & SVNSLC_SHOWREMOVEDANDPRESENT))
			{
				if (PathFileExists(entry->GetPath().GetWinPath()))
				{
					m_arListArray.push_back(i);
					if ((dwCheck & SVNSLC_SHOWREMOVEDANDPRESENT)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
					{
						if (bAllowCheck)
							entry->checked = true;
					}
					AddEntry(entry, langID, listIndex++);
				}
			}
			else if ((dwShow & showFlags)||((dwShow & SVNSLC_SHOWDIRECTFILES)&&(entry->direct)&&(!entry->IsFolder())))
			{
				m_arListArray.push_back(i);
				if ((dwCheck & showFlags)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
				{
					if (bAllowCheck)
						entry->checked = true;
				}
				AddEntry(entry, langID, listIndex++);
			}
			else if ((dwShow & showFlags)||((dwShow & SVNSLC_SHOWDIRECTFOLDER)&&(entry->direct)&&entry->IsFolder()))
			{
				m_arListArray.push_back(i);
				if ((dwCheck & showFlags)||((dwCheck & SVNSLC_SHOWDIRECTS)&&(entry->direct)))
				{
					if (bAllowCheck)
						entry->checked = true;
				}
				AddEntry(entry, langID, listIndex++);
			}
		}
#endif
	}

	SetItemCount(listIndex);

	m_ColumnManager.UpdateRelevance (m_arStatusArray, m_arListArray);

	AdjustColumnWidths();

	SetRedraw(TRUE);
	GetStatisticsString();

	CHeaderCtrl * pHeader = GetHeaderCtrl();
	HDITEM HeaderItem = {0};
	HeaderItem.mask = HDI_FORMAT;
	for (int i=0; i<pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (m_nSortedColumn)
	{
		pHeader->GetItem(m_nSortedColumn, &HeaderItem);
		HeaderItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(m_nSortedColumn, &HeaderItem);
	}

	if (nSelectedEntry)
	{
		SetItemState(nSelectedEntry, LVIS_SELECTED, LVIS_SELECTED);
		EnsureVisible(nSelectedEntry, false);
	}
	else
	{
		// Restore the item at the top of the list.
		for (int i=0;GetTopIndex() != nTopIndex;i++)
		{
			if ( !EnsureVisible(nTopIndex+i,false) )
				break;
		}
	}

	m_bEmpty = (GetItemCount() == 0);
	Invalidate();
#endif
}

void CGitStatusListCtrl::StoreScrollPos()
{
	m_sScrollPos.enabled = true;
	m_sScrollPos.nTopIndex = GetTopIndex();
	m_sScrollPos.selMark = GetSelectionMark();
	POSITION posSelectedEntry = GetFirstSelectedItemPosition();
	m_sScrollPos.nSelectedEntry = 0;
	if (posSelectedEntry)
		m_sScrollPos.nSelectedEntry = GetNextSelectedItem(posSelectedEntry);
}

void CGitStatusListCtrl::RestoreScrollPos()
{
	if (!m_sScrollPos.enabled || CRegDWORD(L"Software\\TortoiseGit\\RememberFileListPosition", TRUE) != TRUE)
		return;

	if (m_sScrollPos.nSelectedEntry)
	{
		SetItemState(m_sScrollPos.nSelectedEntry, LVIS_SELECTED, LVIS_SELECTED);
		EnsureVisible(m_sScrollPos.nSelectedEntry, false);
	}
	else
	{
		// Restore the item at the top of the list.
		for (int i = 0; GetTopIndex() != m_sScrollPos.nTopIndex; ++i)
		{
			if (!EnsureVisible(m_sScrollPos.nTopIndex + i, false))
				break;
		}
	}
	if (m_sScrollPos.selMark >= 0)
	{
		SetSelectionMark(m_sScrollPos.selMark);
		SetItemState(m_sScrollPos.selMark, LVIS_FOCUSED, LVIS_FOCUSED);
	}
	m_sScrollPos.enabled = false;
}

int CGitStatusListCtrl::GetColumnIndex(int mask)
{
	for (int i = 0; i < 32; ++i)
		if(mask&0x1)
			return i;
		else
			mask=mask>>1;
	return -1;
}

CString CGitStatusListCtrl::GetCellText(int listIndex, int column)
{
	static CString from(MAKEINTRESOURCE(IDS_STATUSLIST_FROM));
	static bool abbreviateRenamings(static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\AbbreviateRenamings", FALSE)) == TRUE);
	static bool relativeTimes = (CRegDWORD(L"Software\\TortoiseGit\\RelativeTimes", FALSE) != FALSE);
	static const CString empty;

	CAutoReadLock locker(m_guard);
	const auto* entry = GetListEntry(listIndex);
	if (!entry)
		return empty;

	switch (column)
	{
	case 0: // relative path
		// similar code in FileDiffDlg.cpp::GetFilename
		if (!(entry->m_Action & (CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_COPY) && !entry->GetGitOldPathString().IsEmpty()))
			return entry->GetGitPathString();

		if (!abbreviateRenamings)
		{
			CString entryname = entry->GetGitPathString();
			entryname += L' ';
			// relative path
			entryname.AppendFormat(from, static_cast<LPCTSTR>(entry->GetGitOldPathString()));
			return entryname;
		}

		return entry->GetAbbreviatedRename();

	case 1: // GITSLC_COLFILENAME
		return entry->GetFileOrDirectoryName();

	case 2: // GITSLC_COLEXT
		return entry->GetFileExtension();

	case 3: // GITSLC_COLSTATUS
		return entry->GetActionName();

	case 4: // GITSLC_COLADD
		return entry->m_StatAdd;

	case 5: // GITSLC_COLDEL
		return entry->m_StatDel;

	case 6: // GITSLC_COLMODIFICATIONDATE
		if (!(entry->m_Action & CTGitPath::LOGACTIONS_DELETED) && m_ColumnManager.IsRelevant(GetColumnIndex(GITSLC_COLMODIFICATIONDATE)))
		{
			__int64 filetime = entry->GetLastWriteTime();
			if (filetime)
				return CLoglistUtils::FormatDateAndTime(CTime(CGit::filetime_to_time_t(filetime)), DATE_SHORTDATE, true, relativeTimes);
		}
		return empty;

	case 7: // GITSLC_COLSIZE
		if (!(entry->IsDirectory() || !m_ColumnManager.IsRelevant(GetColumnIndex(GITSLC_COLSIZE))))
		{
			TCHAR buf[100] = { 0 };
			StrFormatByteSize64(entry->GetFileSize(), buf, 100);
			return buf;
		}
		return empty;

#if 0
	default: // user-defined properties
		if (column < m_ColumnManager.GetColumnCount())
		{
			assert(m_ColumnManager.IsUserProp(column));

			const CString& name = m_ColumnManager.GetName(column);
			auto propEntry = m_PropertyMap.find(entry->GetPath());
			if (propEntry != m_PropertyMap.end())
			{
				if (propEntry->second.HasProperty(name))
				{
					const CString& propVal = propEntry->second[name];
					return propVal.IsEmpty()
						? m_sNoPropValueText
						: propVal;
				}
			}
		}
#endif
	}
	return empty;
}

void CGitStatusListCtrl::AddEntry(size_t arStatusArrayIndex, CTGitPath * GitPath, WORD /*langID*/, int listIndex)
{
	CAutoWriteLock locker(m_guard);
	ScopedInDecrement blocker(m_nBlockItemChangeHandler);
	CString path = GitPath->GetGitPathString();

	// Load the icons *now* so the icons are cached when showing them later in the
	// WM_PAINT handler.
	// Problem is that (at least on Win10), loading the icons in the WM_PAINT
	// handler triggers an OLE operation, which should not happen in WM_PAINT at all
	// (see ..\VC\atlmfc\src\mfc\olemsgf.cpp, COleMessageFilter::OnMessagePending() for details about this)
	// By loading the icons here, they get cached and the OLE operation won't happen
	// later in the WM_PAINT handler.
	// This solves the 'hang' which happens in the commit dialog if images are
	// shown in the file list.
	int icon_idx = 0;
	if (GitPath->IsDirectory())
	{
		icon_idx = m_nIconFolder;
		m_nShownSubmodules++;
	}
	else
	{
		icon_idx = SYS_IMAGE_LIST().GetPathIconIndex(*GitPath);
		m_nShownFiles++;
	}
	switch (GitPath->m_Action)
	{
	case CTGitPath::LOGACTIONS_ADDED:
	case CTGitPath::LOGACTIONS_COPY:
		m_nShownAdded++;
		break;
	case CTGitPath::LOGACTIONS_DELETED:
	case CTGitPath::LOGACTIONS_MISSING:
	case CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING:
		m_nShownDeleted++;
		break;
	case CTGitPath::LOGACTIONS_REPLACED:
	case CTGitPath::LOGACTIONS_MODIFIED:
	case CTGitPath::LOGACTIONS_MERGED:
		m_nShownModified++;
		break;
	case CTGitPath::LOGACTIONS_UNMERGED:
	case CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_ADDED:
	case CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_MODIFIED:
		m_nShownConflicted++;
		break;
	case CTGitPath::LOGACTIONS_UNVER:
		m_nShownUnversioned++;
		break;
	default:
		m_nShownUnversioned++;
		break;
	}

	LVITEM lvItem = { 0 };
	lvItem.iItem = listIndex;
	lvItem.lParam = reinterpret_cast<LPARAM>(GitPath);
	lvItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
	lvItem.pszText = LPSTR_TEXTCALLBACK;
	lvItem.stateMask = LVIS_OVERLAYMASK;
	if (m_restorepaths.find(GitPath->GetWinPathString()) != m_restorepaths.end())
		lvItem.state = INDEXTOOVERLAYMASK(OVL_RESTORE);
	lvItem.iImage = icon_idx;
	InsertItem(&lvItem);

	m_arListArray.push_back(arStatusArrayIndex);

	SetCheck(listIndex, GitPath->m_Checked);
	if (GitPath->m_Checked)
		m_nSelected++;

	SetItemGroup(listIndex, GitPath);
}

int CGitStatusListCtrl::GetChangeListIdForPath(const CTGitPath* GitPath)
{
	auto pathChangelistIt = m_pathToChangelist.find(GitPath->GetGitPathString());
	if (pathChangelistIt == m_pathToChangelist.cend())
		return -1;

	auto it = m_changelists.find(pathChangelistIt->second);
	if (it == m_changelists.cend())
		return -1;

	return it->second;
}

bool CGitStatusListCtrl::SetItemGroup(int item, const CTGitPath* pGitPath)
{
	CAutoWriteLock locker(m_guard);

	int groupindex = GetChangeListIdForPath(pGitPath);
	if (groupindex == -1)
	{
		if ((pGitPath->m_Action & CTGitPath::LOGACTIONS_SKIPWORKTREE) || (pGitPath->m_Action & CTGitPath::LOGACTIONS_ASSUMEVALID))
			groupindex = 3;
		else if (pGitPath->m_Action & CTGitPath::LOGACTIONS_IGNORE)
			groupindex = 2;
		else if (pGitPath->m_Action & CTGitPath::LOGACTIONS_UNVER)
			groupindex = 1;
		else
			groupindex = pGitPath->m_ParentNo & (PARENT_MASK | MERGE_MASK);
	}

	if (groupindex < 0)
		return false;
	LVITEM i = {0};
	i.mask = LVIF_GROUPID;
	i.iItem = item;
	i.iSubItem = 0;
	i.iGroupId = groupindex;

	return !!SetItem(&i);
}

void CGitStatusListCtrl::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;

	CAutoReadWeakLock lock(m_guard);
	if (!lock.IsAcquired())
		return;

	if (m_arStatusArray.empty())
		return;

	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;
	Show(m_dwShow, 0, m_bShowFolders,false,true);
}

BOOL CGitStatusListCtrl::OnLvnItemchanged(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;
	CWnd* pParent = GetLogicalParent();
	if (pParent && pParent->GetSafeHwnd())
		pParent->SendMessage(GITSLNM_ITEMCHANGED, pNMLV->iItem);

	if (m_nBlockItemChangeHandler)
		return FALSE;

	if ((pNMLV->uNewState==0)||(pNMLV->uNewState & LVIS_SELECTED)||(pNMLV->uNewState & LVIS_FOCUSED))
		return FALSE;

	CAutoWriteWeakLock writeLock(m_guard);
	if (!writeLock.IsAcquired())
	{
		NotifyCheck();
		return FALSE;
	}

	bool bSelected = !!(ListView_GetItemState(m_hWnd, pNMLV->iItem, LVIS_SELECTED) & LVIS_SELECTED);
	int nListItems = GetItemCount();

	// was the item checked?

	if (GetCheck(pNMLV->iItem))
	{
		{
			ScopedInDecrement blocker(m_nBlockItemChangeHandler);
			CheckEntry(pNMLV->iItem, nListItems);
		}
		if (bSelected)
		{
			ScopedInDecrement blocker(m_nBlockItemChangeHandler);
			POSITION pos = GetFirstSelectedItemPosition();
			int index;
			while ((index = GetNextSelectedItem(pos)) >= 0)
			{
				if (index != pNMLV->iItem)
					CheckEntry(index, nListItems);
			}
		}
	}
	else
	{
		{
			ScopedInDecrement blocker(m_nBlockItemChangeHandler);
			UncheckEntry(pNMLV->iItem, nListItems);
		}
		if (bSelected)
		{
			ScopedInDecrement blocker(m_nBlockItemChangeHandler);
			POSITION pos = GetFirstSelectedItemPosition();
			int index;
			while ((index = GetNextSelectedItem(pos)) >= 0)
			{
				if (index != pNMLV->iItem)
					UncheckEntry(index, nListItems);
			}
		}
	}

	GetStatisticsString();
	NotifyCheck();

	return FALSE;
}

void CGitStatusListCtrl::CheckEntry(int index, int /*nListItems*/)
{
	CAutoWriteLock locker(m_guard);
	auto path = GetListEntry(index);
	if (!path)
		return;
	m_mapFilenameToChecked[path->GetGitPathString()] = true;
	SetCheck(index, TRUE);
	// if an unversioned item was checked, then we need to check if
	// the parent folders are unversioned too. If the parent folders actually
	// are unversioned, then check those too.
#if 0
	if (entry->status == git_wc_status_unversioned)
	{
		// we need to check the parent folder too
		const CTGitPath& folderpath = entry->path;
		for (int i=0; i< nListItems; ++i)
		{
			FileEntry * testEntry = GetListEntry(i);
			ASSERT(testEntry);
			if (!testEntry)
				continue;
			if (!testEntry->checked)
			{
				if (testEntry->path.IsAncestorOf(folderpath) && (!testEntry->path.IsEquivalentTo(folderpath)))
				{
					SetEntryCheck(testEntry,i,true);
					m_nSelected++;
				}
			}
		}
	}
	bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
	if ( (entry->status == git_wc_status_deleted) || (m_bCheckChildrenWithParent) || (bShift) )
	{
		// if a deleted folder gets checked, we have to check all
		// children of that folder too.
		if (entry->path.IsDirectory())
			SetCheckOnAllDescendentsOf(entry, true);

		// if a deleted file or folder gets checked, we have to
		// check all parents of this item too.
		for (int i=0; i<nListItems; ++i)
		{
			FileEntry * testEntry = GetListEntry(i);
			ASSERT(testEntry);
			if (!testEntry)
				continue;
			if (!testEntry->checked)
			{
				if (testEntry->path.IsAncestorOf(entry->path) && (!testEntry->path.IsEquivalentTo(entry->path)))
				{
					if ((testEntry->status == git_wc_status_deleted)||(m_bCheckChildrenWithParent))
					{
						SetEntryCheck(testEntry,i,true);
						m_nSelected++;
						// now we need to check all children of this parent folder
						SetCheckOnAllDescendentsOf(testEntry, true);
					}
				}
			}
		}
	}
#endif
	if ( !path->m_Checked )
	{
		path->m_Checked = TRUE;
		m_nSelected++;
	}
}

void CGitStatusListCtrl::UncheckEntry(int index, int /*nListItems*/)
{
	CAutoWriteLock locker(m_guard);
	auto path = GetListEntry(index);
	if (!path)
		return;
	SetCheck(index, FALSE);
	m_mapFilenameToChecked[path->GetGitPathString()] = false;
	// item was unchecked
#if 0
	if (entry->path.IsDirectory())
	{
		// disable all files within an unselected folder, except when unchecking a folder with property changes
		bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
		if ( (entry->status != git_wc_status_modified) || (bShift) )
		{
			SetCheckOnAllDescendentsOf(entry, false);
		}
	}
	else if (entry->status == git_wc_status_deleted)
	{
		// a "deleted" file was unchecked, so uncheck all parent folders
		// and all children of those parents
		for (int i=0; i<nListItems; i++)
		{
			FileEntry * testEntry = GetListEntry(i);
			ASSERT(testEntry);
			if (!testEntry)
				continue;
			if (testEntry->checked)
			{
				if (testEntry->path.IsAncestorOf(entry->path))
				{
					if (testEntry->status == git_wc_status_deleted)
					{
						SetEntryCheck(testEntry,i,false);
						m_nSelected--;

						SetCheckOnAllDescendentsOf(testEntry, false);
					}
				}
			}
		}
	}
#endif
	if ( path->m_Checked )
	{
		path->m_Checked  = FALSE;
		m_nSelected--;
	}
}
void CGitStatusListCtrl::BuildStatistics()
{
	CAutoReadLock locker(m_guard);

	// now gather some statistics
	m_nUnversioned = 0;
	m_nNormal = 0;
	m_nModified = 0;
	m_nAdded = 0;
	m_nDeleted = 0;
	m_nConflicted = 0;
	m_nTotal = 0;
	m_nSelected = 0;
	m_nLineAdded = 0;
	m_nLineDeleted = 0;
	m_nRenamed = 0;

	for (size_t i = 0; i < m_arStatusArray.size(); ++i)
	{
		int status = m_arStatusArray[i]->m_Action;

		m_nLineAdded += _wtol(m_arStatusArray[i]->m_StatAdd);
		m_nLineDeleted += _wtol(m_arStatusArray[i]->m_StatDel);

		if(status&(CTGitPath::LOGACTIONS_ADDED|CTGitPath::LOGACTIONS_COPY))
			m_nAdded++;

		if(status&CTGitPath::LOGACTIONS_DELETED)
			m_nDeleted++;

		if(status&(CTGitPath::LOGACTIONS_REPLACED|CTGitPath::LOGACTIONS_MODIFIED))
			m_nModified++;

		if(status&CTGitPath::LOGACTIONS_UNMERGED)
			m_nConflicted++;

		if(status&(CTGitPath::LOGACTIONS_IGNORE|CTGitPath::LOGACTIONS_UNVER))
			m_nUnversioned++;

		if(status&(CTGitPath::LOGACTIONS_REPLACED))
			m_nRenamed++;

		if (m_arStatusArray[i]->m_Checked)
			m_nSelected++;
	}
}

int CGitStatusListCtrl::GetGroupFromPoint(POINT * ppt)
{
	// the point must be relative to the upper left corner of the control

	if (!ppt)
		return -1;
	if (!IsGroupViewEnabled())
		return -1;

	POINT pt = *ppt;
	pt.x = 10;
	UINT flags = 0;
	int nItem = -1;
	RECT rc;
	GetWindowRect(&rc);
	while (((flags & LVHT_BELOW) == 0)&&(pt.y < rc.bottom))
	{
		nItem = HitTest(pt, &flags);
		if ((flags & LVHT_ONITEM)||(flags & LVHT_EX_GROUP_HEADER))
		{
			// the first item below the point

			// check if the point is too much right (i.e. if the point
			// is farther to the right than the width of the item)
			RECT r;
			GetItemRect(nItem, &r, LVIR_LABEL);
			if (ppt->x > r.right)
				return -1;

			LVITEM lv = {0};
			lv.mask = LVIF_GROUPID;
			lv.iItem = nItem;
			GetItem(&lv);
			int groupID = lv.iGroupId;
			// now we search upwards and check if the item above this one
			// belongs to another group. If it belongs to the same group,
			// we're not over a group header
			while (pt.y >= 0)
			{
				pt.y -= 2;
				nItem = HitTest(pt, &flags);
				if ((flags & LVHT_ONITEM)&&(nItem >= 0))
				{
					// the first item below the point
					LVITEM lv2 = {0};
					lv2.mask = LVIF_GROUPID;
					lv2.iItem = nItem;
					GetItem(&lv2);
					if (lv2.iGroupId != groupID)
						return groupID;
					else
						return -1;
				}
			}
			if (pt.y < 0)
				return groupID;
			return -1;
		}
		pt.y += 2;
	};
	return -1;
}

void CGitStatusListCtrl::OnContextMenuGroup(CWnd * /*pWnd*/, CPoint point)
{
	POINT clientpoint = point;
	ScreenToClient(&clientpoint);
	if ((IsGroupViewEnabled())&&(GetGroupFromPoint(&clientpoint) >= 0))
	{
		CAutoReadWeakLock readLock(m_guard);
		if (!readLock.IsAcquired())
			return;

		CMenu popup;
		if (popup.CreatePopupMenu())
		{
			CString temp;
			temp.LoadString(IDS_STATUSLIST_CHECKGROUP);
			popup.AppendMenu(MF_STRING | MF_ENABLED, IDGITLC_CHECKGROUP, temp);
			temp.LoadString(IDS_STATUSLIST_UNCHECKGROUP);
			popup.AppendMenu(MF_STRING | MF_ENABLED, IDGITLC_UNCHECKGROUP, temp);
			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
			bool bCheck = false;
			switch (cmd)
			{
			case IDGITLC_CHECKGROUP:
				bCheck = true;
				// fall through here
			case IDGITLC_UNCHECKGROUP:
				{
					int group = GetGroupFromPoint(&clientpoint);
					// go through all items and check/uncheck those assigned to the group
					// but block the OnLvnItemChanged handler
					for (int i=0; i<GetItemCount(); ++i)
					{
						LVITEM lv = { 0 };
						lv.mask = LVIF_GROUPID;
						lv.iItem = i;
						GetItem(&lv);

						if (lv.iGroupId == group)
						{
							auto entry = GetListEntry(i);
							if (entry)
							{
								bool bOldCheck = entry->m_Checked;
								SetEntryCheck(entry, i, bCheck);
								if (bCheck != bOldCheck)
								{
									if (bCheck)
										m_nSelected++;
									else
										m_nSelected--;
								}
							}
						}

					}
					GetStatisticsString();
					NotifyCheck();
				}
				break;
			}
		}
	}
}

void CGitStatusListCtrl::OnContextMenuList(CWnd * pWnd, CPoint point)
{
	bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);

	CAutoWriteWeakLock writeLock(m_guard);
	if (!writeLock.IsAcquired())
		return;

	auto selectedCount = GetSelectedCount();
	int selIndex = GetSelectionMark();
	int selSubitem = -1;
	if (selectedCount > 0)
	{
		CPoint pt = point;
		ScreenToClient(&pt);
		LVHITTESTINFO hittest = { 0 };
		hittest.flags = LVHT_ONITEM;
		hittest.pt = pt;
		if (this->SubItemHitTest(&hittest) >= 0)
			selSubitem = hittest.iSubItem;
	}
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		GetItemRect(selIndex, &rect, LVIR_LABEL);
		ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	if (selectedCount == 0 && m_bHasCheckboxes)
	{
		// nothing selected could mean the context menu is requested for
		// a group header
		OnContextMenuGroup(pWnd, point);
	}
	else if (selIndex >= 0)
	{
		auto filepath = GetListEntry(selIndex);
		if (!filepath)
			return;

		//const CTGitPath& filepath = entry->path;
		int wcStatus = filepath->m_Action;
		// entry is selected, now show the popup menu
		CIconMenu popup;
		CMenu changelistSubMenu;
		CMenu ignoreSubMenu;
		CIconMenu clipSubMenu;
		CMenu shellMenu;
		if (popup.CreatePopupMenu())
		{
			//Add Menu for version controlled file

			if (selectedCount > 0 && (wcStatus & CTGitPath::LOGACTIONS_UNMERGED))
			{
				if (selectedCount == 1 && (m_dwContextMenus & GITSLC_POPCONFLICT))
				{
					popup.AppendMenuIcon(IDGITLC_EDITCONFLICT, IDS_MENUCONFLICT, IDI_CONFLICT);
					popup.SetDefaultItem(IDGITLC_EDITCONFLICT, FALSE);
				}
				if (m_dwContextMenus & GITSLC_POPRESOLVE)
				{
					popup.AppendMenuIcon(IDGITLC_RESOLVECONFLICT, IDS_STATUSLIST_CONTEXT_RESOLVED, IDI_RESOLVE);
					CString tmp, mineTitle, theirsTitle;
					CAppUtils::GetConflictTitles(nullptr, mineTitle, theirsTitle, m_bIsRevertTheirMy);
					tmp.Format(IDS_SVNPROGRESS_MENURESOLVEUSING, static_cast<LPCTSTR>(theirsTitle));
					if (m_bIsRevertTheirMy)
					{
						popup.AppendMenuIcon(IDGITLC_RESOLVEMINE, tmp, IDI_RESOLVE);
						tmp.Format(IDS_SVNPROGRESS_MENURESOLVEUSING, static_cast<LPCTSTR>(mineTitle));
						popup.AppendMenuIcon(IDGITLC_RESOLVETHEIRS, tmp, IDI_RESOLVE);
					}
					else
					{
						popup.AppendMenuIcon(IDGITLC_RESOLVETHEIRS, tmp, IDI_RESOLVE);
						tmp.Format(IDS_SVNPROGRESS_MENURESOLVEUSING, static_cast<LPCTSTR>(mineTitle));
						popup.AppendMenuIcon(IDGITLC_RESOLVEMINE, tmp, IDI_RESOLVE);
					}
				}
				if ((m_dwContextMenus & GITSLC_POPCONFLICT)||(m_dwContextMenus & GITSLC_POPRESOLVE))
					popup.AppendMenu(MF_SEPARATOR);
			}

			if (selectedCount > 0)
			{
				if (wcStatus & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE))
				{
					if (m_dwContextMenus & GITSLC_POPADD)
					{
						popup.AppendMenuIcon(IDGITLC_ADD, IDS_STATUSLIST_CONTEXT_ADD, IDI_ADD);
						if (!filepath->IsDirectory() && bShift)
						{
							popup.AppendMenuIcon(IDGITLC_ADD_EXE, IDS_STATUSLIST_CONTEXT_ADD_EXE, IDI_ADD);
							popup.AppendMenuIcon(IDGITLC_ADD_LINK, IDS_STATUSLIST_CONTEXT_ADD_LINK, IDI_ADD);
						}
					}
					if (m_dwContextMenus & GITSLC_POPCOMMIT)
						popup.AppendMenuIcon(IDGITLC_COMMIT, IDS_STATUSLIST_CONTEXT_COMMIT, IDI_COMMIT);
				}
			}

			if (!(wcStatus & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE)) && !((wcStatus & CTGitPath::LOGACTIONS_MISSING) && wcStatus != (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED) && wcStatus != (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MODIFIED)) && selectedCount > 0)
			{
				bool bEntryAdded = false;
				if (m_dwContextMenus & GITSLC_POPCOMPAREWITHBASE)
				{
					if(filepath->m_ParentNo & MERGE_MASK)
						popup.AppendMenuIcon(IDGITLC_COMPARE, IDS_TREE_DIFF, IDI_DIFF);
					else
						popup.AppendMenuIcon(IDGITLC_COMPARE, IDS_LOG_COMPAREWITHBASE, IDI_DIFF);

					if (!(wcStatus & (CTGitPath::LOGACTIONS_UNMERGED)) || selectedCount != 1)
						popup.SetDefaultItem(IDGITLC_COMPARE, FALSE);
					bEntryAdded = true;
				}

				if (!g_Git.IsInitRepos() && (m_dwContextMenus & GITSLC_POPGNUDIFF))
				{
					popup.AppendMenuIcon(IDGITLC_GNUDIFF1, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
					bEntryAdded = true;
				}

				if ((m_dwContextMenus & this->GetContextMenuBit(IDGITLC_COMPAREWC)) && m_bHasWC)
				{
					if (!m_CurrentVersion.IsEmpty())
					{
						popup.AppendMenuIcon(IDGITLC_COMPAREWC, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
						bEntryAdded = true;
					}
				}

				if (bEntryAdded)
					popup.AppendMenu(MF_SEPARATOR);
			}

			if (!m_Rev1.IsEmpty() && !m_Rev2.IsEmpty())
			{
				if (m_dwContextMenus & this->GetContextMenuBit(IDGITLC_COMPARETWOREVISIONS))
				{
					popup.AppendMenuIcon(IDGITLC_COMPARETWOREVISIONS, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
					popup.SetDefaultItem(IDGITLC_COMPARETWOREVISIONS, FALSE);
				}
				if (m_dwContextMenus & this->GetContextMenuBit(IDGITLC_GNUDIFF2REVISIONS))
					popup.AppendMenuIcon(IDGITLC_GNUDIFF2REVISIONS, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
			}

			//Select Multi item
			if (selectedCount > 0)
			{
				if (selectedCount == 2 && (m_dwContextMenus & GITSLC_POPCOMPARETWOFILES))
				{
					POSITION pos = GetFirstSelectedItemPosition();
					int index = GetNextSelectedItem(pos);
					if (index >= 0)
					{
						auto entry2 = GetListEntry(index);
						bool firstEntryExistsAndIsFile = entry2 && !entry2->IsDirectory();
						index = GetNextSelectedItem(pos);
						if (index >= 0)
						{
							entry2 = GetListEntry(index);
							if (firstEntryExistsAndIsFile && entry2 && !entry2->IsDirectory())
								popup.AppendMenuIcon(IDGITLC_COMPARETWOFILES, IDS_STATUSLIST_CONTEXT_COMPARETWOFILES, IDI_DIFF);
						}
					}
				}
			}

			if (selectedCount > 0 && (!(wcStatus & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE))) && m_bHasWC)
			{
				if ((m_dwContextMenus & GITSLC_POPCOMMIT) && m_CurrentVersion.IsEmpty() && !(wcStatus & (CTGitPath::LOGACTIONS_SKIPWORKTREE | CTGitPath::LOGACTIONS_ASSUMEVALID)))
					popup.AppendMenuIcon(IDGITLC_COMMIT, IDS_STATUSLIST_CONTEXT_COMMIT, IDI_COMMIT);

				if ((m_dwContextMenus & GITSLC_POPREVERT) && m_CurrentVersion.IsEmpty())
					popup.AppendMenuIcon(IDGITLC_REVERT, IDS_MENUREVERT, IDI_REVERT);

				if ((m_dwContextMenus & GITSLC_POPSKIPWORKTREE) && m_CurrentVersion.IsEmpty() && !(wcStatus & (CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_SKIPWORKTREE)))
					popup.AppendMenuIcon(IDGITLC_SKIPWORKTREE, IDS_STATUSLIST_SKIPWORKTREE);

				if ((m_dwContextMenus & GITSLC_POPASSUMEVALID) && m_CurrentVersion.IsEmpty() && !(wcStatus & (CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_UNMERGED | CTGitPath::LOGACTIONS_ASSUMEVALID)))
					popup.AppendMenuIcon(IDGITLC_ASSUMEVALID, IDS_MENUASSUMEVALID);

				if ((m_dwContextMenus & GITSLC_POPUNSETIGNORELOCALCHANGES) && m_CurrentVersion.IsEmpty() && (wcStatus & (CTGitPath::LOGACTIONS_SKIPWORKTREE | CTGitPath::LOGACTIONS_ASSUMEVALID)))
					popup.AppendMenuIcon(IDGITLC_UNSETIGNORELOCALCHANGES, IDS_STATUSLIST_UNSETIGNORELOCALCHANGES);

				if (m_dwContextMenus & GITSLC_POPRESTORE && !filepath->IsDirectory())
				{
					if (m_restorepaths.find(filepath->GetWinPathString()) == m_restorepaths.end())
						popup.AppendMenuIcon(IDGITLC_CREATERESTORE, IDS_MENUCREATERESTORE, IDI_RESTORE);
					else
						popup.AppendMenuIcon(IDGITLC_RESTOREPATH, IDS_MENURESTORE, IDI_RESTORE);
				}

				if ((m_dwContextMenus & GetContextMenuBit(IDGITLC_REVERTTOREV)) && !m_CurrentVersion.IsEmpty() && !(wcStatus & CTGitPath::LOGACTIONS_DELETED))
				{
					popup.AppendMenuIcon(IDGITLC_REVERTTOREV, IDS_LOG_POPUP_REVERTTOREV, IDI_REVERT);
				}

				if ((m_dwContextMenus & GetContextMenuBit(IDGITLC_REVERTTOPARENT)) && !m_CurrentVersion.IsEmpty() && !(wcStatus & CTGitPath::LOGACTIONS_ADDED))
				{
					CString str;
					str.LoadString(IDS_LOG_POPUP_REVERTTOPARENT);
					CString revStr;
					revStr.Format(L"%s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), (filepath->m_ParentNo & PARENT_MASK) + 1);
					GitRev rev;
					CGitHash parentHash;
					if (g_Git.GetHash(parentHash, revStr) == 0 && rev.GetCommit(parentHash.ToString()) == 0)
					{
						CString commitTitle = rev.GetSubject();
						if (commitTitle.GetLength() > 20)
						{
							commitTitle.Truncate(20);
							commitTitle += L"...";
						}
						str.AppendFormat(L": \"%s\" (%s)", static_cast<LPCTSTR>(commitTitle), static_cast<LPCTSTR>(parentHash.ToString(g_Git.GetShortHASHLength())));
					}
					else
						str.AppendFormat(L" (%s)", static_cast<LPCTSTR>(parentHash.ToString(g_Git.GetShortHASHLength())));
					popup.AppendMenuIcon(IDGITLC_REVERTTOPARENT, str, IDI_REVERT);
				}
			}

			if (selectedCount == 1 && !(wcStatus & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE)))
			{
				if (m_dwContextMenus & GITSLC_POPSHOWLOG)
					popup.AppendMenuIcon(IDGITLC_LOG, IDS_REPOBROWSE_SHOWLOG, IDI_LOG);
				if (m_dwContextMenus & GITSLC_POPSHOWLOGSUBMODULE && filepath->IsDirectory())
					popup.AppendMenuIcon(IDGITLC_LOGSUBMODULE, IDS_LOG_SUBMODULE, IDI_LOG);
				if (m_dwContextMenus & GITSLC_POPSHOWLOGOLDNAME && (wcStatus & (CTGitPath::LOGACTIONS_REPLACED|CTGitPath::LOGACTIONS_COPY) && !filepath->GetGitOldPathString().IsEmpty()))
					popup.AppendMenuIcon(IDGITLC_LOGOLDNAME, IDS_STATUSLIST_SHOWLOGOLDNAME, IDI_LOG);
				if ((m_dwContextMenus & GITSLC_POPBLAME) && !filepath->IsDirectory() && !(wcStatus & CTGitPath::LOGACTIONS_DELETED) && !((wcStatus & CTGitPath::LOGACTIONS_ADDED) && m_CurrentVersion.IsEmpty()) && m_bHasWC)
					popup.AppendMenuIcon(IDGITLC_BLAME, IDS_MENUBLAME, IDI_BLAME);
			}

			if (selectedCount > 0)
			{
				if ((m_dwContextMenus & GetContextMenuBit(IDGITLC_EXPORT)) && !(wcStatus & (CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING)))
					popup.AppendMenuIcon(IDGITLC_EXPORT, IDS_LOG_POPUP_EXPORT, IDI_EXPORT);
			}

			if (selectedCount == 1)
			{
				if (m_dwContextMenus & this->GetContextMenuBit(IDGITLC_SAVEAS) && ! filepath->IsDirectory() && !(wcStatus & CTGitPath::LOGACTIONS_DELETED))
					popup.AppendMenuIcon(IDGITLC_SAVEAS, IDS_LOG_POPUP_SAVE, IDI_SAVEAS);

				if (m_dwContextMenus & GITSLC_POPOPEN && !filepath->IsDirectory() && !(wcStatus & (CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING)))
				{
					popup.AppendMenuIcon(IDGITLC_VIEWREV, IDS_LOG_POPUP_VIEWREV, IDI_NOTEPAD);
					popup.AppendMenuIcon(IDGITLC_OPEN, IDS_REPOBROWSE_OPEN, IDI_OPEN);
					popup.AppendMenuIcon(IDGITLC_OPENWITH, IDS_LOG_POPUP_OPENWITH, IDI_OPEN);
				}

				if (m_dwContextMenus & GITSLC_POPEXPLORE && !(wcStatus & CTGitPath::LOGACTIONS_DELETED) && m_bHasWC)
					popup.AppendMenuIcon(IDGITLC_EXPLORE, IDS_STATUSLIST_CONTEXT_EXPLORE, IDI_EXPLORER);

				if (m_dwContextMenus & GITSLC_POPPREPAREDIFF && !(wcStatus & CTGitPath::LOGACTIONS_DELETED))
				{
					popup.AppendMenu(MF_SEPARATOR);
					popup.AppendMenuIcon(IDGITLC_PREPAREDIFF, IDS_PREPAREDIFF, IDI_DIFF);
					if (!m_sMarkForDiffFilename.IsEmpty())
					{
						CString diffWith;
						if (filepath->GetGitPathString() == m_sMarkForDiffFilename)
							diffWith = m_sMarkForDiffVersion;
						else
						{
							PathCompactPathEx(CStrBuf(diffWith, 2 * GIT_HASH_SIZE), m_sMarkForDiffFilename, 2 * GIT_HASH_SIZE, 0);
							diffWith += L':' + m_sMarkForDiffVersion.Left(g_Git.GetShortHASHLength());
						}
						CString menuEntry;
						menuEntry.Format(IDS_MENUDIFFNOW, static_cast<LPCTSTR>(diffWith));
						popup.AppendMenuIcon(IDGITLC_PREPAREDIFF_COMPARE, menuEntry, IDI_DIFF);
					}
				}
			}
			if (selectedCount > 0)
			{
//				if (((wcStatus == git_wc_status_unversioned)||(wcStatus == git_wc_status_ignored))&&(m_dwContextMenus & SVNSLC_POPDELETE))
//					popup.AppendMenuIcon(IDSVNLC_DELETE, IDS_MENUREMOVE, IDI_DELETE);
//				if ((wcStatus != Git_wc_status_unversioned)&&(wcStatus != git_wc_status_ignored)&&(wcStatus != Git_wc_status_deleted)&&(wcStatus != Git_wc_status_added)&&(m_dwContextMenus & GitSLC_POPDELETE))
//				{
//					if (bShift)
//						popup.AppendMenuIcon(IDGitLC_REMOVE, IDS_MENUREMOVEKEEP, IDI_DELETE);
//					else
//						popup.AppendMenuIcon(IDGitLC_REMOVE, IDS_MENUREMOVE, IDI_DELETE);
//				}
				if ((wcStatus & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE | CTGitPath::LOGACTIONS_MISSING))/*||(wcStatus == git_wc_status_deleted)*/)
				{
					if (m_dwContextMenus & GITSLC_POPDELETE)
						popup.AppendMenuIcon(IDGITLC_DELETE, IDS_MENUREMOVE, IDI_DELETE);
				}
				if ( (wcStatus & CTGitPath::LOGACTIONS_UNVER || wcStatus & CTGitPath::LOGACTIONS_DELETED) )
				{
					if (m_dwContextMenus & GITSLC_POPIGNORE)
					{
						CTGitPathList ignorelist;
						FillListOfSelectedItemPaths(ignorelist);
						//check if all selected entries have the same extension
						bool bSameExt = true;
						CString sExt;
						for (int i=0; i<ignorelist.GetCount(); ++i)
						{
							if (sExt.IsEmpty() && (i==0))
								sExt = ignorelist[i].GetFileExtension();
							else if (sExt.CompareNoCase(ignorelist[i].GetFileExtension())!=0)
								bSameExt = false;
						}
						if (bSameExt)
						{
							if (ignoreSubMenu.CreateMenu())
							{
								CString ignorepath;
								if (ignorelist.GetCount()==1)
									ignorepath = ignorelist[0].GetFileOrDirectoryName();
								else
									ignorepath.Format(IDS_MENUIGNOREMULTIPLE, ignorelist.GetCount());
								ignoreSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDGITLC_IGNORE, ignorepath);
								if (!sExt.IsEmpty())
								{
									ignorepath = L'*' + sExt;
									ignoreSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDGITLC_IGNOREMASK, ignorepath);
								}
								if (ignorelist.GetCount() == 1 && !ignorelist[0].GetContainingDirectory().GetGitPathString().IsEmpty())
									ignoreSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDGITLC_IGNOREFOLDER, ignorelist[0].GetContainingDirectory().GetGitPathString());
								CString temp;
								temp.LoadString(IDS_MENUIGNORE);
								popup.InsertMenu(static_cast<UINT>(-1), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(ignoreSubMenu.m_hMenu), temp);
							}
						}
						else
						{
							CString temp;
							if (ignorelist.GetCount()==1)
								temp.LoadString(IDS_MENUIGNORE);
							else
								temp.Format(IDS_MENUIGNOREMULTIPLE, ignorelist.GetCount());
							popup.AppendMenuIcon(IDGITLC_IGNORE, temp, IDI_IGNORE);
							if (!sExt.IsEmpty())
							{
								temp.Format(IDS_MENUIGNOREMULTIPLEMASK, ignorelist.GetCount());
								popup.AppendMenuIcon(IDGITLC_IGNOREMASK, temp, IDI_IGNORE);
							}
						}
					}
				}
			}



			if (selectedCount > 0)
			{
				popup.AppendMenu(MF_SEPARATOR);

				if (clipSubMenu.CreatePopupMenu())
				{
					CString temp;
					clipSubMenu.AppendMenuIcon(IDGITLC_COPYFULL, IDS_STATUSLIST_CONTEXT_COPYFULLPATHS, IDI_COPYCLIP);
					clipSubMenu.AppendMenuIcon(IDGITLC_COPYRELPATHS, IDS_STATUSLIST_CONTEXT_COPYRELPATHS, IDI_COPYCLIP);
					clipSubMenu.AppendMenuIcon(IDGITLC_COPYFILENAMES, IDS_STATUSLIST_CONTEXT_COPYFILENAMES, IDI_COPYCLIP);
					clipSubMenu.AppendMenuIcon(IDGITLC_COPYEXT, IDS_STATUSLIST_CONTEXT_COPYEXT, IDI_COPYCLIP);
					if (selSubitem >= 0)
					{
						temp.Format(IDS_STATUSLIST_CONTEXT_COPYCOL, static_cast<LPCWSTR>(m_ColumnManager.GetName(selSubitem)));
						clipSubMenu.AppendMenuIcon(IDGITLC_COPYCOL, temp, IDI_COPYCLIP);
					}
					temp.LoadString(IDS_LOG_POPUP_COPYTOCLIPBOARD);
					popup.InsertMenu(static_cast<UINT>(-1), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(clipSubMenu.m_hMenu), temp);
				}

				if ((m_dwContextMenus & GITSLC_POPCHANGELISTS)
					&&(wcStatus != git_wc_status_unversioned)&&(wcStatus != git_wc_status_none))
				{
					popup.AppendMenu(MF_SEPARATOR);
					// changelist commands
					if (HasChangelistInSelection())
						popup.AppendMenuIcon(IDGITLC_REMOVEFROMCS, IDS_STATUSLIST_CONTEXT_REMOVEFROMCS);
					if (changelistSubMenu.CreateMenu())
					{
						CString temp;
						temp.LoadString(IDS_STATUSLIST_CONTEXT_CREATECS);
						changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDGITLC_CREATECS, temp);

						{
							changelistSubMenu.AppendMenu(MF_SEPARATOR);
							changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, IDGITLC_CREATEIGNORECS, GITSLC_IGNORECHANGELIST);
						}

						if (!m_changelists.empty())
						{
							// find the changelist names
							bool bNeedSeparator = true;
							int cmdID = IDGITLC_MOVETOCS;
							for (auto it = m_changelists.cbegin(); it != m_changelists.cend(); ++it, ++cmdID)
							{
								if (it->first.Compare(GITSLC_IGNORECHANGELIST))
								{
									if (bNeedSeparator)
									{
										changelistSubMenu.AppendMenu(MF_SEPARATOR);
										bNeedSeparator = false;
									}
									changelistSubMenu.AppendMenu(MF_STRING | MF_ENABLED, cmdID, it->first);
								}
							}
						}
						temp.LoadString(IDS_STATUSLIST_CONTEXT_MOVETOCS);
						popup.AppendMenu(MF_POPUP|MF_STRING, reinterpret_cast<UINT_PTR>(changelistSubMenu.GetSafeHmenu()), temp);
					}
				}
			}

			m_hShellMenu = nullptr;
			if (selectedCount > 0 && !(wcStatus & (CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MISSING)) && m_bHasWC && m_CurrentVersion.IsEmpty() && shellMenu.CreatePopupMenu())
			{
				// insert the shell context menu
				popup.AppendMenu(MF_SEPARATOR);
				popup.InsertMenu(static_cast<UINT>(-1), MF_BYPOSITION | MF_POPUP, reinterpret_cast<UINT_PTR>(shellMenu.m_hMenu), CString(MAKEINTRESOURCE(IDS_STATUSLIST_CONTEXT_SHELL)));
				m_hShellMenu = shellMenu.m_hMenu;
			}

			int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, this);
			g_IContext2 = nullptr;
			g_IContext3 = nullptr;
			if (m_pContextMenu)
			{
				if (cmd >= SHELL_MIN_CMD && cmd <= SHELL_MAX_CMD) // see if returned idCommand belongs to shell menu entries)
				{
					CMINVOKECOMMANDINFOEX cmi = { 0 };
					cmi.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
					cmi.fMask = CMIC_MASK_UNICODE | CMIC_MASK_PTINVOKE;
					if (GetKeyState(VK_CONTROL) < 0)
						cmi.fMask |= CMIC_MASK_CONTROL_DOWN;
					if (bShift)
						cmi.fMask |= CMIC_MASK_SHIFT_DOWN;
					cmi.hwnd = m_hWnd;
					cmi.lpVerb = MAKEINTRESOURCEA(cmd - SHELL_MIN_CMD);
					cmi.lpVerbW = MAKEINTRESOURCEW(cmd - SHELL_MIN_CMD);
					cmi.nShow = SW_SHOWNORMAL;
					cmi.ptInvoke = point;

					m_pContextMenu->InvokeCommand(reinterpret_cast<LPCMINVOKECOMMANDINFO>(&cmi));

					cmd = 0;
				}
				m_pContextMenu->Release();
				m_pContextMenu = nullptr;
			}
			if (g_pFolderhook)
			{
				delete g_pFolderhook;
				g_pFolderhook = nullptr;
			}
			if (g_psfDesktopFolder)
			{
				g_psfDesktopFolder->Release();
				g_psfDesktopFolder = nullptr;
			}
			for (int i = 0; i < g_pidlArrayItems; i++)
			{
				if (g_pidlArray[i])
					CoTaskMemFree(g_pidlArray[i]);
			}
			if (g_pidlArray)
				CoTaskMemFree(g_pidlArray);
			g_pidlArray = nullptr;
			g_pidlArrayItems = 0;

			m_bWaitCursor = true;
			bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
			//int iItemCountBeforeMenuCmd = GetItemCount();
			//bool bForce = false;
			switch (cmd)
			{
			case IDGITLC_VIEWREV:
				OpenFile(filepath, ALTERNATIVEEDITOR);
				break;

			case IDGITLC_OPEN:
				OpenFile(filepath,OPEN);
				break;

			case IDGITLC_OPENWITH:
				OpenFile(filepath,OPEN_WITH);
				break;

			case IDGITLC_EXPLORE:
				CAppUtils::ExploreTo(GetSafeHwnd(), g_Git.CombinePath(filepath));
				break;

			case IDGITLC_PREPAREDIFF:
				m_sMarkForDiffFilename = filepath->GetGitPathString();
				m_sMarkForDiffVersion = m_CurrentVersion.ToString();
				break;

			case IDGITLC_PREPAREDIFF_COMPARE:
				{
					CTGitPath savedFile(m_sMarkForDiffFilename);
					CGitDiff::Diff(GetParentHWND(), filepath, &savedFile, m_CurrentVersion.ToString(), m_sMarkForDiffVersion, false, false, 0, bShift);
				}
				break;

			case IDGITLC_CREATERESTORE:
				{
					POSITION pos = GetFirstSelectedItemPosition();
					while (pos)
					{
						int index = GetNextSelectedItem(pos);
						auto entry2 = GetListEntry(index);
						if (!entry2 || entry2->IsDirectory())
							continue;
						if (m_restorepaths.find(entry2->GetWinPathString()) != m_restorepaths.end())
							continue;
						CTGitPath tempFile = CTempFiles::Instance().GetTempFilePath(false);
						// delete the temp file: the temp file has the FILE_ATTRIBUTE_TEMPORARY flag set
						// and copying the real file over it would leave that temp flag.
						DeleteFile(tempFile.GetWinPath());
						if (CopyFile(g_Git.CombinePath(entry2), tempFile.GetWinPath(), FALSE))
						{
							m_restorepaths[entry2->GetWinPathString()] = tempFile.GetWinPathString();
							SetItemState(index, INDEXTOOVERLAYMASK(OVL_RESTORE), LVIS_OVERLAYMASK);
						}
					}
					Invalidate();
				}
				break;

			case IDGITLC_RESTOREPATH:
				{
					if (CMessageBox::Show(GetParentHWND(), IDS_STATUSLIST_RESTOREPATH, IDS_APPNAME, 2, IDI_QUESTION, IDS_RESTOREBUTTON, IDS_ABORTBUTTON) == 2)
						break;
					POSITION pos = GetFirstSelectedItemPosition();
					while (pos)
					{
						int index = GetNextSelectedItem(pos);
						auto entry2 = GetListEntry(index);
						if (!entry2)
							continue;
						if (m_restorepaths.find(entry2->GetWinPathString()) == m_restorepaths.end())
							continue;
						if (CopyFile(m_restorepaths[entry2->GetWinPathString()], g_Git.CombinePath(entry2), FALSE))
						{
							CPathUtils::Touch(entry2->GetWinPathString());
							m_restorepaths.erase(entry2->GetWinPathString());
							SetItemState(index, 0, LVIS_OVERLAYMASK);
						}
					}
					Invalidate();
				}
				break;

			// Compare current version and work copy.
			case IDGITLC_COMPAREWC:
				{
					if (!CheckMultipleDiffs())
						break;
					POSITION pos = GetFirstSelectedItemPosition();
					while ( pos )
					{
						int index = GetNextSelectedItem(pos);
						StartDiffWC(index);
					}
				}
				break;

			// Compare with base version. when current version is zero, compare workcopy and HEAD.
			case IDGITLC_COMPARE:
				{
					if (!CheckMultipleDiffs())
						break;
					POSITION pos = GetFirstSelectedItemPosition();
					while ( pos )
					{
						int index = GetNextSelectedItem(pos);
						StartDiff(index);
					}
				}
				break;

			case IDGITLC_COMPARETWOREVISIONS:
				{
					if (!CheckMultipleDiffs())
						break;
					POSITION pos = GetFirstSelectedItemPosition();
					while ( pos )
					{
						int index = GetNextSelectedItem(pos);
						StartDiffTwo(index);
					}
				}
				break;

			case IDGITLC_COMPARETWOFILES:
				{
					POSITION pos = GetFirstSelectedItemPosition();
					if (pos)
					{
						auto firstfilepath = GetListEntry(GetNextSelectedItem(pos));
						if (!firstfilepath)
							break;

						auto secondfilepath = GetListEntry(GetNextSelectedItem(pos));
						if (!secondfilepath)
							break;

						CString sCmd;
						if (m_CurrentVersion.IsEmpty())
							sCmd.Format(L"/command:diff /path:\"%s\" /endrev:%s /path2:\"%s\" /startrev:%s /hwnd:%p", firstfilepath->GetWinPath(), firstfilepath->Exists() ? GIT_REV_ZERO : L"HEAD", secondfilepath->GetWinPath(), secondfilepath->Exists() ? GIT_REV_ZERO : L"HEAD", reinterpret_cast<void*>(m_hWnd));
						else
							sCmd.Format(L"/command:diff /path:\"%s\" /startrev:%s /path2:\"%s\" /endrev:%s /hwnd:%p", firstfilepath->GetWinPath(), firstfilepath->m_Action & CTGitPath::LOGACTIONS_DELETED ? static_cast<LPCTSTR>(m_CurrentVersion.ToString() + L"~1") : static_cast<LPCTSTR>(m_CurrentVersion.ToString()), secondfilepath->GetWinPath(), secondfilepath->m_Action & CTGitPath::LOGACTIONS_DELETED ? static_cast<LPCTSTR>(m_CurrentVersion.ToString() + L"~1") : static_cast<LPCTSTR>(m_CurrentVersion.ToString()), reinterpret_cast<void*>(m_hWnd));
						if (bShift)
							sCmd += L" /alternative";
						CAppUtils::RunTortoiseGitProc(sCmd);
					}
				}
				break;

			case IDGITLC_GNUDIFF1:
				{
					if (!CheckMultipleDiffs())
						break;
					POSITION pos = GetFirstSelectedItemPosition();
					while (pos)
					{
						auto selectedFilepath = GetListEntry(GetNextSelectedItem(pos));
						if (m_CurrentVersion.IsEmpty())
						{
							CString fromwhere;
							if (m_amend)
								fromwhere = L"~1";
							CAppUtils::StartShowUnifiedDiff(m_hWnd, *selectedFilepath, GitRev::GetHead() + fromwhere, *selectedFilepath, GitRev::GetWorkingCopy(), bShift);
						}
						else
						{
							if ((selectedFilepath->m_ParentNo & (PARENT_MASK | MERGE_MASK)) == 0)
								CAppUtils::StartShowUnifiedDiff(m_hWnd, *selectedFilepath, m_CurrentVersion.ToString() + L"~1", *selectedFilepath, m_CurrentVersion.ToString(), bShift);
							else
							{
								CString str;
								if (!(selectedFilepath->m_ParentNo & MERGE_MASK))
									str.Format(L"%s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), (selectedFilepath->m_ParentNo & PARENT_MASK) + 1);

								CAppUtils::StartShowUnifiedDiff(m_hWnd, *selectedFilepath, str, *selectedFilepath, m_CurrentVersion.ToString(), bShift, false, false, false, !!(selectedFilepath->m_ParentNo & MERGE_MASK));
							}
						}
					}
				}
				break;

			case IDGITLC_GNUDIFF2REVISIONS:
				{
					if (!CheckMultipleDiffs())
						break;
					POSITION pos = GetFirstSelectedItemPosition();
					while (pos)
					{
						auto entry = GetListEntry(GetNextSelectedItem(pos));
						CAppUtils::StartShowUnifiedDiff(m_hWnd, *entry, m_Rev2.ToString(), *entry, m_Rev1.ToString(), bShift);
					}
				}
				break;

			case IDGITLC_ADD:
			case IDGITLC_ADD_EXE:
			case IDGITLC_ADD_LINK:
				{
					CTGitPathList paths;
					FillListOfSelectedItemPaths(paths, true);

					CGitProgressDlg progDlg;
					AddProgressCommand addCommand;
					progDlg.SetCommand(&addCommand);
					addCommand.SetShowCommitButtonAfterAdd((m_dwContextMenus & GITSLC_POPCOMMIT) != 0);
					addCommand.SetPathList(paths);
					if (cmd == IDGITLC_ADD_EXE)
						addCommand.SetExecutable();
					else if (cmd == IDGITLC_ADD_LINK)
						addCommand.SetSymlink();
					progDlg.SetItemCount(paths.GetCount());
					progDlg.DoModal();

					// reset unchecked status
					POSITION pos = GetFirstSelectedItemPosition();
					int index;
					while ((index = GetNextSelectedItem(pos)) >= 0)
						m_mapFilenameToChecked.erase(GetListEntry(index)->GetGitPathString());

					if (GetLogicalParent() && GetLogicalParent()->GetSafeHwnd())
						GetLogicalParent()->SendMessage(GITSLNM_NEEDSREFRESH);

					SetRedraw(TRUE);
				}
				break;

			case IDGITLC_DELETE:
				DeleteSelectedFiles();
				break;

			case IDGITLC_BLAME:
				{
					CAppUtils::LaunchTortoiseBlame(g_Git.CombinePath(filepath), m_CurrentVersion.ToString());
				}
				break;

			case IDGITLC_LOG:
			case IDGITLC_LOGSUBMODULE:
				{
					CString sCmd;
					sCmd.Format(L"/command:log /path:\"%s\"", static_cast<LPCTSTR>(g_Git.CombinePath(filepath)));
					if (cmd == IDGITLC_LOG && filepath->IsDirectory())
						sCmd += L" /submodule";
					if (!m_sDisplayedBranch.IsEmpty())
						sCmd += L" /range:\"" + m_sDisplayedBranch + L'"';
					CAppUtils::RunTortoiseGitProc(sCmd, false, !(cmd == IDGITLC_LOGSUBMODULE));
				}
				break;

			case IDGITLC_LOGOLDNAME:
				{
					CTGitPath oldName(filepath->GetGitOldPathString());
					CString sCmd;
					sCmd.Format(L"/command:log /path:\"%s\"", static_cast<LPCTSTR>(g_Git.CombinePath(oldName)));
					if (!m_sDisplayedBranch.IsEmpty())
						sCmd += L" /range:\"" + m_sDisplayedBranch + L'"';
					CAppUtils::RunTortoiseGitProc(sCmd);
				}
				break;

			case IDGITLC_EDITCONFLICT:
				{
					if (CAppUtils::ConflictEdit(GetParentHWND(), *filepath, bShift, m_bIsRevertTheirMy, GetLogicalParent() ? GetLogicalParent()->GetSafeHwnd() : nullptr))
					{
						CString conflictedFile = g_Git.CombinePath(filepath);
						if (!PathFileExists(conflictedFile) && GetLogicalParent() && GetLogicalParent()->GetSafeHwnd())
						{
							GetLogicalParent()->SendMessage(GITSLNM_NEEDSREFRESH);
							break;
						}
						StoreScrollPos();
						Show(m_dwShow, 0, m_bShowFolders, 0, true);
					}
				}
				break;

			case IDGITLC_RESOLVETHEIRS: //follow up
			case IDGITLC_RESOLVEMINE:   //follow up
			case IDGITLC_RESOLVECONFLICT:
				{
					if (CMessageBox::Show(GetParentHWND(), IDS_PROC_RESOLVE, IDS_APPNAME, MB_ICONQUESTION | MB_YESNO) == IDYES)
					{
						CAppUtils::resolve_with resolveWith = CAppUtils::RESOLVE_WITH_CURRENT;
						if (((!this->m_bIsRevertTheirMy) && cmd == IDGITLC_RESOLVETHEIRS) || ((this->m_bIsRevertTheirMy) && cmd == IDGITLC_RESOLVEMINE))
							resolveWith = CAppUtils::RESOLVE_WITH_THEIRS;
						else if (((!this->m_bIsRevertTheirMy) && cmd == IDGITLC_RESOLVEMINE) || ((this->m_bIsRevertTheirMy) && cmd == IDGITLC_RESOLVETHEIRS))
							resolveWith = CAppUtils::RESOLVE_WITH_MINE;

						CSysProgressDlg sysProgressDlg;
						auto nListItems = GetSelectedCount();
						if (nListItems >= 5)
						{
							CString tmp, mineTitle, theirsTitle;
							CAppUtils::GetConflictTitles(nullptr, mineTitle, theirsTitle, m_bIsRevertTheirMy);
							tmp.LoadString(IDS_PROGRS_TITLE_RESOLVE);
							if (m_bIsRevertTheirMy)
							{
								if (cmd == IDGITLC_RESOLVEMINE)
									tmp.Format(IDS_SVNPROGRESS_MENURESOLVEUSING, static_cast<LPCTSTR>(theirsTitle));
								else if (cmd == IDGITLC_RESOLVETHEIRS)
									tmp.Format(IDS_SVNPROGRESS_MENURESOLVEUSING, static_cast<LPCTSTR>(mineTitle));
							}
							else
							{
								if (cmd == IDGITLC_RESOLVETHEIRS)
									tmp.Format(IDS_SVNPROGRESS_MENURESOLVEUSING, static_cast<LPCTSTR>(theirsTitle));
								else if (cmd == IDGITLC_RESOLVEMINE)
									tmp.Format(IDS_SVNPROGRESS_MENURESOLVEUSING, static_cast<LPCTSTR>(mineTitle));
							}

							sysProgressDlg.SetTitle(L"TortoiseGit");
							sysProgressDlg.SetLine(1, tmp);
							sysProgressDlg.SetTime(true);
							sysProgressDlg.SetShowProgressBar(true);
							sysProgressDlg.ShowModal(this, true);
						}
						auto currentTicks = GetTickCount64();

						bool needsFullRefresh = false;
						POSITION pos = GetFirstSelectedItemPosition();
						decltype(nListItems) j = 0;
						while (pos != 0)
						{
							int index = GetNextSelectedItem(pos);
							auto fentry = GetListEntry(index);
							if (!fentry)
								continue;

							if (sysProgressDlg.IsVisible())
							{
								if (GetTickCount64() - currentTicks > 1000UL || j == nListItems - 1 || j == 0)
								{
									sysProgressDlg.SetLine(2, fentry->GetGitPathString(), true);
									sysProgressDlg.SetProgress(j, nListItems);
									AfxGetThread()->PumpMessage(); // process messages, in order to avoid freezing; do not call this too often: this takes time!
									currentTicks = GetTickCount64();
								}
								++j;
							}

							if (CAppUtils::ResolveConflict(GetParentHWND(), *fentry, resolveWith) == 0 && fentry->m_Action & CTGitPath::LOGACTIONS_UNMERGED)
								needsFullRefresh = true;

							if (sysProgressDlg.HasUserCancelled())
								break;
						}
						sysProgressDlg.Stop();
						if (needsFullRefresh && CRegDWORD(L"Software\\TortoiseGit\\RefreshFileListAfterResolvingConflict", TRUE) == TRUE)
						{
							CWnd* pParent = GetLogicalParent();
							if (pParent && pParent->GetSafeHwnd())
								pParent->SendMessage(GITSLNM_NEEDSREFRESH);
							SetRedraw(TRUE);
							break;
						}
						StoreScrollPos();
						Show(m_dwShow, 0, m_bShowFolders,0,true);
					}
				}
				break;

			case IDGITLC_IGNORE:
				{
					CTGitPathList ignorelist;
					//std::vector<CString> toremove;
					FillListOfSelectedItemPaths(ignorelist, true);

					if (!CAppUtils::IgnoreFile(GetParentHWND(), ignorelist, false))
						break;

					SetRedraw(FALSE);
					CWnd* pParent = GetLogicalParent();
					if (pParent && pParent->GetSafeHwnd())
					{
						pParent->SendMessage(GITSLNM_NEEDSREFRESH);
					}
					SetRedraw(TRUE);
				}
				break;

			case IDGITLC_IGNOREMASK:
				{
					CString common;
					CTGitPathList ignorelist;
					FillListOfSelectedItemPaths(ignorelist, true);

					if (!CAppUtils::IgnoreFile(GetParentHWND(), ignorelist, true))
						break;

					SetRedraw(FALSE);
					CWnd* pParent = GetLogicalParent();
					if (pParent && pParent->GetSafeHwnd())
					{
						pParent->SendMessage(GITSLNM_NEEDSREFRESH);
					}

					SetRedraw(TRUE);
				}
				break;

			case IDGITLC_IGNOREFOLDER:
				{
					CTGitPathList ignorelist;
					ignorelist.AddPath(filepath->GetContainingDirectory());

					if (!CAppUtils::IgnoreFile(GetParentHWND(), ignorelist, false))
						break;

					SetRedraw(FALSE);
					CWnd *pParent = GetLogicalParent();
					if (pParent && pParent->GetSafeHwnd())
						pParent->SendMessage(GITSLNM_NEEDSREFRESH);

					SetRedraw(TRUE);
				}
				break;
			case IDGITLC_COMMIT:
				{
					CTGitPathList targetList;
					FillListOfSelectedItemPaths(targetList);
					CTGitPath tempFile = CTempFiles::Instance().GetTempFilePath(false);
					VERIFY(targetList.WriteToFile(tempFile.GetWinPathString()));
					CString commandline = L"/command:commit /pathfile:\"";
					commandline += tempFile.GetWinPathString();
					commandline += L'"';
					commandline += L" /deletepathfile";
					CAppUtils::RunTortoiseGitProc(commandline);
				}
				break;
			case IDGITLC_REVERT:
				{
					// If at least one item is not in the status "added"
					// we ask for a confirmation
					BOOL bConfirm = FALSE;
					POSITION pos = GetFirstSelectedItemPosition();
					int index;
					while ((index = GetNextSelectedItem(pos)) >= 0)
					{
						auto fentry = GetListEntry(index);
						if(fentry && fentry->m_Action &CTGitPath::LOGACTIONS_MODIFIED && !fentry->IsDirectory())
						{
							bConfirm = TRUE;
							break;
						}
					}

					CString str;
					str.Format(IDS_PROC_WARNREVERT, selectedCount);

					if (!bConfirm || MessageBox(str, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						CTGitPathList targetList;
						FillListOfSelectedItemPaths(targetList);

						// make sure that the list is reverse sorted, so that
						// children are removed before any parents
						targetList.SortByPathname(true);

						// put all reverted files in the trashbin, except the ones with 'added'
						// status because they are not restored by the revert.
						CTGitPathList delList;
						POSITION pos2 = GetFirstSelectedItemPosition();
						int index2;
						while ((index2 = GetNextSelectedItem(pos2)) >= 0)
						{
							auto entry = GetListEntry(index2);
							if (entry&&(!(entry->m_Action& CTGitPath::LOGACTIONS_ADDED))
									&& (!(entry->m_Action& CTGitPath::LOGACTIONS_REPLACED)) && !entry->IsDirectory())
							{
								CTGitPath fullpath;
								fullpath.SetFromWin(g_Git.CombinePath(entry));
								delList.AddPath(fullpath);
							}
						}
						if (DWORD(CRegDWORD(L"Software\\TortoiseGit\\RevertWithRecycleBin", TRUE)))
							delList.DeleteAllFiles(true);

						CString revertToCommit = L"HEAD";
						if (m_amend)
							revertToCommit = L"HEAD~1";
						if (CString err; g_Git.Revert(revertToCommit, targetList, err))
							MessageBox(L"Revert failed:\n" + err, L"TortoiseGit", MB_ICONERROR);
						else
						{
							bool updateStatusList = false;
							for (int i = 0 ; i < targetList.GetCount(); ++i)
							{
								int nListboxEntries = GetItemCount();
								for (int nItem=0; nItem<nListboxEntries; ++nItem)
								{
									auto path = GetListEntry(nItem);
									if (path->GetGitPathString()==targetList[i].GetGitPathString() && !path->IsDirectory())
									{
										if(path->m_Action & CTGitPath::LOGACTIONS_ADDED)
										{
											path->m_Action = CTGitPath::LOGACTIONS_UNVER;
											SetEntryCheck(path,nItem,false);
											updateStatusList = true;
#if 0 // revert an added file and some entry will be cloned (part 1/2)
											SetItemGroup(nItem,1);
											this->m_StatusFileList.RemoveItem(*path);
											this->m_UnRevFileList.AddPath(*path);
											//this->m_IgnoreFileList.RemoveItem(*path);
#endif
										}
										else
										{
											if (GetCheck(nItem))
												m_nSelected--;
											RemoveListEntry(nItem);
										}
										break;
									}
									else if (path->GetGitPathString()==targetList[i].GetGitPathString() && path->IsDirectory() && path->IsWCRoot())
									{
										CString sCmd;
										sCmd.Format(L"/command:revert /path:\"%s\"", static_cast<LPCTSTR>(path->GetGitPathString()));
										CCommonAppUtils::RunTortoiseGitProc(sCmd);
									}
								}
							}
							SetRedraw(TRUE);
#if 0 // revert an added file and some entry will be cloned (part 2/2)
							Show(m_dwShow, 0, m_bShowFolders,updateStatusList,true);
							NotifyCheck();
#else
							if (updateStatusList && GetLogicalParent() && GetLogicalParent()->GetSafeHwnd())
								GetLogicalParent()->SendMessage(GITSLNM_NEEDSREFRESH);
#endif
						}
					}
				}
				break;

			case IDGITLC_ASSUMEVALID:
				SetGitIndexFlagsForSelectedFiles(IDS_PROC_MARK_ASSUMEVALID, BST_CHECKED, BST_INDETERMINATE);
				break;
			case IDGITLC_SKIPWORKTREE:
				SetGitIndexFlagsForSelectedFiles(IDS_PROC_MARK_SKIPWORKTREE, BST_INDETERMINATE, BST_CHECKED);
				break;
			case IDGITLC_UNSETIGNORELOCALCHANGES:
				SetGitIndexFlagsForSelectedFiles(IDS_PROC_UNSET_IGNORELOCALCHANGES, BST_UNCHECKED, BST_UNCHECKED);
				break;
			case IDGITLC_COPYFULL:
			case IDGITLC_COPYRELPATHS:
			case IDGITLC_COPYFILENAMES:
				CopySelectedEntriesToClipboard(GITSLC_COLFILENAME, cmd);
				break;
			case IDGITLC_COPYEXT:
				CopySelectedEntriesToClipboard(static_cast<DWORD>(-1), 0);
				break;
			case IDGITLC_COPYCOL:
				CopySelectedEntriesToClipboard(DWORD(1) << selSubitem, 0);
				break;
			case IDGITLC_EXPORT:
				FilesExport();
				break;
			case IDGITLC_SAVEAS:
				FileSaveAs(filepath);
				break;

			case IDGITLC_REVERTTOREV:
				RevertSelectedItemToVersion();
				break;
			case IDGITLC_REVERTTOPARENT:
				RevertSelectedItemToVersion(true);
				break;
#if 0
			case IDSVNLC_COMMIT:
				{
					CTSVNPathList targetList;
					FillListOfSelectedItemPaths(targetList);
					CTSVNPath tempFile = CTempFiles::Instance().GetTempFilePath(false);
					VERIFY(targetList.WriteToFile(tempFile.GetWinPathString()));
					CString commandline = CPathUtils::GetAppDirectory();
					commandline += L"TortoiseGitProc.exe /command:commit /pathfile:\"";
					commandline += tempFile.GetWinPathString();
					commandline += L'"';
					commandline += L" /deletepathfile";
					CAppUtils::LaunchApplication(commandline, nullptr, false);
				}
				break;
#endif
			case IDGITLC_CREATEIGNORECS:
				MoveToChangelist(GITSLC_IGNORECHANGELIST);
				SaveChangelists();
				break;
			case IDGITLC_REMOVEFROMCS:
				RemoveFromChangelist();
				SaveChangelists();
				break;
			case IDGITLC_CREATECS:
			{
				CCreateChangelistDlg dlg;
				if (dlg.DoModal() == IDOK)
				{
					MoveToChangelist(dlg.m_sName);
					SaveChangelists();
				}
			}
				break;
			default:
				{
					if (cmd < IDGITLC_MOVETOCS)
						break;

					// find the changelist name
					CString sChangelist;
					int cmdID = IDGITLC_MOVETOCS;

					SetRedraw(FALSE);
					for (auto it = m_changelists.cbegin(); it != m_changelists.cend(); ++it, ++cmdID)
					{
						if (cmd == cmdID)
						{
							sChangelist = it->first;
							break;
						}
					}

					if (!sChangelist.IsEmpty())
					{
						MoveToChangelist(sChangelist);
						SaveChangelists();
					}
					SetRedraw(TRUE);
				}
				break;

			} // switch (cmd)
			m_bWaitCursor = false;
			GetStatisticsString();
			//int iItemCountAfterMenuCmd = GetItemCount();
			//if (iItemCountAfterMenuCmd != iItemCountBeforeMenuCmd)
			//{
			//	CWnd* pParent = GetParent();
			//	if (pParent && pParent->GetSafeHwnd())
			//	{
			//		pParent->SendMessage(SVNSLNM_ITEMCOUNTCHANGED);
			//	}
			//}
		} // if (popup.CreatePopupMenu())
	} // if (selIndex >= 0)
}

void CGitStatusListCtrl::MoveToChangelist(const CString& name)
{
	CAutoReadLock locker(m_guard);

	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int index = GetNextSelectedItem(pos);
		auto pGitPath = GetListEntry(index);
		if (name.Compare(GITSLC_IGNORECHANGELIST) == 0)
			SetEntryCheck(pGitPath, index, false);
		m_pathToChangelist.insert_or_assign(pGitPath->GetGitPathString(), name);
	}

	PrepareGroups();

	for (int i = 0; i < GetItemCount(); ++i)
		SetItemGroup(i, GetListEntry(i));
}

void CGitStatusListCtrl::RemoveFromChangelist()
{
	CAutoReadLock locker(m_guard);

	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		auto pGitPath = GetListEntry(GetNextSelectedItem(pos));
		m_pathToChangelist.erase(pGitPath->GetGitPathString());
	}

	PrepareGroups();

	for (int i = 0; i < GetItemCount(); ++i)
		SetItemGroup(i, GetListEntry(i));
}

void CGitStatusListCtrl::SetGitIndexFlagsForSelectedFiles(UINT message, BOOL assumevalid, BOOL skipworktree)
{
	if (CMessageBox::Show(GetParentHWND(), message, IDS_APPNAME, MB_YESNO | MB_DEFBUTTON2 | MB_ICONQUESTION) != IDYES)
		return;

	CAutoReadLock locker(m_guard);

	CAutoRepository repository(g_Git.GetGitRepository());
	if (!repository)
	{
		MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	CAutoIndex gitindex;
	if (git_repository_index(gitindex.GetPointer(), repository))
	{
		MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	POSITION pos = GetFirstSelectedItemPosition();
	int index = -1;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		auto path = GetListEntry(index);
		if (path == nullptr)
			continue;

		size_t idx;
		if (!git_index_find(&idx, gitindex, CUnicodeUtils::GetMulti(path->GetGitPathString(), CP_UTF8)))
		{
			git_index_entry *e = const_cast<git_index_entry *>(git_index_get_byindex(gitindex, idx)); // HACK
			if (assumevalid == BST_UNCHECKED)
				e->flags &= ~GIT_INDEX_ENTRY_VALID;
			else if (assumevalid == BST_CHECKED)
				e->flags |= GIT_INDEX_ENTRY_VALID;
			if (skipworktree == BST_UNCHECKED)
				e->flags_extended &= ~GIT_INDEX_ENTRY_SKIP_WORKTREE;
			else if (skipworktree == BST_CHECKED)
				e->flags_extended |= GIT_INDEX_ENTRY_SKIP_WORKTREE;
			git_index_add(gitindex, e);
		}
		else
			MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONERROR);
	}

	if (git_index_write(gitindex))
	{
		MessageBox(g_Git.GetLibGit2LastErr(), L"TortoiseGit", MB_ICONERROR);
		return;
	}

	if (nullptr != GetLogicalParent() && nullptr != GetLogicalParent()->GetSafeHwnd())
		GetLogicalParent()->SendMessage(GITSLNM_NEEDSREFRESH);

	SetRedraw(TRUE);
}

void CGitStatusListCtrl::OnContextMenu(CWnd* pWnd, CPoint point)
{
	__super::OnContextMenu(pWnd, point);
	if (pWnd == this)
		OnContextMenuList(pWnd, point);
}

void CGitStatusListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	CAutoReadWeakLock readLock(m_guard);
	if (!readLock.IsAcquired())
		return;

	if (pNMLV->iItem < 0)
		return;

	auto file = GetListEntry(pNMLV->iItem);

	if (file->m_Action & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE)) {
		StartDiffWC(pNMLV->iItem);
		return;
	}
	if( file->m_Action&CTGitPath::LOGACTIONS_UNMERGED )
	{
		if (CAppUtils::ConflictEdit(GetParentHWND(), *file, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), m_bIsRevertTheirMy, GetLogicalParent() ? GetLogicalParent()->GetSafeHwnd() : nullptr))
		{
			CString conflictedFile = g_Git.CombinePath(file);
			if (!PathFileExists(conflictedFile) && GetLogicalParent() && GetLogicalParent()->GetSafeHwnd())
			{
				GetLogicalParent()->SendMessage(GITSLNM_NEEDSREFRESH);
				return;
			}
			StoreScrollPos();
			Show(m_dwShow, 0, m_bShowFolders, 0, true);
		}
	}
	else if ((file->m_Action & CTGitPath::LOGACTIONS_MISSING) && file->m_Action != (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED) && file->m_Action != (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MODIFIED))
		return;
	else
	{
		if (!m_Rev1.IsEmpty() && !m_Rev2.IsEmpty())
			StartDiffTwo(pNMLV->iItem);
		else
			StartDiff(pNMLV->iItem);
	}
}
void CGitStatusListCtrl::StartDiffTwo(int fileindex)
{
	if(fileindex<0)
		return;

	CAutoReadLock locker(m_guard);
	auto ptr = GetListEntry(fileindex);
	if (!ptr)
		return;
	CTGitPath file1 = *ptr;

	if (file1.m_Action & CTGitPath::LOGACTIONS_ADDED)
		CGitDiff::DiffNull(GetParentHWND(), &file1, m_Rev1.ToString(), true, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	else if (file1.m_Action & CTGitPath::LOGACTIONS_DELETED)
		CGitDiff::DiffNull(GetParentHWND(), &file1, m_Rev2.ToString(), false, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	else
		CGitDiff::Diff(GetParentHWND(), &file1, &file1, m_Rev1.ToString(), m_Rev2.ToString(), false, false, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
}

void CGitStatusListCtrl::StartDiffWC(int fileindex)
{
	if(fileindex<0)
		return;

	CAutoReadLock locker(m_guard);
	auto ptr = GetListEntry(fileindex);
	if (!ptr)
		return;
	CTGitPath file1 = *ptr;
	file1.m_Action = 0; // reset action, so that diff is not started as added/deleted file; see issue #1757

	CGitDiff::Diff(GetParentHWND(), &file1, &file1, GIT_REV_ZERO, m_CurrentVersion.ToString(), false, false, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
}

void CGitStatusListCtrl::StartDiff(int fileindex)
{
	if(fileindex<0)
		return;

	CAutoReadLock locker(m_guard);
	auto ptr = GetListEntry(fileindex);
	if (!ptr)
		return;
	CTGitPath file1 = *ptr;
	CTGitPath file2;
	if(file1.m_Action & (CTGitPath::LOGACTIONS_REPLACED|CTGitPath::LOGACTIONS_COPY))
		file2.SetFromGit(file1.GetGitOldPathString());
	else
		file2=file1;

	if (m_CurrentVersion.IsEmpty())
	{
		CString fromwhere;
		if(m_amend && (file1.m_Action & CTGitPath::LOGACTIONS_ADDED) == 0)
			fromwhere = L"~1";
		if( g_Git.IsInitRepos())
			CGitDiff::DiffNull(GetParentHWND(), GetListEntry(fileindex),
			GIT_REV_ZERO, true, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		else if( file1.m_Action&CTGitPath::LOGACTIONS_ADDED )
			CGitDiff::DiffNull(GetParentHWND(), GetListEntry(fileindex),
			m_CurrentVersion.ToString() + fromwhere, true, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		else if( file1.m_Action&CTGitPath::LOGACTIONS_DELETED )
			CGitDiff::DiffNull(GetParentHWND(), GetListEntry(fileindex),
			GitRev::GetHead() + fromwhere, false, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		else
			CGitDiff::Diff(GetParentHWND(), &file1,&file2,
					CString(GIT_REV_ZERO),
					GitRev::GetHead() + fromwhere, false, false, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
	}
	else
	{
		CGitHash hash;
		CString fromwhere = m_CurrentVersion.ToString() + L"~1";
		if(m_amend)
			fromwhere = m_CurrentVersion.ToString() + L"~2";
		bool revfail = !!g_Git.GetHash(hash, fromwhere);
		if (revfail || (file1.m_Action & file1.LOGACTIONS_ADDED))
			CGitDiff::DiffNull(GetParentHWND(), &file1, m_CurrentVersion.ToString(), true, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		else if (file1.m_Action & file1.LOGACTIONS_DELETED)
		{
			if (file1.m_ParentNo > 0)
				fromwhere.Format(L"%s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), file1.m_ParentNo + 1);

			CGitDiff::DiffNull(GetParentHWND(), &file1, fromwhere, false, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
		}
		else
		{
			if( file1.m_ParentNo & MERGE_MASK)
			{
				CTGitPath base, theirs, mine, merge;

				CString temppath;
				GetTempPath(temppath);
				temppath.TrimRight(L'\\');

				mine.SetFromGit(temppath + L'\\' + file1.GetFileOrDirectoryName() + L".LOCAL" + file1.GetFileExtension());
				theirs.SetFromGit(temppath + L'\\' + file1.GetFileOrDirectoryName() + L".REMOTE" + file1.GetFileExtension());
				base.SetFromGit(temppath + L'\\' + file1.GetFileOrDirectoryName() + L".BASE" + file1.GetFileExtension());

				CFile tempfile;
				//create a empty file, incase stage is not three
				tempfile.Open(mine.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
				tempfile.Close();
				tempfile.Open(theirs.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
				tempfile.Close();
				tempfile.Open(base.GetWinPathString(),CFile::modeCreate|CFile::modeReadWrite);
				tempfile.Close();

				merge.SetFromGit(temppath + L'\\' + file1.GetFileOrDirectoryName() + L".Merged" + file1.GetFileExtension());

				int parent1=-1, parent2 =-1;
				for (size_t i = 0; i < m_arStatusArray.size(); ++i)
				{
					if(m_arStatusArray[i]->GetGitPathString() == file1.GetGitPathString())
					{
						if(m_arStatusArray[i]->m_ParentNo & MERGE_MASK)
						{
						}
						else
						{
							if(parent1<0)
								parent1 = m_arStatusArray[i]->m_ParentNo & PARENT_MASK;
							else if (parent2 <0)
								parent2 = m_arStatusArray[i]->m_ParentNo & PARENT_MASK;
						}
					}
				}

				if (g_Git.GetOneFile(m_CurrentVersion.ToString(), file1, merge.GetWinPathString()))
					CMessageBox::Show(GetParentHWND(), IDS_STATUSLIST_FAILEDGETMERGEFILE, IDS_APPNAME, MB_OK | MB_ICONERROR);

				if(parent1>=0)
				{
					CString str;
					str.Format(L"%s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), parent1 + 1);

					if (g_Git.GetOneFile(str, file1, mine.GetWinPathString()))
						CMessageBox::Show(GetParentHWND(), IDS_STATUSLIST_FAILEDGETMERGEFILE, IDS_APPNAME, MB_OK | MB_ICONERROR);
				}

				if(parent2>=0)
				{
					CString str;
					str.Format(L"%s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), parent2 + 1);

					if (g_Git.GetOneFile(str, file1, theirs.GetWinPathString()))
						CMessageBox::Show(GetParentHWND(), IDS_STATUSLIST_FAILEDGETMERGEFILE, IDS_APPNAME, MB_OK | MB_ICONERROR);
				}

				if(parent1>=0 && parent2>=0)
				{
					CString cmd, output;
					cmd.Format(L"git.exe merge-base %s^%d %s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), parent1 + 1,
						static_cast<LPCTSTR>(m_CurrentVersion.ToString()), parent2 + 1);

					if (!g_Git.Run(cmd, &output, nullptr, CP_UTF8))
					{
						if (g_Git.GetOneFile(output.Left(2 * GIT_HASH_SIZE), file1, base.GetWinPathString()))
							CMessageBox::Show(GetParentHWND(), IDS_STATUSLIST_FAILEDGETBASEFILE, IDS_APPNAME, MB_OK | MB_ICONERROR);
					}
				}
				CAppUtils::StartExtMerge(!!(GetAsyncKeyState(VK_SHIFT) & 0x8000), base, theirs, mine, merge, L"BASE", L"REMOTE", L"LOCAL");
			}
			else
			{
				CString str;
				if( (file1.m_ParentNo&PARENT_MASK) == 0)
					str = L"~1";
				else
					str.Format(L"^%d", (file1.m_ParentNo & PARENT_MASK) + 1);
				CGitDiff::Diff(GetParentHWND(), &file1,&file2,
					m_CurrentVersion.ToString(),
					m_CurrentVersion.ToString() + str, false, false, 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			}
		}
	}
}

CString CGitStatusListCtrl::GetStatisticsString(bool simple)
{
	CString sNormal = CString(MAKEINTRESOURCE(IDS_STATUSNORMAL));
	CString sAdded = CString(MAKEINTRESOURCE(IDS_STATUSADDED));
	CString sDeleted = CString(MAKEINTRESOURCE(IDS_STATUSDELETED));
	CString sModified = CString(MAKEINTRESOURCE(IDS_STATUSMODIFIED));
	CString sConflicted = CString(MAKEINTRESOURCE(IDS_STATUSCONFLICTED));
	CString sUnversioned = CString(MAKEINTRESOURCE(IDS_STATUSUNVERSIONED));
	CString sRenamed = CString(MAKEINTRESOURCE(IDS_STATUSREPLACED));
	CString sToolTip;
	if(simple)
	{
		sToolTip.Format(IDS_STATUSLIST_STATUSLINE1,
			this->m_nLineAdded,this->m_nLineDeleted,
				static_cast<LPCTSTR>(sModified), m_nModified,
				static_cast<LPCTSTR>(sAdded), m_nAdded,
				static_cast<LPCTSTR>(sDeleted), m_nDeleted,
				static_cast<LPCTSTR>(sRenamed), m_nRenamed
			);
	}
	else
	{
		sToolTip.Format(IDS_STATUSLIST_STATUSLINE2,
			this->m_nLineAdded,this->m_nLineDeleted,
				static_cast<LPCTSTR>(sNormal), m_nNormal,
				static_cast<LPCTSTR>(sUnversioned), m_nUnversioned,
				static_cast<LPCTSTR>(sModified), m_nModified,
				static_cast<LPCTSTR>(sAdded), m_nAdded,
				static_cast<LPCTSTR>(sDeleted), m_nDeleted,
				static_cast<LPCTSTR>(sConflicted), m_nConflicted
			);
	}
	CString sStats;
	sStats.FormatMessage(IDS_COMMITDLG_STATISTICSFORMAT, m_nSelected, GetItemCount());
	if (m_pStatLabel)
		m_pStatLabel->SetWindowText(sStats);

	if (m_pSelectButton)
	{
		if (m_nSelected == 0)
			m_pSelectButton->SetCheck(BST_UNCHECKED);
		else if (m_nSelected != GetItemCount())
			m_pSelectButton->SetCheck(BST_INDETERMINATE);
		else
			m_pSelectButton->SetCheck(BST_CHECKED);
	}

	if (m_pConfirmButton)
		m_pConfirmButton->EnableWindow(m_nSelected>0);

	return sToolTip;
}

CString CGitStatusListCtrl::GetCommonDirectory(bool bStrict)
{
	CAutoReadLock locker(m_guard);
	if (!bStrict)
	{
		// not strict means that the selected folder has priority
		if (!m_StatusFileList.GetCommonDirectory().IsEmpty())
			return m_StatusFileList.GetCommonDirectory().GetWinPath();
	}

	CTGitPathList list;
	int nListItems = GetItemCount();
	for (int i=0; i<nListItems; ++i)
	{
		auto* entry = GetListEntry(i);
		if (entry->IsEmpty())
			continue;
		list.AddPath(*entry);
	}
	return list.GetCommonRoot().GetWinPath();
}

void CGitStatusListCtrl::SelectAll(bool bSelect, bool /*bIncludeNoCommits*/)
{
	CWaitCursor waitCursor;
	// block here so the LVN_ITEMCHANGED messages
	// get ignored
	ScopedInDecrement blocker(m_nBlockItemChangeHandler);

	{
		CAutoWriteLock locker(m_guard);
		SetRedraw(FALSE);

		int nListItems = GetItemCount();
		if (bSelect)
			m_nSelected = nListItems;
		else
			m_nSelected = 0;

		for (int i=0; i<nListItems; ++i)
		{
			auto path = GetListEntry(i);
			if (!path)
				continue;
			//if ((bIncludeNoCommits)||(entry->GetChangeList().Compare(SVNSLC_IGNORECHANGELIST)))
			SetEntryCheck(path,i,bSelect);
		}
	}
	SetRedraw(TRUE);
	GetStatisticsString();
	NotifyCheck();
}

void CGitStatusListCtrl::Check(DWORD dwCheck, bool check)
{
	CWaitCursor waitCursor;
	// block here so the LVN_ITEMCHANGED messages
	// get ignored
	{
		CAutoWriteLock locker(m_guard);
		SetRedraw(FALSE);
		ScopedInDecrement blocker(m_nBlockItemChangeHandler);

		int nListItems = GetItemCount();

		for (int i = 0; i < nListItems; ++i)
		{
			auto entry = GetListEntry(i);
			if (!entry)
				continue;

			DWORD showFlags = entry->m_Action;
			if (entry->IsDirectory())
				showFlags |= GITSLC_SHOWSUBMODULES;
			else
				showFlags |= GITSLC_SHOWFILES;

			if (check && (showFlags & dwCheck) && !GetCheck(i) && !(entry->IsDirectory() && m_bDoNotAutoselectSubmodules && !(dwCheck & GITSLC_SHOWSUBMODULES)))
			{
				SetEntryCheck(entry, i, true);
				m_nSelected++;
			}
			else if (!check && (showFlags & dwCheck) && GetCheck(i))
			{
				SetEntryCheck(entry, i, false);
				m_nSelected--;
			}
		}
	}
	SetRedraw(TRUE);
	GetStatisticsString();
	NotifyCheck();
}

void CGitStatusListCtrl::OnLvnGetInfoTip(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVGETINFOTIP pGetInfoTip = reinterpret_cast<LPNMLVGETINFOTIP>(pNMHDR);
	*pResult = 0;
	if (CRegDWORD(L"Software\\TortoiseGit\\ShowListFullPathTooltip", TRUE) != TRUE)
		return;

	CAutoReadWeakLock readLock(m_guard);
	if (!readLock.IsAcquired())
		return;

	auto entry = GetListEntry(pGetInfoTip->iItem);

	if (entry)
		if (pGetInfoTip->cchTextMax > entry->GetGitPathString().GetLength() + g_Git.m_CurrentDir.GetLength())
		{
			CString str = g_Git.CombinePath(entry->GetWinPathString());
			wcsncpy_s(pGetInfoTip->pszText, pGetInfoTip->cchTextMax, str.GetBuffer(), pGetInfoTip->cchTextMax - 1);
		}
}

BOOL CGitStatusListCtrl::OnNMCustomdraw(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>( pNMHDR );

	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;

	// First thing - check the draw stage. If it's the control's prepaint
	// stage, then tell Windows we want messages for every item.

	switch (pLVCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		*pResult = CDRF_NOTIFYITEMDRAW;
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color.

			// Tell Windows we want a callback (be)for(e) painting the subitems.
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			CAutoReadWeakLock readLock(m_guard, 0);
			if (!readLock.IsAcquired())
				return TRUE;

			if (m_arStatusArray.size() > static_cast<DWORD_PTR>(pLVCD->nmcd.dwItemSpec))
			{
				auto entry = GetListEntry(static_cast<int>(pLVCD->nmcd.dwItemSpec));
				if (!entry)
					return TRUE;

				// coloring
				// ========
				// black  : unversioned, normal
				// purple : added
				// blue   : modified
				// brown  : missing, deleted, replaced
				// green  : merged (or potential merges)
				// red    : conflicts or sure conflicts
				COLORREF crText;
				if(entry->m_Action & CTGitPath::LOGACTIONS_GRAY)
					crText = RGB(128,128,128);
				else if(entry->m_Action & CTGitPath::LOGACTIONS_UNMERGED)
					crText = m_Colors.GetColor(CColors::Conflict);
				else if(entry->m_Action & (CTGitPath::LOGACTIONS_MODIFIED))
					crText = m_Colors.GetColor(CColors::Modified);
				else if(entry->m_Action & (CTGitPath::LOGACTIONS_ADDED|CTGitPath::LOGACTIONS_COPY))
					crText = m_Colors.GetColor(CColors::Added);
				else if(entry->m_Action & CTGitPath::LOGACTIONS_DELETED)
					crText = m_Colors.GetColor(CColors::Deleted);
				else if(entry->m_Action & CTGitPath::LOGACTIONS_REPLACED)
					crText = m_Colors.GetColor(CColors::Renamed);
				else if(entry->m_Action & CTGitPath::LOGACTIONS_MERGED)
					crText = m_Colors.GetColor(CColors::Merged);
				else
					crText = GetSysColor(COLOR_WINDOWTEXT);
				// Store the color back in the NMLVCUSTOMDRAW struct.
				pLVCD->clrText = crText;
			}
		}
		break;

		case CDDS_ITEMPREPAINT | CDDS_ITEM | CDDS_SUBITEM:
			return FALSE; // allow parent to handle this
			break;
	}
	return TRUE;
}

void CGitStatusListCtrl::OnLvnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult)
{
	auto pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	*pResult = 0;

	// Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	CAutoReadWeakLock readLock(m_guard, 0);
	if (readLock.IsAcquired())
	{
		if (pItem->mask & LVIF_TEXT)
		{
			CString text = GetCellText(pItem->iItem, pItem->iSubItem);
			lstrcpyn(pItem->pszText, text, pItem->cchTextMax - 1);
		}
	}
	else
		pItem->mask = 0;
}

BOOL CGitStatusListCtrl::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd != this)
		return CListCtrl::OnSetCursor(pWnd, nHitTest, message);
	if (!m_bWaitCursor && !m_bBusy)
	{
		HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
		SetCursor(hCur);
		return CListCtrl::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(nullptr, IDC_WAIT);
	SetCursor(hCur);
	return TRUE;
}

void CGitStatusListCtrl::RemoveListEntry(int index)
{
	CAutoWriteLock locker(m_guard);
	DeleteItem(index);

	m_arStatusArray.erase(m_arStatusArray.cbegin() + index);

#if 0
	delete m_arStatusArray[m_arListArray[index]];
	m_arStatusArray.erase(m_arStatusArray.begin()+m_arListArray[index]);
	m_arListArray.erase(m_arListArray.begin()+index);
	for (int i = index; i < static_cast<int>(m_arListArray.size()); ++i)
	{
		m_arListArray[i]--;
	}
#endif
}

///< Set a checkbox on an entry in the listbox
// NEVER, EVER call SetCheck directly, because you'll end-up with the checkboxes and the 'checked' flag getting out of sync
void CGitStatusListCtrl::SetEntryCheck(CTGitPath* pEntry, int listboxIndex, bool bCheck)
{
	CAutoWriteLock locker(m_guard);
	pEntry->m_Checked = bCheck;
	m_mapFilenameToChecked[pEntry->GetGitPathString()] = bCheck;
	SetCheck(listboxIndex, bCheck);
}

void CGitStatusListCtrl::ResetChecked(const CTGitPath& entry)
{
	CAutoWriteLock locker(m_guard);
	CTGitPath adjustedEntry;
	if (g_Git.m_CurrentDir[g_Git.m_CurrentDir.GetLength() - 1] == L'\\')
		adjustedEntry.SetFromWin(entry.GetWinPathString().Right(entry.GetWinPathString().GetLength() - g_Git.m_CurrentDir.GetLength()));
	else
		adjustedEntry.SetFromWin(entry.GetWinPathString().Right(entry.GetWinPathString().GetLength() - g_Git.m_CurrentDir.GetLength() - 1));
	if (entry.IsDirectory())
	{
		STRING_VECTOR toDelete;
		for (auto it = m_mapFilenameToChecked.begin(); it != m_mapFilenameToChecked.end(); ++it)
		{
			if (adjustedEntry.IsAncestorOf(it->first))
				toDelete.emplace_back(it->first);
		}
		for (const auto& file : toDelete)
			m_mapFilenameToChecked.erase(file);
		return;
	}
	m_mapFilenameToChecked.erase(adjustedEntry.GetGitPathString());
}

#if 0
void CGitStatusListCtrl::SetCheckOnAllDescendentsOf(const FileEntry* parentEntry, bool bCheck)
{
	CAutoWriteLock locker(m_guard);
	int nListItems = GetItemCount();
	for (int j=0; j< nListItems ; ++j)
	{
		FileEntry * childEntry = GetListEntry(j);
		ASSERT(childEntry);
		if (!childEntry || childEntry == parentEntry)
			continue;
		if (childEntry->checked != bCheck)
		{
			if (parentEntry->path.IsAncestorOf(childEntry->path))
			{
				SetEntryCheck(childEntry,j,bCheck);
				if(bCheck)
					m_nSelected++;
				else
					m_nSelected--;
			}
		}
	}
}
#endif

void CGitStatusListCtrl::WriteCheckedNamesToPathList(CTGitPathList& pathList)
{
	pathList.Clear();
	CAutoReadLock locker(m_guard);
	int nListItems = GetItemCount();
	for (int i = 0; i< nListItems; ++i)
	{
		auto entry = GetListEntry(i);
		if (entry->m_Checked)
			pathList.AddPath(*entry);
	}
	pathList.SortByPathname();
}


/// Build a path list of all the selected items in the list (NOTE - SELECTED, not CHECKED)
void CGitStatusListCtrl::FillListOfSelectedItemPaths(CTGitPathList& pathList, bool /*bNoIgnored*/)
{
	pathList.Clear();

	CAutoReadLock locker(m_guard);
	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		auto entry = GetListEntry(index);
		//if ((bNoIgnored)&&(entry->status == git_wc_status_ignored))
		//	continue;
		pathList.AddPath(*entry);
	}
}

UINT CGitStatusListCtrl::OnGetDlgCode()
{
	// we want to process the return key and not have that one
	// routed to the default pushbutton
	return CListCtrl::OnGetDlgCode() | DLGC_WANTALLKEYS;
}

void CGitStatusListCtrl::OnNMReturn(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;
	CAutoReadWeakLock readLock(m_guard);
	if (!readLock.IsAcquired())
		return;
	if (!CheckMultipleDiffs())
		return;
	bool needsRefresh = false;
	bool resolvedTreeConfict = false;
	POSITION pos = GetFirstSelectedItemPosition();
	while ( pos )
	{
		int index = GetNextSelectedItem(pos);
		if (index < 0)
			return;
		auto file = GetListEntry(index);
		if (file == nullptr)
			return;
		if (file->m_Action & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE))
			StartDiffWC(index);
		else if ((file->m_Action & CTGitPath::LOGACTIONS_UNMERGED))
		{
			if (CAppUtils::ConflictEdit(GetParentHWND(), *file, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000), m_bIsRevertTheirMy, GetLogicalParent() ? GetLogicalParent()->GetSafeHwnd() : nullptr))
			{
				CString conflictedFile = g_Git.CombinePath(file);
				needsRefresh = needsRefresh || !PathFileExists(conflictedFile);
				resolvedTreeConfict = resolvedTreeConfict || (file->m_Action & CTGitPath::LOGACTIONS_UNMERGED) == 0;
			}
		}
		else if ((file->m_Action & CTGitPath::LOGACTIONS_MISSING) && file->m_Action != (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED) && file->m_Action != (CTGitPath::LOGACTIONS_MISSING | CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_MODIFIED))
			continue;
		else
		{
			if (!m_Rev1.IsEmpty() && !m_Rev2.IsEmpty())
				StartDiffTwo(index);
			else
				StartDiff(index);
		}
	}
	if (needsRefresh && GetLogicalParent() && GetLogicalParent()->GetSafeHwnd())
		GetLogicalParent()->SendMessage(GITSLNM_NEEDSREFRESH);
	else if (resolvedTreeConfict)
	{
		StoreScrollPos();
		Show(m_dwShow, 0, m_bShowFolders, 0, true);
	}
}

void CGitStatusListCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Since we catch all keystrokes (to have the enter key processed here instead
	// of routed to the default pushbutton) we have to make sure that other
	// keys like Tab and Esc still do what they're supposed to do
	// Tab = change focus to next/previous control
	// Esc = quit the dialog
	switch (nChar)
	{
	case (VK_TAB):
		{
			::PostMessage(GetLogicalParent()->GetSafeHwnd(), WM_NEXTDLGCTL, GetKeyState(VK_SHIFT)&0x8000, 0);
			return;
		}
		break;
	case (VK_ESCAPE):
		{
			::SendMessage(GetLogicalParent()->GetSafeHwnd(), WM_CLOSE, 0, 0);
		}
		break;
	}

	CListCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CGitStatusListCtrl::PreSubclassWindow()
{
	__super::PreSubclassWindow();
	EnableToolTips(TRUE);
	SetWindowTheme(GetSafeHwnd(), L"Explorer", nullptr);
}

void CGitStatusListCtrl::OnPaint()
{
	LRESULT defres = Default();
	if ((m_bBusy) || (m_bEmpty))
	{
		CString str;
		if (m_bBusy)
		{
			if (m_sBusy.IsEmpty())
				str.LoadString(IDS_STATUSLIST_BUSYMSG);
			else
				str = m_sBusy;
		}
		else
		{
			if (m_sEmpty.IsEmpty())
				str.LoadString(IDS_STATUSLIST_EMPTYMSG);
			else
				str = m_sEmpty;
		}
		COLORREF clrText = ::GetSysColor(COLOR_WINDOWTEXT);
		COLORREF clrTextBk;
		if (IsWindowEnabled())
			clrTextBk = ::GetSysColor(COLOR_WINDOW);
		else
			clrTextBk = ::GetSysColor(COLOR_3DFACE);

		CRect rc;
		GetClientRect(&rc);
		CHeaderCtrl* pHC = GetHeaderCtrl();
		if (pHC)
		{
			CRect rcH;
			pHC->GetItemRect(0, &rcH);
			rc.top += rcH.bottom;
		}
		CDC* pDC = GetDC();
		{
			CMyMemDC memDC(pDC, &rc);

			memDC.SetTextColor(clrText);
			memDC.SetBkColor(clrTextBk);
			memDC.BitBlt(rc.left, rc.top, rc.Width(), rc.Height(), pDC, rc.left, rc.top, SRCCOPY);
			rc.top += 10;
			CGdiObject* oldfont = memDC.SelectObject(CGdiObject::FromHandle(m_uiFont));
			memDC.DrawText(str, rc, DT_CENTER | DT_VCENTER |
				DT_WORDBREAK | DT_NOPREFIX | DT_NOCLIP);
			memDC.SelectObject(oldfont);
		}
		ReleaseDC(pDC);
	}
	if (defres)
	{
		// the Default() call did not process the WM_PAINT message!
		// Validate the update region ourselves to avoid
		// an endless loop repainting
		CRect rc;
		GetUpdateRect(&rc, FALSE);
		if (!rc.IsRectEmpty())
			ValidateRect(rc);
	}
}

void CGitStatusListCtrl::OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

	CAutoReadLock locker(m_guard);

	CTGitPathList pathList;
	FillListOfSelectedItemPaths(pathList);
	if (pathList.IsEmpty())
		return;

	auto pdsrc = std::make_unique<CIDropSource>();
	if (!pdsrc)
		return;
	pdsrc->AddRef();

	GitDataObject* pdobj = new GitDataObject(pathList, m_Rev2.IsEmpty() ? m_CurrentVersion : m_Rev2);
	if (!pdobj)
		return;
	pdobj->AddRef();

	CDragSourceHelper dragsrchelper;

	SetRedraw(false);
	dragsrchelper.InitializeFromWindow(m_hWnd, pNMLV->ptAction, pdobj);
	SetRedraw(true);
	//dragsrchelper.InitializeFromBitmap()
	pdsrc->m_pIDataObj = pdobj;
	pdsrc->m_pIDataObj->AddRef();

	// Initiate the Drag & Drop
	DWORD dwEffect;
	m_bOwnDrag = true;
	::DoDragDrop(pdobj, pdsrc.get(), DROPEFFECT_MOVE | DROPEFFECT_COPY, &dwEffect);
	m_bOwnDrag = false;
	pdsrc->Release();
	pdsrc.release();
	pdobj->Release();

	*pResult = 0;
}

bool CGitStatusListCtrl::EnableFileDrop()
{
	m_bFileDropsEnabled = true;
	return true;
}

bool CGitStatusListCtrl::HasPath(const CTGitPath& path)
{
	CAutoReadLock locker(m_guard);
	CTGitPath adjustedEntry;
	if (g_Git.m_CurrentDir[g_Git.m_CurrentDir.GetLength() - 1] == L'\\')
		adjustedEntry.SetFromWin(path.GetWinPathString().Right(path.GetWinPathString().GetLength() - g_Git.m_CurrentDir.GetLength()));
	else
		adjustedEntry.SetFromWin(path.GetWinPathString().Right(path.GetWinPathString().GetLength() - g_Git.m_CurrentDir.GetLength() - 1));
	for (size_t i=0; i < m_arStatusArray.size(); ++i)
	{
		if (m_arStatusArray[i]->IsEquivalentTo(adjustedEntry))
			return true;
	}

	return false;
}

BOOL CGitStatusListCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 'A':
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					// select all entries
					for (int i=0; i<GetItemCount(); ++i)
						SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
					return TRUE;
				}
			}
			break;
		case 'C':
		case VK_INSERT:
			{
				if (GetAsyncKeyState(VK_CONTROL)&0x8000)
				{
					// copy all selected paths to the clipboard
					if (GetAsyncKeyState(VK_SHIFT)&0x8000)
						CopySelectedEntriesToClipboard(GITSLC_COLFILENAME | GITSLC_COLSTATUS, IDGITLC_COPYRELPATHS);
					else
						CopySelectedEntriesToClipboard(GITSLC_COLFILENAME, IDGITLC_COPYRELPATHS);
					return TRUE;
				}
			}
			break;
		case VK_DELETE:
			{
				if ((GetSelectedCount() > 0) && (m_dwContextMenus & GITSLC_POPDELETE))
				{
					CAutoReadLock locker(m_guard);
					auto filepath = GetListEntry(GetSelectionMark());
					if (filepath != nullptr && (filepath->m_Action & (CTGitPath::LOGACTIONS_UNVER | CTGitPath::LOGACTIONS_IGNORE)))
						DeleteSelectedFiles();
				}
			}
			break;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

bool CGitStatusListCtrl::CopySelectedEntriesToClipboard(DWORD dwCols, int cmd)
{
	if (GetSelectedCount() == 0)
		return false;

	CString sClipboard;

	bool bMultipleColumnSelected = ((dwCols & dwCols - 1) != 0); //  multiple columns are selected (clear least signifient bit and check for zero)

#define ADDTOCLIPBOARDSTRING(x) sClipboard += (sClipboard.IsEmpty() || (sClipboard.Right(1)==L"\n")) ? (x) : ('\t' + x)
#define ADDNEWLINETOCLIPBOARDSTRING() sClipboard += (sClipboard.IsEmpty()) ? L"" : L"\r\n"

	// first add the column titles as the first line
	DWORD selection = 0;
	int count = m_ColumnManager.GetColumnCount();
	for (int column = 0; column < count; ++column)
	{
		if ((dwCols == -1 && m_ColumnManager.IsVisible(column)) || (column < GITSLC_NUMCOLUMNS && (dwCols & (1 << column))))
		{
			if (bMultipleColumnSelected)
				ADDTOCLIPBOARDSTRING(m_ColumnManager.GetName(column));

			selection |= 1 << column;
		}
	}

	if (bMultipleColumnSelected)
		ADDNEWLINETOCLIPBOARDSTRING();

	// maybe clear first line when only one column is selected (btw by select not by dwCols) is simpler(not faster) way
	// but why no title on single column output ?
	// if (selection & selection-1) == 0 ) sClipboard = "";

	CAutoReadLock locker(m_guard);

	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		int index = GetNextSelectedItem(pos);
		// we selected only cols we want, so not other then select test needed
		for (int column = 0; column < count; ++column)
		{
			if (cmd && (GITSLC_COLFILENAME & (1 << column)))
			{
				auto* entry = GetListEntry(index);
				if (entry)
				{
					CString sPath;
					switch (cmd)
					{
					case IDGITLC_COPYFULL:
						sPath = g_Git.CombinePath(entry);
						break;
					case IDGITLC_COPYRELPATHS:
						sPath = entry->GetGitPathString();
						break;
					case IDGITLC_COPYFILENAMES:
						sPath = entry->GetFileOrDirectoryName();
						break;
					}
					ADDTOCLIPBOARDSTRING(sPath);
				}
			}
			else if (selection & (1 << column))
				ADDTOCLIPBOARDSTRING(GetCellText(index, column));
		}

		ADDNEWLINETOCLIPBOARDSTRING();
	}

	return CStringUtils::WriteAsciiStringToClipboard(sClipboard);
}

bool CGitStatusListCtrl::HasChangelistInSelection()
{
	CAutoReadLock locker(m_guard);
	std::set<CString> changelists;
	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		auto pGitPath = GetListEntry(index);
		auto it = m_pathToChangelist.find(pGitPath->GetGitPathString());
		if (it != m_pathToChangelist.cend())
			return true;
	}
	return false;
}

bool CGitStatusListCtrl::PrepareGroups(bool bForce /* = false */)
{
	CAutoWriteLock locker(m_guard);
	bool bHasGroups=false;
	int	max =0;

	for (size_t i = 0; i < m_arStatusArray.size(); ++i)
	{
		int ParentNo = m_arStatusArray[i]->m_ParentNo&PARENT_MASK;
		if( ParentNo > max)
			max=m_arStatusArray[i]->m_ParentNo&PARENT_MASK;
	}

	if (((m_dwShow & GITSLC_SHOWUNVERSIONED) && !m_UnRevFileList.IsEmpty()) ||
		((m_dwShow & GITSLC_SHOWIGNORED) && !m_IgnoreFileList.IsEmpty()) ||
		(m_dwShow & (GITSLC_SHOWASSUMEVALID | GITSLC_SHOWSKIPWORKTREE) && !m_LocalChangesIgnoredFileList.IsEmpty()) ||
		max > 0 || !m_pathToChangelist.empty() || bForce)
	{
		bHasGroups = true;
	}

	RemoveAllGroups();
	EnableGroupView(bHasGroups);

	TCHAR groupname[1024] = { 0 };
	int groupindex = 0;

	if(bHasGroups)
	{
		LVGROUP grp = {0};
		grp.cbSize = sizeof(LVGROUP);
		grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
		groupindex=0;

		//if(m_UnRevFileList.GetCount()>0)
		if(max >0)
		{
			wcsncpy_s(groupname, static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_STATUSLIST_GROUP_MERGEDFILES))), _TRUNCATE);
			grp.pszHeader = groupname;
			grp.iGroupId = MERGE_MASK;
			grp.uAlign = LVGA_HEADER_LEFT;
			InsertGroup(0, &grp);

			CAutoRepository repository(g_Git.GetGitRepository());
			if (!repository)
				MessageBox(CGit::GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_OK | MB_ICONERROR);
			for (groupindex = 0; groupindex <= max; ++groupindex)
			{
				CString str;
				str.Format(IDS_STATUSLIST_GROUP_DIFFWITHPARENT, groupindex + 1);
				if (repository)
				{
					CString rev;
					rev.Format(L"%s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), groupindex + 1);
					CGitHash hash;
					if (!CGit::GetHash(repository, hash, rev))
						str += L": " + hash.ToString(g_Git.GetShortHASHLength());
				}
				grp.pszHeader = str.GetBuffer();
				str.ReleaseBuffer();
				grp.iGroupId = groupindex;
				grp.uAlign = LVGA_HEADER_LEFT;
				InsertGroup(groupindex, &grp);
			}
		}
		else
		{
			wcsncpy_s(groupname, static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_STATUSLIST_GROUP_MODIFIEDFILES))), _TRUNCATE);
			grp.pszHeader = groupname;
			grp.iGroupId = groupindex;
			grp.uAlign = LVGA_HEADER_LEFT;
			InsertGroup(groupindex++, &grp);

			{
				wcsncpy_s(groupname, static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_STATUSLIST_GROUP_NOTVERSIONEDFILES))), _TRUNCATE);
				grp.pszHeader = groupname;
				grp.iGroupId = groupindex;
				grp.uAlign = LVGA_HEADER_LEFT;
				InsertGroup(groupindex++, &grp);
			}

			//if(m_IgnoreFileList.GetCount()>0)
			{
				wcsncpy_s(groupname, static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_STATUSLIST_GROUP_IGNOREDFILES))), _TRUNCATE);
				grp.pszHeader = groupname;
				grp.iGroupId = groupindex;
				grp.uAlign = LVGA_HEADER_LEFT;
				InsertGroup(groupindex++, &grp);
			}

			{
				wcsncpy_s(groupname, static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_STATUSLIST_GROUP_IGNORELOCALCHANGES))), _TRUNCATE);
				grp.pszHeader = groupname;
				grp.iGroupId = groupindex;
				grp.uAlign = LVGA_HEADER_LEFT;
				InsertGroup(groupindex++, &grp);
			}
		}
	}

	m_bHasIgnoreGroup = false;

	// now add the items which don't belong to a group
	LVGROUP grp = {0};
	grp.cbSize = sizeof(LVGROUP);
	grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
	CString sUnassignedName(MAKEINTRESOURCE(IDS_STATUSLIST_UNASSIGNED_CHANGESET));
	wcsncpy_s(groupname, static_cast<LPCTSTR>(sUnassignedName), _TRUNCATE);
	grp.pszHeader = groupname;
	grp.iGroupId = groupindex;
	grp.uAlign = LVGA_HEADER_LEFT;
	InsertGroup(groupindex++, &grp);

	// Refresh changelist to group index map
	m_changelists.clear();
	for (auto it = m_pathToChangelist.cbegin(); it != m_pathToChangelist.cend(); ++it)
		m_changelists.insert_or_assign(it->second, 0);

	for (auto it = m_changelists.begin(); it != m_changelists.end(); ++it)
	{
		if (it->first.Compare(GITSLC_IGNORECHANGELIST) != 0)
		{
			LVGROUP grpchglst = { 0 };
			grpchglst.cbSize = sizeof(LVGROUP);
			grpchglst.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
			wcsncpy_s(groupname, it->first, _TRUNCATE);
			grpchglst.pszHeader = groupname;
			grpchglst.iGroupId = groupindex;
			grpchglst.uAlign = LVGA_HEADER_LEFT;
			it->second = InsertGroup(groupindex++, &grpchglst);
		}
		else
			m_bHasIgnoreGroup = true;
	}

	if (m_bHasIgnoreGroup)
	{
		// and now add the group 'ignore-on-commit'
		std::map<CString,int>::iterator it = m_changelists.find(GITSLC_IGNORECHANGELIST);
		if (it != m_changelists.end())
		{
			grp.cbSize = sizeof(LVGROUP);
			grp.mask = LVGF_ALIGN | LVGF_GROUPID | LVGF_HEADER;
			wcsncpy_s(groupname, GITSLC_IGNORECHANGELIST, _TRUNCATE);
			grp.pszHeader = groupname;
			grp.iGroupId = groupindex;
			grp.uAlign = LVGA_HEADER_LEFT;
			it->second = InsertGroup(groupindex, &grp);
		}
	}
	return bHasGroups;
}

void CGitStatusListCtrl::NotifyCheck()
{
	CWnd* pParent = GetLogicalParent();
	if (pParent && pParent->GetSafeHwnd())
	{
		pParent->SendMessage(GITSLNM_CHECKCHANGED, m_nSelected);
	}
}

int CGitStatusListCtrl::UpdateFileList(const CTGitPathList* list)
{
	CAutoWriteLock locker(m_guard);
	m_CurrentVersion.Empty();

	ATLASSERT(!(m_amend && !m_bIncludedStaged)); // just a safeguard that we always show all files if we want to amend (amending should only be the used from commitdlg)
	g_Git.GetWorkingTreeChanges(m_StatusFileList, m_amend, list, m_bIncludedStaged);

	BOOL bDeleteChecked = FALSE;
	int deleteFromIndex = 0;
	bool needsRefresh = false;
	for (int i = 0; i < m_StatusFileList.GetCount(); ++i)
	{
		auto gitpatch = const_cast<CTGitPath*>(&m_StatusFileList[i]);
		gitpatch->m_Checked = TRUE;

		if ((gitpatch->m_Action & (CTGitPath::LOGACTIONS_ADDED | CTGitPath::LOGACTIONS_REPLACED | CTGitPath::LOGACTIONS_MODIFIED)) && !gitpatch->Exists())
		{
			if (!bDeleteChecked)
			{
				CString message;
				message.Format(IDS_ASK_REMOVE_FROM_INDEX, gitpatch->GetWinPath());
				deleteFromIndex = CMessageBox::ShowCheck(GetSafeHwnd(), message, L"TortoiseGit", 1, IDI_EXCLAMATION, CString(MAKEINTRESOURCE(IDS_RESTORE_FROM_INDEX)), CString(MAKEINTRESOURCE(IDS_REMOVE_FROM_INDEX)), CString(MAKEINTRESOURCE(IDS_IGNOREBUTTON)), nullptr, CString(MAKEINTRESOURCE(IDS_DO_SAME_FOR_REST)), &bDeleteChecked);
			}
			if (deleteFromIndex == 1)
			{
				if (CString err; g_Git.Run(L"git.exe checkout -- \"" + gitpatch->GetWinPathString() + L'"', &err, CP_UTF8))
					MessageBox(L"Restoring from index failed:\n" + err, L"TortoiseGit", MB_ICONERROR);
				else
					needsRefresh = true;
			}
			else if (deleteFromIndex == 2)
			{
				if (CString err; g_Git.Run(L"git.exe rm -f --cache -- \"" + gitpatch->GetWinPathString() + L'"', &err, CP_UTF8))
					MessageBox(L"Removing from index failed:\n" + err, L"TortoiseGit", MB_ICONERROR);
				else
					needsRefresh = true;
			}
		}

		m_arStatusArray.push_back(&m_StatusFileList[i]);
	}

	if (needsRefresh)
		MessageBox(L"Due to changes to the index, please refresh the dialog (e.g., by pressing F5).", L"TortoiseGit", MB_ICONINFORMATION);

	return 0;
}

int CGitStatusListCtrl::UpdateWithGitPathList(CTGitPathList &list)
{
	CAutoWriteLock locker(m_guard);
	m_arStatusArray.clear();
	for (int i = 0; i < list.GetCount(); ++i)
	{
		auto gitpath = const_cast<CTGitPath*>(&list[i]);

		if(gitpath ->m_Action & CTGitPath::LOGACTIONS_HIDE)
			continue;

		gitpath->m_Checked = TRUE;
		m_arStatusArray.push_back(&list[i]);
	}
	return 0;
}

int CGitStatusListCtrl::UpdateUnRevFileList(CTGitPathList &list)
{
	CAutoWriteLock locker(m_guard);
	m_UnRevFileList = list;
	for (int i = 0; i < m_UnRevFileList.GetCount(); ++i)
	{
		auto gitpatch = const_cast<CTGitPath*>(&m_UnRevFileList[i]);
		gitpatch->m_Checked = FALSE;
		m_arStatusArray.push_back(&m_UnRevFileList[i]);
	}
	return 0;
}

int CGitStatusListCtrl::UpdateUnRevFileList(const CTGitPathList* List)
{
	CAutoWriteLock locker(m_guard);
	if (CString err; m_UnRevFileList.FillUnRev(CTGitPath::LOGACTIONS_UNVER, List, &err))
	{
		MessageBox(L"Failed to get UnRev file list\n" + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return -1;
	}

	if (m_StatusFileList.m_Action & CTGitPath::LOGACTIONS_DELETED)
	{
		int unrev = 0;
		int status = 0;
		while (unrev < m_UnRevFileList.GetCount() && status < m_StatusFileList.GetCount())
		{
			auto cmp = CTGitPath::Compare(m_UnRevFileList[unrev], m_StatusFileList[status]);
			if (cmp < 1)
			{
				++unrev;
				continue;
			}
			if (cmp == 1)
				m_UnRevFileList.RemovePath(m_StatusFileList[status]);
			++status;
		}
	}

	for (int i = 0; i < m_UnRevFileList.GetCount(); ++i)
	{
		auto gitpatch = const_cast<CTGitPath*>(&m_UnRevFileList[i]);
		gitpatch->m_Checked = FALSE;
		m_arStatusArray.push_back(&m_UnRevFileList[i]);
	}
	return 0;
}

int CGitStatusListCtrl::UpdateIgnoreFileList(const CTGitPathList* List)
{
	CAutoWriteLock locker(m_guard);
	if (CString err; m_IgnoreFileList.FillUnRev(CTGitPath::LOGACTIONS_IGNORE, List, &err))
	{
		MessageBox(L"Failed to get Ignore file list\n" + err, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return -1;
	}

	for (int i = 0; i < m_IgnoreFileList.GetCount(); ++i)
	{
		auto gitpatch = const_cast<CTGitPath*>(&m_IgnoreFileList[i]);
		gitpatch->m_Checked = FALSE;
		m_arStatusArray.push_back(&m_IgnoreFileList[i]);
	}
	return 0;
}

int CGitStatusListCtrl::UpdateLocalChangesIgnoredFileList(const CTGitPathList* list)
{
	CAutoWriteLock locker(m_guard);
	m_LocalChangesIgnoredFileList.FillBasedOnIndexFlags(GIT_INDEX_ENTRY_VALID, GIT_INDEX_ENTRY_SKIP_WORKTREE, list);
	for (int i = 0; i < m_LocalChangesIgnoredFileList.GetCount(); ++i)
	{
		auto gitpatch = const_cast<CTGitPath*>(&m_LocalChangesIgnoredFileList[i]);
		gitpatch->m_Checked = FALSE;
		m_arStatusArray.push_back(&m_LocalChangesIgnoredFileList[i]);
	}
	return 0;
}

int CGitStatusListCtrl::UpdateFileList(int mask, bool once, const CTGitPathList* pList)
{
	CAutoWriteLock locker(m_guard);
	auto List = (pList && pList->GetCount() >= 1 && !(*pList)[0].GetWinPathString().IsEmpty()) ? pList : nullptr;
	if(mask&CGitStatusListCtrl::FILELIST_MODIFY)
	{
		if(once || (!(m_FileLoaded&CGitStatusListCtrl::FILELIST_MODIFY)))
		{
			UpdateFileList(List);
			m_FileLoaded|=CGitStatusListCtrl::FILELIST_MODIFY;
		}
	}
	if (mask & CGitStatusListCtrl::FILELIST_UNVER || mask & CGitStatusListCtrl::FILELIST_IGNORE)
	{
		if(once || (!(m_FileLoaded&CGitStatusListCtrl::FILELIST_UNVER)))
		{
			UpdateUnRevFileList(List);
			m_FileLoaded|=CGitStatusListCtrl::FILELIST_UNVER;
		}
		if(mask&CGitStatusListCtrl::FILELIST_IGNORE && (once || (!(m_FileLoaded&CGitStatusListCtrl::FILELIST_IGNORE))))
		{
			UpdateIgnoreFileList(List);
			m_FileLoaded |= CGitStatusListCtrl::FILELIST_IGNORE;
		}
	}
	if (mask & CGitStatusListCtrl::FILELIST_LOCALCHANGESIGNORED && (once || (!(m_FileLoaded & CGitStatusListCtrl::FILELIST_LOCALCHANGESIGNORED))))
	{
		UpdateLocalChangesIgnoredFileList(List);
		m_FileLoaded |= CGitStatusListCtrl::FILELIST_LOCALCHANGESIGNORED;
	}
	return 0;
}

void CGitStatusListCtrl::Clear()
{
	CAutoWriteLock locker(m_guard);
	m_FileLoaded=0;
	this->DeleteAllItems();
	this->m_arListArray.clear();
	this->m_arStatusArray.clear();
	this->m_changelists.clear();
	this->m_pathToChangelist.clear();
}

bool CGitStatusListCtrl::CheckMultipleDiffs()
{
	UINT selCount = GetSelectedCount();
	if (selCount > max(DWORD(3), static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\NumDiffWarning", 10))))
	{
		CString message;
		message.Format(IDS_STATUSLIST_WARN_MAXDIFF, selCount);
		return MessageBox(message, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) == IDYES;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
bool CGitStatusListCtrlDropTarget::OnDrop(FORMATETC* pFmtEtc, STGMEDIUM& medium, DWORD* /*pdwEffect*/, POINTL pt)
{
	if (pFmtEtc->cfFormat == CF_HDROP && medium.tymed == TYMED_HGLOBAL)
	{
		HDROP hDrop = static_cast<HDROP>(GlobalLock(medium.hGlobal));
		if (hDrop)
		{
			TCHAR szFileName[MAX_PATH] = {0};

			UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);

			POINT clientpoint;
			clientpoint.x = pt.x;
			clientpoint.y = pt.y;
			ScreenToClient(m_hTargetWnd, &clientpoint);
			if ((m_pGitStatusListCtrl->IsGroupViewEnabled()) && (m_pGitStatusListCtrl->GetGroupFromPoint(&clientpoint) >= 0))
			{
#if 0
				CTGitPathList changelistItems;
				for (UINT i = 0; i < cFiles; ++i)
				{
					if (DragQueryFile(hDrop, i, szFileName, _countof(szFileName)))
						changelistItems.AddPath(CTGitPath(szFileName));
				}
				// find the changelist name
				CString sChangelist;
				LONG_PTR nGroup = m_pGitStatusListCtrl->GetGroupFromPoint(&clientpoint);
				for (std::map<CString, int>::iterator it = m_pGitStatusListCtrl->m_changelists.begin(); it != m_pGitStatusListCtrl->m_changelists.end(); ++it)
					if (it->second == nGroup)
						sChangelist = it->first;

				if (!sChangelist.IsEmpty())
				{
					CGit git;
					if (git.AddToChangeList(changelistItems, sChangelist, git_depth_empty))
					{
						for (int l=0; l<changelistItems.GetCount(); ++l)
						{
							int index = m_pGitStatusListCtrl->GetIndex(changelistItems[l]);
							if (index >= 0)
							{
								auto e = m_pGitStatusListCtrl->GetListEntry(index);
								if (e)
								{
									e->changelist = sChangelist;
									if (!e->IsFolder())
									{
										if (m_pGitStatusListCtrl->m_changelists.find(e->changelist) != m_pGitStatusListCtrl->m_changelists.end())
											m_pGitStatusListCtrl->SetItemGroup(index, m_pGitStatusListCtrl->m_changelists[e->changelist]);
										else
											m_pGitStatusListCtrl->SetItemGroup(index, 0);
									}
								}
							}
							else
							{
								HWND hParentWnd = GetParent(m_hTargetWnd);
								if (hParentWnd)
									::SendMessage(hParentWnd, CGitStatusListCtrl::GITSLNM_ADDFILE, 0, reinterpret_cast<LPARAM>(changelistItems[l].GetWinPath()));
							}
						}
					}
					else
						CMessageBox::Show(m_pGitStatusListCtrl->m_hWnd, git.GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
				}
				else
				{
					SVN git;
					if (git.RemoveFromChangeList(changelistItems, CStringArray(), git_depth_empty))
					{
						for (int l=0; l<changelistItems.GetCount(); ++l)
						{
							int index = m_pGitStatusListCtrl->GetIndex(changelistItems[l]);
							if (index >= 0)
							{
								auto e = m_pGitStatusListCtrl->GetListEntry(index);
								if (e)
								{
									e->changelist = sChangelist;
									m_pGitStatusListCtrl->SetItemGroup(index, 0);
								}
							}
							else
							{
								HWND hParentWnd = GetParent(m_hTargetWnd);
								if (hParentWnd)
									::SendMessage(hParentWnd, CGitStatusListCtrl::GITSLNM_ADDFILE, 0, reinterpret_cast<LPARAM>(changelistItems[l].GetWinPath()));
							}
						}
					}
					else
						CMessageBox::Show(m_pGitStatusListCtrl->m_hWnd, git.GetLastErrorMessage(), L"TortoiseGit", MB_ICONERROR);
				}
#endif
			}
			else
			{
				for (UINT i = 0; i < cFiles; ++i)
				{
					if (!DragQueryFile(hDrop, i, szFileName, _countof(szFileName)))
						continue;

					HWND hParentWnd = GetParent(m_hTargetWnd);
					if (hParentWnd)
						::SendMessage(hParentWnd, CGitStatusListCtrl::GITSLNM_ADDFILE, 0, reinterpret_cast<LPARAM>(szFileName));
				}
			}
		}
		GlobalUnlock(medium.hGlobal);
	}
	return true; //let base free the medium
}

HRESULT STDMETHODCALLTYPE CGitStatusListCtrlDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD __RPC_FAR* pdwEffect)
{
	CIDropTarget::DragOver(grfKeyState, pt, pdwEffect);
	*pdwEffect = DROPEFFECT_COPY;
	if (m_pGitStatusListCtrl)
	{
		POINT clientpoint;
		clientpoint.x = pt.x;
		clientpoint.y = pt.y;
		ScreenToClient(m_hTargetWnd, &clientpoint);
		if ((m_pGitStatusListCtrl->IsGroupViewEnabled()) && (m_pGitStatusListCtrl->GetGroupFromPoint(&clientpoint) >= 0))
			*pdwEffect = DROPEFFECT_NONE;
		else if ((!m_pGitStatusListCtrl->m_bFileDropsEnabled) || (m_pGitStatusListCtrl->m_bOwnDrag))
			*pdwEffect = DROPEFFECT_NONE;
	}
	return S_OK;
}

void CGitStatusListCtrl::FilesExport()
{
	CAutoReadLock locker(m_guard);
	CString exportDir;
	// export all changed files to a folder
	CBrowseFolder browseFolder;
	browseFolder.m_style = BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
	if (browseFolder.Show(GetParentHWND(), exportDir) != CBrowseFolder::OK)
		return;

	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		auto fd = GetListEntry(index);
		// we cannot export directories or folders
		if ((fd->m_Action & CTGitPath::LOGACTIONS_DELETED) || fd->IsDirectory())
			continue;

		CPathUtils::MakeSureDirectoryPathExists(exportDir + L'\\' + fd->GetContainingDirectory().GetWinPathString());
		CString filename = exportDir + L'\\' + fd->GetWinPathString();
		if (m_CurrentVersion.IsEmpty())
		{
			if (!CopyFile(g_Git.CombinePath(fd), filename, false))
			{
				MessageBox(CFormatMessageWrapper(), L"TortoiseGit", MB_OK | MB_ICONERROR);
				return;
			}
		}
		else
		{
			if (g_Git.GetOneFile(m_CurrentVersion.ToString(), *fd, filename))
			{
				CString out;
				out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCTSTR>(fd->GetGitPathString()), static_cast<LPCTSTR>(m_CurrentVersion.ToString()), static_cast<LPCTSTR>(filename));
				if (CMessageBox::Show(GetParentHWND(), g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", 2, IDI_WARNING, CString(MAKEINTRESOURCE(IDS_IGNOREBUTTON)), CString(MAKEINTRESOURCE(IDS_ABORTBUTTON))) == 2)
					return;
			}
		}
	}
}

void CGitStatusListCtrl::FileSaveAs(CTGitPath *path)
{
	CAutoReadLock locker(m_guard);
	CString filename;
	filename.Format(L"%s\\%s-%s%s", static_cast<LPCTSTR>(g_Git.CombinePath(path->GetContainingDirectory())), static_cast<LPCTSTR>(path->GetBaseFilename()), static_cast<LPCTSTR>(m_CurrentVersion.ToString(g_Git.GetShortHASHLength())), static_cast<LPCTSTR>(path->GetFileExtension()));
	if (!CAppUtils::FileOpenSave(filename, nullptr, 0, 0, false, GetSafeHwnd()))
		return;
	if (m_CurrentVersion.IsEmpty())
	{
		if (!CopyFile(g_Git.CombinePath(path), filename, false))
		{
			MessageBox(CFormatMessageWrapper(), L"TortoiseGit", MB_OK | MB_ICONERROR);
			return;
		}
	}
	else
	{
		if (g_Git.GetOneFile(m_CurrentVersion.ToString(), *path, filename))
		{
			CString out;
			out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCTSTR>(path->GetGitPathString()), static_cast<LPCTSTR>(m_CurrentVersion.ToString()), static_cast<LPCTSTR>(filename));
			CMessageBox::Show(GetParentHWND(), g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_OK);
			return;
		}
	}
}

int CGitStatusListCtrl::RevertSelectedItemToVersion(bool parent)
{
	CAutoReadLock locker(m_guard);
	if (m_CurrentVersion.IsEmpty())
		return 0;

	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	std::map<CString, int> versionMap;
	while ((index = GetNextSelectedItem(pos)) >= 0)
	{
		auto fentry = GetListEntry(index);
		CString version;
		if (parent)
		{
			int parentNo = fentry->m_ParentNo & PARENT_MASK;
			CString ref;
			ref.Format(L"%s^%d", static_cast<LPCTSTR>(m_CurrentVersion.ToString()), parentNo + 1);
			CGitHash hash;
			if (g_Git.GetHash(hash, ref))
			{
				MessageBox(g_Git.GetGitLastErr(L"Could not get hash of ref \"" + ref + L"\"."), L"TortoiseGit", MB_ICONERROR);
				continue;
			}

			version = hash.ToString();
		}
		else
			version = m_CurrentVersion.ToString();

		CString filename = fentry->GetGitPathString();
		if (!fentry->GetGitOldPathString().IsEmpty())
			filename = fentry->GetGitOldPathString();
		CString cmd, out;
		cmd.Format(L"git.exe checkout %s -- \"%s\"", static_cast<LPCTSTR>(version), static_cast<LPCTSTR>(filename));
		if (g_Git.Run(cmd, &out, CP_UTF8))
		{
			if (MessageBox(out, L"TortoiseGit", MB_ICONEXCLAMATION | MB_OKCANCEL) == IDCANCEL)
				continue;
		}
		else
			versionMap[version]++;
	}

	CString out;
	for (auto it = versionMap.cbegin(); it != versionMap.cend(); ++it)
	{
		CString versionEntry;
		versionEntry.FormatMessage(IDS_STATUSLIST_FILESREVERTED, it->second, static_cast<LPCTSTR>(it->first));
		out += versionEntry + L"\r\n";
	}
	if (!out.IsEmpty())
	{
		if (GetLogicalParent() && GetLogicalParent()->GetSafeHwnd())
			GetLogicalParent()->SendMessage(GITSLNM_NEEDSREFRESH);
		CMessageBox::Show(GetParentHWND(), out, L"TortoiseGit", MB_OK);
	}
	return 0;
}

void CGitStatusListCtrl::OpenFile(CTGitPath*filepath,int mode)
{
	CString file;
	if (m_CurrentVersion.IsEmpty())
		file = g_Git.CombinePath(filepath);
	else
	{
		file = CTempFiles::Instance().GetTempFilePath(false, *filepath, m_CurrentVersion).GetWinPathString();
		CString cmd,out;
		if(g_Git.GetOneFile(m_CurrentVersion.ToString(), *filepath, file))
		{
			out.FormatMessage(IDS_STATUSLIST_CHECKOUTFILEFAILED, static_cast<LPCTSTR>(filepath->GetGitPathString()), static_cast<LPCTSTR>(m_CurrentVersion.ToString()), static_cast<LPCTSTR>(file));
			CMessageBox::Show(GetParentHWND(), g_Git.GetGitLastErr(out, CGit::GIT_CMD_GETONEFILE), L"TortoiseGit", MB_OK);
			return;
		}
		SetFileAttributes(file, FILE_ATTRIBUTE_READONLY);
	}
	if(mode == ALTERNATIVEEDITOR)
	{
		CAppUtils::LaunchAlternativeEditor(file);
		return;
	}

	if (mode == OPEN)
		CAppUtils::ShellOpen(file, GetSafeHwnd());
	else
		CAppUtils::ShowOpenWithDialog(file, GetSafeHwnd());
}

void CGitStatusListCtrl::DeleteSelectedFiles()
{
	CAutoWriteLock locker(m_guard);
	//Collect paths
	std::vector<int> selectIndex;

	POSITION pos = GetFirstSelectedItemPosition();
	int index;
	while ((index = GetNextSelectedItem(pos)) >= 0)
		selectIndex.push_back(index);

	CAutoRepository repo = g_Git.GetGitRepository();
	if (!repo)
	{
		MessageBox(g_Git.GetLibGit2LastErr(L"Could not open repository."), L"TortoiseGit", MB_OK);
		return;
	}
	CAutoIndex gitIndex;
	if (git_repository_index(gitIndex.GetPointer(), repo))
	{
		g_Git.GetLibGit2LastErr(L"Could not open index.");
		return;
	}
	int needWriteIndex = 0;

	//Create file-list ('\0' separated) for SHFileOperation
	CString filelist;
	for (size_t i = 0; i < selectIndex.size(); ++i)
	{
		index = selectIndex[i];

		auto path = GetListEntry(index);
		if (path == nullptr)
			continue;

		// do not report errors as we could remove an unversioned file
		needWriteIndex += git_index_remove_bypath(gitIndex, CUnicodeUtils::GetUTF8(path->GetGitPathString())) == 0 ? 1 : 0;

		if (!path->Exists())
			continue;

		filelist += path->GetWinPathString();
		filelist += L'|';
	}
	filelist += L'|';
	int len = filelist.GetLength();
	auto buf = std::make_unique<TCHAR[]>(len + 2);
	wcscpy_s(buf.get(), len + 2, filelist);
	CStringUtils::PipesToNulls(buf.get(), len + 2);
	SHFILEOPSTRUCT fileop;
	fileop.hwnd = this->m_hWnd;
	fileop.wFunc = FO_DELETE;
	fileop.pFrom = buf.get();
	fileop.pTo = nullptr;
	fileop.fFlags = FOF_NO_CONNECTED_ELEMENTS | ((GetAsyncKeyState(VK_SHIFT) & 0x8000) ? 0 : FOF_ALLOWUNDO);
	fileop.lpszProgressTitle = L"deleting file";
	int result = SHFileOperation(&fileop);

	if ((result == 0 || len == 1) && (!fileop.fAnyOperationsAborted))
	{
		if (needWriteIndex && git_index_write(gitIndex))
			MessageBox(g_Git.GetLibGit2LastErr(L"Could not write index."), L"TortoiseGit", MB_OK);

		if (needWriteIndex)
		{
			CWnd* pParent = GetLogicalParent();
			if (pParent && pParent->GetSafeHwnd())
				pParent->SendMessage(GITSLNM_NEEDSREFRESH);
			SetRedraw(TRUE);
			return;
		}

		SetRedraw(FALSE);
		POSITION pos2 = nullptr;
		while ((pos2 = GetFirstSelectedItemPosition()) != nullptr)
		{
			int index2 = GetNextSelectedItem(pos2);
			if (GetCheck(index2))
				m_nSelected--;
			m_nTotal--;

			RemoveListEntry(index2);
		}
		SetRedraw(TRUE);
	}
}

BOOL CGitStatusListCtrl::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)
{
	switch (message)
	{
	case WM_MENUCHAR:   // only supported by IContextMenu3
		if (g_IContext3)
		{
			g_IContext3->HandleMenuMsg2(message, wParam, lParam, pResult);
			return TRUE;
		}
		break;

	case WM_DRAWITEM:
	case WM_MEASUREITEM:
		if (wParam)
			break; // if wParam != 0 then the message is not menu-related

	case WM_INITMENU:
	case WM_INITMENUPOPUP:
	{
		HMENU hMenu = reinterpret_cast<HMENU>(wParam);
		if ((hMenu == m_hShellMenu) && (GetMenuItemCount(hMenu) == 0))
		{
			// the shell submenu is populated only on request, i.e. right
			// before the submenu is shown
			if (g_pFolderhook)
			{
				delete g_pFolderhook;
				g_pFolderhook = nullptr;
			}
			CTGitPathList targetList;
			FillListOfSelectedItemPaths(targetList);
			if (!targetList.IsEmpty())
			{
				// get IShellFolder interface of Desktop (root of shell namespace)
				if (g_psfDesktopFolder)
					g_psfDesktopFolder->Release();
				SHGetDesktopFolder(&g_psfDesktopFolder);    // needed to obtain full qualified pidl

				// ParseDisplayName creates a PIDL from a file system path relative to the IShellFolder interface
				// but since we use the Desktop as our interface and the Desktop is the namespace root
				// that means that it's a fully qualified PIDL, which is what we need

				if (g_pidlArray)
				{
					for (int i = 0; i < g_pidlArrayItems; i++)
					{
						if (g_pidlArray[i])
							CoTaskMemFree(g_pidlArray[i]);
					}
					CoTaskMemFree(g_pidlArray);
					g_pidlArray = nullptr;
					g_pidlArrayItems = 0;
				}
				int nItems = targetList.GetCount();
				g_pidlArray = static_cast<LPITEMIDLIST*>(CoTaskMemAlloc((nItems + 10) * sizeof(LPITEMIDLIST)));
				SecureZeroMemory(g_pidlArray, (nItems + 10) * sizeof(LPITEMIDLIST));
				int succeededItems = 0;
				PIDLIST_RELATIVE pidl = nullptr;

				int bufsize = 1024;
				auto filepath = std::make_unique<WCHAR[]>(bufsize);
				for (int i = 0; i < nItems; i++)
				{
					CString fullPath = g_Git.CombinePath(targetList[i].GetWinPath());
					if (bufsize < fullPath.GetLength())
					{
						bufsize = fullPath.GetLength() + 3;
						filepath = std::make_unique<WCHAR[]>(bufsize);
					}
					wcscpy_s(filepath.get(), bufsize, fullPath);
					if (SUCCEEDED(g_psfDesktopFolder->ParseDisplayName(nullptr, 0, filepath.get(), nullptr, &pidl, nullptr)))
						g_pidlArray[succeededItems++] = pidl; // copy pidl to pidlArray
				}
				if (succeededItems == 0)
				{
					CoTaskMemFree(g_pidlArray);
					g_pidlArray = nullptr;
				}

				g_pidlArrayItems = succeededItems;

				if (g_pidlArrayItems)
				{
					CString ext = targetList[0].GetFileExtension();

					ASSOCIATIONELEMENT const rgAssocItem[] =
					{
						{ ASSOCCLASS_PROGID_STR, nullptr, ext },
						{ ASSOCCLASS_SYSTEM_STR, nullptr, ext },
						{ ASSOCCLASS_APP_STR, nullptr, ext },
						{ ASSOCCLASS_STAR, nullptr, nullptr },
						{ ASSOCCLASS_FOLDER, nullptr, nullptr },
					};
					IQueryAssociations* pIQueryAssociations = nullptr;
					if (FAILED(AssocCreateForClasses(rgAssocItem, ARRAYSIZE(rgAssocItem), IID_IQueryAssociations, reinterpret_cast<void**>(&pIQueryAssociations))))
						pIQueryAssociations = nullptr; // not a problem, it works without this

					g_pFolderhook = new CIShellFolderHook(g_psfDesktopFolder, targetList);
					LPCONTEXTMENU icm1 = nullptr;

					DEFCONTEXTMENU dcm = { 0 };
					dcm.hwnd = m_hWnd;
					dcm.psf = g_pFolderhook;
					dcm.cidl = g_pidlArrayItems;
					dcm.apidl = const_cast<PCUITEMID_CHILD_ARRAY>(g_pidlArray);
					dcm.punkAssociationInfo = pIQueryAssociations;
					if (SUCCEEDED(SHCreateDefaultContextMenu(&dcm, IID_IContextMenu, reinterpret_cast<void**>(&icm1))))
					{
						int iMenuType = 0;  // to know which version of IContextMenu is supported
						if (icm1)
						{   // since we got an IContextMenu interface we can now obtain the higher version interfaces via that
							if (icm1->QueryInterface(IID_IContextMenu3, reinterpret_cast<void**>(&m_pContextMenu)) == S_OK)
								iMenuType = 3;
							else if (icm1->QueryInterface(IID_IContextMenu2, reinterpret_cast<void**>(&m_pContextMenu)) == S_OK)
								iMenuType = 2;

							if (m_pContextMenu)
								icm1->Release(); // we can now release version 1 interface, cause we got a higher one
							else
							{
								// since no higher versions were found
								// redirect ppContextMenu to version 1 interface
								iMenuType = 1;
								m_pContextMenu = icm1;
							}
						}
						if (m_pContextMenu)
						{
							// lets fill the our popup menu
							UINT flags = CMF_NORMAL;
							flags |= (GetKeyState(VK_SHIFT) & 0x8000) != 0 ? CMF_EXTENDEDVERBS : 0;
							m_pContextMenu->QueryContextMenu(hMenu, 0, SHELL_MIN_CMD, SHELL_MAX_CMD, flags);


							// subclass window to handle menu related messages in CShellContextMenu
							if (iMenuType > 1)  // only subclass if its version 2 or 3
							{
								if (iMenuType == 2)
									g_IContext2 = static_cast<LPCONTEXTMENU2>(m_pContextMenu);
								else    // version 3
									g_IContext3 = static_cast<LPCONTEXTMENU3>(m_pContextMenu);
							}
						}
					}
					if (pIQueryAssociations)
						pIQueryAssociations->Release();
				}
			}
			if (g_IContext3)
				g_IContext3->HandleMenuMsg2(message, wParam, lParam, pResult);
			else if (g_IContext2)
				g_IContext2->HandleMenuMsg(message, wParam, lParam);
			return TRUE;
		}
	}

	break;
	default:
		break;
	}

	return __super::OnWndMsg(message, wParam, lParam, pResult);
}

CTGitPath* CGitStatusListCtrl::GetListEntry(int index)
{
	ATLASSERT(m_guard.GetCurrentThreadStatus());
	if (static_cast<size_t>(index) >= m_arListArray.size())
	{
		ATLASSERT(FALSE);
		return nullptr;
	}
	if (m_arListArray[index] >= m_arStatusArray.size())
	{
		ATLASSERT(FALSE);
		return nullptr;
	}
	return const_cast<CTGitPath*>(m_arStatusArray[m_arListArray[index]]);
}

void CGitStatusListCtrl::OnSysColorChange()
{
	__super::OnSysColorChange();
	if (m_nBackgroundImageID)
		CAppUtils::SetListCtrlBackgroundImage(GetSafeHwnd(), m_nBackgroundImageID);
}

ULONG CGitStatusListCtrl::GetGestureStatus(CPoint /*ptTouch*/)
{
	return 0;
}

#define CHANGELIST_FILE_NAME	L"tgitchangelist"

void CGitStatusListCtrl::LoadChangelists()
{
	CString tgitChangelistPath;
	if (!GitAdminDir::GetWorktreeAdminDirPath(g_Git.m_CurrentDir, tgitChangelistPath))
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_CHANGELIST_LOAD, IDS_APPNAME, MB_ICONERROR);
		return;
	}

	tgitChangelistPath += CHANGELIST_FILE_NAME;

	if (!PathFileExists(tgitChangelistPath))
		return;

	try
	{
		CStdioFile file(tgitChangelistPath, CFile::typeText | CFile::modeRead | CFile::shareDenyWrite);
		CString changelistName = GITSLC_IGNORECHANGELIST;
		CString strLine;
		while (file.ReadString(strLine))
		{
			strLine = strLine.Trim();
			if (strLine.IsEmpty())
				continue;

			if (CStringUtils::StartsWith(strLine, L"<") && CStringUtils::EndsWith(strLine, L">"))
			{ //this is changelist name
				changelistName = strLine.Mid(1, strLine.GetLength() - 2);
				m_changelists.insert_or_assign(changelistName, 0);
			}
			else // this is git path
				m_pathToChangelist.insert_or_assign(strLine, changelistName);
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		pE->Delete();
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_CHANGELIST_LOAD, IDS_APPNAME, MB_ICONERROR);
	}
}

void CGitStatusListCtrl::SaveChangelists()
{
	CString tgitChangelistPath;
	if (!GitAdminDir::GetWorktreeAdminDirPath(g_Git.m_CurrentDir, tgitChangelistPath))
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_CHANGELIST_SAVE, IDS_APPNAME, MB_ICONERROR);
		return;
	}

	tgitChangelistPath += CHANGELIST_FILE_NAME;

	std::map<CString, std::set<CString>> changelistToPath;
	for (auto it = m_pathToChangelist.cbegin(); it != m_pathToChangelist.cend(); ++it)
	{
		auto itChangelist = changelistToPath.find(it->second);
		if (itChangelist == changelistToPath.end())
		{
			std::set<CString> paths;
			paths.insert(it->first);
			changelistToPath.insert(std::make_pair(it->second, paths));
		}
		else
			itChangelist->second.insert(it->first);
	}

	try
	{
		CStdioFile file(tgitChangelistPath, CFile::typeText | CFile::modeCreate | CFile::modeWrite | CFile::shareDenyWrite);
		for (auto itChangelist = changelistToPath.cbegin(); itChangelist != changelistToPath.cend(); ++itChangelist)
		{
			if (itChangelist != changelistToPath.cbegin())
				file.WriteString(L"\n");

			file.WriteString(L"<" + itChangelist->first + L">\n");
			for (auto itPath = itChangelist->second.cbegin(); itPath != itChangelist->second.cend(); ++itPath)
				file.WriteString(*itPath + L"\n");
		}

		file.Close();
	}
	catch (CFileException* pE)
	{
		pE->Delete();
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_CHANGELIST_SAVE, IDS_APPNAME, MB_ICONERROR);
	}
}

void CGitStatusListCtrl::PruneChangelists()
{
	CAutoReadLock locker(m_guard);
	std::set<CString> unchecked;
	int nListboxEntries = GetItemCount();
	for (int nItem = 0; nItem < nListboxEntries; ++nItem)
	{
		auto pentry = GetListEntry(nItem);
		if (!pentry || (pentry->m_Checked && m_restorepaths.find(pentry->GetWinPathString()) == m_restorepaths.end()))
			continue;

		unchecked.emplace(pentry->GetGitPathString());
	}

	auto it1 = unchecked.cbegin();
	auto it2 = m_pathToChangelist.begin();

	while (it1 != unchecked.cend() && it2 != m_pathToChangelist.end())
	{
		if (*it1 > it2->first)
			it2 = m_pathToChangelist.erase(it2);
		else if (it2->first > *it1)
			++it1;
		else
		{
			++it1;
			++it2;
		}
	}
	while (it2 != m_pathToChangelist.end())
	{
		it2 = m_pathToChangelist.erase(it2);
	}
}
