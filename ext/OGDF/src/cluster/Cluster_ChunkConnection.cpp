/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of initial cut-constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * A feasible ILP solution has to imply a completely connected, planar Sub-Clustergraph.
 * For each cluster that is not connected, additional connection edges have to be inserted
 * between the chunks of the cluster, to obtain c-connectivity.
 * Thus, initial constraints are added that guarantee this behaviour, if the number of chunks
 * is at most 3. If some cluster consists of more than 3 chunks, additional constraints
 * have to be separated during the algorithm.
 *
 * \author Mathias Jansen
 *
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

#include <ogdf/internal/cluster/Cluster_ChunkConnection.h>

using namespace ogdf;

ChunkConnection::ChunkConnection(ABA_MASTER *master, const ArrayBuffer<node>& chunk, const ArrayBuffer<node>& cochunk) :
	BaseConstraint(master, 0, ABA_CSENSE::Greater, 1.0, false, false, true)
{
	chunk.compactMemcpy(m_chunk);
	cochunk.compactMemcpy(m_cochunk);
}


ChunkConnection::~ChunkConnection() {}


int ChunkConnection::coeff(node n1, node n2) {
	//TODO: speedup
	int i,j;
	forall_arrayindices(i,m_chunk) {
		if(m_chunk[i] == n1) {
			forall_arrayindices(j,m_cochunk) {
				if(m_cochunk[j] == n2) {
					return 1;
				}
			}
			return 0;
		} else if(m_chunk[i] == n2) {
			forall_arrayindices(j,m_cochunk) {
				if(m_cochunk[j] == n1) {
					return 1;
				}
			}
			return 0;
		}
	}
	return 0;
}

#endif // USE_ABACUS
