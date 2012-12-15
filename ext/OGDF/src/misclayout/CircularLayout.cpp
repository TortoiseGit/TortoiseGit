/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class CircularLayout
 *
 * \author Carsten Gutwenger
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


//#define OUTPUT

#include <ogdf/misclayout/CircularLayout.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/GraphCopyAttributes.h>
#include <ogdf/basic/Math.h>
#include <ogdf/packing/TileToRowsCCPacker.h>



namespace ogdf {

double angleNormalize(double alpha)
{
	while(alpha < 0)
		alpha += 2*Math::pi;

	while(alpha >= 2*Math::pi)
		alpha -= 2*Math::pi;

	return alpha;
}

bool angleSmaller(double alpha, double beta)
{
	double alphaNorm = angleNormalize(alpha);
	double betaNorm  = angleNormalize(beta);

	double start = betaNorm-Math::pi;

	if(start >= 0) {
		return start < alphaNorm && alphaNorm < betaNorm;
	} else {
		return alphaNorm < betaNorm || alphaNorm >= start+2*Math::pi;
	}
}

double angleDistance(double alpha, double beta)
{
	double alphaNorm = angleNormalize(alpha);
	double betaNorm  = angleNormalize(beta);

	double dist = alphaNorm - betaNorm;
	if (dist < 0) dist += 2*Math::pi;

	return (dist <= Math::pi) ? dist : 2*Math::pi-dist;
}


void angleRangeAdapt(double sectorStart, double sectorEnd, double &start, double &length)
{
	double start1 = angleNormalize(sectorStart);
	double end1   = angleNormalize(sectorEnd);
	double start2 = angleNormalize(start);
	double end2   = angleNormalize(start+length);

	if(end1   < start1) end1   += 2*Math::pi;
	if(start2 < start1) start2 += 2*Math::pi;
	if(end2   < start1) end2   += 2*Math::pi;

	if(start2 > end1) start = start1;
	if(end2   > end1) start = angleNormalize(sectorEnd - length);
}

struct ClusterStructure
{
	ClusterStructure(const Graph &G) : m_G(G), m_clusterOf(G) { }

	operator const Graph &() const { return m_G; }

	void initCluster(int nCluster, const Array<int> &parent);

	void sortChildren(int i,
		List<node> &nodes,
		Array<List<int> > &posList,
		Array<double> &parentWeight,
		Array<double> &dirFromParent,
		List<Tuple2<int,double> > &mainSiteWeights);

	int numberOfCluster() const { return m_nodesIn.size(); }

	void resetNodes(int cluster, const List<node> &nodes);

	const Graph &m_G;
	Array<SList<node> > m_nodesIn;
	NodeArray<int>      m_clusterOf;
	List<int>           m_mainSiteCluster;

	Array<int>        m_parentCluster;
	Array<List<int> > m_childCluster;

	// undefined methods to avoid automatic creation
	ClusterStructure(const ClusterStructure &);
	ClusterStructure &operator=(const ClusterStructure &);
};


void ClusterStructure::resetNodes(int cluster, const List<node> &nodes)
{
	OGDF_ASSERT(m_nodesIn[cluster].size() == nodes.size());

	SList<node> &L = m_nodesIn[cluster];

	L.clear();

	ListConstIterator<node> it;
	for(it = nodes.begin(); it.valid(); ++it)
		L.pushBack(*it);
}


void ClusterStructure::initCluster(int nCluster, const Array<int> &parent)
{
	m_nodesIn      .init(nCluster);
	m_parentCluster.init(nCluster);
	m_childCluster .init(nCluster);

	node v;
	forall_nodes(v,m_G)
		m_nodesIn[m_clusterOf[v]].pushBack(v);

	int i;
	for(i = 0; i < nCluster; ++i) {
		m_parentCluster[i] = parent[i];
		if(parent[i] != -1)
			m_childCluster[parent[i]].pushBack(i);
	}
}


class WeightComparer
{
	typedef Tuple2<int,double> TheWeight;
	static int compare(const TheWeight &x, const TheWeight &y)
	{
		if (x.x2() < y.x2()) return -1;
		else if (x.x2() > y.x2()) return 1;
		else return 0;
	}
	OGDF_AUGMENT_STATICCOMPARER(TheWeight)
};

void ClusterStructure::sortChildren(
	int i,
	List<node> &nodes,
	Array<List<int> > &posList,
	Array<double> &parentWeight,
	Array<double> &dirFromParent,
	List<Tuple2<int,double> > &mainSiteWeights)
{
	const int n = nodes.size();
	const int parent = m_parentCluster[i];

	if (parent != -1)
		posList[parent].clear();

	int pos = 0;
	ListConstIterator<node> it;
	for(it = nodes.begin(); it.valid(); ++it)
	{
		edge e;
		forall_adj_edges(e,*it) {
			node w = e->opposite(*it);
			if (m_clusterOf[w] != i)
				posList[m_clusterOf[w]].pushBack(pos);
		}
		pos++;
	}

	List<Tuple2<int,double> > weights;

	// build list of all adjacent clusters (children + parent)
	List<int> adjClusters = m_childCluster[i];
	if (parent != -1)
		adjClusters.pushBack(parent);

	ListConstIterator<int> itC;
	for(itC = adjClusters.begin(); itC.valid(); ++itC)
	{
		int child = *itC;

		int size = posList[child].size();
		OGDF_ASSERT(size >= 1);
		if(size == 1) {
			weights.pushBack(Tuple2<int,double>(child,posList[child].front()));

		} else {
			// Achtung: Dieser Teil ist noch nicht richtig ausgetestet,da
			// bei Bic.comp. immer nur ein Knoten benachbart ist
			const List<int> &L = posList[child];
			int gapEnd    = L.front();
			int gapLength = L.front() - L.back() + n;

			int posPred = L.front();
			ListConstIterator<int> it;
			for(it = L.begin().succ(); it.valid(); ++it) {
				if (*it - posPred > gapLength) {
					gapEnd    = *it;
					gapLength = *it - posPred;
				}
				posPred = *it;
			}

			int x = (n - gapEnd) % n;

			int sum = 0;
			for(it = L.begin(); it.valid(); ++it)
				sum += ((*it + x) % n);

			double w = double(sum)/double(size);

			w -= x;
			if(w < 0) w += n;

			weights.pushBack(Tuple2<int,double>(child,w));
		}
	}

	WeightComparer weightComparer;
	weights.quicksort(weightComparer);
#ifdef OUTPUT
	cout << "weights after: " << weights << endl;
#endif

	m_childCluster[i].clear();
	ListConstIterator<Tuple2<int,double> > itWeights;

	if(parent != -1) {
		// find list element containing parent cluster
		for(itWeights = weights.begin();
			(*itWeights).x1() != parent;
			itWeights = weights.cyclicSucc(itWeights)) { }

		parentWeight[i] = (*itWeights).x2();
		for(itWeights = weights.cyclicSucc(itWeights);
			(*itWeights).x1() != parent;
			itWeights = weights.cyclicSucc(itWeights))
		{
			m_childCluster[i].pushBack((*itWeights).x1());

			if(m_nodesIn[i].size() == 1)
				dirFromParent[(*itWeights).x1()] = Math::pi;
			else {
				double x = (*itWeights).x2() - parentWeight[i];
				if(x < 0) x += n;
				dirFromParent[(*itWeights).x1()] = x/n*2*Math::pi;
			}
		}

	} else {
		parentWeight[i] = 0;
		for(itWeights = weights.begin(); itWeights.valid(); ++itWeights)
		{
			m_childCluster[i].pushBack((*itWeights).x1());

			// not yet determined!
			dirFromParent[(*itWeights).x1()] = -1;
		}
		mainSiteWeights = weights;
	}
}


struct InfoAC
{
	node m_vBC, m_predCutBC, m_predCut;
	int m_parentCluster;

	InfoAC(node vBC, node predCutBC, node predCut, int parentCluster) {
		m_vBC = vBC;
		m_predCutBC = predCutBC;
		m_predCut = predCut;
		m_parentCluster = parentCluster;
	}
};


class CircleGraph : public Graph
{
public:
	CircleGraph(const ClusterStructure &C, NodeArray<node> &toCircle, int c);

	void order(List<node> &nodes);
	void swapping(List<node> &nodes, int maxIterations);

	node fromCircle(node vCircle) const { return m_fromCircle[vCircle]; }

private:
	NodeArray<node> m_fromCircle;

	void dfs(
		NodeArray<int>  &depth,
		NodeArray<node> &father,
		node v,
		node f,
		int d);
};


CircleGraph::CircleGraph(
	const ClusterStructure &C,
	NodeArray<node> &toCircle,
	int c)
{
	m_fromCircle.init(*this);

	SListConstIterator<node> it;
	for(it = C.m_nodesIn[c].begin(); it.valid(); ++it)
	{
		node vCircle = newNode();
		toCircle    [*it]     = vCircle;
		m_fromCircle[vCircle] = *it;
	}

	for(it = C.m_nodesIn[c].begin(); it.valid(); ++it)
	{
		edge e;
		forall_adj_edges(e,*it) {
			node w = e->target();
			if (w == *it) continue;

			if(C.m_clusterOf[w] == c)
				newEdge(toCircle[*it],toCircle[w]);
		}
	}
}


class DepthBucket : public BucketFunc<node>
{
public:
	DepthBucket(const NodeArray<int> &depth) : m_depth(depth) { }

	int getBucket(const node &v)
	{
		return -m_depth[v];
	}

	// undefined methods to avoid automatic creation
	DepthBucket(const DepthBucket &);
	DepthBucket &operator=(const DepthBucket &);

private:
	const NodeArray<int> &m_depth;
};


// Idee: Benutzung von outerplanarity (nachschlagen!)
void CircleGraph::order(List<node> &nodes)
{
	NodeArray<int>  depth  (*this,0);
	NodeArray<node> father (*this);

	dfs(depth, father, firstNode(), 0, 1);

	SListPure<node> circleNodes;
	allNodes(circleNodes);

	DepthBucket bucket(depth);
	circleNodes.bucketSort(-numberOfNodes(),0,bucket);

	NodeArray<bool> visited(*this,false);

	ListIterator<node> itCombined;
	bool combinedAtRoot = false;

	SListConstIterator<node> it;
	for(it = circleNodes.begin(); it.valid(); ++it)
	{
		node v = *it;
		List<node> currentPath;

		ListIterator<node> itInserted;
		while(v != 0 && !visited[v])
		{
			visited[v] = true;
			itInserted = currentPath.pushBack(v);
			v = father[v];
		}

		if(v && father[v] == 0 && !combinedAtRoot) {
			combinedAtRoot = true;

			while(!currentPath.empty())
				currentPath.moveToSucc(currentPath.begin(),nodes,itCombined);

		} else {
			if (v == 0)
				itCombined = itInserted;

			nodes.conc(currentPath);
		}
	}
}

void CircleGraph::dfs(
	NodeArray<int>  &depth,
	NodeArray<node> &father,
	node v,
	node f,
	int d)
{
	if (depth[v] != 0)
		return;

	depth [v] = d;
	father[v] = f;

	edge e;
	forall_adj_edges(e,v) {
		node w = e->opposite(v);
		if(w == f) continue;

		dfs(depth,father,w,v,d+1);
	}
}


void CircleGraph::swapping(List<node> &nodes, int maxIterations)
{
	ListIterator<node> it;

	if (nodes.size() >= 3)
	{
		NodeArray<int> pos(*this);
		const int n = numberOfNodes();

		int currentPos = 0;
		for(it = nodes.begin(); it.valid(); ++it)
			pos[*it] = currentPos++;

		int iterations = 0;
		bool improvement;
		do {
			improvement = false;

			for(it = nodes.begin(); it.valid(); ++it)
			{
				ListIterator<node> itNext = nodes.cyclicSucc(it);

				node u = *it, v = *itNext;
				// we fake a numbering around the circle starting with u at pos. 0
				// using the formula: (pos[t]-offset) % n
				// and pos[u] + offset = n
				int offset = n - pos[u];

				// we count how many crossings we save when swapping u and v
				int improvementCrosings = 0;

				edge ux;
				forall_adj_edges(ux,u) {
					node x = ux->opposite(u);
					if(x == v) continue;

					int posX = (pos[x] + offset) % n;

					edge vy;
					forall_adj_edges(vy,v) {
						node y = vy->opposite(v);
						if (y == u || y == x) continue;

						int posY = (pos[y] + offset) % n;

						if(posX > posY)
							--improvementCrosings;
						else
							++improvementCrosings;
					}
				}

				if(improvementCrosings > 0) {
					improvement = true;
					swap(*it,*itNext);
					swap(pos[u],pos[v]);
				}
			}
		} while(improvement && ++iterations <= maxIterations);
	}

	for(it = nodes.begin(); it.valid(); ++it)
		*it = m_fromCircle[*it];
}



//---------------------------------------------------------
// Constructor
//---------------------------------------------------------
CircularLayout::CircularLayout()
{
	// set options to defaults
	m_minDistCircle  = 20.0;
	m_minDistLevel   = 20.0;
	m_minDistSibling = 10.0;
	m_minDistCC      = 20.0;
	m_pageRatio      = 1.0;
}


//---------------------------------------------------------
// default call
// uses biconnected components as clusters
//---------------------------------------------------------
void CircularLayout::call(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();
	if(G.empty())
		return;

	// all edges straight-line
	AG.clearAllBends();

	GraphCopy GC;
	GC.createEmpty(G);

	// compute connected component of G
	NodeArray<int> component(G);
	int numCC = connectedComponents(G,component);

	// intialize the array of lists of nodes contained in a CC
	Array<List<node> > nodesInCC(numCC);

	node v;
	forall_nodes(v,G)
		nodesInCC[component[v]].pushBack(v);

	EdgeArray<edge> auxCopy(G);
	Array<DPoint> boundingBox(numCC);

	int i;
	for(i = 0; i < numCC; ++i)
	{
		GC.initByNodes(nodesInCC[i],auxCopy);

		GraphCopyAttributes AGC(GC,AG);

		if(GC.numberOfNodes() == 1)
		{
			node v1 = GC.firstNode();
			AGC.x(v1) = AGC.y(v1) = 0;

		} else {
			// partition nodes into clusters
			// default uses biconnected components as cluster
			ClusterStructure C(GC);
			assignClustersByBiconnectedComponents(C);

			// call the actual layout algorithm with predefined clusters
			doCall(AGC,C);
		}

		node vFirst = GC.firstNode();
		double minX = AGC.x(vFirst), maxX = AGC.x(vFirst),
			minY = AGC.y(vFirst), maxY = AGC.y(vFirst);

		node vCopy;
		forall_nodes(vCopy,GC) {
			node v = GC.original(vCopy);
			AG.x(v) = AGC.x(vCopy);
			AG.y(v) = AGC.y(vCopy);

			if(AG.x(v)-AG.width (v)/2 < minX) minX = AG.x(v)-AG.width(v) /2;
			if(AG.x(v)+AG.width (v)/2 > maxX) maxX = AG.x(v)+AG.width(v) /2;
			if(AG.y(v)-AG.height(v)/2 < minY) minY = AG.y(v)-AG.height(v)/2;
			if(AG.y(v)+AG.height(v)/2 > maxY) maxY = AG.y(v)+AG.height(v)/2;
		}

		minX -= m_minDistCC;
		minY -= m_minDistCC;

		forall_nodes(vCopy,GC) {
			node v = GC.original(vCopy);
			AG.x(v) -= minX;
			AG.y(v) -= minY;
		}

		boundingBox[i] = DPoint(maxX - minX, maxY - minY);
	}

	Array<DPoint> offset(numCC);
	TileToRowsCCPacker packer;
	packer.call(boundingBox,offset,m_pageRatio);

	// The arrangement is given by offset to the origin of the coordinate
	// system. We still have to shift each node and edge by the offset
	// of its connected component.

	for(i = 0; i < numCC; ++i)
	{
		const List<node> &nodes = nodesInCC[i];

		const double dx = offset[i].m_x;
		const double dy = offset[i].m_y;

		// iterate over all nodes in ith CC
		ListConstIterator<node> it;
		for(it = nodes.begin(); it.valid(); ++it)
		{
			node v = *it;

			AG.x(v) += dx;
			AG.y(v) += dy;
		}
	}
}


struct QueuedCirclePosition
{
	int m_cluster;
	double m_minDist;
	double m_sectorStart, m_sectorEnd;

	QueuedCirclePosition(int cluster,
		double minDist,
		double sectorStart,
		double sectorEnd)
	{
		m_cluster = cluster;
		m_minDist = minDist;
		m_sectorStart = sectorStart;
		m_sectorEnd = sectorEnd;
	}
};

struct ClusterRegion
{
	ClusterRegion(int c, double start, double length, double scaleFactor = 1.0)
	{
		m_start       = start;
		m_length      = length;
		m_scaleFactor = scaleFactor;
		m_clusters.pushBack(c);
	}

	double m_start, m_length, m_scaleFactor;
	SList<int> m_clusters;
};


struct SuperCluster
{
	SuperCluster(SList<int> &cluster,
		double direction,
		double length,
		double scaleFactor = 1.0)
	{
		m_direction   = direction;
		m_length      = length;
		m_scaleFactor = scaleFactor;
		m_cluster.conc(cluster);  // cluster is emtpy afterwards!
	}

	double m_direction, m_length, m_scaleFactor;
	SList<int> m_cluster;
};


typedef SuperCluster *PtrSuperCluster;
ostream &operator<<(ostream &os, const PtrSuperCluster &sc)
{
	const double fac = 180 / Math::pi;

	os << "{" << fac*sc->m_direction << "," << fac*sc->m_length << "," << sc->m_scaleFactor << ":" << sc->m_cluster << "}";
	return os;
}


struct SCRegion
{
	SCRegion(SuperCluster &sc) {
		m_length = sc.m_scaleFactor*sc.m_length;
		m_start  = angleNormalize(sc.m_direction - m_length/2);
		m_superClusters.pushBack(&sc);
	}

	double m_start, m_length;
	SList<SuperCluster*> m_superClusters;
};

void outputRegions(List<SCRegion> &regions)
{
	const double fac = 180 / Math::pi;

	cout << "regions:\n";
	ListIterator<SCRegion> it;
	for(it = regions.begin(); it.valid(); ++it)
	{
		cout << "[" << (*it).m_superClusters << ", " <<
			fac*(*it).m_start << ", " << fac*(*it).m_length << "]" << endl;
	}
}


//---------------------------------------------------------
// call for predefined clusters
// performs the actual layout algorithm
//---------------------------------------------------------
void CircularLayout::doCall(GraphCopyAttributes &AG, ClusterStructure &C)
{
	// we consider currently only the case that we have a single main-site cluster
	OGDF_ASSERT(C.m_mainSiteCluster.size() == 1);

	// compute radii of clusters
	const int nCluster = C.numberOfCluster();
	Array<double> radius     (nCluster);
	Array<double> outerRadius(nCluster);

#ifdef OUTPUT
	const double fac = 360/(2*Math::pi);
#endif

	int i;
	for(i = 0; i < nCluster; ++i)
	{
		const int n = C.m_nodesIn[i].size();

		double sumDiameters = 0, maxR = 0;
		SListConstIterator<node> it;
		for(it = C.m_nodesIn[i].begin(); it.valid(); ++it) {
			double d = sqrt(
				AG.getWidth(*it) * AG.getWidth(*it) + AG.getHeight(*it) * AG.getHeight(*it));
			sumDiameters += d;
			if (d/2 > maxR) maxR = d/2;
		}

		if(n == 1) {
			radius     [i] = 0;
			outerRadius[i] = maxR;

		} else if (n == 2) {
			radius     [i] = 0.5*m_minDistCircle + sumDiameters / 4;
			outerRadius[i] = 0.5*m_minDistCircle + sumDiameters / 2;

		} else {
			radius     [i] = (n*m_minDistCircle + sumDiameters) / (2*Math::pi);
			outerRadius[i] = radius[i] + maxR;
		}

#ifdef OUTPUT
		cout << "radius of       " << i << " = " << radius[i] << endl;
		cout << "outer radius of " << i << " = " << outerRadius[i] << endl;
#endif
	}


	int mainSite = C.m_mainSiteCluster.front();

	NodeArray<node> toCircle(C);

	Queue<int> queue;
	queue.append(mainSite);

	Array<List<int> > posList      (nCluster);
	Array<double>     parentWeight (nCluster);
	Array<double>     dirFromParent(nCluster);
	List<Tuple2<int,double> > mainSiteWeights;

	while(!queue.empty())
	{
		int cluster = queue.pop();

		CircleGraph GC(C, toCircle, cluster);

		// order nodes on circle
		List<node> nodes;
		GC.order(nodes);
		GC.swapping(nodes,50);
		C.resetNodes(cluster, nodes);
#ifdef OUTPUT
		cout << "after swapping of " << cluster << ": " << nodes << endl;
#endif

		C.sortChildren(cluster,nodes,posList,parentWeight,dirFromParent,mainSiteWeights);
#ifdef OUTPUT
		cout << "child cluster of " << cluster << ": " << C.m_childCluster[cluster] << endl;
#endif

		// append all children of cluster to queue
		ListConstIterator<int> itC;
		for(itC = C.m_childCluster[cluster].begin(); itC.valid(); ++itC)
			queue.append(*itC);
	}

	// compute positions of circles
	Array<double> preferedAngle(nCluster);
	Array<double> preferedDirection(nCluster);
	computePreferedAngles(C,outerRadius,preferedAngle);

	Array<double> circleDistance(nCluster);
	Array<double> circleAngle(nCluster);

	circleDistance[mainSite] = 0;
	circleAngle[mainSite] = 0;

	Queue<QueuedCirclePosition> circleQueue;
	// sectors are assigned to children weighted by the prefered angles
	double sumPrefAngles = 0;
	double sumChildrenLength = 0;
	ListConstIterator<int> itC;
	for(itC = C.m_childCluster[mainSite].begin(); itC.valid(); ++itC) {
		sumPrefAngles += preferedAngle[*itC];
		sumChildrenLength += 2*outerRadius[*itC]+m_minDistSibling;
	}

	// estimation for distance of child cluster
	double rFromMainSite = max(m_minDistLevel+outerRadius[mainSite],
		sumChildrenLength / (2 * Math::pi));
	// estiamtion for maximal allowed angle (which is 2*maxHalfAngle)
	double maxHalfAngle = acos(outerRadius[mainSite] / rFromMainSite);

	// assignment of angles around main-site with pendulum method -------

	// initialisation
	double minDist   = outerRadius[mainSite] + m_minDistLevel;
	List<SuperCluster>  superClusters;
	List<SCRegion>      regions;
	//Array<double> scaleFactor(nCluster);
	ListConstIterator<Tuple2<int,double> > it;
	for(it = mainSiteWeights.begin(); it.valid(); )
	{
		double currentWeight    = (*it).x2();
		double currentDirection = currentWeight*2*Math::pi/C.m_nodesIn[mainSite].size();
		double sumLength = 0;
		SList<int> currentClusters;

		do {
			int    child  = (*it).x1();
			//double weight = (*it).x2();

			preferedDirection[child] = currentDirection;
			currentClusters.pushBack(child);
			sumLength += preferedAngle[child];

			++it;
		} while (it.valid() && (*it).x2() == currentWeight);

		ListIterator<SuperCluster> itSC = superClusters.pushBack(
			SuperCluster(currentClusters,currentDirection,sumLength,
			(sumLength <= 2*maxHalfAngle) ? 1.0 : 2*maxHalfAngle/sumLength));

		regions.pushBack(SCRegion(*itSC));
	}

#ifdef OUTPUT
	outputRegions(regions);
#endif

	// merging of regions
	bool changed;
	do {
		changed = false;

		ListIterator<SCRegion> itR1, itR2, itR3, itRNext;
		for(itR1 = regions.begin(); itR1.valid() && regions.size() >= 2; itR1 = itRNext)
		{
			itRNext = itR1.succ();

			itR2 = itR1.succ();
			bool finish = !itR2.valid();
			bool doMerge = false;

			if(!itR2.valid()) {
				itR2 = regions.begin();
				double alpha = angleNormalize((*itR1).m_start + 2*Math::pi);
				double beta = angleNormalize((*itR2).m_start);
				double dist = beta - alpha;
				if (dist < 0) dist += 2*Math::pi;
				double dx = (*itR1).m_length - dist;
				doMerge = dx > DBL_EPSILON;
				//doMerge = dist < (*itR1).m_length;

			} else {
				double alpha = angleNormalize((*itR1).m_start);
				double beta = angleNormalize((*itR2).m_start);
				double dist = beta - alpha;
				if (dist < 0) dist += 2*Math::pi;
				double dx = (*itR1).m_length - dist;
				doMerge = dx > DBL_EPSILON;
				//doMerge = dist < (*itR1).m_length;
			}

			if(!doMerge) continue;

			do {
				(*itR1).m_superClusters.conc((*itR2).m_superClusters);

				if(finish) {
					regions.del(itR2);
					break;
				}

				itR3 = itR2.succ();
				finish = !itR3.valid();
				doMerge = false;

				if(!itR3.valid()) {
					itR3 = regions.begin();
					double beta = angleNormalize((*itR3).m_start + 2*Math::pi);
					double alpha = angleNormalize((*itR2).m_start);
					double dist = beta - alpha;
					if (dist < 0) dist += 2*Math::pi;
					double dx = (*itR2).m_length - dist;
					doMerge = dx > DBL_EPSILON;
					//doMerge = dist < (*itR2).m_length;

				} else {
					double beta = angleNormalize((*itR3).m_start);
					double alpha = angleNormalize((*itR2).m_start);
					double dist = beta - alpha;
					if (dist < 0) dist += 2*Math::pi;
					double dx = (*itR2).m_length - dist;
					doMerge = dx > DBL_EPSILON;
					//doMerge = dist < (*itR2).m_length;
				}

				itRNext = itR2.succ();
				regions.del(itR2);

				itR2 = itR3;

			} while(regions.size() >= 2 && doMerge);

			double sectorStart = 0, sectorEnd, sectorLength = 0;
			bool singleRegion = false;
			if(regions.size() == 1) {
				sectorLength = 2*Math::pi;
				singleRegion = true;
			} else {
				sectorEnd   = angleNormalize((*regions.cyclicSucc(itR1)).m_start);
				ListIterator<SCRegion> itPred = regions.cyclicPred(itR1);
				sectorStart = angleNormalize((*itPred).m_start + (*itPred).m_length);
				sectorLength = sectorEnd - sectorStart;
				if(sectorLength < 0) sectorLength += 2*Math::pi;
			}

			changed = true;
			//compute deflection of R1
			double sumLength = 0, maxGap = -1;
			SListConstIterator<SuperCluster*> it, itStartRegion;
			const SList<SuperCluster*> &superClustersR1 = (*itR1).m_superClusters;
			for(it = superClustersR1.begin(); it.valid(); ++it)
			{
				sumLength += (*it)->m_length;

				SListConstIterator<SuperCluster*> itSucc = superClustersR1.cyclicSucc(it);
				double gap = (*itSucc)->m_direction - (*it)->m_direction;
				if (gap < 0) gap += 2*Math::pi;
				if(gap > maxGap) {
					maxGap = gap; itStartRegion = itSucc;
				}
			}

			// compute scaling
			double scaleFactor = (sumLength <= sectorLength) ? 1 : sectorLength/sumLength;

			double sumWAngles = 0;
			double sumDef    = 0;
			(*itR1).m_start = (*itStartRegion)->m_direction - scaleFactor * (*itStartRegion)->m_length/2;
			double posStart  = (*itR1).m_start;
			it = itStartRegion;
			do
			{
				double currentLength = scaleFactor * (*it)->m_length;
				sumDef    += angleDistance((*it)->m_direction, posStart + currentLength/2);
				posStart  += currentLength;

				double currentPos = (*it)->m_direction;
				if (currentPos < (*itR1).m_start)
					currentPos += 2*Math::pi;
				sumWAngles += (*it)->m_length * currentPos;

				it = superClustersR1.cyclicSucc(it);
			} while(it != itStartRegion);

			double deflection = sumDef / (*itR1).m_superClusters.size();
			while(deflection < -Math::pi) deflection += 2*Math::pi;
			while(deflection >  Math::pi) deflection -= 2*Math::pi;

			(*itR1).m_start += deflection;

			double center = sumWAngles / sumLength;
			while(center < 0   ) center += 2*Math::pi;
			while(center > 2*Math::pi) center -= 2*Math::pi;

			double tmpScaleFactor = scaleFactor;
			double left = center - tmpScaleFactor*sumLength/2;
			for(it = (*itR1).m_superClusters.begin(); it.valid(); ++it)
			{
				if(left < center) {
					double minLeft = (*it)->m_direction-maxHalfAngle;
					if(angleSmaller(left, minLeft)) {
						scaleFactor = min(scaleFactor,
							tmpScaleFactor * angleDistance(minLeft,center) / angleDistance(left,center));
					}
					OGDF_ASSERT(scaleFactor > 0);
				}

				double right = left + tmpScaleFactor*(*it)->m_length;

				if(right > center) {
					double maxRight = (*it)->m_direction+maxHalfAngle;
					if(angleSmaller(maxRight, right)) {
						scaleFactor = min(scaleFactor,
							tmpScaleFactor * angleDistance(maxRight,center) / angleDistance(right,center));
					}
					OGDF_ASSERT(scaleFactor > 0);
				}

				double currentLength = right-left;
				if(currentLength < 0) currentLength += 2*Math::pi;
				if(currentLength > 2*maxHalfAngle)
					scaleFactor = min(scaleFactor, 2*maxHalfAngle/currentLength);

				left = right;
			}

			OGDF_ASSERT(scaleFactor > 0);

			// set scale factor for all super clusters in region
			if(!singleRegion) itStartRegion = superClustersR1.begin();
			ListIterator<SCRegion> itFirst;
			it = itStartRegion;
			do
			{
				(*it)->m_scaleFactor = scaleFactor;

				// build new region for each super-cluster in R1
				ListIterator<SCRegion> itInserted =
					regions.insertBefore(SCRegion(*(*it)),itR1);

				if(!singleRegion) {
					angleRangeAdapt(sectorStart, sectorEnd,
						(*itInserted).m_start, (*itInserted).m_length);
				}

				(*itInserted).m_start = angleNormalize((*itInserted).m_start);

				if(!itFirst.valid())
					itFirst = itInserted;

				it = superClustersR1.cyclicSucc(it);
			} while(it != itStartRegion);

			// merge regions
			bool changedInternal;
			do {
				changedInternal = false;

				ListIterator<SCRegion> itA = (singleRegion) ? regions.begin() : itFirst,itB;
				bool finished = false;
				itB = itA.succ();
				for( ; ; )
				{
					if(itB == itR1) {
						if(singleRegion) {
							itB = regions.begin();
							if (itA == itB) break;
							finished = true;
						} else break;
					}

					if(angleSmaller((*itB).m_start, (*itA).m_start + (*itA).m_length))
					{
						(*itA).m_superClusters.conc((*itB).m_superClusters);
						(*itA).m_length += (*itB).m_length;

						//compute deflection of RA
						double sumDef    = 0;
						double posStart  = (*itA).m_start;
						SListConstIterator<SuperCluster*> it;
						for(it = (*itA).m_superClusters.begin(); it.valid(); ++it) {
							double currentDef = (*it)->m_direction - (posStart + (*it)->m_scaleFactor * (*it)->m_length/2);
							if(currentDef > Math::pi) currentDef -= 2*Math::pi;
							if(currentDef < -Math::pi) currentDef += 2*Math::pi;
							sumDef    += currentDef; //(*it)->m_direction - (posStart + (*it)->m_length/2);
							posStart  += (*it)->m_length * (*it)->m_scaleFactor;
						}
						double deflection = sumDef / (*itA).m_superClusters.size();
						(*itA).m_start += deflection;
						(*itA).m_start = angleNormalize((*itA).m_start);

						if(!singleRegion) {
							angleRangeAdapt(sectorStart, sectorEnd,
								(*itA).m_start, (*itA).m_length);
						}

						(*itA).m_start = angleNormalize((*itA).m_start);

						regions.del(itB);
						changedInternal = true;

					} else {
						itA = itB;
					}

					if(finished) break;

					itB = itA.succ();
				}
			} while(changedInternal);

			regions.del(itR1);

#ifdef OUTPUT
			outputRegions(regions);
#endif
		}
	} while(changed);

/*		ListIterator<ClusterRegion> itR1 = regions.begin(),itR2;
		for(itR2 = itR1.succ(); true; itR2 = itR1.succ())
		{
			if(regions.size() == 1)
				break;

			bool finish = !itR2.valid();
			bool doMerge = false;

			if(!itR2.valid()) {
				itR2 = regions.begin();
				doMerge = (*itR2).m_start + 2*Math::pi < (*itR1).m_start + (*itR1).m_length;
			} else
				doMerge = (*itR2).m_start < (*itR1).m_start + (*itR1).m_length;

			if (doMerge)
			{
				(*itR1).m_clusters.conc((*itR2).m_clusters);
				//(*itR1).m_length += (*itR2).m_length; // sp?ter bestimmen

				//compute deflection of R1
				double sumPrefAngles = 0;
				double sumDef = 0;
				double posStart = (*itR1).m_start;
				SListConstIterator<int> it;
				for(it = (*itR1).m_clusters.begin(); it.valid(); ++it)
				{
					sumPrefAngles += preferedAngle[*it];
					sumDef += preferedDirection[*it] - (posStart + preferedAngle[*it]/2);
					posStart += preferedAngle[*it];
				}
				double deflection = sumDef / (*itR1).m_clusters.size();
				(*itR1).m_start += deflection;

				regions.del(itR2);
				changed = true;

				// compute scaling
				double scaleFactor = (sumPrefAngles <= 2*Math::pi) ? 1 : 2*Math::pi/sumPrefAngles;
				double left = (*itR1).m_start;
				double center = left + sumPrefAngles/2;
				for(it = (*itR1).m_clusters.begin(); it.valid(); ++it)
				{
					double minLeft = preferedDirection[*it]-maxHalfAngle;

					if(left < minLeft) {
						scaleFactor =
							(preferedDirection[*it]-maxHalfAngle-center)/(left-center);
					}

					left += preferedAngle[*it]; // "left" is now "right" for this cluster

					double maxRight = preferedDirection[*it]+maxHalfAngle;
					if(maxRight < left) {
						scaleFactor =
							(preferedDirection[*it]+maxHalfAngle-center)/(left-center);
					}
				}

				(*itR1).m_length = scaleFactor * sumPrefAngles;
				(*itR1).m_start  = center - (*itR1).m_length / 2;
				(*itR1).m_scaleFactor = scaleFactor;
				if((*itR1).m_start > 2*Math::pi)
					(*itR1).m_start -= 2*Math::pi;

#ifdef OUTPUT
				outputRegions(regions);
#endif


			} else {
				itR1 = itR2;
			}

			if(finish) break;
		}
	} while(changed);
*/
	ListIterator<SCRegion> itR;
	//double minDist   = outerRadius[mainSite] + m_minDistLevel;
	//double sectorEnd = posStart+2*Math::pi;
	for(itR = regions.begin(); itR.valid(); ++itR)
	{
		double posStart  = (*itR).m_start;

		SListConstIterator<SuperCluster*> itSC;
		for(itSC = (*itR).m_superClusters.begin(); itSC.valid(); ++itSC)
		{
			double scaleFactor = (*itSC)->m_scaleFactor;

			SListConstIterator<int> it;
			for(it = (*itSC)->m_cluster.begin(); it.valid(); ++it)
			{
				double length = scaleFactor * preferedAngle[*it];

				circleAngle[*it] = posStart + length/2;
				circleQueue.append(QueuedCirclePosition(
					*it,minDist,posStart,posStart+length));

				posStart += length;
			}
		}
	}

/*		double posRegionEnd = R1.m_start;

		SListConstIterator<int> it;
		for(it = R1.m_clusters.begin(); it.valid(); ++it)
		{
			posRegionEnd += R1.m_scaleFactor*preferedAngle[*it];
			if(it != R1.m_clusters.rbegin())
			{
				circleQueue.append(QueuedCirclePosition(
					*it,minDist,posStart,posRegionEnd));
				circleAngle[*it] = posRegionEnd - R1.m_scaleFactor*preferedAngle[*it]/2;

				posStart = posRegionEnd;

			} else {
				itR2 = itR1.succ();
				circleAngle[*it] = posRegionEnd - R1.m_scaleFactor*preferedAngle[*it]/2;

				if(itR2.valid()) {
					double gap = (*itR2).m_start - posRegionEnd;
					posRegionEnd += gap * R1.m_scaleFactor*preferedAngle[*it] /
						(R1.m_scaleFactor*preferedAngle[*it] + (*itR2).m_scaleFactor*preferedAngle[(*itR2).m_clusters.front()]);
					circleQueue.append(QueuedCirclePosition(
						*it,minDist,posStart,posRegionEnd));
					posStart = posRegionEnd;
				} else {
					circleQueue.append(QueuedCirclePosition(
						*it,minDist,posStart,sectorEnd));
				}
			}
		}
	}*/
	// end of pendulum method -------------------------------------------


	/*double completeAngle = 2 * Math::pi;
	double angle = 0;
	//double minDist = outerRadius[mainSite] + m_minDistLevel;//
	//ListConstIterator<Tuple2<int,double> > it;//
	double sum = 0;
	for(it = mainSiteWeights.begin(); it.valid(); ++it)
	{
		int    child  = (*it).x1();
		double weight = (*it).x2();

		double delta = completeAngle * preferedAngle[child] / sumPrefAngles;

		//double gammaC = angle+delta/2 - weight*2*Math::pi/C.m_nodesIn[mainSite].size();
		double gammaC = circleAngle[child] - weight*2*Math::pi/C.m_nodesIn[mainSite].size();

		sum += gammaC;

		//circleAngle[child] = angle + delta/2;//
		//circleQueue.append(QueuedCirclePosition(child,minDist,angle,angle+delta));//
		////circleQueue.append(QueuedCirclePosition(child,minDist,
		////	circleAngle[child]-realDelta/2,circleAngle[child]+realDelta/2));
		angle += delta;
	}

	double gammaMainSite = (mainSiteWeights.size() == 0) ? 0 : sum / mainSiteWeights.size();
	if(gammaMainSite < 0) gammaMainSite += 2*Math::pi;*/
	double gammaMainSite = 0;

	while(!circleQueue.empty())
	{
		QueuedCirclePosition qcp = circleQueue.pop();
		int cluster = qcp.m_cluster;

#ifdef OUTPUT
		cout << "cluster = " << cluster << ", start = " << fac*qcp.m_sectorStart <<
			", end = " << fac*qcp.m_sectorEnd << endl;
		cout << "  minDist = " << qcp.m_minDist <<
			", angle = " << fac*circleAngle[cluster] << endl;
#endif

		double delta = qcp.m_sectorEnd - qcp.m_sectorStart;
		if (delta >= Math::pi)
		//if (delta <= Math::pi)
		{
			circleDistance[cluster] = qcp.m_minDist + outerRadius[cluster];

		} else {
			double rMin = (outerRadius[cluster] + m_minDistSibling/2) /
				(sin(delta/2));

			circleDistance[cluster] = max(rMin,qcp.m_minDist+outerRadius[cluster]);
		}

		if(C.m_childCluster[cluster].empty())
			continue;

		minDist = circleDistance[cluster] + outerRadius[cluster] + m_minDistLevel;
		/*double alpha = acos((circleDistance[cluster]-outerRadius[cluster])/minDist);

		if(circleAngle[cluster]-alpha > qcp.m_sectorStart)
			qcp.m_sectorStart = circleAngle[cluster]-alpha;
		if(circleAngle[cluster]+alpha < qcp.m_sectorEnd)
			qcp.m_sectorEnd = circleAngle[cluster]+alpha;*/
		delta = qcp.m_sectorEnd - qcp.m_sectorStart;

		sumPrefAngles = 0;
		for(itC = C.m_childCluster[cluster].begin(); itC.valid(); ++itC)
		{
			sumPrefAngles += preferedAngle[*itC];

			// computing prefered directions
			double r = circleDistance[cluster];
			double a = minDist + outerRadius[*itC];
			double gamma = dirFromParent[*itC];

#ifdef OUTPUT
			cout << "    gamma of " << *itC << " = " << gamma/(2*Math::pi)*360 << endl;
#endif
			if(gamma <= Math::pi/2)
				preferedDirection[*itC] = qcp.m_sectorStart;
			else if(gamma >= 3*Math::pi/2)
				preferedDirection[*itC] = qcp.m_sectorEnd;
			//else if(gamma == Math::pi) // Achtung! nicht Gleichheit ohne Toleranz testen!
			else if(DIsEqual(gamma,Math::pi))
				preferedDirection[*itC] = circleAngle[cluster];
			else {
				double gamma2 = (gamma < Math::pi) ? Math::pi - gamma : gamma - Math::pi;
				double K = 1 + 1 /(tan(gamma2)*tan(gamma2));
				double C = r/(a*tan(gamma2))/K;
				double C2 = sqrt((1-(r/a)*(r/a))/K + C*C);

				double beta = asin(C2-C);
				if (gamma < Math::pi)
					preferedDirection[*itC] = circleAngle[cluster]-beta;
				else
					preferedDirection[*itC] = circleAngle[cluster]+beta;
			}
#ifdef OUTPUT
			cout << "    dir. of  " << *itC << ": " << fac*preferedDirection[*itC] << endl;
#endif
		}

		if(sumPrefAngles >= delta)
		{
			double angle = qcp.m_sectorStart;
			for(itC = C.m_childCluster[cluster].begin(); itC.valid(); ++itC)
			{
				double deltaChild = delta * preferedAngle[*itC] / sumPrefAngles;

				circleAngle[*itC] = angle + deltaChild/2;
				circleQueue.append(QueuedCirclePosition(*itC,minDist,angle,angle+deltaChild));
				angle += deltaChild;
			}

		} else {
			List<ClusterRegion> regions;
			for(itC = C.m_childCluster[cluster].begin(); itC.valid(); ++itC)
			{
				double start  = preferedDirection[*itC]-preferedAngle[*itC]/2;
				double length = preferedAngle[*itC];

				if(start < qcp.m_sectorStart)
					start = qcp.m_sectorStart;
				if(start + length >  qcp.m_sectorEnd)
					start = qcp.m_sectorEnd - length;

				regions.pushBack(ClusterRegion(*itC,start,length));
			}

			bool changed;
			do {
				changed = false;

				ListIterator<ClusterRegion> itR1 = regions.begin(),itR2;
				for(itR2 = itR1.succ(); itR2.valid(); itR2 = itR1.succ())
				{
					if((*itR2).m_start < (*itR1).m_start + (*itR1).m_length)
					{
						(*itR1).m_clusters.conc((*itR2).m_clusters);
						(*itR1).m_length += (*itR2).m_length;

						//compute deflection of R1
						double sumDef = 0;
						double posStart = (*itR1).m_start;
						SListConstIterator<int> it;
						for(it = (*itR1).m_clusters.begin(); it.valid(); ++it) {
							sumDef += preferedDirection[*it] - (posStart + preferedAngle[*it]/2);
							posStart += preferedAngle[*it];
						}
						double deflection = sumDef / (*itR1).m_clusters.size();
						(*itR1).m_start += deflection;

						if((*itR1).m_start < qcp.m_sectorStart)
							(*itR1).m_start = qcp.m_sectorStart;
						if((*itR1).m_start + (*itR1).m_length >  qcp.m_sectorEnd)
							(*itR1).m_start = qcp.m_sectorEnd - (*itR1).m_length;

						regions.del(itR2);
						changed = true;

					} else {
						itR1 = itR2;
					}
				}
			} while(changed);

			double posStart = qcp.m_sectorStart;
			ListIterator<ClusterRegion> itR1, itR2;
			for(itR1 = regions.begin(); itR1.valid(); itR1 = itR2)
			{
				const ClusterRegion &R1 = *itR1;

				double posRegionEnd = R1.m_start;
				SListConstIterator<int> it;
				for(it = R1.m_clusters.begin(); it.valid(); ++it)
				{
					posRegionEnd += preferedAngle[*it];
					if(it != R1.m_clusters.rbegin())
					{
						circleQueue.append(QueuedCirclePosition(
							*it,minDist,posStart,posRegionEnd));
						circleAngle[*it] = posRegionEnd - preferedAngle[*it]/2;

						posStart = posRegionEnd;

					} else {
						itR2 = itR1.succ();
						circleAngle[*it] = posRegionEnd - preferedAngle[*it]/2;
						if(itR2.valid()) {
							double gap = (*itR2).m_start - posRegionEnd;
							posRegionEnd += gap * preferedAngle[*it] /
								(preferedAngle[*it] + preferedAngle[(*itR2).m_clusters.front()]);
							circleQueue.append(QueuedCirclePosition(
								*it,minDist,posStart,posRegionEnd));
							posStart = posRegionEnd;
						} else {
							circleQueue.append(QueuedCirclePosition(
								*it,minDist,posStart,qcp.m_sectorEnd));
						}
					}
				}
			}
		}
	}

#ifdef OUTPUT
	cout << "\ncircle positions:\n";
#endif
	for(i = 0; i < nCluster; ++i) {
#ifdef OUTPUT
		cout << i << ": dist  \t" << circleDistance[i] << endl;
		cout << "    angle \t" << circleAngle[i] << endl;
#endif

		// determine gamma and M
		double mX, mY, gamma;

		if(i == mainSite)
		{
			mX = 0;
			mY = 0;
			gamma = gammaMainSite;

		} else {
			double alpha = circleAngle[i];
			if(alpha <= Math::pi/2) {
				// upper left
				double beta = Math::pi/2 - alpha;
				mX = -circleDistance[i] * cos(beta);
				mY =  circleDistance[i] * sin(beta);
				gamma = 1.5*Math::pi - beta;

			} else if(alpha <= Math::pi) {
				// lower left
				double beta = alpha - Math::pi/2;
				mX = -circleDistance[i] * cos(beta);
				mY = -circleDistance[i] * sin(beta);
				gamma = 1.5*Math::pi + beta;

			} else if(alpha <= 1.5*Math::pi) {
				// lower right
				double beta = 1.5*Math::pi - alpha;
				mX =  circleDistance[i] * cos(beta);
				mY = -circleDistance[i] * sin(beta);
				gamma = Math::pi/2 - beta;

			} else {
				// upper right
				double beta = alpha - 1.5*Math::pi;
				mX =  circleDistance[i] * cos(beta);
				mY =  circleDistance[i] * sin(beta);
				gamma = Math::pi/2 + beta;
			}
		}

		const int n = C.m_nodesIn[i].size();
		int pos = 0;
		SListConstIterator<node> itV;
		for(itV = C.m_nodesIn[i].begin(); itV.valid(); ++itV, ++pos)
		{
			node v = *itV;

			double phi = pos - parentWeight[i];
			if(phi < 0) phi += n;

			phi = phi * 2*Math::pi/n + gamma;
			if(phi >= 2*Math::pi) phi -= 2*Math::pi;

			double x, y;
			if(phi <= Math::pi/2) {
				// upper left
				double beta = Math::pi/2 - phi;
				x = -radius[i] * cos(beta);
				y =  radius[i] * sin(beta);

			} else if(phi <= Math::pi) {
				// lower left
				double beta = phi - Math::pi/2;
				x = -radius[i] * cos(beta);
				y = -radius[i] * sin(beta);

			} else if(phi <= 1.5*Math::pi) {
				// lower right
				double beta = 1.5*Math::pi - phi;
				x =  radius[i] * cos(beta);
				y = -radius[i] * sin(beta);

			} else {
				// upper right
				double beta = phi - 1.5*Math::pi;
				x =  radius[i] * cos(beta);
				y =  radius[i] * sin(beta);
			}

			AG.x(v) = x + mX;
			// minus sign only for debugging!
			AG.y(v) = -(y + mY);
		}
	}

}


void CircularLayout::computePreferedAngles(
	ClusterStructure &C,
	const Array<double> &outerRadius,
	Array<double> &preferedAngle)
{
	const int nCluster = C.numberOfCluster();
	const int mainSite = C.m_mainSiteCluster.front();

	Array<int> level(nCluster);
	Queue<int> Q;

	level[mainSite] = 0;
	Q.append(mainSite);

	int nLevel = 0;
	while(!Q.empty())
	{
		int c = Q.pop();

		nLevel = level[c]+1;
		ListConstIterator<int> it;
		for(it = C.m_childCluster[c].begin(); it.valid(); ++it) {
			level[*it] = nLevel;
			Q.append(*it);
		}
	}

	ListConstIterator<int> it;
	for(it = C.m_childCluster[mainSite].begin(); it.valid(); ++it)
		assignPrefAngle(C,outerRadius,preferedAngle,
			*it,1,outerRadius[mainSite]+m_minDistLevel);

#ifdef OUTPUT
	cout << "\nprefered angles:" << endl;
	for(int i = 0; i < nCluster; ++i)
		cout << i << ": " << 360*preferedAngle[i]/(2*Math::pi) << endl;
#endif
}

void CircularLayout::assignPrefAngle(ClusterStructure &C,
	const Array<double> &outerRadius,
	Array<double> &preferedAngle,
	int c,
	int l,
	double r1)
{
	double maxPrefChild = 0;

	ListConstIterator<int> it;
	for(it = C.m_childCluster[c].begin(); it.valid(); ++it) {
		assignPrefAngle(C,outerRadius,preferedAngle,
			*it,l+1,r1 + m_minDistLevel + 2*outerRadius[c]);
		/*if(preferedAngle[*it] > maxPrefChild)
			maxPrefChild = preferedAngle[*it];*/
		maxPrefChild += preferedAngle[*it];
	}

	double rc = r1 + outerRadius[c];
	//preferedAngle[c] = max((2*outerRadius[c] + m_minDistSibling) / rc, maxPrefChild);
	preferedAngle[c] = max(2*asin((outerRadius[c] + m_minDistSibling/2)/rc), maxPrefChild);
}


//---------------------------------------------------------
// assigns the biconnected components of the graph as clusters
//---------------------------------------------------------
void CircularLayout::assignClustersByBiconnectedComponents(ClusterStructure &C)
{
	const Graph &G = C;

	//---------------------------------------------------------
	// compute biconnected components
	EdgeArray<int> compnum(G);
	int k = biconnectedComponents(G,compnum);


	//---------------------------------------------------------
	// compute BC-tree
	//
	// better: proved a general class BCTree with the functionality
	//
	NodeArray<SList<int> > compV(G);
	Array<SList<node> >    nodeB(k);

	// edgeB[i] = list of edges in component i
	Array<SList<edge> > edgeB(k);
	edge e;
	forall_edges(e,G)
		if(!e->isSelfLoop())
			edgeB[compnum[e]].pushBack(e);

	// construct arrays compV and nodeB such that
	// compV[v] = list of components containing v
	// nodeB[i] = list of vertices in component i
	NodeArray<bool> mark(G,false);

	int i;
	for(i = 0; i < k; ++i) {
		SListConstIterator<edge> itEdge;
		for(itEdge = edgeB[i].begin(); itEdge.valid(); ++itEdge)
		{
			edge e = *itEdge;

			if (!mark[e->source()]) {
				mark[e->source()] = true;
				nodeB[i].pushBack(e->source());
			}
			if (!mark[e->target()]) {
				mark[e->target()] = true;
				nodeB[i].pushBack(e->target());
			}
		}

		SListConstIterator<node> itNode;
		for(itNode = nodeB[i].begin(); itNode.valid(); ++itNode)
		{
			node v = *itNode;
			compV[v].pushBack(i);
			mark[v] = false;
		}
	}
	mark.init();

	Graph BCTree;
	NodeArray<int>  componentOf(BCTree,-1);
	NodeArray<node> cutVertexOf(BCTree,0);
	Array<node>     nodeOf(k);

	for(i = 0; i < k; ++i) {
		node vBC = BCTree.newNode();
		componentOf[vBC] = i;
		nodeOf[i] = vBC;
	}

	node v;
	forall_nodes(v,G)
	{
		if (compV[v].size() > 1) {
			node vBC = BCTree.newNode();
			cutVertexOf[vBC] = v;
			SListConstIterator<int> it;
			for(it = compV[v].begin(); it.valid(); ++it)
				BCTree.newEdge(vBC,nodeOf[*it]);
		}
	}

	//---------------------------------------------------------
	// find center of BC-tree
	//
	// we currently use the center of the tree as main-site cluster
	// alternatives are: "weighted" center (concerning size of BC's,
	//                   largest component
	//
	node centerBC = 0;

	if(BCTree.numberOfNodes() == 1)
	{
		centerBC = BCTree.firstNode();

	} else {
		NodeArray<int> deg(BCTree);
		Queue<node> leaves;

		node vBC;
		forall_nodes(vBC,BCTree) {
			deg[vBC] = vBC->degree();
			if(deg[vBC] == 1)
				leaves.append(vBC);
		}

		node current = 0;
		while(!leaves.empty())
		{
			current = leaves.pop();

			edge e;
			forall_adj_edges(e,current) {
				node w = e->opposite(current);
				if (--deg[w] == 1)
					leaves.append(w);
			}
		}

		OGDF_ASSERT(current != 0);
		centerBC = current;

		// if center node current of BC-Tree is a cut-vertex, we choose the
		// maximal bic. comp. containing current as centerBC
		if (componentOf[centerBC] == -1) {
			int sizeCenter = 0;
			node vCand = 0;

			edge e;
			forall_adj_edges(e,current) {
				node w = e->opposite(current);
				int sizeW = sizeBC(w);
				if(sizeW > sizeCenter) {
					vCand = w;
					sizeCenter = sizeW;
				}
			}

			// take maximal bic. comp only if not a a bridge
			if(vCand && nodeB[componentOf[vCand]].size() > 2)
				centerBC = vCand;

		// if a bridge is chosen as center, we take the closest non-bridge
		} else if(nodeB[componentOf[centerBC]].size() == 2 && centerBC->degree() == 2)
		{
			//Queue<adjEntry> Q;
			SListPure<adjEntry> currentCand, nextCand;
			nextCand.pushBack(centerBC->firstAdj());
			nextCand.pushBack(centerBC->lastAdj());

			bool found = false;
			int bestSize = -1;
			while(!nextCand.empty() && !found)
			{
				currentCand.conc(nextCand);

				while(!currentCand.empty())
				{
					adjEntry adjParent = currentCand.popFrontRet()->twin();

					for(adjEntry adj = adjParent->cyclicSucc(); adj != adjParent; adj = adj->cyclicSucc())
					{
						adjEntry adjB = adj->twin();
						node vB = adjB->theNode();
						if(nodeB[componentOf[vB]].size() > 2) {
							int candSize = sizeBC(vB);
							if(!found || candSize > bestSize) {
								centerBC = vB;
								bestSize = candSize;
								found = true;
							}
						}
						adjEntry adjB2 = adjB->cyclicSucc();
						if(adjB2 != adjB)
							nextCand.pushBack(adjB2);
					}
				}
			}
		}
	}

#ifdef OUTPUT
	cout << "bic. comp.\n";
	for(i = 0; i < k; ++i)
		cout << i << ": " << nodeB[i] << endl;

	cout << "\nBC-Tree:\n";
	forall_nodes(v,BCTree) {
		cout << v << " [" << componentOf[v] << "," << cutVertexOf[v] << "]: ";
		edge e;
		forall_adj_edges(e,v)
			cout << e->opposite(v) << " ";
		cout << endl;
	}
	BCTree.writeGML("BC-Tree.gml");
#endif


	//---------------------------------------------------------
	// assign cluster
	//
	// we traverse the tree from the center to the outside
	// cut-vertices are assigned to the inner cluster which contains them
	// exception: bridges are no cluster at all if outer cut-vertex is only
	//   connected to one non-bridge [bridge -> c -> non bridge]
	int currentCluster = 0;
	Queue<InfoAC> Q;
	Array<int> parentCluster(k+1);

	if(componentOf[centerBC] == -1)
	{ // case cut vertex as center
		parentCluster[currentCluster] = -1;
		C.m_clusterOf[cutVertexOf[centerBC]] = currentCluster;

		edge e;
		forall_adj_edges(e,centerBC) {
			node bBC = e->opposite(centerBC);
			Q.append(InfoAC(bBC,centerBC,cutVertexOf[centerBC],currentCluster));
		}

		++currentCluster;

	} else { // case bic. comp. as center
		Q.append(InfoAC(centerBC,0,0,-1));
	}

	while(!Q.empty())
	{
		InfoAC info = Q.pop();

		// bridge?
		if(nodeB[componentOf[info.m_vBC]].size() == 2 &&
			info.m_predCut != 0 &&
			info.m_vBC->degree() == 2)
		{
			node wBC = info.m_vBC->firstAdj()->twinNode();
			if(wBC == info.m_predCutBC)
				wBC = info.m_vBC->lastAdj()->twinNode();

			if(wBC->degree() == 2)
			{
				node bBC = wBC->firstAdj()->twinNode();;
				if(bBC == info.m_vBC)
					bBC = wBC->lastAdj()->twinNode();

				if(nodeB[componentOf[bBC]].size() != 2)
				{
					Q.append(InfoAC(bBC,wBC,0,info.m_parentCluster));
					continue; // case already handled
				}
			}
		}

		SListConstIterator<node> itV;
		for(itV = nodeB[componentOf[info.m_vBC]].begin(); itV.valid(); ++itV)
			if (*itV != info.m_predCut)
				C.m_clusterOf[*itV] = currentCluster;

		parentCluster[currentCluster] = info.m_parentCluster;

		edge e1;
		forall_adj_edges(e1,info.m_vBC)
		{
			node wBC = e1->opposite(info.m_vBC);
			if(wBC == info.m_predCutBC) continue;

			edge e2;
			forall_adj_edges(e2,wBC) {
				node bBC = e2->opposite(wBC);
				if (bBC == info.m_vBC) continue;

				Q.append(InfoAC(bBC,wBC,cutVertexOf[wBC],currentCluster));
			}
		}

		++currentCluster;
	}

	C.initCluster(currentCluster,parentCluster);
	// in this case, the main-site cluster is always the first created
	C.m_mainSiteCluster.pushBack(0);

#ifdef OUTPUT
	cout << "\ncluster:\n";
	for(i = 0; i < currentCluster; ++i) {
		cout << i << ": " << C.m_nodesIn[i] << endl;
		cout << "   parent = " << C.m_parentCluster[i] << ", children " << C.m_childCluster[i] << endl;
	}
	cout << "main-site cluster: " << C.m_mainSiteCluster << endl;
#endif
}


int CircularLayout::sizeBC(node vB)
{
	int sum = 0;
	adjEntry adj;
	forall_adj(adj,vB)
		sum += adj->twinNode()->degree() - 1;
	return sum;
}




} // end namespace ogdf

