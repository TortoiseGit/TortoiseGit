/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of crossings matrix.
 *
 * \author Andrea Wagner
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


#include <ogdf/layered/CrossingsMatrix.h>

namespace ogdf
{
//-------------------------------------------------------------------
//                        CrossingMatrix
//-------------------------------------------------------------------

CrossingsMatrix::CrossingsMatrix(const Hierarchy &H)
{
	int max_len = 0;
	for (int i = 0; i < H.size(); i++)
	{
		int len = H[i].size();
		if (len > max_len)
			max_len = len;
	}

	map.init(max_len);
	matrix.init(0, max_len - 1, 0, max_len - 1);
	m_bigM = 10000;
}


void CrossingsMatrix::init(Level &L)
{
	const Hierarchy &H = L.hierarchy();

	for (int i = 0; i < L.size(); i++)
	{
		map[i] = i;
		for (int j = 0; j < L.size(); j++)
			matrix(i,j) = 0;
	}

	for (int i = 0; i < L.size(); i++)
	{
		node v = L[i];
		const Array<node> &L_adj_i = L.adjNodes(v);

		for(int k = 0; k < L_adj_i.size(); k++)
		{
			int pos_adj_k = H.pos(L_adj_i[k]);
			for (int j = i + 1; j < L.size(); j++)
			{
				const Array<node> &L_adj_j = L.adjNodes(L[j]);

				for (int l = 0; l < L_adj_j.size(); l++)
				{
					int pos_adj_l = H.pos(L_adj_j[l]);
					matrix(i,j) += (pos_adj_k > pos_adj_l);
					matrix(j,i) += (pos_adj_l > pos_adj_k);
				}
			}
		}
	}
}


void CrossingsMatrix::init(Level &L, const EdgeArray<unsigned int> *edgeSubGraph)
{
	OGDF_ASSERT(edgeSubGraph != 0);
	init(L);

	const Hierarchy &H = L.hierarchy();
	const GraphCopy &HC(H);

	// calculate max number of graphs in edgeSubGraph
	edge d;
	int max = 0;
	forall_edges(d, HC.original()) {
		for (int i = 31; i > max; i--)
		{
			if((*edgeSubGraph)[d] & (1 << i))
				max = i;
		}
	}

	// calculation differs from ordinary init since we need the edges and not only the nodes
	for (int k = 0; k <= max; k++) {
		for (int i = 0; i < L.size(); i++)
		{
			node v = L[i];
			edge e;
			// H.direction == 1 if direction == upward
			if (H.direction()) {
				forall_adj_edges(e,v) {
					if ((e->source() == v) && ((*edgeSubGraph)[HC.original(e)] & (1 << k))) {
						int pos_adj_e = H.pos(e->target());
						for (int j = i+1; j < L.size(); j++) {
							node w = L[j];
							edge f;
							forall_adj_edges(f,w) {
								if ((f->source() == w) && ((*edgeSubGraph)[HC.original(f)] & (1 << k)))
								{
									int pos_adj_f = H.pos(f->target());
									matrix(i,j) += m_bigM * (pos_adj_e > pos_adj_f);
									matrix(j,i) += m_bigM * (pos_adj_f > pos_adj_e);
								}
							}
						}
					}
				}
			}
			else {
				forall_adj_edges(e,v) {
					if ((e->target() == v) && ((*edgeSubGraph)[HC.original(e)] & (1 << k))) {
						int pos_adj_e = H.pos(e->source());
						for (int j = i+1; j < L.size(); j++) {
							node w = L[j];
							edge f;
							forall_adj_edges(f,w) {
								if ((f->target() == w) && ((*edgeSubGraph)[HC.original(f)] & (1 << k)))
								{
									int pos_adj_f = H.pos(f->source());
									matrix(i,j) += m_bigM * (pos_adj_e > pos_adj_f);
									matrix(j,i) += m_bigM * (pos_adj_f > pos_adj_e);
								}
							}
						}
					}
				}
			}
		}
	}
}


} // end namespace ogdf
