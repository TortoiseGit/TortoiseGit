/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class numexcept (handling of numeric problems).
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


#include "numexcept.h"
#include <ogdf/basic/basic.h>

#define epsilon 0.1
#define POS_SMALL_DOUBLE 1e-300
#define POS_BIG_DOUBLE   1e+300


namespace ogdf {


DPoint numexcept::choose_distinct_random_point_in_disque(DPoint old_point,
	double xmin,double xmax,double ymin,double ymax)
{
	const int BILLION = 1000000000;
	double mindist;//minimal distance from old_point to the boundaries of the disc
	double mindist_to_xmin,mindist_to_xmax,mindist_to_ymin,mindist_to_ymax;
	double rand_x,rand_y;
	DPoint new_point;

	mindist_to_xmin = old_point.m_x - xmin;
	mindist_to_xmax = xmax -  old_point.m_x;
	mindist_to_ymin = old_point.m_y - ymin;
	mindist_to_ymax = ymax -  old_point.m_y;

	mindist = min(min(mindist_to_xmin,mindist_to_xmax), min(mindist_to_ymin,mindist_to_ymax));

	if(mindist > 0)
	do {
		//assign random double values in range (-1,1)
		rand_x = 2*(double(randomNumber(1,BILLION)+1)/(BILLION+2)-0.5);
		rand_y = 2*(double(randomNumber(1,BILLION)+1)/(BILLION+2)-0.5);
		new_point.m_x = old_point.m_x+mindist*rand_x*epsilon;
		new_point.m_y = old_point.m_y+mindist*rand_y*epsilon;
	} while((old_point == new_point)||((old_point-new_point).norm() >= mindist*epsilon));

	else if(mindist == 0) //old_point lies at the boundaries
	{//else1
		double mindist_x =0;
		double mindist_y =0;

		if (mindist_to_xmin > 0)
			mindist_x = (-1)* mindist_to_xmin;
		else if (mindist_to_xmax > 0)
			mindist_x = mindist_to_xmax;
		if (mindist_to_ymin > 0)
			mindist_y = (-1)* mindist_to_ymin;
		else if (mindist_to_ymax > 0)
			mindist_y = mindist_to_ymax;

		if((mindist_x != 0)||(mindist_y != 0))
		do {
			//assign random double values in range (0,1)
			rand_x = double(randomNumber(1,BILLION)+1)/(BILLION+2);
			rand_y = double(randomNumber(1,BILLION)+1)/(BILLION+2);
			new_point.m_x = old_point.m_x+mindist_x*rand_x*epsilon;
			new_point.m_y = old_point.m_y+mindist_y*rand_y*epsilon;
		} while(old_point == new_point);
	else
		cout<<"Error DIM2:: box is equal to old_pos"<<endl;
	}//else1

	else //mindist < 0
	{//else2
		cout<<"Error DIM2:: choose_distinct_random_point_in_disque: old_point not ";
		cout<<"in box"<<endl;
	}//else2

	return new_point;
}


DPoint numexcept::choose_distinct_random_point_in_radius_epsilon(DPoint old_pos)
{
	double xmin = old_pos.m_x-1*epsilon;
	double xmax = old_pos.m_x+1*epsilon;
	double ymin = old_pos.m_y-1*epsilon;
	double ymax = old_pos.m_y+1*epsilon;

	return choose_distinct_random_point_in_disque(old_pos,xmin,xmax,ymin,ymax);
}


bool numexcept::f_rep_near_machine_precision(double distance,DPoint& force )
{
	const double  POS_BIG_LIMIT =    POS_BIG_DOUBLE   *  1e-190;
	const double  POS_SMALL_LIMIT =  POS_SMALL_DOUBLE *  1e190;
	const int BILLION = 1000000000;

	if(distance > POS_BIG_LIMIT)
	{
		//create random number in range (0,1)
		double randx = double(randomNumber(1,BILLION)+1)/(BILLION+2);
		double randy = double(randomNumber(1,BILLION)+1)/(BILLION+2);
		int rand_sign_x = randomNumber(0,1);
		int rand_sign_y = randomNumber(0,1);
		force.m_x = POS_SMALL_LIMIT*(1+randx)*pow(-1.0,rand_sign_x);
		force.m_y = POS_SMALL_LIMIT*(1+randy)*pow(-1.0,rand_sign_y);
		return true;

	} else if (distance < POS_SMALL_LIMIT)
	{
		//create random number in range (0,1)
		double randx = double(randomNumber(1,BILLION)+1)/(BILLION+2);
		double randy = double(randomNumber(1,BILLION)+1)/(BILLION+2);
		int rand_sign_x = randomNumber(0,1);
		int rand_sign_y = randomNumber(0,1);
		force.m_x = POS_BIG_LIMIT*randx*pow(-1.0,rand_sign_x);
		force.m_y = POS_BIG_LIMIT*randy*pow(-1.0,rand_sign_y);
		return true;

	} else
		return false;
}


bool numexcept::f_near_machine_precision(double distance,DPoint& force )
{
	const double  POS_BIG_LIMIT =    POS_BIG_DOUBLE   *  1e-190;
	const double  POS_SMALL_LIMIT =  POS_SMALL_DOUBLE *  1e190;
	const int BILLION = 1000000000;

	if(distance < POS_SMALL_LIMIT)
	{
		//create random number in range (0,1)
		double randx =  double(randomNumber(1,BILLION)+1)/(BILLION+2);
		double randy =  double(randomNumber(1,BILLION)+1)/(BILLION+2);
		int rand_sign_x = randomNumber(0,1);
		int rand_sign_y = randomNumber(0,1);
		force.m_x = POS_SMALL_LIMIT*(1+randx)*pow(-1.0,rand_sign_x);
		force.m_y = POS_SMALL_LIMIT*(1+randy)*pow(-1.0,rand_sign_y);
		return true;

	} else if (distance > POS_BIG_LIMIT)
	{
		//create random number in range (0,1)
		double randx =  double(randomNumber(1,BILLION)+1)/(BILLION+2);
		double randy =  double(randomNumber(1,BILLION)+1)/(BILLION+2);
		int rand_sign_x = randomNumber(0,1);
		int rand_sign_y = randomNumber(0,1);
		force.m_x = POS_BIG_LIMIT*randx*pow(-1.0,rand_sign_x);
		force.m_x = POS_BIG_LIMIT*randy*pow(-1.0,rand_sign_y);
		return true;

	} else
		return false;
}


bool numexcept::nearly_equal(double a,double b)
{
	double delta = 1e-10;
	double small_b,big_b;

	if(b > 0) {
		small_b = b*(1-delta);
		big_b   = b*(1+delta);

	} else //b <= 0
	{
		small_b = b*(1+delta);
		big_b   = b*(1-delta);
	}

	if((small_b <= a) && (a <= big_b))
		return true;
	else
		return false;
}

}//namespace ogdf
