/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of a clustering algorithm (by Auber, Chiricota,
 * Melancon).
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

#include <ogdf/graphalg/Clusterer.h>
#include <assert.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf {

Clusterer::Clusterer(const Graph &G) :
	ClustererModule(G),
	m_recursive(true),
	m_autoThreshNum(0)
{
	//use automatic thresholds
	//m_thresholds.pushFront(3.0);
	//m_thresholds.pushFront(1.4);
	m_defaultThresholds.pushFront(1.6);
	m_defaultThresholds.pushBack(3.2);
	m_defaultThresholds.pushBack(4.5);
	m_stopIndex = 0.7;
}


//computes a list of SimpleCluster pointers, where each SimpleCluster
//models a cluster and has a parent (0 if root)
//list of cluster structures will be parameter
void Clusterer::computeClustering(SList<SimpleCluster*> &clustering)
{
	//save to which cluster the vertices belong
	//0 is root cluster
	SimpleCluster *root = new SimpleCluster();
	root->m_size = m_pGraph->numberOfNodes();

	//we push root to the front afterwards
	//clustering.pushFront(root);
	NodeArray<SimpleCluster*> vCluster(*m_pGraph, root);

	//we delete edges and therefore work on a copy
	GraphCopy GC(*m_pGraph);
	//multiedges are not used for the computation
	makeSimple(GC);
	//based on strengths, we compute a clustering
	//Case 1:with depth one (or #thresholds)
	//Case 2: Recursively

	edge e;
	ListIterator<double> it;

	if (m_thresholds.size() > 0)
		it = m_thresholds.begin();
	else
	{
			OGDF_ASSERT(m_defaultThresholds.size() > 0);
			it = m_defaultThresholds.begin();
	}

	//are there more cases than (not) rec.?
	int fall = (m_recursive ? 2 : 1);
	//int fall = 2;
	if (fall == 2)
	{
		//we just use the first value of auto/default/threshs
		//assert(m_thresholds.size() == 1);
		//Case2:
		//we compute the edge strengths on the components recursively
		//we therefore only need a single threshold

		//we compute the edge strengths
		EdgeArray<double> strengths(GC,0.0);

		//we need a criterion to stop the process
		//we stop if there are no edges deleted or
		//the average node clustering index in our copy rises
		//above m_stopIndex
		bool edgesDeleted = true;

		while (edgesDeleted && (averageCIndex(GC)<m_stopIndex))
		{

			computeEdgeStrengths(GC, strengths);
			if (m_autoThreshNum > 0)
			{
				OGDF_ASSERT(m_autoThresholds.size() > 0);
				it = m_autoThresholds.begin();
			}
			if (it.valid())
			{
				//collect edges that will be deleted
				List<edge> le;
				forall_edges(e, GC)
				{
					if (strengths[e] < *it)
					{
						le.pushFront(e);
					}
				}
				//stop criterion
				if (le.size() == 0)
				{
					edgesDeleted = false;
					continue;
				}
				//delete edges
				ListIterator<edge> itle = le.begin();
				while (itle.valid())
				{
					GC.delEdge(*itle);
					itle++;
				}
				//gather cluster information
				node v;
				//vertices within a connected component are always part
				//of the same cluster which then will be parent of the new clusters

				StackPure<node> S;
				NodeArray<bool> done(GC, false);
				forall_nodes(v,GC)
				{
					if (done[v]) continue;
					done[v] = true;
					S.push(v);
					SimpleCluster* parent = vCluster[GC.original(v)];
					SList<node> theCluster;

					List<node> compNodes; //nodes in a component

					while(!S.empty())
					{
						node w = S.pop();
						compNodes.pushFront(w);
						edge e;
						forall_adj_edges(e,w)
						{
							node x = e->opposite(w);
							if (!(done[x]))
							{
								done[x] = true;
								S.push(x);
							}
						}
					}//While
					//run over nodes and set cluster info
					//do not construct trivial clusters
					if ( (parent->m_size > compNodes.size()) &&
						(compNodes.size()>2))
					{
						SimpleCluster* s = new SimpleCluster();
						s->setParent(parent);

						parent->pushBackChild(s);
						clustering.pushFront(s);
						ListIterator<node> it = compNodes.begin();
						while (it.valid())
						{
							vCluster[GC.original(*it)] = s;
							//vertex leaves parent to s
							parent->m_size--;
							s->m_size++;
							it++;
						}//While
					}
				}//forallnodes
			}//if thresholds
		}//stop criterion
	}
	else //fall 2
	{
		//we compute the edge strengths
		EdgeArray<double> strengths(*m_pGraph,0.0);
		computeEdgeStrengths(strengths);
		if (m_autoThreshNum > 0)
		{
			OGDF_ASSERT(m_autoThresholds.size() > 0);
			it = m_autoThresholds.begin();
		}
		//Case1:

		while (it.valid())
		{
			//collect edges that will be deleted
			List<edge> le;
			forall_edges(e, GC)
			{
				if (strengths[e] < *it)
				{
					le.pushFront(e);
				}
			}
			//delete edges
			ListIterator<edge> itle = le.begin();
			while (itle.valid())
			{
				GC.delCopy(*itle);
				itle++;
			}
			//gather cluster information
			node v;
			//vertices within a connected component are always part
			//of the same cluster which then will be parent of the new clusters

			StackPure<node> S;
			NodeArray<bool> done(GC, false);
			forall_nodes(v,GC)
			{
				if (done[v]) continue;
				done[v] = true;
				S.push(v);
				SimpleCluster* parent = vCluster[GC.original(v)];
				SList<node> theCluster;

				List<node> compNodes; //nodes in a component

				//do not immediately construct a cluster, first gather and store
				//vertices, then test if the cluster fits some constraints, e.g. size
				while(!S.empty())
				{
					node w = S.pop();
					compNodes.pushFront(w);
					edge e;
					forall_adj_edges(e,w)
					{
						node x = e->opposite(w);
						if (!(done[x]))
						{
							done[x] = true;
							S.push(x);
						}
					}
				}//While

				//run over nodes and set cluster info
				//do not construct trivial clusters
				if ( (parent->m_size > compNodes.size()) &&
					(compNodes.size()>2))
				{
					SimpleCluster* s = new SimpleCluster;
					s->setParent(parent);
					//s->m_index = -1;
					parent->pushBackChild(s);
					clustering.pushFront(s);
					ListIterator<node> it = compNodes.begin();
					while (it.valid())
					{
						vCluster[GC.original(*it)] = s;
						//vertex leaves parent to s
						parent->m_size--;
						s->m_size++;
						it++;
					}//while
				}
			}//while thresholds

			it++;
		}//while
	}//case 2

	clustering.pushFront(root);
	//we now have the clustering and know for each vertex, to which cluster
	//it was assigned in the last iteration (vCluster)
	//we update the lists of children
	node v;
	forall_nodes(v, *m_pGraph)
	{
		vCluster[v]->pushBackVertex(v);
	}
}//computeClustering

void Clusterer::computeEdgeStrengths(EdgeArray<double> &strength)
{
	const Graph &G = *m_pGraph;
	computeEdgeStrengths(G, strength);
}

void Clusterer::computeEdgeStrengths(const Graph &G, EdgeArray<double> &strength)
{
	edge e;
	strength.init(G, 0.0);
	double minStrength = 5.0, maxStrength = 0.0; //used to derive automatic thresholds
	//5 is the maximum possible value (sum of five values 0-1)

	//A Kompromiss: Entweder immer Nachbarn der Nachbarn oder einmal berechnen und speichern (gut
	//wenn haeufig benoetigt (hoher Grad), braucht aber viel Platz
	//B Was ist schneller: Listen nachher nochmal durchlaufen und loeschen, wenn Doppelnachbar oder
	//gleich beim Auftreten loeschen ueber Iterator

	//First, compute the sets Mw, Mv, Wwv. Then check their connectivity.
	//Use a list for the vertices that are solely connected to w
	forall_edges(e, G)
	{
		List<node> vNb;
		List<node> wNb;
		List<node> bNb; //neighbour to both vertices
		EdgeArray<bool> processed(G, false); //edge was processed
		NodeArray<int> nba(G, 0); //neighbours of v and w: 1v, 2both, 3w

		node v = e->source();
		node w = e->target();

		adjEntry adjE;
		//neighbourhood sizes
		int sizeMv = 0;
		int sizeMw = 0;
		int sizeWvw = 0;
		//neighbourhood links
		//int rMv = 0; //within MV
		//int rMw = 0;
		int rWvw = 0;

		int rMvMw = 0; //from Mv to Mw
		int rMvWvw = 0;
		int rMwWvw = 0;

		//-----------------------------------------
		//Compute neighbourhood
		//Muss man selfloops gesondert beruecksichtigen
		forall_adj(adjE, v)
		{
			node u = adjE->twinNode();
			if (u == v) continue;
			if ( u != w )
			{
				nba[u] = 1;
			}
		}
		forall_adj(adjE, w)
		{
			node u = adjE->twinNode();
			if (u == w) continue;
			if ( u != v )
			{
				if (nba[u] == 1)
				{
					nba[u] = 2;
				}
				else
				{
					if (nba[u] != 2) nba[u] = 3;

					sizeMw++;
					wNb.pushFront(u);
				}
			}
		}// foralladjw

		//Problem in der Laufzeit ist die paarweise Bewertung der Nachbarschaft
		//ohne Nutzung vorheriger Informationen

		//We know the neighbourhood of v and w and have to compute the connectivity
		forall_adj(adjE, v)
		{
			node u = adjE->twinNode();

			if ( u != w )
			{
				adjEntry adjE2;
				//check if u is in Mv
				if (nba[u] == 1)
				{
					//vertex in Mv
					sizeMv++;
					//check links within Mv, to Mw and Wvw
					forall_adj(adjE2, u)
					{
						processed[adjE2->theEdge()] = true;
						node t = adjE2->twinNode();
						//test links to other sets
						switch (nba[t]) {
							//case 1: rMv++; break;
						case 2: rMvWvw++; break;
						case 3: rMvMw++; break;
						}//switch

					}

				}
				else
				{
					//vertex in Wvw, nba == 2
					assert(nba[u] == 2);
					sizeWvw++;
					forall_adj(adjE2, u)
					{
						node t = adjE2->twinNode();
						//processed testen?
						//test links to other sets
						switch (nba[t]) {
							//case 1: rMv++; break;
						case 2: rWvw++; break;
						case 3: rMwWvw++; break;
						}//switch

					}

				}//else
			}//if not w
		}//foralladj

		//Now compute the ratio of existing edges to maximal number
		//(complete graph)

		double sMvWvw = 0.0;
		double sMwWvw = 0.0;
		double sWvw = 0.0;
		double sMvMw = 0.0;
		//we have to cope with special cases
		int smult = sizeMv*sizeWvw;
		if (smult != 0) sMvWvw = (double)rMvWvw/smult;
		smult = sizeMw*sizeWvw;
		if (smult != 0) sMwWvw = (double)rMwWvw/smult;
		smult = (sizeMv*sizeMw);
		if (smult != 0) sMvMw = (double)rMvMw/smult;


		if (sizeWvw > 1)
			sWvw   = 2.0*rWvw/(sizeWvw*(sizeWvw-1));
		else if (sizeWvw == 1) sWvw = 1.0;
		//Ratio of cycles of size 3 and 4
		double cycleProportion = ((double)sizeWvw/(sizeWvw+sizeMv+sizeMw));
		double edgeStrength = sMvWvw+sMwWvw+sWvw+sMvMw+cycleProportion;

		if (m_autoThreshNum > 0)
		{
			if (minStrength > edgeStrength) minStrength = edgeStrength;
			if (maxStrength < edgeStrength) maxStrength = edgeStrength;
		}

		//cout<<"sWerte: "<<sMvWvw<<"/"<<sMwWvw<<"/"<<sWvw<<"/"<<sMvMw<<"\n";
		//cout << "CycleProportion "<<cycleProportion<<"\n";
		//cout << "EdgeStrength "<<edgeStrength<<"\n";
		strength[e] = edgeStrength;

		//true neighbours are not adjacent to v
		//cout << "Checking true neighbours of w\n";

	}//foralledges
	if (m_autoThreshNum > 0)
	{
		if (m_autoThresholds.size()>0) m_autoThresholds.clear();
		if (maxStrength > minStrength)
		{
			//cout << "Max: "<<maxStrength<< " Min: "<<minStrength<<"\n";
			double step = (maxStrength-minStrength)/((double)m_autoThreshNum+1.0);
			double val = minStrength+step;
			for (int i = 0; i<m_autoThreshNum; i++)
			{
				m_autoThresholds.pushBack(val);
				val += step;
			}
		}//if interval
		else m_autoThresholds.pushBack(maxStrength); //Stops computation
	}
}

//computes clustering and translates the computed structure into C
void Clusterer::createClusterGraph(ClusterGraph &C)
{
	if (&(C.getGraph()) != m_pGraph) throw PreconditionViolatedException();
	//clear existing entries
	C.semiClear();

	//we compute the edge strengths
	EdgeArray<double> strengths(*m_pGraph,0.0);
	computeEdgeStrengths(strengths);

	//based on strengths, we compute a clustering
	//Case 1:with depth one (or #thresholds)
	//Case 2: Recursively

	//Case1:
	GraphCopy GC(*m_pGraph);
	edge e;
	ListIterator<double> it = m_thresholds.begin();

	while (it.valid())
	{
		//collect edges that will be deleted
		List<edge> le;
		forall_edges(e, GC)
		{
			if (strengths[e] < *it)
			{
				le.pushFront(e);
			}
		}
		//delete edges
		ListIterator<edge> itle = le.begin();
		while (itle.valid())
		{
			GC.delCopy(*itle);
			itle++;
		}
		//gather cluster information
		node v;
		//vertices within a connected component are always part
		//of the same cluster which then will be parent of the new clusters

		StackPure<node> S;
		NodeArray<bool> done(GC, false);
		forall_nodes(v,GC)
		{
			if (done[v]) continue;
			done[v] = true;
			S.push(v);
			cluster parent = C.clusterOf(GC.original(v));
			SList<node> theCluster;

			while(!S.empty())
			{
				node w = S.pop();
				theCluster.pushFront(GC.original(w));
				edge e;
				forall_adj_edges(e,w)
				{
					node x = e->opposite(w);
					if (!(done[x]))
					{
						done[x] = true;
						S.push(x);
					}
				}
			}//While
			//create the cluster
			C.createCluster(theCluster, parent);

		}

		it++;
	}//while


}

void Clusterer::setClusteringThresholds(const List<double> &threshs)
{
	//we copy the values, should be a low number
	m_thresholds.clear();
	ListConstIterator<double> it = threshs.begin();
	while (it.valid())
	{
		m_thresholds.pushFront((*it));
		it++;
	}
}

}//namespace ogdf
