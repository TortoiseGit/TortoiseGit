// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2013, 2015-2016, 2018-2019, 2021-2023 - TortoiseGit

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
// PathListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PatchListCtrl.h"
#include "IconMenu.h"
#include "AppUtils.h"
#include "Git.h"
#include "AppUtils.h"
// CPatchListCtrl

IMPLEMENT_DYNAMIC(CPatchListCtrl, CListCtrl)

CPatchListCtrl::CPatchListCtrl()
: m_ContextMenuMask(0xFFFFFFFF)
{
}

CPatchListCtrl::~CPatchListCtrl()
{
}

void CPatchListCtrl::PreSubclassWindow()
{
	__super::PreSubclassWindow();

	// use the default font, create a copy of it and
	// change the copy to BOLD (leave the rest of the font
	// the same)
	LOGFONT lf = { 0 };
	GetFont()->GetLogFont(&lf);
	lf.lfWeight = FW_BOLD;
	m_boldFont.CreateFontIndirect(&lf);
}

BEGIN_MESSAGE_MAP(CPatchListCtrl, CListCtrl)
	ON_NOTIFY_REFLECT(NM_DBLCLK, &CPatchListCtrl::OnNMDblclk)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, &CPatchListCtrl::OnNMCustomdraw)
	ON_WM_DROPFILES()
END_MESSAGE_MAP()



// CPatchListCtrl message handlers



void CPatchListCtrl::OnNMDblclk(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	*pResult = 0;

	CPoint point(pNMItemActivate->ptAction);
	UINT uFlags = 0;
	HitTest(point, &uFlags);
	if (uFlags == LVHT_ONITEMSTATEICON)
		return;

	CString path=GetItemText(pNMItemActivate->iItem,0);
	CTGitPath gitpath;
	gitpath.SetFromWin(path);

	CAppUtils::StartUnifiedDiffViewer(path, gitpath.GetFilename(), 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
}

void CPatchListCtrl::OnContextMenu(CWnd* /*pWnd*/, CPoint point)
{
	int selected=this->GetSelectedCount();
	POSITION pos=this->GetFirstSelectedItemPosition();
	auto index = this->GetNextSelectedItem(pos);

	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		if(selected == 1)
		{
			if( m_ContextMenuMask&GetMenuMask(MENU_VIEWPATCH))
				popup.AppendMenuIcon(MENU_VIEWPATCH, IDS_MENU_VIEWPATCH, 0);

			if( m_ContextMenuMask&GetMenuMask(MENU_VIEWWITHMERGE))
				popup.AppendMenuIcon(MENU_VIEWWITHMERGE, IDS_MENU_VIEWWITHMERGE, 0);

			popup.SetDefaultItem(MENU_VIEWPATCH, FALSE);
		}
		if(selected >= 1)
		{
			if( m_ContextMenuMask&GetMenuMask(MENU_SENDMAIL))
				popup.AppendMenuIcon(MENU_SENDMAIL, IDS_MENU_SENDMAIL, IDI_MENUSENDMAIL);

			if( m_ContextMenuMask&GetMenuMask(MENU_APPLY))
				popup.AppendMenuIcon(MENU_APPLY, IDS_MENU_APPLY, 0);
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);

		switch (cmd)
		{
		case MENU_VIEWPATCH:
			{
				CString path=GetItemText(index,0);
				CTGitPath gitpath;
				gitpath.SetFromWin(path);

				CAppUtils::StartUnifiedDiffViewer(path, gitpath.GetFilename(), 0, !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
				break;
			}
		case MENU_VIEWWITHMERGE:
			{
				CString path=GetItemText(index,0);
				CTGitPath gitpath;
				gitpath.SetFromWin(path);

				CTGitPath dir;
				dir.SetFromGit(g_Git.m_CurrentDir);

				CAppUtils::StartExtPatch(gitpath,dir);
				break;
			}
		case MENU_SENDMAIL:
			{
				LaunchProc(L"sendmail");
				break;
			}
		case MENU_APPLY:
			{
				LaunchProc(L"importpatch");

				break;
			}
		default:
			break;
		}
	}
}

int CPatchListCtrl::LaunchProc(const CString& command)
{
	CString tempfile=GetTempFile();
	if (tempfile.IsEmpty())
	{
		MessageBox(L"Could not create temp file.", L"TortoiseGit", MB_OK | MB_ICONERROR);
		return -1;
	}

	CTGitPathList paths;
	POSITION pos = GetFirstSelectedItemPosition();
	while(pos)
	{
		int index = this->GetNextSelectedItem(pos);
		paths.AddPath(GetItemText(index, 0));
	}
	if (!paths.WriteToFile(tempfile, false))
	{
		MessageBox(L"Could not write to temp file.", L"TortoiseGit", MB_OK | MB_ICONERROR);
		return -1;
	}

	CString cmd = L"/command:";
	cmd += command;
	cmd += L" /pathfile:\"";
	cmd += tempfile;
	cmd += L"\" /deletepathfile";
	if (!CAppUtils::RunTortoiseGitProc(cmd))
	{
		MessageBox(L"Could not start TortoiseGitProc.exe.", L"TortoiseGit", MB_ICONERROR);
		return -1;
	}
	return 0;
}

void CPatchListCtrl::OnNMCustomdraw(NMHDR *pNMHDR, LRESULT *pResult)
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

			DWORD_PTR data = this->GetItemData(static_cast<int>(pNMCD->nmcd.dwItemSpec));
			if(data & (STATUS_APPLY_FAIL | STATUS_APPLY_SUCCESS | STATUS_APPLY_SKIP))
				pNMCD->clrTextBk = RGB(200,200,200);

			switch(data & STATUS_MASK)
			{
			case STATUS_APPLY_SUCCESS:
				pNMCD->clrText = RGB(0,128,0);
				break;
			case STATUS_APPLY_FAIL:
				pNMCD->clrText = RGB(255,0,0);
				break;
			case STATUS_APPLY_SKIP:
				pNMCD->clrText = RGB(128,64,0);
				break;
			}

			if(data & STATUS_APPLYING)
			{
				SelectObject(pNMCD->nmcd.hdc, m_boldFont.GetSafeHandle());
				*pResult = CDRF_NOTIFYSUBITEMDRAW | CDRF_NEWFONT;
			}

		}
		break;
	}
	*pResult = CDRF_DODEFAULT;
}

void CPatchListCtrl::OnDropFiles(HDROP hDropInfo)
{
	UINT nNumFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, nullptr, 0);
	for (UINT i = 0; i < nNumFiles; ++i)
	{
		CString file;
		DragQueryFile(hDropInfo, i, CStrBuf(file, MAX_PATH), MAX_PATH);
		if (PathIsDirectory(file))
			continue;

		// no duplicates
		LVFINDINFO lvInfo;
		lvInfo.flags = LVFI_STRING;
		lvInfo.psz = file;
		if (FindItem(&lvInfo, -1) != -1)
			continue;

		int index = InsertItem(GetItemCount(), file);
		if (index >= 0)
			SetCheck(index, true);
	}
	DragFinish(hDropInfo);
	SetColumnWidth(0, LVSCW_AUTOSIZE);
}
