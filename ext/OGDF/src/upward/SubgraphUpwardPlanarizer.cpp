/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of SubgraphUpwardPlanarizer class.
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

#include <ogdf/upward/SubgraphUpwardPlanarizer.h>
#include <ogdf/upward/FeasibleUpwardPlanarSubgraph.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/upward/UpwardPlanarModule.h>
#include <ogdf/upward/FaceSinkGraph.h>
#include <limits.h>

#ifdef OGDF_DEBUG
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/upward/LayerBasedUPRLayout.h>
#endif


namespace ogdf {

Module::ReturnType SubgraphUpwardPlanarizer::doCall(UpwardPlanRep &UPR,
		const EdgeArray<int>  &cost,
		const EdgeArray<bool> &forbid)
{
	const Graph &G = UPR.original();
	GraphCopy GC(G);

	//reverse some edges in order to obtain a DAG
	List<edge> feedBackArcSet;
	m_acyclicMod.get().call(GC, feedBackArcSet);
	forall_listiterators(edge, it, feedBackArcSet) {
		GC.reverseEdge(*it);
	}

	OGDF_ASSERT(isSimple(G));

	//mapping cost
	EdgeArray<int> cost_GC(GC);
	edge e;
	forall_edges(e, GC) {
		if (forbid[GC.original(e)])
			cost_GC[e] = INT_MAX;
		else
			cost_GC[e] = cost[GC.original(e)];
	}

	// tranform to single source graph by adding a super source s_hat and connect it with the other sources
	EdgeArray<bool> sourceArcs(GC, false);
	node s_hat = GC.newNode();
	node v;
	forall_nodes(v, GC) {
		if (v->indeg() == 0 && v != s_hat) {
			edge e_tmp = GC.newEdge(s_hat, v);
			cost_GC[e_tmp] = 0; // crossings source arcs cause not cost
			sourceArcs[e_tmp] = true;
		}
	}


	/*
	//------------------------------------------------debug
	GraphAttributes AG_GC(GC, GraphAttributes::nodeGraphics|
						GraphAttributes::edgeGraphics|
						GraphAttributes::nodeColor|
						GraphAttributes::edgeColor|
						GraphAttributes::nodeLabel|
						GraphAttributes::edgeLabel
						);
	AG_GC.setAllHeight(30.0);
	AG_GC.setAllWidth(30.0);
	node z;
	forall_nodes(z, AG_GC.constGraph()) {
		char str[255];
		sprintf_s(str, 255, "%d", z->index()); 	// convert to string
		AG_GC.labelNode(z) = str;
	}
	AG_GC.writeGML("c:/temp/GC.gml");
	// --------------------------------------------end debug
	*/

	BCTree BC(GC);
	const Graph &bcTree = BC.bcTree();

	GraphCopy G_dummy;
	G_dummy.createEmpty(G);
	NodeArray<GraphCopy> biComps(bcTree, G_dummy); // bicomps of G; init with an empty graph
	UpwardPlanRep UPR_dummy;
	UPR_dummy.createEmpty(G);
	NodeArray<UpwardPlanRep> uprs(bcTree, UPR_dummy); // the upward planarized representation of the bicomps; init with an empty UpwarPlanRep

	constructComponentGraphs(BC, biComps);

	forall_nodes(v, bcTree) {

		if (BC.typeOfBNode(v) == BCTree::CComp)
			continue;

		GraphCopy &block = biComps[v];

		OGDF_ASSERT(m_subgraph.valid());

		// construct a super source for this block
		node s, s_block;
		hasSingleSource(block, s);
		s_block = block.newNode();
		block.newEdge(s_block, s); //connect s

		UpwardPlanarModule upMod;
		UpwardPlanRep bestUPR;

		//upward planarize if not upward planar
		if (!upMod.upwardPlanarEmbed(block)) {

			for(int i = 0; i < m_runs; i++) {// i multistarts
				UpwardPlanRep UPR_tmp;
				UPR_tmp.createEmpty(block);
				List<edge> delEdges;

				m_subgraph.get().call(UPR_tmp, delEdges);

				UPR_tmp.augment();

				//mark the source arcs of block
				UPR_tmp.m_isSourceArc[UPR_tmp.copy(s_block->firstAdj()->theEdge())] = true;
				adjEntry adj_tmp;
				forall_adj(adj_tmp, UPR_tmp.copy(s_block->firstAdj()->theEdge()->target())) {
					edge e_tmp = UPR_tmp.original(adj_tmp->theEdge());
					if (e_tmp != 0 && block.original(e_tmp) != 0 && sourceArcs[block.original(e_tmp)])
						UPR_tmp.m_isSourceArc[adj_tmp->theEdge()] = true;
				}

				//assign "crossing cost"
				EdgeArray<int> cost_Block(block);
				forall_edges(e, block) {
					if (block.original(e) == 0 || GC.original(block.original(e)) == 0 )
						cost_Block[e] = 0;
					else
						cost_Block[e] = cost_GC[block.original(e)];
				}

				/*
				if (false) {
					//---------------------------------------------------debug
					LayerBasedUPRLayout uprLayout;
					UpwardPlanRep upr_bug(UPR_tmp.getEmbedding());
					adjEntry adj_bug = upr_bug.getAdjEntry(upr_bug.getEmbedding(), upr_bug.getSuperSource(), upr_bug.getEmbedding().externalFace());
					node s_upr_bug = upr_bug.newNode();
					upr_bug.getEmbedding().splitFace(s_upr_bug, adj_bug);
					upr_bug.m_isSourceArc.init(upr_bug, false);
					upr_bug.m_isSourceArc[s_upr_bug->firstAdj()->theEdge()] = true;
					upr_bug.s_hat = s_upr_bug;
					upr_bug.augment();

					GraphAttributes GA_UPR_tmp(UPR_tmp, GraphAttributes::nodeGraphics|
							GraphAttributes::edgeGraphics|
							GraphAttributes::nodeColor|
							GraphAttributes::edgeColor|
							GraphAttributes::nodeLabel|
							GraphAttributes::edgeLabel
							);
					GA_UPR_tmp.setAllHeight(30.0);
					GA_UPR_tmp.setAllWidth(30.0);

					uprLayout.call(upr_bug, GA_UPR_tmp);

					// label the nodes with their index
					node z;
					forall_nodes(z, GA_UPR_tmp.constGraph()) {
						char str[255];
						sprintf_s(str, 255, "%d", z->index()); 	// convert to string
						GA_UPR_tmp.labelNode(z) = str;
						GA_UPR_tmp.y(z)=-GA_UPR_tmp.y(z);
						GA_UPR_tmp.x(z)=-GA_UPR_tmp.x(z);
					}
					edge eee;
					forall_edges(eee, GA_UPR_tmp.constGraph()) {
						DPolyline &line = GA_UPR_tmp.bends(eee);
						ListIterator<DPoint> it;
						for(it = line.begin(); it.valid(); it++) {
							(*it).m_y = -(*it).m_y;
							(*it).m_x = -(*it).m_x;
						}
					}
					GA_UPR_tmp.writeGML("c:/temp/UPR_tmp_fups.gml");
					cout << "UPR_tmp/fups faces:";
					UPR_tmp.outputFaces(UPR_tmp.getEmbedding());
					//end -----------------------------------------------debug
				}
				*/

				delEdges.permute();
				m_inserter.get().call(UPR_tmp, cost_Block, delEdges);

				if (i != 0) {
					if (UPR_tmp.numberOfCrossings() < bestUPR.numberOfCrossings()) {
						//cout << endl << "new cr_nr:" << UPR_tmp.numberOfCrossings() << " old  cr_nr : " << bestUPR.numberOfCrossings() << endl;
						bestUPR = UPR_tmp;
					}
				}
				else
					bestUPR = UPR_tmp;
			}//for
		}
		else { //block is upward planar
			CombinatorialEmbedding Gamma(block);
			FaceSinkGraph fsg((const CombinatorialEmbedding &)Gamma, s_block);
			SList<face> faceList;
			fsg.possibleExternalFaces(faceList);
			Gamma.setExternalFace(faceList.front());

			UpwardPlanRep UPR_tmp(Gamma);
			UPR_tmp.augment();

			//mark the source arcs of  block
			UPR_tmp.m_isSourceArc[UPR_tmp.copy(s->firstAdj()->theEdge())] = true;
				adjEntry adj_tmp;
				forall_adj(adj_tmp, UPR_tmp.copy(s->firstAdj()->theEdge()->target())) {
					edge e_tmp = UPR_tmp.original(adj_tmp->theEdge());
					if (e_tmp != 0 && block.original(e_tmp) != 0 && sourceArcs[block.original(e_tmp)])
						UPR_tmp.m_isSourceArc[adj_tmp->theEdge()] = true;
				}

			bestUPR = UPR_tmp;

			/*
			//debug
			//---------------------------------------------------debug
			GraphAttributes GA_UPR_tmp(UPR_tmp, GraphAttributes::nodeGraphics|
					GraphAttributes::edgeGraphics|
					GraphAttributes::nodeColor|
					GraphAttributes::edgeColor|
					GraphAttributes::nodeLabel|
					GraphAttributes::edgeLabel
					);
			GA_UPR_tmp.setAllHeight(30.0);
			GA_UPR_tmp.setAllWidth(30.0);

			// label the nodes with their index
			forall_nodes(z, GA_UPR_tmp.constGraph()) {
				char str[255];
				sprintf_s(str, 255, "%d", z->index()); 	// convert to string
				GA_UPR_tmp.labelNode(z) = str;
				GA_UPR_tmp.y(z)=-GA_UPR_tmp.y(z);
				GA_UPR_tmp.x(z)=-GA_UPR_tmp.x(z);
			}
			edge eee;
			forall_edges(eee, GA_UPR_tmp.constGraph()) {
				DPolyline &line = GA_UPR_tmp.bends(eee);
				ListIterator<DPoint> it;
				for(it = line.begin(); it.valid(); it++) {
					(*it).m_y = -(*it).m_y;
					(*it).m_x = -(*it).m_x;
				}
			}
			GA_UPR_tmp.writeGML("c:/temp/UPR_tmp_fups.gml");
			cout << "UPR_tmp/fups faces:";
			UPR_tmp.outputFaces(UPR_tmp.getEmbedding());
			//end -----------------------------------------------debug
			*/

		}
		uprs[v] = bestUPR;
	}//forall_nodes

	// compute the number of crossings
	int nr_cr = 0;
	forall_nodes(v, bcTree) {
		if (BC.typeOfBNode(v) != BCTree::CComp)
			nr_cr = nr_cr + uprs[v].numberOfCrossings();
	}

	//merge all component to a graph
	node parent_BC = BC.bcproper(s_hat);
	NodeArray<bool> nodesDone(bcTree, false);
	dfsMerge(GC, BC, biComps, uprs, UPR, 0, parent_BC, nodesDone); // start with the component which contains the super source s_hat

	//augment to single sink graph
	UPR.augment();

	//set crossings
	UPR.crossings = nr_cr;


	//------------------------------------------------debug
	/*
	LayerBasedUPRLayout uprLayout;
	UpwardPlanRep upr_bug(UPR.getEmbedding());
	adjEntry adj_bug = upr_bug.getAdjEntry(upr_bug.getEmbedding(), upr_bug.getSuperSource(), upr_bug.getEmbedding().externalFace());
	node s_upr_bug = upr_bug.newNode();
	upr_bug.getEmbedding().splitFace(s_upr_bug, adj_bug);
	upr_bug.m_isSourceArc.init(upr_bug, false);
	upr_bug.m_isSourceArc[s_upr_bug->firstAdj()->theEdge()] = true;
	upr_bug.s_hat = s_upr_bug;
	upr_bug.augment();
	GraphAttributes AG(UPR, GraphAttributes::nodeGraphics|
						GraphAttributes::edgeGraphics|
						GraphAttributes::nodeColor|
						GraphAttributes::edgeColor|
						GraphAttributes::nodeLabel|
						GraphAttributes::edgeLabel
						);
	AG.setAllHeight(30.0);
	AG.setAllWidth(30.0);

	uprLayout.call(upr_bug, AG);

	forall_nodes(v, AG.constGraph()) {
		int idx;
		idx = v->index();


		if (UPR.original(v) != 0)
			idx = UPR.original(v)->index();


		char str[255];
		sprintf_s(str, 255, "%d", idx); 	// convert to string
		AG.labelNode(v) = str;
		if (UPR.isDummy(v))
			AG.colorNode(v) = "#ff0000";
		AG.y(v)=-AG.y(v);
	}
	// label the edges with their index
	forall_edges(e, AG.constGraph()) {
		char str2[255];
		sprintf_s(str2, 255, "%d", e->index()); 	// convert to string
		AG.labelEdge(e) = str2;
		if (UPR.isSourceArc(e))
			AG.colorEdge(e) = "#00ff00";
		if (UPR.isSinkArc(e))
			AG.colorEdge(e) = "#ff0000";

		DPolyline &line = AG.bends(e);
		ListIterator<DPoint> it;
		for(it = line.begin(); it.valid(); it++) {
			(*it).m_y = -(*it).m_y;
		}
	}
	AG.writeGML("c:/temp/upr_res.gml");
	//cout << "UPR_RES";
	//UPR.outputFaces(UPR.getEmbedding());
	//cout << "Mapping :" << endl;
	//forall_nodes(v, UPR) {
	//	if (UPR.original(v) != 0) {
	//		cout << "node UPR  " << v << "   node G  " << UPR.original(v) << endl;
	//	}
	//}
	// --------------------------------------------end debug
	*/

#ifdef OGDF_DEBUG
	UpwardPlanarModule upMod;
#endif
	OGDF_ASSERT(hasSingleSource(UPR));
	OGDF_ASSERT(isSimple(UPR));
	OGDF_ASSERT(isAcyclic(UPR));
	OGDF_ASSERT(upMod.upwardPlanarityTest(UPR));

/*
	edge eee;
	forall_edges(eee, UPR.original()) {
		if (UPR.isReversed(eee))
			cout << endl << eee << endl;
	}
*/
	return Module::retFeasible;
}



void SubgraphUpwardPlanarizer::dfsMerge(
	const GraphCopy &GC,
	BCTree &BC,
	NodeArray<GraphCopy> &biComps,
	NodeArray<UpwardPlanRep> &uprs,
	UpwardPlanRep &UPR_res,
	node parent_BC,
	node current_BC,
	NodeArray<bool> &nodesDone)
{
	// only one component.
	if (current_BC->degree() == 0) {
		merge(GC, UPR_res, biComps[current_BC], uprs[current_BC]);
		return;
	}

	adjEntry adj;
	forall_adj(adj, current_BC) {
		node next_BC = adj->twin()->theNode();
		if (BC.typeOfBNode(current_BC) == BCTree::CComp) {
			if (parent_BC != 0 && !nodesDone[parent_BC]) {
				merge(GC, UPR_res, biComps[parent_BC], uprs[parent_BC]);
				nodesDone[parent_BC] = true;
			}
			if (!nodesDone[next_BC]) {
				merge(GC, UPR_res, biComps[next_BC], uprs[next_BC]);
				nodesDone[next_BC] = true;
			}
		}
		if (next_BC != parent_BC )
			dfsMerge(GC, BC, biComps, uprs, UPR_res, current_BC, next_BC, nodesDone);
	}
}



void SubgraphUpwardPlanarizer::merge(
	const GraphCopy &GC,
	UpwardPlanRep &UPR_res,
	const GraphCopy &block,
	UpwardPlanRep &UPR)
{
	node startUPR = UPR.getSuperSource()->firstAdj()->theEdge()->target();
	node startRes;

	node startG = GC.original(block.original(UPR.original(startUPR)));

	bool empty = UPR_res.empty();

	if (empty) {

		OGDF_ASSERT(startG == 0);

		// contruct a node in UPR_res assocciated with startUPR
		startRes = UPR_res.newNode();
		UPR_res.m_isSinkArc.init(UPR_res, false);
		UPR_res.m_isSourceArc.init(UPR_res, false);
		UPR_res.s_hat = startRes;
	}
	else {
		startRes = UPR_res.copy(startG);
	}

	OGDF_ASSERT(startRes != 0);

	// compute the adjEntry position (in UPR_res) of the cutvertex startRes
	adjEntry pos = 0;
	if (!empty) {
		adjEntry run, adj_ext = 0, adj_int = 0;
		forall_adj(run, startRes) {
			if (UPR_res.getEmbedding().rightFace(run) == UPR_res.getEmbedding().externalFace()) {
				adj_ext = run;
				break;
			}
			if (run->theEdge()->source() == startRes)
				adj_int = run;
		}
		// cutvertex is a sink in UPR_res
		if (adj_ext == 0 && adj_int == 0) {
			pos = UPR_res.sinkSwitchOf(startRes);
		}
		else {
			if (adj_ext == 0)
				pos = adj_int;
			else
				pos = adj_ext;
		}
		OGDF_ASSERT(pos != 0);
	}

	// construct for each node (except the two super sink and the super source) of UPR a associated of UPR to UPR_res
	NodeArray<node> nodeUPR2UPR_res(UPR, 0);
	nodeUPR2UPR_res[startUPR] = startRes;
	node v;
	forall_nodes(v, UPR) {

		// allready constructed or is super sink or super source
		if (v == startUPR || v == UPR.getSuperSink() || v == UPR.getSuperSink()->firstAdj()->theEdge()->source() || v == UPR.getSuperSource())
			continue;

		node vNew;
		if (UPR.original(v) != 0 ) {
			node vG = GC.original(block.original((UPR.original(v))));
			if (vG != 0)
				vNew = UPR_res.newNode(vG);
			else
				vNew = UPR_res.newNode(); //vG is the super source
		}
		else // crossing dummy, no original node
			vNew = UPR_res.newNode();
		nodeUPR2UPR_res[v] = vNew;
	}

	//add edges of UPR to UPR_res
	EdgeArray<edge> edgeUPR2UPR_res(UPR, 0);
	edge e;
	forall_edges(e, block) {

		if (e->source()->indeg()==0) // the artificial edge with the super source
			continue;

		List<edge> chains = UPR.chain(e);
		edge eG = 0, eGC = block.original(e);
		eG = GC.original(eGC);

		OGDF_ASSERT(!chains.empty());

		//construct new edges in UPR_res
		forall_listiterators(edge, it, chains) {
			node tgt = nodeUPR2UPR_res[(*it)->target()];
			node src = nodeUPR2UPR_res[(*it)->source()];
			edge eNew = UPR_res.newEdge(src, tgt);
			edgeUPR2UPR_res[*it] = eNew;

			if (UPR.isSinkArc(UPR.copy(e)))
				UPR_res.m_isSinkArc[eNew] = true;
			if (UPR.isSourceArc(UPR.copy(e)))
				UPR_res.m_isSourceArc[eNew] = true;

			if (eG == 0) { // edge is associated with a sink arc
				UPR_res.m_eOrig[eNew] = 0;
				continue;
			}

			UPR_res.m_eOrig[eNew] = eG;
			if (chains.size() == 1) { // e is not split
				UPR_res.m_eCopy[eG].pushBack(eNew);
				UPR_res.m_eIterator[eNew] = UPR_res.m_eCopy[eG].begin();
				break;
			}
			UPR_res.m_eCopy[eG].pushBack(eNew);
			UPR_res.m_eIterator[eNew] = UPR_res.m_eCopy[eG].rbegin();
		}
	}


	///*
	//* embed the new component in UPR_res with respect to the embedding of UPR
	//*/

	// for the cut vertex
	if (!empty) {
		adjEntry run = UPR.getAdjEntry(UPR.getEmbedding(), startUPR, UPR.getEmbedding().externalFace());
		run = run->cyclicSucc();
		adjEntry adjStart = run;
		do {
			if (edgeUPR2UPR_res[run->theEdge()] != 0) {
				adjEntry adj_UPR_res = edgeUPR2UPR_res[run->theEdge()]->adjSource();
				UPR_res.moveAdjAfter(adj_UPR_res, pos);
				pos = adj_UPR_res;
			}
			run = run->cyclicSucc();
		} while(run != adjStart);
	}

	forall_nodes(v, UPR) {
		if (v == startUPR && !empty)
			continue;

		node v_UPR_res = nodeUPR2UPR_res[v];
		List<adjEntry> adj_UPR, adj_UPR_res;
		UPR.adjEntries(v, adj_UPR);

		// convert adj_UPR of v to adj_UPR_res of v_UPR_res
		forall_listiterators(adjEntry, it, adj_UPR) {
			edge e_res = edgeUPR2UPR_res[(*it)->theEdge()];
			if (e_res == 0) // associated edges in UPR_res
				continue;
			adjEntry adj_res = e_res->adjSource();
			if (adj_res->theNode() != v_UPR_res)
				adj_res = adj_res->twin();
			adj_UPR_res.pushBack(adj_res);
		}

		UPR_res.sort(v_UPR_res, adj_UPR_res);
	}

	/*
	//---------------------------------------------------debug
	if (!UPR_res.empty()) {
		GraphAttributes GA_UPR_res(UPR_res, GraphAttributes::nodeGraphics|
				GraphAttributes::edgeGraphics|
				GraphAttributes::nodeColor|
				GraphAttributes::edgeColor|
				GraphAttributes::nodeLabel|
				GraphAttributes::edgeLabel
				);
		GA_UPR_res.setAllHeight(30.0);
		GA_UPR_res.setAllWidth(30.0);
		node z;
		// label the nodes with their index
		forall_nodes(z, GA_UPR_res.constGraph()) {
			char str[255];
			sprintf_s(str, 255, "%d", z->index()); 	// convert to string
			GA_UPR_res.labelNode(z) = str;
		}
		GA_UPR_res.writeGML("c:/temp/UPR_res_tmp.gml");
		cout << "UPR_res_tmp faces:";
		UPR_res.outputFaces(UPR_res.getEmbedding());
	}

	GraphAttributes GA_UPR(UPR, GraphAttributes::nodeGraphics|
				GraphAttributes::edgeGraphics|
				GraphAttributes::nodeColor|
				GraphAttributes::edgeColor|
				GraphAttributes::nodeLabel|
				GraphAttributes::edgeLabel
				);
	GA_UPR.setAllHeight(30.0);
	GA_UPR.setAllWidth(30.0);
	node z;
	// label the nodes with their index
	forall_nodes(z, GA_UPR.constGraph()) {
		char str[255];
		sprintf_s(str, 255, "%d", z->index()); 	// convert to string
		GA_UPR.labelNode(z) = str;
	}
	GA_UPR.writeGML("c:/temp/UPR_tmp.gml");
	cout << "UPR_tmp faces:";
	UPR.outputFaces(UPR.getEmbedding());
	//end -----------------------------------------------debug
	*/

	// update UPR_res
	UPR_res.initMe();
}


void SubgraphUpwardPlanarizer::constructComponentGraphs(BCTree &BC, NodeArray<GraphCopy> &biComps)
{
	NodeArray<int> constructed(BC.originalGraph(), -1);
	const Graph &bcTree = BC.bcTree();
	node v;
	int i = 0; // comp. number
	forall_nodes(v, bcTree) {

		if (BC.typeOfBNode(v) == BCTree::CComp)
			continue;

		const SList<edge> &edges_comp = BC.hEdges(v); //bicomp edges
		List<edge> edges_orig;
		forall_slistiterators(edge, it, edges_comp)
			edges_orig.pushBack(BC.original(*it));

		GraphCopy GC;
		GC.createEmpty(BC.originalGraph());
		// construct i-th component graph
		forall_listiterators(edge, it, edges_orig) {
			node srcOrig = (*it)->source();
			node tgtOrig = (*it)->target();
			if (constructed[srcOrig] != i) {
				constructed[srcOrig] = i;
				GC.newNode(srcOrig);
			}
			if (constructed[tgtOrig] != i) {
				constructed[tgtOrig] = i;
				GC.newNode(tgtOrig);
			}
			GC.newEdge(*it);
		}
		biComps[v] = GC;
		i++;
	}
}

}
