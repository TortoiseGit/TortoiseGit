/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class NMM (New Multipole Method).
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


#include <ogdf/internal/energybased/NMM.h>
#include <ogdf/energybased/FMMMLayout.h>
#include <ogdf/basic/Math.h>
#include "numexcept.h"
#include <time.h>


#define MIN_BOX_LENGTH   1e-300

#ifdef __BORLANDC__
	using _STL::log;
#else
	using std::log;
#endif


namespace ogdf {

NMM::NMM()
{
	//set MIN_NODE_NUMBER and using_NMM
	MIN_NODE_NUMBER = 175; using_NMM = true;

	//setting predefined parameters
	precision(4); particles_in_leaves(25);
	tree_construction_way(FMMMLayout::rtcSubtreeBySubtree);
	find_sm_cell(FMMMLayout::scfIteratively);
}


void NMM::calculate_repulsive_forces(
	const Graph &G,
	NodeArray <NodeAttributes>& A,
	NodeArray<DPoint>& F_rep)
{
	if(using_NMM) //use NewMultipoleMethod
		calculate_repulsive_forces_by_NMM(G,A,F_rep);
	else //used the exact naive way
		calculate_repulsive_forces_by_exact_method(G,A,F_rep);
}


void NMM::calculate_repulsive_forces_by_NMM(
	const Graph &G,
	NodeArray<NodeAttributes>& A,
	NodeArray<DPoint>& F_rep)
{
	QuadTreeNM T;
	node v;
	DPoint nullpoint (0,0);
	NodeArray<DPoint> F_direct(G);
	NodeArray<DPoint> F_local_exp(G);
	NodeArray<DPoint> F_multipole_exp(G);
	List<QuadTreeNodeNM*> quad_tree_leaves;

	//initializations

	forall_nodes(v,G)
		F_direct[v]=F_local_exp[v]=F_multipole_exp[v]=nullpoint;

	quad_tree_leaves.clear();
	if(tree_construction_way() == FMMMLayout::rtcPathByPath)
		build_up_red_quad_tree_path_by_path(G,A,T);
	else //tree_construction_way == FMMMLayout::rtcSubtreeBySubtree
		build_up_red_quad_tree_subtree_by_subtree(G,A,T);

	form_multipole_expansions(A,T,quad_tree_leaves);
	calculate_local_expansions_and_WSPRLS(A,T.get_root_ptr());
	transform_local_exp_to_forces(A,quad_tree_leaves,F_local_exp);
	transform_multipole_exp_to_forces(A,quad_tree_leaves,F_multipole_exp);
	calculate_neighbourcell_forces(A,quad_tree_leaves,F_direct);
	add_rep_forces(G,F_direct,F_multipole_exp,F_local_exp,F_rep);

	delete_red_quad_tree_and_count_treenodes(T);
}


inline void NMM::calculate_repulsive_forces_by_exact_method(
	const Graph &G,
	NodeArray<NodeAttributes>& A,
	NodeArray<DPoint>& F_rep)
{
	ExactMethod.calculate_exact_repulsive_forces(G,A,F_rep);
}


void NMM::make_initialisations(
	const Graph &G,
	double bl,
	DPoint d_l_c,
	int p_i_l,
	int p,
	int t_c_w,
	int f_s_c)
{
	if(G.numberOfNodes() >= MIN_NODE_NUMBER) //using_NMM
	{
		using_NMM = true; //indicate that NMM is used for force calculation

		particles_in_leaves(p_i_l);
		precision(p);
		tree_construction_way(t_c_w);
		find_sm_cell(f_s_c);
		down_left_corner = d_l_c; //Export this two values from FMMM
		boxlength = bl;
		init_binko(2* precision());
		init_power_of_2_array();
	}
	else //use exact method
	{
		using_NMM = false; //indicate that exact method is used for force calculation
		ExactMethod.make_initialisations(bl,d_l_c,0);
	}
}


void NMM::deallocate_memory()
{
	if(using_NMM) {
		free_binko();
		free_power_of_2_array();
	}
}


void NMM::update_boxlength_and_cornercoordinate(double b_l, DPoint d_l_c)
{
	if(using_NMM) {
		boxlength = b_l;
		down_left_corner = d_l_c;
	}
	else
		ExactMethod.update_boxlength_and_cornercoordinate(b_l,d_l_c);
}


inline void NMM::init_power_of_2_array()
{
	int p = 1;
	max_power_of_2_index = 30;
	power_of_2 = new int[max_power_of_2_index+1];
	for(int i = 0; i<= max_power_of_2_index; i++) {
		power_of_2[i] = p;
		p*=2;
	}
}


inline void NMM::free_power_of_2_array()
{
	delete [] power_of_2;
}


inline int NMM::power_of_two(int i)
{
	if(i <= max_power_of_2_index)
		return power_of_2[i];
	else
		return static_cast<int>(pow(2.0,i));
}


inline int NMM::maxboxindex (int level)
{
	if ((level < 0 )) {
		cout <<"Failure NMM::maxboxindex :wrong level "<<endl;
		cout <<"level" <<level<<endl;
		return -1;

	} else
		return power_of_two(level)-1;
}


// ************Functions needed for path by path based  tree construction**********

void NMM::build_up_red_quad_tree_path_by_path(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T)
{
	List<QuadTreeNodeNM*> act_leaf_List,new_leaf_List;
	List<QuadTreeNodeNM*> *act_leaf_List_ptr,*new_leaf_List_ptr,*help_ptr;
	List <ParticleInfo> act_x_List_copy,act_y_List_copy;
	QuadTreeNodeNM *act_node_ptr;

	build_up_root_node(G,A,T);

	act_leaf_List.clear();
	new_leaf_List.clear();
	act_leaf_List.pushFront(T.get_root_ptr());
	act_leaf_List_ptr = &act_leaf_List;
	new_leaf_List_ptr = &new_leaf_List;

	while(!act_leaf_List_ptr->empty())
	{
		while(!act_leaf_List_ptr->empty())
		{
			act_node_ptr = act_leaf_List_ptr->popFrontRet();
			make_copy_and_init_Lists(*(act_node_ptr->get_x_List_ptr()),act_x_List_copy,
						*(act_node_ptr->get_y_List_ptr()),act_y_List_copy);
			T.set_act_ptr(act_node_ptr);
			decompose_subtreenode(T,act_x_List_copy,act_y_List_copy,*new_leaf_List_ptr);
		}
		help_ptr = act_leaf_List_ptr;
		act_leaf_List_ptr = new_leaf_List_ptr;
		new_leaf_List_ptr = help_ptr;
	}
}


void NMM::make_copy_and_init_Lists(
	List<ParticleInfo>& L_x_orig,
	List<ParticleInfo>& L_x_copy,
	List<ParticleInfo>& L_y_orig,
	List<ParticleInfo>& L_y_copy)
{
	ListIterator<ParticleInfo> origin_x_item,copy_x_item,origin_y_item,copy_y_item,
							new_cross_ref_item;
	ParticleInfo P_x_orig,P_y_orig,P_x_copy,P_y_copy;
	bool L_x_orig_traversed = false;
	bool L_y_orig_traversed = false;

	L_x_copy.clear();
	L_y_copy.clear();

	origin_x_item = L_x_orig.begin();
	while(!L_x_orig_traversed)
	{
		//reset values
		P_x_orig = *origin_x_item;
		P_x_orig.set_subList_ptr(NULL); //clear subList_ptr
		P_x_orig.set_copy_item(NULL);   //clear copy_item
		P_x_orig.unmark(); //unmark this element
		P_x_orig.set_tmp_cross_ref_item(NULL);//clear tmp_cross_ref_item

		//update L_x_copy
		P_x_copy = P_x_orig;
		L_x_copy.pushBack(P_x_copy);

		//update L_x_orig
		P_x_orig.set_copy_item(L_x_copy.rbegin());
		*origin_x_item = P_x_orig;

		if(origin_x_item != L_x_orig.rbegin())
			origin_x_item = L_x_orig.cyclicSucc(origin_x_item);
		else
			L_x_orig_traversed = true;
	}

	origin_y_item = L_y_orig.begin();
	while(!L_y_orig_traversed)
	{
		//reset values
		P_y_orig = *origin_y_item;
		P_y_orig.set_subList_ptr(NULL); //clear subList_ptr
		P_y_orig.set_copy_item(NULL);   //clear copy_item
		P_y_orig.set_tmp_cross_ref_item(NULL);//clear tmp_cross_ref_item
		P_y_orig.unmark(); //unmark this element

		//update L_x(y)_copy
		P_y_copy = P_y_orig;
		new_cross_ref_item = (*P_y_orig.get_cross_ref_item()).get_copy_item();
		P_y_copy.set_cross_ref_item(new_cross_ref_item);
		L_y_copy.pushBack(P_y_copy);
		P_x_copy = *new_cross_ref_item;
		P_x_copy.set_cross_ref_item(L_y_copy.rbegin());
		*new_cross_ref_item = P_x_copy;

		//update L_y_orig
		P_y_orig.set_copy_item(L_y_copy.rbegin());
		*origin_y_item = P_y_orig;

		if(origin_y_item != L_y_orig.rbegin())
			origin_y_item = L_y_orig.cyclicSucc(origin_y_item);
		else
			L_y_orig_traversed = true;
	}
}


void NMM::build_up_root_node(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T)
{
	T.init_tree();
	T.get_root_ptr()->set_Sm_level(0);
	T.get_root_ptr()->set_Sm_downleftcorner(down_left_corner);
	T.get_root_ptr()->set_Sm_boxlength(boxlength);
	//allocate space for L_x and L_y List of the root node
	T.get_root_ptr()->set_x_List_ptr(OGDF_NEW List<ParticleInfo>);
	T.get_root_ptr()->set_y_List_ptr(OGDF_NEW List<ParticleInfo>);
	create_sorted_coordinate_Lists(G, A, *(T.get_root_ptr()->get_x_List_ptr()), *(T.get_root_ptr()->get_y_List_ptr()));
}


void NMM::create_sorted_coordinate_Lists(
	const Graph& G,
	NodeArray <NodeAttributes>& A,
	List<ParticleInfo>& L_x,
	List<ParticleInfo>& L_y)
{
	ParticleInfo P_x,P_y;
	ListIterator<ParticleInfo> x_item,y_item;
	node v;

	//build up L_x,L_y and link the Lists
	forall_nodes(v,G)
	{
		P_x.set_x_y_coord(A[v].get_x());
		P_y.set_x_y_coord(A[v].get_y());
		P_x.set_vertex(v);
		P_y.set_vertex(v);
		L_x.pushBack(P_x);
		L_y.pushBack(P_y);
		P_x.set_cross_ref_item(L_y.rbegin());
		P_y.set_cross_ref_item(L_x.rbegin());
		*L_x.rbegin() = P_x;
		*L_y.rbegin() = P_y;
	}


	//sort L_x and update the links of L_y
	ParticleInfoComparer comp;
	L_x.quicksort(comp);//Quicksort L_x

	for(x_item = L_x.begin(); x_item.valid();++x_item)
	{
		y_item = (*x_item).get_cross_ref_item();
		P_y = *y_item;
		P_y.set_cross_ref_item(x_item);
		*y_item = P_y;
	}

	//sort L_y and update the links of L_x
	L_y.quicksort(comp);//Quicksort L_x

	for(y_item = L_y.begin(); y_item.valid();++y_item)
	{
		x_item = (*y_item).get_cross_ref_item();
		P_x = *x_item;
		P_x.set_cross_ref_item(y_item);
		*x_item = P_x;
	}
}


void NMM::decompose_subtreenode(
	QuadTreeNM& T,
	List<ParticleInfo>& act_x_List_copy,
	List<ParticleInfo>& act_y_List_copy,
	List<QuadTreeNodeNM*>& new_leaf_List)
{
	QuadTreeNodeNM* act_ptr = T.get_act_ptr();
	int act_particle_number = act_ptr->get_x_List_ptr()->size();
	double x_min,x_max,y_min,y_max;
	List<ParticleInfo> *L_x_l_ptr,*L_x_r_ptr,*L_x_lb_ptr,*L_x_rb_ptr,*L_x_lt_ptr,
		*L_x_rt_ptr;
	List<ParticleInfo> *L_y_l_ptr,*L_y_r_ptr,*L_y_lb_ptr,*L_y_rb_ptr,*L_y_lt_ptr,
		*L_y_rt_ptr;

	L_x_l_ptr = L_x_r_ptr = L_x_lb_ptr = L_x_lt_ptr = L_x_rb_ptr = L_x_rt_ptr = NULL;
	L_y_l_ptr = L_y_r_ptr = L_y_lb_ptr = L_y_lt_ptr = L_y_rb_ptr = L_y_rt_ptr = NULL;

	calculate_boundaries_of_act_node(T.get_act_ptr(),x_min,x_max,y_min,y_max);
	if(find_sm_cell() == FMMMLayout::scfIteratively)
		find_small_cell_iteratively(T.get_act_ptr(),x_min,x_max,y_min,y_max);
	else //find_small_cell == FMMMLayout::scfAluru
		find_small_cell_iteratively(T.get_act_ptr(),x_min,x_max,y_min,y_max);

	if( (act_particle_number > particles_in_leaves()) &&
		((x_max-x_min >=MIN_BOX_LENGTH) || (y_max-y_min >= MIN_BOX_LENGTH )))
	{//if0

		//recursive calls for the half of the quad that contains the most particles

		split_in_x_direction(act_ptr,L_x_l_ptr,L_y_l_ptr,
			L_x_r_ptr,L_y_r_ptr);
		if((L_x_r_ptr == NULL) ||
			(L_x_l_ptr != NULL && L_x_l_ptr->size() > L_x_r_ptr->size()))
		{//if1 left half contains more particles
			split_in_y_direction(act_ptr,L_x_lb_ptr,
				L_y_lb_ptr,L_x_lt_ptr,L_y_lt_ptr);
			if((L_x_lt_ptr == NULL)||
				(L_x_lb_ptr != NULL && L_x_lb_ptr->size() > L_x_lt_ptr->size()))
			{//if2
				T.create_new_lb_child(L_x_lb_ptr,L_y_lb_ptr);
				T.go_to_lb_child();
				decompose_subtreenode(T,act_x_List_copy,act_y_List_copy,new_leaf_List);
				T.go_to_father();
			}//if2
			else //L_x_lt_ptr != NULL &&  L_x_lb_ptr->size() <= L_x_lt_ptr->size()
			{//else1
				T.create_new_lt_child(L_x_lt_ptr,L_y_lt_ptr);
				T.go_to_lt_child();
				decompose_subtreenode(T,act_x_List_copy,act_y_List_copy,new_leaf_List);
				T.go_to_father();
			}//else1
		}//if1
		else //L_x_r_ptr != NULL && (L_x_l_ptr->size() <= L_x_r_ptr->size())
		{//else2 right half contains more particles
			split_in_y_direction(act_ptr,L_x_rb_ptr,
				L_y_rb_ptr,L_x_rt_ptr,L_y_rt_ptr);
			if ((L_x_rt_ptr == NULL) ||
				(L_x_rb_ptr != NULL && L_x_rb_ptr->size() > L_x_rt_ptr->size()))
			{//if3
				T.create_new_rb_child(L_x_rb_ptr,L_y_rb_ptr);
				T.go_to_rb_child();
				decompose_subtreenode(T,act_x_List_copy,act_y_List_copy,new_leaf_List);
				T.go_to_father();
			}//if3
			else// L_x_rt_ptr != NULL && L_x_rb_ptr->size() <= L_x_rt_ptr->size()
			{//else3
				T.create_new_rt_child(L_x_rt_ptr,L_y_rt_ptr);
				T.go_to_rt_child();
				decompose_subtreenode(T,act_x_List_copy,act_y_List_copy,new_leaf_List);
				T.go_to_father();
			}//else3
		}//else2

		//build up the rest of the quad-subLists

		if( L_x_l_ptr != NULL && L_x_lb_ptr == NULL && L_x_lt_ptr == NULL &&
			!act_ptr->child_lb_exists() && !act_ptr->child_lt_exists() )
			split_in_y_direction(act_ptr,L_x_l_ptr,L_x_lb_ptr,L_x_lt_ptr,L_y_l_ptr,
			L_y_lb_ptr,L_y_lt_ptr);
		else if( L_x_r_ptr != NULL && L_x_rb_ptr == NULL && L_x_rt_ptr == NULL &&
			!act_ptr->child_rb_exists() && !act_ptr->child_rt_exists() )
			split_in_y_direction(act_ptr,L_x_r_ptr,L_x_rb_ptr,L_x_rt_ptr,L_y_r_ptr,
			L_y_rb_ptr,L_y_rt_ptr);

		//create rest of the childnodes
		if((!act_ptr->child_lb_exists()) && (L_x_lb_ptr != NULL))
		{
			T.create_new_lb_child(L_x_lb_ptr,L_y_lb_ptr);
			T.go_to_lb_child();
			new_leaf_List.pushBack(T.get_act_ptr());
			T.go_to_father();
		}
		if((!act_ptr->child_lt_exists()) && (L_x_lt_ptr != NULL))
		{
			T.create_new_lt_child(L_x_lt_ptr,L_y_lt_ptr);
			T.go_to_lt_child();
			new_leaf_List.pushBack(T.get_act_ptr());
			T.go_to_father();
		}
		if((!act_ptr->child_rb_exists()) && (L_x_rb_ptr != NULL))
		{
			T.create_new_rb_child(L_x_rb_ptr,L_y_rb_ptr);
			T.go_to_rb_child();
			new_leaf_List.pushBack(T.get_act_ptr());
			T.go_to_father();
		}
		if((!act_ptr->child_rt_exists()) && (L_x_rt_ptr != NULL))
		{
			T.create_new_rt_child(L_x_rt_ptr,L_y_rt_ptr);
			T.go_to_rt_child();
			new_leaf_List.pushBack(T.get_act_ptr());
			T.go_to_father();
		}
		//reset  act_ptr->set_x(y)_List_ptr to avoid multiple deleting of dynamic memory;
		//(only if *act_ptr is a leaf of T the reserved space is freed (and this is
		//sufficient !!!))
		act_ptr->set_x_List_ptr(NULL);
		act_ptr->set_y_List_ptr(NULL);
	}//if0
	else
	{ //else a leaf or machineprecision is reached:
		//The List contained_nodes is set for *act_ptr and the information of
		//act_x_List_copy and act_y_List_copy is used to insert particles into the
		//shorter Lists of previous touched treenodes;additionaly the dynamical allocated
		//space for *act_ptr->get_x(y)_List_ptr() is freed.

		List<node> L;
		ListIterator<ParticleInfo> it;

		//set List contained nodes

		L.clear();
		for(it = act_ptr->get_x_List_ptr()->begin();it.valid();++it)
			L.pushBack((*it).get_vertex());
		T.get_act_ptr()->set_contained_nodes(L);

		//insert particles into previous touched Lists

		build_up_sorted_subLists(act_x_List_copy,act_y_List_copy);

		//free allocated space for *act_ptr->get_x(y)_List_ptr()
		act_ptr->get_x_List_ptr()->clear();//free used space for old L_x,L_y Lists
		act_ptr->get_y_List_ptr()->clear();
	}//else
}


inline void NMM::calculate_boundaries_of_act_node(
	QuadTreeNodeNM* act_ptr,
	double& x_min,
	double& x_max,
	double& y_min,
	double& y_max)
{
	List<ParticleInfo>* L_x_ptr =  act_ptr->get_x_List_ptr();
	List<ParticleInfo>* L_y_ptr =  act_ptr->get_y_List_ptr();

	x_min = (*L_x_ptr->begin()).get_x_y_coord();
	x_max = (*L_x_ptr->rbegin()).get_x_y_coord();
	y_min = (*L_y_ptr->begin()).get_x_y_coord();
	y_max = (*L_y_ptr->rbegin()).get_x_y_coord();
}


bool NMM::in_lt_quad(
	QuadTreeNodeNM* act_ptr,
	double x_min,
	double x_max,
	double y_min,
	double y_max)
{
	double l = act_ptr->get_Sm_downleftcorner().m_x;
	double r = act_ptr->get_Sm_downleftcorner().m_x+act_ptr->get_Sm_boxlength()/2;
	double b = act_ptr->get_Sm_downleftcorner().m_y+act_ptr->get_Sm_boxlength()/2;
	double t = act_ptr->get_Sm_downleftcorner().m_y+act_ptr->get_Sm_boxlength();

	if(l <= x_min && x_max < r && b <= y_min && y_max < t )
		return true;
	else if(x_min == x_max && y_min == y_max && l==r && t == b && x_min ==r && y_min ==b)
		return true;
	else
		return false;
}


bool NMM::in_rt_quad(
	QuadTreeNodeNM* act_ptr,
	double x_min,
	double x_max,
	double y_min,
	double y_max)
{
	double l = act_ptr->get_Sm_downleftcorner().m_x+act_ptr->get_Sm_boxlength()/2;
	double r = act_ptr->get_Sm_downleftcorner().m_x+act_ptr->get_Sm_boxlength();
	double b = act_ptr->get_Sm_downleftcorner().m_y+act_ptr->get_Sm_boxlength()/2;
	double t = act_ptr->get_Sm_downleftcorner().m_y+act_ptr->get_Sm_boxlength();

	if(l <= x_min && x_max < r && b <= y_min && y_max < t )
		return true;
	else if(x_min == x_max && y_min == y_max && l==r && t == b && x_min ==r && y_min ==b)
		return true;
	else
		return false;
}


bool NMM::in_lb_quad(
	QuadTreeNodeNM* act_ptr,
	double x_min,
	double x_max,
	double y_min,
	double y_max)
{
	double l = act_ptr->get_Sm_downleftcorner().m_x;
	double r = act_ptr->get_Sm_downleftcorner().m_x+act_ptr->get_Sm_boxlength()/2;
	double b = act_ptr->get_Sm_downleftcorner().m_y;
	double t = act_ptr->get_Sm_downleftcorner().m_y+act_ptr->get_Sm_boxlength()/2;

	if(l <= x_min && x_max < r && b <= y_min && y_max < t )
		return true;
	else if(x_min == x_max && y_min == y_max && l==r && t == b && x_min ==r && y_min ==b)
		return true;
	else
		return false;
}


bool NMM::in_rb_quad(
	QuadTreeNodeNM* act_ptr,
	double x_min,
	double x_max,
	double y_min,
	double y_max)
{
	double l = act_ptr->get_Sm_downleftcorner().m_x+act_ptr->get_Sm_boxlength()/2;
	double r = act_ptr->get_Sm_downleftcorner().m_x+act_ptr->get_Sm_boxlength();
	double b = act_ptr->get_Sm_downleftcorner().m_y;
	double t = act_ptr->get_Sm_downleftcorner().m_y+act_ptr->get_Sm_boxlength()/2;

	if(l <= x_min && x_max < r && b <= y_min && y_max < t )
		return true;
	else if(x_min == x_max && y_min == y_max && l==r && t == b && x_min ==r && y_min ==b)
		return true;
	else
		return false;
}


void NMM::split_in_x_direction(
	QuadTreeNodeNM* act_ptr,
	List <ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List <ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr)
{
	ListIterator<ParticleInfo> l_item = act_ptr->get_x_List_ptr()->begin();
	ListIterator<ParticleInfo> r_item = act_ptr->get_x_List_ptr()->rbegin();
	ListIterator<ParticleInfo> last_left_item;
	double act_Sm_boxlength_half = act_ptr->get_Sm_boxlength()/2;
	double x_mid_coord = act_ptr->get_Sm_downleftcorner().m_x+ act_Sm_boxlength_half;
	double l_xcoord,r_xcoord;
	bool last_left_item_found = false;
	bool left_particleList_empty = false;
	bool right_particleList_empty = false;
	bool left_particleList_larger = true;

	//traverse *act_ptr->get_x_List_ptr() from left and right

	while(!last_left_item_found)
	{//while
		l_xcoord = (*l_item).get_x_y_coord();
		r_xcoord = (*r_item).get_x_y_coord();
		if(l_xcoord >= x_mid_coord)
		{
			left_particleList_larger = false;
			last_left_item_found = true;
			if(l_item != act_ptr->get_x_List_ptr()->begin())
				last_left_item = act_ptr->get_x_List_ptr()->cyclicPred(l_item);
			else
				left_particleList_empty = true;
		}
		else if(r_xcoord < x_mid_coord)
		{
			last_left_item_found = true;
			if(r_item != act_ptr->get_x_List_ptr()->rbegin())
				last_left_item = r_item;
			else
				right_particleList_empty = true;
		}
		if(!last_left_item_found)
		{
			l_item = act_ptr->get_x_List_ptr()->cyclicSucc(l_item);
			r_item = act_ptr->get_x_List_ptr()->cyclicPred(r_item);
		}
	}//while

	//get the L_x(y) Lists of the bigger half (from *act_ptr->get_x(y)_List_ptr))
	//and make entries in L_x_copy,L_y_copy for the smaller halfs

	if(left_particleList_empty)
	{
		L_x_left_ptr = NULL;
		L_y_left_ptr = NULL;
		L_x_right_ptr = act_ptr->get_x_List_ptr();
		L_y_right_ptr = act_ptr->get_y_List_ptr();
	}
	else if(right_particleList_empty)
	{
		L_x_left_ptr = act_ptr->get_x_List_ptr();
		L_y_left_ptr = act_ptr->get_y_List_ptr();
		L_x_right_ptr = NULL;
		L_y_right_ptr = NULL;
	}
	else if(left_particleList_larger)
		x_delete_right_subLists(act_ptr,L_x_left_ptr,L_y_left_ptr,
			L_x_right_ptr,L_y_right_ptr,last_left_item);
	else //left particleList is smaller or equal to right particleList
		x_delete_left_subLists(act_ptr,L_x_left_ptr,L_y_left_ptr,
			L_x_right_ptr,L_y_right_ptr,last_left_item);
}


void NMM::split_in_y_direction(
	QuadTreeNodeNM* act_ptr,
	List<ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List<ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr)
{
	ListIterator<ParticleInfo> l_item = act_ptr->get_y_List_ptr()->begin();
	ListIterator<ParticleInfo> r_item = act_ptr->get_y_List_ptr()->rbegin();
	ListIterator<ParticleInfo> last_left_item;
	double act_Sm_boxlength_half = act_ptr->get_Sm_boxlength()/2;
	double y_mid_coord = act_ptr->get_Sm_downleftcorner().m_y+ act_Sm_boxlength_half;
	double l_ycoord,r_ycoord;
	bool last_left_item_found = false;
	bool left_particleList_empty = false;
	bool right_particleList_empty = false;
	bool left_particleList_larger = true;
	//traverse *act_ptr->get_y_List_ptr() from left and right

	while(!last_left_item_found)
	{//while
		l_ycoord = (*l_item).get_x_y_coord();
		r_ycoord = (*r_item).get_x_y_coord();
		if(l_ycoord >= y_mid_coord)
		{
			left_particleList_larger = false;
			last_left_item_found = true;
			if(l_item != act_ptr->get_y_List_ptr()->begin())
				last_left_item = act_ptr->get_y_List_ptr()->cyclicPred(l_item);
			else
				left_particleList_empty = true;
		}
		else if(r_ycoord < y_mid_coord)
		{
			last_left_item_found = true;
			if(r_item != act_ptr->get_y_List_ptr()->rbegin())
				last_left_item = r_item;
			else
				right_particleList_empty = true;
		}
		if(!last_left_item_found)
		{
			l_item = act_ptr->get_y_List_ptr()->cyclicSucc(l_item);
			r_item = act_ptr->get_y_List_ptr()->cyclicPred(r_item);
		}
	}//while

	//get the L_x(y) Lists of the bigger half (from *act_ptr->get_x(y)_List_ptr))
	//and make entries in L_x_copy,L_y_copy for the smaller halfs

	if(left_particleList_empty)
	{
		L_x_left_ptr = NULL;
		L_y_left_ptr = NULL;
		L_x_right_ptr = act_ptr->get_x_List_ptr();
		L_y_right_ptr = act_ptr->get_y_List_ptr();
	}
	else if(right_particleList_empty)
	{
		L_x_left_ptr = act_ptr->get_x_List_ptr();
		L_y_left_ptr = act_ptr->get_y_List_ptr();
		L_x_right_ptr = NULL;
		L_y_right_ptr = NULL;
	}
	else if(left_particleList_larger)
		y_delete_right_subLists(act_ptr,L_x_left_ptr,L_y_left_ptr,
			L_x_right_ptr,L_y_right_ptr,last_left_item);
	else //left particleList is smaller or equal to right particleList
		y_delete_left_subLists(act_ptr,L_x_left_ptr,L_y_left_ptr,
			L_x_right_ptr,L_y_right_ptr,last_left_item);
}


void NMM::x_delete_right_subLists(
	QuadTreeNodeNM* act_ptr,
	List <ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List <ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr,
	ListIterator<ParticleInfo> last_left_item)
{
	ParticleInfo act_p_info,p_in_L_x_info,p_in_L_y_info,del_p_info;
	ListIterator<ParticleInfo> act_item,p_in_L_x_item,p_in_L_y_item,del_item;
	bool last_item_reached =false;

	L_x_left_ptr = act_ptr->get_x_List_ptr();
	L_y_left_ptr = act_ptr->get_y_List_ptr();
	L_x_right_ptr = OGDF_NEW List<ParticleInfo>;
	L_y_right_ptr = OGDF_NEW List<ParticleInfo>;

	act_item = L_x_left_ptr->cyclicSucc(last_left_item);

	while(!last_item_reached)
	{//while
		act_p_info = *act_item;
		del_item = act_item;
		del_p_info = act_p_info;

		//save references for *L_x(y)_right_ptr in L_x(y)_copy
		p_in_L_x_item = act_p_info.get_copy_item();
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_subList_ptr(L_x_right_ptr);
		*p_in_L_x_item = p_in_L_x_info;

		p_in_L_y_item = (*act_p_info.get_cross_ref_item()).get_copy_item();
		p_in_L_y_info = *p_in_L_y_item;
		p_in_L_y_info.set_subList_ptr(L_y_right_ptr);
		*p_in_L_y_item = p_in_L_y_info;

		if(act_item != L_x_left_ptr->rbegin())
			act_item = L_x_left_ptr->cyclicSucc(act_item);
		else
			last_item_reached = true;

		//create *L_x(y)_left_ptr
		L_y_left_ptr->del(del_p_info.get_cross_ref_item());
		L_x_left_ptr->del(del_item);
	}//while
}


void NMM::x_delete_left_subLists(
	QuadTreeNodeNM* act_ptr,
	List <ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List <ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr,
	ListIterator<ParticleInfo> last_left_item)
{
	ParticleInfo act_p_info,p_in_L_x_info,p_in_L_y_info,del_p_info;
	ListIterator<ParticleInfo> act_item,p_in_L_x_item,p_in_L_y_item,del_item;
	bool last_item_reached =false;

	L_x_right_ptr = act_ptr->get_x_List_ptr();
	L_y_right_ptr = act_ptr->get_y_List_ptr();
	L_x_left_ptr = OGDF_NEW List<ParticleInfo>;
	L_y_left_ptr = OGDF_NEW List<ParticleInfo>;

	act_item = L_x_right_ptr->begin();

	while(!last_item_reached)
	{//while
		act_p_info = *act_item;
		del_item = act_item;
		del_p_info = act_p_info;

		//save references for *L_x(y)_right_ptr in L_x(y)_copy
		p_in_L_x_item = act_p_info.get_copy_item();
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_subList_ptr(L_x_left_ptr);
		*p_in_L_x_item = p_in_L_x_info;

		p_in_L_y_item =(*act_p_info.get_cross_ref_item()).get_copy_item();
		p_in_L_y_info = *p_in_L_y_item;
		p_in_L_y_info.set_subList_ptr(L_y_left_ptr);
		*p_in_L_y_item = p_in_L_y_info;

		if(act_item != last_left_item)
			act_item = L_x_right_ptr->cyclicSucc(act_item);
		else
			last_item_reached = true;

		//create *L_x(y)_right_ptr
		L_y_right_ptr->del(del_p_info.get_cross_ref_item());
		L_x_right_ptr->del(del_item);
	}//while
}


void NMM::y_delete_right_subLists(
	QuadTreeNodeNM* act_ptr,
	List <ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List <ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr,
	ListIterator<ParticleInfo> last_left_item)
{
	ParticleInfo act_p_info,p_in_L_x_info,p_in_L_y_info,del_p_info;
	ListIterator<ParticleInfo> act_item,p_in_L_x_item,p_in_L_y_item,del_item;
	bool last_item_reached =false;

	L_x_left_ptr = act_ptr->get_x_List_ptr();
	L_y_left_ptr = act_ptr->get_y_List_ptr();
	L_x_right_ptr = OGDF_NEW List<ParticleInfo>;
	L_y_right_ptr = OGDF_NEW List<ParticleInfo>;

	act_item = L_y_left_ptr->cyclicSucc(last_left_item);

	while(!last_item_reached)
	{//while
		act_p_info = *act_item;
		del_item = act_item;
		del_p_info = act_p_info;

		//save references for *L_x(y)_right_ptr in L_x(y)_copy
		p_in_L_y_item = act_p_info.get_copy_item();
		p_in_L_y_info = *p_in_L_y_item;
		p_in_L_y_info.set_subList_ptr(L_y_right_ptr);
		*p_in_L_y_item = p_in_L_y_info;

		p_in_L_x_item = (*act_p_info.get_cross_ref_item()).get_copy_item();
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_subList_ptr(L_x_right_ptr);
		*p_in_L_x_item = p_in_L_x_info;

		if(act_item != L_y_left_ptr->rbegin())
			act_item = L_y_left_ptr->cyclicSucc(act_item);
		else
			last_item_reached = true;

		//create *L_x(y)_left_ptr
		L_x_left_ptr->del(del_p_info.get_cross_ref_item());
		L_y_left_ptr->del(del_item);
	}//while
}


void NMM::y_delete_left_subLists(
	QuadTreeNodeNM* act_ptr,
	List<ParticleInfo>*& L_x_left_ptr,
	List<ParticleInfo>*& L_y_left_ptr,
	List <ParticleInfo>*& L_x_right_ptr,
	List<ParticleInfo>*& L_y_right_ptr,
	ListIterator<ParticleInfo> last_left_item)
{
	ParticleInfo act_p_info,p_in_L_x_info,p_in_L_y_info,del_p_info;
	ListIterator<ParticleInfo> act_item,p_in_L_x_item,p_in_L_y_item,del_item;
	bool last_item_reached =false;

	L_x_right_ptr = act_ptr->get_x_List_ptr();
	L_y_right_ptr = act_ptr->get_y_List_ptr();
	L_x_left_ptr = OGDF_NEW List<ParticleInfo>;
	L_y_left_ptr = OGDF_NEW List<ParticleInfo>;

	act_item = L_y_right_ptr->begin();

	while(!last_item_reached)
	{//while
		act_p_info = *act_item;
		del_item = act_item;
		del_p_info = act_p_info;

		//save references for *L_x(y)_right_ptr in L_x(y)_copy
		p_in_L_y_item = act_p_info.get_copy_item();
		p_in_L_y_info = *p_in_L_y_item;
		p_in_L_y_info.set_subList_ptr(L_y_left_ptr);
		*p_in_L_y_item = p_in_L_y_info;

		p_in_L_x_item = (*act_p_info.get_cross_ref_item()).get_copy_item();
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_subList_ptr(L_x_left_ptr);
		*p_in_L_x_item = p_in_L_x_info;

		if(act_item != last_left_item)
			act_item = L_y_right_ptr->cyclicSucc(act_item);
		else
			last_item_reached = true;

		//create *L_x(y)_right_ptr
		L_x_right_ptr->del(del_p_info.get_cross_ref_item());
		L_y_right_ptr->del(del_item);
	}//while
}


void NMM::split_in_y_direction(
	QuadTreeNodeNM* act_ptr,
	List<ParticleInfo>*& L_x_ptr,
	List<ParticleInfo>*& L_x_b_ptr,
	List<ParticleInfo>*& L_x_t_ptr,
	List<ParticleInfo>*& L_y_ptr,
	List<ParticleInfo>*& L_y_b_ptr,
	List<ParticleInfo>*& L_y_t_ptr)
{
	ListIterator<ParticleInfo> l_item = L_y_ptr->begin();
	ListIterator<ParticleInfo> r_item = L_y_ptr->rbegin();
	ListIterator<ParticleInfo> last_left_item;
	double act_Sm_boxlength_half = act_ptr->get_Sm_boxlength()/2;
	double y_mid_coord = act_ptr->get_Sm_downleftcorner().m_y+ act_Sm_boxlength_half;
	double l_ycoord,r_ycoord;
	bool last_left_item_found = false;
	bool left_particleList_empty = false;
	bool right_particleList_empty = false;
	bool left_particleList_larger = true;

	//traverse *L_y_ptr from left and right

	while(!last_left_item_found)
	{//while
		l_ycoord = (*l_item).get_x_y_coord();
		r_ycoord = (*r_item).get_x_y_coord();
		if(l_ycoord >= y_mid_coord)
		{
			left_particleList_larger = false;
			last_left_item_found = true;
			if(l_item != L_y_ptr->begin())
				last_left_item = L_y_ptr->cyclicPred(l_item);
			else
				left_particleList_empty = true;
		}
		else if(r_ycoord < y_mid_coord)
		{
			last_left_item_found = true;
			if(r_item != L_y_ptr->rbegin())
				last_left_item = r_item;
			else
				right_particleList_empty = true;
		}
		if(!last_left_item_found)
		{
			l_item = L_y_ptr->cyclicSucc(l_item);
			r_item = L_y_ptr->cyclicPred(r_item);
		}
	}//while

 //create *L_x_l(b)_ptr

	if(left_particleList_empty)
	{
		L_x_b_ptr = NULL;
		L_y_b_ptr = NULL;
		L_x_t_ptr = L_x_ptr;
		L_y_t_ptr = L_y_ptr;
	}
	else if(right_particleList_empty)
	{
		L_x_b_ptr = L_x_ptr;
		L_y_b_ptr = L_y_ptr;
		L_x_t_ptr = NULL;
		L_y_t_ptr = NULL;
	}
	else if(left_particleList_larger)
		y_move_right_subLists(L_x_ptr,L_x_b_ptr,L_x_t_ptr,L_y_ptr,L_y_b_ptr,L_y_t_ptr,
			last_left_item);
	else //left particleList is smaller or equal to right particleList
		y_move_left_subLists(L_x_ptr,L_x_b_ptr,L_x_t_ptr,L_y_ptr,L_y_b_ptr,L_y_t_ptr,
			last_left_item);
}


void NMM::y_move_left_subLists(
	List<ParticleInfo>*& L_x_ptr,
	List <ParticleInfo>*& L_x_l_ptr,
	List<ParticleInfo>*& L_x_r_ptr,
	List<ParticleInfo>*& L_y_ptr,
	List <ParticleInfo>*& L_y_l_ptr,
	List<ParticleInfo>*& L_y_r_ptr,
	ListIterator<ParticleInfo> last_left_item)
{
	ParticleInfo p_in_L_x_info,p_in_L_y_info;
	ListIterator<ParticleInfo> p_in_L_x_item,p_in_L_y_item,del_item;
	bool last_item_reached =false;

	L_x_r_ptr = L_x_ptr;
	L_y_r_ptr = L_y_ptr;
	L_x_l_ptr = OGDF_NEW List<ParticleInfo>;
	L_y_l_ptr = OGDF_NEW List<ParticleInfo>;

	p_in_L_y_item = L_y_r_ptr->begin();

	//build up the L_y_Lists and update crossreferences in *L_x_l_ptr
	while(!last_item_reached)
	{//while
		p_in_L_y_info = *p_in_L_y_item;
		del_item = p_in_L_y_item;

		//create *L_x(y)_l_ptr
		L_y_l_ptr->pushBack(p_in_L_y_info);
		p_in_L_x_item = p_in_L_y_info.get_cross_ref_item();
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_cross_ref_item(L_y_l_ptr->rbegin());
		p_in_L_x_info.mark(); //mark this element of the List
		*p_in_L_x_item = p_in_L_x_info;

		if(p_in_L_y_item != last_left_item)
			p_in_L_y_item = L_y_r_ptr->cyclicSucc(p_in_L_y_item);
		else
			last_item_reached = true;

		//create *L_y_r_ptr
		L_y_r_ptr->del(del_item);
	}//while

	//build up the L_x Lists and update crossreferences in *L_y_l_ptr

	last_item_reached = false;
	p_in_L_x_item = L_x_r_ptr->begin();

	while(!last_item_reached)
	{//while
		del_item = p_in_L_x_item;

		if((*del_item).is_marked())
		{
			p_in_L_x_info = *p_in_L_x_item;
			p_in_L_x_info.unmark();
			L_x_l_ptr->pushBack(p_in_L_x_info);
			p_in_L_y_item = p_in_L_x_info.get_cross_ref_item();
			p_in_L_y_info = *p_in_L_y_item;
			p_in_L_y_info.set_cross_ref_item(L_x_l_ptr->rbegin());
			*p_in_L_y_item = p_in_L_y_info;
		}

		if(p_in_L_x_item != L_x_r_ptr->rbegin())
			p_in_L_x_item = L_x_r_ptr->cyclicSucc(p_in_L_x_item);
		else
			last_item_reached = true;

		//create *L_x_r_ptr
		if((*del_item).is_marked())
			L_x_r_ptr->del(del_item);
	}//while
}


void NMM::y_move_right_subLists(
	List<ParticleInfo>*& L_x_ptr,
	List <ParticleInfo>*& L_x_l_ptr,
	List<ParticleInfo>*& L_x_r_ptr,
	List<ParticleInfo>*& L_y_ptr,
	List <ParticleInfo>*& L_y_l_ptr,
	List<ParticleInfo>*& L_y_r_ptr,
	ListIterator<ParticleInfo> last_left_item)
{
	ParticleInfo p_in_L_x_info,p_in_L_y_info;
	ListIterator<ParticleInfo> p_in_L_x_item,p_in_L_y_item,del_item;
	bool last_item_reached =false;

	L_x_l_ptr = L_x_ptr;
	L_y_l_ptr = L_y_ptr;
	L_x_r_ptr = OGDF_NEW List<ParticleInfo>;
	L_y_r_ptr = OGDF_NEW List<ParticleInfo>;

	p_in_L_y_item = L_y_l_ptr->cyclicSucc(last_left_item);

	//build up the L_y_Lists and update crossreferences in *L_x_r_ptr
	while(!last_item_reached)
	{//while
		p_in_L_y_info = *p_in_L_y_item;
		del_item = p_in_L_y_item;

		//create *L_x(y)_r_ptr
		L_y_r_ptr->pushBack(p_in_L_y_info);
		p_in_L_x_item = p_in_L_y_info.get_cross_ref_item();
		p_in_L_x_info = *p_in_L_x_item;
		p_in_L_x_info.set_cross_ref_item(L_y_r_ptr->rbegin());
		p_in_L_x_info.mark(); //mark this element of the List
		*p_in_L_x_item = p_in_L_x_info;

		if(p_in_L_y_item != L_y_l_ptr->rbegin())
			p_in_L_y_item = L_y_l_ptr->cyclicSucc(p_in_L_y_item);
		else
			last_item_reached = true;

		//create *L_y_l_ptr
		L_y_l_ptr->del(del_item);
	}//while

	//build up the L_x Lists and update crossreferences in *L_y_r_ptr

	last_item_reached = false;
	p_in_L_x_item = L_x_l_ptr->begin();

	while(!last_item_reached)
	{//while
		del_item = p_in_L_x_item;

		if((*del_item).is_marked())
		{
			p_in_L_x_info = *p_in_L_x_item;
			p_in_L_x_info.unmark();
			L_x_r_ptr->pushBack(p_in_L_x_info);
			p_in_L_y_item = p_in_L_x_info.get_cross_ref_item();
			p_in_L_y_info = *p_in_L_y_item;
			p_in_L_y_info.set_cross_ref_item(L_x_r_ptr->rbegin());
			*p_in_L_y_item = p_in_L_y_info;
		}

		if(p_in_L_x_item != L_x_l_ptr->rbegin())
			p_in_L_x_item = L_x_l_ptr->cyclicSucc(p_in_L_x_item);
		else
			last_item_reached = true;

		//create *L_x_r_ptr
		if((*del_item).is_marked())
			L_x_l_ptr->del(del_item);
	}//while
}


void NMM::build_up_sorted_subLists(
	List<ParticleInfo>& L_x_copy,
	List<ParticleInfo>& L_y_copy)
{
	ParticleInfo P_x,P_y;
	List<ParticleInfo>  *L_x_ptr,*L_y_ptr;
	ListIterator<ParticleInfo> it,new_cross_ref_item;

	for(it = L_x_copy.begin();it.valid();++it)
		if((*it).get_subList_ptr() != NULL)
		{
			//reset values
			P_x = *it;
			L_x_ptr = P_x.get_subList_ptr();
			P_x.set_subList_ptr(NULL); //clear subList_ptr
			P_x.set_copy_item(NULL);   //clear copy_item
			P_x.unmark(); //unmark this element
			P_x.set_tmp_cross_ref_item(NULL);//clear tmp_cross_ref_item

			//update *L_x_ptr
			L_x_ptr->pushBack(P_x);

			//update L_x_copy
			P_x.set_tmp_cross_ref_item(L_x_ptr->rbegin());
			*it = P_x;
		}

	for(it = L_y_copy.begin();it.valid();++it)
		if((*it).get_subList_ptr() != NULL)
		{
			//reset values
			P_y = *it;
			L_y_ptr = P_y.get_subList_ptr();
			P_y.set_subList_ptr(NULL); //clear subList_ptr
			P_y.set_copy_item(NULL);   //clear copy_item
			P_y.unmark(); //unmark this element
			P_y.set_tmp_cross_ref_item(NULL);//clear tmp_cross_ref_item

			//update *L_x(y)_ptr

			new_cross_ref_item = (*P_y.get_cross_ref_item()).get_tmp_cross_ref_item();
			P_y.set_cross_ref_item(new_cross_ref_item);
			L_y_ptr->pushBack(P_y);
			P_x  = *new_cross_ref_item;
			P_x.set_cross_ref_item(L_y_ptr->rbegin());
			*new_cross_ref_item = P_x;
		}
}


// **********Functions needed for subtree by subtree  tree construction(Begin)*********

void NMM::build_up_red_quad_tree_subtree_by_subtree(
	const Graph& G,
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T)
{
	List<QuadTreeNodeNM*> act_subtree_root_List,new_subtree_root_List;
	List<QuadTreeNodeNM*> *act_subtree_root_List_ptr,*new_subtree_root_List_ptr,*help_ptr;
	QuadTreeNodeNM *subtree_root_ptr;

	build_up_root_vertex(G,T);

	act_subtree_root_List.clear();
	new_subtree_root_List.clear();
	act_subtree_root_List.pushFront(T.get_root_ptr());
	act_subtree_root_List_ptr = &act_subtree_root_List;
	new_subtree_root_List_ptr = &new_subtree_root_List;

	while(!act_subtree_root_List_ptr->empty())
	{
		while(!act_subtree_root_List_ptr->empty())
		{
			subtree_root_ptr = act_subtree_root_List_ptr->popFrontRet();
			construct_subtree(A,T,subtree_root_ptr,*new_subtree_root_List_ptr);
		}
		help_ptr = act_subtree_root_List_ptr;
		act_subtree_root_List_ptr = new_subtree_root_List_ptr;
		new_subtree_root_List_ptr = help_ptr;
	}
}


void NMM::build_up_root_vertex(const Graph&G, QuadTreeNM& T)
{
	node v;

	T.init_tree();
	T.get_root_ptr()->set_Sm_level(0);
	T.get_root_ptr()->set_Sm_downleftcorner(down_left_corner);
	T.get_root_ptr()->set_Sm_boxlength(boxlength);
	T.get_root_ptr()->set_particlenumber_in_subtree(G.numberOfNodes());
	forall_nodes(v,G)
		T.get_root_ptr()->pushBack_contained_nodes(v);
}


void NMM::construct_subtree(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	QuadTreeNodeNM *subtree_root_ptr,
	List<QuadTreeNodeNM*>& new_subtree_root_List)
{
	int n = subtree_root_ptr->get_particlenumber_in_subtree();
	int subtree_depth =  static_cast<int>(max(1.0,floor(Math::log4(n))-2.0));
	int maxindex=1;

	for(int i=1; i<=subtree_depth; i++)
		maxindex *= 2;
	double subtree_min_boxlength = subtree_root_ptr->get_Sm_boxlength()/maxindex;

	if(subtree_min_boxlength >=  MIN_BOX_LENGTH)
	{
		Array2D<QuadTreeNodeNM*> leaf_ptr(0,maxindex-1,0,maxindex-1);
		T.set_act_ptr(subtree_root_ptr);
		if (find_smallest_quad(A,T)) //not all nodes have the same position
		{
			construct_complete_subtree(T,subtree_depth,leaf_ptr,0,0,0);
			set_contained_nodes_for_leaves(A,subtree_root_ptr,leaf_ptr,maxindex);
			T.set_act_ptr(subtree_root_ptr);
			set_particlenumber_in_subtree_entries(T);
			T.set_act_ptr(subtree_root_ptr);
			construct_reduced_subtree(A,T,new_subtree_root_List);
		}
	}
}


void NMM::construct_complete_subtree(
	QuadTreeNM& T,
	int subtree_depth,
	Array2D<QuadTreeNodeNM*>& leaf_ptr,
	int act_depth,
	int act_x_index,
	int act_y_index)
{
	if(act_depth < subtree_depth)
	{
		T.create_new_lt_child();
		T.create_new_rt_child();
		T.create_new_lb_child();
		T.create_new_rb_child();

		T.go_to_lt_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index,
						2*act_y_index+1);
		T.go_to_father();

		T.go_to_rt_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index+1,
						2*act_y_index+1);
		T.go_to_father();

		T.go_to_lb_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index,
						2*act_y_index);
		T.go_to_father();

		T.go_to_rb_child();
		construct_complete_subtree(T,subtree_depth,leaf_ptr,act_depth+1,2*act_x_index+1,
						2*act_y_index);
		T.go_to_father();
	}
	else if (act_depth == subtree_depth)
	{
		leaf_ptr(act_x_index,act_y_index) = T.get_act_ptr();
	}
	else
		cout<<"Error NMM::construct_complete_subtree()"<<endl;
}


void NMM::set_contained_nodes_for_leaves(
	NodeArray<NodeAttributes> &A,
	QuadTreeNodeNM* subtree_root_ptr,
	Array2D<QuadTreeNodeNM*> &leaf_ptr,
	int maxindex)
{
	node v;
	QuadTreeNodeNM* act_ptr;
	double xcoord,ycoord;
	int x_index,y_index;
	double minboxlength = subtree_root_ptr->get_Sm_boxlength()/maxindex;

	while(!subtree_root_ptr->contained_nodes_empty())
	{
		v = subtree_root_ptr->pop_contained_nodes();
		xcoord = A[v].get_x()-subtree_root_ptr->get_Sm_downleftcorner().m_x;
		ycoord = A[v].get_y()-subtree_root_ptr->get_Sm_downleftcorner().m_y;;
		x_index = int(xcoord/minboxlength);
		y_index = int(ycoord/minboxlength);
		act_ptr = leaf_ptr(x_index,y_index);
		act_ptr->pushBack_contained_nodes(v);
		act_ptr->set_particlenumber_in_subtree(act_ptr->get_particlenumber_in_subtree()+1);
	}
}


void NMM::set_particlenumber_in_subtree_entries(QuadTreeNM& T)
{
	int child_nr;

	if(!T.get_act_ptr()->is_leaf())
	{//if
		T.get_act_ptr()->set_particlenumber_in_subtree(0);

		if (T.get_act_ptr()->child_lt_exists())
		{
			T.go_to_lt_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			child_nr = T.get_act_ptr()->get_child_lt_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
		if (T.get_act_ptr()->child_rt_exists())
		{
			T.go_to_rt_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			child_nr = T.get_act_ptr()->get_child_rt_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
		if (T.get_act_ptr()->child_lb_exists())
		{
			T.go_to_lb_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			child_nr = T.get_act_ptr()->get_child_lb_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
		if (T.get_act_ptr()->child_rb_exists())
		{
			T.go_to_rb_child();
			set_particlenumber_in_subtree_entries(T);
			T.go_to_father();
			child_nr = T.get_act_ptr()->get_child_rb_ptr()->get_particlenumber_in_subtree();
			T.get_act_ptr()->set_particlenumber_in_subtree(child_nr + T.get_act_ptr()->
				get_particlenumber_in_subtree());
		}
	}//if
}


void NMM::construct_reduced_subtree(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	List<QuadTreeNodeNM*>& new_subtree_root_List)
{
	do
	{
		QuadTreeNodeNM* act_ptr = T.get_act_ptr();
		delete_empty_subtrees(T);
		T.set_act_ptr(act_ptr);
	}
	while(check_and_delete_degenerated_node(T)== true) ;

	if(!T.get_act_ptr()->is_leaf() && T.get_act_ptr()->get_particlenumber_in_subtree()
		<=  particles_in_leaves())
	{
		delete_sparse_subtree(T,T.get_act_ptr());
	}

	//push leaves that contain many particles
	if(T.get_act_ptr()->is_leaf() && T.get_act_ptr()->
		get_particlenumber_in_subtree() > particles_in_leaves())
		new_subtree_root_List.pushBack(T.get_act_ptr());

	//find smallest quad for leaves of T
	else if(T.get_act_ptr()->is_leaf() && T.get_act_ptr()->
		get_particlenumber_in_subtree() <= particles_in_leaves())
		find_smallest_quad(A,T);

	//recursive calls
	else if(!T.get_act_ptr()->is_leaf())
	{//else
		if(T.get_act_ptr()->child_lt_exists())
		{
			T.go_to_lt_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
		if(T.get_act_ptr()->child_rt_exists())
		{
			T.go_to_rt_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
		if(T.get_act_ptr()->child_lb_exists())
		{
			T.go_to_lb_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
		if(T.get_act_ptr()->child_rb_exists())
		{
			T.go_to_rb_child();
			construct_reduced_subtree(A,T,new_subtree_root_List);
			T.go_to_father();
		}
	}//else
}


void NMM::delete_empty_subtrees(QuadTreeNM& T)
{
	int child_part_nr;
	QuadTreeNodeNM* act_ptr = T.get_act_ptr();

	if(act_ptr->child_lt_exists())
	{
		child_part_nr = act_ptr->get_child_lt_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_lt_ptr());
			act_ptr->set_child_lt_ptr(NULL);
		}
	}

	if(act_ptr->child_rt_exists())
	{
		child_part_nr = act_ptr->get_child_rt_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_rt_ptr());
			act_ptr->set_child_rt_ptr(NULL);
		}
	}

	if(act_ptr->child_lb_exists())
	{
		child_part_nr = act_ptr->get_child_lb_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_lb_ptr());
			act_ptr->set_child_lb_ptr(NULL);
		}
	}

	if(act_ptr->child_rb_exists())
	{
		child_part_nr = act_ptr->get_child_rb_ptr()->get_particlenumber_in_subtree();
		if(child_part_nr == 0)
		{
			T.delete_tree(act_ptr->get_child_rb_ptr());
			act_ptr->set_child_rb_ptr(NULL);
		}
	}
}


bool NMM::check_and_delete_degenerated_node(QuadTreeNM& T)
{
	QuadTreeNodeNM* delete_ptr;
	QuadTreeNodeNM* father_ptr;
	QuadTreeNodeNM* child_ptr;

	bool lt_child = T.get_act_ptr()->child_lt_exists();
	bool rt_child = T.get_act_ptr()->child_rt_exists();
	bool lb_child = T.get_act_ptr()->child_lb_exists();
	bool rb_child = T.get_act_ptr()->child_rb_exists();
	bool is_degenerated = false;

	if(lt_child && !rt_child && !lb_child && !rb_child)
	{//if1
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_lt_ptr();
		if(T.get_act_ptr() == T.get_root_ptr())//special case
		{
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		}
		else//usual case
		{
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if(father_ptr->get_child_lt_ptr() == T.get_act_ptr())
				father_ptr->set_child_lt_ptr(child_ptr);
			else if(father_ptr->get_child_rt_ptr() == T.get_act_ptr())
				father_ptr->set_child_rt_ptr(child_ptr);
			else if(father_ptr->get_child_lb_ptr() == T.get_act_ptr())
				father_ptr->set_child_lb_ptr(child_ptr);
			else if(father_ptr->get_child_rb_ptr() == T.get_act_ptr())
				father_ptr->set_child_rb_ptr(child_ptr);
			else
				cout<<"Error NMM::delete_degenerated_node"<<endl;
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	}//if1
	else  if(!lt_child && rt_child && !lb_child && !rb_child)
	{//if2
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_rt_ptr();
		if(T.get_act_ptr() == T.get_root_ptr())//special case
		{
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		}
		else//usual case
		{
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if(father_ptr->get_child_lt_ptr() == T.get_act_ptr())
				father_ptr->set_child_lt_ptr(child_ptr);
			else if(father_ptr->get_child_rt_ptr() == T.get_act_ptr())
				father_ptr->set_child_rt_ptr(child_ptr);
			else if(father_ptr->get_child_lb_ptr() == T.get_act_ptr())
				father_ptr->set_child_lb_ptr(child_ptr);
			else if(father_ptr->get_child_rb_ptr() == T.get_act_ptr())
				father_ptr->set_child_rb_ptr(child_ptr);
			else
				cout<<"Error NMM::delete_degenerated_node"<<endl;
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	}//if2
	else  if(!lt_child && !rt_child && lb_child && !rb_child)
	{//if3
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_lb_ptr();
		if(T.get_act_ptr() == T.get_root_ptr())//special case
		{
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		}
		else//usual case
		{
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if(father_ptr->get_child_lt_ptr() == T.get_act_ptr())
				father_ptr->set_child_lt_ptr(child_ptr);
			else if(father_ptr->get_child_rt_ptr() == T.get_act_ptr())
				father_ptr->set_child_rt_ptr(child_ptr);
			else if(father_ptr->get_child_lb_ptr() == T.get_act_ptr())
				father_ptr->set_child_lb_ptr(child_ptr);
			else if(father_ptr->get_child_rb_ptr() == T.get_act_ptr())
				father_ptr->set_child_rb_ptr(child_ptr);
			else
				cout<<"Error NMM::delete_degenerated_node"<<endl;
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	}//if3
	else  if(!lt_child && !rt_child && !lb_child && rb_child)
	{//if4
		is_degenerated = true;
		delete_ptr = T.get_act_ptr();
		child_ptr = T.get_act_ptr()->get_child_rb_ptr();
		if(T.get_act_ptr() == T.get_root_ptr())//special case
		{
			T.set_root_ptr(child_ptr);
			T.set_act_ptr(T.get_root_ptr());
		}
		else//usual case
		{
			father_ptr = T.get_act_ptr()->get_father_ptr();
			child_ptr->set_father_ptr(father_ptr);
			if(father_ptr->get_child_lt_ptr() == T.get_act_ptr())
				father_ptr->set_child_lt_ptr(child_ptr);
			else if(father_ptr->get_child_rt_ptr() == T.get_act_ptr())
				father_ptr->set_child_rt_ptr(child_ptr);
			else if(father_ptr->get_child_lb_ptr() == T.get_act_ptr())
				father_ptr->set_child_lb_ptr(child_ptr);
			else if(father_ptr->get_child_rb_ptr() == T.get_act_ptr())
				father_ptr->set_child_rb_ptr(child_ptr);
			else
				cout<<"Error NMM::delete_degenerated_node"<<endl;
			T.set_act_ptr(child_ptr);
		}
		delete delete_ptr;
	}//if4
	return is_degenerated;
}


void NMM::delete_sparse_subtree(QuadTreeNM& T, QuadTreeNodeNM* new_leaf_ptr)
{
	collect_contained_nodes(T,new_leaf_ptr);

	if(new_leaf_ptr->child_lt_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_lt_ptr());
		new_leaf_ptr->set_child_lt_ptr(NULL);
	}
	if(new_leaf_ptr->child_rt_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_rt_ptr());
		new_leaf_ptr->set_child_rt_ptr(NULL);
	}
	if(new_leaf_ptr->child_lb_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_lb_ptr());
		new_leaf_ptr->set_child_lb_ptr(NULL);
	}
	if(new_leaf_ptr->child_rb_exists())
	{
		T.delete_tree(new_leaf_ptr->get_child_rb_ptr());
		new_leaf_ptr->set_child_rb_ptr(NULL);
	}
}


void NMM::collect_contained_nodes(QuadTreeNM& T, QuadTreeNodeNM* new_leaf_ptr)
{
	if(T.get_act_ptr()->is_leaf())
		while(!T.get_act_ptr()->contained_nodes_empty())
			new_leaf_ptr->pushBack_contained_nodes(T.get_act_ptr()->pop_contained_nodes());
	else if(T.get_act_ptr()->child_lt_exists())
	{
		T.go_to_lt_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
	if(T.get_act_ptr()->child_rt_exists())
	{
		T.go_to_rt_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
	if(T.get_act_ptr()->child_lb_exists())
	{
		T.go_to_lb_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
	if(T.get_act_ptr()->child_rb_exists())
	{
		T.go_to_rb_child();
		collect_contained_nodes(T,new_leaf_ptr);
		T.go_to_father();
	}
}


bool NMM::find_smallest_quad(NodeArray<NodeAttributes>& A, QuadTreeNM& T)
{
	OGDF_ASSERT(!T.get_act_ptr()->contained_nodes_empty());
	//if(T.get_act_ptr()->contained_nodes_empty())
	//  cout<<"Error NMM :: find_smallest_quad()"<<endl;
	//else
	// {//else
	List<node>L;
	T.get_act_ptr()->get_contained_nodes(L);
	node v = L.popFrontRet();
	double x_min = A[v].get_x();
	double x_max = x_min;
	double y_min = A[v].get_y();
	double y_max = y_min;

	while(! L.empty())
	{
		v = L.popFrontRet();
		if(A[v].get_x() < x_min)
			x_min = A[v].get_x();
		if(A[v].get_x() > x_max)
			x_max = A[v].get_x();
		if(A[v].get_y() < y_min)
			y_min = A[v].get_y();
		if(A[v].get_y() > y_max)
			y_max = A[v].get_y();
	}
	if(x_min != x_max || y_min != y_max) //nodes are not all at the same position
	{
		if(find_sm_cell() == FMMMLayout::scfIteratively)
			find_small_cell_iteratively(T.get_act_ptr(),x_min,x_max,y_min,y_max);
		else //find_sm_cell == FMMMLayout::scfAluru
			find_small_cell_iteratively(T.get_act_ptr(),x_min,x_max,y_min,y_max);
		return true;
	}
	else
		return false;
	//}//else
}


// ********Functions needed for subtree by subtree  tree construction(END)************

void NMM::find_small_cell_iteratively(
	QuadTreeNodeNM* act_ptr,
	double x_min,
	double x_max,
	double y_min,
	double y_max)
{
	int new_level;
	double new_boxlength;
	DPoint new_dlc;
	bool Sm_cell_found = false;

	while ( !Sm_cell_found && ((x_max-x_min >=MIN_BOX_LENGTH) ||
		(y_max-y_min >=MIN_BOX_LENGTH)) )
	{
		if(in_lt_quad(act_ptr,x_min,x_max,y_min,y_max))
		{
			new_level = act_ptr->get_Sm_level()+1;
			new_boxlength = act_ptr->get_Sm_boxlength()/2;
			new_dlc.m_x = act_ptr->get_Sm_downleftcorner().m_x;
			new_dlc.m_y = act_ptr->get_Sm_downleftcorner().m_y+new_boxlength;
			act_ptr->set_Sm_level(new_level);
			act_ptr->set_Sm_boxlength(new_boxlength);
			act_ptr->set_Sm_downleftcorner(new_dlc);
		}
		else if(in_rt_quad(act_ptr,x_min,x_max,y_min,y_max))
		{
			new_level = act_ptr->get_Sm_level()+1;
			new_boxlength = act_ptr->get_Sm_boxlength()/2;
			new_dlc.m_x = act_ptr->get_Sm_downleftcorner().m_x+new_boxlength;
			new_dlc.m_y = act_ptr->get_Sm_downleftcorner().m_y+new_boxlength;
			act_ptr->set_Sm_level(new_level);
			act_ptr->set_Sm_boxlength(new_boxlength);
			act_ptr->set_Sm_downleftcorner(new_dlc);
		}
		else if(in_lb_quad(act_ptr,x_min,x_max,y_min,y_max))
		{
			new_level = act_ptr->get_Sm_level()+1;
			new_boxlength = act_ptr->get_Sm_boxlength()/2;
			act_ptr->set_Sm_level(new_level);
			act_ptr->set_Sm_boxlength(new_boxlength);
		}
		else if(in_rb_quad(act_ptr,x_min,x_max,y_min,y_max))
		{
			new_level = act_ptr->get_Sm_level()+1;
			new_boxlength = act_ptr->get_Sm_boxlength()/2;
			new_dlc.m_x = act_ptr->get_Sm_downleftcorner().m_x+new_boxlength;
			new_dlc.m_y = act_ptr->get_Sm_downleftcorner().m_y;
			act_ptr->set_Sm_level(new_level);
			act_ptr->set_Sm_boxlength(new_boxlength);
			act_ptr->set_Sm_downleftcorner(new_dlc);
		}
		else Sm_cell_found = true;
	}
}


void NMM::find_small_cell_by_formula(
	QuadTreeNodeNM* act_ptr,
	double x_min,
	double x_max,
	double y_min,
	double y_max)
{
	numexcept N;
	int level_offset = act_ptr->get_Sm_level();
	max_power_of_2_index = 30;//up to this level standard integer arithmetic is used
	DPoint nullpoint (0,0);
	IPoint Sm_position;
	double Sm_dlc_x_coord,Sm_dlc_y_coord;
	double Sm_boxlength;
	int Sm_level;
	DPoint Sm_downleftcorner;
	int j_x = max_power_of_2_index+1;
	int j_y = max_power_of_2_index+1;
	bool rectangle_is_horizontal_line = false;
	bool rectangle_is_vertical_line = false;
	bool rectangle_is_point = false;

	//shift boundaries to the origin for easy calculations
	double x_min_old = x_min;
	double x_max_old = x_max;
	double y_min_old = y_min;
	double y_max_old = y_max;

	Sm_boxlength = act_ptr->get_Sm_boxlength();
	Sm_dlc_x_coord = act_ptr->get_Sm_downleftcorner().m_x;
	Sm_dlc_y_coord = act_ptr->get_Sm_downleftcorner().m_y;

	x_min -= Sm_dlc_x_coord;
	x_max -= Sm_dlc_x_coord;
	y_min -= Sm_dlc_y_coord;
	y_max -= Sm_dlc_y_coord;

	//check if iterative way has to be used
	if (x_min == x_max && y_min == y_max)
		rectangle_is_point = true;
	else if(x_min == x_max && y_min != y_max)
		rectangle_is_vertical_line = true;
	else //x_min != x_max
		j_x = static_cast<int>(ceil(Math::log2(Sm_boxlength/(x_max-x_min))));

	if(x_min != x_max && y_min == y_max)
		rectangle_is_horizontal_line = true;
	else //y_min != y_max
		j_y = static_cast<int>(ceil(Math::log2(Sm_boxlength/(y_max-y_min))));

	if(rectangle_is_point)
	{
		;//keep the old values
	}
	else if ( !N.nearly_equal((x_min_old - x_max_old),(x_min-x_max)) ||
		!N.nearly_equal((y_min_old - y_max_old),(y_min-y_max)) ||
		x_min/Sm_boxlength < MIN_BOX_LENGTH || x_max/Sm_boxlength < MIN_BOX_LENGTH ||
		y_min/Sm_boxlength < MIN_BOX_LENGTH || y_max/Sm_boxlength < MIN_BOX_LENGTH )
		find_small_cell_iteratively(act_ptr,x_min_old,x_max_old,y_min_old,y_max_old);
	else if ( ((j_x > max_power_of_2_index) && (j_y > max_power_of_2_index)) ||
		((j_x > max_power_of_2_index) && !rectangle_is_vertical_line) ||
		((j_y > max_power_of_2_index) && !rectangle_is_horizontal_line) )
		find_small_cell_iteratively(act_ptr,x_min_old,x_max_old,y_min_old,y_max_old);
	else //idea of Aluru et al.
	{//else
		int k,a1,a2,A,j_minus_k;
		double h1,h2;
		int Sm_x_level,Sm_y_level;
		int Sm_x_position,Sm_y_position;

		if(x_min != x_max)
		{//if1
			//calculate Sm_x_level and Sm_x_position
			a1 = static_cast<int>(ceil((x_min/Sm_boxlength)*power_of_two(j_x)));
			a2 = static_cast<int>(floor((x_max/Sm_boxlength)*power_of_two(j_x)));
			h1 = (Sm_boxlength/power_of_two(j_x))* a1;
			h2 = (Sm_boxlength/power_of_two(j_x))* a2;

			//special cases: two tangents or left tangent and righ cutline
			if(((h1 == x_min)&&(h2 == x_max)) || ((h1 == x_min) && (h2 != x_max)) )
				A = a2;
			else if (a1 == a2)  //only one cutline
				A = a1;
			else  //two cutlines or a right tangent and a left cutline (usual case)
			{
				if((a1 % 2) == 0)
					A = a1;
				else
					A = a2;
			}

			j_minus_k = static_cast<int>(Math::log2(1+(A ^ (A-1)))-1);
			k = j_x - j_minus_k;
			Sm_x_level = k-1;
			Sm_x_position = a1/ power_of_two(j_x - Sm_x_level);
		}//if1

		if(y_min != y_max)
		{//if2
			//calculate Sm_y_level and Sm_y_position
			a1 = static_cast<int>(ceil((y_min/Sm_boxlength)*power_of_two(j_y)));
			a2 = static_cast<int>(floor((y_max/Sm_boxlength)*power_of_two(j_y)));
			h1 = (Sm_boxlength/power_of_two(j_y))* a1;
			h2 = (Sm_boxlength/power_of_two(j_y))* a2;

			//special cases: two tangents or bottom tangent and top cutline
			if(((h1 == y_min)&&(h2 == y_max)) || ((h1 == y_min) && (h2 != y_max)) )
				A = a2;
			else if (a1 == a2)  //only one cutline
				A = a1;
			else  //two cutlines or a top tangent and a bottom cutline (usual case)
			{
				if((a1 % 2) == 0)
					A = a1;
				else
					A = a2;
			}

			j_minus_k = static_cast<int>(Math::log2(1+(A ^ (A-1)))-1);
			k = j_y - j_minus_k;
			Sm_y_level = k-1;
			Sm_y_position = a1/ power_of_two(j_y - Sm_y_level);
		}//if2

		if((x_min != x_max) &&(y_min != y_max))//a box with area > 0
		{//if3
			if (Sm_x_level == Sm_y_level)
			{
				Sm_level = Sm_x_level;
				Sm_position.m_x = Sm_x_position;
				Sm_position.m_y = Sm_y_position;
			}
			else if (Sm_x_level < Sm_y_level)
			{
				Sm_level = Sm_x_level;
				Sm_position.m_x = Sm_x_position;
				Sm_position.m_y = Sm_y_position/power_of_two(Sm_y_level-Sm_x_level);
			}
			else //Sm_x_level > Sm_y_level
			{
				Sm_level = Sm_y_level;
				Sm_position.m_x = Sm_x_position/power_of_two(Sm_x_level-Sm_y_level);
				Sm_position.m_y = Sm_y_position;
			}
		}//if3
		else if(x_min == x_max) //a vertical line
		{//if4
			Sm_level = Sm_y_level;
			Sm_position.m_x = static_cast<int> (floor((x_min*power_of_two(Sm_level))/
				Sm_boxlength));
			Sm_position.m_y = Sm_y_position;
		}//if4
		else //y_min == y_max (a horizontal line)
		{//if5
			Sm_level = Sm_x_level;
			Sm_position.m_x = Sm_x_position;
			Sm_position.m_y = static_cast<int> (floor((y_min*power_of_two(Sm_level))/
				Sm_boxlength));
		}//if5

		Sm_boxlength = Sm_boxlength/power_of_two(Sm_level);
		Sm_downleftcorner.m_x = Sm_dlc_x_coord + Sm_boxlength * Sm_position.m_x;
		Sm_downleftcorner.m_y = Sm_dlc_y_coord + Sm_boxlength * Sm_position.m_y;
		act_ptr->set_Sm_level(Sm_level+level_offset);
		act_ptr->set_Sm_boxlength(Sm_boxlength);
		act_ptr->set_Sm_downleftcorner(Sm_downleftcorner);
	}//else
}


inline void NMM::delete_red_quad_tree_and_count_treenodes(QuadTreeNM& T)
{
	T.delete_tree(T.get_root_ptr());
}


inline void NMM::form_multipole_expansions(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	List<QuadTreeNodeNM*>& quad_tree_leaves)
{
	T.set_act_ptr(T.get_root_ptr());
	form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
}


void NMM::form_multipole_expansion_of_subtree(
	NodeArray<NodeAttributes>& A,
	QuadTreeNM& T,
	List<QuadTreeNodeNM*>& quad_tree_leaves)
{
	init_expansion_Lists(T.get_act_ptr());
	set_center(T.get_act_ptr());

	if(T.get_act_ptr()->is_leaf()) //form expansions for leaf nodes
	{//if
		quad_tree_leaves.pushBack(T.get_act_ptr());
		form_multipole_expansion_of_leaf_node(A,T.get_act_ptr());
	}//if
	else //rekursive calls and add shifted expansions
	{//else
		if(T.get_act_ptr()->child_lt_exists())
		{
			T.go_to_lt_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
		if(T.get_act_ptr()->child_rt_exists())
		{
			T.go_to_rt_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
		if(T.get_act_ptr()->child_lb_exists())
		{
			T.go_to_lb_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
		if(T.get_act_ptr()->child_rb_exists())
		{
			T.go_to_rb_child();
			form_multipole_expansion_of_subtree(A,T,quad_tree_leaves);
			add_shifted_expansion_to_father_expansion(T.get_act_ptr());
			T.go_to_father();
		}
	}//else
}


inline void NMM::init_expansion_Lists(QuadTreeNodeNM* act_ptr)
{
	int i;
	Array<complex<double> > nulList (precision()+1);

	for (i = 0;i<=precision();i++)
		nulList[i] = 0;

	act_ptr->set_multipole_exp(nulList,precision());
	act_ptr->set_locale_exp(nulList,precision());
}


void NMM::set_center(QuadTreeNodeNM* act_ptr)
{

	const int BILLION = 1000000000;
	DPoint Sm_downleftcorner = act_ptr->get_Sm_downleftcorner();
	double Sm_boxlength = act_ptr->get_Sm_boxlength();
	double boxcenter_x_coord,boxcenter_y_coord;
	DPoint Sm_dlc;
	double rand_y;

	boxcenter_x_coord = Sm_downleftcorner.m_x + Sm_boxlength * 0.5;
	boxcenter_y_coord = Sm_downleftcorner.m_y + Sm_boxlength * 0.5;

	//for use of complex logarithm: waggle the y-coordinates a little bit
	//such that the new center is really inside the actual box and near the exact center
	rand_y = double(randomNumber(1,BILLION)+1)/(BILLION+2);//rand number in (0,1)
	boxcenter_y_coord = boxcenter_y_coord + 0.001 * Sm_boxlength * rand_y;

	complex<double> boxcenter(boxcenter_x_coord,boxcenter_y_coord);
	act_ptr->set_Sm_center(boxcenter);
}


void NMM::form_multipole_expansion_of_leaf_node(
	NodeArray<NodeAttributes>& A,
	QuadTreeNodeNM* act_ptr)
{
	int k;
	complex<double> Q (0,0);
	complex<double> z_0 = act_ptr->get_Sm_center();//center of actual box
	complex<double> nullpoint (0,0);
	Array<complex<double> > coef (precision()+1);
	complex<double> z_v_minus_z_0_over_k;
	List<node> nodes_in_box;
	int i;
	ListIterator<node> v_it;

	act_ptr->get_contained_nodes(nodes_in_box);

	for(v_it = nodes_in_box.begin();v_it.valid();++v_it)
		Q += 1;
	coef[0] = Q;

	for(i = 1; i<=precision();i++)
		coef[i] = nullpoint;

	for(v_it = nodes_in_box.begin();v_it.valid();++v_it)
	{
		complex<double> z_v (A[*v_it].get_x(),A[*v_it].get_y());
		z_v_minus_z_0_over_k = z_v - z_0;
		for(k=1;k<=precision();k++)
		{
			coef[k] += ((double(-1))*z_v_minus_z_0_over_k)/double(k);
			z_v_minus_z_0_over_k *= z_v - z_0;
		}
	}
	act_ptr->replace_multipole_exp(coef,precision());
}


void NMM::add_shifted_expansion_to_father_expansion(QuadTreeNodeNM* act_ptr)
{
	QuadTreeNodeNM* father_ptr = act_ptr->get_father_ptr();
	complex<double> sum;
	complex<double> z_0,z_1;
	Array<complex<double> > z_0_minus_z_1_over (precision()+1);

	z_1 = father_ptr->get_Sm_center();
	z_0 = act_ptr->get_Sm_center();
	father_ptr->get_multipole_exp()[0] += act_ptr->get_multipole_exp()[0];

	//init z_0_minus_z_1_over
	z_0_minus_z_1_over[0] = 1;
	for(int i = 1; i<= precision(); i++)
		z_0_minus_z_1_over[i] = z_0_minus_z_1_over[i-1] * (z_0 - z_1);

	for(int k=1; k<=precision(); k++)
	{
		sum = (act_ptr->get_multipole_exp()[0]*(double(-1))*z_0_minus_z_1_over[k])/
			double(k) ;
		for(int s=1; s<=k; s++)
			sum +=  act_ptr->get_multipole_exp()[s]*z_0_minus_z_1_over[k-s]* binko(k-1,s-1);
		father_ptr->get_multipole_exp()[k] += sum;
	}
}


void NMM::calculate_local_expansions_and_WSPRLS(
	NodeArray<NodeAttributes>&A,
	QuadTreeNodeNM* act_node_ptr)
{
	List<QuadTreeNodeNM*> I,L,L2,E,D1,D2,M;
	QuadTreeNodeNM *father_ptr,*selected_node_ptr;
	ListIterator<QuadTreeNodeNM*> ptr_it;

	//Step 0: Initializations
	if(! act_node_ptr->is_root())
		father_ptr = act_node_ptr->get_father_ptr();
	I.clear();L.clear();L2.clear();D1.clear();D2.clear();M.clear();

	//Step 1: calculate Lists I (min. ill sep. set), L (interaction List of well sep.
	//nodes , they are used to form the Local Expansions from the multipole expansions),
	//L2 (non bordering leaves that have a larger or equal Sm-cell and  are ill separated;
	//empty if the actual node is a leaf)
	//calculate List D1(bordering leaves that have a larger or equal Sm-cell and are
	//ill separated) and D2 (non bordering leaves that have a larger or equal Sm-cell and
	//are ill separated;empty if the actual node is an interior node)

	//special case: act_node is the root of T
	if (act_node_ptr->is_root())
	{//if
		E.clear();
		if(act_node_ptr->child_lt_exists())
			E.pushBack(act_node_ptr->get_child_lt_ptr());
		if(act_node_ptr->child_rt_exists())
			E.pushBack(act_node_ptr->get_child_rt_ptr());
		if(act_node_ptr->child_lb_exists())
			E.pushBack(act_node_ptr->get_child_lb_ptr());
		if(act_node_ptr->child_rb_exists())
			E.pushBack(act_node_ptr->get_child_rb_ptr());
	}//if

	//usual case: act_node is an interior node of T
	else
	{
		father_ptr->get_D1(E); //bordering leaves of father
		father_ptr->get_I(I);  //min ill sep. nodes of father


		for(ptr_it = I.begin();ptr_it.valid();++ptr_it)
			E.pushBack(*ptr_it);
		I.clear();
	}


	while (!E.empty())
	{//while
		selected_node_ptr = E.popFrontRet();
		if (well_separated(act_node_ptr,selected_node_ptr))
			L.pushBack(selected_node_ptr);
		else if (act_node_ptr->get_Sm_level() < selected_node_ptr->get_Sm_level())
			I.pushBack(selected_node_ptr);
		else if(!selected_node_ptr->is_leaf())
		{
			if(selected_node_ptr->child_lt_exists())
				E.pushBack(selected_node_ptr->get_child_lt_ptr());
			if(selected_node_ptr->child_rt_exists())
				E.pushBack(selected_node_ptr->get_child_rt_ptr());
			if(selected_node_ptr->child_lb_exists())
				E.pushBack(selected_node_ptr->get_child_lb_ptr());
			if(selected_node_ptr->child_rb_exists())
				E.pushBack(selected_node_ptr->get_child_rb_ptr());
		}
		else if(bordering(act_node_ptr,selected_node_ptr))
			D1.pushBack(selected_node_ptr);
		else if( (selected_node_ptr != act_node_ptr)&&(act_node_ptr->is_leaf()))
			D2.pushBack(selected_node_ptr); //direct calculation (no errors produced)
		else if((selected_node_ptr != act_node_ptr)&&!(act_node_ptr->is_leaf()))
			L2.pushBack(selected_node_ptr);
	}//while

	act_node_ptr->set_I(I);
	act_node_ptr->set_D1(D1);
	act_node_ptr->set_D2(D2);

	//Step 2: add local expansions from father(act_node_ptr) and calculate locale
	//expansions for all nodes in L
	if(!act_node_ptr->is_root())
		add_shifted_local_exp_of_parent(act_node_ptr);

	for(ptr_it = L.begin();ptr_it.valid();++ptr_it)
		add_local_expansion(*ptr_it,act_node_ptr);

	//Step 3: calculate locale expansions for all nodes in D2 (simpler than in Step 2)

	for(ptr_it = L2.begin();ptr_it.valid();++ptr_it)
		add_local_expansion_of_leaf(A,*ptr_it,act_node_ptr);

	//Step 4: recursive calls if act_node is not a leaf
	if(!act_node_ptr->is_leaf())
	{
		if(act_node_ptr->child_lt_exists())
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_lt_ptr());
		if(act_node_ptr->child_rt_exists())
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_rt_ptr());
		if(act_node_ptr->child_lb_exists())
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_lb_ptr());
		if(act_node_ptr->child_rb_exists())
			calculate_local_expansions_and_WSPRLS(A,act_node_ptr->get_child_rb_ptr());
	}

	//Step 5: WSPRLS(Well Separateness Preserving Refinement of leaf surroundings)
	//if act_node is a leaf than calculate the list D1,D2 and M from I and D1
	else // *act_node_ptr is a leaf
	{//else
		act_node_ptr->get_D1(D1);
		act_node_ptr->get_D2(D2);

		while(!I.empty())
		{//while
			selected_node_ptr = I.popFrontRet();
			if(selected_node_ptr->is_leaf())
			{
				//here D1 contains larger AND smaller bordering leaves!
				if(bordering(act_node_ptr,selected_node_ptr))
					D1.pushBack(selected_node_ptr);
				else
					D2.pushBack(selected_node_ptr);
			}
			else //!selected_node_ptr->is_leaf()
			{
				if(bordering(act_node_ptr,selected_node_ptr))
				{
					if(selected_node_ptr->child_lt_exists())
						I.pushBack(selected_node_ptr->get_child_lt_ptr());
					if(selected_node_ptr->child_rt_exists())
						I.pushBack(selected_node_ptr->get_child_rt_ptr());
					if(selected_node_ptr->child_lb_exists())
						I.pushBack(selected_node_ptr->get_child_lb_ptr());
					if(selected_node_ptr->child_rb_exists())
						I.pushBack(selected_node_ptr->get_child_rb_ptr());
				}
				else
					M.pushBack(selected_node_ptr);
			}
		}//while
		act_node_ptr->set_D1(D1);
		act_node_ptr->set_D2(D2);
		act_node_ptr->set_M(M);
	}//else
}


bool NMM::well_separated(QuadTreeNodeNM* node_1_ptr, QuadTreeNodeNM* node_2_ptr)
{
	numexcept N;
	double boxlength_1 = node_1_ptr->get_Sm_boxlength();
	double boxlength_2 = node_2_ptr->get_Sm_boxlength();
	double x1_min,x1_max,y1_min,y1_max,x2_min,x2_max,y2_min,y2_max;
	bool x_overlap,y_overlap;

	if(boxlength_1 <= boxlength_2)
	{
		x1_min = node_1_ptr->get_Sm_downleftcorner().m_x;
		x1_max = node_1_ptr->get_Sm_downleftcorner().m_x+boxlength_1;
		y1_min = node_1_ptr->get_Sm_downleftcorner().m_y;
		y1_max = node_1_ptr->get_Sm_downleftcorner().m_y+boxlength_1;

		//blow the box up
		x2_min = node_2_ptr->get_Sm_downleftcorner().m_x-boxlength_2;
		x2_max = node_2_ptr->get_Sm_downleftcorner().m_x+2*boxlength_2;
		y2_min = node_2_ptr->get_Sm_downleftcorner().m_y-boxlength_2;
		y2_max = node_2_ptr->get_Sm_downleftcorner().m_y+2*boxlength_2;
	}
	else //boxlength_1 > boxlength_2
	{
		//blow the box up
		x1_min = node_1_ptr->get_Sm_downleftcorner().m_x-boxlength_1;
		x1_max = node_1_ptr->get_Sm_downleftcorner().m_x+2*boxlength_1;
		y1_min = node_1_ptr->get_Sm_downleftcorner().m_y-boxlength_1;
		y1_max = node_1_ptr->get_Sm_downleftcorner().m_y+2*boxlength_1;

		x2_min = node_2_ptr->get_Sm_downleftcorner().m_x;
		x2_max = node_2_ptr->get_Sm_downleftcorner().m_x+boxlength_2;
		y2_min = node_2_ptr->get_Sm_downleftcorner().m_y;
		y2_max = node_2_ptr->get_Sm_downleftcorner().m_y+boxlength_2;
	}

	//test if boxes overlap
	if((x1_max <= x2_min)|| N.nearly_equal(x1_max,x2_min)||
		(x2_max <= x1_min)|| N.nearly_equal(x2_max,x1_min))
		x_overlap = false;
	else
		x_overlap = true;
	if((y1_max <= y2_min)|| N.nearly_equal(y1_max,y2_min)||
		(y2_max <= y1_min)|| N.nearly_equal(y2_max,y1_min))
		y_overlap = false;
	else
		y_overlap = true;

	if (x_overlap  && y_overlap)
		return false;
	else
		return true;
}


bool NMM::bordering(QuadTreeNodeNM* node_1_ptr,QuadTreeNodeNM* node_2_ptr)
{
	numexcept N;
	double boxlength_1 = node_1_ptr->get_Sm_boxlength();
	double boxlength_2 = node_2_ptr->get_Sm_boxlength();
	double x1_min = node_1_ptr->get_Sm_downleftcorner().m_x;
	double x1_max = node_1_ptr->get_Sm_downleftcorner().m_x+boxlength_1;
	double y1_min = node_1_ptr->get_Sm_downleftcorner().m_y;
	double y1_max = node_1_ptr->get_Sm_downleftcorner().m_y+boxlength_1;
	double x2_min = node_2_ptr->get_Sm_downleftcorner().m_x;
	double x2_max = node_2_ptr->get_Sm_downleftcorner().m_x+boxlength_2;
	double y2_min = node_2_ptr->get_Sm_downleftcorner().m_y;
	double y2_max = node_2_ptr->get_Sm_downleftcorner().m_y+boxlength_2;

	if( ( (x2_min <= x1_min || N.nearly_equal(x2_min,x1_min)) &&
		(x1_max <= x2_max || N.nearly_equal(x1_max,x2_max)) &&
		(y2_min <= y1_min || N.nearly_equal(y2_min,y1_min)) &&
		(y1_max <= y2_max || N.nearly_equal(y1_max,y2_max))    ) ||
		( (x1_min <= x2_min || N.nearly_equal(x1_min,x2_min)) &&
		(x2_max <= x1_max || N.nearly_equal(x2_max,x1_max)) &&
		(y1_min <= y2_min || N.nearly_equal(y1_min,y2_min)) &&
		(y2_max <= y1_max || N.nearly_equal(y2_max,y1_max))    ) )
		return false; //one box contains the other box(inclusive neighbours)
	else
	{//else
		if (boxlength_1 <= boxlength_2)
		{ //shift box1
			if (x1_min < x2_min)
			{ x1_min +=boxlength_1;x1_max +=boxlength_1; }
			else if  (x1_max > x2_max)
			{ x1_min -=boxlength_1;x1_max -=boxlength_1; }
			if (y1_min < y2_min)
			{ y1_min +=boxlength_1;y1_max +=boxlength_1; }
			else if  (y1_max > y2_max)
			{ y1_min -=boxlength_1;y1_max -=boxlength_1; }
		}
		else //boxlength_1 > boxlength_2
		{//shift box2
			if (x2_min < x1_min)
			{ x2_min +=boxlength_2;x2_max +=boxlength_2; }
			else if  (x2_max > x1_max)
			{ x2_min -=boxlength_2;x2_max -=boxlength_2; }
			if (y2_min < y1_min)
			{ y2_min +=boxlength_2;y2_max +=boxlength_2; }
			else if  (y2_max > y1_max)
			{ y2_min -=boxlength_2;y2_max -=boxlength_2; }
		}
		if( ( (x2_min <= x1_min || N.nearly_equal(x2_min,x1_min)) &&
			(x1_max <= x2_max || N.nearly_equal(x1_max,x2_max)) &&
			(y2_min <= y1_min || N.nearly_equal(y2_min,y1_min)) &&
			(y1_max <= y2_max || N.nearly_equal(y1_max,y2_max))    ) ||
			( (x1_min <= x2_min || N.nearly_equal(x1_min,x2_min)) &&
			(x2_max <= x1_max || N.nearly_equal(x2_max,x1_max)) &&
			(y1_min <= y2_min || N.nearly_equal(y1_min,y2_min)) &&
			(y2_max <= y1_max || N.nearly_equal(y2_max,y1_max))    ) )
			return true;
		else
			return false;
	}//else
}


void NMM::add_shifted_local_exp_of_parent(QuadTreeNodeNM* node_ptr)
{
	QuadTreeNodeNM* father_ptr = node_ptr->get_father_ptr();

	complex<double> z_0 = father_ptr->get_Sm_center();
	complex<double> z_1 = node_ptr->get_Sm_center();
	Array<complex<double> > z_1_minus_z_0_over (precision()+1);

	//init z_1_minus_z_0_over
	z_1_minus_z_0_over[0] = 1;
	for(int i = 1; i<= precision(); i++)
		z_1_minus_z_0_over[i] = z_1_minus_z_0_over[i-1] * (z_1 - z_0);


	for(int l = 0; l <= precision();l++)
	{
		complex<double> sum (0,0);
		for(int k = l;k<=precision();k++)
			sum += binko(k,l)*father_ptr->get_local_exp()[k]*z_1_minus_z_0_over[k-l];
		node_ptr->get_local_exp()[l] += sum;
	}
}


void NMM::add_local_expansion(QuadTreeNodeNM* ptr_0, QuadTreeNodeNM* ptr_1)
{
	complex<double> z_0 = ptr_0->get_Sm_center();
	complex<double> z_1 = ptr_1->get_Sm_center();
	complex<double> sum, z_error;
	complex<double> factor;
	complex<double> z_1_minus_z_0_over_k;
	complex<double> z_1_minus_z_0_over_s;
	complex<double> pow_minus_1_s_plus_1;
	complex<double> pow_minus_1_s;

	//Error-Handling for complex logarithm
	if ((std::real(z_1-z_0) <=0) && (std::imag(z_1-z_0) == 0)) //no cont. compl. log fct exists !!!
	{
		z_error = log(z_1 -z_0 + 0.0000001);
		sum = ptr_0->get_multipole_exp()[0] * z_error;
	}
	else
		sum = ptr_0->get_multipole_exp()[0]* log(z_1-z_0);


	z_1_minus_z_0_over_k = z_1 - z_0;
	for(int k = 1; k<=precision(); k++)
	{
		sum += ptr_0->get_multipole_exp()[k]/z_1_minus_z_0_over_k;
		z_1_minus_z_0_over_k *= z_1-z_0;
	}
	ptr_1->get_local_exp()[0] += sum;

	z_1_minus_z_0_over_s = z_1 - z_0;
	for (int s = 1; s <= precision(); s++)
	{
		pow_minus_1_s_plus_1 = (((s+1)% 2 == 0) ? 1 : -1);
		pow_minus_1_s = ((pow_minus_1_s_plus_1 == double(1))? -1 : 1);
		sum = pow_minus_1_s_plus_1*ptr_0->get_multipole_exp()[0]/(z_1_minus_z_0_over_s *
			double(s));
		factor = pow_minus_1_s/z_1_minus_z_0_over_s;
		z_1_minus_z_0_over_s *= z_1-z_0;
		complex<double> sum_2 (0,0);

		z_1_minus_z_0_over_k = z_1 - z_0;
		for(int k=1; k<=precision(); k++)
		{
			sum_2 += binko(s+k-1,k-1)*ptr_0->get_multipole_exp()[k]/z_1_minus_z_0_over_k;
			z_1_minus_z_0_over_k *= z_1-z_0;
		}
		ptr_1->get_local_exp()[s] += sum + factor* sum_2;
	}
}


void NMM::add_local_expansion_of_leaf(
	NodeArray<NodeAttributes>&A,
	QuadTreeNodeNM* ptr_0,
	QuadTreeNodeNM* ptr_1)
{
	List<node> contained_nodes;
	double multipole_0_of_v = 1;//only the first coefficient is not zero
	complex<double> z_1 = ptr_1->get_Sm_center();
	complex<double> z_error;
	complex<double> z_1_minus_z_0_over_s;
	complex<double> pow_minus_1_s_plus_1;

	ptr_0->get_contained_nodes(contained_nodes);

	forall_listiterators(node, v_it, contained_nodes)
	{//forall
		//set position of v as center ( (1,0,....,0) are the multipole coefficients at v)
		complex<double> z_0  (A[*v_it].get_x(),A[*v_it].get_y());

		//now transform multipole_0_of_v to the locale expansion around z_1

		//Error-Handling for complex logarithm
		if ((std::real(z_1-z_0) <=0) && (std::imag(z_1-z_0) == 0)) //no cont. compl. log fct exists!
		{
			z_error = log(z_1 -z_0 + 0.0000001);
			ptr_1->get_local_exp()[0] += multipole_0_of_v * z_error;
		}
		else
			ptr_1->get_local_exp()[0] +=  multipole_0_of_v * log(z_1-z_0);

		z_1_minus_z_0_over_s = z_1 - z_0;
		for (int s = 1;s <= precision();s++)
		{
			pow_minus_1_s_plus_1 = (((s+1)% 2 == 0) ? 1 : -1);
			ptr_1->get_local_exp()[s] += pow_minus_1_s_plus_1*multipole_0_of_v/
				(z_1_minus_z_0_over_s * double(s));
			z_1_minus_z_0_over_s *= z_1-z_0;
		}
	}//forall
}


void NMM::transform_local_exp_to_forces(
	NodeArray <NodeAttributes>&A,
	List<QuadTreeNodeNM*>& quad_tree_leaves,
	NodeArray<DPoint>& F_local_exp)
{
	List<node> contained_nodes;
	complex<double> sum;
	complex<double> complex_null (0,0);
	complex<double> z_0;
	complex<double> z_v_minus_z_0_over_k_minus_1;
	DPoint force_vector;

	//calculate derivative of the potential polynom (= local expansion at leaf nodes)
	//and evaluate it for each node in contained_nodes()
	//and transform the complex number back to the real-world, to obtain the force

	forall_listiterators( QuadTreeNodeNM*, leaf_ptr_ptr,quad_tree_leaves)
	{
		(*leaf_ptr_ptr)->get_contained_nodes(contained_nodes);
		z_0 = (*leaf_ptr_ptr)->get_Sm_center();

		forall_listiterators(node, v_ptr,contained_nodes)
		{
			complex<double> z_v (A[*v_ptr].get_x(),A[*v_ptr].get_y());
			sum = complex_null;
			z_v_minus_z_0_over_k_minus_1 = 1;
			for(int k=1; k<=precision(); k++)
			{
				sum += double(k) * (*leaf_ptr_ptr)->get_local_exp()[k] *
					z_v_minus_z_0_over_k_minus_1;
				z_v_minus_z_0_over_k_minus_1 *= z_v - z_0;
			}
			force_vector.m_x = sum.real();
			force_vector.m_y = (-1) * sum.imag();
			F_local_exp[*v_ptr] = force_vector;
		}
	}
}


void NMM::transform_multipole_exp_to_forces(
	NodeArray<NodeAttributes>& A,
	List<QuadTreeNodeNM*>& quad_tree_leaves,
	NodeArray<DPoint>& F_multipole_exp)
{
	List<QuadTreeNodeNM*> M;
	List<node> act_contained_nodes;
	ListIterator<node> v_ptr;
	complex<double> sum;
	complex<double> z_0;
	complex<double> z_v_minus_z_0_over_minus_k_minus_1;
	DPoint force_vector;

	//for each leaf u in the M-List of an actual leaf v do:
	//calculate derivative of the multipole expansion function at u
	//and evaluate it for each node in v.get_contained_nodes()
	//and transform the complex number back to the real-world, to obtain the force

	forall_listiterators(QuadTreeNodeNM*, act_leaf_ptr_ptr,quad_tree_leaves)
	{
		(*act_leaf_ptr_ptr)->get_contained_nodes(act_contained_nodes);
		(*act_leaf_ptr_ptr)->get_M(M);
		forall_listiterators(QuadTreeNodeNM*, M_node_ptr_ptr,M)
		{
			z_0 = (*M_node_ptr_ptr)->get_Sm_center();
			forall_listiterators(node, v_ptr,act_contained_nodes)
			{
				complex<double> z_v (A[*v_ptr].get_x(),A[*v_ptr].get_y());
				z_v_minus_z_0_over_minus_k_minus_1 = 1.0/(z_v-z_0);
				sum = (*M_node_ptr_ptr)->get_multipole_exp()[0]*
					z_v_minus_z_0_over_minus_k_minus_1;

				for(int k=1; k<=precision(); k++)
				{
					z_v_minus_z_0_over_minus_k_minus_1 /= z_v - z_0;
					sum -= double(k) * (*M_node_ptr_ptr)->get_multipole_exp()[k] *
						z_v_minus_z_0_over_minus_k_minus_1;
				}
				force_vector.m_x = sum.real();
				force_vector.m_y = (-1) * sum.imag();
				F_multipole_exp[*v_ptr] =  F_multipole_exp[*v_ptr] + force_vector;

			}
		}
	}
}


void NMM::calculate_neighbourcell_forces(
	NodeArray<NodeAttributes>& A,
	List <QuadTreeNodeNM*>& quad_tree_leaves,
	NodeArray<DPoint>& F_direct)
{
	numexcept N;
	List<node> act_contained_nodes,neighbour_contained_nodes,non_neighbour_contained_nodes;
	List<QuadTreeNodeNM*> neighboured_leaves;
	List<QuadTreeNodeNM*> non_neighboured_leaves;
	double act_leaf_boxlength,neighbour_leaf_boxlength;
	DPoint act_leaf_dlc,neighbour_leaf_dlc;
	DPoint f_rep_u_on_v;
	DPoint vector_v_minus_u;
	DPoint nullpoint(0,0);
	DPoint pos_u,pos_v;
	double norm_v_minus_u,scalar;
	int length;
	node u,v;

	forall_listiterators(QuadTreeNodeNM*, act_leaf_ptr,quad_tree_leaves)
	{//forall
		(*act_leaf_ptr)->get_contained_nodes(act_contained_nodes);

		if(act_contained_nodes.size() <= particles_in_leaves())
		{//if (usual case)

			//Step1:calculate forces inside act_contained_nodes

			length = act_contained_nodes.size();
			Array<node> numbered_nodes (length+1);
			int k = 1;
			forall_listiterators(node, v_ptr,act_contained_nodes)
			{
				numbered_nodes[k]= *v_ptr;
				k++;
			}

			for(k = 1; k<length; k++)
				for(int l = k+1; l<=length; l++)
				{
					u = numbered_nodes[k];
					v = numbered_nodes[l];
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
					F_direct[v] = F_direct[v] + f_rep_u_on_v;
					F_direct[u] = F_direct[u] - f_rep_u_on_v;
				}

				//Step 2: calculated forces to nodes in act_contained_nodes() of
				//leaf_ptr->get_D1()

				(*act_leaf_ptr)->get_D1(neighboured_leaves);
				act_leaf_boxlength = (*act_leaf_ptr)->get_Sm_boxlength();
				act_leaf_dlc = (*act_leaf_ptr)->get_Sm_downleftcorner();

				forall_listiterators(QuadTreeNodeNM*, neighbour_leaf_ptr,neighboured_leaves)
				{//forall2
					//forget boxes that have already been looked at

					neighbour_leaf_boxlength = (*neighbour_leaf_ptr)->get_Sm_boxlength();
					neighbour_leaf_dlc = (*neighbour_leaf_ptr)->get_Sm_downleftcorner();

					if( (act_leaf_boxlength > neighbour_leaf_boxlength) ||
						(act_leaf_boxlength == neighbour_leaf_boxlength &&
						act_leaf_dlc.m_x < neighbour_leaf_dlc.m_x)
						|| (act_leaf_boxlength == neighbour_leaf_boxlength &&
						act_leaf_dlc.m_x ==  neighbour_leaf_dlc.m_x &&
						act_leaf_dlc.m_y < neighbour_leaf_dlc.m_y) )
					{//if
						(*neighbour_leaf_ptr)->get_contained_nodes(neighbour_contained_nodes);
						forall_listiterators(node, v_ptr,act_contained_nodes)
							forall_listiterators(node, u_ptr, neighbour_contained_nodes)
						{//for
							pos_u = A[*u_ptr].get_position();
							pos_v = A[*v_ptr].get_position();
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
							F_direct[*v_ptr] = F_direct[*v_ptr] + f_rep_u_on_v;
							F_direct[*u_ptr] = F_direct[*u_ptr] - f_rep_u_on_v;
						}//for
					}//if
				}//forall2

				//Step 3: calculated forces to nodes in act_contained_nodes() of
				//leaf_ptr->get_D2()

				(*act_leaf_ptr)->get_D2(non_neighboured_leaves);
				forall_listiterators(QuadTreeNodeNM*, non_neighbour_leaf_ptr,
					non_neighboured_leaves)
				{//forall3
					(*non_neighbour_leaf_ptr)->get_contained_nodes(
						non_neighbour_contained_nodes);
					forall_listiterators(node,v_ptr,act_contained_nodes)
						forall_listiterators(node, u_ptr,non_neighbour_contained_nodes)
					{//for
						pos_u = A[*u_ptr].get_position();
						pos_v = A[*v_ptr].get_position();
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
						F_direct[*v_ptr] = F_direct[*v_ptr] + f_rep_u_on_v;
					}//for
				}//forall3
		}//if(usual case)
		else //special case (more then particles_in_leaves() particles in this leaf)
		{//else
			forall_listiterators(node, v_ptr, act_contained_nodes)
			{
				pos_v = A[*v_ptr].get_position();
				pos_u = N.choose_distinct_random_point_in_radius_epsilon(pos_v);
				vector_v_minus_u = pos_v - pos_u;
				norm_v_minus_u = vector_v_minus_u.norm();
				if(!N.f_rep_near_machine_precision(norm_v_minus_u,f_rep_u_on_v))
				{
					scalar = f_rep_scalar(norm_v_minus_u)/norm_v_minus_u ;
					f_rep_u_on_v.m_x = scalar * vector_v_minus_u.m_x;
					f_rep_u_on_v.m_y = scalar * vector_v_minus_u.m_y;
				}
				F_direct[*v_ptr] =  F_direct[*v_ptr] + f_rep_u_on_v;
			}
		}//else
	}//forall
}


inline void NMM::add_rep_forces(
	const Graph& G,
	NodeArray<DPoint>& F_direct,
	NodeArray<DPoint>& F_multipole_exp,
	NodeArray<DPoint>& F_local_exp,
	NodeArray<DPoint>& F_rep)
{
	node v;
	forall_nodes(v,G)
	{
		F_rep[v] = F_direct[v]+F_local_exp[v]+F_multipole_exp[v];
	}
}


inline double NMM::f_rep_scalar(double d)
{
	if (d > 0)
	{
		return 1/d;
	}
	else
	{
		cout<<"Error NMM:: f_rep_scalar nodes at same position"<<endl;
		return 0;
	}
}


void NMM::init_binko(int t)
{
	typedef double*  double_ptr;

	BK = new double_ptr[t+1];

	for(int i = 0; i<= t ; i++)
	{//for
		BK[i] = new double[i+1];
	}//for

	//Pascal's triangle

	for (int i = 0; i <= t; i++)
		BK[i][0] = BK[i][i] = 1;

	for (int i = 2; i <= t; i ++)
		for (int j = 1; j < i; j++)
		{
			BK[i][j] = BK[i-1][j-1]+BK[i-1][j];
		}
}


inline void NMM::free_binko()
{
	for(int i = 0;i<= 2*precision();i++)
		delete [] BK[i];
	delete [] BK;
}


inline double NMM::binko(int n, int k)
{
	return BK[n][k];
}

}//namespace ogdf
