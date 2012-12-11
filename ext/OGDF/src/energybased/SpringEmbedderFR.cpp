/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Spring-Embedder algorithm
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

#include <ogdf/energybased/SpringEmbedderFR.h>
#include <ogdf/packing/TileToRowsCCPacker.h>
#include <ogdf/basic/GraphCopyAttributes.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <math.h>


namespace ogdf {


SpringEmbedderFR::SpringEmbedderFR()
{
	m_A = 0;
	// default parameters
	m_iterations   = 400;
	m_fineness     = 0.51;

	m_xleft = m_ysmall = 0.0;
	m_xright = m_ybig = 250.0;
	m_noise = true;

	m_scaling = scScaleFunction;
	m_scaleFactor = 8.0;
	m_bbXmin = 0.0;
	m_bbXmax = 100.0;
	m_bbYmin = 0.0;
	m_bbYmax = 100.0;

	m_minDistCC = 20;
	m_pageRatio = 1.0;
}


void SpringEmbedderFR::call(GraphAttributes &AG)
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

		// original
		if (initialize(GC, AGC) == true)
		{
			for(int i = 1; i <= m_iterations; i++)
				mainStep(GC, AGC);

		}
		cleanup();
		// end original

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

	m_lit.init();
}


bool SpringEmbedderFR::initialize(GraphCopy &G, GraphCopyAttributes &AG)
{
	if(G.numberOfNodes() <= 1)
		return false;  // nothing to do

	m_A = 0;

	// compute a suitable area (xleft,ysmall), (xright,ybig)
	// zoom the current layout into that area

	double w_sum = 0.0, h_sum = 0.0;
	double xmin, xmax, ymin, ymax;

	node v = G.firstNode();
	xmin = xmax = AG.x(v);
	ymin = ymax = AG.y(v);

	forall_nodes(v,G) {
		if(AG.x(v) < xmin) xmin = AG.x(v);
		if(AG.x(v) > xmax) xmax = AG.x(v);
		if(AG.y(v) < ymin) ymin = AG.y(v);
		if(AG.y(v) > ymax) ymax = AG.y(v);
		w_sum += AG.getWidth (v);
		h_sum += AG.getHeight(v);
	}

	switch(m_scaling) {
	case scInput:
		m_xleft  = xmin;
		m_xright = xmax;
		m_ysmall = ymin;
		m_ybig  = ymax;
		break;

	case scUserBoundingBox:
	case scScaleFunction:

		if (m_scaling == scUserBoundingBox) {
			m_xleft  = m_bbXmin;
			m_xright = m_bbXmax;
			m_ysmall = m_bbYmin;
			m_ybig   = m_bbYmax;

		} else {
			double sqrt_n = sqrt((double)G.numberOfNodes());
			m_xleft  = 0;
			m_ysmall = 0;
			m_xright = (w_sum > 0) ? m_scaleFactor * w_sum / sqrt_n : 1;
			m_ybig   = (h_sum > 0) ? m_scaleFactor * h_sum / sqrt_n : 1;
		}
		// Compute scaling such that layout coordinates fit into used bounding box
		double fx = (xmax == xmin) ? 1.0 : m_xright / (xmax - xmin);
		double fy = (ymax == ymin) ? 1.0 : m_ybig   / (ymax - ymin);
		// Adjust coordinates accordingly
		forall_nodes(v,G) {
			AG.x(v) = m_xleft  + (AG.x(v) - xmin) * fx;
			AG.y(v) = m_ysmall + (AG.y(v) - ymin) * fy;
		}
	}


	m_lit.init(G);


	m_width  = m_xright - m_xleft;
	m_height = m_ybig - m_ysmall;

	OGDF_ASSERT((m_width >= 0) && (m_height >= 0))

	m_txNull = m_width/50;
	m_tyNull = m_height/50;
	m_tx = m_txNull;
	m_ty = m_tyNull;

	//m_k = sqrt(m_width*m_height / G.numberOfNodes()) / 2;
	m_k = m_fineness * sqrt(m_width*m_height / G.numberOfNodes());
	m_k2 = 2*m_k;
	m_kk = m_k*m_k;

	m_ki = int(m_k);

	if (m_ki == 0) m_ki = 1;

	m_cF = 1;

	// build  matrix of node lists
	m_xA = int(m_width / m_ki + 1);
	m_yA = int(m_height / m_ki + 1);
	m_A = new Array2D<List<node> >(-1,m_xA,-1,m_yA);

	forall_nodes(v,G)
	{
		double xv = AG.x(v);
		double yv = AG.y(v);

		int i = int((xv - m_xleft) / m_ki);
		int j = int((yv - m_ysmall) / m_ki);

		OGDF_ASSERT( (i < m_xA) && (i > -1) )
		OGDF_ASSERT( (j < m_yA) && (j > -1) )

		m_lit[v] = (*m_A)(i,j).pushFront(v);
	}

	return true;
}


#define FREPULSE(d) ((m_k2 > (d)) ? m_kk/(d) : 0)


void SpringEmbedderFR::mainStep(GraphCopy &G, GraphCopyAttributes &AG)
{
	//const Graph &G = AG.constGraph();

	node u,v;
	edge e;

	NodeArray<double> xdisp(G,0);
	NodeArray<double> ydisp(G,0);

	// repulsive forces
	forall_nodes(v,G)
	{
		double xv = AG.x(v);
		double yv = AG.y(v);

		int i = int((xv - m_xleft) / m_ki);
		int j = int((yv - m_ysmall) / m_ki);

		for(int m = -1; m <= 1; m++)
		{
			for(int n = -1; n <= 1; n++)
			{
				ListIterator<node> it;
				for(it = (*m_A)(i+m,j+n).begin(); it.valid(); ++it)
				{
					u = *it;

					if(u == v) continue;
					double xdist = xv - AG.x(u);
					double ydist = yv - AG.y(u);
					double dist = sqrt(xdist*xdist + ydist*ydist);
					if(dist < 1e-3)
						dist = 1e-3;
					xdisp[v] += FREPULSE(dist) * xdist / dist;
					ydisp[v] += FREPULSE(dist) * ydist / dist;
				}
			}
		}
	}

	// attractive forces
	forall_edges(e,G)
	{
		node u = e->source();
		node v = e->target();
		double xdist = AG.x(v) - AG.x(u);
		double ydist = AG.y(v) - AG.y(u);
		double dist = sqrt(xdist*xdist + ydist*ydist);

		double f = (u->degree()+v->degree())/6.0;

		dist /= f;

		double fac = dist / m_k;

		xdisp[v] -= xdist*fac;
		ydisp[v] -= ydist*fac;
		xdisp[u] += xdist*fac;
		ydisp[u] += ydist*fac;
	}

	// noise
	if(m_noise)
	{
		forall_nodes(v,G)
		{
			xdisp[v] *= (double(randomNumber(750,1250))/1000.0);
			ydisp[v] *= (double(randomNumber(750,1250))/1000.0);
		}
	}


	// preventions

	forall_nodes(v,G)
	{
		double xv = AG.x(v);
		double yv = AG.y(v);

		int i0 = int((xv - m_xleft) / m_ki);
		int j0 = int((yv - m_ysmall) / m_ki);

		double xd = xdisp[v];
		double yd = ydisp[v];
		double dist = sqrt(xd*xd+yd*yd);

		if (dist < 1)
			dist = 1;

		xd = m_tx*xd/dist;
		yd = m_ty*yd/dist;

		double xp = xv + xd;
		double yp = yv + yd;

		int i,j;

		if( (xp > m_xleft) && (xp < m_xright) )
		{
			AG.x(v) = xp;
			i = int((xp - m_xleft) / m_ki);
		} else
			i = i0;

		if( (yp > m_ysmall) && (yp < m_ybig) )
		{
			AG.y(v) = yp;
			j = int((yp - m_ysmall) / m_ki);
		} else
			j = j0;

		if( (i != i0) || (j != j0) )
		{
			OGDF_ASSERT(m_lit[v].valid());

			(*m_A)(i0,j0).moveToFront(m_lit[v], (*m_A)(i,j));
		}
	}

	m_tx = m_txNull / mylog2(m_cF);
	m_ty = m_tyNull / mylog2(m_cF);

	m_cF++;
}


} // end namespace ogdf
