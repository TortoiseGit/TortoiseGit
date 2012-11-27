/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Places nodes at the barycenter position of its neighbors.
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

#include <ogdf/energybased/multilevelmixer/BarycenterPlacer.h>

namespace ogdf {

void BarycenterPlacer::placeOneLevel(MultilevelGraph &MLG)
{
	int level = MLG.getLevel();
	while (MLG.getLevel() == level && MLG.getLastMerge() != 0)
	{
		placeOneNode(MLG);
	}
}


void BarycenterPlacer::placeOneNode(MultilevelGraph &MLG)
{
	node merged = MLG.undoLastMerge();
	double x = 0.0;
	double y = 0.0;
	double i = 0.0;
	adjEntry adj;
	forall_adj(adj, merged) {
		if(m_weightedPositions) {
			double weight = 1.0 / MLG.weight(adj->theEdge());
			i = i + weight;
			x += MLG.x(adj->twinNode()) * weight;
			y += MLG.y(adj->twinNode()) * weight;
		} else {
			i = i + 1.f;
			x += MLG.x(adj->twinNode());
			y += MLG.y(adj->twinNode());
		}
	}

	OGDF_ASSERT(i > 0);
	x = x / i;
	y = y / i;

	MLG.x(merged, x + ((m_randomOffset)?(float)randomDouble(-1.0, 1.0):0.f));
	MLG.y(merged, y + ((m_randomOffset)?(float)randomDouble(-1.0, 1.0):0.f));
}


BarycenterPlacer::BarycenterPlacer()
:m_weightedPositions(false)
{
}


void BarycenterPlacer::weightedPositionPriority( bool on )
{
	m_weightedPositions = on;
}

} // namespace ogdf
