// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011 - TortoiseGit
// Copyright (C) 2011 Sven Strickroth <email@cs-ware.de>

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

#include "StdAfx.h"
#include "Patch.h"
#include "csmtp.h"
#include "registry.h"
#include "unicodeutils.h"
#include "hwsmtp.h"
#include "mailmsg.h"
#include "Windns.h"
#include "Git.h"

CPatch::CPatch()
{
}

CPatch::~CPatch()
{
}

void CPatch::ConvertToArray(CString &to,CStringArray &Array)
{
	int start=0;
	while(start>=0)
	{
		CString str=to.Tokenize(_T(";"),start);
		if(!str.IsEmpty())
			Array.Add(str);
	}
}

int CPatch::Send(CString &pathfile,CString &TO,CString &CC,bool bAttachment, bool useMAPI)
{
	if(this->Parser(pathfile))
		return -1;

	CString body;
	CStringArray attachments;
	if(bAttachment)
	{
		attachments.Add(pathfile);
	}
	else
	{
		body = this->m_strBody;
	}

	CString errortext = _T("");
	int ret = SendMail(TO, CC, this->m_Subject, body, attachments, useMAPI, &errortext);
	this->m_LastError=errortext;
	return ret;
}

int CPatch::SendPatchesCombined(CTGitPathList &list,CString &To,CString &CC, CString &subject,bool bAttachment, bool useMAPI,CString *errortext)
{
	CStringArray attachments;
	CString body;
	for(int i=0;i<list.GetCount();i++)
	{
		CPatch patch;
		patch.Parser((CString&)list[i].GetWinPathString());
		if(bAttachment)
		{
			attachments.Add(list[i].GetWinPathString());
			body+=patch.m_Subject;
			body+=_T("\r\n");
		}
		else
		{
			g_Git.StringAppend(&body, (BYTE*)patch.m_Body.GetBuffer(), CP_UTF8, patch.m_Body.GetLength());
		}

	}
	return SendMail(To, CC, subject, body, attachments, useMAPI, errortext);
}

int CPatch::SendMail(CString &To, CString &CC, CString &subject, CString &body, CStringArray &attachments, bool useMAPI, CString *errortext)
{
	CString sender;
	sender.Format(_T("%s <%s>"), g_Git.GetUserName(), g_Git.GetUserEmail());

	if (useMAPI)
	{
		CMailMsg mapiSender;
		BOOL bMAPIInit = mapiSender.MAPIInitialize();
		if(!bMAPIInit)
		{
			if(errortext)
				*errortext = mapiSender.GetLastErrorMsg();
			return -1;
		}

		mapiSender.SetShowComposeDialog(TRUE);
		mapiSender.SetFrom(g_Git.GetUserEmail());
		mapiSender.SetTo(To);
		if(!CC.IsEmpty())
			mapiSender.AddCC(CC);
		mapiSender.SetSubject(subject);
		mapiSender.SetMessage(body);
		for(int i=0; i < attachments.GetSize(); i++)
		{
			mapiSender.AddAttachment(attachments[i]);
		}

		BOOL bSend = mapiSender.Send();
		if (bSend==TRUE)
			return 0;
		else
		{
			if(errortext)
				*errortext = mapiSender.GetLastErrorMsg();
			return -1;
		}
	}
	else
	{
		CHwSMTP mail;
		if(mail.SendSpeedEmail(sender,To,subject,body,NULL,&attachments,CC,25,sender))
			return 0;
		else
		{
			if(errortext)
				*errortext=mail.GetLastErrorText();
			return -1;
		}
	}
}

int CPatch::Parser(CString &pathfile)
{
	CString str;

	CFile PatchFile;

	m_PathFile=pathfile;
	if( ! PatchFile.Open(pathfile,CFile::modeRead) )
		return -1;

#if 0
	int i=0;
	while(i<4)
	{
		PatchFile.ReadString(str);
		if(i==1)
			this->m_Author=str.Right( str.GetLength() - 6 );
		if(i==2)
			this->m_Date = str.Right( str.GetLength() - 6 );
		if(i==3)
			this->m_Subject = str.Right( str.GetLength() - 8 );

		i++;
	}

	LONGLONG offset=PatchFile.GetPosition();
#endif
	PatchFile.Read(m_Body.GetBuffer(PatchFile.GetLength()),PatchFile.GetLength());
	m_Body.ReleaseBuffer();
	PatchFile.Close();

	int start=0;
	CStringA one;
	one=m_Body.Tokenize("\n",start);

	one=m_Body.Tokenize("\n",start);
	if(one.GetLength()>6)
		g_Git.StringAppend(&m_Author, (BYTE*)one.GetBuffer() + 6, CP_UTF8, one.GetLength() - 6);

	one=m_Body.Tokenize("\n",start);
	if(one.GetLength()>6)
		g_Git.StringAppend(&m_Date, (BYTE*)one.GetBuffer() + 6, CP_UTF8, one.GetLength() - 6);

	one=m_Body.Tokenize("\n",start);
	if(one.GetLength()>9)
		g_Git.StringAppend(&m_Subject, (BYTE*)one.GetBuffer() + 9, CP_UTF8, one.GetLength() - 9);

	//one=m_Body.Tokenize("\n",start);

	g_Git.StringAppend(&m_strBody, (BYTE*)m_Body.GetBuffer() + start + 1, CP_UTF8, m_Body.GetLength() - start - 1);

	return 0;
}

void CPatch::GetNameAddress(CString &in, CString &name,CString &address)
{
	int start,end;
	start=in.Find(_T('<'));
	end=in.Find(_T('>'));

	if(start >=0 && end >=0)
	{
		name=in.Left(start);
		address=in.Mid(start+1,end-start-1);
	}
	else
		address=in;
}

