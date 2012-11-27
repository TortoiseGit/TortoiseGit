/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Geometric-classes like DPoint, DPolyline, DRect, DLine,
 *  DScaler
 *
 * \author Joachim Kupke
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


#include "ogdf/basic/geometry.h"
#include "ogdf/basic/GraphAttributes.h"
#include "ogdf/basic/Math.h"
#include <cmath>


namespace ogdf {

//---------------------------------------------------------
// IPoint
//---------------------------------------------------------

ostream &operator<<(ostream &os, const IPoint &ip)
{
	os << "(" << ip.m_x << "," << ip.m_y << ")";
	return os;
}


//---------------------------------------------------------
// DPoint
//---------------------------------------------------------

// gives the euclidean distance between p and *this
double IPoint::distance(const IPoint &p) const
{
	double dx = p.m_x - m_x;
	double dy = p.m_y - m_y;
	return sqrt( (dx*dx) + (dy*dy) );
}


//---------------------------------------------------------
// IPolyline
//---------------------------------------------------------

// calculates the total length of a polyline
double IPolyline::length() const
{
	OGDF_ASSERT(!empty());

	double len = 0.0;
	ListConstIterator<IPoint> pred, iter;

	pred = iter = begin();
	++iter;

	while (iter.valid()) {
		len += (*iter).distance(*pred);
		++pred;
		++iter;
	}

	return len;
}


//---------------------------------------------------------
// DPoint
//---------------------------------------------------------

// gives the euclidean distance between p and *this
double DPoint::distance(const DPoint &p) const
{
	double dx = p.m_x - m_x;
	double dy = p.m_y - m_y;
	return sqrt( (dx*dx) + (dy*dy) );
}

// adds p to *this
DPoint DPoint::operator+(const DPoint &p) const
{
	return DPoint(m_x + p.m_x, m_y + p.m_y);
}

// subtracts p from *this
DPoint DPoint::operator-(const DPoint &p) const
{
	return DPoint(m_x - p.m_x, m_y - p.m_y);
}

// outputs dp
ostream &operator<<(ostream &os, const DPoint &dp)
{
	os << "(" << dp.m_x << "," << dp.m_y << ")";
	return os;
}


//---------------------------------------------------------
// DVector
//---------------------------------------------------------
DVector DVector::operator*(const double val) const
{
	DVector ret(m_x*val, m_y*val);
	return ret;
}

DVector DVector::operator/(const double val) const
{
	DVector ret(m_x/val, m_y/val);
	return ret;
}

// length
double DVector::length() const
{
	return sqrt((m_x * m_x) + (m_y * m_y));
}

// determinante
double DVector::operator^(const DVector &dv) const
{
	return ((m_x * dv.m_y) - (m_y * dv.m_x));
}

// s-product
double DVector::operator*(const DVector &dv) const
{
	return ((m_x * dv.m_x) + (m_y * dv.m_y));
}

// ortho left
DVector DVector::operator++() const
{
	DVector ret;
	if (m_x != 0.0) {
		ret.m_y = 1.0;
		ret.m_x = - m_y / m_x;
	}
	else {
		ret.m_x = 1.0;
		ret.m_y = 0.0;

	}
	return ret;
}

// ortho right
DVector DVector::operator--() const
{
	return (++(*this)) * (-1.0);
}



//---------------------------------------------------------
// DPolyline
//---------------------------------------------------------

const double DPolyline::s_prec = 10000.0;

// calculates the total length of a polyline
double DPolyline::length() const
{
	OGDF_ASSERT(!empty());

	double len = 0.0;
	ListConstIterator<DPoint> pred, iter;

	pred = iter = begin();
	++iter;

	while (iter.valid()) {
		len += (*iter).distance(*pred);
		++pred;
		++iter;
	}

	return len;
}


// gives the point on a polyline, which is fraction*len away from the start
DPoint DPolyline::position(const double fraction, double len) const
{
	OGDF_ASSERT(!empty());
	OGDF_ASSERT(fraction >= 0.0 && fraction <= 1.0);
	if (len < 0.0)
		len = length();
	OGDF_ASSERT(len >= 0.0);

	DPoint p      = (*begin());
	double liter  = 0.0;
	double pos    = len * fraction;
	double seglen = 0.0;
	ListConstIterator<DPoint> pred, iter;

	pred = iter = begin();
	++iter;

	// search the segment, which contains the desired point
	double DX = 0, DY = 0; // for further use
	while (iter.valid()) {
		DX = (*iter).m_x - (*pred).m_x;
		DY = (*iter).m_y - (*pred).m_y;
		seglen = sqrt( (DX*DX) + (DY*DY) );
		liter += seglen;
		if (liter >= pos)
			break;
		++pred;
		++iter;
	}

	if (!iter.valid()) // position not inside the polyline, return last point!
		p = (*rbegin());
	else {
		if (seglen == 0.0) // *pred == *iter and pos is inbetween
			return (*pred);

		double segpos = seglen + pos - liter;

		double dx = DX * segpos / seglen;
		double dy = DY * segpos / seglen;

		p = (*pred);
		p.m_x += dx;
		p.m_y += dy;
	}

	return p;
}


//
void DPolyline::writeGML(ostream &stream) const
{
	Graph g;
	GraphAttributes ag(g);

	node u = NULL;
	node v = NULL;

	ListConstIterator<DPoint> iter;
	for (iter = begin(); iter.valid(); ++iter) {
		v = g.newNode();
		if (u != NULL)
			g.newEdge(u, v);
		u = v;

		ag.x(v) = (*iter).m_x;
		ag.y(v) = (*iter).m_y;
	}
	ag.writeGML(stream);
}


// outputs the GML-file, which illustrates the Polyline as Graph
void DPolyline::writeGML(const char *filename) const
{
	ofstream file(filename);
	writeGML(file);
}


// delete all consecutive double-points
void DPolyline::unify()
{
	if (empty()) return;
	ListIterator<DPoint> iter, next;
	for (iter = next = begin(), ++next; next.valid() && (size() > 2); ++next) {
		if (*iter == *next) {
			del(next);
			next = iter;
		} else
			iter = next;
	}
}


// deletes all points, which are not facets
void DPolyline::normalize()
{
	unify();

	ListIterator<DPoint> iter, next, onext;
	for (iter = begin(); iter.valid(); ++iter) {
		for( ; ; ) {
			next  = iter; next++;
			if (!next.valid()) break;
			onext = next, onext++;
			if (!onext.valid()) break;

			DSegment s1((*iter), (*next));
			DSegment s2((*next), (*onext));
			DRect    r ((*iter), (*onext));

			// is *next on the way from *iter to *onext?
			if (s1.slope() == s2.slope() && r.contains(*next))
				del(next);
			else
				break; /* while */
		}
	}
}


void DPolyline::normalize(DPoint src, DPoint tgt)
{
	if (empty())
		return;

	unify();
	ListIterator<DPoint> iter, next, onext;
	DPoint pCur = src;
	DPoint pNext;
	DPoint pNextNext;
	for (iter = begin(); iter.valid(); ++iter) {
		for( ; ; ) {

			if (!iter.valid())
				break;

			next  = iter;
			pNext = *next;
			next++;

			if (!next.valid()) {
				pNextNext = tgt;
			}
			else
				pNextNext = *next;


			DSegment s1(pCur, pNext);
			DSegment s2(pNext, pNextNext);
			DRect    r (pCur, pNextNext);

			// is *next on the way from *iter to *onext?
			if (s1.slope() == s2.slope() && r.contains(pNext)) {
				del(iter);
				iter = next;
			}
			else
				break; /* while */
		}
		if (iter.valid())
			pCur = *iter;
		else
			break;
	}
}


//
void DPolyline::convertToInt()
{
	ListIterator<DPoint> iter;
	for (iter = begin(); iter.valid(); ++iter) {
		DPoint &p = *iter;
		p.m_x = DRound(p.m_x * s_prec);
		p.m_y = DRound(p.m_y * s_prec);
	}
}

// Removed since I do not see that this makes sense... (CG)
//void DPolyline::reConvertToDouble()
//{
//    ListIterator<DPoint> iter;
//    for (iter = begin(); iter.valid(); ++iter) {
//        DPoint &p = *iter;
//        p.m_x = p.m_x / s_prec;
//        p.m_y = p.m_y / s_prec;
//    }
//}

//---------------------------------------------------------
// DLine
//---------------------------------------------------------

// gives the intersection-point between two lines, returns true, if any
// computes the crossing point between the (infinite) lines
// defined by the endpoints of the DLines, then checks if it
// lies within the two rectangles defined by the DLines endpoints
bool DLine::intersection(
	const DLine &line,
	DPoint &inter,
	bool endpoints) const
{
	double ix, iy;

	//do not return true if parallel edges are encountered
	if (slope() == line.slope()) return false;

	//two possible checks:
	// only check for overlap on endpoints if option parameter set,
	// compute crossing otherwise
	// or skip computation if endpoints overlap (can't have "real" crossing)
	// (currently implemented)
	//if (endpoints) {

	if (m_start == line.m_start || m_start == line.m_end) {
		inter = m_start;
		if (endpoints) return true;
		else return false;
	}

	if (m_end == line.m_start || m_end == line.m_end) {
		inter = m_end;
		if (endpoints) return true;
		else return false;
	}

	//}//if endpoints

	//if the edge is vertical, we cannot compute the slope
	if (isVertical())
		ix = m_start.m_x;
	else
		if (line.isVertical())
			ix = line.m_start.m_x;
		else
			ix = (line.yAbs() - yAbs())/(slope() - line.slope());

	//set iy to the value of the infinite line at xvalue ix
	//use a non-vertical line (can't be both, otherwise they're parallel)
	if (isVertical())
		iy = line.slope() * ix + line.yAbs();
	else
		iy = slope() * ix + yAbs();

	inter = DPoint(ix, iy); //the (infinite) lines cross point

	DRect tRect(line);
	DRect mRect(*this);

	return (tRect.contains(inter) && mRect.contains(inter));
}


// returns true, if line contains p
bool DLine::contains(const DPoint &p) const
{
	if (p == start() || p == end())
		return true;

	// check, if outside rect
	DRect r(start(), end());
	if (!r.contains(p))
		return false;

	if (dx() == 0.0) { // first check, if line is vertical
		if (DIsEqual       (p.m_x, start().m_x)                     &&
			DIsLessEqual   (p.m_y, (max(start().m_y, end().m_y)))   &&
			DIsGreaterEqual(p.m_y, (min(start().m_y, end().m_y))))
			return true;
		return false;
	}

	double dx2p = p.m_x - start().m_x;
	double dy2p = p.m_y - start().m_y;

	if (dx2p == 0.0) // dx() != 0.0, already checked
		return false;

	if (DIsEqual(slope(), (dy2p/dx2p)))
		return true;
	return false;
}


// gives the intersection with the horizontal axis 'horAxis', returns the number of intersections
// 0 = no, 1 = one, 2 = infinity or both end-points, e.g. parallel on this axis
int DLine::horIntersection(const double horAxis, double &crossing) const
{
	if (dy() == 0.0) {
		crossing = 0.0;
		if (m_start.m_y == horAxis)
			return 2;
		else
			return 0;
	}
	if (min(m_start.m_y, m_end.m_y) <= horAxis && max(m_start.m_y, m_end.m_y) >= horAxis) {
		crossing = (m_start.m_x * (m_end.m_y - horAxis) -
			m_end.m_x * (m_start.m_y - horAxis)   ) / dy();
		return 1;
	}
	else {
		crossing = 0.0;
		return 0;
	}
}


// gives the intersection with the vertical axis 'verAxis', returns the number of intersections
// 0 = no, 1 = one, 2 = infinity or both end-points, e.g. parallel on this axis
int DLine::verIntersection(const double verAxis, double &crossing) const
{
	if (dx() == 0.0) {
		crossing = 0.0;
		if (m_start.m_x == verAxis)
			return 2;
		else
			return 0;
	}
	if (min(m_start.m_x, m_end.m_x) <= verAxis && max(m_start.m_x, m_end.m_x) >= verAxis) {
		crossing = (m_start.m_y * (m_end.m_x - verAxis) -
			m_end.m_y * (m_start.m_x - verAxis)   ) / dx();
		return 1;
	}
	else {
		crossing = 0.0;
		return 0;
	}
}

// output the line
ostream &operator<<(ostream &os, const DLine &dl)
{
	os << "Line-Start: " << dl.start() << ", Line-End: " << dl.end();
	return os;
}


//---------------------------------------------------------
// DRect
//---------------------------------------------------------

// output the rect
ostream &operator<<(ostream &os, const DRect &dr)
{
	os << "Rect-LowerLeftPoint: " << dr.p1() << ", Rect-UpperRightPoint: " << dr.p2();
	return os;
}


//---------------------------------------------------------
// DScaler
//---------------------------------------------------------

// output the two rects in the scaler
ostream &operator<<(ostream &os, const DScaler &ds)
{
	os << "Scale from " << ds.from() << " to " << ds.to();
	return os;
}



//----------------------------------------------------------------
// DPolygon
//----------------------------------------------------------------

// gives the segment starting at point 'it'
DSegment DPolygon::segment(ListConstIterator<DPoint> it) const
{
	OGDF_ASSERT(!empty() && size() != 1);
	return DSegment(*it, *cyclicSucc(it));
}



// Assignment operator (for assigning from a rectangle).
DPolygon &DPolygon::operator=(const DRect &rect)
{
	clear();
	DRect  r1(rect);
	DRect  r2(rect);
	if (m_counterclock)
		r2.xInvert();
	else
		r2.yInvert();

	pushBack(r1.p1());
	pushBack(r2.p1());
	pushBack(r1.p2());
	pushBack(r2.p2());

	unify();
	return *this;
}


// inserts the point p, which must ly on the boarder of the polygon, between the two points p1 and p2
// returns the index to that point, which is inserted only once
ListIterator<DPoint> DPolygon::insertPoint(
	const DPoint &p,
	ListIterator<DPoint> p1,
	ListIterator<DPoint> p2)
{
	ListIterator<DPoint> i = p1;

	do {
		DSegment seg = segment(i);
		if (seg.contains(p)) {
			if (seg.start() == p)
				return i;
			else if (seg.end() == p) {
				i = cyclicSucc(i);
				return i;
			}
			else
				return insertAfter(p, i);
		}

		i = cyclicSucc(i);
	} while (i != p2);

	OGDF_ASSERT(false); // Point not in polygon, should not be reached!
	return i;
}


// inserts 'p' on every segment (a,b) with p in the open range ]a, b[
void DPolygon::insertCrossPoint(const DPoint &p)
{
	ListIterator<DPoint> i = begin();

	do {
		DSegment seg = segment(i);
		if (seg.contains(p))
			if (seg.start() != p && seg.end() != p)
				i = insertAfter(p, i);

		i = cyclicSucc(i);
	} while (i != begin());
}


//
int DPolygon::getCrossPoints(const DPolygon &p, List<DPoint> &crossPoints) const
{
	crossPoints.clear();

	ListConstIterator<DPoint> i, j;
	for (i = begin(); i.valid(); ++i) {
		DSegment s1 = segment(i);
		for (j = p.begin(); j.valid(); ++j) {
			DSegment s2 = p.segment(j);

			DPoint intersec;

			if (s1.intersection(s2, intersec))
				crossPoints.pushBack(intersec);
		}
	}
	// unify the list
	ListIterator<DPoint> k, l;
	for (k = crossPoints.begin(); k.valid(); ++k)
		for (l = k, ++l; l.valid(); ++l)
			if (*k == *l) {
				--l;
				crossPoints.del(crossPoints.cyclicSucc(l));
			}

			return crossPoints.size();
}



// delete all consecutive double-points
void DPolygon::unify()
{
	ListIterator<DPoint> iter, next;
	for (iter = begin(); iter.valid(); ++iter) {
		next = cyclicSucc(iter);
		while (*iter == *next) {
			del(next);
			next = cyclicSucc(iter);
			if (iter == next)
				break;
		}
	}
}


// deletes all points, which are not facets
void DPolygon::normalize()
{
	unify();

	ListIterator<DPoint> iter, next;
	for (iter = begin(); iter.valid(); ++iter) {
		for( ; ; ) {
			next = cyclicSucc(iter);
			DSegment s1 = segment(iter);
			DSegment s2 = segment(next);
			DRect    r    (*iter, *cyclicSucc(next));
			if (s1.slope() == s2.slope() && r.contains(*next))
				del(next);
			else
				break; // while
		}
	}
}


//
void DPolygon::writeGML(ostream &stream) const
{
	Graph g;
	GraphAttributes ag(g);

	node u = NULL;
	node v = NULL;
	node first = 0;

	ListConstIterator<DPoint> iter;
	for (iter = begin(); iter.valid(); ++iter) {
		v = g.newNode();
		if (u != NULL)
			g.newEdge(u, v);
		else
			first = v;
		u = v;

		ag.x(v) = (*iter).m_x;
		ag.y(v) = (*iter).m_y;
	}
	g.newEdge(v, first);

	ag.writeGML(stream);
}


// outputs the GML-file, which illustrates the Polygon as Graph
void DPolygon::writeGML(const char *filename) const
{
	ofstream file(filename);
	writeGML(file);
}


// Checks wether a Point /a p is inside the Poylgon or not.
bool DPolygon::containsPoint(DPoint &p) const
{
	if (size() < 3) {
		return false;
	}

	double angle = 0.0;
	DPolygon::const_iterator i = cyclicPred(begin());
	double lastangle = atan2((*i).m_y - p.m_y, (*i).m_x - p.m_x);
	double tempangle = 0.0;
	for (i = begin(); i != end(); i++)
	{
		tempangle = atan2((*i).m_y - p.m_y, (*i).m_x - p.m_x);
		double step = lastangle - tempangle;
		while (step > Math::pi) step -= 2.0*Math::pi;
		while (step < -Math::pi) step += 2.0*Math::pi;
		angle += step;
		lastangle = tempangle;
	}

	double d = angle / (2.0 * Math::pi);
	int rounds = static_cast<int>(d<0?d-.5:d+.5);

	return ((rounds % 2) != 0);
}


// outputs the polygon
ostream &operator<<(ostream &os, const DPolygon &dop)
{
	print(os, dop, ' ');
	return os;
}


}  // end namespace ogdf
