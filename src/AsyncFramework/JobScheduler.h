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

#include "CriticalSection.h"
#include "WaitableEvent.h"

namespace async
{

// forward declarations

class IJob;
class CThread;

/**
 * Central job execution management class. It encapsulates
 * a private worker thread pool and a \ref queue of \ref IJob
 * instance to be executed.
 *
 * There may be many instances of this class. One should use
 * a separate job scheduler for every different kind of resource
 * such as disk, network or CPU. By that, I/O-heavy jobs can
 * be executed mainly independently of CPU load, for instance.
 *
 * Jobs are guarateed to be picked from the queue in strict
 * order but will retire in an arbitrary order depending on
 * the time it takes to execute them. The \ref Schedule method
 * will decide whether more theads schould be activated --
 * depending on the \ref aggressiveThreadStart property and
 * the number of entries in the \ref queue.
 *
 * The worker thread use \ref AssignJob to get the next job
 * from the queue. If there is no such job (queue is empty),
 * the thread gets suspended. Have one or more threads been
 * allocated from the shared pool, the suspended thread will
 * be moved to that shared pool.
 *
 * Upon construction, a fixed number (may be 0) of private
 * worker threads gets created. On top of that, additional
 * threads may be allocated from a singleton shared thread pool.
 * This number is specified by the second constructor parameter.
 *
 * The instance returned by \ref GetDefault() is created with
 * (0, INT_MAX, false), thus is suitable for short-running
 * CPU-heavy jobs. Under high load, it will utilize all CPU
 * resources but not exceed them, i.e. the number of shared
 * worker threads.
 *
 * Please note that the job queue must be empty upon destruction.
 * You may call \ref WaitForEmptyQueue() to achieve this.
 * If you use multiple job schedulers, you may also use that
 * method to wait efficiently for a whole group of jobs
 * to be finished.
 *
 * Another related method is \ref WaitForSomeJobs. It will
 * effiently wait for at least one job to be finished unless
 * the job queue is almost empty (so you don't deadlock waiting
 * for yourself to finish). Call it if some *previous* job must
 * still provide some information that you are waiting for but
 * you don't know the particular job instance. Usually you will
 * need to call this method in a polling loop.
 */

class CJobScheduler
{
private:

    /// access sync

    mutable CCriticalSection mutex;

    /// jobs

    typedef std::pair<IJob*, bool> TJob;

    /**
     * Very low-overhead job queue class.
     * All method calls are amortized O(1) with an
     * average execution time of <10 ticks.
     *
     * Methods have been named and modeled similar
     * to std::queue<>.
     *
     * The data is stored in a single memory buffer.
     * Valid data is [\ref first, \ref last) and is
     * not moved around unless there is an overflow.
     * Overflow condition is \ref last reaching \ref end,
     * i.e. there is no space to add another entry.
     *
     * In that case, all content will either be shifted
     * to the beginning of the buffer (if less than 50%
     * are currently being used) or the buffer itself
     * gets doubled in size.
     */

    class CQueue
    {
    private:

        /// buffer start

        TJob* data;

        /// oldest entry, i.e. first to be extracted.

        TJob* first;

        /// first entry behind the most recently added one.

        TJob* last;

        /// first entry behind the allocated buffer

        TJob* end;

        /// if set, \ref pop removes entries at the front (\ref first).
        /// Otherwise, it will remove them from the back (\ref last).

        bool fifo;

        /// size management

        void Grow (size_t newSize);
        void AutoGrow();

    public:

        /// construction / destruction

        CQueue()
            : data (NULL)
            , first (NULL)
            , last (NULL)
            , end (NULL)
            , fifo (true)
        {
            Grow (1024);
        }
        ~CQueue()
        {
            delete[] data;
        }

        /// queue I/O

        void push (const TJob& job)
        {
            *last = job;
            if (++last == end)
                AutoGrow();
        }
        const TJob& pop()
        {
            return fifo ? *(first++) : *--last;
        }

        /// I/O order

        bool get_fifo() const
        {
            return fifo;
        }
        void set_fifo (bool newValue)
        {
            fifo = newValue;
        }

        /// size info

        bool empty() const
        {
            return first == last;
        }
        size_t size() const
        {
            return last - first;
        }
        size_t capacity() const
        {
            return end - data;
        }
    };

    /// queue of jobs not yet assigned to tasks.

    CQueue queue;

    /// per-thread info

    struct SThreadInfo
    {
        /// Job scheduler currently owning this job.
        /// \a NULL, if this is a shared thread currently
        /// not assigned to any scheduler.

        CJobScheduler* owner;

        /// the actual thread management object

        CThread* thread;
    };

    /**
     * Global pool of shared threads. This is a singleton class.
     *
     * Job schedulers will borrow from this pool via \ref TryAlloc
     * and release (not necessarily the same) threads via
     * \ref Release as as they are no longer used.
     *
     * The number of threads handed out to job scheudulers
     * (\ref allocCount) plus the number of threads still in the
     * \ref pool will not exceed \ref maxCount. Surplus threads
     * may appear if \ref maxCount has been reduced through
     * \ref SetThreadCount. Such threads will be removed
     * automatically in \ref Release.
     *
     * To maximize thread usage, job schedulers may register
     * as "starving". Those will be notified in a FCFS pattern
     * as soon as a thread gets returned to the pool. Hence,
     * it's their chance to allocate additional threads to
     * execute their jobs quicker.
     */

    class CThreadPool
    {
    private:

        /// list of idle shared threads

        std::vector<SThreadInfo*> pool;

        /// number of shared threads currently to job schedulers
        /// (hence, not in \ref pool at the moment).

        size_t allocCount;

        /// maximum number of entries in \ref pool

        mutable size_t maxCount;

        /// number of threads that may still be created (lazily)

        size_t yetToCreate;

        /// access sync. object

        CCriticalSection mutex;

        /// schedulers that would like to have more threads left

        std::vector<CJobScheduler*> starving;

        /// no public construction

        CThreadPool();

        // prevent cloning
        CThreadPool(const CThreadPool&) = delete;
        CThreadPool& operator=(const CThreadPool&) = delete;

        /// remove one entry from \ref starving container.
        /// Return NULL, if container was empty

        CJobScheduler* SelectStarving();

    public:

        /// release threads

        ~CThreadPool();

        /// Meyer's singleton

        static CThreadPool& GetInstance();

        /// pool interface

        SThreadInfo* TryAlloc();
        void Release (SThreadInfo* thread);

        /// set max. number of concurrent threads

        void SetThreadCount (size_t count);

        /// get maximum number of shared threads

        size_t GetThreadCount() const;

        /// manage starving schedulers
        /// (must be notified as soon as there is an idle thread)

        /// entry will be auto-removed upon notification

        void AddStarving (CJobScheduler* scheduler);

        /// must be called before destroying the \ref scheduler.
        /// No-op, if not in \ref starved list.
        /// \returns true if it was found.

        bool RemoveStarving (CJobScheduler* scheduler);
    };

    /**
     * Per-scheduler thread pool. It comprises of two sub-pools:
     * \ref running contains all threads that currenty execute
     * jobs whereas \ref suspended holds currently unused threads.
     *
     * If the latter is depleted, up to \ref maxFromShared
     * threads may be allocated from the shared thread pool
     * (\ref CThreadPool). As soon as threads are no longer
     * 'running', return them to the shared thread pool until
     * \ref fromShared is 0.
     *
     * Should the global pool be exhausted while we would like
     * to allocate more threads, register as "starved".
     *
     * Scheduler-private threads will be created lazily, i.e.
     * if \ref suspended is empty and \ref yetToCreate is not 0
     * yet, the latter will be decremented and a new thread is
     * being started instead of allocating a shared one.
     */

    struct SThreads
    {
        std::vector<SThreadInfo*> running;
        std::vector<SThreadInfo*> suspended;

        /// for speed, cache size() value from the vectors

        size_t runningCount;
        size_t suspendedCount;

        /// number of private threads that may still be created.
        /// (used to start threads lazily).

        size_t yetToCreate;

        /// how many of the \ref running threads have been allocated
        /// from \ref CThreadPool. Must be 0, if \ref suspended is
        /// not empty.

        size_t fromShared;

        /// how many of the \ref running threads have been allocated
        /// from \ref CThreadPool.

        size_t maxFromShared;

        /// suspendedCount + maxFromShared - fromShared
        size_t unusedCount;

        /// if set, we registered at shared pool as "starved"
        volatile bool starved;

        /// if > 0, queued jobs won't get assigned execution threads
        volatile long stopCount;
    };

    SThreads threads;

    /// this will be signalled when the last job is finished

    CWaitableEvent emptyEvent;

    /// number of threads in \ref WaitForSomeJobs

    volatile LONG waitingThreads;

    /// this will be signalled when a thread gets suspended

    CWaitableEvent threadIsIdle;

    /// if this is set, worker threads will be resumed
    /// unconditionally whenever a new job gets added.
    /// Also, worker threads will be destroyed
    /// as soon as the job queue runs low (i.e. threads
    /// won't be kept around in suspended state).

    bool aggressiveThreading;

    /// worker thread function

    static bool ThreadFunc (void* arg);

    /// job execution helper

    TJob AssignJob (SThreadInfo* info);

    /// try to get a thread from the shared pool.
    /// register as "starving" if that failed

    bool AllocateSharedThread();

    /// try to resume or create a private thread.
    /// If that fails, try to allocate a shared thread.

    bool AllocateThread();

    /// Unregister from "starving" list, if we are registered.
    /// This is non-trival as it may race with CThreadPool::Release
    /// trying to notify this instance.

    void StopStarvation();

    /// notification that a new thread may be available
    /// (to be called by CThreadPool only)

    void ThreadAvailable();

    friend class CThreadPool;

public:

    /// Create threads

    CJobScheduler ( size_t threadCount
                  , size_t sharedThreads
                  , bool aggressiveThreading = false
                  , bool fifo = true);

    /// End threads. Job queue must have run empty before calling this.

    ~CJobScheduler(void);

    /// This one will be used for jobs that have not been
    /// assigned to other job schedulers explicitly.

    static CJobScheduler* GetDefault();

    /// Add a new job to the queue. Wake further suspended
    /// threads as the queue gets larger.
    /// If \ref transferOwnership is set, the \ref job object
    /// will be deleted automatically after execution.

    void Schedule (IJob* job, bool transferOwnership);

    /// wait for all current and follow-up jobs to terminate

    void WaitForEmptyQueue();

    /// wait until either all current and follow-up jobs terminated
    /// or the specified timeout has passed. Returns false in case
    /// of a timeout.
    /// Please note that in cases of high contention, internal
    /// retries may cause the timeout to elapse more than once.

    bool WaitForEmptyQueueOrTimeout(DWORD milliSeconds);

    /// Wait for some jobs to be finished.
    /// This function may return immediately if there are
    /// too many threads waiting in this function.

    void WaitForSomeJobs();

    /// Returns the number of jobs waiting for execution.

    size_t GetQueueDepth() const;

    /// Returns the number of threads that currently execute
    /// jobs for this scheduler

    size_t GetRunningThreadCount() const;

    /// remove waiting entries from the queue until their
    /// number drops to or below the given watermark.

    std::vector<IJob*> RemoveJobFromQueue (size_t watermark, bool oldest = true);

    /// increment the internal stop counter. Until @ref Resume() was called
    /// just as often, no further jobs will be sent to the processing threads.
    /// Returns the new value of the stop counter;

    long Stop();

    /// decrement the internal stop counter. If it reaches 0, jobs will
    /// be sent to processing threads again.
    /// Returns the new value of the stop counter;

    long Resume();

    /// access properties of the \ref CThreadPool instances.

    static void SetSharedThreadCount (size_t count);
    static size_t GetSharedThreadCount();

    /// set number of shared threads to the number of
    /// HW threads (#CPUs x #Core/CPU x #SMT/Core)

    static void UseAllCPUs();

    /// \ref returns the number of HW threads.

    static size_t GetHWThreadCount();
};

}