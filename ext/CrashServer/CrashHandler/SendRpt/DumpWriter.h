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

#include <wtypes.h>
#pragma warning(push)
#pragma warning(disable: 4091)
#include <dbghelp.h>
#pragma warning(pop)
#include <map>
#include "DoctorDump.h"
#include "../../CommonLibs/Log/log.h"

class DumpWriter
{
public:
    DumpWriter(Log& log);
    ~DumpWriter();

    void Init(LPCWSTR dbgHelpPath);

    bool WriteMiniDump(
        HANDLE hProcess,
        DWORD dwProcessId,
        MINIDUMP_EXCEPTION_INFORMATION* pExceptInfo,
        LPCTSTR pszFileName,
        MINIDUMP_TYPE DumpType,
        MINIDUMP_CALLBACK_INFORMATION* pCallback);

private:
    typedef BOOL (WINAPI *fnMiniDumpWriteDump)(
        HANDLE hProcess,
        DWORD ProcessId,
        HANDLE hFile,
        MINIDUMP_TYPE DumpType,
        CONST PMINIDUMP_EXCEPTION_INFORMATION ExceptionParam,
        CONST PMINIDUMP_USER_STREAM_INFORMATION UserStreamParam,
        CONST PMINIDUMP_CALLBACK_INFORMATION CallbackParam);

    Log& m_log;
    CStringW m_dbgHelpPath;
    HINSTANCE m_hDbgHelp;
    fnMiniDumpWriteDump m_pfnMiniDumpWriteDump;
};

class DumpFilter
{
public:
    DumpFilter(bool& cancel, DWORD saveOnlyThisThreadID = 0);

    operator MINIDUMP_CALLBACK_INFORMATION*() { return &m_callback; }

private:
    MINIDUMP_CALLBACK_INFORMATION m_callback;
    bool& m_cancel;
    DWORD m_saveOnlyThisThreadID;

    BOOL MinidumpCallback(
        const PMINIDUMP_CALLBACK_INPUT CallbackInput,
        PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);

    static BOOL CALLBACK _MinidumpCallback(
        PVOID CallbackParam,
        const PMINIDUMP_CALLBACK_INPUT CallbackInput,
        PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);
};
