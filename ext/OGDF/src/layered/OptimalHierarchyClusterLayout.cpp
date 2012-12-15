/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of the optimal third phase of the
 * sugiyama algorithm for cluster graphs
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


#include <ogdf/layered/OptimalHierarchyClusterLayout.h>
#include <ogdf/lpsolver/LPSolver.h>
#include <ogdf/basic/Array2D.h>


#ifdef OGDF_LP_SOLVER

namespace ogdf {

int checkSolution(
	Array<int>    &matrixBegin,   // matrixBegin[i] = begin of column i
	Array<int>    &matrixCount,   // matrixCount[i] = number of nonzeroes in column i
	Array<int>    &matrixIndex,   // matrixIndex[n] = index of matrixValue[n] in its column
	Array<double> &matrixValue,	  // matrixValue[n] = non-zero value in matrix
	Array<double> &rightHandSide, // right-hand side of LP constraints
	Array<char>   &equationSense, // 'E' ==   'G' >=   'L' <=
	Array<double> &lowerBound,    // lower bound of x[i]
	Array<double> &upperBound,    // upper bound of x[i]
	Array<double> &x)             // x-vector of optimal solution (if result is lpOptimal)
{
	const double zeroeps = 1.0E-7;

	const int nCols = matrixBegin.size();
	const int nRows = rightHandSide.size();

	Array2D<double> M(0,nCols-1, 0,nRows-1, 0.0);

	int i;
	for(i = 0; i < nCols; ++i)
	{
		for(int j = 0; j < matrixCount[i]; ++j)
		{
			int k = matrixBegin[i] + j;
			M(i,matrixIndex[k]) = matrixValue[k];
		}
	}

	// check inequations
	for(i = 0; i < nRows; ++i)
	{
		double val = 0.0;
		for(int j = 0; j < nCols; ++j)
			val += M(j,i) * x[j];

		switch(equationSense[i]) {
			case 'E':
				if(fabs(val - rightHandSide[i]) > zeroeps)
					return i;
				break;
			case 'G':
				if(!(val+zeroeps >= rightHandSide[i]))
					return i;
				break;
			case 'L':
				if(!(val-zeroeps <= rightHandSide[i]))
					return i;
				break;
			default:
				return -2;
		}
	}

	return -1;
}


//---------------------------------------------------------
// Constructor
//---------------------------------------------------------
OptimalHierarchyClusterLayout::OptimalHierarchyClusterLayout()
{
	m_nodeDistance       = 3;
	m_layerDistance      = 3;
	m_fixedLayerDistance = false;
	m_weightSegments     = 2.0;
	m_weightBalancing    = 0.1;
	m_weightClusters     = 0.05;
}


//---------------------------------------------------------
// Copy Constructor
//---------------------------------------------------------
OptimalHierarchyClusterLayout::OptimalHierarchyClusterLayout(
	const OptimalHierarchyClusterLayout &ohl)
{
	m_nodeDistance       = ohl.nodeDistance();
	m_layerDistance      = ohl.layerDistance();
	m_fixedLayerDistance = ohl.fixedLayerDistance();
	m_weightSegments     = ohl.weightSegments();
	m_weightBalancing    = ohl.weightBalancing();
	m_weightClusters     = ohl.weightClusters();
}


//---------------------------------------------------------
// Assignment Operator
//---------------------------------------------------------
OptimalHierarchyClusterLayout &OptimalHierarchyClusterLayout::operator=(
	const OptimalHierarchyClusterLayout &ohl)
{
	m_nodeDistance       = ohl.nodeDistance();
	m_layerDistance      = ohl.layerDistance();
	m_fixedLayerDistance = ohl.fixedLayerDistance();
	m_weightSegments     = ohl.weightSegments();
	m_weightBalancing    = ohl.weightBalancing();
	m_weightClusters     = ohl.weightClusters();

	return *this;
}


//---------------------------------------------------------
// Call for Cluster Graphs
//---------------------------------------------------------
void OptimalHierarchyClusterLayout::doCall(
	const ExtendedNestingGraph &H,
	ClusterGraphCopyAttributes &ACGC)
{
	// trivial cases
	const int n = H.numberOfNodes();

	if(n == 0)
		return; // nothing to do

	if(n == 1) {
		node v = H.firstNode();
		ACGC.x(v) = 0;
		ACGC.y(v) = 0;
		return;
	}

	m_pH    = &H;
	m_pACGC = &ACGC;

	// actual computation
	computeXCoordinates(H,ACGC);
	computeYCoordinates(H,ACGC);
}


//---------------------------------------------------------
// Compute x-coordinates (LP-based approach) (for cluster graphs)
//---------------------------------------------------------
void OptimalHierarchyClusterLayout::computeXCoordinates(
	const ExtendedNestingGraph& H,
	ClusterGraphCopyAttributes &AGC)
{
	const ClusterGraphCopy &CG = H.getClusterGraph();

	const int k = H.numberOfLayers();

	//
	// preprocessing: determine nodes that are considered as virtual
	//
	m_isVirtual.init(H);

	int i;
	for(i = 0; i < k; ++i)
	{
		int last = -1;

		// Process nodes on layer i from left to right
		Stack<const LHTreeNode*> S;
		S.push(H.layerHierarchyTree(i));
		while(!S.empty())
		{
			const LHTreeNode *vNode = S.pop();

			if(vNode->isCompound()) {
				for(int j = vNode->numberOfChildren()-1; j >= 0; --j)
					S.push(vNode->child(j));

			} else {
				node v = vNode->getNode();

				if(H.isLongEdgeDummy(v) == true) {
					m_isVirtual[v] = true;

					edge e = v->firstAdj()->theEdge();
					if(e->target() == v)
						e = v->lastAdj()->theEdge();
					node u = e->target();

					if(H.verticalSegment(e) == false)
						continue;

					if(H.isLongEdgeDummy(u) == true) {
						int down = H.pos(u);
						if(last != -1 && last > down) {
							m_isVirtual[v] = false;
						} else {
							last = down;
						}
					}
				} else {
					m_isVirtual[v] = false;
				}
			}
		}
	}

	//
	// determine variables of LP
	//
	int nSegments     = 0; // number of vertical segments
	int nRealVertices = 0; // number of real vertices
	int nEdges        = 0; // number of edges not in vertical segments
	int nBalanced     = 0; // number of real vertices with deg > 1 for which balancing constraints may be applied

	m_vIndex.init(H,-1); // for real node: index of x[v] for dummy: index of corresponding segment
	NodeArray<int> bIndex(H,-1); // (relative) index of b[v]
	EdgeArray<int> eIndex(H,-1); // for edge not in vertical segment: its index
	Array<int> count(H.numberOfEdges());	// counts the number of dummy vertices
											// in corresponding segment that are not at
											// position 0

	int nSpacingConstraints = 0;
	for(i = 0; i < k; ++i)
	{
		Stack<const LHTreeNode*> S;
		S.push(H.layerHierarchyTree(i));
		while(!S.empty())
		{
			const LHTreeNode *vNode = S.pop();

			if(vNode->isCompound()) {
				cluster c = vNode->originalCluster();

				if(H.isVirtual(c) == false)
					nSpacingConstraints += (c == CG.rootCluster()) ? 1 : 2;

				for(int j = vNode->numberOfChildren()-1; j >= 0; --j)
					S.push(vNode->child(j));

			} else {
				node v = vNode->getNode();

				// ignore dummy nodes and nodes representing cluster
				// (top or bottom) border
				if(H.type(v) == ExtendedNestingGraph::ntClusterBottom || H.type(v) == ExtendedNestingGraph::ntClusterTop)
					continue;

				++nSpacingConstraints;

				// ignore dummy nodes
				if(m_isVirtual[v] == true)
					continue;

				// we've found a real vertex
				m_vIndex[v] = nRealVertices++;
				if(v->degree() > 1)
					bIndex[v] = nBalanced++;

				// consider all outgoing edges
				edge e;
				forall_adj_edges(e,v) {
					node w = e->target();
					if(w == v)
						continue;

					// we've found an edge not belonging to a vetical segment
					eIndex[e] = nEdges++;

					if(m_isVirtual[w] == false)
						continue;

					do {
						// we've found a vertical segment
						count[nSegments] = 0;
						do {
							m_vIndex[w] = nSegments;
							count[nSegments] += 2;

							// next edge / dummy in segment
							e = e->adjTarget()->cyclicSucc()->theEdge();
							w = e->target();
						} while(m_isVirtual[w] && H.verticalSegment(e));

						++nSegments;

						// edge following vertical segment
						eIndex[e] = nEdges++;

					} while(m_isVirtual[w]);
				}
			}
		}
	}

	// Assign indices to clusters
	int nClusters = 0;
	m_cIndex.init(CG,-1);

	cluster c;
	forall_clusters(c,CG)
		if(H.isVirtual(c) == false)
			m_cIndex[c] = nClusters++;


	// assignment of variables to matrix columns
	//   d_e                  0, ...,                     nEdges-1
	//   x_v       vertexOffset, ..., vertexOffset      + nRealVertices-1
	//   x_s      segmentOffset, ..., segmentOffset     + nSegments-1
	//   b_v     balancedOffset, ..., balancedOffset    + nBalanced-1
	//   l_c   clusterLefOffset, ..., clusterLeftOffset + nClusters-1
	//   r_c clusterRightOffset, ..., clusterRightOffset+ nClusters-1
	LPSolver solver;

	if(m_weightBalancing <= 0.0)
		nBalanced = 0; // no balancing

	const int nCols = nEdges + nRealVertices + nSegments + nBalanced + 2*nClusters;
	const int nRows = 2*nEdges + nSpacingConstraints + 2*nBalanced;

	m_vertexOffset       = nEdges;
	m_segmentOffset      = nEdges + nRealVertices;
	const int balancedOffset     = m_segmentOffset + nSegments;
	m_clusterLeftOffset  = balancedOffset + nBalanced;
	m_clusterRightOffset = m_clusterLeftOffset + nClusters;

	// allocation of matrix
	Array<int> matrixCount(0,nCols-1,0);

	for(i = 0; i < nEdges; ++i) {
		matrixCount[i] = 2;
	}

	for(int jj = 0; jj < k; ++jj) {
		const LHTreeNode *layerRoot = H.layerHierarchyTree(jj);

		Stack<const LHTreeNode*> S;
		S.push(layerRoot);
		while(!S.empty())
		{
			const LHTreeNode *vNode = S.pop();

			if(vNode->isCompound()) {
				i = m_cIndex[vNode->originalCluster()];

				if(i >= 0) {
					int count = (vNode == layerRoot) ? 1 : 2;

					matrixCount[m_clusterLeftOffset  + i] += count;
					matrixCount[m_clusterRightOffset + i] += count;
				}

				for(int j = vNode->numberOfChildren()-1; j >= 0; --j)
					S.push(vNode->child(j));

			} else {
				node v = vNode->getNode();

				// ignore nodes representing cluster (top or bottom) border
				if(H.type(v) == ExtendedNestingGraph::ntClusterBottom ||
					H.type(v) == ExtendedNestingGraph::ntClusterTop)
					continue;

				if(m_isVirtual[v] == false) {
					i = m_vertexOffset + m_vIndex[v];

					int count =  2 + 2*v->degree();
					if(nBalanced > 0) {
						if(v->degree() > 1)
							count += 2;
						adjEntry adj;
						forall_adj(adj,v) {
							node w = adj->twinNode();
							if(bIndex[w] != -1)
								count += 2;
						}
					}

					matrixCount[i] = count;

				} else if (nBalanced > 0) {
					i = m_vIndex[v];
					adjEntry adj;
					forall_adj(adj,v) {
						node w = adj->twinNode();
						if(bIndex[w] != -1)
							count[i] += 2;
					}
				}
			}
		}
	}

	for(i = 0; i < nSegments; ++i) {
		matrixCount[m_segmentOffset+i] = count[i] + 4;
	}

	for(i = 0; i < nBalanced; ++i) {
		matrixCount[balancedOffset+i] = 2;
	}


	// Computation of matrixBegin[i] from given matrixCount values
	Array<int> matrixBegin(nCols);

	int nNonZeroes = 0;
	for(i = 0; i < nCols; ++i) {
		matrixBegin[i] = nNonZeroes;
		nNonZeroes += matrixCount[i];
	}


	int debugNonZeroCount = 0; // for debugging only

	//
	// constraints
	//
	Array<int>    matrixIndex(nNonZeroes);
	Array<double> matrixValue(nNonZeroes);
	Array<char>   equationSense(nRows);
	Array<double> rightHandSide(nRows);

	int currentRow = 0;
	Array<int> currentCol(nCols);
	for(i = 0; i < nCols; ++i)
		currentCol[i] = matrixBegin[i];

	// Constraints:
	//   d_(u,v) - x_u + x_v >= 0
	//   d_(u,v) + x_u - x_v >= 0
	edge e;
	forall_edges(e,H)
	{
		int dCol = eIndex[e];
		if(dCol >= 0) {
			node u = e->source();
			int uCol = m_vIndex[u];
			uCol += (m_isVirtual[u]) ? m_segmentOffset : m_vertexOffset;

			node v = e->target();
			int vCol = m_vIndex[v];
			vCol += (m_isVirtual[v]) ? m_segmentOffset : m_vertexOffset;

			// d_(u,v) - x_u + x_v >= 0

			matrixValue[currentCol[dCol]] = 1.0;
			matrixIndex[currentCol[dCol]] = currentRow;
			++currentCol[dCol];
			debugNonZeroCount++;

			matrixValue[currentCol[uCol]] = -1.0;
			matrixIndex[currentCol[uCol]] = currentRow;
			++currentCol[uCol];
			debugNonZeroCount++;

			matrixValue[currentCol[vCol]] = 1.0;
			matrixIndex[currentCol[vCol]] = currentRow;
			++currentCol[vCol];
			debugNonZeroCount++;

			equationSense[currentRow] = 'G';
			rightHandSide[currentRow] = 0.0;

			++currentRow;

			// d_(u,v) + x_u - x_v >= 0

			matrixValue[currentCol[dCol]] = 1.0;
			matrixIndex[currentCol[dCol]] = currentRow;
			++currentCol[dCol];
			debugNonZeroCount++;

			matrixValue[currentCol[uCol]] = 1.0;
			matrixIndex[currentCol[uCol]] = currentRow;
			++currentCol[uCol];
			debugNonZeroCount++;

			matrixValue[currentCol[vCol]] = -1.0;
			matrixIndex[currentCol[vCol]] = currentRow;
			++currentCol[vCol];
			debugNonZeroCount++;

			equationSense[currentRow] = 'G';
			rightHandSide[currentRow] = 0.0;

			++currentRow;
		}
	}


	// Constraints:
	//   x[v_i] - x[v_(i-1)] >= nodeDistance + 0.5*(width(v_i)+width(v_(i-1))
	for(i = 0; i < k; ++i)
	{
		List<Tuple2<int,double> > L;
		buildLayerList(H.layerHierarchyTree(i), L);

		if(L.size() < 2)
			continue;

		ListConstIterator<Tuple2<int,double> > it1 = L.begin();
		for(ListConstIterator<Tuple2<int,double> > it2 = it1.succ();
			it2.valid(); it1 = it2, ++it2)
		{
			int    uCol   = (*it1).x1();
			double uWidth = (*it1).x2();

			int    vCol   = (*it2).x1();
			double vWidth = (*it2).x2();

			// x_v - x_u >= nodeDistance + 0.5*(width(v)+width(u))
			matrixValue[currentCol[uCol]] = -1.0;
			matrixIndex[currentCol[uCol]] = currentRow;
			++currentCol[uCol];
			debugNonZeroCount++;

			matrixValue[currentCol[vCol]] = 1.0;
			matrixIndex[currentCol[vCol]] = currentRow;
			++currentCol[vCol];
			debugNonZeroCount++;

			equationSense[currentRow] = 'G';
			rightHandSide[currentRow] =
				m_nodeDistance + 0.5*(uWidth + vWidth);

			++currentRow;
		}
	}

	// Constraints:
	//   b[v] - x[v] + 1/deg(v) * sum_{u in Adj(v)} x[u] >= 0
	//   b[v] + x[v] - 1/deg(v) * sum_{u in Adj(v)} x[u] >= 0
	if(nBalanced > 0) {
		node v;
		forall_nodes(v,H)
		{
			int bCol = bIndex[v];
			if(bCol == -1)
				continue;
			bCol += balancedOffset;

			int vCol = m_vertexOffset+m_vIndex[v];

			// b[v] - x[v] + 1/deg(v) * sum_{u in Adj(v)} x[u] >= 0
			matrixValue[currentCol[bCol]] = 1.0;
			matrixIndex[currentCol[bCol]] = currentRow;
			++currentCol[bCol];
			debugNonZeroCount++;

			matrixValue[currentCol[vCol]] = -1.0;
			matrixIndex[currentCol[vCol]] = currentRow;
			++currentCol[vCol];
			debugNonZeroCount++;

			double f = 1.0 / v->degree();
			adjEntry adj;
			forall_adj(adj,v) {
				node u = adj->twinNode();
				int uCol = m_vIndex[u];
				uCol += (m_isVirtual[u]) ? m_segmentOffset : m_vertexOffset;

				matrixValue[currentCol[uCol]] = f;
				matrixIndex[currentCol[uCol]] = currentRow;
				++currentCol[uCol];
				debugNonZeroCount++;
			}

			equationSense[currentRow] = 'G';
			rightHandSide[currentRow] = 0.0;

			++currentRow;

			// b[v] + x[v] - 1/deg(v) * sum_{u in Adj(v)} x[u] >= 0
			matrixValue[currentCol[bCol]] = 1.0;
			matrixIndex[currentCol[bCol]] = currentRow;
			++currentCol[bCol];
			debugNonZeroCount++;

			matrixValue[currentCol[vCol]] = 1.0;
			matrixIndex[currentCol[vCol]] = currentRow;
			++currentCol[vCol];
			debugNonZeroCount++;

			f = -1.0 / v->degree();
			forall_adj(adj,v) {
				node u = adj->twinNode();
				int uCol = m_vIndex[u];
				uCol += (m_isVirtual[u]) ? m_segmentOffset : m_vertexOffset;

				matrixValue[currentCol[uCol]] = f;
				matrixIndex[currentCol[uCol]] = currentRow;
				++currentCol[uCol];
				debugNonZeroCount++;
			}

			equationSense[currentRow] = 'G';
			rightHandSide[currentRow] = 0.0;

			++currentRow;
		}
	}

	OGDF_ASSERT(nNonZeroes == debugNonZeroCount);

	// lower and upper bounds
	Array<double> lowerBound(nCols);
	Array<double> upperBound(nCols);

	for(i = 0; i < nCols; ++i) {
		lowerBound[i] = 0.0;
		upperBound[i] = solver.infinity();
	}


	// objective function
	Array<double> obj(nCols);
	forall_edges(e,H) {
		i = eIndex[e];
		if(i >= 0) {
			// edge segments connecting to a vertical segment
			// (i.e. the original edge is represented by at least
			// three edges in H) get a special weight; all others
			// have weight 1.0
			int sz = H.chain(H.origEdge(e)).size();
			if(sz >= 2) {
				node uOrig = H.origNode(e->source());
				node vOrig = H.origNode(e->target());
				if((uOrig && uOrig->outdeg() == 1) || (vOrig && vOrig->indeg() == 1))
					obj[i] = 1.2*m_weightSegments;
				else
					obj[i] = (sz >= 3) ? m_weightSegments : 1.0;
			} else
				obj[i] = 1.0;

			if(m_isVirtual[e->source()] == false && e->source()->degree() == 1)
				obj[i] += m_weightBalancing;
			if(m_isVirtual[e->target()] == false && e->target()->degree() == 1)
				obj[i] += m_weightBalancing;
		}
	}

	for(i = nEdges; i < balancedOffset; ++i)
		obj[i] = 0.0; // all x_v and x_s do not contribute

	for(; i < m_clusterLeftOffset; ++i)
		obj[i] = m_weightBalancing;

	for(; i < m_clusterRightOffset; ++i)
		obj[i] = -m_weightClusters;

	for(; i < nCols; ++i)
		obj[i] = +m_weightClusters;

	// solve LP
	double optimum;
	Array<double> x(nCols);

	LPSolver::Status status =
		solver.optimize(LPSolver::lpMinimize, obj,
		matrixBegin, matrixCount, matrixIndex, matrixValue,
		rightHandSide, equationSense,
		lowerBound, upperBound,
		optimum, x);

	OGDF_ASSERT(status == LPSolver::lpOptimal);
	int checkResult = checkSolution(matrixBegin, matrixCount, matrixIndex, matrixValue,
		rightHandSide, equationSense, lowerBound, upperBound, x);
	OGDF_ASSERT(checkResult == -1);

	// assign x coordinates
	node v;
	forall_nodes(v,H)
	{
		ExtendedNestingGraph::NodeType t = H.type(v);
		if(t == ExtendedNestingGraph::ntNode || t == ExtendedNestingGraph::ntDummy)
		{
			if(m_isVirtual[v])
				AGC.x(v) = x[m_segmentOffset + m_vIndex[v]];
			else
				AGC.x(v) = x[m_vertexOffset  + m_vIndex[v]];
		}
	}

	forall_clusters(c,CG.getOriginalClusterGraph())
	{
		int i = m_cIndex[CG.copy(c)];
		OGDF_ASSERT(i >= 0);

		AGC.setClusterLeftRight(c,
			x[m_clusterLeftOffset+i], x[m_clusterRightOffset+i]);
	}


	// clean-up
	m_isVirtual.init();
	m_vIndex   .init();
	m_cIndex   .init();
}


void OptimalHierarchyClusterLayout::buildLayerList(
	const LHTreeNode *vNode,
	List<Tuple2<int,double> > &L)
{
	if(vNode->isCompound())
	{
		int i = m_cIndex[vNode->originalCluster()];

		if(i >= 0)
			L.pushBack(Tuple2<int,double>(m_clusterLeftOffset + i, 0));

		for(int j = 0; j < vNode->numberOfChildren(); ++j)
			buildLayerList(vNode->child(j), L);

		if(i >= 0)
			L.pushBack(Tuple2<int,double>(m_clusterRightOffset + i, 0));

	} else {
		node v = vNode->getNode();
		ExtendedNestingGraph::NodeType t = m_pH->type(v);

		if(t != ExtendedNestingGraph::ntClusterBottom &&
			t != ExtendedNestingGraph::ntClusterTop)
		{
			int i = m_vIndex[v];
			i += (m_isVirtual[v]) ? m_segmentOffset : m_vertexOffset;

			L.pushBack(Tuple2<int,double>(i, m_pACGC->getWidth(v)));
		}
	}
}


//---------------------------------------------------------
// Compute y-coordinates for cluster graphs
//---------------------------------------------------------
void OptimalHierarchyClusterLayout::computeYCoordinates(
	const ExtendedNestingGraph& H,
	ClusterGraphCopyAttributes &AGC)
{
	const int k = H.numberOfLayers();

	Array<double> y(k);

	double prevHeight = 0;
	for(int i = 0; i < k; ++i)
	{
		// Compute height of layer i and dy (if required)
		double height = 0;               // height[i]
		double dy     = m_layerDistance; // dy[i]

		Stack<const LHTreeNode*> S;
		S.push(H.layerHierarchyTree(i));
		while(!S.empty())
		{
			const LHTreeNode *vNode = S.pop();

			if(vNode->isCompound()) {
				for(int j = vNode->numberOfChildren()-1; j >= 0; --j)
					S.push(vNode->child(j));

			} else {
				node v = vNode->getNode();

				if(H.type(v) == ExtendedNestingGraph::ntNode) {
					double h = AGC.getHeight(v);
					if(h > height)
						height = h;
				}

				if(m_fixedLayerDistance == false) {
					edge e;
					forall_adj_edges(e,v) {
						node w = e->source();
						if(w != v) {
							double dwv = fabs(AGC.x(w) - AGC.x(v)) / 3.0;
							if(dwv > dy)
								dy = dwv;
						}
					}
				}
			}
		}

		if(dy > 10*m_layerDistance)
			dy = 10*m_layerDistance;

		y[i] = (i > 0) ? y[i-1] + dy +0.5*(height+prevHeight) : 0.5*height;

		prevHeight = height;
	}

	// set y-ccordinates of nodes
	node v;
	forall_nodes(v,H) {
		ExtendedNestingGraph::NodeType t = H.type(v);
		if(t == ExtendedNestingGraph::ntNode || t == ExtendedNestingGraph::ntDummy)
		{
			AGC.y(v) = y[H.rank(v)];
		}
	}

	// set y-coordinates of clusters
	const ClusterGraph &CG = H.getOriginalClusterGraph();

	const double yUnit = m_layerDistance;

	cluster c;
	forall_postOrderClusters(c,CG)
	{
		if(c == CG.rootCluster())
			continue;

		double contentMin = DBL_MAX;
		double contentMax = DBL_MIN;

		ListConstIterator<node> itV;
		for(itV = c->nBegin(); itV.valid(); ++itV) {
			node v = H.copy(*itV);
			double t = AGC.y(v) - 0.5*AGC.getHeight(v);
			double b = AGC.y(v) + 0.5*AGC.getHeight(v);
			OGDF_ASSERT(t <= b);

			if(t < contentMin) contentMin = t;
			if(b > contentMax) contentMax = b;
		}

		ListConstIterator<cluster> itC;
		for(itC = c->cBegin(); itC.valid(); ++itC) {
			double t = AGC.top(*itC);
			double b = AGC.bottom(*itC);
			OGDF_ASSERT(t <= b);

			if(t < contentMin) contentMin = t;
			if(b > contentMax) contentMax = b;
		}

		double currentTop    = y[H.rank(H.top(c))];
		double currentBottom = y[H.rank(H.bottom(c))];

		if(contentMin != DBL_MAX)
		{
			int kt = int(((contentMin-m_layerDistance) - currentTop) / yUnit);
			if(kt >= 1)
				currentTop += kt*yUnit;

			int kb = int((currentBottom - (contentMax+m_layerDistance)) / yUnit);
			if(kb >= 1)
				currentBottom -= kb*yUnit;
		}

		AGC.setClusterTopBottom(c, currentTop, currentBottom);
	}
}

}

#endif
