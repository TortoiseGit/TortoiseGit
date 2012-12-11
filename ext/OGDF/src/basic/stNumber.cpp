/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of st-numbering algorithm
 *
 * \author Sebastian Leipert
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


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Stack.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>

namespace ogdf {

// Needed by the function stNumber
void stSearch(
	const Graph	  &C,
	node			  sink,
	int			  &count,
	NodeArray<int>  &low,
	NodeArray<int>  &dfn,
	NodeArray<edge> &dfsInEdge,
	NodeArray<edge> &followLowPath);

// Needed by the function stNumber
bool stPath(
	StackPure<node>	&path,
	node		 	v,
	adjEntry		&adj,
	NodeArray<bool> &markedNode,
	EdgeArray<bool> &markedEdge,
	NodeArray<int>	&dfn,
	NodeArray<edge> &dfsInEdge,
	NodeArray<edge> &followLowPath);

// Computes an st-Numbering.
// Precondition: G must be biconnected and simple.
// Exception: the Graph is allowed to have isolated nodes.
// The st-numbers are stored in NodeArray. Return value is
// the number t. It is 0, if the computation was unsuccessful.
// The nodes s and t may be specified. In this case
// s and t have to be adjacent.
// If s and t are set 0 and parameter randomized is set to true,
// the st edge is chosen to be a random edge in G.

int stNumber(const Graph &G,
	NodeArray<int> &numbering,
	node s,
	node t,
	bool randomized)
{

	int    count       = 1;

	// Stores for every vertex its LOW number
	NodeArray<int> low(G,0);
	// Stores for every vertex ist DFN number
	NodeArray<int> dfn(G,0);

	// Stores for every vertex if it has been visited dsuring the st-numbering
	NodeArray<bool> markedNode(G,false);
	// Stores for every edge if it has been visited dsuring the st-numbering
	EdgeArray<bool> markedEdge(G,false);

	// Stores for every node its ingoing edge of the dfs tree.
	NodeArray<edge> dfsInEdge(G,0);

	// Stores a path of vertices that have not been visited.
	StackPure<node> path;

	//Stores for every node the outgoing, first edge on the
	// path that defines the low number of the node.
	NodeArray<edge> followLowPath(G,0);

	edge st;
	node v;

	if (s && t)
	{
		bool found = false;
		forall_adj_edges(st,s)
			if (st->opposite(s) == t)
			{
				found = true;
				break;
			}
		if (!found)
			return 0;
	}
	else if (s)
	{
		st = s->firstAdj()->theEdge();
		t = st->opposite(s);
	}
	else if (t)
	{
		st = t->firstAdj()->theEdge();
		s = st->opposite(t);
	}
	else
	{
		if(randomized) {
			// chose a random edge in G
			st = G.chooseEdge();
			if(!st) // graph is empty?
				return 0;
			s = st->source();
			t = st->target();

		} else {
			forall_nodes(s,G)
			{
				if (s->degree() > 0)
				{
					st = s->firstAdj()->theEdge();
					t = st->opposite(s);
					break;
				}
			}
		}
	}
	if (!s || !t)
		return 0;

	// Compute the DFN and LOW numbers
	// of the block.
	dfn[t] = count++;
	low[t] = dfn[t];
	stSearch(G,s,count,low,dfn,dfsInEdge,followLowPath);
	if (low[t] > low[s])
		low[t] = low[s];

	markedNode[s] = true;
	markedNode[t] = true;
	markedEdge[st] = true;

	StackPure<node> nodeStack;	// nodeStack stores the vertices during the
								// computation of the st-numbering.
	nodeStack.push(t);
	nodeStack.push(s);
	count = 1;
	v = nodeStack.pop();
	adjEntry adj = 0;
	while (v != t)
	{
		if (!stPath(path,v,adj,markedNode,markedEdge,dfn,dfsInEdge,followLowPath))
		{
			numbering[v] = count;
			count++;
			adj = 0;
		}
		else
		{
			while (!path.empty())
				nodeStack.push(path.pop());
		}
		v = nodeStack.pop();
	}
	numbering[t] = count;
	return count;
}


// Computes the DFN and LOW numbers of a biconnected component
// Uses DFS strategy
void stSearch(
	const Graph		&G,
	node				v,
	int				&count,
	NodeArray<int>	&low,
	NodeArray<int>	&dfn,
	NodeArray<edge>	&dfsInEdge,
	NodeArray<edge>	&followLowPath)
{
	dfn[v] = count;
	count++;
	low[v] = dfn[v];

	node adj = 0;
	edge e;
	forall_adj_edges(e,v)
	{
		adj = e->opposite(v);

		if(!dfn[adj]) // node not visited yet
		{
			dfsInEdge[adj] = e;
			stSearch(G,adj,count,low,dfn,dfsInEdge,followLowPath);
			if (low[v] > low[adj])
			{
				low[v] = low[adj];
				followLowPath[v] = e;
			}
		}
		else if (low[v] > dfn[adj])
		{
			low[v] = dfn[adj];
			followLowPath[v] = e;
		}
	}
}


bool stPath(StackPure<node>	&path,
			node		 	v,
			adjEntry		&adj,
			NodeArray<bool>	&markedNode,
			EdgeArray<bool> &markedEdge,
			NodeArray<int>	&dfn,
			NodeArray<edge> &dfsInEdge,
			NodeArray<edge> &followLowPath)
{
	edge e;
	node w;
	path.clear();

	if (!adj)
		adj = v->firstAdj(); // no edge has been visited yet
	do {
		e = adj->theEdge();
		adj = adj->succ();
		if (markedEdge[e])
			continue;
		markedEdge[e] = true;

		w = e->opposite(v);

		if (dfsInEdge[w] == e)
		{
			path.push(v);
			while (!markedNode[w])
			{
				e = followLowPath[w];
				path.push(w);
				markedNode[w] = true;
				markedEdge[e] = true;
				w = e->opposite(w);
			}
			return true;
		}
		else if (dfn[v] < dfn[w])
		{
			path.push(v);
			while (!markedNode[w])
			{
				e = dfsInEdge[w];
				path.push(w);
				markedNode[w] = true;
				markedEdge[e] = true;
				w = e->opposite(w);
			}
			return true;
		}

	}while (adj != 0);

	return false;
}


bool testSTnumber(const Graph &G, NodeArray<int> &st_no,int max)
{
	bool   foundLow = false;
	bool   foundHigh = false;
	bool   it_is = true;
	node   v;

	forall_nodes(v,G)
	{
		if (v->degree() == 0)
			continue;

		foundHigh = foundLow = 0;
		if (st_no[v] == 1)
		{
			adjEntry adj;
			forall_adj(adj,v)
			{
				if (st_no[adj->theEdge()->opposite(v)] == max)
					foundLow = foundHigh = 1;
			}
		}

		else if (st_no[v] == max)
		{
			adjEntry adj;
			forall_adj(adj,v)
			{
				if (st_no[adj->theEdge()->opposite(v)] == 1)
					foundLow = foundHigh = 1;
			}
		}

		else
		{
			adjEntry adj;
			forall_adj(adj,v)
			{
				if (st_no[adj->theEdge()->opposite(v)] < st_no[v])
					foundLow = 1;
				else if (st_no[adj->theEdge()->opposite(v)] > st_no[v])
					foundHigh = 1;
			}
		}
		if (!foundLow || !foundHigh)
			it_is = 0;
	}
	return it_is;
}


}
