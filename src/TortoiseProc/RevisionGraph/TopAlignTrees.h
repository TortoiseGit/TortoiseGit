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
class IStandardLayoutNodeAccess;
class CStandardLayoutNodeInfo;

class CTopAlignTrees 
    : public CRevisionGraphOptionImpl<ILayoutOption, 1100, ID_VIEW_TOPALIGNTREES>
{
private:

    /// the individual placement stages

    void GetMinMaxY 
        ( IStandardLayoutNodeAccess* nodeAccess
        , std::vector<int>& minY);
    void MirrorY 
        ( IStandardLayoutNodeAccess* nodeAccess
        , const std::vector<int>& minY);

public:

    /// construction

    CTopAlignTrees (CRevisionGraphOptionList& list);

    /// cast @a layout pointer to the respective modification
    /// interface and write the data.

    virtual void ApplyTo (IRevisionGraphLayout* layout);
};
