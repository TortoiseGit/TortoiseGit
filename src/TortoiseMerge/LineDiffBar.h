// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006, 2008 - TortoiseSVN

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

class CMainFrame;

/**
 * \ingroup TortoiseMerge
 *
 * A Toolbar showing the differences in the views. The Toolbar
 * is best attached to the left of the mainframe. The Toolbar
 * also scrolls the views to the location the user clicks
 * on the bar.
 */
class CLineDiffBar : public CPaneDialog
{
	DECLARE_DYNAMIC(CLineDiffBar)

public:
	CLineDiffBar();
	virtual ~CLineDiffBar();
	BOOL Create(CWnd* pParentWnd, UINT nIDTemplate, UINT nStyle, UINT nID)
	{
		BOOL bRet = CPaneDialog::Create(pParentWnd, nIDTemplate, nStyle, nID);
		m_dwControlBarStyle = 0; // can't float, resize, close, slide
		return bRet;
	}

	CSize CalcFixedLayout(BOOL, BOOL)
	{
		return CSize(32767, 2*m_nLineHeight);
	}

	void			ShowLines(int nLineIndex);
	void			DocumentUpdated();

protected:
	afx_msg void	OnPaint();
	afx_msg void	OnSize(UINT nType, int cx, int cy);
	afx_msg BOOL	OnEraseBkgnd(CDC* pDC);

	CBitmap *		m_pCacheBitmap;

	int				m_nLineIndex;
	int				m_nLineHeight;


	DECLARE_MESSAGE_MAP()
public:
	CMainFrame *	m_pMainFrm;
};


