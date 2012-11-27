/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class GraphReduction.
 *
 * \author Markus Chimani
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


#include <ogdf/graphalg/GraphReduction.h>

namespace ogdf {

GraphReduction::GraphReduction(const Graph &G) : m_pGraph(&G),
m_vOrig(), m_eOrig(), m_vReduction(), m_eReduction() {
	Graph::construct(*m_pGraph,m_vReduction,m_eReduction);

	int d;
	node v,ov;
	edge e1,e2;

	m_vOrig.init(*this);
	m_eOrig.init(*this);
	forall_nodes(v, *m_pGraph)
		m_vOrig[m_vReduction[v]] = v;

	forall_edges(e1, *m_pGraph)
		m_eOrig[m_eReduction[e1]].pushBack(e1);

	// remove selfloops
	forall_edges(e1, *this) {
		if(e1->isSelfLoop()) {
			m_eReduction[e1] = NULL;
			this->delEdge(e1);
		}
	}

	List<node> next;
	forall_nodes(v, *m_pGraph)
		next.pushBack(v);
	while(next.size()) {
		ov = next.front();
		next.popFront();
		v = m_vReduction[ov];
		if( v && (d=v->degree()) < 3) {
			if(d == 2) {
				e1 = v->firstAdj()->theEdge();
				e2 = v->lastAdj()->theEdge();

				if( e1->source() == v) {
					if(e2->source() == v) m_eOrig[e2].reverse();
					this->moveSource(e1, e2->opposite(v));
					for(ListConstIterator<edge> it = m_eOrig[e2].rbegin(); it.valid(); --it) {
						m_eReduction[*it] = e1;
						m_eOrig[e1].pushFront( *it );
					}
				} else {
					if(e2->target() == v) m_eOrig[e2].reverse();
					this->moveTarget(e1, e2->opposite(v));
					for(ListConstIterator<edge> it = m_eOrig[e2].begin(); it.valid(); ++it) {
						m_eReduction[*it] = e1;
						m_eOrig[e1].pushBack( *it );
					}
				}
				m_eOrig[e2].clear();
				this->delEdge(e2);
			} else if(d == 1) {
				e1 = v->firstAdj()->theEdge();
				const List<edge>& el = m_eOrig[e1];
				node nv;
				if(el.size() == 1)
					nv = el.front()->opposite(ov);
				else {
					bool front_e1 = el.front()->source() == ov || el.front()->target() == ov;
					edge e3;
					if(front_e1) {
						e2 = el.back();
						e3 = *(el.rbegin().pred());
					} else {
						e2 = el.front();
						e3 = *(el.begin().succ());
					}
					nv = (e2->source() == e3->source() || e2->source() == e3->target()) ? e2->target() : e2->source();
				}
				next.pushBack(nv);

				for(ListIterator<edge> it = m_eOrig[e1].begin(); it.valid(); it++)
					m_eReduction[*it] = 0;
				this->delEdge(e1);
			}
			m_vReduction[ ov ] = 0;
			this->delNode(v);
		}
	}

#ifdef OGDF_DEBUG
	edge em;
	forall_edges(em, *this) {
		node t = 0;
		OGDF_ASSERT( original(em).front()->source() == original(em->source()) );
		for(ListConstIterator<edge> it = original(em).begin(); it.valid(); ++it) {
			if(t) {
				OGDF_ASSERT( (*it)->source() == t);
			}
		}
		OGDF_ASSERT( original(em).back()->target() == original(em->target()) );
	}
#endif

}

}
