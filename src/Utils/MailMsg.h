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
//    Desc: This class encapsulates the MAPI mail functions.
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _MAILMSG_H_
#define _MAILMSG_H_

#include "stdafx.h"
#include <MAPI.h>

typedef std::map<std::string, std::string> TStrStrMap;

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
	void AddCC(CString sAddress);
	void SetShowComposeDialog(BOOL showComposeDialog);

	BOOL MAPIInitialize();
	void MAPIFinalize();

	static BOOL DetectMailClient(CString& sMailClientName);
	CString GetEmailClientName();
	BOOL Send();
	CString GetLastErrorMsg(){ return m_sErrorMsg; }

protected:
	std::string					m_from;                       // From <address,name>
	std::string					m_to;                         // To <address,name>
	TStrStrMap					m_attachments;                // Attachment <file,title>
	std::vector<std::string>	m_cc;                         // CC receipients
	std::string					m_sSubject;                   // EMail subject
	std::string					m_sMessage;                   // EMail message

	HMODULE						m_hMapi;                      // Handle to MAPI32.DLL
	LPMAPISENDMAIL				m_lpMapiSendMail;             // Mapi func pointer

	BOOL						m_bReady;                     // MAPI is loaded
	BOOL						m_bShowComposeDialog;
	CString						m_sEmailClientName;

	CString						m_sErrorMsg;
};

#endif	// _MAILMSG_H_
