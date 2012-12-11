/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class PlanarSPQRTree
 *
 * \author Carsten Gutwenger
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


#include <ogdf/decomposition/PlanarSPQRTree.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf {

//-------------------------------------------------------------------
//                           PlanarSPQRTree
//-------------------------------------------------------------------

//
// initialization: additionally embeds skeleton graphs or adpots embedding
// given by original graph
void PlanarSPQRTree::init(bool isEmbedded)
{
	if (isEmbedded) {
		adoptEmbedding();

	} else {

		node v;
		forall_nodes(v,tree())
			planarEmbed(skeleton(v).getGraph());
	}
}


void PlanarSPQRTree::adoptEmbedding()
{
	OGDF_ASSERT_IF(dlExtendedChecking, originalGraph().representsCombEmbedding());

	// ordered list of adjacency entries (for one original node) in all
	// skeletons (where this node occurs)
	NodeArray<SListPure<adjEntry> > adjEdges(tree());
	// copy in skeleton of current original node
	NodeArray<node> currentCopy(tree(),0);
	NodeArray<adjEntry> lastAdj(tree(),0);
	SListPure<node> current; // currently processed nodes

	node vOrig;
	forall_nodes(vOrig,originalGraph())
	{
		adjEntry adjOrig;
		forall_adj(adjOrig,vOrig)
		{
			edge            eOrig = adjOrig->theEdge();
			const Skeleton &S     = skeletonOfReal(eOrig);
			edge            eCopy = copyOfReal(eOrig);

			adjEntry adjCopy = (S.original(eCopy->source()) == vOrig) ?
				eCopy->adjSource() : eCopy->adjTarget();

			setPosInEmbedding(adjEdges,currentCopy,lastAdj,current,S,adjCopy);
		}

		SListConstIterator<node> it;
		for(it = current.begin(); it.valid(); ++it) {
			node vT = *it;

			skeleton(vT).getGraph().sort(currentCopy[vT],adjEdges[vT]);

			adjEdges[vT].clear();
			currentCopy[vT] = 0;
		}

		current.clear();
	}
}


void PlanarSPQRTree::setPosInEmbedding(
	NodeArray<SListPure<adjEntry> > &adjEdges,
	NodeArray<node> &currentCopy,
	NodeArray<adjEntry> &lastAdj,
	SListPure<node> &current,
	const Skeleton &S,
	adjEntry adj)
{
	node vT = S.treeNode();

	adjEdges[vT].pushBack(adj);

	node vCopy = adj->theNode();
	node vOrig = S.original(vCopy);

	if(currentCopy[vT] == 0) {
		currentCopy[vT] = vCopy;
		current.pushBack(vT);

		adjEntry adjVirt;
		forall_adj(adjVirt,vCopy) {
			edge eCopy = S.twinEdge(adjVirt->theEdge());
			if (eCopy == 0) continue;
			if (adjVirt == adj) {
				lastAdj[vT] = adj;
				continue;
			}

			const Skeleton &STwin = skeleton(S.twinTreeNode(adjVirt->theEdge()));

			adjEntry adjCopy = (STwin.original(eCopy->source()) == vOrig) ?
				eCopy->adjSource() : eCopy->adjTarget();

			setPosInEmbedding(adjEdges,currentCopy,lastAdj,current,
				STwin, adjCopy);
		}

	} else if (lastAdj[vT] != 0 && lastAdj[vT] != adj) {
		adjEntry adjVirt = lastAdj[vT];
		edge eCopy = S.twinEdge(adjVirt->theEdge());

		const Skeleton &STwin = skeleton(S.twinTreeNode(adjVirt->theEdge()));

		adjEntry adjCopy = (STwin.original(eCopy->source()) == vOrig) ?
			eCopy->adjSource() : eCopy->adjTarget();

		setPosInEmbedding(adjEdges,currentCopy,lastAdj,current,
			STwin, adjCopy);

		lastAdj[vT] = 0;
	}

}


//
// embed original graph according to embedding of skeletons
//
// The procedure also handles the case when some (real or virtual)
// edges are reversed (used in upward-planarity algorithms)
void PlanarSPQRTree::embed(Graph &G)
{
	OGDF_ASSERT(&G == &originalGraph());

	const Skeleton &S = skeleton(rootNode());
	const Graph &M = S.getGraph();

	node v;
	forall_nodes(v,M)
	{
		node vOrig = S.original(v);
		SListPure<adjEntry> adjEdges;

		adjEntry adj;
		forall_adj(adj,v) {
			edge e = adj->theEdge();
			edge eOrig = S.realEdge(e);

			if (eOrig != 0) {
				adjEntry adjOrig = (vOrig == eOrig->source()) ?
					eOrig->adjSource() : eOrig->adjTarget();
				OGDF_ASSERT(adjOrig->theNode() == S.original(v));
				adjEdges.pushBack(adjOrig);

			} else {
				node wT    = S.twinTreeNode(e);
				edge eTwin = S.twinEdge(e);
				expandVirtualEmbed(wT,
					(vOrig == skeleton(wT).original(eTwin->source())) ?
					eTwin->adjSource() : eTwin->adjTarget(),
					adjEdges);
			}
		}

		G.sort(vOrig,adjEdges);
	}

	edge e;
	forall_adj_edges(e,rootNode()) {
		node wT = e->target();
		if (wT != rootNode())
			createInnerVerticesEmbed(G, wT);
	}
}


void PlanarSPQRTree::expandVirtualEmbed(node vT,
	adjEntry adjVirt,
	SListPure<adjEntry> &adjEdges)
{
	const Skeleton &S = skeleton(vT);

	node v = adjVirt->theNode();
	node vOrig = S.original(v);

	adjEntry adj;
	for (adj = adjVirt->cyclicSucc(); adj != adjVirt; adj = adj->cyclicSucc())
	{
		edge e = adj->theEdge();
		edge eOrig = S.realEdge(e);

		if (eOrig != 0) {
			adjEntry adjOrig = (vOrig == eOrig->source()) ?
				eOrig->adjSource() : eOrig->adjTarget();
			OGDF_ASSERT(adjOrig->theNode() == S.original(v));
			adjEdges.pushBack(adjOrig);

		} else {
			node wT    = S.twinTreeNode(e);
			edge eTwin = S.twinEdge(e);
			expandVirtualEmbed(wT,
				(vOrig == skeleton(wT).original(eTwin->source())) ?
				eTwin->adjSource() : eTwin->adjTarget(),
				adjEdges);
		}
	}
}


void PlanarSPQRTree::createInnerVerticesEmbed(Graph &G, node vT)
{
	const Skeleton &S = skeleton(vT);
	const Graph& M = S.getGraph();

	node src = S.referenceEdge()->source();
	node tgt = S.referenceEdge()->target();

	node v;
	forall_nodes(v,M)
	{
		if (v == src || v == tgt) continue;

		node vOrig = S.original(v);
		SListPure<adjEntry> adjEdges;

		adjEntry adj;
		forall_adj(adj,v) {
			edge e = adj->theEdge();
			edge eOrig = S.realEdge(e);

			if (eOrig != 0) {
				adjEntry adjOrig = (vOrig == eOrig->source()) ?
					eOrig->adjSource() : eOrig->adjTarget();
				OGDF_ASSERT(adjOrig->theNode() == S.original(v));
				adjEdges.pushBack(adjOrig);
			} else {
				node wT    = S.twinTreeNode(e);
				edge eTwin = S.twinEdge(e);
				expandVirtualEmbed(wT,
					(vOrig == skeleton(wT).original(eTwin->source())) ?
					eTwin->adjSource() : eTwin->adjTarget(),
					adjEdges);
			}
		}

		G.sort(vOrig,adjEdges);
	}

	edge e;
	forall_adj_edges(e,vT) {
		node wT = e->target();
		if (wT != vT)
			createInnerVerticesEmbed(G, wT);
	}
}



//
// basic update functions for manipulating embeddings

//   reversing the skeleton of an R- or P-node
void PlanarSPQRTree::reverse(node vT)
{
	skeleton(vT).getGraph().reverseAdjEdges();
}


//   swapping two adjacency entries in the skeleton of a P-node
void PlanarSPQRTree::swap(node vT, adjEntry adj1, adjEntry adj2)
{
	OGDF_ASSERT(typeOf(vT) == PNode);

	Graph &M = skeleton(vT).getGraph();

	M.swapAdjEdges(adj1,adj2);
	M.swapAdjEdges(adj1->twin(),adj2->twin());
}


//   swapping two edges in the skeleton of a P-node
void PlanarSPQRTree::swap(node vT, edge e1, edge e2)
{
	OGDF_ASSERT(typeOf(vT) == PNode);

	if (e1->source() == e2->source())
		swap(vT,e1->adjSource(),e2->adjSource());
	else
		swap(vT,e1->adjSource(),e2->adjTarget());
}


//
// number of possible embeddings of original graph
//
double PlanarSPQRTree::numberOfEmbeddings(node vT) const
{
	double num = 1.0;

	switch(typeOf(vT)) {
	case RNode:
		num = 2; break;
	case PNode:
		//node vFirst = skeleton(vT).getGraph().firstNode();
		for (int i = skeleton(vT).getGraph().firstNode()->degree()-1; i >= 2; --i)
			num *= i;
		break;
	case SNode:
		break;
	}

	edge e;
	forall_adj_edges(e,vT) {
		node wT = e->target();
		if(wT != vT)
			num *= numberOfEmbeddings(wT);
	}

	return num;
}



//
// randomly embed skeleton graphs
//
void PlanarSPQRTree::randomEmbed()
{
	node vT;
	forall_nodes(vT,tree()) {
		if (typeOf(vT) == RNode) {
			int doReverse = randomNumber(0,1);

			if (doReverse == 1)
				reverse(vT);

		} else if (typeOf(vT) == PNode) {
			const Skeleton &S = skeleton(vT);
			adjEntry adjRef = S.referenceEdge()->adjSource();

			SList<adjEntry> adjEdges;
			adjEntry adj;
			for (adj = adjRef->cyclicSucc(); adj != adjRef; adj = adj->cyclicSucc())
				adjEdges.pushBack(adj);

			adjEdges.permute();

			adj = adjRef->cyclicSucc();
			SListConstIterator<adjEntry> it;
			for (it = adjEdges.begin(); it.valid(); ++it)
			{
				adjEntry adjNext = *it;
				if (adjNext != adj) {
					swap(vT,adj,adjNext);
					adj = adjNext;
				}
				adj = adj->cyclicSucc();
			}
		}
	}
}


} // end namespace ogdf
