// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2019 - TortoiseGit

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
#include "GitTagCompareList.h"
#include "GitRefCompareList.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "IconMenu.h"
#include "AppUtils.h"
#include "../TortoiseShell/resource.h"
#include "LoglistCommonResource.h"
#include "SysProgressDlg.h"
#include "ProgressDlg.h"

IMPLEMENT_DYNAMIC(CGitTagCompareList, CHintCtrl<CListCtrl>)

BEGIN_MESSAGE_MAP(CGitTagCompareList, CHintCtrl<CListCtrl>)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY(HDN_ITEMCLICKA, 0, OnHdnItemclick)
	ON_NOTIFY(HDN_ITEMCLICKW, 0, OnHdnItemclick)
END_MESSAGE_MAP()

BOOL CGitTagCompareList::m_bSortLogical = FALSE;

enum IDGITRCL
{
	IDGITRCL_MYLOG = 1,
	IDGITRCL_THEIRLOG,
	IDGITRCL_COMPARE,
	IDGITRCL_DELETELOCAL,
	IDGITRCL_DELETEREMOTE,
	IDGITRCL_PUSH,
	IDGITRCL_FETCH,
};

enum IDGITRCLH
{
	IDGITRCLH_HIDEUNCHANGED = 1,
};

inline static bool SortPredicate(bool sortLogical, const CString& e1, const CString& e2)
{
	if (sortLogical)
		return StrCmpLogicalW(e1, e2) < 0;
	return e1.Compare(e2) < 0;
}

CGitTagCompareList::CGitTagCompareList()
	: CHintCtrl<CListCtrl>()
	, colTag(0)
	, colDiff(0)
	, colMyHash(0)
	, colMyMessage(0)
	, colTheirHash(0)
	, colTheirMessage(0)
	, m_bAscending(false)
	, m_nSortedColumn(-1)
{
	m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_CURRENT_USER);
	if (m_bSortLogical)
		m_bSortLogical = !CRegDWORD(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer\\NoStrCmpLogical", 0, false, HKEY_LOCAL_MACHINE);
	m_bHideEqual = CRegDWORD(L"Software\\TortoiseGit\\TagCompareHideEqual", FALSE);
}

void CGitTagCompareList::Init()
{
	int index = 0;
	colTag = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_PROC_TAG)));
	colDiff = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_STATUSLIST_COLSTATUS)));
	colMyHash = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_TAGCOMPARE_LOCALHASH)));
	colMyMessage = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_TAGCOMPARE_LOCALMESSAGE)));
	colTheirHash = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_TAGCOMPARE_REMOTEHASH)));
	colTheirMessage = InsertColumn(index++, CString(MAKEINTRESOURCE(IDS_TAGCOMPARE_REMOTEMESSAGE)));

	SetWindowTheme(m_hWnd, L"Explorer", nullptr);

	if (!!CRegDWORD(L"Software\\TortoiseGit\\SortTagsReversed", 0, false, HKEY_LOCAL_MACHINE) || !!CRegDWORD(L"Software\\TortoiseGit\\SortTagsReversed", 0, false, HKEY_CURRENT_USER))
	{
		m_bAscending = false;
		m_nSortedColumn = 0;
	}
}

int CGitTagCompareList::Fill(const CString& remote, CString& err)
{
	m_remote = remote;
	m_TagList.clear();
	DeleteAllItems();
	{
		CString pleaseWait;
		pleaseWait.LoadString(IDS_PROGRESSWAIT);
		ShowText(pleaseWait, true);
	}
	if (!g_Git.m_IsUseLibGit2)
	{
		err = L"Only available with libgit2 enabled";
		return -1;
	}

	REF_VECTOR remoteTags;
	if (g_Git.GetRemoteTags(remote, remoteTags))
	{
		err = g_Git.GetGitLastErr(L"Could not retrieve remote tags.", CGit::GIT_CMD_FETCH);
		return -1;
	}

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		err = CGit::GetLibGit2LastErr(L"Could not open repository.");
		return -1;
	}

	MAP_HASH_NAME hashMap;
	if (CGit::GetMapHashToFriendName(repo, hashMap))
	{
		err = CGit::GetLibGit2LastErr(L"Could not get all refs.");
		return -1;
	}

	REF_VECTOR localTags;
	for (auto it = hashMap.cbegin(); it != hashMap.cend(); ++it)
	{
		const auto& hash = it->first;
		const auto& refs = it->second;
		std::for_each(refs.cbegin(), refs.cend(), [&](const auto& ref)
		{
			if (CStringUtils::StartsWith(ref, L"refs/tags/"))
			{
				auto tagname = ref.Mid(static_cast<int>(wcslen(L"refs/tags/")));
				localTags.emplace_back(TGitRef{ tagname, hash });
				if (CStringUtils::EndsWith(tagname, L"^{}"))
				{
					tagname.Truncate(tagname.GetLength() - static_cast<int>(wcslen(L"^{}")));
					CAutoObject gitObject;
					if (git_revparse_single(gitObject.GetPointer(), repo, CUnicodeUtils::GetUTF8(tagname)))
						return;
					localTags.emplace_back(TGitRef{ tagname, git_object_id(gitObject) });
				}
			}
		});
	}
	std::sort(remoteTags.begin(), remoteTags.end(), std::bind(SortPredicate, !!m_bSortLogical, std::placeholders::_1, std::placeholders::_2));
	std::sort(localTags.begin(), localTags.end(), std::bind(SortPredicate, !!m_bSortLogical, std::placeholders::_1, std::placeholders::_2));

	auto remoteIt = remoteTags.cbegin();
	auto localIt = localTags.cbegin();

	while (remoteIt != remoteTags.cend() && localIt != localTags.cend())
	{
		if (SortPredicate(!!m_bSortLogical, remoteIt->name, localIt->name))
		{
			AddEntry(repo, remoteIt->name, nullptr, &remoteIt->hash);
			++remoteIt;
			continue;
		}

		if (remoteIt->name == localIt->name)
		{
			AddEntry(repo, remoteIt->name, &localIt->hash, &remoteIt->hash);
			++remoteIt;
		}
		else
			AddEntry(repo, localIt->name, &localIt->hash, nullptr);

		++localIt;
	}

	while (remoteIt != remoteTags.cend())
	{
		AddEntry(repo, remoteIt->name, nullptr, &remoteIt->hash);
		++remoteIt;
	}

	while (localIt != localTags.cend())
	{
		AddEntry(repo, localIt->name, &localIt->hash, nullptr);
		++localIt;
	}

	Show();

	CHeaderCtrl * pHeader = GetHeaderCtrl();
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

	return 0;
}

void CGitTagCompareList::AddEntry(git_repository* repo, const CString& tag, const CGitHash* myHash, const CGitHash* theirHash)
{
	TagEntry entry;
	entry.name = tag;
	if (myHash)
		entry.myHash = *myHash;
	if (theirHash)
		entry.theirHash = *theirHash;

	CAutoCommit oldCommit;
	if (myHash)
	{
		if (!git_commit_lookup(oldCommit.GetPointer(), repo, *myHash))
			entry.myMessage = CGitRefCompareList::GetCommitMessage(oldCommit);
	}

	CAutoCommit newCommit;
	if (theirHash)
	{
		if (!git_commit_lookup(newCommit.GetPointer(), repo, *theirHash))
			entry.theirMessage = CGitRefCompareList::GetCommitMessage(newCommit);
	}

	if (myHash && theirHash)
	{
		if (*myHash == *theirHash)
			entry.diffstate.LoadString(IDS_TAGCOMPARE_SAME);
		else
			entry.diffstate.LoadString(IDS_TAGCOMPARE_DIFFER);
	}
	else if (myHash)
		entry.diffstate.LoadString(IDS_TAGCOMPARE_ONLYLOCAL);
	else
		entry.diffstate.LoadString(IDS_TAGCOMPARE_ONLYREMOTE);
	m_TagList.emplace_back(entry);
}

void CGitTagCompareList::Show()
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
		auto predicate = [](bool sortLogical, int sortColumn, const TagEntry& e1, const TagEntry& e2)
		{
			switch (sortColumn)
			{
			case 0:
				return SortPredicate(sortLogical, e1.name, e2.name);
				break;
			case 1:
				return SortPredicate(false, e1.diffstate, e2.diffstate);
				break;
			case 2:
				return e1.myHash < e2.myHash;
				break;
			case 3:
				return SortPredicate(sortLogical, e1.myMessage, e2.myMessage);
				break;
			case 4:
				return e1.theirHash < e2.theirHash;
				break;
			case 5:
				return SortPredicate(sortLogical, e1.theirMessage, e2.theirMessage);
				break;
			}
			return false;
		};

		if (m_bAscending)
			std::stable_sort(m_TagList.begin(), m_TagList.end(), std::bind(predicate, !!m_bSortLogical, m_nSortedColumn, std::placeholders::_1, std::placeholders::_2));
		else
			std::stable_sort(m_TagList.begin(), m_TagList.end(), std::bind(predicate, !!m_bSortLogical, m_nSortedColumn, std::placeholders::_2, std::placeholders::_1));
	}

	int index = 0;
	for (const auto& entry : m_TagList)
	{
		if (entry.myHash == entry.theirHash && m_bHideEqual)
			continue;

		InsertItem(index, entry.name);
		SetItemText(index, colDiff, entry.diffstate);
		if (!entry.myHash.IsEmpty())
			SetItemText(index, colMyHash, entry.myHash.ToString(g_Git.GetShortHASHLength()));
		SetItemText(index, colMyMessage, entry.myMessage);
		if (!entry.theirHash.IsEmpty())
			SetItemText(index, colTheirHash, entry.theirHash.ToString(g_Git.GetShortHASHLength()));
		SetItemText(index, colTheirMessage, entry.theirMessage);
		++index;
	}
	for (int i = 0; i < GetHeaderCtrl()->GetItemCount(); ++i)
		SetColumnWidth(i, LVSCW_AUTOSIZE_USEHEADER);

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

void CGitTagCompareList::OnHdnItemclick(NMHDR *pNMHDR, LRESULT *pResult)
{
	auto phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	*pResult = 0;

	if (m_TagList.empty())
		return;

	if (m_nSortedColumn == phdr->iItem)
		m_bAscending = !m_bAscending;
	else
		m_bAscending = TRUE;
	m_nSortedColumn = phdr->iItem;

	Show();
}

void CGitTagCompareList::OnContextMenu(CWnd *pWnd, CPoint point)
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

void CGitTagCompareList::OnContextMenuList(CWnd * /*pWnd*/, CPoint point)
{
	int selIndex = GetSelectionMark();
	if (selIndex < 0)
		return;

	CString tag = GetItemText(selIndex, colTag);
	if (CStringUtils::EndsWith(tag, L"^{}"))
		tag.Truncate(tag.GetLength() - static_cast<int>(wcslen(L"^{}")));
	CString myHash = GetItemText(selIndex, colMyHash);
	CString theirHash = GetItemText(selIndex, colTheirHash);
	CIconMenu popup;
	popup.CreatePopupMenu();
	CString logStr;
	if (!myHash.IsEmpty())
	{
		logStr.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(myHash));
		popup.AppendMenuIcon(IDGITRCL_MYLOG, logStr, IDI_LOG);
	}

	if (myHash != theirHash)
	{
		if (!theirHash.IsEmpty())
		{
			logStr.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(theirHash));
			popup.AppendMenuIcon(IDGITRCL_THEIRLOG, logStr, IDI_LOG);
		}

		if (!myHash.IsEmpty() && !theirHash.IsEmpty())
			popup.AppendMenuIcon(IDGITRCL_COMPARE, IDS_LOG_POPUP_COMPAREWITHPREVIOUS, IDI_DIFF);

		popup.AppendMenu(MF_SEPARATOR);

		if (!theirHash.IsEmpty())
			popup.AppendMenuIcon(IDGITRCL_FETCH, IDS_MENUFETCH, IDI_UPDATE);

		if (!myHash.IsEmpty())
			popup.AppendMenuIcon(IDGITRCL_PUSH, IDS_MENUPUSH, IDI_COMMIT);

	}
	popup.AppendMenu(MF_SEPARATOR);

	if (!myHash.IsEmpty())
		popup.AppendMenuIcon(IDGITRCL_DELETELOCAL, IDS_DELETE_LOCALTAG, IDI_DELETE);

	if (!theirHash.IsEmpty())
		popup.AppendMenuIcon(IDGITRCL_DELETEREMOTE, IDS_DELETE_REMOTETAG, IDI_DELETE);

	int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
	switch (cmd)
	{
		case IDGITRCL_MYLOG:
		case IDGITRCL_THEIRLOG:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\" /endrev:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), cmd == IDGITRCL_MYLOG ? static_cast<LPCTSTR>(myHash) : static_cast<LPCTSTR>(theirHash));
			CAppUtils::RunTortoiseGitProc(sCmd);
			break;
		}
		case IDGITRCL_COMPARE:
		{
			CString sCmd;
			sCmd.Format(L"/command:showcompare /path:\"%s\" /revision1:\"%s\" /revision2:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(myHash), static_cast<LPCTSTR>(theirHash));
			if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
				sCmd += L" /alternative";
			CAppUtils::RunTortoiseGitProc(sCmd);
			break;
		}
		case IDGITRCL_DELETELOCAL:
		{
			CString csMessage;
			csMessage.Format(IDS_PROC_DELETEBRANCHTAG, static_cast<LPCTSTR>(tag));
			if (MessageBox(csMessage, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) != IDYES)
				return;

			g_Git.DeleteRef(L"refs/tags/" + tag);

			if (CString err; Fill(m_remote, err))
				MessageBox(err, L"TortoiseGit", MB_ICONERROR);
			break;
		}
		case IDGITRCL_DELETEREMOTE:
		{
			CString csMessage;
			csMessage.Format(IDS_PROC_DELETEBRANCHTAG, static_cast<LPCTSTR>(tag));
			if (MessageBox(csMessage, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) != IDYES)
				return;
			CSysProgressDlg sysProgressDlg;
			sysProgressDlg.SetTitle(CString(MAKEINTRESOURCE(IDS_APPNAME)));
			sysProgressDlg.SetLine(1, CString(MAKEINTRESOURCE(IDS_DELETING_REMOTE_REFS)));
			sysProgressDlg.SetLine(2, CString(MAKEINTRESOURCE(IDS_PROGRESSWAIT)));
			sysProgressDlg.SetShowProgressBar(false);
			sysProgressDlg.ShowModal(this, true);

			STRING_VECTOR list;
			list.push_back(L"refs/tags/" + tag);
			if (g_Git.DeleteRemoteRefs(m_remote, list))
			{
				MessageBox(g_Git.GetGitLastErr(L"Could not delete remote tag.", CGit::GIT_CMD_PUSH), L"TortoiseGit", MB_OK | MB_ICONERROR);
				sysProgressDlg.Stop();
				BringWindowToTop();
				return;
			}

			CString err;
			auto ret = Fill(m_remote, err);
			sysProgressDlg.Stop();
			if (ret)
				MessageBox(err, L"TortoiseGit", MB_ICONERROR);

			BringWindowToTop();
			break;
		}
		case IDGITRCL_PUSH:
		{
			CProgressDlg dlg;
			dlg.m_GitCmd.Format(L"git.exe push --force \"%s\" refs/tags/%s", static_cast<LPCTSTR>(m_remote), static_cast<LPCTSTR>(tag));
			dlg.DoModal();

			if (CString err; Fill(m_remote, err))
				MessageBox(err, L"TortoiseGit", MB_ICONERROR);

			break;
		}
		case IDGITRCL_FETCH:
		{
			CProgressDlg dlg;
			dlg.m_GitCmd.Format(L"git.exe fetch \"%s\" refs/tags/%s:refs/tags/%s", static_cast<LPCTSTR>(m_remote), static_cast<LPCTSTR>(tag), static_cast<LPCTSTR>(tag));
			dlg.DoModal();

			if (CString err; Fill(m_remote, err))
				MessageBox(err, L"TortoiseGit", MB_ICONERROR);

			break;
		}
	}
}

static void AppendMenuChecked(CMenu &menu, UINT nTextID, UINT_PTR nItemID, BOOL checked = FALSE, BOOL enabled = TRUE)
{
	CString text;
	text.LoadString(nTextID);
	menu.AppendMenu(MF_STRING | (enabled ? MF_ENABLED : MF_DISABLED) | (checked ? MF_CHECKED : MF_UNCHECKED), nItemID, text);
}

void CGitTagCompareList::OnContextMenuHeader(CWnd * /*pWnd*/, CPoint point)
{
	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		AppendMenuChecked(popup, IDS_HIDEUNCHANGED, IDGITRCLH_HIDEUNCHANGED, m_bHideEqual);

		int selection = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
		switch (selection)
		{
			case IDGITRCLH_HIDEUNCHANGED:
				m_bHideEqual = !m_bHideEqual;
				Show();
				break;
		}
	}
}

ULONG CGitTagCompareList::GetGestureStatus(CPoint /*ptTouch*/)
{
	return 0;
}
