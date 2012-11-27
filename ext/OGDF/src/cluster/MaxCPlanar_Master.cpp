/*
 * $Revision: 2610 $
 *
 * last checkin:
 *   $Author: klein $
 *   $Date: 2012-07-16 10:04:15 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the master class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
 *
 * This class is managing the optimization.
 * Variables and initial constraints are generated and pools are initialized.
 * Since variables correspond to the edges of a complete graph, node pairs
 * are used mostly instead of edges.
 *
 * \author Markus Chimani, Mathias Jansen, Karsten Klein
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

#ifdef USE_ABACUS

#include <ogdf/internal/cluster/MaxCPlanar_Master.h>
#include <ogdf/internal/cluster/MaxCPlanar_Sub.h>
#include <ogdf/internal/cluster/Cluster_ChunkConnection.h>
//#include <ogdf/internal/cluster/MaxCPlanar_MinimalClusterConnection.h> // not used
#include <ogdf/internal/cluster/Cluster_MaxPlanarEdges.h>
#include <ogdf/planarity/BoyerMyrvold.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/cluster/ClusterArray.h>
//heuristics in case only max planar subgraph is computed
#include <ogdf/planarity/FastPlanarSubgraph.h>
#include <ogdf/planarity/MaximalPlanarSubgraphSimple.h>
#include <ogdf/basic/ArrayBuffer.h>

using namespace ogdf;


#ifdef OGDF_DEBUG
void Master::printGraph(const Graph &G) {
	edge e;
	int i=0;
	Logger::slout() << "The Given Graph" << endl;
	forall_edges(e,G) {
		Logger::slout() << "Edge " << i++ << ": (" << e->source()->index() << "," << e->target()->index() << ") " << endl;
	}
}
#endif

//std::ostream &operator<<(std::ostream &os, const nodePair& v) {
//	os << "(" <<v.v1<<","<<v.v2<<")";
//	return os;
//}


Master::Master(
	const ClusterGraph &C,
	int heuristicLevel,
	int heuristicRuns,
	double heuristicOEdgeBound,
	int heuristicNPermLists,
	int kuratowskiIterations,
	int subdivisions,
	int kSupportGraphs,
	double kHigh,
	double kLow,
	bool perturbation,
	double branchingGap,
	const char *time,
	bool dopricing,
	bool checkCPlanar,
	int numAddVariables,
	double strongConstraintViolation,
	double strongVariableViolation,
	ABA_MASTER::OUTLEVEL ol) : m_fastHeuristicRuns(25),
	m_useDefaultCutPool(true),
	m_cutConnPool(0),
	m_cutKuraPool(0),
	m_checkCPlanar(checkCPlanar),
	m_porta(false),
	m_numAddVariables(numAddVariables),
	m_strongConstraintViolation(strongConstraintViolation),
	m_strongVariableViolation(strongVariableViolation),
	ABA_MASTER("MaxCPlanar", true, dopricing, ABA_OPTSENSE::Max)
{

	// Reference to the given ClusterGraph and the underlying Graph.
	m_C = &C;
	m_G = &(C.getGraph());
	// Create a copy of the graph as we need to modify it
	m_solutionGraph = new GraphCopy(*m_G);
	// Define the maximum number of variables needed.
	// The actual number needed may be much smaller, so there
	// is room for improvement...
	//ToDo: Just count how many vars are added

	//Max number of edges
	//KK: Check this change, added div 2
	int nComplete = (m_G->numberOfNodes()*(m_G->numberOfNodes()-1)) / 2;
	m_nMaxVars = nComplete;
	//to use less variables in case we have only the root cluster,
	//we temporarily set m_nMaxVars to the number of edges
	if ( (m_C->numberOfClusters() == 1) && (isConnected(*m_G)) )
		m_nMaxVars = m_G->numberOfEdges();

	// Computing the main objective function coefficient for the connection edges.
	//int nConnectionEdges = nComplete - m_G->numberOfEdges();
	m_epsilon = (double)(0.2/(2*(m_G->numberOfNodes())));

	// Setting parameters
	m_out = ol;
	m_nKuratowskiIterations = kuratowskiIterations;
	m_nSubdivisions = subdivisions;
	m_nKuratowskiSupportGraphs = kSupportGraphs;
	m_heuristicLevel = heuristicLevel;
	m_nHeuristicRuns = heuristicRuns;
	m_usePerturbation = perturbation;
	m_kuratowskiBoundHigh = kHigh;
	m_kuratowskiBoundLow = kLow;
	m_branchingGap = branchingGap;
	m_maxCpuTime = new ABA_STRING(this,time);
	m_heuristicFractionalBound = heuristicOEdgeBound;
	m_nHeuristicPermutationLists = heuristicNPermLists;
	m_mpHeuristic = true;

	// Further settings
	m_nCConsAdded = 0;
	m_nKConsAdded = 0;
	m_solvesLP = 0;
	m_varsInit = 0;
	m_varsAdded = 0;
	m_varsPotential = 0;
	m_varsMax = 0;
	m_varsCut = 0;
	m_varsKura = 0;
	m_varsPrice = 0;
	m_varsBranch = 0;
	m_activeRepairs = 0;
	m_repairStat.init(100);

	outLevel(ol);
}


Master::~Master() {
	delete m_maxCpuTime;
	delete m_solutionGraph;
}


ABA_SUB *Master::firstSub() {
	return new Sub(this);
}


// Replaces current m_solutionGraph by new GraphCopy based on \a connection list
void Master::updateBestSubGraph(List<nodePair> &original, List<nodePair> &connection, List<edge> &deleted) {

	// Creates a new GraphCopy \a m_solutionGraph and deletes all edges
	// TODO: Extend GraphCopySimple to be usable here: Allow
	// edge deletion and add pure node initialization
	// Is the solutiongraph used during computation anyhow?
	// Otherwise only store the lists
	delete m_solutionGraph;
	m_solutionGraph = new GraphCopy(*m_G);
	edge e = m_solutionGraph->firstEdge();
	edge succ;
	while (e!=0) {
		succ = e->succ();
		m_solutionGraph->delEdge(e);
		e = succ;
	}

	// Delete all edges that have been stored previously in edge lists
	m_allOneEdges.clear();
	m_originalOneEdges.clear();
	m_connectionOneEdges.clear();
	m_deletedOriginalEdges.clear();

	// Update the edge lists according to new solution
	ListConstIterator<nodePair> oit = original.begin();
	node cv,cw;
	while (oit.valid()) {

		// Add all original edges to \a m_solutionGraph
		cv = m_solutionGraph->copy((*oit).v1);
		cw = m_solutionGraph->copy((*oit).v2);
		m_solutionGraph->newEdge(cv,cw);

		m_allOneEdges.pushBack(*oit);
		m_originalOneEdges.pushBack(*oit);

		oit++;
	}

	ListConstIterator<nodePair> cit = connection.begin();
	while (cit.valid()) {

		// Add all new connection edges to \a m_solutionGraph
		cv = m_solutionGraph->copy((*cit).v1);
		cw = m_solutionGraph->copy((*cit).v2);
		m_solutionGraph->newEdge(cv,cw);

		m_allOneEdges.pushBack(*cit);
		m_connectionOneEdges.pushBack(*cit);

		cit++;
	}

	ListConstIterator<edge> dit = deleted.begin();
	while (dit.valid()) {

		m_deletedOriginalEdges.pushBack(*dit);

		dit++;
	}
#ifdef OGDF_DEBUG
	m_solutionGraph->writeGML("UpdateSolutionGraph.gml");
//Just for special debugging purposes:
	if (true) {//note that we output the original graph plus added edges, but don't remove deleted ones
		ClusterArray<cluster> ca(*m_C);
		Graph GG;
		NodeArray<node> na(*m_G);
		ClusterGraph CG(*m_C,GG, ca, na);

		cit = connection.begin();

		List<edge> le;

		while (cit.valid()) {

			// Add all new connection edges to \a m_solutionGraph
			cv = na[(*cit).v1];
			cw = na[(*cit).v2];
			edge e = GG.newEdge(cv,cw);
			le.pushBack(e);

			cit++;
		}

		ClusterGraphAttributes CGA(CG, GraphAttributes::edgeType | GraphAttributes::nodeType |
				GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics | GraphAttributes::edgeColor);
		ListConstIterator<edge> it = le.begin();
		while (it.valid())
		{
			cout << (*it)->graphOf() << "\n";
			cout << &GG << "\n";
			CGA.colorEdge(*it) = "#FF0000";
			it++;
		}
		CGA.writeGML("PlanarExtensionMCPSP.gml");
	}
#endif
}


void Master::getAllOptimalSolutionEdges(List<nodePair> &edges) const {
	edges.clear();
	ListConstIterator<nodePair> it;
	for (it=m_allOneEdges.begin(); it.valid(); ++it) {
		edges.pushBack(*it);
	}
}


void Master::getOriginalOptimalSolutionEdges(List<nodePair> &edges) const {
	edges.clear();
	ListConstIterator<nodePair> it;
	for (it=m_originalOneEdges.begin(); it.valid(); ++it) {
		edges.pushBack(*it);
	}
}


void Master::getConnectionOptimalSolutionEdges(List<nodePair> &edges) const {
	edges.clear();
	ListConstIterator<nodePair> it;
	for (it=m_connectionOneEdges.begin(); it.valid(); ++it) {
		edges.pushBack(*it);
	}
}


void Master::getDeletedEdges(List<edge> &edges) const {
	edges.clear();
	ListConstIterator<edge> it;
	for (it=m_deletedOriginalEdges.begin(); it.valid(); ++it) {
		edges.pushBack(*it);
	}
}

//todo: is called only once, but could be sped up the same way as the co-conn check
void Master::clusterConnection(cluster c, GraphCopy &gc, double &upperBoundC) {
	// For better performance, a node array is used to indicate which nodes are contained
	// in the currently considered cluster.
	NodeArray<bool> vInC(gc,false);
    // First check, if the current cluster \a c is a leaf cluster.
    // If so, compute the number of edges that have at least to be added
    // to make the cluster induced graph connected.
	if (c->cCount()==0) { 	//cluster \a c is a leaf cluster
		GraphCopy *inducedC = new GraphCopy((const Graph&)gc);
		node v,w;
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes); // \a clusterNodes now contains all (original) nodes of cluster \a c.
		ListConstIterator<node> it;
		for (it=clusterNodes.begin(); it.valid(); ++it) {
			vInC[gc.copy(*it)] = true;
		}

		// Delete all nodes from \a inducedC that do not belong to the cluster,
		// in order to obtain the cluster induced graph.
		v = inducedC->firstNode();
		while (v!=0)  {
			w = v->succ();
			if (!vInC[inducedC->original(v)]) inducedC->delNode(v);
			v = w;
		}

		// Determine number of connected components of cluster induced graph.
		//Todo: check could be skipped
		if (!isConnected(*inducedC)) {

			NodeArray<int> conC(*inducedC);
			int nCC = connectedComponents(*inducedC,conC);
			//at least #connected components - 1 edges have to be added.
			upperBoundC -= (nCC-1)*m_largestConnectionCoeff;
		}
		delete inducedC;
	// Cluster \a c is an "inner" cluster. Process all child clusters first.
	} else {	//c->cCount is != 0, process all child clusters first

		ListConstIterator<cluster> cit;
		for (cit=c->cBegin(); cit.valid(); ++cit) {
			clusterConnection(*cit,gc,upperBoundC);
		}

		// Create cluster induced graph.
		GraphCopy *inducedC = new GraphCopy((const Graph&)gc);
		node v,w;
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes); //\a clusterNodes now contains all (original) nodes of cluster \a c.
		ListConstIterator<node> it;
		for (it=clusterNodes.begin(); it.valid(); ++it) {
			vInC[gc.copy(*it)] = true;
		}
		v = inducedC->firstNode();
		while (v!=0)  {
			w = v->succ();
			if (!vInC[inducedC->original(v)]) inducedC->delNode(v);
			v = w;
		}

		// Now collapse each child cluster to one node and determine #connected components of \a inducedC.
		List<node> oChildClusterNodes;
		List<node> cChildClusterNodes;
		for (cit=c->cBegin(); cit.valid(); ++cit) {
			(*cit)->getClusterNodes(oChildClusterNodes);
			ListConstIterator<node> it;
			node copy;
			// Compute corresponding nodes of graph \a inducedC.
			for (it=oChildClusterNodes.begin(); it.valid(); ++it) {
				copy = inducedC->copy(gc.copy(*it));
				cChildClusterNodes.pushBack(copy);
			}
			inducedC->collaps(cChildClusterNodes);
			oChildClusterNodes.clear();
			cChildClusterNodes.clear();
		}
		// Now, check \a inducedC for connectivity.
		if (!isConnected(*inducedC)) {

			NodeArray<int> conC(*inducedC);
			int nCC = connectedComponents(*inducedC,conC);
			//at least #connected components - 1 edges have to added.
			upperBoundC -= (nCC-1)*m_largestConnectionCoeff;
		}
		delete inducedC;
	}
}//clusterConnection

double Master::heuristicInitialLowerBound()
{
	double lbound = 0.0;
	//In case we only have a single (root) cluster, we can
	//use the result of a fast Max Planar Subgraph heuristic
	//to initialize the lower bound
	if ( (m_C->numberOfClusters() == 1) && (m_mpHeuristic) )
	{
		//we run both heuristics that currently exist in OGDF
		//MaxSimple
    	MaximalPlanarSubgraphSimple simpleHeur;
    	List<edge> delEdgesList;
    	simpleHeur.call(*m_G, delEdgesList);
    	FastPlanarSubgraph fastHeur;
    	fastHeur.runs(m_fastHeuristicRuns);
    	List<edge> delEdgesListFast;
    	fastHeur.call(*m_G, delEdgesListFast);
    	lbound = m_G->numberOfEdges()-min(delEdgesList.size(), delEdgesListFast.size());

    	if (!isConnected(*m_G)) lbound = lbound-1.0; //#edges*epsilon
	}//if heuristics used
	return lbound;
}//heuristicInitialLowerBound

double Master::heuristicInitialUpperBound() {

	double upperBoundO = m_G->numberOfEdges();
	double upperBoundC = 0.0;

	// Checking graph for planarity
	// If \a m_G is planar \a upperBound is simply set to the number of edges of \a m_G.
	GraphCopy gc(*m_G);
	BoyerMyrvold bm;
	if (bm.isPlanarDestructive(gc)) upperBoundO = m_G->numberOfEdges();
	else {

		// Extract all possible Kuratowski subdivisions.
		// Compare extracted subdivisions and try to obtain the
		// maximum number of independent subdivisions, i.e. a maximum
		// independent set in the overlap graph.
		// Due to the complexity of this task, we only check if
		// a subdivision (sd) does overlap with a subdivision for which
		// we already decreased the upper bound. In this case,
		// upperBound stays the same.

		upperBoundO = m_G->numberOfEdges();

		GraphCopy *gCopy = new GraphCopy(*m_G);
		SList<KuratowskiWrapper> subDivs;

		bm.planarEmbedDestructive(*gCopy,subDivs,-1);
		//we store a representative and its status for each edge
		//note that there might be an overlap, in that case
		//we keep a representative with status false if existing
		//to check if we can safely reduce the upper bound (ub)
		EdgeArray<edge> subRep(*gCopy, NULL); //store representing edge for sd
		EdgeArray<bool> coverStatus(*gCopy, false); //false means not covered by ub decrease yet

		//runtime for the check: we run over all edges in all subdivisions
		if (subDivs.size() > 0) { // At least one edge has to be deleted to obtain planarity.

			// We run over all subdivisions and check for overlaps
			SListConstIterator<KuratowskiWrapper> sit = subDivs.begin();
			SListConstIterator<edge> eit;
			while( sit.valid() )
			{
				bool covered = false; //may the sd already be covered by a decrease in ub
				//for each edge we set the representative to be the first edge of sd
				edge sdRep = *((*sit).edgeList.begin());//sd is always non-empty
				//we check if any of the edges in sd were already visited and if
				//the representative has status false, in this case, we are not
				//allowed to decrease the ub
				for (eit=(*sit).edgeList.begin(); eit.valid(); ++eit)
				{
					//we already encountered this edge
					if (subRep[*eit] != NULL)
					{
						//and decreased ub for an enclosing sd
						//(could we just break in the if case?)
						if (coverStatus[subRep[*eit]]) covered = true;
						else subRep[*eit] = sdRep; //might need an update
					}
					else subRep[*eit] = sdRep;
				}
				if (!covered)
				{
					coverStatus[sdRep] = true;
					upperBoundO--;
				}//not yet covered, independent
				sit++;
			}//while
		}
		delete gCopy;
	}

	/*
	 * Heuristic can be improved by checking, how many additional C-edges have to be added at least.
	 * A first simple approach is the following:
	 * Since the Graph has to be completely connected in the end, all chunks have to be connected.
	 * Thus the numbers of chunks minus 1 summed up over all clusters is a trivial lower bound.

	* We perform a bottom-up search through the cluster-tree, each time checking the cluster
	 * induced Graph for connectivity. If the Graph is not connected, the number of chunks -1 is added to
	 * a counter. For "inner" clusters we have to collapse all child clusters to one node,
	 * in order to obtain a correct result.
	 */

	GraphCopy gcc(*m_G);
	cluster c = m_C->rootCluster();
	clusterConnection(c, gcc, upperBoundC);

	// Return-value results from the max. number of O-edges that might be contained
	// in an optimal solution minus \a epsilon times the number of C-edges that have
	// to be added at least in any optimal solution. (\a upperBoundC is non-positive)
	return (upperBoundO + upperBoundC);
}

void Master::nodeDistances(node u, NodeArray<NodeArray<int> > &dist) {

	// Computing the graphtheoretical distances of node u
	NodeArray<bool> visited(*m_G);
	List<node> queue;
	visited.fill(false);
    visited[u] = true;
    int nodesVisited = 1;
    adjEntry adj;
    node v;
    forall_adj(adj,u) {
    	visited[adj->twinNode()] = true;
    	nodesVisited++;
    	dist[u][adj->twinNode()] += 1;
    	queue.pushBack(adj->twinNode());
    }
    while (!queue.empty() || nodesVisited!=m_G->numberOfNodes()) {
    	v = queue.front();
    	queue.popFront();
    	forall_adj(adj,v) {
    		if (!visited[adj->twinNode()]) {
    			visited[adj->twinNode()] = true;
    			nodesVisited++;
    			dist[u][adj->twinNode()] += (dist[u][v]+1);
    			queue.pushBack(adj->twinNode());
    		}
    	}
    }
}

bool Master::goodVar(node a, node b) {
	return true; //add all variables even if they are bad (paper submission)
	Logger::slout() << "Good Var? " << a << "->" << b << ": ";
	GraphCopy GC(*m_G);
	edge e = GC.newEdge(GC.copy(a),GC.copy(b));
	BoyerMyrvold bm;
	int ret =  bm.isPlanarDestructive(GC);
	Logger::slout() << ret << "\n";
	return ret;
}

void Master::initializeOptimization() {

	//we don't try heuristic improvement (edge addition) in MaxCPlanarSub
	//when only checking c-planarity
	if (m_checkCPlanar) {
		heuristicLevel(0);
		//TODO Shortcut: lowerbound number original edges (-1) if not pricing
		//enumerationStrategy(BreadthFirst);
	}

	if (pricing())
		varElimMode(NoVarElim);
	else
		varElimMode(ReducedCost);
	conElimMode(Basic);
	if(pricing())
		pricingFreq(1);

	//----------------------------Creation of Variables--------------------------------//

	// Lists for original and connection edges
	List<EdgeVar*> origVars;    // MCh: ArrayBuffer would speed this up
	List<EdgeVar*> connectVars; // MCh: ArrayBuffer would speed this up

	//cluster connectivity only necessary if there are clusters (not for max planar subgraph)
	bool toBeConnected = (!( (m_C->numberOfClusters() == 1) && (isConnected(*m_G)) ) );

    int nComplete = (m_G->numberOfNodes()*(m_G->numberOfNodes()-1))/2;
    int nConnectionEdges = nComplete - m_G->numberOfEdges();

    double perturbMe = (m_usePerturbation)? 0.2*m_epsilon : 0;
    m_deltaCount = nConnectionEdges;
   	m_delta  = (m_deltaCount > 0) ? perturbMe/m_deltaCount : 0;
    double coeff;

    // In order not to place the initial upper bound too low,
    // we use the largest connection edge coefficient for each C-edge
    // to lower the upper bound (since these coeffs are negative,
    // this corresponds to the coeff that is closest to 0).
    m_largestConnectionCoeff = 0.8*m_epsilon;
    m_varsMax = 0;
    node u,v;
    forall_nodes(u,*m_G) {
        v = u->succ();
        while (v!=NULL) {//todo could skip searchedge if toBeConnected
            if(m_G->searchEdge(u,v))
            	origVars.pushBack(new EdgeVar(this,1.0+rand()*perturbMe,EdgeVar::ORIGINAL,u,v));
            else if (toBeConnected) {
            	if( (!m_checkCPlanar) || goodVar(u,v)) {
	            	if(pricing())
	            		m_inactiveVariables.pushBack(nodePair(u,v));
	            	else
	            		connectVars.pushBack( new EdgeVar(this, nextConnectCoeff(), EdgeVar::CONNECT, u,v) );
            	}
            	++m_varsMax;
            }
            v = v->succ();
        }
    }
    m_varsPotential = m_inactiveVariables.size();

	//-------------------Creation of ChunkConnection-Constraints------------------------//

    int nChunks = 0;

	List<ChunkConnection*> constraintsCC;

	// The Graph, in which each cluster-induced Graph is temporarily stored
	Graph subGraph;

	// Since the function inducedSubGraph(..) creates a new Graph \a subGraph, the original
	// nodes have to be mapped to the copies. This mapping is stored in \a orig2new.
	NodeArray<node> orig2new;

	// Iterate over all clusters of the Graph
	ListConstIterator<node> it;
	cluster c;
	forall_clusters(c,*m_C) {

		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes);

		// Compute the cluster-induced Subgraph
		it = clusterNodes.begin();
		inducedSubGraph(*m_G, it, subGraph, orig2new);

		// Compute the number of connected components
		NodeArray<int> components(subGraph);
		int nCC = connectedComponents(subGraph,components);
		nChunks+=nCC;
		// If the cluster consists of more than one connected component,
		// ChunkConnection constraints can be created.
		if (nCC > 1) {

			// Determine each connected component (chunk) of the current cluster-induced Graph
			for (int i=0; i<nCC; ++i) {

				ArrayBuffer<node> cC(subGraph.numberOfNodes());
				ArrayBuffer<node> cCComplement(subGraph.numberOfNodes());
				node v;
				forall_nodes(v,*m_G/*subGraph*/) {
					node n = orig2new[v];
					if(n) {
						if (components[n] == i) cC.push(v);
						else cCComplement.push(v);
					}
				}
				// Creating corresponding constraint
				constraintsCC.pushBack(new ChunkConnection(this, cC, cCComplement));
				// Avoiding duplicates if cluster consists of 2 chunks
				if (nCC == 2) break;

			}//connected component has been processed
		}//end if(nCC > 1)
	}//end forall_clusters
	if(pricing())
		generateVariablesForFeasibility(constraintsCC, connectVars);

	//------------Creation of MinimalClusterConnection-Constraints---------------//
	/*
	List<MinimalClusterConnection*> constraintsMCC;
	cluster succ;
	ClusterArray<bool> connected(*m_C);
	// For each cluster run through all cluster vertices and check if
	// they have an outgoing edge to a different cluster.
	// In this case, mark that cluster in connected as true.
	forall_clusters(c,*m_C) {

		succ = c->succ();
		connected.fill(false);
		List<node> clusterNodes;
		c->getClusterNodes(clusterNodes);
		ListConstIterator<node> it;
		for (it=clusterNodes.begin(); it.valid(); ++it) {
			adjEntry adj;
			forall_adj(adj,*it) {
				if(m_C->clusterOf(adj->twinNode()) != c) {
					connected[m_C->clusterOf(adj->twinNode())] = true;
				}
			}
		}

		//checking if there is an entry in \a connected of value false
		//if so, cluster \a c is not connected to this one and a constraint is created
		List<nodePair> mcc_edges;
		while(succ!=0) {
			if (!connected[succ]) {
				ListConstIterator<node> it;
				//determine all nodePairs between \a c and \a succ and add them to list \a mcc_edges
				for (it=clusterNodes.begin(); it.valid(); ++it) {
					adjEntry adj;
					nodePair np;
					forall_adj(adj,*it) {
						if (m_C->clusterOf(adj->twinNode()) == succ) {
							np.v1 = (*it);
							np.v2 = adj->twinNode();
							mcc_edges.pushBack(np);
						}
					}
				}
				Logger::slout() << "mcc_edges: " << mcc_edges.size() << endl;
				//create new constraint and put it into list \a constraintsMCC
				constraintsMCC.pushBack(new MinimalClusterConnection(this,mcc_edges));
				mcc_edges.clear();
			}
			succ = succ->succ();
		}
	}
	*/

	//------------Creation of MaxPlanarEdges-Constraints---------------//

	List<MaxPlanarEdgesConstraint*> constraintsMPE;

	int nMaxPlanarEdges = 3*m_G->numberOfNodes() - 6;
	constraintsMPE.pushBack(new MaxPlanarEdgesConstraint(this,nMaxPlanarEdges));

	List<node> clusterNodes;
	List<nodePair> clusterEdges;
	forall_clusters(c,*m_C) {

		if (c == m_C->rootCluster()) continue;
		clusterNodes.clear(); clusterEdges.clear();
		c->getClusterNodes(clusterNodes);
		if (clusterNodes.size() >= 4) {
			nodePair np;
			ListConstIterator<node> it;
			ListConstIterator<node> it_succ;
			for (it=clusterNodes.begin(); it.valid(); ++it) {
				it_succ = it.succ();
				while (it_succ.valid()) {
					np.v1 = (*it); np.v2 = (*it_succ);
					clusterEdges.pushBack(np);
					it_succ++;
				}
			}
			int maxPlanarEdges = 3*clusterNodes.size() - 6;
			constraintsMPE.pushBack(new MaxPlanarEdgesConstraint(this,maxPlanarEdges,clusterEdges));
		}
	}


	//------------------------Adding Constraints to the Pool---------------------//

	// Adding constraints to the standardpool
	ABA_BUFFER<ABA_CONSTRAINT *> initConstraints(this,constraintsCC.size()/*+constraintsMCC.size()*/+constraintsMPE.size());

	ListConstIterator<ChunkConnection*> ccIt;
	updateAddedCCons(constraintsCC.size());
	for (ccIt = constraintsCC.begin(); ccIt.valid(); ++ccIt) {
		initConstraints.push(*ccIt);
	}
//	ListConstIterator<MinimalClusterConnection*> mccIt;
//	for (mccIt = constraintsMCC.begin(); mccIt.valid(); ++mccIt) {
//		initConstraints.push(*mccIt);
//	}
	ListConstIterator<MaxPlanarEdgesConstraint*> mpeIt;
	for (mpeIt = constraintsMPE.begin(); mpeIt.valid(); ++mpeIt) {
		initConstraints.push(*mpeIt);
	}
	//output these constraints in a file that can be read by the module
	if (m_porta)
	{

		ofstream ofs(getStdConstraintsFileName());
		if (!ofs) cerr << "Could not open output stream for PORTA constraints file\n";
		else
		{
			ofs << "# Chunkconnection constraints\n";
			//holds the coefficients in a single constraint for all vars defined
			//so far
			List<double> theCoeffs;
			ListIterator<ChunkConnection*> csIt;
			for (csIt = constraintsCC.begin(); ccIt.valid(); ++ccIt) {
				getCoefficients((*csIt), origVars, connectVars, theCoeffs);
				ListConstIterator<double> dIt = theCoeffs.begin();
				while (dIt.valid())
				{
					ofs << (*dIt) << " ";
					dIt++;
				}//while
				//check csense here
				ofs << ">= " << (*csIt)->rhs();
				ofs << "\n";
			}
//			ofs << "# MinimalClusterconnection constraints\n";
//			ListConstIterator<MinimalClusterConnection*> mccIt;
//			for (mccIt = constraintsMCC.begin(); mccIt.valid(); ++mccIt) {
//				getCoefficients((*mccIt), origVars, connectVars, theCoeffs);
//				ListConstIterator<double> dIt = theCoeffs.begin();
//				while (dIt.valid())
//				{
//					ofs << (*dIt) << " ";
//					dIt++;
//				}//while
//				ofs << "<= " << (*mccIt)->rhs();
//				ofs << "\n";
//			}
			ofs << "# MaxPlanarEdges constraints\n";
			ListConstIterator<MaxPlanarEdgesConstraint*> mpeIt;
			for (mpeIt = constraintsMPE.begin(); mpeIt.valid(); ++mpeIt) {
				getCoefficients((*mpeIt), origVars, connectVars, theCoeffs);
				ListConstIterator<double> dIt = theCoeffs.begin();
				while (dIt.valid())
				{
					ofs << (*dIt) << " ";
					dIt++;
				}//while
				ofs << "<= " << (*mpeIt)->rhs();
				ofs << "\n";
			}
			ofs.close();
		}
	}

	//---------------------Adding Variables to the Pool---------------------//

	// Adding variables to the standardpool
	ABA_BUFFER<ABA_VARIABLE *> edgeVariables(this,origVars.size()+connectVars.size());
	ListConstIterator<EdgeVar*> eIt;
	for (eIt = origVars.begin(); eIt.valid(); ++eIt) {
		edgeVariables.push(*eIt);
	}
	for (eIt = connectVars.begin(); eIt.valid(); ++eIt) {
		edgeVariables.push(*eIt);
	}


	//---------------------Initializing the Pools---------------------------//

	int poolsize = (getGraph()->numberOfNodes() * getGraph()->numberOfNodes());
	if (m_useDefaultCutPool)
		initializePools(initConstraints, edgeVariables, m_nMaxVars, poolsize, true);
	else
	{
		initializePools(initConstraints, edgeVariables, m_nMaxVars, 0, false);
		//TODO: How many of them?
		m_cutConnPool = new ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE>(this, poolsize, true);
		m_cutKuraPool = new ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE>(this, poolsize, true);
	}


	//---------------------Initialize Upper Bound---------------------------//

	//if we check only for c-planarity, we cannot set bounds
	if (!m_checkCPlanar)
	{
	 	double upperBound = heuristicInitialUpperBound(); // TODO-TESTING
		dualBound(upperBound); // TODO-TESTING

	//---------------------Initialize Lower Bound---------------------------//

		primalBound(heuristicInitialLowerBound()); // TODO-TESTING
	} else {
		//primalBound(-m_G->numberOfNodes()*3);
	}

	//----------------------Setting Parameters------------------------------//

//	conElimMode(ABA_MASTER::NonBinding);
	outLevel(m_out);
	maxCpuTime(*m_maxCpuTime);

	Logger::ssout() << "#Nodes: " << m_G->numberOfNodes() << "\n";
	Logger::ssout() << "#Edges: " << m_G->numberOfEdges() << "\n";
	Logger::ssout() << "#Clusters: " << m_C->numberOfClusters() << "\n";
	Logger::ssout() << "#Chunks: " << nChunks << "\n";


}

// returns coefficients of all variables in orig and connect in constraint con
// as list coeffs
void Master::getCoefficients(ABA_CONSTRAINT* con,  const List<EdgeVar* > & orig,
	const List<EdgeVar* > & connect, List<double> & coeffs)
{
	coeffs.clear();
	ListConstIterator<EdgeVar*> cIt = orig.begin();
	while (cIt.valid())
	{
		coeffs.pushBack(con->coeff(*cIt));
		cIt++;
	}//while
	cIt = connect.begin();
	while (cIt.valid())
	{
		coeffs.pushBack(con->coeff(*cIt));
		cIt++;
	}//while

}


//output statistics
//and change the list of deleted edges in case only c-planarity is tested
//(to guarantee that the list is non-empty if input is not c-planar)
void Master::terminateOptimization() {
	Logger::slout() << "=================================================\n";
	Logger::slout() << "Terminate Optimization:\n";
	Logger::slout() << "(primal Bound: " << primalBound() << ")\n";
	Logger::slout() << "(dual Bound: " << dualBound() << ")\n";
	Logger::slout() << "*** " << (m_deletedOriginalEdges.size() == 0 ? "" : "NON ") << "C-PLANAR ***\n";
	Logger::slout() << "*** " << (feasibleFound() ? "" : "NO ") << "feasible solution found ***\n";
	Logger::slout() << "=================================================\n";

	Logger::ssout() << "\n";

	Logger::ssout() << "C-Planar: " << (feasibleFound() && (m_deletedOriginalEdges.size() == 0)) << "\n";
	Logger::ssout() << "FeasibleFound: " << feasibleFound() << "\n";
	Logger::ssout() << "Time: "<< getDoubleTime(totalTime()) << "\n";
	Logger::ssout() << "LP-Time: " << getDoubleTime(lpSolverTime()) << "\n";
	Logger::ssout() << "\n";
	Logger::ssout() << "#BB-nodes: " << nSub() << "\n";
	Logger::ssout() << "#LP-relax: " << m_solvesLP << "\n";
	Logger::ssout() << "Added Edges: " <<m_connectionOneEdges.size()<<"\n";

	Logger::ssout() << "#Cut Constraints: " << m_nCConsAdded << "\n";
	Logger::ssout() << "#Kura Constraints: " << m_nKConsAdded << "\n";
	Logger::ssout() << "#Vars-init: " << m_varsInit << "\n";
	Logger::ssout() << "#Vars-used: " << m_varsAdded << "\n";
	Logger::ssout() << "#Vars-potential: " << m_varsPotential << "\n";
	Logger::ssout() << "#Vars-max: " << m_varsMax << "\n";
	Logger::ssout() << "#Vars-cut: " << m_varsCut << "\n";
	Logger::ssout() << "#Vars-kurarepair: " << m_varsKura << "\n";
	Logger::ssout() << "#Vars-price: " << m_varsPrice << "\n";
	Logger::ssout() << "#Vars-branch: " << m_varsBranch << "\n";
	Logger::ssout() << "#Vars-unused: " << m_inactiveVariables.size() << "\n";
	Logger::ssout() << "KuraRepair-Stat: <";
	for(int i =0; i<m_repairStat.size(); ++i) {
		Logger::ssout() << m_repairStat[i] << ",";
	}
	Logger::ssout() << ">\n";

	node n,m;
	edge e;
	forall_nodes(n, *m_G) {
		forall_nodes(m, *m_G) {
			if(m->index()<=n->index()) continue;
			forall_adj_edges(e, n) {
				if(e->opposite(n)==m) {
					Logger::slout() << "ORIG: " << n << "-" << m << "\n";
					continue;
				}
			}
		}
	}
	forall_nodes(n, *m_G) {
		forall_nodes(m, *m_G) {
			if(m->index()<=n->index()) continue;
			forall_adj_edges(e, n) {
				if(e->opposite(n)==m) {
					goto wup;
				}
			}
			forall_listiterators(nodePair, it, m_inactiveVariables) {
				if( ((*it).v1==n && (*it).v2==m) || ((*it).v2==n && (*it).v1==m)) {
					goto wup;
				}
			}
			Logger::slout() << "CONN: " << n << "-" << m << "\n";
			wup:;
		}
	}

	globalPrimalBound = primalBound();
	globalDualBound = dualBound();
}

void Master::generateVariablesForFeasibility(const List<ChunkConnection*>& ccons, List<EdgeVar*>& connectVars) {
	List<ChunkConnection*> cpy(ccons);
//	forall_listiterators(ChunkConnection*, ccit, cpy) {
//		(*ccit)->printMe();
//	}

	ArrayBuffer<ListIterator<nodePair> > creationBuffer(ccons.size());
	forall_nonconst_listiterators(nodePair, npit, m_inactiveVariables) {
		bool select = false;
//		(*npit).printMe();
		ListIterator<ChunkConnection*> ccit = cpy.begin();
		while(ccit.valid()) {
			if((*ccit)->coeff(*npit)) {
				ListIterator<ChunkConnection*> delme = ccit;
				++ccit;
				cpy.del(delme);
				select = true;
			} else
				++ccit;
		}
		if(select) {
//			Logger::slout() << "<--CREATE";
			creationBuffer.push(npit);
		}
		if(cpy.size()==0) break;
	}
//	forall_listiterators(ChunkConnection*, ccit, cpy) {
//		(*ccit)->printMe();
//	}
	OGDF_ASSERT(cpy.size()==0);
	Logger::slout() << "Creating " << creationBuffer.size() << " Connect-Variables for feasibility\n";
	m_varsInit = creationBuffer.size();
	// realize creationList
	for(int i = creationBuffer.size(); i-->0;) {
	  connectVars.pushBack( createVariable( creationBuffer[i] ) );
	}
}

#endif // USE_ABACUS
