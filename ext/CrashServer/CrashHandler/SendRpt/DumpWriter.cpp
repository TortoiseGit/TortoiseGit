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

#include "StdAfx.h"
#include "resource.h"
#include "DumpWriter.h"
#include "..\..\CommonLibs\Log\log.h"

using namespace std;

extern Log g_Log;

static void ExtractFileFromResource(HMODULE hImage, DWORD resId, LPCTSTR path)
{
    g_Log.Debug(_T("Extracting resource %d to \"%s\"..."), resId, path);
    HRSRC hDbghelpRes = FindResource(hImage, MAKEINTRESOURCE(resId), RT_RCDATA);
    if (!hDbghelpRes)
        throw runtime_error("failed to find file in resources");

    HGLOBAL hDbghelpGlobal = LoadResource(hImage, hDbghelpRes);
    if (!hDbghelpGlobal)
        throw runtime_error("failed to load file from resources");

    CAtlFile hFile(CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));
    if (hFile == INVALID_HANDLE_VALUE)
        throw runtime_error("failed to create file");

    if (FAILED(hFile.Write(LockResource(hDbghelpGlobal), SizeofResource(hImage, hDbghelpRes))))
        throw runtime_error("failed to write file");
}

DumpWriter::DumpWriter()
    : m_hDbgHelp(NULL)
    , m_pfnMiniDumpWriteDump(NULL)
{
}

DumpWriter::~DumpWriter()
{
    m_pfnMiniDumpWriteDump = NULL;
    if (m_hDbgHelp)
    {
        FreeLibrary(m_hDbgHelp);
        m_hDbgHelp = NULL;
        DeleteFile(m_szDbghelpPath);
    }
}

void DumpWriter::CreateDbghelp()
{
    TCHAR szTempPath[MAX_PATH];
    GetTempPath(_countof(szTempPath), szTempPath);
    GetTempFileName(szTempPath, _T("dbg"), 0, m_szDbghelpPath);
    DWORD resid = IDR_DBGHELP;
#ifdef USE64
    resid = IDR_DBGHELPX64;
#endif
    ExtractFileFromResource(GetModuleHandle(NULL), resid, m_szDbghelpPath);
}

void DumpWriter::Init()
{
    CreateDbghelp();
    m_hDbgHelp = LoadLibrary(m_szDbghelpPath);
    if (!m_hDbgHelp)
        throw runtime_error("failed to load dbghelp.dll");

    m_pfnMiniDumpWriteDump = (fnMiniDumpWriteDump) GetProcAddress(m_hDbgHelp, "MiniDumpWriteDump");
    if (!m_pfnMiniDumpWriteDump)
        throw runtime_error("failed to get MiniDumpWriteDump");
}

bool DumpWriter::WriteMiniDump(
                   HANDLE hProcess,
                   DWORD dwProcessId,
                   MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo,
                   LPCTSTR pszFileName,
                   MINIDUMP_TYPE DumpType,
                   MINIDUMP_CALLBACK_INFORMATION* pCallback)
{
    if (!m_pfnMiniDumpWriteDump)
        return false;

    HANDLE hFile = CreateFile(pszFileName,
        GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_FLAG_WRITE_THROUGH,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    bool result = m_pfnMiniDumpWriteDump(hProcess, dwProcessId,
        hFile, DumpType, pExceptInfo, NULL, pCallback) != FALSE;

    if (result)
    {
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        result = dwFileSize != INVALID_FILE_SIZE && dwFileSize > 1024;
    }

    CloseHandle(hFile);

    return result;
}
