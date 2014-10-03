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

#ifndef __LOG_MEDIA_H
#define __LOG_MEDIA_H

#include "log.h"
#include "synchro.h"
#include <atlstr.h>

class ConsoleMedia: public ILogMedia
{
public:
    ConsoleMedia();
protected:
    virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
private:
    CriticalSection m_cs;
    HANDLE m_hConsole;
    CONSOLE_SCREEN_BUFFER_INFO m_info;
    bool m_bRedirectedToFile;
};

class FileMedia: public ILogMedia
{
public:
    FileMedia(LPCTSTR pszFilename, bool bAppend, bool bFlush, bool bNewFileDaily = false);
    virtual ~FileMedia();
    virtual bool IsWorking() const;
protected:
    virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
private:
    bool m_bAppend;
    bool m_bFlush;
    bool m_bNewFileDaily;//For this mode to work, file name template must contain %DATE%
    WORD m_wLogYear, m_wLogMonth, m_wLogDay;
    FileHandle m_hFile;
    Handle m_hMutex;
    CString m_sFilename, m_sOrigFilename;
    void CloseLogFile();
    void OpenLogFile();
};

class DebugMedia: public ILogMedia
{
    DWORD m_dwParam;
public:
    DebugMedia(DWORD dwParam);
    virtual bool IsWorking() const;
protected:
    virtual void Write(ELogMessageType type, ELogMessageLevel nLevel, LPCTSTR pszDate, LPCTSTR pszTime, LPCTSTR pszThreadId, LPCTSTR pszThreadName, LPCTSTR pszModule, LPCTSTR pszMessage);
};

#endif