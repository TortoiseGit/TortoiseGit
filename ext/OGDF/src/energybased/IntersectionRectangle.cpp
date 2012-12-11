/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class IntersectionRectangle (checks
 * overlap of rectangles).
 *
 * \author Rene Weiskircher
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


#include <ogdf/internal/energybased/IntersectionRectangle.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/geometry.h>

namespace ogdf {

	// this constructor gets the center point, width and height and sets the corners, the
	// center and the area
	IntersectionRectangle::IntersectionRectangle(const DPoint &c, double width, double height)
	{
		m_center = c;
		double halfwidth = 0.5*width;
		double halfheight = 0.5*height;
		m_p1.m_x = m_center.m_x - halfwidth;
		m_p1.m_y = m_center.m_y - halfheight;
		m_p2.m_x = m_center.m_x + halfwidth;
		m_p2.m_y = m_center.m_y + halfheight;
		m_area = width * height;
	}


	// two rectangles intersect if one of the center points is contained in the other rectangle
	// or if one of the corners of the second rectangle is contained in the first
	bool IntersectionRectangle::intersects(const IntersectionRectangle &ir) const
	{
		bool intersect = false;
		if(inside(ir.m_center) || ir.inside(m_center)) intersect = true;
		else {
			DPoint p1(ir.m_p1.m_x, ir.m_p2.m_y);
			DPoint p2(ir.m_p2.m_x, ir.m_p1.m_y);
			intersect = inside(p1) || inside(p2) || inside(ir.m_p1) || inside(ir.m_p2);
		}
		return intersect;
	}


	// This makes the lower left point the first point of the rectangle, computes
	// the coordinates of the center point and the area.
	void IntersectionRectangle::init() {
		if (width() < 0)
			swap(m_p2.m_x, m_p1.m_x);
		if (height() < 0)
			swap(m_p2.m_y, m_p1.m_y);
		m_area = (m_p2.m_x-m_p1.m_x)*(m_p2.m_y-m_p1.m_y);
		m_center.m_x = m_p1.m_x + 0.5*(m_p2.m_x-m_p1.m_x);
		m_center.m_y = m_p1.m_y + 0.5*(m_p2.m_y-m_p1.m_y);
	}


	// this returns the rectangle defined by the intersection of this and ir. If the intersection
	// is empty, an empty rectangle is returned.
	IntersectionRectangle IntersectionRectangle::intersection(
		const IntersectionRectangle &ir) const
	{
		double top1    = m_p2.m_y;
		double bottom1 = m_p1.m_y;
		double left1   = m_p1.m_x;
		double right1  = m_p2.m_x;

		double top2    = ir.m_p2.m_y;
		double bottom2 = ir.m_p1.m_y;
		double left2   = ir.m_p1.m_x;
		double right2  = ir.m_p2.m_x;

		OGDF_ASSERT(top1 >= bottom1);
		OGDF_ASSERT(left1 <= right1);
		OGDF_ASSERT(top2 >= bottom2);
		OGDF_ASSERT(left2 <= right2);

		double bottomInter = max(bottom1,bottom2);
		double topInter    = min(top1,top2);
		double leftInter   = max(left1,left2);
		double rightInter  = min(right1,right2);

		if(bottomInter > topInter)   return IntersectionRectangle();
		if(leftInter   > rightInter) return IntersectionRectangle();

		return IntersectionRectangle(DPoint(leftInter,bottomInter),DPoint(rightInter,topInter));
	}


	// computes distance to other rectangle
	double IntersectionRectangle::distance(const IntersectionRectangle &ir) const
	{
		double dist = 0.0;
		if(!intersects(ir)) {
			dist = parallelDist(top(),ir.bottom());
			dist = min(dist, parallelDist(left(),ir.right()));
			dist = min(dist, parallelDist(right(),ir.left()));
			dist = min(dist, parallelDist(bottom(),ir.top()));
		}
		return dist;
	}


	// computes distance between two parallel lines
	double IntersectionRectangle::parallelDist(const DLine& d1, const DLine& d2) const
	{
		OGDF_ASSERT((d1.isHorizontal() && d2.isHorizontal()) ||
			(d1.isVertical() && d2.isVertical()));
		double d1min, d1max, d2min, d2max, paraDist, dist;
		if(d1.isVertical()) {
			d1min = d1.start().m_y;
			d1max = d1.end().m_y;
			d2min = d2.start().m_y;
			d2max = d2.end().m_y;
			paraDist = fabs(d1.start().m_x - d2.start().m_x);
		}
		else {
			d1min = d1.start().m_x;
			d1max = d1.end().m_x;
			d2min = d2.start().m_x;
			d2max = d2.end().m_x;
			paraDist = fabs(d1.start().m_y - d2.start().m_y);
		}
		if(d1min > d1max) swap(d1min,d1max);
		if(d2min > d2max) swap(d2min,d2max);
		if(d1min > d2max || d2min > d1max) { // no overlap
			dist = pointDist(d1.start(),d2.start());
			dist = min(dist,pointDist(d1.start(),d2.end()));
			dist = min(dist,pointDist(d1.end(),d2.start()));
			dist = min(dist,pointDist(d1.end(),d2.end()));
		}
		else
			dist = paraDist; // segments overlap
		return dist;
	}


	ostream& operator<<(ostream& out,const IntersectionRectangle &ir)
	{
		out << "\nCenter: " << ir.m_center;
		out << "\nLower left corner: " << ir.m_p1;
		out << "\nUpper right corner: " << ir.m_p2;
		out << "\nWidth: " << ir.width();
		out << "\nHeight: " << ir.height();
		out << "\nArea: " << ir.m_area;
		return out;
	}


	void IntersectionRectangle::move(const DPoint& newCenter)
	{
		double dX = newCenter.m_x - m_center.m_x;
		double dY = newCenter.m_y - m_center.m_y;
		m_center = newCenter;
		m_p1.m_x += dX;
		m_p1.m_y += dY;
		m_p2.m_x += dX;
		m_p2.m_y += dY;
	}

} // end namespace ogdf

