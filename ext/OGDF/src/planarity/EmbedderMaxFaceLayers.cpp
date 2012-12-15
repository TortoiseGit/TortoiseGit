/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Computes an embedding of a graph with maximum external face.
 *
 * See the paper "Graph Embedding with Minimum Depth and Maximum External
 * Face" by C. Gutwenger and P. Mutzel (2004) for details.
 * The algorithm for maximum external face is combined with the
 * algorithm for maximum external layers which defines how to embed
 * blocks into inner faces. See diploma thesis "Algorithmen zur
 * Bestimmung von guten Graph-Einbettungen f&uuml;r orthogonale
 * Zeichnungen" (in german) by Thorsten Kerkhof (2007) for details.
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

#include <ogdf/planarity/EmbedderMaxFaceLayers.h>
#include <ogdf/internal/planarity/EmbedderMaxFaceBiconnectedGraphsLayers.h>
#include <ogdf/internal/planarity/ConnectedSubgraph.h>

namespace ogdf {

void EmbedderMaxFaceLayers::call(Graph& G, adjEntry& adjExternal)
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
		EdgeArray<int> m_edgeLength(G, 1);
		adjEntry m_adjExternal;
		EmbedderMaxFaceBiconnectedGraphsLayers<int>::embed(G, m_adjExternal, m_nodeLength, m_edgeLength);
		adjExternal = m_adjExternal;

		delete pBCTree;
		return;
	}

	//***************************************************************************/
	//First step: calculate maximum face and node lengths
	//***************************************************************************/

	//Find root Block (root node is only node with out-degree of 0):
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

	//compute block graphs and SPQR trees:
	blockG.init(pBCTree->bcTree());
	nBlockEmbedding_to_nH.init(pBCTree->bcTree());
	eBlockEmbedding_to_eH.init(pBCTree->bcTree());
	nH_to_nBlockEmbedding.init(pBCTree->bcTree());
	eH_to_eBlockEmbedding.init(pBCTree->bcTree());
	nodeLength.init(pBCTree->bcTree());
	cstrLength.init(pBCTree->bcTree());
	spqrTrees.init(pBCTree->bcTree(),0);
	computeBlockGraphs(rootBlockNode, 0);

	//Bottom-Up-Traversal:
	edge e;
	forall_adj_edges(e, rootBlockNode)
	{
		node cT = e->source();
		node cH = pBCTree->cutVertex(cT, rootBlockNode);
		node cB = nH_to_nBlockEmbedding[rootBlockNode][cH];

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
			length_v_in_rootBlock += constraintMaxFace(blockNode, cutVertex);
		}
		nodeLength[rootBlockNode][cB] = length_v_in_rootBlock;
	}

	node bT_opt = G.chooseNode(); //= G.chooseNode() only to get rid of warning
	int ell_opt = 0;
	maximumFaceRec(rootBlockNode, bT_opt, ell_opt);


	//****************************************************************************
	//Second step: Embed G by expanding a maximum face in bT_opt
	//****************************************************************************
	newOrder.init(G);
	treeNodeTreated.init(pBCTree->bcTree(), false);
	embedBlock(bT_opt);

	node v;
	forall_nodes(v, G)
		G.sort(v, newOrder[v]);

	forall_nodes(v, pBCTree->bcTree())
		delete spqrTrees[v];

	delete pBCTree;
}


void EmbedderMaxFaceLayers::computeBlockGraphs(const node& bT, const node& cH)
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
	nodeLength[bT].init(blockG[bT], 0);
	cstrLength[bT].init(blockG[bT], 0);
	if (   !blockG[bT].empty()
		&& blockG[bT].numberOfNodes() != 1
		&& blockG[bT].numberOfEdges() > 2)
	{
		spqrTrees[bT] = new StaticSPQRTree(blockG[bT]);
	}
}


int EmbedderMaxFaceLayers::constraintMaxFace(const node& bT, const node& cH)
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
			length_v_in_block += constraintMaxFace(bT2, cutVertex);
		}
		nodeLength[bT][nH_to_nBlockEmbedding[bT][vH]] = length_v_in_block;
	}

	EdgeArray<int> edgeLength(blockG[bT], 1);
	int cstrLengthBc
		= EmbedderMaxFaceBiconnectedGraphsLayers<int>::computeSize(blockG[bT],
			nH_to_nBlockEmbedding[bT][cH],
			nodeLength[bT],
			edgeLength,
			*spqrTrees[bT]);
	cstrLength[bT][nH_to_nBlockEmbedding[bT][cH]] = cstrLengthBc;
	return cstrLengthBc;
}


void EmbedderMaxFaceLayers::maximumFaceRec(const node& bT, node& bT_opt, int& ell_opt)
{
	//(B*, \ell*) := (B, size of a maximum face in B):
	node m_bT_opt = bT;
	EdgeArray<int> edgeLength(blockG[bT], 1);
	NodeArray< EdgeArray<int> > edgeLengthSkel;
	int m_ell_opt = EmbedderMaxFaceBiconnectedGraphsLayers<int>::computeSize(
		blockG[bT],
		nodeLength[bT],
		edgeLength,
		*spqrTrees[bT],
		edgeLengthSkel);

	edge e;
	forall_adj_edges(e, bT)
	{
		if (e->target() != bT)
			continue;
		node cT = e->source();
		node cH = pBCTree->cutVertex(cT, bT);

		EdgeArray<int> edgeLength(blockG[bT], 1);
		cstrLength[bT][nH_to_nBlockEmbedding[bT][cH]] = EmbedderMaxFaceBiconnectedGraphsLayers<int>::computeSize(
			blockG[bT],
			nH_to_nBlockEmbedding[bT][cH],
			nodeLength[bT],
			edgeLength,
			*spqrTrees[bT],
			edgeLengthSkel);

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
				node bT2 = e2->target();
				L += cstrLength[bT2][nH_to_nBlockEmbedding[bT2][pBCTree->cutVertex(cT, bT2)]];
			}
		}

		forall_adj_edges(e2, cT)
		{
			//check if edge is an outgoing edge or the edge from bT to cT:
			if (e2->target() != cT || e2->source() == bT)
				continue;

			//get partner vertex of c in the block graph of B'=e->source():
			node pT = e2->source();
			node partnerV = pBCTree->cutVertex(cT, pT);
			node pB = nH_to_nBlockEmbedding[pT][partnerV];
			nodeLength[pT][pB] = L - cstrLength[pT][pB];

			//pBCTree->originalGraph().chooseNode() just to get rid of warning:
			node thisbT_opt = pBCTree->originalGraph().chooseNode();
			int thisell_opt = 0;
			maximumFaceRec(pT, thisbT_opt, thisell_opt);
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
}


void EmbedderMaxFaceLayers::embedBlock(const node& bT)
{
	ListIterator<adjEntry> after;
	node cT = 0;
	embedBlock(bT, cT, after);
}


void EmbedderMaxFaceLayers::embedBlock(
	const node& bT,
	const node& cT,
	ListIterator<adjEntry>& after)
{
	treeNodeTreated[bT] = true;
	node cH = 0;
	if (!(cT == 0))
		cH = pBCTree->cutVertex(cT, bT);

	//***************************************************************************
	// 1. Compute embedding of block
	//***************************************************************************
	EdgeArray<int> edgeLength(blockG[bT], 1);
	adjEntry m_adjExternal = 0;
	if (cH == 0)
		EmbedderMaxFaceBiconnectedGraphsLayers<int>::embed(blockG[bT], m_adjExternal,
			nodeLength[bT], edgeLength);
	else
		EmbedderMaxFaceBiconnectedGraphsLayers<int>::embed(blockG[bT], m_adjExternal,
			nodeLength[bT], edgeLength, nH_to_nBlockEmbedding[bT][cH]);

	//***************************************************************************
	// 2. Copy block embedding into graph embedding and call recursively
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

	bool DGcomputed = false;
	int extFaceID = 0;
	Graph* p_DG;
	List<node>* p_fPG_to_nDG;
	NodeArray<int>* p_nDG_to_fPG;
	NodeArray< List<adjEntry> >* p_adjacencyList;
	List< List<adjEntry> >* p_faces;
	NodeArray<int>* p_distances;

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
				bool aeExtExists = false;
				adjEntry aeFace = f->firstAdj();
				do
				{
					if (aeFace->theNode() == nSG)
					{
						if (aeFace->succ())
							ae = aeFace->succ();
						else
							ae = nSG->firstAdj();
						aeExtExists = true;
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != f->firstAdj());

				if (!aeExtExists)
				{
					if (!DGcomputed)
					{
						p_DG = new Graph();
						p_fPG_to_nDG = OGDF_NEW List<node>();
						p_nDG_to_fPG = OGDF_NEW NodeArray<int>();
						p_adjacencyList = OGDF_NEW NodeArray< List<adjEntry> >();
						p_faces = OGDF_NEW List< List<adjEntry> >;
						p_distances = OGDF_NEW NodeArray<int>;
						DGcomputed = true;

						//compute dual graph of skeleton graph:
						p_adjacencyList->init(blockG[bT]);
						node nBG;
						forall_nodes(nBG, blockG[bT])
						{
							adjEntry ae_nBG;
							forall_adj(ae_nBG, nBG)
								(*p_adjacencyList)[nBG].pushBack(ae_nBG);
						}

						NodeArray< List<adjEntry> > adjEntryTreated(blockG[bT]);
						forall_nodes(nBG, blockG[bT])
						{
							adjEntry adj;
							forall_adj(adj, nBG)
							{
								if (adjEntryTreated[nBG].search(adj) != -1)
									continue;

								List<adjEntry> newFace;
								adjEntry adj2 = adj;
								do
								{
									newFace.pushBack(adj2);
									adjEntryTreated[adj2->theNode()].pushBack(adj2);
									node tn = adj2->twinNode();
									int idx = (*p_adjacencyList)[tn].search(adj2->twin());
									if (idx - 1 < 0)
										idx = (*p_adjacencyList)[tn].size() - 1;
									else
										idx -= 1;
									adj2 = *((*p_adjacencyList)[tn].get(idx));
								} while (adj2 != adj);
								p_faces->pushBack(newFace);
							}
						} //forall_nodes(nBG, blockG[bT])

						p_nDG_to_fPG->init(*p_DG);

						for (ListIterator< List<adjEntry> > it = p_faces->begin(); it.valid(); it++)
						{
							node nn = p_DG->newNode();
							(*p_nDG_to_fPG)[nn] = p_fPG_to_nDG->search(*(p_fPG_to_nDG->pushBack(nn)));
						}

						NodeArray< List<node> > adjFaces(*p_DG);
						int i = 0;
						for (ListIterator< List<adjEntry> > it = p_faces->begin(); it.valid(); it++)
						{
							int f1_id = i;
							for (ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); it2++)
							{
								int f2_id = 0;
								int j = 0;
								for (ListIterator< List<adjEntry> > it3 = p_faces->begin(); it3.valid(); it3++)
								{
									bool do_break = false;
									for (ListIterator<adjEntry> it4 = (*it3).begin(); it4.valid(); it4++)
									{
										if ((*it4) == (*it2)->twin())
										{
											f2_id = j;
											do_break = true;
											break;
										}
									}
									if (do_break)
										break;
									j++;
								}

								if (   f1_id != f2_id
										&& adjFaces[*(p_fPG_to_nDG->get(f1_id))].search(*(p_fPG_to_nDG->get(f2_id))) == -1
										&& adjFaces[*(p_fPG_to_nDG->get(f2_id))].search(*(p_fPG_to_nDG->get(f1_id))) == -1)
								{
									adjFaces[*(p_fPG_to_nDG->get(f1_id))].pushBack(*(p_fPG_to_nDG->get(f2_id)));
									p_DG->newEdge(*(p_fPG_to_nDG->get(f1_id)), *(p_fPG_to_nDG->get(f2_id)));
								}

								if (*it2 == f->firstAdj())
									extFaceID = f1_id;
							} //for (ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); it2++)
							i++;
						} //for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); it++)

						//compute shortest path from every face to the external face:
						List<edge> DG_edges;
						p_DG->allEdges(DG_edges);
						for (ListIterator<edge> it_e = DG_edges.begin(); it_e.valid(); it_e++)
						{
							node s = (*it_e)->source();
							node t = (*it_e)->target();
							p_DG->newEdge(t, s);
						}
						ShortestPathWithBFM shortestPath;
						node efDG = *(p_fPG_to_nDG->get(extFaceID));
						EdgeArray<int> el(*p_DG, 1);
						p_distances->init(*p_DG);
						NodeArray<edge> pi(*p_DG);
						shortestPath.call(*p_DG, efDG, el, *p_distances, pi);
					} //if (!DGcomputed)

					//choose face with minimal shortest path:
					List<adjEntry> optFace;
					int optFaceDist = -1;
					for (int fID = 0; fID < p_faces->size(); fID++)
					{
						List<adjEntry> theFace = *(p_faces->get(fID));
						adjEntry ae_nSG;
						bool contains_nSG = false;
						for (ListIterator<adjEntry> it_ae = theFace.begin(); it_ae.valid(); it_ae++)
						{
							if ((*it_ae)->theNode() == nSG)
							{
								contains_nSG = true;
								ae_nSG = *it_ae;
								break;
							}
						}

						if (contains_nSG)
						{
							int thisDist = (*p_distances)[*p_fPG_to_nDG->get(fID)];
							if (optFaceDist == -1 || optFaceDist > thisDist)
							{
								optFace = theFace;
								optFaceDist = thisDist;
								if (ae_nSG->succ())
									ae = ae_nSG->succ();
								else
									ae = nSG->firstAdj();
							}
						}
					} //for (int fID = 0; fID < faces.size(); fID++)
				} //if (!aeExtExists)

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

	if (DGcomputed)
	{
		delete p_DG;
		delete p_fPG_to_nDG;
		delete p_nDG_to_fPG;
		delete p_adjacencyList;
		delete p_faces;
		delete p_distances;
	}
}

} // end namespace ogdf
