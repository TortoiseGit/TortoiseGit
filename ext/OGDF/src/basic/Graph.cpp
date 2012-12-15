/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Graph class
 *
 * \author Carsten Gutwenger
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

#include <ogdf/basic/Array.h>
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/fileformats/GmlParser.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/GraphObserver.h>


#define MIN_NODE_TABLE_SIZE (1 << 4)
#define MIN_EDGE_TABLE_SIZE (1 << 4)


namespace ogdf {

Graph::Graph()
{
	m_nNodes = m_nEdges = m_nodeIdCount = m_edgeIdCount = 0;
	m_nodeArrayTableSize = MIN_NODE_TABLE_SIZE;
	m_edgeArrayTableSize = MIN_EDGE_TABLE_SIZE;
}


Graph::Graph(const Graph &G)
{
	m_nNodes = m_nEdges = m_nodeIdCount = m_edgeIdCount = 0;
	copy(G);
	m_nodeArrayTableSize = nextPower2(MIN_NODE_TABLE_SIZE,m_nodeIdCount);
	m_edgeArrayTableSize = nextPower2(MIN_EDGE_TABLE_SIZE,m_edgeIdCount);
}


Graph::~Graph()
{
	ListIterator<NodeArrayBase*> itVNext;
	for(ListIterator<NodeArrayBase*> itV = m_regNodeArrays.begin();
		itV.valid(); itV = itVNext)
	{
		itVNext = itV.succ();
		(*itV)->disconnect();
	}

	ListIterator<EdgeArrayBase*> itENext;
	for(ListIterator<EdgeArrayBase*> itE = m_regEdgeArrays.begin();
		itE.valid(); itE = itENext)
	{
		itENext = itE.succ();
		(*itE)->disconnect();
	}

	ListIterator<AdjEntryArrayBase*> itAdjNext;
	for(ListIterator<AdjEntryArrayBase*> itAdj = m_regAdjArrays.begin();
		itAdj.valid(); itAdj = itAdjNext)
	{
		itAdjNext = itAdj.succ();
		(*itAdj)->disconnect();
	}

	for (node v = m_nodes.begin(); v; v = v->succ()) {
		v->m_adjEdges.~GraphList<AdjElement>();
	}
}


Graph &Graph::operator=(const Graph &G)
{
	clear(); copy(G);
	m_nodeArrayTableSize = nextPower2(MIN_NODE_TABLE_SIZE,m_nodeIdCount);
	m_edgeArrayTableSize = nextPower2(MIN_EDGE_TABLE_SIZE,m_edgeIdCount);
	reinitArrays();

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return *this;
}


void Graph::assign(const Graph &G, NodeArray<node> &mapNode,
	EdgeArray<edge> &mapEdge)
{
	clear();
	copy(G,mapNode,mapEdge);
	m_nodeArrayTableSize = nextPower2(MIN_NODE_TABLE_SIZE,m_nodeIdCount);
	m_edgeArrayTableSize = nextPower2(MIN_EDGE_TABLE_SIZE,m_edgeIdCount);
	reinitArrays();
}


void Graph::construct(const Graph &G, NodeArray<node> &mapNode,
	EdgeArray<edge> &mapEdge)
{
	copy(G,mapNode,mapEdge);
	m_nodeArrayTableSize = nextPower2(MIN_NODE_TABLE_SIZE,m_nodeIdCount);
	m_edgeArrayTableSize = nextPower2(MIN_EDGE_TABLE_SIZE,m_edgeIdCount);
}


void Graph::copy(const Graph &G, NodeArray<node> &mapNode,
	EdgeArray<edge> &mapEdge)
{
	if (G.m_nNodes == 0) return;

	mapNode.init(G,0);

	node vG;
	forall_nodes(vG,G) {
		node v = mapNode[vG] = pureNewNode();
		v->m_indeg = vG->m_indeg;
		v->m_outdeg = vG->m_outdeg;
	}

	if (G.m_nEdges == 0) return;

	mapEdge.init(G,0);

	edge e, eC;
	forall_edges(e,G) {
		m_edges.pushBack(eC = mapEdge[e] =
			OGDF_NEW EdgeElement(
				mapNode[e->source()],mapNode[e->target()],m_edgeIdCount));

		eC->m_adjSrc = OGDF_NEW AdjElement(eC,m_edgeIdCount<<1);
		(eC->m_adjTgt = OGDF_NEW AdjElement(eC,(m_edgeIdCount<<1)|1))
			->m_twin = eC->m_adjSrc;
		eC->m_adjSrc->m_twin = eC->m_adjTgt;
		m_edgeIdCount++;
	}
	m_nEdges = G.m_nEdges;

	EdgeArray<bool> mark(G,false);

	forall_nodes(vG,G) {
		node v = mapNode[vG];
		GraphList<AdjElement> &adjEdges = vG->m_adjEdges;
		for (AdjElement *adjG = adjEdges.begin(); adjG; adjG = adjG->succ()) {
			int id = adjG->m_edge->index();
			edge eC = mapEdge[id];

			adjEntry adj;
			if (eC->isSelfLoop()) {
				if (mark[id])
					adj = eC->m_adjTgt;
				else {
					adj = eC->m_adjSrc;
					mark[id] = true;
				}
			} else
				adj = (v == eC->m_src) ? eC->m_adjSrc : eC->m_adjTgt;

			v->m_adjEdges.pushBack(adj);
			adj->m_node = v;
		}
	}
}


void Graph::copy(const Graph &G)
{
	NodeArray<node> mapNode;
	EdgeArray<edge> mapEdge;
	copy(G,mapNode,mapEdge);

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


void Graph::constructInitByNodes(
	const Graph &G,
	const List<node> &nodes,
	NodeArray<node> &mapNode,
	EdgeArray<edge> &mapEdge)
{
	// clear
	for (node v = m_nodes.begin(); v; v = v->succ()) {
		v->m_adjEdges.~GraphList<AdjElement>();
	}

	m_nodes.clear();
	m_edges.clear();

	m_nNodes = m_nEdges = m_nodeIdCount = m_edgeIdCount = 0;
	m_nodeArrayTableSize = MIN_NODE_TABLE_SIZE;


	// list of edges adjacent to nodes in nodes
	SListPure<edge> edges;

	// create nodes and assemble list of edges
	ListConstIterator<node> itG;
	for(itG = nodes.begin(); itG.valid(); ++itG) {
		node vG = *itG;
		node v = mapNode[vG] = pureNewNode();

		v->m_indeg = vG->m_indeg;
		v->m_outdeg = vG->m_outdeg;

		adjEntry adjG;
		forall_adj(adjG,vG) {
			// corresponding adjacency entries differ by index modulo 2
			// the following conditions makes sure that each edge is
			// added only once to edges
			if ((adjG->m_id & 1) == 0)
				edges.pushBack(adjG->m_edge);
		}
	}

	// create edges
	SListConstIterator<edge> it;
	for(it = edges.begin(); it.valid(); ++it)
	{
		edge eG = *it;
		node v = mapNode[eG->source()];
		node w = mapNode[eG->target()];

		edge eC = mapEdge[eG] = OGDF_NEW EdgeElement(v, w, m_edgeIdCount);
		m_edges.pushBack(eC);

		eC->m_adjSrc = OGDF_NEW AdjElement(eC, m_edgeIdCount<<1);
		(eC->m_adjTgt = OGDF_NEW AdjElement(eC, (m_edgeIdCount<<1)|1))
			->m_twin = eC->m_adjSrc;
		eC->m_adjSrc->m_twin = eC->m_adjTgt;
		++m_edgeIdCount;
		++m_nEdges;
	}

	EdgeArray<bool> mark(G,false);
	for(itG = nodes.begin(); itG.valid(); ++itG) {
		node vG = *itG;
		node v = mapNode[vG];

		GraphList<AdjElement> &adjEdges = vG->m_adjEdges;
		for (AdjElement *adjG = adjEdges.begin(); adjG; adjG = adjG->succ()) {
			int id = adjG->m_edge->index();
			edge eC = mapEdge[id];

			adjEntry adj;
			if (eC->isSelfLoop()) {
				if (mark[id])
					adj = eC->m_adjTgt;
				else {
					adj = eC->m_adjSrc;
					mark[id] = true;
				}
			} else
				adj = (v == eC->m_src) ? eC->m_adjSrc : eC->m_adjTgt;

			v->m_adjEdges.pushBack(adj);
			adj->m_node = v;
		}
	}

/*
		AdjElement *adjSrc = OGDF_NEW AdjElement(v);

		v->m_adjEdges.pushBack(adjSrc);
		//v->m_outdeg++;

		AdjElement *adjTgt = OGDF_NEW AdjElement(w);

		w->m_adjEdges.pushBack(adjTgt);
		//w->m_indeg++;

		adjSrc->m_twin = adjTgt;
		adjTgt->m_twin = adjSrc;

		adjTgt->m_id = (adjSrc->m_id = m_edgeIdCount << 1) | 1;
		edge e = OGDF_NEW EdgeElement(v,w,adjSrc,adjTgt,m_edgeIdCount++);

		++m_nEdges;
		m_edges.pushBack(e);

		mapEdge[eG] = adjSrc->m_edge = adjTgt->m_edge = e;
	}*/

	// set size of associated arrays and reinitialize all (we have now a
	// completely new graph)
	m_nodeArrayTableSize = nextPower2(MIN_NODE_TABLE_SIZE,m_nodeIdCount);
	m_edgeArrayTableSize = nextPower2(MIN_EDGE_TABLE_SIZE,m_edgeIdCount);
	reinitArrays();

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


//------------------
//mainly a copy of the above code, remerge this again
void Graph::constructInitByActiveNodes(
	const List<node> &nodes,
	const NodeArray<bool> &activeNodes,
	NodeArray<node> &mapNode,
	EdgeArray<edge> &mapEdge)
{
	// clear
	for (node v = m_nodes.begin(); v; v = v->succ()) {
		v->m_adjEdges.~GraphList<AdjElement>();
	}

	m_nodes.clear();
	m_edges.clear();

	m_nNodes = m_nEdges = m_nodeIdCount = m_edgeIdCount = 0;
	m_nodeArrayTableSize = MIN_NODE_TABLE_SIZE;


	// list of edges adjacent to nodes in nodes
	SListPure<edge> edges;

	// create nodes and assemble list of edges
	//NOTE: nodes is a list of ACTIVE nodes
	ListConstIterator<node> itG;
	for(itG = nodes.begin(); itG.valid(); ++itG) {
		node vG = *itG;
		node v = mapNode[vG] = pureNewNode();

		//we cannot assign the original degree, as there
		//may be edges to non-active nodes
		//v->m_indeg = vG->m_indeg;
		//v->m_outdeg = vG->m_outdeg;

		int inCount = 0;
		int outCount = 0;
		adjEntry adjG;
		forall_adj(adjG,vG)
		{
			// coresponding adjacency entries differ by index modulo 2
			// the following conditions makes sure that each edge is
			// added only once to edges
			if (activeNodes[adjG->m_edge->opposite(vG)])
			{
				if ((adjG->m_id & 1) == 0)
				{
					edges.pushBack(adjG->m_edge);
				}//if one time
				if (adjG->m_edge->source() == vG) outCount++;
					else inCount++;
			}//if opposite active
		}//foralladj
		v->m_indeg = inCount;
		v->m_outdeg = outCount;
	}//for nodes

	// create edges
	SListConstIterator<edge> it;
	for(it = edges.begin(); it.valid(); ++it)
	{
		edge eG = *it;
		node v = mapNode[eG->source()];
		node w = mapNode[eG->target()];

		AdjElement *adjSrc = OGDF_NEW AdjElement(v);

		v->m_adjEdges.pushBack(adjSrc);
		//v->m_outdeg++;

		AdjElement *adjTgt = OGDF_NEW AdjElement(w);

		w->m_adjEdges.pushBack(adjTgt);
		//w->m_indeg++;

		adjSrc->m_twin = adjTgt;
		adjTgt->m_twin = adjSrc;

		adjTgt->m_id = (adjSrc->m_id = m_edgeIdCount << 1) | 1;
		edge e = OGDF_NEW EdgeElement(v,w,adjSrc,adjTgt,m_edgeIdCount++);

		++m_nEdges;
		m_edges.pushBack(e);

		mapEdge[eG] = adjSrc->m_edge = adjTgt->m_edge = e;
	}

	// set size of associated arrays and reinitialize all (we have now a
	// completely new graph)
	m_nodeArrayTableSize = nextPower2(MIN_NODE_TABLE_SIZE,m_nodeIdCount);
	m_edgeArrayTableSize = nextPower2(MIN_EDGE_TABLE_SIZE,m_edgeIdCount);
	reinitArrays();

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}//constructinitbyactivenodes

//------------------



node Graph::newNode()
{
	++m_nNodes;
	if (m_nodeIdCount == m_nodeArrayTableSize) {
		m_nodeArrayTableSize <<= 1;
		for(ListIterator<NodeArrayBase*> it = m_regNodeArrays.begin();
			it.valid(); ++it)
		{
			(*it)->enlargeTable(m_nodeArrayTableSize);
		}
	}

#ifdef OGDF_DEBUG
	node v = OGDF_NEW NodeElement(this,m_nodeIdCount++);
#else
	node v = OGDF_NEW NodeElement(m_nodeIdCount++);
#endif

	m_nodes.pushBack(v);
	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->nodeAdded(v);

	return v;
}


//what about negative index numbers?
node Graph::newNode(int index)
{
	++m_nNodes;

	if(index >= m_nodeIdCount) {
		m_nodeIdCount = index+1;

		if(index >= m_nodeArrayTableSize) {
			m_nodeArrayTableSize = nextPower2(m_nodeArrayTableSize,index);
			for(ListIterator<NodeArrayBase*> it = m_regNodeArrays.begin();
				it.valid(); ++it)
			{
				(*it)->enlargeTable(m_nodeArrayTableSize);
			}
		}
	}

#ifdef OGDF_DEBUG
	node v = OGDF_NEW NodeElement(this,index);
#else
	node v = OGDF_NEW NodeElement(index);
#endif

	m_nodes.pushBack(v);
	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->nodeAdded(v);
	return v;
}


node Graph::pureNewNode()
{
	++m_nNodes;

#ifdef OGDF_DEBUG
	node v = OGDF_NEW NodeElement(this,m_nodeIdCount++);
#else
	node v = OGDF_NEW NodeElement(m_nodeIdCount++);
#endif

	m_nodes.pushBack(v);
	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->nodeAdded(v);
	return v;
}


// IMPORTANT:
// The indices of the two adjacency entries pointing to an edge differ
// only in the last bit (adjSrc/2 == adjTgt/2)
//
// This can be useful sometimes in order to avoid visiting an edge twice.
edge Graph::createEdgeElement(node v, node w, adjEntry adjSrc, adjEntry adjTgt)
{
	if (m_edgeIdCount == m_edgeArrayTableSize) {
		m_edgeArrayTableSize <<= 1;

		for(ListIterator<EdgeArrayBase*> it = m_regEdgeArrays.begin();
			it.valid(); ++it)
		{
			(*it)->enlargeTable(m_edgeArrayTableSize);
		}

		for(ListIterator<AdjEntryArrayBase*> itAdj = m_regAdjArrays.begin();
			itAdj.valid(); ++itAdj)
		{
			(*itAdj)->enlargeTable(m_edgeArrayTableSize << 1);
		}
	}

	adjTgt->m_id = (adjSrc->m_id = m_edgeIdCount << 1) | 1;
	edge e = OGDF_NEW EdgeElement(v,w,adjSrc,adjTgt,m_edgeIdCount++);
	m_edges.pushBack(e);
	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->edgeAdded(e);
	return e;
}


edge Graph::newEdge(node v, node w, int index)
{
	OGDF_ASSERT(v != 0 && w != 0);
	OGDF_ASSERT(v->graphOf() == this && w->graphOf() == this);

	++m_nEdges;

	AdjElement *adjSrc = OGDF_NEW AdjElement(v);

	v->m_adjEdges.pushBack(adjSrc);
	v->m_outdeg++;

	AdjElement *adjTgt = OGDF_NEW AdjElement(w);

	w->m_adjEdges.pushBack(adjTgt);
	w->m_indeg++;

	adjSrc->m_twin = adjTgt;
	adjTgt->m_twin = adjSrc;

	if(index >= m_edgeIdCount) {
		m_edgeIdCount = index+1;

		if(index >= m_edgeArrayTableSize) {
			m_edgeArrayTableSize = nextPower2(m_edgeArrayTableSize,index);

			for(ListIterator<EdgeArrayBase*> it = m_regEdgeArrays.begin();
				it.valid(); ++it)
			{
				(*it)->enlargeTable(m_edgeArrayTableSize);
			}

			for(ListIterator<AdjEntryArrayBase*> itAdj = m_regAdjArrays.begin();
				itAdj.valid(); ++itAdj)
			{
				(*itAdj)->enlargeTable(m_edgeArrayTableSize << 1);
			}
		}
	}

	adjTgt->m_id = (adjSrc->m_id = index/*m_edgeIdCount*/ << 1) | 1;
	edge e = OGDF_NEW EdgeElement(v,w,adjSrc,adjTgt,index);
	m_edges.pushBack(e);
	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->edgeAdded(e);
	return adjSrc->m_edge = adjTgt->m_edge = e;
}


edge Graph::newEdge(node v, node w)
{
	OGDF_ASSERT(v != 0 && w != 0);
	OGDF_ASSERT(v->graphOf() == this && w->graphOf() == this);

	++m_nEdges;

	AdjElement *adjSrc = OGDF_NEW AdjElement(v);

	v->m_adjEdges.pushBack(adjSrc);
	v->m_outdeg++;

	AdjElement *adjTgt = OGDF_NEW AdjElement(w);

	w->m_adjEdges.pushBack(adjTgt);
	w->m_indeg++;

	adjSrc->m_twin = adjTgt;
	adjTgt->m_twin = adjSrc;

	edge e = createEdgeElement(v,w,adjSrc,adjTgt);

	return adjSrc->m_edge = adjTgt->m_edge = e;
}


edge Graph::newEdge(adjEntry adjStart, adjEntry adjEnd, Direction dir)
{
	OGDF_ASSERT(adjStart != 0 && adjEnd != 0)
	OGDF_ASSERT(adjStart->graphOf() == this && adjEnd->graphOf() == this);

	++m_nEdges;

	node v = adjStart->theNode(), w = adjEnd->theNode();

	AdjElement *adjTgt = OGDF_NEW AdjElement(w);
	AdjElement *adjSrc = OGDF_NEW AdjElement(v);

	if(dir == ogdf::after) {
		w->m_adjEdges.insertAfter(adjTgt,adjEnd);
		v->m_adjEdges.insertAfter(adjSrc,adjStart);
	} else {
		w->m_adjEdges.insertBefore(adjTgt,adjEnd);
		v->m_adjEdges.insertBefore(adjSrc,adjStart);
	}

	w->m_indeg++;
	v->m_outdeg++;

	adjSrc->m_twin = adjTgt;
	adjTgt->m_twin = adjSrc;

	edge e = createEdgeElement(v,w,adjSrc,adjTgt);

	return adjSrc->m_edge = adjTgt->m_edge = e;
}

edge Graph::newEdge(node v, adjEntry adjEnd)
{
	OGDF_ASSERT(v != 0 && adjEnd != 0)
	OGDF_ASSERT(v->graphOf() == this && adjEnd->graphOf() == this);

	++m_nEdges;

	node w = adjEnd->theNode();

	AdjElement *adjTgt = OGDF_NEW AdjElement(w);

	w->m_adjEdges.insertAfter(adjTgt,adjEnd);
	w->m_indeg++;

	AdjElement *adjSrc = OGDF_NEW AdjElement(v);

	v->m_adjEdges.pushBack(adjSrc);
	v->m_outdeg++;

	adjSrc->m_twin = adjTgt;
	adjTgt->m_twin = adjSrc;

	edge e = createEdgeElement(v,w,adjSrc,adjTgt);

	return adjSrc->m_edge = adjTgt->m_edge = e;
}//newedge
//copy of above function with edge ending at v
edge Graph::newEdge(adjEntry adjStart, node v)
{
	OGDF_ASSERT(v != 0 && adjStart != 0)
	OGDF_ASSERT(v->graphOf() == this && adjStart->graphOf() == this);

	++m_nEdges;

	node w = adjStart->theNode();

	AdjElement *adjSrc = OGDF_NEW AdjElement(w);

	w->m_adjEdges.insertAfter(adjSrc, adjStart);
	w->m_outdeg++;

	AdjElement *adjTgt = OGDF_NEW AdjElement(v);

	v->m_adjEdges.pushBack(adjTgt);
	v->m_indeg++;

	adjSrc->m_twin = adjTgt;
	adjTgt->m_twin = adjSrc;

	edge e = createEdgeElement(w,v,adjSrc,adjTgt);

	return adjSrc->m_edge = adjTgt->m_edge = e;
}//newedge


void Graph::move(edge e,
	adjEntry adjSrc,
	Direction dirSrc,
	adjEntry adjTgt,
	Direction dirTgt)
{
	OGDF_ASSERT(e->graphOf() == this);
	OGDF_ASSERT(adjSrc->graphOf() == this && adjTgt->graphOf() == this);
	OGDF_ASSERT(adjSrc != e->m_adjSrc && adjSrc != e->m_adjTgt);
	OGDF_ASSERT(adjTgt != e->m_adjSrc && adjTgt != e->m_adjTgt);

	node v = adjSrc->m_node, w = adjTgt->m_node;
	adjEntry adj1 = e->m_adjSrc, adj2 = e->m_adjTgt;
	e->m_src->m_adjEdges.move(adj1,v->m_adjEdges,adjSrc,dirSrc);
	e->m_tgt->m_adjEdges.move(adj2,w->m_adjEdges,adjTgt,dirTgt);

	e->m_src->m_outdeg--;
	e->m_tgt->m_indeg--;

	adj1->m_node = e->m_src = v;
	adj2->m_node = e->m_tgt = w;

	v->m_outdeg++;
	w->m_indeg++;
}


void Graph::moveTarget(edge e, node v)
{
	OGDF_ASSERT(e->graphOf() == this);
	OGDF_ASSERT(v->graphOf() == this);

	adjEntry adj = e->m_adjTgt;
	e->m_tgt->m_adjEdges.move(adj,v->m_adjEdges);

	e->m_tgt->m_indeg--;
	adj->m_node = e->m_tgt = v;
	v->m_indeg++;
}

void Graph::moveTarget(edge e, adjEntry adjTgt, Direction dir)
{
	node v = adjTgt->theNode();

	OGDF_ASSERT(e->graphOf() == this);
	OGDF_ASSERT(v->graphOf() == this);

	adjEntry adj = e->m_adjTgt;
	e->m_tgt->m_adjEdges.move(adj,v->m_adjEdges, adjTgt, dir);

	e->m_tgt->m_indeg--;
	adj->m_node = e->m_tgt = v;
	v->m_indeg++;
}

// By Leipert
void Graph::moveSource(edge e, node v)
{
	OGDF_ASSERT(e->graphOf() == this);
	OGDF_ASSERT(v->graphOf() == this);

	adjEntry adj = e->m_adjSrc;
	e->m_src->m_adjEdges.move(adj,v->m_adjEdges);

	e->m_src->m_outdeg--;
	adj->m_node = e->m_src = v;
	v->m_outdeg++;
}

void Graph::moveSource(edge e, adjEntry adjSrc, Direction dir)
{
	node v = adjSrc->theNode();

	OGDF_ASSERT(e->graphOf() == this);
	OGDF_ASSERT(v->graphOf() == this);

	adjEntry adj = e->m_adjSrc;
	e->m_src->m_adjEdges.move(adj,v->m_adjEdges, adjSrc, dir);

	e->m_src->m_outdeg--;
	adj->m_node = e->m_src = v;
	v->m_outdeg++;
}

edge Graph::split(edge e)
{
	OGDF_ASSERT(e != 0 && e->graphOf() == this);

	++m_nEdges;

	node u = newNode();
	u->m_indeg = u->m_outdeg = 1;

	adjEntry adjTgt = OGDF_NEW AdjElement(u);
	adjTgt->m_edge = e;
	adjTgt->m_twin = e->m_adjSrc;
	e->m_adjSrc->m_twin = adjTgt;

	// adapt adjacency entry index to hold invariant
	adjTgt->m_id = e->m_adjTgt->m_id;

	u->m_adjEdges.pushBack(adjTgt);

	adjEntry adjSrc = OGDF_NEW AdjElement(u);
	adjSrc->m_twin = e->m_adjTgt;
	u->m_adjEdges.pushBack(adjSrc);

	int oldId = e->m_adjTgt->m_id;
	edge e2 = createEdgeElement(u,e->m_tgt,adjSrc,e->m_adjTgt);
	resetAdjEntryIndex(e->m_adjTgt->m_id,oldId);

	e2->m_adjTgt->m_twin = adjSrc;
	e->m_adjTgt->m_edge = adjSrc->m_edge = e2;

	e->m_tgt = u;
	e->m_adjTgt = adjTgt;
	return e2;
}


void Graph::unsplit(node u)
{
	edge eIn = u->firstAdj()->theEdge();
	edge eOut = u->lastAdj()->theEdge();

	if (eIn->target() != u)
		swap(eIn,eOut);

	unsplit(eIn,eOut);
}



void Graph::unsplit(edge eIn, edge eOut)
{
	node u = eIn->target();

	// u must be a node with exactly one incoming edge eIn and one outgoing
	// edge eOut
	OGDF_ASSERT(u->graphOf() == this && u->indeg() == 1 &&
		u->outdeg() == 1 && eOut->source() == u);

	// none of them is a self-loop!
	OGDF_ASSERT(eIn->isSelfLoop() == false && eOut->isSelfLoop() == false);

	// we reuse these adjacency entries
	adjEntry adjSrc = eIn ->m_adjSrc;
	adjEntry adjTgt = eOut->m_adjTgt;

	eIn->m_tgt = eOut->m_tgt;

	// adapt adjacency entry index to hold invariant
	resetAdjEntryIndex(eIn->m_adjTgt->m_id,adjTgt->m_id);
	adjTgt->m_id = eIn->m_adjTgt->m_id; // correct id of adjacency entry!

	eIn->m_adjTgt = adjTgt;

	adjSrc->m_twin = adjTgt;
	adjTgt->m_twin = adjSrc;

	adjTgt->m_edge = eIn;

	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->edgeDeleted(eOut);
	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->nodeDeleted(u);
	// remove structures that are no longer used
	m_edges.del(eOut);
	m_nodes.del(u);
	--m_nNodes;
	--m_nEdges;

}


void Graph::delNode(node v)
{
	OGDF_ASSERT(v != 0 && v->graphOf() == this)

	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->nodeDeleted(v);

	--m_nNodes;

	GraphList<AdjElement> &adjEdges = v->m_adjEdges;
	AdjElement *adj;
	while((adj = adjEdges.begin()) != 0)
		delEdge(adj->m_edge);

	m_nodes.del(v);
}


void Graph::delEdge(edge e)
{
	OGDF_ASSERT(e != 0 && e->graphOf() == this)

	//  notify all registered observers
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it) (*it)->edgeDeleted(e);

	--m_nEdges;

	node src = e->m_src, tgt = e->m_tgt;

	src->m_adjEdges.del(e->m_adjSrc);
	src->m_outdeg--;
	tgt->m_adjEdges.del(e->m_adjTgt);
	tgt->m_indeg--;

	m_edges.del(e);
}


void Graph::clear()
{
	//tell all structures to clear their graph-initialized data
	for(ListIterator<GraphObserver*> it = m_regStructures.begin();
			it.valid(); ++it)
	{
		(*it)->cleared();
	}//for
	for (node v = m_nodes.begin(); v; v = v->succ()) {
		v->m_adjEdges.~GraphList<AdjElement>();
	}

	m_nodes.clear();
	m_edges.clear();

	m_nNodes = m_nEdges = m_nodeIdCount = m_edgeIdCount = 0;
	m_nodeArrayTableSize = MIN_NODE_TABLE_SIZE;
	reinitArrays();

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


void Graph::reverseEdge(edge e)
{
	OGDF_ASSERT(e != 0 && e->graphOf() == this)
	node &src = e->m_src, &tgt = e->m_tgt;

	swap(src,tgt);
	swap(e->m_adjSrc,e->m_adjTgt);
	src->m_outdeg++; src->m_indeg--;
	tgt->m_outdeg--; tgt->m_indeg++;
}


void Graph::reverseAllEdges()
{
	for (edge e = m_edges.begin(); e; e = e->succ())
		reverseEdge(e);

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


void Graph::reverseAdjEdges()
{
	node v;
	forall_nodes(v,*this)
		reverseAdjEdges(v);
}


node Graph::chooseNode() const
{
	if (m_nNodes == 0) return 0;
	int k = ogdf::randomNumber(0,m_nNodes-1);
	node v = firstNode();
	while(k--) v = v->succ();
	return v;
}


edge Graph::chooseEdge() const
{
	if (m_nEdges == 0) return 0;
	int k = ogdf::randomNumber(0,m_nEdges-1);
	edge e = firstEdge();
	while(k--) e = e->succ();
	return e;
}


edge Graph::searchEdge(node v, node w) const
{
	OGDF_ASSERT(v != 0 && v->graphOf() == this)
	OGDF_ASSERT(w != 0 && w->graphOf() == this)
	adjEntry adj;
	forall_adj(adj,v) {
		if(adj->twinNode() == w) return adj->twin()->theEdge();
	}
	return 0;
}


void Graph::hideEdge(edge e)
{
	OGDF_ASSERT(e != 0 && e->graphOf() == this)
	--m_nEdges;

	node src = e->m_src, tgt = e->m_tgt;

	src->m_adjEdges.delPure(e->m_adjSrc);
	src->m_outdeg--;
	tgt->m_adjEdges.delPure(e->m_adjTgt);
	tgt->m_indeg--;

	m_edges.move(e, m_hiddenEdges);
}


void Graph::restoreEdge(edge e)
{
	++m_nEdges;

	node v = e->m_src;
	v->m_adjEdges.pushBack(e->m_adjSrc);
	++v->m_outdeg;

	node w = e->m_tgt;
	w->m_adjEdges.pushBack(e->m_adjTgt);
	++w->m_indeg;

	m_hiddenEdges.move(e, m_edges);
}


void Graph::restoreAllEdges()
{
	edge e, ePrev;
	for(e = m_hiddenEdges.rbegin(); e != 0; e = ePrev) {
		ePrev = e->pred();
		restoreEdge(e);
	}
}


int Graph::genus() const
{
	if (m_nNodes == 0) return 0;

	int nIsolated = 0;
	node v;
	forall_nodes(v,*this)
		if (v->degree() == 0) ++nIsolated;

	NodeArray<int> component(*this);
	int nCC = connectedComponents(*this,component);

	AdjEntryArray<bool> visited(*this,false);
	int nFaceCycles = 0;

	forall_nodes(v,*this) {
		adjEntry adj1;
		forall_adj(adj1,v) {
			if (visited[adj1]) continue;

			adjEntry adj = adj1;
			do {
				visited[adj] = true;
				adj = adj->faceCycleSucc();
			} while (adj != adj1);

			++nFaceCycles;
		}
	}

	return (m_nEdges - m_nNodes - nIsolated - nFaceCycles + 2*nCC) / 2;
}


ListIterator<NodeArrayBase*> Graph::registerArray(
	NodeArrayBase *pNodeArray) const
{
	return m_regNodeArrays.pushBack(pNodeArray);
}


ListIterator<EdgeArrayBase*> Graph::registerArray(
	EdgeArrayBase *pEdgeArray) const
{
	return m_regEdgeArrays.pushBack(pEdgeArray);
}


ListIterator<AdjEntryArrayBase*> Graph::registerArray(
	AdjEntryArrayBase *pAdjArray) const
{
	return m_regAdjArrays.pushBack(pAdjArray);
}

ListIterator<GraphObserver*> Graph::registerStructure(
	GraphObserver *pStructure) const
{
	return m_regStructures.pushBack(pStructure);
}//registerstructure


void Graph::unregisterArray(ListIterator<NodeArrayBase*> it) const
{
	m_regNodeArrays.del(it);
}


void Graph::unregisterArray(ListIterator<EdgeArrayBase*> it) const
{
	m_regEdgeArrays.del(it);
}


void Graph::unregisterArray(ListIterator<AdjEntryArrayBase*> it) const
{
	m_regAdjArrays.del(it);
}

void Graph::unregisterStructure(ListIterator<GraphObserver*> it) const
{
	m_regStructures.del(it);
}

void Graph::reinitArrays()
{
	ListIterator<NodeArrayBase*> itNode = m_regNodeArrays.begin();
	for(; itNode.valid(); ++itNode)
		(*itNode)->reinit(m_nodeArrayTableSize);

	ListIterator<EdgeArrayBase*> itEdge = m_regEdgeArrays.begin();
	for(; itEdge.valid(); ++itEdge)
		(*itEdge)->reinit(m_edgeArrayTableSize);

	ListIterator<AdjEntryArrayBase*> itAdj = m_regAdjArrays.begin();
	for(; itAdj.valid(); ++itAdj)
		(*itAdj)->reinit(m_edgeArrayTableSize << 1);
}

void Graph::reinitStructures()
{
	//is there a challenge?
	ListIterator<GraphObserver*> itGS = m_regStructures.begin();
	for (;itGS.valid(); ++itGS)
		(*itGS)->reInit();
}


void Graph::resetAdjEntryIndex(int newIndex, int oldIndex)
{
	ListIterator<AdjEntryArrayBase*> itAdj = m_regAdjArrays.begin();
	for(; itAdj.valid(); ++itAdj)
		(*itAdj)->resetIndex(newIndex,oldIndex);
}


int Graph::nextPower2(int start, int idCount)
{
	while (start <= idCount)
		start <<= 1;

	return start;
}


bool Graph::readGML(const char *fileName)
{
	ifstream is(fileName);
	return readGML(is);
}


bool Graph::readGML(istream &is)
{
	GmlParser gml(is);
	if (gml.error()) return false;
	bool result = gml.read(*this);

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return result;
}


void Graph::writeGML(const char *fileName) const
{
	ofstream os(fileName);
	writeGML(os);
}


void Graph::writeGML(ostream &os) const
{
	NodeArray<int> id(*this);
	int nextId = 0;

	os << "Creator \"ogdf::Graph::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	node v;
	forall_nodes(v,*this) {
		os << "  node [\n";
		os << "    id " << (id[v] = nextId++) << "\n";
		os << "  ]\n"; // node
	}

	edge e;
	forall_edges(e,*this) {
		os << "  edge [\n";
		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";
		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}


// read graph in LEDA format from file fileName
bool Graph::readLEDAGraph(const char *fileName)
{
	ifstream is(fileName);
	return readLEDAGraph(is);
}


bool Graph::readToEndOfLine(istream &is)
{
	int c;
	do {
		if (is.eof()) return false;
		c = is.get();
	} while(c != '\n');
	return true;
}


// read graph in LEDA format from input stream is
bool Graph::readLEDAGraph(istream &is)
{
	clear();

	// the first three strings in the LEDA format describe the type of the
	// format, of nodes and of edges. We simply ignore the additional node/
	// edge attributes
	String formatType, nodeType, edgeType;

	is >> formatType;
	is >> nodeType;
	is >> edgeType;

	if (formatType != "LEDA.GRAPH")
		return false;


	// number of nodes
	int n;
	is >> n >> std::ws;

	// create n nodes and ignore n lines
	Array<node> nodes(1,n);
	int i;
	for(i = 1; i <= n; ++i) {
		if (readToEndOfLine(is) == false)
			return false;
		nodes[i] = newNode();
	}

	// number of edges
	int m;
	is >> m;

	for(i = 1; i <= m; ++i) {
		// read index of source and target node
		int src, tgt;
		is >> src >> tgt;

		// indices valid?
		if (src < 1 || n < src || tgt < 1 || n < tgt)
			return false;

		newEdge(nodes[src],nodes[tgt]);

		// ignore rest of line
		if (readToEndOfLine(is) == false)
			return false;
	}

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return true;
}


bool Graph::consistencyCheck() const
{
	int n = 0;
	node v;
	forall_nodes(v,*this) {
#ifdef OGDF_DEBUG
		if (v->graphOf() != this)
			return false;
#endif

		n++;
		int in = 0, out = 0;

		adjEntry adj;
		forall_adj(adj,v) {
			edge e = adj->m_edge;
			if (adj->m_twin->m_edge != e)
				return false;

			if (e->m_adjSrc == adj)
				out++;
			else if (e->m_adjTgt == adj)
				in++;
			else
				return false;

			if (adj->m_node != v)
				return false;

#ifdef OGDF_DEBUG
			if (adj->graphOf() != this)
				return false;
#endif
		}

		if (v->m_indeg != in)
			return false;

		if (v->m_outdeg != out)
			return false;
	}

	if (n != m_nNodes)
		return false;

	int m = 0;
	edge e;
	forall_edges(e,*this) {
#ifdef OGDF_DEBUG
		if (e->graphOf() != this)
			return false;
#endif

		m++;
		if (e->m_adjSrc == e->m_adjTgt)
			return false;

		if (e->m_adjSrc->m_edge != e)
			return false;

		if (e->m_adjTgt->m_edge != e)
			return false;

		if (e->m_adjSrc->m_node != e->m_src)
			return false;

		if (e->m_adjTgt->m_node != e->m_tgt)
			return false;
	}

	if (m != m_nEdges)
		return false;

	return true;
}


void Graph::resetEdgeIdCount(int maxId)
{
	m_edgeIdCount = maxId+1;

#ifdef OGDF_DEBUG
	if (ogdf::debugLevel >= int(ogdf::dlConsistencyChecks)) {
		edge e;
		forall_edges(e,*this)
		{
			// if there is an edge with higer index than maxId, we cannot
			// set the edge id count to maxId+1
			if (e->index() > maxId)
				OGDF_ASSERT(false);
		}
	}
#endif
}


node Graph::splitNode(adjEntry adjStartLeft, adjEntry adjStartRight)
{
	OGDF_ASSERT(adjStartLeft != 0 && adjStartRight != 0);
	OGDF_ASSERT(adjStartLeft->graphOf() == this && adjStartRight->graphOf() == this);
	OGDF_ASSERT(adjStartLeft->theNode() == adjStartRight->theNode());

	node w = newNode();

	adjEntry adj, adjSucc;
	for(adj = adjStartRight; adj != adjStartLeft; adj = adjSucc) {
		adjSucc = adj->cyclicSucc();
		moveAdj(adj,w);
	}

	newEdge(adjStartLeft, adjStartRight, ogdf::before);

	return w;
}


node Graph::contract(edge e)
{
	adjEntry adjSrc = e->adjSource();
	adjEntry adjTgt = e->adjTarget();
	node v = e->source();
	node w = e->target();

	adjEntry adjNext;
	for(adjEntry adj = adjTgt->cyclicSucc(); adj != adjTgt; adj = adjNext)
	{
		adjNext = adj->cyclicSucc();

		edge eAdj = adj->theEdge();
		if(w == eAdj->source())
			moveSource(eAdj, adjSrc, before);
		else
			moveTarget(eAdj, adjSrc, before);
	}

	delNode(adjTgt->theNode());

	return v;
}


void Graph::moveAdj(adjEntry adj, node w)
{
	node v = adj->m_node;

	v->m_adjEdges.move(adj,w->m_adjEdges);
	adj->m_node = w;

	edge e = adj->m_edge;
	if(v == e->m_src) {
		--v->m_outdeg;
		e->m_src = w;
		++w->m_outdeg;
	} else {
		--v->m_indeg;
		e->m_tgt = w;
		++w->m_indeg;
	}
}


ostream &operator<<(ostream &os, ogdf::node v)
{
	if (v) os << v->index(); else os << "nil";
	return os;
}

ostream &operator<<(ostream &os, ogdf::edge e)
{
	if (e) os << "(" << e->source() << "," << e->target() << ")";
	else os << "nil";
	return os;
}

ostream &operator<<(ostream &os, ogdf::adjEntry adj)
{
	if (adj) {
		ogdf::edge e = adj->theEdge();
		if (adj == e->adjSource())
			os << e->source() << "->" << e->target();
		else
			os << e->target() << "->" << e->source();
	} else os << "nil";
	return os;
}


} // end namespace ogdf

