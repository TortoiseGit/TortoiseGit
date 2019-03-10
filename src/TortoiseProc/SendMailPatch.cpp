// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2013, 2015-2016, 2018-2019 - TortoiseGit
// Copyright (C) 2011-2013 Sven Strickroth <email@cs-ware.de>

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
#include "SendMailPatch.h"
#include "SerialPatch.h"

CSendMailPatch::CSendMailPatch(const CString& To, const CString& CC, const CString& subject, bool bAttachment, bool bCombine)
	: CSendMailCombineable(To, CC, subject, bAttachment, bCombine)
{
}

CSendMailPatch::~CSendMailPatch()
{
}

int CSendMailPatch::SendAsSingleMail(const CTGitPath& path, CGitProgressList* instance)
{
	ASSERT(instance);

	CString pathfile(path.GetWinPathString());
	CSerialPatch patch;
	if (patch.Parse(pathfile, !m_bAttachment))
	{
		instance->ReportError(L"Could not open/parse " + pathfile);
		return -2;
	}

	CString body;
	CStringArray attachments;
	if (m_bAttachment)
		attachments.Add(pathfile);
	else
		body = patch.m_strBody;

	return SendMail(path, instance, m_sSenderName, m_sSenderMail, m_sTo, m_sCC, patch.m_Subject, body, attachments);
}

int CSendMailPatch::SendAsCombinedMail(const CTGitPathList &list, CGitProgressList* instance)
{
	ASSERT(instance);

	CStringArray attachments;
	CString body;
	for (int i = 0; i < list.GetCount(); ++i)
	{
		CSerialPatch patch;
		if (patch.Parse(list[i].GetWinPathString(), !m_bAttachment))
		{
			instance->ReportError(L"Could not open/parse " + list[i].GetWinPathString());
			return -2;
		}
		if (m_bAttachment)
		{
			attachments.Add(list[i].GetWinPathString());
			body += patch.m_Subject;
			body += L"\r\n";
		}
		else
		{
			try
			{
				CGit::StringAppend(&body, patch.m_Body, CP_UTF8, patch.m_Body.GetLength());
			}
			catch (CMemoryException *)
			{
				instance->ReportError(L"Out of memory. Could not parse " + list[i].GetWinPathString());
				return -2;
			}
		}
	}
	return SendMail(CTGitPath(), instance, m_sSenderName, m_sSenderMail, m_sTo, m_sCC, m_sSubject, body, attachments);
}
