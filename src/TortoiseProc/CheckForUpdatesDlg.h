// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2014, 2016-2017 - TortoiseGit
// Copyright (C) 2003-2008 - Stefan Kueng

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

#include "StandAloneDlg.h"
#include "UpdateListCtrl.h"
#include "UpdateDownloader.h"
#include "VersioncheckParser.h"
#include "HyperLink.h"
#include "MenuButton.h"
#include "SciEdit.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog class, which checks if there are updated version of TortoiseSVN
 * available.
 */
class CCheckForUpdatesDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CCheckForUpdatesDlg)

public:
	CCheckForUpdatesDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CCheckForUpdatesDlg();

	enum { IDD = IDD_CHECKFORUPDATES };

protected:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedButtonUpdate();
	afx_msg void OnBnClickedDonotaskagain();
	afx_msg LRESULT OnDisplayStatus(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnEndDownload(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnFillChangelog(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnTaskbarBtnCreated(WPARAM, LPARAM);
	virtual void DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	virtual BOOL OnInitDialog() override;
	afx_msg void OnDestroy();
	afx_msg void OnSysColorChange();
	virtual void OnOK() override;
	virtual void OnCancel() override;

	DECLARE_MESSAGE_MAP()

private:
	static UINT CheckThreadEntry(LPVOID pVoid);
	UINT		CheckThread();

	BOOL		m_bThreadRunning;

public:
	BOOL		m_bShowInfo;
	BOOL		m_bForce;

private:
	BOOL		m_bVisible;
	CProgressCtrl	m_progress;
	CComPtr<ITaskbarList3>	m_pTaskbarList;
	CEvent		m_eventStop;
	CWinThread	*m_pDownloadThread;
	CString		m_sFilesURL;

	static UINT	DownloadThreadEntry(LPVOID pParam);
	UINT		DownloadThread();
	bool		Download(CString filename);
	CUpdateDownloader* m_updateDownloader;
	bool		VerifyUpdateFile(const CString& filename, const CString& filenameSignature, const CString& reportingFilename);

	CUpdateListCtrl	m_ctrlFiles;

	CString		m_sUpdateDownloadLink;			///< Where to send a user looking to download a update
	CString		m_sUpdateChangeLogLink;			///< Where to send a user looking to change log
	CHyperLink	m_link;
	CString		GetDownloadsDirectory();
	CMenuButton	m_ctrlUpdate;
	void		FillDownloads(CVersioncheckParser& versionfile);
	CSciEdit	m_cLogMessage;
	void		FillChangelog(CVersioncheckParser& versionfile, bool official);
	static CString GetWinINetError(DWORD err);
	CString		m_sErrors;
	CString		m_sNewVersionNumber;
};
