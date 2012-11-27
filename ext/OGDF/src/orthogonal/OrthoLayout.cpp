/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements planar orthogonal drawing algorithm for
 * mixed-upward embedded graphs
 *
 * \author Carsten Gutwenger, Sebastian Leipert
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


#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/orthogonal/LongestPathCompaction.h>
#include <ogdf/orthogonal/FlowCompaction.h>
#include <ogdf/orthogonal/EdgeRouter.h>
#include <ogdf/internal/orthogonal/RoutingChannel.h>
#include <ogdf/orthogonal/MinimumEdgeDistances.h>
#include <ogdf/orthogonal/OrthoShaper.h>


namespace ogdf {


OrthoLayout::OrthoLayout()
{
	//drawing object distances
	m_separation = 40.0;
	m_cOverhang  = 0.2;
	m_margin     = 40.0;
	//preferred hierarchy direction
	// SHOULD ACTUALLY BE odNorth, but we use odSouth since gml's are flipped!
	m_preferedDir = odSouth;
	m_optionProfile = 0;
	//edge costs
	m_costAssoc   = 1;//should be set by profile
	m_costGen     = 4;
	//align hierarchy nodes on same level
	m_align = false;
	//scale layout while improving it
	m_useScalingCompaction = false;
	m_scalingSteps = 0;
	m_bendBound = 2; //bounds number of bends per edge in ortho shaper

	m_orthoStyle = 0;//0; //traditional 0, progressive 1

}


void OrthoLayout::call(PlanRepUML &PG,
	adjEntry adjExternal,
	Layout &drawing)
{
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


	//classify brother-to-brother hierarchy edges to allow alignment
	if (m_align)
	{
		classifyEdges(PG, adjExternal);
	}//if align
	//compaction with scaling: help node cages to pass by each other
	double l_orsep = m_separation;
	if (m_useScalingCompaction)
	{
		m_scalingSteps = 6;
		double scaleFactor = double(int(1 << m_scalingSteps));
		m_separation = scaleFactor*m_separation; //reduce this step by step in compaction
	}//if scaling


	//***********************************
	// PHASE 1: determine orthogonal shape

	// expand high-degree vertices and generalization mergers
	PG.expand();

	//check preconditions, currently not necessary
	//assureDrawability(PG);

	// get combinatorial embedding
	CombinatorialEmbedding E(PG);
	E.setExternalFace(E.rightFace(adjExternal));

	// determine orthogonal shape
	OrthoRep OR;

	//OrthoFormerUML OF;
	OrthoShaper OFG;

	//set some options
	OFG.align(m_align);    //align brother objects on hierarchy levels
	OFG.traditional(m_orthoStyle > 0 ? false : true); //prefer 90/270 degree angles over 180/180

	// New Call
	OFG.setBendBound(m_bendBound);
	OFG.call(PG,E,OR);

	// remove face splitter
	edge e, eSucc;
	for(e = PG.firstEdge(); e; e = eSucc)
	{
		eSucc = e->succ();
		if(PG.faceSplitter(e)) {
			OR.angle(e->adjSource()->cyclicPred()) = 2;
			OR.angle(e->adjTarget()->cyclicPred()) = 2;
			PG.delEdge(e);
		}
	}

	//******************************************************************
	// PHASE 2: construction of a feasible drawing of the expanded graph

	// expand low degree vertices
	PG.expandLowDegreeVertices(OR);

	OGDF_ASSERT(PG.representsCombEmbedding());

	// restore embedding
	E.computeFaces();
	E.setExternalFace(E.rightFace(adjExternal));

	// apply constructive compaction heuristics

	OR.normalize();
	OR.dissect2(&PG); //OR.dissect();

	OR.orientate(PG,m_preferedDir);

	// compute cage information and routing channels
	OR.computeCageInfoUML(PG);

	// adjust value of cOverhang
	if(m_cOverhang < 0.05)
		m_cOverhang = 0.0;
	if(m_cOverhang > 0.5)
		m_cOverhang = 0.5;

	//temporary grid layout
	GridLayoutMapped gridDrawing(PG,OR,m_separation,m_cOverhang,2);

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

	// call flow compaction on grid
	FlowCompaction fc(0,m_costGen,m_costAssoc);
	fc.align(m_align);
	fc.scalingSteps(m_scalingSteps);

	fc.improvementHeuristics(PG,OR,rcGrid,gridDrawing);

	//remove alignment edges before edgerouter call because compaction
	//may do an unsplit at the nodes corners, which is impossible if
	//there are alignment edges attached
	if (m_align) OR.undissect(false);

	// PHASE 3: routing of edges
	//

	EdgeRouter router;
	MinimumEdgeDistances<int> minDistGrid(PG, gridDrawing.toGrid(m_separation));
	//router.setOrSep(int(gridDrawing.toGrid(l_orsep))); //scaling test
	router.call(PG,OR,gridDrawing,E,rcGrid,minDistGrid, gridDrawing.width(),
		gridDrawing.height(), m_align);


	String msg;
	OGDF_ASSERT(OR.check(msg) == true);

	OR.orientate(pInfoExp->m_corner[odNorth],odNorth);

	//*************************************************
	// PHASE 4: apply improvement compaction heuristics

	// call flow compaction on grid
	fc.improvementHeuristics(PG, OR, minDistGrid, gridDrawing, int(gridDrawing.toGrid(l_orsep)));


	// re-map result
	gridDrawing.remap(drawing);

	// collapse all expanded vertices by introducing a new node in the center
	// of each cage representing the original vertex
	PG.collapseVertices(OR,drawing);

	// finally set the bounding box
	computeBoundingBox(PG,drawing);

	m_separation = l_orsep;
}//call



//-----------------------------------------------------------------------------
//Helpers
//-----------------------------------------------------------------------------
void OrthoLayout::classifyEdges(PlanRepUML &PG, adjEntry &adjExternal)
{
	//classify brother-to-brother hierarchy edges to allow alignment
	//when shifting this to planrep, guarantee edge type correction in planarization
	//save external face entry

	//PG.classifyEdges
	//potential direct connection are all non-gen. edges that are alignUpward
	edge e, eSucc;
	for(e = PG.firstEdge(); e; e = eSucc)
	{
		eSucc = e->succ();
		if (PG.typeOf(e) != Graph::generalization)
		{
			adjEntry as = e->adjSource();
			node v = e->source();
			if ( (PG.alignUpward(as))
				&& (PG.typeOf(e->target()) != Graph::dummy)//TODO: crossings ?
				&& (PG.typeOf(v) != Graph::dummy)
				)
			{
				edge gen1, gen2;
				int stop = 0;
				adjEntry runAE = as->cyclicSucc();
				edge run = runAE->theEdge();
				while ( (stop < v->degree()) &&  //only once
					((PG.typeOf(run) != Graph::generalization) || //search generalization
					(run->source() != v) //outgoing gen
					)
					)
				{
					stop++;
					runAE = runAE->cyclicSucc();
					run = runAE->theEdge();
				}//while
				OGDF_ASSERT(stop <= v->degree());

				//now we have the outgoing generalization (to the merger) at v
				gen1 = run;

				node w = e->target(); //crossings ?
				adjEntry asTwin = as->twin();

				stop = 0;
				runAE = asTwin->cyclicSucc();
				run = runAE->theEdge();
				while ( (stop < w->degree()) &&
					((PG.typeOf(run) != Graph::generalization) ||
					(run->source() != w)
					)
					)
				{
					stop++;
					runAE = runAE->cyclicSucc();
					run = runAE->theEdge();
				}//while
				OGDF_ASSERT(stop <= w->degree());

				//now we have the outgoing generalization (to the merger) at w
				gen2 = run;

				//two possible orientations
				//left to right
				bool ltr = ( gen1->adjSource()->faceCycleSucc() == gen2->adjTarget() );
				//right to left
				bool rtl = ( gen2->adjSource()->faceCycleSucc() == gen1->adjTarget() );
				if (ltr || rtl) //should be disjoint cases because of merger node
				{
					PG.setBrother(e);

					//now check if the embedding does include unnecessary nodes in the slope
					if (ltr)
					{
						//there are edges between e and gen2 at target
						if (!(e->adjTarget()->faceCyclePred() == gen2->adjTarget()))
						{
							OGDF_ASSERT(v != e->target());
							PG.moveAdj(e->adjTarget(), before, gen2->adjTarget()->twin());
						}
						//there are edges between e and gen1 at source
						if (!(e->adjTarget()->faceCycleSucc() == gen1->adjSource()))
						{
							//test if we discard the outer face entry
							if (adjExternal == e->adjSource())
							{
								adjExternal = e->adjSource()->faceCyclePred();
							}
							PG.moveAdj(e->adjSource(), after, gen1->adjSource());
						}
					}//if gen 1 left of gen 2
					if (rtl)
					{
						//there are edges between e and gen2 at target
						if (!(e->adjSource()->faceCycleSucc() == gen2->adjSource()))
						{
							//test if we discard the outer face entry
							if (adjExternal == e->adjTarget())
							{
								adjExternal = e->adjTarget()->faceCycleSucc();
							}
							PG.moveAdj(e->adjTarget(), after, gen2->adjSource());
						}
						//there are edges between e and gen1 at source
						if (!(e->adjSource()->faceCyclePred() == gen1->adjTarget()))
						{
							PG.moveAdj(e->adjSource(), before, gen1->adjSource());
						}

					}//if gen 2 left of gen 1
				}//if
				else PG.setHalfBrother(e);

			}//if upward edge
		}//if not generalization
	}//for
}//classifyedges



// compute bounding box and move final drawing such that it is 0 aligned
// respecting margins
void OrthoLayout::computeBoundingBox(
	const PlanRepUML &PG,
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

