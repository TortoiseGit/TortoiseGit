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
#include "CollapseTreeUpward.h"
#include "VisibleGraphNode.h"
#include "GraphNodeState.h"

// construction

CCollapseTreeUpward::CCollapseTreeUpward 
    ( CRevisionGraphOptionList& list
    , const CGraphNodeStates* nodeStates)
    : inherited (list)
    , nodeStates (nodeStates)
{
}

// implement IModificationOption

void CCollapseTreeUpward::Apply (CVisibleGraph* /* graph */, CVisibleGraphNode* node)
{
    DWORD state = nodeStates->GetFlags (node, true);

    // cut at this tree node, if requested by the node state

    if (state & CGraphNodeStates::COLLAPSED_ABOVE)
        newRoots.push_back (node);
}

void CCollapseTreeUpward::PostFilter (CVisibleGraph* graph)
{
    for (size_t i = 0, count = newRoots.size(); i < count; ++i)
        newRoots[i]->MakeRoot (graph, true);

    newRoots.clear();
}
