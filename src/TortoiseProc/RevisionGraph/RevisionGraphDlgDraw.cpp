// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2011 - TortoiseSVN
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
#include "MyMemDC.h"
#include "Revisiongraphdlg.h"
#include "MessageBox.h"
#include "Git.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TGitPath.h"
//#include "SVNInfo.h"
//#include "SVNDiff.h"
#include ".\revisiongraphwnd.h"
//#include "IRevisionGraphLayout.h"
//#include "UpsideDownLayout.h"
//#include "ShowTreeStripes.h"
#include "registry.h"
#include "UnicodeUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;
using namespace ogdf;

/************************************************************************/
/* Graphing functions													*/
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
		_tcsncpy_s(m_lfBaseFont.lfFaceName, _T("MS Shell Dlg 2"), 32);
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

	if (IsUpdateJobRunning())
	{
		CString fetch = CString(MAKEINTRESOURCE(IDS_PROC_LOADING));
		dc.FillSolidRect(rect, ::GetSysColor(COLOR_APPWORKSPACE));
		dc.ExtTextOut(20,20,ETO_CLIPPED,NULL,fetch,NULL);
		CWnd::OnPaint();
		return;

	}else if (this->m_Graph.empty())
	{
		CString sNoGraphText;
		sNoGraphText.LoadString(IDS_REVGRAPH_ERR_NOGRAPH);
		dc.FillSolidRect(rect, RGB(255,255,255));
		dc.ExtTextOut(20,20,ETO_CLIPPED,NULL,sNoGraphText,NULL);
		return;
	}

	GraphicsDevice dev;
	dev.pDC = &dc;
	DrawGraph(dev, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ), false);

}

void CRevisionGraphWnd::ClearVisibleGlyphs (const CRect& /*rect*/)
{
#if 0
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
#endif
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

void CRevisionGraphWnd::DrawRoundedRect (GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect, int mask)
{

	enum {POINT_COUNT = 8};

	float radius = CORNER_SIZE * m_fZoomFactor;
	PointF points[POINT_COUNT];
	CutawayPoints (rect, radius, points);

	if (graphics.graphics)
	{
		GraphicsPath path;

		if(mask & ROUND_UP)
		{
			path.AddArc (points[0].X, points[1].Y, radius, radius, 180, 90);
			path.AddArc (points[2].X, points[2].Y, radius, radius, 270, 90);
		}else
		{
			path.AddLine(points[0].X, points[1].Y, points[3].X, points[2].Y);
		}

		if(mask & ROUND_DOWN)
		{
			path.AddArc (points[5].X, points[4].Y, radius, radius, 0, 90);
			path.AddArc (points[7].X, points[7].Y, radius, radius, 90, 90);
		}else
		{
			path.AddLine(points[3].X, points[3].Y, points[4].X, points[5].Y);
			path.AddLine(points[4].X, points[5].Y, points[7].X, points[6].Y);
		}

		points[0].Y -= radius / 2;
		path.AddLine (points[7], points[0]);

		if (brush != NULL)
		{
			graphics.graphics->FillPath (brush, &path);
		}
		if (pen != NULL)
			graphics.graphics->DrawPath (pen, &path);
	}
	else if (graphics.pSVG)
	{
		graphics.pSVG->RoundedRectangle((int)rect.X, (int)rect.Y, (int)rect.Width, (int)rect.Height, penColor, penWidth, fillColor, (int)radius, mask);
	}

}

void CRevisionGraphWnd::DrawOctangle (GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect)
{
	enum {POINT_COUNT = 8};

	// show left & right edges of low boxes as "<===>"

	float minCutAway = min (CORNER_SIZE * m_fZoomFactor, rect.Height / 2);

	// larger boxes: remove 25% of the shorter side

	float suggestedCutAway = min (rect.Height, rect.Width) / 4;

	// use the more visible one of the former two

	PointF points[POINT_COUNT];
	CutawayPoints (rect, max (minCutAway, suggestedCutAway), points);

	// now, draw it

	if (graphics.graphics)
	{
		if (brush != NULL)
			graphics.graphics->FillPolygon (brush, points, POINT_COUNT);
		if (pen != NULL)
			graphics.graphics->DrawPolygon (pen, points, POINT_COUNT);
	}
	else if (graphics.pSVG)
	{
		graphics.pSVG->Polygon(points, POINT_COUNT, penColor, penWidth, fillColor);
	}
}



void CRevisionGraphWnd::DrawShape (GraphicsDevice& graphics, const Color& penColor, int penWidth, const Pen* pen, const Color& fillColor, const Brush* brush, const RectF& rect, NodeShape shape)
{
	switch( shape )
	{
	case TSVNRectangle:
		if (graphics.graphics)
		{
			if (brush != NULL)
				graphics.graphics->FillRectangle (brush, rect);
			if (pen != NULL)
				graphics.graphics->DrawRectangle (pen, rect);
		}
		else if (graphics.pSVG)
		{
			graphics.pSVG->RoundedRectangle((int)rect.X, (int)rect.Y, (int)rect.Width, (int)rect.Height, penColor, penWidth, fillColor);
		}
		break;
	case TSVNRoundRect:
		DrawRoundedRect (graphics, penColor, penWidth, pen, fillColor, brush, rect);
		break;
	case TSVNOctangle:
		DrawOctangle (graphics, penColor, penWidth, pen, fillColor, brush, rect);
		break;
	case TSVNEllipse:
		if (graphics.graphics)
		{
			if (brush != NULL)
				graphics.graphics->FillEllipse (brush, rect);
			if (pen != NULL)
				graphics.graphics->DrawEllipse(pen, rect);
		}
		else if (graphics.pSVG)
			graphics.pSVG->Ellipse((int)rect.X, (int)rect.Y, (int)rect.Width, (int)rect.Height, penColor, penWidth, fillColor);
		break;
	default:
		ASSERT(FALSE);  //unknown type
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

#if 0
void CRevisionGraphWnd::DrawShadow (GraphicsDevice& graphics, const RectF& rect,
									Color shadowColor, NodeShape shape)
{
	// draw the shadow

	RectF shadow = rect;
	shadow.Offset (2, 2);

	Pen pen (shadowColor);
	SolidBrush brush (shadowColor);

	DrawShape (graphics, shadowColor, 1, &pen, shadowColor, &brush, shadow, shape);
}
#endif

RectF CRevisionGraphWnd::TransformRectToScreen (const CRect& rect, const CSize& offset) const
{
	PointF leftTop ( rect.left * m_fZoomFactor
					, rect.top * m_fZoomFactor);
	return RectF ( leftTop.X - offset.cx
				, leftTop.Y - offset.cy
				, rect.right * m_fZoomFactor - leftTop.X - 1
				, rect.bottom * m_fZoomFactor - leftTop.Y);
}


RectF CRevisionGraphWnd::GetNodeRect (const node& node, const CSize& offset) const
{
	// get node and position

	CRect rect;
	rect.left = (int) (this->m_GraphAttr.x(node) -  m_GraphAttr.width(node)/2);
	rect.top = (int) (this->m_GraphAttr.y(node) - m_GraphAttr.height(node)/2);
	rect.bottom = (int)( rect.top+ m_GraphAttr.height(node));
	rect.right = (int)(rect.left + m_GraphAttr.width(node));

	RectF noderect (TransformRectToScreen (rect, offset));

	// show two separate lines for touching nodes,
	// unless the scale is too small

	if (noderect.Height > 15.0f)
		noderect.Height -= 1.0f;

	// done

	return noderect;
}


#if 0
RectF CRev
isionGraphWnd::GetBranchCover
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
#endif

#if 0
void CRevisionGraphWnd::DrawShadows (GraphicsDevice& graphics, const CRect& logRect, const CSize& offset)
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
#endif


void CRevisionGraphWnd::DrawSquare
	( GraphicsDevice& graphics
	, const PointF& leftTop
	, const Color& lightColor
	, const Color& darkColor
	, const Color& penColor)
{
	float squareSize = MARKER_SIZE * m_fZoomFactor;

	PointF leftBottom (leftTop.X, leftTop.Y + squareSize);
	RectF square (leftTop, SizeF (squareSize, squareSize));

	if (graphics.graphics)
	{
		LinearGradientBrush lgBrush (leftTop, leftBottom, lightColor, darkColor);
		graphics.graphics->FillRectangle (&lgBrush, square);
		if (squareSize > 4.0f)
		{
			Pen pen (penColor);
			graphics.graphics->DrawRectangle (&pen, square);
		}
	}
	else if (graphics.pSVG)
	{
		graphics.pSVG->GradientRectangle((int)square.X, (int)square.Y, (int)square.Width, (int)square.Height,
										lightColor, darkColor, penColor);
	}
}

#if 0
void CRevisionGraphWnd::DrawGlyph
	( GraphicsDevice& graphics
	, Image* glyphs
	, const PointF& leftTop
	, GlyphType glyph
	, GlyphPosition position)
{
	// special case

	if (glyph == NoGlyph)
		return;

	// bitmap source area

	REAL x = ((REAL)position + (REAL)glyph) * GLYPH_BITMAP_SIZE;

	// screen target area

	float glyphSize = GLYPH_SIZE * m_fZoomFactor;
	RectF target (leftTop, SizeF (glyphSize, glyphSize));

	// scaled copy

	if (graphics.graphics)
	{
		graphics.graphics->DrawImage ( glyphs
			, target
			, x, 0.0f, GLYPH_BITMAP_SIZE, GLYPH_BITMAP_SIZE
			, UnitPixel, NULL, NULL, NULL);
	}
	else if (graphics.pSVG)
	{
		// instead of inserting a bitmap, draw a
		// round rectangle instead.
		// Embedding images would blow up the resulting
		// svg file a lot, and the round rectangle
		// is enough IMHO.
		// Note:
		// images could be embedded like this:
		// <image y="100" x="100" id="imgId1234" xlink:href="data:image/png;base64,...base64endodeddata..." height="16" width="16" />

		graphics.pSVG->RoundedRectangle((int)target.X, (int)target.Y, (int)target.Width, (int)target.Height,
										Color(0,0,0), 2, Color(255,255,255), (int)(target.Width/3.0));
	}
}
#endif

#if 0
void CRevisionGraphWnd::DrawGlyphs
	( GraphicsDevice& graphics
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
#endif

#if 0
void CRevisionGraphWnd::DrawGlyphs
	( GraphicsDevice& graphics
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
#endif

#if 0
void CRevisionGraphWnd::IndicateGlyphDirection
	( GraphicsDevice& graphics
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

		if (graphics.graphics)
			graphics.graphics->FillRectangle (&brush, branchCover);
		else if (graphics.pSVG)
			graphics.pSVG->RoundedRectangle((int)branchCover.X, (int)branchCover.Y, (int)branchCover.Width, (int)branchCover.Height,
											color, 1, color);
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
			if (graphics.graphics)
				graphics.graphics->FillRectangle (&brush, branchCover);
			else if (graphics.pSVG)
				graphics.pSVG->RoundedRectangle((int)branchCover.X, (int)branchCover.Y, (int)branchCover.Width, (int)branchCover.Height,
												color, 1, color);
		}
	}

	if (indicateBelow)
	{
		const CVisibleGraphNode* firstAffected
			= node.node->GetNext();

		RectF branchCover
			= GetBranchCover (nodeList, firstAffected->GetIndex(), false, offset);
		RectF::Union (branchCover, branchCover, glyphCenter);

		if (graphics.graphics)
			graphics.graphics->FillRectangle (&brush, branchCover);
		else if (graphics.pSVG)
			graphics.pSVG->RoundedRectangle((int)branchCover.X, (int)branchCover.Y, (int)branchCover.Width, (int)branchCover.Height,
											color, 1, color);
	}
}

#endif

void CRevisionGraphWnd::DrawMarker
	( GraphicsDevice& graphics
	, const RectF& noderect
	, MarkerPosition /*position*/
	, int /*relPosition*/
	, const Color& penColor
	, int num)
{
	REAL width = 4*this->m_fZoomFactor<1? 1: 4*this->m_fZoomFactor;
	Pen pen(penColor,width);
	DrawRoundedRect(graphics, penColor, (int)width, &pen, Color(0,0,0), NULL, noderect);
	if (num == 1)
	{
		// Roman number 1
		REAL x = max(1, 10 * this->m_fZoomFactor);
		REAL y1 = max(1, 25 * this->m_fZoomFactor);
		REAL y2 = max(1, 5 * this->m_fZoomFactor);
		if(graphics.graphics)
			graphics.graphics->DrawLine(&pen, noderect.X + x, noderect.Y - y1, noderect.X + x, noderect.Y - y2);
	}
	else if (num == 2)
	{
		// Roman number 2
		REAL x1 = max(1, 5 * this->m_fZoomFactor);
		REAL x2 = max(1, 15 * this->m_fZoomFactor);
		REAL y1 = max(1, 25 * this->m_fZoomFactor);
		REAL y2 = max(1, 5 * this->m_fZoomFactor);
		if(graphics.graphics)
		{
			graphics.graphics->DrawLine(&pen, noderect.X + x1, noderect.Y - y1, noderect.X + x1, noderect.Y - y2);
			graphics.graphics->DrawLine(&pen, noderect.X + x2, noderect.Y - y1, noderect.X + x2, noderect.Y - y2);
		}
	}
}

#if 0
void CRevisionGraphWnd::DrawStripes (GraphicsDevice& graphics, const CSize& offset)
{
	// we need to fill this visible area of the the screen
	// (even if there is graph in that part)

	RectF clipRect;
	if (graphics.graphics)
		graphics.graphics->GetVisibleClipBounds (&clipRect);

	// don't show stripes if we don't have multiple roots

	CSyncPointer<const ILayoutRectList> trees (m_state.GetTrees());
	if (trees->GetCount() < 2)
		return;

	// iterate over all trees

	for ( index_t i = 0, count = trees->GetCount(); i < count; ++i)
	{
		// screen coordinates covered by the tree

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
			if (graphics.graphics)
			{
				SolidBrush brush (color);
				graphics.graphics->FillRectangle (&brush, rect);
			}
			else if (graphics.pSVG)
				graphics.pSVG->RoundedRectangle((int)rect.X, (int)rect.Y, (int)rect.Width, (int)rect.Height,
												color, 1, color);
		}
	}
}
#endif

PointF CRevisionGraphWnd::cutPoint(node v,double lw,PointF ps, PointF pt)
{
	double x = m_GraphAttr.x(v);
	double y = m_GraphAttr.y(v);
	double xmin = x - this->m_GraphAttr.width(v)/2 - lw/2;
	double xmax = x + this->m_GraphAttr.width(v)/2 + lw/2;
	double ymin = y - this->m_GraphAttr.height(v)/2 - lw/2;
	double ymax = y + this->m_GraphAttr.height(v)/2 + lw/2;;

	double dx = pt.X - ps.X;
	double dy = pt.Y - ps.Y;

	if(dy != 0) {
		// below
		if(pt.Y > ymax) {
			double t = (ymax-ps.Y) / dy;
			double x = ps.X + t*dx;

			if(xmin <= x && x <= xmax)
				return PointF((REAL)x, (REAL)ymax);

		// above
		} else if(pt.Y < ymin) {
			double t = (ymin-ps.Y) / dy;
			double x = ps.X + t*dx;

			if(xmin <= x && x <= xmax)
				return PointF((REAL)x, (REAL)ymin);

		}
	}

	if(dx != 0) {
		// right
		if(pt.X > xmax) {
			double t = (xmax-ps.X) / dx;
			double y = ps.Y + t*dy;

			if(ymin <= y && y <= ymax)
				return PointF((REAL)xmax, (REAL)y);

		// left
		} else if(pt.X < xmin) {
			double t = (xmin-ps.X) / dx;
			double y = ps.Y + t*dy;

			if(ymin <= y && y <= ymax)
				return PointF((REAL)xmin, (REAL)y);

		}
	}

	return pt;

}

void CRevisionGraphWnd::DrawConnections (GraphicsDevice& graphics, const CRect& /*logRect*/, const CSize& offset)
{

	CArray<PointF> points;
	CArray<CPoint> pts;

	if(graphics.graphics)
		graphics.graphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);

	float penwidth = 2*m_fZoomFactor<1? 1:2*m_fZoomFactor;
	Gdiplus::Pen pen(Color(0,0,0),penwidth);

	// iterate over all visible lines
	edge e;
	forall_edges(e, m_Graph)
	{
		// get connection and point position
		const DPolyline &dpl = this->m_GraphAttr.bends(e);

		points.RemoveAll();
		pts.RemoveAll();

		PointF pt;
		pt.X = (REAL)m_GraphAttr.x(e->source());
		pt.Y = (REAL)m_GraphAttr.y(e->source());

		points.Add(pt);

		ListConstIterator<DPoint> it;
		for(it = dpl.begin(); it.valid(); ++it)
		{
			pt.X =  (REAL)(*it).m_x;
			pt.Y =  (REAL)(*it).m_y;
			points.Add(pt);
		}

		pt.X = (REAL)m_GraphAttr.x(e->target());
		pt.Y = (REAL)m_GraphAttr.y(e->target());

		points.Add(pt);

		points[0] = this->cutPoint(e->source(), 1, points[0], points[1]);
		points[points.GetCount()-1] =  this->cutPoint(e->target(), 1, points[points.GetCount()-1], points[points.GetCount()-2]);
		// draw the connection

		for(int i=0; i < points.GetCount(); i++)
		{
			//CPoint pt;
			points[i].X = points[i].X * this->m_fZoomFactor - offset.cx;
			points[i].Y = points[i].Y * this->m_fZoomFactor - offset.cy;
			//pts.Add(pt);
		}

		if (graphics.graphics)
		{
			graphics.graphics->DrawLines(&pen, points.GetData(), (INT)points.GetCount());

		}
		else if (graphics.pSVG)
		{
			Color color;
			color.SetFromCOLORREF(GetSysColor(COLOR_WINDOWTEXT));
			graphics.pSVG->Polyline(points.GetData(), (int)points.GetCount(), Color(0,0,0), (int)penwidth);
		}

		//draw arrow
		double dx = points[1].X - points[0].X;
		double dy = points[1].Y - points[0].Y;

		double len = sqrt(dx*dx + dy*dy);
		dx = m_ArrowSize * m_fZoomFactor *dx /len;
		dy = m_ArrowSize * m_fZoomFactor *dy /len;

		double p1_x, p1_y, p2_x, p2_y;
		p1_x = dx * m_ArrowCos - dy * m_ArrowSin;
		p1_y = dx * m_ArrowSin + dy * m_ArrowCos;

		p2_x = dx * m_ArrowCos + dy * m_ArrowSin;
		p2_y = -dx * m_ArrowSin + dy * m_ArrowCos;

		//graphics.graphics->DrawLine(&pen, points[0].X,points[0].Y, points[0].X +p1_x,points[0].Y+p1_y);
		//graphics.graphics->DrawLine(&pen, points[0].X,points[0].Y, points[0].X +p2_x,points[0].Y+p2_y);
		GraphicsPath path;

		PointF arrows[5];
		arrows[0].X =  points[0].X;
		arrows[0].Y =  points[0].Y;

		arrows[1].X =  points[0].X + (REAL)p1_x;
		arrows[1].Y =  points[0].Y + (REAL)p1_y;

		arrows[2].X =  points[0].X + (REAL)dx*3/5;
		arrows[2].Y =  points[0].Y + (REAL)dy*3/5;

		arrows[3].X =  points[0].X + (REAL)p2_x;
		arrows[3].Y =  points[0].Y + (REAL)p2_y;

		arrows[4].X =  points[0].X;
		arrows[4].Y =  points[0].Y;

		path.AddLines(arrows, 5);
		path.SetFillMode(FillModeAlternate);
		if(graphics.graphics)
		{
			graphics.graphics->DrawPath(&pen, &path);
		}else if(graphics.pSVG)
		{
			graphics.pSVG->DrawPath(arrows, 5, Color(0,0,0), (int)penwidth, Color(0,0,0));
		}
	}
}



void CRevisionGraphWnd::DrawTexts (GraphicsDevice& graphics, const CRect& /*logRect*/, const CSize& offset)
{
	//COLORREF standardTextColor = GetSysColor(COLOR_WINDOWTEXT);
	if (m_nFontSize <= 0)
		return;

	// iterate over all visible nodes

	if (graphics.pDC)
		graphics.pDC->SetTextAlign (TA_CENTER | TA_TOP);


	CString fontname = CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New"));

	Gdiplus::Font font(fontname.GetBuffer(),(REAL)m_nFontSize,FontStyleRegular);
	SolidBrush blackbrush(Color::Black);

	node v;
	forall_nodes(v,m_Graph)
	{
		// get node and position

		String label=this->m_GraphAttr.labelNode(v);

		RectF noderect (GetNodeRect (v, offset));

		// draw the revision text
		CGitHash hash = this->m_logEntries[v->index()];
		double hight = noderect.Height / (m_HashMap[hash].size()?m_HashMap[hash].size():1);

		if(m_HashMap.find(hash) == m_HashMap.end() || m_HashMap[hash].size() == 0)
		{
			Color background;
			background.SetFromCOLORREF (GetSysColor(COLOR_WINDOW));
			Gdiplus::Pen pen(background,1.0F);
			Color brightColor = LimitedScaleColor (background, RGB(255,0,0), 0.9f);
			Gdiplus::SolidBrush brush(brightColor);

			DrawRoundedRect(graphics, background,1, &pen, brightColor, &brush, noderect);

			if(graphics.graphics)
			{
				graphics.graphics->DrawString(hash.ToString().Left(g_Git.GetShortHASHLength()),-1,
					&font,
					Gdiplus::PointF(noderect.X + this->GetLeftRightMargin()*this->m_fZoomFactor,noderect.Y+this->GetTopBottomMargin()*m_fZoomFactor),
					&blackbrush);
			}
			if(graphics.pSVG)
			{
				graphics.pSVG->Text((int)(noderect.X + this->GetLeftRightMargin() * this->m_fZoomFactor),
											(int)(noderect.Y + this->GetTopBottomMargin() * m_fZoomFactor + m_nFontSize),
											CUnicodeUtils::GetUTF8(fontname), m_nFontSize, false, false, Color::Black,
											CUnicodeUtils::GetUTF8(hash.ToString().Left(g_Git.GetShortHASHLength())));
			}


		}else
		{
			for(int i=0; i < m_HashMap[hash].size(); i++)
			{
				CString shortname;
				CString str = m_HashMap[hash][i];
				RectF rect;

				rect.X = (REAL)noderect.X;
				rect.Y = (REAL)(noderect.Y + hight*i);
				rect.Width = (REAL)noderect.Width;
				rect.Height = (REAL)hight;

				COLORREF colRef = 0;


				if(CGit::GetShortName(str,shortname,_T("refs/heads/")))
				{
					if( shortname == m_CurrentBranch )
						colRef = m_Colors.GetColor(CColors::CurrentBranch);
					else
						colRef = m_Colors.GetColor(CColors::LocalBranch);

				}
				else if(CGit::GetShortName(str,shortname,_T("refs/remotes/")))
				{
					colRef = m_Colors.GetColor(CColors::RemoteBranch);
				}
				else if(CGit::GetShortName(str,shortname,_T("refs/tags/")))
				{
					colRef = m_Colors.GetColor(CColors::Tag);
				}
				else if(CGit::GetShortName(str,shortname,_T("refs/stash")))
				{
					colRef = m_Colors.GetColor(CColors::Stash);
					shortname=_T("stash");
				}
				else if(CGit::GetShortName(str,shortname,_T("refs/bisect/")))
				{
					if(shortname.Find(_T("good")) == 0)
					{
						colRef = m_Colors.GetColor(CColors::BisectGood);
						shortname = _T("good");
					}

					if(shortname.Find(_T("bad")) == 0)
					{
						colRef = m_Colors.GetColor(CColors::BisectBad);
						shortname = _T("bad");
					}
				}else if(CGit::GetShortName(str,shortname,_T("refs/notes/")))
				{
					colRef = m_Colors.GetColor(CColors::NoteNode);
				}

				Gdiplus::Color color(GetRValue(colRef), GetGValue(colRef), GetBValue(colRef));
				Gdiplus::Pen pen(color);
				Gdiplus::SolidBrush brush(color);

				int mask =0;
				mask |= (i==0)? ROUND_UP:0;
				mask |= (i== m_HashMap[hash].size()-1)? ROUND_DOWN:0;
				this->DrawRoundedRect(graphics, color,1,&pen, color,&brush, rect,mask);

				if (graphics.graphics)
				{

					//graphics.graphics->FillRectangle(&SolidBrush(Gdiplus::Color(GetRValue(colRef), GetGValue(colRef), GetBValue(colRef))),
					//		rect);

					graphics.graphics->DrawString(shortname.GetBuffer(),shortname.GetLength(),
						&font,
						Gdiplus::PointF((REAL)(noderect.X + this->GetLeftRightMargin()*m_fZoomFactor),
										(REAL)(noderect.Y + this->GetTopBottomMargin()*m_fZoomFactor+ hight*i)),
						&blackbrush);

					//graphics.graphics->DrawString(shortname.GetBuffer(), shortname.GetLength(), ::new Gdiplus::Font(graphics.pDC->m_hDC), PointF(noderect.X, noderect.Y + hight *i),NULL, NULL);

				}
				else if (graphics.pSVG)
				{

					graphics.pSVG->Text((int)(noderect.X + this->GetLeftRightMargin() * m_fZoomFactor), 
										(int)(noderect.Y + this->GetTopBottomMargin() * m_fZoomFactor + hight * i + m_nFontSize),
										CUnicodeUtils::GetUTF8(fontname), m_nFontSize,
										false, false, Color::Black, CUnicodeUtils::GetUTF8(shortname));

				}
			}
		}
		if ((m_SelectedEntry1 == v))
			DrawMarker(graphics, noderect, mpLeft, 0, Color(0,0, 255), 1);

		if ((m_SelectedEntry2 == v))
			DrawMarker(graphics, noderect, mpLeft, 0, Color(136,0, 21), 2);

	}
}


#if 0
void CRevisionGraphWnd::DrawCurrentNodeGlyphs (GraphicsDevice& graphics, Image* glyphs, const CSize& offset)
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
#endif

void CRevisionGraphWnd::DrawGraph(GraphicsDevice& graphics, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
	CMemDC* memDC = NULL;
	if (graphics.pDC)
	{
		if (!bDirectDraw)
		{
			memDC = new CMemDC (*graphics.pDC, rect);
			graphics.pDC = &memDC->GetDC();
		}

		graphics.pDC->FillSolidRect(rect, GetSysColor(COLOR_WINDOW));
		graphics.pDC->SetBkMode(TRANSPARENT);
	}

	// preparation & sync

	//CSyncPointer<CAllRevisionGraphOptions> options (m_state.GetOptions());
	ClearVisibleGlyphs (rect);

	// transform visible

	CSize offset (nHScrollPos, nVScrollPos);
	CRect logRect ( (int)(offset.cx / m_fZoomFactor)-1
					, (int)(offset.cy / m_fZoomFactor)-1
					, (int)((rect.Width() + offset.cx) / m_fZoomFactor) + 1
					, (int)((rect.Height() + offset.cy) / m_fZoomFactor) + 1);

	// draw the different components

	if (graphics.pDC)
	{
		Graphics* gcs = Graphics::FromHDC(*graphics.pDC);
		graphics.graphics = gcs;
		gcs->SetPageUnit (UnitPixel);
		gcs->SetInterpolationMode (InterpolationModeHighQualityBicubic);
		gcs->SetSmoothingMode(SmoothingModeAntiAlias);
		gcs->SetClip(RectF(Gdiplus::REAL(rect.left), Gdiplus::REAL(rect.top), Gdiplus::REAL(rect.Width()), Gdiplus::REAL(rect.Height())));
	}

//	if (options->GetOption<CShowTreeStripes>()->IsActive())
//		DrawStripes (graphics, offset);

	//if (m_fZoomFactor > SHADOW_ZOOM_THRESHOLD)
	//	DrawShadows (graphics, logRect, offset);

	Bitmap glyphs (AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHGLYPHS));

	DrawTexts (graphics, logRect, offset);
	DrawConnections (graphics, logRect, offset);
	//if (m_showHoverGlyphs)
	//	DrawCurrentNodeGlyphs (graphics, &glyphs, offset);

	// draw preview

	if ((!bDirectDraw)&&(m_Preview.GetSafeHandle())&&(m_bShowOverview)&&(graphics.pDC))
	{
		// draw the overview image rectangle in the top right corner
		CMyMemDC memDC2(graphics.pDC, true);
		memDC2.SetWindowOrg(0, 0);
		HBITMAP oldhbm = (HBITMAP)memDC2.SelectObject(&m_Preview);
		graphics.pDC->BitBlt(rect.Width()-m_previewWidth, rect.Height() -  m_previewHeight, m_previewWidth, m_previewHeight,
			&memDC2, 0, 0, SRCCOPY);
		memDC2.SelectObject(oldhbm);
		// draw the border for the overview rectangle
		m_OverviewRect.left = rect.Width()-m_previewWidth;
		m_OverviewRect.top = rect.Height()-  m_previewHeight;
		m_OverviewRect.right = rect.Width();
		m_OverviewRect.bottom = rect.Height();
		graphics.pDC->DrawEdge(&m_OverviewRect, EDGE_BUMP, BF_RECT);
		// now draw a rectangle where the current view is located in the overview

		CRect viewRect = GetViewRect();
		LONG width = (long)(rect.Width() * m_previewZoom / m_fZoomFactor);
		LONG height = (long)(rect.Height() * m_previewZoom / m_fZoomFactor);
		LONG xpos = (long)(nHScrollPos * m_previewZoom / m_fZoomFactor);
		LONG ypos = (long)(nVScrollPos * m_previewZoom / m_fZoomFactor);
		RECT tempRect;
		tempRect.left = rect.Width()-m_previewWidth+xpos;
		tempRect.top = rect.Height() - m_previewHeight + ypos;
		tempRect.right = tempRect.left + width;
		tempRect.bottom = tempRect.top + height;
		// make sure the position rect is not bigger than the preview window itself
		::IntersectRect(&m_OverviewPosRect, &m_OverviewRect, &tempRect);

		RectF rect2 ( (float)m_OverviewPosRect.left, (float)m_OverviewPosRect.top
					, (float)m_OverviewPosRect.Width(), (float)m_OverviewPosRect.Height());
		if (graphics.graphics)
		{
			SolidBrush brush (Color (64, 0, 0, 0));
			graphics.graphics->FillRectangle (&brush, rect2);
			graphics.pDC->DrawEdge(&m_OverviewPosRect, EDGE_BUMP, BF_RECT);
		}
	}

	// flush changes to screen

	delete graphics.graphics;
	delete memDC;
}

void CRevisionGraphWnd::SetNodeRect(GraphicsDevice& graphics, ogdf::node *pnode, CGitHash rev, int mode )
{
	//multi - line mode. One RefName is one new line
	CString fontname = CRegString(_T("Software\\TortoiseGit\\LogFontName"), _T("Courier New"));
	if(mode == 0)
	{
		if(this->m_HashMap.find(rev) == m_HashMap.end())
		{
			CString shorthash = rev.ToString().Left(g_Git.GetShortHASHLength());
			RectF rect;
			if(graphics.graphics)
			{
				//GetTextExtentPoint32(graphics.pDC->m_hDC, shorthash.GetBuffer(), shorthash.GetLength(), &size);
				Gdiplus::Font font(fontname.GetBuffer(), (REAL)m_nFontSize, FontStyleRegular);
				graphics.graphics->MeasureString(shorthash.GetBuffer(), shorthash.GetLength(),
										&font,
										Gdiplus::PointF(0,0), &rect);

			}
			m_GraphAttr.width(*pnode) = this->GetLeftRightMargin()*2 + rect.Width;
			m_GraphAttr.height(*pnode) = this->GetTopBottomMargin()*2 + rect.Height;
		}
		else
		{
			double xmax=0;
			double ymax=0;
			int lines =0;
			for(int i=0;i<m_HashMap[rev].size();i++)
			{
				RectF rect;
				CString shortref = m_HashMap[rev][i];
				shortref = CGit::GetShortName(shortref,NULL);
				if(graphics.pDC)
				{
					Gdiplus::Font font(fontname.GetBuffer(), (REAL)m_nFontSize, FontStyleRegular);
					graphics.graphics->MeasureString(shortref.GetBuffer(), shortref.GetLength(),
										&font,
										Gdiplus::PointF(0,0), &rect);
					if(rect.Width > xmax)
						xmax = rect.Width;
					if(rect.Height > ymax)
						ymax = rect.Height;
				}
				lines ++;
			}
			m_GraphAttr.width(*pnode) = this->GetLeftRightMargin()*2 + xmax;
			m_GraphAttr.height(*pnode) = (this->GetTopBottomMargin()*2 + ymax) * lines;
		}

	}
}