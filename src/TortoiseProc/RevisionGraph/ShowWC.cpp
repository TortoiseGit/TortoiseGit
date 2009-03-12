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
#include "ShowWC.h"
#include "FullGraphNode.h"
#include "VisibleGraphNode.h"

// construction

CShowWC::CShowWC (CRevisionGraphOptionList& list)
    : inherited (list)
{
}

// implement IRevisionGraphOption: 
// this one must always be executed
// (ICopyFilterOption, if selected; IModificationOption is not)

bool CShowWC::IsActive() const
{
    return true;
}

// implement ICopyFilterOption

ICopyFilterOption::EResult 
CShowWC::ShallRemove (const CFullGraphNode* node) const
{
    // "Pin" HEAD nodes

    return (   node->GetClassification().Is (CNodeClassification::IS_WORKINGCOPY)
            && IsSelected())
         ? ICopyFilterOption::PRESERVE_NODE
         : ICopyFilterOption::KEEP_NODE;
}

// implement IModificationOption (post-filter deleted non-tagged branches)

void CShowWC::Apply (CVisibleGraph*, CVisibleGraphNode* node)
{
    if (!IsSelected())
    {
        CNodeClassification classification = node->GetClassification();
        if (classification.Matches ( CNodeClassification::IS_WORKINGCOPY
                                   , CNodeClassification::IS_MODIFIED_WC))
        {
            classification.Remove (CNodeClassification::IS_WORKINGCOPY);
            node->SetClassification (classification);
        }
    }
}
