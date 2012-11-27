/*
 * $Revision: 2565 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 17:14:54 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Linear time layout algorithm for trees (TreeLayout)
 * based on Walker's algorithm
 *
 * \author Christoph Buchheim
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


#include <ogdf/tree/TreeLayout.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/Math.h>


#include <ogdf/basic/AdjEntryArray.h>
#include <math.h>


#include <ogdf/basic/simple_graph_alg.h>
#include <ogdf/basic/Stack.h>


namespace ogdf {


TreeLayout::TreeLayout()
	:m_siblingDistance(20),
	 m_subtreeDistance(20),
	 m_levelDistance(50),
	 m_treeDistance(50),
	 m_orthogonalLayout(false),
	 m_orientation(topToBottom),
	 m_selectRoot(rootIsSource),
	 m_pGraph(0)
{ }


TreeLayout::TreeLayout(const TreeLayout &tl)
	:m_siblingDistance(tl.m_siblingDistance),
	 m_subtreeDistance(tl.m_subtreeDistance),
	 m_levelDistance(tl.m_levelDistance),
	 m_treeDistance(tl.m_treeDistance),
	 m_orthogonalLayout(tl.m_orthogonalLayout),
	 m_orientation(tl.m_orientation),
	 m_selectRoot(tl.m_selectRoot)
{ }


TreeLayout::~TreeLayout()
{ }


TreeLayout &TreeLayout::operator=(const TreeLayout &tl)
{
	m_siblingDistance  = tl.m_siblingDistance;
	m_subtreeDistance  = tl.m_subtreeDistance;
	m_levelDistance    = tl.m_levelDistance;
	m_treeDistance     = tl.m_treeDistance;
	m_orthogonalLayout = tl.m_orthogonalLayout;
	m_orientation      = tl.m_orientation;
	m_selectRoot       = tl.m_selectRoot;
	return *this;
}


// comparer class used for sorting adjacency entries according to their angle
class TreeLayout::AdjComparer
{
public:
	AdjComparer(const AdjEntryArray<double> &angle) {
		m_pAngle = &angle;
	}

	int compare(const adjEntry &adjX, const adjEntry &adjY) const {
		if ((*m_pAngle)[adjX] < (*m_pAngle)[adjY])
			return -1;
		else
			if ((*m_pAngle)[adjX] > (*m_pAngle)[adjY])
				return 1;
		else
			return 0;
	}
	OGDF_AUGMENT_COMPARER(adjEntry)

private:
	const AdjEntryArray<double> *m_pAngle;
};



void TreeLayout::setRoot(GraphAttributes &AG, Graph &tree)
{
	m_pGraph = &tree;

	NodeArray<bool> visited(tree,false);
	StackPure<node> S;

	node v;
	forall_nodes(v,tree)
	{
		if(visited[v]) continue;

		// process a new connected component
		node root = 0;
		S.push(v);

		while(!S.empty())
		{
			node x = S.pop();
			visited[x] = true;

			if(!root) {
				if(m_selectRoot == rootIsSource) {
					if (x->indeg() == 0)
						root = x;
				} else if (m_selectRoot == rootIsSink) {
					if (x->outdeg() == 0)
						root = x;
				} else { // selectByCoordinate
					root = x;
				}

			} else if(m_selectRoot == rootByCoord) {
				switch(m_orientation)
				{
				case bottomToTop:
					if(AG.y(x) < AG.y(root))
						root = x;
					break;
				case topToBottom:
					if(AG.y(x) > AG.y(root))
						root = x;
					break;
				case leftToRight:
					if(AG.x(x) < AG.x(root))
						root = x;
					break;
				case rightToLeft:
					if(AG.x(x) > AG.x(root))
						root = x;
					break;
				}
			}

			adjEntry adj;
			forall_adj(adj,x) {
				node w = adj->twinNode();
				if(!visited[w])
					S.push(w);
			}
		}

		if(root == 0) {
			undoReverseEdges(AG);
			OGDF_THROW_PARAM(PreconditionViolatedException, pvcForest);
		}

		adjustEdgeDirections(tree,root,0);
	}
}


void TreeLayout::adjustEdgeDirections(Graph &G, node v, node parent)
{
	adjEntry adj;
	forall_adj(adj,v) {
		node w = adj->twinNode();
		if(w == parent) continue;
		edge e = adj->theEdge();
		if(w != e->target()) {
			G.reverseEdge(e);
			m_reversedEdges.pushBack(e);
		}
		adjustEdgeDirections(G,w,v);
	}
}

void TreeLayout::callSortByPositions(GraphAttributes &AG, Graph &tree)
{
	OGDF_ASSERT(&tree == &(AG.constGraph()));

	if (!isFreeForest(tree))
		OGDF_THROW_PARAM(PreconditionViolatedException, pvcForest);

	setRoot(AG,tree);

	// stores angle of adjacency entry
	AdjEntryArray<double> angle(tree);

	AdjComparer cmp(angle);

	node v;
	forall_nodes(v,tree)
	{
		// position of node v
		double cx = AG.x(v);
		double cy = AG.y(v);

		adjEntry adj;
		forall_adj(adj,v)
		{
			// adjacent node
			node w = adj->twinNode();

			// relative position of w to v
			double dx = AG.x(w) - cx;
			double dy = AG.y(w) - cy;

			// if v and w lie on the same point ...
			if (dx == 0 && dy == 0) {
				angle[adj] = 0;
				continue;
			}

			if(m_orientation == leftToRight || m_orientation == rightToLeft)
				swap(dx,dy);
			if(m_orientation == topToBottom || m_orientation == rightToLeft)
				dy = -dy;

			// compute angle of adj
			double alpha = atan2(fabs(dx),fabs(dy));

			if(dx < 0) {
				if(dy < 0)
					angle[adj] = alpha;
				else
					angle[adj] = Math::pi - alpha;
			} else {
				if (dy > 0)
					angle[adj] = Math::pi + alpha;
				else
					angle[adj] = 2*Math::pi - alpha;
			}
		}

		// get list of all adjacency entries at v
		SListPure<adjEntry> entries;
		tree.adjEntries(v, entries);

		// sort entries according to angle
		entries.quicksort(cmp);

		// sort entries accordingly in tree
		tree.sort(v,entries);
	}

	// adjacency lists are now sorted, so we can apply the usual call
	call(AG);
}


void TreeLayout::call(GraphAttributes &AG)
{
	const Graph &tree = AG.constGraph();
	if(tree.numberOfNodes() == 0) return;

	if (!isForest(tree))
		OGDF_THROW_PARAM(PreconditionViolatedException, pvcForest);

	OGDF_ASSERT(m_siblingDistance > 0);
	OGDF_ASSERT(m_subtreeDistance > 0);
	OGDF_ASSERT(m_levelDistance > 0);

	// compute the tree structure
	List<node> roots;
	initializeTreeStructure(tree,roots);

	if(m_orientation == topToBottom || m_orientation == bottomToTop)
	{
		ListConstIterator<node> it;
		double minX = 0, maxX = 0;
		for(it = roots.begin(); it.valid(); ++it)
		{
			node root = *it;

			// compute x-coordinates
			firstWalk(root,AG,true);
			secondWalkX(root,-m_preliminary[root],AG);

			// compute y-coordinates
			computeYCoordinatesAndEdgeShapes(root,AG);

			if(it != roots.begin())
			{
				findMinX(AG,root,minX);

				double shift = maxX + m_treeDistance - minX;

				shiftTreeX(AG,root,shift);
			}

			findMaxX(AG,root,maxX);
		}

		// The computed layout draws a tree upwards. If we want to draw the
		// tree downwards, we simply invert all y-coordinates.
		if(m_orientation == topToBottom)
		{
			node v;
			forall_nodes(v,tree)
				AG.y(v) = -AG.y(v);

			edge e;
			forall_edges(e,tree) {
				ListIterator<DPoint> it;
				for(it = AG.bends(e).begin(); it.valid(); ++it)
					(*it).m_y = -(*it).m_y;
			}
		}

	} else {
		ListConstIterator<node> it;
		double minY = 0, maxY = 0;
		for(it = roots.begin(); it.valid(); ++it)
		{
			node root = *it;

			// compute y-coordinates
			firstWalk(root,AG,false);
			secondWalkY(root,-m_preliminary[root],AG);

			// compute y-coordinates
			computeXCoordinatesAndEdgeShapes(root,AG);

			if(it != roots.begin())
			{
				findMinY(AG,root,minY);

				double shift = maxY + m_treeDistance - minY;

				shiftTreeY(AG,root,shift);
			}

			findMaxY(AG,root,maxY);
		}

		// The computed layout draws a tree upwards. If we want to draw the
		// tree downwards, we simply invert all y-coordinates.
		if(m_orientation == rightToLeft)
		{
			node v;
			forall_nodes(v,tree)
				AG.x(v) = -AG.x(v);

			edge e;
			forall_edges(e,tree) {
				ListIterator<DPoint> it;
				for(it = AG.bends(e).begin(); it.valid(); ++it)
					(*it).m_x = -(*it).m_x;
			}
		}

	}

	// delete the tree structure
	deleteTreeStructure();

	// restore temporarily removed edges again
	undoReverseEdges(AG);
}

void TreeLayout::undoReverseEdges(GraphAttributes &AG)
{
	if(m_pGraph) {
		while(!m_reversedEdges.empty()) {
			edge e = m_reversedEdges.popFrontRet();
			m_pGraph->reverseEdge(e);
			AG.bends(e).reverse();
		}

		m_pGraph = 0;
	}
}

void TreeLayout::findMinX(GraphAttributes &AG, node root, double &minX)
{
	Stack<node> S;
	S.push(root);

	while(!S.empty())
	{
		node v = S.pop();

		double left = AG.x(v) - AG.width(v)/2;
		if(left < minX) minX = left;

		edge e;
		forall_adj_edges(e,v) {
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}

void TreeLayout::findMinY(GraphAttributes &AG, node root, double &minY)
{
	Stack<node> S;
	S.push(root);

	while(!S.empty())
	{
		node v = S.pop();

		double left = AG.y(v) - AG.height(v)/2;
		if(left < minY) minY = left;

		edge e;
		forall_adj_edges(e,v) {
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}


void TreeLayout::shiftTreeX(GraphAttributes &AG, node root, double shift)
{
	Stack<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.pop();

		AG.x(v) += shift;

		edge e;
		forall_adj_edges(e,v) {
			node w = e->target();
			if(w != v) {
				ListIterator<DPoint> itP;
				for(itP = AG.bends(e).begin(); itP.valid(); ++itP)
					(*itP).m_x += shift;
				S.push(w);
			}
		}
	}
}

void TreeLayout::shiftTreeY(GraphAttributes &AG, node root, double shift)
{
	Stack<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.pop();

		AG.y(v) += shift;

		edge e;
		forall_adj_edges(e,v) {
			node w = e->target();
			if(w != v) {
				ListIterator<DPoint> itP;
				for(itP = AG.bends(e).begin(); itP.valid(); ++itP)
					(*itP).m_y += shift;
				S.push(w);
			}
		}
	}
}


void TreeLayout::findMaxX(GraphAttributes &AG, node root, double &maxX)
{
	Stack<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.pop();

		double right = AG.x(v) + AG.width(v)/2;
		if(right > maxX) maxX = right;

		edge e;
		forall_adj_edges(e,v) {
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}

void TreeLayout::findMaxY(GraphAttributes &AG, node root, double &maxY)
{
	Stack<node> S;
	S.push(root);
	while(!S.empty())
	{
		node v = S.pop();

		double right = AG.y(v) + AG.height(v)/2;
		if(right > maxY) maxY = right;

		edge e;
		forall_adj_edges(e,v) {
			node w = e->target();
			if(w != v) S.push(w);
		}
	}
}


//node TreeLayout::initializeTreeStructure(const Graph &tree)
void TreeLayout::initializeTreeStructure(const Graph &tree, List<node> &roots)
{
	node v;

	// initialize node arrays
	m_number     .init(tree,0);
	m_parent     .init(tree,0);
	m_leftSibling.init(tree,0);
	m_firstChild .init(tree,0);
	m_lastChild  .init(tree,0);
	m_thread     .init(tree,0);
	m_ancestor   .init(tree,0);
	m_preliminary.init(tree,0);
	m_modifier   .init(tree,0);
	m_change     .init(tree,0);
	m_shift      .init(tree,0);

	// compute the tree structure

	// find the roots
	//node root = 0;
	forall_nodes(v,tree) {
		if(v->indeg() == 0)
			roots.pushBack(v);
	}

	int childCounter;
	forall_nodes(v,tree) {

		// determine
		// - the parent node of v
		// - the leftmost and rightmost child of v
		// - the numbers of the children of v
		// - the left siblings of the children of v
		// and initialize the actual ancestor of v

		m_ancestor[v] = v;
		if(isLeaf(v)) {
			if(v->indeg() == 0) { // is v a root
				m_parent[v] = 0;
				m_leftSibling[v] = 0;
			}
			else {
				m_firstChild[v] = m_lastChild[v] = 0;
				m_parent[v] = v->firstAdj()->theEdge()->source();
			}
		}
		else {

			// traverse the adjacency list of v
			adjEntry first;    // first leaving edge
			adjEntry stop;     // successor of last leaving edge
			first = v->firstAdj();
			if(v->indeg() == 0) { // is v a root
				stop = first;
				m_parent[v] = 0;
				m_leftSibling[v] = 0;
			}
			else {

				// search for first leaving edge
				while(first->theEdge()->source() == v)
					first = first->cyclicSucc();
				m_parent[v] = first->theEdge()->source();
				stop = first;
				first = first->cyclicSucc();
			}

			// traverse the children of v
			m_firstChild[v] = first->theEdge()->target();
			m_number[m_firstChild[v]] = childCounter = 0;
			m_leftSibling[m_firstChild[v]] = 0;
			adjEntry previous = first;
			while(first->cyclicSucc() != stop) {
				first = first->cyclicSucc();
				m_number[first->theEdge()->target()] = ++childCounter;
				m_leftSibling[first->theEdge()->target()]
					= previous->theEdge()->target();
				previous = first;
			}
			m_lastChild[v] = first->theEdge()->target();
		}
	}
}


void TreeLayout::deleteTreeStructure()
{
	m_number     .init();
	m_parent     .init();
	m_leftSibling.init();
	m_firstChild .init();
	m_lastChild  .init();
	m_thread     .init();
	m_ancestor   .init();
	m_preliminary.init();
	m_modifier   .init();
	m_change     .init();
	m_shift      .init();
}


int TreeLayout::isLeaf(node v) const
{
	OGDF_ASSERT(v != 0);

	// node v is a leaf if and only if no edge leaves v
	return v->outdeg() == 0;
}


node TreeLayout::nextOnLeftContour(node v) const
{
	OGDF_ASSERT(v != 0);
	OGDF_ASSERT(v->graphOf() == m_firstChild.graphOf());
	OGDF_ASSERT(v->graphOf() == m_thread.graphOf());

	// if v has children, the successor of v on the left contour
	// is its leftmost child,
	// otherwise, the successor is the thread of v (may be 0)
	if(m_firstChild[v] != 0)
		return m_firstChild[v];
	else
		return m_thread[v];
}


node TreeLayout::nextOnRightContour(node v) const
{
	OGDF_ASSERT(v != 0);
	OGDF_ASSERT(v->graphOf() == m_lastChild.graphOf());
	OGDF_ASSERT(v->graphOf() == m_thread.graphOf());

	// if v has children, the successor of v on the right contour
	// is its rightmost child,
	// otherwise, the successor is the thread of v (may be 0)
	if(m_lastChild[v] != 0)
		return m_lastChild[v];
	else
		return m_thread[v];
}


void TreeLayout::firstWalk(node subtree,const GraphAttributes &AG,bool upDown)
{
	OGDF_ASSERT(subtree != 0);
	OGDF_ASSERT(subtree->graphOf() == m_leftSibling.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_preliminary.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_firstChild.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_lastChild.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_modifier.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_change.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_shift.graphOf());

	// compute a preliminary x-coordinate for subtree
	if(isLeaf(subtree)) {

		// place subtree close to the left sibling
		node leftSibling = m_leftSibling[subtree];
		if(leftSibling != 0) {
			if(upDown) {
				m_preliminary[subtree] = m_preliminary[leftSibling]
					+ (AG.width(subtree) + AG.width(leftSibling)) / 2
					+ m_siblingDistance;
			} else {
				m_preliminary[subtree] = m_preliminary[leftSibling]
					+ (AG.height(subtree) + AG.height(leftSibling)) / 2
					+ m_siblingDistance;
			}
		}
		else m_preliminary[subtree] = 0;
	}
	else {
		node defaultAncestor = m_firstChild[subtree];

		// collect the children of subtree
		List<node> children;
		node v = m_lastChild[subtree];
		do {
			children.pushFront(v);
			v = m_leftSibling[v];
		} while(v != 0);

		ListIterator<node> it;

		// apply firstwalk and apportion to the children
		for(it = children.begin(); it.valid(); it = it.succ()) {
			firstWalk(*it,AG,upDown);
			apportion(*it,defaultAncestor,AG,upDown);
		}

		// shift the small subtrees
		double shift = 0;
		double change = 0;
		children.reverse();
		for(it = children.begin(); it.valid(); it = it.succ()) {
			m_preliminary[*it] += shift;
			m_modifier[*it] += shift;
			change += m_change[*it];
			shift += m_shift[*it] + change;
		}

		// place the parent node
		double midpoint = (m_preliminary[children.front()] + m_preliminary[children.back()]) / 2;
		node leftSibling = m_leftSibling[subtree];
		if(leftSibling != 0) {
			if(upDown) {
				m_preliminary[subtree] = m_preliminary[leftSibling]
					+ (AG.width(subtree) + AG.width(leftSibling)) / 2
					+ m_siblingDistance;
			} else {
				m_preliminary[subtree] = m_preliminary[leftSibling]
					+ (AG.height(subtree) + AG.height(leftSibling)) / 2
					+ m_siblingDistance;
			}
			m_modifier[subtree] =
				m_preliminary[subtree] - midpoint;
		}
		else m_preliminary[subtree] = midpoint;
	}
}

void TreeLayout::apportion(
	node subtree,
	node &defaultAncestor,
	const GraphAttributes &AG,
	bool upDown)
{
	OGDF_ASSERT(subtree != 0);
	OGDF_ASSERT(subtree->graphOf() == defaultAncestor->graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_leftSibling.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_firstChild.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_modifier.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_ancestor.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_change.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_shift.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_thread.graphOf());

	if(m_leftSibling[subtree] == 0) return;

	// check distance to the left of the subtree
	// and traverse left/right inside/outside contour

	double leftModSumOut = 0;  // sum of modifiers on left outside contour
	double leftModSumIn = 0;   // sum of modifiers on left inside contour
	double rightModSumIn = 0;  // sum of modifiers on right inside contour
	double rightModSumOut = 0; // sum of modifiers on right outside contour

	double moveDistance;
	int numberOfSubtrees;
	node leftAncestor,rightAncestor;

	// start the traversal at the actual level
	node leftContourOut  = m_firstChild[m_parent[subtree]];
	node leftContourIn   = m_leftSibling[subtree];
	node rightContourIn  = subtree;
	node rightContourOut = subtree;
	bool stop = false;
	do {

		// add modifiers
		leftModSumOut  += m_modifier[leftContourOut];
		leftModSumIn   += m_modifier[leftContourIn];
		rightModSumIn  += m_modifier[rightContourIn];
		rightModSumOut += m_modifier[rightContourOut];

		// actualize ancestor for right contour
		m_ancestor[rightContourOut] = subtree;

		if(nextOnLeftContour(leftContourOut) != 0 && nextOnRightContour(rightContourOut) != 0)
		{
			// continue traversal
			leftContourOut  = nextOnLeftContour(leftContourOut);
			leftContourIn   = nextOnRightContour(leftContourIn);
			rightContourIn  = nextOnLeftContour(rightContourIn);
			rightContourOut = nextOnRightContour(rightContourOut);

			// check if subtree has to be moved
			if(upDown) {
				moveDistance = m_preliminary[leftContourIn] + leftModSumIn
					+ (AG.width(leftContourIn) + AG.width(rightContourIn)) / 2
					+ m_subtreeDistance
					- m_preliminary[rightContourIn] - rightModSumIn;
			} else {
				moveDistance = m_preliminary[leftContourIn] + leftModSumIn
					+ (AG.height(leftContourIn) + AG.height(rightContourIn)) / 2
					+ m_subtreeDistance
					- m_preliminary[rightContourIn] - rightModSumIn;
			}
			if(moveDistance > 0) {

				// compute highest different ancestors of leftContourIn
				// and rightContourIn
				if(m_parent[m_ancestor[leftContourIn]] == m_parent[subtree])
					leftAncestor = m_ancestor[leftContourIn];
				else leftAncestor = defaultAncestor;
				rightAncestor = subtree;

				// compute the number of small subtrees in between (plus 1)
				numberOfSubtrees =
					m_number[rightAncestor] - m_number[leftAncestor];

				// compute the shifts and changes of shift
				m_change[rightAncestor] -= moveDistance / numberOfSubtrees;
				m_shift[rightAncestor] += moveDistance;
				m_change[leftAncestor] += moveDistance / numberOfSubtrees;

				// move subtree to the right by moveDistance
				m_preliminary[rightAncestor] += moveDistance;
				m_modifier[rightAncestor] += moveDistance;
				rightModSumIn += moveDistance;
				rightModSumOut += moveDistance;
			}
		}
		else stop = true;
	} while(!stop);

	// adjust threads
	if(nextOnRightContour(rightContourOut) == 0 && nextOnRightContour(leftContourIn) != 0)
	{
		// right subtree smaller than left subforest
		m_thread[rightContourOut] = nextOnRightContour(leftContourIn);
		m_modifier[rightContourOut] += leftModSumIn - rightModSumOut;
	}

	if(nextOnLeftContour(leftContourOut) == 0 && nextOnLeftContour(rightContourIn) != 0)
	{
		// left subforest smaller than right subtree
		m_thread[leftContourOut] = nextOnLeftContour(rightContourIn);
		m_modifier[leftContourOut] += rightModSumIn - leftModSumOut;
		defaultAncestor = subtree;
	}
}


void TreeLayout::secondWalkX(node subtree,
							double modifierSum,
							GraphAttributes &AG)
{
	OGDF_ASSERT(subtree != 0);
	OGDF_ASSERT(subtree->graphOf() == m_preliminary.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_modifier.graphOf());

	// compute final x-coordinates for the subtree
	// by recursively aggregating modifiers
	AG.x(subtree) = m_preliminary[subtree] + modifierSum;
	modifierSum += m_modifier[subtree];
	edge e;
	forall_adj_edges(e,subtree) if(e->target() != subtree)
		secondWalkX(e->target(),modifierSum,AG);
}

void TreeLayout::secondWalkY(node subtree,
							double modifierSum,
							GraphAttributes &AG)
{
	OGDF_ASSERT(subtree != 0);
	OGDF_ASSERT(subtree->graphOf() == m_preliminary.graphOf());
	OGDF_ASSERT(subtree->graphOf() == m_modifier.graphOf());

	// compute final y-coordinates for the subtree
	// by recursively aggregating modifiers
	AG.y(subtree) = m_preliminary[subtree] + modifierSum;
	modifierSum += m_modifier[subtree];
	edge e;
	forall_adj_edges(e,subtree) if(e->target() != subtree)
		secondWalkY(e->target(),modifierSum,AG);
}


void TreeLayout::computeYCoordinatesAndEdgeShapes(node root, GraphAttributes &AG)
{
	OGDF_ASSERT(root != 0);

	// compute y-coordinates and edge shapes
	node v,w;
	edge e;
	List<node> oldLevel;   // the nodes of the old level
	List<node> newLevel;   // the nodes of the new level
	ListIterator<node> it;
	double yCoordinate;    // the y-coordinate for the new level
	double edgeCoordinate; // the y-coordinate for edge bends
	double oldHeight;      // the maximal node height on the old level
	double newHeight;      // the maximal node height on the new level

	// traverse the tree level by level
	newLevel.pushBack(root);
	AG.y(root) = yCoordinate = 0;
	newHeight = AG.height(root);
	while(!newLevel.empty()) {
		oldHeight = newHeight;
		newHeight = 0;
		oldLevel.conc(newLevel);
		while(!oldLevel.empty()) {
			v = oldLevel.popFrontRet();
			forall_adj_edges(e,v) if(e->target() != v) {
				w = e->target();
				newLevel.pushBack(w);

				// compute the shape of edge e
				DPolyline &edgeBends = AG.bends(e);
				edgeBends.clear();
				if(m_orthogonalLayout) {
					edgeCoordinate =
						yCoordinate + (oldHeight + m_levelDistance) / 2;
					edgeBends.pushBack(DPoint(AG.x(v),edgeCoordinate));
					edgeBends.pushBack(DPoint(AG.x(w),edgeCoordinate));
				}

				// compute the maximal node height on the new level
				if(AG.height(e->target()) > newHeight)
					newHeight = AG.height(e->target());
			}
		}

		// assign y-coordinate to the nodes of the new level
		yCoordinate += (oldHeight + newHeight) / 2 + m_levelDistance;
		for(it = newLevel.begin(); it.valid(); it = it.succ())
			AG.y(*it) = yCoordinate;
	}
}

void TreeLayout::computeXCoordinatesAndEdgeShapes(node root, GraphAttributes &AG)
{
	OGDF_ASSERT(root != 0);

	// compute y-coordinates and edge shapes
	node v,w;
	edge e;
	List<node> oldLevel;   // the nodes of the old level
	List<node> newLevel;   // the nodes of the new level
	ListIterator<node> it;
	double xCoordinate;    // the x-coordinate for the new level
	double edgeCoordinate; // the x-coordinate for edge bends
	double oldWidth;       // the maximal node width on the old level
	double newWidth;       // the maximal node width on the new level

	// traverse the tree level by level
	newLevel.pushBack(root);
	AG.x(root) = xCoordinate = 0;
	newWidth = AG.width(root);
	while(!newLevel.empty()) {
		oldWidth = newWidth;
		newWidth = 0;
		oldLevel.conc(newLevel);
		while(!oldLevel.empty()) {
			v = oldLevel.popFrontRet();
			forall_adj_edges(e,v) if(e->target() != v) {
				w = e->target();
				newLevel.pushBack(w);

				// compute the shape of edge e
				DPolyline &edgeBends = AG.bends(e);
				edgeBends.clear();
				if(m_orthogonalLayout) {
					edgeCoordinate =
						xCoordinate + (oldWidth + m_levelDistance) / 2;
					edgeBends.pushBack(DPoint(edgeCoordinate,AG.y(v)));
					edgeBends.pushBack(DPoint(edgeCoordinate,AG.y(w)));
				}

				// compute the maximal node width on the new level
				if(AG.width(e->target()) > newWidth)
					newWidth = AG.width(e->target());
			}
		}

		// assign x-coordinate to the nodes of the new level
		xCoordinate += (oldWidth + newWidth) / 2 + m_levelDistance;
		for(it = newLevel.begin(); it.valid(); it = it.succ())
			AG.x(*it) = xCoordinate;
	}
}


} // end namespace ogdf
