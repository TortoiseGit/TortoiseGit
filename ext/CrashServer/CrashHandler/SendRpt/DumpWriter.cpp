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

#include "stdafx.h"
#include "resource.h"
#include "DumpWriter.h"
#include <map>
#include "utils.h"

using namespace std;

DumpWriter::DumpWriter(Log& log)
    : m_log(log)
    , m_hDbgHelp(NULL)
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
    }
}

void DumpWriter::Init(LPCWSTR dbgHelpPath)
{
    if (dbgHelpPath)
        m_dbgHelpPath = dbgHelpPath;
    m_hDbgHelp = LoadLibraryW(m_dbgHelpPath);
    if (!m_hDbgHelp)
    {
        m_hDbgHelp = LoadLibrary(_T("dbghelp.dll"));
        if (!m_hDbgHelp)
            throw runtime_error("failed to load dbghelp.dll");
    }

#define GetProc(func)\
    m_pfn##func = (fn##func) GetProcAddress(m_hDbgHelp, #func);\
    if (!m_pfn##func)\
        throw runtime_error("failed to get " #func);

    GetProc(MiniDumpWriteDump);
#undef GetProc
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
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (hFile == INVALID_HANDLE_VALUE)
        return false;

    bool result = m_pfnMiniDumpWriteDump(hProcess, dwProcessId,
        hFile, DumpType, pExceptInfo, NULL, pCallback) != FALSE;

    DWORD err = GetLastError();

    if (result)
    {
        DWORD dwFileSize = GetFileSize(hFile, NULL);
        result = dwFileSize != INVALID_FILE_SIZE && dwFileSize > 1024;
        if (!result)
            err = ERROR_INVALID_DATA;
    }
    else
    {
        m_log.Error(_T("MiniDumpWriteDump failed, 0x%08x"), err);
    }

    CloseHandle(hFile);

    SetLastError(err);

    return result;
}

DumpFilter::DumpFilter(bool& cancel, DWORD saveOnlyThisThreadID)
    : m_saveOnlyThisThreadID(saveOnlyThisThreadID), m_cancel(cancel)
{
    m_callback.CallbackRoutine = _MinidumpCallback;
    m_callback.CallbackParam = this;
}

BOOL DumpFilter::MinidumpCallback(const PMINIDUMP_CALLBACK_INPUT CallbackInput, PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
{
    if (!CallbackInput)
        return TRUE;

    switch (CallbackInput->CallbackType)
    {
    case CancelCallback:
        CallbackOutput->CheckCancel = TRUE;
        if (m_cancel)
            CallbackOutput->Cancel = TRUE;
        break;
    case IncludeThreadCallback:
        if (m_saveOnlyThisThreadID != 0 && CallbackInput->IncludeThread.ThreadId != m_saveOnlyThisThreadID)
        {
            CallbackOutput->ThreadWriteFlags = 0;
            return FALSE;
        }
        break;
    case ThreadCallback:
        if (m_saveOnlyThisThreadID != 0 && CallbackInput->Thread.ThreadId != m_saveOnlyThisThreadID)
            CallbackOutput->ThreadWriteFlags = 0;
        break;
    }

    return TRUE;
}

BOOL CALLBACK DumpFilter::_MinidumpCallback(PVOID CallbackParam, const PMINIDUMP_CALLBACK_INPUT CallbackInput, PMINIDUMP_CALLBACK_OUTPUT CallbackOutput)
{
    return static_cast<DumpFilter*>(CallbackParam)->MinidumpCallback(CallbackInput, CallbackOutput);
}
