/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * This class represents the cut-constraints belonging to the ILP formulation.
 * Cut-constraints are dynamically separated be means of cutting plane methods.
 *
 * \author Mathias Jansen
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

#ifdef USE_ABACUS

#include <ogdf/internal/cluster/Cluster_CutConstraint.h>

using namespace ogdf;

CutConstraint::CutConstraint(ABA_MASTER *master, ABA_SUB *sub, List<nodePair> &edges) :
	BaseConstraint(master, sub, ABA_CSENSE::Greater, 1.0, true, true, true)
{
	ListConstIterator<nodePair> it;
	for (it = edges.begin(); it.valid(); ++it) {
		m_cutEdges.pushBack(*it);
	}
}


CutConstraint::~CutConstraint() {}


int CutConstraint::coeff(node n1, node n2) {
	ListConstIterator<nodePair> it;
	for (it = m_cutEdges.begin(); it.valid(); ++it) {
		if ( ((*it).v1 == n1 && (*it).v2 == n2) ||
			 ((*it).v2 == n1 && (*it).v1 == n2) )
		{return 1;}
	}
	return 0;
}

#endif // USE_ABACUS
