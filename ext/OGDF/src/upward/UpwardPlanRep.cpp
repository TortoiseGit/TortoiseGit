/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of PlanRep base class for planar rep.
 *
 * \author Hoi-Ming Wong
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
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/FaceSinkGraph.h>
#include <ogdf/basic/FaceSet.h>
#include <ogdf/basic/tuples.h>



namespace ogdf {


UpwardPlanRep::UpwardPlanRep(const CombinatorialEmbedding &Gamma) :
	GraphCopy(Gamma.getGraph()),
	isAugmented(false),
	t_hat(0),
	extFaceHandle(0),
	crossings(0)
{
	OGDF_ASSERT(Gamma.externalFace() != 0);
	OGDF_ASSERT(hasSingleSource(*this));
	OGDF_ASSERT(isSimple(*this));

	m_isSourceArc.init(*this, false);
	m_isSinkArc.init(*this, false);
	hasSingleSource(*this, s_hat);
	m_Gamma.init(*this);

	//compute the ext. face;
	adjEntry adj;
	node v = this->original(s_hat);
	adj = getAdjEntry(Gamma, v, Gamma.externalFace());
	adj = this->copy(adj->theEdge())->adjSource();
	m_Gamma.setExternalFace(m_Gamma.rightFace(adj));

	//outputFaces(Gamma);

	computeSinkSwitches();
}


UpwardPlanRep::UpwardPlanRep(const GraphCopy &GC, ogdf::adjEntry adj_ext) :
	GraphCopy(GC),
	isAugmented(false),
	t_hat(0),
	extFaceHandle(0),
	crossings(0)
{
	OGDF_ASSERT(adj_ext != 0);
	OGDF_ASSERT(hasSingleSource(*this));

	m_isSourceArc.init(*this, false);
	m_isSinkArc.init(*this, false);
	hasSingleSource(*this, s_hat);
	m_Gamma.init(*this);

	//compute the ext. face;
	node v = copy(GC.original(adj_ext->theNode()));
	extFaceHandle = copy(GC.original(adj_ext->theEdge()))->adjSource();
	if (extFaceHandle->theNode() != v)
		extFaceHandle = extFaceHandle->twin();
	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	adjEntry adj;
	forall_adj(adj, s_hat)
		m_isSourceArc[adj->theEdge()] = true;

	computeSinkSwitches();
}


//copy constructor
UpwardPlanRep::UpwardPlanRep(const UpwardPlanRep &UPR) :
	GraphCopy(),
	isAugmented(UPR.isAugmented),
	crossings(UPR.crossings)
{
	copyMe(UPR);
}


void UpwardPlanRep::copyMe(const UpwardPlanRep &UPR)
{
	NodeArray<node> vCopy;
	EdgeArray<edge> eCopy;

	Graph::construct(UPR,vCopy,eCopy);

	// initGC
	m_pGraph = UPR.m_pGraph;

	m_vOrig.init(*this,0); m_eOrig.init(*this,0);
	m_vCopy.init(*m_pGraph,0); m_eCopy.init(*m_pGraph);
	m_eIterator.init(*this,0);

	node v, w;
	forall_nodes(v,UPR)
		m_vOrig[vCopy[v]] = UPR.m_vOrig[v];

	edge e;
	forall_edges(e,UPR)
		m_eOrig[eCopy[e]] = UPR.m_eOrig[e];

	forall_nodes(v,*this)
		if ((w = m_vOrig[v]) != 0) m_vCopy[w] = v;

	forall_edges(e,*m_pGraph) {
		ListConstIterator<edge> it;
		for (it = UPR.m_eCopy[e].begin(); it.valid(); ++it)
			m_eIterator[eCopy[*it]] = m_eCopy[e].pushBack(eCopy[*it]);
	}

	//GraphCopy::initGC(UPR,vCopy,eCopy);
	m_Gamma.init(*this);
	m_isSinkArc.init(*this, false);
	m_isSourceArc.init(*this, false);

	if (UPR.numberOfNodes() == 0)
		return;

	s_hat = vCopy[UPR.getSuperSource()];
	if (UPR.augmented())
		t_hat = vCopy[UPR.getSuperSink()];

	OGDF_ASSERT(UPR.extFaceHandle != 0);

	e = eCopy[UPR.extFaceHandle->theEdge()];
	v = vCopy[UPR.extFaceHandle->theNode()];
	if (e->adjSource()->theNode() == v)
		extFaceHandle = e->adjSource();
	else
		extFaceHandle = e->adjTarget();

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	forall_edges(e, UPR) {
		edge a = eCopy[e];
		if (UPR.isSinkArc(e))
			m_isSinkArc[a] = true;
		if (UPR.isSourceArc(e))
			m_isSourceArc[a] = true;
	}

	computeSinkSwitches();
}



UpwardPlanRep & UpwardPlanRep::operator =(const UpwardPlanRep &cp)
{
	clear();
	createEmpty(cp.original());
	isAugmented = cp.isAugmented;
	extFaceHandle = 0;
	crossings = cp.crossings;
	copyMe(cp);
	return *this;
}


void UpwardPlanRep::augment()
{
	if (isAugmented)
		return;

	OGDF_ASSERT(hasSingleSource(*this));

	List< Tuple2<adjEntry, adjEntry> > l;
	List<adjEntry> switches;

	hasSingleSource(*this, s_hat);

	adjEntry adj;
	forall_adj(adj, s_hat)
		m_isSourceArc[adj->theEdge()] = true;

	FaceSinkGraph fsg(m_Gamma, s_hat);
	List<adjEntry> dummyList;
	FaceArray< List<adjEntry> > sinkSwitches(m_Gamma, dummyList);
	fsg.sinkSwitches(sinkSwitches);
	m_sinkSwitchOf.init(*this, 0);

	face f;
	forall_faces(f, m_Gamma) {
		adjEntry adj_top;
		switches = sinkSwitches[f];
		if (switches.empty() || f == m_Gamma.externalFace())
			continue;
		else
			adj_top = switches.popFrontRet(); // first switch in the list is a top sink switch

		while (!switches.empty()) {
			adjEntry adj = switches.popFrontRet();
			Tuple2<adjEntry, adjEntry> pair(adj, adj_top);
			l.pushBack(pair);
		}
	}
	// construct sink arcs
	// for the ext. face
	extFaceHandle = getAdjEntry(m_Gamma, s_hat, m_Gamma.externalFace());
	node t = this->newNode();
	switches = sinkSwitches[m_Gamma.externalFace()];

	OGDF_ASSERT(!switches.empty());

	while (!switches.empty()) {
		adjEntry adj = switches.popFrontRet();
		edge e_new;
		if (t->degree() == 0) {
			e_new = m_Gamma.splitFace(adj, t);
		}
		else {
			adjEntry adjTgt = getAdjEntry(m_Gamma, t, m_Gamma.rightFace(adj));
			e_new = m_Gamma.splitFace(adj, adjTgt);
		}
		m_isSinkArc[e_new] = true;
		m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));
	}

	/*
	* set ext. face handle
	* we add a additional node t_hat and an addtional edge e=(t, t_hat)
	* e will never been crossed. we use e as the ext. face handle
	*/
	t_hat = this->newNode();
	adjEntry adjSource = getAdjEntry(m_Gamma, t, m_Gamma.externalFace());
	extFaceHandle = m_Gamma.splitFace(adjSource, t_hat)->adjTarget();
	m_isSinkArc[extFaceHandle->theEdge()] = true; // not really a sink arc !! TODO??

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	//for int. faces
	while (!l.empty()) {
		Tuple2<adjEntry, adjEntry> pair = l.popFrontRet();


		edge e_new;
		if (pair.x2()->theNode()->degree() == 0 ) {
			e_new = m_Gamma.splitFace(pair.x1(), pair.x2()->theNode());
		}
		else {
			adjEntry adjTgt = getAdjEntry(m_Gamma, pair.x2()->theNode(), m_Gamma.rightFace(pair.x1()));
			e_new = m_Gamma.splitFace(pair.x1(), adjTgt);
		}
		m_isSinkArc[e_new] = true;
	}

	isAugmented = true;

	OGDF_ASSERT(isSimple(*this));

	computeSinkSwitches();
}


void UpwardPlanRep::removeSinkArcs(SList<adjEntry> &crossedEdges) {

	if (crossedEdges.size() == 2)
		return;


	SListIterator<adjEntry> itPred = crossedEdges.begin(), itLast = crossedEdges.rbegin(), it;
	for(it = itPred.succ(); it != itLast; ++it)	{
		adjEntry adj = *it;
		if (m_isSinkArc[adj->theEdge()]) {
			m_Gamma.joinFaces(adj->theEdge());
			crossedEdges.delSucc(itPred);
			it = itPred;
			continue;
		}
		itPred = it;
	}
	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));
}


void UpwardPlanRep::insertEdgePathEmbedded(edge eOrig, SList<adjEntry> crossedEdges, EdgeArray<int> &costOrig)
{
	removeSinkArcs(crossedEdges);

	//case the copy v of eOrig->source() is a sink switch
	//we muss remove the sink arcs incident to v, since after inserting eOrig, v is not a sink witch
	node v =  crossedEdges.front()->theNode();
	List<edge> outEdges;
	if (v->outdeg() == 1)
		this->outEdges(v, outEdges); // we delete this edges later

	m_eCopy[eOrig].clear();

	adjEntry adjSrc, adjTgt;
	SListConstIterator<adjEntry> it = crossedEdges.begin();
	SListConstIterator<adjEntry> itLast = crossedEdges.rbegin();

	// iterate over all adjacency entries in crossedEdges except for first
	// and last
	adjSrc = *it;
	List<adjEntry> dirtyList; // left and right face of the element of this list are modified
	for(++it; it != itLast; ++it)
	{
		adjEntry adj = *it;

		bool isASourceArc = false, isASinkArc = false;
		if (m_isSinkArc[adj->theEdge()])
			isASinkArc = true;
		if (m_isSourceArc[adj->theEdge()])
			isASourceArc = true;

		int c = 0;
		if (original(adj->theEdge()) != 0)
			c = costOrig[original(adj->theEdge())];

		// split edge
		node u = m_Gamma.split(adj->theEdge())->source();
		if (!m_isSinkArc[adj->theEdge()] && !m_isSourceArc[adj->theEdge()])
			crossings = crossings + c; // crossing sink/source arcs cost nothing

		// determine target adjacency entry and source adjacency entry
		// in the next iteration step
		adjTgt = u->firstAdj();
		adjEntry adjSrcNext = adjTgt->succ();

		if (adjTgt != adj->twin())
			swap(adjTgt,adjSrcNext);


		edge e_split = adjTgt->theEdge(); // the new split edge
		if (e_split->source() != u)
			e_split = adjSrcNext->theEdge();

		if (isASinkArc)
			m_isSinkArc[e_split] = true;
		if (isASourceArc)
			m_isSourceArc[e_split] = true;

		// insert a new edge into the face
		edge eNew = m_Gamma.splitFace(adjSrc,adjTgt);
		m_eIterator[eNew] = GraphCopy::m_eCopy[eOrig].pushBack(eNew);
		m_eOrig[eNew] = eOrig;
		dirtyList.pushBack(eNew->adjSource());

		adjSrc = adjSrcNext;
	}

	// insert last edge
	edge eNew = m_Gamma.splitFace(adjSrc,*it);
	m_eIterator[eNew] = m_eCopy[eOrig].pushBack(eNew);
	m_eOrig[eNew] = eOrig;
	dirtyList.pushBack(eNew->adjSource());

	// remove the sink arc incident to v
	if(!outEdges.empty()) {
		edge e = outEdges.popFrontRet();
		if (m_isSinkArc[e])
			m_Gamma.joinFaces(e);
	}

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));

	//computeSinkSwitches();
	FaceSinkGraph fsg(m_Gamma, s_hat);
	List<adjEntry> dummyList;
	FaceArray< List<adjEntry> > sinkSwitches(m_Gamma, dummyList);
	fsg.sinkSwitches(sinkSwitches);

	//construct sinkArc for the dirty faces
	List< Tuple2<adjEntry, adjEntry> > l;
	forall_listiterators(adjEntry, it, dirtyList) {
		face fLeft = m_Gamma.leftFace(*it);
		face fRight = m_Gamma.rightFace(*it);
		List<adjEntry> switches = sinkSwitches[fLeft];

		OGDF_ASSERT(!switches.empty());

		constructSinkArcs(fLeft, switches.front()->theNode());

		OGDF_ASSERT(!switches.empty());

		switches = sinkSwitches[fRight];
		constructSinkArcs(fRight, switches.front()->theNode());
	}

	m_Gamma.setExternalFace(m_Gamma.rightFace(extFaceHandle));
	computeSinkSwitches();
}



void UpwardPlanRep::constructSinkArcs(face f, node t)
{
	List<adjEntry> srcList;
	adjEntry adjTgt;

	if (f != m_Gamma.externalFace()) {
		adjEntry adj;
		forall_face_adj(adj, f) {
			node v = adj->theNode();
			if (v == adj->theEdge()->target() && adj->faceCyclePred()->theEdge()->target() == v) {
				if (v != t)
					srcList.pushBack(adj);
				else
					adjTgt = adj; // top-sink-switch of f
			}
		}
		// contruct the sink arcs
		while(!srcList.empty()) {
			adjEntry adjSrc = srcList.popFrontRet();
			edge eNew;
			if (t->degree() == 0)
				eNew = m_Gamma.splitFace(adjSrc, t);
			else {
				adjEntry adjTgt = getAdjEntry(m_Gamma, t, m_Gamma.rightFace(adjSrc));
				eNew = m_Gamma.splitFace(adjSrc, adjTgt);
			}
			m_isSinkArc[eNew] = true;
		}
	}
	else {
		adjEntry adj;
		forall_face_adj(adj, f) {
			node v = adj->theNode();

			OGDF_ASSERT(s_hat != 0);

			if (v->outdeg() == 0 && v != t_hat)
				srcList.pushBack(adj);
		}

		// contruct the sink arcs
		while(!srcList.empty()) {
			adjEntry adjSrc = srcList.popFrontRet();
			edge eNew;
			if (adjSrc->theNode() == adjSrc->theEdge()->source()) // on the right face part of the ext. face
				adjTgt = extFaceHandle;
			else
				adjTgt = extFaceHandle->cyclicPred(); // on the left face part

			eNew = m_Gamma.splitFace(adjSrc, adjTgt);
			m_isSinkArc[eNew] = true;
		}

	}
}


void UpwardPlanRep::computeSinkSwitches()
{
	OGDF_ASSERT(m_Gamma.externalFace() != 0);

	if (s_hat == 0)
		hasSingleSource(*this, s_hat);
	FaceSinkGraph fsg(m_Gamma, s_hat);
	List<adjEntry> dummyList;
	FaceArray< List<adjEntry> > sinkSwitches(m_Gamma, dummyList);
	fsg.sinkSwitches(sinkSwitches);
	m_sinkSwitchOf.init(*this, 0);

	face f;
	forall_faces(f, m_Gamma) {
		List<adjEntry> switches = sinkSwitches[f];
		ListIterator<adjEntry> it = switches.begin();
		for (it = it.succ(); it.valid(); it++) {
			m_sinkSwitchOf[(*it)->theNode()] = (*it);
		}
	}
}


void UpwardPlanRep::initMe()
{
	m_Gamma.init(*this);
	isAugmented = false;

	FaceSinkGraph fsg(m_Gamma, s_hat);
	SList<face> extFaces;
	fsg.possibleExternalFaces(extFaces);

	OGDF_ASSERT(!extFaces.empty());

	face f_ext = 0;
	forall_slistiterators(face, it, extFaces) {
		if (f_ext == 0)
			f_ext = *it;
		else {
			if (f_ext->size() < (*it)->size())
				f_ext = *it;
		}
	}
	m_Gamma.setExternalFace(f_ext);
	adjEntry adj;
	forall_adj(adj, s_hat) {
		if (m_Gamma.rightFace(adj) == m_Gamma.externalFace()) {
			extFaceHandle = adj;
			break;
		}
	}

	computeSinkSwitches();
}


}
