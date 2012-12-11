/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class Overlap
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


#include <ogdf/internal/energybased/Overlap.h>

namespace ogdf {

	Overlap::Overlap(GraphAttributes &AG) : NodePairEnergy("Overlap",AG){}

	double Overlap::computeCoordEnergy(node v1, node v2, const DPoint &p1, const DPoint &p2)
		const
	{
		IntersectionRectangle i1(shape(v1)), i2(shape(v2));
		i1.move(p1);
		i2.move(p2);
		IntersectionRectangle intersection = i1.intersection(i2);
		double area = intersection.area();
		if(area < 0.0) {
			OGDF_ASSERT(area > -0.00001);
			area = 0.0;
		}
		double minArea = min(i1.area(),i2.area());
		double energy = area / minArea;
		return energy;
	}

}// namespace ogdf

