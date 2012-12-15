/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of memory manager for more efficiently
 *        allocating small pieces of memory
 *
 * \author Carsten Gutwenger
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


#include <ogdf/basic/basic.h>


namespace ogdf {


struct PoolMemoryAllocator::PoolVector
{
	MemElemPtr m_pool[ePoolVectorLength];
	PoolVector *m_prev;
};

struct PoolMemoryAllocator::PoolElement
{
	PoolVector *m_currentVector;
	MemElemPtr m_restHead;
	MemElemPtr m_restTail;
	__int16 m_index;
	__int16 m_restCount;
};

struct PoolMemoryAllocator::BlockChain
{
	char m_fill[eBlockSize-sizeof(void*)];
	BlockChain *m_next;
};


PoolMemoryAllocator::PoolElement PoolMemoryAllocator::s_pool[eTableSize];
PoolMemoryAllocator::MemElemPtr PoolMemoryAllocator::s_freeVectors;
PoolMemoryAllocator::BlockChainPtr PoolMemoryAllocator::s_blocks;

#ifdef OGDF_MEMORY_POOL_NTS
PoolMemoryAllocator::MemElemPtr PoolMemoryAllocator::s_tp[eTableSize];
#elif defined(OGDF_NO_COMPILER_TLS)
CriticalSection *PoolMemoryAllocator::s_criticalSection;
pthread_key_t PoolMemoryAllocator::s_tpKey;
#else
CriticalSection *PoolMemoryAllocator::s_criticalSection;
OGDF_DECL_THREAD PoolMemoryAllocator::MemElemPtr PoolMemoryAllocator::s_tp[eTableSize];
#endif


void PoolMemoryAllocator::init()
{
#ifndef OGDF_MEMORY_POOL_NTS
#ifdef OGDF_NO_COMPILER_TLS
	pthread_key_create(&s_tpKey,NULL);
#endif
	s_criticalSection = new CriticalSection(500);
#endif
	initThread();
}


void PoolMemoryAllocator::initThread() {
#if !defined(OGDF_MEMORY_POOL_NTS) && defined(OGDF_NO_COMPILER_TLS)
		pthread_setspecific(s_tpKey,calloc(eTableSize,sizeof(MemElemPtr)));
#endif
}


void PoolMemoryAllocator::cleanup()
{
	BlockChainPtr p = s_blocks;
	while(p != 0) {
		BlockChainPtr pNext = p->m_next;
		free(p);
		p = pNext;
	}

#ifndef OGDF_MEMORY_POOL_NTS
#ifdef OGDF_NO_COMPILER_TLS
	pthread_key_delete(s_tpKey);
#endif
	delete s_criticalSection;
#endif
}


bool PoolMemoryAllocator::checkSize(size_t nBytes) {
	return nBytes < eTableSize;
}


void *PoolMemoryAllocator::allocate(size_t nBytes) {
#if !defined(OGDF_MEMORY_POOL_NTS) && defined(OGDF_NO_COMPILER_TLS)
	MemElemPtr *pFreeBytes = ((MemElemPtr*)pthread_getspecific(s_tpKey))+nBytes;
#else
	MemElemPtr *pFreeBytes = s_tp+nBytes;
#endif
	if (OGDF_LIKELY(*pFreeBytes != 0)) {
		MemElemPtr p = *pFreeBytes;
		*pFreeBytes = p->m_next;
		return p;
	} else {
		return fillPool(*pFreeBytes,__uint16(nBytes));
	}
}


void PoolMemoryAllocator::deallocate(size_t nBytes, void *p) {
#if !defined(OGDF_MEMORY_POOL_NTS) && defined(OGDF_NO_COMPILER_TLS)
	MemElemPtr *pFreeBytes = ((MemElemPtr*)pthread_getspecific(s_tpKey))+nBytes;
#else
	MemElemPtr *pFreeBytes = s_tp+nBytes;
#endif
	MemElemPtr(p)->m_next = *pFreeBytes;
	*pFreeBytes = MemElemPtr(p);
}


void PoolMemoryAllocator::deallocateList(size_t nBytes, void *pHead, void *pTail) {
#if !defined(OGDF_MEMORY_POOL_NTS) && defined(OGDF_NO_COMPILER_TLS)
	MemElemPtr *pFreeBytes = ((MemElemPtr*)pthread_getspecific(s_tpKey))+nBytes;
#else
	MemElemPtr *pFreeBytes = s_tp+nBytes;
#endif
	MemElemPtr(pTail)->m_next = *pFreeBytes;
	*pFreeBytes = MemElemPtr(pHead);
}


PoolMemoryAllocator::MemElemExPtr
PoolMemoryAllocator::collectGroups(
	__uint16 nBytes,
	MemElemPtr &pRestHead,
	MemElemPtr &pRestTail,
	int &nRest)
{
	int n = slicesPerBlock(nBytes);
	pRestHead = 0;

#if !defined(OGDF_MEMORY_POOL_NTS) && defined(OGDF_NO_COMPILER_TLS)
	MemElemPtr p = ((MemElemPtr*)pthread_getspecific(s_tpKey))[nBytes];
#else
	MemElemPtr p = s_tp[nBytes];
#endif
	MemElemExPtr pStart = 0, pLast = 0;
	while(p != 0)
	{
		int i = 0;
		MemElemPtr pHead = p, pTail;
		do {
			pTail = p;
			p = p->m_next;
		} while(++i < n && p != 0);

		pTail->m_next = 0;
		if(i == n) {
			if(pStart == 0)
				pStart = MemElemExPtr(pHead);
			else
				pLast->m_down = MemElemExPtr(pHead);
			pLast = MemElemExPtr(pHead);

		} else {
			pRestHead = pHead;
			pRestTail = pTail;
			nRest = i;
		}
	}
	if (pLast)
		pLast->m_down = 0;

	return pStart;
}


void PoolMemoryAllocator::flushPoolSmall(__uint16 nBytes)
{
	int n = slicesPerBlock(nBytes < eMinBytes ? eMinBytes : nBytes);
	PoolElement &pe = s_pool[nBytes];

#if !defined(OGDF_MEMORY_POOL_NTS) && defined(OGDF_NO_COMPILER_TLS)
	MemElemPtr p = ((MemElemPtr*)pthread_getspecific(s_tpKey))[nBytes];
#else
	MemElemPtr p = s_tp[nBytes];
#endif
	if(pe.m_restHead != 0) {
		pe.m_restTail->m_next = p;
		p = pe.m_restHead;
		pe.m_restHead = 0;
	}

	while(p != 0)
	{
		int i = 0;
		MemElemPtr pHead = p, pTail;
		do {
			pTail = p;
			p = p->m_next;
		} while(++i < n && p != 0);

		if(i == n) {
			incVectorSlot(pe);
			pe.m_currentVector->m_pool[pe.m_index] = pHead;

		} else {
			pe.m_restHead = pHead;
			pe.m_restTail = pTail;
			pe.m_restCount = i;
		}
	}
}


void PoolMemoryAllocator::incVectorSlot(PoolElement &pe)
{
	if(pe.m_currentVector == 0 || ++pe.m_index == ePoolVectorLength) {
		if(s_freeVectors == 0)
			s_freeVectors = allocateBlock(sizeof(PoolVector));

		PoolVector *pv = (PoolVector *)s_freeVectors;
		s_freeVectors = MemElemPtr(pv)->m_next;
		pe.m_currentVector = pv;
		pe.m_index = 0;
	}
}


void PoolMemoryAllocator::flushPool(__uint16 nBytes)
{
#ifndef OGDF_MEMORY_POOL_NTS
	if(nBytes >= sizeof(MemElemEx)) {
		MemElemPtr pRestHead, pRestTail;
		int nRest;
		MemElemExPtr pStart = collectGroups(nBytes, pRestHead, pRestTail, nRest);

		s_criticalSection->enter();
		PoolElement &pe = s_pool[nBytes];

		while(pStart != 0) {
			incVectorSlot(pe);
			pe.m_currentVector->m_pool[pe.m_index] = MemElemPtr(pStart);
			pStart = pStart->m_down;
		}
		if(pRestHead != 0) {
			int n = slicesPerBlock(nBytes);
			pRestTail->m_next = pe.m_restTail;
			int nTotal = nRest + pe.m_restCount;
			if(nTotal >= n) {
				MemElemPtr p = pe.m_restHead;
				int i = n-nRest;
				while(--i > 0)
					p = p->m_next;
				pe.m_restHead = p->m_next;
				pe.m_restCount = nTotal-n;
				incVectorSlot(pe);
				pe.m_currentVector->m_pool[pe.m_index] = pRestHead;
			} else {
				pe.m_restHead = pRestHead;
				pe.m_restCount = nTotal;
			}
		}
		s_criticalSection->leave();

	} else {
		s_criticalSection->enter();
		flushPoolSmall(nBytes);
		s_criticalSection->leave();
	}
#endif
}


void PoolMemoryAllocator::flushPool()
{
#ifndef OGDF_MEMORY_POOL_NTS
	for(__uint16 nBytes = 1; nBytes < eTableSize; ++nBytes) {
#ifdef OGDF_NO_COMPILER_TLS
	MemElemPtr p = ((MemElemPtr*)pthread_getspecific(s_tpKey))[nBytes];
#else
	MemElemPtr p = s_tp[nBytes];
#endif
		if(p != 0)
			flushPool(nBytes);
	}
#endif
}


void *PoolMemoryAllocator::fillPool(MemElemPtr &pFreeBytes, __uint16 nBytes)
{
#ifdef OGDF_MEMORY_POOL_NTS
	pFreeBytes = allocateBlock(nBytes);
#else

	s_criticalSection->enter();

	PoolElement &pe = s_pool[nBytes];
	if(pe.m_currentVector != 0) {
		pFreeBytes = pe.m_currentVector->m_pool[pe.m_index];
		if(--pe.m_index < 0) {
			PoolVector *pV = pe.m_currentVector;
			pe.m_currentVector = pV->m_prev;
			pe.m_index = ePoolVectorLength-1;
			MemElemPtr(pV)->m_next = s_freeVectors;
			s_freeVectors = MemElemPtr(pV);
		}
		s_criticalSection->leave();

	} else {
		s_criticalSection->leave();
		pFreeBytes = allocateBlock(nBytes);
	}
#endif

	MemElemPtr p = pFreeBytes;
	pFreeBytes = p->m_next;
	return p;
}


// __asm __volatile ("":::"memory")      GLIBC


PoolMemoryAllocator::MemElemPtr
PoolMemoryAllocator::allocateBlock(__uint16 nBytes)
{
	if(nBytes < eMinBytes)
		nBytes = eMinBytes;

	MemElemPtr pBlock = (MemElemPtr) malloc(eBlockSize);

	// we altogether create nSlices slices
	int nWords;
	int nSlices = slicesPerBlock(nBytes,nWords);

	MemElemPtr pHead = MemElemPtr(pBlock);
	BlockChainPtr(pBlock)->m_next = s_blocks;
	s_blocks = BlockChainPtr(pBlock);

	do {
		pBlock = pBlock->m_next = pBlock+nWords;
	} while(--nSlices > 1);
	MemElemPtr(pBlock)->m_next = 0;

	return pHead;
}


size_t PoolMemoryAllocator::memoryAllocatedInBlocks()
{
#ifndef OGDF_MEMORY_POOL_NTS
	s_criticalSection->enter();
#endif

	size_t nBlocks = 0;
	for (BlockChainPtr p = s_blocks; p != 0; p = p->m_next)
		++nBlocks;

#ifndef OGDF_MEMORY_POOL_NTS
	s_criticalSection->leave();
#endif

	return nBlocks * eBlockSize;
}


size_t PoolMemoryAllocator::memoryInGlobalFreeList()
{
#ifndef OGDF_MEMORY_POOL_NTS
	s_criticalSection->enter();
#endif

	size_t bytesFree = 0;
	for (int sz = 1; sz < eTableSize; ++sz)
	{
		const PoolElement &pe = s_pool[sz];
		PoolVector *pv = pe.m_currentVector;
		for(; pv != 0; pv = pv->m_prev)
			bytesFree += ePoolVectorLength*sz;
		if(pe.m_restHead != 0)
			bytesFree += pe.m_restCount;
	}

#ifndef OGDF_MEMORY_POOL_NTS
	s_criticalSection->leave();
#endif

	return bytesFree;
}


size_t PoolMemoryAllocator::memoryInThreadFreeList()
{
	size_t bytesFree = 0;
	for (int sz = 1; sz < eTableSize; ++sz)
	{
#if !defined(OGDF_MEMORY_POOL_NTS) && defined(OGDF_NO_COMPILER_TLS)
		MemElemPtr p = ((MemElemPtr*)pthread_getspecific(s_tpKey))[sz];
#else
		MemElemPtr p = s_tp[sz];
#endif
		for(; p != 0; p = p->m_next)
			bytesFree += sz;
	}

	return bytesFree;
}


}
