/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the class DinoUmlDiagramGraph
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


#include <ogdf/fileformats/DinoUmlDiagramGraph.h>


namespace ogdf {

	//
	// C o n s t r u c t o r
	//
	DinoUmlDiagramGraph::DinoUmlDiagramGraph(const DinoUmlModelGraph &umlModelGraph,
											 UmlDiagramType diagramType,
											 String diagramName):
		m_modelGraph(umlModelGraph),
		m_diagramName(diagramName),
		m_diagramType(diagramType)
	{
	}

	//
	// D e s t r u c t o r
	//
	DinoUmlDiagramGraph::~DinoUmlDiagramGraph()
	{
		// Remove elements from lists
		m_containedNodes.clear();
		m_containedEdges.clear();
		m_x.clear();
		m_y.clear();
		m_w.clear();
		m_h.clear();
	}

	//
	// a d d N o d e W i t h G e o m e t r y
	//
	void DinoUmlDiagramGraph::addNodeWithGeometry(
		NodeElement* node,
		double x, double y,
		double w, double h)
	{
		// Append node to the end of the list
		m_containedNodes.pushBack(node);

		// Dito with coordinates
		m_x.pushBack(x);
		m_y.pushBack(y);
		m_w.pushBack(w);
		m_h.pushBack(h);

	}

	//
	// a d d E d g e
	//
	void DinoUmlDiagramGraph::addEdge(EdgeElement* edge)
	{
		// Append edge to the end of the list
		m_containedEdges.pushBack(edge);
	}

	//
	// g e t D i a g r a m T y p e S t r i n g
	//
	String DinoUmlDiagramGraph::getDiagramTypeString() const
	{
		switch(m_diagramType){

		case (classDiagram):
			return String("Class diagram");
			break;
		case (moduleDiagram):
			return String("Module diagram");
			break;
		case (sequenceDiagram):
			return String("Sequence diagram");
			break;
		case (collaborationDiagram):
			return String("Collaboration diagram");
			break;
		case (componentDiagram):
			return String("Component diagram");
			break;
		case (unknownDiagram):
			return String("Unknown type diagram");
			break;
		default:
			return String("");
		}

	} // getDiagramTypeString



	//
	// o u t p u t O p e r a t o r  for DinoUmlDiagramGraph
	//
	ostream &operator<<(ostream &os, const DinoUmlDiagramGraph &diagramGraph)
	{
		// Header with diagram name and type
		os << "\n--- " << diagramGraph.getDiagramTypeString()
			<< " \"" << diagramGraph.m_diagramName << "\" ---\n" << endl;

		// Nodes

		// Initialize iterators
		SListConstIterator<NodeElement*> nodeIt = diagramGraph.m_containedNodes.begin();
		SListConstIterator<double> xIt = diagramGraph.m_x.begin();
		SListConstIterator<double> yIt = diagramGraph.m_y.begin();
		SListConstIterator<double> wIt = diagramGraph.m_w.begin();
		SListConstIterator<double> hIt = diagramGraph.m_h.begin();

		// Traverse lists
		while (nodeIt.valid()){

			os << "Node " << diagramGraph.m_modelGraph.getNodeLabel(*nodeIt)
				<< " with geometry ("
				<< *xIt << ", "
				<< *yIt << ", "
				<< *wIt << ", "
				<< *hIt << ")." << endl;

			++nodeIt;
			++xIt;
			++yIt;
			++wIt;
			++hIt;

		} // while

		// Edges

		// Traverse lists
		SListConstIterator<EdgeElement*> edgeIt = diagramGraph.m_containedEdges.begin();
		for (edgeIt = diagramGraph.m_containedEdges.begin();
			 edgeIt.valid();
			 ++edgeIt)
		{
			os << "Edge between "
				<< diagramGraph.m_modelGraph.getNodeLabel((*edgeIt)->source())
				<< " and "
				<< diagramGraph.m_modelGraph.getNodeLabel((*edgeIt)->target())
				<< endl;
		}

		return os;

	} // <<


} // namespace ogdf
