// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "StdAfx.h"
#include "SearchPathTree.h"
#include "FullGraphNode.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CSearchPathTree::DeLink()
{
	assert (parent);

	if (previous)
		previous->next = next;
	if (next)
		next->previous = previous;

	if (parent->firstChild == this)
		parent->firstChild = next;
	if (parent->lastChild == this)
		parent->lastChild = previous;

	parent = NULL;
}

void CSearchPathTree::Link (CSearchPathTree* newParent)
{
	assert (parent == NULL);
	assert (newParent != NULL);

	// ensure that for every cached path element hierarchy level
	// there is a node in the tree hierarchy

	index_t pathID = path.GetBasePath().GetIndex();
	index_t parentPathID = path.IsFullyCachedPath()
						 ? path.GetDictionary()->GetParent (pathID)
						 : pathID;

	if (newParent->path.GetBasePath().GetIndex() < parentPathID)
	{
		CDictionaryBasedPath parentPath ( path.GetDictionary()
										, parentPathID);
		newParent = new CSearchPathTree ( CDictionaryBasedTempPath (parentPath)
										, (revision_t)NO_REVISION
										, newParent);
	}

	parent = newParent;

	// insert into parent's child list ordered by pathID

	// O(1) insertion in most cases

	if (parent->firstChild == NULL)
	{
		// no child yet

		parent->firstChild = this;
		parent->lastChild = this;
	}
	else if (parent->lastChild->path.GetBasePath().GetIndex() <= pathID)
	{
		// insert at the end

		previous = parent->lastChild;

		parent->lastChild = this;
		previous->next = this;
	}
	else if (parent->firstChild->path.GetBasePath().GetIndex() >= pathID)
	{
		// insert at the beginning

		next = parent->firstChild;

		parent->firstChild = this;
		next->previous = this;
	}
	else
	{
		assert (parent->firstChild != parent->lastChild);

		// scan for insertion position

		CSearchPathTree* node = parent->firstChild;
		while (node->path.GetBasePath().GetIndex() < pathID)
			node = node->next;

		// insert us there

		previous = node->previous;
		next = node;

		previous->next = this;
		node->previous = this;
	}
}

// construction / destruction

CSearchPathTree::CSearchPathTree (const CPathDictionary* dictionary)
	: path (dictionary, std::string())
	, startRevision ((revision_t)NO_REVISION)
	, lastEntry (NULL)
	, parent (NULL)
	, firstChild (NULL)
	, lastChild (NULL)
	, previous (NULL)
	, next (NULL)
{
}

CSearchPathTree::CSearchPathTree ( const CDictionaryBasedTempPath& path
								 , revision_t startrev
								 , CSearchPathTree* parent)
	: path (path)
	, startRevision (startrev)
	, lastEntry (NULL)
	, parent (NULL)
	, firstChild (NULL)
	, lastChild (NULL)
	, previous (NULL)
	, next (NULL)
{
	Link (parent);
}

CSearchPathTree::~CSearchPathTree()
{
	while (firstChild != NULL)
		delete firstChild;

	if (parent)
		DeLink();
}

// add a node for the given path and rev. to the tree

CSearchPathTree* CSearchPathTree::Insert ( const CDictionaryBasedTempPath& path
										 , revision_t startrev)
{
	assert (startrev != NO_REVISION);

	// exact match (will happen on root node only)?

	if (this->path == path)
	{
		startRevision = startrev;
		return this;
	}

	// (partly or fully) overlap with an existing child?

	CDictionaryBasedPath cachedPath = path.GetBasePath();
	for (CSearchPathTree* child = firstChild; child != NULL; child = child->next)
	{
		CDictionaryBasedTempPath commonPath
			= child->path.GetCommonRoot (path);

		if (commonPath != this->path)
		{
			if (child->path == path)
			{
				// there is already a node for the exact same path
				// -> use it, if unused so far; append a new node otherwise

				if (child->startRevision == NO_REVISION)
					child->startRevision = startrev;
				else
					return new CSearchPathTree (path, startrev, this);
			}
			else
			{
				// the path is a (true) sub-node of the child

				return child->Insert (path, startrev);
			}
		}
	}

	// no overlap with any existing node
	// -> create a new child

	return new CSearchPathTree (path, startrev, this);
}

void CSearchPathTree::Remove()
{
	startRevision = revision_t (NO_REVISION);
	lastEntry = NULL;

	CSearchPathTree* node = this;
	while (node->IsEmpty() && (node->parent != NULL))
	{
		CSearchPathTree* temp = node;
		node = node->parent;

		delete temp;
	}
}

// there is a new revision entry for this path

void CSearchPathTree::ChainEntries (CFullGraphNode* entry)
{
	assert (entry != NULL);

	lastEntry = entry;
	startRevision = max (startRevision, entry->GetRevision());
}

// return true for active paths that don't have a revEntry for this revision

bool CSearchPathTree::YetToCover (revision_t revision) const
{
    return    IsActive() 
           && ((lastEntry == NULL) || (lastEntry->GetRevision() < revision));
}

// return next node in pre-order

CSearchPathTree* CSearchPathTree::GetPreOrderNext (CSearchPathTree* lastNode)
{
	if (firstChild != NULL)
        return firstChild;

    // there is no sub-tree

    return GetSkipSubTreeNext (lastNode);
}

// return next node in pre-order but skip this sub-tree

CSearchPathTree* CSearchPathTree::GetSkipSubTreeNext (CSearchPathTree* lastNode)
{
    CSearchPathTree* result = this;
    while ((result != lastNode) && (result->next == NULL))
		result = result->parent;

    return result == lastNode 
        ? result
        : result->next;
}

// find sub-tree of pathID  
// (return NULL if there is no such node)

CSearchPathTree* CSearchPathTree::FindCommonParent (index_t pathID)
{
	index_t nodePathID = path.GetBasePath().GetIndex();

	// collect all path elements to find *below* this one

	index_t pathToFind[MAX_PATH];
	size_t index = 0;

	while ((pathID != NO_INDEX) && (pathID > nodePathID))
	{
		pathToFind[index] = pathID;
		++index;
		assert (index < MAX_PATH);

		pathID = path.GetDictionary()->GetParent (pathID);
	}

	// start search at *this node

	CSearchPathTree* node = this;
	CSearchPathTree* oldNode = node;

	while (node != NULL)
	{
		// follow the desired path until it hits or passes the node

		while ((index != 0) && (nodePathID > pathID))
			pathID = pathToFind[--index];

		// scan sibling branches for a match, if this node wasn't one.
		// Since all are on the *same* level, either
		// * one must match
		// * nodePathID < pathID (i.e. node is a true child)
		// * path not found

		while ((nodePathID < pathID) && (node->next != NULL))
		{
			node = node->next;
			nodePathID = node->path.GetBasePath().GetIndex();
		}

		// end of search or not in this sub-tree?

		if ((index == 0) || (nodePathID != pathID))
			return node;

		// descend one level

		pathID = pathToFind[--index];
		
		oldNode = node;
		node = node->firstChild;
		if (node != NULL)
			nodePathID = node->path.GetBasePath().GetIndex();
	}

	// not found

	return oldNode;
}
