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
#include "VisibleGraph.h"

// construction / destruction

CVisibleGraph::CVisibleGraph()
    : nodeFactory()
{
}

CVisibleGraph::~CVisibleGraph()
{
    Clear();
}

// modification

void CVisibleGraph::Clear()
{
    for (size_t i = roots.size(); i > 0; --i)
        nodeFactory.Destroy (roots[i-1]);

    assert (GetNodeCount() == 0);
    roots.clear();
}

CVisibleGraphNode* CVisibleGraph::Add ( const CFullGraphNode* base
                                      , CVisibleGraphNode* source
                                      , bool preserveNode)
{
    // (only) the first node must have no parent / prev node

    assert ((source == NULL) || !roots.empty());

    CVisibleGraphNode* result 
        = nodeFactory.Create (base, source, preserveNode);

    if (source == NULL)
        roots.push_back (result);

    return result;
}

void CVisibleGraph::ReplaceRoot ( CVisibleGraphNode* oldRoot
                                , CVisibleGraphNode* newRoot)
{
    assert (newRoot->GetPrevious() == NULL);
    assert (newRoot->GetCopySource() == NULL);

    for (size_t i = 0, count = roots.size(); i < count; ++i)
        if (roots[i] == oldRoot)
        {
            roots[i] = newRoot;
            return;
        }

    // we should never get here

    assert (0);
}

void CVisibleGraph::RemoveRoot (CVisibleGraphNode* root)
{
    for (size_t i = 0, count = roots.size(); i < count; ++i)
        if (roots[i] == root)
        {
            roots[i] = roots[count-1];
            roots.pop_back();

            return;
        }

    // we should never get here

    assert (0);
}

void CVisibleGraph::AddRoot (CVisibleGraphNode* root)
{
    assert (root->GetPrevious() == NULL);
    assert (root->GetCopySource() == NULL);

    roots.push_back (root);
}

