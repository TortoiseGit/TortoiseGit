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
