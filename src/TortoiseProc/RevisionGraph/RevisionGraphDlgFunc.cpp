// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN

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
#include "TortoiseProc.h"
#include <gdiplus.h>
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include ".\revisiongraphwnd.h"
#include "CachedLogInfo.h"
#include "RevisionGraph/IRevisionGraphLayout.h"
#include "RevisionGraph/FullGraphBuilder.h"
#include "RevisionGraph/FullGraphFinalizer.h"
#include "RevisionGraph/VisibleGraphBuilder.h"
#include "RevisionGraph/StandardLayout.h"
#include "RevisionGraph/ShowWC.h"
#include "RevisionGraph/ShowWCModification.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

void CRevisionGraphWnd::InitView()
{
    m_bIsRubberBand = false;

    SetScrollbars();
}

void CRevisionGraphWnd::BuildPreview()
{
    m_Preview.DeleteObject();
    if (!m_bShowOverview)
        return;

    // is there a point in drawing this at all?

    int nodeCount = m_state.GetNodeCount();
    if ((nodeCount > REVGRAPH_PREVIEW_MAX_NODES) || (nodeCount == 0))
        return;

    float origZoom = m_fZoomFactor;

    CRect clientRect = GetClientRect();
    CSize preViewSize (max (REVGRAPH_PREVIEW_WIDTH, clientRect.Width() / 4)
                      ,max (REVGRAPH_PREVIEW_HEIGHT, clientRect.Height() / 4));

    // zoom the graph so that it is completely visible in the window
    CRect graphRect = GetGraphRect();
    float horzfact = float(graphRect.Width())/float(preViewSize.cx);
    float vertfact = float(graphRect.Height())/float(preViewSize.cy);
    m_previewZoom = min (DEFAULT_ZOOM, 1.0f/(max(horzfact, vertfact)));

    // make sure the preview window has a minimal size

    m_previewWidth = (int)min (max (graphRect.Width() * m_previewZoom, 30), preViewSize.cx);
    m_previewHeight = (int)min (max (graphRect.Height() * m_previewZoom, 30), preViewSize.cy);

    CClientDC ddc(this);
    CDC dc;
    if (!dc.CreateCompatibleDC(&ddc))
        return;

    m_Preview.CreateCompatibleBitmap(&ddc, m_previewWidth, m_previewHeight);
    HBITMAP oldbm = (HBITMAP)dc.SelectObject (m_Preview);

    // paint the whole graph
    DoZoom (m_previewZoom, false);
    CRect rect (0, 0, m_previewWidth, m_previewHeight);
    GraphicsDevice dev;
    dev.pDC = &dc;
    DrawGraph(dev, rect, 0, 0, true);

    // now we have a bitmap the size of the preview window
    dc.SelectObject(oldbm);
    dc.DeleteDC();

    DoZoom (origZoom, false);
}

void CRevisionGraphWnd::SetScrollbar (int bar, int newPos, int clientMax, int graphMax)
{
    SCROLLINFO ScrollInfo = {sizeof(SCROLLINFO), SIF_ALL};
    GetScrollInfo (bar, &ScrollInfo);

    clientMax = max(1, clientMax);
    int oldHeight = ScrollInfo.nMax <= 0 ? clientMax : ScrollInfo.nMax;
    int newHeight = static_cast<int>(graphMax * m_fZoomFactor);
    int maxPos = max (0, newHeight - clientMax);
    int pos = min (maxPos, newPos >= 0
                         ? newPos
                         : ScrollInfo.nPos * newHeight / oldHeight);

    ScrollInfo.nPos = pos;
    ScrollInfo.nMin = 0;
    ScrollInfo.nMax = newHeight;
    ScrollInfo.nPage = clientMax;
    ScrollInfo.nTrackPos = pos;

    SetScrollInfo(bar, &ScrollInfo);
}

void CRevisionGraphWnd::SetScrollbars (int nVert, int nHorz)
{
    CRect clientrect = GetClientRect();
    const CRect& pRect = GetGraphRect();

    SetScrollbar (SB_VERT, nVert, clientrect.Height(), pRect.Height());
    SetScrollbar (SB_HORZ, nHorz, clientrect.Width(), pRect.Width());
}

CRect CRevisionGraphWnd::GetGraphRect()
{
    return m_state.GetGraphRect();
}

CRect CRevisionGraphWnd::GetClientRect()
{
    CRect clientRect;
    CWnd::GetClientRect (&clientRect);
    return clientRect;
}

CRect CRevisionGraphWnd::GetWindowRect()
{
    CRect windowRect;
    CWnd::GetWindowRect (&windowRect);
    return windowRect;
}

CRect CRevisionGraphWnd::GetViewRect()
{
    CRect result;
    result.UnionRect (GetClientRect(), GetGraphRect());
    return result;
}

int CRevisionGraphWnd::GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;          // number of image encoders
    UINT size = 0;         // size of the image encoder array in bytes

    if (GetImageEncodersSize(&num, &size)!=Ok)
        return -1;
    if(size == 0)
        return -1;  // Failure

    ImageCodecInfo* pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
    if(pImageCodecInfo == NULL)
        return -1;  // Failure

    if (GetImageEncoders(num, size, pImageCodecInfo)==Ok)
    {
        for(UINT j = 0; j < num; ++j)
        {
            if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
            {
                *pClsid = pImageCodecInfo[j].Clsid;
                free(pImageCodecInfo);
                return j;  // Success
            }
        }

    }
    free(pImageCodecInfo);
    return -1;  // Failure
}

bool CRevisionGraphWnd::FetchRevisionData
    ( const CString& path
    , SVNRev pegRevision
    , CProgressDlg* progress
    , ITaskbarList3 * pTaskbarList
    , HWND hWnd)
{
    // (re-)fetch the data
    SVN svn;
    if (svn.GetRepositoryRoot(CTSVNPath(path)) == svn.GetURLFromPath(CTSVNPath(path)))
    {
        m_state.SetLastErrorMessage(CString(MAKEINTRESOURCE(IDS_REVGRAPH_ERR_NOGRAPHFORROOT)));
        return false;
    }

    std::unique_ptr<CFullHistory> newFullHistory (new CFullHistory());

    bool showWCRev
        = m_state.GetOptions()->GetOption<CShowWC>()->IsSelected();
    bool showWCModification
        = m_state.GetOptions()->GetOption<CShowWCModification>()->IsSelected();
    bool result = newFullHistory->FetchRevisionData ( path
                                                    , pegRevision
                                                    , showWCRev
                                                    , showWCModification
                                                    , progress
                                                    , pTaskbarList
                                                    , hWnd);

    m_state.SetLastErrorMessage (newFullHistory->GetLastErrorMessage());

    if (result)
    {
        std::unique_ptr<CFullGraph> newFullGraph (new CFullGraph());

        CFullGraphBuilder builder (*newFullHistory, *newFullGraph);
        builder.Run();

        CFullGraphFinalizer finalizer (*newFullHistory, *newFullGraph);
        finalizer.Run();

        m_state.SetQueryResult ( newFullHistory
                               , newFullGraph
                               , showWCRev || showWCModification);
    }

    return result;
}

bool CRevisionGraphWnd::AnalyzeRevisionData()
{
    CSyncPointer<const CFullGraph> fullGraph (m_state.GetFullGraph());
    if ((fullGraph.get() != NULL) && (fullGraph->GetNodeCount() > 0))
    {
        // filter graph

        CSyncPointer<CAllRevisionGraphOptions> options (m_state.GetOptions());
        options->Prepare();

        std::unique_ptr<CVisibleGraph> visibleGraph (new CVisibleGraph());
        CVisibleGraphBuilder builder ( *fullGraph
                                     , *visibleGraph
                                     , options->GetCopyFilterOptions());
        builder.Run();
        options->GetModificationOptions().Apply (visibleGraph.get());

        index_t index = 0;
        for (size_t i = 0, count = visibleGraph->GetRootCount(); i < count; ++i)
            index = visibleGraph->GetRoot (i)->InitIndex (index);

        // layout nodes

        std::unique_ptr<CStandardLayout> newLayout
            ( new CStandardLayout ( m_state.GetFullHistory()->GetCache()
                                  , visibleGraph.get()
                                  , m_state.GetFullHistory()->GetWCInfo()));
        options->GetLayoutOptions().Apply (newLayout.get());
        newLayout->Finalize();

        // switch state

        m_state.SetAnalysisResult (visibleGraph, newLayout);
    }

    return m_state.GetNodes().get() != NULL;
}

bool CRevisionGraphWnd::IsUpdateJobRunning() const
{
    return (updateJob.get() != NULL) && !updateJob->IsDone();
}

bool CRevisionGraphWnd::GetShowOverview() const
{
    return m_bShowOverview;
}

void CRevisionGraphWnd::SetShowOverview (bool value)
{
    m_bShowOverview = value;
    if (m_bShowOverview)
        BuildPreview();
}

void CRevisionGraphWnd::GetSelected
    ( const CVisibleGraphNode* node
    , bool head
    , CTSVNPath& path
    , SVNRev& rev
    , SVNRev& peg)
{
    CString repoRoot = m_state.GetRepositoryRoot();

    // get path and revision

    path.SetFromSVN (repoRoot + CUnicodeUtils::GetUnicode (node->GetPath().GetPath().c_str()));
    rev = head ? SVNRev::REV_HEAD : node->GetRevision();

    // handle 'modified WC' node

    if (node->GetClassification().Is (CNodeClassification::IS_MODIFIED_WC))
    {
        path.SetFromUnknown (m_sPath);
        rev = SVNRev::REV_WC;

        // don't set peg, if we aren't the first node
        // (i.e. would not be valid for node1)

        if (node == m_SelectedEntry1)
            peg = SVNRev::REV_WC;
    }
    else
    {
        // set head, if still necessary

        if (head && !peg.IsValid())
            peg = node->GetRevision();
    }
}

void CRevisionGraphWnd::CompareRevs(bool bHead)
{
    ASSERT(m_SelectedEntry1 != NULL);
    ASSERT(m_SelectedEntry2 != NULL);

    CSyncPointer<SVN> svn (m_state.GetSVN());

    CTSVNPath url1;
    CTSVNPath url2;
    SVNRev rev1;
    SVNRev rev2;
    SVNRev peg;

    GetSelected (m_SelectedEntry1, bHead, url1, rev1, peg);
    GetSelected (m_SelectedEntry2, bHead, url2, rev2, peg);

    bool alternativeTool = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
    if (m_state.PromptShown())
    {
        SVNDiff diff (svn.get(), this->m_hWnd);
        diff.SetAlternativeTool (alternativeTool);
        diff.ShowCompare (url1, rev1, url2, rev2, peg, L"");
    }
    else
    {
        CAppUtils::StartShowCompare (m_hWnd, url1, rev1,
            url2, rev2, peg, SVNRev(), L"", alternativeTool);
    }
}

void CRevisionGraphWnd::UnifiedDiffRevs(bool bHead)
{
    ASSERT(m_SelectedEntry1 != NULL);
    ASSERT(m_SelectedEntry2 != NULL);

    CSyncPointer<SVN> svn (m_state.GetSVN());

    CTSVNPath url1;
    CTSVNPath url2;
    SVNRev rev1;
    SVNRev rev2;
    SVNRev peg;

    GetSelected (m_SelectedEntry1, bHead, url1, rev1, peg);
    GetSelected (m_SelectedEntry2, bHead, url2, rev2, peg);

    bool alternativeTool = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
    if (m_state.PromptShown())
    {
        SVNDiff diff (svn.get(), this->m_hWnd);
        diff.SetAlternativeTool (alternativeTool);
        diff.ShowUnifiedDiff (url1, rev1, url2, rev2, peg, L"");
    }
    else
    {
        CAppUtils::StartShowUnifiedDiff(m_hWnd, url1, rev1,
            url2, rev2, peg,
            SVNRev(), L"", alternativeTool);
    }
}

void CRevisionGraphWnd::DoZoom (float fZoomFactor, bool updateScrollbars)
{
    float oldzoom = m_fZoomFactor;
    m_fZoomFactor = fZoomFactor;

    m_nFontSize = max(1, int(DEFAULT_ZOOM_FONT * fZoomFactor));
    if (m_nFontSize < SMALL_ZOOM_FONT_THRESHOLD)
        m_nFontSize = min (SMALL_ZOOM_FONT_THRESHOLD, int(SMALL_ZOOM_FONT * fZoomFactor));

    for (int i=0; i<MAXFONTS; i++)
    {
        if (m_apFonts[i] != NULL)
        {
            m_apFonts[i]->DeleteObject();
            delete m_apFonts[i];
        }
        m_apFonts[i] = NULL;
    }

    if (updateScrollbars)
    {
        SCROLLINFO si1 = {sizeof(SCROLLINFO), SIF_ALL};
        GetScrollInfo(SB_VERT, &si1);
        SCROLLINFO si2 = {sizeof(SCROLLINFO), SIF_ALL};
        GetScrollInfo(SB_HORZ, &si2);

        InitView();

        si1.nPos = int(float(si1.nPos)*m_fZoomFactor/oldzoom);
        si2.nPos = int(float(si2.nPos)*m_fZoomFactor/oldzoom);
        SetScrollPos (SB_VERT, si1.nPos);
        SetScrollPos (SB_HORZ, si2.nPos);
    }

    Invalidate (FALSE);
}
