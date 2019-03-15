// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019 - TortoiseGit

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
// PatchViewDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "PatchViewDlg.h"
#include "CommonAppUtils.h"
#include "StringUtils.h"

// CPatchViewDlg dialog

IMPLEMENT_DYNAMIC(CPatchViewDlg, CDialog)

CPatchViewDlg::CPatchViewDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CPatchViewDlg::IDD, pParent)
	, m_ParentDlg(nullptr)
	, m_hAccel(nullptr)
	, m_bShowFindBar(false)
	, m_nPopupSave(0)
{
}

CPatchViewDlg::~CPatchViewDlg()
{
}

void CPatchViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PATCH, m_ctrlPatchView);
}

BEGIN_MESSAGE_MAP(CPatchViewDlg, CDialog)
	ON_WM_SIZE()
	ON_WM_MOVING()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_COMMAND(IDM_SHOWFINDBAR, OnShowFindBar)
	ON_COMMAND(IDM_FINDEXIT, OnEscape)
	ON_COMMAND(IDM_FINDNEXT, OnFindNext)
	ON_COMMAND(IDM_FINDPREV, OnFindPrev)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDEXIT, OnFindExitMessage)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDNEXT, OnFindNextMessage)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDPREV, OnFindPrevMessage)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDRESET, OnFindResetMessage)
	ON_WM_SYSCOLORCHANGE()
END_MESSAGE_MAP()

// CPatchViewDlg message handlers

BOOL CPatchViewDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	m_ctrlPatchView.Init(-1);

	m_ctrlPatchView.SetUDiffStyle();

	m_ctrlPatchView.RegisterContextMenuHandler(this);

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_ACC_PATCHVIEW));

	m_FindBar.Create(IDD_FINDBAR, this);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

void CPatchViewDlg::SetText(const CString& text)
{
	m_ctrlPatchView.Call(SCI_SETREADONLY, FALSE);
	m_ctrlPatchView.SetText(text);
	m_ctrlPatchView.Call(SCI_SETREADONLY, TRUE);
	if (!text.IsEmpty())
	{
		m_ctrlPatchView.Call(SCI_GOTOPOS, 0);
		CRect rect;
		m_ctrlPatchView.GetClientRect(rect);
		m_ctrlPatchView.Call(SCI_SETSCROLLWIDTH, rect.Width() - 4);
	}
}

void CPatchViewDlg::ClearView()
{
	SetText(CString());
}

void CPatchViewDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialog::OnSize(nType, cx, cy);

	if (this->IsWindowVisible())
	{
		CRect rect;
		GetClientRect(rect);
		GetDlgItem(IDC_PATCH)->MoveWindow(0, 0, cx, cy);

		if (m_bShowFindBar)
		{
			::SetWindowPos(m_ctrlPatchView.GetSafeHwnd(), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top - 30, SWP_SHOWWINDOW);
			::SetWindowPos(m_FindBar.GetSafeHwnd(), HWND_TOP, rect.left, rect.bottom - 30, rect.right - rect.left, 30, SWP_SHOWWINDOW);
		}
		else
		{
			::SetWindowPos(m_ctrlPatchView.GetSafeHwnd(), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
			m_FindBar.ShowWindow(SW_HIDE);
		}

		CRect rect2;
		m_ctrlPatchView.GetClientRect(rect2);
		m_ctrlPatchView.Call(SCI_SETSCROLLWIDTH, rect2.Width() - 4);
	}
}

void CPatchViewDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
#define STICKYSIZE 5
	RECT parentRect;
	m_ParentDlg->GetPatchViewParentWnd()->GetWindowRect(&parentRect);
	if (abs(parentRect.right - pRect->left) < STICKYSIZE)
	{
		int width = pRect->right - pRect->left;
		pRect->left = parentRect.right;
		pRect->right = pRect->left + width;
	}
	CDialog::OnMoving(fwSide, pRect);
}

void CPatchViewDlg::OnClose()
{
	CDialog::OnClose();
	m_ParentDlg->TogglePatchView();
}

BOOL CPatchViewDlg::PreTranslateMessage(MSG* pMsg)
{
	if (m_hAccel)
	{
		if (TranslateAccelerator(m_hWnd, m_hAccel, pMsg))
			return TRUE;
	}
	return __super::PreTranslateMessage(pMsg);
}

void CPatchViewDlg::OnEscape()
{
	if (::IsWindowVisible(m_FindBar))
	{
		OnFindExit();
		return;
	}
	SendMessage(WM_CLOSE);
}

void CPatchViewDlg::OnShowFindBar()
{
	m_bShowFindBar = true;
	m_FindBar.ShowWindow(SW_SHOW);
	RECT rect;
	GetClientRect(&rect);
	::SetWindowPos(m_ctrlPatchView.GetSafeHwnd(), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top - 30, SWP_SHOWWINDOW);
	::SetWindowPos(m_FindBar, HWND_TOP, rect.left, rect.bottom - 30, rect.right - rect.left, 30, SWP_SHOWWINDOW);
	m_FindBar.SetFocusTextBox();
	m_ctrlPatchView.Call(SCI_SETSELECTIONSTART, 0);
	m_ctrlPatchView.Call(SCI_SETSELECTIONEND, 0);
	m_ctrlPatchView.Call(SCI_SEARCHANCHOR);
}

void CPatchViewDlg::OnFindNext()
{
	m_ctrlPatchView.Call(SCI_CHARRIGHT);
	m_ctrlPatchView.Call(SCI_SEARCHANCHOR);
	if (m_ctrlPatchView.Call(SCI_SEARCHNEXT, m_FindBar.IsMatchCase() ? SCFIND_MATCHCASE : 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(m_FindBar.GetFindText())))) == -1)
		FlashWindowEx(FLASHW_ALL, 3, 100);
	m_ctrlPatchView.Call(SCI_SCROLLCARET);
}

void CPatchViewDlg::OnFindPrev()
{
	m_ctrlPatchView.Call(SCI_SEARCHANCHOR);
	if (m_ctrlPatchView.Call(SCI_SEARCHPREV, m_FindBar.IsMatchCase() ? SCFIND_MATCHCASE : 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(CUnicodeUtils::GetUTF8(m_FindBar.GetFindText())))) == -1)
		FlashWindowEx(FLASHW_ALL, 3, 100);
	m_ctrlPatchView.Call(SCI_SCROLLCARET);
}

void CPatchViewDlg::OnFindExit()
{
	if (!::IsWindowVisible(m_FindBar))
		return;

	RECT rect;
	GetClientRect(&rect);
	m_bShowFindBar = false;
	m_FindBar.ShowWindow(SW_HIDE);
	::SetWindowPos(m_ctrlPatchView.GetSafeHwnd(), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
	m_ctrlPatchView.SetFocus();
}

void CPatchViewDlg::OnFindReset()
{
	m_ctrlPatchView.Call(SCI_SETSELECTIONSTART, 0);
	m_ctrlPatchView.Call(SCI_SETSELECTIONEND, 0);
	m_ctrlPatchView.Call(SCI_SEARCHANCHOR);
}

LRESULT CPatchViewDlg::OnFindNextMessage(WPARAM, LPARAM)
{
	OnFindNext();
	return 0;
}

LRESULT CPatchViewDlg::OnFindPrevMessage(WPARAM, LPARAM)
{
	OnFindPrev();
	return 0;
}

LRESULT CPatchViewDlg::OnFindExitMessage(WPARAM, LPARAM)
{
	OnFindExit();
	return 0;
}

LRESULT CPatchViewDlg::OnFindResetMessage(WPARAM, LPARAM)
{
	OnFindReset();
	return 0;
}

void CPatchViewDlg::OnSysColorChange()
{
	__super::OnSysColorChange();

	m_ctrlPatchView.SetUDiffStyle();
}

void CPatchViewDlg::OnDestroy()
{
	__super::OnDestroy();
	CRect rect;
	GetWindowRect(&rect);
	CRegStdDWORD(L"Software\\TortoiseGit\\TortoiseProc\\PatchDlgWidth") = rect.Width();
	m_ctrlPatchView.ClearContextMenuHandlers();
}

// CSciEditContextMenuInterface
void CPatchViewDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	CString sMenuItemText;
	sMenuItemText.LoadString(IDS_REPOBROWSE_SAVEAS);
	m_nPopupSave = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupSave, sMenuItemText);
}

bool CPatchViewDlg::HandleMenuItemClick(int cmd, CSciEdit*)
{
	if (cmd == m_nPopupSave)
	{
		CString filename;
		if (CCommonAppUtils::FileOpenSave(filename, nullptr, 0, IDS_PATCHFILEFILTER, false, GetSafeHwnd(), L"diff"))
			CStringUtils::WriteStringToTextFile(filename, m_ctrlPatchView.GetText());
		return true;
	}
	return false;
}
