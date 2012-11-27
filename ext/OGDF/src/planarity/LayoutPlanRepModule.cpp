/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of class LayoutPlanRepModule
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


#include <ogdf/module/LayoutPlanRepModule.h>


namespace ogdf {


void LayoutPlanRepModule::setBoundingBox(PlanRepUML &PG, Layout &drawing)
{
	double maxWidth  = 0;
	double maxHeight = 0;

	// check rightmost and uppermost extension of all (original) nodes
	const List<node> &origInCC = PG.nodesInCC(PG.currentCC());
	ListConstIterator<node> itV;

	for(itV = origInCC.begin(); itV.valid(); ++itV) {
		node vG = *itV;

		double maxX = drawing.x(PG.copy(vG)) + PG.widthOrig(vG)/2;
		if (maxX > maxWidth ) maxWidth  = maxX;

		double maxY = drawing.y(PG.copy(vG)) + PG.heightOrig(vG)/2;
		if (maxY > maxHeight) maxHeight = maxY;

		// check polylines of all (original) edges
		adjEntry adj;
		forall_adj(adj,vG) {
			if ((adj->index() & 1) == 0) continue;
			edge eG = adj->theEdge();

			const List<edge> &path = PG.chain(eG);

			ListConstIterator<edge> itPath;
			for(itPath = path.begin(); itPath.valid(); ++itPath)
			{
				edge e = *itPath;

				// we have to check (only) all interior points, i.e., we can
				// omitt the first and last point since it lies in the box of
				// the source or target node.
				// This version checks also the first for simplicity of the loop.
				node v = e->source();
				if (drawing.x(v) > maxWidth ) maxWidth  = drawing.x(v);
				if (drawing.y(v) > maxHeight) maxHeight = drawing.y(v);

				const DPolyline &dpl = drawing.bends(e);

				ListConstIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it)
				{
					const DPoint &dp = *it;
					if (dp.m_x > maxWidth ) maxWidth  = dp.m_x;
					if (dp.m_y > maxHeight) maxHeight = dp.m_y;
				}
			}
		}
	}


	m_boundingBox = DPoint(maxWidth,maxHeight);
}

} // end namespace ogdf
