/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class BiconnectedShellingOrder which computes
 * a shelling order for a biconnected planar graph.
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


#include <ogdf/planarlayout/BiconnectedShellingOrder.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/SList.h>

#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/simple_graph_alg.h>


//#define OUTPUT_BSO

namespace ogdf {


//---------------------------------------------------------
// pair of node v and list itrator it
//---------------------------------------------------------
struct PairFaceItem;

struct PairNodeItem
{
	// constructor
	PairNodeItem() { }

	PairNodeItem(node v, ListIterator<PairFaceItem> it = ListIterator<PairFaceItem>())
	{
		m_v  = v;
		m_it = it;
	}

	node m_v;
	ListIterator<PairFaceItem> m_it;
};


//---------------------------------------------------------
// pair of face f and list iterator it
//---------------------------------------------------------

struct PairFaceItem
{
	// constructor
	PairFaceItem()
	{
		m_f  = 0;
		m_it = 0;
	}

	PairFaceItem(face f)
	{
		m_f  = f;
		m_it = 0;
	}

	PairFaceItem(face f, ListIterator<PairNodeItem> it)
	{
		m_f  = f;
		m_it = it;
	}

	face m_f;
	ListIterator<PairNodeItem> m_it;
};


// defines (as shortcuts)
// initialization of a variable
#define INIT_VAR(x,var,val) \
	var[x] = (val);          \
	setUpdate(x);

// decrement of a variable
#define DEC_VAR(x,var)     \
	--var[x];               \
	setUpdate(x);

// increment of a variable
#define INC_VAR(x,var)     \
	++var[x];               \
	setUpdate(x);


//---------------------------------------------------------
// class ComputeBicOrder
//---------------------------------------------------------
class ComputeBicOrder
{
public:
	// types of structures to be removed
	enum CandidateType { typeFace, typeNode, typeEdge };


	// constructor
	ComputeBicOrder (
		const Graph &G,                 // biconnected planar graph
		ConstCombinatorialEmbedding &E, // combinatorial embedding of G
		face extFace,              // external face
		double baseRatio);         // size of base (baseRatio * size(extFace)

	// returns external face
	face externalFace() { return m_extFace; }

	// returns face left of adj
	face left(adjEntry adj) { return m_pEmbedding->leftFace(adj); }

	// returns face right of adj
	face right(adjEntry adj) { return m_pEmbedding->rightFace(adj); }

	// if v = c_i, returns face right of c_i -> c_i+1
	face right(node v) { return left(m_nextSucc[v]); }

	// if v = c_i, returns face left of c_i -> c_i-1
	face left(node v) { return right(m_prevPred[v]); }

	// returns number of virtual edges adjacent to v
	int virte (node v);

	// returns successor of v on contour (if v=c_i returns c_i+1)
	node next(node v) { return m_next[v]; }

	// returns predecessor of v on contour (if v=c_i returns c_ii1)
	node prev(node v) { return m_prev[v]; }

	// returns true <=> f contains a virtual edge
	bool cutv(face f) { return (m_virtSrc[f] != 0); }

	// returns true <=> f is a possible next face, i.e
	//   outv(f) >= 3 and outv(f) = oute(f)+1
	bool isPossFace(face f) {
		return (f != externalFace() && m_outv[f] >= 3 && m_outv[f] == m_oute[f]+1);
	}

	// returns true <=> v is a possible next node, i.e.
	//   cutf(v) <= 1, cutf(v) = virte(v), numsf(v) = 0 and deg(v) >= 3
	bool isPossNode(node v) {  // precond.: v on C_k'
		return (m_onBase[v] == false && m_cutf[v] <= 1 &&
			m_cutf[v] == virte(v) && m_numsf[v] == 0 && m_deg[v] >= 3);
	}

	// returns true <=> v=c_i and c_i -> c_i+1 is a possible next virtual edge, i.e.
	//   c_i -> c_i+1 is a virtual edge and (deg(c_i) = 2 and c_i != vLeft) or
	//   deg(c_i+1) = 2 and c_i+1 != vRight)
	bool isPossVirt(node v) {  // precond.: v on C_k'
		return m_virtEdge[v] && ((m_deg[v] == 2 && v != m_vLeft) ||
			(m_deg[next(v)] == 2 && next(v) != m_vRight));
	}

	// stores the next candidate in m_nextType and m_nextF, m_nextV or
	// m_nextE (depending on type)
	// returns true <=> there is a next candidate
	bool getPossible();

	// returns the type of the next candidate
	CandidateType nextPoss() { return m_nextType; }

	// puts nodes on base into shelling order set V
	void setV1(ShellingOrderSet &V);

	// initializes possible nodes and faces
	void initPossibles();

	// removes next possible face
	void removeNextFace(ShellingOrderSet &V);
	// removes next possible node
	void removeNextNode(ShellingOrderSet &V);
	// removes next possible virtual edge
	void removeNextVirt(ShellingOrderSet &V);

	// updates variables of face and nodes contained in m_updateFaces and
	// m_updateNodes; also updates list of possible nodes, faces and virtual edges
	void doUpdate();

	// outputs contour and some node and face variables
	// for debugging only
	void print();

private:
	// if adj = w->v, puts edge (v,w) onto contour
	void edgeToContour(adjEntry adj);
	// puts virtual edge (v,w) onto contour and sets m_nextSucc[v] and m_prevPred[w]
	void virtToContour(node v, node w, adjEntry adjNextSucc, adjEntry adjPrevPred);
	// puts virtual edge (v,w) onto contour
	void virtToContour(node v, node w);

	// returns vertex cl of a face to be removed
	node getFaceCl(face f);

	void setOutv(node v);
	void setSeqp(node cl, node cr);
	void delOuterNode(node v);
	void decSeqp(node v);
	void putOnOuter(node v, face f);
	void delOuterRef(face f);

	// returns faces adjacent with v in list L
	void getAdjFaces(node v, SListPure<face> &L);
	// returns nodes adjacent with v in list L; nodes are sorted from left
	// to right
	void getAdjNodes(node v, SListPure<node> &L);

	// marks face f to be updated
	void setUpdate(face f);
	// marks node v to be updated
	void setUpdate(node v);

	void initVInFStruct(const ConstCombinatorialEmbedding &E);
	bool vInF(node v, face f);
	void delVInF(node v, face f);

	int getBaseChain(ConstCombinatorialEmbedding &E,
		face f,
		double baseRatio,
		adjEntry &adjLeft,
		adjEntry &adjRight);

	adjEntry findMaxBaseChain(ConstCombinatorialEmbedding &E,
		face f,
		int &length);

	const Graph                 *m_pGraph;     // the graph
	ConstCombinatorialEmbedding *m_pEmbedding; // the embedding of the graph

	face m_extFace; // the external face

	adjEntry m_adjLeft; // adjacency entry z_1 -> z_2 (if z_1,...,z_p is base)
	adjEntry m_adjRight; // adjacency entry z_p-1 -> z_p

	node m_vLeft, m_vRight; // left and right node on base
	int m_baseLength;       // length of base

	// next candidate to be removed
	CandidateType m_nextType; // type of next candidate
	face m_nextF; // next face (if m_nextType = typeFace)
	node m_nextV; // next node (if m_nextType = typeNode)
	node m_nextE; // next virtual edge (if m_nextType = typeEdge)

	// variables of nodes
	NodeArray<int>  m_deg;      // current degree
	NodeArray<int>  m_cutf;     // number of adjacent faces f with cutv(f) = true
	NodeArray<int>  m_numsf;    // number of adjacent faces f with outv(f) > seqp(f)+1
	NodeArray<bool> m_onOuter;  // true <=> v on contour
	NodeArray<bool> m_onBase;   // true <=> v on base

	// list iterator in m_possNodes containing node (or nil if not contained)
	NodeArray<ListIterator<node> > m_vLink;
	// list iterator in m_possVirt containing virtual edge at node (or nil if not contained)
	NodeArray<ListIterator<node> > m_virtLink;
	NodeArray<bool>	m_vUpdate;  // true <=> node marked to be updated
	NodeArray<ListPure<PairFaceItem> > m_inOutNodes;

	// variables of faces
	FaceArray<int>	m_outv,		// number of nodes contained in face on contour
					m_oute,		// number of edges contained in face on contour
					m_seqp;		// number of sequential pairs c_i,c_i+1 of face
	FaceArray<node>	m_virtSrc;	// if face contains virtual edge e then source(e), otherwise nil
	// list iterator in m_possFaces containing face (or nil if not contained)
	FaceArray<ListIterator<face> >	m_fLink;
	FaceArray<bool>	m_fUpdate;  // true <=> face marked to be updated
	FaceArray<bool> m_isSf;		// true <=> outv(f) > seqp(f)+1
	FaceArray<ListPure<PairNodeItem> > m_outerNodes;	// list of all nodes of face on contour

	// represantation of the contour
	NodeArray<node>	    m_next, m_prev;
	NodeArray<adjEntry>	m_nextSucc, m_prevPred;
	NodeArray<bool>	    m_virtEdge;	// virt_edge[c_i] = true <=> (c_i,c_i+1) virtuelle Kante

	// lists of possible faces, nodes and edges
	ListPure<face> m_possFaces; // faces with outv(f) >= 3 und outv(f) = oute(f)+1
	ListPure<node> m_possNodes; // nodes with cutf(v) <= 1, cutf(v) = virte(v), numsf(v) = 0 and deg(v) >= 3
	ListPure<node> m_possVirt;  // node v denotes virtual edge e = (v,next[v]), that satisfies
								// (deg(v) = 2 and v != vLeft) or (deg(next(v)) = 2 and next(v) != vRight)

	ListPure <node> m_updateNodes; // list of nodes to be updated
	SListPure<face> m_updateFaces; // list of faces to be updated

	// deciding "v in F ?" in constant time
	NodeArray<List<PairFaceItem> > m_facesOf;
	FaceArray<List<PairNodeItem> > m_nodesOf;
};


void ComputeBicOrder::print()
{
	cout << "contour:\n";
	node v;
	for(v = m_vLeft; v != 0; v = m_next[v])
		cout << " " << v << "[" << m_prev[v] << "," << m_prevPred[v] <<
			" : " << m_next[v] << "," << m_nextSucc[v] <<
			"; " << m_virtEdge[v] << "]\n";

	cout << "node infos:\n";
	forall_nodes(v,*m_pGraph)
		cout << v << ": deg = " << m_deg[v] << ", cutf = " << m_cutf[v] <<
			", numsf = " << m_numsf[v] << endl;

	cout << "face infos:\n";
	face f;
	forall_faces(f,*m_pEmbedding) {
		cout << f->index() << ": outv = " << m_outv[f] << ", oute = " <<
			m_oute[f] << ", seqp = " << m_seqp[f] << ", isSF = " <<
			m_isSf[f] << ", virtSrc = " << m_virtSrc[f] << endl;
	}
	cout << endl;
}


ComputeBicOrder::ComputeBicOrder(const Graph &G, // the graph
	ConstCombinatorialEmbedding &E,        // embedding of the graph
	face extFace,                          // the external face
	double baseRatio)                      // length of the base = baseRatio*size(extface)
{
	m_pGraph = &G;
	m_pEmbedding = &E;

#ifdef OUTPUT_BSO
	cout << "faces:" << endl;
	face fh;
	forall_faces(fh,E) {
		cout << fh->index() << ":";
		adjEntry adj;
		forall_face_adj(adj,fh)
			cout << " " << adj;
		cout << endl;
	}

	cout << "adjacency lists:" << endl;
	node vh;
	forall_nodes(vh,G) {
		cout << vh << ":";
		adjEntry adj;
		forall_adj(adj,vh)
			cout << " " << adj;
		cout << endl;
	}
#endif

	m_vLink   .init(G, ListIterator<node>());
	m_virtLink.init(G, ListIterator<node>());

	m_extFace = extFace;

#ifdef OUTPUT_BSO
	cout << "external face = " << extFace->index() << endl;
#endif

	m_baseLength = getBaseChain(E, m_extFace, baseRatio, m_adjLeft, m_adjRight);
	m_vLeft      = m_adjLeft->theNode();
	m_vRight     = m_adjRight->twinNode();

#ifdef OUTPUT_BSO
	cout << "vLeft = " << m_vLeft << ", " << "vRight = " << m_vRight << endl;
#endif

	// initialization of node and face variables
	m_deg      	 .init (G);
	m_cutf     	 .init (G, 0);
	m_numsf    	 .init (G, 0);
	m_onOuter 	 .init (G, false);
	m_next       .init (G);
	m_prev       .init (G);
	m_nextSucc   .init (G);
	m_prevPred   .init (G);
	m_virtEdge	 .init (G, false);
	m_vUpdate    .init (G, false);
	m_inOutNodes .init (G);
	m_outv       .init (E, 0);
	m_oute       .init (E, 0);
	m_seqp       .init (E, 0);
	m_virtSrc    .init (E, 0);
	m_fLink      .init (E, ListIterator<face>());
	m_fUpdate    .init (E, false);
	m_isSf       .init (E, false);
	m_outerNodes .init (E);
	m_onBase     .init (G, false);

	initVInFStruct(E);

	// initialization of degree
	node v, w;
	forall_nodes(v,G)
		m_deg[v] = v->degree();

	// initialization of m_onBase[v]
	adjEntry adj;
	for(adj = m_adjRight; adj != m_adjLeft; adj = adj->faceCyclePred())
		m_onBase[adj->theNode()] = true;
	m_onBase [m_vLeft] = m_onBase [m_vRight] = true;

	adj = m_adjLeft;
	do {
		v = adj->theNode();
		adjEntry adj2;
		forall_adj(adj2,v)
		{
			face f = E.rightFace(adj2);
			if (f != m_extFace) {
				m_outv[f] ++;
				putOnOuter(v,f);
			}
		}
		adj = adj->faceCyclePred();
	} while(adj != m_adjRight);

	for(adj = m_adjRight->faceCycleSucc(); adj != m_adjLeft; adj = adj->faceCycleSucc())
		m_oute[E.leftFace(adj)]++;

	m_onOuter [m_vLeft] = true;
	m_prevPred[m_vLeft] = m_nextSucc[m_vRight] = 0;
	m_prev[m_vLeft] = m_next[m_vRight] = 0;
	for (adj = m_adjLeft->faceCyclePred(); adj != m_adjRight; adj = adj->faceCyclePred())
	{
		v = adj->twinNode(); w = adj->theNode();
		m_onOuter[w] = true;
		edgeToContour(adj);

		adjEntry adj2;
		forall_adj(adj2,w)
		{
			face f = left(adj2);
			if (vInF(v,f))
				++m_seqp[f];
		}
	}

	for (v = m_vLeft; v != 0; v = next(v))
	{
		forall_adj(adj,v) {
			face f = left(adj);
			if ((m_isSf[f] = (m_outv[f] > m_seqp[f]+1)) == true)
				++m_numsf[v];
		}
	}
}


void ComputeBicOrder::setV1(ShellingOrderSet &V)
{
	V = ShellingOrderSet(m_baseLength, 0, 0);

	int i;
	adjEntry adj;
	for (i = 1, adj = m_adjLeft; i <= m_baseLength;
		i++, adj = adj->faceCycleSucc())
	{
		V[i] = adj->theNode();
	}
}


void ComputeBicOrder::edgeToContour(adjEntry adj)
{
	node v = adj->twinNode(), w = adj->theNode();

	m_next     [v] = w;
	m_prev     [w] = v;
	m_nextSucc [v] = adj->twin()->cyclicSucc();
	m_prevPred [w] = adj->cyclicPred();
	m_virtEdge [v] = false;
}


void ComputeBicOrder::virtToContour(
	node v,
	node w,
	adjEntry adjNextSucc,
	adjEntry adjPrevPred)
{
	m_next     [v] = w;
	m_prev     [w] = v;
	m_nextSucc [v] = adjNextSucc;
	m_prevPred [w] = adjPrevPred;
	m_virtEdge [v] = true;
}


void ComputeBicOrder::virtToContour(node v, node w)
{
	m_next     [v] = w;
	m_prev     [w] = v;
	m_virtEdge [v] = true;
}


void ComputeBicOrder::putOnOuter(node v, face f)
{
	ListIterator<PairNodeItem> it;

	it = m_outerNodes[f].pushBack(PairNodeItem(v));
	(*it).m_it = m_inOutNodes[v].pushBack(PairFaceItem(f,it));
}


void ComputeBicOrder::delOuterRef(face f)
{
	ListPure<PairNodeItem> &L = m_outerNodes[f];
	PairNodeItem x;

	while (!L.empty()) {
		x = L.popFrontRet();
		m_inOutNodes[x.m_v].del(x.m_it);
	}
}


int ComputeBicOrder::virte(node v)
{
	int num = 0;

	if (m_onOuter[v] == true)
	{
		if (m_virtEdge[v] == true)
			num++;
		if (v != m_vLeft && m_virtEdge[prev(v)] == true)
			num++;
	}
	return num;
}


void ComputeBicOrder::initVInFStruct(const ConstCombinatorialEmbedding &E)
{
	const Graph &G = E;

	m_facesOf.init(G);
	m_nodesOf.init(E);

	face f;
	forall_faces(f,E)
	{
		adjEntry adj;
		forall_face_adj(adj,f) {
			node v = adj->theNode();

			ListIterator<PairFaceItem> it = m_facesOf[v].pushBack(PairFaceItem(f));
			(*it).m_it = m_nodesOf[f].pushBack(PairNodeItem(v,it));
		}
	}

	SListPure<node> smallV;
	node v;
	forall_nodes(v,G) {
		if (m_facesOf[v].size() <= 5)
			smallV.pushBack(v);
	}

	SListPure<face> smallF;
	forall_faces(f,E) {
		if (m_nodesOf[f].size() <= 5)
			smallF.pushBack(f);
	}

	for( ; ; )
	{
		if (!smallV.empty()) {
			v = smallV.popFrontRet();

			ListIterator<PairFaceItem> it;
			for(it = m_facesOf[v].begin(); it.valid(); ++it) {
				PairFaceItem f_it = *it;
				m_nodesOf[f_it.m_f].del(f_it.m_it);
				if (m_nodesOf[f_it.m_f].size() == 5)
					smallF.pushBack(f_it.m_f);
			}
		} else if (!smallF.empty()) {
			f = smallF.popFrontRet();
			ListIterator<PairNodeItem> it;
			for(it = m_nodesOf[f].begin(); it.valid(); ++it) {
				PairNodeItem v_it = *it;
				m_facesOf[v_it.m_v].del(v_it.m_it);
				if (m_facesOf[v_it.m_v].size() == 5)
					smallV.pushBack(v_it.m_v);
			}
		} else
			break;
	}
}


bool ComputeBicOrder::vInF(node v, face f)
{
	ListIterator<PairNodeItem> itNI;
	for(itNI = m_nodesOf[f].begin(); itNI.valid(); ++itNI)
		if ((*itNI).m_v == v) return true;

	ListIterator<PairFaceItem> itFI;
	for(itFI = m_facesOf[v].begin(); itFI.valid(); ++itFI)
		if ((*itFI).m_f == f) return true;

	return false;
}


void ComputeBicOrder::delVInF(node v, face f)
{
	List<PairNodeItem> &L_f = m_nodesOf[f];
	List<PairFaceItem> &L_v = m_facesOf[v];

	ListIterator<PairNodeItem> itNI;
	for(itNI = L_f.begin(); itNI.valid(); ++itNI) {
		if ((*itNI).m_v == v) {
			L_f.del(itNI);
			return;
		}
	}

	ListIterator<PairFaceItem> itFI;
	for(itFI = L_v.begin(); itFI.valid(); ++itFI) {
		if ((*itFI).m_f == f) {
			L_v.del(itFI);
			return;
		}
	}
}


void ComputeBicOrder::initPossibles()
{
	face f;
	forall_faces (f, (*m_pEmbedding)) {
		if (isPossFace(f))
			m_fLink[f] = m_possFaces.pushBack(f);
	}

	node v;
	for (v = next(m_vLeft); v != m_vRight; v = next(v))
		if (isPossNode(v))
			m_vLink[v] = m_possNodes.pushBack(v);
}


bool ComputeBicOrder::getPossible()
{
	if (!m_possFaces.empty()) {
		m_nextType = typeFace;
		m_nextF = m_possFaces.popFrontRet();
		return true;

	} else if (!m_possNodes.empty()) {
		m_nextType = typeNode;
		m_nextV = m_possNodes.popFrontRet();
		return true;

	} else if (!m_possVirt.empty()) {
		m_nextType = typeEdge;
		m_nextE = m_possVirt.popFrontRet();
		m_virtLink[m_nextE] = ListIterator<node>();
		return true;

	} else
		return false;
}


node ComputeBicOrder::getFaceCl(face f)
{
	node v;

	if (cutv (f)) {
		v = m_virtSrc [f];

	} else {
		adjEntry adj;
		forall_face_adj(adj, f) {
			if (m_onOuter[v = adj->theNode()] == true && m_deg[v] == 2)
				break;
		}
	}

	while (v != m_vLeft && m_deg[v] == 2)
		v = prev(v);

	return v;
}


void ComputeBicOrder::getAdjFaces(node v, SListPure<face> &L)
{
	L.clear();
	if (m_deg[v] <= 1) return;

	adjEntry adjEnd   = (v != m_vLeft)  ? m_prevPred[v] : m_adjLeft->cyclicPred();
	adjEntry adjStart = (v != m_vRight) ? m_nextSucc[v] : m_adjRight->twin()->cyclicSucc();

	if (left(adjStart) != m_extFace)
		L.pushBack(left(adjStart));

	if (m_deg[v] >= 3) {
		adjEntry adj;
		for (adj = adjStart; adj != adjEnd; adj = adj->cyclicSucc())
			L.pushBack(right(adj));

		L.pushBack(right(adjEnd));
	}
}


void ComputeBicOrder::getAdjNodes(node v, SListPure<node> &L)
{
	adjEntry adjEnd   = (v != m_vLeft)  ? m_prevPred[v] : m_adjLeft->cyclicPred();
	adjEntry adjStart = (v != m_vRight) ? m_nextSucc[v] : m_adjRight->twin()->cyclicSucc();

	L.clear();
	L.pushBack((v != m_vLeft) ? prev(v) : m_adjLeft->twinNode());

	if (m_deg[v] >= 3) {
		adjEntry adj;
		for (adj = adjEnd; adj != adjStart; adj = adj->cyclicPred())
			L.pushBack(adj->twinNode());
		L.pushBack(adjStart->twinNode());
	}
	L.pushBack((v != m_vRight) ? next(v) : m_adjRight->theNode());
}


void ComputeBicOrder::decSeqp(node v)
{
	node vNext = next(v);
	node vPrev = prev(v);

	SListPure<face> L;
	getAdjFaces(v,L);

	SListConstIterator<face> it;
	for(it = L.begin(); it.valid(); ++it) {
		face f = *it;
		if (vInF(vNext,f))
			m_seqp[f]--;
		if (vInF(vPrev,f))
			m_seqp[f]--;
	}
}


void ComputeBicOrder::delOuterNode(node v)
{
	ListIterator<PairFaceItem> it;
	for(it = m_inOutNodes[v].begin(); it.valid(); ++it)
		m_outerNodes[(*it).m_f].del((*it).m_it);
}


void ComputeBicOrder::setOutv(node v)
{
	SListPure<face> L;
	getAdjFaces(v,L);

	SListConstIterator<face> it;
	for(it = L.begin(); it.valid(); ++it) {
		face f = *it;

		INC_VAR(f,m_outv)
		putOnOuter(v,f);
		if (cutv(f) == true) {
			INC_VAR(v, m_cutf)
		}
		if (m_isSf [f]) {
			INC_VAR(v, m_numsf)
		}
	}
}


void ComputeBicOrder::setSeqp(node cl, node cr)
{
	SListPure<face> L;

	node v, w;
	for (v = cl; v != cr; v = w)
	{
		w = next(v);

		node wSmall, wBig;
		if (m_deg[v] < m_deg[w]) {
			wSmall = v;
			wBig   = w;
		} else {
			wSmall = w;
			wBig   = v;
		}

		getAdjFaces(wSmall, L);

		SListConstIterator<face> it;
		for(it = L.begin(); it.valid(); ++it) {
			if (vInF(wBig,*it)) {
				INC_VAR (*it,m_seqp)
			}
		}
	}
}


void ComputeBicOrder::removeNextFace(ShellingOrderSet &V)
{
#ifdef OUTPUT_BSO
	cout << "remove next face: " << m_nextF->index() << endl;
#endif

	node cl = getFaceCl(m_nextF), cr, v;

	V = ShellingOrderSet(m_outv[m_nextF]-2);
	V.left(cl);

	int i;
	for (i = 1, cr = next(cl); cr != m_vRight && m_deg[cr] == 2; i++, cr = next(cr))
		V [i] = cr ;
	V.right (cr);
	V.leftAdj (m_virtEdge[cl]       ? 0 : m_nextSucc[cl]->cyclicSucc()->twin());
	V.rightAdj(m_virtEdge[prev(cr)] ? 0 : m_prevPred[cr]->cyclicPred()->twin());

	if (cutv(m_nextF) && next(m_virtSrc[m_nextF]) == cr)
		setUpdate(cr);

	if (cutv(m_nextF)) {
		DEC_VAR(cl,m_cutf)
		DEC_VAR(cr,m_cutf)
		v = m_virtSrc[m_nextF];
		if (v != cr) {
			m_possVirt.del(m_virtLink[v]);
			m_virtLink[v] = ListIterator<node>();
		}
	}

	adjEntry adj = m_nextSucc[cl]->twin();
	for( ; ; ) {
		edgeToContour(adj);

		if (adj->theNode() == cr)
			break;
		else {
			INIT_VAR(adj->theNode(),m_onOuter,true)
		}

		adj = adj->faceCyclePred();
	}
	DEC_VAR (cl,m_deg)
	DEC_VAR (cr,m_deg)

	for (v = cl; v != cr; v = next(v)) {
		INC_VAR(right(v),m_oute)
		if (v != cl)
			setOutv(v);
	}

	setSeqp(cl, cr);

	// possibly remove virtual edge
	if (cutv(m_nextF)) {
		if (m_virtSrc[m_nextF] == cl) {
			setUpdate(cl);
			m_virtEdge[cl] = false;
		}
		m_virtSrc[m_nextF] = 0;
	}
	delOuterRef(m_nextF);
}


void ComputeBicOrder::removeNextNode(ShellingOrderSet &V)
{
#ifdef OUTPUT_BSO
	cout << "remove next node: " << m_nextV << endl;
#endif

	node cl = prev(m_nextV);
	node cr = next(m_nextV);

	V = ShellingOrderSet(1);
	V[1] = m_nextV;

	if (m_virtEdge[prev(m_nextV)] == true) {
		V.left(m_prevPred[m_nextV]->twinNode());
		V.leftAdj(m_prevPred[m_nextV]);
	} else {
		V.left(prev(m_nextV));
		V.leftAdj(m_prevPred[m_nextV]->cyclicPred());
	}

	if (m_virtEdge[m_nextV] == true) {
		V.right(m_nextSucc[m_nextV]->twinNode());
		V.rightAdj(m_nextSucc[m_nextV]);
	} else {
		V.right(next(m_nextV));
		V.rightAdj(m_nextSucc[m_nextV]->cyclicSucc());
	}

	node vVirt = 0;
	face fVirt = 0;
	if (m_virtEdge[prev(m_nextV)]) {
		INIT_VAR(prev(m_nextV), m_virtEdge, false)
		vVirt = cl;
		fVirt = left(m_nextV);
		m_virtSrc [fVirt] = 0;
	}

	if (m_virtEdge[m_nextV]) {
		if (m_virtLink[m_nextV].valid()) {
			m_possVirt.del(m_virtLink[m_nextV]);
			m_virtLink[m_nextV] = ListIterator<node>();
		}
		vVirt = cr;
		fVirt = right(m_nextV);
		m_virtSrc[fVirt] = 0;
	}

	SListPure<face> L;
	getAdjFaces(m_nextV, L);
	SListConstIterator<face> itF;
	for(itF = L.begin(); itF.valid(); ++itF)
		--m_outv[*itF];

	SListPure<node> L_v;
	getAdjNodes(m_nextV, L_v);

	delOuterNode(m_nextV);
	--m_oute[left (m_nextV)];
	--m_oute[right(m_nextV)];
	decSeqp(m_nextV);

	SListIterator<node> itV;
	for(itV = L_v.begin(); itV.valid(); ++itV) {
		m_onOuter[*itV] = true;
		DEC_VAR (*itV, m_deg)
	}

	face potF = 0;
	node w1 = L_v.popFrontRet();
	bool firstTime = true;
	adjEntry adj,adj2;
	for(itV = L_v.begin(); itV.valid(); ++itV)
	{
		node w = *itV;

		if (firstTime == true) {
			adj2 = m_nextSucc[prev(m_nextV)];
			adj = m_prevPred[m_nextV];
			firstTime = false;

			if (prev(m_nextV) != m_vLeft) {
				face f = left(adj2);
				if (vInF(prev(prev(m_nextV)),f))
					potF = f;
			}

		} else {
			adj2 = adj->twin()->faceCyclePred()->twin();
			adj  = adj->cyclicPred();
		}

		for( ; ; )
		{
			node v = adj2->twinNode();

			if (v != w && m_onOuter[v] == true)
			{
				face f = left(adj2);

				// possibly remove "v in F" relation
				if (adj2->theNode() != w1)
				{
					adjEntry adj1 = adj2->twin()->faceCycleSucc();
					do {
						delVInF(adj1->twinNode(),f);
						adj1 = adj1->faceCycleSucc();
					} while (adj1->theNode() != w1);
				}
				if (f == potF && adj2->theNode() != prev(m_nextV)) {
					DEC_VAR(f,m_seqp)
				}

				// insert new virtual edge
				virtToContour(adj2->theNode(), w, adj2, (w == next(m_nextV)) ?
					m_prevPred[w] : adj->twin()->cyclicPred());

				setUpdate(f);

				INC_VAR(adj2->theNode(),m_deg)
				INC_VAR(w,m_deg)

				if (f != fVirt) {
					ListIterator<PairNodeItem> itU;
					for(itU = m_outerNodes[f].begin(); itU.valid(); ++itU) {
						INC_VAR((*itU).m_v, m_cutf);
					}
				}
				m_virtSrc[f] = adj2->theNode();

				break;
			}

			edgeToContour(adj2->twin());

			if (v == w) {
				delOuterRef(left(adj2));
				break;
			}
			INIT_VAR(v,m_onOuter,true)
			if (adj2->theNode() == cl)
			{
				ListIterator<PairNodeItem> it, itSucc;
				ListPure<PairNodeItem> &L = m_outerNodes[left(adj2)];
				for(it = L.begin(); it.valid(); it = itSucc) {
					itSucc = it.succ();
					if ((*it).m_v == cl) {
						m_inOutNodes[cl].del((*it).m_it);
						L.del(it);
						break;
					}
				}
				m_outv[left(adj2)]--;
			}
			adj2 = adj2->twin()->faceCyclePred()->twin();
		}
		w1 = w;
	}

	for (node v = cl; v != cr; v = next(v)) {
		INC_VAR(right(v),m_oute)
		if (v != cl)
			setOutv(v);
	}

	setSeqp(cl,cr);

	if ((vVirt != 0 && m_virtSrc[fVirt] == 0) ||
		(vVirt ==  cl && m_virtSrc[fVirt] != cl)) {
		DEC_VAR(vVirt,m_cutf)
	}
}


void ComputeBicOrder::removeNextVirt(ShellingOrderSet &V)
{
#ifdef OUTPUT_BSO
	cout << "remove next virt: " << m_nextE << endl;
#endif

	node v, cl = m_nextE, cr = next(m_nextE);
	int i = 0;

	while (m_deg[cl] == 2 && cl != m_vLeft)
		{ cl = prev(cl); i++; }
	while (m_deg[cr] == 2 && cr != m_vRight)
		{ cr = next(cr); i++; }

	V = ShellingOrderSet(i,m_virtEdge[cl] ? 0 : m_prevPred[next(cl)],
		m_virtEdge[prev(cr)] ? 0 : m_nextSucc[prev(cr)]);
	for (i = 1, v = next(cl); v != cr; v = next(v)) {
		V[i++] = v;
		delOuterNode(v);
	}
	V.left (cl);
	V.right(cr);

	face f = right(cl);
	m_virtSrc[f] = cl;

	virtToContour(cl, cr);

	INIT_VAR(f,m_outv,(m_outv[f] - V.len()))
	INIT_VAR(f,m_oute,(m_oute[f] - V.len()))
	INIT_VAR(f,m_seqp,(m_seqp[f] - V.len()-1))
	setSeqp(cl,cr);
	setUpdate(cl);
	setUpdate(cr);
}


void ComputeBicOrder::setUpdate(node v)
{
	if (m_vUpdate[v] == false) {
		m_updateNodes.pushBack(v);
		m_vUpdate[v] = true;
	}
}


void ComputeBicOrder::setUpdate(face f)
{
	if (m_fUpdate[f] == false) {
		m_updateFaces.pushBack(f);
		m_fUpdate[f] = true;
	}
}


void ComputeBicOrder::doUpdate()
{
	while (!m_updateFaces.empty())
	{
		face f = m_updateFaces.popFrontRet();
		m_fUpdate[f] = false;
		bool isSeperatingFace = (m_outv[f] > m_seqp[f]+1);
		if (isSeperatingFace != m_isSf[f])
		{
			ListIterator<PairNodeItem> it;
			for(it = m_outerNodes[f].begin(); it.valid(); ++it)
			{
				if (isSeperatingFace) {
					INC_VAR((*it).m_v,m_numsf)
				} else {
					DEC_VAR((*it).m_v,m_numsf)
				}
			}
			m_isSf[f] = isSeperatingFace;
		}
		bool possible = isPossFace(f);
		if (possible && !m_fLink[f].valid())
			m_fLink[f] = m_possFaces.pushBack(f);
		else if (!possible && m_fLink[f].valid()) {
			m_possFaces.del(m_fLink[f]);
			m_fLink[f] = ListIterator<face>();
		}
	}

	ListIterator<node> it, itPrev;
	for (it = m_updateNodes.rbegin(); it.valid(); it = itPrev)
	{
		itPrev = it.pred();
		node v = *it;
		if (v != m_vLeft && m_virtEdge[prev(v)] == true)
			setUpdate(prev(v));
	}

	while (!m_updateNodes.empty())
	{
		node v = m_updateNodes.popFrontRet();
		m_vUpdate[v] = false;

		bool possible = isPossNode(v);
		if (possible && !m_vLink[v].valid())
			m_vLink[v] = m_possNodes.pushBack(v);
		else if (!possible && m_vLink[v].valid()) {
			m_possNodes.del(m_vLink[v]);
			m_vLink[v] = ListIterator<node>();
		}
		possible = isPossVirt(v);
		if (possible && !m_virtLink[v].valid())
			m_virtLink[v] = m_possVirt.pushBack(v);
		else if (!possible && m_virtLink[v].valid()) {
			m_possVirt.del(m_virtLink[v]);
			m_virtLink[v] = ListIterator<node>();
		}
	}
}


int ComputeBicOrder::getBaseChain(ConstCombinatorialEmbedding &E,
	face f,
	double baseRatio,
	adjEntry &adjLeft,
	adjEntry &adjRight)
{
	int len;
	adjLeft = findMaxBaseChain(E, f, len);
	len = max(2, min(len, (int)(baseRatio*f->size()+0.5)));

	adjRight = adjLeft;
	for (int i = 2; i < len; i++)
		adjRight = adjRight->faceCycleSucc();

	return len;
}


struct QType
{
	QType (adjEntry adj, int i) {
		m_start = adj;
		m_limit = i;
	}
	QType () {
		m_start = 0;
		m_limit = 0;
	}

	adjEntry m_start;
	int      m_limit;
};


adjEntry ComputeBicOrder::findMaxBaseChain(ConstCombinatorialEmbedding &E,
	face f,
	int &length)
{
	const Graph &G = (const Graph &) E;
	int p = f->size();

	NodeArray<int> num(G,-1);

	int i = 0, j, d;

	adjEntry adj;
	forall_face_adj(adj,f)
		num[adj->theNode()] = i++;

	Array<SListPure<int> > diag(0,p-1);
	forall_face_adj(adj,f)
	{
		i = num[adj->theNode()];
		adjEntry adj2;
		for (adj2 = adj->cyclicPred(); adj2 != adj->cyclicSucc();
			adj2 = adj2->cyclicPred())
		{
			j = num[adj2->twinNode()];
			if (j != -1)
				diag[i].pushBack(j);
		}
	}

	SListPure<QType> Q;
	Array<SListIterator<QType> > posInQ (0,p-1,SListIterator<QType>());

	length = 0;
	bool firstRun = true;
	adj = f->firstAdj();
	i = num[adj->theNode()];

	adjEntry adjStart = 0;
	do {
		if (posInQ[i].valid()) {
			adjEntry adj2 = Q.front().m_start;
			d = (i-num[adj2->theNode()]+p) % p +1;
			if (d > length || (d == length && adj2->theNode()->index() < adjStart->theNode()->index())) {
				length = d;
				adjStart = adj2;
			}
			SListIterator<QType> it, itLimit = posInQ[i];
			do {
				it = Q.begin();
				posInQ[(*it).m_limit] = SListIterator<QType>();
				Q.popFront();
			} while (it != itLimit);
		}

		if (diag[i].empty())
			j = (i-2+p) % p;
		else {
			int m = p;
			SListConstIterator<int> it;
			for(it = diag[i].begin(); it.valid(); ++it) {
				int k = *it;
				d = (k-i+p)%p;
				if (d < m) {
					m = d;
					j = k;
				}
			}
			j = (j-1+p) % p;
			if (!firstRun) {
				posInQ[Q.back().m_limit] = 0;
				Q.back().m_limit = j;
				posInQ[j] = Q.rbegin();
			}
		}

		if (firstRun)
			posInQ[j] = Q.pushBack(QType(adj,j));

		adj = adj->faceCycleSucc();
		i = num[adj->theNode()];
		if (i == 0) firstRun = false;
	} while (!Q.empty());

	return adjStart;
}


//---------------------------------------------------------
// BiconnectedShellingOrder
//---------------------------------------------------------

void BiconnectedShellingOrder::doCall(const Graph &G,
	adjEntry adj,
	List<ShellingOrderSet> &partition)
{
	OGDF_ASSERT(isBiconnected(G) == true);
	OGDF_ASSERT(G.representsCombEmbedding() == true);

	ConstCombinatorialEmbedding E(G);

	face extFace = (adj != 0) ? E.rightFace(adj) : E.maximalFace();
	ComputeBicOrder cpo(G,E,extFace,m_baseRatio);

	cpo.initPossibles();

#ifdef OUTPUT_BSO
	cout << "after initialization:\n";
	cpo.print();
#endif

	while(cpo.getPossible())
	{
		switch(cpo.nextPoss())
		{
		case ComputeBicOrder::typeFace:
			partition.pushFront(ShellingOrderSet());
			cpo.removeNextFace(partition.front());
			break;

		case ComputeBicOrder::typeNode:
			partition.pushFront(ShellingOrderSet());
			cpo.removeNextNode(partition.front());
			break;

		case ComputeBicOrder::typeEdge:
			partition.pushFront(ShellingOrderSet());
			cpo.removeNextVirt(partition.front());
			break;
		}

		cpo.doUpdate();

#ifdef OUTPUT_BSO
		cout << "after update:\n";
		cpo.print();
#endif
	}

	partition.pushFront(ShellingOrderSet(2));
	cpo.setV1(partition.front());
}


} // end namespace ogdf

