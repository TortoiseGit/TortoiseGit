// Copyright 2014 Idol Software, Inc.
//
// This file is part of Doctor Dump SDK.
//
// Doctor Dump SDK is free software: you can redistribute it and/or modify
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
    std::vector<BYTE> m_storage;

public:

    Serializer();
    Serializer(const CString& hex);
    Serializer(const BYTE* buf, size_t size);

    CString GetHex() const;

    bool IsReading() const { return m_buf != nullptr; }

    Serializer& SerSimpleType(BYTE* ptr, size_t size);
    Serializer& operator << (BOOL& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (USHORT& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (DWORD& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (HANDLE& val) { return SerSimpleType((BYTE*)&val, sizeof(val)); }
    Serializer& operator << (CStringA& val);
    Serializer& operator << (CStringW& val);
};

Serializer& operator << (Serializer& ser, MINIDUMP_EXCEPTION_INFORMATION& val);
Serializer& operator << (Serializer& ser, Config& cfg);
Serializer& operator << (Serializer& ser, Params& param);
