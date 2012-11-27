/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Computes an embedding of a graph with minimum depth.
 * See paper "Graph Embedding with Minimum Depth and Maximum External
 * Face" by C. Gutwenger and P. Mutzel (2004) for details.
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

#include <ogdf/planarity/EmbedderMinDepth.h>
#include <ogdf/internal/planarity/EmbedderMaxFaceBiconnectedGraphs.h>
#include <ogdf/internal/planarity/ConnectedSubgraph.h>

namespace ogdf {

void EmbedderMinDepth::call(Graph& G, adjEntry& adjExternal)
{
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
		EdgeArray<int> m_edgeLength(G, 0);
		adjEntry m_adjExternal;
		EmbedderMaxFaceBiconnectedGraphs<int>::embed(G, m_adjExternal, m_nodeLength, m_edgeLength);
		adjExternal = m_adjExternal->twin();

		delete pBCTree;
		return;
	}


	//***************************************************************************/
	//First step: calculate min depth and node lengths
	//***************************************************************************/
	//Find root Block (only node with out-degree of 0):
	node rootBlockNode;
	node n;
	forall_nodes(n, pBCTree->bcTree())
	{
		if (n->outdeg() == 0)
		{
			rootBlockNode = n;
			break;
		}
	}

	//compute block graphs:
	blockG.init(pBCTree->bcTree());
	nBlockEmbedding_to_nH.init(pBCTree->bcTree());
	eBlockEmbedding_to_eH.init(pBCTree->bcTree());
	nH_to_nBlockEmbedding.init(pBCTree->bcTree());
	eH_to_eBlockEmbedding.init(pBCTree->bcTree());
	nodeLength.init(pBCTree->bcTree());
	spqrTrees.init(pBCTree->bcTree(),0);
	computeBlockGraphs(rootBlockNode, 0);

	//Edge lengths of BC-tree, values m_{c, B} for all (c, B) \in bcTree:
	m_cB.init(pBCTree->bcTree(), 0);

	//Bottom-up traversal: (set m_cB for all {c, B} \in bcTree)
	nodeLength[rootBlockNode].init(blockG[rootBlockNode], 0);
	edge e;
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
			m_cB[e2] = bottomUpTraversal(blockNode, cutVertex);
		}
	}

	//Top-down traversal: (set m_cB for all {B, c} \in bcTree and get min depth
	//for each block)
	int maxint = 2147483647;
	minDepth.init(pBCTree->bcTree(), maxint);
	M_B.init(pBCTree->bcTree());
	M2.init(pBCTree->bcTree());
	topDownTraversal(rootBlockNode);

	//compute bT_opt:
	int depth = maxint;
	node bT_opt;
	forall_nodes(n, pBCTree->bcTree())
	{
		if (pBCTree->typeOfBNode(n) != BCTree::BComp)
			continue;
		if (minDepth[n] < depth)
		{
			depth = minDepth[n];
			bT_opt = n;
		}
	}

	//****************************************************************************
	//Second step: Embed G by expanding a maximum face in bT_opt
	//****************************************************************************
	newOrder.init(G);
	treeNodeTreated.init(pBCTree->bcTree(), false);
	embedBlock(bT_opt);

	forall_nodes(n, G)
		G.sort(n, newOrder[n]);

	forall_nodes(n, pBCTree->bcTree())
		delete spqrTrees[n];

	delete pBCTree;
}


void EmbedderMinDepth::computeBlockGraphs(const node& bT, const node& cH)
{
	//recursion:
	edge e;
	forall_adj_edges(e, bT)
	{
		if (e->source() == bT)
			continue;

		node cT = e->source();
		edge e2;
		forall_adj_edges(e2, cT)
		{
			if (e2->source() == cT)
				continue;
			node cH2 = pBCTree->cutVertex(cT, e2->source());
			computeBlockGraphs(e2->source(), cH2);
		}
	}

	//embed block bT:
	node m_cH = cH;
	if (m_cH == 0)
		m_cH = pBCTree->cutVertex(bT->firstAdj()->twinNode(), bT);
	ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockG[bT], m_cH,
		nBlockEmbedding_to_nH[bT], eBlockEmbedding_to_eH[bT],
		nH_to_nBlockEmbedding[bT], eH_to_eBlockEmbedding[bT]);

	if ( !blockG[bT].empty()
		&& blockG[bT].numberOfNodes() != 1
		&& blockG[bT].numberOfEdges() > 2)
	{
		spqrTrees[bT] = new StaticSPQRTree(blockG[bT]);
	}
}


int EmbedderMinDepth::bottomUpTraversal(const node& bT, const node& cH)
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
			m_cB[e_cT_bT2] = bottomUpTraversal(bT2, c_in_bT2);

			//update m_B and M_B:
			if (m_B < m_cB[e_cT_bT2])
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				m_B = m_cB[e_cT_bT2];
				M_B.clear();
				M_B.pushBack(cV_in_bT);
			}
			else if (m_B == m_cB[e_cT_bT2] && M_B.search(pBCTree->cutVertex(cT, bT)) == -1)
			{
				node cV_in_bT = pBCTree->cutVertex(cT, bT);
				M_B.pushBack(cV_in_bT);
			}
		}
	}

	//set vertex length for all vertices in bH to 1 if vertex is in M_B:
	nodeLength[bT].init(blockG[bT], 0);
	for (ListIterator<node> iterator = M_B.begin(); iterator.valid(); iterator++)
		nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;

	//leafs of BC-tree:
	if (M_B.size() == 0)
		return 1;

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> edgeLength(blockG[bT], 0);

	//compute maximum external face of block graph and get its size:
	int cstrLength_B_c = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockG[bT],
		nH_to_nBlockEmbedding[bT][cH],
		nodeLength[bT],
		edgeLength,
		*spqrTrees[bT]);

	if (cstrLength_B_c == M_B.size())
		return m_B;
	//else:
	return m_B + 2;
}


void EmbedderMinDepth::topDownTraversal(const node& bT)
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
				if (m_B < m_cB[e_cT_bT2])
				{
					m_B = m_cB[e_cT_bT2];
					M_B[bT].clear();
					M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
				else if (   m_B == m_cB[e_cT_bT2] && M_B[bT].search(pBCTree->cutVertex(cT, bT)) == -1)
				{
					M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
			}//forall_adj_edges(e_cT_bT2, cT)
		}//forall_adj_edges(e_bT_cT, bT)
	}
	//set vertex length for all vertices in bH to 1 if vertex is in M_B:
	nodeLength[bT].fill(0);
	NodeArray<int> m_nodeLength(blockG[bT], 0);
	for (ListIterator<node> iterator = M_B[bT].begin(); iterator.valid(); iterator++)
	{
		nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
		m_nodeLength[nH_to_nBlockEmbedding[bT][*iterator]] = 1;
	}

	//set edge length for all edges in block graph to 0:
	EdgeArray<int> edgeLengthBlock(blockG[bT], 0);

	//compute size of a maximum external face of block graph:
	NodeArray< EdgeArray<int> > edgeLengthSkel;
	int cstrLength_B_c = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
		blockG[bT],
		m_nodeLength,
		edgeLengthBlock,
		*spqrTrees[bT],
		edgeLengthSkel);

	//Prepare recursion by setting m_{c, B} for all edges {B, c} \in bcTree:
	if (M_B[bT].size() > 0)
	{
		node cT1 = pBCTree->bcproper(pBCTree->original(*(M_B[bT].begin())));
		bool calculateNewNodeLengths;
		if (M_B[bT].size() == 1 && cT1 == cT_parent)
			calculateNewNodeLengths = true;
		else
			calculateNewNodeLengths = false;
		forall_adj_edges(e_bT_cT, bT)
		{
			if (e_bT_cT->target() != bT)
				continue;
			node cT = e_bT_cT->source();
			node cH = pBCTree->cutVertex(cT, bT);

			if (M_B[bT].size() == 1 && cT1 == cT)
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
						if (m2 < m_cB[e_cT2_bT2])
						{
							m2 = m_cB[e_cT2_bT2];
							M2[bT].clear();
							M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
						else if (   m2 == m_cB[e_cT2_bT2] && M2[bT].search(pBCTree->cutVertex(cT2, bT)) == -1)
						{
							M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
						}
					}//forall_adj_edges(e_cT2_bT2, cT2)
				}//forall_adj_edges(e_bT_cT2, bT)

				//set vertex length for all vertices in bH to 1 if vertex is in M2 and
				//0 otherwise:
				nodeLength[bT][nH_to_nBlockEmbedding[bT][*(M_B[bT].begin())]] = 0;
				for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); iterator++)
					nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;

				//set edge length for all edges in block graph to 0:
				EdgeArray<int> edgeLength(blockG[bT], 0);

				//compute a maximum external face size of a face containing c in block graph:
				int maxFaceSize = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockG[bT],
					nH_to_nBlockEmbedding[bT][cH],
					nodeLength[bT],
					edgeLength,
					*spqrTrees[bT]);
				if (M2[bT].size() == 0)
					m_cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSize == M2[bT].size())
						m_cB[e_bT_cT] = m2;
					else
						m_cB[e_bT_cT] = m2 + 2;
				}

				if (calculateNewNodeLengths)
					calculateNewNodeLengths = false;
				else
				{
					//reset node lengths:
					for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); iterator++)
						nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 0;
					nodeLength[bT][nH_to_nBlockEmbedding[bT][*(M_B[bT].begin())]] = 1;
				}
			}
			else //M_B.size() != 1
			{
				//Compute a maximum face in block B containing c using the vertex lengths
				//already assigned.

				//set edge length for all edges in block graph to 0:
				EdgeArray<int> edgeLength(blockG[bT], 0);

				//compute a maximum external face size of a face containing c in block graph:
				int maxFaceSize = EmbedderMaxFaceBiconnectedGraphs<int>::computeSize(
					blockG[bT],
					nH_to_nBlockEmbedding[bT][cH],
					nodeLength[bT],
					edgeLength,
					*spqrTrees[bT],
					edgeLengthSkel);
				if (M_B[bT].size() == 0)
					m_cB[e_bT_cT] = 1;
				else
				{
					if (maxFaceSize == M_B[bT].size())
						m_cB[e_bT_cT] = m_B;
					else
						m_cB[e_bT_cT] = m_B + 2;
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
					if (m2 < m_cB[e_cT2_bT2])
					{
						m2 = m_cB[e_cT2_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if (   m2 == m_cB[e_cT2_bT2] && M2[bT].search(pBCTree->cutVertex(cT2, bT)) == -1)
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
				}//forall_adj_edges(e_cT2_bT2, cT2)
			}//forall_adj_edges(e_bT_cT2, bT)

			//set vertex length for all vertices in bH to 1 if vertex is in M2 and
			//0 otherwise:
			nodeLength[bT][nH_to_nBlockEmbedding[bT][*(M_B[bT].begin())]] = 0;
			for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); iterator++)
				nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
		} //if (calculateNewNodeLengths
		else if (M_B[bT].size() == 1)
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
					if (m2 < m_cB[e_cT2_bT2])
					{
						m2 = m_cB[e_cT2_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
					}
					else if (   m2 == m_cB[e_cT2_bT2] && M2[bT].search(pBCTree->cutVertex(cT2, bT)) == -1)
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT2, bT));
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

			topDownTraversal(e_cT_bT2->source());
		}
	}

	//Compute M_B and M2 for embedBlock-function:
	{
		M_B[bT].clear();
		M2[bT].clear();
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
				if (m_B < m_cB[e_cT_bT2])
				{
					m_B = m_cB[e_cT_bT2];
					M_B[bT].clear();
					M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
				else if (   m_B == m_cB[e_cT_bT2] && M_B[bT].search(pBCTree->cutVertex(cT, bT)) == -1)
				{
					M_B[bT].pushBack(pBCTree->cutVertex(cT, bT));
				}
			}//forall_adj_edges(e_cT_bT2, cT)
		}//forall_adj_edges(e_bT_cT, bT)

		if (M_B[bT].size() == 1)
		{
			node cT1 = pBCTree->bcproper(pBCTree->original(*(M_B[bT].begin())));
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
					if (m2 < m_cB[e_cT_bT2])
					{
						m2 = m_cB[e_cT_bT2];
						M2[bT].clear();
						M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
					else if (   m2 == m_cB[e_cT_bT2]
									 && M2[bT].search(pBCTree->cutVertex(cT, bT)) == -1)
					{
						M2[bT].pushBack(pBCTree->cutVertex(cT, bT));
					}
				}//forall_adj_edges(e_cT_bT2, cT)
			}//forall_adj_edges(e_bT_cT, bT)
		}
	}

	if (cstrLength_B_c == M_B[bT].size())
		minDepth[bT] = m_B;
	else
		minDepth[bT] = m_B + 2;
}


void EmbedderMinDepth::embedBlock(const node& bT)
{
	ListIterator<adjEntry> after;
	node cT = 0;
	embedBlock(bT, cT, after);
}


void EmbedderMinDepth::embedBlock(
	const node& bT,
	const node& cT,
	ListIterator<adjEntry>& after)
{
	treeNodeTreated[bT] = true;
	node cH = 0;
	if (!(cT == 0))
		cH = pBCTree->cutVertex(cT, bT);

	//***************************************************************************
	// 1. Compute node lengths depending on M_B, M2 and cT
	//***************************************************************************
	nodeLength[bT].fill(0);
	if (!(cT == 0) && M_B[bT].size() == 1 && *(M_B[bT].begin()) == cH)
	{
		//set node length to 1 if node is in M2 and 0 otherwise
		for (ListIterator<node> iterator = M2[bT].begin(); iterator.valid(); iterator++)
			nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
	}
	else
	{
		//set node length to 1 if node is in M_B and 0 otherwise
		for (ListIterator<node> iterator = M_B[bT].begin(); iterator.valid(); iterator++)
			nodeLength[bT][nH_to_nBlockEmbedding[bT][*iterator]] = 1;
	}

	//***************************************************************************
	// 2. Compute embedding of block
	//***************************************************************************
	EdgeArray<int> edgeLength(blockG[bT], 0);
	adjEntry m_adjExternal = 0;
	if (cH == 0)
		EmbedderMaxFaceBiconnectedGraphs<int>::embed(blockG[bT], m_adjExternal, nodeLength[bT], edgeLength);
	else
		EmbedderMaxFaceBiconnectedGraphs<int>::embed(blockG[bT], m_adjExternal, nodeLength[bT], edgeLength,
			nH_to_nBlockEmbedding[bT][cH]);

	//***************************************************************************
	// 3. Copy block embedding into graph embedding and call recursively
	//    embedBlock for all cut vertices in bT
	//***************************************************************************
	CombinatorialEmbedding CE(blockG[bT]);
	face f = CE.leftFace(m_adjExternal);

	if (*pAdjExternal == 0)
	{
		node on = pBCTree->original(nBlockEmbedding_to_nH[bT][m_adjExternal->theNode()]);
		adjEntry ae1 = on->firstAdj();
		for (adjEntry ae = ae1; ae; ae = ae->succ())
		{
			if (ae->theEdge() == pBCTree->original(eBlockEmbedding_to_eH[bT][m_adjExternal->theEdge()]))
			{
				*pAdjExternal = ae->twin();
				break;
			}
		}
	}

	node nSG;
	forall_nodes(nSG, blockG[bT])
	{
		node nH = nBlockEmbedding_to_nH[bT][nSG];
		node nG = pBCTree->original(nH);
		adjEntry ae = nSG->firstAdj();
		ListIterator<adjEntry>* pAfter;
		if (pBCTree->bcproper(nG) == cT)
			pAfter = &after;
		else
			pAfter = OGDF_NEW ListIterator<adjEntry>();

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
				//bool aeExtExists = false;
				adjEntry aeFace = f->firstAdj();
				//List<edge> extFaceEdges;
				do
				{
					//extFaceEdges.pushBack(aeFace->theEdge());
					if (aeFace->theNode() == nSG)
					{
						if (aeFace->succ())
							ae = aeFace->succ();
						else
							ae = nSG->firstAdj();
						//aeExtExists = true;
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != f->firstAdj());

				//if (aeExtExists)
				//{
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
				//}
				//else
				//{
				//	//cannot embed block into external face, so find a face with an adjacent
				//	//edge of the external face:
				//	bool foundIt = false;
				//	edge adjEdge;
				//	forall_adj_edges(adjEdge, nSG)
				//	{
				//		face m_f = CE.leftFace(adjEdge->adjSource());
				//		adjEntry aeF = m_f->firstAdj();
				//		do
				//		{
				//			if (extFaceEdges.search(aeF->theEdge()) != -1)
				//			{
				//				ae = adjEdge->adjSource();
				//				foundIt = true;
				//				break;
				//			}
				//			aeF = aeF->faceCycleSucc();
				//		} while(aeF != m_f->firstAdj());
				//		if (foundIt)
				//			break;
				//	}
				//}
			}
		}

		//embed all edges of block bT:
		bool after_ae = true;
		for (adjEntry aeNode = ae;
			after_ae || aeNode != ae;
			after_ae = (!after_ae || !aeNode->succ()) ? false : true,
			aeNode = aeNode->succ() ? aeNode->succ() : nSG->firstAdj())
		{
			edge eG = pBCTree->original(eBlockEmbedding_to_eH[bT][aeNode->theEdge()]);
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
	} //forall_nodes(nSG, blockG[bT])
}

} // end namespace ogdf
