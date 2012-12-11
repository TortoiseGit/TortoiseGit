/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class CPlanarSubClusteredGraph.
 * Constructs a c-planar subclustered graph of the input on
 * base of a spanning tree.
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

#include <ogdf/cluster/CPlanarSubClusteredGraph.h>
#include <ogdf/cluster/CconnectClusterPlanar.h>


namespace ogdf {

//precondition: graph is c-connected
void CPlanarSubClusteredGraph::call(const ClusterGraph &CG,
									EdgeArray<bool>& inSub) //original edges in subgraph?

{
	List<edge> leftOver;//original edges not in subgraph
	call(CG, inSub, leftOver);

}//call
//precondition: graph is c-connected
void CPlanarSubClusteredGraph::call(const ClusterGraph &CGO,
									EdgeArray<bool>& inSub, //original edges in subgraph?
									List<edge>& leftOver)//original edges not in subgraph
{
	EdgeArray<double> weightDummy;
	call(CGO, inSub, leftOver, weightDummy);
}

void CPlanarSubClusteredGraph::call(const ClusterGraph &CGO,
									EdgeArray<bool>& inSub, //original edges in subgraph?
									List<edge>& leftOver,   //original edges not in subgraph
									EdgeArray<double>& edgeWeight) //prefer lightweight edges
{
	leftOver.clear();

	//we compute a c-planar subclustered graph by calling
	//CPlanarSubClusteredST and then perform reinsertion on
	//a copy of the computed subclustered graph
	//initialize "call-global" info arrays
	//edge status
	const Graph& origG = CGO.getGraph();
	m_edgeStatus.init(origG, 0);

	CPlanarSubClusteredST CPST;
	if (edgeWeight.valid())
		CPST.call(CGO, inSub, edgeWeight);
	else
		CPST.call(CGO, inSub);

	//now construct the copy
	//we should create a clusterGraph copy function that
	//builds a clustergraph upon a subgraph of the
	//original graph, preliminarily use fullcopy and delete edges

	ClusterArray<cluster> clusterCopy(CGO);
	NodeArray<node> nodeCopy(origG);
	EdgeArray<edge> edgeCopy(origG);
	Graph testG;
	ClusterGraph CG(CGO, testG, clusterCopy, nodeCopy, edgeCopy);

	CconnectClusterPlanar CCCP;

	//-------------------------------------
	//perform reinsertion of leftover edges
	//fill list of uninserted edges

	EdgeArray<bool> visited(origG,false);

	//delete the non-ST edges
	edge e;
	forall_edges(e, origG)
	{
		if (!inSub[e])
		{
			leftOver.pushBack(e); //original edges
			testG.delEdge(edgeCopy[e]);
		}//if
	}//foralledges

	//todo: cope with preferred edges
	//simple reinsertion strategy: just iterate over list and test
	ListIterator<edge> itE = leftOver.begin();
	while (itE.valid())
	{
		//testG=CG.getGraph()
		edge newCopy = testG.newEdge(nodeCopy[(*itE)->source()],
											 nodeCopy[(*itE)->target()]);
		edgeCopy[*itE] = newCopy;

		bool cplanar = CCCP.call(CG);


		if (!cplanar)
		{
			testG.delEdge(newCopy);
			itE++;
		}//if
		else
		{
			ListIterator<edge> itDel = itE;
			itE++;
			leftOver.del(itDel);
		}
	}//while

	/*
	ListConstIterator<edge> it;
	for(it = preferedEdges.begin(); it.valid(); ++it)
	{
		edge eG = *it;
		visited[eG] = true;

		edge eH = testG.newEdge(toTestG[eG->source()],toTestG[eG->target()]);

		if (preferedImplyPlanar == false && isPlanar(H) == false) {
			testG.delEdge(eH);
			delEdges.pushBack(eG);
		}
	}
	*/


}//call

}//end namespace ogdf
