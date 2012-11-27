/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Base class for simultaneous drawing.
 *
 * \author Michael Schulz and Daniel Lueckerath
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

#include<ogdf/simultaneous/SimDraw.h>

namespace ogdf {

//*************************************************************
// default constructor
SimDraw::SimDraw()
{
	m_GA.init(m_G, GraphAttributes::edgeSubGraph);
	m_compareBy = index;
	m_isDummy.init(m_G, false);

} // end constructor


//*************************************************************
// checks whether node is a proper dummy node
// proper dummy means that node is marked as dummy and
// incident edges have at least one common input graph
bool SimDraw::isProperDummy(node v) const
{
	if(!isDummy(v))
		return false;
	int sgb = m_GA.subGraphBits(v->firstAdj()->theEdge());
	edge e;
	forall_adj_edges(e, v)
		sgb &= m_GA.subGraphBits(e);

	return (sgb != 0);

} // end isProperDummy


//*************************************************************
// returns number of dummies
int SimDraw::numberOfDummyNodes() const
{
	int counter = 0;
	node v;
	forall_nodes(v, m_G)
		if(isDummy(v))
			counter++;
	return counter;

} // end numberOfDummyNodes


//*************************************************************
// returns number of phantom dummies
int SimDraw::numberOfPhantomDummyNodes() const
{
	int counter = 0;
	node v;
	forall_nodes(v, m_G)
		if(isPhantomDummy(v))
			counter++;
	return counter;

} // end numberOfProperDummyNodes


//*************************************************************
// returns number of proper dummies
int SimDraw::numberOfProperDummyNodes() const
{
	int counter = 0;
	node v;
	forall_nodes(v, m_G)
		if(isProperDummy(v))
			counter++;
	return counter;

} // end numberOfNonProperDummyNodes


//*************************************************************
// checks whether graph and graphattributes belong to each other
// checks whether all edges have nonzero edgesubgraph value
// returns true if instance is ok
bool SimDraw::consistencyCheck() const
{
	if(&m_G != &(m_GA.constGraph()))
		return false;
	edge e;
	forall_edges(e, m_G)
		if(m_GA.subGraphBits(e) == 0)
			return false;
	return true;

} // end consistencyCheck


//*************************************************************
// calculates maximum number of input graphs
//
int SimDraw::maxSubGraph() const
{
	int max = -1;
	edge e;
	forall_edges(e, m_G)
	{
		for(int i = 31; i > max; i--)
			if(m_GA.inSubGraph(e, i))
				max = i;
	}
	return max;

} // end maxSubGraph


//*************************************************************
//returns number of basic graphs
//
int SimDraw::numberOfBasicGraphs() const
{
	if(m_G.empty())
		return 0;
	return maxSubGraph()+1;

}//end numberOfBasicGraphs


//*************************************************************
// returns Graph consisting of all edges and nodes from SubGraph i
//
const Graph SimDraw::getBasicGraph(int i) const
{
	//get a copy of m_G
	GraphCopy GC(m_G);

	//delete all edges that are not in SubGraph i
	List<edge> LE;
	GC.allEdges(LE);
	forall_listiterators(edge, it, LE)
		if(!(m_GA.inSubGraph(GC.original(*it),i)))
			GC.delCopy(*it);

	//delete all Nodes where degree = 0
	List<node> LN;
	GC.allNodes(LN);
	forall_listiterators(node, it, LN)
		if((*it)->degree() == 0)
			GC.delCopy(*it);

	return GC;

}//end getBasicGraph


//*************************************************************
// returns GraphAttributes associated with basic graph i
//
void SimDraw::getBasicGraphAttributes(int i, GraphAttributes &GA, Graph &G)
{
	G = m_G;
	GA.init(G,m_GA.attributes());

	List<edge> LE;
	m_G.allEdges(LE);
	forall_listiterators(edge,it,LE)
		if(m_GA.inSubGraph(*it,i))
		{
			node v;
			forall_nodes(v,G)
			{
				if(compare(GA,v,m_GA,(*it)->source()))
				{
					if(m_GA.attributes() & GraphAttributes::nodeGraphics)
					{
						GA.x(v) = m_GA.x((*it)->source());
						GA.y(v) = m_GA.y((*it)->source());
						GA.height(v) = m_GA.height((*it)->source());
						GA.width(v) = m_GA.width((*it)->source());
					}

					if(m_GA.attributes() & GraphAttributes::nodeId)
						GA.idNode(v) = m_GA.idNode((*it)->source());

					if(m_GA.attributes() & GraphAttributes::nodeLabel)
						GA.labelNode(v) = m_GA.labelNode((*it)->source());
				}

				if(compare(GA,v,m_GA,(*it)->target()))
				{
					if(m_GA.attributes() & GraphAttributes::nodeGraphics)
					{
						GA.x(v) = m_GA.x((*it)->target());
						GA.y(v) = m_GA.y((*it)->target());
						GA.height(v) = m_GA.height((*it)->target());
						GA.width(v) = m_GA.width((*it)->target());
					}

					if(m_GA.attributes() & GraphAttributes::nodeId)
						GA.idNode(v) = m_GA.idNode((*it)->target());

					if(m_GA.attributes() & GraphAttributes::nodeLabel)
						GA.labelNode(v) = m_GA.labelNode((*it)->target());
				}
			}

			edge e;
			forall_edges(e,G)
			{
				if(compare(GA,e->source(),m_GA,(*it)->source())
					&& compare(GA,e->target(),m_GA,(*it)->target()))
				{
					if(m_GA.attributes() & GraphAttributes::edgeIntWeight)
						GA.intWeight(e) = m_GA.intWeight(*it);

					if(m_GA.attributes() & GraphAttributes::edgeLabel)
						GA.labelEdge(e) = m_GA.labelEdge(*it);

					if(m_GA.attributes() & GraphAttributes::edgeColor)
						GA.colorEdge(e) = m_GA.colorEdge(*it);

					if(m_GA.attributes() & GraphAttributes::edgeGraphics)
						GA.bends(e) = m_GA.bends(*it);
				}
			}
		}
		else
		{
			List<edge> LE2;
			G.allEdges(LE2);
			forall_listiterators(edge, it2, LE2)
			{
				if(compare(GA,(*it2)->source(),m_GA,(*it)->source())
					&& compare(GA,(*it2)->target(),m_GA,(*it)->target()))
				{
					G.delEdge(*it2);
				}
			}
		}

		//remove all Nodes with degree == 0
		//this can change the IDs of the nodes in G.
		List<node> LN;
		G.allNodes(LN);
		forall_listiterators(node, it3, LN)
			if((*it3)->degree() == 0)
				G.delNode(*it3);

}//end getBasicGraphAttributes


//*************************************************************
//adds new GraphAttributes to m_G if maxSubgraph() < 32
//
bool SimDraw::addGraphAttributes(const GraphAttributes & GA)
{
	if(maxSubGraph() >= 31)
		return false;

	//if(compareBy() == label)
	OGDF_ASSERT((compareBy() != label) || (m_GA.attributes() & GraphAttributes::edgeLabel));

	int max = numberOfBasicGraphs();
	bool foundEdge = false;
	node v;
	edge e, f;
	Graph G = GA.constGraph();

	forall_edges(e,G)
	{
		forall_edges(f,m_G)
		{
			if(compare(m_GA, f->source(), GA, e->source()) &&
				compare(m_GA, f->target(), GA, e->target()))
			{
				foundEdge = true;
				m_GA.addSubGraph(f,max);
			}
		}

		if(!foundEdge)
		{
			node s, t;
			bool srcFound = false;
			bool tgtFound = false;
			forall_nodes(v,m_G)
			{
				if(compare(m_GA, v, GA, e->source()))
				{
					s = v;
					srcFound = true;
				}

				if(compare(m_GA, v, GA, e->target()))
				{
					t = v;
					tgtFound = true;
				}
			}

			if(!srcFound)
				s = m_G.newNode(e->source()->index());

			if(!tgtFound)
				t = m_G.newNode(e->target()->index());

			edge d = m_G.newEdge(s,t);
			if(compareBy() == label)
				m_GA.labelEdge(d) = GA.labelEdge(e);

			m_GA.addSubGraph(d, max);
		}
	}
	return true;

}// end addGraphAttributes


//*************************************************************
//adds the new Graph G to the instance m_G if maxSubGraph < 32
//and CompareMode = index.
bool SimDraw::addGraph(const Graph & G)
{
	if(compareBy() == label)
		return false;
	else
	{
		GraphAttributes newGA(G);
		return(addGraphAttributes(newGA));
	}

}//end addGraph


//*************************************************************
//compares two nodes depending on the mode in m_CompareBy
//
bool SimDraw::compare(const GraphAttributes & vGA, node v,
	const GraphAttributes & wGA, node w) const
{
	if(m_compareBy == index)
		return compareById(v,w);
	else if(m_compareBy == label)
		return compareByLabel(vGA, v, wGA, w);
	else
	{
		OGDF_ASSERT( false ); // m_compareBy is not set correctly
		return false;
	}

} // end compare

} // end namespace ogdf
