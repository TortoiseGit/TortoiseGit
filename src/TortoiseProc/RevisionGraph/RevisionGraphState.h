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
#pragma once

#include <gdiplus.h>

#include "RevisionGraph/FullHistory.h"
#include "RevisionGraph/FullGraph.h"
#include "RevisionGraph/VisibleGraph.h"
#include "RevisionGraph/StandardLayout.h"
#include "RevisionGraph/GraphNodeState.h"
#include "RevisionGraph/AllGraphOptions.h"

// simplify access to GDI+

using namespace Gdiplus;

/**
 * An unique_ptr-like template that keeps a lock
 * alongside with the pointer.
 */

template<class T>
class CSyncPointer
{
public:

    /// RAII

    CSyncPointer (CSyncObject* mutex, T* ptr, bool ownsPtr);
    CSyncPointer (const CSyncPointer<T>& rhs);
    ~CSyncPointer();

    CSyncPointer<T>& operator= (const CSyncPointer<T>& rhs);

    /// data access

    T* operator->() const throw();
    T& operator*() const throw();
    bool operator!() const throw();

    T* get() const throw();

private:

    /// the synchronization objects

    CSyncObject* mutex;
    CSingleLock lock;

    /// the actual data

    T* ptr;

    /// if true, we must delete \ref ptr

    mutable bool ownsPtr;
};


// RAII

template <class T>
CSyncPointer<T>::CSyncPointer (CSyncObject* mutex, T* ptr, bool ownsPtr)
    : mutex (mutex)
    , lock (mutex)
    , ptr (ptr)
    , ownsPtr (ownsPtr)
{
}

template <class T>
CSyncPointer<T>::CSyncPointer (const CSyncPointer<T>& rhs)
    : mutex (rhs.mutex)
    , lock (rhs.mutex)
    , rhs.ptr (rhs.ptr)
    , ownsPtr (rhs.ownsPtr)
{
    rhs.ownsPtr = false;
}

template <class T>
CSyncPointer<T>::~CSyncPointer()
{
    if (ownsPtr)
        delete ptr;
}

template <class T>
CSyncPointer<T>& CSyncPointer<T>::operator= (const CSyncPointer<T>& rhs)
{
    if (this != *rhs)
    {
        CSyncObject* oldMutex = mutex;
        mutex = rhs.mutex;
        mutex->Lock();

        if (ownsPtr)
            delete ptr;

        ptr = rhs.ptr;
        ownsPtr = rhs.ownsPtr;
        rhs.ownsPtr = false;

        lock = rhs.lock;

        oldMutex->Unlock();
    }

    return *this;
}

// data access

template <class T>
T* CSyncPointer<T>::operator->() const throw()
{
    return ptr;
}

template <class T>
T& CSyncPointer<T>::operator*() const throw()
{
    return *ptr;
}

template <class T>
bool CSyncPointer<T>::operator!() const throw()
{
    return ptr == NULL;
}

template <class T>
T* CSyncPointer<T>::get() const throw()
{
    return ptr;
}

/**
 * \ingroup TortoiseProc
 * Window class showing a revision graph.
 *
 * The analyzation of the log data is done in the child class CRevisionGraph.
 * Here, we handle the window notifications.
 */
class CRevisionGraphState
{
public:

    /// when glyphs are shown, this will contain the list of all

    struct SVisibleGlyph
    {
        DWORD state;
        PointF leftTop;
        const CVisibleGraphNode* node;

        SVisibleGlyph (DWORD state, const PointF& leftTop, const CVisibleGraphNode* node)
            : state (state), leftTop (leftTop), node (node)
        {
        }
    };

    typedef std::vector<SVisibleGlyph> TVisibleGlyphs;

    /// construction / destruction

    CRevisionGraphState();
    ~CRevisionGraphState();

    /// basic, synchronized data read access

    CSyncPointer<const CAllRevisionGraphOptions> GetOptions() const;
    CSyncPointer<const CFullHistory>             GetFullHistory() const;
    CSyncPointer<const CFullGraph>               GetFullGraph() const;
    CSyncPointer<const CVisibleGraph>            GetVisibleGraph() const;
    CSyncPointer<const ILayoutNodeList>          GetNodes() const;
    CSyncPointer<const ILayoutConnectionList>    GetConnections() const;
    CSyncPointer<const ILayoutTextList>          GetTexts() const;
    CSyncPointer<const ILayoutRectList>          GetTrees() const;
    CSyncPointer<const TVisibleGlyphs>           GetVisibleGlyphs() const;
    CSyncPointer<const CGraphNodeStates>         GetNodeStates() const;

    bool    GetFetchedWCState() const;
    CString GetLastErrorMessage() const;

    /// sync'ed read access but will return writable references

    CSyncPointer<CAllRevisionGraphOptions> GetOptions();
    CSyncPointer<CGraphNodeStates>         GetNodeStates();
    CSyncPointer<TVisibleGlyphs>           GetVisibleGlyphs();
    CSyncPointer<SVN>                      GetSVN();

    /// basic, synchronized data write access

    void SetQueryResult ( std::unique_ptr<CFullHistory>& newHistory
                        , std::unique_ptr<CFullGraph>& newFullGraph
                        , bool newFetchedWCState);
    void SetAnalysisResult ( std::unique_ptr<CVisibleGraph>& newVisibleGraph
                           , std::unique_ptr<CStandardLayout>& newLayout);

    void SetLastErrorMessage (const CString& message);

    /// convenience methods

    CRect        GetGraphRect();
    int          GetNodeCount();

    svn_revnum_t GetHeadRevision() const;
    CString      GetRepositoryRoot() const;
    CString      GetRepositoryUUID() const;
    size_t       GetTreeCount() const;

    bool         PromptShown() const;

private:

    /// sync object

    mutable CCriticalSection mutex;

    /// status info

    bool fetchedWCState;
    CString lastErrorMessage;

    /// revision graph option setting

    CAllRevisionGraphOptions options;

    /// internal revision graph data

    std::unique_ptr<CFullHistory> fullHistory;
    std::unique_ptr<CFullGraph> fullGraph;
    std::unique_ptr<CVisibleGraph> visibleGraph;
    std::unique_ptr<IRevisionGraphLayout> layout;

    /// revision graph UI data

    CGraphNodeStates nodeStates;
    TVisibleGlyphs visibleGlyphs;

};
