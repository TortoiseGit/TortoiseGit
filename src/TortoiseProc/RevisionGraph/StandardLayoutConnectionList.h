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

#include "StandardLayout.h"

class CStandardLayoutConnectionList : public ILayoutConnectionList
{
private:

    const std::vector<CStandardLayoutNodeInfo>& nodes;
    const std::vector<std::pair<index_t, index_t> >& connections;

public:

    /// construction

    CStandardLayoutConnectionList 
        ( const std::vector<CStandardLayoutNodeInfo>& nodes
        , const std::vector<std::pair<index_t, index_t> >& connections);

    /// implement ILayoutItemList

    virtual index_t GetCount() const;

    virtual CString GetToolTip (index_t index) const;

    virtual index_t GetFirstVisible (const CRect& viewRect) const;
    virtual index_t GetNextVisible (index_t prev, const CRect& viewRect) const;
    virtual index_t GetAt (const CPoint& point, long delta) const;

    /// implement ILayoutConnectionList

    virtual SConnection GetConnection (index_t index) const;
};
