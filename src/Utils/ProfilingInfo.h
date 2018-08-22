#pragma once

// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008, 2015 - TortoiseSVN

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

//////////////////////////////////////////////////////////////////////
// required includes
//////////////////////////////////////////////////////////////////////

#ifndef _DEBUG
#ifndef __INTRIN_H_
#include <intrin.h>
#endif
#endif

#ifndef _PSAPI_H_
#include <psapi.h>
#endif

#ifndef _STRING_
#include <string>
#endif

#ifndef _VECTOR_
#include <vector>
#endif

#pragma comment (lib, "psapi.lib")

/**
* Collects the profiling info for a given profiled block / line.
* Records execution count, min, max and accumulated execution time
* in CPU clock ticks.
*/

class CProfilingRecord
{
private:

	/// identification

	const char* name;
	const char* file;
	int line;

	/// collected profiling info

	size_t count;
	unsigned __int64 sum;
	unsigned __int64 minValue;
	unsigned __int64 maxValue;

public:

	/// construction

	CProfilingRecord ( const char* name, const char* file, int line);

	/// record values

	void Add (unsigned __int64 value);

	/// modification

	void Reset();

	/// data access

	const char* GetName() const {return name;}
	const char* GetFile() const {return file;}
	int GetLine() const {return line;}

	size_t GetCount() const {return count;}
	unsigned __int64 GetSum() const {return sum;}
	unsigned __int64 GetMinValue() const {return minValue;}
	unsigned __int64 GetMaxValue() const {return maxValue;}
};

/**
* RAII class that encapsulates a single execution of a profiled
* block / line. The result gets added to an existing profiling record.
*/

class CRecordProfileEvent
{
private:

	CProfilingRecord* record;

	/// the initial CPU counter value

	unsigned __int64 start;

public:

	/// construction: start clock

	CRecordProfileEvent (CProfilingRecord* aRecord);

	/// destruction: time interval to profiling record,
	/// if Stop() had not been called before

	~CRecordProfileEvent();

	/// don't wait for destruction

	void Stop();
};

#ifndef _DEBUG

/// construction / destruction

inline CRecordProfileEvent::CRecordProfileEvent (CProfilingRecord* aRecord)
	: record (aRecord)
	, start (__rdtsc())
{
}

inline CRecordProfileEvent::~CRecordProfileEvent()
{
	if (record)
		record->Add (__rdtsc() - start);
}

#endif

/// don't wait for destruction

inline void CRecordProfileEvent::Stop()
{
	if (record)
	{
		record->Add (__rdtsc() - start);
		record = nullptr;
	}
}

/**
* Singleton class that acts as container for all profiling records.
* You may reset its content as well as write it to disk.
*/

class CProfilingInfo
{
private:

	typedef std::vector<CProfilingRecord*> TRecords;
	TRecords records;

	/// construction / destruction

	CProfilingInfo();
	~CProfilingInfo(void);
	// prevent cloning
	CProfilingInfo(const CProfilingInfo&) = delete;
	CProfilingInfo& operator=(const CProfilingInfo&) = delete;

	/// create report

	std::string GetReport() const;

public:

	/// access to default instance

	static CProfilingInfo* GetInstance();

	/// add a new record

	CProfilingRecord* Create ( const char* name, const char* file, int line);
};

/**
* Profiling macros
*/

#define PROFILE_CONCAT3( a, b )  a##b
#define PROFILE_CONCAT2( a, b )  PROFILE_CONCAT3( a, b )
#define PROFILE_CONCAT( a, b )   PROFILE_CONCAT2( a, b )

/// measures the time from the point of usage to the end of the respective block

#define PROFILE_BLOCK\
	static CProfilingRecord* PROFILE_CONCAT(record,__LINE__) \
		= CProfilingInfo::GetInstance()->Create(__FUNCTION__,__FILE__,__LINE__);\
	CRecordProfileEvent PROFILE_CONCAT(profileSection,__LINE__) (PROFILE_CONCAT(record,__LINE__));

/// measures the time taken to execute the respective code line

#define PROFILE_LINE(line)\
	static CProfilingRecord* PROFILE_CONCAT(record,__LINE__) \
		= CProfilingInfo::GetInstance()->Create(__FUNCTION__,__FILE__,__LINE__);\
	CRecordProfileEvent PROFILE_CONCAT(profileSection,__LINE__) (PROFILE_CONCAT(record,__LINE__));\
	line;\
	PROFILE_CONCAT(profileSection,__LINE__).Stop();
