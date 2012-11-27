/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
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

#include <ogdf/energybased/multilevelmixer/RandomMerger.h>

namespace ogdf {

RandomMerger::RandomMerger()
:m_levelSizeFactor(2.0)
{
}

bool RandomMerger::buildOneLevel(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	int level = MLG.getLevel() + 1;
	int numNodes = G.numberOfNodes();

	if (numNodes <= 3) {
		return false;
	}

	node v;
	int index = 0;
	Array<node> candidates(numNodes);
	forall_nodes(v, G) {
		candidates[index] = v;
		index++;
	}

	int candSize = candidates.size();
	while (candSize > numNodes / m_levelSizeFactor)
	{
		index = randomNumber(0, candSize-1);
		node mergeNode = candidates[index];
		candidates[index] = candidates[candSize-1];
		candSize--;
		node parent;

		if (mergeNode->degree() > 0) {
			int index = randomNumber(0, mergeNode->degree()-1);
			int i = 0;
			adjEntry adj;
			forall_adj(adj, mergeNode) {
				if (i == index) {
					parent = adj->twinNode();
					break;
				} else {
					i++;
				}
			}
		} else {
			do {
				index = randomNumber(0, candSize-1);
				parent = candidates[index];
			} while (parent == mergeNode);
			candidates[index] = candidates[candSize-1];
			candSize--;
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


void RandomMerger::setFactor(double factor)
{
	m_levelSizeFactor = factor;
}

} // namespace ogdf
