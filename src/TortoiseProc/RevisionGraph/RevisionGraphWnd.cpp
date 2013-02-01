// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN
// Copyright (C) 2012-2013 - TortoiseGit

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
#include "Revisiongraphwnd.h"
#include "MessageBox.h"
#include "Git.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TGitPath.h"
//#include "SVNInfo.h"
//#include "SVNDiff.h"
#include "RevisionGraphDlg.h"
//#include "CachedLogInfo.h"
//#include "RevisionIndex.h"
//#include "RepositoryInfo.h"
#include "BrowseFolder.h"
#include "GitProgressDlg.h"
#include "ChangedDlg.h"
//#include "RevisionGraph/StandardLayout.h"
//#include "RevisionGraph/UpsideDownLayout.h"
#include "SysInfo.h"
#include "FormatMessageWrapper.h"

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced formal parameter
#include <ogdf/planarity/PlanarizationLayout.h>
#include <ogdf/planarity/VariableEmbeddingInserter.h>
#include <ogdf/planarity/FastPlanarSubgraph.h>
#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/planarity/EmbedderMinDepthMaxFaceLayers.h>
#pragma warning(pop)

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;
using namespace ogdf;

#if (_WIN32_WINNT < 0x0600)
#define WM_MOUSEHWHEEL				  0x020E
#endif


enum RevisionGraphContextMenuCommands
{
	// needs to start with 1, since 0 is the return value if *nothing* is clicked on in the context menu
	GROUP_MASK = 0xff00,
	ID_SHOWLOG = 1,
	ID_CFM = 2,
	ID_BROWSEREPO,
	ID_COMPAREREVS = 0x100,
	ID_COMPAREHEADS,
	ID_UNIDIFFREVS,
	ID_UNIDIFFHEADS,
	ID_MERGETO = 0x300,
	ID_UPDATE,
	ID_SWITCHTOHEAD,
	ID_SWITCH,
	ID_COPYURL = 0x400,
	ID_EXPAND_ALL = 0x500,
	ID_JOIN_ALL,
	ID_GRAPH_EXPANDCOLLAPSE_ABOVE = 0x600,
	ID_GRAPH_EXPANDCOLLAPSE_RIGHT,
	ID_GRAPH_EXPANDCOLLAPSE_BELOW,
	ID_GRAPH_SPLITJOIN_ABOVE,
	ID_GRAPH_SPLITJOIN_RIGHT,
	ID_GRAPH_SPLITJOIN_BELOW,
};

CRevisionGraphWnd::CRevisionGraphWnd()
	: CWnd()
	, m_SelectedEntry1(NULL)
	, m_SelectedEntry2(NULL)
	, m_pDlgTip(NULL)
	, m_nFontSize(12)
	, m_bTweakTrunkColors(true)
	, m_bTweakTagsColors(true)
	, m_fZoomFactor(DEFAULT_ZOOM)
	, m_ptRubberEnd(0,0)
	, m_ptRubberStart(0,0)
	, m_bShowOverview(false)
	, m_parent (NULL)
	, m_hoverIndex (NULL)
	, m_hoverGlyphs (0)
//	, m_tooltipIndex ((index_t)NO_INDEX)
	, m_showHoverGlyphs (false)
	, m_bIsRubberBand(false)
	, m_previewWidth(0)
	, m_previewHeight(0)
	, m_previewZoom(1)
	, m_dwTicks(0)
	, m_logEntries(&m_LogCache)
	, m_bCurrentBranch(false)
{
	memset(&m_lfBaseFont, 0, sizeof(LOGFONT));
	std::fill_n(m_apFonts, MAXFONTS, (CFont*)NULL);

	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
#define REVGRAPH_CLASSNAME _T("Revgraph_windowclass")
	if (!(::GetClassInfo(hInst, REVGRAPH_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style			= CS_DBLCLKS | CS_OWNDC;
		wndcls.lpfnWndProc	  = ::DefWindowProc;
		wndcls.cbClsExtra	   = wndcls.cbWndExtra = 0;
		wndcls.hInstance		= hInst;
		wndcls.hIcon			= NULL;
		wndcls.hCursor		  = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
		wndcls.lpszMenuName	 = NULL;
		wndcls.lpszClassName	= REVGRAPH_CLASSNAME;

		RegisterClass(&wndcls);
	}

	m_bTweakTrunkColors = CRegDWORD(_T("Software\\TortoiseGit\\RevisionGraph\\TweakTrunkColors"), TRUE) != FALSE;
	m_bTweakTagsColors = CRegDWORD(_T("Software\\TortoiseGit\\RevisionGraph\\TweakTagsColors"), TRUE) != FALSE;
	m_szTip[0]  = 0;
	m_wszTip[0] = 0;

	m_GraphAttr.init(this->m_Graph, ogdf::GraphAttributes::nodeGraphics | ogdf::GraphAttributes::edgeGraphics |
		ogdf:: GraphAttributes::nodeLabel | ogdf::GraphAttributes::nodeColor |
		ogdf::GraphAttributes::edgeColor | ogdf::GraphAttributes::edgeStyle |
		ogdf::GraphAttributes::nodeStyle | ogdf::GraphAttributes::nodeTemplate);

	m_SugiyamLayout.setRanking(::new ogdf::OptimalRanking());
	m_SugiyamLayout.setCrossMin(::new ogdf::MedianHeuristic());

	double pi = 3.1415926;
	m_ArrowCos = cos(pi/8);
	m_ArrowSin = sin(pi/8);
	this->m_ArrowSize = 8;
#if 0
	ogdf::node one = this->m_Graph.newNode();
	ogdf::node two = this->m_Graph.newNode();
	ogdf::node three = this->m_Graph.newNode();
	ogdf::node four = this->m_Graph.newNode();


	m_GraphAttr.width(one)=100;
	m_GraphAttr.height(one)=200;
	m_GraphAttr.width(two)=100;
	m_GraphAttr.height(two)=100;
	m_GraphAttr.width(three)=100;
	m_GraphAttr.height(three)=20;
	m_GraphAttr.width(four)=100;
	m_GraphAttr.height(four)=20;

	m_GraphAttr.labelNode(one)="One";
	m_GraphAttr.labelNode(two)="Two";
	m_GraphAttr.labelNode(three)="three";

	this->m_Graph.newEdge(one, two);
	this->m_Graph.newEdge(one, three);
	this->m_Graph.newEdge(two, four);
	this->m_Graph.newEdge(three, four);

#endif
	FastHierarchyLayout *pOHL = ::new FastHierarchyLayout;
	//It will auto delte when m_SugiyamLayout destory

	pOHL->layerDistance(30.0);
	pOHL->nodeDistance(25.0);

	m_SugiyamLayout.setLayout(pOHL);

#if 0
	//this->m_OHL.layerDistance(30.0);
	//this->m_OHL.nodeDistance(25.0);
	//this->m_OHL.weightBalancing(0.8);
	m_SugiyamLayout.setLayout(&m_OHL);
	m_SugiyamLayout.call(m_GraphAttr);
#endif
#if 0
	PlanarizationLayout pl;

	FastPlanarSubgraph *ps = ::new FastPlanarSubgraph;
	ps->runs(100);
	VariableEmbeddingInserter *ves = ::new VariableEmbeddingInserter;
	ves->removeReinsert(EdgeInsertionModule::rrAll);
	pl.setSubgraph(ps);
	pl.setInserter(ves);

	EmbedderMinDepthMaxFaceLayers *emb = ::new EmbedderMinDepthMaxFaceLayers;
	pl.setEmbedder(emb);

	OrthoLayout *ol =::new OrthoLayout;
	ol->separation(20.0);
	ol->cOverhang(0.4);
	ol->setOptions(2+4);
	ol->preferedDir(OrthoDir::odEast);
	pl.setPlanarLayouter(ol);

	pl.call(m_GraphAttr);

	node v;
	forall_nodes(v,m_Graph) {

		TRACE(_T("node  x %f y %f %f %f\n"),/* m_GraphAttr.idNode(v), */
			m_GraphAttr.x(v),
			m_GraphAttr.y(v),
			m_GraphAttr.width(v),
			m_GraphAttr.height(v)
		);
	}

	edge e;
	forall_edges(e, m_Graph)
	{
		// get connection and point position
		const DPolyline &dpl = this->m_GraphAttr.bends(e);

		ListConstIterator<DPoint> it;
		for(it = dpl.begin(); it.valid(); ++it)
		{
			TRACE(_T("edge %f %f\n"), (*it).m_x, (*it).m_y);
		}
	}
	m_GraphAttr.writeGML("test.gml");
#endif
}

CRevisionGraphWnd::~CRevisionGraphWnd()
{
	for (int i=0; i<MAXFONTS; i++)
	{
		if (m_apFonts[i] != NULL)
		{
			m_apFonts[i]->DeleteObject();
			delete m_apFonts[i];
		}
		m_apFonts[i] = NULL;
	}
	delete m_pDlgTip;
	m_Graph.clear();
}

void CRevisionGraphWnd::DoDataExchange(CDataExchange* pDX)
{
	CWnd::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CRevisionGraphWnd, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipNotify)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipNotify)
	ON_WM_MOUSEWHEEL()
	ON_WM_MOUSEHWHEEL()
	ON_WM_CONTEXTMENU()
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONUP()
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
	ON_MESSAGE(WM_WORKERTHREADDONE,OnWorkerThreadDone)
	ON_WM_CAPTURECHANGED()
END_MESSAGE_MAP()

void CRevisionGraphWnd::Init(CWnd * pParent, LPRECT rect)
{
	WNDCLASS wndcls;
	HINSTANCE hInst = AfxGetInstanceHandle();
#define REVGRAPH_CLASSNAME _T("Revgraph_windowclass")
	if (!(::GetClassInfo(hInst, REVGRAPH_CLASSNAME, &wndcls)))
	{
		// otherwise we need to register a new class
		wndcls.style			= CS_DBLCLKS | CS_OWNDC;
		wndcls.lpfnWndProc	  = ::DefWindowProc;
		wndcls.cbClsExtra	   = wndcls.cbWndExtra = 0;
		wndcls.hInstance		= hInst;
		wndcls.hIcon			= NULL;
		wndcls.hCursor		  = AfxGetApp()->LoadStandardCursor(IDC_ARROW);
		wndcls.hbrBackground	= (HBRUSH) (COLOR_WINDOW + 1);
		wndcls.lpszMenuName	 = NULL;
		wndcls.lpszClassName	= REVGRAPH_CLASSNAME;

		RegisterClass(&wndcls);
	}

	if (!IsWindow(m_hWnd))
		CreateEx(WS_EX_CLIENTEDGE, REVGRAPH_CLASSNAME, _T("RevGraph"), WS_CHILD|WS_VISIBLE|WS_TABSTOP, *rect, pParent, 0);
	m_pDlgTip = new CToolTipCtrl;
	if(!m_pDlgTip->Create(this))
	{
//		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": Unable to add tooltip!\n");
	}
	EnableToolTips();

	memset(&m_lfBaseFont, 0, sizeof(m_lfBaseFont));
	m_lfBaseFont.lfHeight = 0;
	m_lfBaseFont.lfWeight = FW_NORMAL;
	m_lfBaseFont.lfItalic = FALSE;
	m_lfBaseFont.lfCharSet = DEFAULT_CHARSET;
	m_lfBaseFont.lfOutPrecision = OUT_DEFAULT_PRECIS;
	m_lfBaseFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	m_lfBaseFont.lfQuality = DEFAULT_QUALITY;
	m_lfBaseFont.lfPitchAndFamily = DEFAULT_PITCH;

	m_dwTicks = GetTickCount();

	m_parent = dynamic_cast<CRevisionGraphDlg*>(pParent);
}

CPoint CRevisionGraphWnd::GetLogCoordinates (CPoint point) const
{
	// translate point into logical coordinates

	int nVScrollPos = GetScrollPos(SB_VERT);
	int nHScrollPos = GetScrollPos(SB_HORZ);

	return CPoint ( (int)((point.x + nHScrollPos) / m_fZoomFactor)
				  , (int)((point.y + nVScrollPos) / m_fZoomFactor));
}

node CRevisionGraphWnd::GetHitNode (CPoint point, CSize /*border*/) const
{
#if 0
	// any nodes at all?

	CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
	if (!nodeList)
		return index_t(NO_INDEX);

	// search the nodes for one at that grid position

	return nodeList->GetAt (GetLogCoordinates (point), border);
#endif

	node v;
	forall_nodes(v,m_Graph)
	{
		 RectF noderect (GetNodeRect (v, CPoint(GetScrollPos(SB_HORZ),  GetScrollPos(SB_VERT))));
		 if(point.x>noderect.X && point.x <(noderect.X+noderect.Width) &&
			 point.y>noderect.Y && point.y <(noderect.Y+noderect.Height))
		 {
			 return v;
		 }
	}
	return NULL;
}

DWORD CRevisionGraphWnd::GetHoverGlyphs (CPoint /*point*/) const
{
	// if there is no layout, there will be no nodes,
	// hence, no glyphs
	DWORD result = 0;
#if 0
	CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
	if (!nodeList)
		return 0;

	// get node at point or node that is close enough
	// so that point may hit a glyph area

	index_t nodeIndex = GetHitNode(point);
	if (nodeIndex == NO_INDEX)
		nodeIndex = GetHitNode(point, CSize (GLYPH_SIZE, GLYPH_SIZE / 2));

	if (nodeIndex >= nodeList->GetCount())
		return 0;

	ILayoutNodeList::SNode node = nodeList->GetNode (nodeIndex);
	const CVisibleGraphNode* base = node.node;

	// what glyphs should be shown depending on position of point
	// relative to the node rect?

	CPoint logCoordinates = GetLogCoordinates (point);
	CRect r = node.rect;
	CPoint center = r.CenterPoint();

	CRect rightGlyphArea ( r.right - GLYPH_SIZE, center.y - GLYPH_SIZE / 2
						 , r.right + GLYPH_SIZE, center.y + GLYPH_SIZE / 2);
	CRect topGlyphArea ( center.x - GLYPH_SIZE, r.top - GLYPH_SIZE / 2
					   , center.x + GLYPH_SIZE, r.top + GLYPH_SIZE / 2);
	CRect bottomGlyphArea ( center.x - GLYPH_SIZE, r.bottom - GLYPH_SIZE / 2
						  , center.x + GLYPH_SIZE, r.bottom + GLYPH_SIZE / 2);

	bool upsideDown
		= m_state.GetOptions()->GetOption<CUpsideDownLayout>()->IsActive();

	if (upsideDown)
	{
		std::swap (topGlyphArea.top, bottomGlyphArea.top);
		std::swap (topGlyphArea.bottom, bottomGlyphArea.bottom);
	}


	if (rightGlyphArea.PtInRect (logCoordinates))
		result = base->GetFirstCopyTarget() != NULL
			   ? CGraphNodeStates::COLLAPSED_RIGHT | CGraphNodeStates::SPLIT_RIGHT
			   : 0;

	if (topGlyphArea.PtInRect (logCoordinates))
		result = base->GetSource() != NULL
			   ? CGraphNodeStates::COLLAPSED_ABOVE | CGraphNodeStates::SPLIT_ABOVE
			   : 0;

	if (bottomGlyphArea.PtInRect (logCoordinates))
		result = base->GetNext() != NULL
			   ? CGraphNodeStates::COLLAPSED_BELOW | CGraphNodeStates::SPLIT_BELOW
			   : 0;

	// if some nodes have already been split, don't allow collapsing etc.

	CSyncPointer<const CGraphNodeStates> nodeStates (m_state.GetNodeStates());
	if (result & nodeStates->GetFlags (base))
		result = 0;
#endif
	return result;
}
#if 0
const CRevisionGraphState::SVisibleGlyph* CRevisionGraphWnd::GetHitGlyph (CPoint point) const
{
	float glyphSize = GLYPH_SIZE * m_fZoomFactor;

	CSyncPointer<const CRevisionGraphState::TVisibleGlyphs>
		visibleGlyphs (m_state.GetVisibleGlyphs());

	for (size_t i = 0, count = visibleGlyphs->size(); i < count; ++i)
	{
		const CRevisionGraphState::SVisibleGlyph* entry = &(*visibleGlyphs)[i];

		float xRel = point.x - entry->leftTop.X;
		float yRel = point.y - entry->leftTop.Y;

		if (   (xRel >= 0) && (xRel < glyphSize)
			&& (yRel >= 0) && (yRel < glyphSize))
		{
			return entry;
		}
	}

	return NULL;
}
#endif
void CRevisionGraphWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO sinfo = {0};
	sinfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_HORZ, &sinfo);

	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:	  // Scroll to far left.
		sinfo.nPos = sinfo.nMin;
		break;
	case SB_RIGHT:	  // Scroll to far right.
		sinfo.nPos = sinfo.nMax;
		break;
	case SB_ENDSCROLL:   // End scroll.
		break;
	case SB_LINELEFT:	  // Scroll left.
		if (sinfo.nPos > sinfo.nMin)
			sinfo.nPos--;
		break;
	case SB_LINERIGHT:   // Scroll right.
		if (sinfo.nPos < sinfo.nMax)
			sinfo.nPos++;
		break;
	case SB_PAGELEFT:	// Scroll one page left.
		{
			if (sinfo.nPos > sinfo.nMin)
				sinfo.nPos = max(sinfo.nMin, sinfo.nPos - (int) sinfo.nPage);
		}
		break;
	case SB_PAGERIGHT:	  // Scroll one page right.
		{
			if (sinfo.nPos < sinfo.nMax)
				sinfo.nPos = min(sinfo.nMax, sinfo.nPos + (int) sinfo.nPage);
		}
		break;
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		sinfo.nPos = sinfo.nTrackPos;	  // of the scroll box at the end of the drag operation.
		break;
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		sinfo.nPos = sinfo.nTrackPos;	 // position that the scroll box has been dragged to.
		break;
	}
	SetScrollInfo(SB_HORZ, &sinfo);
	Invalidate (FALSE);
	__super::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CRevisionGraphWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	SCROLLINFO sinfo = {0};
	sinfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_VERT, &sinfo);

	// Determine the new position of scroll box.
	switch (nSBCode)
	{
	case SB_LEFT:	  // Scroll to far left.
		sinfo.nPos = sinfo.nMin;
		break;
	case SB_RIGHT:	  // Scroll to far right.
		sinfo.nPos = sinfo.nMax;
		break;
	case SB_ENDSCROLL:   // End scroll.
		break;
	case SB_LINELEFT:	  // Scroll left.
		if (sinfo.nPos > sinfo.nMin)
			sinfo.nPos--;
		break;
	case SB_LINERIGHT:   // Scroll right.
		if (sinfo.nPos < sinfo.nMax)
			sinfo.nPos++;
		break;
	case SB_PAGELEFT:	// Scroll one page left.
		{
			if (sinfo.nPos > sinfo.nMin)
				sinfo.nPos = max(sinfo.nMin, sinfo.nPos - (int) sinfo.nPage);
		}
		break;
	case SB_PAGERIGHT:	  // Scroll one page right.
		{
			if (sinfo.nPos < sinfo.nMax)
				sinfo.nPos = min(sinfo.nMax, sinfo.nPos + (int) sinfo.nPage);
		}
		break;
	case SB_THUMBPOSITION: // Scroll to absolute position. nPos is the position
		sinfo.nPos = sinfo.nTrackPos;	  // of the scroll box at the end of the drag operation.
		break;
	case SB_THUMBTRACK:   // Drag scroll box to specified position. nPos is the
		sinfo.nPos = sinfo.nTrackPos;	 // position that the scroll box has been dragged to.
		break;
	}
	SetScrollInfo(SB_VERT, &sinfo);
	Invalidate(FALSE);
	__super::OnVScroll(nSBCode, nPos, pScrollBar);
}

void CRevisionGraphWnd::OnSize(UINT nType, int cx, int cy)
{
	__super::OnSize(nType, cx, cy);
	SetScrollbars(GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ));
	Invalidate(FALSE);
}

void CRevisionGraphWnd::OnLButtonDown(UINT nFlags, CPoint point)
{

	if (IsUpdateJobRunning())
		return __super::OnLButtonDown(nFlags, point);

//	CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());

	SetFocus();
	bool bHit = false;
	bool bControl = !!(GetKeyState(VK_CONTROL)&0x8000);
	bool bOverview = m_bShowOverview && m_OverviewRect.PtInRect(point);
	if (! bOverview)
	{
#if 0
		const CRevisionGraphState::SVisibleGlyph* hitGlyph
			= GetHitGlyph (point);

		if (hitGlyph != NULL)
		{
			ToggleNodeFlag (hitGlyph->node, hitGlyph->state);
			return __super::OnLButtonDown(nFlags, point);
		}
#endif
		node nodeIndex = GetHitNode (point);
		if (nodeIndex != NULL)
		{
			if (bControl)
			{
				if (m_SelectedEntry1 == nodeIndex)
				{
					if (m_SelectedEntry2)
					{
						m_SelectedEntry1 = m_SelectedEntry2;
						m_SelectedEntry2 = NULL;
					}
					else
						m_SelectedEntry1 = NULL;
				}
				else if (m_SelectedEntry2 == nodeIndex)
					m_SelectedEntry2 = NULL;
				else if (m_SelectedEntry1)
					m_SelectedEntry2 = nodeIndex;
				else
					m_SelectedEntry1 = nodeIndex;
			}
			else
			{
				if (m_SelectedEntry1 == nodeIndex)
					m_SelectedEntry1 = NULL;
				else
					m_SelectedEntry1 = nodeIndex;
				m_SelectedEntry2 = NULL;
			}
			bHit = true;
			Invalidate(FALSE);
		}
	}

	if ((!bHit)&&(!bControl)&&(!bOverview))
	{
		m_SelectedEntry1 = NULL;
		m_SelectedEntry2 = NULL;
		m_bIsRubberBand = true;
		Invalidate(FALSE);
		if (m_bShowOverview && m_OverviewRect.PtInRect(point))
			m_bIsRubberBand = false;
	}
	m_ptRubberStart = point;

	UINT uEnable = MF_BYCOMMAND;
	if ((m_SelectedEntry1 != NULL)&&(m_SelectedEntry2 != NULL))
		uEnable |= MF_ENABLED;
	else
		uEnable |= MF_GRAYED;

	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_COMPAREREVISIONS, uEnable);
	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_UNIFIEDDIFF, uEnable);

	uEnable = MF_BYCOMMAND;
	if ((m_SelectedEntry1 != NULL)&&(m_SelectedEntry2 == NULL))
		uEnable |= MF_ENABLED;
	else
		uEnable |= MF_GRAYED;

	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_UNIFIEDDIFFOFHEADREVISIONS, uEnable);
	EnableMenuItem(GetParent()->GetMenu()->m_hMenu, ID_VIEW_COMPAREHEADREVISIONS, uEnable);

	__super::OnLButtonDown(nFlags, point);
}

void CRevisionGraphWnd::OnCaptureChanged(CWnd *pWnd)
{
	m_bIsRubberBand = false;
	m_ptRubberEnd = CPoint(0,0);

	__super::OnCaptureChanged(pWnd);
}

void CRevisionGraphWnd::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (!m_bIsRubberBand)
		return;	 // we don't have a rubberband, so no zooming necessary

	m_bIsRubberBand = false;
	ReleaseCapture();
	if (IsUpdateJobRunning())
		return __super::OnLButtonUp(nFlags, point);

	// zooming is finished
	m_ptRubberEnd = CPoint(0,0);
	CRect rect = GetClientRect();
	int x = abs(m_ptRubberStart.x - point.x);
	int y = abs(m_ptRubberStart.y - point.y);

	if ((x < 20)&&(y < 20))
	{
		// too small zoom rectangle
		// assume zooming by accident
		Invalidate(FALSE);
		__super::OnLButtonUp(nFlags, point);
		return;
	}

	float xfact = float(rect.Width())/float(x);
	float yfact = float(rect.Height())/float(y);
	float fact = max(yfact, xfact);

	// find out where to scroll to
	x = min(m_ptRubberStart.x, point.x) + GetScrollPos(SB_HORZ);
	y = min(m_ptRubberStart.y, point.y) + GetScrollPos(SB_VERT);

	float fZoomfactor = m_fZoomFactor*fact;
	if (fZoomfactor > 10 * MAX_ZOOM)
	{
		// with such a big zoomfactor, the user
		// most likely zoomed by accident
		Invalidate(FALSE);
		__super::OnLButtonUp(nFlags, point);
		return;
	}
	if (fZoomfactor > MAX_ZOOM)
	{
		fZoomfactor = MAX_ZOOM;
		fact = fZoomfactor/m_fZoomFactor;
	}

	CRevisionGraphDlg * pDlg = (CRevisionGraphDlg*)GetParent();
	if (pDlg)
	{
		m_fZoomFactor = fZoomfactor;
		pDlg->DoZoom (m_fZoomFactor);
		SetScrollbars(int(float(y)*fact), int(float(x)*fact));
	}
	__super::OnLButtonUp(nFlags, point);
}

bool CRevisionGraphWnd::CancelMouseZoom()
{
	bool bRet = m_bIsRubberBand;
	ReleaseCapture();
	if (m_bIsRubberBand)
		Invalidate(FALSE);
	m_bIsRubberBand = false;
	m_ptRubberEnd = CPoint(0,0);
	return bRet;
}

INT_PTR CRevisionGraphWnd::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{

	if (IsUpdateJobRunning())
		return -1;

	node nodeIndex = GetHitNode (point);
	if (m_tooltipIndex != nodeIndex)
	{
		// force tooltip to be updated

		m_tooltipIndex = nodeIndex;
		return -1;
	}

	if (nodeIndex == NULL)
		return -1;

//	if ((GetHoverGlyphs (point) != 0) || (GetHitGlyph (point) != NULL))
//		return -1;

	pTI->hwnd = this->m_hWnd;
	CWnd::GetClientRect(&pTI->rect);
	pTI->uFlags  |= TTF_ALWAYSTIP | TTF_IDISHWND;
	pTI->uId = (UINT_PTR)m_hWnd;
	pTI->lpszText = LPSTR_TEXTCALLBACK;

	return 1;
}

BOOL CRevisionGraphWnd::OnToolTipNotify(UINT /*id*/, NMHDR *pNMHDR, LRESULT *pResult)
{
	if (pNMHDR->idFrom != (UINT_PTR)m_hWnd)
		return FALSE;

	POINT point;
	DWORD ptW = GetMessagePos();
	point.x = GET_X_LPARAM(ptW);
	point.y = GET_Y_LPARAM(ptW);
	ScreenToClient(&point);

	CString strTipText = TooltipText (GetHitNode (point));

	*pResult = 0;
	if (strTipText.IsEmpty())
		return TRUE;

	CSize tooltipSize = UsableTooltipRect();
	strTipText = DisplayableText (strTipText, tooltipSize);

	// need to handle both ANSI and UNICODE versions of the message
	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, tooltipSize.cx);
		pTTTA->lpszText = m_szTip;
		WideCharToMultiByte(CP_ACP, 0, strTipText, -1, m_szTip, strTipText.GetLength()+1, 0, 0);
	}
	else
	{
		TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
		::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, tooltipSize.cx);
		lstrcpyn(m_wszTip, strTipText, strTipText.GetLength()+1);
		pTTTW->lpszText = m_wszTip;
	}

	// show the tooltip for 32 seconds. A higher value than 32767 won't work
	// even though it's nowhere documented!
	::SendMessage(pNMHDR->hwndFrom, TTM_SETDELAYTIME, TTDT_AUTOPOP, 32767);
	return TRUE;	// message was handled
}

CSize CRevisionGraphWnd::UsableTooltipRect()
{
	// get screen size

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	// get current mouse position

	CPoint cursorPos;
	if (GetCursorPos (&cursorPos) == FALSE)
	{
		// we could not determine the mouse position
		// use screen / 2 minus some safety margin

		return CSize (screenWidth / 2 - 20, screenHeight / 2 - 20);
	}

	// tool tip will display in the biggest sector beside the cursor
	// deduct some safety margin (for the mouse cursor itself

	CSize biggestSector
		( max (screenWidth - cursorPos.x - 40, cursorPos.x - 24)
		, max (screenHeight - cursorPos.y - 40, cursorPos.y - 24));

	return biggestSector;
}

CString CRevisionGraphWnd::DisplayableText ( const CString& wholeText
										   , const CSize& tooltipSize)
{
	CDC* dc = GetDC();
	if (dc == NULL)
	{
		// no access to the device context -> truncate hard at 1000 chars

		return wholeText.GetLength() >= MAX_TT_LENGTH_DEFAULT
			? wholeText.Left (MAX_TT_LENGTH_DEFAULT-4) + _T(" ...")
			: wholeText;
	}

	// select the tooltip font

	NONCLIENTMETRICS metrics;
	metrics.cbSize = sizeof (metrics);
	if (!SysInfo::Instance().IsVistaOrLater())
	{
		metrics.cbSize -= sizeof(int);  // subtract the size of the iPaddedBorderWidth member which is not available on XP
	}
	SystemParametersInfo (SPI_GETNONCLIENTMETRICS, metrics.cbSize, &metrics, 0);

	CFont font;
	font.CreateFontIndirect(&metrics.lfStatusFont);
	CFont* pOldFont = dc->SelectObject (&font);

	// split into lines and fill the tooltip rect

	CString result;

	int remainingHeight = tooltipSize.cy;
	int pos = 0;
	while (pos < wholeText.GetLength())
	{
		// extract a whole line

		int nextPos = wholeText.Find ('\n', pos);
		if (nextPos < 0)
			nextPos = wholeText.GetLength();

		CString line = wholeText.Mid (pos, nextPos-pos+1);

		// find a way to make it fit

		CSize size = dc->GetTextExtent (line);
		while (size.cx > tooltipSize.cx)
		{
			line.Delete (line.GetLength()-1);
			int nextPos2 = line.ReverseFind (' ');
			if (nextPos2 < 0)
				break;

			line.Delete (nextPos2+1, line.GetLength() - pos-1);
			size = dc->GetTextExtent (line);
		}

		// enough room for the new line?

		remainingHeight -= size.cy;
		if (remainingHeight <= size.cy)
		{
			result += _T("...");
			break;
		}

		// add the line

		result += line;
		pos += line.GetLength();
	}

	// relase temp. resources

	dc->SelectObject (pOldFont);
	ReleaseDC(dc);

	// ready

	return result;
}

CString CRevisionGraphWnd::TooltipText(node index)
{
	if(index)
	{
		CString str;
		CGitHash	hash = m_logEntries[index->index()];
		GitRev *rev = this->m_LogCache.GetCacheData(hash);
		str += rev->m_CommitHash.ToString();
		str += _T("\n");
		str += rev->GetAuthorName() +_T(" ") + rev->GetAuthorEmail();
		str += _T(" ");
		str += rev->GetAuthorDate().Format(_T("%Y-%m-%d %H:%M"));
		str += _T("\n\n")+rev->GetSubject();
		str += _T("\n");
		str += rev->GetBody();
		return str;

	}else
		return CString();
}

void CRevisionGraphWnd::SaveGraphAs(CString sSavePath)
{

	CString extension = CPathUtils::GetFileExtFromPath(sSavePath);
	if (extension.CompareNoCase(_T(".wmf"))==0)
	{
		// save the graph as an enhanced metafile
		CMetaFileDC wmfDC;
		wmfDC.CreateEnhanced(NULL, sSavePath, NULL, _T("TortoiseGit\0Revision Graph\0\0"));
		float fZoom = m_fZoomFactor;
		m_fZoomFactor = DEFAULT_ZOOM;
		DoZoom(m_fZoomFactor);
		CRect rect;
		rect = GetViewRect();
		GraphicsDevice dev;
		dev.pDC = &wmfDC;
		DrawGraph(dev, rect, 0, 0, true);
		HENHMETAFILE hemf = wmfDC.CloseEnhanced();
		DeleteEnhMetaFile(hemf);
		m_fZoomFactor = fZoom;
		DoZoom(m_fZoomFactor);
	}
	else if (extension.CompareNoCase(_T(".svg"))==0)
	{
		// save the graph as a scalable vector graphic
		SVG svg;
		float fZoom = m_fZoomFactor;
		m_fZoomFactor = DEFAULT_ZOOM;
		DoZoom(m_fZoomFactor);
		CRect rect;
		rect = GetViewRect();
		svg.SetViewSize(rect.Width(), rect.Height());
		GraphicsDevice dev;
		dev.pSVG = &svg;
		DrawGraph(dev, rect, 0, 0, true);
		svg.Save(sSavePath);
		m_fZoomFactor = fZoom;
		DoZoom(m_fZoomFactor);
	}
	else if (extension.CompareNoCase(_T(".gv")) == 0)
	{
		Graphviz graphviz;
		float fZoom = m_fZoomFactor;
		m_fZoomFactor = DEFAULT_ZOOM;
		DoZoom(m_fZoomFactor);
		CRect rect;
		rect = GetViewRect();
		GraphicsDevice dev;
		dev.pGraphviz = &graphviz;
		DrawGraph(dev, rect, 0, 0, true);
		graphviz.Save(sSavePath);
		m_fZoomFactor = fZoom;
		DoZoom(m_fZoomFactor);
	}
	else
	{
		// save the graph as a pixel picture instead of a vector picture
		// create dc to paint on
		try
		{
			CString sErrormessage;
			CWindowDC ddc(this);
			CDC dc;
			if (!dc.CreateCompatibleDC(&ddc))
			{
				CFormatMessageWrapper errorDetails;
				if( errorDetails )
					MessageBox( errorDetails, _T("Error"), MB_OK | MB_ICONINFORMATION );

				return;
			}
			CRect rect;
			rect = GetGraphRect();
			rect.bottom = (LONG)(float(rect.Height()) * m_fZoomFactor);
			rect.right = (LONG)(float(rect.Width()) * m_fZoomFactor);
			BITMAPINFO bmi;
			HBITMAP hbm;
			LPBYTE pBits;
			// Initialize header to 0s.
			SecureZeroMemory(&bmi, sizeof(bmi));
			// Fill out the fields you care about.
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmi.bmiHeader.biWidth = rect.Width();
			bmi.bmiHeader.biHeight = rect.Height();
			bmi.bmiHeader.biPlanes = 1;
			bmi.bmiHeader.biBitCount = 24;
			bmi.bmiHeader.biCompression = BI_RGB;

			// Create the surface.
			hbm = CreateDIBSection(ddc.m_hDC, &bmi, DIB_RGB_COLORS,(void **)&pBits, NULL, 0);
			if (hbm==0)
			{
				CMessageBox::Show(m_hWnd, IDS_REVGRAPH_ERR_NOMEMORY, IDS_APPNAME, MB_ICONERROR);
				return;
			}
			HBITMAP oldbm = (HBITMAP)dc.SelectObject(hbm);
			// paint the whole graph
			GraphicsDevice dev;
			dev.pDC = &dc;
			DrawGraph(dev, rect, 0, 0, true);
			// now use GDI+ to save the picture
			CLSID encoderClsid;
			{
				Bitmap bitmap(hbm, NULL);
				if (bitmap.GetLastStatus()==Ok)
				{
					// Get the CLSID of the encoder.
					int ret = 0;
					if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".png"))==0)
						ret = GetEncoderClsid(L"image/png", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".jpg"))==0)
						ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".jpeg"))==0)
						ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".bmp"))==0)
						ret = GetEncoderClsid(L"image/bmp", &encoderClsid);
					else if (CPathUtils::GetFileExtFromPath(sSavePath).CompareNoCase(_T(".gif"))==0)
						ret = GetEncoderClsid(L"image/gif", &encoderClsid);
					else
					{
						sSavePath += _T(".jpg");
						ret = GetEncoderClsid(L"image/jpeg", &encoderClsid);
					}
					if (ret >= 0)
					{
						CStringW tfile = CStringW(sSavePath);
						bitmap.Save(tfile, &encoderClsid, NULL);
					}
					else
					{
						sErrormessage.Format(IDS_REVGRAPH_ERR_NOENCODER, (LPCTSTR)CPathUtils::GetFileExtFromPath(sSavePath));
					}
				}
				else
				{
					sErrormessage.LoadString(IDS_REVGRAPH_ERR_NOBITMAP);
				}
			}
			dc.SelectObject(oldbm);
			DeleteObject(hbm);
			dc.DeleteDC();
			if (!sErrormessage.IsEmpty())
			{
				::MessageBox(m_hWnd, sErrormessage, _T("TortoiseGit"), MB_ICONERROR);
			}
		}
		catch (CException * pE)
		{
			TCHAR szErrorMsg[2048];
			pE->GetErrorMessage(szErrorMsg, 2048);
			pE->Delete();
			::MessageBox(m_hWnd, szErrorMsg, _T("TortoiseGit"), MB_ICONERROR);
		}
	}

}

BOOL CRevisionGraphWnd::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (IsUpdateJobRunning())
		return __super::OnMouseWheel(nFlags, zDelta, pt);

	if (GetKeyState(VK_CONTROL)&0x8000)
	{
		float newZoom = m_fZoomFactor * (zDelta < 0 ? ZOOM_STEP : 1.0f/ZOOM_STEP);
		DoZoom (max (MIN_ZOOM, min (MAX_ZOOM, newZoom)));
	}
	else
	{
		int orientation = GetKeyState(VK_SHIFT)&0x8000 ? SB_HORZ : SB_VERT;
		int pos = GetScrollPos(orientation);
		pos -= (zDelta);
		SetScrollPos(orientation, pos);
		Invalidate(FALSE);
	}
	return __super::OnMouseWheel(nFlags, zDelta, pt);
}

void CRevisionGraphWnd::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt)
{
	if (IsUpdateJobRunning())
		return __super::OnMouseHWheel(nFlags, zDelta, pt);

	int orientation = GetKeyState(VK_SHIFT)&0x8000 ? SB_VERT : SB_HORZ;
	int pos = GetScrollPos(orientation);
	pos -= (zDelta);
	SetScrollPos(orientation, pos);
	Invalidate(FALSE);

	return __super::OnMouseHWheel(nFlags, zDelta, pt);
}

bool CRevisionGraphWnd::UpdateSelectedEntry (node clickedentry)
{
	if ((m_SelectedEntry1 == NULL)&&(clickedentry == NULL))
		return false;

	if (m_SelectedEntry1 == NULL)
	{
		m_SelectedEntry1 = clickedentry;
		Invalidate(FALSE);
	}
	if ((m_SelectedEntry2 == NULL)&&(clickedentry != m_SelectedEntry1))
	{
		m_SelectedEntry1 = clickedentry;
		Invalidate(FALSE);
	}
	if (m_SelectedEntry1 && m_SelectedEntry2)
	{
		if ((m_SelectedEntry2 != clickedentry)&&(m_SelectedEntry1 != clickedentry))
			return false;
	}
	if (m_SelectedEntry1 == NULL)
		return false;

	return true;
}

void CRevisionGraphWnd::AppendMenu
	( CMenu& popup
	, UINT title
	, UINT command
	, UINT flags)
{
	// separate different groups / section within the context menu

	if (popup.GetMenuItemCount() > 0)
	{
		UINT lastCommand = popup.GetMenuItemID (popup.GetMenuItemCount()-1);
		if ((lastCommand & GROUP_MASK) != (command & GROUP_MASK))
			popup.AppendMenu(MF_SEPARATOR, NULL);
	}

	// actually add the new item

	CString titleString;
	titleString.LoadString (title);
	popup.AppendMenu (MF_STRING | flags, command, titleString);
}

void CRevisionGraphWnd::AddGitOps (CMenu& popup)
{
	bool bothPresent = (m_SelectedEntry2 && m_SelectedEntry1);

	AppendMenu (popup, IDS_REPOBROWSE_SHOWLOG, ID_SHOWLOG);

	if (m_SelectedEntry1 && (m_SelectedEntry2 == NULL))
	{
		//AppendMenu (popup, IDS_SWITCH_TO_THIS, ID_SWITCH);
		AppendMenu(popup, IDS_REVGRAPH_POPUP_COMPAREHEADS, ID_COMPAREHEADS);
		AppendMenu(popup, IDS_REVGRAPH_POPUP_UNIDIFFHEADS,  ID_UNIDIFFHEADS);
	}

	if (bothPresent)
	{
		AppendMenu (popup, IDS_REVGRAPH_POPUP_COMPAREREVS, ID_COMPAREREVS);
		AppendMenu (popup, IDS_REVGRAPH_POPUP_UNIDIFFREVS, ID_UNIDIFFREVS);
	}

}

void CRevisionGraphWnd::AddGraphOps (CMenu& /*popup*/, const CVisibleGraphNode * /*node*/)
{
#if 0
	CSyncPointer<CGraphNodeStates> nodeStates (m_state.GetNodeStates());

	if (node == NULL)
	{
		DWORD state = nodeStates->GetCombinedFlags();
		if (state != 0)
		{
			if (state & CGraphNodeStates::COLLAPSED_ALL)
				AppendMenu (popup, IDS_REVGRAPH_POPUP_EXPAND_ALL, ID_EXPAND_ALL);

			if (state & CGraphNodeStates::SPLIT_ALL)
				AppendMenu (popup, IDS_REVGRAPH_POPUP_JOIN_ALL, ID_JOIN_ALL);
		}
	}
	else
	{
		DWORD state = nodeStates->GetFlags (node);

		if (node->GetSource() || (state & CGraphNodeStates::COLLAPSED_ABOVE))
			AppendMenu ( popup
					   ,   (state & CGraphNodeStates::COLLAPSED_ABOVE)
						 ? IDS_REVGRAPH_POPUP_EXPAND_ABOVE
						 : IDS_REVGRAPH_POPUP_COLLAPSE_ABOVE
					   , ID_GRAPH_EXPANDCOLLAPSE_ABOVE);

		if (node->GetFirstCopyTarget() || (state & CGraphNodeStates::COLLAPSED_RIGHT))
			AppendMenu ( popup
					   ,   (state & CGraphNodeStates::COLLAPSED_RIGHT)
						 ? IDS_REVGRAPH_POPUP_EXPAND_RIGHT
						 : IDS_REVGRAPH_POPUP_COLLAPSE_RIGHT
					   , ID_GRAPH_EXPANDCOLLAPSE_RIGHT);

		if (node->GetNext() || (state & CGraphNodeStates::COLLAPSED_BELOW))
			AppendMenu ( popup
					   ,   (state & CGraphNodeStates::COLLAPSED_BELOW)
						 ? IDS_REVGRAPH_POPUP_EXPAND_BELOW
						 : IDS_REVGRAPH_POPUP_COLLAPSE_BELOW
					   , ID_GRAPH_EXPANDCOLLAPSE_BELOW);

		if (node->GetSource() || (state & CGraphNodeStates::SPLIT_ABOVE))
			AppendMenu ( popup
					   ,   (state & CGraphNodeStates::SPLIT_ABOVE)
						 ? IDS_REVGRAPH_POPUP_JOIN_ABOVE
						 : IDS_REVGRAPH_POPUP_SPLIT_ABOVE
					   , ID_GRAPH_SPLITJOIN_ABOVE);

		if (node->GetFirstCopyTarget() || (state & CGraphNodeStates::SPLIT_RIGHT))
			AppendMenu ( popup
					   ,   (state & CGraphNodeStates::SPLIT_RIGHT)
						 ? IDS_REVGRAPH_POPUP_JOIN_RIGHT
						 : IDS_REVGRAPH_POPUP_SPLIT_RIGHT
					   , ID_GRAPH_SPLITJOIN_RIGHT);

		if (node->GetNext() || (state & CGraphNodeStates::SPLIT_BELOW))
			AppendMenu ( popup
					   ,   (state & CGraphNodeStates::SPLIT_BELOW)
						 ? IDS_REVGRAPH_POPUP_JOIN_BELOW
						 : IDS_REVGRAPH_POPUP_SPLIT_BELOW
					   , ID_GRAPH_SPLITJOIN_BELOW);
	}
#endif
}

CString CRevisionGraphWnd::GetSelectedURL() const
{
#if 0
	if (m_SelectedEntry1 == NULL)
		return CString();

	CString URL = m_state.GetRepositoryRoot()
				+ CUnicodeUtils::GetUnicode (m_SelectedEntry1->GetPath().GetPath().c_str());
	URL = CUnicodeUtils::GetUnicode(CPathUtils::PathEscape(CUnicodeUtils::GetUTF8(URL)));

	return URL;
#endif
	return CString();
}

CString CRevisionGraphWnd::GetWCURL() const
{
#if 0
	CTGitPath path (m_sPath);
	if (path.IsUrl())
		return CString();

	SVNInfo info;
	const SVNInfoData * status
		= info.GetFirstFileInfo (path, SVNRev(), SVNRev());

	return status == NULL ? CString() : status->url;
#endif
	return CString();
}

void CRevisionGraphWnd::DoShowLog()
{

	if(m_SelectedEntry1 == NULL)
		return;

	CString sCmd;

	if(m_SelectedEntry2 != NULL)
		sCmd.Format(_T("/command:log %s /startrev:%s /endrev:%s"),
			this->m_sPath.IsEmpty() ?  _T("") : (_T("/path:\"") + this->m_sPath + _T("\"")),
			this->m_logEntries[m_SelectedEntry1->index()].ToString(),
			this->m_logEntries[m_SelectedEntry2->index()].ToString());
	else
		sCmd.Format(_T("/command:log %s /endrev:%s"),
			this->m_sPath.IsEmpty() ?  _T("") : (_T("/path:\"") + this->m_sPath + _T("\"")),
			this->m_logEntries[m_SelectedEntry1->index()].ToString());

	CAppUtils::RunTortoiseGitProc(sCmd);

}

void CRevisionGraphWnd::DoCheckForModification()
{
	CChangedDlg dlg;
	dlg.m_pathList = CTGitPathList (CTGitPath (m_sPath));
	dlg.DoModal();
}

void CRevisionGraphWnd::DoMergeTo()
{
#if 0
	CString URL = GetSelectedURL();
	CString path = m_sPath;
	CBrowseFolder folderBrowser;
	folderBrowser.SetInfo(CString(MAKEINTRESOURCE(IDS_LOG_MERGETO)));
	if (folderBrowser.Show(GetSafeHwnd(), path, path) == CBrowseFolder::OK)
	{
		CSVNProgressDlg dlg;
		dlg.SetCommand(CSVNProgressDlg::SVNProgress_Merge);
		dlg.SetPathList(CTGitPathList(CTGitPath(path)));
		dlg.SetUrl(URL);
		dlg.SetSecondUrl(URL);
		SVNRevRangeArray revarray;
		revarray.AddRevRange (m_SelectedEntry1->GetRevision()-1, svn_revnum_t(m_SelectedEntry1->GetRevision()));
		dlg.SetRevisionRanges(revarray);
		dlg.DoModal();
	}
#endif
}

void CRevisionGraphWnd::DoUpdate()
{
#if 0
	CSVNProgressDlg progDlg;
	progDlg.SetCommand (CSVNProgressDlg::SVNProgress_Update);
	progDlg.SetOptions (0); // don't ignore externals
	progDlg.SetPathList (CTGitPathList (CTGitPath (m_sPath)));
	progDlg.SetRevision (m_SelectedEntry1->GetRevision());
	progDlg.SetDepth();
	progDlg.DoModal();

	if (m_state.GetFetchedWCState())
		m_parent->UpdateFullHistory();
#endif
}

void CRevisionGraphWnd::DoSwitch()
{
#if 0
	CSVNProgressDlg progDlg;
	progDlg.SetCommand (CSVNProgressDlg::SVNProgress_Switch);
	progDlg.SetPathList (CTGitPathList (CTGitPath (m_sPath)));
	progDlg.SetUrl (GetSelectedURL());
	progDlg.SetRevision (m_SelectedEntry1->GetRevision());
	progDlg.DoModal();

	if (m_state.GetFetchedWCState())
		m_parent->UpdateFullHistory();
#endif
}

void CRevisionGraphWnd::DoSwitchToHead()
{
#if 0
	CSVNProgressDlg progDlg;
	progDlg.SetCommand (CSVNProgressDlg::SVNProgress_Switch);
	progDlg.SetPathList (CTGitPathList (CTGitPath (m_sPath)));
	progDlg.SetUrl (GetSelectedURL());
	progDlg.SetRevision (SVNRev::REV_HEAD);
	progDlg.SetPegRevision (m_SelectedEntry1->GetRevision());
	progDlg.DoModal();

	if (m_state.GetFetchedWCState())
		m_parent->UpdateFullHistory();
#endif
}

void CRevisionGraphWnd::DoBrowseRepo()
{
#if 0
	CString sCmd;
	sCmd.Format(_T("/command:repobrowser /path:\"%s\" /rev:%d"),
	  (LPCTSTR)GetSelectedURL(), m_SelectedEntry1->GetRevision());

	CAppUtils::RunTortoiseProc(sCmd);
#endif
}

void CRevisionGraphWnd::ResetNodeFlags (DWORD /*flags*/)
{
//	m_state.GetNodeStates()->ResetFlags (flags);
//	m_parent->StartWorkerThread();
}

void CRevisionGraphWnd::ToggleNodeFlag (const CVisibleGraphNode * /*node*/, DWORD /*flag*/)
{
#if 0
	CSyncPointer<CGraphNodeStates> nodeStates (m_state.GetNodeStates());

	if (nodeStates->GetFlags (node) & flag)
		nodeStates->ResetFlags (node, flag);
	else
		nodeStates->SetFlags (node, flag);

	m_parent->StartWorkerThread();
#endif
}

void CRevisionGraphWnd::DoCopyUrl()
{
	CStringUtils::WriteAsciiStringToClipboard(GetSelectedURL(), m_hWnd);
}

void CRevisionGraphWnd::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	if (IsUpdateJobRunning())
		return;

	CPoint clientpoint = point;
	this->ScreenToClient(&clientpoint);

	node nodeIndex = GetHitNode (clientpoint);

	if ( !UpdateSelectedEntry (nodeIndex))
	{
		return;
	}

	CMenu popup;
	if (!popup.CreatePopupMenu())
		return;

	AddGitOps (popup);
//	AddGraphOps (popup, clickedentry);

	// if the context menu is invoked through the keyboard, we have to use
	// a calculated position on where to anchor the menu on
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect = GetWindowRect();
		point = rect.CenterPoint();
	}

	int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RIGHTBUTTON, point.x, point.y, this, 0);
	switch (cmd)
	{
	case ID_COMPAREREVS:
		if (m_SelectedEntry1 != NULL)
			CompareRevs(false);
		break;
	case ID_UNIDIFFREVS:
		if (m_SelectedEntry1 != NULL)
			UnifiedDiffRevs(false);
		break;
	case ID_UNIDIFFHEADS:
		if (m_SelectedEntry1 != NULL)
			UnifiedDiffRevs(true);
		break;
	case ID_SHOWLOG:
		DoShowLog();
		break;
	case ID_SWITCH:
		DoSwitch();
		break;
	case ID_COMPAREHEADS:
		if (m_SelectedEntry1 != NULL)
			CompareRevs(true);
		break;


#if 0
	case ID_COMPAREREVS:
		if (m_SelectedEntry1 != NULL)
			CompareRevs(false);
		break;
	case ID_UNIDIFFREVS:
		if (m_SelectedEntry1 != NULL)
			UnifiedDiffRevs(false);
		break;
	case ID_UNIDIFFHEADS:
		if (m_SelectedEntry1 != NULL)
			UnifiedDiffRevs(true);
		break;
	case ID_SHOWLOG:
		DoShowLog();
		break;
	case ID_CFM:
		DoCheckForModification();
		break;
	case ID_MERGETO:
		DoMergeTo();
		break;
	case ID_UPDATE:
		DoUpdate();
		break;
	case ID_SWITCHTOHEAD:
		DoSwitchToHead();
		break;
	case ID_SWITCH:
		DoSwitch();
		break;
	case ID_COPYURL:
		DoCopyUrl();
		break;
	case ID_BROWSEREPO:
		DoBrowseRepo();
		break;
	case ID_EXPAND_ALL:
		ResetNodeFlags (CGraphNodeStates::COLLAPSED_ALL);
		break;
	case ID_JOIN_ALL:
		ResetNodeFlags (CGraphNodeStates::SPLIT_ALL);
		break;
	case ID_GRAPH_EXPANDCOLLAPSE_ABOVE:
		ToggleNodeFlag (clickedentry, CGraphNodeStates::COLLAPSED_ABOVE);
		break;
	case ID_GRAPH_EXPANDCOLLAPSE_RIGHT:
		ToggleNodeFlag (clickedentry, CGraphNodeStates::COLLAPSED_RIGHT);
		break;
	case ID_GRAPH_EXPANDCOLLAPSE_BELOW:
		ToggleNodeFlag (clickedentry, CGraphNodeStates::COLLAPSED_BELOW);
		break;
	case ID_GRAPH_SPLITJOIN_ABOVE:
		ToggleNodeFlag (clickedentry, CGraphNodeStates::SPLIT_ABOVE);
		break;
	case ID_GRAPH_SPLITJOIN_RIGHT:
		ToggleNodeFlag (clickedentry, CGraphNodeStates::SPLIT_RIGHT);
		break;
	case ID_GRAPH_SPLITJOIN_BELOW:
		ToggleNodeFlag (clickedentry, CGraphNodeStates::SPLIT_BELOW);
		break;
#endif
	}

}

void CRevisionGraphWnd::OnMouseMove(UINT nFlags, CPoint point)
{

	if (IsUpdateJobRunning())
	{
		return __super::OnMouseMove(nFlags, point);
	}
	if (!m_bIsRubberBand)
	{
		if (m_bShowOverview && (m_OverviewRect.PtInRect(point))&&(nFlags & MK_LBUTTON))
		{
			// scrolling
			CRect viewRect = GetViewRect();
			int x = (int)((point.x-m_OverviewRect.left - (m_OverviewPosRect.Width()/2)) / m_previewZoom  * m_fZoomFactor);
			int y = (int)((point.y - m_OverviewRect.top - (m_OverviewPosRect.Height()/2)) / m_previewZoom  * m_fZoomFactor);
			x = max(0, x);
			y = max(0, y);
			SetScrollbars(y, x);
			Invalidate(FALSE);
			return __super::OnMouseMove(nFlags, point);
		}
		else
		{
			// update screen if we hover over a different
			// node than during the last redraw

			CPoint clientPoint = point;
			GetCursorPos (&clientPoint);
			ScreenToClient (&clientPoint);

#if 0
			const CRevisionGraphState::SVisibleGlyph* hitGlyph
				= GetHitGlyph (clientPoint);
			const CFullGraphNode* glyphNode
				= hitGlyph ? hitGlyph->node->GetBase() : NULL;

			const CFullGraphNode* hoverNode = NULL;
			if (m_hoverIndex != NO_INDEX)
			{
				CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
				if (m_hoverIndex < nodeList->GetCount())
					hoverNode = nodeList->GetNode (m_hoverIndex).node->GetBase();
			}

			//bool onHoverNodeGlyph = (hoverNode != NULL) && (glyphNode == hoverNode);
			if (   !m_hoverIndex
				&& (   (m_hoverIndex != GetHitNode (clientPoint))))
			{
				m_showHoverGlyphs = false;

				KillTimer (GLYPH_HOVER_EVENT);
				SetTimer (GLYPH_HOVER_EVENT, GLYPH_HOVER_DELAY, NULL);

				Invalidate(FALSE);
			}
#endif
			return __super::OnMouseMove(nFlags, point);
		}
	}
#if 0
	if ((abs(m_ptRubberStart.x - point.x) < 2)&&(abs(m_ptRubberStart.y - point.y) < 2))
	{
		return __super::OnMouseMove(nFlags, point);
	}

	SetCapture();

	if ((m_ptRubberEnd.x != 0)||(m_ptRubberEnd.y != 0))
		DrawRubberBand();
	m_ptRubberEnd = point;
	CRect rect = GetClientRect();
	m_ptRubberEnd.x = max(m_ptRubberEnd.x, rect.left);
	m_ptRubberEnd.x = min(m_ptRubberEnd.x, rect.right);
	m_ptRubberEnd.y = max(m_ptRubberEnd.y, rect.top);
	m_ptRubberEnd.y = min(m_ptRubberEnd.y, rect.bottom);
	DrawRubberBand();
#endif
	__super::OnMouseMove(nFlags, point);
}

BOOL CRevisionGraphWnd::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CRect viewRect = GetViewRect();

	LPTSTR cursorID = IDC_ARROW;
	HINSTANCE resourceHandle = NULL;

	if ((nHitTest == HTCLIENT)&&(pWnd == this)&&(viewRect.Width())&&(viewRect.Height())&&(message))
	{
		POINT pt;
		if (GetCursorPos(&pt))
		{
			ScreenToClient(&pt);
			if (m_OverviewPosRect.PtInRect(pt))
			{
				resourceHandle = AfxGetResourceHandle();
				cursorID = GetKeyState(VK_LBUTTON) & 0x8000
						 ? MAKEINTRESOURCE(IDC_PANCURDOWN)
						 : MAKEINTRESOURCE(IDC_PANCUR);
			}
		}
	}

	HCURSOR hCur = LoadCursor(resourceHandle, MAKEINTRESOURCE(cursorID));
	if (GetCursor() != hCur)
		SetCursor (hCur);

	return TRUE;
}

void CRevisionGraphWnd::OnTimer (UINT_PTR nIDEvent)
{
	if (nIDEvent == GLYPH_HOVER_EVENT)
	{
		KillTimer (GLYPH_HOVER_EVENT);

		m_showHoverGlyphs = true;
		Invalidate (FALSE);
	}
	else
	{
		__super::OnTimer (nIDEvent);
	}
}

LRESULT CRevisionGraphWnd::OnWorkerThreadDone(WPARAM, LPARAM)
{
	// handle potential race condition between PostMessage and leaving job:
	// the background job may not have exited, yet

	if (updateJob.get())
		updateJob->GetResult();

	InitView();
	BuildPreview();

	SCROLLINFO sinfo = {0};
	sinfo.cbSize = sizeof(SCROLLINFO);
	GetScrollInfo(SB_HORZ, &sinfo);
	sinfo.nPos = sinfo.nMax;
	SetScrollInfo(SB_HORZ, &sinfo);

	Invalidate(FALSE);

#if 0
	SVN svn;
	LogCache::CRepositoryInfo& cachedProperties
		= svn.GetLogCachePool()->GetRepositoryInfo();

	CSyncPointer<const CFullHistory> fullHistoy (m_state.GetFullHistory());
	if (fullHistoy.get() != NULL)
	{
		SetDlgTitle (cachedProperties.IsOffline
			( fullHistoy->GetRepositoryUUID()
			, fullHistoy->GetRepositoryRoot()
			, false));
	}

	if (m_parent && !m_parent->GetOutputFile().IsEmpty())
	{
		// save the graph to the output file and exit
		SaveGraphAs(m_parent->GetOutputFile());
		PostQuitMessage(0);
	}
#endif
	return 0;
}

void CRevisionGraphWnd::SetDlgTitle (bool /*offline*/)
{
#if 0
	if (m_sTitle.IsEmpty())
		GetParent()->GetWindowText(m_sTitle);

	CString newTitle;
	if (offline)
		newTitle.Format (IDS_REVGRAPH_DLGTITLEOFFLINE, (LPCTSTR)m_sTitle);
	else
		newTitle = m_sTitle;

	CAppUtils::SetWindowTitle(GetParent()->GetSafeHwnd(), m_sPath, newTitle);
#endif
}
