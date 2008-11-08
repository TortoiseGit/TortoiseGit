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
#pragma once

// include base classes

#include "CopyFilterOptions.h"
#include "ModificationOptions.h"
#include "revisiongraphoptionsimpl.h"
#include "Resource.h"

/** Remove all branches / tags that have been deleted and have
* no surviving copy.
*/

class CRemoveDeletedBranches 
    : public COrderedTraversalOptionImpl
                < CCombineInterface 
                    < ICopyFilterOption
                    , IModificationOption>
                , 250
                , ID_VIEW_REMOVEDELETEDONES
                , true          // crawl branches first
                , false>        // root last
{
public:

    /// construction

    CRemoveDeletedBranches (CRevisionGraphOptionList& list);

    /// implement ICopyFilterOption (pre-filter most nodes)

    virtual EResult ShallRemove (const CFullGraphNode* node) const;

    /// implement IModificationOption (post-filter deleted non-tagged branches)

    virtual void Apply (CVisibleGraph* graph, CVisibleGraphNode* node);
};
