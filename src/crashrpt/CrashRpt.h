///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashRpt.h
//
//    Desc: Defines the interface for the CrashRpt.DLL.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _CRASHRPT_H_
#define _CRASHRPT_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include <windows.h>
#include <wtypes.h>     // BSTR

// CrashRpt.h
#ifdef CRASHRPTAPI

// CRASHRPTAPI should be defined in all of the DLL's source files as
// #define CRASHRPTAPI extern "C" __declspec(dllexport)

#else

// This header file is included by an EXE - export
#define CRASHRPTAPI extern "C" __declspec(dllimport)

#endif

// Client crash callback
typedef BOOL (CALLBACK *LPGETLOGFILE) (LPVOID lpvState);
// Stack trace callback
typedef void (*TraceCallbackFunction)(DWORD_PTR address, const char *ImageName,
									  const char *FunctionName, DWORD_PTR functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data);


//-----------------------------------------------------------------------------
// GetInstance
//    Returns the instance (state information) for the current process. Will create
//    one if required; does not install.
//
// Parameters
//    none
//
// Return Values
//    If the function succeeds, the return value is a pointer to the underlying
//    crash object created.  This state information is required as the first
//    parameter to all other crash report functions.
//
// Remarks
//    none
//
CRASHRPTAPI 
LPVOID 
GetInstance();

//-----------------------------------------------------------------------------
// Install, InstallEx
//    Initializes the library and optionally set the client crash callback and
//    sets up the email details.
//
// Parameters
//    pfn         Client crash callback
//    lpTo        Email address to send crash report
//    lpSubject   Subject line to be used with email
//
// Return Values
//   InstallEx:
//    If the function succeeds, the return value is a pointer to the underlying
//    crash object created.  This state information is required as the first
//    parameter to all other crash report functions.
//   Install:
//    void
//
// Remarks
//    Passing NULL for lpTo will disable the email feature and cause the crash 
//    report to be saved to disk.
//
CRASHRPTAPI
void
Install(
   IN LPGETLOGFILE pfn OPTIONAL,               // client crash callback
   IN LPCSTR lpTo OPTIONAL,                    // Email:to
   IN LPCSTR lpSubject OPTIONAL,               // Email:subject
   IN BOOL bUseUI OPTIONAL                     // UI or console
   );
CRASHRPTAPI 
LPVOID 
InstallEx(
   IN LPGETLOGFILE pfn OPTIONAL,               // client crash callback
   IN LPCSTR lpTo OPTIONAL,                    // Email:to
   IN LPCSTR lpSubject OPTIONAL,               // Email:subject
   IN BOOL bUseUI OPTIONAL                     // UI or console
   );

//-----------------------------------------------------------------------------
// Uninstall, UninstallEx
//    Uninstalls the unhandled exception filter set up in Install().
//
// Parameters
//    lpState     State information returned from Install()
//
// Return Values
//    void
//
// Remarks
//    This call is optional.  The crash report library will automatically 
//    deinitialize when the library is unloaded.  Call this function to
//    unhook the exception filter manually.
//
CRASHRPTAPI void Uninstall();
CRASHRPTAPI 
void 
UninstallEx(
   IN LPVOID lpState                            // State from Install()
   );

CRASHRPTAPI void EnableUI();
CRASHRPTAPI void DisableUI();
//-----------------------------------------------------------------------------
// AddFile, AddFileEx
//    Adds a file to the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    lpFile      Fully qualified file name
//    lpDesc      Description of file, used by details dialog
//
// Return Values
//    void
//
// Remarks
//    This function can be called anytime after Install() to add one or more
//    files to the generated crash report. If lpFile and lpDesc exactly match
//    a previously added pair, it is not added again.
//
CRASHRPTAPI
void
AddFile(
   IN LPCSTR lpFile,                           // File name
   IN LPCSTR lpDesc                            // File desc
   );
CRASHRPTAPI 
void 
AddFileEx(
   IN LPVOID lpState,                           // State from Install()
   IN LPCSTR lpFile,                           // File name
   IN LPCSTR lpDesc                            // File desc
   );

//-----------------------------------------------------------------------------
// RemoveFile
//    Removes a file from the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    lpFile      Fully qualified file name
//
// Return Values
//    void
//
// Remarks
//    The filename must exactly match that provided to AddFile.
//
CRASHRPTAPI
void
RemoveFile(
   IN LPCSTR lpFile                            // File name
   );
CRASHRPTAPI 
void 
RemoveFileEx(
   IN LPVOID lpState,                           // State from Install()
   IN LPCSTR lpFile                            // File name
   );

//-----------------------------------------------------------------------------
// AddRegistryHive
//    Adds a RegistryHive to the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    lpRegistryHive      Fully qualified RegistryHive name
//    lpDesc      Description of RegistryHive, used by details dialog
//
// Return Values
//    void
//
// Remarks
//    This function can be called anytime after Install() to add one or more
//    RegistryHives to the generated crash report. If lpRegistryHive and lpDesc exactly match
//    a previously added pair, it is not added again.
//
CRASHRPTAPI
void
AddRegistryHive(
   IN LPCSTR lpRegistryHive,                   // RegistryHive name
   IN LPCSTR lpDesc                            // RegistryHive desc
   );
CRASHRPTAPI 
void 
AddRegistryHiveEx(
   IN LPVOID lpState,                           // State from Install()
   IN LPCSTR lpRegistryHive,                   // RegistryHive name
   IN LPCSTR lpDesc                            // RegistryHive desc
   );

//-----------------------------------------------------------------------------
// RemoveRegistryHive
//    Removes a RegistryHive from the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    lpRegistryHive      Fully qualified RegistryHive name
//
// Return Values
//    void
//
// Remarks
//    The RegistryHive name must exactly match that provided to AddRegistryHive.
//
CRASHRPTAPI
void 
RemoveRegistryHive(
   IN LPCSTR lpRegistryHive                    // RegistryHive name
   );
CRASHRPTAPI 
void 
RemoveRegistryHiveEx(
   IN LPVOID lpState,                           // State from Install()
   IN LPCSTR lpRegistryHive                    // RegistryHive name
   );

//-----------------------------------------------------------------------------
// AddEventLog
//    Adds an event log to the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    lpEventLog  Event log name ("Application", "Security", "System" or any other known to your system)
//    lpDesc      Description of event log, used by details dialog
//
// Return Values
//    void
//
// Remarks
//    This function can be called anytime after Install() to add one or more
//    event logs to the generated crash report. If lpEventLog and lpDesc exactly match
//    a previously added pair, it is not added again.
//
CRASHRPTAPI
void
AddEventLog(
   IN LPCSTR lpEventLog,                       // Event Log name
   IN LPCSTR lpDesc                            // Event Log desc
   );
CRASHRPTAPI 
void 
AddEventLogEx(
   IN LPVOID lpState,                           // State from Install()
   IN LPCSTR lpEventLog,                       // Event Log name
   IN LPCSTR lpDesc                            // Event Log desc
   );

//-----------------------------------------------------------------------------
// RemoveEventLog
//    Removes a EventLog from the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    lpEventLog      Fully qualified EventLog name
//
// Return Values
//    void
//
// Remarks
//    The EventLog name must exactly match that provided to AddEventLog.
//
CRASHRPTAPI
void 
RemoveEventLog(
   IN LPCSTR lpEventLog                    // EventLog name
   );
CRASHRPTAPI 
void 
RemoveEventLogEx(
   IN LPVOID lpState,                           // State from Install()
   IN LPCSTR lpEventLog                    // EventLog name
   );

//-----------------------------------------------------------------------------
// GenerateErrorReport, GenerateErrorReportEx
//    Generates the crash report.
//
// Parameters
//    lpState     State information returned from Install()
//    pExInfo     Optional; pointer to an EXCEPTION_POINTERS structure
//	  message     Optional; message to include in report
//
// Return Values
//    void
//
// Remarks
//    Call this function to manually generate a crash report.
//    Note that only GenerateErrorReportEx can be supplied exception information.
//    If you are using the basic interfaces and wish to supply exception information,
//    use the call GenerateErrorReportEx, supplying GetInstance() for the state
//    information.
//
CRASHRPTAPI
void
GenerateErrorReport(
   IN BSTR message OPTIONAL
   );
CRASHRPTAPI 
void 
GenerateErrorReportEx(
   IN LPVOID lpState,
   IN PEXCEPTION_POINTERS pExInfo OPTIONAL,
   IN BSTR message OPTIONAL
   );

//-----------------------------------------------------------------------------
// StackTrace
//    Creates a stack trace.
//
// Parameters
//    numSkip     Number of initial stack frames to skip
//    depth       Number of stack frames to process
//	  pFunction   Optional; function to call for each frame
//    pContext    Optional; stack context to trace
//
// Return Values
//    void
//
// Remarks
//    Call this function to manually generate a stack trace. If
//    pFunction is not supplied, stack trace frames are output using
//    OutputDebugString. Note that this function does not require the
//    'lpState'; it can be called even if the crash handler is not
//     installed.
//
CRASHRPTAPI
void
StackTrace(
   IN int numSkip,
   IN int depth OPTIONAL,
   IN TraceCallbackFunction pFunction OPTIONAL,
   IN CONTEXT * pContext OPTIONAL,
   IN LPVOID data OPTIONAL
   );


//-----------------------------------------------------------------------------
// The following functions are identical to the above save that they are callable from Visual Basic
extern "C"
LPVOID
__stdcall
InstallExVB(
   IN LPGETLOGFILE pfn OPTIONAL,                // client crash callback
   IN LPCSTR lpTo OPTIONAL,                    // Email:to
   IN LPCSTR lpSubject OPTIONAL,               // Email:subject
   IN BOOL bUseUI OPTIONAL                     // UI or console
   );


extern "C"
void
__stdcall
UninstallExVB(
   IN LPVOID lpState                            // State from InstallVB()
   );


extern "C"
void
__stdcall
EnableUIVB();

extern "C"
void
__stdcall
DisableUIVB();

extern "C"
void
__stdcall
AddFileExVB(
   IN LPVOID lpState,                           // State from InstallVB()
   IN LPCSTR lpFile,                           // File name
   IN LPCSTR lpDesc                            // File desc
   );

extern "C"
void
__stdcall
RemoveFileExVB(
   IN LPVOID lpState,                           // State from InstallVB()
   IN LPCSTR lpFile                            // File name
   );

extern "C"
void
__stdcall
AddRegistryHiveExVB(
   IN LPVOID lpState,                           // State from InstallVB()
   IN LPCSTR lpRegistryHive,                   // RegistryHive name
   IN LPCSTR lpDesc                            // RegistryHive desc
   );

extern "C"
void
__stdcall
RemoveRegistryHiveExVB(
   IN LPVOID lpState,                           // State from InstallVB()
   IN LPCSTR lpRegistryHive                    // RegistryHive name
   );

extern "C"
void
__stdcall
GenerateErrorReportExVB(
   IN LPVOID lpState,
   IN PEXCEPTION_POINTERS pExInfo OPTIONAL,
   IN BSTR message OPTIONAL
   );

extern "C"
void
__stdcall
StackTraceVB(
   IN int numSkip,
   IN int depth OPTIONAL,
   IN TraceCallbackFunction pFunction OPTIONAL,
   IN CONTEXT * pContext OPTIONAL,
   IN LPVOID data OPTIONAL
   );

extern "C" void __stdcall InstallVB(IN LPGETLOGFILE pfn OPTIONAL, IN LPCSTR lpTo OPTIONAL, IN LPCSTR lpSubject OPTIONAL, IN BOOL bUseUI OPTIONAL);
extern "C" void __stdcall UninstallVB();
extern "C" void __stdcall AddFileVB(LPCSTR file, LPCSTR desc);
extern "C" void __stdcall RemoveFileVB(LPCSTR file);
extern "C" void __stdcall AddRegistryHiveVB(LPCSTR RegistryHive, LPCSTR desc);
extern "C" void __stdcall RemoveRegistryHiveVB(LPCSTR RegistryHive);
extern "C" void __stdcall GenerateErrorReportVB(BSTR message);

#endif
