// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2014 - TortoiseSVN

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

#ifndef __INTRIN_H_
#include <intrin.h>
#endif

#include "ProfilingInfo.h"

#ifdef _DEBUG

//////////////////////////////////////////////////////////////////////
/// construction / destruction
//////////////////////////////////////////////////////////////////////

CRecordProfileEvent::CRecordProfileEvent (CProfilingRecord* aRecord)
	: record (aRecord)
	, start (__rdtsc())
{
}

CRecordProfileEvent::~CRecordProfileEvent()
{
	if (record)
		record->Add (__rdtsc() - start);
}

#endif

//////////////////////////////////////////////////////////////////////
// construction / destruction
//////////////////////////////////////////////////////////////////////

CProfilingRecord::CProfilingRecord ( const char* name
								   , const char* file
								   , int line)
	: name (name)
	, file (file)
	, line (line)
	, count (0)
	, sum (0)
	, minValue (ULLONG_MAX)
	, maxValue (0)
{
}

//////////////////////////////////////////////////////////////////////
// record values
//////////////////////////////////////////////////////////////////////

void CProfilingRecord::Add (unsigned __int64 value)
{
	++count;
	sum += value;

	if (value < minValue)
		minValue = value;
	if (value > maxValue)
		maxValue = value;
}

//////////////////////////////////////////////////////////////////////
// modification
//////////////////////////////////////////////////////////////////////

void CProfilingRecord::Reset()
{
	count = 0;
	sum = 0;

	minValue = LLONG_MAX;
	maxValue = 0;
}

//////////////////////////////////////////////////////////////////////
// construction / destruction
//////////////////////////////////////////////////////////////////////

CProfilingInfo::CProfilingInfo()
{
}

CProfilingInfo::~CProfilingInfo(void)
{
	if (!records.empty())
	{
		// write profile to file

		TCHAR buffer [MAX_PATH] = {0};
		if (GetModuleFileNameEx(GetCurrentProcess(), nullptr, buffer, _countof(buffer)) > 0)
			try
			{
				std::wstring fileName (buffer);
				fileName += L".profile";

				std::string report = GetInstance()->GetReport();

				CFile file (fileName.c_str(), CFile::modeCreate | CFile::modeWrite );
				file.Write(report.c_str(), static_cast<UINT>(report.size()));
			}
			catch (...)
			{
				// ignore all file errors etc.
			}


		// free data

		for (size_t i = 0; i < records.size(); ++i)
			delete records[i];
	}
}

//////////////////////////////////////////////////////////////////////
// access to default instance
//////////////////////////////////////////////////////////////////////

CProfilingInfo* CProfilingInfo::GetInstance()
{
	static CProfilingInfo instance;
	return &instance;
}

//////////////////////////////////////////////////////////////////////
// create a report
//////////////////////////////////////////////////////////////////////

static std::string IntToStr (unsigned __int64 value)
{
	char buffer[100] = { 0 };
	_ui64toa_s (value, buffer, 100, 10);

	std::string result = buffer;
	for (size_t i = 3; i < result.length(); i += 4)
		result.insert (result.length() - i, 1, ',');

	return result;
};

std::string CProfilingInfo::GetReport() const
{
	enum { LINE_LENGTH = 500 };

	char lineBuffer [LINE_LENGTH];
	const char * const format ="%10s%17s%17s%17s%6s %s\t%s\n";

	std::string result;
	result.reserve (LINE_LENGTH * records.size());
	sprintf_s ( lineBuffer, format, "count", "sum", "min", "max", "line", "name", "file");
	result += lineBuffer;

	for (auto iter = records.cbegin(), end = records.cend()
		; iter != end
		; ++iter)
	{
		unsigned __int64 minValue = (*iter)->GetMinValue();
		if (minValue == ULLONG_MAX)
			minValue = 0;

		sprintf_s(lineBuffer, format
					, IntToStr ((*iter)->GetCount()).c_str()
					, IntToStr ((*iter)->GetSum()).c_str()
					, IntToStr (minValue).c_str()
					, IntToStr ((*iter)->GetMaxValue()).c_str()

					, IntToStr ((*iter)->GetLine()).c_str()
					, (*iter)->GetName()
					, (*iter)->GetFile());

		result += lineBuffer;
	}

	return result;
}

//////////////////////////////////////////////////////////////////////
// add a new record
//////////////////////////////////////////////////////////////////////

CProfilingRecord* CProfilingInfo::Create ( const char* name, const char* file, int line)
{
	CProfilingRecord* record = new CProfilingRecord (name, file, line);
	records.push_back (record);

	return record;
}
