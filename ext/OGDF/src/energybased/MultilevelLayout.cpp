/*
 * $Revision: 2620 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 16:28:52 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class MultilevelLayout
 *
 * \author Karsten Klein
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

#include "ogdf/energybased/MultilevelLayout.h"
#include "ogdf/energybased/SpringEmbedderFR.h"

namespace ogdf {

//! Sets the single level layout
void MultilevelLayout::setLayout(LayoutModule* L)
{
	m_mmm->setLevelLayoutModule(L);
}


//! Sets the method used for coarsening
void MultilevelLayout::setMultilevelBuilder(MultilevelBuilder* B)
{
	m_mmm->setMultilevelBuilder(B);
}


//! Sets the placement method used when refining the levels again.
void MultilevelLayout::setPlacer(InitialPlacer* P)
{
	m_mmm->setInitialPlacer(P);
}


MultilevelLayout::MultilevelLayout()
{
	m_mmm = new ModularMultilevelMixer();
	m_sc = new ScalingLayout();
	m_cs = new ComponentSplitterLayout();
	m_pp = new PreprocessorLayout();
	//initial placer, coarsener are the default
	//modules of m_mmm.
	//For the layout, we set a scaling layout with
	//standard level layout FR. This scales the layout
	//on each level (with a constant factor) and then applies the FR.
	m_sc->setSecondaryLayout(new SpringEmbedderFR);
	m_sc->setScalingType(ScalingLayout::st_relativeToDrawing);
	m_sc->setLayoutRepeats(1);

	m_sc->setScaling(1.0, 1.5);
	m_sc->setExtraScalingSteps(2);
	m_mmm->setLevelLayoutModule(m_sc);

	//	m_mmm->setLayoutRepeats(1);
	//	m_mmm->setAllEdgeLenghts(5.0);
	//	m_mmm->setAllNodeSizes(1.0);

	m_cs->setLayoutModule(m_mmm);
	m_pp->setLayoutModule(m_cs);
	m_pp->setRandomizePositions(true);

}//constructor


void MultilevelLayout::call(GraphAttributes &GA, GraphConstraints &GC)
{
	//we assume that both structures work on the same graph

	OGDF_THROW(AlgorithmFailureException);
}


void MultilevelLayout::call(GraphAttributes &GA)
{
	MultilevelGraph MLG(GA);

	// Call the nested call, including preprocessing,
	// component splitting, scaling, level layout.
	m_pp->call(MLG);

	MLG.exportAttributes(GA);
}

} //end namespace ogdf
