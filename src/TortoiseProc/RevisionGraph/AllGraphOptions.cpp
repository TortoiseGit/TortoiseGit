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
#include "AllGraphOptions.h"
#include "Resource.h"

#include "RemoveSimpleChanges.h"
#include "RemoveDeletedBranches.h"
#include "FoldTags.h"
#include "RemoveUnchangedBranches.h"
#include "ShowHeads.h"
#include "ShowWC.h"
#include "ShowWCModification.h"
#include "ExactCopyFroms.h"
#include "RevisionInRange.h"
#include "RemovePathsBySubString.h"

#include "CutTrees.h"
#include "CollapseTreeDownward.h"
#include "CollapseTreeUpward.h"

#include "StandardNodeSizeAssignment.h"
#include "StandardNodePositioning.h"
#include "StrictOrderNodePositioning.h"
#include "TopAlignTrees.h"
#include "UpsideDownLayout.h"
#include "ShowPathsAsDiff.h"
#include "ShowTreeStripes.h"

// construction (create all option objects) / destruction

CAllRevisionGraphOptions::CAllRevisionGraphOptions (const CGraphNodeStates* nodeStates)
{
    // create filter options.

    // The order is critical as it determines the option bit position
    // in the registry DWORD.

    CStandardNodePositioning* standardNodePositioning 
        = new CStandardNodePositioning (*this);
    new CRemoveSimpleChanges (*this);
    new CUpsideDownLayout (*this);
    new CShowHead (*this);
    IRevisionGraphOption* reduceCrossLines
        = new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, ID_VIEW_REDUCECROSSLINES> (*this);
    new CExactCopyFroms (*this);
    new CRevisionGraphOptionImpl<IRevisionGraphOption, 0, 0> (*this);   // 0x40 is not used
    new CFoldTags (*this);
    new CRemoveDeletedBranches (*this);
    new CShowWC (*this);
    new CRemoveUnchangedBranches (*this);
    new CShowWCModification (*this);
    new CShowPathsAsDiff (*this);
    new CShowTreeStripes (*this);
    new CTopAlignTrees (*this);

    (new CRevisionInRange (*this))->ToggleSelection();
    (new CRemovePathsBySubString (*this))->ToggleSelection();

    // tree node collapsing & cutting

    (new CCutTrees (*this, nodeStates))->ToggleSelection();
    (new CCollapseTreeDownward (*this, nodeStates))->ToggleSelection();
    (new CCollapseTreeUpward (*this, nodeStates))->ToggleSelection();

    // create layout options

    (new CStandardNodeSizeAssignment (*this, nodeStates))->ToggleSelection();
    new CStrictOrderNodePositioning (*this, standardNodePositioning, reduceCrossLines);

    // link options as necessary

    standardNodePositioning->SetReduceCrossLines (reduceCrossLines);
}

// access specific sub-sets

CCopyFilterOptions CAllRevisionGraphOptions::GetCopyFilterOptions() const
{
    return GetFilteredList<CCopyFilterOptions, ICopyFilterOption>();
}

CModificationOptions CAllRevisionGraphOptions::GetModificationOptions() const
{
    return GetFilteredList<CModificationOptions, IModificationOption>();
}

CLayoutOptions CAllRevisionGraphOptions::GetLayoutOptions() const
{
    return GetFilteredList<CLayoutOptions, ILayoutOption>();
}
