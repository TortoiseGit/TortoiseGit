// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "StandardNodeSizeAssignment.h"
#include "StandardLayout.h"
#include "VisibleGraphNode.h"
#include "GraphNodeState.h"

// construction

CStandardNodeSizeAssignment::CStandardNodeSizeAssignment 
    ( CRevisionGraphOptionList& list
    , const CGraphNodeStates* nodeStates)
    : CRevisionGraphOptionImpl<ILayoutOption, 100, 0> (list)
    , nodeStates (nodeStates)
{
}

// cast @a layout pointer to the respective modification
// interface and write the data.

void CStandardNodeSizeAssignment::ApplyTo (IRevisionGraphLayout* layout)
{
    // we need access to actual data

    IStandardLayoutNodeAccess* nodeAccess 
        = dynamic_cast<IStandardLayoutNodeAccess*>(layout);
    if (nodeAccess == NULL) 
        return;

    // run

    for (index_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodeAccess->GetNode(i);

        // expand node to show the path, if necessary

        node->requiresPath =   (node->previousInBranch == NULL)
                            || (   node->previousInBranch->node->GetPath() 
                                != node->node->GetPath());

        int height = 28;
        if (node->requiresPath)
        {
            size_t visibleElementCount = node->node->GetPath().GetDepth() 
                                       - node->skipStartPathElements
                                       - node->skipTailPathElements;
            height += 3 + visibleElementCount * 21;
        }

        // shift (root) nodes down, if their source has been folded
        // (otherwise, glyphs would be partly hidden)

        DWORD state = nodeStates->GetFlags (node->node);
        int shift = (state & ( CGraphNodeStates::COLLAPSED_ABOVE 
                             | CGraphNodeStates::SPLIT_ABOVE)) == 0
                  ? 0
                  : 8;

        int extension = (state & ( CGraphNodeStates::COLLAPSED_BELOW 
                                 | CGraphNodeStates::SPLIT_BELOW)) == 0
                      ? 0
                      : 8;

        // set result

        node->requiredSize = CSize (200, height + extension + shift);
        node->rect = CRect (0, shift, 200, height + shift);
    }
}
