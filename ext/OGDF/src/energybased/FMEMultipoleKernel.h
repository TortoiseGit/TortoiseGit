/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class FMEMultipoleKernel.
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

#ifndef OGDF_FME_MULTIPOLE_KERNEL_H
#define OGDF_FME_MULTIPOLE_KERNEL_H

#include "FMEKernel.h"
#include "FMEFunc.h"
#include <algorithm>

namespace ogdf {

struct ArrayPartition
{
	__uint32 begin;
	__uint32 end;

	template<typename Func>
	void for_loop(Func& func)
	{
		for(__uint32 i=begin; i<=end; i++) func(i);
	}
};


class FMEMultipoleKernel : public FMEKernel
{
public:
	FMEMultipoleKernel(FMEThread* pThread) : FMEKernel(pThread) { }

	//! allocate the global and local contexts used by an instance of this kernel
	static FMEGlobalContext* allocateContext(ArrayGraph* pGraph, FMEGlobalOptions* pOptions, __uint32 numThreads);

	//! free the global and local context
	static void deallocateContext(FMEGlobalContext* globalContext);

	//! sub procedure for quadtree construction
	void quadtreeConstruction(ArrayPartition& nodePointPartition);

	//! the single threaded version without fences
	void multipoleApproxSingleThreaded(ArrayPartition& nodePointPartition);

	//! the original algorithm which runs the WSPD completely single threaded
	void multipoleApproxSingleWSPD(ArrayPartition& nodePointPartition);

	//! new but slower method, parallel wspd computation without using the wspd structure
	void multipoleApproxNoWSPDStructure(ArrayPartition& nodePointPartition);

	//! the final version, the wspd structure is only used for the top of the tree
	void multipoleApproxFinal(ArrayPartition& nodePointPartition);

	//! main function of the kernel
	void operator()(FMEGlobalContext* globalContext);

	//! creates an array partition with a default chunksize of 16
	inline ArrayPartition arrayPartition(__uint32 n)
	{
		return arrayPartition(n, threadNr(), numThreads(), 16);
	}

	//! returns an array partition for the given threadNr and thread count
	inline ArrayPartition arrayPartition(__uint32 n, __uint32 threadNr, __uint32 numThreads, __uint32 chunkSize)
	{
		ArrayPartition result;
		if (!n)
		{
			result.begin = 1;
			result.end = 0;
			return result;
		}
		if (n>=numThreads*chunkSize)
		{
			__uint32 s = n/(numThreads*chunkSize);
			__uint32 o = s*chunkSize*threadNr;
			if (threadNr == numThreads-1)
			{
				result.begin = o;
				result.end = n-1;
			}
			else
			{
				result.begin = o;
				result.end = o+s*chunkSize-1;
			}
		} else
		{
			if (threadNr == 0)
			{
				result.begin = 0;
				result.end = n-1;
			} else
			{
				result.begin = 1;
				result.end = 0;
			}
		}
		return result;
	}

	//! for loop on a partition
	template<typename F>
	inline void for_loop(const ArrayPartition& partition, F func)
	{
		if (partition.begin > partition.end)
			return;
		for (__uint32 i=partition.begin; i<= partition.end; i++) func(i);
	}

	//! for loop on the tree partition
	template<typename F>
	inline void for_tree_partition(F functor)
	{
		for (std::list<__uint32>::const_iterator it = m_pLocalContext->treePartition.nodes.begin();
			it!=m_pLocalContext->treePartition.nodes.end(); it++)
		{
			__uint32 node = *it;
			functor(node);
		}
	}

	//! sorting single threaded
	template<typename T, typename C>
	inline void sort_single(T* ptr, __uint32 n, C comparer)
	{
		if (isMainThread())
		{
			std::sort(ptr, ptr + n, comparer);
		}
	}

	//! lazy parallel sorting for num_threads = power of two
	template<typename T, typename C>
	inline void sort_parallel(T* ptr, __uint32 n, C comparer)
	{
		if ((n < numThreads()*1000) || (numThreads() == 1))
			sort_single(ptr, n, comparer);
		else
			sort_parallel(ptr, n, comparer, 0, numThreads());
	}

	//! lazy parallel sorting for num_threads = power of two
	template<typename T, typename C>
	inline void sort_parallel(T* ptr, __uint32 n, C comparer, __uint32 threadNrBegin, __uint32 numThreads)
	{
		if (n <= 1) return;
		if (numThreads == 1)
		{
			std::sort(ptr, ptr + n, comparer);
		}
		else
		{
			__uint32 half = n >> 1;
			__uint32 halfThreads = numThreads >> 1;
			if (this->threadNr() < threadNrBegin + halfThreads)
				sort_parallel(ptr, half, comparer, threadNrBegin, halfThreads);
			else
				sort_parallel(ptr + half, n - half, comparer, threadNrBegin + halfThreads, halfThreads);

			// wait until all threads are ready.
			sync();
			if (this->threadNr() == threadNrBegin)
				std::inplace_merge(ptr, ptr + half, ptr + n, comparer);
		}
	}

private:
	FMEGlobalContext* m_pGlobalContext;
	FMELocalContext* m_pLocalContext;
};

};

#endif
