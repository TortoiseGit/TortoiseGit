/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of class NodePairEnergy
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


#include <ogdf/internal/energybased/NodePairEnergy.h>


#ifdef OGDF_SYSTEM_UNIX
#include <climits>
#endif

#ifdef OGDF_SYSTEM_WINDOWS
#include <limits>
#endif

using namespace std;

namespace ogdf {


NodePairEnergy::NodePairEnergy(const String energyname, GraphAttributes &AG) :
	EnergyFunction(energyname,AG),
	m_candPairEnergy(m_G),
	m_shape(m_G),
	m_adjacentOracle(m_G)
{
	node v;
	double lengthSum = 0;
	forall_nodes(v,m_G) { //saving the shapes of the nodes in m_shape
		DPoint center(AG.x(v),AG.y(v));
		lengthSum += AG.width(v);
		lengthSum += AG.height(v);
		m_shape[v] = IntersectionRectangle(center,AG.width(v),AG.height(v));
	}
	m_G.allNodes(m_nonIsolated);
	ListIterator<node> it, itSucc;
	for(it = m_nonIsolated.begin(); it.valid(); it = itSucc) {
		itSucc = it.succ();
		if((*it)->degree() == 0) m_nonIsolated.del(it);
	}
	m_nodeNums = OGDF_NEW NodeArray<int>(m_G,0);
	int n_num = 1;
	for(it = m_nonIsolated.begin(); it.valid(); ++it) {
		(*m_nodeNums)[*it] = n_num;
		n_num++;
	}
	n_num--;
	m_pairEnergy = new Array2D<double> (1,n_num,1,n_num);
}


void NodePairEnergy::computeEnergy()
{
	int n_num = m_nonIsolated.size();
	double energySum = 0.0;
	Array<node> numNodes(1,n_num);

	ListIterator<node> it;
	for(it = m_nonIsolated.begin(); it.valid(); ++it) {
		numNodes[(*m_nodeNums)[*it]] = *it;
	}
	for(int i = 1; i <= n_num-1 ; i++) {
		for(int j = i+1; j <= n_num; j++) {
			double E = computePairEnergy(numNodes[i],numNodes[j]);
			(*m_pairEnergy)(i,j) = E;
			energySum += E;
		}
	}
	m_energy = energySum;
}


double NodePairEnergy::computePairEnergy(const node v, const node w) const {
	return computeCoordEnergy(v,w,currentPos(v),currentPos(w));
}


void NodePairEnergy::internalCandidateTaken() {
	node v = testNode();
	int candNum = (*m_nodeNums)[v];
	ListIterator<node> it;
	for(it = m_nonIsolated.begin(); it.valid(); ++ it) {
		if((*it) != v) {
			int numit = (*m_nodeNums)[*it];
			(*m_pairEnergy)(min(numit,candNum),max(numit,candNum)) = m_candPairEnergy[*it];
			m_candPairEnergy[*it] = 0.0;
		}
	}
}


void NodePairEnergy::compCandEnergy()
{
	node v = testNode();
	int numv = (*m_nodeNums)[v];
	m_candidateEnergy = energy();
	ListIterator<node> it;
	for(it = m_nonIsolated.begin(); it.valid(); ++ it) {
		if(*it != v) {
			int j = (*m_nodeNums)[*it];
			m_candidateEnergy -= (*m_pairEnergy)(min(j,numv),max(j,numv));
			m_candPairEnergy[*it] = computeCoordEnergy(v,*it,testPos(),currentPos(*it));
			m_candidateEnergy += m_candPairEnergy[*it];
			if(m_candidateEnergy < 0.0) {
				OGDF_ASSERT(m_candidateEnergy > -0.00001);
				m_candidateEnergy = 0.0;
			}
		}
		else m_candPairEnergy[*it] = 0.0;
	}
	OGDF_ASSERT(m_candidateEnergy >= -0.0001);
}


#ifdef OGDF_DEBUG
void NodePairEnergy::printInternalData() const {
	ListConstIterator<node> it;
	for(it = m_nonIsolated.begin(); it.valid(); ++it) {
		cout << "\nNode: " << (*m_nodeNums)[*it];
		cout << " CandidatePairEnergy: " << m_candPairEnergy[*it];
	}
	cout << "\nPair energies:";
	for(int i=1; i< m_nonIsolated.size(); i++)
		for(int j=i+1; j <= m_nonIsolated.size(); j++)
			if((*m_pairEnergy)(i,j) != 0.0)
				cout << "\nEnergy(" << i << ',' << j << ") = " << (*m_pairEnergy)(i,j);
}
#endif

} //namespace ogdf
