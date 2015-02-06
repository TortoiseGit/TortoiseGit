// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI

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
#include <Windows.h>
#include "EventLog.h"

unsigned int getPID();

namespace EventLog {
	static const wchar_t* APPLICATION_NAME = L"TortoiseSI";

	//
	//   FUNCTION: EventLog::writeEvent(wchar_t *, WORD)
	//
	//   PURPOSE: Log a message to the Application event log.
	//
	//   PARAMETERS:
	//   * message - message to be logged.
	//   * wType - the type of event to be logged. The parameter can be one of 
	//     the following values:
	//
	//     EVENTLOG_SUCCESS
	//     EVENTLOG_AUDIT_FAILURE
	//     EVENTLOG_AUDIT_SUCCESS
	//     EVENTLOG_ERROR_TYPE
	//     EVENTLOG_INFORMATION_TYPE
	//     EVENTLOG_WARNING_TYPE
	//
	void writeEvent(std::wstring message, WORD wType) {
		HANDLE hEventSource = NULL;
		LPCWSTR lpszStrings[3] = {NULL, NULL, NULL};
		const wchar_t *name = EventLog::APPLICATION_NAME;

		hEventSource = RegisterEventSource(NULL, name);
		if (hEventSource) {
			std::wstring source = L"Source Module: " + getProcessFilesName();
			std::wstring pid = L"Process ID: " + std::to_wstring(getPID());

			lpszStrings[0] = source.c_str();
			lpszStrings[1] = pid.c_str();
			lpszStrings[2] = message.c_str();

			 ReportEvent(hEventSource,		// Event log handle
				wType,						// Event type
				0,							// Event category
				0,							// Event identifier
				NULL,						// No security identifier
				ARRAYSIZE(lpszStrings),		// Size of lpszStrings array
				0,							// No binary data
				lpszStrings,				// Array of strings
				NULL						// No binary data
				);

			 DeregisterEventSource(hEventSource);
		}
	}

	//
	//   FUNCTION: EventLog::writeErrorLogEntry(wstring)
	//
	//   PURPOSE: Log an ERROR type message to the Application event log.
	//
	//   PARAMETERS:
	//   * error - the error message
	//
	void writeError(std::wstring error) {
		writeEvent(error, EVENTLOG_ERROR_TYPE);
	}
	//
	//   FUNCTION: EventLog::writeInformationLogEntry(wstring)
	//
	//   PURPOSE: Log an INFORMATION type message to the Application event log.
	//
	//   PARAMETERS:
	//   * info - the information message
	//
	void writeInformation(std::wstring info) {
		writeEvent(info, EVENTLOG_INFORMATION_TYPE);
	}

	//
	//   FUNCTION: EventLog::writeWarningLogEntry(wstring)
	//
	//   PURPOSE: Log an WARNING type message to the Application event log.
	//
	//   PARAMETERS:
	//   * warning - the warning message
	//
	void writeWarning(std::wstring warning) {
		writeEvent(warning, EVENTLOG_WARNING_TYPE);
	}
}

std::wstring getProcessFilesName()
{
	wchar_t moduleName[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, moduleName, _countof(moduleName));
	return std::wstring(moduleName);
}

unsigned int getPID() 
{
	return GetCurrentProcessId();
}