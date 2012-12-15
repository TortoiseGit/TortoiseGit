/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class LinearQuadtreeBuilder.
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

#ifndef OGDF_LINEAR_QUADTREE__BUILDER_H
#define OGDF_LINEAR_QUADTREE__BUILDER_H

#include "FastUtils.h"
#include "LinearQuadtree.h"

namespace ogdf {

//! the builder for the LinearQuadtree
class LinearQuadtreeBuilder
{
public:
	//! constructor
	LinearQuadtreeBuilder(LinearQuadtree& treeRef) : tree(treeRef) { n = tree.numberOfPoints(); }

	//! the main build call
	void build();

	//! prepares the node and leaf layer at position l where r is the next position
	void prepareNodeAndLeaf(LinearQuadtree::PointID l, LinearQuadtree::PointID r);

	//! prepares the node and leaf layer from position begin until end (excluding end)
	void prepareTree(LinearQuadtree::PointID begin,  LinearQuadtree::PointID end);

	//! prepares the node and leaf layer for the complete tree from 0 to n (excluding n)
	void prepareTree();

	//! merges the node curr with curr's next node by appending the next nodes children to curr except the first one.
	void mergeWithNext(LinearQuadtree::NodeID curr);

	//! the new link-only recursive builder
	LinearQuadtree::NodeID buildHierarchy(LinearQuadtree::NodeID curr, __uint32 maxLevel);

	//! the main function for the new link-only recursive builder
	void buildHierarchy();

	//! used by restore chain
	inline void restorePushBackChain(LinearQuadtree::NodeID curr)
	{
		if (restoreChainLastNode) tree.setNextNode(restoreChainLastNode, curr); else firstInner = curr;
		restoreChainLastNode = curr;
		numInnerNodes++;
	}

	inline void restoreChain(LinearQuadtree::NodeID curr)
	{
		if (tree.isLeaf(curr))
			return;
		else
		{
			restoreChain(tree.child(curr,0));
			tree.setFirstPoint(curr, tree.firstPoint(tree.child(curr, 0)));
			restorePushBackChain(curr);
			for (__uint32 i = 1; i < tree.numberOfChilds(curr); i++)
				restoreChain(tree.child(curr, i));

			__uint32 lastPoint = tree.firstPoint(tree.child(curr, tree.numberOfChilds(curr)-1)) + tree.numberOfPoints(tree.child(curr, tree.numberOfChilds(curr)-1));
			tree.setNumberOfPoints(curr, lastPoint - tree.firstPoint(curr));
		}
	}

	inline void restoreChain()
	{
		restoreChainLastNode = 0;
		numInnerNodes = 0;
		if (!tree.isLeaf(tree.root()))
			restoreChain(tree.root());
		if (restoreChainLastNode)
			tree.setNextNode(restoreChainLastNode, 0);
	}

	//! returns the level of the first common ancestor of a and b
	inline __uint32 CAL(LinearQuadtree::PointID a, LinearQuadtree::PointID b)
	{
		// 64 bit version
		//if (a<0) return 64; // FIXME: a < 0 is always true (PointID is unsigned int)
		if (b>=tree.numberOfPoints()) return 64;
		__uint32 res = (32-((mostSignificantBit(tree.mortonNr(a) ^ tree.mortonNr(b)))/2));
		return res;
	}

	LinearQuadtree::NodeID firstInner;
	LinearQuadtree::NodeID firstLeaf;

	LinearQuadtree::NodeID lastInner;
	LinearQuadtree::NodeID lastLeaf;
	__uint32 numInnerNodes;
	__uint32 numLeaves;

	LinearQuadtree& tree;
	LinearQuadtree::NodeID restoreChainLastNode;
	LinearQuadtree::PointID n;
};

} // end of namespace ogdf

#endif


