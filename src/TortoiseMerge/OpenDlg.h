// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006, 2009-2010 - TortoiseSVN

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
#include "FileDropEdit.h"
#include "StandAloneDlg.h"

// COpenDlg dialog

class COpenDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(COpenDlg)

public:
	COpenDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~COpenDlg();

// Dialog Data
	enum { IDD = IDD_OPENDLG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void OnBrowseForFile(CString& filepath, UINT nFileFilter = IDS_COMMONFILEFILTER);
	void GroupRadio(UINT nID);
	bool CheckAndEnableClipboardChecker();
	DECLARE_MESSAGE_MAP()
public:
	CString m_sBaseFile;
	CString m_sTheirFile;
	CString m_sYourFile;
	CString m_sUnifiedDiffFile;
	CString m_sPatchDirectory;

protected:
	CFileDropEdit m_cBaseFileEdit;
	CFileDropEdit m_cTheirFileEdit;
	CFileDropEdit m_cYourFileEdit;
	CFileDropEdit m_cDiffFileEdit;
	CFileDropEdit m_cDirEdit;

	afx_msg void OnBnClickedBasefilebrowse();
	afx_msg void OnBnClickedTheirfilebrowse();
	afx_msg void OnBnClickedYourfilebrowse();
	afx_msg void OnBnClickedHelp();
	afx_msg void OnBnClickedDifffilebrowse();
	afx_msg void OnBnClickedDirectorybrowse();
	afx_msg void OnBnClickedMergeradio();
	afx_msg void OnBnClickedApplyradio();

	afx_msg void OnChangeCbChain(HWND hWndRemove, HWND hWndAfter);
	afx_msg void OnDrawClipboard();
	afx_msg void OnDestroy();
	afx_msg void OnBnClickedPatchfromclipboard();

	virtual BOOL OnInitDialog();
	virtual void OnOK();
	void AutoCompleteOn(int controlId);

	BOOL	m_bFromClipboard;
	UINT	m_cFormat;
	HWND	m_nextViewer;
};
