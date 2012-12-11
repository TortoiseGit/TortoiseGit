/*
 * $Revision: 2554 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 11:39:38 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implements class ShellingOrder.
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

#include <ogdf/planarlayout/ShellingOrder.h>
#include <ogdf/basic/BoundedStack.h>
#include <ogdf/basic/SList.h>


namespace ogdf {


void ShellingOrder::init(const Graph &G, const List<ShellingOrderSet> &partition)
{
	m_pGraph = &G;
	m_V.init(1,partition.size());
	m_rank.init(G);

	int i = 1;
	ListConstIterator<ShellingOrderSet> it;
	for(it = partition.begin(); it.valid(); ++it)
	{
		const ShellingOrderSet &S = *it;
		for(int j = 1; j <= S.len(); ++j)
			m_rank[S[j]] = i;

		m_V[i++] = *it;
	}
}


void ShellingOrder::initLeftmost(
	const Graph &G,
	const List<ShellingOrderSet> &partition)
{
	m_pGraph = &G;
	m_V.init(1,partition.size());
	m_rank.init(G);

	NodeArray<SListPure<const ShellingOrderSet *> > crSets(G);
	BoundedStack<node> outerfaceStack(G.numberOfNodes());

	int i, j;

	ListConstIterator<ShellingOrderSet> it;
	for(it = partition.begin(); it.valid(); ++it) {
		node cr = (*it).right();
		if (cr != 0)
			crSets[cr].pushBack(&(*it));
	}

	const ShellingOrderSet &V1 = partition.front();
	for (j = V1.len(); j >= 2; j--)
		outerfaceStack.push(V1[j]);

	m_V[1] = V1;

	i = 2;
	while (!outerfaceStack.empty()) {
		node cr = outerfaceStack.top();
		if (crSets[cr].empty())
			outerfaceStack.pop();
		else {
			m_V[i] = *(crSets[cr].popFrontRet());
			for (j = len(i); j >= 1; j--)
				outerfaceStack.push ( (m_V[i])[j] );
			i++;
		}
	}


	for (i = 1; i <= length(); i++) {
		for (j = 1; j <= m_V[i].len(); ++j) {
			m_rank [(m_V[i])[j]] = i;
		}
	}
}


} // end namespace ogdf

