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
#include "StrictOrderNodePositioning.h"
#include "StandardLayout.h"

#include "VisibleGraph.h"

inline bool CompareNodes 
    ( const std::pair<revision_t, CStandardLayoutNodeInfo*>& lhs
    , const std::pair<revision_t, CStandardLayoutNodeInfo*>& rhs)
{
    return lhs.first > rhs.first;
}

void CStrictOrderNodePositioning::SortRevisions 
    ( IStandardLayoutNodeAccess* nodeAccess
    , std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes)
{
    // fill vector

    index_t count = nodeAccess->GetNodeCount();
    nodes.reserve (count);
    for (index_t i = 0; i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodeAccess->GetNode(i);
        nodes.push_back (std::make_pair (node->node->GetRevision(), node));
    }

    // sort it

    std::sort (nodes.begin(), nodes.end()/*, CompareNodes*/);
}

void CStrictOrderNodePositioning::AssignColumns 
    ( CStandardLayoutNodeInfo* start
    , size_t column
    , std::vector<revision_t>& startRevisions
    , std::vector<revision_t>& endRevisions
    , std::vector<int>& maxWidths)
{
    // there should be no gaps between the branches / trees

    size_t columCount = endRevisions.size();
    assert (columCount >= column);
    assert (columCount == startRevisions.size());
    assert (columCount == maxWidths.size());

    // this range must not be used by the column:

    revision_t firstRevision = start->node->GetRevision();
    revision_t lastRevision = start->lastInBranch->node->GetRevision();

    // find a suitable column

    for (; column < columCount; ++column)
        if (   (firstRevision > endRevisions[column]) 
            || (lastRevision < startRevisions[column]))
        {
            break;
        }

    // mark this branches range as used

    if (columCount > column)
    {
        startRevisions[column] = min (firstRevision, startRevisions[column]);
        endRevisions[column] = max (lastRevision, endRevisions[column]);
    }
    else
    {
        startRevisions.push_back (firstRevision);
        endRevisions.push_back (lastRevision);
        maxWidths.push_back (0);
    }

    // prevent crossing lines / nodes

    if (reduceCrossLines->IsActive() && (start->parentBranch != NULL))
    {
        revision_t connectionFirstRevision = start->parentBranch->node->GetRevision();
        revision_t connectionLastRevision = firstRevision-1;
        for (revision_t i = start->parentBranch->treeShift.cx+1; i <= column; ++i)
        {
            startRevisions[i] = min (connectionFirstRevision, startRevisions[i]);
            endRevisions[i] = max (connectionLastRevision, endRevisions[i]);
        }
    }

    // assign colum to this branch and proceed with sub-branches
    // (as X coordinates, actual coordinates to be assigned in a second run) 

    int maxWidth = 0;
    for ( CStandardLayoutNodeInfo* node = start->lastInBranch
        ; node != NULL
        ; node = node->previousInBranch)
    {
        node->treeShift.cx = static_cast<int>(column);
        if (node->rect.Width() > maxWidth)
            maxWidth = node->rect.Width();

        // add sub-branches

        for ( CStandardLayoutNodeInfo* branch = node->firstSubBranch
            ; branch != NULL
            ; branch = branch->nextBranch)
        {
            AssignColumns ( branch
                          , column+1
                          , startRevisions
                          , endRevisions
                          , maxWidths);
        }
    }

    // update column width

    maxWidths[column] = max (maxWidth, maxWidths[column]);
}

void CStrictOrderNodePositioning::AssignColumns 
    ( std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes
    , std::vector<int>& maxWidths)
{
    std::vector<revision_t> startRevisions;
    std::vector<revision_t> endRevisions;

    startRevisions.reserve (nodes.size());
    endRevisions.reserve (nodes.size());
    maxWidths.reserve (nodes.size());

    for (size_t i = 0, count = nodes.size(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodes[i].second;
        if (   (node->node->GetPrevious() == NULL)
            && (node->node->GetCopySource() == NULL))
        {
            AssignColumns ( node
                          , startRevisions.size()
                          , startRevisions
                          , endRevisions
                          , maxWidths);
        }
    }
}

void CStrictOrderNodePositioning::AssignRows
    (std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes)
{
    // keep track of every columns' top position
    // (probably fewer columns than nodes)

    std::vector<int> columnTops;
    columnTops.insert (columnTops.begin(), nodes.size(), 0);

    // next row start

    int rowStart = 0;

    // process all nodes, group by revision
    // (assign the same row to all nodes of the same revision)

    size_t rangeEnd = 0;
    for ( size_t rangeStart = rangeEnd, count = nodes.size()
        ; rangeStart < count
        ; rangeStart = rangeEnd)
    {
        // find the first entry that hasn't the same revision

        revision_t revision = nodes[rangeStart].first;
        while ((++rangeEnd < count) && (nodes[rangeEnd].first == revision));

        // shift row upward until all nodes match

        for (size_t i = rangeStart; i < rangeEnd; ++i)
        {
            // row must be larger than the top of any column changed 
            // in that revision

            const CStandardLayoutNodeInfo* node = nodes[i].second;
            int column = node->treeShift.cx;
            rowStart = max (rowStart, columnTops[column]);

            // copy targets must be pushed down even further

            const CStandardLayoutNodeInfo* parent = node->parentBranch;
            if ((parent != NULL) && (node->previousInBranch == NULL))
            {
                int sourceBottom = parent->rect.bottom + parent->treeShift.cy;
                rowStart = max (rowStart, sourceBottom);
                rowStart = max (rowStart, columnTops[column] + 6);
            }

            // copy sources as well

            if (reduceCrossLines->IsActive())
            {
                int maxTargetColumn = column;
                for ( const CStandardLayoutNodeInfo* subBranch = node->firstSubBranch
                    ; subBranch != NULL
                    ; subBranch = subBranch->nextBranch)
                {
                    maxTargetColumn = max (maxTargetColumn, subBranch->treeShift.cx);
                }

                int halfHeight = node->rect.Height() / 2;
                for (int i = column+1; i <= maxTargetColumn; ++i)
                {
                    rowStart = max (rowStart, columnTops[i] - halfHeight + 6);
                }
            }
        }

        // assign rowStarts

        for (size_t i = rangeStart; i < rangeEnd; ++i)
        {
            CStandardLayoutNodeInfo* node = nodes[i].second;
            int column = node->treeShift.cx;
            int height = node->rect.Height();

            columnTops[column] = rowStart + height;
            node->treeShift.cy = rowStart;
        }

        // minimum shift between consequtive revisions on different branches

        rowStart += 10;
    }
}

void CStrictOrderNodePositioning::ShiftNodes 
    ( std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> >& nodes
    , std::vector<int>& columWidths)
{
    // calculate column positions

    std::vector<int> columnPositions;
    columnPositions.reserve (columnPositions.size());

    int columnStart = 0;
    for (size_t i = 0, count = columWidths.size(); i < count; ++i)
    {
        columnPositions.push_back (columnStart);
        columnStart += 50 + columWidths[i];
    }

    // move nodes

    for (size_t i = 0, count = nodes.size(); i < count; ++i)
    {
        CStandardLayoutNodeInfo* node = nodes[i].second;
        CSize shift (columnPositions [node->treeShift.cx], node->treeShift.cy);
        node->rect += shift;
    }
}

// construction

CStrictOrderNodePositioning::CStrictOrderNodePositioning 
    ( CRevisionGraphOptionList& list
    , IRevisionGraphOption* standardNodePositioning
    , IRevisionGraphOption* reduceCrossLines)
    : CRevisionGraphOptionImpl<ILayoutOption, 200, 0> (list)
    , standardNodePositioning (standardNodePositioning)
    , reduceCrossLines (reduceCrossLines)
{
}

// implement IRevisionGraphOption: Active if standard layout is disabled.

bool CStrictOrderNodePositioning::IsActive() const
{
    return !standardNodePositioning->IsSelected();
}

// cast @a layout pointer to the respective modification
// interface and write the data.

void CStrictOrderNodePositioning::ApplyTo (IRevisionGraphLayout* layout)
{
    // we need access to actual data

    IStandardLayoutNodeAccess* nodeAccess 
        = dynamic_cast<IStandardLayoutNodeAccess*>(layout);
    if (nodeAccess == NULL) 
        return;

    // assign columns and rows

    std::vector<std::pair<revision_t, CStandardLayoutNodeInfo*> > nodes;
    SortRevisions (nodeAccess, nodes);

    std::vector<int> columnWidths;
    AssignColumns (nodes, columnWidths);
    AssignRows (nodes);

    // post-processing

    ShiftNodes (nodes, columnWidths);
}
