// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2011, 2013, 2015-2019 TortoiseGit

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
#include "refloglist.h"
#include "LoglistUtils.h"
#include "AppUtils.h"

IMPLEMENT_DYNAMIC(CRefLogList, CGitLogList)

CRefLogList::CRefLogList()
{
	m_ColumnRegKey = L"reflog";
	this->m_ContextMenuMask |= this->GetContextMenuBit(ID_LOG);
	this->m_ContextMenuMask &= ~GetContextMenuBit(ID_COMPARETWOCOMMITCHANGES);
}

void CRefLogList::InsertRefLogColumn()
{
	CString temp;

	Init();
	SetStyle();

	static UINT normal[] =
	{
		IDS_HASH,
		IDS_REF,
		IDS_ACTION,
		IDS_MESSAGE,
		IDS_STATUSLIST_COLDATE,
	};

	static int with[] =
	{
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		ICONITEMBORDER+16*4,
		LOGLIST_MESSAGE_MIN,
		ICONITEMBORDER+16*4,
	};
	m_dwDefaultColumns = 0xFFFF;

	SetRedraw(false);

	m_ColumnManager.SetNames(normal, _countof(normal));
	m_ColumnManager.ReadSettings(m_dwDefaultColumns, 0, m_ColumnRegKey + L"loglist", _countof(normal), with);

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
	lstrcpyn(pItem->pszText, L"", pItem->cchTextMax);

	bool bOutOfRange = pItem->iItem >= static_cast<int>(m_arShownList.size());

	*pResult = 0;
	if (m_bNoDispUpdates || bOutOfRange)
		return;

	// Which item number?
	GitRevLoglist* pLogEntry = m_arShownList.SafeGetAt(pItem->iItem);

	CString temp;

	// Which column?
	switch (pItem->iSubItem)
	{
	case REFLOG_HASH:
		if (pLogEntry)
		{
			lstrcpyn(pItem->pszText,pLogEntry->m_CommitHash.ToString(), pItem->cchTextMax - 1);
		}
		break;
	case REFLOG_REF:
		if(pLogEntry)
			lstrcpyn(pItem->pszText, pLogEntry->m_Ref, pItem->cchTextMax - 1);
		break;
	case REFLOG_ACTION:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(pLogEntry->m_RefAction), pItem->cchTextMax - 1);
		break;
	case REFLOG_MESSAGE:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(pLogEntry->GetSubject().Trim()), pItem->cchTextMax - 1);
		break;
	case REFLOG_DATE:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, static_cast<LPCTSTR>(CLoglistUtils::FormatDateAndTime(pLogEntry->GetCommitterDate(), m_DateFormat, true, m_bRelativeTimes)), pItem->cchTextMax - 1);
		break;

	default:
		ASSERT(false);
	}
}

void CRefLogList::OnNMDblclkLoglist(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	// a double click on an entry in the revision list has happened
	*pResult = 0;

	POSITION pos = GetFirstSelectedItemPosition();
	int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	auto pSelLogEntry = m_arShownList.SafeGetAt(indexNext);
	if (!pSelLogEntry)
		return;

	CString cmdline;
	cmdline.Format(L"/command:log /path:\"%s\" /endrev:%s", static_cast<LPCTSTR>(g_Git.CombinePath(m_Path)), static_cast<LPCTSTR>(pSelLogEntry->m_CommitHash.ToString()));
	CAppUtils::RunTortoiseGitProc(cmdline);
}

void CRefLogList::OnNMCustomdrawLoglist(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	// Take the default processing unless we set this to something else below.
	*pResult = CDRF_DODEFAULT;
}

BOOL CRefLogList::OnToolTipText(UINT /*id*/, NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	return FALSE;
}
