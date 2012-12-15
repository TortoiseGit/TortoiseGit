/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements planar orthogonal drawing algorithm for
//   cluster graphs.
 *
 * \author Carsten Gutwenger, Sebastian Leipert, Karsten Klein
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


#include <ogdf/cluster/ClusterOrthoLayout.h>
#include <ogdf/cluster/CconnectClusterPlanarEmbed.h>


#include <ogdf/orthogonal/LongestPathCompaction.h>
#include <ogdf/orthogonal/FlowCompaction.h>
#include <ogdf/orthogonal/EdgeRouter.h>
#include <ogdf/internal/orthogonal/RoutingChannel.h>
#include <ogdf/orthogonal/MinimumEdgeDistances.h>
#include <ogdf/cluster/ClusterOrthoShaper.h>


namespace ogdf {


ClusterOrthoLayout::ClusterOrthoLayout()
{
	//drawing object distances
	m_separation = 40.0;
	m_cOverhang  = 0.2;
	m_margin     = 40.0;
	//preferred hierarchy direction is odNorth, but we use odSouth since gml's are flipped!
	m_preferedDir = odSouth;
	m_optionProfile = 0;
	//edge costs
	m_costAssoc   = 1;
	m_costGen     = 4;
	//align hierarchy nodes on same level
	m_align = false;
	//scale layout while improving it during compaction
	m_useScalingCompaction = false;
	m_scalingSteps = 6;

	m_orthoStyle = 0; //traditional 0, progressive 1
}


/**--------------------------------------
calling function without non-planar edges
*/
void ClusterOrthoLayout::call(ClusterPlanRep &PG,
	adjEntry adjExternal,
	Layout &drawing)
{
	List<NodePair> npEdges; //is empty
	List<edge> newEdges;    //is empty
	Graph G;
	call(PG, adjExternal, drawing, npEdges, newEdges, G);
}//call c-planar

/**---------------------------------------------------
calling function taking the planar representation, the
external face (adjentry), the layout to be filled,
a list of non-planar edges, a list of inserted edges
and the original graph as input
*/
void ClusterOrthoLayout::call(ClusterPlanRep &PG,
	adjEntry adjExternal,
	Layout &drawing,
	List<NodePair>& npEdges,
	List<edge>& newEdges,
	Graph& originalGraph)
{
	// We don't care about UML hierarchies and therefore do not allow alignment
	OGDF_ASSERT(!m_align);

	// if we have only one vertex in PG ...
	if(PG.numberOfNodes() == 1) {
		node v1 = PG.firstNode();
		node vOrig = PG.original(v1);
		double w = PG.widthOrig(vOrig);
		double h = PG.heightOrig(vOrig);

		drawing.x(v1) = m_margin + w/2;
		drawing.y(v1) = m_margin + h/2;
		m_boundingBox = DPoint(w + 2*m_margin, h + 2*m_margin);
		return;
	}

	OGDF_ASSERT(PG.representsCombEmbedding())
	//-------------------------
	// insert cluster boundaries
	PG.ModelBoundaries();
	OGDF_ASSERT(PG.representsCombEmbedding())


	//--------------------------
	// insert non-planar edges
	CombinatorialEmbedding* CE = new CombinatorialEmbedding(PG);
	if (!npEdges.empty())
	{
		CPlanarEdgeInserter CEI;
		CEI.call(PG, *CE, originalGraph, npEdges, newEdges);
	}//if

	//------------------------------------------------------------
	// now we set the external face, currently to the largest face
	adjEntry extAdj = 0;
	int maximum = 0;
	edge e, eSucc;

	for(e = PG.firstEdge(); e; e = eSucc)
	{
		eSucc = e->succ();
		if ( PG.clusterOfEdge(e) == PG.getClusterGraph().rootCluster() )
		{
			int asSize = CE->rightFace(e->adjSource())->size();
			if ( asSize > maximum)
			{
				maximum = asSize;
				extAdj = e->adjSource();
			}
			int atSize = CE->rightFace(e->adjTarget())->size();
			if ( atSize > maximum)
			{
				maximum = atSize;
				extAdj = e->adjTarget();
			}

		}//if root edge

	}//for

	delete CE;

	//returns adjEntry in rootcluster
	adjExternal = extAdj;
	OGDF_ASSERT(adjExternal != 0);


	//----------------------------------------------------------
	//Compaction scaling: help node cages to pass by each other:
	//First, the layout is blown up and then shrunk again in several steps
	//We change the separation value and save the original value.
	double l_orsep = m_separation;
	if (m_useScalingCompaction)
	{
		double scaleFactor = double(int(1 << m_scalingSteps));
		m_separation = scaleFactor*m_separation; //reduce this step by step in compaction
	}//if scaling

	//***********************************
	// PHASE 1: determine orthogonal shape

	//-------------------------------------------------------
	// expand high-degree vertices and generalization mergers
	PG.expand();

	// get combinatorial embedding
	CombinatorialEmbedding E(PG);
	E.setExternalFace(E.rightFace(adjExternal));

	// orthogonal shape representation
	OrthoRep OR;

	ClusterOrthoShaper COF;

	//set some options
	COF.align(false); //cannot be used yet with clusters
	COF.traditional(m_orthoStyle > 0 ? false : true); //prefer 90/270 degree angles over 180/180
	//bend cost depends on cluster depths avoiding unnecessary "inner" bends
	COF.bendCostTopDown(ClusterOrthoShaper::topDownCost);

	// New Call
	//COF.call(PG,E,OR,2);
	// Original call without bend bounds(still valid)
	COF.call(PG, E, OR);

	String msg;
	OGDF_ASSERT(OR.check(msg))

	//******************************************************************
	// PHASE 2: construction of a feasible drawing of the expanded graph

	//---------------------------
	// expand low degree vertices
	PG.expandLowDegreeVertices(OR);

	OGDF_ASSERT(PG.representsCombEmbedding());

	//------------------
	// restore embedding
	E.computeFaces();
	E.setExternalFace(E.rightFace(adjExternal));

	OGDF_ASSERT(OR.check(msg))

	//----------
	//COMPACTION

	//--------------------------
	// apply constructive compaction heuristics
	OR.normalize();
	OR.dissect();

	OR.orientate(PG,m_preferedDir);

	OGDF_ASSERT(OR.check(msg))

	// compute cage information and routing channels
	OR.computeCageInfoUML(PG);
	//temporary grid layout
	GridLayoutMapped gridDrawing(PG,OR,m_separation,m_cOverhang,4);

	RoutingChannel<int> rcGrid(PG,gridDrawing.toGrid(m_separation),m_cOverhang);
	rcGrid.computeRoutingChannels(OR, m_align);

	node v;
	const OrthoRep::VertexInfoUML *pInfoExp;
	forall_nodes(v,PG) {
		pInfoExp = OR.cageInfo(v);

		if (pInfoExp) break;
	}

	FlowCompaction fca(0,m_costGen,m_costAssoc);
	fca.constructiveHeuristics(PG,OR,rcGrid,gridDrawing);

	OR.undissect(m_align);

	if (!m_align)  {OGDF_ASSERT(OR.check(msg))}

	//--------------------------
	//apply improvement compaction heuristics
	// call flow compaction on grid
	FlowCompaction fc(0,m_costGen,m_costAssoc);
	fc.align(m_align);
	fc.scalingSteps(m_scalingSteps);

	fc.improvementHeuristics(PG,OR,rcGrid,gridDrawing);

	if (m_align) OR.undissect(false);

	//**************************
	// PHASE 3: routing of edges

	OGDF_ASSERT(OR.check(msg) == true);

	EdgeRouter router;
	MinimumEdgeDistances<int> minDistGrid(PG, gridDrawing.toGrid(m_separation));
	//router.setOrSep(int(gridDrawing.toGrid(l_orsep))); //scaling test
	router.call(PG,OR,gridDrawing,E,rcGrid,minDistGrid, gridDrawing.width(),
		gridDrawing.height(), m_align);

	OGDF_ASSERT(OR.check(msg) == true);

	OR.orientate(pInfoExp->m_corner[odNorth],odNorth);

	//*******************************************************
	// PHASE 4: apply improvement compaction heuristics again

	// call flow compaction on grid
	fc.improvementHeuristics(PG, OR, minDistGrid, gridDrawing, int(gridDrawing.toGrid(l_orsep)));

	// re-map result
	gridDrawing.remap(drawing);

	//postProcess(PG);

	//--------------------------
	// collapse all expanded vertices by introducing a new node in the center
	// of each cage representing the original vertex
	PG.collapseVertices(OR,drawing);

	// finally set the bounding box
	computeBoundingBox(PG,drawing);

	//set the separation again to the input value
	m_separation = l_orsep;
}//call



// compute bounding box and move final drawing such that it is 0 aligned
// respecting margins
void ClusterOrthoLayout::computeBoundingBox(
	const ClusterPlanRep &PG,
	Layout &drawing)
{
	double minX, maxX, minY, maxY;

	minX = maxX = drawing.x(PG.firstNode());
	minY = maxY = drawing.y(PG.firstNode());

	node v;
	forall_nodes(v,PG)
	{
		double x = drawing.x(v);
		if (x < minX) minX = x;
		if (x > maxX) maxX = x;

		double y = drawing.y(v);
		if (y < minY) minY = y;
		if (y > maxY) maxY = y;
	}

	double deltaX = m_margin - minX;
	double deltaY = m_margin - minY;

	forall_nodes(v,PG)
	{
		drawing.x(v) += deltaX;
		drawing.y(v) += deltaY;
	}

	m_boundingBox = DPoint(maxX+deltaX+m_margin, maxY+deltaY+m_margin);
}


} // end namespace ogdf

