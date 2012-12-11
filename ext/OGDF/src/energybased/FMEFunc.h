/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Definitions of various auxiliary classes for FME layout.
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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_FME_FUNC_H
#define OGDF_FME_FUNC_H

#include "ArrayGraph.h"
#include "LinearQuadtree.h"
#include "LinearQuadtreeExpansion.h"
#include "LinearQuadtreeBuilder.h"
#include "WSPD.h"
#include "FMEKernel.h"
#include <list>

namespace ogdf {

//! struct for distributing subtrees to the threads
struct FMETreePartition
{
	std::list<LinearQuadtree::NodeID> nodes;
	__uint32 pointCount;

	template<typename Func>
	void for_loop(Func& func)
	{
		for (std::list<__uint32>::const_iterator it = nodes.begin();it!=nodes.end(); it++)
			func(*it);
	}
};

struct FMENodeChainPartition
{
	__uint32 begin;
	__uint32 numNodes;
};


//! the main global options for a run
struct FMEGlobalOptions
{
	float preProcTimeStep;				//!< time step factor for the preprocessing step
	float preProcEdgeForceFactor;		//!< edge force factor for the preprocessing step
	__uint32 preProcMaxNumIterations;	//!< number of iterations the preprocessing is applied

	float timeStep;						//!< time step factor for the main step
	float edgeForceFactor;				//!< edge force factor for the main step
	float repForceFactor;				//!< repulsive force factor for the main step
	float normEdgeLength;				//!< average edge length when desired edge length are normalized
	float normNodeSize;					//!< average node size when node sizes are normalized
	__uint32 maxNumIterations;			//!< maximum number of iterations in the main step
	__uint32 minNumIterations;			//!< minimum number of iterations to be done regardless of any other conditions

	bool doPrepProcessing;				//!< enable preprocessing
	bool doPostProcessing;				//!< enable postprocessing

	double stopCritForce;				//!< stopping criteria
	double stopCritAvgForce;				//!< stopping criteria
	double stopCritConstSq;				//!< stopping criteria

	__uint32 multipolePrecision;
};


//! forward decl of local context struct
struct FMELocalContext;

/*!
 * Global Context
*/
struct FMEGlobalContext
{
	FMELocalContext** pLocalContext;		//!< all local contexts
	__uint32 numThreads;					//!< number of threads, local contexts
	ArrayGraph* pGraph;						//!< pointer to the array graph
	LinearQuadtree* pQuadtree;				//!< pointer to the quadtree
	LinearQuadtreeExpansion* pExpansion;	//!< pointer to the coeefficients
	WSPD* pWSPD;							//!< pointer to the well separated pairs decomposition
	float* globalForceX;					//!< the global node force x array
	float* globalForceY;					//!< the global node force y array
	FMEGlobalOptions* pOptions;				//!< pointer to the global options
	bool earlyExit;							//!< var for the main thread to notify the other threads that they are done
	float scaleFactor;						//!< var
	float coolDown;
	float min_x;							//!< global point, node min x coordinate for bounding box calculations
	float max_x;							//!< global point, node max x coordinate for bounding box calculations
	float min_y;							//!< global point, node min y coordinate for bounding box calculations
	float max_y;							//!< global point, node max y coordinate for bounding box calculations
	double currAvgEdgeLength;
};


/*!
 * Local Thread Context
*/
struct FMELocalContext
{
	FMEGlobalContext* pGlobalContext;		//!< pointer to the global context
	float* forceX;							//!< local force array for all nodes, points
	float* forceY;							//!< local force array for all nodes, points
	double maxForceSq;						//!< local maximum force
	double avgForce;						//!< local maximum force
	float min_x;							//!< global point, node min x coordinate for bounding box calculations
	float max_x;							//!< global point, node max x coordinate for bounding box calculations
	float min_y;							//!< global point, node min y coordinate for bounding box calculations
	float max_y;							//!< global point, node max y coordinate for bounding box calculations
	double currAvgEdgeLength;
	FMETreePartition treePartition;			  //!< tree partition assigned to the thread
	FMENodeChainPartition innerNodePartition; //!< chain of inner nodes assigned to the thread
	FMENodeChainPartition leafPartition;	  //!< chain of leaf nodes assigned to the thread

	LinearQuadtree::NodeID firstInnerNode;    //!< first inner nodes the thread prepared
	LinearQuadtree::NodeID lastInnerNode;     //!< last inner nodes the thread prepared
	__uint32 numInnerNodes;					  //!< number of inner nodes the thread prepared

	LinearQuadtree::NodeID firstLeaf;		  //!< first leaves the thread prepared
	LinearQuadtree::NodeID lastLeaf;		  //!< last leaves the thread prepared
	__uint32 numLeaves;						  //!< number of leaves the thread prepared
};


//! creates a min max functor for the x coords of the node
static inline min_max_functor<float> min_max_x_function(FMELocalContext* pLocalContext)
{
	return min_max_functor<float>(pLocalContext->pGlobalContext->pGraph->nodeXPos(), pLocalContext->min_x, pLocalContext->max_x);
}

//! creates a min max functor for the y coords of the node
static inline min_max_functor<float> min_max_y_function(FMELocalContext* pLocalContext)
{
	return min_max_functor<float>(pLocalContext->pGlobalContext->pGraph->nodeYPos(), pLocalContext->min_y, pLocalContext->max_y);
}

class LQMortonFunctor
{
public:
	inline LQMortonFunctor ( FMELocalContext* pLocalContext )
	{
		x = pLocalContext->pGlobalContext->pGraph->nodeXPos();
		y = pLocalContext->pGlobalContext->pGraph->nodeYPos();
		s = pLocalContext->pGlobalContext->pGraph->nodeSize();
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		translate_x = -quadtree->minX();
		translate_y = -quadtree->minY();
		scale = quadtree->scaleInv();
	}

	inline __uint32 operator()(void) const
	{
		return quadtree->numberOfPoints();
	}

	inline void operator()(__uint32 i)
	{
		LinearQuadtree::LQPoint& p = quadtree->point(i);
		__uint32 ref = p.ref;
		p.mortonNr = mortonNumber<__uint64, __uint32>((__uint32)((x[ref] + translate_x)*scale), (__uint32)((y[ref] + translate_y)*scale));
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	float translate_x;
	float translate_y;
	double scale;
	float* x;
	float* y;
	float* s;
};


//! Point-to-Multipole functor
struct p2m_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;

	p2m_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e) : tree(t), expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		__uint32 numPointsInLeaf = tree.numberOfPoints(nodeIndex);
		__uint32 firstPointOfLeaf = tree.firstPoint(nodeIndex);
		for (__uint32 pointIndex=firstPointOfLeaf; pointIndex < (firstPointOfLeaf+numPointsInLeaf); pointIndex++)
		{
			expansions.P2M(pointIndex, nodeIndex);
		}
	}
};


//! creates a Point-to-Multipole functor
static inline p2m_functor p2m_function(FMELocalContext* pLocalContext)
{
	return p2m_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion);
}


//! Multipole-to-Multipole functor
struct m2m_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;

	m2m_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e) : tree(t), expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID parent, LinearQuadtree::NodeID child)
	{
		expansions.M2M(child, parent);
	}

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		tree.forall_children(pair_call(*this, nodeIndex))(nodeIndex);
	}
};


//! creates Multipole-to-Multipole functor
static inline m2m_functor m2m_function(FMELocalContext* pLocalContext)
{
	return m2m_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion);
}


//! Multipole-to-Local functor
struct m2l_functor
{
	LinearQuadtreeExpansion& expansions;

	m2l_functor(LinearQuadtreeExpansion& e) : expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndexSource, LinearQuadtree::NodeID nodeIndexReceiver)
	{
		expansions.M2L(nodeIndexSource, nodeIndexReceiver);
	}
};


//! creates Multipole-to-Local functor
static inline m2l_functor m2l_function(FMELocalContext* pLocalContext)
{
	return m2l_functor(*pLocalContext->pGlobalContext->pExpansion);
}


//! Local-to-Local functor
struct l2l_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;

	l2l_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e) : tree(t), expansions(e) { }

	inline void operator()(LinearQuadtree::NodeID parent, LinearQuadtree::NodeID child)
	{
		expansions.L2L(parent, child);
	}

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		tree.forall_children(pair_call(*this, nodeIndex))(nodeIndex);
	}
};


//! creates Local-to-Local functor
static inline l2l_functor l2l_function(FMELocalContext* pLocalContext)
{
	return l2l_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion);
}


//! Local-to-Point functor
struct l2p_functor
{
	const LinearQuadtree& tree;
	LinearQuadtreeExpansion& expansions;
	float* fx;
	float* fy;

	l2p_functor(const LinearQuadtree& t, LinearQuadtreeExpansion& e, float* x, float* y) : tree(t), expansions(e), fx(x), fy(y) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndex, LinearQuadtree::PointID pointIndex)
	{
		expansions.L2P(nodeIndex, pointIndex, fx[pointIndex], fy[pointIndex]);
	}

	inline void operator()(LinearQuadtree::PointID pointIndex)
	{
		LinearQuadtree::NodeID nodeIndex = tree.pointLeaf(pointIndex);
		this->operator ()(nodeIndex, pointIndex);
	}
};


//! creates Local-to-Point functor
static inline l2p_functor l2p_function(FMELocalContext* pLocalContext)
{
	return l2p_functor(*pLocalContext->pGlobalContext->pQuadtree, *pLocalContext->pGlobalContext->pExpansion, pLocalContext->forceX, pLocalContext->forceY);
}


//! Local-to-Point functor
struct p2p_functor
{
	const LinearQuadtree& tree;
	float* fx;
	float* fy;

	p2p_functor(const LinearQuadtree& t, float* x, float* y) : tree(t), fx(x), fy(y) { }

	inline void operator()(LinearQuadtree::NodeID nodeIndexA, LinearQuadtree::NodeID nodeIndexB)
	{
		__uint32 offsetA = tree.firstPoint(nodeIndexA);
		__uint32 offsetB = tree.firstPoint(nodeIndexB);
		__uint32 numPointsA = tree.numberOfPoints(nodeIndexA);
		__uint32 numPointsB = tree.numberOfPoints(nodeIndexB);
		eval_direct_fast(tree.pointX() + offsetA, tree.pointY() + offsetA, tree.pointSize() + offsetA, fx + offsetA, fy + offsetA, numPointsA,
						 tree.pointX() + offsetB, tree.pointY() + offsetB, tree.pointSize() + offsetB, fx + offsetB, fy + offsetB, numPointsB);
	}

	inline void operator()(LinearQuadtree::NodeID nodeIndex)
	{
		__uint32 offset = tree.firstPoint(nodeIndex);
		__uint32 numPoints = tree.numberOfPoints(nodeIndex);
		eval_direct_fast(tree.pointX() + offset, tree.pointY() + offset, tree.pointSize() + offset, fx + offset, fy + offset, numPoints);
	}
};


//! creates Local-to-Point functor
static inline p2p_functor p2p_function(FMELocalContext* pLocalContext)
{
	return p2p_functor(*pLocalContext->pGlobalContext->pQuadtree, pLocalContext->forceX, pLocalContext->forceY);
}


//! The partitioner which partitions the quadtree into subtrees and partitions the sequence of inner nodes and leaves
class LQPartitioner
{
public:
	LQPartitioner( FMELocalContext* pLocalContext )
	{
		numThreads = pLocalContext->pGlobalContext->numThreads;
		tree = pLocalContext->pGlobalContext->pQuadtree;
		localContexts = pLocalContext->pGlobalContext->pLocalContext;
	}

	void partitionNodeChains()
	{
		__uint32 numInnerNodesPerThread = tree->numberOfInnerNodes() / numThreads;
		if (numInnerNodesPerThread < 25)
		{
			localContexts[0]->innerNodePartition.begin = tree->firstInnerNode();
			localContexts[0]->innerNodePartition.numNodes =  tree->numberOfInnerNodes();
			for (__uint32 i=1; i< numThreads; i++)
			{
				localContexts[i]->innerNodePartition.numNodes = 0;
			}
		} else
		{

			LinearQuadtree::NodeID curr = tree->firstInnerNode();
			currThread = 0;
			localContexts[0]->innerNodePartition.begin = curr;
			localContexts[0]->innerNodePartition.numNodes = 0;
			for (__uint32 i=0; i< tree->numberOfInnerNodes(); i++)
			{
				localContexts[currThread]->innerNodePartition.numNodes++;
				curr = tree->nextNode(curr);
				if ((localContexts[currThread]->innerNodePartition.numNodes>=numInnerNodesPerThread) && (currThread < numThreads-1))
				{
					currThread++;
					localContexts[currThread]->innerNodePartition.numNodes = 0;
					localContexts[currThread]->innerNodePartition.begin = curr;
				}
			}

		}

		__uint32 numLeavesPerThread = tree->numberOfLeaves() / numThreads;
		if (numLeavesPerThread < 25)
		{
			localContexts[0]->leafPartition.begin = tree->firstLeaf();
			localContexts[0]->leafPartition.numNodes =  tree->numberOfLeaves();
			for (__uint32 i=1; i< numThreads; i++)
			{
				localContexts[i]->leafPartition.numNodes = 0;
			}
		} else
		{
			LinearQuadtree::NodeID curr = tree->firstLeaf();
			currThread = 0;
			localContexts[0]->leafPartition.begin = curr;
			localContexts[0]->leafPartition.numNodes = 0;
			for (__uint32 i=0; i< tree->numberOfLeaves(); i++)
			{
				localContexts[currThread]->leafPartition.numNodes++;
				curr = tree->nextNode(curr);
				if ((localContexts[currThread]->leafPartition.numNodes>=numLeavesPerThread) && (currThread < numThreads-1))
				{
					currThread++;
					localContexts[currThread]->leafPartition.numNodes = 0;
					localContexts[currThread]->leafPartition.begin = curr;
				}
			}
		}
	}

	void partition()
	{
		partitionNodeChains();
		currThread = 0;
		numPointsPerThread = tree->numberOfPoints() / numThreads;
		for (__uint32 i=0; i < numThreads; i++)
		{
			localContexts[i]->treePartition.nodes.clear();
			localContexts[i]->treePartition.pointCount = 0;
		}
		if (numThreads>1)
			newPartition();
	}

	void newPartition(__uint32 node)
	{
		__uint32 bound = tree->numberOfPoints() / (numThreads*numThreads);

		if (tree->isLeaf(node) || (tree->numberOfPoints(node) < bound))
			l_par.push_back(node);
		else
			for (__uint32 i = 0; i < tree->numberOfChilds(node); i++)
				newPartition(tree->child(node, i));
	}

	void newPartition()
	{
		l_par.clear();
		newPartition(tree->root());
		__uint32 bound = (tree->numberOfPoints() / (numThreads)) + (tree->numberOfPoints() / (numThreads*numThreads*2));
		while (!l_par.empty())
		{
			FMETreePartition* partition = currPartition();
			__uint32 v = l_par.front();
			if (((partition->pointCount + tree->numberOfPoints(v)) <= bound) ||
				(currThread==numThreads-1))
			{
				partition->pointCount += tree->numberOfPoints(v);
				partition->nodes.push_back(v);
				tree->nodeFence(v);
				l_par.pop_front();
			} else
			{
				currThread++;
			}
		}
	}

	FMETreePartition* currPartition()
	{
		return &localContexts[currThread]->treePartition;
	}

private:
	__uint32 numPointsPerThread;
	__uint32 numThreads;
	__uint32 currThread;
	__uint32 currPointCount;
	std::list<__uint32> l_par;
	LinearQuadtree* tree;
	FMELocalContext** localContexts;
};


class LQPointUpdateFunctor
{
public:
	inline LQPointUpdateFunctor ( FMELocalContext* pLocalContext )
	{
		x = pLocalContext->pGlobalContext->pGraph->nodeXPos();
		y = pLocalContext->pGlobalContext->pGraph->nodeYPos();
		s = pLocalContext->pGlobalContext->pGraph->nodeSize();
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
	}

	inline __uint32 operator()(void) const
	{
		return quadtree->numberOfPoints();
	}

	inline void operator()(__uint32 i)
	{
		LinearQuadtree::LQPoint& p = quadtree->point(i);
		__uint32 ref = p.ref;
		quadtree->setPoint(i, x[ref], y[ref], s[ref]);
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	float* x;
	float* y;
	float* s;
};


/*!
 * Computes the coords and size of the i-th node in the LinearQuadtree
 */
class LQCoordsFunctor
{
public:
	inline LQCoordsFunctor(	FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
	}

	inline __uint32 operator()(void) const
	{
		return quadtree->numberOfNodes();
	}

	inline void operator()( __uint32 i )
	{
		quadtree->computeCoords(i);
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
};


/*!
 * Converts the multipole expansion coefficients from all nodes which are well separated from the i-th node
 * to local expansion coefficients and adds them to the local expansion coefficients of the i-th node
 */
class M2LFunctor
{
public:
	inline M2LFunctor( FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
		wspd = pLocalContext->pGlobalContext->pWSPD;
	}

	inline __uint32 operator()(void) const
	{
		return quadtree->numberOfNodes();
	}

	inline void operator()(__uint32 i)
	{
		__uint32 currEntryIndex = wspd->firstPairEntry(i);
		for (__uint32 k = 0; k < wspd->numWSNodes(i); k++)
		{
			__uint32 j = wspd->wsNodeOfPair(currEntryIndex, i);
			quadtreeExp->M2L(j, i);
			currEntryIndex = wspd->nextPair(currEntryIndex, i);
		}
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
	WSPD* wspd;
};


/*!
 * Calculates the repulsive forces acting between all nodes inside the cell of the i-th LinearQuadtree node.
 */
class NDFunctor
{
public:
	inline NDFunctor( FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
		forceArrayX = pLocalContext->forceX;
		forceArrayY = pLocalContext->forceY;
	}

	inline __uint32 operator()(void) const
	{
		return quadtree->numberOfDirectNodes();
	}

	inline void operator()(__uint32 i)
	{
		__uint32 node = quadtree->directNode(i);
		__uint32 offset = quadtree->firstPoint(node);
		__uint32 numPoints = quadtree->numberOfPoints(node);
		eval_direct_fast(quadtree->pointX() + offset, quadtree->pointY() + offset, quadtree->pointSize() + offset, forceArrayX + offset, forceArrayY + offset, numPoints);
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
	float* forceArrayX;
	float* forceArrayY;
};


/*!
 * Calculates the repulsive forces acting between all nodes of the direct interacting cells of the i-th node
 */
class D2DFunctor
{
public:
	inline D2DFunctor( FMELocalContext* pLocalContext )
	{
		quadtree = pLocalContext->pGlobalContext->pQuadtree;
		quadtreeExp = pLocalContext->pGlobalContext->pExpansion;
		forceArrayX = pLocalContext->forceX;
		forceArrayY = pLocalContext->forceY;
	}

	inline __uint32 operator()(void) const
	{
		return quadtree->numberOfDirectPairs();
	}

	inline void operator()(__uint32 i)
	{
		__uint32 nodeA = quadtree->directNodeA(i);
		__uint32 nodeB = quadtree->directNodeB(i);
		__uint32 offsetA = quadtree->firstPoint(nodeA);
		__uint32 offsetB = quadtree->firstPoint(nodeB);
		__uint32 numPointsA = quadtree->numberOfPoints(nodeA);
		__uint32 numPointsB = quadtree->numberOfPoints(nodeB);
		eval_direct_fast(quadtree->pointX() + offsetA, quadtree->pointY() + offsetA, quadtree->pointSize() + offsetA, forceArrayX + offsetA, forceArrayY + offsetA, numPointsA,
						 quadtree->pointX() + offsetB, quadtree->pointY() + offsetB, quadtree->pointSize() + offsetB, forceArrayX + offsetB, forceArrayY + offsetB, numPointsB);
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	LinearQuadtree* quadtree;
	LinearQuadtreeExpansion* quadtreeExp;
	float* forceArrayX;
	float* forceArrayY;
};


enum
{
	EDGE_FORCE_SUB_REP			= 0x2,
	EDGE_FORCE_DIV_DEGREE		= 0x8
};


template<unsigned int FLAGS>
class EdgeForceFunctor
{
public:
	inline EdgeForceFunctor( FMELocalContext* pLocalContext )
	{
		pGraph = pLocalContext->pGlobalContext->pGraph;
		x = pGraph->nodeXPos();
		y = pGraph->nodeYPos();
		edgeInfo = pGraph->edgeInfo();
		nodeInfo = pGraph->nodeInfo();
		desiredEdgeLength = pGraph->desiredEdgeLength();
		nodeSize = pGraph->nodeSize();
		forceArrayX = pLocalContext->forceX;
		forceArrayY = pLocalContext->forceY;
	}

	inline __uint32 operator()(void) const
	{
		return pGraph->numEdges();
	}

	inline void operator()(__uint32 i)
	{
		const EdgeAdjInfo& e_info = edgeInfo[i];
		const NodeAdjInfo& a_info = nodeInfo[e_info.a];
		const NodeAdjInfo& b_info = nodeInfo[e_info.b];

		float d_x = x[e_info.a] - x[e_info.b];
		float d_y = y[e_info.a] - y[e_info.b];
		float d_sq = d_x*d_x + d_y*d_y;

		float f = (float)(logf(d_sq)*0.5f-logf(desiredEdgeLength[i]));

		float fa = f*0.25f;
		float fb = f*0.25f;

		if (FLAGS & EDGE_FORCE_DIV_DEGREE)
		{
			fa = (float)(fa/((float)a_info.degree));
			fb = (float)(fb/((float)b_info.degree));
		}

		if (FLAGS & EDGE_FORCE_SUB_REP)
		{
			fa += (nodeSize[e_info.b] / d_sq);
			fb += (nodeSize[e_info.a] / d_sq);
		}
		forceArrayX[e_info.a] -= fa*d_x;
		forceArrayY[e_info.a] -= fa*d_y;
		forceArrayX[e_info.b] += fb*d_x;
		forceArrayY[e_info.b] += fb*d_y;
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	float* x;
	float* y;
	EdgeAdjInfo* edgeInfo;
	NodeAdjInfo* nodeInfo;

	ArrayGraph* pGraph;
	float* desiredEdgeLength;
	float* nodeSize;
	float* forceArrayX;
	float* forceArrayY;
};


template<unsigned int FLAGS>
static inline EdgeForceFunctor<FLAGS> edge_force_function( FMELocalContext* pLocalContext )
{
	return EdgeForceFunctor<FLAGS>( pLocalContext );
}


enum
{
	COLLECT_NO_FACTOR			= 0x00,
	COLLECT_EDGE_FACTOR			= 0x01,
	COLLECT_REPULSIVE_FACTOR	= 0x02,
	COLLECT_EDGE_FACTOR_PREP	= 0x04,
	COLLECT_TREE_2_GRAPH_ORDER	= 0x08,
	COLLECT_ZERO_THREAD_ARRAY	= 0x10
};


template<unsigned int FLAGS>
class CollectForceFunctor
{
public:

	inline CollectForceFunctor( FMELocalContext* pLocalContext )
	{
		numContexts = pLocalContext->pGlobalContext->numThreads;
		globalContext = pLocalContext->pGlobalContext;
		localContexts = pLocalContext->pGlobalContext->pLocalContext;
		globalArrayX = globalContext->globalForceX;
		globalArrayY = globalContext->globalForceY;
		pGraph = pLocalContext->pGlobalContext->pGraph;
		if (FLAGS & COLLECT_EDGE_FACTOR)
			factor = pLocalContext->pGlobalContext->pOptions->edgeForceFactor;
		else
		if (FLAGS & COLLECT_REPULSIVE_FACTOR)
			factor = pLocalContext->pGlobalContext->pOptions->repForceFactor;
		else
		if (FLAGS & COLLECT_EDGE_FACTOR_PREP)
			factor = pLocalContext->pGlobalContext->pOptions->preProcEdgeForceFactor;
		else
			factor = 1.0;
	}

	inline __uint32 operator()(void) const
	{
		return pGraph->numNodes();
	}


	inline void operator()(__uint32 i)
	{
		float sumX = 0.0f;
		float sumY = 0.0f;
		for (__uint32 j=0; j < numContexts; j++)
		{
			float* localArrayX = localContexts[j]->forceX;
			float* localArrayY = localContexts[j]->forceY;
			sumX += localArrayX[i];
			sumY += localArrayY[i];
			if (FLAGS & COLLECT_ZERO_THREAD_ARRAY)
			{
				localArrayX[i] = 0.0f;
				localArrayY[i] = 0.0f;
			}
		}

		if (FLAGS & COLLECT_TREE_2_GRAPH_ORDER)
		{
			__uint32 l = (globalContext->pQuadtree->refOfPoint(i));
			if (FLAGS & COLLECT_REPULSIVE_FACTOR)
			{
				// prevent some evil effects
				if (pGraph->nodeInfo(l).degree > 100)
				{
					sumX /= (float)pGraph->nodeInfo(l).degree;
					sumY /= (float)pGraph->nodeInfo(l).degree;
				}
			}
			globalArrayX[l] += sumX*factor;
			globalArrayY[l] += sumY*factor;

		}
		else
		{
			if (FLAGS & COLLECT_REPULSIVE_FACTOR)
			{
				// prevent some evil effects
				if (pGraph->nodeInfo(i).degree > 100)
				{
					sumX /= (float)pGraph->nodeInfo(i).degree;
					sumY /= (float)pGraph->nodeInfo(i).degree;
				}
			}
			globalArrayX[i] += sumX*factor;
			globalArrayY[i] += sumY*factor;
		}
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	ArrayGraph*	pGraph;
	FMEGlobalContext* globalContext;
	FMELocalContext** localContexts;
	float* globalArrayX;
	float* globalArrayY;
	__uint32 numContexts;
	float factor;
};


template<unsigned int FLAGS>
static inline CollectForceFunctor<FLAGS> collect_force_function( FMELocalContext* pLocalContext )
{
	return CollectForceFunctor<FLAGS>( pLocalContext );
}


enum {
	TIME_STEP_NORMAL = 0x1,
	TIME_STEP_PREP = 0x2,
	ZERO_GLOBAL_ARRAY = 0x4,
	USE_NODE_MOVE_RAD = 0x8
};

template<unsigned int FLAGS>
class NodeMoveFunctor
{
public:
	inline NodeMoveFunctor( FMELocalContext* pLocalContext )// float* xCoords, float* yCoords, float ftimeStep, float* globalForceArray): x(xCoords), y(yCoords), timeStep(ftimeStep), forceArray(globalForceArray)
	{
		if (FLAGS & TIME_STEP_NORMAL)
			timeStep = pLocalContext->pGlobalContext->pOptions->timeStep * pLocalContext->pGlobalContext->coolDown;
		else
		if (FLAGS & TIME_STEP_PREP)
			timeStep = pLocalContext->pGlobalContext->pOptions->preProcTimeStep;
		else
			timeStep = 1.0;
		pGraph = pLocalContext->pGlobalContext->pGraph;
		x = pGraph->nodeXPos();
		y = pGraph->nodeYPos();
		nodeMoveRadius = pGraph->nodeMoveRadius();
		forceArrayX = pLocalContext->pGlobalContext->globalForceX;
		forceArrayY = pLocalContext->pGlobalContext->globalForceY;
		localContext = pLocalContext;
		currentEdgeLength = pLocalContext->pGlobalContext->pGraph->desiredEdgeLength();
	}

/*	inline void operator()(void) const
	{
		return pGraph->numNodes();
	}; */

	inline void operator()(__uint32 i)
	{
		float d_x = forceArrayX[i]* timeStep;
		float d_y = forceArrayY[i]* timeStep;
		double dsq = (d_x*d_x + d_y*d_y);
		double d = sqrt(dsq);

		localContext->maxForceSq = max<double>(localContext->maxForceSq, (double)dsq );
		localContext->avgForce += d;
		if (d < FLT_MAX)
		{
			x[i] += (float)((d_x));
			y[i] += (float)((d_y));
			if (FLAGS & ZERO_GLOBAL_ARRAY)
			{
				forceArrayX[i] = 0.0;
				forceArrayY[i] = 0.0;
			} else
			{
				forceArrayX[i] = (float)((d_x));;
				forceArrayY[i] = (float)((d_y));;
			}
		} else
		{
			forceArrayX[i] = 0.0;
			forceArrayY[i] = 0.0;
		}
	}

	inline void operator()(__uint32 begin, __uint32 end)
	{
		for (__uint32 i = begin; i <= end; i++)
		{
			this->operator()(i);
		}
	}

private:
	float timeStep;
	float* x;
	float* y;
	float* forceArrayX;
	float* forceArrayY;
	float* nodeMoveRadius;
	float* currentEdgeLength;
	ArrayGraph* pGraph;
	FMELocalContext* localContext;
};


template<unsigned int FLAGS>
static inline NodeMoveFunctor<FLAGS> node_move_function( FMELocalContext* pLocalContext )
{
	return NodeMoveFunctor<FLAGS>( pLocalContext );
}


template<typename TYP>
inline void for_loop_array_set(__uint32 threadNr, __uint32 numThreads, TYP* a, __uint32 n, TYP value)
{
	__uint32 s = n/numThreads;
	__uint32 o = s*threadNr;
	if (threadNr == numThreads-1)
		s = s + (n % numThreads);

	for (__uint32 i = 0; i < s; i++ ) { a[o+i] = value; }
}

} // end of namespace

#endif

