/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief The algorithm computes a planar embedding with minimum
 *   depth if the embedding for all blocks of the graph is given.
 *   For details see the paper "Minimum Depth Graph Drawing" by
 *   M. Pizzonia and R. Tamassia.
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

#include <ogdf/planarity/EmbedderMinDepthPiTa.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/internal/planarity/ConnectedSubgraph.h>

namespace ogdf {

void EmbedderMinDepthPiTa::call(Graph& G, adjEntry& adjExternal)
{
	adjExternal = 0;
	pAdjExternal = &adjExternal;
	node n;
	edge e;

	//simple base cases:
	if (G.numberOfNodes() <= 1)
		return;

	if (G.numberOfEdges() == 1)
	{
		edge e = G.firstEdge();
		adjExternal = e->adjSource();
		return;
	}

	if(useExtendedDepthDefinition()) {
		dummyNodes.clear();
		pBCTree = new BCTree(G);
		if (pBCTree->bcTree().numberOfNodes() != 1)
		{
			node bT;
			forall_nodes(bT, pBCTree->bcTree())
			{
				if (pBCTree->typeOfBNode(bT) != BCTree::BComp)
					continue;

				node cT = bT->firstAdj()->twinNode();
				node cH = pBCTree->cutVertex(cT, bT);
				Graph SG;
				NodeArray<node> nSG_to_nG;
				ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), SG, cH, nSG_to_nG);
				if (SG.numberOfEdges() == 1)
				{
					node dummyNodePG = G.newNode();
					dummyNodes.pushBack(dummyNodePG);
					node SGnode1 = SG.chooseEdge()->source();
					node PGnode1 = pBCTree->original(nSG_to_nG[SGnode1]);
					node SGnode2 = SG.chooseEdge()->target();
					node PGnode2 = pBCTree->original(nSG_to_nG[SGnode2]);
					G.newEdge(PGnode1, dummyNodePG);
					G.newEdge(PGnode2, dummyNodePG);
				}
			}
		}
		delete pBCTree;
	}

	//HINT: Edges are directed from child to parent in BC-trees
	pBCTree = new BCTree(G);

	//base case of biconnected graph:
	if (pBCTree->bcTree().numberOfNodes() == 1)
	{
		planarEmbed(G);
		CombinatorialEmbedding CE(G);
		adjExternal = CE.chooseFace()->firstAdj();
		deleteDummyNodes(G, adjExternal);

		delete pBCTree;
		return;
	}


	//***************************************************************************/
	//First step: embed all blocks
	//***************************************************************************/
	newOrder.init(G);
	nodeLength.init(pBCTree->bcTree());
	oneEdgeBlockNodes.clear();

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

	blockG.init(pBCTree->bcTree());
	nBlockEmbedding_to_nH.init(pBCTree->bcTree());
	eBlockEmbedding_to_eH.init(pBCTree->bcTree());
	nH_to_nBlockEmbedding.init(pBCTree->bcTree());
	eH_to_eBlockEmbedding.init(pBCTree->bcTree());
	embedBlocks(rootBlockNode, 0);

	//***************************************************************************/
	//Second step: Constrained Minimization
	//***************************************************************************/
	node vT = rootBlockNode->firstAdj()->twinNode();
	bcTreePG.clear();
	nBCTree_to_npBCTree.init(bcTreePG);
	npBCTree_to_nBCTree.init(pBCTree->bcTree());
	forall_nodes(n, pBCTree->bcTree())
	{
		node m = bcTreePG.newNode();
		nBCTree_to_npBCTree[m] = n;
		npBCTree_to_nBCTree[n] = m;
	}
	forall_edges(e, pBCTree->bcTree())
	{
		if (e->source() == vT)
			bcTreePG.newEdge(npBCTree_to_nBCTree[e->target()], npBCTree_to_nBCTree[vT]);
		else
			bcTreePG.newEdge(npBCTree_to_nBCTree[e->source()], npBCTree_to_nBCTree[e->target()]);
	}

	G_nT.init(pBCTree->bcTree());
	nG_nT_to_nPG.init(pBCTree->bcTree());
	nPG_to_nG_nT.init(pBCTree->bcTree());
	eG_nT_to_ePG.init(pBCTree->bcTree());
	ePG_to_eG_nT.init(pBCTree->bcTree());
	Gamma_adjExt_nT.init(pBCTree->bcTree());

	tmpAdjExtFace = 0;
	embedCutVertex(npBCTree_to_nBCTree[vT], true);
	forall_nodes(n, G)
		G.sort(n, newOrder[n]);

	//adjExternal = tmpAdjExtFace;
	//deleteDummyNodes(G, adjExternal);
	//return;

	//***************************************************************************/
	//Fourth step: Find the knot of the block cutface tree of the embedding and,
	//if needed, modify it into a minimum diameter embedding.
	//***************************************************************************/
	//a) Compute dual graph:
	NodeArray< List<adjEntry> > adjacencyList(G);
	forall_nodes(n, G)
	{
		adjEntry ae;
		forall_adj(ae, n)
			adjacencyList[n].pushBack(ae);
	}

	NodeArray< List<adjEntry> > adjEntryTreated(G);
	faces.clear();
	forall_nodes(n, G)
	{
		adjEntry adj;
		forall_adj(adj, n)
		{
			if (adjEntryTreated[n].search(adj) != -1)
				continue;

			List<adjEntry> newFace;
			adjEntry adj2 = adj;
			do
			{
				newFace.pushBack(adj2);
				adjEntryTreated[adj2->theNode()].pushBack(adj2);
				node tn = adj2->twinNode();
				int idx = adjacencyList[tn].search(adj2->twin());
				if (idx - 1 < 0)
					idx = adjacencyList[tn].size() - 1;
				else
					idx -= 1;
				adj2 = *(adjacencyList[tn].get(idx));
			} while (adj2 != adj);
			faces.pushBack(newFace);
		}
	}

	Graph DG;
	fPG_to_nDG.clear();
	nDG_to_fPG.init(DG);

	for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); it++)
	{
		node nn = DG.newNode();
		nDG_to_fPG[nn] = fPG_to_nDG.search(*(fPG_to_nDG.pushBack(nn)));
	}

	int extFaceID = 0;
	NodeArray< List<node> > adjFaces(DG);
	int i = 0;
	for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); it++)
	{
		int f1_id = i;
		for (ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); it2++)
		{
			int f2_id = 0;
			int j = 0;
			for (ListIterator< List<adjEntry> > it3 = faces.begin(); it3.valid(); it3++)
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
					&& adjFaces[*(fPG_to_nDG.get(f1_id))].search(*(fPG_to_nDG.get(f2_id))) == -1
					&& adjFaces[*(fPG_to_nDG.get(f2_id))].search(*(fPG_to_nDG.get(f1_id))) == -1)
			{
				adjFaces[*(fPG_to_nDG.get(f1_id))].pushBack(*(fPG_to_nDG.get(f2_id)));
				DG.newEdge(*(fPG_to_nDG.get(f1_id)), *(fPG_to_nDG.get(f2_id)));
			}

			if (*it2 == tmpAdjExtFace)
				extFaceID = f1_id;
		}
		i++;
	}

	//b) compute block-cutface tree, its diametral tree Tdiam and find the knot.
	pm_blockCutfaceTree = new BCTree(DG);
	BCTree& m_blockCutfaceTree = *pm_blockCutfaceTree;

	//if graph has only one cutface, return computed embedding with
	//this cutface as external face:
	if (m_blockCutfaceTree.numberOfCComps() == 0)
	{
		if (pBCTree->numberOfBComps() == 1)
			adjExternal = tmpAdjExtFace;
		else
		{
			node bT = rootBlockNode;
			if (blockG[rootBlockNode].numberOfEdges() != 1)
			{
				adjEntry ae_cT = bT->firstAdj()->twin();
				adjEntry ae_cT2 = ae_cT->succ();
				if (!ae_cT2)
					ae_cT2 = ae_cT->theNode()->firstAdj();
				bT = ae_cT2->twinNode();
			}
			edge eB = blockG[bT].chooseEdge();
			edge ePG = pBCTree->original(eBlockEmbedding_to_eH[bT][eB]);
			adjExternal = ePG->adjSource();
		}

		delete pBCTree;
		delete pm_blockCutfaceTree;
		deleteDummyNodes(G, adjExternal);
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		return;//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	}

	node m_rootOfBlockCutfaceTree;
	forall_nodes(n, m_blockCutfaceTree.bcTree())
	{
		if (n->outdeg() == 0)
		{
			m_rootOfBlockCutfaceTree = n;
			break;
		}
	}

	//if only one cutface exists, this face is the optimum external face:
	if (m_blockCutfaceTree.numberOfCComps() == 1)
	{
		node nr = m_rootOfBlockCutfaceTree->firstAdj()->twinNode();
		node cv = m_blockCutfaceTree.cutVertex(nr, nr);
		node ocv = m_blockCutfaceTree.original(cv);
		int cfid = nDG_to_fPG[ocv];
		adjExternal = (*((*(faces.get(cfid))).begin()));
		delete pBCTree;
		delete pm_blockCutfaceTree;
		deleteDummyNodes(G, adjExternal);
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		return;//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	}

	blockCutfaceTree.clear();
	nBlockCutfaceTree_to_nm_blockCutfaceTree.init(blockCutfaceTree);
	nm_blockCutfaceTree_to_nBlockCutfaceTree.init(m_blockCutfaceTree.bcTree());
	forall_nodes(n, m_blockCutfaceTree.bcTree())
	{
		node m = blockCutfaceTree.newNode();
		nBlockCutfaceTree_to_nm_blockCutfaceTree[m] = n;
		nm_blockCutfaceTree_to_nBlockCutfaceTree[n] = m;
	}
	forall_edges(e, m_blockCutfaceTree.bcTree())
		blockCutfaceTree.newEdge(nm_blockCutfaceTree_to_nBlockCutfaceTree[e->source()],
			nm_blockCutfaceTree_to_nBlockCutfaceTree[e->target()]);

	//Root tree at external face. If external face is not a cutface in the
	//block-cutface tree, choose an arbitrary cutface as root, because
	//current external face cannot be the optimum external face.
	node rDG = *(fPG_to_nDG.get(extFaceID));
	node rmBCFT = m_blockCutfaceTree.bcproper(rDG);
	if (m_blockCutfaceTree.typeOfBNode(rmBCFT) != BCTree::CComp)
	{
		rmBCFT = m_rootOfBlockCutfaceTree->firstAdj()->twinNode();

		//adjExternal = *((*(faces.get(nDG_to_fPG[m_blockCutfaceTree.original(m_blockCutfaceTree.cutVertex(m_rootOfBlockCutfaceTree->firstAdj()->twinNode(), m_rootOfBlockCutfaceTree->firstAdj()->twinNode()))]))).begin());
		//return;
	}

	node rootOfBlockCutfaceTree = nm_blockCutfaceTree_to_nBlockCutfaceTree[rmBCFT];
	invertPath(blockCutfaceTree, rootOfBlockCutfaceTree, 0);

	edgeLength_blockCutfaceTree.init(blockCutfaceTree);
	computeBlockCutfaceTreeEdgeLengths(rootOfBlockCutfaceTree);
	nBlockCutfaceTree_to_nTdiam.init(blockCutfaceTree);
	nTdiam_to_nBlockCutfaceTree.init(Tdiam);
	Tdiam_initialized = false;
	computeTdiam(rootOfBlockCutfaceTree);

	//if Tdiam is empty, following steps are not necessary:
	if (Tdiam_initialized)
	{
		node knot = nTdiam_to_nBlockCutfaceTree[knotTdiam];
		node m_knot = nBlockCutfaceTree_to_nm_blockCutfaceTree[knot];

		//d) compute mapping bDG_to_bPG and bPG_to_bDG
		bDG_to_bPG.init(blockCutfaceTree);
		bPG_to_bDG.init(pBCTree->bcTree());
		edge e_root_to_nbDG;
		forall_adj_edges(e_root_to_nbDG, rootOfBlockCutfaceTree)
		{
			node nbDG = e_root_to_nbDG->source();
			List<node> tmpBlocksNodes;
			List<node> tmpChildBlocks;
			node bPG = computeBlockMapping(nbDG, rootOfBlockCutfaceTree, tmpBlocksNodes, tmpChildBlocks);
			bDG_to_bPG[nbDG] = bPG;
			bPG_to_bDG[bPG] = nbDG;
		}

		//c) if needed, modify the embedding into a minimum depth diameter embedding
		if (   m_blockCutfaceTree.typeOfBNode(m_knot) == BCTree::BComp
				&& rootOfBlockCutfaceTree != knotTdiam)
		{
			//node bT = bDG_to_bPG[knot];
			List<node> childrenOfKnot;
			List<node> childrenOfKnot_bT;
			List<node> childrenOfKnotInBCTree;
			edge e_kBC_to_cBC;
			{
				forall_adj_edges(e_kBC_to_cBC, bDG_to_bPG[knot])
				{
					if (e_kBC_to_cBC->target() != bDG_to_bPG[knot])
						continue;

					childrenOfKnotInBCTree.pushBack(e_kBC_to_cBC->source());
				}
			}
			edge e_knot_to_w;
			forall_adj_edges(e_knot_to_w, knotTdiam)
			{
				if (e_knot_to_w->target() != knotTdiam)
					continue;

				node child = nTdiam_to_nBlockCutfaceTree[e_knot_to_w->source()];
				node childBCFTree = nBlockCutfaceTree_to_nm_blockCutfaceTree[child];
				edge e_childBCFTree_to_b;
				forall_adj_edges(e_childBCFTree_to_b, childBCFTree)
				{
					if (e_childBCFTree_to_b->target() != childBCFTree)
						continue;

					node bT = e_childBCFTree_to_b->target();
					node bBCTree = bDG_to_bPG[bT];
					node connectingNode = 0;
					while (connectingNode == 0)
					{
						node parent_bBCTree;
						edge eParent;
						forall_adj_edges(eParent, bBCTree)
						{
							if (eParent->source() == bBCTree)
							{
								parent_bBCTree = eParent->target();
								break;
							}
						}
						if (childrenOfKnotInBCTree.search(parent_bBCTree) != -1)
						{
							connectingNode = parent_bBCTree;
							childrenOfKnot_bT.pushBack(bBCTree);
						}
						else
						{
							forall_adj_edges(eParent, parent_bBCTree)
							{
								if (eParent->source() == parent_bBCTree)
								{
									bBCTree = eParent->target();
									break;
								}
							}
						}
					}
					childrenOfKnot.pushBack(pBCTree->original(connectingNode));
				}
			}
			CombinatorialEmbedding CE(blockG[bDG_to_bPG[m_knot]]);
			face f;
			forall_faces(f, CE)
			{
				int numOfEntriesFromList = 0;
				adjEntry ae;
				forall_face_adj(ae, f)
				{
					node orgNode = pBCTree->original(nBlockEmbedding_to_nH[bDG_to_bPG[m_knot]] [ae->theNode()]);
					if (childrenOfKnot.search(orgNode) != -1)
						numOfEntriesFromList++;
				}
				if (numOfEntriesFromList == childrenOfKnot.size())
				{
					//i) remove embedding of blocks
					NodeArray< NodeArray< List<adjEntry> > > adjList(pBCTree->bcTree(), G);
					for (ListIterator<node> it = childrenOfKnot.begin(); it.valid(); it++)
					{
						node nG = *it;
						node bT = *(childrenOfKnot_bT.get(childrenOfKnot.search(*it)));
						List<node> nodeList;
						blockG[bT].allNodes(nodeList);
						ListIterator<adjEntry> it_ae;
						for (it_ae = newOrder[nG].begin(); it_ae.valid(); it_ae++)
						{
							node otherNode = (*it_ae)->twinNode();
							if (nodeList.search(otherNode) != -1)
							{
								ListIterator<adjEntry> pred_it = it_ae.pred();
								adjList[bT][nG].pushBack(*it_ae);
								newOrder[nG].del(it_ae);
								if (pred_it.valid())
									it_ae = pred_it;
								else
									it_ae = newOrder[nG].begin();
							}
						}
					}

					//ii) embed blocks into f
					for (ListIterator<node> it = childrenOfKnot.begin(); it.valid(); it++)
					{
						node nG = *it;
						node bT = *(childrenOfKnot_bT.get(childrenOfKnot.search(*it)));
						//find adjEntry of nG in f
						adjEntry ae;
						forall_face_adj(ae, f)
						{
							if (pBCTree->original(nBlockEmbedding_to_nH[bT][ae->theNode()]) == nG)
							{
								break;
							}
						}
						ListIterator<adjEntry> after = newOrder[nG].get(newOrder[nG].search(ae));
						for (ListIterator<adjEntry> it_cpy = adjList[bT][nG].begin(); it_cpy.valid(); it_cpy++)
							after = newOrder[nG].insertAfter(*it_cpy, after);
					}

					//done:
					break;
				}
			}

			forall_nodes(n, G)
				G.sort(n, newOrder[n]);
		}
	}

	//***************************************************************************/
	//Fifth step: Select face with minimum eccentricity in the block-cutface tree
	//as external face.
	//***************************************************************************/

	eccentricity.init(blockCutfaceTree, 0);
	eccentricity_alt.init(blockCutfaceTree, 0);
	eccentricityBottomUp(rootOfBlockCutfaceTree);
	eccentricityTopDown(rootOfBlockCutfaceTree);
	node cf_opt;
	int ecc_opt = -1;
	node nBCFT;
	forall_nodes(nBCFT, blockCutfaceTree)
	{
		node n_mBCFT = nBlockCutfaceTree_to_nm_blockCutfaceTree[nBCFT];
		if (m_blockCutfaceTree.typeOfBNode(n_mBCFT) != BCTree::CComp)
			continue;

		if (eccentricity[nBCFT] < ecc_opt || ecc_opt == -1)
		{
			ecc_opt = eccentricity[nBCFT];
			cf_opt = nBCFT;
		}
	}
	node cf_opt_mBCFT = nBlockCutfaceTree_to_nm_blockCutfaceTree[cf_opt];
	node cf_opt_H = m_blockCutfaceTree.cutVertex(cf_opt_mBCFT, cf_opt_mBCFT);
	node cf_opt_DG = m_blockCutfaceTree.original(cf_opt_H);
	adjExternal = *((*(faces.get(nDG_to_fPG[cf_opt_DG]))).begin());

	delete pBCTree;
	delete pm_blockCutfaceTree;
	deleteDummyNodes(G, adjExternal);
}


int EmbedderMinDepthPiTa::eccentricityBottomUp(const node& nT)
{
	int thisEccentricity[2] = {0, 0};
	edge e_nT_to_mT;
	forall_adj_edges(e_nT_to_mT, nT)
	{
		if (e_nT_to_mT->target() != nT)
			continue;

		node mT = e_nT_to_mT->source();
		int mT_eccentricity = eccentricityBottomUp(mT) + 1;
		if (mT_eccentricity > thisEccentricity[0])
		{
			thisEccentricity[1] = thisEccentricity[0];
			thisEccentricity[0] = mT_eccentricity;
		}
		else if (mT_eccentricity > thisEccentricity[1])
			thisEccentricity[1] = mT_eccentricity;
	}

	eccentricity[nT] = thisEccentricity[0];
	eccentricity_alt[nT] = thisEccentricity[1];
	return thisEccentricity[0];
}


void EmbedderMinDepthPiTa::eccentricityTopDown(const node& nT)
{
	int thisEccentricity = eccentricity[nT];
	int thisEccentricity_alt = eccentricity_alt[nT];
	edge e_nT_to_mT;
	forall_adj_edges(e_nT_to_mT, nT)
	{
		if (e_nT_to_mT->source() != nT)
			continue;

		node mT = e_nT_to_mT->target();
		if (eccentricity[mT] == thisEccentricity + 1 && eccentricity_alt[mT] + 1 >= thisEccentricity)
		{
			thisEccentricity_alt = thisEccentricity;
			thisEccentricity = eccentricity_alt[mT] + 1;
		}
		else if (eccentricity[mT] != thisEccentricity + 1 && eccentricity[mT] + 1 > thisEccentricity)
		{
			thisEccentricity_alt = thisEccentricity;
			thisEccentricity = eccentricity[mT] + 1;
		}
		else if (eccentricity_alt[mT] + 1 > thisEccentricity_alt)
			thisEccentricity_alt = eccentricity_alt[mT] + 1;
	}
	eccentricity[nT] = thisEccentricity;
	eccentricity_alt[nT] = thisEccentricity_alt;

	{
		forall_adj_edges(e_nT_to_mT, nT)
		{
			if (e_nT_to_mT->target() != nT)
				continue;

			node mT = e_nT_to_mT->source();
			eccentricityTopDown(mT);
		}
	}
}


node EmbedderMinDepthPiTa::computeBlockMapping(
	const node& bDG,
	const node& parent,
	List<node>& blocksNodes,
	List<node>& childBlocks)
{
	List<node> childNodes;
	edge e_bDG_to_cDG;
	forall_adj_edges(e_bDG_to_cDG, bDG)
	{
		if (e_bDG_to_cDG->target() != bDG)
			continue;

		node cf = e_bDG_to_cDG->source();
		edge e_cf_to_bDG2;
		forall_adj_edges(e_cf_to_bDG2, cf)
		{
			if (e_cf_to_bDG2->target() != cf)
				continue;

			node bDG2 = e_cf_to_bDG2->source();

			//recursion:
			List<node> thisBlocksNodes;
			List<node> thisChildBlocks;
			node bDG2_map = computeBlockMapping(bDG2, cf, thisBlocksNodes, thisChildBlocks);
			childBlocks.conc(thisChildBlocks);
			childBlocks.pushBack(bDG2_map);
			bDG_to_bPG[bDG2] = bDG2_map;
			bPG_to_bDG[bDG2_map] = bDG2;
			childNodes.conc(thisBlocksNodes);
		}

		List<node> m_childNodes = childNodes;
		for (ListIterator<node> it = m_childNodes.begin(); it.valid(); it++)
		{
			node n = *it;
			bool delete_node = false;

			if (n->degree() == 1)
			{
				//node of one-edge-block
				delete_node = true;
			}
			else if (pBCTree->typeOfGNode(n) != BCTree::CutVertex)
			{
				//node is a non-cutvertex node of another block
				delete_node = true;
			}
			else if (n->degree() == 2)
			{
				//node of one-edge-block
				delete_node = true;
			}
			else
			{
				//test if cutvertex does not connect current block with
				//child (in block-cutface tree) (mapped) block
				int numOfBlocksInList = 0;
				node cH = pBCTree->bcproper(n);
				node cT = pBCTree->cutVertex(cH, cH);
				edge e_cT_bT;
				forall_adj_edges(e_cT_bT, cT)
				{
					node bT = (e_cT_bT->source() == cT) ? e_cT_bT->target() : e_cT_bT->source();
					if (childBlocks.search(bT) != -1)
						numOfBlocksInList++;
				}
				if (numOfBlocksInList == cT->degree())
					delete_node = true;
			}

			if (delete_node)
				childNodes.del(childNodes.get(childNodes.search(n)));
		}
	}

	node parentT = nBlockCutfaceTree_to_nm_blockCutfaceTree[parent];
	node bDGT = nBlockCutfaceTree_to_nm_blockCutfaceTree[bDG];
	node parentH = pm_blockCutfaceTree->cutVertex(parentT, bDGT);
	Graph SG;
	NodeArray<node> nSG_to_nH;
	ConnectedSubgraph<int>::call(pm_blockCutfaceTree->auxiliaryGraph(),
		SG, parentH, nSG_to_nH);

	List<node> blockNodesDG;
	node nSG;
	forall_nodes(nSG, SG)
	{
		if (parentH == nSG_to_nH[nSG])
			continue;

		int faceID_PG = nDG_to_fPG[pm_blockCutfaceTree->original(nSG_to_nH[nSG])];
		ListIterator<adjEntry> beginIt = (*(faces.get(faceID_PG))).begin();
		for (ListIterator<adjEntry> it = beginIt; it.valid(); it++)
		{
			node nPG = (*it)->theNode();
			if (childNodes.search(nPG) == -1
				&& blockNodesDG.search(nPG) == -1
				&& oneEdgeBlockNodes.search(nPG) == -1)
			{
				blockNodesDG.pushBack(nPG);
			}
		}
	}

	node bT;
	forall_nodes(bT, pBCTree->bcTree())
	{
		if (pBCTree->typeOfBNode(bT) != BCTree::BComp)
			continue;

		bool isSearchedBlock = true;
		node n;
		forall_nodes(n, blockG[bT])
		{
			if (blockNodesDG.search(pBCTree->original(nBlockEmbedding_to_nH[bT][n])) == -1)
			{
				isSearchedBlock = false;
				break;
			}
		}
		if (isSearchedBlock)
		{
			node nChild;
			forall_nodes(nChild, blockG[bT])
				blocksNodes.pushBack(pBCTree->original(nBlockEmbedding_to_nH[bT][nChild]));
			return bT;
		}
	}

	//ohoh...
	return 0;
}


void EmbedderMinDepthPiTa::invertPath(Graph& G, const node& n, const edge& e)
{
	edge e2;
	forall_adj_edges(e2, n)
	{
		if (e != e2 && e2->source() == n)
		{
			invertPath(G, e2->target(), e2);
			G.reverseEdge(e2);
		}
	}
}


void EmbedderMinDepthPiTa::computeTdiam(const node& n)
{
	if (n->indeg() == 0)
		return;

	int maxEdgeLength = -1;
	int numEdgesWithMaxLength = 0;
	edge e_n_to_m;
	{
		forall_adj_edges(e_n_to_m, n)
		{
			if (e_n_to_m->target() != n)
				continue;
			int thisEdgeLength = edgeLength_blockCutfaceTree[e_n_to_m];
			if (thisEdgeLength > maxEdgeLength)
			{
				maxEdgeLength = thisEdgeLength;
				numEdgesWithMaxLength = 1;
			}
			else if (thisEdgeLength == maxEdgeLength)
				numEdgesWithMaxLength++;
		}
	}

	forall_adj_edges(e_n_to_m, n)
	{
		if (e_n_to_m->target() != n)
			continue;

		if (edgeLength_blockCutfaceTree[e_n_to_m] < maxEdgeLength)
			continue;

		node m = e_n_to_m->source();
		bool Tdiam_was_initialized = Tdiam_initialized;
		if (numEdgesWithMaxLength > 1 && !Tdiam_initialized)
		{
			node nTdiam = Tdiam.newNode();
			nBlockCutfaceTree_to_nTdiam[n] = nTdiam;
			nTdiam_to_nBlockCutfaceTree[nTdiam] = n;
			knotTdiam = nTdiam;
			Tdiam_initialized = true;
		}

		if (Tdiam_was_initialized || numEdgesWithMaxLength > 1)
		{
			node mTdiam = Tdiam.newNode();
			nBlockCutfaceTree_to_nTdiam[m] = mTdiam;
			nTdiam_to_nBlockCutfaceTree[mTdiam] = m;
			node source_nTdiam = nBlockCutfaceTree_to_nTdiam[n];
			node target_nTdiam = mTdiam;
			Tdiam.newEdge(source_nTdiam, target_nTdiam);
		}

		computeTdiam(m);
	}
}


int EmbedderMinDepthPiTa::computeBlockCutfaceTreeEdgeLengths(const node& n)
{
	if (n->indeg() == 0)
		return 0;

	int maxChildrenEdgeLength = 0;
	edge e_n_to_m;
	forall_adj_edges(e_n_to_m, n)
	{
		if (e_n_to_m->target() != n)
			continue;

		node m = e_n_to_m->source();
		edgeLength_blockCutfaceTree[e_n_to_m] = computeBlockCutfaceTreeEdgeLengths(m);
		if (edgeLength_blockCutfaceTree[e_n_to_m] > maxChildrenEdgeLength)
			maxChildrenEdgeLength = edgeLength_blockCutfaceTree[e_n_to_m];
	}
	return maxChildrenEdgeLength + 1;
}


void EmbedderMinDepthPiTa::embedBlocks(const node& bT, const node& cH)
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
			embedBlocks(e2->source(), cH2);
		}
	}

	//embed block bT:
	node m_cH = cH;
	if (m_cH == 0)
		m_cH = pBCTree->cutVertex(bT->firstAdj()->twinNode(), bT);
	ConnectedSubgraph<int>::call(pBCTree->auxiliaryGraph(), blockG[bT], m_cH,
		nBlockEmbedding_to_nH[bT], eBlockEmbedding_to_eH[bT],
		nH_to_nBlockEmbedding[bT], eH_to_eBlockEmbedding[bT]);
	planarEmbed(blockG[bT]);
	nodeLength[bT].init(blockG[bT], 0);

	if(!useExtendedDepthDefinition())
	{
		if (blockG[bT].numberOfEdges() == 1)
		{
			node n;
			forall_nodes(n, blockG[bT])
			{
				node nOrg = pBCTree->original(nBlockEmbedding_to_nH[bT][n]);
				if (nOrg->degree() == 1)
					oneEdgeBlockNodes.pushBack(nOrg);
			}
		}
	}
}


void EmbedderMinDepthPiTa::embedCutVertex(const node& vT, bool root /*= false*/)
{
	node vTp = nBCTree_to_npBCTree[vT];
	nG_nT_to_nPG[vTp].init(G_nT[vTp]);
	nPG_to_nG_nT[vTp].init(pBCTree->originalGraph());
	eG_nT_to_ePG[vTp].init(G_nT[vTp]);
	ePG_to_eG_nT[vTp].init(pBCTree->originalGraph());

	node vG_nT = G_nT[vTp].newNode();
	node adj_bT = vT->firstAdj()->twinNode();
	node vH = pBCTree->cutVertex(vTp, nBCTree_to_npBCTree[adj_bT]);
	node vG = pBCTree->original(vH);
	nG_nT_to_nPG[vTp][vG_nT] = vG;
	nPG_to_nG_nT[vTp][vG] = vG_nT;
	Gamma_adjExt_nT[vTp] = 0;

	//add Gamma(b) of children b of vT into Gamma(vT):
	edge e_vT_bT;
	forall_adj_edges(e_vT_bT, vT)
	{
		if (e_vT_bT->target() != vT)
			continue;

		node bT = e_vT_bT->source();
		node bTp = nBCTree_to_npBCTree[bT];
		//node vH = pBCTree->cutVertex(vTp, bTp);
		if (bT->indeg() == 0) //leaf
		{
			//Let \Gamma(B) be the prescribed embedding of block B, with external face
			//equal to one of the candidate cutfaces of (B, v).
			nG_nT_to_nPG[bTp].init(G_nT[bTp]);
			eG_nT_to_ePG[bTp].init(G_nT[bTp]);
			nPG_to_nG_nT[bTp].init(pBCTree->originalGraph());
			ePG_to_eG_nT[bTp].init(pBCTree->originalGraph());

			node vBG;
			forall_nodes(vBG, blockG[bTp])
			{
				node noG = pBCTree->original(nBlockEmbedding_to_nH[bTp][vBG]);
				node noG_bT = G_nT[bTp].newNode();
				nG_nT_to_nPG[bTp][noG_bT] = noG;
				nPG_to_nG_nT[bTp][noG] = noG_bT;
			}

			edge eBG;
			forall_edges(eBG, blockG[bTp])
			{
				edge edG = pBCTree->original(eBlockEmbedding_to_eH[bTp][eBG]);
				node edG_bT_source = nPG_to_nG_nT[bTp][edG->source()];
				node edG_bT_target = nPG_to_nG_nT[bTp][edG->target()];
				edge edG_bT = G_nT[bTp].newEdge(edG_bT_source, edG_bT_target);
				ePG_to_eG_nT[bTp][edG] = edG_bT;
				eG_nT_to_ePG[bTp][edG_bT] = edG;
			}

			Gamma_adjExt_nT[bTp] = nPG_to_nG_nT[bTp][vG]->firstAdj();

			//copy adjacency entry orderings:
			NodeArray< List<adjEntry> > newOrder_G_bTp(G_nT[bTp]);
			node nB;
			forall_nodes(nB, blockG[bTp])
			{
				node nG = pBCTree->original(nBlockEmbedding_to_nH[bTp][nB]);
				ListIterator<adjEntry> after;
				for (adjEntry aeNode = nB->firstAdj(); aeNode; aeNode = aeNode->succ())
				{
					edge eG = pBCTree->original(eBlockEmbedding_to_eH[bTp][aeNode->theEdge()]);
					edge eG_bT = ePG_to_eG_nT[bTp][eG];
					node nG_bT = nPG_to_nG_nT[bTp][nG];
					if (nG == eG->source())
					{
						if (!after.valid())
							after = newOrder_G_bTp[nG_bT].pushBack(eG_bT->adjSource());
						else
							after = newOrder_G_bTp[nG_bT].insertAfter(eG_bT->adjSource(), after);
					}
					else //!(nG == eG->source())
					{
						if (!after.valid())
							after = newOrder_G_bTp[nG_bT].pushBack(eG_bT->adjTarget());
						else
							after = newOrder_G_bTp[nG_bT].insertAfter(eG_bT->adjTarget(), after);
					}
				} //for (adjEntry aeNode = nB->firstAdj(); aeNode; aeNode = aeNode->succ())
			} //forall_nodes(nB, blockG[bTp])

			forall_nodes(nB, G_nT[bTp])
				G_nT[bTp].sort(nB, newOrder_G_bTp[nB]);
		}
		else //if not leaf
		{
			//Let \Gamma(B) = embed(B)
			embedBlockVertex(bT, vT);
		}

		//add all nodes and edges of Gamma(bT) to Gamma(vT):
		node n_Gamma_bT;
		forall_nodes(n_Gamma_bT, G_nT[bTp])
		{
			node nPG = nG_nT_to_nPG[bTp][n_Gamma_bT];
			if (nPG != vG)
			{
				node n_G_vT = G_nT[vTp].newNode();
				nG_nT_to_nPG[vTp][n_G_vT] = nPG;
				nPG_to_nG_nT[vTp][nPG] = n_G_vT;
			}
		}
		edge e_Gamma_bT;
		forall_edges(e_Gamma_bT, G_nT[bTp])
		{
			edge ePG = eG_nT_to_ePG[bTp][e_Gamma_bT];
			node n_G_vT_source = nPG_to_nG_nT[vTp][ePG->source()];
			node n_G_vT_target = nPG_to_nG_nT[vTp][ePG->target()];
			edge e_G_vT = G_nT[vTp].newEdge(n_G_vT_source, n_G_vT_target);
			eG_nT_to_ePG[vTp][e_G_vT] = ePG;
			ePG_to_eG_nT[vTp][ePG] = e_G_vT;
		}

		//set adjacency entry of external face for Gamma(vT), if not already assigned:
		if (Gamma_adjExt_nT[vTp] == 0)
		{
			// TESTCODE
			// BUG (actually occurs below): graph of v is wrong
			//node v = Gamma_adjExt_nT[bTp]->theNode(); // graphOf: ???
			//NodeArray<node> &na = nG_nT_to_nPG[bTp];  // graphOf: G_nT[bTp]
			//node xxx = na[v];  // BUG
			// END TESTCODE

			node nodeG = nG_nT_to_nPG[bTp][Gamma_adjExt_nT[bTp]->theNode()];
			node nodeG_vT = nPG_to_nG_nT[vTp][nodeG];
			node twinG = nG_nT_to_nPG[bTp][Gamma_adjExt_nT[bTp]->twinNode()];
			node twinG_vT = nPG_to_nG_nT[vTp][twinG];
			adjEntry ae;
			forall_adj(ae, nodeG_vT)
			{
				if (ae->twinNode() == twinG_vT)
				{
					Gamma_adjExt_nT[vTp] = ae;
					break;
				}
			}
		}

		if (root)
		{
			//set adjacency entry of external face for G, if not already assigned:
			if (tmpAdjExtFace == 0)
			{
				node nodeG = nG_nT_to_nPG[bTp][Gamma_adjExt_nT[bTp]->theNode()];
				node twinG = nG_nT_to_nPG[bTp][Gamma_adjExt_nT[bTp]->twinNode()];
				adjEntry ae;
				forall_adj(ae, nodeG)
				{
					if (ae->twinNode() == twinG)
					{
						tmpAdjExtFace = ae->twin();
						break;
					}
				}
			}
		}
	} //forall_adj_edges(e_vT_bT, vT)

	ListIterator<adjEntry> after;
	NodeArray< List<adjEntry> > newOrder_G_vT(G_nT[vTp]);
	{
		forall_adj_edges(e_vT_bT, vT)
		{
			if (e_vT_bT->target() != vT)
				continue;

			node bT = e_vT_bT->source();
			node bTp = nBCTree_to_npBCTree[bT];
			//node vH = pBCTree->cutVertex(vTp, bTp);

			//compute new order:
			node nB;
			forall_nodes(nB, G_nT[bTp])
			{
				node nG = nG_nT_to_nPG[bTp][nB];
				adjEntry ae = nB->firstAdj();
				ListIterator<adjEntry>* pAfter;
				if (nG == vG)
				{
					pAfter = &after;

					//find adjacency entry of nB which lies on external face, if it exists:
					adjEntry aeFace = Gamma_adjExt_nT[bTp];
					do
					{
						if (aeFace->theNode() == nB)
						{
							if (aeFace->succ())
								ae = aeFace->succ();
							else
								ae = nB->firstAdj();
							break;
						}
						aeFace = aeFace->faceCycleSucc();
					} while(aeFace != Gamma_adjExt_nT[bTp]);
				}
				else
					pAfter = OGDF_NEW ListIterator<adjEntry>();

				//embed all edges of Gamma(B):
				bool after_ae = true;
				for (adjEntry aeNode = ae;
						 after_ae || aeNode != ae;
						 after_ae = (!after_ae || !aeNode->succ()) ? false : true,
							 aeNode = aeNode->succ() ? aeNode->succ() : nB->firstAdj())
				{
					edge eG = eG_nT_to_ePG[bTp][aeNode->theEdge()];
					edge eG_vT = ePG_to_eG_nT[vTp][eG];
					node nG_vT = nPG_to_nG_nT[vTp][nG];
					if (nG == eG->source())
					{
						if (!pAfter->valid())
							*pAfter = newOrder_G_vT[nG_vT].pushBack(eG_vT->adjSource());
						else
							*pAfter = newOrder_G_vT[nG_vT].insertAfter(eG_vT->adjSource(), *pAfter);
					}
					else //!(nG == eG->source())
					{
						if (!pAfter->valid())
							*pAfter = newOrder_G_vT[nG_vT].pushBack(eG_vT->adjTarget());
						else
							*pAfter = newOrder_G_vT[nG_vT].insertAfter(eG_vT->adjTarget(), *pAfter);
					}
				} //for (adjEntry aeNode = ae; ...

				if (nG != vG)
					delete pAfter;
			} //forall_nodes(nB, G_nT[bTp])
		} //forall_adj_edges(e_vT_bT, vT)
	}

	//apply new order:
	node n_G_vT;
	forall_nodes(n_G_vT, G_nT[vTp])
		G_nT[vTp].sort(n_G_vT, newOrder_G_vT[n_G_vT]);

	if (root)
	{
		node n;
		forall_nodes(n, pBCTree->originalGraph())
		{
			newOrder[n].clear();
			node nG_vT = nPG_to_nG_nT[vTp][n];
			for (ListIterator<adjEntry> it = newOrder_G_vT[nG_vT].begin(); it.valid(); it++)
			{
				node twinPGnode = nG_nT_to_nPG[vTp][(*it)->twinNode()];
				adjEntry ae_n;
				forall_adj(ae_n, n)
				{
					if (ae_n->twinNode() == twinPGnode)
					{
						newOrder[n].pushBack(ae_n);
						break;
					}
				}
			}
		}
	}
}


void EmbedderMinDepthPiTa::embedBlockVertex(const node& bT, const node& parent_cT)
{
	node bTp = nBCTree_to_npBCTree[bT];

	//compute Gamma(v) for all children of bT in the BC-tree:
	edge e_bT_to_cT;
	forall_adj_edges(e_bT_to_cT, bT)
	{
		if (e_bT_to_cT->target() != bT)
			continue;

		node cT = e_bT_to_cT->source();
		embedCutVertex(cT);
	}

	//compute all candidate cutfaces of (bT, cT):
	List<face> candidateCutfaces;
	CombinatorialEmbedding CE(blockG[bTp]);
	node nParentH = pBCTree->cutVertex(nBCTree_to_npBCTree[parent_cT], bTp);
	node parent_cB = nH_to_nBlockEmbedding[bTp][nParentH];
	adjEntry ae_parent_cB;
	forall_adj(ae_parent_cB, parent_cB)
	{
		face lf = CE.leftFace(ae_parent_cB);
		if (candidateCutfaces.search(lf) == -1)
			candidateCutfaces.pushBack(lf);

		face rf = CE.rightFace(ae_parent_cB);
		if (candidateCutfaces.search(rf) == -1)
			candidateCutfaces.pushBack(rf);
	}

	//For all candidate cutfaces f of (bT, cT) compute
	//delta(f) = max_{v \in T, v \in f} (depth(Gamma(v)))
	//and f_B = face with max delta(f) and maximum number of cutvertices with deepest embedding.
	face f_B = *(candidateCutfaces.begin());
	int max_delta_f = 0;
	for (ListIterator<face> it_f = candidateCutfaces.begin(); it_f.valid(); it_f++)
	{
		face f = *it_f;
		int maxDepth = 0;
		adjEntry ae_f = f->firstAdj();
		do
		{
			node nB = ae_f->theNode();
			node nH = nBlockEmbedding_to_nH[bTp][nB];
			node nG = pBCTree->original(nH);
			if (pBCTree->typeOfGNode(nG) == BCTree::CutVertex)
			{
				node nTp = pBCTree->bcproper(nG);
				node nT = npBCTree_to_nBCTree[nTp];
				if (nT != parent_cT)
				{
					int depth_nT = depthCutvertex(nT);
					if (depth_nT > maxDepth)
					{
						maxDepth = depth_nT;
					}
				}
			}

			ae_f = ae_f->faceCycleSucc();
		} while(ae_f != f->firstAdj());

		if (maxDepth > max_delta_f)
		{
			f_B = f;
			max_delta_f = maxDepth;
		}
	}

	//embed all cutvertices incident to f_B into f_B and all other cutvertices
	//into an arbitrary cutface:
	Gamma_adjExt_nT[bTp] = f_B->firstAdj();

	//G_nT[bT] = blockG[bT]:
	nG_nT_to_nPG[bTp].init(G_nT[bTp]);
	nPG_to_nG_nT[bTp].init(pBCTree->originalGraph());
	eG_nT_to_ePG[bTp].init(G_nT[bTp]);
	ePG_to_eG_nT[bTp].init(pBCTree->originalGraph());
	node n_blockG_bT;
	forall_nodes(n_blockG_bT, blockG[bTp])
	{
		node nH = nBlockEmbedding_to_nH[bTp][n_blockG_bT];
		node nPG = pBCTree->original(nH);
		node n_G_bT = G_nT[bTp].newNode();
		nG_nT_to_nPG[bTp][n_G_bT] = nPG;
		nPG_to_nG_nT[bTp][nPG] = n_G_bT;
	}
	edge e_blockG_bT;
	forall_edges(e_blockG_bT, blockG[bTp])
	{
		edge eH = eBlockEmbedding_to_eH[bTp][e_blockG_bT];
		edge ePG = pBCTree->original(eH);
		node n_G_bT_source = nPG_to_nG_nT[bTp][ePG->source()];
		node n_G_bT_target = nPG_to_nG_nT[bTp][ePG->target()];
		edge e_G_bT = G_nT[bTp].newEdge(n_G_bT_source, n_G_bT_target);
		eG_nT_to_ePG[bTp][e_G_bT] = ePG;
		ePG_to_eG_nT[bTp][ePG] = e_G_bT;
	}

	//add nodes and edges of Gamma(cT) for all children cT of bT:
	{
		forall_adj_edges(e_bT_to_cT, bT)
		{
			if (e_bT_to_cT->target() != bT)
				continue;

			node cT = e_bT_to_cT->source();
			node cTp = nBCTree_to_npBCTree[cT];
			node cPG = pBCTree->original(pBCTree->cutVertex(cTp, bTp));
			node n_G_cT;
			forall_nodes(n_G_cT, G_nT[cTp])
			{
				node nPG = nG_nT_to_nPG[cTp][n_G_cT];
				if (nPG != cPG)
				{
					node n_G_bT = G_nT[bTp].newNode();
					nG_nT_to_nPG[bTp][n_G_bT] = nPG;
					nPG_to_nG_nT[bTp][nPG] = n_G_bT;
				}
			}
			edge e_G_cT;
			forall_edges(e_G_cT, G_nT[cTp])
			{
				edge ePG = eG_nT_to_ePG[cTp][e_G_cT];
				node n_G_bT_source = nPG_to_nG_nT[bTp][ePG->source()];
				node n_G_bT_target = nPG_to_nG_nT[bTp][ePG->target()];
				edge e_G_bT = G_nT[bTp].newEdge(n_G_bT_source, n_G_bT_target);
				eG_nT_to_ePG[bTp][e_G_bT] = ePG;
				ePG_to_eG_nT[bTp][ePG] = e_G_bT;
			}
		}
	}

	//compute new order of adjacency edges for all nodes depending on Gamma(v)
	//for all children v of bT and the given embedding for block bT:
	NodeArray< List<adjEntry> > newOrder_bT(G_nT[bTp]);
	forall_nodes(n_blockG_bT, blockG[bTp])
	{
		node nH = nBlockEmbedding_to_nH[bTp][n_blockG_bT];
		node nG = pBCTree->original(nH);
		adjEntry ae = n_blockG_bT->firstAdj();
		ListIterator<adjEntry> after;

		if (pBCTree->typeOfGNode(nG) == BCTree::CutVertex)
		{
			node cTp = pBCTree->bcproper(nG);
			if (cTp != nBCTree_to_npBCTree[parent_cT])
			{
				//find adjacency entry of n_blockG_bT which lies on external face of G_nT[cTp]:
				adjEntry ae_G_cT;
				adjEntry aeFace = Gamma_adjExt_nT[cTp];
				do
				{
					if (nG_nT_to_nPG[cTp][aeFace->theNode()] == nG)
					{
						if (aeFace->succ())
							ae_G_cT = aeFace->succ();
						else
							ae_G_cT = aeFace->theNode()->firstAdj();
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != Gamma_adjExt_nT[cTp]);

				//embed all edges of Gamma(cT):
				node n_G_cT;
				forall_nodes(n_G_cT, G_nT[cTp])
				{
					node nG2 = nG_nT_to_nPG[cTp][n_G_cT];

					adjEntry ae;
					ListIterator<adjEntry>* pAfter;
					if (nG2 == nG)
					{
						ae = ae_G_cT;
						pAfter = &after;
					}
					else
					{
						ae = n_G_cT->firstAdj();
						pAfter = OGDF_NEW ListIterator<adjEntry>();
					}

					bool after_ae = true;
					for (adjEntry aeNode = ae;
							 after_ae || aeNode != ae;
							 after_ae = (!after_ae || !aeNode->succ()) ? false : true,
								 aeNode = aeNode->succ() ? aeNode->succ() : n_G_cT->firstAdj())
					{
						edge eG = eG_nT_to_ePG[cTp][aeNode->theEdge()];
						edge e_G_bT = ePG_to_eG_nT[bTp][eG];
						node n_G_bT2 = nPG_to_nG_nT[bTp][nG2];
						if (nG2 == eG->source())
						{
							if (!pAfter->valid())
								*pAfter = newOrder_bT[n_G_bT2].pushBack(e_G_bT->adjSource());
							else
								*pAfter = newOrder_bT[n_G_bT2].insertAfter(e_G_bT->adjSource(), *pAfter);
						}
						else
						{
							if (!pAfter->valid())
								*pAfter = newOrder_bT[n_G_bT2].pushBack(e_G_bT->adjTarget());
							else
								*pAfter = newOrder_bT[n_G_bT2].insertAfter(e_G_bT->adjTarget(), *pAfter);
						}
					} //for (adjEntry aeNode = ae; aeNode; aeNode = aeNode->succ())

					if (nG2 != nG)
						delete pAfter;
				} //forall_nodes(n_G_cT, G_nT[cT])

				//find adjacency entry of n_blockG_bT which lies on face f_B:
				aeFace = f_B->firstAdj();
				do
				{
					if (aeFace->theNode() == n_blockG_bT)
					{
						if (aeFace->succ())
							ae = aeFace->succ();
						else
							ae = n_blockG_bT->firstAdj();
						break;
					}
					aeFace = aeFace->faceCycleSucc();
				} while(aeFace != f_B->firstAdj());
			} //if (cT != parent_cT)
		} //if (pBCTree->typeOfGNode(nG) == BCTree::CutVertex)

		//embed all edges of block bT:
		bool after_ae = true;
		for (adjEntry aeNode = ae;
			after_ae || aeNode != ae;
			after_ae = (!after_ae || !aeNode->succ()) ? false : true,
			aeNode = aeNode->succ() ? aeNode->succ() : n_blockG_bT->firstAdj())
		{
			edge eG = pBCTree->original(eBlockEmbedding_to_eH[bTp][aeNode->theEdge()]);
			edge e_G_bT = ePG_to_eG_nT[bTp][eG];
			node n_G_bT = nPG_to_nG_nT[bTp][nG];
			if (nG == eG->source())
			{
				if (!after.valid())
					after = newOrder_bT[n_G_bT].pushBack(e_G_bT->adjSource());
				else
					after = newOrder_bT[n_G_bT].insertAfter(e_G_bT->adjSource(), after);
			}
			else
			{
				if (!after.valid())
					after = newOrder_bT[n_G_bT].pushBack(e_G_bT->adjTarget());
				else
					after = newOrder_bT[n_G_bT].insertAfter(e_G_bT->adjTarget(), after);
			}
		} //for (adjEntry aeNode = ae; aeNode; aeNode = aeNode->succ())
	} //forall_nodes(nSG, blockG[bT])

	//apply new order:
	node n_G_bT;
	forall_nodes(n_G_bT, G_nT[bTp])
		G_nT[bTp].sort(n_G_bT, newOrder_bT[n_G_bT]);
}


int EmbedderMinDepthPiTa::depthBlock(const node& bT/*, const node& parent_cT*/)
{
	node bTp = nBCTree_to_npBCTree[bT];
	//node parent_cTp = nBCTree_to_npBCTree[parent_cT];
	//node parent_cH = pBCTree->cutVertex(parent_cTp, bTp);
	//node parent_cPG = pBCTree->original(parent_cH);
	//node parent_cG_nT = nPG_to_nG_nT[bTp][parent_cPG];

	int dP = 0;
	int dNP = 0;

	//compute dP = max_{v incident to f_B} depth(Gamma(v)), f_B = extFace, and
	//dNP = 2 + max_{v not incident to f_B} depth(Gamma(v)):
	int maxDepth_dP = 0;
	int maxDepth_dNP = 0;
	edge e_bT_cT;
	forall_adj_edges(e_bT_cT, bT)
	{
		if (e_bT_cT->target() != bT)
			continue;

		node cT = e_bT_cT->source();
		node cTp = nBCTree_to_npBCTree[cT];
		node cH = pBCTree->cutVertex(cTp, bTp);
		node cPG = pBCTree->original(cH);
		node cG_nT = nPG_to_nG_nT[bTp][cPG];

		bool v_incident_to_fB = false;
		adjEntry ae = Gamma_adjExt_nT[bTp];
		do
		{
			if (ae->theNode() == cG_nT)
			{
				v_incident_to_fB = true;
				break;
			}
			ae = ae->faceCycleSucc();
		} while (ae != Gamma_adjExt_nT[bTp]);

		int depth_Gamma_cT = depthCutvertex(cT);
		if (v_incident_to_fB)
		{
			if (depth_Gamma_cT > maxDepth_dP)
				maxDepth_dP = depth_Gamma_cT;
		}
		else
		{
			if (depth_Gamma_cT > maxDepth_dNP)
				maxDepth_dNP = depth_Gamma_cT;
		}
	}

	if (dP > 2 + dNP)
		return dP;
	//else:
	return 2 + dNP;
}


int EmbedderMinDepthPiTa::depthCutvertex(const node& cT)
{
	//return max_{B \in children(v)} depth(Gamma(B))
	int maxDepth = 0;

	edge e_cT_bT;
	forall_adj_edges(e_cT_bT, cT)
	{
		if (e_cT_bT->target() != cT)
			continue;

		node bT = e_cT_bT->source();
		int thisDepth = depthBlock(bT/*, cT*/);
		if (thisDepth > maxDepth)
			maxDepth = thisDepth;
	}

	return maxDepth;
}


void EmbedderMinDepthPiTa::deleteDummyNodes(Graph& G, adjEntry& adjExternal)
{
	if(!useExtendedDepthDefinition())
		return;

	node adjExtNode = adjExternal->theNode();
	node adjExtTwinNode = adjExternal->twinNode();
	if (dummyNodes.search(adjExtNode) != -1)
	{
		adjEntry succ = adjExternal->succ();
		if (!succ)
			succ = adjExtNode->firstAdj();
		node succTwinNode = succ->twinNode();

		//find edge between adjExtTwinNode and succTwinNode:
		adjEntry ae;
		forall_adj(ae, adjExtTwinNode)
		{
			if (ae->twinNode() == succTwinNode)
			{
				adjExternal = ae;
				break;
			}
		}
	}
	else if (dummyNodes.search(adjExtTwinNode) != -1)
	{
		adjEntry succ = adjExternal->twin()->succ();
		if (!succ)
			succ = adjExtTwinNode->firstAdj();
		node succTwinNode = succ->twinNode();

		//find edge between adjExtNode and succTwinNode:
		adjEntry ae;
		forall_adj(ae, adjExtNode)
		{
			if (ae->twinNode() == succTwinNode)
			{
				adjExternal = ae;
				break;
			}
		}
	}

	for (ListIterator<node> it = dummyNodes.begin(); it.valid(); it++)
		G.delNode(*it);
}

} // end namespace ogdf
