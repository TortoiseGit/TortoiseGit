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

#include "FullGraphNode.h"

class CFullGraph
{
private:

    CFullGraphNode::CFactory nodeFactory;

    /// the graph is actually a tree

    CFullGraphNode* root;
    size_t nodeCount;

public:

    /// construction / destruction

    CFullGraph(void);
    ~CFullGraph(void);

    /// modification

    CFullGraphNode* Add ( const CDictionaryBasedTempPath& path
                        , revision_t revision
                        , CNodeClassification classification
                        , CFullGraphNode* source);

    void Replace ( CFullGraphNode* toReplace
                 , CFullGraphNode::CCopyTarget*& toMove
                 , CNodeClassification newClassification);

    /// member access

    CFullGraphNode* GetRoot();
    const CFullGraphNode* GetRoot() const;

    size_t GetNodeCount() const;
};

/// member access

inline const CFullGraphNode* CFullGraph::GetRoot() const
{
    return root;
}

inline CFullGraphNode* CFullGraph::GetRoot()
{
    return root;
}

inline size_t CFullGraph::GetNodeCount() const
{
    return nodeCount;
}
