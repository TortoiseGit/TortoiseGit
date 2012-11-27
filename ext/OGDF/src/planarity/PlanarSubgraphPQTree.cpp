/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the class PlanarSubgraphPQTree.
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

#include <ogdf/internal/planarity/PlanarSubgraphPQTree.h>

namespace ogdf{

// Replaces the pertinent subtree by a P-node with leaves as children
// corresponding to the incoming edges of the node v. These edges
// are to be specified by their keys stored in leafKeys.
void PlanarSubgraphPQTree::
ReplaceRoot(SListPure<PlanarLeafKey<whaInfo*>*> &leafKeys)
{
	if (m_pertinentRoot->status() == PQNodeRoot::FULL)
		ReplaceFullRoot(leafKeys);
	else
		ReplacePartialRoot(leafKeys);
}

// Initializes a PQTree by a set of leaves that will korrespond to
// the set of Keys stored in leafKeys.
int PlanarSubgraphPQTree::
Initialize(SListPure<PlanarLeafKey<whaInfo*>*> &leafKeys)
{
	SListIterator<PlanarLeafKey<whaInfo*>* >  it;

	SListPure<PQLeafKey<edge,whaInfo*,bool>*> castLeafKeys;
	for (it = leafKeys.begin(); it.valid(); ++it)
		castLeafKeys.pushBack((PQLeafKey<edge,whaInfo*,bool>*) *it);

	return PQTree<edge,whaInfo*,bool>::Initialize(castLeafKeys);
}


// Reduction reduced a set of leaves determined by their keys stored
// in leafKeys. Integer redNumber is for debugging only.
bool PlanarSubgraphPQTree::Reduction(
	SListPure<PlanarLeafKey<whaInfo*>*>   &leafKeys,
	SList<PQLeafKey<edge,whaInfo*,bool>*> &eliminatedKeys)
{
	SListPure<PQLeafKey<edge,whaInfo*,bool>*> castLeafKeys;

	SListIterator<PlanarLeafKey<whaInfo*>* >  it;
	for (it = leafKeys.begin(); it.valid(); ++it)
	{
		castLeafKeys.pushBack((PQLeafKey<edge,whaInfo*,bool>*) *it);
	}

	determineMinRemoveSequence(castLeafKeys,eliminatedKeys);
	removeEliminatedLeaves(eliminatedKeys);

	SListIterator<PQLeafKey<edge,whaInfo*,bool>* >  itn = castLeafKeys.begin();
	SListIterator<PQLeafKey<edge,whaInfo*,bool>* >  itp = itn++;
	for (; itn.valid();)
	{
		if ((*itn)->nodePointer()->status()== PQNodeRoot::WHA_DELETE)
		{
			itn++;
			castLeafKeys.delSucc(itp);
		}
		else
			itp = itn++;
	}

	if ((*castLeafKeys.begin())->nodePointer()->status() == PQNodeRoot::WHA_DELETE)
		castLeafKeys.popFront();


	return Reduce(castLeafKeys);
}



// Function ReplaceFullRoot either replaces the full root
// or one full child of a partial root of a pertinent subtree
// by a single P-node  with leaves corresponding the keys stored in leafKeys.
void PlanarSubgraphPQTree::
ReplaceFullRoot(SListPure<PlanarLeafKey<whaInfo*>*> &leafKeys)
{

	PQLeaf<edge,whaInfo*,bool>          *leafPtr     = 0; // dummy
	PQInternalNode<edge,whaInfo*,bool>	*nodePtr     = 0; // dummy
	PQNode<edge,whaInfo*,bool>		    *currentNode = 0; // dummy
	SListIterator<PlanarLeafKey<whaInfo*>* >  it;

	if (!leafKeys.empty() && leafKeys.front() == leafKeys.back())
	{
		//ReplaceFullRoot: replace pertinent root by a single leaf
		leafPtr = OGDF_NEW PQLeaf<edge,whaInfo*,bool>(m_identificationNumber++,
			PQNodeRoot::EMPTY,(PQLeafKey<edge,whaInfo*,bool>*)leafKeys.front());
		exchangeNodes(m_pertinentRoot,(PQNode<edge,whaInfo*,bool>*) leafPtr);
		if (m_pertinentRoot == m_root)
			m_root = (PQNode<edge,whaInfo*,bool>*) leafPtr;
	}
	else if (!leafKeys.empty()) // at least two leaves
	{
		//replace pertinent root by a $P$-node
		if ((m_pertinentRoot->type() == PQNodeRoot::PNode) ||
			(m_pertinentRoot->type() == PQNodeRoot::QNode))
		{
			nodePtr = (PQInternalNode<edge,whaInfo*,bool>*)m_pertinentRoot;
			nodePtr->type(PQNodeRoot::PNode);
			nodePtr->status(PQNodeRoot::PERTROOT);
			nodePtr->childCount(0);
			while (!fullChildren(m_pertinentRoot)->empty())
			{
				currentNode = fullChildren(m_pertinentRoot)->popFrontRet();
				removeChildFromSiblings(currentNode);
			}
		}
		else if (m_pertinentRoot->type() == PQNodeRoot::leaf)
		{
			nodePtr = OGDF_NEW PQInternalNode<edge,whaInfo*,bool>(m_identificationNumber++,
														 PQNodeRoot::PNode,PQNodeRoot::EMPTY);
			exchangeNodes(m_pertinentRoot,nodePtr);
		}
		SListPure<PQLeafKey<edge,whaInfo*,bool>*> castLeafKeys;
		for (it = leafKeys.begin(); it.valid(); ++it)
			castLeafKeys.pushBack((PQLeafKey<edge,whaInfo*,bool>*) *it);
		addNewLeavesToTree(nodePtr,castLeafKeys);
	}

}


// Function ReplacePartialRoot replaces all full nodes by a single P-node
// with leaves corresponding the keys stored in leafKeys.
void PlanarSubgraphPQTree::
	ReplacePartialRoot(SListPure<PlanarLeafKey<whaInfo*>*> &leafKeys)

{
	PQNode<edge,whaInfo*,bool>  *currentNode = NULL;

	m_pertinentRoot->childCount(m_pertinentRoot->childCount() + 1 -
		fullChildren(m_pertinentRoot)->size());

	while (fullChildren(m_pertinentRoot)->size() > 1)
	{
		currentNode = fullChildren(m_pertinentRoot)->popFrontRet();
		removeChildFromSiblings(currentNode);
	}

	currentNode = fullChildren(m_pertinentRoot)->popFrontRet();

	currentNode->parent(m_pertinentRoot);
	m_pertinentRoot = currentNode;
	ReplaceFullRoot(leafKeys);

}


/**
The function removeEliminatedLeaves handles the difficult task of
cleaning up after every reduction.

After a reduction is complete, different kind of garbage has to be
handled.
\begin{itemize}
\item Pertinent leaves that are not in the maximal pertinent sequence.
	from the $PQ$-tree in order to get it reducable have to be deleted.
\item The memory of some pertinent nodes, that have only pertinent leaves not beeing
	in the maximal pertinent sequence in their frontier has to be freed.
\item Pertinent nodes that have only one child left after the removal
	of pertinent leaves not beeing in the maximal pertinent sequence
	have to be deleted.
\item The memory of all full nodes has to be freed, since the complete
	pertinent subtree is replaced by a $P$-node after the reduction.
\item Nodes, that have been removed during the call of the function [[Reduce]]
	of the base class template [[PQTree]] from the $PQ$-tree have to be
	kept but marked as nonexisting.
\end{itemize}.
*/

/**************************************************************************************
							removeEliminatedLeaves
***************************************************************************************/

void PlanarSubgraphPQTree::
removeEliminatedLeaves(SList<PQLeafKey<edge,whaInfo*,bool>*> &eliminatedKeys)
{
	PQNode<edge,whaInfo*,bool>*  nodePtr = 0;
	PQNode<edge,whaInfo*,bool>*  parent  = 0;
	PQNode<edge,whaInfo*,bool>*  sibling = 0;

	SListIterator<PQLeafKey<edge,whaInfo*,bool>*> it;
	for (it = eliminatedKeys.begin(); it.valid(); it++)
	{
		nodePtr = (*it)->nodePointer();
		parent = nodePtr->parent();
		sibling = nodePtr->getNextSib(NULL);

		removeNodeFromTree(parent,nodePtr);
		checkIfOnlyChild(sibling,parent);
		if (parent->status() == PQNodeRoot::TO_BE_DELETED)
		{
			parent->status(PQNodeRoot::WHA_DELETE);
		}
		nodePtr->status(PQNodeRoot::WHA_DELETE);
	}
}



}
