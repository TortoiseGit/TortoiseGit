// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018 - TortoiseGit
// Copyright (C) 2010-2017 - TortoiseSVN

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

// filter utiltiy method
bool CLogDlgFilter::Match(const std::wstring& text) const
{
	// empty text does not match
	if (text.empty())
		return false;

	if (m_patterns.empty())
		return text.find(m_sFilterText) != std::string::npos;

	for (const auto& pattern : m_patterns)
	{
		try
		{
			if (!std::regex_search(text, pattern, std::regex_constants::match_any))
				return false;
		}
		catch (std::exception& /*e*/)
		{
			return false;
		}
	}

	return true;
}

void CLogDlgFilter::GetMatchRanges(std::vector<CHARRANGE>& ranges, CString textUTF16, int offset) const
{
	// normalize to lower case
	if (!m_bCaseSensitive && !textUTF16.IsEmpty())
		textUTF16.MakeLower();

	if (m_patterns.empty())
	{
		ATLASSERT(!m_sFilterText.empty());

		auto toScan = (LPCTSTR)textUTF16;
		auto toFind = m_sFilterText.c_str();
		size_t toFindLength = m_sFilterText.size();
		auto pFound = wcsstr(toScan, toFind);
		while (pFound)
		{
			CHARRANGE range;
			range.cpMin = (LONG)(pFound - toScan) + offset;
			range.cpMax = (LONG)(range.cpMin + toFindLength);
			ranges.push_back(range);
			pFound = wcsstr(pFound + 1, toFind);
		}
	}
	else
	{
		for (auto it = m_patterns.cbegin(); it != m_patterns.cend(); ++it)
		{
			const std::wcregex_iterator end;
			for (std::wcregex_iterator it2((LPCTSTR)textUTF16, (LPCTSTR)textUTF16 + textUTF16.GetLength(), *it); it2 != end; ++it2)
			{
				ptrdiff_t matchposID = it2->position(0);
				CHARRANGE range = { (LONG)(matchposID) + offset, (LONG)(matchposID + (*it2)[0].str().size()) + offset };
				ranges.push_back(range);
			}
		}
	}

	if (ranges.empty())
		return;

	auto begin = ranges.begin();
	auto end = ranges.end();

	std::sort(begin, end, [](const CHARRANGE& lhs, const CHARRANGE& rhs) -> bool { return lhs.cpMin < rhs.cpMin; });

	// once we are at it, merge adjacent and / or overlapping ranges
	auto target = begin;
	for (auto source = begin + 1; source != end; ++source)
	{
		if (target->cpMax < source->cpMin)
			*(++target) = *source;
		else
			target->cpMax = max(target->cpMax, source->cpMax);
	}
	ranges.erase(++target, end);
}

// called to parse a (potentially incorrect) regex spec
bool CLogDlgFilter::ValidateRegexp(const CString& regexp_str, std::vector<std::wregex>& patterns)
{
	try
	{
		std::regex_constants::syntax_option_type type = m_bCaseSensitive ? (std::regex_constants::ECMAScript) : (std::regex_constants::ECMAScript | std::regex_constants::icase);

		auto pat = std::wregex(regexp_str, type);
		patterns.push_back(pat);
		return true;
	}
	catch (std::exception&)
	{
	}

	return false;
}

// construction
CLogDlgFilter::CLogDlgFilter()
	: m_dwAttributeSelector(UINT_MAX)
	, m_bCaseSensitive(false)
	, m_bNegate(false)
{
}

CLogDlgFilter::CLogDlgFilter(const CLogDlgFilter& rhs)
{
	operator=(rhs);
}

CLogDlgFilter::CLogDlgFilter(const CString& filter, bool filterWithRegex, DWORD selectedFilter, bool caseSensitive)
	: m_dwAttributeSelector(selectedFilter)
	, m_bCaseSensitive(caseSensitive)
	, m_bNegate(false)
{
	// decode string matching spec
	CString filterText = filter;

	// if the first char is '!', negate the filter
	if (filter.GetLength() && filter[0] == L'!')
	{
		m_bNegate = true;
		filterText = filterText.Mid(1);
	}

	bool useRegex = filterWithRegex && !filterText.IsEmpty();

	if (useRegex)
		useRegex = ValidateRegexp(filterText, m_patterns);
	else if (!caseSensitive)
		m_sFilterText = filterText.MakeLower();
	else
		m_sFilterText = filterText;
}

bool CLogDlgFilter::operator()(GitRevLoglist* pRev, CGitLogListBase* loglist) const
{
	if (m_patterns.empty() && m_sFilterText.empty())
		return !m_bNegate;

	// we need to perform expensive string / pattern matching
	scratch.clear();
	if (m_dwAttributeSelector & (LOGFILTER_SUBJECT | LOGFILTER_MESSAGES))
	{
		scratch += pRev->GetSubject();
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_MESSAGES)
	{
		scratch += pRev->GetBody();
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_BUGID)
	{
		scratch += loglist->m_ProjectProperties.FindBugID(pRev->GetSubjectBody());
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_AUTHORS)
	{
		scratch += pRev->GetAuthorName();
		scratch += L'\n';
		scratch += pRev->GetCommitterName();
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_EMAILS)
	{
		scratch += pRev->GetAuthorEmail();
		scratch += L'\n';
		scratch += pRev->GetCommitterEmail();
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_REVS)
	{
		scratch += pRev->m_CommitHash.ToString();
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_NOTES)
	{
		scratch += pRev->m_Notes;
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_REFNAME)
	{
		auto refList = loglist->m_HashMap.find(pRev->m_CommitHash);
		if (refList != loglist->m_HashMap.cend())
		{
			for (const auto& ref : (*refList).second)
			{
				scratch += ref;
				scratch += L'\n';
			}
		}
	}
	if (m_dwAttributeSelector & LOGFILTER_ANNOTATEDTAG)
	{
		scratch += loglist->GetTagInfo(pRev);
		scratch += L'\n';
	}
	if (m_dwAttributeSelector & LOGFILTER_PATHS)
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

	if (!m_bCaseSensitive)
		_wcslwr_s(&scratch.at(0), scratch.size() + 1); // make sure the \0 at the end is included

	return Match(scratch) ^ m_bNegate;
}

CLogDlgFilter& CLogDlgFilter::operator=(const CLogDlgFilter& rhs)
{
	if (this != &rhs)
	{
		m_dwAttributeSelector = rhs.m_dwAttributeSelector;
		m_bCaseSensitive = rhs.m_bCaseSensitive;
		m_bNegate = rhs.m_bNegate;

		m_sFilterText = rhs.m_sFilterText;
		m_patterns = rhs.m_patterns;

		scratch.clear();
	}

	return *this;
}

bool CLogDlgFilter::operator==(const CLogDlgFilter& rhs) const
{
	if (this == &rhs)
		return true;

	// if we are using regexes we cannot say whether filters are equal
	if (!m_patterns.empty() || !rhs.m_patterns.empty())
		return false;

	// compare global flags
	if (m_bNegate != rhs.m_bNegate || m_dwAttributeSelector != rhs.m_dwAttributeSelector || m_bCaseSensitive != rhs.m_bCaseSensitive)
		return false;

	// compare sub-string defs
	if (m_sFilterText != rhs.m_sFilterText)
		return false;

	// no difference detected
	return true;
}

bool CLogDlgFilter::operator!=(const CLogDlgFilter& rhs) const
{
	return !operator==(rhs);
}

bool CLogDlgFilter::IsFilterActive() const
{
	return !(m_patterns.empty() && m_sFilterText.empty());
}
