/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class WSPD (well-separated pair decomposition).
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

#ifndef OGDF_WSPD_H
#define OGDF_WSPD_H

#include "LinearQuadtree.h"

namespace ogdf {

//! class for storing per node information
struct WSPDNodeInfo
{
	__uint32 numWSNodes; // total count of pairs where is either the first or second node
	__uint32 firstEntry; // the first pair in the nodes chain
	__uint32 lastEntry;  // the last pair in the nodes chain
};


//! class for storing per pair information
struct WSPDPairInfo
{
	__uint32 a;		// first node of the pair
	__uint32 b;		// second node of the pair
	__uint32 a_next;// next pair in the chain of the first node
	__uint32 b_next;// next pair in the chain of the second node
};


//! class for the Well-Separated-Pairs-Decomposition (WSPD)
class WSPD
{
public:
	typedef LinearQuadtree::NodeID NodeID;

	//! the constructor. allocates the mem
	WSPD(__uint32 maxNumNodes);

	//! destructor
	~WSPD(void);

	//! returns the max number of nodes. (Equals the max number of nodes in the lin quadtree)
	inline __uint32 maxNumNodes() const
	{
		return m_maxNumNodes;
	}

	//! returns the number of well separated nodes for node a
	inline __uint32 numWSNodes(NodeID a) const
	{
		return m_nodeInfo[a].numWSNodes;
	}

	//! returns the total number of pairs
	inline __uint32 numPairs() const
	{
		return m_numPairs;
	}

	//! returns the maximum number of pairs
	inline __uint32 maxNumPairs() const
	{
		return m_maxNumPairs;
	}

	//! resets the array
	void clear();

	//! add a well separated pair (a, b)
	void addWSP(NodeID a, NodeID b)
	{
		// get the index of a free element
		__uint32 e_index = m_numPairs++;

		// get the pair entry
		WSPDPairInfo& e = pairInfo(e_index);

		// (a,b) is the pair we are adding
		e.a = a;
		e.b = b;

		// get the node info
		WSPDNodeInfo& aInfo = nodeInfo(a);
		WSPDNodeInfo& bInfo = nodeInfo(b);

		// if a is part of at least one pair
		if (aInfo.numWSNodes)
		{
			// adjust the links
			WSPDPairInfo& a_e = pairInfo(aInfo.lastEntry);
			// check which one is a
			if (a==a_e.a)
				a_e.a_next = e_index;
			else
				a_e.b_next = e_index;
		} else
		{
			// this pair is the first for a => set the firstEntry link
			aInfo.firstEntry = e_index;
		}

		// same for b: if a is part of at least one pair
		if (bInfo.numWSNodes)
		{
			// adjust the links
			WSPDPairInfo& b_e = pairInfo(bInfo.lastEntry);
			// check which one is b
			if (b==b_e.a)
				b_e.a_next = e_index;
			else
				b_e.b_next = e_index;
		} else
		{
			// this pair is the first for b => set the firstEntry link
			bInfo.firstEntry = e_index;
		}
		// and the lastEntry link
		aInfo.lastEntry = e_index;
		bInfo.lastEntry = e_index;
		// one more pair for each node
		aInfo.numWSNodes++;
		bInfo.numWSNodes++;
	}

	//! returns the PairInfo by index
	inline WSPDPairInfo& pairInfo(__uint32 pairIndex) const
	{
		return m_pairs[pairIndex];
	}

	//! returns the NodeInfo by index
	inline WSPDNodeInfo& nodeInfo(NodeID node) const
	{
		return m_nodeInfo[node];
	}

	//! returns the index of the next pair of curr for node a
	inline __uint32 nextPair(__uint32 currPairIndex, NodeID a) const
	{
		const WSPDPairInfo& currInfo = pairInfo(currPairIndex);
		if (currInfo.a == a)
			return currInfo.a_next;
		return currInfo.b_next;
	}

	//! returns the other node a is paired with in pair with the given index
	inline __uint32 wsNodeOfPair(__uint32 currPairIndex, NodeID a) const
	{
		const WSPDPairInfo& currInfo = pairInfo(currPairIndex);
		if (currInfo.a == a)
			return currInfo.b;
		return currInfo.a;
	}

	//! returns the index of the first pair of node node
	inline __uint32 firstPairEntry(NodeID node) const
	{
		return m_nodeInfo[node].firstEntry;
	}

	// returns the size excluding small member vars (for profiling only)
	unsigned long sizeInBytes() const;

private:
	//! allocates all memory
	void allocate();

	//! releases all memory
	void deallocate();

	//! the max number of nodes. (Equals the max number of nodes in the lin quadtree)
	__uint32 m_maxNumNodes;

	//! the array which holds the wspd information for one quadtree node
	WSPDNodeInfo* m_nodeInfo;

	//! the array containing all pairs
	WSPDPairInfo* m_pairs;

	//! the total number of pairs
	__uint32 m_numPairs;

	//! the upper bound for the number of pairs
	__uint32 m_maxNumPairs;
};

} // end of namespace ogdf

#endif

