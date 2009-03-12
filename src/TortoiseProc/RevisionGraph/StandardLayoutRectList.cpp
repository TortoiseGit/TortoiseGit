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
#include "StandardLayoutRectList.h"

// construction

CStandardLayoutRectList::CStandardLayoutRectList 
    ( const std::vector<CRect>& rects)
    : rects (rects)
{
}

// implement ILayoutItemList

index_t CStandardLayoutRectList::GetCount() const
{
    return static_cast<index_t>(rects.size());
}

CString CStandardLayoutRectList::GetToolTip (index_t /* index */) const
{
    return CString();
}

index_t CStandardLayoutRectList::GetFirstVisible (const CRect& viewRect) const
{
    return GetNextVisible (static_cast<index_t>(-1), viewRect);
}

index_t CStandardLayoutRectList::GetNextVisible ( index_t prev
                                                , const CRect& viewRect) const
{
    for (size_t i = prev+1, count = rects.size(); i < count; ++i)
        if (FALSE != CRect().IntersectRect (rects[i], viewRect))
            return static_cast<index_t>(i);

    return static_cast<index_t>(NO_INDEX);
}

index_t CStandardLayoutRectList::GetAt (const CPoint& point, CSize delta) const
{
    for (size_t i = 0, count = rects.size(); i < count; ++i)
    {
        const CRect& rect = rects[i];
        if (   (rect.top - point.y <= delta.cy)
            && (rect.left - point.x <= delta.cx)
            && (point.y - rect.bottom <= delta.cy)
            && (point.x - rect.right <= delta.cx))
        {
            return static_cast<index_t>(i);
        }
    }

    return static_cast<index_t>(NO_INDEX);
}

// implement ILayoutRectList

CRect CStandardLayoutRectList::GetRect (index_t index) const
{
    return rects[index];
}
