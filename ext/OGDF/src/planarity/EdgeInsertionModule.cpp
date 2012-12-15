/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of class EdgeInsertionModule
 *
 * \author Carsten Gutwenger
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


#include <ogdf/module/EdgeInsertionModule.h>


namespace ogdf {


#ifdef OGDF_DEBUG

bool EdgeInsertionModule::checkCrossingGens(const PlanRepUML &PG)
{
	edge e;
	forall_edges(e,PG) {
		Graph::EdgeType et = PG.typeOf(e);
		if (et != Graph::generalization && et != Graph::association)
			return false;
	}

	node v;
	forall_nodes(v,PG)
	{
		if (PG.typeOf(v) == PlanRepUML::dummy && v->degree() == 4) {
			adjEntry adj = v->firstAdj();

			edge e1 = adj->theEdge();
			edge e2 = adj->succ()->theEdge();

			if (PG.typeOf(e1) == Graph::generalization &&
				PG.typeOf(e2) == Graph::generalization)
				return false;
		}
	}

	return true;
}

#endif


} // end namespace ogdf

