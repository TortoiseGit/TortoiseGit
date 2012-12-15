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

#include <ogdf/energybased/multilevelmixer/IndependentSetMerger.h>

namespace ogdf {

IndependentSetMerger::IndependentSetMerger()
:m_base(2.f)
{
}


void IndependentSetMerger::buildAllLevels(MultilevelGraph &MLG)
{
	m_numLevels = 1;
	MLG.updateReverseIndizes();

	std::vector< std::vector<node> > levelNodes;
	Graph &G = MLG.getGraph();

	// calc MIS
	NodeArray<bool> nodeMarks(G, false);
	std::vector<node> IScandidates;
	node v;
	forall_nodes(v, G) {
		IScandidates.push_back(v);
	}
	levelNodes.push_back(std::vector<node>());
	while(!IScandidates.empty()) {
		// select random node
		int index = randomNumber(0, (int)IScandidates.size()-1);
		node ISnode = IScandidates[index];
		IScandidates[index] = IScandidates.back();
		IScandidates.pop_back();

		if(!nodeMarks[ISnode]) {
			adjEntry adj;
			forall_adj(adj, ISnode) {
				nodeMarks[adj->twinNode()] = true;
			}
			levelNodes[0].push_back(ISnode);
		}
	}

	bool end = false;
	unsigned int i = 0;
	do {
		std::vector<node> lvl = prebuildLevel(G, levelNodes[i], i);
		end = lvl.size() <= 2;
		if(!end) {
			levelNodes.push_back(std::vector<node>(lvl));
			i++;
		}
	} while (!end);

	for (i = 0; i < levelNodes.size(); i++) {
		if (levelNodes[i].empty()) {
			continue;
		}
		buildOneLevel(MLG, levelNodes[i]);
		m_numLevels++;
	}

	MLG.updateReverseIndizes();
}


std::vector<node> IndependentSetMerger::prebuildLevel(const Graph &G, const std::vector<node> &oldLevel, int level)
{
	std::vector<node> levelNodes;
	std::vector<node> oldLevelNodes;
	std::map<node, int> marks;
	for (std::vector<node>::const_iterator i = oldLevel.begin(); i != oldLevel.end(); i++) {
		marks[*i] = 1;
		oldLevelNodes.push_back(*i);
	}

	while (!oldLevelNodes.empty()) {
		int index = randomNumber(0, (int)oldLevelNodes.size()-1);
		node oldNode = oldLevelNodes[index];
		oldLevelNodes[index] = oldLevelNodes.back();
		oldLevelNodes.pop_back();

		if (marks[oldNode] == 1) {
			NodeArray<bool> seen(G, false);
			std::vector<node> stacks[2];
			int one = 1;
			int two = 0;
			stacks[one].push_back(oldNode);
			levelNodes.push_back(oldNode);
			// BFS bis m_base^level
			unsigned int depth = 0;
			while(!stacks[one].empty()) {
				node bfsNode = stacks[one].back();
				stacks[one].pop_back();

				if (!seen[bfsNode]) {
					if (marks[bfsNode] == 1) {
						marks[bfsNode] = 2;
					}
					seen[bfsNode] = true;
					adjEntry adj;
					forall_adj(adj, bfsNode) {
						stacks[two].push_back(adj->twinNode());
					}
				}
				if (stacks[one].empty()) {
					depth++;
					int temp = one;
					one = two;
					two = temp;
					if (depth > pow(m_base, level)) {
						break;
					}
				}
			}
		}
	}

	return levelNodes;
}


bool IndependentSetMerger::buildOneLevel(MultilevelGraph &MLG, std::vector<node> &levelNodes)
{
	Graph &G = MLG.getGraph();
	int level = MLG.getLevel() + 1;

	int numNodes = G.numberOfNodes();

	if (numNodes <= 3) {
		return false;
	}

	std::map<node, node> parents;
	node v;
	forall_nodes(v, G) {
		parents[v] = 0;
	}

	std::vector<node> mergeOrder;
	NodeArray<bool> seen(G, false);
	std::vector<node> stacks[2];
	int one = 1;
	int two = 0;
	for(std::vector<node>::iterator i = levelNodes.begin(); i != levelNodes.end(); i++) {
		stacks[one].push_back(*i);
		parents[*i] = *i;
	}
	// parallel BFS auf allen levelNodes
	while (!stacks[one].empty()) {
		node bfsNode = stacks[one].back();
		stacks[one].pop_back();

		if (!seen[bfsNode]) {
			seen[bfsNode] = true;
			adjEntry adj;
			forall_adj(adj, bfsNode) {
				node twin = adj->twinNode();
				stacks[two].push_back(twin);
				if(parents[twin] == 0) {
					parents[twin] = bfsNode;
					mergeOrder.push_back(twin);
				}
			}
		}
		if (stacks[one].empty()) {
			int temp = one;
			one = two;
			two = temp;
		}
	}

	for (std::vector<node>::iterator i = mergeOrder.begin(); i != mergeOrder.end(); i++) {
		node mergeNode = *i;
		node parent = mergeNode;
		while(parents[parent] != parent) {
			parent = parents[parent];
		}

		NodeMerge * NM = new NodeMerge(level);
		bool ret = MLG.changeNode(NM, parent, MLG.radius(parent), mergeNode);
		OGDF_ASSERT( ret );
		MLG.moveEdgesToParent(NM, mergeNode, parent, true, m_adjustEdgeLengths);
		ret = MLG.postMerge(NM, mergeNode);
		if( !ret ) {
			delete NM;
		}
	}

	return true;
}

void IndependentSetMerger::setSearchDepthBase( float base )
{
	m_base = base;
}

} // namespace ogdf
