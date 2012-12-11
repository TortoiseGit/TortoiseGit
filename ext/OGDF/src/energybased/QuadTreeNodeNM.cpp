/*
 * $Revision: 2555 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 12:12:10 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class QuadTreeNodeNM.
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


#include <ogdf/internal/energybased/QuadTreeNodeNM.h>


namespace ogdf {

ostream &operator<< (ostream & output, const QuadTreeNodeNM & A)
{
	output <<" Sm_level: "<<A.Sm_level<<" Sm_downleftcorner: "<<A.Sm_downleftcorner
		<<" Sm boxlength: "<<A.Sm_boxlength<<" Sm_center: "<<A.Sm_center
		<<"spnumber: "<<A.subtreeparticlenumber;
	if(A.father_ptr == NULL)
		output <<" is root ";
	if((A.child_lt_ptr == NULL) ||(A.child_rt_ptr == NULL) || (A.child_lb_ptr == NULL)||
		(A.child_rb_ptr == NULL))
	{
		output <<" (no child in ";
		if(A.child_lt_ptr == NULL)
			output <<" lt";
		if(A.child_rt_ptr == NULL)
			output <<" rt";
		if(A.child_lb_ptr == NULL)
			output <<" lb";
		if(A.child_rb_ptr == NULL)
			output <<" rb";
		output<<" quad) ";
	}

	output<<" L_x: ";
	if(A.L_x_ptr == NULL)
		output<<"no list specified";
	else if(A.L_x_ptr->empty())
		output <<"is empty";
	else
	{
		forall_listiterators(ParticleInfo, it,*A.L_x_ptr)
			output<<"  "<<*it;
	}

	output<<" L_y: ";
	if(A.L_y_ptr == NULL)
		output<<"no list specified";
	else if(A.L_y_ptr->empty())
		output <<"is empty";
	else
	{
		forall_listiterators(ParticleInfo, it,*A.L_y_ptr)
			output<<"  "<<*it;
	}

	output<<" I: ";
	if(A.I.empty())
		output <<"is empty";
	else
	{
		forall_listiterators(QuadTreeNodeNM*, v_ptr,A.I)
			output<<" ["<<(*v_ptr)->get_Sm_level()<<" , "
			<<(*v_ptr)->get_Sm_downleftcorner()<<","
			<<(*v_ptr)->get_Sm_boxlength()<<"]";
	}

	output<<" D1: ";
	if(A.D1.empty())
		output <<"is empty";
	else
	{
		forall_listiterators(QuadTreeNodeNM*, v_ptr,A.D1)
			output<<" ["<<(*v_ptr)->get_Sm_level()<<" , "
			<<(*v_ptr)->get_Sm_downleftcorner()<<","
			<<(*v_ptr)->get_Sm_boxlength()<<"]";
	}

	output<<" D2: ";
	if(A.D2.empty())
		output <<"is empty";
	else
	{
		forall_listiterators(QuadTreeNodeNM*, v_ptr,A.D2)
			output<<" ["<<(*v_ptr)->get_Sm_level()<<" , "
			<<(*v_ptr)->get_Sm_downleftcorner()<<","
			<<(*v_ptr)->get_Sm_boxlength()<<"]";
	}

	output<<" M: ";
	if(A.M.empty())
		output <<"is empty";
	else
	{
		forall_listiterators(QuadTreeNodeNM*, v_ptr,A.M)
			output<<" ["<<(*v_ptr)->get_Sm_level()<<" , "
			<<(*v_ptr)->get_Sm_downleftcorner()<<","
			<<(*v_ptr)->get_Sm_boxlength()<<"]";
	}
	output<<" contained_nodes ";
	if(A.contained_nodes.empty())
		output <<"is empty";
	else
	{
		forall_listiterators(node,v_it,A.contained_nodes)
			output<<(*v_it)->index()<<" ";
	}
	return output;
}


istream &operator>> (istream & input,  QuadTreeNodeNM & A)
{
	input >> A.Sm_level;
	return input;
}


QuadTreeNodeNM::QuadTreeNodeNM()
{
	DPoint double_null(0,0);
	complex<double> comp_null(0,0);

	L_x_ptr = NULL; ;L_y_ptr = NULL;
	subtreeparticlenumber = 0;
	Sm_level = 0;
	Sm_downleftcorner = double_null;
	Sm_boxlength = 0;
	Sm_center = comp_null;
	ME = NULL;
	LE = NULL;
	contained_nodes.clear();
	I.clear();D1.clear();D2.clear();M.clear();
	father_ptr = NULL;
	child_lt_ptr = child_rt_ptr = child_lb_ptr = child_rb_ptr = NULL;
}


QuadTreeNodeNM::~QuadTreeNodeNM()
{
	if(L_x_ptr != NULL)
	{
		delete L_x_ptr;
		L_x_ptr = NULL;
	}
	if(L_y_ptr != NULL)
	{
		delete L_y_ptr;
		L_y_ptr = NULL;
	}
	contained_nodes.clear();
	I.clear();D1.clear();D2.clear();M.clear();
	delete [] ME;
	delete [] LE;
}

}//namespace ogdf
