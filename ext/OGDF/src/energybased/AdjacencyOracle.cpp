/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief  Implementation of class AjacencyOracle
 *
 * This class is used to efficiently test if two vertices
 * are adjacent. It is basically a wrapper for a 2D-Matrix.
 * This file contains the code for the construction of the
 * matrix and the query function.
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


#include <ogdf/internal/energybased/AdjacencyOracle.h>

namespace ogdf {

	//! Builds a 2D-array indexed by the numbers of vertices.
	/**
	* It uses only the part
	* of the matrix left of the diagonal (where the first index is smaller than the
	* second. For each pair of vertices, the corresponding entry in the matrix is set true
	* if and only if the two vertices are adjacent.
	*/
	AdjacencyOracle::AdjacencyOracle(const Graph &G) : m_nodeNum(G)
	{
		int i = 1;
		node v;
		forall_nodes(v,G) m_nodeNum[v] = i++;
		int nodeNum = i-1;
		m_adjacencyMatrix = new Array2D<bool> (1,i,1,i);
		for(i = 1; i < nodeNum; i++)
			for(int j = i+1; j <= nodeNum; j++)
				(*m_adjacencyMatrix)(i,j) = false;
		edge e;
		forall_edges(e,G) {
			int num1 = m_nodeNum[e->source()];
			int num2 = m_nodeNum[e->target()];
			(*m_adjacencyMatrix)(min(num1,num2),max(num1,num2)) = true;
		}
	}


	//! Returns true if two vertices are adjacent.
	/**
	* Note that only the entries in the 2D matrix where the first
	* index is smaller than the second is used and so wehave to
	* pay attention that the first index is smaller than the second.
	*/
	bool AdjacencyOracle::adjacent(const node v, const node w) const
	{
		int num1 = m_nodeNum[v];
		int num2 = m_nodeNum[w];
		return (*m_adjacencyMatrix)(min(num1,num2),max(num1,num2));
	}
}
