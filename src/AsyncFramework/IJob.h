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

#pragma once

namespace async
{

// forward declaration

class CJobScheduler;

/**
 * Common interface to all job objects that can be handled by
 * \ref CJobScheduler instances.
 *
 * Jobs will be executed only once making the \ref done state
 * the final state of the internal state machine.
 */

class IJob
{
public:

    /**
     * Possible values returned by \ref GetStatus().
     * The state machine linearly follows the value order.
     */

    enum Status
    {
        waiting = 0,
        running = 1,
        suspended = 2,
        done = 3
    };

    /// destruction

    virtual ~IJob(void) {}

    /// call this to put the job into the scheduler.
    /// If \ref transferOwnership is set, the scheduler must
    /// delete this instance after execution.

    virtual void Schedule ( bool transferOwnership
                          , CJobScheduler* scheduler) = 0;

    /// will be called by job execution thread.

    virtual void Execute() = 0;

    /// may be called by other (observing) threads.
    /// Please note that the information returned may already
    /// be outdated as soon as the function returns.

    virtual Status GetStatus() const = 0;

    /// Efficiently wait for instance to reach the
    /// \ref done \ref Status.
    /// If \ref inlineExecution is set, the job will be
    /// executed in the current thread if it is still waiting.

    virtual void WaitUntilDone (bool inlineExecution = false) = 0;

private:

    /// Called by the \ref CJobScheduler instance before
    /// it actually add this job to its execution list.

    virtual void OnSchedule (CJobScheduler* scheduler) = 0;

    /// Called by the \ref CJobScheduler instance after
    /// it removed this job from its execution list.
    /// The job must not be deleted before this function
    /// has been called.

    virtual void OnUnSchedule (CJobScheduler* scheduler) = 0;

    // callbacks are ment to be used by \ref CJobScheduler only

    friend class CJobScheduler;
};

}