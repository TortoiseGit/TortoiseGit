/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class Constraint, which is a base
 * class for classes responsible for specifying and storing
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


#include "ogdf/basic/Constraints.h"
#include <ogdf/fileformats/DinoXmlParser.h>
#include <ogdf/fileformats/Ogml.h>


namespace ogdf {


bool Constraint::buildFromOgml(XmlTagObject* constraintTag, Hashing <String, node> * nodes)
{
	return true;
}

bool Constraint::storeToOgml(int id, ostream & os, int indentStep)
{
	return true;
}

/*
void Constraint::generateIndent(char ** indent, const int & indentSize)
{
	// free memory block (INFO: indent must point to an array of chars or to NULL)
	delete [] *indent;
	// instantiate array of chars
	*indent = new char[indentSize + 1];
	// if memory couldn't be allocated, we throw an exception
	if (!*indent) {
		OGDF_THROW(InsufficientMemoryException); // don't use regular throw!
	}
	// fill char array
	for(int i = 0; i < indentSize; ++i) {
		(*indent)[i] = INDENTCHAR;
	}
	// terminate string
	(*indent)[indentSize] = '\0';
}*/

} //end namespace ogdf
