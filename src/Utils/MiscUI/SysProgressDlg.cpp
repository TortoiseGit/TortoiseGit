// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2011 - TortoiseGit
// Copyright (C) 2003-2006,2008-2011 - TortoiseSVN

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
#include "stdafx.h"
#include "SysProgressDlg.h"

CSysProgressDlg::CSysProgressDlg()
	: m_pIDlg(NULL)
	, m_isVisible(false)
	, m_dwDlgFlags(PROGDLG_NORMAL)
	, m_hWndProgDlg(NULL)
{
	EnsureValid();
}

CSysProgressDlg::~CSysProgressDlg()
{
	if (IsValid())
	{
		if (m_isVisible)			//still visible, so stop first before destroying
			m_pIDlg->StopProgressDialog();

		m_pIDlg.Release();
		m_hWndProgDlg = NULL;
	}
}

bool CSysProgressDlg::EnsureValid()
{
	if(IsValid())
		return true;

	HRESULT hr = m_pIDlg.CoCreateInstance (CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER);
	return (SUCCEEDED(hr));
}

void CSysProgressDlg::SetTitle(LPCTSTR szTitle)
{
	USES_CONVERSION;
	if (IsValid())
	{
		m_pIDlg->SetTitle(T2COLE(szTitle));
	}
}
void CSysProgressDlg::SetTitle ( UINT idTitle)
{
	SetTitle(CString(MAKEINTRESOURCE(idTitle)));
}

void CSysProgressDlg::SetLine(DWORD dwLine, LPCTSTR szText, bool bCompactPath /* = false */)
{
	USES_CONVERSION;
	if (IsValid())
	{
		m_pIDlg->SetLine(dwLine, T2COLE(szText), bCompactPath, NULL);
	}
}

#ifdef _MFC_VER
void CSysProgressDlg::SetCancelMsg ( UINT idMessage )
{
	SetCancelMsg(CString(MAKEINTRESOURCE(idMessage)));
}
#endif // _MFC_VER

void CSysProgressDlg::SetCancelMsg(LPCTSTR szMessage)
{
	USES_CONVERSION;
	if (IsValid())
	{
		m_pIDlg->SetCancelMsg(T2COLE(szMessage), NULL);
	}
}

void CSysProgressDlg::SetAnimation(HINSTANCE hinst, UINT uRsrcID)
{
	if (IsValid())
	{
		m_pIDlg->SetAnimation(hinst, uRsrcID);
	}
}
#ifdef _MFC_VER
void CSysProgressDlg::SetAnimation(UINT uRsrcID)
{
	if (IsValid())
	{
		m_pIDlg->SetAnimation(AfxGetResourceHandle(), uRsrcID);
	}
}
#endif
void CSysProgressDlg::SetTime(bool bTime /* = true */)
{
	m_dwDlgFlags &= ~(PROGDLG_NOTIME | PROGDLG_AUTOTIME);

	if (bTime)
		m_dwDlgFlags |= PROGDLG_AUTOTIME;
	else
		m_dwDlgFlags |= PROGDLG_NOTIME;
}

void CSysProgressDlg::SetShowProgressBar(bool bShow /* = true */)
{
	if (bShow)
		m_dwDlgFlags &= ~PROGDLG_NOPROGRESSBAR;
	else
		m_dwDlgFlags |= PROGDLG_NOPROGRESSBAR;
}
#ifdef _MFC_VER
HRESULT CSysProgressDlg::ShowModal (CWnd* pwndParent, BOOL immediately /* = true */)
{
	EnsureValid();
	return ShowModal(pwndParent->GetSafeHwnd(), immediately);
}

HRESULT CSysProgressDlg::ShowModeless(CWnd* pwndParent, BOOL immediately)
{
	EnsureValid();
	return ShowModeless(pwndParent->GetSafeHwnd(), immediately);
}

void CSysProgressDlg::FormatPathLine ( DWORD dwLine, UINT idFormatText, ...)
{
	va_list args;
	va_start(args, idFormatText);

	CString sText;
	sText.FormatMessageV(CString(MAKEINTRESOURCE(idFormatText)), &args);
	SetLine(dwLine, sText, true);

	va_end(args);
}

void CSysProgressDlg::FormatPathLine ( DWORD dwLine, CString FormatText, ...)
{
	va_list args;
	va_start(args, FormatText);

	CString sText;
	sText.FormatMessageV(FormatText, &args);
	SetLine(dwLine, sText, true);

	va_end(args);
}

void CSysProgressDlg::FormatNonPathLine(DWORD dwLine, UINT idFormatText, ...)
{
	va_list args;
	va_start(args, idFormatText);

	CString sText;
	sText.FormatMessageV(CString(MAKEINTRESOURCE(idFormatText)), &args);
	SetLine(dwLine, sText, false);

	va_end(args);
}

#endif
HRESULT CSysProgressDlg::ShowModal(HWND hWndParent, BOOL immediately /* = true */)
{
	EnsureValid();
	m_hWndProgDlg = NULL;
	if (!IsValid())
		return E_FAIL;
	m_hWndParent = hWndParent;
	HRESULT hr = m_pIDlg->StartProgressDialog(hWndParent, NULL, m_dwDlgFlags | PROGDLG_MODAL, NULL);
	if(FAILED(hr))
		return hr;

	ATL::CComPtr<IOleWindow> pOleWindow;
	HRESULT hr2 = m_pIDlg.QueryInterface(&pOleWindow);
	if(SUCCEEDED(hr2))
	{
		hr2 = pOleWindow->GetWindow(&m_hWndProgDlg);
		if(SUCCEEDED(hr2))
		{
			// StartProgressDialog creates a new thread to host the progress window.
			// When the window receives WM_DESTROY message StopProgressDialog() wrongly
			// attempts to re-enable the parent in the calling thread (our thread),
			// after the progress window is destroyed and the progress thread has died.
			// When the progress window dies, the system tries to assign a new foreground window.
			// It cannot assign to hwndParent because StartProgressDialog (w/PROGDLG_MODAL) disabled the parent window.
			// So the system hands the foreground activation to the next process that wants it in the
			// system foreground queue. Thus we lose our right to recapture the foreground window.
			// The way to fix this bug is to insert a call to EnableWindow(hWndParent) in the WM_DESTROY
			// handler for the progress window in the progress thread.

			// To do that, we Subclass the progress dialog
			// Since the window and thread created by the progress dialog object live on a few
			// milliseconds after calling Stop() and Release(), we must not store anything
			// in member variables of this class but must only store everything in the window
			// itself: thus we use SetProp()/GetProp() to store the data.
			if (!m_isVisible)
			{
				m_OrigProc = (WNDPROC) SetWindowLongPtr(m_hWndProgDlg, GWLP_WNDPROC, (LONG_PTR) fnSubclass);
				SetProp(m_hWndProgDlg, L"ParentWindow", m_hWndParent);
				SetProp(m_hWndProgDlg, L"OrigProc", m_OrigProc);
			}
			if(immediately)
				ShowWindow(m_hWndProgDlg, SW_SHOW);
		}
	}

	m_isVisible = true;
	return hr;
}

HRESULT CSysProgressDlg::ShowModeless(HWND hWndParent, BOOL immediately)
{
	EnsureValid();
	m_hWndProgDlg = NULL;
	if (!IsValid())
		return E_FAIL;
	m_hWndParent = hWndParent;
	HRESULT hr = m_pIDlg->StartProgressDialog(hWndParent, NULL, m_dwDlgFlags, NULL);
	if(FAILED(hr))
		return hr;

	ATL::CComPtr<IOleWindow> pOleWindow;
	HRESULT hr2 = m_pIDlg.QueryInterface(&pOleWindow);
	if(SUCCEEDED(hr2))
	{
		hr2 = pOleWindow->GetWindow(&m_hWndProgDlg);
		if(SUCCEEDED(hr2))
		{
			// see comment in ShowModal() for why we subclass the window
			if (!m_isVisible)
			{
				m_OrigProc = (WNDPROC) SetWindowLongPtr(m_hWndProgDlg, GWLP_WNDPROC, (LONG_PTR) fnSubclass);
				SetProp(m_hWndProgDlg, L"ParentWindow", m_hWndParent);
				SetProp(m_hWndProgDlg, L"OrigProc", m_OrigProc);
			}
			if (immediately)
				ShowWindow(m_hWndProgDlg, SW_SHOW);
		}
	}
	m_isVisible = true;
	return hr;
}

void CSysProgressDlg::SetProgress(DWORD dwProgress, DWORD dwMax)
{
	if (IsValid())
	{
		m_pIDlg->SetProgress(dwProgress, dwMax);
	}
}

void CSysProgressDlg::SetProgress64(ULONGLONG u64Progress, ULONGLONG u64ProgressMax)
{
	if (IsValid())
	{
		m_pIDlg->SetProgress64(u64Progress, u64ProgressMax);
	}
}

bool CSysProgressDlg::HasUserCancelled()
{
	if (!IsValid())
		return false;

	return (0 != m_pIDlg->HasUserCancelled());
}

void CSysProgressDlg::Stop()
{
	if ((m_isVisible)&&(IsValid()))
	{
		m_pIDlg->StopProgressDialog();
		// Sometimes the progress dialog sticks around after stopping it,
		// until the mouse pointer is moved over it or some other triggers.
		// We hide the window here immediately.
		if (m_hWndProgDlg)
		{
			ShowWindow(m_hWndProgDlg, SW_HIDE);
		}
		m_isVisible = false;
		m_pIDlg.Release();

		m_hWndProgDlg = NULL;
	}
}

void CSysProgressDlg::ResetTimer()
{
	if (IsValid())
	{
		m_pIDlg->Timer(PDTIMER_RESET, NULL);
	}
}

LRESULT CSysProgressDlg::fnSubclass(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	LONG_PTR origproc = (LONG_PTR)GetProp(hwnd, L"OrigProc");
	if (uMsg == WM_DESTROY)
	{
		HWND hParent = (HWND)GetProp(hwnd, L"ParentWindow");
		EnableWindow(hParent, TRUE);
		SetFocus(hParent);
		SetWindowLongPtr (hwnd, GWLP_WNDPROC, origproc);
		RemoveProp(hwnd, L"ParentWindow");
		RemoveProp(hwnd, L"OrigProc");
	}
	return CallWindowProc ((WNDPROC)origproc, hwnd, uMsg, wParam, lParam);
}
