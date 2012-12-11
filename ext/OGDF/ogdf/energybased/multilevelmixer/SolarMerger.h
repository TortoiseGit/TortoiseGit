/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Merges nodes with solar system rules.
 *
 * \author Gereon Bartel
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

#include <ogdf/energybased/multilevelmixer/MultilevelBuilder.h>

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_SOLAR_MERGER_H
#define OGDF_SOLAR_MERGER_H


namespace ogdf {

class OGDF_EXPORT SolarMerger : public MultilevelBuilder
{
	struct PathData {
		PathData(int targetSun = 0, double length = 0.0f, int number = 0)
			: targetSun(targetSun), length(length), number(number) { }

		int targetSun;
		double length;
		int number;
	};

	bool m_sunSelectionSimple;
	bool m_massAsNodeRadius;
	NodeArray<unsigned int> m_mass;
	NodeArray<double> m_radius;
	NodeArray<int> m_celestial; // 0 = unknown, 1 = sun, 2 = planet, 3 = moon
	NodeArray<node> m_orbitalCenter;
	NodeArray<double> m_distanceToOrbit;
	NodeArray< std::vector<PathData> > m_pathDistances;
	std::map< int, std::map<int, PathData> > m_interSystemPaths;

	node sunOf(node object);
	double distanceToSun(node object, MultilevelGraph &MLG);
	void addPath(node sourceSun, node targetSun, double distance);
	void findInterSystemPaths(Graph &G, MultilevelGraph &MLG);
	int calcSystemMass(node v);
	bool collapsSolarSystem(MultilevelGraph &MLG, node sun, int level);
	bool buildOneLevel(MultilevelGraph &MLG);
	std::vector<node> selectSuns(MultilevelGraph &MLG);

public:
	SolarMerger(bool simple = false, bool massAsNodeRadius = false);

	void buildAllLevels(MultilevelGraph &MLG);
};

} // namespace ogdf

#endif
