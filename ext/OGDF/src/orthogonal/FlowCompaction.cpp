/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements constructive and improvement heurisitcs for
 * comapction applying computation of min-cost flow in the
 * dual of the constraint graph
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


#include <ogdf/orthogonal/FlowCompaction.h>
#include <ogdf/orthogonal/CompactionConstraintGraph.h>
#include <ogdf/graphalg/MinCostFlowReinelt.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/planarity/PlanRepUML.h>
#include <limits.h>


//#define foutputRC
//#define foutputMD


namespace ogdf {


// output in gml-format with special edge colouring
// arcs with cost 0 are green, other arcs red
void printCCGx(const char *filename,
	const CompactionConstraintGraph<int> &D,
	const GridLayoutMapped &drawing);

// output in gml-format with special edge colouring
// arcs with cost 0 are green, other arcs red
void printCCGy(const char *filename,
	const CompactionConstraintGraph<int> &D,
	const GridLayoutMapped &drawing);


void writeGridDrawing(const char *name, PlanRep &PG, GridLayoutMapped &drawing)
{
	ofstream os(name);

	node v;
	forall_nodes(v,PG) {
		os << v->index() << ": " << drawing.x(v) << ", " << drawing.y(v) << endl;
	}
}



// constructor
FlowCompaction::FlowCompaction(int maxImprovementSteps,
	int costGen,
	int costAssoc)
{
	m_maxImprovementSteps = maxImprovementSteps;
	m_costGen   = costGen;
	m_costAssoc = costAssoc;
	m_cageExpense = true;
	m_numGenSteps = 3; //number of improvement steps for generalizations only + 1
	m_scalingSteps = 0;
	m_align = false;
}


// constructive heuristics for orthogonal representation OR
void FlowCompaction::constructiveHeuristics(
	PlanRep &PG,
	OrthoRep &OR,
	const RoutingChannel<int> &rc,
	GridLayoutMapped &drawing)
{
	OGDF_ASSERT(OR.isOrientated());

	// x-coordinates of vertical segments
	CompactionConstraintGraph<int> Dx(OR, PG, odEast, rc.separation(),
		m_costGen, m_costAssoc, m_align);
	Dx.insertVertexSizeArcs(PG, drawing.width(), rc);

	NodeArray<int> xDx(Dx.getGraph(), 0);
	computeCoords(Dx, xDx);

	// y-coordinates of horizontal segments
	CompactionConstraintGraph<int> Dy(OR, PG, odNorth, rc.separation(),
		m_costGen, m_costAssoc, m_align);
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
void FlowCompaction::improvementHeuristics(
	PlanRep &PG,
	OrthoRep &OR,
	const RoutingChannel<int> &rc,
	GridLayoutMapped &drawing)
{
	OGDF_ASSERT(OR.isOrientated());

	double costs = double(INT_MAX), lastCosts;
	int steps = 0, maxSteps = m_maxImprovementSteps;
	if (maxSteps == 0) maxSteps = INT_MAX;

	// OPTIMIZATION POTENTIAL:
	// update constraint graphs "incrementally" by only re-inserting
	// visibility arcs
	do {
		lastCosts = costs;
		++steps;

		// x-coordinates of vertical segments
		CompactionConstraintGraph<int> Dx(OR, PG, odEast, rc.separation(),
			m_costGen, m_costAssoc, m_align);

		Dx.insertVertexSizeArcs(PG, drawing.width(), rc);
		Dx.insertVisibilityArcs(PG, drawing.x(), drawing.y());

		NodeArray<int> xDx(Dx.getGraph(), 0);

		// set position of segments in order to fix arcs of length 0 in
		// computeCoords()
		node w;
		forall_nodes(w,Dx.getGraph())
		{
			if (!Dx.extraNode(w))
				xDx[w] = drawing.x(Dx.nodesIn(w).front());
			else
				xDx[w] = drawing.x(Dx.extraRep(w)) + Dx.extraOfs(w);
		}


#ifdef foutputRC
		String fileName;
		fileName.sprintf("Dx_%d.gml",steps);
		printCCGx(fileName,Dx,drawing);
#endif

		//first steps: only vertical generalizations
		if ((steps > 0) && (steps<m_numGenSteps)) //first compact cages
			computeCoords(Dx, xDx, true, false, true, true);
		else
			computeCoords(Dx, xDx, true, false, true, false);

		// final x-coordinates of vertices
		node v;
		forall_nodes(v,PG) {
			drawing.x(v) = xDx[Dx.pathNodeOf(v)];
		}


#ifdef foutputRC
		fileName.sprintf("Gx_%d.gml",steps);
		PlanRepUML &PGUml = (PlanRepUML&)PG;
		PGUml.writeGML(fileName,OR,drawing);
#endif

		// y-coordinates of horizontal segments
		CompactionConstraintGraph<int> Dy(OR, PG, odNorth, rc.separation(),
			m_costGen, m_costAssoc, m_align);

		Dy.insertVertexSizeArcs(PG, drawing.height(), rc);
		Dy.insertVisibilityArcs(PG, drawing.y(), drawing.x());


		NodeArray<int> yDy(Dy.getGraph(), 0);

		// set position of segments in order to fix arcs of length 0 in
		// computeCoords()
		forall_nodes(w,Dy.getGraph())
		{
			if (!Dy.extraNode(w)) //maybe only nec in next impro
				yDy[w] = drawing.y(Dy.nodesIn(w).front());
			else
				yDy[w] = drawing.y(Dy.extraRep(w)) + Dy.extraOfs(w);
		}

#ifdef foutputRC
		fileName.sprintf("Dy_%d.gml",steps);
		printCCGy(fileName,Dy,drawing);
#endif

		if ((steps > 0) && (steps<m_numGenSteps)) //first compact cages
			computeCoords(Dy, yDy, true, false, true, true);
		else
			computeCoords(Dy, yDy, true, false, true, false);

		// final y-coordinates of vertices
		forall_nodes(v,PG) {
			drawing.y(v) = yDy[Dy.pathNodeOf(v)];
		}

#ifdef foutputRC
		fileName.sprintf("Gy_%d.gml",steps);
		PGUml.writeGML(fileName,OR,drawing);
#endif

		costs = Dx.computeTotalCosts(xDx) + Dy.computeTotalCosts(yDy);

	} while (steps < maxSteps && (steps < m_numGenSteps || costs < lastCosts));//(steps == 1 || costs < lastCosts));

}


void FlowCompaction::improvementHeuristics(
	PlanRep &PG,
	OrthoRep &OR,
	//const
	MinimumEdgeDistances<int> &minDist,
	GridLayoutMapped &drawing,
	int originalSeparation //test for compaction improvement
	)
{
	OGDF_ASSERT(OR.isOrientated());

	double costs = double(INT_MAX), lastCosts;
	int steps = 0, maxSteps = m_maxImprovementSteps;
	if (maxSteps == 0) maxSteps = INT_MAX;

	// OPTIMIZATION POTENTIAL:
	// update constraint graphs "incrementally" by only re-inserting
	// visibility arcs
	do {
		lastCosts = costs;
		++steps;

		// x-coordinates of vertical segments
		CompactionConstraintGraph<int> Dx(OR, PG, odEast, originalSeparation,
			//minDist.separation(),
			m_costGen, m_costAssoc, m_align);

		Dx.insertVertexSizeArcs(PG, drawing.width(), minDist);
		Dx.insertVisibilityArcs(PG, drawing.x(), drawing.y(), minDist);

#ifdef foutputMD
		String fileName;
		fileName.sprintf("Dx_%d.gml",steps);
		printCCGx(fileName,Dx,drawing);
#endif

		NodeArray<int> xDx(Dx.getGraph(), 0);

		// set position of segments in order to fix arcs of length 0 in
		// computeCoords()
		node w;
		forall_nodes(w,Dx.getGraph())
		{
			if (!Dx.extraNode(w))
				xDx[w] = drawing.x(Dx.nodesIn(w).front());
			else xDx[w] = drawing.x(Dx.extraRep(w)) + Dx.extraOfs(w);
		}

		if ((steps > 0) && (steps<m_numGenSteps)) //first compact cages
			computeCoords(Dx, xDx, true, true, true, true);
		else
			computeCoords(Dx, xDx, true, true, true, false);

		//computeCoords(Dx, xDx, true, true, true);

		// final x-coordinates of vertices
		node v;
		forall_nodes(v,PG) {
			drawing.x(v) = xDx[Dx.pathNodeOf(v)];
		}

#ifdef foutputMD
		fileName.sprintf("Gx_%d.txt",steps);
		writeGridDrawing(fileName, PG, drawing);

		fileName.sprintf("Gx_%d.gml",steps);
		PlanRepUML &PGUml = (PlanRepUML&)PG;
		PGUml.writeGML(fileName,OR,drawing);
#endif

		// y-coordinates of horizontal segments
		CompactionConstraintGraph<int> Dy(OR, PG, odNorth, originalSeparation,
			//minDist.separation(),
			m_costGen, m_costAssoc, m_align);

		Dy.insertVertexSizeArcs(PG, drawing.height(), minDist);
		Dy.insertVisibilityArcs(PG,drawing.y(),drawing.x(),minDist);

		NodeArray<int> yDy(Dy.getGraph(), 0);

		// set position of segments in order to fix arcs of length 0 in
		// computeCoords()
		forall_nodes(w,Dy.getGraph())
		{
			if (!Dy.extraNode(w))
				yDy[w] = drawing.y(Dy.nodesIn(w).front());
			else
				yDy[w] = drawing.y(Dy.extraRep(w)) + Dy.extraOfs(w);
		}

#ifdef foutputMD
		fileName.sprintf("Dy_%d.gml",steps);
		printCCGy(fileName,Dy,drawing);

		fileName.sprintf("c-edges y %d.txt", steps);
		ofstream os(fileName);
		const Graph &Gd = Dy.getGraph();
		edge ee;
		forall_edges(ee,Gd)
			os << (yDy[ee->target()] - yDy[ee->source()]) << "   " <<
				ee->source()->index() << " -> " << ee->target()->index() << endl;
		os.close();
#endif

		//computeCoords(Dy, yDy, true, true, true);
		if ((steps > 0) && (steps < m_numGenSteps)) //first compact cages
			computeCoords(Dy, yDy, true, true, true, true);
		else
			computeCoords(Dy, yDy, true, true, true, false);

		// final y-coordinates of vertices
		forall_nodes(v,PG) {
			drawing.y(v) = yDy[Dy.pathNodeOf(v)];
		}

#ifdef foutputMD
		fileName.sprintf("Gy_%d.txt",steps);
		writeGridDrawing(fileName, PG, drawing);

		fileName.sprintf("Gy_%d.gml",steps);
		PGUml.writeGML(fileName,OR,drawing);
#endif

		costs = Dx.computeTotalCosts(xDx) + Dy.computeTotalCosts(yDy);

		if (steps <= m_scalingSteps) minDist.separation(max(originalSeparation, minDist.separation() / 2));

	} while (steps < maxSteps && (steps < max(m_scalingSteps + 1, m_numGenSteps) || costs < lastCosts));
}



// computes coordinates pos of horizontal (resp. vertical) segments by
// computing a min-cost flow in the dual of the constraint graph D
void FlowCompaction::computeCoords(
	CompactionConstraintGraph<int> &D,
	NodeArray<int> &pos,
	bool fixZeroLength,
	bool fixVertexSize,
	bool improvementHeuristics,
	bool onlyGen //compact only generalizations
	)
{
	Graph &Gd = D.getGraph();

	//D.writeGML("computecoords.gml");

	//
	// embed constraint graph such that all sources and sinks lie
	// in a common face

	D.embed();
	CombinatorialEmbedding E(Gd);

	//
	// construct dual graph
	Graph dual;
	FaceArray<node> dualNode(E);
	m_dualEdge.init(Gd);

	// insert a node in the dual graph for each face in E
	face f;
	forall_faces(f,E) {
		dualNode[f] = dual.newNode();
	}

	// Insert an edge into the dual graph for each edge in Gd
	// The edges are directed from the left face to the right face.
	edge e;
	forall_edges(e,Gd)
	{
		node vLeft  = dualNode[E.rightFace(e->adjTarget())];
		node vRight = dualNode[E.rightFace(e->adjSource())];
		edge eDual  = dual.newEdge(vLeft,vRight);
		m_dualEdge[e] = eDual;
	}


	MinCostFlowReinelt mcf;

	const int infinity = mcf.infinity();

	NodeArray<int> supply(dual,0);
	EdgeArray<int> lowerBound(dual), upperBound(dual,infinity);
	EdgeArray<int> cost(dual);
	m_flow.init(dual);

	forall_edges(e,Gd)
	{
		edge eDual = m_dualEdge[e];

		lowerBound[eDual] = D.length(e);
		cost      [eDual] = D.cost(e);

		// if fixZeroLength is activated, we fix the length of all arcs
		// which have length 0 in the current drawing to 0.
		// This has to be changed if we use special constructs for allowing
		// left or right bends
		int currentLength = pos[e->target()] - pos[e->source()];
		if ((fixZeroLength && currentLength == 0) && (D.typeOf(e) == cetFixToZeroArc))
			lowerBound[eDual] = upperBound[eDual] = 0;
		else if (improvementHeuristics && currentLength < lowerBound[eDual])
			lowerBound[eDual] = currentLength;

		//fix length of alignment edges after some rounds
		//(preliminary use onlyGen setting here)

		if (m_align && improvementHeuristics)
		{
			if (D.alignmentArc(e) && !onlyGen) //use onlyGen instead of extra bool to test
				upperBound[eDual] = currentLength;

		}//if align


		//fix cage boundary edge segments to values smaller than sep
		if ( improvementHeuristics && D.fixOnBorder(e) && (currentLength < D.separation()) )
		{
			if (currentLength < lowerBound[eDual]) lowerBound[eDual] = currentLength;
			//lowerBound[eDual] = test fuer skalierung

			upperBound[eDual] = currentLength;
		}

		//reducible arcs are currently out of play
		//maybe they will be inserted later on for some special purpose,
		//therefore we keep the code
		OGDF_ASSERT(D.typeOf(e) != cetReducibleArc)
		//     if (D.typeOf(e) == cetReducibleArc)
		//         {
		//OGDF_ASSERT(false);
		//           lowerBound[eDual] = min(0, currentLength);
		//           upperBound[eDual] = infinity;
		//         }
		//should we reset median arcs here?

		if (onlyGen)
		{
			//vertexsize sind aber nur innen, border muss noch geaendert werden
			//also expansion!=generalization
			if (!(D.verticalArc(e) || (D.typeOf(e) == cetVertexSizeArc)
				|| D.onBorder(e)) ) {
				lowerBound[eDual] = currentLength;
				upperBound[eDual] = infinity;
			}
		}

	}//forall Gd edges

	if (fixVertexSize) {
		forall_edges(e,Gd) {
			if (D.typeOf(e) == cetVertexSizeArc) {
				edge eDual = m_dualEdge[e];
				upperBound[eDual] = lowerBound[eDual];
			}
		}
	}


	//
	// apply min-cost flow
	if (dual.numberOfNodes() == 1)
	{
		edge eDual;
		forall_edges(eDual,dual)
			m_flow[eDual] = lowerBound[eDual];

	} else {
#ifdef OGDF_DEBUG
		bool feasible =
#endif
			mcf.call(dual,lowerBound,upperBound,cost,supply,m_flow);

		OGDF_ASSERT(feasible);
	}

	//
	// interpret result; set coordinates of segments
	// note: positions are currently not 0-aligned!
	NodeArray<bool> visited(Gd,false);
	dfsAssignPos(visited, pos, Gd.firstNode(), 0);

	// free resources
	m_dualEdge.init();
	m_flow.init();
}


// compute position of nodes in the constraint graph by a DFS-traversel
// if (v,w) is an edge in D and len is the flow on the corresponding dual
// edge, we know that pos[v] + len = pos[w]
void FlowCompaction::dfsAssignPos(
	NodeArray<bool> &visited,
	NodeArray<int> &pos,
	node v,
	int x)
{
	pos[v] = x;
	visited[v] = true;

	edge e;
	forall_adj_edges(e,v) {
		node w = e->opposite(v);
		if (visited[w]) continue;

		if (e->source() == v)
			dfsAssignPos(visited,pos,w,x + m_flow[m_dualEdge[e]]);
		else
			dfsAssignPos(visited,pos,w,x - m_flow[m_dualEdge[e]]);
	}
}



// output in gml-format with special edge colouring
// arcs with cost 0 are green, other arcs red
void writeCcgGML(const CompactionConstraintGraph<int> &D,
	const GraphAttributes &AG,
	const char *filename)
{
	ofstream os(filename);
	const Graph &G = D.getGraph();

	NodeArray<int> id(G);
	int nextId = 0;

	os.setf(ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::writeCcgGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	node v;
	forall_nodes(v,G) {
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";
		os << "    label \"" << v << "\"\n";

		os << "    graphics [\n";
		os << "      x " << AG.x(v) << "\n";
		os << "      y " << AG.y(v) << "\n";
		os << "      w " << AG.width(v) << "\n";
		os << "      h " << AG.height(v) << "\n";
		os << "    ]\n"; // graphics

		os << "  ]\n"; // node
	}


	edge e;
	forall_edges(e,G) {
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		os << "    graphics [\n";

		os << "      type \"line\"\n";
		os << "      arrow \"last\"\n";

		switch(D.typeOf(e))
		{
		case cetBasicArc: // red
			os << "      fill \"#FF0000\"\n";
			break;
		case cetVertexSizeArc: // blue
			os << "      fill \"#0000FF\"\n";
			break;
		case cetVisibilityArc: // green
			os << "      fill \"#00FF00\"\n";
			break;
		case cetReducibleArc: // pink
			os << "      fill \"#FF00FF\"\n";
			break;
		case cetFixToZeroArc: // violet
			os << "      fill \"#AF00FF\"\n";
			break;
		case cetMedianArc: // black
			os << "      fill \"#0F000F\"\n";
			break;
		}

		const DPolyline &dpl = AG.bends(e);
		if (!dpl.empty()) {
			os << "      Line [\n";
			os << "        point [ x " << AG.x(e->source()) << " y " <<
				AG.y(e->source()) << " ]\n";

			ListConstIterator<DPoint> it;
			for(it = dpl.begin(); it.valid(); ++it)
				os << "        point [ x " << (*it).m_x << " y " << (*it).m_y << " ]\n";

			os << "        point [ x " << AG.x(e->target()) << " y " <<
				AG.y(e->target()) << " ]\n";

			os << "      ]\n"; // Line
		}

		os << "    ]\n"; // graphics
		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}

void printCCGx(const char *filename,
	const CompactionConstraintGraph<int> &D,
	const GridLayoutMapped &drawing)
{
	const Graph &Gd = D.getGraph();
	const NodeArray<int> &x = drawing.x();
	const NodeArray<int> &y = drawing.y();

	node v;
	edge e;

	GraphAttributes AG(Gd,GraphAttributes::nodeLabel | GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics);

	forall_nodes(v,Gd)
	{
		if (D.extraNode(v))
		{
			AG.height(v) = 1.0;
			AG.width (v) = 1.0;

			AG.x(v) = drawing.x(D.extraRep(v)) + D.extraOfs(v);

			continue;
		}

		const SListPure<node> &L = D.nodesIn(v);
		if (L.empty())
			continue;//should not happen, extraNode

		node v1 = L.front();
		int minY = y[v1];
		int maxY = y[v1];
		SListConstIterator<node> it;
		for(it = L.begin(); it.valid(); ++it) {
			if (y[*it] < minY) minY = y[*it];
			if (y[*it] > maxY) maxY = y[*it];
		}
		AG.y(v) = 0.5*drawing.toDouble(minY + maxY);
		AG.x(v) = drawing.toDouble(x[v1]);
		AG.height(v) = (maxY != minY) ? drawing.toDouble(maxY - minY) : 0.1;
		AG.width(v) = 1;
	}

	const Graph &G = D.getOrthoRep();
	forall_edges(e,G) {
		edge eD = D.basicArc(e);
		if (eD == 0) continue;

		AG.bends(eD).pushFront(DPoint(AG.x(eD->source()),drawing.toDouble(drawing.y(e->source()))));
		AG.bends(eD).pushBack (DPoint(AG.x(eD->target()),drawing.toDouble(drawing.y(e->source()))));
	}
	writeCcgGML(D,AG,filename);
}


void printCCGy(const char *filename,
	const CompactionConstraintGraph<int> &D,
	const GridLayoutMapped &drawing)
{
	const Graph &Gd = D.getGraph();
	const NodeArray<int> &x = drawing.x();
	const NodeArray<int> &y = drawing.y();

	node v;
	edge e;

	GraphAttributes AG(Gd,GraphAttributes::nodeLabel | GraphAttributes::nodeGraphics | GraphAttributes::edgeGraphics);

	forall_nodes(v,Gd)
	{
		if (D.extraNode(v))
		{
			AG.height(v) = 1.0;
			AG.width (v) = 1.0;

			//AG.x(v) = drawing.x(D.extraRep(v)) + D.extraOfs(v);

			continue;
		}

		const SListPure<node> &L = D.nodesIn(v);
		if (L.empty())
			continue;//should not happen, extraNode

		node v1 = L.front();
		int minX = x[v1];
		int maxX = x[v1];
		SListConstIterator<node> it;
		for(it = L.begin(); it.valid(); ++it) {
			if (x[*it] < minX) minX = x[*it];
			if (x[*it] > maxX) maxX = x[*it];
		}
		AG.x(v) = 0.5*drawing.toDouble(minX + maxX);
		AG.y(v) = drawing.toDouble(y[v1]);
		AG.width(v) = (minX != maxX) ? drawing.toDouble(maxX - minX) : 0.1;
		AG.height(v) = 1;
	}

	const Graph &G = D.getOrthoRep();
	forall_edges(e,G) {
		edge eD = D.basicArc(e);
		if (eD == 0) continue;
		AG.bends(eD).pushFront(DPoint(drawing.toDouble(drawing.x(e->source())),
			AG.y(eD->source())));
		AG.bends(eD).pushBack (DPoint(drawing.toDouble(drawing.x(e->source())),
			AG.y(eD->target())));
	}

	writeCcgGML(D,AG,filename);
}



} // end namespace ogdf

