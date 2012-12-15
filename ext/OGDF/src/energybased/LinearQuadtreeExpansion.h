/*
 * $Revision: 2559 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 15:04:28 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class LinearQuadtreeExpansion.
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

#ifndef OGDF_LINEAR_QUADTREE_EXPANSION_H
#define OGDF_LINEAR_QUADTREE_EXPANSION_H

#include "LinearQuadtree.h"

namespace ogdf {

class LinearQuadtreeExpansion
{
public:
	//! constructor
	LinearQuadtreeExpansion(__uint32 precision, const LinearQuadtree& tree);

	//! destructor
	~LinearQuadtreeExpansion(void);

	//! adds a point with the given charge to the receiver expansion
	void P2M(__uint32 point, __uint32 receiver);

	//! shifts the source multipole coefficient to the center of the receiver and adds them
	void M2M(__uint32 source, __uint32 receiver);

	//! converts the source multipole coefficient in to a local coefficients at the center of the receiver and adds them
	void M2L(__uint32 source, __uint32 receiver);

	//! shifts the source local coefficient to the center of the receiver and adds them
	void L2L(__uint32 source, __uint32 receiver);

	//! evaluates the derivate of the local expansion at the point and adds the forces to fx fy
	void L2P(__uint32 source, __uint32 point, float& fx, float& fy);

	//! returns the size in bytes
	__uint32 sizeInBytes() const { return m_numExp*m_numCoeff*sizeof(double)*4; }

	//! returns the array with multipole coefficients
	inline double* multiExp() const { return m_multiExp; }

	//! returns the array with local coefficients
	inline double* localExp() const { return m_localExp; }

	//! number of coefficients per expansions
	inline __uint32 numCoeff() const { return m_numCoeff; }

	//! the quadtree
	const LinearQuadtree& tree() { return m_tree; }
private:

	//! allocates the space for the coeffs
	void allocate();

	//! releases the memory for the coeffs
	void deallocate();

	//! the Quadtree reference
	const LinearQuadtree& m_tree;
public:
	//! the big multipole expansione coeff array
	double* m_multiExp;

	//! the big local expansion coeff array
	double* m_localExp;

public:
	//! the number of multipole (locale) expansions
	__uint32 m_numExp;

	//! the number of coeff per expansions
	__uint32 m_numCoeff;

	BinCoeff<double> binCoef;
};


} // end of namespace ogdf

#endif
