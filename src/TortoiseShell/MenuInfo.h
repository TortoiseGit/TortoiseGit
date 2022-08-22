// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2022 - TortoiseGit

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

enum GitCommands
{
		ShellSeparator = 0,
		ShellSubMenu = 1,
		ShellSubMenuFolder,
		ShellSubMenuFile,
		ShellSubMenuLink,
		ShellSubMenuMultiple,
		ShellMenuCommit,
		ShellMenuAdd,
		ShellMenuRevert,
		ShellMenuCleanup,
		ShellMenuResolve,
		ShellMenuSwitch,
		ShellMenuExport,
		ShellMenuAbout,
		ShellMenuCreateRepos,
		ShellMenuCopy,
		ShellMenuMerge,
		ShellMenuSettings,
		ShellMenuRemove,
		ShellMenuRemoveKeep,
		ShellMenuRename,
		ShellMenuUpdateExt,
		ShellMenuDiff,
		ShellMenuPrevDiff,
		ShellMenuDiffTwo,
		ShellMenuDropCopyAdd,
		ShellMenuDropMoveAdd,
		ShellMenuDropMove,
		ShellMenuDropMoveRename,
		ShellMenuDropCopy,
		ShellMenuDropCopyRename,
		ShellMenuDropExport,
		ShellMenuDropExportExtended,
		ShellMenuLog,
		ShellMenuConflictEditor,
		ShellMenuHelp,
		ShellMenuShowChanged,
		ShellMenuIgnoreSub,
		ShellMenuDeleteIgnoreSub,
		ShellMenuIgnore,
		ShellMenuDeleteIgnore,
		ShellMenuIgnoreCaseSensitive,
		ShellMenuDeleteIgnoreCaseSensitive,
		ShellMenuRefLog,
		ShellMenuRefBrowse,
		ShellMenuBlame,
		ShellMenuApplyPatch,
		ShellMenuRevisionGraph,
		ShellMenuUnIgnoreSub,
		ShellMenuUnIgnoreCaseSensitive,
		ShellMenuUnIgnore,
		ShellMenuClipPaste,
		ShellMenuPull,
		ShellMenuPush,
		ShellMenuClone,
		ShellMenuBranch,
		ShellMenuTag,
		ShellMenuFormatPatch,
		ShellMenuImportPatch,
		ShellMenuFetch,
		ShellMenuRebase,
		ShellMenuStashSave,
		ShellMenuStashApply,
		ShellMenuStashList,
		ShellMenuStashPop,
		ShellMenuSubAdd,
		ShellMenuSubSync,
		ShellMenuSendMail,
		ShellMenuGitSVNRebase,
		ShellMenuGitSVNDCommit,
		ShellMenuGitSVNDFetch,
		ShellMenuGitSVNIgnore,      //import svn ignore
		ShellMenuSync,
		ShellMenuBisectStart,
		ShellMenuBisectGood,
		ShellMenuBisectBad,
		ShellMenuBisectReset,
		ShellMenuRepoBrowse,
		ShellMenuLogSubmoduleFolder,
		ShellMenuDaemon,
		ShellMenuMergeAbort,
		ShellMenuDiffLater,
		ShellMenuImportPatchDrop,
		ShellMenuBisectSkip,
		ShellMenuLFSMenu,
		ShellMenuLFSLocks,
		ShellMenuLFSLock,
		ShellMenuLFSUnlock,
		ShellMenuWorktree,
		ShellMenuDropNewWorktree,
		ShellMenuLastEntry			// used to mark the menu array end
};

	// helper struct for context menu entries
struct YesNoPair
{
		DWORD				yes;
		DWORD				no;
};
struct MenuInfo
{
		GitCommands			command;		///< the command which gets executed for this menu entry
		unsigned __int64	menuID;			///< the menu ID to recognize the entry. NULL if it shouldn't be added to the context menu automatically
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
