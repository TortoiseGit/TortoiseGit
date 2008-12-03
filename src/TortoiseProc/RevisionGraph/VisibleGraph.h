// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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

// required includes

#include "VisibleGraphNode.h"

/**
* Contains a filtered copy of some \ref CFullGraph instance.
* 
* Acts as factory and container for all nodes and their sub-structres.
*/

class CVisibleGraph
{
private:

    CVisibleGraphNode::CFactory nodeFactory;

    /// the graph is actually a forest of trees

    std::vector<CVisibleGraphNode*> roots;

public:

    /// construction / destruction

    CVisibleGraph(void);
    ~CVisibleGraph(void);

    /// modification

    void Clear();
    CVisibleGraphNode* Add ( const CFullGraphNode* base
                           , CVisibleGraphNode* source
                           , bool preserveNode);

    void ReplaceRoot (CVisibleGraphNode* oldRoot, CVisibleGraphNode* newRoot);
    void RemoveRoot (CVisibleGraphNode* root);
    void AddRoot (CVisibleGraphNode* root);

    /// member access

    size_t GetRootCount() const;
    CVisibleGraphNode* GetRoot (size_t index);
    const CVisibleGraphNode* GetRoot (size_t index) const;

    size_t GetNodeCount() const;

    /// factory access

    CVisibleGraphNode::CFactory& GetFactory();
};

/// member access

inline size_t CVisibleGraph::GetRootCount() const
{
    return roots.size();
}

inline const CVisibleGraphNode* CVisibleGraph::GetRoot (size_t index) const
{
    return roots[index];
}

inline CVisibleGraphNode* CVisibleGraph::GetRoot (size_t index)
{
    return roots[index];
}

inline size_t CVisibleGraph::GetNodeCount() const
{
    return nodeFactory.GetNodeCount();
}

inline CVisibleGraphNode::CFactory& CVisibleGraph::GetFactory()
{
    return nodeFactory;
}
