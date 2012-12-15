/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class CPlanarSubClusteredST.
 * Computes a (c-connected) spanning tree of a c-connected graph.
 *
 * \author Karsten Klein
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

 #include <ogdf/internal/cluster/CPlanarSubClusteredST.h>

#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>

//debug
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

void CPlanarSubClusteredST::initialize(const ClusterGraph& CG)
{
	//initialize "call-global" info arrays
	m_allocCluster.init(CG.getGraph(), 0);
	//edge status
	//m_edgeStatus.init(CG.getGraph(), 0);
	//edge to rep edge
	m_repEdge.init(CG.getGraph(), 0);
	//nodes and clusters to rep nodes
	m_cRepNode.init(CG, 0);
	m_vRepNode.init(CG.getGraph(), 0);
}

void CPlanarSubClusteredST::call(const ClusterGraph &CG, EdgeArray<bool>& inST)
{
	initialize(CG);
	inST.fill(false);

	//representationsgraphs for every cluster, on clustergraph
	ClusterArray<Graph*> l_clusterRepGraph(CG, 0);
	computeRepresentationGraphs(CG, l_clusterRepGraph);

	//now we compute the spanning trees on the representation graphs
	//we should save the selection info on the original edge
	//are statically on the repgraphedges (we only have edge -> repedge
	//information) but
	ClusterArray< EdgeArray<bool> > l_inTree(CG);

	cluster c;
	forall_clusters(c, CG)
	{
		l_inTree[c].init(*l_clusterRepGraph[c], false);
		//compute STs
		NodeArray<bool> visited(*l_clusterRepGraph[c], false);
		dfsBuildSpanningTree(l_clusterRepGraph[c]->firstNode(),
							 l_inTree[c],
							 visited);
	}//forallclusters

	OGDF_ASSERT(isConnected(CG.getGraph()));

	//compute the subclustered graph by constructing a spanning tree
	//using only the representation edges used in STs on the repgraphs
	NodeArray<bool> visited(CG.getGraph(), false);

	dfsBuildOriginalST(CG.getGraph().firstNode(),
					   l_inTree,
					   inST,
					   visited);

	//unregister the edgearrays to avoid destructor failure after
	//representation graph deletion
	forall_clusters(c, CG)
	{
		l_inTree[c].init();
	}

	deleteRepresentationGraphs(CG, l_clusterRepGraph);
}//call

void CPlanarSubClusteredST::call(const ClusterGraph& CG,
		EdgeArray<bool>& inST,
		EdgeArray<double>& weight)
{
	initialize(CG);

	//representationsgraphs for every cluster, on clustergraph
	ClusterArray<Graph*> l_clusterRepGraph(CG, 0);
	computeRepresentationGraphs(CG, l_clusterRepGraph);

	//Now we compute the spanning trees on the representation graphs
	//are statically on the repgraphedges (we only have edge -> repedge
	//information)
	ClusterArray< EdgeArray<bool> > l_inTree(CG);
	//Weight of the representation edges
	ClusterArray< EdgeArray<double> > l_repWeight(CG);
	//Copy the weight
	cluster c;
	edge e;
	forall_clusters(c, CG)
	{
		l_repWeight[c].init(*l_clusterRepGraph[c], 0.0);
	}
	forall_edges(e, CG.getGraph())
	{
		l_repWeight[m_allocCluster[e]][m_repEdge[e]] = weight[e];
	}

	forall_clusters(c, CG)
	{
		l_inTree[c].init(*l_clusterRepGraph[c], false);
		//compute STs
		computeMinST(*l_clusterRepGraph[c], l_repWeight[c],
			l_inTree[c]);
	}//forallclusters

	OGDF_ASSERT(isConnected(CG.getGraph()));

	//Compute the subclustered graph
	forall_edges(e, CG.getGraph())
	{
		if (l_inTree[m_allocCluster[e]][m_repEdge[e]])
			inST[e] = true;
		else inST[e] = false;
	}
#ifdef OGDF_DEBUG
	GraphCopy cg(CG.getGraph());
	forall_edges(e, CG.getGraph())
	{
		if (!inST[e])
			cg.delCopy(cg.copy(e));
	}
	OGDF_ASSERT(isConnected(cg));
	OGDF_ASSERT(cg.numberOfEdges() == cg.numberOfNodes()-1)

#endif
	//unregister the edgearrays to avoid destructor failure after
	//representation graph deletion
	forall_clusters(c, CG)
	{
		l_inTree[c].init();
		l_repWeight[c].init();
	}

	deleteRepresentationGraphs(CG, l_clusterRepGraph);
}//call

//spanning tree on input graph setting edge status and using
//repgraph spanning tree information
void CPlanarSubClusteredST::dfsBuildOriginalST(node v,
	ClusterArray< EdgeArray<bool> > &treeEdges,    //edges in repgraph
	EdgeArray<bool>& inST,                         //original edges
	NodeArray<bool> &visited)
{

	visited[v] = true;

	edge e;
	forall_adj_edges(e,v)
	{
		//no selfloops
		node w = e->opposite(v);
		if (w == v) continue;
		//only repgraph ST edges are allowed
		//we should save the common cluster at the first computation above,
		//otherwise running time m*m*c
		//cluster c1, c2;
		cluster allocCluster = m_allocCluster[e];

		OGDF_ASSERT(allocCluster != 0);

		if (! treeEdges[allocCluster][m_repEdge[e]]) continue;

		//(this part is always connected in original!)

		if(!visited[w]) {
			//treeEdges[e] = true;
			//m_edgeStatus[e] |= 1; //e is in ST
			inST[e] = true;
			dfsBuildOriginalST(w, treeEdges, inST, visited);
		}
	}
}

//we should later provide a minimum st to allow weights on edges
void CPlanarSubClusteredST::dfsBuildSpanningTree(
	node v,
	EdgeArray<bool> &treeEdges,
	NodeArray<bool> &visited)
{
	OGDF_ASSERT(isConnected(*(v->graphOf())));
	visited[v] = true;

	edge e;
	forall_adj_edges(e,v)
	{
		node w = e->opposite(v);
		if(w == v) continue;

		if(!visited[w]) {
			treeEdges[e] = true;
		//	m_genDebug++; //debugonly
			dfsBuildSpanningTree(w,treeEdges,visited);
		}
	}
}


}//end namespace ogdf
