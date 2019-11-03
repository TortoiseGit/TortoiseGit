﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2016, 2018-2019 - TortoiseGit
// Copyright (C) 2003-2006,2008-2010, 2014 - TortoiseSVN

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
#include "registry.h"

//////////////////////////////////////////////////////////////////////////////////////////////

#if defined(__CSTRINGT_H__) || defined(__ATLSTR_H__)
CRegBase::CRegBase()
{
}

CRegBase::CRegBase (const CString& key, bool force, HKEY base, REGSAM sam)
	: CRegBaseCommon<CString> (key, force, base, sam)
{
	m_key.TrimLeft(L'\\');
	int backslashpos = m_key.ReverseFind('\\');
	m_path = m_key.Left(backslashpos);
	m_path.TrimRight(L'\\');
	m_key = m_key.Mid(backslashpos);
	m_key.Trim(L'\\');
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////

CRegStdBase::CRegStdBase()
{
}

CRegStdBase::CRegStdBase (const tstring& key, bool force, HKEY base, REGSAM sam)
	: CRegBaseCommon<tstring> (key, force, base, sam)
{
	tstring::size_type pos = key.find_last_of(L'\\');
	m_path = key.substr(0, pos);
	m_key = key.substr(pos + 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __ATLTYPES_H__   // defines CRect
CRegRect::CRegRect(void)
	: CRegTypedBase<CRect, CRegBase>(CRect(0,0,0,0))
{
}

CRegRect::CRegRect(const CString& key, const CRect& def, bool force, HKEY base, REGSAM sam)
	: CRegTypedBase<CRect, CRegBase> (key, def, force, base, sam)
{
	read();
}

void CRegRect::InternalRead (HKEY hKey, CRect& value)
{
	DWORD size = 0;
	DWORD type = 0;
	LastError = RegQueryValueEx(hKey, m_key, nullptr, &type, nullptr, &size);

	if (LastError == ERROR_SUCCESS)
	{
		auto buffer = std::make_unique<char[]>(size);
		if ((LastError = RegQueryValueEx(hKey, m_key, nullptr, &type, reinterpret_cast<BYTE*>(buffer.get()), &size)) == ERROR_SUCCESS)
		{
			ASSERT(type==REG_BINARY);
			value = CRect(reinterpret_cast<LPRECT>(buffer.get()));
		}
	}
}

void CRegRect::InternalWrite (HKEY hKey, const CRect& value)
{
	LastError = RegSetValueEx(hKey, m_key, 0, REG_BINARY, reinterpret_cast<const BYTE*>(static_cast<LPCRECT>(value)), sizeof(value));
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __ATLTYPES_H__   // defines CPoint
CRegPoint::CRegPoint(void)
	: CRegTypedBase<CPoint, CRegBase>(CPoint(0,0))
{
}

CRegPoint::CRegPoint(const CString& key, const CPoint& def, bool force, HKEY base, REGSAM sam)
	: CRegTypedBase<CPoint, CRegBase> (key, def, force, base, sam)
{
	read();
}

void CRegPoint::InternalRead (HKEY hKey, CPoint& value)
{
	DWORD size = 0;
	DWORD type = 0;
	LastError = RegQueryValueEx(hKey, m_key, nullptr, &type, nullptr, &size);

	if (LastError == ERROR_SUCCESS)
	{
		auto buffer = std::make_unique<char[]>(size);
		if ((LastError = RegQueryValueEx(hKey, m_key, nullptr, &type, reinterpret_cast<BYTE*>(buffer.get()), &size)) == ERROR_SUCCESS)
		{
			ASSERT(type==REG_BINARY);
			value = CPoint(*reinterpret_cast<POINT*>(buffer.get()));
		}
	}
}

void CRegPoint::InternalWrite (HKEY hKey, const CPoint& value)
{
	LastError = RegSetValueEx(hKey, m_key, 0, REG_BINARY, reinterpret_cast<const BYTE*>(&value), sizeof(value));
}
#endif

/////////////////////////////////////////////////////////////////////

#ifdef __AFXCOLL_H__   // defines CStringList
CRegistryKey::CRegistryKey(const CString& key, HKEY base, REGSAM sam)
{
	m_base = base;
	m_hKey = nullptr;
	m_sam = sam;
	m_path = key;
	m_path.TrimLeft(L'\\');
}

CRegistryKey::~CRegistryKey()
{
	if (m_hKey)
		RegCloseKey(m_hKey);
}

DWORD CRegistryKey::createKey()
{
	DWORD disp;
	DWORD rc = RegCreateKeyEx(m_base, m_path, 0, L"", REG_OPTION_NON_VOLATILE, KEY_WRITE | m_sam, nullptr, &m_hKey, &disp);
	if (rc != ERROR_SUCCESS)
		return rc;
	return RegCloseKey(m_hKey);
}

DWORD CRegistryKey::removeKey()
{
	RegOpenKeyEx(m_base, m_path, 0, KEY_WRITE|m_sam, &m_hKey);
	return SHDeleteKey(m_base, static_cast<LPCTSTR>(m_path));
}

bool CRegistryKey::getValues(CStringList& values)
{
	values.RemoveAll();

	if (RegOpenKeyEx(m_base, m_path, 0, KEY_EXECUTE|m_sam, &m_hKey)==ERROR_SUCCESS)
	{
		for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; ++i)
		{
			TCHAR value[255] = { 0 };
			DWORD size = _countof(value);
			rc = RegEnumValue(m_hKey, i, value, &size, nullptr, nullptr, nullptr, nullptr);
			if (rc == ERROR_SUCCESS)
				values.AddTail(value);
		}
	}

	return !values.IsEmpty();
}

bool CRegistryKey::getSubKeys(CStringList& subkeys)
{
	subkeys.RemoveAll();

	if (RegOpenKeyEx(m_base, m_path, 0, KEY_EXECUTE|m_sam, &m_hKey)==ERROR_SUCCESS)
	{
		for (int i = 0, rc = ERROR_SUCCESS; rc == ERROR_SUCCESS; ++i)
		{
			TCHAR value[1024] = { 0 };
			DWORD size = _countof(value);
			FILETIME last_write_time;
			rc = RegEnumKeyEx(m_hKey, i, value, &size, nullptr, nullptr, nullptr, &last_write_time);
			if (rc == ERROR_SUCCESS)
				subkeys.AddTail(value);
		}
	}

	return !subkeys.IsEmpty();
}
#endif
