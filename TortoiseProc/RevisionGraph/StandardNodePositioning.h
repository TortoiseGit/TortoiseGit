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

#include "Resource.h"
#include "LayoutOptions.h"
#include "RevisionGraphOptionsImpl.h"

class IRevisionGraphLayout;
class CStandardLayoutNodeInfo;

class CStandardNodePositioning 
    : public CRevisionGraphOptionImpl<ILayoutOption, 200, ID_VIEW_GROUPBRANCHES>
{
private:

    // additional options that modify the behavior of this one

    IRevisionGraphOption* reduceCrossLines;

    /// the individual placement stages

    void StackSubTree 
        ( CStandardLayoutNodeInfo* node
        , std::vector<long>& branchColumnStarts
        , std::vector<long>& branchColumnEnds
        , std::vector<long>& localColumnStarts
        , std::vector<long>& localColumnEnds);
    void AppendBranch 
        ( CStandardLayoutNodeInfo* start
        , std::vector<long>& columnStarts
        , std::vector<long>& columnEnds
        , std::vector<long>& localColumnStarts
        , std::vector<long>& localColumnEnds);
    void PlaceBranch 
        ( CStandardLayoutNodeInfo* start
        , std::vector<long>& columnStarts
        , std::vector<long>& columnEnds);

    void ShiftNodes 
        ( CStandardLayoutNodeInfo* node
        , CSize delta);
    CRect BoundingRect 
        (const CStandardLayoutNodeInfo* node);

public:

    /// construction

    CStandardNodePositioning (CRevisionGraphOptionList& list);

    /// link to sub-option

    void SetReduceCrossLines (IRevisionGraphOption* reduceCrossLines);

    /// cast @a layout pointer to the respective modification
    /// interface and write the data.

    virtual void ApplyTo (IRevisionGraphLayout* layout);
};
