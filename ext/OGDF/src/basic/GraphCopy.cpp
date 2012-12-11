/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of GraphCopySimple and GraphCopy classes
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


#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/FaceSet.h>


namespace ogdf {


//---------------------------------------------------------
// GraphCopySimple
// simple graph copies (no support for edge splitting)
//---------------------------------------------------------

GraphCopySimple::GraphCopySimple(const Graph &G) : m_pGraph(&G)
{
	Graph::construct(G,m_vCopy,m_eCopy);

	m_vOrig.init(*this,0);
	m_eOrig.init(*this,0);

	node v;
	forall_nodes(v,G)
		m_vOrig[m_vCopy[v]] = v;

	edge e;
	forall_edges(e,G)
		m_eOrig[m_eCopy[e]] = e;
}


GraphCopySimple::GraphCopySimple(const GraphCopySimple &GC)
{
	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::construct(GC,vCopy,eCopy);
	initGC(GC,vCopy,eCopy);
}


GraphCopySimple &GraphCopySimple::operator=(const GraphCopySimple &GC)
{
	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::assign(GC,vCopy,eCopy);
	initGC(GC,vCopy,eCopy);

	return *this;
}


void GraphCopySimple::initGC(const GraphCopySimple &GC,
	NodeArray<node> &vCopy,
	EdgeArray<edge> &eCopy)
{
	m_pGraph = GC.m_pGraph;

	m_vOrig.init(*this,0); m_eOrig.init(*this,0);
	m_vCopy.init(*m_pGraph,0); m_eCopy.init(*m_pGraph,0);

	node v;
	forall_nodes(v,GC)
		m_vCopy[m_vOrig[vCopy[v]] = GC.m_vOrig[v]] = vCopy[v];

	edge e;
	forall_edges(e,GC) {
		edge eOrig = GC.m_eOrig[e];
		m_eOrig[eCopy[e]] = eOrig;
		if (eOrig)
			m_eCopy[eOrig] = eCopy[e];
	}
}


//---------------------------------------------------------
// GraphCopy
// graph copies (support for edge splitting)
//---------------------------------------------------------

GraphCopy::GraphCopy(const Graph &G) : m_pGraph(&G)
{
	EdgeArray<edge> eCopy;
	Graph::construct(G,m_vCopy,eCopy);

	m_vOrig.init(*this,0); m_eOrig.init(*this,0);
	m_eCopy.init(*m_pGraph);
	m_eIterator.init(*this,0);

	node v;
	forall_nodes(v,G)
		m_vOrig[m_vCopy[v]] = v;

	edge e;
	forall_edges(e,G) {
		m_eIterator[eCopy[e]] = m_eCopy[e].pushBack(eCopy[e]);
		m_eOrig[eCopy[e]] = e;
	}
}


GraphCopy::GraphCopy(const GraphCopy &GC)
{
	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::construct(GC,vCopy,eCopy);
	initGC(GC,vCopy,eCopy);
}


void GraphCopy::initGC(const GraphCopy &GC,
	NodeArray<node> &vCopy,
	EdgeArray<edge> &eCopy)
{
	m_pGraph = GC.m_pGraph;

	m_vOrig.init(*this,0); m_eOrig.init(*this,0);
	m_vCopy.init(*m_pGraph,0); m_eCopy.init(*m_pGraph);
	m_eIterator.init(*this,0);

	node v, w;
	forall_nodes(v,GC)
		m_vOrig[vCopy[v]] = GC.original(v);

	edge e;
	forall_edges(e,GC)
		m_eOrig[eCopy[e]] = GC.original(e);

	forall_nodes(v,*this)
		if ((w = m_vOrig[v]) != 0) m_vCopy[w] = v;

	forall_edges(e,*m_pGraph) {
		ListConstIterator<edge> it;
		for (it = GC.m_eCopy[e].begin(); it.valid(); ++it)
			m_eIterator[eCopy[*it]] = m_eCopy[e].pushBack(eCopy[*it]);
	}
}


void GraphCopy::createEmpty(const Graph &G)
{
	m_pGraph = &G;

	m_vCopy.init(G,0);
	m_eCopy.init(G);
	m_vOrig.init(*this,0);
	m_eOrig.init(*this,0);
	m_eIterator.init(*this,0);
}


void GraphCopy::initByNodes(const List<node> &nodes, EdgeArray<edge> &eCopy)
{
	Graph::constructInitByNodes(*m_pGraph,nodes,m_vCopy,eCopy);

	ListConstIterator<node> itV;
	for(itV = nodes.begin(); itV.valid(); ++itV)
	{
		node v = *itV;

		m_vOrig[m_vCopy[v]] = v;

		adjEntry adj;
		forall_adj(adj,v) {
			if ((adj->index() & 1) == 0) {
				edge e = adj->theEdge();
				//
				// edge ec = eCopy[e];
				//
				m_eIterator[eCopy[e]] = m_eCopy[e].pushBack(eCopy[e]);
				m_eOrig[eCopy[e]] = e;
			}
		}
	}

}

void GraphCopy::initByActiveNodes(
	const List<node> &nodes,
	const NodeArray<bool> &activeNodes,
	EdgeArray<edge> &eCopy)
{
	Graph::constructInitByActiveNodes(nodes, activeNodes, m_vCopy, eCopy);

	ListConstIterator<node> itV;
	for(itV = nodes.begin(); itV.valid(); ++itV)
	{
		node v = *itV;

		m_vOrig[m_vCopy[v]] = v;

		adjEntry adj;
		forall_adj(adj,v) {
			if ((adj->index() & 1) == 0) {
				edge e = adj->theEdge();
				//
				// edge ec = eCopy[e];
				//
				OGDF_ASSERT(m_eCopy[e].size() == 0)
				if (activeNodes[e->opposite(v)])
				{
					m_eIterator[eCopy[e]] = m_eCopy[e].pushBack(eCopy[e]);
					m_eOrig[eCopy[e]] = e;
				}
			}
		}
	}

}


GraphCopy &GraphCopy::operator=(const GraphCopy &GC)
{
	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::assign(GC,vCopy,eCopy);
	initGC(GC,vCopy,eCopy);

	return *this;
}


void GraphCopy::setOriginalEmbedding()
{
	node v;
	forall_nodes(v, *m_pGraph)
	{
		if (m_vCopy[v] != 0)
		{
			adjEntry adjOr;
			List<adjEntry> newAdjOrder;
			newAdjOrder.clear();

			forall_adj(adjOr, v)
			{
				if (m_eCopy[adjOr->theEdge()].size() > 0){
					//we have outgoing adjEntries for all
					//incoming and outgoing edges, check the direction
					//to find the correct copy adjEntry
					bool outEdge = (adjOr == (adjOr->theEdge()->adjSource()));

					OGDF_ASSERT(chain(adjOr->theEdge()).size() == 1);
					edge cEdge = chain(adjOr->theEdge()).front();
					adjEntry cAdj = (outEdge ? cEdge->adjSource() : cEdge->adjTarget());
					newAdjOrder.pushBack(cAdj);
				}
			}//foralladj
			sort(copy(v), newAdjOrder);
		}
	}//forallnodes
}


edge GraphCopy::split(edge e)
{
	edge eNew  = Graph::split(e);
	edge eOrig = m_eOrig[e];

	if ((m_eOrig[eNew] = eOrig) != 0) {
		m_eIterator[eNew] = m_eCopy[eOrig].insert(eNew,m_eIterator[e],after);
	}

	return eNew;
}


void GraphCopy::unsplit(edge eIn, edge eOut)
{
	edge eOrig = m_eOrig[eOut];

	// update chain of eOrig if eOrig exists
	if (eOrig != 0) {
		m_eCopy[eOrig].del(m_eIterator[eOut]);
	}

	Graph::unsplit(eIn,eOut);
}


edge GraphCopy::newEdge(edge eOrig)
{
	OGDF_ASSERT(eOrig != 0 && eOrig->graphOf() == m_pGraph);
	OGDF_ASSERT(m_eCopy[eOrig].empty()); // no support for edge splitting!

	edge e = Graph::newEdge(m_vCopy[eOrig->source()], m_vCopy[eOrig->target()]);
	m_eCopy[m_eOrig[e] = eOrig].pushBack(e);

	return e;
}

edge GraphCopy::newEdge(edge eOrig, adjEntry adjSrc, node w)
{
	OGDF_ASSERT(eOrig != 0 && eOrig->graphOf() == m_pGraph);
	OGDF_ASSERT(m_eCopy[eOrig].empty()); // no support for edge splitting!
	OGDF_ASSERT(w == m_vCopy[eOrig->target()]);

	edge e = Graph::newEdge(adjSrc, w);
	m_eCopy[m_eOrig[e] = eOrig].pushBack(e);

	return e;
}

edge GraphCopy::newEdge(edge eOrig, node v, adjEntry adjTgt)
{
	OGDF_ASSERT(eOrig != 0 && eOrig->graphOf() == m_pGraph);
	OGDF_ASSERT(m_eCopy[eOrig].empty()); // no support for edge splitting!
	OGDF_ASSERT(v == m_vCopy[eOrig->source()]);

	edge e = Graph::newEdge(v, adjTgt);
	m_eCopy[m_eOrig[e] = eOrig].pushBack(e);

	return e;
}


//inserts edge preserving the embedding
//todo: rename adjEnd to show the symmetric character
edge GraphCopy::newEdge(node v, adjEntry adjEnd, edge eOrig, CombinatorialEmbedding &E)
{
	OGDF_ASSERT(v != 0 && adjEnd != 0)
	OGDF_ASSERT(v->graphOf() == this && adjEnd->graphOf() == this);
	OGDF_ASSERT(&(E.getGraph()) == this)
	OGDF_ASSERT(m_eCopy[eOrig].size() == 0)

	//check which direction is correct
	edge e;
	if (original(v) == eOrig->source())
		e = E.splitFace(v, adjEnd);
	else
		e = E.splitFace(adjEnd, v);
	m_eIterator[e] = m_eCopy[eOrig].pushBack(e);
	m_eOrig[e] = eOrig;

	return e;
}//newedge


void GraphCopy::setEdge(edge eOrig, edge eCopy){
	OGDF_ASSERT(eOrig != 0 && eOrig->graphOf() == m_pGraph);
	OGDF_ASSERT(eCopy != 0 && eCopy->graphOf() == this);
	OGDF_ASSERT(eCopy->target() == m_vCopy[eOrig->target()]);
	OGDF_ASSERT(eCopy->source() == m_vCopy[eOrig->source()]);
	OGDF_ASSERT(m_eCopy[eOrig].empty());

	m_eCopy[m_eOrig[eCopy] = eOrig].pushBack(eCopy);
}


void GraphCopy::insertEdgePathEmbedded(
	edge eOrig,
	CombinatorialEmbedding &E,
	const SList<adjEntry> &crossedEdges)
{
	m_eCopy[eOrig].clear();

	adjEntry adjSrc, adjTgt;
	SListConstIterator<adjEntry> it = crossedEdges.begin();
	SListConstIterator<adjEntry> itLast = crossedEdges.rbegin();

	// iterate over all adjacency entries in crossedEdges except for first
	// and last
	adjSrc = *it;
	for(++it; it != itLast; ++it)
	{
		adjEntry adj = *it;
		// split edge
		node u = E.split(adj->theEdge())->source();

		// determine target adjacency entry and source adjacency entry
		// in the next iteration step
		adjTgt = u->firstAdj();
		adjEntry adjSrcNext = adjTgt->succ();

		if (adjTgt != adj->twin())
			swap(adjTgt,adjSrcNext);

		// insert a new edge into the face
		edge eNew = E.splitFace(adjSrc,adjTgt);
		m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
		m_eOrig[eNew] = eOrig;

		adjSrc = adjSrcNext;
	}

	// insert last edge
	edge eNew = E.splitFace(adjSrc,*it);
	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
	m_eOrig[eNew] = eOrig;
}


void GraphCopy::insertEdgePath(edge eOrig, const SList<adjEntry> &crossedEdges)
{
	node v = copy(eOrig->source());

	SListConstIterator<adjEntry> it;
	for(it = crossedEdges.begin(); it.valid(); ++it)
	{
		node u = split((*it)->theEdge())->source();

		edge eNew = newEdge(v,u);
		m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
		m_eOrig[eNew] = eOrig;

		v = u;
	}

	edge eNew = newEdge(v,copy(eOrig->target()));
	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
	m_eOrig[eNew] = eOrig;
}

void GraphCopy::insertEdgePath(node srcOrig, node tgtOrig, const SList<adjEntry> &crossedEdges)
{
	node v = copy(srcOrig);

	SListConstIterator<adjEntry> it;
	for(it = crossedEdges.begin(); it.valid(); ++it)
	{
		node u = split((*it)->theEdge())->source();

		edge eNew = newEdge(v,u);
	//	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
		m_eOrig[eNew] = 0;

		v = u;
	}

	edge eNew = newEdge(v,copy(tgtOrig));
	//m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
	m_eOrig[eNew] = 0;
}

//inserts crossing between two copy edges already in PlanRep
//returns newly introduced copy edge of crossed edge
//the crossing edge parameter is changed to allow iteration
//over an edge's crossings in the edge direction
//the parameter topDown describes he following:
// if the crossingEdge is running horizontally from left to right,
// is the crossedEdge direction top->down?
edge GraphCopy::insertCrossing(
	edge& crossingEdge,
	edge crossedEdge,
	bool topDown)
	//const SList<edge> &crossedCopies)
{
	//we first split the crossed edge
	edge e = split(crossedEdge);
	edge eOrig = original(crossingEdge);
	adjEntry adSource = crossingEdge->adjSource();

	//now we delete the crossing copy edge and replace it
	//by two edges adjacent to the crossing vertex
	//we have to consider the copy ordering of the
	//original edge
	//we have to keep the correct order of the adjacency entries
	//because even without a combinatorial embedding, the
	//ordering of the edges may already be fixed
	//Problem: wie erkennt man die Reihenfolge am split?
	//Man muss die Richtung der gekreuzten Kante kennen
	//=>Parameter, und hier adjSource und adjTarget trennen
	edge eNew1, eNew2;
	if (topDown)
	{
		//case 1: crossingEdge runs top-down
		eNew1 = newEdge(adSource, e->adjSource());
		eNew2 = newEdge(e->adjSource()->cyclicPred(),
			crossingEdge->adjTarget()->cyclicPred());
	}
	else
	{
		//case 2: crossingEdge runs bottom-up
		eNew1 = newEdge(adSource, e->adjSource()->cyclicPred());
		eNew2 = newEdge(e->adjSource(), crossingEdge->adjTarget()->cyclicPred());
	}//else bottom up
	//insert new edge after old entry
	m_eIterator[eNew1] = m_eCopy[eOrig].insert(eNew1, m_eIterator[crossingEdge]);
	m_eOrig[eNew1] = eOrig;
	m_eIterator[eNew2] = m_eCopy[eOrig].insert(eNew2, m_eIterator[eNew1]);
	m_eOrig[eNew2] = eOrig;
	//now we delete the input copy edge
	m_eCopy[eOrig].del(m_eIterator[crossingEdge]);
	delEdge(crossingEdge);
	crossingEdge = eNew2;

	return e;//eNew2;
}

void GraphCopy::delCopy(edge e)
{
	edge eOrig = m_eOrig[e];

	delEdge(e);
	if (eOrig == 0)	return;

	OGDF_ASSERT(m_eCopy[eOrig].size() == 1);
	m_eCopy[eOrig].clear();
}


void GraphCopy::delCopy(node v)
{
	node w = m_vOrig[v];
	if (w != 0) m_vCopy[w] = 0;

	adjEntry adj;
	forall_adj(adj,v)
	{
		edge eo = m_eOrig[adj->theEdge()];
		if (eo != 0)
			m_eCopy[eo].clear();
	}

	delNode(v);
}


void GraphCopy::removeEdgePathEmbedded(
	CombinatorialEmbedding &E,
	edge eOrig,
	FaceSetPure &newFaces)
{
	const List<edge> &path = m_eCopy[eOrig];
	ListConstIterator<edge> it = path.begin();

	newFaces.insert(E.joinFaces(*it));

	for(++it; it.valid(); ++it)
	{
		edge e = *it;
		node u = e->source();

		newFaces.remove(E.rightFace(e->adjSource()));
		newFaces.remove(E.rightFace(e->adjTarget()));

		newFaces.insert(E.joinFaces(e));

		edge eIn = u->firstAdj()->theEdge();
		edge eOut = u->lastAdj()->theEdge();
		if (eIn->target() != u)
			swap(eIn,eOut);

		E.unsplit(eIn,eOut);
	}

	m_eCopy[eOrig].clear();
}


void GraphCopy::removeEdgePath(edge eOrig)
{
	const List<edge> &path = m_eCopy[eOrig];
	ListConstIterator<edge> it = path.begin();

	delEdge(*it);

	for(++it; it.valid(); ++it)
	{
		edge e = *it;
		node u = e->source();

		delEdge(e);

		edge eIn = u->firstAdj()->theEdge();
		edge eOut = u->lastAdj()->theEdge();
		if (eIn->target() != u)
			swap(eIn,eOut);

		unsplit(eIn,eOut);
	}

	m_eCopy[eOrig].clear();
}



bool GraphCopy::consistencyCheck() const
{
	if (Graph::consistencyCheck() == false)
		return false;

	const Graph &G = *m_pGraph;

	node v, vG;
	forall_nodes(vG,G) {
		v = m_vCopy[vG];
#ifdef OGDF_DEBUG
		if (v && v->graphOf() != this)
			return false;
#endif
		if (v && m_vOrig[v] != vG)
			return false;
	}

	forall_nodes(v,*this) {
		vG = m_vOrig[v];
#ifdef OGDF_DEBUG
		if(vG && vG->graphOf() != &G)
			return false;
#endif
		if (vG && m_vCopy[vG] != v)
			return false;
	}

	edge e, eG;
	forall_edges(eG,G) {
		const List<edge> &path = m_eCopy[eG];
		ListConstIterator<edge> it;
		for(it = path.begin(); it.valid(); ++it) {
			e = *it;
#ifdef OGDF_DEBUG
			if (e->graphOf() != this)
				return false;
#endif
			if (m_eOrig[e] != eG)
				return false;
		}
	}

	forall_edges(e,*this) {
		eG = m_eOrig[e];
#ifdef OGDF_DEBUG
		if(eG && eG->graphOf() != &G)
			return false;
#endif
	}

	return true;
}



} // end namespace ogdf
