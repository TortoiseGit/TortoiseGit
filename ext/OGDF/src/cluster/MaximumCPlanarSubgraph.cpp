 /*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class MaximumCPlanarSubgraph
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

#ifdef USE_ABACUS

#include <ogdf/cluster/MaximumCPlanarSubgraph.h>
#include <ogdf/cluster/CconnectClusterPlanar.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <sstream>

//#define writefeasiblegraphs

namespace ogdf {

struct connStruct {
	bool connected;
	node v1, v2;
	edge e;
};

Module::ReturnType MaximumCPlanarSubgraph::doCall(const ClusterGraph &G,
            List<edge> &delEdges,
            List<nodePair> &addedEdges)
{
#ifdef OGDF_DEBUG
    cout << "Creating new Masterproblem for clustergraph with "<<G.getGraph().numberOfNodes()<<" nodes\n";
#endif
    Master* cplanMaster = new Master(G,m_heuristicLevel,
    								 m_heuristicRuns,
    							     m_heuristicOEdgeBound,
    							     m_heuristicNPermLists,
    							     m_kuratowskiIterations,
    							     m_subdivisions,
    								 m_kSupportGraphs,
    								 m_kuratowskiHigh,
    								 m_kuratowskiLow,
    								 m_perturbation,
    								 m_branchingGap,
    								 m_time,
    								 m_pricing,
    								 m_checkCPlanar,
    								 m_numAddVariables,
    								 m_strongConstraintViolation,
    								 m_strongVariableViolation,
    								 m_ol);

    cplanMaster->setPortaFile(m_portaOutput);
    cplanMaster->useDefaultCutPool() = m_defaultCutPool;
#ifdef OGDF_DEBUG
    cout << "Starting Optimization\n";
#endif
	ABA_MASTER::STATUS status;
	try {
    	status = cplanMaster->optimize();
	}
	catch (...)
	{
		#ifdef OGDF_DEBUG
		cout << "ABACUS Optimization failed...\n";
		#endif
	}
    m_totalTime    = getDoubleTime(*cplanMaster->totalTime());
    m_heurTime     = getDoubleTime(*cplanMaster->improveTime());
    m_sepTime      = getDoubleTime(*cplanMaster->separationTime());
    m_lpTime       = getDoubleTime(*cplanMaster->lpTime());
    m_lpSolverTime = getDoubleTime(*cplanMaster->lpSolverTime());
    m_totalWTime   = getDoubleTime(*cplanMaster->totalCowTime());
    m_numKCons     = cplanMaster->addedKConstraints();
    m_numCCons     = cplanMaster->addedCConstraints();
    m_numLPs       = cplanMaster->nLp();
    m_numBCs       = cplanMaster->nSub();
    m_numSubSelected = cplanMaster->nSubSelected();
    m_numVars      = cplanMaster->nMaxVars()-cplanMaster->getNumInactiveVars();
#ifdef OGDF_DEBUG
	m_solByHeuristic = cplanMaster->m_solByHeuristic;
#endif
#ifdef OGDF_DEBUG
	if(cplanMaster->pricing())
		Logger::slout() << "Pricing was ON\n";
   	Logger::slout()<<"ABACUS returned with status '"<< ABA_MASTER::STATUS_[status] <<"'\n"<<flush;
#endif

    List<nodePair> allEdges;
    cplanMaster->getDeletedEdges(delEdges);
    cplanMaster->getConnectionOptimalSolutionEdges(addedEdges);
    cplanMaster->getAllOptimalSolutionEdges(allEdges);
    int delE = delEdges.size();
    int addE = addedEdges.size();

#ifdef OGDF_DEBUG
    cout<<delE<< " Number of deleted edges, "<<addE<<" Number of added edges "<<
    allEdges.size()<<" gesamt"<<"\n";
#endif

	if (m_portaOutput)
	{
		writeFeasible(getPortaFileName(), *cplanMaster, status);
	}

	delete cplanMaster;
    switch (status) {
    case ABA_MASTER::Optimal: return Module::retOptimal; break;
    case ABA_MASTER::Error: return Module::retError; break;
    default: break;
    }//switch

    return Module::retError;
}//docall for clustergraph


//returns list of all clusters in subtree at c in bottom up order
void MaximumCPlanarSubgraph::getBottomUpClusterList(const cluster c, List< cluster > & theList)
{
	ListConstIterator<cluster> it = c->cBegin();
	while (it.valid())
	{
		getBottomUpClusterList((*it), theList);
		it++;
	}
	theList.pushBack(c);
}

//outputs the set of feasible solutions
void MaximumCPlanarSubgraph::writeFeasible(const String &filename,
	Master &master,
	ABA_MASTER::STATUS &status)
{
	const ClusterGraph& CG = *(master.getClusterGraph());
	const Graph& G = CG.getGraph();
	node v;
	//first compute the nodepairs that are potential candidates to connect
	//chunks in a cluster
	//potential connection edges
	NodeArray< NodeArray<bool> > potConn(G);
	forall_nodes(v, G)
	{
		potConn[v].init(G, false);
	}
	//we perform a bottom up cluster tree traversal
	List< cluster > clist;
	getBottomUpClusterList(CG.rootCluster(), clist);
	//could use postordertraversal instead

	List< nodePair > connPairs; //holds all connection node pairs
	//counts the number of potential connectivity edges
	//int potCount = 0; //equal to number of true values in potConn

	//we run through the clusters and check connected components
	//we consider all possible edges connecting CCs in a cluster,
	//even if they may be connected by edges in a child cluster
	//(to get the set of all feasible solutions)

	ListConstIterator< cluster > it = clist.begin();
	while (it.valid())
	{
		cluster c = (*it);
		//we compute the subgraph induced by vertices in c
		GraphCopy gcopy;
		gcopy.createEmpty(G);
		List<node> clusterNodes;
		//would be more efficient if we would just merge the childrens' vertices
		//and add c's
		c->getClusterNodes(clusterNodes);
		NodeArray<bool> activeNodes(G, false); //true for all cluster nodes
		EdgeArray<edge> copyEdge(G); //holds the edge copy
		ListConstIterator<node> itn = clusterNodes.begin();
		while (itn.valid())
		{
			activeNodes[(*itn)] = true;
			itn++;
		}
		gcopy.initByActiveNodes(clusterNodes, activeNodes, copyEdge);
		//gcopy now represents the cluster induced subgraph

		//we compute the connected components and store all nodepairs
		//that connect two of them
		NodeArray<int> component(gcopy);
		connectedComponents(gcopy, component);
		//now we run over all vertices and compare the component
		//number of adjacent vertices. If they differ, we found a
		//potential connection edge. We do not care if we find them twice.
		forall_nodes(v, gcopy)
		{
			node w;
			forall_nodes(w, gcopy)
			{
				if (component[v] != component[w])
				{
					cout <<"Indizes: "<<v->index()<<":"<<w->index()<<"\n";
					node vg = gcopy.original(v);
					node wg = gcopy.original(w);
					bool newConn = !((vg->index() < wg->index()) ? potConn[vg][wg] : potConn[wg][vg]);
					if (newConn)
					{
						nodePair np; np.v1 = vg; np.v2 = wg;
						connPairs.pushBack(np);
						if (vg->index() < wg->index())
							potConn[vg][wg] = true;
						else
							potConn[wg][vg] = true;
					}
				}
			}//nodes
		}//nodes

		it++;
	}//while

	cout << "Potentielle Verbindungskanten: "<< connPairs.size()<<"\n";

	//we run through our candidates and save them in an array
	//that can be used for dynamic graph updates
	int i = 0;
	connStruct *cons = new connStruct[connPairs.size()];
	ListConstIterator< nodePair > itnp = connPairs.begin();
	while (itnp.valid())
	{
		connStruct cs;
		cs.connected = false;
		cs.v1 = (*itnp).v1;
		cs.v2 = (*itnp).v2;
		cs.e  = 0;

		cons[i] = cs;
		i++;
		itnp++;
	}//while

	//-------------------------------------------------------------------------
	// WARNING: this is extremely slow for graphs with a large number of cluster
	// chunks now we test all possible connection edge combinations for c-planarity
	Graph G2;

	NodeArray<node> origNodes(CG.getGraph());
	ClusterArray<cluster> origCluster(CG);
	EdgeArray<edge> origEdges(CG.getGraph());
	ClusterGraph testCopy(CG, G2, origCluster, origNodes, origEdges);

	ofstream os(filename.cstr());

	// Output dimension of the LP (number of variables)
	os << "DIM = " << connPairs.size() << "\n";
    os << "COMMENT\n";
	char* stat = new char[10];
	std::sprintf(stat, "%s \n","unknown");
	switch (status) {
     	case ABA_MASTER::Optimal: std::sprintf(stat, "%s \n", "Optimal"); break;
    	case ABA_MASTER::Error: std::sprintf(stat, "%s \n", "Error"); break;
    	default: break;
    }//switch
	os << stat << "\n";
	delete[] stat;
	for (i = 0; i < connPairs.size(); i++)
	{
		os << "Var " << i << ": " << origNodes[cons[i].v1]->index() << "->" << origNodes[cons[i].v2] << "\n";
	}

	os << "CONV_SECTION\n";

	int j = 0; //debug
	if (connPairs.size() > 0)
	while (true)
	{
		//we create the next test configuration by incrementing the edge selection array
		//we create the corresponding graph dynamically on the fly
		i = 0;
		while ( (i < connPairs.size()) && (cons[i].connected == true) )
		{
			cons[i].connected = false;
			OGDF_ASSERT(cons[i].e != 0);
			G2.delEdge(cons[i].e);
			i++;
		}//while
		if (i >= connPairs.size()) break;
		//cout<<"v1graph: "<<&(*(cons[i].v1->graphOf()))<<"\n";
		//cout<<"origNodesgraph: "<<&(*(origNodes.graphOf()))<<"\n";
		cons[i].connected = true; //i.e., (false) will never be a feasible solution
		cons[i].e = G2.newEdge(origNodes[cons[i].v1], origNodes[cons[i].v2]);


		//and test it for c-planarity
		CconnectClusterPlanar CCCP;
		bool cplanar = CCCP.call(testCopy);

		//c-planar graphs define a feasible solution
		if (cplanar)
		{
			cout << "Feasible solution found\n";
			for (int j = 0; j < connPairs.size(); j++)
			{
				char ch = (cons[j].connected ? '1' : '0');
				cout << ch;
				os << ch << " ";
			}
			cout << "\n";
			os << "\n";
#ifdef writefeasiblegraphs
			char* fn = new char[20];
			std::sprintf(fn, "cGraph%d.gml",j++);
			testCopy.writeGML(fn);
			delete[] fn;
#endif
		}
	}//while counting

	delete[] cons;

	os << "\nEND" <<"\n";
	os.close();

	//return;

	os.open(getIeqFileName());
	os << "DIM = " << m_numVars << "\n";
	// Output the status as a comment
	os << "COMMENT\n";
	char* lstat = new char[10];
	std::sprintf(stat, "%s \n","unknown");
	switch (status) {
     	case ABA_MASTER::Optimal: std::sprintf(lstat, "%s \n", "Optimal"); break;
    	case ABA_MASTER::Error: std::sprintf(lstat, "%s \n", "Error"); break;
    	default: break;
    }//switch
	os << lstat << "\n";
	delete[] lstat;
	// In case 0 is not a valid solution, some PORTA functions need
	//a valid solution in the ieq file
	os << "VALID\n";

	os << "\nLOWER_BOUNDS\n";

	for (i = 0; i < m_numVars; i++) os << "0 ";
	os << "\n";

	os << "\nHIGHER_BOUNDS\n";
	for (i = 0; i < m_numVars; i++) os << "1 ";
	os << "\n";

	os << "\nINEQUALITIES_SECTION\n";
	//we first read the standard constraint that are written
	//into a text file by the optimization master
	ifstream isf(master.getStdConstraintsFileName());
	if (!isf)
	{
		cerr << "Could not open optimization master's standard constraint file\n";
		os << "#No standard constraints read\n";
	}
	else
	{
		char* fileLine = new char[maxConLength()];
		while (isf.getline(fileLine, maxConLength()))
		{
			//skip commment lines
			if (fileLine[0] == '#') continue;
			int count = 1;
			std::istringstream iss(fileLine);
			char d;
			bool rhs = false;
			while (iss >> d)
			{
				if ( rhs || ( (d == '<') || (d == '>') || (d == '=') ) )
				{
					os << d;
					rhs = true;
				}
				else
				{
					if (d != '0')
					{
						os <<"+"<< d <<"x"<<count;
					}
					count++;
				}
			}//while chars
			os <<"\n";
		}
		delete[] fileLine;
	}//ifstream
	//now we read the cut pools from the master
	if (master.useDefaultCutPool())
	{
		os << "#No cut constraints read from master\n";
		//ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE> *connCon = master.cutPool();
	}
	else
	{
		ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE> *connCon = master.getCutConnPool();
		ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE> *kuraCon = master.getCutKuraPool();
		ABA_STANDARDPOOL<ABA_VARIABLE, ABA_CONSTRAINT> *stdVar = master.varPool();
		OGDF_ASSERT(connCon != 0);
		OGDF_ASSERT(kuraCon != 0);
		cout << connCon->number() << " Constraints im MasterConnpool \n";
		cout << kuraCon->number() << " Constraints im MasterKurapool \n";
		cout << connCon->size() << " Größe ConnPool"<<"\n";
		outputCons(os, connCon, stdVar);
		outputCons(os, kuraCon, stdVar);
	}//else
	os << "\nEND" <<"\n";
	os.close();
	cout << "Cutting is set: "<<master.cutting()<<"\n";
	//cout <<"Bounds for the variables:\n";
	//ABA_SUB &theSub = *(master.firstSub());
	//for ( i = 0; i < theSub.nVar(); i++)
	//{
	//	cout << i << ": " << theSub.lBound(i) << " - " << theSub.uBound(i) << "\n";
	//}
	/*// OLD CRAP
	cout << "Constraints: \n";
	ABA_STANDARDPOOL< ABA_CONSTRAINT, ABA_VARIABLE > *spool = master.conPool();
	ABA_STANDARDPOOL< ABA_CONSTRAINT, ABA_VARIABLE > *cpool = master.cutPool();

	cout << spool->size() << " Constraints im Masterpool \n";
	cout << cpool->size() << " Constraints im Mastercutpool \n";

	cout << "ConPool Constraints \n";
	for ( i = 0; i < spool->size(); i++)
	{
		ABA_POOLSLOT< ABA_CONSTRAINT, ABA_VARIABLE > * sloty = spool->slot(i);
		ABA_CONSTRAINT *mycon = sloty->conVar();
		switch (mycon->sense()->sense())
		{
			case ABA_CSENSE::Less: cout << "<" << "\n"; break;
			case ABA_CSENSE::Greater: cout << ">" << "\n"; break;
			case ABA_CSENSE::Equal: cout << "=" << "\n"; break;
			default: cout << "Inequality sense doesn't make any sense \n"; break;
		}//switch
	}
	cout << "CutPool Constraints \n";
	for ( i = 0; i < cpool->size(); i++)
	{
		ABA_POOLSLOT< ABA_CONSTRAINT, ABA_VARIABLE > * sloty = cpool->slot(i);
		ABA_CONSTRAINT *mycon = sloty->conVar();
		switch (mycon->sense()->sense())
		{
			case ABA_CSENSE::Less: cout << "<" << "\n"; break;
			case ABA_CSENSE::Greater: cout << ">" << "\n"; break;
			case ABA_CSENSE::Equal: cout << "=" << "\n"; break;
			default: cout << "Inequality sense doesn't make any sense \n"; break;
		}//switch
	}
	*/
	/*
	for ( i = 0; i < theSub.nCon(); i++)
	{
		ABA_CONSTRAINT &theCon = *(theSub.constraint(i));

		for ( i = 0; i < theSub.nVar(); i++)
		{
			double c = theCon.coeff(theSub.variable(i));
			if (c != 0)
				cout << c;
			else cout << "  ";
		}
		switch (theCon.sense()->sense())
		{
			case ABA_CSENSE::Less: cout << "<" << "\n"; break;
			case ABA_CSENSE::Greater: cout << ">" << "\n"; break;
			case ABA_CSENSE::Equal: cout << "=" << "\n"; break;
			default: cout << "doesn't make any sense \n"; break;
		}//switch
		#include<limits> //for numeric_limits
  float fl;
  while(!(std::cin >> fl))
  {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<streamsize>::max(),'\n');
  }
	}*/
}//writeportaieq

void MaximumCPlanarSubgraph::outputCons(ofstream &os,
	ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE> *connCon,
	ABA_STANDARDPOOL<ABA_VARIABLE, ABA_CONSTRAINT> *stdVar)
{
	int i;
	for ( i = 0; i < connCon->number(); i++)
		{
			ABA_POOLSLOT< ABA_CONSTRAINT, ABA_VARIABLE > * sloty = connCon->slot(i);
			ABA_CONSTRAINT *mycon = sloty->conVar();
			OGDF_ASSERT(mycon != 0);
			int count;
			for (count = 0; count < stdVar->size(); count++)
			{
				ABA_POOLSLOT< ABA_VARIABLE, ABA_CONSTRAINT > * slotv = stdVar->slot(count);
				ABA_VARIABLE *myvar = slotv->conVar();
				double d = mycon->coeff(myvar);
				if (d != 0.0) //precision!
				{
					os <<"+"<< d <<"x"<<count+1;
				}
			}//for
			switch (mycon->sense()->sense())
			{
				case ABA_CSENSE::Less: os << " <= "; break;
				case ABA_CSENSE::Greater: os << " >= "; break;
				case ABA_CSENSE::Equal: os << " = "; break;
				default: os << "Inequality sense doesn't make any sense \n";
						 cerr << "Inequality sense unknown \n";
						break;
			}//switch
			os << mycon->rhs();
			os << "\n";
		}
}

} //end namespace ogdf

#endif //USE_ABACUS
