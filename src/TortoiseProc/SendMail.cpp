// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2016, 2018-2019 - TortoiseGit

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#include "stdafx.h"
#include "SendMail.h"
#include "HwSMTP.h"
#include "MailMsg.h"
#include "Git.h"
#include "WindowsCredentialsStore.h"

class CAppUtils;

CSendMail::CSendMail(const CString& To, const CString& CC, bool bAttachment)
	: m_sSenderName(g_Git.GetUserName())
	, m_sSenderMail(g_Git.GetUserEmail())
	, m_sTo(To)
	, m_sCC(CC)
	, m_bAttachment(bAttachment)
{
}

CSendMail::~CSendMail(void)
{
}

int CSendMail::SendMail(const CTGitPath &item, CGitProgressList * instance, const CString& FromName, const CString& FromMail, const CString& To, const CString& CC, const CString &subject, const CString& body, CStringArray &attachments)
{
	ASSERT(instance);
	int retry = 0;
	while (retry < 3)
	{
		if (instance->IsCancelled() == TRUE)
		{
			instance->ReportUserCanceled();
			return -1;
		}

		instance->AddNotify(new CGitProgressList::NotificationData(item, IDS_SVNACTION_SENDMAIL_START), CColors::Modified);

		CString error;
		if (SendMail(FromName, FromMail, To, CC, subject, body, attachments, &error) == 0)
			return 0;

		instance->ReportError(error);

		if (instance->IsCancelled() == FALSE) // do not retry/sleep if the user already canceled
		{
			++retry;
			if (retry < 3)
			{
				CString temp;
				temp.LoadString(IDS_SVNACTION_SENDMAIL_RETRY);
				instance->ReportNotification(temp);
				Sleep(2000);
			}
		}
	}
	return -1;
}

int CSendMail::SendMail(const CString& FromName, const CString& FromMail, const CString& To, const CString& CC, const CString& subject, const CString& body, CStringArray &attachments, CString *errortext)
{
	if (CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\DeliveryType", SEND_MAIL_MAPI) == SEND_MAIL_MAPI)
	{
		CMailMsg mapiSender;
		BOOL bMAPIInit = mapiSender.MAPIInitialize();
		if (!bMAPIInit)
		{
			if (errortext)
				*errortext = mapiSender.GetLastErrorMsg();
			return -1;
		}

		mapiSender.SetShowComposeDialog(TRUE);
		mapiSender.SetFrom(FromMail, FromName);
		mapiSender.SetTo(To);
		if (!CC.IsEmpty())
			mapiSender.SetCC(CC);
		mapiSender.SetSubject(subject);
		mapiSender.SetMessage(body);
		for (int i = 0; i < attachments.GetSize(); ++i)
			mapiSender.AddAttachment(attachments[i]);

		BOOL bSend = mapiSender.Send();
		if (bSend == TRUE)
			return 0;
		else
		{
			if (errortext)
				*errortext = mapiSender.GetLastErrorMsg();
			return -1;
		}
	}
	else
	{
		CString sender;
		sender.Format(L"%s <%s>", static_cast<LPCTSTR>(CHwSMTP::GetEncodedHeader(FromName)), static_cast<LPCTSTR>(FromMail));

		CHwSMTP mail;
		if (CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\DeliveryType", SEND_MAIL_SMTP_CONFIGURED) == SEND_MAIL_SMTP_CONFIGURED)
		{
			CString recipients(To);
			if (!CC.IsEmpty())
				recipients += L";" + CC;
			CCredentials credentials;
			CWindowsCredentialsStore::GetCredential(L"TortoiseGit:SMTP-Credentials", credentials);
			if (mail.SendEmail(static_cast<CString>(CRegString(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Address", L"")), &credentials, static_cast<BOOL>(CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\AuthenticationRequired", FALSE)), sender, recipients, subject, body, &attachments, CC, static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Port", 25)), sender, To, static_cast<DWORD>(CRegDWORD(L"Software\\TortoiseGit\\TortoiseProc\\SendMail\\Encryption", 0))) == TRUE)
				return 0;
			else
			{
				if (errortext)
					*errortext = mail.GetLastErrorText();
				return -1;
			}
		}
		else if (mail.SendSpeedEmail(sender, To, subject, body, &attachments, CC, sender))
			return 0;
		else
		{
			if (errortext)
				*errortext = mail.GetLastErrorText();
			return -1;
		}
	}
}

CSendMailCombineable::CSendMailCombineable(const CString& To, const CString& CC, const CString& subject, bool bAttachment, bool bCombine)
	: CSendMail(To, CC, bAttachment)
	, m_sSubject(subject)
	, m_bCombine(bCombine)
{
}

CSendMailCombineable::~CSendMailCombineable()
{
}

int CSendMailCombineable::Send(const CTGitPathList& list, CGitProgressList* instance)
{
	if (m_bCombine)
		return SendAsCombinedMail(list, instance);
	else
	{
		instance->SetItemCountTotal(list.GetCount() + 1);
		for (int i = 0; i < list.GetCount(); ++i)
		{
			instance->SetItemProgress(i);
			if (SendAsSingleMail(list[i], instance))
				return -1;
		}
		instance->SetItemProgress(list.GetCount() + 1);
	}

	return 0;
}

int GetFileContents(CString &filename, CString &content)
{
	CStdioFile file;
	if (file.Open(filename, CFile::modeRead))
	{
		CString str;
		while (file.ReadString(str))
		{
			content += str;
			str.Empty();
			content += L'\n';
		}
		return 0;
	}
	else
		return -1;
}

int CSendMailCombineable::SendAsSingleMail(const CTGitPath& path, CGitProgressList* instance)
{
	ASSERT(instance);

	CString pathfile(path.GetWinPathString());

	CString body;
	CStringArray attachments;
	if (m_bAttachment)
		attachments.Add(pathfile);
	else if (GetFileContents(pathfile, body))
	{
		instance->ReportError(L"Could not open " + pathfile);
		return -2;
	}

	return SendMail(path, instance, m_sSenderName, m_sSenderMail, m_sTo, m_sCC, m_sSubject, body, attachments);
}

int CSendMailCombineable::SendAsCombinedMail(const CTGitPathList &list, CGitProgressList* instance)
{
	ASSERT(instance);

	CStringArray attachments;
	CString body;
	for (int i = 0; i < list.GetCount(); ++i)
	{
		if (m_bAttachment)
			attachments.Add(list[i].GetWinPathString());
		else
		{
			CString filename(list[i].GetWinPathString());
			body += filename + L":\n";
			if (GetFileContents(filename, body))
			{
				instance->ReportError(L"Could not open " + filename);
				return -2;
			}
			body += L'\n';
		}
	}
	return SendMail(CTGitPath(), instance, m_sSenderName, m_sSenderMail, m_sTo, m_sCC, m_sSubject, body, attachments);
}
