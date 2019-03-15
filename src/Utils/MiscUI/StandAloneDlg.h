// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2015-2016, 2019 - TortoiseGit
// Copyright (C) 2003-2015, 2018 - TortoiseSVN

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

#include "ResizableDialog.h"
#include "TaskbarUUID.h"
#include "Tooltip.h"
#include "CommonDialogFunctions.h"
#include "EditWordBreak.h"
#pragma comment(lib, "htmlhelp.lib")

#define DIALOG_BLOCKHORIZONTAL 1
#define DIALOG_BLOCKVERTICAL 2

/**
 * \ingroup TortoiseProc
 *
 * A template which can be used as the base-class of dialogs which form the main dialog
 * of a 'dialog-style application'
 * Just provides the boiler-plate code for dealing with application icons
 *
 * \remark Replace all references to CDialog or CResizableDialog in your dialog class with
 * either CResizableStandAloneDialog, CStandAloneDialog or CStateStandAloneDialog, as appropriate
 */
template <typename BaseType> class CStandAloneDialogTmpl : public BaseType, protected CommonDialogFunctions<BaseType>
{
protected:
	CStandAloneDialogTmpl(UINT nIDTemplate, CWnd* pParentWnd = nullptr) : BaseType(nIDTemplate, pParentWnd), CommonDialogFunctions(this)
	{
		m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	}
	virtual BOOL OnInitDialog() override
	{
		BaseType::OnInitDialog();

		// Set the icon for this dialog.  The framework does this automatically
		//  when the application's main window is not a dialog
		SetIcon(m_hIcon, TRUE);			// Set big icon
		SetIcon(m_hIcon, FALSE);		// Set small icon

		EnableToolTips();
		m_tooltips.Create(this);

		auto CustomBreak = static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\UseCustomWordBreak", 2));
		if (CustomBreak)
			SetUrlWordBreakProcToChildWindows(GetSafeHwnd(), CustomBreak == 2);

		return FALSE;
	}

	virtual BOOL PreTranslateMessage(MSG* pMsg) override
	{
		m_tooltips.RelayEvent(pMsg, this);
		if (pMsg->message == WM_KEYDOWN)
		{
			int nVirtKey = static_cast<int>(pMsg->wParam);

			if (nVirtKey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000 ) )
			{
				TCHAR buffer[129];
				::GetClassName(pMsg->hwnd, buffer, _countof(buffer) - 1);

				if (_wcsnicmp(buffer, L"EDIT", _countof(buffer) - 1) == 0)
				{
					::PostMessage(pMsg->hwnd,EM_SETSEL,0,-1);
					return TRUE;
				}
			}
		}
		return BaseType::PreTranslateMessage(pMsg);
	}
	afx_msg void OnPaint()
	{
		if (IsIconic())
		{
			CPaintDC dc(this); // device context for painting

			SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

			// Center icon in client rectangle
			int cxIcon = GetSystemMetrics(SM_CXICON);
			int cyIcon = GetSystemMetrics(SM_CYICON);
			CRect rect;
			GetClientRect(&rect);
			int x = (rect.Width() - cxIcon + 1) / 2;
			int y = (rect.Height() - cyIcon + 1) / 2;

			// Draw the icon
			dc.DrawIcon(x, y, m_hIcon);
		}
		else
			BaseType::OnPaint();
	}
	/**
	 * Wrapper around the CWnd::EnableWindow() method, but
	 * makes sure that a control that has the focus is not disabled
	 * before the focus is passed on to the next control.
	 */
	BOOL DialogEnableWindow(UINT nID, BOOL bEnable)
	{
		CWnd * pwndDlgItem = GetDlgItem(nID);
		if (!pwndDlgItem)
			return FALSE;
		if (bEnable)
			return pwndDlgItem->EnableWindow(bEnable);
		if (GetFocus() == pwndDlgItem)
		{
			SendMessage(WM_NEXTDLGCTL, 0, FALSE);
		}
		return pwndDlgItem->EnableWindow(bEnable);
	}

	/**
	 * Refreshes the cursor by forcing a WM_SETCURSOR message.
	 */
	void RefreshCursor()
	{
		POINT pt;
		GetCursorPos(&pt);
		SetCursorPos(pt.x, pt.y);
	}

protected:
	CToolTips	m_tooltips;

	DECLARE_MESSAGE_MAP()

private:
	HCURSOR OnQueryDragIcon()
	{
		return static_cast<HCURSOR>(m_hIcon);
	}
protected:
	virtual void HtmlHelp(DWORD_PTR dwData, UINT nCmd = 0x000F) override
	{
		CWinApp* pApp = AfxGetApp();
		ASSERT_VALID(pApp);
		ASSERT(pApp->m_pszHelpFilePath);
		// to call HtmlHelp the m_fUseHtmlHelp must be set in
		// the application's constructor
		ASSERT(pApp->m_eHelpType == afxHTMLHelp);

		CWaitCursor wait;

		PrepareForHelp();
		// run the HTML Help engine
		if (!::HtmlHelp(m_hWnd, pApp->m_pszHelpFilePath, nCmd, dwData))
		{
			AfxMessageBox(AFX_IDP_FAILED_TO_LAUNCH_HELP);
		}
	}

	afx_msg LRESULT OnTaskbarButtonCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		SetUUIDOverlayIcon(m_hWnd);
		return 0;
	}

	HICON m_hIcon;
};

class CStateDialog : public CDialog, public CResizableWndState
{
public:
	CStateDialog()
	: CDialog()
	, m_bEnableSaveRestore(false)
	, m_bRectOnly(false)
	{}
	CStateDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr)
	: CDialog(nIDTemplate, pParentWnd)
	, m_bEnableSaveRestore(false)
	, m_bRectOnly(false)
	{}
	CStateDialog(LPCTSTR lpszTemplateName, CWnd* pParentWnd = nullptr)
	: CDialog(lpszTemplateName, pParentWnd)
	, m_bEnableSaveRestore(false)
	, m_bRectOnly(false)
	{}
	virtual ~CStateDialog() {};

private:
	// flags
	bool m_bEnableSaveRestore;
	bool m_bRectOnly;

	// internal status
	CString m_sSection;			// section name (identifies a parent window)

protected:
	// overloaded method, but since this dialog class is for non-resizable dialogs,
	// the bHorzResize and bVertResize params are ignored and passed as false
	// to the base method.
	void EnableSaveRestore(LPCTSTR pszSection, bool bRectOnly = FALSE, BOOL bHorzResize = TRUE, BOOL bVertResize = TRUE)
	{
		UNREFERENCED_PARAMETER(bHorzResize);
		UNREFERENCED_PARAMETER(bVertResize);
		m_sSection = pszSection;

		m_bEnableSaveRestore = true;
		m_bRectOnly = bRectOnly;

		// restore immediately
		LoadWindowRect(pszSection, bRectOnly, false, false);
	};

	virtual CWnd* GetResizableWnd() const override
	{
		// make the layout know its parent window
		return CWnd::FromHandle(m_hWnd);
	};

	afx_msg void OnDestroy()
	{
		if (m_bEnableSaveRestore)
			SaveWindowRect(m_sSection, m_bRectOnly);
		CDialog::OnDestroy();
	};

	DECLARE_MESSAGE_MAP()
};

class CResizableStandAloneDialog : public CStandAloneDialogTmpl<CResizableDialog>
{
public:
	CResizableStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);

private:
	DECLARE_DYNAMIC(CResizableStandAloneDialog)

protected:
	virtual BOOL	OnInitDialog() override;
	afx_msg void	OnSizing(UINT fwSide, LPRECT pRect);
	afx_msg void	OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void	OnNcMButtonUp(UINT nHitTest, CPoint point);
	afx_msg void	OnNcRButtonUp(UINT nHitTest, CPoint point);
	afx_msg LRESULT OnNcHitTest(CPoint point);

	DECLARE_MESSAGE_MAP()

protected:
	int			m_nResizeBlock;
	long		m_width;
	long		m_height;

	void BlockResize(int block)
	{
		m_nResizeBlock = block;
	}

	void EnableSaveRestore(LPCTSTR pszSection, bool bRectOnly = FALSE)
	{
		// call the base method with the bHorzResize and bVertResize parameters
		// figured out from the resize block flags.
		__super::EnableSaveRestore(pszSection, bRectOnly, (m_nResizeBlock & DIALOG_BLOCKHORIZONTAL) == 0, (m_nResizeBlock & DIALOG_BLOCKVERTICAL) == 0);
	};

private:
	bool		m_bVertical;
	bool		m_bHorizontal;
	CRect		m_rcOrgWindowRect;
};

class CStandAloneDialog : public CStandAloneDialogTmpl<CDialog>
{
public:
	CStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
protected:
	DECLARE_MESSAGE_MAP()
private:
	DECLARE_DYNAMIC(CStandAloneDialog)
};

class CStateStandAloneDialog : public CStandAloneDialogTmpl<CStateDialog>
{
public:
	CStateStandAloneDialog(UINT nIDTemplate, CWnd* pParentWnd = nullptr);
protected:
	DECLARE_MESSAGE_MAP()
private:
	DECLARE_DYNAMIC(CStateStandAloneDialog)
};

const UINT TaskBarButtonCreated = RegisterWindowMessage(L"TaskbarButtonCreated");

BEGIN_TEMPLATE_MESSAGE_MAP(CStandAloneDialogTmpl, BaseType, BaseType)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_REGISTERED_MESSAGE(TaskBarButtonCreated, OnTaskbarButtonCreated)
END_MESSAGE_MAP()

