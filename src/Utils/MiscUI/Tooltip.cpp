// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011 - Sven Strickroth <email@cs-ware.de>
// Copyright (C) 2008-2012, 2015 - TortoiseSVN

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
#include "Tooltip.h"


BEGIN_MESSAGE_MAP(CToolTips, CToolTipCtrl)
	ON_NOTIFY_REFLECT_EX( TTN_NEEDTEXT, &CToolTips::OnTtnNeedText )
END_MESSAGE_MAP()



BOOL CToolTips::OnTtnNeedText(NMHDR *pNMHDR, LRESULT *pResult)
{
	if (pNMHDR->code == TTN_NEEDTEXTW)
	{
		auto lpnmtdi = reinterpret_cast<LPNMTTDISPINFO>(pNMHDR);
		UINT_PTR nID = pNMHDR->idFrom;

		if (lpnmtdi->uFlags & TTF_IDISHWND)
		{
			// idFrom is actually the HWND of the tool
			nID = ::GetDlgCtrlID(reinterpret_cast<HWND>(nID));
		}

		auto iterFind = toolTextMap.find(static_cast<unsigned int>(nID));
		if (iterFind != toolTextMap.end())
		{
			lpnmtdi->lpszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(iterFind->second));
			lpnmtdi->hinst = AfxGetResourceHandle();
			*pResult = 0;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CToolTips::AddTool(CWnd* pWnd, UINT nIDText, LPCRECT lpRectTool /* = nullptr */, UINT_PTR nIDTool /* = 0 */)
{
	CString sTemp = LoadTooltip(nIDText);
	toolTextMap[::GetDlgCtrlID(pWnd->GetSafeHwnd())] = sTemp;
	return CToolTipCtrl::AddTool(pWnd, LPSTR_TEXTCALLBACK, lpRectTool, nIDTool);
}

BOOL CToolTips::AddTool(CWnd* pWnd, LPCTSTR lpszText /* = LPSTR_TEXTCALLBACK */, LPCRECT lpRectTool /* = nullptr */, UINT_PTR nIDTool /* = 0 */)
{
	if (lpszText != LPSTR_TEXTCALLBACK)
		toolTextMap[::GetDlgCtrlID(pWnd->GetSafeHwnd())] = CString(lpszText);
	return CToolTipCtrl::AddTool(pWnd, lpszText, lpRectTool, nIDTool);
}

void CToolTips::AddTool(int nIdWnd, UINT nIdText, LPCRECT lpRectTool /* = nullptr */, UINT_PTR nIDTool /* = 0 */)
{
	AddTool(static_cast<CDialog*>(m_pParentWnd)->GetDlgItem(nIdWnd), nIdText, lpRectTool, nIDTool);
}

void CToolTips::AddTool(int nIdWnd, CString sBalloonTipText, LPCRECT lpRectTool /* = nullptr */, UINT_PTR nIDTool /* = 0 */)
{
	AddTool(static_cast<CDialog*>(m_pParentWnd)->GetDlgItem(nIdWnd), sBalloonTipText, lpRectTool, nIDTool);
}

void CToolTips::DelTool( CWnd* pWnd, UINT_PTR nIDTool /* = 0 */)
{
	toolTextMap.erase(::GetDlgCtrlID(pWnd->GetSafeHwnd()));
	return CToolTipCtrl::DelTool(pWnd, nIDTool);
}

void CToolTips::DelTool( int nIdWnd, UINT_PTR nIDTool /* = 0 */)
{
	return DelTool(static_cast<CDialog*>(m_pParentWnd)->GetDlgItem(nIdWnd), nIDTool);
}

BOOL CToolTips::ShowBalloon(CWnd *pWnd, UINT nIDText, UINT nIDTitle, UINT icon /* = 0 */)
{
	CString sTemp = LoadTooltip(nIDText);

	const HWND hwndTT = CreateWindow
		(
		TOOLTIPS_CLASS,
		L"",
		TTS_NOPREFIX|TTS_BALLOON|TTS_ALWAYSTIP|TTS_CLOSE,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		pWnd->GetSafeHwnd(),
		nullptr,
		nullptr,
		nullptr
		);
	if (!hwndTT)
		return FALSE;

	TOOLINFO ti = { 0 };
	ti.cbSize = sizeof(ti);
	ti.uFlags = TTF_TRACK | TTF_IDISHWND | TTF_PARSELINKS;
	ti.hwnd = pWnd->GetSafeHwnd();
	ti.lpszText = sTemp.GetBuffer();
	::SendMessage(hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
	::SendMessage(hwndTT, TTM_SETTITLE, icon, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(CString(MAKEINTRESOURCE(nIDTitle)))));
	::SendMessage(hwndTT, TTM_SETMAXTIPWIDTH, 0, 800);

	// Position the tooltip below the control
	RECT rc;
	::GetWindowRect(pWnd->GetSafeHwnd(), &rc);
	::SendMessage(hwndTT, TTM_TRACKPOSITION, 0, MAKELONG(rc.left + 10, rc.bottom));

	// Show the tooltip
	::SendMessage(hwndTT, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&ti));

	return TRUE;
}

void CToolTips::ShowBalloon(int nIdWnd, UINT nIdText, UINT nIDTitle, UINT icon /* = 0 */)
{
	ShowBalloon(static_cast<CDialog*>(m_pParentWnd)->GetDlgItem(nIdWnd), nIdText, nIDTitle, icon);
}

CString CToolTips::LoadTooltip( UINT nIDText )
{
	CString sTemp;
	sTemp.LoadString(nIDText);
	// tooltips can't handle \t and single \n, only spaces and \r\n
	sTemp.Replace('\t', ' ');
	sTemp.Replace(L"\r\n", L"\n");
	sTemp.Replace(L"\n", L"\r\n");
	return sTemp;
}

void CToolTips::RelayEvent(LPMSG lpMsg, CWnd * dlgWnd)
{
	if (lpMsg->message == WM_NULL)
		return;

	if(dlgWnd && ((lpMsg->message == WM_MOUSEMOVE) || (lpMsg->message == WM_NCMOUSEMOVE)) && (lpMsg->hwnd == dlgWnd->m_hWnd))
	{
		// allow tooltips for disabled controls
		CRect  rect;
		POINT  pt;

		pt.x = LOWORD(lpMsg->lParam);
		pt.y = HIWORD(lpMsg->lParam);

		for (auto it = toolTextMap.cbegin(); it != toolTextMap.cend(); ++it)
		{
			CWnd * pWndCtrl = dlgWnd->GetDlgItem(it->first);
			if (pWndCtrl == nullptr)
				continue;
			pWndCtrl->GetWindowRect(&rect);
			if (lpMsg->message == WM_MOUSEMOVE)
				dlgWnd->ScreenToClient(&rect);

			if(rect.PtInRect(pt) )
			{
				// save the original parameters
				HWND origHwnd = lpMsg->hwnd;
				LPARAM origLParam = lpMsg->lParam;

				// translate and relay the mouse move message to
				// the tooltip control as if they were sent
				// by the disabled control
				lpMsg->hwnd = pWndCtrl->m_hWnd;

				if (lpMsg->message == WM_MOUSEMOVE)
					dlgWnd->ClientToScreen(&pt);
				pWndCtrl->ScreenToClient(&pt);
				lpMsg->lParam = MAKELPARAM(pt.x, pt.y);

				__super::RelayEvent(lpMsg);

				// restore the original parameters
				lpMsg->hwnd = origHwnd;
				lpMsg->lParam = origLParam;
				return;
			}
		}
	}

	// Let the ToolTip process this message.
	__super::RelayEvent(lpMsg);
}
