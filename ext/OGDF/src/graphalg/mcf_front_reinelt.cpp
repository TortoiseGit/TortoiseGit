/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief front-end for min-most flow algorithm (Reinelt)
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


#include <ogdf/graphalg/MinCostFlowReinelt.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>




namespace ogdf {

bool MinCostFlowReinelt::call(
	const Graph &G,
	const EdgeArray<int> &lowerBound,
	const EdgeArray<int> &upperBound,
	const EdgeArray<int> &cost,
	const NodeArray<int> &supply,
	EdgeArray<int> &flow)
{
	NodeArray<int> dual(G);
	return call(G, lowerBound, upperBound, cost, supply, flow, dual);
}


// computes min-cost-flow
// returns true if a minimum cost flow could be found
bool MinCostFlowReinelt::call(
	const Graph &G,
	const EdgeArray<int> &lowerBound,
	const EdgeArray<int> &upperBound,
	const EdgeArray<int> &cost,
	const NodeArray<int> &supply,
	EdgeArray<int> &flow,
	NodeArray<int> &dual)
{
	OGDF_ASSERT(checkProblem(G,lowerBound,upperBound,supply) == true);

	const int n = G.numberOfNodes();
	const int m = G.numberOfEdges();

	// assign indices 0, ..., n-1 to nodes in G
	// (this is not guaranteed for v->index() )
	NodeArray<int> vIndex(G);
	// assigning supply
	Array<int> mcfSupply(n);

	node v;
	int i = 0;
	forall_nodes(v, G) {
		mcfSupply[i] = supply[v];
		vIndex[v] = ++i;
	}


	// allocation of arrays for arcs
	Array<int> mcfTail(m);
	Array<int> mcfHead(m);
	Array<int> mcfLb(m);
	Array<int> mcfUb(m);
	Array<int> mcfCost(m);
	Array<int> mcfFlow(m);
	Array<int> mcfDual(n+1); // dual[n] = dual variable of root struct

	// set input data in edge arrays
	int nSelfLoops = 0;
	i = 0;
	edge e;
	forall_edges(e, G)
	{
		// We handle self-loops in the network already in the front-end
		// (they are just set to the lower bound below when copying result)
		if(e->isSelfLoop()) {
			nSelfLoops++;
			continue;
		}

		mcfTail[i] = vIndex[e->source()];
		mcfHead[i] = vIndex[e->target()];
		mcfLb  [i] = lowerBound[e];
		mcfUb  [i] = upperBound[e];
		mcfCost[i] = cost[e];

		++i;
	}


	int retCode; // return (error or success) code
	int objVal;  // value of flow

	// call actual min-cost-flow function
	// mcf does not support single nodes
	if ( n > 1)
	{
		//mcf does not support single edges
		if (m < 2)
		{
			if ( m == 1)
			{
				e = G.firstEdge();
				flow[e] = lowerBound[e];
			}
			retCode = 0;
		}
		else retCode = mcf(n, m-nSelfLoops, mcfSupply, mcfTail, mcfHead, mcfLb, mcfUb,
			mcfCost, mcfFlow, mcfDual, &objVal);
	}
	else retCode = 0;


	// copy resulting flow for return
	i = 0;
	forall_edges(e, G)
	{
		if(e->isSelfLoop()) {
			flow[e] = lowerBound[e];
			continue;
		}

		flow[e] = mcfFlow[i];
		if (retCode == 0) {
			OGDF_ASSERT( (flow[e]>=lowerBound[e]) && (flow[e]<= upperBound[e]) )
		}
		++i;
	}

	// copy resulting dual values for return
	i = 0;
	forall_nodes(v, G) {
		dual[v] = mcfDual[i];
		++i;
	}

	// successful if retCode == 0
	return (retCode == 0);
}


} // end namespace ogdf

