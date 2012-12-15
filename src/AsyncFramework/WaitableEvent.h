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

#pragma once

namespace async
{

/**
* A waitable event that can be signaled only once.
* Also, only one waiting thread is allowed.
*
* This is a more lightweight implementation than
* \ref CWaitableEvent as the \ref event handle is
* only allocated if \ref WaitFor() requires it.
*/

class COneShotEvent
{
private:

    /// OS-specific event object

    HANDLE event;
    volatile LONG state;

public:

    /// construction / destruction: manage event handle

    COneShotEvent();
    ~COneShotEvent();

    /// eventing interface

    void Set();
    bool Test() const;
    void WaitFor();

    /// returns false in case of a timeout

    bool WaitForEndOrTimeout(DWORD milliSeconds);
};

/**
* A waitable event that must be reset manually.
* This implementation uses a global pool of event
* handles to minimize OS handle creation and destruction
* overhead.
*/

class CWaitableEvent
{
private:

    /// OS-specific event object

    HANDLE event;

public:

    /// construction / destruction: manage event handle

    CWaitableEvent();
    ~CWaitableEvent();

    /// eventing interface

    void Set();
    void Reset();
    bool Test() const;
    void WaitFor();

    /// returns false in case of a timeout

    bool WaitForEndOrTimeout(DWORD milliSeconds);
};

}
