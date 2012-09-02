// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseGit
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
#include "HyperLink.h"
#include "MenuButton.h"
#include "SciEdit.h"

/**
 * \ingroup TortoiseProc
 * Helper dialog class, which checks if there are updated version of TortoiseSVN
 * available.
 */
class CCheckForUpdatesDlg : public CStandAloneDialog
{
	DECLARE_DYNAMIC(CCheckForUpdatesDlg)

public:
	CCheckForUpdatesDlg(CWnd* pParent = NULL);   // standard constructor
	virtual ~CCheckForUpdatesDlg();

	enum { IDD = IDD_CHECKFORUPDATES };

	struct DOWNLOADSTATUS
	{
		ULONG ulProgress;
		ULONG ulProgressMax;
	};

protected:
	afx_msg void OnStnClickedCheckresult();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnBnClickedButtonUpdate();
	afx_msg LRESULT OnDisplayStatus(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnEndDownload(WPARAM, LPARAM lParam);
	afx_msg LRESULT OnFillChangelog(WPARAM, LPARAM lParam);
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

	DECLARE_MESSAGE_MAP()

private:
	static UINT CheckThreadEntry(LPVOID pVoid);
	UINT		CheckThread();

	BOOL		m_bThreadRunning;

public:
	BOOL		m_bShowInfo;

private:
	BOOL		m_bVisible;
	CProgressCtrl	m_progress;
	CEvent		m_eventStop;
	CWinThread	*m_pDownloadThread;
	CString		m_sFilesURL;
	std::vector<CString> m_fileNames;

	static UINT	DownloadThreadEntry(LPVOID pParam);
	UINT		DownloadThread();
	bool		Download(CString filename);

	CUpdateListCtrl	m_ctrlFiles;

	CString		m_sUpdateDownloadLink;			///< Where to send a user looking to download a update
	CString		m_sUpdateChangeLogLink;			///< Where to send a user looking to change log
	CHyperLink	m_link;
	CString		GetDownloadsDirectory();
	CMenuButton	m_ctrlUpdate;
	BOOL		VerifySignature(CString fileName);
	void		FillDownloads(CStdioFile &file, CString version);
	CSciEdit	m_cLogMessage;
	void		FillChangelog(CStdioFile &file);
};

class CBSCallbackImpl : public IBindStatusCallback
{
public:
	CBSCallbackImpl(HWND hWnd, HANDLE hEventStop);

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, void **ppvObject);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	// IBindStatusCallback methods
	STDMETHOD(OnStartBinding)(DWORD, IBinding *);
	STDMETHOD(GetPriority)(LONG *);
	STDMETHOD(OnLowResource)(DWORD);
	STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText);
	STDMETHOD(OnStopBinding)(HRESULT, LPCWSTR);
	STDMETHOD(GetBindInfo)(DWORD *, BINDINFO *);
	STDMETHOD(OnDataAvailable)(DWORD, DWORD, FORMATETC *, STGMEDIUM *);
	STDMETHOD(OnObjectAvailable)(REFIID, IUnknown *);

protected:
	ULONG m_ulObjRefCount;

private:
	HWND m_hWnd;
	HANDLE m_hEventStop;
};
