/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class UpwardPlanarSubgraphSimple which computes
 * an upward planar subgraph of a single-source acyclic digraph
 *
 * \author Hoi-Ming Wong
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

#include <ogdf/upward/FeasibleUpwardPlanarSubgraph.h>
#include <ogdf/upward/UpwardPlanarModule.h>
#include <ogdf/upward/FaceSinkGraph.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {


Module::ReturnType FeasibleUpwardPlanarSubgraph::call(
	Graph &G,
	GraphCopy &FUPS,
	adjEntry &extFaceHandle,
	List<edge> &delEdges,
	bool multisources,
	int runs)
{

#ifdef OGDF_DEBUG
	UpwardPlanarModule upMod;
	OGDF_ASSERT(!upMod.upwardPlanarityTest(G));
#endif

	delEdges.clear();

	//current fups, its embedding and the removed edges
	GraphCopy FUPS_cur;
	List<edge> delEdges_cur;

	call(G, FUPS, extFaceHandle, delEdges, multisources);

	for (int i = 1; i < runs; ++i) {
		adjEntry extFaceHandle_cur;
		call(G, FUPS_cur, extFaceHandle_cur, delEdges_cur, multisources);

		// use new result??
		if (delEdges_cur.size() < delEdges.size()) {
			FUPS = FUPS_cur;
			extFaceHandle = FUPS.copy(FUPS_cur.original(extFaceHandle_cur->theEdge()))->adjSource();
			delEdges = delEdges_cur;
		}
	}
	return Module::retFeasible;
}


Module::ReturnType FeasibleUpwardPlanarSubgraph::call(
	const Graph &G,
	GraphCopy &FUPS,
	adjEntry &extFaceHandle,
	List<edge> &delEdges,
	bool multisources)
{
	FUPS = GraphCopy(G);
	delEdges.clear();
	node s_orig;
	hasSingleSource(G, s_orig);
	List<edge> nonTreeEdges_orig;
	getSpanTree(FUPS, nonTreeEdges_orig, true, multisources);
	CombinatorialEmbedding Gamma(FUPS);
	nonTreeEdges_orig.permute(); // random order

	//insert nonTreeEdges
	UpwardPlanarModule upMod;
	while (!nonTreeEdges_orig.empty()) {
		// make identical copy GC of Fups
		//and insert e_orig in GC
		GraphCopy GC = FUPS;
		edge e_orig = nonTreeEdges_orig.popFrontRet();
		//node a = GC.copy(e_orig->source());
		//node b = GC.copy(e_orig->target());
		GC.newEdge(e_orig);

		if (upMod.upwardPlanarEmbed(GC)) { //upward embedded the fups and check feasibility
			CombinatorialEmbedding Beta(GC);

			//choose a arbitrary feasibel ext. face
			FaceSinkGraph fsg(Beta, GC.copy(s_orig));
			SList<face> ext_faces;
			fsg.possibleExternalFaces(ext_faces);
			OGDF_ASSERT(!ext_faces.empty());
			Beta.setExternalFace(ext_faces.front());

			GraphCopy M = GC; // use a identical copy of GC to constrcut the merge graph of GC
			adjEntry extFaceHandle_cur = getAdjEntry(Beta, GC.copy(s_orig), Beta.externalFace());
			adjEntry adj_orig = GC.original(extFaceHandle_cur->theEdge())->adjSource();

			if (constructMergeGraph(M, adj_orig, nonTreeEdges_orig)) {
				FUPS = GC;
				extFaceHandle = FUPS.copy(GC.original(extFaceHandle_cur->theEdge()))->adjSource();
				continue;
			}
			else {
				//Beta is not feasible
				delEdges.pushBack(e_orig);
			}
		}
		else {
			// not ok, GC is not feasible
			delEdges.pushBack(e_orig);
		}
	}

	return Module::retFeasible;
}


void FeasibleUpwardPlanarSubgraph::getSpanTree(GraphCopy &GC, List<edge> &delEdges, bool random, bool multisource)
{
	delEdges.clear();
	if (GC.numberOfNodes() == 1)
		return; // nothing to do
	node s;
	hasSingleSource(GC, s);
	NodeArray<bool> visited(GC, false);
	EdgeArray<bool> isTreeEdge(GC,false);
	List<node> toDo;

	// the original graph is a multisource graph. The sources are connected with the super source s.
	// so do not delete the incident edges of s
	if (multisource){
		// put all incident edges of the source to treeEdges
		adjEntry adj;
		forall_adj(adj, s) {
			isTreeEdge[adj->theEdge()] = true;
			visited[adj->theEdge()->target()];
			toDo.pushBack(adj->theEdge()->target());
		}
	}
	else
		toDo.pushBack(s);


	//traversing with dfs
	forall_listiterators(node, it, toDo) {
		node start = *it;
		adjEntry adj;
		forall_adj(adj, start) {
			node v = adj->theEdge()->target();
			if (!visited[v])
				dfs_visit(GC, adj->theEdge(), visited, isTreeEdge, random);
		}
	}

	// delete all non tree edgesEdges to obtain a span tree
	List<edge> l;
	edge e;
	forall_edges(e, GC) {
		if (!isTreeEdge[e])
			l.pushBack(e);
	}
	while (!l.empty()) {
		e = l.popFrontRet();
		delEdges.pushBack(GC.original(e));
		GC.delCopy(e);
	}
}



void FeasibleUpwardPlanarSubgraph::dfs_visit(
	const Graph &G,
	edge e,
	NodeArray<bool> &visited,
	EdgeArray<bool> &treeEdges,
	bool random)
{
	treeEdges[e] = true;
	List<edge> elist;
	G.outEdges(e->target(), elist);
	if (!elist.empty()) {
		if (random)
			elist.permute();
		ListIterator<edge> it;
		for (it = elist.begin(); it.valid(); ++it) {
			edge ee = *it;
			if (!visited[ee->target()])
				dfs_visit(G, ee, visited, treeEdges, random);
		}
	}
	visited[e->target()] = true;
}


bool FeasibleUpwardPlanarSubgraph::constructMergeGraph(
	GraphCopy &M,
	adjEntry adj_orig,
	const List<edge> &orig_edges)
{
	CombinatorialEmbedding Beta(M);

	//set ext. face of Beta
	adjEntry ext_adj = M.copy(adj_orig->theEdge())->adjSource();
	Beta.setExternalFace(Beta.rightFace(ext_adj));

	FaceSinkGraph fsg(Beta, M.copy(adj_orig->theNode()));
	SList<node> aug_nodes;
	SList<edge> aug_edges;
	SList<face> fList;
	fsg.possibleExternalFaces(fList); // use this method to call the methode checkForest()
	node v_ext = fsg.faceNodeOf(Beta.externalFace());

	OGDF_ASSERT(v_ext != 0);

	fsg.stAugmentation(v_ext, M, aug_nodes, aug_edges);

	//add the deleted edges
	forall_listiterators(edge, it, orig_edges) {
		node a = M.copy((*it)->source());
		node b = M.copy((*it)->target());
		M.newEdge(a, b);
	}
	return (isAcyclic(M));
}




} // end namespace ogdf

