/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class GalaxyMultilevelBuilder.
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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_GALAXY_MULTILEVEL_H
#define OGDF_GALAXY_MULTILEVEL_H

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/simple_graph_alg.h>
#include "ArrayGraph.h"
#include "FastUtils.h"
#include <algorithm>

namespace ogdf {

class GalaxyMultilevel
{
public:
	typedef List<Tuple2< node, int > > NearSunList;

	struct LevelNodeInfo
	{
		float mass;
		float radius;
		node parent;
		NearSunList nearSuns;
	};

	struct LevelEdgeInfo
	{
		float length;
	};

	GalaxyMultilevel(Graph* pGraph)
	{
		m_pFinerMultiLevel = 0;
		m_pCoarserMultiLevel = 0;
		m_pGraph = pGraph;
		m_pNodeInfo = new NodeArray<LevelNodeInfo>(*m_pGraph);
		m_pEdgeInfo = new EdgeArray<LevelEdgeInfo>(*m_pGraph);
		node v;
		forall_nodes(v, *m_pGraph)
		{
			(*m_pNodeInfo)[v].mass = 1.0;
		}
		levelNumber = 0;
	}

	GalaxyMultilevel(GalaxyMultilevel* prev)
	{
		m_pCoarserMultiLevel = 0;
		m_pFinerMultiLevel = prev;
		m_pFinerMultiLevel->m_pCoarserMultiLevel = this;
		m_pGraph = 0;
		m_pNodeInfo = 0;
		levelNumber = prev->levelNumber + 1;
	}

	~GalaxyMultilevel() { }

	GalaxyMultilevel* m_pFinerMultiLevel;
	GalaxyMultilevel* m_pCoarserMultiLevel;
	Graph* m_pGraph;
	NodeArray<LevelNodeInfo>* m_pNodeInfo;
	EdgeArray<LevelEdgeInfo>* m_pEdgeInfo;
	int levelNumber;
};


class GalaxyMultilevelBuilder
{
public:
	struct LevelNodeState
	{
		node lastVisitor;
		double sysMass;
		int label;
		float edgeLengthFromSun;
	};

	struct NodeOrderInfo
	{
		node theNode;
	};

	GalaxyMultilevel* build(GalaxyMultilevel* pMultiLevel);

private:
	void computeSystemMass();
	void sortNodesBySystemMass();
	void createResult(GalaxyMultilevel* pMultiLevelResult);
	void labelSystem(node u, node v, int d, float df);
	void labelSystem();
	Graph* m_pGraph;
	Graph* m_pGraphResult;
	List<node> m_sunNodeList;
	List<edge> m_interSystemEdges;
	NodeArray<GalaxyMultilevel::LevelNodeInfo>* m_pNodeInfo;
	EdgeArray<GalaxyMultilevel::LevelEdgeInfo>* m_pEdgeInfo;
	NodeArray<GalaxyMultilevel::LevelNodeInfo>* m_pNodeInfoResult;
	EdgeArray<GalaxyMultilevel::LevelEdgeInfo>* m_pEdgeInfoResult;
	NodeArray<LevelNodeState> m_nodeState;
	NodeOrderInfo* m_nodeMassOrder;
	RandomNodeSet* m_pRandomSet;
	int m_dist;
};


class NodeMassComparer
{
public:
	NodeMassComparer(const NodeArray< GalaxyMultilevelBuilder::LevelNodeState>& nodeState) : m_nodeState(nodeState) { }

	// used for std::sort
	inline bool operator()(const GalaxyMultilevelBuilder::NodeOrderInfo& a, const GalaxyMultilevelBuilder::NodeOrderInfo& b) const
	{
		return m_nodeState[a.theNode].sysMass < m_nodeState[b.theNode].sysMass;
	}

private:
	const NodeArray< GalaxyMultilevelBuilder::LevelNodeState >& m_nodeState;
};

} // end of namespace ogdf

#endif
