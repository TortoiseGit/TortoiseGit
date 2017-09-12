// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "StandAloneDlg.h"
#include "registry.h"

/**
 * \ingroup TortoiseProc
 * Settings page to configure the overlay icon set to use.
 */
class CSetOverlayIcons : public ISettingsPropPage
{
	DECLARE_DYNAMIC(CSetOverlayIcons)

public:
	CSetOverlayIcons();
	virtual ~CSetOverlayIcons();

	UINT GetIconID() override { return IDI_ICONSET; }

// Dialog Data
	enum { IDD = IDD_SETOVERLAYICONS };

protected:
	virtual void	DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL	OnInitDialog() override;
	virtual BOOL	OnApply() override;
	afx_msg void	OnBnClickedListradio();
	afx_msg void	OnBnClickedSymbolradio();
	afx_msg void	OnCbnSelchangeIconsetcombo();

	void			ShowIconSet(bool bSmallIcons);
	void			AddFileTypeGroup(CString sFileType, bool bSmallIcons);
	DECLARE_MESSAGE_MAP()

	int				m_selIndex;
	CString			m_sIconSet;
	CComboBox		m_cIconSet;
	CListCtrl		m_cIconList;

	CString			m_sIconPath;
	CString			m_sOriginalIconSet;
	CString			m_sNormal;
	CString			m_sModified;
	CString			m_sConflicted;
	CString			m_sReadOnly;
	CString			m_sDeleted;
	CString			m_sAdded;
	CString			m_sLocked;
	CString			m_sIgnored;
	CString			m_sUnversioned;
	CImageList		m_ImageList;
	CImageList		m_ImageListBig;

	CRegString		m_regNormal;
	CRegString		m_regModified;
	CRegString		m_regConflicted;
	CRegString		m_regReadOnly;
	CRegString		m_regDeleted;
	CRegString		m_regLocked;
	CRegString		m_regAdded;
	CRegString		m_regIgnored;
	CRegString		m_regUnversioned;
};
