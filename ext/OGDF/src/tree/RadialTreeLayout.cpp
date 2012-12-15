/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Linear time layout algorithm for free trees (RadialTreeLayout).
 *
 * Based on chapter 3.1.1 Radial Drawings of Graph Drawing by
 * Di Battista, Eades, Tamassia, Tollis
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


#include <ogdf/tree/RadialTreeLayout.h>


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Queue.h>
#include <ogdf/basic/Stack.h>
#include <ogdf/basic/Math.h>


namespace ogdf {


RadialTreeLayout::RadialTreeLayout()
	:m_levelDistance(50),
	 m_connectedComponentDistance(50),
	 m_selectRoot(rootIsCenter)
{ }


RadialTreeLayout::RadialTreeLayout(const RadialTreeLayout &tl)
	:m_levelDistance(tl.m_levelDistance),
	 m_connectedComponentDistance(tl.m_connectedComponentDistance),
	 m_selectRoot(tl.m_selectRoot)
{ }


RadialTreeLayout::~RadialTreeLayout()
{ }


RadialTreeLayout &RadialTreeLayout::operator=(const RadialTreeLayout &tl)
{
	m_levelDistance              = tl.m_levelDistance;
	m_connectedComponentDistance = tl.m_connectedComponentDistance;
	m_selectRoot                 = tl.m_selectRoot;

	return *this;
}


void RadialTreeLayout::call(GraphAttributes &AG)
{
	const Graph &tree = AG.constGraph();
	if(tree.numberOfNodes() == 0) return;

	if (!isTree(tree))
		OGDF_THROW_PARAM(PreconditionViolatedException, pvcForest);

	OGDF_ASSERT(m_levelDistance > 0);

	// determine root of tree (m_root)
	FindRoot(tree);

	// compute m_level[v], m_parent[v], m_leaves[v], m_numLevels
	ComputeLevels(tree);

	// computes diameter of each node
	ComputeDiameters(AG);

	// computes m_angle[v] and m_wedge[v]
	ComputeAngles(tree);

	// computes final coordinates of nodes
	ComputeCoordinates(AG);
}


void RadialTreeLayout::FindRoot(const Graph &G)
{
	node v;

	switch(m_selectRoot) {
		case rootIsSource:
			forall_nodes(v,G)
				if(v->indeg() == 0)
					m_root = v;
			break;

		case rootIsSink:
			forall_nodes(v,G)
				if(v->outdeg() == 0)
					m_root = v;
			break;

		case rootIsCenter:
			{
				NodeArray<int> degree(G);
				Queue<node> leaves;

				forall_nodes(v,G) {
					if((degree[v] = v->degree()) == 1)
						leaves.append(v);
				}

				while(!leaves.empty()) {
					v = leaves.pop();

					adjEntry adj;
					forall_adj(adj, v) {
						node u = adj->twinNode();
						if(--degree[u] == 1)
							leaves.append(u);
					}
				}

				m_root = v;
			}
			break;
	}
}


void RadialTreeLayout::ComputeLevels(const Graph &G)
{
	m_parent.init(G);
	m_level.init(G);
	m_leaves.init(G,0);

	Queue<node> Q;
	Stack<node> S;

	Q.append(m_root);
	m_parent[m_root] = 0;
	m_level [m_root] = 0;

	int maxLevel = 0;

	while(!Q.empty())
	{
		node v = Q.pop();
		S.push(v);
		int levelV = m_level[v];

		bool isLeaf = true;

		adjEntry adj;
		forall_adj(adj, v) {
			node u = adj->twinNode();
			if(u == m_parent[v])
				continue;

			isLeaf = false;

			Q.append(u);
			m_parent[u] = v;
			m_level[u] = maxLevel = levelV+1;
		}

		// number of leaves in a subtree rooted at a leaf is 1
		if(isLeaf)
			m_leaves[v] = 1.0 / levelV;
	}

	m_numLevels = maxLevel + 1;

	// compute number of leaves in subtree (already computed for leaves)
	while(!S.empty())
	{
		node v = S.pop();
		node p = m_parent[v];

		if(p != 0)
			m_leaves[p] += m_leaves[v];
	}
}


void RadialTreeLayout::ComputeDiameters(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();

	m_diameter.init(G);
	m_nodes.init(m_numLevels);
	m_width.init(m_numLevels);
	m_width.fill(0);

	node v;
	forall_nodes(v,G)
	{
		int i = m_level[v];
		m_nodes[i].pushBack(v);

		double w = AG.width(v);
		double h = AG.height(v);

		m_diameter[v] = sqrt(w*w+h*h);

		double m = max(w, h);
		m = max(m, sqrt(w*w+h*h));

		if(m_diameter[v] > m_width[i])
			m_width[i] = m_diameter[v];
	}
}



void RadialTreeLayout::ComputeAngles(const Graph &G)
{
	m_angle.init(G);
	m_wedge.init(G);
	m_radius.init(m_numLevels);
	m_grouping.init(G);

	Queue<node> Q;
	NodeArray<double> restWeight(G);

	Q.append(m_root);
	m_angle[m_root] = 0;
	m_wedge[m_root] = 2*Math::pi;
	m_radius[0] = 0;

	//Grouping grouping;
	//double D, W;

	NodeArray<double> D(G), W(G);

	int iProcessed = 0;

	while(!Q.empty())
	{
		node v = Q.pop();
		node p = m_parent[v];

		// nothing to do if v is a leaf
		if(p != 0 && v->degree() == 1)
			continue;

		int i = m_level[v];
		if(i+1 > iProcessed) {
			m_radius[i+1] = m_radius[i] + 0.5*(m_width[i+1]+m_width[i]) + m_levelDistance;

			ComputeGrouping(i);

			SListConstIterator<node> it;
			for(it = m_nodes[i].begin(); it.valid(); ++it)
			{
				node w = *it;

				m_grouping[w].computeAdd(D[w],W[w]);

				double deltaL = 0.0;
				ListConstIterator<Group> itG;
				for(itG = m_grouping[w].begin(); itG.valid(); ++itG)
				{
					const Group &g = *itG;
					if(g.m_leafGroup)
						continue;

					double deltaLG;
					double weightedAdd = W[w] / g.m_sumW * g.add();

					deltaLG = 2 * W[w] / m_leaves[g.leftVertex()] * g.m_leftAdd - weightedAdd;
					if(deltaLG > deltaL)
						deltaL = deltaLG;

					deltaLG = 2 * W[w] / m_leaves[g.rightVertex()] * g.m_rightAdd - weightedAdd;
					if(deltaLG > deltaL)
						deltaL = deltaLG;
				}

				double r = (deltaL + D[w]) / m_wedge[w];
				if(r > m_radius[i+1])
					m_radius[i+1] = r;
			}

			// ********
			/*deltaL = (m_radius[i+1] * 2*Math::pi) - D;

			double offset = 0;
			for(itG = grouping.begin(); itG.valid(); ++itG)
			{
				const Group &g = *itG;

				SListConstIterator<node> itV;
				for(itV = g.m_nodes.begin(); itV.valid(); ++itV)
				{
					node v = *itV;

					double s = m_diameter[v] + m_levelDistance;
					if(g.m_leafGroup == false)
						s += m_leaves[v] / g.m_sumW * g.add() + m_leaves[v] / W * deltaL;

					double desiredWedge = s / m_radius[i+1];

					double allowedWedge = 2 * acos(m_radius[i] / m_radius[i+1]);
					m_wedge[v] = min(desiredWedge,allowedWedge);

					m_angle[v] = offset + 0.5*desiredWedge;
					offset += desiredWedge;

					Q.append(v);
				}
			}
*/

			//*************************
/*			SListConstIterator<node> it;
			for(it = m_nodes[i].begin(); it.valid(); ++it)
			{
				node w = *it;

				// compute weight of all non-leaves
				double weight = 0.0;

				adjEntry adjSon;
				forall_adj(adjSon,w)
				{
					node u = adjSon->twinNode();
					if(u == m_parent[w])
						continue;
					if(u->degree() > 1)
						weight += m_leaves[u];
				}

				restWeight[w] = weight;

				double D = (w->degree() - 1) * m_levelDistance;

				forall_adj(adjSon,w)
				{
					node u = adjSon->twinNode();
					if(u == m_parent[w])
						continue;

					D += m_diameter[u];
				}

				double r = D / m_wedge[w];
				if(r > m_radius[i+1])
					m_radius[i+1] = r;
			}*/

			iProcessed = i+1;
		}


		double deltaL = (m_radius[i+1] * m_wedge[v]) - D[v];
		double offset = m_angle[v] - 0.5*m_wedge[v];

		ListConstIterator<Group> itG;
		for(itG = m_grouping[v].begin(); itG.valid(); ++itG)
		{
			const Group &g = *itG;

			SListConstIterator<node> it;
			for(it = g.m_nodes.begin(); it.valid(); ++it)
			{
				node u = *it;

				double s = m_diameter[u] + m_levelDistance;
				if(g.m_leafGroup == false)
					s += m_leaves[u] / g.m_sumW * g.add() + m_leaves[u] / W[v] * deltaL;

				double desiredWedge = s / m_radius[i+1];

				double allowedWedge = 2 * acos(m_radius[i] / m_radius[i+1]);
				m_wedge[u] = min(desiredWedge,allowedWedge);

				m_angle[u] = offset + 0.5*desiredWedge;
				offset += desiredWedge;

				Q.append(u);
			}
		}


/*
		double restWedge = m_wedge[v];
		adjEntry adj;
		forall_adj(adj,v)
		{
			node u = adj->twinNode();
			if(u == m_parent[v])
				continue;

			m_wedge[u] = (m_diameter[u] + m_levelDistance) / m_radius[i+1];
			restWedge -= m_wedge[u];
		}

		double offset = m_angle[v] - 0.5*m_wedge[v];

		adj = v->firstAdj();
		adjEntry adjStop;
		if(p != 0) {
			while(adj->twinNode() != p)
				adj = adj->cyclicSucc();
			adjStop = adj;
			adj = adj->cyclicSucc();
		} else {
			adjStop = adj;
		}

		do
		{
			node u = adj->twinNode();

			double desiredWedge;

			if(u->degree() == 1) {
				desiredWedge = m_wedge[u];

			} else {
				desiredWedge = m_wedge[u] + m_leaves[u] / restWeight[v] * restWedge;

				double allowedWedge = 2 * acos(m_radius[i] / m_radius[i+1]);
				m_wedge[u] = min(desiredWedge,allowedWedge);
			}

			m_angle[u] = offset + 0.5*desiredWedge;
			offset += desiredWedge;

			Q.append(u);

			adj = adj->cyclicSucc();
		} while(adj != adjStop);*/
	}

	m_outerRadius = m_radius[m_numLevels-1] + 0.5*m_width[m_numLevels-1];
}

void RadialTreeLayout::Grouping::computeAdd(double &D, double &W)
{
	D = W = 0;

	ListIterator<Group> it;
	for(it = begin(); it.valid(); ++it)
	{
		Group &g = *it;

		D += g.m_sumD;

		if(g.m_leafGroup == true)
			continue;

		W += g.m_sumW;

		ListIterator<Group> itL;

		itL = it.pred();
		if(itL.valid() == false) {
			g.m_leftAdd = 0.0;
		} else {
			ListIterator<Group> itR = itL.pred();
			if(itR.valid() == false)
				g.m_leftAdd = (*itL).m_sumD;
			else
				g.m_leftAdd = (*itL).m_sumD * g.m_sumW / (*itR).m_sumW;
		}

		itL = it.succ();
		if(itL.valid() == false) {
			g.m_leftAdd = 0.0;
		} else {
			ListIterator<Group> itR = itL.succ();
			if(itR.valid() == false)
				g.m_leftAdd = (*itL).m_sumD;
			else
				g.m_leftAdd = (*itL).m_sumD * g.m_sumW / (*itR).m_sumW;
		}
	}
}

// compute grouping for sons of nodes on level i
void RadialTreeLayout::ComputeGrouping(int i)
{
	SListConstIterator<node> it;
	for(it = m_nodes[i].begin(); it.valid(); ++it)
	{
		node v = *it;
		node p = m_parent[v];

		Grouping &grouping = m_grouping[v];
		ListIterator<Group> currentGroup;

		adjEntry adj = v->firstAdj();
		adjEntry adjStop;
		if(p != 0) {
			while(adj->twinNode() != p)
				adj = adj->cyclicSucc();
			adjStop = adj;
			adj = adj->cyclicSucc();
		} else {
			adjStop = adj;
		}

		do
		{
			node u = adj->twinNode();

			if(!currentGroup.valid() || (*currentGroup).isSameType(u) == false)
			{
				currentGroup = grouping.pushBack(Group(this,u));

			} else {
				(*currentGroup).append(u);
			}

			adj = adj->cyclicSucc();
		} while(adj != adjStop);
	}
}


void RadialTreeLayout::ComputeCoordinates(GraphAttributes &AG)
{
	const Graph &G = AG.constGraph();

	//double mx = m_outerRadius + 0.5*m_connectedComponentDistance;
	//double my = mx;

	node v;
	forall_nodes(v,G) {
		double r = m_radius[m_level[v]];
		double alpha = m_angle[v];

		AG.x(v) = r * cos(alpha);
		AG.y(v) = r * sin(alpha);
	}

	AG.clearAllBends();
}

}
