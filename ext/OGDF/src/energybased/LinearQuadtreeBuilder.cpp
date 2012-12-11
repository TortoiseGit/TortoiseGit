/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class LinearQuadtreeBuilder.
 *
 * \author Martin Gronemann
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

#include "LinearQuadtree.h"
#include "LinearQuadtreeBuilder.h"

namespace ogdf {

void LinearQuadtreeBuilder::prepareNodeAndLeaf(LinearQuadtree::PointID leafPos, LinearQuadtree::PointID nextLeafPos)
{
	numLeaves++;
	// first init the leaf on its layer
	tree.initLeaf(leafPos, leafPos, nextLeafPos-leafPos, nextLeafPos);
	// second the node on the inner node layer
	tree.initInnerNode(leafPos + n, leafPos, nextLeafPos, CAL(leafPos, nextLeafPos), nextLeafPos + n);

	lastInner = leafPos + n;
	lastLeaf = leafPos;
}


void LinearQuadtreeBuilder::prepareTree(LinearQuadtree::PointID begin,  LinearQuadtree::PointID end)
{
	LinearQuadtree::PointID i = begin;
	firstLeaf = begin;
	firstInner = firstLeaf+n;
	numLeaves = 0;
	numInnerNodes = 0;
	while (i<end)
	{
		LinearQuadtree::PointID leafPos = i;
		while ((i<end) && (tree.mortonNr(leafPos) == tree.mortonNr(i)))
		{
			tree.setPointLeaf(i, leafPos);
			i++;
		}
		prepareNodeAndLeaf(leafPos, i);
	}
}


void LinearQuadtreeBuilder::prepareTree()
{
	prepareTree(0, n);
}


void LinearQuadtreeBuilder::mergeWithNext(LinearQuadtree::NodeID curr)
{
	LinearQuadtree::NodeID next = tree.nextNode(curr);
	for (__uint32 i = 1; i < tree.numberOfChilds(next); i++)
	{
		tree.setChild(curr, tree.numberOfChilds(curr), tree.child(next, i));
		tree.setNumberOfChilds(curr, tree.numberOfChilds(curr)+1);
	}
	tree.setNextNode(curr, tree.nextNode(next));
}


LinearQuadtree::NodeID LinearQuadtreeBuilder::buildHierarchy(LinearQuadtree::NodeID curr, __uint32 maxLevel)
{
	while ((tree.nextNode(curr)!=lastInner) && (tree.level(tree.nextNode(curr)) < maxLevel))
	{
		LinearQuadtree::NodeID curr_next = tree.nextNode(curr);
		if (tree.level(curr) == tree.level(curr_next))
		{
			mergeWithNext(curr);
		}
		else
		if (tree.level(curr) < tree.level(curr_next))
		{
			tree.setChild(curr_next, 0, curr);
			//pushBackChain(curr);
			curr = curr_next;
		}
		else // if tree.level(curr) > tree.level(curr_next)
		{
			LinearQuadtree::NodeID right_node = buildHierarchy(curr_next, tree.level(curr));
			tree.setChild(curr, tree.numberOfChilds(curr)-1, right_node);
			tree.setNextNode(curr, tree.nextNode(right_node));
		}
	}
	//pushBackChain(curr);
	return curr;
}


void LinearQuadtreeBuilder::buildHierarchy()
{
	tree.clear();
	restoreChainLastNode = 0;
	tree.m_root = buildHierarchy(n, 128);
}


void LinearQuadtreeBuilder::build()
{
	numInnerNodes = 0;
	buildHierarchy();
	restoreChain();;

	tree.m_firstInner = firstInner;
	tree.m_numInnerNodes = numInnerNodes;
	tree.m_firstLeaf = firstLeaf;
	tree.m_numLeaves = numLeaves;
}

} // end of namespace
