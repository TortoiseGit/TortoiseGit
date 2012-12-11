/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class DynamicSPQRForest
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


#include <ogdf/decomposition/DynamicSPQRForest.h>
#include <ogdf/basic/GraphCopy.h>
#include "../decomposition/TricComp.h"


namespace ogdf {


void DynamicSPQRForest::init ()
{
	m_bNode_SPQR.init(m_B,0);
	m_bNode_numS.init(m_B,0);
	m_bNode_numP.init(m_B,0);
	m_bNode_numR.init(m_B,0);
	m_tNode_type.init(m_T,SComp);
	m_tNode_owner.init(m_T);
	m_tNode_hRefEdge.init(m_T);
	m_tNode_hEdges.init(m_T);
	m_tNode_isMarked.init(m_T,false);
	m_hEdge_position.init(m_H);
	m_hEdge_tNode.init(m_H);
	m_hEdge_twinEdge.init(m_H,0);
	m_htogc.init(m_H);
}


void DynamicSPQRForest::createSPQR (node vB) const
{
	Graph GC;
	NodeArray<node> origNode(GC,0);
	EdgeArray<edge> origEdge(GC,0);
	SListConstIterator<edge> iH;

	for (iH=m_bNode_hEdges[vB].begin(); iH.valid(); ++iH)
		m_htogc[(*iH)->source()] = m_htogc[(*iH)->target()] = 0;

	for (iH=m_bNode_hEdges[vB].begin(); iH.valid(); ++iH) {
		edge eH = *iH;
		node sH = eH->source();
		node tH = eH->target();
		node& sGC = m_htogc[sH];
		node& tGC = m_htogc[tH];
		if (!sGC) { sGC = GC.newNode(); origNode[sGC] = sH; }
		if (!tGC) { tGC = GC.newNode(); origNode[tGC] = tH; }
		origEdge[GC.newEdge(sGC,tGC)] = eH;
	}

	TricComp tricComp(GC);

	const GraphCopySimple& GCC = *tricComp.m_pGC;

	EdgeArray<node> partnerNode(GCC,0);
	EdgeArray<edge> partnerEdge(GCC,0);

	for (int i=0; i<tricComp.m_numComp; ++i) {
		const TricComp::CompStruct &C = tricComp.m_component[i];

		if (C.m_edges.empty()) continue;

		node vT = m_T.newNode();
		m_tNode_owner[vT] = vT;

		switch(C.m_type) {
			case TricComp::bond:
				m_tNode_type[vT] = PComp;
				m_bNode_numP[vB]++;
				break;
			case TricComp::polygon:
				m_tNode_type[vT] = SComp;
				m_bNode_numS[vB]++;
				break;
			case TricComp::triconnected:
				m_tNode_type[vT] = RComp;
				m_bNode_numR[vB]++;
				break;
		}

		for (ListConstIterator<edge> iGCC=C.m_edges.begin(); iGCC.valid(); ++iGCC) {
			edge eGCC = *iGCC;
			edge eH = GCC.original(eGCC);
			if (eH) eH = origEdge[eH];
			else {
				node uH = origNode[GCC.original(eGCC->source())];
				node vH = origNode[GCC.original(eGCC->target())];
				eH = m_H.newEdge(uH,vH);

				if (!partnerNode[eGCC]) {
					partnerNode[eGCC] = vT;
					partnerEdge[eGCC] = eH;
				}
				else {
					m_T.newEdge(partnerNode[eGCC],vT);
					m_hEdge_twinEdge[eH] = partnerEdge[eGCC];
					m_hEdge_twinEdge[partnerEdge[eGCC]] = eH;
				}
			}
			m_hEdge_position[eH] = m_tNode_hEdges[vT].pushBack(eH);
			m_hEdge_tNode[eH] = vT;
		}
	}

	m_bNode_SPQR[vB] = m_hEdge_tNode[origEdge[GC.firstEdge()]];
	m_tNode_hRefEdge[m_bNode_SPQR[vB]] = 0;

	SList<node> lT;
	lT.pushBack(m_bNode_SPQR[vB]);
	lT.pushBack(0);
	while (!lT.empty()) {
		node vT = lT.popFrontRet();
		node wT = lT.popFrontRet();
		for (ListConstIterator<edge> iH=m_tNode_hEdges[vT].begin(); iH.valid(); ++iH) {
			edge eH = *iH;
			edge fH = m_hEdge_twinEdge[eH];
			if (!fH) continue;
			node uT = m_hEdge_tNode[fH];
			if (uT==wT) m_tNode_hRefEdge[vT] = eH;
			else {
				lT.pushBack(uT);
				lT.pushBack(vT);
			}
		}
	}
}


node DynamicSPQRForest::uniteSPQR (node vB, node sT, node tT)
{
	if (m_tNode_type[tT]==SComp) m_bNode_numS[vB]--;
	else if (m_tNode_type[tT]==PComp) m_bNode_numP[vB]--;
	else if (m_tNode_type[tT]==RComp) m_bNode_numR[vB]--;
	if (!sT) {
		m_bNode_numR[vB]++;
		sT = tT;
	}
	else {
		if (m_tNode_hEdges[sT].size()<m_tNode_hEdges[tT].size()) { node uT = sT; sT = tT; tT = uT; }
		m_tNode_owner[tT] = sT;
		m_tNode_hEdges[sT].conc(m_tNode_hEdges[tT]);
	}
	m_tNode_type[sT] = RComp;
	return sT;
}


node DynamicSPQRForest::findSPQR (node vT) const
{
	if (!vT) return vT;
	if (m_tNode_owner[vT]==vT) return vT;
	return m_tNode_owner[vT] = findSPQR(m_tNode_owner[vT]);
}


node DynamicSPQRForest::findNCASPQR (node sT, node tT) const
{
	if (m_tNode_isMarked[sT]) return sT;
	m_tNode_isMarked[sT] = true;
	node uT = m_tNode_hRefEdge[sT] ? spqrproper(m_hEdge_twinEdge[m_tNode_hRefEdge[sT]]) : 0;
	if (uT) uT = findNCASPQR(tT,uT);
	else for (uT=tT; !m_tNode_isMarked[uT]; uT=spqrproper(m_hEdge_twinEdge[m_tNode_hRefEdge[uT]]));
	m_tNode_isMarked[sT] = false;
	return uT;
}


SList<node>& DynamicSPQRForest::findPathSPQR (node sH, node tH, node& rT) const
{
	SList<node>& pT = *OGDF_NEW SList<node>;
	node sT = spqrproper(sH->firstAdj()->theEdge());
	node tT = spqrproper(tH->firstAdj()->theEdge());
	node nT = findNCASPQR(sT,tT);
	while (sT!=nT) {
		edge eH = m_tNode_hRefEdge[sT];
		node uH = eH->source();
		node vH = eH->target();
		if (uH!=sH && vH!=sH) pT.pushBack(sT);
		if (uH==tH || vH==tH) { rT = sT; return pT; }
		sT = spqrproper(m_hEdge_twinEdge[eH]);
	}
	SListIterator<node> iT = pT.rbegin();
	while (tT!=nT) {
		edge eH = m_tNode_hRefEdge[tT];
		node uH = eH->source();
		node vH = eH->target();
		if (uH!=tH && vH!=tH) {
			if (iT.valid()) pT.insertAfter(tT,iT);
			else pT.pushFront(tT);
		}
		if (uH==sH || vH==sH) { rT = tT; return pT; }
		tT = spqrproper(m_hEdge_twinEdge[eH]);
	}
	if (iT.valid()) pT.insertAfter(nT,iT);
	else pT.pushFront(nT);
	rT = nT; return pT;
}


SList<node>& DynamicSPQRForest::findPathSPQR (node sH, node tH) const
{
	node vB = bComponent(m_hNode_gNode[sH],m_hNode_gNode[tH]);
	if (!vB) return *OGDF_NEW SList<node>;
	if (!m_bNode_SPQR[vB]) {
		if (m_bNode_hEdges[vB].size()<3) return *OGDF_NEW SList<node>;
		createSPQR(vB);
	}
	node rT;
	SList<node>& pT = findPathSPQR(sH,tH,rT);
	if (pT.empty()) if (rT) pT.pushBack(rT);
	return pT;
}


edge DynamicSPQRForest::virtualEdge (node vT, node wT) const
{
	edge eH = m_tNode_hRefEdge[vT];
	if (eH) {
		eH = m_hEdge_twinEdge[eH];
		if (spqrproper(eH)==wT) return eH;
	}
	eH = m_tNode_hRefEdge[wT];
	if (eH) {
		if (spqrproper(m_hEdge_twinEdge[eH])==vT) return eH;
	}
	return 0;
}


edge DynamicSPQRForest::updateInsertedEdgeSPQR (node vB, edge eG)
{
	node sG = eG->source();
	node tG = eG->target();
	node sH = repVertex(sG,vB);
	node tH = repVertex(tG,vB);
	edge eH = m_H.newEdge(sH,tH);
	m_gEdge_hEdge[eG] = eH;
	m_hEdge_gEdge[eH] = eG;

	adjEntry aH;
	forall_adj (aH,sH) {
		edge fH = aH->theEdge();
		if (fH==eH) continue;
		if (fH->opposite(sH)!=tH) continue;
		node vT = spqrproper(fH);
		if (m_tNode_type[vT]==PComp) {
			m_hEdge_position[eH] = m_tNode_hEdges[vT].pushBack(eH);
			m_hEdge_tNode[eH] = vT;
			return eG;
		}
		edge gH = m_hEdge_twinEdge[fH];
		if (!gH) {
			m_bNode_numP[vB]++;
			node nT = m_T.newNode();
			m_tNode_type[nT] = PComp;
			m_tNode_owner[nT] = nT;
			edge v1 = m_H.newEdge(sH,tH);
			edge v2 = m_H.newEdge(sH,tH);
			m_hEdge_position[v1] = m_tNode_hEdges[vT].insertAfter(v1,m_hEdge_position[fH]);
			m_tNode_hEdges[vT].del(m_hEdge_position[fH]);
			m_hEdge_position[v2] = m_tNode_hEdges[nT].pushBack(v2);
			m_hEdge_position[fH] = m_tNode_hEdges[nT].pushBack(fH);
			m_hEdge_position[eH] = m_tNode_hEdges[nT].pushBack(eH);
			m_hEdge_tNode[v1] = vT;
			m_hEdge_twinEdge[v1] = m_tNode_hRefEdge[nT] = v2;
			m_hEdge_tNode[v2] = m_hEdge_tNode[eH] = m_hEdge_tNode[fH] = nT;
			m_hEdge_twinEdge[v2] = v1;
			return eG;
		}
		node wT = spqrproper(gH);
		if (m_tNode_type[wT]==PComp) {
			m_hEdge_position[eH] = m_tNode_hEdges[vT].pushBack(eH);
			m_hEdge_tNode[eH] = vT;
		}
		else {
			m_bNode_numP[vB]++;
			node nT = m_T.newNode();
			m_tNode_type[nT] = PComp;
			m_tNode_owner[nT] = nT;
			edge v1 = m_tNode_hRefEdge[vT];
			if (!v1) v1 = m_tNode_hRefEdge[wT];
			else if (spqrproper(m_hEdge_twinEdge[v1])!=wT) v1 = m_tNode_hRefEdge[wT];
			edge v4 = m_hEdge_twinEdge[v1];
			edge v2 = m_H.newEdge(v1->source(),v1->target());
			edge v3 = m_H.newEdge(v4->source(),v4->target());
			m_hEdge_twinEdge[v1] = v2;
			m_hEdge_twinEdge[v2] = v1;
			m_hEdge_twinEdge[v3] = v4;
			m_hEdge_twinEdge[v4] = v3;
			m_hEdge_position[v2] = m_tNode_hEdges[nT].pushBack(v2);
			m_hEdge_position[eH] = m_tNode_hEdges[nT].pushBack(eH);
			m_hEdge_position[v3] = m_tNode_hEdges[nT].pushBack(v3);
			m_hEdge_tNode[v2] = m_hEdge_tNode[eH] = m_hEdge_tNode[v3] = nT;
			m_tNode_hRefEdge[nT] = v3;
		}
		return eG;
	}

	node rT;
	SList<node>& pT = findPathSPQR(sH,tH,rT);
	if (pT.size()<2) {
		if (m_tNode_type[rT]==RComp) {
			m_hEdge_position[eH] = m_tNode_hEdges[rT].pushBack(eH);
			m_hEdge_tNode[eH] = rT;
		}
		else {
			List<edge>& aH = m_tNode_hEdges[rT];
			SList<edge> bH;
			bool a_is_parent = true;
			ListIterator<edge> iH = aH.begin();
			node uH = sH;
			while (uH!=tH) {
				node xH = (*iH)->source();
				node yH = (*iH)->target();
				if (xH==uH) uH = yH;
				else if (yH==uH) uH = xH;
				else { iH = aH.cyclicSucc(iH); continue; }
				if (*iH==m_tNode_hRefEdge[rT]) a_is_parent = false;
				bH.pushBack(*iH);
				ListIterator<edge> jH = iH;
				iH = aH.cyclicSucc(iH);
				aH.del(jH);
			}
			m_bNode_numS[vB]++;
			m_bNode_numP[vB]++;
			node sT = m_T.newNode();
			node pT = m_T.newNode();
			m_tNode_type[sT] = SComp;
			m_tNode_type[pT] = PComp;
			m_tNode_owner[sT] = sT;
			m_tNode_owner[pT] = pT;
			edge v1 = m_H.newEdge(sH,tH);
			edge v2 = m_H.newEdge(sH,tH);
			edge v3 = m_H.newEdge(sH,tH);
			edge v4 = m_H.newEdge(sH,tH);
			m_hEdge_twinEdge[v1] = v2;
			m_hEdge_twinEdge[v2] = v1;
			m_hEdge_twinEdge[v3] = v4;
			m_hEdge_twinEdge[v4] = v3;
			m_hEdge_position[v1] = m_tNode_hEdges[sT].pushBack(v1);
			m_hEdge_position[v2] = m_tNode_hEdges[pT].pushBack(v2);
			m_hEdge_position[eH] = m_tNode_hEdges[pT].pushBack(eH);
			m_hEdge_position[v3] = m_tNode_hEdges[pT].pushBack(v3);
			m_hEdge_position[v4] = m_tNode_hEdges[rT].pushBack(v4);
			m_hEdge_tNode[v1] = sT;
			m_hEdge_tNode[v2] = m_hEdge_tNode[eH] = m_hEdge_tNode[v3] = pT;
			m_hEdge_tNode[v4] = rT;
			for (SListConstIterator<edge> iH=bH.begin(); iH.valid(); ++iH) {
				m_hEdge_position[*iH] = m_tNode_hEdges[sT].pushBack(*iH);
				m_hEdge_tNode[*iH] = sT;
			}
			if (a_is_parent) {
				m_tNode_hRefEdge[sT] = v1;
				m_tNode_hRefEdge[pT] = v3;
			}
			else {
				m_tNode_hRefEdge[sT] = m_tNode_hRefEdge[rT];
				m_tNode_hRefEdge[pT] = v2;
				m_tNode_hRefEdge[rT] = v4;
				if (!m_tNode_hRefEdge[sT]) m_bNode_SPQR[vB] = sT;
			}
		}
	}
	else {
		node xT = 0;
		SList<edge> absorbedEdges;
		SList<edge> virtualEdgesInPath;
		SList<edge> newVirtualEdges;

		edge rH = m_tNode_hRefEdge[rT];

		SListIterator<node> iT = pT.begin();
		SListIterator<node> jT = iT;

		virtualEdgesInPath.pushBack(0);
		for (++jT; jT.valid(); ++iT, ++jT) {
			edge gH,fH = m_tNode_hRefEdge[*iT];
			if (!fH) {
				gH = m_tNode_hRefEdge[*jT];
				fH = m_hEdge_twinEdge[gH];
			}
			else {
				gH = m_hEdge_twinEdge[fH];
				if (spqrproper(gH)!=*jT) {
					gH = m_tNode_hRefEdge[*jT];
					fH = m_hEdge_twinEdge[gH];
				}
			}
			virtualEdgesInPath.pushBack(fH);
			virtualEdgesInPath.pushBack(gH);
		}
		virtualEdgesInPath.pushBack(0);

		for (iT=pT.begin(); iT.valid(); ++iT) {
			edge fH = virtualEdgesInPath.popFrontRet();
			edge gH = virtualEdgesInPath.popFrontRet();
			if (m_tNode_type[*iT]==SComp) {
				List<edge>& aH = m_tNode_hEdges[*iT];
				SList<edge> bH;
				ListIterator<edge> iH,jH;
				node zH;

				node uH = 0;
				if (!fH) { fH = gH; uH = sH; }
				else if (!gH) uH = tH;

				node vH = fH->source();
				node wH = fH->target();

				if (uH) {
					iH = jH = m_hEdge_position[fH];
					for( ; ; ) {
						iH = aH.cyclicSucc(iH);
						node xH = (*iH)->source();
						node yH = (*iH)->target();
						if (xH==vH) { zH = vH; vH = yH; break; }
						if (xH==wH) { zH = wH; wH = vH; vH = yH; break; }
						if (yH==vH) { zH = vH; vH = xH; break; }
						if (yH==wH) { zH = wH; wH = vH; vH = xH; break; }
					}
					m_H.delEdge(*jH);
					aH.del(jH);
					jH = iH;
					iH = aH.cyclicSucc(iH);
					bH.pushBack(*jH);
					aH.del(jH);
					while (vH!=uH) {
						for( ; ; ) {
							node xH = (*iH)->source();
							node yH = (*iH)->target();
							if (xH==vH) { vH = yH; break; }
							if (yH==vH) { vH = xH; break; }
							iH = aH.cyclicSucc(iH);
						}
						jH = iH;
						iH = aH.cyclicSucc(iH);
						bH.pushBack(*jH);
						aH.del(jH);
					}
					if (bH.size()==1) {
						edge nH = bH.front();
						if (nH==rH) rT = 0;
						absorbedEdges.pushBack(nH);
					}
					else {
						m_bNode_numS[vB]++;
						node nT = m_T.newNode();
						m_tNode_type[nT] = SComp;
						m_tNode_owner[nT] = nT;
						while (!bH.empty()) {
							edge nH = bH.popFrontRet();
							m_hEdge_position[nH] = m_tNode_hEdges[nT].pushBack(nH);
							m_hEdge_tNode[nH] = nT;
							if (nH==rH) rT = nT;
						}
						edge nH = m_H.newEdge(vH,zH);
						m_hEdge_position[nH] = m_tNode_hEdges[nT].pushBack(nH);
						m_hEdge_tNode[nH] = nT;
						if (nT==rT) {
							m_tNode_hRefEdge[nT] = rH;
							if (!rH) m_bNode_SPQR[vB] = nT;
							rH = nH;
						}
						else m_tNode_hRefEdge[nT] = nH;
						newVirtualEdges.pushBack(nH);
					}
					if (m_tNode_hEdges[*iT].size()==1) xT = uniteSPQR(vB,xT,*iT);
					else {
						edge nH = m_H.newEdge(wH,vH);
						m_hEdge_position[nH] = m_tNode_hEdges[*iT].pushBack(nH);
						m_hEdge_tNode[nH] = *iT;
						if (*iT==rT) rH = nH;
						else m_tNode_hRefEdge[*iT] = nH;
						newVirtualEdges.pushBack(nH);
					}
				}
				else if (aH.size()==3) {
					aH.del(m_hEdge_position[fH]);
					aH.del(m_hEdge_position[gH]);
					m_H.delEdge(fH);
					m_H.delEdge(gH);
					xT = uniteSPQR(vB,xT,*iT);
				}
				else {
					node xH = gH->source();
					node yH = gH->target();
					edge nH = 0;
					if (vH==xH) nH = m_H.newEdge(wH,yH);
					else if (vH==yH) nH = m_H.newEdge(wH,xH);
					else if (wH==xH) nH = m_H.newEdge(vH,yH);
					else if (wH==yH) nH = m_H.newEdge(vH,xH);
					if (nH) {
						m_hEdge_position[nH] = aH.insertAfter(nH,m_hEdge_position[gH]);
						m_hEdge_tNode[nH] = *iT;
						if (*iT==rT) rH = nH;
						else m_tNode_hRefEdge[*iT] = nH;
						aH.del(m_hEdge_position[fH]);
						aH.del(m_hEdge_position[gH]);
						m_H.delEdge(fH);
						m_H.delEdge(gH);
						newVirtualEdges.pushBack(nH);
					}
					else {
						iH = jH = m_hEdge_position[fH];
						for( ; ; ) {
							iH = aH.cyclicSucc(iH);
							node xH = (*iH)->source();
							node yH = (*iH)->target();
							if (xH==vH) { zH = vH; vH = yH; break; }
							if (xH==wH) { zH = wH; wH = vH; vH = yH; break; }
							if (yH==vH) { zH = vH; vH = xH; break; }
							if (yH==wH) { zH = wH; wH = vH; vH = xH; break; }
						}
						m_H.delEdge(*jH);
						aH.del(jH);
						jH = iH;
						iH = aH.cyclicSucc(iH);
						bH.pushBack(*jH);
						aH.del(jH);
						node pH = gH->source();
						node qH = gH->target();
						while (vH!=pH && vH!=qH) {
							for( ; ; ) {
								node xH = (*iH)->source();
								node yH = (*iH)->target();
								if (xH==vH) { vH = yH; break; }
								if (yH==vH) { vH = xH; break; }
								iH = aH.cyclicSucc(iH);
							}
							jH = iH;
							iH = aH.cyclicSucc(iH);
							bH.pushBack(*jH);
							aH.del(jH);
						}
						aH.del(m_hEdge_position[gH]);
						m_H.delEdge(gH);
						if (bH.size()==1) {
							edge nH = bH.front();
							if (nH==rH) rT = 0;
							absorbedEdges.pushBack(nH);
						}
						else {
							m_bNode_numS[vB]++;
							node nT = m_T.newNode();
							m_tNode_type[nT] = SComp;
							m_tNode_owner[nT] = nT;
							while (!bH.empty()) {
								edge nH = bH.popFrontRet();
								m_hEdge_position[nH] = m_tNode_hEdges[nT].pushBack(nH);
								m_hEdge_tNode[nH] = nT;
								if (nH==rH) rT = nT;
							}
							edge nH = m_H.newEdge(vH,zH);
							m_hEdge_position[nH] = m_tNode_hEdges[nT].pushBack(nH);
							m_hEdge_tNode[nH] = nT;
							if (nT==rT) {
								m_tNode_hRefEdge[nT] = rH;
								if (!rH) m_bNode_SPQR[vB] = nT;
								rH = nH;
							}
							else m_tNode_hRefEdge[nT] = nH;
							newVirtualEdges.pushBack(nH);
						}
						if (m_tNode_hEdges[*iT].size()==1) xT = uniteSPQR(vB,xT,*iT);
						else {
							edge nH = m_H.newEdge(wH,vH==pH?qH:pH);
							m_hEdge_position[nH] = m_tNode_hEdges[*iT].pushBack(nH);
							m_hEdge_tNode[nH] = *iT;
							if (*iT==rT) rH = nH;
							else m_tNode_hRefEdge[*iT] = nH;
							newVirtualEdges.pushBack(nH);
						}
					}
				}
			}
			else {
				if (fH) {
					m_tNode_hEdges[*iT].del(m_hEdge_position[fH]);
					m_H.delEdge(fH);
				}
				if (gH) {
					m_tNode_hEdges[*iT].del(m_hEdge_position[gH]);
					m_H.delEdge(gH);
				}
				if (m_tNode_type[*iT]==PComp) {
					if (m_tNode_hEdges[*iT].size()>1) {
						edge nH = m_tNode_hEdges[*iT].front();
						nH = m_H.newEdge(nH->source(),nH->target());
						m_hEdge_position[nH] = m_tNode_hEdges[*iT].pushBack(nH);
						m_hEdge_tNode[nH] = *iT;
						if (*iT==rT) rH = nH;
						else m_tNode_hRefEdge[*iT] = nH;
						newVirtualEdges.pushBack(nH);
					}
					else xT = uniteSPQR(vB,xT,*iT);
				}
				else xT = uniteSPQR(vB,xT,*iT);
			}
		}
		if (!xT) {
			m_bNode_numR[vB]++;
			xT = m_T.newNode();
			m_tNode_type[xT] = RComp;
			m_tNode_owner[xT] = xT;
		}
		while (!newVirtualEdges.empty()) {
			edge oH = newVirtualEdges.popFrontRet();
			edge nH = m_H.newEdge(oH->source(),oH->target());
			m_hEdge_position[nH] = m_tNode_hEdges[xT].pushBack(nH);
			m_hEdge_tNode[nH] = xT;
			m_hEdge_twinEdge[nH] = oH;
			m_hEdge_twinEdge[oH] = nH;
		}
		while (!absorbedEdges.empty()) {
			edge nH = absorbedEdges.popFrontRet();
			m_hEdge_position[nH] = m_tNode_hEdges[xT].pushBack(nH);
			m_hEdge_tNode[nH] = xT;
		}
		m_hEdge_position[eH] = m_tNode_hEdges[xT].pushBack(eH);
		m_hEdge_tNode[eH] = xT;
		if (!rT) m_tNode_hRefEdge[xT] = rH;
		else if (findSPQR(rT)!=xT) m_tNode_hRefEdge[xT] = m_hEdge_twinEdge[rH];
		else {
			m_tNode_hRefEdge[xT] = rH;
			if (!rH) m_bNode_SPQR[vB] = xT;
		}
	}
	delete &pT;
	return eG;
}


node DynamicSPQRForest::updateInsertedNodeSPQR (node vB, edge eG, edge fG)
{
	node vG = fG->source();
	node wG = fG->target();
	node vH = m_H.newNode();
	node wH = repVertex(wG,vB);
	m_gNode_hNode[vG] = vH;
	m_hNode_gNode[vH] = vG;
	edge fH = m_H.newEdge(vH,wH);
	m_gEdge_hEdge[fG] = fH;
	m_hEdge_gEdge[fH] = fG;
	edge eH = m_gEdge_hEdge[eG];
	m_H.moveTarget(eH,vH);
	node vT = spqrproper(eH);
	if (m_tNode_type[vT]==SComp) {
		m_hEdge_position[fH] = m_tNode_hEdges[vT].insertAfter(fH,m_hEdge_position[eH]);
		m_hEdge_tNode[fH] = vT;
	}
	else {
		m_bNode_numS[vB]++;
		node nT = m_T.newNode();
		m_tNode_type[nT] = SComp;
		m_tNode_owner[nT] = nT;
		node uH = eH->source();
		node wH = fH->target();
		edge v1 = m_H.newEdge(uH,wH);
		edge v2 = m_H.newEdge(uH,wH);
		m_hEdge_position[v1] = m_tNode_hEdges[vT].insertAfter(v1,m_hEdge_position[eH]);
		m_tNode_hEdges[vT].del(m_hEdge_position[eH]);
		m_hEdge_position[v2] = m_tNode_hEdges[nT].pushBack(v2);
		m_hEdge_position[eH] = m_tNode_hEdges[nT].pushBack(eH);
		m_hEdge_position[fH] = m_tNode_hEdges[nT].pushBack(fH);
		m_hEdge_tNode[v1] = vT;
		m_hEdge_twinEdge[v1] = m_tNode_hRefEdge[nT] = v2;
		m_hEdge_tNode[v2] = m_hEdge_tNode[eH] = m_hEdge_tNode[fH] = nT;
		m_hEdge_twinEdge[v2] = v1;
	}
	return vG;
}


edge DynamicSPQRForest::updateInsertedEdge (edge eG)
{
	node sG = eG->source();
	node tG = eG->target();
	node vB = bComponent(sG,tG);
	if (vB) {
		if (m_bNode_SPQR[vB]) {
			edge eH = m_gEdge_hEdge[updateInsertedEdgeSPQR(vB,eG)];
			m_bNode_hEdges[vB].pushBack(eH);
			m_hEdge_bNode[eH] = vB;
		}
		else DynamicBCTree::updateInsertedEdge(eG);
	}
	else {
		node nT = 0;
		int numS,numP,numR;
		SList<node>& pB = findPath(sG,tG);
		SListIterator<node> jB = pB.begin();
		SListIterator<node> iB = jB;
		while (iB.valid()) { if (m_bNode_SPQR[*iB]) break; ++iB; }
		if (iB.valid()) {
			nT = m_T.newNode();
			m_tNode_type[nT] = SComp;
			m_tNode_owner[nT] = nT;
			m_tNode_hRefEdge[nT] = 0;
			numS = 1;
			numP = 0;
			numR = 0;
			node sH = repVertex(sG,*jB);
			for (iB=jB; iB.valid(); ++iB) {
				node tH = (++jB).valid() ? cutVertex(*jB,*iB) : repVertex(tG,*iB);
				node mT;
				edge mH,nH;
				switch (numberOfEdges(*iB)) {
					case 0:
						break;
					case 1:
						nH = m_bNode_hEdges[*iB].front();
						m_hEdge_position[nH] = m_tNode_hEdges[nT].pushBack(nH);
						m_hEdge_tNode[nH] = nT;
						break;
					case 2:
						mT = m_T.newNode();
						m_tNode_type[mT] = PComp;
						m_tNode_owner[mT] = mT;
						mH = m_bNode_hEdges[*iB].front();
						m_hEdge_position[mH] = m_tNode_hEdges[mT].pushBack(mH);
						m_hEdge_tNode[mH] = mT;
						mH = m_bNode_hEdges[*iB].back();
						m_hEdge_position[mH] = m_tNode_hEdges[mT].pushBack(mH);
						m_hEdge_tNode[mH] = mT;
						mH = m_H.newEdge(sH,tH);
						m_hEdge_position[mH] = m_tNode_hEdges[mT].pushBack(mH);
						m_hEdge_tNode[mH] = mT;
						nH = m_H.newEdge(sH,tH);
						m_hEdge_position[nH] = m_tNode_hEdges[nT].pushBack(nH);
						m_hEdge_tNode[nH] = nT;
						m_hEdge_twinEdge[mH] = nH;
						m_hEdge_twinEdge[nH] = mH;
						m_tNode_hRefEdge[mT] = mH;
						numP++;
						break;
					default:
						if (!m_bNode_SPQR[*iB]) createSPQR(*iB);
						edge mG = m_G.newEdge(m_hNode_gNode[sH],m_hNode_gNode[tH]);
						updateInsertedEdgeSPQR(*iB,mG);
						mH = m_gEdge_hEdge[mG];
						mT = spqrproper(mH);
						m_G.delEdge(mG);
						m_hEdge_gEdge[mH] = 0;
						nH = m_H.newEdge(sH,tH);
						m_hEdge_position[nH] = m_tNode_hEdges[nT].pushBack(nH);
						m_hEdge_tNode[nH] = nT;
						m_hEdge_twinEdge[mH] = nH;
						m_hEdge_twinEdge[nH] = mH;
						for( ; ; ) {
							nH = m_tNode_hRefEdge[mT];
							m_tNode_hRefEdge[mT] = mH;
							if (!nH) break;
							mH = m_hEdge_twinEdge[nH];
							mT = spqrproper(mH);
						}
						numS += m_bNode_numS[*iB];
						numP += m_bNode_numP[*iB];
						numR += m_bNode_numR[*iB];
				}
				if (jB.valid()) sH = cutVertex(*iB,*jB);
			}
		}
		delete &pB;
		DynamicBCTree::updateInsertedEdge(eG);
		if (nT) {
			edge eH = m_gEdge_hEdge[eG];
			m_hEdge_position[eH] = m_tNode_hEdges[nT].pushBack(eH);
			m_hEdge_tNode[eH] = nT;
			node eB = bcproper(eG);
			m_bNode_SPQR[eB] = nT;
			m_bNode_numS[eB] = numS;
			m_bNode_numP[eB] = numP;
			m_bNode_numR[eB] = numR;
		}
	}
	return eG;
}


node DynamicSPQRForest::updateInsertedNode (edge eG, edge fG)
{
	node vB = bcproper(eG);
	if (m_bNode_SPQR[vB]) {
		node uG = updateInsertedNodeSPQR(vB,eG,fG);
		m_gNode_isMarked[uG] = false;
		edge fH = m_gEdge_hEdge[fG];
		m_bNode_hEdges[vB].pushBack(fH);
		m_hEdge_bNode[fH] = vB;
		m_hNode_bNode[fH->source()] = vB;
		m_bNode_numNodes[vB]++;
		return uG;
	}
	return DynamicBCTree::updateInsertedNode(eG,fG);
}


}
