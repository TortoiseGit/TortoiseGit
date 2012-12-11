/*
* $Revision: 2616 $
*
* last checkin:
*   $Author: gutwenger $
*   $Date: 2012-07-16 15:34:43 +0200 (Mo, 16. Jul 2012) $
***************************************************************/

/** \file
 * \brief implementation of class PlanarizationLayout.
 *
 * applies planarization approach for drawing UML diagrams
 * by calling a planar layouter for every planarized connected
 * component
 * Static and incremental calls available
 * Replaces cliques (if m_processCliques is set) to speed up
 * the computation, this does only work in non-UML mode
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


#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/FastPlanarSubgraph.h>
#include <ogdf/planarity/FixedEmbeddingInserter.h>
#include <ogdf/planarity/SimpleEmbedder.h>
#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/TopologyModule.h>
#include <ogdf/basic/precondition.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/graphalg/CliqueFinder.h>


namespace ogdf {

//Constructor:
//set default values for parameters and set planar layouter,
//planarization modules
PlanarizationLayout::PlanarizationLayout()
{
	//modules
	m_subgraph.set(new FastPlanarSubgraph);
	m_inserter.set(new FixedEmbeddingInserter);
	m_planarLayouter.set(new OrthoLayout);
	m_packer.set(new TileToRowsCCPacker);
	m_embedder.set(new SimpleEmbedder);

	//parameters
	m_pageRatio      = 1.0;
	m_processCliques = false; //do not search for cliques, only if not uml
	m_cliqueSize     = 10;
	m_fakeTree       = true; //change edge type from generalization to association
}


void PlanarizationLayout::reembed(PlanRepUML &PG, int ccNumber, bool l_align,
	bool l_gensExist)
{
	//TODO: update by reinitialization?
	//PG.initActiveCC(i);
	//first we remove all inserted crossings
	node v;
	List<node> crossings;
	forall_nodes(v, PG)
	{
		if (PG.isCrossingType(v))
		{
			crossings.pushBack(v);
		}
	}//forallnodes
	ListIterator<node> it = crossings.begin();
	while (it.valid())
	{
		PG.removeCrossing((*it));
		it++;
	}//while
	//***************************************
	// first phase: Compute a planar subgraph
	//***************************************

	// The planar subgraph should contain as many generalizations
	// as possible, hence we put all generalizations into the list
	// preferedEdges.
	List<edge> preferedEdges;
	edge e;
	EdgeArray<int> costOrig(PG.original(), 1);
	forall_edges(e,PG)
	{
		if (PG.typeOf(e) == Graph::generalization)
		{
			if (l_align) l_gensExist = true;
			preferedEdges.pushBack(e);
			edge ori = PG.original(e);
			//high cost to allow alignment without crossings
			if ( (l_align) &&
				((ori && (PG.typeOf(e->target()) == Graph::generalizationMerger))
				|| (PG.alignUpward(e->adjSource()))
				)
				)
				costOrig[ori] = 10;

		}//generalization
	}//foralledges

	List<edge> deletedEdges;
	m_subgraph.get().callAndDelete(PG, preferedEdges, deletedEdges);


	//**************************************
	// second phase: Re-insert deleted edges
	//**************************************

	m_inserter.get().callForbidCrossingGens(PG, costOrig, deletedEdges);

	OGDF_ASSERT(isPlanar(PG));


	//
	// determine embedding of PG
	//

	// We currently compute any embedding and choose the maximal face
	// as external face

	// if we use FixedEmbeddingInserter, we have to re-use the computed
	// embedding, otherwise crossing nodes can turn into "touching points"
	// of edges (alternatively, we could compute a new embedding and
	// finally "remove" such unnecessary crossings).
	if(!PG.representsCombEmbedding())
		planarEmbed(PG);


	// CG: This code does not do anything...
	//adjEntry adjExternal = 0;

	//if(PG.numberOfEdges() > 0)
	//{
	//	CombinatorialEmbedding E(PG);
	//	//face fExternal = E.maximalFace();
	//	face fExternal = findBestExternalFace(PG,E);
	//	adjExternal = fExternal->firstAdj();
	//	//while (PG.sinkConnect(adjExternal->theEdge()))
	//	//	adjExternal = adjExternal->faceCycleSucc();
	//}
}//reembed


//---------------------------------------------------------
//compute a layout with the given embedding, take special
//care of  crossings (assumes that all crossings are non-ambiguous
//and can be derived from node/bend positions)
//---------------------------------------------------------
void PlanarizationLayout::callFixEmbed(UMLGraph &umlGraph)
{
	m_nCrossings = 0;

	if(((const Graph &)umlGraph).empty())
		return;

	//check necessary preconditions
	preProcess(umlGraph);

	int l_layoutOptions = m_planarLayouter.get().getOptions();
	bool l_align = ((l_layoutOptions & umlOpAlign)>0);

	//--------------------------------------------------------
	//first, we sort all edges around each node corresponding
	//to the given layout in umlGraph
	//then we insert the mergers
	//try to rebuild this: insert mergers only in copy
	bool umlMerge = false; //Standard: false
	int i;

	//*********************************************************
	// first phase: Compute planarized representation from input
	//*********************************************************

	//now we derive a planar representation of the input
	//graph using its layout information
	PlanRepUML PG(umlGraph);

	const int numCC = PG.numberOfCCs();

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);


	//******************************************
	//now planarize CCs and apply drawing module
	for(i = 0; i < numCC; ++i)
	{
		// we treat connected component i
		// PG is set to a copy of this CC

		PG.initCC(i);

		int nOrigVerticesPG = PG.numberOfNodes();

		//alignment: check wether gens exist, special treatment is necessary
		bool l_gensExist = false; //set this for all CC's, start with first gen,
		//this setting can be mixed among CC's without problems

		//*********************************************************
		// we don't need to compute a planar subgraph, because we
		// use the given embedding
		//*********************************************************

		adjEntry adjExternal = 0;

		//here lies the main difference to the other call
		//we first planarize by using the given layout and then
		//set the embedding corresponding to the input
		bool embedded;
		TopologyModule TM;
		try {
			embedded = TM.setEmbeddingFromGraph(PG, umlGraph, adjExternal, umlMerge);
		}//try
		catch (...)
		{
			//TODO: check for graph changes that are not undone
			embedded = false;
		}


		//-------------------------------------------------
		//if not embedded correctly
		if (!embedded)
		{
			reembed(PG, i, l_align, l_gensExist);
		}//if !embedded

		//-------------------------------------------------

		CombinatorialEmbedding E(PG);

		if (!umlMerge)
			PG.setupIncremental(i, E);

		//
		// determine embedding of PG
		//

		// We currently compute any embedding and choose the maximal face
		// as external face

		// if we use FixedEmbeddingInserter, we have to re-use the computed
		// embedding, otherwise crossing nodes can turn into "touching points"
		// of edges (alternatively, we could compute a new embedding and
		// finally "remove" such unnecessary crossings).

		if((adjExternal == 0) && PG.numberOfEdges() > 0)
		{
			//face fExternal = E.maximalFace();
			face fExternal = findBestExternalFace(PG,E);
			adjExternal = fExternal->firstAdj();
			//while (PG.sinkConnect(adjExternal->theEdge()))
			//	adjExternal = adjExternal->faceCycleSucc();
		}

		m_nCrossings += PG.numberOfNodes() - nOrigVerticesPG;


		//*********************************************************
		// third phase: Compute layout of planarized representation
		//*********************************************************

		Layout drawing(PG);

		//distinguish between CC's with/without generalizations
		//this changes the input layout modules options!
		if (l_gensExist)
			m_planarLayouter.get().setOptions(l_layoutOptions);
		else m_planarLayouter.get().setOptions((l_layoutOptions & ~umlOpAlign));

		//***************************************
		//call the Layouter for the CC's UMLGraph
		m_planarLayouter.get().call(PG,adjExternal,drawing);

		// copy layout into umlGraph
		// Later, we move nodes and edges in each connected component, such
		// that no two overlap.
		const List<node> &origInCC = PG.nodesInCC(i);
		ListConstIterator<node> itV;

		//set position for original nodes and set bends for
		//all edges
		for(itV = origInCC.begin(); itV.valid(); ++itV)
		{
			node vG = *itV;

			umlGraph.x(vG) = drawing.x(PG.copy(vG));
			umlGraph.y(vG) = drawing.y(PG.copy(vG));

			adjEntry adj;
			forall_adj(adj,vG)
			{
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				drawing.computePolylineClear(PG,eG,umlGraph.bends(eG));
			}//foralladj
		}//for orig nodes

		if (!umlMerge)
		{
			//insert bend point for incremental mergers
			const SList<node>& mergers = PG.incrementalMergers(i);
			SListConstIterator<node> itMerger = mergers.begin();
			while (itMerger.valid())
			{
				node vMerger = (*itMerger);
				//due to the fact that the merger may be expanded, we are
				//forced to run through the face
				adjEntry adjMerger = PG.expandAdj(vMerger);
				//check if there is an expansion face
				if (adjMerger != 0)
				{
					//we run through the expansion face to the connected edges
					//this edge is to dummy corner
					adjEntry runAdj = adjMerger->faceCycleSucc();
					while (runAdj != adjMerger)
					{
						node vConnect = runAdj->theNode();
						//because of the node collapse using the original
						//edges instead of the merger copy edges (should be
						//fixed for incremental mode) the  degree is 4
						//if (vConnect->degree() != 3)
						if (vConnect->degree() != 4)
						{
							runAdj = runAdj->faceCycleSucc();
							continue;
						}
						edge eCopy = runAdj->cyclicPred()->theEdge();
						OGDF_ASSERT(eCopy->target() == runAdj->theNode())
						OGDF_ASSERT(PG.isGeneralization(eCopy))
						OGDF_ASSERT(PG.original(eCopy))
						umlGraph.bends(PG.original(eCopy)).pushBack(
						DPoint(drawing.x(vMerger), drawing.y(vMerger)));
						runAdj = runAdj->faceCycleSucc();

					}

				}
				else //currently all nodes are expanded, but this is not guaranteed
				{
					forall_adj(adjMerger, vMerger)
					{
						if (adjMerger->theEdge()->target() == vMerger)
						{
							edge eOrig = PG.original(adjMerger->theEdge());
							if (eOrig)
								//incoming merger edges always have an original here!
								umlGraph.bends(eOrig).pushBack(DPoint(drawing.x(vMerger),
								drawing.y(vMerger)));

						}
					}//forall adj
				}
				itMerger++;
			}//while merger nodes
		}

		// the width/height of the layout has been computed by the planar
		// layout algorithm; required as input to packing algorithm
		boundingBox[i] = m_planarLayouter.get().getBoundingBox();
	}//for cc's

	postProcess(umlGraph);

	//----------------------------------------
	// Arrange layouts of connected components
	//----------------------------------------

	arrangeCCs(PG, umlGraph, boundingBox);

	umlGraph.undoGenMergers();
	umlGraph.removeUnnecessaryBendsHV();
}//callfixembed



//-----------------------------------------------------------------------------
//call function: compute an UML layout for graph umlGraph
//-----------------------------------------------------------------------------
void PlanarizationLayout::call(UMLGraph &umlGraph)
{
	m_nCrossings = 0;

	if(((const Graph &)umlGraph).empty())
		return;

	//check necessary preconditions
	preProcess(umlGraph);

	//---------------------------------------------------
	// preprocessing: insert a merger for generalizations
	umlGraph.insertGenMergers();

	PlanRepUML PG(umlGraph);
	const int numCC = PG.numberOfCCs();

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);


	//alignment section (should not be here, because planarlayout should
	//not know about the meaning of layouter options and should not cope
	//with them), move later
	//we have to distinguish between cc's with and without generalizations
	//if the alignment option is set
	int l_layoutOptions = m_planarLayouter.get().getOptions();
	bool l_align = ((l_layoutOptions & umlOpAlign)>0);
	//end alignment section

	//------------------------------------------
	//now planarize CCs and apply drawing module
	int i;
	for(i = 0; i < numCC; ++i)
	{
		// we treat connected component i
		// PG is set to a copy of this CC
		PG.initCC(i);

		int nOrigVerticesPG = PG.numberOfNodes();

		//alignment: check wether gens exist, special treatment is necessary
		bool l_gensExist = false; //set this for all CC's, start with first gen,
		//this setting can be mixed among CC's without problems

		//---------------------------------------
		// first phase: Compute a planar subgraph
		//---------------------------------------

		// The planar subgraph should contain as many generalizations
		// as possible, hence we put all generalizations into the list
		// preferredEdges.
		// In the case of clique processing we have to prevent the
		// clique replacement (star) edges from being crossed.
		// We do not allow that clique replacement is done in the case
		// of UML diagrams, therefore we can temporarily set the type of
		// all deleted and replacement edges to generalization to
		// avoid crossings
		EdgeArray<Graph::EdgeType> savedType(PG);
		EdgeArray<Graph::EdgeType> savedOrigType(PG.original()); //for deleted copies

		List<edge> preferedEdges;
		edge e;
		EdgeArray<int> costOrig(PG.original(), 1);
		//edgearray for reinserter call: which edge may never be crossed?
		EdgeArray<bool> noCrossingEdge(PG.original(), false);
		forall_edges(e,PG)
		{
			edge ori = PG.original(e);
			if (m_processCliques)
			{
				savedType[e] = PG.typeOf(e);
				if (ori)
				{
					if (umlGraph.isReplacement(ori))
					{
						preferedEdges.pushBack(e);
						costOrig[ori] = 10;
						PG.setGeneralization(e);
						noCrossingEdge[ori] = true;
						continue;
					}//if clique replacement

				}//if
			}//if cliques

			if (PG.typeOf(e) == Graph::generalization)
			{
				if (l_align) l_gensExist = true;
				OGDF_ASSERT(!ori || !(noCrossingEdge[ori]));
				preferedEdges.pushBack(e);

				//high cost to allow alignment without crossings
				if (l_align && (
					(ori && (PG.typeOf(e->target()) == Graph::generalizationMerger))
						|| PG.alignUpward(e->adjSource())
					))
				 costOrig[ori] = 10;

			}//generalization
		}//foralledges

		List<edge> deletedEdges;
		m_subgraph.get().callAndDelete(PG, preferedEdges, deletedEdges);

		if (m_processCliques)
		{
			ListIterator<edge> itEdge = deletedEdges.begin();
			while (itEdge.valid())
			{
				savedOrigType[*itEdge] = PG.typeOrig(*itEdge);
				umlGraph.type(*itEdge) = Graph::generalization;

				OGDF_ASSERT(!(umlGraph.isReplacement(*itEdge)))

				itEdge++;
			}//while
		}//if cliques

		//--------------------------------------
		// second phase: Re-insert deleted edges
		//--------------------------------------

		if (m_processCliques)
			m_inserter.get().call(PG, costOrig, noCrossingEdge, deletedEdges);
		else
			m_inserter.get().callForbidCrossingGens(PG, costOrig, deletedEdges);

		//reset the changed edge types
		if (m_processCliques)
		{
			forall_edges(e, PG)
			{
				//if replacement
				edge ori = PG.original(e);
				if (ori)
					if (umlGraph.isReplacement(ori))
					{
						OGDF_ASSERT(PG.chain(ori).size() == 1)
						PG.setType(e, savedType[e]);
						umlGraph.type(ori) = Graph::association;
					}
			}//foralledges
			//set the type of the reinserted edges in original and copy
			ListIterator<edge> itEdge = deletedEdges.begin();
			while (itEdge.valid())
			{
				umlGraph.type(*itEdge) = savedOrigType[*itEdge];
				const List<edge> &le = PG.chain(*itEdge);
				ListConstIterator<edge> it2 = le.begin();
				while (it2.valid())
				{
					PG.setType(*it2, savedOrigType[*itEdge]);
					it2++;
				}//while
				itEdge++;
			}//while
		}//if cliques

		//-----------------------------------------------------------------
		//additional check for clique processing
		//guarantee that there is no crossing in replacement, otherwise
		//we can not cluster a star
		//fileName.sprintf("planar2.gml");

		//if (m_processCliques)
		//{
		//	SListConstIterator<node> itDV = umlGraph.centerNodes().begin();
		//	while (itDV.valid())
		//	{
		//		node cent = PG.copy(*itDV);
		//		OGDF_ASSERT(cent->degree() == (*itDV)->degree())
		//			adjEntry adj;
		//		forall_adj(adj, cent)
		//		{
		//			OGDF_ASSERT(PG.original(adj->twinNode()))
		//				OGDF_ASSERT(!(PG.isGeneralization(adj->theEdge())))
		//				PG.setBrother(adj->theEdge());//only output coloring

		//		}//foralladj
		//		itDV++;
		//	}//while
		//	Layout fakeDrawing(PG);
		//	PG.writeGML(fileName, fakeDrawing);
		//}//if clique replacement
		//----------------------------------------------------------


		//
		// determine embedding of PG
		//

		// We currently compute any embedding and choose the maximal face
		// as external face

		// if we use FixedEmbeddingInserter, we have to re-use the computed
		// embedding, otherwise crossing nodes can turn into "touching points"
		// of edges (alternatively, we could compute a new embedding and
		// finally "remove" such unnecessary crossings).
		if(!PG.representsCombEmbedding())
			planarEmbed(PG);
		adjEntry adjExternal = 0;

		if(PG.numberOfEdges() > 0)
		{
			CombinatorialEmbedding E(PG);
			face fExternal = findBestExternalFace(PG,E);
			adjExternal = fExternal->firstAdj();
		}

		m_nCrossings += PG.numberOfNodes() - nOrigVerticesPG;


		//---------------------------------------------------------
		// third phase: Compute layout of planarized representation
		//---------------------------------------------------------

		if (m_processCliques)
		{
			//insert boundaries around clique representation node
			//and compute a representation layout for the cliques
			//(is needed to guarantee the size of the replacement
			//boundary)
			//conserve external face information
			SListPure<node> centerNodes = umlGraph.centerNodes();
			SListIterator<node> itNode = centerNodes.begin();
			while (itNode.valid())
			{
				PG.insertBoundary(*itNode, adjExternal);
				itNode++;
			}

		}//if cliques

		Layout drawing(PG);

		//distinguish between CC's with/without generalizations
		//this changes the input layout modules options!
		if (l_gensExist)
			m_planarLayouter.get().setOptions(l_layoutOptions);
		else m_planarLayouter.get().setOptions((l_layoutOptions & ~umlOpAlign));

		//***************************************
		//call the Layouter for the CC's UMLGraph
		m_planarLayouter.get().call(PG,adjExternal,drawing);

		//--------------------------------------
		//we now have to reposition clique nodes
		if (m_processCliques)
		{
			//--------------------------------------------------------
			//first, we derive the current size of the boundary around
			//the center nodes, then we position the clique nodes
			//in a circular fashion in the boundary

			//the node array is only used for the simple anchor move strategy at the
			//end of this if and can later be removed
			NodeArray<bool> isClique(PG, false);

			SListPure<node> centerNodes = umlGraph.centerNodes();
			SListIterator<node> itNode = centerNodes.begin();
			while (itNode.valid())
			{
				node centerNode = (*itNode);
				adjEntry adjBoundary = PG.boundaryAdj(centerNode);
				//-----------------------------------------------------
				//derive the boundary size
				//if the boundary does not exist (connected component is clique), we
				//only run over the nodes adjacent to centerNode
				double minx = DBL_MAX, maxx = -DBL_MAX, miny = DBL_MAX, maxy = -DBL_MAX;
				if (adjBoundary)
				{
					adjEntry adjRunner = adjBoundary;
					//explore the dimension and position of the boundary rectangle
					//TODO: guarantee (edge types?) that we run around a boundary
					do {
						double vx = drawing.x(adjRunner->theNode());
						double vy = drawing.y(adjRunner->theNode());
						if (vx < minx) minx = vx;
						if (vx > maxx) maxx = vx;
						if (vy < miny) miny = vy;
						if (vy > maxy) maxy = vy;

						//are we at a bend or a crossing?
						OGDF_ASSERT((adjRunner->twinNode()->degree() == 2) ||
							(adjRunner->twinNode()->degree() == 4))
							//bend
						if (adjRunner->twinNode()->degree() < 4)
							adjRunner = adjRunner->faceCycleSucc();
						else adjRunner = adjRunner->faceCycleSucc()->cyclicPred();
					} while (adjRunner != adjBoundary);
				}//if boundary exists
				else
				{
					forall_adj(adjBoundary, centerNode)
					{
						node w = adjBoundary->twinNode();
						double vx = drawing.x(PG.copy(w));
						double vy = drawing.y(PG.copy(w));
						if (vx < minx) minx = vx;
						if (vx > maxx) maxx = vx;
						if (vy < miny) miny = vy;
						if (vy > maxy) maxy = vy;
					}
				}//else
				//-----------------------------------------------------------

				//we now have to arrange the nodes on a circle with positions
				//that respect the position within the given rectangle, i.e.
				//the node with highest position should be on top etc. . This
				//helps in avoiding unnecessary crossings of outgoing edges
				//with the clique circle and unnecessary long edges to the anchors
				//Note that the ordering around centerNode in umlGraph is different
				//to the ordering in the drawing (defined on PG)
				//recompute size of clique and the node positions
				//test
				//---------------------------------------------------------
				//derive the ordering of the nodes around centerNode in the
				//planarized copy
				List<node> adjNodes;
				fillAdjNodes(adjNodes, PG, centerNode, isClique, drawing);

				//-----------------------------
				//compute clique node positions
				umlGraph.computeCliquePosition(adjNodes, centerNode,
					min(maxx-minx,maxy-miny));
				//testend

				double centralX = (maxx-minx)/2.0+minx;
				double centralY = (maxy-miny)/2.0+miny;
				double circleX  = umlGraph.cliqueRect(centerNode).width()/2.0;
				double circleY  = umlGraph.cliqueRect(centerNode).height()/2.0;

				//now we have the position and size of the rectangle around
				//the clique

				//assign shifted coordinates to drawing
				forall_adj(adjBoundary, centerNode)
				{
					node w = adjBoundary->twinNode();
					drawing.x(PG.copy(w)) = centralX-circleX+umlGraph.cliquePos(w).m_x;
					drawing.y(PG.copy(w)) = centralY-circleY+umlGraph.cliquePos(w).m_y;
				}

				itNode++;
			}//while

			//simple strategy to move anchor positions too (they are not needed:
			// move to same position)
			node w;
			forall_nodes(w, PG)
			{
				//forall clique nodes shift the anchor points
				if (isClique[w])
				{
					adjEntry adRun = w->firstAdj();
					do
					{
						node wOpp = adRun->twinNode();
						drawing.x(wOpp) = drawing.x(w);
						drawing.y(wOpp) = drawing.y(w);
						adRun = adRun->cyclicSucc();
					} while (adRun != w->firstAdj());

				}
			}

		}//if cliques

		// copy layout into umlGraph
		// Later, we move nodes and edges in each connected component, such
		// that no two overlap.
		const List<node> &origInCC = PG.nodesInCC(i);
		ListConstIterator<node> itV;

		for(itV = origInCC.begin(); itV.valid(); ++itV) {
			node vG = *itV;

			umlGraph.x(vG) = drawing.x(PG.copy(vG));
			umlGraph.y(vG) = drawing.y(PG.copy(vG));

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				drawing.computePolylineClear(PG,eG,umlGraph.bends(eG));
			}
		}

		// the width/height of the layout has been computed by the planar
		// layout algorithm; required as input to packing algorithm
		boundingBox[i] = m_planarLayouter.get().getBoundingBox();
	}//for cc's

	//----------------------------------------
	// Arrange layouts of connected components
	//----------------------------------------

	arrangeCCs(PG, umlGraph, boundingBox);

	umlGraph.undoGenMergers();
	umlGraph.removeUnnecessaryBendsHV();

	//new position after adding of cliqueprocess, check if correct
	postProcess(umlGraph);
}//call



//-----------------------------------------------------------------------------
//static call function: compute a layout for graph umlGraph without
//special UML or interactive features, clique processing etc.
//-----------------------------------------------------------------------------

void PlanarizationLayout::doSimpleCall(GraphAttributes *pGA)
{
	m_nCrossings = 0;

	if(pGA->constGraph().empty())
		return;

	PlanRepUML *pPG =  new PlanRepUML(*pGA);
	const int numCC = pPG->numberOfCCs();

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);

	//------------------------------------------
	//now planarize CCs and apply drawing module
	int i;
	for(i = 0; i < numCC; ++i)
	{
		// we treat connected component i
		// PG is set to a copy of this CC
		pPG->initCC(i);

		int nOrigVerticesPG = pPG->numberOfNodes();

		//---------------------------------------
		// first phase: Compute a planar subgraph
		//---------------------------------------

		// The planar subgraph should contain as many generalizations
		// as possible, hence we put all generalizations into the list
		// preferredEdges.

		List<edge> preferedEdges;
		edge e;
		EdgeArray<int> costOrig(pPG->original(), 1);
		//edgearray for reinserter call: which edge may never be crossed?
		EdgeArray<bool> noCrossingEdge(pPG->original(), false);
		forall_edges(e,*pPG) {
			if (pPG->typeOf(e) == Graph::generalization)
				preferedEdges.pushBack(e);
		}

		List<edge> deletedEdges;
		m_subgraph.get().callAndDelete(*pPG, preferedEdges, deletedEdges);

		//--------------------------------------
		// second phase: Re-insert deleted edges
		//--------------------------------------

		m_inserter.get().callForbidCrossingGens(*pPG, costOrig, deletedEdges);

		// ... and embed resulting planar graph
		adjEntry adjExternal = 0;
		m_embedder.get().call(*pPG, adjExternal);

		m_nCrossings += pPG->numberOfNodes() - nOrigVerticesPG;


		//---------------------------------------------------------
		// third phase: Compute layout of planarized representation
		//---------------------------------------------------------

		Layout drawing(*pPG);

		//---------------------------------------
		//call the Layouter for the CC's UMLGraph
		m_planarLayouter.get().call(*pPG,adjExternal,drawing);

		// copy layout into umlGraph
		// Later, we move nodes and edges in each connected component, such
		// that no two overlap.
		const List<node> &origInCC = pPG->nodesInCC(i);
		ListConstIterator<node> itV;

		for(itV = origInCC.begin(); itV.valid(); ++itV) {
			node vG = *itV;

			pGA->x(vG) = drawing.x(pPG->copy(vG));
			pGA->y(vG) = drawing.y(pPG->copy(vG));

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0)
					continue;
				edge eG = adj->theEdge();
				drawing.computePolylineClear(*pPG,eG,pGA->bends(eG));
			}
		}

		// the width/height of the layout has been computed by the planar
		// layout algorithm; required as input to packing algorithm
		boundingBox[i] = m_planarLayouter.get().getBoundingBox();
	}//for cc's

	//----------------------------------------
	// Arrange layouts of connected components
	//----------------------------------------

	arrangeCCs(*pPG, *pGA, boundingBox);
	delete pPG;
}//simplecall


//-----------------------------------------------------------------------------
//call function for simultaneous drawing: compute a layout for graph umlGraph
// without special UML or interactive features, clique processing etc.
//-----------------------------------------------------------------------------
void PlanarizationLayout::callSimDraw(UMLGraph &umlGraph)
{
	//this simple call method does not care about any special treatments
	//of subgraphs, layout informations etc., therefore we save the
	//option status and set them back later on
	bool l_saveCliqueHandling = m_processCliques;
	m_processCliques = false;

	m_nCrossings = 0;

	const Graph &G = umlGraph.constGraph();
	if(G.empty())
		return;

	PlanRepUML PG(umlGraph);

	const int numCC = PG.numberOfCCs();

	// (width,height) of the layout of each connected component
	Array<DPoint> boundingBox(numCC);

	int i;

	//------------------------------------------
	//now planarize CCs and apply drawing module
	for(i = 0; i < numCC; ++i)
	{
		// we treat connected component i
		// PG is set to a copy of this CC

		PG.initCC(i);

		int nOrigVerticesPG = PG.numberOfNodes();

		edge e;
		EdgeArray<int> costOrig(PG.original(), 1);
		EdgeArray<unsigned int> esgOrig(PG.original(), 0);
		forall_edges(e, G)
			esgOrig[e] = umlGraph.subGraphBits(e);

		//---------------------------------------
		// first phase: Compute a planar subgraph
		//---------------------------------------

		List<edge> deletedEdges;
		m_subgraph.get().callAndDelete(PG, deletedEdges);

		//**************************************
		// second phase: Re-insert deleted edges
		//**************************************

		m_inserter.get().call(PG, costOrig, deletedEdges, esgOrig);

		//calls embedder module:
		adjEntry adjExternal = 0;
		m_embedder.get().call(PG, adjExternal);

		m_nCrossings += PG.numberOfNodes() - nOrigVerticesPG;


		//*********************************************************
		// third phase: Compute layout of planarized representation
		//*********************************************************

		Layout drawing(PG);

		//***************************************
		//call the Layouter for the CC's UMLGraph
		m_planarLayouter.get().call(PG,adjExternal,drawing);

		// copy layout into umlGraph
		// Later, we move nodes and edges in each connected component, such
		// that no two overlap.
		const List<node> &origInCC = PG.nodesInCC(i);
		ListConstIterator<node> itV;

		for(itV = origInCC.begin(); itV.valid(); ++itV) {
			node vG = *itV;

			umlGraph.x(vG) = drawing.x(PG.copy(vG));
			umlGraph.y(vG) = drawing.y(PG.copy(vG));

			adjEntry adj;
			forall_adj(adj,vG) {
				if ((adj->index() & 1) == 0) continue;
				edge eG = adj->theEdge();

				drawing.computePolylineClear(PG,eG,umlGraph.bends(eG));
			}
		}


		// the width/height of the layout has been computed by the planar
		// layout algorithm; required as input to packing algorithm
		boundingBox[i] = m_planarLayouter.get().getBoundingBox();
	}//for cc's

	//----------------------------------------
	// Arrange layouts of connected components
	//----------------------------------------

	arrangeCCs(PG, umlGraph, boundingBox);

	umlGraph.removeUnnecessaryBendsHV();
	m_processCliques = l_saveCliqueHandling;
}//callSimDraw


//#################################################################

//additional help functions

void PlanarizationLayout::assureDrawability(UMLGraph &UG)
{
	//preliminary
	//self loops are killed by the caller (e.g., the plugin interface)
	//should be done here later

	const Graph& G = UG;

	//check for selfloops and handle them
	edge e;
	forall_edges(e, G) {
		if (e->isSelfLoop())
			OGDF_THROW_PARAM(PreconditionViolatedException, pvcSelfLoop);
	}

	// check for generalization - nontrees
	// if m_fakeTree is set, change type of "back" edges to association
	m_fakedGens.clear();//?
	if (!dfsGenTree(UG, m_fakedGens, m_fakeTree))
		OGDF_THROW_PARAM(PreconditionViolatedException, pvcTreeHierarchies);

	else {
		ListConstIterator<edge> itE = m_fakedGens.begin();
		while (itE.valid()) {
			UG.type(*itE) = Graph::association;
			itE++;
		}
	}
}//assureDrawability



void PlanarizationLayout::preProcess(UMLGraph &UG)
{
	assureDrawability(UG);

	if (m_processCliques)
	{
		UG.setDefaultCliqueCenterSize(m_planarLayouter.get().separation());
		const Graph& G = (const Graph &)UG;
		CliqueFinder cf(G);
		cf.setMinSize(m_cliqueSize);

		List< List<node> > cliques;
		cf.call(cliques);
		//now replace all found cliques by stars
		UG.replaceByStar(cliques);
	}
	//TODO: sollte kein else sein, aber man muss abfangen,
	//ob eine Kante geloescht wurde (und die Info nachher wieder bereitstellen)
	else
	{
		const SListPure<UMLGraph::AssociationClass*> &acList = UG.assClassList();
		SListConstIterator<UMLGraph::AssociationClass*> it = acList.begin();
		while (it.valid())
		{
			UG.modelAssociationClass((*it));
			it++;
		}

	}
}//preprocess


void PlanarizationLayout::postProcess(UMLGraph& UG)
{
	//reset the type of faked associations to generalization
	if (m_fakeTree)
	{
		ListIterator<edge> itE = m_fakedGens.begin();
		while (itE.valid())
		{
			UG.type(*itE) = Graph::generalization;
			itE++;
		}
	}

	UG.undoAssociationClasses();
	if (m_processCliques)
		UG.undoStars();
}//postProcess


// find best suited external face according to certain criteria
face PlanarizationLayout::findBestExternalFace(
	const PlanRep &PG,
	const CombinatorialEmbedding &E)
{
	FaceArray<int> weight(E);

	face f;
	forall_faces(f,E)
		weight[f] = f->size();

	node v;
	forall_nodes(v,PG)
	{
		if(PG.typeOf(v) != Graph::generalizationMerger)
			continue;

		adjEntry adj;
		forall_adj(adj,v) {
			if(adj->theEdge()->source() == v)
				break;
		}

		OGDF_ASSERT(adj->theEdge()->source() == v);

		node w = adj->theEdge()->target();
		bool isBase = true;

		adjEntry adj2;
		forall_adj(adj2, w) {
			edge e = adj2->theEdge();
			if(e->target() != w && PG.typeOf(e) == Graph::generalization) {
				isBase = false;
				break;
			}
		}

		if(isBase == false)
			continue;

		face f1 = E.leftFace(adj);
		face f2 = E.rightFace(adj);

		weight[f1] += v->indeg();
		if(f2 != f1)
			weight[f2] += v->indeg();
	}

	face fBest = E.firstFace();
	forall_faces(f,E)
		if(weight[f] > weight[fBest])
			fBest = f;

	return fBest;
}


void PlanarizationLayout::arrangeCCs(PlanRep &PG, GraphAttributes &GA, Array<DPoint> &boundingBox)
{
	int numCC = PG.numberOfCCs();
	Array<DPoint> offset(numCC);
	m_packer.get().call(boundingBox,offset,m_pageRatio);

	// The arrangement is given by offset to the origin of the coordinate
	// system. We still have to shift each node and edge by the offset
	// of its connected component.

	for(int i = 0; i < numCC; ++i) {
		const List<node> &nodes = PG.nodesInCC(i);

		const double dx = offset[i].m_x;
		const double dy = offset[i].m_y;

		// iterate over all nodes in ith CC
		ListConstIterator<node> it;
		for(it = nodes.begin(); it.valid(); ++it)
		{
			node v = *it;

			GA.x(v) += dx;
			GA.y(v) += dy;

			adjEntry adj;
			forall_adj(adj,v) {
				if ((adj->index() & 1) == 0) continue;
				edge e = adj->theEdge();

				DPolyline &dpl = GA.bends(e);
				ListIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it) {
					(*it).m_x += dx;
					(*it).m_y += dy;
				}
			}
		}
	}
}


//collects and stores nodes adjacent to centerNode in adjNodes
void PlanarizationLayout::fillAdjNodes(List<node>& adjNodes,
	PlanRepUML& PG,
	node centerNode,
	NodeArray<bool>& isClique,
	Layout& drawing)
{
	//at this point, cages are collapsed, i.e. we have a center node
	//in the planrep representing the copy of the original node
	node cCopy = PG.copy(centerNode);
	OGDF_ASSERT(cCopy != 0)
	OGDF_ASSERT(cCopy->degree() == centerNode->degree())
	OGDF_ASSERT(cCopy->degree() > 1)
	//store the node with mostright position TODO: consider cage
	node rightNode = 0;

	//due to the implementation in PlanRepUML::collapseVertices, the
	//ordering of the nodes in the copy does not correspond to the
	//ordering in the used embedding, but to the original graph
	//we therefore need to run around the cage to search for the
	//attached edges

	adjEntry adjRun = cCopy->firstAdj();
	do
	{
		//we search for the edge outside the node cage
		//anchor node
		OGDF_ASSERT(adjRun->twinNode()->degree() == 4)
		//should be cs->cs, but doesnt work, comb. embedding?
		//adjEntry outerEdgeAdj = adjRun->twin()->cyclicSucc()->cyclicSucc();
		//TODO: braucht man glaube ich gar nicht, da bereits erste Kante Original hat
		adjEntry outerEdgeAdj = adjRun->twin()->cyclicSucc();
		//this may fail in case of bends if there are no orig edges, but there
		//always is one!?
		while (!PG.original(outerEdgeAdj->theEdge()))
			outerEdgeAdj = outerEdgeAdj->cyclicSucc();
		OGDF_ASSERT(outerEdgeAdj != adjRun)

		//hier besser: if... und alle anderen ignorieren
		edge umlEdge = PG.original(outerEdgeAdj->theEdge());
		OGDF_ASSERT(umlEdge != 0)
		node u = umlEdge->opposite(centerNode);
		adjNodes.pushBack(u);
		isClique[PG.copy(u)] = true;

		//----------------------------------------------
		//part to delete all bends that lie within the clique rectangle
		//first we identify the copy node of the clique node we currently
		//look at
		node uCopy = PG.copy(u);
		OGDF_ASSERT(uCopy)
		adjEntry adjURun = uCopy->firstAdj();
		do
		{
			//we search for the edge outside the node cage
			OGDF_ASSERT(adjURun->twinNode()->degree() == 4)
			adjEntry outerEdgeUAdj = adjURun->twin()->cyclicSucc();
			while (!PG.original(outerEdgeUAdj->theEdge()))
				outerEdgeUAdj = outerEdgeUAdj->cyclicSucc();
			OGDF_ASSERT(outerEdgeUAdj != adjRun)
			//outerEdgeUAdj points outwards, edge does too per implementation,
			//but we don't want to rely on that fact
			bool outwards;
			edge potKill = outerEdgeUAdj->theEdge();
			node splitter;
			if (potKill->source() == outerEdgeUAdj->theNode()) //Could use opposite
			{
				splitter = potKill->target();
				outwards = true;
			}
			else
			{
				splitter = potKill->source();
				outwards = false;
			}
			//we erase bends and should check the node type here, but the only
			//case that can happen is bend
			while (splitter->degree() == 2)
			{
				if (outwards)
				{
					PG.unsplit(potKill, potKill->adjTarget()->cyclicSucc()->theEdge());
					splitter = potKill->target();
				}
				else
				{
					edge ek = potKill->adjSource()->cyclicSucc()->theEdge();
					PG.unsplit(ek, potKill);
					potKill = ek;
					splitter = potKill->source();
				}
			}//while

			adjURun = adjURun->cyclicPred(); //counterclockwise, Succ clockwise
		} while (adjURun != uCopy->firstAdj());

		//----------------------------------------------

		//check if node is better suited to lie at the right position
		if (rightNode != 0)
		{
			if (drawing.x(PG.copy(u)) > drawing.x(PG.copy(rightNode)))
			{
				rightNode = u;
			}
		}
		else
		{
			rightNode = u;
		}

		adjRun = adjRun->cyclicPred(); //counterclockwise, Succ clockwise
	} while (adjRun != cCopy->firstAdj());

	//---------------------------------------
	//adjust ordering to start with rightNode
	//if (true) //zum debuggen ausschalten koennen
		while (adjNodes.front() != rightNode)
		{
			node tempV = adjNodes.popFrontRet();
			adjNodes.pushBack(tempV);
		}
}//filladjNodes

} // end namespace ogdf
