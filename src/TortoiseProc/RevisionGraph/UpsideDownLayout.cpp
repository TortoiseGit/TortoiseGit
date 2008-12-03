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
#include "UpsideDownLayout.h"
#include "StandardLayout.h"

std::pair<int, int> CUpsideDownLayout::GetMinMaxY 
    (IStandardLayoutNodeAccess* nodeAccess)
{
    int minY = INT_MAX;
    int maxY = INT_MIN;

    for (size_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        const CRect& rect = nodeAccess->GetNode(i)->rect;
        minY = min (minY, rect.top);
        maxY = max (maxY, rect.bottom);
    }

    return std::make_pair (minY, maxY);
}

void CUpsideDownLayout::MirrorY 
    ( IStandardLayoutNodeAccess* nodeAccess
    , std::pair<int, int> minMaxY)
{
    for (size_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        CRect& rect = nodeAccess->GetNode(i)->rect;
        int origTop = rect.top;

        rect.top = minMaxY.first + minMaxY.second - rect.bottom;
        rect.bottom = minMaxY.first + minMaxY.second - origTop;
    }
}

// construction

CUpsideDownLayout::CUpsideDownLayout 
    ( CRevisionGraphOptionList& list)
    : CRevisionGraphOptionImpl<ILayoutOption, 1000, ID_VIEW_TOPDOWN> (list)
{
}

// implement IRevisionGraphOption: Active if top-down is not selected.

bool CUpsideDownLayout::IsActive() const
{
    return !IsSelected();
}

// cast @a layout pointer to the respective modification
// interface and write the data.

void CUpsideDownLayout::ApplyTo (IRevisionGraphLayout* layout)
{
    // we need access to actual data

    IStandardLayoutNodeAccess* nodeAccess 
        = dynamic_cast<IStandardLayoutNodeAccess*>(layout);
    if (nodeAccess == NULL) 
        return;

    // get dimensions

    std::pair<int, int> minMaxY = GetMinMaxY (nodeAccess);

    // mirror Y coordinates

    MirrorY (nodeAccess, minMaxY);
}
