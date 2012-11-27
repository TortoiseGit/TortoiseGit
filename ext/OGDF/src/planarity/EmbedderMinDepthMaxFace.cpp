/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Computes an embedding of a graph with minimum depth and
 * maximum external face. See paper "Graph Embedding with Minimum
 * Depth and Maximum External Face" by C. Gutwenger and P. Mutzel
 * (2004) for details.
 *
 * \author Thorsten Kerkhof
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

#include <ogdf/planarity/EmbedderMinDepthMaxFace.h>
#include <ogdf/internal/planarity/ConnectedSubgraph.h>
#include <ogdf/internal/planarity/EmbedderMaxFaceBiconnectedGraphs.h>

namespace ogdf {

void EmbedderMinDepthMaxFace::call(Graph& G, adjEntry& adjExternal)
{
	edge e;
	node n;
	int maxint = 2147483647;

	adjExternal = 0;
	pAdjExternal = &adjExternal;

	//simple base cases:
	if (G.numberOfNodes() <= 1)
		return;
	if (G.numberOfEdges() == 1)
	{
		edge e = G.firstEdge();
		adjExternal = e->adjSource();
		return;
	}

	//HINT: Edges are directed from child to parent in BC-trees
	pBCTree = new BCTree(G);

	//base case of biconnected graph:
	if (pBCTree->bcTree().numberOfNodes() == 1)
	{
		NodeArray<int> m_nodeLength(G, 0);
		EdgeArray<int> m_edgeLength(G, 1);
		adjEntry m_adjExternal;
		EmbedderMaxFaceBiconnectedGraphs<int>::embed(G,
			m_adjExternal,
			m_nodeLength,
			m_edgeLength);
		adjExternal = m_adjExternal->twin();

		delete pBCTree;
		return;
	}


	//============================================================================
	// First step: calculate min depth and node lengths
	//============================================================================
	//Find root Block (only node with out-degree of 0):
	node rootBlockNode;
	forall_nodes(n, pBCTree->bcTree())
	{
		if (n->outdeg() == 0)
		{
			rootBlockNode = n;
			break;
		}
	}

	/****************************************************************************/
	/* MIN DEPTH                                                                */
	/****************************************************************************/
	//Node lengths of block graph:
	md_nodeLength.init(pBCTree->auxiliaryGraph(), 0);

	//Edge lengths of BC-tree, values m_{c, B} for all (c, B) \in bcTree:
	md_m_cB.init(pBCTree->bcTree(), 0);

	//Bottom-up traversal: (set m_cB for all {c, B} \in bcTree)
	forall_adj_edges(e, rootBlockNode)
	{
		node cT = e->source();
		//node cH = pBCTree->cutVertex(cT, rootBlockNode);

		//set length of c in block graph of root block node:
		edge e2;
		forall_adj_edges(e2, cT)
		{
			if (e2->target() != cT)
				continue;

			node blockNode = e2->source();
			node cutVertex = pBCTree->cutVertex(cT, blockNode);

			//Start recursion:
			md_m_cB[e2] = md_bottomUpTraversal(blockNode, cutVertex);
		}
	}

	//Top-down traversal: (set m_cB for all {B, c} \in bcTree and get min depth
	//for each block)
	md_nodeLength.fill(0);
	md_minDepth.init(pBCTree->bcTree(), maxint);
	md_M_B.init(pBCTree->bcTree());
	md_M2.init(pBCTree->bcTree());
	md_topDownTraversal(rootBlockNode);

	/****************************************************************************/
	/* MAX FACE                                                                 */
	/****************************************************************************/
	mf_cstrLength.init(pBCTree->auxiliaryGraph(), 0);
	mf_nodeLength.init(pBCTree->auxiliaryGraph(), 0);
	mf_maxFaceSize.init(pBCTree->bcTree(), 0);

	//Bottom-Up-Traversal:
	{
		forall_adj_edges(e, rootBlockNode)
		{
			node cT = e->source();
			node cH = pBCTree->cutVertex(cT, rootBlockNode);

			//set length of v in block graph of root block node:
			int length_v_in_rootBlock = 0;
			edge e2;
			forall_adj_edges(e2, cT)
			{
				//check if edge is an incoming edge:
				if (e2->target() != cT)
					continue;

				node blockNode = e2->source();
				node cutVertex = pBCTree->cutVertex(cT, blockNode);
				length_v_in_rootBlock += mf_constraintMaxFace(blockNode, cutVertex);
			}
			mf_nodeLength[cH] = length_v_in_rootBlock;
		}
	}

	node mf_bT_opt = G.chooseNode(); //= G.chooseNode() only to get rid of warning
	int mf_ell_opt = 0;
	mf_maximumFaceRec(rootBlockNode, mf_bT_opt, mf_ell_opt);

	/****************************************************************************/
	/* MIN DEPTH + MAX FACE                                                     */
	/****************************************************************************/
	//compute bT_opt:
	mdmf_edgeLength.init(pBCTree->auxiliaryGraph(), MDMFLengthAttribute(0, 1));
	mdmf_nodeLength.init(pBCTree->auxiliaryGraph(), MDMFLengthAttribute(0, 0));
	int d_opt = maxint;
	int ell_opt = -1;
	node bT_opt;
	node bT;
	forall_nodes(bT, pBCTree->bcTree())
	{
		if (pBCTree->typeOfBNode(bT) != BCTree::BComp)
			continue;
		if (   md_minDepth[bT] < d_opt
			|| (md_minDepth[bT] == d_opt && mf_maxFaceSize[bT] > ell_opt))
		{
			d_opt = md_minDepth[bT];
			ell_opt = mf_maxFaceSize[bT];
			bT_opt = bT;
		}
	}

	//============================================================================
	// Second step: Embed G by expanding a maximum face in bT_opt
	//============================================================================
	newOrder.init(G);
	treeNodeTreated.init(pBCTree->bcTree(), false);
	//reset md_nodeLength and set them during embedBlock call, because they are
	//calculated for starting embedding with rootBlockNode, which is not
	//guarenteed
	md_nodeLength.fill(0);
	embedBlock(bT_opt);

	forall_nodes(n, G)
		G.sort(n, newOrder[n]);

	delete pBCTree;
}


int EmbedderMinDepthMaxFace::md_bottomUpTraversal(const node& bT, const node& cH)
{
	int m_B = 0; //max_{c \in B} m_B(c)
	List<node> M_B; //{c \in B | m_B(c) = m_B}

	//Recursion:
	edge e;
	forall_adj_edges(e, bT)
	{
		if (e->target() != bT)
			continue;
		node cT = e->source();
		//node c_in_bT = pBCTree->cutVertex(cT, bT);

		//set length of c in block graph of root block node:
		edge e_cT_bT2;
		forall_adj_edges(e_cT_bT2, cT)
		{
			if (e == e_cT_bT2)
				continue;

			node bT2 = e_cT_bT2->source();
			node c_in_bT2 = pBCTree->cutVertex(cT, bT2);
			md_m_cB[e_cT_bT2] = md_bottomUpTraversal(bT2, c_in_bT2);

			//update m_B and M_B:
			if (m_B < md_m_cB[e_cT_bT2])
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				m_B = md_m_cB[e_cT_bT2];
				M_B.clear();
				M_B.pushBack(cV_in_bT);
			}
			else if ( m_B == md_m_cB[e_cT_bT2] && M_B.search(pBCTree->cutVertex(cT, bT)) == -1)
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				M_B.pushBack(cV_in_bT);
			}
		}
	}

	//set vertex length for all vertices in bH to 1 if vertex is in M_B:
	for (ListIterator<node> iterator = M_B.begin(); iterator.valid(); iterator++)
		md_nodeLength[*iterator] = 1;

	//generate block graph of bT:
	Graph blockGraph_bT;
	node cInBlockGraph_bT;
	NodeArray<int> nodeLengthSG(blockGraph_bT);
	ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph_bT, cH,
		cInBlockGraph_bT, md_nodeLength, nodeLengthSG);

	//leafs of BC-tree:
	if (M_B.size() == 0)
		return 1;

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> edgeLength(blockGraph_bT, 0);

	//compute maximum external face of block graph and get its size:
	int cstrLength_B_c
		= EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
			blockGraph_bT, cInBlockGraph_bT, nodeLengthSG, edgeLength);

	if (cstrLength_B_c == M_B.size())
		return m_B;
	//else:
	return m_B + 2;
}


void EmbedderMinDepthMaxFace::md_topDownTraversal(const node& bT)
{
	//m_B(c) = max {0} \cup {m_{c, B'} | c \in B', B' \neq B}
	int m_B = 0; //max_{c \in B} m_B(c)

	//Compute m_B and M_B:
	node cT_parent = 0;
	edge e_bT_cT;
	{
		forall_adj_edges(e_bT_cT, bT)
		{
			if (e_bT_cT->source() == bT)
				cT_parent = e_bT_cT->target();
			node cT = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
			edge e_cT_bT2;
			forall_adj_edges(e_cT_bT2, cT)
			{
				if (e_cT_bT2 == e_bT_cT)
					continue;

				//update m_B and M_B:
				if (m_B < md_m_cB[e_cT_bT2])
				{
					m_B = md_m_cB[e_cT_bT2];
					md_M_B[bT].clear();
					md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
				else if ( m_B == md_m_cB[e_cT_bT2] && md_M_B[bT].search(pBCTree->cutVertex(cT, bT)) == -1)
				{
					md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
			}//forall_adj_edges(e_cT_bT2, cT)
		}//forall_adj_edges(e_bT_cT, bT)
	}
	//set vertex length for all vertices in bH to 1 if vertex is in M_B:
	NodeArray<int> m_nodeLength(pBCTree->auxiliaryGraph(), 0);
	for (ListIterator<node> iterator = md_M_B[bT].begin(); iterator.valid(); iterator++)
	{
		md_nodeLength[*iterator] = 1;
		m_nodeLength[*iterator] = 1;
	}

	//generate block graph of bT:
	Graph blockGraph_bT;
	NodeArray<int> nodeLengthSG(blockGraph_bT);
	NodeArray<node> nG_to_nSG;
	ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph_bT,
															 (*(pBCTree->hEdges(bT).begin()))->source(),
															 m_nodeLength, nodeLengthSG, nG_to_nSG);

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> edgeLengthBlock(blockGraph_bT, 0);

	//compute size of a maximum external face of block graph:
	StaticSPQRTree* spqrTree = 0;
	if (   !blockGraph_bT.empty()
		&& blockGraph_bT.numberOfNodes() != 1
		&& blockGraph_bT.numberOfEdges() > 2)
	{
		spqrTree = new StaticSPQRTree(blockGraph_bT);
	}
	NodeArray< EdgeArray<int> > edgeLengthSkel;
	int cstrLength_B_c = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockGraph_bT, nodeLengthSG, edgeLengthBlock, *spqrTree, edgeLengthSkel);

	//Prepare recursion by setting m_{c, B} for all edges {B, c} \in bcTree:
	if (md_M_B[bT].size() > 0)
	{
		node cT1 = pBCTree->bcproper(pBCTree->original(*(md_M_B[bT].begin())));
		bool calculateNewNodeLengths;
		if (md_M_B[bT].size() == 1 && cT1 == cT_parent)
			calculateNewNodeLengths = true;
		else
			calculateNewNodeLengths = false;
		forall_adj_edges(e_bT_cT, bT)
		{
			if (e_bT_cT->target() != bT)
				continue;
			node cT = e_bT_cT->source();
			node cH = pBCTree->cutVertex(cT, bT);

			if (md_M_B[bT].size() == 1 && cT1 == cT)
			{
				//Compute new vertex lengths according to
				//m2 = max_{v \in V_B, v != c} m_B(v) and
				//M2 = {c \in V_B \ {v} | m_B(c) = m2}.
				int m2 = 0;

				//Compute m2 and M2:
				edge e_bT_cT2;
				forall_adj_edges(e_bT_cT2, bT)
				{
					node cT2 = (e_bT_cT2->source() == bT) ? e_bT_cT2->target() : e_bT_cT2->source();
					if (cT1 == cT2)
						continue;
					edge e_cT2_bT2;
					forall_adj_edges(e_cT2_bT2, cT2)
					{
						if (e_cT2_bT2 == e_bT_cT2)
							continue;

						//update m_B and M_B:
						if (m2 < md_m_cB[e_cT2_bT2])
						{
							m2 = md_m_cB[e_cT2_bT2];
							md_M2[bT].clear();
							md_M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
						else if ( m2 == md_m_cB[e_cT2_bT2] && md_M2[bT].search(pBCTree->cutVertex(cT2, bT)) == -1)
						{
							md_M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
					}//forall_adj_edges(e_cT2_bT2, cT2)
				}//forall_adj_edges(e_bT_cT2, bT)

				//set vertex length for all vertices in bH to 1 if vertex is in M2 and
				//0 otherwise:
				md_nodeLength[*(md_M_B[bT].begin())] = 0;
				for (ListIterator<node> iterator = md_M2[bT].begin(); iterator.valid(); iterator++)
					md_nodeLength[*iterator] = 1;

				Graph blockGraph_bT;
				node cInBlockGraph_bT;
				NodeArray<int> nodeLengthSG(blockGraph_bT);
				ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph_bT, cH,
					cInBlockGraph_bT, md_nodeLength, nodeLengthSG);

				//set edge length for all edges in block graph to 0:
				EdgeArray<int> edgeLength(blockGraph_bT, 0);

				//compute a maximum external face size of a face containing c in block graph:
				int maxFaceSize = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockGraph_bT,
					cInBlockGraph_bT,
					nodeLengthSG,
					edgeLength);
				if (md_M2[bT].size() == 0)
					md_m_cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSize == md_M2[bT].size())
						md_m_cB[e_bT_cT] = m2;
					else
						md_m_cB[e_bT_cT] = m2 + 2;
				}

				if (calculateNewNodeLengths)
					calculateNewNodeLengths = false;
				else
				{
					//reset node lengths:
					for (ListIterator<node> iterator = md_M2[bT].begin(); iterator.valid(); iterator++)
						md_nodeLength[*iterator] = 0;
					md_nodeLength[*(md_M_B[bT].begin())] = 1;
				}
			}
			else //M_B.size() != 1
			{
				//compute a maximum external face size of a face containing c in block graph:
				node cInBlockGraph_bT = nG_to_nSG[cH];
				int maxFaceSize = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockGraph_bT,
					cInBlockGraph_bT,
					nodeLengthSG,
					edgeLengthBlock,
					*spqrTree,
					edgeLengthSkel);
				if (md_M_B[bT].size() == 0)
					md_m_cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSize == md_M_B[bT].size())
						md_m_cB[e_bT_cT] = m_B;
					else
						md_m_cB[e_bT_cT] = m_B + 2;
				}
			}
		}//forall_adj_edges(e_bT_cT, bT)

		if (calculateNewNodeLengths)
		{
			//Compute new vertex lengths according to
			//m2 = max_{v \in V_B, v != c} m_B(v) and
			//M2 = {c \in V_B \ {v} | m_B(c) = m2}.
			int m2 = 0;

			//Compute m2 and M2:
			edge e_bT_cT2;
			forall_adj_edges(e_bT_cT2, bT)
			{
				node cT2 = (e_bT_cT2->source() == bT) ? e_bT_cT2->target() : e_bT_cT2->source();
				if (cT1 == cT2)
					continue;
				edge e_cT2_bT2;
				forall_adj_edges(e_cT2_bT2, cT2)
				{
					if (e_cT2_bT2 == e_bT_cT2)
						continue;

					//update m_B and M_B:
					if (m2 < md_m_cB[e_cT2_bT2])
					{
						m2 = md_m_cB[e_cT2_bT2];
						md_M2[bT].clear();
						md_M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if ( m2 == md_m_cB[e_cT2_bT2] && md_M2[bT].search(pBCTree->cutVertex(cT2, bT)) == -1)
					{
						md_M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
				}//forall_adj_edges(e_cT2_bT2, cT2)
			}//forall_adj_edges(e_bT_cT2, bT)

			//set vertex length for all vertices in bH to 1 if vertex is in M2 and
			//0 otherwise:
			md_nodeLength[*(md_M_B[bT].begin())] = 0;
			for (ListIterator<node> iterator = md_M2[bT].begin(); iterator.valid(); iterator++)
				md_nodeLength[*iterator] = 1;
		} //if (calculateNewNodeLengths
		else if (md_M_B[bT].size() == 1)
		{
			//Compute M2 = {c \in V_B \ {v} | m_B(c) = m2} with
			//m2 = max_{v \in V_B, v != c} m_B(v).
			int m2 = 0;
			edge e_bT_cT2;
			forall_adj_edges(e_bT_cT2, bT)
			{
				node cT2 = (e_bT_cT2->source() == bT) ? e_bT_cT2->target() : e_bT_cT2->source();
				if (cT1 == cT2)
					continue;
				edge e_cT2_bT2;
				forall_adj_edges(e_cT2_bT2, cT2)
				{
					if (e_cT2_bT2 == e_bT_cT2)
						continue;

					//update m_B and M_B:
					if (m2 < md_m_cB[e_cT2_bT2])
					{
						m2 = md_m_cB[e_cT2_bT2];
						md_M2[bT].clear();
						md_M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if ( m2 == md_m_cB[e_cT2_bT2] && md_M2[bT].search(pBCTree->cutVertex(cT2, bT)) == -1)
					{
						md_M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
				}//forall_adj_edges(e_cT2_bT2, cT2)
			}//forall_adj_edges(e_bT_cT2, bT)
		}
	}

	//Recursion:
	forall_adj_edges(e_bT_cT, bT)
	{
		if (e_bT_cT->target() != bT)
			continue;

		node cT = e_bT_cT->source();
		edge e_cT_bT2;
		forall_adj_edges(e_cT_bT2, cT)
		{
			if (e_cT_bT2 == e_bT_cT)
				continue;

			md_topDownTraversal(e_cT_bT2->source());
		}
	}

	//Compute M_B and M2 for embedBlock-function:
	{
		md_M_B[bT].clear();
		md_M2[bT].clear();
		m_B = 0;
		int m2 = 0;
		forall_adj_edges(e_bT_cT, bT)
		{
			node cT = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
			edge e_cT_bT2;
			forall_adj_edges(e_cT_bT2, cT)
			{
				if (e_bT_cT == e_cT_bT2)
					continue;

				//update m_B and M_B:
				if (m_B < md_m_cB[e_cT_bT2])
				{
					m_B = md_m_cB[e_cT_bT2];
					md_M_B[bT].clear();
					md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
				else if ( m_B == md_m_cB[e_cT_bT2] && md_M_B[bT].search(pBCTree->cutVertex(cT, bT)) == -1)
				{
					md_M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
			}//forall_adj_edges(e_cT_bT2, cT)
		}//forall_adj_edges(e_bT_cT, bT)

		if (md_M_B[bT].size() == 1)
		{
			node cT1 = pBCTree->bcproper(pBCTree->original(*(md_M_B[bT].begin())));
			forall_adj_edges(e_bT_cT, bT)
			{
				node cT2 = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
				if (cT1 == cT2)
					continue;
				node cT = (e_bT_cT->source() == bT) ? e_bT_cT->target() : e_bT_cT->source();
				edge e_cT_bT2;
				forall_adj_edges(e_cT_bT2, cT)
				{
					//update m2 and M2:
					if (m2 < md_m_cB[e_cT_bT2])
					{
						m2 = md_m_cB[e_cT_bT2];
						md_M2[bT].clear();
						md_M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
					else if (   m2 == md_m_cB[e_cT_bT2]
									 && md_M2[bT].search(pBCTree->cutVertex(cT, bT)) == -1)
					{
						md_M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
				}//forall_adj_edges(e_cT_bT2, cT)
			}//forall_adj_edges(e_bT_cT, bT)
		}
	}

	if (cstrLength_B_c == md_M_B[bT].size())
		md_minDepth[bT] = m_B;
	else
		md_minDepth[bT] = m_B + 2;

	delete spqrTree;
}


int EmbedderMinDepthMaxFace::mf_constraintMaxFace(const node& bT, const node& cH)
{
	//forall (v \in B, v \neq c) do:
	//  length_B(v) := \sum_{(v, B') \in B} ConstraintMaxFace(B', v);
	edge e;
	forall_adj_edges(e, bT)
	{
		if (e->target() != bT)
			continue;
		node vT = e->source();
		node vH = pBCTree->cutVertex(vT, bT);

		//set length of vertex v in block graph of bT:
		int length_v_in_block = 0;
		edge e2;
		forall_adj_edges(e2, vT)
		{
			//check if edge is an incoming edge:
			if (e2->target() != vT)
				continue;

			node bT2 = e2->source();
			node cutVertex = pBCTree->cutVertex(vT, bT2);
			length_v_in_block += mf_constraintMaxFace(bT2, cutVertex);
		}
		mf_nodeLength[vH] = length_v_in_block;
	}

	mf_nodeLength[cH] = 0;
	Graph blockGraph;
	node cInBlockGraph;
	NodeArray<int> nodeLengthSG(blockGraph);
	ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph, cH, cInBlockGraph,
		mf_nodeLength, nodeLengthSG);
	EdgeArray<int> edgeLengthSG(blockGraph, 1);
	int cstrLengthBc = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockGraph, cInBlockGraph, nodeLengthSG, edgeLengthSG);
	mf_cstrLength[cH] = cstrLengthBc;
	return cstrLengthBc;
}


void EmbedderMinDepthMaxFace::mf_maximumFaceRec(const node& bT, node& bT_opt, int& ell_opt)
{
	//(B*, \ell*) := (B, size of a maximum face in B):
	node m_bT_opt = bT;
	Graph blockGraph_bT;
	NodeArray<int> nodeLengthSG(blockGraph_bT);
	NodeArray<node> nG_to_nSG;
	ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockGraph_bT,
		(*(pBCTree->hEdges(bT).begin()))->source(), mf_nodeLength, nodeLengthSG, nG_to_nSG);
	EdgeArray<int> edgeLengthSG(blockGraph_bT, 1);
	StaticSPQRTree* spqrTree = 0;
	if ( !blockGraph_bT.empty()
		&& blockGraph_bT.numberOfNodes() != 1
		&& blockGraph_bT.numberOfEdges() > 2)
	{
		spqrTree = new StaticSPQRTree(blockGraph_bT);
	}
	NodeArray< EdgeArray<int> > edgeLengthSkel;
	int m_ell_opt = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockGraph_bT, nodeLengthSG, edgeLengthSG, *spqrTree, edgeLengthSkel);
	mf_maxFaceSize[bT] = m_ell_opt;

	edge e;
	forall_adj_edges(e, bT)
	{
		if (e->target() != bT)
			continue;
		node cT = e->source();
		node cH = pBCTree->cutVertex(cT, bT);

		//cstrLengthBc := size of a maximum face in B containing c:
		node cInBlockGraph_bT = nG_to_nSG[cH];
		mf_cstrLength[cH]
			= EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
				blockGraph_bT, cInBlockGraph_bT, nodeLengthSG, edgeLengthSG, *spqrTree, edgeLengthSkel);

		//L := \sum_{(B', c) \in bcTree} cstrLength(B', c)
		int L = 0;
		edge e2;
		{
			forall_adj_edges(e2, cT)
			{
				//check if edge is an incoming edge:
				if (e2->source() != cT)
					continue;

				//get partner vertex of c in the block graph of B'=e->target() and add
				//cstrLength(B', c) to L:
				L += mf_cstrLength[pBCTree->cutVertex(cT, e2->target())];
			}
		}

		forall_adj_edges(e2, cT)
		{
			//check if edge is an outgoing edge or the edge from bT to cT:
			if (e2->target() != cT || e2->source() == bT)
				continue;

			//get partner vertex of c in the block graph of B'=e->source():
			node partnerV = pBCTree->cutVertex(cT, e2->source());
			mf_nodeLength[partnerV] = L - mf_cstrLength[partnerV];

			//pBCTree->originalGraph().chooseNode() just to get rid of warning:
			node thisbT_opt = pBCTree->originalGraph().chooseNode();
			int thisell_opt = 0;
			mf_maximumFaceRec(e2->source(), thisbT_opt, thisell_opt);
			if (thisell_opt > m_ell_opt)
			{
				m_bT_opt = thisbT_opt;
				m_ell_opt = thisell_opt;
			}
		}
	}

	//return (B*, \ell*):
	bT_opt = m_bT_opt;
	ell_opt = m_ell_opt;

	//if (   !blockGraph_bT.empty()
	//    && blockGraph_bT.numberOfNodes() != 1
	//    && blockGraph_bT.numberOfEdges() != 1)
	//{
		delete spqrTree;
	//}
}


void EmbedderMinDepthMaxFace::embedBlock(const node& bT)
{
	ListIterator<adjEntry> after;
	node cT = 0;
	embedBlock(bT, cT, after);
}


void EmbedderMinDepthMaxFace::embedBlock(
	const node& bT,
	const node& cT,
	ListIterator<adjEntry>& after)
{
	treeNodeTreated[bT] = true;
	node nSG;
	node cH = 0;
	if (!(cT == 0))
		cH = pBCTree->cutVertex(cT, bT);

	//***************************************************************************
	// 1. Compute MinDepth node lengths depending on M_B, M2 and cT
	//***************************************************************************
	if (!(cT == 0) && md_M_B[bT].size() == 1 && *(md_M_B[bT].begin()) == cH)
	{
		//set node length to 1 if node is in M2 and 0 otherwise
		for (ListIterator<node> iterator = md_M2[bT].begin(); iterator.valid(); iterator++)
			md_nodeLength[*iterator] = 1;
	}
	else
	{
		//set node length to 1 if node is in M_B and 0 otherwise
		for (ListIterator<node> iterator = md_M_B[bT].begin(); iterator.valid(); iterator++)
			md_nodeLength[*iterator] = 1;
	}

	//***************************************************************************
	// 2. Set MinDepthMaxFace node lengths
	//***************************************************************************
	//create subgraph (block bT):
	node nodeInBlock = cH;
	if (nodeInBlock == 0)
		nodeInBlock = (*(pBCTree->hEdges(bT).begin()))->source();
	Graph SG;
	NodeArray<MDMFLengthAttribute> nodeLengthSG;
	EdgeArray<MDMFLengthAttribute> edgeLengthSG;
	NodeArray<node> nSG_to_nG;
	EdgeArray<edge> eSG_to_eG;
	node nodeInBlockSG;
	ConnectedSubgraph<MDMFLengthAttribute>::call(
		pBCTree->auxiliaryGraph(), SG,
		nodeInBlock, nodeInBlockSG,
		nSG_to_nG, eSG_to_eG,
		mdmf_nodeLength, nodeLengthSG,
		mdmf_edgeLength, edgeLengthSG);

	//copy (0, 1)-min depth node lengths into nodeLengthSG d component and max
	//face sice node lengths into l component:
	forall_nodes(nSG, SG)
	{
		nodeLengthSG[nSG].d = md_nodeLength[nSG_to_nG[nSG]];
		nodeLengthSG[nSG].l = mf_nodeLength[nSG_to_nG[nSG]];
	}

	//***************************************************************************
	// 3. Compute embedding of block
	//***************************************************************************
	adjEntry m_adjExternal = 0;
	if (cH == 0)
		EmbedderMaxFaceBiconnectedGraphs<MDMFLengthAttribute>::embed(
			SG, m_adjExternal, nodeLengthSG, edgeLengthSG);
	else
		EmbedderMaxFaceBiconnectedGraphs<MDMFLengthAttribute>::embed(
			SG, m_adjExternal, nodeLengthSG, edgeLengthSG, nodeInBlockSG);

	//***************************************************************************
	// 4. Copy block embedding into graph embedding and call recursively
	//    embedBlock for all cut vertices in bT
	//***************************************************************************
	CombinatorialEmbedding CE(SG);
	face f = CE.leftFace(m_adjExternal);

	if (*pAdjExternal == 0)
	{
		node on = pBCTree->original(nSG_to_nG[m_adjExternal->theNode()]);
		adjEntry ae1 = on->firstAdj();
		for (adjEntry ae = ae1; ae; ae = ae->succ())
		{
			if (ae->theEdge() == pBCTree->original(eSG_to_eG[m_adjExternal->theEdge()]))
			{
				*pAdjExternal = ae->twin();
				break;
			}
		}
	}

	forall_nodes(nSG, SG)
	{
		node nH = nSG_to_nG[nSG];
		node nG = pBCTree->original(nH);
		adjEntry ae = nSG->firstAdj();
		ListIterator<adjEntry>* pAfter;
		if (pBCTree->bcproper(nG) == cT)
			pAfter = &after;
		else
			pAfter = new ListIterator<adjEntry>();

		if (pBCTree->typeOfGNode(nG) == BCTree::CutVertex)
		{
			node cT2 = pBCTree->bcproper(nG);
			bool no_recursion = false;
			if (cT2 == cT)
			{
				node parent_bT_of_cT2;
				edge e_cT2_to_bT2;
				forall_adj_edges(e_cT2_to_bT2, cT2)
				{
					if (e_cT2_to_bT2->source() == cT2)
					{
						parent_bT_of_cT2 = e_cT2_to_bT2->target();
						break;
					}
				}
				if (treeNodeTreated[parent_bT_of_cT2])
					no_recursion = true;
			}

			if (no_recursion)
			{
				//find adjacency entry of nSG which lies on external face f:
				adjEntry aeFace = f->firstAdj();
				do
				{
					if (aeFace->theNode() == nSG)
					{
						if (aeFace->succ())
							ae = aeFace->succ();
						else
							ae = nSG->firstAdj();
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != f->firstAdj());
			}
			else //!no_recursion
			{
				//(if exists) find adjacency entry of nSG which lies on external face f:
				adjEntry aeFace = f->firstAdj();
				do
				{
					if (aeFace->theNode() == nSG)
					{
						if (aeFace->succ())
							ae = aeFace->succ();
						else
							ae = nSG->firstAdj();
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != f->firstAdj());

				edge e_cT2_to_bT2;
				forall_adj_edges(e_cT2_to_bT2, cT2)
				{
					node bT2;
					if (e_cT2_to_bT2->source() == cT2)
						bT2 = e_cT2_to_bT2->target();
					else
						bT2 = e_cT2_to_bT2->source();
					if (!treeNodeTreated[bT2])
						embedBlock(bT2, cT2, *pAfter);
				}
			}
		}

		//embed all edges of block bT:
		bool after_ae = true;
		for (adjEntry aeNode = ae;
			after_ae || aeNode != ae;
			after_ae = (!after_ae || !aeNode->succ()) ? false : true,
			aeNode = aeNode->succ() ? aeNode->succ() : nSG->firstAdj())
		{
			edge eG = pBCTree->original(eSG_to_eG[aeNode->theEdge()]);
			if (nG == eG->source())
			{
				if (!pAfter->valid())
					*pAfter = newOrder[nG].pushBack(eG->adjSource());
				else
					*pAfter = newOrder[nG].insertAfter(eG->adjSource(), *pAfter);
			}
			else //!(nG == eG->source())
			{
				if (!pAfter->valid())
					*pAfter = newOrder[nG].pushBack(eG->adjTarget());
				else
					*pAfter = newOrder[nG].insertAfter(eG->adjTarget(), *pAfter);
			}
		} //for (adjEntry aeNode = ae; aeNode; aeNode = aeNode->succ())

		if (!(*pAfter == after))
			delete pAfter;
	} //forall_nodes(nSG, SG)
}

} // end namespace ogdf
