/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class GraphConstraints that handles
 * drawing constraints specified for a specific graph.
 *
 * \author PG478
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
#include <ogdf/basic/Constraints.h>

namespace ogdf {

void GraphConstraints::nodeDeleted(node v)
{
	ListConstIterator<Constraint *> it;
	for(it = m_List.begin(); it.valid(); ++it)
	{
		Constraint *c = *it;
		c->nodeDeleted(v);
	}
}

List<Constraint *> GraphConstraints::getConstraintsOfType(int type)
{
	List<Constraint *> res;
	ListConstIterator<Constraint *> it;
	for(it = m_List.begin(); it.valid(); ++it)
	{
		Constraint *c = *it;
		if (type==c->getType())
		{
			if (c->isValid()) res.pushBack(c);
		}
	}
	return res;
}

}
