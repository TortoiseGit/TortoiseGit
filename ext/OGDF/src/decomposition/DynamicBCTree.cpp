/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class DynamicBCTree
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


#include <ogdf/decomposition/DynamicBCTree.h>


namespace ogdf {


void DynamicBCTree::init ()
{
	m_bNode_owner.init(m_B);
	m_bNode_degree.init(m_B);
	node vB;
	forall_nodes (vB,m_B) {
		m_bNode_owner[vB] = vB;
		m_bNode_degree[vB] = vB->degree();
	}
}


node DynamicBCTree::unite (node uB, node vB, node wB)
{
	node uH = cutVertex(vB,uB);
	node vH = cutVertex(vB,vB);
	node wH = cutVertex(vB,wB);

	node mH,sH;
	if (uH->degree()>=wH->degree()) { mH = uH; sH = wH; } else { mH = wH; sH = uH; }

	node mB,sB,tB;
	if (m_bNode_numNodes[uB]>=m_bNode_numNodes[wB]) { mB = uB; sB = wB; } else { mB = wB; sB = uB; }
	if (m_bNode_degree[vB]==2) {
		if (m_bNode_numNodes[mB]==0) { mB = vB; sB = uB; tB = wB; } else tB = vB;
	}

	if (m_bNode_hParNode[vB]==uH) {
		m_bNode_hParNode[vB] = mH;
		m_bNode_hRefNode[mB] = m_bNode_hRefNode[uB];
		m_bNode_hParNode[mB] = m_bNode_hParNode[uB];
	}
	else if (m_bNode_hParNode[vB]==wH) {
		m_bNode_hParNode[vB] = mH;
		m_bNode_hRefNode[mB] = m_bNode_hRefNode[wB];
		m_bNode_hParNode[mB] = m_bNode_hParNode[wB];
	}
	else if (m_bNode_degree[vB]==2) {
		m_bNode_hRefNode[mB] = 0;
		m_bNode_hParNode[mB] = 0;
	}
	else {
		m_bNode_hRefNode[mB] = mH;
		m_bNode_hParNode[mB] = vH;
	}

	adjEntry aH = sH->firstAdj();
	while (aH) {
		adjEntry bH = aH->succ();
		if (aH->theEdge()->source()==sH) m_H.moveSource(aH->theEdge(),mH);
		else m_H.moveTarget(aH->theEdge(),mH);
		aH = bH;
	}
	m_H.delNode(sH);

	m_numB--;
	m_bNode_owner[sB] = mB;
	m_bNode_hEdges[mB].conc(m_bNode_hEdges[sB]);
	m_bNode_numNodes[mB] = m_bNode_numNodes[uB] + m_bNode_numNodes[wB] - 1;
	m_bNode_degree[mB] = m_bNode_degree[uB] + m_bNode_degree[wB] - 1;

	if (m_bNode_degree[vB]==2) {
		m_numC--;
		m_bNode_type[vB] = BComp;
		m_gNode_hNode[m_hNode_gNode[vH]] = mH;
		m_H.delNode(vH);
		m_bNode_owner[tB] = mB;
		m_bNode_hEdges[mB].conc(m_bNode_hEdges[tB]);
		m_bNode_degree[mB]--;
	}
	else m_bNode_degree[vB]--;

	return mB;
}


node DynamicBCTree::find (node vB) const
{
	if (!vB) return 0;
	if (m_bNode_owner[vB]==vB) return vB;
	return m_bNode_owner[vB] = find(m_bNode_owner[vB]);
}


node DynamicBCTree::bcproper (node vG) const
{
	if (!vG) return 0;
	node vH = m_gNode_hNode[vG];
	return m_hNode_bNode[vH] = find(m_hNode_bNode[vH]);
}


node DynamicBCTree::bcproper (edge eG) const
{
	if (!eG) return 0;
	edge eH = m_gEdge_hEdge[eG];
	return m_hEdge_bNode[eH] = find(m_hEdge_bNode[eH]);
}


node DynamicBCTree::parent (node vB) const
{
	if (!vB) return 0;
	node vH = m_bNode_hParNode[vB];
	if (!vH) return 0;
	return m_hNode_bNode[vH] = find(m_hNode_bNode[vH]);
}


node DynamicBCTree::condensePath (node sG, node tG)
{
	SList<node>& pB = findPath(sG,tG);
	SListConstIterator<node> iB = pB.begin();
	node uB = *iB++;
	if (iB.valid()) {
		if (m_bNode_type[uB]==CComp) uB = *iB++;
		while (iB.valid()) {
			node vB = *iB++;
			if (!iB.valid()) break;
			node wB = *iB++;
			uB = unite(uB,vB,wB);
		}
	}
	delete &pB;
	return uB;
}


edge DynamicBCTree::updateInsertedEdge (edge eG)
{
	node vB = condensePath(eG->source(),eG->target());
	edge eH = m_H.newEdge(repVertex(eG->source(),vB),repVertex(eG->target(),vB));
	m_bNode_hEdges[vB].pushBack(eH);
	m_hEdge_bNode[eH] = vB;
	m_hEdge_gEdge[eH] = eG;
	m_gEdge_hEdge[eG] = eH;
	return eG;
}


node DynamicBCTree::updateInsertedNode (edge eG, edge fG)
{
	node eB = bcproper(eG);
	node uG = fG->source();
	m_gNode_isMarked[uG] = false;

	if (numberOfEdges(eB)==1) {
		node tG = fG->target();
		node sH = m_gEdge_hEdge[eG]->target();
		m_hNode_gNode[sH] = uG;

		node uB = m_B.newNode();
		node uH = m_H.newNode();
		m_bNode_type[uB] = CComp;
		m_bNode_owner[uB] = uB;
		m_bNode_numNodes[uB] = 1;
		m_bNode_degree[uB] = 2;
		m_bNode_isMarked[uB] = false;
		m_bNode_hRefNode[uB] = uH;
		m_hNode_bNode[uH] = uB;
		m_hNode_gNode[uH] = uG;
		m_gNode_hNode[uG] = uH;

		node fB = m_B.newNode();
		node vH = m_H.newNode();
		node wH = m_H.newNode();
		edge fH = m_H.newEdge(vH,wH);
		m_bNode_type[fB] = BComp;
		m_bNode_owner[fB] = fB;
		m_bNode_numNodes[fB] = 2;
		m_bNode_degree[fB] = 2;
		m_bNode_isMarked[fB] = false;
		m_bNode_hEdges[fB].pushBack(fH);
		m_hNode_bNode[vH] = fB;
		m_hNode_bNode[wH] = fB;
		m_hEdge_bNode[fH] = fB;
		m_hNode_gNode[vH] = uG;
		m_hNode_gNode[wH] = tG;
		m_hEdge_gEdge[fH] = fG;
		m_gEdge_hEdge[fG] = fH;

		node tH = m_gNode_hNode[tG];
		if (m_bNode_hParNode[eB]==tH) {
			m_bNode_hParNode[eB] = uH;
			m_bNode_hParNode[uB] = vH;
			m_bNode_hRefNode[fB] = wH;
			m_bNode_hParNode[fB] = tH;
		}
		else {
			node tB = bcproper(tG);
			m_bNode_hParNode[tB] = wH;
			m_bNode_hRefNode[fB] = vH;
			m_bNode_hParNode[fB] = uH;
			m_bNode_hParNode[uB] = sH;
		}
	}
	else {
		edge fH = m_H.split(m_gEdge_hEdge[eG]);
		m_bNode_hEdges[eB].pushBack(fH);
		m_hEdge_bNode[fH] = eB;
		m_hEdge_gEdge[fH] = fG;
		m_gEdge_hEdge[fG] = fH;
		node uH = fH->source();
		m_bNode_numNodes[eB]++;
		m_hNode_bNode[uH] = eB;
		m_hNode_gNode[uH] = uG;
		m_gNode_hNode[uG] = uH;
	}
	return uG;
}


node DynamicBCTree::bComponent (node uG, node vG) const
{
	node uB = this->bcproper(uG);
	node vB = this->bcproper(vG);
	if (uB==vB) return uB;
	if (typeOfBNode(uB)==BComp) {
		if (typeOfBNode(vB)==BComp) return 0;
		if (this->parent(uB)==vB) return uB;
		if (this->parent(vB)==uB) return uB;
		return 0;
	}
	if (typeOfBNode(vB)==BComp) {
		if (this->parent(uB)==vB) return vB;
		if (this->parent(vB)==uB) return vB;
		return 0;
	}
	node pB = this->parent(uB);
	node qB = this->parent(vB);
	if (pB==qB) return pB;
	if (this->parent(pB)==vB) return pB;
	if (this->parent(qB)==uB) return qB;
	return 0;
}



}
