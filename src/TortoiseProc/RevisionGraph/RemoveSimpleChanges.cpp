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
#include "RemoveSimpleChanges.h"
#include "FullGraphNode.h"
#include "VisibleGraphNode.h"

// construction

CRemoveSimpleChanges::CRemoveSimpleChanges (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IRevisionGraphOption: This option is negated.

bool CRemoveSimpleChanges::IsActive() const
{
    return !IsSelected();
}

// implement ICopyFilterOption (pre-filter most nodes)

ICopyFilterOption::EResult 
CRemoveSimpleChanges::ShallRemove (const CFullGraphNode* node) const
{
    // "M", not a branch point and will not become one due to removed copy-froms

    bool nodeIsModificationOnly
        =    (node->GetClassification().Is (CNodeClassification::IS_MODIFIED))
          && (node->GetFirstCopyTarget() == NULL);

    const CFullGraphNode* next = node->GetNext();
    bool nextIsModificationOnly
        =    (next != NULL)
          && (next->GetClassification().Is (CNodeClassification::IS_MODIFIED))
          && (next->GetFirstCopyTarget() == NULL);

    return nodeIsModificationOnly && nextIsModificationOnly
        ? ICopyFilterOption::REMOVE_NODE
        : ICopyFilterOption::KEEP_NODE;
}

// implement IModificationOption (post-filter unused copy-from nodes)

void CRemoveSimpleChanges::Apply (CVisibleGraph* graph, CVisibleGraphNode* node)
{
    // "M", not a branch point, not the HEAD

    if (   (node->GetClassification().Matches 
                ( CNodeClassification::IS_MODIFIED
                , static_cast<DWORD>(CNodeClassification::MUST_BE_PRESERVED)))
        && (node->GetFirstTag() == NULL)
        && (node->GetFirstCopyTarget() == NULL))
    {
        node->DropNode (graph);
    }
}
