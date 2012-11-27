/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class GalaxyMultilevelBuilder.
 *
 * \author Martin Gronemann
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

#include "GalaxyMultilevel.h"
#include <algorithm>
#include "FastUtils.h"
#include <ogdf/basic/Array.h>

namespace ogdf {


void GalaxyMultilevelBuilder::computeSystemMass()
{
	node v = 0;
	forall_nodes(v, *m_pGraph)
	{
		m_nodeState[v].sysMass = (*m_pNodeInfo)[v].mass;
		m_nodeState[v].label = 0;
		m_nodeState[v].lastVisitor = v;
	}

	forall_nodes(v, *m_pGraph)
	{
		adjEntry adj;
		forall_adj(adj, v)
		{
			m_nodeState[v].sysMass += (*m_pNodeInfo)[adj->twinNode()].mass;
		}

		if (v->degree()==1)
			m_nodeState[v].sysMass *= m_pGraph->numberOfNodes() ;
	}
}


void swap(ogdf::GalaxyMultilevelBuilder::NodeOrderInfo& a, ogdf::GalaxyMultilevelBuilder::NodeOrderInfo& b)
{
	ogdf::GalaxyMultilevelBuilder::NodeOrderInfo t = a;
	a = b;
	b = t;
}


void GalaxyMultilevelBuilder::sortNodesBySystemMass()
{
	int i = 0;
	node v = 0;
	m_pRandomSet = new RandomNodeSet(*m_pGraph);
	for (i=0;i<m_pGraph->numberOfNodes();i++)
	{
		v = m_pRandomSet->chooseNode();
		m_pRandomSet->removeNode(v);
		m_nodeMassOrder[i].theNode = v;
	}

	delete m_pRandomSet;
	std::sort(m_nodeMassOrder, m_nodeMassOrder+(m_pGraph->numberOfNodes()), NodeMassComparer( m_nodeState ));
}


void GalaxyMultilevelBuilder::labelSystem(node u, node v, int d, float df)
{
	if (d>0)
	{
		adjEntry adj;
		forall_adj(adj, v)
		{
			node w = adj->twinNode();
			// this node may have been labeled before but its closer to the current sun
			if (m_nodeState[w].label < d)
			{
				float currDistFromSun = (*m_pEdgeInfo)[adj->theEdge()].length + df /*+ (*m_pNodeInfo)[w].radius*/;
				// check if we relabeling by a new sun
				if (m_nodeState[w].lastVisitor != u)
				{
					// maybe this node has never been labeled
					m_nodeState[w].lastVisitor = u;
					m_nodeState[w].edgeLengthFromSun = currDistFromSun;
				}
				// finally relabel it
				m_nodeState[w].edgeLengthFromSun = min(m_nodeState[w].edgeLengthFromSun, currDistFromSun);
				m_nodeState[w].label = d;
				labelSystem(u, w, d-1, currDistFromSun /*+(*m_pNodeInfo)[w].radius*/);
			}
		}
	}
}


void GalaxyMultilevelBuilder::labelSystem()
{
	m_sunNodeList.clear();
	node v = 0;
	forall_nodes(v, *m_pGraph)
	{
		m_nodeState[v].sysMass = 0;
		m_nodeState[v].label = 0;
		m_nodeState[v].lastVisitor = v;
	}

	for (int i=0; i < m_pGraph->numberOfNodes(); i++)
	{
		v = m_nodeMassOrder[i].theNode;
		if (m_nodeState[v].label == 0)
		{
			m_sunNodeList.pushBack(v);
			m_nodeState[v].label = (m_dist+1);
			m_nodeState[v].edgeLengthFromSun = 0.0;//(*m_pNodeInfo)[v].radius;
			labelSystem(v, v, m_dist, m_nodeState[v].edgeLengthFromSun);
		}
	}
}


GalaxyMultilevel* GalaxyMultilevelBuilder::build(GalaxyMultilevel* pMultiLevel)
{
	m_dist = 2;
	m_pGraph = pMultiLevel->m_pGraph;
	m_pNodeInfo = pMultiLevel->m_pNodeInfo;
	m_pEdgeInfo = pMultiLevel->m_pEdgeInfo;
	m_nodeMassOrder = (NodeOrderInfo*)MALLOC_16(sizeof(NodeOrderInfo)*m_pGraph->numberOfNodes());
	m_nodeState.init(*m_pGraph);

	this->computeSystemMass();
	this->sortNodesBySystemMass();
	this->labelSystem();
	GalaxyMultilevel* pMultiLevelResult = new GalaxyMultilevel(pMultiLevel);
	this->createResult(pMultiLevelResult);;

	FREE_16(m_nodeMassOrder);

	return pMultiLevelResult;
}


void GalaxyMultilevelBuilder::createResult(GalaxyMultilevel* pMultiLevelResult)
{
	pMultiLevelResult->m_pGraph = new Graph();
	m_pGraphResult = pMultiLevelResult->m_pGraph;

	NodeArray<node> toResultNode(*m_pGraph, 0);
	// create all sun nodes
	for(List<node>::iterator it = m_sunNodeList.begin(); it.valid(); it++)
	{
		node v = *it;
		node vResult = m_pGraphResult->newNode();
		toResultNode[v] = vResult;
	}

	pMultiLevelResult->m_pNodeInfo = new NodeArray<GalaxyMultilevel::LevelNodeInfo>(*m_pGraphResult);
	m_pNodeInfoResult = pMultiLevelResult->m_pNodeInfo;

	// calculate the real system mass. this may not be the same as calculated before
	node u;
	forall_nodes(u, *m_pGraphResult)
	{
		(*m_pNodeInfoResult)[u].radius = 0.0f;
		(*m_pNodeInfoResult)[u].mass = 0.0f;
	}
	forall_nodes(u, *m_pGraph)
	{
		node uSun = m_nodeState[u].lastVisitor;
		node uSunResult = toResultNode[uSun];
		(*m_pNodeInfo)[u].parent = uSunResult;
		(*m_pNodeInfoResult)[uSunResult].mass +=((*m_pNodeInfo)[u].mass);
		(*m_pNodeInfoResult)[uSunResult].radius = max( (*m_pNodeInfoResult)[uSunResult].radius, m_nodeState[u].edgeLengthFromSun);//m_nodeState[u].edgeLengthFromSun;//m_nodeState[u].edgeLengthFromSun;// max( (*m_pNodeInfoResult)[uSunResult].radius, m_nodeState[u].edgeLengthFromSun);
	}

	pMultiLevelResult->m_pEdgeInfo = new EdgeArray<GalaxyMultilevel::LevelEdgeInfo>(*m_pGraphResult);
	m_pEdgeInfoResult = pMultiLevelResult->m_pEdgeInfo;

	edge e;
	forall_edges(e, *m_pGraph)
	{
		node v = e->source();
		node w = e->target();
		node vSun = m_nodeState[v].lastVisitor;
		node wSun = m_nodeState[w].lastVisitor;
		if (vSun != wSun)
		{
			node vSunResult = toResultNode[vSun];
			node wSunResult = toResultNode[wSun];
			edge eResult = m_pGraphResult->newEdge(vSunResult, wSunResult);
			(*m_pEdgeInfoResult)[eResult].length = m_nodeState[v].edgeLengthFromSun + (*m_pEdgeInfo)[e].length + m_nodeState[w].edgeLengthFromSun;
		}
	}

	// make fast parallel free
	NodeArray<node> lastVisit(*m_pGraphResult, 0);
	node v;
	forall_nodes(v, *m_pGraphResult)
	{
		if (v->degree()>1)
		{
			adjEntry adj = v->firstAdj();
			do{
				node w = adj->twinNode();
				edge e = adj->theEdge();
				adj = adj->cyclicSucc();
				if (lastVisit[w] ==v)
					m_pGraphResult->delEdge(e);
				else
					lastVisit[w] = v;
			} while (adj !=v->firstAdj());
		}
	}
}

} // end of namespace

