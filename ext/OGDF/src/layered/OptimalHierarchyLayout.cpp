/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of the optimal
//   third phase of the sugiyama algorithm
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


#include <ogdf/layered/OptimalHierarchyLayout.h>
#include <ogdf/layered/Hierarchy.h>
#include <ogdf/lpsolver/LPSolver.h>


#ifdef OGDF_LP_SOLVER

namespace ogdf {


//---------------------------------------------------------
// Constructor
//---------------------------------------------------------
OptimalHierarchyLayout::OptimalHierarchyLayout()
{
	m_nodeDistance       = 3;
	m_layerDistance      = 3;
	m_fixedLayerDistance = false;
	m_weightSegments     = 2.0;
	m_weightBalancing    = 0.1;
}


//---------------------------------------------------------
// Copy Constructor
//---------------------------------------------------------
OptimalHierarchyLayout::OptimalHierarchyLayout(const OptimalHierarchyLayout &ohl)
{
	m_nodeDistance       = ohl.nodeDistance();
	m_layerDistance      = ohl.layerDistance();
	m_fixedLayerDistance = ohl.fixedLayerDistance();
	m_weightSegments     = ohl.weightSegments();
	m_weightBalancing    = ohl.weightBalancing();
}


//---------------------------------------------------------
// Assignment Operator
//---------------------------------------------------------
OptimalHierarchyLayout &OptimalHierarchyLayout::operator=(const OptimalHierarchyLayout &ohl)
{
	m_nodeDistance       = ohl.nodeDistance();
	m_layerDistance      = ohl.layerDistance();
	m_fixedLayerDistance = ohl.fixedLayerDistance();
	m_weightSegments     = ohl.weightSegments();
	m_weightBalancing    = ohl.weightBalancing();

	return *this;
}


//---------------------------------------------------------
// Call for Graphs
//---------------------------------------------------------
void OptimalHierarchyLayout::doCall(const Hierarchy& H,GraphCopyAttributes &AGC)
{
	// trivial cases
	const GraphCopy &GC = H;
	const int n = GC.numberOfNodes();

	if(n == 0)
		return; // nothing to do

	if(n == 1) {
		node v = GC.firstNode();
		AGC.x(v) = 0;
		AGC.y(v) = 0;
		return;
	}

	// actual computation
	computeXCoordinates(H,AGC);
	computeYCoordinates(H,AGC);
}


//---------------------------------------------------------
// Compute x-coordinates (LP-based approach) (for graphs)
//---------------------------------------------------------
void OptimalHierarchyLayout::computeXCoordinates(
	const Hierarchy& H,
	GraphCopyAttributes &AGC)
{
	const GraphCopy &GC = H;
	const int k = H.size();

	//
	// preprocessing: determine nodes that are considered as virtual
	//
	NodeArray<bool> isVirtual(GC);

	int i;
	for(i = 0; i < k; ++i)
	{
		const Level &L = H[i];
		int last = -1;
		for(int j = 0; j < L.size(); ++j)
		{
			node v = L[j];

			if(H.isLongEdgeDummy(v) == true) {
				isVirtual[v] = true;

				node u = v->firstAdj()->theEdge()->target();
				if(u == v) u = v->lastAdj()->theEdge()->target();

				if(H.isLongEdgeDummy(u) == true) {
					int down = H.pos(u);
					if(last != -1 && last > down) {
						isVirtual[v] = false;
					} else {
						last = down;
					}
				}
			} else
				isVirtual[v] = false;
		}
	}

	//
	// determine variables of LP
	//
	int nSegments     = 0;	// number of vertical segments
	int nRealVertices = 0;	// number of real vertices
	int nEdges        = 0;	// number of edges not in vertical segments
	int nBalanced     = 0;	// number of real vertices with deg > 1 for which
							// balancing constraints may be applied

	NodeArray<int> vIndex(GC,-1);	// for real node: index of x[v]
									// for dummy: index of corresponding segment
	NodeArray<int> bIndex(GC,-1);	// (relative) index of b[v]
	EdgeArray<int> eIndex(GC,-1);	// for edge not in vertical segment:
									//   its index
	Array<int> count(GC.numberOfEdges());	// counts the number of dummy vertices
											// in corresponding segment that are not at
											// position 0

	for(i = 0; i < k; ++i)
	{
		const Level &L = H[i];
		for(int j = 0; j < L.size(); ++j) {
			node v = L[j];
			if(isVirtual[v] == true)
				continue;

			// we've found a real vertex
			vIndex[v] = nRealVertices++;
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

				if(isVirtual[w] == false)
					continue;

				// we've found a vertical segment
				count[nSegments] = 0;
				do {
					vIndex[w] = nSegments;
					const int high = H[H.rank(w)].high();
					if(high > 0) {
						if (H.pos(w) == 0 || H.pos(w) == high)
							++count[nSegments];
						else
							count[nSegments] += 2;
					}

					// next edge / dummy in segment
					e = e->adjTarget()->cyclicSucc()->theEdge();
					w = e->target();
				} while(isVirtual[w]);

				// edge following vertical segment
				eIndex[e] = nEdges++;

				++nSegments;
			}
		}
	}

	// assignment of variables to matrix columns
	//   d_e              0, ..., nEdges-1
	//   x_v   vertexOffset, ..., vertexOffset+nRealVertices-1
	//   x_s  segmentOffset, ..., segmentOffset+nSegments-1
	//   b_v balancedOffset, ..., balancedOffset+nBalanced-1
	LPSolver solver;

	if(m_weightBalancing <= 0.0)
		nBalanced = 0; // no balancing

	const int nCols = nEdges + nRealVertices + nSegments + nBalanced;
	const int nRows = 2*nEdges + GC.numberOfNodes() - k + 2*nBalanced;

	const int vertexOffset   = nEdges;
	const int segmentOffset  = nEdges + nRealVertices;
	const int balancedOffset = segmentOffset + nSegments;

	// allocation of matrix
	Array<int> matrixBegin(nCols);
	Array<int> matrixCount(nCols);

	int nNonZeroes = 0;
	for(i = 0; i < nEdges; ++i) {
		matrixBegin[i] = nNonZeroes;
		nNonZeroes += (matrixCount[i] = 2);
	}

	for(int jj = 0; jj < k; ++jj) {
		const Level &L = H[jj];
		for(int j = 0; j < L.size(); ++j) {
			node v = L[j];

			if(isVirtual[v] == false) {
				i = vertexOffset + vIndex[v];
				matrixBegin[i] = nNonZeroes;

				int high = H[H.rank(v)].high();
				int cstrSep;
				if(high == 0)
					cstrSep = 0;
				else if (H.pos(v) == 0 || H.pos(v) == high)
					cstrSep = 1;
				else
					cstrSep = 2;

				int count =  cstrSep + 2*v->degree();
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
				nNonZeroes += matrixCount[i];

			} else if (nBalanced > 0) {
				i = vIndex[v];
				adjEntry adj;
				forall_adj(adj,v) {
					node w = adj->twinNode();
					if(bIndex[w] != -1)
						count[i] += 2;
				}
			}
		}
	}

	for(i = 0; i < nSegments; ++i) {
		matrixBegin[segmentOffset+i] = nNonZeroes;
		nNonZeroes += (matrixCount[segmentOffset+i] = count[i] + 4);
	}

	for(i = 0; i < nBalanced; ++i) {
		matrixBegin[balancedOffset+i] = nNonZeroes;
		nNonZeroes += (matrixCount[balancedOffset+i] = 2);
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
	forall_edges(e,GC)
	{
		int dCol = eIndex[e];
		if(dCol >= 0) {
			node u = e->source();
			int uCol = vIndex[u];
			uCol += (isVirtual[u]) ? segmentOffset : vertexOffset;

			node v = e->target();
			int vCol = vIndex[v];
			vCol += (isVirtual[v]) ? segmentOffset : vertexOffset;

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
		const Level &L = H[i];
		for(int j = 1; j < L.size(); ++j)
		{
			node u = L[j-1];
			int uCol = vIndex[u];
			uCol += (isVirtual[u]) ? segmentOffset : vertexOffset;

			node v = L[j];
			int vCol = vIndex[v];
			vCol += (isVirtual[v]) ? segmentOffset : vertexOffset;

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
				m_nodeDistance + 0.5*(AGC.getWidth(v)+AGC.getWidth(u));

			++currentRow;
		}
	}

	// Constraints:
	//   b[v] - x[v] + 1/deg(v) * sum_{u in Adj(v)} x[u] >= 0
	//   b[v] + x[v] - 1/deg(v) * sum_{u in Adj(v)} x[u] >= 0
	if(nBalanced > 0) {
		for(i = 0; i < k; ++i)
		{
			const Level &L = H[i];
			for(int j = 0; j < L.size(); ++j)
			{
				node v = L[j];
				int bCol = bIndex[v];
				if(bCol == -1)
					continue;
				bCol += balancedOffset;

				int vCol = vertexOffset+vIndex[v];

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
					int uCol = vIndex[u];
					uCol += (isVirtual[u]) ? segmentOffset : vertexOffset;

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
					int uCol = vIndex[u];
					uCol += (isVirtual[u]) ? segmentOffset : vertexOffset;

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
	forall_edges(e,GC) {
		i = eIndex[e];
		if(i >= 0) {
			// edge segments connecting to a vertical segment
			// (i.e. the original edge is represented by at least
			// three edges in GC) get a special weight; all others
			// have weight 1.0
			obj[i] = (GC.chain(GC.original(e)).size() >= 3) ? m_weightSegments : 1.0;
			if(isVirtual[e->source()] == false && e->source()->degree() == 1)
				obj[i] += m_weightBalancing;
			if(isVirtual[e->target()] == false && e->target()->degree() == 1)
				obj[i] += m_weightBalancing;
		}
	}

	for(i = nEdges; i < balancedOffset; ++i)
		obj[i] = 0.0; // all x_v and x_s do not contribute

	for(; i < nCols; ++i)
		obj[i] = m_weightBalancing;


	// output problem
	/*ofstream os("c:\\work\\GDE\\out.txt");
	os << "nRows = " << nRows << "\n";
	os << "nCols = " << nCols << "\n";
	os << "nNonZeroes = " << nNonZeroes << "\n";
	os << "\nmatrixBegin, matrixCount:\n";
	for(i = 0; i < nCols; ++i)
		os << " [" << i << "]  " << matrixBegin[i] << ", " << matrixCount[i] << "\n";
	os << "\nmatrixIndex, matrixValue:\n";
	for(i = 0; i < nNonZeroes; ++i)
		os << " [" << i << "]  " << matrixIndex[i] << ", " << matrixValue[i] << "\n";
	os << "\nequationSense, rightHandSide:\n";
	for(i = 0; i < nRows; ++i)
		os << " [" << i << "]  " << equationSense[i] << ", " << rightHandSide[i] << "\n";

	os.flush();*/


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

	/*os << "\nx\n";
	for(i = 0; i < nCols; ++i)
		os << " [" << i << "]  " << x[i] << "\n";

	os.close();*/

	// assign x coordinates
	node v;
	forall_nodes(v,GC) {
		if(isVirtual[v])
			AGC.x(v) = x[segmentOffset+vIndex[v]];
		else
			AGC.x(v) = x[vertexOffset+vIndex[v]];
	}
}


//---------------------------------------------------------
// Compute y-coordinates (for graphs)
//---------------------------------------------------------
void OptimalHierarchyLayout::computeYCoordinates(
	const Hierarchy& H,
	GraphCopyAttributes &AGC)
{
	const int k = H.size();
	int i;

	// compute height of each layer
	Array<double> height(0,k-1,0.0);

	for(i = 0; i < k; ++i) {
		const Level &L = H[i];
		for(int j = 0; j < L.size(); ++j) {
			double h = AGC.getHeight(L[j]);
			if(h > height[i])
				height[i] = h;
		}
	}


	// assign y-coordinates
	double yPos = 0.5 * height[0];

	for(i = 0; ; ++i)
	{
		const Level &L = H[i];
		for(int j = 0; j < L.size(); ++j)
			AGC.y(L[j]) = yPos;

		if(i == k-1)
			break;

		double dy = m_layerDistance;

		if(m_fixedLayerDistance == false)
		{
			for(int j = 0; j < L.size(); ++j) {
				node v = L[j];
				edge e;
				forall_adj_edges(e,v) {
					node w = e->target();
					if(w != v) {
						double dvw = fabs(AGC.x(v)-AGC.x(w)) / 3.0;
						if(dvw > dy)
							dy = dvw;
					}
				}
			}

			if(dy > 10*m_layerDistance)
				dy = 10*m_layerDistance;
		}

		yPos += dy + 0.5 * (height[i] + height[i+1]);
	}
}

}

#endif
