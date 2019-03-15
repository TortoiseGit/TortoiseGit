// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit

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
#include "SyncDlg.h"
#include "SyncTabCtrl.h"

IMPLEMENT_DYNAMIC(CSyncTabCtrl, CMFCTabCtrl)

CSyncTabCtrl::CSyncTabCtrl()
{
}

CSyncTabCtrl::~CSyncTabCtrl()
{
}

BEGIN_MESSAGE_MAP(CSyncTabCtrl, CMFCTabCtrl)
	ON_NOTIFY(EN_LINK, IDC_CMD_LOG, OnEnLinkLog)
	ON_EN_VSCROLL(IDC_CMD_LOG, OnEnscrollLog)
	ON_EN_HSCROLL(IDC_CMD_LOG, OnEnscrollLog)
END_MESSAGE_MAP()

void CSyncTabCtrl::OnEnLinkLog(NMHDR *pNMHDR, LRESULT *pResult)
{
	static_cast<CSyncDlg*>(GetParent())->OnEnLinkLog(pNMHDR, pResult);
}

void CSyncTabCtrl::OnEnscrollLog()
{
	static_cast<CSyncDlg*>(GetParent())->OnEnscrollLog();
}
