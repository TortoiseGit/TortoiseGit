/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Auxiliary data structure for (node,int) pair.
 *
 * \author Stefan Hachul
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

//Data structure for representing nodes and an int value (needed for class ogdf/list)
//to perform bucket sort.

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_NODE_H
#define OGDF_NODE_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/Graph_d.h>
#include <iostream>

namespace ogdf {

	class Node
	{
		friend int value(const Node& A) { return A.value; }

		friend ostream &operator<< (ostream & output,const Node & A)
		{
			output <<"node index ";
			if(A.vertex == NULL)
				output<<"nil";
			else
				output<<A.vertex->index();
			output<<" value "<< A.value;
			return output;
		}

		friend istream &operator>> (istream & input,Node & A) {
			input >> A.value;
			return input;
		}

	public:
		Node() { vertex = NULL; value = 0; }        //constructor
		~Node() { }    //destructor


		void set_Node(node v,int a) { vertex = v; value = a; }
		int  get_value() const { return value; }
		node get_node() const { return vertex; }

	private:
		node vertex;
		int value ;
	};

}//namespace ogdf
#endif


