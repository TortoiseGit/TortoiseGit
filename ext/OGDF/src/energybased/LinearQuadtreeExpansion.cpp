/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class LinearQuadtreeExpansion.
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

#include "LinearQuadtreeExpansion.h"
#include "ComplexDouble.h"
#include "WSPD.h"
#include <complex>

using namespace ogdf::sse;

namespace ogdf {

LinearQuadtreeExpansion::LinearQuadtreeExpansion(__uint32 precision, const LinearQuadtree& tree) : m_tree(tree), m_numCoeff(precision), binCoef(2*m_numCoeff)
{
	m_numExp = m_tree.maxNumberOfNodes();
	allocate();
}


LinearQuadtreeExpansion::~LinearQuadtreeExpansion(void)
{
	deallocate();
}


void LinearQuadtreeExpansion::allocate()
{
	m_multiExp = (double*)MALLOC_16(m_numCoeff*sizeof(double)*2*m_numExp);
	m_localExp = (double*)MALLOC_16(m_numCoeff*sizeof(double)*2*m_numExp);
}


void LinearQuadtreeExpansion::deallocate()
{
	FREE_16(m_multiExp);
	FREE_16(m_localExp);
}


void LinearQuadtreeExpansion::P2M(__uint32 point, __uint32 receiver)
{
	double* receiv_coeff = m_multiExp + receiver*(m_numCoeff<<1);
	const double q = (double)m_tree.pointSize(point);
	const double x = (double)m_tree.pointX(point);
	const double y = (double)m_tree.pointY(point);
	const double centerX = (double)m_tree.nodeX(receiver);
	const double centerY = (double)m_tree.nodeY(receiver);
	// a0 += q_i
	receiv_coeff[0] += q;
	// a_1..m
	ComplexDouble ak;
	// p - z0
	ComplexDouble delta(ComplexDouble(x, y) - ComplexDouble(centerX, centerY));
	// (p - z0)^k
	ComplexDouble delta_k(delta);
	for (__uint32 k=1; k<m_numCoeff; k++)
	{
		ak.load(receiv_coeff+(k<<1));
		ak -= delta_k*(q/(double)k);
		ak.store(receiv_coeff+(k<<1));
		delta_k *= delta;
	}
}


void LinearQuadtreeExpansion::L2P(__uint32 source, __uint32 point, float& fx, float& fy)
{
	const double* source_coeff = m_localExp + source*(m_numCoeff << 1);
	const double x = (double)m_tree.pointX(point);
	const double y = (double)m_tree.pointY(point);
	const double centerX = (double)m_tree.nodeX(source);
	const double centerY = (double)m_tree.nodeY(source);
	ComplexDouble ak;
	ComplexDouble res;
	ComplexDouble delta(ComplexDouble(x,y) - ComplexDouble(centerX, centerY));
	ComplexDouble delta_k(1);
	for (__uint32 k=1; k<m_numCoeff; k++)
	{
		ak.load(source_coeff+(k<<1));
		res += ak*delta_k*(double)k;
		delta_k *= delta;
	}
	res = res.conj();
	double resTemp[2];
	res.store_unaligned(resTemp);

	fx -= ((float)resTemp[0]);
	fy -= ((float)resTemp[1]);
}


void LinearQuadtreeExpansion::M2M(__uint32 source, __uint32 receiver)
{
	double* receiv_coeff = m_multiExp + receiver*(m_numCoeff<<1);
	double* source_coeff = m_multiExp + source*(m_numCoeff<<1);

	const double center_x_source   = (double)m_tree.nodeX(source);
	const double center_y_source   = (double)m_tree.nodeY(source);
	const double center_x_receiver = (double)m_tree.nodeX(receiver);
	const double center_y_receiver = (double)m_tree.nodeY(receiver);

	ComplexDouble delta(ComplexDouble(center_x_source, center_y_source) - ComplexDouble(center_x_receiver, center_y_receiver));

	ComplexDouble a(source_coeff);
	ComplexDouble b(receiv_coeff);
	b += a;
	b.store(receiv_coeff);
	for (__uint32 l=1;l<m_numCoeff;l++)
	{
		b.load(receiv_coeff+(l<<1));
		ComplexDouble delta_k(1.0,0.0);
		for (__uint32 k=0;k<l;k++)
		{
			a.load(source_coeff+((l-k)<<1));
			b += a*delta_k*binCoef.value(l-1, k);
			delta_k *= delta;
		}
		a.load(source_coeff);
		b -= a*delta_k*(1/(double)l);
		b.store(receiv_coeff+(l<<1));
	}
}


void LinearQuadtreeExpansion::L2L(__uint32 source, __uint32 receiver)
{
	double* receiv_coeff = m_localExp + receiver*(m_numCoeff<<1);
	double* source_coeff = m_localExp + source*(m_numCoeff<<1);

	const double center_x_source   = (double)m_tree.nodeX(source);
	const double center_y_source   = (double)m_tree.nodeY(source);
	const double center_x_receiver = (double)m_tree.nodeX(receiver);
	const double center_y_receiver = (double)m_tree.nodeY(receiver);

	ComplexDouble center_receiver(center_x_receiver, center_y_receiver);
	ComplexDouble center_source(center_x_source, center_y_source);
	ComplexDouble delta(center_source - center_receiver);

	for (__uint32 l=0;l<m_numCoeff;l++)
	{
		ComplexDouble b(receiv_coeff+(l<<1));
		ComplexDouble delta_k(1.0,0.0);
		for (__uint32 k=l;k<m_numCoeff;k++)
		{
			ComplexDouble a(source_coeff+(k<<1));
			b += a*delta_k*binCoef.value(k, l);
			delta_k *= delta;
		}
		b.store(receiv_coeff+(l<<1));
	}
}


void LinearQuadtreeExpansion::M2L(__uint32 source, __uint32 receiver)
{
	double* receiv_coeff = m_localExp + receiver*(m_numCoeff<<1);
	double* source_coeff = m_multiExp + source*(m_numCoeff<<1);

	const float center_x_source   = (float)m_tree.nodeX(source);
	const float center_y_source   = (float)m_tree.nodeY(source);
	const float center_x_receiver = (float)m_tree.nodeX(receiver);
	const float center_y_receiver = (float)m_tree.nodeY(receiver);

	ComplexDouble center_receiver(center_x_receiver, center_y_receiver);
	ComplexDouble center_source(center_x_source, center_y_source);
	ComplexDouble delta0(center_source - center_receiver);

	ComplexDouble delta1 = -delta0;
	ComplexDouble delta1_l(delta1);
	ComplexDouble a;
	ComplexDouble a0(source_coeff);
	ComplexDouble b;
	ComplexDouble sum;
	for (__uint32 l=1;l<m_numCoeff;l++)
	{
		b.load(receiv_coeff+(l<<1));
		sum = a0*(-1/(double)l);
		ComplexDouble delta0_k(delta0);
		for (__uint32 k=1;k<m_numCoeff;k++)
		{
			a.load(source_coeff+(k<<1));
			sum += (a*binCoef.value(l+k-1, k-1)) / delta0_k;
			delta0_k *= delta0;
		}

		b += sum/delta1_l;
		b.store(receiv_coeff+(l<<1));
		delta1_l *= delta1;
	}

	// b0
	b.load(receiv_coeff);
	//std::complex<double> sdelta(m_center[(receiver<<1)] - m_center[(source<<1)],  m_center[(receiver<<1)+1] - m_center[(source<<1) +1]);//, m_x[receiver] - m_x[source], m_y[receiver] - m_y[source]);
	/*std::complex<double> sdelta(center_x_receiver - center_x_source,
								center_y_receiver - center_y_source);//, m_x[receiver] - m_x[source], m_y[receiver] - m_y[source]);

	if ((sdelta.real() <=0) && (sdelta.imag() == 0)) //no cont. compl. log fct exists !!!
		sdelta = log(sdelta + 0.00000001);
	else
		sdelta = log(sdelta);

	*/
	double r = delta1.length();//= sqrt(sdelta.real()*sdelta.real()+sdelta.imag()*sdelta.imag());
	double phi = atan((center_x_receiver - center_x_source)/(center_y_receiver - center_y_source));
	// sum = a0*log(z1 - z0)
	b += a0*ComplexDouble(log(r), phi);
	// (z1 - z0)^1
	//b += a0*ComplexDouble(sdelta.real(), sdelta.imag());
	delta1_l = delta1;
	for (__uint32 k=1;k<m_numCoeff;k++)
	{
		a.load(source_coeff+(k<<1));
		b += a/delta1_l;
		//(z1 - z0)^k
		delta1_l *= delta1;
	}
	b.store(receiv_coeff);
}

} // end of namespace ogdf
