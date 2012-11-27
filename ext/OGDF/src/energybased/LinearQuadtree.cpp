/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class LinearQuadtree.
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

#include "LinearQuadtree.h"
#include "WSPD.h"

namespace ogdf {

void LinearQuadtree::init(float min_x, float min_y, float max_x, float max_y)
{
	m_min_x = min_x;
	m_min_y = min_y;
	m_max_x = max_x;
	m_max_y = max_y;
	m_sideLengthGrid = ((double)(0x1 << 24) - 1.0);
	m_sideLengthPoints = (double)max(m_max_x - m_min_x, m_max_y - m_min_y);
	m_scaleInv = (m_sideLengthGrid / m_sideLengthPoints);
	m_cellSize = m_sideLengthPoints /((double)m_sideLengthGrid);
	clear();
}


void LinearQuadtree::clear()
{
	m_numWSP = 0;
	m_numNotWSP = 0;
	m_numDirectNodes = 0;
	m_WSPD->clear();
}


LinearQuadtree::LinearQuadtree(__uint32 n, float* origXPos, float* origYPos, float* origSize) : m_origXPos(origXPos), m_origYPos(origYPos), m_origSize(origSize)
{
	allocate(n);
	m_numPoints = n;
	m_maxNumNodes = 2*n;
}


LinearQuadtree::~LinearQuadtree(void)
{
	deallocate();
}


void LinearQuadtree::allocate(__uint32 n)
{
	m_numPoints = n;
	m_maxNumNodes = 2*n;
	m_tree = (LQNode*)MALLOC_16(m_maxNumNodes*sizeof(LQNode));
	m_nodeXPos = (float*)MALLOC_16(m_maxNumNodes*sizeof(float));
	m_nodeYPos = (float*)MALLOC_16(m_maxNumNodes*sizeof(float));
	m_nodeSize = (float*)MALLOC_16(m_maxNumNodes*sizeof(float));
	m_points = (LQPoint*)MALLOC_16(m_numPoints*sizeof(LQPoint));
	for (__uint32 i = 0; i < m_numPoints; i++)
		m_points[i].ref = i;
	m_pointXPos = (float*)MALLOC_16(m_numPoints*sizeof(float));
	m_pointYPos = (float*)MALLOC_16(m_numPoints*sizeof(float));
	m_pointSize = (float*)MALLOC_16(m_numPoints*sizeof(float));
	m_notWspd = (LQWSPair*)MALLOC_16(m_maxNumNodes*sizeof(LQWSPair)*27);
	m_directNodes = (NodeID*)MALLOC_16(m_maxNumNodes*sizeof(NodeID));
	m_WSPD = new WSPD(m_maxNumNodes);
}


void LinearQuadtree::deallocate()
{
	FREE_16(m_tree);
	FREE_16(m_nodeXPos);
	FREE_16(m_nodeYPos);
	FREE_16(m_nodeSize);
	FREE_16(m_points);
	FREE_16(m_pointXPos);
	FREE_16(m_pointYPos);
	FREE_16(m_pointSize);
	FREE_16(m_notWspd);
	FREE_16(m_directNodes);
	delete m_WSPD;
}


__uint64 LinearQuadtree::sizeInBytes() const
{
	return m_numPoints*sizeof(LQPoint) +
		m_maxNumNodes*sizeof(LQNode) +
		m_maxNumNodes*sizeof(LQWSPair)*27 +
		m_maxNumNodes*sizeof(NodeID) +
		m_WSPD->sizeInBytes();
}


//! iterates back in the sequence until the first point with another morton number occures, returns that point +1
LinearQuadtree::PointID LinearQuadtree::findFirstPointInCell(LinearQuadtree::PointID somePointInCell) const
{
	if (somePointInCell==0) return 0;
	LinearQuadtree::PointID result = somePointInCell-1;
	while (mortonNr(somePointInCell) == mortonNr(result))
	{
		if (result==0) return 0;
		result--;
	}
	return result+1;
}


void LinearQuadtree::addWSPD(NodeID s, NodeID t)
{
	m_numWSP++;
	m_WSPD->addWSP(s, t);
}


void LinearQuadtree::addDirectPair(NodeID s, NodeID t)
{
	m_notWspd[m_numNotWSP].a = s;
	m_notWspd[m_numNotWSP].b = t;
	m_numNotWSP++;
}


void LinearQuadtree::addDirect(NodeID s)
{
	m_directNodes[m_numDirectNodes] = s;
	m_numDirectNodes++;
}

} // end of namespace ogdf
