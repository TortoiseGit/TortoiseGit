// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2011 - TortoiseGit

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
#include "resource.h"
#include "RefLogDlg.h"
#include "git.h"
#include "RefLogList.h"

IMPLEMENT_DYNAMIC(CRefLogList, CGitLogList)

CRefLogList::CRefLogList()
{
	m_ColumnRegKey=_T("reflog");
	this->m_ContextMenuMask |= this->GetContextMenuBit(ID_LOG);
}

void CRefLogList::InsertRefLogColumn()
{
	CString temp;

	CRegDWORD regFullRowSelect(_T("Software\\TortoiseGit\\FullRowSelect"), TRUE);
	DWORD exStyle = LVS_EX_HEADERDRAGDROP | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP | LVS_EX_SUBITEMIMAGES;
	if (DWORD(regFullRowSelect))
		exStyle |= LVS_EX_FULLROWSELECT;
	SetExtendedStyle(exStyle);

	static UINT normal[] =
	{
		IDS_HASH,
		IDS_REF,
		IDS_ACTION,
		IDS_MESSAGE,
	};

	static int with[] =
	{
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		LOGLIST_MESSAGE_MIN,
	};
	m_dwDefaultColumns = 0xFFFF;

	SetRedraw(false);

	m_ColumnManager.SetNames(normal, _countof(normal));
	m_ColumnManager.ReadSettings(m_dwDefaultColumns,0, m_ColumnRegKey+_T("loglist"), _countof(normal), with);

	SetRedraw(true);

}

void CRefLogList::OnLvnGetdispinfoLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);

	// Create a pointer to the item
	LV_ITEM* pItem = &(pDispInfo)->item;

	// Do the list need text information?
	if (!(pItem->mask & LVIF_TEXT))
		return;

	// By default, clear text buffer.
	lstrcpyn(pItem->pszText, _T(""), pItem->cchTextMax);

	bool bOutOfRange = pItem->iItem >= ShownCountWithStopped();

	*pResult = 0;
	if (m_bNoDispUpdates || bOutOfRange)
		return;

	// Which item number?
	int itemid = pItem->iItem;
	GitRev * pLogEntry = NULL;
	if (itemid < m_arShownList.GetCount())
		pLogEntry = reinterpret_cast<GitRev*>(m_arShownList.GetAt(pItem->iItem));

	CString temp;

	// Which column?
	switch (pItem->iSubItem)
	{
	case this->REFLOG_HASH:
		if (pLogEntry)
		{
			lstrcpyn(pItem->pszText,pLogEntry->m_CommitHash.ToString(), pItem->cchTextMax);
		}
		break;
	case REFLOG_REF:
		if(pLogEntry)
			lstrcpyn(pItem->pszText, pLogEntry->m_Ref, pItem->cchTextMax);
		break;
	case REFLOG_ACTION:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->m_RefAction, pItem->cchTextMax);
		break;
	case REFLOG_MESSAGE:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, (LPCTSTR)pLogEntry->GetSubject().Trim(), pItem->cchTextMax);
		break;

	default:
		ASSERT(false);
	}
}

void CRefLogList::OnNMCustomdrawLoglist(NMHDR *pNMHDR, LRESULT *pResult)
{
	UNREFERENCED_PARAMETER(pNMHDR);
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;
}
