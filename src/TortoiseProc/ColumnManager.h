// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2016, 2019 - TortoiseGit
// Copyright (C) 2003-2008, 2014 - TortoiseSVN

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

/**
* \ingroup TortoiseProc
* Helper class for CGitStatusListCtrl that represents
* the columns visible and their order as well as
* persisting that data in the registry.
*
* It assigns logical index values to the (potential) columns:
* 0 .. GitSLC_NUMCOLUMNS-1 contain the standard attributes
*
* The column vector contains the columns that are actually
* available in the control.
*
*/
class ColumnManager
{
public:

	/// construction / destruction

	ColumnManager(CListCtrl* control) : control(control), m_dwDefaultColumns(0) {};
	~ColumnManager() {};

	/// registry access

	void ReadSettings(DWORD defaultColumns, DWORD hideColumns, const CString& containerName, int ReadSettings, int* withlist = nullptr);
	void WriteSettings() const;

	/// read column definitions

	int GetColumnCount() const;                     ///< total number of columns
	bool IsVisible(int column) const;
	int GetInvisibleCount() const;
	bool IsRelevant(int column) const;
	CString GetName(int column) const;
	int SetNames(UINT* buff, int size);
	int GetWidth(int column, bool useDefaults = false) const;
	int GetVisibleWidth(int column, bool useDefaults) const;
	void SetRightAlign(int column) const;

	/// switch columns on and off
	void SetVisible(int column, bool visible);

	/// tracking column modifications
	void ColumnMoved(int column, int position);
	/**
	manual: 0: automatic updates, 1: manual updates, 2: manual updates and set to optimal width, 3: reset manual adjusted state
	*/
	void ColumnResized(int column, int manual = 0);

	/// call these to update the user-prop list
	/// (will also auto-insert /-remove new list columns)

	/// don't clutter the context menu with irrelevant prop info

	void RemoveUnusedProps();

	/// bring everything back to its "natural" order
	void ResetColumns(DWORD defaultColumns);

	void OnHeaderDblClick(NMHDR* pNMHDR, LRESULT* pResult)
	{
		LPNMHEADER header = reinterpret_cast<LPNMHEADER>(pNMHDR);
		if (header && (header->iItem >= 0) && (header->iItem < GetColumnCount()))
		{
			bool bShift = !!(GetAsyncKeyState(VK_SHIFT) & 0x8000);
			ColumnResized(header->iItem, bShift ? 3 : 2);
		}
		*pResult = 0;
	}

	void OnColumnResized(NMHDR* pNMHDR, LRESULT* pResult)
	{
		LPNMHEADER header = reinterpret_cast<LPNMHEADER>(pNMHDR);
		if (header && (header->iItem >= 0) && (header->iItem < GetColumnCount()))
			ColumnResized(header->iItem, 1);
		*pResult = 0;
	}

	void OnColumnMoved(NMHDR* pNMHDR, LRESULT* pResult)
	{
		LPNMHEADER header = reinterpret_cast<LPNMHEADER>(pNMHDR);
		*pResult = TRUE;
		if (header
			&& (header->iItem >= 0)
			&& (header->iItem < GetColumnCount())
			// only allow the reordering if the column was not moved left of the first
			// visible item - otherwise the 'invisible' columns are not at the far left
			// anymore and we get all kinds of redrawing problems.
			&& (header->pitem)
			&& (header->pitem->iOrder >= GetInvisibleCount()))
		{
			ColumnMoved(header->iItem, header->pitem->iOrder);
		}
	}

	void OnHdnBegintrack(NMHDR* pNMHDR, LRESULT* pResult)
	{
		LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
		*pResult = 0;
		if (phdr->iItem < 0 || phdr->iItem >= static_cast<int>(itemName.size()))
			return;

		if (IsVisible(phdr->iItem))
			return;
		*pResult = 1;
	}

	int OnHdnItemchanging(NMHDR* pNMHDR, LRESULT* pResult)
	{
		LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
		*pResult = 0;
		if (phdr->iItem < 0 || phdr->iItem >= static_cast<int>(itemName.size()))
			return 0;

		// visible columns may be modified
		if (IsVisible(phdr->iItem))
			return 0;

		// columns already marked as "invisible" internally may be (re-)sized to 0
		if (phdr->pitem && (phdr->pitem->mask == HDI_WIDTH) && (phdr->pitem->cxy == 0))
			return 0;

		if (phdr->pitem && (phdr->pitem->mask != HDI_WIDTH))
			return 0;

		*pResult = 1;
		return 1;
	}
	void OnContextMenuHeader(CWnd* pWnd, CPoint point, bool isGroundEnable = false);

private:
	void AddMenuItem(CMenu* pop)
	{
		UINT uCheckedFlags = MF_STRING | MF_ENABLED | MF_CHECKED;
		UINT uUnCheckedFlags = MF_STRING | MF_ENABLED;

		for (int i = 1; i < static_cast<int>(itemName.size()); ++i)
		{
			if (IsRelevant(i))
				pop->AppendMenu(IsVisible(i)
				? uCheckedFlags
				: uUnCheckedFlags
				, i
				, GetName(i));
		}
	}

	DWORD m_dwDefaultColumns;

	/// initialization utilities
	void ParseWidths(const CString& widths);
	void SetStandardColumnVisibility(DWORD visibility);
	void ParseColumnOrder(const CString& widths);

	/// map internal column order onto visible column order
	/// (all invisibles in front)

	std::vector<int> GetGridColumnOrder() const;
	void ApplyColumnOrder();

	/// utilities used when writing data to the registry
	DWORD GetSelectedStandardColumns() const;
	CString GetWidthString() const;
	CString GetColumnOrderString() const;

	/// our parent control and its data
	CListCtrl* control;

	/// where to store in the registry
	CString registryPrefix;

	/// all columns in their "natural" order
	struct ColumnInfo
	{
		int index;          ///< is a user prop when < GitSLC_USERPROPCOLOFFSET
		int width;
		bool visible;
		bool relevant;      ///< set to @a visible, if no *shown* item has that property
		bool adjusted;
	};

	std::vector<ColumnInfo> columns;

	/// user-defined properties
	std::set<CString> itemProps;

	/// global column ordering including unused user props
	std::vector<int> columnOrder;

	std::vector<int> itemName;
};
