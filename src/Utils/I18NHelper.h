// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2020 - TortoiseGit

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

class CI18NHelper
{
private:
	CI18NHelper() = delete;

	static std::wstring AdjustVersion(const std::wstring& sVer)
	{
		if (std::count(sVer.cbegin(), sVer.cend(), L'.') != 3)
			return {};

		if (auto lastDot = sVer.find_last_of(L'.');  lastDot >= wcslen(L"1.2.3")) // strip last entry; MSI also ignores the build number
			return sVer.substr(0, lastDot + 1);

		return {};
	}

public:
	static bool DoVersionStringsMatch(const std::wstring& sVer1, const std::wstring& sVer2)
	{
		auto sAdjustedVer1 = AdjustVersion(sVer1);
		auto sAdjustedVer2 = AdjustVersion(sVer2);

		if (sAdjustedVer1.empty() || sAdjustedVer2.empty())
			return false;

		return AdjustVersion(sVer1) == AdjustVersion(sVer2);
	}
};
