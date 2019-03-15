// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009-2019 - TortoiseGit

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
#include "LogDlg.h"
#include "BrowseRefsDlg.h"
#include "HistoryCombo.h"
#include "LoglistUtils.h"
#include "UnicodeUtils.h"
#include "Tooltip.h"

class CBranchCombox
{
public:
	CBranchCombox()
		: m_LocalBranchFilter(gPickRef_Head)
		, m_RemoteBranchFilter(gPickRef_Remote)
		, m_DialogName(L"sync")
		, m_pTooltip(nullptr)
	{
	}
protected:
	CHistoryCombo m_ctrlLocalBranch;
	CHistoryCombo m_ctrlRemoteBranch;
	int m_LocalBranchFilter;
	int m_RemoteBranchFilter;

	CToolTips *m_pTooltip;

	CString m_DialogName;

	CString m_RegKeyRemoteBranch;

	void  CbnSelchangeLocalBranch()
	{
		//Select pull-remote from current branch
		CString pullRemote, pullBranch;
		g_Git.GetRemoteTrackedBranch(m_ctrlLocalBranch.GetString(), pullRemote, pullBranch);

		this->SetRemote(pullRemote);

		CString defaultUpstream;
		defaultUpstream.Format(L"remotes/%s/%s", static_cast<LPCTSTR>(pullRemote), static_cast<LPCTSTR>(pullBranch));
		int found = m_ctrlRemoteBranch.FindStringExact(0, defaultUpstream);
		if(found >= 0)
			m_ctrlRemoteBranch.SetCurSel(found);
		else if(!pullBranch.IsEmpty())
		{
			int index=m_ctrlRemoteBranch.FindStringExact(0,pullBranch);
			if( index<0 )
				m_ctrlRemoteBranch.AddString(pullBranch);
			else
				m_ctrlRemoteBranch.SetCurSel(index);
		}
		else
			m_ctrlRemoteBranch.SetCurSel(-1);

		AddBranchToolTips(m_ctrlLocalBranch, m_pTooltip);

		LocalBranchChange();
	};
	void  CbnSelchangeRemoteBranch()
	{
		if(this->m_RegKeyRemoteBranch.IsEmpty())
			AddBranchToolTips(m_ctrlRemoteBranch, m_pTooltip);

		RemoteBranchChange();
	}
	void  BnClickedButtonBrowseLocalBranch()
	{
		if (CBrowseRefsDlg::PickRefForCombo(m_ctrlLocalBranch, m_LocalBranchFilter))
			CbnSelchangeLocalBranch();
	}
	void  BnClickedButtonBrowseRemoteBranch()
	{
		if(!this->m_RegKeyRemoteBranch.IsEmpty())
		{
			CString remoteBranchName;
			CString remoteName;
			this->m_ctrlRemoteBranch.GetWindowText(remoteBranchName);
			//remoteName = m_Remote.GetString();
			//remoteBranchName = remoteName + '/' + remoteBranchName;
			remoteBranchName = CBrowseRefsDlg::PickRef(false, remoteBranchName, gPickRef_Remote);
			if(remoteBranchName.IsEmpty())
				return; //Canceled

			remoteBranchName = remoteBranchName.Mid(static_cast<int>(wcslen(L"refs/remotes/"))); //Strip 'refs/remotes/'
			int slashPlace = remoteBranchName.Find('/');
			remoteName = remoteBranchName.Left(slashPlace);
			remoteBranchName = remoteBranchName.Mid(slashPlace + 1); //Strip remote name (for example 'origin/')

			//Select remote
			//int remoteSel = m_Remote.FindStringExact(0,remoteName);
			//if(remoteSel >= 0)
			//	m_Remote.SetCurSel(remoteSel);
			this->SetRemote(remoteName);

			//Select branch
			m_ctrlRemoteBranch.AddString(remoteBranchName);
			CbnSelchangeRemoteBranch();

		}
		else
		{
			if (CBrowseRefsDlg::PickRefForCombo(m_ctrlRemoteBranch, m_RemoteBranchFilter))
				CbnSelchangeRemoteBranch();
		}
	}

	virtual void LocalBranchChange(){};
	virtual void RemoteBranchChange(){};
	virtual void SetRemote(CString remote){};

	void AddBranchToolTips(CHistoryCombo& pBranch, CToolTips* tip)
	{
		pBranch.DisableTooltip();

		if (!tip)
			return;

		CString text = pBranch.GetString();
		if (text.IsEmpty())
			return;

		GitRev rev;
		if (rev.GetCommit(text))
		{
			MessageBox(nullptr, rev.GetLastErr(), L"TortoiseGit", MB_ICONERROR);
			return;
		}

		CString tooltip;
		tooltip.Format(L"%s: %s\n%s: %s <%s>\n%s: %s\n%s:\n%s\n%s",
						static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_REVISION))),
						static_cast<LPCTSTR>(rev.m_CommitHash.ToString()),
						static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_AUTHOR))),
						static_cast<LPCTSTR>(rev.GetAuthorName()),
						static_cast<LPCTSTR>(rev.GetAuthorEmail()),
						static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_DATE))),
						static_cast<LPCTSTR>(CLoglistUtils::FormatDateAndTime(rev.GetAuthorDate(), DATE_LONGDATE)),
						static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(IDS_LOG_MESSAGE))),
						static_cast<LPCTSTR>(rev.GetSubject()),
						static_cast<LPCTSTR>(rev.GetBody()));

		if (tooltip.GetLength() > 8000)
		{
			tooltip.Truncate(8000);
			tooltip += L"...";
		}

		tip->AddTool(pBranch.GetComboBoxCtrl(), tooltip);
	}

	void LoadBranchInfo()
	{
		m_ctrlLocalBranch.SetMaxHistoryItems(0x0FFFFFFF);
		m_ctrlRemoteBranch.SetMaxHistoryItems(0x0FFFFFFF);

		STRING_VECTOR list;
		list.clear();
		int current=0;

		g_Git.GetBranchList(list,&current,CGit::BRANCH_LOCAL_F);

		m_ctrlLocalBranch.SetList(list);

		if(this->m_RegKeyRemoteBranch.IsEmpty())
		{
			list.clear();
			g_Git.GetBranchList(list, nullptr, CGit::BRANCH_REMOTE);

			m_ctrlRemoteBranch.SetList(list);
		}
		else
		{
			m_ctrlRemoteBranch.Reset();
			m_ctrlRemoteBranch.LoadHistory(m_RegKeyRemoteBranch, L"sync");
		}

		if(!this->m_strLocalBranch.IsEmpty())
			m_ctrlLocalBranch.AddString(m_strLocalBranch);
		else
			m_ctrlLocalBranch.SetCurSel(current);

		if(!m_strRemoteBranch.IsEmpty())
		{
			m_ctrlRemoteBranch.AddString(m_strRemoteBranch);
			m_ctrlRemoteBranch.SetCurSel(m_ctrlRemoteBranch.GetCount()-1);
		}
		else
			CbnSelchangeLocalBranch();

		this->LocalBranchChange();
		this->RemoteBranchChange();
	}

public:
	CString m_strLocalBranch;
	CString m_strRemoteBranch;

	void SaveHistory()
	{
		if(!this->m_RegKeyRemoteBranch.IsEmpty())
		{
			m_ctrlRemoteBranch.AddString(m_strRemoteBranch);
			m_ctrlRemoteBranch.SaveHistory();
		}
	}
};

#define BRANCH_COMBOX_DDX \
	DDX_Control(pDX, IDC_COMBOBOXEX_LOCAL_BRANCH,		m_ctrlLocalBranch);      \
	DDX_Control(pDX, IDC_COMBOBOXEX_REMOTE_BRANCH,		m_ctrlRemoteBranch);     \

#define BRANCH_COMBOX_EVENT \
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_LOCAL_BRANCH,   OnCbnSelchangeLocalBranch)	\
	ON_CBN_SELCHANGE(IDC_COMBOBOXEX_REMOTE_BRANCH,  OnCbnSelchangeRemoteBranch)	\
	ON_BN_CLICKED(IDC_BUTTON_LOCAL_BRANCH,			OnBnClickedButtonBrowseLocalBranch) \
	ON_BN_CLICKED(IDC_BUTTON_REMOTE_BRANCH,			OnBnClickedButtonBrowseRemoteBranch) \

#define BRANCH_COMBOX_ADD_ANCHOR() \
	AddAnchor(IDC_COMBOBOXEX_LOCAL_BRANCH,TOP_LEFT);\
	AddAnchor(IDC_COMBOBOXEX_REMOTE_BRANCH,TOP_RIGHT);\
	AddAnchor(IDC_BUTTON_LOCAL_BRANCH,TOP_LEFT);\
	AddAnchor(IDC_BUTTON_REMOTE_BRANCH,TOP_RIGHT);\
	AddAnchor(IDC_STATIC_REMOTE_BRANCH,TOP_RIGHT);

#define BRANCH_COMBOX_EVENT_HANDLE() \
	afx_msg void OnCbnSelchangeLocalBranch(){CbnSelchangeLocalBranch();} \
	afx_msg void OnCbnSelchangeRemoteBranch(){CbnSelchangeRemoteBranch();}\
	afx_msg void OnBnClickedButtonBrowseLocalBranch(){BnClickedButtonBrowseLocalBranch();}\
	afx_msg void OnBnClickedButtonBrowseRemoteBranch(){BnClickedButtonBrowseRemoteBranch();}
