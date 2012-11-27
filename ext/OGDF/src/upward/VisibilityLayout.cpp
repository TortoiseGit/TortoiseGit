/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of visibility layout algorithm.
 *
 * \author Hoi-Ming Wong and Carsten Gutwenger
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

#include <ogdf/upward/VisibilityLayout.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/FaceArray.h>
#include <math.h>

namespace ogdf {


void VisibilityLayout::call(GraphAttributes &GA)
{
	if (GA.constGraph().numberOfNodes() <= 1)
		return;

	//call upward planarizer
	UpwardPlanRep UPR;
	UPR.createEmpty(GA.constGraph());
	m_upPlanarizer.get().call(UPR);
	layout(GA, UPR);
}


void VisibilityLayout::layout(GraphAttributes &GA, const UpwardPlanRep &UPROrig)
{
	UpwardPlanRep UPR = UPROrig;

	//clear some data
	edge e;
	forall_edges(e, GA.constGraph()) {
		GA.bends(e).clear();
	}

	int minGridDist = 1;
	node v;
	forall_nodes(v, GA.constGraph()) {
		if (minGridDist < max(GA.height(v), GA.width(v)))
			minGridDist = (int) max(GA.height(v), GA.width(v));
	}
	minGridDist = max(minGridDist*2+1, m_grid_dist);

	CombinatorialEmbedding &gamma = UPR.getEmbedding();
	//add edge (s,t)
	adjEntry adjSrc = 0;
	forall_adj(adjSrc, UPR.getSuperSource()) {
		if (gamma.rightFace(adjSrc) == gamma.externalFace())
			break;
	}

	OGDF_ASSERT(adjSrc != 0);

	edge e_st = UPR.newEdge(adjSrc, UPR.getSuperSink()); // on the right
	gamma.computeFaces();
	gamma.setExternalFace(gamma.rightFace(e_st->adjSource()));

	constructVisibilityRepresentation(UPR);

	// the preliminary postion
	NodeArray<int> xPos(UPR);
	NodeArray<int> yPos(UPR);

	// node Position
	forall_nodes(v, UPR) {
		NodeSegment vVis = nodeToVis[v];
		int x = (int) (vVis.x_l + vVis.x_r)/2 ; // median positioning
		xPos[v] = x;
		yPos[v] = vVis.y;

		if (UPR.original(v) != 0) {
			node vOrig = UPR.original(v);
			//final position
			GA.x(vOrig) = x * minGridDist;
			GA.y(vOrig)	= vVis.y * minGridDist;
		}
	}

	//compute bendpoints
	forall_edges(e, GA.constGraph()) {
		List<edge> chain = UPR.chain(e);
		forall_listiterators(edge, it, chain) {
			edge eUPR = *it;
			EdgeSegment eVis = edgeToVis[eUPR];
			if (chain.size() == 1) {
				if ((yPos[eUPR->target()] - yPos[eUPR->source()]) > 1) {
					DPoint p1(eVis.x*minGridDist, (yPos[eUPR->source()]+1)*minGridDist);
					DPoint p2(eVis.x*minGridDist, (yPos[eUPR->target()]-1)*minGridDist);
					GA.bends(e).pushBack(p1);
					if (yPos[eUPR->source()]+1 != yPos[eUPR->target()]-1)
						GA.bends(e).pushBack(p2);
				}
			}
			else {
				//short edge
				if ((yPos[eUPR->target()] - yPos[eUPR->source()]) == 1) {
					if (UPR.original(eUPR->target()) == 0) {
						node tgtUPR = eUPR->target();
						DPoint p(xPos[tgtUPR]*minGridDist, yPos[tgtUPR]*minGridDist);
						GA.bends(e).pushBack(p);
					}
				}
				//long edge
				else {
					DPoint p1(eVis.x*minGridDist, (yPos[eUPR->source()]+1)*minGridDist);
					DPoint p2(eVis.x*minGridDist, (yPos[eUPR->target()]-1)*minGridDist);
					GA.bends(e).pushBack(p1);
					if (yPos[eUPR->source()]+1 != yPos[eUPR->target()]-1)
						GA.bends(e).pushBack(p2);
					if (UPR.original(eUPR->target()) == 0) {
						node tgtUPR = eUPR->target();
						DPoint p(xPos[tgtUPR]*minGridDist, yPos[tgtUPR]*minGridDist);
						GA.bends(e).pushBack(p);
					}
				}
			}
		}

		DPolyline &poly = GA.bends(e);
		DPoint pSrc(GA.x(e->source()), GA.y(e->source()));
		DPoint pTgt(GA.x(e->target()), GA.y(e->target()));
		poly.normalize(pSrc, pTgt);
	}//forall_edges
}


void VisibilityLayout::constructDualGraph(UpwardPlanRep &UPR)
{
	CombinatorialEmbedding &gamma = UPR.getEmbedding();

	faceToNode.init(gamma, 0);
	leftFace_node.init(UPR, 0);
	rightFace_node.init(UPR, 0);
	leftFace_edge.init(UPR, 0);
	rightFace_edge.init(UPR, 0);

	//construct a node for each face f
	face f;
	forall_faces(f, gamma) {
		faceToNode[f] = D.newNode();

		if (f == gamma.externalFace())
			s_D = faceToNode[f] ;

		//compute face switches
		node s, t;
		adjEntry adj;
		forall_face_adj(adj, f) {
			adjEntry adjNext = adj->faceCycleSucc();
			if (adjNext->theEdge()->source() == adj->theEdge()->source())
				s = adjNext->theEdge()->source();
			if (adjNext->theEdge()->target() == adj->theEdge()->target())
				t = adjNext->theEdge()->target();
		}

		//compute left and right face
		bool passSource = false;
		if (f == gamma.externalFace()) {
			adj = UPR.getSuperSink()->firstAdj();
			if (gamma.rightFace(adj) != gamma.externalFace())
				adj = adj->cyclicSucc();
		}
		else
			adj = UPR.getAdjEntry(gamma, t, f);

		adjEntry adjBegin = adj;
		do {
			node v = adj->theEdge()->source();
			if (!passSource) {
				if (v != s)
					leftFace_node[v] = f;
				leftFace_edge[adj->theEdge()] = f;
			}
			else {
				if (v != s)
					rightFace_node[v] = f;
				rightFace_edge[adj->theEdge()] = f;
			}
			if (adj->theEdge()->source() == s)
				passSource = true;
			adj = adj->faceCycleSucc();
		} while(adj != adjBegin);
	}
	t_D = D.newNode(); // the second (right) node associated with the external face

	//construct dual edges
	edge e;
	forall_edges(e, UPR) {
		face f_r = rightFace_edge[e];
		face f_l = leftFace_edge[e];
		node u = faceToNode[f_l];
		node v = faceToNode[f_r];
		if (f_r == gamma.externalFace() || f_r == f_l)
			D.newEdge(u, t_D);
		else
			D.newEdge(u,v);
	}

	OGDF_ASSERT(isConnected(D));
}


void VisibilityLayout::constructVisibilityRepresentation(UpwardPlanRep &UPR)
{
	constructDualGraph(UPR);
	//makeSimple(D);
	//if (t_D->degree() <= 1)
	//	D.newEdge(s_D, t_D); // make biconnected


	//OGDF_ASSERT(isSimple(UPR));
	//OGDF_ASSERT(isBiconnected(UPR));
	//OGDF_ASSERT(isSimple(D));
	//OGDF_ASSERT(isBiconnected(D));

	//compute top. numbering
	NodeArray<int> topNumberUPR(UPR);
	NodeArray<int> topNumberD(D);

	topologicalNumbering(UPR, topNumberUPR);
	topologicalNumbering(D, topNumberD);

	nodeToVis.init(UPR);
	edgeToVis.init(UPR);

	node v;
	forall_nodes(v, UPR) {
		NodeSegment vVis;

		//cout << "node : " << v << " stNum: " << topNumberUPR[v] << endl;

		if (v == UPR.getSuperSource() || v == UPR.getSuperSink()) {
			vVis.y = topNumberUPR[v];
			vVis.x_l = topNumberD[s_D];
			vVis.x_r = topNumberD[t_D]-1;
			nodeToVis[v] =vVis;
			continue;
		}

		vVis.y = topNumberUPR[v];
		face f_v = leftFace_node[v];
		node vD = faceToNode[f_v];
		vVis.x_l = topNumberD[vD];
		f_v = rightFace_node[v];
		vD = faceToNode[f_v];
		vVis.x_r = topNumberD[vD]-1;
		nodeToVis[v] =vVis;
	}

	edge e;
	forall_edges(e, UPR) {
		EdgeSegment eVis;
		face f_v = leftFace_edge[e];
		node vD = faceToNode[f_v];
		eVis.x = topNumberD[vD];
		eVis.y_b = topNumberUPR[e->source()];
		eVis.y_t = topNumberUPR[e->target()];
		edgeToVis[e] = eVis;
	}
}

}// namespace
