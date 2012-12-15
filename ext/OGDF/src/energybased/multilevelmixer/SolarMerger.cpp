/*
 * $Revision: 2552 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-05 16:45:20 +0200 (Do, 05. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Merges nodes with Solar System rules
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

#include <ogdf/energybased/multilevelmixer/SolarMerger.h>

namespace ogdf {

SolarMerger::SolarMerger(bool simple, bool massAsNodeRadius)
:m_sunSelectionSimple(simple), m_massAsNodeRadius(massAsNodeRadius)
{
}


int SolarMerger::calcSystemMass(node v) {
	unsigned int sum = m_mass[v];
	adjEntry adj;
	forall_adj(adj, v) {
		sum += m_mass[adj->twinNode()];
	}
	return sum;
}


std::vector<node> SolarMerger::selectSuns(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	std::vector<node> suns;
	std::vector<node> candidates;

	node v;
	forall_nodes(v, G) {
		candidates.push_back(v);
	}

	if (m_sunSelectionSimple) {
		while (!candidates.empty()) {
			// select random node
			int index = randomNumber(0, (int)candidates.size()-1);
			node sun = candidates[index];
			candidates[index] = candidates.back();
			candidates.pop_back();
			if (m_celestial[sun] != 0) {
				continue;
			}
			bool hasForeignPlanet = false;
			adjEntry adj;
			forall_adj(adj, sun) {
				if(m_celestial[adj->twinNode()] != 0) {
					hasForeignPlanet = true;
					break;
				}
			}
			if (hasForeignPlanet) {
				continue;
			}
			// mark node as sun
			m_celestial[sun] = 1;
			suns.push_back(sun);
			// mark neighbours as planet
			forall_adj(adj, sun) {
				m_celestial[adj->twinNode()] = 2;
				m_orbitalCenter[adj->twinNode()] = sun;
				m_distanceToOrbit[adj->twinNode()] = MLG.weight(adj->theEdge());
			}
		}
	} else {
		while (!candidates.empty()) {
			std::vector< std::pair<node, int> > sunCandidates;
			int i = 1;
			int n = 10;
			while (i<=n && !candidates.empty()) {
				// select random node
				int index = randomNumber(0, (int)candidates.size()-1);
				node rndNode = candidates[index];
				candidates[index] = candidates.back();
				candidates.pop_back();
				if (m_celestial[rndNode] != 0) {
					continue;
				}
				bool hasForeignPlanet = false;
				adjEntry adj;
				forall_adj(adj, rndNode) {
					if(m_celestial[adj->twinNode()] != 0) {
						hasForeignPlanet = true;
						break;
					}
				}
				if (hasForeignPlanet) {
					continue;
				}
				unsigned int mass = calcSystemMass(rndNode);
				sunCandidates.push_back(std::pair<node, int>(rndNode, mass));
				i++;
			}
			if (sunCandidates.empty()) {
				continue;
			}

			node minNode = sunCandidates.front().first;
			unsigned int minMass = sunCandidates.front().second;
			// select sun with smalles mass from sunCandidates
			for (std::vector< std::pair<node, int> >::iterator i = sunCandidates.begin();
				i != sunCandidates.end(); i++)
			{
				node nod = (*i).first;
				unsigned int mass = (*i).second;
				if (mass < minMass) {
					minMass = mass;
					minNode = nod;
				}
			}

			for (std::vector< std::pair<node, int> >::iterator i = sunCandidates.begin();
				i != sunCandidates.end(); i++)
			{
				node temp = (*i).first;
				if (temp == minNode) {
					continue;
				}
				candidates.push_back(temp);
			}

			// mark node as sun
			m_celestial[minNode] = 1;
			suns.push_back(minNode);
			// mark neighbours as planet
			adjEntry adj;
			forall_adj(adj, minNode) {
				m_celestial[adj->twinNode()] = 2;
				m_orbitalCenter[adj->twinNode()] = minNode;
				m_distanceToOrbit[adj->twinNode()] = MLG.weight(adj->theEdge());
			}
		}
	}

	forall_nodes(v, G) {
		if (m_celestial[v] == 0) {
			m_celestial[v] = 3;
			adjEntry adj;
			std::vector<adjEntry> planets;
			node planet;
			forall_adj(adj, v) {
				planet = adj->twinNode();
				if (m_celestial[planet] == 2) {
					planets.push_back(adj);
				}
			}
			OGDF_ASSERT(planets.size() > 0);
			int index = randomNumber(0, (int)planets.size()-1);
			planet = planets[index]->twinNode();
			m_orbitalCenter[v] = planet;
			m_distanceToOrbit[v] = MLG.weight(planets[index]->theEdge());
		}
	}

	return suns;
}


void SolarMerger::buildAllLevels(MultilevelGraph &MLG)
{
	m_numLevels = 1;
	Graph &G = MLG.getGraph();
	if (m_massAsNodeRadius || !m_sunSelectionSimple) {
		m_mass.init(G, 1);
		m_radius.init(G);
		node v;
		forall_nodes(v, G) {
			m_radius[v] = MLG.radius(v);
		}
	}
	MLG.updateReverseIndizes();
	while (buildOneLevel(MLG))
	{//this is not needed anymore locally since Multilevelbuilder keeps this info
		m_numLevels++;
	}
	MLG.updateReverseIndizes();
}


node SolarMerger::sunOf(node object)
{
	if (object == 0 || m_celestial[object] == 0) {
		return 0;
	}
	if (m_celestial[object] == 1) {
		return object;
	}
	return sunOf(m_orbitalCenter[object]);
}


void SolarMerger::addPath(node sourceSun, node targetSun, double distance)
{
	node source = sourceSun;
	node target = targetSun;
	if (targetSun->index() < sourceSun->index()) {
		source = targetSun;
		target = sourceSun;
	}
	PathData data = m_interSystemPaths[source->index()][target->index()];
	OGDF_ASSERT(data.targetSun == target->index() || data.number == 0);

	int num = data.number;
	double len = data.length;

	len = len * num + distance;
	num++;
	len /= num;
	data.length = len;
	data.number = num;
	data.targetSun = target->index();

	m_interSystemPaths[source->index()][target->index()] = data;
}


double SolarMerger::distanceToSun(node object, MultilevelGraph &MLG)
{
	double dist = 0.0;

	if (object == 0 || m_celestial[object] <= 1) {
		return dist;
	}

	node center = m_orbitalCenter[object];
	OGDF_ASSERT(center != 0);

	bool found = false;
	adjEntry adj;
	forall_adj(adj, object) {
		if (adj->twinNode() == center) {
			found = true;
			dist = MLG.weight(adj->theEdge());
			OGDF_ASSERT(dist > 0);
			break;
		}
	}
	OGDF_ASSERT(found);

	return distanceToSun(center, MLG) + dist;
}


void SolarMerger::findInterSystemPaths(Graph &G, MultilevelGraph &MLG)
{
	edge e;
	forall_edges(e, G) {
		node source = e->source();
		node target = e->target();
		if (sunOf(source) != sunOf(target)) {
			// construct intersystempath
			double len = distanceToSun(source, MLG) + distanceToSun(target, MLG) + MLG.weight(e);
			OGDF_ASSERT(len > 0);
			addPath(sunOf(source), sunOf(target), len);

			// save positions of nodes on the path.
			node src = source;
			do {
				double dist = distanceToSun(src, MLG);
				m_pathDistances[src].push_back(PathData(sunOf(target)->index(), dist / len, 1));
				src = m_orbitalCenter[src];
			} while(src != 0);

			node tgt = target;
			do {
				double dist = distanceToSun(tgt, MLG);
				m_pathDistances[tgt].push_back(PathData(sunOf(source)->index(), dist / len, 1));
				tgt = m_orbitalCenter[tgt];
			} while(tgt != 0);
		}
	}
}


bool SolarMerger::buildOneLevel(MultilevelGraph &MLG)
{
	Graph &G = MLG.getGraph();
	int level = MLG.getLevel() + 1;

	int numNodes = G.numberOfNodes();

	if (numNodes <= 3) {
		return false;
	}

	m_orbitalCenter.init(G, 0);
	m_distanceToOrbit.init(G, 1.0);
	m_pathDistances.init(G, std::vector<PathData>());
	m_celestial.init(G, 0);
	m_interSystemPaths.clear();

	std::vector<node> suns = selectSuns(MLG);

	if (suns.empty()) {
		return false;
	}

	findInterSystemPaths(G, MLG);

	for(std::vector<node>::iterator i = suns.begin(); i != suns.end(); i++) {
		if (!collapsSolarSystem(MLG, *i, level)) {
			return false;
		}
	}

	NodeMerge * lastMerge = MLG.getLastMerge();
	edge e;
	forall_edges(e, G) {
		node source = e->source();
		node target = e->target();
		if (target->index() < source->index())
		{
			node temp = source;
			source = target;
			target = temp;
		}

		if (!m_interSystemPaths[source->index()].empty()) {
			if (m_interSystemPaths[source->index()][target->index()].number != 0) {
				MLG.changeEdge(lastMerge, e, m_interSystemPaths[source->index()][target->index()].length, source, target);
			}
		}
	}

	return true;
}


bool SolarMerger::collapsSolarSystem(MultilevelGraph &MLG, node sun, int level)
{
	bool retVal = false;

	std::vector<node> systemNodes;
	unsigned int mass = 0;
	if (m_massAsNodeRadius || !m_sunSelectionSimple) {
		mass = m_mass[sun];
	}

	OGDF_ASSERT(m_celestial[sun] == 1)

	adjEntry adj;
	forall_adj(adj, sun) {
#ifdef OGDF_DEBUG
		node planet = adj->twinNode();
#endif
		OGDF_ASSERT(m_celestial[planet] == 2)
		OGDF_ASSERT(m_orbitalCenter[planet] == sun)
		systemNodes.push_back(adj->twinNode());
	}
	forall_adj(adj, sun) {
		node planet = adj->twinNode();
		OGDF_ASSERT(m_celestial[planet] == 2)
		OGDF_ASSERT(m_orbitalCenter[planet] == sun)
		adjEntry adj2;
		forall_adj(adj2, planet) {
			node moon = adj2->twinNode();
			if(m_celestial[moon] == 3 && m_orbitalCenter[moon] == planet) {
				systemNodes.push_back(moon);
			}
		}
	}

	if (m_massAsNodeRadius || !m_sunSelectionSimple) {
		for(std::vector<node>::iterator i = systemNodes.begin(); i != systemNodes.end(); i++) {
			mass += m_mass[*i];
		}
		m_mass[sun] = mass;
	}

	for(std::vector<node>::iterator i = systemNodes.begin(); i != systemNodes.end(); i++) {
		node mergeNode = *i;

		if (MLG.getNode(sun->index()) != sun
			|| MLG.getNode(mergeNode->index()) != mergeNode)
		{
			return false;
		}

		NodeMerge * NM = new NodeMerge(level);
		std::vector<PathData> positions = m_pathDistances[mergeNode];
		for (std::vector<PathData>::iterator j = positions.begin(); j != positions.end(); j++) {
			NM->m_position.push_back(std::pair<int,double>((*j).targetSun, (*j).length));
		}

		bool ret;
		if (i == systemNodes.begin() && m_massAsNodeRadius) {
			ret = MLG.changeNode(NM, sun, sqrt((float)m_mass[sun]) * m_radius[sun], mergeNode);
		} else {
			ret = MLG.changeNode(NM, sun, MLG.radius(sun), mergeNode);
		}
		OGDF_ASSERT( ret );
		MLG.moveEdgesToParent(NM, mergeNode, sun, true, m_adjustEdgeLengths);
		ret = MLG.postMerge(NM, mergeNode);
		if( !ret ) {
			delete NM;
		} else {
			retVal = true;
		}
	}

	return retVal;
}

} // namespace ogdf
