/*
 * $Revision: 2571 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 17:25:20 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of shelling order used by the Mixed-Model
 * layout algorithm.
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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_MM_ORDER_H
#define OGDF_MM_ORDER_H


#include <ogdf/planarlayout/ShellingOrder.h>


namespace ogdf {


class MMOrder
{
public:
	MMOrder() { }

	void init(PlanRep &PG, ShellingOrderModule &compOrder, adjEntry adjExternal);

	int rank(node v) const {
		return m_lmc.rank(v);
	}

	int length() const {
		return m_lmc.length();
	}

	const ShellingOrderSet &operator[](int k) const {
		return m_lmc[k];
	}

	node operator()(int k, int i) const {
		return m_lmc(k,i);
	}

	int len(int k) const {
		return m_lmc.len(k);
	}

	Array<node> m_left, m_right;


private:
	ShellingOrder m_lmc;
};


} // end namespace ogdf


#endif
