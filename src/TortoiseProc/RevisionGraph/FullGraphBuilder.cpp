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
#include "FullGraphBuilder.h"
#include "CachedLogInfo.h"
#include "RevisionIndex.h"
#include "SearchPathTree.h"
#include "FullHistory.h"
#include "FullGraph.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFullGraphBuilder::CFullGraphBuilder (const CFullHistory& history, CFullGraph& graph)
    : history (history)
    , graph (graph)
{
}

CFullGraphBuilder::~CFullGraphBuilder(void)
{
}

void CFullGraphBuilder::Run()
{
    // special case: empty log

    if (history.GetHeadRevision() == NO_REVISION)
        return;

    // frequently used objects

	const CCachedLogInfo* cache = history.GetCache();
	const CRevisionIndex& revisions = cache->GetRevisions();
	const CRevisionInfoContainer& revisionInfo = cache->GetLogInfo();

	// initialize the paths we have to search for

	std::auto_ptr<CSearchPathTree> searchTree 
		(new CSearchPathTree (&revisionInfo.GetPaths()));
    searchTree->Insert (*history.GetStartPath(), history.GetStartRevision());

	// the range of copy-to info that applies to the current revision

    SCopyInfo** lastFromCopy = history.GetFirstCopyFrom();
    SCopyInfo** lastToCopy = history.GetFirstCopyTo();

	// collect nodes to draw ... revision by revision

    for ( revision_t revision = history.GetStartRevision()
        , head = history.GetHeadRevision()
        ; revision <= head
        ; ++revision)
	{
        // any known changes in this revision?

		index_t index = revisions[revision];
		CDictionaryBasedPath basePath = index == NO_INDEX
                                      ? CDictionaryBasedPath (NULL, index)
                                      : revisionInfo.GetRootPath (index);

		if (basePath.IsValid())
        {
	        // collect search paths that have been deleted in this container
	        // (delay potential node deletion until we finished tree traversal)

	        std::vector<CSearchPathTree*> toRemove;

            // special handling for replacements: 
            // we must delete the old branches first and *then* add (some of) these
            // again in the same revision

            if ((   revisionInfo.GetSumChanges (index) 
                  & CRevisionInfoContainer::ACTION_REPLACED) != 0)
            {
		        CSearchPathTree* startNode 
			        = searchTree->FindCommonParent (basePath.GetIndex());

                // crawl (possibly) affected sub-tree

			    AnalyzeReplacements ( revision
						            , revisionInfo.GetChangesBegin (index)
						            , revisionInfo.GetChangesEnd (index)
						            , startNode
						            , toRemove);

		        // remove deleted search paths

		        for (size_t i = 0, count = toRemove.size(); i < count; ++i)
			        toRemove[i]->Remove();

                toRemove.clear();
            }

		    // handle remaining copy-to entries
		    // (some may have a fromRevision that does not touch the fromPath)

		    AddCopiedPaths ( revision
					       , searchTree.get()
					       , lastToCopy);

		    // we are looking for search paths that (may) overlap 
		    // with the revisions' changes

	        // pre-order search-tree traversal

		    CSearchPathTree* startNode 
			    = searchTree->FindCommonParent (basePath.GetIndex());

		    if (startNode->GetPath().IsSameOrChildOf (basePath))
		    {
			    CSearchPathTree* searchNode = startNode;

			    AnalyzeRevisions ( revision
						         , revisionInfo.GetChangesBegin (index)
						         , revisionInfo.GetChangesEnd (index)
						         , searchNode
						         , toRemove);

                startNode = startNode->GetParent();
		    }
		    else
		    {
			    CDictionaryBasedPath commonRoot
				    = basePath.GetCommonRoot (startNode->GetPath().GetBasePath());
			    startNode = searchTree->FindCommonParent (commonRoot.GetIndex());
		    }

        #ifdef _DEBUG
            if (startNode != NULL)
            {
                // only valid for parents of the uppermost modified path

	            for (CRevisionInfoContainer::CChangesIterator iter = revisionInfo.GetChangesBegin (index)
                    , last = revisionInfo.GetChangesEnd (index)
                    ; iter != last
                    ; ++iter)
                {
                    assert (startNode->GetPath().IsSameOrParentOf (iter->GetPathID()));
                }
            }
        #endif

		    // mark changes on parent search nodes

            assert (revisionInfo.GetChangesBegin (index) != revisionInfo.GetChangesEnd (index));

		    for ( CSearchPathTree* searchNode = startNode
			    ; searchNode != NULL
			    ; searchNode = searchNode->GetParent())
	        {
			    if (searchNode->IsActive())
				    AnalyzeAsChanges (revision, searchNode);
	        }

		    // remove deleted search paths

		    for (size_t i = 0, count = toRemove.size(); i < count; ++i)
			    toRemove[i]->Remove();
        }

		// handle remaining copy-to entries
		// (some may have a fromRevision that does not touch the fromPath).
        // We must execute that even if there is no info for this revision
        // (it may still be the copy-from-rev of some add).

		FillCopyTargets ( revision
						, searchTree.get()
						, lastFromCopy);
	}
}

void CFullGraphBuilder::AnalyzeReplacements ( revision_t revision
									          , CRevisionInfoContainer::CChangesIterator first
									          , CRevisionInfoContainer::CChangesIterator last
									          , CSearchPathTree* startNode
									          , std::vector<CSearchPathTree*>& toRemove)
{
	typedef CRevisionInfoContainer::CChangesIterator IT;

	CSearchPathTree* searchNode = startNode;
	do
	{
		// in many cases, we want only to see additions, 
		// deletions and replacements

		bool skipSubTree = true;

		const CDictionaryBasedTempPath& path = searchNode->GetPath();

		// we must not modify inactive nodes

		if (searchNode->IsActive())
		{
			// looking for the closet change that affected the path

			for (IT iter = first; iter != last; ++iter)
			{
				index_t changePathID = iter->GetPathID();

                if (   (iter->GetAction() == CRevisionInfoContainer::ACTION_REPLACED)
				    && (path.GetBasePath().IsSameOrChildOf (changePathID)))
				{
					skipSubTree = false;

					// create & init the new graph node

                    CFullGraphNode* newNode 
                        = graph.Add ( path
                                    , revision
                                    , CNodeClassification::IS_DELETED
                                    , searchNode->GetLastEntry());

					// link entries for the same search path

					searchNode->ChainEntries (newNode);

                    // end of path

					toRemove.push_back (searchNode);

					// we will create at most one node per path and revision

					break;
				}
			}
		}
		else
		{
			// can we skip the whole sub-tree?

			for (IT iter = first; iter != last; ++iter)
			{
				if (path.IsSameOrParentOf (iter->GetPathID()))
				{
					skipSubTree = false;
					break;
				}
			}
		}

		// to the next node

		searchNode = skipSubTree
				   ? searchNode->GetSkipSubTreeNext (startNode)
				   : searchNode->GetPreOrderNext (startNode);
	}
	while (searchNode != startNode);

}

void CFullGraphBuilder::AnalyzeRevisions ( revision_t revision
									  , CRevisionInfoContainer::CChangesIterator first
									  , CRevisionInfoContainer::CChangesIterator last
									  , CSearchPathTree* startNode
									  , std::vector<CSearchPathTree*>& toRemove)
{
	typedef CRevisionInfoContainer::CChangesIterator IT;

	CSearchPathTree* searchNode = startNode;
	do
	{
		// in many cases, we want only to see additions, 
		// deletions and replacements

		bool skipSubTree = true;

		const CDictionaryBasedTempPath& path = searchNode->GetPath();

		// we must not modify inactive nodes

		if (searchNode->IsActive())
		{
			// looking for the closet change that affected the path

			for (IT iter = first; iter != last; ++iter)
			{
				index_t changePathID = iter->GetPathID();

				if (   (path.IsSameOrParentOf (changePathID))
					|| (  (iter->GetAction() != CRevisionInfoContainer::ACTION_CHANGED)
					   && path.GetBasePath().IsSameOrChildOf (changePathID)))
				{
					skipSubTree = false;

					CDictionaryBasedPath changePath = iter->GetPath();

					// construct the classification member

				    // show modifications within the sub-tree as "modified"
				    // (otherwise, deletions would terminate the path)

                    CNodeClassification classification 
                        = CNodeClassification::IS_MODIFIED;
					if (path.GetBasePath().GetIndex() >= changePath.GetIndex())
                    {
                        switch (iter->GetRawChange())
                        {
                        case CRevisionInfoContainer::ACTION_CHANGED: 
                            break;

                        case CRevisionInfoContainer::ACTION_DELETED: 
                            classification = CNodeClassification::IS_DELETED;
                            break;

                        case  CRevisionInfoContainer::ACTION_ADDED
                            + CRevisionInfoContainer::HAS_COPY_FROM: 
                        case  CRevisionInfoContainer::ACTION_REPLACED 
                            + CRevisionInfoContainer::HAS_COPY_FROM: 
                            classification = CNodeClassification::IS_ADDED 
                                           + CNodeClassification::IS_COPY_TARGET;
                            break;

                        case CRevisionInfoContainer::ACTION_ADDED: 
                        case CRevisionInfoContainer::ACTION_REPLACED: 
                            classification = CNodeClassification::IS_ADDED;
                            break;
                        }
                    }

                    // handle copy-from special case:

                    if (   (classification.Is (CNodeClassification::IS_ADDED))
						&& (searchNode->GetLastEntry() != NULL)
                        && !iter->HasFromPath())
					{
						// we may not add paths that already exist:
						// D /trunk/OldSub
						// A /trunk/New
						// A /trunk/New/OldSub	/trunk/OldSub@r-1
                        // don't add /trunk/New again

						continue;
					}

					// create & init the new graph node

                    CFullGraphNode* newNode 
                        = graph.Add (path, revision, classification, searchNode->GetLastEntry());

					// link entries for the same search path

					searchNode->ChainEntries (newNode);

					// end of path?

					if (classification.Is (CNodeClassification::IS_DELETED))
						toRemove.push_back (searchNode);

					// we will create at most one node per path and revision

					break;
				}
			}
		}
		else
		{
			// can we skip the whole sub-tree?

			for (IT iter = first; iter != last; ++iter)
			{
				index_t changePathID = iter->GetPathID();

				if (   path.IsSameOrParentOf (changePathID)
					|| (  (iter->GetAction() != CRevisionInfoContainer::ACTION_CHANGED)
					   && path.GetBasePath().IsSameOrChildOf (changePathID)))
				{
					skipSubTree = false;
					break;
				}
			}
		}

		// to the next node

		searchNode = skipSubTree
				   ? searchNode->GetSkipSubTreeNext (startNode)
				   : searchNode->GetPreOrderNext (startNode);
	}
	while (searchNode != startNode);

}

void CFullGraphBuilder::AnalyzeAsChanges ( revision_t revision
    									 , CSearchPathTree* searchNode)
{
	// create & init the new graph node

    CFullGraphNode* newNode = graph.Add ( searchNode->GetPath()
                                        , revision
                                        , CNodeClassification::IS_MODIFIED
                                        , searchNode->GetLastEntry());

	// link entries for the same search path

	searchNode->ChainEntries (newNode);
}

void CFullGraphBuilder::AddCopiedPaths ( revision_t revision
								         , CSearchPathTree* rootNode
								         , SCopyInfo**& lastToCopy)
{
    // find range of copies that point to this revision

	SCopyInfo** firstToCopy = lastToCopy;
    history.GetCopyToRange (firstToCopy, lastToCopy, revision);

	// create search paths for all *relevant* paths added in this revision

	for (SCopyInfo** iter = firstToCopy; iter != lastToCopy; ++iter)
	{
		const std::vector<SCopyInfo::STarget>& targets = (*iter)->targets;
		for (size_t i = 0, count = targets.size(); i < count; ++i)
		{
			const SCopyInfo::STarget& target = targets[i];
			CSearchPathTree* node = rootNode->Insert (target.path, revision);
			node->ChainEntries (target.source);
		}
	}
}

void CFullGraphBuilder::FillCopyTargets ( revision_t revision
								          , CSearchPathTree* rootNode
								          , SCopyInfo**& lastFromCopy)
{
    // find range of copies that start from this revision

    SCopyInfo** firstFromCopy = lastFromCopy;
    history.GetCopyFromRange (firstFromCopy, lastFromCopy, revision);

	// create search paths for all *relevant* paths added in this revision

	for (SCopyInfo** iter = firstFromCopy; iter != lastFromCopy; ++iter)
	{
		SCopyInfo* copy = *iter;
		std::vector<SCopyInfo::STarget>& targets = copy->targets;

		// crawl the whole sub-tree for path matches

		CSearchPathTree* startNode 
			= rootNode->FindCommonParent (copy->fromPathIndex);
		if (!startNode->GetPath().IsSameOrChildOf (copy->fromPathIndex))
			continue;

		CSearchPathTree* searchNode = startNode;
		do
		{
			const CDictionaryBasedTempPath& path = searchNode->GetPath();
            assert (path.IsSameOrChildOf (copy->fromPathIndex));

			// got this path copied?

			if (searchNode->IsActive())
			{
				CDictionaryBasedPath fromPath ( path.GetDictionary()
											  , copy->fromPathIndex);

                // is there a better match in that target revision?
                // example log @r106:
                // A /trunk/F    /trunk/branches/b/F    100
                // R /trunk/F/a  /trunk/branches/b/F/a  105
                // -> don't copy from r100 but from r105

                if (IsLatestCopySource ( revision
                                       , copy->toRevision
                                       , fromPath
                                       , path))
                {
                    // check for another special case:
                    // A /branches/b    /trunk 100
                    // D /branches/b/a
                    // -> don't add a path if we are following /trunk/a

        			CDictionaryBasedTempPath targetPath
					    = path.ReplaceParent ( fromPath
									         , CDictionaryBasedPath ( path.GetDictionary()
															        , copy->toPathIndex));

                    if (TargetPathExists (copy->toRevision, targetPath.GetBasePath()))
                    {
                        // o.k. this is actual a copy we have to add to the tree

                        CFullGraphNode* entry = searchNode->GetLastEntry();
                        if ((entry == NULL) || (entry->GetRevision() < revision))
				        {
					        // the copy source graph node has yet to be created

                            entry = graph.Add ( path
                                              , revision
                                              , CNodeClassification::IS_COPY_SOURCE
                                              , entry);

					        // link entries for the same search path

					        searchNode->ChainEntries (entry);
				        }

                        // add & schedule the new search path

				        SCopyInfo::STarget target (entry, targetPath);
				        targets.push_back (target);
                    }
			    }
            }

			// select next node

			searchNode = searchNode->GetPreOrderNext (startNode);
		}
		while (searchNode != startNode);
	}
}

bool CFullGraphBuilder::IsLatestCopySource ( revision_t fromRevision
                                             , revision_t toRevision
                                             , const CDictionaryBasedPath& fromPath
                                             , const CDictionaryBasedTempPath& currentPath)
{
    // try to find a "later" / "closer" copy source

    // example log @r106 (toRevision):
    // A /trunk/F    /trunk/branches/b/F    100
    // R /trunk/F/a  /trunk/branches/b/F/a  105
    // -> return r105

    const CCachedLogInfo* cache = history.GetCache();
    const CRevisionInfoContainer& logInfo = cache->GetLogInfo();
    index_t index = cache->GetRevisions()[toRevision];

    // search it

    for ( CRevisionInfoContainer::CChangesIterator 
          iter = logInfo.GetChangesBegin (index)
        , end = logInfo.GetChangesEnd (index)
        ; iter != end
        ; ++iter)
    {
        // is this a copy of the current path?

        if (   iter->HasFromPath() 
            && currentPath.IsSameOrChildOf (iter->GetFromPathID()))
        {
            // a later change?

            if (iter->GetFromRevision() > fromRevision)
                return false;

            // a closer sub-path?

            if (iter->GetFromPathID() > fromPath.GetIndex())
                return false;
        }
    }

    // (fromRevision, fromGraph) is the best match

    return true;
}

bool CFullGraphBuilder::TargetPathExists ( revision_t revision
                                         , const CDictionaryBasedPath& path)
{
    // follow additions and deletions to determine whether the path exists
    // after the given revision

    // A /branches/b    /trunk 100
    // D /branches/b/a
    // -> /branches/b/a does not exist

    const CCachedLogInfo* cache = history.GetCache();
    const CRevisionInfoContainer& logInfo = cache->GetLogInfo();
    index_t index = cache->GetRevisions()[revision];

    // short-cut: if there are no deletions, we should be fine

    if (!(logInfo.GetSumChanges (index) & CRevisionInfoContainer::ACTION_DELETED))
        return true;

    // crawl changes and update this flag:

    bool exists = false;
    for ( CRevisionInfoContainer::CChangesIterator 
          iter = logInfo.GetChangesBegin (index)
        , end = logInfo.GetChangesEnd (index)
        ; iter != end
        ; ++iter)
    {
        // does this change affect the path?

        if (path.IsSameOrChildOf (iter->GetPathID()))
        {
            switch (iter->GetRawChange())
            {
            case CRevisionInfoContainer::ACTION_DELETED :
                // deletion? -> does not exist

                exists = false;
                break;

            case CRevisionInfoContainer::ACTION_ADDED
               + CRevisionInfoContainer::HAS_COPY_FROM:
            case CRevisionInfoContainer::ACTION_REPLACED
               + CRevisionInfoContainer::HAS_COPY_FROM:
                // copy? -> does exist

                // We can safely assume here, that the source tree
                // contains the path in question as this is why we
                // called this function at all.

                exists = true;
                break;

            case CRevisionInfoContainer::ACTION_ADDED:
                // exact addition? -> does exist

                if (iter->GetPathID() == path.GetIndex())
                    exists = true;

                break;
            }
        }
    }

    return exists;
}

