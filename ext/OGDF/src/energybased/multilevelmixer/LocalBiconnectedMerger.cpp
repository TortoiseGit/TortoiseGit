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

#include <ogdf/energybased/multilevelmixer/LocalBiconnectedMerger.h>
#include <ogdf/decomposition/BCTree.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/HashArray.h>

namespace ogdf {

LocalBiconnectedMerger::LocalBiconnectedMerger()
:m_levelSizeFactor(2.0)
{
}


bool LocalBiconnectedMerger::canMerge( Graph &G, node parent, node mergePartner )
{
	return canMerge(G, parent, mergePartner, 1) && canMerge(G, parent, mergePartner, 0);
}


bool LocalBiconnectedMerger::canMerge( Graph &G, node parent, node mergePartner, int testStrength )
{
	if ( parent->degree() <= 2 || mergePartner->degree() <= 2 || m_isCut[parent] || m_isCut[mergePartner] ) {
		return true;
	}

	unsigned int nodeLimit = (int)log((double)G.numberOfNodes()) * 2 + 50;
	unsigned int visitedNodes = 0;
	m_realNodeMarks.clear();
	HashArray<node, int> nodeMark(-1);

	HashArray<node, bool> seen(false);
	seen[parent] = true;
	seen[mergePartner] = true;

	HashArray<node, int> neighborStatus(0);
	neighborStatus[parent] = -1;
	neighborStatus[mergePartner] = -1;

	List<node> bfsQueue;
	List<node> neighbors;
	int minIndex = INT_MAX;
	adjEntry adj;
	forall_adj(adj, parent) {
		node temp = adj->twinNode();
		bfsQueue.pushBack(temp);
		nodeMark[temp] = temp->index();
		if(neighborStatus[temp] == 0) {
			neighbors.pushBack(temp);
			neighborStatus[temp] = 1;
			if (temp->index() < minIndex) {
				minIndex = temp->index();
			}
		}
	}
	forall_adj(adj, mergePartner) {
		node temp = adj->twinNode();
		bfsQueue.pushBack(temp);
		nodeMark[temp] = temp->index();
		if(neighborStatus[temp] == 0) {
			neighbors.pushBack(temp);
			neighborStatus[temp] = 1;
			if (temp->index() < minIndex) {
				minIndex = temp->index();
			}
		}
	}

	List<node> nonReachedNeighbors;

	if (testStrength > 0)
	{
		minIndex = INT_MAX;
		for (List<node>::iterator i = neighbors.begin(); i != neighbors.end(); i++) {
			node temp = *i;
			forall_adj(adj, temp) {
				node neighbor = adj->twinNode();
				if (neighborStatus[neighbor] == 0 && !seen[neighbor]) {
					nonReachedNeighbors.pushBack(neighbor);
					neighborStatus[neighbor] = 2;
					bfsQueue.pushBack(neighbor);
					nodeMark[neighbor] = neighbor->index();
					if (neighbor->index() < minIndex) {
						minIndex = neighbor->index();
					}
				}
			}
		}

		for (List<node>::iterator i = neighbors.begin(); i != neighbors.end(); i++) {
			seen[*i] = true;
		}
		neighbors.clear();
	}

	nonReachedNeighbors.conc(neighbors);

	if (nonReachedNeighbors.empty()) {
		return true;
	}

	// BFS from all neighbors
	while(!bfsQueue.empty() && visitedNodes < nodeLimit) {
		node temp = bfsQueue.popFrontRet();
		if (seen[temp] || m_isCut[temp]) {
			continue;
		}
		seen[temp] = true;
		visitedNodes++;

		forall_adj(adj, temp) {
			node neighbor = adj->twinNode();
			if (neighbor == parent || neighbor == mergePartner) {
				continue;
			}
			if (nodeMark[neighbor] == -1) {
				nodeMark[neighbor] = realNodeMark(nodeMark[temp]);
			} else {
				int neighborNM = realNodeMark(nodeMark[neighbor]);
				int tempNM = realNodeMark(nodeMark[temp]);
				int minNM = min(tempNM, neighborNM);
				if (tempNM != neighborNM) {
					int maxNM = max(tempNM, neighborNM);
					nodeMark[neighbor] = minNM;
					nodeMark[temp] = minNM;
					m_realNodeMarks[maxNM] = minNM;
					if (minNM == minIndex) {
						// check nonReachedNeighbors
						for (List<node>::iterator i = nonReachedNeighbors.begin(); i != nonReachedNeighbors.end(); ) {
							if (realNodeMark(nodeMark[*i]) == minIndex) {
								List<node>::iterator j = i;
								i++;
								nonReachedNeighbors.del(j);
							} else {
								i++;
							}
						}
						if (nonReachedNeighbors.empty()) {
							return true;
						}
					}
				}
			}
			if (!seen[neighbor]) {
				bfsQueue.pushBack(neighbor);
			}
		}
	}

	// free some space
	m_realNodeMarks.clear();
	return false;
}


bool LocalBiconnectedMerger::doMergeIfPossible( Graph &G, MultilevelGraph &MLG, node parent, node mergePartner, int level )
{
	if (canMerge(G, parent, mergePartner)) {
		return doMerge(MLG, parent, mergePartner, level);
	}
	return true;
}


// tracks substitute Nodes
// updates the bi-connectivity check data structure m_isCut
// merges the nodes
bool LocalBiconnectedMerger::doMerge( MultilevelGraph &MLG, node parent, node mergePartner, int level )
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
	if (m_isCut[mergePartner]) {
		m_isCut[parent] = true;
	}
	return true;
}


void LocalBiconnectedMerger::initCuts(Graph &G)
{
	// BCTree does not work for large graphs due to recursion depth (stack overflow)
	// Uncomment below to get speedup once BCTree is fixed.

//	BCTree BCT(G);
	m_isCut.init(G, false);

/*	node v;
	forall_nodes(v, G) {
		if( BCT.typeOfGNode(v) == BCTree::GNodeType::CutVertex ) {
			m_isCut[v] = true;
		}
	}*/
}


bool LocalBiconnectedMerger::buildOneLevel(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	int level = MLG.getLevel() + 1;
//	std::cout << "Level: " << level << std::endl;

	m_substituteNodes.init(G, 0);
	initCuts(G);

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
		//KK: looks like a flaw? TODO: Check the return value concept
		retVal = doMergeIfPossible(G, MLG, parent, mergeNode, level);
	}

	if (numNodes == G.numberOfNodes()) {
		return false;
	} else {
		return retVal;
	}
}


void LocalBiconnectedMerger::setFactor(double factor)
{
	m_levelSizeFactor = factor;
}


int LocalBiconnectedMerger::realNodeMark( int index )
{
	if (!m_realNodeMarks.isDefined(index) || m_realNodeMarks[index] == index) {
		return index;
	} else {
		return realNodeMark(m_realNodeMarks[index]);
	}
}

} // namespace ogdf
