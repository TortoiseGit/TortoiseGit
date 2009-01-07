///////////////////////////////////////////////////////////////////////////////
//
//  Module: CrashHandler.cpp
//
//    Desc: See CrashHandler.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CrashHandler.h"
#include "zlibcpp.h"
#include "excprpt.h"
#include "maindlg.h"
#include "mailmsg.h"
#include "WriteRegistry.h"
#include "resource.h"

#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#pragma comment(lib, "shlwapi")
#pragma comment(lib, "comctl32")

#include <algorithm>

BOOL g_bNoCrashHandler;// don't use the crash handler but let the system handle it

// maps crash objects to processes
map<DWORD, CCrashHandler*> _crashStateMap;

// unhandled exception callback set with SetUnhandledExceptionFilter()
LONG WINAPI CustomUnhandledExceptionFilter(PEXCEPTION_POINTERS pExInfo)
{
	OutputDebugString("Exception\n");
   if (EXCEPTION_BREAKPOINT == pExInfo->ExceptionRecord->ExceptionCode)
   {
	   // Breakpoint. Don't treat this as a normal crash.
	   return EXCEPTION_CONTINUE_SEARCH;
   }

   if (g_bNoCrashHandler)
   {
	   return EXCEPTION_CONTINUE_SEARCH;
   }

   BOOL result = false;
   if (_crashStateMap.find(GetCurrentProcessId()) != _crashStateMap.end())
	result = _crashStateMap[GetCurrentProcessId()]->GenerateErrorReport(pExInfo, NULL);

   // If we're in a debugger, return EXCEPTION_CONTINUE_SEARCH to cause the debugger to stop;
   // or if GenerateErrorReport returned FALSE (i.e. drop into debugger).
   return (!result || IsDebuggerPresent()) ? EXCEPTION_CONTINUE_SEARCH : EXCEPTION_EXECUTE_HANDLER;
}

CCrashHandler * CCrashHandler::GetInstance()
{
	CCrashHandler *instance = NULL;
	if (_crashStateMap.find(GetCurrentProcessId()) != _crashStateMap.end())
		instance = _crashStateMap[GetCurrentProcessId()];
	if (instance == NULL) {
		// will register
		instance = new CCrashHandler();
	}
	return instance;
}

CCrashHandler::CCrashHandler():
	m_oldFilter(NULL),
	m_lpfnCallback(NULL),
	m_pid(GetCurrentProcessId()),
	m_ipc_event(NULL),
	m_rpt(NULL),
	m_installed(false),
	m_hModule(NULL),
	m_bUseUI(TRUE),
	m_wantDebug(false)
{
   // wtl initialization stuff...
	HRESULT hRes = ::CoInitialize(NULL);
	if (hRes != S_OK)
		m_pid = 0;
	else
		_crashStateMap[m_pid] = this;
}

void CCrashHandler::Install(LPGETLOGFILE lpfn, LPCTSTR lpcszTo, LPCTSTR lpcszSubject, BOOL bUseUI)
{
	if (m_pid == 0)
		return;
#ifdef _DEBUG
	OutputDebugString("::Install\n");
#endif
	if (m_installed) {
		Uninstall();
	}
   // save user supplied callback
   m_lpfnCallback = lpfn;
   // save optional email info
   m_sTo = lpcszTo;
   m_sSubject = lpcszSubject;
   m_bUseUI = bUseUI;

   // This is needed for CRT to not show dialog for invalid param
   // failures and instead let the code handle it.
   _CrtSetReportMode(_CRT_ASSERT, 0);
   // add this filter in the exception callback chain
   m_oldFilter = SetUnhandledExceptionFilter(CustomUnhandledExceptionFilter);
/*m_oldErrorMode=*/ SetErrorMode( SEM_FAILCRITICALERRORS );

   m_installed = true;
}

void CCrashHandler::Uninstall()
{
#ifdef _DEBUG
	OutputDebugString("Uninstall\n");
#endif
   // reset exception callback (to previous filter, which can be NULL)
   SetUnhandledExceptionFilter(m_oldFilter);
   m_installed = false;
}

void CCrashHandler::EnableUI()
{
	m_bUseUI = TRUE;
}

void CCrashHandler::DisableUI()
{
	m_bUseUI = FALSE;
}

void CCrashHandler::DisableHandler()
{
	g_bNoCrashHandler = TRUE;
}

void CCrashHandler::EnableHandler()
{
	g_bNoCrashHandler = FALSE;
}

CCrashHandler::~CCrashHandler()
{

	Uninstall();

	_crashStateMap.erase(m_pid);

	::CoUninitialize();

}

void CCrashHandler::AddFile(LPCTSTR lpFile, LPCTSTR lpDesc)
{
   // make sure we don't already have the file
   RemoveFile(lpFile);
   // make sure the file exists
   HANDLE hFile = ::CreateFile(
					 lpFile,
					 GENERIC_READ,
					 FILE_SHARE_READ | FILE_SHARE_WRITE,
					 NULL,
					 OPEN_EXISTING,
					 FILE_ATTRIBUTE_NORMAL,
					 0);
   if (hFile != INVALID_HANDLE_VALUE)
   {
	  // add file to report
	  m_files.push_back(TStrStrPair(lpFile, lpDesc));
	  ::CloseHandle(hFile);
   }
}

void CCrashHandler::RemoveFile(LPCTSTR lpFile)
{
	TStrStrVector::iterator iter;
	for (iter = m_files.begin(); iter != m_files.end(); ++iter) {
		if ((*iter).first == lpFile) {
			iter = m_files.erase(iter);
			break;
		}
	}
}

void CCrashHandler::AddRegistryHive(LPCTSTR lpRegistryHive, LPCTSTR lpDesc)
{
   // make sure we don't already have the RegistryHive
   RemoveRegistryHive(lpRegistryHive);
   // Unfortunately, we have no easy way of verifying the existence of
   // the registry hive.
   // add RegistryHive to report
   m_registryHives.push_back(TStrStrPair(lpRegistryHive, lpDesc));
}

void CCrashHandler::RemoveRegistryHive(LPCTSTR lpRegistryHive)
{
	TStrStrVector::iterator iter;
	for (iter = m_registryHives.begin(); iter != m_registryHives.end(); ++iter) {
		if ((*iter).first == lpRegistryHive) {
			iter = m_registryHives.erase(iter);
		}
	}
}

void CCrashHandler::AddEventLog(LPCTSTR lpEventLog, LPCTSTR lpDesc)
{
   // make sure we don't already have the EventLog
   RemoveEventLog(lpEventLog);
   // Unfortunately, we have no easy way of verifying the existence of
   // the event log..
   // add EventLog to report
   m_eventLogs.push_back(TStrStrPair(lpEventLog, lpDesc));
}

void CCrashHandler::RemoveEventLog(LPCTSTR lpEventLog)
{
	TStrStrVector::iterator iter;
	for (iter = m_eventLogs.begin(); iter != m_eventLogs.end(); ++iter) {
		if ((*iter).first == lpEventLog) {
			iter = m_eventLogs.erase(iter);
		}
	}
}

DWORD WINAPI CCrashHandler::DialogThreadExecute(LPVOID pParam)
{
   // New thread. This will display the dialog and handle the result.
   CCrashHandler * self = reinterpret_cast<CCrashHandler *>(pParam);
   CMainDlg          mainDlg;
   string			 sTempFileName = CUtility::getTempFileName();
   CZLib             zlib;

   // delete existing copy, if any
   DeleteFile(sTempFileName.c_str());

   // zip the report
   if (!zlib.Open(sTempFileName))
      return TRUE;
   
   // add report files to zip
   TStrStrVector::iterator cur = self->m_files.begin();
   for (cur = self->m_files.begin(); cur != self->m_files.end(); cur++)
     if (PathFileExists((*cur).first.c_str()))
      zlib.AddFile((*cur).first);

   zlib.Close();

   mainDlg.m_pUDFiles = &self->m_files;
   mainDlg.m_sendButton = !self->m_sTo.empty();

   INITCOMMONCONTROLSEX used = {
	   sizeof(INITCOMMONCONTROLSEX),
	   ICC_LISTVIEW_CLASSES | ICC_WIN95_CLASSES | ICC_BAR_CLASSES | ICC_USEREX_CLASSES
   };
   InitCommonControlsEx(&used);

   INT_PTR status = mainDlg.DoModal(GetModuleHandle("CrashRpt.dll"), IDD_MAINDLG, GetDesktopWindow());
   if (IDOK == status || IDC_SAVE == status)
   {
      if (IDC_SAVE == status || self->m_sTo.empty() || 
          !self->MailReport(*self->m_rpt, sTempFileName.c_str(), mainDlg.m_sEmail.c_str(), mainDlg.m_sDescription.c_str()))
      {
		  // write user data to file if to be supplied
		  if (!self->m_userDataFile.empty()) {
			   HANDLE hFile = ::CreateFile(
								 self->m_userDataFile.c_str(),
								 GENERIC_READ | GENERIC_WRITE,
								 FILE_SHARE_READ | FILE_SHARE_WRITE,
								 NULL,
								 CREATE_ALWAYS,
								 FILE_ATTRIBUTE_NORMAL,
								 0);
			   if (hFile != INVALID_HANDLE_VALUE)
			   {
				   static const char e_mail[] = "E-mail:";
				   static const char newline[] = "\r\n";
				   static const char description[] = "\r\n\r\nDescription:";
				   DWORD writtenBytes;
				   ::WriteFile(hFile, e_mail, sizeof(e_mail) - 1, &writtenBytes, NULL);
				   ::WriteFile(hFile, mainDlg.m_sEmail.c_str(), mainDlg.m_sEmail.size(), &writtenBytes, NULL);
				   ::WriteFile(hFile, description, sizeof(description) - 1, &writtenBytes, NULL);
				   ::WriteFile(hFile, mainDlg.m_sDescription.c_str(), mainDlg.m_sDescription.size(), &writtenBytes, NULL);
				   ::WriteFile(hFile, newline, sizeof(newline) - 1, &writtenBytes, NULL);
				   ::CloseHandle(hFile);
				  // redo zip file to add user data
				   // delete existing copy, if any
				   DeleteFile(sTempFileName.c_str());

				   // zip the report
				   if (!zlib.Open(sTempFileName))
					  return TRUE;
   
				   // add report files to zip
				   TStrStrVector::iterator cur = self->m_files.begin();
				   for (cur = self->m_files.begin(); cur != self->m_files.end(); cur++)
					   if (PathFileExists((*cur).first.c_str()))
						zlib.AddFile((*cur).first);

				   zlib.Close();
			   }
		  }
         self->SaveReport(*self->m_rpt, sTempFileName.c_str());
      }
   }

   DeleteFile(sTempFileName.c_str());

   self->m_wantDebug = IDC_DEBUG == status;
   // signal main thread to resume
   ::SetEvent(self->m_ipc_event);
   // set flag for debugger break

   // exit thread
   ::ExitThread(0);
   // keep compiler happy.
   return TRUE;
}

BOOL CCrashHandler::GenerateErrorReport(PEXCEPTION_POINTERS pExInfo, BSTR message)
{
   CExceptionReport  rpt(pExInfo, message);
   unsigned int      i;
   // save state of file list prior to generating report
   TStrStrVector     save_m_files = m_files;
	char temp[_MAX_PATH];

	GetTempPath(sizeof temp, temp);


   // let client add application specific files to report
   if (m_lpfnCallback && !m_lpfnCallback(this))
      return TRUE;

   m_rpt = &rpt;

   // if no e-mail address, add file to contain user data
   m_userDataFile = "";
   if (m_sTo.empty()) {
	   m_userDataFile = temp + string("\\") + CUtility::getAppName() + "_UserInfo.txt";
	   HANDLE hFile = ::CreateFile(
						 m_userDataFile.c_str(),
						 GENERIC_READ | GENERIC_WRITE,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 NULL,
						 CREATE_ALWAYS,
						 FILE_ATTRIBUTE_NORMAL,
						 0);
	   if (hFile != INVALID_HANDLE_VALUE)
	   {
		   static const char description[] = "Your e-mail and description will go here.";
		   DWORD writtenBytes;
		   ::WriteFile(hFile, description, sizeof(description)-1, &writtenBytes, NULL);
		   ::CloseHandle(hFile);
		   m_files.push_back(TStrStrPair(m_userDataFile, LoadResourceString(IDS_USER_DATA)));
	   } else {
		   return m_wantDebug;
	   }
   }



   // add crash files to report
   string crashFile = rpt.getCrashFile();
   string crashLog = rpt.getCrashLog();
   m_files.push_back(TStrStrPair(crashFile, LoadResourceString(IDS_CRASH_DUMP)));
   m_files.push_back(TStrStrPair(crashLog, LoadResourceString(IDS_CRASH_LOG)));

   // Export registry hives and add to report
   std::vector<string> extraFiles;
   string file;
   string number;
   TStrStrVector::iterator iter;
   int n = 0;

   for (iter = m_registryHives.begin(); iter != m_registryHives.end(); iter++) {
	   ++n;
	   TCHAR buf[MAX_PATH] = {0};
	   _tprintf_s(buf, "%d", n);
	   number = buf;
	   file = temp + string("\\") + CUtility::getAppName() + "_registry" + number + ".reg";
	   ::DeleteFile(file.c_str());

	   // we want to export in a readable format. Unfortunately, RegSaveKey saves in a binary
	   // form, so let's use our own function.
	   if (WriteRegistryTreeToFile((*iter).first.c_str(), file.c_str())) {
		   extraFiles.push_back(file);
		   m_files.push_back(TStrStrPair(file, (*iter).second));
	   } else {
		   OutputDebugString("Could not write registry hive\n");
	   }
   }
   //
   // Add the specified event log(s). Note that this will not work on Win9x/WinME.
   //
   for (iter = m_eventLogs.begin(); iter != m_eventLogs.end(); iter++) {
		HANDLE h;
		h = OpenEventLog( NULL,    // use local computer
				 (*iter).first.c_str());   // source name
		if (h != NULL) {

			file = temp + string("\\") + CUtility::getAppName() +  "_" + (*iter).first + ".evt";

			DeleteFile(file.c_str());

			if (BackupEventLog(h, file.c_str())) {
				m_files.push_back(TStrStrPair(file, (*iter).second));
			   extraFiles.push_back(file);
			} else {
				OutputDebugString("could not backup log\n");
			}
			CloseEventLog(h);
		} else {
			OutputDebugString("could not open log\n");
		}
   }
 

   // add symbol files to report
   for (i = 0; i < (UINT)rpt.getNumSymbolFiles(); i++)
      m_files.push_back(TStrStrPair(rpt.getSymbolFile(i).c_str(), 
      string("Symbol File")));
 
   //remove the crash handler, just in case the dialog crashes...
   Uninstall();
   if (m_bUseUI)
   {
	   // Start a new thread to display the dialog, and then wait
	   // until it completes
	   m_ipc_event = ::CreateEvent(NULL, FALSE, FALSE, "ACrashHandlerEvent");
	   if (m_ipc_event == NULL)
		   return m_wantDebug;
	   DWORD threadId;
	   if (::CreateThread(NULL, 0, DialogThreadExecute,
		   reinterpret_cast<LPVOID>(this), 0, &threadId) == NULL)
		   return m_wantDebug;
	   ::WaitForSingleObject(m_ipc_event, INFINITE);
	   CloseHandle(m_ipc_event);
   }
   else
   {
	   string sTempFileName = CUtility::getTempFileName();
	   CZLib             zlib;

	   sTempFileName += _T(".zip");
	   // delete existing copy, if any
	   DeleteFile(sTempFileName.c_str());

	   // zip the report
	   if (!zlib.Open(sTempFileName))
		   return TRUE;

	   // add report files to zip
	   TStrStrVector::iterator cur = m_files.begin();
	   for (cur = m_files.begin(); cur != m_files.end(); cur++)
		   if (PathFileExists((*cur).first.c_str()))
			   zlib.AddFile((*cur).first);
	   zlib.Close();
	   fprintf(stderr, "a zipped crash report has been saved to\n");
	   _ftprintf(stderr, sTempFileName.c_str());
	   fprintf(stderr, "\n");
	   if (!m_sTo.empty())
	   {
		 fprintf(stderr, "please send the report to ");
		 _ftprintf(stderr, m_sTo.c_str());
		 fprintf(stderr, "\n");
	   }
   }
   // clean up - delete files we created
   ::DeleteFile(crashFile.c_str());
   ::DeleteFile(crashLog.c_str());
   if (!m_userDataFile.empty()) {
	   ::DeleteFile(m_userDataFile.c_str());
   }

   std::vector<string>::iterator file_iter;
   for (file_iter = extraFiles.begin(); file_iter != extraFiles.end(); file_iter++) {
	   ::DeleteFile(file_iter->c_str());
   }

   // restore state of file list
   m_files = save_m_files;

   m_rpt = NULL;

   return !m_wantDebug;
}

BOOL CCrashHandler::SaveReport(CExceptionReport&, LPCTSTR lpcszFile)
{
   // let user more zipped report
   return (CopyFile(lpcszFile, CUtility::getSaveFileName().c_str(), TRUE));
}

BOOL CCrashHandler::MailReport(CExceptionReport&, LPCTSTR lpcszFile,
                               LPCTSTR lpcszEmail, LPCTSTR lpcszDesc)
{
   CMailMsg msg;
   msg
      .SetTo(m_sTo)
      .SetFrom(lpcszEmail)
      .SetSubject(m_sSubject.empty()?_T("Incident Report"):m_sSubject)
      .SetMessage(lpcszDesc)
      .AddAttachment(lpcszFile, CUtility::getAppName() + _T(".zip"));

   return (msg.Send());
}

string CCrashHandler::LoadResourceString(UINT id)
{
	static int address;
	char buffer[512];
	if (m_hModule == NULL) {
		m_hModule = GetModuleHandle("CrashRpt.dll");
	}
	buffer[0] = '\0';
	if (m_hModule) {
		LoadString(m_hModule, id, buffer, sizeof buffer);
	}
	return buffer;
}
