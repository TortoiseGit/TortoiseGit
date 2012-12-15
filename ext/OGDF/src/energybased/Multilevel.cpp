/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class Multlevel (used by FMMMLayout).
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


#include "Multilevel.h"
#include "Set.h"
#include "Node.h"
#include <ogdf/basic/Array.h>
#include <ogdf/basic/Math.h>
#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/energybased/FMMMLayout.h>


namespace ogdf {

void Multilevel::create_multilevel_representations(
	Graph& G,
	NodeArray<NodeAttributes>& A,
	EdgeArray<EdgeAttributes>& E,
	int rand_seed,
	int galaxy_choice,
	int min_Graph_size,
	int random_tries,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray <NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int & max_level)
{
	//make initialisations;
	srand(rand_seed);
	G_mult_ptr[0] = &G; //init graph at level 0 to the original undirected simple
	A_mult_ptr[0] = &A; //and loopfree connected graph G/A/E
	E_mult_ptr[0] = &E;

	int bad_edgenr_counter = 0;
	int act_level = 0;
	Graph* act_Graph_ptr = G_mult_ptr[0];

	while( (act_Graph_ptr->numberOfNodes() > min_Graph_size) &&
		edgenumbersum_of_all_levels_is_linear(G_mult_ptr,act_level,bad_edgenr_counter) )
	{
		Graph* G_new = new (Graph);
		NodeArray<NodeAttributes>* A_new = OGDF_NEW NodeArray<NodeAttributes>;
		EdgeArray<EdgeAttributes>* E_new = OGDF_NEW EdgeArray<EdgeAttributes>;
		G_mult_ptr[act_level+1] = G_new;
		A_mult_ptr[act_level+1] = A_new;
		E_mult_ptr[act_level+1] = E_new;

		init_multilevel_values(G_mult_ptr,A_mult_ptr,E_mult_ptr,act_level);
		partition_galaxy_into_solar_systems(G_mult_ptr,A_mult_ptr,E_mult_ptr,rand_seed,
			galaxy_choice,random_tries,act_level);
		collaps_solar_systems(G_mult_ptr,A_mult_ptr,E_mult_ptr,act_level);

		act_level++;
		act_Graph_ptr = G_mult_ptr[act_level];
	}
	max_level = act_level;
}


bool Multilevel::edgenumbersum_of_all_levels_is_linear(
	Array<Graph*> &G_mult_ptr,
	int act_level,
	int& bad_edgenr_counter)
{
	if(act_level == 0)
		return true;
	else
	{
		if(G_mult_ptr[act_level]->numberOfEdges()<=
			0.8 * double (G_mult_ptr[act_level-1]->numberOfEdges()))
			return true;
		else if(bad_edgenr_counter < 5)
		{
			bad_edgenr_counter++;
			return true;
		}
		else
			return false;
	}
}


inline void  Multilevel::init_multilevel_values(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int level)
{
	node v;
	forall_nodes(v,*G_mult_ptr[level])
		(*A_mult_ptr[level])[v].init_mult_values();

	edge e;
	forall_edges(e,*G_mult_ptr[level])
		(*E_mult_ptr[level])[e].init_mult_values();
}


inline void Multilevel::partition_galaxy_into_solar_systems(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int rand_seed,
	int galaxy_choice,
	int random_tries,
	int act_level)
{
	create_suns_and_planets(G_mult_ptr, A_mult_ptr, E_mult_ptr, rand_seed, galaxy_choice,
		random_tries, act_level);
	create_moon_nodes_and_pm_nodes(G_mult_ptr, A_mult_ptr, E_mult_ptr, act_level);
}


void Multilevel::create_suns_and_planets(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int rand_seed,
	int galaxy_choice,
	int random_tries,
	int act_level)
{
	Set Node_Set;
	node v, sun_node, planet_node, newNode, pos_moon_node;
	edge sun_edge, e;
	double dist_to_sun;
	List<node> planet_nodes;
	List<node> sun_nodes;

	//make initialisations
	sun_nodes.clear();
	Node_Set.set_seed(rand_seed); //set seed for random number generator
	forall_nodes(v,*G_mult_ptr[act_level])
		if(act_level == 0) (*A_mult_ptr[act_level])[v].set_mass(1);
	if(galaxy_choice == FMMMLayout::gcUniformProb)
		Node_Set.init_node_set(*G_mult_ptr[act_level]);
	else //galaxy_choice != gcUniformProb in FMMMLayout
		Node_Set.init_node_set(*G_mult_ptr[act_level],*A_mult_ptr[act_level]);


	while (!Node_Set.empty_node_set())
	{//while
		//randomly select a sun node
		planet_nodes.clear();
		if(galaxy_choice == FMMMLayout::gcUniformProb)
			sun_node = Node_Set.get_random_node();
		else if (galaxy_choice == FMMMLayout::gcNonUniformProbLowerMass)
			sun_node = Node_Set.get_random_node_with_lowest_star_mass(random_tries);
		else //galaxy_choice == FMMMLayout::gcNonUniformProbHigherMass
			sun_node = Node_Set.get_random_node_with_highest_star_mass(random_tries);
		sun_nodes.pushBack(sun_node);

		//create new node at higher level that represents the collapsed solar_system
		newNode = G_mult_ptr[act_level+1]->newNode();

		//update information for sun_node
		(*A_mult_ptr[act_level])[sun_node].set_higher_level_node(newNode);
		(*A_mult_ptr[act_level])[sun_node].set_type(1);
		(*A_mult_ptr[act_level])[sun_node].set_dedicated_sun_node(sun_node);
		(*A_mult_ptr[act_level])[sun_node].set_dedicated_sun_distance(0);

		//update information for planet_nodes
		forall_adj_edges(sun_edge,sun_node)
		{
			dist_to_sun = (*E_mult_ptr[act_level])[sun_edge].get_length();
			if (sun_edge->source() != sun_node)
				planet_node = sun_edge->source();
			else
				planet_node =  sun_edge->target();
			(*A_mult_ptr[act_level])[planet_node].set_type(2);
			(*A_mult_ptr[act_level])[planet_node].set_dedicated_sun_node(sun_node);
			(*A_mult_ptr[act_level])[planet_node].set_dedicated_sun_distance(dist_to_sun);
			planet_nodes.pushBack(planet_node);
		}

		//delete all planet_nodes and possible_moon_nodes from Node_Set

		ListConstIterator<node> planet_node_ptr;
		//forall_listiterators(node,planet_node_ptr,planet_nodes)
		for(planet_node_ptr = planet_nodes.begin(); planet_node_ptr.valid(); ++planet_node_ptr)
			if(!Node_Set.is_deleted(*planet_node_ptr))
				Node_Set.delete_node(*planet_node_ptr);

		for(planet_node_ptr = planet_nodes.begin(); planet_node_ptr.valid(); ++planet_node_ptr)
			//forall_listiterators(node,planet_node_ptr,planet_nodes)
		{
			forall_adj_edges(e,*planet_node_ptr)
			{
				if(e->source() == *planet_node_ptr)
					pos_moon_node = e->target();
				else
					pos_moon_node = e->source();
				if(!Node_Set.is_deleted(pos_moon_node))
					Node_Set.delete_node(pos_moon_node);
			}
		}
	}//while

	//init *A_mult_ptr[act_level+1] and set NodeAttributes information for new nodes
	A_mult_ptr[act_level+1]->init(*G_mult_ptr[act_level+1]);
	forall_listiterators(node, sun_node_ptr, sun_nodes)
	{
		newNode = (*A_mult_ptr[act_level])[*sun_node_ptr].get_higher_level_node();
		(*A_mult_ptr[act_level+1])[newNode].set_NodeAttributes((*A_mult_ptr[act_level])
			[*sun_node_ptr].get_width(),
			(*A_mult_ptr[act_level])
			[*sun_node_ptr].get_height(),
			(*A_mult_ptr[act_level])
			[*sun_node_ptr].get_position(),
			*sun_node_ptr,NULL);
		(*A_mult_ptr[act_level+1])[newNode].set_mass(0);
	}
}


void Multilevel::create_moon_nodes_and_pm_nodes(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int act_level)
{
	edge e;
	node v, nearest_neighbour_node, neighbour_node, dedicated_sun_node;
	double dist_to_nearest_neighbour, dedicated_sun_distance;
	bool first_adj_edge;
	int neighbour_type;
	edge moon_edge;

	forall_nodes(v,*G_mult_ptr[act_level])
		if((*A_mult_ptr[act_level])[v].get_type() == 0) //a moon node
		{//forall
			//find nearest neighbour node
			first_adj_edge = true;
			forall_adj_edges(e,v)
			{//forall2
				if(v == e->source())
					neighbour_node = e->target();
				else
					neighbour_node = e->source();
				neighbour_type = (*A_mult_ptr[act_level])[neighbour_node].get_type();
				if( (neighbour_type == 2) || (neighbour_type == 3) )
				{//if_1
					if(first_adj_edge)
					{//if
						first_adj_edge = false;
						moon_edge = e;
						dist_to_nearest_neighbour = (*E_mult_ptr[act_level])[e].get_length();
						nearest_neighbour_node =  neighbour_node;
					}//if
					else if(dist_to_nearest_neighbour >(*E_mult_ptr[act_level])[e].get_length())
					{//else
						moon_edge = e;
						dist_to_nearest_neighbour = (*E_mult_ptr[act_level])[e].get_length();
						nearest_neighbour_node = neighbour_node;
					}//else
				}//if_1
			}//forall2
			//find dedic. solar system for v and update information in *A_mult_ptr[act_level]
			//and *E_mult_ptr[act_level]

			(*E_mult_ptr[act_level])[moon_edge].make_moon_edge(); //mark this edge
			dedicated_sun_node = (*A_mult_ptr[act_level])[nearest_neighbour_node].
				get_dedicated_sun_node();
			dedicated_sun_distance = dist_to_nearest_neighbour + (*A_mult_ptr[act_level])
				[nearest_neighbour_node].get_dedicated_sun_distance();
			(*A_mult_ptr[act_level])[v].set_type(4);
			(*A_mult_ptr[act_level])[v].set_dedicated_sun_node(dedicated_sun_node);
			(*A_mult_ptr[act_level])[v].set_dedicated_sun_distance(dedicated_sun_distance);
			(*A_mult_ptr[act_level])[v].set_dedicated_pm_node(nearest_neighbour_node);

			//identify nearest_neighbour_node as a pm_node and update its information

			(*A_mult_ptr[act_level])[nearest_neighbour_node].set_type(3);
			(*A_mult_ptr[act_level])[nearest_neighbour_node].
				get_dedicated_moon_node_List_ptr()->pushBack(v);
		}//forall
}


inline void Multilevel::collaps_solar_systems(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int act_level)
{
	EdgeArray<double> new_edgelength;
	calculate_mass_of_collapsed_nodes(G_mult_ptr, A_mult_ptr, act_level);
	create_edges_edgedistances_and_lambda_Lists(G_mult_ptr, A_mult_ptr, E_mult_ptr,
		new_edgelength, act_level);
	delete_parallel_edges_and_update_edgelength(G_mult_ptr, E_mult_ptr, new_edgelength,
		act_level);
}


inline void Multilevel::calculate_mass_of_collapsed_nodes(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray <NodeAttributes>*> &A_mult_ptr,
	int act_level)
{
	node v;
	node dedicated_sun,high_level_node;

	forall_nodes(v,*G_mult_ptr[act_level])
	{
		dedicated_sun = (*A_mult_ptr[act_level])[v].get_dedicated_sun_node();
		high_level_node =  (*A_mult_ptr[act_level])[dedicated_sun].get_higher_level_node();
		(*A_mult_ptr[act_level+1])[high_level_node].set_mass((*A_mult_ptr[act_level+1])
			[high_level_node].get_mass()+1);
	}
}


void Multilevel::create_edges_edgedistances_and_lambda_Lists(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	EdgeArray<double>& new_edgelength,int
	act_level)
{
	edge e, e_new;
	node s_node, t_node;
	node s_sun_node, t_sun_node;
	node high_level_sun_s, high_level_sun_t;
	double length_e, length_s_edge, length_t_edge, newlength;
	double lambda_s, lambda_t;
	List<edge> inter_solar_system_edges;

	//create new edges at act_level+1 and create for each inter solar system edge  at
	//act_level a link to its corresponding edge

	forall_edges(e,*G_mult_ptr[act_level])
	{//forall
		s_node = e->source();
		t_node = e->target();
		s_sun_node =  (*A_mult_ptr[act_level])[s_node].get_dedicated_sun_node();
		t_sun_node =  (*A_mult_ptr[act_level])[t_node].get_dedicated_sun_node();
		if( s_sun_node != t_sun_node) //a inter solar system edge
		{//if
			high_level_sun_s = (*A_mult_ptr[act_level])[s_sun_node].get_higher_level_node();
			high_level_sun_t = (*A_mult_ptr[act_level])[t_sun_node].get_higher_level_node();

			//create new edge in *G_mult_ptr[act_level+1]
			e_new = G_mult_ptr[act_level+1]->newEdge(high_level_sun_s,high_level_sun_t);
			(*E_mult_ptr[act_level])[e].set_higher_level_edge(e_new);
			inter_solar_system_edges.pushBack(e);
		}//if
	}//forall

	//init new_edgelength calculate the values of new_edgelength and the lambda Lists

	new_edgelength.init(*G_mult_ptr[act_level+1]);
	forall_listiterators(edge, e_ptr, inter_solar_system_edges)
	{//forall
		s_node = (*e_ptr)->source();
		t_node = (*e_ptr)->target();
		s_sun_node =  (*A_mult_ptr[act_level])[s_node].get_dedicated_sun_node();
		t_sun_node =  (*A_mult_ptr[act_level])[t_node].get_dedicated_sun_node();
		length_e = (*E_mult_ptr[act_level])[*e_ptr].get_length();
		length_s_edge =(*A_mult_ptr[act_level])[s_node].get_dedicated_sun_distance();
		length_t_edge =(*A_mult_ptr[act_level])[t_node].get_dedicated_sun_distance();
		newlength = length_s_edge + length_e + length_t_edge;

		//set new edge_length in *G_mult_ptr[act_level+1]
		e_new = (*E_mult_ptr[act_level])[*e_ptr].get_higher_level_edge();
		new_edgelength[e_new] = newlength;

		//create entries in lambda Lists
		lambda_s = length_s_edge/newlength;
		lambda_t = length_t_edge/newlength;
		(*A_mult_ptr[act_level])[s_node].get_lambda_List_ptr()->pushBack(lambda_s);
		(*A_mult_ptr[act_level])[t_node].get_lambda_List_ptr()->pushBack(lambda_t);
		(*A_mult_ptr[act_level])[s_node].get_neighbour_sun_node_List_ptr()->pushBack(
			t_sun_node);
		(*A_mult_ptr[act_level])[t_node].get_neighbour_sun_node_List_ptr()->pushBack(
			s_sun_node);
	}//forall
}


void Multilevel::delete_parallel_edges_and_update_edgelength(
	Array<Graph*> &G_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	EdgeArray<double>& new_edgelength,int
	act_level)
{
	EdgeMaxBucketFunc get_max_index;
	EdgeMinBucketFunc get_min_index;
	edge e_act, e_save;
	Edge f_act;
	List<Edge> sorted_edges;
	Graph* Graph_ptr = G_mult_ptr[act_level+1];
	int save_s_index,save_t_index,act_s_index,act_t_index;
	int counter = 1;

	//make *G_mult_ptr[act_level+1] undirected
	makeSimpleUndirected(*G_mult_ptr[act_level+1]);

	//sort the List sorted_edges
	forall_edges(e_act,*Graph_ptr)
	{
		f_act.set_Edge(e_act,Graph_ptr);
		sorted_edges.pushBack(f_act);
	}

	sorted_edges.bucketSort(0,Graph_ptr->numberOfNodes()-1,get_max_index);
	sorted_edges.bucketSort(0,Graph_ptr->numberOfNodes()-1,get_min_index);

	//now parallel edges are consecutive in sorted_edges
	forall_listiterators(Edge, EdgeIterator,sorted_edges)
	{//for
		e_act = (*EdgeIterator).get_edge();
		act_s_index = e_act->source()->index();
		act_t_index = e_act->target()->index();

		if(EdgeIterator != sorted_edges.begin())
		{//if
			if( (act_s_index == save_s_index && act_t_index == save_t_index) ||
				(act_s_index == save_t_index && act_t_index == save_s_index) )
			{
				new_edgelength[e_save] += new_edgelength[e_act];
				Graph_ptr->delEdge(e_act);
				counter++;
			}
			else
			{
				if (counter > 1)
				{
					new_edgelength[e_save] /= counter;
					counter = 1;
				}
				save_s_index = act_s_index;
				save_t_index = act_t_index;
				e_save = e_act;
			}
		}//if
		else //first edge
		{
			save_s_index = act_s_index;
			save_t_index = act_t_index;
			e_save = e_act;
		}
	}//for

	//treat special case (last edges were multiple edges)
	if(counter >1)
		new_edgelength[e_save] /= counter;

	//init *E_mult_ptr[act_level+1] and import EdgeAttributes
	E_mult_ptr[act_level+1]->init(*G_mult_ptr[act_level+1]);
	forall_edges(e_act,*Graph_ptr)
		(*E_mult_ptr[act_level+1])[e_act].set_length(new_edgelength[e_act]);
}


void Multilevel::find_initial_placement_for_level(
	int level,
	int init_placement_way,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr)
{
	List<node> pm_nodes;
	set_initial_positions_of_sun_nodes(level, G_mult_ptr, A_mult_ptr);
	set_initial_positions_of_planet_and_moon_nodes(level, init_placement_way, G_mult_ptr,
		A_mult_ptr, E_mult_ptr, pm_nodes);
	set_initial_positions_of_pm_nodes(level, init_placement_way, A_mult_ptr,
		E_mult_ptr, pm_nodes);
}


void Multilevel::set_initial_positions_of_sun_nodes(
	int level,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray <NodeAttributes>*> &A_mult_ptr)
{
	node v_high, v_act;
	DPoint new_pos;
	forall_nodes(v_high,*G_mult_ptr[level+1])
	{
		v_act = (*A_mult_ptr[level+1])[v_high].get_lower_level_node();
		new_pos = (*A_mult_ptr[level+1])[v_high].get_position();
		(*A_mult_ptr[level])[v_act].set_position(new_pos);
		(*A_mult_ptr[level])[v_act].place();
	}
}


void Multilevel::set_initial_positions_of_planet_and_moon_nodes(
	int level,
	int init_placement_way,
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	List<node>& pm_nodes)
{
	double lambda, dedicated_sun_distance;
	int node_type;
	node v, v_adj, dedicated_sun;
	edge e;
	DPoint new_pos,dedicated_sun_pos, adj_sun_pos;
	List<DPoint> L;
	ListIterator<double> lambdaIterator;

	create_all_placement_sectors(G_mult_ptr,A_mult_ptr,E_mult_ptr,level);
	forall_nodes(v,*G_mult_ptr[level])
	{//for
		node_type = (*A_mult_ptr[level])[v].get_type();
		if(node_type == 3)
			pm_nodes.pushBack(v);
		else if(node_type == 2 || node_type == 4) //a planet_node or moon_node
		{//else
			L.clear();
			dedicated_sun = (*A_mult_ptr[level])[v].get_dedicated_sun_node();
			dedicated_sun_pos = (*A_mult_ptr[level])[dedicated_sun].get_position();
			dedicated_sun_distance = (*A_mult_ptr[level])[v].get_dedicated_sun_distance();

			if(init_placement_way == FMMMLayout::ipmAdvanced)
			{
				forall_adj_edges(e,v)
				{
					if(e->source() != v)
						v_adj = e->source();
					else
						v_adj = e->target();
					if( ( (*A_mult_ptr[level])[v].get_dedicated_sun_node() ==
						(*A_mult_ptr[level])[v_adj].get_dedicated_sun_node() ) &&
						( (*A_mult_ptr[level])[v_adj].get_type() != 1 ) &&
						( (*A_mult_ptr[level])[v_adj].is_placed() ) )
					{
						new_pos = calculate_position(dedicated_sun_pos,(*A_mult_ptr[level])
							[v_adj].get_position(),dedicated_sun_distance,
							(*E_mult_ptr[level])[e].get_length());
						L.pushBack(new_pos);
					}
				}
			}
			if ((*A_mult_ptr[level])[v].get_lambda_List_ptr()->empty())
			{//special case
				if(L.empty())
				{
					new_pos = create_random_pos(dedicated_sun_pos,(*A_mult_ptr[level])
						[v].get_dedicated_sun_distance(),
						(*A_mult_ptr[level])[v].get_angle_1(),
						(*A_mult_ptr[level])[v].get_angle_2());
					L.pushBack(new_pos);
				}
			}//special case
			else
			{//usual case
				lambdaIterator = (*A_mult_ptr[level])[v].get_lambda_List_ptr()->begin();

				forall_listiterators(node, adj_sun_ptr,*(*A_mult_ptr[level])[v].
					get_neighbour_sun_node_List_ptr())
				{
					lambda = *lambdaIterator;
					adj_sun_pos = (*A_mult_ptr[level])[*adj_sun_ptr].get_position();
					new_pos = get_waggled_inbetween_position(dedicated_sun_pos,adj_sun_pos,
						lambda);
					L.pushBack(new_pos);
					if(lambdaIterator != (*A_mult_ptr[level])[v].get_lambda_List_ptr()
						->rbegin())
						lambdaIterator = (*A_mult_ptr[level])[v].get_lambda_List_ptr()
						->cyclicSucc(lambdaIterator);
				}
			}//usual case

			(*A_mult_ptr[level])[v].set_position(get_barycenter_position(L));
			(*A_mult_ptr[level])[v].place();
		}//else
	}//for
}


void Multilevel::create_all_placement_sectors(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int level)
{
	node v_high, w_high, sun_node, v, ded_sun;
	edge e_high;
	List<DPoint> adj_pos;
	double angle_1, angle_2, act_angle_1, act_angle_2, next_angle, min_next_angle;
	DPoint start_pos, end_pos;
	int MAX = 10; //the biggest of at most MAX random selected sectors is choosen
	int steps;
	ListIterator<DPoint> it, next_pos_ptr;
	bool first_angle;


	forall_nodes(v_high,(*G_mult_ptr[level+1]))
	{//forall
		//find pos of adjacent nodes
		adj_pos.clear();
		DPoint v_high_pos ((*A_mult_ptr[level+1])[v_high].get_x(),
			(*A_mult_ptr[level+1])[v_high].get_y());
		forall_adj_edges(e_high,v_high)
			if(!(*E_mult_ptr[level+1])[e_high].is_extra_edge())
			{
				if(v_high == e_high->source())
					w_high = e_high->target();
				else
					w_high = e_high->source();

				DPoint w_high_pos ((*A_mult_ptr[level+1])[w_high].get_x(),
					(*A_mult_ptr[level+1])[w_high].get_y());
				adj_pos.pushBack(w_high_pos);
			}
			if(adj_pos.empty()) //easy case
			{
				angle_1 = 0;
				angle_2 = 6.2831853;
			}
			else if(adj_pos.size() == 1) //special case
			{
				//create angle_1
				start_pos = *adj_pos.begin();
				DPoint x_parallel_pos (v_high_pos.m_x + 1, v_high_pos.m_y);
				angle_1 = angle(v_high_pos,x_parallel_pos,start_pos);
				//create angle_2
				angle_2 = angle_1 + Math::pi;
			}
			else //usual case
			{//else
				steps = 1;
				it = adj_pos.begin();
				do
				{
					//create act_angle_1
					start_pos = *it;
					DPoint x_parallel_pos (v_high_pos.m_x + 1, v_high_pos.m_y);
					act_angle_1 = angle(v_high_pos,x_parallel_pos,start_pos);
					//create act_angle_2
					first_angle = true;

					for(next_pos_ptr = adj_pos.begin();next_pos_ptr.valid();++next_pos_ptr)
					{
						next_angle = angle(v_high_pos,start_pos,*next_pos_ptr);

						if(start_pos != *next_pos_ptr && (first_angle || next_angle <
							min_next_angle))
						{
							min_next_angle = next_angle;
							first_angle = false;
						}
					}
					act_angle_2 = act_angle_1 + min_next_angle;
					if((it == adj_pos.begin())||((act_angle_2-act_angle_1)>(angle_2-angle_1)))
					{
						angle_1 = act_angle_1;
						angle_2 = act_angle_2;
					}
					if(it != adj_pos.rbegin())
						it = adj_pos.cyclicSucc(it);
					steps++;
				}
				while((steps <= MAX) && (it != adj_pos.rbegin()));

				if(angle_1 == angle_2)
					angle_2 = angle_1 + Math::pi;
			}//else

			//import angle_1 and angle_2 to the dedicated suns at level level
			sun_node = (*A_mult_ptr[level+1])[v_high].get_lower_level_node();
			(*A_mult_ptr[level])[sun_node].set_angle_1(angle_1);
			(*A_mult_ptr[level])[sun_node].set_angle_2(angle_2);
	}//forall

	//import the angle values from the values of the dedicated sun nodes
	forall_nodes(v,*G_mult_ptr[level])
	{
		ded_sun = (*A_mult_ptr[level])[v].get_dedicated_sun_node();
		(*A_mult_ptr[level])[v].set_angle_1((*A_mult_ptr[level])[ded_sun].get_angle_1());
		(*A_mult_ptr[level])[v].set_angle_2((*A_mult_ptr[level])[ded_sun].get_angle_2());
	}
}


void Multilevel::set_initial_positions_of_pm_nodes(
	int level,
	int init_placement_way,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	List<node>& pm_nodes)
{
	double moon_dist, sun_dist, lambda;
	node v_adj, sun_node;
	edge e;
	DPoint sun_pos, moon_pos, new_pos, adj_sun_pos;
	List<DPoint> L;
	ListIterator<double> lambdaIterator;

	forall_listiterators(node,v_ptr,pm_nodes)
	{//forall
		L.clear();
		sun_node = (*A_mult_ptr[level])[*v_ptr].get_dedicated_sun_node();
		sun_pos =  (*A_mult_ptr[level])[sun_node].get_position();
		sun_dist = (*A_mult_ptr[level])[*v_ptr].get_dedicated_sun_distance();

		if(init_placement_way == FMMMLayout::ipmAdvanced)
		{//if
			forall_adj_edges(e,*v_ptr)
			{
				if(e->source() != *v_ptr)
					v_adj = e->source();
				else
					v_adj = e->target();
				if( (!(*E_mult_ptr[level])[e].is_moon_edge()) &&
					( (*A_mult_ptr[level])[*v_ptr].get_dedicated_sun_node() ==
					(*A_mult_ptr[level])[v_adj].get_dedicated_sun_node() ) &&
					( (*A_mult_ptr[level])[v_adj].get_type() != 1 ) &&
					( (*A_mult_ptr[level])[v_adj].is_placed() ) )
				{
					new_pos = calculate_position(sun_pos,(*A_mult_ptr[level])[v_adj].
						get_position(),sun_dist,(*E_mult_ptr[level])
						[e].get_length());
					L.pushBack(new_pos);
				}
			}
		}//if
		forall_listiterators(node, moon_node_ptr,*(*A_mult_ptr[level])[*v_ptr].
			get_dedicated_moon_node_List_ptr())
		{
			moon_pos = (*A_mult_ptr[level])[*moon_node_ptr].get_position();
			moon_dist =  (*A_mult_ptr[level])[*moon_node_ptr].get_dedicated_sun_distance();
			lambda = sun_dist/moon_dist;
			new_pos = get_waggled_inbetween_position(sun_pos,moon_pos,lambda);
			L.pushBack(new_pos);
		}

		if (!(*A_mult_ptr[level])[*v_ptr].get_lambda_List_ptr()->empty())
		{
			lambdaIterator = (*A_mult_ptr[level])[*v_ptr].get_lambda_List_ptr()->begin();

			forall_listiterators(node,adj_sun_ptr,*(*A_mult_ptr[level])[*v_ptr].
				get_neighbour_sun_node_List_ptr())
			{
				lambda = *lambdaIterator;
				adj_sun_pos = (*A_mult_ptr[level])[*adj_sun_ptr].get_position();
				new_pos = get_waggled_inbetween_position(sun_pos,adj_sun_pos,lambda);
				L.pushBack(new_pos);
				if(lambdaIterator != (*A_mult_ptr[level])[*v_ptr].get_lambda_List_ptr()
					->rbegin())
					lambdaIterator = (*A_mult_ptr[level])[*v_ptr].get_lambda_List_ptr()
					->cyclicSucc(lambdaIterator);
			}
		}

		(*A_mult_ptr[level])[*v_ptr].set_position(get_barycenter_position(L));
		(*A_mult_ptr[level])[*v_ptr].place();
	}//forall
}


inline DPoint Multilevel::create_random_pos(DPoint center,double radius,double angle_1,
	double angle_2)
{
	const int BILLION = 1000000000;
	DPoint new_point;
	double rnd = double(randomNumber(1,BILLION)+1)/(BILLION+2);//rand number in (0,1)
	double rnd_angle = angle_1 +(angle_2-angle_1)*rnd;
	double dx = cos(rnd_angle) * radius;
	double dy = sin(rnd_angle) * radius;
	new_point.m_x = center.m_x + dx ;
	new_point.m_y = center.m_y + dy;
	return new_point;
}


inline DPoint Multilevel::get_waggled_inbetween_position(DPoint s, DPoint t, double lambda)
{
	const double WAGGLEFACTOR = 0.05;
	const int BILLION = 1000000000;
	DPoint inbetween_point;
	inbetween_point.m_x = s.m_x + lambda*(t.m_x - s.m_x);
	inbetween_point.m_y = s.m_y + lambda*(t.m_y - s.m_y);
	double radius = WAGGLEFACTOR * (t-s).norm();
	double rnd = double(randomNumber(1,BILLION)+1)/(BILLION+2);//rand number in (0,1)
	double rand_radius =  radius * rnd;
	return create_random_pos(inbetween_point,rand_radius,0,6.2831853);
}


inline DPoint Multilevel::get_barycenter_position(List<DPoint>& L)
{
	DPoint sum (0,0);
	DPoint barycenter;

	forall_listiterators(DPoint, act_point_ptr,L)
		sum = sum + (*act_point_ptr);
	barycenter.m_x = sum.m_x/L.size();
	barycenter.m_y = sum.m_y/L.size();
	return barycenter;
}


inline DPoint Multilevel::calculate_position(DPoint P, DPoint Q, double dist_P, double dist_Q)
{
	double dist_PQ = (P-Q).norm();
	double lambda = (dist_P + (dist_PQ - dist_P - dist_Q)/2)/dist_PQ;
	return get_waggled_inbetween_position(P,Q,lambda);
}


void Multilevel::delete_multilevel_representations(
	Array<Graph*> &G_mult_ptr,
	Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
	Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
	int max_level)
{
	for(int i=1; i<= max_level; i++)
	{
		delete G_mult_ptr[i];
		delete A_mult_ptr[i];
		delete E_mult_ptr[i];
	}
}


double Multilevel::angle(DPoint& P, DPoint& Q, DPoint& R)
{
	double dx1 = Q.m_x - P.m_x;
	double dy1 = Q.m_y - P.m_y;
	double dx2 = R.m_x - P.m_x;
	double dy2 = R.m_y - P.m_y;
	double fi;//the angle

	if ((dx1 == 0 && dy1 == 0) || (dx2 == 0 && dy2 == 0))
		cout<<"Multilevel::angle()"<<endl;

	double norm  = (dx1*dx1+dy1*dy1)*(dx2*dx2+dy2*dy2);
	double cosfi = (dx1*dx2+dy1*dy2) / sqrt(norm);

	if (cosfi >=  1.0 ) fi = 0;
	if (cosfi <= -1.0 ) fi = Math::pi;
	else
	{
		fi = acos(cosfi);
		if (dx1*dy2 < dy1*dx2) fi = -fi;
		if (fi < 0) fi += 2*Math::pi;
	}
	return fi;
}

}//namespace ogdf
