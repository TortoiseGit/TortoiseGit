/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Offers variety of possible SimDraw creations.
 *
 * \author Michael Schulz and Daniel Lueckerath
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

#include <ogdf/simultaneous/SimDrawCreator.h>
#include <ogdf/basic/graph_generators.h>

namespace ogdf {

//*************************************************************
//sets all edgeSubGraph values to zero
//
void SimDrawCreator::clearESG()
{
	edge e;
	forall_edges(e,*m_G)
		m_GA->subGraphBits(e) = 0;

}//end clearESG


//*************************************************************
//gives each edge in m_G a random edgeSubGraph value
//works with two graphs
void SimDrawCreator::randomESG2(int doubleESGProbability)
{
	OGDF_ASSERT( doubleESGProbability <= 100 );
	OGDF_ASSERT( doubleESGProbability >= 0 );

	clearESG();

	edge e;
	forall_edges(e,*m_G)
	{
		//all edges have a chance of doubleESGProbability (in percent)
		//to belong to both input graphs
		int doubleESGRandom = rand() % 100;
		if(doubleESGRandom < doubleESGProbability)
		{
			m_GA->addSubGraph(e, 0);
			m_GA->addSubGraph(e, 1);
		}
		else
		{
			// all edges, which do not belong to both input graphs
			// have a 50/50 chance to belong to graph 0 or to graph 1
			int singleESGRandom = rand() % 2;
			m_GA->addSubGraph(e, singleESGRandom);
		}
	}

}//end randomESG2


//*************************************************************
//gives each edge in m_G a random edgeSubGraph value
//works with three graphs
void SimDrawCreator::randomESG3(int doubleESGProbability, int tripleESGProbability)
{
	OGDF_ASSERT( doubleESGProbability + tripleESGProbability <= 100 );
	OGDF_ASSERT( doubleESGProbability >= 0 );
	OGDF_ASSERT( tripleESGProbability >= 0 );

	clearESG();

	edge e;
	forall_edges(e,*m_G)
	{
		/*All edges have a chance of tripleESGProbability (in percent)
		to belong to all three graphs.*/
		int multipleESGRandom = rand() % 100;
		if(multipleESGRandom < doubleESGProbability + tripleESGProbability)
		{
			m_GA->addSubGraph(e,0);
			m_GA->addSubGraph(e,1);
			m_GA->addSubGraph(e,2);

			/*Furthermore, all edges have a chance of doubleESGProbability
			to belong to two of three graphs.*/
			if(multipleESGRandom >= tripleESGProbability)
			{
				int removeESGRandom = rand() % 3;
				m_GA->removeSubGraph(e, removeESGRandom);
			}
		}
		else
		{
			//all edges, which do not belong to two or three graphs
			//have a 33 percent chance to belong to one of the three graphs.
			int singleESGRandom = rand() % 3;
			m_GA->addSubGraph(e, singleESGRandom);
		}
	}

}//end randomESG3


//*************************************************************
//gives each edge a random edgeSubgraph value
//works with graphNumber number of graphs
void SimDrawCreator::randomESG(int graphNumber)
{
	OGDF_ASSERT( 0 < graphNumber && graphNumber < 32 );

	int max = (int)pow((double)2,graphNumber+1)-1;
	edge e;
	forall_edges(e,*m_G)
	{
		int randomESGValue = 1 + rand() % max;
		m_GA->subGraphBits(e) = randomESGValue;
	}

}//end randomESG


//*************************************************************
//
//
void SimDrawCreator::createRandom(int numberOfNodes,
	int numberOfEdges,
	int numberOfBasicGraphs)
{
	OGDF_ASSERT( 0 < numberOfBasicGraphs && numberOfBasicGraphs < 32 );
	randomSimpleGraph(*m_G, numberOfNodes, numberOfEdges);
	randomESG(numberOfBasicGraphs);

}//end createRandom


} // end namespace ogdf
