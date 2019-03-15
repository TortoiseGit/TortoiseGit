// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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
// ImportPatchDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TortoiseProc.h"
#include "ImportPatchDlg.h"
#include "Git.h"
#include "MessageBox.h"
#include "AppUtils.h"
#include "SmartHandle.h"
#include "LoglistCommonResource.h"
#include "DPIAware.h"

// CImportPatchDlg dialog

IMPLEMENT_DYNAMIC(CImportPatchDlg, CResizableStandAloneDialog)

CImportPatchDlg::CImportPatchDlg(CWnd* pParent /*=nullptr*/)
	: CResizableStandAloneDialog(CImportPatchDlg::IDD, pParent)
	, m_LoadingThread(FALSE)
	, m_CurrentItem(0)
	, m_bExitThread(FALSE)
	, m_bThreadRunning(FALSE)
	, m_b3Way(BST_CHECKED)
	, m_bIgnoreSpace(BST_CHECKED)
	, m_bAddSignedOffBy(BST_UNCHECKED)
	, m_bKeepCR(BST_CHECKED)
{
	m_cList.m_ContextMenuMask &=~ m_cList.GetMenuMask(CPatchListCtrl::MENU_APPLY);
}

CImportPatchDlg::~CImportPatchDlg()
{
}

void CImportPatchDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PATCH,m_cList);
	DDX_Control(pDX, IDC_AM_SPLIT, m_wndSplitter);
	DDX_Check(pDX, IDC_CHECK_3WAY, m_b3Way);
	DDX_Check(pDX, IDC_CHECK_IGNORE_SPACE, m_bIgnoreSpace);
	DDX_Check(pDX, IDC_SIGN_OFF, m_bAddSignedOffBy);
	DDX_Check(pDX, IDC_KEEP_CR, m_bKeepCR);
}

void CImportPatchDlg::AddAmAnchor()
{
	AddAnchor(IDC_AM_TAB,TOP_LEFT,BOTTOM_RIGHT);
	AddAnchor(IDC_LIST_PATCH, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_AM_SPLIT, TOP_LEFT, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_ADD, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_UP, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_DOWN, TOP_RIGHT);
	AddAnchor(IDC_BUTTON_REMOVE, TOP_RIGHT);

	AddAnchor(IDOK,BOTTOM_RIGHT);
	AddAnchor(IDCANCEL,BOTTOM_RIGHT);
	AddAnchor(IDHELP, BOTTOM_RIGHT);

	AddAnchor(IDC_AM_DUMY_TAB, TOP_LEFT, BOTTOM_RIGHT);

	this->AddOthersToAnchor();
}

void CImportPatchDlg::SetSplitterRange()
{
	if ((m_cList)&&(m_ctrlTabCtrl))
	{
		CRect rcTop;
		m_cList.GetWindowRect(rcTop);
		ScreenToClient(rcTop);
		CRect rcMiddle;
		m_ctrlTabCtrl.GetWindowRect(rcMiddle);
		ScreenToClient(rcMiddle);
		if (rcMiddle.Height() && rcMiddle.Width())
			m_wndSplitter.SetRange(rcTop.top + CDPIAware::Instance().ScaleY(160), rcMiddle.bottom - CDPIAware::Instance().ScaleY(160));
	}
}

BOOL CImportPatchDlg::OnInitDialog()
{
	CResizableStandAloneDialog::OnInitDialog();
	CAppUtils::MarkWindowAsUnpinnable(m_hWnd);

	// Let the TaskbarButtonCreated message through the UIPI filter. If we don't
	// do this, Explorer would be unable to send that message to our window if we
	// were running elevated. It's OK to make the call all the time, since if we're
	// not elevated, this is a no-op.
	CHANGEFILTERSTRUCT cfs = { sizeof(CHANGEFILTERSTRUCT) };
	typedef BOOL STDAPICALLTYPE ChangeWindowMessageFilterExDFN(HWND hWnd, UINT message, DWORD action, PCHANGEFILTERSTRUCT pChangeFilterStruct);
	CAutoLibrary hUser = AtlLoadSystemLibraryUsingFullPath(L"user32.dll");
	if (hUser)
	{
		auto pfnChangeWindowMessageFilterEx = reinterpret_cast<ChangeWindowMessageFilterExDFN*>(GetProcAddress(hUser, "ChangeWindowMessageFilterEx"));
		if (pfnChangeWindowMessageFilterEx)
			pfnChangeWindowMessageFilterEx(m_hWnd, TaskBarButtonCreated, MSGFLT_ALLOW, &cfs);
	}
	m_pTaskbarList.Release();
	if (FAILED(m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList)))
		m_pTaskbarList = nullptr;

	CRect rectDummy;

	GetClientRect(m_DlgOrigRect);
	m_cList.GetClientRect(m_PatchListOrigRect);

	CWnd *pwnd=this->GetDlgItem(IDC_AM_DUMY_TAB);
	pwnd->GetWindowRect(&rectDummy);
	this->ScreenToClient(rectDummy);

	if (!m_ctrlTabCtrl.Create(CMFCTabCtrl::STYLE_FLAT, rectDummy, this, IDC_AM_TAB))
	{
		TRACE0("Failed to create output tab window\n");
		return FALSE;      // fail to create
	}
	m_ctrlTabCtrl.SetResizeMode(CMFCTabCtrl::RESIZE_NO);
	// Create output panes:

	if (!m_PatchCtrl.Create(L"Scintilla", L"source", 0, rectDummy, &m_ctrlTabCtrl, 0, 0))
	{
		TRACE0("Failed to create log message control");
		return FALSE;
	}
	m_PatchCtrl.Init(-1);
	m_PatchCtrl.Call(SCI_SETREADONLY, TRUE);
	m_PatchCtrl.SetUDiffStyle();

	if (!m_wndOutput.Create(L"Scintilla", L"source", 0, rectDummy, &m_ctrlTabCtrl, 0, 0))
	{
		TRACE0("Failed to create output windows\n");
		return -1;      // fail to create
	}
	m_wndOutput.Init(-1);
	m_wndOutput.Call(SCI_SETREADONLY, TRUE);

	m_tooltips.AddTool(IDC_CHECK_3WAY,IDS_AM_3WAY_TT);
	m_tooltips.AddTool(IDC_CHECK_IGNORE_SPACE,IDS_AM_IGNORE_SPACE_TT);

	m_ctrlTabCtrl.AddTab(&m_PatchCtrl, CString(MAKEINTRESOURCE(IDS_PATCH)), 0);
	m_ctrlTabCtrl.AddTab(&m_wndOutput, CString(MAKEINTRESOURCE(IDS_LOG)), 1);

	AdjustControlSize(IDC_CHECK_3WAY);
	AdjustControlSize(IDC_CHECK_IGNORE_SPACE);
	AdjustControlSize(IDC_SIGN_OFF);
	AdjustControlSize(IDC_KEEP_CR);

	AddAmAnchor();

	m_PathList.SortByPathname(true);
	m_cList.SetExtendedStyle(m_cList.GetExtendedStyle() | LVS_EX_CHECKBOXES | LVS_EX_DOUBLEBUFFER);
	m_cList.InsertColumn(0, L"");
	m_cList.SetExtendedStyle((CRegDWORD(L"Software\\TortoiseGit\\FullRowSelect", TRUE) ? LVS_EX_FULLROWSELECT : 0) | m_cList.GetExtendedStyle());

	for (int i = 0; i < m_PathList.GetCount(); ++i)
	{
		m_cList.InsertItem(0,m_PathList[i].GetWinPath());
		m_cList.SetCheck(0,true);
	}
	m_cList.SetColumnWidth(0, LVSCW_AUTOSIZE);

	DWORD yPos = CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\AMDlgSizer");
	RECT rcDlg, rcLogMsg, rcFileList;
	GetClientRect(&rcDlg);
	m_cList.GetWindowRect(&rcLogMsg);
	ScreenToClient(&rcLogMsg);
	this->m_ctrlTabCtrl.GetWindowRect(&rcFileList);
	ScreenToClient(&rcFileList);
	if (yPos)
	{
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		int delta = yPos - rectSplitter.top;
		if ((rcLogMsg.bottom + delta > rcLogMsg.top) && (rcLogMsg.bottom + delta < rcFileList.bottom - CDPIAware::Instance().ScaleY(30)))
		{
			m_wndSplitter.SetWindowPos(nullptr, 0, yPos, 0, 0, SWP_NOSIZE);
			DoSize(delta);
		}
	}

	CAppUtils::SetListCtrlBackgroundImage(m_cList.GetSafeHwnd(), IDI_IMPORTPATHCES_BKG);

	CString sWindowTitle;
	GetWindowText(sWindowTitle);
	CAppUtils::SetWindowTitle(m_hWnd, g_Git.m_CurrentDir, sWindowTitle);

	EnableSaveRestore(L"ImportDlg");

	SetSplitterRange();

	return TRUE;
}

BEGIN_MESSAGE_MAP(CImportPatchDlg, CResizableStandAloneDialog)
	ON_LBN_SELCHANGE(IDC_LIST_PATCH, &CImportPatchDlg::OnLbnSelchangeListPatch)
	ON_BN_CLICKED(IDC_BUTTON_ADD, &CImportPatchDlg::OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_UP, &CImportPatchDlg::OnBnClickedButtonUp)
	ON_BN_CLICKED(IDC_BUTTON_DOWN, &CImportPatchDlg::OnBnClickedButtonDown)
	ON_BN_CLICKED(IDC_BUTTON_REMOVE, &CImportPatchDlg::OnBnClickedButtonRemove)
	ON_BN_CLICKED(IDOK, &CImportPatchDlg::OnBnClickedOk)
	ON_WM_SIZE()
	ON_WM_SYSCOLORCHANGE()
	ON_WM_THEMECHANGED()
	ON_BN_CLICKED(IDCANCEL, &CImportPatchDlg::OnBnClickedCancel)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PATCH, &CImportPatchDlg::OnHdnItemchangedListPatch)
	ON_REGISTERED_MESSAGE(TaskBarButtonCreated, OnTaskbarBtnCreated)
END_MESSAGE_MAP()


// CImportPatchDlg message handlers

void CImportPatchDlg::OnLbnSelchangeListPatch()
{
	if(m_cList.GetSelectedCount() == 0)
	{
		this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(FALSE);
		this->GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(FALSE);
	}
	else
	{
		this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(TRUE);
		this->GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(TRUE);
	}
}

void CImportPatchDlg::OnBnClickedButtonAdd()
{
	CFileDialog dlg(TRUE, nullptr, nullptr, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_ALLOWMULTISELECT, CString(MAKEINTRESOURCE(IDS_PATCHFILEFILTER)));
	dlg.m_ofn.nMaxFile = 65536;
	TCHAR path[65536] = { 0 };
	dlg.m_ofn.lpstrFile = path;
	INT_PTR ret = dlg.DoModal();
	SetCurrentDirectory(g_Git.m_CurrentDir);
	if (ret == IDOK)
	{
		POSITION pos;
		pos = dlg.GetStartPosition();
		while (pos)
		{
			CString file = dlg.GetNextPathName(pos);
			file.Trim();
			if(!file.IsEmpty())
			{
				int index = m_cList.InsertItem(m_cList.GetItemCount(), file);
				if (index >= 0)
					m_cList.SetCheck(index, true);
			}
		}
		m_cList.SetColumnWidth(0, LVSCW_AUTOSIZE);
	}
}

void CImportPatchDlg::OnBnClickedButtonUp()
{
	POSITION pos;
	pos = m_cList.GetFirstSelectedItemPosition();

	// do nothing if the first selected item is the first item in the list
	if (m_cList.GetNextSelectedItem(pos) == 0)
		return;

	pos = m_cList.GetFirstSelectedItemPosition();

	while (pos)
	{
		int index = m_cList.GetNextSelectedItem(pos);
		if(index >= 1)
		{
			CString old = m_cList.GetItemText(index, 0);
			BOOL oldState = m_cList.GetCheck(index);
			m_cList.DeleteItem(index);
			m_cList.InsertItem(index - 1, old);
			m_cList.SetCheck(index - 1, oldState);
			m_cList.SetItemState(index - 1, LVIS_SELECTED, LVIS_SELECTED);
			m_cList.SetItemState(index, 0, LVIS_SELECTED);
		}
	}
}

void CImportPatchDlg::OnBnClickedButtonDown()
{
	if (m_cList.GetSelectedCount() == 0)
		return;

	POSITION pos;
	pos = m_cList.GetFirstSelectedItemPosition();
	// use an array to store all selected item indexes; the user won't select too much items
	auto indexes = std::make_unique<int[]>(m_cList.GetSelectedCount());
	int i = 0;
	while(pos)
		indexes[i++] = m_cList.GetNextSelectedItem(pos);

	// don't move any item if the last selected item is the last item in the m_CommitList
	// (that would change the order of the selected items)
	if(indexes[m_cList.GetSelectedCount() - 1] < m_cList.GetItemCount() - 1)
	{
		// iterate over the indexes backwards in order to correctly move multiselected items
		for (i = m_cList.GetSelectedCount() - 1; i >= 0; i--)
		{
			int index = indexes[i];
			CString old = m_cList.GetItemText(index, 0);
			BOOL oldState = m_cList.GetCheck(index);
			m_cList.DeleteItem(index);
			m_cList.InsertItem(index + 1, old);
			m_cList.SetCheck(index + 1, oldState);
			m_cList.SetItemState(index + 1, LVIS_SELECTED, LVIS_SELECTED);
			m_cList.SetItemState(index, 0, LVIS_SELECTED);
		}
	}
}

void CImportPatchDlg::OnBnClickedButtonRemove()
{
	POSITION pos;
	pos = m_cList.GetFirstSelectedItemPosition();
	while (pos)
	{
		int index = m_cList.GetNextSelectedItem(pos);
		m_cList.DeleteItem(index);
		pos = m_cList.GetFirstSelectedItemPosition();
	}
	m_cList.SetColumnWidth(0, LVSCW_AUTOSIZE);
}

UINT CImportPatchDlg::PatchThread()
{
	CTGitPath path;
	path.SetFromWin(g_Git.m_CurrentDir);

	int i=0;
	UpdateOkCancelText();
	for (i = m_CurrentItem; i < m_cList.GetItemCount(); ++i)
	{
		if (m_pTaskbarList)
		{
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
			m_pTaskbarList->SetProgressValue(m_hWnd, i, m_cList.GetItemCount());
		}

		m_cList.SetItemData(i, CPatchListCtrl::STATUS_APPLYING|m_cList.GetItemData(i));

		CRect rect;
		this->m_cList.GetItemRect(i,&rect,LVIR_BOUNDS);
		this->m_cList.InvalidateRect(rect);

		m_cList.SetColumnWidth(0, LVSCW_AUTOSIZE);
		m_cList.EnsureVisible(i, FALSE);

		if(m_bExitThread)
			break;

		if(m_cList.GetCheck(i))
		{
			CString cmd;

			while(path.HasRebaseApply())
			{
				if (m_pTaskbarList)
					m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);

				int ret = CMessageBox::Show(GetSafeHwnd(), IDS_PROC_APPLYPATCH_REBASEDIRFOUND,
												  IDS_APPNAME,
												   1, IDI_ERROR, IDS_ABORTBUTTON, IDS_SKIPBUTTON, IDS_RESOLVEDBUTTON);

				switch(ret)
				{
				case 1:
					cmd = L"git.exe am --abort";
					break;
				case 2:
					cmd = L"git.exe am --skip";
					++i;
					break;
				case 3:
					cmd = L"git.exe am --resolved";
					break;
				default:
					cmd.Empty();
				}
				if(cmd.IsEmpty())
				{
					m_bExitThread = TRUE;
					break;
				}

				this->AddLogString(cmd);
				CString output;
				if (g_Git.Run(cmd, &output, CP_UTF8))
				{
					this->AddLogString(output);
					this->AddLogString(CString(MAKEINTRESOURCE(IDS_FAIL)));
				}
				else
				{
					this->AddLogString(CString(MAKEINTRESOURCE(IDS_DONE)));
				}
			}

			if(m_bExitThread)
				break;

			cmd = L"git.exe am ";

			if(this->m_bAddSignedOffBy)
				cmd += L"--signoff ";

			if(this->m_b3Way)
				cmd += L"--3way ";

			if(this->m_bIgnoreSpace)
				cmd += L"--ignore-space-change ";

			if(this->m_bKeepCR)
				cmd += L"--keep-cr ";

			cmd += L'"';
			cmd += m_cList.GetItemText(i,0);
			cmd += L'"';

			this->AddLogString(cmd);
			CString output;
			if (g_Git.Run(cmd, &output, CP_UTF8))
			{
				//keep STATUS_APPLYING to let user retry failed patch
				m_cList.SetItemData(i, CPatchListCtrl::STATUS_APPLY_FAIL|CPatchListCtrl::STATUS_APPLYING);
				this->AddLogString(output);
				this->AddLogString(CString(MAKEINTRESOURCE(IDS_FAIL)));
				if (m_pTaskbarList)
					m_pTaskbarList->SetProgressState(m_hWnd, TBPF_ERROR);
				break;

			}
			else
			{
				m_cList.SetItemData(i,  CPatchListCtrl::STATUS_APPLY_SUCCESS);
				this->AddLogString(CString(MAKEINTRESOURCE(IDS_SUCCESS)));
			}

		}
		else
		{
			CString sMessage;
			sMessage.Format(IDS_PROC_SKIPPATCH, static_cast<LPCTSTR>(m_cList.GetItemText(i, 0)));
			AddLogString(sMessage);
			m_cList.SetItemData(i, CPatchListCtrl::STATUS_APPLY_SKIP);
		}

		m_cList.SetItemData(m_CurrentItem, (~CPatchListCtrl::STATUS_APPLYING)&m_cList.GetItemData(i));
		++m_CurrentItem;

		this->m_cList.GetItemRect(i,&rect,LVIR_BOUNDS);
		this->m_cList.InvalidateRect(rect);

		UpdateOkCancelText();
	}

	//in case am fail, need refresh finial item status
	CRect rect;
	this->m_cList.GetItemRect(i,&rect,LVIR_BOUNDS);
	this->m_cList.InvalidateRect(rect);

	this->m_cList.GetItemRect(m_CurrentItem,&rect,LVIR_BOUNDS);
	this->m_cList.InvalidateRect(rect);

	if (m_pTaskbarList)
	{
		m_pTaskbarList->SetProgressValue(m_hWnd, m_CurrentItem, m_cList.GetItemCount());
		if (m_bExitThread && m_CurrentItem != m_cList.GetItemCount())
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_PAUSED);
		else if (!m_bExitThread && m_CurrentItem == m_cList.GetItemCount())
			m_pTaskbarList->SetProgressState(m_hWnd, TBPF_NORMAL);
	}

	EnableInputCtrl(true);
	InterlockedExchange(&m_bThreadRunning, FALSE);
	UpdateOkCancelText();
	return 0;
}

void CImportPatchDlg::OnBnClickedOk()
{
	m_PathList.Clear();
	this->UpdateData();

	SaveSplitterPos();

	if (!CAppUtils::CheckUserData(GetSafeHwnd()))
		return;

	if(IsFinish())
	{
		this->OnOK();
		return;
	}

	m_ctrlTabCtrl.SetActiveTab(1);

	EnableInputCtrl(false);
	InterlockedExchange(&m_bThreadRunning, TRUE);
	InterlockedExchange(&this->m_bExitThread, FALSE);
	if ((m_LoadingThread = AfxBeginThread(ThreadEntry, this)) == nullptr)
	{
		InterlockedExchange(&m_bThreadRunning, FALSE);
		CMessageBox::Show(GetSafeHwnd(), IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
	}
}

void CImportPatchDlg::DoSize(int delta)
{
	this->RemoveAllAnchors();

	auto hdwp = BeginDeferWindowPos(6);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_LIST_PATCH), 0, 0, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_AM_TAB), 0, delta, 0, 0);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECK_3WAY), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_CHECK_IGNORE_SPACE), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_SIGN_OFF), 0, delta, 0, delta);
	hdwp = CSplitterControl::ChangeRect(hdwp, GetDlgItem(IDC_KEEP_CR), 0, delta, 0, delta);
	EndDeferWindowPos(hdwp);

	this->AddAmAnchor();
	// adjust the minimum size of the dialog to prevent the resizing from
	// moving the list control too far down.
	CRect rcLogMsg;
	m_cList.GetClientRect(rcLogMsg);
	SetMinTrackSize(CSize(m_DlgOrigRect.Width(), m_DlgOrigRect.Height()-m_PatchListOrigRect.Height()+rcLogMsg.Height()));

	SetSplitterRange();
//	m_CommitList.Invalidate();

//	GetDlgItem(IDC_LOGMESSAGE)->Invalidate();

	this->m_ctrlTabCtrl.Invalidate();
}

void CImportPatchDlg::OnSize(UINT nType, int cx, int cy)
{
	CResizableStandAloneDialog::OnSize(nType, cx, cy);

	SetSplitterRange();
}

LRESULT CImportPatchDlg::DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) {
	case WM_NOTIFY:
		if (wParam == IDC_AM_SPLIT)
		{
			auto pHdr = reinterpret_cast<SPC_NMHDR*>(lParam);
			DoSize(pHdr->delta);
		}
		break;
	}

	return CResizableStandAloneDialog::DefWindowProc(message, wParam, lParam);
}

void CImportPatchDlg::SaveSplitterPos()
{
	if (!IsIconic())
	{
		CRegDWORD regPos(L"Software\\TortoiseGit\\TortoiseProc\\ResizableState\\AMDlgSizer");
		RECT rectSplitter;
		m_wndSplitter.GetWindowRect(&rectSplitter);
		ScreenToClient(&rectSplitter);
		regPos = rectSplitter.top;
	}
}

void CImportPatchDlg::EnableInputCtrl(BOOL b)
{
	this->GetDlgItem(IDC_BUTTON_UP)->EnableWindow(b);
	this->GetDlgItem(IDC_BUTTON_DOWN)->EnableWindow(b);
	this->GetDlgItem(IDC_BUTTON_REMOVE)->EnableWindow(b);
	this->GetDlgItem(IDC_BUTTON_ADD)->EnableWindow(b);
	this->GetDlgItem(IDOK)->EnableWindow(b);
}

void CImportPatchDlg::UpdateOkCancelText()
{
	if (this->m_bThreadRunning && !IsFinish())
	{
		this->GetDlgItem(IDOK)->EnableWindow(FALSE);
		this->GetDlgItem(IDCANCEL)->SetWindowText(CString(MAKEINTRESOURCE(IDS_ABORTBUTTON)));
	}
	else if (!IsFinish())
	{
		this->GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	else
	{
		this->GetDlgItem(IDCANCEL)->EnableWindow(FALSE);
		this->GetDlgItem(IDOK)->SetWindowText(CString(MAKEINTRESOURCE(IDS_OKBUTTON)));
	}
}
void CImportPatchDlg::OnBnClickedCancel()
{
	if(this->m_bThreadRunning)
	{
		InterlockedExchange(&m_bExitThread,TRUE);
	}
	else
	{
		CTGitPath path;
		path.SetFromWin(g_Git.m_CurrentDir);
		if(path.HasRebaseApply())
			if (CMessageBox::Show(GetSafeHwnd(), IDS_PROC_APPLYPATCH_GITAMACTIVE, IDS_APPNAME, MB_YESNO | MB_ICONQUESTION) == IDYES)
			{
				CString output;
				if (g_Git.Run(L"git.exe am --abort", &output, CP_UTF8))
					MessageBox(output, L"TortoiseGit", MB_OK | MB_ICONERROR);
			}
		OnCancel();
	}
}

void CImportPatchDlg::AddLogString(const CString& str)
{
	this->m_wndOutput.SendMessage(SCI_SETREADONLY, FALSE);
	CStringA sTextA = m_wndOutput.StringForControl(str);//CUnicodeUtils::GetUTF8(str);
	this->m_wndOutput.SendMessage(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>(static_cast<LPCSTR>(sTextA)));
	this->m_wndOutput.SendMessage(SCI_REPLACESEL, 0, reinterpret_cast<LPARAM>("\n"));
	this->m_wndOutput.SendMessage(SCI_SETREADONLY, TRUE);
	//this->m_wndOutput.SendMessage(SCI_LINESCROLL,0,0x7FFFFFFF);
}
BOOL CImportPatchDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		switch (pMsg->wParam)
		{
		/* Avoid TAB control destroy but dialog exist*/
		case VK_ESCAPE:
		case VK_CANCEL:
			{
				TCHAR buff[129];
				::GetClassName(pMsg->hwnd, buff, _countof(buff) - 1);

				/* Use MSFTEDIT_CLASS http://msdn.microsoft.com/en-us/library/bb531344.aspx */
				if (_wcsnicmp(buff, MSFTEDIT_CLASS, _countof(buff) - 1) == 0 ||	//Unicode and MFC 2012 and later
					_wcsnicmp(buff, RICHEDIT_CLASS, _countof(buff) - 1) == 0 ||	//ANSI or MFC 2010
					_wcsnicmp(buff, L"Scintilla", _countof(buff) - 1) == 0 ||
					_wcsnicmp(buff, L"SysListView32", _countof(buff) - 1) == 0 ||
					::GetParent(pMsg->hwnd) == this->m_ctrlTabCtrl.m_hWnd)
				{
					this->PostMessage(WM_KEYDOWN,VK_ESCAPE,0);
					return TRUE;
				}
			}
		}
	}
	return CResizableStandAloneDialog::PreTranslateMessage(pMsg);
}

void CImportPatchDlg::OnHdnItemchangedListPatch(NMHDR * /*pNMHDR*/, LRESULT *pResult)
{
	*pResult = 0;

	if(this->m_cList.GetSelectedCount() != 1)
	{
		m_PatchCtrl.SendMessage(SCI_SETREADONLY, FALSE);
		m_PatchCtrl.SetText(CString());
		m_PatchCtrl.SendMessage(SCI_SETREADONLY, TRUE);
	}
	else
	{
		CString text;

		POSITION pos;
		pos = m_cList.GetFirstSelectedItemPosition();
		int selected = m_cList.GetNextSelectedItem(pos);

		if(selected>=0&& selected< m_cList.GetItemCount())
		{
			CString str = m_cList.GetItemText(selected,0);
			m_PatchCtrl.SendMessage(SCI_SETREADONLY, FALSE);
			m_PatchCtrl.SetText(text);
			m_PatchCtrl.LoadFromFile(str);
			m_PatchCtrl.SendMessage(SCI_SETREADONLY, TRUE);

		}
		else
		{
			m_PatchCtrl.SendMessage(SCI_SETREADONLY, FALSE);
			m_PatchCtrl.SetText(text);
			m_PatchCtrl.SendMessage(SCI_SETREADONLY, TRUE);
		}
	}
}

LRESULT CImportPatchDlg::OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam)
{
	m_pTaskbarList.Release();
	m_pTaskbarList.CoCreateInstance(CLSID_TaskbarList);
	return __super::OnTaskbarButtonCreated(wParam, lParam);
}

void CImportPatchDlg::OnSysColorChange()
{
	__super::OnSysColorChange();
	m_PatchCtrl.SetUDiffStyle();
	m_wndOutput.SetColors(true);
	CAppUtils::SetListCtrlBackgroundImage(m_cList.GetSafeHwnd(), IDI_IMPORTPATHCES_BKG);
}

LRESULT CImportPatchDlg::OnThemeChanged()
{
	CMFCVisualManager::GetInstance()->DestroyInstance();
	return 0;
}
