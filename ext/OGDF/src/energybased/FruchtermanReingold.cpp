/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class FruchtermanReingold (computation of forces).
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


#include <ogdf/internal/energybased/FruchtermanReingold.h>

#include "numexcept.h"
#include <ogdf/basic/Array2D.h>


namespace ogdf {

FruchtermanReingold::FruchtermanReingold()
{
	grid_quotient(2);
}


void FruchtermanReingold::calculate_exact_repulsive_forces(
	const Graph &G,
	NodeArray<NodeAttributes> &A,
	NodeArray<DPoint>& F_rep)
{
	//naive algorithm by Fruchterman & Reingold
	numexcept N;
	node v,u;
	DPoint f_rep_u_on_v;
	DPoint vector_v_minus_u;
	DPoint pos_u,pos_v;
	DPoint nullpoint (0,0);
	double norm_v_minus_u;
	long node_number = G.numberOfNodes();
	Array<node> array_of_the_nodes (node_number+1);
	long counter = 1;
	long i,j;
	double scalar;

	forall_nodes(v,G)
		F_rep[v]= nullpoint;

	forall_nodes(v,G)
	{
		array_of_the_nodes[counter]=v;
		counter++;
	}

	for(i = 1; i<node_number;i++)
		for(j = i+1;j<=node_number;j++)
		{
			u = array_of_the_nodes[i];
			v = array_of_the_nodes[j];
			pos_u = A[u].get_position();
			pos_v = A[v].get_position();
			if (pos_u == pos_v)
			{//if2  (Exception handling if two nodes have the same position)
				pos_u = N.choose_distinct_random_point_in_radius_epsilon(pos_u);
		}//if2
		vector_v_minus_u = pos_v - pos_u;
		norm_v_minus_u = vector_v_minus_u.norm();
		if(!N.f_rep_near_machine_precision(norm_v_minus_u,f_rep_u_on_v))
		{
			scalar = f_rep_scalar(norm_v_minus_u)/norm_v_minus_u ;
			f_rep_u_on_v.m_x = scalar * vector_v_minus_u.m_x;
			f_rep_u_on_v.m_y = scalar * vector_v_minus_u.m_y;
		}
		F_rep[v] = F_rep[v] + f_rep_u_on_v;
		F_rep[u] = F_rep[u] - f_rep_u_on_v;
	}
}


void FruchtermanReingold::calculate_approx_repulsive_forces(
	const Graph &G,
	NodeArray<NodeAttributes> &A,
	NodeArray<DPoint>& F_rep)
{
	//GRID algorithm by Fruchterman & Reingold
	numexcept N;
	List<IPoint> neighbour_boxes;
	List<node> neighbour_box;
	IPoint act_neighbour_box;
	IPoint neighbour;
	DPoint f_rep_u_on_v;
	DPoint vector_v_minus_u;
	DPoint nullpoint (0,0);
	DPoint pos_u,pos_v;
	double norm_v_minus_u;
	double scalar;

	int i,j,act_i,act_j,k,l,length;
	node u,v;
	double x,y,gridboxlength;//length of a box in the GRID
	int x_index,y_index;

	//init F_rep
	forall_nodes(v,G)
		F_rep[v]= nullpoint;

	//init max_gridindex and set contained_nodes;

	max_gridindex = static_cast<int> (sqrt(double(G.numberOfNodes()))/grid_quotient())-1;
	max_gridindex = ((max_gridindex > 0)? max_gridindex : 0);
	Array2D<List<node> >  contained_nodes (0,max_gridindex, 0, max_gridindex);

	for(i=0;i<= max_gridindex;i++)
		for(j=0;j<= max_gridindex;j++)
		{
			contained_nodes(i,j).clear();
		}

		gridboxlength = boxlength/(max_gridindex+1);
		forall_nodes(v,G)
		{
			x = A[v].get_x()-down_left_corner.m_x;//shift comput. box to nullpoint
			y = A[v].get_y()-down_left_corner.m_y;
			x_index = static_cast<int>(x/gridboxlength);
			y_index = static_cast<int>(y/gridboxlength);
			contained_nodes(x_index,y_index).pushBack(v);
		}

		//force calculation

		for(i=0;i<= max_gridindex;i++)
			for(j=0;j<= max_gridindex;j++)
			{
				//step1: calculate forces inside contained_nodes(i,j)

				length = contained_nodes(i,j).size();
				Array<node> nodearray_i_j (length+1);
				k = 1;
				forall_listiterators(node, v_it,contained_nodes(i,j))
				{
					nodearray_i_j[k]= *v_it;
					k++;
				}

				for(k = 1; k<length;k++)
					for(l = k+1;l<=length;l++)
					{
						u = nodearray_i_j[k];
						v = nodearray_i_j[l];
						pos_u = A[u].get_position();
						pos_v = A[v].get_position();
						if (pos_u == pos_v)
						{//if2  (Exception handling if two nodes have the same position)
							pos_u = N.choose_distinct_random_point_in_radius_epsilon(pos_u);
						}//if2
						vector_v_minus_u = pos_v - pos_u;
						norm_v_minus_u = vector_v_minus_u.norm();

						if(!N.f_rep_near_machine_precision(norm_v_minus_u,f_rep_u_on_v))
						{
							scalar = f_rep_scalar(norm_v_minus_u)/norm_v_minus_u ;
							f_rep_u_on_v.m_x = scalar * vector_v_minus_u.m_x;
							f_rep_u_on_v.m_y = scalar * vector_v_minus_u.m_y;
						}
						F_rep[v] = F_rep[v] + f_rep_u_on_v;
						F_rep[u] = F_rep[u] - f_rep_u_on_v;
					}

					//step 2: calculated forces to nodes in neighbour boxes

					//find_neighbour_boxes

					neighbour_boxes.clear();
					for(k = i -1;k <= i+1;k++)
						for(l = j-1;l <= j+1;l++)
							if ( (k>=0) && (l>=0) && (k<=max_gridindex) && (l<=max_gridindex))
							{
								neighbour.m_x = k;
								neighbour.m_y = l;
								if ((k != i) || (l != j) )
									neighbour_boxes.pushBack(neighbour);
							}


							//forget neighbour_boxes that already had access to this box
							forall_listiterators(IPoint, act_neighbour_box_it,neighbour_boxes)
							{//forall
								act_i = (*act_neighbour_box_it).m_x;
								act_j = (*act_neighbour_box_it).m_y;
								if((act_j == j+1)||((act_j == j)&&(act_i == i+1)))
								{//if1
									forall_listiterators(node,v_it,contained_nodes(i,j))
										forall_listiterators(node,u_it,contained_nodes(act_i,act_j))
									{//for
										pos_u = A[*u_it].get_position();
										pos_v = A[*v_it].get_position();
										if (pos_u == pos_v)
										{//if2  (Exception handling if two nodes have the same position)
											pos_u = N.choose_distinct_random_point_in_radius_epsilon(pos_u);
										}//if2
										vector_v_minus_u = pos_v - pos_u;
										norm_v_minus_u = vector_v_minus_u.norm();

										if(!N.f_rep_near_machine_precision(norm_v_minus_u,f_rep_u_on_v))
										{
											scalar = f_rep_scalar(norm_v_minus_u)/norm_v_minus_u ;
											f_rep_u_on_v.m_x = scalar * vector_v_minus_u.m_x;
											f_rep_u_on_v.m_y = scalar * vector_v_minus_u.m_y;
										}
										F_rep[*v_it] = F_rep[*v_it] + f_rep_u_on_v;
										F_rep[*u_it] = F_rep[*u_it] - f_rep_u_on_v;
									}//for
								}//if1
							}//forall
			}
}


void FruchtermanReingold::make_initialisations(double bl, DPoint d_l_c, int grid_quot)
{
	grid_quotient(grid_quot);
	down_left_corner = d_l_c; //export this two values from FMMM
	boxlength = bl;
}


inline double FruchtermanReingold::f_rep_scalar(double d)
{
	if (d > 0) {
		return 1/d;

	} else {
		cout<<"Error FruchtermanReingold:: f_rep_scalar nodes at same position"<<endl;
		return 0;
	}
}

}//namespace ogdf
