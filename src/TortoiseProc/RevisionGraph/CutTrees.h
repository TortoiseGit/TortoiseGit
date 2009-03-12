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

#include "ModificationOptions.h"
#include "Resource.h"

// forward declarations

class CGraphNodeStates;

/**
* Make nodes a root node if the respective node state says 
* that the node was cut from the graph.
*/

class CCutTrees 
    : public CModificationOptionImpl< IModificationOption
                                    , 2000
                                    , 0
                                    , false     // branches last
                                    , true      // from roots to leaves
                                    , false>    // this is not a cyclic option
{
private:

    /// node states that decide where to cut

    const CGraphNodeStates* nodeStates;

public:

    /// construction

    CCutTrees ( CRevisionGraphOptionList& list
              , const CGraphNodeStates* nodeStates);

    /// implement IModificationOption

    virtual void Apply (CVisibleGraph* graph, CVisibleGraphNode* node);
};
