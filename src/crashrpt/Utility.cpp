///////////////////////////////////////////////////////////////////////////////
//
//  Module: Utility.cpp
//
//    Desc: See Utility.h
//
// Copyright (c) 2007 TortoiseSVN
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Utility.h"
#include "resource.h"

namespace CUtility {

BSTR CUtility::AllocSysString(string s)
{
#if defined(_UNICODE) || defined(OLE2ANSI)
	BSTR bstr = ::SysAllocStringLen(s.c_str(), s.size());
#else
	int nLen = MultiByteToWideChar(CP_ACP, 0, s.c_str(),
		s.size(), NULL, NULL);
	BSTR bstr = ::SysAllocStringLen(NULL, nLen);
	if(bstr != NULL)
		MultiByteToWideChar(CP_ACP, 0, s.c_str(), s.size(), bstr, nLen);
#endif
	return bstr;
}

FILETIME CUtility::getLastWriteFileTime(string sFile)
{
   FILETIME          ftLocal = {0};
   HANDLE            hFind;
   WIN32_FIND_DATA   ff32;
   hFind = FindFirstFile(sFile.c_str(), &ff32);
   if (INVALID_HANDLE_VALUE != hFind)
   {
      FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
      FindClose(hFind);        
   }
   return ftLocal;
}

FILETIME CUtility::getLastWriteFileTime(WCHAR * wszFile)
{
	FILETIME          ftLocal = {0};
	HANDLE            hFind;
	WIN32_FIND_DATAW  ff32;
	hFind = FindFirstFileW(wszFile, &ff32);
	if (INVALID_HANDLE_VALUE != hFind)
	{
		FileTimeToLocalFileTime(&(ff32.ftLastWriteTime), &ftLocal);
		FindClose(hFind);        
	}
	return ftLocal;
}

string CUtility::getAppName()
{
   TCHAR szFileName[_MAX_PATH];
   GetModuleFileName(NULL, szFileName, _MAX_FNAME);

   string sAppName; // Extract from last '\' to '.'

   *_tcsrchr(szFileName, '.') = '\0';
   sAppName = (_tcsrchr(szFileName, '\\')+1);

   return sAppName;
}


string CUtility::getTempFileName()
{
   static int counter = 0;
   TCHAR szTempDir[MAX_PATH - 14]   = _T("");
   TCHAR szTempFile[MAX_PATH]       = _T("");

   if (GetTempPath(MAX_PATH - 14, szTempDir))
      GetTempFileName(szTempDir, getAppName().c_str(), ++counter, szTempFile);

   return szTempFile;
}


string CUtility::getSaveFileName()
{
   string sFilter = _T("Zip Files (*.zip)");

   OPENFILENAME ofn = {0};			// common dialog box structure
   TCHAR szFile[MAX_PATH] = {0};	// buffer for file name
   // Initialize OPENFILENAME
   ofn.lStructSize = sizeof(OPENFILENAME);
   ofn.hwndOwner = NULL;
   ofn.lpstrFile = szFile;
   ofn.nMaxFile = _countof(szFile);
   ofn.Flags = OFN_OVERWRITEPROMPT;

   ofn.lpstrFilter = sFilter.c_str();
   ofn.nFilterIndex = 1;
   // Display the Open dialog box. 
   bool bTargetSelected = !!GetSaveFileName(&ofn);

   DeleteFile(ofn.lpstrFile);  // Just in-case it already exist
   return ofn.lpstrFile;
}

};
