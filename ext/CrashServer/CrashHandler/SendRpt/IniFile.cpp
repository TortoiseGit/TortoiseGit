#include "StdAfx.h"
#include "IniFile.h"
#include "utils.h"

IniFile::IniFile(const wchar_t* path)
{
    CHandle file(CreateFile(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0));
    if (file == INVALID_HANDLE_VALUE)
        throw std::runtime_error("ini file not found");

    DWORD size = GetFileSize(file, NULL);
    std::vector<BYTE> data(size);

    if (!ReadFile(file, data.data(), (DWORD)data.size(), &size, NULL))
        throw std::runtime_error("failed to read ini file");

    Parse(data.data(), size);
}

IniFile::IniFile(HMODULE hImage, DWORD resId)
{
    auto data = ExtractDataFromResource(hImage, resId);
    Parse(data.first, data.second);
}

const wchar_t* GetEol(const wchar_t* p, const wchar_t* end)
{
    while (p < end && *p != L'\n')
        ++p;
    return p;
}

void IniFile::Parse(LPCVOID data, size_t size)
{
    LPCWSTR begin = (LPCWSTR)data;
    LPCWSTR end = begin + size / sizeof(*begin);

    if (*begin == 0xFEFF)
        ++begin;

    Section* currentSection = nullptr;
    CString* varToAppend = nullptr;

    while (1)
    {
        const wchar_t* eol = GetEol(begin, end);
        if (begin >= eol)
            break;

        CStringW line(begin, int(eol - begin));
        begin = eol + 1;

        line.Trim(L" \t\r\n");

        if (varToAppend)
        {
            bool appendNext = line.Right(1) == L"\\";
            if (appendNext)
                line.Delete(line.GetLength() - 1);
            *varToAppend += line;
            if (!appendNext)
                varToAppend = nullptr;
            continue;
        }

        if (line.IsEmpty() || line[0] == L';')
            continue;

        if (line[0] == L'[' && line[line.GetLength() - 1] == L']')
        {
            currentSection = &m_sections[line.Trim(L"[]")];
            continue;
        }

        if (!currentSection)
            continue;

        int equalPos = line.Find(L'=');
        if (equalPos == -1 || equalPos == 0)
            continue;
        CStringW var = line.Left(equalPos);
        CStringW text = line.Mid(equalPos + 1);
        if (text.Right(1) == L"\\")
        {
            text.Delete(text.GetLength() - 1);
            varToAppend = &(*currentSection)[var];
        }

        (*currentSection)[var] = text;
    }
}

CStringW IniFile::GetString(const wchar_t* section, const wchar_t* variable)
{
    auto sec = m_sections.find(section);
    if (sec != m_sections.end())
    {
        auto var = sec->second.find(variable);
        if (var != sec->second.end())
            return var->second;
    }
    return CStringW();
}
