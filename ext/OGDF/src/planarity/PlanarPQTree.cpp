/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the class PlanarPQTree.
 *
 * Implements a PQTree with added features for the planarity test.
 * Used by BoothLueker.
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


#include <ogdf/internal/planarity/PlanarPQTree.h>

namespace ogdf{

// Overriding the function doDestruction (see basic.h)
// Allows deallocation of lists of PlanarLeafKey<IndInfo*>
// and PQLeafKey<edge,IndInfo*,bool> in constant time using OGDF
// memory management.

typedef PlanarLeafKey<IndInfo*> *PtrPlanarLeafKeyI;

template<>
inline bool doDestruction<PtrPlanarLeafKeyI>(const PtrPlanarLeafKeyI*) { return false; }


typedef PQLeafKey<edge,IndInfo*,bool> *PtrPQLeafKeyEIB;

template<>
inline bool doDestruction<PtrPQLeafKeyEIB>(const PtrPQLeafKeyEIB*) { return false; }



// Replaces the pertinent subtree by a P-node with leaves as children
// corresponding to the incoming edges of the node v. These edges
// are to be specified by their keys stored in leafKeys.
void PlanarPQTree::ReplaceRoot(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys)
{
	if (m_pertinentRoot->status() == PQNodeRoot::FULL)
		ReplaceFullRoot(leafKeys);
	else
		ReplacePartialRoot(leafKeys);
}



// The function [[emptyAllPertinentNodes]] has to be called after a reduction
// has been processed. This overloaded function first destroys all full nodes
// by marking them as TO_BE_DELETED and then calling the base class function
// [[emptyAllPertinentNodes]].
void PlanarPQTree::emptyAllPertinentNodes()
{
	ListIterator<PQNode<edge,IndInfo*,bool>*> it;
	for (it = m_pertinentNodes->begin(); it.valid(); it++)
	{
		PQNode<edge,IndInfo*,bool>* nodePtr = (*it);
		if (nodePtr->status() == PQNodeRoot::FULL)
			destroyNode(nodePtr);
	}
	if (m_pertinentRoot)
		m_pertinentRoot->status(PQNodeRoot::FULL);

	PQTree<edge,IndInfo*,bool>::emptyAllPertinentNodes();
}


// Initializes a PQTree by a set of leaves that will korrespond to
// the set of Keys stored in leafKeys.
int PlanarPQTree::Initialize(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys)
{
	SListIterator<PlanarLeafKey<IndInfo*>* >  it;
	SListPure<PQLeafKey<edge,IndInfo*,bool>*> castLeafKeys;
	for (it = leafKeys.begin(); it.valid(); ++it)
		castLeafKeys.pushBack((PQLeafKey<edge,IndInfo*,bool>*) *it);

	return PQTree<edge,IndInfo*,bool>::Initialize(castLeafKeys);
}


// Reduction reduced a set of leaves determined by their keys stored
// in leafKeys. Integer redNumber is for debugging only.
bool PlanarPQTree::Reduction(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys)
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
void PlanarPQTree::ReplaceFullRoot(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys)
{
	if (!leafKeys.empty() && leafKeys.front() == leafKeys.back())
	{
		//ReplaceFullRoot: replace pertinent root by a single leaf
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
		PQInternalNode<edge,IndInfo*,bool> *nodePtr = 0; // dummy
		//replace pertinent root by a $P$-node
		if ((m_pertinentRoot->type() == PQNodeRoot::PNode) ||
			(m_pertinentRoot->type() == PQNodeRoot::QNode))
		{
			nodePtr = (PQInternalNode<edge,IndInfo*,bool>*)m_pertinentRoot;
			nodePtr->type(PQNodeRoot::PNode);
			nodePtr->childCount(0);
			while (!fullChildren(m_pertinentRoot)->empty())
				removeChildFromSiblings(fullChildren(m_pertinentRoot)->popFrontRet());
		}
		else if (m_pertinentRoot->type() == PQNodeRoot::leaf)
		{
			nodePtr = OGDF_NEW PQInternalNode<edge,IndInfo*,bool>(m_identificationNumber++,
														 PQNodeRoot::PNode,PQNodeRoot::EMPTY);
			exchangeNodes(m_pertinentRoot,nodePtr);
			m_pertinentRoot = 0;  // check for this emptyAllPertinentNodes
		}

		SListPure<PQLeafKey<edge,IndInfo*,bool>*> castLeafKeys;
		SListIterator<PlanarLeafKey<IndInfo*>* >  it;
		for (it = leafKeys.begin(); it.valid(); ++it)
			castLeafKeys.pushBack((PQLeafKey<edge,IndInfo*,bool>*) *it);
		addNewLeavesToTree(nodePtr,castLeafKeys);
	}
}


// Function ReplacePartialRoot replaces all full nodes by a single P-node
// with leaves corresponding the keys stored in leafKeys.
void PlanarPQTree::ReplacePartialRoot(SListPure<PlanarLeafKey<IndInfo*>*> &leafKeys)
{
	m_pertinentRoot->childCount(m_pertinentRoot->childCount() + 1 -
		fullChildren(m_pertinentRoot)->size());

	while (fullChildren(m_pertinentRoot)->size() > 1)
		removeChildFromSiblings(fullChildren(m_pertinentRoot)->popFrontRet());

	PQNode<edge,IndInfo*,bool> *currentNode = fullChildren(m_pertinentRoot)->popFrontRet();

	currentNode->parent(m_pertinentRoot);
	m_pertinentRoot = currentNode;
	ReplaceFullRoot(leafKeys);
}


}
