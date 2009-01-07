///////////////////////////////////////////////////////////////////////////////
//
//  Module: Utility.h
//
//    Desc: Misc static helper methods
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include "stdafx.h"


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CUtility
// 
// See the module comment at top of file.
//
namespace CUtility 
{

	BSTR AllocSysString(string s);

   //-----------------------------------------------------------------------------
   // getLastWriteFileTime
   //    Returns the time the file was last modified in a FILETIME structure.
   //
   // Parameters
   //    sFile       Fully qualified file name
   //
   // Return Values
   //    FILETIME structure
   //
   // Remarks
   //
   FILETIME 
   getLastWriteFileTime(
      string sFile
      );
   FILETIME
	   getLastWriteFileTime(
	   WCHAR * wszFile
	   );
   //-----------------------------------------------------------------------------
   // getAppName
   //    Returns the application module's file name
   //
   // Parameters
   //    none
   //
   // Return Values
   //    File name of the executable
   //
   // Remarks
   //    none
   //
   string 
   getAppName();

   //-----------------------------------------------------------------------------
   // getSaveFileName
   //    Presents the user with a save as dialog and returns the name selected.
   //
   // Parameters
   //    none
   //
   // Return Values
   //    Name of the file to save to, or "" if the user cancels.
   //
   // Remarks
   //    none
   //
   string 
   getSaveFileName();
	
   //-----------------------------------------------------------------------------
   // getTempFileName
   //    Returns a generated temporary file name
   //
   // Parameters
   //    none
   //
   // Return Values
   //    Temporary file name
   //
   // Remarks
   //
   string 
   getTempFileName();
};

