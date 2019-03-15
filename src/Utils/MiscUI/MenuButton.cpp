// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2015-2017, 2019 - TortoiseGit
// Copyright (C) 2011,2015-2016 - Sven Strickroth <email@cs-ware.de>

//based on:
// Copyright (C) 2003-2006,2008 - Stefan Kueng

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
#include "MenuButton.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CMenuButton, CMFCMenuButton)

CMenuButton::CMenuButton(void) : CMFCMenuButton()
	, m_nDefault(0)
	, m_bMarkDefault(TRUE)
	, m_bShowCurrentItem(true)
	, m_bRealMenuIsActive(false)
	, m_bAlwaysShowArrow(false)
{
	m_bOSMenu = TRUE;
	m_bDefaultClick = TRUE;
	m_bTransparent = TRUE;
	m_bMenuIsActive = TRUE;
	m_bStayPressed = TRUE;

	m_btnMenu.CreatePopupMenu();
	m_hMenu = m_btnMenu.GetSafeHmenu();
}

CMenuButton::~CMenuButton(void)
{
}

bool CMenuButton::SetCurrentEntry(INT_PTR entry)
{
	if (entry < 0 || entry >= m_sEntries.GetCount() || !m_bShowCurrentItem)
		return false;

	m_nDefault = entry + 1;
	SetWindowText(m_sEntries[entry]);
	if (m_bMarkDefault)
		m_btnMenu.SetDefaultItem(static_cast<UINT>(m_nDefault), FALSE);

	return true;
}

void CMenuButton::RemoveAll()
{
	for (int index = 0; index < m_sEntries.GetCount(); index++)
		m_btnMenu.RemoveMenu(0, MF_BYPOSITION);
	m_sEntries.RemoveAll();
	m_nDefault = m_nMenuResult = 0;
	m_bMenuIsActive = TRUE;
}

INT_PTR CMenuButton::AddEntry(const CString& sEntry, UINT uIcon /*= 0U*/)
{
	INT_PTR ret = m_sEntries.Add(sEntry);
	m_btnMenu.AppendMenuIcon(m_sEntries.GetCount(), sEntry, uIcon);
	if (m_sEntries.GetCount() == 2)
		m_bMenuIsActive = FALSE;

	if (ret == 0)
		SetCurrentEntry(ret);
	return ret;
}

INT_PTR CMenuButton::AddEntries(const CStringArray& sEntries)
{
	INT_PTR ret = -1;
	for (int index = 0; index < sEntries.GetCount(); index++)
	{
		if (index == 0)
			ret = AddEntry(sEntries[index]);
		else
			AddEntry(sEntries[index]);
	}
	return ret;
}

void CMenuButton::FixFont()
{
	CWnd* pWnd = GetParent();

	if (pWnd == nullptr)
		return;

	CFont* pFont = pWnd->GetFont();
	if (pFont == nullptr)
		return;

	LOGFONT logfont;
	if (pFont->GetLogFont(&logfont) == 0)
		return;

	if (m_Font.CreateFontIndirect(&logfont) == FALSE)
		return;

	SetFont(&m_Font);
	Invalidate();
}

BEGIN_MESSAGE_MAP(CMenuButton, CMFCMenuButton)
	ON_WM_DESTROY()
	ON_CONTROL_REFLECT_EX(BN_CLICKED, &CMenuButton::OnClicked)
	ON_WM_SYSCOLORCHANGE()
	ON_WM_THEMECHANGED()
END_MESSAGE_MAP()


BOOL CMenuButton::OnClicked()
{
	SetCurrentEntry(GetCurrentEntry());

	// let the parent handle the message the usual way
	return FALSE;
}

BOOL CMenuButton::PreTranslateMessage(MSG* pMsg)
{
	if(pMsg->message == WM_KEYDOWN)
	{
		switch(pMsg->wParam)
		{
		case VK_RETURN:
		case VK_SPACE:
			if (m_bMenuIsActive && !m_bRealMenuIsActive)
			{
				GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
				return TRUE;
			}
		case VK_F4:
			OnShowMenu();
			return TRUE;
		}
	}

	return CMFCMenuButton::PreTranslateMessage(pMsg);
}

void CMenuButton::OnDestroy()
{
	m_sEntries.RemoveAll();

	CMFCMenuButton::OnDestroy();
}

void CMenuButton::OnDraw(CDC* pDC, const CRect& rect, UINT uiState)
{
	if (m_bMenuIsActive && !m_bRealMenuIsActive && !m_bAlwaysShowArrow)
		CMFCButton::OnDraw(pDC, rect, uiState);
	else
		CMFCMenuButton::OnDraw(pDC, rect, uiState);

	if (m_Font.m_hObject == nullptr)
		FixFont();
}

void CMenuButton::OnShowMenu()
{
	m_bRealMenuIsActive = true;

	// Begin CMFCMenuButton::OnShowMenu()
	if (m_hMenu == NULL || m_bMenuIsActive)
	{
		return;
	}

	CRect rectWindow;
	GetWindowRect(rectWindow);

	int x, y;

	if (m_bRightArrow)
	{
		x = rectWindow.right;
		y = rectWindow.top;
	}
	else
	{
		x = rectWindow.left;
		y = rectWindow.bottom;
	}

	if (m_bStayPressed)
	{
		m_bPushed = TRUE;
		m_bHighlighted = TRUE;
	}

	m_bMenuIsActive = TRUE;
	Invalidate();

	TPMPARAMS params;
	params.cbSize = sizeof(TPMPARAMS);
	params.rcExclude = rectWindow;
	m_nMenuResult = ::TrackPopupMenuEx(m_hMenu, TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD | TPM_VERTICAL, x, y, GetSafeHwnd(), &params);

	CWnd* pParent = GetParent();

#ifdef _DEBUG
	if ((pParent->IsKindOf(RUNTIME_CLASS(CDialog))) && (!pParent->IsKindOf(RUNTIME_CLASS(CDialogEx))))
	{
		TRACE(_T("CMFCMenuButton parent is CDialog, should be CDialogEx for popup menu handling to work correctly.\n"));
	}
#endif

	if (m_nMenuResult != 0)
	{
		//-------------------------------------------------------
		// Trigger mouse up event(to button click notification):
		//-------------------------------------------------------
		if (pParent != NULL)
		{
			pParent->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
		}
	}

	m_bPushed = FALSE;
	m_bHighlighted = FALSE;
	m_bMenuIsActive = FALSE;

	Invalidate();
	UpdateWindow();

	if (m_bCaptured)
	{
		ReleaseCapture();
		m_bCaptured = FALSE;
	}
	// End CMFCMenuButton::OnShowMenu()

	m_bRealMenuIsActive = false;
}

LRESULT CMenuButton::OnThemeChanged()
{
	CMFCVisualManager::GetInstance()->DestroyInstance();
	m_Font.DeleteObject();
	return 0L;
}

void CMenuButton::OnSysColorChange()
{
	__super::OnSysColorChange();
	GetGlobalData()->UpdateSysColors();
}
