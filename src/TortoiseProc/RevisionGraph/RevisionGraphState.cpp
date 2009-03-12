// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2009 - TortoiseSVN

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
#include "stdafx.h"
#include "RevisionGraphState.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// construction / destruction

CRevisionGraphState::CRevisionGraphState()
	: fetchedWCState(false)
    , options (&nodeStates)
{
}

CRevisionGraphState::~CRevisionGraphState()
{
}

// basic, synchronized data read access

CSyncPointer<const CAllRevisionGraphOptions> CRevisionGraphState::GetOptions() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const CAllRevisionGraphOptions>
        ( &mutex
        , &options
        , false);
}

CSyncPointer<const CFullHistory> CRevisionGraphState::GetFullHistory() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const CFullHistory>
        ( &mutex
        , fullHistory.get()
        , false);
}

CSyncPointer<const CFullGraph> CRevisionGraphState::GetFullGraph() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const CFullGraph>
        ( &mutex
        , fullGraph.get()
        , false);
}

CSyncPointer<const CVisibleGraph> CRevisionGraphState::GetVisibleGraph() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const CVisibleGraph>
        ( &mutex
        , visibleGraph.get()
        , false);
}

CSyncPointer<const ILayoutNodeList> CRevisionGraphState::GetNodes() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const ILayoutNodeList>
        ( &mutex
        , layout.get() == NULL ? NULL : layout->GetNodes()
        , true);
}

CSyncPointer<const ILayoutConnectionList> CRevisionGraphState::GetConnections() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const ILayoutConnectionList>
        ( &mutex
        , layout.get() == NULL ? NULL : layout->GetConnections()
        , true);
}

CSyncPointer<const ILayoutTextList> CRevisionGraphState::GetTexts() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const ILayoutTextList>
        ( &mutex
        , layout.get() == NULL ? NULL : layout->GetTexts()
        , true);
}

CSyncPointer<const ILayoutRectList> CRevisionGraphState::GetTrees() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const ILayoutRectList>
        ( &mutex
        , layout.get() == NULL ? NULL : layout->GetTrees()
        , true);
}

bool CRevisionGraphState::GetFetchedWCState() const
{
    return fetchedWCState;
}

CString CRevisionGraphState::GetLastErrorMessage() const
{
    CSingleLock lock (&mutex);
    return lastErrorMessage;
}

CSyncPointer<CAllRevisionGraphOptions> CRevisionGraphState::GetOptions() 
{
    CSingleLock lock (&mutex);
    return CSyncPointer<CAllRevisionGraphOptions>
        ( &mutex
        , &options
        , false);
}

CSyncPointer<const CGraphNodeStates> CRevisionGraphState::GetNodeStates() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const CGraphNodeStates>
        ( &mutex
        , &nodeStates
        , false);
}

CSyncPointer<CGraphNodeStates> CRevisionGraphState::GetNodeStates()
{
    CSingleLock lock (&mutex);
    return CSyncPointer<CGraphNodeStates>
        ( &mutex
        , &nodeStates
        , false);
}

CSyncPointer<const CRevisionGraphState::TVisibleGlyphs> CRevisionGraphState::GetVisibleGlyphs() const
{
    CSingleLock lock (&mutex);
    return CSyncPointer<const TVisibleGlyphs>
        ( &mutex
        , &visibleGlyphs
        , false);
}

CSyncPointer<CRevisionGraphState::TVisibleGlyphs> CRevisionGraphState::GetVisibleGlyphs()
{
    CSingleLock lock (&mutex);
    return CSyncPointer<TVisibleGlyphs>
        ( &mutex
        , &visibleGlyphs
        , false);
}

CSyncPointer<SVN> CRevisionGraphState::GetSVN()
{
    CSingleLock lock (&mutex);
    return CSyncPointer<SVN>
        ( &mutex
        , fullHistory.get() == NULL ? NULL : &fullHistory->GetSVN()
        , false);
}

// basic, synchronized data write access

void CRevisionGraphState::SetQueryResult 
    ( std::auto_ptr<CFullHistory>& newHistory
    , std::auto_ptr<CFullGraph>& newFullGraph
    , bool newFetchedWCState)
{
    CSingleLock lock (&mutex);

    // delete the old data first before, potentially partially,
    // rerouting the references to the new data

    CGraphNodeStates::TSavedData oldStates = nodeStates.SaveData();
    nodeStates.ResetFlags (UINT_MAX);
    visibleGlyphs.clear();

    layout.reset();
    visibleGraph.reset();
    fullGraph.reset();
    fullHistory.reset();

    fullHistory.reset (newHistory.release());
    fullGraph.reset (newFullGraph.release());

    fetchedWCState = newFetchedWCState;

    nodeStates.LoadData (oldStates, fullGraph.get());
}

void CRevisionGraphState::SetAnalysisResult 
    ( std::auto_ptr<CVisibleGraph>& newVisibleGraph
    , std::auto_ptr<CStandardLayout>& newLayout)
{
    CSingleLock lock (&mutex);

    visibleGlyphs.clear();

    layout.reset();
    visibleGraph.reset();

    visibleGraph.reset (newVisibleGraph.release());
    layout.reset (newLayout.release());
}

void CRevisionGraphState::SetLastErrorMessage (const CString& message)
{
    CSingleLock lock (&mutex);
    lastErrorMessage = message;
}

// convenience methods

CRect CRevisionGraphState::GetGraphRect()
{
    CSingleLock lock (&mutex);
    return layout.get() != NULL
        ? layout->GetRect()
        : CRect (0,0,0,0);
}

int CRevisionGraphState::GetNodeCount()
{
    CSingleLock lock (&mutex);

    return visibleGraph.get() != NULL
        ? static_cast<int>(visibleGraph->GetNodeCount())
        : 0;
}

svn_revnum_t CRevisionGraphState::GetHeadRevision() const
{
    CSingleLock lock (&mutex);

    return fullHistory.get() != NULL
        ? fullHistory->GetHeadRevision()
        : 0;
}

CString CRevisionGraphState::GetRepositoryRoot() const
{
    CSingleLock lock (&mutex);

    return fullHistory.get() != NULL
        ? fullHistory->GetRepositoryRoot()
        : CString();
}

CString CRevisionGraphState::GetRepositoryUUID() const
{
    CSingleLock lock (&mutex);

    return fullHistory.get() != NULL
        ? fullHistory->GetRepositoryUUID()
        : CString();
}

size_t CRevisionGraphState::GetTreeCount() const
{
    CSingleLock lock (&mutex);

    return layout.get() != NULL
        ? layout->GetTrees()->GetCount()
        : 0;
}

bool CRevisionGraphState::PromptShown() const
{
    CSingleLock lock (&mutex);

    return fullHistory.get() != NULL
        ? fullHistory->GetSVN().PromptShown()
        : false;
}

