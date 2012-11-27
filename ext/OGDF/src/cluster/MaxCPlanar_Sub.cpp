/*
 * $Revision: 2611 $
 *
 * last checkin:
 *   $Author: klein $
 *   $Date: 2012-07-16 10:50:21 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the subproblem class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
 * Contains separation algorithms as well as primal heuristics.
 *
 * \authors Markus Chimani, Mathias Jansen, Karsten Klein
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

#include <ogdf/internal/cluster/MaxCPlanar_Sub.h>
#include <ogdf/internal/cluster/Cluster_EdgeVar.h>
#include <ogdf/internal/cluster/KuratowskiConstraint.h>
#include <ogdf/internal/cluster/Cluster_CutConstraint.h>
#include <ogdf/internal/cluster/Cluster_ChunkConnection.h>
#include <ogdf/internal/cluster/Cluster_MaxPlanarEdges.h>
#include <ogdf/graphalg/MinimumCut.h>
#include <ogdf/basic/BinaryHeap2.h>
#include <ogdf/cluster/CconnectClusterPlanar.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/MinHeap.h>
#include <ogdf/basic/ArrayBuffer.h>

#include <abacus/lpsub.h>
#include <abacus/setbranchrule.h>

//output intermediate results when new sons are generated
//#define IM_OUTPUT
#ifdef IM_OUTPUT
#include <fstream>
#include <iostream>
#include <ogdf/basic/GraphAttributes.h>
#endif

using namespace ogdf;


Sub::Sub(ABA_MASTER *master) : ABA_SUB(master,500,((Master*)master)->m_inactiveVariables.size(),2000,false),bufferedForCreation(10),detectedInfeasibility(false),inOrigSolveLp(false) {
	m_constraintsFound = false;
	m_sepFirst = false;
//	for(int k=0; k<nVar(); ++k) {
//		EdgeVar* ev = dynamic_cast<EdgeVar*>(variable(k));
//		if(ev->theEdgeType()==EdgeVar::ORIGINAL)
//			fsVarStat(k)->status()->status(ABA_FSVARSTAT::SetToUpperBound);
//	}

		// only output below...
#ifdef OGDF_DEBUG
	Logger::slout() << "Construct 1st Sub\n";
	Logger::slout() << nVar() << " " << nCon() << "\n";
	int i;
	for(i=0; i<nVar(); ++i) {
		(dynamic_cast<EdgeVar*>(variable(i)))->printMe(Logger::slout()); Logger::slout() << "\n";
	}
	for(i=0; i<nCon(); ++i) {
		ABA_CONSTRAINT* c = constraint(i);
		ChunkConnection* ccon = dynamic_cast<ChunkConnection*>(c);
		MaxPlanarEdgesConstraint* cmax = dynamic_cast<MaxPlanarEdgesConstraint*>(c);
		if(ccon) {
			Logger::slout() << "ChunkConstraint: Chunk=";
			int j;
			forall_arrayindices(j,ccon->m_chunk) {
				Logger::slout() << ccon->m_chunk[j] << ",";
			}
			Logger::slout() << " Co-Chunk=";
			forall_arrayindices(j,ccon->m_cochunk) {
				Logger::slout() << ccon->m_cochunk[j] << ",";
			}
			Logger::slout() << "\n";
		} else if(cmax) {
			Logger::slout() << "MaxPlanarEdgesConstraint: rhs=" << cmax->rhs() << ", graphCons=" << cmax->m_graphCons << ", nodePairs=";
			forall_listiterators(nodePair, it, cmax->m_edges) {
				Logger::slout() << "("<<(*it).v1<<","<<(*it).v2<<")";
			}
			Logger::slout() << "\n";
		} else {
			Logger::slout() << "** Unexpected Constraint\n";
		}
	}
#endif //OGDF_DEBUG
}


Sub::Sub(ABA_MASTER *master,ABA_SUB *father,ABA_BRANCHRULE *rule,List<ABA_CONSTRAINT*>& criticalConstraints):
ABA_SUB(master,father,rule), bufferedForCreation(10),detectedInfeasibility(false),inOrigSolveLp(false) {
	m_constraintsFound = false;
	m_sepFirst = false;
	criticalSinceBranching.exchange(criticalConstraints); // fast load
	Logger::slout() << "Construct Child Sub " << id() << "\n";
}


Sub::~Sub() {}


ABA_SUB *Sub::generateSon(ABA_BRANCHRULE *rule) {
	//dualBound_ = realDualBound;

	const double minViolation = 0.001; // value fixed from abacus...
	#ifdef IM_OUTPUT
		if (father() == 0)
		{
			ofstream output("Intermediate.txt",ios::app);
			if (!output) cerr<<"Could not open file for intermediate results!\n";

			output << "Intermediate Results at branching in Root Node\n";
			output << "Number of  graph nodes: " << ((Master*)master_)->getGraph()->numberOfNodes() <<"\n";
			output << "Number of  graph edges: " << ((Master*)master_)->getGraph()->numberOfEdges() <<"\n";

			output << "Current number of active variables: " << nVar() << "\n";
			output << "Current number of active constraints: " << nCon() << "\n";

			output << "Current lower bound: " << lowerBound()<<"\n";
			output << "Current upper bound: " << upperBound()<<"\n";

			output << "Global primal bound: " << ((Master*)master_)->getPrimalBound()<<"\n";
			output << "Global dual bound: " << ((Master*)master_)->getDualBound()<<"\n";

			output << "Added K cons: " << ((Master*)master_)->addedKConstraints()<<"\n";
			output << "Added C cons: " << ((Master*)master_)->addedCConstraints()<<"\n";
			output.close();

			GraphAttributes GA(*(((Master*)master_)->getGraph()), GraphAttributes::edgeStyle |
				GraphAttributes::edgeDoubleWeight | GraphAttributes::edgeColor |
				GraphAttributes::edgeGraphics);

			edge e;
			for (int i=0; i<((Master*)master_)->nVar(); ++i)
			//forall_edges(e,  *(((Master*)master_)->getGraph())
			{
				EdgeVar *e = (EdgeVar*)variable(i);
				if (e->theEdgeType() == EdgeVar::ORIGINAL)
				{
					GA.doubleWeight(e->theEdge()) = xVal(i);
					GA.edgeWidth(e->theEdge()) = 10.0*xVal(i);
					if (xVal(i) == 1.0) GA.colorEdge(e->theEdge()) = "#FF0000";
				}//if real edge
			}//forall variables
			GA.writeGML("WeightedIntermediateGraph.gml");
		}
	#endif

	List< ABA_CONSTRAINT* > criticalConstraints;
	if (master()->pricing())
	{
		//ABA_SETBRANCHRULE* srule = (ABA_SETBRANCHRULE*)(rule);
		ABA_SETBRANCHRULE* srule;
		srule = dynamic_cast<ABA_SETBRANCHRULE*>(rule);
		OGDF_ASSERT( srule ); // hopefully no other branching stuff...
		//Branching by setting a variable to 0 may
		//result in infeasibility of the current system
		//because connectivity constraints may not be feasible
		//with the current set of variables
		if(!srule->setToUpperBound()) { // 0-branching
			int varidx = srule->variable();
			EdgeVar* var = (EdgeVar*)variable(varidx);

			Logger::slout() << "FIXING VAR: ";
			var->printMe(Logger::slout());
			Logger::slout() << "\n";

			for(int i = nCon(); i-->0;) {
				ABA_CONSTRAINT* con = constraint(i);
				double coeff = con->coeff(var);
				if(con->sense()->sense()==ABA_CSENSE::Greater && coeff>0.99) {
					// check: yVal gives the slack and is always negative or 0
					double slk;
					//slk = yVal(i);
					slk = con->slack(actVar(),xVal_);
					//quick hack using ABACUS value, needs to be corrected
					if (slk > 0.0 && slk < minViolation)
					{
						slk = 0.0;
#ifdef OGDF_DEBUG
						cout << "Set slack to 0.0\n";
#endif
					}
					if(slk > 0.0) {
						Logger::slout() << "ohoh..." << slk << " "; var->printMe(Logger::slout()); Logger::slout()<<flush;
					}
					OGDF_ASSERT( slk <= 0.0 )
					double zeroSlack = slk+xVal(varidx)*coeff;
					if(zeroSlack > 0.0001) { // setting might introduce infeasibility
	// TODO: is the code below valid (in terms of theory) ???
	// "es reicht wenn noch irgendeine nicht-auf-0-fixierte variable im constraint existiert, die das rettet
	// mögliches problem: was wenn alle kanten bis auf die aktive kante in einem kuratowski constraint
	// auf 1 fixiert sind (zB grosse teile wegen planaritätstest-modus, und ein paar andere wg. branching).
	//
	//					bool good = false; // does there exist another good variable?
	//					for(int j = nVar(); j-->0;) {
	//						if(con->coeff(variable[j])>0.99 && VARIABLE[j]-NOT-FIXED-TO-0) {
	//							good = true;
	//							break;
	//						}
	//					}
	//					if(!good)
						criticalConstraints.pushBack(con);
					}
				}
			}
		}
	}//pricing

	return new Sub(master_, this, rule, criticalConstraints);
}


int Sub::selectBranchingVariable(int &variable) {
	//dualBound_ = realDualBound;

	return ABA_SUB::selectBranchingVariable(variable);

	/// the stuff below does NOTHING!

	int variableABA;
	int found = ABA_SUB::selectBranchingVariable(variableABA);
	if (found == 0) {
		//Edge *e = (Edge*)(this->variable(variableABA));
		//cout << "Branching variable is: " << (e->theEdgeType()==ORIGINAL ? "Original" : "Connect") << " Edge (";
		//cout << e->sourceNode()->index() << "," << e->targetNode()->index() << ") having value: ";
		//cout << xVal(variableABA) << " and coefficient " << ((Edge*)this->variable(variableABA))->objCoeff() << endl;
		variable = variableABA;
		return 0;
	}
	return 1;
}


int Sub::selectBranchingVariableCandidates(ABA_BUFFER<int> &candidates) {
//	if(master()->m_checkCPlanar)
//		return ABA_SUB::selectBranchingVariableCandidates(candidates);

	ABA_BUFFER<int> candidatesABA(master_,1);
	int found = ABA_SUB::selectBranchingVariableCandidates(candidatesABA);

    if (found == 1) return 1;
    else {
    	int i = candidatesABA.pop();
    	EdgeVar *e = (EdgeVar*)variable(i);
    	if (e->theEdgeType() == EdgeVar::ORIGINAL) {
    		OGDF_ASSERT( !master()->m_checkCPlanar )
    		candidates.push(i);
    		return 0;
    	} else {

    		// Checking if a more appropriate o-edge can be found according to the global parameter
    		//\a branchingOEdgeSelectGap. Candidates are stored in list \a oEdgeCandits.
    		List<int> oEdgeCandits;
    		for (int j=0; j<nVar(); ++j) {
    			EdgeVar *e = (EdgeVar*)variable(j);
    			if (e->theEdgeType() == EdgeVar::ORIGINAL) {
    				if ( (xVal(j) >= (0.5-((Master*)master_)->branchingOEdgeSelectGap())) &&
    					 (xVal(j) <= (0.5+((Master*)master_)->branchingOEdgeSelectGap())) ) {
    					 	oEdgeCandits.pushBack(j);
    					 }
    			}
    		}
    		if (oEdgeCandits.empty()) {
    			candidates.push(i);
    			return 0;
    		} else {

	    		// Choose one of those edges randomly.
	    		int random = randomNumber(0,oEdgeCandits.size()-1);
	    		int index = random;
	    		ListConstIterator<int> it = oEdgeCandits.get(index);
	    		candidates.push(*it);
	    		return 0;
    		}
    	}
    }
}


void Sub::updateSolution() {

	List<nodePair> originalOneEdges;
	List<nodePair> connectionOneEdges;
	List<edge> deletedEdges;
	nodePair np;
//	for (int i=0; i<((Master*)master_)->nVars(); ++i) {
	for (int i=0; i<nVar(); ++i) {
		if (xVal(i) >= 1.0-(master_->eps())) {

			EdgeVar *e = (EdgeVar*)variable(i);
			np.v1 = e->sourceNode();
			np.v2 = e->targetNode();
			if (e->theEdgeType() == EdgeVar::ORIGINAL) originalOneEdges.pushBack(np);
			else connectionOneEdges.pushBack(np);
		}
		else {

			EdgeVar *e = (EdgeVar*)variable(i);
			if (e->theEdgeType() == EdgeVar::ORIGINAL) {
				deletedEdges.pushBack(e->theEdge());
			}
		}
	}
#ifdef OGDF_DEBUG
	((Master*)master_)->m_solByHeuristic = false;
#endif
	((Master*)master_)->updateBestSubGraph(originalOneEdges,connectionOneEdges,deletedEdges);
}


double Sub::subdivisionLefthandSide(SListConstIterator<KuratowskiWrapper> kw, GraphCopy *gc) {

	double lefthandSide = 0.0;
	node v,w;
//	for (int i=0; i<((Master*)master_)->nVars(); ++i) {
	for (int i=0; i<nVar(); ++i) {
		EdgeVar *e = (EdgeVar*)variable(i);
		v = e->sourceNode();
		w = e->targetNode();
		SListConstIterator<edge> it;
		for (it=(*kw).edgeList.begin(); it.valid(); ++it) {
			if ( ((*it)->source() == gc->copy(v) && (*it)->target() == gc->copy(w) ) ||
			 	((*it)->source() == gc->copy(w) && (*it)->target() == gc->copy(v) ) ) {
			 		lefthandSide += xVal(i);
			 	}
		}
	}
	return lefthandSide;
}


///////////////////////////////////////////////////
//					HEURISTIC
///////////////////////////////////////////////////


int Sub::getArrayIndex(double lpValue) {
	int index = 0;
	double x = 1.0;
	double listRange = (1.0/((Master*)master_)->numberOfHeuristicPermutationLists());
	while (x >= lpValue) {
		x -= listRange;
		if (lpValue >= x) return index;
		index++;
	}
	return index;
}


void Sub::childClusterSpanningTree(
	GraphCopy &GC,
	List<edgeValue> &clusterEdges,
	List<nodePair> &MSTEdges)
{
	// Testing and adding of edges in \a clusterEdges is performed randomized.
	// Dividing edges into original and connection 1-edges and fractional edges.
	// Tree is built using edges in this order.
	List<edgeValue> oneOEdges;
	List<edgeValue> oneToFracBoundOEdges;
	List<edgeValue> leftoverEdges;
	ListConstIterator<edgeValue> it = clusterEdges.begin();
	while(it.valid()) {

		if ((*it).lpValue >= (1.0-master_->eps())) {
			if ((*it).original) oneOEdges.pushBack(*it);
			else leftoverEdges.pushBack(*it);
		} else if ((*it).lpValue >= ((Master*)master_)->getHeuristicFractionalBound()) {
				if ((*it).original) oneToFracBoundOEdges.pushBack(*it);
				else leftoverEdges.pushBack(*it);
		}
		else leftoverEdges.pushBack(*it);
		it++;
	}

	// Try to create spanning tree with original 1-edges.
	if(oneOEdges.size() > 1) oneOEdges.permute();
	edge newEdge;
	node v,w;
	nodePair np;
	for (it=oneOEdges.begin(); it.valid(); ++it) {
		v = (*it).src;
		w = (*it).trg;
		newEdge = GC.newEdge(GC.copy(v),GC.copy(w));
		//Union-Find could make this faster...
		if (!isAcyclicUndirected(GC)) GC.delEdge(newEdge);
		else {
			np.v1 = v; np.v2 = w;
			MSTEdges.pushBack(np);
		}
		if (GC.numberOfEdges() == GC.numberOfNodes()-1) return;
	}
	//is there a case this if would return without having returned above?
	if (isConnected(GC)) return;

	// Create two Arrays of lists containing nodePairs that have "similar" fractional value.
	// "Similar" is defined by the parameter \a Master->nPermutationLists.
	double listRange = (1.0/((Master*)master_)->numberOfHeuristicPermutationLists());
	double range = 0.0;
	int indexCount = 0;
	while ( (1.0-((Master*)master_)->getHeuristicFractionalBound()) > range ) {
		indexCount++;
		range += listRange;
	}
	Array<List<edgeValue> > oEdgePermLists(0,indexCount);
	Array<List<edgeValue> > leftoverPermLists(0,((Master*)master_)->numberOfHeuristicPermutationLists());

	// Distributing edges in \a oneToFracBoundOEdges and \a leftoverEdges among the permutation lists.
	int index;
	for (it=oneToFracBoundOEdges.begin(); it.valid(); ++it) {
		index = getArrayIndex((*it).lpValue);
		oEdgePermLists[index].pushBack(*it);
	}
	for (it=leftoverEdges.begin(); it.valid(); ++it) {
		index = getArrayIndex((*it).lpValue);
		leftoverPermLists[index].pushBack(*it);
	}


	for (int i=0; i<oEdgePermLists.size(); ++i) {
		if (oEdgePermLists[i].size() > 1) oEdgePermLists[i].permute();

		for (it=oEdgePermLists[i].begin(); it.valid(); ++it) {
			v = (*it).src;
			w = (*it).trg;
			newEdge = GC.newEdge(GC.copy(v),GC.copy(w));
			if (!isAcyclicUndirected(GC)) {
				GC.delCopy(newEdge);
			}
			else {
				np.v1 = v; np.v2 = w;
				MSTEdges.pushBack(np);
			}
			if (GC.numberOfEdges() == GC.numberOfNodes()-1) return;
		}

		if (isConnected(GC)) return;
	}

	for (int i=0; i<leftoverPermLists.size(); ++i) {
		if (leftoverPermLists[i].size() > 1) leftoverPermLists[i].permute();

		for (it=leftoverPermLists[i].begin(); it.valid(); ++it) {
			v = (*it).src;
			w = (*it).trg;
			newEdge = GC.newEdge(GC.copy(v),GC.copy(w));
			if (!isAcyclicUndirected(GC)) {
				GC.delCopy(newEdge);
			}
			else {
				np.v1 = v; np.v2 = w;
				MSTEdges.pushBack(np);
			}
			if (GC.numberOfEdges() == GC.numberOfNodes()-1) return;
		}

		if (isConnected(GC)) return;
	}

	//Todo: What is happening here? Do we have to abort?
	if (!isConnected(GC)) cerr << "Error. For some reason no spanning tree could be computed" << endl << flush;
	return;
}


void Sub::clusterSpanningTree(
	ClusterGraph &C,
	cluster c,
	ClusterArray<List<nodePair> > &treeEdges,
	ClusterArray<List<edgeValue> > &clusterEdges)
{
	//looks like a minspantree problem with some weights based
	//on edge status and LP-value, right?
	GraphCopy *GC;
	if (c->cCount() == 0) { // Cluster is a leaf, so MST can be computed.

		// Create a cluster induced GraphCopy of \a G and delete all edges.
		GC = new GraphCopy(C.getGraph());
		node v = GC->firstNode();
		node v_succ;
		while (v!=0) {
			v_succ = v->succ();
			if (C.clusterOf(GC->original(v)) != c) {
				GC->delNode(v);
			}
			v = v_succ;
		}
		edge e = GC->firstEdge();
		edge e_succ;
		while (e!=0) {
			e_succ = e->succ();
			GC->delCopy(e);
			e = e_succ;
		}
		childClusterSpanningTree(*GC,clusterEdges[c],treeEdges[c]);
		delete GC;
		return;
	}

	// If cluster \a c has further children, they have to be processed first.
	ListConstIterator<cluster> cit;
	for (cit=c->cBegin(); cit.valid(); ++cit) {

		clusterSpanningTree(C,(*cit),treeEdges,clusterEdges);

		// Computed treeEdges for the child clusters have to be added to \a treeEdges for current cluster.
		ListConstIterator<nodePair> it;
		for (it=treeEdges[(*cit)].begin(); it.valid(); ++it) {
			treeEdges[c].pushBack(*it);
		}
	}

	// A spanning tree has been computed for all children of cluster \a c.
	// Thus, a spanning tree for \a c can now be computed.
	// \a treeEdges[c] now contains all edges that have been previously added
	// during the computation of the trees for its child clusters.

	// Create GraphCopy induced by nodes in \a nodes.
	GC = new GraphCopy(C.getGraph());
	NodeArray<bool> isInCluster(*GC,false);
	List<node> clusterNodes;
	c->getClusterNodes(clusterNodes);
	ListConstIterator<node> it;
	for (it=clusterNodes.begin(); it.valid(); ++it) {
		isInCluster[GC->copy(*it)] = true;
	}
	node v = GC->firstNode();
	node v_succ;
	while (v!=0) {
		v_succ = v->succ();
		if (!isInCluster[v]) {
			GC->delNode(v);
		}
		v = v_succ;
	}
	edge e = GC->firstEdge();
	edge e_succ;
	while (e!=0) {
		e_succ = e->succ();
		GC->delCopy(e);
		e = e_succ;
	}
	// Edges that have been added in child clusters by computing a spanning tree
	// have to be added to the GraphCopy.
	ListConstIterator<nodePair> it2;
	node cv,cw;
	for (it2=treeEdges[c].begin(); it2.valid(); ++it2) {
		cv = GC->copy((*it2).v1);
		cw = GC->copy((*it2).v2);
		GC->newEdge(cv,cw);
	}

	// Compute relevant nodePairs, i.e. all nodePairs induced by cluster c
	// leaving out already added ones.
	List<edgeValue> clusterNodePairs;
	ListConstIterator<edgeValue> it3;
	for (it3=clusterEdges[c].begin(); it3.valid(); ++it3) {
		cv = GC->copy((*it3).src);
		cw = GC->copy((*it3).trg);
		//TODO in liste speichern ob kante vorhanden dann kein searchedge
		if (!GC->searchEdge(cv,cw)) clusterNodePairs.pushBack(*it3);
	}

	childClusterSpanningTree(*GC,clusterNodePairs,treeEdges[c]);
	delete GC;
	return;
}


double Sub::heuristicImprovePrimalBound(
	List<nodePair> &origEdges,
	List<nodePair> &conEdges,
	List<edge> &delEdges)
{

	origEdges.clear(); conEdges.clear(); delEdges.clear();

	double oEdgeObjValue = 0.0;
	double cEdgeObjValue = 0.0;
	int originalEdgeCounter = 0;

	// A copy of the Clustergraph has to be created.
	// To be able to have access to the original nodes after the heuristic has been performed,
	// we maintain the Arrays \a originalClusterTable and \a originalNodeTable.
	Graph G;
	ClusterArray<cluster> originalClusterTable(*(((Master*)master_)->getClusterGraph()));
	NodeArray<node> originalNodeTable(*(((Master*)master_)->getGraph()));
	ClusterGraph CC( *(((Master*)master_)->getClusterGraph()),G,originalClusterTable,originalNodeTable );

	//NodeArray \a reverseNodeTable is indexized by \a G and contains the corresponding original nodes
	NodeArray<node> reverseNodeTable(G);
	node v;
	forall_nodes(v,*(((Master*)master_)->getGraph())) {
		node w = originalNodeTable[v];
		reverseNodeTable[w] = v;
	}

	// First, nodePairs have to be sorted in increasing order of their LP-value.
	// Therefore a Binary Heap is built and read once to obtain a sorted list of the nodePairs.
	List<edgeValue> globalNodePairList;
	BinaryHeap2<double,edgeValue> BH_all(nVar());
	edgeValue ev;
	node ov,ow;
	double lpValue;
	cluster lca;
	for (int i=0; i<nVar(); ++i) {
		ov = ((EdgeVar*)variable(i))->sourceNode();
		ow = ((EdgeVar*)variable(i))->targetNode();
		ev.src = originalNodeTable[ov]; //the node copies in G
		ev.trg = originalNodeTable[ow];
		ev.e   = ((EdgeVar*)variable(i))->theEdge(); //the original edge
		lpValue = 1.0-xVal(i);
		ev.lpValue = xVal(i);
		if ( ((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::ORIGINAL ) ev.original = true;
		else ev.original = false;
		BH_all.insert(ev,lpValue);
	}

	// ClusterArray \a clusterEdges contains for each cluster all corresponding edgeValues
	// in increasing order of their LP-values.
	ClusterArray<List<edgeValue> > clusterEdges(CC);
	for (int i=0; i<nVar(); ++i) {
		ev = BH_all.extractMin();
		lca = CC.commonCluster(ev.src,ev.trg);
		clusterEdges[lca].pushBack(ev);
		globalNodePairList.pushBack(ev);
	}

	// For each cluster \a spanningTreeNodePairs contains the computed nodePairs, edgeValues respectivly.
	ClusterArray<List<nodePair> > spanningTreesNodePairs(CC);
	clusterSpanningTree(
		CC,
		originalClusterTable[(((Master*)master_)->getClusterGraph())->rootCluster()],
		spanningTreesNodePairs,
		clusterEdges);
	// \a spanningTreesNodePairs[CC->rootCluster] now contains the edges of the computed tree.

	// Create the induced ClusterGraph.
	edge e = G.firstEdge();
	edge e_succ;
	while (e!=0) {
		e_succ = e->succ();
		G.delEdge(e);
		e = e_succ;
	}
	ListConstIterator<nodePair> it2;
	for (it2=spanningTreesNodePairs[CC.rootCluster()].begin(); it2.valid(); ++it2) {
		G.newEdge((*it2).v1,(*it2).v2);
	}

	// Creating two lists \a cEdgeNodePairs and \a oEdgeNodePairs in increasing order of LP-values.
	int nOEdges = 0;
	List<edgeValue> oEdgeNodePairs;
	BinaryHeap2<double,edgeValue> BH_oEdges(nVar());
	node cv,cw;
	nodePair np;
	for (int i=0; i<nVar(); ++i) {

		cv = originalNodeTable[ ((EdgeVar*)variable(i))->sourceNode() ];
		cw = originalNodeTable[ ((EdgeVar*)variable(i))->targetNode() ];
		//todo searchedge?
		if (!G.searchEdge(cv,cw)) {
			ev.src = cv; ev.trg = cw;
			lpValue = 1.0-xVal(i);
			ev.lpValue = xVal(i);
			if ( ((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::ORIGINAL ) {
				ev.e = ((EdgeVar*)variable(i))->theEdge(); //the original edge
				BH_oEdges.insert(ev,lpValue);
				nOEdges++;
			}
		} else { // Edge is contained in G.
			np.v1 = ((EdgeVar*)variable(i))->sourceNode();
			np.v2 = ((EdgeVar*)variable(i))->targetNode();

			if ( ((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::ORIGINAL ) {
				originalEdgeCounter++;
				oEdgeObjValue += 1.0;
				// Since edges that have been added are not deleted in further steps,
				// list \a origEdges may be updated in this step.
				origEdges.pushBack(np);

			} else {
				// Since edges that have been added are not deleted in further steps,
				// list \a conEdges may be updated in this step.
				conEdges.pushBack(np);
			}
		}
	}

	for (int i=1; i<=nOEdges; ++i) {
		oEdgeNodePairs.pushBack(BH_oEdges.extractMin());
	}

	//INSERTING LEFTOVER NODEPAIRS IN INCREASING ORDER OF LP_VALUE AND CHECKING FOR C-PLANARITY

	List<edgeValue> oneOEdges;
	List<edgeValue> fracEdges;
	ListConstIterator<edgeValue> it4 = oEdgeNodePairs.begin();
	while(it4.valid()) {
		if ((*it4).lpValue >= (1.0-master_->eps())) {
			oneOEdges.pushBack(*it4);
		} else {
			fracEdges.pushBack(*it4);
		}
		it4++;
	}

	// Randomly permute the edges in \a oneOEdges.
	oneOEdges.permute();
	ListConstIterator<edgeValue> it3;
	edge addEdge;
	bool cPlanar;
	CconnectClusterPlanar cccp;
	for (it3=oneOEdges.begin(); it3.valid(); ++it3) {

		addEdge = G.newEdge((*it3).src,(*it3).trg);
		cPlanar = cccp.call(CC);
		if (!cPlanar) {
			G.delEdge(addEdge);
			// Since edges that have been added are not deleted in further steps,
			// list \a origEdges may be updated in this step.
			np.v1 = reverseNodeTable[(*it3).src];
			np.v2 = reverseNodeTable[(*it3).trg];
			//Hier aus oneOEdges
			delEdges.pushBack((*it3).e);
		} else {
			originalEdgeCounter++;
			oEdgeObjValue += 1.0;
			// Since edges that have been added are not deleted in further steps,
			// list \a origEdges may be updated in this step.
			np.v1 = reverseNodeTable[(*it3).src];
			np.v2 = reverseNodeTable[(*it3).trg];
			origEdges.pushBack(np);
		}
	}


	Array<List<edgeValue> > leftoverPermLists(0,((Master*)master_)->numberOfHeuristicPermutationLists());

	// Distributing edges in \a fracEdges among the permutation lists.
	int index;
	for (it3=fracEdges.begin(); it3.valid(); ++it3) {
		index = getArrayIndex((*it3).lpValue);
		leftoverPermLists[index].pushBack(*it3);
	}

	for (int i=0; i<leftoverPermLists.size(); ++i) {
		// Testing of fractional values is also performed randomized.
		leftoverPermLists[i].permute();

		node u,v;
		for (it3=leftoverPermLists[i].begin(); it3.valid(); ++it3) {
			u = (*it3).src;
			v = (*it3).trg;
			addEdge = G.newEdge(u,v);
			cPlanar = cccp.call(CC);
			if (!cPlanar) {
				G.delEdge(addEdge);
				// Since edges that have been added are not deleted in further steps,
				// list \a origEdges may be updated in this step.
				np.v1 = reverseNodeTable[(*it3).src];
				np.v2 = reverseNodeTable[(*it3).trg];
				//Hier aus fracedges
				delEdges.pushBack((*it3).e);
			} else {
				originalEdgeCounter++;
				oEdgeObjValue += 1.0;
				// Since edges that have been added are not deleted in further steps,
				// list \a origEdges may be updated in this step.
				np.v1 = reverseNodeTable[(*it3).src];
				np.v2 = reverseNodeTable[(*it3).trg];
				origEdges.pushBack(np);
			}
		}
	} // All leftover permlists have been checked

	// If the Graph created so far contains all original edges, the Graph is c-planar.
	// Todo: And we are finished...
	if (originalEdgeCounter == ((Master*)master_)->getGraph()->numberOfEdges()) {

#ifdef OGDF_DEBUG
	((Master*)master_)->m_solByHeuristic = true;
#endif

		((Master*)master_)->updateBestSubGraph(origEdges,conEdges,delEdges);

		master_->primalBound(oEdgeObjValue+0.79);
	}

	return (oEdgeObjValue+0.79);
}



//////////////////////////////////////////////
//				OLD HEURISTIC
//////////////////////////////////////////////

/*
void Sub::minimumSpanningTree(
	GraphCopy &GC,
	List<nodePair> &clusterEdges,
	List<nodePair> &MSTEdges)
{

	//list \a clusterEdges contains nodePairs of \a G in increasing order of LP-value
	//\a GC is a GraphCopy of G and contains previously added edges of the child clusters.

	nodePair np;
	node cv,cw;
	edge newEdge;
	ListConstIterator<nodePair> it;
	//function valid() returns true, even if the list clusterEdges is empty. WHY???
	//maybe because the loop doesn't "recognise" that elements are removed from list
	//\clusterEdges, but works on the original list the whole time
	for (it=clusterEdges.begin(); it.valid(); ++it) {
		if (clusterEdges.size() == 0) break;

		np = clusterEdges.front();
		cv = GC.copy(np.v1);
		cw = GC.copy(np.v2);
		newEdge = GC.newEdge(cv,cw);

		if (!isAcyclicUndirected(GC)) {
			GC.delEdge(newEdge);
		} else {
			MSTEdges.pushBack(np);
		}
		clusterEdges.popFront();

		//if the number of edges of \a GC is one less than the number of nodes,
		//the search can be stopped, because a tree has been computed and no further edges can be added
		if (GC.numberOfEdges() == GC.numberOfNodes()-1) break;
	}
}


void Sub::recursiveMinimumSpanningTree(
	ClusterGraph &C,
	cluster c,
	ClusterArray<List<nodePair> > &treeEdges,
	List<nodePair> &edgesByIncLPValue,
	List<node> &clusterNodes)
{

	//node forwarding
	//nodes corresponding to cluster \a c are added to given list \a clusterNodes
	//necessary, to have quick access to the relevant nodes for building up the GraphCopy for the cluster
	ListConstIterator<node> it;
	for (it=c->nBegin(); it.valid(); ++it) {
		clusterNodes.pushBack(*it);
	}

	GraphCopy *cG;
	if (c->cCount() == 0) { //cluster is a leaf, so MST can be computed

		//Create a cluster induced GraphCopy of \a G and delete all edges
		cG = new GraphCopy(C.getGraph());
		node v = cG->firstNode();
		node v_succ;
		while (v!=0) {
			v_succ = v->succ();
			if (C.clusterOf(cG->original(v)) != c) {
				cG->delNode(v);
			}
			v = v_succ;
		}
		edge e = cG->firstEdge();
		edge e_succ;
		while (e!=0) {
			e_succ = e->succ();
			cG->delEdge(e);
			e = e_succ;
		}

		//Determining the relevant nodePairs of the cluster-induced GraphCopy
		//in increasing order of LP-value
		//performance should be improved, maybe by using a more sophisticated data structure
		List<nodePair> clusterNodePairs;
		ListConstIterator<nodePair> it;
		for (it=edgesByIncLPValue.begin(); it.valid(); ++it) {
			if (C.clusterOf((*it).v1) == c && C.clusterOf((*it).v2) == c) {
				clusterNodePairs.pushBack(*it);
			}
		}

		minimumSpanningTree(*cG,clusterNodePairs,treeEdges[c]);
		delete cG;
		return;
	}

	//If cluster \a c has further children, they have to be processed first
	ListConstIterator<cluster> cit;
	List<node> nodes;
	for (cit=c->cBegin(); cit.valid(); ++cit) {

		recursiveMinimumSpanningTree(C,(*cit),treeEdges,edgesByIncLPValue,nodes);

		//computed treeEdges for the child clusters have to be added to treeEdges for current cluster
		ListConstIterator<nodePair> it;
		for (it=treeEdges[(*cit)].begin(); it.valid(); ++it) {
			treeEdges[c].pushBack(*it);
		}
	}
	//The MSTs of all children of cluster \a c have been computed
	//So MST for \a c can now be computed

	//updating node lists
	for (it=nodes.begin(); it.valid(); ++it) {
		clusterNodes.pushBack(*it);
	}
	for (it=c->nBegin(); it.valid(); ++it) {
		nodes.pushBack(*it);
	}
	//now list \a nodes contains all nodes belonging to cluster \a c

	//create GraphCopy induced by nodes in \a nodes
	cG = new GraphCopy(C.getGraph());
	NodeArray<bool> isInCluster(*cG,false);
	for (it=nodes.begin(); it.valid(); ++it) {
		isInCluster[cG->copy(*it)] = true;
	}
	node v = cG->firstNode();
	node v_succ;
	while (v!=0) {
		v_succ = v->succ();
		if (!isInCluster[v]) {
			cG->delNode(v);
		}
		v = v_succ;
	}
	edge e = cG->firstEdge();
	edge e_succ;
	while (e!=0) {
		e_succ = e->succ();
		cG->delEdge(e);
		e = e_succ;
	}
	//edges that have been added in child clusters by computing an MST have to be added to the Graphcopy
	ListConstIterator<nodePair> it2;
	node cGv,cGw;
	for (it2=treeEdges[c].begin(); it2.valid(); ++it2) {
		cGv = cG->copy((*it2).v1);
		cGw = cG->copy((*it2).v2);
		cG->newEdge(cGv,cGw);
	}

	//compute relevant nodePairs, i.e. all nodePairs induced by cluster leaving out already added ones
	List<nodePair> clusterNodePairs;
	ListConstIterator<nodePair> it3;
	for (it3=edgesByIncLPValue.begin(); it3.valid(); ++it3) {
		cGv = cG->copy((*it3).v1);
		cGw = cG->copy((*it3).v2);
		//todo remove searchedge
		if (isInCluster[cGv] && isInCluster[cGw] && !cG->searchEdge(cGv,cGw)) clusterNodePairs.pushBack(*it3);
	}

	minimumSpanningTree(*cG,clusterNodePairs,treeEdges[c]);
	delete cG;
	return;
}


double Sub::heuristicImprovePrimalBoundDet(
	List<nodePair> &origEdges,
	List<nodePair> &conEdges,
	List<nodePair> &delEdges)
{

	origEdges.clear(); conEdges.clear(); delEdges.clear();

	//the primal value of the heuristically computed ILP-solution
	double oEdgeObjValue = 0.0;
	double cEdgeObjValue = 0.0;
	int originalEdgeCounter = 0;

	//a copy of the Clustergraph has to be created.
	//to be able to have access to the original nodes after the heuristic has been performed,
	//we maintain the Arrays \a originalClusterTable and \a originalNodeTable
	Graph G;
	ClusterArray<cluster> originalClusterTable(*(((Master*)master_)->getClusterGraph()));
	NodeArray<node> originalNodeTable(*(((Master*)master_)->getGraph()));
	ClusterGraph CC( *(((Master*)master_)->getClusterGraph()),G,originalClusterTable,originalNodeTable );

	//NodeArray \a reverseNodeTable is indexized by \a G and contains the corresponding original nodes
	NodeArray<node> reverseNodeTable(G);
	node v;
	forall_nodes(v,*(((Master*)master_)->getGraph())) {
		node w = originalNodeTable[v];
		reverseNodeTable[w] = v;
	}

	//first, nodePairs have to be sorted in increasing order of their LP-value
	//therefore a Binary Heap is build and read once, to obtain a sorted list of the nodePairs
	List<nodePair> globalNodePairList;
	BinaryHeap2<double,nodePair> BH_all(nVar());
	nodePair np;
	node ov,ow;
	double lpValue;
	for (int i=0; i<nVar(); ++i) {
		ov = ((Edge*)variable(i))->sourceNode();
		ow = ((Edge*)variable(i))->targetNode();
		np.v1 = originalNodeTable[ov];
		np.v2 = originalNodeTable[ow];
		lpValue = 1.0-xVal(i);
		BH_all.insert(np,lpValue);
	}
	for (int i=0; i<nVar(); ++i) {
		np = BH_all.extractMin();
		ov = reverseNodeTable[np.v1]; ow = reverseNodeTable[np.v2];
		globalNodePairList.pushBack(np);
	}

	//for each cluster \a spanningTreeNodePairs contains the MST nodePairs (of original Graph)
	ClusterArray<List<nodePair> > spanningTreesNodePairs(CC);
	List<node> nodes;
	recursiveMinimumSpanningTree(
		CC,
		originalClusterTable[(((Master*)master_)->getClusterGraph())->rootCluster()],
		spanningTreesNodePairs,
		globalNodePairList,
		nodes);
	//\a spanningTreesNodePairs[CC->rootCluster] now contains the edges of the computed Tree

	//create the induced ClusterGraph
	edge e = G.firstEdge();
	edge e_succ;
	while (e!=0) {
		e_succ = e->succ();
		G.delEdge(e);
		e = e_succ;
	}
	ListConstIterator<nodePair> it2;
	for (it2=spanningTreesNodePairs[CC.rootCluster()].begin(); it2.valid(); ++it2) {
		G.newEdge((*it2).v1,(*it2).v2);
	}

	//creating two lists \a cEdgeNodePairs and \a oEdgeNodePairs in increasing order of LP-values.
	int nOEdges = 0;
	int nCEdges = 0;
	List<nodePair> cEdgeNodePairs;
	List<nodePair> oEdgeNodePairs;
	BinaryHeap2<double,nodePair> BH_cEdges(nVar());
	BinaryHeap2<double,nodePair> BH_oEdges(nVar());
	node cv,cw;
	for (int i=0; i<nVar(); ++i) {

		cv = originalNodeTable[ ((Edge*)variable(i))->sourceNode() ];
		cw = originalNodeTable[ ((Edge*)variable(i))->targetNode() ];
		if ((G.searchEdge(cv,cw))==NULL) {
			np.v1 = cv; np.v2 = cw;
			lpValue = 1.0-xVal(i);
			if ( ((Edge*)variable(i))->theEdgeType() == ORIGINAL ) {
				BH_oEdges.insert(np,lpValue);
				nOEdges++;
			} else {
				BH_cEdges.insert(np,lpValue);
				nCEdges++;
			}
		} else { //edge is contained in G
			np.v1 = ((Edge*)variable(i))->sourceNode();
			np.v2 = ((Edge*)variable(i))->targetNode();

			if ( ((Edge*)variable(i))->theEdgeType() == ORIGINAL ) {
				originalEdgeCounter++;
				oEdgeObjValue += 1.0;
				//since edges that have been added are not deleted in further steps,
				//list \a origEdges may be updated in this step.
				origEdges.pushBack(np);

			} else {
				//cEdgeObjValue -= ((Master*)master_)->epsilon();
				//since edges that have been added are not deleted in further steps,
				//list \a conEdges may be updated in this step.
				conEdges.pushBack(np);
			}
		}
	}

	for (int i=1; i<=nCEdges; ++i) {
		cEdgeNodePairs.pushBack(BH_cEdges.extractMin());
	}
	for (int i=1; i<=nOEdges; ++i) {
		oEdgeNodePairs.pushBack(BH_oEdges.extractMin());
	}


	//INSERTING LEFTOVER NODEPAIRS IN INCREASING ORDER OF LP_VALUE AND CHECKING FOR C-PLANARITY
	//first, O-Edges are testet and added.

	ListConstIterator<nodePair> it3;
	edge addEdge;
	bool cPlanar;
	CconnectClusterPlanar cccp;
	for (it3=oEdgeNodePairs.begin(); it3.valid(); ++it3) {

		addEdge = G.newEdge((*it3).v1,(*it3).v2);
		cPlanar = cccp.call(CC);
		if (!cPlanar) {
			G.delEdge(addEdge);
			//since edges that have been added are not deleted in further steps,
			//list \a origEdges may be updated in this step.
			np.v1 = reverseNodeTable[(*it3).v1];
			np.v2 = reverseNodeTable[(*it3).v2];
			delEdges.pushBack(np);
		} else {
			originalEdgeCounter++;
			oEdgeObjValue += 1.0;
			//since edges that have been added are not deleted in further steps,
			//list \a origEdges may be updated in this step.
			np.v1 = reverseNodeTable[(*it3).v1];
			np.v2 = reverseNodeTable[(*it3).v2];
			origEdges.pushBack(np);
		}
	}

	//if the Graph created so far contains all original edges, the Graph is c-planar.
	if (originalEdgeCounter == ((Master*)master_)->getGraph()->numberOfEdges()) {
		//cout << "Graph is c-planar! Heuristic has computed a solution that contains all original edges" << endl;
		((Master*)master_)->updateBestSubGraph(origEdges,conEdges,delEdges);
		//cout << "value of solution is: " << oEdgeObjValue << endl;
		master_->primalBound(oEdgeObjValue+0.79);
	}


	cout << "the objective function value of heuristically computed ILP-solution is: " << (oEdgeObjValue + cEdgeObjValue) << endl;

	return (oEdgeObjValue+0.79);
}
*/


int Sub::improve(double &primalValue) {

	if ( ((Master*)master_)->getHeuristicLevel() == 0 ) return 0;

	// If \a heuristicLevel is set to value 1, the heuristic is only run,
	// if current solution is fractional and no further constraints have been found.
	if ( ((Master*)master_)->getHeuristicLevel() == 1 ) {
		if (!integerFeasible() && !m_constraintsFound) {

			List<nodePair> origEdges;
			List<nodePair> conEdges;
			List<edge> delEdges;

			for (int i=((Master*)master_)->getHeuristicRuns(); i>0; i--) {

				origEdges.clear(); conEdges.clear(); delEdges.clear();
				double heuristic = heuristicImprovePrimalBound(origEdges,conEdges,delEdges);

				// \a heuristic contains now the objective function value (primal value)
				// of the heuristically computed ILP-solution.
				// We have to check if this solution is better than the currently best primal solution.
				if(master_->betterPrimal(heuristic)) {
#ifdef OGDF_DEBUG
	((Master*)master_)->m_solByHeuristic = true;
#endif
					// Best primal solution has to be updated.
					((Master*)master_)->updateBestSubGraph(origEdges,conEdges,delEdges);
					primalValue = heuristic;
					return 1;
				}
			}
			return 0;
		}

	// If \a heuristicLevel is set to value 2, the heuristic is run after each
	// LP-optimization step, i.e. after each iteration.
	} else if ( ((Master*)master_)->getHeuristicLevel() == 2 ) {
		List<nodePair> origEdges;
		List<nodePair> conEdges;
		List<edge> delEdges;

		double heuristic = heuristicImprovePrimalBound(origEdges,conEdges,delEdges);

		// \a heuristic contains now the objective function value (primal value)
		// of the heuristically computed ILP-solution.
		// We have to check if this solution is better than the currently best primal solution.
		if(master_->betterPrimal(heuristic)) {
#ifdef OGDF_DEBUG
	((Master*)master_)->m_solByHeuristic = true;
#endif
			// Best primal solution has to be updated
			((Master*)master_)->updateBestSubGraph(origEdges,conEdges,delEdges);
			primalValue = heuristic;
			return 1;
		}
		return 0;
	}

	// For any other value of \a m_heuristicLevel the function returns 0.
	return 0;
}

//! Computes the number of bags within the given cluster \a c
//! (non recursive)
//! A bag is a set of chunks within the cluster that are connected
//! via subclusters
int Sub::clusterBags(ClusterGraph &CG, cluster c)
{
    const Graph& G = CG.getGraph();
    if (G.numberOfNodes() == 0) return 0;
    int numChunks = 0; //number of chunks (CCs) within cluster c
    int numBags;       //number of bags (Constructs consisting of CCs connected by subclusters)

    //stores the nodes belonging to c
    List<node> nodesInCluster;
    //stores the corresponding interator to the list element for each node
    NodeArray<ListIterator<node> > listPointer(G);

    NodeArray<bool> isVisited(G, false);
    NodeArray<bool> inCluster(G, false);
    NodeArray<node> parent(G); //parent for path to representative in bag gathering

    //saves status of all nodes in hierarchy subtree at c
    c->getClusterNodes(nodesInCluster);
    int num = nodesInCluster.size();
    if (num == 0) return 0;

//    cout << "#Starting clusterBags with cluster of size " << num << "\n";

    //now we store the  iterators
    ListIterator<node> it = nodesInCluster.begin();
    while (it.valid())
    {
        listPointer[(*it)] = it;
        inCluster[(*it)] = true;
        it++;
    }//while

    int count = 0;

    //now we make a traversal through the induced subgraph,
    //jumping between the chunks
    while (count < num)
    {
        numChunks++;
        node start = nodesInCluster.popFrontRet();

        //do a BFS and del all visited nodes in nodesInCluster using listPointer
        Queue<node> activeNodes; //could use arraybuffer
        activeNodes.append(start);
        isVisited[start] = true;
        edge e;
        while (!activeNodes.empty())
        {
            node v = activeNodes.pop(); //running node
            parent[v] = start; //representative points to itself
//            cout << "Setting parent of " << v->index() << "  to " << start->index() << "\n";
            count++;

            forall_adj_edges(e, v)
            {
                node w = e->opposite(v);

                if (v == w) continue; // ignore self-loops

                if (inCluster[w] && !isVisited[w])
                {
                    //use for further traversal
                    activeNodes.append(w);
                    isVisited[w] = true;
                    //remove the node from the candidate list
                    nodesInCluster.del(listPointer[w]);
                }
            }
        }//while

    }//while

//    cout << "Number of chunks: " << numChunks << "\n";
    //Now all node parents point to the representative of their chunk (start node in search)
    numBags = numChunks; //We count backwards if chunks are connected by subcluster

    //Now we use an idea similar to UNION FIND to gather the bags
    //out of the chunks. Each node has a parent pointer, leading to
    //a representative. Initially, it points to the rep of the chunk,
    //but each time we encounter a subcluster connecting multiple
    //chunks, we let all of them point to the same representative.
    ListConstIterator<cluster> itC = c->cBegin();
    while (itC.valid())
    {
        List<node> nodesInChild;
        (*itC)->getClusterNodes(nodesInChild);
        cout << nodesInChild.size() << "\n";
        ListConstIterator<node> itN = nodesInChild.begin();
        node bagRep; //stores the representative for the whole bag
        if (itN.valid()) bagRep = getRepresentative(*itN, parent);
//        cout << " bagRep is " << bagRep->index() << "\n";
        while (itN.valid())
        {
            node w = getRepresentative(*itN, parent);
//            cout << "  Rep is: " << w->index() << "\n";
            if (w != bagRep)
            {
                numBags--; //found nodes with different representative, merge
                parent[w] = bagRep;
                parent[*itN] = bagRep; //shorten search path
//                cout << "  Found new node with different rep, setting numBags to " << numBags << "\n";
            }
            itN++;
        }//While all nodes in subcluster

        itC++;
    }//while all child clusters

    return numBags;
//    cout << "#Number of bags: " << numBags << "\n";
}//clusterBags


//! returns connectivity status for complete connectivity
//! returns 1 in this case, 0 otherwise
// New version using arrays to check cluster affiliation during graph traversal,
// old version used graph copies

// For complete connectivity also the whole graph needs to
// be connected (root cluster). It therefore does not speed up
// the check to test connectivity of the graph in advance.
// Note that then a cluster induced graph always has to be
// connected to the complement, besides one of the two is empty.

// Uses an array that keeps the information on the cluster
// affiliation and bfs to traverse the graph.
//we rely on the fact that support is a graphcopy of the underlying graph
//with some edges added or removed
bool Sub::checkCConnectivity(const GraphCopy& support)
{

	const ClusterGraph &CG = *((Master*)master_)->getClusterGraph();
	const Graph& G = CG.getGraph();
	//if there are no nodes, there is nothing to check
	if (G.numberOfNodes() < 2) return true;

	cluster c;

	//there is always at least the root cluster
	forall_clusters(c, CG)
	{
		// For each cluster, the induced graph partitions the graph into two sets.
		// When the cluster is empty, we still check the complement and vice versa.
	    bool set1Connected = false;

	    //this initialization can be done faster by using the
	    //knowledge of the cluster hierarchy and only
	    //constructing the NA once for the graph (bottom up tree traversal)
	    NodeArray<bool> inCluster(G, false);
	    NodeArray<bool> isVisited(G, false);

	    //saves status of all nodes in hierarchy subtree at c
	    int num = c->getClusterNodes(inCluster);

	    int count = 0;
	    //search in graph should reach num and V-num nodes
	    node complementStart = 0;

	    //we start with a non-empty set
	    node start = G.firstNode();
	    bool startState = inCluster[start];

	    Queue<node> activeNodes; //could use arraybuffer
	    activeNodes.append(start);
	    isVisited[start] = true;

		//could do a shortcut here for case |c| = 1, but
		//this would make the code more complicated without large benefit
	    edge e;
	    node u;
	    while (!activeNodes.empty())
	    {
	        node v = activeNodes.pop(); //running node
	        count++;
	        u = support.copy(v);

	        forall_adj_edges(e, u)
	        {
	            node w = support.original(e->opposite(support.copy(v)));

	            if (v == w) continue; // ignore self-loops

	            if (inCluster[w] != startState) complementStart = w;
	            else if (!isVisited[w])
	            {
	                activeNodes.append(w);
	                isVisited[w] = true;
	            }
	        }
	    }//while
	    //check if we reached all nodes
	    //we assume that the graph is connected, otherwise check
	    //fails for root cluster anyway
	    //(we could have a connected cluster and a connected complement)

	    //condition depends on the checked set, cluster or complement
	    set1Connected = (startState == true ? (count == num) : (count == G.numberOfNodes() - num));
	    //cout << "Set 1 connected: " << set1Connected << " Cluster? " << startState << "\n";

	    if (!set1Connected) return false;
	    //check if the complement of set1 is also connected
	    //two cases: complement empty: ok
	    //           complement not empty,
	    //           but no complementStart found: error
	    //In case of the root cluster, this always triggers,
	    //therefore we have to continue
	    if (G.numberOfNodes() == count)
	        continue;
	    OGDF_ASSERT(complementStart != 0);

	    activeNodes.append(complementStart);
	    isVisited[complementStart] = true;
	    startState = ! startState;
	    int ccount = 0;
	    while (!activeNodes.empty())
	    {
	        node v = activeNodes.pop(); //running node
	        ccount++;
	        u = support.copy(v);

	        forall_adj_edges(e, u)
	        {
	            node w = support.original(e->opposite(support.copy(v)));

	            if (v == w) continue; // ignore self-loops

	            if (!isVisited[w])
	            {
	                activeNodes.append(w);
	                isVisited[w] = true;
	            }
	        }
	    }//while
	    //Check if we reached all nodes
	    if (!(ccount + count == G.numberOfNodes()))
	        return false;
	}//forallclusters
	return true;
}

//only left over for experimental evaluation of speedups
bool Sub::checkCConnectivityOld(const GraphCopy& support)
{
	//Todo: It seems to me that this is not always necessary:
	//For two clusters, we could stop even if support is not connected
	if (isConnected(support)) {

		GraphCopy *cSupport;
		cluster c = ((Master*)master_)->getClusterGraph()->firstCluster();

		while (c != NULL) {
			// Determining the nodes of current cluster
			List<node> clusterNodes;
			c->getClusterNodes(clusterNodes);

			// Step1: checking the restgraph for connectivity
			GraphCopy cSupportRest((const Graph&)support);
			ListIterator<node> it;
			node cv1, cv2;

			for (it=clusterNodes.begin(); it.valid(); ++it) {

				cv1 = support.copy(*it);
				cv2 = cSupportRest.copy(cv1);
				cSupportRest.delNode(cv2);
			}

			// Checking \a cSupportRest for connectivity
			if (!isConnected(cSupportRest)) {
				return false;
			}

			// Step2: checking the cluster induced subgraph for connectivity
			cSupport = new GraphCopy((const Graph&)support);
			NodeArray<bool> inCluster(*((Master*)master_)->getGraph());
			inCluster.fill(false);

			for (it=clusterNodes.begin(); it.valid(); ++it) {
				inCluster[*it] = true;
			}
			node v = ((Master*)master_)->getGraph()->firstNode();
			node succ;
			while (v!=0) {
				succ = v->succ();
				if (!inCluster[v]) {
					cv1 = support.copy(v);
					cv2 = cSupport->copy(cv1);
					cSupport->delNode(cv2);
				}
				v = succ;
			}
			if (!isConnected(*cSupport)) {
				return false;
			}
			delete cSupport;

			// Next cluster
			c = c->succ();
		}

	} else {
		return false;
	}
	return true;

}

bool Sub::feasible() {
//	cout << "Checking feasibility\n";

	if (!integerFeasible()) {
		return false;
	}
	else {

		//----------------------------------------------------------------
		// Checking if the solution induced graph is completely connected.
		GraphCopy support(*((Master*)master_)->getGraph());
		intSolutionInducedGraph(support);

		//introduced merely for debug checks
		bool cc = checkCConnectivity(support);
		bool ccOld = checkCConnectivityOld(support);
		if (cc != ccOld)
		{
			cout << "CC: "<<cc<<" CCOLD: "<<ccOld<<"\n";

		}
		OGDF_ASSERT (cc == ccOld);
		if (!cc) return false;

		//------------------------
		// Checking if the solution induced graph is planar.

		if (BoyerMyrvold().isPlanarDestructive(support)) {

			// Current solution is integer feasible, completely connected and planar.
			// Checking, if the objective function value of this subproblem is > than
			// the current optimal primal solution.
			// If so, the solution induced graph is updated.
			double primalBoundValue = (double)(floor(lp_->value()) + 0.79);
			if (master_->betterPrimal(primalBoundValue)) {
				master_->primalBound(primalBoundValue);
				updateSolution();
			}
			return true;

		} else {
			return false;
		}
	}
}//feasible

static void dfsIsConnected(node v, NodeArray<bool> &visited, int &count)
{
	count++;
	visited[v] = true;

	edge e;
	forall_adj_edges(e,v) {
		node w = e->opposite(v);
		if (!visited[w]) dfsIsConnected(w,visited,count);
	}
}

/*
bool Sub::fastfeasible() {

	if (!integerFeasible()) {
		return false;
	}
	else {

		// Checking if the solution induced Graph is completely connected.
		GraphCopy support(*((Master*)master_)->getGraph());
		intSolutionInducedGraph(support);

		//Todo: It seems to me that this is not always necessary:
		//For two clusters, we could stop even if support is not connected
		//we also do not need the root cluster
		if (isConnected(support)) {

			GraphCopy *cSupport;
			cluster c = ((Master*)master_)->getClusterGraph()->firstCluster();

			while (c != NULL)
			{
				if (c == ((Master*)master_)->getClusterGraph().rootCluster())
				{
					//attention: does the rest of the code rely on the fact
					//that the root is connected
					// Next cluster
					c = c->succ();
					continue;
				}//if root cluster
				// Determining the nodes of current cluster
				List<node> clusterNodes;
				c->getClusterNodes(clusterNodes);

				int count = 0;
				NodeArray<bool> blocked(support, false);
				// Step1: checking the restgraph for connectivity
				ListIterator<node> it;
				node cv1, cv2;

				for (it=clusterNodes.begin(); it.valid(); ++it)
				{
					blocked[*it] = true;

				}

				// Checking \a cSupportRest for connectivity

				if (clusterNodes.size() < support.numberOfNodes())
				{
					//search for a node outside c
					//should be done more efficiently, rewrite this
					node runv = support->firstNode();
					while (blocked[runv] == true) {runv = runv->succ();}

					dfsIsConnected(runv,blocked,count);
					if (count != support.numberOfNodes()-clusterNodes.size())
						return false;
				}


				// Step2: checking the cluster induced subgraph for connectivity
				//NodeArray<bool> inCluster(*((Master*)master_)->getGraph());
				//inCluster.fill(false);
				blocked.init(support, true);

				for (it=clusterNodes.begin(); it.valid(); ++it) {
					blocked[*it] = false;
				}
				node v = ((Master*)master_)->getGraph()->firstNode();
				node succ;
				while (v!=0) {
					succ = v->succ();
					if (!inCluster[v]) {
						cv1 = support.copy(v);
						cv2 = cSupport->copy(cv1);
						cSupport->delNode(cv2);
					}
					v = succ;
				}
				if (!isConnected(*cSupport)) {
					return false;
				}
				delete cSupport;

				// Next cluster
				c = c->succ();
			}

		} else {
			return false;
		}

		// Checking for planarity

		BoyerMyrvold bm;
		bool planar = bm.planarDestructive(support);
		if (planar) {

			// Current solution is integer feasible, completely connected and planar.
			// Checking, if the objective function value of this subproblem is > than
			// the current optimal primal solution.
			// If so, the solution induced graph is updated.
			double primalBoundValue = (double)(floor(lp_->value()) + 0.79);
			if (master_->betterPrimal(primalBoundValue)) {
				master_->primalBound(primalBoundValue);
				updateSolution();
			}
			return true;

		} else {
			return false;
		}
	}
}//fastfeasible

*/
void Sub::intSolutionInducedGraph(GraphCopy &support) {

	edge e, ce;
	node v, w, cv, cw;
	for (int i=0; i<nVar(); ++i) {
		if ( xVal(i) >= 1.0-(master_->eps()) ) {

			if (((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::CONNECT) {

				// If Connection-variables have value == 1.0 they have to be ADDED to the support graph.
				v = ((EdgeVar*)variable(i))->sourceNode();
				w = ((EdgeVar*)variable(i))->targetNode();
				cv = support.copy(v);
				cw = support.copy(w);
				support.newEdge(cv,cw);
			}
		} else {

			// If Original-variables have value == 0.0 they have to be DELETED from the support graph.
			if (((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::ORIGINAL) {
				e = ((EdgeVar*)variable(i))->theEdge();
				ce = support.copy(e);
				support.delEdge(ce);
			}
		}
	}
}


void Sub::kuratowskiSupportGraph(GraphCopy &support, double low, double high) {

	edge e, ce;
	node v, w, cv, cw;
	for (int i=0; i<nVar(); ++i) {

		if (xVal(i) >= high) {

			// If variables have value >= \a high and are of type CONNECT
			// they are ADDED to the support graph.
			if (((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::CONNECT) {

				v = ((EdgeVar*)variable(i))->sourceNode();
				w = ((EdgeVar*)variable(i))->targetNode();
				cv = support.copy(v);
				cw = support.copy(w);
				support.newEdge(cv,cw);
			} else continue;
		} else if (xVal(i) <= low) {

			// If variables have value <= \a low and are of type ORIGINAL
			// they are DELETED from the support graph.
			if (((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::ORIGINAL) {

				e = ((EdgeVar*)variable(i))->theEdge();
				ce = support.copy(e);
				//delCopy!
				support.delEdge(ce);
			} else continue;
		}

		else {	// Value of current variable lies between \a low and \a high.

			// Variable is added/deleted randomized according to its current value.

			// Variable of type ORIGINAL is deleted with probability 1-xVal(i).
			if (((EdgeVar*)variable(i))->theEdgeType() == EdgeVar::ORIGINAL) {

				double ranVal = randomDouble(0.0,1.0);
				if (ranVal > xVal(i)) {
					e = ((EdgeVar*)variable(i))->theEdge();
					ce = support.copy(e);
					support.delEdge(ce);
				}

			} else {
				// Variable of type CONNECT is added with probability of xVal(i).

				double ranVal = randomDouble(0.0,1.0);
				if (ranVal < xVal(i)) {
					v = ((EdgeVar*)variable(i))->sourceNode();
					w = ((EdgeVar*)variable(i))->targetNode();
					cv = support.copy(v);
					cw = support.copy(w);
					// searchEdge ist hier wohl �berfl�ssig... (assertion)
					if (!support.searchEdge(cv,cw)) support.newEdge(cv,cw);
				}
			}
		}

	} // end for(int i=0; i<nVar(); ++i)
}


void Sub::connectivitySupportGraph(GraphCopy &support, EdgeArray<double> &weight) {

	// Step 1+2: Create the support graph & Determine edge weights and fill the EdgeArray \a weight.
	// MCh: warning: modified by unifying both steps. performance was otherwise weak.
	edge e, ce;
	node v, w, cv, cw;
	//initializes weight array to original graph (values undefined)
	weight.init(support);
	for (int i=0; i<nVar(); ++i) {
		EdgeVar* var = ((EdgeVar*)variable(i));
		double val = xVal(i);
		//weight array entry is set for all nonzero values
		if (val > master()->eps()) {
			// Connection edges have to be added.
			if (var->theEdgeType() == EdgeVar::CONNECT) {
				v = var->sourceNode();
				w = var->targetNode();
				cv = support.copy(v);
				cw = support.copy(w);
				weight[ support.newEdge(cv,cw) ] = val;
			} else
				weight[ support.chain(var->theEdge()).front() ] = val;
		} else {
		// Original edges have to be deleted if their current value equals 0.0.
		//if (val <= master()->eps()) {
			if (var->theEdgeType() == EdgeVar::ORIGINAL) {
				ce = support.copy( var->theEdge() );
				support.delCopy(ce); //MCh: was: delEdge
			}
		}
	}
	//TODO: KK: Removed this (think it is safe), test!
	// Step 2:
	/*
	for (int i=0; i<nVar(); ++i)
	{
		v = ((EdgeVar*)variable(i))->sourceNode();
		w = ((EdgeVar*)variable(i))->targetNode();
		//TODO: Inefficient! Speed search up
		e = support.searchEdge(support.copy(v),support.copy(w));
		if (e) weight[e] = xVal(i);
	}
	*/
}

//----------------------------Computation of Cutting Planes-------------------------//
//															       			        //
//Implementation and usage of separation algorithmns						        //
//for the Kuratowski- and the Connectivity- constraints						        //
//													  						        //
//----------------------------------------------------------------------------------//

int Sub::separateReal(double minViolate) {

	// The number of generated and added constraints.
	// Each time a constraint is created and added to the buffer, the variable \a count is incremented.
	// When adding the created constraints \a nGenerated and \a count are checked for equality.
	int nGenerated = 0;
	int count = 0;
	m_constraintsFound = false;

	if(master()->m_useDefaultCutPool)
		nGenerated = constraintPoolSeparation(0,0,minViolate);
	if(nGenerated>0) return nGenerated;

	//-------------------------------CUT-SEPARATION--------------------------------------//

	// We first try to regenerate cuts from our cutpools
	nGenerated = separateConnPool(minViolate);
	if (nGenerated > 0)
	{
#ifdef OGDF_DEBUG
	Logger::slout()<<"con-regeneration.";
#endif
		return nGenerated;
		//TODO: Check if/how we can proceed here, i.e. should we have this else?
	}
	else
	{
#ifdef OGDF_DEBUG
//	cout<<"Connectivity Regeneration did not work\n";
#endif
		GraphCopy support (*((Master*)master())->getGraph());
		EdgeArray<double> w;
		connectivitySupportGraph(support,w);

		// Buffer for the constraints
		int nClusters = (((Master*)master_)->getClusterGraph())->numberOfClusters();
		//ABA_BUFFER<ABA_CONSTRAINT *> cConstraints(master_,2*nClusters);

		GraphCopy *c_support;
		EdgeArray<double> c_w;
		cluster c;

		// INTER-CLUSTER CONNECTIVITY

		forall_clusters(c,*((Master*)master_)->getClusterGraph()) {

			c_support = new GraphCopy((const Graph&)support);
			c_w.init(*c_support);

			// Copying edge weights to \a c_w.
			List<double> weights;
			edge e,c_e;
			forall_edges(e,support) {
				weights.pushBack(w[e]);
			}
			ListConstIterator<double> wIt = weights.begin();
			forall_edges(c_e,*c_support) {
				if (wIt.valid()) c_w[c_e] = (*wIt);
				wIt++;
			}

			// Residue graph is determined and stored in \a c_support.
			List<node> clusterNodes;
			c->getClusterNodes(clusterNodes);
			ListIterator<node> it;
			node cCopy1, cCopy2;
			for (it=clusterNodes.begin(); it.valid(); ++it) {
				cCopy1 = support.copy(*it);
				cCopy2 = c_support->copy(cCopy1);
				c_support->delNode(cCopy2);
			}

			// Checking if Graph is connected.
			if (isConnected(*c_support)) {

				MinCut mc(*c_support,c_w);
				List<edge> cutEdges;
				double mincutV = mc.minimumCut();
				if (mincutV < 1.0-master()->eps()-minViolate) {

					mc.cutEdges(cutEdges,*c_support);

					List<nodePair> cutNodePairs;
					ListConstIterator<edge> cutEdgesIt;
					node v,w,cv,cw,ccv,ccw;
					nodePair np;
					for (cutEdgesIt=cutEdges.begin();cutEdgesIt.valid();cutEdgesIt++) {
						v = (*cutEdgesIt)->source();
						w = (*cutEdgesIt)->target();
						cv = c_support->original(v);
						cw = c_support->original(w);
						ccv = support.original(cv);
						ccw = support.original(cw);
						np.v1 = ccv;
						np.v2 = ccw;
						cutNodePairs.pushBack(np);
					}

					// Create constraint
					bufferedForCreation.push(new CutConstraint((Master*)master(),this, cutNodePairs));
					count++;
				}

			}//end Graph is connected

			else {
				NodeArray<int> comp(*c_support);
				connectedComponents(*c_support,comp);
				List<node> partition;
				NodeArray<bool> isInPartition(*c_support);
				isInPartition.fill(false);
				node v;
				forall_nodes(v,*c_support) {
					if (comp[v] == 0) {
						partition.pushBack(v);
						isInPartition[v] = true;
					}
				}

				// Computing nodePairs defining the cut.
				List<nodePair> cutEdges;
				ListConstIterator<node> it;
				nodePair np;
				for (it=partition.begin(); it.valid(); ++it) {
					node w,cv,cw;
					forall_nodes(w,*c_support) {
						if ( (w!=(*it)) && !isInPartition[w] ) {
							cw = c_support->original(w);
							cv = c_support->original(*it);
							np.v1 = support.original(cw);
							np.v2 = support.original(cv);
							cutEdges.pushBack(np);
						}
					}
				}

				// Create cut-constraint
				bufferedForCreation.push(new CutConstraint((Master*)master(), this, cutEdges)); // always violated enough
				count++;

			}//end Graph is not connected
			delete c_support;

		}//end forall_clusters

		// INTRA-CLUSTER CONNECTIVITY

		// The initial constraints can not guarantee the connectivity of a cluster.
		// Thus, for each cluster we have to check, if the induced Graph is connected.
		// If so, we compute the mincut and create a corresponding constraint.
		// Otherwise a constraint is created in the same way as above.

		forall_clusters(c,*((Master*)master_)->getClusterGraph()) {

			c_support = new GraphCopy((const Graph&)support);
			c_w.init(*c_support);

			List<double> weights;
			edge e,c_e;
			forall_edges(e,support) {
				weights.pushBack(w[e]);
			}
			ListConstIterator<double> wIt = weights.begin();
			forall_edges(c_e,*c_support) {
				if (wIt.valid()) c_w[c_e] = (*wIt);
				wIt++;
			}

			// Cluster induced Graph is determined and stored in \a c_support.
			ListIterator<node> it;
			List<node> clusterNodes;
			c->getClusterNodes(clusterNodes);
			NodeArray<bool> isInCluster(*c_support);
			isInCluster.fill(false);
			node cv;
			for (it=clusterNodes.begin(); it.valid(); ++it) {
				cv = support.copy(*it);
				isInCluster[c_support->copy(cv)] = true;
			}
			node v = c_support->firstNode();
			node succ;
			while (v!=0) {
				succ = v->succ();
				if (!isInCluster[v]) {
					c_support->delNode(v);
				}
				v = succ;
			}

			// Checking if Graph is connected.
			if (isConnected(*c_support)) {

				MinCut mc(*c_support,c_w);
				List<edge> cutEdges;
				double x = mc.minimumCut();
				if (x < 1.0-master()->eps()-minViolate) {
					mc.cutEdges(cutEdges,*c_support);
					List<nodePair> cutNodePairs;
					ListConstIterator<edge> cutEdgesIt;
					node v,w,cv,cw,ccv,ccw;
					nodePair np;
					for (cutEdgesIt=cutEdges.begin();cutEdgesIt.valid();cutEdgesIt++) {
						v = (*cutEdgesIt)->source();
						w = (*cutEdgesIt)->target();
						cv = c_support->original(v);
						cw = c_support->original(w);
						ccv = support.original(cv);
						ccw = support.original(cw);
						np.v1 = ccv;
						np.v2 = ccw;
						cutNodePairs.pushBack(np);
					}

					// Create constraint
					bufferedForCreation.push(new CutConstraint((Master*)master(),this, cutNodePairs));
					count++;
				}
			}//end Graph is connected

			else {
				NodeArray<int> comp(*c_support);
				connectedComponents(*c_support,comp);
				List<node> partition;
				NodeArray<bool> isInPartition(*c_support);
				isInPartition.fill(false);
				node v;
				forall_nodes(v,*c_support) {
					if (comp[v] == 0) {
						partition.pushBack(v);
						isInPartition[v] = true;
					}
				}

				List<nodePair> cutEdges;
				ListConstIterator<node> it;
				nodePair np;
				for (it=partition.begin(); it.valid(); ++it) {
					node w,cv,cw;
					forall_nodes(w,*c_support) {
						if ( (w!=(*it)) && !isInPartition[w] ) {
							cw = c_support->original(w);
							cv = c_support->original(*it);
							np.v1 = support.original(cw);
							np.v2 = support.original(cv);
							cutEdges.pushBack(np);
						}
					}
				}

				// Create Cut-constraint
				bufferedForCreation.push(new CutConstraint((Master*)master(), this, cutEdges)); // always violated enough.

				count++;
			}//end Graph is not connected
			delete c_support;
		}

		// Adding constraints
		if(count>0) {
			if(master()->pricing())
				nGenerated = createVariablesForBufferedConstraints();
			if(nGenerated==0) {
				ABA_BUFFER<ABA_CONSTRAINT*> cons(master(),count);
				while(!bufferedForCreation.empty()) {
					Logger::slout() <<"\n"; ((CutConstraint*&)bufferedForCreation.top())->printMe(Logger::slout());
					cons.push( bufferedForCreation.popRet() );
				}
				OGDF_ASSERT( bufferedForCreation.size()==0 );
				nGenerated = addCutCons(cons);
				OGDF_ASSERT( nGenerated == count );
				master()->updateAddedCCons(nGenerated);
			}
			m_constraintsFound = true;
			return nGenerated;
		}
	}//if no regeneration of connectivity cuts was possible

	//------------------------KURATOWSKI-SEPARATION----------------------------//

	// We first try to regenerate cuts from our cutpools
	nGenerated = separateKuraPool(minViolate);
	if (nGenerated > 0) {
		Logger::slout()<<"kura-regeneration.";
		return nGenerated; //TODO: Check if/how we can proceed here
	}
	// Since the Kuratowski support graph is computed from fractional values, an extracted
	// Kuratowski subdivision might not be violated by the current solution.
	// Thus, the separation algorithm is run several times, each time checking if the first
	// extracted subdivision is violated.
	// If no violated subdivisions have been extracted after \a nKuratowskiIterations iterations,
	// the algorithm behaves like "no constraints have been found".

	GraphCopy *kSupport;
	SList<KuratowskiWrapper> kuratowskis;
	BoyerMyrvold *bm1; BoyerMyrvold *bm2;
	bool violatedFound = false;

	// The Kuratowski support graph is created randomized  with probability xVal (1-xVal) to 0 (1).
	// Because of this, Kuratowski-constraints might not be found in the current support graph.
	// Thus, up to \a m_nKSupportGraphs are computed and checked for planarity.

	for (int i=0; i<((Master*)master_)->getNKuratowskiSupportGraphs(); ++i) {

		// If a violated constraint has been found, no more iterations have to be performed.
		if (violatedFound) break;

		kSupport = new GraphCopy (*((Master*)master())->getGraph());
		kuratowskiSupportGraph(*kSupport,((Master*)master_)->getKBoundLow(),((Master*)master_)->getKBoundHigh());

		if (isPlanar(*kSupport)) {
			delete kSupport;
			continue;
		}

		int iteration = 1;
		while(((Master*)master_)->getKIterations() >= iteration) {

			// Testing support graph for planarity.
			bm2 = new BoyerMyrvold();
			bool planar = bm2->planarEmbedDestructive(*kSupport, kuratowskis, ((Master*)master_)->getNSubdivisions(),false,false,true);
			delete bm2;

			// Checking if first subdivision is violated by current solution
			// Performance should be improved somehow!!!
			SListConstIterator<KuratowskiWrapper> kw = kuratowskis.begin();
			SListConstIterator<KuratowskiWrapper> succ;
			double leftHandSide = subdivisionLefthandSide(kw,kSupport);

			// Only violated constraints are created and added
			// if \a leftHandSide is greater than the number of edges in subdivision -1, the constraint is violated by current solution.
			if (leftHandSide > (*kw).edgeList.size()-(1-master()->eps()-minViolate)) {

				violatedFound = true;

				// Buffer for new Kuratowski constraints
				ABA_BUFFER<ABA_CONSTRAINT *> kConstraints(master_,kuratowskis.size());

				SListPure<edge> subdiv;
				SListPure<nodePair> subdivOrig;
				SListConstIterator<edge> eit;
				nodePair np;
				node v,w;

				for (eit = (*kw).edgeList.begin(); eit.valid(); ++eit) {
					subdiv.pushBack(*eit);
				}
				for (SListConstIterator<edge> sit = subdiv.begin(); sit.valid(); ++sit) {
					v = (*sit)->source();
					w = (*sit)->target();
					np.v1 = kSupport->original(v);
					np.v2 = kSupport->original(w);
					subdivOrig.pushBack(np);
				}

				// Adding first Kuratowski constraint to the buffer.
				kConstraints.push(new KuratowskiConstraint ((Master*)master(), subdivOrig.size(), subdivOrig));
				count++;
				subdiv.clear();
				subdivOrig.clear();

				// Checking further extracted subdivisions for violation.
				kw++;
				while(kw.valid()) {
					leftHandSide = subdivisionLefthandSide(kw,kSupport);
					if (leftHandSide > (*kw).edgeList.size()-(1-master()->eps()-minViolate)) {

						for (eit = (*kw).edgeList.begin(); eit.valid(); ++eit) {
							subdiv.pushBack(*eit);
						}
						for (SListConstIterator<edge> sit = subdiv.begin(); sit.valid(); ++sit) {
							v = (*sit)->source();
							w = (*sit)->target();
							np.v1 = kSupport->original(v);
							np.v2 = kSupport->original(w);
							subdivOrig.pushBack(np);
						}

						// Adding Kuratowski constraint to the buffer.
						kConstraints.push(new KuratowskiConstraint ((Master*)master(), subdivOrig.size(), subdivOrig) );
						count++;
						subdiv.clear();
						subdivOrig.clear();
					}
					kw++;
				}

				// Adding constraints to the pool.
				for(int i=0; i<kConstraints.number(); ++i) {
					Logger::slout() <<"\n"; ((KuratowskiConstraint*&)kConstraints[i])->printMe(Logger::slout());
				}
				nGenerated += addKuraCons(kConstraints);
				if (nGenerated != count)
				cerr << "Number of added constraints doesn't match number of created constraints" << endl;
				break;

			} else {
				kuratowskis.clear();
				iteration++;
			}
		}
		delete kSupport;
	}

	if (nGenerated > 0) {
		((Master*)master_)->updateAddedKCons(nGenerated);
		m_constraintsFound = true;
	}
	return nGenerated;
}

int Sub::createVariablesForBufferedConstraints() {
	List<ABA_CONSTRAINT*> crit;
	for(int i = bufferedForCreation.size(); i-->0;) {
//		((CutConstraint*)bufferedForCreation[i])->printMe(); Logger::slout() << ": ";
		for(int j=nVar(); j-->0;) {
//			((EdgeVar*)variable(j))->printMe();
//			Logger::slout() << "=" << bufferedForCreation[i]->coeff(variable(j)) << "/ ";
			if(bufferedForCreation[i]->coeff(variable(j))!=0.0) {
//				Logger::slout() << "!";
				goto nope;
			}
		}
		crit.pushBack(bufferedForCreation[i]);
		nope:;
	}
	if(crit.size()==0) return 0;
	ArrayBuffer<ListIterator<nodePair> > creationBuffer(crit.size());
	forall_nonconst_listiterators(nodePair, npit, master()->m_inactiveVariables) {
		bool select = false;
		ListIterator<ABA_CONSTRAINT*> ccit = crit.begin();
		while(ccit.valid()) {
			if(((BaseConstraint*)(*ccit))->coeff(*npit)) {
				ListIterator<ABA_CONSTRAINT*> delme = ccit;
				++ccit;
				crit.del(delme);
				select = true;
			} else
				++ccit;
		}
		if(select) creationBuffer.push(npit);
		if(crit.size()==0) break;
	}
	if( crit.size() ) { // something remained here...
		for(int i = bufferedForCreation.size(); i-->0;) {
			delete bufferedForCreation[i];
		}
		detectedInfeasibility = true;
		return 0; // a positive value denotes infeasibility
	}
	OGDF_ASSERT(crit.size()==0);
	ABA_BUFFER<ABA_VARIABLE*> vars(master(),creationBuffer.size());
	master()->m_varsCut += creationBuffer.size();
	int gen = creationBuffer.size();
	for(int j = gen; j-->0;) {
		vars.push( master()->createVariable( creationBuffer[j] ) );
	}
	myAddVars(vars);
	return -gen;
}

int Sub::pricingReal(double minViolate) {
	if(!master()->pricing()) return 0; // no pricing
	Top10Heap<Prioritized<ListIterator<nodePair> > > goodVar(master()->m_numAddVariables);
	forall_nonconst_listiterators(nodePair, it, master()->m_inactiveVariables) {
		double rc;
		EdgeVar v(master(), -master()->m_epsilon, EdgeVar::CONNECT, (*it).v1, (*it).v2);
		if(v.violated(rc) && rc>=minViolate) {
			Prioritized<ListIterator<nodePair> > entry(it,rc);
			goodVar.pushBlind( entry );
		}
	}

	int nv = goodVar.size();
	if(nv > 0) {
		ABA_BUFFER<ABA_VARIABLE*> vars(master(),nv);
		for(int i = nv; i-->0;) {
			ListIterator<nodePair> it = goodVar[i].item();
			vars.push( master()->createVariable(it) );
		}
		myAddVars(vars);
	}
	return nv;
}

int Sub::repair() {
	//warning. internal abacus stuff BEGIN
	bInvRow_ = new double[nCon()];
	lp_->getInfeas(infeasCon_, infeasVar_, bInvRow_);
	//warning. internal abacus stuff END

	// only output begin
	Logger::slout() << "lpInfeasCon=" << lp_->infeasCon()->number()
		<< " var="<< infeasVar_
		<< " con="<< infeasCon_<< "\n";
	for(int i=0; i<nCon(); ++i)
		Logger::slout() << bInvRow_[i] << " " << flush;
	Logger::slout() << "\n" << flush;
	for(int i=0; i<nCon(); ++i) {
		if(bInvRow_[i]!=0) {
			Logger::slout() << bInvRow_[i] << ": " << flush;
			ChunkConnection* chc = dynamic_cast<ChunkConnection*>(constraint(i));
			if(chc) chc->printMe(Logger::slout());
			CutConstraint* cuc = dynamic_cast<CutConstraint*>(constraint(i));
			if(cuc) cuc->printMe(Logger::slout());
			KuratowskiConstraint* kc = dynamic_cast<KuratowskiConstraint*>(constraint(i));
			if(kc) kc->printMe(Logger::slout());
			Logger::slout() << "\n" << flush;
		}
	}
	// only output end

	int added = 0;
	ABA_BUFFER<ABA_VARIABLE*> nv(master(),1);
	for(int i=0; i<nCon(); ++i) {
		if(bInvRow_[i]<0) { // negativ: infeasible cut or chunk constraint, or oversatisfies kura
			BaseConstraint* b = dynamic_cast<BaseConstraint*>(constraint(i));
			if(!b) continue; // was: oversatisfied kura. nothing we can do here
			OGDF_ASSERT(b);
			forall_nonconst_listiterators(nodePair, it, master()->m_inactiveVariables) {
				if(b->coeff(*it)) {
					Logger::slout() << "\tFeasibility Pricing: ";
					nv.push( master()->createVariable(it) );
					Logger::slout() << "\n";
					myAddVars(nv);
					added = 1;
					goto done;
				}
			}
		}
	}
done:
	//warning. internal abacus stuff BEGIN
	delete[] bInvRow_;
	//warning. internal abacus stuff END
	master()->m_varsKura += added;
	return added;
}

int Sub::solveLp() {
	m_reportCreation = 0;
	const double minViolation = 0.001; // value fixed by abacus...

	Logger::slout() << "SolveLp\tNode=" << this->id() << "\titeration=" << this->nIter_ << "\n";


	if(master()->pricing() && id()>1 && nIter_==1) { // ensure that global variables are really added...
		ABA_STANDARDPOOL<ABA_VARIABLE, ABA_CONSTRAINT>* vp = master()->varPool();
		int addMe = vp->number() - nVar();
		OGDF_ASSERT(addMe >=0 );
		if(addMe) {
			Logger::slout() << "ARRRGGGG!!!! ABACUS SUCKS!!\n";
			Logger::slout() << nVar() << " variables of " << vp->number() << " in model. Fetching " << addMe << ".\n" << flush;
			//master()->activeVars->loadIndices(this); // current indexing scheme
			m_reportCreation = 0;
			for(int i=0; i<vp->size(); ++i ) {
				ABA_POOLSLOT<ABA_VARIABLE, ABA_CONSTRAINT> * slot = vp->slot(i);
				ABA_VARIABLE* v = slot->conVar();
				if(v && !v->active()) {
					addVarBuffer_->insert(slot,true);
					--m_reportCreation;
				}
			}
			OGDF_ASSERT(m_reportCreation == -addMe);
			return 0; // rerun;
		}
	}


	if(master()->m_checkCPlanar && master()->feasibleFound()) {
		Logger::slout() << "Feasible Solution Found. That's good enough! C-PLANAR\n";
		master()->clearActiveRepairs();
		return 1;
	}

	if(bufferedForCreation.size()) {
		m_reportCreation = bufferedForCreation.size();
		ABA_BUFFER<ABA_CONSTRAINT*> cons(master(),bufferedForCreation.size());
		while(!bufferedForCreation.empty()) {
			((CutConstraint*&)bufferedForCreation.top())->printMe(Logger::slout());Logger::slout() <<"\n";
			cons.push( bufferedForCreation.popRet() );
		}
		OGDF_ASSERT( bufferedForCreation.size()==0 );
		addCutCons(cons);
		master()->updateAddedCCons(m_reportCreation);
		master()->clearActiveRepairs();
		return 0;
	}

	inOrigSolveLp = true;
	++(master()->m_solvesLP);
	int ret = ABA_SUB::solveLp();
	inOrigSolveLp = false;
	if(ret) {
		if(!(master()->m_checkCPlanar))
			return ret;
		if(master()->pricing()) {
			if(criticalSinceBranching.size()) {
				ListIterator<nodePair> best;
				Array<ListIterator<ABA_CONSTRAINT*> > bestKickout;
				int bestCCnt = 0;
				forall_nonconst_listiterators(nodePair, nit, master()->m_inactiveVariables) {
					ArrayBuffer<ListIterator<ABA_CONSTRAINT*> > kickout(criticalSinceBranching.size());
					forall_nonconst_listiterators(ABA_CONSTRAINT*, cit, criticalSinceBranching) {
						BaseConstraint* bc = dynamic_cast<BaseConstraint*>(*cit);
						OGDF_ASSERT(bc);
						if( bc->coeff(*nit) > 0.99) {
							kickout.push(cit);
						}
					}
					if(kickout.size() > bestCCnt) {
						bestCCnt = kickout.size();
						best = nit;
						kickout.compactMemcpy(bestKickout);
					}
				}
				if(bestCCnt>0) {
					ABA_BUFFER<ABA_VARIABLE*> vars(master(),1);
					vars.push( master()->createVariable(best) );
					myAddVars(vars);
					int i;
					forall_arrayindices(i,bestKickout)
						criticalSinceBranching.del(bestKickout[i]);
					m_reportCreation = -1;
					++(master()->m_varsBranch);
					master()->clearActiveRepairs();
					return 0;
				}
				criticalSinceBranching.clear(); // nothing helped... resorting to full repair
//				master()->clearActiveRepairs();
//				return 0;
			} //else {
			m_reportCreation = -repair();
			if(m_reportCreation<0) {
				++(master()->m_activeRepairs);
				return 0;
			}
			//}
		}
		master()->clearActiveRepairs();
		dualBound_ = -master()->infinity();


#ifdef OGDF_DEBUG
	forall_listiterators(nodePair, it, master()->m_inactiveVariables) {
		int t = (*it).v1->index();
		if(t==0) {
			if( (*it).v2->index()==35 )
				Logger::slout() << "VAR MISSING: 0-35\n";
		}
		else {
			if(t%6==0) continue;
			if((*it).v2->index()==t+5 )
				Logger::slout() << "VAR MISSING: " << t << "-" << (t+5) << "\n";
		}
	}
	for(int t = 0; t<nVar(); ++t) {
		EdgeVar* v = (EdgeVar*)(variable(t));
		if( ( v->sourceNode()->index()==27 && v->targetNode()->index()==32 ) ||
				( v->sourceNode()->index()==32 && v->targetNode()->index()==27 )) {
			Logger::slout() << "VAR 27-32: " << xVal(t) << "(" << lBound(t) << "," << uBound(t) << ")\n";
		}
	}
	for(int t = 0; t<nVar(); ++t) {
		EdgeVar* v = (EdgeVar*)(variable(t));
		if(v->theEdgeType()==EdgeVar::CONNECT) {
			if(lBound(t)==uBound(t)) {
				Logger::slout() << "VAR FIXED: ";
				v->printMe(Logger::slout());
				Logger::slout() << " TO " << lBound(t) << "\n";
			}
		}
	}
#endif //OGDF_DEBUG

		// infeasibleSub(); // great! a virtual function that is private...
		Logger::slout() << "\tInfeasible\n";
		return 1; // report any errors
	}
	master()->clearActiveRepairs();
	OGDF_ASSERT( !lp_->infeasible() );
	//is set here for pricing only
	if(master()->m_checkCPlanar) // was: master()->pricing()
		dualBound_=master()->infinity();//666
	Logger::slout() << "\t\tLP-relaxation: " <<  lp_->value() << "\n";
    Logger::slout() << "\t\tLocal/Global dual bound: " << dualBound() << "/" << master_->dualBound() << endl;
	realDualBound = lp_->value();

	//if(master()->m_checkCPlanar2 && dualBound()<master()->m_G->numberOfEdges()-0.79) {
	//	dualBound(-master()->infinity());
	//	return 1;
	//}


	if(!master()->pricing()) {
		m_reportCreation = separateReal(minViolation);//use ...O for output
	} else {
		m_sepFirst = !m_sepFirst;
		if(m_sepFirst) {
			if( m_reportCreation = separateRealO(master()->m_strongConstraintViolation) ) return 0;
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (a)"<< endl; return 1; }
			if( m_reportCreation = -pricingRealO(master()->m_strongVariableViolation) ) return 0;
			if( m_reportCreation = separateRealO(minViolation)) return 0;
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (b)"<< endl; return 1; }
			m_reportCreation = -pricingRealO(minViolation);
		} else {
			if( m_reportCreation = -pricingRealO(master()->m_strongVariableViolation) ) return 0;
			if( m_reportCreation = separateRealO(master()->m_strongConstraintViolation) ) return 0;
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (c)"<< endl; return 1; }
			if( m_reportCreation = -pricingRealO(minViolation)) return 0;
			m_reportCreation = separateRealO(minViolation);
			if( detectedInfeasibility ) { Logger::slout() << "Infeasibility detected (d)"<< endl; return 1; }
		}
	}
	return 0;
}


#endif // USE_ABACUS
