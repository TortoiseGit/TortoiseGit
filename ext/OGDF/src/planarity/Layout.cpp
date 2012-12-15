/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of class Layout
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

#include <ogdf/basic/Layout.h>
#include <ogdf/planarity/PlanRep.h>


namespace ogdf {


void Layout::computePolyline(GraphCopy &GC, edge eOrig, DPolyline &dpl) const
{
	dpl.clear();

	const List<edge> &edgePath = GC.chain(eOrig);

	// The corresponding edge path in the copy must contain at least 1 edge!
	OGDF_ASSERT(edgePath.size() >= 1);

	// iterate over all edges in the corresponding edge path in the copy
	ListConstIterator<edge> it;
	edge e;
	bool firstTime = true;
	for(it = edgePath.begin(); it.valid(); ++it) {
		e = *it;
		node v = e->source();

		// append point of source node of e ...
		if (!firstTime)
			dpl.pushBack(DPoint(m_x[v],m_y[v]));
		else
			firstTime = false;

		// ... and polyline of e
		const DPolyline &segment = m_bends[e];
		ListConstIterator<DPoint> itSeg;

		for(itSeg = segment.begin(); itSeg.valid(); ++itSeg)
			dpl.pushBack(*itSeg);
	}
}


// faster version of computePolylineClear
// clears the list of bend points  of all edges in the edge path
// in the copy corresponding to eOrig!
void Layout::computePolylineClear(PlanRep &PG, edge eOrig, DPolyline &dpl)
{
	dpl.clear();

	const List<edge> &edgePath = PG.chain(eOrig);

	// The corresponding edge path in the copy must contain at least 1 edge!
	OGDF_ASSERT(edgePath.size() >= 1);

	// iterate over all edges in the corresponding edge path in the copy
	ListConstIterator<edge> it;
	edge e;
	bool firstTime = true;
	for(it = edgePath.begin(); it.valid(); ++it) {
		e = *it;
		node v = e->source();

		// append point of source node of e ...
		if (!firstTime)
			dpl.pushBack(DPoint(m_x[v],m_y[v]));
		else
			firstTime = false;

		// ... and polyline of e
		dpl.conc(m_bends[e]);
	}
	node w = e->target();
	if (PG.typeOf(w) == Graph::generalizationExpander)
		dpl.pushBack(DPoint(m_x[w],m_y[w]));
}



} // end namespace ogdf
