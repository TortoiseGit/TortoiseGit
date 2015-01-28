// TortoiseSI - a Windows shell extension for easy version control

// Copyright (C) 2015 - TortoiseSI
// Copyright (C) 2013 - TortoiseGit
// Copyright (C) 2012 - 2014 - TortoiseSVN

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
//#include "../../ext/CrashServer/CrashHandler/CrashHandler/CrashHandler.h"
#include <time.h>
#include <string>
#include <tchar.h>
#include <DbgHelp.h>

// dummy define, needed only when we use crashrpt instead of this.
#define CR_AF_MAKE_FILE_COPY 0

__forceinline HMODULE get_my_module_handle(void)
{
	static int s_module_marker = 0;
	MEMORY_BASIC_INFORMATION memory_basic_information;
	if (!VirtualQuery(&s_module_marker, &memory_basic_information, sizeof(memory_basic_information)))
	{
		return NULL;
	}
	return (HMODULE)memory_basic_information.AllocationBase;
}

/**
 * \ingroup Utils
 * helper class for the DoctorDumpSDK
 */
class CCrashReport
{
private:
	CCrashReport(void)
	{

	}

	~CCrashReport(void)
	{
/*		if (!m_IsReadyToExit)
			return;

		// If crash has happen not in main thread we should wait here until report will be sent
		// or else program will be terminated after return from main() and report sending will be halted.
		while (!m_IsReadyToExit())
			::Sleep(100);

		if (m_bSkipAssertsAdded)
			RemoveVectoredExceptionHandler(SkipAsserts);*/
	}

public:
	static CCrashReport&    Instance()
	{
		static CCrashReport instance;
		return instance;
	}

	int                     Uninstall(void) { return FALSE; }
	int                     AddFile2(LPCTSTR pszFile,LPCTSTR pszDestFile,LPCTSTR /*pszDesc*/,DWORD /*dwFlags*/)
	{
//		return AddFileToReport(pszFile, pszDestFile) ? 1 : 0;
	}

	//! Checks that crash handling was enabled.
	//! \return Return \b true if crash handling was enabled.
	bool IsCrashHandlingEnabled() const
	{
		return m_bWorking;
	}
#if 0
	//! Initializes crash handler.
	//! \note You may call this function multiple times if some data has changed.
	//! \return Return \b true if crash handling was enabled.
	bool InitCrashHandler() throw()
	{
		if (!m_InitCrashHandler)
			return false;

		// TODO

		return m_bWorking;
	}


	//! You may remove any key that was added previously to crash report by \a AddUserInfoToReport.
	//! \return If the function succeeds, the return value is \b true.
	//! \note This function is thread safe.
	bool RemoveUserInfoFromReport(
		LPCWSTR key                        //!< [in] key string that will be removed from the report.
		)
	{
		if (!m_RemoveUserInfoFromReport)
			return false;
		m_RemoveUserInfoFromReport(key);
		return true;
	}

	//! You may add any file to crash report. This file will be read when crash appears and will be sent within the report.
	//! Multiple files may be added. Filename of the file in the report may be changed to any name.
	//! \return If the function succeeds, the return value is \b true.
	//! \note This function is thread safe.
	bool AddFileToReport(
		LPCWSTR path,                       //!< [in] Path to the file, that will be added to the report.
		LPCWSTR reportFileName /* = NULL */ //!< [in] Filename that will be used in report for this file. If parameter is \b NULL, original name from path will be used.
		) throw()
	{
		if (!m_AddFileToReport)
			return false;
		m_AddFileToReport(path, reportFileName);
		return true;
	}

	//! Remove from report the file that was registered earlier to be sent within report.
	//! \return If the function succeeds, the return value is \b true.
	//! \note This function is thread safe.
	bool RemoveFileFromReport(
		LPCWSTR path    //!< [in] Path to the file, that will be removed from the report.
		) throw()
	{
		if (!m_RemoveFileFromReport)
			return false;
		m_RemoveFileFromReport(path);
		return true;
	}

	//! Fills version field (V) of ApplicationInfo with product version
	//! found in the executable file of the current process.
	//! \return If the function succeeds, the return value is \b true.
	bool GetVersionFromApp(
		ApplicationInfo* appInfo //!< [out] Pointer to ApplicationInfo structure. Its version field (V) will be set to product version.
		) throw()
	{
		if (!m_GetVersionFromApp)
			return false;
		return m_GetVersionFromApp(appInfo) != FALSE;
	}

	//! Fill version field (V) of ApplicationInfo with product version found in the file specified.
	//! \return If the function succeeds, the return value is \b true.
	bool GetVersionFromFile(
		LPCWSTR path,               //!< [in] Path to the file product version will be extracted from.
		ApplicationInfo* appInfo    //!< [out] Pointer to ApplicationInfo structure. Its version field (V) will be set to product version.
		) throw()
	{
		if (!m_GetVersionFromFile)
			return false;
		return m_GetVersionFromFile(path, appInfo) != FALSE;
	}
#endif
	//! If you do not own the process your code running in (for example you write a plugin to some
	//! external application) you need to properly initialize CrashHandler using \b ownProcess option.
	//! Also you need to explicitly catch all exceptions in all entry points to your code and in all
	//! threads you create. To do so use this construction:
	//! \code
	//! bool SomeEntryPoint(PARAM p)
	//! {
	//!     __try
	//!     {
	//!         return YouCode(p);
	//!     }
	//!     __except (CrashHandler::SendReport(GetExceptionInformation()))
	//!     {
	//!         ::ExitProcess(0); // It is better to stop the process here or else corrupted data may incomprehensibly crash it later.
	//!         return false;
	//!     }
	//! }
	//! \endcode
	LONG SendReport(
		EXCEPTION_POINTERS* exceptionPointers   //!< [in] Pointer to EXCEPTION_POINTERS structure. You should get it using GetExceptionInformation()
												//!<      function inside __except keyword.
		)
	{
//		if (!m_SendReport)
			return EXCEPTION_CONTINUE_SEARCH;
		// There is no crash handler but asserts should continue anyway
	//	if (exceptionPointers->ExceptionRecord->ExceptionCode == ExceptionAssertionViolated)
		//	return EXCEPTION_CONTINUE_EXECUTION;
		//return m_SendReport(exceptionPointers);
	}
#if 0
	//! To send a report about violated assertion you can throw exception with this exception code
	//! using: \code RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 0, NULL); \endcode
	//! Execution will continue after report will be sent (EXCEPTION_CONTINUE_EXECUTION would be used).
	//! You may pass grouping string as first parameter (see \a SkipDoctorDump_SendAssertionViolated).
	//! \note If you called CrashHandler constructor and crshhndl.dll was missing you still may using this exception.
	//!       It will be caught, ignored and execution will continue. \ref SendReport function also works safely
	//!       when crshhndl.dll was missing.
	static const DWORD ExceptionAssertionViolated = ((DWORD)0xCCE17000);

	//! Sends assertion violation report from this point and continue execution.
	//! \sa ExceptionAssertionViolated
	//! \note Functions containing "SkipDoctorDump" will be ignored in stack parsing.
	void SkipDoctorDump_SendAssertionViolated(
		LPCSTR dumpGroup = NULL     //!< [in] All dumps with that group will be separated from dumps with same stack but another group. Set parameter to \b NULL if no grouping is required.
		) const
	{
		if (!m_bWorking)
			return;
		if (dumpGroup)
			::RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 1, reinterpret_cast<ULONG_PTR*>(&dumpGroup));
		else
			::RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 0, NULL);
	}
#endif
private:

	static LONG CALLBACK SkipAsserts(EXCEPTION_POINTERS* pExceptionInfo)
	{
//		if (pExceptionInfo->ExceptionRecord->ExceptionCode == ExceptionAssertionViolated)
	//		return EXCEPTION_CONTINUE_EXECUTION;
		return EXCEPTION_CONTINUE_SEARCH;
	}

	bool m_bLoaded;
	bool m_bWorking;
	bool m_bSkipAssertsAdded;
};
#if 0
class CCrashReportTGit
{
public:

	//! Installs exception handlers to the caller process
	CCrashReportTGit(LPCTSTR appname, USHORT versionMajor, USHORT versionMinor, USHORT versionMicro, USHORT versionBuild, const char* /*buildDate*/, bool bOwnProcess = true)
	: m_nInstallStatus(0)
	{
		ApplicationInfo appInfo = { 0 };
		appInfo.ApplicationInfoSize = sizeof(ApplicationInfo);
		appInfo.ApplicationGUID = "7fbde3fc-94e9-408b-b5c8-62bd4e203570";
		appInfo.Prefix = "tgit";
		appInfo.AppName = appname;
		appInfo.Company = L"TortoiseGit";

		appInfo.Hotfix = 0;
		appInfo.V[0] = versionMajor;
		appInfo.V[1] = versionMinor;
		appInfo.V[2] = versionMicro;
		appInfo.V[3] = versionBuild;

		HandlerSettings handlerSettings = { 0 };
		handlerSettings.HandlerSettingsSize = sizeof(handlerSettings);
		handlerSettings.LeaveDumpFilesInTempFolder = FALSE;
		handlerSettings.UseWER = FALSE;
		handlerSettings.OpenProblemInBrowser = TRUE;

		CCrashReport::Instance().InitCrashHandler(&appInfo, &handlerSettings, bOwnProcess);
	}


	//! Deinstalls exception handlers from the caller process
	~CCrashReportTGit()
	{
		CCrashReport::Instance().Uninstall();
	}

	//! Install status
	int m_nInstallStatus;
};
#endif
