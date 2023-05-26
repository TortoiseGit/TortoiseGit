// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2023 - TortoiseGit
// Copyright (C) 2003-2006. 2011, 2017 - TortoiseSVN

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
#include "AnimationManager.h"

#pragma warning(push)
#pragma warning(disable: 4458) // declaration of 'xxx' hides class member
#include <gdiplus.h>
#pragma warning(pop)

/////////////////////////////////////////////////////////////////////////////
// CSplitterControl window

#define SPN_SIZED WM_USER + 1
#define CW_LEFTALIGN 1
#define CW_RIGHTALIGN 2
#define CW_TOPALIGN 3
#define CW_BOTTOMALIGN 4
#define SPS_VERTICAL 1
#define SPS_HORIZONTAL 2
struct SPC_NMHDR
{
	NMHDR hdr;
	int delta;
};

class CSplitterControl : public CStatic
{
// Construction
public:
	CSplitterControl();
	virtual		~CSplitterControl();

	static HDWP	ChangeRect(HDWP hdwp, CWnd* pWnd, int dleft, int dtop, int dright, int dbottom);
	void		SetRange(int nMin, int nMax);
	void		SetRange(int nSubtraction, int nAddition, int nRoot);

	int			GetSplitterStyle();
	int			SetSplitterStyle(int nStyle = SPS_VERTICAL);

protected:
	void			MoveWindowTo(CPoint pt);
	void			PreSubclassWindow() override;
	afx_msg void	OnPaint();
	afx_msg void	OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL	OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void	OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void	OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);
	afx_msg LRESULT	OnMouseLeave(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()

private:
	bool		m_bIsPressed  = false;
	int			m_nType = 0;
	int			m_nX = 0;
	int			m_nY = 0;
	int			m_nMin = -1;
	int			m_nMax = -1;
	int			m_nSavePos = 0;
	bool		m_bMouseOverControl = false;
	ULONG_PTR	m_gdiPlusToken = 0;
	IUIAnimationVariablePtr m_AnimVarHot;
};
