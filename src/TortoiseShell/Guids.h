// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - Stefan Kueng
// Copyright (C) 2015 TortoiseSI

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


// The class IDs of these Shell extension class.
//
// class ids:
//
// TODO update?
// 30351346-7B7D-4FCC-81B4-1E394CA267EB
// 30351347-7B7D-4FCC-81B4-1E394CA267EB
// 30351348-7B7D-4FCC-81B4-1E394CA267EB
// 30351349-7B7D-4FCC-81B4-1E394CA267EB
// 3035134A-7B7D-4FCC-81B4-1E394CA267EB
// 3035134B-7B7D-4FCC-81B4-1E394CA267EB
// 3035134C-7B7D-4FCC-81B4-1E394CA267EB
// 3035134D-7B7D-4FCC-81B4-1E394CA267EB
// 3035134E-7B7D-4FCC-81B4-1E394CA267EB
// 3035134F-7B7D-4FCC-81B4-1E394CA267EB
//
//
// NOTE!!!  If you use this shell extension as a starting point,
//          you MUST change the GUID below.  Simply run UUIDGEN.EXE
//          to generate a new GUID.
//

// {3F496DF1-A773-4730-A267-DFEC451C640F}
const CLSID CLSID_TortoiseSI_UPTODATE = 
{ 0x3f496df1, 0xa773, 0x4730, { 0xa2, 0x67, 0xdf, 0xec, 0x45, 0x1c, 0x64, 0xf } };

// {57E5A936-19F4-4709-9F96-047DBEF1A9CA}
const CLSID CLSID_TortoiseSI_MODIFIED =
{ 0x57e5a936, 0x19f4, 0x4709, { 0x9f, 0x96, 0x4, 0x7d, 0xbe, 0xf1, 0xa9, 0xca } };

// {4E4FFC8D-FC20-4640-8CC6-527393BEACD8}
const CLSID CLSID_TortoiseSI_CONFLICTING = 
{ 0x4e4ffc8d, 0xfc20, 0x4640, { 0x8c, 0xc6, 0x52, 0x73, 0x93, 0xbe, 0xac, 0xd8 } };

// {54B31F7A-5780-4C5E-BB2A-C62103E4EAD2}
const CLSID CLSID_TortoiseSI_UNCONTROLLED =
{ 0x54b31f7a, 0x5780, 0x4c5e, { 0xbb, 0x2a, 0xc6, 0x21, 0x3, 0xe4, 0xea, 0xd2 } };

// {39049ADF-D3BF-43C2-988D-6F67BD6D57A2}
const CLSID CLSID_TortoiseSI_DROPHANDLER =
{ 0x39049adf, 0xd3bf, 0x43c2, { 0x98, 0x8d, 0x6f, 0x67, 0xbd, 0x6d, 0x57, 0xa2 } };

// {EF48F555-C840-4C95-80B2-85BF62F416C6}
const CLSID CLSID_TortoiseSI_READONLY =
{ 0xef48f555, 0xc840, 0x4c95, { 0x80, 0xb2, 0x85, 0xbf, 0x62, 0xf4, 0x16, 0xc6 } };

// {627BDF38-20FF-4926-8051-3E69682DB3BC}
const CLSID CLSID_TortoiseSI_DELETED =
{ 0x627bdf38, 0x20ff, 0x4926, { 0x80, 0x51, 0x3e, 0x69, 0x68, 0x2d, 0xb3, 0xbc } };

// {331E54FC-D086-4711-8936-F8B97C2C5996}
const CLSID CLSID_TortoiseSI_LOCKED =
{ 0x331e54fc, 0xd086, 0x4711, { 0x89, 0x36, 0xf8, 0xb9, 0x7c, 0x2c, 0x59, 0x96 } };

// {FE682D2F-95BB-4599-8F25-EE120722D397}
const CLSID CLSID_TortoiseSI_ADDED = 
{ 0xfe682d2f, 0x95bb, 0x4599, { 0x8f, 0x25, 0xee, 0x12, 0x7, 0x22, 0xd3, 0x97 } };

// {03F3F1CE-E2C7-4A67-B9C1-3FF7979DB0F5}
const CLSID CLSID_TortoiseSI_IGNORED =
{ 0x3f3f1ce, 0xe2c7, 0x4a67, { 0xb9, 0xc1, 0x3f, 0xf7, 0x97, 0x9d, 0xb0, 0xf5 } };

// {AC3F5A7A-6FBC-41A7-9B38-7ACC20EF0B3C}
const CLSID CLSID_TortoiseSI_UNVERSIONED =
{ 0xac3f5a7a, 0x6fbc, 0x41a7, { 0x9b, 0x38, 0x7a, 0xcc, 0x20, 0xef, 0xb, 0x3c } };
