/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements classes GridLayout and GridLayoutMapped
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


#include <ogdf/basic/GridLayoutMapped.h>
#include <ogdf/planarity/PlanRepUML.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/basic/Layout.h>
#include <ogdf/basic/HashArray.h>


namespace ogdf {


//---------------------------------------------------------
// GridLayout
//---------------------------------------------------------

IPolyline GridLayout::polyline(edge e) const
{
	IPolyline ipl = m_bends[e];
	IPoint ipStart = IPoint(m_x[e->source()],m_y[e->source()]);
	IPoint ipEnd   = IPoint(m_x[e->target()],m_y[e->target()]);

	if(ipl.empty() || ipStart != ipl.front())
		ipl.pushFront(ipStart);

	if(ipEnd != ipl.back() || ipl.size() < 2)
		ipl.pushBack(ipEnd);

	return ipl;
}

struct OGDF_EXPORT GridPointInfo
{
	GridPointInfo() : m_v(0), m_e(0) { }
	GridPointInfo(node v) : m_v(v), m_e(0) { }
	GridPointInfo(edge e) : m_v(0), m_e(e) { }

	bool operator==(const GridPointInfo &i) const {
		return (m_v == i.m_v && m_e == i.m_e);
	}

	bool operator!=(const GridPointInfo &i) const {
		return !operator==(i);
	}

	node m_v;
	edge m_e;
};

ostream &operator<<(ostream &os, const GridPointInfo &i)
{
	if(i.m_v == 0 && i.m_e == 0)
		os << "{}";
	else if(i.m_v != 0)
		os << "{node " << i.m_v << "}";
	else
		os << "{edge " << i.m_e << "}";

	return os;
}


class IPointHashFunc {
public:
	int hash(const IPoint &ip) {
		return 7*ip.m_x + 23*ip.m_y;
	}
};


bool GridLayout::checkLayout()
{
	const Graph &G = *m_x.graphOf();
	HashArray<IPoint,GridPointInfo> H;

	node v;
	forall_nodes(v,G)
	{
		IPoint ip = IPoint(m_x[v],m_y[v]);
		GridPointInfo i = H[ip];
		if(i != GridPointInfo()) {
			cout << "conflict of " << v << " with " << H[ip] << endl;
			return false;
		}

		H[ip] = GridPointInfo(v);
	}

	edge e;
	forall_edges(e,G)
	{
		const IPolyline &bends = m_bends[e];

		ListConstIterator<IPoint> it;
		for(it = bends.begin(); it.valid(); ++it) {
			GridPointInfo i = H[*it];
			if(i != GridPointInfo()) {
				cout << "conflict of bend point " << (*it) << " of edge " << e << " with " << H[*it] << endl;
				return false;
			}

			H[*it] = GridPointInfo(e);
		}

	}

	return true;
}

bool GridLayout::isRedundant(IPoint &p1, IPoint &p2, IPoint &p3)
{
	int dzy1 = p3.m_x - p2.m_x;
	int dzy2 = p3.m_y - p2.m_y;
	int dyx1 = p2.m_x - p1.m_x;

	if (dzy1 == 0) return (dyx1 == 0 || dzy2 == 0);

	int f = dyx1 * dzy2;

	return (f % dzy1 == 0 && (p2.m_y - p1.m_y) == f / dzy1);
}

void GridLayout::compact(IPolyline &ip)
{
	if (ip.size() < 3) return;

	ListIterator<IPoint> it = ip.begin();
	IPoint p = *it; ++it;
	for (it = it.succ(); it.valid(); ++it) {
		ListIterator<IPoint> itPred = it.pred();
		if(p == *itPred || isRedundant(p,*itPred,*it)) {
			ip.del(itPred);
		} else {
			p = *itPred;
		}
	}
}

IPolyline GridLayout::getCompactBends(edge e) const
{
	IPolyline ipl = m_bends[e];

	if (ipl.size() == 0) return ipl;

#if 0
	IPoint ip_first = ipl.front();
	IPoint ip_last  = ipl.back();
#endif

	IPoint ip_src(m_x[e->source()],m_y[e->source()]);
	IPoint ip_tgt(m_x[e->target()],m_y[e->target()]);

	ipl.pushFront(ip_src);
	ipl.pushBack (ip_tgt);

	compact(ipl);

	ipl.popFront();
	ipl.popBack();

#if 0
	if (ip_first != ip_src && (ipl.empty() || ip_first != ipl.front()))
		ipl.pushFront(ip_first);
	if (ip_last != ip_tgt && (ipl.empty() || ip_last != ipl.back()))
		ipl.pushBack(ip_last);
#endif

	return ipl;
}


void GridLayout::compactAllBends()
{
	const Graph &G = *m_x.graphOf();

	edge e;
	forall_edges(e,G)
		m_bends[e] = getCompactBends(e);
}

void GridLayout::remap(Layout &drawing)
{
	const Graph &G = *m_x.graphOf();

	node v;
	forall_nodes(v,G) {
		drawing.x(v) = m_x[v];
		drawing.y(v) = m_y[v];
	}

}


void GridLayout::computeBoundingBox(int &xmin, int &xmax, int &ymin, int &ymax)
{
	const Graph *pG = m_x.graphOf();

	if(pG == 0 || pG->empty()) {
		xmin = xmax = ymin = ymax = 0;
		return;
	}

	xmin = ymin = INT_MAX;
	xmax = ymax = INT_MIN;

	node v;
	forall_nodes(v,*pG) {
		int x = m_x[v];
		if(x < xmin) xmin = x;
		if(x > xmax) xmax = x;

		int y = m_y[v];
		if(y < ymin) ymin = y;
		if(y > ymax) ymax = y;
	}

	edge e;
	forall_edges(e,*pG) {
		ListConstIterator<IPoint> it;
		for(it = m_bends[e].begin(); it.valid(); ++it) {
			int x = (*it).m_x;
			if(x < xmin) xmin = x;
			if(x > xmax) xmax = x;

			int y = (*it).m_y;
			if(y < ymin) ymin = y;
			if(y > ymax) ymax = y;
		}
	}
}


int GridLayout::manhattanDistance(const IPoint &ip1, const IPoint &ip2)
{
	return abs(ip2.m_x-ip1.m_x) + abs(ip2.m_y-ip1.m_y);
}

double GridLayout::euclideanDistance(const IPoint &ip1, const IPoint &ip2)
{
	double dx = ip2.m_x-ip1.m_x;
	double dy = ip2.m_y-ip1.m_y;

	return sqrt(dx*dx + dy*dy);
}


int GridLayout::totalManhattanEdgeLength() const
{
	const Graph *pG = m_x.graphOf();
	int length = 0;

	edge e;
	forall_edges(e,*pG)
		length += manhattanEdgeLength(e);
	//{
	//	IPoint ip1 = IPoint(m_x[e->source()],m_y[e->source()]);
	//	ListConstIterator<IPoint> it = m_bends[e].begin();
	//	for(; it.valid(); ++it) {
	//		length += manhattanDistance(ip1,*it);
	//		ip1 = *it;
	//	}
	//	length += manhattanDistance(ip1,IPoint(m_x[e->target()],m_y[e->target()]));
	//}

	return length;
}


int GridLayout::maxManhattanEdgeLength() const
{
	const Graph *pG = m_x.graphOf();
	int length = 0;

	edge e;
	forall_edges(e,*pG)
		length = max( length, manhattanEdgeLength(e) );

	return length;
}


int GridLayout::manhattanEdgeLength(edge e) const
{
	int length = 0;

	IPoint ip1 = IPoint(m_x[e->source()],m_y[e->source()]);
	ListConstIterator<IPoint> it = m_bends[e].begin();
	for(; it.valid(); ++it) {
		length += manhattanDistance(ip1,*it);
		ip1 = *it;
	}
	length += manhattanDistance(ip1,IPoint(m_x[e->target()],m_y[e->target()]));

	return length;
}


double GridLayout::totalEdgeLength() const
{
	const Graph *pG = m_x.graphOf();
	double length = 0;

	edge e;
	forall_edges(e,*pG) {
		IPoint ip1 = IPoint(m_x[e->source()],m_y[e->source()]);
		ListConstIterator<IPoint> it = m_bends[e].begin();
		for(; it.valid(); ++it) {
			length += euclideanDistance(ip1,*it);
			ip1 = *it;
		}
		length += euclideanDistance(ip1,IPoint(m_x[e->target()],m_y[e->target()]));
	}

	return length;
}


int GridLayout::numberOfBends() const
{
	const Graph *pG = m_x.graphOf();
	int num = 0;

	edge e;
	forall_edges(e,*pG)
		num += m_bends[e].size();

	return num;
}


//---------------------------------------------------------
// GridLayoutMapped
//---------------------------------------------------------

GridLayoutMapped::GridLayoutMapped(
	const PlanRep &PG,
	const OrthoRep &OR,
	double separation,
	double cOverhang,
	int fineness) :
		GridLayout(PG), m_gridWidth(PG,0), m_gridHeight(PG,0), m_pPG(&PG)
{
	// determine grid mapping factor
	double minDelta = separation;

	node v;
	forall_nodes(v,PG)
	{
		node vOrig = PG.original(v);
		if(vOrig == 0) continue;

		const OrthoRep::VertexInfoUML *pInfo = OR.cageInfo(v);

		for(int s = 0; s <= 3; ++s) {
			const OrthoRep::SideInfoUML &si = pInfo->m_side[s];
			double size = (s & 1) ? PG.widthOrig(vOrig) : PG.heightOrig(vOrig);
			if (size == 0) continue;

			if(si.m_adjGen) {
				int k = max(si.m_nAttached[0],si.m_nAttached[1]);
				if (k == 0)
					minDelta = min(minDelta, size/2);
				else
					minDelta = min(minDelta, size / (2*(k + cOverhang)));

			} else {
				if (si.m_nAttached[0] == 0)
					minDelta = min(minDelta, size);

				else if ( !((si.m_nAttached[0] == 1) && (cOverhang == 0.0)) ) //hier cov= 0 abfangen
					minDelta =  min(minDelta,
						size / (si.m_nAttached[0] - 1 + 2*cOverhang));
				else
					minDelta = min(minDelta, size/2);
			}

		}

	}

	if(0 < cOverhang && cOverhang < 1) {
		/*double tryMinDelta = max(cOverhang * minDelta, separation/10000.0);
		double tryMinDelta = max(cOverhang * minDelta, separation/10000.0);
		if(tryMinDelta < minDelta)
			minDelta = tryMinDelta;*/
		minDelta *= cOverhang;
	}

	m_fMapping = fineness / minDelta;


	// initialize grid sizes of vertices
	forall_nodes(v,PG)
	{
		node vOrig = PG.original(v);

		if(vOrig) {
			m_gridWidth [v] = toGrid(PG.widthOrig (vOrig));
			m_gridHeight[v] = toGrid(PG.heightOrig(vOrig));
		}
	}
}


void GridLayoutMapped::remap(Layout &drawing)
{
	node v;
	forall_nodes(v,*m_pPG) {
		//should use toDouble here
		drawing.x(v) = (m_x[v]/cGridScale) / m_fMapping;
		drawing.y(v) = (m_y[v]/cGridScale) / m_fMapping;
	}
}


} // end namespace ogdf

