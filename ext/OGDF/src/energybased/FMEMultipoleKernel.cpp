/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class FMEMultipoleKernel.
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

#include "FMEMultipoleKernel.h"
#include "ArrayGraph.h"
#include "LinearQuadtree.h"
#include "LinearQuadtreeBuilder.h"
#include "LinearQuadtreeExpansion.h"
#include "WSPD.h"
#include <algorithm>

namespace ogdf {

void FMEMultipoleKernel::quadtreeConstruction(ArrayPartition& pointPartition)
{
	FMELocalContext*  localContext	= m_pLocalContext;
	FMEGlobalContext* globalContext = m_pGlobalContext;
	LinearQuadtree&	tree			= *globalContext->pQuadtree;

	// precompute the bounding box for the quadtree points from the graph nodes
	for_loop(pointPartition, min_max_x_function(localContext));
	for_loop(pointPartition, min_max_y_function(localContext));

	// wait until the thread's bounding box is computed
	sync();

	// let the main thread computed the bounding box of the bounding boxes
	if (isMainThread())
	{
		globalContext->min_x = globalContext->pLocalContext[0]->min_x;
		globalContext->min_y = globalContext->pLocalContext[0]->min_y;
		globalContext->max_x = globalContext->pLocalContext[0]->max_x;
		globalContext->max_y = globalContext->pLocalContext[0]->max_y;
		for (__uint32 j=1; j < numThreads(); j++)
		{
			globalContext->min_x = min(globalContext->min_x, globalContext->pLocalContext[j]->min_x);
			globalContext->min_y = min(globalContext->min_y, globalContext->pLocalContext[j]->min_y);
			globalContext->max_x = max(globalContext->max_x, globalContext->pLocalContext[j]->max_x);
			globalContext->max_y = max(globalContext->max_y, globalContext->pLocalContext[j]->max_y);
		}
		tree.init(globalContext->min_x, globalContext->min_y, globalContext->max_x, globalContext->max_y);
		globalContext->coolDown *= 0.999f;
		tree.clear();
	}
	// wait because the morton number computation needs the bounding box
	sync();
	// udpate morton number to prepare them for sorting
	for_loop(pointPartition, LQMortonFunctor(localContext));
	// wait so we can sort them by morton number
	sync();

#ifdef OGDF_FME_PARALLEL_QUADTREE_SORT
	// use a simple parallel sorting algorithm
	LinearQuadtree::LQPoint* points = tree.pointArray();
	sort_parallel(points, tree.numberOfPoints(), LQPointComparer);
#else
	if (isMainThread())
	{
		LinearQuadtree::LQPoint* points = tree.pointArray();
		sort_single(points, tree.numberOfPoints(), LQPointComparer);
	}
#endif
	// wait because the quadtree builder needs the sorted order
	sync();
	// if not a parallel run, we can do the easy way
	if (isSingleThreaded())
	{
		LinearQuadtreeBuilder builder(tree);
		// prepare the tree
		builder.prepareTree();
		// and link it
		builder.build();
		LQPartitioner partitioner( localContext );
		partitioner.partition();
	} else // the more difficult part
	{
		// snap the left point of the interval of the thread to the first in the cell
		LinearQuadtree::PointID beginPoint = tree.findFirstPointInCell(pointPartition.begin);
		LinearQuadtree::PointID endPoint_plus_one;
		// if this thread is the last one, no snapping required for the right point
		if (threadNr()==numThreads()-1)
			endPoint_plus_one = tree.numberOfPoints();
		else // find the left point of the next thread
			endPoint_plus_one = tree.findFirstPointInCell(pointPartition.end+1);

		// now we can prepare the snapped interval
		LinearQuadtreeBuilder builder(tree);
		// this function prepares the tree from begin point to endPoint_plus_one-1 (EXCLUDING endPoint_plus_one)
		builder.prepareTree(beginPoint, endPoint_plus_one);
		// save the start, end and count of the inner node chain in the context
		localContext->firstInnerNode = builder.firstInner;
		localContext->lastInnerNode = builder.lastInner;
		localContext->numInnerNodes = builder.numInnerNodes;
		// save the start, end and count of the leaf node chain in the context
		localContext->firstLeaf = builder.firstLeaf;
		localContext->lastLeaf = builder.lastLeaf;
		localContext->numLeaves = builder.numLeaves;
		// wait until all are finished
		sync();

		// now the main thread has to link the tree
		if (isMainThread())
		{
			// with his own builder
			LinearQuadtreeBuilder sbuilder(tree);
			// first we need the complete chain data
			sbuilder.firstInner = globalContext->pLocalContext[0]->firstInnerNode;
			sbuilder.firstLeaf = globalContext->pLocalContext[0]->firstLeaf;
			sbuilder.numInnerNodes = globalContext->pLocalContext[0]->numInnerNodes;
			sbuilder.numLeaves = globalContext->pLocalContext[0]->numLeaves;
			for (__uint32 j=1; j < numThreads(); j++)
			{
				sbuilder.numLeaves += globalContext->pLocalContext[j]->numLeaves;
				sbuilder.numInnerNodes += globalContext->pLocalContext[j]->numInnerNodes;
			}
			sbuilder.lastInner = globalContext->pLocalContext[numThreads()-1]->lastInnerNode;
			sbuilder.lastLeaf = globalContext->pLocalContext[numThreads()-1]->lastLeaf;
			// Link the tree
			sbuilder.build();
			// and run the partitions
			LQPartitioner partitioner(localContext);
			partitioner.partition();
		}
	}
	// wait for tree to finish
	sync();
	// now update the copy of the point data
	for_loop(pointPartition, LQPointUpdateFunctor(localContext));
	// compute the nodes coordinates and sizes
	tree.forall_tree_nodes(LQCoordsFunctor(localContext), localContext->innerNodePartition.begin, localContext->innerNodePartition.numNodes)();
	tree.forall_tree_nodes(LQCoordsFunctor(localContext), localContext->leafPartition.begin, localContext->leafPartition.numNodes)();
}





void FMEMultipoleKernel::multipoleApproxSingleThreaded(ArrayPartition& nodePointPartition)
{
	FMELocalContext*  localContext	= m_pLocalContext;
	FMEGlobalContext* globalContext = m_pGlobalContext;
	LinearQuadtree&	tree			= *globalContext->pQuadtree;
	if (isMainThread())
	{
		tree.bottom_up_traversal(					// do a bottom up traversal M2M pass
			if_then_else(tree.is_leaf_condition(),	// if the current node is a leaf
				p2m_function(localContext),			// then calculate the multipole coeff. due to the points in the leaf
				m2m_function(localContext)			// else shift the coefficents of all children to center of the inner node
			)
		)(tree.root());

		tree.forall_well_separated_pairs(				// do a wspd traversal M2L direct eval
			pair_vice_versa(m2l_function(localContext)),// M2L for a well-separated pair
			p2p_function(localContext),					// direct evaluation
			p2p_function(localContext)					// direct evaluation
		)(tree.root());

		tree.top_down_traversal(						// top down traversal
			if_then_else( tree.is_leaf_condition(),		// if the node is a leaf
				do_nothing(),							// then do nothing, we will deal with this case later
				l2l_function(localContext)				// else shift the nodes local coeffs to the children
			)
		)(tree.root());// start at the root

		// evaluate all leaves and store the forces in the threads array
		for_loop(nodePointPartition,				// loop over points
			func_comp(								// composition of two statements
				l2p_function(localContext),			// evaluate the forces due to the local expansion in the corresponding leaf
				collect_force_function				// collect the forces of all threads with the following options:
				<
					COLLECT_REPULSIVE_FACTOR | 		// multiply by the repulsive factor stored in the global options
					COLLECT_TREE_2_GRAPH_ORDER |	// threads data is stored in quadtree leaf order, transform it into graph order
					COLLECT_ZERO_THREAD_ARRAY		// reset threads array
				>(localContext)
			)
		);
	}
}

//! the original algorithm which runs the WSPD completely single threaded
void FMEMultipoleKernel::multipoleApproxSingleWSPD(ArrayPartition& nodePointPartition)
{
	FMELocalContext*  localContext	= m_pLocalContext;
	FMEGlobalContext* globalContext = m_pGlobalContext;
	LinearQuadtree&	tree			= *globalContext->pQuadtree;

	// let the main thread run the WSPD
	if (isMainThread())
	{
		// The Well-separated pairs decomposition
		tree.forall_well_separated_pairs(		// do a wspd traversal
				tree.StoreWSPairFunction(),		// store the ws pairs in the WSPD
				tree.StoreDirectPairFunction(), // store the direct pairs
				tree.StoreDirectNodeFunction()	// store the direct nodes
			)(tree.root()); // start at the root
	}

	// Note: dont wait for the WSPD to finish. We dont need it yet for
	// the big multihreaded bottom up traversal.
	for_tree_partition(								// for all roots in the threads tree partition
		tree.bottom_up_traversal(					// do a bottom up traversal
			if_then_else(tree.is_leaf_condition(),	// if the current node is a leaf
				p2m_function(localContext),			// then calculate the multipole coeff. due to the points in the leaf
				m2m_function(localContext)			// else shift the coefficents of all children to center of the inner node
			)
		)
	);
	sync();
	// top of the tree has to be done by the main thread
	if (isMainThread())
	{
		tree.bottom_up_traversal(						// start a bottom up traversal
				if_then_else(tree.is_leaf_condition(),	// if the current node is a leaf
					p2m_function(localContext),			// then calculate the multipole coeff. due to the points in the leaf
					m2m_function(localContext)			// else shift the coefficents of all children to center of the inner node
				),
				not_condition(tree.is_fence_condition()) // stop when the fence to the threads is reached
			)(tree.root());// start at the root
	}
	// wait until all nodes in the quadtree have their mulipole coeff
	sync();
	// M2L pass with the WSPD
	tree.forall_tree_nodes(M2LFunctor(localContext), localContext->innerNodePartition.begin, localContext->innerNodePartition.numNodes)();
	tree.forall_tree_nodes(M2LFunctor(localContext), localContext->leafPartition.begin, localContext->leafPartition.numNodes)();
	// D2D pass and store in the thread force array
	for_loop(arrayPartition(tree.numberOfDirectPairs()), D2DFunctor(localContext));
	// D pass and store in the thread force array
	for_loop(arrayPartition(tree.numberOfDirectNodes()), NDFunctor(localContext));
	// wait until all local coeffs and all direct forces are computed
	sync();
	// big multihreaded top down traversal. top of the tree has to be done by the main thread
	if (isMainThread())
	{
		tree.top_down_traversal(					// top down traversal
			if_then_else( tree.is_leaf_condition(), // if the node is a leaf
				do_nothing(),						// then do nothing, we will deal with L2P pass later
				l2l_function(localContext)			// else shift the nodes local coeffs to the children
			),
			not_condition(tree.is_fence_condition()) //stop when the fence to the threads is reached
		)(tree.root());// start at the root
	}
	sync();
	// the rest is done by the threads
	for_tree_partition(								// for all roots in the threads tree partition
		tree.top_down_traversal(					// do a top down traversal
			if_then_else( tree.is_leaf_condition(),	// if the node is a leaf
				do_nothing(),						// then do nothing, we will deal with L2P pass later
				l2l_function(localContext)			// else shift the nodes local coeffs to the children
			)
		)
	);
	// wait until the traversal is finished and all leaves have their accumulated local coeffs
	sync();
	// evaluate all leaves and store the forces in the threads array (Note: we can store them in the global array but then we have to use random access)
	// we can start immediately to collect the forces because we evaluated before point by point
	for_loop(nodePointPartition,				// loop over threads points
		func_comp(								// composition of two statements
			l2p_function(localContext),			// evaluate the forces due to the local expansion in the corresponding leaf
			collect_force_function				// collect the forces of all threads with the following options:
			<
				COLLECT_REPULSIVE_FACTOR | 		// multiply by the repulsive factor stored in the global options
				COLLECT_TREE_2_GRAPH_ORDER |	// threads data is stored in quadtree leaf order, transform it into graph order
				COLLECT_ZERO_THREAD_ARRAY		// reset threads array to zero
			>(localContext)
		)
	);
}


//! new but slower method, parallel wspd computation without using the wspd structure
void FMEMultipoleKernel::multipoleApproxNoWSPDStructure(ArrayPartition& nodePointPartition)
{
	FMELocalContext*  localContext	= m_pLocalContext;
	FMEGlobalContext* globalContext = m_pGlobalContext;
	LinearQuadtree&	tree			= *globalContext->pQuadtree;
	// big multihreaded bottom up traversal.
	for_tree_partition(								// for all roots in the threads tree partition
		tree.bottom_up_traversal(					// do a bottom up traversal
			if_then_else(tree.is_leaf_condition(),	// if the current node is a leaf
				p2m_function(localContext),			// then calculate the multipole coeff. due to the points in the leaf
				m2m_function(localContext)			// else shift the coefficents of all children to center of the inner node
			)
		)
	);
	sync();

	// top of the tree has to be done by the main thread
	if (isMainThread())
	{
		tree.bottom_up_traversal(					// start a bottom up traversal
			if_then_else(tree.is_leaf_condition(),	// if the current node is a leaf
				p2m_function(localContext),			// then calculate the multipole coeff. due to the points in the leaf
				m2m_function(localContext)			// else shift the coefficents of all children to center of the inner node
			),
			not_condition(tree.is_fence_condition()))(tree.root());// start at the root, stop when the fence to the threads is reached

		tree.forall_well_separated_pairs(					// do a wspd traversal
			pair_vice_versa(m2l_function(localContext)),	// M2L for a well-separated pair
			p2p_function(localContext),						// direct evaluation
			p2p_function(localContext),						// direct evaluation
			not_condition(tree.is_fence_condition()))(tree.root());
	}
	// wait until all local coeffs and all direct forces are computed
	sync();

	// now a wspd traversal for the roots in the tree partition
	for_tree_partition(
		tree.forall_well_separated_pairs(					// do a wspd traversal
			pair_vice_versa(m2l_function(localContext)),	// M2L for a well-separated pair
			p2p_function(localContext),						// direct evaluation
			p2p_function(localContext)						// direct evaluation
		)
	);
	// wait until all local coeffs and all direct forces are computed
	sync();

	// big multihreaded top down traversal. Top of the tree has to be done by the main thread
	if (isMainThread())
	{
		tree.top_down_traversal(					// top down traversal
			if_then_else( tree.is_leaf_condition(), // if the node is a leaf
				do_nothing(),						// then do nothing, we will deal with this case later
				l2l_function(localContext)			// else shift the nodes local coeffs to the children
			),
			not_condition(tree.is_fence_condition()) //stop when the fence to the threads is reached
		)(tree.root());								 // start at the root
	}
	// wait until the traversal is finished and all roots of the threads have their coefficients
	sync();

	for_tree_partition(								// for all roots in the threads tree partition
		tree.top_down_traversal(					// do a top down traversal
			if_then_else( tree.is_leaf_condition(),	// if the node is a leaf
				do_nothing(),						// then do nothing, we will deal with this case later
				l2l_function(localContext)			// else shift the nodes local coeffs to the children
			)
		)
	);
	// wait until the traversal is finished and all leaves have their accumulated local coeffs
	sync();
	// evaluate all leaves and store the forces in the threads array (Note we can store them in the global array but then we have to use random access)
	// we can start immediately to collect the forces because we evaluated before point by point
	for_loop(nodePointPartition,				// loop over threads points
		func_comp(								// composition of two statements
			l2p_function(localContext),			// evaluate the forces due to the local expansion in the corresponding leaf
			collect_force_function				// collect the forces of all threads with the following options:
			<
				COLLECT_REPULSIVE_FACTOR | 		// multiply by the repulsive factor stored in the global options
				COLLECT_TREE_2_GRAPH_ORDER |	// threads data is stored in quadtree leaf order, transform it into graph order
				COLLECT_ZERO_THREAD_ARRAY		// reset threads array
			>(localContext)
		)
	);
}


//! the final approximation algorithm which runs the wspd parallel without storing it in the threads subtrees
void FMEMultipoleKernel::multipoleApproxFinal(ArrayPartition& nodePointPartition)
{
	FMELocalContext*  localContext	= m_pLocalContext;
	FMEGlobalContext* globalContext = m_pGlobalContext;
	LinearQuadtree&	tree			= *globalContext->pQuadtree;
	// big multihreaded bottom up traversal.
	for_tree_partition(								// for all roots in the threads tree partition
		tree.bottom_up_traversal(					// do a bottom up traversal
			if_then_else(tree.is_leaf_condition(),	// if the current node is a leaf
				p2m_function(localContext),			// then calculate the multipole coeff. due to the points in the leaf
				m2m_function(localContext)			// else shift the coefficents of all children to center of the inner node
			)
		)
	);
	sync();
	// top of the tree has to be done by the main thread
	if (isMainThread())
	{
		tree.bottom_up_traversal(					// start a bottom up traversal
			if_then_else(tree.is_leaf_condition(),	// if the current node is a leaf
				p2m_function(localContext),			// then calculate the multipole coeff. due to the points in the leaf
				m2m_function(localContext)			// else shift the coefficents of all children to center of the inner node
			),
			not_condition(tree.is_fence_condition()))(tree.root());// start at the root, stop when the fence to the threads is reached

		tree.forall_well_separated_pairs(	// do a wspd traversal
			tree.StoreWSPairFunction(),		// store the ws pairs in the WSPD
			tree.StoreDirectPairFunction(), // store the direct pairs
			tree.StoreDirectNodeFunction(),	// store the direct nodes
			not_condition(tree.is_fence_condition()))(tree.root());
	}
	// wait for the main thread to finish
	sync();

	// M2L pass with the WSPD for the result of the single threaded pass above
	tree.forall_tree_nodes(M2LFunctor(localContext), localContext->innerNodePartition.begin, localContext->innerNodePartition.numNodes)();
	tree.forall_tree_nodes(M2LFunctor(localContext), localContext->leafPartition.begin, localContext->leafPartition.numNodes)();

	// D2D pass and store in the thread force array
	for_loop(arrayPartition(tree.numberOfDirectPairs()), D2DFunctor(localContext));
	for_loop(arrayPartition(tree.numberOfDirectNodes()), NDFunctor(localContext));

	// wait until all local coeffs and all direct forces are computed
	sync();

	// the rest of the WSPD can be done on the fly by the thread
	for_tree_partition(
		tree.forall_well_separated_pairs(					// do a wspd traversal
			pair_vice_versa(m2l_function(localContext)),	// M2L for a well-separated pair
			p2p_function(localContext),						// direct evaluation
			p2p_function(localContext)						// direct evaluation
		)
	);
	// wait until all local coeffs and all direct forces are computed
	sync();

	// big multihreaded top down traversal. top of the tree has to be done by the main thread
	if (isMainThread())
	{
		tree.top_down_traversal(						// top down traversal L2L pass
			if_then_else( tree.is_leaf_condition(),		// if the node is a leaf
				do_nothing(),							// then do nothing, we will deal with this case later
				l2l_function(localContext)				// else shift the nodes local coeffs to the children
			),
			not_condition(tree.is_fence_condition())	// stop when the fence to the threads is reached
		)(tree.root());									// start at the root,
	}
	// wait for the top of the tree
	sync();

	for_tree_partition(								// for all roots in the threads tree partition L2L pass
		tree.top_down_traversal(					// do a top down traversal
			if_then_else( tree.is_leaf_condition(),	// if the node is a leaf
				do_nothing(),						// then do nothing, we will deal with this case later
				l2l_function(localContext)			// else shift the nodes local coeffs to the children
			)
		)
	);
	// wait until the traversal is finished and all leaves have their accumulated local coeffs
	sync();
	// evaluate all leaves and store the forces in the threads array (Note we can store them in the global array but then we have to use random access)
	// we can start immediately to collect the forces because we evaluated before point by point
	for_loop(nodePointPartition,				// loop over threads points
		func_comp(								// composition of two statements
			l2p_function(localContext),			// evaluate the forces due to the local expansion in the corresponding leaf
			collect_force_function				// collect the forces of all threads with the following options:
			<
				COLLECT_REPULSIVE_FACTOR | 		// multiply by the repulsive factor stored in the global options
				COLLECT_TREE_2_GRAPH_ORDER |	// threads data is stored in quadtree leaf order, transform it into graph order
				COLLECT_ZERO_THREAD_ARRAY		// reset threads array
			>(localContext)
		)
	);
}



void FMEMultipoleKernel::operator()(FMEGlobalContext* globalContext)
{
	__uint32					maxNumIterations    =  globalContext->pOptions->maxNumIterations;
	__uint32					minNumIterations    =  globalContext->pOptions->minNumIterations;
	ArrayGraph&					graph				= *globalContext->pGraph;
	LinearQuadtree&				tree				= *globalContext->pQuadtree;
	LinearQuadtreeExpansion&	treeExp				= *globalContext->pExpansion;
	FMELocalContext*			localContext		= globalContext->pLocalContext[threadNr()];
	FMEGlobalOptions*			options				= globalContext->pOptions;
	float*						threadsForceArrayX	= localContext->forceX;
	float*						threadsForceArrayY	= localContext->forceY;
	float*						globalForceArrayX	= globalContext->globalForceX;
	float*						globalForceArrayY	= globalContext->globalForceY;

	ArrayPartition edgePartition = arrayPartition(graph.numEdges());
	ArrayPartition nodePointPartition = arrayPartition(graph.numNodes());

	m_pLocalContext = localContext;
	m_pGlobalContext = globalContext;
	/****************************/
	/* INIT						*/
	/****************************/
	//! reset the global force array
	for_loop_array_set(threadNr(), numThreads(), globalForceArrayX, tree.numberOfPoints(), 0.0f);
	for_loop_array_set(threadNr(), numThreads(), globalForceArrayY, tree.numberOfPoints(), 0.0f);

	// reset the threads force array
	for (__uint32 i = 0; i < tree.numberOfPoints(); i++)
	{
		threadsForceArrayX[i] = 0.0f;
		threadsForceArrayY[i] = 0.0f;
	}

	__uint32 maxNumIt = options->preProcMaxNumIterations;
	for (__uint32 currNumIteration = 0; ((currNumIteration < maxNumIt) ); currNumIteration++)
	{
		// iterate over all edges and store the resulting forces in the threads array
		for_loop(edgePartition,
			edge_force_function< EDGE_FORCE_DIV_DEGREE > (localContext)	// divide the forces by degree of the node to avoid oscilation
			);
		// wait until all edges are done
		sync();
		// now collect the forces in parallel and put the sum into the global array and move the nodes accordingly
		for_loop(nodePointPartition,
			func_comp(
			collect_force_function<COLLECT_EDGE_FACTOR_PREP | COLLECT_ZERO_THREAD_ARRAY >(localContext),
			node_move_function<TIME_STEP_PREP | ZERO_GLOBAL_ARRAY>(localContext)
			)
			);
	}
	if (isMainThread())
	{
		globalContext->coolDown = 1.0f;
	}
	sync();

	for (__uint32 currNumIteration = 0; ((currNumIteration < maxNumIterations) && !globalContext->earlyExit); currNumIteration++)
	{
		// reset the coefficients
		for_loop_array_set(threadNr(), numThreads(), treeExp.m_multiExp, treeExp.m_numExp*(treeExp.m_numCoeff << 1), 0.0);
		for_loop_array_set(threadNr(), numThreads(), treeExp.m_localExp, treeExp.m_numExp*(treeExp.m_numCoeff << 1), 0.0);

		localContext->maxForceSq = 0.0;
		localContext->avgForce = 0.0;

		// construct the quadtree
		quadtreeConstruction(nodePointPartition);
		// wait for all threads to finish
		sync();

		if (isSingleThreaded()) // if is single threaded run the simple approximation
			multipoleApproxSingleThreaded(nodePointPartition);
		else // otherwise use the partitioning
			multipoleApproxFinal(nodePointPartition);
		// now wait until all forces are summed up in the global array and mapped to graph node order
		sync();

		// run the edge forces
		for_loop(edgePartition,							// iterate over all edges and sum up the forces in the threads array
			edge_force_function< EDGE_FORCE_DIV_DEGREE >(localContext)	// divide the forces by degree of the node to avoid oscilation
		);
		// wait until edges are finished
		sync();

		// collect the edge forces and move nodes without waiting
		for_loop(nodePointPartition,
			func_comp(
				 collect_force_function<COLLECT_EDGE_FACTOR | COLLECT_ZERO_THREAD_ARRAY>(localContext),
				 node_move_function<TIME_STEP_NORMAL | ZERO_GLOBAL_ARRAY>(localContext)
			)
		);
		// wait so we can decide if we need another iteration
		sync();
		// check the max force square for all threads
		if (isMainThread())
		{
			double maxForceSq = 0.0;
			for (__uint32 j=0; j < numThreads(); j++)
				maxForceSq = max(globalContext->pLocalContext[j]->maxForceSq, maxForceSq);

			// if we are allowed to quit and the max force sq falls under the threshold tell all threads we are done
			if ((currNumIteration >= minNumIterations) && (maxForceSq < globalContext->pOptions->stopCritForce ))
			{
				globalContext->earlyExit = true;
			}
		}
		// this is required to wait for the earlyExit result
		sync();
	}
}

//! allcates the global context and for all threads a local context
FMEGlobalContext* FMEMultipoleKernel::allocateContext(ArrayGraph* pGraph, FMEGlobalOptions* pOptions, __uint32 numThreads)
{
	FMEGlobalContext* globalContext = new FMEGlobalContext();

	globalContext->numThreads = numThreads;
	globalContext->pOptions = pOptions;
	globalContext->pGraph = pGraph;
	globalContext->pQuadtree = new LinearQuadtree(pGraph->numNodes(), pGraph->nodeXPos(), pGraph->nodeYPos(), pGraph->nodeSize());
	globalContext->pWSPD = globalContext->pQuadtree->wspd();
	globalContext->pExpansion = new LinearQuadtreeExpansion(globalContext->pOptions->multipolePrecision, (*globalContext->pQuadtree));
	__uint32 numPoints = globalContext->pQuadtree->numberOfPoints();
	typedef FMELocalContext* FMELocalContextPtr;

	globalContext->pLocalContext = new FMELocalContextPtr[numThreads];
	globalContext->globalForceX = (float*)MALLOC_16(sizeof(float)*numPoints);
	globalContext->globalForceY = (float*)MALLOC_16(sizeof(float)*numPoints);
	for (__uint32 i=0; i < numThreads; i++)
	{
		globalContext->pLocalContext[i] = new FMELocalContext;
		globalContext->pLocalContext[i]->forceX = (float*)MALLOC_16(sizeof(float)*numPoints);
		globalContext->pLocalContext[i]->forceY = (float*)MALLOC_16(sizeof(float)*numPoints);
		globalContext->pLocalContext[i]->pGlobalContext = globalContext;
	}
	return globalContext;
}

//! frees the memory for all contexts
void FMEMultipoleKernel::deallocateContext(FMEGlobalContext* globalContext)
{
	__uint32 numThreads = globalContext->numThreads;
	for (__uint32 i=0; i < numThreads; i++)
	{
		FREE_16(globalContext->pLocalContext[i]->forceX);
		FREE_16(globalContext->pLocalContext[i]->forceY);
		delete globalContext->pLocalContext[i];
	}
	FREE_16(globalContext->globalForceX);
	FREE_16(globalContext->globalForceY);
	delete[] globalContext->pLocalContext;
	delete globalContext->pExpansion;
	delete globalContext->pQuadtree;
	delete globalContext;
}

}
