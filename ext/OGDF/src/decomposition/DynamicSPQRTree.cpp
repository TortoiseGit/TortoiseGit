/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of classes DynamicSkeleton and DynamicSPQRTree
 *
 * \author Jan Papenfu&szlig;
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


#include <ogdf/decomposition/DynamicSPQRTree.h>


namespace ogdf {

//-------------------------------------------------------------------
//                          DynamicSkeleton
//-------------------------------------------------------------------

DynamicSkeleton::DynamicSkeleton (const DynamicSPQRTree* T, node vB) : Skeleton(vB), m_owner(T)
{
	m_origNode.init(m_M,0);
	m_origEdge.init(m_M,0);
}


const SPQRTree &DynamicSkeleton::owner () const
{
	return *m_owner;
}


node DynamicSkeleton::original (node vM) const
{
	return m_owner->m_hNode_gNode[m_origNode[vM]];
}


edge DynamicSkeleton::realEdge (edge eM) const
{
	return m_owner->m_hEdge_gEdge[m_origEdge[eM]];
}


edge DynamicSkeleton::twinEdge (edge eM) const
{
	edge eH = m_owner->m_hEdge_twinEdge[m_origEdge[eM]];
	if (!eH) return 0;
	m_owner->skeleton(m_owner->spqrproper(eH));
	return m_owner->m_skelEdge[eH];
}


node DynamicSkeleton::twinTreeNode (edge eM) const
{
	edge eH = m_owner->m_hEdge_twinEdge[m_origEdge[eM]];
	if (!eH) return 0;
	return m_owner->spqrproper(eH);
}


//-------------------------------------------------------------------
//                          DynamicSPQRTree
//-------------------------------------------------------------------

//
// initialization: builds tree, skeleton graphs and cross references
//
void DynamicSPQRTree::init (edge eG)
{
	createSPQR(bcproper(eG));
	rootTreeAt(eG);
	m_sk.init(m_T,0);
	m_skelEdge.init(m_H,0);
	m_mapV.init(m_H,0);
	m_cpV = 0;
}


//
// createSkeleton: creates a skeleton graph
//
DynamicSkeleton& DynamicSPQRTree::createSkeleton (node vT) const
{
	DynamicSkeleton& S = *OGDF_NEW DynamicSkeleton(this,vT);

	SList<node> inMapV;

	for (ListConstIterator<edge> itH=m_tNode_hEdges[vT].begin(); itH.valid(); ++itH)
	{
		edge eH = *itH;
		node sH = eH->source();
		node tH = eH->target();

		edge& eM = m_skelEdge[eH];
		node& sM = m_mapV[sH];
		node& tM = m_mapV[tH];

		if (!sM) {
			sM = S.m_M.newNode();
			S.m_origNode[sM] = sH;
			inMapV.pushBack(sH);
		}

		if (!tM) {
			tM = S.m_M.newNode();
			S.m_origNode[tM] = tH;
			inMapV.pushBack(tH);
		}

		eM = S.m_M.newEdge(sM,tM);
		S.m_origEdge[eM] = eH;
	}

	while(!inMapV.empty()) m_mapV[inMapV.popFrontRet()] = 0;

	S.m_referenceEdge = m_tNode_hRefEdge[vT];
	if(S.m_referenceEdge) S.m_referenceEdge = m_skelEdge[S.m_referenceEdge];

	m_sk[vT] = &S;
	return S;
}


//
// destructor: deletes skeleton graphs
//
DynamicSPQRTree::~DynamicSPQRTree ()
{
	node vB;
	forall_nodes (vB,m_T) delete m_sk[vB];

	delete m_cpV;
}


List<node> DynamicSPQRTree::nodesOfType (NodeType t) const
{
	TNodeType tt = (TNodeType)t;
	List<node> L;
	node vT;
	forall_nodes (vT,m_T) {
		if (m_tNode_owner[vT]!=vT) continue;
		if (m_tNode_type[vT]==tt) L.pushBack(vT);
	}
	return L;
}


node DynamicSPQRTree::rootTreeAt (edge eG)
{
	node vT = rootTreeAt(spqrproper(m_gEdge_hEdge[eG]));
	m_rootEdge = eG;
	return vT;
}


node DynamicSPQRTree::rootTreeAt (node vT)
{
	vT = findSPQR(vT);
	node uT = vT;
	edge eH = 0;
	for( ; ; ) {
		edge fH = m_tNode_hRefEdge[uT];
		m_tNode_hRefEdge[uT] = eH;
		if (!fH) break;
		eH = m_hEdge_twinEdge[fH];
		uT = spqrproper(eH);
	}
	m_rootEdge = 0;
	return m_bNode_SPQR[m_B.firstNode()] = vT;
}


edge DynamicSPQRTree::updateInsertedEdge (edge eG)
{
	SList<node> marked;
	node sH = m_gNode_hNode[eG->source()];
	node tH = m_gNode_hNode[eG->target()];
	adjEntry aH;
	forall_adj (aH,sH) {
		edge fH = aH->theEdge();
		node vT = spqrproper(fH);
		if (fH->opposite(sH)==tH) {
			if (m_tNode_type[vT]==PComp) {
				DynamicSPQRForest::updateInsertedEdge(eG);
				if (m_sk[vT]) {
					edge eH = m_gEdge_hEdge[eG];
					edge fM = m_skelEdge[fH];
					node sM = fM->source();
					node tM = fM->target();
					if (eH->source()==m_sk[vT]->m_origNode[tM]) { node uM = sM; sM = tM; tM = uM; }
					m_skelEdge[eH] = m_sk[vT]->getGraph().newEdge(sM,tM);
					m_sk[vT]->m_origEdge[m_skelEdge[eH]] = eH;
				}
				return eG;
			}
			else if (!m_hEdge_twinEdge[fH]) {
				DynamicSPQRForest::updateInsertedEdge(eG);
				if (m_sk[vT]) {
					edge gH = m_hEdge_twinEdge[m_tNode_hEdges[m_hEdge_tNode[fH]].front()];
					m_skelEdge[gH] = m_skelEdge[fH];
					m_sk[vT]->m_origEdge[m_skelEdge[gH]] = gH;
				}
				return eG;
			}
			else {
				m_tNode_isMarked[vT] = true;
				marked.pushBack(vT);
			}
		}
		else {
			m_tNode_isMarked[vT] = true;
			marked.pushBack(vT);
		}
	}
	int count = 0;
	node found[2];
	forall_adj (aH,tH) {
		edge fH = aH->theEdge();
		node vT = spqrproper(fH);
		if (!m_tNode_isMarked[vT]) continue;
		found[count++] = vT;
		m_tNode_isMarked[vT] = false;
	}
	while (!marked.empty()) m_tNode_isMarked[marked.popFrontRet()] = false;
	if (count==0) {
		node rT;
		SList<node>& pT = findPathSPQR(sH,tH,rT);
		for (SListIterator<node> iT=pT.begin(); iT.valid(); ++iT)
			if (m_sk[*iT]) {
				delete m_sk[*iT];
				m_sk[*iT] = 0;
			}
		delete &pT;
	}
	else if (count==1) {
		node vT = found[0];
		if (m_sk[vT]) {
			delete m_sk[vT];
			m_sk[vT] = 0;
		}
	}
	return DynamicSPQRForest::updateInsertedEdge(eG);
}


node DynamicSPQRTree::updateInsertedNode (edge eG, edge fG)
{
	edge eH = m_gEdge_hEdge[eG];
	node vT = spqrproper(eH);
	if (m_tNode_type[vT]==SComp) {
		DynamicSPQRForest::updateInsertedNode(eG,fG);
		if (m_sk[vT]) {
			edge fH = m_gEdge_hEdge[fG];
			edge fM = m_skelEdge[fH] = m_sk[vT]->getGraph().split(m_skelEdge[eH]);
			m_sk[vT]->m_origNode[fM->source()] = fH->source();
			m_sk[vT]->m_origEdge[fM] = fH;
		}
	}
	else {
		DynamicSPQRForest::updateInsertedNode(eG,fG);
		if (m_sk[vT]) {
			edge gH = m_hEdge_twinEdge[m_tNode_hEdges[spqrproper(eH)].front()];
			edge gM = m_skelEdge[gH] = m_skelEdge[eH];
			m_sk[vT]->m_origEdge[gM] = gH;
		}
	}
	return fG->source();
}


} // end namespace ogdf
