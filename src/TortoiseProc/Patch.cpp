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
	if(this->Parser(pathfile)	)
		return -1;

	CStringArray attachments;
	if(bAttachment)
	{
		attachments.Add(pathfile);
	}

	CString sender;
	sender.Format(_T("%s <%s> "),g_Git.GetUserName(),g_Git.GetUserEmail());

	if (useMAPI)
	{
		CMailMsg mapiSender;
		BOOL bMAPIInit = mapiSender.MAPIInitialize();
		if(!bMAPIInit)
		{
			m_LastError = mapiSender.GetLastErrorMsg();
			return -1;
		}

		mapiSender.SetShowComposeDialog(TRUE);
		mapiSender.SetFrom(g_Git.GetUserEmail());
		mapiSender.SetTo(TO);
		mapiSender.SetSubject(m_Subject);
		mapiSender.SetMessage(m_strBody);
		if(bAttachment)
			mapiSender.AddAttachment(pathfile);

		BOOL bSend = mapiSender.Send();
		if (bSend==TRUE)
			return 0;
		else
		{
			m_LastError = mapiSender.GetLastErrorMsg();
			return -1;
		}
	}
	else
	{
		CHwSMTP mail;	
		if(mail.SendSpeedEmail(this->m_Author,TO,this->m_Subject,this->m_strBody,NULL,&attachments,CC,25,sender))
			return 0;
		else
		{
			this->m_LastError=mail.GetLastErrorText();
			return -1;
		}
	}
#if 0
	CRegString server(REG_SMTP_SERVER);
	CRegDWORD  port(REG_SMTP_PORT,25);
	CRegDWORD  bAuth(REG_SMTP_ISAUTH);
	CRegString  user(REG_SMTP_USER);
	CRegString  password(REG_SMTP_PASSWORD);

	mail.SetSMTPServer(CUnicodeUtils::GetUTF8(server),port);

	AddRecipient(mail,TO,false);
	AddRecipient(mail,CC,true);

	if( bAttachment )
		mail.AddAttachment(CUnicodeUtils::GetUTF8(pathfile));

	CString name,address;
	GetNameAddress(this->m_Author,name,address);
	mail.SetSenderName(CUnicodeUtils::GetUTF8(name));
	mail.SetSenderMail(CUnicodeUtils::GetUTF8(address));

	mail.SetXPriority(XPRIORITY_NORMAL);
	mail.SetXMailer("The Bat! (v3.02) Professional");

	mail.SetSubject(CUnicodeUtils::GetUTF8(this->m_Subject));

	mail.SetMessageBody((char*)&this->m_Body[0]);

	if(bAuth)
	{
		mail.SetLogin(CUnicodeUtils::GetUTF8((CString&)user));
		mail.SetPassword(CUnicodeUtils::GetUTF8((CString&)password));
	}

	return !mail.Send();
#endif


}
int CPatch::Send(CTGitPathList &list,CString &To,CString &CC, CString &subject,bool bAttachment, bool useMAPI,CString *errortext)
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
			g_Git.StringAppend(&body,(BYTE*)patch.m_Body.GetBuffer(),CP_ACP,patch.m_Body.GetLength());
		}

	}

	CString sender;
	sender.Format(_T("%s <%s> "),g_Git.GetUserName(),g_Git.GetUserEmail());

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
	
	int i=0;
#if 0
	while(i<4)
	{   PatchFile.ReadString(str);
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
		g_Git.StringAppend(&m_Author,(BYTE*)one.GetBuffer()+6,CP_ACP,one.GetLength()-6);

	one=m_Body.Tokenize("\n",start);
	if(one.GetLength()>6)
		g_Git.StringAppend(&m_Date,(BYTE*)one.GetBuffer()+6,CP_ACP,one.GetLength()-6);

	one=m_Body.Tokenize("\n",start);
	if(one.GetLength()>8)
		g_Git.StringAppend(&m_Subject,(BYTE*)one.GetBuffer()+8,CP_ACP,one.GetLength()-8);

	//one=m_Body.Tokenize("\n",start);
	
	g_Git.StringAppend(&m_strBody,(BYTE*)m_Body.GetBuffer()+start+1,CP_ACP,m_Body.GetLength()-start-1);
	
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
#if 0
void CPatch::AddRecipient(CSmtp &mail, CString &tolist, bool isCC)
{
	int pos=0;
	while(pos>=0)
	{
		CString one=tolist.Tokenize(_T(";"),pos);
		int start=one.Find(_T('<'));
		int end = one.Find(_T('>'));
		CStringA name;
		CStringA address;
		if( start>=0 && end >=0)
		{
			name=CUnicodeUtils::GetUTF8(one.Left(start));
			address=CUnicodeUtils::GetUTF8(one.Mid(start+1,end-start-1));
			if(address.IsEmpty())
				continue;
			if(isCC)
				mail.AddCCRecipient(address,name);
			else
				mail.AddRecipient(address,name);

		}else
		{
			if(one.IsEmpty())
				continue;
			if(isCC)
				mail.AddCCRecipient(CUnicodeUtils::GetUTF8(one));
			else
				mail.AddRecipient(CUnicodeUtils::GetUTF8(one));
		}
	}
}
#endif
