// Copyright 2012 Idol Software, Inc.
//
// This file is part of CrashHandler library.
//
// CrashHandler library is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include "Config.h"

class Serializer
{
    const BYTE*  m_buf;
    size_t       m_size;

public:
    std::vector<BYTE> m_storage;

    CString GetHex() const;
    Serializer();
    Serializer(const CString& hex);
    Serializer(const BYTE* buf, size_t size);

    Serializer& SerSimpleType(BYTE* ptr, size_t size);
    Serializer& operator << (BOOL& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (USHORT& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (DWORD& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (HANDLE& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (CStringA& val);
    Serializer& operator << (CStringW& val);

    template <typename T>
    friend Serializer& operator << (Serializer& ser, T& val);
};

Serializer& operator << (Serializer& ser, MINIDUMP_EXCEPTION_INFORMATION& val);
Serializer& operator << (Serializer& ser, Config& cfg);
Serializer& operator << (Serializer& ser, Params& param);
