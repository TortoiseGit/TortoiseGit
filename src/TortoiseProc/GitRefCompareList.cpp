// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2016 - TortoiseGit

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
#include "GitRefCompareList.h"
#include "registry.h"
#include "UnicodeUtils.h"
#include "IconMenu.h"
#include "AppUtils.h"
#include "..\TortoiseShell\resource.h"
#include "LoglistCommonResource.h"

IMPLEMENT_DYNAMIC(CGitRefCompareList, CHintCtrl<CListCtrl>)

BEGIN_MESSAGE_MAP(CGitRefCompareList, CHintCtrl<CListCtrl>)
	ON_WM_CONTEXTMENU()
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
	, colChange(0)
	, colOldHash(0)
	, colOldMessage(0)
	, colNewHash(0)
	, colNewMessage(0)
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
		entry.oldHash = oldHash->ToString().Left(g_Git.GetShortHASHLength());
	if (newHash)
		entry.newHash = newHash->ToString().Left(g_Git.GetShortHASHLength());

	CAutoCommit oldCommit;
	if (oldHash)
	{
		if (!git_commit_lookup(oldCommit.GetPointer(), repo, (const git_oid *)&oldHash->m_hash))
			entry.oldMessage = GetCommitMessage(oldCommit);
	}

	CAutoCommit newCommit;
	if (newHash)
	{
		if (!git_commit_lookup(newCommit.GetPointer(), repo, (const git_oid *)&newHash->m_hash))
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
			if (!git_graph_ahead_behind(&ahead, &behind, repo, (const git_oid *)&newHash->m_hash, (const git_oid *)&oldHash->m_hash))
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
	return (int)m_RefList.size() - 1;
}

void CGitRefCompareList::Show()
{
	DeleteAllItems();
	std::sort(m_RefList.begin(), m_RefList.end(), SortPredicate);
	int index = 0;
	for (const auto& entry : m_RefList)
	{
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
		SetItemText(index, colChange, entry.change);
		SetItemText(index, colOldHash, entry.oldHash);
		SetItemText(index, colOldMessage, entry.oldMessage);
		SetItemText(index, colNewHash, entry.newHash);
		SetItemText(index, colNewMessage, entry.newMessage);
		index++;
	}
}

void CGitRefCompareList::Clear()
{
	m_RefList.clear();
	DeleteAllItems();
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
	if (selIndex < 0 || (size_t)selIndex >= m_RefList.size())
		return;

	CString refName = m_RefList[selIndex].fullName;
	CString oldHash = m_RefList[selIndex].oldHash;
	CString newHash = m_RefList[selIndex].newHash;
	CIconMenu popup;
	popup.CreatePopupMenu();
	CString logStr;
	if (!oldHash.IsEmpty())
	{
		logStr.Format(IDS_SHOWLOG_OF, (LPCTSTR)oldHash);
		popup.AppendMenuIcon(IDGITRCL_OLDLOG, logStr, IDI_LOG);
	}
	if (!newHash.IsEmpty() && oldHash != newHash)
	{
		logStr.Format(IDS_SHOWLOG_OF, (LPCTSTR)newHash);
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
			sCmd.Format(L"/command:log /path:\"%s\" /endrev:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir, cmd == IDGITRCL_OLDLOG ? (LPCTSTR)oldHash : (LPCTSTR)newHash);
			CAppUtils::RunTortoiseGitProc(sCmd);
			break;
		}
		case IDGITRCL_COMPARE:
		{
			CString sCmd;
			sCmd.Format(L"/command:showcompare /path:\"%s\" /revision1:\"%s\" /revision2:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir, (LPCTSTR)oldHash, (LPCTSTR)newHash);
			if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
				sCmd += L" /alternative";
			CAppUtils::RunTortoiseGitProc(sCmd);
			break;
		}
		case IDGITRCL_REFLOG:
		{
			CString sCmd;
			sCmd.Format(L"/command:reflog /path:\"%s\" /ref:\"%s\"", (LPCTSTR)g_Git.m_CurrentDir, (LPCTSTR)refName);
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

