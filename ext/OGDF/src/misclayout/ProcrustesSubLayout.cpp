/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class ProcrustesSubLayout
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
 * Version 2 or 3 as published by the Free Software Foundation
 * and appearing in the files LICENSE_GPL_v2.txt and
 * LICENSE_GPL_v3.txt included in the packaging of this file.
 *
 * \par
 * In addition, as a special exception, you have permission to link
 * this software with the libraries of the COIN-OR Osi project
 * (http://www.coin-or.org/projects/Osi.xml), all libraries required
 * by Osi, and all LP-solver libraries directly supported by the
 * COIN-OR Osi project, and distribute executables, as long as
 * you follow the requirements of the GNU General Public License
 * in regard to all of the software in the executable aside from these
 * third-party libraries.
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

#include <ogdf/misclayout/ProcrustesSubLayout.h>

namespace ogdf {

//! Creates an instance of circular layout.
ProcrustesSubLayout::ProcrustesSubLayout(LayoutModule* pSubLayout) : m_pSubLayout(pSubLayout), m_scaleToInitialLayout(true)
{
	// nothing
}

void ProcrustesSubLayout::copyFromGraphAttributes(const GraphAttributes& graphAttributes, ProcrustesPointSet& pointSet)
{
	const Graph& graph = graphAttributes.constGraph();
	int i = 0;
	for (node v = graph.firstNode(); v; v = v->succ())
	{
		pointSet.set(i, graphAttributes.x(v), graphAttributes.y(v));
		i++;
	}
}

void ProcrustesSubLayout::translate(GraphAttributes& graphAttributes, double dx, double dy)
{
	const Graph& graph = graphAttributes.constGraph();
	for (node v = graph.firstNode(); v; v = v->succ())
	{
		graphAttributes.x(v) += dx;
		graphAttributes.y(v) += dy;
	}
}

void ProcrustesSubLayout::rotate(GraphAttributes& graphAttributes, double angle)
{
	const Graph& graph = graphAttributes.constGraph();
	for (node v = graph.firstNode(); v; v = v->succ())
	{
		double x = cos(angle)*graphAttributes.x(v) - sin(angle)*graphAttributes.y(v);
		double y = sin(angle)*graphAttributes.x(v) + cos(angle)*graphAttributes.y(v);
		graphAttributes.x(v) = x;
		graphAttributes.y(v) = y;
	}
}

void ProcrustesSubLayout::scale(GraphAttributes& graphAttributes, double scale)
{
	const Graph& graph = graphAttributes.constGraph();
	for (node v = graph.firstNode(); v; v = v->succ())
	{
		graphAttributes.x(v) *= scale;
		graphAttributes.y(v) *= scale;
	}
}

void ProcrustesSubLayout::flipY(GraphAttributes& graphAttributes)
{
	const Graph& graph = graphAttributes.constGraph();
	for (node v = graph.firstNode(); v; v = v->succ())
	{
		graphAttributes.y(v) = -graphAttributes.y(v);
	}
}

//! Computes a circular layout for graph attributes \a GA.
void ProcrustesSubLayout::call(GraphAttributes& graphAttributes)
{
	// any layout?
	if (!m_pSubLayout)
		return;

	const Graph& graph = graphAttributes.constGraph();

	// the nodes as points from the initial layout before
	ProcrustesPointSet initialPointSet(graph.numberOfNodes());
	copyFromGraphAttributes(graphAttributes, initialPointSet);
	initialPointSet.normalize();

	// call the layout algorithm
	m_pSubLayout->call(graphAttributes);

	// two new pointsets, one which holds the new layout
	ProcrustesPointSet newPointSet(graph.numberOfNodes());
	copyFromGraphAttributes(graphAttributes, newPointSet);
	newPointSet.normalize();
	newPointSet.rotateTo(initialPointSet);

	// and one which holds the new layout with flipped y coords
	ProcrustesPointSet newFlippedPointSet(graph.numberOfNodes());
	copyFromGraphAttributes(graphAttributes, newFlippedPointSet);
	newFlippedPointSet.normalize(true);
	newFlippedPointSet.rotateTo(initialPointSet);

	// which layout is better
	bool useFlippedLayout = initialPointSet.compare(newFlippedPointSet) < initialPointSet.compare(newPointSet);
	double scaleFactor = initialPointSet.scale();
	if (useFlippedLayout)
	{
		reverseTransform(graphAttributes, newFlippedPointSet);
		if (!m_scaleToInitialLayout)
			scaleFactor = newFlippedPointSet.scale();
	}
	else
	{
		reverseTransform(graphAttributes, newPointSet);
		if (!m_scaleToInitialLayout)
			scaleFactor = newFlippedPointSet.scale();
	}

	// everything is uniform and rotated. now get back to the initial layout
	scale(graphAttributes, scaleFactor);
	translate(graphAttributes, initialPointSet.originX(), initialPointSet.originY());
}

void ProcrustesSubLayout::reverseTransform(GraphAttributes& graphAttributes, const ProcrustesPointSet& pointSet)
{
	translate(graphAttributes, -pointSet.originX(), -pointSet.originY());
	if (pointSet.isFlipped())
		flipY(graphAttributes);
	scale(graphAttributes, 1.0/pointSet.scale());
	rotate(graphAttributes, pointSet.angle());
}

ProcrustesPointSet::ProcrustesPointSet(int numPoints) :
	m_numPoints(numPoints),
	m_originX(0.0),
	m_originY(0.0),
	m_scale(1.0),
	m_angle(0.0),
	m_flipped(false)
{
	m_x = new double[m_numPoints];
	m_y = new double[m_numPoints];
}

ProcrustesPointSet::~ProcrustesPointSet()
{
	delete[] m_x;
	delete[] m_y;
}

void ProcrustesPointSet::normalize(bool flip)
{
	// upppppps
	if (!m_numPoints)
		return;

	// calculate the avg center
	m_originX = 0.0;
	m_originY = 0.0;
	for (int i = 0; i < m_numPoints; ++i)
	{
		m_originX += m_x[i];
		m_originY += m_y[i];
	}
	// average
	m_originX /= (double)m_numPoints;
	m_originY /= (double)m_numPoints;

	// center points and calculate root mean square distance (RMDS)
	if (m_numPoints > 1)
	{
		m_scale = 0.0;
		for (int i = 0; i < m_numPoints; ++i)
		{
			// translate
			m_x[i] -= m_originX;
			m_y[i] -= m_originY;
			// while we are here: sum up for RMDS
			m_scale += m_x[i]*m_x[i] + m_y[i]*m_y[i];
		}
		// the ROOT MEAN in root mean square distance
		m_scale = sqrt(m_scale / (double)m_numPoints);
	} else {
		m_scale = 1.0;
	}

	// rescale all points to uniform scale
	double scaleInv = 1.0 / m_scale;
	for (int i = 0; i < m_numPoints; ++i)
	{
		// scaling
		m_x[i] *= scaleInv;
		m_y[i] *= scaleInv;
	}

	m_flipped = flip;
	if (m_flipped)
	{
		for (int i = 0; i < m_numPoints; ++i)
		{
			m_y[i] = -m_y[i];
		}
	}
}

void ProcrustesPointSet::rotateTo(const ProcrustesPointSet& other)
{
	// calculate angle between the two normalized point sets
	double a = 0.0;
	double b = 0.0;
	for (int i = 0; i < m_numPoints; ++i)
	{
		a += m_x[i]*other.m_y[i] - m_y[i]*other.m_x[i];
		b += m_x[i]*other.m_x[i] + m_y[i]*other.m_y[i];
	}

	// note: atan and me never have been friends really.
	// i hope i'm not missing anything here!
	m_angle	= atan2(a, b);

	// now rotate the points
	for (int i = 0; i < m_numPoints; ++i)
	{
		double x = cos(m_angle)*m_x[i] - sin(m_angle)*m_y[i];
		double y = sin(m_angle)*m_x[i] + cos(m_angle)*m_y[i];
		m_x[i] = x;
		m_y[i] = y;
	}
}

double ProcrustesPointSet::compare(const ProcrustesPointSet& other) const
{
	double result = 0.0;
	// calculate the comparison value
	for (int i = 0; i < m_numPoints; ++i)
	{
		double dx = other.m_x[i] - m_x[i];
		double dy = other.m_y[i] - m_y[i];
		result += dx*dx + dy*dy;
	}
	// somehow similiar to rmds, see wikipedia for further details
	result = sqrt(result);
	return result;
}

} // end of namespace ogdf
