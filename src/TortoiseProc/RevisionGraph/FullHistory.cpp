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
#include "fullHistory.h"

#include "resource.h"
#include "client.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "PathUtils.h"
#include "SVN.h"
#include "TSVNPath.h"
#include "SVNError.h"
#include "SVNInfo.h"
#include "CachedLogInfo.h"
#include "RepositoryInfo.h"
#include "RevisionIndex.h"
#include "CopyFollowingLogIterator.h"
#include "ProgressDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CFullHistory::CFullHistory(void) 
    : cancelled (false)
    , progress (NULL)
    , headRevision ((revision_t)NO_REVISION)
    , pegRevision ((revision_t)NO_REVISION)
    , wcRevision ((revision_t)NO_REVISION)
    , copyInfoPool (sizeof (SCopyInfo), 1024)
    , copyToRelation (NULL)
    , copyToRelationEnd (NULL)
    , copyFromRelation (NULL)
    , copyFromRelationEnd (NULL)
    , cache (NULL)
{
	memset (&ctx, 0, sizeof (ctx));
	parentpool = svn_pool_create(NULL);

	Err = svn_config_ensure(NULL, parentpool);
	pool = svn_pool_create (parentpool);
	// set up the configuration
	if (Err == 0)
		Err = svn_config_get_config (&(ctx.config), g_pConfigDir, pool);

	if (Err != 0)
	{
		::MessageBox(NULL, this->GetLastErrorMessage(), _T("TortoiseSVN"), MB_ICONERROR);
		svn_error_clear(Err);
		svn_pool_destroy (pool);
		svn_pool_destroy (parentpool);
		exit(-1);
	}

	// set up authentication
	prompt.Init(pool, &ctx);

	ctx.cancel_func = cancel;
	ctx.cancel_baton = this;

	//set up the SVN_SSH param
	CString tsvn_ssh = CRegString(_T("Software\\TortoiseGit\\SSH"));
	if (tsvn_ssh.IsEmpty())
		tsvn_ssh = CPathUtils::GetAppDirectory() + _T("TortoisePlink.exe");
	tsvn_ssh.Replace('\\', '/');
	if (!tsvn_ssh.IsEmpty())
	{
		svn_config_t * cfg = (svn_config_t *)apr_hash_get (ctx.config, SVN_CONFIG_CATEGORY_CONFIG,
			APR_HASH_KEY_STRING);
		svn_config_set(cfg, SVN_CONFIG_SECTION_TUNNELS, "ssh", CUnicodeUtils::GetUTF8(tsvn_ssh));
	}
}

CFullHistory::~CFullHistory(void)
{
    ClearCopyInfo();

    svn_error_clear(Err);
	svn_pool_destroy (parentpool);
}

void CFullHistory::ClearCopyInfo()
{
	delete copyToRelation;
	delete copyFromRelation;

	copyToRelation = NULL;
	copyToRelationEnd = NULL;
	copyFromRelation = NULL;
	copyFromRelationEnd = NULL;

    for (size_t i = 0, count = copiesContainer.size(); i < count; ++i)
        copiesContainer[i]->Destroy (copyInfoPool);

    copiesContainer.clear();
}

svn_error_t* CFullHistory::cancel(void *baton)
{
	CFullHistory * self = (CFullHistory *)baton;
	if (self->cancelled)
	{
		CString temp;
		temp.LoadString(IDS_SVN_USERCANCELLED);
		return svn_error_create(SVN_ERR_CANCELLED, NULL, CUnicodeUtils::GetUTF8(temp));
	}
	return SVN_NO_ERROR;
}

// implement ILogReceiver

void CFullHistory::ReceiveLog ( LogChangedPathArray* changes
					          , svn_revnum_t rev
                              , const StandardRevProps* stdRevProps
                              , UserRevPropArray* userRevProps
                              , bool mergesFollow)
{
    // fix release mode compiler warning

    UNREFERENCED_PARAMETER(changes);
    UNREFERENCED_PARAMETER(stdRevProps);
    UNREFERENCED_PARAMETER(userRevProps);
    UNREFERENCED_PARAMETER(mergesFollow);

	// update internal data

	if ((headRevision < (revision_t)rev) || (headRevision == NO_REVISION))
		headRevision = rev;

	// update progress bar and check for user pressing "Cancel" somewhere

	static DWORD lastProgressCall = 0;
	if (lastProgressCall < GetTickCount() - 200)
	{
		lastProgressCall = GetTickCount();

	    if (progress)
	    {
		    CString text, text2;
		    text.LoadString(IDS_REVGRAPH_PROGGETREVS);
		    text2.Format(IDS_REVGRAPH_PROGCURRENTREV, rev);

		    progress->SetLine(1, text);
		    progress->SetLine(2, text2);
		    progress->SetProgress (headRevision - rev, headRevision);
            if (!progress->IsVisible())
    	        progress->ShowModeless ((CWnd*)NULL);

		    if (progress->HasUserCancelled())
		    {
			    cancelled = true;
			    throw SVNError (cancel (this));
		    }
        }
	}
}

bool CFullHistory::FetchRevisionData ( CString path
                                     , SVNRev pegRev
                                     , bool showWCRev
                                     , CProgressDlg* progress)
{
	// set some text on the progress dialog, before we wait
	// for the log operation to start
    this->progress = progress;

	CString temp;
	temp.LoadString(IDS_REVGRAPH_PROGGETREVS);
    progress->SetLine(1, temp);
    progress->SetLine(2, _T(""));
    progress->SetProgress(0, 1);

	// prepare the path for Subversion
    CTSVNPath svnPath (path);
	CStringA url = CPathUtils::PathEscape 
                        (CUnicodeUtils::GetUTF8 
                            (svn.GetURLFromPath (svnPath)));

    // we have to get the log from the repository root

	CTSVNPath rootPath;
    svn_revnum_t head;
    if (FALSE == svn.GetRootAndHead (svnPath, rootPath, head))
	{
		Err = svn_error_dup(svn.Err);
		return false;
	}

    if (pegRev.IsHead())
        pegRev = head;

    headRevision = head;
    repoRoot = rootPath.GetSVNPathString();
    relPath = CPathUtils::PathUnescape (url.Mid (repoRoot.GetLength()));
    repoRoot = CPathUtils::PathUnescape (repoRoot);

	// fix issue #360: use WC revision as peg revision

    pegRevision = pegRev;
    if (pegRevision == NO_REVISION)
    {
	    CTSVNPath svnPath (path);
	    if (!svnPath.IsUrl())
	    {
		    SVNInfo info;
		    const SVNInfoData * baseInfo 
			    = info.GetFirstFileInfo (svnPath, SVNRev(), SVNRev());
            if (baseInfo != NULL)
                pegRevision = baseInfo->rev;
	    }
    }

	// fetch missing data from the repository
	try
	{
        // select / construct query object and optimize revision range to fetch

		svnQuery.reset (new CSVNLogQuery (&ctx, pool));
        revision_t firstRevision = 0;

        if (svn.GetLogCachePool()->IsEnabled())
        {
            CLogCachePool* pool = svn.GetLogCachePool();
		    query.reset (new CCacheLogQuery (pool, svnQuery.get()));

            // get the cache and the lowest missing revision
            // (in off-line mode, the query may not find the cache as 
            // it cannot contact the server to get the UUID)

            uuid = pool->GetRepositoryInfo().GetRepositoryUUID (svnPath);
            cache = pool->GetCache (uuid, GetRepositoryRoot());
            firstRevision = cache->GetRevisions().GetFirstMissingRevision(1);

			// if the cache is already complete, the firstRevision here is
			// HEAD+1 - that revision does not exist and would throw an error later

			if (firstRevision > headRevision)
				firstRevision = headRevision;
        }
        else
        {
		    query.reset (new CCacheLogQuery (svn, svnQuery.get()));
            cache = NULL;
        }

        // actually fetch the data

		query->Log ( CTSVNPathList (rootPath)
				   , headRevision
				   , headRevision
				   , firstRevision
				   , 0
				   , false		// strictNodeHistory
				   , this
                   , false		// includeChanges (log cache fetches them automatically)
                   , false		// includeMerges
                   , true		// includeStandardRevProps
                   , false		// includeUserRevProps
                   , TRevPropNames());

        // store WC path

        if (cache == NULL)
	        cache = query->GetCache();

	    const CPathDictionary* paths = &cache->GetLogInfo().GetPaths();
        wcPath.reset (new CDictionaryBasedTempPath (paths, (const char*)relPath));
        wcRevision = pegRev;

	    // Find the revision the working copy is on, we mark that revision
	    // later in the graph (handle option changes properly!).
        // For performance reasons, we only don't do it if we want to display it.

        if (showWCRev)
        {
            svn_revnum_t maxrev = wcRevision;
            svn_revnum_t minrev;
	        bool switched, modified, sparse;
	        if (svn.GetWCRevisionStatus ( CTSVNPath (path)
								        , true
								        , minrev
								        , maxrev
								        , switched
								        , modified
								        , sparse))
	        {
		        wcRevision = maxrev;
	        }
        }

        // analyse the data

        AnalyzeRevisionData();

        // pre-process log data (invert copy-relationship)

    	BuildForwardCopies();
	}
	catch (SVNError& e)
	{
		Err = svn_error_create (e.GetCode(), NULL, e.GetMessage());
		return false;
	}

	return true;
}

void CFullHistory::AnalyzeRevisionData()
{
	svn_error_clear(Err);
    Err = NULL;

	ClearCopyInfo();

    // special case: empty log

    if (headRevision == NO_REVISION)
        return;

    // we don't have a peg revision yet, set it to HEAD

    if (pegRevision == NO_REVISION)
        pegRevision = headRevision;

    // in case our path was renamed and had a different name in the past,
	// we have to find out that name now, because we will analyze the data
	// from lower to higher revisions

    startPath.reset (new CDictionaryBasedTempPath (*wcPath));

    CCopyFollowingLogIterator iterator (cache, pegRevision, *startPath);
	iterator.Retry();
	startRevision = pegRevision;

	while ((iterator.GetRevision() > 0) && !iterator.EndOfPath())
	{
        if (iterator.DataIsMissing())
        {
            iterator.ToNextAvailableData();
        }
        else
        {
		    startRevision = iterator.GetRevision();
    		iterator.Advance();
        }
	}

	*startPath = iterator.GetPath();
}

inline bool AscendingFromRevision (const SCopyInfo* lhs, const SCopyInfo* rhs)
{
	return lhs->fromRevision < rhs->fromRevision;
}

inline bool AscendingToRevision (const SCopyInfo* lhs, const SCopyInfo* rhs)
{
	return lhs->toRevision < rhs->toRevision;
}

void CFullHistory::BuildForwardCopies()
{
	// iterate through all revisions and fill copyToRelation:
	// for every copy-from info found, add an entry

	const CRevisionIndex& revisions = cache->GetRevisions();
	const CRevisionInfoContainer& revisionInfo = cache->GetLogInfo();

	// for all revisions ...

	copiesContainer.reserve (revisions.GetLastRevision());
	for ( revision_t revision = revisions.GetFirstRevision()
		, last = revisions.GetLastRevision()
		; revision < last
		; ++revision)
	{
		// ... in the cache ...

		index_t index = revisions[revision];
		if (   (index != NO_INDEX) 
			&& (revisionInfo.GetSumChanges (index) & CRevisionInfoContainer::HAS_COPY_FROM))
		{
			// ... examine all changes ...

			for ( CRevisionInfoContainer::CChangesIterator 
					iter = revisionInfo.GetChangesBegin (index)
				, end = revisionInfo.GetChangesEnd (index)
				; iter != end
				; ++iter)
			{
				// ... and if it has a copy-from info ...

				if (iter->HasFromPath())
				{
					// ... add it to the list

                    SCopyInfo* copyInfo = SCopyInfo::Create (copyInfoPool);

					copyInfo->fromRevision = iter->GetFromRevision();
					copyInfo->fromPathIndex = iter->GetFromPathID();
					copyInfo->toRevision = revision;
					copyInfo->toPathIndex = iter->GetPathID();

					copiesContainer.push_back (copyInfo);
				}
			}
		}
	}

	// sort container by source revision and path

    copyToRelation = new SCopyInfo*[copiesContainer.size()];
    copyToRelationEnd = copyToRelation + copiesContainer.size();

    copyFromRelation = new SCopyInfo*[copiesContainer.size()];
    copyFromRelationEnd = copyFromRelation + copiesContainer.size();
#pragma warning( push )
#pragma warning( disable : 4996 )
	std::copy (copiesContainer.begin(), copiesContainer.end(), copyToRelation);
    std::copy (copiesContainer.begin(), copiesContainer.end(), copyFromRelation);
#pragma warning( pop ) 

	std::sort (copyToRelation, copyToRelationEnd, &AscendingToRevision);
	std::sort (copyFromRelation, copyFromRelationEnd, &AscendingFromRevision);
}

CString CFullHistory::GetLastErrorMessage() const
{
	return SVN::GetErrorString(Err);
}

void CFullHistory::GetCopyFromRange ( SCopyInfo**& first
                                    , SCopyInfo**& last
                                    , revision_t revision) const
{
	// find first entry for this revision (or first beyond)

	while (   (first != copyFromRelationEnd) 
		   && ((*first)->fromRevision < revision))
		++first;

	// find first beyond this revision

	last = first;
	while (   (last != copyFromRelationEnd) 
		   && ((*last)->fromRevision <= revision))
		++last;
}

void CFullHistory::GetCopyToRange ( SCopyInfo**& first
                                  , SCopyInfo**& last
                                  , revision_t revision) const
{
	// find first entry for this revision (or first beyond)

	while (   (first != copyToRelationEnd) 
		   && ((*first)->toRevision < revision))
		++first;

	// find first beyond this revision

	last = first;
	while (   (last != copyToRelationEnd) 
		   && ((*last)->toRevision <= revision))
		++last;
}
