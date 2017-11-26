// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016-2017 - TortoiseGit
// Copyright (C) 2011, 2016 - Sven Strickroth <email@cs-ware.de>

//based on:
// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "IconMenu.h"

/**
 * \ingroup Utils
 * A button control with a menu to choose from different
 * actions. Clicking on the left "button" part is the same
 * as with a normal button, clicking on the right "arrow"
 * part will bring up a menu where the user can choose what
 * action the button should do.
 */
class CMenuButton : public CMFCMenuButton
{
public:
	DECLARE_DYNCREATE(CMenuButton);

	CMenuButton(void);
	virtual ~CMenuButton(void);

	/**
	 * Inserts a text to be shown in the button menu.
	 * The text is inserted at the end of the menu string list.
	 * \return the index of the inserted item. This index is
	 * returned in GetCurrentEntry().
	 */
	INT_PTR AddEntry(const CString& sEntry, UINT uIcon = 0U);

	/**
	 * Inserts an array of strings to be shown in the
	 * button menu. The strings are inserted at the end
	 * of the menu string list.
	 * \return the index of the first inserted item. This index
	 * is returned in GetCurrentEntry().
	 */
	INT_PTR	AddEntries(const CStringArray& sEntries);

	/**
	 * Returns the currently shown entry index of the button.
	 */
	INT_PTR	GetCurrentEntry() const { return (m_nMenuResult == 0) ? m_nDefault - 1 : m_nMenuResult - 1; }

	/**
	 * Sets which of the menu strings should be shown in the
	 * button and be active.
	 * \return true if successful
	 */
	bool	SetCurrentEntry(INT_PTR entry);

	/**
	 * Determines if the button control is drawn with the XP
	 * themes or without. The default is \a true, which means
	 * the control is drawn with theming support if the underlying
	 * OS has it enabled and supports it.
	 */
	void	RemoveAll();

	bool	m_bMarkDefault;

	/** Don't show text on Button */
	bool	m_bShowCurrentItem;

	bool	m_bAlwaysShowArrow;

protected:
	virtual BOOL PreTranslateMessage(MSG* pMsg) override;

	bool	m_bRealMenuIsActive;
	virtual void OnShowMenu() override;
	afx_msg void OnDraw(CDC* pDC, const CRect& rect, UINT uiState);
	afx_msg void OnSysColorChange();
	afx_msg LRESULT OnThemeChanged();

	afx_msg BOOL OnClicked();
	afx_msg void OnDestroy();

	CIconMenu	m_btnMenu;
	INT_PTR	m_nDefault;

	DECLARE_MESSAGE_MAP()

	CStringArray m_sEntries;

	CFont m_Font;
	void FixFont();
};
