/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class WSPD (well-separated pair decomposition).
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

#include "WSPD.h"
#include "FastUtils.h"

namespace ogdf {

WSPD::WSPD(__uint32 maxNumNodes) : m_maxNumNodes(maxNumNodes)
{
	m_maxNumPairs = maxNumNodes*2;
	m_numPairs = 0;
	allocate();
	clear();
}


WSPD::~WSPD(void)
{
	deallocate();
}


unsigned long WSPD::sizeInBytes() const
{
	return m_maxNumNodes*sizeof(WSPDNodeInfo) +
		m_maxNumPairs*sizeof(WSPDPairInfo);
}


void WSPD::allocate()
{
	m_nodeInfo = (WSPDNodeInfo*)MALLOC_16(m_maxNumNodes*sizeof(WSPDNodeInfo));
	m_pairs = (WSPDPairInfo*)MALLOC_16(m_maxNumPairs*sizeof(WSPDPairInfo));
}


void WSPD::deallocate()
{
	FREE_16(m_nodeInfo);
	FREE_16(m_pairs);
}


void WSPD::clear()
{
	for (__uint32 i = 0; i < m_maxNumNodes; i++)
	{
		m_nodeInfo[i].numWSNodes = 0;
	}
	m_numPairs = 0;
}

} // end of namespace ogdf
