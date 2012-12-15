/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class QuadTreeNM.
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


#include <ogdf/internal/energybased/QuadTreeNM.h>

namespace ogdf {

QuadTreeNM::QuadTreeNM()
{
	root_ptr = act_ptr =NULL;
}


void QuadTreeNM::create_new_lt_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lt_ptr(new_ptr);
}


void QuadTreeNM::create_new_lt_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lt_ptr(new_ptr);
}


void QuadTreeNM::create_new_rt_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rt_ptr(new_ptr);
}


void QuadTreeNM::create_new_rt_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y+act_ptr->get_Sm_boxlength()/2;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rt_ptr(new_ptr);
}


void QuadTreeNM::create_new_lb_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lb_ptr(new_ptr);
}


void QuadTreeNM::create_new_lb_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_lb_ptr(new_ptr);
}


void QuadTreeNM::create_new_rb_child(
	List<ParticleInfo>* L_x_ptr,
	List<ParticleInfo>* L_y_ptr)
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_x_List_ptr(L_x_ptr);
	new_ptr->set_y_List_ptr(L_y_ptr);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rb_ptr(new_ptr);
}


void QuadTreeNM::create_new_rb_child()
{
	QuadTreeNodeNM* new_ptr = new QuadTreeNodeNM();

	DPoint old_Sm_dlc = act_ptr->get_Sm_downleftcorner();
	DPoint new_Sm_dlc;
	new_Sm_dlc.m_x = old_Sm_dlc.m_x+act_ptr->get_Sm_boxlength()/2;
	new_Sm_dlc.m_y = old_Sm_dlc.m_y;

	new_ptr->set_Sm_level(act_ptr->get_Sm_level()+1);
	new_ptr->set_Sm_downleftcorner(new_Sm_dlc);
	new_ptr->set_Sm_boxlength((act_ptr->get_Sm_boxlength())/2);
	new_ptr->set_father_ptr(act_ptr);
	act_ptr->set_child_rb_ptr(new_ptr);
}


void QuadTreeNM::delete_tree(QuadTreeNodeNM* node_ptr)
{
	if(node_ptr != NULL)
	{
		if(node_ptr->get_child_lt_ptr() != NULL)
			delete_tree(node_ptr->get_child_lt_ptr());
		if(node_ptr->get_child_rt_ptr() != NULL)
			delete_tree(node_ptr->get_child_rt_ptr());
		if(node_ptr->get_child_lb_ptr() != NULL)
			delete_tree(node_ptr->get_child_lb_ptr());
		if(node_ptr->get_child_rb_ptr() != NULL)
			delete_tree(node_ptr->get_child_rb_ptr());
		delete node_ptr;
		if (node_ptr == root_ptr)
			root_ptr = NULL;
	}
}


void QuadTreeNM::delete_tree_and_count_nodes(QuadTreeNodeNM* node_ptr, int& nodecounter)
{
	if(node_ptr != NULL)
	{
		nodecounter++;
		if(node_ptr->get_child_lt_ptr() != NULL)
			delete_tree_and_count_nodes(node_ptr->get_child_lt_ptr(),nodecounter);
		if(node_ptr->get_child_rt_ptr() != NULL)
			delete_tree_and_count_nodes(node_ptr->get_child_rt_ptr(),nodecounter);
		if(node_ptr->get_child_lb_ptr() != NULL)
			delete_tree_and_count_nodes(node_ptr->get_child_lb_ptr(),nodecounter);
		if(node_ptr->get_child_rb_ptr() != NULL)
			delete_tree_and_count_nodes(node_ptr->get_child_rb_ptr(),nodecounter);
		delete node_ptr;
		if (node_ptr == root_ptr)
			root_ptr = NULL;
	}
}


void QuadTreeNM::cout_preorder(QuadTreeNodeNM* node_ptr)
{
	if(node_ptr != NULL)
	{
		cout<< *node_ptr <<endl;
		if(node_ptr->get_child_lt_ptr() != NULL)
			cout_preorder(node_ptr->get_child_lt_ptr());
		if(node_ptr->get_child_rt_ptr() != NULL)
			cout_preorder(node_ptr->get_child_rt_ptr());
		if(node_ptr->get_child_lb_ptr() != NULL)
			cout_preorder(node_ptr->get_child_lb_ptr());
		if(node_ptr->get_child_rb_ptr() != NULL)
			cout_preorder(node_ptr->get_child_rb_ptr());
	}
}


void QuadTreeNM::cout_preorder(QuadTreeNodeNM* node_ptr, int precision)
{
	int i;
	if(node_ptr != NULL)
	{
		complex<double>* L =node_ptr->get_local_exp();
		complex<double>* M =node_ptr->get_multipole_exp();
		cout<< *node_ptr <<endl;
		cout<<" ME: ";
		for(i = 0; i<= precision;i++)
			cout<<M[i]<<" ";cout<<endl;
		cout<<" LE: ";
		for(i = 0; i<= precision;i++)
			cout<<L[i]<<" ";cout<<endl<<endl;

		if(node_ptr->get_child_lt_ptr() != NULL)
			cout_preorder(node_ptr->get_child_lt_ptr(),precision);
		if(node_ptr->get_child_rt_ptr() != NULL)
			cout_preorder(node_ptr->get_child_rt_ptr(),precision);
		if(node_ptr->get_child_lb_ptr() != NULL)
			cout_preorder(node_ptr->get_child_lb_ptr(),precision);
		if(node_ptr->get_child_rb_ptr() != NULL)
			cout_preorder(node_ptr->get_child_rb_ptr(),precision);
	}
}

}//namespace ogdf
