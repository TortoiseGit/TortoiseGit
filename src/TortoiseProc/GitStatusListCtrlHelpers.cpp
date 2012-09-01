// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN
// Copyright (C) 2008-2012 - TortoiseGit

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
#include ".\resource.h"
#include "GitStatusListCtrl.h"
#include <iterator>

// registry version number of column-settings of GitLogListBase
#define GITSLC_COL_VERSION 5

#ifndef assert
#define assert(x) ATLASSERT(x)
#endif
// assign property list
#if 0
PropertyList&
PropertyList::operator= (const char* rhs)
{
	// do you really want to replace the property list?

	assert (properties.empty());
	properties.clear();

	// add all properties in the list

	while ((rhs != NULL) && (*rhs != 0))
	{
		const char* next = strchr (rhs, ' ');

		CString name (rhs, static_cast<int>(next == NULL ? strlen (rhs) : next - rhs));
		properties.insert (std::make_pair (name, CString()));

		rhs = next == NULL ? NULL : next+1;
	}

	// done

	return *this;
}

// collect property names in a set

void PropertyList::GetPropertyNames (std::set<CString>& names)
{
	for ( CIT iter = properties.begin(), end = properties.end()
		; iter != end
		; ++iter)
	{
		names.insert (iter->first);
	}
}

// get a property value.

CString PropertyList::operator[](const CString& name) const
{
	CIT iter = properties.find (name);

	return iter == properties.end()
		? CString()
		: iter->second;
}

// set a property value.

CString& PropertyList::operator[](const CString& name)
{
	return properties[name];
}

/// check whether that property has been set on this item.

bool PropertyList::HasProperty (const CString& name) const
{
	return properties.find (name) != properties.end();
}

// due to frequent use: special check for svn:needs-lock

bool PropertyList::IsNeedsLockSet() const
{
	static const CString svnNeedsLock = _T("svn:needs-lock");
	return HasProperty (svnNeedsLock);
}

#endif
// registry access

void ColumnManager::ReadSettings
	( DWORD defaultColumns
	, DWORD hideColumns
	, const CString& containerName
	, int maxsize
	, int * widthlist)
{
	// defaults
	DWORD selectedStandardColumns = defaultColumns & ~hideColumns;
	m_dwDefaultColumns = defaultColumns & ~hideColumns;

	columns.resize (maxsize);
	int power = 1;
	for (size_t i = 0; i < maxsize; ++i)
	{
		columns[i].index = static_cast<int>(i);
		if(widthlist==NULL)
			columns[i].width = 0;
		else
			columns[i].width = widthlist[i];
		columns[i].visible = true;
		columns[i].relevant = !(hideColumns & power);
		power *= 2;
	}

//	userProps.clear();

	// where the settings are stored within the registry

	registryPrefix = _T("Software\\TortoiseGit\\StatusColumns\\") + containerName;

	// we accept settings of current version only
	bool valid = (DWORD)CRegDWORD (registryPrefix + _T("Version"), 0xff) == GITSLC_COL_VERSION;
	if (valid)
	{
		// read (possibly different) column selection

		selectedStandardColumns = CRegDWORD (registryPrefix, selectedStandardColumns) & ~hideColumns;

		// read column widths

		CString colWidths = CRegString (registryPrefix + _T("_Width"));

		ParseWidths (colWidths);
	}

	// process old-style visibility setting

	SetStandardColumnVisibility (selectedStandardColumns);

	// clear all previously set header columns

	int c = ((CHeaderCtrl*)(control->GetDlgItem(0)))->GetItemCount()-1;
	while (c>=0)
		control->DeleteColumn(c--);

	// create columns

	for (int i = 0, count = GetColumnCount(); i < count; ++i)
		control->InsertColumn (i, GetName(i), LVCFMT_LEFT, IsVisible(i)&&IsRelevant(i) ? -1 : GetVisibleWidth(i, false));

	// restore column ordering

	if (valid)
		ParseColumnOrder (CRegString (registryPrefix + _T("_Order")));
	else
		ParseColumnOrder (CString());

	ApplyColumnOrder();

	// auto-size the columns so we can see them while fetching status
	// (seems the same values will not take affect in InsertColumn)

	for (int i = 0, count = GetColumnCount(); i < count; ++i)
		if (IsVisible(i))
			control->SetColumnWidth (i, GetVisibleWidth (i, true));
}

void ColumnManager::WriteSettings() const
{
	CRegDWORD regVersion (registryPrefix + _T("Version"), 0, TRUE);
	regVersion = GITSLC_COL_VERSION;

	// write (possibly different) column selection

	CRegDWORD regStandardColumns (registryPrefix, 0, TRUE);
	regStandardColumns = GetSelectedStandardColumns();

	// write column widths

	CRegString regWidths (registryPrefix + _T("_Width"), CString(), TRUE);
	regWidths = GetWidthString();

	// write column ordering

	CRegString regColumnOrder (registryPrefix + _T("_Order"), CString(), TRUE);
	regColumnOrder = GetColumnOrderString();
}

// read column definitions

int ColumnManager::GetColumnCount() const
{
	return static_cast<int>(columns.size());
}

bool ColumnManager::IsVisible (int column) const
{
	size_t index = static_cast<size_t>(column);
	assert (columns.size() > index);

	return columns[index].visible;
}

int ColumnManager::GetInvisibleCount() const
{
	int invisibleCount = 0;
	for (std::vector<ColumnInfo>::const_iterator it = columns.begin(); it != columns.end(); ++it)
	{
		if (!it->visible)
			invisibleCount++;
	}
	return invisibleCount;
}

bool ColumnManager::IsRelevant (int column) const
{
	size_t index = static_cast<size_t>(column);
	assert (columns.size() > index);

	return columns[index].relevant;
}

int ColumnManager::SetNames(UINT* buffer, int size)
{
	itemName.clear();
	for(int i=0;i<size;i++)
		itemName.push_back(*buffer++);
	return 0;
}

CString ColumnManager::GetName (int column) const
{
	// standard columns
	size_t index = static_cast<size_t>(column);
	if (index < itemName.size())
	{
		CString result;
		result.LoadString (itemName[index]);
		return result;
	}

	// user-prop columns

	//	if (index < columns.size())
	//		return userProps[columns[index].index - SVNSLC_USERPROPCOLOFFSET].name;

	// default: empty

	return CString();
}

int ColumnManager::GetWidth (int column, bool useDefaults) const
{
	size_t index = static_cast<size_t>(column);
	assert (columns.size() > index);

	int width = columns[index].width;
	if ((width == 0) && useDefaults)
		width = LVSCW_AUTOSIZE;

	return width;
}

int ColumnManager::GetVisibleWidth (int column, bool useDefaults) const
{
	return IsVisible (column)
		? GetWidth (column, useDefaults)
		: 0;
}

// switch columns on and off

void ColumnManager::SetVisible
	( int column
	, bool visible)
{
	size_t index = static_cast<size_t>(column);
	assert (index < columns.size());

	if (columns[index].visible != visible)
	{
		columns[index].visible = visible;
		columns[index].relevant |= visible;
		if (!visible)
			columns[index].width = 0;

		control->SetColumnWidth (column, GetVisibleWidth (column, true));
		ApplyColumnOrder();

		control->Invalidate (FALSE);
	}
}

// tracking column modifications

void ColumnManager::ColumnMoved (int column, int position)
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
		gridColumnOrder.erase(std::find(gridColumnOrder.begin(), gridColumnOrder.end(), index));
		next = gridColumnOrder[visiblePosition];
	}

	// move logical column index just in front of that "next" column

	columnOrder.erase (std::find ( columnOrder.begin(), columnOrder.end(), index));
	columnOrder.insert ( std::find ( columnOrder.begin(), columnOrder.end(), next), index);

	// make sure, invisible columns are still put in front of all others

	ApplyColumnOrder();
}

void ColumnManager::ColumnResized (int column)
{
	size_t index = static_cast<size_t>(column);
	assert (index < columns.size());
	assert (columns[index].visible);

	int width = control->GetColumnWidth (column);
	columns[index].width = width;

	control->Invalidate  (FALSE);
}

void ColumnManager::RemoveUnusedProps()
{
	// determine what column indexes / IDs to keep.
	// map them onto new IDs (we may delete some IDs in between)

	std::map<int, int> validIndices;

	for (size_t i = 0, count = columns.size(); i < count; ++i)
	{
		int index = columns[i].index;

		if (itemProps.find (GetName((int)i)) != itemProps.end()
			|| columns[i].visible)
		{
			validIndices[index] = index;
		}
	}

	// remove everything else:

	// remove from columns and control.
	// also update index values in columns

	for (size_t i = columns.size(); i > 0; --i)
	{
		std::map<int, int>::const_iterator iter
			= validIndices.find (columns[i-1].index);

		if (iter == validIndices.end())
		{
			control->DeleteColumn (static_cast<int>(i-1));
			columns.erase (columns.begin() + i-1);
		}
		else
		{
			columns[i-1].index = iter->second;
		}
	}

	// remove from and update column order

	for (size_t i = columnOrder.size(); i > 0; --i)
	{
		std::map<int, int>::const_iterator iter
			= validIndices.find (columnOrder[i-1]);

		if (iter == validIndices.end())
			columnOrder.erase (columnOrder.begin() + i-1);
		else
			columnOrder[i-1] = iter->second;
	}
}

// bring everything back to its "natural" order

void ColumnManager::ResetColumns (DWORD defaultColumns)
{
	// update internal data

	std::sort (columnOrder.begin(), columnOrder.end());

	for (size_t i = 0, count = columns.size(); i < count; ++i)
	{
		columns[i].width = 0;
		columns[i].visible = (i < 32) && (((defaultColumns >> i) & 1) != 0);
	}

	// update UI

	for (int i = 0, count = GetColumnCount(); i < count; ++i)
		control->SetColumnWidth (i, GetVisibleWidth (i, true));

	ApplyColumnOrder();

	control->Invalidate (FALSE);
}

// initialization utilities

void ColumnManager::ParseWidths (const CString& widths)
{
	for (int i = 0, count = widths.GetLength() / 8; i < count; ++i)
	{
		long width = _tcstol (widths.Mid (i*8, 8), NULL, 16);
		if (i < itemName.size())
		{
			// a standard column

			columns[i].width = width;
		}
		else
		{
			// there is no such column

			assert (width == 0);
		}
	}
}

void ColumnManager::SetStandardColumnVisibility
	(DWORD visibility)
{
	for (size_t i = 0; i < itemName.size(); ++i)
	{
		columns[i].visible = (visibility & 1) > 0;
		visibility /= 2;
	}
}

void ColumnManager::ParseColumnOrder
	(const CString& widths)
{
	std::set<int> alreadyPlaced;
	columnOrder.clear();

	// place columns according to valid entries in orderString

	for (int i = 0, count = widths.GetLength() / 2; i < count; ++i)
	{
		int index = _tcstol (widths.Mid (i*2, 2), NULL, 16);
		if ((index < itemName.size()))
		{
			alreadyPlaced.insert (index);
			columnOrder.push_back (index);
		}
	}

	// place the remaining colums behind it

	for (int i = 0; i < itemName.size(); ++i)
		if (alreadyPlaced.find (i) == alreadyPlaced.end())
			columnOrder.push_back (i);
}

// map internal column order onto visible column order
// (all invisibles in front)

std::vector<int> ColumnManager::GetGridColumnOrder()
{
	// extract order of used columns from order of all columns

	std::vector<int> result;
	result.reserve (GITSLC_MAXCOLUMNCOUNT+1);

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
					result.push_back (static_cast<int>(k));
			}
		}

		visible = !visible;
	}
	while (visible);

	return result;
}

void ColumnManager::ApplyColumnOrder()
{
	// extract order of used columns from order of all columns

	int order[GITSLC_MAXCOLUMNCOUNT+1];
	SecureZeroMemory (order, sizeof (order));

	std::vector<int> gridColumnOrder = GetGridColumnOrder();
	std::copy (gridColumnOrder.begin(), gridColumnOrder.end(), stdext::checked_array_iterator<int*>(&order[0], sizeof(order)));

	// we must have placed all columns or something is really fishy ..

	assert (gridColumnOrder.size() == columns.size());
	assert (GetColumnCount() == ((CHeaderCtrl*)(control->GetDlgItem(0)))->GetItemCount());

	// o.k., apply our column ordering

	control->SetColumnOrderArray (GetColumnCount(), order);
}

// utilities used when writing data to the registry

DWORD ColumnManager::GetSelectedStandardColumns() const
{
	DWORD result = 0;
	for (size_t i = itemName.size(); i > 0; --i)
		result = result * 2 + (columns[i-1].visible ? 1 : 0);

	return result;
}

CString ColumnManager::GetWidthString() const
{
	CString result;

	// regular columns

	TCHAR buf[10];
	for (size_t i = 0; i < itemName.size(); ++i)
	{
		_stprintf_s (buf, 10, _T("%08X"), columns[i].width);
		result += buf;
	}

	return result;
}

CString ColumnManager::GetColumnOrderString() const
{
	CString result;

	TCHAR buf[3];
	for (size_t i = 0, count = columnOrder.size(); i < count; ++i)
	{
		_stprintf_s (buf, 3, _T("%02X"), columnOrder[i]);
		result += buf;
	}

	return result;
}

// sorter utility class, only used by GitStatusList!

CSorter::CSorter ( ColumnManager* columnManager
									, int sortedColumn
									, bool ascending)
									: columnManager (columnManager)
									, sortedColumn (sortedColumn)
									, ascending (ascending)
{
}

bool CSorter::operator() (const CTGitPath* entry1 , const CTGitPath* entry2) const
{
#define SGN(x) ((x)==0?0:((x)>0?1:-1))

	int result = 0;
	switch (sortedColumn)
	{
	case 7: // File size
		{
			if (result == 0)
			{
				__int64 fileSize1 = entry1->IsDirectory() ? 0 : entry1->GetFileSize();
				__int64 fileSize2 = entry2->IsDirectory() ? 0 : entry2->GetFileSize();
				
				result = int(fileSize1 - fileSize2);
			}
		}
	case 6: //Last Modification Date
		{
			if (result == 0)
			{
				__int64 writetime1 = entry1->GetLastWriteTime();
				__int64 writetime2 = entry2->GetLastWriteTime();

				FILETIME* filetime1 = (FILETIME*)(__int64*)&writetime1;
				FILETIME* filetime2 = (FILETIME*)(__int64*)&writetime2;

				result = CompareFileTime(filetime1, filetime2);
			}
		}
	case 5: //Del Number
		{
			if (result == 0)
			{
//				result = entry1->lock_comment.CompareNoCase(entry2->lock_comment);
				result = A2L(entry1->m_StatDel)-A2L(entry2->m_StatDel);
			}
		}
	case 4: //Add Number
		{
			if (result == 0)
			{
				//result = entry1->lock_owner.CompareNoCase(entry2->lock_owner);
				result = A2L(entry1->m_StatAdd)-A2L(entry2->m_StatAdd);
			}
		}

	case 3: // Status
		{
			if (result == 0)
			{
				result = entry1->GetActionName(entry1->m_Action).CompareNoCase(entry2->GetActionName(entry2->m_Action));
			}
		}
	case 2: //Ext file
		{
			if (result == 0)
			{
				result = entry1->GetFileExtension().CompareNoCase(entry2->GetFileExtension());
			}
		}
	case 1: // File name
		{
			if (result == 0)
			{
				result = entry1->GetFileOrDirectoryName().CompareNoCase(entry2->GetFileOrDirectoryName());
			}
		}
	case 0: // Full path column
		{
			if (result == 0)
			{
				result = CTGitPath::Compare(entry1->GetGitPathString(), entry2->GetGitPathString());
			}
		}
	} // switch (m_nSortedColumn)
	if (!ascending)
		result = -result;

	return result < 0;
}
