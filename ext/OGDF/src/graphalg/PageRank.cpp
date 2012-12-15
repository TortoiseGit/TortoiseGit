/*
 * $Revision: 2597 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-15 19:26:11 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of basic page rank.
 *
 * \author Martin Gronemann
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


#include <ogdf/graphalg/PageRank.h>

namespace ogdf {


void BasicPageRank::call(
	const Graph& graph,
	const EdgeArray<double>& edgeWeight,
	NodeArray<double>& pageRankResult)
{
	const double initialPageRank = 1.0 / (double)graph.numberOfNodes();
	const double maxPageRankDeltaBound = initialPageRank * m_threshold;

	// the two ping pong buffer
	NodeArray<double> pageRankPing(graph, 0.0);
	NodeArray<double> pageRankPong(graph, 0.0);

	NodeArray<double>* pCurrPageRank = &pageRankPing;
	NodeArray<double>* pNextPageRank = &pageRankPong;

	NodeArray<double> nodeNorm(graph);

	for (node v = graph.firstNode(); v; v = v->succ())
	{
		double sum = 0.0;
		for (adjEntry adj = v->firstAdj(); adj; adj = adj->succ())
		{
			edge e = adj->theEdge();
			sum += edgeWeight[e];
		}
		nodeNorm[v] = 1.0 / sum;
	}

	pCurrPageRank->init(graph, initialPageRank);

	// main iteration loop
	int numIterations = 0;
	bool converged = false;
	// check conditions
	while ( !converged && (numIterations < m_maxNumIterations) )
	{
		// init the result of this iteration
		pNextPageRank->init(graph, (1.0 - m_dampingFactor) / (double)graph.numberOfNodes());
		// calculate the transfer between each node
		for (edge e = graph.firstEdge(); e; e = e->succ())
		{
			node v = e->source();
			node w = e->target();

			double vwTransfer = (edgeWeight[e] * nodeNorm[v] * (*pCurrPageRank)[v]);
			double wvTransfer = (edgeWeight[e] * nodeNorm[w] * (*pCurrPageRank)[w]);
			(*pNextPageRank)[w] += vwTransfer;
			(*pNextPageRank)[v] += wvTransfer;
		}

		// damping and calculating change
		double maxPageRankDelta = 0.0;
		for (node v = graph.firstNode(); v; v = v->succ())
		{
			(*pNextPageRank)[v] *= m_dampingFactor;
			double pageRankDelta = fabs((*pNextPageRank)[v] - (*pCurrPageRank)[v]);
			maxPageRankDelta = std::max(maxPageRankDelta, pageRankDelta);
		}

		// swap ping and pong, pong ping, ping pong, lalalala
		std::swap(pNextPageRank, pCurrPageRank);
		numIterations++;

		// check if the change is small enough
		converged = (maxPageRankDelta < maxPageRankDeltaBound);
	}

	// normalization
	double maxPageRank = (*pCurrPageRank)[graph.firstNode()];
	double minPageRank = (*pCurrPageRank)[graph.firstNode()];
	for (node v = graph.firstNode(); v; v = v->succ())
	{
		maxPageRank = std::max(maxPageRank, (*pCurrPageRank)[v]);
		minPageRank = std::min(minPageRank, (*pCurrPageRank)[v]);
	}

	// init result
	pageRankResult.init(graph);
	for (node v = graph.firstNode(); v; v = v->succ())
	{
		double r = ((*pCurrPageRank)[v] - minPageRank) / (maxPageRank - minPageRank);
		pageRankResult[v] = r;
	}
	// result is now between 0 and 1
}

} // end of namespace ogdf
