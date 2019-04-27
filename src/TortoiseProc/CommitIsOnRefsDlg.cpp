// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2019 - TortoiseGit

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
#include "TortoiseProc.h"
#include "Git.h"
#include "CommitIsOnRefsDlg.h"
#include "StringUtils.h"
#include "BrowseRefsDlg.h"
#include "RefLogDlg.h"
#include "LogDlg.h"
#include "LoglistUtils.h"
#include "AppUtils.h"
#include "FileDiffDlg.h"
#include "MessageBox.h"

// CCommitIsOnRefsDlg dialog

UINT CCommitIsOnRefsDlg::WM_GETTINGREFSFINISHED = RegisterWindowMessage(L"TORTOISEGIT_CommitIsOnRefs_GETTINGREFSFINISHED");

IMPLEMENT_DYNAMIC(CCommitIsOnRefsDlg, CResizableStandAloneDialog)

CCommitIsOnRefsDlg::CCommitIsOnRefsDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CCommitIsOnRefsDlg::IDD, pParent)
	, m_bThreadRunning(FALSE)
	, m_bRefsLoaded(false)
	, m_bHasWC(true)
	, m_bNonModalParentHWND(nullptr)
{
}

CCommitIsOnRefsDlg::~CCommitIsOnRefsDlg()
{
}

void CCommitIsOnRefsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_REF_LEAFS, m_cRefList);
	DDX_Control(pDX, IDC_FILTER, m_cFilter);
	DDX_Control(pDX, IDC_COMMIT, m_cRevEdit);
	DDX_Control(pDX, IDC_SELREF, m_cSelRevBtn);
}

BEGIN_MESSAGE_MAP(CCommitIsOnRefsDlg, CResizableStandAloneDialog)
	ON_BN_CLICKED(IDC_SELREF, OnBnClickedSelRevBtn)
	ON_BN_CLICKED(IDC_LOG, OnBnClickedShowLog)
	ON_EN_CHANGE(IDC_FILTER, OnEnChangeEditFilter)
	ON_REGISTERED_MESSAGE(CFilterEdit::WM_FILTEREDIT_CANCELCLICKED, OnClickedCancelFilter)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_REF_LEAFS, OnItemChangedListRefs)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_REF_LEAFS, OnNMDblClickListRefs)
	ON_MESSAGE(ENAC_UPDATE, OnEnChangeCommit)
	ON_WM_TIMER()
	ON_WM_CONTEXTMENU()
	ON_WM_SETCURSOR()
	ON_REGISTERED_MESSAGE(WM_GETTINGREFSFINISHED, OnGettingRefsFinished)
END_MESSAGE_MAP()

// CCommitIsOnRefsDlg message handlers

void CCommitIsOnRefsDlg::OnCancel()
{
	if (m_bThreadRunning)
		return;

	if (m_bNonModalParentHWND)
	{
		DestroyWindow();
		return;
	}

	__super::OnCancel();
}

void CCommitIsOnRefsDlg::PostNcDestroy()
{
	if (m_bNonModalParentHWND)
		delete this;
}

BOOL CCommitIsOnRefsDlg::OnInitDialog()
{
	__super::OnInitDialog();

	AddAnchor(IDC_FILTER, BOTTOM_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LABEL_FILTER, BOTTOM_LEFT);
	AddAnchor(IDC_SELREF, TOP_RIGHT);
	AddAnchor(IDC_COMMIT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_STATIC_SUBJECT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_LIST_REF_LEAFS, TOP_LEFT, BOTTOM_RIGHT);
	AddAnchor(IDC_LOG, TOP_RIGHT);
	AddOthersToAnchor();

	m_bHasWC = !GitAdminDir::IsBareRepo(g_Git.m_CurrentDir);

	m_cSelRevBtn.m_bRightArrow = TRUE;
	m_cSelRevBtn.m_bDefaultClick = FALSE;
	m_cSelRevBtn.m_bMarkDefault = FALSE;
	m_cSelRevBtn.m_bShowCurrentItem = FALSE;
	m_cSelRevBtn.AddEntry(CString(MAKEINTRESOURCE(IDS_REFBROWSE)));
	m_cSelRevBtn.AddEntry(CString(MAKEINTRESOURCE(IDS_LOG)));
	m_cSelRevBtn.AddEntry(CString(MAKEINTRESOURCE(IDS_REFLOG)));

	EnableSaveRestore(L"CommitIsOnRefsDlg");

	CImageList* imagelist = new CImageList();
	imagelist->Create(IDB_BITMAP_REFTYPE, 16, 3, RGB(255, 255, 255));
	m_cRefList.SetImageList(imagelist, LVSIL_SMALL);

	CRect rect;
	m_cRefList.GetClientRect(&rect);
	m_cRefList.InsertColumn(0, L"Ref", 0, rect.Width() - 50);
	if (CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE))
		m_cRefList.SetExtendedStyle(m_cRefList.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

	m_cFilter.SetCancelBitmaps(IDI_CANCELNORMAL, IDI_CANCELPRESSED, 14, 14);
	m_cFilter.SetInfoIcon(IDI_FILTEREDIT, 19, 19);

	m_cRevEdit.Init();
	m_cRevEdit.SetWindowText(m_Rev);

	StartGetRefsThread();

	m_cRevEdit.SetFocus();

	return FALSE;
}

void CCommitIsOnRefsDlg::AddToList()
{
	m_cRefList.DeleteAllItems();

	CString filter;
	m_cFilter.GetWindowText(filter);

	int item = 0;
	for (size_t i = 0; i < m_RefList.size(); ++i)
	{
		int nImage = -1;
		CString ref = m_RefList[i];
		if (CStringUtils::StartsWith(ref, L"refs/tags/"))
			nImage = 0;
		else if (CStringUtils::StartsWith(ref, L"refs/remotes/"))
			nImage = 2;
		else if (CStringUtils::StartsWith(ref, L"refs/heads/"))
			nImage = 1;

		if (ref.Find(filter) >= 0)
			m_cRefList.InsertItem(item++, ref, nImage);
	}

	if (item)
		m_cRefList.ShowText(L"");
	else
		m_cRefList.ShowText(CString(MAKEINTRESOURCE(IDS_ERROR_NOREF)));
}

void CCommitIsOnRefsDlg::OnEnChangeEditFilter()
{
	SetTimer(IDT_FILTER, 1000, nullptr);
}

void CCommitIsOnRefsDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == IDT_FILTER)
	{
		KillTimer(IDT_FILTER);
		AddToList();
	}
	else if (nIDEvent == IDT_INPUT)
	{
		KillTimer(IDT_INPUT);
		StartGetRefsThread();
	}

	__super::OnTimer(nIDEvent);
}

LRESULT CCommitIsOnRefsDlg::OnClickedCancelFilter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	OnTimer(IDT_FILTER);
	return TRUE;
}

void CCommitIsOnRefsDlg::OnBnClickedShowLog()
{
	CString cmd;
	cmd.Format(L"/command:log /rev:%s", static_cast<LPCTSTR>(m_gitrev.m_CommitHash.ToString()));
	CAppUtils::RunTortoiseGitProc(cmd);
}

void CCommitIsOnRefsDlg::OnBnClickedSelRevBtn()
{
	INT_PTR entry = m_cSelRevBtn.GetCurrentEntry();
	if (entry == 0) /* Browse Refence*/
	{
		{
			CString str = CBrowseRefsDlg::PickRef();
			if (str.IsEmpty())
				return;

			if (FillRevFromString(str))
				return;

			m_cRevEdit.SetWindowText(str);
		}
	}

	if (entry == 1) /*Log*/
	{
		CLogDlg dlg;
		CString revision;
		m_cRevEdit.GetWindowText(revision);
		dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
		dlg.SetSelect(true);
		if (dlg.DoModal() == IDOK)
		{
			if (dlg.GetSelectedHash().empty())
				return;

			if (FillRevFromString(dlg.GetSelectedHash().at(0).ToString()))
				return;

			m_cRevEdit.SetWindowText(dlg.GetSelectedHash().at(0).ToString());
		}
		else
			return;
	}

	if (entry == 2) /*RefLog*/
	{
		CRefLogDlg dlg;
		if (dlg.DoModal() == IDOK)
		{
			if (FillRevFromString(dlg.m_SelectedHash.ToString()))
				return;

			m_cRevEdit.SetWindowText(dlg.m_SelectedHash.ToString());
		}
		else
			return;
	}

	StartGetRefsThread();
	KillTimer(IDT_INPUT);
}

LRESULT CCommitIsOnRefsDlg::OnEnChangeCommit(WPARAM, LPARAM)
{
	SetTimer(IDT_INPUT, 1000, nullptr);
	return 0;
}

void CCommitIsOnRefsDlg::OnContextMenu(CWnd* pWnd, CPoint point)
{
	if (!pWnd || pWnd != &m_cRefList)
		return;
	if (m_cRefList.GetSelectedCount() == 0)
		return;
	// if the context menu is invoked through the keyboard, we have to use
	// a calculated position on where to anchor the menu on
	if ((point.x == -1) && (point.y == -1))
	{
		CRect rect;
		m_cRefList.GetItemRect(m_cRefList.GetSelectionMark(), &rect, LVIR_LABEL);
		m_cRefList.ClientToScreen(&rect);
		point = rect.CenterPoint();
	}
	CIconMenu popup;
	if (popup.CreatePopupMenu())
	{
		STRING_VECTOR selectedRefs;
		POSITION pos = m_cRefList.GetFirstSelectedItemPosition();
		while (pos)
			selectedRefs.push_back(m_cRefList.GetItemText(m_cRefList.GetNextSelectedItem(pos), 0));
		bool needSep = false;
		if (selectedRefs.size() == 2)
		{
			popup.AppendMenuIcon(eCmd_Diff, IDS_LOG_POPUP_COMPARETWO, IDI_DIFF);
			popup.AppendMenuIcon(eCmd_UnifiedDiff, IDS_LOG_POPUP_GNUDIFF, IDI_DIFF);
			popup.AppendMenu(MF_SEPARATOR, NULL);
			CString menu;
			menu.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedRefs, m_sLastSelected, L"..")));
			popup.AppendMenuIcon(eCmd_ViewLogRange, menu, IDI_LOG);
			menu.Format(IDS_SHOWLOG_OF, static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedRefs, m_sLastSelected, L"...")));
			popup.AppendMenuIcon(eCmd_ViewLogRangeReachableFromOnlyOne, menu, IDI_LOG);
			needSep = true;
		}
		else if (selectedRefs.size() == 1)
		{
			popup.AppendMenuIcon(eCmd_ViewLog, IDS_FILEDIFF_LOG, IDI_LOG);
			popup.AppendMenu(MF_SEPARATOR, NULL);
			popup.AppendMenuIcon(eCmd_RepoBrowser, IDS_LOG_BROWSEREPO, IDI_REPOBROWSE);
			if (m_bHasWC)
			{
				popup.AppendMenu(MF_SEPARATOR);
				popup.AppendMenuIcon(eCmd_DiffWC, IDS_LOG_POPUP_COMPARE, IDI_DIFF);
			}
			needSep = true;
		}

		if (!selectedRefs.empty())
		{
			if (needSep)
				popup.AppendMenu(MF_SEPARATOR, NULL);

			popup.AppendMenuIcon(eCmd_Copy, IDS_SCIEDIT_COPY, IDI_COPYCLIP);
		}

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, this);
		switch (cmd)
		{
		case eCmd_Diff:
		{
			CFileDiffDlg dlg;
			dlg.SetDiff(
				nullptr,
				selectedRefs[0],
				selectedRefs[1]);
			dlg.DoModal();
		}
		break;
		case eCmd_UnifiedDiff:
			CAppUtils::StartShowUnifiedDiff(GetSafeHwnd(), CTGitPath(), selectedRefs.at(0), CTGitPath(), selectedRefs.at(1), !!(GetAsyncKeyState(VK_SHIFT) & 0x8000));
			break;
		case eCmd_ViewLog:
		{
			CString sCmd = L"/command:log";
			sCmd += L" /path:\"" + g_Git.m_CurrentDir + L"\" ";
			sCmd += L" /endrev:" + selectedRefs.at(0);
			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
		case eCmd_ViewLogRange:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\" /range:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedRefs, m_sLastSelected, L"..")));
			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
		case eCmd_ViewLogRangeReachableFromOnlyOne:
		{
			CString sCmd;
			sCmd.Format(L"/command:log /path:\"%s\" /range:\"%s\"", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(GetTwoSelectedRefs(selectedRefs, m_sLastSelected, L"...")));
			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
		case eCmd_RepoBrowser:
			CAppUtils::RunTortoiseGitProc(L"/command:repobrowser /path:\"" + g_Git.m_CurrentDir + L"\" /rev:" + selectedRefs[0]);
			break;
		case eCmd_Copy:
			CopySelectionToClipboard();
			break;
		case eCmd_DiffWC:
		{
			CString sCmd;
			sCmd.Format(L"/command:showcompare /path:\"%s\" /revision1:%s /revision2:%s", static_cast<LPCTSTR>(g_Git.m_CurrentDir), static_cast<LPCTSTR>(selectedRefs[0]), static_cast<LPCTSTR>(GitRev::GetWorkingCopy()));
			if (!!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
				sCmd += L" /alternative";

			CAppUtils::RunTortoiseGitProc(sCmd);
		}
		break;
		}
	}
}

void CCommitIsOnRefsDlg::OnItemChangedListRefs(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	*pResult = 0;

	if (pNMListView->iItem >= 0 && m_RefList.size() > static_cast<size_t>(pNMListView->iItem) && (pNMListView->uNewState & LVIS_SELECTED))
		m_sLastSelected = m_RefList[pNMListView->iItem];
}

void CCommitIsOnRefsDlg::OnNMDblClickListRefs(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;

	LPNMLISTVIEW pNMListView = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if (m_bNonModalParentHWND && pNMListView->iItem >= 0 && m_RefList.size() > static_cast<size_t>(pNMListView->iItem))
	{
		if (::SendMessage(m_bNonModalParentHWND, CGitLogListBase::m_ScrollToRef, reinterpret_cast<WPARAM>(&m_RefList[pNMListView->iItem]), 0) != 0)
			FlashWindowEx(FLASHW_ALL, 2, 100);
	}
}

CString CCommitIsOnRefsDlg::GetTwoSelectedRefs(const STRING_VECTOR& selectedRefs, const CString& lastSelected, const CString& separator)
{
	ASSERT(selectedRefs.size() == 2);

	if (selectedRefs.at(0) == lastSelected)
		return g_Git.StripRefName(selectedRefs.at(1)) + separator + g_Git.StripRefName(lastSelected);
	else
		return g_Git.StripRefName(selectedRefs.at(0)) + separator + g_Git.StripRefName(lastSelected);
}

void CCommitIsOnRefsDlg::StartGetRefsThread()
{
	if (InterlockedExchange(&m_bThreadRunning, TRUE))
		return;

	KillTimer(IDT_INPUT);

	SetDlgItemText(IDC_STATIC_SUBJECT, L"");
	m_tooltips.DelTool(IDC_STATIC_SUBJECT);

	DialogEnableWindow(IDC_LOG, FALSE);
	DialogEnableWindow(IDC_SELREF, FALSE);
	DialogEnableWindow(IDC_FILTER, FALSE);

	m_RefList.clear();
	m_cRefList.ShowText(CString(MAKEINTRESOURCE(IDS_STATUSLIST_BUSYMSG)));
	m_cRefList.DeleteAllItems();
	RefreshCursor();

	m_cRevEdit.GetWindowText(m_Rev);

	if (!AfxBeginThread(GetRefsThreadEntry, this))
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

UINT CCommitIsOnRefsDlg::GetRefsThreadEntry(LPVOID pVoid)
{
	return static_cast<CCommitIsOnRefsDlg*>(pVoid)->GetRefsThread();
}

UINT CCommitIsOnRefsDlg::GetRefsThread()
{
	if (!m_bRefsLoaded)
	{
		m_cRevEdit.RemoveSearchAll();
		STRING_VECTOR refs;
		g_Git.GetRefList(refs);
		for (const auto& ref : refs)
			m_cRevEdit.AddSearchString(ref);
		m_bRefsLoaded = true;
	}

	if (m_Rev.IsEmpty() || m_gitrev.GetCommit(m_Rev))
	{
		SendMessage(WM_GETTINGREFSFINISHED);

		InterlockedExchange(&m_bThreadRunning, FALSE);
		return 0;
	}

	if (g_Git.GetRefsCommitIsOn(m_RefList, m_gitrev.m_CommitHash, true, true, CGit::BRANCH_ALL))
	{
		MessageBox(g_Git.GetGitLastErr(L"Could not get all refs."), L"TortoiseGit", MB_ICONERROR);
		return 0;
	}
	SendMessage(WM_GETTINGREFSFINISHED);

	InterlockedExchange(&m_bThreadRunning, FALSE);
	return 0;
}

BOOL CCommitIsOnRefsDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	if (pWnd != &m_cRefList)
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
	if (!m_bThreadRunning)
	{
		HCURSOR hCur = LoadCursor(nullptr, IDC_ARROW);
		SetCursor(hCur);
		return CResizableStandAloneDialog::OnSetCursor(pWnd, nHitTest, message);
	}
	HCURSOR hCur = LoadCursor(nullptr, IDC_WAIT);
	SetCursor(hCur);
	return TRUE;
}

LRESULT CCommitIsOnRefsDlg::OnGettingRefsFinished(WPARAM, LPARAM)
{
	DialogEnableWindow(IDC_LOG, TRUE);
	DialogEnableWindow(IDC_SELREF, TRUE);
	DialogEnableWindow(IDC_FILTER, TRUE);

	if (m_Rev.IsEmpty())
	{
		m_cRefList.ShowText(L"");
		InvalidateRect(nullptr);
		RefreshCursor();
		return 0;
	}

	if (!m_gitrev.GetLastErr().IsEmpty())
	{
		CString msg;
		msg.Format(IDS_PROC_REFINVALID, static_cast<LPCTSTR>(m_Rev));
		m_cRefList.ShowText(msg + L'\n' + m_gitrev.GetLastErr());

		InvalidateRect(nullptr);
		RefreshCursor();
		return 0;
	}

	SetDlgItemText(IDC_STATIC_SUBJECT, m_gitrev.m_CommitHash.ToString(g_Git.GetShortHASHLength()) + L": " + m_gitrev.GetSubject());
	if (!m_gitrev.m_CommitHash.IsEmpty())
		m_tooltips.AddTool(IDC_STATIC_SUBJECT, CLoglistUtils::FormatDateAndTime(m_gitrev.GetAuthorDate(), DATE_SHORTDATE) + L"  " + m_gitrev.GetAuthorName());

	AddToList();

	InvalidateRect(nullptr);
	RefreshCursor();
	return 0;
}

BOOL CCommitIsOnRefsDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		case 'A':
		{
			if (GetFocus() != GetDlgItem(IDC_LIST_REF_LEAFS))
				break;
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				// select all entries
				for (int i = 0; i < m_cRefList.GetItemCount(); ++i)
					m_cRefList.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
				return TRUE;
			}
		}
		break;
		case 'C':
		case VK_INSERT:
		{
			if (GetFocus() != GetDlgItem(IDC_LIST_REF_LEAFS))
				break;
			if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
			{
				CopySelectionToClipboard();
				return TRUE;
			}
		}
		break;
		case VK_F5:
		{
			m_bRefsLoaded = false;
			OnTimer(IDT_INPUT);
		}
		break;
		case VK_ESCAPE:
			if (GetFocus() == GetDlgItem(IDC_FILTER) && m_cFilter.GetWindowTextLength())
			{
				m_cFilter.SetWindowText(L"");
				OnTimer(IDT_FILTER);
				return TRUE;
			}
			break;
		}
	}
	return __super::PreTranslateMessage(pMsg);
}

void CCommitIsOnRefsDlg::CopySelectionToClipboard()
{
	// copy all selected paths to the clipboard
	POSITION pos = m_cRefList.GetFirstSelectedItemPosition();
	int index;
	CString sTextForClipboard;
	while ((index = m_cRefList.GetNextSelectedItem(pos)) >= 0)
	{
		sTextForClipboard += m_cRefList.GetItemText(index, 0);
		sTextForClipboard += L"\r\n";
	}
	CStringUtils::WriteAsciiStringToClipboard(sTextForClipboard);
}
