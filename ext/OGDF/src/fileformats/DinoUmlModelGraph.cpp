/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the class DinoUmlModelGraph
 *
 * \author Dino Ahr
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


#include <ogdf/fileformats/DinoUmlModelGraph.h>


namespace ogdf {

	//
	// C o n s t r u c t o r
	//
	DinoUmlModelGraph::DinoUmlModelGraph(){

		// Initialize arrays
		m_nodeLabel.init(*this);
		m_eType.init(*this,Graph::association);
		m_vType.init(*this,Graph::vertex);

	}

	//
	// D e s t r u c t o r
	//
	DinoUmlModelGraph::~DinoUmlModelGraph(){

		// ??? Destroy arrays
	}

	//
	// o u t p u t O p e r a t o r  for DinoUmlModelGraph
	//
	ostream &operator<<(ostream &os, const DinoUmlModelGraph &modelGraph)
	{
		// Header
		os << "\n--- UmlModelGraph ---\n" << endl;

		// Traverse graph

		// Nodes
		NodeElement *v;
		os << "Classes/Interfaces:\n" << endl;
		forall_nodes(v,modelGraph) {
			os << "\t" << modelGraph.getNodeLabel(v) << endl;
		}

		// Edges
		EdgeElement *e;
		os << "\nRelations:\n" << endl;
		forall_edges(e,modelGraph) {
			os << "\t";

			if (modelGraph.type(e) == Graph::association){
				os << "Association between ";
			}
			if (modelGraph.type(e) == Graph::generalization){
				os << "Generalization between ";
			}
			if (modelGraph.type(e) == Graph::dependency){
				os << "Dependency between ";
			}

			os << modelGraph.getNodeLabel(e->source()) << " and "
				<< modelGraph.getNodeLabel(e->target()) << endl;
		}

		return os;

	} // <<


} // namespace ogdf
