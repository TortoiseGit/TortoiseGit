// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012 - TortoiseSVN

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
#include "../../ext/CrashServer/CrashHandler/CrashHandler/CrashHandler.h"
#include "../version.h"
#include <time.h>
#include <string>
#include <tchar.h>

// dummy define, needed only when we use crashrpt instead of this.
#define CR_AF_MAKE_FILE_COPY 0

/**
 * \ingroup Utils
 * helper class for the CrashServerSDK
 */
class CCrashReport
{
private:
	CCrashReport(void)
	{
		LoadDll();
	}

	~CCrashReport(void)
	{
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
		return AddFileToReport(pszFile, pszDestFile) ? 1 : 0;
	}


	//! Initializes crash handler.
	//! \note You may call this function multiple times if some data has changed.
	//! \return Return \b true if crash handling was enabled.
	bool InitCrashHandler(
		ApplicationInfo* applicationInfo,   //!< [in] Pointer to the ApplicationInfo structure that identifies your application.
		HandlerSettings* handlerSettings,   //!< [in] Pointer to the HandlerSettings structure that customizes crash handling behavior. This paramenter can be \b NULL.
		BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
		//!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
		//!<      catch exceptions. See \ref SendReport for more information.
		) throw()
	{
		if (!m_InitCrashHandler)
			return false;

		return m_InitCrashHandler(applicationInfo, handlerSettings, ownProcess) != FALSE;
	}

	//! You may add any key/value pair to crash report.
	//! \return If the function succeeds, the return value is \b true.
	bool AddUserInfoToReport(
		LPCWSTR key,                        //!< [in] key string that will be added to the report.
		LPCWSTR value                       //!< [in] value for the key.
		) throw()
	{
		if (!m_AddUserInfoToReport)
			return false;
		m_AddUserInfoToReport(key, value);
		return true;
	}

	//! You may add any file to crash report. This file will be read when crash appears and will be sent within the report.
	//! Multiple files may be added. Filename of the file in the report may be changed to any name.
	//! \return If the function succeeds, the return value is \b true.
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
		if (!m_SendReport)
			return EXCEPTION_CONTINUE_SEARCH;
		// There is no crash handler but asserts should continue anyway
		if (exceptionPointers->ExceptionRecord->ExceptionCode == ExceptionAssertionViolated)
			return EXCEPTION_CONTINUE_EXECUTION;
		return m_SendReport(exceptionPointers);
	}

	//! To send a report about violated assertion you can throw exception with this exception code
	//! using: \code RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 0, NULL); \endcode
	//! Execution will continue after report will be sent (EXCEPTION_CONTINUE_EXECUTION would be used).
	//! \note If you called CrashHandler constructor and crshhdnl.dll was missing you still may using this exception.
	//!       It will be catched, ignored and execution will continue. \ref SendReport function also works safely
	//!       when crshhdnl.dll was missing.
	static const DWORD ExceptionAssertionViolated = ((DWORD)0xCCE17000);

	//! Sends assertion violation report from this point and continue execution.
	//! \sa ExceptionAssertionViolated
	//! \note Functions prefixed with "CrashServer_" will be ignored in stack parsing.
	void CrashServer_SendAssertionViolated()
	{
		if (!m_InitCrashHandler)
			return;
		::RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 0, NULL);
	}

private:
	bool LoadDll() throw()
	{
		bool result = false;

		// hCrshhndlDll should not be unloaded, crash may appear even after return from main().
		// So hCrshhndlDll is not saved after construction.
		HMODULE hCrshhndlDll = ::LoadLibraryW(L"crshhndl.dll");
		if (hCrshhndlDll != NULL)
		{
			m_InitCrashHandler = (pfnInitCrashHandler) GetProcAddress(hCrshhndlDll, "InitCrashHandler");
			m_SendReport = (pfnSendReport) GetProcAddress(hCrshhndlDll, "SendReport");
			m_IsReadyToExit = (pfnIsReadyToExit) GetProcAddress(hCrshhndlDll, "IsReadyToExit");
			m_AddUserInfoToReport = (pfnAddUserInfoToReport) GetProcAddress(hCrshhndlDll, "AddUserInfoToReport");
			m_AddFileToReport = (pfnAddFileToReport) GetProcAddress(hCrshhndlDll, "AddFileToReport");
			m_RemoveFileFromReport = (pfnRemoveFileFromReport) GetProcAddress(hCrshhndlDll, "RemoveFileFromReport");
			m_GetVersionFromApp = (pfnGetVersionFromApp) GetProcAddress(hCrshhndlDll, "GetVersionFromApp");
			m_GetVersionFromFile = (pfnGetVersionFromFile) GetProcAddress(hCrshhndlDll, "GetVersionFromFile");

			result = m_InitCrashHandler
				&& m_SendReport
				&& m_IsReadyToExit
				&& m_AddUserInfoToReport
				&& m_AddFileToReport
				&& m_RemoveFileFromReport
				&& m_GetVersionFromApp
				&& m_GetVersionFromFile;
		}

		// if no crash processing was started, we need to ignore ExceptionAssertionViolated exceptions.
		if (!result)
			::AddVectoredExceptionHandler(TRUE, SkipAsserts);

		return result;
	}

	static LONG CALLBACK SkipAsserts(EXCEPTION_POINTERS* pExceptionInfo)
	{
		if (pExceptionInfo->ExceptionRecord->ExceptionCode == ExceptionAssertionViolated)
			return EXCEPTION_CONTINUE_EXECUTION;
		return EXCEPTION_CONTINUE_SEARCH;
	}

	typedef BOOL (*pfnInitCrashHandler)(ApplicationInfo* applicationInfo, HandlerSettings* handlerSettings, BOOL ownProcess);
	typedef LONG (*pfnSendReport)(EXCEPTION_POINTERS* exceptionPointers);
	typedef BOOL (*pfnIsReadyToExit)();
	typedef void (*pfnAddUserInfoToReport)(LPCWSTR key, LPCWSTR value);
	typedef void (*pfnAddFileToReport)(LPCWSTR path, LPCWSTR reportFileName /* = NULL */);
	typedef void (*pfnRemoveFileFromReport)(LPCWSTR path);
	typedef BOOL (*pfnGetVersionFromApp)(ApplicationInfo* appInfo);
	typedef BOOL (*pfnGetVersionFromFile)(LPCWSTR path, ApplicationInfo* appInfo);

	pfnInitCrashHandler m_InitCrashHandler;
	pfnSendReport m_SendReport;
	pfnIsReadyToExit m_IsReadyToExit;
	pfnAddUserInfoToReport m_AddUserInfoToReport;
	pfnAddFileToReport m_AddFileToReport;
	pfnRemoveFileFromReport m_RemoveFileFromReport;
	pfnGetVersionFromApp m_GetVersionFromApp;
	pfnGetVersionFromFile m_GetVersionFromFile;
};


class CCrashReportTGit
{
public:

	//! Installs exception handlers to the caller process
	CCrashReportTGit(LPCTSTR appname, bool bOwnProcess = true)
	: m_nInstallStatus(0)
	{
		char s_month[6];
		int month, day, year;
		struct tm t = {0};
		static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
		sscanf_s(__DATE__, "%s %d %d", s_month, _countof(s_month)-1, &day, &year);
		month = (int)((strstr(month_names, s_month)-month_names))/3;

		t.tm_mon = month;
		t.tm_mday = day;
		t.tm_year = year - 1900;
		t.tm_isdst = -1;
		__time64_t compiletime = _mktime64(&t);

		__time64_t now;
		_time64(&now);

		if ((now - compiletime)<(60*60*24*31*4))
		{
			ApplicationInfo appInfo;
			memset(&appInfo, 0, sizeof(appInfo));
			appInfo.ApplicationInfoSize = sizeof(ApplicationInfo);
			appInfo.ApplicationGUID = "7fbde3fc-94e9-408b-b5c8-62bd4e203570";
			appInfo.Prefix = "tgit";
			appInfo.AppName = appname;
			appInfo.Company = L"TortoiseGit";

			appInfo.Hotfix = 0;
			appInfo.V[0] = TGIT_VERMAJOR;
			appInfo.V[1] = TGIT_VERMINOR;
			appInfo.V[2] = TGIT_VERMICRO;
			appInfo.V[3] = TGIT_VERBUILD;

			HandlerSettings handlerSettings;
			memset(&handlerSettings, 0, sizeof(handlerSettings));
			handlerSettings.HandlerSettingsSize = sizeof(handlerSettings);
			handlerSettings.LeaveDumpFilesInTempFolder = FALSE;
			handlerSettings.UseWER = FALSE;
			handlerSettings.OpenProblemInBrowser = TRUE;
			handlerSettings.SubmitterID = 0;

			CCrashReport::Instance().InitCrashHandler(&appInfo, &handlerSettings, bOwnProcess);
		}
	}


	//! Deinstalls exception handlers from the caller process
	~CCrashReportTGit()
	{
		CCrashReport::Instance().Uninstall();
	}

	//! Install status
	int m_nInstallStatus;
};

