/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of FME kernel.
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

#include "FMEKernel.h"
#include "ComplexDouble.h"

using namespace ogdf::sse;

namespace ogdf {

#ifdef OGDF_FME_KERNEL_USE_SSE_DIRECT

inline void eval_direct_aligned_SSE(
	float* ptr_x1, float* ptr_y1, float* ptr_s1, float* ptr_fx1, float* ptr_fy1, size_t n1,
	float* ptr_x2, float* ptr_y2, float* ptr_s2, float* ptr_fx2, float* ptr_fy2, size_t n2)
{
	for (__uint32 i=0; i < n1; i+=4)
	{
		// register for i
		__m128 x      = _mm_load_ps( ptr_x1 + i );
		__m128 y      = _mm_load_ps( ptr_y1 + i );
		__m128 s	  = _mm_load_ps( ptr_s1 + i );
		__m128 fx_sum = _mm_load_ps( ptr_fx1+ i );
		__m128 fy_sum = _mm_load_ps( ptr_fy1+ i );

		__m128 x_shuffled;
		__m128 y_shuffled;
		__m128 s_shuffled;
		__m128 fx_sum_shuffled;
		__m128 fy_sum_shuffled;

		for (__uint32 j=i+4; j < n2; j+=4)
		{
			x_shuffled		= _mm_load_ps( ptr_x2 +j );
			y_shuffled		= _mm_load_ps( ptr_y2 +j );
			s_shuffled		= _mm_load_ps( ptr_s2 +j );
			fx_sum_shuffled	= _mm_load_ps( ptr_fx2+j );
			fy_sum_shuffled	= _mm_load_ps( ptr_fy2+j );

			for (int k=0;k<4;k++)
			{
				__m128 dx = _mm_sub_ps(x, x_shuffled);
				__m128 dy = _mm_sub_ps(y, y_shuffled);
				__m128 s_sum = _mm_add_ps(s, s_shuffled);
				__m128 f  = _MM_COMPUTE_FORCE(dx,dy,s_sum);
//				__m128 dsq= _mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy));
//				__m128 f  = _mm_div_ps(_mm_add_ps(s, s_shuffled), dsq);
				__m128 fx = _mm_mul_ps(dx, f);
				__m128 fy = _mm_mul_ps(dy, f);
				// add forces to i's sum
				fx_sum = _mm_add_ps(fx_sum, fx);
				fy_sum = _mm_add_ps(fy_sum, fy);
				// sub forces from j's sum
				fx_sum_shuffled = _mm_sub_ps(fx_sum_shuffled, fx);
				fy_sum_shuffled = _mm_sub_ps(fy_sum_shuffled, fy);
				// shuffle everything for the next pairing
				x_shuffled		= _mm_shuffle_ps(x_shuffled,  x_shuffled,  _MM_SHUFFLE(0,3,2,1));
				y_shuffled		= _mm_shuffle_ps(y_shuffled,  y_shuffled,  _MM_SHUFFLE(0,3,2,1));
				s_shuffled		= _mm_shuffle_ps(s_shuffled,  s_shuffled,  _MM_SHUFFLE(0,3,2,1));
				fx_sum_shuffled = _mm_shuffle_ps(fx_sum_shuffled, fx_sum_shuffled, _MM_SHUFFLE(0,3,2,1));
				fy_sum_shuffled = _mm_shuffle_ps(fy_sum_shuffled, fy_sum_shuffled, _MM_SHUFFLE(0,3,2,1));
			}
			// note: after shuffling 4 times we are back to original order
			_mm_store_ps(ptr_fx2 + j, fx_sum_shuffled);
			_mm_store_ps(ptr_fy2 + j, fy_sum_shuffled);
		}
		_mm_store_ps( ptr_fx1 + i, fx_sum );
		_mm_store_ps( ptr_fy1 + i, fy_sum );
	}
}

//! kernel function to evaluate forces between 4 points with coords x, y (16 byte aligned) directly. result is stored in fx, fy
inline void eval_direct_aligned_SSE(float* ptr_x, float* ptr_y, float* ptr_s, float* ptr_fx, float* ptr_fy)
{
/*	// SSE 2 implementation
	// A = (a, b, c, d) (original order)
	__m128 x_A  = _mm_load_ps( ptr_x );
	__m128 y_A  = _mm_load_ps( ptr_y );
	__m128 s_A  = _mm_load_ps( ptr_s );
	__m128 fx_A = _mm_load_ps( ptr_fx );
	__m128 fy_A = _mm_load_ps( ptr_fy );

	// B = (b, c, d, a) (1x left rotate)
	__m128 x_B = _mm_shuffle_ps( x_A, x_A, _MM_SHUFFLE(0,3,2,1) );
	__m128 y_B = _mm_shuffle_ps( y_A, y_A, _MM_SHUFFLE(0,3,2,1) );
	__m128 s_B = _mm_shuffle_ps( s_A, s_A, _MM_SHUFFLE(0,3,2,1) );

	// C = (c, d, a, b) (2x left rotate)
	__m128 x_C = _mm_shuffle_ps( x_B, x_B, _MM_SHUFFLE(0,3,2,1) );
	__m128 y_C = _mm_shuffle_ps( y_B, y_B, _MM_SHUFFLE(0,3,2,1) );
	__m128 s_C = _mm_shuffle_ps( s_B, s_B, _MM_SHUFFLE(0,3,2,1) );

	// A x B = ( f(a,b), f(b,c), f(c,d), f(d,a) )
	__m128 x_AB = _mm_sub_ps( x_A, x_B ); // dx = Ax - Bx
	__m128 y_AB = _mm_sub_ps( y_A, y_B ); // dy = Ay - By
	__m128 s_AB = _mm_add_ps( s_A, s_B ); // s = As + Bs
	__m128 dsq_AB = _mm_add_ps( _mm_mul_ps( x_AB, x_AB ), _mm_mul_ps( y_AB, y_AB ) ); // f = s/d^2
	__m128 f_AB =  _mm_div_ps( s_AB, dsq_AB );
	__m128 fx_AB = _mm_mul_ps( x_AB, f_AB ); 	// fx(a,b), fx(b,c), fx(c,d), fx(d,a)
	__m128 fy_AB = _mm_mul_ps( y_AB, f_AB );	// fy(a,b), fy(b,c), fy(c,d), fy(d,a)
	// unshuffle
	__m128 fx_AB_shuffled = _mm_shuffle_ps( fx_AB, fx_AB, _MM_SHUFFLE(2,1,0,3) ); // fx(d,a), fx(a,b), fx(b,c), fx(c,d)
	__m128 fy_AB_shuffled = _mm_shuffle_ps( fy_AB, fy_AB, _MM_SHUFFLE(2,1,0,3) ); // fy(d,a), fy(a,b), fy(b,c), fy(c,d)

	// A x C = ( f(a,c), f(b,d), f(c,a), f(d,b) )
	__m128 x_AC = _mm_sub_ps( x_A, x_C ); // dx = Ax - Cx
	__m128 y_AC = _mm_sub_ps( y_A, y_C ); // dy = Ay - Cy
	__m128 s_AC = _mm_add_ps( s_A, s_C ); // s = As + Cs
	__m128 dsq_AC = _mm_add_ps( _mm_mul_ps( x_AC, x_AC ), _mm_mul_ps(y_AC, y_AC)); // f = s/d^2
	__m128 f_AC  = _mm_div_ps( s_AC, dsq_AC );
	__m128 fx_AC = _mm_mul_ps( x_AC, f_AC ); // fx(a,c), fx(b,d), fx(c,a), fx(d,b)
	__m128 fy_AC = _mm_mul_ps( y_AC, f_AC ); // fy(a,c), fy(b,d), fy(c,a), fy(d,b)

	// Note: f(x,y) = -f(y,x)
	// f(a) = f(a,b) - f(d,a) + f(a,c) = f(a,b) + f(a,d) + f(a,c)
	// f(b) = f(b,c) - f(a,b) + f(b,d) = f(b,c) + f(b,a) + f(b,d)
	// f(c) = f(c,d) - f(b,c) + f(c,a) = f(c,d) + f(c,b) + f(c,a)
	// f(d) = f(d,a) - f(c,d) + f(d,b) = f(d,a) + f(d,c) + f(d,b)
	fx_A = _mm_add_ps(fx_A, fx_AB);
	fy_A = _mm_add_ps(fy_A, fy_AB);

	fx_A = _mm_sub_ps(fx_A, fx_AB_shuffled);
	fy_A = _mm_sub_ps(fy_A, fy_AB_shuffled);

	fx_A = _mm_add_ps(fx_A, fx_AC);
	fy_A = _mm_add_ps(fy_A, fy_AC);

	_mm_store_ps(ptr_fx, fx_A);
	_mm_store_ps(ptr_fy, fy_A);
*/
	// SSE 2 implementation (compressed)
	// A = (a, b, c, d) (original order)
	__m128 x      = _mm_load_ps( ptr_x );
	__m128 y      = _mm_load_ps( ptr_y );
	__m128 s	  = _mm_load_ps( ptr_s );
	__m128 fx_sum = _mm_load_ps( ptr_fx );
	__m128 fy_sum = _mm_load_ps( ptr_fy );

	// B = (b, c, d, a) (1x left rotate)
	__m128 x_shuffled = _mm_shuffle_ps( x, x, _MM_SHUFFLE(0,3,2,1) );
	__m128 y_shuffled = _mm_shuffle_ps( y, y, _MM_SHUFFLE(0,3,2,1) );
	__m128 s_shuffled = _mm_shuffle_ps( s, s, _MM_SHUFFLE(0,3,2,1) );

	// A x B = ( f(a,b), f(b,c), f(c,d), f(d,a) )
	__m128 dx	= _mm_sub_ps( x, x_shuffled );								// dx = x_A - x_B
	__m128 dy	= _mm_sub_ps( y, y_shuffled );								// dy = y_A - y_B
	__m128 s_sum = _mm_add_ps(s, s_shuffled);
	__m128 f  = _MM_COMPUTE_FORCE(dx,dy,s_sum);
//	__m128 dsq	= _mm_add_ps( _mm_mul_ps( dx, dx ),
//							  _mm_mul_ps( dy, dy ) );						// dsq = (dx*dx + dy*dy)
//	__m128 f	= _mm_div_ps( _mm_add_ps( s, s_shuffled ), dsq );			// f = (s_A+s_B)/d^2
	__m128 fx	= _mm_mul_ps( dx, f ); 	// ( fx(a,b), fx(b,c), fx(c,d), fx(d,a) )
	__m128 fy	= _mm_mul_ps( dy, f );	// ( fy(a,b), fy(b,c), fy(c,d), fy(d,a) )

	fx_sum = _mm_add_ps( fx_sum, _mm_sub_ps( fx, _mm_shuffle_ps( fx, fx, _MM_SHUFFLE(2,1,0,3) ) ) ); // ( fx(a,b), fx(b,c), fx(c,d), fx(d,a) ) - ( fx(d,a), fx(a,b), fx(b,c), fx(c,d) )
	fy_sum = _mm_add_ps( fy_sum, _mm_sub_ps( fy, _mm_shuffle_ps( fy, fy, _MM_SHUFFLE(2,1,0,3) ) ) ); // ( fy(a,b), fy(b,c), fy(c,d), fy(d,a) ) - ( fy(d,a), fy(a,b), fy(b,c), fy(c,d) )

	// B = (c, d, a, b) (2x left rotate)
	x_shuffled = _mm_shuffle_ps( x_shuffled, x_shuffled, _MM_SHUFFLE(0,3,2,1) );
	y_shuffled = _mm_shuffle_ps( y_shuffled, y_shuffled, _MM_SHUFFLE(0,3,2,1) );
	s_shuffled = _mm_shuffle_ps( s_shuffled, s_shuffled, _MM_SHUFFLE(0,3,2,1) );

	// A x B = ( f(a,c), f(b,d), f(c,a), f(d,b) )
	dx = _mm_sub_ps( x, x_shuffled );										// dx = x_A - x_B
	dy = _mm_sub_ps( y, y_shuffled );										// dy = y_A - y_B
//	dsq = _mm_add_ps( _mm_mul_ps( dx, dx ),
//					  _mm_mul_ps( dy, dy ) );								// dsq = (dx*dx + dy*dy)
//	f = _mm_div_ps( _mm_add_ps( s, s_shuffled ), dsq );						// f = (s_A+s_B)/d^2
	s_sum = _mm_add_ps(s, s_shuffled);
	f  = _MM_COMPUTE_FORCE(dx,dy,s_sum);
	fx = _mm_mul_ps( dx, f );												// fx(a,c), fx(b,d), fx(c,a), fx(d,b)
	fy = _mm_mul_ps( dy, f );												// fy(a,c), fy(b,d), fy(c,a), fy(d,b)

	fx_sum = _mm_add_ps(fx_sum, fx);  // ( fx(a,c), fx(b,d), fx(c,a), fx(d,b) )
	fy_sum = _mm_add_ps(fy_sum, fy);  // ( fy(a,c), fy(b,d), fy(c,a), fy(d,b) )

	// Note: f(x,y) = -f(y,x)
	// f(a) = f(a,b) - f(d,a) + f(a,c) = f(a,b) + f(a,d) + f(a,c)
	// f(b) = f(b,c) - f(a,b) + f(b,d) = f(b,c) + f(b,a) + f(b,d)
	// f(c) = f(c,d) - f(b,c) + f(c,a) = f(c,d) + f(c,b) + f(c,a)
	// f(d) = f(d,a) - f(c,d) + f(d,b) = f(d,a) + f(d,c) + f(d,b)
	_mm_store_ps( ptr_fx, fx_sum );
	_mm_store_ps( ptr_fy, fy_sum );
}

inline void eval_direct_aligned_SSE(float* ptr_x, float* ptr_y, float* ptr_s, float* ptr_fx, float* ptr_fy, size_t n)
{
	for (__uint32 i=0; i < n; i+=4)
	{
		// register for i
		__m128 x      = _mm_load_ps( ptr_x + i );
		__m128 y      = _mm_load_ps( ptr_y + i );
		__m128 s	  = _mm_load_ps( ptr_s + i );
		__m128 fx_sum = _mm_load_ps( ptr_fx+ i );
		__m128 fy_sum = _mm_load_ps( ptr_fy+ i );

		// register for j, first we need to deal with Block(i,i)
		__m128 x_shuffled = _mm_shuffle_ps( x, x, _MM_SHUFFLE(0,3,2,1) );
		__m128 y_shuffled = _mm_shuffle_ps( y, y, _MM_SHUFFLE(0,3,2,1) );
		__m128 s_shuffled = _mm_shuffle_ps( s, s, _MM_SHUFFLE(0,3,2,1) );
		__m128 fx_sum_shuffled;
		__m128 fy_sum_shuffled;

		// begin copy paste from above
		__m128 dx	= _mm_sub_ps( x, x_shuffled );								// dx = x_A - x_B
		__m128 dy	= _mm_sub_ps( y, y_shuffled );								// dy = y_A - y_B
//		__m128 dsq	= _mm_add_ps( _mm_mul_ps( dx, dx ),
//								  _mm_mul_ps( dy, dy ) );						// dsq = (dx*dx + dy*dy)
//		__m128 f	= _mm_div_ps( _mm_add_ps( s, s_shuffled ), dsq );			// f = (s_A+s_B)/d^2
		__m128 s_sum = _mm_add_ps(s, s_shuffled);
		__m128 f    = _MM_COMPUTE_FORCE(dx,dy,s_sum);
		__m128 fx	= _mm_mul_ps( dx, f ); 	// ( fx(a,b), fx(b,c), fx(c,d), fx(d,a) )
		__m128 fy	= _mm_mul_ps( dy, f );	// ( fy(a,b), fy(b,c), fy(c,d), fy(d,a) )

		fx_sum = _mm_add_ps( fx_sum, _mm_sub_ps( fx, _mm_shuffle_ps( fx, fx, _MM_SHUFFLE(2,1,0,3) ) ) ); // ( fx(a,b), fx(b,c), fx(c,d), fx(d,a) ) - ( fx(d,a), fx(a,b), fx(b,c), fx(c,d) )
		fy_sum = _mm_add_ps( fy_sum, _mm_sub_ps( fy, _mm_shuffle_ps( fy, fy, _MM_SHUFFLE(2,1,0,3) ) ) ); // ( fy(a,b), fy(b,c), fy(c,d), fy(d,a) ) - ( fy(d,a), fy(a,b), fy(b,c), fy(c,d) )

		x_shuffled = _mm_shuffle_ps( x_shuffled, x_shuffled, _MM_SHUFFLE(0,3,2,1) );
		y_shuffled = _mm_shuffle_ps( y_shuffled, y_shuffled, _MM_SHUFFLE(0,3,2,1) );
		s_shuffled = _mm_shuffle_ps( s_shuffled, s_shuffled, _MM_SHUFFLE(0,3,2,1) );

		dx = _mm_sub_ps( x, x_shuffled );										// dx = x_A - x_B
		dy = _mm_sub_ps( y, y_shuffled );										// dy = y_A - y_B
//		dsq = _mm_add_ps( _mm_mul_ps( dx, dx ),
//						  _mm_mul_ps( dy, dy ) );								// dsq = (dx*dx + dy*dy)
//		f = _mm_div_ps( _mm_add_ps( s, s_shuffled ), dsq );						// f = (s_A+s_B)/d^2
		s_sum = _mm_add_ps(s, s_shuffled);
		f  = _MM_COMPUTE_FORCE(dx,dy,s_sum);
		fx = _mm_mul_ps( dx, f );												// fx(a,c), fx(b,d), fx(c,a), fx(d,b)
		fy = _mm_mul_ps( dy, f );												// fy(a,c), fy(b,d), fy(c,a), fy(d,b)

		fx_sum = _mm_add_ps(fx_sum, fx);										// ( fx(a,c), fx(b,d), fx(c,a), fx(d,b) )
		fy_sum = _mm_add_ps(fy_sum, fy);

		// end copy paste from above
		for (__uint32 j=i+4; j < n; j+=4)
		{
			x_shuffled		= _mm_load_ps( ptr_x +j );
			y_shuffled		= _mm_load_ps( ptr_y +j );
			s_shuffled		= _mm_load_ps( ptr_s +j );
			fx_sum_shuffled	= _mm_load_ps( ptr_fx+j );
			fy_sum_shuffled	= _mm_load_ps( ptr_fy+j );

			for (int k=0;k<4;k++)
			{
				dx = _mm_sub_ps(x, x_shuffled);
				dy = _mm_sub_ps(y, y_shuffled);
//				dsq= _mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy));
//				f  = _mm_div_ps(_mm_add_ps(s, s_shuffled), dsq);
				s_sum = _mm_add_ps(s, s_shuffled);
				f  = _MM_COMPUTE_FORCE(dx,dy,s_sum);
				fx = _mm_mul_ps(dx, f);
				fy = _mm_mul_ps(dy, f);
				// add forces to i's sum
				fx_sum = _mm_add_ps(fx_sum, fx);
				fy_sum = _mm_add_ps(fy_sum, fy);
				// sub forces from j's sum
				fx_sum_shuffled = _mm_sub_ps(fx_sum_shuffled, fx);
				fy_sum_shuffled = _mm_sub_ps(fy_sum_shuffled, fy);
				// shuffle everything for the next pairing
				x_shuffled		= _mm_shuffle_ps(x_shuffled,  x_shuffled,  _MM_SHUFFLE(0,3,2,1));
				y_shuffled		= _mm_shuffle_ps(y_shuffled,  y_shuffled,  _MM_SHUFFLE(0,3,2,1));
				s_shuffled		= _mm_shuffle_ps(s_shuffled,  s_shuffled,  _MM_SHUFFLE(0,3,2,1));
				fx_sum_shuffled = _mm_shuffle_ps(fx_sum_shuffled, fx_sum_shuffled, _MM_SHUFFLE(0,3,2,1));
				fy_sum_shuffled = _mm_shuffle_ps(fy_sum_shuffled, fy_sum_shuffled, _MM_SHUFFLE(0,3,2,1));
			}
			// note: after shuffling 4 times we are back to original order
			_mm_store_ps(ptr_fx + j, fx_sum_shuffled);
			_mm_store_ps(ptr_fy + j, fy_sum_shuffled);
		}
		_mm_store_ps( ptr_fx + i, fx_sum );
		_mm_store_ps( ptr_fy + i, fy_sum );
	}
}

void eval_direct_fast(float* x, float* y, float* s, float* fx, float* fy, size_t n)
{
	if (n>8)
	{
		const float* ptr = x;
		const float* fence1 =  align_16_next_ptr(ptr);
		const float* fence2 =  align_16_prev_ptr(ptr+n);
		const size_t numA = fence1 - ptr;
		const size_t numB = fence2 - fence1;
		const size_t numC = (ptr+n) - fence2;
		const size_t numAB = fence2 - ptr;

		// eval AxA
		eval_direct(x, y, s, fx, fy, numA);

		// eval BxA
		eval_direct(x+numA, y+numA, s+numA, fx+numA, fy+numA, numB,
					x, y, s, fx, fy, numA);
		// eval AxC
		eval_direct(x, y, s, fx, fy, numA,
					x+numAB, y+numAB, s+numAB, fx+numAB, fy+numAB, numC);
		// eval BxB
		//eval_direct(x+numA, y+numA, s+numA, fx+numA, fy+numA, numB);
		eval_direct_aligned_SSE(x+numA, y+numA, s+numA, fx+numA, fy+numA, numB);
		// eval BxC
		eval_direct(x+ numA, y+ numA, s+ numA, fx+ numA, fy+ numA, numB,
					x+numAB, y+numAB, s+numAB, fx+numAB, fy+numAB, numC);

		// eval CxC
		eval_direct(x+numAB, y+numAB, s+numAB, fx+numAB, fy+numAB, numC);
	} else
	{
		eval_direct(x, y, s, fx, fy, n);
	}
}
//! kernel function to evaluate forces between two sets of points with coords x1, y1 (x2, y2) directly. result is stored in fx1, fy1 (fx2, fy2
void eval_direct_fast(float* x1, float* y1, float* s1, float* fx1, float* fy1, size_t n1,
					 float* x2, float* y2, float* s2, float* fx2, float* fy2, size_t n2)
{
	if ((n1>8) && (n2>8))
	{
		const float* ptr1 = x1;
		const float* ptr2 = x2;
		const float* fence11 =  align_16_next_ptr(ptr1);
		const float* fence21 =  align_16_next_ptr(ptr2);
		const float* fence12 =  align_16_prev_ptr(ptr1+n1);
		const float* fence22 =  align_16_prev_ptr(ptr2+n2);
		const size_t numA1 = fence11 - ptr1;
		const size_t numA2 = fence21 - ptr2;
		const size_t numB1 = fence12 - fence11;
		const size_t numB2 = fence22 - fence21;
		const size_t numC1 = (ptr1+n1) - fence12;
		const size_t numC2 = (ptr2+n2) - fence22;
		const size_t numAB1 = fence12 - ptr1;
		const size_t numAB2 = fence22 - ptr2;

		// eval A1 x ALL2
		eval_direct(x1, y1, s1, fx1, fy1, numA1,
					x2, y2, s2, fx2, fy2, n2);

		// eval C1x ALL2
		eval_direct(x1, y1, s1, fx1, fy1, numC1,
					x2, y2, s2, fx2, fy2, n2);

		// eval B1x A2
		eval_direct(x1+numA1, y1+numA1, s1+numA1, fx1+numA1, fy1+numA1, numB1,
					x2	   , y2     , s2     , fx2     , fy2     , numA2);

		// eval B1x C2
		eval_direct(x1+ numA1 , y1+ numA1, s1+ numA1, fx1+ numA1, fy1+ numA1, numB1,
					x2+ numAB2, y2+numAB2, s2+numAB2, fx2+numAB2, fy2+numAB2, numC2);

		// finally B1xB2
		eval_direct_aligned_SSE(x1+numA1, y1+numA1, s1+numA1, fx1+numA1, fy1+numA1, numB1,
							x2+numA2, y2+numA2, s2+numA2, fx2+numA2, fy2+numA2, numB2);
	} else
	{
		eval_direct(x1, y1, s1, fx1, fy1, n1, x2, y2, s2, fx2, fy2, n2);
	}
}
#endif


/*template<typename T>
inline __w64 int align_16_begin(T* ptr)
{
	return align_16_next_ptr(ptr) - ptr;
	//return ((T*)((((int)ptr) + 15) &~ 0x0F)) - ptr;
}

template<typename T, typename N>
inline __w64 int align_16_end(T* ptr, const N n)
{
	return (n) - ( (ptr+n) - align_16_prev_ptr( ptr+n ) );
	//return ((T*)(((int)(ptr + n)) &~ 0x0F)) - ptr;
}*/

//
//  ______________________
//  |AxC|    BxC     |CxC|
//  |___|____________|___|
//  |   |            |   |
//  |AxB|            |   |
//  |   |     BxB    |   |
//  |   |            |   |
//  |___|____________|___|
//  |AxA|            |   |
//  |___|____________|___|
//   x   fence1    fence2 x+n
/*void eval_direct_sse(float* x, float* y, float* s, float* fx, float* fy, __uint32 n)
{
	if (n>8)
	{
		const float* ptr = x;
		const float* fence1 =  align_16_next_ptr(ptr);
		const float* fence2 =  align_16_prev_ptr(ptr+n);
		const __w64 int numA = fence1 - ptr;
		const __w64 int numB = fence2 - fence1;
		const __w64 int numC = (ptr+n) - fence2;
		const __w64 int numAB = fence2 - ptr;

		// eval AxA
		eval_direct(x, y, s, fx, fy, numA);

		// eval BxA
		eval_direct(x+numA, y+numA, s+numA, fx+numA, fy+numA, numB,
					x, y, s, fx, fy, numA);
		// eval AxC
		eval_direct(x, y, s, fx, fy, numA,
					x+numAB, y+numAB, s+numAB, fx+numAB, fy+numAB, numC);
		// eval BxB
		//eval_direct(x+numA, y+numA, s+numA, fx+numA, fy+numA, numB);
		eval_direct_aligned_aligned_sse(x+numA, y+numA, s+numA, fx+numA, fy+numA, numB);
		//eval_direct_aligned_aligned_sse(x+numA, y+numA, s+numA, fx+numA, fy+numA, numB);
		// eval BxC
		eval_direct(x+ numA, y+ numA, s+ numA, fx+ numA, fy+ numA, numB,
					x+numAB, y+numAB, s+numAB, fx+numAB, fy+numAB, numC);

		// eval CxC
		eval_direct(x+numAB, y+numAB, s+numAB, fx+numAB, fy+numAB, numC);
	} else
	{
		for (__uint32 i=0; i < n; i++)
		{
			for (__uint32 j=i+1; j < n; j++)
			{
				float dx = x[i] - x[j];
				float dy = y[i] - y[j];
				float dsq = dx*dx + dy*dy;
				float f = (s[i]+s[j]) / dsq;
				fx[i] += dx*f;
				fy[i] += dy*f;
				fx[j] -= dx*f;
				fy[j] -= dy*f;
			}
		}
	}*/

/*	//const void* start16 = (void*)(((int)x + 15) &~ 0x0F);
	//const void* end16 = (void*)(((int)(y + n)) &~ 0x0F);
	//std::cout << x  << " " << n << " begin i: " << x+align_16_begin(x) << " end i: " << x+align_16_end(x, n) << std::endl;
	int i = 0;
	// i beginning (not 16 byte aligned)
	for (i = 0; i < (align_16_begin(x)); i++)
	{
		const float* inner_ptr = x + i + 1;
		int j;
		// i beginning (not 16 byte aligned)
		// j beginning (not 16 byte aligned)
		for (j = i + 1; j < (i+1+align_16_begin(inner_ptr)); j++)
		{
			// 1 vs 1
			float dx = x[i] - x[j];
			float dy = y[i] - y[j];
			float dsq = dx*dx + dy*dy;
			float f = (s[i]+s[j]) / dsq;
			fx[i] += dx*f;
			fy[i] += dy*f;
			fx[j] -= dx*f;
			fy[j] -= dy*f;
		}
		// i beginning (not 16 byte aligned)
		// j 16 byte aligned blocks
		for ( ; j < (i+1+align_16_end(inner_ptr, n-i)); j += 4)
		{
			if (!is_align_16(x+j)) std::cout << "Not aligned j 1 vs 4"<< std::endl;
			// 1 vs 4
			Vec4f dx(x[i] - Vec4f(x+j));
			Vec4f dy(y[i] - Vec4f(y+j));
			Vec4f dsq(dx*dx + dy*dy);
			Vec4f f = (s[i] + Vec4f(s+j)) / dsq;
			Vec4f fxv(dx*f);
			Vec4f fyv(dy*f);
			Vec4f fx_j(fx+j);
			Vec4f fy_j(fy+j);
			fx[i] += fxv.hsum();
			fy[i] += fyv.hsum();
			fx_j -= fxv;
			fy_j -= fyv;

			fx_j.store(fx+j);
			fy_j.store(fy+j);
		}
		// i beginning (not 16 byte aligned)
		// j end (not 16 byte aligned)
		for ( ; j < (n); j++)
		{
			// 1 vs 1
			float dx = x[i] - x[j];
			float dy = y[i] - y[j];
			float dsq = dx*dx + dy*dy;
			float f = (s[i]+s[j]) / dsq;
			fx[i] += dx*f;
			fy[i] += dy*f;
			fx[j] -= dx*f;
			fy[j] -= dy*f;
		}
	}
	// i 16 byte aligned blocks
	for ( ; i < (align_16_end(x, n)); i += 4)
	{
		if (!is_align_16(x+i)) std::cout << "Not aligned 4 vs 1"<< std::endl;
		Vec4f x_i(x+i);
		Vec4f y_i(y+i);
		Vec4f s_i(s+i);
		Vec4f fx_i(fx+i);
		Vec4f fy_i(fy+i);

		const float* inner_ptr = x + i + 1;
		int j;
		// i 16 byte aligned blocks
		// j beginning (not 16 byte aligned)
		for (j = i + 1; j < (i+1+align_16_begin(inner_ptr)); j++)
		{
			if (!is_align_16(x+i)) std::cout << "Not aligned i 4 vs 1 "<< j<< std::endl;
			// 4 vs 1
			Vec4f dx = x_i - x[j];
			Vec4f dy = y_i - y[j];
			Vec4f dsq = dx*dx + dy*dy;
			Vec4f f = (s_i + s[j]) / dsq;
			Vec4f fxv(dx*f);
			Vec4f fyv(dy*f);
			fx_i += fxv;
			fy_i += fyv;
			fx[j] -= fxv.hsum();
			fy[j] -= fyv.hsum();
		}
		// i 16 byte aligned blocks
		// j 16 byte aligned blocks
		for ( ; j < i+1+align_16_end(inner_ptr, n-i); j += 4)
		{
			//std::cout << j << std::endl;
			//std::cout << x+align_16_begin(inner_ptr) << std::endl;
			//if (!is_align_16(x+i)) std::cout << "Not aligned i  4 vs 4 " << x+i<< std::endl;
			//if (!is_align_16(x+j)) std::cout << "Not aligned j  4 vs 4 "<<  n-i << " "<< x+j<< std::endl;

			// 4 vs 4
			Vec4f fx_j(fx+j);
			Vec4f fy_j(fy+j);

			Vec4f dx(x_i - Vec4f(x+j));
			Vec4f dy(y_i - Vec4f(y+j));

			Vec4f dsq(dx*dx + dy*dy);
			Vec4f f = (s_i + Vec4f(s+j)) / dsq;

			Vec4f fxv(dx*f);
			Vec4f fyv(dy*f);

			fx_i += fxv;
			fy_i += fyv;
			fx_j -= fxv;
			fy_j -= fyv;

			fx_j.store(fx+j);
			fy_j.store(fy+j);
		}
		// i 16 byte aligned blocks
		// j end (not 16 byte aligned)
		for ( ; j < (n); j++)
		{
			// 4 vs 1
			Vec4f dx = x_i - x[j];
			Vec4f dy = y_i - y[j];
			Vec4f dsq = dx*dx + dy*dy;
			Vec4f f = (s_i + s[j]) / dsq;
			Vec4f fxv(dx*f);
			Vec4f fyv(dy*f);
			fx_i += fxv;
			fy_i += fyv;
			fx[j] -= fxv.hsum();
			fy[j] -= fyv.hsum();
		}
		fx_i.store(fx+i);
		fy_i.store(fy+i);
	}
	// i end (not 16 byte aligned)
	for ( ; i < (n); i++)
	{
		const float* inner_ptr = x + i + 1;
		int j;
		//std::cout << x  << " " << n << " begin j: " << align_16_begin(inner_ptr) << " end j: " << align_16_end(inner_ptr, n-i-1) << std::endl;
		// i end (not 16 byte aligned)
		// j beginning (not 16 byte aligned)
		//std::cout << "before first loop i: "  << i<< std::endl;
		for (j = i + 1; j < i+1+(align_16_begin(inner_ptr)); j++)
		{
			//std::cout << i << " " << j << std::endl;
			// 1 vs 1
			float dx = x[i] - x[j];
			float dy = y[i] - y[j];
			float dsq = dx*dx + dy*dy;
			float f = (s[i]+s[j]) / dsq;
			fx[i] += dx*f;
			fy[i] += dy*f;
			fx[j] -= dx*f;
			fy[j] -= dy*f;
		}

		//std::cout << "after first loop" << std::endl;
		// i beginning (not 16 byte aligned)
		// j 16 byte aligned blocks
		for ( ; j < i+1+(align_16_end(inner_ptr, n-i)); j += 4)
		{
			if (!is_align_16(x+j)) std::cout << "Not aligned"<< std::endl;
			// 1 vs 4
			Vec4f dx(x[i] - Vec4f(x+j));
			Vec4f dy(y[i] - Vec4f(y+j));
			Vec4f dsq(dx*dx + dy*dy);
			Vec4f f = (s[i] + Vec4f(s+j)) / dsq;
			Vec4f fxv(dx*f);
			Vec4f fyv(dy*f);
			Vec4f fx_j(fx+j);
			Vec4f fy_j(fy+j);
			fx[i] += fxv.hsum();
			fy[i] += fyv.hsum();
			fx_j -= fxv;
			fy_j -= fyv;

			fx_j.store(fx+j);
			fy_j.store(fy+j);
		}
		// i beginning (not 16 byte aligned)
		// j end (not 16 byte aligned)
		for ( ; j < (n); j++)
		{
			// 1 vs 1
			float dx = x[i] - x[j];
			float dy = y[i] - y[j];
			float dsq = dx*dx + dy*dy;
			float f = (s[i]+s[j]) / dsq;
			fx[i] += dx*f;
			fy[i] += dy*f;
			fx[j] -= dx*f;
			fy[j] -= dy*f;
		}
	}
	} else
	{
		for (__uint32 i=0; i < n; i++)
		{
			for (__uint32 j=i+1; j < n; j++)
			{
				float dx = x[i] - x[j];
				float dy = y[i] - y[j];
				float dsq = dx*dx + dy*dy;
				float f = (s[i]+s[j]) / dsq;
				fx[i] += dx*f;
				fy[i] += dy*f;
				fx[j] -= dx*f;
				fy[j] -= dy*f;
			}
		}
	}*/
//}



void fast_multipole_l2p(double* localCoeffiecients, __uint32 numCoeffiecients, double centerX, double centerY,
				float x, float y, float q, float& fx, float&fy)
{
	double* source_coeff = localCoeffiecients; //+ source*(m_numCoeff << 1);
	ComplexDouble ak;
	ComplexDouble res;
	ComplexDouble delta(ComplexDouble(x,y) - ComplexDouble(centerX, centerY));// + (source << 1))); //m_x[source], y - m_y[source]);
	ComplexDouble delta_k(1);
	for (__uint32 k=1; k<numCoeffiecients; k++)
	{
		ak.load(source_coeff+(k<<1));
		res += ak*delta_k*(double)k;
		delta_k *= delta;
	}
	res = res.conj();
	double resTemp[2];
	res.store_unaligned(resTemp);
#ifdef FME_KERNEL_USE_OLD
	fx -= (float)resTemp[0];
	fy -= (float)resTemp[1];
#else
	fx -= (float)resTemp[0]*q;
	fy -= (float)resTemp[1]*q;
#endif
}


void fast_multipole_p2m(double* mulitCoeffiecients, __uint32 numCoeffiecients, double centerX, double centerY,
					float x, float y, float q)
{
	double* receiv_coeff = mulitCoeffiecients;
	// a0 += q_i
	receiv_coeff[0] += (double)q;
	// a_1..m
	ComplexDouble ak;
	// p - z0
	ComplexDouble delta(ComplexDouble(x, y) - ComplexDouble(centerX, centerY));
	// (p - z0)^k
	ComplexDouble delta_k(delta);
	for (__uint32 k=1; k<numCoeffiecients; k++)
	{
		ak.load(receiv_coeff+(k<<1));
		ak -= delta_k*(q/(double)k);
		ak.store(receiv_coeff+(k<<1));
		delta_k *= delta;
	}
}

}
