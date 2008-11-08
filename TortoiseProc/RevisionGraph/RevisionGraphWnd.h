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
#include "RevisionGraph/FullHistory.h"
#include "RevisionGraph/FullGraph.h"
#include "RevisionGraph/VisibleGraph.h"
#include "RevisionGraph/IRevisionGraphLayout.h"
#include "ProgressDlg.h"
#include "Colors.h"
#include "SVNDiff.h"
#include "AppUtils.h"

using namespace Gdiplus;

#define REVGRAPH_PREVIEW_WIDTH 100
#define REVGRAPH_PREVIEW_HEIGHT 200

// we need at least 5x2 pixels per node 
// to draw a meaningful pre-view

#define REVGRAPH_PREVIEW_MAX_NODES (REVGRAPH_PREVIEW_HEIGHT * REVGRAPH_PREVIEW_WIDTH / 10)

// don't try to draw nodes smaller than that:

#define REVGRAPH_MIN_NODE_HIGHT (0.5f)

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
#define	MAX_TT_LENGTH			60000
#define	MAX_TT_LENGTH_DEFAULT	1000

// forward declarations

class CVisibleGraphNode;
class IRevisionGraphLayout;
class CAllRevisionGraphOptions;

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
	CRevisionGraphWnd();   // standard constructor
	virtual ~CRevisionGraphWnd();
	enum 
    { 
        IDD = IDD_REVISIONGRAPH,
        WM_WORKERTHREADDONE = WM_APP +1
    };


	CString			m_sPath;
    SVNRev          m_pegRev;
	volatile LONG	m_bThreadRunning;
	CProgressDlg* 	m_pProgress;

	void			InitView();
	void			Init(CWnd * pParent, LPRECT rect);
	void			SaveGraphAs(CString sSavePath);

    bool            FetchRevisionData ( const CString& path
                                      , SVNRev pegRevision
                                      , const CAllRevisionGraphOptions& options);
    bool            AnalyzeRevisionData (const CAllRevisionGraphOptions& options);
    CString         GetLastErrorMessage() const;

    bool            GetShowOverview() const;
    void            SetShowOverview (bool value);

	void			CompareRevs(bool bHead);
	void			UnifiedDiffRevs(bool bHead);

	CRect           GetViewRect();
    int             GetNodeCount();
	void			DoZoom(float nZoomFactor);

    void            SetDlgTitle (bool offline);

    svn_revnum_t    GetHeadRevision() const;             
    CString         GetRepositoryRoot() const;             
    CString         GetRepositoryUUID() const;

protected:
	DWORD			m_dwTicks;
	CRect			m_OverviewPosRect;
	CRect			m_OverviewRect;

	BOOL			m_bShowOverview;

    std::auto_ptr<CFullHistory>         m_fullHistory;
    std::auto_ptr<CFullGraph>           m_fullGraph;
    std::auto_ptr<CVisibleGraph>        m_visibleGraph;
    std::auto_ptr<IRevisionGraphLayout> m_layout;

	const CVisibleGraphNode * m_SelectedEntry1;
	const CVisibleGraphNode * m_SelectedEntry2;
	LOGFONT			m_lfBaseFont;
	CFont *			m_apFonts[MAXFONTS];
	int				m_nFontSize;
	CToolTipCtrl *	m_pDlgTip;
	char			m_szTip[MAX_TT_LENGTH+1];
	wchar_t			m_wszTip[MAX_TT_LENGTH+1];
    CString			m_sTitle;

	float			m_fZoomFactor;
	CColors			m_Colors;
    bool            m_bTweakTrunkColors;
    bool            m_bTweakTagsColors;
	bool			m_bIsRubberBand;
	CPoint			m_ptRubberStart;
	CPoint			m_ptRubberEnd;

	CBitmap			m_Preview;
	int				m_previewWidth;
	int				m_previewHeight;
    float           m_previewZoom;
	
	virtual void	DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	afx_msg void	OnPaint();
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg INT_PTR	OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL	OnToolTipNotify(UINT id, NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg BOOL	OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void	OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg LRESULT	OnWorkerThreadDone(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()
private:

    typedef bool (SVNDiff::*TDiffFunc)(const CTSVNPath& url1, const SVNRev& rev1, 
	            					   const CTSVNPath& url2, const SVNRev& rev2, 
						               SVNRev peg,
						               bool ignoreancestry,
						               bool blame);
    typedef bool (*TStartDiffFunc)(HWND hWnd, const CTSVNPath& url1, const SVNRev& rev1, 
						           const CTSVNPath& url2, const SVNRev& rev2, 
						           const SVNRev& peg, const SVNRev& headpeg,
						           bool bAlternateDiff,
						           bool bIgnoreAncestry,
                                   bool blame);

    void            Compare (TDiffFunc diffFunc, TStartDiffFunc startDiffFunc, bool bHead);
    bool            PromptShown() const;

	void			SetScrollbars(int nVert = 0, int nHorz = 0, int oldwidth = 0, int oldheight = 0);
	CRect       	GetGraphRect();
	CFont*			GetFont(BOOL bItalic = FALSE, BOOL bBold = FALSE);

    CSize           UsableTooltipRect();
    CString         DisplayableText (const CString& wholeText, const CSize& tooltipSize);
    CString         TooltipText (index_t index);

    index_t         GetHitNode (CPoint point) const;

    typedef PointF TCutRectangle[8];
    void            CutawayPoints (const RectF& rect, float cutLen, TCutRectangle& result);
    void            DrawRoundedRect (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect);
	void			DrawOctangle (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect);
    void            DrawShape (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect, NodeShape shape);
	void			DrawShadow(Graphics& graphics, const RectF& rect,
							   Color shadowColor, NodeShape shape);
	void			DrawNode(Graphics& graphics, const RectF& rect,
							 COLORREF contourRef, Color overlayColor,
                             const CVisibleGraphNode *node, NodeShape shape);
    RectF           GetNodeRect (const ILayoutNodeList::SNode& node, const CSize& offset) const;

    void            DrawShadows (Graphics& graphics, const CRect& logRect, const CSize& offset);
    void            DrawNodes (Graphics& graphics, const CRect& logRect, const CSize& offset);
    void            DrawConnections (CDC* pDC, const CRect& logRect, const CSize& offset);
    void            DrawTexts (CDC* pDC, const CRect& logRect, const CSize& offset);
    void			DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw);

	int				GetEncoderClsid(const WCHAR* format, CLSID* pClsid);
	void			DrawRubberBand();

	void			BuildPreview();
};
