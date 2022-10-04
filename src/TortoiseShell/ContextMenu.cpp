// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2012, 2014-2016, 2018 - TortoiseSVN
// Copyright (C) 2008-2022 - TortoiseGit

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
#include <comutil.h>
#include <winrt/base.h>
#include <wrl/client.h>
#include <windowsx.h>
#include "ShellExt.h"
#include "ItemIDList.h"
#include "PreserveChdir.h"
#include "UnicodeUtils.h"
#include "Git.h"
#include "GitStatus.h"
#include "TGitPath.h"
#include "PathUtils.h"
#include "CreateProcessHelper.h"
#include "FormatMessageWrapper.h"
#include "../TGitCache/CacheInterface.h"
#include "resource.h"
#include "LoadIconEx.h"
#include "ClipboardHelper.h"

#pragma comment(lib, "comsupp.lib")

#define GetPIDLFolder(pida) reinterpret_cast<LPCITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + (pida)->aoffset[0])
#define GetPIDLItem(pida, i) reinterpret_cast<LPCITEMIDLIST>(reinterpret_cast<LPBYTE>(pida) + (pida)->aoffset[i + 1])

int g_shellidlist=RegisterClipboardFormat(CFSTR_SHELLIDLIST);

extern MenuInfo menuInfo[];
static int g_syncSeq = 0;

STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder, LPDATAOBJECT pDataObj, HKEY /* hRegKey */)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: Initialize\n");
	PreserveChdir preserveChdir;
	files_.clear();
	folder_.clear();
	uuidSource.clear();
	uuidTarget.clear();
	itemStates = 0;
	itemStatesFolder = 0;
	std::wstring statuspath;
	git_wc_status_kind fetchedstatus = git_wc_status_none;
	// get selected files/folders
	if (pDataObj)
	{
		STGMEDIUM medium;
		FORMATETC fmte = { static_cast<CLIPFORMAT>(g_shellidlist),
			nullptr,
			DVASPECT_CONTENT,
			-1,
			TYMED_HGLOBAL};
		HRESULT hres = pDataObj->GetData(&fmte, &medium);

		if (SUCCEEDED(hres) && medium.hGlobal)
		{
			if (m_State == FileStateDropHandler)
			{
				if (!CRegStdDWORD(L"Software\\TortoiseGit\\EnableDragContextMenu", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY))
				{
					ReleaseStgMedium(&medium);
					return S_OK;
				}

				FORMATETC etc = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg = { TYMED_HGLOBAL };
				if ( FAILED( pDataObj->GetData ( &etc, &stg )))
				{
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}


				auto drop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
				if (!drop)
				{
					ReleaseStgMedium ( &stg );
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}

				int count = DragQueryFile(drop, UINT(-1), nullptr, 0);
				if (count == 1)
					itemStates |= ITEMIS_ONLYONE;
				for (int i = 0; i < count; i++)
				{
					// find the path length in chars
					UINT len = DragQueryFile(drop, i, nullptr, 0);
					if (len == 0)
						continue;
					auto szFileName = std::make_unique<wchar_t[]>(len + 1);
					if (0 == DragQueryFile(drop, i, szFileName.get(), len + 1))
						continue;
					auto str = std::wstring(szFileName.get());
					if ((!str.empty()) && (g_ShellCache.IsContextPathAllowed(szFileName.get())))
					{
						{
							CTGitPath strpath;
							strpath.SetFromWin(str.c_str());
							itemStates |= (strpath.GetFileExtension().CompareNoCase(L".diff") == 0) ? ITEMIS_PATCHFILE : 0;
							itemStates |= (strpath.GetFileExtension().CompareNoCase(L".patch") == 0) ? ITEMIS_PATCHFILE : 0;
						}
						files_.push_back(str);
						if (i == 0)
						{
							//get the git status of the item
							git_wc_status_kind status = git_wc_status_none;
							CTGitPath askedpath;
							askedpath.SetFromWin(str.c_str());
							CString workTreePath;
							if (!askedpath.HasAdminDir(&workTreePath) && GitAdminDir::IsBareRepo(str.c_str()))
								itemStates |= ITEMIS_BAREREPO; // TODO: optimize
							uuidSource = workTreePath;
							try
							{
								if (g_ShellCache.GetCacheType() == ShellCache::exe && g_ShellCache.IsPathAllowed(str.c_str()))
								{
									CTGitPath tpath(str.c_str());
									if (!tpath.HasAdminDir())
									{
										status = git_wc_status_none;
										continue;
									}
									if (tpath.IsAdminDir())
									{
										status = git_wc_status_none;
										continue;
									}
									TGITCacheResponse itemStatus = { 0 };
									if (m_remoteCacheLink.GetStatusFromRemoteCache(tpath, &itemStatus, true))
									{
										fetchedstatus = status = static_cast<git_wc_status_kind>(itemStatus.m_status);
										if (askedpath.IsDirectory())//if ((stat.status->entry)&&(stat.status->entry->kind == git_node_dir))
										{
											itemStates |= ITEMIS_FOLDER;
											if ((status != git_wc_status_unversioned) && (status != git_wc_status_ignored) && (status != git_wc_status_none))
												itemStates |= ITEMIS_FOLDERINGIT;
										}
									}
								}
								else
								{
									GitStatus stat;
									stat.GetStatus(CTGitPath(str.c_str()), false, false, true);
									if (stat.status)
									{
										statuspath = str;
										status = stat.status->status;
										fetchedstatus = status;
										if ( askedpath.IsDirectory() )//if ((stat.status->entry)&&(stat.status->entry->kind == git_node_dir))
										{
											itemStates |= ITEMIS_FOLDER;
											if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
												itemStates |= ITEMIS_FOLDERINGIT;
										}
										//if ((stat.status->entry)&&(stat.status->entry->uuid))
										//	uuidSource = CUnicodeUtils::StdGetUnicode(stat.status->entry->uuid);
									}
									else
									{
										// sometimes, git_client_status() returns with an error.
										// in that case, we have to check if the working copy is versioned
										// anyway to show the 'correct' context menu
										if (askedpath.HasAdminDir())
											status = git_wc_status_normal;
									}
								}
							}
							catch ( ... )
							{
								CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in GitStatus::GetStatus()\n");
							}

							// TODO: should we really assume any sub-directory to be versioned
							//       or only if it contains versioned files
							itemStates |= askedpath.GetAdminDirMask();

							if ((status == git_wc_status_unversioned) || (status == git_wc_status_ignored) || (status == git_wc_status_none))
								itemStates &= ~ITEMIS_INGIT;

							if (status == git_wc_status_ignored)
								itemStates |= ITEMIS_IGNORED;
							if (status == git_wc_status_normal)
								itemStates |= ITEMIS_NORMAL;
							if (status == git_wc_status_conflicted)
								itemStates |= ITEMIS_CONFLICTED;
							if (status == git_wc_status_added)
								itemStates |= ITEMIS_ADDED;
							if (status == git_wc_status_deleted)
								itemStates |= ITEMIS_DELETED;
						}
					}
				} // for (int i = 0; i < count; i++)
				GlobalUnlock ( drop );
				ReleaseStgMedium ( &stg );

			} // if (m_State == FileStateDropHandler)
			else
			{
				//Enumerate PIDLs which the user has selected
				auto cida = static_cast<CIDA*>(GlobalLock(medium.hGlobal));
				ItemIDList parent( GetPIDLFolder (cida));

				int count = cida->cidl;
				BOOL statfetched = FALSE;
				for (int i = 0; i < count; ++i)
				{
					ItemIDList child (GetPIDLItem (cida, i), &parent);
					std::wstring str = child.toString();
					if ((str.empty() == false)&&(g_ShellCache.IsContextPathAllowed(str.c_str())))
					{
						//check if our menu is requested for a git admin directory
						if (GitAdminDir::IsAdminDirPath(str.c_str()))
							continue;

						files_.push_back(str);
						CTGitPath strpath;
						strpath.SetFromWin(str.c_str());
						itemStates |= (strpath.GetFileExtension().CompareNoCase(L".diff") == 0) ? ITEMIS_PATCHFILE : 0;
						itemStates |= (strpath.GetFileExtension().CompareNoCase(L".patch") == 0) ? ITEMIS_PATCHFILE : 0;
						if (!statfetched)
						{
							//get the git status of the item
							git_wc_status_kind status = git_wc_status_none;
							if ((g_ShellCache.IsSimpleContext())&&(strpath.IsDirectory()))
							{
								if (strpath.HasAdminDir())
									status = git_wc_status_normal;
							}
							else
							{
								try
								{
									if (g_ShellCache.GetCacheType() == ShellCache::exe && g_ShellCache.IsPathAllowed(str.c_str()))
									{
										CTGitPath tpath(str.c_str());
										if(!tpath.HasAdminDir())
										{
											status = git_wc_status_none;
											continue;
										}
										if(tpath.IsAdminDir())
										{
											status = git_wc_status_none;
											continue;
										}
										TGITCacheResponse itemStatus = { 0 };
										if (m_remoteCacheLink.GetStatusFromRemoteCache(tpath, &itemStatus, true))
										{
											fetchedstatus = status = static_cast<git_wc_status_kind>(itemStatus.m_status);
											if (strpath.IsDirectory())//if ((stat.status->entry)&&(stat.status->entry->kind == git_node_dir))
											{
												itemStates |= ITEMIS_FOLDER;
												if ((status != git_wc_status_unversioned) && (status != git_wc_status_ignored) && (status != git_wc_status_none))
													itemStates |= ITEMIS_FOLDERINGIT;
											}
											if (status == git_wc_status_conflicted)//if ((stat.status->entry)&&(stat.status->entry->conflict_wrk))
												itemStates |= ITEMIS_CONFLICTED;
										}
									}
									else
									{
										GitStatus stat;
										if (strpath.HasAdminDir())
											stat.GetStatus(strpath, false, false, true);
										statuspath = str;
										if (stat.status)
										{
											status = stat.status->status;
											fetchedstatus = status;
											if ( strpath.IsDirectory() )//if ((stat.status->entry)&&(stat.status->entry->kind == git_node_dir))
											{
												itemStates |= ITEMIS_FOLDER;
												if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
													itemStates |= ITEMIS_FOLDERINGIT;
											}
											// TODO: do we need to check that it's not a dir? does conflict options makes sense for dir in git?
											if (status == git_wc_status_conflicted)//if ((stat.status->entry)&&(stat.status->entry->conflict_wrk))
												itemStates |= ITEMIS_CONFLICTED;
											//if ((stat.status->entry)&&(stat.status->entry->uuid))
											//	uuidSource = CUnicodeUtils::StdGetUnicode(stat.status->entry->uuid);
										}
										else
										{
											// sometimes, git_client_status() returns with an error.
											// in that case, we have to check if the working copy is versioned
											// anyway to show the 'correct' context menu
											if (strpath.HasAdminDir())
											{
												status = git_wc_status_normal;
												fetchedstatus = status;
											}
										}
									}
									statfetched = TRUE;
								}
								catch ( ... )
								{
									CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in GitStatus::GetStatus()\n");
								}
							}

							itemStates |= strpath.GetAdminDirMask();

							if ((status == git_wc_status_unversioned)||(status == git_wc_status_ignored)||(status == git_wc_status_none))
								itemStates &= ~ITEMIS_INGIT;
							if (status == git_wc_status_ignored)
							{
								itemStates |= ITEMIS_IGNORED;
							}

							if (status == git_wc_status_normal)
								itemStates |= ITEMIS_NORMAL;
							if (status == git_wc_status_conflicted)
								itemStates |= ITEMIS_CONFLICTED;
							if (status == git_wc_status_added)
								itemStates |= ITEMIS_ADDED;
							if (status == git_wc_status_deleted)
								itemStates |= ITEMIS_DELETED;
						}
					}
				} // for (int i = 0; i < count; ++i)
				ItemIDList child (GetPIDLItem (cida, 0), &parent);
				if (g_ShellCache.HasGITAdminDir(child.toString().c_str(), FALSE))
					itemStates |= ITEMIS_INVERSIONEDFOLDER;

				if (GitAdminDir::IsBareRepo(child.toString().c_str()))
					itemStates = ITEMIS_BAREREPO;

				GlobalUnlock(medium.hGlobal);

				// if the item is a versioned folder, check if there's a patch file
				// in the clipboard to be used in "Apply Patch"
				UINT cFormatDiff = RegisterClipboardFormat(L"TGIT_UNIFIEDDIFF");
				if (cFormatDiff)
				{
					if (IsClipboardFormatAvailable(cFormatDiff))
						itemStates |= ITEMIS_PATCHINCLIPBOARD;
				}
				if (IsClipboardFormatAvailable(CF_HDROP))
					itemStates |= ITEMIS_PATHINCLIPBOARD;

			}

			ReleaseStgMedium ( &medium );
			if (medium.pUnkForRelease)
			{
				IUnknown* relInterface = medium.pUnkForRelease;
				relInterface->Release();
			}
		}
	}

	// get folder background
	if (pIDFolder)
	{
		ItemIDList list(pIDFolder);
		folder_ = list.toString();
		git_wc_status_kind status = git_wc_status_none;
		if (IsClipboardFormatAvailable(CF_HDROP))
			itemStatesFolder |= ITEMIS_PATHINCLIPBOARD;

		CTGitPath askedpath;
		askedpath.SetFromWin(folder_.c_str());

		if (g_ShellCache.IsContextPathAllowed(folder_.c_str()))
		{
			if (folder_.compare(statuspath)!=0)
			{
				CString worktreePath;
				askedpath.HasAdminDir(&worktreePath);
				uuidTarget = worktreePath;
				try
				{
					if (g_ShellCache.GetCacheType() == ShellCache::exe && g_ShellCache.IsPathAllowed(folder_.c_str()))
					{
						CTGitPath tpath(folder_.c_str());
						if(!tpath.HasAdminDir())
							status = git_wc_status_none;
						else if(tpath.IsAdminDir())
							status = git_wc_status_none;
						else
						{
							TGITCacheResponse itemStatus = { 0 };
							if (m_remoteCacheLink.GetStatusFromRemoteCache(tpath, &itemStatus, true))
								status = static_cast<git_wc_status_kind>(itemStatus.m_status);
						}
					}
					else
					{
						GitStatus stat;
						stat.GetStatus(CTGitPath(folder_.c_str()), false, false, true);
						if (stat.status)
							status = stat.status->status;
						else
						{
							// sometimes, git_client_status() returns with an error.
							// in that case, we have to check if the working copy is versioned
							// anyway to show the 'correct' context menu
							if (askedpath.HasAdminDir())
								status = git_wc_status_normal;
						}
					}

					//if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
					itemStatesFolder |= askedpath.GetAdminDirMask();

					if ((status == git_wc_status_unversioned)||(status == git_wc_status_ignored)||(status == git_wc_status_none))
						itemStates &= ~ITEMIS_INGIT;

					if (status == git_wc_status_normal)
						itemStatesFolder |= ITEMIS_NORMAL;
					if (status == git_wc_status_conflicted)
						itemStatesFolder |= ITEMIS_CONFLICTED;
					if (status == git_wc_status_added)
						itemStatesFolder |= ITEMIS_ADDED;
					if (status == git_wc_status_deleted)
						itemStatesFolder |= ITEMIS_DELETED;

					if (GitAdminDir::IsBareRepo(askedpath.GetWinPath()))
						itemStatesFolder = ITEMIS_BAREREPO;
				}
				catch ( ... )
				{
					CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in GitStatus::GetStatus()\n");
				}
			}
			else
			{
				status = fetchedstatus;
			}
			//if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
			itemStatesFolder |= askedpath.GetAdminDirMask();

			if (status == git_wc_status_ignored)
				itemStatesFolder |= ITEMIS_IGNORED;
			itemStatesFolder |= ITEMIS_FOLDER;
			if (files_.empty())
				itemStates |= ITEMIS_ONLYONE;
			if (m_State != FileStateDropHandler)
				itemStates |= itemStatesFolder;
		}
		else
		{
			folder_.clear();
			status = fetchedstatus;
		}
	}
	if (files_.size() == 2)
		itemStates |= ITEMIS_TWO;
	if ((files_.size() == 1)&&(g_ShellCache.IsContextPathAllowed(files_.front().c_str())))
	{
		itemStates |= ITEMIS_ONLYONE;
		if (m_State != FileStateDropHandler)
		{
			if (PathIsDirectory(files_.front().c_str()))
			{
				folder_ = files_.front();
				git_wc_status_kind status = git_wc_status_none;
				CTGitPath askedpath;
				askedpath.SetFromWin(folder_.c_str());

				if (folder_.compare(statuspath)!=0)
				{
					try
					{
						GitStatus stat;
						stat.GetStatus(CTGitPath(folder_.c_str()), false, false, true);
						if (stat.status)
							status = stat.status->status;
					}
					catch ( ... )
					{
						CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Exception in GitStatus::GetStatus()\n");
					}
				}
				else
				{
					status = fetchedstatus;
				}
				//if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
				itemStates |= askedpath.GetAdminDirMask();

				if ((status == git_wc_status_unversioned)||(status == git_wc_status_ignored)||(status == git_wc_status_none))
					itemStates &= ~ITEMIS_INGIT;

				if (status == git_wc_status_ignored)
					itemStates |= ITEMIS_IGNORED;
				itemStates |= ITEMIS_FOLDER;
				if (status == git_wc_status_added)
					itemStates |= ITEMIS_ADDED;
				if (status == git_wc_status_deleted)
					itemStates |= ITEMIS_DELETED;

			}
		}

	}

	return S_OK;
}

void CShellExt::InsertGitMenu(BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, GitCommands com, UINT /*uFlags*/)
{
	wchar_t menutextbuffer[512] = { 0 };
	wchar_t verbsbuffer[255] = { 0 };
	MAKESTRING(stringid);

	if (istop && menu)
	{
		//menu entry for the top context menu, so append an "Git " before
		//the menu text to indicate where the entry comes from
		wcscpy_s(menutextbuffer, L"Git ");
		if (!g_ShellCache.HasShellMenuAccelerators())
		{
			// remove the accelerators
			std::wstring temp = stringtablebuffer;
			temp.erase(std::remove(temp.begin(), temp.end(), '&'), temp.end());
			wcscpy_s(stringtablebuffer, temp.c_str());
		}
	}
	wcscat_s(menutextbuffer, stringtablebuffer);

	// insert branch name into "Git Commit..." entry, so it looks like "Git Commit "master"..."
	// so we have an easy and fast way to check the current branch
	// (the other alternative is using a separate disabled menu entry, the code is already done but commented out)
	if (com == ShellMenuCommit)
	{
		// get branch name
		CTGitPath path(folder_.empty() ? files_.front().c_str() : folder_.c_str());
		CString sProjectRoot;
		CString sBranchName;

		if (path.GetAdminDirMask() & ITEMIS_SUBMODULE)
		{
			if (istop)
				wcscpy_s(menutextbuffer, L"Git ");
			else
				menutextbuffer[0] = L'\0';
			MAKESTRING(IDS_MENUCOMMITSUBMODULE);
			wcscat_s(menutextbuffer, stringtablebuffer);
		}

		if (path.HasAdminDir(&sProjectRoot) && !CGit::GetCurrentBranchFromFile(sProjectRoot, sBranchName))
		{
			if (sBranchName.GetLength() == 2 * GIT_HASH_SIZE)
			{
				// if SHA1 only show 4 first bytes
				BOOL bIsSha1 = TRUE;
				for (int i = 0; i < 2 * GIT_HASH_SIZE; ++i)
					if ( !iswxdigit(sBranchName[i]) )
					{
						bIsSha1 = FALSE;
						break;
					}
				if (bIsSha1)
					sBranchName = sBranchName.Left(g_Git.GetShortHASHLength()) + L"...";
			}

			// sanity check
			if (sBranchName.GetLength() > 64)
				sBranchName = sBranchName.Left(64) + L"...";

			// scan to before "..."
			LPWSTR s = menutextbuffer + wcslen(menutextbuffer)-1;
			if (s > menutextbuffer)
			{
				while (s > menutextbuffer)
				{
					if (*s != L'.')
					{
						s++;
						break;
					}
					s--;
				}
			}
			else
			{
				s = menutextbuffer;
			}

			// append branch name and end with ...
			wcscpy_s(s, 255 - wcslen(menutextbuffer) - 1, L" -> \"" + CStringUtils::EscapeAccellerators(sBranchName) + L"\"...");
		}
	}

	if (com == ShellMenuDiffLater)
	{
		std::wstring sPath = regDiffLater;
		if (!sPath.empty())
		{
			// add the path of the saved file
			wchar_t compact[2 * GIT_HASH_SIZE] = { 0 };
			PathCompactPathEx(compact, sPath.c_str(), _countof(compact) - 1, 0);
			MAKESTRING(IDS_MENUDIFFNOW);
			CString sMenu;
			sMenu.Format(CString(stringtablebuffer), compact);
			wcscpy_s(menutextbuffer, sMenu);
		}
	}

	MENUITEMINFO menuiteminfo = { 0 };
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
	menuiteminfo.fType = MFT_STRING;
	menuiteminfo.dwTypeData = menutextbuffer;
	if (icon)
	{
		menuiteminfo.fMask |= MIIM_BITMAP;
		menuiteminfo.hbmpItem = m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, icon);
	}
	menuiteminfo.wID = static_cast<UINT>(id);
	if (menu)
		InsertMenuItem(menu, pos, TRUE, &menuiteminfo);
	else
		m_explorerCommands.push_back(CExplorerCommand(menutextbuffer, icon, com, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
	if (istop)
	{
		//menu entry for the top context menu, so append an "Git " before
		//the menu text to indicate where the entry comes from
		wcscpy_s(menutextbuffer, L"Git ");
	}
	LoadString(g_hResInst, stringid, verbsbuffer, _countof(verbsbuffer));
	wcscat_s(menutextbuffer, verbsbuffer);
	auto verb = std::wstring(menutextbuffer);
	if (verb.find('&') != -1)
	{
		verb.erase(verb.find('&'),1);
	}
	myVerbsMap[verb] = id - idCmdFirst;
	myVerbsMap[verb] = id;
	myVerbsIDMap[id - idCmdFirst] = verb;
	myVerbsIDMap[id] = verb;
	// We store the relative and absolute diameter
	// (drawitem callback uses absolute, others relative)
	myIDMap[id - idCmdFirst] = com;
	myIDMap[id] = com;
	if (!istop)
		mySubMenuMap[pos] = com;
}

bool CShellExt::WriteClipboardPathsToTempFile(std::wstring& tempfile)
{
	tempfile = std::wstring();
	//write all selected files and paths to a temporary file
	//for TortoiseGitProc.exe to read out again.
	DWORD written = 0;
	DWORD pathlength = GetTortoiseGitTempPath(0, nullptr);
	auto path = std::make_unique<wchar_t[]>(pathlength + 1);
	auto tempFile = std::make_unique<wchar_t[]>(pathlength + 100);
	GetTortoiseGitTempPath(pathlength+1, path.get());
	GetTempFileName(path.get(), L"git", 0, tempFile.get());
	tempfile = std::wstring(tempFile.get());

	CAutoFile file = ::CreateFile(tempFile.get(),
		GENERIC_WRITE,
		FILE_SHARE_READ,
		nullptr,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_TEMPORARY,
		nullptr);

	if (!file)
		return false;

	if (!IsClipboardFormatAvailable(CF_HDROP))
		return false;
	CClipboardHelper clipboardHelper;
	if (!clipboardHelper.Open(nullptr))
		return false;

	HGLOBAL hglb = GetClipboardData(CF_HDROP);
	SCOPE_EXIT
	{
		GlobalUnlock(hglb);
	};
	auto hDrop = static_cast<HDROP>(GlobalLock(hglb));
	if (!hDrop)
		return false;
	SCOPE_EXIT { GlobalUnlock(hDrop); };

	wchar_t szFileName[MAX_PATH] = { 0 };
	UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, nullptr, 0);
	for(UINT i = 0; i < cFiles; ++i)
	{
		DragQueryFile(hDrop, i, szFileName, _countof(szFileName));
		std::wstring filename = szFileName;
		::WriteFile (file, filename.c_str(), static_cast<DWORD>(filename.size()) * sizeof(wchar_t), &written, nullptr);
		::WriteFile(file, L"\n", 2, &written, nullptr);
	}

	return true;
}

std::wstring CShellExt::WriteFileListToTempFile(bool bFoldersOnly, const std::vector<std::wstring>& files, const std::wstring folder)
{
	//write all selected files and paths to a temporary file
	//for TortoiseGitProc.exe to read out again.
	DWORD pathlength = GetTortoiseGitTempPath(0, nullptr);
	auto path = std::make_unique<wchar_t[]>(pathlength + 1);
	auto tempFile = std::make_unique<wchar_t[]>(pathlength + 100);
	GetTortoiseGitTempPath(pathlength + 1, path.get());
	GetTempFileName(path.get(), L"git", 0, tempFile.get());
	auto retFilePath = std::wstring(tempFile.get());

	CAutoFile file = ::CreateFile (tempFile.get(),
								GENERIC_WRITE,
								FILE_SHARE_READ,
								nullptr,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_TEMPORARY,
								nullptr);

	if (!file)
	{
		MessageBox(nullptr, L"Could not create temporary file. Please check (the permissions of) your temp-folder: " + CString(tempFile.get()), L"TortoiseGit", MB_ICONERROR);
		return std::wstring();
	}

	DWORD written = 0;
	if (files.empty())
	{
		::WriteFile (file, folder.c_str(), static_cast<DWORD>(folder.size()) * sizeof(wchar_t), &written, 0);
		::WriteFile(file, L"\n", 2, &written, 0);
	}

	for (const auto& file_ : files)
	{
		if (bFoldersOnly && !PathIsDirectory(file_.c_str()))
			continue;

		::WriteFile(file, file_.c_str(), static_cast<DWORD>(file_.size()) * sizeof(wchar_t), &written, 0);
		::WriteFile(file, L"\n", 2, &written, 0);
	}
	return retFilePath;
}

STDMETHODIMP CShellExt::QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT &indexMenu)
{
	if (!CRegStdDWORD(L"Software\\TortoiseGit\\EnableDragContextMenu", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY))
		return S_OK;

	PreserveChdir preserveChdir;
	LoadLangDll();

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return S_OK;					//we don't change the default action

	if (files_.empty() || folder_.empty())
		return S_OK;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return S_OK;

	bool bSourceAndTargetFromSameRepository = ((uuidSource.size() == uuidTarget.size() && _wcsnicmp(uuidSource.c_str(), uuidTarget.c_str(), uuidSource.size()) == 0)) || uuidSource.empty() || uuidTarget.empty();

	//the drop handler only has eight commands, but not all are visible at the same time:
	//if the source file(s) are under version control then those files can be moved
	//to the new location or they can be moved with a rename,
	//if they are unversioned then they can be added to the working copy
	//if they are versioned, they also can be exported to an unversioned location
	UINT idCmd = idCmdFirst;

	bool moveAvailable = false;
	// Git move here
	// available if source is versioned but not added, target is versioned, source and target from same repository or target folder is added
	if ((bSourceAndTargetFromSameRepository || (itemStatesFolder & ITEMIS_ADDED)) && (itemStatesFolder & ITEMIS_FOLDERINGIT) && ((itemStates & (ITEMIS_NORMAL | ITEMIS_INGIT | ITEMIS_FOLDERINGIT))))
	{
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVEMENU, 0, idCmdFirst, ShellMenuDropMove, uFlags);
		moveAvailable = true;
	}

	// Git move and rename here
	// available if source is a single, versioned but not added item, target is versioned, source and target from same repository or target folder is added
	if ((bSourceAndTargetFromSameRepository || (itemStatesFolder & ITEMIS_ADDED)) && (itemStatesFolder & ITEMIS_FOLDERINGIT) && (itemStates & (ITEMIS_NORMAL | ITEMIS_INGIT | ITEMIS_FOLDERINGIT)) && (itemStates & ITEMIS_ONLYONE))
	{
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVERENAMEMENU, 0, idCmdFirst, ShellMenuDropMoveRename, uFlags);
		moveAvailable = true;
	}

	// Git copy here
	// available if source is versioned but not added, target is versioned, source and target from same repository or target folder is added
	//if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINGIT)&&(itemStates & ITEMIS_INGIT)&&((~itemStates) & ITEMIS_ADDED))
	//	InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYMENU, 0, idCmdFirst, ShellMenuDropCopy, uFlags);

	// Git copy and rename here, source and target from same repository
	// available if source is a single, versioned but not added item, target is versioned or target folder is added
	//if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINGIT)&&(itemStates & ITEMIS_INGIT)&&(itemStates & ITEMIS_ONLYONE)&&((~itemStates) & ITEMIS_ADDED))
	//	InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYRENAMEMENU, 0, idCmdFirst, ShellMenuDropCopyRename, uFlags);

	// Git add here
	// available if target is versioned and source is either unversioned or from another repository
	if ((itemStatesFolder & ITEMIS_FOLDERINGIT) && (((~itemStates) & ITEMIS_INGIT) || !bSourceAndTargetFromSameRepository) && !moveAvailable)
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYADDMENU, 0, idCmdFirst, ShellMenuDropCopyAdd, uFlags);

	// Git export here
	// available if source is versioned and a folder
	//if ((itemStates & ITEMIS_INGIT)&&(itemStates & ITEMIS_FOLDER))
	//	InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTMENU, 0, idCmdFirst, ShellMenuDropExport, uFlags);

	// Git export all here
	// available if source is versioned and a folder
	//if ((itemStates & ITEMIS_INGIT)&&(itemStates & ITEMIS_FOLDER))
	//	InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTEXTENDEDMENU, 0, idCmdFirst, ShellMenuDropExportExtended, uFlags);

	// apply patch
	// available if source is a patchfile
	if (itemStates & ITEMIS_PATCHFILE)
	{
		if (itemStates & ITEMIS_ONLYONE)
			InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_MENUAPPLYPATCH, 0, idCmdFirst, ShellMenuApplyPatch, uFlags);
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_MENUIMPORTPATCH, 0, idCmdFirst, ShellMenuImportPatchDrop, uFlags);
	}

	if ((itemStates & ITEMIS_ONLYONE) && (itemStates & (ITEMIS_WCROOT | ITEMIS_BAREREPO)) && !(itemStatesFolder & ITEMIS_INVERSIONEDFOLDER))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPNEWWORKTREE, 0, idCmdFirst, ShellMenuDropNewWorktree, uFlags);

	// separator
	if (idCmd != idCmdFirst)
		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);

	TweakMenu(hMenu);

	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, static_cast<USHORT>(idCmd - idCmdFirst)));
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu, UINT indexMenu, UINT idCmdFirst, UINT /*idCmdLast*/, UINT uFlags)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: QueryContextMenu itemStates=%ld\n", itemStates);
	PreserveChdir preserveChdir;

	//first check if our drop handler is called
	//and then (if true) provide the context menu for the
	//drop handler
	if (m_State == FileStateDropHandler)
	{
		return QueryDropContext(uFlags, idCmdFirst, hMenu, indexMenu);
	}

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return S_OK;					//we don't change the default action

	if (files_.empty() && folder_.empty())
		return S_OK;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return S_OK;

	if (IsIllegalFolder(folder_))
		return S_OK;

	if (folder_.empty())
	{
		// folder is empty, but maybe files are selected
		if (files_.empty())
			return S_OK;	// nothing selected - we don't have a menu to show
		// check whether a selected entry is an UID - those are namespace extensions
		// which we can't handle
		if (std::any_of(files_.cbegin(), files_.cend(), [](auto& file) { return CStringUtils::StartsWith(file.c_str(), L"::{"); }))
			return S_OK;
	}
	else
	{
		if (CStringUtils::StartsWith(folder_.c_str(), L"::{"))
			return S_OK;
	}

	if (((uFlags & CMF_EXTENDEDVERBS) == 0) && g_ShellCache.HideMenusForUnversionedItems())
	{
		if ((itemStates & (ITEMIS_INGIT | ITEMIS_INVERSIONEDFOLDER | ITEMIS_FOLDERINGIT | ITEMIS_BAREREPO)) == 0)
			return S_OK;
	}

	//check if our menu is requested for a git admin directory
	if (GitAdminDir::IsAdminDirPath(folder_.c_str()))
		return S_OK;

	if (uFlags & CMF_EXTENDEDVERBS)
		itemStates |= ITEMIS_EXTENDED;

	regDiffLater.read();
	if (!std::wstring(regDiffLater).empty())
		itemStates |= ITEMIS_HASDIFFLATER;

	const BOOL bShortcut = !!(uFlags & CMF_VERBSONLY);
	if ( bShortcut && (files_.size()==1))
	{
		// Don't show the context menu for a link if the
		// destination is not part of a working copy.
		// It would only show the standard menu items
		// which are already shown for the lnk-file.
		CString path = files_.front().c_str();
		if (!GitAdminDir::HasAdminDir(path))
		{
			return S_OK;
		}
	}

	if (hMenu)
	{
		//check if we already added our menu entry for a folder.
		//we check that by iterating through all menu entries and check if
		//the dwItemData member points to our global ID string. That string is set
		//by our shell extension when the folder menu is inserted.
		wchar_t menubuf[MAX_PATH] = { 0 };
		int count = GetMenuItemCount(hMenu);
		for (int i=0; i<count; ++i)
		{
			MENUITEMINFO miif = { 0 };
			miif.cbSize = sizeof(MENUITEMINFO);
			miif.fMask = MIIM_DATA;
			miif.dwTypeData = menubuf;
			miif.cch = _countof(menubuf);
			GetMenuItemInfo(hMenu, i, TRUE, &miif);
			if (miif.dwItemData == reinterpret_cast<ULONG_PTR>(g_MenuIDString))
				return S_OK;
		}
	}

	LoadLangDll();
	UINT idCmd = idCmdFirst;

	//create the sub menu
	HMENU subMenu = hMenu ? CreateMenu() : nullptr;
	int indexSubMenu = 0;

	unsigned __int64 topMenu11 = g_ShellCache.GetMenuLayout11();
	unsigned __int64 topmenu = g_ShellCache.GetMenuLayout();
	unsigned __int64 menumask = g_ShellCache.GetMenuMask();
	unsigned __int64 menuex = g_ShellCache.GetMenuExt();

	int menuIndex = 0;
	bool bAddSeparator = false;
	bool bMenuEntryAdded = false;
	bool bMenuEmpty = true;
	if (hMenu)
	{
		// insert separator at start
		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr); idCmd++;
	}
	bool bShowIcons = !!DWORD(CRegStdDWORD(L"Software\\TortoiseGit\\ShowContextMenuIcons", TRUE, false, HKEY_CURRENT_USER, KEY_WOW64_64KEY));

	while (menuInfo[menuIndex].command != ShellMenuLastEntry)
	{
		if (menuInfo[menuIndex].command == ShellSeparator)
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
			bool bInsertMenu = ShouldInsertItem(menuInfo[menuIndex]);
			if (menuInfo[menuIndex].menuID & menuex)
			{
				if( !(itemStates & ITEMIS_EXTENDED) )
					bInsertMenu = false;
			}

			if (menuInfo[menuIndex].menuID & (~menumask))
			{
				if (bInsertMenu)
				{
					bool bIsTop = ((topmenu & menuInfo[menuIndex].menuID) != 0);
					// insert a separator
					if ((bMenuEntryAdded)&&(bAddSeparator)&&(!bIsTop))
					{
						bAddSeparator = false;
						bMenuEntryAdded = false;
						if (subMenu)
							InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr);
						else
							m_explorerCommands.push_back(CExplorerCommand(L"", 0, 0, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
						idCmd++;
					}

					bool isMenu11 = ((topMenu11 & menuInfo[menuIndex].menuID) != 0);
					// handle special cases (sub menus)
					if ((menuInfo[menuIndex].command == ShellMenuIgnoreSub)||(menuInfo[menuIndex].command == ShellMenuUnIgnoreSub)||(menuInfo[menuIndex].command == ShellMenuDeleteIgnoreSub))
					{
						if (hMenu || isMenu11)
						{
							if (InsertIgnoreSubmenus(idCmd, idCmdFirst, hMenu, subMenu, indexMenu, indexSubMenu, topmenu, bShowIcons, uFlags))
								bMenuEntryAdded = true;
						}
					}
					else if (menuInfo[menuIndex].command == ShellMenuLFSMenu)
					{
						if (InsertLFSSubmenu(idCmd, idCmdFirst, hMenu, subMenu, indexMenu, indexSubMenu, topmenu, bShowIcons, uFlags))
							bMenuEntryAdded = true;
					}
					else
					{
						bIsTop = ((topmenu & menuInfo[menuIndex].menuID) != 0);

						if (hMenu || isMenu11)
						{
							// insert the menu entry
							InsertGitMenu(bIsTop,
										  bIsTop ? hMenu : subMenu,
										  bIsTop ? indexMenu++ : indexSubMenu++,
										  idCmd++,
										  menuInfo[menuIndex].menuTextID,
										  bShowIcons ? menuInfo[menuIndex].iconID : 0,
										  idCmdFirst,
										  menuInfo[menuIndex].command,
										  uFlags);
							if (!bIsTop)
							{
								bMenuEntryAdded = true;
								bMenuEmpty = false;
								bAddSeparator = false;
							}
						}
					}
				}
			}
		}
		menuIndex++;
	}

	// do not show TortoiseGit menu if it's empty
	if (bMenuEmpty)
	{
		if (idCmd - idCmdFirst > 0)
		{
			if (hMenu)
			{
				//separator after
				InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr); idCmd++;
				TweakMenu(hMenu);
			}
		}

		//return number of menu items added
		return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, static_cast<USHORT>(idCmd - idCmdFirst)));
	}

	//add sub menu to main context menu
	//don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
	//see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
	MAKESTRING(IDS_MENUSUBMENU);
	if (!g_ShellCache.HasShellMenuAccelerators())
	{
		// remove the accelerators
		std::wstring temp = stringtablebuffer;
		temp.erase(std::remove(temp.begin(), temp.end(), '&'), temp.end());
		wcscpy_s(stringtablebuffer, temp.c_str());
	}
	MENUITEMINFO menuiteminfo = { 0 };
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	menuiteminfo.fType = MFT_STRING;
	menuiteminfo.dwTypeData = stringtablebuffer;

	UINT uIcon = bShowIcons ? IDI_APP : 0;
	if (!folder_.empty())
	{
		uIcon = bShowIcons ? IDI_MENUFOLDER : 0;
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuFolder;
		myIDMap[idCmd] = ShellSubMenuFolder;
		menuiteminfo.dwItemData = reinterpret_cast<ULONG_PTR>(g_MenuIDString);
	}
	else if (!bShortcut && (files_.size()==1))
	{
		uIcon = bShowIcons ? IDI_MENUFILE : 0;
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuFile;
		myIDMap[idCmd] = ShellSubMenuFile;
	}
	else if (bShortcut && (files_.size()==1))
	{
		uIcon = bShowIcons ? IDI_MENULINK : 0;
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuLink;
		myIDMap[idCmd] = ShellSubMenuLink;
	}
	else if (!files_.empty())
	{
		uIcon = bShowIcons ? IDI_MENUMULTIPLE : 0;
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuMultiple;
		myIDMap[idCmd] = ShellSubMenuMultiple;
	}
	else
	{
		myIDMap[idCmd - idCmdFirst] = ShellSubMenu;
		myIDMap[idCmd] = ShellSubMenu;
	}
	menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING;
	if (uIcon)
	{
		menuiteminfo.fMask |= MIIM_BITMAP;
		menuiteminfo.hbmpItem = m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, uIcon);
	}
	menuiteminfo.hSubMenu = subMenu;
	menuiteminfo.wID = idCmd++;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);

	//separator after
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR | MF_BYPOSITION, 0, nullptr); idCmd++;

	TweakMenu(hMenu);

	//return number of menu items added
	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, static_cast<USHORT>(idCmd - idCmdFirst)));
}

void CShellExt::TweakMenu(HMENU hMenu)
{
	MENUINFO MenuInfo = {};
	MenuInfo.cbSize  = sizeof(MenuInfo);
	MenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
	MenuInfo.dwStyle = MNS_CHECKORBMP;
	SetMenuInfo(hMenu, &MenuInfo);
}

void CShellExt::AddPathCommand(std::wstring& gitCmd, LPCWSTR command, bool bFilesAllowed, const std::vector<std::wstring>& files, const std::wstring folder)
{
	gitCmd += command;
	gitCmd += L" /path:\"";
	if ((bFilesAllowed) && !files.empty())
		gitCmd += files.front();
	else
		gitCmd += folder;
	gitCmd += L'"';
}

void CShellExt::AddPathFileCommand(std::wstring& gitCmd, LPCWSTR command, const std::vector<std::wstring>& files, const std::wstring folder, bool bFoldersOnly = false)
{
	std::wstring tempfile = WriteFileListToTempFile(bFoldersOnly, files, folder);
	gitCmd += command;
	gitCmd += L" /pathfile:\"";
	gitCmd += tempfile;
	gitCmd += L'"';
	gitCmd += L" /deletepathfile";
}

void CShellExt::AddPathFileDropCommand(std::wstring& gitCmd, LPCWSTR command, const std::vector<std::wstring>& files, const std::wstring folder)
{
	std::wstring tempfile = WriteFileListToTempFile(false, files, folder);
	gitCmd += command;
	gitCmd += L" /pathfile:\"";
	gitCmd += tempfile;
	gitCmd += L'"';
	gitCmd += L" /deletepathfile";
	gitCmd += L" /droptarget:\"";
	gitCmd += folder;
	gitCmd += L'"';
}

// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	PreserveChdir preserveChdir;
	HRESULT hr = E_INVALIDARG;
	if (!lpcmi)
		return hr;

	if (!files_.empty() || !folder_.empty())
	{
		UINT_PTR idCmd = LOWORD(lpcmi->lpVerb);

		if (HIWORD(lpcmi->lpVerb))
		{
			auto verb = MultibyteToWide(lpcmi->lpVerb);
			const auto verb_it = myVerbsMap.lower_bound(verb);
			if (verb_it != myVerbsMap.end() && verb_it->first == verb)
				idCmd = verb_it->second;
			else
				return hr;
		}

		// See if we have a handler interface for this id
		std::map<UINT_PTR, UINT_PTR>::const_iterator id_it = myIDMap.lower_bound(idCmd);
		if (id_it != myIDMap.end() && id_it->first == idCmd)
		{
			InvokeCommand(static_cast<int>(id_it->second),
						  static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)),
						  uuidSource,
						  lpcmi->hwnd,
						  itemStates,
						  itemStatesFolder,
						  files_,
						  folder_,
						  regDiffLater,
						  nullptr);
			myIDMap.clear();
			myVerbsIDMap.clear();
			myVerbsMap.clear();

			hr = S_OK;
		} // if (id_it != myIDMap.end() && id_it->first == idCmd)
	} // if (files_.empty() || folder_.empty())
	return hr;
}

void CShellExt::InvokeCommand(int cmd, const std::wstring& appDir, const std::wstring uuidSource, HWND hParent, DWORD itemStates, DWORD itemStatesFolder, const std::vector<std::wstring>& paths, const std::wstring& folder, CRegStdString& regDiffLater, Microsoft::WRL::ComPtr<IUnknown> site)
{
	// TortoiseGitProc expects a command line of the form:
	//"/command:<commandname> /pathfile:<path> [/deletepathfile] ...
	//  or
	//"/command:<commandname> /path:<path> ...
	//
	//* path is a path to a single file/directory for commands which only act on single items (log, sync, ...)
	//* pathfile is a path to a temporary file which contains a list of file paths
	CTraceToOutputDebugString::Instance()(__FUNCTION__);
	std::wstring gitCmd = L" /command:";
	switch (cmd)
	{
		//#region case
	case ShellMenuSync:
	{
		wchar_t syncSeq[12] = { 0 };
		swprintf_s(syncSeq, L"%d", g_syncSeq++);
		AddPathCommand(gitCmd, L"sync", false, paths, folder);
		gitCmd += L" /seq:";
		gitCmd += syncSeq;
	}
	break;
	case ShellMenuSubSync:
		AddPathFileCommand(gitCmd, L"subsync", paths, folder);
		if (itemStatesFolder & ITEMIS_SUBMODULECONTAINER || (itemStates & ITEMIS_SUBMODULECONTAINER && itemStates & ITEMIS_WCROOT && itemStates & ITEMIS_ONLYONE))
		{
			gitCmd += L" /bkpath:\"";
			gitCmd += folder;
			gitCmd += L'"';
		}
		break;
	case ShellMenuUpdateExt:
		AddPathFileCommand(gitCmd, L"subupdate", paths, folder);
		if (itemStatesFolder & ITEMIS_SUBMODULECONTAINER || (itemStates & ITEMIS_SUBMODULECONTAINER && itemStates & ITEMIS_WCROOT && itemStates & ITEMIS_ONLYONE))
		{
			gitCmd += L" /bkpath:\"";
			gitCmd += folder;
			gitCmd += L'"';
		}
		break;
	case ShellMenuCommit:
		AddPathFileCommand(gitCmd, L"commit", paths, folder);
		break;
	case ShellMenuAdd:
		AddPathFileCommand(gitCmd, L"add", paths, folder);
		break;
	case ShellMenuIgnore:
		AddPathFileCommand(gitCmd, L"ignore", paths, folder);
		break;
	case ShellMenuIgnoreCaseSensitive:
		AddPathFileCommand(gitCmd, L"ignore", paths, folder);
		gitCmd += L" /onlymask";
		break;
	case ShellMenuDeleteIgnore:
		AddPathFileCommand(gitCmd, L"ignore", paths, folder);
		gitCmd += L" /delete";
		break;
	case ShellMenuDeleteIgnoreCaseSensitive:
		AddPathFileCommand(gitCmd, L"ignore", paths, folder);
		gitCmd += L" /delete /onlymask";
		break;
	case ShellMenuUnIgnore:
		AddPathFileCommand(gitCmd, L"unignore", paths, folder);
		break;
	case ShellMenuUnIgnoreCaseSensitive:
		AddPathFileCommand(gitCmd, L"unignore", paths, folder);
		gitCmd += L" /onlymask";
		break;
	case ShellMenuMergeAbort:
		AddPathCommand(gitCmd, L"merge", true, paths, folder);
		gitCmd += L" /abort";
		break;
	case ShellMenuRevert:
		AddPathFileCommand(gitCmd, L"revert", paths, folder);
		break;
	case ShellMenuCleanup:
		AddPathFileCommand(gitCmd, L"cleanup", paths, folder);
		break;
	case ShellMenuSendMail:
		AddPathFileCommand(gitCmd, L"sendmail", paths, folder);
		break;
	case ShellMenuResolve:
		AddPathFileCommand(gitCmd, L"resolve", paths, folder);
		break;
	case ShellMenuSwitch:
		AddPathCommand(gitCmd, L"switch", false, paths, folder);
		break;
	case ShellMenuExport:
		AddPathCommand(gitCmd, L"export", false, paths, folder);
		break;
	case ShellMenuAbout:
		gitCmd += L"about";
		break;
	case ShellMenuCreateRepos:
		AddPathCommand(gitCmd, L"repocreate", false, paths, folder);
		break;
	case ShellMenuMerge:
		AddPathCommand(gitCmd, L"merge", false, paths, folder);
		break;
	case ShellMenuCopy:
		AddPathCommand(gitCmd, L"copy", true, paths, folder);
		break;
	case ShellMenuSettings:
		AddPathCommand(gitCmd, L"settings", true, paths, folder);
		break;
	case ShellMenuHelp:
		gitCmd += L"help";
		break;
	case ShellMenuRename:
		AddPathCommand(gitCmd, L"rename", true, paths, folder);
		break;
	case ShellMenuRemove:
		AddPathFileCommand(gitCmd, L"remove", paths, folder);
		if (itemStates & ITEMIS_SUBMODULE)
			gitCmd += L" /submodule";
		break;
	case ShellMenuRemoveKeep:
		AddPathFileCommand(gitCmd, L"remove", paths, folder);
		gitCmd += L" /keep";
		break;
	case ShellMenuDiff:
		gitCmd += L"diff /path:\"";
		if (paths.size() == 1)
			gitCmd += paths.front();
		else if (paths.size() == 2)
		{
			auto I = paths.cbegin();
			gitCmd += *I;
			++I;
			gitCmd += L"\" /path2:\"";
			gitCmd += *I;
		}
		else
			gitCmd += folder;
		gitCmd += L'"';
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
			gitCmd += L" /alternative";
		break;
	case ShellMenuDiffLater:
		if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		{
			gitCmd.clear();
			regDiffLater.removeValue();
		}
		else if (paths.size() == 1)
		{
			if (std::wstring(regDiffLater).empty())
			{
				gitCmd.clear();
				regDiffLater = paths[0];
			}
			else
			{
				AddPathCommand(gitCmd, L"diff", true, paths, folder);
				gitCmd += L" /path2:\"";
				gitCmd += std::wstring(regDiffLater);
				gitCmd += L'"';
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
					gitCmd += L" /alternative";
				regDiffLater.removeValue();
			}
		}
		else
			gitCmd.clear();
		break;
	case ShellMenuPrevDiff:
		AddPathCommand(gitCmd, L"prevdiff", true, paths, folder);
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
			gitCmd += L" /alternative";
		break;
	case ShellMenuDiffTwo:
		AddPathCommand(gitCmd, L"diffcommits", true, paths, folder);
		break;
	case ShellMenuDropCopyAdd:
		AddPathFileDropCommand(gitCmd, L"dropcopyadd", paths, folder);
		break;
	case ShellMenuDropCopy:
		AddPathFileDropCommand(gitCmd, L"dropcopy", paths, folder);
		break;
	case ShellMenuDropCopyRename:
		AddPathFileDropCommand(gitCmd, L"dropcopy", paths, folder);
		gitCmd += L" /rename";
		break;
	case ShellMenuDropMove:
		AddPathFileDropCommand(gitCmd, L"dropmove", paths, folder);
		break;
	case ShellMenuDropMoveRename:
		AddPathFileDropCommand(gitCmd, L"dropmove", paths, folder);
		gitCmd += L" /rename";
		break;
	case ShellMenuDropExport:
		AddPathFileDropCommand(gitCmd, L"dropexport", paths, folder);
		break;
	case ShellMenuDropExportExtended:
		AddPathFileDropCommand(gitCmd, L"dropexport", paths, folder);
		gitCmd += L" /extended";
		break;
	case ShellMenuLog:
	case ShellMenuLogSubmoduleFolder:
		AddPathCommand(gitCmd, L"log", true, paths, folder);
		if (cmd == ShellMenuLogSubmoduleFolder)
			gitCmd += L" /submodule";
		break;
	case ShellMenuDaemon:
		AddPathCommand(gitCmd, L"daemon", true, paths, folder);
		break;
	case ShellMenuRevisionGraph:
		AddPathCommand(gitCmd, L"revisiongraph", true, paths, folder);
		break;
	case ShellMenuConflictEditor:
		AddPathCommand(gitCmd, L"conflicteditor", true, paths, folder);
		if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
			gitCmd += L" /alternative";
		break;
	case ShellMenuGitSVNRebase:
		AddPathCommand(gitCmd, L"svnrebase", false, paths, folder);
		break;
	case ShellMenuGitSVNDCommit:
		AddPathCommand(gitCmd, L"svndcommit", true, paths, folder);
		break;
	case ShellMenuGitSVNDFetch:
		AddPathCommand(gitCmd, L"svnfetch", false, paths, folder);
		break;
	case ShellMenuGitSVNIgnore:
		AddPathCommand(gitCmd, L"svnignore", false, paths, folder);
		break;
	case ShellMenuRebase:
		AddPathCommand(gitCmd, L"rebase", false, paths, folder);
		break;
	case ShellMenuShowChanged:
		if (paths.size() > 1)
			AddPathFileCommand(gitCmd, L"repostatus", paths, folder);
		else
			AddPathCommand(gitCmd, L"repostatus", true, paths, folder);
		break;
	case ShellMenuRepoBrowse:
		AddPathCommand(gitCmd, L"repobrowser", false, paths, folder);
		break;
	case ShellMenuRefBrowse:
		AddPathCommand(gitCmd, L"refbrowse", false, paths, folder);
		break;
	case ShellMenuRefLog:
		AddPathCommand(gitCmd, L"reflog", false, paths, folder);
		break;
	case ShellMenuStashSave:
		AddPathCommand(gitCmd, L"stashsave", true, paths, folder);
		break;
	case ShellMenuStashApply:
		AddPathCommand(gitCmd, L"stashapply", false, paths, folder);
		break;
	case ShellMenuStashPop:
		AddPathCommand(gitCmd, L"stashpop", false, paths, folder);
		break;
	case ShellMenuStashList:
		AddPathCommand(gitCmd, L"reflog", false, paths, folder);
		gitCmd += L" /ref:refs/stash";
		break;
	case ShellMenuBisectStart:
		AddPathCommand(gitCmd, L"bisect", false, paths, folder);
		gitCmd += L" /start";
		break;
	case ShellMenuBisectGood:
		AddPathCommand(gitCmd, L"bisect", false, paths, folder);
		gitCmd += L" /good";
		break;
	case ShellMenuBisectBad:
		AddPathCommand(gitCmd, L"bisect", false, paths, folder);
		gitCmd += L" /bad";
		break;
	case ShellMenuBisectSkip:
		AddPathCommand(gitCmd, L"bisect", false, paths, folder);
		gitCmd += L" /skip";
		break;
	case ShellMenuBisectReset:
		AddPathCommand(gitCmd, L"bisect", false, paths, folder);
		gitCmd += L" /reset";
		break;
	case ShellMenuSubAdd:
		AddPathCommand(gitCmd, L"subadd", false, paths, folder);
		break;
	case ShellMenuBlame:
		AddPathCommand(gitCmd, L"blame", true, paths, folder);
		break;
	case ShellMenuApplyPatch:
	{
		auto localPaths = paths;
		if ((itemStates & ITEMIS_PATCHINCLIPBOARD) && ((~itemStates) & ITEMIS_PATCHFILE))
		{
			// if there's a patch file in the clipboard, we save it
			// to a temporary file and tell TortoiseGitMerge to use that one
			UINT cFormat = RegisterClipboardFormat(L"TGIT_UNIFIEDDIFF");
			CClipboardHelper clipboardHelper;
			if (cFormat && clipboardHelper.Open(nullptr))
			{
				HGLOBAL hglb = GetClipboardData(cFormat);
				auto lpstr = static_cast<LPCSTR>(GlobalLock(hglb));

				DWORD len = GetTortoiseGitTempPath(0, nullptr);
				auto path = std::make_unique<wchar_t[]>(len + 1);
				auto tempF = std::make_unique<wchar_t[]>(len + 100);
				GetTortoiseGitTempPath(len + 1, path.get());
				GetTempFileName(path.get(), TEXT("git"), 0, tempF.get());
				std::wstring sTempFile = std::wstring(tempF.get());

				FILE* outFile;
				size_t patchlen = strlen(lpstr);
				_wfopen_s(&outFile, sTempFile.c_str(), L"wb");
				if (outFile)
				{
					size_t size = fwrite(lpstr, sizeof(char), patchlen, outFile);
					if (size == patchlen)
					{
						itemStates |= ITEMIS_PATCHFILE;
						localPaths.clear();
						localPaths.push_back(sTempFile);
					}
					fclose(outFile);
				}
				GlobalUnlock(hglb);
			}
		}
		if (itemStates & ITEMIS_PATCHFILE)
		{
			gitCmd = L" /diff:\"";
			if (!localPaths.empty())
			{
				gitCmd += localPaths.front();
				if (itemStatesFolder & ITEMIS_FOLDERINGIT)
				{
					gitCmd += L"\" /patchpath:\"";
					gitCmd += folder;
				}
			}
			else
				gitCmd += folder;
			if (itemStates & ITEMIS_INVERSIONEDFOLDER)
				gitCmd += L"\" /wc";
			else
				gitCmd += L'"';
		}
		else
		{
			gitCmd = L" /patchpath:\"";
			if (!localPaths.empty())
				gitCmd += localPaths.front();
			else
				gitCmd += folder;
			gitCmd += L'"';
		}
		RunCommand(appDir + L"TortoiseGitMerge.exe", gitCmd, L"TortoiseGitMerge launch failed", site);
	}
		return;
	case ShellMenuClipPaste:
	{
		std::wstring tempfile;
		if (WriteClipboardPathsToTempFile(tempfile))
		{
			bool bCopy = true;
			UINT cPrefDropFormat = RegisterClipboardFormat(L"Preferred DropEffect");
			if (cPrefDropFormat)
			{
				CClipboardHelper clipboardHelper;
				if (clipboardHelper.Open(hParent))
				{
					HGLOBAL hglb = GetClipboardData(cPrefDropFormat);
					if (hglb)
					{
						auto effect = static_cast<DWORD*>(GlobalLock(hglb));
						if (*effect == DROPEFFECT_MOVE)
							bCopy = false;
						GlobalUnlock(hglb);
					}
				}
			}

			if (bCopy)
				gitCmd += L"pastecopy /pathfile:\"";
			else
				gitCmd += L"pastemove /pathfile:\"";
			gitCmd += tempfile;
			gitCmd += L'"';
			gitCmd += L" /deletepathfile";
			gitCmd += L" /droptarget:\"";
			gitCmd += folder;
			gitCmd += L'"';
		}
		else
			return;
		break;
	}
	case ShellMenuClone:
		AddPathCommand(gitCmd, L"clone", false, paths, folder);
		break;
	case ShellMenuPull:
		AddPathFileCommand(gitCmd, L"pull", paths, folder, true);
		break;
	case ShellMenuPush:
		AddPathFileCommand(gitCmd, L"push", paths, folder, true);
		break;
	case ShellMenuBranch:
		AddPathCommand(gitCmd, L"branch", false, paths, folder);
		break;
	case ShellMenuTag:
		AddPathCommand(gitCmd, L"tag", false, paths, folder);
		break;
	case ShellMenuFormatPatch:
		AddPathCommand(gitCmd, L"formatpatch", false, paths, folder);
		break;
	case ShellMenuImportPatch:
		AddPathFileCommand(gitCmd, L"importpatch", paths, folder);
		break;
	case ShellMenuImportPatchDrop:
		AddPathFileDropCommand(gitCmd, L"importpatch", paths, folder);
		break;
	case ShellMenuFetch:
		AddPathFileCommand(gitCmd, L"fetch", paths, folder, true);
		break;
	case ShellMenuLFSLocks:
		AddPathFileCommand(gitCmd, L"lfslocks", paths, folder);
		break;
	case ShellMenuLFSLock:
		AddPathFileCommand(gitCmd, L"lfslock", paths, folder);
		break;
	case ShellMenuLFSUnlock:
		AddPathFileCommand(gitCmd, L"lfsunlock", paths, folder);
		break;
	case ShellMenuWorktree:
		AddPathCommand(gitCmd, L"worktreelist", false, paths, folder);
		break;
	case ShellMenuDropNewWorktree:
		AddPathFileDropCommand(gitCmd, L"dropnewworktree", paths, folder);
		break;

	default:
		break;
		//#endregion
	} // switch (id_it->second)
	if (!gitCmd.empty())
	{
		gitCmd += L" /hwnd:";
		wchar_t buf[30] = { 0 };
		swprintf_s(buf, L"%p", static_cast<void*>(hParent));
		gitCmd += buf;
		RunCommand(appDir + L"TortoiseGitProc.exe", gitCmd, L"TortoiseGitProc launch failed", site);
	}
}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT FAR * /*reserved*/, LPSTR pszName, UINT cchMax)
{
	PreserveChdir preserveChdir;
	//do we know the id?
	std::map<UINT_PTR, UINT_PTR>::const_iterator id_it = myIDMap.lower_bound(idCmd);
	if (id_it == myIDMap.end() || id_it->first != idCmd)
	{
		return E_INVALIDARG;		//no, we don't
	}

	LoadLangDll();
	HRESULT hr = E_INVALIDARG;

	MAKESTRING(IDS_MENUDESCDEFAULT);
	int menuIndex = 0;
	while (menuInfo[menuIndex].command != ShellMenuLastEntry)
	{
		if (menuInfo[menuIndex].command == static_cast<GitCommands>(id_it->second))
		{
			MAKESTRING(menuInfo[menuIndex].menuDescID);
			break;
		}
		menuIndex++;
	}

	const wchar_t* desc = stringtablebuffer;
	switch(uFlags)
	{
	case GCS_HELPTEXTA:
		{
			std::string help = WideToMultibyte(desc);
			lstrcpynA(pszName, help.c_str(), cchMax - 1);
			hr = S_OK;
			break;
		}
	case GCS_HELPTEXTW:
		{
			std::wstring help = desc;
			lstrcpynW(reinterpret_cast<LPWSTR>(pszName), help.c_str(), cchMax - 1);
			hr = S_OK;
			break;
		}
	case GCS_VERBA:
		{
			const auto verb_id_it = myVerbsIDMap.lower_bound(idCmd);
			if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
			{
				std::string help = WideToMultibyte(verb_id_it->second);
				lstrcpynA(pszName, help.c_str(), cchMax - 1);
				hr = S_OK;
			}
		}
		break;
	case GCS_VERBW:
		{
			const auto verb_id_it = myVerbsIDMap.lower_bound(idCmd);
			if (verb_id_it != myVerbsIDMap.end() && verb_id_it->first == idCmd)
			{
				std::wstring help = verb_id_it->second;
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": verb : %ws\n", help.c_str());
				lstrcpynW(reinterpret_cast<LPWSTR>(pszName), help.c_str(), cchMax - 1);
				hr = S_OK;
			}
		}
		break;
	}
	return hr;
}

STDMETHODIMP CShellExt::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res;
	return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

STDMETHODIMP CShellExt::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	PreserveChdir preserveChdir;

	LRESULT res;
	if (!pResult)
		pResult = &res;
	*pResult = FALSE;

	LoadLangDll();
	switch (uMsg)
	{
	case WM_MEASUREITEM:
		{
			auto lpmis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
			if (!lpmis)
				break;
			lpmis->itemWidth = 16;
			lpmis->itemHeight = 16;
			*pResult = TRUE;
		}
		break;
	case WM_DRAWITEM:
		{
			LPCWSTR resource;
			auto lpdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
			if (!lpdis || lpdis->CtlType != ODT_MENU)
				return S_OK;		//not for a menu
			resource = GetMenuTextFromResource(static_cast<int>(myIDMap[lpdis->itemID]));
			if (!resource)
				return S_OK;
			int iconWidth = GetSystemMetrics(SM_CXSMICON);
			int iconHeight = GetSystemMetrics(SM_CYSMICON);
			CAutoIcon hIcon = LoadIconEx(g_hResInst, resource, iconWidth, iconHeight);
			if (!hIcon)
				return S_OK;
			DrawIconEx(lpdis->hDC,
				lpdis->rcItem.left,
				lpdis->rcItem.top + (lpdis->rcItem.bottom - lpdis->rcItem.top - iconHeight) / 2,
				hIcon, iconWidth, iconHeight,
				0, nullptr, DI_NORMAL);
			*pResult = TRUE;
		}
		break;
	case WM_MENUCHAR:
		{
			wchar_t* szItem;
			if (HIWORD(wParam) != MF_POPUP)
				return S_OK;
			int nChar = LOWORD(wParam);
			if (_istascii(static_cast<wint_t>(nChar)) && _istupper(static_cast<wint_t>(nChar)))
				nChar = tolower(nChar);
			// we have the char the user pressed, now search that char in all our
			// menu items
			std::vector<UINT_PTR> accmenus;
			for (auto It = mySubMenuMap.cbegin(); It != mySubMenuMap.cend(); ++It)
			{
				LPCWSTR resource = GetMenuTextFromResource(static_cast<int>(mySubMenuMap[It->first]));
				if (!resource)
					continue;
				szItem = stringtablebuffer;
				wchar_t* amp = wcschr(szItem, L'&');
				if (!amp)
					continue;
				amp++;
				int ampChar = LOWORD(*amp);
				if (_istascii(static_cast<wint_t>(ampChar)) && _istupper(static_cast<wint_t>(ampChar)))
					ampChar = tolower(ampChar);
				if (ampChar == nChar)
				{
					// yep, we found a menu which has the pressed key
					// as an accelerator. Add that menu to the list to
					// process later.
					accmenus.push_back(It->first);
				}
			}
			if (accmenus.empty())
			{
				// no menu with that accelerator key.
				*pResult = MAKELONG(0, MNC_IGNORE);
				return S_OK;
			}
			if (accmenus.size() == 1)
			{
				// Only one menu with that accelerator key. We're lucky!
				// So just execute that menu entry.
				*pResult = MAKELONG(accmenus[0], MNC_EXECUTE);
				return S_OK;
			}
			if (accmenus.size() > 1)
			{
				// we have more than one menu item with this accelerator key!
				MENUITEMINFO mif;
				mif.cbSize = sizeof(MENUITEMINFO);
				mif.fMask = MIIM_STATE;
				for (auto it = accmenus.cbegin(); it != accmenus.cend(); ++it)
				{
					GetMenuItemInfo(reinterpret_cast<HMENU>(lParam), static_cast<UINT>(*it), TRUE, &mif);
					if (mif.fState == MFS_HILITE)
					{
						// this is the selected item, so select the next one
						++it;
						if (it == accmenus.end())
							*pResult = MAKELONG(accmenus[0], MNC_SELECT);
						else
							*pResult = MAKELONG(*it, MNC_SELECT);
						return S_OK;
					}
				}
				*pResult = MAKELONG(accmenus[0], MNC_SELECT);
			}
		}
		break;
	default:
		return S_OK;
	}

	return S_OK;
}

LPCWSTR CShellExt::GetMenuTextFromResource(int id)
{
	wchar_t textbuf[255] = { 0 };
	LPCWSTR resource = nullptr;
	unsigned __int64 layout = g_ShellCache.GetMenuLayout();
	space = 6;

	int menuIndex = 0;
	while (menuInfo[menuIndex].command != ShellMenuLastEntry)
	{
		if (menuInfo[menuIndex].command == id)
		{
			MAKESTRING(menuInfo[menuIndex].menuTextID);
			resource = MAKEINTRESOURCE(menuInfo[menuIndex].iconID);
			switch (id)
			{
			case ShellSubMenuMultiple:
			case ShellSubMenuLink:
			case ShellSubMenuFolder:
			case ShellSubMenuFile:
			case ShellSubMenu:
				space = 0;
				break;
			default:
				space = (layout & menuInfo[menuIndex].menuID) ? 0 : 6;
				if (layout & menuInfo[menuIndex].menuID)
				{
					wcscpy_s(textbuf, L"Git ");
					wcscat_s(textbuf, stringtablebuffer);
					wcscpy_s(stringtablebuffer, textbuf);
				}
				break;
			}
			return resource;
		}
		menuIndex++;
	}
	return nullptr;
}

bool CShellExt::IsIllegalFolder(const std::wstring& folder)
{
	static const GUID code[] = {
		FOLDERID_RecycleBinFolder,
		FOLDERID_CDBurning,
		FOLDERID_Favorites,
		FOLDERID_CommonStartMenu,
		FOLDERID_NetworkFolder,
		FOLDERID_ConnectionsFolder,
		FOLDERID_ControlPanelFolder,
		FOLDERID_Cookies,
		FOLDERID_Favorites,
		FOLDERID_Fonts,
		FOLDERID_History,
		FOLDERID_InternetFolder,
		FOLDERID_InternetCache,
		FOLDERID_NetHood,
		FOLDERID_NetworkFolder,
		FOLDERID_PrintersFolder,
		FOLDERID_PrintHood,
		FOLDERID_Recent,
		FOLDERID_SendTo,
		FOLDERID_StartMenu,
	};
	for (int i = 0; i < _countof(code); i++)
	{
		CComHeapPtr<WCHAR> pszPath;
		if (SHGetKnownFolderPath(code[i], 0, nullptr, &pszPath) != S_OK)
			continue;
		if (!pszPath[0])
			continue;
		if (wcscmp(pszPath, folder.c_str()) == 0)
			return true;
	}
	return false;
}

bool CShellExt::InsertLFSSubmenu(UINT& idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT& indexMenu, int& indexSubMenu, unsigned __int64 topmenu, bool bShowIcons, UINT uFlags)
{
	static MenuInfo infos[] = {
		{ ShellMenuLFSLocks,	0,	IDI_REPOBROWSE,	IDS_MENULFSLOCKS,	IDS_MENUDESCLFSLOCKS,	{ ITEMIS_FOLDERINGIT | ITEMIS_ONLYONE, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		{ ShellMenuLFSLock,		0,	IDI_LFSLOCK,	IDS_MENULFSLOCK,	IDS_MENUDESCLFSLOCK,	{ ITEMIS_INGIT | ITEMIS_INVERSIONEDFOLDER, ITEMIS_FOLDER }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
		{ ShellMenuLFSUnlock,	0,	IDI_LFSUNLOCK,	IDS_MENULFSUNLOCK,	IDS_MENUDESCLFSUNLOCK,	{ ITEMIS_INGIT | ITEMIS_INVERSIONEDFOLDER, ITEMIS_FOLDER }, { 0, 0 }, { 0, 0 }, { 0, 0 } },
	};

	if (folder_.empty() && files_.empty())
		return false;

	CTGitPath askedpath;
	askedpath.SetFromWin(folder_.empty() ? files_.front().c_str() : folder_.c_str());
	if (!askedpath.HasLFS())
		return false;

	HMENU lfssubmenu = hMenu ? CreateMenu() : nullptr;
	int indexlfssub = 0;
	bool anyMenu = false;

	for (const auto& info : infos)
	{
		if (ShouldInsertItem(info))
		{
			InsertGitMenu(false, lfssubmenu, indexlfssub++, idCmd++, info.menuTextID, bShowIcons ? info.iconID : 0, idCmdFirst, info.command, uFlags);
			anyMenu = true;
		}
	}

	if (!anyMenu)
		return false;

	MENUITEMINFO menuiteminfo = { 0 };
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING | MIIM_BITMAP;
	menuiteminfo.fType = MFT_STRING;
	menuiteminfo.hbmpItem = m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, IDI_LFS);
	menuiteminfo.hSubMenu = lfssubmenu;
	menuiteminfo.wID = idCmd;

	SecureZeroMemory(stringtablebuffer, sizeof(stringtablebuffer));
	GetMenuTextFromResource(ShellMenuLFSMenu);
	menuiteminfo.dwTypeData = stringtablebuffer;
	menuiteminfo.cch = static_cast<UINT>(min(wcslen(menuiteminfo.dwTypeData), static_cast<size_t>(UINT_MAX)));

	if (hMenu)
		InsertMenuItem((topmenu & MENULFS) ? hMenu : subMenu, (topmenu & MENULFS) ? indexMenu++ : indexSubMenu++, TRUE, &menuiteminfo);
	myIDMap[idCmd - idCmdFirst] = ShellMenuLFSMenu;
	myIDMap[idCmd++] = ShellMenuLFSMenu;

	return true;
}

bool CShellExt::InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT &indexMenu, int &indexSubMenu, unsigned __int64 topmenu, bool bShowIcons, UINT /*uFlags*/)
{
	HMENU ignoresubmenu = nullptr;
	int indexignoresub = 0;
	bool bShowIgnoreMenu = false;
	wchar_t maskbuf[MAX_PATH] = {0};		// MAX_PATH is ok, since this only holds a filename
	wchar_t ignorepath[MAX_PATH] = {0};		// MAX_PATH is ok, since this only holds a filename
	std::vector<CExplorerCommand> exCmds;
	if (files_.empty())
		return false;
	UINT icon = bShowIcons ? IDI_IGNORE : 0;

	auto I = files_.cbegin();
	if (wcsrchr(I->c_str(), L'\\'))
		wcscpy_s(ignorepath, wcsrchr(I->c_str(), L'\\') + 1);
	else
		wcscpy_s(ignorepath, I->c_str());
	if ((itemStates & ITEMIS_IGNORED) && (!ignoredprops.empty()))
	{
		// check if the item name is ignored or the mask
		size_t p = 0;
		const size_t pathLength = wcslen(ignorepath);
		while ( (p=ignoredprops.find( ignorepath,p )) != -1 )
		{
			if ((p == 0 || ignoredprops[p - 1] == wchar_t('\n'))
				&& (p + pathLength == ignoredprops.length() || ignoredprops[p + pathLength + 1] == wchar_t('\n')))
			{
				break;
			}
			p++;
		}
		if (p!=-1)
		{
			ignoresubmenu = hMenu ? CreateMenu() : nullptr;
			if (hMenu)
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
			else
				exCmds.push_back(CExplorerCommand(ignorepath, 0, ShellMenuUnIgnore, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
			auto verb = std::wstring(ignorepath);
			myVerbsMap[verb] = idCmd - idCmdFirst;
			myVerbsMap[verb] = idCmd;
			myVerbsIDMap[idCmd - idCmdFirst] = verb;
			myVerbsIDMap[idCmd] = verb;
			myIDMap[idCmd - idCmdFirst] = ShellMenuUnIgnore;
			myIDMap[idCmd++] = ShellMenuUnIgnore;
			bShowIgnoreMenu = true;
		}
		wcscpy_s(maskbuf, L"*");
		if (wcsrchr(ignorepath, L'.'))
		{
			wcscat_s(maskbuf, wcsrchr(ignorepath, L'.'));
			p = ignoredprops.find(maskbuf);
			if ((p!=-1) &&
				((ignoredprops.compare(maskbuf) == 0) || (ignoredprops.find(L'\n', p) == p + wcslen(maskbuf) + 1) || (ignoredprops.rfind(L'\n', p) == p - 1)))
			{
				if (!ignoresubmenu)
					ignoresubmenu = CreateMenu();
				if (hMenu)
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
				else
					exCmds.push_back(CExplorerCommand(maskbuf, 0, ShellMenuUnIgnoreCaseSensitive, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
				auto verb = std::wstring(maskbuf);
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
				if (hMenu)
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				else
					exCmds.push_back(CExplorerCommand(ignorepath, 0, ShellMenuDeleteIgnore, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
				myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnore;
				myIDMap[idCmd++] = ShellMenuDeleteIgnore;

				wcscpy_s(maskbuf, L"*");
				if (!(itemStates & ITEMIS_FOLDER) && wcsrchr(ignorepath, L'.'))
				{
					wcscat_s(maskbuf, wcsrchr(ignorepath, L'.'));
					if (hMenu)
						InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
					else
						exCmds.push_back(CExplorerCommand(maskbuf, 0, ShellMenuDeleteIgnoreCaseSensitive, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
					auto verb = std::wstring(maskbuf);
					myVerbsMap[verb] = idCmd - idCmdFirst;
					myVerbsMap[verb] = idCmd;
					myVerbsIDMap[idCmd - idCmdFirst] = verb;
					myVerbsIDMap[idCmd] = verb;
					myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreCaseSensitive;
					myIDMap[idCmd++] = ShellMenuDeleteIgnoreCaseSensitive;
				}
			}
			else
			{
				if (hMenu)
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				else
					exCmds.push_back(CExplorerCommand(ignorepath, 0, ShellMenuIgnore, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
				myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
				myIDMap[idCmd++] = ShellMenuIgnore;

				wcscpy_s(maskbuf, L"*");
				if (!(itemStates & ITEMIS_FOLDER) && wcsrchr(ignorepath, L'.'))
				{
					wcscat_s(maskbuf, wcsrchr(ignorepath, L'.'));
					if (hMenu)
						InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING, idCmd, maskbuf);
					else
						exCmds.push_back(CExplorerCommand(maskbuf, 0, ShellMenuIgnoreCaseSensitive, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
					auto verb = std::wstring(maskbuf);
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
			// note: as of Windows 7, the shell does not pass more than 16 items from a multiselection
			// in the Initialize() call before the QueryContextMenu() call. Which means even if the user
			// has selected more than 16 files, we won't know about that here.
			// Note: after QueryContextMenu() exits, Initialize() is called again with all selected files.
			if (itemStates & ITEMIS_INGIT)
			{
				if (files_.size() >= 16)
				{
					MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLE2);
					wcscpy_s(ignorepath, stringtablebuffer);
				}
				else
				{
					MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLE);
					swprintf_s(ignorepath, stringtablebuffer, files_.size());
				}
				if (hMenu)
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				else
					exCmds.push_back(CExplorerCommand(ignorepath, 0, ShellMenuDeleteIgnore, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
				auto verb = std::wstring(ignorepath);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnore;
				myIDMap[idCmd++] = ShellMenuDeleteIgnore;

				if (files_.size() >= 16)
				{
					MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK2);
					wcscpy_s(ignorepath, stringtablebuffer);
				}
				else
				{
					MAKESTRING(IDS_MENUDELETEIGNOREMULTIPLEMASK);
					swprintf_s(ignorepath, stringtablebuffer, files_.size());
				}
				if (hMenu)
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				else
					exCmds.push_back(CExplorerCommand(ignorepath, 0, ShellMenuDeleteIgnoreCaseSensitive, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
				verb = std::wstring(ignorepath);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuDeleteIgnoreCaseSensitive;
				myIDMap[idCmd++] = ShellMenuDeleteIgnoreCaseSensitive;
			}
			else
			{
				if (files_.size() >= 16)
				{
					MAKESTRING(IDS_MENUIGNOREMULTIPLE2);
					wcscpy_s(ignorepath, stringtablebuffer);
				}
				else
				{
					MAKESTRING(IDS_MENUIGNOREMULTIPLE);
					swprintf_s(ignorepath, stringtablebuffer, files_.size());
				}
				if (hMenu)
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				else
					exCmds.push_back(CExplorerCommand(ignorepath, 0, ShellMenuIgnore, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
				auto verb = std::wstring(ignorepath);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
				myIDMap[idCmd++] = ShellMenuIgnore;

				if (files_.size() >= 16)
				{
					MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK2);
					wcscpy_s(ignorepath, stringtablebuffer);
				}
				else
				{
					MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK);
					swprintf_s(ignorepath, stringtablebuffer, files_.size());
				}
				if (hMenu)
					InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
				else
					exCmds.push_back(CExplorerCommand(ignorepath, 0, ShellMenuIgnoreCaseSensitive, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
				verb = std::wstring(ignorepath);
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
		MENUITEMINFO menuiteminfo = { 0 };
		menuiteminfo.cbSize = sizeof(menuiteminfo);
		menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING;
		if (icon)
		{
			menuiteminfo.fMask |= MIIM_BITMAP;
			menuiteminfo.hbmpItem = m_iconBitmapUtils.IconToBitmapPARGB32(g_hResInst, icon);
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
		menuiteminfo.cch = static_cast<UINT>(min(wcslen(menuiteminfo.dwTypeData), static_cast<size_t>(UINT_MAX)));

		if (hMenu)
			InsertMenuItem((topmenu & MENUIGNORE) ? hMenu : subMenu, (topmenu & MENUIGNORE) ? indexMenu++ : indexSubMenu++, TRUE, &menuiteminfo);
		else
		{
			m_explorerCommands.push_back(CExplorerCommand(L"", 0, ShellSeparator, static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll)), uuidSource, itemStates, itemStatesFolder, files_, {}, m_site));
			for (const auto& cmd : exCmds)
			{
				m_explorerCommands.push_back(cmd);
				std::wstring prep = stringtablebuffer;
				prep += L": ";
				m_explorerCommands.back().PrependTitleWith(prep);
			}
			// currently, explorer does not support subcommands which their own subcommands. Once it does,
			// use the line below instead of the ones above
			//m_explorerCommands.push_back(CExplorerCommand(stringTableBuffer, icon, ShellMenuUnIgnoreSub, GetAppDirectory(), uuidSource, itemStates, itemStatesFolder, m_files, exCmds));
		}
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
}

void CShellExt::RunCommand(const std::wstring& path, const std::wstring& command, LPCWSTR errorMessage, Microsoft::WRL::ComPtr<IUnknown> site)
{
	if (site)
	{
		Microsoft::WRL::ComPtr<IServiceProvider> serviceProvider;
		if (SUCCEEDED(site.As(&serviceProvider)))
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IServiceProvider\n");
			Microsoft::WRL::ComPtr<IShellBrowser> shellBrowser;
			if (SUCCEEDED(serviceProvider->QueryService(SID_SShellBrowser, IID_IShellBrowser, &shellBrowser)))
			{
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IShellBrowser\n");
				Microsoft::WRL::ComPtr<IShellView> shellView;
				if (SUCCEEDED(shellBrowser->QueryActiveShellView(&shellView)))
				{
					CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IShellView\n");
					Microsoft::WRL::ComPtr<IDispatch> spdispView;
					if (SUCCEEDED(shellView->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARGS(&spdispView))))
					{
						CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IDispatch\n");
						Microsoft::WRL::ComPtr<IShellFolderViewDual> spFolderView;
						if (SUCCEEDED(spdispView.As(&spFolderView)))
						{
							CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IShellFolderViewDual\n");
							Microsoft::WRL::ComPtr<IDispatch> spdispShell;
							if (SUCCEEDED(spFolderView->get_Application(&spdispShell)))
							{
								Microsoft::WRL::ComPtr<IShellDispatch2> spdispShell2;
								if (SUCCEEDED(spdispShell.As(&spdispShell2)))
								{
									// without this, the launched app is not moved to the foreground
									AllowSetForegroundWindow(ASFW_ANY);

									if (SUCCEEDED(spdispShell2->ShellExecute(_bstr_t{ path.c_str() },
																			 _variant_t{ command.c_str() },
																			 _variant_t{ L"" },
																			 _variant_t{ L"open" },
																			 _variant_t{ SW_NORMAL })))
									{
										return;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	if (CCreateProcessHelper::CreateProcessDetached(path.c_str(), command.c_str()))
	{
		// process started - exit
		return;
	}

	MessageBox(nullptr, CFormatMessageWrapper(), errorMessage, MB_OK | MB_ICONERROR);
}

bool CShellExt::ShouldInsertItem(const MenuInfo& item) const
{
	return ShouldEnableMenu(item.first) || ShouldEnableMenu(item.second) ||
		ShouldEnableMenu(item.third) || ShouldEnableMenu(item.fourth);
}

bool CShellExt::ShouldEnableMenu(const YesNoPair& pair) const
{
	if (pair.yes && pair.no)
	{
		if (((pair.yes & itemStates) == pair.yes) && ((pair.no & (~itemStates)) == pair.no))
			return true;
	}
	else if ((pair.yes) && ((pair.yes & itemStates) == pair.yes))
		return true;
	else if ((pair.no) && ((pair.no & (~itemStates)) == pair.no))
		return true;
	return false;
}

// IExplorerCommand
HRESULT __stdcall CShellExt::GetTitle(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszName)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetTitle\n");
	SHStrDupW(L"TortoiseGit", ppszName);
	return S_OK;
}

HRESULT __stdcall CShellExt::GetIcon(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszIcon)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetIcon\n");
	std::wstring iconPath = static_cast<LPCWSTR>(CPathUtils::GetAppDirectory(g_hmodThisDll));
	iconPath += L"TortoiseGitProc.exe,-";
	iconPath += std::to_wstring(IDI_APP);
	SHStrDupW(iconPath.c_str(), ppszIcon);
	return S_OK;
}

HRESULT __stdcall CShellExt::GetToolTip(IShellItemArray* /*psiItemArray*/, LPWSTR* ppszInfotip)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetToolTip\n");
	*ppszInfotip = nullptr;
	return E_NOTIMPL;
}

HRESULT __stdcall CShellExt::GetCanonicalName(GUID* pguidCommandName)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetCanonicalName\n");
	*pguidCommandName = GUID_NULL;
	return S_OK;
}

HRESULT __stdcall CShellExt::GetState(IShellItemArray* psiItemArray, BOOL fOkToBeSlow, EXPCMDSTATE* pCmdState)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetState\n");
	*pCmdState = ECS_ENABLED;
	Microsoft::WRL::ComPtr<IShellItemArray> ownItemArray;
	if (m_site)
	{
		Microsoft::WRL::ComPtr<IOleWindow> oleWindow;
		m_site.As(&oleWindow);
		if (oleWindow)
		{
			// We don't want to show the menu on the classic context menu.
			// The classic menu provides an IOleWindow, but the main context
			// menu in Win11 does not, except on the left tree view.
			// So we check the window class name: if it's "NamespaceTreeControl",
			// then we're dealing with the main context menu of the tree view.
			// If it's not, then we're dealing with the classic context menu
			// and there we hide this menu entry.
			HWND hWnd = nullptr;
			oleWindow->GetWindow(&hWnd);
			wchar_t szWndClassName[MAX_PATH] = { 0 };
			GetClassName(hWnd, szWndClassName, _countof(szWndClassName));
			if (wcscmp(szWndClassName, L"NamespaceTreeControl"))
			{
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetState - hidden\n");
				*pCmdState = ECS_HIDDEN;
				return S_OK;
			}
			else
			{
				// tree view
				if (!psiItemArray)
				{
					typedef DPI_AWARENESS_CONTEXT(WINAPI * SetThreadDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);
					SetThreadDpiAwarenessContextProc SetThreadDpiAwarenessContext = reinterpret_cast<SetThreadDpiAwarenessContextProc>(GetProcAddress(GetModuleHandle(L"user32"), "SetThreadDpiAwarenessContext"));
					DPI_AWARENESS_CONTEXT context = DPI_AWARENESS_CONTEXT_UNAWARE;
					// the shell disables dpi awareness for extensions, so enable them explicitly
					if (SetThreadDpiAwarenessContext)
						context = SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
					Microsoft::WRL::ComPtr<INameSpaceTreeControl> nameSpaceTreeCtrl;
					oleWindow.As(&nameSpaceTreeCtrl);
					if (nameSpaceTreeCtrl)
					{
						// we do a hit test on the tree view to get the right-clicked item.
						// however this only works if the menu shows up due to a right-click.
						// if the menu is shown because of a key press (right windows key),
						// then this will get the wrong item if the mouse pointer is somewhere
						// over the tree view while the key is clicked!
						// if the mouse pointer is NOT over the tree view when the menu is brought up
						// via keyboard, then this works fine.
						auto msgPos = GetMessagePos();
						POINT msgPoint{};
						msgPoint.x = GET_X_LPARAM(msgPos);
						msgPoint.y = GET_Y_LPARAM(msgPos);
						POINT pt = msgPoint;
						ScreenToClient(hWnd, &pt);
						Microsoft::WRL::ComPtr<IShellItem> shellItem;

						if (!psiItemArray)
						{
							nameSpaceTreeCtrl->HitTest(&pt, &shellItem);
							if (shellItem)
								SHCreateShellItemArrayFromShellItem(shellItem.Get(), IID_IShellItemArray, &ownItemArray);
							else
								nameSpaceTreeCtrl->GetSelectedItems(&ownItemArray);
						}
					}
					if (SetThreadDpiAwarenessContext)
						SetThreadDpiAwarenessContext(context);
					if (!ownItemArray)
					{
						CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetState - hidden\n");
						*pCmdState = ECS_HIDDEN;
						return S_OK;
					}
					else
						psiItemArray = ownItemArray.Get();
				}
			}
		}
	}

	if (!fOkToBeSlow)
		return E_PENDING;

	Initialize(nullptr, nullptr, nullptr);
	Microsoft::WRL::ComPtr<IShellItemArray> itemArray;
	if (!psiItemArray)
	{
		// context menu for a folder background (no selection),
		// so try to get the current path of the explorer window instead
		auto path = ExplorerViewPath(m_site);
		if (path.empty())
		{
			*pCmdState = ECS_HIDDEN;
			return S_OK;
		}
		PIDLIST_ABSOLUTE pidl{};
		if (SUCCEEDED(SHParseDisplayName(path.c_str(), nullptr, &pidl, 0, nullptr)))
		{
			if (SUCCEEDED(SHCreateShellItemArrayFromIDLists(1, const_cast<LPCITEMIDLIST*>(&pidl), itemArray.GetAddressOf())))
			{
				if (itemArray)
				{
					psiItemArray = itemArray.Get();
				}
			}
		}
	}
	if (psiItemArray)
	{
		IDataObject* pDataObj = nullptr;
		if (SUCCEEDED(psiItemArray->BindToHandler(nullptr, BHID_DataObject, IID_IDataObject, reinterpret_cast<void**>(&pDataObj))))
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: Initialize from GetState()\n");

			Initialize(nullptr, pDataObj, nullptr);
			pDataObj->Release();
			pDataObj = nullptr;
		}
		else
			*pCmdState = ECS_HIDDEN;
	}

	if (g_ShellCache.HideMenusForUnversionedItems() && (GetKeyState(VK_SHIFT) & 0x8000) == 0)
	{
		if ((itemStates & (ITEMIS_INGIT | ITEMIS_INVERSIONEDFOLDER | ITEMIS_FOLDERINGIT)) == 0)
			*pCmdState = ECS_HIDDEN;
	}
	if (*pCmdState != ECS_HIDDEN)
	{
		m_explorerCommands.clear();
		QueryContextMenu(nullptr, 0, 0, 0, CMF_EXTENDEDVERBS | CMF_NORMAL);
		if (m_explorerCommands.empty())
			*pCmdState = ECS_HIDDEN;
	}

	return S_OK;
}

HRESULT __stdcall CShellExt::Invoke(IShellItemArray* /*psiItemArray*/, IBindCtx* /*pbc*/)
{
	return E_NOTIMPL;
}

HRESULT __stdcall CShellExt::GetFlags(EXPCMDFLAGS* pFlags)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: GetFlags\n");
	*pFlags = ECF_HASSUBCOMMANDS;
	return S_OK;
}

HRESULT __stdcall CShellExt::EnumSubCommands(IEnumExplorerCommand** ppEnum)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Shell :: EnumSubCommands\n");
	m_explorerCommands.clear();
	QueryContextMenu(nullptr, 0, 0, 0, CMF_EXTENDEDVERBS | CMF_NORMAL);
	*ppEnum = new CExplorerCommandEnum(m_explorerCommands);
	(*ppEnum)->AddRef();
	return S_OK;
}

std::wstring CShellExt::ExplorerViewPath(const Microsoft::WRL::ComPtr<IUnknown>& site)
{
	CTraceToOutputDebugString::Instance()(__FUNCTION__ "\n");
	std::wstring path;
	if (site)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got site\n");
		Microsoft::WRL::ComPtr<IServiceProvider> serviceProvider;
		if (SUCCEEDED(site.As(&serviceProvider)))
		{
			CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IServiceProvider\n");
			Microsoft::WRL::ComPtr<IShellBrowser> shellBrowser;
			if (SUCCEEDED(serviceProvider->QueryService(SID_SShellBrowser, IID_IShellBrowser, &shellBrowser)))
			{
				CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IShellBrowser\n");
				Microsoft::WRL::ComPtr<IShellView> shellView;
				if (SUCCEEDED(shellBrowser->QueryActiveShellView(&shellView)))
				{
					CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IShellView\n");
					Microsoft::WRL::ComPtr<IFolderView> folderView;
					if (SUCCEEDED(shellView.As(&folderView)))
					{
						CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IFolderView\n");
						Microsoft::WRL::ComPtr<IPersistFolder2> persistFolder;
						if (SUCCEEDED(folderView->GetFolder(IID_IPersistFolder2, (LPVOID*)&persistFolder)))
						{
							CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got IPersistFolder2\n");
							PIDLIST_ABSOLUTE curFolder;
							if (SUCCEEDED(persistFolder->GetCurFolder(&curFolder)))
							{
								CTraceToOutputDebugString::Instance()(__FUNCTION__ ": got GetCurFolder\n");
								wchar_t buf[MAX_PATH] = { 0 };
								// find the path of the folder
								if (SHGetPathFromIDList(curFolder, buf))
								{
									CTraceToOutputDebugString::Instance()(__FUNCTION__ L": got SHGetPathFromIDList : %s\n", buf);
									path = buf;
								}
								CoTaskMemFree(curFolder);
							}
						}
					}
				}
			}
		}
	}

	return path;
}
