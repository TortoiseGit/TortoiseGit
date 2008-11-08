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
#include "ExactCopyFroms.h"
#include "VisibleGraphNode.h"

// construction

CExactCopyFroms::CExactCopyFroms (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IRevisionGraphOption: This option must always be applied.

bool CExactCopyFroms::IsActive() const
{
    return true;
}

// implement IModificationOption:
// remove the pure copy sources

void CExactCopyFroms::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // remove node, if it is neither "M", "A", "D" nor "R"

    if (node->GetClassification().Matches (0, CNodeClassification::IS_OPERATION_MASK))
    {
        // is this node still necessary?

        CVisibleGraphNode* next = node->GetNext();

        bool isCopySource =    (node->GetFirstTag() != NULL)
                            || (node->GetFirstCopyTarget() != NULL)
                            || (   (next != NULL) 
                                && (next->GetClassification().Is 
                                        (CNodeClassification::IS_RENAMED)));

        // remove it, if it is either no longer necessary or not wanted at all

        if (!IsSelected() || !isCopySource)
            node->DropNode (graph);
    }
}
