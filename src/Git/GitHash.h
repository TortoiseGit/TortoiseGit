// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#if defined(_MFC_VER)
#include "afx.h"
#endif
#define GIT_HASH_SIZE 20

class CGitHash
{
public:
	unsigned char m_hash[GIT_HASH_SIZE];

	CGitHash()
	{
		memset(m_hash,0, GIT_HASH_SIZE);
	}
	CGitHash(char *p)
	{
		memcpy(m_hash,p,GIT_HASH_SIZE);
	}
	CGitHash & operator = (CString &str)
	{
		CGitHash hash(str);
		*this = hash;
		return *this;
	}
	CGitHash(CString &str)
	{
		if (!IsValidSHA1(str))
		{
#ifdef ASSERT
			ASSERT(FALSE);
#endif
			return;
		}

		for(int i=0;i<GIT_HASH_SIZE;i++)
		{
			unsigned char a;
			a=0;
			for(int j=2*i;j<=2*i+1;j++)
			{
				a =a<<4;

					TCHAR ch = str[j];
				if(ch >= _T('0') && ch <= _T('9'))
					a |= (ch - _T('0'))&0xF;
				else if(ch >=_T('A') && ch <= _T('F'))
					a |= ((ch - _T('A'))&0xF) + 10 ;
				else if(ch >=_T('a') && ch <= _T('f'))
					a |= ((ch - _T('a'))&0xF) + 10;

			}
			m_hash[i]=a;
		}
	}

	void ConvertFromStrA(char *str)
	{
		for(int i=0;i<GIT_HASH_SIZE;i++)
		{
			unsigned char a;
			a=0;
			for(int j=2*i;j<=2*i+1;j++)
			{
				a =a<<4;

				char ch = str[j];
				if(ch >= '0' && ch <= '9')
					a |= (ch - ('0'))&0xF;
				else if(ch >=('A') && ch <= ('F'))
					a |= ((ch - ('A'))&0xF) + 10 ;
				else if(ch >=_T('a') && ch <= ('f'))
					a |= ((ch - ('a'))&0xF) + 10;

			}
			m_hash[i]=a;
		}
	}
	void Empty()
	{
		memset(m_hash,0, GIT_HASH_SIZE);
	}
	bool IsEmpty()
	{
		for(int i=0;i<GIT_HASH_SIZE;i++)
		{
			if(m_hash[i] != 0)
				return false;
		}
		return true;
	}

	CString ToString()
	{
		CString str;
		CString a;
		for(int i=0;i<GIT_HASH_SIZE;i++)
		{
			a.Format(_T("%02x"),m_hash[i]);
			str+=a;
		}
		return str;
	}
	operator CString ()
	{
		return ToString();
	}

	bool operator == (const CGitHash &hash)
	{
		return memcmp(m_hash,hash.m_hash,GIT_HASH_SIZE) == 0;
	}


	friend bool operator<(const CGitHash& left, const CGitHash& right)
	{
		return memcmp(left.m_hash,right.m_hash,GIT_HASH_SIZE) < 0;
	}

	friend bool operator>(const CGitHash& left, const CGitHash& right)
	{
		return memcmp(left.m_hash, right.m_hash, GIT_HASH_SIZE) > 0;
	}

	friend bool operator != (const CGitHash& left, const CGitHash& right)
	{
		return memcmp(left.m_hash, right.m_hash, GIT_HASH_SIZE) != 0;
	}
#if defined(_MFC_VER)
	friend CArchive& AFXAPI operator<<(CArchive& ar, CGitHash& hash)
	{
		for(int i=0;i<GIT_HASH_SIZE;i++)
			ar<<hash.m_hash[i];
		return ar;
	}
	friend CArchive& AFXAPI operator>>(CArchive& ar, CGitHash& hash)
	{
		for(int i=0;i<GIT_HASH_SIZE;i++)
			ar>>hash.m_hash[i];
		return ar;
	}
#endif
	static bool IsValidSHA1(const CString &possibleSHA1)
	{
		if (possibleSHA1.GetLength() != 2 * GIT_HASH_SIZE)
			return false;
		for (int i = 0; i < possibleSHA1.GetLength(); i++)
		{
			if (!((possibleSHA1[i] >= '0' && possibleSHA1[i] <= '9') || (possibleSHA1[i] >= 'a' && possibleSHA1[i] <= 'f') || (possibleSHA1[i] >= 'A' && possibleSHA1[i] <= 'F')))
				return false;
		}
		return true;
	}
};

