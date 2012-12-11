/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class PlanarityGrid
 *
 * The PlanarityGrid energy function counts the number of
 * crossings. It contains two UniformGrids: One for the
 * current layout and one for the candidate layout.
 *
 * \author Rene Weiskircher
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


#include <ogdf/internal/energybased/PlanarityGrid.h>

namespace ogdf {

	PlanarityGrid::~PlanarityGrid()
	{
		delete m_currentGrid;
		if(m_candidateGrid != NULL)
			delete m_candidateGrid;
	}


	// intialize m_currentLayout and m_candidateLayout
	PlanarityGrid::PlanarityGrid(GraphAttributes &AG):
	EnergyFunction("PlanarityGrid",AG), m_layout(AG)
	{
		m_currentGrid = new UniformGrid(AG);
		m_candidateGrid = NULL;
	}


	// computes energy of layout, stores it and sets the crossingMatrix
	void PlanarityGrid::computeEnergy()
	{
		m_energy = m_currentGrid->numberOfCrossings();
	}


	// computes the energy if the node returned by testNode() is moved
	// to position testPos().
	void PlanarityGrid::compCandEnergy()
	{
		if(m_candidateGrid != NULL)
			delete m_candidateGrid;
		node v = testNode();
		const DPoint& newPos = testPos();
		if(m_currentGrid->newGridNecessary(v,newPos))
			m_candidateGrid = new UniformGrid(m_layout,v,newPos);
		else
			m_candidateGrid = new UniformGrid(*m_currentGrid,v,newPos);
		m_candidateEnergy = m_candidateGrid->numberOfCrossings();
	}


	// this functions sets the currentGrid to the candidateGrid
	void PlanarityGrid::internalCandidateTaken() {
		delete m_currentGrid;
		m_currentGrid = m_candidateGrid;
		m_candidateGrid = NULL;
	}


#ifdef OGDF_DEBUG
void PlanarityGrid::printInternalData() const {
	cout << "\nCurrent grid: " << *m_currentGrid;
	cout << "\nCandidate grid: ";
	if(m_candidateGrid != NULL)
		cout << *m_candidateGrid;
	else cout << "empty.";
}
#endif

}
