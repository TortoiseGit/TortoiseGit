// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011,2015 - Sven Strickroth <email@cs-ware.de>

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
	, m_nDefault(-1)
	, m_bMarkDefault(TRUE)
	, m_bRealMenuIsActive(false)
{
	m_bOSMenu = TRUE;
	m_bDefaultClick = TRUE;
	m_bTransparent = TRUE;
	m_bMenuIsActive = TRUE;

	m_btnMenu.CreatePopupMenu();
	m_hMenu = m_btnMenu.GetSafeHmenu();
}

CMenuButton::~CMenuButton(void)
{
}

bool CMenuButton::SetCurrentEntry(INT_PTR entry)
{
	if (entry < 0 || entry >= m_sEntries.GetCount())
		return false;

	m_nDefault = entry + 1;
	SetWindowText(m_sEntries[entry]);
	if (m_bMarkDefault)
		m_btnMenu.SetDefaultItem((UINT)m_nDefault, FALSE);

	return true;
}

void CMenuButton::RemoveAll()
{
	for (int index = 0; index < m_sEntries.GetCount(); index++)
		m_btnMenu.RemoveMenu(0, MF_BYPOSITION);
	m_sEntries.RemoveAll();
	m_nDefault = m_nMenuResult = -1;
	m_bMenuIsActive = TRUE;
}

INT_PTR CMenuButton::AddEntry(UINT iconId, const CString& sEntry)
{
	INT_PTR ret = m_sEntries.Add(sEntry);
	m_btnMenu.AppendMenuIcon(m_sEntries.GetCount(), sEntry, iconId);
	if (m_sEntries.GetCount() == 2)
		m_bMenuIsActive = FALSE;

	if (ret == 0)
		SetCurrentEntry(ret);
	return ret;
}

INT_PTR CMenuButton::AddEntry(const CString& sEntry)
{
	INT_PTR ret = m_sEntries.Add(sEntry);
	m_btnMenu.AppendMenuIcon(m_sEntries.GetCount(), sEntry);
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
				GetParent()->SendMessage(WM_COMMAND, MAKEWPARAM(GetDlgCtrlID(), BN_CLICKED), (LPARAM)m_hWnd);
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
	if (m_bMenuIsActive && !m_bRealMenuIsActive)
		CMFCButton::OnDraw(pDC, rect, uiState);
	else
		CMFCMenuButton::OnDraw(pDC, rect, uiState);

	if (m_Font.m_hObject == nullptr)
		FixFont();
}

void CMenuButton::OnShowMenu()
{
	m_bRealMenuIsActive = true;
	CMFCMenuButton::OnShowMenu();
	m_bRealMenuIsActive = false;
}
