/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief useable example of the Modular Multilevel Mixer
 *
 * \author Gereon Bartel
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

#include <ogdf/energybased/multilevelmixer/MMMExampleNiceLayout.h>
#include <ogdf/basic/PreprocessorLayout.h>
#include <ogdf/packing/ComponentSplitterLayout.h>
#include <ogdf/energybased/multilevelmixer/ModularMultilevelMixer.h>
#include <ogdf/energybased/multilevelmixer/ScalingLayout.h>
#include <ogdf/energybased/FastMultipoleEmbedder.h>

#include <ogdf/energybased/multilevelmixer/EdgeCoverMerger.h>
#include <ogdf/energybased/multilevelmixer/BarycenterPlacer.h>

namespace ogdf {

MMMExampleNiceLayout::MMMExampleNiceLayout()
{
}


void MMMExampleNiceLayout::call(GraphAttributes &GA)
{
	MultilevelGraph MLG(GA);
	call(MLG);
	MLG.exportAttributes(GA);
}


void MMMExampleNiceLayout::call(MultilevelGraph &MLG)
{
	// Fast Multipole Embedder
	FastMultipoleEmbedder * FME = new FastMultipoleEmbedder();
	FME->setNumIterations(1000);
	FME->setRandomize(false);

	// Fast Edges Only Embedder
	FastMultipoleEmbedder * FEOE = new FastMultipoleEmbedder();
	FEOE->setNumIterations(0);
	FEOE->setRandomize(false);

	// Edge Cover Merger
	EdgeCoverMerger * ECM = new EdgeCoverMerger();
	ECM->setFactor(2.0);
	ECM->setEdgeLengthAdjustment(0); // BEFORE (but arg is int!): ECM->setEdgeLengthAdjustment(0.1);

	// Barycenter Placer with weighted Positions
	BarycenterPlacer * BP = new BarycenterPlacer();
	BP->weightedPositionPriority(true);

	// No Scaling
	ScalingLayout * SL = new ScalingLayout();
	SL->setExtraScalingSteps(0);
	SL->setScaling(1.0, 1.0);
	SL->setScalingType(ScalingLayout::st_relativeToDrawing);
	SL->setSecondaryLayout(FME);
	SL->setLayoutRepeats(1);

	ModularMultilevelMixer *MMM = new ModularMultilevelMixer;
	MMM->setLayoutRepeats(1);
//	MMM->setAllEdgeLenghts(5.0);
//	MMM->setAllNodeSizes(1.0);
	MMM->setLevelLayoutModule(SL);
	MMM->setInitialPlacer(BP);
	MMM->setMultilevelBuilder(ECM);

	ComponentSplitterLayout *CS = new ComponentSplitterLayout;
	CS->setLayoutModule(MMM);
	PreprocessorLayout PPL;
	PPL.setLayoutModule(CS);
	PPL.setRandomizePositions(true);

	PPL.call(MLG);
}

} // namespace ogdf

