/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Preprocessor Layout simplifies Graphs for use in other Algorithms
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

#include <ogdf/basic/PreprocessorLayout.h>

namespace ogdf {

PreprocessorLayout::PreprocessorLayout()
: m_randomize(false)
{

}


void PreprocessorLayout::call(GraphAttributes &GA)
{
	if (m_secondaryLayout.valid()) {
		MultilevelGraph MLG(GA);
		call(MLG);
		MLG.exportAttributes(GA);
	}
}


void PreprocessorLayout::call(MultilevelGraph &MLG)
{
	m_deletedEdges.clear();
	Graph * G = &(MLG.getGraph());

	node v;

	double sqrsize;
	if (m_randomize) sqrsize = 2.0*sqrt((double)G->numberOfNodes())*MLG.averageRadius();

	forall_nodes(v, *G) {
		if (MLG.radius(v) <= 0) {
			MLG.radius(v, 1.0);
		}
		if (m_randomize) {
			MLG.x(v, randomDouble( -sqrsize, sqrsize ));//-5.0, 5.0));
			MLG.y(v, randomDouble( -sqrsize, sqrsize ));
		}
	}
	if (m_secondaryLayout.valid()) {

		call(*G, MLG);

		m_secondaryLayout.get().call(MLG.getGraphAttributes());
		MLG.updateReverseIndizes();

		for(std::vector<EdgeData>::iterator i = m_deletedEdges.begin(); i != m_deletedEdges.end(); i++ ) {
			int index = (*i).edgeIndex;
			edge temp = G->newEdge(MLG.getNode((*i).sourceIndex), MLG.getNode((*i).targetIndex), index);
			MLG.weight(temp, (float)(*i).weight);
		}
	}
}


void PreprocessorLayout::call(Graph &G, MultilevelGraph &MLG)
{
	std::vector<edge> deletedEdges;

	edge e;
	forall_edges(e, G) {
		int index = e->index();
		if (e->source() == e->target()) {
			deletedEdges.push_back(e);
			m_deletedEdges.push_back(EdgeData(index, e->source()->index(), e->target()->index(), MLG.weight(e)));
		} else {
			adjEntry adj;
			forall_adj(adj, e->source()) {
				if (adj->theEdge()->index() < index && adj->twinNode() == e->target()) {
					deletedEdges.push_back(e);
					m_deletedEdges.push_back(EdgeData(index, e->source()->index(), e->target()->index(), MLG.weight(e)));
					break;
				}
			}
		}
	}

	for (std::vector<edge>::iterator i = deletedEdges.begin(); i != deletedEdges.end(); i++) {
		G.delEdge(*i);
	}
}


} // namespace ogdf
