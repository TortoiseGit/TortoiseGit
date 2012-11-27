/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief planar biconnected augmentation algorithm with fixed
 * 		  embedding
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


#include <ogdf/augmentation/PlanarAugmentationFix.h>

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/decomposition/DynamicBCTree.h>


// for debug-outputs
//#define PLANAR_AUGMENTATION_FIX_DEBUG

// for additional planarity checks after each "round"
//#define PLANAR_AUGMENTATION_FIX_DEBUG_PLANARCHECK


namespace ogdf {


/********************************************************
 *
 * implementation of class PlanarAugmentationFix
 *
 *******************************************************/

//	----------------------------------------------------
//	doCall
//
//  ----------------------------------------------------
void PlanarAugmentationFix::doCall(Graph& g, List<edge>& L)
{
	L.clear();
	m_pResult = &L;

 	m_pGraph = &g;
 	m_pEmbedding = new CombinatorialEmbedding(*m_pGraph);

	NodeArray<bool> activeNodes(*m_pGraph, false);
	List<node> activeNodesList;

	face actFace;

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "original graph:" << endl << flush;
		node n;
		forall_nodes(n, *m_pGraph){
			cout << n->index() << ": ";
			adjEntry nAdj;
			forall_adj(nAdj, n)
				cout << nAdj << " ";
			cout << endl << flush;
		}
	#endif

	List<face> faces;

	forall_faces(actFace, *m_pEmbedding){
		faces.pushBack(actFace);
	}

	m_eCopy.init(*m_pGraph, 0);
	m_graphCopy.createEmpty(*m_pGraph);

	while (faces.size() > 0){
		actFace = faces.popFrontRet();

		adjEntry adjOuterFace = 0;

		adjEntry adjFirst = actFace->firstAdj();
		if (m_pEmbedding->leftFace(adjFirst) != actFace)
			adjFirst = adjFirst->twin();

		adjEntry adjFace = adjFirst;

		if (m_pEmbedding->numberOfFaces() == 1){
			// we just have only one face (this is the outer face)
			adjOuterFace = adjFace;
		}

		#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			cout << "======================" << endl
				 << "face " << actFace->index() <<":" << endl
				 << adjFace->theNode()->index() << "(" << adjFace << ")"<< ", ";
		#endif

		activeNodesList.pushBack(adjFace->theNode());
		activeNodes[adjFace->theNode()] = true;
		adjFace = adjFace->twin()->cyclicSucc();

		bool augmentationRequired = false;

		while (adjFace != adjFirst){
			if ((adjOuterFace == 0) && (m_pEmbedding->leftFace(adjFace) != m_pEmbedding->rightFace(adjFace)))
				adjOuterFace = adjFace;

			if (activeNodes[adjFace->theNode()] == false){
				activeNodesList.pushBack(adjFace->theNode());
				activeNodes[adjFace->theNode()] = true;

				#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
					cout << adjFace->theNode()->index() << "(" << adjFace << ")"<< ", ";
				#endif
			}
			else{
				augmentationRequired = true;

				#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
					cout << adjFace->theNode()->index() << "(" << adjFace << ")(nc), ";
				#endif
			}

			adjFace = adjFace->twin()->cyclicSucc();
		}

		if (augmentationRequired){
			m_graphCopy.createEmpty(*m_pGraph);
			m_graphCopy.initByActiveNodes(activeNodesList, activeNodes, m_eCopy);
			m_graphCopy.setOriginalEmbedding();

			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				cout << " graphCopy:" << endl << flush;
				forall_nodes(n, m_graphCopy){
					cout << n->index() << "(" << (m_graphCopy.original(n))->index() << "):" << flush;
					adjEntry nAdj;
					forall_adj(nAdj, n){
						cout << nAdj << "(" << nAdj->theEdge() << "[" << nAdj->index() << "]) " << flush;
					}
					cout << endl << flush;
				}
			#endif

			adjEntry adjOuterFaceCopy = (m_graphCopy.copy(adjOuterFace->theEdge()))->adjSource();
			if (adjOuterFaceCopy->theNode() != m_graphCopy.copy(adjOuterFace->theNode()))
				adjOuterFaceCopy = adjOuterFaceCopy->twin();

			augment(adjOuterFaceCopy);

		}
		else{
			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				cout << endl << flush;
			#endif
		}


		edge e;
		// set all entries in activeNodes to default value false
		//  for next iteration
		ListIterator<node> anIt = activeNodesList.begin();
		while (anIt.valid()){
			activeNodes[*anIt] = false;
			forall_adj_edges(e, *anIt)
				m_eCopy[e] = 0;
			anIt++;
		}
		activeNodesList.clear();

		#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			cout << "======================" << endl << flush;
		#endif

	}

	delete m_pEmbedding;

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		if (isBiconnected(*m_pGraph))
			cout << " graph is biconnected" << endl << flush;
		else
			cout << " graph is NOT biconnected!!!" << endl << flush;

		if (pisPlanar(*m_pGraph))
			cout << " graph is planar" << endl << flush;
		else
			cout << " graph is NOT planar!!!" << endl << flush;
	#endif
}



//	----------------------------------------------------
//	augment()
//
//		the main augmentation function
//
//  ----------------------------------------------------
void PlanarAugmentationFix::augment(adjEntry adjOuterFace)
{
	CombinatorialEmbedding* actComb = new CombinatorialEmbedding(m_graphCopy);
	m_pActEmbedding = actComb;

	DynamicBCTree* actBCTree = new DynamicBCTree(m_graphCopy);
	m_pBCTree = actBCTree;

	m_pActEmbedding->setExternalFace(m_pActEmbedding->rightFace(adjOuterFace));

	node bFaceNode = m_pBCTree->bcproper(adjOuterFace->theEdge());

	m_isLabel.init(m_pBCTree->bcTree(), 0);
	m_belongsTo.init(m_pBCTree->bcTree(), 0);
	m_belongsToIt.init(m_pBCTree->bcTree(), 0);

	List<node> pendants;

	node v;
	node root;

	// init pendants
	forall_nodes(v, m_pBCTree->bcTree()){
		if (m_pBCTree->parent(v) == 0){
			root = v;
		}
		if (v->degree() == 1){
			// pendant
			if (v != bFaceNode){
				pendants.pushBack(v);
			}
		}
	}

	if (root != bFaceNode){
		// change root of the bc-tree to the b-node that includes the actual face
		modifyBCRoot(root, bFaceNode);
	}

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << " augment(): pendants:" << flush;
		forall_nodes(v, m_pBCTree->bcTree()){
			if (m_pBCTree->parent(v) == 0){
				cout << "[root: " << v->index() << flush;
				if (m_pBCTree->typeOfBNode(v) == BCTree::BComp)
					cout << "(b)], " << flush;
				else
					cout << "(c)], " << flush;
			}
			if (v->degree() == 1){
				// pendant
				if (v != bFaceNode){
					cout << v->index() << ", " << flush;
				}
				else
					cout << v->index() << "(is root), " << flush;
			}
		}
		cout << endl << flush;
	#endif

	m_actBCRoot = bFaceNode;
	m_labels.clear();

	// call reduceChain for all pendants
	ListIterator<node> itPendant = pendants.begin();
	while (itPendant.valid()){
		reduceChain(*itPendant);
		itPendant++;
	}

	// main augmentation loop
	while (m_labels.size() > 0){

		#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
			cout << endl
				 << " *-----------------------------------" << endl
				 << " * augment(): #labels  : " << m_labels.size() << endl << flush;

			ListIterator<pa_label> labelIt = m_labels.begin();
			ListIterator<node> pendantIt;
			int lNr = 1;
			while (labelIt.valid()){
				cout << " * label in list with position " << lNr << " : " << flush;
				pendantIt = ((*labelIt)->m_pendants).begin();
				while (pendantIt.valid()){
					cout << (*pendantIt)->index() << " " << flush;
					pendantIt++;
				}
				cout << endl << flush;
				lNr++;
				labelIt++;
			}
			cout << " *-----------------------------------" << endl << endl << flush;
		#endif


		if (m_labels.size() == 1){
			connectSingleLabel();
		}
		else{
			node pendant1, pendant2;
			adjEntry adjV1, adjV2;
			bool foundMatching = findMatching(pendant1, pendant2, adjV1, adjV2);
			if (!foundMatching)
				findMatchingRev(pendant1, pendant2, adjV1, adjV2);

			connectPendants(pendant1, pendant2, adjV1, adjV2);
		}

		#ifdef PLANAR_AUGMENTATION_FIX_DEBUG_PLANARCHECK
			if (isPlanar(m_graphCopy))
				cout << " graphCopy is planar" << endl << flush;
			else{
				cout << " graphCopy is NOT planar!!!" << endl << flush;;
			}
		#endif
	} // main loop


	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG_PLANARCHECK
		if (isBiconnected(m_graphCopy))
			cout << " graphCopy is biconnected" << endl << flush;
		else{
			cout << " graphCopy is NOT biconnected!!!" << endl << flush;
		}

		if (isPlanar(m_graphCopy))
			cout << " graphCopy is planar" << endl << flush;
		else{
			cout << " graphCopy is NOT planar!!!" << endl << flush;
		}
	#endif

	m_pActEmbedding = 0;
	m_pBCTree = 0;
	delete(actComb);
	delete(actBCTree);
}



//	----------------------------------------------------
//	reduceChain()
//
//  ----------------------------------------------------
void PlanarAugmentationFix::reduceChain(node p)
{
	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "reduceChain() " << p->index();
		if (m_pBCTree->typeOfBNode(p) == BCTree::BComp)
			cout << "[b]"<< flush;
		else
		 	cout << "[c]"<< flush;
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

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << ", stopCause == ";
		switch(stopCause){
			case paPlanarity:
				// cannot occur
				break;
			case paCDegree:
				cout << "paCDegree ";
				break;
			case paBDegree:
				cout << "paBDegree ";
				break;
			case paRoot:
				cout << "paRoot ";
				break;
		}
	#endif

	pa_label l;

	if (stopCause == paCDegree || stopCause == paRoot){

		if (m_isLabel[last].valid()){
			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				cout << "reduceChain(): add " << p->index() << " to another label" << endl << flush;
			#endif
			// l is the label that last is the head of
			l = *(m_isLabel[last]);
			// add the actual pendant p to l
			addPendant(p, l);
			// set the stop-cause
			l->stopCause(stopCause);
		}
		else{
			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				cout << "reduceChain(): creating a new label with pendant " << p->index() << endl << flush;
			#endif
			newLabel(last, 0,  p, stopCause);
		}
	}
	else{
		// stopCause == paBDegree
		parent = m_pBCTree->parent(last);

		if (m_isLabel[parent].valid()){
			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				cout << "reduceChain(): add " << p->index() << " to another label" << endl << flush;
			#endif
			l = *(m_isLabel[parent]);
			addPendant(p, l);
		}
		else{
			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				cout << "reduceChain(): creating a new label with pendant " << p->index() << endl << flush;
			#endif
			newLabel(last, parent, p, paBDegree);
		}
	}
}



//	----------------------------------------------------
//	followPath(node v, node& last)
//
//  ----------------------------------------------------
paStopCause PlanarAugmentationFix::followPath(node v, node& last)
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
			else{
				if (m_pBCTree->DynamicBCTree::parent(bcNode) == 0)
					return paRoot;
				else
					return paBDegree;
			}
		}

		if (m_pBCTree->typeOfBNode(bcNode) == BCTree::CComp){
			last = bcNode;
		}

		// iterate to parent node
		bcNode = m_pBCTree->DynamicBCTree::parent(bcNode);
	}
	return paRoot;
}



//	----------------------------------------------------
// 	findMatching(node& pendant1, node& pendant2,...)
//
//  ----------------------------------------------------
bool PlanarAugmentationFix::findMatching(node& pendant1, node& pendant2, adjEntry& adjV1, adjEntry& adjV2)
{
	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "findMatching()" << endl << flush;
	#endif

	pa_label l = m_labels.front();
	pendant2 = 0;
	adjV1 = adjV2 = 0;
	pendant1 = m_pBCTree->find(l->getFirstPendant());
	node pendantFirst = pendant1;

	node cutV = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[pendant1]];
	adjEntry adj = cutV->firstAdj();
	if (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == pendant1){
		while (m_pBCTree->DynamicBCTree::bcproper(adj->twinNode()) == pendant1){
			adjV1 = adj->twin();
			adj = adj->cyclicSucc();
		}
	}
	else{
		while (m_pBCTree->DynamicBCTree::bcproper(adj->twinNode()) != pendant1)
			adj = adj->cyclicPred();
		adjV1 = adj->twin();
		adj = adj->cyclicSucc();
	}

	// adjV1 is the very adjEntry that belongs to pendant1, points to the cutvertex
	//   and is the rightmost adjEntry with this properties
	adjV1 = adjV1->cyclicPred();

	bool loop = true;
	bool found = false;

	node cutvBFNode = 0;
	bool dominatingTree = false;

	while (loop){

		if (m_pBCTree->typeOfGNode(adj->theNode()) == BCTree::CutVertex){
			if (!dominatingTree){
				if ((adj->theNode() == cutvBFNode)){
					dominatingTree = true;
				}
				else{
					if ((cutvBFNode == 0) && (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == m_actBCRoot))
						cutvBFNode = adj->theNode();
				}
			}
		}
		else{
			node actPendant = m_pBCTree->DynamicBCTree::bcproper(adj->theNode());

			if ((m_pBCTree->m_bNode_degree[actPendant] == 1) && (actPendant != m_actBCRoot) && (actPendant != pendant1)){

				if (m_belongsTo[actPendant] == l){
					// found pendant of same label
					adjV1 = adj->cyclicPred();
				 	pendant1 = actPendant;
				 	l->m_pendants.del(m_belongsToIt[pendant1]);
				 	m_belongsToIt[pendant1] = l->m_pendants.pushFront(pendant1);
				 	if (dominatingTree){
				 		cutvBFNode = 0;
					 }
				 }
				 else{
				 	// found pendant of different label
				 	if (dominatingTree){
				 		// we have a dominating tree
				 		if (cutvBFNode != 0){
				 			// corrupt situation

				 			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
				 				cout << "findMatching(): found dominating tree: corrupt situation " << endl << flush;
				 			#endif

				 			pendant1 = pendantFirst;
				 			loop = false;
				 			found = false;
				 		}
				 		else{
				 			// correct situation

				 			#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
					 			cout << "findMatching(): found dominating tree: correct situation " << endl << flush;
				 			#endif

					 		adjV2 = adj->cyclicPred();
						 	pendant2 = actPendant;
						 	loop = false;
						 	found = true;
				 		}
				 	}
				 	else{
				 		// no dominating tree
						adjV2 = adj->cyclicPred();
						pendant2 = actPendant;
						loop = false;
						found = true;
					}
				}
			}
		}

		adj = adj->twin()->cyclicSucc();
	}

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "findMatching() finished with found == "<< found << endl << flush;
	#endif

	return (found);
}



//	----------------------------------------------------
// 	findMatchingRev(node& pendant1, node& pendant2,...)
//
//  ----------------------------------------------------
void PlanarAugmentationFix::findMatchingRev(node& pendant1, node& pendant2, adjEntry& adjV1, adjEntry& adjV2)
{
	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "findMatchingRev() " << endl << flush;
	#endif

	pa_label l = m_belongsTo[pendant1];
	pendant2 = 0;
	adjV1 = adjV2 = 0;


	node cutV = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[pendant1]];
	adjEntry adj = cutV->firstAdj();
	if (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == pendant1){
		while (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) == pendant1){
			adjV1 = adj->twin();
			adj = adj->cyclicPred();
		}
	}
	else{
		while (m_pBCTree->DynamicBCTree::bcproper(adj->theEdge()) != pendant1)
			adj = adj->cyclicSucc();
		adjV1 = adj->twin();
		adj = adj->cyclicPred();
	}

	bool loop = true;

	while (loop){

		if (m_pBCTree->typeOfGNode(adj->theNode()) == BCTree::Normal){

			node actPendant = m_pBCTree->DynamicBCTree::bcproper(adj->theNode());

			if (m_pBCTree->m_bNode_degree[actPendant] == 1){

				if (m_belongsTo[actPendant] == l){
					// found pendant of same label
					adjV1 = adj;
					pendant1 = actPendant;
					l->m_pendants.del(m_belongsToIt[pendant1]);
					m_belongsToIt[pendant1] = l->m_pendants.pushBack(pendant1);
				}
				else{
				 	// found pendant of different label
				 	adjV2 = adj;
					pendant2 = actPendant;
					loop = false;
				}

			}
		}

		adj = adj->twin()->cyclicPred();
	}
}



//	----------------------------------------------------
//	connectPendants(...)
//
//  ----------------------------------------------------
void PlanarAugmentationFix::connectPendants(node pendant1, node pendant2, adjEntry adjV1, adjEntry adjV2)
{
	edge newEdgeCopy = m_pActEmbedding->splitFace(adjV1, adjV2);

	adjEntry adjOrigV1 = (m_graphCopy.original(adjV1->theEdge()))->adjSource();
	if (adjOrigV1->theNode() != m_graphCopy.original(adjV1->theNode()))
		adjOrigV1 = adjOrigV1->twin();

	adjEntry adjOrigV2 = (m_graphCopy.original(adjV2->theEdge()))->adjSource();
	if (adjOrigV2->theNode() != m_graphCopy.original(adjV2->theNode()))
		adjOrigV2 = adjOrigV2->twin();

	edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
	m_pResult->pushBack(newEdgeOrig);

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "connectPendants(" << pendant1->index() << ", " << pendant2->index() << ", " << adjV1 << ", " << adjV2 << ")" << endl << flush;
		cout << "                leads to edge " << newEdgeCopy << endl << flush;
		cout << "                and new edge in the orig. " << newEdgeOrig << endl << flush;
	#endif

	m_pBCTree->updateInsertedEdge(newEdgeCopy);
	m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);

	pa_label l1 = m_belongsTo[pendant1];
	pa_label l2 = m_belongsTo[pendant2];

	deletePendant(pendant1);
	deletePendant(pendant2);

	if (l2->size() > 0){
		if (l2->size() == 1){
			node pendant = l2->getFirstPendant();
			deleteLabel(l2);
			reduceChain(pendant);
		}
		else{
			removeLabel(l2);
			insertLabel(l2);
		}
	}
	else
		deleteLabel(l2);

	if (l1->size() > 0){
		if (l1->size() == 1){
			node pendant = l1->getFirstPendant();
			deleteLabel(l1);
			reduceChain(pendant);
		}
		else{
			removeLabel(l1);
			insertLabel(l1);
		}
	}
	else
		deleteLabel(l1);

	m_actBCRoot = m_pBCTree->find(m_actBCRoot);

	node bcNode = m_pBCTree->DynamicBCTree::bcproper(newEdgeCopy);
	if ((bcNode != pendant1) && (bcNode != pendant2) && (m_pBCTree->m_bNode_degree[bcNode] == 1)
	&& (bcNode != m_actBCRoot))
		reduceChain(bcNode);
}



//	----------------------------------------------------
//	connectSingleLabel(...)
//
//  ----------------------------------------------------
void PlanarAugmentationFix::connectSingleLabel()
{
	pa_label l = m_labels.front();
	node pendant1 = l->getFirstPendant();

	adjEntry adj;
	node cutV = m_pBCTree->m_hNode_gNode[m_pBCTree->m_bNode_hParNode[pendant1]];
	adjEntry adjRun = cutV->firstAdj();
	m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge());

	if (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == pendant1){
		while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == pendant1){
			adj = adjRun->twin();
			adjRun = adjRun->cyclicSucc();
		}
	}
	else{
		while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) != pendant1)
			adjRun = adjRun->cyclicPred();
		adj = adjRun->twin();
		adjRun = adjRun->cyclicSucc();
	}

	adjEntry adjFirst = adj;
	adj = adj->cyclicPred();

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
	bool singleEdgePendant = false;
	if (adj == adj->cyclicSucc())
		singleEdgePendant = true;

		cout << "connectSinlgeLabel(): cutv=" << cutV << ", adj=" << adj << ", adjRun=" << adjRun << "(pred=" << adjRun->cyclicPred() <<")"
			 << ", adjFirst=" << adjFirst << ", singleEdgePendant=" << singleEdgePendant << endl << flush;
	#endif

	bool connectLeft  = false;
	bool connectRight = false;
	node adjBNode = 0;
	node lastConnectedPendant = 0;

	if (l->size() > 1){

		node cutvBFNode = 0;
		bool loop = true;

		adjBNode = m_pBCTree->bcproper(adj->theEdge());

		// first connect pendants "on the right" of the first pendant
		while (loop){

			if (m_pBCTree->typeOfGNode(adjRun->theNode()) == BCTree::CutVertex){
				if ((adjRun->theNode() == cutvBFNode)){
					loop = false;
				}
				else{
					if ((cutvBFNode == 0) && (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == m_actBCRoot))
						cutvBFNode = adjRun->theNode();
				}
			}
			else{
				node actPendant = m_pBCTree->DynamicBCTree::bcproper(adjRun->theNode());

				if ((m_pBCTree->m_bNode_degree[actPendant] == 1)
				&& (actPendant != m_pBCTree->find(adjBNode))
				&& (actPendant != lastConnectedPendant)
				&& (actPendant != m_actBCRoot)){

					if (!connectRight)
						connectRight = true;

					lastConnectedPendant = actPendant;
					adjRun = adjRun->cyclicPred();

					edge newEdgeCopy = m_pActEmbedding->splitFace(adj, adjRun);

					adjEntry adjOrigV1 = (m_graphCopy.original(adj->theEdge()))->adjSource();
					if (adjOrigV1->theNode() != m_graphCopy.original(adj->theNode()))
						adjOrigV1 = adjOrigV1->twin();
					adjEntry adjOrigV2 = (m_graphCopy.original(adjRun->theEdge()))->adjSource();
					if (adjOrigV2->theNode() != m_graphCopy.original(adjRun->theNode()))
						adjOrigV2 = adjOrigV2->twin();

					edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
					m_pResult->pushBack(newEdgeOrig);

					#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
						cout << "connectSingleLabel(): splitFace (on the right) with " << adj << " and " << adjRun << endl << flush;
						cout << "                      leads to edge " << newEdgeCopy << endl << flush;
						cout << "                      newEdge in the orig. graph " << newEdgeOrig << endl << flush;
					#endif

					m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);
					adjRun = adjRun->cyclicSucc()->cyclicSucc();
				}
			}
			adjRun = adjRun->twin()->cyclicSucc();
		} // while (loop)

		// now connect pendants "on the left" of the first pendant
		adjRun = adjFirst->twin();
		while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == pendant1)
			adjRun = adjRun->cyclicPred();
		adj = adjRun->cyclicSucc()->twin();

		cutvBFNode = 0;
		loop = true;

		while (loop){

			if (m_pBCTree->typeOfGNode(adjRun->theNode()) == BCTree::CutVertex){
				if ((adjRun->theNode() == cutvBFNode)){
					loop = false;
				}
				else{
					if ((cutvBFNode == 0) && (m_pBCTree->DynamicBCTree::bcproper(adjRun->theEdge()) == m_actBCRoot))
						cutvBFNode = adjRun->theNode();
				}
			}
			else{
				node actPendant = m_pBCTree->DynamicBCTree::bcproper(adjRun->theNode());
				if  ((m_pBCTree->m_bNode_degree[actPendant] == 1)
				&& (actPendant != m_pBCTree->find(adjBNode))
				&& (actPendant != lastConnectedPendant)
				&& (actPendant != m_actBCRoot)){

					if (!connectLeft)
						connectLeft = true;

					lastConnectedPendant = actPendant;
					edge newEdgeCopy = m_pActEmbedding->splitFace(adj, adjRun);

					adjEntry adjOrigV1 = (m_graphCopy.original(adj->theEdge()))->adjSource();
					if (adjOrigV1->theNode() != m_graphCopy.original(adj->theNode()))
						adjOrigV1 = adjOrigV1->twin();
					adjEntry adjOrigV2 = (m_graphCopy.original(adjRun->theEdge()))->adjSource();
					if (adjOrigV2->theNode() != m_graphCopy.original(adjRun->theNode()))
						adjOrigV2 = adjOrigV2->twin();

					edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
					m_pResult->pushBack(newEdgeOrig);

					#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
						cout << "connectSingleLabel(): splitFace (on the left) with " << adj << " and " << adjRun << endl << flush;
						cout << "                      leads to edge " << newEdgeCopy << endl << flush;
						cout << "                      newEdge in the orig. graph " << newEdgeOrig << endl << flush;
					#endif

					m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);
					adj = adj->cyclicSucc();
				}
			}

			adjRun = adjRun->twin()->cyclicPred();
		}
	}

	adjRun = adj->cyclicSucc();

	while (m_pBCTree->DynamicBCTree::bcproper(adjRun->theNode()) != m_pBCTree->find(m_actBCRoot)){
		adjRun = adjRun->twin()->cyclicSucc();
	}
	adjRun = adjRun->cyclicPred();

	edge newEdgeCopy = m_pActEmbedding->splitFace(adj, adjRun);

	adjEntry adjOrigV1 = (m_graphCopy.original(adj->theEdge()))->adjSource();
	if (adjOrigV1->theNode() != m_graphCopy.original(adj->theNode()))
		adjOrigV1 = adjOrigV1->twin();
	adjEntry adjOrigV2 = (m_graphCopy.original(adjRun->theEdge()))->adjSource();
	if (adjOrigV2->theNode() != m_graphCopy.original(adjRun->theNode()))
		adjOrigV2 = adjOrigV2->twin();

	edge newEdgeOrig = m_pEmbedding->splitFace(adjOrigV1, adjOrigV2);
	m_pResult->pushBack(newEdgeOrig);

	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "connectSingleLabel(): splitFace (last edge) with " << adj << " and " << adjRun << endl << flush;
		cout << "                      leads to edge " << newEdgeCopy << endl << flush;
		cout << "                      newEdge in the orig. graph " << newEdgeOrig << endl << flush;
	#endif

	m_graphCopy.setEdge(newEdgeOrig, newEdgeCopy);

	deleteLabel(l);
}



//	----------------------------------------------------
//	newLabel(...)
//
//  ----------------------------------------------------
pa_label PlanarAugmentationFix::newLabel(node cutvertex, node parent, node pendant, paStopCause whyStop)
{
	pa_label l;
	l = OGDF_NEW PALabel(parent, cutvertex, whyStop);

	m_belongsTo[pendant] = l;
	m_belongsToIt[pendant] = (l->m_pendants).pushBack(pendant);

	if (parent != 0)
		m_isLabel[parent] = m_labels.pushBack(l);
	else
		m_isLabel[cutvertex] = m_labels.pushBack(l);

	return l;
}



//	----------------------------------------------------
//	deleteLabel(...)
//
//  ----------------------------------------------------
void PlanarAugmentationFix::deleteLabel(pa_label& l, bool removePendants){
	ListIterator<pa_label> labelIt = m_isLabel[l->parent()];

	m_labels.del(labelIt);
	m_isLabel[l->parent()] = 0;

	ListIterator<node> pendantIt = (l->m_pendants).begin();
	while (pendantIt.valid()){
		m_belongsTo[*pendantIt] = 0;
		m_belongsToIt[*pendantIt] = 0;
		pendantIt++;
	}

	delete(l);
	l = 0;
}



//	----------------------------------------------------
//	removeLabel(...)
//
//  ----------------------------------------------------
void PlanarAugmentationFix::removeLabel(pa_label& l){
	ListIterator<pa_label> labelIt = m_isLabel[l->parent()];

	m_labels.del(labelIt);
}



//	----------------------------------------------------
//	addPendant(pa_label& l, node pendant)
//
//  ----------------------------------------------------
void PlanarAugmentationFix::addPendant(node p, pa_label& l)
{
	m_belongsTo[p] = l;
	m_belongsToIt[p] = (l->m_pendants).pushBack(p);

	node newParent = m_pBCTree->find(l->parent());

	m_labels.del(m_isLabel[l->parent()]);
	m_isLabel[newParent] = insertLabel(l);
}



//	----------------------------------------------------
//	deletePendant(node pendant)
//
//  ----------------------------------------------------
void PlanarAugmentationFix::deletePendant(node pendant){
	(m_belongsTo[pendant])->removePendant(m_belongsToIt[pendant]);
	m_belongsTo[pendant] = 0;
	m_belongsToIt[pendant] = 0;
}



//	----------------------------------------------------
//	insertLabel(pa_label l)
//
//  ----------------------------------------------------
ListIterator<pa_label> PlanarAugmentationFix::insertLabel(pa_label l)
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
//	modifyBCRoot()
//
//  ----------------------------------------------------
void PlanarAugmentationFix::modifyBCRoot(node oldRoot, node newRoot)
{
	#ifdef PLANAR_AUGMENTATION_FIX_DEBUG
		cout << "modifyBCRoot()" << endl << flush;
	#endif

	SList<node> *path = m_pBCTree->findPathBCTree(oldRoot, newRoot);
	SListIterator<node> it = path->begin();
	SListIterator<node> it2 = path->begin();

	while (it.valid()){
		if (it2 != it){
			changeBCRoot((*it2), (*it));
		}

		it2 = it;
		it++;
	}

	delete path;
}



//	----------------------------------------------------
//	changeBCRoot()
//
//  ----------------------------------------------------
void PlanarAugmentationFix::changeBCRoot(node oldRoot, node newRoot)
{
	//   for the old root:
	m_pBCTree->m_bNode_hRefNode[oldRoot] = m_pBCTree->m_bNode_hParNode[newRoot];
	m_pBCTree->m_bNode_hParNode[oldRoot] = m_pBCTree->m_bNode_hRefNode[newRoot];

	//   for the new root:
	// m_pBCTree->m_bNode_hRefNode[newRoot] = no update required;
	m_pBCTree->m_bNode_hParNode[newRoot] = 0;
}


} // end namespace ogdf
