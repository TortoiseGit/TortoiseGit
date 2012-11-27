/*
 * $Revision: 2549 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 23:09:19 +0200 (Mi, 04. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of mathematical constants, functions.
 *
 * \author Carsten Gutwenger
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


#include <ogdf/basic/Math.h>
#include <limits.h>


namespace ogdf {


	const double Math::pi     = 3.14159265358979323846;
	const double Math::pi_2   = 1.57079632679489661923;
	const double Math::pi_4   = 0.785398163397448309616;
	const double Math::two_pi = 2*3.14159265358979323846;

	const double Math::e    = 2.71828182845904523536;

	const double Math::log_of_2 = log(2.0);
	const double Math::log_of_4 = log(4.0);

	int factorials[13] = {
		1, 1, 2, 6, 24, 120, 720, 5040, 40320, 362880,
		3628800, 39916800, 479001600
	};

	double factorials_d[20] = {
		1.0, 1.0, 2.0, 6.0, 24.0, 120.0, 720.0, 5040.0, 40320.0, 362880.0,
		3628800.0, 39916800.0, 479001600.0, 6227020800.0, 87178291200.0,
		1307674368000.0, 20922789888000.0, 355687428096000.0,
		6402373705728000.0, 121645100408832000.0
	};

	int Math::binomial(int n, int k)
	{
		if(k>n/2) k = n-k;
		if(k == 0) return 1;
		int r = n;
		for(int i = 2; i<=k; ++i)
			r = (r * (n+1-i))/i;
		return r;
	}

	double Math::binomial_d(int n, int k)
	{
		if(k>n/2) k = n-k;
		if(k == 0) return 1.0;
		double r = n;
		for(int i = 2; i<=k; ++i)
			r = (r * (n+1-i))/i;
		return r;
	}

	int Math::factorial(int n)
	{
		if(n < 0) return 1;
		if(n > 12) return INT_MAX; // not representable by int

		return factorials[n];
	}

	double Math::factorial_d(int n)
	{
		if(n < 0) return 1.0;

		double f = 1.0;
		for(; n > 19; --n)
			f *= n;

		return f * factorials_d[n];
	}

} // namespace ogdf
