/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of stress-majorization layout algorithm.
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

#include <ogdf/energybased/StressMajorizationSimple.h>

//For initial layouts
#include <ogdf/energybased/FMMMLayout.h>

#include <ogdf/basic/SList.h>

//only debugging
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf {
const double  StressMajorization::startVal = DBL_MAX - 1.0;
const double  StressMajorization::minVal = DBL_MIN;
const double  StressMajorization::desMinLength = 0.0001;

void  StressMajorization::initialize(
	GraphAttributes& GA,
	const EdgeArray<double>& eLength,
	NodeArray< NodeArray<double> >& oLength,
	NodeArray< NodeArray<double> >& weights,
	double & maxDist,
	bool simpleBFS)
{
	node v;
	const Graph &G = GA.constGraph();

	m_prevEnergy =  startVal;
	m_prevLEnergy =  startVal;

	// all edges straight-line
	GA.clearAllBends();
	if (!m_useLayout)
		shufflePositions(GA);

	//the shortest path lengths
	forall_nodes (v, G) oLength[v].init(G, DBL_MAX);
	forall_nodes (v, G) weights[v].init(G, 0.0);

	//-------------------------------------
	//computes shortest path distances d_ij
	//-------------------------------------
	if (simpleBFS)
	{
		//we use simply BFS n times
		//TODO experimentally compare speed, also with bintree dijkstra
#ifdef OGDF_DEBUG
		double timeUsed;
		usedTime(timeUsed);
#endif
		maxDist = allpairsspBFS(G, oLength, weights);
#ifdef OGDF_DEBUG
		timeUsed = usedTime(timeUsed);
		cout << "\n******APSP BFS runtime: \n";
#endif
	}
	else
	{
		EdgeArray<double> adaptedLength(G);
		adaptLengths(G, GA, eLength, adaptedLength);
		//we use simply the BFM n times or Floyd instead, leading to cubic runtime
		//TODO experimentally compare speed, also with bintree dijkstra
		maxDist = allpairssp(G, adaptedLength, oLength, weights, DBL_MAX);
	}

	if (m_radial)
	{
		//TODO: Also allow different ways to compute centrality (closeness,...)
		// and incorporate node sizes
		//if centrality
		computeRadii(G, oLength, maxDist);
	}
//Todo: Here we could add distance values depending on given node sizes
}//initialize


//uses closeness to compute the radii
void StressMajorization::computeRadii(const Graph& G, const NodeArray< NodeArray<double> >& distances,
	double diameter)
{
	m_radii.init(G, 1.0);
	//compute a center (simple version)
	node v, w;
	double minMax = DBL_MAX;
	node center = 0;
	int numCentralNodes = 0;
	double maxCloseness = 0.0;
	double minCloseness = DBL_MAX;

	// inverse of sum of shortest path distances
	NodeArray<double> closeness(G, 0.0);

	//Compute closeness values and min/max
	forall_nodes(v, G)
	{
		double maxDist = 0.0;
		forall_nodes(w, G)
		{
			if (v != w) closeness[v] += distances[v][w];
			if (distances[v][w] > maxDist) maxDist = distances[v][w];
		}
		if (maxDist < minMax)
		{
			minMax = maxDist;
			center = v;
		}
		closeness[v] = (G.numberOfNodes()-1)/closeness[v]; //was 1/

		//Check for min/max closeness
		if (DIsGreater(closeness[v], maxCloseness))
		{
			maxCloseness = closeness[v];
			numCentralNodes = 1;
		}
		else if (DIsEqual(closeness[v], maxCloseness)) numCentralNodes++;
		if (DIsGreater(minCloseness, closeness[v]))
			minCloseness = closeness[v];
	}

	//see paper by Brandes, Kenis, Wagner
	double coincideOfs = min(0.5, double(numCentralNodes)/double((G.numberOfNodes()-1)));

	OGDF_ASSERT(center != 0);
	forall_nodes(v, G)
	{
		//cout << "Diameter: "<<diameter<<"\n";
		m_radii[v] = diameter/double(2.0) * (1 - (closeness[v]-minCloseness)/(maxCloseness-minCloseness+coincideOfs));
		//cout << "Radius: "<<m_radii[v]<<"\n";
	}

}//computeRadii


void StressMajorization::mainStep(GraphAttributes& GA,
	NodeArray< NodeArray<double> >& oLength,
	NodeArray< NodeArray<double> >& weights,
	const double maxDist)
{
	const Graph &G = GA.constGraph();
	node v;

	//preparation for upward constraints
	edge e;
	NodeArray< NodeArray<double> > upWeight(G);
	forall_nodes(v, G) upWeight[v].init(G, 0.0);
	//try to add upward constraints by looking at the adjacent edges
	forall_edges(e, G)
	{
		upWeight[e->source()][e->target()] = 1.0;
		upWeight[e->target()][e->source()] = -1.0;
	}

#ifdef OGDF_DEBUG
	NodeArray<int> nodeCounts(G, 0);
#endif
	NodeArray<double> invPosNorm(G);
	NodeArray< NodeArray<double> > invDistb(G); //bij in paper Pich Brandes

	//this value is needed quite often
	NodeArray<double> wijSum(G, 0.0); //sum of w_ij for each i over j!= i

	forall_nodes(v, G)
	{
		invDistb[v].init(G);
		node w;
		forall_nodes(w, G)
		{
			if (v != w) wijSum[v]+= weights[v][w];
		}
	}

	//--------------------------------------------------
	// Iterations can either be done until a predefined
	// number m_numSteps+m_itFac*|G| is reached or a break condition
	// is fullfilled
	//--------------------------------------------------
	//for iteration checking
	double weightsum = 1.0;
	if (m_upward) weightsum = 0.93;

	OGDF_ASSERT(DIsGreater(m_numSteps+m_itFac*G.numberOfNodes(), 0.0));
	double kinv = weightsum/double(m_numSteps+m_itFac*G.numberOfNodes());

	NodeArray<double> newX(G);
	NodeArray<double> newY(G);
	//weighting for different optimization criteria
	double t = 0.0;

	for (double tt = 0; tt <= weightsum; tt+= kinv)
	{
		double iterStressSum = 0.0;
		if (m_radial || m_upward) t = tt;
		else t = 0.0;
		//compute inverse norm of position for radial distance constraints
		forall_nodes(v, G)
		{
			double px = GA.x(v);
			double py = GA.y(v);
			//set values a_i as in paper pseudo code (simplified)
			double tmp = px*px+py*py;
			if (DIsGreater(tmp, 0.0))
				invPosNorm[v] = 1/double(sqrt(tmp));
			else invPosNorm[v] = 0.0;
			//cout << "invPosNorm: "<<invPosNorm[v]<<"\n";

			//set values b_ij as in paper pseudo code (simplified)
			node w;
			forall_nodes(w, G)
			{
				double tmpij = (px-GA.x(w))*(px-GA.x(w))+(py-GA.y(w))*(py-GA.y(w));
				invDistb[v][w] = (DIsGreater(tmpij, 0.0) ? 1/double(sqrt(tmpij)) : 0.0);
				//cout << "invdistb: "<<invDistb[v][w]<<"\n";
			}

		}

		forall_nodes(v, G)
		{
			//value corresponding to radial constraint
			double radOfsX = 0.0;
			double radOfsY = 0.0;
			double tmpinvsq = 0.0;

			double upWeightSum = 0.0;

			if(m_upward)
			{
				upWeightSum = 0.05+v->degree()/100.0;
			}

			if(m_radial)
			{
				double tmp = radius(v);
				OGDF_ASSERT(DIsGreater(tmp, 0.0))
					tmpinvsq = 1/(tmp*tmp);
				double tmppart = t*tmpinvsq*tmp*invPosNorm[v];

				radOfsX = tmppart*GA.x(v);
				radOfsY = tmppart*GA.y(v);
			}

			//upward constraints
			double upOfs = 0.0; //only for y coordinate

			//sum over all other nodes
			node w;
			double stressSumX = 0.0;
			double stressSumY = 0.0;
			forall_nodes(w, G)
			{
				if ( v != w )
				{
					stressSumX += weights[v][w]*(GA.x(w)+oLength[v][w]*(GA.x(v)-GA.x(w))*invDistb[v][w]);
					stressSumY += weights[v][w]*(GA.y(w)+oLength[v][w]*(GA.y(v)-GA.y(w))*invDistb[v][w]);
					//also add the influence of adjacent nodes that are not placed correctly
					if (m_upward)
					{
						double val = upWeight[v][w];
						//adjacent nodes influence each other
						if (!DIsEqual(val, 0.0))
						{
							if (DIsGreater(val, 0.0)) //v is source
							{
								if (GA.y(v) > GA.y(w)-1.0)
								{
									upOfs -= val*(GA.y(w)-1.0*invDistb[v][w]);
								}
							}
//							else //v is target
//                       	{
//                        		if (GA.y(w) > GA.y(v)-1.0)
//                        		{
//                        			upOfs -= val*(GA.y(w)+(fabs(GA.y(v)-GA.y(w)))*invDistb[v][w]);
//                        		}
//                        	}
						}

					}
				}
			}
			upOfs *= upWeightSum;
			if (m_radial || m_upward)
			{
				stressSumX *= (1-t);
				stressSumY *= (1-t);
			}

			//main fraction
			newX[v] = (stressSumX+radOfsX)/double(((1-t)*wijSum[v]+t*tmpinvsq));
			//experimental, only valid if disjoint constraints
			newY[v] = (stressSumY+radOfsY+0.2*upOfs)/double(((1-t)*wijSum[v]+t*tmpinvsq+0.2*upWeightSum));
			GA.x(v) = newX[v];
			GA.y(v) = newY[v];
			iterStressSum += stressSumX;
			iterStressSum += stressSumY;
		}
		//Alternatively, we can do the update after the computation step
		//            forall_nodes(v, G)
		//            {
		//GA.x(v) = newX[v];
		//GA.y(v) = newY[v];
		//            }
		if (finished(iterStressSum))
		{
			cout << iterStressSum<<"\n";
			break;
		}
	}//outer loop: iterations or threshold

}//mainStep


void StressMajorization::doCall(GraphAttributes& GA, const EdgeArray<double>& eLength, bool simpleBFS)
{
	const Graph& G = GA.constGraph();
	double maxDist; //maximum distance between nodes
	NodeArray< NodeArray<double> > oLength(G);//first distance, then original length
	NodeArray< NodeArray<double> > weights(G);//standard weights as in MCGee,Kamada/Kawai

	//only for debugging
	OGDF_ASSERT(isConnected(G));

	//compute relevant values
	initialize(GA, eLength, oLength, weights, maxDist, simpleBFS);

	//main loop with node movement
	mainStep(GA, oLength, weights, maxDist);

	if (simpleBFS) scale(GA);
}


void StressMajorization::call(GraphAttributes& GA)
{
	const Graph &G = GA.constGraph();
	if(G.numberOfEdges() < 1)
		return;

	EdgeArray<double> eLength(G);//, 1.0);is not used
	doCall(GA, eLength, true);
}//call


void  StressMajorization::call(GraphAttributes& GA,  const EdgeArray<double>& eLength)
{
	const Graph &G = GA.constGraph();
	if(G.numberOfEdges() < 1)
		return;

	doCall(GA, eLength, false);
}//call with edge lengths


//changes given edge lengths (interpreted as weight factors)
//according to additional parameters like node size etc.
void  StressMajorization::adaptLengths(
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


void  StressMajorization::shufflePositions(GraphAttributes& GA)
{
	//random layout? FMMM? classical MDS? see Paper of Pich and Brandes
	//just hope that we have low sigma values (distance error)
	FMMMLayout fm;
	//fm.call(GA);
}//shufflePositions



/**
 * Initialise the original estimates from nodes and edges.
 */

//we could speed this up by not using nested NodeArrays and
//by not doing the fully symmetrical computation on undirected graphs
//All Pairs Shortest Path Floyd, initializes the whole matrix
//returns maximum distance. Does not detect negative cycles (lead to neg. values on diagonal)
//threshold is the value for the distance of non-adjacent nodes, distance has to be
//initialized with
// The weight parameter here is just for the stress majorization
// and directly set here for speedup
double StressMajorization::allpairssp(
	const Graph& G,
	const EdgeArray<double>& eLengths,
	NodeArray< NodeArray<double> >& distance,
	NodeArray< NodeArray<double> >& weights,
	const double threshold)
{
	node v;
	edge e;
	double maxDist = -threshold;

	forall_nodes(v, G)
	{
		distance[v][v] = 0.0f;
		weights[v][v] = 0.0f;
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
					weights[u][w] = 1/(distance[u][w]*distance[u][w]);
					//distance[w][u] = distance[u][w]; //is done anyway afterwards
				}
				if (distance[u][w] < threshold)
					maxDist = max(maxDist,distance[u][w]);
			}
		}
	}
	//debug output
#ifdef OGDF_DEBUG
	forall_nodes(v, G)
	{
		if (distance[v][v] < 0.0) cerr << "\n###Error in shortest path computation###\n\n";
	}
	cout << "Maxdist: "<<maxDist<<"\n";
	forall_nodes(u, G)
	{
		forall_nodes(w, G)
		{
			cout << "Distance " << u->index() << " -> "<<w->index()<<" "<<distance[u][w]<<"\n";
		}
	}
#endif
	return maxDist;
}//allpairssp


//the same without weights, i.e. all pairs shortest paths with BFS
//Runs in time |V|²
//for compatibility, distances are double
// The weight parameter here is just for the stress majorization
// and directly set here for speedup
double  StressMajorization::allpairsspBFS(
	const Graph& G,
	NodeArray< NodeArray<double> >& distance,
	NodeArray< NodeArray<double> >& weights)
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
					weights[v][u] = 1/(d*d);
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
#ifdef OGDF_DEBUG
	node u, w;
	cout << "Maxdist: "<<maxDist<<"\n";
	forall_nodes(u, G)
	{
		forall_nodes(w, G)
		{
			cout << "Distance " << u->index() << " -> "<<w->index()<<" "<<distance[u][w]<<"\n";
		}
	}
#endif
	return maxDist;
}//allpairsspBFS


void  StressMajorization::scale(GraphAttributes& GA)
{
//Simple version: Just scale to max needed
//We run over all nodes, find the largest distance needed and scale
//the edges uniformly
	node v;
	edge e;
	double maxFac = 0.0;
	forall_edges(e, GA.constGraph())
	{
		double w1 = sqrt(GA.width(e->source())*GA.width(e->source())+
						 GA.height(e->source())*GA.height(e->source()));
		double w2 = sqrt(GA.width(e->target())*GA.width(e->target())+
						 GA.height(e->target())*GA.height(e->target()));
		w2 = (w1+w2)/2.0; //half length of both diagonals
		double xdist = GA.x(e->source())-GA.x(e->target());
		double ydist = GA.y(e->source())-GA.y(e->target());
		double elength = sqrt(xdist*xdist+ydist*ydist);
		w2 = m_distFactor * w2 / elength;//relative to edge length
		if (w2 > maxFac)
			maxFac = w2;
	}

	if (maxFac > 0.0)
	{
		forall_nodes(v, GA.constGraph())
		{
			GA.x(v) = GA.x(v)*maxFac;
			GA.y(v) = GA.y(v)*maxFac;
		}
#ifdef OGDF_DEBUG
		cout << "Scaled by factor "<<maxFac<<"\n";
#endif
	}
}//Scale

}//namespace
