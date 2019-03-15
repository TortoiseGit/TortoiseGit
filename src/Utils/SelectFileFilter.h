// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010-2012, 2014 - TortoiseSVN

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

#include "StringUtils.h"

class CSelectFileFilter {
public:
	CSelectFileFilter(UINT stringId);
	CSelectFileFilter() {}
	~CSelectFileFilter() {}

	operator const TCHAR*() { return buffer.get(); }
	void Load(UINT stringId);
	UINT GetCount() const { return static_cast<UINT>(filternames.size()); }
	operator const COMDLG_FILTERSPEC*() { return filterspec.get(); }

private:
	std::unique_ptr<TCHAR[]> buffer;
	std::unique_ptr<COMDLG_FILTERSPEC[]> filterspec;
	std::vector<CString> filternames;
	std::vector<CString> filtermasks;

	void ResetAll();
};

inline CSelectFileFilter::CSelectFileFilter(UINT stringId)
{
	Load(stringId);
}

inline void CSelectFileFilter::Load(UINT stringId)
{
	ResetAll();
	CString sFilter;
	sFilter.LoadString(stringId);
	const int bufferLength = sFilter.GetLength()+4;
	buffer = std::make_unique<TCHAR[]>(bufferLength);
	wcscpy_s (buffer.get(), bufferLength, sFilter);
	CStringUtils::PipesToNulls(buffer.get());

	int pos = 0;
	CString temp;
	for (;;)
	{
		temp = sFilter.Tokenize(L"|", pos);
		if (temp.IsEmpty())
		{
			break;
		}
		filternames.push_back(temp);
		temp = sFilter.Tokenize(L"|", pos);
		filtermasks.push_back(temp);
	}
	filterspec.reset(new COMDLG_FILTERSPEC[filternames.size()]);
	for (size_t i = 0; i < filternames.size(); ++i)
	{
		filterspec[i].pszName = filternames[i];
		filterspec[i].pszSpec = filtermasks[i];
	}
}

inline void CSelectFileFilter::ResetAll()
{
	buffer.reset();
	// First release the struct that references the vectors, then clear the vectors
	filterspec.reset();
	filternames.clear();
	filtermasks.clear();
}
