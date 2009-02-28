// TortoiseSVN - a Windows shell extension for easy version control

// Copyright (C) 2003-2006,2008 - TortoiseSVN

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

CSysProgressDlg::CSysProgressDlg() :
			m_pIDlg(NULL),
		    m_bValid(false),			//not valid by default
            m_isVisible(false),
		    m_dwDlgFlags(PROGDLG_NORMAL)
{
	EnsureValid();
}

CSysProgressDlg::~CSysProgressDlg()
{
    if (m_bValid)
    {
	    if (m_isVisible)			//still visible, so stop first before destroying
	        m_pIDlg->StopProgressDialog();

    	m_pIDlg->Release();
    }
}

bool CSysProgressDlg::EnsureValid()
{
	if (!m_bValid)
	{
		HRESULT hr;

		hr = CoCreateInstance (CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER,
			IID_IProgressDialog, (void**)&m_pIDlg);

		if (SUCCEEDED(hr))
			m_bValid = true;				//instance successfully created
	}
	return m_bValid;
}
void CSysProgressDlg::SetTitle(LPCTSTR szTitle)
{
    USES_CONVERSION;
    if (m_bValid)
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
	if (m_bValid)
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
	if (m_bValid)
	{
		m_pIDlg->SetCancelMsg(T2COLE(szMessage), NULL);
	}
}

void CSysProgressDlg::SetAnimation(HINSTANCE hinst, UINT uRsrcID)
{
	if (m_bValid)
	{
		m_pIDlg->SetAnimation(hinst, uRsrcID);
	}
}
#ifdef _MFC_VER
void CSysProgressDlg::SetAnimation(UINT uRsrcID)
{
	if (m_bValid)
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
HRESULT CSysProgressDlg::ShowModal (CWnd* pwndParent)
{
	EnsureValid();
	return ShowModal(pwndParent->GetSafeHwnd());
}

HRESULT CSysProgressDlg::ShowModeless(CWnd* pwndParent)
{
	EnsureValid();
	return ShowModeless(pwndParent->GetSafeHwnd());
}

void CSysProgressDlg::FormatPathLine ( DWORD dwLine, UINT idFormatText, ...)
{
	va_list args;
	va_start(args, idFormatText);

	CString sText;
	sText.FormatV(CString(MAKEINTRESOURCE(idFormatText)), args);
	SetLine(dwLine, sText, true);

	va_end(args);
}

void CSysProgressDlg::FormatPathLine ( DWORD dwLine, CString FormatText, ...)
{
	va_list args;
	va_start(args, FormatText);

	CString sText;
	sText.FormatV(CString(FormatText), args);
	SetLine(dwLine, sText, true);

	va_end(args);
}

void CSysProgressDlg::FormatNonPathLine(DWORD dwLine, UINT idFormatText, ...)
{
	va_list args;
	va_start(args, idFormatText);

	CString sText;
	sText.FormatV(CString(MAKEINTRESOURCE(idFormatText)), args);
	SetLine(dwLine, sText, false);

	va_end(args);
}

#endif
HRESULT CSysProgressDlg::ShowModal (HWND hWndParent)
{
	EnsureValid();
	HRESULT hr;
	if (m_bValid)
	{

		hr = m_pIDlg->StartProgressDialog(hWndParent,
			NULL,
			m_dwDlgFlags | PROGDLG_MODAL,
			NULL);

		if (SUCCEEDED(hr))
		{
			m_isVisible = true;
		}
		return hr;
	}
	return E_FAIL;
}

HRESULT CSysProgressDlg::ShowModeless(HWND hWndParent)
{
	EnsureValid();
	HRESULT hr = E_FAIL;

	if (m_bValid)
	{
		hr = m_pIDlg->StartProgressDialog(hWndParent, NULL, m_dwDlgFlags, NULL);

		if (SUCCEEDED(hr))
		{
			m_isVisible = true;

			// The progress window can be remarkably slow to display, particularly
			// if its parent is blocked.
			// This process finds the hwnd for the progress window and gives it a kick...
			IOleWindow *pOleWindow;
			HRESULT hr=m_pIDlg->QueryInterface(IID_IOleWindow,(LPVOID *)&pOleWindow);
			if(SUCCEEDED(hr))
			{
				HWND hDlgWnd;

				hr=pOleWindow->GetWindow(&hDlgWnd);
				if(SUCCEEDED(hr))
				{
					ShowWindow(hDlgWnd, SW_NORMAL);
				}
				pOleWindow->Release();
			}
		}
	}
	return hr;
}

void CSysProgressDlg::SetProgress(DWORD dwProgress, DWORD dwMax)
{
	if (m_bValid)
	{
		m_pIDlg->SetProgress(dwProgress, dwMax);
	}
}


void CSysProgressDlg::SetProgress64(ULONGLONG u64Progress, ULONGLONG u64ProgressMax)
{
	if (m_bValid)
	{
		m_pIDlg->SetProgress64(u64Progress, u64ProgressMax);
	}
}


bool CSysProgressDlg::HasUserCancelled()
{
	if (m_bValid)
	{
		return (0 != m_pIDlg->HasUserCancelled());
	}
	return FALSE;
}

void CSysProgressDlg::Stop()
{
    if ((m_isVisible)&&(m_bValid))
    {
        m_pIDlg->StopProgressDialog();
		//Sometimes the progress dialog sticks around after stopping it,
		//until the mouse pointer is moved over it or some other triggers.
		//This process finds the hwnd of the progress dialog and hides it
		//immediately.
		IOleWindow *pOleWindow;
		HRESULT hr=m_pIDlg->QueryInterface(IID_IOleWindow,(LPVOID *)&pOleWindow);
		if(SUCCEEDED(hr))
		{
			HWND hDlgWnd;

			hr=pOleWindow->GetWindow(&hDlgWnd);
			if(SUCCEEDED(hr))
			{
				ShowWindow(hDlgWnd, SW_HIDE);
			}
			pOleWindow->Release();
		}
        m_isVisible = false;
		m_pIDlg->Release();
		m_bValid = false;
    }
}

void CSysProgressDlg::ResetTimer()
{
	if (m_bValid)
	{
		m_pIDlg->Timer(PDTIMER_RESET, NULL);
	}
}
