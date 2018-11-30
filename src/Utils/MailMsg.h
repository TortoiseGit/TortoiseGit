/*************************************************************************************
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
  All rights reserved.

  Adjusted by Sven Strickroth <email@cs-ware.de>, 2011-2018
   * added flag to show mail compose dialog
   * make it work with 32-64bit inconsistencies (http://msdn.microsoft.com/en-us/library/dd941355.aspx)
   * auto extract filenames of attachments
   * make work with multiple recipients (to|cc)
   * fix non ascii chars in subject, text and attachment paths
   * See Git history of the TortoiseGit project for more details

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

typedef std::map<CString, CString> TStrStrMap;

typedef struct MailAddress
{
	CString email;
	CString name;

	MailAddress() {};

	MailAddress(const CString& email, const CString& name)
	: email(email)
	, name(name)
	{}
} MailAddress;

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

	// Operations
	void SetTo(const CString& sAddress);
	void SetFrom(const CString& sAddresses, const CString& sName);
	void SetSubject(const CString& sSubject);
	void SetMessage(const CString& sMessage);
	void AddAttachment(const CString& sAttachment, CString sTitle = L"");
	void SetCC(const CString& sAddresses);
	void SetShowComposeDialog(BOOL showComposeDialog);

	BOOL MAPIInitialize();

	static BOOL DetectMailClient(CString& sMailClientName);
	BOOL Send();
	CString GetLastErrorMsg(){ return m_sErrorMsg; }

protected:
	MailAddress					m_from;
	std::vector<MailAddress>	m_to;                         // To receipients
	TStrStrMap					m_attachments;                // Attachment <file,title>
	std::vector<MailAddress>	m_cc;                         // CC receipients
	CString						m_sSubject;                   // EMail subject
	CString						m_sMessage;                   // EMail message

	BOOL						m_bShowComposeDialog;

	CString						m_sErrorMsg;
};

#endif	// _MAILMSG_H_
