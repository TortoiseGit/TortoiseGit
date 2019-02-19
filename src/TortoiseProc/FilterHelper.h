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
#pragma once
#include <regex>

class CGitLogListBase;

class CFilterHelper {
private:
	std::vector<std::wregex> m_patterns;

	/// sub-string matching info
	enum Prefix
	{
		and,
		or,
		and_not,
	};

	struct SCondition
	{
		/// sub-strings to find; normalized to lower case
		std::wstring subString;

		/// depending on the presense of a prefix, indicate
		/// how the sub-string match / mismatch gets combined
		/// with the current match result
		Prefix prefix;

		/// index within @ref subStringConditions of the
		/// next condition with prefix==or. 0, if no such
		/// condition follows.
		size_t nextOrIndex;
	};

	/// list of sub-strings to find; normalized to lower case
	std::vector<SCondition> subStringConditions;

	// construction utility
	void AddSubString(CString token, Prefix prefix);

	/// if false, normalize all strings to lower case before comparing them
	bool m_bCaseSensitive;

	/// attribute selector
	/// (i.e. what members of GitRevLoglist shall be used for comparison)
	DWORD m_dwAttributeSelector;

	/// negate pattern matching result
	bool m_bNegate;

protected:
	/// temp / scratch objects to minimize the number memory
	/// allocation operations
	mutable std::wstring scratch;

	/// filter utiltiy method
	bool Match(std::wstring& text) const;

	inline bool CalculateFinalResult(bool result) const { return result ^ m_bNegate; }

public:
	/// construction
	CFilterHelper();
	CFilterHelper(const CFilterHelper& rhs);
	CFilterHelper(const CString& filter, bool filterWithRegex, DWORD selectedFilter, bool caseSensitive);
	virtual ~CFilterHelper();

	/// returns a vector with all the ranges where a match was found.
	void GetMatchRanges(std::vector<CHARRANGE>& ranges, CString text, int offset) const;

	/// assignment operator
	CFilterHelper& operator=(const CFilterHelper& rhs);

	/// compare filter specs
	bool operator==(const CFilterHelper& rhs) const;
	bool operator!=(const CFilterHelper& rhs) const;

	/// returns true if there's something to filter for
	inline bool IsFilterActive() const { return !(m_patterns.empty() && subStringConditions.empty()); }

	/// called to parse a (potentially incorrect) regex spec
	bool ValidateRegexp(const CString& regexp_str, std::vector<std::wregex>& patterns);

	inline DWORD GetSelectedFilters() const { return m_dwAttributeSelector; }
};
