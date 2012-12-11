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
#include "Thread.h"

namespace async
{

/// the actual thread function

void CThread::ThreadFunc (void* arg)
{
//    CCrashReportThread crashthread;
    CThread* self = reinterpret_cast<CThread*>(arg);

    // maybe, we were started in "suspended" mode

    if (self->suspended)
        self->resume.WaitFor();

    // main loop

    while (!self->terminated)
    {
        if ((*self->func)(self->args))
        {
            // auto-deletion

            self->terminated = true;
            self->done.Set();
            delete self;

            return;
        }

        if (self->suspended)
            self->resume.WaitFor();
    }

    // indicate that the thread is no longer processing any data

    self->done.Set();
}

/// auto-start thread during construction

CThread::CThread (bool (*func)(void *), void* args, bool startSuspended)
    : thread (NULL)
    , terminated (false)
    , suspended (startSuspended)
    , func (func)
    , args (args)
{
    thread = _beginthread (&ThreadFunc, 0, this);
}

CThread::~CThread(void)
{
    if (thread != 0)
        Terminate();
}

/// thread control

void CThread::Suspend()
{
    resume.Reset();
    suspended = true;
}

void CThread::Resume()
{
    suspended = false;
    resume.Set();
}

void CThread::Terminate()
{
    terminated = true;

    Resume();
    done.WaitFor();
}

}