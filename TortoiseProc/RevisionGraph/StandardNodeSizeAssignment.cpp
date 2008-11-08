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
#include "StandardNodeSizeAssignment.h"
#include "StandardLayout.h"
#include "VisibleGraphNode.h"

// construction

CStandardNodeSizeAssignment::CStandardNodeSizeAssignment 
    ( CRevisionGraphOptionList& list)
    : CRevisionGraphOptionImpl<ILayoutOption, 100, 0> (list)
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

        node->requiresPath =   (node->previousInBranch == NULL)
                            || (   node->previousInBranch->node->GetPath() 
                                != node->node->GetPath());

        int hight = 28;
        if (node->requiresPath)
            hight += 3 + node->node->GetPath().GetDepth() * 21;

        node->requiredSize = CSize (200, hight);
        node->rect = CRect (0, 0, 200, hight);
    }
}
