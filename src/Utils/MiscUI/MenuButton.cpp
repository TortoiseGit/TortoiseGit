// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011, 2015-2017, 2019-2020 - TortoiseGit
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
#include "Theme.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(CMenuButton, CMFCMenuButton)

CMenuButton::CMenuButton(void) : CThemeMFCMenuButton()
	, m_nDefault(0)
	, m_bMarkDefault(TRUE)
	, m_bShowCurrentItem(true)
	, m_bAlwaysShowArrow(false)
{
	m_bOSMenu = TRUE;
	m_bDefaultClick = TRUE;
	m_bTransparent = TRUE;
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

	SetWindowText(m_sEntries[entry]);
	if (m_bMarkDefault)
	{
		m_nDefault = entry + 1;
		m_btnMenu.SetDefaultItem(static_cast<UINT>(m_nDefault), FALSE);
	}

	return true;
}

void CMenuButton::RemoveAll()
{
	for (int index = 0; index < m_sEntries.GetCount(); index++)
		m_btnMenu.RemoveMenu(0, MF_BYPOSITION);
	m_sEntries.RemoveAll();
	m_nDefault = m_nMenuResult = 0;
}

INT_PTR CMenuButton::AddEntry(const CString& sEntry, UINT uIcon /*= 0U*/)
{
	INT_PTR ret = m_sEntries.Add(sEntry);
	m_btnMenu.AppendMenuIcon(m_sEntries.GetCount(), sEntry, uIcon);

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
	ON_WM_LBUTTONDOWN()
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
			if (!m_bMenuIsActive)
			{
				GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
				return TRUE;
			}
		case VK_F4:
			if (m_bAlwaysShowArrow)
				GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), reinterpret_cast<LPARAM>(m_hWnd));
			else
				OnShowMenu();
			return TRUE;
		}
	}

	return __super::PreTranslateMessage(pMsg);
}

void CMenuButton::OnDestroy()
{
	m_sEntries.RemoveAll();

	__super::OnDestroy();
}

BOOL CMenuButton::IsPressed()
{
	return __super::IsPressed() || m_bChecked;
}

void CMenuButton::OnDraw(CDC* pDC, const CRect& rect, UINT uiState)
{
	m_bNoArrow = !m_bAlwaysShowArrow && m_sEntries.GetCount() < 2;
	if (!m_bAlwaysShowArrow && m_bNoArrow && !CTheme::Instance().IsDarkTheme())
		CMFCButton::OnDraw(pDC, rect, uiState);
	else
		__super::OnDraw(pDC, rect, uiState);

	if (m_Font.m_hObject == nullptr)
		FixFont();
}

void CMenuButton::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_bAlwaysShowArrow || m_sEntries.GetCount() < 2)
		CMFCButton::OnLButtonDown(nFlags, point);
	else
		__super::OnLButtonDown(nFlags, point);
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
