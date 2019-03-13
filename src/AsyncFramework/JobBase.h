/***************************************************************************
 *   Copyright (C) 2009-2010, 2012 by Stefan Fuhrmann                      *
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

#include "IJob.h"
#include "WaitableEvent.h"

namespace async
{

/**
 * Common base implementation of \ref IJob.
 * All your job class deriving from this one
 * has to do is to implement \ref InternalExecute.
 */

class CJobBase : public IJob
{
private:

    /// waitable event. Will be set after \ref Execute() finished.

    COneShotEvent executionDone;

    /// waitable event. Will be set when the job can be deleted.

    COneShotEvent deletable;

    /// TRUE until Execute() is called

    volatile LONG waiting;

    /// if set, we should not run at all or at least try to terminate asap

    volatile LONG terminated;

    /// if set, \ref finished will not be signalled unless
    /// \ref Execute is called from the scheduler.

    volatile LONG scheduled;

    /// For now, update the internal @a scheduled flag only.

    void OnSchedule (CJobScheduler* scheduler) override;

    /// For now, update the internal @a scheduled flag only.

    void OnUnSchedule (CJobScheduler* scheduler) override;

protected:

    /// base class is not intended for creation

    CJobBase(void);

    /// implement this in your job class

    virtual void InternalExecute() = 0;

    /// asserts that the job is deletable

    virtual ~CJobBase(void);

public:

    /// call this to put the job into the scheduler

    virtual void Schedule (bool transferOwnership, CJobScheduler* scheduler) override;

    // will be called by job execution thread

    virtual void Execute() override;

    /// may be called by other (observing) threads

    virtual Status GetStatus() const override;

    /// wait until job execution finished.
    /// If @ref inlineExecution is set, the job will be
    /// executed in the current thread if it is still waiting.

    virtual void WaitUntilDone (bool inlineExecution = false) override;

    /// returns false in case of a timeout

    virtual bool WaitUntilDoneOrTimeout(DWORD milliSeconds);

    /// request early termination.
    /// Will even prevent execution if not yet started.
    /// Execution will still finish 'successfully', i.e
    /// results in \ref IJob::done state.

    virtual void Terminate();

    /// \returns true if termination has been requested.
    /// Please note that execution may still be ongoing
    /// despite the termination request.

    virtual bool HasBeenTerminated() const;

    /// Call @a delete on this job as soon as it is safe to do so.

    virtual void Delete (bool terminate);

};

}
