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
#include "RevisionGraphOptionsImpl.h"

/**
* Extends the base interface with a method that has full
* modifying access to a given visible graph.
*/

class IModificationOption : public IOrderedTraversalOption
{
public:

    /// If true, the option shall be applied with all other
    /// clyclic options more than once until the graph is stable.

    virtual bool IsCyclic() const = 0;

    /// Apply / execute the filter.

    virtual void Apply (CVisibleGraph* graph, CVisibleGraphNode* node) = 0;

    /// will be called after each tree traversal.
    /// Use this to modify the tree is a way that interfers
    /// with the standard traveral, for instance.

    virtual void PostFilter (CVisibleGraph* graph) = 0;
};

/**
 * Standard implementation of IModificationOption.
 */

template<class Base, int Prio, UINT ID, bool CopyiesFirst, bool RootFirst, bool Cyclic>
class CModificationOptionImpl 
    : public COrderedTraversalOptionImpl<Base, Prio, ID, CopyiesFirst, RootFirst>
{
protected:

    /// for simplied construction by the _derived_ class

    typedef typename CModificationOptionImpl< Base
                                            , Prio
                                            , ID
                                            , CopyiesFirst
                                            , RootFirst
                                            , Cyclic> inherited;

public:

    /// construction / destruction

    CModificationOptionImpl (CRevisionGraphOptionList& list)
        : COrderedTraversalOptionImpl<Base, Prio, ID, CopyiesFirst, RootFirst>(list)
    {
    }

    /// implement IModificationOption

    virtual bool IsCyclic() const {return Cyclic;}
    virtual void PostFilter (CVisibleGraph*) {};
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
    void InternalApply (CVisibleGraph* graph, bool cyclicFilters);

public:

    /// construction / destruction

    CModificationOptions (const std::vector<IModificationOption*>& options);
    virtual ~CModificationOptions() {}

    /// apply all filters 

    void Apply (CVisibleGraph* graph);
};

