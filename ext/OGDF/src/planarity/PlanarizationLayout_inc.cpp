/*
* $Revision: 2559 $
*
* last checkin:
*   $Author: gutwenger $
*   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
***************************************************************/

/** \file
 * \brief implementation of class UMLPlanarizationLayout.
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
#include <ogdf/basic/TopologyModule.h>
#include <ogdf/planarity/PlanRepInc.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/planarity/SimpleIncNodeInserter.h>


namespace ogdf {

//-----------------------------------------------------------------------------
//incremental call: takes a fixed part of the input
//graph (indicated by fixedNodes/Edges==true), embeds it using
//the input layout, then inserts the remaining part into this embedding
//currently, only the subgraph induced by the fixed nodes is fixed
void PlanarizationLayout::callIncremental(
	UMLGraph &umlGraph,
	NodeArray<bool> &fixedNodes,
	const EdgeArray<bool> & /* fixedEdges */)
{
	try {

		if(((const Graph &)umlGraph).empty())
			return;
		//-------------------
		//check preconditions
		//(change edge types in umlGraph at non-tree hierarchies
		//or replace cliques by nodes)
		preProcess(umlGraph);

		m_nCrossings = 0; //number of inserted crossings

		//use the options set at the planar layouter
		int l_layoutOptions = m_planarLayouter.get().getOptions();
		bool l_align = ((l_layoutOptions & umlOpAlign)>0);

		//check: only use alignment mode if there are generalizations
		bool l_gensExist = false; //set this for all CC's, start with first gen

		//-------------------------------------------------------
		//first, we sort all edges around each node corresponding
		//to the given layout in umlGraph, then we insert the mergers
		//May be inserted in original or in copy
		bool umlMerge = false; //Standard: insertion in copy
		//if (umlMerge)
		//{
		//	umlGraph.sortEdgesFromLayout();
		//	prepareIncrementalMergers(umlGraph);
		//}
		//-----------------------------------------------------------
		//second, we embed the fixed part corresponding to the layout
		//given in umlgraph
		//TODO: check if we still need the global lists, do this
		//for all CCs
		//we create lists of the additional elements, maybe this can
		//later be done implicitly within some other loop

		//----------------------------------------------------------
		// first phase: Compute planarized representation from input
		//----------------------------------------------------------

		//------------------------------------------------
		//now we derive a partial planar representation of
		//the input graph using its layout information
		PlanRepInc PG(umlGraph, fixedNodes);

		const int numCC = PG.numberOfCCs();

		//TODO: adjust for CC number before/after insertion
		// (width,height) of the layout of each connected component
		Array<DPoint> boundingBox(numCC);

		//------------------------------------------
		//now planarize CCs and apply drawing module
		int i;
		for(i = 0; i < numCC; ++i)
		{
			// we treat connected component i where PG is set
			// to a copy of this CC consisting only of fixed nodes
			node minActive = PG.initMinActiveCC(i);
			//if a single node was made active, we update its status
			if (minActive != 0)
			{
				fixedNodes[minActive] = true;
			}

#ifdef OGDF_DEBUG
			edge e;
			forall_edges(e, PG)
			{
				edge eOrig = PG.original(e);
				if (eOrig)
				{
					OGDF_ASSERT(PG.chain(eOrig).size() <= 1)
				}
			}//foralledges
#endif

			int nOrigVerticesPG = PG.numberOfNodes();
			//TODO: check if copying is really necessary
			//we want to sort the additional nodes and therefore
			//copy the list
			List<node> addNodes;
			ListConstIterator<node> it = PG.nodesInCC().begin();
			while (it.valid())
			{
				if (!fixedNodes[(*it)])
					addNodes.pushBack((*it));
				it++;
			}//while


			//--------------------------------------------
			//now we insert the additional nodes and edges
			//sort the additional nodes
			//simple strategy: sort by their #connections to the fixed part
			//we can therefore only count if we have fixed nodes
			if ((addNodes.size() > 1) && (PG.nodesInCC().size() != addNodes.size()))
				sortIncrementalNodes(addNodes, fixedNodes);

			//TODO: guarantee that the CC is non-empty and connected
			//insert a first part otherwise
			//DONE: unconnected parts are connected in a chain

			//attention: the following list pop relies on the property of
			//the sorting algorithm, that the first node has a connection
			//to a fixed node or otherwise none of the addnodes has a
			//connection to fixednodes (its a CC on its own)
			//in the worst case, we have to introduce a tree, which equals
			//the steinertree problem, which is NP-hard. As we do not want
			//to preinsert nodes that do not have layout information (they
			//do have, but we do not consider it (e.g. for randomly
			//inserted nodes)), we avoid the problem by inserting artificial
			//edges connecting all CCs, building a tree of CCs

			//now we derive the embedding given in umlgraph for the fixed part
			//we work on a copy of umlgraph, because we have to add/delete nodes
			//and edges and create a PlanRep on an intermediate representation
			adjEntry adjExternal = 0;

			//--------------------------------------------
			//here lies the main difference to the static call
			//we first set the embedding corresponding to the
			//input and then planarize the given layout
			TopologyModule TM;

			bool embedded = true;
			try { //TODO:should be catched within setEmbeddingFromGraph
				//do not yet compute external face, embed and planarize
				//with layout given in umlGraph
				embedded = TM.setEmbeddingFromGraph(PG, umlGraph,
					adjExternal, false, umlMerge);
			}//try
			catch(...)
			{
				embedded = false;
			}//catch

			//returns true if connnectivity edges introduced
			//TODO: hierhin oder eins nach unten?
			PG.makeTreeConnected(adjExternal);

			//if embedding could not be set correctly, use standard embedding
			if (!embedded)
			{
				reembed(PG, i, l_align, l_gensExist);
			}

			//--------------------------------------------
			//now we compute a combinatorial embedding on
			//the partial PlanRep that is used for node insertion
			//and the external face
			bool singleNode = (PG.numberOfNodes() == 1);
			if ((PG.numberOfEdges() > 0) || singleNode)
			{
				CombinatorialEmbedding E(PG);

				//if we have edges, but no external face, find one
				//we have also to select one later if there are no edges yet
				if((adjExternal == 0) && PG.numberOfEdges() > 0)
				{
					//face fExternal = E.maximalFace();
					face fExternal = findBestExternalFace(PG,E);
					adjExternal = fExternal->firstAdj();
					//while (PG.sinkConnect(adjExternal->theEdge()))
					//	adjExternal = adjExternal->faceCycleSucc();
				}
				if ( (adjExternal != 0) && (PG.numberOfEdges() > 0) )
					E.setExternalFace(E.rightFace(adjExternal));

				//-------------------------------------------------
				//we insert additional nodes into the given PlanRep
				SimpleIncNodeInserter inserter(PG);
				ListIterator<node> itAdd = addNodes.begin();
				while (itAdd.valid())
				{
#ifdef OGDF_DEBUG
					edge eDebug = (*itAdd)->firstAdj()->theEdge();
#endif
					OGDF_ASSERT(PG.chain(eDebug).size() <= 1)
					//we can check here if PG CC connected and speed
					//up insertion by not updating CC part information
					inserter.insertCopyNode((*itAdd), E, umlGraph.type((*itAdd)));

					//select an arbitrary external face if there was only one fixed node
					if (singleNode && (PG.numberOfEdges() > 0))
					{
						adjExternal = PG.firstEdge()->adjSource();
						E.setExternalFace(E.rightFace(adjExternal));
					}
					else
					{
						adjExternal = E.externalFace()->firstAdj();
						int eNum = max(10, PG.numberOfEdges()+1);
						int count = 0;
						while ((adjExternal->theNode() == adjExternal->twinNode()) &&
							(count < eNum))
						{
							adjExternal = adjExternal->faceCycleSucc();
							count++;
						}
						OGDF_ASSERT(count < eNum)
					}

					itAdd++;
				}//while

				if (!umlMerge)
					PG.setupIncremental(i, E);
				OGDF_ASSERT(E.consistencyCheck())

					//we now have a complete representation of the
					//original CC

					m_nCrossings += PG.numberOfNodes() - nOrigVerticesPG;

				//********************
				//copied from fixembed

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
						//first save a pointer to the bends of the generalization
						//leading to the target
						DPolyline dpUp;

						//check if there is an expansion face
						if (adjMerger != 0)
						{
							//if there are bends on the edge, copy them
							//TODO: check where to derive the bends from
							//if eUp is nil
							adjEntry adjUp = adjMerger->cyclicPred();
							OGDF_ASSERT(PG.isGeneralization(adjUp->theEdge()))
								edge eUp = PG.original(adjUp->theEdge());
							//There is never an original here in current incremental algo
							//OGDF_ASSERT(eUp)
							if (eUp) dpUp = umlGraph.bends(eUp);
							//we run through the expansion face to the connected edges
							//this edge is to dummy corner
							adjEntry runAdj = adjMerger->faceCycleSucc();
							while (runAdj != adjMerger)
							{
								node vConnect = runAdj->theNode();
								//because of the node collapse using the original
								//edges instead of the merger copy edges (should be
								//fixed for incremental mode) the  degree is 4
								//FIXED!?
								//if (vConnect->degree() != 4)
								//while?
								if (vConnect->degree() != 3)
								{
									runAdj = runAdj->faceCycleSucc();
									continue;
								}
								edge eCopy = runAdj->cyclicPred()->theEdge();
								OGDF_ASSERT(eCopy->target() == runAdj->theNode())
									OGDF_ASSERT(PG.isGeneralization(eCopy))
									OGDF_ASSERT(PG.original(eCopy))

									DPolyline &eBends = umlGraph.bends(PG.original(eCopy));

								eBends.pushBack(
									DPoint(drawing.x(vMerger), drawing.y(vMerger)));

								if (eUp)
								{
									ListConstIterator<DPoint> itDp;
									for(itDp = dpUp.begin(); itDp.valid(); ++itDp)
										eBends.pushBack(*itDp);
								}
								else
									eBends.pushBack(DPoint(drawing.x(adjUp->twinNode()),
									drawing.y(adjUp->twinNode())));

								runAdj = runAdj->faceCycleSucc();

							}

						}
						else //currently all nodes are expanded, but this is not guaranteed
						{
							//first save the bends
							adjEntry adjUp = 0;
							forall_adj(adjMerger, vMerger)
							{
								//upgoing edge
								if (adjMerger->theEdge()->source() == vMerger)
								{
									adjUp = adjMerger;
									OGDF_ASSERT(PG.isGeneralization(adjMerger->theEdge()))
										edge eUp = PG.original(adjMerger->theEdge());
									//check if this is
									//a) the merger up edge and not a connectivity edge
									//b) what to do if there is no original of outgoing edge
									if (eUp)
										dpUp = umlGraph.bends(eUp);
									break;
								}//if

							}//foralladj
							forall_adj(adjMerger, vMerger)
							{
								if (adjMerger->theEdge()->target() == vMerger)
								{
									edge eOrig = PG.original(adjMerger->theEdge());
									if (eOrig)
									{
										//incoming merger edges always have an original here!
										umlGraph.bends(eOrig).pushBack(DPoint(drawing.x(vMerger),
											drawing.y(vMerger)));

										//was there an original edge?
										if (dpUp.size()>0)
										{
											ListConstIterator<DPoint> itDp;
											for(itDp = dpUp.begin(); itDp.valid(); ++itDp)
												umlGraph.bends(eOrig).pushBack(*itDp);
										}//if
										else
										{
											if (adjUp)
												umlGraph.bends(eOrig).pushBack(DPoint(drawing.x(adjUp->twinNode()),
												drawing.y(adjUp->twinNode())));
										}//else
									}//if

								}
							}//forall adj
						}//else
						itMerger++;
					}//while merger nodes
				}//if !umlMerge

				//umlGraph.writeGML("C:\\FullLayout2Inc.gml");

				// the width/height of the layout has been computed by the planar
				// layout algorithm; required as input to packing algorithm
				boundingBox[i] = m_planarLayouter.get().getBoundingBox();

				//*******************

			}//if #edges > 0

			else
			{
				//TODO: what if there are no edges but the insertion edges
				//DONE: we make the CC treeConnected
				//Nonetheless we have to compute a layout here
				OGDF_ASSERT(PG.numberOfNodes() < 2)
			}



			//TODO: set m_crossings here
			//m_nCrossings += PG.numberOfNodes() - numOrigNodes;

		}//for connected components

		//*******************
		//TODO: check shifting to new place

		//----------------------------------------
		// Arrange layouts of connected components
		//----------------------------------------

		arrangeCCs(PG, umlGraph, boundingBox);

		if (umlMerge) umlGraph.undoGenMergers();

		umlGraph.removeUnnecessaryBendsHV();

		//********************
		//new position after adding clique process, check if correct
		postProcess(umlGraph);
	}//try
	catch (...)
	{
		call(umlGraph);
		return;
	}
}//callIncremental


//Insertion order of added nodes:
//sorting strategy: we count all adjacent nodes that are in
//fixedNodes and sort the nodes correspondingly
//In addition, we guarantee that no node is inserted before at least one
//of its neighbours is inserted

//compute how far away from the fixed part the added nodes lay
//uses BFS
void PlanarizationLayout::getFixationDistance(node startNode,
	HashArray<int, int> &distance,
	const NodeArray<bool> &fixedNodes)
{
	//we only change the distance for nodes with distance <=0 because they are not
	//connected to the fixed part
	//we only care about nodes which are members of array distance
	HashArray<int, bool> indexMark(false);
	//the BFS queue
	QueuePure<node> nodeQ;

	nodeQ.append(startNode);
	indexMark[startNode->index()] = true;
	while (!nodeQ.empty())
	{
		adjEntry adjE;
		node topNode = nodeQ.pop();
		//hier aufpassen: geht nur, wenn kein fixedNode eine Distance hat
		//alternativ: alle auf null
		//zur sicherheit: fixed uebergeben und vergleichen
		bool fixedBase = fixedNodes[topNode];

		forall_adj(adjE, topNode)
		{
			node testNode = adjE->twinNode();
			int ind = testNode->index();

			if (!indexMark[ind])
			{
				indexMark[ind] = true;
				nodeQ.append(testNode);
			}

			//we have a distance value, i.e. ind is an additional node
			if (!fixedNodes[testNode])
			{
				if (distance[ind] <= 0)
				{
					//should never occur (check: nodes not in CC?)
					if (fixedBase)
					{
						distance[ind] = max(-1, distance[ind]);
						OGDF_ASSERT(false)
					}
					else
					{
						if (distance[ind] == 0) distance[ind] = min(-1, distance[topNode->index()] - 1);
						else distance[ind] = min(-1, max(distance[ind], distance[topNode->index()] - 1));
					}//else
				}//if node without contact to fixed nodes
			}//if distance (is an addnode)
		}//foralladj
	}//while

}//getFixationDistance

//Attention: changing this behavior makes it necessary to check
//the call procedure for the case where one of the addnodes is
//made fix to allow insertion of an edge
void PlanarizationLayout::sortIncrementalNodes(List<node> &addNodes,
	const NodeArray<bool> &fixedNodes)
{
	//if there are no fixed nodes, we can not sort
	//todo: do some other sorting

	//we count all adjacent fixed nodes
	//store degree by node index
	HashArray<int, int> indexToDegree(0);
	ListIterator<node> it = addNodes.begin();
	adjEntry adjE;
	node someFixedNode = 0;

	while (it.valid())
	{
		if ((*it)->degree() < 1)
		{
			indexToDegree[(*it)->index()] = 0;
			it++;
			continue;
		}
		int vDegree = 0;
		adjE = (*it)->firstAdj();
		do {
			if (fixedNodes[adjE->twinNode()])
			{
				vDegree++;
				someFixedNode = adjE->twinNode();
			}
			adjE = adjE->cyclicSucc();
		} while (adjE != (*it)->firstAdj());

		indexToDegree[(*it)->index()] = vDegree;

		it++;
	}//while
	//for all nodes that are not connected to the fixed part we have to guarantee
	//that they are not inserted before one of their neighbours is inserted
	//therefore we set negative values for all nodes corresponding to their distance
	//to the fixed part
	OGDF_ASSERT(someFixedNode != 0)
		if (someFixedNode == 0) throw AlgorithmFailureException();
	//we start the BFS at some fixed node
	getFixationDistance(someFixedNode,indexToDegree, fixedNodes);


	//we sort the nodes in decreasing vDegree value order
	AddNodeComparer comp(indexToDegree);
	addNodes.quicksort(comp);

}//sortincrementalnodes

} // end namespace ogdf
