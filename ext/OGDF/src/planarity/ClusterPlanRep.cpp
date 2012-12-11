/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of ClusterPlanRep class
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

#include <ogdf/cluster/ClusterPlanRep.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/Layout.h>
#include <ogdf/basic/GridLayoutMapped.h>
#include <ogdf/basic/tuples.h>

#include <iomanip>


enum edgeDir {undef, in, out};

namespace ogdf {

ClusterPlanRep::ClusterPlanRep(
	const ClusterGraphAttributes &acGraph,
	const ClusterGraph &clusterGraph)
	:
	PlanRep(acGraph),
	m_pClusterGraph(&clusterGraph)
{
	OGDF_ASSERT(&clusterGraph.getGraph() == &acGraph.constGraph())

	m_edgeClusterID.init(*this, -1);
	m_nodeClusterID.init(*this, -1);

	//const Graph &CG = clusterGraph;
	//const Graph &G  = acGraph.constGraph();

	//if (&acGraph != 0)
	//{
	//	OGDF_ASSERT(&CG == &G);
	//}

	m_rootAdj = 0;

	//cluster numbers don't need to be consecutive
	cluster ci;
	forall_clusters(ci, clusterGraph)
		m_clusterOfIndex[ci->index()] = ci; //numbers are unique
}//constructor


void ClusterPlanRep::initCC(int i)
{
	PlanRep::initCC(i);

	//this means that for every reinitialization IDs are set
	//again, but this should not lead to problems
	//it cant be done in the constructor because the copies
	//in CCs are not yet initialized then
	//they are maintained for original nodes and for crossings
	//nodes on cluster boundaries
	const Graph &CG = *m_pClusterGraph;
	node v;
	forall_nodes(v, CG)
	{
		m_nodeClusterID[copy(v)] = m_pClusterGraph->clusterOf(v)->index();
	}//forallnodes

	//todo: initialize dummy node ids for different CCs

	//initialize all edges totally contained in a single cluster
	edge e;
	forall_edges(e, *this)
	{
		if (ClusterID(e->source()) == ClusterID(e->target()))
			m_edgeClusterID[e] = ClusterID(e->source());
	}//foralledges

}//initCC

/**
 * Inserts edge eOrig
 * This is only an insertion for graphs with already modeled
 * boundary edges, otherwise cluster recognition won't work
 * */
void ClusterPlanRep::insertEdgePathEmbedded(
	edge eOrig,
	CombinatorialEmbedding &E,
	const SList<adjEntry> &crossedEdges)
{
	//inherited insert
	PlanRep::insertEdgePathEmbedded(eOrig,E,crossedEdges);

	//update node cluster ids for crossing dummies
	ListConstIterator<edge> it;
	for(it = chain(eOrig).begin(); it.valid(); ++it)
	{
		node dummy = (*it)->target();
		if (dummy == copy(eOrig->target())) continue;

		OGDF_ASSERT(dummy->degree() == 4)

		//get the entries on the crossed edge
		adjEntry adjIn = (*it)->adjTarget();
		adjEntry adjC1 = adjIn->cyclicPred();
		adjEntry adjC2 = adjIn->cyclicSucc();

		//insert edge end points have the problem that the next one
		//does not need to have a clusterid yet
		//therefore we use the crossed edges endpoints
		//the two endpoints of the splitted edge
		node v1 = adjC1->twinNode();
		node v2 = adjC2->twinNode();
		OGDF_ASSERT(v1 != dummy)
		OGDF_ASSERT(v2 != dummy)
		node orV1 = original(v1);
		node orV2 = original(v2);
		//inserted edges are never boundaries
		//cases:
		//cross boundary edge
		//cross edge between different boundaries
		//cross edge between boundary and crossing/end
		//cross edge between crossings/ends
		//there are three types of nodes: orig, boundary, crossing
		//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		//should only be used with modeled boundaries
		cluster c1, c2;
		if (orV1 && orV2)
		{
			OGDF_ASSERT(m_pClusterGraph->clusterOf(orV1) ==
						 m_pClusterGraph->clusterOf(orV2));
			OGDF_ASSERT(m_nodeClusterID[v1] != -1)
			m_nodeClusterID[dummy] = m_nodeClusterID[v1];
			continue;
		}//if two originals
		if (orV1 || orV2) //a dummy (crossing/boundary) and an original
		{
			node orV = (orV1 ? orV1 : orV2);
			//node vOr = (orV1 ? v1 : v2);
			node vD  = (orV1 ? v2 : v1);
			cluster orC = m_pClusterGraph->clusterOf(orV);
			cluster dC  = clusterOfDummy(vD);

			OGDF_ASSERT( (orC == dC) ||		//original and crossing
				(orC == dC->parent()) || //original and boundary
				(orC->parent() == dC) )

			if (orC == dC) m_nodeClusterID[dummy] = orC->index();
			else if (orC == dC->parent()) m_nodeClusterID[dummy] = orC->index();
				 else OGDF_THROW (AlgorithmFailureException);
			continue;
		}//if one original
		//no originals, only crossings/boundaries
		c1 = clusterOfDummy(v1);
		c2 = clusterOfDummy(v2);
		OGDF_ASSERT( (c1 == c2) ||		//min.one crossing
			(c1 == c2->parent()) || //min. one boundary
			(c1->parent() == c2) ||
			(c1->parent() == c2->parent())
		)

		if (c1 == c2)
			m_nodeClusterID[dummy] = c1->index();
		else if (c1 == c2->parent())
			m_nodeClusterID[dummy] = c1->index();
		else if (c2 == c1->parent())
			m_nodeClusterID[dummy] = c2->index();
		else
			m_nodeClusterID[dummy] = c1->parent()->index();
		continue;


	}//for

}//insertEdgePathEmbedded

//use cluster structure to insert edges representing the cluster boundaries
void ClusterPlanRep::ModelBoundaries()
{
	//clusters hold their adjacent adjEntries and edges in lists, but after
	//insertion of boundaries these lists are outdated due to edge splittings

	//is the clusteradjacent edge outgoing?
	//2 means undef (only at inner leaf cluster)
	AdjEntryArray<int> outEdge(*m_pClusterGraph, 2); //0 in 1 out
	//what edge is currently adjacent to cluster (after possible split)
	//with original adjEntry in clusteradjlist
	AdjEntryArray<edge> currentEdge(*m_pClusterGraph, 0);

	List<adjEntry> rootEdges; //edges that can be used to set the outer face

	convertClusterGraph(m_pClusterGraph->rootCluster(), currentEdge, outEdge);
}

//recursively insert cluster boundaries for all clusters in cluster tree
void ClusterPlanRep::convertClusterGraph(cluster act,
										 AdjEntryArray<edge>& currentEdge,
										 AdjEntryArray<int>& outEdge)
{

	//are we at the first call (no real cluster)
	bool isRoot = (act == m_pClusterGraph->rootCluster());

	//check if we have to set the external face adj by hand
	if (isRoot && !act->cBegin().valid())
		m_rootAdj = firstEdge()->adjSource(); //only root cluster present
	//check if leaf cluster in cluster tree (most inner)
	bool isLeaf = false;
	if ((!act->cBegin().valid()) && (!isRoot)) isLeaf = true;
	// Test children first
	ListConstIterator<cluster> it;
	for (it = act->cBegin(); it.valid();)
	{
		ListConstIterator<cluster> succ = it.succ();
		convertClusterGraph((*it), currentEdge, outEdge);

		it = succ;
	}
	//do not convert root cluster
	if (isRoot) return;

		OGDF_ASSERT(this->representsCombEmbedding())

	insertBoundary(act, currentEdge, outEdge, isLeaf);

	OGDF_ASSERT(this->representsCombEmbedding())


}//convertclustergraph

//inserts Boundary for a single cluster, needs the cluster and updates a
//hashtable linking splitted original edges (used in clusteradjlist) to the
//current (new) edge, if cluster is leafcluster, we check and set the edge
//direction and the adjacent edge corresponding to the adjEntries in the clusters
//adjEntry List
void ClusterPlanRep::insertBoundary(cluster C,
									AdjEntryArray<edge>& currentEdge,
									AdjEntryArray<int>& outEdge,
									bool clusterIsLeaf)
{
	//we insert edges to represent the cluster boundary
	//by splitting the outgoing edges and connecting the
	//split nodes

	OGDF_ASSERT(this->representsCombEmbedding())

	//retrieve the outgoing edges

	//TODO: nichtverbundene Cluster abfangen

	SList<adjEntry> outAdj;
	//outgoing adjEntries in clockwise order
	m_pClusterGraph->adjEntries(C, outAdj);

	//now split the edges and save adjEntries
	//we maintain two lists of adjentries
	List<adjEntry> targetEntries, sourceEntries;
	//we need to find out if edge is outgoing
	bool isOut = false;
	SListIterator<adjEntry> it = outAdj.begin();
	//if no outAdj exist, we have a connected component
	//and dont need a boundary, change this when unconnected
	//graphs are allowed
	if (!it.valid()) return;

	while (it.valid())
	{
		//if clusterIsLeaf, save the corresponding direction and edge
		if (clusterIsLeaf)
		{
			//save the current, unsplitted edge
			//be careful with clusterleaf connecting, layered cl
			if (currentEdge[(*it)] == 0)
				currentEdge[(*it)] = copy((*it)->theEdge());
			//set twin here?

			//check direction, adjEntry is outgoing, compare with edge
			outEdge[(*it)] = ( ((*it) == (*it)->theEdge()->adjSource()) ? 1 : 0);
		}

		//workaround for nonleaf edges
		if (outEdge[(*it)] == 2)
			outEdge[(*it)] = ( ((*it) == (*it)->theEdge()->adjSource()) ? 1 : 0);

		if (currentEdge[(*it)] == 0)
		{
			//may already be splitted from head
			currentEdge[(*it)] = copy((*it)->theEdge());
		}


		//We need to find the real edge here
		edge splitEdge = currentEdge[(*it)];

		//...outgoing...?
		OGDF_ASSERT(outEdge[(*it)] != 2);
		isOut = outEdge[(*it)] == 1;

		edge newEdge = split(splitEdge);

		//store the adjEntries depending on in/out direction
		if (isOut)
		{
			//splitresults "upper" edge to old target is newEdge!?
			//only update for outgoing edges, ingoing stay actual
			currentEdge[(*it)] = newEdge;
			currentEdge[(*it)->twin()] = newEdge;
			sourceEntries.pushBack(newEdge->adjSource());
			targetEntries.pushBack(splitEdge->adjTarget());

			m_nodeClusterID[newEdge->source()] = C->index();
				//m_nodeClusterID[splitEdge->source()];
		}//if outgoing
		else
		{
			sourceEntries.pushBack(splitEdge->adjTarget());
			targetEntries.pushBack(newEdge->adjSource());

			m_nodeClusterID[newEdge->source()] = C->index();
		}//else outgoing

		//always set some rootAdj for external face
		if ( (C->parent() == m_pClusterGraph->rootCluster()) && !(it.succ().valid()))
		{
			//save the adjentry corresponding to new splitresult edge
			m_rootAdj = currentEdge[(*it)]->adjSource();
			OGDF_ASSERT(m_rootAdj != 0);
		}//if

		//go on with next edge
		it++;

	}//while outedges


	//we need pairs of adjEntries
	OGDF_ASSERT(targetEntries.size() == sourceEntries.size());
	//now flip first target entry to front
	//should be nonempty
	adjEntry flipper = targetEntries.popFrontRet();
	targetEntries.pushBack(flipper);

	//connect the new nodes to form the boundary
	while (!targetEntries.empty())
	{

		edge e = newEdge(sourceEntries.popFrontRet(), targetEntries.popFrontRet());
		//set type of new edges
		setClusterBoundary(e);
		m_edgeClusterID[e] = C->index();

		OGDF_ASSERT(this->representsCombEmbedding())
	}

	OGDF_ASSERT(this->representsCombEmbedding())

}//insertBoundary



void ClusterPlanRep::expand(bool lowDegreeExpand)
{
	PlanRep::expand(lowDegreeExpand);
	//update cluster info
	node v;
	forall_nodes(v, *this)
	{
		if (expandedNode(v) != 0)
		{
			OGDF_ASSERT(m_nodeClusterID[expandedNode(v)] != -1)
			m_nodeClusterID[v] = m_nodeClusterID[expandedNode(v)];
		}
	}
}//expand

void ClusterPlanRep::expandLowDegreeVertices(OrthoRep &OR)
{
	PlanRep::expandLowDegreeVertices(OR);
	//update cluster info
	node v;
	forall_nodes(v, *this)
	{
		if (expandedNode(v) != 0)
		{
			OGDF_ASSERT(m_nodeClusterID[expandedNode(v)] != -1)
			m_nodeClusterID[v] = m_nodeClusterID[expandedNode(v)];
		}
	}
}//expandlowdegree


//*****************************************************************************
//file output

void ClusterPlanRep::writeGML(const char *fileName, const Layout &drawing)
{
	ofstream os(fileName);
	writeGML(os,drawing);
}


void ClusterPlanRep::writeGML(const char *fileName)
{
	Layout drawing(*this);
	ofstream os(fileName);
	writeGML(os,drawing);
}


void ClusterPlanRep::writeGML(ostream &os, const Layout &drawing)
{
	const Graph &G = *this;

	NodeArray<int> id(*this);
	int nextId = 0;

	os.setf(ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::GraphAttributes::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	node v;
	forall_nodes(v,G) {

		node ori = original(v);

		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";

		os << "    graphics [\n";
		os << "      x " << drawing.x(v) << "\n";
		os << "      y " << drawing.y(v) << "\n";
		os << "      w " << 10.0 << "\n";
		os << "      h " << 10.0 << "\n";
		os << "      type \"rectangle\"\n";
		os << "      width 1.0\n";
		if (typeOf(v) == Graph::generalizationMerger) {
			os << "      type \"oval\"\n";
			os << "      fill \"#0000A0\"\n";
		}
		else if (typeOf(v) == Graph::generalizationExpander) {
			os << "      type \"oval\"\n";
			os << "      fill \"#00FF00\"\n";
		}
		else if (typeOf(v) == Graph::highDegreeExpander ||
			typeOf(v) == Graph::lowDegreeExpander)
			os << "      fill \"#FFFF00\"\n";
		else if (typeOf(v) == Graph::dummy)
			os << "      type \"oval\"\n";

		else
		if (m_pClusterGraph->clusterOf(ori)->index() != 0) //cluster
		{
			//only < 16
			os << "      fill \"#" << std::hex << std::setw(6) << std::setfill('0')
				<< m_pClusterGraph->clusterOf(ori)->index()*256*256+
					m_pClusterGraph->clusterOf(ori)->index()*256+
					m_pClusterGraph->clusterOf(ori)->index()*4 << std::dec << "\"\n";
		}
		else
		{
		if (v->degree() > 4)
			os << "      fill \"#FFFF00\"\n";

		else
			os << "      fill \"#000000\"\n";
		}

		os << "    ]\n"; // graphics

		os << "  ]\n"; // node
	}


	edge e;
	forall_edges(e,G) {
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		os << "    generalization " << typeOf(e) << "\n";

		os << "    graphics [\n";

		os << "      type \"line\"\n";

		if (typeOf(e) == Graph::generalization)
		{
			os << "      arrow \"last\"\n";

			os << "      fill \"#FF0000\"\n";
			os << "      width 3.0\n";
		}
		else
		{

			if (typeOf(e->source()) == Graph::generalizationExpander ||
				typeOf(e->source()) == Graph::generalizationMerger ||
				typeOf(e->target()) == Graph::generalizationExpander ||
				typeOf(e->target()) == Graph::generalizationMerger)
			{
				os << "      arrow \"none\"\n";
				if (isBrother(e))
					os << "      fill \"#F0F000\"\n"; //gelb
				else if (isHalfBrother(e))
					os << "      fill \"#FF00AF\"\n";
				else if (isClusterBoundary(e))
					os << "      fill \"#FF0000\"\n";
				else
					os << "      fill \"#FF0000\"\n";
			}
			else
				os << "      arrow \"none\"\n";
			if (isBrother(e))
				os << "      fill \"#F0F000\"\n"; //gelb
			else if (isHalfBrother(e))
				os << "      fill \"#FF00AF\"\n";
			else if (isClusterBoundary(e))
				os << "      fill \"#FF0000\"\n";
			else
				os << "      fill \"#00000F\"\n";
			os << "      width 1.0\n";
		}//else generalization
		os << "    ]\n"; // graphics

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}


}//namespace

