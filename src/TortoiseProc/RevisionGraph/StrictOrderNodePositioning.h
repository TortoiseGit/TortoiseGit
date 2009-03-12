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

#include "LayoutOptions.h"
#include "RevisionGraphOptionsImpl.h"
#include "./Containers/LogCacheGlobals.h"

class IRevisionGraphLayout;
class IStandardLayoutNodeAccess;
class CStandardLayoutNodeInfo;

using namespace LogCache;

class CStrictOrderNodePositioning 
    : public CRevisionGraphOptionImpl<ILayoutOption, 200, 0>
{
private:

    // active, if this is one not active

    IRevisionGraphOption* standardNodePositioning;

    // additional options that modify the behavior of this one

    IRevisionGraphOption* reduceCrossLines;

    /// the individual placement stages

    void SortRevisions 
        ( IStandardLayoutNodeAccess* nodeAccess
        , std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes);
    void AssignColumns 
        ( CStandardLayoutNodeInfo* start
        , size_t column
        , std::vector<revision_t>& startRevisions
        , std::vector<revision_t>& endRevisions
        , std::vector<int>& maxWidths);
    void AssignColumns 
        ( std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes
        , std::vector<int>& maxWidths);
    void AssignRows
        (std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes);
    void ShiftNodes 
        ( std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes
        , std::vector<int>& columWidths);
    
public:

    /// construction

    CStrictOrderNodePositioning 
        ( CRevisionGraphOptionList& list
        , IRevisionGraphOption* standardNodePositioning
        , IRevisionGraphOption* reduceCrossLines);

    /// implement IRevisionGraphOption: Active if standard layout is disabled.

    virtual bool IsActive() const; 

    /// cast @a layout pointer to the respective modification
    /// interface and write the data.

    virtual void ApplyTo (IRevisionGraphLayout* layout);
};
