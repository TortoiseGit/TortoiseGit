/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of a heuristical method to find cliques
 * in a given input graph.
 *
 * \author Karsten Klein
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


#include <ogdf/graphalg/CliqueFinder.h>
#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/NodeComparer.h>

#ifdef OGDF_DEBUG
#include <ogdf/basic/GraphAttributes.h>
#endif
#include <math.h>
#include <ogdf/basic/geometry.h>


namespace ogdf {


//constructor
CliqueFinder::CliqueFinder(const Graph &G) : m_pGraph(&G), m_pCopy(0),
	m_minDegree(2),
	m_numberOfCliques(0),
	m_postProcess(ppSimple),
	m_callByList(false),
	m_pList(0),
	m_density(100)
{
	try {
		m_pCopy = new GraphCopy(G);
		m_copyCliqueNumber.init(*m_pCopy, -1);
		m_usedNode.init(*m_pCopy, false);
	}
	catch (...)
	{
		OGDF_THROW(InsufficientMemoryException);
	}
}//constructor

CliqueFinder::~CliqueFinder()
{
	if (m_pCopy != 0)
	{
		//we have to uninitialize the nodearray before destroying
		//the graph
		m_copyCliqueNumber.init();
		m_usedNode.init();

		delete m_pCopy;
	}
}//destructor



//---------------------------------------------------
//calls
//---------------------------------------------------
//Call with NodeArray, each clique will be assigned a
//different number, each node gets the number of the
//clique it is contained in, -1 if not a clique member
void CliqueFinder::call(NodeArray<int> &cliqueNumber)
{
	m_callByList = false;
	m_pList = 0;
	//First find the cliques: doCall
	doCall(m_minDegree);
	//Then set the result: setResults(cliqueNumber);
	setResults(cliqueNumber);

}//call

//call with list of node lists, on return these lists contain
//the nodes in each clique that is found
void CliqueFinder::call(List< List<node> > &cliqueLists)
{
	m_callByList = true;
	m_pList = &cliqueLists;
	m_pList->clear();

	//First find the cliques: doCall
	doCall(m_minDegree);
	//setresult is called in doCall for every treated component

	m_pList = 0;
}//call


//---------------------------------------------------------
//actual call
//minDegree default 2, all other nodes are skipped
//only high values have an impact because we only
//work on triconnected components, skipping all low
//degree nodes (but we make the test graphcopy biconnected
//afterwards)
void CliqueFinder::doCall(int minDegree)
{
	//---------------------------------------------
	//initialize structures and check preconditions
	//---------------------------------------------
	m_copyCliqueNumber.init(*m_pCopy, -1);
	m_usedNode.init(*m_pCopy, false);
	makeParallelFreeUndirected(*m_pCopy); //it doesnt make sense to count loops
	makeLoopFree(*m_pCopy);               //or parallel edges

	m_numberOfCliques = 0;
	//We first indentify the biconnected components of
	//the graph to allow the use of the SPQR-tree data
	//Structure. Latter then separates the different
	//triconnected components on which we finally work

	//TODO: delete all copy nodes with degree < minDegree

	int nodeNum = m_pGraph->numberOfNodes();
	//TODO: change for non-cliques, where this is not necessary
	if (nodeNum < minDegree) return; //nothing to find for cliques

	//-------------------------------------------------------
	//Special cases:
	//Precondition for SPQR-trees: graph has at least 3 nodes
	//or 2 nodes and at least 3 edges
	//TODO: check this after makebiconnected

	//----------------------------
	//set values for trivial cases
	if (nodeNum < 3)
	{
		//only set numbers for the special case
		if (nodeNum == 2)
		{
			if (m_pGraph->numberOfEdges() >= 1)  //> 2)
			{
				node w = m_pCopy->firstNode();
				m_copyCliqueNumber[w] = 0;
				w = w->succ();
				m_copyCliqueNumber[w] = 0;
			}
			else
			{
				if (minDegree == 0)
				{
					node w = m_pCopy->firstNode();
					m_copyCliqueNumber[w] = 0;
					w = w->succ();
					m_copyCliqueNumber[w] = 1;
				}//if no mindegree

			}
		}//if two nodes
		else if ( (nodeNum == 1) && (minDegree <= 0))
				m_copyCliqueNumber[m_pCopy->firstNode()] = 0;

		return;
	}//graph too small

	OGDF_ASSERT(m_pCopy != 0)

	//save the original edges
	EdgeArray<bool> originalEdge(*m_pCopy, true);
	List<edge> added;


	//we make the copy biconnected, this keeps the triconnected
	//components


	//-------------------------------------------------------------
	//store the original node degrees:
	//afterwards we want to be able to sort the nodes corresponding
	//to their real degree, not the one with the additional
	//connectivity edges
	NodeArray<int> realDegree(*m_pCopy, -1);//no isolated nodes exist here
	//relative degree, number of high degree neighbours
	NodeArray<int> relDegree(*m_pCopy, 0);//no isolated nodes exist here
	node v;
	forall_nodes(v, *m_pCopy)
	{
		realDegree[v] = v->degree();
		if (v->degree() > 0)
		{
			adjEntry adRun = v->firstAdj();
			while (adRun)
			{
				adjEntry succ = adRun->succ();
				if (adRun->twinNode()->degree() >= minDegree)
					relDegree[v]++;
				adRun = succ;
			}//while
		}//if not isolated

	}//forallnodes

	makeBiconnected(*m_pCopy, added);

	//TODO: We can decrease node degrees by the number of adjacent
	//low degree nodes to sort them only by number of relevant connections
	//PARTIALLY DONE: relDegree

	//storing the component number, there are no isolated nodes now
	EdgeArray<int> component(*m_pCopy);

	StaticSPQRTree spqrTree(*m_pCopy);

	//Return if there are no R-nodes
	if (spqrTree.numberOfRNodes() == 0)
	{
		//TODO:set node numbers for cliques
		//that are not triconnected
		//each edge is a min. clique for mindegree 1
		//each triangle for mindegree 2?

		return;
	}//if

	//the degree of the original node
	//within the triconnected component
	NodeArray<int> ccDegree(*m_pCopy, 0);

	forall_nodes(v, spqrTree.tree())
	{
		//we search for dense subgraphs in R-Nodes
		//heuristics:
		//sort the nodes by their degree within the component
		//in descending order, then start cliques by initializing
		//them with the first node and checking the remaining,
		//starting new cliques with nodes that don't fit in the
		//existing cliques (stored in cliqueList)

		if (spqrTree.typeOf(v) == SPQRTree::RNode)
		{
			//retrieve the skeleton
			Skeleton &s = spqrTree.skeleton(v);
			node w;
			Graph &skeletonG = s.getGraph();

			//list of cliques
			List< List<node>* > cliqueList;

			//we insert all nodes into a list to sort them
			List<node> sortList;

			//save the usable edges within the triconnected component
			EdgeArray<bool> usableEdge(*m_pCopy, false);

			//derive the degree of the original node
			//within the triconnected component
			forall_nodes(w, skeletonG)
			{
				node vOrig = s.original(w);

				edge eSkel;
				forall_adj_edges(eSkel, w)
				{
					edge goodEdge = s.realEdge(eSkel);
					bool isGoodEdge = goodEdge != 0;
					if (isGoodEdge) isGoodEdge = m_pCopy->original(goodEdge) != 0;
					//if (s.realEdge(eSkel))
					if (isGoodEdge)
					{
						ccDegree[vOrig]++;
						usableEdge[goodEdge] = true;
					}
				}//foralladjedges

				sortList.pushBack(vOrig);

			}//forall_nodes

			//sort the nodes corresponding to their degree
			NodeComparer<int> ncomp(ccDegree, false);
			sortList.quicksort(ncomp);

			ListIterator<node> itNode = sortList.begin();

			while(itNode.valid())
			{

				//hier erst vergleichen, ob Knoten Grad > aktcliquengroesse,
				//dann ob mit clique verbunden
				//alternativ koennte man stattdessen fuer jeden gewaehlten
				//Knoten nur noch seine Nachbarn als Kandidaten zulassen
				//hier sollte man mal ein paar Strategien testen, z.B.
				//streng nach Listenordnung vorgehen oder eine "Breitensuche"
				//vom Startknoten aus..
				node vCand = *itNode;

				//node can appear in several 3connected components
				if (m_usedNode[vCand])
				{
					itNode++;
					continue;
				}//if already used

				//if there are only "small" degree nodes left, we stop
				//if (vCand->degree() < minDegree)
				if (ccDegree[vCand] < minDegree)
					break;

				//------------------------------------------------
				//successively check the node against the existing
				//clique candidates

				//run through the clique candidates to find a matching
				//node set
				bool setFound = false;
				ListIterator< List<node>* > itCand = cliqueList.begin();
				while (itCand.valid())
				{

					//in the case of cliques, the node needs min degree
					//greater or equal to current clique size
					//TODO: adapt to dense subgraphs
					bool isCand = false;
					if (m_density == 100)
						isCand = (vCand->degree() >= (*itCand)->size());
					else isCand = (vCand->degree() >= ceil(m_density*(*itCand)->size()/100.0));
					if (isCand)
					{
						//TODO: insert adjacency oracle here to speed
						//up the check?
						//TODO: check if change from clique to dense subgraph criterion
						//violates some preconditions for our search
						if (allAdjacent(vCand, (*itCand)))
						{
							OGDF_ASSERT(m_usedNode[*itNode] == false)
							(*itCand)->pushBack(*itNode);
							setFound = true;
							m_usedNode[(*itNode)] = true;

							//bubble sort the clique after insertion of the node
							//while size > predsize swap positions
							ListIterator< List<node>* > itSearch = itCand.pred();
							if (itSearch.valid())
							{
								while (itSearch.valid() &&
									( (*itCand)->size() > (*itSearch)->size()) )
								{
									itSearch--;
								}
								//If valid, move behind itSearch, else move to front
								if (!itSearch.valid())
									cliqueList.moveToFront(itCand);
								else cliqueList.moveToSucc(itCand, itSearch);
							}//if valid

							break;
						}//if node fits into node set
					}//if sufficient degree
					//hier kann man mit else breaken, wenn Liste immer sortiert ist

					itCand++;
				}//while clique candidates

				//create a new candidate if necessary
				if (!setFound)
				{
					List<node>* cliqueCandidate = OGDF_NEW List<node>();
					itCand = cliqueList.pushBack(cliqueCandidate);
					OGDF_ASSERT(m_usedNode[*itNode] == false)
					cliqueCandidate->pushBack(*itNode);
					m_usedNode[(*itNode)] = true;

				}//if no candidate yet

				itNode++;
			}//while valid

			//TODO: cliquelist vielleicht durch einen member ersetzen
			//und nicht das delete vergessen!
#ifdef OGDF_DEBUG
			int numC1 = cliqueList.size();

			int nodeNum = 0;
			ListIterator< List<node>* > itDeb = cliqueList.begin();
			while (itDeb.valid())
			{
				if ( (*itDeb)->size() > minDegree )
					nodeNum = nodeNum +(*itDeb)->size();
				itDeb++;
			}
			checkCliques(cliqueList, false);
			double realTime;
			ogdf::usedTime(realTime);
#endif
			postProcessCliques(cliqueList, usableEdge);
#ifdef OGDF_DEBUG
			realTime = ogdf::usedTime(realTime);

			int nodeNum2 = 0;
			itDeb = cliqueList.begin();
			while (itDeb.valid())
			{
				if ( (*itDeb)->size() > minDegree )
					nodeNum2 = nodeNum2 +(*itDeb)->size();
				itDeb++;
			}
			if (nodeNum2 > nodeNum)
			{
				cout<<"\nAnzahl Cliquen vor PP: "<<numC1<<"\n";
				cout<<"Anzahl Cliquen nach PP: "<<cliqueList.size()<<"\n";
				cout<<"Anzahl Knoten in grossen Cliquen: "<<nodeNum<<"\n";
				cout<<"Anzahl Knoten danach in grossen Cliquen: "<<nodeNum2<<"\n\n";
			}
			//cout << "Used postprocessing time: " << realTime << "\n" << flush;
			checkCliques(cliqueList, false);
#endif

			//now we run through the list until the remaining node sets
			//are to small to be of interest
			ListIterator< List<node>* > itCand = cliqueList.begin();
			while (itCand.valid())
			{
				if ( (*itCand)->size() <= minDegree ) break;

				ListIterator<node> itV = (*itCand)->begin();
				while (itV.valid())
				{
					node u = (*itV);
					OGDF_ASSERT(m_copyCliqueNumber[u] == -1)
					m_copyCliqueNumber[u] = m_numberOfCliques;
					itV++;
				}//while clique nodes
				m_numberOfCliques++;
				itCand++;
			}//while
			//TODO: only set numbers if return value is not a list
			//of clique node lists
			setResults(cliqueList);

			//free the allocated memory
			ListIterator< List<node>* > itCl = cliqueList.begin();
			while (itCl.valid())
			{
				delete (*itCl);
				itCl++;
			}//while

			//debug
			//GraphAttributes AG(skeletonG);
			//char *ch = new char[50];
			//sprintf(ch, "c:\\temp\\ogdl\\skeleton%d.gml",j++);
			//AG.writeGML(ch);
			//delete ch


		}//if
	}//forallnodes

}//docall

//revisits cliques that are bad candidates and rearranges them,
//using only edges with usableeEdge == true
void CliqueFinder::postProcessCliques(
	List< List<node>* > &cliqueList,
	EdgeArray<bool> &usableEdge)
{
	//TODO:hier aufpassen, das man nicht Knoten ausserhalb des
	//R-Knotens nimmt
	if (m_postProcess == ppNone) return;

	//------------------------------------------------------
	//we run over all leftover nodes and try to find cliques
	List<node> leftOver;

	//list of additional cliques
	List< List<node>* > cliqueAdd;

	//-----------------------------------
	//First we check the nodes set by the
	//heuristic for dense subgraphs
	//best would be to reinsert them immediatedly after
	//each found subgraph to allow reuse
	ListIterator< List<node>* > itCand = cliqueList.begin();
	if (m_density != 100)
	while (itCand.valid())
	{
		if ((*itCand)->size() > m_minDegree)
		{
			NodeArray<bool> inList(*m_pCopy, false);
			ListIterator<node> itNode = (*itCand)->begin();
			while (itNode.valid())
			{
				inList[*itNode] = true;
				itNode++;
			}//while

			itNode = (*itCand)->begin();
			while (itNode.valid())
			{
				int adCount = 0; //counts number of nodes adj. to *itNode in itCand
				//check if inGraph degree is high enough
				//and allow reuse otherwise
				adjEntry adE = (*itNode)->firstAdj();
				for (int i = 0; i < (*itNode)->degree(); i++)
				{
					if (usableEdge[adE->theEdge()])
					{
						if (inList[adE->twinNode()])
						adCount++;
					}
					adE = adE->cyclicSucc();
				}//for

				//now delete nodes if connectivity to small
				if (DIsLess(adCount, ceil(((*itCand)->size()-1)*m_density/100.0)))
				{
					leftOver.pushBack(*itNode);
					m_usedNode[*itNode] = false;
					inList[*itNode] = false;
					ListIterator<node> itDel = itNode;
					itNode++;
					(*itCand)->del(itDel);
					continue;
				}
				itNode++;
			}//while

		}//if
		else break;
		itCand++;
	}//while


	itCand = cliqueList.begin();
	while (itCand.valid())
	{
		if ((*itCand)->size() <= m_minDegree)
		{
			while (!((*itCand)->empty()))
			{
				node v = (*itCand)->popFrontRet();
				leftOver.pushBack(v);
				m_usedNode[v] = false;
				OGDF_ASSERT(!m_usedNode[v])
				OGDF_ASSERT(m_copyCliqueNumber[v] == -1)
			}//while nodes
			ListIterator< List<node>* > itDel = itCand;
			delete (*itDel);
			itCand++;
			cliqueList.del(itDel);//del
			//Todo: remove empty lists here
			continue;
		}//if

		itCand++;
	}//while lists

	//now we have all left over nodes in list leftOver
	//vorsicht:  wenn wir hier evaluate benutzen, duerfen wir
	//nicht Knoten hinzunehmen, die schon in Cliquen benutzt sind
	NodeArray<int> value(*m_pCopy);
	NodeComparer<int> cmp(value, false);
	ListIterator<node> itNode = leftOver.begin();
	while (itNode.valid())
	{
		node vVal = (*itNode);
		value[vVal] = evaluate(vVal, usableEdge);
		itNode++;
	}//while

	leftOver.quicksort(cmp);

	//--------------------------------------------------
	//now start a new search at the most qualified nodes
	//TODO: Option: wieviele?
	itNode = leftOver.begin();
	while (itNode.valid())
	{
		//some nodes can already be assigned in earlier iterations
		if (m_usedNode[*itNode])
		{
			itNode++;
			continue;
		}//if already used

		//TODO: hier an dense subgraphs anpassen

		//TODO: this is inefficient because we already
		//ran through the neighbourhood
		//this is the same loop as in evaluate and
		//should not be run twice, but its not efficient
		//to save the neighbour degree values for every
		//run of evaluate
		//##############################
		NodeArray<bool> neighbour(*m_pCopy, false);
		NodeArray<int>  neighbourDegree(*m_pCopy, 0);
		adjEntry adj1;
		node v = *itNode;
		OGDF_ASSERT(!m_usedNode[v])
		forall_adj(adj1, v)
		{
			if (!usableEdge[adj1->theEdge()]) continue;
			node w = adj1->twinNode();
			if (!m_usedNode[w]) neighbour[w] = true;
		}//foralladj

		List<node> *neighbours = new List<node>();
		//this loop counts every triangle (two times)
		//TODO: man kann besser oben schon liste neighbours fuellen
		//und hier nur noch darueber laufen
		forall_adj(adj1, v)
		{
			//if (!usableEdge[adj1->theEdge()]) continue;
			node w = adj1->twinNode();
			//if (m_usedNode[w]) continue; //dont use paths over other cliques
			if (!neighbour[w]) continue;
			OGDF_ASSERT(!m_usedNode[w])
			adjEntry adj2;

			OGDF_ASSERT(m_copyCliqueNumber[w] == -1)
			neighbours->pushBack(w);
			neighbourDegree[w]++; //connected to v

			forall_adj(adj2, w)
			{
				if (!usableEdge[adj2->theEdge()]) continue;
				node u = adj2->twinNode();
				if (m_usedNode[u]) continue; //dont use paths over other cliques
				if (neighbour[u])
				{
					neighbourDegree[w]++;
				}//if
			}//foralladj
		}//foralladj
		//##############################

		//---------------------------------------------
		//now we have a (dense) set of nodes and we can
		//delete the nodes with the smallest degree up
		//to a TODO certain amount
		cmp.init(neighbourDegree);
		neighbours->quicksort(cmp);

		//neighbours->clear();
		findClique(v, *neighbours);
		/*
		ListIterator<node> itDense = neighbours->rbegin();
		while (itDense.valid())
		{
			//TODO: hier die Bedingung an Dichte statt
			//Maximalgrad
			//removes all possible but one clique around v
			if (neighbourDegree[*itDense] < neighbours->size())
				neighbours->del(itDense);


			itDense--;
		}//while
		*/

		//hier noch usenode setzen und startknoten hinzufuegen
		//we found a dense subgraph
		if (neighbours->size() >= m_minDegree)
		{
			OGDF_ASSERT(allAdjacent(v, neighbours))
			neighbours->pushFront(v);

			ListIterator<node> itUsed = neighbours->begin();
			while (itUsed.valid())
			{
				OGDF_ASSERT(m_usedNode[*itUsed] == false)
				//TODO: hier gleich die liste checken
				m_usedNode[(*itUsed)] = true;
				itUsed++;
			}//while
			cliqueAdd.pushBack(neighbours);
#ifdef OGDF_DEBUG
			OGDF_ASSERT(cliqueOK(neighbours))
#endif
		}//if
		else
		{
			ListIterator<node> itUsed = neighbours->begin();
			while (itUsed.valid())
			{
				ListIterator<node> itDel = itUsed;
				//delete (*itDel);
				itUsed++;
				neighbours->del(itDel);
				//neighbours->remove(itDel);
			}//while
			//neighbours->clear();
			delete neighbours; //hier vielleicht clear?
		}

		itNode++;
	}//while

	cliqueList.conc(cliqueAdd);

}//postprocess

int CliqueFinder::evaluate(node v, EdgeArray<bool> &usableEdge)
{
	int value = 0;
	//Simple version: run through the neighbourhood of
	//every node and try to find a triangle path back
	//(triangles over neighbours are mandatory for cliques)
	//we run through the neighbourhood of v up to depth
	//2 and check if we can reach v again
	//erst mal die einfache Variante ohne Zwischenspeicherung
	//der Ergebnisse fuer andere Knoten
	//TODO: Geht das auch effizienter als mit Nodearray?
	NodeArray<bool> neighbour(*m_pCopy, false);
	adjEntry adj1;
	forall_adj(adj1, v)
	{
		if (!usableEdge[adj1->theEdge()]) continue;
		node w = adj1->twinNode();
		if (!m_usedNode[w]) neighbour[w] = true;
	}//foralladj

	//this loop counts every triangle (twice)
	forall_adj(adj1, v)
	{
		if (!usableEdge[adj1->theEdge()]) continue;
		adjEntry adj2;
		node w = adj1->twinNode();
		if (m_usedNode[w]) continue; //dont use paths over other cliques
		forall_adj(adj2, w)
		{
			if (!usableEdge[adj2->theEdge()]) continue;
			node u = adj2->twinNode();
			if (m_usedNode[u]) continue; //dont use paths over other cliques
			if (neighbour[u])
			{
				value++;
			}//if
		}//foralladj
	}//foralladj
	return value;
}//evaluate

//searchs for a clique around node v in list neighbours
//runs through list and performs numRandom additional
//random runs, permuting the sequence
//the result is returned in the list neighbours
void CliqueFinder::findClique(
	node v,
	List<node> &neighbours,
	int numRandom)
{
	//TODO:should be realdegree
	if (v->degree() < m_minDegree) neighbours.clear();
	List<node> clique;  //used to check clique criteria
	OGDF_ASSERT(!m_usedNode[v])
	clique.pushBack(v); //mandatory
	//we could assume that neighbours are always neighbours
	//of v and push the first node. too, here
	ListIterator<node> itNode = neighbours.begin();
	while (itNode.valid())
	{
		if ( ((*itNode)->degree() < clique.size()) ||
			((*itNode)->degree() < m_minDegree) )
		{
			ListIterator<node> itDel = itNode;
			//delete (*itDel);
			itNode++;
			neighbours.del(itDel); //remove

			continue;
		}
		if (allAdjacent((*itNode), &clique))
		{
			clique.pushBack((*itNode));
			itNode++;
		}
		else
		{
			ListIterator<node> itDel = itNode;
			itNode++;
			neighbours.del(itDel);
		}

	}//while
	//we can stop if the degree is smaller than the clique size
	int k;
	for (k = 0; k <numRandom; k++)
	{
		//TODO
	}//for



}//findclique


void CliqueFinder::setResults(NodeArray<int> & cliqueNum)
{
	//Todo: Assert that cliqueNum is an array on m_pGraph
	//set the clique numbers for the original nodes, -1 if not
	//member of a clique
	node v;

	forall_nodes(v, *m_pGraph)
	{
		node w = m_pCopy->copy(v);
		OGDF_ASSERT(w)
		cliqueNum[v] = m_copyCliqueNumber[w];

	}//forallnodes
}//setresults

//set results values from copy node lists to original node
void CliqueFinder::setResults(List< List<node>* > &cliqueLists)
{
	if (!m_callByList) return;
	if (!m_pList)      return;

	//m_pList->clear(); //componentwise
	//run through the clique lists
	ListIterator< List<node>* > it = cliqueLists.begin();
	while (it.valid())
	{
		List<node> l_list; //does this work locally?
		//run through the cliques
		ListIterator<node> itNode = (*it)->begin();
		while(itNode.valid())
		{
			node u = m_pCopy->original((*itNode));
			if (u)
				l_list.pushBack(u);
			itNode++;
		}//while

		m_pList->pushBack(l_list);
		it++;
	}//while
}//setresults

//same as above
void CliqueFinder::setResults(List< List<node> > &cliqueLists)
{
	//run through the clique lists
	ListIterator< List<node> > it = cliqueLists.begin();
	while (it.valid())
	{
		//run through the cliques
		ListIterator<node> itNode = (*it).begin();
		while(itNode.valid())
		{
			itNode++;
		}//while

		it++;
	}//while
}//setresults

//Graph must be parallelfree
//checks if v is adjacent to (min. m_density percent of) all nodes in vList
inline bool CliqueFinder::allAdjacent(node v, List<node>* vList)
{
	int threshold = int(ceil(max(1.0, ceil((vList->size()*m_density/100.0)))));
	//we do not want to run into some rounding error if m_density == 100
	if (m_density == 100) {if (v->degree() < vList->size()) return false;}
	else if (DIsLess(v->degree(), threshold)) return false;
	if (vList->size() == 0) return true;
	int adCount = 0;
	//Check: can the runtime be improved, e.g. by an adjacency oracle
	//or by running degree times through the list?
	NodeArray<bool> inList(*m_pCopy, false);//(v->graphOf()), false);
	ListIterator<node> it = vList->begin();
	while (it.valid())
	{
		inList[(*it)] = true;
		it++;
	}//while

	adjEntry adE = v->firstAdj();
	for (int i = 0; i < v->degree(); i++)
	{
		if (inList[adE->twinNode()])
			adCount++;
		adE = adE->cyclicSucc();
	}//for

	//we do not want to run into some rounding error if m_density == 100
	if (m_density == 100) { if (adCount == vList->size()) return true;}
	else if (DIsGreaterEqual(adCount, threshold))
		return true;

	return false;
}//allAdjacent

//-----------------------------------------------------------------------------
//Debug

//check

void CliqueFinder::checkCliques(List< List<node>* > &cliqueList, bool sizeCheck)
{
	//check if size is ok and if all nodes are mutually connected
	ListIterator< List<node>* > itList = cliqueList.begin();

	while (itList.valid())
	{
		if (sizeCheck)
		{
			OGDF_ASSERT((*itList)->size()> m_minDegree)
		}
		OGDF_ASSERT(cliqueOK((*itList)))

		itList++;
	}//while

}//checkCliques

bool CliqueFinder::cliqueOK(List<node> *clique)
{
	bool result = true;
	NodeArray<int> connect(*m_pCopy, 0);
	ListIterator<node> itV = clique->begin();

	while (itV.valid())
	{
		adjEntry adj1;
		forall_adj(adj1, (*itV))
		{
			connect[adj1->twinNode()]++;

		}//foralladj
		itV++;
	}//while
	itV = clique->begin();
	while (itV.valid())
	{
		if (m_density == 100)
		{
			if (connect[(*itV)] < (clique->size()-1))
				return false;
		}
		else
		{
			//due to the current heuristics, we can not guarantee any
			//value
			//TODO:postprocess and delete all "bad" nodes
			//double minVal = (clique->size()-1)*m_density/100.0;
			//if (DIsLess(connect[(*itV)], minVal))
			//	return false;
		}
		itV++;
	}//while

	return result;
}//cliqueOK


//output
#ifdef OGDF_DEBUG
void CliqueFinder::writeGraph(Graph &G, NodeArray<int> &cliqueNumber, const String &fileName)
{
	GraphAttributes AG(G, GraphAttributes::nodeGraphics | GraphAttributes::nodeColor |
		GraphAttributes::nodeLabel);
	node v;
	//char *hexString = new char[7];
	String colorString;
	forall_nodes(v, G)
	{
		colorString = "#";
		char s1[8];
		char s2[8];
		char s3[8];
		int num = cliqueNumber[v];
		int col1, col2, col3;
		if (num != -1)
		{
			col1 = abs(((num * 191) + 123) % 256);
			col2 = abs(((num * 131) + 67) % 256);
			col3 = abs(((num * 7) + 17) % 256);
		}
		else col1 = col2 = col3 = 0;

		ogdf::sprintf(s1,8,"%02X",col1);//s1[2] = '0';
		ogdf::sprintf(s2,8,"%02X",col2);//s2[2] = '0';
		ogdf::sprintf(s3,8,"%02X",col3);//s3[2] = '0';
		////String st1
		colorString+=s1[0];colorString+=s1[1];
		colorString+=s2[0];colorString+=s2[1];
		colorString+=s3[0];colorString+=s3[1];
		AG.colorNode(v) = colorString;
		AG.labelNode(v).sprintf("%d", num);

	}//forall_nodes

	AG.writeGML(fileName);

	//delete hexString;
}
#endif

}//namespace ogdf
