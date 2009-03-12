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

#include "IRevisionGraphLayout.h"

class CVisibleGraphNode;
class CVisibleGraph;

namespace LogCache
{
    class CCachedLogInfo;
}

class CStandardLayoutNodeInfo
{
public:

    /// the node to place within the layout

    const CVisibleGraphNode* node;

    /// links for faster navigation

    CStandardLayoutNodeInfo* parentBranch;
    CStandardLayoutNodeInfo* firstSubBranch;
    CStandardLayoutNodeInfo* nextInBranch;
    CStandardLayoutNodeInfo* previousInBranch;
    CStandardLayoutNodeInfo* lastInBranch;
    CStandardLayoutNodeInfo* nextBranch;
    CStandardLayoutNodeInfo* previousBranch;
    CStandardLayoutNodeInfo* lastBranch;

    /// the graph may consist of multiple trees.
    /// root(node) = graph (node)->GetRoot (rootID);

    index_t rootID;

    /// branch sizes

    index_t subTreeWidth;
    index_t subTreeHeight;
    index_t subTreeWeight;
    index_t branchLength;

    /// node properties

    bool requiresRevision;
    bool requiresPath;
    bool requiresGap;

    /// number of path elements that shall not be shown
    /// (used by "show diff path" option)

    index_t skipStartPathElements;
    index_t skipTailPathElements;

    /// required size(s) to display all content

    CSize requiredSize;

    /// temp. value used to store the offset of the final position
    /// to the current one (logical coordinates)

    CSize treeShift;
    CSize subTreeShift;

    /// actual position (logical coordinates)

    CRect rect;

    /// initialization

    CStandardLayoutNodeInfo();
};

/**
* utility interface that gives layout options access to the layout info
*/

class IStandardLayoutNodeAccess
{
public:

    /// make sub-classes deletable through the base interface

    virtual ~IStandardLayoutNodeAccess() {};

    /// access graph node layout

    virtual index_t GetNodeCount() const = 0;
    virtual CStandardLayoutNodeInfo* GetNode (index_t index) = 0;
};

class CStandardLayout 
    : public IRevisionGraphLayout
    , public IStandardLayoutNodeAccess
{
public:

    struct STextInfo
    {
        index_t nodeIndex;
        int subPathIndex;   // 0 -> revNum, pathElementIndex otherwise

        STextInfo (index_t nodeIndex, int subPathIndex)
            : nodeIndex (nodeIndex)
            , subPathIndex (subPathIndex) 
        {
        }
    };

private:

    /// source of revision data

    const CCachedLogInfo* cache;

    /// logical tree structure

    const CVisibleGraph* graph;

    /// nodes (in the order defined by CVisibleGraphNode::index)

    std::vector<CStandardLayoutNodeInfo> nodes;

    /// connections with length > 0. Stored as node index pairs.

    std::vector<std::pair<index_t, index_t> > connections;

    /// non-empty texts. Stored as node index, 'is path' pairs

    std::vector<STextInfo> texts;

    /// bounding rects of the individual trees

    std::vector<CRect> trees;

    /// area that covers all visible items

    CRect boundingRect;

    /// layout creation

    void SortNodes();
    void InitializeNodes ( const CVisibleGraphNode* node
                         , CStandardLayoutNodeInfo* parentBranch);
    void InitializeNodes();

    void CreateConnections();
    void CreateTexts();

    void CloseTreeBoundingRectGaps();
    void CalculateTreeBoundingRects();

    void CalculateBoundingRect();

public:

    /// construction / destruction

    CStandardLayout (const CCachedLogInfo* cache, const CVisibleGraph* graph);
    virtual ~CStandardLayout(void);

    /// call this after executing the format options

    void Finalize();

    /// implement IRevisionGraphLayout

    virtual CRect GetRect() const;

    virtual const ILayoutRectList* GetTrees() const;
    virtual const ILayoutNodeList* GetNodes() const;
    virtual const ILayoutConnectionList* GetConnections() const;
    virtual const ILayoutTextList* GetTexts() const;

    /// implement IStandardLayoutNodeAccess

    virtual index_t GetNodeCount() const;
    virtual CStandardLayoutNodeInfo* GetNode (index_t index);
};

