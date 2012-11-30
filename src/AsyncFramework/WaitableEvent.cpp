/***************************************************************************
 *   Copyright (C) 2009 by Stefan Fuhrmann                                 *
 *   stefanfuhrmann@alice-dsl.de                                           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "stdafx.h"
#include "WaitableEvent.h"
#include "CriticalSection.h"

namespace async
{

// recycler for OS constructs

namespace
{
    class CWaitableEventPool
    {
    private:

        // multi-threading sync.

        CCriticalSection mutex;

        // allocated, currently usused handles

        std::vector<HANDLE> handles;

        // flag indicating that this instance has not been destroyed, yet

        volatile LONG alive;

        // construction / destruction:
        // free all handles upon destruction at latest

        CWaitableEventPool();
        ~CWaitableEventPool();

    public:

        // Meyer's singleton

        static CWaitableEventPool* GetInstance();

        // recycling interface:
        // alloc at request

        HANDLE Alloc();
        void AutoAlloc (HANDLE& handle);
        void Release (HANDLE event);
        void Clear();
    };

    // construction / destruction:

    CWaitableEventPool::CWaitableEventPool()
        : alive (TRUE)
    {
    }

    CWaitableEventPool::~CWaitableEventPool()
    {
        Clear();
        InterlockedExchange (&alive, FALSE);
    }

    // Meyer's singleton

    CWaitableEventPool* CWaitableEventPool::GetInstance()
    {
        static CWaitableEventPool instance;
        return &instance;
    }

    // recycling interface:
    // alloc at request

    HANDLE CWaitableEventPool::Alloc()
    {
        if (InterlockedCompareExchange (&alive, TRUE, TRUE))
        {
            CCriticalSectionLock lock (mutex);
            if (!handles.empty())
            {
                HANDLE result = handles.back();
                handles.pop_back();
                return result;
            }
        }

        return CreateEvent (NULL, TRUE, FALSE, NULL);
    }

    void CWaitableEventPool::AutoAlloc (HANDLE& handle)
    {
        if (handle != NULL)
            return;

        if (InterlockedCompareExchange (&alive, TRUE, TRUE))
        {
            CCriticalSectionLock lock (mutex);
            if (!handles.empty())
            {
                handle = handles.back();
                handles.pop_back();
                return;
            }
        }

        handle = CreateEvent (NULL, TRUE, FALSE, NULL);
    }

    void CWaitableEventPool::Release (HANDLE event)
    {
        ResetEvent (event);
        if (InterlockedCompareExchange (&alive, TRUE, TRUE))
        {
            CCriticalSectionLock lock (mutex);
            handles.push_back (event);
        }
        else
        {
            CloseHandle (event);
        }
    }

    void CWaitableEventPool::Clear()
    {
        CCriticalSectionLock lock (mutex);

        while (!handles.empty())
            CloseHandle (Alloc());
    }
}

// construction / destruction: manage event handle

COneShotEvent::COneShotEvent()
    : event (NULL)
    , state (FALSE)
{
}

COneShotEvent::~COneShotEvent()
{
    if (event != NULL)
        CWaitableEventPool::GetInstance()->Release (event);
}

// eventing interface

void COneShotEvent::Set()
{
    if (InterlockedExchange (&state, TRUE) == FALSE)
        if (event != NULL)
            SetEvent (event);
}

bool COneShotEvent::Test() const
{
    return state == TRUE;
}

void COneShotEvent::WaitFor()
{
    if (state == FALSE)
    {
        CWaitableEventPool::GetInstance()->AutoAlloc (event);
        if (state == FALSE)
            WaitForSingleObject (event, INFINITE);
    }
}

bool COneShotEvent::WaitForEndOrTimeout(DWORD milliSeconds)
{
    if (state == FALSE)
    {
        CWaitableEventPool::GetInstance()->AutoAlloc (event);
        return WaitForSingleObject (event, milliSeconds) == WAIT_OBJECT_0;
    }

    return true;
}

// construction / destruction: manage event handle

CWaitableEvent::CWaitableEvent()
    : event (CWaitableEventPool::GetInstance()->Alloc())
{
}

CWaitableEvent::~CWaitableEvent()
{
    CWaitableEventPool::GetInstance()->Release (event);
}

// eventing interface

void CWaitableEvent::Set()
{
    SetEvent (event);
}

void CWaitableEvent::Reset()
{
    ResetEvent (event);
}

bool CWaitableEvent::Test() const
{
    return WaitForSingleObject (event, 0) == WAIT_OBJECT_0;
}

void CWaitableEvent::WaitFor()
{
    WaitForSingleObject (event, INFINITE);
}

bool CWaitableEvent::WaitForEndOrTimeout(DWORD milliSeconds)
{
    return WaitForSingleObject (event, milliSeconds) == WAIT_OBJECT_0;
}


}
