/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class MMSubgraphPlanarizer.
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

#include <ogdf/planarity/MMSubgraphPlanarizer.h>
#include <ogdf/planarity/FastPlanarSubgraph.h>
#include <ogdf/planarity/MMFixedEmbeddingInserter.h>


namespace ogdf {


MMSubgraphPlanarizer::MMSubgraphPlanarizer()
{
	FastPlanarSubgraph *s = new FastPlanarSubgraph();
	s->runs(100);
	m_subgraph.set(s);

	MMFixedEmbeddingInserter *pInserter = new MMFixedEmbeddingInserter();
	pInserter->removeReinsert(MMEdgeInsertionModule::rrAll);
	m_inserter.set(pInserter);

	m_permutations = 1;
}


Module::ReturnType MMSubgraphPlanarizer::doCall(PlanRepExpansion &PG,
	int cc,
	const EdgeArray<bool> *forbid,
	int& crossingNumber,
	int& numNS,
	int& numSN)
{
	OGDF_ASSERT(m_permutations >= 1);

	List<edge> deletedEdges;
	PG.initCC(cc);

	ReturnType retValue ;

	if(forbid != 0) {
		List<edge> preferedEdges;
		edge e;
		forall_edges(e, PG) {
			edge eOrig = PG.originalEdge(e);
			if(eOrig && (*forbid)[eOrig])
				preferedEdges.pushBack(e);
		}

		retValue = m_subgraph.get().call(PG, preferedEdges, deletedEdges, true);

	} else {
		retValue = m_subgraph.get().call(PG, deletedEdges);
	}

	if(isSolution(retValue) == false)
		return retValue;

	for(ListIterator<edge> it = deletedEdges.begin(); it.valid(); ++it)
		*it = PG.originalEdge(*it);

	int bestcr = -1;

	for(int i = 1; i <= m_permutations; ++i)
	{
		for(ListConstIterator<edge> it = deletedEdges.begin(); it.valid(); ++it)
			PG.delCopy(PG.copy(*it));

		deletedEdges.permute();

		if(forbid != 0)
			m_inserter.get().call(PG, deletedEdges, *forbid);
		else
			m_inserter.get().call(PG, deletedEdges);

		crossingNumber = PG.computeNumberOfCrossings();

		if(i == 1 || crossingNumber < bestcr) {
			bestcr = crossingNumber;
			numNS = PG.numberOfNodeSplits();
			numSN = PG.numberOfSplittedNodes();
		}

		PG.initCC(cc);
	}

	crossingNumber = bestcr;

	return retFeasible;
}


} // namspace ogdf
