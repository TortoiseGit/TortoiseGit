// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseGit

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
//#include "GitProperties.h"
#include "GitStatus.h"
#include "TGitPath.h"

#define GetPIDLFolder(pida) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define GetPIDLItem(pida, i) (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

int g_shellidlist=RegisterClipboardFormat(CFSTR_SHELLIDLIST);

CShellExt::MenuInfo CShellExt::menuInfo[] =
{	
	{ ShellMenuClone,						MENUCLONE,			IDI_CLONE,				IDS_MENUCLONE,			IDS_MENUDESCCHECKOUT,
	ITEMIS_FOLDER, ITEMIS_INSVN|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuPull,						MENUPULL,			IDI_PULL,				IDS_MENUPULL,			IDS_MENUPULL,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuFetch,						MENUFETCH,			IDI_PULL,				IDS_MENUFETCH,			IDS_MENUFETCH,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuPush,						MENUPUSH,			IDI_PUSH,				IDS_MENUPUSH,			IDS_MENUPULL,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

//	{ ShellMenuCheckout,					MENUCHECKOUT,		IDI_CHECKOUT,			IDS_MENUCHECKOUT,			IDS_MENUDESCCHECKOUT,
//	ITEMIS_FOLDER, ITEMIS_INSVN|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0, 0 },

//	{ ShellMenuUpdate,						MENUUPDATE,			IDI_UPDATE,				IDS_MENUUPDATE,				IDS_MENUDESCUPDATE,				
//	ITEMIS_INSVN,	ITEMIS_ADDED, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuCommit,						MENUCOMMIT,			IDI_COMMIT,				IDS_MENUCOMMIT,				IDS_MENUDESCCOMMIT,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuDiff,						MENUDIFF,			IDI_DIFF,				IDS_MENUDIFF,				IDS_MENUDESCDIFF,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_NORMAL, ITEMIS_TWO, 0, 0, 0, 0, 0 },

	{ ShellMenuPrevDiff,					MENUPREVDIFF,			IDI_DIFF,				IDS_MENUPREVDIFF,			IDS_MENUDESCPREVDIFF,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0 },

//	{ ShellMenuUrlDiff,						MENUURLDIFF,		IDI_DIFF,				IDS_MENUURLDIFF,			IDS_MENUDESCURLDIFF,
//	ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, 0, ITEMIS_FOLDERINSVN|ITEMIS_EXTENDED|ITEMIS_ONLYONE, 0, 0, 0, 0, 0 },

	{ ShellMenuLog,							MENULOG,			IDI_LOG,				IDS_MENULOG,				IDS_MENUDESCLOG,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, 0, 0 },

//	{ ShellMenuRepoBrowse,					MENUREPOBROWSE,		IDI_REPOBROWSE,			IDS_MENUREPOBROWSE,			IDS_MENUDESCREPOBROWSE,
//	ITEMIS_ONLYONE, 0, ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, 0, 0, 0, 0 },

	{ ShellMenuShowChanged,					MENUSHOWCHANGED,	IDI_SHOWCHANGED,		IDS_MENUSHOWCHANGED,		IDS_MENUDESCSHOWCHANGED,
	ITEMIS_INSVN|ITEMIS_ONLYONE, 0, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, 0, 0, 0, 0},

	{ ShellMenuRebase,					    MENUREBASE,			IDI_REBASE,				IDS_MENUREBASE,				IDS_MENUREBASE,
	ITEMIS_INSVN|ITEMIS_ONLYONE, 0, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, 0, 0, 0, 0},

//	{ ShellMenuRevisionGraph,				MENUREVISIONGRAPH,	IDI_REVISIONGRAPH,		IDS_MENUREVISIONGRAPH,		IDS_MENUDESCREVISIONGRAPH,
//	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, 0, 0, 0, 0},

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuConflictEditor,				MENUCONFLICTEDITOR,	IDI_CONFLICT,			IDS_MENUCONFLICT,			IDS_MENUDESCCONFLICT,
	ITEMIS_INSVN|ITEMIS_CONFLICTED, ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuResolve,						MENURESOLVE,		IDI_RESOLVE,			IDS_MENURESOLVE,			IDS_MENUDESCRESOLVE,
	ITEMIS_INSVN|ITEMIS_CONFLICTED, 0, ITEMIS_INSVN|ITEMIS_FOLDER, 0, ITEMIS_FOLDERINSVN, 0, 0, 0 },

//	{ ShellMenuUpdateExt,					MENUUPDATEEXT,		IDI_UPDATE,				IDS_MENUUPDATEEXT,			IDS_MENUDESCUPDATEEXT,
//	ITEMIS_INSVN, ITEMIS_ADDED, ITEMIS_FOLDERINSVN, ITEMIS_ADDED, 0, 0, 0, 0 },

	{ ShellMenuRename,						MENURENAME,			IDI_RENAME,				IDS_MENURENAME,				IDS_MENUDESCRENAME,
	ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_INVERSIONEDFOLDER, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRemove,						MENUREMOVE,			IDI_DELETE,				IDS_MENUREMOVE,				IDS_MENUDESCREMOVE,
	ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER, ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRemoveKeep,					MENUREMOVE,			IDI_DELETE,				IDS_MENUREMOVEKEEP,			IDS_MENUDESCREMOVEKEEP,
	ITEMIS_INSVN|ITEMIS_INVERSIONEDFOLDER|ITEMIS_EXTENDED, ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuRevert,						MENUREVERT,			IDI_REVERT,				IDS_MENUREVERT,				IDS_MENUDESCREVERT,
	ITEMIS_INSVN, ITEMIS_NORMAL|ITEMIS_ADDED, ITEMIS_FOLDERINSVN, ITEMIS_ADDED, 0, 0, 0, 0 },

	{ ShellMenuRevert,						MENUREVERT,			IDI_REVERT,				IDS_MENUUNDOADD,			IDS_MENUDESCUNDOADD,
	ITEMIS_ADDED, ITEMIS_NORMAL, ITEMIS_FOLDERINSVN|ITEMIS_ADDED, 0, 0, 0, 0, 0 },

	{ ShellMenuDelUnversioned,				MENUDELUNVERSIONED,	IDI_DELUNVERSIONED,		IDS_MENUDELUNVERSIONED,		IDS_MENUDESCDELUNVERSIONED,
	ITEMIS_FOLDER|ITEMIS_INSVN|ITEMIS_EXTENDED, 0, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_EXTENDED, 0, 0, 0, 0, 0 },

	{ ShellMenuCleanup,						MENUCLEANUP,		IDI_CLEANUP,			IDS_MENUCLEANUP,			IDS_MENUDESCCLEANUP,
	ITEMIS_INSVN|ITEMIS_FOLDER, 0, ITEMIS_FOLDERINSVN|ITEMIS_FOLDER, 0, 0, 0, 0, 0 },

//	{ ShellMenuLock,						MENULOCK,			IDI_LOCK,				IDS_MENU_LOCK,				IDS_MENUDESC_LOCK,
//	ITEMIS_INSVN, ITEMIS_LOCKED|ITEMIS_ADDED, ITEMIS_FOLDERINSVN, ITEMIS_LOCKED|ITEMIS_ADDED, 0, 0, 0, 0 },

//	{ ShellMenuUnlock,						MENUUNLOCK,			IDI_UNLOCK,				IDS_MENU_UNLOCK,			IDS_MENUDESC_UNLOCK,
//	ITEMIS_INSVN|ITEMIS_LOCKED, 0, ITEMIS_FOLDER|ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0 },

//	{ ShellMenuUnlockForce,					MENUUNLOCK,			IDI_UNLOCK,				IDS_MENU_UNLOCKFORCE,		IDS_MENUDESC_UNLOCKFORCE,
//	ITEMIS_INSVN|ITEMIS_LOCKED, 0, ITEMIS_FOLDER|ITEMIS_INSVN|ITEMIS_EXTENDED, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

//	{ ShellMenuCopy,						MENUCOPY,			IDI_COPY,				IDS_MENUBRANCH,				IDS_MENUDESCCOPY,
//	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, 0, 0, 0, 0 },

	{ ShellMenuSwitch,						MENUSWITCH,			IDI_SWITCH,				IDS_MENUSWITCH,				IDS_MENUDESCSWITCH,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, 0, 0, 0, 0 },

	{ ShellMenuMerge,						MENUMERGE,			IDI_MERGE,				IDS_MENUMERGE,				IDS_MENUDESCMERGE,
	ITEMIS_INSVN|ITEMIS_ONLYONE, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, 0, 0, 0, 0 },
	{ ShellMenuMergeAll,					MENUMERGEALL,		IDI_MERGE,				IDS_MENUMERGEALL,			IDS_MENUDESCMERGEALL,
	ITEMIS_INSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, ITEMIS_ADDED, ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE|ITEMIS_EXTENDED, 0, 0, 0, 0, 0 },

	{ ShellMenuBranch,						MENUCOPY,			IDI_COPY,				IDS_MENUBRANCH,				IDS_MENUDESCCOPY,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },
	{ ShellMenuTag,							MENUTAG,			IDI_TAG,				IDS_MENUTAG,				IDS_MENUDESCCOPY,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuExport,						MENUEXPORT,			IDI_EXPORT,				IDS_MENUEXPORT,				IDS_MENUDESCEXPORT,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

//	{ ShellMenuRelocate,					MENURELOCATE,		IDI_RELOCATE,			IDS_MENURELOCATE,			IDS_MENUDESCRELOCATE,
//	ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, ITEMIS_FOLDERINSVN|ITEMIS_ONLYONE, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuCreateRepos,					MENUCREATEREPOS,	IDI_CREATEREPOS,		IDS_MENUCREATEREPOS,		IDS_MENUDESCCREATEREPOS,
	ITEMIS_FOLDER, ITEMIS_INSVN|ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuAdd,							MENUADD,			IDI_ADD,				IDS_MENUADD,				IDS_MENUDESCADD,
	ITEMIS_INVERSIONEDFOLDER, ITEMIS_INSVN, ITEMIS_INSVN|ITEMIS_FOLDER, 0, ITEMIS_IGNORED, 0, ITEMIS_DELETED, ITEMIS_FOLDER|ITEMIS_ONLYONE },

//	{ ShellMenuAddAsReplacement,			MENUADD,			IDI_ADD,				IDS_MENUADDASREPLACEMENT,	IDS_MENUADDASREPLACEMENT,
//	ITEMIS_DELETED|ITEMIS_ONLYONE, ITEMIS_FOLDER, 0, 0, 0, 0, 0, 0 },

//	{ ShellMenuImport,						MENUIMPORT,			IDI_IMPORT,				IDS_MENUIMPORT,				IDS_MENUDESCIMPORT,
//	ITEMIS_FOLDER, ITEMIS_INSVN, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuBlame,						MENUBLAME,			IDI_BLAME,				IDS_MENUBLAME,				IDS_MENUDESCBLAME,
	ITEMIS_NORMAL|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_ADDED, 0, 0, 0, 0, 0, 0 },
	// TODO: original code is ITEMIS_INSVN|ITEMIS_ONLYONE, makes sense to only allow blaming of versioned files
	//       why was this changed, is this related to GitStatus?

	{ ShellMenuIgnoreSub,					MENUIGNORE,			IDI_IGNORE,				IDS_MENUIGNORE,				IDS_MENUDESCIGNORE,
	ITEMIS_INVERSIONEDFOLDER,C, 0, 0, 0, 0, 0, 0 },

	{ ShellMenuUnIgnoreSub,					MENUIGNORE,			IDI_IGNORE,				IDS_MENUUNIGNORE,			IDS_MENUDESCUNIGNORE,
	ITEMIS_IGNORED, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

//	{ ShellMenuCherryPick,					MENUCHERRYPICK,		IDI_CREATEPATCH,		IDS_MENUCHERRYPICK,		IDS_MENUDESCCREATEPATCH,
//	ITEMIS_INSVN, ITEMIS_NORMAL, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuFormatPatch,					MENUFORMATPATCH,	IDI_CREATEPATCH,		IDS_MENUFORMATPATCH,		IDS_MENUDESCCREATEPATCH,
	ITEMIS_INSVN, ITEMIS_NORMAL, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuImportPatch,					MENUIMPORTPATCH,	IDI_PATCH,				IDS_MENUIMPORTPATCH,		IDS_MENUDESCCREATEPATCH,
	ITEMIS_INSVN, ITEMIS_NORMAL, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },


	{ ShellMenuCreatePatch,					MENUCREATEPATCH,	IDI_CREATEPATCH,		IDS_MENUCREATEPATCH,		IDS_MENUDESCCREATEPATCH,
	ITEMIS_INSVN, ITEMIS_NORMAL, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellMenuApplyPatch,					MENUAPPLYPATCH,		IDI_PATCH,				IDS_MENUAPPLYPATCH,			IDS_MENUDESCAPPLYPATCH,
	ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_FOLDERINSVN, ITEMIS_ADDED, ITEMIS_ONLYONE|ITEMIS_PATCHFILE, 0, ITEMIS_FOLDERINSVN, ITEMIS_ADDED, 0, 0 },

	{ ShellMenuProperties,					MENUPROPERTIES,		IDI_PROPERTIES,			IDS_MENUPROPERTIES,			IDS_MENUDESCPROPERTIES,
	ITEMIS_INSVN, 0, ITEMIS_FOLDERINSVN, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},
//	{ ShellMenuClipPaste,					MENUCLIPPASTE,		IDI_CLIPPASTE,			IDS_MENUCLIPPASTE,			IDS_MENUDESCCLIPPASTE,
//	ITEMIS_INSVN|ITEMIS_FOLDER|ITEMIS_PATHINCLIPBOARD, 0, 0, 0, 0, 0, 0, 0 },

	{ ShellSeparator, 0, 0, 0, 0, 0, 0, 0, 0},

	{ ShellMenuSettings,					MENUSETTINGS,		IDI_SETTINGS,			IDS_MENUSETTINGS,			IDS_MENUDESCSETTINGS,
	ITEMIS_FOLDER, 0, 0, ITEMIS_FOLDER, 0, 0, 0, 0 },
	{ ShellMenuHelp,						MENUHELP,			IDI_HELP,				IDS_MENUHELP,				IDS_MENUDESCHELP,
	ITEMIS_FOLDER, 0, 0, ITEMIS_FOLDER, 0, 0, 0, 0 },
	{ ShellMenuAbout,						MENUABOUT,			IDI_ABOUT,				IDS_MENUABOUT,				IDS_MENUDESCABOUT,
	ITEMIS_FOLDER, 0, 0, ITEMIS_FOLDER, 0, 0, 0, 0 },

	// the sub menus - they're not added like the the commands, therefore the menu ID is zero
	// but they still need to be in here, because we use the icon and string information anyway.
	{ ShellSubMenu,							NULL,				IDI_APP,				IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuFile,						NULL,				IDI_MENUFILE,			IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuFolder,					NULL,				IDI_MENUFOLDER,			IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuLink,						NULL,				IDI_MENULINK,			IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	{ ShellSubMenuMultiple,					NULL,				IDI_MENUMULTIPLE,		IDS_MENUSUBMENU,			0,
	0, 0, 0, 0, 0, 0, 0, 0 },
	// mark the last entry to tell the loop where to stop iterating over this array
	{ ShellMenuLastEntry,					0,					0,						0,							0,
	0, 0, 0, 0, 0, 0, 0, 0 },
};


STDMETHODIMP CShellExt::Initialize(LPCITEMIDLIST pIDFolder,
                                   LPDATAOBJECT pDataObj,
                                   HKEY /* hRegKey */)
{

	ATLTRACE("Shell :: Initialize\n");
	PreserveChdir preserveChdir;
	files_.clear();
	folder_.erase();
	uuidSource.erase();
	uuidTarget.erase();
	itemStates = 0;
	itemStatesFolder = 0;
	stdstring statuspath;
	git_wc_status_kind fetchedstatus = git_wc_status_none;
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
			if (m_State == FileStateDropHandler)
			{

				FORMATETC etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
				STGMEDIUM stg = { TYMED_HGLOBAL };
				if ( FAILED( pDataObj->GetData ( &etc, &stg )))
				{
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}


				HDROP drop = (HDROP)GlobalLock(stg.hGlobal);
				if ( NULL == drop )
				{
					ReleaseStgMedium ( &stg );
					ReleaseStgMedium ( &medium );
					return E_INVALIDARG;
				}

				int count = DragQueryFile(drop, (UINT)-1, NULL, 0);
				if (count == 1)
					itemStates |= ITEMIS_ONLYONE;
				for (int i = 0; i < count; i++)
				{
					// find the path length in chars
					UINT len = DragQueryFile(drop, i, NULL, 0);
					if (len == 0)
						continue;
					TCHAR * szFileName = new TCHAR[len+1];
					if (0 == DragQueryFile(drop, i, szFileName, len+1))
					{
						delete [] szFileName;
						continue;
					}
					stdstring str = stdstring(szFileName);
					delete [] szFileName;
					if ((str.empty() == false)&&(g_ShellCache.IsContextPathAllowed(szFileName)))
					{
						if (itemStates & ITEMIS_ONLYONE)
						{
							CTGitPath strpath;
							strpath.SetFromWin(str.c_str());
							itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".diff"))==0) ? ITEMIS_PATCHFILE : 0;
							itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".patch"))==0) ? ITEMIS_PATCHFILE : 0;
						}
						files_.push_back(str);
						if (i == 0)
						{
							//get the Subversion status of the item
							git_wc_status_kind status = git_wc_status_none;
							CTGitPath askedpath;
							askedpath.SetFromWin(str.c_str());
							try
							{
								GitStatus stat;
								stat.GetStatus(CTGitPath(str.c_str()), false, true, true);
								if (stat.status)
								{
									statuspath = str;
									status = GitStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
									fetchedstatus = status;
									//if ((stat.status->entry)&&(stat.status->entry->lock_token))
									//	itemStates |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
									if ( askedpath.IsDirectory() )//if ((stat.status->entry)&&(stat.status->entry->kind == git_node_dir))
									{
										itemStates |= ITEMIS_FOLDER;
										if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
											itemStates |= ITEMIS_FOLDERINSVN;
									}
									//if ((stat.status->entry)&&(stat.status->entry->present_props))
									//{
									//	if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
									//		itemStates |= ITEMIS_NEEDSLOCK;
									//}
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
							catch ( ... )
							{
								ATLTRACE2(_T("Exception in GitStatus::GetStatus()\n"));
							}

							// TODO: should we really assume any sub-directory to be versioned
							//       or only if it contains versioned files
							if ( askedpath.IsDirectory() )
							{
								if (askedpath.HasAdminDir())
									itemStates |= ITEMIS_INSVN;
							}
							if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
								itemStates |= ITEMIS_INSVN;
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
				CIDA* cida = (CIDA*)GlobalLock(medium.hGlobal);
				ItemIDList parent( GetPIDLFolder (cida));

				int count = cida->cidl;
				BOOL statfetched = FALSE;
				for (int i = 0; i < count; ++i)
				{
					ItemIDList child (GetPIDLItem (cida, i), &parent);
					stdstring str = child.toString();
					if ((str.empty() == false)&&(g_ShellCache.IsContextPathAllowed(str.c_str())))
					{
						//check if our menu is requested for a subversion admin directory
						if (g_GitAdminDir.IsAdminDirPath(str.c_str()))
							continue;

						files_.push_back(str);
						CTGitPath strpath;
						strpath.SetFromWin(str.c_str());
						itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".diff"))==0) ? ITEMIS_PATCHFILE : 0;
						itemStates |= (strpath.GetFileExtension().CompareNoCase(_T(".patch"))==0) ? ITEMIS_PATCHFILE : 0;
						if (!statfetched)
						{
							//get the Subversion status of the item
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
									GitStatus stat;
									if (strpath.HasAdminDir())
										stat.GetStatus(strpath, false, true, true);
									statuspath = str;
									if (stat.status)
									{
										status = GitStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
										fetchedstatus = status;
										//if ((stat.status->entry)&&(stat.status->entry->lock_token))
										//	itemStates |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
										if ( strpath.IsDirectory() )//if ((stat.status->entry)&&(stat.status->entry->kind == git_node_dir))
										{
											itemStates |= ITEMIS_FOLDER;
											if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
												itemStates |= ITEMIS_FOLDERINSVN;
										}
										// TODO: do we need to check that it's not a dir? does conflict options makes sense for dir in git?
										if (status == git_wc_status_conflicted)//if ((stat.status->entry)&&(stat.status->entry->conflict_wrk))
											itemStates |= ITEMIS_CONFLICTED;
										//if ((stat.status->entry)&&(stat.status->entry->present_props))
										//{
										//	if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
										//		itemStates |= ITEMIS_NEEDSLOCK;
										//}
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
									statfetched = TRUE;
								}
								catch ( ... )
								{
									ATLTRACE2(_T("Exception in GitStatus::GetStatus()\n"));
								}
							}

							// TODO: should we really assume any sub-directory to be versioned
							//       or only if it contains versioned files
							if ( strpath.IsDirectory() )
							{
								if (strpath.HasAdminDir())
									itemStates |= ITEMIS_INSVN;
							}
							if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
								itemStates |= ITEMIS_INSVN;
							if (status == git_wc_status_ignored)
							{
								itemStates |= ITEMIS_IGNORED;
								// the item is ignored. Get the svn:ignored properties so we can (maybe) later
								// offer a 'remove from ignored list' entry
//								GitProperties props(strpath.GetContainingDirectory(), false);
//								ignoredprops.empty();
//								for (int p=0; p<props.GetCount(); ++p)
//								{
//									if (props.GetItemName(p).compare(stdstring(_T("svn:ignore")))==0)
//									{
//										std::string st = props.GetItemValue(p);
//										ignoredprops = MultibyteToWide(st.c_str());
//										// remove all escape chars ('\\')
//										std::remove(ignoredprops.begin(), ignoredprops.end(), '\\');
//										break;
//									}
//								}
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
				if (g_ShellCache.HasSVNAdminDir(child.toString().c_str(), FALSE))
					itemStates |= ITEMIS_INVERSIONEDFOLDER;
				GlobalUnlock(medium.hGlobal);

				// if the item is a versioned folder, check if there's a patch file
				// in the clipboard to be used in "Apply Patch"
				UINT cFormatDiff = RegisterClipboardFormat(_T("TGIT_UNIFIEDDIFF"));
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
				IUnknown* relInterface = (IUnknown*)medium.pUnkForRelease;
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

		if ((folder_.compare(statuspath)!=0)&&(g_ShellCache.IsContextPathAllowed(folder_.c_str())))
		{
			
			try
			{
				GitStatus stat;
				stat.GetStatus(CTGitPath(folder_.c_str()), false, true, true);
				if (stat.status)
				{
					status = GitStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
//					if ((stat.status->entry)&&(stat.status->entry->lock_token))
//						itemStatesFolder |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
//					if ((stat.status->entry)&&(stat.status->entry->present_props))
//					{
//						if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
//							itemStatesFolder |= ITEMIS_NEEDSLOCK;
//					}
//					if ((stat.status->entry)&&(stat.status->entry->uuid))
//						uuidTarget = CUnicodeUtils::StdGetUnicode(stat.status->entry->uuid);
				
				}
				else
				{
					// sometimes, git_client_status() returns with an error.
					// in that case, we have to check if the working copy is versioned
					// anyway to show the 'correct' context menu
					if (askedpath.HasAdminDir())
						status = git_wc_status_normal;
				}
				
				//if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
				if (askedpath.HasAdminDir())
					itemStatesFolder |= ITEMIS_INSVN;
				if (status == git_wc_status_normal)
					itemStatesFolder |= ITEMIS_NORMAL;
				if (status == git_wc_status_conflicted)
					itemStatesFolder |= ITEMIS_CONFLICTED;
				if (status == git_wc_status_added)
					itemStatesFolder |= ITEMIS_ADDED;
				if (status == git_wc_status_deleted)
					itemStatesFolder |= ITEMIS_DELETED;

			}
			catch ( ... )
			{
				ATLTRACE2(_T("Exception in GitStatus::GetStatus()\n"));
			}
		}
		else
		{
			status = fetchedstatus;
		}
		//if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
		if (askedpath.HasAdminDir())
		{
			itemStatesFolder |= ITEMIS_FOLDERINSVN;
		}
		if (status == git_wc_status_ignored)
			itemStatesFolder |= ITEMIS_IGNORED;
		itemStatesFolder |= ITEMIS_FOLDER;
		if (files_.size() == 0)
			itemStates |= ITEMIS_ONLYONE;
		if (m_State != FileStateDropHandler)
			itemStates |= itemStatesFolder;

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
						stat.GetStatus(CTGitPath(folder_.c_str()), false, true, true);
						if (stat.status)
						{
							status = GitStatus::GetMoreImportant(stat.status->text_status, stat.status->prop_status);
//							if ((stat.status->entry)&&(stat.status->entry->lock_token))
//								itemStates |= (stat.status->entry->lock_token[0] != 0) ? ITEMIS_LOCKED : 0;
//							if ((stat.status->entry)&&(stat.status->entry->present_props))
//							{
//								if (strstr(stat.status->entry->present_props, "svn:needs-lock"))
//									itemStates |= ITEMIS_NEEDSLOCK;
//							}
//							if ((stat.status->entry)&&(stat.status->entry->uuid))
//								uuidTarget = CUnicodeUtils::StdGetUnicode(stat.status->entry->uuid);
						}
					}
					catch ( ... )
					{
						ATLTRACE2(_T("Exception in GitStatus::GetStatus()\n"));
					}
				}
				else
				{
					status = fetchedstatus;
				}
				//if ((status != git_wc_status_unversioned)&&(status != git_wc_status_ignored)&&(status != git_wc_status_none))
				if (askedpath.HasAdminDir())
					itemStates |= ITEMIS_FOLDERINSVN;
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
	
	return NOERROR;
}

void CShellExt::InsertGitMenu(BOOL istop, HMENU menu, UINT pos, UINT_PTR id, UINT stringid, UINT icon, UINT idCmdFirst, GitCommands com, UINT uFlags)
{
	TCHAR menutextbuffer[512] = {0};
	TCHAR verbsbuffer[255] = {0};
	MAKESTRING(stringid);

	if (istop)
	{
		//menu entry for the top context menu, so append an "Git " before
		//the menu text to indicate where the entry comes from
		_tcscpy_s(menutextbuffer, 255, _T("Git "));
	}
	_tcscat_s(menutextbuffer, 255, stringtablebuffer);
#if 1
	// insert branch name into "Git Commit..." entry, so it looks like "Git Commit "master"..."
	// so we have an easy and fast way to check the current branch
	// (the other alternative is using a separate disabled menu entry, the code is already done but commented out)
	if (com == ShellMenuCommit)
	{
		// get branch name
		CTGitPath path(folder_.empty() ? files_.front().c_str() : folder_.c_str());
		CString sProjectRoot;
		CString sBranchName;

		if (path.HasAdminDir(&sProjectRoot) && !g_Git.GetCurrentBranchFromFile(sProjectRoot, sBranchName))
		{
			if (sBranchName.GetLength() == 40)
			{
				// if SHA1 only show 4 first bytes
				BOOL bIsSha1 = TRUE;
				for (int i=0; i<40; i++)
					if ( !iswxdigit(sBranchName[i]) )
					{
						bIsSha1 = FALSE;
						break;
					}
				if (bIsSha1)
					sBranchName = sBranchName.Left(8) + _T("....");
			}

			// sanity check
			if (sBranchName.GetLength() > 64)
				sBranchName = sBranchName.Left(64) + _T("...");

			// scan to before "..."
			LPTSTR s = menutextbuffer + _tcslen(menutextbuffer)-1;
			if (s > menutextbuffer)
			{
				while (s > menutextbuffer)
				{
					if (*s != _T('.'))
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
			_tcscpy(s, _T(" -> \"") + sBranchName + _T("\"..."));
		}
	}
#endif
	if ((fullver < 0x500)||(fullver == 0x500 && !(uFlags&~(CMF_RESERVED|CMF_EXPLORE))))
	{
		// on win2k, the context menu does not work properly if we use
		// icon bitmaps. At least the menu text is empty in the context menu
		// for folder backgrounds (seems like a win2k bug).
		// the workaround is to use the check/unchecked bitmaps, which are drawn
		// with AND raster op, but it's better than nothing at all
		InsertMenu(menu, pos, MF_BYPOSITION | MF_STRING , id, menutextbuffer);
		if (icon)
		{
			HBITMAP bmp = IconToBitmap(icon); 
			SetMenuItemBitmaps(menu, pos, MF_BYPOSITION, bmp, bmp);
		}
	}
	else
	{
		MENUITEMINFO menuiteminfo;
		SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
		menuiteminfo.cbSize = sizeof(menuiteminfo);
		menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
		menuiteminfo.fType = MFT_STRING;
		menuiteminfo.dwTypeData = menutextbuffer;
		if (icon)
		{
			menuiteminfo.fMask |= MIIM_BITMAP;
			menuiteminfo.hbmpItem = (fullver >= 0x600) ? IconToBitmapPARGB32(icon) : HBMMENU_CALLBACK;
		}
		menuiteminfo.wID = id;
		InsertMenuItem(menu, pos, TRUE, &menuiteminfo);
	}

	if (istop)
	{
		//menu entry for the top context menu, so append an "Git " before
		//the menu text to indicate where the entry comes from
		_tcscpy_s(menutextbuffer, 255, _T("Git "));
	}
	LoadString(g_hResInst, stringid, verbsbuffer, sizeof(verbsbuffer));
	_tcscat_s(menutextbuffer, 255, verbsbuffer);
	stdstring verb = stdstring(menutextbuffer);
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

HBITMAP CShellExt::IconToBitmap(UINT uIcon)
{
	std::map<UINT, HBITMAP>::iterator bitmap_it = bitmaps.lower_bound(uIcon);
	if (bitmap_it != bitmaps.end() && bitmap_it->first == uIcon)
		return bitmap_it->second;

	HICON hIcon = (HICON)LoadImage(g_hResInst, MAKEINTRESOURCE(uIcon), IMAGE_ICON, 12, 12, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;

	RECT rect;

	rect.right = ::GetSystemMetrics(SM_CXMENUCHECK);
	rect.bottom = ::GetSystemMetrics(SM_CYMENUCHECK);

	rect.left = rect.top = 0;

	HWND desktop = ::GetDesktopWindow();
	if (desktop == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	HDC screen_dev = ::GetDC(desktop);
	if (screen_dev == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	// Create a compatible DC
	HDC dst_hdc = ::CreateCompatibleDC(screen_dev);
	if (dst_hdc == NULL)
	{
		DestroyIcon(hIcon);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Create a new bitmap of icon size
	HBITMAP bmp = ::CreateCompatibleBitmap(screen_dev, rect.right, rect.bottom);
	if (bmp == NULL)
	{
		DestroyIcon(hIcon);
		::DeleteDC(dst_hdc);
		::ReleaseDC(desktop, screen_dev); 
		return NULL;
	}

	// Select it into the compatible DC
	HBITMAP old_dst_bmp = (HBITMAP)::SelectObject(dst_hdc, bmp);
	if (old_dst_bmp == NULL)
	{
		DestroyIcon(hIcon);
		return NULL;
	}

	// Fill the background of the compatible DC with the white color
	// that is taken by menu routines as transparent
	::SetBkColor(dst_hdc, RGB(255, 255, 255));
	::ExtTextOut(dst_hdc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);

	// Draw the icon into the compatible DC
	::DrawIconEx(dst_hdc, 0, 0, hIcon, rect.right, rect.bottom, 0, NULL, DI_NORMAL);

	// Restore settings
	::SelectObject(dst_hdc, old_dst_bmp);
	::DeleteDC(dst_hdc);
	::ReleaseDC(desktop, screen_dev); 
	DestroyIcon(hIcon);
	if (bmp)
		bitmaps.insert(bitmap_it, std::make_pair(uIcon, bmp));
	return bmp;
}

bool CShellExt::WriteClipboardPathsToTempFile(stdstring& tempfile)
{
	bool bRet = true;
	tempfile = stdstring();
	//write all selected files and paths to a temporary file
	//for TortoiseProc.exe to read out again.
	DWORD written = 0;
	DWORD pathlength = GetTempPath(0, NULL);
	TCHAR * path = new TCHAR[pathlength+1];
	TCHAR * tempFile = new TCHAR[pathlength + 100];
	GetTempPath (pathlength+1, path);
	GetTempFileName (path, _T("git"), 0, tempFile);
	tempfile = stdstring(tempFile);

	HANDLE file = ::CreateFile (tempFile,
		GENERIC_WRITE, 
		FILE_SHARE_READ, 
		0, 
		CREATE_ALWAYS, 
		FILE_ATTRIBUTE_TEMPORARY,
		0);

	delete [] path;
	delete [] tempFile;
	if (file == INVALID_HANDLE_VALUE)
		return false;

	if (!IsClipboardFormatAvailable(CF_HDROP))
		return false;
	if (!OpenClipboard(NULL))
		return false;

	stdstring sClipboardText;
	HGLOBAL hglb = GetClipboardData(CF_HDROP);
	HDROP hDrop = (HDROP)GlobalLock(hglb);
	if(hDrop != NULL)
	{
		TCHAR szFileName[MAX_PATH];
		UINT cFiles = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0); 
		for(UINT i = 0; i < cFiles; ++i)
		{
			DragQueryFile(hDrop, i, szFileName, sizeof(szFileName));
			stdstring filename = szFileName;
			::WriteFile (file, filename.c_str(), filename.size()*sizeof(TCHAR), &written, 0);
			::WriteFile (file, _T("\n"), 2, &written, 0);
		}
		GlobalUnlock(hDrop);
	}
	else bRet = false;
	GlobalUnlock(hglb);

	CloseClipboard();
	::CloseHandle(file);

	return bRet;
}

stdstring CShellExt::WriteFileListToTempFile()
{
	//write all selected files and paths to a temporary file
	//for TortoiseProc.exe to read out again.
	DWORD pathlength = GetTempPath(0, NULL);
	TCHAR * path = new TCHAR[pathlength+1];
	TCHAR * tempFile = new TCHAR[pathlength + 100];
	GetTempPath (pathlength+1, path);
	GetTempFileName (path, _T("git"), 0, tempFile);
	stdstring retFilePath = stdstring(tempFile);
	
	HANDLE file = ::CreateFile (tempFile,
								GENERIC_WRITE, 
								FILE_SHARE_READ, 
								0, 
								CREATE_ALWAYS, 
								FILE_ATTRIBUTE_TEMPORARY,
								0);

	delete [] path;
	delete [] tempFile;
	if (file == INVALID_HANDLE_VALUE)
		return stdstring();
		
	DWORD written = 0;
	if (files_.empty())
	{
		::WriteFile (file, folder_.c_str(), folder_.size()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	}

	for (std::vector<stdstring>::iterator I = files_.begin(); I != files_.end(); ++I)
	{
		::WriteFile (file, I->c_str(), I->size()*sizeof(TCHAR), &written, 0);
		::WriteFile (file, _T("\n"), 2, &written, 0);
	}
	::CloseHandle(file);
	return retFilePath;
}

STDMETHODIMP CShellExt::QueryDropContext(UINT uFlags, UINT idCmdFirst, HMENU hMenu, UINT &indexMenu)
{
	PreserveChdir preserveChdir;
	LoadLangDll();

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return NOERROR;					//we don't change the default action

	if ((files_.size() == 0)||(folder_.size() == 0))
		return NOERROR;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return NOERROR;

	bool bSourceAndTargetFromSameRepository = (uuidSource.compare(uuidTarget) == 0) || uuidSource.empty() || uuidTarget.empty();

	//the drop handler only has eight commands, but not all are visible at the same time:
	//if the source file(s) are under version control then those files can be moved
	//to the new location or they can be moved with a rename, 
	//if they are unversioned then they can be added to the working copy
	//if they are versioned, they also can be exported to an unversioned location
	UINT idCmd = idCmdFirst;

	// Git move here
	// available if source is versioned but not added, target is versioned, source and target from same repository or target folder is added
	if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&((itemStates & ITEMIS_INSVN)&&((~itemStates) & ITEMIS_ADDED)))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVEMENU, 0, idCmdFirst, ShellMenuDropMove, uFlags);

	// Git move and rename here
	// available if source is a single, versioned but not added item, target is versioned, source and target from same repository or target folder is added
	if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_ONLYONE)&&((~itemStates) & ITEMIS_ADDED))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPMOVERENAMEMENU, 0, idCmdFirst, ShellMenuDropMoveRename, uFlags);

	// Git copy here
	// available if source is versioned but not added, target is versioned, source and target from same repository or target folder is added
	if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_INSVN)&&((~itemStates) & ITEMIS_ADDED))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYMENU, 0, idCmdFirst, ShellMenuDropCopy, uFlags);

	// Git copy and rename here, source and target from same repository
	// available if source is a single, versioned but not added item, target is versioned or target folder is added
	if ((bSourceAndTargetFromSameRepository||(itemStatesFolder & ITEMIS_ADDED))&&(itemStatesFolder & ITEMIS_FOLDERINSVN)&&(itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_ONLYONE)&&((~itemStates) & ITEMIS_ADDED))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYRENAMEMENU, 0, idCmdFirst, ShellMenuDropCopyRename, uFlags);

	// Git add here
	// available if target is versioned and source is either unversioned or from another repository
	if ((itemStatesFolder & ITEMIS_FOLDERINSVN)&&(((~itemStates) & ITEMIS_INSVN)||!bSourceAndTargetFromSameRepository))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPCOPYADDMENU, 0, idCmdFirst, ShellMenuDropCopyAdd, uFlags);

	// Git export here
	// available if source is versioned and a folder
	if ((itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_FOLDER))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTMENU, 0, idCmdFirst, ShellMenuDropExport, uFlags);

	// Git export all here
	// available if source is versioned and a folder
	if ((itemStates & ITEMIS_INSVN)&&(itemStates & ITEMIS_FOLDER))
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_DROPEXPORTEXTENDEDMENU, 0, idCmdFirst, ShellMenuDropExportExtended, uFlags);

	// apply patch
	// available if source is a patchfile
	if (itemStates & ITEMIS_PATCHFILE)
		InsertGitMenu(FALSE, hMenu, indexMenu++, idCmd++, IDS_MENUAPPLYPATCH, 0, idCmdFirst, ShellMenuApplyPatch, uFlags);

	// separator
	if (idCmd != idCmdFirst)
		InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 

	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
}

STDMETHODIMP CShellExt::QueryContextMenu(HMENU hMenu,
                                         UINT indexMenu,
                                         UINT idCmdFirst,
                                         UINT /*idCmdLast*/,
                                         UINT uFlags)
{
	ATLTRACE("Shell :: QueryContextMenu\n");
	PreserveChdir preserveChdir;
	
	//first check if our drop handler is called
	//and then (if true) provide the context menu for the
	//drop handler
	if (m_State == FileStateDropHandler)
	{
		return QueryDropContext(uFlags, idCmdFirst, hMenu, indexMenu);
	}

	if ((uFlags & CMF_DEFAULTONLY)!=0)
		return NOERROR;					//we don't change the default action

	if ((files_.size() == 0)&&(folder_.size() == 0))
		return NOERROR;

	if (((uFlags & 0x000f)!=CMF_NORMAL)&&(!(uFlags & CMF_EXPLORE))&&(!(uFlags & CMF_VERBSONLY)))
		return NOERROR;

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
	if (IsIllegalFolder(folder_, csidlarray))
		return NOERROR;

	if (folder_.empty())
	{
		// folder is empty, but maybe files are selected
		if (files_.size() == 0)
			return NOERROR;	// nothing selected - we don't have a menu to show
		// check whether a selected entry is an UID - those are namespace extensions
		// which we can't handle
		for (std::vector<stdstring>::const_iterator it = files_.begin(); it != files_.end(); ++it)
		{
			if (_tcsncmp(it->c_str(), _T("::{"), 3)==0)
				return NOERROR;
		}
	}

	//check if our menu is requested for a subversion admin directory
	if (g_GitAdminDir.IsAdminDirPath(folder_.c_str()))
		return NOERROR;

	if (uFlags & CMF_EXTENDEDVERBS)
		itemStates |= ITEMIS_EXTENDED;

	const BOOL bShortcut = !!(uFlags & CMF_VERBSONLY);
	if ( bShortcut && (files_.size()==1))
	{
		// Don't show the context menu for a link if the
		// destination is not part of a working copy.
		// It would only show the standard menu items
		// which are already shown for the lnk-file.
		CString path = files_.front().c_str();
		if ( !g_GitAdminDir.HasAdminDir(path) )
		{
			return NOERROR;
		}
	}

	//check if we already added our menu entry for a folder.
	//we check that by iterating through all menu entries and check if 
	//the dwItemData member points to our global ID string. That string is set
	//by our shell extension when the folder menu is inserted.
	TCHAR menubuf[MAX_PATH];
	int count = GetMenuItemCount(hMenu);
	for (int i=0; i<count; ++i)
	{
		MENUITEMINFO miif;
		SecureZeroMemory(&miif, sizeof(MENUITEMINFO));
		miif.cbSize = sizeof(MENUITEMINFO);
		miif.fMask = MIIM_DATA;
		miif.dwTypeData = menubuf;
		miif.cch = sizeof(menubuf)/sizeof(TCHAR);
		GetMenuItemInfo(hMenu, i, TRUE, &miif);
		if (miif.dwItemData == (ULONG_PTR)g_MenuIDString)
			return NOERROR;
	}

	LoadLangDll();
	UINT idCmd = idCmdFirst;

	//create the sub menu
	HMENU subMenu = CreateMenu();
	int indexSubMenu = 0;

	unsigned __int64 topmenu = g_ShellCache.GetMenuLayout();
	unsigned __int64 menumask = g_ShellCache.GetMenuMask();

	int menuIndex = 0;
	bool bAddSeparator = false;
	bool bMenuEntryAdded = false;
	bool bMenuEmpty = true;
	// insert separator at start
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;
	bool bShowIcons = !!DWORD(CRegStdWORD(_T("Software\\TortoiseGit\\ShowContextMenuIcons"), TRUE));
	// ?? TSV disabled icons for win2k and earlier, but they work for win2k and should work for win95 and up
	/*if (fullver <= 0x500)
		bShowIcons = false;*/

#if 0
	if (itemStates & (ITEMIS_INSVN|ITEMIS_FOLDERINSVN))
	{
		// show current branch name (as a "read-only" menu entry)

		CTGitPath path(folder_.empty() ? files_.front().c_str() : folder_.c_str());
		CString sProjectRoot;
		CString sBranchName;

		if (path.HasAdminDir(&sProjectRoot) && !g_Git.GetCurrentBranchFromFile(sProjectRoot, sBranchName))
		{
			if (sBranchName.GetLength() == 40)
			{
				// if SHA1 only show 4 first bytes
				BOOL bIsSha1 = TRUE;
				for (int i=0; i<40; i++)
					if ( !iswxdigit(sBranchName[i]) )
					{
						bIsSha1 = FALSE;
						break;
					}
				if (bIsSha1)
					sBranchName = sBranchName.Left(8) + _T("....");
			}

			sBranchName = _T('"') + sBranchName + _T('"');

			const int icon = IDI_COPY;
			const int pos = indexMenu++;
			const int id = idCmd++;

			if ((fullver < 0x500)||(fullver == 0x500 && !(uFlags&~(CMF_RESERVED|CMF_EXPLORE))))
			{
				InsertMenu(hMenu, pos, MF_DISABLED|MF_GRAYED|MF_BYPOSITION|MF_STRING, id, sBranchName);
				HBITMAP bmp = IconToBitmap(icon); 
				SetMenuItemBitmaps(hMenu, pos, MF_BYPOSITION, bmp, bmp);
			}
			else
			{
				MENUITEMINFO menuiteminfo;
				SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
				menuiteminfo.cbSize = sizeof(menuiteminfo);
				menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING | MIIM_STATE;
				menuiteminfo.fState = MFS_DISABLED;
				menuiteminfo.fType = MFT_STRING;
				menuiteminfo.dwTypeData = (LPWSTR)sBranchName.GetString();
				if (icon)
				{
					menuiteminfo.fMask |= MIIM_BITMAP;
					menuiteminfo.hbmpItem = (fullver >= 0x600) ? IconToBitmapPARGB32(icon) : HBMMENU_CALLBACK;

					if (menuiteminfo.hbmpItem == HBMMENU_CALLBACK)
					{
						// WM_DRAWITEM uses myIDMap to get icon, we use the same icon as create branch
						myIDMap[id - idCmdFirst] = ShellMenuBranch;
						myIDMap[id] = ShellMenuBranch;
					}
				}
				menuiteminfo.wID = id;
				InsertMenuItem(hMenu, pos, TRUE, &menuiteminfo);
			}
		}
	}
#endif

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
		}
		else
		{
			// check the conditions whether to show the menu entry or not
			bool bInsertMenu = false;

			if (menuInfo[menuIndex].firstyes && menuInfo[menuIndex].firstno)
			{
				if (((menuInfo[menuIndex].firstyes & itemStates) == menuInfo[menuIndex].firstyes)
					&&
					((menuInfo[menuIndex].firstno & (~itemStates)) == menuInfo[menuIndex].firstno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].firstyes)&&((menuInfo[menuIndex].firstyes & itemStates) == menuInfo[menuIndex].firstyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].firstno)&&((menuInfo[menuIndex].firstno & (~itemStates)) == menuInfo[menuIndex].firstno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].secondyes && menuInfo[menuIndex].secondno)
			{
				if (((menuInfo[menuIndex].secondyes & itemStates) == menuInfo[menuIndex].secondyes)
					&&
					((menuInfo[menuIndex].secondno & (~itemStates)) == menuInfo[menuIndex].secondno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].secondyes)&&((menuInfo[menuIndex].secondyes & itemStates) == menuInfo[menuIndex].secondyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].secondno)&&((menuInfo[menuIndex].secondno & (~itemStates)) == menuInfo[menuIndex].secondno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].thirdyes && menuInfo[menuIndex].thirdno)
			{
				if (((menuInfo[menuIndex].thirdyes & itemStates) == menuInfo[menuIndex].thirdyes)
					&&
					((menuInfo[menuIndex].thirdno & (~itemStates)) == menuInfo[menuIndex].thirdno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].thirdyes)&&((menuInfo[menuIndex].thirdyes & itemStates) == menuInfo[menuIndex].thirdyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].thirdno)&&((menuInfo[menuIndex].thirdno & (~itemStates)) == menuInfo[menuIndex].thirdno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].fourthyes && menuInfo[menuIndex].fourthno)
			{
				if (((menuInfo[menuIndex].fourthyes & itemStates) == menuInfo[menuIndex].fourthyes)
					&&
					((menuInfo[menuIndex].fourthno & (~itemStates)) == menuInfo[menuIndex].fourthno))
					bInsertMenu = true;
			}
			else if ((menuInfo[menuIndex].fourthyes)&&((menuInfo[menuIndex].fourthyes & itemStates) == menuInfo[menuIndex].fourthyes))
				bInsertMenu = true;
			else if ((menuInfo[menuIndex].fourthno)&&((menuInfo[menuIndex].fourthno & (~itemStates)) == menuInfo[menuIndex].fourthno))
				bInsertMenu = true;

			if (menuInfo[menuIndex].menuID & (~menumask))
			{
				if (bInsertMenu)
				{
					// insert a separator
					if ((bMenuEntryAdded)&&(bAddSeparator))
					{
						bAddSeparator = false;
						bMenuEntryAdded = false;
						InsertMenu(subMenu, indexSubMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); 
						idCmd++;
					}
					
					// handle special cases (sub menus)
					if ((menuInfo[menuIndex].command == ShellMenuIgnoreSub)||(menuInfo[menuIndex].command == ShellMenuUnIgnoreSub))
					{
						InsertIgnoreSubmenus(idCmd, idCmdFirst, hMenu, subMenu, indexMenu, indexSubMenu, topmenu, bShowIcons, uFlags);
						bMenuEntryAdded = true;
						bMenuEmpty = false;
					}
					else
					{
						bool bIsTop = ((topmenu & menuInfo[menuIndex].menuID) != 0);

						// insert the menu entry
						InsertGitMenu(	bIsTop,
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
						}
					}
				}
			}
		}
		menuIndex++;
	}

	//add sub menu to main context menu
	//don't use InsertMenu because this will lead to multiple menu entries in the explorer file menu.
	//see http://support.microsoft.com/default.aspx?scid=kb;en-us;214477 for details of that.
	MAKESTRING(IDS_MENUSUBMENU);
	MENUITEMINFO menuiteminfo;
	SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
	menuiteminfo.cbSize = sizeof(menuiteminfo);
	menuiteminfo.fType = MFT_STRING;
 	menuiteminfo.dwTypeData = stringtablebuffer;

	UINT uIcon = bShowIcons ? IDI_APP : 0;
	if (folder_.size())
	{
		uIcon = bShowIcons ? IDI_MENUFOLDER : 0;
		myIDMap[idCmd - idCmdFirst] = ShellSubMenuFolder;
		myIDMap[idCmd] = ShellSubMenuFolder;
		menuiteminfo.dwItemData = (ULONG_PTR)g_MenuIDString;
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
	else if (files_.size() > 1)
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
	HBITMAP bmp = NULL;
	if ((fullver < 0x500)||(fullver == 0x500 && !(uFlags&~(CMF_RESERVED|CMF_EXPLORE))))
	{
		menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
		if (uIcon)
		{
			menuiteminfo.fMask |= MIIM_CHECKMARKS;
			bmp = IconToBitmap(uIcon);
			menuiteminfo.hbmpChecked = bmp;
			menuiteminfo.hbmpUnchecked = bmp;
		}
	}
	else
	{
		menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING;
		if (uIcon)
		{
			menuiteminfo.fMask |= MIIM_BITMAP;
			menuiteminfo.hbmpItem = (fullver >= 0x600) ? IconToBitmapPARGB32(uIcon) : HBMMENU_CALLBACK;
		}
	}
	menuiteminfo.hSubMenu = subMenu;
	menuiteminfo.wID = idCmd++;
	InsertMenuItem(hMenu, indexMenu++, TRUE, &menuiteminfo);

	//separator after
	InsertMenu(hMenu, indexMenu++, MF_SEPARATOR|MF_BYPOSITION, 0, NULL); idCmd++;

	//return number of menu items added
	return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, 0, (USHORT)(idCmd - idCmdFirst)));
}


// This is called when you invoke a command on the menu:
STDMETHODIMP CShellExt::InvokeCommand(LPCMINVOKECOMMANDINFO lpcmi)
{
	PreserveChdir preserveChdir;
	HRESULT hr = E_INVALIDARG;
	if (lpcmi == NULL)
		return hr;

	std::string command;
	std::string parent;
	std::string file;

	if ((files_.size() > 0)||(folder_.size() > 0))
	{
		UINT idCmd = LOWORD(lpcmi->lpVerb);

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
		std::map<UINT_PTR, UINT_PTR>::const_iterator id_it = myIDMap.lower_bound(idCmd);
		if (id_it != myIDMap.end() && id_it->first == idCmd)
		{
			STARTUPINFO startup;
			PROCESS_INFORMATION process;
			memset(&startup, 0, sizeof(startup));
			startup.cb = sizeof(startup);
			memset(&process, 0, sizeof(process));
			CRegStdString tortoiseProcPath(_T("Software\\TortoiseGit\\ProcPath"), _T("TortoiseProc.exe"), false, HKEY_LOCAL_MACHINE);
			CRegStdString tortoiseMergePath(_T("Software\\TortoiseGit\\TMergePath"), _T("TortoiseMerge.exe"), false, HKEY_LOCAL_MACHINE);

			//TortoiseProc expects a command line of the form:
			//"/command:<commandname> /pathfile:<path> /startrev:<startrevision> /endrev:<endrevision> /deletepathfile
			// or
			//"/command:<commandname> /path:<path> /startrev:<startrevision> /endrev:<endrevision>
			//
			//* path is a path to a single file/directory for commands which only act on single items (log, checkout, ...)
			//* pathfile is a path to a temporary file which contains a list of file paths
			stdstring svnCmd = _T(" /command:");
			stdstring tempfile;
			switch (id_it->second)
			{
				//#region case
			case ShellMenuCheckout:
				svnCmd += _T("checkout /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuUpdate:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("update /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuUpdateExt:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("update /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /rev");
				break;
			case ShellMenuCommit:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("commit /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuAdd:
			case ShellMenuAddAsReplacement:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("add /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuIgnore:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("ignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuIgnoreCaseSensitive:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("ignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /onlymask");
				break;
			case ShellMenuUnIgnore:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("unignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuUnIgnoreCaseSensitive:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("unignore /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /onlymask");
				break;
			case ShellMenuRevert:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("revert /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuDelUnversioned:
				svnCmd += _T("delunversioned /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuCleanup:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("cleanup /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuResolve:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("resolve /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuSwitch:
				svnCmd += _T("switch /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuImport:
				svnCmd += _T("import /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuExport:
				svnCmd += _T("export /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuAbout:
				svnCmd += _T("about");
				break;
			case ShellMenuCreateRepos:
				svnCmd += _T("repocreate /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuMerge:
				svnCmd += _T("merge /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuMergeAll:
				svnCmd += _T("mergeall /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuCopy:
				svnCmd += _T("copy /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuSettings:
				svnCmd += _T("settings");
				break;
			case ShellMenuHelp:
				svnCmd += _T("help");
				break;
			case ShellMenuRename:
				svnCmd += _T("rename /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuRemove:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("remove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuRemoveKeep:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("remove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /keep");
				break;
			case ShellMenuDiff:
				svnCmd += _T("diff /path:\"");
				if (files_.size() == 1)
					svnCmd += files_.front();
				else if (files_.size() == 2)
				{
					std::vector<stdstring>::iterator I = files_.begin();
					svnCmd += *I;
					I++;
					svnCmd += _T("\" /path2:\"");
					svnCmd += *I;
				}
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
					svnCmd += _T(" /alternative");
				break;
			case ShellMenuPrevDiff:
				svnCmd += _T("prevdiff /path:\"");
				if (files_.size() == 1)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
					svnCmd += _T(" /alternative");
				break;
			case ShellMenuUrlDiff:
				svnCmd += _T("urldiff /path:\"");
				if (files_.size() == 1)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuDropCopyAdd:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropcopyadd /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"";)
					break;
			case ShellMenuDropCopy:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropcopy /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"";)
					break;
			case ShellMenuDropCopyRename:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropcopy /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\" /rename";)
					break;
			case ShellMenuDropMove:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropmove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuDropMoveRename:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropmove /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\" /rename";)
				break;
			case ShellMenuDropExport:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropexport /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuDropExportExtended:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("dropexport /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				svnCmd += _T(" /droptarget:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				svnCmd += _T(" /extended");
				break;
			case ShellMenuLog:
				svnCmd += _T("log /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuConflictEditor:
				svnCmd += _T("conflicteditor /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuRelocate:
				svnCmd += _T("relocate /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuRebase:
				svnCmd += _T("rebase /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuShowChanged:
				if (files_.size() > 1)
                {
				    tempfile = WriteFileListToTempFile();
				    svnCmd += _T("repostatus /pathfile:\"");
				    svnCmd += tempfile;
    				svnCmd += _T("\"");
    				svnCmd += _T(" /deletepathfile");
                }
                else
                {
                    svnCmd += _T("repostatus /path:\"");
				    if (files_.size() > 0)
					    svnCmd += files_.front();
				    else
					    svnCmd += folder_;
    				svnCmd += _T("\"");
                }
				break;
			case ShellMenuRepoBrowse:
				svnCmd += _T("repobrowser /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuBlame:
				svnCmd += _T("blame /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuCreatePatch:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("createpatch /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuApplyPatch:
				if ((itemStates & ITEMIS_PATCHINCLIPBOARD) && ((~itemStates) & ITEMIS_PATCHFILE))
				{
					// if there's a patch file in the clipboard, we save it
					// to a temporary file and tell TortoiseMerge to use that one
					UINT cFormat = RegisterClipboardFormat(_T("TGIT_UNIFIEDDIFF"));
					if ((cFormat)&&(OpenClipboard(NULL)))
					{ 
						HGLOBAL hglb = GetClipboardData(cFormat); 
						LPCSTR lpstr = (LPCSTR)GlobalLock(hglb); 

						DWORD len = GetTempPath(0, NULL);
						TCHAR * path = new TCHAR[len+1];
						TCHAR * tempF = new TCHAR[len+100];
						GetTempPath (len+1, path);
						GetTempFileName (path, TEXT("git"), 0, tempF);
						std::wstring sTempFile = std::wstring(tempF);
						delete [] path;
						delete [] tempF;

						FILE * outFile;
						size_t patchlen = strlen(lpstr);
						_tfopen_s(&outFile, sTempFile.c_str(), _T("wb"));
						if(outFile)
						{
							size_t size = fwrite(lpstr, sizeof(char), patchlen, outFile);
							if (size == patchlen)
							{
								itemStates |= ITEMIS_PATCHFILE;
								files_.clear();
								files_.push_back(sTempFile);
							}
							fclose(outFile);
						}
						GlobalUnlock(hglb); 
						CloseClipboard(); 
					} 
				}
				if (itemStates & ITEMIS_PATCHFILE)
				{
					svnCmd = _T(" /diff:\"");
					if (files_.size() > 0)
					{
						svnCmd += files_.front();
						if (itemStatesFolder & ITEMIS_FOLDERINSVN)
						{
							svnCmd += _T("\" /patchpath:\"");
							svnCmd += folder_;
						}
					}
					else
						svnCmd += folder_;
					if (itemStates & ITEMIS_INVERSIONEDFOLDER)
						svnCmd += _T("\" /wc");
					else
						svnCmd += _T("\"");
				}
				else
				{
					svnCmd = _T(" /patchpath:\"");
					if (files_.size() > 0)
						svnCmd += files_.front();
					else
						svnCmd += folder_;
					svnCmd += _T("\"");
				}
				myIDMap.clear();
				myVerbsIDMap.clear();
				myVerbsMap.clear();
				if (CreateProcess(((stdstring)tortoiseMergePath).c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
				{
					LPVOID lpMsgBuf;
					FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
						FORMAT_MESSAGE_FROM_SYSTEM | 
						FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						GetLastError(),
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
						(LPTSTR) &lpMsgBuf,
						0,
						NULL 
						);
					MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("TortoiseMerge launch failed"), MB_OK | MB_ICONINFORMATION );
					LocalFree( lpMsgBuf );
				}
				CloseHandle(process.hThread);
				CloseHandle(process.hProcess);
				return NOERROR;
				break;
			case ShellMenuRevisionGraph:
				svnCmd += _T("revisiongraph /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuProperties:
				tempfile = WriteFileListToTempFile();
				svnCmd += _T("properties /pathfile:\"");
				svnCmd += tempfile;
				svnCmd += _T("\"");
				svnCmd += _T(" /deletepathfile");
				break;
			case ShellMenuClipPaste:
				if (WriteClipboardPathsToTempFile(tempfile))
				{
					bool bCopy = true;
					UINT cPrefDropFormat = RegisterClipboardFormat(_T("Preferred DropEffect"));
					if (cPrefDropFormat)
					{
						if (OpenClipboard(lpcmi->hwnd))
						{
							HGLOBAL hglb = GetClipboardData(cPrefDropFormat);
							if (hglb)
							{
								DWORD* effect = (DWORD*) GlobalLock(hglb);
								if (*effect == DROPEFFECT_MOVE)
									bCopy = false;
								GlobalUnlock(hglb);
							}
							CloseClipboard();
						}
					}

					if (bCopy)
						svnCmd += _T("pastecopy /pathfile:\"");
					else
						svnCmd += _T("pastemove /pathfile:\"");
					svnCmd += tempfile;
					svnCmd += _T("\"");
					svnCmd += _T(" /deletepathfile");
					svnCmd += _T(" /droptarget:\"");
					svnCmd += folder_;
					svnCmd += _T("\"");
				}
				else return NOERROR;
				break;
			case ShellMenuClone:
				svnCmd += _T("clone /path:\"");
				svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuPull:
				svnCmd += _T("pull /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuPush:
				svnCmd += _T("push /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuBranch:
				svnCmd += _T("branch /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			
			case ShellMenuTag:
				svnCmd += _T("tag /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;

			case ShellMenuFormatPatch:
				svnCmd += _T("formatpatch /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;

			case ShellMenuImportPatch:
				svnCmd += _T("importpatch /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;

			case ShellMenuCherryPick:
				svnCmd += _T("cherrypick /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;
			case ShellMenuFetch:
				svnCmd += _T("fetch /path:\"");
				if (files_.size() > 0)
					svnCmd += files_.front();
				else
					svnCmd += folder_;
				svnCmd += _T("\"");
				break;

			default:
				break;
				//#endregion
			} // switch (id_it->second) 
			svnCmd += _T(" /hwnd:");
			TCHAR buf[30];
			_stprintf_s(buf, 30, _T("%d"), lpcmi->hwnd);
			svnCmd += buf;
			myIDMap.clear();
			myVerbsIDMap.clear();
			myVerbsMap.clear();
			if (CreateProcess(((stdstring)tortoiseProcPath).c_str(), const_cast<TCHAR*>(svnCmd.c_str()), NULL, NULL, FALSE, 0, 0, 0, &startup, &process)==0)
			{
				LPVOID lpMsgBuf;
				FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | 
					FORMAT_MESSAGE_FROM_SYSTEM | 
					FORMAT_MESSAGE_IGNORE_INSERTS,
					NULL,
					GetLastError(),
					MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
					(LPTSTR) &lpMsgBuf,
					0,
					NULL 
					);
				MessageBox( NULL, (LPCTSTR)lpMsgBuf, _T("TortoiseProc Launch failed"), MB_OK | MB_ICONINFORMATION );
				LocalFree( lpMsgBuf );
			}
			CloseHandle(process.hThread);
			CloseHandle(process.hProcess);
			hr = NOERROR;
		} // if (id_it != myIDMap.end() && id_it->first == idCmd) 
	} // if ((files_.size() > 0)||(folder_.size() > 0)) 
	return hr;

}

// This is for the status bar and things like that:
STDMETHODIMP CShellExt::GetCommandString(UINT_PTR idCmd,
                                         UINT uFlags,
                                         UINT FAR * /*reserved*/,
                                         LPSTR pszName,
                                         UINT cchMax)
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
		if (menuInfo[menuIndex].command == (GitCommands)id_it->second)
		{
			MAKESTRING(menuInfo[menuIndex].menuDescID);
			break;
		}
		menuIndex++;
	}

	const TCHAR * desc = stringtablebuffer;
	switch(uFlags)
	{
	case GCS_HELPTEXTA:
		{
			std::string help = WideToMultibyte(desc);
			lstrcpynA(pszName, help.c_str(), cchMax);
			hr = S_OK;
			break; 
		}
	case GCS_HELPTEXTW: 
		{
			wide_string help = desc;
			lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax); 
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
				ATLTRACE("verb : %ws\n", help.c_str());
				lstrcpynW((LPWSTR)pszName, help.c_str(), cchMax); 
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
	if (pResult == NULL)
		pResult = &res;
	*pResult = FALSE;

	LoadLangDll();
	switch (uMsg)
	{
	case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* lpmis = (MEASUREITEMSTRUCT*)lParam;
			if (lpmis==NULL||lpmis->CtlType!=ODT_MENU)
				break;
			lpmis->itemWidth += 2;
			if (lpmis->itemHeight < 16)
				lpmis->itemHeight = 16;
			*pResult = TRUE;
		}
		break;
	case WM_DRAWITEM:
		{
			LPCTSTR resource;
			DRAWITEMSTRUCT* lpdis = (DRAWITEMSTRUCT*)lParam;
			if ((lpdis==NULL)||(lpdis->CtlType != ODT_MENU))
				return S_OK;		//not for a menu
			resource = GetMenuTextFromResource(myIDMap[lpdis->itemID]);
			if (resource == NULL)
				return S_OK;
			HICON hIcon = (HICON)LoadImage(g_hResInst, resource, IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
			if (hIcon == NULL)
				return S_OK;
			DrawIconEx(lpdis->hDC,
				lpdis->rcItem.left - 16,
				lpdis->rcItem.top + (lpdis->rcItem.bottom - lpdis->rcItem.top - 16) / 2,
				hIcon, 16, 16,
				0, NULL, DI_NORMAL);
			DestroyIcon(hIcon);
			*pResult = TRUE;
		}
		break;
	case WM_MENUCHAR:
		{
			LPCTSTR resource;
			TCHAR *szItem;
			if (HIWORD(wParam) != MF_POPUP)
				return NOERROR;
			int nChar = LOWORD(wParam);
			if (_istascii((wint_t)nChar) && _istupper((wint_t)nChar))
				nChar = tolower(nChar);
			// we have the char the user pressed, now search that char in all our
			// menu items
			std::vector<int> accmenus;
			for (std::map<UINT_PTR, UINT_PTR>::iterator It = mySubMenuMap.begin(); It != mySubMenuMap.end(); ++It)
			{
				resource = GetMenuTextFromResource(mySubMenuMap[It->first]);
				if (resource == NULL)
					continue;
				szItem = stringtablebuffer;
				TCHAR * amp = _tcschr(szItem, '&');
				if (amp == NULL)
					continue;
				amp++;
				int ampChar = LOWORD(*amp);
				if (_istascii((wint_t)ampChar) && _istupper((wint_t)ampChar))
					ampChar = tolower(ampChar);
				if (ampChar == nChar)
				{
					// yep, we found a menu which has the pressed key
					// as an accelerator. Add that menu to the list to
					// process later.
					accmenus.push_back(It->first);
				}
			}
			if (accmenus.size() == 0)
			{
				// no menu with that accelerator key.
				*pResult = MAKELONG(0, MNC_IGNORE);
				return NOERROR;
			}
			if (accmenus.size() == 1)
			{
				// Only one menu with that accelerator key. We're lucky!
				// So just execute that menu entry.
				*pResult = MAKELONG(accmenus[0], MNC_EXECUTE);
				return NOERROR;
			}
			if (accmenus.size() > 1)
			{
				// we have more than one menu item with this accelerator key!
				MENUITEMINFO mif;
				mif.cbSize = sizeof(MENUITEMINFO);
				mif.fMask = MIIM_STATE;
				for (std::vector<int>::iterator it = accmenus.begin(); it != accmenus.end(); ++it)
				{
					GetMenuItemInfo((HMENU)lParam, *it, TRUE, &mif);
					if (mif.fState == MFS_HILITE)
					{
						// this is the selected item, so select the next one
						++it;
						if (it == accmenus.end())
							*pResult = MAKELONG(accmenus[0], MNC_SELECT);
						else
							*pResult = MAKELONG(*it, MNC_SELECT);
						return NOERROR;
					}
				}
				*pResult = MAKELONG(accmenus[0], MNC_SELECT);
			}
		}
		break;
	default:
		return NOERROR;
	}

	return NOERROR;
}

LPCTSTR CShellExt::GetMenuTextFromResource(int id)
{
	TCHAR textbuf[255];
	LPCTSTR resource = NULL;
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
				space = layout & menuInfo[menuIndex].menuID ? 0 : 6;
				if (layout & (menuInfo[menuIndex].menuID)) 
				{
					_tcscpy_s(textbuf, 255, _T("Git "));
					_tcscat_s(textbuf, 255, stringtablebuffer);
					_tcscpy_s(stringtablebuffer, 255, textbuf);
				}
				break;
			}
			return resource;
		}
		menuIndex++;
	}
	return NULL;
}

bool CShellExt::IsIllegalFolder(std::wstring folder, int * cslidarray)
{
	int i=0;
	TCHAR buf[MAX_PATH];	//MAX_PATH ok, since SHGetSpecialFolderPath doesn't return the required buffer length!
	LPITEMIDLIST pidl = NULL;
	while (cslidarray[i])
	{
		++i;
		pidl = NULL;
		if (SHGetFolderLocation(NULL, cslidarray[i-1], NULL, 0, &pidl)!=S_OK)
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

void CShellExt::InsertIgnoreSubmenus(UINT &idCmd, UINT idCmdFirst, HMENU hMenu, HMENU subMenu, UINT &indexMenu, int &indexSubMenu, unsigned __int64 topmenu, bool bShowIcons, UINT uFlags)
{
	HMENU ignoresubmenu = NULL;
	int indexignoresub = 0;
	bool bShowIgnoreMenu = false;
	TCHAR maskbuf[MAX_PATH];		// MAX_PATH is ok, since this only holds a filename
	TCHAR ignorepath[MAX_PATH];		// MAX_PATH is ok, since this only holds a filename
	if (files_.size() == 0)
		return;
	UINT icon = bShowIcons ? IDI_IGNORE : 0;

	std::vector<stdstring>::iterator I = files_.begin();
	if (_tcsrchr(I->c_str(), '\\'))
		_tcscpy_s(ignorepath, MAX_PATH, _tcsrchr(I->c_str(), '\\')+1);
	else
		_tcscpy_s(ignorepath, MAX_PATH, I->c_str());
	if ((itemStates & ITEMIS_IGNORED)&&(ignoredprops.size() > 0))
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
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
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
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
			myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
			myIDMap[idCmd++] = ShellMenuIgnore;

			_tcscpy_s(maskbuf, MAX_PATH, _T("*"));
			if (_tcsrchr(ignorepath, '.'))
			{
				_tcscat_s(maskbuf, MAX_PATH, _tcsrchr(ignorepath, '.'));
				InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, maskbuf);
				stdstring verb = stdstring(maskbuf);
				myVerbsMap[verb] = idCmd - idCmdFirst;
				myVerbsMap[verb] = idCmd;
				myVerbsIDMap[idCmd - idCmdFirst] = verb;
				myVerbsIDMap[idCmd] = verb;
				myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitive;
				myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitive;
			}
		}
		else
		{
			MAKESTRING(IDS_MENUIGNOREMULTIPLE);
			_stprintf_s(ignorepath, MAX_PATH, stringtablebuffer, files_.size());
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
			stdstring verb = stdstring(ignorepath);
			myVerbsMap[verb] = idCmd - idCmdFirst;
			myVerbsMap[verb] = idCmd;
			myVerbsIDMap[idCmd - idCmdFirst] = verb;
			myVerbsIDMap[idCmd] = verb;
			myIDMap[idCmd - idCmdFirst] = ShellMenuIgnore;
			myIDMap[idCmd++] = ShellMenuIgnore;

			MAKESTRING(IDS_MENUIGNOREMULTIPLEMASK);
			_stprintf_s(ignorepath, MAX_PATH, stringtablebuffer, files_.size());
			InsertMenu(ignoresubmenu, indexignoresub++, MF_BYPOSITION | MF_STRING , idCmd, ignorepath);
			verb = stdstring(ignorepath);
			myVerbsMap[verb] = idCmd - idCmdFirst;
			myVerbsMap[verb] = idCmd;
			myVerbsIDMap[idCmd - idCmdFirst] = verb;
			myVerbsIDMap[idCmd] = verb;
			myIDMap[idCmd - idCmdFirst] = ShellMenuIgnoreCaseSensitive;
			myIDMap[idCmd++] = ShellMenuIgnoreCaseSensitive;
		}
	}

	if (bShowIgnoreMenu)
	{
		MENUITEMINFO menuiteminfo;
		SecureZeroMemory(&menuiteminfo, sizeof(menuiteminfo));
		menuiteminfo.cbSize = sizeof(menuiteminfo);
		if (fullver < 0x500 || (fullver == 0x500 && !(uFlags&~(CMF_RESERVED|CMF_EXPLORE))))
		{
			menuiteminfo.fMask = MIIM_STRING | MIIM_ID | MIIM_SUBMENU | MIIM_DATA;
			if (icon)
			{
				HBITMAP bmp = IconToBitmap(icon);
				menuiteminfo.fMask |= MIIM_CHECKMARKS;
				menuiteminfo.hbmpChecked = bmp;
				menuiteminfo.hbmpUnchecked = bmp;
			}
		}
		else
		{
			menuiteminfo.fMask = MIIM_FTYPE | MIIM_ID | MIIM_SUBMENU | MIIM_DATA | MIIM_STRING;
			if (icon)
			{
				menuiteminfo.fMask |= MIIM_BITMAP;
				menuiteminfo.hbmpItem = (fullver >= 0x600) ? IconToBitmapPARGB32(icon) : HBMMENU_CALLBACK;
			}
		}
		menuiteminfo.fType = MFT_STRING;
		menuiteminfo.hSubMenu = ignoresubmenu;
		menuiteminfo.wID = idCmd;
		SecureZeroMemory(stringtablebuffer, sizeof(stringtablebuffer));
		if (itemStates & ITEMIS_IGNORED)
			GetMenuTextFromResource(ShellMenuUnIgnoreSub);
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
}

HBITMAP CShellExt::IconToBitmapPARGB32(UINT uIcon)
{
	std::map<UINT, HBITMAP>::iterator bitmap_it = bitmaps.lower_bound(uIcon);
	if (bitmap_it != bitmaps.end() && bitmap_it->first == uIcon)
		return bitmap_it->second;

	HICON hIcon = (HICON)LoadImage(g_hResInst, MAKEINTRESOURCE(uIcon), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
	if (!hIcon)
		return NULL;

	if (pfnBeginBufferedPaint == NULL || pfnEndBufferedPaint == NULL || pfnGetBufferedPaintBits == NULL)
		return NULL;

	SIZE sizIcon;
	sizIcon.cx = GetSystemMetrics(SM_CXSMICON);
	sizIcon.cy = GetSystemMetrics(SM_CYSMICON);

	RECT rcIcon;
	SetRect(&rcIcon, 0, 0, sizIcon.cx, sizIcon.cy);
	HBITMAP hBmp = NULL;

	HDC hdcDest = CreateCompatibleDC(NULL);
	if (hdcDest)
	{
		if (SUCCEEDED(Create32BitHBITMAP(hdcDest, &sizIcon, NULL, &hBmp)))
		{
			HBITMAP hbmpOld = (HBITMAP)SelectObject(hdcDest, hBmp);
			if (hbmpOld)
			{
				BLENDFUNCTION bfAlpha = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
				BP_PAINTPARAMS paintParams = {0};
				paintParams.cbSize = sizeof(paintParams);
				paintParams.dwFlags = BPPF_ERASE;
				paintParams.pBlendFunction = &bfAlpha;

				HDC hdcBuffer;
				HPAINTBUFFER hPaintBuffer = pfnBeginBufferedPaint(hdcDest, &rcIcon, BPBF_DIB, &paintParams, &hdcBuffer);
				if (hPaintBuffer)
				{
					if (DrawIconEx(hdcBuffer, 0, 0, hIcon, sizIcon.cx, sizIcon.cy, 0, NULL, DI_NORMAL))
					{
						// If icon did not have an alpha channel we need to convert buffer to PARGB
						ConvertBufferToPARGB32(hPaintBuffer, hdcDest, hIcon, sizIcon);
					}

					// This will write the buffer contents to the destination bitmap
					pfnEndBufferedPaint(hPaintBuffer, TRUE);
				}

				SelectObject(hdcDest, hbmpOld);
			}
		}

		DeleteDC(hdcDest);
	}

	DestroyIcon(hIcon);

	if(hBmp)
		bitmaps.insert(bitmap_it, std::make_pair(uIcon, hBmp));
	return hBmp;
}

HRESULT CShellExt::Create32BitHBITMAP(HDC hdc, const SIZE *psize, __deref_opt_out void **ppvBits, __out HBITMAP* phBmp)
{
	*phBmp = NULL;

	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = psize->cx;
	bmi.bmiHeader.biHeight = psize->cy;
	bmi.bmiHeader.biBitCount = 32;

	HDC hdcUsed = hdc ? hdc : GetDC(NULL);
	if (hdcUsed)
	{
		*phBmp = CreateDIBSection(hdcUsed, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
		if (hdc != hdcUsed)
		{
			ReleaseDC(NULL, hdcUsed);
		}
	}
	return (NULL == *phBmp) ? E_OUTOFMEMORY : S_OK;
}

HRESULT CShellExt::ConvertBufferToPARGB32(HPAINTBUFFER hPaintBuffer, HDC hdc, HICON hicon, SIZE& sizIcon)
{
	RGBQUAD *prgbQuad;
	int cxRow;
	HRESULT hr = pfnGetBufferedPaintBits(hPaintBuffer, &prgbQuad, &cxRow);
	if (SUCCEEDED(hr))
	{
		ARGB *pargb = reinterpret_cast<ARGB *>(prgbQuad);
		if (!HasAlpha(pargb, sizIcon, cxRow))
		{
			ICONINFO info;
			if (GetIconInfo(hicon, &info))
			{
				if (info.hbmMask)
				{
					hr = ConvertToPARGB32(hdc, pargb, info.hbmMask, sizIcon, cxRow);
				}

				DeleteObject(info.hbmColor);
				DeleteObject(info.hbmMask);
			}
		}
	}

	return hr;
}

bool CShellExt::HasAlpha(__in ARGB *pargb, SIZE& sizImage, int cxRow)
{
	ULONG cxDelta = cxRow - sizImage.cx;
	for (ULONG y = sizImage.cy; y; --y)
	{
		for (ULONG x = sizImage.cx; x; --x)
		{
			if (*pargb++ & 0xFF000000)
			{
				return true;
			}
		}

		pargb += cxDelta;
	}

	return false;
}

HRESULT CShellExt::ConvertToPARGB32(HDC hdc, __inout ARGB *pargb, HBITMAP hbmp, SIZE& sizImage, int cxRow)
{
	BITMAPINFO bmi;
	ZeroMemory(&bmi, sizeof(bmi));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biCompression = BI_RGB;

	bmi.bmiHeader.biWidth = sizImage.cx;
	bmi.bmiHeader.biHeight = sizImage.cy;
	bmi.bmiHeader.biBitCount = 32;

	HRESULT hr = E_OUTOFMEMORY;
	HANDLE hHeap = GetProcessHeap();
	void *pvBits = HeapAlloc(hHeap, 0, bmi.bmiHeader.biWidth * 4 * bmi.bmiHeader.biHeight);
	if (pvBits)
	{
		hr = E_UNEXPECTED;
		if (GetDIBits(hdc, hbmp, 0, bmi.bmiHeader.biHeight, pvBits, &bmi, DIB_RGB_COLORS) == bmi.bmiHeader.biHeight)
		{
			ULONG cxDelta = cxRow - bmi.bmiHeader.biWidth;
			ARGB *pargbMask = static_cast<ARGB *>(pvBits);

			for (ULONG y = bmi.bmiHeader.biHeight; y; --y)
			{
				for (ULONG x = bmi.bmiHeader.biWidth; x; --x)
				{
					if (*pargbMask++)
					{
						// transparent pixel
						*pargb++ = 0;
					}
					else
					{
						// opaque pixel
						*pargb++ |= 0xFF000000;
					}
				}

				pargb += cxDelta;
			}

			hr = S_OK;
		}

		HeapFree(hHeap, 0, pvBits);
	}

	return hr;
}

