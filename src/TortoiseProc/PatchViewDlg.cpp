// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2017, 2019-2022 - TortoiseGit

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
#include "MessageBox.h"
#include "PatchViewDlg.h"
#include "CommonAppUtils.h"
#include "StringUtils.h"
#include "DPIAware.h"
#include "StagingOperations.h"
#include "Git.h"
#include "EnableStagingTypes.h"

#pragma comment(lib, "Dwmapi.lib")

// CPatchViewDlg dialog

UINT CPatchViewDlg::WM_PARTIALSTAGINGREFRESHPATCHVIEW = RegisterWindowMessage(L"TORTOISEGIT_COMMIT_PARTIALSTAGINGREFRESHPATCHVIEW"); // same string in CommitDlg.cpp!!!

IMPLEMENT_DYNAMIC(CPatchViewDlg, CStandAloneDialog)

#define SEARCHBARHEIGHT 30

CPatchViewDlg::CPatchViewDlg(CWnd* pParent /*=nullptr*/)
	: CStandAloneDialog(CPatchViewDlg::IDD, pParent)
	, m_ParentDlg(nullptr)
	, m_hAccel(nullptr)
	, m_bShowFindBar(false)
	, m_nPopupSave(0)
	, m_nStageHunks(0)
	, m_nStageLines(0)
	, m_nUnstageHunks(0)
	, m_nUnstageLines(0)
	, m_nEnableStagingType(EnableStagingTypes::None)
{
}

CPatchViewDlg::~CPatchViewDlg()
{
}

void CPatchViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CStandAloneDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_PATCH, m_ctrlPatchView);
}

BEGIN_MESSAGE_MAP(CPatchViewDlg, CStandAloneDialog)
	ON_WM_SIZE()
	ON_WM_MOVING()
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_COMMAND(IDM_SHOWFINDBAR, OnShowFindBar)
	ON_COMMAND(IDM_FINDEXIT, OnEscape)
	ON_COMMAND(IDM_FINDNEXT, OnFindNext)
	ON_COMMAND(IDM_FINDPREV, OnFindPrev)
	ON_COMMAND(ID_STAGING_STAGESELECTEDLINES, OnStageLines)
	ON_COMMAND(ID_STAGING_STAGESELECTEDHUNKS, OnStageHunks)
	ON_COMMAND(ID_UNSTAGING_UNSTAGESELECTEDLINES, OnUnstageLines)
	ON_COMMAND(ID_UNSTAGING_UNSTAGESELECTEDHUNKS, OnUnstageHunks)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDEXIT, OnFindExitMessage)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDNEXT, OnFindNextMessage)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDPREV, OnFindPrevMessage)
	ON_REGISTERED_MESSAGE(CFindBar::WM_FINDRESET, OnFindResetMessage)
END_MESSAGE_MAP()

// CPatchViewDlg message handlers

BOOL CPatchViewDlg::OnInitDialog()
{
	CStandAloneDialog::OnInitDialog();

	auto hIcon = LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_PATCH));
	SetIcon(hIcon, TRUE);
	SetIcon(hIcon, FALSE);

	m_ctrlPatchView.Init(-1);

	m_ctrlPatchView.SetUDiffStyle();

	m_ctrlPatchView.Call(SCI_SETSCROLLWIDTH, 1);
	m_ctrlPatchView.Call(SCI_SETSCROLLWIDTHTRACKING, TRUE);

	m_ctrlPatchView.RegisterContextMenuHandler(this);

	m_hAccel = LoadAccelerators(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_ACC_PATCHVIEW));

	m_FindBar.Create(IDD_FINDBAR, this);

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}

// This is intended to be called by the commit window when staging support is enabled there and
// we were invoked becaused the user clicked on "Partial Staging>>" or "Partial Unstaging>>".
// We assume the commit window will later on pass us the output of "git diff" (for staging) or
// "git diff --cached" (for unstaging), of one file only.
// If we were invoked from somewhere else (e.g. the File Diff Dialog), this will never be called
// and all staging menu items will stay disabled.
void CPatchViewDlg::EnableStaging(EnableStagingTypes enableStagingType)
{
	EnableMenuItem(GetMenu()->GetSafeHmenu(), ID_STAGING_STAGESELECTEDHUNKS, enableStagingType == EnableStagingTypes::Staging ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(GetMenu()->GetSafeHmenu(), ID_STAGING_STAGESELECTEDLINES, enableStagingType == EnableStagingTypes::Staging ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(GetMenu()->GetSafeHmenu(), ID_UNSTAGING_UNSTAGESELECTEDHUNKS, enableStagingType == EnableStagingTypes::Unstaging ? MF_ENABLED : MF_DISABLED);
	EnableMenuItem(GetMenu()->GetSafeHmenu(), ID_UNSTAGING_UNSTAGESELECTEDLINES, enableStagingType == EnableStagingTypes::Unstaging ? MF_ENABLED : MF_DISABLED);

	switch (enableStagingType)
	{
	case EnableStagingTypes::None:
		SetWindowText(CString(MAKEINTRESOURCE(IDS_VIEWPATCH)));
		break;
	case EnableStagingTypes::Staging:
		SetWindowText(CString(MAKEINTRESOURCE(IDS_VIEWPATCH_INDEX_WORKTREE)));
		break;
	case EnableStagingTypes::Unstaging:
		SetWindowText(CString(MAKEINTRESOURCE(IDS_VIEWPATCH_HEAD_INDEX)));
		break;
	}

	m_nEnableStagingType = enableStagingType; // This will be used to determine which context menu items to show
}

void CPatchViewDlg::SetText(const CString& text)
{
	m_ctrlPatchView.SetText(text);
	if (!text.IsEmpty())
	{
		m_ctrlPatchView.Call(SCI_GOTOPOS, 0);
		CRect rect;
		m_ctrlPatchView.GetClientRect(rect);
	}
}

void CPatchViewDlg::ClearView()
{
	SetText(CString());
}

static int GetBorderAjustment(HWND parentHWND, const RECT& parentRect)
{
	CRect recta{ 0, 0, 0, 0 };
	if (SUCCEEDED(::DwmGetWindowAttribute(parentHWND, DWMWA_EXTENDED_FRAME_BOUNDS, &recta, sizeof(recta))))
		return 2 * (recta.left - parentRect.left) + 1;

	return 0;
}

void CPatchViewDlg::OnSize(UINT nType, int cx, int cy)
{
	CStandAloneDialog::OnSize(nType, cx, cy);

	if (this->IsWindowVisible())
	{
		CRect rect;
		GetClientRect(rect);
		GetDlgItem(IDC_PATCH)->MoveWindow(0, 0, cx, cy);

		if (m_bShowFindBar)
		{
			::SetWindowPos(m_ctrlPatchView.GetSafeHwnd(), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top - CDPIAware::Instance().ScaleX(SEARCHBARHEIGHT), SWP_SHOWWINDOW);
			::SetWindowPos(m_FindBar.GetSafeHwnd(), HWND_TOP, rect.left, rect.bottom - CDPIAware::Instance().ScaleX(SEARCHBARHEIGHT + 2), rect.right - rect.left, CDPIAware::Instance().ScaleX(SEARCHBARHEIGHT), SWP_SHOWWINDOW);
		}
		else
		{
			::SetWindowPos(m_ctrlPatchView.GetSafeHwnd(), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_SHOWWINDOW);
			m_FindBar.ShowWindow(SW_HIDE);
		}
	}
}

void CPatchViewDlg::OnMoving(UINT fwSide, LPRECT pRect)
{
#define STICKYSIZE 5
	RECT parentRect;
	m_ParentDlg->GetPatchViewParentWnd()->GetWindowRect(&parentRect);

	int adjust = GetBorderAjustment(m_ParentDlg->GetPatchViewParentWnd()->GetSafeHwnd(), parentRect);
	if (abs(parentRect.right - pRect->left - adjust) < STICKYSIZE)
	{
		int width = pRect->right - pRect->left;
		pRect->left = parentRect.right - adjust;
		pRect->right = pRect->left + width;
	}
	CStandAloneDialog::OnMoving(fwSide, pRect);
}

void CPatchViewDlg::ParentOnMoving(HWND parentHWND, LPRECT pRect)
{
	if (!::IsWindow(m_hWnd))
		return;

	if (!::IsWindow(parentHWND))
		return;

	RECT patchrect;
	GetWindowRect(&patchrect);

	RECT parentRect;
	::GetWindowRect(parentHWND, &parentRect);

	int adjust = GetBorderAjustment(parentHWND, parentRect);
	if (patchrect.left == parentRect.right - adjust)
		SetWindowPos(nullptr, patchrect.left - (parentRect.left - pRect->left), patchrect.top - (parentRect.top - pRect->top), 0, 0, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
}

void CPatchViewDlg::ParentOnSizing(HWND parentHWND, LPRECT pRect)
{
	if (!::IsWindow(m_hWnd))
		return;

	if (!::IsWindow(parentHWND))
		return;

	RECT patchrect;
	GetWindowRect(&patchrect);

	RECT parentRect;
	::GetWindowRect(parentHWND, &parentRect);

	int adjust = GetBorderAjustment(parentHWND, parentRect);
	if (patchrect.left != parentRect.right - adjust)
		return;

	if (patchrect.bottom == parentRect.bottom)
		patchrect.bottom -= (parentRect.bottom - pRect->bottom);
	if (patchrect.top == parentRect.top)
		patchrect.top -= parentRect.top - pRect->top;

	SetWindowPos(nullptr, patchrect.left - (parentRect.right - pRect->right), patchrect.top - (parentRect.top - pRect->top), patchrect.right - patchrect.left, patchrect.bottom - patchrect.top, SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
}

void CPatchViewDlg::ShowAndAlignToParent()
{
	CRect rect;
	m_ParentDlg->GetPatchViewParentWnd()->GetWindowRect(&rect);
	int adjust = GetBorderAjustment(m_ParentDlg->GetPatchViewParentWnd()->GetSafeHwnd(), rect);
	rect.left = rect.right - adjust;
	rect.right = rect.left;
	int xPos = CDPIAware::Instance().ScaleX(static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\PatchDlgWidth")));
	if (xPos)
		rect.right += xPos;
	else
		rect.right += rect.Width();

	WINDOWPLACEMENT wp;
	m_ParentDlg->GetPatchViewParentWnd()->GetWindowPlacement(&wp);
	if (wp.showCmd != SW_MAXIMIZE)
		SetWindowPos(nullptr, rect.left, rect.top, rect.Width(), rect.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	else if (auto monitor = MonitorFromRect(rect, MONITOR_DEFAULTTONULL); !monitor)
	{
		CRect pos;
		GetWindowRect(&pos);
		SetWindowPos(nullptr, 0, 0, xPos ? xPos : pos.Width(), pos.Height(), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);
	}
	else
	{
		MONITORINFO monitorinfo;
		monitorinfo.cbSize = sizeof(MONITORINFO);
		GetMonitorInfo(monitor, &monitorinfo);
		SetWindowPos(nullptr, monitorinfo.rcWork.left, monitorinfo.rcWork.top, min(rect.Width(), static_cast<int>(monitorinfo.rcWork.right - monitorinfo.rcWork.left)), min(rect.Height(), static_cast<int>(monitorinfo.rcWork.bottom - monitorinfo.rcWork.top)), SWP_SHOWWINDOW | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);
	}
}

void CPatchViewDlg::OnClose()
{
	CStandAloneDialog::OnClose();
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
	RECT rect;
	GetClientRect(&rect);
	::SetWindowPos(m_ctrlPatchView.GetSafeHwnd(), HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top - CDPIAware::Instance().ScaleX(SEARCHBARHEIGHT), SWP_SHOWWINDOW);
	::SetWindowPos(m_FindBar, HWND_TOP, rect.left, rect.bottom - CDPIAware::Instance().ScaleX(SEARCHBARHEIGHT + 2), rect.right - rect.left, CDPIAware::Instance().ScaleX(SEARCHBARHEIGHT), SWP_SHOWWINDOW);
	if (auto selstart = m_ctrlPatchView.Call(SCI_GETSELECTIONSTART), selend = m_ctrlPatchView.Call(SCI_GETSELECTIONEND); selstart != selend)
	{
		auto linebuf = std::make_unique<char[]>(selend - selstart + 1);
		Sci_TextRange range = { static_cast<Sci_PositionCR>(selstart), static_cast<Sci_PositionCR>(selend), linebuf.get() };
		if (m_ctrlPatchView.Call(SCI_GETTEXTRANGE, 0, reinterpret_cast<LPARAM>(&range)) > 0)
			m_FindBar.SetFindText(m_ctrlPatchView.StringFromControl(linebuf.get()));
	}
	m_FindBar.SetFocusTextBox();
}

void CPatchViewDlg::OnFindNext()
{
	if (m_FindBar.GetFindText().IsEmpty())
	{
		OnShowFindBar();
		return;
	}
	DoSearch(false);
}

void CPatchViewDlg::DoSearch(bool reverse)
{
	Sci_Position lastcursor = m_ctrlPatchView.Call(SCI_GETSELECTIONEND);
	if (!reverse)
		m_ctrlPatchView.Call(SCI_SETTARGETRANGE, lastcursor, m_ctrlPatchView.Call(SCI_GETLENGTH));
	else
	{
		lastcursor = m_ctrlPatchView.Call(SCI_GETSELECTIONSTART);
		m_ctrlPatchView.Call(SCI_SETTARGETRANGE, lastcursor, 0);
	}

	auto searchText = CUnicodeUtils::GetUTF8(m_FindBar.GetFindText());
	m_ctrlPatchView.Call(SCI_SETSEARCHFLAGS, m_FindBar.IsMatchCase() ? SCFIND_MATCHCASE : SCFIND_NONE);
	if (auto pos = m_ctrlPatchView.Call(SCI_SEARCHINTARGET, searchText.GetLength(), reinterpret_cast<LPARAM>(static_cast<LPCSTR>(searchText))); pos >= 0)
	{
		m_ctrlPatchView.Call(SCI_SETSELECTION, pos, pos + searchText.GetLength());
		m_ctrlPatchView.Call(SCI_SCROLLCARET);
	}
	else
	{
		m_ctrlPatchView.Call(SCI_SETSELECTION, lastcursor, lastcursor);
		FlashWindowEx(FLASHW_ALL, 3, 100);
	}
}

void CPatchViewDlg::OnFindPrev()
{
	if (m_FindBar.GetFindText().IsEmpty())
	{
		OnShowFindBar();
		return;
	}
	DoSearch(true);
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

void CPatchViewDlg::OnStageHunks()
{
	StageOrUnstageSelectedLinesOrHunks(StagingType::StageHunks);
}

void CPatchViewDlg::OnStageLines()
{
	StageOrUnstageSelectedLinesOrHunks(StagingType::StageLines);
}

void CPatchViewDlg::OnUnstageHunks()
{
	StageOrUnstageSelectedLinesOrHunks(StagingType::UnstageHunks);
}

void CPatchViewDlg::OnUnstageLines()
{
	StageOrUnstageSelectedLinesOrHunks(StagingType::UnstageLines);
}

int CPatchViewDlg::GetFirstLineNumberSelected()
{
	auto selstart = m_ctrlPatchView.Call(SCI_GETSELECTIONSTART);
	return static_cast<int>(m_ctrlPatchView.Call(SCI_LINEFROMPOSITION, selstart));
}

int CPatchViewDlg::GetLastLineNumberSelected()
{
	auto selend = m_ctrlPatchView.Call(SCI_GETSELECTIONEND);
	return static_cast<int>(m_ctrlPatchView.Call(SCI_LINEFROMPOSITION, selend));
}

void CPatchViewDlg::StageOrUnstageSelectedLinesOrHunks(StagingType stagingType)
{
	auto documentLength = static_cast<Sci_Position>(m_ctrlPatchView.Call(SCI_GETLENGTH));
	auto wholePatchBuf = std::make_unique<char[]>(documentLength + 1);
	m_ctrlPatchView.Call(SCI_GETTEXT, documentLength + 1, reinterpret_cast<LPARAM>(wholePatchBuf.get()));

	int lineCount = static_cast<int>(m_ctrlPatchView.Call(SCI_GETLINECOUNT));

	CDiffLinesForStaging lines(wholePatchBuf.get(), lineCount, GetFirstLineNumberSelected(), GetLastLineNumberSelected());
	auto op = StagingOperations(&lines);
	std::string strPatch;
	if (stagingType == StagingType::StageLines || stagingType == StagingType::UnstageLines)
		strPatch = op.CreatePatchBufferToStageOrUnstageSelectedLines(stagingType);
	else if (stagingType == StagingType::StageHunks || stagingType == StagingType::UnstageHunks)
		strPatch = op.CreatePatchBufferToStageOrUnstageSelectedHunks();
	else
		return; // this should never happen
	if (strPatch.empty())
	{
		CMessageBox::Show(GetSafeHwnd(), IDS_ERROR_PARTIALSTAGING, IDS_APPNAME, MB_OK | MB_ICONERROR);
		return;
	}

	CTGitPath::StagingStatus newStatus; // this will be sent to the commit dialog so that it can update the file checkbox/status
	if (strcmp(wholePatchBuf.get(), strPatch.c_str()) == 0)
	{
		if (stagingType == StagingType::StageLines || stagingType == StagingType::StageHunks)
			newStatus = CTGitPath::StagingStatus::TotallyStaged;
		else // stagingType == StagingType::UnstageLines || stagingType == StagingType::UnstageHunks
			newStatus = CTGitPath::StagingStatus::TotallyUnstaged;
	}
	else // if the patch to be applied is different than the whole diff, the file is still partially staged or became partially staged
		newStatus = CTGitPath::StagingStatus::PartiallyStaged;

	CString tempPatch = StagingOperations::WritePatchBufferToTemporaryFile(strPatch);
	if (tempPatch.IsEmpty())
		return;

	CString out;
	int ret;
	if (stagingType == StagingType::StageHunks || stagingType == StagingType::StageLines)
		ret = g_Git.ApplyPatchToIndex(tempPatch, &out);
	else //if (stagingType == StagingType::UnstageHunks || stagingType == StagingType::UnstageLines)
		ret = g_Git.ApplyPatchToIndexReverse(tempPatch, &out);
	if (ret != 0)
	{
		MessageBox(out, L"TortoiseGit", MB_OK | MB_ICONERROR);
		return;
	}

	// Tell the commit window we partially staged a file and ask it to update ourselves with the updated diff
	m_ParentDlg->GetPatchViewParentWnd()->SendMessage(WM_PARTIALSTAGINGREFRESHPATCHVIEW, static_cast<WPARAM>(newStatus));
}

void CPatchViewDlg::OnDestroy()
{
	__super::OnDestroy();
	CRect rect;
	GetWindowRect(&rect);
	CRegStdDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\PatchDlgWidth") = CDPIAware::Instance().UnscaleX(rect.Width());
	m_ctrlPatchView.ClearContextMenuHandlers();
}

// CSciEditContextMenuInterface
void CPatchViewDlg::InsertMenuItems(CMenu& mPopup, int& nCmd)
{
	CString sMenuItemText;
	sMenuItemText.LoadString(IDS_REPOBROWSE_SAVEAS);
	m_nPopupSave = nCmd++;
	mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nPopupSave, sMenuItemText);
	// We need to reset those to 0:
	m_nStageHunks = 0;
	m_nStageLines = 0;
	m_nUnstageHunks = 0;
	m_nUnstageLines = 0;
	if (m_nEnableStagingType == EnableStagingTypes::Staging || m_nEnableStagingType == EnableStagingTypes::Unstaging)
		mPopup.AppendMenu(MF_SEPARATOR);
	if (m_nEnableStagingType == EnableStagingTypes::Staging)
	{
		sMenuItemText.LoadString(IDS_PROC_STAGE_SELECTED_HUNKS);
		m_nStageHunks = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nStageHunks, sMenuItemText);

		sMenuItemText.LoadString(IDS_PROC_STAGE_SELECTED_LINES);
		m_nStageLines = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nStageLines, sMenuItemText);
	}
	if (m_nEnableStagingType == EnableStagingTypes::Unstaging)
	{
		sMenuItemText.LoadString(IDS_PROC_UNSTAGE_SELECTED_HUNKS);
		m_nUnstageHunks = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nUnstageHunks, sMenuItemText);

		sMenuItemText.LoadString(IDS_PROC_UNSTAGE_SELECTED_LINES);
		m_nUnstageLines = nCmd++;
		mPopup.AppendMenu(MF_STRING | MF_ENABLED, m_nUnstageLines, sMenuItemText);
	}
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
	else if (cmd == m_nStageHunks)
	{
		OnStageHunks();
		return true;
	}
	else if (cmd == m_nStageLines)
	{
		OnStageLines();
		return true;
	}
	else if (cmd == m_nUnstageHunks)
	{
		OnUnstageHunks();
		return true;
	}
	else if (cmd == m_nUnstageLines)
	{
		OnUnstageLines();
		return true;
	}
	return false;
}
