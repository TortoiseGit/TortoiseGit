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
#include "EventLog.h"
#include "ShellCache.h"

namespace EventLog {

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
	void writeEvent(const wchar_t *message, WORD wType) {
		HANDLE hEventSource = NULL;
		LPCWSTR lpszStrings[2] = {NULL, NULL};
		const wchar_t *name = EventLog::APPLICATION_NAME;

		hEventSource = RegisterEventSource(NULL, name);
		if (hEventSource) {
			lpszStrings[0] = name;
			lpszStrings[1] = message;

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
		writeEvent(error.c_str(), EVENTLOG_ERROR_TYPE);
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
		writeEvent(info.c_str(), EVENTLOG_INFORMATION_TYPE);
	}

	void writeDebug(std::wstring info) {
		static ShellCache shellCache;

		if (shellCache.IsDebugLogging()) {
			writeInformation(info);
		}
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
		writeEvent(warning.c_str(), EVENTLOG_WARNING_TYPE);
	}
}