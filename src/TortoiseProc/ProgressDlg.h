// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit

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
#include "GestureEnabledControl.h"

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

typedef std::function<void()> PostCmdAction;

class PostCmd
{
public:
	PostCmd(UINT icon, UINT msgId, PostCmdAction action)
	: icon(icon)
	, action(action)
	{
		label.LoadString(msgId);
	}

	PostCmd(UINT msgId, PostCmdAction action)
	: PostCmd(0, msgId, action)
	{
	}

	PostCmd(UINT icon, CString label, PostCmdAction action)
	: icon(icon)
	, action(action)
	, label(label)
	{
	}

	PostCmd(CString label, PostCmdAction action)
	: PostCmd(0, label, action)
	{
	}

	UINT			icon;
	CString			label;
	PostCmdAction	action;
};

typedef std::vector<PostCmd> PostCmdList;
typedef std::function<void(DWORD status, PostCmdList&)> PostCmdCallback;
typedef std::function<void(DWORD& exitCode, CString& extraMsg)> PostExecCallback;

class CProgressDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CProgressDlg)
public:
	CProgressDlg(CWnd* pParent = nullptr); // standard constructor
	virtual ~CProgressDlg();

private:
	virtual BOOL OnInitDialog() override;

	// Dialog Data
	enum { IDD = IDD_GITPROGRESS };

public:
	CString					m_GitCmd;
	PostCmdCallback			m_PostCmdCallback;
	std::vector<CString>	m_GitCmdList;
	PostExecCallback		m_PostExecCallback; // After executing command line, this callback can modify exit code / display extra message
	STRING_VECTOR			m_GitDirList;
	CString					m_PreText;		// optional text to show in log window before running command
	CString					m_PreFailText;	// optional fail text to show in log window
	bool					m_bShowCommand;	// whether to display the command in the log window (default true)
	CString					m_LogFile;
	bool					m_bBufferAll;	// Buffer All to improve speed when there are many file add at commit
	int						m_iBufferAllImmediateLines; // When Buffer All is enabled, we might not get important oputput of git at the beginning, so, ignore buffer all for the first 10 lines...
	GitProgressAutoClose	m_AutoClose;
	CGit *					m_Git;

	DWORD					m_GitStatus;
	CString					m_LogText;

	CString					GetLogText() const { CString text; m_Log.GetWindowText(text); return text; }

private:
	PostCmdList				m_PostCmdList;
	void					WriteLog() const;
	CMenuButton				m_ctrlPostCmd;

	CProgressCtrl			m_Progress;

	CGestureEnabledControlTmpl<CRichEditCtrl> m_Log;
	CAnimateCtrl			m_Animate;
	CStatic					m_CurrentWork;
	CWinThread*				m_pThread;

	volatile bool			m_bAbort;
	bool					m_bDone;
	ULONGLONG				m_startTick;

	virtual void			DoDataExchange(CDataExchange* pDX) override;    // DDX/DDV support
	static UINT				ProgressThreadEntry(LPVOID pVoid);
	UINT					ProgressThread();

	CStringA				m_LogTextA;

	void					ParserCmdOutput(char ch);
	static const int		s_iProgressLinesLimit;

	LRESULT					OnProgressUpdateUI(WPARAM wParam,LPARAM lParam);

	afx_msg LRESULT			OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	void					OnCancel();
	afx_msg void			OnClose();

	afx_msg void			OnEnscrollLog();
	afx_msg void			OnEnLinkLog(NMHDR* pNMHDR, LRESULT* pResult);

	CGitGuardedByteArray	m_Databuf;
	virtual CString Convert2UnionCode(char *buff, int size=-1)
	{
		CString str;
		CGit::StringAppend(&str, buff, CP_UTF8, size);
		return str;
	}

		int						m_BufStart;

	DECLARE_MESSAGE_MAP()

	//Share with Sync Dailog
	static int ParsePercentage(CString &log, int pos);

	static void	ClearESC(CString &str);

public:
	static void	ParserCmdOutput(CRichEditCtrl &log,CProgressCtrl &progressctrl,HWND m_hWnd,CComPtr<ITaskbarList3> m_pTaskbarList,
									CStringA& oneline, char ch, CWnd* CurrentWork = nullptr);

	/**
	 *@param dirlist if empty, the current directory of param git is used; otherwise each entry in param cmdlist uses the corresponding entry in param dirlist
	 */
	static UINT	RunCmdList(CWnd* pWnd, STRING_VECTOR& cmdlist, STRING_VECTOR& dirlist, bool bShowCommand, CString* pfilename, volatile bool* bAbort, CGitGuardedByteArray* pdata, CGit* git = &g_Git);

	static void KillProcessTree(DWORD dwProcessId, unsigned int depth = 0);

	static void InsertColorText(CRichEditCtrl &edit,CString text,COLORREF rgb);

private:
	afx_msg void OnBnClickedOk();
	afx_msg void OnBnClickedButton1();

	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	typedef struct {
		int id;
		int cnt;
		int wmid;
	} ACCELLERATOR;
	std::map<TCHAR, ACCELLERATOR>	m_accellerators;
	HACCEL							m_hAccel;
	virtual LRESULT DefWindowProc(UINT message, WPARAM wParam, LPARAM lParam) override;
};

class CCommitProgressDlg:public CProgressDlg
{
public:
	CCommitProgressDlg(CWnd* pParent = nullptr) : CProgressDlg(pParent)
	{
	}

	virtual CString Convert2UnionCode(char* buff, int size = -1) override;
};
