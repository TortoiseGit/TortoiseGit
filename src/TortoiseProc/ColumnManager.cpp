// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
// Copyright (C) 2008, 2014 - TortoiseSVN

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
#include "ColumnManager.h"
#include "LoglistCommonResource.h"
#include <iterator>

// registry version number of column-settings of both GitLogListBase and GitStatusListCtrl
#define GITSLC_COL_VERSION 6
#define MAX_COLUMNS 0xff

#ifndef assert
#define assert(x) ATLASSERT(x)
#endif
// assign property list
#if 0
PropertyList&
PropertyList::operator= (const char* rhs)
{
	// do you really want to replace the property list?
	assert(properties.empty());
	properties.clear();

	// add all properties in the list
	while (rhs && *rhs)
	{
		const char* next = strchr(rhs, ' ');

		CString name(rhs, static_cast<int>(!next ? strlen(rhs) : next - rhs));
		properties.insert(std::make_pair(name, CString()));

		rhs = !next ? nullptr : next + 1;
	}

	// done
	return *this;
}

// collect property names in a set

void PropertyList::GetPropertyNames(std::set<CString>& names)
{
	for (CIT iter = properties.cbegin(), end = properties.cend(); iter != end; ++iter)
		names.insert(iter->first);
}

// get a property value.

CString PropertyList::operator[](const CString& name) const
{
	CIT iter = properties.find(name);
	return iter == properties.end() ? CString() : iter->second;
}

// set a property value.

CString& PropertyList::operator[](const CString& name)
{
	return properties[name];
}

/// check whether that property has been set on this item.

bool PropertyList::HasProperty(const CString& name) const
{
	return properties.find(name) != properties.end();
}
#endif
// registry access

void ColumnManager::ReadSettings(DWORD defaultColumns, DWORD hideColumns, const CString& containerName, int maxsize, int* widthlist)
{
	// defaults
	DWORD selectedStandardColumns = defaultColumns & ~hideColumns;
	m_dwDefaultColumns = defaultColumns & ~hideColumns;

	columns.resize(maxsize);
	int power = 1;
	for (int i = 0; i < maxsize; ++i)
	{
		columns[i].index = static_cast<int>(i);
		if (!widthlist)
			columns[i].width = 0;
		else
			columns[i].width = widthlist[i];
		columns[i].visible = true;
		columns[i].relevant = !(hideColumns & power);
		columns[i].adjusted = false;
		power *= 2;
	}

	//	userProps.clear();

	// where the settings are stored within the registry
	registryPrefix = L"Software\\TortoiseGit\\StatusColumns\\" + containerName;

	// we accept settings of current version only
	bool valid = static_cast<DWORD>(CRegDWORD(registryPrefix + L"Version", 0xff)) == GITSLC_COL_VERSION;
	if (valid)
	{
		// read (possibly different) column selection
		selectedStandardColumns = CRegDWORD(registryPrefix, selectedStandardColumns) & ~hideColumns;

		// read column widths
		CString colWidths = CRegString(registryPrefix + L"_Width");

		ParseWidths(colWidths);
	}

	// process old-style visibility setting
	SetStandardColumnVisibility(selectedStandardColumns);

	// clear all previously set header columns
	int c = control->GetHeaderCtrl()->GetItemCount() - 1;
	while (c >= 0)
		control->DeleteColumn(c--);

	// create columns
	for (int i = 0, count = GetColumnCount(); i < count; ++i)
		control->InsertColumn(i, GetName(i), LVCFMT_LEFT, IsVisible(i) && IsRelevant(i) ? -1 : GetVisibleWidth(i, false));

	// restore column ordering
	if (valid)
		ParseColumnOrder(CRegString(registryPrefix + L"_Order"));
	else
		ParseColumnOrder(CString());

	ApplyColumnOrder();

	// auto-size the columns so we can see them while fetching status
	// (seems the same values will not take affect in InsertColumn)
	for (int i = 0, count = GetColumnCount(); i < count; ++i)
	{
		if (IsVisible(i))
			control->SetColumnWidth(i, GetVisibleWidth(i, true));
	}
}

void ColumnManager::WriteSettings() const
{
	CRegDWORD regVersion(registryPrefix + L"Version", 0, TRUE);
	regVersion = GITSLC_COL_VERSION;

	// write (possibly different) column selection
	CRegDWORD regStandardColumns(registryPrefix, 0, TRUE);
	regStandardColumns = GetSelectedStandardColumns();

	// write column widths
	CRegString regWidths(registryPrefix + L"_Width", CString(), TRUE);
	regWidths = GetWidthString();

	// write column ordering
	CRegString regColumnOrder(registryPrefix + L"_Order", CString(), TRUE);
	regColumnOrder = GetColumnOrderString();
}

// read column definitions

int ColumnManager::GetColumnCount() const
{
	return static_cast<int>(columns.size());
}

bool ColumnManager::IsVisible(int column) const
{
	size_t index = static_cast<size_t>(column);
	assert(columns.size() > index);

	return columns[index].visible;
}

int ColumnManager::GetInvisibleCount() const
{
	return static_cast<int>(std::count_if(columns.cbegin(), columns.cend(), [](auto& column) { return !column.visible; }));
}

bool ColumnManager::IsRelevant(int column) const
{
	size_t index = static_cast<size_t>(column);
	assert(columns.size() > index);

	return columns[index].relevant;
}

int ColumnManager::SetNames(UINT* buffer, int size)
{
	itemName.clear();
	for (int i = 0; i < size; ++i)
		itemName.push_back(*buffer++);
	return 0;
}

void ColumnManager::SetRightAlign(int column) const
{
	assert(static_cast<size_t>(column) < columns.size());

	LVCOLUMN col = { 0 };
	col.mask = LVCF_FMT;
	col.fmt = LVCFMT_RIGHT;
	control->SetColumn(column, &col);

	control->Invalidate(FALSE);
}

CString ColumnManager::GetName(int column) const
{
	// standard columns
	size_t index = static_cast<size_t>(column);
	if (index < itemName.size())
	{
		CString result;
		result.LoadString(itemName[index]);
		return result;
	}

	// user-prop columns
	//	if (index < columns.size())
	//		return userProps[columns[index].index - SVNSLC_USERPROPCOLOFFSET].name;

	// default: empty

	return CString();
}

int ColumnManager::GetWidth(int column, bool useDefaults) const
{
	size_t index = static_cast<size_t>(column);
	assert(columns.size() > index);

	int width = columns[index].width;
	if ((width == 0) && useDefaults)
	{
		if (index > 0)
		{
			int cx = control->GetStringWidth(GetName(column)) + 20; // 20 pixels for col separator and margin

			for (int i = 0, itemCnt = control->GetItemCount(); i < itemCnt; ++i)
			{
				// get the width of the string and add 14 pixels for the column separator and margins
				int linewidth = control->GetStringWidth(control->GetItemText(i, column)) + 14;
				if (cx < linewidth)
					cx = linewidth;
			}
			return cx;
		}
		return LVSCW_AUTOSIZE_USEHEADER;
	}
	return width;
}

int ColumnManager::GetVisibleWidth(int column, bool useDefaults) const
{
	return IsVisible(column) ? GetWidth(column, useDefaults) : 0;
}

// switch columns on and off

void ColumnManager::SetVisible(int column, bool visible)
{
	size_t index = static_cast<size_t>(column);
	assert(index < columns.size());

	if (columns[index].visible != visible)
	{
		columns[index].visible = visible;
		columns[index].relevant |= visible;
		if (!visible)
			columns[index].width = 0;

		control->SetColumnWidth(column, GetVisibleWidth(column, true));
		ApplyColumnOrder();

		control->Invalidate(FALSE);
	}
}

// tracking column modifications
void ColumnManager::ColumnMoved(int column, int position)
{
	// in front of what column has it been inserted?
	int index = columns[column].index;

	std::vector<int> gridColumnOrder = GetGridColumnOrder();

	size_t visiblePosition = static_cast<size_t>(position);
	size_t columnCount = gridColumnOrder.size();

	int next = -1;
	if (visiblePosition < columnCount - 1)
	{
		// the new position (visiblePosition) is the column id w/o the moved column
		gridColumnOrder.erase(std::find(gridColumnOrder.cbegin(), gridColumnOrder.cend(), index));
		next = gridColumnOrder[visiblePosition];
	}

	// move logical column index just in front of that "next" column
	columnOrder.erase(std::find(columnOrder.cbegin(), columnOrder.cend(), index));
	columnOrder.insert(std::find(columnOrder.cbegin(), columnOrder.cend(), next), index);

	// make sure, invisible columns are still put in front of all others
	ApplyColumnOrder();
}

void ColumnManager::ColumnResized(int column, int manual)
{
	size_t index = static_cast<size_t>(column);
	assert(index < columns.size());
	assert(columns[index].visible);

	int width = control->GetColumnWidth(column);
	if (manual != 0)
		columns[index].adjusted = (manual < 3);
	if (manual == 2)
	{
		control->SetColumnWidth(column, LVSCW_AUTOSIZE);
		columns[index].width = control->GetColumnWidth(column);
	}
	else if (manual == 3)
	{
		columns[index].width = 0;
		control->SetColumnWidth(column, GetWidth(column, true));
	}
	else
		columns[index].width = width;

	control->Invalidate(FALSE);
}

void ColumnManager::RemoveUnusedProps()
{
	// determine what column indexes / IDs to keep.
	// map them onto new IDs (we may delete some IDs in between)

	std::map<int, int> validIndices;

	for (size_t i = 0, count = columns.size(); i < count; ++i)
	{
		int index = columns[i].index;

		if (itemProps.find(GetName(static_cast<int>(i))) != itemProps.end() || columns[i].visible)
			validIndices[index] = index;
	}

	// remove everything else:

	// remove from columns and control.
	// also update index values in columns

	for (size_t i = columns.size(); i > 0; --i)
	{
		const auto iter = validIndices.find(columns[i - 1].index);

		if (iter == validIndices.cend())
		{
			control->DeleteColumn(static_cast<int>(i - 1));
			columns.erase(columns.cbegin() + i - 1);
		}
		else
			columns[i - 1].index = iter->second;
	}

	// remove from and update column order

	for (size_t i = columnOrder.size(); i > 0; --i)
	{
		const auto iter = validIndices.find(columnOrder[i - 1]);

		if (iter == validIndices.cend())
			columnOrder.erase(columnOrder.cbegin() + i - 1);
		else
			columnOrder[i - 1] = iter->second;
	}
}

// bring everything back to its "natural" order

void ColumnManager::ResetColumns(DWORD defaultColumns)
{
	// update internal data
	std::sort(columnOrder.begin(), columnOrder.end());

	for (size_t i = 0, count = columns.size(); i < count; ++i)
	{
		columns[i].width = 0;
		columns[i].visible = (i < 32) && (((defaultColumns >> i) & 1) != 0);
		columns[i].adjusted = false;
	}

	// update UI
	for (int i = 0, count = GetColumnCount(); i < count; ++i)
		control->SetColumnWidth(i, GetVisibleWidth(i, true));

	ApplyColumnOrder();

	control->Invalidate(FALSE);
}

// initialization utilities
void ColumnManager::ParseWidths(const CString& widths)
{
	for (int i = 0, count = widths.GetLength() / 8; i < count; ++i)
	{
		long width = wcstol(widths.Mid(i * 8, 8), nullptr, 16);
		if (i < static_cast<int>(itemName.size()))
		{
			// a standard column
			if (width != MAXLONG)
			{
				columns[i].width = width;
				columns[i].adjusted = true;
			}
		}
		else
		{
			// there is no such column
			assert(width == 0);
		}
	}
}

void ColumnManager::SetStandardColumnVisibility(DWORD visibility)
{
	for (size_t i = 0; i < itemName.size(); ++i)
	{
		columns[i].visible = (visibility & 1) > 0;
		visibility /= 2;
	}
}

void ColumnManager::ParseColumnOrder(const CString& widths)
{
	std::set<int> alreadyPlaced;
	columnOrder.clear();

	// place columns according to valid entries in orderString

	for (int i = 0, count = widths.GetLength() / 2; i < count; ++i)
	{
		int index = wcstol(widths.Mid(i * 2, 2), nullptr, 16);
		if ((index < static_cast<int>(itemName.size())))
		{
			alreadyPlaced.insert(index);
			columnOrder.push_back(index);
		}
	}

	// place the remaining colums behind it
	for (int i = 0; i < static_cast<int>(itemName.size()); ++i)
		if (alreadyPlaced.find(i) == alreadyPlaced.end())
			columnOrder.push_back(i);
}

// map internal column order onto visible column order
// (all invisibles in front)
std::vector<int> ColumnManager::GetGridColumnOrder() const
{
	// extract order of used columns from order of all columns

	std::vector<int> result;
	result.reserve(MAX_COLUMNS + 1);

	size_t colCount = columns.size();
	bool visible = false;

	do
	{
		// put invisible cols in front

		for (size_t i = 0, count = columnOrder.size(); i < count; ++i)
		{
			int index = columnOrder[i];
			for (size_t k = 0; k < colCount; ++k)
			{
				const ColumnInfo& column = columns[k];
				if ((column.index == index) && (column.visible == visible))
					result.push_back(static_cast<int>(k));
			}
		}

		visible = !visible;
	} while (visible);

	return result;
}

void ColumnManager::ApplyColumnOrder()
{
	// extract order of used columns from order of all columns
	int order[MAX_COLUMNS + 1] = { 0 };

	std::vector<int> gridColumnOrder = GetGridColumnOrder();
	std::copy(gridColumnOrder.cbegin(), gridColumnOrder.cend(), stdext::checked_array_iterator<int*>(&order[0], sizeof(order)));

	// we must have placed all columns or something is really fishy ..
	assert(gridColumnOrder.size() == columns.size());
	assert(GetColumnCount() == control->GetHeaderCtrl()->GetItemCount());

	// o.k., apply our column ordering
	control->SetColumnOrderArray(GetColumnCount(), order);
}

// utilities used when writing data to the registry

DWORD ColumnManager::GetSelectedStandardColumns() const
{
	DWORD result = 0;
	for (size_t i = itemName.size(); i > 0; --i)
		result = result * 2 + (columns[i - 1].visible ? 1 : 0);

	return result;
}

CString ColumnManager::GetWidthString() const
{
	CString result;

	// regular columns
	TCHAR buf[10] = { 0 };
	for (size_t i = 0; i < itemName.size(); ++i)
	{
		_stprintf_s(buf, L"%08X", columns[i].adjusted ? columns[i].width : MAXLONG);
		result += buf;
	}

	return result;
}

CString ColumnManager::GetColumnOrderString() const
{
	CString result;

	TCHAR buf[3] = { 0 };
	for (size_t i = 0, count = columnOrder.size(); i < count; ++i)
	{
		_stprintf_s(buf, L"%02X", columnOrder[i]);
		result += buf;
	}

	return result;
}

void ColumnManager::OnContextMenuHeader(CWnd* pWnd, CPoint point, bool isGroundEnable /* = false*/)
{
	if ((point.x == -1) && (point.y == -1))
	{
		auto pHeaderCtrl = static_cast<CHeaderCtrl*>(pWnd);
		CRect rect;
		pHeaderCtrl->GetItemRect(0, &rect);
		pHeaderCtrl->ClientToScreen(&rect);
		point = rect.CenterPoint();
	}

	CMenu popup;
	if (popup.CreatePopupMenu())
	{
		int columnCount = GetColumnCount();

		CString temp;
		UINT uCheckedFlags = MF_STRING | MF_ENABLED | MF_CHECKED;
		UINT uUnCheckedFlags = MF_STRING | MF_ENABLED;

		// build control menu

		//temp.LoadString(IDS_STATUSLIST_SHOWGROUPS);
		//popup.AppendMenu(isGroundEnable? uCheckedFlags : uUnCheckedFlags, columnCount, temp);

		temp.LoadString(IDS_STATUSLIST_RESETCOLUMNORDER);
		popup.AppendMenu(uUnCheckedFlags, columnCount + 2, temp);
		popup.AppendMenu(MF_SEPARATOR);

		// standard columns
		AddMenuItem(&popup);

		// user-prop columns:
		// find relevant ones and sort 'em

		std::map<CString, int> sortedProps;
		for (int i = static_cast<int>(itemName.size()); i < columnCount; ++i)
			if (IsRelevant(i))
				sortedProps[GetName(i)] = i;

		if (!sortedProps.empty())
		{
			// add 'em to the menu

			popup.AppendMenu(MF_SEPARATOR);

			for (auto iter = sortedProps.cbegin(), end = sortedProps.cend(); iter != end; ++iter)
			{
				popup.AppendMenu(IsVisible(iter->second)
					? uCheckedFlags
					: uUnCheckedFlags
					, iter->second
					, iter->first);
			}
		}

		// show menu & let user pick an entry

		int cmd = popup.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_NONOTIFY, point.x, point.y, pWnd);
		if ((cmd >= 1) && (cmd < columnCount))
			SetVisible(cmd, !IsVisible(cmd));
		else if (cmd == columnCount)
		{
			pWnd->GetParent()->SendMessage(LVM_ENABLEGROUPVIEW, !isGroundEnable, NULL);
			//EnableGroupView(!isGroundEnable);
		}
		else if (cmd == columnCount + 1)
			RemoveUnusedProps();
		else if (cmd == columnCount + 2)
		{
			temp.LoadString(IDS_CONFIRMRESETCOLUMNORDER);
			if (MessageBox(pWnd->m_hWnd, temp, L"TortoiseGit", MB_YESNO | MB_ICONQUESTION) == IDYES)
				ResetColumns(m_dwDefaultColumns);
		}
	}
}
