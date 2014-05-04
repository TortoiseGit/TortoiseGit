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

#include "StandAloneDlg.h"
#include "Git.h"
#include "MenuButton.h"
#include <functional>

#define MSG_PROGRESSDLG_UPDATE_UI	(WM_USER+121)

// CProgressDlg dialog
#define MSG_PROGRESSDLG_START 0
#define MSG_PROGRESSDLG_RUN   50
#define MSG_PROGRESSDLG_END   110
#define MSG_PROGRESSDLG_FAILED 111

typedef enum {
	AUTOCLOSE_NO,
	AUTOCLOSE_IF_NO_OPTIONS,
	AUTOCLOSE_IF_NO_ERRORS,
} GitProgressAutoClose;

class CProgressDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CProgressDlg)
public:
	typedef std::function<void()> PostCmdCallback;

	CProgressDlg(CWnd* pParent = NULL); // standard constructor
	virtual ~CProgressDlg();

private:
	virtual BOOL OnInitDialog();

	// Dialog Data
	enum { IDD = IDD_GITPROGRESS };

public:
	CString					m_GitCmd;
	CStringArray			m_PostCmdList;
	CStringArray			m_PostFailCmdList;
	PostCmdCallback			m_PostCmdCallback;
	std::vector<CString>	m_GitCmdList;
	STRING_VECTOR			m_GitDirList;
	CString					m_PreText;		// optional text to show in log window before running command
	bool					m_bShowCommand;	// whether to display the command in the log window (default true)
	CString					m_LogFile;
	bool					m_bBufferAll;	// Buffer All to improve speed when there are many file add at commit
	GitProgressAutoClose	m_AutoClose;
	CGit *					m_Git;

	DWORD					m_GitStatus;
	CString					m_LogText;

	CString					GetLogText() const { CString text; m_Log.GetWindowText(text); return text; }

private:
	void					WriteLog() const;
	CMenuButton				m_ctrlPostCmd;

	CProgressCtrl			m_Progress;

	CRichEditCtrl			m_Log;
	CAnimateCtrl			m_Animate;
	CStatic					m_CurrentWork;
	CWinThread*				m_pThread;

	bool					m_bAbort;
	bool					m_bDone;
	DWORD					m_startTick;

	virtual void			DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	static UINT				ProgressThreadEntry(LPVOID pVoid);
	UINT					ProgressThread();

	CStringA				m_LogTextA;

	void					ParserCmdOutput(char ch);
	void					RemoveLastLine(CString &str);

	LRESULT					OnProgressUpdateUI(WPARAM wParam,LPARAM lParam);

	afx_msg LRESULT			OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	void					OnCancel();
	afx_msg void			OnClose();

	CGitByteArray			m_Databuf;
	virtual CString Convert2UnionCode(char *buff, int size=-1)
	{
		CString str;
		g_Git.StringAppend(&str, (BYTE*)buff, CP_UTF8, size);
		return str;
	}

		int						m_BufStart;

	DECLARE_MESSAGE_MAP()

	//Share with Sync Dailog
	static int	FindPercentage(CString &log);

	static void	ClearESC(CString &str);

public:
	static void	ParserCmdOutput(CRichEditCtrl &log,CProgressCtrl &progressctrl,HWND m_hWnd,CComPtr<ITaskbarList3> m_pTaskbarList,
									CStringA &oneline, char ch,CWnd *CurrentWork=NULL);

	/**
	 *@param dirlist if empty, the current directory of param git is used; otherwise each entry in param cmdlist uses the corresponding entry in param dirlist
	 */
	static UINT	RunCmdList(CWnd *pWnd, STRING_VECTOR &cmdlist, STRING_VECTOR &dirlist, bool bShowCommand, CString *pfilename, bool *bAbort, CGitByteArray *pdata, CGit *git = &g_Git);

	static void KillProcessTree(DWORD dwProcessId, unsigned int depth = 0);

	static void InsertColorText(CRichEditCtrl &edit,CString text,COLORREF rgb);

private:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();

	virtual BOOL PreTranslateMessage(MSG* pMsg);
};

class CCommitProgressDlg:public CProgressDlg
{
public:
	CCommitProgressDlg(CWnd* pParent = NULL):CProgressDlg(pParent)
	{
	}

	virtual CString Convert2UnionCode(char *buff, int size=-1);
};
