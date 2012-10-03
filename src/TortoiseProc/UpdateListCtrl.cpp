// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit

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
// CUpdateListCtrl

#include "stdafx.h"
#include "UpdateListCtrl.h"

IMPLEMENT_DYNAMIC(CUpdateListCtrl, CListCtrl)

CUpdateListCtrl::CUpdateListCtrl()
{
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf = {0};
	GetObject(hFont, sizeof(LOGFONT), &lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont = CreateFontIndirect(&lf);
}

CUpdateListCtrl::~CUpdateListCtrl()
{
	if (m_boldFont)
		DeleteObject(m_boldFont);
}

BEGIN_MESSAGE_MAP(CUpdateListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CUpdateListCtrl::OnNMCustomdraw)
END_MESSAGE_MAP()



// CUpdateListCtrl message handlers
void CUpdateListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVCUSTOMDRAW *pNMCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	*pResult = 0;


	switch (pNMCD->nmcd.dwDrawStage)
	{
	case CDDS_PREPAINT:
		{
			*pResult = CDRF_NOTIFYITEMDRAW;
			return;
		}
		break;
	case CDDS_ITEMPREPAINT:
		{
			// This is the prepaint stage for an item. Here's where we set the
			// item's text color.

			// Tell Windows to send draw notifications for each subitem.
			*pResult = CDRF_NOTIFYSUBITEMDRAW;

			CUpdateListCtrl::Entry *data = (CUpdateListCtrl::Entry *)this->GetItemData((int)pNMCD->nmcd.dwItemSpec);
			switch(data->m_status & STATUS_MASK)
			{
			case STATUS_SUCCESS:
				pNMCD->clrText = RGB(0,128,0);
				break;
			case STATUS_FAIL:
				pNMCD->clrText = RGB(255,0,0);
				break;
			case STATUS_IGNORE:
				pNMCD->clrText = RGB(128,128,128);
				break;
			}

			if(data->m_status & STATUS_DOWNLOADING)
			{
				SelectObject(pNMCD->nmcd.hdc, m_boldFont);
				*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
			}

		}
		break;
	}
	*pResult = CDRF_DODEFAULT;
}
