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
	{
		// require all strings to be present
		bool current_value = true;
		for (size_t i = 0, count = subStringConditions.size(); i < count; ++i)
		{
			const SCondition& condition = subStringConditions[i];
			bool found = text.find(condition.subString) != std::wstring::npos;
			switch (condition.prefix)
			{
			case and_not:
				found = !found;
				// fallthrough
			case and:
				if (!found)
				{
					// not a match, so skip to the next "+"-prefixed item
					if (condition.nextOrIndex == 0)
						return false;

					current_value = false;
					i = condition.nextOrIndex - 1;
				}
				break;

			case or:
				current_value |= found;
				if (!current_value)
				{
					// not a match, so skip to the next "+"-prefixed item
					if (condition.nextOrIndex == 0)
						return false;

					i = condition.nextOrIndex - 1;
				}
				break;
			}
		}
	}

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
		auto toScan = (LPCTSTR)textUTF16;
		for (auto iter = subStringConditions.cbegin(), end = subStringConditions.cend(); iter != end; ++iter)
		{
			if (iter->prefix == and_not)
				continue;

			auto toFind = iter->subString.c_str();
			size_t toFindLength = iter->subString.size();
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

// construction utility
void CLogDlgFilter::AddSubString(CString token, Prefix prefix)
{
	if (token.IsEmpty())
		return;

	if (!m_bCaseSensitive)
		token.MakeLower();

	// add condition to list
	SCondition condition = { token, prefix, 0 };
	subStringConditions.push_back(condition);

	// update previous conditions
	size_t newPos = subStringConditions.size() - 1;
	if (prefix == or)
	{
		for (size_t i = newPos; i > 0; --i)
		{
			if (subStringConditions[i - 1].nextOrIndex > 0)
				break;

			subStringConditions[i - 1].nextOrIndex = newPos;
		}
	}
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
		filterText = filterText.Mid((int)wcslen(L"!"));
	}

	bool useRegex = filterWithRegex && !filterText.IsEmpty();

	if (useRegex)
		useRegex = ValidateRegexp(filterText, m_patterns);
	else
	{
		// now split the search string into words so we can search for each of them
		int curPos = 0;
		int length = filterText.GetLength();

		while (curPos < length && curPos >= 0)
		{
			// skip spaces
			for (; (curPos < length) && (filterText[curPos] == L' '); ++curPos)
			{
			}

			// has it a prefix?
			Prefix prefix = and;
			if (curPos < length)
			{
				switch (filterText[curPos])
				{
				case L'-':
					prefix = and_not;
					++curPos;
					break;

				case L'+':
					prefix = or;
					++curPos;
					break;
				}
			}

			// escaped string?
			if (curPos < length && filterText[curPos] == L'"')
			{
				CString subString;
				while (++curPos < length)
				{
					if (filterText[curPos] == L'"')
					{
						// double double quotes?
						if (++curPos < length && filterText[curPos] == L'"')
						{
							// keep one and continue within sub-string
							subString.AppendChar(L'"');
						}
						else
						{
							// end of sub-string?
							if (curPos >= length || filterText[curPos] == L' ')
								break;
							else
							{
								// add to sub-string & continue within it
								subString.AppendChar(L'"');
								subString.AppendChar(filterText[curPos]);
							}
						}
					}
					else
						subString.AppendChar(filterText[curPos]);
				}

				AddSubString(subString, prefix);
				++curPos;
			}

			// ordinary sub-string
			AddSubString(filterText.Tokenize(L" ", curPos), prefix);
		}
	}
}

bool CLogDlgFilter::operator()(GitRevLoglist* pRev, CGitLogListBase* loglist) const
{
	if (m_patterns.empty() && subStringConditions.empty())
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

		subStringConditions = rhs.subStringConditions;
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
	if (subStringConditions.size() != rhs.subStringConditions.size())
		return false;

	for (auto lhsIt = subStringConditions.cbegin(), lhsEnd = subStringConditions.cend(), rhsIt = rhs.subStringConditions.cbegin(); lhsIt != lhsEnd; ++rhsIt, ++lhsIt)
	{
		if (lhsIt->subString != rhsIt->subString || lhsIt->prefix != rhsIt->prefix || lhsIt->nextOrIndex != rhsIt->nextOrIndex)
			return false;
	}

	// no difference detected
	return true;
}

bool CLogDlgFilter::operator!=(const CLogDlgFilter& rhs) const
{
	return !operator==(rhs);
}

bool CLogDlgFilter::IsFilterActive() const
{
	return !(m_patterns.empty() && subStringConditions.empty());
}
