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

#include "IniFile.h"

class Translator
{
public:
    Translator(
        const wchar_t* appName,
        const wchar_t* company,
        HMODULE hImage,
        DWORD resId,
        const wchar_t* path);

    CStringW GetString(const wchar_t* variable);
    CStringW GetAppName() { return m_appName; }

private:
    CStringW m_appName, m_company;
    CStringW m_userLanguage;
    std::unique_ptr<IniFile> m_custom;
    IniFile m_default;
};

