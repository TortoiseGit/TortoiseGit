// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2007-2007 - TortoiseSVN

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

///////////////////////////////////////////////////////////////
//
// CHighResClock
//
//		high resolution clock for performance measurement.
//		Depending on the hardware it will provide µsec
//		resolution and accuracy.
//
//		May not be available on all machines.
//
///////////////////////////////////////////////////////////////

class CHighResClock
{
private:

	LARGE_INTEGER start;
	LARGE_INTEGER taken;

public:

	// construction (starts measurement) / destruction

	CHighResClock()
	{
		taken.QuadPart = 0;
		Start();
	}

	~CHighResClock()
	{
	}

	// (re-start)

	void Start()
	{
		QueryPerformanceCounter(&start);
	}

	// set "taken" to time since last Start()

	void Stop()
	{
		QueryPerformanceCounter(&taken);
		taken.QuadPart -= start.QuadPart;
	}

	// time in microseconds between last Start() and last Stop()

	DWORD GetMusecsTaken() const
	{
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return static_cast<DWORD>((taken.QuadPart * 1000000) / frequency.QuadPart);
	}
};
