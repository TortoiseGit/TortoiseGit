/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements classes StaticSkeleton and StaticSPQRTree
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


#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/GraphCopy.h>
#include "TricComp.h"


namespace ogdf {

//-------------------------------------------------------------------
//                           StaticSkeleton
//-------------------------------------------------------------------

StaticSkeleton::StaticSkeleton(const StaticSPQRTree *T, node vT) : Skeleton(vT), m_owner(T)
{
	m_orig.init(m_M,0);
	m_real.init(m_M,0);
	m_treeEdge.init(m_M,0);
}


const SPQRTree &StaticSkeleton::owner() const
{
	return *m_owner;
}


edge StaticSkeleton::twinEdge (edge e) const
{
	edge et = m_treeEdge[e];
	if (et == 0)
		return 0;

	return (et->source() == m_treeNode) ?
		m_owner->m_skEdgeTgt[et] : m_owner->m_skEdgeSrc[et];
}


node StaticSkeleton::twinTreeNode (edge e) const
{
	edge et = m_treeEdge[e];
	if (et == 0)
		return 0;
	return et->opposite(m_treeNode);
}


//-------------------------------------------------------------------
//                           StaticSPQRTree
//-------------------------------------------------------------------

//
// initialization: builds tree, skeleton graphs and cross references
//
void StaticSPQRTree::init(edge eRef)
{
	TricComp tricComp(*m_pGraph);
	init(eRef,tricComp);
}

void StaticSPQRTree::init(edge eRef, TricComp &tricComp)
{
	m_cpV = 0;
	const GraphCopySimple &GC = *tricComp.m_pGC;

	m_type.init(m_tree,SNode);
	m_sk.init(m_tree,0);

	m_skEdgeSrc.init(m_tree,0);
	m_skEdgeTgt.init(m_tree,0);

	NodeArray<node> mapV(GC,0);
	BoundedStack<node> inMapV(GC.numberOfNodes());

	EdgeArray<node> partnerNode(GC,0);
	EdgeArray<edge> partnerEdge(GC,0);

	m_numS = m_numP = m_numR = 0;

	for (int i = 0; i < tricComp.m_numComp; i++) {
		const TricComp::CompStruct &C = tricComp.m_component[i];

		if (C.m_edges.empty()) continue;

		node vT = m_tree.newNode();

		switch(C.m_type) {
		case TricComp::bond:
			m_type[vT] = PNode;
			m_numP++; break;

		case TricComp::polygon:
			m_type[vT] = SNode;
			m_numS++; break;

		case TricComp::triconnected:
			m_type[vT] = RNode;
			m_numR++; break;
		}

		m_sk[vT] = OGDF_NEW StaticSkeleton(this,vT);
		StaticSkeleton &S = *m_sk[vT];

		ListConstIterator<edge> itE;
		for(itE = C.m_edges.begin(); itE.valid(); ++itE)
		{
			edge e = *itE;
			edge eG  = GC.original(e);

			node uGC = e->source(), vGC = e->target();
			node uM = mapV[uGC], vM = mapV[vGC];

			if (uM == 0) {
				uM = mapV[uGC] = S.m_M.newNode();
				inMapV.push(uGC);
				S.m_orig[uM] = GC.original(uGC);
			}
			if (vM == 0) {
				vM = mapV[vGC] = S.m_M.newNode();
				inMapV.push(vGC);
				S.m_orig[vM] = GC.original(vGC);
			}

			// normalize direction of virtual edges
			if(eG == 0 && GC.original(vGC) < GC.original(uGC))
				swap(uM,vM);

			edge eM  = S.m_M.newEdge(uM,vM);

			if (eG == 0) {
				if (partnerNode[e] == 0) {
					partnerNode[e] = vT;
					partnerEdge[e] = eM;

				} else {
					edge eT = m_tree.newEdge(partnerNode[e],vT);
					StaticSkeleton &pS = *m_sk[partnerNode[e]];
					pS.m_treeEdge[partnerEdge[e]] = S.m_treeEdge[eM] = eT;
					m_skEdgeSrc[eT] = partnerEdge[e];
					m_skEdgeTgt[eT] = eM;
				}

			} else {
				S.m_real[eM] = eG;
				m_copyOf[eG] = eM;
				if (eG->source() != S.original(eM->source()))
					S.m_M.reverseEdge(eM);
				m_skOf  [eG] = &S;
			}
		}

		while(!inMapV.empty())
			mapV[inMapV.pop()] = 0;
	}

	rootTreeAt(eRef);
}


//
// destructor: deletes skeleton graphs
//
StaticSPQRTree::~StaticSPQRTree()
{
	node vT;

	forall_nodes(vT,m_tree)
		delete m_sk[vT];

	delete m_cpV;
}


List<node> StaticSPQRTree::nodesOfType(NodeType t) const
{
	List<node> L;
	node v;
	forall_nodes(v,m_tree)
		if (m_type[v] == t) L.pushBack(v);

	return L;
}


//
// rooting of tree at edge e
node StaticSPQRTree::rootTreeAt(edge e)
{
	m_rootEdge = e;
	m_rootNode = m_skOf[e]->treeNode();

	m_sk[m_rootNode]->m_referenceEdge = m_copyOf[e];
	rootRec(m_rootNode,0);

	return m_rootNode;
}


node StaticSPQRTree::rootTreeAt(node v)
{
	m_rootEdge = 0;
	m_rootNode = v;

	m_sk[m_rootNode]->m_referenceEdge = 0;
	rootRec(m_rootNode,0);

	return m_rootNode;
}


void StaticSPQRTree::rootRec(node v, edge eFather)
{
	edge e;
	forall_adj_edges(e,v)
	{
		if (e == eFather) continue;

		node w = e->target();
		if (w == v) {
			m_tree.reverseEdge(e);
			swap(m_skEdgeSrc[e],m_skEdgeTgt[e]);
			w = e->target();
		}

		m_sk[w]->m_referenceEdge = m_skEdgeTgt[e];
		rootRec(w,e);
	}
}


} // end namespace ogdf
