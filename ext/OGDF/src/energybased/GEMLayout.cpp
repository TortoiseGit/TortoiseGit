/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementations of class GEMLayout.
 *
 * Fast force-directed layout algorithm (GEMLayout) based on Frick et al.'s algorithm
 *
 * \author Christoph Buchheim
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


#include <ogdf/energybased/GEMLayout.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/GraphCopyAttributes.h>
#include <ogdf/packing/TileToRowsCCPacker.h>


namespace ogdf {

GEMLayout::GEMLayout() :
	m_numberOfRounds(30000),
	m_minimalTemperature(0.005),
	m_initialTemperature(12.0),
	m_gravitationalConstant(1.0/16.0), //original paper value
	m_desiredLength(5.0),
	m_maximalDisturbance(0),
	m_rotationAngle(Math::pi/3.0),
	m_oscillationAngle(Math::pi_2),
	m_rotationSensitivity(0.01),
	m_oscillationSensitivity(0.3),
	m_attractionFormula(1),
	m_minDistCC(20),
	m_pageRatio(1.0)
{ }

GEMLayout::GEMLayout(const GEMLayout &fl) :
	m_numberOfRounds(fl.m_numberOfRounds),
	m_minimalTemperature(fl.m_minimalTemperature),
	m_initialTemperature(fl.m_initialTemperature),
	m_gravitationalConstant(fl.m_gravitationalConstant),
	m_desiredLength(fl.m_desiredLength),
	m_maximalDisturbance(fl.m_maximalDisturbance),
	m_rotationAngle(fl.m_rotationAngle),
	m_oscillationAngle(fl.m_oscillationAngle),
	m_rotationSensitivity(fl.m_rotationSensitivity),
	m_oscillationSensitivity(fl.m_oscillationSensitivity),
	m_attractionFormula(fl.m_attractionFormula),
	m_minDistCC(fl.m_minDistCC),
	m_pageRatio(fl.m_pageRatio)
{ }


GEMLayout::~GEMLayout() { }


GEMLayout &GEMLayout::operator=(const GEMLayout &fl)
{
	m_numberOfRounds = fl.m_numberOfRounds;
	m_minimalTemperature = fl.m_minimalTemperature;
	m_initialTemperature = fl.m_initialTemperature;
	m_gravitationalConstant = fl.m_gravitationalConstant;
	m_desiredLength = fl.m_desiredLength;
	m_maximalDisturbance = fl.m_maximalDisturbance;
	m_rotationAngle = fl.m_rotationAngle;
	m_oscillationAngle = fl.m_oscillationAngle;
	m_rotationSensitivity = fl.m_rotationSensitivity;
	m_oscillationSensitivity = fl.m_oscillationSensitivity;
	m_attractionFormula = fl.m_attractionFormula;
	return *this;
}


void GEMLayout::call(GraphAttributes &AG)
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
		node vCopy;
		forall_nodes(vCopy, GC) {
			node vOrig = GC.original(vCopy);
			AGC.x(vCopy) = AG.x(vOrig);
			AGC.y(vCopy) = AG.y(vOrig);
		}

		SList<node> permutation;
		node v;

		// initialize node data
		m_impulseX.init(GC,0);
		m_impulseY.init(GC,0);
		m_skewGauge.init(GC,0);
		m_localTemperature.init(GC,m_initialTemperature);

		// initialize other data
		m_globalTemperature = m_initialTemperature;
		m_barycenterX = 0;
		m_barycenterY = 0;
		forall_nodes(v,GC) {
			m_barycenterX += weight(v) * AGC.x(v);
			m_barycenterY += weight(v) * AGC.y(v);
		}
		m_cos = cos(m_oscillationAngle / 2.0);
		m_sin = sin(Math::pi / 2 + m_rotationAngle / 2.0);

		// main loop
		int counter = m_numberOfRounds;
		while(DIsGreater(m_globalTemperature,m_minimalTemperature) && counter--) {

			// choose nodes by random permutations
			if(permutation.empty()) {
				forall_nodes(v,GC)
					permutation.pushBack(v);
				permutation.permute();
			}
			v = permutation.popFrontRet();

			// compute the impulse of node v
			computeImpulse(GC,AGC,v);

			// update node v
			updateNode(GC,AGC,v);

		}

		node vFirst = GC.firstNode();
		double minX = AGC.x(vFirst), maxX = AGC.x(vFirst),
			minY = AGC.y(vFirst), maxY = AGC.y(vFirst);

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


	// free node data
	m_impulseX.init();
	m_impulseY.init();
	m_skewGauge.init();
	m_localTemperature.init();
}

void GEMLayout::computeImpulse(GraphCopy &G, GraphCopyAttributes &AG,node v) {
	//const Graph &G = AG.constGraph();
	int n = G.numberOfNodes();

	node u;
	edge e;
	double deltaX,deltaY,delta,deltaSqu;
	double desiredLength,desiredSqu;

	// add double node radius to desired edge length
	desiredLength = m_desiredLength + length(AG.getHeight(v),AG.getWidth(v));
	desiredSqu = desiredLength * desiredLength;

	// compute attraction to center of gravity
	m_newImpulseX = (m_barycenterX / n - AG.x(v)) * m_gravitationalConstant;
	m_newImpulseY = (m_barycenterY / n - AG.y(v)) * m_gravitationalConstant;

	// disturb randomly
	int maxIntDisturbance = (int)(m_maximalDisturbance * 10000);
	m_newImpulseX +=
		(double)(randomNumber(-maxIntDisturbance,maxIntDisturbance) / 10000);
	m_newImpulseY +=
		(double)(randomNumber(-maxIntDisturbance,maxIntDisturbance) / 10000);

	// compute repulsive forces
	forall_nodes(u,G)
		if(u != v ) {
			deltaX = AG.x(v) - AG.x(u);
			deltaY = AG.y(v) - AG.y(u);
			delta = length(deltaX,deltaY);
			if(DIsGreater(delta,0)) {
				deltaSqu = delta * delta;
				m_newImpulseX += deltaX * desiredSqu / deltaSqu;
				m_newImpulseY += deltaY * desiredSqu / deltaSqu;
			}
	}

	// compute attractive forces
	forall_adj_edges(e,v) {
		u = e->opposite(v);
		deltaX = AG.x(v) - AG.x(u);
		deltaY = AG.y(v) - AG.y(u);
		delta = length(deltaX,deltaY);
		if(m_attractionFormula == 1) {
			m_newImpulseX -= deltaX * delta / (desiredLength * weight(v));
			m_newImpulseY -= deltaY * delta / (desiredLength * weight(v));
		}
		else {
			deltaSqu = delta * delta;
			m_newImpulseX -= deltaX * deltaSqu / (desiredSqu * weight(v));
			m_newImpulseY -= deltaY * deltaSqu / (desiredSqu * weight(v));
		}
	}

}

void GEMLayout::updateNode(GraphCopy &G, GraphCopyAttributes &AG,node v) {
	//const Graph &G = AG.constGraph();
	int n = G.numberOfNodes();
	double impulseLength;

	impulseLength = length(m_newImpulseX,m_newImpulseY);
	if(DIsGreater(impulseLength,0)) {

		// scale impulse by node temperature
		m_newImpulseX *= m_localTemperature[v] / impulseLength;
		m_newImpulseY *= m_localTemperature[v] / impulseLength;

		// move node
		AG.x(v) += m_newImpulseX;
		AG.y(v) += m_newImpulseY;

		// adjust barycenter
		m_barycenterX += weight(v) * m_newImpulseX;
		m_barycenterY += weight(v) * m_newImpulseY;

		impulseLength = length(m_newImpulseX,m_newImpulseY)
						* length(m_impulseX[v],m_impulseY[v]);
		if(DIsGreater(impulseLength,0)) {

			m_globalTemperature -= m_localTemperature[v] / n;

			// compute sine and cosine of angle between old and new impulse
			double sinBeta,cosBeta;
			sinBeta = (m_newImpulseX * m_impulseX[v]
				- m_newImpulseY * m_impulseY[v])
					/ impulseLength;
			cosBeta = (m_newImpulseX * m_impulseX[v]
				+ m_newImpulseY * m_impulseY[v])
					/ impulseLength;

			// check for rotation
			if(DIsGreater(sinBeta,m_sin))
				m_skewGauge[v] += m_rotationSensitivity;

			// check for oscillation
			if(DIsGreater(length(cosBeta),m_cos))
				m_localTemperature[v] *=
					(1 + cosBeta * m_oscillationSensitivity);

			// cool down according to skew gauge
			m_localTemperature[v] *= (1.0 - length(m_skewGauge[v]));
			if(DIsGreaterEqual(m_localTemperature[v],m_initialTemperature))
				m_localTemperature[v] = m_initialTemperature;

			// adjust global temperature
			m_globalTemperature += m_localTemperature[v] / n;
		}

		// save impulse
		m_impulseX[v] = m_newImpulseX;
		m_impulseY[v] = m_newImpulseY;
	}
}

} // end namespace ogdf
