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
#include "RemoveDeletedBranches.h"
#include "FullGraphNode.h"
#include "VisibleGraphNode.h"

// construction

CRemoveDeletedBranches::CRemoveDeletedBranches (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CRemoveDeletedBranches::ShallRemove (const CFullGraphNode* node) const
{
    // "M", not a branch point, not the HEAD

    return node->GetClassification().Is (CNodeClassification::ALL_COPIES_DELETED)
         ? ICopyFilterOption::REMOVE_SUBTREE
         : ICopyFilterOption::KEEP_NODE;
}

/// implement IModificationOption (post-filter deleted non-tagged branches)

void CRemoveDeletedBranches::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    if (   (node->GetNext() == NULL) 
        && (node->GetFirstCopyTarget() == NULL)
        && (node->GetFirstTag() == NULL)
        && (node->GetClassification().Is (CNodeClassification::PATH_ONLY_DELETED)))
    {
        node->DropNode (graph, true);
    }
}
