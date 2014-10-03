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

#ifndef _SYNCHRO_H__INCLUDE_
#define _SYNCHRO_H__INCLUDE_

#include <windows.h>

class Handle
{
protected:
    HANDLE m_hHandle;
public:
    Handle(HANDLE hHandle = NULL): m_hHandle(hHandle) {}
    ~Handle() { Close(); }
    operator HANDLE() { return m_hHandle; }
    Handle& operator = (HANDLE hHandle) { Close(); m_hHandle = hHandle; return *this; }
    void Close() { if (m_hHandle != NULL) { CloseHandle(m_hHandle); m_hHandle = NULL; } }
};

class FileHandle
{
protected:
    HANDLE m_hHandle;
public:
    FileHandle(HANDLE hHandle = INVALID_HANDLE_VALUE): m_hHandle(hHandle) {}
    ~FileHandle() { Close(); }
    operator HANDLE() const { return m_hHandle; }
    FileHandle& operator = (HANDLE hHandle) { Close(); m_hHandle = hHandle; return *this; }
    void Close() { if (m_hHandle != INVALID_HANDLE_VALUE) { CloseHandle(m_hHandle); m_hHandle = INVALID_HANDLE_VALUE; } }
};


class Event: public Handle
{
public:
    Event(LPSECURITY_ATTRIBUTES pAttr = NULL, bool bManual = true, bool bState = false, LPCTSTR pName = NULL)
        : Handle( CreateEvent(pAttr, bManual, bState, pName)) {}
    void Set() { SetEvent(m_hHandle); }
    void Reset() { ResetEvent(m_hHandle); }
};

template <class T>
class SyncLockT
{
public:
    SyncLockT(SyncLockT& lock): m_SyncObj(lock.m_SyncObj) { m_SyncObj.Lock(); }
    SyncLockT(T& SyncObj): m_SyncObj(SyncObj) { m_SyncObj.Lock(); }
    ~SyncLockT() { m_SyncObj.Unlock(); }
private:
    SyncLockT &operator=(const SyncLockT& lock);
    T& m_SyncObj;
};

class CriticalSection
{
public:
    CriticalSection()    { InitializeCriticalSection(&m_cs); }
    ~CriticalSection()   { DeleteCriticalSection(&m_cs); }
    void Lock()   { EnterCriticalSection(&m_cs); }
    void Unlock() { LeaveCriticalSection(&m_cs); }

    typedef SyncLockT<CriticalSection> SyncLock;
private:
    CRITICAL_SECTION m_cs;
};

#endif