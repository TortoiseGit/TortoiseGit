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

// that the line from node downward have any visible copy targets?

bool CFoldTags::CopyTargetsVisibile (const CVisibleGraphNode* node) const
{
    for (; node != NULL; node = node->GetNext())
        if (node->GetFirstCopyTarget() != NULL)
            return true;

    return false;
}

// construction

CFoldTags::CFoldTags (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IModificationOption

void CFoldTags::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // We will not remove tags that get copied to non-tags
    // that have not yet been removed from the graph.

    DWORD forbidden = CNodeClassification::COPIES_TO_TRUNK 
                    | CNodeClassification::COPIES_TO_BRANCH 
                    | CNodeClassification::COPIES_TO_OTHER;

    CNodeClassification classification = node->GetClassification();
    bool isTag = classification.Matches (CNodeClassification::IS_TAG, 0);
    bool isFinalTag = classification.Matches (CNodeClassification::IS_TAG, forbidden);

    // fold tags at the point of their creation

    if (isTag)
    {
        // this is a node on some tag but maybe not the creation point
        // (e.g. if we don't want to fold a tag and that tag gets
        // modified in several revisions)

        if (   (node->GetCopySource() != NULL)
            || (   (node->GetPrevious() != NULL)
                && classification.Is (CNodeClassification::IS_RENAMED)))
        {
            // this is the point where the tag got created / renamed
            // (CopyTargetsVisibile can be expensive -> don't do it
            // in the first "if" condition 4 lines above)

            if (isFinalTag || !CopyTargetsVisibile (node))
            {
                // it does not copy to branch etc.
                // (if it copies to a tag, that one will be folded first
                // and we will fold this one only in the next iteration)

                node->FoldTag (graph);
            }
        }
    }
}
