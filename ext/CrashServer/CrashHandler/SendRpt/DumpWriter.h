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

#include <wtypes.h>
#include <dbghelp.h>

class DumpWriter
{
public:
    DumpWriter();
    ~DumpWriter();

    void Init();

    bool WriteMiniDump(
        HANDLE hProcess,
        DWORD dwProcessId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo,
        LPCTSTR pszFileName,
        MINIDUMP_TYPE DumpType,
        MINIDUMP_CALLBACK_INFORMATION* pCallback);

private:
    void CreateDbghelp();

    typedef BOOL (WINAPI *fnMiniDumpWriteDump)(
        IN HANDLE hProcess,
        IN DWORD ProcessId,
        IN HANDLE hFile,
        IN MINIDUMP_TYPE DumpType,
        IN CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam, OPTIONAL
        IN CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam, OPTIONAL
        IN CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam OPTIONAL);

    TCHAR m_szDbghelpPath[MAX_PATH];
    HINSTANCE m_hDbgHelp;
    fnMiniDumpWriteDump m_pfnMiniDumpWriteDump;
};
