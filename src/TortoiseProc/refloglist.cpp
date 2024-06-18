// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2011, 2013, 2015-2020, 2023-2024 TortoiseGit

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
#include "DPIAware.h"

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

	auto columnWidth = CDPIAware::Instance().ScaleX(GetSafeHwnd(), ICONITEMBORDER + 16 * 4);
	static int columnWidths[] =
	{
		columnWidth,
		columnWidth,
		columnWidth,
		CDPIAware::Instance().ScaleX(GetSafeHwnd(), LOGLIST_MESSAGE_MIN),
		columnWidth,
	};
	static_assert(_countof(normal) == _countof(columnWidths));
	m_dwDefaultColumns = 0xFFFF;

	SetRedraw(false);

	m_ColumnManager.SetNames(normal, _countof(normal));
	constexpr int columnVersion = 6; // adjust when changing number/names/etc. of columns
	m_ColumnManager.ReadSettings(m_dwDefaultColumns, 0, m_ColumnRegKey + L"loglist", columnVersion, _countof(normal), columnWidths);

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
			lstrcpyn(pItem->pszText, static_cast<LPCWSTR>(pLogEntry->m_RefAction), pItem->cchTextMax - 1);
		break;
	case REFLOG_MESSAGE:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, static_cast<LPCWSTR>(pLogEntry->GetSubject().Trim()), pItem->cchTextMax - 1);
		break;
	case REFLOG_DATE:
		if (pLogEntry)
			lstrcpyn(pItem->pszText, static_cast<LPCWSTR>(CLoglistUtils::FormatDateAndTime(pLogEntry->GetCommitterDate(), m_DateFormat, true, m_bRelativeTimes)), pItem->cchTextMax - 1);
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
	const int indexNext = GetNextSelectedItem(pos);
	if (indexNext < 0)
		return;

	auto pSelLogEntry = m_arShownList.SafeGetAt(indexNext);
	if (!pSelLogEntry)
		return;

	CString cmdline;
	cmdline.Format(L"/command:log /path:\"%s\" /endrev:%s", static_cast<LPCWSTR>(g_Git.CombinePath(m_Path)), static_cast<LPCWSTR>(pSelLogEntry->m_CommitHash.ToString()));
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

void CRefLogList::CopySelectionToClipBoard(int toCopy)
{
	CString sClipdata;
	POSITION pos = GetFirstSelectedItemPosition();
	if (pos)
	{
		CString sRev;
		sRev.LoadString(IDS_LOG_REVISION);
		CString sDate;
		sDate.LoadString(IDS_LOG_DATE);
		CString sMessage;
		sMessage.LoadString(IDS_LOG_MESSAGE);
		bool first = true;
		while (pos)
		{
			auto pLogEntry = m_arShownList.SafeGetAt(GetNextSelectedItem(pos));

			if (toCopy == ID_COPYCLIPBOARDFULL)
			{
				sClipdata.AppendFormat(L"%s: %s\r\n%s: %s\r\n%s: %s%s%s\r\n",
									static_cast<LPCWSTR>(sRev), static_cast<LPCWSTR>(pLogEntry->m_CommitHash.ToString()),
									static_cast<LPCWSTR>(sDate), static_cast<LPCWSTR>(CLoglistUtils::FormatDateAndTime(pLogEntry->GetCommitterDate(), m_DateFormat, true, m_bRelativeTimes)),
									static_cast<LPCWSTR>(sMessage), static_cast<LPCWSTR>(pLogEntry->m_RefAction), pLogEntry->m_RefAction.IsEmpty() ? L"" : L": ", static_cast<LPCWSTR>(pLogEntry->GetSubjectBody(true)));
			}
			else if (toCopy == ID_COPYCLIPBOARDMESSAGES)
			{
				sClipdata += L"* ";
				if (!pLogEntry->m_RefAction.IsEmpty())
				{
					sClipdata += pLogEntry->m_RefAction;
					sClipdata += L": ";
				}
				sClipdata += pLogEntry->GetSubjectBody(true);
				sClipdata += L"\r\n\r\n";
			}
			else
			{
				if (!first)
					sClipdata += L"\r\n";
				sClipdata += pLogEntry->m_CommitHash.ToString();
			}

			first = false;
		}
		CStringUtils::WriteAsciiStringToClipboard(sClipdata, GetSafeHwnd());
	}
}
