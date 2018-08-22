// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2006 - Stefan Kueng

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
#include "TGitPath.h"

// CCacheDlg dialog
class CCacheDlg : public CDialog
{
// Construction
public:
	CCacheDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_CACHE_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedWatchtestbutton();

	DECLARE_MESSAGE_MAP()

	CString m_sRootPath;
	CStringArray m_filelist;
	HANDLE m_hPipe;
	OVERLAPPED m_Overlapped;
	HANDLE m_hEvent;
	CComCriticalSection m_critSec;
	static UINT TestThreadEntry(LPVOID pVoid);
	UINT TestThread();
	void ClosePipe();
	bool EnsurePipeOpen();
	bool GetStatusFromRemoteCache(const CTGitPath& Path, bool bRecursive);
	void RemoveFromCache(const CString& path);

	void TouchFile(const CString& path);
	void CopyRemoveCopy(const CString& path);

	static UINT WatchTestThreadEntry(LPVOID pVoid);
	UINT WatchTestThread();
};
