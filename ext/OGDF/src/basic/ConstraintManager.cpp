/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class ConstraintManager that handles
 * drawing constraints.
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

//#include <ogdf/String.h>
#include <ogdf/basic/Constraints.h>
//#include <constraints/AnchorConstraint.h>
//#include <constraints/AlignmentConstraint.h>
//#include <constraints/SequenceConstraint.h>

namespace ogdf {

Constraint *ConstraintManager::createConstraintByName(const Graph &G, String *name)
{
//	if ((*name) == "Anchor") return new AnchorConstraint(G); else
//	if ((*name) == "Alignment") return new AlignmentConstraint(G,0.0); else
//	if ((*name) == "Sequence") return new SequenceConstraint(G,true); else
	return NULL;
}

String ConstraintManager::getClassnameOfConstraint(Constraint *c) {
//	if (c->getType()==AnchorConstraint::getStaticType()) return "Anchor";
//	if (c->getType()==AlignmentConstraint::getStaticType()) return "Alignment";
//	if (c->getType()==SequenceConstraint::getStaticType()) return "Sequence";
	return "";
}

}
