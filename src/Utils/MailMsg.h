/************************************************************************************* 
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
  All rights reserved.
 
  Redistribution and use in source and binary forms, with or without modification, 
  are permitted provided that the following conditions are met:
 
   * Redistributions of source code must retain the above copyright notice, this 
     list of conditions and the following disclaimer.
 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation 
     and/or other materials provided with the distribution.
 
   * Neither the name of the author nor the names of its contributors 
     may be used to endorse or promote products derived from this software without 
     specific prior written permission.
 

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
  OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT 
  SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, 
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
  TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR 
  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
  STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
  OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************************/

///////////////////////////////////////////////////////////////////////////////
//
//  Module: MailMsg.h
//
//    Desc: This class encapsulates the MAPI and CMC mail functions.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAILMSG_H_
#define _MAILMSG_H_

#include "stdafx.h"

typedef std::map<std::string, std::string> TStrStrMap;

//
// Define CMC entry points
//
typedef CMC_return_code (FAR PASCAL *LPCMCLOGON) \
   (CMC_string, CMC_string, CMC_string, CMC_object_identifier, \
   CMC_ui_id, CMC_uint16, CMC_flags, CMC_session_id FAR*, \
   CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCSEND) \
   (CMC_session_id, CMC_message FAR*, CMC_flags, \
   CMC_ui_id, CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCLOGOFF) \
   (CMC_session_id, CMC_ui_id, CMC_flags, CMC_extension FAR*);

typedef CMC_return_code (FAR PASCAL *LPCMCQUERY) \
   (CMC_session_id, CMC_enum, CMC_buffer, CMC_extension FAR*);


////////////////////////////// Class Definitions /////////////////////////////

// ===========================================================================
// CMailMsg
// 
// See the module comment at top of file.
//
class CMailMsg  
{
public:
	
  // Construction/destruction
  CMailMsg();
	virtual ~CMailMsg();
  
  // Operations
  void SetTo(CString sAddress);
  void SetFrom(CString sAddress);
  void SetSubject(CString sSubject);
  void SetMessage(CString sMessage);
  void AddAttachment(CString sAttachment, CString sTitle = _T(""));

  BOOL MAPIInitialize();
  void MAPIFinalize();
  
  static BOOL DetectMailClient(CString& sMailClientName);
  CString GetEmailClientName();
  BOOL Send();
  BOOL MAPISend();   
  BOOL CMCSend();
  CString GetLastErrorMsg(){ return m_sErrorMsg; }

protected:

   std::string    m_from;                       // From <address,name>
   std::string    m_to;                         // To <address,name>
   TStrStrMap     m_attachments;                // Attachment <file,title>
   std::string    m_sSubject;                   // EMail subject
   std::string    m_sMessage;                   // EMail message

   HMODULE        m_hMapi;                      // Handle to MAPI32.DLL
   LPCMCQUERY     m_lpCmcQueryConfiguration;    // Cmc func pointer
   LPCMCLOGON     m_lpCmcLogon;                 // Cmc func pointer
   LPCMCSEND      m_lpCmcSend;                  // Cmc func pointer
   LPCMCLOGOFF    m_lpCmcLogoff;                // Cmc func pointer
   LPMAPILOGON    m_lpMapiLogon;                // Mapi func pointer
   LPMAPISENDMAIL m_lpMapiSendMail;             // Mapi func pointer
   LPMAPILOGOFF   m_lpMapiLogoff;               // Mapi func pointer
   
   BOOL           m_bReady;                     // MAPI is loaded
   CString        m_sEmailClientName;

   CString        m_sErrorMsg;
};

#endif	// _MAILMSG_H_
