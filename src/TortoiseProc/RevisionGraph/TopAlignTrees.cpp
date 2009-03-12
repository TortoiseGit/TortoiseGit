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
#include "TopAlignTrees.h"
#include "StandardLayout.h"

void CTopAlignTrees::GetMinMaxY 
    ( IStandardLayoutNodeAccess* nodeAccess
    , std::vector<int>& minY)
{
    for (size_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        const CStandardLayoutNodeInfo* node = nodeAccess->GetNode(i);

        index_t rootID = node->rootID;
        while (minY.size() <= rootID)
            minY.push_back (INT_MAX);

        int& y = minY[rootID];
        y = min (y, node->rect.bottom - node->requiredSize.cy);
    }
}

void CTopAlignTrees::MirrorY 
    ( IStandardLayoutNodeAccess* nodeAccess
    , const std::vector<int>& minY)
{
    for (size_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodeAccess->GetNode(i);

        int y = minY[node->rootID];
        CRect& rect = node->rect;

        rect.top -= y;
        rect.bottom -= y;
    }
}

// construction

CTopAlignTrees::CTopAlignTrees 
    ( CRevisionGraphOptionList& list)
    : CRevisionGraphOptionImpl<ILayoutOption, 1100, ID_VIEW_TOPALIGNTREES> (list)
{
}

// cast @a layout pointer to the respective modification
// interface and write the data.

void CTopAlignTrees::ApplyTo (IRevisionGraphLayout* layout)
{
    // we need access to actual data

    IStandardLayoutNodeAccess* nodeAccess 
        = dynamic_cast<IStandardLayoutNodeAccess*>(layout);
    if (nodeAccess == NULL) 
        return;

    // get the sub-tree dimensions

    std::vector<int> minMaxY;
    GetMinMaxY (nodeAccess, minMaxY);

    // mirror Y coordinates

    MirrorY (nodeAccess, minMaxY);
}
