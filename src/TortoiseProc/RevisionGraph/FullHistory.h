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

#include "SVN.h"
#include "SVNPrompt.h"
#include "SVNLogQuery.h"
#include "CacheLogQuery.h"

class CFullGraphNode;

/**
 * \ingroup TortoiseProc
 * helper struct containing information about a copy operation in the revision graph
 */
class SCopyInfo
{
public:

	revision_t fromRevision;
	index_t fromPathIndex;
	revision_t toRevision;
	index_t toPathIndex;

	struct STarget
	{
		CFullGraphNode* source;
		CDictionaryBasedTempPath path;

		STarget ( CFullGraphNode* source
				, const CDictionaryBasedTempPath& path)
			: source (source)
			, path (path)
		{
		}
	};

	std::vector<STarget> targets;

    static SCopyInfo* Create (boost::pool<>& copyInfoPool)
    {
        SCopyInfo* result = static_cast<SCopyInfo*>(copyInfoPool.malloc());
        new (result) SCopyInfo();
        return result;
    }

    void Destroy (boost::pool<>& copyInfoPool)
    {
        this->~SCopyInfo();
        copyInfoPool.free (this);
    }
};

/**
 * \ingroup TortoiseProc
 * Handles and analyzes log data to produce a revision graph.
 * 
 * Since Subversion only stores information where each entry is copied \b from
 * and not where it is copied \b to, the first thing we do here is crawl all
 * revisions and create separate CRevisionEntry objects where we store the
 * information where those are copied \b to.
 *
 * In a next step, we go again through all the CRevisionEntry objects to find
 * out if they are related to the path we're looking at. If they are, we mark
 * them as \b in-use.
 */
class CFullHistory : private ILogReceiver
{
public:

    /// construction / destruction

	CFullHistory(void);
	~CFullHistory(void);

    /// query data

	bool						FetchRevisionData ( CString path
                                                  , SVNRev pegRev
                                                  , bool showWCRev
                                                  , bool showWCModification
                                                  , CProgressDlg* progress);

    /// data access

	CString						GetLastErrorMessage() const;

	svn_revnum_t				GetHeadRevision() const {return headRevision;}
	svn_revnum_t				GetPegRevision() const {return pegRevision;}
	CString						GetRepositoryRoot() const {return CString (repoRoot);}
	CString						GetRepositoryUUID() const {return uuid;}
	CString						GetRelativePath() const {return CString (relPath);}

    const CDictionaryBasedTempPath* GetStartPath() const {return startPath.get();}
    revision_t                  GetStartRevision() const {return startRevision;}

    const CDictionaryBasedTempPath* GetWCPath() const {return wcPath.get();}
    revision_t                  GetWCRevision() const {return wcRevision;}
    bool                        GetWCModified() const {return wcModified;}

    SCopyInfo**                 GetFirstCopyFrom() const {return copyFromRelation;}
    SCopyInfo**                 GetFirstCopyTo() const {return copyToRelation;}
    void                        GetCopyFromRange (SCopyInfo**& first, SCopyInfo**& last, revision_t revision) const;
    void                        GetCopyToRange (SCopyInfo**& first, SCopyInfo**& last, revision_t revision) const;

    SVN&                        GetSVN() {return svn;}
    const CCachedLogInfo*       GetCache() const {return cache;}

private:

    /// data members

    CProgressDlg* 	            progress;

    CStringA					repoRoot;
	CStringA					relPath;
    CString                     uuid;
	revision_t					headRevision;
	revision_t					pegRevision;
    revision_t                  firstRevision;

	svn_client_ctx_t 			ctx;
	SVNPrompt					prompt;
	SVN							svn;

	svn_error_t *				Err;			///< Global error object struct
	apr_pool_t *				parentpool;
	apr_pool_t *				pool;			///< memory pool

	bool						cancelled;

    const CCachedLogInfo*       cache;
	std::auto_ptr<CSVNLogQuery> svnQuery;
	std::auto_ptr<CCacheLogQuery> query;

    std::auto_ptr<CDictionaryBasedTempPath> startPath;
    revision_t                  startRevision;

	std::auto_ptr<CDictionaryBasedTempPath> wcPath;
    revision_t                  wcRevision;
    bool                        wcModified;

    boost::pool<>               copyInfoPool;
	std::vector<SCopyInfo*>		copiesContainer;
	SCopyInfo**		            copyToRelation;
	SCopyInfo**		            copyToRelationEnd;
	SCopyInfo**		            copyFromRelation;
	SCopyInfo**		            copyFromRelationEnd;

    /// SVN callback

	static svn_error_t*			cancel(void *baton);

    /// utility methods

    void                        ClearCopyInfo();
	void						AnalyzeRevisionData();
	void						BuildForwardCopies();
	
	/// implement ILogReceiver

	void ReceiveLog ( LogChangedPathArray* changes
					, svn_revnum_t rev
                    , const StandardRevProps* stdRevProps
                    , UserRevPropArray* userRevProps
                    , bool mergesFollow);

};
