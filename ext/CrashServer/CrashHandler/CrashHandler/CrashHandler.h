// Copyright 2014 Idol Software, Inc.
//
// This file is part of Doctor Dump SDK.
//
// Doctor Dump SDK is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef __CRASH_HANDLER_H__
#define __CRASH_HANDLER_H__

#include <windows.h>

#if defined(CRASHHANDLER_ENABLE_RELEASE_ASSERTS)
#   include <assert.h>
#       ifndef _DEBUG
#           undef assert
//! When _DEBUG macro is defined works as standard assert macro from assert.h header.
//! When _DEBUG macro is not defined evaluates an expression and, when the result is false, sends report and continues execution.
//! To enable this redefined assert behavior define project wide \b CRASHHANDLER_ENABLE_RELEASE_ASSERTS macro and include CrashHandler.h in each translation unit as soon as possible.
//! \note All assert calls before CrashHandler.h inclusion would work as standard asserts.
#           define assert(expr) ((void) (!(expr) && (SkipDoctorDump_ReportAssertionViolation<__COUNTER__>(__FUNCTION__ ": "#expr " is false" ), true)))
#       endif // !_DEBUG
#endif // CRASHHANDLER_ENABLE_RELEASE_ASSERTS

namespace {

    // This template should be in anonymous namespace since __COUNTER__ is unique only for a single translation unit (as anonymous namespace items)
    template<unsigned uniqueAssertId>
    __forceinline static void SkipDoctorDump_ReportAssertionViolation(LPCSTR dumpGroup)
    {
        static LONG volatile isAlreadyReported = FALSE;
        if (TRUE == InterlockedCompareExchange(&isAlreadyReported, TRUE, FALSE))
            return;
        ::RaiseException(CrashHandler::ExceptionAssertionViolated, 0, 1, reinterpret_cast<ULONG_PTR*>(&dumpGroup));
    }

} // namespace {

//! Contains data that identifies your application.
struct ApplicationInfo
{
    DWORD   ApplicationInfoSize;        //!< Size of this structure. Should be set to sizeof(ApplicationInfo).
    LPCSTR  ApplicationGUID;            //!< GUID assigned to this application in form 00000000-0000-0000-0000-000000000000.
    LPCSTR  Prefix;                     //!< Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
    LPCWSTR AppName;                    //!< Application name that will be shown in message box.
    LPCWSTR Company;                    //!< Company name that will be shown in message box.
    USHORT  V[4];                       //!< Version of this application.
    USHORT  Hotfix;                     //!< Version of hotfix for this application (reserved for future use, should be 0).
    LPCWSTR PrivacyPolicyUrl;           //!< URL to privacy policy. If NULL default privacy policy is used.
};

//! \brief Contains crash handling behavior customization parameters.
//!
//! Default values for all parameters is 0/FALSE.
struct HandlerSettings
{
    DWORD   HandlerSettingsSize;        //!< Size of this structure. Should be set to sizeof(HandlerSettings).
    BOOL    LeaveDumpFilesInTempFolder; //!< To leave error reports in temp folder you should set this member to TRUE. Your support or test lab teams can use that reports later.
    BOOL    OpenProblemInBrowser;       //!< To open Web-page belonging to the uploaded report after it was uploaded set this member to TRUE. It is useful for test lab to track the bug or write some comments.
    BOOL    UseWER;                     //!< To continue use Microsoft Windows Error Reporting (WER) set this member to TRUE. In that case after Doctor Dump send report dialog Microsoft send report dialog also will be shown. This can be necessary in case of Windows Logo program.
    DWORD   SubmitterID;                //!< Doctor Dump user ID. Uploaded report will be marked as uploaded by this user. This is useful for Doctor Dump and bug tracking system integration. Set to \b 0 if user using this application is anonymous.
    BOOL    SendAdditionalDataWithoutApproval; //!< To automatically accepted the question "Do you want to send more information about the problem?" set this member to TRUE .
    BOOL    OverrideDefaultFullDumpType;//!< To override default type of data gathered by the library set this member to TRUE and set required type of data in \a FullDumpType.
    DWORD   FullDumpType;               //!< The type of information to be generated when full dump is requested by Doctor Dump. This parameter can be one or more of the values from the MINIDUMP_TYPE enumeration.
    LPCWSTR LangFilePath;               //!< To customize localization set this member to the path to the language file (including file name).
    LPCWSTR SendRptPath;                //!< Set this member to NULL to use default behavior when SendRpt is named sendrpt.exe and exist in same folder with crshhndl.dll. Set full path in other cases.
    LPCWSTR DbgHelpPath;                //!< Set this member to NULL to use default behavior when DbgHelp is named dbghelp.dll and exist in same folder with crshhndl.dll. Set full path in other cases.
                                        //!< \note You should use dbghelp.dll that distributed with crshhndl.dll and not the one in Windows\\System32 folder, because only that dll supports all needed functionality. See <a href="http://msdn.microsoft.com/en-us/library/windows/desktop/ms679294(v=vs.85).aspx">DbgHelp Versions</a> for more information.
};

//! \brief To enable crash processing you should create an instance of this class.
//!
//! It should be created as global static object and correctly initialized.
//! Also you may instantiate it in your main() or WinMain() function as soon as possible.
class CrashHandler
{
public:
    //! \example Sample.cpp
    //! This is an example of how to use the CrashHandler class.

    //! CrashHandler constructor. Loads crshhndl.dll and initializes crash handling.
    //! \note The crshhndl.dll is allowed to be missing. In that case there will be no crash handling.
    CrashHandler(
        LPCSTR  applicationGUID,            //!< [in] GUID assigned to this application.
        LPCWSTR appName,                    //!< [in] Application name that will be shown in message box.
        LPCWSTR company                     //!< [in] Company name that will be shown in message box.
        ) throw()
    {
        if (!LoadDll())
            return;

        InitCrashHandler(applicationGUID, NULL, appName, company, TRUE);
    }

    //! CrashHandler constructor. Loads crshhndl.dll and initializes crash handling.
    //! \note The crshhndl.dll is allowed to be missing. In that case there will be no crash handling.
    CrashHandler(
        LPCSTR  applicationGUID,            //!< [in] GUID assigned to this application.
        LPCSTR  prefix,                     //!< [in] Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
        LPCWSTR appName,                    //!< [in] Application name that will be shown in message box.
        LPCWSTR company,                    //!< [in] Company name that will be shown in message box.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                            //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                            //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!LoadDll())
            return;

        InitCrashHandler(applicationGUID, prefix, appName, company, ownProcess);
    }

    //! CrashHandler constructor. Loads crshhndl.dll and initializes crash handling.
    //! \note The crshhndl.dll is allowed to be missing. In that case there will be no crash handling.
    CrashHandler(
        LPCWSTR crashHandlerPath,               //!< [in] Path to crshhndl.dll file. File may be renamed.
        LPCSTR  applicationGUID,            //!< [in] GUID assigned to this application.
        LPCSTR  prefix,                     //!< [in] Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
        LPCWSTR appName,                    //!< [in] Application name that will be shown in message box.
        LPCWSTR company,                    //!< [in] Company name that will be shown in message box.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
        //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
        //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!LoadDll(crashHandlerPath))
            return;

        InitCrashHandler(applicationGUID, prefix, appName, company, ownProcess);
    }

    //! CrashHandler constructor. Loads crshhndl.dll and initializes crash handling.
    //! \note The crshhndl.dll is allowed to be missing. In that case there will be no crash handling.
    CrashHandler(
        ApplicationInfo* applicationInfo,   //!< [in] Pointer to the ApplicationInfo structure that identifies your application.
        HandlerSettings* handlerSettings,   //!< [in] Pointer to the HandlerSettings structure that customizes crash handling behavior. This parameter can be \b NULL.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                            //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                            //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!LoadDll())
            return;

        InitCrashHandler(applicationInfo, handlerSettings, ownProcess);
    }

    //! CrashHandler constructor. Loads crshhndl.dll and initializes crash handling.
    //! \note The crshhndl.dll is allowed to be missing. In that case there will be no crash handling.
    CrashHandler(
        LPCWSTR crashHandlerPath,               //!< [in] Path to crshhndl.dll file. File may be renamed.
        ApplicationInfo* applicationInfo,   //!< [in] Pointer to the ApplicationInfo structure that identifies your application.
        HandlerSettings* handlerSettings,   //!< [in] Pointer to the HandlerSettings structure that customizes crash handling behavior. This parameter can be \b NULL.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                            //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                            //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!LoadDll(crashHandlerPath))
            return;

        InitCrashHandler(applicationInfo, handlerSettings, ownProcess);
    }


    //! CrashHandler constructor. Loads crshhndl.dll. You should call \ref InitCrashHandler to turn on crash handling.
    //! \note The crshhndl.dll is allowed to be missing. In that case there will be no crash handling.
    CrashHandler(
        LPCWSTR crashHandlerPath = NULL         //!< [in] Path to crshhndl.dll file. File may be renamed.
        ) throw()
    {
        LoadDll(crashHandlerPath);
    }

    //! CrashHandler destructor.
    //! \note It doesn't unload crshhndl.dll and doesn't disable crash handling since crash may appear on very late phase of application exit.
    //!       For example destructor of some static variable that is called after return from main() may crash.
    ~CrashHandler()
    {
        if (!m_IsReadyToExit)
            return;

        // If crash has happen not in main thread we should wait here until report will be sent
        // or else program will be terminated after return from main() and report sending will be halted.
        while (!m_IsReadyToExit())
            ::Sleep(100);

#if _WIN32_WINNT >= 0x0501 /*_WIN32_WINNT_WINXP*/
        if (m_bSkipAssertsAdded)
            RemoveVectoredExceptionHandler(SkipAsserts);
#endif
    }

    //! Checks that crash handling was enabled.
    //! \return Return \b true if crash handling was enabled.
    bool IsCrashHandlingEnabled() const
    {
        return m_bWorking;
    }

    //! Initializes crash handler.
    //! \note You may call this function multiple times if some data has changed.
    //! \return Return \b true if crash handling was enabled.
    bool InitCrashHandler(
        ApplicationInfo* applicationInfo,   //!< [in] Pointer to the ApplicationInfo structure that identifies your application.
        HandlerSettings* handlerSettings,   //!< [in] Pointer to the HandlerSettings structure that customizes crash handling behavior. This parameter can be \b NULL.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                            //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                            //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!m_InitCrashHandler)
            return false;

        m_bWorking = m_InitCrashHandler(applicationInfo, handlerSettings, ownProcess) != FALSE;

        return m_bWorking;
    }

    //! Initializes crash handler.
    //! \note You may call this function multiple times if some data has changed.
    //! \return Return \b true if crash handling was enabled.
    bool InitCrashHandler(
        LPCSTR  applicationGUID,            //!< [in] GUID assigned to this application.
        LPCSTR  prefix,                     //!< [in] Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
        LPCWSTR appName,                    //!< [in] Application name that will be shown in message box.
        LPCWSTR company,                    //!< [in] Company name that will be shown in message box.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                            //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                            //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!m_GetVersionFromApp)
            return false;

        ApplicationInfo appInfo;
        memset(&appInfo, 0, sizeof(appInfo));
        appInfo.ApplicationInfoSize = sizeof(appInfo);
        appInfo.ApplicationGUID = applicationGUID;
        appInfo.Prefix = prefix;
        appInfo.AppName = appName;
        appInfo.Company = company;
        if (!m_GetVersionFromApp(&appInfo))
            appInfo.V[0] = 1;

        HandlerSettings handlerSettings;
        memset(&handlerSettings, 0, sizeof(handlerSettings));
        handlerSettings.HandlerSettingsSize = sizeof(handlerSettings);
        handlerSettings.OpenProblemInBrowser = TRUE;

        return InitCrashHandler(&appInfo, &handlerSettings, ownProcess);
    }

    //! \note This function is experimental and may not be available and may not be support by Doctor Dump in the future.
    //! You may set custom information for your possible report.
    //! This text will be available on Doctor Dump dumps page.
    //! The text should not contain private information.
    //! \return If the function succeeds, the return value is \b true.
    bool SetCustomInfo(
        LPCWSTR text                        //!< [in] custom info for the report. The text will be cut to 100 characters.
        )
    {
        if (!m_SetCustomInfo)
            return false;
        m_SetCustomInfo(text);
        return true;
    }

    //! You may add any key/value pair to crash report.
    //! \return If the function succeeds, the return value is \b true.
    //! \note This function is thread safe.
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
    //! You may pass grouping string as first parameter (see \a SkipDoctorDump_SendAssertionViolated).
    //! \note If you called CrashHandler constructor and crshhndl.dll was missing you still may using this exception.
    //!       It will be catched, ignored and execution will continue. \ref SendReport function also works safely
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

private:
    bool LoadDll(LPCWSTR crashHandlerPath = NULL) throw()
    {
        m_bLoaded = false;
        m_bWorking = false;
        m_bSkipAssertsAdded = false;
        m_InitCrashHandler = NULL;
        m_SendReport = NULL;
        m_IsReadyToExit = NULL;
        m_SetCustomInfo = NULL;
        m_AddUserInfoToReport = NULL;
        m_RemoveUserInfoFromReport = NULL;
        m_AddFileToReport = NULL;
        m_RemoveFileFromReport = NULL;
        m_GetVersionFromApp = NULL;
        m_GetVersionFromFile = NULL;

        // hCrashHandlerDll should not be unloaded, crash may appear even after return from main().
        // So hCrashHandlerDll is not saved after construction.
        HMODULE hCrashHandlerDll = ::LoadLibraryW(crashHandlerPath ? crashHandlerPath : L"crshhndl.dll");
        if (hCrashHandlerDll != NULL)
        {
            m_InitCrashHandler = (pfnInitCrashHandler) GetProcAddress(hCrashHandlerDll, "InitCrashHandler");
            m_SendReport = (pfnSendReport) GetProcAddress(hCrashHandlerDll, "SendReport");
            m_IsReadyToExit = (pfnIsReadyToExit) GetProcAddress(hCrashHandlerDll, "IsReadyToExit");
            m_SetCustomInfo = (pfnSetCustomInfo) GetProcAddress(hCrashHandlerDll, "SetCustomInfo");
            m_AddUserInfoToReport = (pfnAddUserInfoToReport) GetProcAddress(hCrashHandlerDll, "AddUserInfoToReport");
            m_RemoveUserInfoFromReport = (pfnRemoveUserInfoFromReport) GetProcAddress(hCrashHandlerDll, "RemoveUserInfoFromReport");
            m_AddFileToReport = (pfnAddFileToReport) GetProcAddress(hCrashHandlerDll, "AddFileToReport");
            m_RemoveFileFromReport = (pfnRemoveFileFromReport) GetProcAddress(hCrashHandlerDll, "RemoveFileFromReport");
            m_GetVersionFromApp = (pfnGetVersionFromApp) GetProcAddress(hCrashHandlerDll, "GetVersionFromApp");
            m_GetVersionFromFile = (pfnGetVersionFromFile) GetProcAddress(hCrashHandlerDll, "GetVersionFromFile");

            m_bLoaded = m_InitCrashHandler
                && m_SendReport
                && m_IsReadyToExit
                && m_SetCustomInfo
                && m_AddUserInfoToReport
                && m_RemoveUserInfoFromReport
                && m_AddFileToReport
                && m_RemoveFileFromReport
                && m_GetVersionFromApp
                && m_GetVersionFromFile;
        }

#if _WIN32_WINNT >= 0x0501 /*_WIN32_WINNT_WINXP*/
        // if no crash processing was started, we need to ignore ExceptionAssertionViolated exceptions.
        if (!m_bLoaded)
        {
            ::AddVectoredExceptionHandler(TRUE, SkipAsserts);
            m_bSkipAssertsAdded = true;
        }
#endif

        return m_bLoaded;
    }

    static LONG CALLBACK SkipAsserts(EXCEPTION_POINTERS* pExceptionInfo)
    {
        if (pExceptionInfo->ExceptionRecord->ExceptionCode == ExceptionAssertionViolated)
            return EXCEPTION_CONTINUE_EXECUTION;
        return EXCEPTION_CONTINUE_SEARCH;
    }

    bool m_bLoaded;
    bool m_bWorking;
    bool m_bSkipAssertsAdded;

    typedef BOOL (*pfnInitCrashHandler)(ApplicationInfo* applicationInfo, HandlerSettings* handlerSettings, BOOL ownProcess);
    typedef LONG (*pfnSendReport)(EXCEPTION_POINTERS* exceptionPointers);
    typedef BOOL (*pfnIsReadyToExit)();
    typedef void (*pfnSetCustomInfo)(LPCWSTR text);
    typedef void (*pfnAddUserInfoToReport)(LPCWSTR key, LPCWSTR value);
    typedef void (*pfnRemoveUserInfoFromReport)(LPCWSTR key);
    typedef void (*pfnAddFileToReport)(LPCWSTR path, LPCWSTR reportFileName /* = NULL */);
    typedef void (*pfnRemoveFileFromReport)(LPCWSTR path);
    typedef BOOL (*pfnGetVersionFromApp)(ApplicationInfo* appInfo);
    typedef BOOL (*pfnGetVersionFromFile)(LPCWSTR path, ApplicationInfo* appInfo);

    pfnInitCrashHandler m_InitCrashHandler;
    pfnSendReport m_SendReport;
    pfnIsReadyToExit m_IsReadyToExit;
    pfnSetCustomInfo m_SetCustomInfo;
    pfnAddUserInfoToReport m_AddUserInfoToReport;
    pfnRemoveUserInfoFromReport m_RemoveUserInfoFromReport;
    pfnAddFileToReport m_AddFileToReport;
    pfnRemoveFileFromReport m_RemoveFileFromReport;
    pfnGetVersionFromApp m_GetVersionFromApp;
    pfnGetVersionFromFile m_GetVersionFromFile;
};

#endif // __CRASH_HANDLER_H__