﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2020, 2023-2025 - TortoiseGit

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
#pragma once
#include "afxwin.h"
#include "LogDlg.h"
#include "BrowseRefsDlg.h"
#include "MessageBox.h"
#include "registry.h"
#include "StringUtils.h"

static UINT WM_GUIUPDATES = RegisterWindowMessage(L"TORTOISEGIT_CHOOSEVERSION_GUIUPDATES");

class CChooseVersion
{
public:
	CString m_initialRefName;

private:
	CWnd*	m_pWin = nullptr;
	CWinThread* m_pLoadingThread = nullptr;
	static UINT LoadingThreadEntry(LPVOID pVoid)
	{
		return static_cast<CChooseVersion*>(pVoid)->LoadingThread();
	};
	volatile LONG m_bLoadingThreadRunning = FALSE;
	std::atomic<bool> m_bExitLoadingThread;
	struct GUI_UPDATE_DATA
	{
		STRING_VECTOR branches;
		int current_branch_idx = -1;
		STRING_VECTOR tags;
	};
	std::atomic<std::shared_ptr<GUI_UPDATE_DATA>> m_refData;

protected:
	CHistoryCombo	m_ChooseVersioinBranch;
	CHistoryCombo	m_ChooseVersioinTags;
	CHistoryCombo	m_ChooseVersioinVersion;
	CButton			m_RadioBranch;
	CButton			m_RadioTag;
	CString			m_pendingRefName;
	bool			m_bNotFullName = true;
	bool			m_bSkipCurrentBranch = false;

	//Notification when version changed. Can be implemented in derived classes.
	virtual void OnVersionChanged(){}

	afx_msg void OnBnClickedChooseRadio()
	{
		this->m_ChooseVersioinTags.EnableWindow(FALSE);
		this->m_ChooseVersioinBranch.EnableWindow(FALSE);
		this->m_ChooseVersioinVersion.EnableWindow(FALSE);
		m_pWin->GetDlgItem(IDC_BUTTON_BROWSE_REF)->EnableWindow(FALSE);
		m_pWin->GetDlgItem(IDC_BUTTON_SHOW)->EnableWindow(FALSE);
		m_bIsBranch = false;
		const int radio = m_pWin->GetCheckedRadioButton(IDC_RADIO_HEAD, IDC_RADIO_VERSION);
		switch (radio)
		{
		case IDC_RADIO_HEAD:
			break;
		case IDC_RADIO_BRANCH:
			this->m_ChooseVersioinBranch.EnableWindow(TRUE);
			m_pWin->GetDlgItem(IDC_BUTTON_BROWSE_REF)->EnableWindow(TRUE);
			m_bIsBranch = true;
			break;
		case IDC_RADIO_TAGS:
			this->m_ChooseVersioinTags.EnableWindow(TRUE);
			break;
		case IDC_RADIO_VERSION:
			this->m_ChooseVersioinVersion.EnableWindow(TRUE);
			m_pWin->GetDlgItem(IDC_BUTTON_SHOW)->EnableWindow(TRUE);
		break;
		}
		// enable version browse button if Version is selected
		m_pWin->GetDlgItem(IDC_BUTTON_SHOW)->EnableWindow(radio == IDC_RADIO_VERSION);
		OnVersionChanged();
	}

	void OnBnClickedChooseVersion()
	{
		// use the git log to allow selection of a version
		CLogDlg dlg;
		if (dlg.IsThreadRunning())
		{
			CMessageBox::Show(m_pWin->GetSafeHwnd(), IDS_PROC_LOG_ONLYONCE, IDS_APPNAME, MB_ICONEXCLAMATION);
			return;
		}
		CString revision;
		m_ChooseVersioinVersion.GetWindowText(revision);
		dlg.SetParams(CTGitPath(), CTGitPath(), revision, revision, 0);
		// tell the dialog to use mode for selecting revisions
		dlg.SetSelect(true);
		dlg.ShowWorkingTreeChanges(false);
		// only one revision must be selected however
		dlg.SingleSelection(true);
		if (dlg.DoModal() == IDOK && !dlg.GetSelectedHash().empty())
		{
			m_ChooseVersioinVersion.SetWindowText(dlg.GetSelectedHash().at(0).ToString());
			OnVersionChanged();
		}
		m_pWin->BringWindowToTop(); /* cf. issue #3493 */
	}

	void UpdateRevsionName()
	{
		const int radio = m_pWin->GetCheckedRadioButton(IDC_RADIO_HEAD, IDC_RADIO_VERSION);
		switch (radio)
		{
		case IDC_RADIO_HEAD:
			this->m_VersionName = L"HEAD";
			break;
		case IDC_RADIO_BRANCH:
			this->m_VersionName=m_ChooseVersioinBranch.GetString();
			if (!m_VersionName.IsEmpty() && !g_Git.IsBranchTagNameUnique(this->m_VersionName))
				this->m_VersionName = L"refs/heads/" + this->m_VersionName;
			break;
		case IDC_RADIO_TAGS:
			this->m_VersionName = m_ChooseVersioinTags.GetString();
			if (!m_VersionName.IsEmpty() && !g_Git.IsBranchTagNameUnique(this->m_VersionName))
				this->m_VersionName = L"refs/tags/" + m_ChooseVersioinTags.GetString();
			break;
		case IDC_RADIO_VERSION:
			this->m_VersionName=m_ChooseVersioinVersion.GetString();
			break;
		}
	}
	void SetDefaultChoose(int id)
	{
		m_pWin->CheckRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION,id);
		OnBnClickedChooseRadio();
	}

	void OnBnClickedButtonBrowseRef()
	{
		CString origRef;
		UpdateRevsionName();
		CString resultRef = CBrowseRefsDlg::PickRef(false, m_VersionName, gPickRef_All);
		if(resultRef.IsEmpty())
		{
			m_pendingRefName = m_VersionName;
			m_bNotFullName = true;
			InitChooseVersion(true, true);
			return;
		}
		m_pendingRefName = resultRef;
		m_bNotFullName = false;
		InitChooseVersion(true, true);
	}

	void SelectRef(CString refName, bool bRefNameIsPossiblyNotFullName = true)
	{
		if(bRefNameIsPossiblyNotFullName)
		{
			//Make sure refName is a full ref name first
			CString fullRefName = g_Git.GetFullRefName(refName);
			if(!fullRefName.IsEmpty())
				refName = fullRefName;
		}

		if (CStringUtils::StartsWith(refName, L"refs/"))
			refName = refName.Mid(static_cast<int>(wcslen(L"refs/")));
		if (CStringUtils::StartsWith(refName, L"heads/"))
		{
			refName = refName.Mid(static_cast<int>(wcslen(L"heads/")));
			SetDefaultChoose(IDC_RADIO_BRANCH);
			m_ChooseVersioinBranch.SetCurSel(
				m_ChooseVersioinBranch.FindStringExact(-1, refName));
		}
		else if (CStringUtils::StartsWith(refName, L"remotes/"))
		{
			SetDefaultChoose(IDC_RADIO_BRANCH);
			m_ChooseVersioinBranch.SetCurSel(
				m_ChooseVersioinBranch.FindStringExact(-1, refName));
		}
		else if (CStringUtils::StartsWith(refName, L"tags/"))
		{
			refName = refName.Mid(static_cast<int>(wcslen(L"refs/")));
			if (CStringUtils::EndsWith(refName, L"^{}"))
				refName.Truncate(refName.GetLength() - static_cast<int>(wcslen(L"^{}")));
			SetDefaultChoose(IDC_RADIO_TAGS);
			m_ChooseVersioinTags.SetCurSel(
				m_ChooseVersioinTags.FindStringExact(-1, refName));
		}
		else
		{
			SetDefaultChoose(IDC_RADIO_VERSION);
			m_ChooseVersioinVersion.AddString(refName);
		}
		OnVersionChanged();
	}

	UINT LoadingThread()
	{
		SCOPE_EXIT{ InterlockedExchange(&m_bLoadingThreadRunning, FALSE); };

		auto refData = std::make_shared<GUI_UPDATE_DATA>();
		auto& data = *refData;
		g_Git.GetBranchList(data.branches, &data.current_branch_idx, CRegDWORD(L"Software\\TortoiseGit\\BranchesIncludeFetchHead", TRUE) ? CGit::BRANCH_ALL_F : CGit::BRANCH_ALL, m_bSkipCurrentBranch);

		if (m_bExitLoadingThread)
			return 0;

		g_Git.GetTagList(data.tags);

		m_refData = refData;

		m_pWin->PostMessage(WM_GUIUPDATES);

		return 0;
	}

	void UpdateGUI()
	{
		if (auto data = m_refData.exchange(nullptr); data)
		{
			m_ChooseVersioinBranch.SetList(data->branches);
			m_ChooseVersioinBranch.SetCurSel(data->current_branch_idx);
			m_ChooseVersioinTags.SetList(data->tags);
			m_ChooseVersioinTags.SetCurSel(0);
			if (auto pCurrentBranch = m_pWin->GetDlgItem(IDC_CURRENTBRANCH); pCurrentBranch)
				pCurrentBranch->SetWindowText(g_Git.GetCurrentBranch());
		}

		m_RadioBranch.EnableWindow(TRUE);
		m_RadioTag.EnableWindow(TRUE);

		if (m_pendingRefName.IsEmpty())
			OnVersionChanged();
		else
			SelectRef(m_pendingRefName, m_bNotFullName);

		if (m_bIsFirstTimeToSetFocus)
		{
			if (m_pWin->GetDlgItem(IDC_COMBOBOXEX_BRANCH)->IsWindowEnabled())
				m_pWin->GetDlgItem(IDC_COMBOBOXEX_BRANCH)->SetFocus();
			else if (m_pWin->GetDlgItem(IDC_COMBOBOXEX_TAGS)->IsWindowEnabled())
				m_pWin->GetDlgItem(IDC_COMBOBOXEX_TAGS)->SetFocus();
			else if (m_pWin->GetDlgItem(IDC_COMBOBOXEX_VERSION)->IsWindowEnabled())
				m_pWin->GetDlgItem(IDC_COMBOBOXEX_VERSION)->SetFocus();
		}
		m_bIsFirstTimeToSetFocus = false;
		m_pWin->GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	void InitChooseVersion(bool setFocusToBranchComboBox = false, bool bReInit = false)
	{
		m_ChooseVersioinBranch.SetMaxHistoryItems(0x7FFFFFFF);
		m_ChooseVersioinTags.SetMaxHistoryItems(0x7FFFFFFF);

		m_bIsBranch = false;
		m_RadioBranch.EnableWindow(FALSE);
		m_RadioTag.EnableWindow(FALSE);

		m_bIsFirstTimeToSetFocus = setFocusToBranchComboBox;
		if (!bReInit)
		{
			m_pendingRefName = m_initialRefName;
			m_bNotFullName = true;
		}

		m_pWin->GetDlgItem(IDOK)->EnableWindow(FALSE);

		InterlockedExchange(&m_bLoadingThreadRunning, TRUE);
		m_bExitLoadingThread = false;
		if ((m_pLoadingThread = AfxBeginThread(LoadingThreadEntry, this, 0, CREATE_SUSPENDED)) == nullptr)
		{
			InterlockedExchange(&m_bLoadingThreadRunning, FALSE);
			CMessageBox::Show(nullptr, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
			return;
		}

		m_pLoadingThread->m_bAutoDelete = FALSE;
		m_pLoadingThread->ResumeThread();
	}
	void WaitForFinishLoading()
	{
		if(m_bLoadingThreadRunning && m_pLoadingThread)
		{
			m_bExitLoadingThread = true;
			while (::WaitForSingleObject(m_pLoadingThread->m_hThread, 1000) == WAIT_TIMEOUT)
				CTraceToOutputDebugString::Instance()(_T(__FUNCTION__) L": Waiting for ref loading thread to exit...\n");
		}
		delete m_pLoadingThread;
		m_pLoadingThread = nullptr;
	}
public:
	CString m_VersionName;
	bool	m_bIsBranch = false;
	bool	m_bIsFirstTimeToSetFocus = false;
	CChooseVersion(CWnd *win)
		: m_pWin(win)
	{
	};

	~CChooseVersion()
	{
		WaitForFinishLoading();
	};
};

#define CHOOSE_VERSION_DDX \
	DDX_Control(pDX, IDC_COMBOBOXEX_BRANCH,		m_ChooseVersioinBranch); \
	DDX_Control(pDX, IDC_COMBOBOXEX_TAGS,		m_ChooseVersioinTags); \
	DDX_Control(pDX, IDC_COMBOBOXEX_VERSION,	m_ChooseVersioinVersion); \
	DDX_Control(pDX, IDC_RADIO_BRANCH, m_RadioBranch);\
	DDX_Control(pDX, IDC_RADIO_TAGS, m_RadioTag);

#define CHOOSE_VERSION_EVENT\
	ON_REGISTERED_MESSAGE(WM_GUIUPDATES,	OnUpdateGUIHost)\
	ON_BN_CLICKED(IDC_RADIO_HEAD,			OnBnClickedChooseRadioHost)\
	ON_BN_CLICKED(IDC_RADIO_BRANCH,			OnBnClickedChooseRadioHost)\
	ON_BN_CLICKED(IDC_RADIO_TAGS,			OnBnClickedChooseRadioHost)\
	ON_BN_CLICKED(IDC_BUTTON_SHOW, 			OnBnClickedShow)\
	ON_BN_CLICKED(IDC_RADIO_VERSION,		OnBnClickedChooseRadioHost)\
	ON_BN_CLICKED(IDC_BUTTON_BROWSE_REF,	OnBnClickedButtonBrowseRefHost)

#define CHOOSE_VERSION_ADDANCHOR								\
	{															\
		AddAnchor(IDC_COMBOBOXEX_BRANCH, TOP_LEFT, TOP_RIGHT);	\
		AddAnchor(IDC_COMBOBOXEX_TAGS, TOP_LEFT, TOP_RIGHT);	\
		AddAnchor(IDC_COMBOBOXEX_VERSION, TOP_LEFT, TOP_RIGHT);	\
		AddAnchor(IDC_GROUP_BASEON, TOP_LEFT, TOP_RIGHT);		\
		AddAnchor(IDC_BUTTON_SHOW,TOP_RIGHT);					\
		AddAnchor(IDC_BUTTON_BROWSE_REF,TOP_RIGHT);				\
		if (auto pCurrentBranch = GetDlgItem(IDC_CURRENTBRANCH); pCurrentBranch) \
			AddAnchor(IDC_CURRENTBRANCH, TOP_LEFT, TOP_RIGHT); \
	}

#define CHOOSE_EVENT_RADIO() \
	LRESULT OnUpdateGUIHost(WPARAM, LPARAM) { UpdateGUI(); return 0; } \
	afx_msg void OnBnClickedChooseRadioHost(){OnBnClickedChooseRadio();}\
	afx_msg void OnBnClickedShow(){OnBnClickedChooseVersion();}\
	afx_msg void OnBnClickedButtonBrowseRefHost(){OnBnClickedButtonBrowseRef();}
