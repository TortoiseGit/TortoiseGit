// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2007-2008 - TortoiseSVN

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
#include "Tooltip.h"
#include "Registry.h"
#include "ILogReceiver.h"

class CProgressDlg;

/**
 * \ingroup TortoiseProc
 * Settings page to configure miscellaneous stuff. 
 */
class CSettingsLogCaches 
    : public ISettingsPropPage
    , private ILogReceiver
{
	DECLARE_DYNAMIC(CSettingsLogCaches)

public:
	CSettingsLogCaches();
	virtual ~CSettingsLogCaches();
	
	UINT GetIconID() {return IDI_CACHELIST;}

    // update cache list

	virtual BOOL OnSetActive();

// Dialog Data
	enum { IDD = IDD_SETTINGSLOGCACHELIST };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	afx_msg void OnBnClickedDetails();
	afx_msg void OnBnClickedUpdate();
	afx_msg void OnBnClickedExport();
	afx_msg void OnBnClickedDelete();

	afx_msg LRESULT OnRefeshRepositoryList (WPARAM wParam, LPARAM lParam);
	afx_msg void OnNMDblclkRepositorylist(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLvnItemchangedRepositorylist(NMHDR *pNMHDR, LRESULT *pResult);

    DECLARE_MESSAGE_MAP()
private:
	CToolTips		m_tooltips;

	CListCtrl       m_cRepositoryList;

    /// current repository list

    typedef std::multimap<CString, CString> TRepos;
    typedef TRepos::value_type TRepo;
    typedef TRepos::const_iterator IT;
    TRepos          repos;

    TRepo GetSelectedRepo();
    void FillRepositoryList();

    static UINT WorkerThread(LPVOID pVoid);

    /// used by cache update

    CProgressDlg*   progress;
    CString    headRevision;

    void ReceiveLog ( LogChangedPathArray* changes
	                , CString rev
                    , const StandardRevProps* stdRevProps
                    , UserRevPropArray* userRevProps
                    , bool mergesFollow);
};
