// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "HistoryCombo.h"
#include "../registry.h"

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
#include "../SysImageList.h"
#endif

#define MAX_HISTORY_ITEMS 25

CHistoryCombo::CHistoryCombo(BOOL bAllowSortStyle /*=FALSE*/ )
{
	m_nMaxHistoryItems = MAX_HISTORY_ITEMS;
	m_bAllowSortStyle = bAllowSortStyle;
	m_bURLHistory = FALSE;
	m_bPathHistory = FALSE;
	m_hWndToolTip = NULL;
	m_ttShown = FALSE;
	m_bDyn = FALSE;
	m_bWantReturn = FALSE;
}

CHistoryCombo::~CHistoryCombo()
{
}

BOOL CHistoryCombo::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!m_bAllowSortStyle)  //turn off CBS_SORT style
		cs.style &= ~CBS_SORT;
	cs.style |= CBS_AUTOHSCROLL;
	m_bDyn = TRUE;
	return CComboBoxEx::PreCreateWindow(cs);
}

BOOL CHistoryCombo::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN)
	{
		bool bShift = !!(GetKeyState(VK_SHIFT) & 0x8000);
		int nVirtKey = (int) pMsg->wParam;
		
		if (nVirtKey == VK_RETURN)
			return OnReturnKeyPressed();
		else if (nVirtKey == VK_DELETE && bShift && GetDroppedState() )
		{
			RemoveSelectedItem();
			return TRUE;
		}

		if (nVirtKey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000 ) )
		{
			GetEditCtrl()->SetSel(0, -1);
			return TRUE;
		}
	}
	else if (pMsg->message == WM_MOUSEMOVE && this->m_bDyn ) 
	{
		if ((pMsg->wParam & MK_LBUTTON) == 0)
		{
			CPoint pt;
			pt.x = LOWORD(pMsg->lParam);
			pt.y = HIWORD(pMsg->lParam);
			OnMouseMove((UINT)pMsg->wParam, pt);
			return TRUE;
		}
	}
	else if ((pMsg->message == WM_MOUSEWHEEL || pMsg->message == WM_MOUSEHWHEEL) && !GetDroppedState())
	{
		return TRUE;
	}

	return CComboBoxEx::PreTranslateMessage(pMsg);
}

BEGIN_MESSAGE_MAP(CHistoryCombo, CComboBoxEx)
	ON_WM_MOUSEMOVE()
	ON_WM_TIMER()
	ON_WM_CREATE()
END_MESSAGE_MAP()

int CHistoryCombo::AddString(CString str, INT_PTR pos,BOOL isSel)
{
	if (str.IsEmpty())
		return -1;
	
	COMBOBOXEXITEM cbei;
	SecureZeroMemory(&cbei, sizeof cbei);
	cbei.mask = CBEIF_TEXT;

	if (pos < 0)
        cbei.iItem = GetCount();
	else
		cbei.iItem = pos;

	str.Trim(_T(" "));
	CString combostring = str;
	combostring.Replace('\r', ' ');
	combostring.Replace('\n', ' ');
	str=combostring=combostring.Trim();
	cbei.pszText = const_cast<LPTSTR>(combostring.GetString());

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
	if (m_bURLHistory)
	{
		cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(str);
		if (cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex())
		{
			if (str.Left(5) == _T("http:"))
				cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(_T(".html"));
			else if (str.Left(6) == _T("https:"))
				cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(_T(".shtml"));
			else if (str.Left(5) == _T("file:"))
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
			else if (str.Left(4) == _T("git:"))
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
			else if (str.Left(4) == _T("ssh:"))
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
			else
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
		}
		cbei.iSelectedImage = cbei.iImage;
		cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	}
	if (m_bPathHistory)
	{
		cbei.iImage = SYS_IMAGE_LIST().GetFileIconIndex(str);
		if (cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex())
		{
			cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
		}
		cbei.iSelectedImage = cbei.iImage;
		cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	}
#endif

	//search the Combo for another string like this
	//and do not insert if found
	int nIndex = FindStringExact(-1, combostring);
	if (nIndex != -1)
	{
		if (nIndex > cbei.iItem)
		{
			DeleteItem(nIndex);
			m_arEntries.RemoveAt(nIndex);
		}
		else
		{
			if(isSel)
				SetCurSel(nIndex);
			return nIndex;
		}
	}

	int nRet = InsertItem(&cbei);
	if (nRet >= 0)
		m_arEntries.InsertAt(nRet, str);

	//truncate list to m_nMaxHistoryItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
	{
		DeleteItem(m_nMaxHistoryItems);
		m_arEntries.RemoveAt(m_nMaxHistoryItems);
	}

	if(isSel)
		SetCurSel(nRet);
	return nRet;
}

CString CHistoryCombo::LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix)
{
	if (lpszSection == NULL || lpszKeyPrefix == NULL || *lpszSection == '\0')
		return _T("");

	m_sSection = lpszSection;
	m_sKeyPrefix = lpszKeyPrefix;

	int n = 0;
	CString sText;
	do
	{
		//keys are of form <lpszKeyPrefix><entrynumber>
		CString sKey;
		sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n++);
		sText = CRegString(sKey);
		if (!sText.IsEmpty())
			AddString(sText);
	} while (!sText.IsEmpty() && n < m_nMaxHistoryItems);

	SetCurSel(-1);

	ModifyStyleEx(WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE, 0);

	// need to resize the control for correct display
	CRect rect;
	GetWindowRect(rect);
	GetParent()->ScreenToClient(rect);
	MoveWindow(rect.left, rect.top, rect.Width(),100);

	return sText;
}

void CHistoryCombo::SaveHistory()
{
	if (m_sSection.IsEmpty())
		return;

	//add the current item to the history
	CString sCurItem;
	GetWindowText(sCurItem);
	sCurItem.Trim();
	if (!sCurItem.IsEmpty())
		AddString(sCurItem, 0);
	//save history to registry/inifile
	int nMax = min(GetCount(), m_nMaxHistoryItems + 1);
	for (int n = 0; n < nMax; n++)
	{
		CString sKey;
		sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
		CRegString regkey = CRegString(sKey);
		regkey = m_arEntries.GetAt(n);
	}
	//remove items exceeding the max number of history items
	for (int n = nMax; ; n++)
	{
		CString sKey;
		sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
		CRegString regkey = CRegString(sKey);
		CString sText = regkey;
		if (sText.IsEmpty())
			break;
		regkey.removeValue(); // remove entry
	}
}

void CHistoryCombo::ClearHistory(BOOL bDeleteRegistryEntries/*=TRUE*/)
{
	ResetContent();
	if (! m_sSection.IsEmpty() && bDeleteRegistryEntries)
	{
		//remove profile entries
		CString sKey;
		for (int n = 0; ; n++)
		{
			sKey.Format(_T("%s\\%s%d"), (LPCTSTR)m_sSection, (LPCTSTR)m_sKeyPrefix, n);
			CRegString regkey = CRegString(sKey);
			CString sText = regkey;
			if (sText.IsEmpty())
				break;
			regkey.removeValue(); // remove entry
		}
	}
}

void CHistoryCombo::SetURLHistory(BOOL bURLHistory)
{
	m_bURLHistory = bURLHistory;

	if (m_bURLHistory)
	{
		HWND hwndEdit;
		// use for ComboEx
		hwndEdit = (HWND)::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0);
		if (NULL == hwndEdit)
		{
			// Try the unofficial way of getting the edit control CWnd*
			CWnd* pWnd = this->GetDlgItem(1001);
			if(pWnd)
			{
				hwndEdit = pWnd->GetSafeHwnd();
			}
		}
		if (hwndEdit)
			SHAutoComplete(hwndEdit, SHACF_URLALL);
	}

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
	SetImageList(&SYS_IMAGE_LIST());
#endif
}

void CHistoryCombo::SetPathHistory(BOOL bPathHistory)
{
	m_bPathHistory = bPathHistory;

	if (m_bPathHistory)
	{
		HWND hwndEdit;
		// use for ComboEx
		hwndEdit = (HWND)::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0);
		if (NULL == hwndEdit)
		{
			//if not, try the old standby
			if(hwndEdit==NULL)
			{
				CWnd* pWnd = this->GetDlgItem(1001);
				if(pWnd)
				{
					hwndEdit = pWnd->GetSafeHwnd();
				}
			}
		}
		if (hwndEdit)
			SHAutoComplete(hwndEdit, SHACF_FILESYSTEM);
	}

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
	SetImageList(&SYS_IMAGE_LIST());
#endif
}

void CHistoryCombo::SetMaxHistoryItems(int nMaxItems)
{
	m_nMaxHistoryItems = nMaxItems;

	//truncate list to nMaxItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
		DeleteString(m_nMaxHistoryItems);
}
void CHistoryCombo::AddString(STRING_VECTOR &list,BOOL isSel)
{
	for(unsigned int i=0;i<list.size();i++)
	{
		AddString(list[i], -1, isSel);
	}
}
CString CHistoryCombo::GetString() const
{
	CString str;
	int sel;
	sel = GetCurSel();
	DWORD style=GetStyle();
	
	if (sel == CB_ERR ||(m_bURLHistory)||(m_bPathHistory) || (!(style&CBS_SIMPLE)))
	{
		GetWindowText(str);
		return str;
	}

	return m_arEntries.GetAt(sel);
}

BOOL CHistoryCombo::RemoveSelectedItem()
{
	int nIndex = GetCurSel();
	if (nIndex == CB_ERR)
	{
		return FALSE;
	}

	DeleteItem(nIndex);
	m_arEntries.RemoveAt(nIndex);

	if ( nIndex < GetCount() )
	{
		// index stays the same to select the
		// next item after the item which has
		// just been deleted
	}
	else
	{
		// the end of the list has been reached
		// so we select the previous item
		nIndex--;
	}

	if ( nIndex == -1 )
	{
		// The one and only item has just been
		// deleted -> reset window text since
		// there is no item to select
		SetWindowText(_T(""));
	}
	else
	{
		SetCurSel(nIndex);
	}

	// Since the dialog might be canceled we
	// should now save the history. Before that
	// set the selection to the first item so that
	// the items will not be reordered and restore
	// the selection after saving.
	SetCurSel(0);
	SaveHistory();
	if ( nIndex != -1 )
	{
		SetCurSel(nIndex);
	}

	return TRUE;
}

void CHistoryCombo::PreSubclassWindow()
{
	CComboBoxEx::PreSubclassWindow();

	if (!m_bDyn)
		CreateToolTip();
}

void CHistoryCombo::OnMouseMove(UINT nFlags, CPoint point)
{
	CRect rectClient;
	GetClientRect(&rectClient);
	int nComboButtonWidth = ::GetSystemMetrics(SM_CXHTHUMB) + 2;
	rectClient.right = rectClient.right - nComboButtonWidth;

	if (rectClient.PtInRect(point))
	{
		ClientToScreen(&rectClient);

		CString strText = GetString();
		m_ToolInfo.lpszText = (LPTSTR)(LPCTSTR)strText;

		HDC hDC = ::GetDC(m_hWnd);

		CFont *pFont = GetFont();
		HFONT hOldFont = (HFONT) ::SelectObject(hDC, (HFONT) *pFont);

		SIZE size;
		::GetTextExtentPoint32(hDC, strText, strText.GetLength(), &size);
		::SelectObject(hDC, hOldFont);
		::ReleaseDC(m_hWnd, hDC);

		if (size.cx > (rectClient.Width() - 6))
		{
			rectClient.left += 1;
			rectClient.top += 3;

			COLORREF rgbText = ::GetSysColor(COLOR_WINDOWTEXT);
			COLORREF rgbBackground = ::GetSysColor(COLOR_WINDOW);

			CWnd *pWnd = GetFocus();
			if (pWnd)
			{
				if (pWnd->m_hWnd == m_hWnd)
				{
					rgbText = ::GetSysColor(COLOR_HIGHLIGHTTEXT);
					rgbBackground = ::GetSysColor(COLOR_HIGHLIGHT);
				}
			}

			if (!m_ttShown)
			{
				::SendMessage(m_hWndToolTip, TTM_SETTIPBKCOLOR, rgbBackground, 0);
				::SendMessage(m_hWndToolTip, TTM_SETTIPTEXTCOLOR, rgbText, 0);
				::SendMessage(m_hWndToolTip, TTM_UPDATETIPTEXT, 0, (LPARAM) &m_ToolInfo);
				::SendMessage(m_hWndToolTip, TTM_TRACKPOSITION, 0, (LPARAM)MAKELONG(rectClient.left, rectClient.top));
				::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, TRUE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
				SetTimer(1, 80, NULL);
				SetTimer(2, 2000, NULL);
				m_ttShown = TRUE;
			}
		}
		else
		{
			::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
			m_ttShown = FALSE;
		}
	}
	else
	{
		::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
		m_ttShown = FALSE;
	}

	CComboBoxEx::OnMouseMove(nFlags, point);
}

void CHistoryCombo::OnTimer(UINT_PTR nIDEvent)
{
	CPoint point;
	DWORD ptW = GetMessagePos();
	point.x = GET_X_LPARAM(ptW);
	point.y = GET_Y_LPARAM(ptW);
	ScreenToClient(&point);

	CRect rectClient;
	GetClientRect(&rectClient);
	int nComboButtonWidth = ::GetSystemMetrics(SM_CXHTHUMB) + 2;

	rectClient.right = rectClient.right - nComboButtonWidth;

	if (!rectClient.PtInRect(point))
	{
		KillTimer(nIDEvent);
		::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
		m_ttShown = FALSE;
	}
	if (nIDEvent == 2)
	{
		// tooltip timeout, just deactivate it
		::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, (LPARAM)(LPTOOLINFO) &m_ToolInfo);
		// don't set m_ttShown to FALSE, because we don't want the tooltip to show up again
		// without the mouse pointer first leaving the control and entering it again
	}

	CComboBoxEx::OnTimer(nIDEvent);
}

void CHistoryCombo::CreateToolTip()
{
	// create tooltip
	m_hWndToolTip = ::CreateWindowEx(WS_EX_TOPMOST,
		TOOLTIPS_CLASS,
		NULL,
		TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		m_hWnd,
		NULL,
		NULL,
		NULL);

	// initialize tool info struct
	memset(&m_ToolInfo, 0, sizeof(m_ToolInfo));
	m_ToolInfo.cbSize = sizeof(m_ToolInfo);
	m_ToolInfo.uFlags = TTF_TRANSPARENT;
	m_ToolInfo.hwnd = m_hWnd;

	::SendMessage(m_hWndToolTip, TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
	::SendMessage(m_hWndToolTip, TTM_ADDTOOL, 0, (LPARAM) (LPTOOLINFO) &m_ToolInfo);
	::SendMessage(m_hWndToolTip, TTM_SETTIPBKCOLOR, ::GetSysColor(COLOR_HIGHLIGHT), 0);
	::SendMessage(m_hWndToolTip, TTM_SETTIPTEXTCOLOR, ::GetSysColor(COLOR_HIGHLIGHTTEXT), 0);

	CRect rectMargins(0,-1,0,-1);
	::SendMessage(m_hWndToolTip, TTM_SETMARGIN, 0, (LPARAM)&rectMargins);

	CFont *pFont = GetFont();
	::SendMessage(m_hWndToolTip, WM_SETFONT, (WPARAM)(HFONT)*pFont, FALSE);
}

int CHistoryCombo::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBoxEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_bDyn)
		CreateToolTip();

	return 0;
}
