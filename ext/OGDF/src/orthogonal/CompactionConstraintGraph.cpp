/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class CompactionConstraintGraphBase.
 *
 * Represents base class for CompactionConstraintGraph<ATYPE>
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


#include <ogdf/orthogonal/CompactionConstraintGraph.h>
#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/extended_graph_alg.h>

//#define foutput

namespace ogdf {


// constructor for orthogonal representation
// builds constraint graph with basic arcs
CompactionConstraintGraphBase::CompactionConstraintGraphBase(
	const OrthoRep &OR,
	const PlanRep &PG,
	OrthoDir arcDir,
	int costGen,
	int costAssoc,
	bool align
) :
	m_pathNode(OR),
	m_edgeToBasicArc(OR,0)
{
	OGDF_ASSERT(&PG == &(const Graph &)OR);

	m_path        .init(*this);
	m_cost        .init(*this,costAssoc);
	m_type        .init(*this,cetBasicArc);
	m_verticalGen .init(PG, false);
	m_verticalArc .init(*this, false);
	m_border      .init(*this, false);
	m_alignmentArc.init(*this, false);
	m_pathToEdge  .init(*this, 0);
	m_originalEdge.init(*this, 0);

	m_pPR       = &PG;//only used in detecting cage visibility arcs
	m_pOR       = &OR;
	m_align     = align;
	m_arcDir    = arcDir;
	m_oppArcDir = OR.oppDir(arcDir);
	m_edgeCost[Graph::generalization] = costGen;
	m_edgeCost[Graph::association   ] = costAssoc;

	edge e;
	forall_edges(e, PG)
	{
		if ((PG.typeOf(e) == Graph::generalization) && (!PG.isExpansionEdge(e)))
			m_verticalGen[e] = true;
	}//for

	insertPathVertices(PG);
	insertBasicArcs(PG);
}


// insert vertex for each segment
void CompactionConstraintGraphBase::insertPathVertices(const PlanRep &PG)
{
	NodeArray<node> genOpposite(PG,0);

	node v;
	forall_nodes(v,PG)
	{
		const OrthoRep::VertexInfoUML *vi = m_pOR->cageInfo(v);
		if (vi == 0 || PG.typeOf(v) == Graph::generalizationMerger) continue;

		adjEntry adjGen = vi->m_side[m_arcDir   ].m_adjGen;
		adjEntry adjOpp = vi->m_side[m_oppArcDir].m_adjGen;
		if (adjGen != 0 && adjOpp != 0)
		{
			node v1 = adjGen->theNode();
			node v2 = adjOpp->theNode();
			genOpposite[genOpposite[v1] = v2] = v1;
		}
	}

	//TODO: hier abfragen, ob Kantensegment einer Originalkante
	//und diese hat Multikantenstatus => man muss sich die Abstaende am Rand merken
	//und auf alle Segmente anwenden

	NodeArray<bool> visited(PG,false);

	forall_nodes(v,PG)
	{
		if (!visited[v]) {
			node pathVertex = newNode();

			dfsInsertPathVertex(v, pathVertex, visited, genOpposite);

			//test for multi sep
			//if exact two nodes in path, edge between is original edge segment,
			//save this to recall the minsep for all segments later over original
			//muss man hier originaledge als rueckgabe reintun???
			if ((m_path[pathVertex].size() == 2) && m_pathToEdge[pathVertex])
			{

			}//if original segment
			else m_pathToEdge[pathVertex] = 0;
		}
	}
}


// insert all graph vertices into segment pathVertex
void CompactionConstraintGraphBase::dfsInsertPathVertex(
	node v,
	node pathVertex,
	NodeArray<bool> &visited,
	const NodeArray<node> &genOpposite)
{
	visited[v] = true;
	m_path[pathVertex].pushFront(v);
	m_pathNode[v] = pathVertex;

	adjEntry adj;
	forall_adj(adj,v)
	{
		OrthoDir dirAdj = m_pOR->direction(adj);
		OGDF_ASSERT(dirAdj != odUndefined);

		if (dirAdj != m_arcDir && dirAdj != m_oppArcDir) {
			//for multiedges, only useful if only one edge considered on path
			//maybe zero if no original edge exists
			if (!m_pathToEdge[pathVertex])
			{
				//only reset later for multi edge segments
				m_pathToEdge[pathVertex] = m_pPR->original(adj->theEdge());
				//used for all vertices

			}

			node w = adj->theEdge()->opposite(v);
			if (!visited[w])
				dfsInsertPathVertex(w, pathVertex, visited, genOpposite);
		}
	}

	node w = genOpposite[v];
	if (w != 0 && !visited[w])
		dfsInsertPathVertex(w, pathVertex, visited, genOpposite);
}



//
// insert an arc for each edge with direction m_arcDir
void CompactionConstraintGraphBase::insertBasicArcs(const PlanRep &PG)
{
	const Graph &G = *m_pOR;

	node v;
	forall_nodes(v,G)
	{
		node start = m_pathNode[v];

		adjEntry adj;
		forall_adj(adj,v) {
			if (m_pOR->direction(adj) == m_arcDir) {
				edge e = newEdge(start, m_pathNode[adj->theEdge()->opposite(v)]);
				m_edgeToBasicArc[adj] = e;

				m_cost[e] = m_edgeCost[PG.typeOf(adj->theEdge())];

				//try to pull nodes up in hierarchies
				if ( (PG.typeOf(adj->theEdge()) == Graph::generalization) &&
					(PG.typeOf(adj->theEdge()->target()) == Graph::generalizationExpander) &&
					!(PG.isExpansionEdge(adj->theEdge()))
					)
				{
					if (m_align)
					{
						//got to be higher than vertexarccost*doublebendfactor
						m_cost[e] = 4000*m_cost[e]; //use parameter later corresponding
						m_alignmentArc[e] = true;
					}//if align
					//to compconsgraph::doublebendfactor
					else m_cost[e] = 2*m_cost[e];
				}

				//set generalization type
				if (verticalGen(adj->theEdge())) m_verticalArc[e] = true;
				//set onborder
				if (PG.isDegreeExpansionEdge(adj->theEdge()))
				{
					edge borderE = adj->theEdge();
					node v1 = borderE->source();
					node v2 = borderE->target();
					m_border[e] = ((v1->degree()>2) && (v2->degree()>2) ? 2 : 1);
				}

			}
		}
	}
}


// embeds constraint graph such that all sources and sinks lie in a common
// face
void CompactionConstraintGraphBase::embed()
{
	NodeArray<bool> onExternal(*this,false);
	const CombinatorialEmbedding &E = *m_pOR;
	face fExternal = E.externalFace();

	adjEntry adj;
	forall_face_adj(adj,fExternal)
		onExternal[m_pathNode[adj->theNode()]] = true;

	// compute lists of sources and sinks
	SList<node> sources, sinks;

	node v;
	forall_nodes(v,*this) {
		if (onExternal[v]) {
			if (v->indeg() == 0)
				sources.pushBack(v);
			if (v->outdeg() == 0)
				sinks.pushBack(v);
		}
	}

#ifdef foutput
writeGML("c:\\temp\\CCgraphbefore.gml", onExternal);
#endif

	// determine super source and super sink
	node s,t;
	if (sources.size() > 1)
	{
		s = newNode();
		SListIterator<node> it;
		for (it = sources.begin(); it.valid(); it++)
			newEdge(s,*it);
	}
	else
		s = sources.front();

	if (sinks.size() > 1)
	{
		t = newNode();
		SListIterator<node> it;
		for (it = sinks.begin(); it.valid(); it++)
			newEdge(*it,t);
	}
	else
		t = sinks.front();

	edge st = newEdge(s,t);

#ifdef foutput
writeGML(String("c:\\temp\\CCgraph%d.gml"));//,randomNumber(0,20)));
writeGML("c:\\temp\\CClastgraph.gml");
#endif

	bool isPlanar = planarEmbed(*this);
	if (!isPlanar) OGDF_THROW(AlgorithmFailureException);


	delEdge(st);
	if (sources.size() > 1)
		delNode(s);
	if (sinks.size() > 1)
		delNode(t);
}


// computes topological numbering on the segments of the constraint graph.
// Usage: If used on the basic (and vertex size) arcs, the numbering can be
//   used in order to serve as sorting criteria for respecting the given
//   embedding, e.g., when computing visibility arcs and allowing edges
//   with length 0.
void CompactionConstraintGraphBase::computeTopologicalSegmentNum(
	NodeArray<int> &topNum)
{
	NodeArray<int> indeg(*this);
	StackPure<node> sources;

	node v;
	forall_nodes(v,*this) {
		topNum[v] = 0;
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

			if (topNum[w] < topNum[v] + 1)
				topNum[w] = topNum[v] + 1;

			if (--indeg[w] == 0)
				sources.push(w);
		}
	}
}



class BucketFirstIndex : public BucketFunc<Tuple2<node,node> >
{
public:
	int getBucket(const Tuple2<node,node> &t) {
		return t.x1()->index();
	}
};

class BucketSecondIndex : public BucketFunc<Tuple2<node,node> >
{
public:
	int getBucket(const Tuple2<node,node> &t) {
		return t.x2()->index();
	}
};


// remove "arcs" from visibArcs which we already have in the constraint graph
// (as basic arcs)
void CompactionConstraintGraphBase::removeRedundantVisibArcs(
	SListPure<Tuple2<node,node> > &visibArcs)
{
	// bucket sort list of all edges
	SListPure<edge> all;
	allEdges(all);
	parallelFreeSort(*this,all);

	// bucket sort visibArcs
	BucketFirstIndex bucketSrc;
	visibArcs.bucketSort(0,maxNodeIndex(),bucketSrc);

	BucketSecondIndex bucketTgt;
	visibArcs.bucketSort(0,maxNodeIndex(),bucketTgt);

	// now, in both lists, arcs are sorted by increasing target index,
	// and arcs with the same target index by increasing source index.
	SListConstIterator<edge> itAll = all.begin();
	SListIterator<Tuple2<node,node> > it, itNext, itPrev;

	// for each arc in visibArcs, we check if it is also contained in list all
	for(it = visibArcs.begin(); it.valid(); it = itNext)
	{
		// required since we delete from the list we traverse
		itNext = it.succ();
		int i = (*it).x1()->index();
		int j = (*it).x2()->index();

		// skip all arcs with smaller target index
		while(itAll.valid() && (*itAll)->target()->index() < j)
			++itAll;

		// no more arcs => no more duplicates, so return
		if (!itAll.valid()) break;

		// if target index is j, we also skip all arcs with target index i
		// and source index smaller than i
		while(itAll.valid() && (*itAll)->target()->index() == j && (*itAll)->source()->index() < i)
			++itAll;

		// no more arcs => no more duplicates, so return
		if (!itAll.valid()) break;

		// if (i,j) is already present, we delete it from visibArcs
		if ((*itAll)->source()->index() == i &&
			(*itAll)->target()->index() == j)
		{
			//visibArcs.del(it);
			if (itPrev.valid())
				visibArcs.delSucc(itPrev);
			else
				visibArcs.popFront();
		} else
			itPrev = it;
	}//for visibArcs

	//****************************CHECK for
	//special treatment for cage visibility
	//two cases: input node cage: just compare arbitrary node
	//           merger cage: check first if there are mergers
	itPrev = 0;
	for(it = visibArcs.begin(); it.valid(); it = itNext)
	{

		itNext = it.succ();

		OGDF_ASSERT(!(m_path[(*it).x1()].empty()) && !(m_path[(*it).x1()].empty()));

		node boundRepresentant1 = m_path[(*it).x1()].front();
		node boundRepresentant2 = m_path[(*it).x2()].front();
		node en1 = m_pPR->expandedNode(boundRepresentant1);
		node en2 = m_pPR->expandedNode(boundRepresentant2);
		//do not allow visibility constraints in fixed cages
		//due to non-planarity with middle position constraints

		if ( ( en1 && en2 ) && ( en1 == en2) )
		{
			if (itPrev.valid()) visibArcs.delSucc(itPrev);
			else visibArcs.popFront();
		}
		else
		{
			//check if its a genmergerspanning vis arc, merge cases later
			node firstn = 0, secondn = 0;
			SListIterator< node > itn;
			for (itn = m_path[(*it).x1()].begin(); itn.valid(); itn++)
			{
				node en = m_pPR->expandedNode(*itn);
				if (!en) continue;
				if (!(m_pPR->typeOf(*itn) == Graph::generalizationExpander)) continue;
				else {firstn = en; break;}
			}//for
			for (itn = m_path[(*it).x2()].begin(); itn.valid(); itn++)
			{
				node en = m_pPR->expandedNode(*itn);
				if (!en) continue;
				if (!(m_pPR->typeOf(*itn) == Graph::generalizationExpander)) continue;
				else {secondn = en; break;}
			}//for
			if ((firstn && secondn) && (firstn == secondn))
			{
				if (itPrev.valid()) visibArcs.delSucc(itPrev);
				else visibArcs.popFront();
			}
			else itPrev = it;
		}
	}//for visibArcs

}



// output in gml-format with special edge colouring
// arcs with cost 0 are green, other arcs red
void CompactionConstraintGraphBase::writeGML(const char *filename) const
{
	ofstream os(filename);
	writeGML(os);
}
void CompactionConstraintGraphBase::writeGML(const char *filename, NodeArray<bool> one) const
{
	ofstream os(filename);
	writeGML(os, one);
}

void CompactionConstraintGraphBase::writeGML(ostream &os) const
{
	const Graph &G = *this;

	NodeArray<int> id(*this);
	int nextId = 0;

	os.setf(ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::CompactionConstraintGraphBase::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	node v;
	forall_nodes(v,G) {
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";

		os << "    graphics [\n";
		os << "      x 0.0\n";
		os << "      y 0.0\n";
		os << "      w 30.0\n";
		os << "      h 30.0\n";

		os << "      fill \"#FFFF00\"\n";
		os << "    ]\n"; // graphics

		os << "  ]\n"; // node
	}


	edge e;
	forall_edges(e,G) {
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		// nice idea to use the edge length as edge labels, but Graphwin-
		// garbage is not able to show the graph (thinks it has to set
		// the y-coordinates of all nodes to NAN)
#if 0
		os << "    label \"";
		writeLength(os,e);
		os << "\"\n";
#endif

		os << "    graphics [\n";

		os << "      type \"line\"\n";
		os << "      arrow \"last\"\n";
		switch(m_type[e])
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
		case cetReducibleArc: // rose
			os << "      fill \"#FF00FF\"\n";
			break;
		case cetFixToZeroArc: //violett
			os << "      fill \"#3F00FF\"\n";
			break;
		OGDF_NODEFAULT
		}

		os << "    ]\n"; // graphics

#if 0
		os << "    LabelGraphics [\n";
		os << "      type \"text\"\n";
		os << "      fill \"#000000\"\n";
		os << "      anchor \"w\"\n";
		os << "    ]\n";
#endif

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}//writegml

void CompactionConstraintGraphBase::writeGML(ostream &os, NodeArray<bool> one) const
{
	const Graph &G = *this;

	NodeArray<int> id(*this);
	int nextId = 0;

	os.setf(ios::showpoint);
	os.precision(10);

	os << "Creator \"ogdf::CompactionConstraintGraphBase::writeGML\"\n";
	os << "graph [\n";
	os << "  directed 1\n";

	node v;
	forall_nodes(v,G) {
		os << "  node [\n";

		os << "    id " << (id[v] = nextId++) << "\n";

		os << "    graphics [\n";
		os << "      x 0.0\n";
		os << "      y 0.0\n";
		os << "      w 30.0\n";
		os << "      h 30.0\n";
		if ((one[v])) {
			os << "      fill \"#FF0F0F\"\n";
		} else {
			os << "      fill \"#FFFF00\"\n";
		}
		os << "    ]\n"; // graphics

		os << "  ]\n"; // node
	}


	edge e;
	forall_edges(e,G) {
		os << "  edge [\n";

		os << "    source " << id[e->source()] << "\n";
		os << "    target " << id[e->target()] << "\n";

		// nice idea to use the edge length as edge labels, but Graphwin-
		// garbage is not able to show the graph (thinks it has to set
		// the y-coordinates of all nodes to NAN)
#if 0
		os << "    label \"";
		writeLength(os,e);
		os << "\"\n";
#endif

		os << "    graphics [\n";

		os << "      type \"line\"\n";
		os << "      arrow \"last\"\n";
		switch(m_type[e])
		{
		case cetBasicArc: // red
			os << "      fill \"#FF0000\"\n";
			break;
		case cetVertexSizeArc: // blue
			os << "      fill \"#0000FF\"\n";
			break;
		case cetVisibilityArc: // green
			os <<       "fill \"#00FF00\"\n";
			break;
		case cetReducibleArc: // rose
			os << "      fill \"#FF00FF\"\n";
			break;
		case cetFixToZeroArc: //violett
			os << "      fill \"#3F00FF\"\n";
			break;
		OGDF_NODEFAULT
		}

		os << "    ]\n"; // graphics

#if 0
		os << "    LabelGraphics [\n";
		os << "      type \"text\"\n";
		os << "      fill \"#000000\"\n";
		os << "      anchor \"w\"\n";
		os << "    ]\n";
#endif

		os << "  ]\n"; // edge
	}

	os << "]\n"; // graph
}


} // end namespace ogdf
