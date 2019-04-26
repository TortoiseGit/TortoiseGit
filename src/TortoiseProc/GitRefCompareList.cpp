// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2019 - TortoiseGit

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
// GitRefCompareList.cpp : implementation file
//
#include "stdafx.h"
#include "resource.h"
#include "Git.h"
#include "GitRefCompareList.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "IconMenu.h"
#include "AppUtils.h"
#include "../TortoiseShell/resource.h"
#include "LoglistCommonResource.h"

IMPLEMENT_DYNAMIC(CGitRefCompareList, CHintCtrl<CListCtrl>)

BEGIN_MESSAGE_MAP(CGitRefCompareList, CHintCtrl<CListCtrl>)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
END_MESSAGE_MAP()

BOOL CGitRefCompareList::m_bSortLogical = FALSE;

enum IDGITRCL
{
	IDGITRCL_OLDLOG = 1,
	IDGITRCL_NEWLOG,
	IDGITRCL_COMPARE,
	IDGITRCL_REFLOG,
};

enum IDGITRCLH
{
	IDGITRCLH_HIDEUNCHANGED = 1,
};

CGitRefCompareList::CGitRefCompareList()
	: CHintCtrl<CListCtrl>()
	, colRef(0)
	, colRefType(0)
	, colChange(0)
	, colOldHash(0)
	, colOldMessage(0)
	, colNewHash(0)
	, colNewMessage(0)
	, m_bAscending(false)
	, m_nSortedColumn(-1)
{
	m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
	if (m_bSortLogical)
		m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
	m_bHideUnchanged = CRegDWORD(L"Software\\TortoiseGit\\RefCompareHideUnchanged", FALSE);
}

void CGitRefCompareList::Init()
{
	int index = 0;
	colRef = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_REF)));
	colRefType = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_REF)));
	colChange = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_CHANGETYPE)));
	colOldHash = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_OLDHASH)));
	colOldMessage = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_OLDMESSAGE)));
	colNewHash = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_NEWHASH)));
	colNewMessage = InsertColumn(index++,CString(MAKEINTRESOURCE(IDS_NEWMESSAGE)));
	for (int i = 0; i < index; ++i)
		SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);
	SetColumnWidth(colRef, 130);

	CImageList *imagelist = new CImageList();
	imagelist->Create(IDB_BITMAP_REFTYPE, 16, 3, RGB(255, 255, 255));
	SetImageList(imagelist, LVSIL_SMALL);

	SetWindowTheme(m_hWnd, L"Explorer", nullptr);
}

int CGitRefCompareList::AddEntry(git_repository* repo, const CString& ref, const CGitHash* oldHash, const CGitHash* newHash)
{
	RefEntry entry;
	entry.fullName = ref;
	entry.shortName = CGit::GetShortName(ref, &entry.refType);
	if (oldHash)
		entry.oldHash = oldHash->ToString(g_Git.GetShortHASHLength());
	if (newHash)
		entry.newHash = newHash->ToString(g_Git.GetShortHASHLength());

	CAutoCommit oldCommit;
	if (oldHash)
	{
		if (!git_commit_lookup(oldCommit.GetPointer(), repo, *oldHash))
			entry.oldMessage = GetCommitMessage(oldCommit);
	}

	CAutoCommit newCommit;
	if (newHash)
	{
		if (!git_commit_lookup(newCommit.GetPointer(), repo, *newHash))
			entry.newMessage = GetCommitMessage(newCommit);
	}

	if (oldHash && newHash)
	{
		if (*oldHash == *newHash)
		{
			entry.change.LoadString(IDS_SAME);
			entry.changeType = ChangeType::Same;
		}
		else
		{
			size_t ahead = 0, behind = 0;
			if (!git_graph_ahead_behind(&ahead, &behind, repo, *newHash, *oldHash))
			{
				CString change;
				if (ahead > 0 && behind == 0)
				{
					entry.change.Format(IDS_FORWARDN, ahead);
					entry.changeType = ChangeType::FastForward;
				}
				else if (ahead == 0 && behind > 0)
				{
					entry.change.Format(IDS_REWINDN, behind);
					entry.changeType = ChangeType::Rewind;
				}
				else
				{
					git_time_t oldTime = git_commit_committer(oldCommit)->when.time;
					git_time_t newTime = git_commit_committer(newCommit)->when.time;
					if (oldTime < newTime)
					{
						entry.change.LoadString(IDS_SUBMODULEDIFF_NEWERTIME);
						entry.changeType = ChangeType::NewerTime;
					}
					else if (oldTime > newTime)
					{
						entry.change.LoadString(IDS_SUBMODULEDIFF_OLDERTIME);
						entry.changeType = ChangeType::OlderTime;
					}
					else
					{
						entry.change.LoadString(IDS_SUBMODULEDIFF_SAMETIME);
						entry.changeType = ChangeType::SameTime;
					}
				}
			}
		}
	}
	else if (oldHash)
	{
		entry.change.LoadString(IDS_DELETED);
		entry.changeType = ChangeType::Deleted;
	}
	else if (newHash)
	{
		entry.change.LoadString(IDS_NEW);
		entry.changeType = ChangeType::New;
	}

	m_RefList.push_back(entry);
	return static_cast<int>(m_RefList.size()) - 1;
}

inline static bool StringComparePredicate(bool sortLogical, const CString& e1, const CString& e2)
{
	if (sortLogical)
		return StrCmpLogicalW(e1, e2) < 0;
	return e1.Compare(e2) < 0;
}

static CString RefTypeString(CGit::REF_TYPE reftype)
{
	CString type;
	switch (reftype)
	{
	case CGit::REF_TYPE::LOCAL_BRANCH:
		type.LoadString(IDS_PROC_BRANCH);
		break;
	case CGit::REF_TYPE::REMOTE_BRANCH:
		type.LoadString(IDS_PROC_REMOTEBRANCH);
		break;
	case CGit::REF_TYPE::ANNOTATED_TAG:
	case CGit::REF_TYPE::TAG:
		type.LoadString(IDS_PROC_TAG);
		break;
	}
	return type;
}

void CGitRefCompareList::Show()
{
	{
		CString pleaseWait;
		pleaseWait.LoadString(IDS_PROGRESSWAIT);
		ShowText(pleaseWait, true);
	}
	SetRedraw(false);
	DeleteAllItems();

	if (m_nSortedColumn >= 0)
	{
		auto predicate = [](bool sortLogical, int sortColumn, const RefEntry& e1, const RefEntry& e2)
		{
			switch (sortColumn)
			{
			case 0:
				return StringComparePredicate(sortLogical, e1.shortName, e2.shortName);
				break;
			case 1:
				return StringComparePredicate(false, RefTypeString(e1.refType), RefTypeString(e2.refType));
				break;
			case 2:
				return StringComparePredicate(false, e1.change, e2.change);
				break;
			case 3:
				return e1.oldHash.Compare(e2.oldHash) < 0;
				break;
			case 4:
				return StringComparePredicate(sortLogical, e1.oldMessage, e2.oldMessage);
				break;
			case 5:
				return e1.newHash.Compare(e2.newHash) < 0;
				break;
			case 6:
				return StringComparePredicate(sortLogical, e1.newMessage, e2.newMessage);
				break;
			}
			return false;
		};

		if (m_bAscending)
			std::stable_sort(m_RefList.begin(), m_RefList.end(), std::bind(predicate, m_bSortLogical, m_nSortedColumn, std::placeholders::_1, std::placeholders::_2));
		else
			std::stable_sort(m_RefList.begin(), m_RefList.end(), std::bind(predicate, m_bSortLogical, m_nSortedColumn, std::placeholders::_2, std::placeholders::_1));
	}
	else
		std::sort(m_RefList.begin(), m_RefList.end(), SortPredicate);

	int index = 0;
	int listIdx = -1;
	for (const auto& entry : m_RefList)
	{
		++listIdx;
		if (entry.changeType == ChangeType::Same && m_bHideUnchanged)
			continue;

		int nImage = -1;
		if (entry.refType == CGit::REF_TYPE::LOCAL_BRANCH)
			nImage = 1;
		else if (entry.refType == CGit::REF_TYPE::REMOTE_BRANCH)
			nImage = 2;
		else if (entry.refType == CGit::REF_TYPE::ANNOTATED_TAG || entry.refType == CGit::REF_TYPE::TAG)
			nImage = 0;
		InsertItem(index, entry.shortName, nImage);
		SetItemText(index, colRefType, RefTypeString(entry.refType));
		SetItemText(index, colChange, entry.change);
		SetItemText(index, colOldHash, entry.oldHash);
		SetItemText(index, colOldMessage, entry.oldMessage);
		SetItemText(index, colNewHash, entry.newHash);
		SetItemText(index, colNewMessage, entry.newMessage);
		SetItemData(index, listIdx);
		++index;
	}

	auto pHeader = GetHeaderCtrl();
	HDITEM HeaderItem = { 0 };
	HeaderItem.mask = HDI_FORMAT;
	for (int i = 0; i < pHeader->GetItemCount(); ++i)
	{
		pHeader->GetItem(i, &HeaderItem);
		HeaderItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
		pHeader->SetItem(i, &HeaderItem);
	}
	if (m_nSortedColumn >= 0)
	{
		pHeader->GetItem(m_nSortedColumn, &HeaderItem);
		HeaderItem.fmt |= (m_bAscending ? HDF_SORTUP : HDF_SORTDOWN);
		pHeader->SetItem(m_nSortedColumn, &HeaderItem);
	}
	SetRedraw(true);

	if (!index)
	{
		CString empty;
		empty.LoadString(IDS_COMPAREREV_NODIFF);
		ShowText(empty, true);
	}
	else
		ShowText(L"", true);
}

void CGitRefCompareList::Clear()
{
	m_RefList.clear();
	DeleteAllItems();
}

void CGitRefCompareList::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;

	if (m_RefList.empty())
		return;

	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;

	Show();
}

void CGitRefCompareList::OnContextMenu(CWnd *pWnd, CPoint point)
{
	if (pWnd == this)
	{
		OnContextMenuList(pWnd, point);
	}
	else if (pWnd == GetHeaderCtrl())
	{
		OnContextMenuHeader(pWnd, point);
	}
}

void CGitRefCompareList::OnContextMenuList(CWnd * /*pWnd*/, CPoint point)
{
	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return;

	int index = static_cast<int>(GetItemData(selIndex));
	if (index < 0 || static_cast<size_t>(index) >= m_RefList.size())
		return;

	CString refName = m_RefList[index].fullName;
	CString oldHash = m_RefList[index].oldHash;
	CString newHash = m_RefList[index].newHash;
	CIconMenu popup;
	popup.CreatePopupMenu();
	CString logStr;
	if (!oldHash.IsEmpty())
	{
		logStr.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(oldHash));
		popup.AppendMenuIcon(IDGITRCL_OLDLOG, logStr, IDI_LOG);
	}
	if (!newHash.IsEmpty() && oldHash != newHash)
	{
		logStr.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(newHash));
		popup.AppendMenuIcon(IDGITRCL_NEWLOG, logStr, IDI_LOG);
	}
	if (!oldHash.IsEmpty() && !newHash.IsEmpty() && oldHash != newHash)
		popup.AppendMenuIcon(IDGITRCL_COMPARE, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);
	popup.AppendMenuIcon(IDGITRCL_REFLOG, IDS_MENUREFLOG, IDI_LOG);

	int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
	AfxGetApp()->DoWaitCursor(1);
	switch (cmd)
	{
		case IDGITRCL_OLDLOG:
		case IDGITRCL_NEWLOG:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\" /endrev:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), cmd == IDGITRCL_OLDLOG ? static_cast<LPCTSTR>(oldHash) : static_cast<LPCTSTR>(newHash));
			CAppUtils::RunTortoiseGitProc(sCmd);
			break;
		}
		case IDGITRCL_COMPARE:
		{
			CString sCmd;
			sCmd.Format(L"/command:showcompare /path:\"%s\" /revision1:\"%s\" /revision2:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(oldHash), static_cast<LPCTSTR>(newHash));
			if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
				sCmd += L" /alternative";
			CAppUtils::RunTortoiseGitProc(sCmd);
			break;
		}
		case IDGITRCL_REFLOG:
		{
			CString sCmd;
			sCmd.Format(L"/command:reflog /path:\"%s\" /ref:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(refName));
			CAppUtils::RunTortoiseGitProc(sCmd);
			break;
		}
	}
	AfxGetApp()->DoWaitCursor(-1);
}

static void AppendMenuChecked(CMenu &menu, UINT nTextID, UINT_PTR nItemID, BOOL checked = FALSE, BOOL enabled = TRUE)
{
	CString text;
	text.LoadString(nTextID);
	menu.AppendMenu(MF_STRING | (enabled ? MF_ENABLED : MF_DISABLED) | (checked ? MF_CHECKED : MF_UNCHECKED), nItemID, text);
}

void CGitRefCompareList::OnContextMenuHeader(CWnd * /*pWnd*/, CPoint point)
{
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		AppendMenuChecked(popup, IDS_HIDEUNCHANGED, IDGITRCLH_HIDEUNCHANGED, m_bHideUnchanged);

		int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
		switch (selection)
		{
			case IDGITRCLH_HIDEUNCHANGED:
				m_bHideUnchanged = !m_bHideUnchanged;
				Show();
				break;
		}
	}
}

CString CGitRefCompareList::GetCommitMessage(git_commit *commit)
{
	int encode = CP_UTF8;
	const char *encodingString = git_commit_message_encoding(commit);
	if (encodingString != nullptr)
		encode = CUnicodeUtils::GetCPCode(CUnicodeUtils::GetUnicode(encodingString));

	CString message = CUnicodeUtils::GetUnicode(git_commit_message(commit), encode);
	int start = 0;
	message = message.Tokenize(L"\n", start);
	return message;
}

bool CGitRefCompareList::SortPredicate(const RefEntry &e1, const RefEntry &e2)
{
	if (e1.changeType < e2.changeType)
		return true;
	if (e1.changeType > e2.changeType)
		return false;
	if (m_bSortLogical)
		return StrCmpLogicalW(e1.fullName, e2.fullName) < 0;
	return e1.fullName.Compare(e2.fullName) < 0;
}

ULONG CGitRefCompareList::GetGestureStatus(CPoint /*ptTouch*/)
{
	return 0;
}
