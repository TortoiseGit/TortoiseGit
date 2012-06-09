// Copyright 2012 Idol Software, Inc.
//
// This file is part of CrashHandler library.
//
// CrashHandler library is free software: you can redistribute it and/or modify
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

//! Contains data that identifies your application.
struct ApplicationInfo
{
    DWORD   ApplicationInfoSize;        //!< Size of this structure. Should be set to sizeof(ApplicationInfo).
    LPCSTR  ApplicationGUID;            //!< GUID assigned to this application.
    LPCSTR  Prefix;                     //!< Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
    LPCWSTR AppName;                    //!< Application name that will be used in message box.
    LPCWSTR Company;                    //!< Company name that will be used in message box.
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
    BOOL    UseWER;                     //!< To continue use Microsoft Windows Error Reporting (WER) set this member to TRUE. In that case after Crash Server send report dialog Microsoft send report dialog also will be shown. This can be necessary in case of Windows Logo program.
    DWORD   SubmitterID;                //!< Crash Server user ID. Uploaded report will be marked as uploaded by this user. This is useful for Crash Server and bug tracking system integration. Set to \b 0 if user using this application is anonymous.
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
    //! \note The crshhndl.dll may missing. In that case there will be no crash handling.
    CrashHandler(
        LPCSTR  applicationGUID,            //!< [in] GUID assigned to this application.
        LPCSTR  prefix,                     //!< [in] Prefix that will be used with the dump name: YourPrefix_v1.v2.v3.v4_YYYYMMDD_HHMMSS.mini.dmp.
        LPCWSTR appName,                    //!< [in] Application name that will be used in message box.
        LPCWSTR company,                    //!< [in] Company name that will be used in message box.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                            //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                            //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!LoadDll())
            return;

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

        m_InitCrashHandler(&appInfo, &handlerSettings, ownProcess);
    }

    //! CrashHandler constructor. Loads crshhndl.dll and initializes crash handling.
    //! \note The crshhndl.dll may missing. In that case there will be no crash handling.
    CrashHandler(
        ApplicationInfo* applicationInfo,   //!< [in] Pointer to the ApplicationInfo structure that identifies your application.
        HandlerSettings* handlerSettings,   //!< [in] Pointer to the HandlerSettings structure that customizes crash handling behavior. This paramenter can be \b NULL.
        BOOL    ownProcess = TRUE           //!< [in] If you own the process your code running in set this option to \b TRUE. If don't (for example you write
                                            //!<      a plugin to some external application) set this option to \b FALSE. In that case you need to explicitly
                                            //!<      catch exceptions. See \ref SendReport for more information.
        ) throw()
    {
        if (!LoadDll())
            return;

        m_InitCrashHandler(applicationInfo, handlerSettings, ownProcess);
    }

    //! CrashHandler constructor. Loads crshhndl.dll. You should call \ref InitCrashHandler to turn on crash handling.
    //! \note The crshhndl.dll may missing. In that case there will be no crash handling.
    CrashHandler() throw()
    {
        LoadDll();
    }

    //! CrashHandler destructor.
    //! \note It doesn't unload crshhndl.dll and doesn't disable crash handling since crash may appear on very late phase of application exit.
    //!       For example destructor of some static variable that is called after return from main() may crash.
    ~CrashHandler()
    {
        if (!m_IsReadyToExit)
            return;

        // If crash has happen not in main thread we should wait here until report will be sent
        // or else program will be terminated after return from main() and report send will be halted.
        while (!m_IsReadyToExit())
            ::Sleep(100);
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

#if _WIN32_WINNT >= 0x0501 /*_WIN32_WINNT_WINXP*/
        // if no crash processing was started, we need to ignore ExceptionAssertionViolated exceptions.
        if (!result)
            ::AddVectoredExceptionHandler(TRUE, SkipAsserts);
#endif

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

#endif // __CRASH_HANDLER_H__