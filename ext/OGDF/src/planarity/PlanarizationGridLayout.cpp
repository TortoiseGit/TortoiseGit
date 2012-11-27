/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements planarization with grid layout.
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


#include <ogdf/planarity/PlanarizationGridLayout.h>
#include <ogdf/planarity/FastPlanarSubgraph.h>
#include <ogdf/planarity/FixedEmbeddingInserter.h>
#include <ogdf/planarlayout/MixedModelLayout.h>
#include <ogdf/packing/TileToRowsCCPacker.h>


namespace ogdf {


PlanarizationGridLayout::PlanarizationGridLayout()
{
	m_subgraph      .set(new FastPlanarSubgraph);
	m_inserter      .set(new FixedEmbeddingInserter);
	m_planarLayouter.set(new MixedModelLayout);
	m_packer        .set(new TileToRowsCCPacker);

	m_pageRatio = 1.0;
}


void PlanarizationGridLayout::doCall(
	const Graph &G,
	GridLayout &gridLayout,
	IPoint &bb)
{
	m_nCrossings = 0;
	if(G.empty()) return;

	PlanRep PG(G);

	const int numCC = PG.numberOfCCs();
	// (width,height) of the layout of each connected component
	Array<IPoint> boundingBox(numCC);

	int i;
	for(i = 0; i < numCC; ++i)
	{
		PG.initCC(i);
		const int nOrigVerticesPG = PG.numberOfNodes();

		List<edge> deletedEdges;
		m_subgraph.get().callAndDelete(PG, deletedEdges);

		m_inserter.get().call(PG,deletedEdges);

		m_nCrossings += PG.numberOfNodes() - nOrigVerticesPG;

		GridLayout gridLayoutPG(PG);
		m_planarLayouter.get().callGrid(PG,gridLayoutPG);

		// copy grid layout of PG into grid layout of G
		ListConstIterator<node> itV;
		for(itV = PG.nodesInCC(i).begin(); itV.valid(); ++itV)
		{
			node vG = *itV;

			gridLayout.x(vG) = gridLayoutPG.x(PG.copy(vG));
			gridLayout.y(vG) = gridLayoutPG.y(PG.copy(vG));

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();
				IPolyline &ipl = gridLayout.bends(eG);
				ipl.clear();

				bool firstTime = true;
				ListConstIterator<edge> itE;
				for(itE = PG.chain(eG).begin(); itE.valid(); ++itE) {
					if(!firstTime) {
						node v = (*itE)->source();
						ipl.pushBack(IPoint(gridLayoutPG.x(v),gridLayoutPG.y(v)));
					} else
						firstTime = false;
					ipl.conc(gridLayoutPG.bends(*itE));
				}
			}
		}

		boundingBox[i] = m_planarLayouter.get().gridBoundingBox();
		boundingBox[i].m_x += 1; // one row/column space between components
		boundingBox[i].m_y += 1;
	}

	Array<IPoint> offset(numCC);
	m_packer.get().call(boundingBox,offset,m_pageRatio);

	bb.m_x = bb.m_y = 0;
	for(i = 0; i < numCC; ++i)
	{
		const List<node> &nodes = PG.nodesInCC(i);

		const int dx = offset[i].m_x;
		const int dy = offset[i].m_y;

		if(boundingBox[i].m_x + dx > bb.m_x)
			bb.m_x = boundingBox[i].m_x + dx;
		if(boundingBox[i].m_y + dy > bb.m_y)
			bb.m_y = boundingBox[i].m_y + dy;

		// iterate over all nodes in i-th cc
		ListConstIterator<node> it;
		for(it = nodes.begin(); it.valid(); ++it)
		{
			node vG = *it;

			gridLayout.x(vG) += dx;
			gridLayout.y(vG) += dy;

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				ListIterator<IPoint> it;
				for(it = gridLayout.bends(eG).begin(); it.valid(); ++it) {
					(*it).m_x += dx;
					(*it).m_y += dy;
				}
			}
		}
	}

	bb.m_x -= 1; // remove margin of topmost/rightmost box
	bb.m_y -= 1;
}


} // end namespace ogdf

