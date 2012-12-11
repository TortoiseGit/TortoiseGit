/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Kamada-Kaway layout algorithm.
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

#include <ogdf/energybased/SpringEmbedderKK.h>
#include <ogdf/basic/SList.h>

//only debugging
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {
const double SpringEmbedderKK::startVal = DBL_MAX - 1.0;
const double SpringEmbedderKK::minVal = DBL_MIN;
const double SpringEmbedderKK::desMinLength = 0.0001;

void SpringEmbedderKK::initialize(
	GraphAttributes& GA,
	NodeArray<dpair>& partialDer,
	const EdgeArray<double>& eLength,
	NodeArray< NodeArray<double> >& oLength,
	NodeArray< NodeArray<double> >& sstrength,
	double & maxDist,
	bool simpleBFS)
{
	node v;
	const Graph &G = GA.constGraph();
	//hier if m_spread vorlayout oder muss der Nutzer das extern erledigen?
	m_prevEnergy =  startVal;
	m_prevLEnergy =  startVal;

	// all edges straight-line
	GA.clearAllBends();
	if (!m_useLayout)
		shufflePositions(GA);

	//the shortest path lengths
	forall_nodes (v, G) oLength[v].init(G, DBL_MAX);

	//-------------------------------------
	//computes shortest path distances d_ij
	//-------------------------------------
	if (simpleBFS)
	{
		//we use simply BFS n times
		//TODO experimentally compare speed, also with bintree dijkstra
//#ifdef OGDF_DEBUG
//		double timeUsed;
//		usedTime(timeUsed);
//#endif
		maxDist = allpairsspBFS(G, oLength);
//#ifdef OGDF_DEBUG
//		timeUsed = usedTime(timeUsed);
//		cout << "\n******APSP BFS runtime: \n";
//#endif
	}
	else
	{
		EdgeArray<double> adaptedLength(G);
		adaptLengths(G, GA, eLength, adaptedLength);
		//we use simply the BFM n times or Floyd instead, leading to cubic runtime
		//TODO experimentally compare speed, also with bintree dijkstra
		maxDist = allpairssp(G, adaptedLength, oLength, DBL_MAX);
	}
	//------------------------------------
	//computes original spring length l_ij
	//------------------------------------

	//first we determine desirable edge length L
	//nodes sizes may be non-uniform, we approximate the display size (grid)
	//this part relies on the fact that node sizes are set != zero
	//TODO check later if this is a good choice
	double L = m_desLength; //desirable length
	double Lzero; //Todo check with m_zeroLength
	if (L < desMinLength)
	{
		double swidth = 0.0f, sheight = 0.0f;

		// Do all nodes lie on the same point? Check by computing BB of centers
		// Then, perform simple shifting in layout
		node vFirst = G.firstNode();
		double minX = GA.x(vFirst), maxX = GA.x(vFirst),
				minY = GA.y(vFirst), maxY = GA.y(vFirst);
		// Two goals:
		//				add node sizes to estimate desirable length
		//				compute BB to check degeneracy
		forall_nodes(v, G)
		{
			swidth += GA.width(v);
			sheight += GA.height(v);

			if(GA.x(v) < minX) minX = GA.x(v);
			if(GA.x(v) > maxX) maxX = GA.x(v);
			if(GA.y(v) < minY) minY = GA.y(v);
			if(GA.y(v) > maxY) maxY = GA.y(v);
		}

		double sroot = maxDist;//sqrt(G.numberOfNodes());
		swidth = swidth / sroot;
		sheight = sheight / sroot;
		Lzero = max(2.0*sroot, 2.0*(swidth + sheight));
		//test for multilevel
		Lzero = max(max(maxX-minX, maxY-minY), 2.0*Lzero);
		//cout << "Lzero: "<<Lzero<<"\n";


		L = Lzero / maxDist;
//#ifdef OGDF_DEBUG
//		cout << "Desirable edge length computed: "<<L<<"\n";
//#endif
	}//set L != 0
	//--------------------------------------------------
	// Having L we can compute the original lengths l_ij
	// Computes spring strengths k_ij
	//--------------------------------------------------
	node w;
	double dij;
	forall_nodes(v, G)
	{
		sstrength[v].init(G);
		forall_nodes(w, G)
		{
			dij = oLength[v][w];
			if (dij == DBL_MAX)
			{
				sstrength[v][w] = minVal;
			}
			else
			{
				oLength[v][w] = L * dij;
				sstrength[v][w] = m_K / (dij * dij);
			}
		}
	}
}//initialize


void SpringEmbedderKK::mainStep(GraphAttributes& GA,
								NodeArray<dpair>& partialDer,
								NodeArray< NodeArray<double> >& oLength,
								NodeArray< NodeArray<double> >& sstrength,
								const double maxDist)
{
	const Graph &G = GA.constGraph();
	node v;

#ifdef OGDF_DEBUG
	NodeArray<int> nodeCounts(G, 0);
	int nodeCount = 0; //number of moved nodes
#endif
	// Now we compute delta_m, we search for the node with max value
	double delta_m = 0.0f;
	double delta_v;
	node best_m = G.firstNode();

	// Compute the partial derivatives first
	forall_nodes (v, G)
	{
		dpair parder = computeParDers(v, GA, sstrength, oLength);
		partialDer[v] = parder;
		//delta_m is sqrt of squares of partial derivatives
		delta_v = sqrt(parder.x1()*parder.x1() + parder.x2()*parder.x2());

		if (delta_v > delta_m)
		{
			best_m = v;
			delta_m = delta_v;
		}
	}

	int globalItCount, localItCount;
	if (m_computeMaxIt)
	{
		globalItCount = m_gItBaseVal+m_gItFactor*G.numberOfNodes();
		localItCount = 2*G.numberOfNodes();
	}
	else
	{
		globalItCount = m_maxGlobalIt;
		localItCount = m_maxLocalIt;
	}


	while (globalItCount-- > 0 && !finished(delta_m))
	{
#ifdef OGDF_DEBUG
//		cout <<"G: "<<globalItCount <<"\n";
//		cout <<"New iteration on "<<best_m->index()<<"\n";
		nodeCount++;
		nodeCounts[best_m]++;
#endif
		// The contribution best_m makes to the partial derivatives of
		// each vertex.
		NodeArray<dpair> p_partials(G);
		forall_nodes(v, G)
		{
			p_partials[v] = computeParDer(v, best_m, GA, sstrength, oLength);
		}

		localItCount = 0;
		do {
#ifdef OGDF_DEBUG
//			cout <<"  New local iteration\n";
//			cout <<"   L: "<<localItCount <<"\n";
#endif
			// Compute the 4 elements of the Jacobian
			double dE_dx_dx = 0.0f, dE_dx_dy = 0.0f, dE_dy_dx = 0.0f, dE_dy_dy = 0.0f;
			forall_nodes(v, G)
			{
				if (v != best_m) {
					double x_diff = GA.x(best_m) - GA.x(v);
					double y_diff = GA.y(best_m) - GA.y(v);
					double dist = sqrt(x_diff * x_diff + y_diff * y_diff);
					double dist3 = dist * dist * dist;
					OGDF_ASSERT(dist3 != 0.0);
					double k_mi = sstrength[best_m][v];
					double l_mi = oLength[best_m][v];
					dE_dx_dx += k_mi * (1 - (l_mi * y_diff * y_diff)/dist3);
					dE_dx_dy += k_mi * l_mi * x_diff * y_diff / dist3;
					dE_dy_dx += k_mi * l_mi * x_diff * y_diff / dist3;
					dE_dy_dy += k_mi * (1 - (l_mi * x_diff * x_diff)/dist3);
				}
			}

			// Solve for delta_x and delta_y
			double dE_dx = partialDer[best_m].x1();
			double dE_dy = partialDer[best_m].x2();

			double delta_x =
				(dE_dx_dy * dE_dy - dE_dy_dy * dE_dx)
				/ (dE_dx_dx * dE_dy_dy - dE_dx_dy * dE_dy_dx);

			double delta_y =
				(dE_dx_dx * dE_dy - dE_dy_dx * dE_dx)
				/ (dE_dy_dx * dE_dx_dy - dE_dx_dx * dE_dy_dy);


			// Move p by (delta_x, delta_y)
#ifdef OGDF_DEBUG
			// cout <<"   x"<< delta_x<<"\n";
			// cout <<"   y" <<delta_y<<"\n";
#endif
			GA.x(best_m) += delta_x;
			GA.y(best_m) += delta_y;

			// Recompute partial derivatives and delta_p
			dpair deriv = computeParDers(best_m, GA, sstrength, oLength);
			partialDer[best_m] = deriv;

			delta_m =
				sqrt(deriv.x1()*deriv.x1() + deriv.x2()*deriv.x2());
		} while (localItCount-- > 0 && !finishedNode(delta_m));

		// Select new best_m by updating each partial derivative and delta
		node old_p = best_m;
		forall_nodes(v, G)
		{
			dpair old_deriv_p = p_partials[v];
			dpair old_p_partial =
				computeParDer(v, old_p, GA, sstrength, oLength);
			dpair deriv = partialDer[v];

			deriv.x1() += old_p_partial.x1() - old_deriv_p.x1();
			deriv.x2() += old_p_partial.x2() - old_deriv_p.x2();

			partialDer[v] = deriv;
			double delta = sqrt(deriv.x1()*deriv.x1() + deriv.x2()*deriv.x2());

			if (delta > delta_m) {
				best_m = v;
				delta_m = delta;
			}
		}
	}//while
#ifdef OGDF_DEBUG
//  cout << "NodeCount: "<<nodeCount<<"\n";
//  forall_nodes(v, G)
//  {
//   	cout<<"Counts "<<v->index()<<": "<<nodeCounts[v]<<"\n";
//  }
#endif
}//mainStep


void SpringEmbedderKK::doCall(GraphAttributes& GA, const EdgeArray<double>& eLength, bool simpleBFS)
{
	const Graph& G = GA.constGraph();
	NodeArray<dpair> partialDer(G); //stores the partial derivative per node
	double maxDist; //maximum distance between nodes
	NodeArray< NodeArray<double> > oLength(G);//first distance, then original length
	NodeArray< NodeArray<double> > sstrength(G);//the spring strength

	//only for debugging
	OGDF_ASSERT(isConnected(G));

	//compute relevant values
	initialize(GA, partialDer, eLength, oLength, sstrength, maxDist, simpleBFS);

	//main loop with node movement
	mainStep(GA, partialDer, oLength, sstrength, maxDist);

	if (simpleBFS) scale(GA);
}


void SpringEmbedderKK::call(GraphAttributes& GA)
{
	const Graph &G = GA.constGraph();
	if(G.numberOfEdges() < 1)
		return;

	EdgeArray<double> eLength(G);//, 1.0);is not used
	doCall(GA, eLength, true);
}//call

void SpringEmbedderKK::call(GraphAttributes& GA,  const EdgeArray<double>& eLength)
{
	const Graph &G = GA.constGraph();
	if(G.numberOfEdges() < 1)
		return;

	doCall(GA, eLength, false);
}//call with edge lengths


//changes given edge lengths (interpreted as weight factors)
//according to additional parameters like node size etc.
void SpringEmbedderKK::adaptLengths(
	const Graph& G,
	const GraphAttributes& GA,
	const EdgeArray<double>& eLengths,
	EdgeArray<double>& adaptedLengths)
{
	//we use the edge lengths as factor and try to respect
	//the node sizes such that each node has enough distance
	edge e;
	//adapt to node sizes
	forall_edges(e, G)
	{
		double smax = max(GA.width(e->source()), GA.height(e->source()));
		double tmax = max(GA.width(e->target()), GA.height(e->target()));
		if (smax+tmax > 0.0)
			adaptedLengths[e] = (1+eLengths[e])*((smax+tmax));///2.0);
		else adaptedLengths[e] = 5.0*eLengths[e];
	}
}//adaptLengths

void SpringEmbedderKK::shufflePositions(GraphAttributes& GA)
{
//first check if degenerated or
//just position all on a circle or random layout?
}//shufflePositions



// Compute contribution of vertex u to the first partial
// derivatives (dE/dx_m, dE/dy_m) (for vertex m) (eq. 7 and 8 in paper)
SpringEmbedderKK::dpair SpringEmbedderKK::computeParDer(
	node m,
	node u,
	GraphAttributes& GA,
	NodeArray< NodeArray<double> >& ss,
	NodeArray< NodeArray<double> >& dist)
{
	dpair result(0.0, 0.0);
	if (m != u)
	{
		double x_diff = GA.x(m) - GA.x(u);
		double y_diff = GA.y(m) - GA.y(u);
		double distance = sqrt(x_diff * x_diff + y_diff * y_diff);
		result.x1() = (ss[m][u]) * (x_diff - (dist[m][u])*x_diff/distance);
		result.x2() = (ss[m][u]) * (y_diff - (dist[m][u])*y_diff/distance);
	}

	return result;
}


//compute partial derivative for v
SpringEmbedderKK::dpair SpringEmbedderKK::computeParDers(node v,
	GraphAttributes& GA,
	NodeArray< NodeArray<double> >& ss,
	NodeArray< NodeArray<double> >& dist)
{
	node u;
	dpair result(0.0, 0.0);
	forall_nodes(u, GA.constGraph())
	{
		dpair deriv = computeParDer(v, u, GA, ss, dist);
		result.x1() += deriv.x1();
		result.x2() += deriv.x2();
	}

	return result;
}


/**
 * Initialise the original estimates from nodes and edges.
 */

//we could speed this up by not using nested NodeArrays and
//by not doing the fully symmetrical computation on undirected graphs
//All Pairs Shortest Path Floyd, initializes the whole matrix
//returns maximum distance. Does not detect negative cycles (lead to neg. values on diagonal)
//threshold is the value for the distance of non-adjacent nodes, distance has to be
//initialized with
double SpringEmbedderKK::allpairssp(const Graph& G, const EdgeArray<double>& eLengths, NodeArray< NodeArray<double> >& distance,
	const double threshold)
{
	node v;
	edge e;
	double maxDist = -threshold;

	forall_nodes(v, G)
	{
		distance[v][v] = 0.0f;
	}

	//TODO: Experimentally compare this against
	// all nodes and incident edges (memory access) on huge graphs
	forall_edges(e, G)
	{
		distance[e->source()][e->target()] = eLengths[e];
		distance[e->target()][e->source()] = eLengths[e];
	}

///**
// * And run the main loop of the algorithm.
// */
	node u, w;
	forall_nodes(v, G)
	{
		forall_nodes(u, G)
		{
			forall_nodes(w, G)
			{
				if ((distance[u][v] < threshold) && (distance[v][w] < threshold))
				{
					distance[u][w] = min( distance[u][w], distance[u][v] + distance[v][w] );
					//distance[w][u] = distance[u][w]; //is done anyway afterwards
				}
				if (distance[u][w] < threshold)
					maxDist = max(maxDist,distance[u][w]);
			}
		}
	}
	//debug output
//#ifdef OGDF_DEBUG
//	forall_nodes(v, G)
//	{
//		if (distance[v][v] < 0.0) cerr << "\n###Error in shortest path computation###\n\n";
//	}
//	cout << "Maxdist: "<<maxDist<<"\n";
//	forall_nodes(u, G)
//	{
//	forall_nodes(w, G)
//	{
////		cout << "Distance " << u->index() << " -> "<<w->index()<<" "<<distance[u][w]<<"\n";
//	}
//	}
//#endif
	return maxDist;
}//allpairssp


//the same without weights, i.e. all pairs shortest paths with BFS
//Runs in time |V|²
//for compatibility, distances are double
double SpringEmbedderKK::allpairsspBFS(const Graph& G, NodeArray< NodeArray<double> >& distance)
{
	node v;
	double maxDist = 0;

	forall_nodes(v, G)
	{
		distance[v][v] = 0.0f;
	}

	v = G.firstNode();

	//start in each node once
	while (v != 0)
	{
		//do a bfs
		NodeArray<bool> mark(G, true);
		SListPure<node> bfs;
		bfs.pushBack(v);
		mark[v] = false;

		while (!bfs.empty())
		{
			node w = bfs.popFrontRet();
			edge e;
			double d = distance[v][w]+1.0f;
			forall_adj_edges(e,w)
			{
				node u = e->opposite(w);
				if (mark[u])
				{
					mark[u] = false;
					bfs.pushBack(u);
					distance[v][u] = d;
					maxDist = max(maxDist,d);
				}
			}
		}//while

		v = v->succ();
	}//while
	//check for negative cycles
	forall_nodes(v, G)
	{
		if (distance[v][v] < 0.0) cerr << "\n###Error in shortest path computation###\n\n";
	}

//debug output
//#ifdef OGDF_DEBUG
//	node u, w;
//	cout << "Maxdist: "<<maxDist<<"\n";
//	forall_nodes(u, G)
//	{
//	forall_nodes(w, G)
//	{
////		cout << "Distance " << u->index() << " -> "<<w->index()<<" "<<distance[u][w]<<"\n";
//	}
//	}
//#endif
	return maxDist;
}//allpairsspBFS


void SpringEmbedderKK::scale(GraphAttributes& GA)
{
//Simple version: Just scale to max needed
//We run over all edges, find the largest distance needed and scale
//the edges uniformly
	node v;
	edge e;
	double maxFac = 0.0;
	bool scale = true;
	forall_edges(e, GA.constGraph())
	{
		double w1 = sqrt(GA.width(e->source())*GA.width(e->source())+
						 GA.height(e->source())*GA.height(e->source()));
		double w2 = sqrt(GA.width(e->target())*GA.width(e->target())+
						 GA.height(e->target())*GA.height(e->target()));
		w2 = (w1+w2)/2.0; //half length of both diagonals
		double xs = GA.x(e->source());
		double xt = GA.x(e->target());
		double ys = GA.y(e->source());
		double yt = GA.y(e->target());
		double xdist = xs-xt;
		double ydist = ys-yt;
		if ((fabs(xs) > (DBL_MAX / 2.0)-1) || (fabs(xt)> (DBL_MAX/2.0)-1) ||
			(fabs(ys)> (DBL_MAX/2.0)-1) || (fabs(yt)> (DBL_MAX/2.0)-1))
			scale = false; //never scale with huge numbers
		//(even though the drawing may be small and could be shifted to origin)
		double elength = sqrt(xdist*xdist+ydist*ydist);

		//Avoid a max factor of inf!!
		if (DIsGreater(elength, 0.0001))
		{
			w2 = m_distFactor * w2 / elength;//relative to edge length

			if (w2 > maxFac)
				maxFac = w2;
		}
	}


	if (maxFac > 1.0 && (maxFac < (DBL_MAX/2.0)-1) && scale) //only scale to increase distance
	{
		//if maxFac is large, we scale in steps until we reach a threshold
		if (maxFac > 2048)
		{
			double scaleF = maxFac+0.00001;
			double base = 2.0;
			maxFac = base;

			while (scale && maxFac<scaleF)
			{
			forall_nodes(v, GA.constGraph())
			{
				GA.x(v) = GA.x(v)*base;
				GA.y(v) = GA.y(v)*base;
				if (GA.x(v) > (DBL_MAX / base)-1 || GA.y(v) > (DBL_MAX / base) - 1)
					scale = false;
			}
			maxFac *= base;
			}
		}
		else
		{
			forall_nodes(v, GA.constGraph())
			{
				GA.x(v) = GA.x(v)*maxFac;
				GA.y(v) = GA.y(v)*maxFac;
			}
		}
//#ifdef OGDF_DEBUG
//		cout << "Scaled by factor "<<maxFac<<"\n";
//#endif
	}
}//Scale

}//namespace
