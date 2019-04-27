// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2011, 2015 - TortoiseSVN
// Copyright (C) 2012-2013, 2015-2019 - TortoiseGit

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
#include "RevisionGraphDlg.h"
#include "Git.h"
#include "TempFile.h"
#include "UnicodeUtils.h"
#include "TGitPath.h"
//#include "SVNInfo.h"
//#include "SVNDiff.h"
#include "RevisionGraphWnd.h"
//#include "IRevisionGraphLayout.h"
//#include "UpsideDownLayout.h"
//#include "ShowTreeStripes.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "DPIAware.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

using namespace Gdiplus;

Color GetColorFromSysColor(int nIndex)
{
	Color color;
	color.SetFromCOLORREF(GetSysColor(nIndex));
	return color;
}

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
	if (!m_apFonts[nIndex])
	{
		m_apFonts[nIndex] = new CFont;
		m_lfBaseFont.lfWeight = bBold ? FW_BOLD : FW_NORMAL;
		m_lfBaseFont.lfItalic = static_cast<BYTE>(bItalic);
		m_lfBaseFont.lfStrikeOut = FALSE;
		m_lfBaseFont.lfHeight = -CDPIAware::Instance().PointsToPixelsY(m_nFontSize);
		// use the empty font name, so GDI takes the first font which matches
		// the specs. Maybe this will help render chinese/japanese chars correctly.
		wcsncpy_s(m_lfBaseFont.lfFaceName, L"MS Shell Dlg 2", _countof(m_lfBaseFont.lfFaceName) - 1);
		if (!m_apFonts[nIndex]->CreateFontIndirect(&m_lfBaseFont))
		{
			delete m_apFonts[nIndex];
			m_apFonts[nIndex] = nullptr;
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
		dc.ExtTextOut(20, 20, ETO_CLIPPED, nullptr, fetch, nullptr);
		CWnd::OnPaint();
		return;

	}else if (this->m_Graph.empty())
	{
		CString sNoGraphText;
		sNoGraphText.LoadString(IDS_REVGRAPH_ERR_NOGRAPH);
		dc.FillSolidRect(rect, RGB(255,255,255));
		dc.ExtTextOut(20, 20, ETO_CLIPPED, nullptr, sNoGraphText, nullptr);
		return;
	}

	GraphicsDevice dev;
	dev.pDC = &dc;
	DrawGraph(dev, rect, GetScrollPos(SB_VERT), GetScrollPos(SB_HORZ), false);
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
			path.AddLine(points[0].X, points[1].Y, points[3].X, points[2].Y);

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

		if (brush)
			graphics.graphics->FillPath (brush, &path);
		if (pen)
			graphics.graphics->DrawPath (pen, &path);
	}
	else if (graphics.pSVG)
		graphics.pSVG->RoundedRectangle(static_cast<int>(rect.X), static_cast<int>(rect.Y), static_cast<int>(rect.Width), static_cast<int>(rect.Height), penColor, penWidth, fillColor, static_cast<int>(radius), mask);
}

inline BYTE LimitedScaleColor (BYTE c1, BYTE c2, float factor)
{
	BYTE scaled = c2 + static_cast<BYTE>((c1 - c2) * factor);
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

RectF CRevisionGraphWnd::TransformRectToScreen (const CRect& rect, const CSize& offset) const
{
	PointF leftTop ( rect.left * m_fZoomFactor
					, rect.top * m_fZoomFactor);
	return RectF ( leftTop.X - offset.cx
				, leftTop.Y - offset.cy
				, rect.right * m_fZoomFactor - leftTop.X - 1
				, rect.bottom * m_fZoomFactor - leftTop.Y);
}


RectF CRevisionGraphWnd::GetNodeRect(const ogdf::node& node, const CSize& offset) const
{
	// get node and position

	CRect rect;
	rect.left = static_cast<int>(this->m_GraphAttr.x(node) - m_GraphAttr.width(node)/2);
	rect.top = static_cast<int>(this->m_GraphAttr.y(node) - m_GraphAttr.height(node)/2);
	rect.bottom = static_cast<int>(rect.top + m_GraphAttr.height(node));
	rect.right = static_cast<int>(rect.left + m_GraphAttr.width(node));

	RectF noderect (TransformRectToScreen (rect, offset));

	// show two separate lines for touching nodes,
	// unless the scale is too small

	if (noderect.Height > 15.0f)
		noderect.Height -= 1.0f;

	// done

	return noderect;
}

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
	DrawRoundedRect(graphics, penColor, static_cast<int>(width), &pen, Color(0,0,0), nullptr, noderect);
	if (num == 1)
	{
		// Roman number 1
		REAL x = max(1.0f, 10 * this->m_fZoomFactor);
		REAL y1 = max(1.0f, 25 * this->m_fZoomFactor);
		REAL y2 = max(1.0f, 5 * this->m_fZoomFactor);
		if(graphics.graphics)
		{
			graphics.graphics->DrawLine(&pen, noderect.X + x, noderect.Y - y1, noderect.X + x, noderect.Y - y2);
			if (m_SelectedEntry2)
			{
				CString base(L'(');
				base.AppendFormat(IDS_PROC_DIFF_BASE);
				base += L')';
				SolidBrush blackbrush(penColor);
				Gdiplus::Font font(CAppUtils::GetLogFontName(), static_cast<REAL>(m_nFontSize), FontStyleRegular);
				graphics.graphics->DrawString(base, base.GetLength(), &font, Gdiplus::PointF(noderect.X + x + width, noderect.Y - y1), &blackbrush);
			}
		}
	}
	else if (num == 2)
	{
		// Roman number 2
		REAL x1 = max(1.0f, 5 * this->m_fZoomFactor);
		REAL x2 = max(1.0f, 15 * this->m_fZoomFactor);
		REAL y1 = max(1.0f, 25 * this->m_fZoomFactor);
		REAL y2 = max(1.0f, 5 * this->m_fZoomFactor);
		if(graphics.graphics)
		{
			graphics.graphics->DrawLine(&pen, noderect.X + x1, noderect.Y - y1, noderect.X + x1, noderect.Y - y2);
			graphics.graphics->DrawLine(&pen, noderect.X + x2, noderect.Y - y1, noderect.X + x2, noderect.Y - y2);
		}
	}
}

PointF CRevisionGraphWnd::cutPoint(ogdf::node v, double lw, PointF ps, PointF pt)
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
			x = ps.X + t * dx;

			if(xmin <= x && x <= xmax)
				return PointF(static_cast<REAL>(x), static_cast<REAL>(ymax));

		// above
		} else if(pt.Y < ymin) {
			double t = (ymin-ps.Y) / dy;
			x = ps.X + t * dx;

			if(xmin <= x && x <= xmax)
				return PointF(static_cast<REAL>(x), static_cast<REAL>(ymin));
		}
	}

	if(dx != 0) {
		// right
		if(pt.X > xmax) {
			double t = (xmax-ps.X) / dx;
			y = ps.Y + t * dy;

			if(ymin <= y && y <= ymax)
				return PointF(static_cast<REAL>(xmax), static_cast<REAL>(y));

		// left
		} else if(pt.X < xmin) {
			double t = (xmin-ps.X) / dx;
			y = ps.Y + t * dy;

			if(ymin <= y && y <= ymax)
				return PointF(static_cast<REAL>(xmin), static_cast<REAL>(y));
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
	Gdiplus::Pen pen(GetColorFromSysColor(COLOR_WINDOWTEXT), penwidth);

	// iterate over all visible lines
	ogdf::edge e;
	forall_edges(e, m_Graph)
	{
		// get connection and point position
		const auto& dpl = this->m_GraphAttr.bends(e);

		points.RemoveAll();
		pts.RemoveAll();

		PointF pt;
		pt.X = static_cast<REAL>(m_GraphAttr.x(e->source()));
		pt.Y = static_cast<REAL>(m_GraphAttr.y(e->source()));

		points.Add(pt);

		for (auto it = dpl.begin(); it.valid(); ++it)
		{
			pt.X =  static_cast<REAL>((*it).m_x);
			pt.Y =  static_cast<REAL>((*it).m_y);
			points.Add(pt);
		}

		pt.X = static_cast<REAL>(m_GraphAttr.x(e->target()));
		pt.Y = static_cast<REAL>(m_GraphAttr.y(e->target()));

		points.Add(pt);

		points[0] = this->cutPoint(e->source(), 1, points[0], points[1]);
		points[points.GetCount()-1] =  this->cutPoint(e->target(), 1, points[points.GetCount()-1], points[points.GetCount()-2]);
		// draw the connection

		for (int i = 0; i < points.GetCount(); ++i)
		{
			//CPoint pt;
			points[i].X = points[i].X * this->m_fZoomFactor - offset.cx;
			points[i].Y = points[i].Y * this->m_fZoomFactor - offset.cy;
			//pts.Add(pt);
		}

		if (graphics.graphics)
			graphics.graphics->DrawLines(&pen, points.GetData(), static_cast<int>(points.GetCount()));
		else if (graphics.pSVG)
			graphics.pSVG->Polyline(points.GetData(), static_cast<int>(points.GetCount()), Color(0, 0, 0), static_cast<int>(penwidth));
		else if (graphics.pGraphviz)
		{
			CString hash1 = L'g' + m_logEntries[e->target()->index()].ToString(g_Git.GetShortHASHLength());
			CString hash2 = L'g' + m_logEntries[e->source()->index()].ToString(g_Git.GetShortHASHLength());
			graphics.pGraphviz->DrawEdge(hash1, hash2);
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

		arrows[1].X =  points[0].X + static_cast<REAL>(p1_x);
		arrows[1].Y =  points[0].Y + static_cast<REAL>(p1_y);

		arrows[2].X =  points[0].X + static_cast<REAL>(dx * 3 / 5);
		arrows[2].Y =  points[0].Y + static_cast<REAL>(dy * 3 / 5);

		arrows[3].X =  points[0].X + static_cast<REAL>(p2_x);
		arrows[3].Y =  points[0].Y + static_cast<REAL>(p2_y);

		arrows[4].X =  points[0].X;
		arrows[4].Y =  points[0].Y;

		path.AddLines(arrows, 5);
		path.SetFillMode(FillModeAlternate);
		if(graphics.graphics)
			graphics.graphics->DrawPath(&pen, &path);
		else if(graphics.pSVG)
			graphics.pSVG->DrawPath(arrows, 5, Color(0,0,0), static_cast<int>(penwidth), Color(0, 0, 0));
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

	CString fontname = CAppUtils::GetLogFontName();

	Gdiplus::Font font(fontname, static_cast<REAL>(m_nFontSize), FontStyleRegular);
	SolidBrush blackbrush(static_cast<ARGB>(Color::Black));

	DWORD revGraphUseLocalForCur = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\Graph\\RevGraphUseLocalForCur");

	ogdf::node v;
	forall_nodes(v,m_Graph)
	{
		// get node and position
		RectF noderect (GetNodeRect (v, offset));

		// draw the revision text
		CGitHash hash = this->m_logEntries[v->index()];
		double hight = noderect.Height / (!m_HashMap[hash].empty() ? m_HashMap[hash].size() : 1);

		if (m_HashMap.find(hash) == m_HashMap.end() || m_HashMap[hash].empty())
		{
			Color background;
			background.SetFromCOLORREF (GetSysColor(COLOR_WINDOW));
			Gdiplus::Pen pen(background,1.0F);
			Color brightColor = LimitedScaleColor (background, RGB(255,0,0), 0.9f);
			Gdiplus::SolidBrush brush(brightColor);

			DrawRoundedRect(graphics, background,1, &pen, brightColor, &brush, noderect);

			if(graphics.graphics)
			{
				graphics.graphics->DrawString(hash.ToString(g_Git.GetShortHASHLength()), -1,
					&font,
					Gdiplus::PointF(noderect.X + this->GetLeftRightMargin()*this->m_fZoomFactor,noderect.Y+this->GetTopBottomMargin()*m_fZoomFactor),
					&blackbrush);
			}
			if(graphics.pSVG)
			{
				graphics.pSVG->Text(static_cast<int>(noderect.X + this->GetLeftRightMargin() * this->m_fZoomFactor),
											static_cast<int>(noderect.Y + this->GetTopBottomMargin() * m_fZoomFactor + m_nFontSize),
											CUnicodeUtils::GetUTF8(fontname), m_nFontSize, false, false, static_cast<ARGB>(Color::Black),
											CUnicodeUtils::GetUTF8(hash.ToString(g_Git.GetShortHASHLength())));
			}
			if (graphics.pGraphviz)
			{
				CString shortHash = hash.ToString(g_Git.GetShortHASHLength());
				graphics.pGraphviz->DrawNode(L'g' + shortHash, shortHash, fontname, m_nFontSize, background, brightColor, static_cast<int>(noderect.Height));
			}
		}else
		{
			if (graphics.pGraphviz)
			{
				CString id = L'g' + hash.ToString(g_Git.GetShortHASHLength());
				graphics.pGraphviz->BeginDrawTableNode(id, fontname, m_nFontSize, static_cast<int>(noderect.Height));
			}

			for (size_t i = 0; i < m_HashMap[hash].size(); ++i)
			{
				CString shortname;
				CString str = m_HashMap[hash][i];
				RectF rect;

				rect.X = static_cast<REAL>(noderect.X);
				rect.Y = static_cast<REAL>(noderect.Y + hight * i);
				rect.Width = static_cast<REAL>(noderect.Width);
				rect.Height = static_cast<REAL>(hight);

				COLORREF colRef = m_Colors.GetColor(CColors::OtherRef);

				CGit::REF_TYPE refType;
				shortname = CGit::GetShortName(str, &refType);
				switch (refType)
				{
				case CGit::REF_TYPE::LOCAL_BRANCH:
					if (!revGraphUseLocalForCur && shortname == m_CurrentBranch)
						colRef = m_Colors.GetColor(CColors::CurrentBranch);
					else
						colRef = m_Colors.GetColor(CColors::LocalBranch);
					break;
				case CGit::REF_TYPE::REMOTE_BRANCH:
					colRef = m_Colors.GetColor(CColors::RemoteBranch);
					break;
				case CGit::REF_TYPE::ANNOTATED_TAG:
				case CGit::REF_TYPE::TAG:
					colRef = m_Colors.GetColor(CColors::Tag);
					break;
				case CGit::REF_TYPE::STASH:
					colRef = m_Colors.GetColor(CColors::Stash);
					break;
				case CGit::REF_TYPE::BISECT_GOOD:
					colRef = m_Colors.GetColor(CColors::BisectGood);
					break;
				case CGit::REF_TYPE::BISECT_BAD:
					colRef = m_Colors.GetColor(CColors::BisectBad);
					break;
				case CGit::REF_TYPE::BISECT_SKIP:
					colRef = m_Colors.GetColor(CColors::BisectSkip);
					break;
				case CGit::REF_TYPE::NOTES:
					colRef = m_Colors.GetColor(CColors::NoteNode);
					break;
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

					graphics.graphics->DrawString(shortname, shortname.GetLength(),
						&font,
						Gdiplus::PointF(static_cast<REAL>(noderect.X + this->GetLeftRightMargin() * m_fZoomFactor),
										static_cast<REAL>(noderect.Y + this->GetTopBottomMargin() * m_fZoomFactor + hight * i)),
						&blackbrush);

					//graphics.graphics->DrawString(shortname.GetBuffer(), shortname.GetLength(), ::new Gdiplus::Font(graphics.pDC->m_hDC), PointF(noderect.X, noderect.Y + hight * i), nullptr, nullptr);

				}
				else if (graphics.pSVG)
					graphics.pSVG->Text(static_cast<int>(noderect.X + this->GetLeftRightMargin() * m_fZoomFactor),
										static_cast<int>(noderect.Y + this->GetTopBottomMargin() * m_fZoomFactor + hight * i + m_nFontSize),
										CUnicodeUtils::GetUTF8(fontname), m_nFontSize,
										false, false, static_cast<ARGB>(Color::Black), CUnicodeUtils::GetUTF8(shortname));
				else if (graphics.pGraphviz)
					graphics.pGraphviz->DrawTableNode(shortname, color);
			}

			if (graphics.pGraphviz)
				graphics.pGraphviz->EndDrawTableNode();
		}
		if ((m_SelectedEntry1 == v))
			DrawMarker(graphics, noderect, mpLeft, 0, GetColorFromSysColor(COLOR_HIGHLIGHT), 1);

		if ((m_SelectedEntry2 == v))
			DrawMarker(graphics, noderect, mpLeft, 0, Color(136,0, 21), 2);
	}
}

void CRevisionGraphWnd::DrawGraph(GraphicsDevice& graphics, const CRect& rect, int nVScrollPos, int nHScrollPos, bool bDirectDraw)
{
	CMemDC* memDC = nullptr;
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

	// transform visible

	CSize offset (nHScrollPos, nVScrollPos);
	CRect logRect ( static_cast<int>(offset.cx / m_fZoomFactor)-1
					, static_cast<int>(offset.cy / m_fZoomFactor)-1
					, static_cast<int>((rect.Width() + offset.cx) / m_fZoomFactor) + 1
					, static_cast<int>((rect.Height() + offset.cy) / m_fZoomFactor) + 1);

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

	Bitmap glyphs (AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_REVGRAPHGLYPHS));

	DrawTexts (graphics, logRect, offset);
	DrawConnections (graphics, logRect, offset);

	// draw preview
	if ((!bDirectDraw)&&(m_Preview.GetSafeHandle())&&(m_bShowOverview)&&(graphics.pDC))
	{
		// draw the overview image rectangle in the top right corner
		CMyMemDC memDC2(graphics.pDC, true);
		memDC2.SetWindowOrg(0, 0);
		CBitmap* oldhbm = memDC2.SelectObject(&m_Preview);
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

		LONG width = static_cast<long>(rect.Width() * m_previewZoom / m_fZoomFactor);
		LONG height = static_cast<long>(rect.Height() * m_previewZoom / m_fZoomFactor);
		LONG xpos = static_cast<long>(nHScrollPos * m_previewZoom / m_fZoomFactor);
		LONG ypos = static_cast<long>(nVScrollPos * m_previewZoom / m_fZoomFactor);
		RECT tempRect;
		tempRect.left = rect.Width()-m_previewWidth+xpos;
		tempRect.top = rect.Height() - m_previewHeight + ypos;
		tempRect.right = tempRect.left + width;
		tempRect.bottom = tempRect.top + height;
		// make sure the position rect is not bigger than the preview window itself
		::IntersectRect(&m_OverviewPosRect, &m_OverviewRect, &tempRect);

		RectF rect2 (static_cast<float>(m_OverviewPosRect.left), static_cast<float>(m_OverviewPosRect.top),
						static_cast<float>(m_OverviewPosRect.Width()), static_cast<float>(m_OverviewPosRect.Height()));
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
	CString fontname = CAppUtils::GetLogFontName();
	if(mode == 0)
	{
		if(this->m_HashMap.find(rev) == m_HashMap.end())
		{
			CString shorthash = rev.ToString(g_Git.GetShortHASHLength());
			RectF rect;
			if(graphics.graphics)
			{
				//GetTextExtentPoint32(graphics.pDC->m_hDC, shorthash.GetBuffer(), shorthash.GetLength(), &size);
				Gdiplus::Font font(fontname, static_cast<REAL>(m_nFontSize), FontStyleRegular);
				graphics.graphics->MeasureString(shorthash, shorthash.GetLength(),
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
			for (size_t i = 0; i < m_HashMap[rev].size(); ++i)
			{
				RectF rect;
				CString shortref = m_HashMap[rev][i];
				shortref = CGit::GetShortName(shortref, nullptr);
				if(graphics.pDC)
				{
					Gdiplus::Font font(fontname, static_cast<REAL>(m_nFontSize), FontStyleRegular);
					graphics.graphics->MeasureString(shortref, shortref.GetLength(),
										&font,
										Gdiplus::PointF(0,0), &rect);
					if(rect.Width > xmax)
						xmax = rect.Width;
					if(rect.Height > ymax)
						ymax = rect.Height;
				}
				++lines;
			}
			m_GraphAttr.width(*pnode) = this->GetLeftRightMargin()*2 + xmax;
			m_GraphAttr.height(*pnode) = (this->GetTopBottomMargin()*2 + ymax) * lines;
		}
	}
}
