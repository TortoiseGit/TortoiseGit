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

#include "RevisionInfoContainer.h"

namespace LogCache
{
    class CDictionaryBasedTempPath;
    class CCachedLogInfo;
}

using namespace LogCache;

class CFullHistory;
class CFullGraph;
class CSearchPathTree;
class SCopyInfo;

/**
 * \ingroup TortoiseProc
 */
class CFullGraphBuilder
{
public:

	CFullGraphBuilder (const CFullHistory& history, CFullGraph& graph);
	~CFullGraphBuilder(void);

	void Run();

private:

	void AnalyzeReplacements ( revision_t revision
						     , CRevisionInfoContainer::CChangesIterator first
						     , CRevisionInfoContainer::CChangesIterator last
						     , CSearchPathTree* startNode
						     , std::vector<CSearchPathTree*>& toRemove);
	void AnalyzeRevisions ( revision_t revision
						  , CRevisionInfoContainer::CChangesIterator first
						  , CRevisionInfoContainer::CChangesIterator last
						  , CSearchPathTree* startNode
						  , std::vector<CSearchPathTree*>& toRemove);
    void AnalyzeAsChanges ( revision_t revision
    					  , CSearchPathTree* searchNode);
	void AddCopiedPaths ( revision_t revision
					    , CSearchPathTree* rootNode
					    , SCopyInfo**& lastToCopy);
	void FillCopyTargets ( revision_t revision
					     , CSearchPathTree* rootNode
					     , SCopyInfo**& lastFromCopy);
    bool IsLatestCopySource ( revision_t fromRevision
                            , revision_t toRevision
                            , const CDictionaryBasedPath& fromPath
                            , const CDictionaryBasedTempPath& currentPath);
    bool TargetPathExists ( revision_t revision
                          , const CDictionaryBasedPath& path);

    /// data members

    const CFullHistory& history;
    CFullGraph& graph;
};
