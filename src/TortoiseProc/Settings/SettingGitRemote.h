// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015, 2017 - TortoiseGit

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
#include "SettingsPropPage.h"
#include "registry.h"

// CSettingGitRemote dialog
class CSettingGitRemote : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSettingGitRemote)

public:
	enum
	{
		REMOTE_NAME		=0x1,
		REMOTE_URL		=0x2,
		REMOTE_PUTTYKEY	=0x4,
		REMOTE_TAGOPT	=0x8,
		REMOTE_PRUNE	=0x10,
		REMOTE_PRUNEALL	=0x20,
		REMOTE_PUSHDEFAULT	= 0x40,
		REMOTE_PUSHURL	=0x80,
	};
	CSettingGitRemote();
	virtual ~CSettingGitRemote();
	UINT GetIconID() override { return IDI_GITREMOTE; }
// Dialog Data
	enum { IDD = IDD_SETTINREMOTE };

	bool		m_bNoFetch;

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnLbnSelchangeListRemote();
	afx_msg void OnEnChangeEditRemote();
	afx_msg void OnEnChangeEditUrl();
	afx_msg void OnEnChangeEditPushUrl();
	afx_msg void OnEnChangeEditPuttyKey();
	afx_msg void OnCbnSelchangeComboTagOpt();
	afx_msg void OnBnClickedCheckprune();
	afx_msg void OnBnClickedCheckpruneall();
	afx_msg void OnBnClickedCheckpushdefault();
	afx_msg void OnBnClickedButtonRemove();
	afx_msg void OnBnClickedButtonRenameRemote();

	virtual BOOL OnInitDialog() override;
	virtual BOOL OnApply() override;

	BOOL IsRemoteExist(CString &remote);
	bool IsRemoteCollideWithRefspec(CString remote);

	BOOL Save(CString key, CString value);
	BOOL SaveGeneral(CString key, CString value);

	int			m_ChangedMask;

	CListBox	m_ctrlRemoteList;
	CString		m_strRemote;
	CString		m_strUrl;
	CString		m_strPushUrl;

	CString		m_strPuttyKeyfile;
	CComboBox	m_ctrlTagOpt;
	BOOL		m_bPushDefault;
	BOOL		m_bPrune;
	BOOL		m_bPruneAll;
};
