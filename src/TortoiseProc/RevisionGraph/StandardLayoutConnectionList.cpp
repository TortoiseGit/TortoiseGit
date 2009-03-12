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
#include "StandardLayoutConnectionList.h"

// construction

CStandardLayoutConnectionList::CStandardLayoutConnectionList 
    ( const std::vector<CStandardLayoutNodeInfo>& nodes
    , const std::vector<std::pair<index_t, index_t> >& connections)
    : nodes (nodes)
    , connections (connections)
{
}

// implement ILayoutItemList

index_t CStandardLayoutConnectionList::GetCount() const
{
    return static_cast<index_t>(connections.size());
}

CString CStandardLayoutConnectionList::GetToolTip (index_t /* index */) const
{
    return CString();
}

index_t CStandardLayoutConnectionList::GetFirstVisible 
    (const CRect& viewRect) const
{
    return GetNextVisible (static_cast<index_t>(-1), viewRect);
}

index_t CStandardLayoutConnectionList::GetNextVisible 
    ( index_t prev
    , const CRect& viewRect) const
{
    for (size_t i = prev+1, count = connections.size(); i < count; ++i)
    {
        const std::pair<index_t, index_t>& connection = connections[i];

        CRect commonRect;
        commonRect.UnionRect ( nodes[connection.first].rect
                             , nodes[connection.second].rect);
        if (FALSE != CRect().IntersectRect (commonRect, viewRect))
            return static_cast<index_t>(i);
    }

    return static_cast<index_t>(NO_INDEX);
}

index_t CStandardLayoutConnectionList::GetAt 
    ( const CPoint& /* point */
    , CSize /* delta */) const
{
    return static_cast<index_t>(NO_INDEX);
}

// implement ILayoutConnectionList

CStandardLayoutConnectionList::SConnection 
CStandardLayoutConnectionList::GetConnection (index_t index) const
{
    // determine the end points of the connection

    const std::pair<index_t, index_t>& connection = connections[index];
    const CRect* source = &nodes[connection.first].rect;
    const CRect* dest = &nodes[connection.second].rect;

    if (source->left > dest->right)
        std::swap (source, dest);

	CPoint startPoint;
	CPoint endPoint;

    if (source->right < dest->left)
    {
        // there is a vertical gap between source and dest

        startPoint.x = source->right;
        startPoint.y = (source->top + source->bottom) / 2;
    }
    else
    {
        // there must be a horizontal gap

        if (source->top > dest->top)
            std::swap (source, dest);

        startPoint.x = (source->left + source->right) / 2;
        startPoint.y = source->bottom;
    }

    endPoint.x = (dest->left + dest->right) / 2;
    endPoint.y = dest->top > startPoint.y ? dest->top : dest->bottom;
    
    // Bezier points

    static CPoint points[4];

	points[0] = startPoint;
	points[1].x = (startPoint.x + endPoint.x) / 2;		// first control point
	points[1].y = startPoint.y;
	points[2].x = endPoint.x;							// second control point
	points[2].y = startPoint.y;
	points[3] = endPoint;

    // construct result

    SConnection result;

    result.style = 0;
    result.numberOfPoints = 4;
    result.points = points;

    return result;
}
