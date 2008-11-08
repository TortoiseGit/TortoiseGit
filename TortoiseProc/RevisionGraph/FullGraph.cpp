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
#include "StdAfx.h"
#include "FullGraph.h"

// construction / destruction

CFullGraph::CFullGraph()
    : nodeFactory()
    , root (NULL)
    , nodeCount (0)
{
}

CFullGraph::~CFullGraph()
{
    if (root)
        nodeFactory.Destroy (root);
}

// modification

CFullGraphNode* CFullGraph::Add ( const CDictionaryBasedTempPath& path
                                , revision_t revision
                                , CNodeClassification classification
                                , CFullGraphNode* source)
{
    // (only) the first node must have no parent / prev node

    assert ((source == NULL) == (root == NULL));

    CFullGraphNode* result 
        = nodeFactory.Create (path, revision, classification, source);

    ++nodeCount;
    if (root == NULL)
        root = result;

    return result;
}

void CFullGraph::Replace ( CFullGraphNode* toReplace
                         , CFullGraphNode::CCopyTarget*& toMove
                         , CNodeClassification newClassification)
{
    // we support simple cases only

    assert (toReplace->GetPrevious() != NULL);
    assert (toReplace->GetNext() == NULL);
    assert (toReplace->GetFirstCopyTarget() == NULL);
    assert (toMove->value() != NULL);

    // replace

    nodeFactory.Replace (toReplace, toMove, newClassification);

    --nodeCount;
}


