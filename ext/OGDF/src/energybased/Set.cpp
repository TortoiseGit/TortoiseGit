/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class Set.
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


#include "Set.h"

namespace ogdf {

Set::Set()
{
	last_selectable_index_of_S_node = -1;
	S_node = NULL;
	using_S_node = false;
}


Set::~Set()
{
	if (using_S_node) delete [] S_node;
}


void Set::set_seed(int rand_seed)
{
	srand(rand_seed);
}


void Set::init_node_set(Graph& G)
{
	using_S_node = true;
	node v;

	S_node = new node[G.numberOfNodes()];
	position_in_node_set.init(G);

	forall_nodes(v,G)
	{
		S_node[v->index()] = v;
		position_in_node_set[v] = v->index();
	}
	last_selectable_index_of_S_node = G.numberOfNodes()-1;
}


bool Set::empty_node_set()
{
	if(last_selectable_index_of_S_node < 0)
		return true;
	else
		return false;
}


bool Set::is_deleted(node v)
{
	if (position_in_node_set[v] > last_selectable_index_of_S_node )
		return true;
	else
		return false;
}


void Set::delete_node(node del_node)
{
	int del_node_index = position_in_node_set[del_node];
	node last_selectable_node = S_node[last_selectable_index_of_S_node];

	S_node[last_selectable_index_of_S_node] = del_node;
	S_node[del_node_index] = last_selectable_node;
	position_in_node_set[del_node] = last_selectable_index_of_S_node;
	position_in_node_set[last_selectable_node] = del_node_index;
	last_selectable_index_of_S_node -=1;
}


//---------------- for set of nodes with uniform probability -------------------

node Set::get_random_node()
{
	int rand_index = randomNumber(0,last_selectable_index_of_S_node);
	node random_node =  S_node[rand_index];
	node last_selectable_node = S_node[last_selectable_index_of_S_node];

	S_node[last_selectable_index_of_S_node] = random_node;
	S_node[rand_index] = last_selectable_node;
	position_in_node_set[random_node] = last_selectable_index_of_S_node;
	position_in_node_set[last_selectable_node] = rand_index;
	last_selectable_index_of_S_node -=1;
	return random_node;
}


//---------------- for set of nodes with weighted  probability ------------------

void Set::init_node_set(Graph& G,NodeArray<NodeAttributes>& A)
{
	node v,v_adj;
	edge e_adj;

	init_node_set(G);
	mass_of_star.init(G);
	forall_nodes(v,G)
	{
		mass_of_star[v] = A[v].get_mass();
		forall_adj_edges(e_adj, v)
		{
			if(e_adj->source() != v)
				v_adj = e_adj->source();
			else
				v_adj = e_adj->target();
			mass_of_star[v] += A[v_adj].get_mass();
		}
	}
}

//---------------- for set of nodes with ``lower mass'' probability --------------

node Set::get_random_node_with_lowest_star_mass(int rand_tries)
{
	int rand_index,new_rand_index,min_mass;
	int i = 1;
	node random_node,new_rand_node,last_trie_node,last_selectable_node;

	//randomly select rand_tries distinct!!! nodes from S_node and select the one
	//with the lowest mass

	int last_trie_index = last_selectable_index_of_S_node;
	while( (i<= rand_tries) && (last_trie_index >= 0) )
	{//while
		last_trie_node = S_node[last_trie_index];
		new_rand_index = randomNumber(0,last_trie_index);
		new_rand_node = S_node[new_rand_index];
		S_node[last_trie_index] = new_rand_node;
		S_node[new_rand_index] = last_trie_node;
		position_in_node_set[new_rand_node] = last_trie_index;
		position_in_node_set[last_trie_node] = new_rand_index;

		if( (i == 1) || (min_mass > mass_of_star[S_node[last_trie_index]]) )
		{
			rand_index = last_trie_index;
			random_node = S_node[last_trie_index];
			min_mass = mass_of_star[random_node];
		}
		i++;
		last_trie_index -=1;
	}//while

	//now rand_index and random_node have been fixed
	last_selectable_node = S_node[last_selectable_index_of_S_node];
	S_node[last_selectable_index_of_S_node] = random_node;
	S_node[rand_index] = last_selectable_node;
	position_in_node_set[random_node] = last_selectable_index_of_S_node;
	position_in_node_set[last_selectable_node] = rand_index;
	last_selectable_index_of_S_node -=1;
	return random_node;
}


//---------------- for set of nodes with ``higher mass'' probability --------------

node Set::get_random_node_with_highest_star_mass(int rand_tries)
{
	int rand_index,new_rand_index,min_mass;
	int i = 1;
	node random_node,new_rand_node,last_trie_node,last_selectable_node;

	//randomly select rand_tries distinct!!! nodes from S_node and select the one
	//with the lowest mass

	int last_trie_index = last_selectable_index_of_S_node;
	while( (i<= rand_tries) && (last_trie_index >= 0) )
	{//while
		last_trie_node = S_node[last_trie_index];
		new_rand_index = randomNumber(0,last_trie_index);
		new_rand_node = S_node[new_rand_index];
		S_node[last_trie_index] = new_rand_node;
		S_node[new_rand_index] = last_trie_node;
		position_in_node_set[new_rand_node] = last_trie_index;
		position_in_node_set[last_trie_node] = new_rand_index;

		if( (i == 1) || (min_mass < mass_of_star[S_node[last_trie_index]]) )
		{
			rand_index = last_trie_index;
			random_node = S_node[last_trie_index];
			min_mass = mass_of_star[random_node];
		}
		i++;
		last_trie_index -=1;
	}//while

	//now rand_index and random_node have been fixed
	last_selectable_node = S_node[last_selectable_index_of_S_node];
	S_node[last_selectable_index_of_S_node] = random_node;
	S_node[rand_index] = last_selectable_node;
	position_in_node_set[random_node] = last_selectable_index_of_S_node;
	position_in_node_set[last_selectable_node] = rand_index;
	last_selectable_index_of_S_node -=1;
	return random_node;
}

}//namespace ogdf
