// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2022, 2026 - TortoiseGit

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
#include <span>
#include "Globals.h"

enum class TGitShellCommand
{
		Separator = 0,
		SubMenu = 1,
		SubMenuFolder,
		SubMenuFile,
		SubMenuLink,
		SubMenuMultiple,
		Commit,
		Add,
		Revert,
		Cleanup,
		Resolve,
		Switch,
		Export,
		About,
		CreateRepo,
		Copy,
		Merge,
		Settings,
		Remove,
		RemoveKeep,
		Rename,
		SubmoduleUpdate,
		Diff,
		PrevDiff,
		DiffTwo,
		DropCopyAdd,
		DropMoveAdd,
		DropMove,
		DropMoveRename,
		DropCopy,
		DropCopyRename,
		DropExport,
		DropExportExtended,
		Log,
		ConflictEditor,
		Help,
		ShowChanged,
		IgnoreSub,
		DeleteIgnoreSub,
		Ignore,
		DeleteIgnore,
		IgnoreCaseSensitive,
		DeleteIgnoreCaseSensitive,
		RefLog,
		RefBrowser,
		Blame,
		ApplyPatch,
		RevisionGraph,
		UnIgnoreSub,
		UnIgnoreCaseSensitive,
		UnIgnore,
		ClipPaste,
		Pull,
		Push,
		Clone,
		Branch,
		Tag,
		FormatPatch,
		ImportPatch,
		Fetch,
		Rebase,
		StashSave,
		StashApply,
		StashList,
		StashPop,
		SubmoduleAdd,
		SubmoduleSync,
		Sendmail,
		SVNRebase,
		SVNDCommit,
		SVNDFetch,
		SVNIgnore, //import svn ignore
		Sync,
		BisectStart,
		BisectGood,
		BisectBad,
		BisectReset,
		RepoBrowse,
		LogSubmoduleFolder,
		Daemon,
		MergeAbort,
		DiffLater,
		ImportPatchDrop,
		BisectSkip,
		LFSMenu,
		LFSLocks,
		LFSLock,
		LFSUnlock,
		Worktree,
		DropNewWorktree,
		Inaccessible,
};

	// helper struct for context menu entries
struct YesNoPair
{
		DWORD				yes;
		DWORD				no;
};
struct MenuInfo
{
		TGitShellCommand	command;		///< the command which gets executed for this menu entry
		TGitContextMenuEntries	menuID;			///< the menu ID to recognize the entry. None if it shouldn't be added to the context menu automatically
		UINT				iconID;			///< the icon to show for the menu entry
		UINT				menuTextID;		///< the text of the menu entry
		UINT				menuDescID;		///< the description text for the menu entry
		/// the following 8 params are for checking whether the menu entry should
		/// be added automatically, based on states of the selected item(s).
		/// The 'yes' states must be set, the 'no' states must not be set
		/// the four pairs are OR'ed together, the 'yes'/'no' states are AND'ed together.
		YesNoPair			first;
		YesNoPair			second;
		YesNoPair			third;
		YesNoPair			fourth;
};
std::span<const MenuInfo> GetTGitMenuInfo() noexcept;
