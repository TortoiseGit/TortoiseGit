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
#include "LogDlgFilter.h"
#include "LogDlg.h"

bool CLogDlgFilter::operator()(GitRevLoglist* pRev, CGitLogListBase* loglist, const MAP_HASH_NAME& hashMapRefs) const
{
	if (!IsFilterActive())
		return CalculateFinalResult(true);

	// we need to perform expensive string / pattern matching
	scratch.clear();
	if (GetSelectedFilters() & (LOGFILTER_SUBJECT | LOGFILTER_MESSAGES))
	{
		scratch += pRev->GetSubject();
		scratch += L'\n';
	}
	if (GetSelectedFilters() & LOGFILTER_MESSAGES)
	{
		scratch += pRev->GetBody();
		scratch += L'\n';
	}
	if (GetSelectedFilters() & LOGFILTER_BUGID)
	{
		scratch += loglist->m_ProjectProperties.FindBugID(pRev->GetSubjectBody());
		scratch += L'\n';
	}
	if (GetSelectedFilters() & LOGFILTER_AUTHORS)
	{
		scratch += pRev->GetAuthorName();
		scratch += L'\n';
		scratch += pRev->GetCommitterName();
		scratch += L'\n';
	}
	if (GetSelectedFilters() & LOGFILTER_EMAILS)
	{
		scratch += pRev->GetAuthorEmail();
		scratch += L'\n';
		scratch += pRev->GetCommitterEmail();
		scratch += L'\n';
	}
	if (GetSelectedFilters() & LOGFILTER_REVS)
	{
		scratch += pRev->m_CommitHash.ToString();
		scratch += L'\n';
	}
	if (GetSelectedFilters() & LOGFILTER_NOTES)
	{
		scratch += pRev->m_Notes;
		scratch += L'\n';
	}
	if (GetSelectedFilters() & (LOGFILTER_REFNAME | LOGFILTER_ANNOTATEDTAG))
	{
		auto refList = hashMapRefs.find(pRev->m_CommitHash);
		if (refList != hashMapRefs.cend())
		{
			if (GetSelectedFilters() & LOGFILTER_REFNAME)
			{
				for (const auto& ref : (*refList).second)
				{
					scratch += ref;
					scratch += L'\n';
				}
			}
			if (GetSelectedFilters() & LOGFILTER_ANNOTATEDTAG)
			{
				scratch += loglist->GetTagInfo((*refList).second);
				scratch += L'\n';
			}
		}
	}
	if (GetSelectedFilters() & LOGFILTER_PATHS)
	{
		/* Because changed files list is loaded on demand when gui show, files will empty when files have not fetched.
		   we can add it back by using one-way diff(with outnumber changed and rename detect.
		   here just need changed filename list. one-way is much quicker.
		 */
		if (pRev->m_IsFull)
		{
			const auto& pathList = pRev->GetFiles(loglist);
			for (int i = 0; i < pathList.GetCount(); ++i)
			{
				scratch += pathList[i].GetWinPath();
				scratch += L'|';
				scratch += pathList[i].GetGitOldPathString();
				scratch += L'\n';
			}
		}
		else
		{
			if (!pRev->m_IsSimpleListReady)
				pRev->SafeGetSimpleList(&g_Git);

			for (size_t i = 0; i < pRev->m_SimpleFileList.size(); ++i)
			{
				scratch += pRev->m_SimpleFileList[i];
				scratch += L'\n';
			}
		}
	}

	return CalculateFinalResult(Match(scratch));
}
