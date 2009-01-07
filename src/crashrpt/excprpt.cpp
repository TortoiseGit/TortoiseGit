///////////////////////////////////////////////////////////////////////////////
//
//  Module: excprpt.cpp
//
//    Desc: See excprpt.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////


#include "stdafx.h"
#include "excprpt.h"
#include "utility.h"

#include "StackTrace.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------------
// CExceptionReport::CExceptionReport
//
// 
//
CExceptionReport::CExceptionReport(PEXCEPTION_POINTERS ExceptionInfo, BSTR message)
{
   m_excpInfo = ExceptionInfo;
   m_message = message;
   TCHAR szModName[_MAX_FNAME + 1];
   GetModuleFileName(NULL, szModName, _MAX_FNAME);
   m_sModule = szModName;
   m_sCommandLine = GetCommandLine();
   m_frameNumber = 0;
}


//-----------------------------------------------------------------------------
// CExceptionReport::getCrashFile
//
// Creates the dump file returning the file name
//
string CExceptionReport::getCrashFile()
{
   TCHAR buf[MAX_PATH] = {0};
   _stprintf_s(buf, MAX_PATH, _T("%s\\%s.dmp"), _tgetenv("TEMP"), CUtility::getAppName().c_str());

   // Create the file
   HANDLE hFile = CreateFile(
      buf,
      GENERIC_WRITE,
      0,
      NULL,
      CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL,
      NULL);

   //
   // Write the minidump to the file
   //
   if (hFile)
   {
      writeDumpFile(hFile, m_excpInfo, reinterpret_cast<void *>(this));
   }

   // Close file
   CloseHandle(hFile);

   return string(buf);
}


void CExceptionReport::writeDumpFile(HANDLE hFile, PEXCEPTION_POINTERS excpInfo, void *data)
{
	if (excpInfo == NULL) {
      // Generate exception to get proper context in dump
	   __try {
		   RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
	   } __except(writeDumpFile(hFile, GetExceptionInformation(), data), EXCEPTION_CONTINUE_EXECUTION) {
	   }
	} else {
      MINIDUMP_EXCEPTION_INFORMATION eInfo;
      eInfo.ThreadId = GetCurrentThreadId();
      eInfo.ExceptionPointers = excpInfo;
      eInfo.ClientPointers = FALSE;

      MINIDUMP_CALLBACK_INFORMATION cbMiniDump;
      cbMiniDump.CallbackRoutine = CExceptionReport::miniDumpCallback;
      cbMiniDump.CallbackParam = data;

      MiniDumpWriteDump(
         GetCurrentProcess(),
         GetCurrentProcessId(),
         hFile,
         MiniDumpWithIndirectlyReferencedMemory,
         excpInfo ? &eInfo : NULL,
		 NULL,
         &cbMiniDump);
   }
}

//-----------------------------------------------------------------------------
// CExceptionReport::getCrashLog
//
// Creates the XML log file returning the name
//
string CExceptionReport::getCrashLog()
{
   string sFile;
   MSXML2::IXMLDOMDocument *pDoc  = NULL;
   MSXML2::IXMLDOMNode *root      = NULL;
   MSXML2::IXMLDOMNode *node      = NULL;
   MSXML2::IXMLDOMNode *newNode   = NULL;
   BSTR rootName = ::SysAllocString(L"Exception");
   VARIANT v;

   CoInitialize(NULL);

   // Create an empty XML document
   CHECKHR(CoCreateInstance(
      MSXML2::CLSID_DOMDocument, 
      NULL, 
      CLSCTX_INPROC_SERVER,
      MSXML2::IID_IXMLDOMDocument, 
      (void**)&pDoc));

   // Create root node
   root = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, rootName);

   //
   // Add exception record node
   //
   if (m_excpInfo)
   {
      node = CreateExceptionRecordNode(pDoc, m_excpInfo->ExceptionRecord);
      CHECKHR(root->appendChild(node, &newNode));
      // The XML Document should now own the node.
      SAFERELEASE(node);
      SAFERELEASE(newNode);
   }

   //
   // Add optional message node
   //
   if (m_message != NULL) {
	   node = CreateMsgNode(pDoc, m_message);
	   CHECKHR(root->appendChild(node, &newNode));
	   // The XML Document should now own the node.
	   SAFERELEASE(node);
	   SAFERELEASE(newNode);
   }

   //
   // Add processor node
   //
   node = CreateProcessorNode(pDoc);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);

   //
   // Add OS node
   //
   node = CreateOSNode(pDoc);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);

   //
   // Add modules node
   //
   node = CreateModulesNode(pDoc);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);

   //
   // Add stack walkback node
   //
   node = CreateWalkbackNode(pDoc, m_excpInfo != NULL ? m_excpInfo->ContextRecord : NULL);
   CHECKHR(root->appendChild(node, &newNode));
   // The XML Document should now own the node.
   SAFERELEASE(node);
   SAFERELEASE(newNode);


   // Add the root to the doc
   CHECKHR(pDoc->appendChild(root, NULL));

   //
   // Create dat file name and save
   //
   TCHAR buf[MAX_PATH] = {0};
   _tprintf_s(buf, _T("%s\\%s.xml"), getenv("TEMP"), CUtility::getAppName());
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = CUtility::AllocSysString(buf);
   pDoc->save(v);
   SysFreeString(V_BSTR(&v));

CleanUp:
   SAFERELEASE(pDoc);
   SAFERELEASE(root);
   SAFERELEASE(node);
   SAFERELEASE(newNode);
   SysFreeString(rootName);

   CoUninitialize();

   return sFile;
}


//-----------------------------------------------------------------------------
// CExceptionReport::getNumSymbolFiles
//
// Returns the number of symbols files found
//
int CExceptionReport::getNumSymbolFiles()
{
   return m_symFiles.size();
}


//-----------------------------------------------------------------------------
// CExceptionReport::getSymbolFile
//
// Returns the symbol file name given an index
//
string CExceptionReport::getSymbolFile(int index)
{
   string ret;

   if (0 < index && index < (int)m_symFiles.size())
      ret = m_symFiles[index];

   return ret;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateDOMNode
//
// Helper function 
//
MSXML2::IXMLDOMNode*
CExceptionReport::CreateDOMNode(MSXML2::IXMLDOMDocument* pDoc, 
                                int type, 
                                BSTR bstrName)
{
    MSXML2::IXMLDOMNode * node;
    VARIANT vtype;

    vtype.vt = VT_I4;
    V_I4(&vtype) = (int)type;

    pDoc->createNode(vtype, bstrName, NULL, &node);
    return node;
}

//-----------------------------------------------------------------------------
// CreateExceptionSymbolAttributes
//
// Create attributes in the exception record with the symbolic info, if available
//
void  CExceptionReport::CreateExceptionSymbolAttributes(DWORD_PTR /*address*/, const char * /*ImageName*/,
									  const char *FunctionName, DWORD_PTR functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data)
{
   string sAddr;
   BSTR funcName					= ::SysAllocString(L"FunctionName");
   BSTR funcDispName				= ::SysAllocString(L"FunctionDisplacement");
   BSTR fileName					= ::SysAllocString(L"Filename");
   BSTR lineName					= ::SysAllocString(L"LineNumber");
   BSTR lineDispName				= ::SysAllocString(L"LineDisplacement");
   CExceptionReport	*self = reinterpret_cast<CExceptionReport *>(data);

   VARIANT v;

   // don't need ImageName [module], as that is already done
   if (FunctionName != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(FunctionName);
		self->m_exception_element->setAttribute(funcName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
		TCHAR buf[MAX_PATH] = {0};
		_tprintf_s(buf, offsetFormat, functionDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(buf);
		self->m_exception_element->setAttribute(funcDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }

   if (Filename != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(Filename);
		self->m_exception_element->setAttribute(fileName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		TCHAR buf[MAX_PATH] = {0};
		_tprintf_s(buf, _T("%d"), LineNumber);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(buf);
		self->m_exception_element->setAttribute(lineName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		_tprintf_s(buf, offsetFormat, lineDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(buf);
		self->m_exception_element->setAttribute(lineDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }
   ::SysFreeString(funcName);
   ::SysFreeString(funcDispName);
   ::SysFreeString(fileName);
   ::SysFreeString(lineName);
   ::SysFreeString(lineDispName);
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateExceptionRecordNode
//
//
//
MSXML2::IXMLDOMNode*
CExceptionReport::CreateExceptionRecordNode(MSXML2::IXMLDOMDocument* pDoc, 
                                            EXCEPTION_RECORD* pExceptionRecord)
{
   MSXML2::IXMLDOMNode*     pNode    = NULL;
   MSXML2::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"ExceptionRecord");
   BSTR modName                     = ::SysAllocString(L"ModuleName");
   BSTR codeName                    = ::SysAllocString(L"ExceptionCode");
   BSTR descName                    = ::SysAllocString(L"ExceptionDescription");
   BSTR addrName                    = ::SysAllocString(L"ExceptionAddress");
   BSTR commandlineName				= ::SysAllocString(L"CommandLine");
   
   VARIANT v;
   string sAddr;

   // Create exception record node
   pNode = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

   //
   // Set module name attribute
   //
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = CUtility::AllocSysString(m_sModule);
   pElement->setAttribute(modName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set command line name attribute
   //
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = CUtility::AllocSysString(m_sCommandLine);
   pElement->setAttribute(commandlineName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set exception code
   //
   TCHAR buf[MAX_PATH] = {0};
   _tprintf_s(buf, _T("%#x"), pExceptionRecord->ExceptionCode);
   m_sException = buf;
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = CUtility::AllocSysString(buf);
   pElement->setAttribute(codeName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set exception description
   //
   V_VT(&v)    = VT_BSTR;
   switch (pExceptionRecord->ExceptionCode)
   {
   case EXCEPTION_ACCESS_VIOLATION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_ACCESS_VIOLATION");
      break;
   case EXCEPTION_DATATYPE_MISALIGNMENT:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_DATATYPE_MISALIGNMENT");
      break;
   case EXCEPTION_BREAKPOINT:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_BREAKPOINT");
      break;
   case EXCEPTION_SINGLE_STEP:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_SINGLE_STEP");
      break;
   case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_ARRAY_BOUNDS_EXCEEDED");
      break;
   case EXCEPTION_FLT_DENORMAL_OPERAND:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_DENORMAL_OPERAND");
      break;
   case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_DIVIDE_BY_ZERO");
      break;
   case EXCEPTION_FLT_INEXACT_RESULT:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_INEXACT_RESULT");
      break;
   case EXCEPTION_FLT_INVALID_OPERATION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_INVALID_OPERATION");
      break;
   case EXCEPTION_FLT_OVERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_OVERFLOW");
      break;
   case EXCEPTION_FLT_STACK_CHECK:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_STACK_CHECK");
      break;
   case EXCEPTION_FLT_UNDERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_FLT_UNDERFLOW");
      break;
   case EXCEPTION_INT_DIVIDE_BY_ZERO:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INT_DIVIDE_BY_ZERO");
      break;
   case EXCEPTION_INT_OVERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INT_OVERFLOW");
      break;
   case EXCEPTION_PRIV_INSTRUCTION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_PRIV_INSTRUCTION");
      break;
   case EXCEPTION_IN_PAGE_ERROR:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_IN_PAGE_ERROR");
      break;
   case EXCEPTION_ILLEGAL_INSTRUCTION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_ILLEGAL_INSTRUCTION");
      break;
   case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_NONCONTINUABLE_EXCEPTION");
      break;
   case EXCEPTION_STACK_OVERFLOW:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_STACK_OVERFLOW");
      break;
   case EXCEPTION_INVALID_DISPOSITION:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INVALID_DISPOSITION");
      break;
   case EXCEPTION_GUARD_PAGE:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_GUARD_PAGE");
      break;
   case EXCEPTION_INVALID_HANDLE:
      V_BSTR(&v) = ::SysAllocString(L"EXCEPTION_INVALID_HANDLE");
      break;
   default:
      V_BSTR(&v) = L"EXCEPTION_UNKNOWN";
      break;
   }
   pElement->setAttribute(descName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set exception address
   //
   _tprintf_s(buf, _T("%#x"), pExceptionRecord->ExceptionAddress);
   m_sAddress = sAddr;
   V_VT(&v)    = VT_BSTR;
   V_BSTR(&v)  = CUtility::AllocSysString(buf);
   pElement->setAttribute(addrName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   // Try to include symbolic information
   m_exception_element = pElement;
   AddressToSymbol(reinterpret_cast<DWORD_PTR>(pExceptionRecord->ExceptionAddress)-1,
	   CreateExceptionSymbolAttributes,
	   reinterpret_cast<void *>(this));
CleanUp:
   ::SysFreeString(nodeName);
   ::SysFreeString(modName);
   ::SysFreeString(codeName);
   ::SysFreeString(addrName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateProcessorNode
//
//
//
MSXML2::IXMLDOMNode*
CExceptionReport::CreateProcessorNode(MSXML2::IXMLDOMDocument* pDoc)
{
   MSXML2::IXMLDOMNode*     pNode    = NULL;
   MSXML2::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"Processor");
   BSTR archName                    = ::SysAllocString(L"Architecture");
   BSTR levelName                   = ::SysAllocString(L"Level");
   BSTR numberName                  = ::SysAllocString(L"NumberOfProcessors");
   SYSTEM_INFO si;
   VARIANT v;

   // Create exception record node
   pNode = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

   //
   // Get processor info
   //
   GetSystemInfo(&si);

   //
   // Set architecture
   //
   V_VT(&v) = VT_BSTR;
   switch (si.wProcessorArchitecture)
   {
   case PROCESSOR_ARCHITECTURE_INTEL:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_INTEL");
      break;
   case PROCESSOR_ARCHITECTURE_MIPS:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_MIPS");
      break;
   case PROCESSOR_ARCHITECTURE_ALPHA:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_ALPHA");
      break;
   case PROCESSOR_ARCHITECTURE_PPC:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_PPC");
      break;
   case PROCESSOR_ARCHITECTURE_SHX:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_SHX");
      break;
   case PROCESSOR_ARCHITECTURE_ARM:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_ARM");
      break;
   case PROCESSOR_ARCHITECTURE_IA64:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_IA64");
      break;
   case PROCESSOR_ARCHITECTURE_ALPHA64:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_ALPHA64");
      break;
   case PROCESSOR_ARCHITECTURE_AMD64:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_AMD64");
      break;
   case PROCESSOR_ARCHITECTURE_IA32_ON_WIN64:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_IA32_ON_WIN64");
      break;
   case PROCESSOR_ARCHITECTURE_UNKNOWN:
      V_BSTR(&v) = ::SysAllocString(L"PROCESSOR_ARCHITECTURE_UNKNOWN");
      break;
   default:
      V_BSTR(&v) = ::SysAllocString(L"Unknown");
   }
   pElement->setAttribute(archName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set level
   //
   V_VT(&v) = VT_BSTR;
   if (PROCESSOR_ARCHITECTURE_INTEL == si.wProcessorArchitecture)
   {
      switch (si.wProcessorLevel)
      {
      case 3:
         V_BSTR(&v) = ::SysAllocString(L"Intel 30386");
         break;
      case 4:
         V_BSTR(&v) = ::SysAllocString(L"Intel 80486");
         break;
      case 5:
         V_BSTR(&v) = ::SysAllocString(L"Intel Pentium");
         break;
      case 6:
         V_BSTR(&v) = ::SysAllocString(L"Intel Pentium Pro or Pentium II");
         break;
      default:
         V_BSTR(&v) = ::SysAllocString(L"Unknown");
      }
   }
   pElement->setAttribute(levelName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   //
   // Set num of processors
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = si.dwNumberOfProcessors;
   pElement->setAttribute(numberName, v);

CleanUp:
   ::SysFreeString(nodeName);
   ::SysFreeString(archName);
   ::SysFreeString(levelName);
   ::SysFreeString(numberName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateOSNode
//
//
//
MSXML2::IXMLDOMNode* 
CExceptionReport::CreateOSNode(MSXML2::IXMLDOMDocument* pDoc)
{
   MSXML2::IXMLDOMNode*     pNode    = NULL;
   MSXML2::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"OperatingSystem");
   BSTR majorName                   = ::SysAllocString(L"MajorVersion");
   BSTR minorName                   = ::SysAllocString(L"MinorVersion");
   BSTR buildName                   = ::SysAllocString(L"BuildNumber");
   BSTR csdName                     = ::SysAllocString(L"CSDVersion");
   OSVERSIONINFO oi;
   VARIANT v;

   // Create exception record node
   pNode = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

   //
   // Get OS info
   //
   oi.dwOSVersionInfoSize = sizeof(oi);
   GetVersionEx(&oi);

   //
   // Set major version
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = oi.dwMajorVersion;
   pElement->setAttribute(majorName, v);

   //
   // Set minor version
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = oi.dwMinorVersion;
   pElement->setAttribute(minorName, v);

   //
   // Set build version
   //
   V_VT(&v) = VT_I4;
   V_I4(&v) = oi.dwBuildNumber;
   pElement->setAttribute(buildName, v);

   //
   // Set CSD version
   //
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = CUtility::AllocSysString(oi.szCSDVersion);
   pElement->setAttribute(csdName, v);
   ::SysFreeString(V_BSTR(&v));

CleanUp:
   ::SysFreeString(nodeName);
   ::SysFreeString(majorName);
   ::SysFreeString(minorName);
   ::SysFreeString(buildName);
   ::SysFreeString(csdName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::CreateModulesNode
//
//
//
MSXML2::IXMLDOMNode* 
CExceptionReport::CreateModulesNode(MSXML2::IXMLDOMDocument* pDoc)
{
   MSXML2::IXMLDOMNode*     pNode    = NULL;
   MSXML2::IXMLDOMNode*     pNode2   = NULL;
   MSXML2::IXMLDOMNode*     pNewNode = NULL;
   MSXML2::IXMLDOMElement*  pElement = NULL;
   MSXML2::IXMLDOMElement*  pElement2= NULL;
   BSTR nodeName                    = ::SysAllocString(L"Modules");
   BSTR nodeName2                   = ::SysAllocString(L"Module");
   BSTR fullPath                    = ::SysAllocString(L"FullPath");
   BSTR baseAddrName                = ::SysAllocString(L"BaseAddress");
   BSTR sizeName                    = ::SysAllocString(L"Size");
   BSTR timeStampName               = ::SysAllocString(L"TimeStamp");
   BSTR fileVerName                 = ::SysAllocString(L"FileVersion");
   BSTR prodVerName                 = ::SysAllocString(L"ProductVersion");

   string sAddr;
   VARIANT v;


   // Create modules node
   pNode = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, nodeName);

   //
   // Add module information (freeing storage as we go)
   // 
   MINIDUMP_MODULE_CALLBACK item;
   std::vector<MINIDUMP_MODULE_CALLBACK>::iterator iter;
   for (iter = m_modules.begin(); iter != m_modules.end(); iter++)
   {
	   item = *iter;
      // Create module node
      pNode2 = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, nodeName2);

      // Get element interface
      CHECKHR(pNode2->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

      //
      // Set full path
      //
      V_VT(&v) = VT_BSTR;
	  V_BSTR(&v) = SysAllocString(item.FullPath);
      pElement->setAttribute(fullPath, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set base address
      //
	  TCHAR buf[MAX_PATH] = {0};
	  _tprintf_s(buf, addressFormat, item.BaseOfImage);
      V_VT(&v) = VT_BSTR;
	  V_BSTR(&v) = CUtility::AllocSysString(buf);
      pElement->setAttribute(baseAddrName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set module size
      //
	  _tprintf_s(buf, sizeFormat, item.SizeOfImage);
      V_VT(&v) = VT_BSTR;
	  V_BSTR(&v) = CUtility::AllocSysString(buf);
      pElement->setAttribute(sizeName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set timestamp
      //
      FILETIME    ft = CUtility::getLastWriteFileTime(item.FullPath);
      SYSTEMTIME  st = {0};

      FileTimeToSystemTime(&ft, &st);

	  _tprintf_s(buf, _T("%02u/%02u/%04u %02u:%02u:%02u"), 
		  st.wMonth, 
		  st.wDay, 
		  st.wYear, 
		  st.wHour, 
		  st.wMinute, 
		  st.wSecond);

      V_VT(&v) = VT_BSTR;
	  V_BSTR(&v) = CUtility::AllocSysString(buf);
      pElement->setAttribute(timeStampName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set file version
      //
	  _tprintf_s(buf,"%d.%d.%d.%d", 
		  HIWORD(item.VersionInfo.dwFileVersionMS),
		  LOWORD(item.VersionInfo.dwFileVersionMS),
		  HIWORD(item.VersionInfo.dwFileVersionLS),
		  LOWORD(item.VersionInfo.dwFileVersionLS));

	  V_VT(&v) = VT_BSTR;
	  V_BSTR(&v) = CUtility::AllocSysString(buf);
      pElement->setAttribute(fileVerName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Set product version
      //
	  _tprintf_s(buf, "%d.%d.%d.%d", 
		  HIWORD(item.VersionInfo.dwProductVersionMS),
		  LOWORD(item.VersionInfo.dwProductVersionMS),
		  HIWORD(item.VersionInfo.dwProductVersionLS),
		  LOWORD(item.VersionInfo.dwProductVersionLS));

	  V_VT(&v) = VT_BSTR;
	  V_BSTR(&v) = CUtility::AllocSysString(buf);
      pElement->setAttribute(prodVerName, v);
      // Recycle variant
      SysFreeString(V_BSTR(&v));

      //
      // Append module to modules
      //
      pNode->appendChild(pNode2, &pNewNode);
      // The XML Document should now own the node.
      SAFERELEASE(pNode2);
      SAFERELEASE(pElement2);
      SAFERELEASE(pNewNode);

	  free(item.FullPath);
   }
   m_modules.clear();

CleanUp:

   ::SysFreeString(nodeName);
   ::SysFreeString(nodeName2);
   ::SysFreeString(fullPath);
   ::SysFreeString(baseAddrName);
   ::SysFreeString(sizeName);
   ::SysFreeString(timeStampName);
   ::SysFreeString(fileVerName);
   ::SysFreeString(prodVerName);
   SAFERELEASE(pNode2);
   SAFERELEASE(pNewNode);
   SAFERELEASE(pElement);
   SAFERELEASE(pElement2);

   return pNode;
}

//-----------------------------------------------------------------------------
// CreateMsgNode
//
// Builds the application-defined message node
//
MSXML2::IXMLDOMNode * 
CExceptionReport::CreateMsgNode(MSXML2::IXMLDOMDocument* pDoc, BSTR message)
{
   MSXML2::IXMLDOMNode*     pNode    = NULL;
   MSXML2::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"ApplicationDescription");

   // Create CrashDescription record node
   pNode = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

   pElement->put_text(message);

CleanUp:
   ::SysFreeString(nodeName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CreateWalkbackEntryNode
//
// Create a single node in the stack walback
//
void
CExceptionReport::CreateWalkbackEntryNode(DWORD_PTR address, const char *ImageName,
									  const char *FunctionName, DWORD_PTR functionDisp,
									  const char *Filename, DWORD LineNumber, DWORD lineDisp,
									  void *data)
{
   MSXML2::IXMLDOMNode*     pNode    = NULL;
   MSXML2::IXMLDOMElement*  pElement = NULL;
   MSXML2::IXMLDOMNode*     pNewNode = NULL;
   string sAddr;
   BSTR nodeName                    = ::SysAllocString(L"Frame");
   BSTR frameName					= ::SysAllocString(L"FrameNumber");
   BSTR addrName					= ::SysAllocString(L"ReturnAddress");
   BSTR moduleName					= ::SysAllocString(L"ModuleName");
   BSTR funcName					= ::SysAllocString(L"FunctionName");
   BSTR funcDispName				= ::SysAllocString(L"FunctionDisplacement");
   BSTR fileName					= ::SysAllocString(L"Filename");
   BSTR lineName					= ::SysAllocString(L"LineNumber");
   BSTR lineDispName				= ::SysAllocString(L"LineDisplacement");
   CExceptionReport	*self = reinterpret_cast<CExceptionReport *>(data);

   // Create frame record node
   pNode = self->CreateDOMNode(self->m_stack_doc, MSXML2::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));


   VARIANT v;

   self->m_frameNumber++;
   TCHAR buf[MAX_PATH] = {0};
   _tprintf_s(buf, _T("%d"), self->m_frameNumber);

   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = CUtility::AllocSysString(buf);
   pElement->setAttribute(frameName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   _tprintf_s(buf, offsetFormat, address);
   V_VT(&v) = VT_BSTR;
   V_BSTR(&v) = CUtility::AllocSysString(buf);
   pElement->setAttribute(addrName, v);
   // Recycle variant
   SysFreeString(V_BSTR(&v));

   if (ImageName != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(ImageName);
		pElement->setAttribute(moduleName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }

   if (FunctionName != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(FunctionName);
		pElement->setAttribute(funcName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
		_tprintf_s(buf, offsetFormat, functionDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(buf);
		pElement->setAttribute(funcDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }

   if (Filename != NULL) {
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(Filename);
		pElement->setAttribute(fileName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		_tprintf_s(buf, _T("%d"), LineNumber);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(buf);
		pElement->setAttribute(lineName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));

		_tprintf_s(buf, offsetFormat, lineDisp);
		V_VT(&v) = VT_BSTR;
		V_BSTR(&v) = CUtility::AllocSysString(buf);
		pElement->setAttribute(lineDispName, v);
		// Recycle variant
		SysFreeString(V_BSTR(&v));
   }
   // add to walkback element

   self->m_stack_element->appendChild(pNode, &pNewNode);
   SAFERELEASE(pNewNode);
   // The XML Document should now own the node.
CleanUp:
   SAFERELEASE(pNode);
   SAFERELEASE(pElement);
   ::SysFreeString(nodeName);
   ::SysFreeString(frameName);
   ::SysFreeString(addrName);
   ::SysFreeString(moduleName);
   ::SysFreeString(funcName);
   ::SysFreeString(funcDispName);
   ::SysFreeString(fileName);
   ::SysFreeString(lineName);
   ::SysFreeString(lineDispName);
}

//-----------------------------------------------------------------------------
// CreateWalkbackNode
//
// Builds the stack walkback list
//
MSXML2::IXMLDOMNode * 
CExceptionReport::CreateWalkbackNode(MSXML2::IXMLDOMDocument* pDoc, CONTEXT *pContext)
{
   MSXML2::IXMLDOMNode*     pNode    = NULL;

   MSXML2::IXMLDOMElement*  pElement = NULL;
   BSTR nodeName                    = ::SysAllocString(L"CallStack");

   // Create CallStack record node
   pNode = CreateDOMNode(pDoc, MSXML2::NODE_ELEMENT, nodeName);

   // Get element interface
   CHECKHR(pNode->QueryInterface(MSXML2::IID_IXMLDOMElement, (void**)&pElement));

   // create the trace
   //  set static variables for use by CreateWalkbackEntryNode
   m_stack_element = pElement;
   m_stack_doc = pDoc;
   m_frameNumber = 0;
   // If no context is supplied, skip 1 frames:
   //  1 this function
   //  ??
   DoStackTrace(pContext == NULL ? 1 : 0, 9999, CreateWalkbackEntryNode, pContext, this);

CleanUp:
   ::SysFreeString(nodeName);
   SAFERELEASE(pElement);

   return pNode;
}

//-----------------------------------------------------------------------------
// CExceptionReport::miniDumpCallback
//
// Mini dump module callback.  Hit once for each module processed by
// MiniDumpWriteDump.  Builds a linked list of all module names which is
// eventually used to create the <modules> node in the XML log file.
//
BOOL CALLBACK 
CExceptionReport::miniDumpCallback(PVOID data,
                                   CONST PMINIDUMP_CALLBACK_INPUT CallbackInput,
                                   PMINIDUMP_CALLBACK_OUTPUT)
{
   CExceptionReport *self = reinterpret_cast<CExceptionReport*>(data);
   if (ModuleCallback == CallbackInput->CallbackType)
   {
      MINIDUMP_MODULE_CALLBACK item;

      item = CallbackInput->Module;
      item.FullPath = _wcsdup(CallbackInput->Module.FullPath);
	  self->m_modules.push_back(item);
   }

   return TRUE;
}