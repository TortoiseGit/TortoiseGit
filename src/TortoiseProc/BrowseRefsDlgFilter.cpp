// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018-2019 - TortoiseGit

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
#include "BrowseRefsDlgFilter.h"
#include "BrowseRefsDlg.h"
#include "GitLogListBase.h"

bool CBrowseRefsDlgFilter::operator()(const CShadowTree* pTree, const CString& ref) const
{
	if (!IsFilterActive())
		return CalculateFinalResult(true);

	// we need to perform expensive string / pattern matching
	scratch.clear();

	if (GetSelectedFilters() & LOGFILTER_REFNAME)
	{
		scratch += ref;
		scratch += L'\n';
	}
	if (GetSelectedFilters() & LOGFILTER_SUBJECT)
	{
		scratch += pTree->m_csSubject;
		scratch += L'\n';
	}

	if (GetSelectedFilters() & LOGFILTER_AUTHORS)
	{
		scratch += pTree->m_csAuthor;
		scratch += L'\n';
	}

	if (GetSelectedFilters() & LOGFILTER_REVS)
	{
		scratch += pTree->m_csRefHash;
		scratch += L'\n';
	}

	return CalculateFinalResult(Match(scratch));
}
