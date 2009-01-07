///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashHandler.h
//
//    Desc: CCrashHandler is the main class used by crashrpt to manage all
//          of the details associated with handling the exception, generating
//          the report, gathering client input, and sending the report.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "crashrpt.h"      // defines LPGETLOGFILE callback
#include "excprpt.h"       // bulk of crash report generation

#ifndef TStrStrVector
#include <vector>

typedef std::pair<string,string> TStrStrPair;
typedef std::vector<TStrStrPair> TStrStrVector;
#endif // !defined TStrStrVector


extern BOOL g_bNoCrashHandler;// don't use the crash handler but let the system handle it

////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CCrashHandler
// 
// See the module comment at top of file.
//
class CCrashHandler  
{
public:

   //-----------------------------------------------------------------------------
   // GetInstance (static)
   //    Returns the instance for the current process. Creates one if necessary.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    none
   //
   static CCrashHandler * GetInstance();


   //-----------------------------------------------------------------------------
   // Install
   //    Installs the crash handler..
   //
   // Parameters
   //    lpfn        Client crash callback
   //    lpcszTo     Email address to send crash report
   //    lpczSubject Subject line to be used with email
   //
   // Return Values
   //    none
   //
   // Remarks
   //    Passing NULL for lpTo will disable the email feature and cause the crash 
   //    report to be saved to disk.
   //
   void Install(
      LPGETLOGFILE lpfn = NULL,           // Client crash callback
      LPCTSTR lpcszTo = NULL,             // EMail:To
      LPCTSTR lpcszSubject = NULL,        // EMail:Subject
      BOOL bUseUI = TRUE
      );

   //-----------------------------------------------------------------------------
   // Unnstall
   //    Removes the crash handler..
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    none
   //
   void Uninstall();

   //-----------------------------------------------------------------------------
   // EnableUI
   //    Enables the UI part
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    none
   //
   void EnableUI();

   //-----------------------------------------------------------------------------
   // DisableUI
   //    Disables the UI part
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    none
   //
   void DisableUI();

   //-----------------------------------------------------------------------------
   // DisableUI
   //    Disables the exception handler
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    none
   //
   void DisableHandler();

   //-----------------------------------------------------------------------------
   // DisableUI
   //    Enables the custom exception handler
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    none
   //
   void EnableHandler();

   //-----------------------------------------------------------------------------
   // ~CCrashHandler
   //    Uninitializes the crashrpt library.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    none
   //
   virtual 
   ~CCrashHandler();

   //-----------------------------------------------------------------------------
   // AddFile
   //    Adds a file to the crash report.
   //
   // Parameters
   //    lpFile      Fully qualified file name
   //    lpDesc      File description
   //
   // Return Values
   //    none
   //
   // Remarks
   //    Call this function to include application specific file(s) in the crash
   //    report.  For example, application logs, initialization files, etc.
   //
   void 
   AddFile(
      LPCTSTR lpFile,                     // File nae
      LPCTSTR lpDesc                      // File description
      );

   //-----------------------------------------------------------------------------
   // RemoveFile
   //    Removes a file from the crash report.
   //
   // Parameters
   //    lpFile      Fully qualified file name
   //
   // Return Values
   //    none
   //
   // Remarks
   //    lpFile must exactly match that passed to AddFile.
   //
   void 
   RemoveFile(
      LPCTSTR lpFile                      // File nae
      );

   //-----------------------------------------------------------------------------
   // AddRegistryHive
   //    Adds a registry hive to the crash report.
   //
   // Parameters
   //    lpKey       Fully registry eky
   //    lpDesc      Description
   //
   // Return Values
   //    none
   //
   // Remarks
   //    Call this function to include application specific registry hive(s) in the crash
   //    report.
   //
   void 
   AddRegistryHive(
      LPCTSTR lpKey,                      // Registry key
      LPCTSTR lpDesc                      // description
      );

   //-----------------------------------------------------------------------------
   // RemoveRegistryHive
   //    Removes a registry hive from the crash report.
   //
   // Parameters
   //    lpKey       Full registry key
   //
   // Return Values
   //    none
   //
   // Remarks
   //    lpKey must exactly match that passed to AddRegistryHive.
   //
   void 
   RemoveRegistryHive(
      LPCTSTR lpKey                       // Registry key
      );

   //-----------------------------------------------------------------------------
   // AddEventLog
   //    Adds an event log to the crash report.
   //
   // Parameters
   //    lpKey       Event log name ("Application", "System", "Security")
   //    lpDesc      Description
   //
   // Return Values
   //    none
   //
   // Remarks
   //    Call this function to include application specific registry hive(s) in the crash
   //    report.
   //
   void 
   AddEventLog(
      LPCTSTR lpKey,                      // Event log name
      LPCTSTR lpDesc                      // description
      );

   //-----------------------------------------------------------------------------
   // RemoveEventLog
   //    Removes an event log from the crash report.
   //
   // Parameters
   //    lpKey       Event log name
   //
   // Return Values
   //    none
   //
   // Remarks
   //    lpKey must exactly match that passed to AddEventLog.
   //
   void 
   RemoveEventLog(
      LPCTSTR lpKey                       // Registry key
      );

   //-----------------------------------------------------------------------------
   // GenerateErrorReport
   //    Produces a crash report.
   //
   // Parameters
   //    pExInfo     Pointer to an EXCEPTION_POINTERS structure
   //
   // Return Values
   //    BOOL        TRUE if exception to be executed; FALSE
   //                if to search for another handler. This
   //                should be used to allow breaking into
   //                the debugger, where appropriate.
   //
   // Remarks
   //    Call this function to manually generate a crash report.
   //
   BOOL 
   GenerateErrorReport(
      PEXCEPTION_POINTERS pExInfo,         // Exception pointers (see MSDN)
	  BSTR message = NULL
      );


protected:

   //-----------------------------------------------------------------------------
   // CCrashHandler
   //    Initializes the library and optionally set the client crash callback and
   //    sets up the email details.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    none
   //
   // Remarks
   //    Passing NULL for lpTo will disable the email feature and cause the crash 
   //    report to be saved to disk.
   //
   CCrashHandler(
      );

   //-----------------------------------------------------------------------------
   // SaveReport
   //    Presents the user with a file save dialog and saves the crash report
   //    file to disk.  This function is called if an Email:To was not provided
   //    in the constructor.
   //
   // Parameters
   //    rpt         The report details
   //    lpcszFile   The zipped crash report
   //
   // Return Values
   //    True is successful.
   //
   // Remarks
   //    none
   //
   BOOL 
   SaveReport(
      CExceptionReport &rpt, 
      LPCTSTR lpcszFile
      );

   //-----------------------------------------------------------------------------
   // MailReport
   //    Mails the zipped crash report to the address specified.
   //
   // Parameters
   //    rpt         The report details
   //    lpcszFile   The zipped crash report
   //    lpcszEmail  The Email:To
   //    lpcszDesc   
   //
   // Return Values
   //    TRUE is successful.
   //
   // Remarks
   //    MAPI is used to send the report.
   //
   BOOL 
   MailReport(
      CExceptionReport &rpt, 
      LPCTSTR lpcszFile, 
      LPCTSTR lpcszEmail, 
      LPCTSTR lpcszSubject
      );

   //-----------------------------------------------------------------------------
   // DialogThreadExecute
   //    Displays the dialog and handles the user's reply. Executed as a separate
   //    thread.
   //
   // Parameters
   //    pParam      Standard CreateThreadParameter; set to pointer to CCrashHandler
   //
   // Return Values
   //    none
   //
   // Remarks
   //    Started from GenerateErrorReport via CreateThread. This ensures the caller
   //    is stopped (and will not confuse state by dispatching messages).
   //
   static DWORD WINAPI CCrashHandler::DialogThreadExecute(LPVOID pParam);

   string LoadResourceString(UINT id);
   LPTOP_LEVEL_EXCEPTION_FILTER  m_oldFilter;      // previous exception filter
   LPGETLOGFILE                  m_lpfnCallback;   // client crash callback
   int                           m_pid;            // process id
   TStrStrVector                 m_files;          // custom files to add
   TStrStrVector				 m_registryHives;  // custom registry hives to save
   TStrStrVector				 m_eventLogs;      // custom event logs to save
   string						 m_sTo;            // Email:To
   string                        m_sSubject;       // Email:Subject
   HANDLE                        m_ipc_event;      // Event for dialog thread synchronization
   CExceptionReport              *m_rpt;            // Exception report for dialog
   bool                          m_installed;      // True if already installed
   HMODULE                       m_hModule;        // Module handle for loading resource strings
   string						 m_userDataFile;   // file to save user input when m_sTo is empty
   bool                          m_wantDebug;      // user pushed Debug button
   BOOL							 m_bUseUI;		   // use an UI or print to the error output
};

