// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008 - TortoiseSVN
// Copyright (C) 2008-2011 - TortoiseGit

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
#define BLAME_COLUMN_VERSION 4

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
	bool valid = (DWORD)CRegDWORD (registryPrefix + _T("Version"), 0xff) == BLAME_COLUMN_VERSION;
	if (valid)
	{
		// read (possibly different) column selection

		selectedStandardColumns = CRegDWORD (registryPrefix, selectedStandardColumns) & ~hideColumns;

		// read user-prop lists

		CString userPropList = CRegString (registryPrefix + _T("UserProps"));
		CString shownUserProps = CRegString (registryPrefix + _T("ShownUserProps"));

		ParseUserPropSettings (userPropList, shownUserProps);

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
	regVersion = BLAME_COLUMN_VERSION;

	// write (possibly different) column selection

	CRegDWORD regStandardColumns (registryPrefix, 0, TRUE);
	regStandardColumns = GetSelectedStandardColumns();

	// write user-prop lists

	CRegString regUserProps (registryPrefix + _T("UserProps"), CString(), TRUE);
	regUserProps = GetUserPropList();

	CRegString regShownUserProps (registryPrefix + _T("ShownUserProps"), CString(), TRUE);
	regShownUserProps = GetShownUserProps();

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

bool ColumnManager::IsUserProp (int column) const
{
	size_t index = static_cast<size_t>(column);
	assert (columns.size() > index);

	return columns[index].index >= GITSLC_USERPROPCOLOFFSET;
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
		width = LVSCW_AUTOSIZE_USEHEADER;

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

	for (; (visiblePosition < columnCount)
			&& !columns[gridColumnOrder[visiblePosition]].visible
			; ++visiblePosition )
	{
	}

	int next = -1;
	if (visiblePosition != columnCount)
	{
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

	int propertyIndex = columns[index].index;
	if (propertyIndex >= GITSLC_USERPROPCOLOFFSET)
		userProps[propertyIndex - GITSLC_USERPROPCOLOFFSET].width = width;

	control->Invalidate  (FALSE);
}

// call these to update the user-prop list
// (will also auto-insert /-remove new list columns)
#if 0
void ColumnManager::UpdateUserPropList
	(const std::vector<FileEntry*>& files)
{
	// collect all user-defined props

	std::set<CString> aggregatedProps;
	for (size_t i = 0, count = files.size(); i < count; ++i)
		files[i]->present_props.GetPropertyNames (aggregatedProps);

	aggregatedProps.erase (_T("svn:needs-lock"));
	itemProps = aggregatedProps;

	// add new ones to the internal list

	std::set<CString> newProps = aggregatedProps;
	for (size_t i = 0, count = userProps.size(); i < count; ++i)
		newProps.erase (userProps[i].name);

	while (newProps.size() + userProps.size()
			> SVNSLC_MAXCOLUMNCOUNT - SVNSLC_USERPROPCOLOFFSET)
		newProps.erase (--newProps.end());

	typedef std::set<CString>::const_iterator CIT;
	for ( CIT iter = newProps.begin(), end = newProps.end()
		; iter != end
		; ++iter)
	{
		int index = static_cast<int>(userProps.size())
				  + SVNSLC_USERPROPCOLOFFSET;
		columnOrder.push_back (index);

		UserProp userProp;
		userProp.name = *iter;
		userProp.width = 0;

		userProps.push_back (userProp);
	}

	// remove unused columns from control.
	// remove used ones from the set of aggregatedProps.

	for (size_t i = columns.size(); i > 0; --i)
		if ((columns[i-1].index >= SVNSLC_USERPROPCOLOFFSET)
			&& (aggregatedProps.erase (GetName ((int)i-1)) == 0))
		{
			// this user-prop has not been set on any item

			if (!columns[i-1].visible)
			{
				control->DeleteColumn (static_cast<int>(i-1));
				columns.erase (columns.begin() + i-1);
			}
		}

	// aggregatedProps now contains new columns only.
	// we can't use newProps here because some props may have been used
	// earlier but were not in the recent list of used props.
	// -> they are neither in columns[] nor in newProps.

	for ( CIT iter = aggregatedProps.begin(), end = aggregatedProps.end()
		; iter != end
		; ++iter)
	{
		// get the logical column index / ID

		int index = -1;
		int width = 0;
		for (size_t i = 0, count = userProps.size(); i < count; ++i)
			if (userProps[i].name == *iter)
			{
				index = static_cast<int>(i) + SVNSLC_USERPROPCOLOFFSET;
				width = userProps[i].width;
				break;
			}

		assert (index != -1);

		// find insertion position

		std::vector<ColumnInfo>::iterator columnIter = columns.begin();
		std::vector<ColumnInfo>::iterator end = columns.end();
		for (; (columnIter != end) && columnIter->index < index; ++columnIter);
		int pos = static_cast<int>(columnIter - columns.begin());

		ColumnInfo column;
		column.index = index;
		column.width = width;
		column.visible = false;

		columns.insert (columnIter, column);

		// update control

		int result = control->InsertColumn (pos, *iter, LVCFMT_LEFT, GetVisibleWidth(pos, false));
		assert (result != -1);
		UNREFERENCED_PARAMETER(result);
	}

	// update column order

	ApplyColumnOrder();
}
#endif
#if 0
void ColumnManager::UpdateRelevance
	( const std::vector<FileEntry*>& files
	, const std::vector<size_t>& visibleFiles)
{
	// collect all user-defined props that belong to shown files

	std::set<CString> aggregatedProps;
	for (size_t i = 0, count = visibleFiles.size(); i < count; ++i)
		files[visibleFiles[i]]->present_props.GetPropertyNames (aggregatedProps);

	aggregatedProps.erase (_T("svn:needs-lock"));
	itemProps = aggregatedProps;

	// invisible columns for unused props are not relevant

	for (int i = 0, count = GetColumnCount(); i < count; ++i)
		if (IsUserProp(i) && !IsVisible(i))
		{
			columns[i].relevant
				= aggregatedProps.find (GetName(i)) != aggregatedProps.end();
		}

}
#endif
// don't clutter the context menu with irrelevant prop info

bool ColumnManager::AnyUnusedProperties() const
{
	return columns.size() < userProps.size() + itemName.size();
}

void ColumnManager::RemoveUnusedProps()
{
	// determine what column indexes / IDs to keep.
	// map them onto new IDs (we may delete some IDs in between)

	std::map<int, int> validIndices;
	int userPropID = GITSLC_USERPROPCOLOFFSET;

	for (size_t i = 0, count = columns.size(); i < count; ++i)
	{
		int index = columns[i].index;

		if (itemProps.find (GetName((int)i)) != itemProps.end()
			|| columns[i].visible
			|| index < GITSLC_USERPROPCOLOFFSET)
		{
			validIndices[index] = index < GITSLC_USERPROPCOLOFFSET
								? index
								: userPropID++;
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

	// remove from user props

	for (size_t i = userProps.size(); i > 0; --i)
	{
		int index = static_cast<int>(i)-1 + GITSLC_USERPROPCOLOFFSET;
		if (validIndices.find (index) == validIndices.end())
			userProps.erase (userProps.begin() + i-1);
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

	for (size_t i = 0, count = userProps.size(); i < count; ++i)
		userProps[i].width = 0;

	// update UI

	for (int i = 0, count = GetColumnCount(); i < count; ++i)
		control->SetColumnWidth (i, GetVisibleWidth (i, true));

	ApplyColumnOrder();

	control->Invalidate (FALSE);
}

// initialization utilities

void ColumnManager::ParseUserPropSettings(const CString& userPropList, const CString& shownUserProps)
{
	assert (userProps.empty());

	static CString delimiters (_T(" "));

	// parse list of visible user-props

	std::set<CString> visibles;

	int pos = 0;
	CString name = shownUserProps.Tokenize (delimiters, pos);
	while (!name.IsEmpty())
	{
		visibles.insert (name);
		name = shownUserProps.Tokenize (delimiters, pos);
	}

	// create list of all user-props

	pos = 0;
	name = userPropList.Tokenize (delimiters, pos);
	while (!name.IsEmpty())
	{
		bool visible = visibles.find (name) != visibles.end();

		UserProp newEntry;
		newEntry.name = name;
		newEntry.width = 0;

		userProps.push_back (newEntry);

		// auto-create columns for visible user-props
		// (others may be added later)

		if (visible)
		{
			ColumnInfo newColumn;
			newColumn.width = 0;
			newColumn.visible = true;
			newColumn.relevant = true;
			newColumn.index = static_cast<int>(userProps.size())
							+ GITSLC_USERPROPCOLOFFSET - 1;

			columns.push_back (newColumn);
		}

		name = userPropList.Tokenize (delimiters, pos);
	}
}

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
		else if (i >= GITSLC_USERPROPCOLOFFSET)
		{
			// a user-prop column

			size_t index = static_cast<size_t>(i - GITSLC_USERPROPCOLOFFSET);
			assert (index < userProps.size());
			userProps[index].width = width;

			for (size_t k = 0, count = columns.size(); k < count; ++k)
				if (columns[k].index == i)
					columns[k].width = width;
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

	int limit = static_cast<int>(GITSLC_USERPROPCOLOFFSET + userProps.size());
	for (int i = 0, count = widths.GetLength() / 2; i < count; ++i)
	{
		int index = _tcstol (widths.Mid (i*2, 2), NULL, 16);
		if ((index < itemName.size())
			|| ((index >= GITSLC_USERPROPCOLOFFSET) && (index < limit)))
		{
			alreadyPlaced.insert (index);
			columnOrder.push_back (index);
		}
	}

	// place the remaining colums behind it

	for (int i = 0; i < itemName.size(); ++i)
		if (alreadyPlaced.find (i) == alreadyPlaced.end())
			columnOrder.push_back (i);

	for (int i = GITSLC_USERPROPCOLOFFSET; i < limit; ++i)
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

CString ColumnManager::GetUserPropList() const
{
	CString result;

	for (size_t i = 0, count = userProps.size(); i < count; ++i)
		result += userProps[i].name + _T(' ');

	return result;
}

CString ColumnManager::GetShownUserProps() const
{
	CString result;

	for (size_t i = 0, count = columns.size(); i < count; ++i)
	{
		size_t index = static_cast<size_t>(columns[i].index);
		if (columns[i].visible && (index >= GITSLC_USERPROPCOLOFFSET))
			result += userProps[index - GITSLC_USERPROPCOLOFFSET].name
					+ _T(' ');
	}

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

	// range with no column IDs

	result += CString ('0', 8 * (GITSLC_USERPROPCOLOFFSET - itemName.size()));

	// user-prop columns

	for (size_t i = 0, count = userProps.size(); i < count; ++i)
	{
		_stprintf_s (buf, 10, _T("%08X"), userProps[i].width);
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
	default:
		if ((result == 0) && (sortedColumn > 0))
		{
			// N/A props are "less than" empty props

//			const CString& propName = columnManager->GetName (sortedColumn);

//			bool entry1HasProp = entry1->present_props.HasProperty (propName);
//			bool entry2HasProp = entry2->present_props.HasProperty (propName);

//			if (entry1HasProp)
//			{
//				result = entry2HasProp
//					? entry1->present_props[propName].Compare
//					(entry2->present_props[propName])
//					: 1;
//			}
//			else
//			{
//				result = entry2HasProp ? -1 : 0;
//			}
		}
	} // switch (m_nSortedColumn)
	if (!ascending)
		result = -result;

	return result < 0;
}
