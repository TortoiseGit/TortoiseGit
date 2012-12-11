/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class MMCrossingMinimizationModule.
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

#include <ogdf/module/MMCrossingMinimizationModule.h>
#include <ogdf/basic/simple_graph_alg.h>


namespace ogdf {


Module::ReturnType MMCrossingMinimizationModule::call(
	const Graph &G,
	const List<node> &splittableNodes,
	int &cr,
	const EdgeArray<bool> *forbid)
{
	cr = 0; m_nodeSplits = 0; m_splittedNodes = 0;

	NodeArray<bool> splittable(G,false);
	ListConstIterator<node> itV;
	for(itV = splittableNodes.begin(); itV.valid(); ++itV)
		splittable[*itV] = true;

	EdgeArray<int> comp(G,-1);
	int c = biconnectedComponents(G, comp);

	Array<List<edge> > edges(c);
	edge e;
	forall_edges(e,G) {
		edges[comp[e]].pushBack(e);
	}

	NodeArray<node> map(G,0);

	for(int i = 0; i < c; ++i)
	{
		if(edges[i].size() < 9)
			continue;

		Graph B;
		List<node> nodes;
		List<node> splittableNodesB;

		EdgeArray<bool> *forbidB = 0;
		if(forbid) forbidB = new EdgeArray<bool>(B, false);

		ListConstIterator<edge> it;
		for(it = edges[i].begin(); it.valid(); ++it)
		{
			edge e = *it;
			node v = e->source(), w = e->target();

			if(map[v] == 0) {
				map[v] = B.newNode();
				nodes.pushBack(v);
				if(splittable[v])
					splittableNodesB.pushBack(map[v]);
			}
			if(map[w] == 0) {
				map[w] = B.newNode();
				nodes.pushBack(w);
				if(splittable[w])
					splittableNodesB.pushBack(map[w]);
			}

			edge eB = B.newEdge(map[v],map[w]);
			if(forbidB)
				(*forbidB)[eB] = (*forbid)[e];
		}

		PlanRepExpansion PG(B,splittableNodesB);

		int crcc, numNS = 0, numSN = 0;
		ReturnType ret = doCall(PG,0,forbidB,crcc,numNS,numSN);
		delete forbidB;
		if(isSolution(ret) == false)
			return ret;
		cr += crcc;
		m_nodeSplits    += numNS;
		m_splittedNodes += numSN;

		ListConstIterator<node> itV;
		for(itV = nodes.begin(); itV.valid(); ++itV)
			map[*itV] = 0;
	}

	return retFeasible;
}


Module::ReturnType MMCrossingMinimizationModule::call(
	const Graph &G,
	int &cr,
	const EdgeArray<bool> *forbid)
{
	cr = 0; m_nodeSplits = 0; m_splittedNodes = 0;

	EdgeArray<int> comp(G,-1);
	int c = biconnectedComponents(G, comp);

	Array<List<edge> > edges(c);
	edge e;
	forall_edges(e,G) {
		edges[comp[e]].pushBack(e);
	}

	NodeArray<node> map(G,0);

	for(int i = 0; i < c; ++i)
	{
		if(edges[i].size() < 9)
			continue;

		Graph B;
		List<node> nodes;

		ListConstIterator<edge> it;
		for(it = edges[i].begin(); it.valid(); ++it)
		{
			edge e = *it;
			node v = e->source(), w = e->target();

			if(map[v] == 0) {
				map[v] = B.newNode();
				nodes.pushBack(v);
			}
			if(map[w] == 0) {
				map[w] = B.newNode();
				nodes.pushBack(w);
			}

			B.newEdge(map[v],map[w]);
		}

		PlanRepExpansion PG(B);

		int crcc, numNS = 0, numSN = 0;
		ReturnType ret = doCall(PG,0,forbid,crcc,numNS,numSN);
		if(isSolution(ret) == false)
			return ret;
		cr += crcc;
		m_nodeSplits    += numNS;
		m_splittedNodes += numSN;

		ListConstIterator<node> itV;
		for(itV = nodes.begin(); itV.valid(); ++itV)
			map[*itV] = 0;
	}

	return retFeasible;
}


} // namspace ogdf

