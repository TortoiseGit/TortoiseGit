/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Definition of class ComplexDouble for fast complex number arithmetic.
 *
 * \author Martin Gronemann
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

#ifndef OGDF_COMPLEX_DOUBLE_H
#define OGDF_COMPLEX_DOUBLE_H

#include "FastUtils.h"
#include <math.h>

namespace ogdf {
namespace sse {

//! Class to generate instrinsics for complex number arithmetic functions
#ifdef OGDF_FME_KERNEL_USE_SSE
class ComplexDouble
{
public:
	__m128d reg;

	// ---------------------------------------------------
	//	CONSTRUCTORS
	// ---------------------------------------------------
	inline ComplexDouble()
	{
		reg = _mm_setzero_pd();
	}

	inline ComplexDouble(const ComplexDouble& other)
	{
		reg = other.reg;
	}

	inline ComplexDouble(double x)
	{
		reg = _mm_setr_pd((x), (0));
	}

	inline ComplexDouble(double x, double y)
	{
		reg = _mm_setr_pd((x), (y));
	}

	inline ComplexDouble(const double* ptr)
	{
		reg = _mm_load_pd(ptr);
	}


	inline ComplexDouble(__m128d r) : reg(r)
	{
	}

	inline ComplexDouble(float x, float y)
	{
		reg =  _mm_cvtps_pd(_mm_setr_ps((x), (y), 0, 0));
	}

	// ---------------------------------------------------
	//	Standard arithmetic
	// ---------------------------------------------------
	inline ComplexDouble operator+(const ComplexDouble& other) const
	{
		return ComplexDouble( _mm_add_pd(reg, other.reg) );
	}

	inline ComplexDouble operator-(const ComplexDouble& other) const
	{
		return ComplexDouble( _mm_sub_pd(reg, other.reg) );
	}

	inline ComplexDouble operator-(void) const
	{
		return ComplexDouble( _mm_sub_pd(_mm_setzero_pd(), reg) );
	}

	inline ComplexDouble operator*(const ComplexDouble& other) const
	{
		// ---------------------------------
		// | a0*b0 - a1*b1 | a0*b1 + a1*b0 |
		// ---------------------------------
		// bt = | b1 | b0 |
		__m128d b_t = _mm_shuffle_pd(other.reg, other.reg, _MM_SHUFFLE2(0, 1));
		// left = | a0*b0 | a1*b1 |
		__m128d left = _mm_mul_pd(reg, other.reg);
		// right = | a0*b1 | a1*b0 |
		__m128d right = _mm_mul_pd(reg, b_t);
		// left = | a0*b0 | -a1*b1 |
		left = _mm_mul_pd(left, _mm_setr_pd(1.0,  -1.0) ) ;
		// left = | a0*b0 + (-a1*b1) | a0*b1 + a1*b0 |
		return ComplexDouble( _mm_hadd_pd ( left, right ) );
	}

	inline ComplexDouble operator/(const ComplexDouble& other) const
	{
		// 1/(length(other)^2 * this * other.conj;
		// bt = | b0 | -b1 |
		__m128d conj_reg = _mm_mul_pd(other.reg, _mm_setr_pd(1.0, -1.0) ) ;
		// bt = | b1 | b0 |
		__m128d b_t = _mm_shuffle_pd(conj_reg, conj_reg, _MM_SHUFFLE2(0, 1));
		// left = | a0*b0 | a1*b1 |
		__m128d left = _mm_mul_pd(reg, conj_reg);
		// right = | a0*b1 | a1*b0 |
		__m128d right = _mm_mul_pd(reg, b_t);
		// left = | a0*b0 | -a1*b1 |
		left = _mm_mul_pd(left, _mm_setr_pd(1.0, -1.0) ) ;
		// left = | a0*b0 + (-a1*b1) | a0*b1 + a1*b0 |
		__m128d product = _mm_hadd_pd ( left, right ) ;
		// product = reg*other.reg.conj
		// l = b0*b0 | b1*b1
		__m128d l = _mm_mul_pd(conj_reg, conj_reg );
		// l = b0*b0 + b1*b1 | b0*b0 + b1*b1
		l = _mm_hadd_pd(l, l);
		// l = length^2 | length^2
		return ComplexDouble( _mm_div_pd(product, l));
	}

	inline ComplexDouble operator*(double scalar) const
	{
		return ComplexDouble( _mm_mul_pd(reg, _mm_setr_pd(scalar, scalar)) );
	}

	inline ComplexDouble operator/(double scalar) const
	{
		//double rcp = 1.0/scalar;
		return ComplexDouble( _mm_div_pd(reg, _mm_setr_pd(scalar, scalar)) );
	}

	inline ComplexDouble operator*(unsigned int scalar) const
	{
		return ComplexDouble( _mm_mul_pd(reg, _mm_setr_pd((double)scalar, (double)scalar)) );
	}

	inline void operator+=(const ComplexDouble& other)
	{
		reg = _mm_add_pd(reg, other.reg);
	}

	inline void operator-=(const ComplexDouble& other)
	{
		reg = _mm_sub_pd(reg, other.reg);
	}

	inline void operator*=(const ComplexDouble& other)
	{
		// bt = | b1 | b0 |
		__m128d b_t = _mm_shuffle_pd(other.reg, other.reg, _MM_SHUFFLE2(0, 1));
		// left = | a0*b0 | a1*b1 |
		__m128d left = _mm_mul_pd(reg, other.reg);
		// right = | a0*b1 | a1*b0 |
		__m128d right = _mm_mul_pd(reg, b_t);
		// left = | a0*b0 | -a1*b1 |
		left = _mm_mul_pd(left, _mm_setr_pd(1.0, -1.0) ) ;
		// left = | a0*b0 + (-a1*b1) | a0*b1 + a1*b0 |
		reg = _mm_hadd_pd ( left, right ) ;
	}

	inline void operator*=(double scalar)
	{
		// (real*scalar, imag*scalar)
		reg = _mm_mul_pd(reg, _mm_setr_pd(scalar, scalar));
	}

	inline void operator/=(const ComplexDouble& other)
	{
		// 1/(length(other)^2 * this * other.conj;
		// bt = | b0 | -b1 |
		__m128d conj_reg = _mm_mul_pd(other.reg, _mm_setr_pd(1.0, -1.0) ) ;
		// bt = | b1 | b0 |
		__m128d b_t = _mm_shuffle_pd(conj_reg, conj_reg, _MM_SHUFFLE2(0, 1));
		// left = | a0*b0 | a1*b1 |
		__m128d left = _mm_mul_pd(reg, conj_reg);
		// right = | a0*b1 | a1*b0 |
		__m128d right = _mm_mul_pd(reg, b_t);
		// left = | a0*b0 | -a1*b1 |
		left = _mm_mul_pd(left, _mm_setr_pd(1.0, -1.0) ) ;
		// left = | a0*b0 + (-a1*b1) | a0*b1 + a1*b0 |
		__m128d product = _mm_hadd_pd ( left, right ) ;
		// l = b0*b0 | b1*b1
		__m128d l = _mm_mul_pd(conj_reg, conj_reg );
		// l = b0*b0 + b1*b1 | b0*b0 + b1*b1
		l = _mm_hadd_pd(l, l);
		// l = length^2 | length^2
		reg = _mm_div_pd(product, l);
	}

	// ---------------------------------------------------
	//	Additional arithmetic
	// ---------------------------------------------------
	inline double length() const
	{
		// sqrt(real*real + imag*imag)
		double res;
		__m128d r = _mm_mul_pd(reg, reg );
		r = _mm_hadd_pd(r, _mm_setzero_pd());
		r = _mm_sqrt_sd(r, r);
		_mm_store_sd(&res, r);
		return res;
	}

	inline ComplexDouble conj() const
	{
		// (real, -imag)
		return ComplexDouble( _mm_mul_pd(reg, _mm_setr_pd(1.0, -1.0) ) );
	}

	// ---------------------------------------------------
	//	Assignment
	// ---------------------------------------------------
	inline void operator=(const ComplexDouble& other)
	{
		reg = other.reg;
	}

	//! load from 16byte aligned ptr
	inline void operator=(double* ptr)
	{
		reg = _mm_load_pd(ptr);
	}


	// ---------------------------------------------------
	//	LOAD, STORE
	// ---------------------------------------------------

	//! load from 16byte aligned ptr
	inline void load(const double* ptr)
	{
		reg = _mm_load_pd(ptr);
	}

	//! load from unaligned ptr
	inline void load_unaligned(const double* ptr)
	{
		reg = _mm_loadu_pd(ptr);
	}

	//! store to 16byte aligned ptr
	inline void store(double* ptr) const
	{
		_mm_store_pd(ptr, reg);
	}

	//! store to unaligned ptr
	inline void store_unaligned(double* ptr) const
	{
		_mm_storeu_pd(ptr, reg);
	}
};

#else
class ComplexDouble
{
public:
	double reg[2];

	// ---------------------------------------------------
	//	CONSTRUCTORS
	// ---------------------------------------------------
	inline ComplexDouble( )
	{
		reg[0] = 0.0;
		reg[1] = 0.0;
	}

	inline ComplexDouble(const ComplexDouble& other)
	{
		reg[0] = other.reg[0];
		reg[1] = other.reg[1];
	}

	inline ComplexDouble(double x)
	{
		reg[0] = x;
		reg[1] = 0;
	}

	inline ComplexDouble(double x, double y)
	{
		reg[0] = x;
		reg[1] = y;
	}

	inline ComplexDouble(double* ptr)
	{
		reg[0] = ptr[0];
		reg[1] = ptr[1];
	}

	// ---------------------------------------------------
	//	Standard arithmetic
	// ---------------------------------------------------
	inline ComplexDouble operator+(const ComplexDouble& other) const
	{
		return ComplexDouble( reg[0] + other.reg[0], reg[1] + other.reg[1] );
	}

	inline ComplexDouble operator-(const ComplexDouble& other) const
	{
		return ComplexDouble( reg[0] - other.reg[0], reg[1] - other.reg[1] );
	}

	inline ComplexDouble operator-(void) const
	{
		return ComplexDouble( -reg[0] , -reg[1] );
	}

	inline ComplexDouble operator*(const ComplexDouble& other) const
	{
		return ComplexDouble( reg[0]*other.reg[0] - reg[1]*other.reg[1], reg[0]*other.reg[1] + reg[1]*other.reg[0] );
	}

	inline ComplexDouble operator/(const ComplexDouble& other) const
	{
		return ((*this) *other.conj() / (other.reg[0]*other.reg[0] + other.reg[1]*other.reg[1]));
	}

	inline ComplexDouble operator*(double scalar) const
	{
		return ComplexDouble( reg[0]*scalar, reg[1]*scalar );
	}

	inline ComplexDouble operator/(double scalar) const
	{
		return ComplexDouble( reg[0]/scalar, reg[1]/scalar );
	}

	inline ComplexDouble operator*(unsigned int scalar) const
	{
		return ComplexDouble( reg[0]*(double)scalar, reg[1]*(double)scalar );
	}

	inline void operator+=(const ComplexDouble& other)
	{
		reg[0] += other.reg[0];
		reg[1] += other.reg[1];
	}

	inline void operator-=(const ComplexDouble& other)
	{
		reg[0] -= other.reg[0];
		reg[1] -= other.reg[1];
	}

	inline void operator*=(const ComplexDouble& other)
	{
		double t[2];
		t[0] = reg[0]*other.reg[0] - reg[1]*other.reg[1];
		t[1] = reg[0]*other.reg[1] + reg[1]*other.reg[0];
		reg[0] = t[0];
		reg[1] = t[1];
	}

	inline void operator*=(double scalar)
	{
		reg[0] *= scalar;
		reg[1] *= scalar;
	}

	inline void operator/=(const ComplexDouble& other)
	{
		ComplexDouble t = other.conj() / (other.reg[0]*other.reg[0] + other.reg[1]*other.reg[1]);
		double r[2];
		r[0] = reg[0]*t.reg[0] - reg[1]*t.reg[1];
		r[1] = reg[0]*t.reg[1] + reg[1]*t.reg[0];
		reg[0] = r[0];
		reg[1] = r[1];
	}

	// ---------------------------------------------------
	//	Additional arithmetic
	// ---------------------------------------------------
	inline double length() const
	{
		// sqrt(real*real + imag*imag)
		return sqrt(reg[0]*reg[0] + reg[1]*reg[1]);
	}

	inline ComplexDouble conj() const
	{
		// (real, -imag)
		return ComplexDouble( reg[0], -reg[1] );
	}


	// ---------------------------------------------------
	//	Assignment
	// ---------------------------------------------------
	inline void operator=(const ComplexDouble& other)
	{
		reg[0] = other.reg[0];
		reg[1] = other.reg[1];
	}

	//! load from 16byte aligned ptr
	inline void operator=(double* ptr)
	{
		reg[0] = ptr[0];
		reg[1] = ptr[1];
	}

	// ---------------------------------------------------
	//	LOAD, STORE
	// ---------------------------------------------------

	//! load from 16byte aligned ptr
	inline void load(const double* ptr)
	{
		reg[0] = ptr[0];
		reg[1] = ptr[1];
	}

	//! load from unaligned ptr
	inline void load_unaligned(const double* ptr)
	{
		reg[0] = ptr[0];
		reg[1] = ptr[1];
	}

	//! store to 16byte aligned ptr
	inline void store(double* ptr) const
	{
		ptr[0] = reg[0];
		ptr[1] = reg[1];
	}

	//! store to unaligned ptr
	inline void store_unaligned(double* ptr) const
	{
		ptr[0] = reg[0];
		ptr[1] = reg[1];
	}
};

#endif
};

} // end of namespace ogdf::sse

#endif // _COMPLEX_DOUBLE_H_

