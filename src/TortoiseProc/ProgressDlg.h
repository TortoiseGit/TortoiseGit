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

#include "StandAloneDlg.h"
#include "git.h"
#include "MenuButton.h"
#include "Win7.h"

#define MSG_PROGRESSDLG_UPDATE_UI	(WM_USER+121)

// CProgressDlg dialog
#define MSG_PROGRESSDLG_START 0
#define MSG_PROGRESSDLG_RUN   50
#define MSG_PROGRESSDLG_END   110
#define MSG_PROGRESSDLG_FAILED 111

class CProgressDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CProgressDlg)
public:
	CProgressDlg(CWnd* pParent = NULL); // standard constructor
	virtual ~CProgressDlg();

private:
	virtual BOOL OnInitDialog();

	// Dialog Data
	enum { IDD = IDD_GITPROGRESS };

public:
	CString					m_Title;
	CString					m_GitCmd;
	CStringArray			m_PostCmdList;
	std::vector<CString>	m_GitCmdList;
	CString					m_PreText;		// optional text to show in log window before running command
	bool					m_bShowCommand;	// whether to display the command in the log window (default true)
	CString					m_LogFile;
	bool					m_bBufferAll;	// Buffer All to improve speed when there are many file add at commit
	bool					m_bAutoCloseOnSuccess;

	DWORD					m_GitStatus;
	CString					m_LogText;

private:
	CMenuButton				m_ctrlPostCmd;

	CProgressCtrl			m_Progress;

	CRichEditCtrl			m_Log;
	CAnimateCtrl			m_Animate;
	CStatic					m_CurrentWork;
	CWinThread*				m_pThread;
	volatile LONG			m_bThreadRunning;

	bool					m_bAbort;
	bool					m_bDone;
	bool					m_bAltAbortPress;
	DWORD					m_startTick;

	virtual void			DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	static UINT				ProgressThreadEntry(LPVOID pVoid);
	UINT					ProgressThread();

	CStringA				m_LogTextA;

	void					ParserCmdOutput(char ch);
	void					RemoveLastLine(CString &str);

	LRESULT					CProgressDlg::OnProgressUpdateUI(WPARAM wParam,LPARAM lParam);

	afx_msg LRESULT			OnTaskbarBtnCreated(WPARAM wParam, LPARAM lParam);
	CComPtr<ITaskbarList3>	m_pTaskbarList;

	void					OnCancel();
	afx_msg void			OnClose();
	void					InsertCRLF(); //Insert \r before \n
	void					KillProcessTree(DWORD dwProcessId, unsigned int depth = 0);

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

	static UINT	RunCmdList(CWnd *pWnd,std::vector<CString> &cmdlist,bool bShowCommand,CString *pfilename,bool *bAbort,CGitByteArray *pdata=NULL);

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
