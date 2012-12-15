/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Places Nodes at the Positio of the merge-partner
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

#include <ogdf/energybased/multilevelmixer/ZeroPlacer.h>

namespace ogdf {

ZeroPlacer::ZeroPlacer()
:m_randomRange(1.0)
{
}


void ZeroPlacer::setRandomRange(double range)
{
	m_randomRange = range;
}


void ZeroPlacer::placeOneLevel(MultilevelGraph &MLG)
{
	int level = MLG.getLevel();
	while (MLG.getLevel() == level && MLG.getLastMerge() != 0)
	{
		placeOneNode(MLG);
	}
}


void ZeroPlacer::placeOneNode(MultilevelGraph &MLG)
{
	NodeMerge * NM = MLG.getLastMerge();
	node parent = MLG.getNode(NM->m_changedNodes[0]);
	node merged = MLG.undoLastMerge();
	MLG.x(merged, MLG.x(parent) + ((m_randomOffset)?(float)randomDouble(-m_randomRange, m_randomRange):0.f));
	MLG.y(merged, MLG.y(parent) + ((m_randomOffset)?(float)randomDouble(-m_randomRange, m_randomRange):0.f));
}

} // namespace ogdf
