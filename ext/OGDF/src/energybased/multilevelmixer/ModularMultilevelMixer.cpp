/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief MMM is a Multilevel Graph drawing Algorithm that can use different modules.
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



#include <ogdf/basic/basic.h>
#include <ogdf/energybased/multilevelmixer/ModularMultilevelMixer.h>
#include <ogdf/energybased/multilevelmixer/SolarMerger.h>
#include <ogdf/energybased/multilevelmixer/BarycenterPlacer.h>
#include <ogdf/energybased/FastMultipoleEmbedder.h>
#include <ogdf/energybased/SpringEmbedderFR.h>
#include <time.h>

#ifdef OGDF_MMM_LEVEL_OUTPUTS
#include <sstream>
#include <string>
#endif


namespace ogdf {

ModularMultilevelMixer::ModularMultilevelMixer()
{
	// options
	m_times              = 1;
	m_fixedEdgeLength    = -1.0f;
	m_fixedNodeSize      = -1.0f;
	m_coarseningRatio    = 1.0;
	m_levelBound         = false;
	m_randomize          = false;

	// module options
	setMultilevelBuilder(new SolarMerger);
	setInitialPlacer    (new BarycenterPlacer);
	setLevelLayoutModule(new SpringEmbedderFR);
}


void ModularMultilevelMixer::call(GraphAttributes &GA)
{   //ensure consistent behaviour of the two call Methods
	MultilevelGraph MLG(GA);
	call(MLG);
	MLG.exportAttributes(GA);
}


void ModularMultilevelMixer::call(MultilevelGraph &MLG)
{
	const Graph &G = MLG.getGraph();

	m_errorCode = ercNone;
	clock_t time = clock();
	if ((m_multilevelBuilder.valid() == false || m_initialPlacement.valid() == false) && m_oneLevelLayoutModule.valid() == false) {
		OGDF_THROW(AlgorithmFailureException);
	}

	if (m_fixedEdgeLength > 0.0) {
		edge e;
		forall_edges(e,G) {
			MLG.weight(e, m_fixedEdgeLength);
		}
	}

	if (m_fixedNodeSize > 0.0) {
		node v;
		forall_nodes(v,G) {
			MLG.radius(v, m_fixedNodeSize);
		}
	}

	if (m_multilevelBuilder.valid() && m_initialPlacement.valid())
	{
		double lbound = 16.0 * log(double(G.numberOfNodes()))/log(2.0);
		m_multilevelBuilder.get().buildAllLevels(MLG);

		//Part for experiments: Stop if number of levels too high
#ifdef OGDF_MMM_LEVEL_OUTPUTS
		int nlevels = m_multilevelBuilder.get().getNumLevels();
#endif
		if (m_levelBound)
		{
			if ( m_multilevelBuilder.get().getNumLevels() > lbound)
			{
				m_errorCode = ercLevelBound;
				return;
			}
		}
		node v;
		if (m_randomize)
		{
			forall_nodes(v,G) {
				MLG.x(v, (float)randomDouble(-1.0, 1.0));
				MLG.y(v, (float)randomDouble(-1.0, 1.0));
			}
		}

		while(MLG.getLevel() > 0)
		{
			if (m_oneLevelLayoutModule.valid()) {
				for(int i = 1; i <= m_times; i++) {
					m_oneLevelLayoutModule.get().call(MLG.getGraphAttributes());
				}
			}

#ifdef OGDF_MMM_LEVEL_OUTPUTS
			//Debugging output
			std::stringstream ss;
			ss << nlevels--;
			std::string s;
			ss >> s;
			s = "LevelLayout"+s;
			String fs(s.c_str());
			fs += ".gml";
			MLG.writeGML(fs);
#endif

			MLG.moveToZero();

			int nNodes = G.numberOfNodes();
			m_initialPlacement.get().placeOneLevel(MLG);
			m_coarseningRatio = float(G.numberOfNodes()) / nNodes;

#ifdef OGDF_MMM_LEVEL_OUTPUTS
			//debug only
			s = s+"_placed.gml";
			MLG.writeGML(String(s.c_str()));
#endif
		} //while level
	}

	//Final level

	if(m_finalLayoutModule.valid() ||  m_oneLevelLayoutModule.valid())
	{
		LayoutModule &lastLayoutModule = (m_finalLayoutModule.valid() != 0 ? m_finalLayoutModule.get() : m_oneLevelLayoutModule.get());

		for(int i = 1; i <= m_times; i++) {
			lastLayoutModule.call(MLG.getGraphAttributes());
		}
	}

	time = clock() - time;
}


} // namespace ogdf
