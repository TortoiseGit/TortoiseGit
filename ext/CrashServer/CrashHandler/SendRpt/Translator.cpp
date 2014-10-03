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

#include "StdAfx.h"
#include "Translator.h"

static const wchar_t* DefaultSection = L"crashhandler";

Translator::Translator(const wchar_t* appName, const wchar_t* company, HMODULE hImage, DWORD resId, const wchar_t* path)
    : m_appName(appName)
    , m_company(company)
    , m_default(hImage, resId)
{
    if (path && *path)
        m_custom.reset(new IniFile(path));
    // if (WINVER >= 0x0600) Use LOCALE_SNAME en-us
    // LOCALE_SISO639LANGNAME returns en
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SISO639LANGNAME, m_userLanguage.GetBuffer(1024), 1024);
    m_userLanguage.ReleaseBuffer(-1);
    m_userLanguage = DefaultSection + (L"-" + m_userLanguage);
}

CStringW Translator::GetString(const wchar_t* variable)
{
    CStringW result;
    if (m_custom)
    {
        result = m_custom->GetString(m_userLanguage, variable);
        if (result.IsEmpty())
            result = m_custom->GetString(DefaultSection, variable);
    }
    if (result.IsEmpty())
        result = m_default.GetString(m_userLanguage, variable);
    if (result.IsEmpty())
        result = m_default.GetString(DefaultSection, variable);
    if (result.IsEmpty())
        MessageBoxW(0, variable, L"Doesn't translated string", MB_ICONERROR);

    result.Replace(L"\\n", L"\n");
    result.Replace(L"[AppName]", m_appName);
    result.Replace(L"[Company]", m_company);

    return result;
}
