// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2009 - TortoiseSVN

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
#include "MyMemDC.h"
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "SVN.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TSVNPath.h"
#include "SVNInfo.h"
#include "SVNDiff.h"
#include ".\revisiongraphwnd.h"
#include "IRevisionGraphLayout.h"
#include "UpsideDownLayout.h"
#include "ShowTreeStripes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

/************************************************************************/
/* Graphing functions                                                   */
/************************************************************************/
CFont* CRevisionGraphWnd::GetFont(BOOL bItalic /*= FALSE*/, BOOL bBold /*= FALSE*/)
{
	int nIndex = 0;
	if (bBold)
		nIndex |= 1;
	if (bItalic)
		nIndex |= 2;
	if (m_apFonts[nIndex] == NULL)
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = (BYTE) bItalic;
		m_lfBaseFont.lfStrikeOut = (BYTE) FALSE;
		CDC * pDC = GetDC();
		m_lfBaseFont.lfHeight = -MulDiv(m_nFontSize, GetDeviceCaps(pDC->m_hDC, LOGPIXELSY), 72);
		ReleaseDC(pDC);
		// use the empty font name, so GDI takes the first font which matches
		// the specs. Maybe this will help render chinese/japanese chars correctly.
		_tcsncpy_s(m_lfBaseFont.lfFaceName, 32, _T("MS Shell Dlg 2"), 32);
		if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
		{
			delete m_apFonts[nIndex];
			m_apFonts[nIndex] = NULL;
			return CWnd::GetFont();
		}
	}
	return m_apFonts[nIndex];
}

BOOL CRevisionGraphWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
	return TRUE;
}

void CRevisionGraphWnd::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	CRect rect = GetClientRect();
	if (m_bThreadRunning)
	{
		dc.FillSolidRect(rect, ::GetSysColor(COLOR_APPWORKSPACE));
		CWnd::OnPaint();
		return;
	}
    else if (!m_state.GetNodes())
	{
		CString sNoGraphText;
		sNoGraphText.LoadString(IDS_REVGRAPH_ERR_NOGRAPH);
		dc.FillSolidRect(rect, RGB(255,255,255));
		dc.ExtTextOut(20,20,ETO_CLIPPED,NULL,sNoGraphText,NULL);
		return;
	}
	
    DrawGraph(&dc, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ), false);
}

void CRevisionGraphWnd::ClearVisibleGlyphs (const CRect& rect)
{
    float glyphSize = GLYPH_SIZE * m_fZoomFactor;

    CSyncPointer<CRevisionGraphState::TVisibleGlyphs> 
        visibleGlyphs (m_state.GetVisibleGlyphs());

    for (size_t i = visibleGlyphs->size(), count = i; i > 0; --i)
    {
        const PointF& leftTop = (*visibleGlyphs)[i-1].leftTop;
        CRect glyphRect ( static_cast<int>(leftTop.X)
                        , static_cast<int>(leftTop.Y)
                        , static_cast<int>(leftTop.X + glyphSize)
                        , static_cast<int>(leftTop.Y + glyphSize));

        if (CRect().IntersectRect (glyphRect, rect))
        {
            (*visibleGlyphs)[i-1] = (*visibleGlyphs)[--count];
            visibleGlyphs->pop_back();
        }
    }
}

void CRevisionGraphWnd::CutawayPoints (const RectF& rect, float cutLen, TCutRectangle& result)
{
    result[0] = PointF (rect.X, rect.Y + cutLen);
    result[1] = PointF (rect.X + cutLen, rect.Y);
    result[2] = PointF (rect.GetRight() - cutLen, rect.Y);
    result[3] = PointF (rect.GetRight(), rect.Y + cutLen);
    result[4] = PointF (rect.GetRight(), rect.GetBottom() - cutLen);
    result[5] = PointF (rect.GetRight() - cutLen, rect.GetBottom());
    result[6] = PointF (rect.X + cutLen, rect.GetBottom());
    result[7] = PointF (rect.X, rect.GetBottom() - cutLen);
}

void CRevisionGraphWnd::DrawRoundedRect (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect)
{
    enum {POINT_COUNT = 8};

    float radius = 16 * m_fZoomFactor;
	PointF points[POINT_COUNT];
    CutawayPoints (rect, radius, points);

    GraphicsPath path;
    path.AddArc (points[0].X, points[1].Y, radius, radius, 180, 90);
    path.AddArc (points[2].X, points[2].Y, radius, radius, 270, 90);
    path.AddArc (points[5].X, points[4].Y, radius, radius, 0, 90);
    path.AddArc (points[7].X, points[7].Y, radius, radius, 90, 90);

    points[0].Y -= radius / 2;
    path.AddLine (points[7], points[0]);

    if (brush != NULL)
        graphics.FillPath (brush, &path);
    if (pen != NULL)
        graphics.DrawPath (pen, &path);
}

void CRevisionGraphWnd::DrawOctangle (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect)
{
    enum {POINT_COUNT = 8};

    // show left & right edges of low boxes as "<===>"

    float minCutAway = min (16 * m_fZoomFactor, rect.Height / 2);

    // larger boxes: remove 25% of the shorter side

    float suggestedCutAway = min (rect.Height, rect.Width) / 4;

    // use the more visible one of the former two

	PointF points[POINT_COUNT];
    CutawayPoints (rect, max (minCutAway, suggestedCutAway), points);

    // now, draw it

    if (brush != NULL)
        graphics.FillPolygon (brush, points, POINT_COUNT);
    if (pen != NULL)
        graphics.DrawPolygon (pen, points, POINT_COUNT);
}

void CRevisionGraphWnd::DrawShape (Graphics& graphics, const Pen* pen, const Brush* brush, const RectF& rect, NodeShape shape)
{
	switch( shape )
	{
	case TSVNRectangle:
        if (brush != NULL)
            graphics.FillRectangle (brush, rect);
        if (pen != NULL)
            graphics.DrawRectangle (pen, rect);
		break;
	case TSVNRoundRect:
		DrawRoundedRect (graphics, pen, brush, rect);
		break;
	case TSVNOctangle:
		DrawOctangle (graphics, pen, brush, rect);
		break;
	case TSVNEllipse:
        if (brush != NULL)
            graphics.FillEllipse (brush, rect);
        if (pen != NULL)
            graphics.DrawEllipse(pen, rect);
		break;
	default:
		ASSERT(FALSE);	//unknown type
		return;
	}
}

inline BYTE LimitedScaleColor (BYTE c1, BYTE c2, float factor)
{
    BYTE scaled = c2 + (BYTE)((c1-c2)*factor);
    return c1 < c2
        ? max (c1, scaled)
        : min (c1, scaled);
}

Color LimitedScaleColor (const Color& c1, const Color& c2, float factor)
{
    return Color ( LimitedScaleColor (c1.GetA(), c2.GetA(), factor)
                 , LimitedScaleColor (c1.GetR(), c2.GetR(), factor)
                 , LimitedScaleColor (c1.GetG(), c2.GetG(), factor)
                 , LimitedScaleColor (c1.GetB(), c2.GetB(), factor));
}

inline BYTE Darken (BYTE c)
{
    return c < 0xc0
        ? (c / 3) * 2
        : BYTE(int(2*c) - 0x100);
}

Color Darken (const Color& c)
{
    return Color ( 0xff
                 , Darken (c.GetR())
                 , Darken (c.GetG())
                 , Darken (c.GetB()));
}

BYTE MaxComponentDiff (const Color& c1, const Color& c2)
{
    int rDiff = abs ((int)c1.GetR() - (int)c2.GetR());
    int gDiff = abs ((int)c1.GetG() - (int)c2.GetG());
    int bDiff = abs ((int)c1.GetB() - (int)c2.GetB());

    return (BYTE) max (max (rDiff, gDiff), bDiff);
}

void CRevisionGraphWnd::DrawShadow (Graphics& graphics, const RectF& rect,
                                    Color shadowColor, NodeShape shape)
{
	// draw the shadow

	RectF shadow = rect;
    shadow.Offset (2, 2);

    Pen pen (shadowColor);
	SolidBrush brush (shadowColor);

    DrawShape (graphics, &pen, &brush, shadow, shape);
}

void CRevisionGraphWnd::DrawNode(Graphics& graphics, const RectF& rect,
                                 Color contour, Color overlayColor, 
                                 const CVisibleGraphNode *node, NodeShape shape)
{
    // special case: line deleted but deletion node removed
    // (don't show as "deleted" if the following node has been folded / split)

    enum 
    {
        MASK = CGraphNodeStates::COLLAPSED_BELOW | CGraphNodeStates::SPLIT_BELOW
    };

    CNodeClassification nodeClassification = node->GetClassification();
    if (   (node->GetNext() == NULL) 
        && (nodeClassification.Is (CNodeClassification::PATH_ONLY_DELETED))
        && ((m_state.GetNodeStates()->GetFlags (node) & MASK) == 0))
    {
        contour = m_Colors.GetColor (CColors::gdpDeletedNode);
    }

    // calculate the GDI+ color values we need to draw the node

    Color background;
    background.SetFromCOLORREF (GetSysColor(COLOR_WINDOW));
    Color textColor;
    if (nodeClassification.Is (CNodeClassification::IS_MODIFIED_WC))
        textColor = m_Colors.GetColor (CColors::gdpWCNodeBorder);
    else
        textColor.SetFromCOLORREF (GetSysColor(COLOR_WINDOWTEXT));

	Color brightColor = LimitedScaleColor (background, contour, 0.9f);

	// Draw the main shape

    bool isWorkingCopy 
        = nodeClassification.Is (CNodeClassification::IS_WORKINGCOPY);
    bool textAsBorderColor 
        = nodeClassification.IsAnyOf ( CNodeClassification::IS_LAST
                                     | CNodeClassification::IS_MODIFIED_WC)
        | nodeClassification.Matches ( CNodeClassification::IS_COPY_SOURCE
                                     , CNodeClassification::IS_OPERATION_MASK);

    Color penColor = textAsBorderColor
                   ? textColor
                   : contour;

    Pen pen (penColor, isWorkingCopy ? 3.0f : 1.0f);
    SolidBrush brush (brightColor);
    DrawShape (graphics, &pen, &brush, rect, shape);

    // overlay with some other color

    if (overlayColor.GetValue() != 0)
    {
        SolidBrush brush2 (overlayColor);
        DrawShape (graphics, &pen, &brush2, rect, shape);
    }
}

RectF CRevisionGraphWnd::TransformRectToScreen (const CRect& rect, const CSize& offset) const
{
    PointF leftTop ( rect.left * m_fZoomFactor
                   , rect.top * m_fZoomFactor);
	return RectF ( leftTop.X - offset.cx
                 , leftTop.Y - offset.cy
                 , rect.right * m_fZoomFactor - leftTop.X - 1
                 , rect.bottom * m_fZoomFactor - leftTop.Y);
}

RectF CRevisionGraphWnd::GetNodeRect (const ILayoutNodeList::SNode& node, const CSize& offset) const
{
    // get node and position

    RectF noderect (TransformRectToScreen (node.rect, offset));

    // show two separate lines for touching nodes, 
    // unless the scale is too small

    if (noderect.Height > 4.0f)
        noderect.Height -= 1.0f;

    // done

    return noderect;
}

RectF CRevisionGraphWnd::GetBranchCover 
    ( const ILayoutNodeList* nodeList
    , index_t nodeIndex
    , bool upward
    , const CSize& offset)
{
    // construct a rect that covers the respective branch

    CRect cover (0, 0, 0, 0);
    while (nodeIndex != NO_INDEX)
    {
        ILayoutNodeList::SNode node = nodeList->GetNode (nodeIndex);
        cover |= node.rect;

        const CVisibleGraphNode* nextNode = upward
            ? node.node->GetPrevious()
            : node.node->GetNext();

        nodeIndex = nextNode == NULL ? NO_INDEX : nextNode->GetIndex();
    }

    // expand it just a little to make it look nicer

    cover.InflateRect (10, 2, 10, 2);

    // and now, transfrom it

    return TransformRectToScreen (cover, offset);
}

void CRevisionGraphWnd::DrawShadows (Graphics& graphics, const CRect& logRect, const CSize& offset)
{
    // shadow color to use

    Color background;
    background.SetFromCOLORREF (GetSysColor(COLOR_WINDOW));
    Color textColor;
    textColor.SetFromCOLORREF (GetSysColor(COLOR_WINDOWTEXT));

    Color shadowColor = LimitedScaleColor (background, ARGB (Color::Black), 0.5f);

    // iterate over all visible nodes

    CSyncPointer<const ILayoutNodeList> nodes (m_state.GetNodes());
    for ( index_t index = nodes->GetFirstVisible (logRect)
        ; index != NO_INDEX
        ; index = nodes->GetNextVisible (index, logRect))
	{
        // get node and position

        ILayoutNodeList::SNode node = nodes->GetNode (index);
		RectF noderect (GetNodeRect (node, offset));

        // actual drawing

		switch (node.style)
		{
		case ILayoutNodeList::SNode::STYLE_DELETED:
		case ILayoutNodeList::SNode::STYLE_RENAMED:
			DrawShadow (graphics, noderect, shadowColor, TSVNOctangle);
			break;
		case ILayoutNodeList::SNode::STYLE_ADDED:
            DrawShadow(graphics, noderect, shadowColor, TSVNRoundRect);
            break;
		case ILayoutNodeList::SNode::STYLE_LAST:
			DrawShadow(graphics, noderect, shadowColor, TSVNEllipse);
			break;
		default:
            DrawShadow(graphics, noderect, shadowColor, TSVNRectangle);
			break;
		}
    }
}

void CRevisionGraphWnd::DrawSquare 
    ( Graphics& graphics
    , const PointF& leftTop
    , const Color& lightColor
    , const Color& darkColor
    , const Color& penColor)
{
    float squareSize = MARKER_SIZE * m_fZoomFactor;

    PointF leftBottom (leftTop.X, leftTop.Y + squareSize);
    RectF square (leftTop, SizeF (squareSize, squareSize));

    Pen pen (penColor, max (1, 1.5f * m_fZoomFactor));
    LinearGradientBrush lgBrush (leftTop, leftBottom, lightColor, darkColor);
    graphics.FillRectangle (&lgBrush, square);
    graphics.DrawRectangle (&pen, square);
}

void CRevisionGraphWnd::DrawGlyph 
    ( Graphics& graphics
    , Image* glyphs
    , const PointF& leftTop
    , GlyphType glyph
    , GlyphPosition position)
{
    // special case

    if (glyph == NoGlyph)
        return;

    // bitmap source area

    REAL x = ((REAL)position + (REAL)glyph) * GLYPH_SIZE;

    // screen target area

    float glyphSize = GLYPH_SIZE * m_fZoomFactor;
    RectF target (leftTop, SizeF (glyphSize, glyphSize));

    // scaled copy

    graphics.DrawImage ( glyphs
                       , target
                       , x, 0.0f, GLYPH_SIZE, GLYPH_SIZE
                       , UnitPixel, NULL, NULL, NULL);
}

void CRevisionGraphWnd::DrawGlyphs
    ( Graphics& graphics
    , Image* glyphs
    , const CVisibleGraphNode* node
    , const PointF& center
    , GlyphType glyph1
    , GlyphType glyph2
    , GlyphPosition position
    , DWORD state1
    , DWORD state2
    , bool showAll)
{
    // don't show collapse and cut glyths by default

    if (!showAll && ((glyph1 == CollapseGlyph) || (glyph1 == SplitGlyph)))
        glyph1 = NoGlyph;
    if (!showAll && ((glyph2 == CollapseGlyph) || (glyph2 == SplitGlyph)))
        glyph2 = NoGlyph;

    // glyth2 shall be set only if 2 glyphs are in use

    if (glyph1 == NoGlyph)
    {
        std::swap (glyph1, glyph2);
        std::swap (state1, state2);
    }

    // anything to do?

    if (glyph1 == NoGlyph)
        return;

    // 1 or 2 glyphs?

    CSyncPointer<CRevisionGraphState::TVisibleGlyphs> 
        visibleGlyphs (m_state.GetVisibleGlyphs());

    float squareSize = GLYPH_SIZE * m_fZoomFactor;
    if (glyph2 == NoGlyph)
    {
        PointF leftTop (center.X - 0.5f * squareSize, center.Y - 0.5f * squareSize);
        DrawGlyph (graphics, glyphs, leftTop, glyph1, position);
        visibleGlyphs->push_back 
            (CRevisionGraphState::SVisibleGlyph (state1, leftTop, node));
    }
    else
    {
        PointF leftTop1 (center.X - squareSize, center.Y - 0.5f * squareSize);
        DrawGlyph (graphics, glyphs, leftTop1, glyph1, position);
        visibleGlyphs->push_back 
            (CRevisionGraphState::SVisibleGlyph (state1, leftTop1, node));

        PointF leftTop2 (center.X, center.Y - 0.5f * squareSize);
        DrawGlyph (graphics, glyphs, leftTop2, glyph2, position);
        visibleGlyphs->push_back 
            (CRevisionGraphState::SVisibleGlyph (state2, leftTop2, node));
    }
}

void CRevisionGraphWnd::DrawGlyphs
    ( Graphics& graphics
    , Image* glyphs
    , const CVisibleGraphNode* node
    , const RectF& nodeRect
    , DWORD state
    , DWORD allowed
    , bool upsideDown)
{
    // shortcut

    if ((state == 0) && (allowed == 0))
        return;

    // draw all glyphs

    PointF topCenter (0.5f * nodeRect.GetLeft() + 0.5f * nodeRect.GetRight(), nodeRect.GetTop());
    PointF rightCenter (nodeRect.GetRight(), 0.5f * nodeRect.GetTop() + 0.5f * nodeRect.GetBottom());
    PointF bottomCenter (0.5f * nodeRect.GetLeft() + 0.5f * nodeRect.GetRight(), nodeRect.GetBottom());

    DrawGlyphs ( graphics
               , glyphs
               , node
               , upsideDown ? bottomCenter : topCenter
               , (state & CGraphNodeStates::COLLAPSED_ABOVE) ? ExpandGlyph : CollapseGlyph
               , (state & CGraphNodeStates::SPLIT_ABOVE) ? JoinGlyph : SplitGlyph
               , upsideDown ? Below : Above
               , CGraphNodeStates::COLLAPSED_ABOVE
               , CGraphNodeStates::SPLIT_ABOVE
               , (allowed & CGraphNodeStates::COLLAPSED_ABOVE) != 0);

    DrawGlyphs ( graphics
               , glyphs
               , node
               , rightCenter
               , (state & CGraphNodeStates::COLLAPSED_RIGHT) ? ExpandGlyph : CollapseGlyph
               , (state & CGraphNodeStates::SPLIT_RIGHT) ? JoinGlyph : SplitGlyph
               , Right
               , CGraphNodeStates::COLLAPSED_RIGHT
               , CGraphNodeStates::SPLIT_RIGHT
               , (allowed & CGraphNodeStates::COLLAPSED_RIGHT) != 0);

    DrawGlyphs ( graphics
               , glyphs
               , node
               , upsideDown ? topCenter : bottomCenter
               , (state & CGraphNodeStates::COLLAPSED_BELOW) ? ExpandGlyph : CollapseGlyph
               , (state & CGraphNodeStates::SPLIT_BELOW) ? JoinGlyph : SplitGlyph
               , upsideDown ? Above : Below
               , CGraphNodeStates::COLLAPSED_BELOW
               , CGraphNodeStates::SPLIT_BELOW
               , (allowed & CGraphNodeStates::COLLAPSED_BELOW) != 0);
}

void CRevisionGraphWnd::IndicateGlyphDirection
    ( Graphics& graphics
    , const ILayoutNodeList* nodeList
    , const ILayoutNodeList::SNode& node
    , const RectF& nodeRect
    , DWORD glyphs
    , bool upsideDown
    , const CSize& offset)
{
    // shortcut

    if (glyphs == 0)
        return;

    // where to place the indication?

    bool indicateAbove = (glyphs & CGraphNodeStates::COLLAPSED_ABOVE) != 0;
    bool indicateRight = (glyphs & CGraphNodeStates::COLLAPSED_RIGHT) != 0;
    bool indicateBelow = (glyphs & CGraphNodeStates::COLLAPSED_BELOW) != 0;

    // fill indication area a semi-transparent blend of red 
    // and the background color

    Color color;
    color.SetFromCOLORREF (GetSysColor(COLOR_WINDOW));
    color.SetValue ((color.GetValue() & 0x807f7f7f) + 0x800000);

    SolidBrush brush (color);

    // draw the indication (only one condition should match)

    RectF glyphCenter = indicateAbove ^ upsideDown
        ? RectF (nodeRect.X, nodeRect.Y - 1.0f, 0.0f, 0.0f)
        : RectF (nodeRect.X, nodeRect.GetBottom() - 1.0f, 0.0f, 0.0f);

    if (indicateAbove)
    {
        const CVisibleGraphNode* firstAffected = node.node->GetSource();

        assert (firstAffected);
        RectF branchCover 
            = GetBranchCover (nodeList, firstAffected->GetIndex(), true, offset);
        RectF::Union (branchCover, branchCover, glyphCenter);

        graphics.FillRectangle (&brush, branchCover);
    }

    if (indicateRight)
    {
        for ( const CVisibleGraphNode::CCopyTarget* branch 
                = node.node->GetFirstCopyTarget()
            ; branch != NULL
            ; branch = branch->next())
        {
            RectF branchCover 
                = GetBranchCover (nodeList, branch->value()->GetIndex(), false, offset);
            graphics.FillRectangle (&brush, branchCover);
        }
    }

    if (indicateBelow)
    {
        const CVisibleGraphNode* firstAffected 
            = node.node->GetNext();

        RectF branchCover 
            = GetBranchCover (nodeList, firstAffected->GetIndex(), false, offset);
        RectF::Union (branchCover, branchCover, glyphCenter);

        graphics.FillRectangle (&brush, branchCover);
    }
}

void CRevisionGraphWnd::DrawMarker 
    ( Graphics& graphics
    , const RectF& noderect
    , MarkerPosition position
    , int relPosition
    , int colorIndex )
{
	// marker size
    float squareSize = MARKER_SIZE * m_fZoomFactor;
    float squareDist = min ( (noderect.Height - squareSize) / 2
                           , squareSize / 2);

    // position

    REAL offset = squareSize * (0.75f + relPosition);
    REAL left = position == mpRight
              ? noderect.GetRight() - offset - squareSize 
              : noderect.GetLeft() + offset;
    PointF leftTop (left, noderect.Y + squareDist);

    // color

    Color lightColor (m_Colors.GetColor (CColors::ctMarkers, colorIndex));
    Color darkColor (Darken (lightColor));
    Color borderColor (0x80000000);

    // draw it

    DrawSquare (graphics, leftTop, lightColor, darkColor, borderColor);
}

void CRevisionGraphWnd::DrawStripes (Graphics& graphics, const CSize& offset)
{
    // we need to fill this visible area of the the screen 
    // (even if there is graph in that part)

    RectF clipRect;
    graphics.GetVisibleClipBounds (&clipRect);

    // don't show stripes if we don't have mutiple roots

    CSyncPointer<const ILayoutRectList> trees (m_state.GetTrees());
    if (trees->GetCount() < 2)
        return;

    // iterate over all trees

    for ( index_t i = 0, count = trees->GetCount(); i < count; ++i)
	{
        // screen coordinates coverd by the tree

        CRect tree = trees->GetRect(i);
        REAL left = tree.left * m_fZoomFactor;
        REAL right = tree.right * m_fZoomFactor;
	    RectF rect ( left - offset.cx
                   , clipRect.Y
                   , i+1 == count ? clipRect.Width : right - left
                   , clipRect.Height);

        // relevant?

        if (rect.IntersectsWith (clipRect))
        {
            // draw the background stripe

            Color color (  (i & 1) == 0 
                         ? m_Colors.GetColor (CColors::gdpStripeColor1) 
                         : m_Colors.GetColor (CColors::gdpStripeColor2));
            SolidBrush brush (color);
            graphics.FillRectangle (&brush, rect);
        }
    }
}

void CRevisionGraphWnd::DrawNodes (Graphics& graphics, Image* glyphs, const CRect& logRect, const CSize& offset)
{
    CSyncPointer<CGraphNodeStates> nodeStates (m_state.GetNodeStates());
    CSyncPointer<const ILayoutNodeList> nodes (m_state.GetNodes());

    bool upsideDown 
        = m_state.GetOptions()->GetOption<CUpsideDownLayout>()->IsActive();

    // iterate over all visible nodes

    for ( index_t index = nodes->GetFirstVisible (logRect)
        ; index != NO_INDEX
        ; index = nodes->GetNextVisible (index, logRect))
	{
        // get node and position

        ILayoutNodeList::SNode node = nodes->GetNode (index);
		RectF noderect (GetNodeRect (node, offset));

        // actual drawing

        Color transparent (0);
        Color overlayColor = transparent;

		switch (node.style)
		{
		case ILayoutNodeList::SNode::STYLE_DELETED:
			DrawNode(graphics, noderect, m_Colors.GetColor(CColors::gdpDeletedNode), transparent, node.node, TSVNOctangle);
			break;

        case ILayoutNodeList::SNode::STYLE_ADDED:
            if (m_bTweakTagsColors && node.node->GetClassification().Is (CNodeClassification::IS_TAG))
                overlayColor = m_Colors.GetColor(CColors::gdpTagOverlay);
            else if (m_bTweakTrunkColors && node.node->GetClassification().Is (CNodeClassification::IS_TRUNK))
                overlayColor = m_Colors.GetColor(CColors::gdpTrunkOverlay);
            DrawNode(graphics, noderect, m_Colors.GetColor(CColors::gdpAddedNode), overlayColor, node.node, TSVNRoundRect);
            break;

        case ILayoutNodeList::SNode::STYLE_RENAMED:
            DrawNode(graphics, noderect, m_Colors.GetColor(CColors::gdpRenamedNode), overlayColor, node.node, TSVNOctangle);
			break;

        case ILayoutNodeList::SNode::STYLE_LAST:
			DrawNode(graphics, noderect, m_Colors.GetColor(CColors::gdpLastCommitNode), transparent, node.node, TSVNEllipse);
			break;

        case ILayoutNodeList::SNode::STYLE_MODIFIED:
			DrawNode(graphics, noderect, m_Colors.GetColor(CColors::gdpModifiedNode), transparent, node.node, TSVNRectangle);
			break;

        case ILayoutNodeList::SNode::STYLE_MODIFIED_WC:
			DrawNode(graphics, noderect, m_Colors.GetColor(CColors::gdpWCNode), transparent, node.node, TSVNEllipse);
			break;

        default:
            DrawNode(graphics, noderect, m_Colors.GetColor(CColors::gdpUnchangedNode), transparent, node.node, TSVNRectangle);
			break;
		}

    	// Draw the "tagged" icon

        if (node.node->GetFirstTag() != NULL)
            DrawMarker (graphics, noderect, mpRight, 0, 0);

        if ((m_SelectedEntry1 == node.node) || (m_SelectedEntry2 == node.node))
            DrawMarker (graphics, noderect, mpLeft, 0, 1);

        // expansion glypths etc.

        DrawGlyphs (graphics, glyphs, node.node, noderect, nodeStates->GetFlags (node.node), 0, upsideDown);
    }
}

void CRevisionGraphWnd::DrawConnections (CDC* pDC, const CRect& logRect, const CSize& offset)
{
    enum {MAX_POINTS = 100};
    CPoint points[MAX_POINTS];

	CPen newpen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
	CPen * pOldPen = pDC->SelectObject(&newpen);

    // iterate over all visible lines

    CSyncPointer<const ILayoutConnectionList> connections (m_state.GetConnections());
    for ( index_t index = connections->GetFirstVisible (logRect)
        ; index != NO_INDEX
        ; index = connections->GetNextVisible (index, logRect))
	{
        // get connection and point position

        ILayoutConnectionList::SConnection connection 
            = connections->GetConnection (index);

        if (connection.numberOfPoints > MAX_POINTS)
            continue;

        for (index_t i = 0; i < connection.numberOfPoints; ++i)
        {
            points[i].x = (int)(connection.points[i].x * m_fZoomFactor) - offset.cx;
            points[i].y = (int)(connection.points[i].y * m_fZoomFactor) - offset.cy;
        }

		// draw the connection

		pDC->PolyBezier (points, connection.numberOfPoints);
	}

	pDC->SelectObject(pOldPen);
}

void CRevisionGraphWnd::DrawTexts (CDC* pDC, const CRect& logRect, const CSize& offset)
{
	COLORREF standardTextColor = GetSysColor(COLOR_WINDOWTEXT);
    if (m_nFontSize <= 0)
        return;

    // iterate over all visible nodes

    pDC->SetTextAlign (TA_CENTER | TA_TOP);
    CSyncPointer<const ILayoutTextList> texts (m_state.GetTexts());
    for ( index_t index = texts->GetFirstVisible (logRect)
        ; index != NO_INDEX
        ; index = texts->GetNextVisible (index, logRect))
	{
        // get node and position

        ILayoutTextList::SText text = texts->GetText (index);
		CRect textRect ( (int)(text.rect.left * m_fZoomFactor) - offset.cx
                       , (int)(text.rect.top * m_fZoomFactor) - offset.cy
                       , (int)(text.rect.right * m_fZoomFactor) - offset.cx
                       , (int)(text.rect.bottom * m_fZoomFactor) - offset.cy);

		// draw the revision text

		pDC->SetTextColor (text.style == ILayoutTextList::SText::STYLE_WARNING
                            ? m_Colors.GetColor (CColors::gdpWCNodeBorder).ToCOLORREF()
                            : standardTextColor );
        pDC->SelectObject (GetFont (FALSE, text.style != ILayoutTextList::SText::STYLE_DEFAULT));
        pDC->ExtTextOut ((textRect.left + textRect.right)/2, textRect.top, 0, &textRect, text.text, NULL);
    }
}

void CRevisionGraphWnd::DrawCurrentNodeGlyphs (Graphics& graphics, Image* glyphs, const CSize& offset)
{
    CSyncPointer<const ILayoutNodeList> nodeList (m_state.GetNodes());
    bool upsideDown 
        = m_state.GetOptions()->GetOption<CUpsideDownLayout>()->IsActive();

    // don't draw glyphs if we are outside the client area 
    // (e.g. within a scrollbar)

    CPoint point;
    GetCursorPos (&point);
    ScreenToClient (&point);
    if (!GetClientRect().PtInRect (point))
        return;

    // expansion glypths etc.

    m_hoverIndex = GetHitNode (point);
    m_hoverGlyphs = GetHoverGlyphs (point);

    if ((m_hoverIndex != NO_INDEX) || (m_hoverGlyphs != 0))
    {
        index_t nodeIndex = m_hoverIndex == NO_INDEX
                          ? GetHitNode (point, CSize (GLYPH_SIZE, GLYPH_SIZE / 2))
                          : m_hoverIndex;

        if (nodeIndex >= nodeList->GetCount())
            return;

        ILayoutNodeList::SNode node = nodeList->GetNode (nodeIndex);
        RectF noderect (GetNodeRect (node, offset));

        DWORD flags = m_state.GetNodeStates()->GetFlags (node.node);

        IndicateGlyphDirection (graphics, nodeList.get(), node, noderect, m_hoverGlyphs, upsideDown, offset);
        DrawGlyphs (graphics, glyphs, node.node, noderect, flags, m_hoverGlyphs, upsideDown);
    }
}

void CRevisionGraphWnd::DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
    CMemDC* memDC = NULL;
    if (!bDirectDraw)
    {
        memDC = new CMemDC (*pDC, rect);
        pDC = &memDC->GetDC();
    }
	
	pDC->FillSolidRect(rect, GetSysColor(COLOR_WINDOW));
	pDC->SetBkMode(TRANSPARENT);

    // preparation & sync

    CSyncPointer<CAllRevisionGraphOptions> options (m_state.GetOptions());
    ClearVisibleGlyphs (rect);

    // transform visible

    CSize offset (nHScrollPos, nVScrollPos);
    CRect logRect ( (int)(offset.cx / m_fZoomFactor)-1
                  , (int)(offset.cy / m_fZoomFactor)-1
                  , (int)((rect.Width() + offset.cx) / m_fZoomFactor) + 1
                  , (int)((rect.Height() + offset.cy) / m_fZoomFactor) + 1);

    // draw the different components

    Graphics* graphics = Graphics::FromHDC(*pDC);
    graphics->SetPageUnit (UnitPixel);
    graphics->SetInterpolationMode (InterpolationModeHighQualityBicubic);

    if (options->GetOption<CShowTreeStripes>()->IsActive())
        DrawStripes (*graphics, offset);

    if (m_fZoomFactor > 0.2f)
        DrawShadows (*graphics, logRect, offset);

    Bitmap glyphs (AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHGLYPHS));

    DrawNodes (*graphics, &glyphs, logRect, offset);
    DrawConnections (pDC, logRect, offset);
    DrawTexts (pDC, logRect, offset);

    if (m_showHoverGlyphs)
        DrawCurrentNodeGlyphs (*graphics, &glyphs, offset);

    // draw preview

	if ((!bDirectDraw)&&(m_Preview.GetSafeHandle())&&(m_bShowOverview))
	{
		// draw the overview image rectangle in the top right corner
		CMyMemDC memDC2(pDC, true);
		memDC2.SetWindowOrg(0, 0);
		HBITMAP oldhbm = (HBITMAP)memDC2.SelectObject(&m_Preview);
		pDC->BitBlt(rect.Width()-m_previewWidth, 0, m_previewWidth, m_previewHeight, 
			&memDC2, 0, 0, SRCCOPY);
		memDC2.SelectObject(oldhbm);
		// draw the border for the overview rectangle
		m_OverviewRect.left = rect.Width()-m_previewWidth;
		m_OverviewRect.top = 0;
		m_OverviewRect.right = rect.Width();
		m_OverviewRect.bottom = m_previewHeight;
		pDC->DrawEdge(&m_OverviewRect, EDGE_BUMP, BF_RECT);
		// now draw a rectangle where the current view is located in the overview

        CRect viewRect = GetViewRect();
        LONG width = (long)(rect.Width() * m_previewZoom / m_fZoomFactor);
        LONG height = (long)(rect.Height() * m_previewZoom / m_fZoomFactor);
		LONG xpos = (long)(nHScrollPos * m_previewZoom / m_fZoomFactor);
		LONG ypos = (long)(nVScrollPos * m_previewZoom / m_fZoomFactor);
		RECT tempRect;
		tempRect.left = rect.Width()-m_previewWidth+xpos;
		tempRect.top = ypos;
		tempRect.right = tempRect.left + width;
		tempRect.bottom = tempRect.top + height;
		// make sure the position rect is not bigger than the preview window itself
		::IntersectRect(&m_OverviewPosRect, &m_OverviewRect, &tempRect);

        RectF rect2 ( (float)m_OverviewPosRect.left, (float)m_OverviewPosRect.top
                   , (float)m_OverviewPosRect.Width(), (float)m_OverviewPosRect.Height());
        SolidBrush brush (Color (64, 0, 0, 0));
        graphics->FillRectangle (&brush, rect2);
		pDC->DrawEdge(&m_OverviewPosRect, EDGE_BUMP, BF_RECT);
	}

    // flush changes to screen

    if (graphics)
        delete graphics;

	if (memDC)
		delete memDC;
}

void CRevisionGraphWnd::DrawRubberBand()
{
	CDC * pDC = GetDC();
	pDC->SetROP2(R2_NOT);
	pDC->SelectObject(GetStockObject(NULL_BRUSH));
	pDC->Rectangle(min(m_ptRubberStart.x, m_ptRubberEnd.x), min(m_ptRubberStart.y, m_ptRubberEnd.y), 
		max(m_ptRubberStart.x, m_ptRubberEnd.x), max(m_ptRubberStart.y, m_ptRubberEnd.y));
	ReleaseDC(pDC);
}

