// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2010 - TortoiseSVN

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
#include "Win7.h"
#include <initguid.h>
#include <propkeydef.h>
#if (NTDDI_VERSION < 0x06010000)

#define INITGUID
DEFINE_GUID(CLSID_EnumerableObjectCollection,0x2d3468c1,0x36a7,0x43b6,0xac,0x24,0xd3,0xf0,0x2F,0xd9,0x60,0x7a);
DEFINE_GUID(CLSID_DestinationList,0x77f10cf0,0x3db5,0x4966,0xb5,0x20,0xb7,0xc5,0x4f,0xd3,0x5e,0xd6);
DEFINE_PROPERTYKEY(PKEY_Title, 0xF29F85E0, 0x4FF9, 0x1068, 0xAB, 0x91, 0x08, 0x00, 0x2B, 0x27, 0xB3, 0xD9, 2);
DEFINE_PROPERTYKEY(PKEY_AppUserModel_IsDestListSeparator, 0x9F4C2855, 0x9F79, 0x4B39, 0xA8, 0xD0, 0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3, 6);

DEFINE_GUID(FOLDERTYPEID_SVNWC,       0xC1D29ED1, 0xCC8B, 0x4790, 0xA3, 0x45, 0xEC, 0x87, 0xDE, 0x96, 0xE9, 0x76);

#endif /* (NTDDI_VERSION < NTDDI_WIN7) */
