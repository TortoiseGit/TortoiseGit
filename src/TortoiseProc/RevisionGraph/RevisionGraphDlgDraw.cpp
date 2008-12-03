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
#include "RevisionGraph/IRevisionGraphLayout.h"

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
	CRect rect;
	GetClientRect(&rect);
	if (m_bThreadRunning)
	{
		dc.FillSolidRect(rect, ::GetSysColor(COLOR_APPWORKSPACE));
		CWnd::OnPaint();
		return;
	}
    else if (m_layout.get() == NULL)
	{
		CString sNoGraphText;
		sNoGraphText.LoadString(IDS_REVGRAPH_ERR_NOGRAPH);
		dc.FillSolidRect(rect, RGB(255,255,255));
		dc.ExtTextOut(20,20,ETO_CLIPPED,NULL,sNoGraphText,NULL);
		return;
	}
	
    DrawGraph(&dc, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ), false);
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

	PointF points[POINT_COUNT];
    CutawayPoints (rect, min (rect.Height, rect.Width) / 4, points);

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
                                 COLORREF contourRef, Color overlayColor, 
                                 const CVisibleGraphNode *node, NodeShape shape)
{
    // special case: line deleted but deletion node removed

    if (   (node->GetNext() == NULL) 
        && (node->GetClassification().Is (CNodeClassification::PATH_ONLY_DELETED)))
    {
        contourRef = m_Colors.GetColor(CColors::DeletedNode);
    }

    bool nodeSelected = (m_SelectedEntry1 == node) || (m_SelectedEntry2 == node);

    // calculate the RGB color values we need to draw the node

    Color contour;
    contour.SetFromCOLORREF (contourRef);

    Color background;
    background.SetFromCOLORREF (GetSysColor(COLOR_WINDOW));
    Color textColor;
    textColor.SetFromCOLORREF (GetSysColor(COLOR_WINDOWTEXT));

	Color selColor = LimitedScaleColor (background, contour, 0.5f);
	Color brightColor = LimitedScaleColor (background, contour, 0.9f);

	// Draw the main shape

    bool isWorkingCopy = node->GetClassification().Is (CNodeClassification::IS_WORKINGCOPY);
    Color penColor = (contour.GetValue() == Color::White) || isWorkingCopy
                     ? textColor
                     : contour;
    Color brushColor = nodeSelected ? selColor : brightColor;

    Pen pen (penColor, isWorkingCopy ? 3.0f : 1.0f);
    SolidBrush brush (brushColor);

    Pen* penRef = overlayColor.GetValue() == 0 ? &pen : NULL;
    DrawShape (graphics, penRef, &brush, rect, shape);

    // overlay with some other color

    if (overlayColor.GetValue() != 0)
    {
        SolidBrush brush (overlayColor);
        DrawShape (graphics, &pen, &brush, rect, shape);
    }
}

RectF CRevisionGraphWnd::GetNodeRect (const ILayoutNodeList::SNode& node, const CSize& offset) const
{
    // get node and position

    PointF leftTop ( node.rect.left * m_fZoomFactor
                   , node.rect.top * m_fZoomFactor);
	RectF noderect ( leftTop.X - offset.cx
                   , leftTop.Y - offset.cy
                   , node.rect.right * m_fZoomFactor - leftTop.X - 1
                   , node.rect.bottom * m_fZoomFactor - leftTop.Y);

    // show two separate lines for touching nodes, 
    // unless the scale is too small

    if (noderect.Height > 4.0f)
        noderect.Height -= 1.0f;

    // done

    return noderect;
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

    std::auto_ptr<const ILayoutNodeList> nodes (m_layout->GetNodes());
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

void CRevisionGraphWnd::DrawNodes (Graphics& graphics, const CRect& logRect, const CSize& offset)
{
    // iterate over all visible nodes

    std::auto_ptr<const ILayoutNodeList> nodes (m_layout->GetNodes());
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
			DrawNode(graphics, noderect, m_Colors.GetColor(CColors::DeletedNode), transparent, node.node, TSVNOctangle);
			break;

        case ILayoutNodeList::SNode::STYLE_ADDED:
            if (m_bTweakTagsColors && node.node->GetClassification().Is (CNodeClassification::IS_TAG))
                overlayColor = Color (128, 250, 250, 92);
            else if (m_bTweakTrunkColors && node.node->GetClassification().Is (CNodeClassification::IS_TRUNK))
                overlayColor = Color (64, 64, 255, 64);
            DrawNode(graphics, noderect, m_Colors.GetColor(CColors::AddedNode), overlayColor, node.node, TSVNRoundRect);
            break;

        case ILayoutNodeList::SNode::STYLE_RENAMED:
            if (m_bTweakTagsColors && node.node->GetClassification().Is (CNodeClassification::IS_TAG))
                overlayColor = Color (128, 92, 160, 160);
            else if (m_bTweakTrunkColors && node.node->GetClassification().Is (CNodeClassification::IS_TRUNK))
                overlayColor = Color (64, 0, 255, 160);
            DrawNode(graphics, noderect, m_Colors.GetColor(CColors::RenamedNode), overlayColor, node.node, TSVNOctangle);
			break;

        case ILayoutNodeList::SNode::STYLE_LAST:
			DrawNode(graphics, noderect, m_Colors.GetColor(CColors::LastCommitNode), transparent, node.node, TSVNEllipse);
			break;

        case ILayoutNodeList::SNode::STYLE_MODIFIED:
			DrawNode(graphics, noderect, GetSysColor(COLOR_WINDOWTEXT), transparent, node.node, TSVNRectangle);
			break;

        default:
            DrawNode(graphics, noderect, GetSysColor(COLOR_WINDOW), transparent, node.node, TSVNRectangle);
			break;
		}

    	// Draw the "tagged" icon

        if (node.node->GetFirstTag() != NULL)
		{
			// draw the icon
            float squareSize = 16 * m_fZoomFactor;
            float squareDist = min ( (noderect.Height - squareSize) / 2
                                   , squareSize / 2);

            PointF leftTop (noderect.GetRight() - squareSize * 3 / 2, noderect.Y + squareDist);
            PointF leftBottom (leftTop.X, leftTop.Y + squareSize);
            RectF square (leftTop, SizeF (squareSize, squareSize));

            Pen pen (Color(128, 0, 0, 0));
            LinearGradientBrush lgBrush (leftTop, leftBottom, Color (250, 250, 92), Color (230, 230, 64));
            graphics.FillRectangle (&lgBrush, square);
            graphics.DrawRectangle (&pen, square);
		}
    }
}

void CRevisionGraphWnd::DrawConnections (CDC* pDC, const CRect& logRect, const CSize& offset)
{
    enum {MAX_POINTS = 100};
    CPoint points[MAX_POINTS];

	CPen newpen(PS_SOLID, 0, GetSysColor(COLOR_WINDOWTEXT));
	CPen * pOldPen = pDC->SelectObject(&newpen);

    // iterate over all visible lines

    std::auto_ptr<const ILayoutConnectionList> connections (m_layout->GetConnections());
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
	COLORREF textcolor = GetSysColor(COLOR_WINDOWTEXT);
    if (m_nFontSize <= 0)
        return;

    // iterate over all visible nodes

    pDC->SetTextAlign (TA_CENTER | TA_TOP);
    std::auto_ptr<const ILayoutTextList> texts (m_layout->GetTexts());
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

		pDC->SetTextColor (textcolor);
        pDC->SelectObject (GetFont (FALSE, text.style));
        pDC->ExtTextOut ((textRect.left + textRect.right)/2, textRect.top, 0, &textRect, text.text, NULL);
    }
}

void CRevisionGraphWnd::DrawGraph(CDC* pDC, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
	CDC * memDC = bDirectDraw
                ? pDC
                : new CMyMemDC(pDC);
	
	memDC->FillSolidRect(rect, GetSysColor(COLOR_WINDOW));
	memDC->SetBkMode(TRANSPARENT);

    // transform visible

    CSize offset (nHScrollPos, nVScrollPos);
    CRect logRect ( (int)(offset.cx / m_fZoomFactor)-1
                  , (int)(offset.cy / m_fZoomFactor)-1
                  , (int)((rect.Width() + offset.cx) / m_fZoomFactor) + 1
                  , (int)((rect.Height() + offset.cy) / m_fZoomFactor) + 1);

    // draw the different components

    Graphics graphics (*memDC);
    graphics.SetPageUnit (UnitPixel);

	if (m_fZoomFactor > 0.2f)
        DrawShadows (graphics, logRect, offset);

    DrawNodes (graphics, logRect, offset);
    DrawConnections (memDC, logRect, offset);
    DrawTexts (memDC, logRect, offset);

	// find out which nodes are in the visible area of the client rect

	if ((!bDirectDraw)&&(m_Preview.GetSafeHandle())&&(m_bShowOverview))
	{
		// draw the overview image rectangle in the top right corner
		CMyMemDC memDC2(memDC, true);
		memDC2.SetWindowOrg(0, 0);
		HBITMAP oldhbm = (HBITMAP)memDC2.SelectObject(&m_Preview);
		memDC->BitBlt(rect.Width()-m_previewWidth, 0, m_previewWidth, m_previewHeight, 
			&memDC2, 0, 0, SRCCOPY);
		memDC2.SelectObject(oldhbm);
		// draw the border for the overview rectangle
		m_OverviewRect.left = rect.Width()-m_previewWidth;
		m_OverviewRect.top = 0;
		m_OverviewRect.right = rect.Width();
		m_OverviewRect.bottom = m_previewHeight;
		memDC->DrawEdge(&m_OverviewRect, EDGE_BUMP, BF_RECT);
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

        RectF rect ( (float)m_OverviewPosRect.left, (float)m_OverviewPosRect.top
                   , (float)m_OverviewPosRect.Width(), (float)m_OverviewPosRect.Height());
        SolidBrush brush (Color (64, 0, 0, 0));
        graphics.FillRectangle (&brush, rect);
		memDC->DrawEdge(&m_OverviewPosRect, EDGE_BUMP, BF_RECT);
	}

	if (!bDirectDraw)
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

