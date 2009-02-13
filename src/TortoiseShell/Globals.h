// TortoiseSVN - a Windows shell extension for easy version control

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

//#define MENUCHECKOUT		0x0000000000000001
//#define MENUUPDATE		0x0000000000000002
#define MENUCOMMIT			0x0000000000000004
#define MENUADD				0x0000000000000008
#define MENUREVERT			0x0000000000000010
#define MENUCLEANUP			0x0000000000000020
//#define MENURESOLVE		0x0000000000000040
#define MENUSWITCH			0x0000000000000080
//#define MENUIMPORT		0x0000000000000100
#define MENUEXPORT			0x0000000000000200
#define MENUCREATEREPOS		0x0000000000000400
#define MENUCOPY			0x0000000000000800
#define MENUMERGE			0x0000000000001000
#define MENUREMOVE			0x0000000000002000
#define MENURENAME			0x0000000000004000
//#define MENUUPDATEEXT		0x0000000000008000
#define MENUDIFF			0x0000000000010000
#define MENULOG				0x0000000000020000
#define MENUCONFLICTEDITOR	0x0000000000040000
//#define MENURELOCATE		0x0000000000080000
#define MENUSHOWCHANGED		0x0000000000100000
#define MENUIGNORE			0x0000000000200000
//#define MENUREPOBROWSE	0x0000000000400000
#define MENUBLAME			0x0000000000800000
#define MENUCREATEPATCH		0x0000000001000000
#define MENUAPPLYPATCH		0x0000000002000000
//#define MENUREVISIONGRAPH	0x0000000004000000
//#define MENULOCK			0x0000000008000000
//#define MENUUNLOCK		0x0000000010000000
#define MENUPROPERTIES		0x0000000020000000
//#define MENUURLDIFF		0x0000000040000000
#define MENUDELUNVERSIONED	0x0000000080000000
#define MENUMERGEALL		0x0000000100000000
#define MENUPREVDIFF		0x0000000200000000
//#define MENUCLIPPASTE		0x0000000400000000
#define MENUPULL			0x0000000800000000
#define MENUPUSH			0x0000001000000000
#define MENUCLONE           0x0000002000000000
#define MENUTAG				0x0000004000000000
#define MENUFORMATPATCH		0x0000008000000000
#define MENUIMPORTPATCH		0x0000010000000000
#define MENUCHERRYPICK		0x0000020000000000
#define MENUFETCH			0x0000040000000000


#define MENUSETTINGS		0x2000000000000000
#define MENUHELP			0x4000000000000000
#define MENUABOUT			0x8000000000000000

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
#define ITEMIS_INSVN				0x00000004
#define ITEMIS_CONFLICTED			0x00000008
#define ITEMIS_FOLDER				0x00000010
#define ITEMIS_FOLDERINSVN			0x00000020
#define ITEMIS_NORMAL				0x00000040
#define ITEMIS_IGNORED				0x00000080
#define ITEMIS_INVERSIONEDFOLDER	0x00000100
#define ITEMIS_ADDED				0x00000200
#define ITEMIS_DELETED				0x00000400
#define ITEMIS_LOCKED				0x00000800
#define ITEMIS_PATCHFILE			0x00001000
// #define ITEMIS_SHORTCUT			0x00002000 //unused
#define ITEMIS_NEEDSLOCK			0x00004000
#define ITEMIS_PATCHINCLIPBOARD		0x00008000
#define ITEMIS_PATHINCLIPBOARD      0x00010000
#define ITEMIS_TWO					0x00020000

