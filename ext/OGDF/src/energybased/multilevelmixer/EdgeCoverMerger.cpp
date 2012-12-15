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

#include <ogdf/energybased/multilevelmixer/EdgeCoverMerger.h>

namespace ogdf {

EdgeCoverMerger::EdgeCoverMerger()
:m_levelSizeFactor(2.0)
{
}

bool EdgeCoverMerger::buildOneLevel(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	int level = MLG.getLevel() + 1;
	m_substituteNodes.init(G, 0);

	int numNodes = G.numberOfNodes();

	if (numNodes <= 3) {
		return false;
	}

	NodeArray<bool> nodeMarks(G, false);
	std::vector<edge> untouchedEdges;
	std::vector<edge> matching;
	std::vector<edge> edgeCover;
	std::vector<edge> rest;
	edge e;
	forall_edges(e, G) {
		untouchedEdges.push_back(e);
	}

	while (!untouchedEdges.empty())
	{
		int rndIndex = randomNumber(0, (int)untouchedEdges.size()-1);
		edge randomEdge = untouchedEdges[rndIndex];
		untouchedEdges[rndIndex] = untouchedEdges.back();
		untouchedEdges.pop_back();

		node one = randomEdge->source();
		node two = randomEdge->target();
		if (!nodeMarks[one] && !nodeMarks[two]) {
			matching.push_back(randomEdge);
			nodeMarks[one] = true;
			nodeMarks[two] = true;
		} else {
			rest.push_back(randomEdge);
		}
	}

	while (!rest.empty())
	{
		int rndIndex = randomNumber(0, (int)rest.size()-1);
		edge randomEdge = rest[rndIndex];
		rest[rndIndex] = rest.back();
		rest.pop_back();

		node one = randomEdge->source();
		node two = randomEdge->target();
		if (!nodeMarks[one] || !nodeMarks[two]) {
			edgeCover.push_back(randomEdge);
			nodeMarks[one] = true;
			nodeMarks[two] = true;
		}
	}

	bool retVal = false;

	while ((!matching.empty() || !edgeCover.empty()) && G.numberOfNodes() > numNodes / m_levelSizeFactor) {
		int rndIndex;
		edge coveringEdge;

		if (!matching.empty()) {
			rndIndex = randomNumber(0, (int)matching.size()-1);
			coveringEdge = matching[rndIndex];
			matching[rndIndex] = matching.back();
			matching.pop_back();
		} else {
			rndIndex = randomNumber(0, (int)edgeCover.size()-1);
			coveringEdge = edgeCover[rndIndex];
			edgeCover[rndIndex] = edgeCover.back();
			edgeCover.pop_back();
		}

		node mergeNode;
		node parent;

		// choose high degree node as parent!
		mergeNode = coveringEdge->source();
		parent = coveringEdge->target();
		if (mergeNode->degree() > parent->degree()) {
			mergeNode = coveringEdge->target();
			parent = coveringEdge->source();
		}

		while(m_substituteNodes[parent] != 0) {
			parent = m_substituteNodes[parent];
		}
		while(m_substituteNodes[mergeNode] != 0) {
			mergeNode = m_substituteNodes[mergeNode];
		}

		if (MLG.getNode(parent->index()) != parent
			|| MLG.getNode(mergeNode->index()) != mergeNode
			|| parent == mergeNode)
		{
			continue;
		}

		retVal = doMerge(MLG, parent, mergeNode, level);
	}

	return retVal;
}


void EdgeCoverMerger::setFactor(double factor)
{
	m_levelSizeFactor = factor;
}


// tracks substitute Nodes
bool EdgeCoverMerger::doMerge( MultilevelGraph &MLG, node parent, node mergePartner, int level )
{
	NodeMerge * NM = new NodeMerge(level);
	bool ret = MLG.changeNode(NM, parent, MLG.radius(parent), mergePartner);
	OGDF_ASSERT( ret );
	MLG.moveEdgesToParent(NM, mergePartner, parent, true, m_adjustEdgeLengths);
	ret = MLG.postMerge(NM, mergePartner);
	if( !ret ) {
		delete NM;
		return false;
	}
	m_substituteNodes[mergePartner] = parent;
	return true;
}

} // namespace ogdf
