// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN

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
		LPNMTTDISPINFO lpnmtdi = (LPNMTTDISPINFO)pNMHDR;
		UINT_PTR nID = pNMHDR->idFrom;

		if (lpnmtdi->uFlags & TTF_IDISHWND)
		{
			// idFrom is actually the HWND of the tool 
			nID = ::GetDlgCtrlID((HWND)nID);
		}
		if (toolTextMap.find(nID) != toolTextMap.end())
		{
			lpnmtdi->lpszText = (LPTSTR)(LPCTSTR)(CString)toolTextMap[nID];
			lpnmtdi->hinst = AfxGetResourceHandle();
			*pResult = 0;
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CToolTips::AddTool(CWnd* pWnd, UINT nIDText, LPCRECT lpRectTool /* = NULL */, UINT_PTR nIDTool /* = 0 */)
{
	CString sTemp;
	sTemp.LoadString(nIDText);
	// tooltips can't handle \t and single \n, only spaces and \r\n
	sTemp.Replace('\t', ' ');
	sTemp.Replace(_T("\r\n"), _T("\n"));
	sTemp.Replace(_T("\n"), _T("\r\n"));
	toolTextMap[::GetDlgCtrlID(pWnd->GetSafeHwnd())] = sTemp;
	return CToolTipCtrl::AddTool(pWnd, LPSTR_TEXTCALLBACK, lpRectTool, nIDTool);
}

BOOL CToolTips::AddTool(CWnd* pWnd, LPCTSTR lpszText /* = LPSTR_TEXTCALLBACK */, LPCRECT lpRectTool /* = NULL */, UINT_PTR nIDTool /* = 0 */)
{
	if (lpszText != LPSTR_TEXTCALLBACK)
		toolTextMap[::GetDlgCtrlID(pWnd->GetSafeHwnd())] = CString(lpszText);
	return CToolTipCtrl::AddTool(pWnd, lpszText, lpRectTool, nIDTool);
}

void CToolTips::AddTool(int nIdWnd, UINT nIdText, LPCRECT lpRectTool /* = NULL */, UINT_PTR nIDTool /* = 0 */)
{
	AddTool(((CDialog*)m_pParentWnd)->GetDlgItem(nIdWnd), nIdText, lpRectTool, nIDTool);
}

void CToolTips::AddTool(int nIdWnd, CString sBalloonTipText, LPCRECT lpRectTool /* = NULL */, UINT_PTR nIDTool /* = 0 */)
{
	AddTool(((CDialog*)m_pParentWnd)->GetDlgItem(nIdWnd), sBalloonTipText, lpRectTool, nIDTool);
}


