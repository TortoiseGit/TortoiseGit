/***************************************************************************
 *   Copyright (C) 2009-2010 by Stefan Fuhrmann                            *
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
#include "JobBase.h"
#include "JobScheduler.h"

namespace async
{

// For now, update the internal @a scheduled flag only.

void CJobBase::OnSchedule (CJobScheduler*)
{
    if (InterlockedExchange (&scheduled, TRUE) == TRUE)
        assert (0);
}

// For now, update the internal @a scheduled flag only.

void CJobBase::OnUnSchedule (CJobScheduler*)
{
    if (InterlockedExchange (&scheduled, FALSE) == FALSE)
        assert (0);
    else
        if (executionDone.Test())
            deletable.Set();
}

// nothing special during construction / destuction

CJobBase::CJobBase(void)
    : waiting (TRUE)
    , terminated (FALSE)
    , scheduled (FALSE)
{
}

CJobBase::~CJobBase(void)
{
    assert (deletable.Test());
}

// call this to put the job into the scheduler

void CJobBase::Schedule (bool transferOwnership, CJobScheduler* scheduler)
{
    if (scheduler == NULL)
        scheduler = CJobScheduler::GetDefault();

    scheduler->Schedule (this, transferOwnership);
}

// will be called by job execution thread

void CJobBase::Execute()
{
    // intended for one-shot execution only
    // skip jobs that have already been executed or
    // are already/still being executed.

    if (InterlockedExchange (&waiting, FALSE) == TRUE)
    {
        if (terminated == FALSE)
            InternalExecute();

        executionDone.Set();

        if (scheduled == FALSE)
            deletable.Set();
    }
}

// may be called by other (observing) threads

IJob::Status CJobBase::GetStatus() const
{
    if (waiting == TRUE)
        return IJob::waiting;

    return executionDone.Test() ? IJob::done : IJob::running;
}

void CJobBase::WaitUntilDone (bool inlineExecution)
{
    if (inlineExecution)
        Execute();

    executionDone.WaitFor();
}

bool CJobBase::WaitUntilDoneOrTimeout(DWORD milliSeconds)
{
    return executionDone.WaitForEndOrTimeout(milliSeconds);
}

// handle early termination

void CJobBase::Terminate()
{
    InterlockedExchange (&terminated, TRUE);
}

bool CJobBase::HasBeenTerminated() const
{
    return terminated == TRUE;
}

// Call @a delete on this job as soon as it is safe to do so.

void CJobBase::Delete (bool terminate)
{
    if (terminate)
        Terminate();

    if (waiting)
        Execute();

    deletable.WaitFor();
    delete this;
}

}