/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Merges nodes with neighbour to get a Multilevel Graph
 *
 * \author Gereon Bartel
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

#include <ogdf/energybased/multilevelmixer/MatchingMerger.h>

namespace ogdf {

MatchingMerger::MatchingMerger()
:m_selectByMass(false)
{
}

bool MatchingMerger::buildOneLevel(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	int level = MLG.getLevel() + 1;

	int numNodes = G.numberOfNodes();

	if (level == 1 && m_selectByMass) {
		m_mass.init(G, 1);
	}

	if (numNodes <= 3) {
		return false;
	}

	NodeArray<bool> nodeMarks(G, false);
	std::vector<edge> matching;
	std::vector<node> candidates;

	node v;
	forall_nodes(v, G) {
		candidates.push_back(v);
	}

	while (!candidates.empty())
	{
		int rndIndex = randomNumber(0, (int)candidates.size()-1);
		node one = candidates[rndIndex];
		candidates[rndIndex] = candidates.back();
		candidates.pop_back();

		if (nodeMarks[one]) {
			continue;
		}
		nodeMarks[one] = true;

		std::vector<node> candNeighbors;
		std::vector<edge> candEdges;
		adjEntry adj;
		unsigned int minMass = UINT_MAX;
		forall_adj(adj, one) {
			node cand = adj->twinNode();
			if (!nodeMarks[cand] && (!m_selectByMass || m_mass[cand] <= minMass))
			{
				if (m_selectByMass && m_mass[cand] < minMass) {
					minMass = m_mass[cand];
					candNeighbors.clear();
					candEdges.clear();
				}
				candNeighbors.push_back(cand);
				candEdges.push_back(adj->theEdge());
			}
		}
		if (candNeighbors.empty()) {
			continue;
		}
		int index = randomNumber(0, int(candNeighbors.size())-1);
		nodeMarks[candNeighbors[index]] = true;
		matching.push_back(candEdges[index]);
	}

	while (!matching.empty()) {
		edge matchingEdge = matching.back();
		matching.pop_back();

		node mergeNode;
		node parent;

		// choose high degree node as parent!
		mergeNode = matchingEdge->source();
		parent = matchingEdge->target();
		if (mergeNode->degree() > parent->degree()) {
			mergeNode = matchingEdge->target();
			parent = matchingEdge->source();
		}

		NodeMerge * NM = new NodeMerge(level);
		bool ret = MLG.changeNode(NM, parent, MLG.radius(parent), mergeNode);
		OGDF_ASSERT( ret );
		if (m_selectByMass) {
			m_mass[parent] = m_mass[parent] + m_mass[mergeNode];
		}
		MLG.moveEdgesToParent(NM, mergeNode, parent, true, m_adjustEdgeLengths);
		ret = MLG.postMerge(NM, mergeNode);
		if( !ret ) {
			delete NM;
		}
	}

	return true;
}


void MatchingMerger::selectByNodeMass( bool on )
{
	m_selectByMass = on;
}

} // namespace ogdf
