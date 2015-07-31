// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2010, 2012, 2015 - TortoiseSVN

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
#include "registry.h"

class CMainFrame;
class CBaseView;

/**
 * \ingroup TortoiseMerge
 *
 * A Toolbar showing the differences in the views. The Toolbar
 * is best attached to the left of the mainframe. The Toolbar
 * also scrolls the views to the location the user clicks
 * on the bar.
 */
class CLocatorBar : public CPaneDialog
{
	DECLARE_DYNAMIC(CLocatorBar)

public:
	CLocatorBar();
	virtual ~CLocatorBar();
	BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID)
	{
		BOOL bRet = CPaneDialog::Create(pParentWnd, nIDTemplate, nStyle, nID);
		m_dwControlBarStyle = 0; // can't float, resize, close, slide
		CRect rc;
		GetClientRect(&rc);
		m_minWidth = rc.Width();
		return bRet;
	}

	void			DocumentUpdated();

protected:
	virtual CSize	CalcFixedLayout(BOOL bStretch, BOOL bHorz);
	afx_msg void	OnPaint();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg LRESULT	OnMouseLeave(WPARAM wParam, LPARAM lParam);
	void			ScrollOnMouseMove(const CPoint& point );
	void			ScrollViewToLine(CBaseView* view, int nLine) const;
	void			PaintView(CDC& cacheDC, CBaseView* view, CDWordArray& indents, CDWordArray& states,
						const CRect& rect, int stripeIndex);
	void			DrawFishEye(CDC& dc, const CRect& rect );
	void			DocumentUpdated(CBaseView* view, CDWordArray& indents, CDWordArray& states);

	CBitmap *		m_pCacheBitmap;

	int				m_minWidth;
	int				m_nLines;
	CPoint			m_MousePos;
	CDWordArray		m_arLeftIdent;
	CDWordArray		m_arLeftState;
	CDWordArray		m_arRightIdent;
	CDWordArray		m_arRightState;
	CDWordArray		m_arBottomIdent;
	CDWordArray		m_arBottomState;

	CRegDWORD		m_regUseFishEye;
	DECLARE_MESSAGE_MAP()
public:
	CMainFrame *	m_pMainFrm;
};
