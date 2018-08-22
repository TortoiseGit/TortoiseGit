// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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

class CBstrSafeVector {
public:
	CBstrSafeVector() : controlled(nullptr) {}
	CBstrSafeVector( ULONG count );
	~CBstrSafeVector() { release(); }

	const SAFEARRAY* operator ->() { return controlled; }
	operator SAFEARRAY*() { return controlled; }
	operator const SAFEARRAY*() const { return controlled; }

	SAFEARRAY** operator&();

	HRESULT PutElement( LONG index, const CString& value );

private:
	SAFEARRAY* controlled;

	void release();
};

inline CBstrSafeVector::CBstrSafeVector( ULONG count )
{
	controlled = SafeArrayCreateVector(VT_BSTR, 0, count);
}

inline SAFEARRAY** CBstrSafeVector::operator&()
{
	release();
	return &controlled;
}

inline HRESULT CBstrSafeVector::PutElement( LONG index, const CString& value )
{
	ATL::CComBSTR valueBstr;
	valueBstr.Attach( value.AllocSysString() );
	return SafeArrayPutElement(controlled, &index, valueBstr);
}

inline void CBstrSafeVector::release()
{
	if (!controlled)
		return;
	SafeArrayDestroy(controlled);
	controlled = 0;
}
