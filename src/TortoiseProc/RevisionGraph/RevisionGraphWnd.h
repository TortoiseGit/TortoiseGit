// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN
// Copyright (C) 2012-2016 - TortoiseGit

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
//#include "RevisionGraph/RevisionGraphState.h"
#pragma warning(push)
#pragma warning(disable: 4481) // nonstandard extension used: override specifier 'override'
#include "Future.h"
#pragma warning(pop)
#include "ProgressDlg.h"
#include "Colors.h"
//#include "SVNDiff.h"
#include "AppUtils.h"
#include "SVG.h"
#include "LogDlgHelper.h"
#include "Graphviz.h"

#pragma warning(push)
#pragma warning(disable: 4100) // unreferenced formal parameter
#include <ogdf/layered/SugiyamaLayout.h>
#include <ogdf/layered/OptimalRanking.h>
#include <ogdf/layered/MedianHeuristic.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#pragma warning(pop)

typedef void CVisibleGraphNode;
typedef int index_t;

using namespace Gdiplus;
using namespace async;

enum
{
	REVGRAPH_PREVIEW_WIDTH = 100,
	REVGRAPH_PREVIEW_HEIGHT = 200,

	// don't draw pre-views with more than that number of nodes

	REVGRAPH_PREVIEW_MAX_NODES = 10000
};

// don't try to draw nodes smaller than that:

#define REVGRAPH_MIN_NODE_HIGHT (0.5f)

enum
{
	// size of the node marker

	MARKER_SIZE = 11,

	// radius of the rounded / slanted box corners  of the expand / collapse / split / join square gylphs

	CORNER_SIZE = 12,

	// font sizes

	DEFAULT_ZOOM_FONT = 9,			// default font size
	SMALL_ZOOM_FONT = 11,			// rel. larger font size for small zoom factors
	SMALL_ZOOM_FONT_THRESHOLD = 6,	// max. "small zoom" font size after scaling

	// size of the expand / collapse / split / join square gylphs

	GLYPH_BITMAP_SIZE = 16,
	GLYPH_SIZE = 12,

	// glyph display delay definitions

	GLYPH_HOVER_EVENT = 10,		// timer ID for the glyph display delay
	GLYPH_HOVER_DELAY = 250,	// delay until the glyphs are shown [ms]
};

// zoom control

const float MIN_ZOOM = 0.01f;
const float MAX_ZOOM = 2.0f;
const float DEFAULT_ZOOM = 1.0f;
const float ZOOM_STEP = 0.9f;

// don't draw shadows below this zoom level

const float SHADOW_ZOOM_THRESHOLD = 0.2f;

/**
 * \ingroup TortoiseProc
 * node shapes for the revision graph
 */
enum NodeShape
{
	TSVNRectangle,
	TSVNRoundRect,
	TSVNOctangle,
	TSVNEllipse
};

#define MAXFONTS				4
#define MAX_TT_LENGTH			60000
#define MAX_TT_LENGTH_DEFAULT	1000

// forward declarations

class CRevisionGraphDlg;

// simplify usage of classes from other namespaces

//using async::IJob;
//using async::CFuture;

/**
 * \ingroup TortoiseProc
 * Window class showing a revision graph.
 *
 * The analyzation of the log data is done in the child class CRevisionGraph.
 * Here, we handle the window notifications.
 */
class CRevisionGraphWnd : public CWnd //, public CRevisionGraph
{
public:
	CRevisionGraphWnd();	// standard constructor
	virtual ~CRevisionGraphWnd();
	enum
	{
		IDD = IDD_REVISIONGRAPH,
		WM_WORKERTHREADDONE = WM_APP +1
	};


	CString			m_sPath;
	GitRev			m_pegRev;

	CLogCache			m_LogCache;
	CLogDataVector		m_logEntries;
	MAP_HASH_NAME		m_HashMap;
	CString				m_CurrentBranch;
	CGitHash				m_HeadHash;

	BOOL		m_bCurrentBranch;
	BOOL		m_bLocalBranches;
	CString		m_FromRev;
	CString		m_ToRev;

	void ReloadHashMap()
	{
		m_HashMap.clear();
		if (g_Git.GetMapHashToFriendName(m_HashMap))
			MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);
		m_CurrentBranch=g_Git.GetCurrentBranch();
		if (g_Git.GetHash(m_HeadHash, L"HEAD"))
			MessageBox(g_Git.GetGitLastErr(L"Could not get HEAD hash."), L"TortoiseGit", MB_ICONERROR);
	}

	std::auto_ptr<CFuture<bool>> updateJob;
//	CRevisionGraphState m_state;

	void			InitView();
	void			Init(CWnd * pParent, LPRECT rect);
	void			SaveGraphAs(CString sSavePath);

	bool			FetchRevisionData ( const CString& path
										, CProgressDlg* progress
										, ITaskbarList3* pTaskbarList
										, HWND hWnd);
	bool			AnalyzeRevisionData();
	bool			IsUpdateJobRunning() const;

	bool			GetShowOverview() const;
	void			SetShowOverview (bool value);

	void			GetSelected (const CVisibleGraphNode* node, bool head, CTGitPath& path, GitRev& rev, GitRev& peg);
	void			CompareRevs(const CString& revTo);
	void			UnifiedDiffRevs(bool bHead);

	CRect			GetGraphRect();
	CRect			GetClientRect();
	CRect			GetWindowRect();
	CRect			GetViewRect();
	void			DoZoom (float nZoomFactor, bool updateScrollbars = true);
	bool			CancelMouseZoom();

	void			SetDlgTitle (bool offline);

	void			BuildPreview();

protected:
	ULONGLONG		m_ullTicks;
	CRect			m_OverviewPosRect;
	CRect			m_OverviewRect;

	bool			m_bShowOverview;

	CRevisionGraphDlg *m_parent;

	ogdf::node		m_HeadNode;
	ogdf::node		m_SelectedEntry1;
	ogdf::node		m_SelectedEntry2;
	LOGFONT			m_lfBaseFont;
	CFont *			m_apFonts[MAXFONTS];
	int				m_nFontSize;
	CToolTipCtrl *	m_pDlgTip;
	char			m_szTip[MAX_TT_LENGTH+1];
	wchar_t			m_wszTip[MAX_TT_LENGTH+1];
	CString			m_sTitle;

	float			m_fZoomFactor;
	CColors			m_Colors;
	bool			m_bTweakTrunkColors;
	bool			m_bTweakTagsColors;
	bool			m_bIsCanvasMove;
	CPoint			m_ptMoveCanvas;
	CPoint			m_ptRubberEnd;

	CBitmap			m_Preview;
	int				m_previewWidth;
	int				m_previewHeight;
	float			m_previewZoom;

//	index_t			m_hoverIndex;	// node the cursor currently hovers over
	ogdf::node		m_hoverIndex;
	DWORD			m_hoverGlyphs;	// the glyphs shown for \ref m_hoverIndex
	mutable ogdf::node m_tooltipIndex;	// the node index we fetched the tooltip for
	bool			m_showHoverGlyphs;	// if true, show the glyphs we currently hover over
										// (will be activated only after some delay)

	CString		GetFriendRefName(ogdf::node);
	STRING_VECTOR		GetFriendRefNames(ogdf::node, CGit::REF_TYPE* refTypes = nullptr, int refTypeCount = 0);

	ogdf::Graph	m_Graph;
	ogdf::GraphAttributes m_GraphAttr;
	ogdf::SugiyamaLayout m_SugiyamLayout;

	CRect	m_GraphRect;

	int	 GetLeftRightMargin() {return 20;};
	int	 GetTopBottomMargin() {return 5;};
	virtual void	DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL	OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL	OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void	OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void	OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnTimer(UINT_PTR nIDEvent);
	afx_msg void	OnCaptureChanged(CWnd *pWnd);
	afx_msg LRESULT OnWorkerThreadDone(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
private:

	double m_ArrowCos;
	double m_ArrowSin;
	double m_ArrowSize;

	enum MarkerPosition
	{
		mpLeft = 0,
		mpRight = 1,
	};

	enum GlyphType
	{
		NoGlyph = -1,
		ExpandGlyph = 0,	// "+"
		CollapseGlyph = 1,	// "-"
		SplitGlyph = 2,		// "x"
		JoinGlyph = 3,		// "o"
	};

	enum GlyphPosition
	{
		Above = 0,
		Right = 4,
		Below = 8,
	};

	class GraphicsDevice
	{
	public:
		GraphicsDevice()
			: pDC(nullptr)
			, graphics(nullptr)
			, pSVG(nullptr)
			, pGraphviz(nullptr)
		{
		}
		~GraphicsDevice() {}
	public:
		CDC *				pDC;
		Graphics *			graphics;
		SVG *				pSVG;
		Graphviz *			pGraphviz;
	};

	class SVGGrouper
	{
	public:
		SVGGrouper(SVG * pSVG)
		{
			m_pSVG = pSVG;
			if (m_pSVG)
				m_pSVG->StartGroup();
		}
		~SVGGrouper()
		{
			if (m_pSVG)
				m_pSVG->EndGroup();
		}
	private:
		SVGGrouper() {}

		SVG *	m_pSVG;
	};

	bool			UpdateSelectedEntry (ogdf::node clickedentry);
	void			AppendMenu (CMenu& popup, UINT title, UINT command, UINT flags = MF_ENABLED);
	void			AppendMenu(CMenu& popup, CString title, UINT command, CString* extra = nullptr, CMenu* submenu = nullptr);
	void			AddGitOps (CMenu& popup);
	void			AddGraphOps (CMenu& popup, const CVisibleGraphNode * node);
	CString			GetSelectedURL() const;
	CString			GetWCURL() const;
	void			DoShowLog();
	void			DoCheckForModification();
	void			DoMergeTo();
	void			DoUpdate();
	void			DoSwitch(CString rev);
	void			DoSwitchToHead();
	void			DoBrowseRepo();
	void			ResetNodeFlags (DWORD flags);
	void			ToggleNodeFlag (const CVisibleGraphNode *node, DWORD flag);
	void			DoCopyRefs();

	void			SetScrollbar (int bar, int newPos, int clientMax, int graphMax);
	void			SetScrollbars (int nVert = -1, int nHorz = -1);
	CFont*			GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE);

	CSize			UsableTooltipRect();
	CString			DisplayableText (const CString& wholeText, const CSize& tooltipSize);
	CString			TooltipText (ogdf::node index);

	CPoint			GetLogCoordinates (CPoint point) const;
	ogdf::node		GetHitNode (CPoint point, CSize border = CSize (0, 0)) const;
	DWORD			GetHoverGlyphs (CPoint point) const;
	PointF			cutPoint(ogdf::node v,double lw,PointF ps, PointF pt);

//	const CRevisionGraphState::SVisibleGlyph* GetHitGlyph (CPoint point) const;

	void			ClearVisibleGlyphs (const CRect& rect);

	typedef PointF TCutRectangle[8];
	void			CutawayPoints (const RectF& rect, float cutLen, TCutRectangle& result);
	enum
	{
		ROUND_UP = 0x1,
		ROUND_DOWN = 0x2,
		ROUND_BOTH = 0x3,
	};
	void			DrawRoundedRect (GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect, int mask=ROUND_BOTH);
	void			DrawOctangle (GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect);
	void			DrawShape (GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect, NodeShape shape);
	void			DrawShadow(GraphicsDevice& graphics, const RectF& rect,
							   Color shadowColor, NodeShape shape);
	void			DrawNode(GraphicsDevice& graphics, const RectF& rect,
							 Color contour, Color overlayColor,
							 const CVisibleGraphNode *node, NodeShape shape);
	RectF			TransformRectToScreen (const CRect& rect, const CSize& offset) const;
	RectF			GetNodeRect (const ogdf::node& v, const CSize& offset) const;
//	RectF			GetBranchCover (const ILayoutNodeList* nodeList, index_t nodeIndex, bool upward, const CSize& offset);

	void			DrawSquare (GraphicsDevice& graphics, const PointF& leftTop,
								const Color& lightColor, const Color& darkColor, const Color& penColor);
	void			DrawGlyph (GraphicsDevice& graphics, Image* glyphs, const PointF& leftTop,
							   GlyphType glyph, GlyphPosition position);
	void			DrawGlyphs (GraphicsDevice& graphics, Image* glyphs, const CVisibleGraphNode* node, const PointF& center,
								GlyphType glyph1, GlyphType glyph2, GlyphPosition position, DWORD state1, DWORD state2, bool showAll);
	void			DrawGlyphs (GraphicsDevice& graphics, Image* glyphs, const CVisibleGraphNode* node, const RectF& nodeRect,
								DWORD state, DWORD allowed, bool upsideDown);
	void			DrawMarker ( GraphicsDevice& graphics, const RectF& noderect
							   , MarkerPosition position, int relPosition, const Color& penColor, int num);
//	void			IndicateGlyphDirection ( GraphicsDevice& graphics, const ILayoutNodeList* nodeList
//										 , const ILayoutNodeList::SNode& node, const RectF& nodeRect
//										 , DWORD glyphs, bool upsideDown, const CSize& offset);

	void			DrawStripes (GraphicsDevice& graphics, const CSize& offset);

	void			DrawShadows (GraphicsDevice& graphics, const CRect& logRect, const CSize& offset);
	void			DrawNodes (GraphicsDevice& graphics, Image* glyphs, const CRect& logRect, const CSize& offset);
	void			DrawConnections (GraphicsDevice& graphics, const CRect& logRect, const CSize& offset);
	void			DrawTexts (GraphicsDevice& graphics, const CRect& logRect, const CSize& offset);
	void			DrawCurrentNodeGlyphs (GraphicsDevice& graphics, Image* glyphs, const CSize& offset);
	void			DrawGraph(GraphicsDevice& graphics, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw);

	int				GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	void			DrawRubberBand();
	void	SetNodeRect(GraphicsDevice& graphics, ogdf::node *pnode, CGitHash rev, int mode = 0);
};
