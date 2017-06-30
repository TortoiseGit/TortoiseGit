// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016 - TortoiseGit

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
#include "ColumnManager.h"

template <typename BaseType> class CResizableColumnsListCtrl : public BaseType
{
public:
	CResizableColumnsListCtrl()
		: BaseType()
		, m_ColumnManager(this)
		, m_bAllowHiding(true)
	{}

	DECLARE_MESSAGE_MAP()

protected:
	void OnDestroy()
	{
		SaveColumnWidths();
		BaseType::OnDestroy();
	}

	void OnHeaderDblClick(NMHDR* pNMHDR, LRESULT* pResult)
	{
		m_ColumnManager.OnHeaderDblClick(pNMHDR, pResult);

		*pResult = FALSE;
	}

	void OnColumnResized(NMHDR* pNMHDR, LRESULT* pResult)
	{
		m_ColumnManager.OnColumnResized(pNMHDR, pResult);

		*pResult = FALSE;
	}

	void OnColumnMoved(NMHDR* pNMHDR, LRESULT* pResult)
	{
		m_ColumnManager.OnColumnMoved(pNMHDR, pResult);

		Invalidate(FALSE);
	}

	void OnContextMenuHeader(CWnd* pWnd, CPoint point)
	{
		m_ColumnManager.OnContextMenuHeader(pWnd, point, !!IsGroupViewEnabled());
	}

	void OnContextMenu(CWnd* pWnd, CPoint point)
	{
		if (pWnd == GetHeaderCtrl() && m_bAllowHiding)
			OnContextMenuHeader(pWnd, point);
		else if (pWnd == this && m_ContextMenuHandler)
			m_ContextMenuHandler(point);
	}

	// prevent users from extending our hidden (size 0) columns
	void OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult)
	{
		m_ColumnManager.OnHdnBegintrack(pNMHDR, pResult);
	}

	// prevent any function from extending our hidden (size 0) columns
	void OnHdnItemchanging(NMHDR* pNMHDR, LRESULT* pResult)
	{
		if (!m_ColumnManager.OnHdnItemchanging(pNMHDR, pResult))
			Default();
	}

	typedef std::function<void(CPoint point)> ContextMenuHandler;
	ContextMenuHandler m_ContextMenuHandler;

public:
	void Init()
	{
		CRegDWORD regFullRowSelect(L"Software\\TortoiseGit\\FullRowSelect", TRUE);
		DWORD exStyle = LVS_EX_HEADERDRAGDROP;
		if (DWORD(regFullRowSelect))
			exStyle |= LVS_EX_FULLROWSELECT;
		SetExtendedStyle(GetExtendedStyle() | exStyle);
	}

	void SetListContextMenuHandler(ContextMenuHandler pContextMenuHandler)
	{
		m_ContextMenuHandler = pContextMenuHandler;
	}

	void AdjustColumnWidths()
	{
		auto header = GetHeaderCtrl();
		if (!header)
			return;
		int maxcol = header->GetItemCount() - 1;
		for (int col = 0; col <= maxcol; col++)
			SetColumnWidth(col, m_ColumnManager.GetWidth(col, true));
	}
	virtual void SaveColumnWidths()
	{
		auto header = GetHeaderCtrl();
		if (!header)
			return;
		int maxcol = header->GetItemCount() - 1;
		for (int col = 0; col <= maxcol; col++)
			if (m_ColumnManager.IsVisible(col))
				m_ColumnManager.ColumnResized(col);

		m_ColumnManager.WriteSettings();
	}

	bool			m_bAllowHiding;
	ColumnManager	m_ColumnManager;
};

BEGIN_TEMPLATE_MESSAGE_MAP(CResizableColumnsListCtrl, BaseType, BaseType)
	ON_NOTIFY(HDN_BEGINTRACKA, 0, OnHdnBegintrack)
	ON_NOTIFY(HDN_BEGINTRACKW, 0, OnHdnBegintrack)
	ON_NOTIFY(HDN_ENDTRACK, 0, OnColumnResized)
	ON_NOTIFY(HDN_ENDDRAG, 0, OnColumnMoved)
	ON_NOTIFY(HDN_DIVIDERDBLCLICK, 0, OnHeaderDblClick)
	ON_NOTIFY(HDN_ITEMCHANGINGA, 0, OnHdnItemchanging)
	ON_NOTIFY(HDN_ITEMCHANGINGW, 0, OnHdnItemchanging)
	ON_WM_DESTROY()
	ON_WM_CONTEXTMENU()
END_MESSAGE_MAP()
