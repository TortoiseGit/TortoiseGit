///////////////////////////////////////////////////////////////////////////////
//
//  Module: excprpt.h
//
//    Desc: This class generates the dump and xml overview files.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <dbghelp.h>
// STL generates various warnings.
// 4100: unreferenced formal parameter
// 4663: C++ language change: to explicitly specialize class template...
// 4018: signed/unsigned mismatch
// 4245: conversion from <a> to <b>: signed/unsigned mismatch
#pragma warning(push, 3)
#pragma warning(disable: 4100)
#pragma warning(disable: 4663)
#pragma warning(disable: 4018)
#pragma warning(disable: 4245)
#include <vector>
#pragma warning(pop)

// Import MSXML interfaces
#import "msxml6.dll" named_guids raw_interfaces_only

//
// COM helper macros
//
#define CHECKHR(x) {HRESULT hr = x; if (FAILED(hr)) goto CleanUp;}
#define SAFERELEASE(p) {if (p) {(p)->Release(); p = NULL;}}


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CExceptionReport
// 
// See the module comment at top of file.
//
class CExceptionReport  
{
public:
	CExceptionReport(PEXCEPTION_POINTERS ExceptionInfo, BSTR message);

   string getSymbolFile(int index);
	int getNumSymbolFiles();
	string getCrashLog();
	string getCrashFile();
   string getModuleName() { return m_sModule; };
   string getExceptionCode() { return m_sException; };
   string getExceptionAddr() { return m_sAddress; };

private:
   string m_sCommandLine;
   string m_sModule;
   string m_sException;
   string m_sAddress;

   PEXCEPTION_POINTERS m_excpInfo;
   BSTR           m_message;
   vector<string> m_symFiles;

   // used by stack walkback
   MSXML2::IXMLDOMElement* m_stack_element;
   MSXML2::IXMLDOMDocument* m_stack_doc;
   int m_frameNumber;

   // used by exception node creation, symbol translations
   MSXML2::IXMLDOMElement*  m_exception_element;

   // used by dump callback
   std::vector<MINIDUMP_MODULE_CALLBACK>	m_modules;

   static void writeDumpFile(HANDLE file, PEXCEPTION_POINTERS m_excpInfo, void *data);

   MSXML2::IXMLDOMNode* CreateDOMNode(MSXML2::IXMLDOMDocument* pDoc, 
                                            int type, 
                                            BSTR bstrName);

   MSXML2::IXMLDOMNode* CreateExceptionRecordNode(MSXML2::IXMLDOMDocument* pDoc, 
                                                        EXCEPTION_RECORD* pExceptionRecord);
   static void CreateExceptionSymbolAttributes(DWORD_PTR address, const char *ImageName,
									  const char *FunctionName, DWORD_PTR functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data);

   MSXML2::IXMLDOMNode* CreateProcessorNode(MSXML2::IXMLDOMDocument* pDoc);

   MSXML2::IXMLDOMNode* CreateOSNode(MSXML2::IXMLDOMDocument* pDoc);

   MSXML2::IXMLDOMNode* CreateModulesNode(MSXML2::IXMLDOMDocument* pDoc);

   MSXML2::IXMLDOMNode* CreateMsgNode(MSXML2::IXMLDOMDocument* pDoc, BSTR message);

   MSXML2::IXMLDOMNode* CreateWalkbackNode(MSXML2::IXMLDOMDocument* pDoc, CONTEXT *pContext);

   static void CreateWalkbackEntryNode(DWORD_PTR address, const char *ImageName,
									  const char *FunctionName, DWORD_PTR functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data);

   static BOOL CALLBACK miniDumpCallback(PVOID CallbackParam,
                                         CONST PMINIDUMP_CALLBACK_INPUT CallbackInput,
                                         PMINIDUMP_CALLBACK_OUTPUT CallbackOutput);
};

