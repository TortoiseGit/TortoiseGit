/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * If some cluster has no connection to some other cluster,
 * the optimal solution might insert a new connection-edge between theese two clusters,
 * to obtain connectivity. Since the objective function minimizes the number
 * of new connection-edges, at most one new egde will be inserted between
 * two clusters that are not connected.
 * This behaviour of the LP-solution is guaranteed from the beginning, by creating
 * an initial constraint for each pair of non-connected clusters,
 * which is implemented by this constraint class.
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

#include <ogdf/internal/cluster/MaxCPlanar_MinimalClusterConnection.h>

using namespace ogdf;


MinimalClusterConnection::MinimalClusterConnection(ABA_MASTER *master, List<nodePair> &edges) :
	ABA_CONSTRAINT(master, 0, ABA_CSENSE::Less, 1.0, false, false, true)
{
	ListConstIterator<nodePair> it;
	for (it = edges.begin(); it.valid(); ++it) {
		m_edges.pushBack(*it);
	}
}


MinimalClusterConnection::~MinimalClusterConnection() {}


double MinimalClusterConnection::coeff(ABA_VARIABLE *v) {
	//TODO: speedup, we know between which nodepairs edges exist...
	EdgeVar *e = (EdgeVar *)v;
	ListConstIterator<nodePair> it;
	for (it = m_edges.begin(); it.valid(); ++it) {
		if ( ((*it).v1 == e->sourceNode() && (*it).v2 == e->targetNode()) ||
			 ((*it).v2 == e->sourceNode() && (*it).v1 == e->targetNode()) )
		{return 1.0;}
	}
	return 0.0;
}

#endif // USE_ABACUS
