/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief A simple embedder algorithm.
 *
 * \author Thorsten Kerkhof
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

#include <ogdf/planarity/SimpleEmbedder.h>

namespace ogdf {

	void SimpleEmbedder::call(Graph& G, adjEntry& adjExternal)
	{
		OGDF_ASSERT(isPlanar(G));

		//----------------------------------------------------------
		//
		// determine embedding of G
		//

		// We currently compute any embedding and choose the maximal face
		// as external face

		// if we use FixedEmbeddingInserter, we have to re-use the computed
		// embedding, otherwise crossing nodes can turn into "touching points"
		// of edges (alternatively, we could compute a new embedding and
		// finally "remove" such unnecessary crossings).
		adjExternal = 0;
		if(!G.representsCombEmbedding())
			planarEmbed(G);

		if (G.numberOfEdges() > 0)
		{
			CombinatorialEmbedding E(G);
			//face fExternal = E.maximalFace();
			face fExternal = findBestExternalFace(G, E);
			adjExternal = fExternal->firstAdj();
		}
	}


	face SimpleEmbedder::findBestExternalFace(
		const PlanRep& PG,
		const CombinatorialEmbedding& E)
	{
		FaceArray<int> weight(E);

		face f;
		forall_faces(f,E)
			weight[f] = f->size();

		node v;
		forall_nodes(v,PG)
		{
			if(PG.typeOf(v) != Graph::generalizationMerger)
				continue;

			adjEntry adj;
			forall_adj(adj,v) {
				if(adj->theEdge()->source() == v)
					break;
			}

			OGDF_ASSERT(adj->theEdge()->source() == v);

			node w = adj->theEdge()->target();
			bool isBase = true;

			adjEntry adj2;
			forall_adj(adj2, w) {
				edge e = adj2->theEdge();
				if(e->target() != w && PG.typeOf(e) == Graph::generalization) {
					isBase = false;
					break;
				}
			}

			if(isBase == false)
				continue;

			face f1 = E.leftFace(adj);
			face f2 = E.rightFace(adj);

			weight[f1] += v->indeg();
			if(f2 != f1)
				weight[f2] += v->indeg();
		}

		face fBest = E.firstFace();
		forall_faces(f,E)
			if(weight[f] > weight[fBest])
				fBest = f;

		return fBest;
	}

} // end namespace ogdf
