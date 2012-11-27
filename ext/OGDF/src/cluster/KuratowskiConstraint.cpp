/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * These constraints represent the planarity-constraints belonging to the
 * ILP formulation. These constraints are dynamically separated.
 * For the separation the planarity test algorithm by Boyer and Myrvold is used.
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

#include <ogdf/internal/cluster/KuratowskiConstraint.h>

using namespace ogdf;

KuratowskiConstraint::KuratowskiConstraint(ABA_MASTER *master, int nEdges, SListPure<nodePair> &ks) :
	ABA_CONSTRAINT(master, 0, ABA_CSENSE::Less, nEdges-1, true, false, true)
{
	SListConstIterator<nodePair> it;
	for (it = ks.begin(); it.valid(); ++it) {
		m_subdivision.pushBack(*it);
	}
}


KuratowskiConstraint::~KuratowskiConstraint() {}


double KuratowskiConstraint::coeff(ABA_VARIABLE *v) {
	EdgeVar *e = (EdgeVar*)v;
	for (ListConstIterator<nodePair> it = m_subdivision.begin(); it.valid(); ++it) {
		if( ((*it).v1 == e->sourceNode() && (*it).v2 == e->targetNode()) ||
			((*it).v1 == e->targetNode() && (*it).v2 == e->sourceNode()) )
		{return 1.0;}
	}
	return 0.0;
}

#endif // USE_ABACUS
