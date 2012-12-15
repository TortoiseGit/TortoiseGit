/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of PlanRep base class for planar rep.
 *
 * \author Karsten Klein
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


#include <ogdf/basic/simple_graph_alg.h>

#include <ogdf/basic/extended_graph_alg.h>

#include <ogdf/planarity/PlanRep.h>


namespace ogdf {

PlanRep::PlanRep(const Graph& AG) :
	GraphCopy(),
	m_pGraphAttributes(NULL),
	m_boundaryAdj(AG, 0),//*this, 0),
	m_oriEdgeTypes(AG, 0),
	m_eAuxCopy(AG)
	{
		m_vType        .init(*this, Graph::dummy);
		m_nodeTypes    .init(*this, 0); //the new node type info
		m_expandedNode .init(*this, 0);
		m_expandAdj    .init(*this, 0);
		m_expansionEdge.init(*this, 0);
		m_eType        .init(*this, Graph::association); //should be dummy  or standard, but isnt checked correctly,
		m_edgeTypes    .init(*this, 0); //the new edge type info

		const Graph &G = AG;

		// special way of initializing GraphCopy; we start with an empty copy
		// and add components by need
		GraphCopy::createEmpty(G);

	// compute connected component of G
	NodeArray<int> component(G);
	m_numCC = connectedComponents(G,component);

	// intialize the array of lists of nodes contained in a CC
	m_nodesInCC.init(m_numCC);

	node v;
	forall_nodes(v,G)
		m_nodesInCC[component[v]].pushBack(v);

	m_currentCC = -1;  // not yet initialized
}


PlanRep::PlanRep(const GraphAttributes& AG) :
	GraphCopy(),
	m_pGraphAttributes(&AG),
	m_boundaryAdj(AG.constGraph(), 0),
	m_oriEdgeTypes(AG.constGraph(), 0),
	m_eAuxCopy(AG.constGraph())
{
	m_vType        .init(*this,Graph::dummy);
	m_nodeTypes    .init(*this, 0); //the new node type info
	m_expandedNode .init(*this,0);
	m_expandAdj    .init(*this,0);
	m_expansionEdge.init(*this, 0);
	m_eType        .init(*this, Graph::association); //should be dummy  or standard, but isnt checked correctly,
	m_edgeTypes    .init(*this, 0); //the new edge type info

	const Graph &G = AG.constGraph();

	// special way of initializing GraphCopy; we start with an empty copy
	// and add components by need
	GraphCopy::createEmpty(G);

	// compute connected component of G
	NodeArray<int> component(G);
	m_numCC = connectedComponents(G,component);

	// intialize the array of lists of nodes contained in a CC
	m_nodesInCC.init(m_numCC);

	node v;
	forall_nodes(v,G)
		m_nodesInCC[component[v]].pushBack(v);

	m_currentCC = -1;  // not yet initialized
}//PlanRep


void PlanRep::initCC(int i)
{
	// delete copy / chain fields for originals of nodes in current cc
	// (since we remove all these copies in initByNodes(...)
	if (m_currentCC >= 0)
	{
		const List<node> &origInCC = nodesInCC(i);
		ListConstIterator<node> itV;

		for(itV = origInCC.begin(); itV.valid(); ++itV) {
			node vG = *itV;

			m_vCopy[vG] = 0;

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				m_eCopy[eG].clear();
			}
		}
	}

	m_currentCC = i;
	GraphCopy::initByNodes(m_nodesInCC[i], m_eAuxCopy);

	// set type of edges (gen. or assoc.) in the current CC
	edge e;
	forall_edges(e,*this)
		setCopyType(e, original(e));

	if(m_pGraphAttributes == 0)
		return;

	// The following part is only relevant with given graph attributes!

	node v;
	forall_nodes(v,*this)
	{
		m_vType[v] = m_pGraphAttributes->type(original(v));
		if (m_pGraphAttributes->isAssociationClass(original(v))) {
			OGDF_ASSERT(v->degree() == 1)
			edge e = v->firstAdj()->theEdge();
			setAssClass(e);
		}
	}
}//initCC


//inserts Boundary for a given group of nodes
//can e.g. be used to split clique replacements from the rest of the graph
//precondition: is only applicable for planar embedded subgraphs
//
//void PlanRep::insertBoundary(List<node> &group)
//{
//TODO
//Difficulty: efficiently find the outgoing edges
//}
//special version for stars
//works on copy nodes
//precondition: center is the center of a star-like subgraph
//induced by the neighbours of center, subgraph has connection
//to the rest of the graph
//keeps the given embedding
void PlanRep::insertBoundary(node centerOrig, adjEntry& adjExternal)//, CombinatorialEmbedding &E)
{
	//we insert edges to represent the boundary
	//by splitting the outgoing edges and connecting the
	//split nodes
	node center = copy(centerOrig);
	OGDF_ASSERT(center)

	if (center->degree() < 1) return;

	OGDF_ASSERT(original(center))

	//---------------------------
	//retrieve the outgoing edges
	//we run over all nodes adjacent to center and add their
	//adjacent edges
	SList<adjEntry> outAdj;

	adjEntry adj;
	forall_adj(adj, center)
	{
		//--------------------------------------------------------------
		//check if external face was saved over adjEntry on center edges

		//we want to stay in the same (external) face, next adjEntry
		//may get split later on, succ(succ) can never be within this clique
		//(and split) because all clique node - clique node connections are deleted
		//IFF the target node is connnected to some non-clique part of the graph
		//the search in this case would loop if there is no connection to the rest of
		//the graph
		if (adjExternal == adj) //outgoing
		{
			if (adj->twinNode()->degree() == 1)
			{
				do {
					adjExternal = adjExternal->faceCycleSucc();
				} while ( (adjExternal->theNode() == center) ||
					(adjExternal->twinNode() == center));
			}//if degree 1
			else
				adjExternal = adjExternal->faceCycleSucc()->faceCycleSucc();
		}
		if  (adjExternal == adj->twin()) //incoming
		{
			if (adj->twinNode()->degree() == 1)
			{
				do {
					adjExternal = adjExternal->faceCycleSucc();
				} while ( (adjExternal->theNode() == center) ||
					(adjExternal->twinNode() == center));
			}//if degree 1
			else
				adjExternal = adjExternal->faceCyclePred()->faceCyclePred();
		}
		adjEntry stopper = adj->twin();
		adjEntry runner = stopper->cyclicSucc();
		while (runner != stopper)
		{
			outAdj.pushBack(runner);
			runner = runner->cyclicSucc();
		}//while

	}//foralladj of center

	//we do not insert a boundary if the subgraph is not
	//connected to the rest of the graph
	if (outAdj.empty()) return;

	//---------------------------------------
	//now split the edges and save adjEntries
	//we maintain two lists of adjentries
	List<adjEntry> targetEntries, sourceEntries;

	//we need to find out if edge is outgoing
	bool isOut = false;
	SListIterator<adjEntry> it = outAdj.begin();
	while (it.valid())
	{
		//outgoing edges of clique nodes
		adjEntry splitAdj = (*it);

		edge splitEdge = splitAdj->theEdge();

		isOut = (splitAdj->theNode() == splitEdge->source());

		//-------------------------------------------------------
		//check if external face was saved over edges to be split
		bool splitOuter = false;
		bool splitInner = false;
		if (adjExternal == splitAdj)
		{
			splitOuter = true;
			//adjExternal = adjExternal->faceCycleSucc();
		}
		if (adjExternal == splitAdj->twin())
		{
			splitInner = true;
			//adjExternal = adjExternal->faceCyclePred();
		}


		//combinatorial version
		//edge newEdge = E.split(splitEdge);
		//simple version
		edge newEdge = split(splitEdge);
		setCrossingType(newEdge->source());

		//store the adjEntries depending on in/out direction
		if (isOut)
		{
			//splitresults "upper" edge to old target node is newEdge!
			sourceEntries.pushBack(newEdge->adjSource());
			targetEntries.pushBack(splitEdge->adjTarget());
			if (splitOuter) adjExternal = newEdge->adjSource();
			if (splitInner) adjExternal = newEdge->adjTarget();
		}//if outgoing
		else
		{
			sourceEntries.pushBack(splitEdge->adjTarget());
			targetEntries.pushBack(newEdge->adjSource());
			if (splitOuter) adjExternal = splitEdge->adjTarget();
			if (splitInner) adjExternal = splitEdge->adjSource();
		}//else outgoing

		//go on with next edge
		it++;

	}//while outedges


	//we need pairs of adjEntries
	OGDF_ASSERT(targetEntries.size() == sourceEntries.size());
	//now flip first target entry to front
	//should be nonempty
	adjEntry flipper = targetEntries.popFrontRet();
	targetEntries.pushBack(flipper);

	edge e;

	//connect the new nodes to form the boundary
	while (!targetEntries.empty())
	{
		//combinatorial version
		//edge e = E.splitFace(sourceEntries.popFrontRet(), targetEntries.popFrontRet());
		//simple version
		e = newEdge(sourceEntries.popFrontRet(), targetEntries.popFrontRet());
		this->typeOf(e) = Graph::association;
		setCliqueBoundary(e);
	}

	//keep it simple: just assign the last adjEntry to boundaryAdj
	//we have to save at the original, the copy may be replaced
	OGDF_ASSERT(m_boundaryAdj[original(center)] == 0)
	m_boundaryAdj[original(center)] = e->adjSource();

}//insertBoundary


//*****************************************************************************
//embedding and crossings

bool PlanRep::embed()
{
	return planarEmbed(*this);
}

void PlanRep::removeUnnecessaryCrossing(
	adjEntry adjA1,
	adjEntry adjA2,
	adjEntry adjB1,
	adjEntry adjB2)
{
	node v = adjA1->theNode();

	if(adjA1->theEdge()->source() == v)
		moveSource(adjA1->theEdge(), adjA2->twin(), before);
	else
		moveTarget(adjA1->theEdge(), adjA2->twin(), before);

	if(adjB1->theEdge()->source() == v)
		moveSource(adjB1->theEdge(), adjB2->twin(), before);
	else
		moveTarget(adjB1->theEdge(), adjB2->twin(), before);

	edge eOrigA = original(adjA1->theEdge());
	edge eOrigB = original(adjB1->theEdge());

	if (eOrigA != 0)
		m_eCopy[eOrigA].del(m_eIterator[adjA2->theEdge()]);
	if (eOrigB != 0)
		m_eCopy[eOrigB].del(m_eIterator[adjB2->theEdge()]);

	delEdge(adjB2->theEdge());
	delEdge(adjA2->theEdge());

	delNode(v);
}

void PlanRep::removePseudoCrossings()
{
	node v, vSucc;
	for(v = firstNode(); v != 0; v = vSucc)
	{
		vSucc = v->succ();

		if (typeOf(v) != PlanRep::dummy || v->degree() != 4)
			continue;

		adjEntry adj1 = v->firstAdj();
		adjEntry adj2 = adj1->succ();
		adjEntry adj3 = adj2->succ();
		adjEntry adj4 = adj3->succ();

		if(original(adj1->theEdge()) == original(adj2->theEdge()))
			removeUnnecessaryCrossing(adj1,adj2,adj3,adj4);
		else if (original(adj2->theEdge()) == original(adj3->theEdge()))
			removeUnnecessaryCrossing(adj2,adj3,adj4,adj1);
	}
}

void PlanRep::insertEdgePathEmbedded(
	edge eOrig,
	CombinatorialEmbedding &E,
	const SList<adjEntry> &crossedEdges)
{
	GraphCopy::insertEdgePathEmbedded(eOrig,E,crossedEdges);
	Graph::EdgeType edgeType = m_pGraphAttributes ?
		m_pGraphAttributes->type(eOrig) : Graph::association;

	long et = m_oriEdgeTypes[eOrig];

	ListConstIterator<edge> it;
	for(it = chain(eOrig).begin(); it.valid(); ++it)
	{
		m_eType[*it] = edgeType;
		m_edgeTypes[*it] = et;
		if (!original((*it)->target()))
		{
			OGDF_ASSERT((*it)->target()->degree() == 4)
				OGDF_ASSERT(it != chain(eOrig).rbegin())
			setCrossingType((*it)->target());
		}
	}
}

void PlanRep::insertEdgePath(
	edge eOrig,
	const SList<adjEntry> &crossedEdges)
{
	GraphCopy::insertEdgePath(eOrig,crossedEdges);

	//old types
	Graph::EdgeType edgeType = m_pGraphAttributes ?
		m_pGraphAttributes->type(eOrig) : Graph::association;

	//new types
	long et = m_oriEdgeTypes[eOrig];

	ListConstIterator<edge> it;
	for(it = chain(eOrig).begin(); it.valid(); ++it)
	{
		m_eType[*it] = edgeType;
		m_edgeTypes[*it] = et;
		if (!original((*it)->target()))
		{
			OGDF_ASSERT((*it)->target()->degree() == 4)
			setCrossingType((*it)->target());
		}
	}
}

edge PlanRep::insertCrossing(
	edge &crossingEdge,
	edge crossedEdge,
	bool topDown)
{
	EdgeType eTypi = m_eType[crossingEdge];
	EdgeType eTypd = m_eType[crossedEdge];
	edgeType eTypsi = m_edgeTypes[crossingEdge];
	edgeType eTypsd = m_edgeTypes[crossedEdge];

	edge newCopy = GraphCopy::insertCrossing(crossingEdge, crossedEdge, topDown);

	//Do not use original types, they may differ from the copy
	//type due to conflict resolution in preprocessing (expand crossings)
	m_eType[crossingEdge] = eTypi;
	m_eType[newCopy]      = eTypd;
	m_edgeTypes[crossingEdge] = eTypsi;
	m_edgeTypes[newCopy]      = eTypsd;

	setCrossingType(newCopy->source());
	OGDF_ASSERT(isCrossingType(newCopy->source()))

	//TODO: hier sollte man die NodeTypes setzen, d.h. crossing

	return newCopy;

}


void PlanRep::removeCrossing(node v)
{
	OGDF_ASSERT(v->degree() == 4)
	OGDF_ASSERT(isCrossingType(v))

	adjEntry a1 = v->firstAdj();
	adjEntry b1 = a1->cyclicSucc();
	adjEntry a2 = b1->cyclicSucc();
	adjEntry b2 = a2->cyclicSucc();

	removeUnnecessaryCrossing(a1, a2, b1, b2);


}//removeCrossing



void PlanRep::expand(bool lowDegreeExpand)
{
	node v;
	forall_nodes(v,*this)
	{

		// Replace vertices with high degree by cages and
		// replace degree 4 vertices with two generalizations
		// adjacent in the embedding list by a cage.
		if ((v->degree() > 4)  && (typeOf(v) != Graph::dummy) && !lowDegreeExpand)
		{
			edge e;

			//Set the type of the node v. It remains in the graph
			// as one of the nodes of the expanded face.
			typeOf(v) = Graph::highDegreeExpander;

			// Scan the list of edges of v to find the adjacent edges of v
			// according to the planar embedding. All except one edge
			// will get a new adjacent node
			SList<edge> adjEdges;
			{forall_adj_edges(e,v)
				adjEdges.pushBack(e);
			}

			//The first edge remains at v. remove it from the list.
			e = adjEdges.popFrontRet();

			// Create the list of high degree expanders
			// We need degree(v)-1 of them to construct a face.
			// and set expanded Node to v
			setExpandedNode(v, v);
			SListPure<node> expander;
			for (int i = 0; i < v->degree()-1; i++)
			{
				node u = newNode();
				typeOf(u) = Graph::highDegreeExpander;
				setExpandedNode(u, v);
				expander.pushBack(u);
			}

			// We move the target node of each ingoing generalization of v to a new
			// node stored in expander.
			// Note that, for each such edge e, the target node of the original
			// edge is then different from the original of the target node of e
			// (the latter is 0 because u is a new (dummy) node)
			SListConstIterator<edge> it;
			SListConstIterator<node> itn;

			NodeArray<adjEntry> ar(*this);

			itn = expander.begin();

			for (it = adjEdges.begin(); it.valid(); it++)
			{
				// Did we allocate enough dummy nodes?
				OGDF_ASSERT(itn.valid());

				if ((*it)->source() == v)
					moveSource(*it,*itn);
				else
					moveTarget(*it,*itn);
				ar[*itn] = (*itn)->firstAdj();
				itn++;
			}
			ar[v] = v->firstAdj();

			// Now introduce the circular list of new edges
			// forming the border of the merge face. Keep the embedding.
			adjEntry adjPrev = v->firstAdj();

//			cout <<endl << "INTRODUCING CIRCULAR EDGES" << endl;
			for (itn = expander.begin(); itn.valid(); itn++)
			{
//				cout << adjPrev << " " << (*itn)->firstAdj() << endl;
				e = Graph::newEdge(adjPrev,(*itn)->firstAdj());
				setExpansionEdge(e, 2);//can be removed if edgetypes work properly

				setExpansion(e);
				setAssociation(e);

				typeOf(e) = association; //???

				if (!expandAdj(v))
					expandAdj(v) = e->adjSource();
				adjPrev = (*itn)->firstAdj();
			}

			e = newEdge(adjPrev,v->lastAdj());

			typeOf(e) = association; //???
			setExpansionEdge(e, 2);//can be removed if edgetypes work properly
			setAssociation(e);

		}//highdegree

		// Replace all vertices with degree > 2 by cages.
		else if (v->degree() >= 2  && typeOf(v) != Graph::dummy &&
				 lowDegreeExpand)
		{
			edge e;

			//Set the type of the node v. It remains in the graph
			// as one of the nodes of the expanded face.
			typeOf(v) = Graph::lowDegreeExpander; //high??

			// Scan the list of edges of v to find the adjacent edges of v
			// according to the planar embedding. All except one edge
			// will get a new adjacent node
			SList<edge> adjEdges;
			{forall_adj_edges(e,v)
				adjEdges.pushBack(e);
			}

			//The first edge remains at v. remove it from the list.
			// Check if it is a generalization.
			e = adjEdges.popFrontRet();

			// Create the list of high degree expanders
			// We need degree(v)-1 of them to construct a face.
			// and set expanded Node to v
			setExpandedNode(v, v);
			SListPure<node> expander;
			for (int i = 0; i < v->degree()-1; i++)
			{
				node u = newNode();
				typeOf(u) = Graph::highDegreeExpander;
				setExpandedNode(u, v);
				expander.pushBack(u);
			}

			// We move the target node of each ingoing generalization of v to a new
			// node stored in expander.
			// Note that, for each such edge e, the target node of the original
			// edge is then different from the original of the target node of e
			// (the latter is 0 because u is a new (dummy) node)
			SListConstIterator<edge> it;
			SListConstIterator<node> itn;

			NodeArray<adjEntry> ar(*this);

			itn = expander.begin();

			for (it = adjEdges.begin(); it.valid(); it++)
			{
				// Did we allocate enough dummy nodes?
				OGDF_ASSERT(itn.valid());

				if ((*it)->source() == v)
					moveSource(*it,*itn);
				else
					moveTarget(*it,*itn);
				ar[*itn] = (*itn)->firstAdj();
				itn++;
			}
			ar[v] = v->firstAdj();

			// Now introduce the circular list of new edges
			// forming the border of the merge face. Keep the embedding.
			adjEntry adjPrev = v->firstAdj();

			for (itn = expander.begin(); itn.valid(); itn++)
			{
				e = newEdge(adjPrev,(*itn)->firstAdj());
				if (!expandAdj(v)) expandAdj(v) = e->adjSource();
				typeOf(e) = association; //???
				setExpansionEdge(e, 2);

				//new types
				setAssociation(e); //should be dummy type?
				setExpansion(e);

				adjPrev = (*itn)->firstAdj();
			}
			e = newEdge(adjPrev,v->lastAdj());
			typeOf(e) = association; //???
			setExpansionEdge(e, 2);
		}

	}//forallnodes

}//expand


void PlanRep::expandLowDegreeVertices(OrthoRep &OR)
{
	node v;
	forall_nodes(v,*this)
	{
		if (!(isVertex(v)) || expandAdj(v) != 0)
			continue;

		SList<edge> adjEdges;
		SListPure<Tuple2<node,int> > expander;

		node u = v;
		bool firstTime = true;

		setExpandedNode(v, v);

		adjEntry adj;
		forall_adj(adj,v) {
			adjEdges.pushBack(adj->theEdge());

			if(!firstTime)
				u = newNode();

			setExpandedNode(u, v);
			typeOf(u) = Graph::lowDegreeExpander;
			expander.pushBack(Tuple2<node,int>(u,OR.angle(adj)));
			firstTime = false;
		}

		SListConstIterator<edge> it;
		SListConstIterator<Tuple2<node,int> > itn;

		itn = expander.begin().succ();

		for (it = adjEdges.begin().succ(); it.valid(); ++it)
		{
			// Did we allocate enough dummy nodes?
			OGDF_ASSERT(itn.valid());

			if ((*it)->source() == v)
				moveSource(*it,(*itn).x1());
			else
				moveTarget(*it,(*itn).x1());
			++itn;
		}

		adjEntry adjPrev = v->firstAdj();
		itn = expander.begin();
		int nBends = (*itn).x2();

		edge e;
		for (++itn; itn.valid(); itn++)
		{
			e = newEdge(adjPrev,(*itn).x1()->firstAdj());

			OR.bend(e->adjSource()).set(convexBend,nBends);
			OR.bend(e->adjTarget()).set(reflexBend,nBends);
			OR.angle(adjPrev) = 1;
			OR.angle(e->adjSource()) = 2;
			OR.angle(e->adjTarget()) = 1;

			nBends = (*itn).x2();

			typeOf(e) = association; //???
			setExpansionEdge(e, 2);

			adjPrev = (*itn).x1()->firstAdj();
		}

		e = newEdge(adjPrev,v->lastAdj());
		typeOf(e) = association; //???
		setExpansionEdge(e, 2);

		expandAdj(v) = e->adjSource();

		OR.bend(e->adjSource()).set(convexBend,nBends);
		OR.bend(e->adjTarget()).set(reflexBend,nBends);
		OR.angle(adjPrev) = 1;
		OR.angle(e->adjSource()) = 2;
		OR.angle(e->adjTarget()) = 1;

	}
}//expandlowdegreevertices

void PlanRep::collapseVertices(const OrthoRep &OR, Layout &drawing)
{
	node v;
	forall_nodes(v,*this) {
		const OrthoRep::VertexInfoUML *vi = OR.cageInfo(v);

		if(vi == 0 ||
			(typeOf(v) != Graph::highDegreeExpander &&
			typeOf(v) != Graph::lowDegreeExpander))
			continue;

		node vOrig = original(v);
		OGDF_ASSERT(vOrig != 0);

		node vCenter = newNode();
		m_vOrig[vCenter] = vOrig;
		m_vCopy[vOrig] = vCenter;
		m_vOrig[v] = 0;

		node lowerLeft  = vi->m_corner[odNorth]->theNode();
		node lowerRight = vi->m_corner[odWest ]->theNode();
		node upperLeft  = vi->m_corner[odEast ]->theNode();
		drawing.x(vCenter) = 0.5*(drawing.x(lowerLeft)+drawing.x(lowerRight));
		drawing.y(vCenter) = 0.5*(drawing.y(lowerLeft)+drawing.y(upperLeft ));

		edge eOrig;
		forall_adj_edges(eOrig,vOrig) {
			if(eOrig->target() == vOrig) {
				node connect = m_eCopy[eOrig].back()->target();
				edge eNew = newEdge(connect,vCenter);
				m_eOrig[eNew] = eOrig;
				m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);

			} else {
				node connect = m_eCopy[eOrig].front()->source();
				edge eNew = newEdge(vCenter,connect);
				m_eOrig[eNew] = eOrig;
				m_eIterator[eNew] = m_eCopy[eOrig].pushFront(eNew);
			}
		}
	}
}

//-------------------------
//object types
//set type of eCopy according to type of eOrig
void PlanRep::setCopyType(edge eCopy, edge eOrig)
{
	OGDF_ASSERT(original(eCopy) == eOrig)
	m_eType[eCopy] = m_pGraphAttributes ? m_pGraphAttributes->type(eOrig) : Graph::association;
	if (eOrig)
	{
		switch (m_pGraphAttributes ? m_pGraphAttributes->type(eOrig) : Graph::association)
		{
			case Graph::generalization: setGeneralization(eCopy); break;
			case Graph::association: setAssociation(eCopy); break;
			case Graph::dependency: setDependency(eCopy); break;
			OGDF_NODEFAULT
		}//switch
	}//if original
}//setCopyType


void PlanRep::removeDeg1Nodes(Stack<Deg1RestoreInfo> &S, const NodeArray<bool> &mark)
{
	for(node v = firstNode(); v != 0; v = v->succ())
	{
		if(mark[v] || v->degree() == 0)
			continue;

		adjEntry adjRef;
		for(adjRef = v->firstAdj();
			adjRef != 0 && mark[adjRef->twinNode()];
			adjRef = adjRef->succ()) ;

		if(adjRef == 0) {
			// only marked nodes adjacent with v (need no reference entry)
			adjEntry adj;
			forall_adj(adj,v) {
				node x = adj->twinNode();
				S.push(Deg1RestoreInfo(m_eOrig[adj->theEdge()],m_vOrig[x],0));
				delCopy(x);
			}

		} else {
			adjEntry adj, adjNext, adjStart = adjRef;
			for(adj = adjRef->cyclicSucc(); adj != adjStart; adj = adjNext)
			{
				adjNext = adj->cyclicSucc();
				node x = adj->twinNode();
				if(mark[x]) {
					S.push(Deg1RestoreInfo(m_eOrig[adj->theEdge()],m_vOrig[x],adjRef));
					delCopy(x);
				} else
					adjRef = adj;
			}
		}
	}
}


void PlanRep::restoreDeg1Nodes(Stack<Deg1RestoreInfo> &S, List<node> &deg1s)
{
	while(!S.empty())
	{
		Deg1RestoreInfo info = S.pop();
		adjEntry adjRef = info.m_adjRef;
		node     vOrig  = info.m_deg1Original;
		edge     eOrig  = info.m_eOriginal;

		node v = newNode(vOrig);

		if(adjRef) {
			if(vOrig == eOrig->source())
				newEdge(eOrig, v, adjRef);
			else
				newEdge(eOrig, adjRef, v);
		} else {
			if(vOrig == eOrig->source())
				newEdge(eOrig);
			else
				newEdge(eOrig);
		}
		deg1s.pushBack(v);
	}
}


node PlanRep::newCopy(node v, Graph::NodeType vTyp)
{
	OGDF_ASSERT(m_vCopy[v] == 0)

	node u = newNode();
	m_vCopy[v] = u;
	m_vOrig[u] = v;
	//TODO:Typ?
	m_vType[u] = vTyp;

	return u;
}

//inserts copy for original edge eOrig after adAfter
edge PlanRep::newCopy(node v, adjEntry adAfter, edge eOrig)
{
	OGDF_ASSERT(eOrig->graphOf() == &(original()))
	OGDF_ASSERT(m_eCopy[eOrig].size() == 0)
	edge e;
	if (adAfter != 0)
		e = Graph::newEdge(v, adAfter);
	else
	{
		node w = copy(eOrig->opposite(original(v)));
		OGDF_ASSERT(w)
		e = Graph::newEdge(v, w);
	}//else
	m_eOrig[e] = eOrig;
	m_eIterator[e] = m_eCopy[eOrig].pushBack(e);
	//set type of copy
	if (m_pGraphAttributes != 0)
		setCopyType(e, eOrig);

	return e;
}
//inserts copy for original edge eOrig preserving the embedding
edge PlanRep::newCopy(node v, adjEntry adAfter, edge eOrig, CombinatorialEmbedding &E)
{
	OGDF_ASSERT(eOrig->graphOf() == &(original()))
	OGDF_ASSERT(m_eCopy[eOrig].size() == 0)

	edge e;
	//GraphCopy checks direction for us
	e = GraphCopy::newEdge(v, adAfter, eOrig, E);
	//set type of copy
	if (m_pGraphAttributes != 0)
		setCopyType(e, eOrig);

	return e;
}


edge PlanRep::split(edge e)
{
	bool cageBound = (m_expandedNode[e->source()] && m_expandedNode[e->target()])
		&& (m_expandedNode[e->source()] == m_expandedNode[e->target()]);
	node expNode = (cageBound ? m_expandedNode[e->source()] : 0);

	edge eNew = GraphCopy::split(e);
	m_eType[eNew] = m_eType[e];
	m_edgeTypes[eNew] = m_edgeTypes[e];
	m_expansionEdge[eNew] = m_expansionEdge[e];

	m_expandedNode[eNew->source()] = expNode;

	return eNew;
}


} // end namespace ogdf
