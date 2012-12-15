/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief ScalingLayout scales and calls a secondary layout
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


#include <ogdf/energybased/multilevelmixer/ScalingLayout.h>


namespace ogdf {

ScalingLayout::ScalingLayout() :
	m_minScaling(1.0),
	m_maxScaling(2.0),
	m_mmm(NULL),
	m_desEdgeLength(1.0),
	m_extraScalingSteps(0),
	m_layoutRepeats(1),
	m_scalingType(st_relativeToDrawing)
{
}


void ScalingLayout::call(GraphAttributes &GA)
{
	MultilevelGraph MLG(GA);
	call(MLG);
	MLG.exportAttributes(GA);
}


void ScalingLayout::call(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	double avgDesiredEdgeLength = 0.0;
	edge e;

	if (m_scalingType == st_relativeToAvgLength) {
		forall_edges(e, G) {
			avgDesiredEdgeLength += MLG.weight(e);
		}
		avgDesiredEdgeLength /= G.numberOfNodes();
	}

	double finalScaling = m_maxScaling;
	if ( (m_scalingType == st_absolute) && (m_mmm != 0))
	{
		finalScaling = max(m_mmm->coarseningRatio(), m_minScaling);
	}

	double avgStartEdgeLength = 0.0;
	for (unsigned int i = 0; i <= m_extraScalingSteps; i++) {
		double step;
		//KK this looks strange, shouldn't we start with step = 1 if extrascaling?
		//now we scale from max to min...
		if (m_extraScalingSteps > 0) {
			step = (double)i / (double)m_extraScalingSteps;
		} else {
			step = 0;
		}
		double scalingFactor = m_minScaling * step + finalScaling * (1.0-step);

		if (m_scalingType == st_absolute)
		{
			MLG.moveToZero();
			#ifdef OGDF_DEBUG
					cout << "Fix Scaling:  "<<scalingFactor<<" \n";
			#endif
			// scale to scaling
			node v;
			forall_nodes(v, G) {
				MLG.x(v, MLG.x(v) * scalingFactor);
				MLG.y(v, MLG.y(v) * scalingFactor);
			}
		}
		else
		{
			double avgEdgeLength = 0.0;
			forall_edges(e, G) {
				double x = MLG.x(e->source()) - MLG.x(e->target());
				double y = MLG.y(e->source()) - MLG.y(e->target());
				avgEdgeLength += sqrt( x*x + y*y );
			}
			avgEdgeLength /= G.numberOfNodes();

			if(avgEdgeLength <= 0.0) {
				MLG.moveToZero();
			} else {
				double scaling = 1.0;
				if (m_scalingType == st_relativeToDrawing) {
					if (i == 0) {
						avgStartEdgeLength = avgEdgeLength;
					}
					scaling = scalingFactor * avgStartEdgeLength / avgEdgeLength;
				} else {
						if (m_scalingType == st_relativeToDesiredLength)
						{
							scaling = scalingFactor * m_desEdgeLength / avgEdgeLength;
						} else //st_relativeToAvgLength
							scaling = scalingFactor * avgDesiredEdgeLength / avgEdgeLength;
	#ifdef OGDF_DEBUG
					cout << "Scaling: F/s "<<scalingFactor<<" "<<scaling<<"\n";
	#endif
				}

				MLG.moveToZero();

				// scale to scaling
				node v;
				forall_nodes(v, G) {
					MLG.x(v, MLG.x(v) * scaling);
					MLG.y(v, MLG.y(v) * scaling);
				}
			}
		}//no fix scaling

		if (m_secondaryLayoutModule.valid()) {
			for (unsigned int j = 1; j <= m_layoutRepeats; j++) {
				m_secondaryLayoutModule.get().call(MLG.getGraphAttributes());
			}
		}
	}
}


void ScalingLayout::setScaling(double min, double max)
{
	m_minScaling = min;
	m_maxScaling = max;
}


void ScalingLayout::setExtraScalingSteps(unsigned int steps)
{
	m_extraScalingSteps = steps;
}


void ScalingLayout::setSecondaryLayout(LayoutModule * layout)
{
	m_secondaryLayoutModule.set(layout);
}

void ScalingLayout::setMMM(ModularMultilevelMixer* mmm)
{
	m_mmm = mmm;
}


void ScalingLayout::setScalingType(ScalingType type)
{
	m_scalingType = type;
}


void ScalingLayout::setLayoutRepeats(unsigned int repeats)
{
	m_layoutRepeats = repeats;
}

void ScalingLayout::setDesiredEdgeLength(double eLength)
{
	m_desEdgeLength = eLength;
}

} // namespace ogdf
