// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2007, 2009-2010, 2013 - TortoiseSVN

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
#include "TortoiseMerge.h"
#include "AboutDlg.h"
#include "svn_version.h"
#include "svn_diff.h"
#include "../../apr/include/apr_version.h"
#include "../../apr-util/include/apu_version.h"
#include "../version.h"

// CAboutDlg dialog

IMPLEMENT_DYNAMIC(CAboutDlg, CStandAloneDialog)
CAboutDlg::CAboutDlg(CWnd* pParent /*=nullptr*/)
	: CStandAloneDialog(CAboutDlg::IDD, pParent)
{
}

CAboutDlg::~CAboutDlg()
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WEBLINK, m_cWebLink);
	DDX_Control(pDX, IDC_SUPPORTLINK, m_cSupportLink);
}


BEGIN_MESSAGE_MAP(CAboutDlg, CStandAloneDialog)
	ON_WM_TIMER()
	ON_WM_MOUSEMOVE()
	ON_WM_CLOSE()
END_MESSAGE_MAP()


BOOL CAboutDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	//set the version string
	CString temp, boxtitle;
	boxtitle.Format(IDS_ABOUTVERSIONBOX, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, _T(TGIT_PLATFORM), _T(TGIT_VERDATE));
	SetDlgItemText(IDC_VERSIONBOX, boxtitle);
	const svn_version_t * diffver = svn_diff_version();
	temp.Format(IDS_ABOUTVERSION, TGIT_VERMAJOR, TGIT_VERMINOR, TGIT_VERMICRO, TGIT_VERBUILD, _T(TGIT_PLATFORM), _T(TGIT_VERDATE),
		diffver->major, diffver->minor, diffver->patch, static_cast<LPCTSTR>(CString(diffver->tag)),
		APR_MAJOR_VERSION, APR_MINOR_VERSION, APR_PATCH_VERSION,
		APU_MAJOR_VERSION, APU_MINOR_VERSION, APU_PATCH_VERSION);
	SetDlgItemText(IDC_VERSIONABOUT, temp);
	this->SetWindowText(L"TortoiseGitMerge");

	CPictureHolder tmpPic;
	tmpPic.CreateFromBitmap(IDB_LOGOFLIPPED);
	m_renderSrc.Create32BitFromPicture(&tmpPic,468,64);
	m_renderDest.Create32BitFromPicture(&tmpPic,468,64);

	m_waterEffect.Create(468,64);
	SetTimer(ID_EFFECTTIMER, 40, nullptr);
	SetTimer(ID_DROPTIMER, 300, nullptr);

	m_cWebLink.SetURL(L"https://tortoisegit.org/");
	m_cSupportLink.SetURL(L"http://tortoisesvn.tigris.org/contributors.html");

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CAboutDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == ID_EFFECTTIMER)
	{
		m_waterEffect.Render(static_cast<DWORD*>(m_renderSrc.GetDIBits()), static_cast<DWORD*>(m_renderDest.GetDIBits()));
		CClientDC dc(this);
		CPoint ptOrigin(15,20);
		m_renderDest.Draw(&dc,ptOrigin);
	}
	if (nIDEvent == ID_DROPTIMER)
	{
		CRect r;
		r.left = 15;
		r.top = 20;
		r.right = r.left + m_renderSrc.GetWidth();
		r.bottom = r.top + m_renderSrc.GetHeight();
		m_waterEffect.Blob(random(r.left,r.right), random(r.top, r.bottom), 2, 400, m_waterEffect.m_iHpage);
	}
	CStandAloneDialog::OnTimer(nIDEvent);
}

void CAboutDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect r;
	r.left = 15;
	r.top = 20;
	r.right = r.left + m_renderSrc.GetWidth();
	r.bottom = r.top + m_renderSrc.GetHeight();

	if (r.PtInRect(point) != FALSE)
	{
		// dibs are drawn upside down...
		point.y -= 20;
		point.y = 64-point.y;

		if (nFlags & MK_LBUTTON)
			m_waterEffect.Blob(point.x -15,point.y,5,1600,m_waterEffect.m_iHpage);
		else
			m_waterEffect.Blob(point.x -15,point.y,2,50,m_waterEffect.m_iHpage);

	}

	CStandAloneDialog::OnMouseMove(nFlags, point);
}

void CAboutDlg::OnClose()
{
	KillTimer(ID_EFFECTTIMER);
	KillTimer(ID_DROPTIMER);
	CStandAloneDialog::OnClose();
}
