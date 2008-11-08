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

#include "RevisionGraphOptions.h"

/**
* Extends the base interface with a method that has full
* modifying access to a given visible graph.
*/

class IModificationOption : public IOrderedTraversalOption
{
public:

    virtual void Apply (CVisibleGraph* graph, CVisibleGraphNode* node) = 0;
};

/**
* Filtered sub-set of \ref CAllRevisionGraphOptions.
* It applies all options in the order defined by their \ref priority.
* The option instances may transform the graph any way they consider fit.
*
* Contains only \ref IModificationOption instances.
*/

class CModificationOptions : public CRevisionGraphOptionList
{
private:

    std::vector<IModificationOption*> options;

    /// apply a filter using differnt traversal orders

    void TraverseFromRootCopiesFirst ( IModificationOption* option
                                     , CVisibleGraph* graph
                                     , CVisibleGraphNode* node);
    void TraverseToRootCopiesFirst ( IModificationOption* option
                                   , CVisibleGraph* graph
                                   , CVisibleGraphNode* node);
    void TraverseFromRootCopiesLast ( IModificationOption* option
                                    , CVisibleGraph* graph
                                    , CVisibleGraphNode* node);
    void TraverseToRootCopiesLast ( IModificationOption* option
                                  , CVisibleGraph* graph
                                  , CVisibleGraphNode* node);

public:

    /// construction / destruction

    CModificationOptions (const std::vector<IModificationOption*>& options);
    virtual ~CModificationOptions() {}

    /// apply all filters 

    void Apply (CVisibleGraph* graph);
};

