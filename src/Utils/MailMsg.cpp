/*************************************************************************************
  This file is a part of CrashRpt library.

  Copyright (c) 2003, Michael Carruth
  All rights reserved.

  Adjusted by Sven Strickroth <email@cs-ware.de>, 2011-2018
   * make it work with no attachments
   * added flag to show mail compose dialog
   * make it work with 32-64bit inconsistencies (http://msdn.microsoft.com/en-us/library/dd941355.aspx)
   * auto extract filenames of attachments
   * make work with multiple recipients (to|cc)
   * fix non ascii chars in subject, text or attachment paths
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
//  Module: MailMsg.cpp
//
//    Desc: See MailMsg.h
//
// Copyright (c) 2003 Michael Carruth
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MailMsg.h"
#include "UnicodeUtils.h"
#include "StringUtils.h"
#include <MAPI.h>
#include "MapiUnicodeHelp.h"

CMailMsg::CMailMsg()
{
	m_bShowComposeDialog	= FALSE;
}

void CMailMsg::SetFrom(const CString& sAddress, const CString& sName)
{
	m_from.email = L"SMTP:" + sAddress;
	m_from.name = sName;
}

static void addAdresses(std::vector<MailAddress>& recipients, const CString& sAddresses)
{
	int start = 0;
	while (start >= 0)
	{
		CString address = sAddresses.Tokenize(L";", start);
		CString name;
		CStringUtils::ParseEmailAddress(address, address, &name);
		if (address.IsEmpty())
			continue;
		recipients.emplace_back(L"SMTP:" + address, name);
	}
}

void CMailMsg::SetTo(const CString& sAddresses)
{
	addAdresses(m_to, sAddresses);
}

void CMailMsg::SetSubject(const CString& sSubject)
{
	m_sSubject = sSubject;
}

void CMailMsg::SetMessage(const CString& sMessage)
{
	m_sMessage = sMessage;
};

void CMailMsg::SetShowComposeDialog(BOOL showComposeDialog)
{
	m_bShowComposeDialog = showComposeDialog;
};

void CMailMsg::SetCC(const CString& sAddresses)
{
	addAdresses(m_cc, sAddresses);
}

void CMailMsg::AddAttachment(const CString& sAttachment, CString sTitle)
{
	if (sTitle.IsEmpty())
	{
		int position = sAttachment.ReverseFind(L'\\');
		if(position >=0)
		{
			sTitle = sAttachment.Mid(position+1);
		}
		else
		{
			sTitle = sAttachment;
		}
	}
	m_attachments[sAttachment] = sTitle;
}

BOOL CMailMsg::DetectMailClient(CString& sMailClientName)
{
	CRegKey regKey;
	TCHAR buf[1024] = L"";
	LONG lResult = lResult = regKey.Open(HKEY_CURRENT_USER, L"SOFTWARE\\Clients\\Mail", KEY_READ);
	if(lResult!=ERROR_SUCCESS)
	{
		lResult = regKey.Open(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Clients\\Mail", KEY_READ);
	}

	if(lResult==ERROR_SUCCESS)
	{
		ULONG buf_size = 1023;
#pragma warning(disable:4996)
		LONG result = regKey.QueryValue(buf, L"", &buf_size);
#pragma warning(default:4996)
		if(result==ERROR_SUCCESS)
		{
			sMailClientName = buf;
			return TRUE;
		}
		regKey.Close();
	}
	else
	{
		sMailClientName = "Not Detected";
	}

	return FALSE;
}

BOOL CMailMsg::MAPIInitialize()
{
	// Determine if there is default email program

	CString sMailClientName;
	if(!DetectMailClient(sMailClientName))
	{
		m_sErrorMsg = L"Error detecting E-mail client";
		return FALSE;
	}
	else
	{
		m_sErrorMsg = L"Detected E-mail client " + sMailClientName;
	}

	return TRUE;
}

BOOL CMailMsg::Send()
{
	// set from
	MapiRecipDescW originator = { 0 };
	originator.ulRecipClass = MAPI_ORIG;
	originator.lpszAddress = const_cast<LPWSTR>(static_cast<LPCWSTR>(m_from.email));
	originator.lpszName = const_cast<LPWSTR>(static_cast<LPCWSTR>(m_from.name));

	std::vector<MapiRecipDescW> recipients;
	auto addRecipient = [&recipients](ULONG ulRecipClass, const MailAddress& recipient)
	{
		MapiRecipDescW repipDesc = { 0 };
		repipDesc.ulRecipClass = ulRecipClass;
		repipDesc.lpszAddress = const_cast<LPWSTR>(static_cast<LPCWSTR>(recipient.email));
		repipDesc.lpszName = const_cast<LPWSTR>(static_cast<LPCWSTR>(recipient.name));
		recipients.emplace_back(repipDesc);
	};
	// add to recipients
	std::for_each(m_to.cbegin(), m_to.cend(), std::bind(addRecipient, MAPI_TO, std::placeholders::_1));
	// add cc receipients
	std::for_each(m_cc.cbegin(), m_cc.cend(), std::bind(addRecipient, MAPI_CC, std::placeholders::_1));

	// add attachments
	std::vector<MapiFileDescW> attachments;
	std::for_each(m_attachments.cbegin(), m_attachments.cend(), [&attachments](auto& attachment)
	{
		MapiFileDescW fileDesc = { 0 };
		fileDesc.nPosition = 0xFFFFFFFF;
		fileDesc.lpszPathName = const_cast<LPWSTR>(static_cast<LPCWSTR>(attachment.first));
		fileDesc.lpszFileName = const_cast<LPWSTR>(static_cast<LPCWSTR>(attachment.second));
		attachments.emplace_back(fileDesc);
	});

	MapiMessageW message = { 0 };
	message.lpszSubject						= const_cast<LPWSTR>(static_cast<LPCWSTR>(m_sSubject));
	message.lpszNoteText					= const_cast<LPWSTR>(static_cast<LPCWSTR>(m_sMessage));
	message.lpOriginator					= &originator;
	message.nRecipCount						= static_cast<ULONG>(recipients.size());
	message.lpRecips						= recipients.data();
	message.nFileCount						= static_cast<ULONG>(attachments.size());
	message.lpFiles							= attachments.data();

	ULONG status = MAPISendMailHelper(NULL, 0, &message, (m_bShowComposeDialog ? MAPI_DIALOG : 0) | MAPI_LOGON_UI, 0);

	if(status!=SUCCESS_SUCCESS)
	{
		m_sErrorMsg.Format(L"MAPISendMail has failed with code %lu.", status);
	}

	return (SUCCESS_SUCCESS == status);
}
