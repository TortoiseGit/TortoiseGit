/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief implementation of the class ExtractKuratowskis
 *
 * \author Jens Schmidt
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


#include <ogdf/planarity/ExtractKuratowskis.h>


namespace ogdf {

// reinitializes backtracking. all paths will be traversed again. startedges are
// either startInclude or not startExclude, all startedges have to contain the flag
// startFlag
void DynamicBacktrack::init(
				const node start,
				const node end,
				const bool less,
				const int flag,
				const int startFlag = 0,
				const edge startInclude = NULL,
				const edge startExlude = NULL)
{
	OGDF_ASSERT(start!=NULL && end!=NULL);
	this->start = start;
	this->end = end;
	this->less = less;
	this->flag = flag;

	// init stack
	stack.clear();
	adjEntry adj;
	if (startInclude == NULL) {
		forall_adj(adj,start) {
			if (((m_flags[adj->theEdge()] & startFlag) == startFlag) &&
					adj->theEdge() != startExlude) {
				stack.push(NULL);
				stack.push(adj);
			}
		}
	} else {
		forall_adj(adj,start) {
			if (adj->theEdge() == startInclude &&
				(m_flags[adj->theEdge()] & startFlag) == startFlag) {
				stack.push(NULL);
				stack.push(adj);
			}
		}
	}

	// init array parent
	if (!stack.empty()) {
		m_parent.fill(NULL);
		m_parent[start] = stack.top();
	}
}

// returns the next possible path from start to endnode, if exists.
// endnode returns the last traversed node.
bool DynamicBacktrack::addNextPath(SListPure<edge>& list, node& endnode) {
	adjEntry adj;
	node v = NULL;
	node temp;

	while (!stack.empty()) {
		// backtrack
		adj = stack.pop();

		// return from a child node: delete parent
		if (adj==NULL) {
			// go to parent and delete visited flag
			temp = v;
			v = m_parent[temp]->theNode();
			m_parent[temp] = NULL;
			continue;
		}

		// get and mark node
		v = adj->twinNode();
		m_parent[v] = adj;

		// path found
		if ((less && m_dfi[v]<m_dfi[end]) || (!less && v==end))
		{
			// extract path
			endnode = v;
			list.clear();
			list.pushBack(adj->theEdge());
			while(adj->theNode() != start) {
				adj = m_parent[adj->theNode()];
				list.pushBack(adj->theEdge());
			}

			// in a following call of this method we'll have to reconstruct the actual
			// state, therefore delete the last NULLs and visited flags on stack
			while (!stack.empty() && stack.top()==NULL) {
				stack.pop();
				temp = v;
				v = m_parent[temp]->theNode();
				m_parent[temp] = NULL;
			}

			// return bool, if path found
			return true;
		}

		// push all possible child-nodes
		forall_adj(adj,v) {
			// if edge is signed and target node was not visited before
			if ((m_flags[adj->theEdge()] & flag) && (m_parent[adj->twinNode()]==NULL)) {
				stack.push(NULL);
				stack.push(adj);
			}
		}
	}
	return false;
}

// returns the next possible path from start to endnode, if exists.
// endnode returns the last traversed node. all paths avoid "exclude"-nodes, except if
// on an edge with flag "exceptOnEdge". only the part of the path, that doesn't
// contain "exclude"-nodes is finally added. Here also the startedges computed in init()
// are considered to match these conditions.
bool DynamicBacktrack::addNextPathExclude(
				SListPure<edge>& list,
				node& endnode,
				const NodeArray<int>& nodeflags,
				int exclude,
				int exceptOnEdge) {
	adjEntry adj;
	node v = NULL;
	node temp;

	while (!stack.empty()) {
		// backtrack
		adj = stack.pop();

		// return from a child node: delete parent
		if (adj==NULL) {
			// go to parent and delete visited flag
			temp = v;
			v = m_parent[temp]->theNode();
			m_parent[temp] = NULL;
			continue;
		}

		// get and mark node
		v = adj->twinNode();

		// check if startedges computed in init() match th conditions
		if (nodeflags[v]==exclude && !(m_flags[adj->theEdge()] & exceptOnEdge)) {
			OGDF_ASSERT(stack.top()==NULL);
			stack.pop();
			continue;
		}
		m_parent[v] = adj;

		// path found
		if ((less && m_dfi[v] < m_dfi[end]) || (!less && v==end))
		{
			// extract path vice versa until the startnode or an exclude-node is found
			endnode = v;
			list.clear();
			OGDF_ASSERT(nodeflags[v] != exclude);
			list.pushBack(adj->theEdge());
			while (adj->theNode() != start && nodeflags[adj->theNode()] != exclude) {
				adj = m_parent[adj->theNode()];
				list.pushBack(adj->theEdge());
			}

			// in a following call of this method we'll have to reconstruct the actual
			// state, therefore delete the last NULLs and visited flags on stack
			while (!stack.empty() && stack.top()==NULL) {
				stack.pop();
				temp = v;
				v = m_parent[temp]->theNode();
				m_parent[temp] = NULL;
			}

			// return bool, if path found
			return true;
		}

		// push all possible child-nodes
		forall_adj(adj,v) {
			node x = adj->twinNode();
			edge e = adj->theEdge();
			// if edge is signed and target node was not visited before
			if ((m_flags[e] & flag) && m_parent[x]==NULL)
			{
				// don't allow exclude-nodes, if not on an except-edge
				if ((nodeflags[x] != exclude) || (m_flags[e] & exceptOnEdge))
				{
					stack.push(NULL);
					stack.push(adj);
				}
			}
		}
	}
	return false;
}

// class ExtractKuratowski
ExtractKuratowskis::ExtractKuratowskis(BoyerMyrvoldPlanar& bm) :
	BMP(bm),
	m_g(bm.m_g),
	m_embeddingGrade(bm.m_embeddingGrade),
	m_avoidE2Minors(bm.m_avoidE2Minors),

	m_wasHere(m_g,0),

	// initialize Members of BoyerMyrvoldPlanar
	m_dfi(bm.m_dfi),
	m_nodeFromDFI(bm.m_nodeFromDFI),
	m_adjParent(bm.m_adjParent)
{
	OGDF_ASSERT(m_embeddingGrade == BoyerMyrvoldPlanar::doFindUnlimited ||
				m_embeddingGrade > 0);
	// if only structures are limited, subdivisions must not be limited
	if (bm.m_limitStructures) m_embeddingGrade = BoyerMyrvoldPlanar::doFindUnlimited;
	m_nodeMarker = 0;

	// flip Graph and merge virtual with real nodes, if not already done
	bm.flipBicomp(1,-1,m_wasHere,true,true);
}

// returns the type of Kuratowski subdivision in list (none, K33 or K5)
int ExtractKuratowskis::whichKuratowski(
					const Graph& m_g,
					const NodeArray<int>& /*m_dfi*/,
					const SListPure<edge>& list) {
	OGDF_ASSERT(!list.empty());
	EdgeArray<int> edgenumber(m_g,0);

	// count edges
	SListConstIterator<edge> it;
	for (it = list.begin(); it.valid(); ++it) {
		edge e = *it;
		if (edgenumber[e] == 1) {
			return ExtractKuratowskis::none;
		}
		edgenumber[e] = 1;
	}

	return whichKuratowskiArray(m_g,/*m_dfi,*/edgenumber);
}

// returns the type of Kuratowski subdivision in list (none, K33 or K5)
// the edgenumber has to be 1 for used edges, otherwise 0
int ExtractKuratowskis::whichKuratowskiArray(
					const Graph& m_g,
					//const NodeArray<int>& /* m_dfi */,
					EdgeArray<int>& edgenumber)
{
	edge e,ed;
	node v;
	NodeArray<int> nodenumber(m_g,0);
	int K33Partition[6] = {0,-1,-1,-1,-1,-1};
	bool K33Links[6][6] = {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},
							{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}};

	node K33Nodes[6];
	node K5Nodes[5];

	#ifdef OGDF_DEBUG
	forall_edges(e,m_g) OGDF_ASSERT(edgenumber[e] == 0 || edgenumber[e] == 1);
	#endif

	// count incident nodes
	SListConstIterator<edge> it;
	int allEdges = 0;
	forall_edges(e,m_g) {
		if (edgenumber[e] == 1) {
			++allEdges;
			++nodenumber[e->source()];
			++nodenumber[e->target()];
		}
	}
	if (allEdges < 9) {
		return ExtractKuratowskis::none;
	}

	int degree3nodes = 0;
	int degree4nodes = 0;
	forall_nodes(v,m_g) {
		if (nodenumber[v] > 4 || nodenumber[v] == 1) {
			return ExtractKuratowskis::none;
		}
		if (nodenumber[v]==3) {
			K33Nodes[degree3nodes] = v;
			++degree3nodes;
		} else if (nodenumber[v]==4) {
			K5Nodes[degree4nodes] = v;
			++degree4nodes;
		}
	}

	// check on K3,3
	int paths = 0;
	if (degree3nodes == 6) {
		if (degree4nodes > 0) {
			return ExtractKuratowskis::none;
		}
		for (int i=0; i<6; ++i) {
			forall_adj_edges(e,K33Nodes[i]) {
				if (edgenumber[e] > 0) { // not visited
					edgenumber[e] = -2; // visited
					v = e->opposite(K33Nodes[i]);
					// traverse nodedegree-2 path until degree-3 node found
					while (nodenumber[v] != 3) {
						nodenumber[v] = -2; // visited
						forall_adj_edges(ed,v) if (edgenumber[ed] > 0) break;
						OGDF_ASSERT(edgenumber[ed] > 0);
						edgenumber[ed] = -2; // visited
						v = ed->opposite(v);
					}
					int ii;
					for (ii=0; ii<6; ++ii) if (K33Nodes[ii]==v) break;
					OGDF_ASSERT(ii>=0 && ii<=5);
					if (K33Partition[i] != K33Partition[ii]) {
						++paths;
						if (K33Partition[ii]==-1) K33Partition[ii] = !K33Partition[i];
						if (!K33Links[i][ii]) {
							K33Links[i][ii] = true;
						} else {
							return ExtractKuratowskis::none;
						}
					} else {
						return ExtractKuratowskis::none;
					}
				}
			}
		}
		if (paths==9) {
			return ExtractKuratowskis::K33;
		} else {
			return ExtractKuratowskis::none;
		}
	} else if (degree4nodes == 5) {
		// check on K5
		if (degree3nodes > 0) {
			return ExtractKuratowskis::none;
		}
		for (int i=0; i<5; ++i) {
			forall_adj_edges(e,K5Nodes[i]) {
				if (edgenumber[e] > 0) { // not visited
					edgenumber[e] = -2; // visited
					v = e->opposite(K5Nodes[i]);
					// traverse nodedegree-2 path until degree-4 node found
					while (nodenumber[v] != 4) {
						nodenumber[v] = -2; // visited
						forall_adj_edges(ed,v) if (edgenumber[ed] > 0) break;
						if (edgenumber[ed] <= 0) break;
						edgenumber[ed] = -2; // visited
						v = ed->opposite(v);
					}
					if (nodenumber[v] == 4) ++paths;
				}
			}
		}
		if (paths==10) {
			return ExtractKuratowskis::K5;
		} else {
			return ExtractKuratowskis::none;
		}
	} else {
		return ExtractKuratowskis::none;
	}
}

// returns true, if kuratowski EdgeArray isn't already contained in output
bool ExtractKuratowskis::isANewKuratowski(
		//const Graph& g,
		const EdgeArray<int>& test,
		const SList<KuratowskiWrapper>& output)
{
	SListConstIterator<KuratowskiWrapper> itW;
	SListConstIterator<edge> it;
	for (itW = output.begin(); itW.valid(); ++itW) {
		bool differentEdgeFound = false;
		for (it = (*itW).edgeList.begin(); it.valid(); ++it) {
			if (!test[*it]) {
				differentEdgeFound = true;
				break;
			}
		}
		if (!differentEdgeFound) {
			cerr << "\nERROR: Kuratowski is already in list as subdivisiontype "
				<< (*itW).subdivisionType << "\n";
			return false;
		}
	}
	return true;
}

// returns true, if kuratowski edgelist isn't already contained in output
bool ExtractKuratowskis::isANewKuratowski(
		const Graph& g,
		const SListPure<edge>& kuratowski,
		const SList<KuratowskiWrapper>& output)
{
	EdgeArray<int> test(g,0);
	SListConstIterator<edge> it;
	for (it = kuratowski.begin(); it.valid(); ++it) test[*it] = 1;
	return isANewKuratowski(/*g,*/test,output);
}

// returns adjEntry of the edge between node high and that node
// with the lowest DFI not less than the DFI of low
inline adjEntry ExtractKuratowskis::adjToLowestNodeBelow(node high, int low) {
	adjEntry adj;
	int result = 0;
	int temp;
	adjEntry resultAdj = NULL;
	forall_adj(adj,high) {
		temp = m_dfi[adj->twinNode()];
		if (temp >= low && (result==0 || temp < result)) {
			result = temp;
			resultAdj = adj->twin();
		}
	}
	if (result==0) {
		return NULL;
	} else return resultAdj;
}

// add DFS-path from node bottom to node top to edgelist
// each virtual node has to be merged
inline void ExtractKuratowskis::addDFSPath(
						SListPure<edge>& list,
						node bottom,
						node top) {
	if (bottom == top) return;
	adjEntry adj = m_adjParent[bottom];
	list.pushBack(adj->theEdge());
	while (adj->theNode() != top) {
		adj = m_adjParent[adj->theNode()];
		list.pushBack(adj->theEdge());
	}
}

// the same as above but list is reversed
inline void ExtractKuratowskis::addDFSPathReverse(
						SListPure<edge>& list,
						node bottom,
						node top) {
	if (bottom == top) return;
	adjEntry adj = m_adjParent[bottom];
	list.pushFront(adj->theEdge());
	while (adj->theNode() != top) {
		adj = m_adjParent[adj->theNode()];
		list.pushFront(adj->theEdge());
	}
}

// separate list1 from edges already contained in list2
inline void ExtractKuratowskis::truncateEdgelist(
					SListPure<edge>& list1,
					const SListPure<edge>& list2)
{
	SListConstIterator<edge> it = list2.begin();
	while (!list1.empty() && it.valid() && list1.front() == *it) {
		list1.popFront();
		++it;
	}
}

// extracts a type A minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorA(
				SList<KuratowskiWrapper>& output,
				const KuratowskiStructure& k,
				//const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	OGDF_ASSERT(k.RReal != k.V);
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper A;

	// add all external face edges
	addExternalFacePath(A.edgeList,k.externalFacePath);

	// add the path from v to u, this is only possible after computation of pathX and pathY
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(A.edgeList,k.V,endnodeX);
	} else addDFSPath(A.edgeList,k.V,endnodeY);

	// copy other paths to subdivision
	SListConstIterator<edge> it;
	for(it = pathX.begin(); it.valid(); ++it) A.edgeList.pushBack(*it);
	for(it = pathY.begin(); it.valid(); ++it) A.edgeList.pushBack(*it);
	for(it = pathW.begin(); it.valid(); ++it) A.edgeList.pushBack(*it);
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,A.edgeList) == ExtractKuratowskis::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,A.edgeList,output));
	A.subdivisionType = KuratowskiWrapper::A;
	A.V = k.V;
	output.pushBack(A);
}

// extracts a type B minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorB(
				SList<KuratowskiWrapper>& output,
				//NodeArray<int>& nodeflags,
				//const int nodemarker,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper B;

	// find ExternE-struct suitable for wNode
	SListIterator<ExternE> itExternW;
	for (itExternW = info.externEStart; (*itExternW).theNode != info.w; ++itExternW)
		;
	OGDF_ASSERT(itExternW.valid() && (*itExternW).theNode == info.w);
	ExternE& externE(*itExternW);
	OGDF_ASSERT(externE.theNode == pathW.front()->source() ||
				externE.theNode == pathW.front()->target());

	// check, if a external path sharing the first pathW-edge exists
	SListIterator<int> itStart;
	SListIterator<node> itEnd = externE.endnodes.begin();
	SListIterator<SListPure<edge> > itPath = externE.externalPaths.begin();
	SListIterator<edge> itEdge;
	for (itStart = externE.startnodes.begin(); itStart.valid(); ++itStart) {
		if (*itStart != m_dfi[pathW.front()->opposite(info.w)]) {
			++itEnd;
			++itPath;
			continue;
		}

		// if path was preprocessed, copy path
		node endnodeWExtern = *itEnd;
		if (!(*itPath).empty()) {
			B.edgeList = (*itPath);
		} else {
			// else traverse external Path starting with z. forbid edges starting at W,
			// that are different from the edge w->z.
			adjEntry adj = adjToLowestNodeBelow(endnodeWExtern,*itStart);
			B.edgeList.pushFront(adj->theEdge());
			addDFSPathReverse(B.edgeList,adj->theNode(),info.w);

			// copy list
			*itPath = B.edgeList;
		}

		// truncate pathZ from edges already contained in pathW
		OGDF_ASSERT(B.edgeList.front() == pathW.front());
		truncateEdgelist(B.edgeList,pathW);

		// add external face edges
		addExternalFacePath(B.edgeList,k.externalFacePath);

		// compute dfi-minimum and maximum of all three paths to node Ancestor u
		// add the dfs-path from minimum to maximum
		node min,max;
		if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
			min = endnodeX;
			max = endnodeY;
		} else {
			min = endnodeY;
			max = endnodeX;
		}
		if (m_dfi[endnodeWExtern] < m_dfi[min]) {
			min = endnodeWExtern;
		} else {
			if (m_dfi[endnodeWExtern] > m_dfi[max]) max = endnodeWExtern;
		}
		addDFSPath(B.edgeList,max,min);

		// copy other paths to subdivision
		SListConstIterator<edge> it;
		for (it = pathX.begin(); it.valid(); ++it) B.edgeList.pushBack(*it);
		for (it = pathY.begin(); it.valid(); ++it) B.edgeList.pushBack(*it);
		for (it = pathW.begin(); it.valid(); ++it) B.edgeList.pushBack(*it);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,B.edgeList) == ExtractKuratowskis::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,B.edgeList,output));
		if (info.minorType & WInfo::A) {
			B.subdivisionType = KuratowskiWrapper::AB;
		} else B.subdivisionType = KuratowskiWrapper::B;
		B.V = k.V;
		output.pushBack(B);
		B.edgeList.clear();

//		break;
	}
}

// extracts a type B minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorBBundles(
				SList<KuratowskiWrapper>& output,
				NodeArray<int>& nodeflags,
				const int nodemarker,
				const KuratowskiStructure& k,
				EdgeArray<int>& flags,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	KuratowskiWrapper B;
	OGDF_ASSERT(flags[pathW.back()] & DynamicBacktrack::pertinentPath);

	// check, if pertinent pathW (w->u) traverses node z
	if (!(flags[pathW.back()] & DynamicBacktrack::externalPath)) return;

	// mark single pathW in flags, so that pathW and the externalPath
	// don't interfere later
	SListConstIterator<edge> itE;
	for (itE = pathW.begin(); itE.valid(); ++itE) {
		flags[*itE] |= DynamicBacktrack::singlePath;
		nodeflags[(*itE)->source()] = nodemarker;
		nodeflags[(*itE)->target()] = nodemarker;
	}

	// traverse all possible external Paths out of z. forbid edges starting at W,
	// that are different from the edge w->z
	node endnodeWExtern;
	DynamicBacktrack backtrackExtern(m_g,m_dfi,flags);
	backtrackExtern.init(info.w,k.V,true,DynamicBacktrack::externalPath,
						DynamicBacktrack::externalPath,pathW.back(),NULL);
	while (backtrackExtern.addNextPathExclude(B.edgeList,endnodeWExtern,nodeflags,
										nodemarker,DynamicBacktrack::singlePath)) {
		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			break;

		// add external face edges
		addExternalFacePath(B.edgeList,k.externalFacePath);

		// compute dfi-minimum and maximum of all three paths to node Ancestor u
		// add the dfs-path from minimum to maximum
		node min,max;
		if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
			min = endnodeX;
			max = endnodeY;
		} else {
			min = endnodeY;
			max = endnodeX;
		}
		if (m_dfi[endnodeWExtern] < m_dfi[min]) min = endnodeWExtern;
			else if (m_dfi[endnodeWExtern] > m_dfi[max]) max = endnodeWExtern;
		addDFSPath(B.edgeList,max,min);

		// copy other paths to subdivision
		SListConstIterator<edge> it;
		for (it = pathX.begin(); it.valid(); ++it) B.edgeList.pushBack(*it);
		for (it = pathY.begin(); it.valid(); ++it) B.edgeList.pushBack(*it);
		for (it = pathW.begin(); it.valid(); ++it) B.edgeList.pushBack(*it);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,B.edgeList) == ExtractKuratowskis::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,B.edgeList,output));
		if (info.minorType & WInfo::A) {
			B.subdivisionType = KuratowskiWrapper::AB;
		} else B.subdivisionType = KuratowskiWrapper::B;
		B.V = k.V;
		output.pushBack(B);
		B.edgeList.clear();
	}

	// delete marked single pathW
	for (itE = pathW.begin(); itE.valid(); ++itE) {
		flags[*itE] &= ~DynamicBacktrack::singlePath;
	}
}

// extracts a type C minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorC(
				SList<KuratowskiWrapper>& output,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper C;
	SListPure<edge> tempC;

	// the case, that px is above stopX
	OGDF_ASSERT(info.pxAboveStopX || info.pyAboveStopY);
	SListConstIterator<adjEntry> itE;

	// add the path from v to u, this is only possible after computation
	// of pathX and pathY
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(tempC,k.V,endnodeX);
	} else addDFSPath(tempC,k.V,endnodeY);

	// add highestFacePath of wNode
	OGDF_ASSERT(info.highestXYPath->size() >= 2);
	for (itE=info.highestXYPath->begin().succ(); itE.valid(); ++itE) {
		tempC.pushBack((*itE)->theEdge());
	}

	// the case, that px is above stopX
	if (info.pxAboveStopX) {
		C.edgeList = tempC;

		// add the external face path edges except the path from py/stopY to R
		node end;
		if (info.pyAboveStopY) {
			end = info.highestXYPath->back()->theNode();
		} else end = k.stopY;
		for (itE=k.externalFacePath.begin(); itE.valid(); ++itE) {
			C.edgeList.pushBack((*itE)->theEdge());
			if ((*itE)->theNode() == end) break;
		}

		// copy other paths to subdivision
		SListConstIterator<edge> it;
		for (it = pathX.begin(); it.valid(); ++it) C.edgeList.pushBack(*it);
		for (it = pathY.begin(); it.valid(); ++it) C.edgeList.pushBack(*it);
		for (it = pathW.begin(); it.valid(); ++it) C.edgeList.pushBack(*it);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,C.edgeList) == ExtractKuratowskis::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,C.edgeList,output));
		if (info.minorType & WInfo::A) {
			C.subdivisionType = KuratowskiWrapper::AC;
		} else C.subdivisionType = KuratowskiWrapper::C;
		C.V = k.V;
		output.pushBack(C);
		C.edgeList.clear();
	}

	// the case, that py is above stopY
	if (info.pyAboveStopY) {
		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			return;

		C.edgeList = tempC;

		// add the external face path edges except the path from px/stopX to R
		node start;
		if (info.pxAboveStopX) {
			start = info.highestXYPath->front()->theNode();
		} else start = k.stopX;
		bool after = false;
		for (itE=k.externalFacePath.begin(); itE.valid(); ++itE) {
			if (after) {
				C.edgeList.pushBack((*itE)->theEdge());
			} else if ((*itE)->theNode() == start) after = true;
		}

		// copy other paths to subdivision
		SListConstIterator<edge> it;
		for (it = pathX.begin(); it.valid(); ++it) C.edgeList.pushBack(*it);
		for (it = pathY.begin(); it.valid(); ++it) C.edgeList.pushBack(*it);
		for (it = pathW.begin(); it.valid(); ++it) C.edgeList.pushBack(*it);
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,C.edgeList) == ExtractKuratowskis::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,C.edgeList,output));
		if (info.minorType & WInfo::A) {
			C.subdivisionType = KuratowskiWrapper::AC;
		} else C.subdivisionType = KuratowskiWrapper::C;
		C.V = k.V;
		output.pushBack(C);
	}
}

// extracts a type D minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorD(
				SList<KuratowskiWrapper>& output,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper D;

	// add the path from v to u, this is only possible after computation of pathX and pathY
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(D.edgeList,k.V,endnodeX);
	} else addDFSPath(D.edgeList,k.V,endnodeY);

	// add the external face path edges except the part from R to the nearest of
	// the two nodes stopX and px resp. the part to stopY/py
	node start;
	if (info.pxAboveStopX) {
		start = info.highestXYPath->front()->theNode();
	} else start = k.stopX;
	node end;
	if (info.pyAboveStopY) {
		end = info.highestXYPath->back()->theNode();
	} else end = k.stopY;
	node temp;
	SListConstIterator<adjEntry> itE;
	bool between = false;
	for (itE=k.externalFacePath.begin(); itE.valid(); ++itE) {
		temp = (*itE)->theNode();
		if (between) D.edgeList.pushBack((*itE)->theEdge());
		if (temp == start) {
			between = true;
		} else if (temp == end) between = false;
	}

	// add highestFacePath of wNode
	OGDF_ASSERT(info.highestXYPath->size() >= 2);
	for (itE=info.highestXYPath->begin().succ(); itE.valid(); ++itE) {
		D.edgeList.pushBack((*itE)->theEdge());
	}

	// add path from first zNode to R
	OGDF_ASSERT(!info.zPath->empty());
	for (itE=info.zPath->begin().succ(); itE.valid(); ++itE) {
		D.edgeList.pushBack((*itE)->theEdge());
	}

	// copy other paths to subdivision
	SListConstIterator<edge> it;
	for (it = pathX.begin(); it.valid(); ++it) D.edgeList.pushBack(*it);
	for (it = pathY.begin(); it.valid(); ++it) D.edgeList.pushBack(*it);
	for (it = pathW.begin(); it.valid(); ++it) D.edgeList.pushBack(*it);
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,D.edgeList) == ExtractKuratowskis::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,D.edgeList,output));
		if (info.minorType & WInfo::A) {
			D.subdivisionType = KuratowskiWrapper::AD;
		} else D.subdivisionType = KuratowskiWrapper::D;
	D.V = k.V;
	output.pushBack(D);
}

// extracts a subtype E1 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE1(
				SList<KuratowskiWrapper>& output,
				int before,
				//const node z,
				const node px,
				const node py,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	OGDF_ASSERT(before == -1 || before == 1);
	KuratowskiWrapper E1;
	SListConstIterator<edge> itE;

	// add highestFacePath of wNode
	SListConstIterator<adjEntry> it;
	for (it=info.highestXYPath->begin().succ(); it.valid(); ++it)
		E1.edgeList.pushBack((*it)->theEdge());

	if (before == -1) {
		// z is before w on external face path

		// add pathY
		for (itE = pathY.begin(); itE.valid(); ++itE) E1.edgeList.pushBack(*itE);

		// add the path from v to u, this is only possible after computation of
		// pathX and pathY
		if (m_dfi[endnodeZ] < m_dfi[endnodeY]) {
			addDFSPath(E1.edgeList,k.V,endnodeZ);
		} else addDFSPath(E1.edgeList,k.V,endnodeY);

		// add the external face path edges except the part from stopY/py to R
		node stop;
		if (info.pyAboveStopY) {
			stop = py;
		} else stop = k.stopY;
		for (it=k.externalFacePath.begin(); it.valid(); ++it) {
			E1.edgeList.pushBack((*it)->theEdge());
			if ((*it)->theNode() == stop) break;
		}
	} else {
		// z is after w on external face path

		// if minor A occurs, add the dfs-path from node RReal to V, that isn't anymore
		// involved because of removing pathY
		if (k.RReal != k.V) addDFSPath(E1.edgeList,k.RReal,k.V);

		// add pathX
		for (itE = pathX.begin(); itE.valid(); ++itE) E1.edgeList.pushBack(*itE);

		// add the path from v to u, this is only possible after computation of
		// pathX and pathY
		if (m_dfi[endnodeZ] < m_dfi[endnodeX]) {
			addDFSPath(E1.edgeList,k.V,endnodeZ);
		} else addDFSPath(E1.edgeList,k.V,endnodeX);

		// add the external face path edges except the part from stopX/px to R
		node start;
		if (info.pxAboveStopX) {
			start = px;
		} else start = k.stopX;
		bool after = false;
		for (it=k.externalFacePath.begin(); it.valid(); ++it) {
			if (after) {
				E1.edgeList.pushBack((*it)->theEdge());
			} else if ((*it)->theNode() == start) after = true;
		}
	}

	// add pathW and pathZ
	for (itE = pathW.begin(); itE.valid(); ++itE) E1.edgeList.pushBack(*itE);
	for (itE = pathZ.begin(); itE.valid(); ++itE) E1.edgeList.pushBack(*itE);
	// push this subdivision to kuratowskilist
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E1.edgeList) == ExtractKuratowskis::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E1.edgeList,output));
	if (info.minorType & WInfo::A) {
		E1.subdivisionType = KuratowskiWrapper::AE1;
	} else E1.subdivisionType = KuratowskiWrapper::E1;
	E1.V = k.V;
	output.pushBack(E1);
}

// extracts a subtype E2 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE2(
				SList<KuratowskiWrapper>& output,
				/*int before,
				const node z,
				const node px,
				const node py,*/
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				//const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ/*,
				const node endnodeZ*/
				)
{
	OGDF_ASSERT(!m_avoidE2Minors);

	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E2;

	// add the path from v to u
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		addDFSPath(E2.edgeList,k.V,endnodeX);
	} else addDFSPath(E2.edgeList,k.V,endnodeY);

	// add the external face path edges
	SListConstIterator<adjEntry> it;
	for (it=k.externalFacePath.begin(); it.valid(); ++it)
		E2.edgeList.pushBack((*it)->theEdge());

	// add pathX, pathY and pathZ
	SListConstIterator<edge> itE;
	for (itE = pathX.begin(); itE.valid(); ++itE) E2.edgeList.pushBack(*itE);
	for (itE = pathY.begin(); itE.valid(); ++itE) E2.edgeList.pushBack(*itE);
	for (itE = pathZ.begin(); itE.valid(); ++itE) E2.edgeList.pushBack(*itE);
	// push this subdivision to kuratowskilist
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E2.edgeList) == ExtractKuratowskis::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E2.edgeList,output));
	if (info.minorType & WInfo::A) {
		E2.subdivisionType = KuratowskiWrapper::AE2;
	} else E2.subdivisionType = KuratowskiWrapper::E2;
	E2.V = k.V;
	output.pushBack(E2);
}

// extracts a subtype E3 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE3(
				SList<KuratowskiWrapper>& output,
				int before,
				const node z,
				const node px,
				const node py,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E3;
	OGDF_ASSERT(endnodeX != endnodeY);

	// add pathZ
	SListConstIterator<edge> itE;
	for (itE = pathZ.begin(); itE.valid(); ++itE) E3.edgeList.pushBack(*itE);

	// add highestFacePath px <-> py
	SListConstIterator<adjEntry> it;
	for (it=info.highestXYPath->begin().succ(); it.valid(); ++it)
		E3.edgeList.pushBack((*it)->theEdge());

	// check, if endnodeX or endnodeY is descendant
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		OGDF_ASSERT(m_dfi[endnodeZ] < m_dfi[endnodeY]);

		// add the path from v to u
		if (m_dfi[endnodeX] < m_dfi[endnodeZ]) {
			addDFSPath(E3.edgeList,k.V,endnodeX);
		} else addDFSPath(E3.edgeList,k.V,endnodeZ);

		// add the external face path edges except max(px,stopX)<->min(z,w) and v<->nearest(py,stopY)
		node temp,start1,end1,start2;
		if (info.pxAboveStopX) {
			start1 = k.stopX;
		} else start1 = px;
		if (before<=0) {
			end1 = z;
		} else end1 = info.w;
		if (info.pyAboveStopY) {
			start2 = py;
		} else start2 = k.stopY;
		bool between = false;
		for (it=k.externalFacePath.begin(); it.valid(); ++it) {
			temp = (*it)->theNode();
			if (!between) E3.edgeList.pushBack((*it)->theEdge());
			if (temp == start1) between = true;
				else if (temp == start2) break;
				else if (temp == end1) between = false;
		}
	} else {
		OGDF_ASSERT(m_dfi[endnodeZ] < m_dfi[endnodeX]);

		// add the path from v to u
		if (m_dfi[endnodeY] < m_dfi[endnodeZ]) {
			addDFSPath(E3.edgeList,k.V,endnodeY);
		} else addDFSPath(E3.edgeList,k.V,endnodeZ);

		// add the external face path edges except v<->min(px,stopX) and max(w,z)<->nearest(py,stopY)
		node temp,end1,start2,end2;
		if (info.pxAboveStopX) {
			end1 = px;
		} else end1 = k.stopX;
		if (before>0) {
			start2 = z;
		} else start2 = info.w;
		if (info.pyAboveStopY) {
			end2 = k.stopY;
		} else end2 = py;
		bool between = true;
		for (it=k.externalFacePath.begin(); it.valid(); ++it) {
			temp = (*it)->theNode();
			if (!between) E3.edgeList.pushBack((*it)->theEdge());
			if (temp == end1) between = false;
				else if (temp == start2) between = true;
				else if (temp == end2) between = false;
		}
	}

	// add pathX, pathY and pathW
	for (itE = pathX.begin(); itE.valid(); ++itE) E3.edgeList.pushBack(*itE);
	for (itE = pathY.begin(); itE.valid(); ++itE) E3.edgeList.pushBack(*itE);
	for (itE = pathW.begin(); itE.valid(); ++itE) E3.edgeList.pushBack(*itE);
	// push this subdivision to kuratowskilist
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E3.edgeList) == ExtractKuratowskis::K33);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E3.edgeList,output));
	if (info.minorType & WInfo::A) {
		E3.subdivisionType = KuratowskiWrapper::AE3;
	} else E3.subdivisionType = KuratowskiWrapper::E3;
	E3.V = k.V;
	output.pushBack(E3);
}

// extracts a subtype E4 minor.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE4(
				SList<KuratowskiWrapper>& output,
				int before,
				const node z,
				const node px,
				const node py,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E4;
	SListPure<edge> tempE4;
	OGDF_ASSERT((px != k.stopX && !info.pxAboveStopX) ||
				(py != k.stopY && !info.pyAboveStopY));

	// add pathZ
	SListConstIterator<edge> itE;
	for (itE = pathZ.begin(); itE.valid(); ++itE) tempE4.pushBack(*itE);

	// add highestFacePath px <-> py
	SListConstIterator<adjEntry> it;
	for (it = info.highestXYPath->begin().succ(); it.valid(); ++it)
		tempE4.pushBack((*it)->theEdge());

	// compute dfi-minimum and maximum of all three paths to node Ancestor u
	// add the dfs-path from minimum to maximum
	node min,max;
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		min = endnodeX;
		max = endnodeY;
	} else {
		min = endnodeY;
		max = endnodeX;
	}
	if (m_dfi[endnodeZ] < m_dfi[min]) min = endnodeZ;
		else if (m_dfi[endnodeZ] > m_dfi[max]) max = endnodeZ;
	addDFSPath(tempE4,max,min);

	if (px != k.stopX && !info.pxAboveStopX) {
		E4.edgeList = tempE4;

		// add the external face path edges except max(w,z)<->min(py,stopY)
		node temp,start,end;
		if (before<=0) {
			start = info.w;
		} else start = z;
		if (info.pyAboveStopY) {
			end = k.stopY;
		} else end = py;
		bool between = false;
		for (it=k.externalFacePath.begin(); it.valid(); ++it) {
			temp = (*it)->theNode();
			if (!between) E4.edgeList.pushBack((*it)->theEdge());
			if (temp == start) between = true;
				else if (temp == end) between = false;
		}

		// add pathX, pathY and pathW
		for (itE = pathX.begin(); itE.valid(); ++itE) E4.edgeList.pushBack(*itE);
		for (itE = pathY.begin(); itE.valid(); ++itE) E4.edgeList.pushBack(*itE);
		for (itE = pathW.begin(); itE.valid(); ++itE) E4.edgeList.pushBack(*itE);
		// push this subdivision to kuratowski-list
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E4.edgeList) == ExtractKuratowskis::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E4.edgeList,output));
		if (info.minorType & WInfo::A) {
			E4.subdivisionType = KuratowskiWrapper::AE4;
		} else E4.subdivisionType = KuratowskiWrapper::E4;
		E4.V = k.V;
		output.pushBack(E4);
	}

	if (py != k.stopY && !info.pyAboveStopY) {
		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			return;

		E4.edgeList = tempE4;

		// add the external face path edges except max(px,stopX)<->min(w,z)
		node temp,start,end;
		if (info.pxAboveStopX) {
			start = k.stopX;
		} else start = px;
		if (before <= 0) {
			end = z;
		} else end = info.w;

		bool between = false;
		for (it=k.externalFacePath.begin(); it.valid(); ++it) {
			temp = (*it)->theNode();
			if (!between) E4.edgeList.pushBack((*it)->theEdge());
			if (temp == start) between = true;
				else if (temp == end) between = false;
		}

		// add pathX, pathY and pathW
		for (itE = pathX.begin(); itE.valid(); ++itE) E4.edgeList.pushBack(*itE);
		for (itE = pathY.begin(); itE.valid(); ++itE) E4.edgeList.pushBack(*itE);
		for (itE = pathW.begin(); itE.valid(); ++itE) E4.edgeList.pushBack(*itE);
		// push this subdivision to kuratowski-list
		OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E4.edgeList) == ExtractKuratowskis::K33);
		OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E4.edgeList,output));
		if (info.minorType & WInfo::A) {
			E4.subdivisionType = KuratowskiWrapper::AE4;
		} else E4.subdivisionType = KuratowskiWrapper::E4;
		E4.V = k.V;
		output.pushBack(E4);
	}
}

// extracts a subtype E5 minor (the only minortype which represents a K5).
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE5(
				SList<KuratowskiWrapper>& output,
				/*int before,
				const node z,
				const node px,
				const node py,*/
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW,
				const SListPure<edge>& pathZ,
				const node endnodeZ)
{
	// check, if we have found enough subdivisions
	if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
				output.size() >= m_embeddingGrade)
		return;

	KuratowskiWrapper E5;
	//OGDF_ASSERT(px==k.stopX && py==k.stopY && z==info.w && k.V == k.RReal);
	OGDF_ASSERT((endnodeX == endnodeY && m_dfi[endnodeZ] <= m_dfi[endnodeX]) ||
				(endnodeX == endnodeZ && m_dfi[endnodeY] <= m_dfi[endnodeX]) ||
				(endnodeY == endnodeZ && m_dfi[endnodeX] <= m_dfi[endnodeY]));

	// compute dfi-minimum of all three paths to node Ancestor u and
	// add the dfs-path from minimum to V
	node min;
	if (m_dfi[endnodeX] < m_dfi[endnodeY]) {
		min = endnodeX;
	} else if (m_dfi[endnodeY] < m_dfi[endnodeZ]) {
		min = endnodeY;
	} else {
		min = endnodeZ;
	}
	addDFSPath(E5.edgeList,k.V,min);

	// add pathZ
	SListConstIterator<edge> itE;
	for (itE = pathZ.begin(); itE.valid(); ++itE) E5.edgeList.pushBack(*itE);

	// add highestFacePath px <-> py
	SListConstIterator<adjEntry> it;
	for (it=info.highestXYPath->begin().succ(); it.valid(); ++it)
		E5.edgeList.pushBack((*it)->theEdge());

	// add the external face path edges
	for (it=k.externalFacePath.begin(); it.valid(); ++it) {
		E5.edgeList.pushBack((*it)->theEdge());
	}

	// add pathX, pathY and pathW
	for (itE = pathX.begin(); itE.valid(); ++itE) E5.edgeList.pushBack(*itE);
	for (itE = pathY.begin(); itE.valid(); ++itE) E5.edgeList.pushBack(*itE);
	for (itE = pathW.begin(); itE.valid(); ++itE) E5.edgeList.pushBack(*itE);
	// push this subdivision to kuratowski-list
	OGDF_ASSERT(whichKuratowski(m_g,m_dfi,E5.edgeList) == ExtractKuratowskis::K5);
	OGDF_ASSERT(!m_avoidE2Minors || isANewKuratowski(m_g,E5.edgeList,output));
	E5.subdivisionType = KuratowskiWrapper::E5;
	E5.V = k.V;
	output.pushBack(E5);
}

// extracts a type E minor through splitting in different subtypes.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorE(
				SList<KuratowskiWrapper>& output,
				bool firstXPath,
				bool firstYPath,
				bool firstWPath,
				bool firstWOnHighestXY,
				const KuratowskiStructure& k,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// find external paths for each extern node z on the lower external face
	OGDF_ASSERT(info.externEStart.valid() && info.externEEnd.valid());

	int before = -1; // -1= before, 0=equal, 1=after
	SListConstIterator<edge> itW;
	SListConstIterator<ExternE> it;
	node px = info.highestXYPath->front()->theNode();
	node py = info.highestXYPath->back()->theNode();

	adjEntry temp;
	node z;
	SListPure<edge> pathZ;
	node endnodeZ;
	SListConstIterator<node> itZEndnode;
	SListConstIterator<int> itZStartnode;
	SListConstIterator<SListPure<edge> > itEPath;

	// consider only the nodes between px and py
	for (it = info.externEStart; it.valid(); ++it) {
		const ExternE& externE(*it);
		z = externE.theNode;

		if (z == info.w) {
			OGDF_ASSERT(z == pathW.front()->source() || z == pathW.front()->target());
			// z = wNode
			before = 0;

			itZStartnode = externE.startnodes.begin();
			itEPath = externE.externalPaths.begin();
			for (itZEndnode = externE.endnodes.begin(); itZEndnode.valid();
											++itZEndnode,++itZStartnode,++itEPath) {
				endnodeZ = *itZEndnode;
				SListPure<edge>& externalPath(const_cast<SListPure<edge>& >(*itEPath));

				if (!externalPath.empty()) {
					// get preprocessed path
					pathZ = externalPath;
				} else {
					temp = adjToLowestNodeBelow(endnodeZ,*itZStartnode);
					pathZ.clear();
					pathZ.pushFront(temp->theEdge());
					addDFSPathReverse(pathZ,temp->theNode(),z);

					// copy path
					externalPath = pathZ;
				}

				// minortype E2 on z=wNode
				if (!m_avoidE2Minors && firstWPath && firstWOnHighestXY &&
						m_dfi[endnodeZ] > m_dfi[endnodeX] &&
						m_dfi[endnodeZ] > m_dfi[endnodeY]) {
						extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
										endnodeX,pathY,endnodeY,/*pathW,*/pathZ/*,endnodeZ*/);
				}

				// truncate pathZ from edges already contained in pathW
				truncateEdgelist(pathZ,pathW);

				// minortype E3 on z=wNode
				if (endnodeX != endnodeY &&
						(m_dfi[endnodeX] > m_dfi[endnodeZ] || m_dfi[endnodeY] > m_dfi[endnodeZ])) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4 on z=wNode
				if ((px != k.stopX && !info.pxAboveStopX) ||
						(py != k.stopY && !info.pyAboveStopY)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}

				// minortype E5 (K5)
				if (px == k.stopX && py == k.stopY && k.V == k.RReal &&
						((endnodeX == endnodeY && m_dfi[endnodeZ] <= m_dfi[endnodeX]) ||
						 (endnodeX == endnodeZ && m_dfi[endnodeY] <= m_dfi[endnodeX]) ||
						 (endnodeY == endnodeZ && m_dfi[endnodeX] <= m_dfi[endnodeY]))) {
					// check, if pathZ shares no edge with pathW
					if (*itZStartnode != m_dfi[pathW.front()->opposite(z)]) {
						extractMinorE5(output,/*before,z,px,py,*/k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
					}
				}
			}
		} else {
			// z != wNode, check position of node z
			if (z == info.firstExternEAfterW) before = 1;
			OGDF_ASSERT(before != 0);
			OGDF_ASSERT(z != pathW.front()->source() && z != pathW.front()->target());

			itZStartnode = externE.startnodes.begin();
			for (itZEndnode = externE.endnodes.begin(); itZEndnode.valid();
													 ++itZEndnode,++itZStartnode) {
				endnodeZ = *itZEndnode;

				temp = adjToLowestNodeBelow(endnodeZ,*itZStartnode);
				pathZ.clear();
				pathZ.pushFront(temp->theEdge());
				addDFSPathReverse(pathZ,temp->theNode(),z);

				// split in minorE-subtypes

				// minortype E1
				if ((before == -1 && firstXPath) || (before == 1 && firstYPath)) {
					extractMinorE1(output,before,/*z,*/px,py,k,info,pathX,
								endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E2
				if (!m_avoidE2Minors && firstWPath && firstWOnHighestXY
								&& m_dfi[endnodeZ] > m_dfi[endnodeX]
								&& m_dfi[endnodeZ] > m_dfi[endnodeY]) {
					extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
									endnodeX,pathY,endnodeY,/*pathW,*/pathZ/*,endnodeZ*/);
				}
				// minortype E3
				if (endnodeX != endnodeY && (m_dfi[endnodeX] > m_dfi[endnodeZ] ||
											m_dfi[endnodeY] > m_dfi[endnodeZ])) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4
				if ((px != k.stopX && !info.pxAboveStopX) ||
									(py != k.stopY && !info.pyAboveStopY)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
			}
		}

		// check if last node was reached
		if (it == info.externEEnd) break;
	}
}

// extracts a type E minor through splitting in different subtypes.
// each virtual node has to be merged into its real counterpart.
void ExtractKuratowskis::extractMinorEBundles(
				SList<KuratowskiWrapper>& output,
				bool firstXPath,
				bool firstYPath,
				bool firstWPath,
				bool firstWOnHighestXY,
				NodeArray<int>& nodeflags,
				const int nodemarker,
				const KuratowskiStructure& k,
				EdgeArray<int>& flags,
				const WInfo& info,
				const SListPure<edge>& pathX,
				const node endnodeX,
				const SListPure<edge>& pathY,
				const node endnodeY,
				const SListPure<edge>& pathW)
{
	// perform backtracking for each extern node z on the lower external face
	OGDF_ASSERT(info.externEStart.valid() && info.externEEnd.valid());
	SListPure<edge> pathZ;
	node z;
	node endnodeZ;
	int before = -1; // -1= before, 0=equal to wNode, 1=after
	SListConstIterator<edge> itW;
	SListConstIterator<ExternE> it;
	node px = info.highestXYPath->front()->theNode();
	node py = info.highestXYPath->back()->theNode();
	DynamicBacktrack backtrackZ(m_g,m_dfi,flags);

	// mark all nodes of the single pathW in flags, so that pathW and
	// the externalPath don't interfere later
	for (itW = pathW.begin(); itW.valid(); ++itW) {
		flags[*itW] |= DynamicBacktrack::singlePath;
		nodeflags[(*itW)->source()] = nodemarker;
		nodeflags[(*itW)->target()] = nodemarker;
	}

	// consider only the nodes between px and py
	for (it = info.externEStart; it.valid(); ++it) {
		z = (*it).theNode;

		if (z == info.w) {
			OGDF_ASSERT(z == pathW.back()->source() || z == pathW.back()->target());
			// z = wNode
			before = 0;

			// minortype E2 on z=wNode
			// on the first pathW: consider all pathsZ
			if (!m_avoidE2Minors && firstWPath && firstWOnHighestXY) {
				backtrackZ.init(z,k.V,true,DynamicBacktrack::externalPath,
								DynamicBacktrack::externalPath,NULL,NULL);
				while (backtrackZ.addNextPath(pathZ,endnodeZ)) {
					if (m_dfi[endnodeZ] > m_dfi[endnodeX] &&
						m_dfi[endnodeZ] > m_dfi[endnodeY]) {
						extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
										endnodeX,pathY,endnodeY/*,pathW*/,pathZ/*,endnodeZ*/);
					}
				}
			}

			backtrackZ.init(z,k.V,true,DynamicBacktrack::externalPath,
							DynamicBacktrack::externalPath,NULL,NULL);
			while (backtrackZ.addNextPathExclude(pathZ,endnodeZ,
						nodeflags,nodemarker,DynamicBacktrack::singlePath)) {
				// minortype E3 on z=wNode
				if (endnodeX != endnodeY && (m_dfi[endnodeX] > m_dfi[endnodeZ] ||
											m_dfi[endnodeY] > m_dfi[endnodeZ])) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4 on z=wNode
				if ((px != k.stopX && !info.pxAboveStopX) ||
						(py != k.stopY && !info.pyAboveStopY)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E5 (K5)
				if (px == k.stopX && py == k.stopY && k.V == k.RReal &&
						((endnodeX == endnodeY && m_dfi[endnodeZ] <= m_dfi[endnodeX]) ||
						 (endnodeX == endnodeZ && m_dfi[endnodeY] <= m_dfi[endnodeX]) ||
						 (endnodeY == endnodeZ && m_dfi[endnodeX] <= m_dfi[endnodeY]))) {
					// instead of slower code:
					//backtrackZ.init(z,k.V,true,DynamicBacktrack::externalPath,
					//					DynamicBacktrack::externalPath,NULL,pathW.back());
					//while (backtrackZ.addNextPathExclude(pathZ,endnodeZ,nodeflags,nodemarker,0)) {
					if (pathZ.back() != pathW.back() &&
						(pathZ.back()->source() == z || pathZ.back()->target() == z)) {
						extractMinorE5(output,/*before,z,px,py,*/k,info,pathX,
										endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
					}
				}
			}
		} else {
			// z != wNode, check position of node z
			if (z == info.firstExternEAfterW) before = 1;
			OGDF_ASSERT(before != 0);
			OGDF_ASSERT(z != pathW.back()->source() && z != pathW.back()->target());

			backtrackZ.init(z,k.V,true,DynamicBacktrack::externalPath,
							DynamicBacktrack::externalPath,NULL,NULL);
			while (backtrackZ.addNextPath(pathZ,endnodeZ)) {
				// split in minorE-subtypes

				// minortype E1
				if ((before == -1 && firstXPath) || (before == 1 && firstYPath)) {
					extractMinorE1(output,before,/*z,*/px,py,k,info,pathX,
								endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E2
				if (!m_avoidE2Minors && firstWPath && firstWOnHighestXY
								&& m_dfi[endnodeZ] > m_dfi[endnodeX]
								&& m_dfi[endnodeZ] > m_dfi[endnodeY]) {
					extractMinorE2(output,/*before,z,px,py,*/k,info,pathX,
									endnodeX,pathY,endnodeY,/*pathW,*/pathZ/*,endnodeZ*/);
				}
				// minortype E3
				if (endnodeX != endnodeY && (m_dfi[endnodeX] > m_dfi[endnodeZ] ||
											m_dfi[endnodeY] > m_dfi[endnodeZ])) {
					extractMinorE3(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
				// minortype E4
				if ((px != k.stopX && !info.pxAboveStopX) ||
									(py != k.stopY && !info.pyAboveStopY)) {
					extractMinorE4(output,before,z,px,py,k,info,pathX,
									endnodeX,pathY,endnodeY,pathW,pathZ,endnodeZ);
				}
			}
		}

		// check if last node was reached
		if (it == info.externEEnd) break;

		// check, if we have found enough subdivisions
		if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
					output.size() >= m_embeddingGrade)
			break;
	}

	// delete marked single pathW
	for (itW = pathW.begin(); itW.valid(); ++itW) {
		flags[*itW] &= ~DynamicBacktrack::singlePath;
	}
}

// extracts all kuratowski subdivisions and adds them to output
void ExtractKuratowskis::extract(
				const SListPure<KuratowskiStructure>& allKuratowskis,
				SList<KuratowskiWrapper>& output)
{
	SListConstIterator<KuratowskiStructure> itAll;
	SListConstIterator<WInfo> itInfo;
	SListConstIterator<edge> itS;
	SListConstIterator<SListPure<edge> > itL;

	SListPure<edge> pathX,pathY;
	node endnodeX,endnodeY;
	adjEntry temp;

	SListConstIterator<node> itXEndnode;
	SListConstIterator<node> itYEndnode;
	SListConstIterator<int> itXStartnode;
	SListConstIterator<int> itYStartnode;

	// consider all different kuratowski structures
	for (itAll=allKuratowskis.begin(); itAll.valid(); ++itAll) {
		const KuratowskiStructure& k(*itAll);

		// compute all possible external paths of stopX and stopY (pathX,pathY)
		bool firstXPath = true;
		itXStartnode = k.stopXStartnodes.begin();
		for (itXEndnode = k.stopXEndnodes.begin(); itXEndnode.valid(); ++itXEndnode) {
			endnodeX = *itXEndnode;
			pathX.clear();
			temp = adjToLowestNodeBelow(endnodeX,*(itXStartnode++));
			pathX.pushBack(temp->theEdge());
			addDFSPath(pathX,temp->theNode(),k.stopX);

			bool firstYPath = true;
			itYStartnode = k.stopYStartnodes.begin();
			for (itYEndnode = k.stopYEndnodes.begin(); itYEndnode.valid(); ++itYEndnode) {
				endnodeY = *itYEndnode;
				pathY.clear();
				temp = adjToLowestNodeBelow(endnodeY,*(itYStartnode++));
				pathY.pushBack(temp->theEdge());
				addDFSPath(pathY,temp->theNode(),k.stopY);

				// if minor A occurs, other minortypes are possible with adding
				// the dfs-path from node RReal to V
				if (k.RReal != k.V) addDFSPath(pathY,k.RReal,k.V);

				// consider all possible wNodes
				SListPure<adjEntry>* oldHighestXYPath = NULL;
				for (itInfo = k.wNodes.begin(); itInfo.valid(); ++itInfo) {
					const WInfo& info(*itInfo);

					// compute all possible internal paths of this wNode
					bool firstWPath = true; // avoid multiple identical subdivisions in E2
					for (itL = info.pertinentPaths.begin(); itL.valid(); ++itL) {
						const SListPure<edge>& pathW(*itL);
						OGDF_ASSERT(!pathX.empty() && !pathY.empty() && !pathW.empty());

						// extract minor A
						if (info.minorType & WInfo::A)
							extractMinorA(output,k,/*info,*/pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor B
						if (info.minorType & WInfo::B) {
							++m_nodeMarker;
							extractMinorB(output,/*m_wasHere,++m_nodeMarker,*/k,
										info,pathX,endnodeX,pathY,endnodeY,pathW);
						}

						// extract minor C
						if (info.minorType & WInfo::C)
							extractMinorC(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor D
						if (info.minorType & WInfo::D)
							extractMinorD(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor E including all subtypes
						if (info.minorType & WInfo::E) {
							extractMinorE(output,firstXPath,firstYPath,firstWPath,
										oldHighestXYPath!=info.highestXYPath,k,info,
										pathX,endnodeX,pathY,endnodeY,pathW);
						}

						if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
									output.size() >= m_embeddingGrade)
							return;
						firstWPath = false;
						// break;
					}
					oldHighestXYPath = info.highestXYPath;
				}
				firstYPath = false;
				// break;
			}
			firstXPath = false;
			// break;
		}
	}
}

// extracts all kuratowski subdivisions and adds them to output
void ExtractKuratowskis::extractBundles(
				const SListPure<KuratowskiStructure>& allKuratowskis,
				SList<KuratowskiWrapper>& output)
{
	SListConstIterator<KuratowskiStructure> itAll;
	SListConstIterator<WInfo> itInfo;
	SListConstIterator<edge> itS;

	SListPure<edge> pathX,pathY,pathW;
	node endnodeX,endnodeY;

	EdgeArray<int> flags(m_g,0);
	DynamicBacktrack backtrackX(m_g,m_dfi,flags);
	DynamicBacktrack backtrackY(m_g,m_dfi,flags);
	DynamicBacktrack backtrackW(m_g,m_dfi,flags);

	// consider all different kuratowski structures
	for (itAll=allKuratowskis.begin(); itAll.valid(); ++itAll) {
		const KuratowskiStructure& k(*itAll);

		// create pertinent and external flags
		for (itS = k.pertinentSubgraph.begin(); itS.valid(); ++itS)
			flags[*itS] |= DynamicBacktrack::pertinentPath;
		for (itS = k.externalSubgraph.begin(); itS.valid(); ++itS)
			flags[*itS] |= DynamicBacktrack::externalPath;

		// compute all possible external paths of stopX and stopY (pathX,pathY)
		bool firstXPath = true;
		backtrackX.init(k.stopX,k.V,true,DynamicBacktrack::externalPath,
						DynamicBacktrack::externalPath,NULL,NULL);
		while (backtrackX.addNextPath(pathX,endnodeX)) {
			bool firstYPath = true;
			backtrackY.init(k.stopY,k.V,true,DynamicBacktrack::externalPath,
							DynamicBacktrack::externalPath,NULL,NULL);
			while (backtrackY.addNextPath(pathY,endnodeY)) {

				// if minor A occurs, other minortypes are possible with adding
				// the dfs-path from node RReal to V
				if (k.RReal != k.V) addDFSPath(pathY,k.RReal,k.V);

				// consider all possible wNodes
				SListPure<adjEntry>* oldHighestXYPath = NULL;
				for (itInfo = k.wNodes.begin(); itInfo.valid(); ++itInfo) {
					const WInfo& info(*itInfo);

					// compute all possible internal paths of this wNode
					bool firstWPath = true; // avoid multiple identical subdivisions in E2
					backtrackW.init(info.w,k.V,false,DynamicBacktrack::pertinentPath,
									DynamicBacktrack::pertinentPath,NULL,NULL);
					node dummy;
					while (backtrackW.addNextPath(pathW,dummy)) {
						OGDF_ASSERT(!pathX.empty() && !pathY.empty() && !pathW.empty());

						// extract minor A
						if (info.minorType & WInfo::A)
							extractMinorA(output,k,/*info,*/pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor B
						if (info.minorType & WInfo::B)
							extractMinorBBundles(output,m_wasHere,++m_nodeMarker,k,flags,
										info,pathX,endnodeX,pathY,endnodeY,pathW);

						// extract minor C
						if (info.minorType & WInfo::C)
							extractMinorC(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor D
						if (info.minorType & WInfo::D)
							extractMinorD(output,k,info,pathX,endnodeX,pathY,
										endnodeY,pathW);

						// extract minor E including all subtypes
						if (info.minorType & WInfo::E) {
							extractMinorEBundles(output,firstXPath,firstYPath,firstWPath,
										oldHighestXYPath!=info.highestXYPath,
										m_wasHere,++m_nodeMarker,k,flags,info,
										pathX,endnodeX,pathY,endnodeY,pathW);
						}

						if (m_embeddingGrade > BoyerMyrvoldPlanar::doFindUnlimited &&
									output.size() >= m_embeddingGrade)
							return;
						firstWPath = false;
						// break;
					}
					oldHighestXYPath = info.highestXYPath;
				}
				firstYPath = false;
				// break;
			}
			firstXPath = false;
			// break;
		}

		// delete pertinent and external flags
		for (itS = k.pertinentSubgraph.begin(); itS.valid(); ++itS)
			flags[*itS] = 0;
		for (itS = k.externalSubgraph.begin(); itS.valid(); ++itS)
			flags[*itS] = 0;
	}
}


}
