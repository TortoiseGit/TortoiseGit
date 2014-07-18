// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit

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

static UINT WM_GUIUPDATES = RegisterWindowMessage(_T("TORTOISEGIT_CHOOSEVERSION_GUIUPDATES"));

class CChooseVersion
{
public:
	CString m_initialRefName;

private:
	CWnd *	m_pWin;
	CWinThread*			m_pLoadingThread;
	static UINT LoadingThreadEntry(LPVOID pVoid)
	{
		return ((CChooseVersion*)pVoid)->LoadingThread();
	};
	volatile LONG 		m_bLoadingThreadRunning;

protected:
	CHistoryCombo	m_ChooseVersioinBranch;
	CHistoryCombo	m_ChooseVersioinTags;
	CHistoryCombo	m_ChooseVersioinVersion;
	CButton			m_RadioBranch;
	CButton			m_RadioTag;
	CString			m_pendingRefName;
	bool			m_bNotFullName;
	BOOL			m_AbortLoading;

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
		int radio=m_pWin->GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
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
		// tell the dialog to use mode for selecting revisions
		dlg.SetSelect(true);
		// only one revision must be selected however
		dlg.SingleSelection(true);
		if ( dlg.DoModal() == IDOK )
		{
			// get selected hash if any
			CString selectedHash = dlg.GetSelectedHash();
			// load into window, do this even if empty so that it is clear that nothing has been selected
			m_ChooseVersioinVersion.SetWindowText( selectedHash );
			OnVersionChanged();
		}
	}

	void UpdateRevsionName()
	{
		int radio=m_pWin->GetCheckedRadioButton(IDC_RADIO_HEAD,IDC_RADIO_VERSION);
		switch (radio)
		{
		case IDC_RADIO_HEAD:
			this->m_VersionName=_T("HEAD");
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
			InitChooseVersion(false, true);
			return;
		}
		m_pendingRefName = resultRef;
		m_bNotFullName = false;
		InitChooseVersion(false, true);
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

		if(wcsncmp(refName,L"refs/",5)==0)
			refName = refName.Mid(5);
		if(wcsncmp(refName,L"heads/",6)==0)
		{
			refName = refName.Mid(6);
			SetDefaultChoose(IDC_RADIO_BRANCH);
			m_ChooseVersioinBranch.SetCurSel(
				m_ChooseVersioinBranch.FindStringExact(-1, refName));
		}
		else if(wcsncmp(refName,L"remotes/",8)==0)
		{
			SetDefaultChoose(IDC_RADIO_BRANCH);
			m_ChooseVersioinBranch.SetCurSel(
				m_ChooseVersioinBranch.FindStringExact(-1, refName));
		}
		else if(wcsncmp(refName,L"tags/",5)==0)
		{
			refName = refName.Mid(5);
			refName.Replace(_T("^{}"), _T(""));
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
		do
		{
			STRING_VECTOR list;
			int current = -1;
			g_Git.GetBranchList(list, &current, CGit::BRANCH_ALL_F, &m_AbortLoading);
			if (m_AbortLoading)
				break;
			m_ChooseVersioinBranch.SetList(list, &m_AbortLoading);
			if (m_AbortLoading)
				break;
			m_ChooseVersioinBranch.SetCurSel(current);

			list.clear();
			g_Git.GetTagList(list, &m_AbortLoading);
			if (m_AbortLoading)
				break;
			m_ChooseVersioinTags.SetList(list, &m_AbortLoading);
			if (m_AbortLoading)
				break;
			m_ChooseVersioinTags.SetCurSel(0);

			m_pWin->SendMessage(WM_GUIUPDATES);
		} while (0);

		InterlockedExchange(&m_bLoadingThreadRunning, FALSE);
		return 0;
	}
	void UpdateGUI()
	{
		m_RadioBranch.SetWindowText(L"Branch");
		m_RadioTag.SetWindowText(L"Tag");
		m_RadioBranch.EnableWindow(TRUE);
		m_RadioTag.EnableWindow(TRUE);

		if (m_pendingRefName.IsEmpty())
			OnVersionChanged();
		else
			SelectRef(m_pendingRefName, m_bNotFullName);

		if (m_bIsFirstTimeToSetFocus && m_pWin->GetDlgItem(IDC_COMBOBOXEX_BRANCH)->IsWindowEnabled())
			m_pWin->GetDlgItem(IDC_COMBOBOXEX_BRANCH)->SetFocus();
		m_bIsFirstTimeToSetFocus = false;
		m_pWin->GetDlgItem(IDOK)->EnableWindow(TRUE);
	}
	void InitChooseVersion(bool setFocusToBranchComboBox = false, bool bReInit = false, bool bWaitLoading = true)
	{
		m_ChooseVersioinBranch.SetMaxHistoryItems(0x7FFFFFFF);
		m_ChooseVersioinTags.SetMaxHistoryItems(0x7FFFFFFF);

		m_bIsBranch = false;
		m_RadioBranch.SetWindowText(L"Loading Branches");
		m_RadioTag.SetWindowText(L"Loading Tags");
		m_RadioBranch.EnableWindow(FALSE);
		m_RadioTag.EnableWindow(FALSE);

		m_bIsFirstTimeToSetFocus = setFocusToBranchComboBox;
		if (!bReInit)
		{
			m_pendingRefName = m_initialRefName;
			m_bNotFullName = true;
		}

		if (bWaitLoading)
			m_pWin->GetDlgItem(IDOK)->EnableWindow(FALSE);

		InterlockedExchange(&m_bLoadingThreadRunning, TRUE);

		if ( (m_pLoadingThread=AfxBeginThread(LoadingThreadEntry, this)) ==NULL)
		{
			InterlockedExchange(&m_bLoadingThreadRunning, FALSE);
			CMessageBox::Show(NULL, IDS_ERR_THREADSTARTFAILED, IDS_APPNAME, MB_OK | MB_ICONERROR);
		}
	}
	void WaitForFinishLoading()
	{
		if(m_bLoadingThreadRunning && m_pLoadingThread)
		{
			m_AbortLoading = TRUE;
			::WaitForSingleObject(m_pLoadingThread->m_hThread, 20000);
		}
	}
public:
	CString m_VersionName;
	bool	m_bIsBranch;
	bool	m_bIsFirstTimeToSetFocus;
	CChooseVersion(CWnd *win)
	: m_bIsBranch(false)
	, m_bIsFirstTimeToSetFocus(false)
	, m_pLoadingThread(nullptr)
	, m_bLoadingThreadRunning(FALSE)
	, m_bNotFullName(true)
	, m_AbortLoading(FALSE)
	{
		m_pWin=win;
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
	}

#define CHOOSE_EVENT_RADIO() \
	LRESULT OnUpdateGUIHost(WPARAM, LPARAM) { UpdateGUI(); return 0; } \
	afx_msg void OnBnClickedChooseRadioHost(){OnBnClickedChooseRadio();}\
	afx_msg void OnBnClickedShow(){OnBnClickedChooseVersion();}\
	afx_msg void OnBnClickedButtonBrowseRefHost(){OnBnClickedButtonBrowseRef();}
