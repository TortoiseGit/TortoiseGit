/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the class EmbedPQTree.
 *
 * Implements a PQTree with added features for the planar
 * embedding algorithm. Used by BoothLueker.
 *
 * \author Sebastian Leipert
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


#include <ogdf/internal/planarity/EmbedPQTree.h>

namespace ogdf{

// Overriding the function doDestruction (see basic.h)
// Allows deallocation of lists of PQLeafKey<edge,IndInfo*,bool>
// in constant time using OGDF memory management.


typedef PQLeafKey<edge,IndInfo*,bool> *PtrPQLeafKeyEIB;

template<>
inline bool doDestruction<PtrPQLeafKeyEIB>(const PtrPQLeafKeyEIB*) { return false;  }



// Replaces the pertinent subtree by a P-node with leaves as children
// corresponding to the incoming edges of the node v. These edges
// are to be specified by their keys stored in leafKeys.
// The function returns the frontier of the pertinent subtree and
// the direction indicators found within the pertinent leaves.
// The direction indicators are returned in two list:
// opposed: containing the keys of indicators pointing into reverse
// frontier scanning direction (thus their corsponding list has to be
// reversed.
// nonOpposed: containing the keys of indicators pointing into
// frontier scanning direction (thus their corsponding list do not need
// reversed in the first place)

void EmbedPQTree::ReplaceRoot(
	SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys,
	SListPure<edge> &frontier,
	SListPure<node> &opposed,
	SListPure<node> &nonOpposed,
	node v)
{
	SListPure<PQBasicKey<edge,IndInfo*,bool>*> nodeFrontier;

	if (leafKeys.empty() && m_pertinentRoot == m_root)
	{
		front(m_pertinentRoot,nodeFrontier);
		m_pertinentRoot = 0;  // check for this emptyAllPertinentNodes

	} else {
		if (m_pertinentRoot->status() == PQNodeRoot::FULL)
			ReplaceFullRoot(leafKeys,nodeFrontier,v);
		else
			ReplacePartialRoot(leafKeys,nodeFrontier,v);
	}

	// Check the frontier and get the direction indicators.
	while (!nodeFrontier.empty())
	{
		PQBasicKey<edge,IndInfo*,bool>* entry = nodeFrontier.popFrontRet();
		if (entry->userStructKey()) // is a regular leaf
			frontier.pushBack(entry->userStructKey());

		else if (entry->userStructInfo()) {
			if (entry->userStructInfo()->changeDir)
				opposed.pushBack(entry->userStructInfo()->v);
			else
				nonOpposed.pushBack(entry->userStructInfo()->v);
		}
	}
}


// The function [[emptyAllPertinentNodes]] has to be called after a reduction
// has been processed. This overloaded function first destroys all full nodes
// by marking them as TO_BE_DELETED and then calling the base class function
// [[emptyAllPertinentNodes]].
void EmbedPQTree::emptyAllPertinentNodes()
{
	ListIterator<PQNode<edge,IndInfo*,bool>*> it;

	for (it = m_pertinentNodes->begin(); it.valid(); it++)
	{
		PQNode<edge,IndInfo*,bool>* nodePtr = (*it);
		if (nodePtr->status() == PQNodeRoot::FULL)
			destroyNode(nodePtr);
	}
	if (m_pertinentRoot) // Node was kept in the tree. Do not free it.
		m_pertinentRoot->status(PQNodeRoot::FULL);

	PQTree<edge,IndInfo*,bool>::emptyAllPertinentNodes();
}



void EmbedPQTree::clientDefinedEmptyNode(PQNode<edge,IndInfo*,bool>* nodePtr)
{
	if (nodePtr->status() == PQNodeRoot::INDICATOR)
		delete nodePtr;
	else
		PQTree<edge,IndInfo*,bool>::clientDefinedEmptyNode(nodePtr);
}



// Initializes a PQTree by a set of leaves that will korrespond to
// the set of Keys stored in leafKeys.
int EmbedPQTree::Initialize(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys)
{
	SListIterator<PlanarLeafKey<IndInfo*>* > it;

	SListPure<PQLeafKey<edge,IndInfo*,bool>*> castLeafKeys;
	for (it = leafKeys.begin(); it.valid(); ++it)
		castLeafKeys.pushBack((PQLeafKey<edge,IndInfo*,bool>*) *it);

	return PQTree<edge,IndInfo*,bool>::Initialize(castLeafKeys);
}


// Reduction reduced a set of leaves determined by their keys stored
// in leafKeys. Integer redNumber is for debugging only.
bool EmbedPQTree::Reduction(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys)
{
	SListIterator<PlanarLeafKey<IndInfo*>* >  it;

	SListPure<PQLeafKey<edge,IndInfo*,bool>*> castLeafKeys;
	for (it = leafKeys.begin(); it.valid(); ++it)
		castLeafKeys.pushBack((PQLeafKey<edge,IndInfo*,bool>*) *it);

	return PQTree<edge,IndInfo*,bool>::Reduction(castLeafKeys);
}



// Function ReplaceFullRoot either replaces the full root
// or one full child of a partial root of a pertinent subtree
// by a single P-node  with leaves corresponding the keys stored in leafKeys.
// Furthermore it scans the frontier of the pertinent subtree, and returns it
// in frontier.
// If called by ReplacePartialRoot, the function ReplaceFullRoot handles
// the introduction of the direction  indicator. (This must be indicated
// by addIndicator.
// Node v determines the node related to the pertinent leaves. It is needed
// to assign the dirrection indicator to this sequence.

void EmbedPQTree::ReplaceFullRoot(
	SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys,
	SListPure<PQBasicKey<edge,IndInfo*,bool>*> &frontier,
	node v,
	bool addIndicator,
	PQNode<edge,IndInfo*,bool> *opposite)
{
	EmbedIndicator *newInd = 0;

	front(m_pertinentRoot,frontier);
	if (addIndicator)
	{
		IndInfo *newInfo = OGDF_NEW IndInfo(v);
		PQNodeKey<edge,IndInfo*,bool> *nodeInfoPtr = OGDF_NEW PQNodeKey<edge,IndInfo*,bool>(newInfo);
		newInd = OGDF_NEW EmbedIndicator(m_identificationNumber++, nodeInfoPtr);
		newInd->setNodeInfo(nodeInfoPtr);
		nodeInfoPtr->setNodePointer(newInd);
	}

	if (!leafKeys.empty() && leafKeys.front() == leafKeys.back())
	{
		//ReplaceFullRoot: replace pertinent root by a single leaf
		if (addIndicator)
		{
			opposite = m_pertinentRoot->getNextSib(opposite);
			if (!opposite) // m_pertinentRoot is endmost child
			{
				addNodeToNewParent(m_pertinentRoot->parent(), newInd, m_pertinentRoot, opposite);
			}
			else
				addNodeToNewParent(0,newInd,m_pertinentRoot,opposite);

			// Setting the sibling pointers into opposite direction of
			// scanning the front allows to track swaps of the indicator
			newInd->changeSiblings(m_pertinentRoot,0);
			newInd->changeSiblings(opposite,0);
			newInd->putSibling(m_pertinentRoot,PQNodeRoot::LEFT);
			newInd->putSibling(opposite,PQNodeRoot::RIGHT);
		}
		PQLeaf<edge,IndInfo*,bool> *leafPtr =
			OGDF_NEW PQLeaf<edge,IndInfo*,bool>(m_identificationNumber++,
			PQNodeRoot::EMPTY,(PQLeafKey<edge,IndInfo*,bool>*)leafKeys.front());
		exchangeNodes(m_pertinentRoot,(PQNode<edge,IndInfo*,bool>*) leafPtr);
 		if (m_pertinentRoot == m_root)
			m_root = (PQNode<edge,IndInfo*,bool>*) leafPtr;
		m_pertinentRoot = 0;  // check for this emptyAllPertinentNodes
	}

	else if (!leafKeys.empty()) // at least two leaves
	{
		//replace pertinent root by a $P$-node
		if (addIndicator)
		{
			opposite = m_pertinentRoot->getNextSib(opposite);
			if (!opposite) // m_pertinentRoot is endmost child
			{
				addNodeToNewParent(m_pertinentRoot->parent(), newInd, m_pertinentRoot, opposite);
			}
			else
				addNodeToNewParent(0, newInd, m_pertinentRoot, opposite);

			// Setting the sibling pointers into opposite direction of
			// scanning the front allows to track swaps of the indicator
			newInd->changeSiblings(m_pertinentRoot,0);
			newInd->changeSiblings(opposite,0);
			newInd->putSibling(m_pertinentRoot,PQNodeRoot::LEFT);
			newInd->putSibling(opposite,PQNodeRoot::RIGHT);
		}

		PQInternalNode<edge,IndInfo*,bool> *nodePtr = 0; // dummy
		if ((m_pertinentRoot->type() == PQNodeRoot::PNode) ||
			(m_pertinentRoot->type() == PQNodeRoot::QNode))
		{
			nodePtr = (PQInternalNode<edge,IndInfo*,bool>*)m_pertinentRoot;
			nodePtr->type(PQNodeRoot::PNode);
			nodePtr->childCount(0);
			while (!fullChildren(m_pertinentRoot)->empty())
			{
				PQNode<edge,IndInfo*,bool> *currentNode =
					fullChildren(m_pertinentRoot)->popFrontRet();
				removeChildFromSiblings(currentNode);
			}
		}
		else if (m_pertinentRoot->type() == PQNodeRoot::leaf)
		{
			nodePtr = OGDF_NEW PQInternalNode<edge,IndInfo*,bool>(m_identificationNumber++,
														 PQNodeRoot::PNode,PQNodeRoot::EMPTY);
			exchangeNodes(m_pertinentRoot,nodePtr);
			m_pertinentRoot = 0;  // check for this emptyAllPertinentNodes
		}

		SListPure<PQLeafKey<edge,IndInfo*,bool>*> castLeafKeys;
		SListIterator<PlanarLeafKey<IndInfo*>* > it;
		for (it = leafKeys.begin(); it.valid(); ++it)
			castLeafKeys.pushBack((PQLeafKey<edge,IndInfo*,bool>*) *it);
		addNewLeavesToTree(nodePtr,castLeafKeys);
	}
}


// Function ReplacePartialRoot replaces all full nodes by a single P-node
// with leaves corresponding the keys stored in leafKeys.
// Furthermore it scans the frontier of the pertinent subtree, and returns it
// in frontier.
// node v determines the node related to the pertinent leaves. It is needed
// to assign the dirrection indicator to this sequence.

void EmbedPQTree::ReplacePartialRoot(
	SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys,
	SListPure<PQBasicKey<edge,IndInfo*,bool>*> &frontier,
	node v)
{
	m_pertinentRoot->childCount(m_pertinentRoot->childCount() + 1 -
								fullChildren(m_pertinentRoot)->size());

	PQNode<edge,IndInfo*,bool> *predNode = 0; // dummy
	PQNode<edge,IndInfo*,bool> *beginSequence = 0; // marks begin consecuitve seqeunce
	PQNode<edge,IndInfo*,bool> *endSequence	= 0; // marks end consecutive sequence
	PQNode<edge,IndInfo*,bool> *beginInd = 0;	// initially, marks direct sibling indicator
												// next to beginSequence not contained
												// in consectuive sequence

	// Get beginning and end of sequence.
	while (fullChildren(m_pertinentRoot)->size())
	{
		PQNode<edge,IndInfo*,bool> *currentNode = fullChildren(m_pertinentRoot)->popFrontRet();
		if (!clientSibLeft(currentNode) ||
			clientSibLeft(currentNode)->status() == PQNodeRoot::EMPTY)
		{
			if (!beginSequence)
			{
				beginSequence = currentNode;
				predNode = clientSibLeft(currentNode);
				beginInd =PQTree<edge,IndInfo*,bool>::clientSibLeft(currentNode);
			}
			else
				endSequence = currentNode;
		}
		else if (!clientSibRight(currentNode) ||
				 clientSibRight(currentNode)->status() == PQNodeRoot::EMPTY )
		{
			if (!beginSequence)
			{
				beginSequence = currentNode;
				predNode = clientSibRight(currentNode);
				beginInd =PQTree<edge,IndInfo*,bool>::clientSibRight(currentNode);
			}
			else
				endSequence = currentNode;
		}
	}

	SListPure<PQBasicKey<edge,IndInfo*,bool>*> partialFrontier;


	// Now scan the sequence of full nodes. Remove all of them but the last.
	// Call ReplaceFullRoot on the last one.
	// For every full node get its frontier. Scan intermediate indicators.

	PQNode<edge,IndInfo*,bool> *currentNode = beginSequence;
	while (currentNode != endSequence)
	{
		PQNode<edge,IndInfo*,bool>* nextNode =
			clientNextSib(currentNode,predNode);
		front(currentNode,partialFrontier);
		frontier.conc(partialFrontier);

		PQNode<edge,IndInfo*,bool>* currentInd	= PQTree<edge,IndInfo*,bool>::
			clientNextSib(currentNode,beginInd);

		// Scan for intermediate direction indicators.
		while (currentInd != nextNode)
		{
			PQNode<edge,IndInfo*,bool> *nextInd = PQTree<edge,IndInfo*,bool>::
				clientNextSib(currentInd,currentNode);
			if (currentNode == currentInd->getSib(PQNodeRoot::RIGHT)) //Direction changed
				currentInd->getNodeInfo()->userStructInfo()->changeDir = true;
			frontier.pushBack((PQBasicKey<edge,IndInfo*,bool>*)
								currentInd->getNodeInfo());
			removeChildFromSiblings(currentInd);
			m_pertinentNodes->pushBack(currentInd);
			currentInd = nextInd;
		}

		removeChildFromSiblings(currentNode);
		currentNode = nextNode;
	}

	currentNode->parent(m_pertinentRoot);
	m_pertinentRoot = currentNode;
	ReplaceFullRoot(leafKeys,partialFrontier,v,true,beginInd);
	frontier.conc(partialFrontier);
}



// Overloads virtual function of base class PQTree
// Allows ignoring the virtual direction indicators during
// the template matching algorithm.
PQNode<edge,IndInfo*,bool>* EmbedPQTree::clientSibLeft(
	PQNode<edge,IndInfo*,bool> *nodePtr) const
{
	PQNode<edge,IndInfo*,bool> *predNode = nodePtr;
	nodePtr = PQTree<edge,IndInfo*,bool>::clientSibLeft(predNode);
	while (nodePtr && nodePtr->status() == PQNodeRoot::INDICATOR)
	{
		PQNode<edge,IndInfo*,bool> *holdSib = predNode;
		predNode = nodePtr;
		nodePtr  = predNode->getNextSib(holdSib);
	}

	return nodePtr;
}


// Overloads virtual function of base class PQTree
// Allows ignoring the virtual direction indicators during
// the template matching algorithm.
PQNode<edge,IndInfo*,bool>* EmbedPQTree::clientSibRight(
	PQNode<edge,IndInfo*,bool> *nodePtr) const
{
	PQNode<edge,IndInfo*,bool> *predNode = nodePtr;
	nodePtr = PQTree<edge,IndInfo*,bool>::clientSibRight(predNode);
	while (nodePtr && nodePtr->status() == PQNodeRoot::INDICATOR)
	{
		PQNode<edge,IndInfo*,bool> *holdSib = predNode;
		predNode = nodePtr;
		nodePtr = predNode->getNextSib(holdSib);
	}

	return nodePtr;
}


// Overloads virtual function of base class PQTree
// Allows ignoring the virtual direction indicators during
// the template matching algorithm.
PQNode<edge,IndInfo*,bool>* EmbedPQTree::clientLeftEndmost(
	PQNode<edge,IndInfo*,bool> *nodePtr) const
{
	PQNode<edge,IndInfo*,bool> *left = PQTree<edge,IndInfo*,bool>::clientLeftEndmost(nodePtr);

	if (!left || left->status() != PQNodeRoot::INDICATOR)
		return left;
	else
		return clientNextSib(left,NULL);
}


// Overloads virtual function of base class PQTree
// Allows ignoring the virtual direction indicators during
// the template matching algorithm.
PQNode<edge,IndInfo*,bool>* EmbedPQTree::clientRightEndmost(
	PQNode<edge,IndInfo*,bool> *nodePtr) const
{
	PQNode<edge,IndInfo*,bool> *right = PQTree<edge,IndInfo*,bool>::clientRightEndmost(nodePtr);

	if (!right || right->status() != PQNodeRoot::INDICATOR)
		return right;
	else
		return clientNextSib(right,NULL);
}


// Overloads virtual function of base class PQTree
// Allows ignoring the virtual direction indicators during
// the template matching algorithm.
PQNode<edge,IndInfo*,bool>* EmbedPQTree::clientNextSib(
	PQNode<edge,IndInfo*,bool> *nodePtr,
	PQNode<edge,IndInfo*,bool> *other) const
{
	PQNode<edge,IndInfo*,bool> *left = clientSibLeft(nodePtr);
	if (left  != other) return left;

	PQNode<edge,IndInfo*,bool> *right = clientSibRight(nodePtr);
	if (right != other) return right;

	return 0;
}


// Overloads virtual function of base class PQTree
// Allows to print debug information on the direction indicators
const char* EmbedPQTree::clientPrintStatus(PQNode<edge,IndInfo*,bool> *nodePtr)
{
	if (nodePtr->status() == PQNodeRoot::INDICATOR)
		return "INDICATOR";
	else
		return PQTree<edge,IndInfo*,bool>::clientPrintStatus(nodePtr);
}


// The function front scans the frontier of nodePtr. It returns the keys
// of the leaves found in the frontier of nodePtr in a SListPure.
// These keys include keys of direction indicators detected in the frontier.
//
// CAREFUL: Funktion marks all full nodes for destruction.
//			Only to be used in connection with replaceRoot.
//
void EmbedPQTree::front(
	PQNode<edge,IndInfo*,bool>* nodePtr,
	SListPure<PQBasicKey<edge,IndInfo*,bool>*> &keys)
{
	Stack<PQNode<edge,IndInfo*,bool>*> S;
	S.push(nodePtr);

	while (!S.empty())
	{
		PQNode<edge,IndInfo*,bool> *checkNode = S.pop();

		if (checkNode->type() == PQNodeRoot::leaf)
			keys.pushBack((PQBasicKey<edge,IndInfo*,bool>*) checkNode->getKey());
		else
		{
			PQNode<edge,IndInfo*,bool>* firstSon = 0;
			if (checkNode->type() == PQNodeRoot::PNode)
			{
				firstSon = checkNode->referenceChild();
			}
 			else if (checkNode->type() == PQNodeRoot::QNode)
			{
				firstSon = checkNode->getEndmost(PQNodeRoot::RIGHT);
				// By this, we make sure that we start on the left side
				// since the left endmost child will be on top of the stack
			}

			if (firstSon->status() == PQNodeRoot::INDICATOR)
			{
				keys.pushBack((PQBasicKey<edge,IndInfo*,bool>*) firstSon->getNodeInfo());
				m_pertinentNodes->pushBack(firstSon);
				destroyNode(firstSon);
			}
			else
				S.push(firstSon);

			PQNode<edge,IndInfo*,bool> *nextSon = firstSon->getNextSib(0);
			PQNode<edge,IndInfo*,bool> *oldSib = firstSon;
			while (nextSon && nextSon != firstSon)
			{
				if (nextSon->status() == PQNodeRoot::INDICATOR)
				{
					// Direction indicators point with their left sibling pointer
					// in the direction of their sequence. If an indicator is scanned
					// from the opposite direction, coming from its right sibling
					// the corresponding sequence must be reversed.
					if (oldSib == nextSon->getSib(PQNodeRoot::LEFT)) //Direction changed
						nextSon->getNodeInfo()->userStructInfo()->changeDir = true;
					keys.pushBack((PQBasicKey<edge,IndInfo*,bool>*) nextSon->getNodeInfo());
					m_pertinentNodes->pushBack(nextSon);
				}
				else
					S.push(nextSon);

				PQNode<edge,IndInfo*,bool> *holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}
		}
	}
}



// The function front scans the frontier of nodePtr. It returns the keys
// of the leaves found in the frontier of nodePtr in a SListPure.
// These keys include keys of direction indicators detected in the frontier.
//
// No direction is assigned to the direction indicators.
//
void EmbedPQTree::getFront(
	PQNode<edge,IndInfo*,bool>* nodePtr,
	SListPure<PQBasicKey<edge,IndInfo*,bool>*> &keys)
{
	Stack<PQNode<edge,IndInfo*,bool>*> S;
	S.push(nodePtr);

	while (!S.empty())
	{
		PQNode<edge,IndInfo*,bool> *checkNode = S.pop();

		if (checkNode->type() == PQNodeRoot::leaf)
			keys.pushBack((PQBasicKey<edge,IndInfo*,bool>*) checkNode->getKey());
		else
		{
			PQNode<edge,IndInfo*,bool>* firstSon  = 0;
			if (checkNode->type() == PQNodeRoot::PNode)
			{
				firstSon = checkNode->referenceChild();
			}
 			else if (checkNode->type() == PQNodeRoot::QNode)
			{
				firstSon = checkNode->getEndmost(PQNodeRoot::RIGHT);
				// By this, we make sure that we start on the left side
				// since the left endmost child will be on top of the stack
			}

			if (firstSon->status() == PQNodeRoot::INDICATOR)
			{
				keys.pushBack((PQBasicKey<edge,IndInfo*,bool>*) firstSon->getNodeInfo());
			}
			else
				S.push(firstSon);

			PQNode<edge,IndInfo*,bool> *nextSon = firstSon->getNextSib(0);
			PQNode<edge,IndInfo*,bool> *oldSib = firstSon;
			while (nextSon && nextSon != firstSon)
			{
				if (nextSon->status() == PQNodeRoot::INDICATOR)
					keys.pushBack((PQBasicKey<edge,IndInfo*,bool>*) nextSon->getNodeInfo());
				else
					S.push(nextSon);

				PQNode<edge,IndInfo*,bool> *holdSib = nextSon->getNextSib(oldSib);
				oldSib = nextSon;
				nextSon = holdSib;
			}
		}
	}
}



}
