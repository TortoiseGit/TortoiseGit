/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Hashing (class HashingBase)
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

#include <ogdf/basic/Hashing.h>


namespace ogdf {

HashingBase::HashingBase(int minTableSize)
{
	m_count = 0;
	init(m_minTableSize = minTableSize);
}


HashingBase::HashingBase(const HashingBase &H)
{
	copyAll(H);
}


HashingBase::~HashingBase()
{
	free(m_table);
}


void HashingBase::init(int tableSize)
{
	OGDF_ASSERT(tableSize >= m_minTableSize)

	m_tableSize = tableSize;
	m_hashMask = tableSize-1;
	m_tableSizeHigh = tableSize << 1;
	m_tableSizeLow  = (tableSize > m_minTableSize) ? (tableSize >> 1) : -1;

	m_table = (HashElementBase **)calloc(tableSize,sizeof(HashElementBase *));
}


void HashingBase::destroyAll()
{
	HashElementBase **pList = m_table, **pListStop = m_table+m_tableSize;

	for(; pList != pListStop; ++pList) {
		HashElementBase *pElement = *pList, *pNext;
		for (; pElement; pElement = pNext) {
			pNext = pElement->next();
			destroy(pElement);
		}
	}
}


void HashingBase::copyAll(const HashingBase &H)
{
	m_count = 0;
	init(H.m_tableSize);

	HashElementBase **pList = H.m_table;
	HashElementBase **pListStop = H.m_table+m_tableSize;

	for(; pList != pListStop; ++pList) {
		HashElementBase *pElement = *pList;
		for (; pElement; pElement = pElement->next())
			insert(H.copy(pElement));
	}
}


void HashingBase::clear()
{
	destroyAll();
	free(m_table);

	m_count = 0;
	init(m_minTableSize);
}


HashingBase &HashingBase::operator=(const HashingBase &H)
{
	destroyAll();
	free(m_table);
	copyAll(H);
	return *this;
}


void HashingBase::resize(int newTableSize)
{
	HashElementBase **oldTable = m_table;
	HashElementBase **oldTableStop = oldTable + m_tableSize;

	init(newTableSize);

	for(HashElementBase **pOldList = oldTable;
		pOldList != oldTableStop; ++pOldList)
	{
		HashElementBase *pElement = *pOldList, *pNext;
		for(; pElement; pElement = pNext) {
			pNext = pElement->m_next;

			HashElementBase **pList = m_table +
				(pElement->m_hashValue & m_hashMask);
			pElement->m_next = *pList;
			*pList = pElement;
		}
	}

	free(oldTable);
}


void HashingBase::insert(HashElementBase *pElement)
{
	if (++m_count == m_tableSizeHigh)
		resize(m_tableSizeHigh);

	HashElementBase **pList = m_table + (pElement->m_hashValue & m_hashMask);
	pElement->m_next = *pList;
	*pList = pElement;
}


void HashingBase::del(HashElementBase *pElement)
{
	HashElementBase **pList = m_table + (pElement->m_hashValue & m_hashMask);
	HashElementBase *pPrev = *pList;

	if (pPrev == pElement) {
		*pList = pElement->m_next;

	} else {
		while (pPrev->m_next != pElement) pPrev = pPrev->m_next;
		pPrev->m_next = pElement->m_next;
	}

	if (--m_count == m_tableSizeLow)
		resize(m_tableSizeLow);
}


HashElementBase *HashingBase::firstElement(HashElementBase ***pList) const
{
	HashElementBase **pStop = m_table + m_tableSize;
	for(*pList = m_table; *pList != pStop; ++(*pList))
		if (**pList) return **pList;

	return 0;
}


HashElementBase *HashingBase::nextElement(HashElementBase ***pList,
	HashElementBase *pElement) const
{
	if ((pElement = pElement->next()) != 0) return pElement;

	HashElementBase **pStop = m_table + m_tableSize;
	for(++(*pList); *pList != pStop; ++(*pList))
		if (**pList) return **pList;

	return 0;
}



} // end namespace ogdf
