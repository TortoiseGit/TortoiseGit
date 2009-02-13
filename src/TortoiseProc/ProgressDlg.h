#pragma once

#include "StandAloneDlg.h"

// CProgressDlg dialog

class CProgressDlg : public CResizableStandAloneDialog
{
	DECLARE_DYNAMIC(CProgressDlg)

public:
	CProgressDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CProgressDlg();
	virtual BOOL OnInitDialog();
// Dialog Data
	enum { IDD = IDD_GITPROGRESS };
	CString m_GitCmd;
	CString m_LogFile;

	CProgressCtrl m_Progress;
	
	CEdit		  m_Log;
	CAnimateCtrl  m_Animate;
	CStatic		  m_CurrentWork;
	CWinThread*				m_pThread;	
	volatile LONG			m_bThreadRunning;
	DWORD			  m_GitStatus;
	BOOL		  m_bShowCommand;	// whether to display the command in the log window (default true)
	CString		  m_PreText;		// optional text to show in log window before running command
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	static UINT ProgressThreadEntry(LPVOID pVoid);
	UINT		ProgressThread();

	void		ParserCmdOutput(TCHAR ch);
	int			FindPercentage(CString &log);
	void        RemoveLastLine(CString &str);

	CString		m_LogText;
	DECLARE_MESSAGE_MAP()
};
