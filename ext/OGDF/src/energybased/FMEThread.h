/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class FMEThread.
 *
 * \author Martin Gronemann
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_FME_THREAD_H
#define OGDF_FME_THREAD_H

#include <ogdf/basic/Thread.h>
#include <ogdf/basic/Barrier.h>
#include "FastUtils.h"

namespace ogdf {

class ArrayGraph;
class LinearQuadtree;
class LinearQuadtreeExpansion;
class FMEThreadPool;
class FMEThread;
class WSPD;

//! AutoLock class to easily lock a scope
class AutoLock
{
public:
	AutoLock(CriticalSection* cs) : m_pCriticalSection(cs)  { m_pCriticalSection->enter(); }

	~AutoLock() { m_pCriticalSection->leave(); }

private:
	CriticalSection* m_pCriticalSection;
};


/*!
 * The thread task class
 * used only as an interface
*/
class FMETask
{
public:
	virtual ~FMETask() { }
	// the doWork method to override
	virtual void doWork() = 0;
};


/*!
 * Class used to invoke a functor or function inside a thread.
*/
template<typename FuncInvokerType>
class FMEFuncInvokerTask : public FMETask
{
public:
	//! constructor with an invoker
	FMEFuncInvokerTask(FuncInvokerType f) : funcInvoker(f) { }

	//! overrides the task doWork() method and invokes the function or functor
	void doWork() {	funcInvoker(); }
private:
	//! the invoker
	FuncInvokerType funcInvoker;
};


/*!
 * The fast multipole embedder work thread class
*/
class FMEThread : public Thread
{
public:
	//! construtor
	FMEThread(FMEThreadPool* pThreadPool, __uint32 threadNr);

	//! returns the index of the thread ( 0.. numThreads()-1 )
	inline __uint32 threadNr() const { return m_threadNr; }

	//! returns the total number of threads in the pool
	inline __uint32 numThreads() const { return m_numThreads; }

	//! returns true if this is the main thread ( the main thread is always the first thread )
	inline bool isMainThread() const { return (m_threadNr == 0); }

	//! returns the ThreadPool this thread belongs to
	inline FMEThreadPool* threadPool() const { return m_pThreadPool; }

	//! thread sync call
	void sync();
#if defined(OGDF_SYSTEM_UNIX) && defined(OGDF_FME_THREAD_AFFINITY)
	void unixSetAffinity();

	//! the main work function
	void doWork() {  unixSetAffinity(); m_pTask->doWork(); delete m_pTask; m_pTask = 0; };
#else
	//! the main work function
	void doWork() { m_pTask->doWork(); delete m_pTask; m_pTask = 0; }
#endif
	//! sets the actual task
	void setTask(FMETask* pTask) { m_pTask = pTask; }

private:
	__uint32 m_threadNr;

	__uint32 m_numThreads;

	FMEThreadPool* m_pThreadPool;

	FMETask* m_pTask;
};


class FMEThreadPool
{
public:
	FMEThreadPool(__uint32 numThreads);

	~FMEThreadPool();

	//! returns the number of threads in this pool
	inline __uint32 numThreads() const { return m_numThreads; }

	//! returns the threadNr-th thread
	inline FMEThread* thread(__uint32 threadNr) const { return m_pThreads[threadNr]; }

	//! returns the barrier instance used to sync the threads during execution
	inline Barrier* syncBarrier() const { return m_pSyncBarrier; }

	//! runs one iteration. This call blocks the main thread
	void runThreads();

	template<typename KernelType, typename ArgType1>
	void runKernel(ArgType1 arg1)
	{
		for (__uint32 i=0; i < numThreads(); i++)
		{
			KernelType kernel(thread(i));
			FuncInvoker<KernelType, ArgType1> invoker(kernel, arg1);
			thread(i)->setTask(new FMEFuncInvokerTask< FuncInvoker< KernelType, ArgType1 > >(invoker));
		}
		runThreads();
	}

private:
	void allocate();

	void deallocate();

	__uint32 m_numThreads;

	FMEThread** m_pThreads;

	Barrier* m_pSyncBarrier;
};

}

#endif

