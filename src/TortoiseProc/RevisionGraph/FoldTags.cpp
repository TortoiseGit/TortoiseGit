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
#include "FoldTags.h"
#include "VisibleGraphNode.h"

// construction

CFoldTags::CFoldTags (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IModificationOption

void CFoldTags::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // we won't fold this node, if there are modifications on this
    // or any of its sub-branches.
    // Also, we will not remove tags that get copied to non-tags.

    DWORD forbidden = CNodeClassification::COPIES_TO_TRUNK 
                    | CNodeClassification::COPIES_TO_BRANCH 
                    | CNodeClassification::COPIES_TO_OTHER;

    // fold tags at the point of their creation

    if (node->GetClassification().Matches (CNodeClassification::IS_TAG, forbidden))
        if (   (node->GetCopySource() != NULL)
            || node->GetClassification().Is (CNodeClassification::IS_RENAMED))
        {
            node->FoldTag (graph);
        }
}
