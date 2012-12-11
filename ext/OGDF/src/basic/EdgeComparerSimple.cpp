/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of EdgeComparerSimple.
 *
 * Compare incident edges of a node based on the position of
 * the last bend point or the position of the adjacent node
 * given by the GraphAttributes
 *
 * \author Bernd Zey
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


#include <ogdf/basic/EdgeComparerSimple.h>
#include <ogdf/basic/geometry.h>

namespace ogdf {


int EdgeComparerSimple::compare(const adjEntry &e1, const adjEntry &e2) const
{
	// set true if the algorithm should consider the bend-points
	bool useBends = true;

	double xP1, xP2, yP1, yP2;

	DPolyline poly = m_AG->bends(e1->theEdge());
	ListIterator<DPoint> it;
	DPoint pE1, pE2;

	if ((useBends) && (poly.size() > 2)){
		it = poly.begin();

		while (it.valid()){
			it++;
		}

		if (e1->theEdge()->source() == basis){
			it = poly.begin();
			it++;
		}
		else{
			it = poly.rbegin();
			it--;
		}
		pE1 = *it;
	}
	else{
		pE1.m_x = m_AG->x((e1->twinNode()));
		pE1.m_y = m_AG->y((e1->twinNode()));
	}

	poly = m_AG->bends(e2->theEdge());
	if ((useBends) && (poly.size() > 2)){
		it = poly.begin();

		while (it.valid()){
			it++;
		}

		if (e2->theEdge()->source() == basis){
			it = poly.begin();
			it++;
		}
		else{
			it = poly.rbegin();
			it--;
		}
		pE2 = *it;
	}
	else{
		pE2.m_x = m_AG->x((e2->twinNode()));
		pE2.m_y = m_AG->y((e2->twinNode()));
	}


	xP1 = -(m_AG->x(basis)) + (pE1.m_x);
	yP1 = -(m_AG->y(basis)) + (pE1.m_y);

	xP2 = -(m_AG->x(basis)) + (pE2.m_x);
	yP2 = -(m_AG->y(basis)) + (pE2.m_y);

	if ((yP1 >= 0) && (yP2 < 0))
		return 1;
	if ((yP1 < 0) && (yP2 >= 0))
		return -1;
	if ((yP1 >= 0) && (yP2 >= 0)){

		if ((xP1 >= 0) && (xP2 < 0))
			return -1;
		if ((xP1 < 0) && (xP2 >= 0))
			return 1;

		xP1 = xP1 / (sqrt(xP1*xP1 + yP1*yP1));
		xP2 = xP2 / (sqrt(xP2*xP2 + yP2*yP2));
		if (xP1 > xP2)
			return -1;
		else
			return 1;
	}
	if ((yP1 < 0) && (yP2 < 0)){

		if ((xP1 >= 0) && (xP2 < 0))
			return 1;
		if ((xP1 < 0) && (xP2 >= 0))
			return -1;

		xP1 = xP1 / (sqrt(xP1*xP1 + yP1*yP1));
		xP2 = xP2 / (sqrt(xP2*xP2 + yP2*yP2));
		if (xP1 > xP2)
			return 1;
		else
			return -1;
	}

	return 0;
}

}//namespace ogdf
