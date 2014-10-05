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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <tchar.h>
#include <assert.h>
#include <time.h>
#include <crtdbg.h>
#include "log_media.h"

#ifndef WINAPI_FAMILY_PARTITION
#define WINAPI_FAMILY_PARTITION(Partition) 0
#endif

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#include <VersionHelpers.h>
#endif

#define ASSERT(f) assert(f)

#ifndef VERIFY
#   ifdef _DEBUG
#       define VERIFY(f) ASSERT(f)
#   else
#       define VERIFY(f) ((void)(f))
#   endif
#endif

static DWORD g_dwThreadNameTlsId = TLS_OUT_OF_INDEXES;
static LPTSTR pszUnknownThreadName = _T("Unknown");
void InitializeLog()
{
    static bool bInitialized = false;
    if (bInitialized)
        return;
    bInitialized = true;
    g_dwThreadNameTlsId = TlsAlloc();
    ASSERT(g_dwThreadNameTlsId != TLS_OUT_OF_INDEXES);
}
LPCTSTR GetLogThreadName()
{
    ASSERT(g_dwThreadNameTlsId != TLS_OUT_OF_INDEXES);
    if (g_dwThreadNameTlsId == TLS_OUT_OF_INDEXES)
        return NULL;
    return static_cast<LPCTSTR>(TlsGetValue(g_dwThreadNameTlsId));
}
void FreeLogThreadName()
{
    ASSERT(g_dwThreadNameTlsId != TLS_OUT_OF_INDEXES);
    if (g_dwThreadNameTlsId == TLS_OUT_OF_INDEXES)
        return;
    LPTSTR pThreadName = static_cast<LPTSTR>(TlsGetValue(g_dwThreadNameTlsId));
    if (pThreadName != pszUnknownThreadName)
    {
        free(pThreadName);
        VERIFY(TlsSetValue(g_dwThreadNameTlsId, pszUnknownThreadName));
    }
}
void SetLogThreadName(LPCTSTR pszThreadName)
{
    ASSERT(g_dwThreadNameTlsId != TLS_OUT_OF_INDEXES);
    if (g_dwThreadNameTlsId == TLS_OUT_OF_INDEXES)
        return;
    FreeLogThreadName();
    VERIFY(TlsSetValue(g_dwThreadNameTlsId, LPVOID(pszThreadName != NULL ? _tcsdup(pszThreadName) : pszUnknownThreadName)));
//      SetDebuggerThreadName(-1, __LPCSTR(pszThreadName));
}

//! Создаёт путь к файлу.
bool CreateFileDir(LPCTSTR pszFilePath)
{
    // Найдём полный путь
    TCHAR szBuffer[1024];
    LPTSTR pszFilePart;
    DWORD dwRes = GetFullPathName(pszFilePath, sizeof(szBuffer)/sizeof(*szBuffer), szBuffer, &pszFilePart);
    if (dwRes == 0
        || dwRes >= sizeof(szBuffer)/sizeof(*szBuffer)
        || pszFilePart == NULL)
        return false;

    // Отрежем имя файла
    *pszFilePart = _T('\0');

    CString sPath(szBuffer);
    sPath.Replace(_T('/'), _T('\\'));
    ASSERT(sPath.Right(1) == _T("\\"));

    int nPos;
    if (sPath.Left(2) == _T("\\\\"))
        nPos = sPath.Find(_T("\\"), sPath.Find(_T("\\"), 2) + 1) + 1; // Пропустим имя компьютера и шары
    else
        nPos = 4; // Пропустим имя диска
    while ( (nPos = sPath.Find(_T('\\'), nPos + 1)) != -1)
    {
        if (!CreateDirectory(sPath.Left(nPos), NULL) && GetLastError() != ERROR_ALREADY_EXISTS)
            return false;
    }
    return true;
}

void LogBase::SetThreadName(LPCTSTR pszThreadName)
{
    SetLogThreadName(pszThreadName);
}

LPCTSTR LogBase::GetThreadName()
{
    LPCTSTR pszName = GetLogThreadName();
    return pszName ? pszName : _T("Unknown");
}


LogMediaPtr LogBase::GetAppLogMedia()
{
    static LogMediaPtr pAppLog(new LogMediaProxy());
    return pAppLog;
}

void LogBase::SetAppLogMedia(LogMediaPtr pLog)
{
    ASSERT(pLog != GetAppLogMedia());
    if (pLog == GetAppLogMedia())
        return;
    std::static_pointer_cast<LogMediaProxy>(GetAppLogMedia())->SetLog(pLog);
}

LogMediaPtr LogBase::CreateConsoleMedia()
{
    static LogMediaPtr pConsoleMedia(new ConsoleMedia());
    if (pConsoleMedia->IsWorking())
        return pConsoleMedia;
    return LogMediaPtr();
}

LogMediaPtr LogBase::CreateFileMedia(LPCTSTR pszFilename, bool bAppend, bool bFlush, bool bNewFileDaily)
{
    LogMediaPtr pLog(new FileMedia(pszFilename, bAppend, bFlush, bNewFileDaily));
    if (pLog->IsWorking())
        return pLog;
    return LogMediaPtr();
}

LogMediaPtr LogBase::CreateDebugMedia(DWORD dwParam)
{
    LogMediaPtr pDebugMedia(new DebugMedia(dwParam));
    if (pDebugMedia->IsWorking())
        return pDebugMedia;
    return LogMediaPtr();
}


//////////////////////////////////////////////////////////////////////////
//
//////////////////////////////////////////////////////////////////////////

void FormatLogMessage(ELogMessageType type,
                      ELogMessageLevel nLevel,
                      LPCTSTR pszDate,
                      LPCTSTR pszTime,
                      LPCTSTR pszThreadId,
                      LPCTSTR pszThreadName,
                      LPCTSTR pszModule,
                      LPCTSTR pszMessage,
                      CString& output)
{
#if 1
    output.Empty();
    output.Preallocate(1024);

    if (type != eLM_DirectOutput)
    {
        output += pszDate;
        output += _T(' ');
        output += pszTime;
#if defined(LOG_THREAD_NAME)
        output += _T(" [");
        output += pszThreadId;
        output += _T(":");
        size_t nThreadNameLen = _tcslen(pszThreadName);
        if (nThreadNameLen > 12)
            output.append(pszThreadName, 12);
        else
        {
            output.append(12 - nThreadNameLen, _T(' '));
            output += pszThreadName;
        }
        output += _T("] ");
#else
        output += _T(" ");
        output += pszThreadId;
        output += _T(" ");
#endif

        switch (type)
        {
        case eLM_Info:
            output += _T('I');
            break;
        case eLM_Debug:
            output += _T('-');
            break;
        case eLM_Warning:
            output += _T('W');
            break;
        case eLM_Error:
            output += _T('E');
            break;
        default:
            ASSERT(false);
        }

        if (nLevel > 0)
            output.AppendFormat(_T("%i"), nLevel);
        else
            output += _T(' ');
#if 0
        output += _T(" : [");
        output += pszModule;
        output += _T("] ");
#else
        output += _T(" : ");
#endif
    }
    output += pszMessage;
    output.TrimRight();
#else
    output.Empty();
    output.reserve(1024);

    output += pszDate;
    output += _T(' ');
    output += pszTime;
    output += _T("\t");
    output += pszThreadId;
    output += _T("\t");
    output += pszThreadName;
    output += _T("\t");
    output += pszModule;
    output += _T("\t");

    switch (type)
    {
    case eLM_Info:
        output += _T("Inf");
        break;
    case eLM_Debug:
        output += _T("Dbg");
        break;
    case eLM_Warning:
        output += _T("Wrn");
        break;
    case eLM_Error:
        output += _T("Err");
        break;
    default:
        ASSERT(false);
    }

    if (nLevel > 0)
        output.AppendFormat(_T("%i"), nLevel);
    output += _T('\t');
    output += pszMessage;
    output.TrimRight();
#endif
}

CString GetSystemInformation()
{
    CString info = _T("Microsoft Windows ");

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
    if (IsWindows8Point1OrGreater())
        info += _T(">= 8.1");
    else if (IsWindows8OrGreater())
        info += _T("8.1");
    else if (IsWindows7SP1OrGreater())
        info += _T("7 SP1");
    else if (IsWindows7OrGreater())
        info += _T("7");
    else if (IsWindowsVistaSP2OrGreater())
        info += _T("Vista SP2");
    else if (IsWindowsVistaSP1OrGreater())
        info += _T("Vista SP1");
    else if (IsWindowsVistaOrGreater())
        info += _T("Vista");

    if (IsWindowsServer())
        info += _T(" Server");
    else
        info += _T(" Client");
#else
    OSVERSIONINFOEX rcOS;
    ZeroMemory(&rcOS, sizeof(rcOS));
    rcOS.dwOSVersionInfoSize = sizeof(rcOS);

    GetVersionEx((OSVERSIONINFO*)&rcOS);

    if (rcOS.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        switch (rcOS.dwMinorVersion)
        {
        case 0: info += _T("95"); break;
        case 10: info += _T("98"); break;
        case 90: info += _T("Me"); break;
        }
        if (!_tcscmp(rcOS.szCSDVersion, _T(" A")))
            info += _T(" Second Edition");
        else
            if (!_tcscmp(rcOS.szCSDVersion, _T(" C")))
                info += _T(" OSR2");
    }
    else if (rcOS.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        switch (rcOS.dwMajorVersion)
        {
        case 3: info += _T("NT 3.51"); break;
        case 4: info += _T("NT 4.0"); break;
        case 5:
            switch (rcOS.dwMinorVersion)
            {
            case 0: info += _T("2000"); break;
            case 1: info += _T("XP"); break;
            case 2: info += _T("2003 Server"); break;
            default: ASSERT(false); info += _T("future version"); break;
            }
            break;
        case 6:
            switch (rcOS.dwMinorVersion)
            {
            case 0: info += rcOS.wProductType == VER_NT_WORKSTATION ? _T("Vista") : _T("Server 2008"); break;
            case 1: info += rcOS.wProductType == VER_NT_WORKSTATION ? _T("7") : _T("Server 2008 R2"); break;
            case 2: info += rcOS.wProductType == VER_NT_WORKSTATION ? _T("8") : _T("Server 2012"); break;
            case 3: info += rcOS.wProductType == VER_NT_WORKSTATION ? _T("8.1") : _T("Server 2012 R2"); break;
            default:
                {
                    CString future;
                    future.Format(_T("%ld.%ld%s"), rcOS.dwMajorVersion, rcOS.dwMinorVersion, VER_NT_WORKSTATION ? _T("") : _T(" Server"));
                    info += future;
                    break;
                }
            }
            break;
        default:
            {
                CString future;
                future.Format(_T("%ld.%ld%s"), rcOS.dwMajorVersion, rcOS.dwMinorVersion, VER_NT_WORKSTATION ? _T("") : _T(" Server"));
                info += future;
                break;
            }
        }
        if (_tcslen(rcOS.szCSDVersion) > 0)
        {
            info += _T(", ");
            info += rcOS.szCSDVersion;
        }
    }
#endif
    return info;
}

CString GetModuleInformation()
{
    CString info;
    TCHAR szApp[MAX_PATH];
    if (!GetModuleFileName(NULL, szApp, _countof(szApp)))
        return info;

    info = szApp;

    DWORD dwDummy;
    DWORD dwInfoSize = GetFileVersionInfoSize(szApp, &dwDummy);
    if (dwInfoSize)
    {
        LPVOID pInfoBuffer = _alloca(dwInfoSize);
        if (GetFileVersionInfo(szApp, 0, dwInfoSize, pInfoBuffer))
        {
            VS_FIXEDFILEINFO *pInfo = NULL;
            UINT nInfoSize = 0;
            if (VerQueryValue (pInfoBuffer, _T("\\"), (LPVOID *)&pInfo, &nInfoSize) && nInfoSize == sizeof(VS_FIXEDFILEINFO))
            {
                info.AppendFormat(_T(" (%d.%d.%d.%d)"),
                            HIWORD(pInfo->dwFileVersionMS),
                            LOWORD(pInfo->dwFileVersionMS),
                            HIWORD(pInfo->dwFileVersionLS),
                            LOWORD(pInfo->dwFileVersionLS)
                        );
            }
        }
    }

    info.AppendFormat(_T(" PID: %d (0x%x)"), GetCurrentProcessId(), GetCurrentProcessId());

    return info;
}

//////////////////////////////////////////////////////////////////////////
// CConsoleMedia
//////////////////////////////////////////////////////////////////////////

ConsoleMedia::ConsoleMedia()
{
    m_bRedirectedToFile = true;

    m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (m_hConsole == NULL)
    {
        if (!AllocConsole() //|| !SetConsoleTitle(_T("Log"))
            || (m_hConsole = GetStdHandle(STD_OUTPUT_HANDLE)) == NULL)
            return;
    }
    GetConsoleScreenBufferInfo(m_hConsole, &m_info);

    DWORD t;
    m_bRedirectedToFile = GetConsoleMode(m_hConsole, &t) == FALSE;
}

void ConsoleMedia::Write(ELogMessageType type,
                          ELogMessageLevel nLevel,
                          LPCTSTR pszDate,
                          LPCTSTR pszTime,
                          LPCTSTR pszThreadId,
                          LPCTSTR pszThreadName,
                          LPCTSTR pszModule,
                          LPCTSTR pszMessage)
{
    if (m_hConsole==NULL)
        return;
    WORD color = FOREGROUND_GREEN|FOREGROUND_INTENSITY;
    //FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE|FOREGROUND_INTENSITY|BACKGROUND_RED|BACKGROUND_GREEN|BACKGROUND_BLUE|BACKGROUND_INTENSITY;
    switch (type)
    {
    case eLM_Debug:
        color = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_BLUE;
        break;
    case eLM_Warning:
        color = FOREGROUND_RED|FOREGROUND_GREEN|FOREGROUND_INTENSITY;
        break;
    case eLM_Error:
        color = FOREGROUND_RED|FOREGROUND_INTENSITY;
        break;
    }

    CString output;
    FormatLogMessage(type, nLevel, pszDate, pszTime, pszThreadId, pszThreadName, pszModule, pszMessage, output);

    if (m_bRedirectedToFile)
    {
        output += _T("\n");
        CStringA outputA = output;
        outputA.AnsiToOem();

        CriticalSection::SyncLock lock(m_cs);
        DWORD dwWritten;
        WriteFile(m_hConsole, (LPCSTR)outputA, (DWORD)outputA.GetLength(), &dwWritten, NULL);
    }
    else
    {
#ifndef _UNICODE
        output.AnsiToOem();
#endif
        CriticalSection::SyncLock lock(m_cs);
        SetConsoleTextAttribute(m_hConsole, color);
        DWORD dwWritten;
        WriteConsole(m_hConsole, (LPCTSTR)output, (DWORD)output.GetLength(), &dwWritten, NULL);

        SetConsoleTextAttribute(m_hConsole, m_info.wAttributes);
        WriteConsole(m_hConsole, _T("\n"), 1, &dwWritten, NULL);
    }
}

//////////////////////////////////////////////////////////////////////////
// CFileMedia
//////////////////////////////////////////////////////////////////////////

FileMedia::FileMedia(LPCTSTR pszFilename, bool bAppend, bool bFlush, bool bNewFileDaily)
    : m_bAppend(bAppend), m_bFlush(bFlush), m_bNewFileDaily(bNewFileDaily), m_wLogYear(0), m_wLogMonth(0), m_wLogDay(0),
    m_sOrigFilename(pszFilename)
{
    OpenLogFile();
}

FileMedia::~FileMedia()
{
    CloseLogFile();
}

void FileMedia::CloseLogFile()
{
    if (m_hFile != INVALID_HANDLE_VALUE && WaitForSingleObject(m_hMutex, 1000) != WAIT_TIMEOUT)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        CString header;
        header.Format(
            _T("=================================================\r\n")
            _T("=== Trace Log Finished on %i-%02i-%02i %02i:%02i:%02i ===\r\n")
            _T("=================================================\r\n")
            , st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

        LONG dwDistanceToMoveHigh = 0; // Нужен, чтобы писать в файлы больше 4Гб
        VERIFY(SetFilePointer(m_hFile, 0, &dwDistanceToMoveHigh, FILE_END) != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR);
        CStringA headerA = header;
        DWORD dwWritten;
//      VERIFY(WriteFile(m_hFile, (LPCTSTR)header, (DWORD) header.size() * sizeof(TCHAR), &dwWritten, NULL));
        VERIFY(WriteFile(m_hFile, (LPCSTR)headerA, (DWORD) headerA.GetLength() * sizeof(CHAR), &dwWritten, NULL));
        VERIFY(ReleaseMutex(m_hMutex));
    }
}

bool FileMedia::IsWorking() const
{
    return m_hFile != INVALID_HANDLE_VALUE;
}

void FileMedia::OpenLogFile()
{
    SYSTEMTIME st;
    GetLocalTime(&st);

    m_sFilename = m_sOrigFilename;
    m_sFilename.Replace(_T("%DATETIME%"), _T("%DATE% %TIME%"));
    if (m_sFilename.Find(_T("%DATE%")) != -1)
    {
        TCHAR bufdate[128];
        _stprintf_s(bufdate, _T("%i-%02i-%02i"), st.wYear, st.wMonth, st.wDay);
        m_sFilename.Replace(_T("%DATE%"), bufdate);
        m_wLogYear = st.wYear;
        m_wLogMonth = st.wMonth;
        m_wLogDay = st.wDay;
    }
    if (m_sFilename.Find(_T("%TIME%")) != -1)
    {
        GetLocalTime(&st);
        TCHAR buftime[128];
        _stprintf_s(buftime, _T("%02i-%02i"), st.wHour, st.wMinute);
        m_sFilename.Replace(_T("%TIME%"), buftime);
    }

    if (!CreateFileDir(m_sFilename))
    {
        _RPT1(_CRT_ERROR, "FileMedia: Can't create folder '%S'", (LPCWSTR) CStringW(m_sFilename));
        return;
    }


    // Создадим для доступа к этому файлу мьютекс
    CString sMtx(m_sFilename);
    sMtx.MakeUpper();
    for (int i = 0, size = sMtx.GetLength(); i < size; ++i)
    {
        if (!_istalnum(sMtx[i]))
            sMtx.SetAt(i, _T('_'));
    }
    m_hMutex = CreateMutex(NULL, TRUE, (LPCTSTR)sMtx);
    DWORD dwMtxError = GetLastError();

    m_hFile = CreateFile(m_sFilename,
        GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        (m_bAppend || dwMtxError == ERROR_ALREADY_EXISTS) ? OPEN_ALWAYS : CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        if (dwMtxError != ERROR_ALREADY_EXISTS)
            VERIFY(ReleaseMutex(m_hMutex));
        m_hMutex.Close();
        return;
    }

    if (dwMtxError != ERROR_ALREADY_EXISTS)
    {
        CString header;
        header.Format(
            _T("================================================\r\n")
            _T("=== Trace Log Started on %i-%02i-%02i %02i:%02i:%02i ===\r\n")
            _T("=== %s ===\r\n")
            _T("================================================\r\n")
            _T("\r\n%s\r\n\r\n")
            , st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond
            , (LPCTSTR)GetSystemInformation()
            , (LPCTSTR)GetModuleInformation());

        LONG dwDistanceToMoveHigh = 0; // Нужен, чтобы писать в файлы больше 4Гб
        VERIFY(SetFilePointer(m_hFile, 0, &dwDistanceToMoveHigh, FILE_END) != INVALID_SET_FILE_POINTER || GetLastError() == NO_ERROR);
        CStringA headerA = header;
        DWORD dwWritten;
        VERIFY(WriteFile(m_hFile, (LPCSTR)headerA, (DWORD)headerA.GetLength() * sizeof(CHAR), &dwWritten, NULL));
        if (m_hMutex != NULL)
            VERIFY(ReleaseMutex(m_hMutex));
    }
}

void FileMedia::Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage)
{
    if (m_bNewFileDaily && m_wLogYear != 0)
    {
        SYSTEMTIME st;
        GetLocalTime(&st);
        if (st.wDay != m_wLogDay || st.wMonth != m_wLogMonth || st.wDay != m_wLogDay)
        {
            CloseLogFile();
            OpenLogFile();
        }
    }

    if (WaitForSingleObject(m_hMutex, 1000) == WAIT_TIMEOUT)
        return;

    // trying to restore access
    if (m_hFile == INVALID_HANDLE_VALUE)
    {
        m_hFile = CreateFile(m_sFilename,
            GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
        if (m_hFile == INVALID_HANDLE_VALUE)
        {
            VERIFY(ReleaseMutex(m_hMutex));
            return;
        }
    }

    LONG dwDistanceToMoveHigh = 0; // Нужен, чтобы писать в файлы больше 4Гб
    if (SetFilePointer(m_hFile, 0, &dwDistanceToMoveHigh, FILE_END) == INVALID_SET_FILE_POINTER
        && GetLastError() != NO_ERROR)
    {
        _RPT0(_CRT_ERROR, "FileMedia: Can't set file pointer");
        m_hFile.Close();
        VERIFY(ReleaseMutex(m_hMutex));
        return;
    }

    CString output;
    FormatLogMessage(type, nLevel, pszDate, pszTime, pszThreadId, pszThreadName, pszModule, pszMessage, output);
    output += _T("\r\n");

    CStringA outputA = output;

    DWORD dwWritten;
    VERIFY(WriteFile(m_hFile, (LPCSTR)outputA, (DWORD) outputA.GetLength() * sizeof(CHAR), &dwWritten, NULL));
    if (m_bFlush)
        VERIFY(FlushFileBuffers(m_hFile));
    VERIFY(ReleaseMutex(m_hMutex));
}

//////////////////////////////////////////////////////////////////////////
// CDebugMedia
//////////////////////////////////////////////////////////////////////////

DebugMedia::DebugMedia(DWORD dwParam)
    : m_dwParam(dwParam)
{
}

extern "C" WINBASEAPI BOOL WINAPI IsDebuggerPresent ( VOID );

bool DebugMedia::IsWorking() const
{
    return (m_dwParam & LogBase::DEBUGMEDIA_FORCE) ? true : IsDebuggerPresent() != FALSE;
}

void DebugMedia::Write(ELogMessageType type,
                        ELogMessageLevel nLevel,
                        LPCTSTR pszDate,
                        LPCTSTR pszTime,
                        LPCTSTR pszThreadId,
                        LPCTSTR pszThreadName,
                        LPCTSTR pszModule,
                        LPCTSTR pszMessage)
{
    CString output;
    FormatLogMessage(type, nLevel, pszDate, pszTime, pszThreadId, pszThreadName, pszModule, pszMessage, output);
    output += _T('\n');

    OutputDebugString(output);
    if (type == eLM_Error && (m_dwParam & LogBase::DEBUGMEDIA_REPORTERRORS))
    {
        _RPT1(_CRT_ERROR, "%ws", static_cast<LPCWSTR>(output));
    }
}

//////////////////////////////////////////////////////////////////////////
// LogMediaProxy
//////////////////////////////////////////////////////////////////////////

LogMediaProxy::LogMediaProxy(const LogMediaPtr& pLog)
    : m_pLog(pLog)
{
}

LogMediaProxy::~LogMediaProxy()
{
}

void LogMediaProxy::SetLog(const LogMediaPtr& pLog)
{
    CriticalSection::SyncLock lock(m_cs);
    m_pLog = pLog;
}

void LogMediaProxy::Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage)
{
    CriticalSection::SyncLock lock(m_cs);
    if (m_pLog)
        m_pLog->Write(type, nLevel, pszDate, pszTime, pszThreadId, pszThreadName, pszModule, pszMessage);
}

bool LogMediaProxy::Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule)
{
    CriticalSection::SyncLock lock(m_cs);
    if (m_pLog)
        return m_pLog->Check(type, nLevel, pszModule);
    return false;
}

//////////////////////////////////////////////////////////////////////////
// LogMediaColl
//////////////////////////////////////////////////////////////////////////

LogMediaColl::LogMediaColl()
{
}

LogMediaColl::~LogMediaColl()
{
}

void LogMediaColl::Add(const LogMediaPtr& pMedia)
{
    if (!pMedia)
        return;
    CriticalSection::SyncLock lock(m_cs);
    m_MediaColl.push_back(pMedia);
}

void LogMediaColl::Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage)
{
    CriticalSection::SyncLock lock(m_cs);
    for (MediaColl::iterator it = m_MediaColl.begin(), end = m_MediaColl.end(); it != end; ++it)
        (*it)->Write(type, nLevel, pszDate, pszTime, pszThreadId, pszThreadName, pszModule, pszMessage);
}

bool LogMediaColl::Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule)
{
    CriticalSection::SyncLock lock(m_cs);
    // Если хотя бы один лог пропускает, то и мы пропускаем.
    for (MediaColl::iterator it = m_MediaColl.begin(), end = m_MediaColl.end(); it != end; ++it)
        if ((*it)->Check(type, nLevel, pszModule))
            return true;
        return false;
};

//////////////////////////////////////////////////////////////////////////
// CFilterLogMedia
//////////////////////////////////////////////////////////////////////////

FilterLogMedia::FilterLogMedia(const LogMediaPtr& pMedia)
    : m_pMedia(pMedia)
{
}

FilterLogMedia::~FilterLogMedia()
{
}

void FilterLogMedia::AddFilter(FilterPtr pFilter)
{
    CriticalSection::SyncLock lock(m_cs);
    m_FilterColl.push_back(pFilter);
}

void FilterLogMedia::Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage)
{
    if (!m_pMedia)
        return;
    {
        CriticalSection::SyncLock lock(m_cs);
        for (FilterColl::iterator it = m_FilterColl.begin(), end = m_FilterColl.end(); it != end; ++it)
            if (!(*it)->Check(type, nLevel, pszModule))
                return;
    }
    m_pMedia->Write(type, nLevel, pszDate, pszTime, pszThreadId, pszThreadName, pszModule, pszMessage);
}

bool FilterLogMedia::Check(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszModule)
{
    if (!m_pMedia)
        return false;
    {
        CriticalSection::SyncLock lock(m_cs);
        for (FilterColl::iterator it = m_FilterColl.begin(), end = m_FilterColl.end(); it != end; ++it)
            if (!(*it)->Check(type, nLevel, pszModule))
                return false;
    }
    return m_pMedia->Check(type, nLevel, pszModule);
}

//////////////////////////////////////////////////////////////////////////
// LogBase
//////////////////////////////////////////////////////////////////////////

LogBase::LogBase(LogMediaPtr pMedia,    LPCTSTR pszModule)
    : m_szModule(pszModule)
    , m_pMedia(pMedia)
{
    ASSERT(pszModule != NULL);
}

LogBase::~LogBase()
{
}

template <typename T, size_t nSize>
class StackResizableBuf
{
    T loc_data[nSize];
    void Clear() { if (data != loc_data) delete[] data; }
public:
    StackResizableBuf() { data = loc_data; size = nSize; }
    ~StackResizableBuf() { Clear(); }
    void Resize(size_t nNewSize) { Clear(); data = new T[nNewSize]; size = nNewSize; }

    T* data;
    size_t size;
};

bool LogBase::IsFiltered(ELogMessageType type, ELogMessageLevel nLevel)
{
    return !m_pMedia || !m_pMedia->Check(type, nLevel, m_szModule.c_str());
}

void LogBase::WriteVA(ELogMessageType type,
                        ELogMessageLevel nLevel,
                        LPCTSTR pszModule,
                        LPCTSTR pszMessage,
                        va_list args)  throw()
{
    if (IsFiltered(type, nLevel)) // не обрабатываем сообщение, если оно не попадает в лог
        return;
    // 75% времени тратится на new и delete, поэтому первую попытку попробуем сделать без него.
    StackResizableBuf<TCHAR, 16*1024> buf;
    int pos;
    for (;;)
    {
        pos = _vsntprintf_s(buf.data, buf.size, buf.size - 1, pszMessage, args);
        if (pos != -1)
            break;
        // BUG 16456 FIX, см. в конец WriteVAW почему
        //if (buf.size >= 1024 * 256)
        if (buf.size >= 1024 * 1024 * 10) //Increased limit for DumpServer
        {
            pos = (int)buf.size - 1;
            break;
        }
        buf.Resize(buf.size * 2);
    }
    if (pos >= 0)
    {
        buf.data[pos] = 0;
        SYSTEMTIME st;
        GetLocalTime(&st);
        TCHAR bufdate[128], buftime[128], bufthread[128];
        _stprintf_s(bufdate, _T("%i-%02i-%02i"), st.wYear, st.wMonth, st.wDay);
        _stprintf_s(buftime, _T("%02i:%02i:%02i.%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        _stprintf_s(bufthread, _T("%4x"), GetCurrentThreadId());
        m_pMedia->Write(type, nLevel, bufdate, buftime, bufthread, GetThreadName()
            , pszModule ? pszModule : m_szModule.c_str(), buf.data);
    }
}

void LogBase::WriteVAW(ELogMessageType type,
                         ELogMessageLevel nLevel,
                         LPCWSTR pszModule,
                         LPCWSTR pszMessage,
                         va_list args)  throw()
{
#if defined(_UNICODE) || defined(UNICODE)
    WriteVA(type, nLevel, pszModule, pszMessage, args);
#else

    if (IsFiltered(type, nLevel)) // не обрабатываем сообщение, если оно не попадает в лог
        return;
    // 75% времени тратится на new и delete, поэтому первую попытку попробуем сделать без него.
    StackResizableBuf<WCHAR, 1024> buf;
    int pos;
    for (;;)
    {
        pos = _vsnwprintf(buf.data, buf.size - 1, pszMessage, args);
        if (pos != -1)
            break;
        // BUG 16456 FIX, см. в конец WriteVAW почему
        if (buf.size >= 1024 * 256)
        {
            pos = buf.size - 1;
            break;
        }
        buf.Resize(buf.size * 2);
    }
    if (pos >= 0)
    {
        buf.data[pos] = 0;
        LPTSTR pszStr = static_cast<LPTSTR>(_alloca(buf.size));
        if (0 == WideCharToMultiByte(CP_ACP, 0/*WC_DEFAULTCHAR*/, buf.data, pos + 1, pszStr, buf.size, NULL, NULL))
        {
            _RPT1(_CRT_ERROR, "Can't convert Unicode string (error #0x%08X)", GetLastError());
            return;
        }

        LPCTSTR pszMod;
        if (pszModule == NULL)
            pszMod = m_szModule.c_str();
        else
        {
            size_t nModuleLen = wcslen(pszModule) + 1;
            LPTSTR pszModBuf = static_cast<LPTSTR>(_alloca(nModuleLen));
            if (0 == WideCharToMultiByte(CP_ACP, 0/*WC_DEFAULTCHAR*/, pszModule, nModuleLen, pszModBuf, nModuleLen, NULL, NULL))
            {
                _RPT1(_CRT_ERROR, "Can't convert Unicode string (error #0x%08X)", GetLastError());
                return;
            }
            pszMod = pszModBuf;
        }

        SYSTEMTIME st;
        GetLocalTime(&st);
        TCHAR bufdate[128], buftime[128], bufthread[128];
        _stprintf(bufdate, _T("%i-%02i-%02i"), st.wYear, st.wMonth, st.wDay);
        _stprintf(buftime, _T("%02i:%02i:%02i.%03d"), st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
        _stprintf(bufthread, _T("%03x"), GetCurrentThreadId());
        m_pMedia->Write(type, nLevel, bufdate, buftime, bufthread, GetThreadName(), pszMod, pszStr);
    }
#endif
    /* BUG 16456 FIX
    _vsnwprintf спотыкается на строках с символом 0xFFFF,
    возвращает -1 при любой длине буфера. Соответсвтвенно
    буфер увеличивается (Resize) пока не хватит памяти,
    а потом вызывается _vsnwprintf на NULL.
    Решение: Т.к. _vsnwprintf, даже если нехватает буфера, начало заполняет,
            то просто ограничим буфер 256кб (а 0 на конце и так ставим на всякий случай)

    #include <stdarg.h>
    #include <stdio.h>

    void WriteVAW(wchar_t* pszMessage, va_list args)
    {
        wchar_t buf[1000];
        int n = _vsnwprintf(buf, 1000, pszMessage, args);
        printf("Return %i\nBuf is: \"%ls\"", n, buf); // n будет -1, хотя буфера хватает !!!
    }
    void WriteW(wchar_t* pszMessage, ...)
    {
        va_list ap;
        va_start(ap, pszMessage);
        WriteVAW(pszMessage, ap);
        va_end(ap);
    }
    void main()
    {
        WriteW(L"%ls!", L"123\xffff");
    }
    */
}


//////////////////////////////////////////////////////////////////////////
// CLocalLog
//////////////////////////////////////////////////////////////////////////

LocalLog::LocalLog(const LogParam& param, LPCTSTR pszModule)
    : LogBase(param.m_pMedia, param.m_pszModule ? param.m_pszModule : pszModule)
{
}

LocalLog::LocalLog(LogMediaPtr pMedia, LPCTSTR pszModule)
    : LogBase(pMedia, pszModule)
{
    ASSERT(pszModule != NULL);
}

LocalLog::LocalLog(const LogBase& log, LPCTSTR pszModule)
    : LogBase(log.GetMedia(), pszModule)
{
}

LocalLog::~LocalLog()
{
}

//////////////////////////////////////////////////////////////////////////
// Log
//////////////////////////////////////////////////////////////////////////

Log::Log(const LogParam& param, LPCTSTR pszModule)
    : LogBase(param.m_pMedia, param.m_pszModule ? param.m_pszModule : pszModule)
{
}

Log::Log(LogMediaPtr pMedia, LPCTSTR pszModule)
    : LogBase(pMedia, pszModule)
{
}

Log::Log(const LogBase& log, LPCTSTR pszModule)
    : LogBase(log.GetMedia(), pszModule)
{
}

Log::~Log()
{
}

void Log::SetParams(LogMediaPtr pMedia, LPCTSTR pszModule)
{
    CriticalSection::SyncLock lock(m_cs);
    m_pMedia  = pMedia;
    if (pszModule != NULL)
        m_szModule = pszModule;
}

void Log::WriteVA(ELogMessageType type,
                    ELogMessageLevel nLevel,
                    LPCTSTR pszModule,
                    LPCTSTR pszMessage,
                    va_list args) throw()
{
    CriticalSection::SyncLock lock(m_cs);
    LogBase::WriteVA(type, nLevel, pszModule, pszMessage, args);
}

void Log::WriteVAW(ELogMessageType type,
                     ELogMessageLevel nLevel,
                     LPCWSTR pszModule,
                     LPCWSTR pszMessage,
                     va_list args) throw()
{
    CriticalSection::SyncLock lock(m_cs);
    LogBase::WriteVAW(type, nLevel, pszModule, pszMessage, args);
}

//////////////////////////////////////////////////////////////////////////
// CFilterLog
//////////////////////////////////////////////////////////////////////////

FilterLog::~FilterLog()
{
}
