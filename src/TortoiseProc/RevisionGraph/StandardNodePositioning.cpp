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
#include "StandardNodePositioning.h"
#include "StandardLayout.h"

#include "VisibleGraph.h"

void CStandardNodePositioning::StackSubTree 
    ( CStandardLayoutNodeInfo* node
    , std::vector<long>& branchColumnStarts
    , std::vector<long>& branchColumnEnds
    , std::vector<long>& localColumnStarts
    , std::vector<long>& localColumnEnds)
{
    // the highest position allowed for any branch
    // (if actually reached, node must be at 0,0)

    long branchMinY = localColumnEnds[0];

    // shift sub-branches down by at least this value to make room for
    // the connection line to it

    long connectionShift = node->rect.top + node->rect.Height() / 2 + 10;

    // shift subtree downwards until there is no overlap with upper sub-trees

    long subTreeMinY = branchMinY + connectionShift;
    if (reduceCrossLines->IsActive())
    {
        // create a "stairs-like" branch start:
        // columns must start at the same line or above their right neigbour

        long minStart = LONG_MAX;
        for (size_t i = branchColumnStarts.size(); i > 0; --i)
        {
            minStart = min (minStart, branchColumnStarts[i-1] - connectionShift);
            branchColumnStarts[i-1] = minStart;

            subTreeMinY = max (subTreeMinY, localColumnEnds[i] - minStart);
        }
    }
    else
    {
        for (size_t i = 0, count = branchColumnStarts.size(); i < count; ++i)
        {
            subTreeMinY = max ( subTreeMinY
                              , localColumnEnds[i+1] - branchColumnStarts[i] + connectionShift);
        }
    }

    // store how much the sub-tree has to be shifted
    // (will be applied to .rect in a second pass)

    node->subTreeShift.cx = node->requiredSize.cx + 50;
    node->subTreeShift.cy = subTreeMinY;

    // adjust y-coord of the start node

    long nodeYShift = node->subTreeShift.cy - connectionShift;
    node->rect.top += nodeYShift;
    node->rect.bottom += nodeYShift;

    // update column heights

    long subTreeYShift = node->subTreeShift.cy;
    localColumnStarts[0] = min (localColumnStarts[0], node->rect.top);
    localColumnEnds[0] = max (localColumnEnds[0], node->rect.top + node->requiredSize.cy);

    for (size_t i = 0, count = branchColumnStarts.size(); i < count; ++i)
    {
        localColumnStarts[i+1] = min ( localColumnStarts[i+1]
                                     , subTreeYShift + branchColumnStarts[i]);
        localColumnEnds[i+1] = max ( localColumnEnds[i+1]
                                   , subTreeYShift + branchColumnEnds[i]);
    }
}

void CStandardNodePositioning::AppendBranch 
    ( CStandardLayoutNodeInfo* start
    , std::vector<long>& columnStarts
    , std::vector<long>& columnEnds
    , std::vector<long>& localColumnStarts
    , std::vector<long>& localColumnEnds)
{
    // push the new branch as far to the left as possible

    size_t startsCount = columnStarts.size();
    size_t localCount = localColumnEnds.size();

    size_t insertionIndex = startsCount;
    while (insertionIndex > 0)
    {
        bool fits = true;
        for ( size_t i = 0
            , count = min (startsCount - insertionIndex+1, localCount)
            ; i < count
            ; ++i)
        {
            if (columnStarts [i + insertionIndex-1] < localColumnEnds [i] + 4)
            {
                fits = false;
                break;
            }
        }

        if (fits == false)
            break;

        --insertionIndex;
    }

    // move the whole branch to the right

    start->treeShift.cx = insertionIndex * 250;

    // update the overlapping part

    size_t offset = min (localCount, startsCount - insertionIndex);
    for (size_t i = 0; i < offset; ++i)
        columnStarts [i + insertionIndex] = localColumnStarts[i];

    // just append the column y-ranges 
    // (column 0 is for the chain that starts at the "start" node)

    columnStarts.insert ( columnStarts.end()
                        , localColumnStarts.begin() + offset
                        , localColumnStarts.end());
    columnEnds.insert ( columnEnds.end()
                      , localColumnEnds.begin() + offset
                      , localColumnEnds.end());
}

void CStandardNodePositioning::PlaceBranch 
    ( CStandardLayoutNodeInfo* start
    , std::vector<long>& columnStarts
    , std::vector<long>& columnEnds)
{
    // lower + upper bounds for the start node and all its branche columns

    std::vector<long> localColumnStarts;
    std::vector<long> localColumnEnds;

    // lower + upper bounds for the columns of one sub-branch of the start node

    std::vector<long> branchColumnStarts;
    std::vector<long> branchColumnEnds;

    for ( CStandardLayoutNodeInfo* node = start
        ; node != NULL
        ; node = node->nextInBranch)
    {
        // collect branches

        branchColumnStarts.clear();
        branchColumnEnds.clear();

        for ( CStandardLayoutNodeInfo* branch = node->firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
        {
            PlaceBranch (branch, branchColumnStarts, branchColumnEnds);
        }

        // stack them and this node

        size_t subTreeWidth = branchColumnEnds.size()+1;
        if (localColumnStarts.size() < subTreeWidth)
        {
            localColumnStarts.resize (subTreeWidth, LONG_MAX);
            localColumnEnds.resize (subTreeWidth, 0);
        }

        StackSubTree ( node
                     , branchColumnStarts, branchColumnEnds
                     , localColumnStarts, localColumnEnds);
    }

    // append node and branches horizontally to sibblings of the start node

    AppendBranch ( start
                 , columnStarts, columnEnds
                 , localColumnStarts, localColumnEnds);
}

void CStandardNodePositioning::ShiftNodes 
    ( CStandardLayoutNodeInfo* node
    , CSize delta)
{
    // walk along this branch

    delta += node->treeShift;
    for ( ; node != NULL; node = node->nextInBranch)
    {
        node->rect += delta;
        node->subTreeShift += delta;

        // shift sub-branches

        for ( CStandardLayoutNodeInfo* branch = node->firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
        {
            ShiftNodes (branch, node->subTreeShift);
        }
    }
}

CRect CStandardNodePositioning::BoundingRect 
    (const CStandardLayoutNodeInfo* node)
{
    // walk along this branch

    CRect result = node->rect;
    for ( ; node != NULL; node = node->nextInBranch)
    {
        result.UnionRect (result, node->rect);

        // shift sub-branches

        for ( CStandardLayoutNodeInfo* branch = node->firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
        {
            result.UnionRect (result, BoundingRect (branch));
        }
    }

    return result;
}

// construction

CStandardNodePositioning::CStandardNodePositioning 
    ( CRevisionGraphOptionList& list)
    : CRevisionGraphOptionImpl<ILayoutOption, 200, ID_VIEW_GROUPBRANCHES> (list)
    , reduceCrossLines (NULL)
{
}

// link to sub-option

void CStandardNodePositioning::SetReduceCrossLines 
    (IRevisionGraphOption* reduceCrossLines)
{
    this->reduceCrossLines = reduceCrossLines;
}

// cast @a layout pointer to the respective modification
// interface and write the data.

void CStandardNodePositioning::ApplyTo (IRevisionGraphLayout* layout)
{
    // we need access to actual data

    IStandardLayoutNodeAccess* nodeAccess 
        = dynamic_cast<IStandardLayoutNodeAccess*>(layout);
    if (nodeAccess == NULL) 
        return;

    // calculate the displacement for every node (member subTreeShift)

    CSize treeShift (0,0);
    for (index_t i = 0, count = nodeAccess->GetNodeCount(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodeAccess->GetNode(i);
        if (node->node->GetSource() == NULL)
        {
            // we found a root -> place it

            std::vector<long> columnStarts;
            std::vector<long> ColumnEnds;

            PlaceBranch (node, columnStarts, ColumnEnds);

            // actually move the node rects to thier final position

            ShiftNodes (node, treeShift);
            treeShift.cx = BoundingRect (node).right + 100;
        }
    }
}
