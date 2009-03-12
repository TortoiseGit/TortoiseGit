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

///////////////////////////////////////////////////////////////
// required includes
///////////////////////////////////////////////////////////////

#include "./Containers/DictionaryBasedTempPath.h"

using namespace LogCache;

///////////////////////////////////////////////////////////////
// forward declarations
///////////////////////////////////////////////////////////////

class CFullGraph;
class CFullGraphNode;
class CVisibleGraphNode;

/**
 * This is a container that stores all nodes that have been
 * collapsed or cut.
 */

class CGraphNodeStates
{
public:

    /** All possible node states. All freely combinable.
     * Not all of them are in use.
     */

    enum
    {
        COLLAPSED_ABOVE = 0x01,  ///< hide previous or copy source node, respectively
        COLLAPSED_BELOW = 0x02,  ///< hide next node (and following) in the same line
        COLLAPSED_LEFT  = 0x04,  ///< not used, yet
        COLLAPSED_RIGHT = 0x08,  ///< hide sub-trees that expand to the right side

        COLLAPSED_ALL   =  CGraphNodeStates::COLLAPSED_ABOVE
                         | CGraphNodeStates::COLLAPSED_RIGHT
                         | CGraphNodeStates::COLLAPSED_LEFT
                         | CGraphNodeStates::COLLAPSED_BELOW,

        SPLIT_ABOVE     = 0x10,  ///< make this a new graph root node
        SPLIT_BELOW     = 0x20,  ///< make the next node a new graph root node
        SPLIT_LEFT      = 0x40,  ///< not used, yet
        SPLIT_RIGHT     = 0x80,  ///< show all sub-trees as separate graphs

        SPLIT_ALL       =  CGraphNodeStates::SPLIT_ABOVE
                         | CGraphNodeStates::SPLIT_RIGHT
                         | CGraphNodeStates::SPLIT_LEFT
                         | CGraphNodeStates::SPLIT_BELOW,
    };

    /// used tempoarily to hold the status while the query is re-run
    /// (i.e. when the node pointers will become invalid)

    typedef std::pair<revision_t, CDictionaryBasedTempPath> TNodeDescriptor;
    typedef std::map<TNodeDescriptor, DWORD> TSavedStates;

    typedef TSavedStates TSavedData;

private:

    /// associates a state to the given graph node

    typedef std::map<const CFullGraphNode*, DWORD> TStates;
    TStates states;

    /// utiltiy methods: restore state from saved data

    void RestoreStates ( const TSavedStates& saved
                       , const CFullGraphNode* node);

    /// \ref ResetFlags() may call this multiple times if links are defined

    void InternalResetFlags (const CFullGraphNode* node, DWORD flags);

    /// traverse the unfiltered tree in the given direction
    /// and look for a suitable node state.

    typedef std::pair<const CFullGraphNode*, DWORD> TFlaggedNode;
    typedef std::vector<const CFullGraphNode*> TFlaggedNodes;

    TFlaggedNode FindPreviousRelevant ( const CVisibleGraphNode* node
                                      , DWORD flags
                                      , bool withinAsWell) const;

    TFlaggedNode FindNextRelevant ( const CVisibleGraphNode* node
                                  , DWORD flags
                                  , bool withinAsWell) const;

    TFlaggedNode FindRightRelevant (const CVisibleGraphNode* node) const;
    TFlaggedNodes FindSplitSubtrees (const CVisibleGraphNode* node) const;

    /// store, update and qeuery state

    void SetFlags (const CFullGraphNode* node, DWORD flags);
    void ResetFlags (const CFullGraphNode* node, DWORD flags);
    DWORD GetFlags (const CFullGraphNode* node) const;

public:

    /// construction / destruction

    CGraphNodeStates(void);
    ~CGraphNodeStates(void);

    /// store, update and qeuery state

    void SetFlags (const CVisibleGraphNode* node, DWORD flags);
    void ResetFlags (const CVisibleGraphNode* node, DWORD flags);

    /// crawl the tree, find the next relavant entries and combine
    /// the status info. Include (or don't) flags between visible
    /// nodes of the same branch.

    DWORD GetFlags (const CVisibleGraphNode* node, bool withinAsWell = false) const;

    /// quick update all

    void ResetFlags (DWORD flags);

    /// disjuctive combination of all flags currently set

    DWORD GetCombinedFlags() const;

    /// re-qeuery support

    TSavedData SaveData() const;
    void LoadData (const TSavedData& saved, const CFullGraph* graph);
};
