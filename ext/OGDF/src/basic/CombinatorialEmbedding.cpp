/*
 * $Revision: 2555 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 12:12:10 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class CombinatorialEmbedding
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


#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>


#define MIN_FACE_TABLE_SIZE (1 << 4)


namespace ogdf {

ConstCombinatorialEmbedding::ConstCombinatorialEmbedding()
{
	m_cpGraph = 0;
	m_externalFace = 0;
	m_nFaces = m_faceIdCount = 0;
	m_faceArrayTableSize = MIN_FACE_TABLE_SIZE;
}


ConstCombinatorialEmbedding::ConstCombinatorialEmbedding(const Graph &G) :
	m_cpGraph(&G), m_rightFace(G,0)
{
	computeFaces();
}

ConstCombinatorialEmbedding::ConstCombinatorialEmbedding(
	const ConstCombinatorialEmbedding &C)
	: m_cpGraph(C.m_cpGraph), m_rightFace(*C.m_cpGraph,0)
{
	computeFaces();

	if(C.m_externalFace == 0)
		m_externalFace = 0;
	else
		m_externalFace = m_rightFace[C.m_externalFace->firstAdj()];
}

ConstCombinatorialEmbedding &ConstCombinatorialEmbedding::operator=(
	const ConstCombinatorialEmbedding &C)
{
	init(*C.m_cpGraph);

	if(C.m_externalFace == 0)
		m_externalFace = 0;
	else
		m_externalFace = m_rightFace[C.m_externalFace->firstAdj()];

	return *this;
}

void ConstCombinatorialEmbedding::init(const Graph &G)
{
	m_cpGraph = &G;
	m_rightFace.init(G,0);
	computeFaces();
}


void ConstCombinatorialEmbedding::init()
{
	m_cpGraph = 0;
	m_externalFace = 0;
	m_nFaces = m_faceIdCount = 0;
	m_faceArrayTableSize = MIN_FACE_TABLE_SIZE;
	m_rightFace.init();
	m_faces.clear();

	reinitArrays();
}


void ConstCombinatorialEmbedding::computeFaces()
{
	m_externalFace = 0; // no longer valid!
	m_faceIdCount = 0;
	m_faces.clear();

	m_rightFace.fill(0);

	node v;
	forall_nodes(v,*m_cpGraph) {
		adjEntry adj;
		forall_adj(adj,v) {
			if (m_rightFace[adj]) continue;

#ifdef OGDF_DEBUG
			face f = OGDF_NEW FaceElement(this,adj,m_faceIdCount++);
#else
			face f = OGDF_NEW FaceElement(adj,m_faceIdCount++);
#endif

			m_faces.pushBack(f);

			adjEntry adj2 = adj;
			do {
				m_rightFace[adj2] = f;
				f->m_size++;
				adj2 = adj2->faceCycleSucc();
			} while (adj2 != adj);
		}
	}

	m_nFaces = m_faceIdCount;
	m_faceArrayTableSize = Graph::nextPower2(MIN_FACE_TABLE_SIZE,m_faceIdCount);
	reinitArrays();

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


face ConstCombinatorialEmbedding::createFaceElement(adjEntry adjFirst)
{
	if (m_faceIdCount == m_faceArrayTableSize) {
		m_faceArrayTableSize <<= 1;
		for(ListIterator<FaceArrayBase*> it = m_regFaceArrays.begin();
			it.valid(); ++it)
		{
			(*it)->enlargeTable(m_faceArrayTableSize);
		}
	}

#ifdef OGDF_DEBUG
	face f = OGDF_NEW FaceElement(this,adjFirst,m_faceIdCount++);
#else
	face f = OGDF_NEW FaceElement(adjFirst,m_faceIdCount++);
#endif

	m_faces.pushBack(f);
	m_nFaces++;

	return f;
}


edge CombinatorialEmbedding::split(edge e)
{
	face f1 = m_rightFace[e->adjSource()];
	face f2 = m_rightFace[e->adjTarget()];

	edge e2 = m_pGraph->split(e);

	m_rightFace[e->adjSource()] = m_rightFace[e2->adjSource()] = f1;
	f1->m_size++;
	m_rightFace[e->adjTarget()] = m_rightFace[e2->adjTarget()] = f2;
	f2->m_size++;

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return e2;
}


void CombinatorialEmbedding::unsplit(edge eIn, edge eOut)
{
	face f1 = m_rightFace[eIn->adjSource()];
	face f2 = m_rightFace[eIn->adjTarget()];

	--f1->m_size;
	--f2->m_size;

	if (f1->m_adjFirst == eOut->adjSource())
		f1->m_adjFirst = eIn->adjSource();

	if (f2->m_adjFirst == eIn->adjTarget())
		f2->m_adjFirst = eOut->adjTarget();

	m_pGraph->unsplit(eIn,eOut);
}


node CombinatorialEmbedding::splitNode(adjEntry adjStartLeft, adjEntry adjStartRight)
{
	face fL = leftFace(adjStartLeft);
	face fR = leftFace(adjStartRight);

	node u = m_pGraph->splitNode(adjStartLeft,adjStartRight);

	adjEntry adj = adjStartLeft->cyclicPred();

	m_rightFace[adj] = fL;
	++fL->m_size;
	m_rightFace[adj->twin()] = fR;
	++fR->m_size;

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return u;
}


node CombinatorialEmbedding::contract(edge e)
{
	// Since we remove face e, we also remove adjSrc and adjTgt.
	// We make sure that node of them is stored as first adjacency
	// entry of a face.
	adjEntry adjSrc = e->adjSource();
	adjEntry adjTgt = e->adjTarget();

	face fSrc = m_rightFace[adjSrc];
	face fTgt = m_rightFace[adjTgt];

	if(fSrc->m_adjFirst == adjSrc) {
		adjEntry adj = adjSrc->faceCycleSucc();
		fSrc->m_adjFirst = (adj != adjTgt) ? adj : adj->faceCycleSucc();
	}

	if(fTgt->m_adjFirst == adjTgt) {
		adjEntry adj = adjTgt->faceCycleSucc();
		fTgt->m_adjFirst = (adj != adjSrc) ? adj : adj->faceCycleSucc();
	}

	node v = m_pGraph->contract(e);
	--fSrc->m_size;
	--fTgt->m_size;

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return v;
}


edge CombinatorialEmbedding::splitFace(adjEntry adjSrc, adjEntry adjTgt)
{
	OGDF_ASSERT(m_rightFace[adjSrc] == m_rightFace[adjTgt])
	OGDF_ASSERT(adjSrc != adjTgt)

	edge e = m_pGraph->newEdge(adjSrc,adjTgt);

	face f1 = m_rightFace[adjTgt];
	face f2 = createFaceElement(adjSrc);

	adjEntry adj = adjSrc;
	do {
		m_rightFace[adj] = f2;
		f2->m_size++;
		adj = adj->faceCycleSucc();
	} while (adj != adjSrc);

	f1->m_adjFirst = adjTgt;
	f1->m_size += (2 - f2->m_size);
	m_rightFace[e->adjSource()] = f1;

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return e;
}

//special version of the above function doing a pushback of the new edge
//on the adjacency list of v making it possible to insert new degree 0
//nodes into a face
edge CombinatorialEmbedding::splitFace(node v, adjEntry adjTgt)
{
	adjEntry adjSrc = v->lastAdj();
	edge e = 0;
	bool degZ = v->degree() == 0;
	if (degZ) {
		e = m_pGraph->newEdge(v, adjTgt);
	}
	else
	{
		OGDF_ASSERT(m_rightFace[adjSrc] == m_rightFace[adjTgt])
		OGDF_ASSERT(adjSrc != adjTgt)
		e = m_pGraph->newEdge(adjSrc,adjTgt); //could use ne(v,ad) here, too
	}

	face f1 = m_rightFace[adjTgt];
	//if v already had an adjacent edge, we split the face in two faces
	int subSize = 0;
	if (!degZ)
	{
		face f2 = createFaceElement(adjSrc);

		adjEntry adj = adjSrc;
		do
		{
			m_rightFace[adj] = f2;
			f2->m_size++;
			adj = adj->faceCycleSucc();
		} while (adj != adjSrc);
		subSize = f2->m_size;
	}//if not zero degree
	else
	{
		m_rightFace[e->adjTarget()] = f1;
	}

	f1->m_adjFirst = adjTgt;
	f1->m_size += (2 - subSize);
	m_rightFace[e->adjSource()] = f1;

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return e;
}//splitface
//--
//-----------------
//incremental stuff
//special version of the above function doing a pushback of the new edge
//on the adjacency list of v making it possible to insert new degree 0
//nodes into a face, end node v
edge CombinatorialEmbedding::splitFace(adjEntry adjSrc, node v)
{
	adjEntry adjTgt = v->lastAdj();
	edge e = 0;
	bool degZ = v->degree() == 0;
	if (degZ)
	{
		e = m_pGraph->newEdge(adjSrc, v);
	}
	else
	{
		OGDF_ASSERT(m_rightFace[adjSrc] == m_rightFace[adjTgt])
		OGDF_ASSERT(adjSrc != adjTgt)
		e = m_pGraph->newEdge(adjSrc, adjTgt); //could use ne(v,ad) here, too
	}

	face f1 = m_rightFace[adjSrc];
	//if v already had an adjacent edge, we split the face in two faces
	int subSize = 0;
	if (!degZ)
	{
		face f2 = createFaceElement(adjTgt);

		adjEntry adj = adjTgt;
		do
		{
			m_rightFace[adj] = f2;
			f2->m_size++;
			adj = adj->faceCycleSucc();
		} while (adj != adjTgt);
		subSize = f2->m_size;
	}//if not zero degree
	else
	{
		m_rightFace[e->adjSource()] = f1;
	}

	f1->m_adjFirst = adjSrc;
	f1->m_size += (2 - subSize);
	m_rightFace[e->adjTarget()] = f1;

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return e;
}//splitface

//update face information after inserting a merger ith edge e in a copy graph
void CombinatorialEmbedding::updateMerger(edge e, face fRight, face fLeft)
{
	//two cases: a single face/two faces
	fRight->m_size++;
	fLeft->m_size++;
	m_rightFace[e->adjSource()] = fRight;
	m_rightFace[e->adjTarget()] = fLeft;
	//check for first adjacency entry
	if (fRight != fLeft)
	{
		fRight->m_adjFirst = e->adjSource();
		fLeft->m_adjFirst  = e->adjTarget();
	}//if
}//updateMerger

//--


face CombinatorialEmbedding::joinFaces(edge e)
{
	OGDF_ASSERT(e->graphOf() == m_pGraph);

	// get the two faces adjacent to e
	face f1 = m_rightFace[e->adjSource()];
	face f2 = m_rightFace[e->adjTarget()];

	OGDF_ASSERT(f1 != f2);

	// we will reuse the largest face and delete the other one
	if (f2->m_size > f1->m_size)
		swap(f1,f2);

	// the size of the joined face is the sum of the sizes of the two faces
	// f1 and f2 minus the two adjacency entries of e
	f1->m_size += f2->m_size - 2;

	// If the stored (first) adjacency entry of f1 belongs to e, we must set
	// it to the next entry in the face, because we will remove it by deleting
	// edge e
	if (f1->m_adjFirst->theEdge() == e)
		f1->m_adjFirst = f1->m_adjFirst->faceCycleSucc();

	// each adjacency entry in f2 belongs now to f1
	adjEntry adj1 = f2->firstAdj(), adj = adj1;
	do {
		m_rightFace[adj] = f1;
	} while((adj = adj->faceCycleSucc()) != adj1);

	m_pGraph->delEdge(e);

	m_faces.del(f2);
	--m_nFaces;

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());

	return f1;
}


void CombinatorialEmbedding::reverseEdge(edge e)
{
	// reverse edge in graph
	m_pGraph->reverseEdge(e);

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


void CombinatorialEmbedding::moveBridge(adjEntry adjBridge, adjEntry adjBefore)
{
	OGDF_ASSERT(m_rightFace[adjBridge] == m_rightFace[adjBridge->twin()]);
	OGDF_ASSERT(m_rightFace[adjBridge] != m_rightFace[adjBefore]);

	face fOld = m_rightFace[adjBridge];
	face fNew = m_rightFace[adjBefore];

	adjEntry adjCand = adjBridge->faceCycleSucc();

	int sz = 0;
	adjEntry adj;
	for(adj = adjBridge->twin(); adj != adjCand; adj = adj->faceCycleSucc()) {
		if(fOld->m_adjFirst == adj)
			fOld->m_adjFirst = adjCand;
		m_rightFace[adj] = fNew;
		++sz;
	}

	fOld->m_size -= sz;
	fNew->m_size += sz;

	edge e = adjBridge->theEdge();
	if(e->source() == adjBridge->twinNode())
		m_pGraph->moveSource(e, adjBefore, after);
	else
		m_pGraph->moveTarget(e, adjBefore, after);

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


void CombinatorialEmbedding::removeDeg1(node v)
{
	OGDF_ASSERT(v->degree() == 1);

	adjEntry adj = v->firstAdj();
	face     f   = m_rightFace[adj];

	if(f->m_adjFirst == adj || f->m_adjFirst == adj->twin())
		f->m_adjFirst = adj->faceCycleSucc();
	f->m_size -= 2;

	m_pGraph->delNode(v);

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


void CombinatorialEmbedding::clear()
{
	m_pGraph->clear();

	m_faces.clear();

	m_nFaces = m_faceIdCount = 0;
	m_faceArrayTableSize = MIN_FACE_TABLE_SIZE;
	m_externalFace = 0;

	reinitArrays();

	OGDF_ASSERT_IF(dlConsistencyChecks, consistencyCheck());
}


face ConstCombinatorialEmbedding::chooseFace() const
{
	if (m_nFaces == 0) return 0;

	int k = ogdf::randomNumber(0,m_nFaces-1);
	face f = firstFace();
	while(k--) f = f->succ();

	return f;
}


face ConstCombinatorialEmbedding::maximalFace() const
{
	if (m_nFaces == 0) return 0;

	face fMax = firstFace();
	int max = fMax->size();

	for(face f = fMax->succ(); f != 0; f = f->succ())
	{
		if (f->size() > max) {
			max = f->size();
			fMax = f;
		}
	}

	return fMax;
}


ListIterator<FaceArrayBase*> ConstCombinatorialEmbedding::
	registerArray(FaceArrayBase *pFaceArray) const
{
	return m_regFaceArrays.pushBack(pFaceArray);
}


void ConstCombinatorialEmbedding::unregisterArray(
	ListIterator<FaceArrayBase*> it) const
{
	m_regFaceArrays.del(it);
}


void ConstCombinatorialEmbedding::reinitArrays()
{
	ListIterator<FaceArrayBase*> it = m_regFaceArrays.begin();
	for(; it.valid(); ++it)
		(*it)->reinit(m_faceArrayTableSize);
}


bool ConstCombinatorialEmbedding::consistencyCheck()
{
	if (m_cpGraph->consistencyCheck() == false)
		return false;

	if(m_cpGraph->representsCombEmbedding() == false)
		return false;

	AdjEntryArray<bool> visited(*m_cpGraph,false);
	int nF = 0;

	face f;
	forall_faces(f,*this) {
#ifdef OGDF_DEBUG
		if (f->embeddingOf() != this)
			return false;
#endif

		nF++;

		adjEntry adj = f->firstAdj(), adj2 = adj;
		int sz = 0;
		do {
			sz++;
			if (visited[adj2] == true)
				return false;

			visited[adj2] = true;

			if (m_rightFace[adj2] != f)
				return false;

			adj2 = adj2->faceCycleSucc();
		} while(adj2 != adj);

		if (f->size() != sz)
			return false;
	}

	if (nF != m_nFaces)
		return false;

	node v;
	forall_nodes(v,*m_cpGraph) {
		adjEntry adj;
		forall_adj(adj,v) {
			if (visited[adj] == false)
				return false;
		}
	}

	return true;
}

} // end namespace ogdf
