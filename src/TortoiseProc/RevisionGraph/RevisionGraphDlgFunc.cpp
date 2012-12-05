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
#include "Git.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TGitPath.h"
//#include "SVNInfo.h"
//#include ".\revisiongraphwnd.h"
//#include "CachedLogInfo.h"
//#include "RevisionGraph/IRevisionGraphLayout.h"
//#include "RevisionGraph/FullGraphBuilder.h"
//#include "RevisionGraph/FullGraphFinalizer.h"
//#include "RevisionGraph/VisibleGraphBuilder.h"
//#include "RevisionGraph/StandardLayout.h"
//#include "RevisionGraph/ShowWC.h"
//#include "RevisionGraph/ShowWCModification.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;
using namespace ogdf;

void CRevisionGraphWnd::InitView()
{
    m_bIsRubberBand = false;

    SetScrollbars();
}

void CRevisionGraphWnd::BuildPreview()
{
#if 0
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
#endif
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
	return m_GraphRect;
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
    , GitRev pegRevision
    , CProgressDlg* progress
    , ITaskbarList3 * pTaskbarList
    , HWND hWnd)
{

	this->m_logEntries.ParserFromLog(NULL,-1,CGit::LOG_INFO_SIMPILFY_BY_DECORATION|CGit::LOG_INFO_ALL_BRANCH);
	
	ReloadHashMap();
	this->m_Graph.clear();

	CArray<node> nodes;
	GraphicsDevice dev;
	dev.pDC = this->GetDC();
	dev.graphics = Graphics::FromHDC(dev.pDC->m_hDC);
	dev.graphics->SetPageUnit (UnitPixel);

	for(int i=0;i<m_logEntries.size();i++)
	{
		node nd;
		nd = this->m_Graph.newNode();
		nodes.Add(nd);
		m_GraphAttr.width(nd)=100;
		m_GraphAttr.height(nd)=20;
		SetNodeRect(dev, &nd, m_logEntries[i], 0);
	}

	for(int i=0; i<m_logEntries.size();i++)
	{
		GitRev rev=m_logEntries.GetGitRevAt(i);
		for(int j=0; j<rev.m_ParentHash.size();j++)
		{
			TRACE(_T("edge %d - %d\n"),i, m_logEntries.m_HashMap[rev.m_ParentHash[j]]);
			m_Graph.newEdge(nodes[i], nodes[m_logEntries.m_HashMap[rev.m_ParentHash[j]]]);
		}
	}

	//this->m_OHL.layerDistance(30.0);
    //this->m_OHL.nodeDistance(25.0);
    //this->m_OHL.weightBalancing(0.8);
   
	m_SugiyamLayout.call(m_GraphAttr);

	node v;
	double xmax = 0;
	double ymax = 0;
	forall_nodes(v,m_Graph)
	{
		double x = m_GraphAttr.x(v) + m_GraphAttr.width(v)/2;
		double y = m_GraphAttr.y(v) + m_GraphAttr.height(v)/2;
		if(x>xmax)
			xmax = x;
		if(y>ymax)
			ymax = y;
	}
	
	this->m_GraphRect.top=m_GraphRect.left=0;
	m_GraphRect.bottom = ymax;
	m_GraphRect.right = xmax;

	return true;
}

bool CRevisionGraphWnd::AnalyzeRevisionData()
{
#if 0
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
#endif
	return true;
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
    , CTGitPath& path
    , GitRev& rev
    , GitRev& peg)
{
#if 0
    CString repoRoot = m_state.GetRepositoryRoot();

    // get path and revision

    path.SetFromSVN (repoRoot + CUnicodeUtils::GetUnicode (node->GetPath().GetPath().c_str()));
    rev = head ? GitRev::REV_HEAD : node->GetRevision();

    // handle 'modified WC' node

    if (node->GetClassification().Is (CNodeClassification::IS_MODIFIED_WC))
    {
        path.SetFromUnknown (m_sPath);
        rev = GitRev::REV_WC;

        // don't set peg, if we aren't the first node
        // (i.e. would not be valid for node1)

        if (node == m_SelectedEntry1)
            peg = GitRev::REV_WC;
    }
    else
    {
        // set head, if still necessary

        if (head && !peg.IsValid())
            peg = node->GetRevision();
    }
#endif
}

void CRevisionGraphWnd::CompareRevs(bool bHead)
{
    ASSERT(m_SelectedEntry1 != NULL);
    ASSERT(m_SelectedEntry2 != NULL);
#if 0
    CSyncPointer<SVN> svn (m_state.GetSVN());

    CTGitPath url1;
    CTGitPath url2;
    GitRev rev1;
    GitRev rev2;
    GitRev peg;

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
            url2, rev2, peg, GitRev(), L"", alternativeTool);
    }
#endif
}

void CRevisionGraphWnd::UnifiedDiffRevs(bool bHead)
{
    ASSERT(m_SelectedEntry1 != NULL);
    ASSERT(m_SelectedEntry2 != NULL);
#if 0
    CSyncPointer<SVN> svn (m_state.GetSVN());

    CTGitPath url1;
    CTGitPath url2;
    GitRev rev1;
    GitRev rev2;
    GitRev peg;

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
            GitRev(), L"", alternativeTool);
    }
#endif
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
