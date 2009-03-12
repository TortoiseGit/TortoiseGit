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
#include "RemoveUnchangedBranches.h"
#include "VisibleGraphNode.h"

// construction

CRemoveUnchangedBranches::CRemoveUnchangedBranches (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IModificationOption

void CRemoveUnchangedBranches::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // We will not remove tags and trunks as well as those branches
    // that have been modified.

    DWORD forbidden = CNodeClassification::IS_MODIFIED 
                    | CNodeClassification::PATH_ONLY_MODIFIED 
                    | CNodeClassification::IS_TRUNK 
                    | CNodeClassification::IS_TAG;

    // is this node part of a non-modified branch?

    if (node->GetClassification().Matches (0, forbidden))
    {
        // maybe, there as a modification before this node
        // and that modification has been hidden by other filters

        if (   (node->GetPrevious() == NULL)
            || !node->GetPrevious()->GetClassification().Is (CNodeClassification::PATH_ONLY_MODIFIED))
        {
            // remove it and preserve tags
            // (to preserve tags, we must keep root nodes as well)

            if (!node->IsRoot() || !node->GetFirstTag())
                node->DropNode (graph, true);
        }
    }
}
