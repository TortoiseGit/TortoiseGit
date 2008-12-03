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
#pragma once

#include "DictionaryBasedTempPath.h"

using namespace LogCache;

class CFullGraphNode;

/**
 * \ingroup TortoiseProc
 * Helper class for the revision graph to search the whole path tree
 */
class CSearchPathTree
{
private:

	CDictionaryBasedTempPath path;

	/// when this entry becomes valid / active

	revision_t startRevision;

	/// previous node for this path (or its copy source)

	CFullGraphNode* lastEntry;

	/// tree pointers

	CSearchPathTree* parent;
	CSearchPathTree* firstChild;
	CSearchPathTree* lastChild;
	CSearchPathTree* previous;
	CSearchPathTree* next;

	/// utilities: handle node insertion / removal

	void DeLink();
	void Link (CSearchPathTree* newParent);

public:

	/// construction / destruction

	CSearchPathTree (const CPathDictionary* dictionary);
	CSearchPathTree ( const CDictionaryBasedTempPath& path
					, revision_t startrev
					, CSearchPathTree* parent);

	~CSearchPathTree();

	/// add a node for the given path and rev. to the tree

	CSearchPathTree* Insert ( const CDictionaryBasedTempPath& path
							, revision_t startrev);
	void Remove();

	/// there is a new revision entry for this path

	void ChainEntries (CFullGraphNode* entry);

	/// property access

	const CDictionaryBasedTempPath& GetPath() const
	{
		return path;
	}

	revision_t GetStartRevision() const
	{
		return startRevision;
	}

	void SetStartRevision (revision_t revision)
	{
		startRevision = revision;
	}

	CFullGraphNode* GetLastEntry() const
	{
		return lastEntry;
	}

	CSearchPathTree* GetParent() const
	{
		return parent;
	}

	CSearchPathTree* GetFirstChild() const
	{
		return firstChild;
	}

	CSearchPathTree* GetLastChild() const
	{
		return lastChild;
	}

	CSearchPathTree* GetNext() const
	{
		return next;
	}

	CSearchPathTree* GetPrevious() const
	{
		return previous;
	}

	bool IsActive() const
	{
		return startRevision != NO_REVISION;
	}

	bool IsEmpty() const
	{
		return !IsActive() && (firstChild == NULL);
	}

    /// return true for active paths that don't have a revEntry for this revision

    bool YetToCover (revision_t revision) const;

    /// return next node in pre-order

    CSearchPathTree* GetPreOrderNext (CSearchPathTree* lastNode = NULL);

    /// return next node in pre-order but skip this sub-tree

    CSearchPathTree* GetSkipSubTreeNext (CSearchPathTree* lastNode = NULL);

	/// find sub-tree of pathID  
	/// (return closet match if there is no such node)

	CSearchPathTree* FindCommonParent (index_t pathID);
};
