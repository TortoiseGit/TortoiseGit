// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006 - Stefan Kueng

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

#include "Watereffect.h"
#include "Dib.h"
#include "HyperLink.h"

#define ID_EFFECTTIMER 1111
#define ID_DROPTIMER 1112

/**
 * \ingroup TortoiseMerge
 * Class for showing an About box of TortoiseMerge. Contains a Picture
 * with the TortoiseMerge logo with a nice water effect. See CWaterEffect
 * for the implementation.
 */
class CAboutDlg : public CDialog
{
	DECLARE_DYNAMIC(CAboutDlg)

public:
	CAboutDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUT };

protected:
	HICON m_hIcon;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	DECLARE_MESSAGE_MAP()
public:

private:
	CWaterEffect m_waterEffect;
	CDib m_renderSrc;
	CDib m_renderDest;
	CHyperLink m_cWebLink;
	CHyperLink m_cSupportLink;
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
