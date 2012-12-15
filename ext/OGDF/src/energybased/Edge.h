/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class Edge.
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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_EDGE_H
#define OGDF_EDGE_H


#include <ogdf/basic/Graph.h>


namespace ogdf {

class Edge
{
	//helping data structure for deleting parallel edges in class FMMMLayout and
	//Multilevel (needed for the bucket sort algorithm)

	//outputstream for Edge
	friend ostream &operator<< (ostream & output, const Edge & E)
	{
		output <<"edge_index " << E.e->index() << " Graph_ptr " << E.Graph_ptr << " angle"
			<< E.angle << " cut vertex " << E.cut_vertex->index();
		return output;
	}

	//inputstream for Edge
	friend istream &operator>> (istream & input,  Edge & E)
	{
		input >> E;//.e>>E.Graph_ptr;
		return input;
	}

public:
	//constructor
	Edge() {
		e = NULL;
		Graph_ptr = NULL;
		angle = 0;
		cut_vertex = NULL;
	}

	~Edge() { } //destructor

	void set_Edge (edge f,Graph* g_ptr) {
		Graph_ptr = g_ptr;
		e = f;
	}

	void set_Edge(edge f,double i,node c) {
		angle = i;
		e = f;
		cut_vertex = c;
	}

	Graph* get_Graph_ptr() const { return Graph_ptr; }
	edge get_edge() const { return e; }
	double get_angle() const { return angle; }
	node get_cut_vertex() const { return cut_vertex; }

private:
	edge e;
	Graph* Graph_ptr;
	double angle;
	node cut_vertex;
};


class EdgeMaxBucketFunc : public BucketFunc<Edge>
{
public:
	EdgeMaxBucketFunc() {};

	int getBucket(const Edge& E) { return get_max_index(E); }

private:
	//returns the maximum index of e
	int get_max_index(const Edge& E) {
		int source_index = E.get_edge()->source()->index();
		int target_index = E.get_edge()->target()->index();
		OGDF_ASSERT(source_index != target_index);
		if(source_index < target_index) return target_index;
		else /*if (source_index > target_index)*/ return source_index;
		//else cout<<"Error Edge::get_max_index() The Graph has a self loop"<<endl;
	}
};


class EdgeMinBucketFunc : public BucketFunc<Edge>
{
public:
	EdgeMinBucketFunc() { }

	int getBucket(const Edge& E) { return get_min_index(E); }

private:

	//returns the minimum index of e
	int get_min_index(const Edge& E)
	{
		int source_index = E.get_edge()->source()->index();
		int target_index = E.get_edge()->target()->index();
		OGDF_ASSERT(source_index != target_index);
		if(source_index < target_index) return source_index;
		else /*if (source_index > target_index)*/ return target_index;
		//else cout<<"Error Edge::get_min_index() The Graph has a self loop"<<endl;
	}
};

}//namespace ogdf
#endif

