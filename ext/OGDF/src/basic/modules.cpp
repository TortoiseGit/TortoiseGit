/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of module base classes
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


#include <ogdf/module/LayoutModule.h>
#include <ogdf/module/AcyclicSubgraphModule.h>


namespace ogdf {

void AcyclicSubgraphModule::callAndReverse(Graph &G, List<edge> &reversed)
{
	call(G,reversed);

	ListConstIterator<edge> it;
	for(it = reversed.begin(); it.valid(); ++it)
		G.reverseEdge(*it);
}


void AcyclicSubgraphModule::callAndReverse(Graph &G)
{
	List<edge> reversed;
	callAndReverse(G,reversed);
}


void AcyclicSubgraphModule::callAndDelete(Graph &G)
{
	List<edge> arcSet;
	call(G,arcSet);

	ListConstIterator<edge> it;
	for(it = arcSet.begin(); it.valid(); ++it)
		G.delEdge(*it);
}


} // end namespace ogdf
