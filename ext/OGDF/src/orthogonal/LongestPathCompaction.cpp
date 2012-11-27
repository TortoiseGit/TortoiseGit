/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements constructive and improvement heurisitcs for
 * longest-paths based compaction of orthogonal drawings
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


#include <ogdf/orthogonal/LongestPathCompaction.h>
#include <ogdf/orthogonal/CompactionConstraintGraph.h>
#include <ogdf/basic/Stack.h>
#include <ogdf/basic/GridLayout.h>
#include <limits.h>


namespace ogdf {


// constructor
LongestPathCompaction::LongestPathCompaction(bool tighten,
	int maxImprovementSteps)
{
	m_tighten             = tighten;
	m_maxImprovementSteps = maxImprovementSteps;
}


// constructive heuristics for orthogonal representation OR
void LongestPathCompaction::constructiveHeuristics(
	PlanRepUML &PG,
	OrthoRep &OR,
	const RoutingChannel<int> &rc,
	GridLayoutMapped &drawing)
{
	OGDF_ASSERT(OR.isOrientated());

	// x-coordinates of vertical segments
	CompactionConstraintGraph<int> Dx(OR, PG, odEast, rc.separation());
	Dx.insertVertexSizeArcs(PG, drawing.width(), rc);

	NodeArray<int> xDx(Dx.getGraph(), 0);
	computeCoords(Dx, xDx);

	// y-coordinates of horizontal segments
	CompactionConstraintGraph<int> Dy(OR, PG, odNorth, rc.separation());
	Dy.insertVertexSizeArcs(PG, drawing.height(), rc);

	NodeArray<int> yDy(Dy.getGraph(), 0);
	computeCoords(Dy, yDy);

	// final coordinates of vertices
	node v;
	forall_nodes(v,PG) {
		drawing.x(v) = xDx[Dx.pathNodeOf(v)];
		drawing.y(v) = yDy[Dy.pathNodeOf(v)];
	}
}


// improvement heuristics for orthogonal drawing
void LongestPathCompaction::improvementHeuristics(
	PlanRepUML &PG,
	OrthoRep &OR,
	const RoutingChannel<int> &rc,
	GridLayoutMapped &drawing)
{
	OGDF_ASSERT(OR.isOrientated());

	int costs, lastCosts;
	int steps = 0, maxSteps = m_maxImprovementSteps;
	if (maxSteps == 0) maxSteps = INT_MAX;

	// OPTIMIZATION POTENTIAL:
	// update constraint graphs "incrementally" by only re-inserting
	// visibility arcs
	costs = 0;
	do {
		lastCosts = costs;
		++steps;

		// x-coordinates of vertical segments
		CompactionConstraintGraph<int> Dx(OR, PG, odEast, rc.separation());
		Dx.insertVertexSizeArcs(PG, drawing.width(), rc);
		Dx.insertVisibilityArcs(PG, drawing.x(),drawing.y());

		NodeArray<int> xDx(Dx.getGraph(), 0);
		computeCoords(Dx, xDx);

		// final x-coordinates of vertices
		node v;
		forall_nodes(v,PG) {
			drawing.x(v) = xDx[Dx.pathNodeOf(v)];
		}


		// y-coordinates of horizontal segments
		CompactionConstraintGraph<int> Dy(OR, PG, odNorth, rc.separation());
		Dy.insertVertexSizeArcs(PG, drawing.height(), rc);
		Dy.insertVisibilityArcs(PG, drawing.y(),drawing.x());

		NodeArray<int> yDy(Dy.getGraph(), 0);
		computeCoords(Dy, yDy);

		// final y-coordinates of vertices
		forall_nodes(v,PG) {
			drawing.y(v) = yDy[Dy.pathNodeOf(v)];
		}

		costs = Dx.computeTotalCosts(xDx) + Dy.computeTotalCosts(yDy);

	} while (steps < maxSteps && (steps == 1 || costs < lastCosts));
}



// computes coordinates pos of horizontal (resp. vertical) segments by
// computing longest paths in the constraint graph D
void LongestPathCompaction::computeCoords(
	const CompactionConstraintGraph<int> &D,
	NodeArray<int> &pos)
{
	const Graph &Gd = D.getGraph();

	// compute a first ranking with usual longest paths
	applyLongestPaths(D,pos);


	if (m_tighten == true)
	{
		// improve cost of ranking by moving pseudo-components
		moveComponents(D,pos);


		// find node with minimal position
		SListConstIterator<node> it = m_pseudoSources.begin();
		int min = pos[*it];
		for(++it; it.valid(); ++it) {
			if (pos[*it] < min)
				min = pos[*it];
		}

		// move all nodes such that node with minimum position has position 0
		node v;
		forall_nodes(v,Gd)
			pos[v] -= min;

	}

	// free resources
	m_pseudoSources.clear();
	m_component.init();
}


void LongestPathCompaction::applyLongestPaths(
	const CompactionConstraintGraph<int> &D,
	NodeArray<int> &pos)
{
	const Graph &Gd = D.getGraph();

	m_component.init(Gd);

	NodeArray<int> indeg(Gd);
	StackPure<node> sources;

	node v;
	forall_nodes(v,Gd) {
		indeg[v] = v->indeg();
		if(indeg[v] == 0)
			sources.push(v);
	}

	while(!sources.empty())
	{
		node v = sources.pop();

		int predComp = -1; // means "unset"
		bool isPseudoSource = true;

		edge e;
		forall_adj_edges(e,v) {
			if(e->source() != v) {
				// incoming edge
				if (D.cost(e) > 0) {
					isPseudoSource = false;
					node w = e->source();
					// is tight?
					if (pos[w] + D.length(e) == pos[v]) {
						if (predComp == -1)
							predComp = m_component[w];
						else if (predComp != m_component[w])
							predComp = 0; // means "vertex is in no pseudo-comp.
					}
				}

			} else {
				// outgoing edge
				node w = e->target();

				if (pos[w] < pos[v] + D.length(e))
					pos[w] = pos[v] + D.length(e);

				if (--indeg[w] == 0)
					sources.push(w);
			}
		}

		if (predComp == -1)
			predComp = 0;

		if( isPseudoSource) {
			m_pseudoSources.pushFront(v);
			m_component[v] = m_pseudoSources.size();
		} else {
			m_component[v] = predComp;
		}
	}
}



void LongestPathCompaction::moveComponents(
	const CompactionConstraintGraph<int> &D,
	NodeArray<int> &pos)
{
	const Graph &Gd = D.getGraph();

	// compute for each component the list of nodes contained
	Array<SListPure<node> > nodesInComp(1,m_pseudoSources.size());

	node v;
	forall_nodes(v,Gd) {
		if (m_component[v] > 0)
		nodesInComp[m_component[v]].pushBack(v);
	}


	// iterate over all pseudo-sources in reverse topological order
	SListConstIterator<node> it;
	for(it = m_pseudoSources.begin(); it.valid(); ++it)
	{
		node v = *it;
		int c = m_component[v];

		// list of outgoing/incoming edges of pseudo-component C(v)
		SListPure<edge> outCompV, inCompV;

		//cout << "component " << c << endl;
		SListConstIterator<node> itW;
		for(itW = nodesInComp[c].begin(); itW.valid(); ++itW)
		{
			node w = *itW;
			//cout << " " << w;
			edge e;
			forall_adj_edges(e,w) {
				if(m_component[e->target()] != c) {
					outCompV.pushBack(e);
				} else if (m_component[e->source()] != c)
					inCompV.pushBack(e);
			}
		}
		//cout << endl;

		if(outCompV.empty())
			continue;

		SListConstIterator<edge> itE = outCompV.begin();
		int costOut = D.cost(*itE);
		int delta = (pos[(*itE)->target()] - pos[(*itE)->source()]) -
						D.length(*itE);

		for(++itE; itE.valid(); ++itE) {
			costOut += D.cost(*itE);
			int d = (pos[(*itE)->target()] - pos[(*itE)->source()]) -
						D.length(*itE);
			if (d < delta)
				delta = d;
		}

		//cout << "  delta = " << delta << ", costOut = " << costOut << endl;

		// if all outgoing edges have cost 0, we wouldn't save any cost!
		if (costOut == 0) continue;

		// move component up by delta; this shortens all outgoing edges and
		// enlarges all incoming edges (which have cost 0)
		for(itW = nodesInComp[c].begin(); itW.valid(); ++itW)
			pos[*itW] += delta;
	}

}



/*
// computes coordinates pos of horizontal (resp. vertical) segments by
// computing longest paths in the constraint graph D
void LongestPathCompaction::computeCoords(
	CompactionConstraintGraph &D,
	NodeArray<double> &pos)
{
	const Graph &Gd = D.getGraph();

	NodeArray<int> indeg(Gd);
	StackPure<node> sources;

	node v;
	forall_nodes(v,Gd) {
		indeg[v] = v->indeg();
		if(indeg[v] == 0)
			sources.push(v);
	}

	while(!sources.empty())
	{
		node v = sources.pop();

		edge e;
		forall_adj_edges(e,v) {
			if(e->source() != v) continue;

			node w = e->target();

			if (pos[w] < pos[v] + D.length(e))
				pos[w] = pos[v] + D.length(e);

			if (--indeg[w] == 0)
				sources.push(w);
		}
	}
}
*/


} // end namespace ogdf

