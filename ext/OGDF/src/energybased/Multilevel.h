/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class Multlevel (used by FMMMLayout).
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

#ifndef OGDF_MULTILEVEL_H
#define OGDF_MULTILEVEL_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/List.h>
#include "Edge.h"
#include <ogdf/internal/energybased/NodeAttributes.h>
#include <ogdf/internal/energybased/EdgeAttributes.h>


namespace ogdf {

class Multilevel
{
public:

	Multilevel() { }     //constructor
	~Multilevel() { }    //destructor

	//The multilevel representations *G_mult_ptr/*A_mult_ptr/*E_mult_ptr for
	//G/A/E are created. The maximum multilevel is calculated, too.
	void create_multilevel_representations(Graph& G,NodeArray<NodeAttributes>& A,
		EdgeArray <EdgeAttributes>& E,
		int rand_seed,
		int galaxy_choice,
		int min_Graph_size,
		int rand_tries,
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int & max_level);

	//The initial placement of the nodes at multilevel level are created by the
	//placements of the nodes of the graphs at the lower level (if init_placement_way
	//is 0) or additionally using information of the actual level ( if
	//init_placement_way == 1). Precondition: level < max_level
	void find_initial_placement_for_level(
		int level,
		int init_placement_way,
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray <NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr);

	//Free dynamically allocated memory.
	void delete_multilevel_representations(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int max_level);

private:

	//This function returns true if act_level = 0 or if act_level >0 and the
	//number of edges at the actual level is <= 80% of the number of edges of the
	//previous level or if the actual edgenumber is >80% of the number of edges of the
	//previous level, but bad_edgecounter is <= 5. In this case edgecounter is
	//incremented. In all other cases false is returned.
	bool edgenumbersum_of_all_levels_is_linear(
		Array<Graph*> &G_mult_ptr,
		int act_level,
		int&bad_edgenr_counter);

	//The multilevel values of *A_mult_ptr[level][v] are set to the default values
	//for all nodes v in *G_mult_ptr[level]
	void init_multilevel_values(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int level);

	//The nodeset(galaxy) of *G_mult_ptr[act_level] is partitioned in s,p,pm,m nodes.
	//The dedicated s,p,pm,m nodes define a subgraph (called solar system).
	//For each solar system a new node is created in *G_mult_ptr[act_level+1] and
	//it is linked with the corresponding sun node at act_level; the mass of this node
	//is set to the mass of the solar system. Additionally for each node in *G_mult_ptr
	//[act_level] the dedicated sun node and the distance to its dedicates sun node is
	//calculated
	void partition_galaxy_into_solar_systems(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int rand_seed,
		int galaxy_choice,
		int random_tries,
		int act_level);

	//The sun and planet nodes are created by choosing the sun nodes randomly with
	//uniform or weighted probability (depending on galaxy_choice)
	void create_suns_and_planets(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int rand_seed,
		int galaxy_choice,
		int random_tries,
		int act_level);

	//Partitions the nodes of *G_mult_ptr[act_level] that have not been assigned yet,
	//to moon nodes of a nearest planet or pm node and identify this planet as a
	//pm-node if this has not been done before.
	void create_moon_nodes_and_pm_nodes(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int act_level);

	//Using information generated in partition_galaxy_into_solar_systems  we
	//create the edge set of *G_mult_ptr[act_level+1] and for each node at act_level+1
	//the list of sun nodes of neighbouring sun systems and the corresponding lambda
	//values.
	void collaps_solar_systems(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int act_level);

	//The mass of all nodes at level act_level+1 is set to the mass of its dedicated
	//solar_system at level act_level.
	void calculate_mass_of_collapsed_nodes(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		int act_level);

	//The edges , new_edgelength and the lambda lists at level act_level+1 are created
	//(the graph may contain parallel edges afterwards).
	void create_edges_edgedistances_and_lambda_Lists(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		EdgeArray<double>& new_edgelength,
		int act_level);

	//Parallel edges at level act_level+1 are deleted and the edgelength of the
	//remaining edge is set to the average edgelength of all its parallel edges.
	void delete_parallel_edges_and_update_edgelength(
		Array<Graph*> &G_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		EdgeArray<double>& new_edgelength,
		int act_level);

	//The initial positions of all sun_nodes at level level are set.
	void set_initial_positions_of_sun_nodes(
		int level,
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr);

	//The initial positions of the planet/moon_nodes at level level are calculated here
	//and a list of all pm_nodes is returned.
	void set_initial_positions_of_planet_and_moon_nodes(
		int level,
		int init_placement_way,
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		List<node> &pm_nodes);

	//The values of angle_1 and angle_2 that restrict the area of the placement for
	//all nodes that are not adjacent to other solar systems are created for all nodes
	//at multilevel level.
	void create_all_placement_sectors(
		Array<Graph*> &G_mult_ptr,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		int level);

	//The initial positions of the pm nodes are calculated by the position of the
	//dedicated sun and moon_nodes.
	void set_initial_positions_of_pm_nodes(
		int level,
		int init_placement_way,
		Array<NodeArray<NodeAttributes>*> &A_mult_ptr,
		Array<EdgeArray<EdgeAttributes>*> &E_mult_ptr,
		List<node>& pm_nodes);

	//Returns a random point with radius radius between angle_1 and angle_2.
	DPoint create_random_pos(DPoint center, double radius, double angle_1, double angle_2);

	//Returns roughtly the position s +lambda*(t-s) + some random waggling.
	DPoint get_waggled_inbetween_position(DPoint s, DPoint t, double lambda);

	//Returns the barycenter position of all points in L (the mass of all point is
	//regarded as equal).
	DPoint get_barycenter_position(List<DPoint>& L);

	//Creates a waggled position on the line PQ, depending on dist_P and dist_Q
	//needed in case init_placement_way() == 1.
	DPoint calculate_position(DPoint P,DPoint Q, double dist_P, double dist_Q);

	//Calculates the angle between PQ and PS in [0,2pi)
	double angle(DPoint& P, DPoint& Q, DPoint& R);
};

}//namespace ogdf
#endif

