/***************************************************************************
 *   Copyright (C) 2009-2011 by Stefan Fuhrmann                            *
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
#include "JobScheduler.h"
#include "IJob.h"
#include "Thread.h"

namespace async
{

// queue size management

void CJobScheduler::CQueue::Grow (size_t newSize)
{
    TJob* newData = new TJob[newSize];

    size_t count = size();
    if (first)
        memmove (newData, first, count * sizeof (TJob[1]));
    delete[] data;

    data = newData;
    end = newData + newSize;
    first = newData;
    last = newData + count;
}

void CJobScheduler::CQueue::AutoGrow()
{
    size_t count = size();

    if (count * 2 <= capacity())
    {
        memmove (data, first, count * sizeof (TJob[1]));
        first = data;
        last = data + count;
    }
    else
    {
        Grow (capacity() * 2);
    }
}

// remove one entry from \ref starving container.
// Return NULL, if container was empty

CJobScheduler* CJobScheduler::CThreadPool::SelectStarving()
{
    CCriticalSectionLock lock (mutex);
    if (starving.empty() || pool.empty())
        return NULL;

    std::vector<CJobScheduler*>::iterator begin = starving.begin();

    CJobScheduler* scheduler = *begin;
    starving.erase (begin);

    return scheduler;
}

// create empty thread pool

CJobScheduler::CThreadPool::CThreadPool()
    : yetToCreate (0)
    , allocCount (0)
    , maxCount (0)
{
}

// release threads

CJobScheduler::CThreadPool::~CThreadPool()
{
    SetThreadCount(0);
}

// Meyer's singleton

CJobScheduler::CThreadPool& CJobScheduler::CThreadPool::GetInstance()
{
    static CThreadPool instance;
    return instance;
}

// pool interface

CJobScheduler::SThreadInfo* CJobScheduler::CThreadPool::TryAlloc()
{
    CCriticalSectionLock lock (mutex);
    if (pool.empty())
    {
        if (yetToCreate == 0)
            return NULL;

        // lazy thread creation

        SThreadInfo* info = new SThreadInfo;
        info->owner = NULL;
        info->thread = new CThread (&ThreadFunc, info, true);

        --yetToCreate;
        ++allocCount;

        return info;
    }

    CJobScheduler::SThreadInfo* thread = pool.back();
    pool.pop_back();
    ++allocCount;

    return thread;
}

void CJobScheduler::CThreadPool::Release (SThreadInfo* thread)
{
    {
        // put back into pool, unless its capacity has been exceeded

        CCriticalSectionLock lock (mutex);
        if (pool.size() + --allocCount < maxCount)
        {
            pool.push_back (thread);

            // shortcut

            if (starving.empty())
                return;
        }
        else
        {
            // pool overflow -> terminate thread

            delete thread->thread;
            delete thread;
        }
    }

    // notify starved schedulers that there may be some threads to alloc

    for ( CJobScheduler* scheduler = SelectStarving()
        ; scheduler != NULL
        ; scheduler = SelectStarving())
    {
        scheduler->ThreadAvailable();
    }
}

// set max. number of concurrent threads

void CJobScheduler::CThreadPool::SetThreadCount (size_t count)
{
    CCriticalSectionLock lock (mutex);

    maxCount = count;
    if (pool.size() + allocCount < maxCount)
        yetToCreate = maxCount - pool.size() + allocCount;

    while ((pool.size() + allocCount > maxCount) && !pool.empty())
    {
        SThreadInfo* info = pool.back();
        pool.pop_back();

        delete info->thread;
        delete info;
    }
}

size_t CJobScheduler::CThreadPool::GetThreadCount() const
{
    return maxCount;
}

// manage starving schedulers

void CJobScheduler::CThreadPool::AddStarving (CJobScheduler* scheduler)
{
    CCriticalSectionLock lock (mutex);
    starving.push_back (scheduler);
}

bool CJobScheduler::CThreadPool::RemoveStarving (CJobScheduler* scheduler)
{
    CCriticalSectionLock lock (mutex);

    typedef std::vector<CJobScheduler*>::iterator TI;
    TI begin = starving.begin();
    TI end = starving.end();

    TI newEnd = std::remove_copy (begin, end, begin, scheduler);
    if (newEnd == end)
        return false;

    starving.erase (newEnd, end);
    return true;
}

// job execution helpers

CJobScheduler::TJob CJobScheduler::AssignJob (SThreadInfo* info)
{
    CCriticalSectionLock lock (mutex);

    // wake up threads that waited for some work to finish

    if (waitingThreads > 0)
        threadIsIdle.Set();

    // suspend this thread if there is no work left

    if (queue.empty() || (threads.stopCount != 0))
    {
        // suspend

        info->thread->Suspend();

        // remove from "running" list and put it back either to
        // "suspended" pool or global shared pool

        bool terminateThread = false;
        for (size_t i = 0, count = threads.running.size(); i < count; ++i)
            if (threads.running[i] == info)
            {
                threads.running[i] = threads.running[count-1];
                threads.running.pop_back();

                // still in debt of shared pool?

                if (threads.fromShared > 0)
                {
                    // if we are actually idle, we aren't starving anymore

                    StopStarvation();

                    // put back to global pool

                    info->owner = NULL;
                    CThreadPool::GetInstance().Release (info);

                    --threads.fromShared;
                }
                else if (aggressiveThreading)
                {
                    // don't keep private idle threads

                    delete info;
                    ++threads.yetToCreate;
                    terminateThread = true;
                }
                else
                {

                    // add to local pool

                    threads.suspended.push_back (info);
                    ++threads.suspendedCount;
                }

                // signal empty queue, if necessary

                ++threads.unusedCount;
                if (--threads.runningCount == 0)
                    emptyEvent.Set();

                break;
            }

        return TJob(static_cast<IJob*>(nullptr), terminateThread);
    }

    // extract one job

    return queue.pop();
}

// try to get a thread from the shared pool.
// register as "starving" if that failed

bool CJobScheduler::AllocateSharedThread()
{
    if (!threads.starved)
    {
        SThreadInfo* info = CThreadPool::GetInstance().TryAlloc();
        if (info != NULL)
        {
            ++threads.fromShared;
            --threads.unusedCount;
            ++threads.runningCount;

            threads.running.push_back (info);
            info->owner = this;
            info->thread->Resume();

            return true;
        }
        else
        {
            threads.starved = true;
            CThreadPool::GetInstance().AddStarving (this);
        }
    }

    return false;
}

bool CJobScheduler::AllocateThread()
{
    if (threads.suspendedCount > 0)
    {
        // recycle suspended, private thread

        SThreadInfo* info = threads.suspended.back();
        threads.suspended.pop_back();

        --threads.suspendedCount;
        --threads.unusedCount;
        ++threads.runningCount;

        threads.running.push_back (info);
        info->thread->Resume();
    }
    else if (threads.yetToCreate > 0)
    {
        // time to start a new private thread

        --threads.yetToCreate;

        --threads.unusedCount;
        ++threads.runningCount;

        SThreadInfo* info = new SThreadInfo;
        info->owner = this;
        info->thread = new CThread (&ThreadFunc, info, true);
        threads.running.push_back (info);

        info->thread->Resume();
    }
    else
    {
        // try to allocate a shared thread

        if (threads.fromShared < threads.maxFromShared)
            return AllocateSharedThread();

        return false;
    }

    return true;
}

// unregister from "starving" list.
// This may race with CThreadPool::Release -> must loop here.

void CJobScheduler::StopStarvation()
{
    while (threads.starved)
        if (CThreadPool::GetInstance().RemoveStarving (this))
        {
            threads.starved = false;
            break;
        }
}

// worker thread function

bool CJobScheduler::ThreadFunc (void* arg)
{
    SThreadInfo* info = reinterpret_cast<SThreadInfo*>(arg);

    TJob job = info->owner->AssignJob (info);
    if (job.first != NULL)
    {
        // run the job

        job.first->Execute();

        // it is no longer referenced by the scheduler

        job.first->OnUnSchedule (info->owner);

        // is it our responsibility to clean up this job?

        if (job.second)
            delete job.first;

        // continue

        return false;
    }
    else
    {
        // maybe, auto-delete thread object

        return job.second;
    }
}

// Create & remove threads

CJobScheduler::CJobScheduler
    ( size_t threadCount
    , size_t sharedThreads
    , bool aggressiveThreading
    , bool fifo)
    : waitingThreads (0)
    , aggressiveThreading (aggressiveThreading)
{
    threads.runningCount = 0;
    threads.suspendedCount = 0;

    threads.fromShared = 0;
    threads.maxFromShared = sharedThreads;

    threads.unusedCount = threadCount + sharedThreads;
    threads.yetToCreate = threadCount;

    threads.starved = false;
    threads.stopCount = 0;

    queue.set_fifo (fifo);

    // auto-initialize shared threads

    if (GetSharedThreadCount() == 0)
        UseAllCPUs();
}

CJobScheduler::~CJobScheduler(void)
{
    StopStarvation();
    WaitForEmptyQueue();

    assert (threads.running.empty());
    assert (threads.fromShared == 0);

    for (size_t i = 0, count = threads.suspended.size(); i < count; ++i)
    {
        SThreadInfo* info = threads.suspended[i];
        delete info->thread;
        delete info;
    }
}

// Meyer's singleton

CJobScheduler* CJobScheduler::GetDefault()
{
    static CJobScheduler instance (0, SIZE_MAX);
    return &instance;
}

// job management:
// add new job

void CJobScheduler::Schedule (IJob* job, bool transferOwnership)
{
    TJob toAdd (job, transferOwnership);
    job->OnSchedule (this);

    CCriticalSectionLock lock (mutex);

    if (queue.empty())
        emptyEvent.Reset();

    queue.push (toAdd);
    if (threads.stopCount != 0)
        return;

    bool addThread = aggressiveThreading
                 || (   (queue.size() > 2 * threads.runningCount)
                     && (threads.unusedCount > 0));

    if (addThread)
        AllocateThread();
}

// notification that a new thread may be available

void CJobScheduler::ThreadAvailable()
{
    CCriticalSectionLock lock (mutex);
    threads.starved = false;

    while (   (threads.stopCount != 0)
           && (queue.size() > 2 * threads.runningCount)
           && (threads.suspendedCount == 0)
           && (threads.yetToCreate == 0)
           && (threads.fromShared < threads.maxFromShared))
    {
         if (!AllocateSharedThread())
            return;
    }
}

// wait for all current and follow-up jobs to terminate

void CJobScheduler::WaitForEmptyQueue()
{
    WaitForEmptyQueueOrTimeout(INFINITE);
}

bool CJobScheduler::WaitForEmptyQueueOrTimeout(DWORD milliSeconds)
{
    for (;;)
    {
        {
            CCriticalSectionLock lock (mutex);

            // if the scheduler has been stopped, we need to remove
            // the waiting jobs manually

            if (threads.stopCount != 0)
                while (!queue.empty())
                {
                    const TJob& job = queue.pop();

                    job.first->OnUnSchedule(this);
                    if (job.second)
                        delete job.first;
                }

            // empty queue and no jobs still being processed?

            if ((threads.runningCount == 0) && queue.empty())
                return true;

            // we will be woken up as soon as both containers are empty

            emptyEvent.Reset();
        }

        if (!emptyEvent.WaitForEndOrTimeout(milliSeconds))
            return false;
    }
    //return true;
}

// wait for some jobs to be finished.

void CJobScheduler::WaitForSomeJobs()
{
    {
        CCriticalSectionLock lock (mutex);
        if (  static_cast<size_t>(InterlockedIncrement (&waitingThreads))
            < threads.runningCount)
        {
            // there are enough running job threads left
            // -> wait for one of them to run idle *or*
            //    for too many of them to enter this method

            threadIsIdle.Reset();
        }
        else
        {
            // number of waiting jobs nor equal to or greater than
            // the number of running job threads
            // -> wake them all

            threadIsIdle.Set();
            InterlockedDecrement (&waitingThreads);

            return;
        }
    }

    threadIsIdle.WaitFor();
    InterlockedDecrement (&waitingThreads);
}

// Returns the number of jobs waiting for execution.

size_t CJobScheduler::GetQueueDepth() const
{
    CCriticalSectionLock lock (mutex);
    return queue.size();
}

// Returns the number of threads that currently execute
// jobs for this scheduler

size_t CJobScheduler::GetRunningThreadCount() const
{
    CCriticalSectionLock lock (mutex);
    return threads.runningCount;
}

// remove waiting entries from the queue until their
// number drops to or below the given watermark.

std::vector<IJob*> CJobScheduler::RemoveJobFromQueue
    ( size_t watermark
    , bool oldest)
{
    std::vector<IJob*> removed;

    {
        CCriticalSectionLock lock (mutex);
        if (queue.size() > watermark)
        {
            size_t toRemove = queue.size() - watermark;
            removed.reserve (toRemove);

            // temporarily change the queue extraction strategy
            // such that we remove jobs from the requested end
            // (fifo -> oldest are at front, otherwise they are
            // at the back)

            bool fifo = queue.get_fifo();
            queue.set_fifo (oldest == fifo);

            // remove 'em

            for (size_t i = 0; i < toRemove; ++i)
            {
                IJob* job = queue.pop().first;
                job->OnUnSchedule (this);

                removed.push_back (job);
            }

            // restore job execution order

            queue.set_fifo (fifo);
        }
    }

    return removed;
}

long CJobScheduler::Stop()
{
    CCriticalSectionLock lock (mutex);
    return ++threads.stopCount;
}

long CJobScheduler::Resume()
{
    CCriticalSectionLock lock (mutex);
    assert (threads.stopCount > 0);

    if (--threads.stopCount == 0)
    {
        while (   (    (queue.size() > threads.runningCount)
                    && aggressiveThreading)
               || (    (queue.size() > 4 * threads.runningCount)
                    && (threads.unusedCount > 0)))
        {
            if (!AllocateThread())
                break;
        }
    }

    return threads.stopCount;
}



// set max. number of concurrent threads

void CJobScheduler::SetSharedThreadCount (size_t count)
{
    CThreadPool::GetInstance().SetThreadCount(count);
}

size_t CJobScheduler::GetSharedThreadCount()
{
    return CThreadPool::GetInstance().GetThreadCount();
}

void CJobScheduler::UseAllCPUs()
{
    SetSharedThreadCount (GetHWThreadCount());
}

size_t CJobScheduler::GetHWThreadCount()
{
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);

    size_t sysNumProcs = si.dwNumberOfProcessors;
#else
    size_t sysNumProcs = sysconf (_SC_NPROCESSORS_CONF);
#endif

    return sysNumProcs;
}

}
