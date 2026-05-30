// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2018-2024, 2026 - TortoiseGit
// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "menuinfo.h"
#include "resource.h"
#include "Globals.h"

// defaults are specified in ShellCache.h

constinit const MenuInfo menuInfo[] =
{
	{ TGitShellCommand::Inaccessible,				TGitContextMenuEntries::Inaccessible,		IDI_CLEANUP,			IDS_MENUINACCESSIBLE,		IDS_MENUDESCINACCESSIBLE,
		{ITEMIS_INACCESSIBLE, ITEMIS_INGIT|ITEMIS_FOLDERINGIT|ITEMIS_BAREREPO}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Clone,						TGitContextMenuEntries::Clone,				IDI_CLONE,				IDS_MENUCLONE,				IDS_MENUDESCCLONE,
		{ITEMIS_FOLDER, ITEMIS_INGIT|ITEMIS_FOLDERINGIT|ITEMIS_BAREREPO|ITEMIS_INACCESSIBLE}, {ITEMIS_FOLDER|ITEMIS_IGNORED, 0}, {ITEMIS_FOLDER|ITEMIS_EXTENDED, 0}, {0, 0} },

	{ TGitShellCommand::Pull,						TGitContextMenuEntries::Pull,				IDI_PULL,				IDS_MENUPULL,				IDS_MENUDESCPULL,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {ITEMIS_WCROOT, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Fetch,						TGitContextMenuEntries::Fetch,				IDI_UPDATE,				IDS_MENUFETCH,				IDS_MENUDESCFETCH,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {ITEMIS_BAREREPO, 0}, {ITEMIS_WCROOT, 0}, {0, 0} },

	{ TGitShellCommand::Push,						TGitContextMenuEntries::Push,				IDI_PUSH,				IDS_MENUPUSH,				IDS_MENUDESCPUSH,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {ITEMIS_BAREREPO, 0}, {ITEMIS_WCROOT, 0}, {0, 0} },

	{ TGitShellCommand::Sync,						TGitContextMenuEntries::Sync,				IDI_RELOCATE,			IDS_MENUSYNC,				IDS_MENUDESCSYNC,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0}, {0, 0} },


	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Commit,						TGitContextMenuEntries::Commit,				IDI_COMMIT,				IDS_MENUCOMMIT,				IDS_MENUDESCCOMMIT,
		{ITEMIS_INGIT, 0}, {ITEMIS_FOLDERINGIT, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::SVNDCommit,					TGitContextMenuEntries::SVNDCommit,			IDI_COMMIT,				IDS_MENUSVNDCOMMIT,			IDS_MENUSVNDCOMMIT_DESC,
		{ITEMIS_INGIT|ITEMIS_GITSVN, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {ITEMIS_FOLDERINGIT|ITEMIS_GITSVN, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0} },

	{ TGitShellCommand::SVNRebase,					TGitContextMenuEntries::SVNRebase,			IDI_REBASE,				IDS_MENUSVNREBASE,			IDS_MENUSVNREBASE_DESC,
		{ITEMIS_FOLDERINGIT|ITEMIS_GITSVN|ITEMIS_ONLYONE, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::SVNDFetch,					TGitContextMenuEntries::SVNFetch,			IDI_UPDATE,				IDS_MENUSVNFETCH,			IDS_MENUDESCSVNFETCH,
		{ITEMIS_FOLDERINGIT|ITEMIS_GITSVN|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::SVNIgnore,					TGitContextMenuEntries::SVNIgnore,			IDI_IGNORE,				IDS_MENUSVNIGNORE,			IDS_MENUSVNIGNORE_DESC,
		{ITEMIS_FOLDERINGIT|ITEMIS_GITSVN|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Diff,						TGitContextMenuEntries::Diff,				IDI_DIFF,				IDS_MENUDIFF,				IDS_MENUDESCDIFF,
		{ITEMIS_INGIT|ITEMIS_ONLYONE, 0}, {ITEMIS_TWO, ITEMIS_FOLDER}, {0, 0}, {0, 0} },

	{ TGitShellCommand::DiffLater,					TGitContextMenuEntries::DiffLater,			IDI_DIFF,				IDS_MENUDIFFLATER,			IDS_MENUDESCDIFFLATER,
		{ITEMIS_ONLYONE, ITEMIS_FOLDER}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::PrevDiff,					TGitContextMenuEntries::PrevDiff,			IDI_DIFF,				IDS_MENUPREVDIFF,			IDS_MENUDESCPREVDIFF,
		{ITEMIS_INGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Log,							TGitContextMenuEntries::Log,				IDI_LOG,				IDS_MENULOG,				IDS_MENUDESCLOG,
		{ITEMIS_INGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_FOLDER|ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_BAREREPO, 0} },

	{ TGitShellCommand::LogSubmoduleFolder,			TGitContextMenuEntries::LogSubmodule,		IDI_LOG,				IDS_MENULOGSUBMODULE,		IDS_MENUDESCLOG,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE|ITEMIS_WCROOT|ITEMIS_SUBMODULE, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::RefLog,						TGitContextMenuEntries::RefLog,				IDI_LOG,				IDS_MENUREFLOG,				IDS_MENUDESCREFLOG,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {ITEMIS_BAREREPO, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::RefBrowser,					TGitContextMenuEntries::RefBrowser,			IDI_REPOBROWSE,			IDS_MENUREFBROWSE,			IDS_MENUDESCREFBROWSE,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {ITEMIS_BAREREPO, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Daemon,						TGitContextMenuEntries::Daemon,				IDI_DAEMON,				IDS_MENUDAEMON,				IDS_MENUDESCDAEMON,
		{ITEMIS_INGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_FOLDER|ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_BAREREPO, 0} },

	{ TGitShellCommand::RevisionGraph,				TGitContextMenuEntries::RevisionGraph,		IDI_REVISIONGRAPH,		IDS_MENUREVISIONGRAPH,		IDS_MENUDESCREVISIONGRAPH,
		{ITEMIS_INGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_FOLDER|ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_ADDED}, {ITEMIS_BAREREPO, 0} },

	{ TGitShellCommand::RepoBrowse,					TGitContextMenuEntries::RepoBrowser,		IDI_REPOBROWSE,			IDS_MENUREPOBROWSE,			IDS_MENUDESCREPOBROWSE,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {ITEMIS_BAREREPO|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::ShowChanged,					TGitContextMenuEntries::ShowChanged,		IDI_SHOWCHANGED,		IDS_MENUSHOWCHANGED,		IDS_MENUDESCSHOWCHANGED,
		{ITEMIS_INGIT, 0}, {ITEMIS_FOLDER|ITEMIS_FOLDERINGIT, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Rebase,						TGitContextMenuEntries::Rebase,				IDI_REBASE,				IDS_MENUREBASE,				IDS_MENUREBASE,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::StashSave,					TGitContextMenuEntries::StashSave,			IDI_SHELVE,				IDS_MENUSTASHSAVE,			IDS_MENUSTASHSAVE,
		{ITEMIS_INGIT|ITEMIS_ONLYONE, ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::StashApply,					TGitContextMenuEntries::StashApply,			IDI_UNSHELVE,			IDS_MENUSTASHAPPLY,			IDS_MENUSTASHAPPLY,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE|ITEMIS_STASH, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::StashPop,					TGitContextMenuEntries::StashPop,			IDI_UNSHELVE,			IDS_MENUSTASHPOP,			IDS_MENUSTASHPOP,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE|ITEMIS_STASH, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::StashList,					TGitContextMenuEntries::StashList,			IDI_LOG,				IDS_MENUSTASHLIST,			IDS_MENUSTASHLIST,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE|ITEMIS_STASH, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::BisectStart,					TGitContextMenuEntries::Bisect,				IDI_BISECT,				IDS_MENUBISECTSTART,		IDS_MENUDESCBISECTSTART,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::BisectGood,					TGitContextMenuEntries::Bisect,				IDI_THUMB_UP,			IDS_MENUBISECTGOOD,			IDS_MENUDESCBISECTGOOD,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE|ITEMIS_BISECT, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::BisectBad,					TGitContextMenuEntries::Bisect,				IDI_THUMB_DOWN,			IDS_MENUBISECTBAD,			IDS_MENUDESCBISECTBAD,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE|ITEMIS_BISECT, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::BisectSkip,					TGitContextMenuEntries::Bisect,				IDI_BISECT,				IDS_MENUBISECTSKIP,			IDS_MENUDESCBISECTSKIP,
		{ ITEMIS_FOLDERINGIT | ITEMIS_ONLYONE | ITEMIS_BISECT, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 } },

	{ TGitShellCommand::BisectReset,					TGitContextMenuEntries::Bisect,				IDI_BISECT_RESET,		IDS_MENUBISECTRESET,		IDS_MENUDESCBISECTRESET,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE|ITEMIS_BISECT, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::ConflictEditor,				TGitContextMenuEntries::Conflicteditor,		IDI_CONFLICT,			IDS_MENUCONFLICT,			IDS_MENUDESCCONFLICT,
		{ITEMIS_INGIT|ITEMIS_CONFLICTED, ITEMIS_FOLDER}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Resolve,						TGitContextMenuEntries::Resolve,			IDI_RESOLVE,			IDS_MENURESOLVE,			IDS_MENUDESCRESOLVE,
		{ITEMIS_INGIT|ITEMIS_CONFLICTED, 0}, {ITEMIS_INGIT|ITEMIS_FOLDER, 0}, {ITEMIS_FOLDERINGIT, 0}, {0, 0} },

	{ TGitShellCommand::MergeAbort,					TGitContextMenuEntries::Merge,				IDI_MERGEABORT,			IDS_MENUMERGEABORT,			IDS_MENUDESCMERGEABORT,
		{ITEMIS_INGIT|ITEMIS_MERGEACTIVE, 0}, {ITEMIS_FOLDERINGIT|ITEMIS_MERGEACTIVE, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Rename,						TGitContextMenuEntries::Rename,				IDI_RENAME,				IDS_MENURENAME,				IDS_MENUDESCRENAME,
		{ITEMIS_INGIT|ITEMIS_ONLYONE|ITEMIS_INVERSIONEDFOLDER, ITEMIS_WCROOT}, {ITEMIS_WCROOT|ITEMIS_SUBMODULE, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Remove,						TGitContextMenuEntries::Remove,				IDI_DELETE,				IDS_MENUREMOVE,				IDS_MENUDESCREMOVE,
		{ITEMIS_INGIT|ITEMIS_INVERSIONEDFOLDER, ITEMIS_ADDED|ITEMIS_WCROOT}, {ITEMIS_FOLDERINGIT|ITEMIS_WCROOT|ITEMIS_SUBMODULE, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::RemoveKeep,					TGitContextMenuEntries::RemoveKeep,			IDI_DELETE,				IDS_MENUREMOVEKEEP,			IDS_MENUDESCREMOVEKEEP,
		{ITEMIS_INGIT|ITEMIS_INVERSIONEDFOLDER, ITEMIS_ADDED|ITEMIS_WCROOT}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Revert,						TGitContextMenuEntries::Revert,				IDI_REVERT,				IDS_MENUREVERT,				IDS_MENUDESCREVERT,
		{ITEMIS_INGIT, ITEMIS_NORMAL}, {ITEMIS_FOLDERINGIT, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Cleanup,						TGitContextMenuEntries::Cleanup,			IDI_CLEANUP,			IDS_MENUCLEANUP,			IDS_MENUDESCCLEANUP,
		{ITEMIS_FOLDERINGIT|ITEMIS_FOLDER, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::LFSMenu,						TGitContextMenuEntries::LFS,				IDI_LFS,				IDS_MENULFS,				IDS_MENUDESCLFS,
		{ITEMIS_INVERSIONEDFOLDER|ITEMIS_INGIT}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

//	{ TGitShellCommand::Copy,						TGitContextMenuEntries::Copy,				IDI_COPY,				IDS_MENUBRANCH,				IDS_MENUDESCCOPY,
//	ITEMIS_INGIT|ITEMIS_ONLYONE, ITEMIS_ADDED }, {ITEMIS_FOLDER|ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Switch,						TGitContextMenuEntries::Switch,				IDI_SWITCH,				IDS_MENUSWITCH,				IDS_MENUDESCSWITCH,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Merge,						TGitContextMenuEntries::Merge,				IDI_MERGE,				IDS_MENUMERGE,				IDS_MENUDESCMERGE,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, ITEMIS_BISECT|ITEMIS_MERGEACTIVE}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Branch,						TGitContextMenuEntries::Branch,				IDI_COPY,				IDS_MENUBRANCH,				IDS_MENUDESCCOPY,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::Tag,							TGitContextMenuEntries::Tag,				IDI_TAG,				IDS_MENUTAG,				IDS_MENUDESCCOPY,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Export,						TGitContextMenuEntries::Export,				IDI_EXPORT,				IDS_MENUEXPORT,				IDS_MENUDESCEXPORT,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {ITEMIS_BAREREPO, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::CreateRepo,					TGitContextMenuEntries::CreateRepo,			IDI_CREATEREPOS,		IDS_MENUCREATEREPOS,		IDS_MENUDESCCREATEREPOS,
		{ITEMIS_FOLDER, ITEMIS_INGIT|ITEMIS_FOLDERINGIT|ITEMIS_BAREREPO|ITEMIS_INACCESSIBLE}, {ITEMIS_FOLDER|ITEMIS_IGNORED, 0}, {ITEMIS_FOLDER|ITEMIS_EXTENDED, ITEMIS_INGIT}, {0, 0} },

	{ TGitShellCommand::Add,							TGitContextMenuEntries::Add,				IDI_ADD,				IDS_MENUADD,				IDS_MENUDESCADD,
		{ITEMIS_INVERSIONEDFOLDER, ITEMIS_INGIT}, {ITEMIS_INGIT|ITEMIS_FOLDER, 0}, {ITEMIS_IGNORED, 0}, {ITEMIS_DELETED, ITEMIS_FOLDER|ITEMIS_ONLYONE} },

	{ TGitShellCommand::Blame,						TGitContextMenuEntries::Blame,				IDI_BLAME,				IDS_MENUBLAME,				IDS_MENUDESCBLAME,
		{ITEMIS_INGIT|ITEMIS_ONLYONE, ITEMIS_FOLDER|ITEMIS_ADDED}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::IgnoreSub,					TGitContextMenuEntries::Ignore,				IDI_IGNORE,				IDS_MENUIGNORE,				IDS_MENUDESCIGNORE,
		{ITEMIS_INVERSIONEDFOLDER, ITEMIS_IGNORED|ITEMIS_INGIT|ITEMIS_WCROOT}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::DeleteIgnoreSub,				TGitContextMenuEntries::Ignore,				IDI_IGNORE,				IDS_MENUDELETEIGNORE,		IDS_MENUDESCDELETEIGNORE,
		{ITEMIS_INVERSIONEDFOLDER|ITEMIS_INGIT, ITEMIS_IGNORED|ITEMIS_WCROOT}, {0, 0}, {0, 0}, {0, 0} },

	// no support for this atm since we do not use "ignoredprops"-vector in ContextMenu.cpp
//	{ TGitShellCommand::UnIgnoreSub,					MENUIGNORE,			IDI_IGNORE,				IDS_MENUUNIGNORE,			IDS_MENUDESCUNIGNORE,
//		{ITEMIS_IGNORED, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Worktree,					TGitContextMenuEntries::Worktree,			IDI_COPY,				IDS_MENUWORKTREE,			IDS_MENUDESCWORKTREE,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0 }, {ITEMIS_BAREREPO, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::SubmoduleAdd,				TGitContextMenuEntries::SubmoduleAdd,		IDI_ADD,				IDS_MENUSUBADD,				IDS_MENUSUBADD,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::SubmoduleUpdate,				TGitContextMenuEntries::SubmoduleUpdate,	IDI_UPDATE,				IDS_MENUUPDATEEXT,			IDS_MENUDESCUPDATEEXT,
		{ITEMIS_FOLDERINGIT|ITEMIS_SUBMODULECONTAINER, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::SubmoduleSync,				TGitContextMenuEntries::SubmoduleSync,		IDI_MENUSYNC,			IDS_MENUSUBSYNC,			IDS_MENUSUBSYNC,
		{ITEMIS_FOLDERINGIT|ITEMIS_SUBMODULECONTAINER, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::FormatPatch,					TGitContextMenuEntries::FormatPatch,		IDI_CREATEPATCH,		IDS_MENUFORMATPATCH,		IDS_MENUDESCCREATEPATCH,
		{ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0}, {0, 0} },

	// Really apply patch
	{ TGitShellCommand::ImportPatch,					TGitContextMenuEntries::ImportPatch,		IDI_PATCH,				IDS_MENUIMPORTPATCH,		IDS_MENUDESCIMPORTPATCH,
		{ITEMIS_PATCHFILE, 0}, {ITEMIS_FOLDERINGIT|ITEMIS_ONLYONE, 0}, {0, 0}, {0, 0} },

	// Review Patch
	{ TGitShellCommand::ApplyPatch,					TGitContextMenuEntries::ApplyPatch,			IDI_PATCH,				IDS_MENUAPPLYPATCH,			IDS_MENUDESCAPPLYPATCH,
		{ITEMIS_PATCHFILE|ITEMIS_ONLYONE, 0}, {ITEMIS_EXTENDED|ITEMIS_ONLYONE, ITEMIS_FOLDER}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Sendmail,					TGitContextMenuEntries::Sendmail,			IDI_MENUSENDMAIL,		IDS_MENUSENDMAIL,			IDS_MENUDESSENDMAIL,
		{ITEMIS_PATCHFILE, 0}, {ITEMIS_EXTENDED, ITEMIS_FOLDER}, {0, 0}, {0, 0} },

// we do not support paste atm
//	{ ShellSeparator, 0, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },
//	{ TGitShellCommand::ClipPaste,					MENUCLIPPASTE,		IDI_CLIPPASTE,			IDS_MENUCLIPPASTE,			IDS_MENUDESCCLIPPASTE,
//		{ITEMIS_INGIT|ITEMIS_FOLDER|ITEMIS_PATHINCLIPBOARD, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Separator, TGitContextMenuEntries::None, 0, 0, 0, {0, 0}, {0, 0}, {0, 0}, {0, 0} },

	{ TGitShellCommand::Settings,					TGitContextMenuEntries::Settings,			IDI_SETTINGS,			IDS_MENUSETTINGS,			IDS_MENUDESCSETTINGS,
		{ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0} },
	{ TGitShellCommand::Help,						TGitContextMenuEntries::Help,				IDI_HELP,				IDS_MENUHELP,				IDS_MENUDESCHELP,
		{ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0} },
	{ TGitShellCommand::About,						TGitContextMenuEntries::About,				IDI_ABOUT,				IDS_MENUABOUT,				IDS_MENUDESCABOUT,
		{ITEMIS_FOLDER, 0}, {0, ITEMIS_FOLDER}, {0, 0}, {0, 0} },

	// the sub menus - they're not added like the commands, therefore the menu ID is zero
	// but they still need to be in here, because we use the icon and string information anyway.
	{ TGitShellCommand::SubMenu,					TGitContextMenuEntries::None,				IDI_APP,				IDS_MENUSUBMENU,			0,
		{0, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::SubMenuFile,						TGitContextMenuEntries::None,				IDI_MENUFILE,			IDS_MENUSUBMENU,			0,
		{0, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::SubMenuFolder,					TGitContextMenuEntries::None,				IDI_MENUFOLDER,			IDS_MENUSUBMENU,			0,
		{0, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::SubMenuLink,						TGitContextMenuEntries::None,				IDI_MENULINK,			IDS_MENUSUBMENU,			0,
		{0, 0}, {0, 0}, {0, 0}, {0, 0} },
	{ TGitShellCommand::SubMenuMultiple,					TGitContextMenuEntries::None,				IDI_MENUMULTIPLE,		IDS_MENUSUBMENU,			0,
		{0, 0}, {0, 0}, {0, 0}, {0, 0} },
};
static_assert(std::span<const MenuInfo>(menuInfo).size() < INT_MAX, "must and will never happen, but we cast size() to int at some places");

std::span<const MenuInfo> GetTGitMenuInfo() noexcept
{
	return menuInfo;
}
