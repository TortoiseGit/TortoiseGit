/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class Attraction.
 *
 * \author Rene Weiskircher
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

#include <ogdf/internal/energybased/Attraction.h>

namespace ogdf {

const double Attraction::MULTIPLIER = 2.0;


//initializes internal data, like name and layout
Attraction::Attraction(GraphAttributes &AG) : NodePairEnergy("Attraction", AG) {

	reinitializeEdgeLength(MULTIPLIER);
}


//computes preferred edge length as the average of all widths and heights of the vertices
//multiplied by the multiplier
void Attraction::reinitializeEdgeLength(double multi)
{
	double lengthSum(0.0);
	node v;
	forall_nodes(v,m_G) {
		const IntersectionRectangle &i = shape(v);
		lengthSum += i.width();
		lengthSum += i.height();
	}
	lengthSum /= (2*m_G.numberOfNodes());
	// lengthSum is now the average of all lengths and widths
	m_preferredEdgeLength = multi * lengthSum;

}//reinitializeEdgeLength


//the energy of a pair of vertices is computed as the square of the difference between the
//actual distance and the preferred edge length
double Attraction::computeCoordEnergy(node v1, node v2, const DPoint &p1, const DPoint &p2)
const
{
	double energy = 0.0;
	if(adjacent(v1,v2)) {
		IntersectionRectangle i1(shape(v1)), i2(shape(v2));
		i1.move(p1);
		i2.move(p2);
		energy = i1.distance(i2) - m_preferredEdgeLength;
		energy *= energy;
	}
	return energy;
}


#ifdef OGDF_DEBUG
void Attraction::printInternalData() const {
	NodePairEnergy::printInternalData();
	cout << "\nPreferred edge length: " << m_preferredEdgeLength;
}
#endif

}// namespace ogdf
