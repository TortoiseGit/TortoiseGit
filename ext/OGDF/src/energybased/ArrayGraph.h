/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class ArrayGraph.
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

#ifndef OGDF_ARRAY_GRAPH_H
#define OGDF_ARRAY_GRAPH_H

#include <ogdf/basic/GraphAttributes.h>

namespace ogdf {

//! struct which keeps information aboout incident edges (16 bytes)
struct NodeAdjInfo
{
	__uint32 degree; // total count of pairs where is either the first or second node
	__uint32 firstEntry; // the first pair in the edges chain
	__uint32 lastEntry;  // the last pair in the edges chain
	__uint32 unused; // not used yet
};

//! struct which keeps information about an edge (16 bytes)
struct EdgeAdjInfo
{
	__uint32 a;		// first node of the pair
	__uint32 b;		// second node of the pair
	__uint32 a_next;// next pair in the chain of the first node
	__uint32 b_next;// next pair in the chain of the second node
};


class ArrayGraph
{
public:
	ArrayGraph();
	ArrayGraph(__uint32 maxNumNodes, __uint32 maxNumEdges);
	ArrayGraph(const GraphAttributes& GA, const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize);
	~ArrayGraph();

	//! returns the number of nodes
	/**
	 * \return the number of nodes
	 */
	inline __uint32 numNodes() const { return m_numNodes; }

	//! returns the number of nodes
	/**
	 * \return the number of nodes
	 */
	inline __uint32 numEdges() const { return m_numEdges; }

	//! updates an array graph from GraphAttributes with the given edge lenghts and node sizes and creates the edges;
	/**
	 * The nodes and edges are ordered in the same way like in the Graph instance.
	 * @param GA the GraphAttributes to read from
	 * @param edgeLength the desired edge length
	 */
	void readFrom(const GraphAttributes& GA, const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize);

	//! updates an array graph with the given positions, edge lenghts and node sizes and creates the edges
	/**
	 * The nodes and edges are ordered in the same way like in the Graph instance.
	 * @param G the Graph to traverse
	 * @param xPos the x coordinate for each node
	 * @param yPos the y coordinate for each node
	 * @param nodeSize the x coordinate for each node in G
	 * @param edgeLength the desired edge length for each edge
	 */
	template<typename C_T, typename E_T, typename S_T>
	void readFrom(const Graph& G, NodeArray<C_T>& xPos, NodeArray<C_T>& yPos, const EdgeArray<E_T>& edgeLength, const NodeArray<S_T>& nodeSize)
	{
		m_numNodes = 0;
		m_numEdges = 0;
		NodeArray<__uint32> nodeIndex(G);
		node v;
		m_numNodes = 0;
		m_numEdges = 0;
		m_desiredAvgEdgeLength = 0;
		m_avgNodeSize = 0;
		forall_nodes(v, G)
		{
			m_nodeXPos[m_numNodes] = (float)xPos[v];
			m_nodeYPos[m_numNodes] = (float)yPos[v];
			m_nodeSize[m_numNodes] = (float)nodeSize[v];
			m_avgNodeSize += nodeSize[v];
			nodeIndex[v] = m_numNodes;
			m_numNodes++;
		}
		m_avgNodeSize = m_avgNodeSize / (double)m_numNodes;

		edge e;
		forall_edges(e, G)
		{
			pushBackEdge(nodeIndex[e->source()], nodeIndex[e->target()], (float)edgeLength[e]);
		}
		m_desiredAvgEdgeLength = m_desiredAvgEdgeLength / (double)m_numEdges;
	}

	//! writes the data back to GraphAttributes
	/**
	 * The function does not require to be the same Graph, only the order of nodes and edges
	 * is important
	 * @param GA the GraphAttributes to update
	 */
	void writeTo(GraphAttributes& GA);

	//! writes the data back to node arrays with the given coordinate type
	/**
	 * The function does not require to be the same Graph, only the order of nodes and edges
	 * is important
	 * @param xPos the x coordinate array to update
	 * @param yPos the y coordinate array to update
	 */
	template<typename C_T>
	void writeTo(const Graph& G, NodeArray<C_T>& xPos, NodeArray<C_T>& yPos)
	{
		node v;
		__uint32 i = 0;
		forall_nodes(v, G)
		{
			xPos[v] = m_nodeXPos[i];
			yPos[v] = m_nodeYPos[i];
			i++;
		}
	}

	//! returns the adjacency information for a node
	inline NodeAdjInfo& nodeInfo(__uint32 i) { return m_nodeAdj[i];	}

	//! returns the adjacency information for a node
	inline const NodeAdjInfo& nodeInfo(__uint32 i) const { return m_nodeAdj[i];	}

	//! returns the adjacency information for an edge
	inline EdgeAdjInfo& edgeInfo(__uint32 i) { return m_edgeAdj[i];	}

	//! returns the adjacency information for an edge
	inline const EdgeAdjInfo& edgeInfo(__uint32 i) const { return m_edgeAdj[i];	}

	//! returns the NodeAdjInfo array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline NodeAdjInfo* nodeInfo() { return m_nodeAdj; }

	//! returns the NodeAdjInfo array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline const NodeAdjInfo* nodeInfo() const { return m_nodeAdj; }

	//! returns the EdgeAdjInfo array for all edges
	/**
	 * The array is 16 byte aligned
	 */
	inline EdgeAdjInfo* edgeInfo() { return m_edgeAdj; }

	//! returns the EdgeAdjInfo array for all edges
	/**
	 * The array is 16 byte aligned
	 */
	inline const EdgeAdjInfo* edgeInfo() const { return m_edgeAdj; }

	//! returns the x coord array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline float* nodeXPos() { return m_nodeXPos; }

	//! returns the x coord array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline const float* nodeXPos() const { return m_nodeXPos; }

	//! returns the y coord array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline float* nodeYPos() { return m_nodeYPos; }

	//! returns the y coord array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline const float* nodeYPos() const { return m_nodeYPos; }

	//! returns the node size array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline float* nodeSize() { return m_nodeSize; }


	//! returns the node movement radius array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline float* nodeMoveRadius() { return m_nodeMoveRadius; }

	//! returns the node size array for all nodes
	/**
	 * The array is 16 byte aligned
	 */
	inline const float* nodeSize() const { return m_nodeSize; }

	//! returns the edge length array for all edges
	/**
	 * The array is 16 byte aligned
	 */
	inline float* desiredEdgeLength() { return m_desiredEdgeLength; }

	//! returns the edge length array for all edges
	/**
	 * The array is 16 byte aligned
	 */
	inline const float* desiredEdgeLength() const { return m_desiredEdgeLength; }

	//! returns the index of the first pair of node node
	inline __uint32 firstEdgeAdjIndex(__uint32 nodeIndex) const
	{
		return nodeInfo(nodeIndex).firstEntry;
	};

	//! returns the index of the next pair of curr for node a
	inline __uint32 nextEdgeAdjIndex(__uint32 currEdgeAdjIndex, __uint32 nodeIndex) const
	{
		const EdgeAdjInfo& currInfo = edgeInfo(currEdgeAdjIndex);
		if (currInfo.a == nodeIndex)
			return currInfo.a_next;
		return currInfo.b_next;
	}

	//! returns the other node a is paired with in pair with the given index
	inline __uint32 twinNodeIndex(__uint32 currEdgeAdjIndex, __uint32 nodeIndex) const
	{
		const EdgeAdjInfo& currInfo = edgeInfo(currEdgeAdjIndex);
		if (currInfo.a == nodeIndex)
			return currInfo.b;
		return currInfo.a;
	}

	template<typename Func>
	void for_all_nodes(__uint32 begin, __uint32 end, Func func)
	{
		for(__uint32 i=begin; i <=end; i++)
			func(i);
	}


	inline float avgDesiredEdgeLength() const { return (float)m_desiredAvgEdgeLength; }

	inline float avgNodeSize() const { return (float)m_avgNodeSize; }

	void transform(float translate, float scale);
	void centerGraph();

private:

	//! internal function used by readFrom
	void pushBackEdge(__uint32 a, __uint32 b, float desiredEdgeLength);

	//! internal function used allocate all arrays
	void allocate(__uint32 numNodes, __uint32 numEdges);

	//! internal function used allocate all arrays
	void deallocate();

	//! internal function used to clear the arrays
	void clear()
	{
		for (__uint32 i=0; i < m_numNodes; i++)
			nodeInfo(i).degree = 0;

		m_numNodes = 0;
		m_numEdges = 0;
	}

	//! number of nodes in the graph
	__uint32 m_numNodes;

	//! number of edges in the graph
	__uint32 m_numEdges;

	//! x coordinates
	float* m_nodeXPos;

	//! x coordinates
	float* m_nodeYPos;

	//! size of the node
	float* m_nodeSize;

	//! avg node size
	double  m_avgNodeSize;

	//! maximum node movement length
	float* m_nodeMoveRadius;

	//! edge length
	float* m_desiredEdgeLength;

	double m_currentAvgEdgeLength;

	//! avg edge length
	double m_desiredAvgEdgeLength;

	//! information about adjacent edges
	NodeAdjInfo* m_nodeAdj;

	//! information about adjacent nodes
	EdgeAdjInfo* m_edgeAdj;
};

} // end of namespace ogdf

#endif

