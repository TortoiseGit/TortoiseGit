/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class FastMultipoleEmbedder.
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

#include <ogdf/energybased/FastMultipoleEmbedder.h>
#include "FastUtils.h"
#include "ArrayGraph.h"
#include "LinearQuadtree.h"
#include "LinearQuadtreeExpansion.h"
#include "FMEThread.h"
#include "GalaxyMultilevel.h"
#include "FMEMultipoleKernel.h"

namespace ogdf {

FastMultipoleEmbedder::FastMultipoleEmbedder()
{
	m_precisionParameter = 5;
	m_defaultEdgeLength = 1.0;
	m_defaultNodeSize = 1.0;
	m_numIterations = 100;
	m_randomize = true;
	m_numberOfThreads = 0;
	m_maxNumberOfThreads = 1; //the only save value
}

FastMultipoleEmbedder::~FastMultipoleEmbedder(void)
{
	// nothing
}

void FastMultipoleEmbedder::initOptions()
{
	m_pOptions->preProcTimeStep = 0.5;			// 0.5
	m_pOptions->preProcMaxNumIterations = 20;	// 20
	m_pOptions->preProcEdgeForceFactor = 0.5;	// 0.5
	m_pOptions->timeStep = 0.25;				// 0.25
	m_pOptions->edgeForceFactor = 1.0;			// 1.00;
	m_pOptions->repForceFactor = 2.0;			// 2.0;
	m_pOptions->stopCritConstSq = 2000400;		// 2000400;
	m_pOptions->stopCritAvgForce = 0.1f;		//
	m_pOptions->minNumIterations = 4;			// 4
	m_pOptions->multipolePrecision = m_precisionParameter;
}

/*
void FastMultipoleEmbedder::call(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	call(G, MLG.getXArray(), MLG.getYArray(), MLG.getWArray(), MLG.getRArray());
}
*/

void FastMultipoleEmbedder::call(const Graph& G, NodeArray<float>& nodeXPosition, NodeArray<float>& nodeYPosition,
								 const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize)
{
	allocate(G.numberOfNodes(), G.numberOfEdges());
	m_pGraph->readFrom(G, nodeXPosition, nodeYPosition, edgeLength, nodeSize);
	run(m_numIterations);
	m_pGraph->writeTo(G, nodeXPosition, nodeYPosition);
	deallocate();
}

void FastMultipoleEmbedder::call(GraphAttributes &GA)
{
	EdgeArray<float> edgeLength(GA.constGraph());
	NodeArray<float> nodeSize(GA.constGraph());
	node v;
	edge e;
	forall_nodes(v, GA.constGraph())
	{
		nodeSize[v] = (float)sqrt(GA.width(v)*GA.width(v) + GA.height(v)*GA.height(v)) * 0.5f;
	}

	forall_edges(e, GA.constGraph())
	{
		edgeLength[e] = nodeSize[e->source()] + nodeSize[e->target()];
	}
	call(GA, edgeLength, nodeSize);
}

void FastMultipoleEmbedder::call(GraphAttributes &GA, const EdgeArray<float>& edgeLength, const NodeArray<float>& nodeSize)
{
	allocate(GA.constGraph().numberOfNodes(), GA.constGraph().numberOfEdges());
	m_pGraph->readFrom(GA, edgeLength, nodeSize);
	run(m_numIterations);
	m_pGraph->writeTo(GA);
	deallocate();

	edge e;
	forall_edges(e, GA.constGraph())
	{
		GA.bends(e).clear();
	}
}

void FastMultipoleEmbedder::run(__uint32 numIterations)
{
	if (m_pGraph->numNodes() == 0) return;
	if (m_pGraph->numNodes() == 1)
	{
		m_pGraph->nodeXPos()[0] = 0.0f;
		m_pGraph->nodeYPos()[0] = 0.0f;
		return;
	}

	if (m_randomize)
	{
		double avgNodeSize = 0.0;
		for (__uint32 i = 0; i < m_pGraph->numNodes(); i++)
		{
			avgNodeSize += m_pGraph->nodeSize()[i];
		}

		avgNodeSize = (avgNodeSize / (double)m_pGraph->numNodes());
		for (__uint32 i = 0; i < m_pGraph->numNodes(); i++)
		{
			m_pGraph->nodeXPos()[i] = (float)(randomDouble(-(double)m_pGraph->numNodes(), (double)m_pGraph->numNodes())*avgNodeSize*2);
			m_pGraph->nodeYPos()[i] = (float)(randomDouble(-(double)m_pGraph->numNodes(), (double)m_pGraph->numNodes())*avgNodeSize*2);
		}
	}

	m_pOptions->maxNumIterations = numIterations;
	m_pOptions->stopCritForce = (((float)m_pGraph->numNodes())*((float)m_pGraph->numNodes())*m_pGraph->avgNodeSize()) / m_pOptions->stopCritConstSq;
	if (m_pGraph->numNodes() < 100)
		runSingle();
	else
		runMultipole();
}


void FastMultipoleEmbedder::runMultipole()
{
	FMEGlobalContext* pGlobalContext = FMEMultipoleKernel::allocateContext(m_pGraph, m_pOptions, m_threadPool->numThreads());
	m_threadPool->runKernel<FMEMultipoleKernel>(pGlobalContext);
	FMEMultipoleKernel::deallocateContext(pGlobalContext);
}


void FastMultipoleEmbedder::runSingle()
{
	FMESingleKernel kernel;
	kernel(*m_pGraph, m_pOptions->timeStep, m_pOptions->minNumIterations, m_pOptions->maxNumIterations, m_pOptions->stopCritForce);
}


void FastMultipoleEmbedder::allocate(__uint32 numNodes, __uint32 numEdges)
{
	m_pOptions = new FMEGlobalOptions();
	m_pGraph = new ArrayGraph(numNodes, numEdges);
	initOptions();
	if (!m_maxNumberOfThreads)
	{
		__uint32 availableThreads = System::numberOfProcessors();
		__uint32 minNodesPerThread = 100;
		m_numberOfThreads = numNodes / minNodesPerThread;
		m_numberOfThreads = max<__uint32>(1, m_numberOfThreads);
		m_numberOfThreads = prevPowerOfTwo(min<__uint32>(m_numberOfThreads, availableThreads));
	} else
	{
		__uint32 availableThreads = min<__uint32>(m_maxNumberOfThreads, System::numberOfProcessors());
		__uint32 minNodesPerThread = 100;
		m_numberOfThreads = numNodes / minNodesPerThread;
		m_numberOfThreads = max<__uint32>(1, m_numberOfThreads);
		m_numberOfThreads = prevPowerOfTwo(min<__uint32>(m_numberOfThreads, availableThreads));
	}
	m_threadPool = new FMEThreadPool(m_numberOfThreads);
}


void FastMultipoleEmbedder::deallocate()
{
	delete m_threadPool;
	delete m_pGraph;
	delete m_pOptions;
}



void FastMultipoleMultilevelEmbedder::dumpCurrentLevel(const String& filename)
{
	const Graph& G = *(m_pCurrentLevel->m_pGraph);
	GraphAttributes GA(G);
	node v = 0;
	forall_nodes(v, G)
	{
		GalaxyMultilevel::LevelNodeInfo& nodeInfo = (*(m_pCurrentLevel->m_pNodeInfo))[v];
		GA.x(v) = (*m_pCurrentNodeXPos)[v];
		GA.y(v) = (*m_pCurrentNodeYPos)[v];
		GA.width(v) = GA.height(v)= nodeInfo.radius / sqrt(2.0);
	}
	GA.writeGML(filename);
}

void FastMultipoleMultilevelEmbedder::call(GraphAttributes &GA)
{
	EdgeArray<float> edgeLengthAuto(GA.constGraph());
	computeAutoEdgeLength(GA, edgeLengthAuto);
	m_multiLevelNumNodesBound = 10; //10
	const Graph& t = GA.constGraph();
	if (t.numberOfNodes() <= 25)
	{
		FastMultipoleEmbedder fme;
		fme.setNumberOfThreads(this->m_iMaxNumThreads);
		fme.setRandomize(true);
		fme.setNumIterations(500);
		fme.call(GA);
		return;
	}

	run(GA, edgeLengthAuto);

	edge e;
	forall_edges(e, GA.constGraph())
	{
		GA.bends(e).clear();
	}
}

void FastMultipoleMultilevelEmbedder::computeAutoEdgeLength(const GraphAttributes& GA, EdgeArray<float>& edgeLength, float factor)
{
	edge e = 0;
	node v = 0;
	node w = 0;
	forall_edges(e, GA.constGraph())
	{
		v = e->source();
		w = e->target();
		float radius_v = (float)sqrt(GA.width(v)*GA.width(v) + GA.height(v)*GA.height(v)) * 0.5f;
		float radius_w = (float)sqrt(GA.width(w)*GA.width(w) + GA.height(w)*GA.height(w)) * 0.5f;
		float sum = radius_v + radius_w;
		if (DIsEqual(sum, 0.0))
			sum = 1.0;
		edgeLength[e] = factor*(sum);
	}
}

void FastMultipoleMultilevelEmbedder::run(GraphAttributes& GA, const EdgeArray<float>& edgeLength)
{
	// too lazy for new, delete
	NodeArray<float> nodeXPos1;
	NodeArray<float> nodeYPos1;
	NodeArray<float> nodeXPos2;
	NodeArray<float> nodeYPos2;
	EdgeArray<float> edgeLength1;
	NodeArray<float> nodeSize1;

	m_pCurrentNodeXPos	= &nodeXPos1;
	m_pCurrentNodeYPos	= &nodeYPos1;
	m_pLastNodeXPos		= &nodeXPos2;
	m_pLastNodeYPos		= &nodeYPos2;
	m_pCurrentEdgeLength= &edgeLength1;
	m_pCurrentNodeSize	= &nodeSize1;
	Graph* pGraph = const_cast<Graph*>(&(GA.constGraph()));

	// create all multilevels
	this->createMultiLevelGraphs(pGraph, GA, edgeLength);
	// init the coarsest level
	initCurrentLevel();
#ifdef OGDF_DEBUG
	String str;
	str.sprintf("d:\\level%d_in.gml", m_iCurrentLevelNr);
	this->dumpCurrentLevel(str);
#endif

	//-------------------------
	// layout the current level
	layoutCurrentLevel();
	//-------------------------

#ifdef OGDF_DEBUG
	str.sprintf("d:\\level%d_out.gml", m_iCurrentLevelNr);
	this->dumpCurrentLevel(str);
#endif

	//-----------------------------
	//proceed with remaining levels
	while (m_iCurrentLevelNr > 0)
	{
		// move to finer level
		nextLevel();
		// init the arrays for current level
		initCurrentLevel();
		// assign positions from last to current
		assignPositionsFromPrevLevel();
#ifdef OGDF_DEBUG
		str.sprintf("d:\\level%d_in.gml", m_iCurrentLevelNr);
		this->dumpCurrentLevel(str);
#endif
		// layout the current level
		layoutCurrentLevel();

#ifdef OGDF_DEBUG
		str.sprintf("d:\\level%d_out.gml", m_iCurrentLevelNr);
		this->dumpCurrentLevel(str);
#endif
	}
	// the finest level is processed
	// assumes m_pCurrentGraph == GA.constGraph
	writeCurrentToGraphAttributes(GA);
	// clean up multilevels
	deleteMultiLevelGraphs();
}

void FastMultipoleMultilevelEmbedder::createMultiLevelGraphs(Graph* pGraph, GraphAttributes& GA, const EdgeArray<float>& finestLevelEdgeLength)
{
	m_pCurrentLevel = new GalaxyMultilevel(pGraph);
	m_pFinestLevel = m_pCurrentLevel;
	initFinestLevel(GA, finestLevelEdgeLength);
	m_iNumLevels = 1;
	m_iCurrentLevelNr = 0;

	GalaxyMultilevelBuilder builder;
	while (m_pCurrentLevel->m_pGraph->numberOfNodes() > m_multiLevelNumNodesBound)
	{
		GalaxyMultilevel* newLevel = builder.build(m_pCurrentLevel);
		m_pCurrentLevel = newLevel;
		m_iNumLevels++;
		m_iCurrentLevelNr++;
	}
	m_pCoarsestLevel = m_pCurrentLevel;
	m_pCurrentGraph = m_pCurrentLevel->m_pGraph;
}


void FastMultipoleMultilevelEmbedder::writeCurrentToGraphAttributes(GraphAttributes& GA)
{
	node v;
	forall_nodes(v, (*m_pCurrentGraph))
	{
		GA.x(v) = (*m_pCurrentNodeXPos)[v];
		GA.y(v) = (*m_pCurrentNodeYPos)[v];
	}
}

void FastMultipoleMultilevelEmbedder::nextLevel()
{
	m_pCurrentLevel = m_pCurrentLevel->m_pFinerMultiLevel;
	std::swap(m_pLastNodeXPos, m_pCurrentNodeXPos);
	std::swap(m_pLastNodeYPos, m_pCurrentNodeYPos);
	m_iCurrentLevelNr--;
}

void FastMultipoleMultilevelEmbedder::initFinestLevel(GraphAttributes &GA, const EdgeArray<float>& edgeLength)
{
	node v = 0;
	node w = 0;
	edge e = 0;
	//NodeArray<float> perimeter(GA.constGraph(), 0.0);
	forall_nodes(v, GA.constGraph())
	{
		GalaxyMultilevel::LevelNodeInfo& nodeInfo = (*(m_pFinestLevel->m_pNodeInfo))[v];
		nodeInfo.mass = 1.0;
		float r = (float)sqrt(GA.width(v)*GA.width(v) + GA.height(v)*GA.height(v)) * 0.5f;
		nodeInfo.radius = r;
	}

	forall_edges(e, GA.constGraph())
	{
		GalaxyMultilevel::LevelEdgeInfo& edgeInfo = (*(m_pFinestLevel->m_pEdgeInfo))[e];
		v = e->source();
		w = e->target();
		GalaxyMultilevel::LevelNodeInfo& vNodeInfo = (*(m_pFinestLevel->m_pNodeInfo))[v];
		GalaxyMultilevel::LevelNodeInfo& wNodeInfo = (*(m_pFinestLevel->m_pNodeInfo))[w];
		edgeInfo.length = (vNodeInfo.radius +  wNodeInfo.radius) + edgeLength[e];
	}
}

void FastMultipoleMultilevelEmbedder::initCurrentLevel()
{
	m_pCurrentGraph = m_pCurrentLevel->m_pGraph;
	m_pCurrentNodeXPos->init(*m_pCurrentGraph, 0.0f);
	m_pCurrentNodeYPos->init(*m_pCurrentGraph, 0.0f);
	m_pCurrentEdgeLength->init(*m_pCurrentGraph, 1.0f);
	m_pCurrentNodeSize->init(*m_pCurrentGraph, 1.0f);
	const Graph& G = *(m_pCurrentLevel->m_pGraph);
	node v = 0;

	forall_nodes(v, G)
	{
		GalaxyMultilevel::LevelNodeInfo& nodeInfo = (*(m_pCurrentLevel->m_pNodeInfo))[v];
		(*m_pCurrentNodeSize)[v] = ((float)nodeInfo.radius);//(((float)nodeInfo.radius));
	}

	edge e = 0;
	forall_edges(e, G)
	{
		GalaxyMultilevel::LevelEdgeInfo& edgeInfo = (*(m_pCurrentLevel->m_pEdgeInfo))[e];
		(*m_pCurrentEdgeLength)[e] = edgeInfo.length*0.25f;
	}
}

void FastMultipoleMultilevelEmbedder::assignPositionsFromPrevLevel()
{
	float scaleFactor = 1.4f;// 1.4f;//1.4f; //1.4f
	// init m_pCurrent Pos from m_pLast Pos
	const Graph& G = *(m_pCurrentLevel->m_pGraph);
	node v = 0;
	forall_nodes(v, G)
	{
		GalaxyMultilevel::LevelNodeInfo& nodeInfo = (*(m_pCurrentLevel->m_pNodeInfo))[v];
		float x = (float)((*m_pLastNodeXPos)[nodeInfo.parent] + (float)randomDouble(-1.0, 1.0));
		float y = (float)((*m_pLastNodeYPos)[nodeInfo.parent] + (float)randomDouble(-1.0, 1.0));
		(*m_pCurrentNodeXPos)[v] = x*scaleFactor;
		(*m_pCurrentNodeYPos)[v] = y*scaleFactor;
	}
}

void FastMultipoleMultilevelEmbedder::layoutCurrentLevel()
{
	FastMultipoleEmbedder fme;
	fme.setNumberOfThreads(this->m_iMaxNumThreads);
	fme.setRandomize(m_iCurrentLevelNr == (m_iNumLevels-1));
	fme.setNumIterations(numberOfIterationsByLevelNr(m_iCurrentLevelNr));
	fme.call((*m_pCurrentGraph), (*m_pCurrentNodeXPos), (*m_pCurrentNodeYPos), (*m_pCurrentEdgeLength), (*m_pCurrentNodeSize));
}

void FastMultipoleMultilevelEmbedder::deleteMultiLevelGraphs()
{
	GalaxyMultilevel* l = m_pCoarsestLevel;
	GalaxyMultilevel* toDelete = l;
	while (l)
	{
		toDelete = l;
		l = l->m_pFinerMultiLevel;
		delete (toDelete->m_pNodeInfo);
		delete (toDelete->m_pEdgeInfo);
		if (toDelete != m_pFinestLevel)
			delete (toDelete->m_pGraph);
		delete toDelete;
	}
}

__uint32 FastMultipoleMultilevelEmbedder::numberOfIterationsByLevelNr(__uint32 levelNr)
{
	return 200*(levelNr+1)*(levelNr+1);
}


} // end of namespace
