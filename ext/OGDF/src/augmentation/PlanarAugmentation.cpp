/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief planar biconnected augmentation approximation algorithm
 *
 * \author Bernd Zey
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


#include <ogdf/augmentation/PlanarAugmentation.h>

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/decomposition/DynamicBCTree.h>


// for debug-outputs
//#define PLANAR_AUGMENTATION_DEBUG

// for checking planarity directly after inserting a new edge
//   and additional planarity tests after each augmentation round
//#define PLANAR_AUGMENTATION_DEBUG_PLANARCHECK


namespace ogdf {


/********************************************************
 *
 * implementation of class PALabel
 *
 *******************************************************/

void PALabel::removePendant(node pendant)
{
	if (m_pendants.size() > 0){
		ListIterator<node> it = m_pendants.begin();
		for (; it.valid(); ++it)
			if ((*it) == pendant){
				m_pendants.del(it);
				break;
			}
	}
}


/********************************************************
 *
 * implementation of class PlanarAugmentation
 *
 *******************************************************/


//	----------------------------------------------------
//	doCall
//
//  ----------------------------------------------------
void PlanarAugmentation::doCall(Graph& g, List<edge>& L)
{
	m_nPlanarityTests = 0;

	L.clear();
	m_pResult = &L;

	m_pGraph = &g;

	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "Graph G has no self loops = " << isLoopFree(*m_pGraph) << endl;
		cout << "Graph G is planar         = " << isPlanar(*m_pGraph)<< endl;
		cout << "Graph G is connected      = " << isConnected(*m_pGraph) << endl;
		cout << "Graph G is biconnected    = " << isBiconnected(*m_pGraph) << endl;
	#endif

	// create the bc-tree
	if (m_pGraph->numberOfNodes() > 1){

		if (!isConnected(*m_pGraph)){
			if(m_pGraph->numberOfEdges() == 0){
				// one edge is required
				m_pResult->pushBack(m_pGraph->newEdge(m_pGraph->firstNode(), m_pGraph->firstNode()->succ()));
			}

			makeConnectedByPendants();
		}

		m_pBCTree = new DynamicBCTree(*m_pGraph);

		// init the m_adjNonChildren-NodeArray with all adjEntries of the bc-tree
		m_adjNonChildren.init(m_pBCTree->m_B);

		node v;
		adjEntry adj;
		forall_nodes(v, m_pBCTree->bcTree()){
			if (v->firstAdj() != 0){
				m_adjNonChildren[v].pushFront(v->firstAdj());
				adj = v->firstAdj()->cyclicSucc();
				while (adj != v->firstAdj()){
					m_adjNonChildren[v].pushBack(adj);
					adj = adj->cyclicSucc();
				}
			}
		}
		m_isLabel.init(m_pBCTree->bcTree(), 0);
		m_belongsTo.init(m_pBCTree->bcTree(), 0);

		// call main function
		augment();
	}
}



//	----------------------------------------------------
//	makeConnectedByPendants()
//
//		makes graph connected by inserting edges between
//		  nodes of pendants of the connected components
//
//  ----------------------------------------------------
void PlanarAugmentation::makeConnectedByPendants()
{
	DynamicBCTree bcTreeTemp(*m_pGraph, true);

	NodeArray<int> components;
	components.init(*m_pGraph, 0);

	int compCnt = connectedComponents(*m_pGraph, components);

	List<node> getConnected;

	Array<bool> compConnected(compCnt);
	for (int i=0; i<compCnt; i++){
		compConnected[i] = false;
	}

	node v;
	forall_nodes(v, *m_pGraph){
		if (v->degree() == 0){
			// found a seperated node that will be connected
			getConnected.pushBack(v);
			compConnected[components[v]] = true;
		}
	}

	forall_nodes(v, *m_pGraph){
		if ((compConnected[components[v]] == false) && (bcTreeTemp.bcproper(v)->degree() <= 1)){
			// found a node that will be connected
			getConnected.pushBack(v);
			compConnected[components[v]] = true;
		}
	}

	ListIterator<node> it = getConnected.begin();
	ListIterator<node> itBefore = getConnected.begin();
	while (it.valid()){
		if (it != itBefore){
			// insert edge between it and itBefore
			m_pResult->pushBack(m_pGraph->newEdge(*it, *itBefore));
			itBefore++;
		}
		it++;
	}
}



//	----------------------------------------------------
//	augment()
//
//		the main augmentation function
//
//  ----------------------------------------------------
void PlanarAugmentation::augment()
{
	node v, rootPendant = 0;

	// first initialize the list of pendants
	forall_nodes(v, m_pBCTree->bcTree()){
		if (v->degree() == 1){
			#ifdef PLANAR_AUGMENTATION_DEBUG
				cout << "augment(): found pendant with index " << v->index();
			#endif
			if (m_pBCTree->parent(v) == 0){
				rootPendant = v;
				#ifdef PLANAR_AUGMENTATION_DEBUG
					cout << " is root! (also inserted into pendants-list!)" << endl << flush;
				#endif
			}
			else{
				#ifdef PLANAR_AUGMENTATION_DEBUG
					cout << endl << flush;
				#endif
			}
			m_pendants.pushBack(v);
		}
	}

	if (rootPendant != 0){
		// the root of the bc-tree is also a pendant
		// this has to be changed

		node bAdjNode = rootPendant->firstAdj()->twinNode();

		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "augment(): changing root in bc-tree because root is a pendant!" << endl << flush;
			cout << "augment(): index of old root = " << rootPendant->index() << ", new root = " << bAdjNode->index() << endl << flush;
		#endif

		// modify the bc-tree-structure
		modifyBCRoot(rootPendant, bAdjNode);
	}

	// call reduceChain for all pendants
	if (m_pendants.size() > 1){
		ListIterator<node> it = m_pendants.begin();
		for (; it.valid(); ++it){
			reduceChain((*it));
		}
	}

	// it can appear that reduceChain() inserts some edges
	//  in case of non-planarity (paPlanarity)
	// so there are new pendants and obsolete pendants
	//  the obsolete pendants are collected in m_pendantsToDel
	if (m_pendantsToDel.size() > 0){
		ListIterator<node> delIt = m_pendantsToDel.begin();
		for (; delIt.valid(); delIt = m_pendantsToDel.begin()){
			deletePendant(*delIt);
			m_pendantsToDel.del(delIt);
		}
	}

	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "augment(): after reduceChain() for every pendant:" << endl;
		cout << "           #labels = " << m_labels.size() << endl;
		cout << "           #pendants = " << m_pendants.size() << endl << endl;
		cout << "STARTING MAIN LOOP:" << endl;
		cout << endl << flush;
	#endif

	// main loop
	while(!m_labels.empty()){
		// foundMatching indicates if there are 2 labels that can be connected
		bool foundMatching;
		// labels first and second are going to be computed by findMatching
		// and foundMatching=true or foundMatching=false
		// first is always != 0 after findMatching(...)
		pa_label first, second = 0;

		foundMatching = findMatching(first, second);

		// no matching labels were found
		if (!foundMatching){

			// we have only one label
			if (m_labels.size() == 1){

				if (m_pendants.size() > 1)
					//m_labels.size() == 1 &&  m_pendants.size() > 1
					// join the pendants of this label
					joinPendants(first);
				else{
					//m_labels.size() == 1 &&  m_pendants.size() == 1
					connectInsideLabel(first);
				}
			}
			else{
				// m_labels.size() > 1

				if (first->size() == 1){
					// m_labels.size() > 1 && first->size() == 1
					// connect the
					connectInsideLabel(first);
				}
				else{
					// m_labels.size() > 1 && first->size() > 1
					// so connect all pendants of label first
					joinPendants(first);
				}
			}
		}
		else{

			// foundMatching == true
			connectLabels(first, second);
		}

		// output after each round:
		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << endl << "augment(): output after one round:" << endl;
			cout         << "           #labels   = " << m_labels.size() << endl;
			cout         << "           #pendants = " << m_pendants.size() << endl;
			#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
				cout << "graph is planar == " << isPlanar(*m_pGraph) << endl;
				cout << "graph is biconnected == " << isBiconnected(*m_pGraph) << endl;
			#endif
			cout << endl << flush;

			ListIterator<pa_label> labelIt = m_labels.begin();
			int pos = 1;
			for (; labelIt.valid(); labelIt++){
				cout << "pos " << pos << ": ";
				if ((m_isLabel[(*labelIt)->parent()]).valid())
					cout << " OK, parent-index = " << (*labelIt)->parent()->index()
						<< ", size = " << (*labelIt)->size() << endl << flush;
				else
					cout << " ERROR, parent-index = " << (*labelIt)->parent()->index()
						<< ", size = " << (*labelIt)->size()<< endl << flush;

				pos++;
			}
			cout << endl << flush;
		#endif
		// : output after each round

	}//main loop

	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << endl << "FINISHED MAIN LOOP" << endl << endl;
		cout << "# planarity tests = " << m_nPlanarityTests << endl;
		#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARITY
			cout << "resulting Graph is biconnected = " << isBiconnected(*m_pGraph) << endl;
			cout << "resulting Graph is planar = " << isPlanar(*m_pGraph) << endl;
		#endif
		cout << endl << flush;
	#endif

	terminate();
}



//	----------------------------------------------------
//	reduceChain(node p, label labelOld)
//
//		finds the "parent" (->label) for a pendant p of the BC-Tree
// 		 and creates a new label or inserts the pendant to another
//	     label
//		reduceChain can also insert edges in case of paPlanarity
//
//  ----------------------------------------------------
void PlanarAugmentation::reduceChain(node p, pa_label labelOld)
{
	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "reduceChain(" << p->index() << ")";
	#endif

	// parent = parent of p in the BC-Tree
	// if p is the root, then parent == 0
	node parent = m_pBCTree->DynamicBCTree::parent(p);

	// last is going to be the last cutvertex in the computation of followPath()
	node last;
	paStopCause stopCause;

	// traverse from parent to the root of the bc-tree and check several
	// conditions. last is going to be the last cutvertex on this path
	stopCause = followPath(parent, last);

	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << ", stopCause == ";
		switch(stopCause){
			case paPlanarity:
				cout << "paPlanarity" << endl << flush;
				break;
			case paCDegree:
				cout << "paCDegree" << endl << flush;
				break;
			case paBDegree:
				cout << "paBDegree" << endl << flush;
				break;
			case paRoot:
				cout << "paRoot" << endl << flush;
				break;
		}
	#endif


	if (stopCause == paPlanarity){
		node adjToCutP    = adjToCutvertex(p);
		node adjToCutLast = adjToCutvertex(m_pBCTree->DynamicBCTree::parent(last), last);

		// computes path in bc-tree between bcproper(adjToCutP) and bcproper(adjToCutLast)
		SList<node>& path = m_pBCTree->findPath(adjToCutP, adjToCutLast);

		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "reduceChain(): inserting edge between " << adjToCutP->index()
				 << " and " << adjToCutLast->index() << endl << flush;
		#endif

		// create new edge
		edge e = m_pGraph->newEdge(adjToCutP, adjToCutLast);

		// insert the edge into the result-list
		m_pResult->pushBack(e);

		// update the bc-Tree with new edge
		m_pBCTree->updateInsertedEdge(e);

		// find the new arised pendant
		node newPendant = m_pBCTree->find(p);

		if (newPendant != p){
			// delete the old pendant
			// cannot delete the pendant immediatly
			// because that would affect the outer loop in augment()
			m_pendantsToDel.pushBack(p);
			// insert the new arised pendant
			// at the front of m_pendants becuse that doesn't affect the outer loop in augment()
			m_pendants.pushFront(newPendant);
		}

		// updating m_adjNonChildren
		updateAdjNonChildren(newPendant, path);

		// check if newPendant is the new root of the bc-tree
		if (m_pBCTree->DynamicBCTree::parent(newPendant) == 0){
			#ifdef PLANAR_AUGMENTATION_DEBUG
				cout << "reduceChain(): new arised pendant is the new root of the bc-tree, it has degree "
					 << m_pBCTree->m_bNode_degree[newPendant] << endl << flush;
			#endif

			node newRoot = (*(m_adjNonChildren[newPendant].begin()))->twinNode();

			 // modify bc-tree-structure
			 modifyBCRoot(newPendant, newRoot);
		}

		delete(&path);

		// delete label if necessary
		if (labelOld != 0){
			deleteLabel(labelOld);
		}

		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "reduceChain(): calling reduceChain() with newPendant = " << newPendant->index() << endl << flush;
		#endif

		// call reduceChain with the new arised pendant
		reduceChain(newPendant);
	}

	pa_label l;

	if (stopCause == paCDegree || stopCause == paRoot){

		if (labelOld != 0){
			if (labelOld->head() == last){
				// set the stop-cause
				labelOld->stopCause(stopCause);
			}
			else
				deleteLabel(labelOld);
		}

		if (m_isLabel[last].valid()){
			// l is the label that last is the head of
			l = *(m_isLabel[last]);
			// add the actual pendant p to l
			addPendant(p, l);
			// set the stop-cause
			l->stopCause(stopCause);
		}
		else{
			newLabel(last, p, stopCause);
		}
	}

	if (stopCause == paBDegree){
		if (labelOld != 0){
			if (labelOld->head() != last){
				deleteLabel(labelOld);
				newLabel(last, p, paBDegree);
			}
			else{
				labelOld->stopCause(paBDegree);
			}
		}
		else{
			newLabel(last, p, paBDegree);
		}
	}
} // reduceChain()



//	----------------------------------------------------
//	followPath(node v, node &last)
//
//		traverses the BC-Tree upwards from v
//		  (v is always a parent of a pendant)
//		last becomes the last cutvertex before we return
//
//  ----------------------------------------------------
paStopCause PlanarAugmentation::followPath(node v, node& last)
{
	last = 0;
	node bcNode = m_pBCTree->find(v);

	if (m_pBCTree->typeOfBNode(bcNode) == BCTree::CComp){
		last = bcNode;
	}

	while (bcNode != 0){
		int deg = m_pBCTree->m_bNode_degree[bcNode];

		if (deg > 2){
			if (m_pBCTree->typeOfBNode(bcNode) == BCTree::CComp){
				last = bcNode;
				return paCDegree;
			}
			else
				return paBDegree;
		}

		// deg == 2 (case deg < 2 cannot occur)
		if (m_pBCTree->typeOfBNode(bcNode) == BCTree::CComp){
			last = bcNode;
		}
		else{
			// bcNode is a BComp and degree is 2
			if (m_pBCTree->numberOfNodes(bcNode) > 4){
				// check planarity if number of nodes > 4
				// because only than a K5- or k33-Subdivision can be included

				node adjBCNode = 0;

				bool found = false;
				SListIterator<adjEntry> childIt = m_adjNonChildren[bcNode].begin();
				while (!found && childIt.valid()){
					if (m_pBCTree->find((*childIt)->twinNode()) != last){
						found = true;
						adjBCNode = m_pBCTree->find((*childIt)->twinNode());
					}
					childIt++;
				}

				// get nodes in biconnected-components graph of m_pBCTree
				node hNode = m_pBCTree->m_bNode_hRefNode[last];
				node hNode2 = m_pBCTree->m_bNode_hRefNode[adjBCNode];

				// check planarity for corresponding graph-nodes of hNode and hNode2
				if (!planarityCheck(m_pBCTree->m_hNode_gNode[hNode],
									m_pBCTree->m_hNode_gNode[hNode2])){
					return paPlanarity;
				}
			}
		}
		// iterate to parent node
		bcNode = m_pBCTree->DynamicBCTree::parent(bcNode);
	}
	// reached the bc-tree-root
	return paRoot;
}



//	----------------------------------------------------
//	planarityCheck(node v1, node v2)
//
// 		checks planarity for the new edge (v1, v2)
// 		 v1 and v2 are nodes in the original graph
//
//  ----------------------------------------------------
bool PlanarAugmentation::planarityCheck(node v1, node v2)
{
	// first simple tests
	if (v1 == v2){
		return true;
	}

	// check if edge (v1, v2) already exists
	if (v1->firstAdj()->twinNode() == v2){
		return true;
	}
	adjEntry adjTest = v1->firstAdj()->cyclicSucc();
	while (adjTest != v1->firstAdj()){
		if (v1->firstAdj()->twinNode() == v2){
			return true;
		}
		adjTest = adjTest->cyclicSucc();
	}

	// test planarity for edge (v1, v2)
	edge e = m_pGraph->newEdge(v1, v2);

	m_nPlanarityTests++;

	bool planar = planarEmbed(*m_pGraph);

	// finally delete the edge
	m_pGraph->delEdge(e);

	return planar;
}



//	----------------------------------------------------
//	adjToCutvertex(node v, node cutvertex)
//
// 		returns the vertex in the original graph that
//  	 belongs to v (B-Component in the BC-Graph and pendant)
//  	 and is adjacent to the cutvertex (also node of the BC-Graph)
//		if cutvertex == 0 then the cutvertex of the parent of v
//		 is considered
//
//  ----------------------------------------------------
node PlanarAugmentation::adjToCutvertex(node v, node cutvertex)
{
	node nodeAdjToCutVertex;

	if (cutvertex == 0){

		// set nodeAdjToCutVertex to the node in the original graph,
		//  that corresponds to the parent (c-component) of v in the bc-tree
		nodeAdjToCutVertex = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[v]];

		// adj = adjEntry at the cutvertex
		adjEntry adj = nodeAdjToCutVertex->firstAdj();

		while (m_pBCTree->DynamicBCTree::bcproper(adj->twinNode()) != v)
			adj = adj->cyclicSucc();

		nodeAdjToCutVertex = adj->twinNode();

	}
	else{
		// set nodeAdjToCutVertex to the node in the original graph,
		//  corresponding to the cutvertex in the bc-tree
		nodeAdjToCutVertex = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[cutvertex]];

		// adj = adjEntry at the cutvertex
		adjEntry adj = nodeAdjToCutVertex->firstAdj();

		bool found = false;

		if (m_pBCTree->bComponent(nodeAdjToCutVertex, adj->twinNode()) == v){
			found = true;
			nodeAdjToCutVertex = adj->twinNode();
		}
		else{
			adj = adj->cyclicSucc();
			while ((!found) && (adj != nodeAdjToCutVertex->firstAdj())){
				if (m_pBCTree->bComponent(nodeAdjToCutVertex, adj->twinNode()) == v){
					nodeAdjToCutVertex = adj->twinNode();
					found = true;
				}
				adj = adj->cyclicSucc();
			}
		}
	}
	return nodeAdjToCutVertex;
}



//	----------------------------------------------------
//	findLastBefore(node pendant, node ancestor)
//
// 		returns the last vertex before ancestor
//		  on the path from pendant to ancestor
//
//  ----------------------------------------------------
node PlanarAugmentation::findLastBefore(node pendant, node ancestor)
{
	node bcNode = pendant;
	while ((bcNode) && (m_pBCTree->DynamicBCTree::parent(bcNode) != ancestor))
		bcNode = m_pBCTree->DynamicBCTree::parent(bcNode);

	if (!bcNode){
		// should never occur
		return 0;
	}

	return bcNode;
}



//	----------------------------------------------------
//	deletePendant(node p)
//
// 		deletes pendant p from the list of all pendants
//		deletes p also from the label it belongs to
//
//  ----------------------------------------------------
void PlanarAugmentation::deletePendant(node p, bool removeFromLabel)
{
	ListIterator<node> mPendantsIt = m_pendants.begin();

	bool deleted = false;
	while (!deleted && mPendantsIt.valid()){
		ListIterator<node> itSucc = mPendantsIt.succ();
		if ((*mPendantsIt) == p){
			m_pendants.del(mPendantsIt);
			deleted = true;
		}
		mPendantsIt = itSucc;
	}

	if ((removeFromLabel) && (m_belongsTo[p] != 0)){
		(m_belongsTo[p])->removePendant(p);
		m_belongsTo[p] = 0;
	}
}



//	----------------------------------------------------
//	removeAllPendants(label& l)
//
// 		deletes a label
//		and - if desired - removes the pendants belonging to l
//
//  ----------------------------------------------------
void PlanarAugmentation::removeAllPendants(pa_label& l)
{
	while (l->size() > 0){
		m_belongsTo[l->getFirstPendant()] = 0;
		l->removeFirstPendant();
	}
}



//	----------------------------------------------------
//	addPendant(pa_label& l, node pendant)
//
// 		adds a pendant p to a label l
//		re-inserts also l to m_labels
//
//  ----------------------------------------------------
void PlanarAugmentation::addPendant(node p, pa_label& l)
{
	m_belongsTo[p] = l;
	l->addPendant(p);

	node newParent = m_pBCTree->find(l->parent());

	m_labels.del(m_isLabel[l->parent()]);
	m_isLabel[newParent] = insertLabel(l);
}



//	----------------------------------------------------
//	joinPendants(pa_label& l)
//
// 		connects all pendants of the label
//
//  ----------------------------------------------------
void PlanarAugmentation::joinPendants(pa_label& l)
{
	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "joinPendants(): l->size()==" << l->size() << endl << flush;
	#endif

	node pendant1 = l->getFirstPendant();
	// delete pendant from m_pendants but not from the label it belongs to
	deletePendant(pendant1, false);

	SList<edge> newEdges;

	// traverse through pendant-list and connect them
	ListIterator<node> pendantIt = (l->m_pendants).begin();
	while (pendantIt.valid()){

		if (*pendantIt != pendant1){

			// delete pendant from m_pendants but not from the label it belongs to
			deletePendant(*pendantIt, false);

			#ifdef PLANAR_AUGMENTATION_DEBUG
				cout << "joinPendants(): connectPendants: " << pendant1->index()
					<< " and " << (*pendantIt)->index() << endl << flush;
			#endif

			// connect pendants and insert edge in newEdges
			newEdges.pushBack(connectPendants(pendant1, *pendantIt));

			// iterate pendant1
			pendant1 = *pendantIt;
		}
		pendantIt++;
	}

	// update new edges
	updateNewEdges(newEdges);

	removeAllPendants(l);

	SListIterator<edge> edgeIt = newEdges.begin();
	node newBlock = (m_pBCTree->DynamicBCTree::bcproper(*edgeIt));
	if (m_pBCTree->m_bNode_degree[newBlock] == 1){
		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "joinPendants(): new block " << newBlock->index() << " has degree 1 " << endl << flush;
		#endif

		m_belongsTo[newBlock] = l;
		addPendant(newBlock, l);
		m_pendants.pushBack(newBlock);

	}
	else{
		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "joinPendants(): new block has degree " << m_pBCTree->m_bNode_degree[newBlock] << endl << flush;
		#endif
		deleteLabel(l);
	}
}



//	----------------------------------------------------
//	connectInsideLabel(label& l)
//
// 		connects the only pendant of l with
//		  a computed "ancestor"
//
//  ----------------------------------------------------
void PlanarAugmentation::connectInsideLabel(pa_label& l)
{
	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "connectInsideLabel(): l->size() == " << l->size() << ", parent = " << l->parent()->index()
			 << ", head = " << l->head()->index() << endl << flush;
	#endif

	node head = l->head();
	node pendant = l->getFirstPendant();

	node ancestor = m_pBCTree->DynamicBCTree::parent(head);

	node v1 = adjToCutvertex(pendant);

	// check if head is the root of the BC-Tree
	if (ancestor == 0){
		node wrongAncestor = findLastBefore(pendant, head);

		SListIterator<adjEntry> adjIt = m_adjNonChildren[head].begin();
		bool found = false;
		while ((!found) && (adjIt.valid())){

			if (m_pBCTree->find((*adjIt)->twinNode()) != wrongAncestor){
				ancestor = m_pBCTree->find((*adjIt)->twinNode());
				found = true;
			}
			adjIt++;
		}
	}

	node v2 = adjToCutvertex(ancestor, head);

	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "connectInsideLabel(): inserting edge between " << v1->index() << " and " << v2->index() << endl << flush;
	#endif

	SList<edge> newEdges;
	edge e = m_pGraph->newEdge(v1, v2);
	newEdges.pushFront(e);

	#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
		if (!isPlanar(*m_pGraph))
			cout << "connectInsideLabel(): CRITICAL ERROR!!! inserted non-planar edge!!! (in connectInsideLabel())" << endl << flush;
	#endif

	updateNewEdges(newEdges);

	node newBlock = m_pBCTree->DynamicBCTree::bcproper(e);

	// delete label l, and also the pendant
	deleteLabel(l);

	if (m_pBCTree->m_bNode_degree[newBlock] == 1){
		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "connectInsideLabel(): new block " << newBlock->index() << " has degree 1... calling reduceChain() ";
		#endif
		m_pendants.pushBack(newBlock);
		if ((m_belongsTo[newBlock] != 0) && (m_belongsTo[newBlock]->size() == 1)){
			reduceChain(newBlock, m_belongsTo[newBlock]);
		}
		else{
			reduceChain(newBlock);
			// it can appear that reduceChain() inserts some edges
			// so there are new pendants and obsolete pendants
			//  the obsolete pendants are collected in m_pendantsToDel
			if (m_pendantsToDel.size() > 0){
				ListIterator<node> delIt = m_pendantsToDel.begin();
				for (; delIt.valid(); delIt = m_pendantsToDel.begin()){
					deletePendant(*delIt);
					m_pendantsToDel.del(delIt);
				}
			}
		}
	}
}



//	----------------------------------------------------
//	connectPendants(node pendant1, node pendant2)
//
// 		connects the two pendants with a new edge
//
//  ----------------------------------------------------
edge PlanarAugmentation::connectPendants(node pendant1, node pendant2)
{
	node v1 = adjToCutvertex(pendant1);
	node v2 = adjToCutvertex(pendant2);

	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "connectPendants(): inserting edge between " << v1->index() << " and " << v2->index() << endl << flush;
	#endif

	edge e = m_pGraph->newEdge(v1, v2);

	#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
		if (!(isPlanar(*m_pGraph)))
			cout << "connectLabels(): CRITICAL ERROR in connectPendants: inserted edge is not planar!!!" << endl << flush;
	#endif

	return e;
}



//	----------------------------------------------------
//	insertLabel(pa_label l)
//
// 		inserts a label l at the correct position in the list
//
//  ----------------------------------------------------
ListIterator<pa_label> PlanarAugmentation::insertLabel(pa_label l)
{
	if (m_labels.size() == 0){
		return m_labels.pushFront(l);
	}
	else{
		ListIterator<pa_label> it = m_labels.begin();
		while (it.valid() && ((*it)->size() > l->size())){
			it++;
		}
		if (!it.valid())
			return m_labels.pushBack(l);
		else
			return m_labels.insert(l, it, before);
	}
}



//	----------------------------------------------------
//	deleteLabel(pa_label& l, bool removePendants)
//
// 		deletes a label
//		and - if desired - removes the pendants belonging to l
//
//  ----------------------------------------------------
void PlanarAugmentation::deleteLabel(pa_label& l, bool removePendants)
{
	ListIterator<pa_label> labelIt = m_isLabel[l->parent()];

	m_labels.del(labelIt);
	m_isLabel[l->parent()] = 0;

	ListIterator<node> pendantIt = (l->m_pendants).begin();
	while (pendantIt.valid()){
		m_belongsTo[*pendantIt] = 0;
		pendantIt++;
	}

	if (removePendants){
		pendantIt = (l->m_pendants).begin();
		while (pendantIt.valid()){

			ListIterator<node> mPendantsIt = m_pendants.begin();

			bool deleted = false;
			while (!deleted && mPendantsIt.valid()){
				ListIterator<node> itSucc = mPendantsIt.succ();
				if ((*mPendantsIt) == *pendantIt){
					m_pendants.del(mPendantsIt);
					deleted = true;
				}
				mPendantsIt = itSucc;
			}
			pendantIt++;
		}
	}

	delete(l);
	l = 0;
}



//	----------------------------------------------------
//	connectLabels(pa_label first, pa_label second);
//
// 		connects the pendants of first with the pendants
//		  of second.
//		first.size() >= second.size()
//
//  ----------------------------------------------------
void PlanarAugmentation::connectLabels(pa_label first, pa_label second)
{
	ListIterator<node> pendantIt;

	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "connectLabels(), first->size()=="<< first->size() << " , second->size()=="
			<< second->size() << endl << flush;

		pendantIt = (first->m_pendants).begin();
		cout << "connectLabels(): label first = ";
		for (; pendantIt.valid(); pendantIt++){
			cout << (*pendantIt)->index() << ", ";
		}
		pendantIt = (second->m_pendants).begin();
		cout << " || " << endl << "label second = ";
		for (; pendantIt.valid(); pendantIt++){
			cout << (*pendantIt)->index() << ", ";
		}
		cout << endl << flush;
	#endif

	SList<edge> newEdges;
	pendantIt = (second->m_pendants).begin();

	// stores the pendants of label first that were connected
	// because first.size() => second.size()
	SList<node> getConnected;
	node v2;
	int n = 0;

	while (pendantIt.valid()){
		v2 = first->getPendant(n);
		getConnected.pushBack(v2);
		newEdges.pushBack(connectPendants(v2, *pendantIt));

		#ifdef PLANAR_AUGMENTATION_DEBUG_PLANARCHECK
			if (!(isPlanar(*m_pGraph)))
				cout << "connectLabels(): CRITICAL ERROR: inserted edge is not planar!!!" << endl << flush;
		#endif

		n++;
		pendantIt++;
	}

	updateNewEdges(newEdges);
	deleteLabel(second);

	node newBlock = m_pBCTree->DynamicBCTree::bcproper(newEdges.front());
	#ifdef PLANAR_AUGMENTATION_DEBUG
		cout << "connectLabels(): newBlock->index() == " << newBlock->index() << ", degree == "
			 << m_pBCTree->m_bNode_degree[newBlock] << endl << flush;
	#endif

	SListIterator<node> pendantIt2 = getConnected.begin();
	while (pendantIt2.valid()){

		//first->removePendant(*pendantIt2);
		deletePendant(*pendantIt2);

		pendantIt2++;
	}

	if (first->size() != 0){
		m_labels.del(m_isLabel[first->parent()]);
		m_isLabel[m_pBCTree->find(first->parent())] = insertLabel(first);

		pendantIt = (first->m_pendants).begin();
		for (; pendantIt.valid(); pendantIt++){
			m_belongsTo[m_pBCTree->find(*pendantIt)] = first;
		}
	}
	else{	// first->size() == 0
		deleteLabel(first);
	}

	if (m_pBCTree->m_bNode_degree[newBlock] == 1){

		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "connectLabels(): m_bNode_degree[" << newBlock->index() << "] == 1... calling reduceChain()" << endl << flush;
		#endif

		m_pendants.pushBack(newBlock);

		if ((m_belongsTo[newBlock] != 0) && (m_belongsTo[newBlock]->size() == 1)){
			reduceChain(newBlock, m_belongsTo[newBlock]);
		}
		else{
			reduceChain(newBlock);

			// it can appear that reduceChain() inserts some edges
			// so there are new pendants and obsolete pendants
			//  the obsolete pendants are collected in m_pendantsToDel
			if (m_pendantsToDel.size() > 0){
				ListIterator<node> delIt = m_pendantsToDel.begin();
				for (; delIt.valid(); delIt = m_pendantsToDel.begin()){
					deletePendant(*delIt);
					m_pendantsToDel.del(delIt);
				}
			}
		}
	}
	else{
		#ifdef PLANAR_AUGMENTATION_DEBUG
			cout << "connectLabels(): newBlock is no new pendant ! degree == " << m_pBCTree->m_bNode_degree[newBlock] << endl << flush;
		#endif
	}
}



//	----------------------------------------------------
//	newLabel(node cutvertex, node block, paStopCause whyStop)
//
// 		creates a new label and inserts it into m_labels
//
//  ----------------------------------------------------
pa_label PlanarAugmentation::newLabel(node cutvertex, node p, paStopCause whyStop)
{
	pa_label l = OGDF_NEW PALabel(0, cutvertex, whyStop);
	l->addPendant(p);
	m_belongsTo[p] = l;
	m_isLabel[cutvertex] = m_labels.pushBack(l);
	return l;
}



//	----------------------------------------------------
//	findMatching(pa_label& first, pa_label& second)
//
// 		trys to find two matching labels
//		first will be the label with max. size that has
//		  a matching label
//
//  ----------------------------------------------------
bool PlanarAugmentation::findMatching(pa_label& first, pa_label& second)
{
	first = m_labels.front();
	second = 0;
	pa_label l = 0;

	ListIterator<pa_label> it = m_labels.begin();
	while (it.valid()){
		second = *it;

		if (second != first){
			if ( (l != 0) && (second->size() < l->size()) ){
				second = l;
				return true;
			}

			if (l != 0){

				if ( connectCondition(second, first)
					&& planarityCheck(m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[second->head()]],
					m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[first->head()]]) )
				{
						return true;
				}
			}
			else{	// l == 0

				if ( planarityCheck(m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[second->head()]],
									m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hRefNode[first->head()]]) ){
					if (connectCondition(second, first)){
						return true;
					}
					l = second;
				}
			}
		}
		it++;
	}

	if (!l)
		return false;

	second = l;
	return true;
}



//	----------------------------------------------------
//	connectCondition(pa_label a, pa_label b)
//
// 		checks the connect-condition for label a and b
//
//  ----------------------------------------------------
bool PlanarAugmentation::connectCondition(pa_label a, pa_label b)
{
	bool found = false;

	if ( (a->isBLabel()) && (b->size() == 1) ){
		found = true;
	}

	int deg1 = m_pBCTree->m_bNode_degree[m_pBCTree->find(a->head())] - b->size() +1;
	int deg2 = m_pBCTree->m_bNode_degree[m_pBCTree->find(b->head())] - b->size() +1;

	if ((deg1 > 2) && (deg2 > 2)){
		return true;
	}
	if ((deg1 > 2) || (deg2 > 2)){
		if (found){
			return true;
		}
		else
			found = true;
	}
	SList<node> *path = m_pBCTree->findPathBCTree(a->head(), b->head());
	SListIterator<node> it = path->begin();
	node bcNode;

	for (; it.valid(); it++){
		bcNode = m_pBCTree->find(*it);

		if ((bcNode != a->parent()) && (bcNode != b->parent())){
			if (m_pBCTree->m_bNode_degree[bcNode] > 2){
				if (found) {
					delete path;
					return true;
				} else
					found = true;
			}
			if ((m_pBCTree->typeOfBNode(bcNode) == BCTree::BComp)
				&& (m_pBCTree->m_bNode_degree[bcNode] > 3))
			{
				delete path;
				return true;
			}
		}
	}

	delete path;
	return !found;
}



//	----------------------------------------------------
//	updateAdjNonChildren()
//
// 		update of m_adjNonChildren
//		newBlock is the node all nodes on path now belong to
//
//  ----------------------------------------------------
void PlanarAugmentation::updateAdjNonChildren(node newBlock, SList<node>& path)
{
	SListIterator<node> pathIt = path.begin();

	SListIterator<adjEntry> childIt = m_adjNonChildren[newBlock].begin();
	SListIterator<adjEntry> prevIt  = m_adjNonChildren[newBlock].begin();
	// first update m_adjNonChildren[newBlock] by deleting wrong adjEntries
	while (childIt.valid()){
		if (m_pBCTree->find((*childIt)->twinNode()) == newBlock){
			if (childIt == m_adjNonChildren[newBlock].begin()){
				m_adjNonChildren[newBlock].popFront();
				childIt = m_adjNonChildren[newBlock].begin();
				prevIt  = m_adjNonChildren[newBlock].begin();
			}
			else{
				childIt = prevIt;
				m_adjNonChildren[newBlock].delSucc(prevIt);
				childIt++;
			}
		}
		else{
			prevIt = childIt;
			childIt++;
		}
	}

	// now run through list of all path-nodes
	// and update m_adjNonChildren[pathIt] if they do not belong to another bc-node
	// or insert adjEntries to m_adjNonChildren[newBlock]
	while (pathIt.valid()){

		if (*pathIt != newBlock){
			if (*pathIt == m_pBCTree->find(*pathIt)){

				childIt = m_adjNonChildren[*pathIt].begin();
				prevIt  = m_adjNonChildren[*pathIt].begin();

				while (childIt.valid()){
					if (m_pBCTree->find((*childIt)->twinNode()) == (*pathIt)){
						if (childIt == m_adjNonChildren[*pathIt].begin()){
							m_adjNonChildren[*pathIt].popFront();
							childIt = m_adjNonChildren[*pathIt].begin();
							prevIt  = m_adjNonChildren[*pathIt].begin();
						}
						else{
							childIt = prevIt;
							m_adjNonChildren[*pathIt].delSucc(prevIt);
							childIt++;
						}
					}
					else{
						prevIt = childIt;
						childIt++;
					}
				}
			}
			else{	// (*pathIt != m_pBCTree->find(*pathIt))
				childIt = m_adjNonChildren[*pathIt].begin();

				while (childIt.valid()){
					if (m_pBCTree->find((*childIt)->twinNode()) != newBlock){
						// found a child of *pathIt, that has an adjacent bc-node
						//  that doesn't belong to newBlock
							m_adjNonChildren[newBlock].pushBack(*childIt);
					}
					childIt++;
				}
				m_adjNonChildren[*pathIt].clear();
			}
		}
		pathIt++;
	}
}



//	----------------------------------------------------
//	updateBCRoot()
//
// 		modifys the root of the bc-tree
//
//  ----------------------------------------------------
void PlanarAugmentation::modifyBCRoot(node oldRoot, node newRoot)
{
	// status before updates:
	//   m_pBCTree->m_bNode_hRefNode[oldRoot] = 0
	//   m_pBCTree->m_bNode_hParNode[oldRoot] = 0

	//   m_pBCTree->m_bNode_hRefNode[newRoot] = single isolated vertex in b-comp-graph of this c-comp
	//   m_pBCTree->m_bNode_hParNode[newRoot] = cutvertex in b-comp-graph corresponding to rootPendant

	// updates:
	//   for the old root:
	m_pBCTree->m_bNode_hRefNode[oldRoot] = m_pBCTree->m_bNode_hParNode[newRoot];
	m_pBCTree->m_bNode_hParNode[oldRoot] = m_pBCTree->m_bNode_hRefNode[newRoot];

	//   for the new root:
	// m_pBCTree->m_bNode_hRefNode[newRoot] = no update required;
	m_pBCTree->m_bNode_hParNode[newRoot] = 0;
}



//	----------------------------------------------------
//	updateNewEdges(const SList<edge> &newEdges)
//
// 		updates the bc-tree-structure and m_adjNonChildren,
//		also adds all edges of newEdges to m_pResult
//
//  ----------------------------------------------------
void PlanarAugmentation::updateNewEdges(const SList<edge> &newEdges)
{
	SListConstIterator<edge> edgeIt = newEdges.begin();
	while (edgeIt.valid()){
		m_pResult->pushBack(*edgeIt);

		SList<node>& path = m_pBCTree->findPath((*edgeIt)->source(), (*edgeIt)->target());

		m_pBCTree->updateInsertedEdge(*edgeIt);
		node newBlock = m_pBCTree->DynamicBCTree::bcproper(*edgeIt);

		updateAdjNonChildren(newBlock, path);

		if ((m_pBCTree->parent(newBlock) == 0)
		&& (m_pBCTree->m_bNode_degree[newBlock] == 1)) {
			// the new block is a pendant and also the new root of the bc-tree
			node newRoot = 0;
			newRoot = (*(m_adjNonChildren[newBlock].begin()))->twinNode();
			 modifyBCRoot(newBlock, newRoot);
		} //if ((m_pBCTree->parent(newBlock) == 0) && (m_pBCTree->m_bNode_degree[newBlock] == 1))

		delete(&path);
		edgeIt++;
	} // while (edgeIt.valid)
}



//	----------------------------------------------------
//	terminate()
//
// 		cleanup before finish
//
//  ----------------------------------------------------
void PlanarAugmentation::terminate()
{
	while (m_labels.size() > 0){
		pa_label l = m_labels.popFrontRet();
		delete (l);
	}

	m_pendants.clear();
	node v;
	forall_nodes(v, m_pBCTree->m_B)
		m_adjNonChildren[v].clear();

	delete(m_pBCTree);
}

} // end namespace ogdf
