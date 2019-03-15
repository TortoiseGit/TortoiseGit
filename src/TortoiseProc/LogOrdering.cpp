// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016, 2019 - TortoiseGit

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
#include "LogOrdering.h"
#include "Git.h"
#include "registry.h"

IMPLEMENT_DYNAMIC(CLogOrdering, CDialog)
CLogOrdering::CLogOrdering(CWnd* pParent /*=nullptr*/)
	: CDialog(CLogOrdering::IDD, pParent)
{
}

CLogOrdering::~CLogOrdering()
{
}

void CLogOrdering::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBOBOXEX_ORDERING, m_cLogOrdering);
}

BEGIN_MESSAGE_MAP(CLogOrdering, CDialog)
END_MESSAGE_MAP()

BOOL CLogOrdering::OnInitDialog()
{
	CDialog::OnInitDialog();

	int ind = m_cLogOrdering.AddString(CString(MAKEINTRESOURCE(IDS_LOG_CHRONOLOGICALREVERSEDORDER)));
	m_cLogOrdering.SetItemData(ind, CGit::LOG_ORDER_CHRONOLOGIALREVERSED);
	ind = m_cLogOrdering.AddString(L"--topo-order " + CString(MAKEINTRESOURCE(IDS_TORTOISEGITDEFAULT)));
	m_cLogOrdering.SetItemData(ind, CGit::LOG_ORDER_TOPOORDER);
	ind = m_cLogOrdering.AddString(L"--date-order");
	m_cLogOrdering.SetItemData(ind, CGit::LOG_ORDER_DATEORDER);

	DWORD curOrder = CRegDWORD(L"Software\\TortoiseGit\\LogOrderBy", CGit::LOG_ORDER_TOPOORDER);
	for (int i = 0; i < m_cLogOrdering.GetCount(); ++i)
		if (m_cLogOrdering.GetItemData(i) == curOrder)
			m_cLogOrdering.SetCurSel(i);

	return TRUE;
}

void CLogOrdering::OnOK()
{
	if (m_cLogOrdering.GetCurSel() != CB_ERR)
		CRegDWORD(L"Software\\TortoiseGit\\LogOrderBy") = static_cast<DWORD>(m_cLogOrdering.GetItemData(m_cLogOrdering.GetCurSel()));

	__super::OnOK();
}
