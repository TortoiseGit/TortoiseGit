// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "RevGraphFilterDlg.h"


IMPLEMENT_DYNAMIC(CRevGraphFilterDlg, CDialog)

CRevGraphFilterDlg::CRevGraphFilterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRevGraphFilterDlg::IDD, pParent)
	, m_sFilterPaths(_T(""))
    , m_removeSubTree (FALSE)
	, m_sFromRev(_T(""))
	, m_sToRev(_T(""))
    , m_minrev (1)
    , m_maxrev (1)
{

}

CRevGraphFilterDlg::~CRevGraphFilterDlg()
{
}

void CRevGraphFilterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_PATHFILTER, m_sFilterPaths);
	DDX_Check(pDX, IDC_REMOVESUBTREE, m_removeSubTree);
	DDX_Control(pDX, IDC_FROMSPIN, m_cFromSpin);
	DDX_Control(pDX, IDC_TOSPIN, m_cToSpin);
	DDX_Text(pDX, IDC_FROMREV, m_sFromRev);
	DDX_Text(pDX, IDC_TOREV, m_sToRev);
}


BEGIN_MESSAGE_MAP(CRevGraphFilterDlg, CDialog)
	ON_NOTIFY(UDN_DELTAPOS, IDC_FROMSPIN, &CRevGraphFilterDlg::OnDeltaposFromspin)
	ON_NOTIFY(UDN_DELTAPOS, IDC_TOSPIN, &CRevGraphFilterDlg::OnDeltaposTospin)
END_MESSAGE_MAP()

BOOL CRevGraphFilterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_cFromSpin.SetBuddy(GetDlgItem(IDC_FROMREV));
	m_cFromSpin.SetRange32(1, m_HeadRev);
	m_cFromSpin.SetPos32(m_minrev);

	m_cToSpin.SetBuddy(GetDlgItem(IDC_TOREV));
	m_cToSpin.SetRange32(1, m_HeadRev);
	m_cToSpin.SetPos32(m_maxrev);

	return TRUE;
}

void CRevGraphFilterDlg::OnDeltaposFromspin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);

	// the 'from' revision is about to be changed.
	// Since the 'from' revision must not be higher or the same as the 'to'
	// revision, we have to block this if the user tries to do that.
	if ((pNMUpDown->iPos + pNMUpDown->iDelta) >= m_cToSpin.GetPos32())
	{
		// block the change
		pNMUpDown->iDelta = 0;
	}
	*pResult = 0;
}

void CRevGraphFilterDlg::OnDeltaposTospin(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	
	// the 'to' revision is about to be changed.
	// Since the 'to' revision must not be lower or the same as the 'from'
	// revision, we have to block this if the user tries to do that.
	if ((pNMUpDown->iPos + pNMUpDown->iDelta) <= m_cFromSpin.GetPos32())
	{
		// block the change
		pNMUpDown->iDelta = 0;
	}
	*pResult = 0;
}

void CRevGraphFilterDlg::GetRevisionRange(svn_revnum_t& minrev, svn_revnum_t& maxrev)
{
	minrev = m_minrev;
	maxrev = m_maxrev;
}

void CRevGraphFilterDlg::SetRevisionRange (svn_revnum_t minrev, svn_revnum_t maxrev)
{
	m_minrev = minrev;
	m_maxrev = maxrev;
}

void CRevGraphFilterDlg::OnOK()
{
	m_minrev = m_cFromSpin.GetPos32();
	m_maxrev = m_cToSpin.GetPos32();
	CDialog::OnOK();
}
