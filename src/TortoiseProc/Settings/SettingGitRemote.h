// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit

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
#include "SettingsPropPage.h"
#include "Tooltip.h"
#include "registry.h"
#include "afxwin.h"

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
	};
	CSettingGitRemote(CString cmdPath);
	virtual ~CSettingGitRemote();
	UINT GetIconID() {return IDI_GITREMOTE;}
// Dialog Data
	enum { IDD = IDD_SETTINREMOTE };

	bool		m_bNoFetch;

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnBnClickedButtonAdd();
	afx_msg void OnLbnSelchangeListRemote();
	afx_msg void OnEnChangeEditRemote();
	afx_msg void OnEnChangeEditUrl();
	afx_msg void OnEnChangeEditPuttyKey();
	afx_msg void OnCbnSelchangeComboTagOpt();
	afx_msg void OnBnClickedButtonRemove();

	BOOL OnInitDialog();
	BOOL OnApply();

	BOOL IsRemoteExist(CString &remote);

	void Save(CString key, CString value);

	int			m_ChangedMask;

	CString		m_cmdPath;

	CListBox	m_ctrlRemoteList;
	CString		m_strRemote;
	CString		m_strUrl;

	CString		m_strPuttyKeyfile;
	CComboBox	m_ctrlTagOpt;
};
