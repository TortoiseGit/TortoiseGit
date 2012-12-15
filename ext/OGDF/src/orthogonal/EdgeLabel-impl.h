/*
 * $Archive: /ogdl/ogdf/EdgeLabel.cpp $
 * $Revision: 2572 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 17:32:30 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Class EdgeLabel handles edge label position computation.
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


//#define foutput

//#if !defined(_MSC_VER) && !defined(__BORLANDC__)
//#include <ogdf/labeling/EdgeLabel.h>
//#endif


const int coordBound = 1000000;

//To be done: erzeuge fuer jeden Kandidaten einen Eintrag in m_candList,
//speichere Ueberlappungen durch Zeigerliste oder Listiterator?


namespace ogdf {

	//due to the visio interface we cant initialize all members
	//here, aswe dont have a layout yet
	template <class coordType>
	ELabelPos<coordType>::ELabelPos()
	{
		//initialize all members, check which labels are to be assigned
		//m_assigned is set to false for all input labels and will be set
		//to true if a position is assigned

		m_candStyle = 1;
		m_placeHeuristic = 1;
		m_posNum = 7; //(3) maybe for segment or whole length (4 candidates)
		m_endInsertion = true;

		m_countFeatureIntersect = false;

		m_ug = 0;
		m_prup = 0;

		m_segMargin = 0;

	}//eLabelPos


	template <class coordType>
	void ELabelPos<coordType>::init(PlanRepUML& pru,
		GridLayoutMapped& L,
		ELabelInterface<coordType>& eli)
	{

		m_poly.init(pru.original());
		m_segInfo.init(pru.original());
		m_featureInfo.init(pru.original());
		m_edgeLength.init(pru.original());

		m_eli = &eli;
		m_gl = &L;
		m_prup = &pru;

		m_ug = 0;

		m_defaultDistance = eli.distDefault();

		m_segMargin = 1;

		int i;
		for (i = 0; i < labelNum; i++)
		{
			m_assigned[i].init(pru.original(), false);

			m_candPosList[i].init(pru.original());
			m_candList[i].init(pru.original());

			m_distance[i].init(pru.original(), m_defaultDistance);
			m_intersect[i].init(pru.original());
		}//for


	}//init

	template <class coordType>
	void ELabelPos<coordType>::call(PlanRepUML& pru,
		GridLayoutMapped& L,
		ELabelInterface<coordType>& eli) //int
	{

		init(pru, L, eli);

		OGDF_ASSERT(m_prup != 0);

		edge e;

		//initialize
		initFeatureRectangles(); //grab the node size and position
		initSegments();  //grab the segment position and number

		//start computation
		computeCandidates();
		testFeatureIntersect();

		//if there is only one candidate for an edge,
		//we don't need to choose, but keep it for testing intersect.

		//build a data structure to decide feasible positioning
		//      initStructure();

		//we don't need to delete in the list, only update match structure
		testAllIntersect();

		//now we know (in m_intersect) which labels intersect each other
		//and have to choose the best candidates
		//heuristic: first, cope with the labels having no candidates
		//without intersection, for them choose a candidate
		//having a) minimum intersection number
		//       b) intersection with labels having as many good
		//          candidates as possible
		//should be collision-free!
		//After that, check the remaining edges, choose a non-intersecting
		//candidate (maybe having highest feature-distance)

		//check all edges in current CC
		EdgeArray<bool> checked(m_prup->original(), false);

		node v;
		forall_nodes(v, *m_prup)
			//forall_edges(e, m_prup->original())
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{
				//e = m_prup->original(e1);
				//if (!e) continue;
				if (checked[e]) continue;

				checked[e] = true;

				//run over all label positions, high constant...
				int i;
				for (i = 0; i < labelNum; i++)
				{
					bool intersectFree = false;
					int minIntersect = coordBound;
					int minIndex = 0; //index of minimum intersection numbers
					for (int j=0; j < m_candPosList[i][e].size(); j++)
					{
						int intersections = (*m_intersect[i][e].get(j)).size();
						if (intersections > 0)
						{
							if (intersections < minIntersect) minIndex = j;
						}//if problem
						else intersectFree = true;
					}//for candidates
					//first round: only assign problematic labels
					if ((m_candPosList[i][e].size() > 0) && !intersectFree)
					{
						eli.getLabel(e).setX((eLabelTyp)i, (*m_candPosList[i][e].get(minIndex)).m_x);
						eli.getLabel(e).setY((eLabelTyp)i, (*m_candPosList[i][e].get(minIndex)).m_y);

						m_assigned[i][e] = true;
					}//if
					else //delete all intersecting candidates
					{
						ListIterator< GenericPoint<coordType> > l_it = m_candPosList[i][e].begin();
						int k = 0;

						while (l_it.valid())
						{
							int intersections = (*m_intersect[i][e].get(k)).size();

							if (intersections > 0)
							{
								ListIterator< GenericPoint<coordType> > l_dummy = l_it;
								l_it++;
								m_candPosList[i][e].del(l_dummy);
								//l_it++;
							}//if problem

							if (l_it.valid()) l_it++;
							k++;
						}//while

					}//else
				}//for labeltypes

			}
		}//forallnodes //edges

		//now we have to work on the labels without any good or
		//bad candidates (all were intersecting original nodes)

		//after having assigned all problematic labels,
		//we have to cope with the remaining labels / edges
		//check all edges in current CC
		//EdgeArray<bool>
		checked.init(m_prup->original(), false);

		//node v;
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{

				if (checked[e]) continue;

				checked[e] = true;

				//run over all label positions, high constant...
				int i;
				for (i = 0; i < labelNum; i++)
				{
					//hier sollte  man einen bestimmten (z.B. Mitte) aus allen intersectfree waehlen
					if (!(m_assigned[i])[e]) //assign intersection free
					{
						//after preprocessing, all should have an entry, but preliminary
						if (m_candPosList[i][e].size() > 0)
						{
							int preferredIndex = int(floor(double(m_candPosList[i][e].size())/2));//2.0
							//check: labelnum <4 => ln-2 <=1 should not happen
							if ((i == 0) || (i == 1)) preferredIndex = 0;
							if ( ((i == labelNum-2) || (i == labelNum-1))  && (m_candPosList[i][e].size() > 0) )
								preferredIndex = m_candPosList[i][e].size() - 1;
							eli.getLabel(e).setX((eLabelTyp)i,
								(*m_candPosList[i][e].get(preferredIndex)).m_x);
							eli.getLabel(e).setY((eLabelTyp)i,
								(*m_candPosList[i][e].get(preferredIndex)).m_y);

							m_assigned[i][e] = true;
						}//if
					}//if
				}//for

			}//foralladjedges
		}//forallnodes
#ifdef foutput
		writeGML("result.gml", opResult);
#endif

	}//call


	//forall edges and all label types, candidate placement positions
	//are inserted in m_candPosList
	template <class coordType>
	void ELabelPos<coordType>::computeCandidates()
	{

#ifdef foutput
		//ofstream fout("candpos.txt",ios::app);
#endif
		edge e;
		//check all edges in current CC
		EdgeArray<bool> checked(m_prup->original(), false);

		node v;
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{
				//e = m_prup->original(e1);
				//if (!e) continue;
				if (checked[e]) continue;

				checked[e] = true;

				//different heuristics
				switch (m_candStyle)
				{
					//standard method, just split edge length
				case 1 :
					{
						int labelIndex; //the label index, 0 = end1,...

						//alle Endlabel (e1/m1, e2/m2) werden moeglichst paarweise gegenueberliegend
						//angeordnet, dafuer wird der Platz dann ungleichmaessig verteilt (aber nach welchem Schluessel?)
						//ausserdem koennen sich die Bereiche fuer Typen ueberlappen

						//hier muessen noch abstaende von original graph features hinzu
						//its not clear yet if posNum is length or number
						//labelNum is the OGDL-wide number of possible label types
						//defined in ELabelInterface.h
						//m_posNum is number of pos. cand. for candstyle 1, like ppinch
						//distancestep defines the distance between consecutive label
						//candidates
						double distanceStep = m_edgeLength[e] / (labelNum*m_posNum);
						int distance = 0; //running distance from start //+mindist
						coordType standardDist = max(m_eli->minFeatDist(),
							coordType(floor(double(distance))) );
						//int currentLengthPos = 0; //label pos. relative to edge length
						int segIndex = 0; //segment index = index of starting point in m_poly
						int currentPos = 1; //number of label positions
						SegmentInfo si = (*m_segInfo[e].get(segIndex));
						int currentEnd = (int)si.length;

						int segStartX = (int)((*m_poly[e].get(segIndex)).m_x);
						int segStartY = (int)((*m_poly[e].get(segIndex)).m_y);
						//all labeltypes*the number of cand. per labeltype
						while (currentPos <= labelNum*m_posNum)
						{
#ifdef foutput
							//fout<<"  segment:\n";
							//fout<<"     dir: "<<si.direction<<"\n";
#endif
							int segStartLabel = currentPos-1;
							//stay within current segment
							//while ((currentPos*standardDist < currentEnd)
							while ((distance < currentEnd)
								&& (currentPos <= labelNum*m_posNum))
							{
								//we process the first segment for end1/mult1
								//and the last for end2/mult2
								//derive the correct label type
								labelIndex = int(floor( (double)(currentPos-1) / m_posNum ));

								//derive info about current label
								int aktLabelWidth = (int)(m_eli->getWidth(e, (eLabelTyp)labelIndex));
								int aktLabelHeight = (int)(m_eli->getHeight(e, (eLabelTyp)labelIndex));
								int alwHalf = int(ceil((double)(aktLabelWidth) / 2));
								int alhHalf = int(ceil((double)(aktLabelHeight) / 2));
								//derive symmetry label size
								int symmLabel = 0;
								int symmW = 0, symmH = 0;
								switch (labelIndex)
								{
								case 0: symmLabel = 1; break;
								case 1: symmLabel = 0; break;
								case labelNum-2: symmLabel = labelNum-1; break;
								case labelNum-1: symmLabel = labelNum-2; break;
								}//switch
								symmW = int(ceil( (double)(m_eli->getWidth(e, (eLabelTyp)symmLabel) ) / 2));
								symmH = int(ceil( (double)(m_eli->getHeight(e, (eLabelTyp)symmLabel) ) / 2));
#ifdef foutput
								//fout<<"     labelIndex: "<<labelIndex<<" Typ: "<<(eLabelTyp)labelIndex<<"\n";
#endif
								GenericPoint<coordType> candidate;
								GenericPoint<coordType> symmCandidate; //end label on the opposite edge side
								//now assign positions according to segment direction
								//endlabels are shifted depending on direction and type/distance
								//in inner switch statement
								//ausserdem einfache Tests
								//necessary because left lower corner needs labelsize shift
								//in some cases, but they are difficult to check later
								//hier sollte man auch doppelpos. fuer Namen einfuehren und dabei m_distance zulassen
								switch (si.direction)
								{
								case odWest:  //horizontal west
									{
										candidate.m_x = segStartX - int(floor((double)((currentPos - segStartLabel))*distanceStep));
										//(currentPos - segStartLabel)*standardDist;
										candidate.m_y = segStartY;

										switch (labelIndex)
										{
										case 0: symmCandidate.m_y = candidate.m_y -
													(m_distance[symmLabel][e] + symmH);
											candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										case 1: symmCandidate.m_y = candidate.m_y +
													(m_distance[symmLabel][e] + symmH);
											candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										case labelNum-2: symmCandidate.m_y = candidate.m_y -
															 (m_distance[symmLabel][e] + symmH);
											candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										case labelNum-1: symmCandidate.m_y = candidate.m_y +
															 (m_distance[symmLabel][e] + symmH);
											candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										}//switch
										break;
									}//odWest
								case odNorth:
									{
										candidate.m_x = segStartX;
										candidate.m_y = segStartY + int(floor((currentPos - segStartLabel)*distanceStep));
										//	(currentPos - segStartLabel)*standardDist;

										switch (labelIndex)
										{
										case 0: symmCandidate.m_x = candidate.m_x +
													m_distance[symmLabel][e] + symmW;
											candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										case 1: symmCandidate.m_x = candidate.m_x -
													(m_distance[symmLabel][e] + symmW);
											candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										case labelNum-2: symmCandidate.m_x = candidate.m_x +
															 (m_distance[symmLabel][e] + symmW);
											candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										case labelNum-1: symmCandidate.m_x = candidate.m_x -
															 (m_distance[symmLabel][e] + symmW);
											candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										}//switch
										break;
									}//odNorth
								case odEast:  //horizontal
									{
										candidate.m_x = segStartX + int(floor((currentPos - segStartLabel)*distanceStep));
										//+ (currentPos - segStartLabel)*standardDist;
										candidate.m_y = segStartY;

										switch (labelIndex)
										{
										case 0: symmCandidate.m_y = candidate.m_y -
													(m_distance[symmLabel][e] + symmH);
											candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										case 1: symmCandidate.m_y = candidate.m_y +
													(m_distance[symmLabel][e] + symmH);
											candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										case labelNum-2: symmCandidate.m_y = candidate.m_y -
															 (m_distance[symmLabel][e] + symmH);
											candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										case labelNum-1: symmCandidate.m_y = candidate.m_y +
															 (m_distance[symmLabel][e] + symmH);
											candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
											symmCandidate.m_x = candidate.m_x;
											break;
										}//switch
										break;
									}//odEast
								case odSouth:
									{
										candidate.m_x = segStartX;
										candidate.m_y = segStartY - int(floor((currentPos - segStartLabel)*distanceStep));
										//(currentPos - segStartLabel)*standardDist;

										switch (labelIndex)
										{
										case 0: symmCandidate.m_x = candidate.m_x +
													m_distance[symmLabel][e] + symmW;
											candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										case 1: symmCandidate.m_x = candidate.m_x -
													(m_distance[symmLabel][e] + symmW);
											candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										case labelNum-2: symmCandidate.m_x = candidate.m_x +
															 m_distance[symmLabel][e] + symmW;
											candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										case labelNum-1: symmCandidate.m_x = candidate.m_x -
															 (m_distance[symmLabel][e] + symmW);
											candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
											symmCandidate.m_y = candidate.m_y;
											break;
										}//switch
										break;
									}//odSouth
								default: OGDF_THROW_PARAM(AlgorithmFailureException, afcLabel);
								}//switch

								currentPos++;
								distance = int(floor(currentPos*distanceStep));
								List<LabelInfo> l_dummy;
								m_candPosList[labelIndex][e].pushBack(candidate);
								//will be replaced by
								m_candList[labelIndex][e].pushBack(PosInfo(e, (eLabelTyp)labelIndex, candidate));
								m_intersect[labelIndex][e].pushBack(l_dummy);
								//end labels can be on opposite edge sides
								if ( (labelIndex == 0) ||
									(labelIndex == 1) ||
									(labelIndex == labelNum-2) ||
									(labelIndex == labelNum-1)
									)
								{
									m_candPosList[symmLabel][e].pushBack(symmCandidate);
									//will be replaced by
									m_candList[symmLabel][e].pushBack(PosInfo(e, (eLabelTyp)symmLabel, symmCandidate));
									m_intersect[symmLabel][e].pushBack(l_dummy);
								}

							}//while

							//jump to new segment
							segIndex++;

							if (segIndex < m_segInfo[e].size())
							{
								si = (*m_segInfo[e].get(segIndex));
								segStartX = (int)(*m_poly[e].get(segIndex)).m_x;
								segStartY = (int)(*m_poly[e].get(segIndex)).m_y;
								currentEnd += (int)si.length;
							}//if
							else break;
						}//while
						//for (labelIndex = 0; labelIndex < labelNum; labelIndex++)
						//no we have processed all edge segments

						break;
					}//case
				default : break;
				}//switch
				int forI;
				for(forI = 0; forI < labelNum; forI++)
				{
					OGDF_ASSERT(m_candPosList[forI][e].size() > 0);
				}//For
			}//foralladjedges
		}//forallnodes
#ifdef foutput
		writeGML();
#endif

	}//computeCandidates


	//build up intersection structure
	template <class coordType>
	void ELabelPos<coordType>::initStructure()
	{
		//simple heuristic:
		//pair (edge, labeltyp) has node with neighbours for all candidates
		//intersecting candidates are connected
		edge e;
		node v;

		//back link graph nodes and positions
		NodeArray<LabelInfo> l_labelInfo(m_intersectGraph);
		//forward link
		//link pair (edge, labeltyp) to node in intersectgraph
		EdgeArray<node> l_pairNode[labelNum];
		//use same order in candidate list to navigate in l_posNode
		EdgeArray< List<node> > l_posNode[labelNum];
		for (int i = 0; i < labelNum; i++)
		{
			l_pairNode[i].init(m_prup->original());
			l_posNode[i].init(m_prup->original());
		}//for

		//insert nodes in m_intersectGraph for all pairs
		//check all edges in current CC
		EdgeArray<bool> checked(m_prup->original(), false);

		//node v;
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{
				//e = m_prup->original(e1);
				//if (!e) continue;
				if (checked[e]) continue;

				checked[e] = true;

				//      forall_edges(e, m_prup->original())
				//      {
				for (int i = 0; i < labelNum; i++)
				{
					v = m_intersectGraph.newNode();

					LabelInfo li;
					li.m_e = e; li.m_labelTyp = i;
					l_labelInfo[v] = li;

					l_pairNode[i][e] = v;

					//insert nodes for the candidate positions for all pairs
					//and connect them to the pairs representant
					List< GenericPoint<coordType> >& l_posList = posList(e,i); //all candidates for label Nr. i

					ListIterator< GenericPoint<coordType> > it = l_posList.begin();
					//int candIndex = 0;
					while (it.valid())
					{
						node w = m_intersectGraph.newNode();
						l_posNode[i][e].pushBack(w);

						//candIndex++;
						it++;
					}//while
				}//for
			}//foralladjedges
		}//forallnodes

		//now connect intersecting label representants
		forall_edges(e, m_prup->original())
		{
			for (int i = 0; i < labelNum; i++)
			{
			}//for
		}//forall_edges

	}//initStructure


	//check candidate positions for feature intersection
	//intersecting cp are deleted from the list, but saved in a pool
	//for possible assignment to left over edges
	template <class coordType>
	void ELabelPos<coordType>::testFeatureIntersect()
	{

		//labeltree not implemented yet, just check with all graph features
		//initialize label structure

		//assure spare candidate
		EdgeArray< GenericPoint<coordType> > l_saveCandidate[labelNum];
		for (int k = 0; k < labelNum; k++) l_saveCandidate[k].init(m_prup->original());
		saveRecovery(l_saveCandidate);

		edge e;
		//for all edge labels and their cand. pos., we check intersections
		//with original graph features
		//check all edges in current CC
		EdgeArray<bool> checked(m_prup->original(), false);

		node v;
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{
				//e = m_prup->original(e1);
				//if (!e) continue;
				if (checked[e]) continue;

				checked[e] = true;

				//      forall_edges(e, m_prup->original())
				//      {
				int i;
				for (i = 0; i < labelNum; i++)
				{
					//hier und beim berechnen der Kandidaten noch aendern:
					//man braucht ja echte Labelposition und die Groesse, wichtig
					//auch, da man die Richtung des Labels wissen muss
					//insert labels into structure
					List< GenericPoint<coordType> >& l_posList = posList(e,i); //all candidates for label Nr. i
					//will be replaced by
					List<PosInfo>& l_candList = candList(e, i);

					//check intersection with original graph nodes:

					//simple version one after another
					ListIterator< GenericPoint<coordType> > it = l_posList.begin();
					while (it.valid())
					{

						//label size and position
						int minXLabel = (int)((*it).m_x - (m_eli->getWidth(e, (eLabelTyp)i) / 2));
						int maxXLabel = (int)((*it).m_x + (m_eli->getWidth(e, (eLabelTyp)i) / 2));
						int minYLabel = (int)((*it).m_y - (m_eli->getHeight(e, (eLabelTyp)i) / 2));
						int maxYLabel = (int)((*it).m_y + (m_eli->getHeight(e, (eLabelTyp)i) / 2));

						node w;
						//simple version: check with all original node rectangles
						bool intersect = false;
						forall_nodes(w, m_prup->original())
						{
							FeatureInfo& fi = m_featureInfo[w];

							intersect = ( (minXLabel <= fi.max_x) &&
								(maxXLabel >= fi.min_x) &&
								(minYLabel <= fi.max_y) &&
								(maxYLabel >= fi.min_y) );

							if (intersect) break;

						}//forall_nodes
						if (intersect)
						{
							ListIterator< GenericPoint<coordType> > itX = it;
							it++;
							l_posList.del(itX);

							continue;
						}

						it++;
					}//while

					//now check if there is a proper candidate left
					if (l_posList.empty())
					{
						//assure proper label position assignment
						//search a new position and give edge highest priority
						l_posList.pushBack(l_saveCandidate[i][e]);
					}//if

					//if there is only one cand, we don't need to decide, but there
					//may be no intersection (test only for secu. testing)

				}//for
			}//foralladjedges
		}//forallnodes
#ifdef foutput
		writeGML("label2.gml");
#endif
	}//testfeatureintersect


	template <class coordType>
	void ELabelPos<coordType>::saveRecovery(EdgeArray< GenericPoint<coordType> > (&saveCandidate)[labelNum])
	{
		//a labeltype - dependant extra candidate for label positioning should
		//be defined here to help in case of empty cand.list after featureintersect

		//simple version: use one (middle?) of the original candidates

		edge e;
		//check all edges in current CC
		EdgeArray<bool> checked(m_prup->original(), false);

		node v;
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{
				//e = m_prup->original(e1);
				//if (!e) continue;
				if (checked[e]) continue;

				checked[e] = true;
				//forall_edges(e, mapGraph)
				//{
				int i;
				for (i = 0; i < labelNum; i++)
				{
					List< GenericPoint<coordType> >& l_posList = posList(e,i); //all candidates for label Nr. i
					//hier: sichere, das Eintrag vorhanden
					saveCandidate[i][e] = (*l_posList.get(int(floor((double)(l_posList.size())/2))));
				}//for labeltypes
			}//foralladjedges
		}//forallnodes

	}//saveRecovery


	//check label positions for label/label intersection
	//don't consider same edge label/label intersection
	template <class coordType>
	void ELabelPos<coordType>::testAllIntersect()
	{
		List<FeatureLink> l_featureList;
		//as long as labeltree is not implemented, use a worst case quadratic
		//test better than round robin
		edge e;

		//for all edge labels and their cand. pos., we check intersections
		//with other label positions
		//insert candidates in list
		//check all edges in current CC
		EdgeArray<bool> checked(m_prup->original(), false);

		node v;
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{
				//e = m_prup->original(e1);
				//if (!e) continue;
				if (checked[e]) continue;

				checked[e] = true;

				//      forall_edges(e, m_prup->original())
				//      {
				int i;
				for (i = 0; i < labelNum; i++)
				{
					//insert labels into structure
					List< GenericPoint<coordType> >& l_posList = posList(e,i); //all candidates for label Nr. i

					//simple version one after another
					int index = 0;
					ListIterator< GenericPoint<coordType> > it = l_posList.begin();
					while (it.valid())
					{

						//label size and position
						FeatureInfo fi;
						fi.size_x = m_eli->getWidth(e, (eLabelTyp)i);
						fi.size_y = m_eli->getHeight(e, (eLabelTyp)i);
						fi.min_x = (*it).m_x - int(ceil((fi.size_x / 2)));
						fi.max_x = (*it).m_x + int(ceil((fi.size_x / 2)));
						fi.min_y = (*it).m_y - int(ceil((fi.size_y / 2)));
						fi.max_y = (*it).m_y + int(ceil((fi.size_y / 2)));

						//hier muss noch der Knoten hin
						l_featureList.pushBack(FeatureLink(e, (eLabelTyp)i, 0, fi, index));

						index++;
						it++;
					}//while


				}//for

			}//foralladjedges

		}//forallnodes
		//*****************************************
		//now check
		//*********************************************

		//Sort the label positions and perform sweepline run
		FeatureComparer fc;
		l_featureList.quicksort(fc);

		//run over all candidate positions and check intersections
		//objects are active, if their y-intervall encloses the current
		//sweepline position

		FeatureLink runFI; //next label position to be processed
		ListIterator<FeatureLink> l_it = l_featureList.begin();

		List<FeatureLink> sweepLine;

		int aktY = -coordBound - 1; //current y-coordinate of sweepline

		//run over all inserted label positions
		while (l_it.valid())
		{

			OGDF_ASSERT((*l_it).m_fi.min_y > aktY);

			aktY = (int)((*l_it).m_fi.min_y);

			ListIterator<FeatureLink> itSweep = sweepLine.begin(); //sweepline iterator

			//loesche alle abgelaufenen events
			while (itSweep.valid())
			{
				if ((*itSweep).m_fi.max_y < aktY)
				{
					ListIterator<FeatureLink> itNext = itSweep.succ();
					sweepLine.del(itSweep);
					itSweep = itNext;
					continue;
				}
				itSweep++;
			}//while

			itSweep = sweepLine.begin();
			//now we have only active events left, with y-intervall
			//including current sweepline-position

			//run over all starting objects
			while (l_it.valid() && (aktY == (*l_it).m_fi.min_y))
			{
				itSweep = sweepLine.begin();
				//run over sweepline to find position and check intersection
				//sort sweepline by increasing min_x-value
				FeatureLink& fl = (*l_it);

				int aktX = (int)((*l_it).m_fi.min_x);

				//skip/test objects left of l_it
				while (itSweep.valid() && (aktX > (*itSweep).m_fi.min_x) )
				{
					//test on intersection
					if (aktX <= (*itSweep).m_fi.max_x)
					{
						FeatureLink& fls = (*itSweep);
						//intersecting another position for the same label is ok
						if (!( (fl.m_elt == fls.m_elt) && (fl.m_edge == fls.m_edge) ))
							(*m_intersect[fl.m_elt][fl.m_edge].get(fl.m_index)).pushBack(
							LabelInfo(fls.m_edge, fls.m_elt, fls.m_index));
					}
					itSweep++;
				}//while left
				//sweepline empty or object on same pos. / to the right found
				//insert into sweepline: end or after last
				if (!itSweep.valid())
				{
					sweepLine.pushBack((*l_it));
				}//if rightmost entry
				else
				{
					sweepLine.insertBefore((*l_it), itSweep);
					/*
					//now check for intersection until min_x > l_it.max_x
					while ( itSweep.valid() && ((*itSweep).m_fi.min_x <= (*l_it).m_fi.max_x) )
					{
					//hier muss man noch abfragen, ob es nicht die gleiche (e,typ) Sorte ist
					FeatureLink& fls = (*itSweep);
					(*m_intersect[fl.m_elt][fl.m_edge].get(fl.m_index)).pushBack(
					LabelInfo(fls.m_edge, fls.m_elt, fls.m_index));
					itSweep++;
					}//while*/
				}//else insert before
				//now check for intersection until min_x > l_it.max_x
				while ( itSweep.valid() && ((*itSweep).m_fi.min_x <= (*l_it).m_fi.max_x) )
				{
					FeatureLink& fls = (*itSweep);
					//intersecting another position for the same label is ok
					if (!( (fl.m_elt == fls.m_elt) && (fl.m_edge == fls.m_edge) ))
						(*m_intersect[fl.m_elt][fl.m_edge].get(fl.m_index)).pushBack(
						LabelInfo(fls.m_edge, fls.m_elt, fls.m_index));
					itSweep++;
				}//while
				l_it++;
			}//while starting

			//l_it++;
		}//while l_it


		//******************************************
		//now check if there is a proper candidate left
		/*
		if (l_posList.empty())
		{
		//assure proper label position assignment
		//search a new position and give edge highest priority

		}//if
		*/
		//if there is only one cand, we don't need to decide, but there
		//may be no intersection (test only for secu. testing
		//check end
		//*****************************************

#ifdef foutput
		writeGML("label3.gml", opOmitIntersect);
#endif

	}//testAllIntersect


	template <class coordType>
	void ELabelPos<coordType>::initSegments()
	{
		//we count the number of segments and their position/length
		edge e;
		//check all edges in current CC
		EdgeArray<bool> checked(m_prup->original(), false);

		node v;
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->original(v);
			if (!w) continue;

			forall_adj_edges(e, w)
			{
				//e = m_prup->original(e1);
				//if (!e) continue;
				if (checked[e]) continue;

				checked[e] = true;

				//      forall_edges(e, m_prup->original())
				//      {
				edge ec;
				int crossCount = 0;
				int bendCount = 0;
				int length = 0;

				if (m_prup->chain(e).size() < 1) continue; //dont count
				//non-existing edges from other connected components

				edge et = m_prup->copy(e); //chain(e).front

				//fill the m_poly polyline for original e
				m_poly[e].pushBack( GenericPoint<coordType>(m_gl->x(et->source()), m_gl->y(et->source())));

				//count all inner nodes as crossings or bends
				ListConstIterator<edge> l_it;


				int currNum = 1;
				for(l_it = m_prup->chain(e).begin(); l_it.valid(); ++l_it)
				{
					SegmentInfo si;

					ec = *l_it;
					GenericPoint<coordType> ip1(m_gl->x(ec->source()), m_gl->y(ec->source()));
					GenericPoint<coordType> ip2(m_gl->x(ec->target()), m_gl->y(ec->target()));
					m_poly[e].pushBack(ip2);
					int len = max(abs(m_gl->x(ec->target()) - m_gl->x(ec->source()) ),
						abs(m_gl->y(ec->target())- m_gl->y(ec->source()) ) );
					//set segmentinfo values
					si.length = len;
					si.number = currNum;

					OrthoDir od;
					bool isHor = (ip1.m_y == ip2.m_y); //may still be same place
					if (isHor)
					{
						if (ip1.m_x > ip2.m_x) od = odWest;
						else od = odEast;
					}//if isHor
					else
					{
						//check m_x == m_x
						if (ip1.m_y < ip2.m_y) od = odNorth;
						else od = odSouth;
					}//else
					si.direction = od;

					m_segInfo[e].pushBack(si);

					//set overall edge length
					length+= len;

					//dont count last edge
					if ( l_it == m_prup->chain(e).rbegin() ) continue;
					node nextV = ec->target();//depends on chain direction
					if (nextV->degree()==4) crossCount++;
					else bendCount++;

					currNum++;
				}//forall copy edges in list
				m_edgeLength[e] = length;

			}//foralladjedges
		}//forall nodes //edges in pru

	}//initSegments


	template <class coordType>
	void ELabelPos<coordType>::initFeatureRectangles()
	{
		node v;
		//forall original nodes: get cage size and position
		//if applied to result graph and layout, we dont need this
		NodeArray<bool> checked(m_prup->original(), false);
		forall_nodes(v, *m_prup)
		{
			node w = m_prup->expandedNode(v);
			if (w != 0)
			{
				int aktX = m_gl->x(v);
				int aktY = m_gl->y(v);
				node wo = m_prup->original(w);
				if (wo)
				{
					if (checked[wo]) //already processed node
					{
						if (aktX > m_featureInfo[wo].max_x)
							m_featureInfo[wo].max_x = aktX;
						if (aktY > m_featureInfo[wo].max_y)
							m_featureInfo[wo].max_y = aktY;
						if (aktX < m_featureInfo[wo].min_x)
							m_featureInfo[wo].min_x = aktX;
						if (aktY < m_featureInfo[wo].min_y)
							m_featureInfo[wo].min_y = aktY;
						m_featureInfo[wo].size_x = m_featureInfo[wo].max_x - m_featureInfo[wo].min_x;
						m_featureInfo[wo].size_y = m_featureInfo[wo].max_y - m_featureInfo[wo].min_y;
					}//if
					else //new node
					{
						FeatureInfo fi;
						fi.min_x = fi.max_x = aktX;
						fi.min_y = fi.max_y = aktY;
						fi.size_x = fi.size_y = 0;

						m_featureInfo[m_prup->original(w)] = fi;

						checked[m_prup->original(w)] = true;

					}//else
				}//if original
			}//if
		}//forallnodes

		//now all expanded (original) nodes have a featureinfo

	}//initFeatureRectangles



	//*************************************************************************
	//AttributedGraph-call section
	template <class coordType>
	void ELabelPos<coordType>::call(GraphAttributes& ug, ELabelInterface<coordType>& eli) //double
	{
		initUML(ug, eli);

		OGDF_ASSERT(m_ug != 0);

		edge e;
		//initialize
		initUMLFeatureRectangles(); //grab the node size and position
		initUMLSegments();  //grab the segment position and number

		//start computation
		computeUMLCandidates();//compute possible positions

		testUMLFeatureIntersect();//check for graph object intersection

		//if there is only one candidate for an edge,
		//we don't need to choose, but keep it for testing intersect.

		//build a data structure to decide feasible positioning
		//initStructure();

		testUMLAllIntersect();//now check for label intersection

		//************************************************************
		//now we know (in m_intersect) which labels intersect each other
		//and have to choose the best candidates
		//heuristics:
		//a)
		//first, cope with the labels having no candidates
		//without intersection, for them choose a candidate
		//having a) minimum intersection number
		//       b) intersection with labels having as many good
		//          candidates as possible
		//should be collision-free!
		//After that, check the remaining edges, choose a non-intersecting
		//candidate (maybe having highest feature-distance)

		//b)
		//first assign intersection-free candidates and update the status
		//of all other label candidates (possibly creating new intersection
		//free candidates)

		//c) store all candidates in a heap with assigned costs and assign
		//them in output order


		//dynamically set m_active to csAssigned for all candidates of labels
		//that get a position assignment which are not assigned themselves

		m_placeHeuristic = 2;

		//c)
		if (m_placeHeuristic == 2)
		{
			int assignmentCount = 0;
			edge e;
			forall_edges(e, m_ug->constGraph())
			{
				EdgeLabel<coordType>& l_el = eli.getLabel(e);
				//run over all label positions
				int i;
				for (i = 0; i < labelNum; i++)
				{
					if (!l_el.usedLabel((eLabelTyp)i)) continue;

					//run over all candidates and compare their intersection numbers
					ListIterator< PosInfo > l_it = candList(e, i).begin();
					while (l_it.valid())
					{
						double* cost = &((*l_it).m_cost);
						PosInfo* myPos = &(*l_it);
						int* update = &myPos->m_posIndex;
						m_candidateHeap.insert(myPos, *cost, update);

						l_it++;
					}//while candidates
				}//for
			}//foralledges

			//*****************************************
			//now assign labels by extracting from heap

			while (!m_candidateHeap.empty())
			{
				PosInfo* nextPos = m_candidateHeap.extractMin();
				while ( m_assigned[nextPos->m_typ][nextPos->m_edge]  )
				{
					if (m_candidateHeap.empty())
					{
						nextPos = 0;
						break;
					}//if
					nextPos = m_candidateHeap.extractMin();
				}//while

				//now nextPos holds the next unassigned PosInfo
				if (nextPos == 0) break;

				edge e = nextPos->m_edge;
				eLabelTyp elt = nextPos->m_typ;
				//we use costs
				//OGDF_ASSERT(nextPos->m_active == csActive);

				//assign the candidate
				eli.getLabel(e).setX(elt, nextPos->m_coord.m_x);
				eli.getLabel(e).setY(elt, nextPos->m_coord.m_y);
				m_assigned[elt][e] = true;

				nextPos->m_active = csUsed; //is the only used candidate for this pair (e, elt)
				assignmentCount++;

				//now update all unused candidates
				//unused candidates get status csAssigned,
				//their intersections are updated (priority) to find new minimal intersection candidates
				List < PosInfo >& l_cands = candList(e, elt);
				ListIterator< PosInfo > l_itCands = l_cands.begin();
				while (l_itCands.valid())
				{
					if ( ((*l_itCands).m_active != csUsed) &&
						((*l_itCands).m_active != csFIntersect) //they are not inserted in lists!
						)
					{
						//TODO: teste, ob fuer fintersect auch die anzahl korrigiert wird
						(*l_itCands).m_active = csAssigned;
						List <PosInfo*>& l_sects = (*l_itCands).m_intersect;
						ListIterator<PosInfo*> l_itSects = l_sects.begin();
						while (l_itSects.valid())
						{
							//ATTENTION: we only update the intersection number, not the list!
							(*l_itSects)->m_numActive--;
							OGDF_ASSERT((*l_itSects)->m_numActive >= 0);
							double newCost = m_candidateHeap.getPriority((*l_itSects)->m_posIndex)
								- costLI();
							(*l_itSects)->m_cost = newCost;
							m_candidateHeap.decreaseKey((*l_itSects)->m_posIndex,
								newCost
								);

							l_itSects++;

						}//while

					}//if

					l_itCands++;
				}//while


			}//while candidates

		}//placeheuristic c

		//store intersection free candidates for version a)
		//preliminary till concept expanded
		EdgeArray< List< PosInfo* > > l_freeCand[labelNum];
		for (int k=0; k < labelNum; k++)
		{
			l_freeCand[k].init(m_ug->constGraph());
		}//for


		//a)
		//*******************************************************************
		if (m_placeHeuristic == 0)
		{
			forall_edges(e, m_ug->constGraph())
			{
				EdgeLabel<coordType>& l_el = eli.getLabel(e);
				//run over all label positions
				int i;
				for (i = 0; i < labelNum; i++)
				{
					if (!l_el.usedLabel((eLabelTyp)i)) continue;

					bool intersectFree = false;
					int minIntersect = coordBound;
					PosInfo* minPointer; //PosInfo of cand. with minimum intersection numbers

					//run over all candidates and compare their intersection numbers
					ListIterator< PosInfo > l_it = candList(e, i).begin();
					while (l_it.valid())
					{
						//TODO: hier muss man zwischen csFIntersect und anderen unterscheiden
						int intersections = (*l_it).m_intersect.size();
						if (intersections > 0)
						{
							//TODO: hier auf Anzahl testen ( b) )
							if (intersections < minIntersect) minPointer = &(*l_it);
						}//if problem
						else intersectFree = true;
						l_it++;
					}//while candidates

					//first round: only assign problematic labels
					if ((candList(e, i).size() > 0) && !intersectFree)
					{
						eli.getLabel(e).setX((eLabelTyp)i, minPointer->m_coord.m_x);
						eli.getLabel(e).setY((eLabelTyp)i, minPointer->m_coord.m_y);

						minPointer->m_active = csUsed;

						m_assigned[i][e] = true;
						//TODO: forall other candidates, set status to csassigned
					}//if
					else //delete all intersecting candidates
					{
						ListIterator< PosInfo > l_it = candList(e, i).begin();

						while (l_it.valid())
						{
							int intersections = (*l_it).m_intersect.size();

							if (intersections == 0) l_freeCand[i][e].pushBack(&(*l_it));

							l_it++;
						}//while

					}//else
				}//for labeltypes
			}//foralledges
		}//if heuristics 0
		//**********************************************************

		//b)
		//**********************************************************
		if (m_placeHeuristic == 1)
		{

			int assignmentCount = 0;

			//****************************************************
			//Assigning position candidates to labels
			//m_numAssignment is the counted number of used labels

			while  (assignmentCount < m_numAssignment)
			{
				//**************************************************
				//assign intersection free label position candidates
				while (!m_freeLabels.empty())
				{
					PosInfo* l_front = m_freeLabels.popFrontRet();
					//remove already assigned leading candidates
					//entweder m_assigned oder posinfostatus, da pointer
					while (m_assigned[l_front->m_typ][l_front->m_edge])
					{
						if (m_freeLabels.empty())
						{
							l_front = 0;
							break;
						}//if
						l_front = m_freeLabels.popFrontRet();
					}//while assigned
					//look if there is still something to do
					if (l_front != 0)
					{
						edge e = l_front->m_edge;
						eLabelTyp elt = l_front->m_typ;
						OGDF_ASSERT(l_front->m_active == csActive);

						//assign the candidate
						eli.getLabel(e).setX(elt, l_front->m_coord.m_x);
						eli.getLabel(e).setY(elt, l_front->m_coord.m_y);
						m_assigned[elt][e] = true;
						l_front->m_active = csUsed; //is the only used candidate for this pair (e, elt)
						assignmentCount++;

						//now update all unused candidates
						//unused candidates get status csAssigned,
						//their intersections are updated to find new minimal intersection candidates
						List < PosInfo >& l_cands = candList(e, elt);
						ListIterator< PosInfo > l_itCands = l_cands.begin();
						while (l_itCands.valid())
						{
							if ( ((*l_itCands).m_active != csUsed) &&
								((*l_itCands).m_active != csFIntersect) //they are not inserted in lists!
								)
							{
								//TODO: teste, ob fuer fintersect auch die anzahl korrigiert wird
								(*l_itCands).m_active = csAssigned;
								List <PosInfo*>& l_sects = (*l_itCands).m_intersect;
								ListIterator<PosInfo*> l_itSects = l_sects.begin();
								while (l_itSects.valid())
								{
									//ATTENTION: we only update the intersection number, not the list!!
									// (could use lis::pos(ListIterator))
									(*l_itSects)->m_numActive--;
									//if the label is now free of intersections, insert it in list
									if ( ((*l_itSects)->m_numActive == 0) &&
										((*l_itSects)->m_active != csFIntersect) )
										m_freeLabels.pushBack((*l_itSects));
									l_itSects++;

								}//while

							}//if

							l_itCands++;
						}//while

					}//if valid

				}//while free labels

				//***********************************************
				//assign intersecting candidates if necessary
				//hier jetzt einen Kandidaten auswaehlen, der nicht intersectionfree ist,
				//aber bestimmte Voraussetzungen mitbringt, etwa wenige oder viele
				//Ueberschneidungen
				//choose a good candidate among intersecting label positions
				if (!m_sectLabels.empty())
				{
					PosInfo* l_sect = m_sectLabels.popFrontRet();

					while ( m_freeLabels.empty() && (l_sect != 0) )
					{
						//**************************************
						//check if we havent used this entry yet
						//TODO: dont use csFIntersect
						while ( (l_sect->m_active == csAssigned) ||
							(l_sect->m_active == csUsed) ||
							(l_sect->m_numActive == 0)
							)
						{
							if (m_sectLabels.empty())
							{
								l_sect = 0;
								break;
							}//if
							l_sect = m_sectLabels.popFrontRet();

						}//while invalid entries

						//********************************
						//assign an intersecting candidate
						//TODO: use priority queue and choose "good" entry
						if (l_sect != 0)
						{
							edge e = l_sect->m_edge;
							eLabelTyp elt = l_sect->m_typ;
							OGDF_ASSERT((l_sect->m_active == csActive)
								|| (l_sect->m_active == csFIntersect) //preliminary
								);

							//assign the candidate
							eli.getLabel(e).setX(elt, l_sect->m_coord.m_x);
							eli.getLabel(e).setY(elt, l_sect->m_coord.m_y);
							m_assigned[elt][e] = true;
							l_sect->m_active = csUsed; //is the only used candidate for this pair (e, elt)
							assignmentCount++;

							//now update all intersected candidates and all unused candidates
							//unused candidates get status csAssigned,
							//their intersections are updated to find new minimal intersection candidates
							List < PosInfo >& l_cands = candList(e, elt);
							ListIterator< PosInfo > l_itCands = l_cands.begin();
							while (l_itCands.valid())
							{
								if ( ((*l_itCands).m_active != csUsed) &&
									((*l_itCands).m_active != csFIntersect)
									)
								{
									//TODO: teste, ob fuer fintersect auch die anzahl korrigiert wird
									(*l_itCands).m_active = csAssigned;
									List <PosInfo*>& l_sects = (*l_itCands).m_intersect;
									ListIterator<PosInfo*> l_itSects = l_sects.begin();
									while (l_itSects.valid())
									{
										//ATTENTION: we only update the intersection number, not the list!!
										// (could use lis::pos(ListIterator))
										(*l_itSects)->m_numActive--;
										//if the label is now free of intersections, insert it in list
										if ( ((*l_itSects)->m_numActive == 0) &&
											((*l_itSects)->m_active != csFIntersect) )
											m_freeLabels.pushBack((*l_itSects));
										l_itSects++;

									}//while

								}//if

								l_itCands++;
							}//while
						}//if valid entry found

					}//while no other than intersecting labels
				}//if

				//dann kontrollieren, ob es weder Ueberschneidungsfreie noch geeignete
				//(!csFIntersect) Kandidaten gibt und danach einen mit Knotenueberschneidung
				//waehlen
				//we may have a problem: no more candidates without feature intersection
				if ( m_freeLabels.empty() &&
					m_sectLabels.empty() &&
					(assignmentCount < m_numAssignment)
					)
				{
					//Probeweise: Nochmal durch alle laufen und testen
					//TODO: loeschen
					//************************************************
					edge e;
					forall_edges(e, m_ug->constGraph())
					{
						EdgeLabel<coordType>& l_el = eli.getLabel(e);

						//run over all label positions, high constant...
						int i;
						for (i = 0; i < labelNum; i++)
						{
							if (m_assigned[i][e]) //already assigned
								continue;

							if (!l_el.usedLabel((eLabelTyp)i)) //no input label
								continue;

							List < PosInfo >& l_cands = candList(e, i);

							int preferredIndex = int(floor(l_cands.size()/2.0));

							l_el.setX((eLabelTyp)i,
								(*(l_cands.get(preferredIndex))).m_coord.m_x);
							l_el.setY((eLabelTyp)i,
								(*(l_cands.get(preferredIndex))).m_coord.m_y);

							m_assigned[i][e] = true;
							assignmentCount++;
							//TODO: check every time if intersection free labels are produced


							//TODO: choose good candidate
							/*
							ListIterator< PosInfo > l_itCands = l_cands.begin();
							while (l_itCands.valid())
							{
							OGDF_ASSERT( (*l_itCands).m_active == csFIntersect) );

							//choose a "good" candidate
							//if option m_countFI is set, use feature intersection number

							}//while
							*/
						}//for all types

					}//for all edges

					OGDF_ASSERT(assignmentCount == m_numAssignment);

					//************************************************

				}//if nonFIntersecting label position candidates could be found


			}//while not all
		}//if heuristics 1

		if (m_placeHeuristic == 0)
		{
			//a)
			//now we have to work on the labels without any good or
			//bad candidates (all were intersecting original nodes)

			//after having assigned all problematic labels,
			//we have to cope with the remaining labels / edges
			forall_edges(e, m_ug->constGraph())
			{
				EdgeLabel<coordType>& l_el = eli.getLabel(e);

				//run over all label positions, high constant...
				int i;
				for (i = 0; i < labelNum; i++)
				{

					if (!l_el.usedLabel((eLabelTyp)i)) continue;

					//hier sollte  man einen bestimmten (z.B. Mitte) aus allen intersectfree waehlen
					if (!(m_assigned[i])[e]) //assign intersection free
					{
						//after preprocessing, all should have an entry, but preliminary
						if (l_freeCand[i][e].size() > 0)
						{
							int preferredIndex = int(floor(double(l_freeCand[i][e].size())/2));//2.0
							//check: labelnum <4 => ln-2 <=1 should not happen
							if ((i == 0) || (i == 1)) preferredIndex = 0;
							if ( ((i == labelNum-2) || (i == labelNum-1))  && (l_freeCand[i][e].size() > 0) )
								preferredIndex = l_freeCand[i][e].size() - 1;

							eli.getLabel(e).setX((eLabelTyp)i,
								(*l_freeCand[i][e].get(preferredIndex))->m_coord.m_x);
							eli.getLabel(e).setY((eLabelTyp)i,
								(*l_freeCand[i][e].get(preferredIndex))->m_coord.m_y);

							m_assigned[i][e] = true;
						}//if
					}//if
				}//for

				//OGDF_ASSERT(m_assigned[e]);
			}//foralledges
		}//if heur 0


#ifdef foutput
		writeUMLGML("umlLabelCost.gml");
		writeUMLGML("umlresultUML.gml", opResult);
#endif

	}//call AttributedGraph


	template <class coordType>
	void ELabelPos<coordType>::initUML(GraphAttributes& ug,
		ELabelInterface<coordType>& eli)
	{
		const Graph &G = ug.constGraph();

		m_segInfo.init(G);
		m_featureInfo.init(G);
		m_edgeLength.init(G);
		m_poly.init(G);

		m_eli = &eli;
		m_ug = &ug;

		m_prup = 0;
		m_gl = 0;

		m_defaultDistance = eli.distDefault();

		m_segMargin = 0.001;

		int i;
		for (i = 0; i < labelNum; i++)
		{
			m_assigned[i].init(G, false);
			m_candList[i].init(G); //holds PosInfo for all candidates
			m_distance[i].init(G, m_defaultDistance);
			m_intersect[i].init(G);
		}//for

		m_numAssignment = 0;
		m_endInsertion = true;
		m_endLabelPlacement = true;

		m_posCost = true;
		m_symCost = false;

	}//initUML


	template <class coordType>
	void ELabelPos<coordType>::computeUMLCandidates()
	{

#ifdef foutput
		//ofstream fout("candposUML.txt",ios::app);
#endif
		edge e;
		forall_edges(e, m_ug->constGraph())
		{
#ifdef foutput
			//fout<<"Kante: \n";
#endif
			//different heuristics
			switch (m_candStyle)
			{
				//standard method, just split edge length
			case 1 :
				{
					int labelIndex; //the label index, 0 = end1,...

					//Endlabel (e1/m1, e2/m2) placed pairwise
					//TODO: overlapping regions
					//TODO: distances to original graph features

					//set a distance for equal distribution on the edge length
					coordType standardDist = //max(m_eli->minFeatDist(),  //max/min!!??
						coordType((m_edgeLength[e] / (labelNum*m_posNum + 1)));
					//range for "start" and "end" endlabel
					coordType startRange = m_edgeLength[e] / 3.0;
					coordType endRange = m_edgeLength[e] / 3.0;
					//set a distance for endlabel to end placement
					coordType startDist = (m_endLabelPlacement ? (standardDist / 3.0) : standardDist);
					coordType endDist = (m_endLabelPlacement ? (standardDist / 3.0) : standardDist);

					int segIndex = 0; //segment index = index of starting point in m_poly
					int currentPos = 1; //number of label positions

					//start and end point of current segment (position relative to edge length)
					SegmentInfo si = (*m_segInfo[e].get(segIndex));
					coordType currentStart = 0.0;
					coordType currentEnd = si.length;

					coordType segOfs = 0.0; //offset needed if new segment does not start at k*standardDist

					coordType range = currentPos*standardDist; //range on edge that is already used

					coordType segStartX = (*m_poly[e].get(segIndex)).m_x;
					coordType segStartY = (*m_poly[e].get(segIndex)).m_y;

					//all labeltypes*the number of cand. per labeltype
					while (currentPos <= labelNum*m_posNum) //todo: labelnumber sum
					{
						int segStartLabel = currentPos-1;
						//stay within current segment
						while ((range < currentEnd)
							&& (currentPos <= labelNum*m_posNum))
						{
							//we process the first segment for end1/mult1
							//and the last for end2/mult2
							labelIndex = int(floor( (double)((currentPos-1)) / m_posNum ));

							int optPosDistance = 0;
							if ( (labelIndex == 0) || (labelIndex == 1))
								optPosDistance = currentPos-1;
							else if ( (labelIndex == labelNum-1) || (labelIndex == labelNum-2))
								optPosDistance = labelNum*m_posNum - currentPos;

							OGDF_ASSERT(optPosDistance >= 0);

							//derive info about current label
							coordType aktLabelWidth = m_eli->getWidth(e, (eLabelTyp)labelIndex);
							coordType aktLabelHeight = m_eli->getHeight(e, (eLabelTyp)labelIndex);
							coordType alwHalf = (aktLabelWidth / 2.0);
							coordType alhHalf = (aktLabelHeight / 2.0);

							//derive symmetry label size
							int symmLabel = 0;
							coordType symmW = 0, symmH = 0;

							switch (labelIndex)
							{
							case 0: symmLabel = 1; break;
							case 1: symmLabel = 0; break;
							case labelNum-2: symmLabel = labelNum-1; break;
							case labelNum-1: symmLabel = labelNum-2; break;
							}//switch
							symmW = ( m_eli->getWidth(e, (eLabelTyp)symmLabel) / 2.0);
							symmH = ( m_eli->getHeight(e, (eLabelTyp)symmLabel) / 2.0);
#ifdef foutput
							//fout<<"     labelIndex: "<<labelIndex<<" Typ: "<<(eLabelTyp)labelIndex<<"\n";
#endif

							coordType stepping = (currentPos - segStartLabel)*standardDist;

							GenericPoint<coordType> candidate;
							GenericPoint<coordType> symmCandidate; //end label on the opposite edge side
							//now assign positions according to segment direction
							//endlabels are shifted depending on direction and type/distance
							//in inner switch statement
							//ausserdem einfache Tests
							//necessary because left lower corner needs labelsize shift
							//in some cases, but they are difficult to check later
							//hier sollte man auch doppelpos. fuer Namen einfuehren und dabei m_distance zulassen
							switch (si.direction)
							{
							case odWest:  //horizontal west
								{
									candidate.m_x = segStartX - stepping + segOfs;
									candidate.m_y = segStartY;

									switch (labelIndex)
									{
									case 0: symmCandidate.m_y = candidate.m_y -
												(m_distance[symmLabel][e] + symmH);
										candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									case 1: symmCandidate.m_y = candidate.m_y +
												(m_distance[symmLabel][e] + symmH);
										candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									case labelNum-2: symmCandidate.m_y = candidate.m_y -
														 (m_distance[symmLabel][e] + symmH);
										candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									case labelNum-1: symmCandidate.m_y = candidate.m_y +
														 (m_distance[symmLabel][e] + symmH);
										candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									}//switch
									break;
								}//odWest
							case odNorth:
								{
									candidate.m_x = segStartX;
									candidate.m_y = segStartY + stepping - segOfs;

									switch (labelIndex)
									{
									case 0: symmCandidate.m_x = candidate.m_x +
												m_distance[symmLabel][e] + symmW;
										candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									case 1: symmCandidate.m_x = candidate.m_x -
												(m_distance[symmLabel][e] + symmW);
										candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									case labelNum-2: symmCandidate.m_x = candidate.m_x +
														 (m_distance[symmLabel][e] + symmW);
										candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									case labelNum-1: symmCandidate.m_x = candidate.m_x -
														 (m_distance[symmLabel][e] + symmW);
										candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									}//switch
									break;
								}//odNorth
							case odEast:  //horizontal
								{
									candidate.m_x = segStartX + stepping - segOfs;
									candidate.m_y = segStartY;

									switch (labelIndex)
									{
									case 0: symmCandidate.m_y = candidate.m_y -
												(m_distance[symmLabel][e] + symmH);
										candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									case 1: symmCandidate.m_y = candidate.m_y +
												(m_distance[symmLabel][e] + symmH);
										candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									case labelNum-2: symmCandidate.m_y = candidate.m_y -
														 (m_distance[symmLabel][e] + symmH);
										candidate.m_y += (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									case labelNum-1: symmCandidate.m_y = candidate.m_y +
														 (m_distance[symmLabel][e] + symmH);
										candidate.m_y -= (m_distance[labelIndex][e] + alhHalf);
										symmCandidate.m_x = candidate.m_x;
										break;
									}//switch
									break;
								}//odEast
							case odSouth:
								{
									candidate.m_x = segStartX;
									candidate.m_y = segStartY - stepping + segOfs;

									switch (labelIndex)
									{
									case 0: symmCandidate.m_x = candidate.m_x +
												m_distance[symmLabel][e] + symmW;
										candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									case 1: symmCandidate.m_x = candidate.m_x -
												(m_distance[symmLabel][e] + symmW);
										candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									case labelNum-2: symmCandidate.m_x = candidate.m_x +
														 m_distance[symmLabel][e] + symmW;
										candidate.m_x -= (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									case labelNum-1: symmCandidate.m_x = candidate.m_x -
														 (m_distance[symmLabel][e] + symmW);
										candidate.m_x += (m_distance[labelIndex][e] + alwHalf);
										symmCandidate.m_y = candidate.m_y;
										break;
									}//switch
									break;
								}//odSouth
							default: OGDF_THROW_PARAM(AlgorithmFailureException, afcLabel);
							}//switch

							currentPos++;
							range = currentPos*standardDist;

							List<LabelInfo> l_dummy;

							m_candList[labelIndex][e].pushBack(PosInfo(e,
								(eLabelTyp)labelIndex,
								candidate,
								currentPos-1));
							//add cost for distance to edge end
							PosInfo& thePos = m_candList[labelIndex][e].back();
							if (usePosCost()) thePos.m_cost += optPosDistance*costPos();

							m_intersect[labelIndex][e].pushBack(l_dummy);
							//end labels can be on opposite edge sides
							if ( (labelIndex == 0) ||
								(labelIndex == 1) ||
								(labelIndex == labelNum-2) ||
								(labelIndex == labelNum-1)
								)
							{
								m_candList[symmLabel][e].pushBack(PosInfo(e,
									(eLabelTyp)symmLabel,
									symmCandidate,
									currentPos-1));

								//add cost for distance to edge end
								PosInfo& thePos = m_candList[symmLabel][e].back();
								if (usePosCost()) thePos.m_cost += optPosDistance*costPos();

								m_intersect[symmLabel][e].pushBack(l_dummy);
							}

						}//while

						//jump to new segment
						segIndex++;

						if (segIndex < m_segInfo[e].size())
						{
							si = (*m_segInfo[e].get(segIndex));
							segStartX = (*m_poly[e].get(segIndex)).m_x;
							segStartY = (*m_poly[e].get(segIndex)).m_y;
							currentStart = currentEnd;
							currentEnd += si.length;

							segOfs = standardDist - (range - currentStart);
						}//if
						else break;
					}//while
					//for (labelIndex = 0; labelIndex < labelNum; labelIndex++)
					//no we have processed all edge segments

					break;
				}
			default : break;
			}//switch
		}//forall
#ifdef foutput
		writeUMLGML();
#endif

	}//computeUMLCandidates


	template <class coordType>
	void ELabelPos<coordType>::initUMLSegments()
	{
		//we count the number of segments and their position/length
		edge e;
#ifdef foutput
		//ofstream outf("labelpositionen.txt");
#endif
		forall_edges(e, m_ug->constGraph())
		{
			DPoint dp1, dp2;
#ifdef foutput
			//outf<<"\n\nKante: "<<e->source()<<"->"<<e->target()<<"\n";
#endif
			coordType length = 0;

			OGDF_ASSERT(m_poly[e].size() == 0);

			//bend points do (!!!not) include start and end node

			DPolyline& theBends = m_ug->bends(e);

			OGDF_ASSERT(theBends.size()>1);

			//count all inner nodes as crossings or bends
			ListConstIterator<DPoint> l_it;

			l_it = theBends.begin();
			dp1 = *l_it;
			GenericPoint<coordType> ip2(dp1.m_x, dp1.m_y);
			m_poly[e].pushBack(ip2);
#ifdef foutput
			//outf<<"Startpunkt: "<<ip2.m_x<<"/"<<ip2.m_y<<"\n";
#endif

			int currNum = 1;

			//TODO: Test, ob hier fuer start und endpunkt knotengroesse abgezogen werden muss

			for(l_it = ++l_it; l_it.valid(); l_it++)
			{
				SegmentInfo si;
#ifdef foutput
				//outf<<"Segment Nummer: "<<currNum<<"\n";
#endif
				dp2 = *l_it;

				GenericPoint<coordType> ip2(dp2.m_x, dp2.m_y);

				//derive the segment direction
				OrthoDir od;
				bool isHor = DIsEqual(dp1.m_y, dp2.m_y, 1e-9); //may still be same place
				if (isHor)
				{
					if (DIsGreater(dp1.m_x, dp2.m_x, 1e-9)) od = odWest;
					else od = odEast;
				}//if isHor
				else
				{
					if (!DIsEqual(dp1.m_x, dp2.m_x, 1e-9))
						OGDF_THROW_PARAM(PreconditionViolatedException, pvcOrthogonal);
					//check m_x == m_x
					if (DIsLess(dp1.m_y, dp2.m_y, 1e-9)) od = odNorth;
					else od = odSouth;
				}//else
#ifdef foutput
				//outf<<"direction: "<<od<<"\n";
#endif
				m_poly[e].pushBack(ip2);
#ifdef foutput
				//outf<<"Schreibe SegmentEndPunkt: "<<ip2.m_x<<"/"<<ip2.m_y<<"\n";
#endif

				coordType len = max(fabs(dp2.m_x - dp1.m_x) ,
					fabs(dp2.m_y - dp1.m_y ) );
				//set segmentinfo values
				si.length = len;
				si.number = currNum;

				//insert the coordinates so they can later be used in edge intersection test
				si.min_x = min(dp1.m_x, dp2.m_x);
				si.max_x = max(dp1.m_x, dp2.m_x);
				si.min_y = min(dp1.m_y, dp2.m_y);
				si.max_y = max(dp1.m_y, dp2.m_y);

				si.direction = od;

				m_segInfo[e].pushBack(si);
#ifdef foutput
				//outf<<"Segmentlaenge: "<<len<<"\n";
#endif
				//set overall edge length
				length+= len;

				dp1 = dp2;
				currNum++;
			}//forall copy edges in list

#ifdef foutput
			//outf<<"Kantenlaenge: "<<length<<"\n";
#endif

			m_edgeLength[e] = length;

		}//forall nodes //edges in pru

	}//initUMLSegments


	template <class coordType>
	void ELabelPos<coordType>::initUMLFeatureRectangles()
	{
		node v;
		//forall original nodes: get cage size and position
		//if applied to result graph and layout, we dont need this

		forall_nodes(v, m_ug->constGraph())
		{

			double aktX = m_ug->x(v);
			double aktY = m_ug->y(v);

			FeatureInfo fi;
			fi.max_x = aktX + m_ug->width(v)/2.0;
			fi.min_x = aktX - m_ug->width(v)/2.0;
			fi.max_y = aktY + m_ug->height(v)/2.0;
			fi.min_y = aktY - m_ug->height(v)/2.0;
			fi.size_x = m_ug->width(v);   //fi.max_x - fi.min_x;
			fi.size_y = m_ug->height(v);  //fi.max_y - fi.min_y;

			m_featureInfo[v] = fi;

		}//forallnodes

		//now all expanded (original) nodes have a featureinfo

	}//initUMLFeatureRectangles


	//*****************************************************************
	//test intersection of label position candidates with graph objects
	template <class coordType>
	void ELabelPos<coordType>::testUMLFeatureIntersect()
	{

		//set m_active to 1 for all feature intersecting position candidates
		//initialize label structure

		//assure spare candidate
		EdgeArray< GenericPoint<coordType> > l_saveCandidate[labelNum];
		//TODO: nur falls usedLabel
		for (int k = 0; k < labelNum; k++) l_saveCandidate[k].init(m_ug->constGraph());
		saveUMLRecovery(l_saveCandidate);

		edge e;
		//for all edge labels and their cand. pos., we check intersections
		//with original graph features

		forall_edges(e, m_ug->constGraph())
		{
			int i;
			for (i = 0; i < labelNum; i++)
			{

				//TODO: check if usedLabel
				if (m_eli->getLabel(e).usedLabel((eLabelTyp)i)) m_numAssignment++;


				//all PosInfos for candidates of label i
				List<PosInfo>& l_candList = candList(e, i);

				//check intersection with original graph nodes:

				ListIterator< PosInfo > itPos = l_candList.begin();

				int numCand = l_candList.size();
				int numIS = 0; //number of intersecting candidates

				while (itPos.valid())
				{
					//save intersection status if the number is to be counted
					bool hasIntersection = false;

					GenericPoint<coordType>& coord =(*itPos).m_coord;

					//label size and position

					coordType minXLabel = coord.m_x - (m_eli->getWidth(e, (eLabelTyp)i)  / 2.0);
					coordType maxXLabel = coord.m_x + (m_eli->getWidth(e, (eLabelTyp)i)  / 2.0);
					coordType minYLabel = coord.m_y - (m_eli->getHeight(e, (eLabelTyp)i) / 2.0);
					coordType maxYLabel = coord.m_y + (m_eli->getHeight(e, (eLabelTyp)i) / 2.0);

					node w;
					//simple version: check with all original node rectangles
					bool intersect = false;
					forall_nodes(w, m_ug->constGraph())
					{
						FeatureInfo& fi = m_featureInfo[w];

						intersect = ( (minXLabel <= fi.max_x) &&
							(maxXLabel >= fi.min_x) &&
							(minYLabel <= fi.max_y) &&
							(maxYLabel >= fi.min_y) );

						hasIntersection = intersect || hasIntersection; //save positive result
						//falls man eine Gewichtung nach Anzahl der Probleme vornehmen
						//moechte
						if (!m_countFeatureIntersect)
						{ if (intersect) break;}
						else //count intersections
						{
							(*itPos).m_numFeatures++;
						}//else


					}//forall_nodes

					//now this is quadratic in |E| !!!, only test, change to sweepline
					edge e2;
					forall_edges(e2,m_ug->constGraph())
					{
						if (e == e2) continue; //no costs for own edge
						ListIterator<SegmentInfo> l_sit = m_segInfo[e2].begin();
						while (l_sit.valid())
						{
							bool eIntersect = ( (minXLabel <= ((*l_sit).max_x+segmentMargin())) &&
								(maxXLabel >= ((*l_sit).min_x-segmentMargin())) &&
								(minYLabel <= ((*l_sit).max_y+segmentMargin())) &&
								(maxYLabel >= ((*l_sit).min_y-segmentMargin())) );
							if (eIntersect) (*itPos).m_cost += costEI();
							l_sit++;
						}//while

					}//foralledges


					if (hasIntersection)
					{
						numIS++;
						//change status of candidate instead of deletion
						(*itPos).m_active = csFIntersect;
						(*itPos).m_cost += costFI(); //todo: use multiple costs for multiple int.
						itPos++;

						continue;
					}

					itPos++;
				}//while

				//TODO: hier stand einsetzung des savecandidate, man muesste hier
				//oder in call dann einen "besten" Ersatzkandidaten einfuegen
				if (numIS == l_candList.size())
				{
				}//if

				//if there is only one cand, we don't need to decide, but there
				//may be no intersection (test only for secu. testing)

			}//for

		}//forallnodes
#ifdef foutput
		writeUMLGML("label2UML.gml", opOmitFIntersect);
#endif
	}//testumlfeatureintersect


	//test intersection among label position candidates
	template <class coordType>
	void ELabelPos<coordType>::testUMLAllIntersect()
	{
		List<FeatureLink> l_featureList;
		//as long as labeltree is not implemented, use a worst case quadratic
		//test better than round robin
		edge e;

		//for all edge labels and their cand. pos., we check intersections
		//with other label positions
		//insert candidates in list
		//check all edges in current CC

		forall_edges(e, m_ug->constGraph())
		{
			int i;
			for (i = 0; i < labelNum; i++)
			{
				//insert labels into structure

				if (!m_eli->getLabel(e).usedLabel((eLabelTyp)i)) continue;

				List< PosInfo >& l_candList = candList(e,i);

				//simple version one after another
				int index = 0;

				ListIterator< PosInfo > itPos = l_candList.begin();

				while (itPos.valid())
				{
					//label size and position
					FeatureInfo fi;
					fi.size_x = m_eli->getWidth(e, (eLabelTyp)i);
					fi.size_y = m_eli->getHeight(e, (eLabelTyp)i);
					fi.min_x = (*itPos).m_coord.m_x - (fi.size_x / 2.0);
					fi.max_x = (*itPos).m_coord.m_x + (fi.size_x / 2.0);
					fi.min_y = (*itPos).m_coord.m_y - (fi.size_y / 2.0);
					fi.max_y = (*itPos).m_coord.m_y + (fi.size_y / 2.0);

					//hier muss noch der Knoten hin
					l_featureList.pushBack(FeatureLink(e, (eLabelTyp)i, 0, fi, index, (*itPos) ));

					index++;
					itPos++;
				}//while


			}//for

		}//foralledges
		//*****************************************
		//now check
		//*********************************************
		//if intersection found, insert an entry in the PosInfo-pointer List
		//PosInfo::m_intersect
		//and increment the active intersection counter
		//PosInfo::m_numActive
		//Sort the label positions and perform sweepline run
		FeatureComparer fc;
		l_featureList.quicksort(fc);

		//run over all candidate positions and check intersections
		//objects are active, if their y-intervall encloses the current
		//sweepline position

		FeatureLink runFI; //next label position to be processed
		ListIterator<FeatureLink> l_it = l_featureList.begin();

		List<FeatureLink> sweepLine;

		coordType aktY = -coordBound - 1; //current y-coordinate of sweepline

		//run over all inserted label positions
		while (l_it.valid())
		{

			OGDF_ASSERT(DIsGreater((*l_it).m_fi.min_y, aktY));

			aktY = (*l_it).m_fi.min_y;

			ListIterator<FeatureLink> itSweep = sweepLine.begin(); //sweepline iterator

			//********************************
			//delete old events
			while (itSweep.valid())
			{
				if (DIsLess((*itSweep).m_fi.max_y, aktY))
				{
					ListIterator<FeatureLink> itNext = itSweep.succ();
					sweepLine.del(itSweep);
					itSweep = itNext;
					continue;
				}
				itSweep++;
			}//while

			itSweep = sweepLine.begin();
			//now we have only active events left, with y-intervall
			//including current sweepline-position

			//*****************************
			//run over all starting objects
			while ( l_it.valid() && (DIsEqual(aktY,(*l_it).m_fi.min_y)) )
			{
				itSweep = sweepLine.begin();
				//run over sweepline to find position and check intersection
				//sort sweepline by increasing min_x-value
				FeatureLink& fl = (*l_it);
				//get the corresponding PosInfo
				//PosInfo& aktPosInfo =

				//int aktX = (int)((*l_it).m_fi.min_x);
				coordType aktX = ((*l_it).m_fi.min_x);

				//skip/test objects left of l_it
				while (itSweep.valid() && DIsGreater(aktX, (*itSweep).m_fi.min_x) )
				{
					//test on intersection
					if (DIsLessEqual(aktX, (*itSweep).m_fi.max_x))
					{
						FeatureLink& fls = (*itSweep);
						//intersecting another position for the same label is ok
						if (!( (fl.m_elt == fls.m_elt) && (fl.m_edge == fls.m_edge) ))
						{
							//insert PosInfo pointer into PosInfo::m_intersect and increment m_numActive
							//in both labels
							PosInfo& pi1 = (*(fl.m_posInfo));
							PosInfo& pi2 = (*(fls.m_posInfo));

							pi1.m_intersect.pushBack(fls.m_posInfo);
							pi2.m_intersect.pushBack(fl.m_posInfo);
							pi1.m_numActive++;
							pi2.m_numActive++;

							pi1.m_cost += costLI();
							pi2.m_cost += costLI();

						}//if intersection
					}
					itSweep++;
				}//while left

				//sweepline empty or object on same pos. / to the right found
				//insert into sweepline: end or after last
				if (!itSweep.valid())
				{
					sweepLine.pushBack((*l_it));
				}//if rightmost entry
				else
				{
					sweepLine.insertBefore((*l_it), itSweep);

				}//else insert before

				//now check for intersection until min_x > l_it.max_x
				while ( itSweep.valid() && (DIsLessEqual((*itSweep).m_fi.min_x, (*l_it).m_fi.max_x)) )
				{
					FeatureLink& fls = (*itSweep);

					//intersecting another position for the same label is ok
					if (!( (fl.m_elt == fls.m_elt) && (fl.m_edge == fls.m_edge) ))
					{
						//insert PosInfo pointer into PosInfo::m_intersect aund increment m_numActive
						PosInfo& pi1 = (*(fl.m_posInfo));
						PosInfo& pi2 = (*(fls.m_posInfo));

						pi1.m_intersect.pushBack(fls.m_posInfo);
						pi2.m_intersect.pushBack(fl.m_posInfo);
						pi1.m_numActive++;
						pi2.m_numActive++;

						pi1.m_cost += costLI();
						pi2.m_cost += costLI();


					}//if intersection of other label candidate
					itSweep++;
				}//while
				l_it++;
			}//while starting

			//l_it++;
		}//while l_it

		//now search for intersectionfree label candidates

		forall_edges(e, m_ug->constGraph())
		{
			int i;
			for (i = 0; i < labelNum; i++)
			{
				//insert labels into structure

				if (!m_eli->getLabel(e).usedLabel((eLabelTyp)i)) continue;

				List< PosInfo >& l_candList = candList(e,i);

				//simple version one after another
				int index = 0;

				ListIterator< PosInfo > itPos = l_candList.begin();

				while (itPos.valid())
				{
					if (!((*itPos).m_active == csFIntersect) )
					{
						if (!m_endInsertion)
						{
							if ((*itPos).m_intersect.empty())
								m_freeLabels.pushBack(&(*itPos));
							else m_sectLabels.pushBack(&(*itPos));
						}//if not endInsertion
						else
						{
							switch (i)
							{
							case 0:
							case 1:   //start labels, changeable
								if ((*itPos).m_intersect.empty())
									m_freeLabels.pushBack(&(*itPos));
								else m_sectLabels.pushBack(&(*itPos));
								break;
							case labelNum-1:
							case labelNum-2: //end labels
								if ((*itPos).m_intersect.empty())
									m_freeLabels.pushFront(&(*itPos));
								else m_sectLabels.pushFront(&(*itPos));
								break;
							default:
								if ((*itPos).m_intersect.empty())
									m_freeLabels.pushBack(&(*itPos));
								else m_sectLabels.pushBack(&(*itPos));

								break;

							}//switch

						}//else
					}//if feature intersection

					itPos++;
				}//while candidates
			}//for label types
		}//forall edges
		/*
		l_it = l_featureList.begin();

		while (l_it.valid())
		{
		//TODO: check csFIntersect status
		if ( !(((*l_it).m_posInfo)->m_active == csFIntersect) )
		{
		if ( ((*l_it).m_posInfo)->m_intersect.empty() )
		m_freeLabels.pushBack((*l_it).m_posInfo);
		else m_sectLabels.pushBack((*l_it).m_posInfo);
		}//if no feature intersection

		#ifdef foutput
		ofstream ofs("Intersections.txt", ios::app);
		ofs << "\nNeuer Kandidat\n" << ((*l_it).m_posInfo)->m_intersect.size()
		<< "Ueberschneidungen\n" << "Position: "
		<< ((*l_it).m_posInfo)->m_coord.m_x <<"/"<< ((*l_it).m_posInfo)->m_coord.m_y << "\n";
		#endif
		l_it++;
		}//while
		#ifdef foutput
		ofstream ofs("Intersections.txt", ios::app);
		ofs << "\nENDE DER FEATURELISTE\n" <<flush;
		#endif


		*/

#ifdef foutput
		writeUMLGML("label3UML.gml", opOmitIntersect);
#endif

	}//testUMLAllIntersect



	template <class coordType>
	void ELabelPos<coordType>::saveUMLRecovery(EdgeArray< GenericPoint<coordType> >
		(&saveCandidate)[labelNum])
	{

		const Graph& mapGraph = m_ug->constGraph();
		//a labeltype - dependant extra candidate for label positioning should
		//be defined here to help in case of empty cand.list after featureintersect

		//simple version: use one (middle?) of the original candidates

		edge e;
		forall_edges(e, mapGraph)
		{
			int i;
			for (i = 0; i < labelNum; i++)
			{
				//TODO: hier abfragen, ob label vorhanden =>etwas schneller
				List< PosInfo >& l_posList = candList(e,i); //all candidates for label Nr. i
				//hier: sichere, das Eintrag vorhanden
				int savePos;
				//am besten immer Mitte nehmen, da am wenigsten in Knoten
				if ((i == 0) || (i ==1)) savePos = 0;
				else if ((i == labelNum-1) || (i == labelNum-2))
					savePos = l_posList.size()-1;
				else savePos = int(floor((double)(l_posList.size())/2));
				if (savePos < 0) savePos = 0;
				saveCandidate[i][e] = (*l_posList.get(savePos)).m_coord;
			}//for labeltypes
		}//foralledges

	}//saveUMLRecovery


	//*************************************************************************
	//output section
	template <class coordType>
	void ELabelPos<coordType>::writeGML(const char *filename, OutputParameter sectOmit)
	{
		OGDF_ASSERT(m_prup);

		const Graph& G = *m_prup;

		ofstream os(filename);

		NodeArray<int> id(*m_prup);
		int nextId = 0;

		os.setf(ios::showpoint);
		os.precision(10);

		os << "Creator \"ogdf::ELabelPos::writeGML\"\n";
		os << "graph [\n";
		os << "  directed 1\n";

		node v;
		forall_nodes(v,G) {
			os << "  node [\n";
			os << "    id " << (id[v] = nextId++) << "\n";
			os << "    label \"" << v->index() << "\"\n";
			os << "    graphics [\n";
			os << "      x " << double(m_gl->x(v)) << "\n";
			os << "      y " << double(m_gl->y(v)) << "\n";
			os << "      w " << 3.0 << "\n";
			os << "      h " << 3.0 << "\n";
			os << "      type \"rectangle\"\n";
			os << "      width 1.0\n";
			if (m_prup->typeOf(v) == Graph::generalizationMerger) {
				os << "      type \"oval\"\n";
				os << "      fill \"#0000A0\"\n";
			}
			else if (m_prup->typeOf(v) == Graph::generalizationExpander) {
				os << "      type \"oval\"\n";
				os << "      fill \"#00FF00\"\n";
			}
			else if (m_prup->typeOf(v) == Graph::highDegreeExpander ||
				m_prup->typeOf(v) == Graph::lowDegreeExpander)
				os << "      fill \"#FFFF00\"\n";
			else if (m_prup->typeOf(v) == Graph::dummy)
				os << "      type \"oval\"\n";

			else if (v->degree() > 4)
				os << "      fill \"#FFFF00\"\n";

			else
				os << "      fill \"#000000\"\n";

			os << "    ]\n"; // graphics
			os << "  ]\n"; // node
		}//forallnodes

		edge e;
		forall_edges(e,G) {
			os << "  edge [\n";
			os << "    source " << id[e->source()] << "\n";
			os << "    target " << id[e->target()] << "\n";

			os << "    generalization " << m_prup->typeOf(e) << "\n";

			os << "    graphics [\n";

			os << "      type \"line\"\n";

			if (m_prup->typeOf(e) == Graph::generalization)
			{
				if (m_prup->typeOf(e->target()) == Graph::generalizationExpander)
					os << "      arrow \"none\"\n";
				else
					os << "      arrow \"last\"\n";
				os << "      fill \"#FF0000\"\n";
				os << "      width 2.0\n";
			}
			else
			{
				if (m_prup->typeOf(e->source()) == Graph::generalizationExpander ||
					m_prup->typeOf(e->source()) == Graph::generalizationMerger ||
					m_prup->typeOf(e->target()) == Graph::generalizationExpander ||
					m_prup->typeOf(e->target()) == Graph::generalizationMerger)
				{
					os << "      arrow \"none\"\n";
					os << "      fill \"#F0F00F\"\n";
					//os << "      fill \"#FF0000\"\n";
				}
				else if (m_prup->original(e) == 0)
				{
					os << "      arrow \"none\"\n";
					os << "      fill \"#AFAFAF\"\n";
				}
				else
					os << "      arrow \"none\"\n";
				os << "      width 1.0\n";
			}//else generalization

			os << "    ]\n"; // graphics

			os << "  ]\n"; // edge
		}//foralledges

		forall_edges(e, m_prup->original())
		{
			int i;
			for (i=0; i < labelNum; i++)
			{
				for (int j=0; j < m_candPosList[i][e].size(); j++)
				{
					if ( (sectOmit == opOmitIntersect)&&
						((*m_intersect[i][e].get(j)).size() > 0) ) continue;
					os << "  node [\n";
					os << "    id " << nextId++ << "\n";
					os << "    label \"" << 999 << "\"\n";
					os << "    graphics [\n";

					if (sectOmit == opResult)
					{
						os << "      x " << double(m_eli->getLabel(e).getX((eLabelTyp)i)) << "\n";
						os << "      y " << double(m_eli->getLabel(e).getY((eLabelTyp)i)) << "\n";
					}
					else
					{
						os << "      x " << double((*m_candPosList[i][e].get(j)).m_x) << "\n";
						os << "      y " << double((*m_candPosList[i][e].get(j)).m_y) << "\n";
					}
					os << "      w " << double(m_eli->getWidth(e, (eLabelTyp)i)) << "\n";
					os << "      h " << double(m_eli->getHeight(e, (eLabelTyp)i)) << "\n";
					os << "      type \"rectangle\"\n";
					os << "      width 1.0\n";

					//os << "      type \"oval\"\n";
					switch (i)
					{
					case 0: os << "      fill \"#2F2F0F\"\n"; break;
					case 1: os << "      fill \"#4F4F0F\"\n"; break;
					case 2: os << "      fill \"#6F6F0F\"\n"; break;
					case 3: os << "      fill \"#0F7F7F\"\n"; break;
					case 4: os << "      fill \"#0F9F9F\"\n"; break;
					}//switch

					os << "    ]\n"; // graphics
					os << "  ]\n"; // node

					if (sectOmit == opResult) break;
				}//for j
			}//for i
		}//foralledges

		os << "  ]\n"; // graph

	}//writegml


	//UML version
	template <class coordType>
	void ELabelPos<coordType>::writeUMLGML(const char *filename, OutputParameter sectOmit)
	{
		OGDF_ASSERT(m_ug);

		ofstream os(filename);

		const Graph& G = m_ug->constGraph();

		NodeArray<int> id(G);
		int nextId = 0;

		os.setf(ios::showpoint);
		os.precision(10);

		os << "Creator \"ogdf::ELabelPos::writeGML\"\n";
		os << "graph [\n";
		os << "  directed 1\n";

		node v;
		forall_nodes(v,G) {
			os << "  node [\n";
			os << "    id " << (id[v] = nextId++) << "\n";
			os << "    label \"" << v->index() << "\"\n";
			os << "    graphics [\n";
			os << "      x " << double(m_ug->x(v)) << "\n";
			os << "      y " << double(m_ug->y(v)) << "\n";
			os << "      w " << double(m_ug->width(v)) << "\n";
			os << "      h " << double(m_ug->height(v)) << "\n";
			os << "      type \"rectangle\"\n";
			os << "      width 0.01\n";
			if (m_ug->type(v) == Graph::generalizationMerger) {
				os << "      type \"oval\"\n";
				os << "      fill \"#0000A0\"\n";
			}
			else if (m_ug->type(v) == Graph::generalizationExpander) {
				os << "      type \"oval\"\n";
				os << "      fill \"#00FF00\"\n";
			}
			else if (m_ug->type(v) == Graph::highDegreeExpander ||
				m_ug->type(v) == Graph::lowDegreeExpander)
				os << "      fill \"#FFFF00\"\n";
			else if (m_ug->type(v) == Graph::dummy)
				os << "      type \"oval\"\n";

			else if (v->degree() > 4)
				os << "      fill \"#FFFF00\"\n";

			else
				os << "      fill \"#FFFF00\"\n";
			//os << "      fill \"#000000\"\n";

			os << "    ]\n"; // graphics
			os << "  ]\n"; // node
		}//forallnodes

		edge e;
		forall_edges(e,G) {
			os << "  edge [\n";
			os << "    source " << id[e->source()] << "\n";
			os << "    target " << id[e->target()] << "\n";

			os << "    generalization " << m_ug->type(e) << "\n";

			os << "    graphics [\n";

			os << "      type \"line\"\n";

			if (m_ug->type(e) == Graph::generalization)
			{
				if (m_ug->type(e->target()) == Graph::generalizationExpander)
					os << "      arrow \"none\"\n";
				else
					os << "      arrow \"last\"\n";
				os << "      fill \"#FF0000\"\n";
				os << "      width 2.0\n";
			}
			else
			{
				if (m_ug->type(e->source()) == Graph::generalizationExpander ||
					m_ug->type(e->source()) == Graph::generalizationMerger ||
					m_ug->type(e->target()) == Graph::generalizationExpander ||
					m_ug->type(e->target()) == Graph::generalizationMerger)
				{
					os << "      arrow \"none\"\n";
					os << "      fill \"#F0F00F\"\n";
					//os << "      fill \"#FF0000\"\n";
				}
				else
					os << "      arrow \"none\"\n";
				os << "      width 1.0\n";
			}//else generalization
			//output bends
			const DPolyline &dpl = m_ug->bends(e);
			if (!dpl.empty()) {
				os << "      Line [\n";
				os << "        point [ x " << m_ug->x(e->source()) << " y " <<
					m_ug->y(e->source()) << " ]\n";

				ListConstIterator<DPoint> it;
				for(it = dpl.begin(); it.valid(); ++it)
					os << "        point [ x " << (*it).m_x << " y " << (*it).m_y << " ]\n";

				os << "        point [ x " << m_ug->x(e->target()) << " y " <<
					m_ug->y(e->target()) << " ]\n";

				os << "      ]\n"; // Line
			}
			//bends

			os << "    ]\n"; // graphics

			os << "  ]\n"; // edge
		}//foralledges

		forall_edges(e, G)
		{
			int i;
			for (i=0; i < labelNum; i++)
			{
				if (!m_eli->getLabel(e).usedLabel((eLabelTyp)i)) continue;

				ListIterator< PosInfo > l_it = candList(e, i).begin();

				while (l_it.valid())
				{
					if ( ( (sectOmit == opOmitIntersect)&&
						(((*l_it).m_intersect.size() > 0) ||
						((*l_it).m_active == csFIntersect)
						)
						) ||
						( (sectOmit == opOmitFIntersect)&&
						((*l_it).m_active == csFIntersect) )
						)
					{
						l_it++;
						continue;
					}
					os << "  node [\n";
					os << "    id " << nextId++ << "\n";
					os << "    label \"" << id[e->source()] <<"/"<<id[e->target()]<<"/sects"
						<<(*l_it).m_intersect.size()<< " /cost " <<
						double((*l_it).m_cost) <<"\"\n";
					os << "    graphics [\n";

					if (sectOmit == opResult)
					{
						os << "      x " << double(m_eli->getLabel(e).getX((eLabelTyp)i)) << "\n";
						os << "      y " << double(m_eli->getLabel(e).getY((eLabelTyp)i)) << "\n";
					}
					else
					{
						os << "      x " << double((*l_it).m_coord.m_x) << "\n";
						os << "      y " << double((*l_it).m_coord.m_y) << "\n";
					}
					os << "      w " << double(m_eli->getWidth(e, (eLabelTyp)i)) << "\n";
					os << "      h " << double(m_eli->getHeight(e, (eLabelTyp)i)) << "\n";
					os << "      type \"rectangle\"\n";
					os << "      width 0.01\n";

					//os << "      type \"oval\"\n";
					//Change background color of labels here
					switch (i)
					{
					case 0: os << "      line \"#00FF00\"\n"; break;
					case 1: os << "      line \"#00FF00\"\n"; break;
					case 2: os << "      line \"#FF0000\"\n"; break;
					case 3: os << "      line \"#0000FF\"\n"; break;
					case 4: os << "      line \"#0000FF\"\n"; break;
					default: os << "      line \"#FF00FF\"\n";
						/* case 0: os << "      fill \"#2F2F0F\"\n"; break;
						case 1: os << "      fill \"#4F4F0F\"\n"; break;
						case 2: os << "      fill \"#6F6F0F\"\n"; break;
						case 3: os << "      fill \"#0F7F7F\"\n"; break;
						case 4: os << "      fill \"#0F9F9F\"\n"; break;*/
					}//switch

					os << "      fill \"#FFFFFF\"\n";

					os << "    ]\n"; // graphics
					os << "  ]\n"; // node

					if (sectOmit == opResult) break;
					l_it++;
				}//while l_it
			}//for i
		}//foralledges

		os << "]\n"; // graph


	}//writeumlgml


}//namespace ogdf
