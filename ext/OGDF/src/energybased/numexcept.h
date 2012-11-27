/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class numexcept (handling of numeric problems).
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

#ifndef OGDF_NUMEXCEPT_H
#define OGDF_NUMEXCEPT_H

#include <ogdf/basic/geometry.h>

namespace ogdf {

//--------------------------------------------------------------------------
// this class is developed for exceptions that might occure, when nodes are
// placed at the same position and a new random position has to be found, or
// when the calculated forces are near the machine accuracy, where no
// reasonable numeric and logic calculations are possible any more
//---------------------------------------------------------------------------

	class numexcept
	{
	public:

		//Returns a distinct random point within the smallest disque D with center
		//old_point that is contained in the box defined by xmin,...,ymax; The size of
		//D is shrunk by multiplying with epsilon = 0.1; Precondition:
		//old_point is contained in the box and the box is not equal to old_point.
		DPoint choose_distinct_random_point_in_disque(
			DPoint old_point,
			double xmin,
			double xmax,
			double ymin,
			double ymax);

		//A random point (distinct from old_pos) on the disque around old_pos with
		//radius epsilon = 0.1 is computed.
		DPoint choose_distinct_random_point_in_radius_epsilon(DPoint old_pos);

		//If distance has a value near the machine precision the repulsive force calculation
		//is not possible (calculated values exceed the machine accuracy) in this cases
		//true is returned and force is set to a reasonable value that does
		//not cause problems; Else false is returned and force keeps unchanged.
		bool f_rep_near_machine_precision(double distance, DPoint& force);

		//If distance has a value near the machine precision the (attractive)force
		//calculation is not possible (calculated values exceed the machine accuracy) in
		//this cases true is returned and force is set to a reasonable value that does
		//not cause problems; Else false is returned and force keeps unchanged.
		bool f_near_machine_precision(double distance, DPoint& force);

		//Returns true if a is "nearly" equal to b (needed, when machine accuracy is
		//insufficient in functions well_seperated and bordering of NMM)
		bool nearly_equal(double a, double b);

	};

}//namespace ogdf
#endif

