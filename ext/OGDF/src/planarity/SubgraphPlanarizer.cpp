/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class SubgraphPlanarizer.
 *
 * \author Markus Chimani
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

#include <ogdf/planarity/SubgraphPlanarizer.h>
#include <ogdf/planarity/FastPlanarSubgraph.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>

namespace ogdf
{

SubgraphPlanarizer::SubgraphPlanarizer()
{
	FastPlanarSubgraph* s= new FastPlanarSubgraph();
	s->runs(100);
	m_subgraph.set(s);

	VariableEmbeddingInserter* i = new VariableEmbeddingInserter();
	i->removeReinsert(VariableEmbeddingInserter::rrAll);
	m_inserter.set(i);

	m_permutations = 1;
	m_setTimeout = true;
}

Module::ReturnType SubgraphPlanarizer::doCall(PlanRep &PG,
	int cc,
	const EdgeArray<int>  &cost,
	const EdgeArray<bool> &forbid,
	const EdgeArray<unsigned int>  &subgraphs,
	int& crossingNumber)
{
	OGDF_ASSERT(m_permutations >= 1);

	OGDF_ASSERT(!(useSubgraphs()) || useCost()); // ersetze durch exception handling

	double startTime;
	usedTime(startTime);

	if(m_setTimeout)
		m_subgraph.get().timeLimit(m_timeLimit);

	List<edge> deletedEdges;
	PG.initCC(cc);
	EdgeArray<int> costPG(PG);
	edge e;
	forall_edges(e,PG)
		costPG[e] = cost[PG.original(e)];
	ReturnType retValue = m_subgraph.get().call(PG, costPG, deletedEdges);
	if(isSolution(retValue) == false)
		return retValue;

	for(ListIterator<edge> it = deletedEdges.begin(); it.valid(); ++it)
		*it = PG.original(*it);

	bool foundSolution = false;
	CrossingStructure cs;
	for(int i = 1; i <= m_permutations; ++i)
	{
		const int nG = PG.numberOfNodes();

		for(ListConstIterator<edge> it = deletedEdges.begin(); it.valid(); ++it)
			PG.delCopy(PG.copy(*it));

		deletedEdges.permute();

		if(m_setTimeout)
			m_inserter.get().timeLimit(
				(m_timeLimit >= 0) ? max(0.0,m_timeLimit - usedTime(startTime)) : -1);

		ReturnType ret;
		if(useForbid()) {
			if(useCost()) {
				if(useSubgraphs())
					ret = m_inserter.get().call(PG, cost, forbid, deletedEdges, subgraphs);
				else
					ret = m_inserter.get().call(PG, cost, forbid, deletedEdges);
			} else
				ret = m_inserter.get().call(PG, forbid, deletedEdges);
		} else {
			if(useCost()) {
				if(useSubgraphs())
					ret = m_inserter.get().call(PG, cost, deletedEdges, subgraphs);
				else
					ret = m_inserter.get().call(PG, cost, deletedEdges);
			} else
				ret = m_inserter.get().call(PG, deletedEdges);
		}

		if(isSolution(ret) == false)
			continue; // no solution found, that's bad...

		if(!useCost())
			crossingNumber = PG.numberOfNodes() - nG;
		else {
			crossingNumber = 0;
			node n;
			forall_nodes(n, PG) {
				if(PG.original(n) == 0) { // dummy found -> calc cost
					edge e1 = PG.original(n->firstAdj()->theEdge());
					edge e2 = PG.original(n->lastAdj()->theEdge());
					if(useSubgraphs()) {
						int subgraphCounter = 0;
						for(int i=0; i<32; i++) {
							if(((subgraphs[e1] & (1<<i))!=0) && ((subgraphs[e2] & (1<<i)) != 0))
								subgraphCounter++;
						}
						crossingNumber += (subgraphCounter*cost[e1] * cost[e2]);
					} else
						crossingNumber += cost[e1] * cost[e2];
				}
			}
		}

		if(i == 1 || crossingNumber < cs.weightedCrossingNumber()) {
			foundSolution = true;
			cs.init(PG, crossingNumber);
		}

		if(localLogMode() == LM_STATISTIC) {
			if(m_permutations <= 200 ||
				i <= 10 || (i <= 30 && (i % 2) == 0) || (i > 30 && i <= 100 && (i % 5) == 0) || ((i % 10) == 0))
				sout() << "\t" << cs.weightedCrossingNumber();
		}

		PG.initCC(cc);

		if(m_timeLimit >= 0 && usedTime(startTime) >= m_timeLimit) {
			if(foundSolution == false)
				return retTimeoutInfeasible; // not able to find a solution...
			break;
		}
	}

	cs.restore(PG,cc); // restore best solution in PG
	crossingNumber = cs.weightedCrossingNumber();

	OGDF_ASSERT(isPlanar(PG) == true);

	return retFeasible;
}


void SubgraphPlanarizer::CrossingStructure::init(PlanRep &PG, int weightedCrossingNumber)
{
	m_weightedCrossingNumber = weightedCrossingNumber;
	m_crossings.init(PG.original());

	m_numCrossings = 0;
	NodeArray<int> index(PG,-1);
	node v;
	forall_nodes(v,PG)
		if(PG.isDummy(v))
			index[v] = m_numCrossings++;

	edge ePG;
	forall_edges(ePG,PG)
	{
		if(PG.original(ePG->source()) != 0) {
			edge e = PG.original(ePG);
			ListConstIterator<edge> it = PG.chain(e).begin();
			for(++it; it.valid(); ++it) {
				//cout << index[(*it)->source()] << " ";
				m_crossings[e].pushBack(index[(*it)->source()]);
			}
		}
	}
}

void SubgraphPlanarizer::CrossingStructure::restore(PlanRep &PG, int cc)
{
	//PG.initCC(cc);

	Array<node> id2Node(0,m_numCrossings-1,0);

	SListPure<edge> edges;
	PG.allEdges(edges);

	for(SListConstIterator<edge> itE = edges.begin(); itE.valid(); ++itE)
	{
		edge ePG = *itE;
		edge e = PG.original(ePG);

		SListConstIterator<int> it;
		for(it = m_crossings[e].begin(); it.valid(); ++it)
		{
			node x = id2Node[*it];
			edge ePGOld = ePG;
			ePG = PG.split(ePG);
			node y = ePG->source();

			if(x == 0) {
				id2Node[*it] = y;
			} else {
				PG.moveTarget(ePGOld, x);
				PG.moveSource(ePG, x);
				PG.delNode(y);
			}
		}
	}
}

} // namspace ogdf
