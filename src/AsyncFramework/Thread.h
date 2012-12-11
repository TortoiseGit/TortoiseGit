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

#include "WaitableEvent.h"

/**
* Repeatedly executes a function until \ref Terminate
* is called. The function should not be long-running.
* If the function returns \a true, the thread will terminate
* automatically and delete the CThread instance.
*
* \ref Suspend and \ref Resume will not wait until the
* thread actually enters the respective state.
* \ref Terminate, however, will only return when \ref func
* has exited and will not be called again.
*
* Please note that this class is designed to be controlled
* by _one_ outer thread. Trying to control it from multiple
* threads may result in unstable behavior.
*/

namespace async
{

class CThread
{
private:

    /// the thread handle

    uintptr_t thread;

    /// if set, the thread function should exit asap

    volatile bool terminated;

    /// set, if the thread function has terminated

    CWaitableEvent done;

    /// allow for the thread to become inactive

    volatile bool suspended;
    CWaitableEvent suspend;
    CWaitableEvent resume;

    /// to be executed cyclicly

    bool (*func)(void *);
    void* args;

    /// the actual thread function

    static void ThreadFunc (void*);

public:

    /// auto-start thread during construction

    CThread (bool (*func)(void *), void* args, bool startSuspended = false);
    ~CThread(void);

    /// thread control

    void Suspend();
    void Resume();
    void Terminate();
};

}
