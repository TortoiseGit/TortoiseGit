// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2012 - TortoiseSVN
// Copyright (C) 2013-2017, 2019 - TortoiseGit

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

int CHistoryCombo::m_nGitIconIndex = 0;

CHistoryCombo::CHistoryCombo(BOOL bAllowSortStyle /*=FALSE*/ )
	: m_nMaxHistoryItems ( MAX_HISTORY_ITEMS)
	, m_bAllowSortStyle(bAllowSortStyle)
	, m_bURLHistory(FALSE)
	, m_bPathHistory(FALSE)
	, m_hWndToolTip(nullptr)
	, m_ttShown(FALSE)
	, m_bDyn(FALSE)
	, m_bWantReturn(FALSE)
	, m_bTrim(TRUE)
	, m_bCaseSensitive(FALSE)
{
	SecureZeroMemory(&m_ToolInfo, sizeof(m_ToolInfo));
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
		int nVirtKey = static_cast<int>(pMsg->wParam);

		if (nVirtKey == VK_RETURN)
		{
			if (GetDroppedState())
			{
				// Don't directly pass return to parent if selecting an item in drop down list
				ShowDropDown(FALSE);
				return TRUE;
			}
			return OnReturnKeyPressed();
		}
		else if (nVirtKey == VK_DELETE && bShift && GetDroppedState() )
		{
			RemoveSelectedItem();
			return TRUE;
		}

		if (nVirtKey == 'A' && (GetKeyState(VK_CONTROL) & 0x8000 ) )
		{
			CEdit *edit = GetEditCtrl();
			if (edit)
				edit->SetSel(0, -1);
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
			OnMouseMove(static_cast<UINT>(pMsg->wParam), pt);
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

int CHistoryCombo::AddString(const CString& str, INT_PTR pos /* = -1*/, BOOL isSel /* = TRUE */)
{
	if (str.IsEmpty())
		return -1;

	if (pos < 0)
		pos = GetCount();

	CString combostring = str;
	combostring.Replace('\r', ' ');
	combostring.Replace('\n', ' ');
	if (m_bTrim)
		combostring.Trim();
	if (combostring.IsEmpty())
		return -1;

	//search the Combo for another string like this
	//and do not insert if found
	int nIndex = m_bCaseSensitive ? FindStringExactCaseSensitive(-1, combostring) : FindStringExact(-1, combostring);
	if (nIndex != -1)
	{
		if (nIndex > pos)
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

	//truncate list to m_nMaxHistoryItems
	int nNumItems = GetCount();
	for (int n = m_nMaxHistoryItems; n < nNumItems; n++)
	{
		DeleteItem(m_nMaxHistoryItems);
		m_arEntries.RemoveAt(m_nMaxHistoryItems);
	}

	int nRet = InsertEntry(combostring, pos);

	if (isSel)
		SetCurSel(nRet);
	return nRet;
}

int CHistoryCombo::InsertEntry(const CString& combostring, INT_PTR pos)
{
	COMBOBOXEXITEM cbei = { 0 };
	cbei.mask = CBEIF_TEXT;
	cbei.iItem = pos;

	cbei.pszText = const_cast<LPTSTR>(combostring.GetString());

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
	if (m_bURLHistory)
	{
		cbei.iImage = SYS_IMAGE_LIST().GetPathIconIndex(combostring);
		if (cbei.iImage == 0 || cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex())
		{
			if (CStringUtils::StartsWith(combostring, L"http:"))
				cbei.iImage = SYS_IMAGE_LIST().GetPathIconIndex(L".html");
			else if (CStringUtils::StartsWith(combostring, L"https:"))
				cbei.iImage = SYS_IMAGE_LIST().GetPathIconIndex(L".html");
			else if (CStringUtils::StartsWith(combostring, L"file:"))
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
			else if (CStringUtils::StartsWith(combostring, L"git:"))
				cbei.iImage = m_nGitIconIndex;
			else if (CStringUtils::StartsWith(combostring, L"ssh:"))
				cbei.iImage = m_nGitIconIndex;
			else
				cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
		}
		cbei.iSelectedImage = cbei.iImage;
		cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	}
	if (m_bPathHistory)
	{
		cbei.iImage = SYS_IMAGE_LIST().GetPathIconIndex(combostring);
		if (cbei.iImage == SYS_IMAGE_LIST().GetDefaultIconIndex())
		{
			cbei.iImage = SYS_IMAGE_LIST().GetDirIconIndex();
		}
		cbei.iSelectedImage = cbei.iImage;
		cbei.mask |= CBEIF_IMAGE | CBEIF_SELECTEDIMAGE;
	}
#endif

	int nRet = InsertItem(&cbei);
	if (nRet >= 0)
		m_arEntries.InsertAt(nRet, combostring);

	return nRet;
}

void CHistoryCombo::SetList(const STRING_VECTOR& list)
{
	Reset();
	for (size_t i = 0; i < list.size(); ++i)
	{
		CString combostring = list[i];
		combostring.Replace('\r', ' ');
		combostring.Replace('\n', ' ');
		if (m_bTrim)
			combostring.Trim();
		if (combostring.IsEmpty())
			continue;

		InsertEntry(combostring, i);
	}
}

CString CHistoryCombo::LoadHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix)
{
	if (!lpszSection || !lpszKeyPrefix || *lpszSection == '\0')
		return L"";

	m_sSection = lpszSection;
	m_sKeyPrefix = lpszKeyPrefix;

	int n = 0;
	CString sText;
	do
	{
		//keys are of form <lpszKeyPrefix><entrynumber>
		CString sKey;
		sKey.Format(L"%s\\%s%d", static_cast<LPCTSTR>(m_sSection), static_cast<LPCTSTR>(m_sKeyPrefix), n++);
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
	if (m_bTrim)
		sCurItem.Trim();
	if (!sCurItem.IsEmpty())
		AddString(sCurItem, 0);
	//save history to registry/inifile
	int nMax = min(GetCount(), m_nMaxHistoryItems + 1);
	for (int n = 0; n < nMax; n++)
	{
		CString sKey;
		sKey.Format(L"%s\\%s%d", static_cast<LPCTSTR>(m_sSection), static_cast<LPCTSTR>(m_sKeyPrefix), n);
		CRegString regkey(sKey);
		regkey = m_arEntries.GetAt(n);
	}
	//remove items exceeding the max number of history items
	for (int n = nMax; ; n++)
	{
		CString sKey;
		sKey.Format(L"%s\\%s%d", static_cast<LPCTSTR>(m_sSection), static_cast<LPCTSTR>(m_sKeyPrefix), n);
		CRegString regkey(sKey);
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
			sKey.Format(L"%s\\%s%d", static_cast<LPCTSTR>(m_sSection), static_cast<LPCTSTR>(m_sKeyPrefix), n);
			CRegString regkey(sKey);
			CString sText = regkey;
			if (sText.IsEmpty())
				break;
			regkey.removeValue(); // remove entry
		}
	}
}

void CHistoryCombo::RemoveEntryFromHistory(LPCTSTR lpszSection, LPCTSTR lpszKeyPrefix, const CString& entryToRemove)
{
	if (entryToRemove.IsEmpty())
		return;
	CString sText;
	bool found = false;
	int n = -1;
	do
	{
		CString sKey;
		sKey.Format(L"%s\\%s%d", lpszSection, lpszKeyPrefix, ++n);
		CRegString regkey(sKey);
		sText = regkey;
		if (sText == entryToRemove)
		{
			regkey.removeValue();
			found = true;
			++n;
			break;
		}
	} while (!sText.IsEmpty());
	if (!found)
		return;
	for (;; ++n)
	{
		CString sKey;
		sKey.Format(L"%s\\%s%d", lpszSection, lpszKeyPrefix, n);
		CRegString regkey(sKey);
		sText = regkey;
		if (!sText.IsEmpty())
		{
			CString sKeyNew;
			sKeyNew.Format(L"%s\\%s%d", lpszSection, lpszKeyPrefix, n - 1);
			CRegString regkeyNew(sKeyNew);
			regkeyNew = sText;
			regkey.removeValue();
			continue;
		}
		else
			break;
	}
}

void CHistoryCombo::SetURLHistory(BOOL bURLHistory)
{
	m_bURLHistory = bURLHistory;

	if (m_bURLHistory)
	{
		// use for ComboEx
		HWND hwndEdit = reinterpret_cast<HWND>(::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0));
		if (!hwndEdit)
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
		// use for ComboEx
		HWND hwndEdit = reinterpret_cast<HWND>(::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0));
		if (!hwndEdit)
		{
			CWnd* pWnd = this->GetDlgItem(1001);
			if (pWnd)
				hwndEdit = pWnd->GetSafeHwnd();
		}
		if (hwndEdit)
			SHAutoComplete(hwndEdit, SHACF_FILESYSTEM);
	}

#ifdef HISTORYCOMBO_WITH_SYSIMAGELIST
	SetImageList(&SYS_IMAGE_LIST());
#endif
}

void CHistoryCombo::SetCustomAutoSuggest(BOOL listEntries, BOOL bPathHistory, BOOL bURLHistory)
{
	m_bPathHistory = bPathHistory;
	m_bURLHistory = bURLHistory;

	// use for ComboEx
	HWND hwndEdit = reinterpret_cast<HWND>(::SendMessage(this->m_hWnd, CBEM_GETEDITCONTROL, 0, 0));
	if (!hwndEdit)
	{
		CWnd* pWnd = this->GetDlgItem(1001);
		if (pWnd)
			hwndEdit = pWnd->GetSafeHwnd();
	}
	if (hwndEdit)
	{
		CComPtr<IObjMgr> pom;
		if (!SUCCEEDED(CoCreateInstance(CLSID_ACLMulti, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pom))))
			return;

		if (listEntries)
		{
			CComPtr<IUnknown> punkSource;
			CComPtr<CCustomAutoCompleteSource> pcacs = new CCustomAutoCompleteSource(m_arEntries);
			if (SUCCEEDED(pcacs->QueryInterface(IID_PPV_ARGS(&punkSource))))
				pom->Append(punkSource);
		}

		if (m_bPathHistory)
		{
			CComPtr<IUnknown> punkSource2;
			if (SUCCEEDED(CoCreateInstance(CLSID_ACListISF, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&punkSource2))))
			{
				CComPtr<IACList2> pal2;
				if (SUCCEEDED(punkSource2->QueryInterface(IID_PPV_ARGS(&pal2))))
					pal2->SetOptions(ACLO_FILESYSDIRS);
				pom->Append(punkSource2);
			}
		}

		if (m_bURLHistory)
		{
			CComPtr<IUnknown> punkSource3;
			if (SUCCEEDED(CoCreateInstance(CLSID_ACLHistory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&punkSource3))))
				pom->Append(punkSource3);

			CComPtr<IUnknown> punkSource4;
			if (SUCCEEDED(CoCreateInstance(CLSID_ACLMRU, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&punkSource4))))
				pom->Append(punkSource4);
		}

		CComPtr<IAutoComplete> pac;
		if (!SUCCEEDED(CoCreateInstance(CLSID_AutoComplete, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pac))))
			return;
		pac->Init(hwndEdit, pom, nullptr, nullptr);

		CComPtr<IAutoComplete2> pac2;
		if (SUCCEEDED(pac->QueryInterface(IID_PPV_ARGS(&pac2))))
			pac2->SetOptions(ACO_AUTOSUGGEST);
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

CString CHistoryCombo::GetString() const
{
	CString str;
	int sel = GetCurSel();
	DWORD style=GetStyle();

	if (sel == CB_ERR ||(m_bURLHistory)||(m_bPathHistory) || (!(style&CBS_SIMPLE)))
	{
		GetWindowText(str);
		if (m_bTrim)
			str.Trim();
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
		SetWindowText(L"");
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

		m_ToolText = GetString();
		m_ToolInfo.lpszText = const_cast<LPTSTR>(static_cast<LPCTSTR>(m_ToolText));

		HDC hDC = ::GetDC(m_hWnd);

		CFont *pFont = GetFont();
		HFONT hOldFont = static_cast<HFONT>(::SelectObject(hDC, *pFont));

		SIZE size;
		::GetTextExtentPoint32(hDC, m_ToolText, m_ToolText.GetLength(), &size);
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
				::SendMessage(m_hWndToolTip, TTM_UPDATETIPTEXT, 0, reinterpret_cast<LPARAM>(&m_ToolInfo));
				::SendMessage(m_hWndToolTip, TTM_TRACKPOSITION, 0, MAKELONG(rectClient.left, rectClient.top));
				::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, TRUE, reinterpret_cast<LPARAM>(&m_ToolInfo));
				SetTimer(1, 80, nullptr);
				SetTimer(2, 2000, nullptr);
				m_ttShown = TRUE;
			}
		}
		else
		{
			::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ToolInfo));
			m_ttShown = FALSE;
		}
	}
	else
	{
		::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ToolInfo));
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
		::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ToolInfo));
		m_ttShown = FALSE;
	}
	if (nIDEvent == 2)
	{
		// tooltip timeout, just deactivate it
		::SendMessage(m_hWndToolTip, TTM_TRACKACTIVATE, FALSE, reinterpret_cast<LPARAM>(&m_ToolInfo));
		// don't set m_ttShown to FALSE, because we don't want the tooltip to show up again
		// without the mouse pointer first leaving the control and entering it again
	}

	CComboBoxEx::OnTimer(nIDEvent);
}

void CHistoryCombo::CreateToolTip()
{
	// create tooltip
	m_hWndToolTip = ::CreateWindowEx(NULL,
		TOOLTIPS_CLASS,
		nullptr,
		TTS_NOPREFIX | TTS_ALWAYSTIP,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		m_hWnd,
		nullptr,
		nullptr,
		nullptr);

	// initialize tool info struct
	memset(&m_ToolInfo, 0, sizeof(m_ToolInfo));
	m_ToolInfo.cbSize = sizeof(m_ToolInfo);
	m_ToolInfo.uFlags = TTF_TRANSPARENT;
	m_ToolInfo.hwnd = m_hWnd;

	::SendMessage(m_hWndToolTip, TTM_SETMAXTIPWIDTH, 0, SHRT_MAX);
	::SendMessage(m_hWndToolTip, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&m_ToolInfo));
	::SendMessage(m_hWndToolTip, TTM_SETTIPBKCOLOR, ::GetSysColor(COLOR_HIGHLIGHT), 0);
	::SendMessage(m_hWndToolTip, TTM_SETTIPTEXTCOLOR, ::GetSysColor(COLOR_HIGHLIGHTTEXT), 0);

	CRect rectMargins(0,-1,0,-1);
	::SendMessage(m_hWndToolTip, TTM_SETMARGIN, 0, reinterpret_cast<LPARAM>(&rectMargins));

	CFont *pFont = GetFont();
	::SendMessage(m_hWndToolTip, WM_SETFONT, reinterpret_cast<WPARAM>(static_cast<HFONT>(*pFont)), FALSE);
}

int CHistoryCombo::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CComboBoxEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (m_bDyn)
		CreateToolTip();

	return 0;
}

int CHistoryCombo::FindStringExactCaseSensitive(int nIndexStart, LPCTSTR lpszFind)
{
	nIndexStart = max(0, nIndexStart);
	for (int i = nIndexStart; i < GetCount(); i++)
	{
		CString exactString;
		GetLBText(i, exactString);
		if (exactString == lpszFind)
		{
			return i;
		}
	}
	return -1;
}

CCustomAutoCompleteSource::CCustomAutoCompleteSource(const CStringArray& pData)
	: m_pData(pData)
	, m_index(0)
	, m_cRefCount(0)
{
}

STDMETHODIMP CCustomAutoCompleteSource::QueryInterface(REFIID riid, void** ppvObject)
{
	if (!ppvObject)
		return E_POINTER;
	*ppvObject = nullptr;
	if (IsEqualIID(IID_IUnknown, riid) || IsEqualIID(IID_IEnumString, riid))
		*ppvObject = static_cast<IEnumString*>(this);
	else
		return E_NOINTERFACE;

	AddRef();
	return S_OK;
}

STDMETHODIMP_(ULONG) CCustomAutoCompleteSource::AddRef()
{
	return ++m_cRefCount;
}

STDMETHODIMP_(ULONG) CCustomAutoCompleteSource::Release()
{
	--m_cRefCount;
	if (m_cRefCount == 0)
	{
		delete this;
		return 0;
	}
	return m_cRefCount;
}

STDMETHODIMP_(HRESULT) CCustomAutoCompleteSource::Clone(IEnumString** ppenum)
{
	if (!ppenum)
		return E_POINTER;

	CCustomAutoCompleteSource* pnew = new CCustomAutoCompleteSource(m_pData);

	pnew->AddRef();
	*ppenum = pnew;

	return S_OK;
}

STDMETHODIMP_(HRESULT) CCustomAutoCompleteSource::Next(ULONG celt, LPOLESTR* rgelt, ULONG* pceltFetched)
{
	if (!celt)
		celt = 1;

	ULONG i = 0;
	for (; i < celt && m_index < m_pData.GetCount(); i++)
	{
		rgelt[i] = static_cast<LPWSTR>(::CoTaskMemAlloc(sizeof(WCHAR) * (m_pData.GetAt(m_index).GetLength() + 1)));
		lstrcpy(rgelt[i], m_pData.GetAt(m_index));

		if (pceltFetched)
			++*pceltFetched;

		++m_index;
	}

	if (i == celt)
		return S_OK;

	return S_FALSE;
}

STDMETHODIMP_(HRESULT) CCustomAutoCompleteSource::Reset()
{
	m_index = 0;
	return S_OK;
}

STDMETHODIMP_(HRESULT) CCustomAutoCompleteSource::Skip(ULONG celt)
{
	m_index += celt;
	if (m_index >= m_pData.GetCount())
		return S_FALSE;
	return S_OK;
}
