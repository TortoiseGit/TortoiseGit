// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012, 2019-2022, 2026 - TortoiseGit
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
#pragma once

enum class TGitContextMenuEntries : unsigned __int64
{
	None			= 0x0000000000000000,
	Sync			= 0x0000000000000002,
	Commit			= 0x0000000000000004,
	Add				= 0x0000000000000008,
	Revert			= 0x0000000000000010,
	Cleanup			= 0x0000000000000020,
	Resolve			= 0x0000000000000040,
	Switch			= 0x0000000000000080,
	Sendmail		= 0x0000000000000100,
	Export			= 0x0000000000000200,
	CreateRepo		= 0x0000000000000400,
	Branch			= 0x0000000000000800,
	Merge			= 0x0000000000001000,
	Remove			= 0x0000000000002000,
	Rename			= 0x0000000000004000,
	SubmoduleUpdate	= 0x0000000000008000,
	Diff			= 0x0000000000010000,
	Log				= 0x0000000000020000,
	Conflicteditor	= 0x0000000000040000,
	RefBrowser		= 0x0000000000080000,
	ShowChanged		= 0x0000000000100000,
	Ignore			= 0x0000000000200000,
	RefLog			= 0x0000000000400000,
	Blame			= 0x0000000000800000,
	RepoBrowser		= 0x0000000001000000,
	ApplyPatch		= 0x0000000002000000,
	RemoveKeep		= 0x0000000004000000,
	SVNRebase		= 0x0000000008000000,
	SVNDCommit		= 0x0000000010000000,
	SVNIgnore		= 0x0000000040000000,
	LogSubmodule	= 0x0000000100000000,
	PrevDiff		= 0x0000000200000000,
	ClipPaste		= 0x0000000400000000,
	Pull			= 0x0000000800000000,
	Push			= 0x0000001000000000,
	Clone			= 0x0000002000000000,
	Tag				= 0x0000004000000000,
	FormatPatch		= 0x0000008000000000,
	ImportPatch		= 0x0000010000000000,
	DiffLater		= 0x0000020000000000,
	Fetch			= 0x0000040000000000,
	Rebase			= 0x0000080000000000,
	StashSave		= 0x0000100000000000,
	StashApply		= 0x0000200000000000,
	StashList		= 0x0000400000000000,
	SubmoduleAdd	= 0x0000800000000000,
	SubmoduleSync	= 0x0001000000000000,
	StashPop		= 0x0002000000000000,
	DiffTwo			= 0x0004000000000000,
	Bisect			= 0x0008000000000000,
	Inaccessible	= 0x0010000000000000,
	SVNFetch		= 0x0080000000000000,
	RevisionGraph	= 0x0100000000000000,
	Daemon			= 0x0200000000000000,
	Worktree		= 0x0400000000000000,
	LFS				= 0x1000000000000000,
	Settings		= 0x2000000000000000,
	Help			= 0x4000000000000000,
	About			= 0x8000000000000000,
};

constexpr inline TGitContextMenuEntries operator|(TGitContextMenuEntries a, TGitContextMenuEntries b) noexcept
{
	return static_cast<TGitContextMenuEntries>(static_cast<std::underlying_type_t<TGitContextMenuEntries>>(a) | static_cast<std::underlying_type_t<TGitContextMenuEntries>>(b));
}
constexpr inline TGitContextMenuEntries operator&(TGitContextMenuEntries a, TGitContextMenuEntries b) noexcept
{
	return static_cast<TGitContextMenuEntries>(static_cast<std::underlying_type_t<TGitContextMenuEntries>>(a) & static_cast<std::underlying_type_t<TGitContextMenuEntries>>(b));
}
constexpr inline TGitContextMenuEntries& operator|=(TGitContextMenuEntries& self, TGitContextMenuEntries a) noexcept
{
	self = self | a;
	return self;
}

template <typename E>
constexpr E from_underlying(std::underlying_type_t<E> value) noexcept
{
	static_assert(std::is_enum_v<E>);
	return static_cast<E>(value);
}

constexpr TGitContextMenuEntries to_TGitContextMenuEntries(DWORD high, DWORD low) noexcept
{
	static_assert(sizeof(std::underlying_type_t<TGitContextMenuEntries>) == 2 * sizeof(DWORD));
	ULARGE_INTEGER tmp;
	tmp.HighPart = high;
	tmp.LowPart = low;
	return from_underlying<TGitContextMenuEntries>(tmp.QuadPart);
}

/**
 * \ingroup TortoiseShell
 * Since we need an own COM-object for every different
 * Icon-Overlay implemented this enum defines which class
 * is used.
 */
enum FileState
{
	FileStateUncontrolled,
	FileStateVersioned,
	FileStateModified,
	FileStateConflict,
	FileStateDeleted,
	FileStateReadOnly,
	FileStateLockedOverlay,
	FileStateAddedOverlay,
	FileStateIgnoredOverlay,
	FileStateUnversionedOverlay,
	FileStateDropHandler,
	FileStateInvalid
};

#define ITEMIS_ONLYONE				0x00000001
#define ITEMIS_EXTENDED				0x00000002
#define ITEMIS_INGIT				0x00000004
#define ITEMIS_CONFLICTED			0x00000008
#define ITEMIS_FOLDER				0x00000010
#define ITEMIS_FOLDERINGIT			0x00000020
#define ITEMIS_NORMAL				0x00000040
#define ITEMIS_IGNORED				0x00000080
#define ITEMIS_INVERSIONEDFOLDER	0x00000100
#define ITEMIS_ADDED				0x00000200
#define ITEMIS_DELETED				0x00000400
#define ITEMIS_PATCHFILE			0x00001000
// #define ITEMIS_SHORTCUT			0x00002000 //unused
#define ITEMIS_PATCHINCLIPBOARD		0x00008000
#define ITEMIS_PATHINCLIPBOARD      0x00010000
#define ITEMIS_TWO					0x00020000
#define ITEMIS_SUBMODULECONTAINER	0x00040000
#define ITEMIS_GITSVN				0x00080000
#define ITEMIS_STASH				0x00100000
#define ITEMIS_WCROOT				0x00200000
#define ITEMIS_BISECT				0x00400000
#define ITEMIS_BAREREPO				0x00800000
#define ITEMIS_SUBMODULE			0x01000000
#define ITEMIS_MERGEACTIVE			0x02000000
#define ITEMIS_HASDIFFLATER         0x04000000
#define ITEMIS_INACCESSIBLE			0x08000000
