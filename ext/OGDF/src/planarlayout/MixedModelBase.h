/*
 * $Revision: 2571 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 17:25:20 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Base functionality of Mixed-Model layout algorithm
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

#ifndef OGDF_MIXED_MODEL_BASE_H
#define OGDF_MIXED_MODEL_BASE_H


#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/GridLayout.h>
#include <ogdf/module/AugmentationModule.h>
#include <ogdf/module/ShellingOrderModule.h>
#include <ogdf/module/EmbedderModule.h>

#include "MMOrder.h"
#include "IOPoints.h"


namespace ogdf {


class MixedModelBase
{
public:
	MixedModelBase(PlanRep &PG, GridLayout &gridLayout) :
		m_PG(PG), m_adjExternal(0), m_gridLayout(gridLayout), m_iops(PG) { }

	virtual ~MixedModelBase() { }

	void computeOrder(
		AugmentationModule &augmenter,
		EmbedderModule *pEmbedder,
		adjEntry adjExternal,
		ShellingOrderModule &compOrder);

	void assignIopCoords();

	void placeNodes();
	void computeXCoords();
	void computeYCoords();

	void setBends();
	void postprocessing1();
	void postprocessing2();


	// functions for debugging output

	void printMMOrder(std::ostream &os);
	void printInOutPoints(std::ostream &os);
	void print(std::ostream &os, const InOutPoint &iop);
	void printNodeCoords(std::ostream &os);

	// avoid creation of assignment operator
	MixedModelBase &operator=(const MixedModelBase &);

private:
	PlanRep &m_PG;
	adjEntry m_adjExternal;

	GridLayout &m_gridLayout;

	MMOrder  m_mmo;
	IOPoints m_iops;
	Stack<PlanRep::Deg1RestoreInfo> m_deg1RestoreStack;

	Array<int> m_dyl, m_dyr;
	Array<ListConstIterator<InOutPoint> > m_leftOp, m_rightOp;
	NodeArray<ListConstIterator<InOutPoint> > m_nextLeft, m_nextRight;
	NodeArray<int> m_dxla, m_dxra;


	bool exists(adjEntry adj) {
		return m_PG.isDummy(adj->theEdge()) == false;
	}

	bool hasLeft (int k) const;
	bool hasRight(int k) const;

	void removeDeg1Nodes();

	void firstPoint(int &x, int &y, adjEntry adj);
	bool isRedundant(int x1, int y1, int x2, int y2, int x3, int y3);
};


} // end namespace ogdf


#endif
