//////////////////////////////////////////////////////////////////////
// Original class CFastSmtp written by 
// christopher w. backen <immortal@cox.net>
// More details at: http://www.codeproject.com/KB/IP/zsmtp.aspx
// 
// Modifications:
// 1. name of the class and some functions
// 2. new functions added: SendData,ReceiveData and more
// 3. authentication added
// 4. attachments added
// introduced by Jakub Piwowarczyk
// More details at: http://www.codeproject.com/KB/mcpp/CSmtp.aspx
//////////////////////////////////////////////////////////////////////

#include "CSmtp.h"

#pragma warning(push)
#pragma warning(disable:4786)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSmtp::CSmtp()
{
	// Initialize variables
	m_oError = CSMTP_NO_ERROR;
	m_iXPriority = XPRIORITY_NORMAL;
	m_iSMTPSrvPort = 0;

	m_pcLocalHostName = NULL;
	m_pcMailFrom = NULL;
	m_pcNameFrom = NULL;
	m_pcSubject = NULL;
	m_pcMsgBody = NULL;
	m_pcXMailer = NULL;
	m_pcReplyTo = NULL;
	m_pcLogin = NULL;
	m_pcPassword = NULL;
	m_pcSMTPSrvName = NULL;
	
	if((RecvBuf = new char[BUFFER_SIZE]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	
	if((SendBuf = new char[BUFFER_SIZE]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	
	// Initialize WinSock
	WORD wVer = MAKEWORD(2,2);    
	if (WSAStartup(wVer,&wsaData) != NO_ERROR)
	{
		m_oError = CSMTP_WSA_STARTUP;
		return;
	}
	if (LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) 
	{
		m_oError = CSMTP_WSA_VER;
		WSACleanup();
		return;
	}
}

CSmtp::~CSmtp()
{
	// Clear vectors
	Recipients.clear();
	CCRecipients.clear();
	BCCRecipients.clear();
	Attachments.clear();

	// Free memory
	if (m_pcLocalHostName)
		delete[] m_pcLocalHostName;
	if (m_pcMailFrom)
		delete[] m_pcMailFrom;
	if (m_pcNameFrom)
		delete[] m_pcNameFrom;
	if (m_pcSubject)
		delete[] m_pcSubject;
	if (m_pcMsgBody)
		delete[] m_pcMsgBody;
	if (m_pcXMailer)
		delete[] m_pcXMailer;
	if (m_pcReplyTo)
		delete[] m_pcReplyTo;
	if (m_pcLogin)
		delete[] m_pcLogin;
	if (m_pcPassword)
		delete[] m_pcPassword;
	if(SendBuf)
		delete[] SendBuf;
	if(RecvBuf)
		delete[] RecvBuf;
	
	// Cleanup
	WSACleanup();
}

//////////////////////////////////////////////////////////////////////
// Methods
//////////////////////////////////////////////////////////////////////

bool CSmtp::AddAttachment(const char *path)
{
	std::string str(path);
	Attachments.insert(Attachments.end(),str);
	return true;
}

bool CSmtp::AddRecipient(const char *email, const char *name)
{
	assert(email);
	
	if(!email)
	{
		m_oError = CSMTP_UNDEF_RECIPENT_MAIL;
		return false;
	}

	Recipent recipent;
	recipent.Mail.insert(0,email);
	name!=NULL ? recipent.Name.insert(0,name) : recipent.Name.insert(0,"");

	Recipients.insert(Recipients.end(), recipent);

	return true;    
}

bool CSmtp::AddCCRecipient(const char *email, const char *name)
{
	assert(email);
	
	if(!email)
	{
		m_oError = CSMTP_UNDEF_RECIPENT_MAIL;
		return false;
	}

	Recipent recipent;
	recipent.Mail.insert(0,email);
	name!=NULL ? recipent.Name.insert(0,name) : recipent.Name.insert(0,"");

	CCRecipients.insert(CCRecipients.end(), recipent);

	return true;
}

bool CSmtp::AddBCCRecipient(const char *email, const char *name)
{
	assert(email);
	
	if(!email)
	{
		m_oError = CSMTP_UNDEF_RECIPENT_MAIL;
		return false;
	}

	Recipent recipent;
	recipent.Mail.insert(0,email);
	name!=NULL ? recipent.Name.insert(0,name) : recipent.Name.insert(0,"");

	BCCRecipients.insert(BCCRecipients.end(), recipent);

	return true;
}

bool CSmtp::Send()
{
	unsigned int i,rcpt_count,res,FileId;
	char *FileBuf = NULL, *FileName = NULL;
	FILE* hFile = NULL;
	unsigned long int FileSize,TotalSize,MsgPart;

	// ***** CONNECTING TO SMTP SERVER *****

	assert(m_pcSMTPSrvName);

	// connecting to remote host:
	if( (hSocket = ConnectRemoteServer(m_pcSMTPSrvName, m_iSMTPSrvPort)) == INVALID_SOCKET ) 
	{
		m_oError = CSMTP_WSA_INVALID_SOCKET;
		return false;
	}
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 220:
			break;
		default:
			m_oError = CSMTP_SERVER_NOT_READY;
			return false;
	}

	// EHLO <SP> <domain> <CRLF>
	sprintf(SendBuf,"EHLO %s\r\n",GetLocalHostName()!=NULL ? m_pcLocalHostName : "domain");
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 250:
			break;
		default:
			m_oError = CSMTP_COMMAND_EHLO;
			return false;
	}

	// AUTH <SP> LOGIN <CRLF>
	strcpy(SendBuf,"AUTH LOGIN\r\n");
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 334:
			break;
		default:
			m_oError = CSMTP_COMMAND_AUTH_LOGIN;
			return false;
	}

	// send login:
	if(!m_pcLogin)
	{
		m_oError = CSMTP_UNDEF_LOGIN;
		return false;
	}
	std::string encoded_login = base64_encode(reinterpret_cast<const unsigned char*>(m_pcLogin),strlen(m_pcLogin));
	sprintf(SendBuf,"%s\r\n",encoded_login.c_str());
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 334:
			break;
		default:
			m_oError = CSMTP_UNDEF_XYZ_RESPOMSE;
			return false;
	}
	
	// send password:
	if(!m_pcPassword)
	{
		m_oError = CSMTP_UNDEF_PASSWORD;
		return false;
	}
	std::string encoded_password = base64_encode(reinterpret_cast<const unsigned char*>(m_pcPassword),strlen(m_pcPassword));
	sprintf(SendBuf,"%s\r\n",encoded_password.c_str());
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 235:
			break;
		case 535:
			m_oError = CSMTP_BAD_LOGIN_PASS;
			return false;
		default:
			m_oError = CSMTP_UNDEF_XYZ_RESPOMSE;
			return false;
	}

	// ***** SENDING E-MAIL *****
	
	// MAIL <SP> FROM:<reverse-path> <CRLF>
	if(m_pcMailFrom == NULL)
	{
		m_oError = CSMTP_UNDEF_MAILFROM;
		return false;
	}
	sprintf(SendBuf,"MAIL FROM:<%s>\r\n",m_pcMailFrom);
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 250:
			break;
		default:
			m_oError = CSMTP_COMMAND_MAIL_FROM;
			return false;
	}

	// RCPT <SP> TO:<forward-path> <CRLF>
	rcpt_count = Recipients.size();
	for(i=0;i<Recipients.size();i++)
	{
		sprintf(SendBuf,"RCPT TO:<%s>\r\n",(Recipients.at(i).Mail).c_str());
		if(!SendData())
			return false;
		Sleep(DELAY_IN_MS);
		if(!ReceiveData())
			return false;

		switch(SmtpXYZdigits())
		{
			case 250:
				break;
			default:
				m_oError = CSMTP_COMMAND_RCPT_TO;
				rcpt_count--;
		}
	}
	if(!rcpt_count)
		return false;
	for(i=0;i<CCRecipients.size();i++)
	{
		sprintf(SendBuf,"RCPT TO:<%s>\r\n",(CCRecipients.at(i).Mail).c_str());
		if(!SendData())
			return false;
		Sleep(DELAY_IN_MS);
		if(!ReceiveData())
			return false;
	}
	for(i=0;i<BCCRecipients.size();i++)
	{
		sprintf(SendBuf,"RCPT TO:<%s>\r\n",(BCCRecipients.at(i).Mail).c_str());
		if(!SendData())
			return false;
		Sleep(DELAY_IN_MS);
		if(!ReceiveData())
			return false;
	}
	
	// DATA <CRLF>
	strcpy(SendBuf,"DATA\r\n");
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;
	
	switch(SmtpXYZdigits())
	{
		case 354:
			break;
		default:
			m_oError = CSMTP_COMMAND_DATA;
			return false;
	}
	
	// send header(s)
	if(!FormatHeader(SendBuf))
	{
		m_oError = CSMTP_UNDEF_MSG_HEADER;
		return false;
	}
	if(!SendData())
		return false;

	// send text message
	sprintf(SendBuf,"%s\r\n",m_pcMsgBody); // NOTICE: each line ends with <CRLF>
	if(!SendData())
		return false;

	// next goes attachments (if they are)
	if((FileBuf = new char[55]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return false;
	}
	if((FileName = new char[255]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return false;
	}
	TotalSize = 0;
	for(FileId=0;FileId<Attachments.size();FileId++)
	{
		strcpy(FileName,Attachments[FileId].c_str());

		sprintf(SendBuf,"--%s\r\n",BOUNDARY_TEXT);
		strcat(SendBuf,"Content-Type: application/x-msdownload; name=\"");
		strcat(SendBuf,&FileName[Attachments[FileId].find_last_of("\\") + 1]);
		strcat(SendBuf,"\"\r\n");
		strcat(SendBuf,"Content-Transfer-Encoding: base64\r\n");
		strcat(SendBuf,"Content-Disposition: attachment; filename=\"");
		strcat(SendBuf,&FileName[Attachments[FileId].find_last_of("\\") + 1]);
		strcat(SendBuf,"\"\r\n");
		strcat(SendBuf,"\r\n");

		if(!SendData())
			return false;

		// opening the file:
		hFile = fopen(FileName,"rb");
		if(hFile == NULL)
		{
			m_oError = CSMTP_FILE_NOT_EXIST;
			break;
		}
		
		// checking file size:
		FileSize = 0;
		while(!feof(hFile))
			FileSize += fread(FileBuf,sizeof(char),54,hFile);
		TotalSize += FileSize;

		// sending the file:
		if(TotalSize/1024 > MSG_SIZE_IN_MB*1024)
			m_oError = CSMTP_MSG_TOO_BIG;
		else
		{
			fseek (hFile,0,SEEK_SET);

			MsgPart = 0;
			for(i=0;i<FileSize/54+1;i++)
			{
				res = fread(FileBuf,sizeof(char),54,hFile);
				MsgPart ? strcat(SendBuf,base64_encode(reinterpret_cast<const unsigned char*>(FileBuf),res).c_str())
					      : strcpy(SendBuf,base64_encode(reinterpret_cast<const unsigned char*>(FileBuf),res).c_str());
				strcat(SendBuf,"\r\n");
				MsgPart += res + 2;
				if(MsgPart >= BUFFER_SIZE/2)
				{ // sending part of the message
					MsgPart = 0;
					if(!SendData())
					{
						delete[] FileBuf;
						delete[] FileName;
						fclose(hFile);
						return false;
					}
				}
			}
			if(MsgPart)
			{
				if(!SendData())
				{
					delete[] FileBuf;
					delete[] FileName;
					fclose(hFile);
					return false;
				}
			}
		}
		fclose(hFile);
	}
	delete[] FileBuf;
	delete[] FileName;
	
	// sending last message block (if there is one or more attachments)
	if(Attachments.size())
	{
		sprintf(SendBuf,"\r\n--%s--\r\n",BOUNDARY_TEXT);
		if(!SendData())
			return false;
	}
	
	// <CRLF> . <CRLF>
	strcpy(SendBuf,"\r\n.\r\n");
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 250:
			break;
		default:
			m_oError = CSMTP_MSG_BODY_ERROR;
			return false;
	}

	// ***** CLOSING CONNECTION *****
	
	// QUIT <CRLF>
	strcpy(SendBuf,"QUIT\r\n");
	if(!SendData())
		return false;
	Sleep(DELAY_IN_MS);
	if(!ReceiveData())
		return false;

	switch(SmtpXYZdigits())
	{
		case 221:
			break;
		default:
			m_oError = CSMTP_COMMAND_QUIT;
			hSocket = NULL;
			return false;
	}

	closesocket(hSocket);
	hSocket = NULL;
	return true;
}

SOCKET CSmtp::ConnectRemoteServer(const char *server,const unsigned short port)
{
	short nProtocolPort;
	LPHOSTENT lpHostEnt;
	LPSERVENT lpServEnt;
	SOCKADDR_IN sockAddr;
	SOCKET hServerSocket = INVALID_SOCKET;
	struct in_addr addr;
	
	// If the user input is an alpha name for the host, use gethostbyname()
	// If not, get host by addr (assume IPv4)
	if(isalpha(server[0]))
		lpHostEnt = gethostbyname(server);
	else
	{
		addr.s_addr = inet_addr(server);
    if(addr.s_addr == INADDR_NONE) 
		{
			m_oError = CSMTP_BAD_IPV4_ADDR;
			return INVALID_SOCKET;
		} 
		else
			lpHostEnt = gethostbyaddr((char *) &addr, 4, AF_INET);
	}

	if(lpHostEnt != NULL)
	{
		if((hServerSocket = socket(PF_INET, SOCK_STREAM,0)) != INVALID_SOCKET)
		{
			if(port != NULL)
				nProtocolPort = htons(port);
			else
			{
				lpServEnt = getservbyname("mail", 0);
				if (lpServEnt == NULL)
					nProtocolPort = htons(25);
				else 
					nProtocolPort = lpServEnt->s_port;
			}
			
			sockAddr.sin_family = AF_INET;
			sockAddr.sin_port = nProtocolPort;
			sockAddr.sin_addr = *((LPIN_ADDR)*lpHostEnt->h_addr_list);
			if(connect(hServerSocket,(PSOCKADDR)&sockAddr,sizeof(sockAddr)) == SOCKET_ERROR)
			{
				m_oError = CSMTP_WSA_CONNECT;
				hServerSocket = INVALID_SOCKET;
			}
		}
		else
		{
			m_oError = CSMTP_WSA_INVALID_SOCKET;
			return INVALID_SOCKET;
		}
	}
	else
	{
		m_oError = CSMTP_WSA_GETHOSTBY_NAME_ADDR;
		return INVALID_SOCKET;
	}

	return hServerSocket;
}

int CSmtp::SmtpXYZdigits()
{
	assert(RecvBuf);
	if(RecvBuf == NULL)
		return 0;
	return (RecvBuf[0]-'0')*100 + (RecvBuf[1]-'0')*10 + RecvBuf[2]-'0';
}

bool CSmtp::FormatHeader(char* header)
{
	int i,s = 0;
	TCHAR szDate[500];
	TCHAR sztTime[500];
	char *to = NULL;
	char *cc = NULL;
	char *bcc = NULL;

	// check for at least one recipient
	if(Recipients.size())
	{
		for (unsigned int i=s=0;i<Recipients.size();i++)
			s += Recipients[i].Mail.size() + Recipients[i].Name.size() + 3;
		if (s == 0) 
			s = 1;
		if((to = new char[s]) == NULL)
		{
			m_oError = CSMTP_LACK_OF_MEMORY;
			return false;
		}

		to[0] = '\0';
		for (i=0;i<Recipients.size();i++)
		{
			i > 0 ? strcat(to,","):strcpy(to,"");
			strcat(to,Recipients[i].Name.c_str());
			strcat(to,"<");
			strcat(to,Recipients[i].Mail.c_str());
			strcat(to,">");
		}
	}
	else
	{
		m_oError = CSMTP_UNDEF_RECIPENTS;
		return false;
	}

	if(CCRecipients.size())
	{
		for (i=s=0;i<CCRecipients.size();i++)
			s += CCRecipients[i].Mail.size() + CCRecipients[i].Name.size() + 3;
		if (s == 0)
			s = 1;
		if((cc = new char[s]) == NULL)
		{
			m_oError = CSMTP_LACK_OF_MEMORY;
			delete[] to;
			return false;
		}

		cc[0] = '\0';
		for (i=0;i<CCRecipients.size();i++)
		{
			i > 0 ? strcat(cc,","):strcpy(cc,"");
			strcat(cc,CCRecipients[i].Name.c_str());
			strcat(cc,"<");
			strcat(cc,CCRecipients[i].Mail.c_str());
			strcat(cc,">");
		}
	}

	if(BCCRecipients.size())
	{
		for (i=s=0;i<BCCRecipients.size();i++)
			s += BCCRecipients[i].Mail.size() + BCCRecipients[i].Name.size() + 3;
		if(s == 0)
			s=1;
		if((bcc = new char[s]) == NULL)
		{
			m_oError = CSMTP_LACK_OF_MEMORY;
			delete[] to;
			delete[] cc;
			return false;
		}

		bcc[0] = '\0';
		for (i=0;i<BCCRecipients.size();i++)
		{
			i > 0 ? strcat(bcc,","):strcpy(bcc,"");
			strcat(bcc,BCCRecipients[i].Name.c_str());
			strcat(bcc,"<");
			strcat(bcc,BCCRecipients[i].Mail.c_str());
			strcat(bcc,">");
		}
	}
	
	// Date: <SP> <dd> <SP> <mon> <SP> <yy> <SP> <hh> ":" <mm> ":" <ss> <SP> <zone> <CRLF>
	SYSTEMTIME st={0};
	::GetSystemTime(&st);
	::GetDateFormat(MAKELCID(0x0409,SORT_DEFAULT),0,&st,"ddd\',\' dd MMM yyyy",szDate,sizeof(szDate));
	::GetTimeFormat(MAKELCID(0x0409,SORT_DEFAULT),TIME_FORCE24HOURFORMAT,&st,"HH\':\'mm\':\'ss",sztTime,sizeof(sztTime));
	sprintf(header,"Date: %s %s\r\n", szDate, sztTime); 
	
	// From: <SP> <sender>  <SP> "<" <sender-email> ">" <CRLF>
	if(m_pcMailFrom == NULL)
	{
		m_oError = CSMTP_UNDEF_MAILFROM;
    delete[] to;
    delete[] cc;
    delete[] bcc;
		return false;
	}
	strcat(header,"From: ");	
	if(m_pcNameFrom)
		strcat(header, m_pcNameFrom);
	strcat(header," <");
	strcat(header,m_pcMailFrom);
	strcat(header, ">\r\n");

	// X-Mailer: <SP> <xmailer-app> <CRLF>
	if (m_pcXMailer != NULL)
	{
		strcat(header,"X-Mailer: ");
		strcat(header, m_pcXMailer);
		strcat(header, "\r\n");
	}

	// Reply-To: <SP> <reverse-path> <CRLF>
	if(m_pcReplyTo != NULL)
	{
		strcat(header, "Reply-To: ");
		strcat(header, m_pcReplyTo);
		strcat(header, "\r\n");
	}

	// X-Priority: <SP> <number> <CRLF>
	switch(m_iXPriority)
	{
		case XPRIORITY_HIGH:
			strcat(header,"X-Priority: 2 (High)\r\n");
			break;
		case XPRIORITY_NORMAL:
			strcat(header,"X-Priority: 3 (Normal)\r\n");
			break;
		case XPRIORITY_LOW:
			strcat(header,"X-Priority: 4 (Low)\r\n");
			break;
		default:
			strcat(header,"X-Priority: 3 (Normal)\r\n");
	}

	// To: <SP> <remote-user-mail> <CRLF>
	strcat(header,"To: ");
	strcat(header, to);
	strcat(header, "\r\n");

	// Cc: <SP> <remote-user-mail> <CRLF>
	if(CCRecipients.size())
	{
		strcat(header,"Cc: ");
		strcat(header, cc);
		strcat(header, "\r\n");
	}

	if(BCCRecipients.size())
	{
		strcat(header,"Bcc: ");
		strcat(header, bcc);
		strcat(header, "\r\n");
	}

	// Subject: <SP> <subject-text> <CRLF>
	if(m_pcSubject == NULL) 
	{
		m_oError = CSMTP_UNDEF_SUBJECT;
		strcat(header, "Subject:  ");
	}
	else
	{
	  strcat(header, "Subject: ");
	  strcat(header, m_pcSubject);
	}
	strcat(header, "\r\n");
	
	// MIME-Version: <SP> 1.0 <CRLF>
	strcat(header,"MIME-Version: 1.0\r\n");
	if(!Attachments.size())
	{ // no attachments
		strcat(header,"Content-type: text/plain; charset=US-ASCII\r\n");
		strcat(header,"Content-Transfer-Encoding: 7bit\r\n");
		strcat(SendBuf,"\r\n");
	}
	else
	{ // there is one or more attachments
		strcat(header,"Content-Type: multipart/mixed; boundary=\"");
		strcat(header,BOUNDARY_TEXT);
		strcat(header,"\"\r\n");
		strcat(header,"\r\n");
		// first goes text message
		strcat(SendBuf,"--");
		strcat(SendBuf,BOUNDARY_TEXT);
		strcat(SendBuf,"\r\n");
		strcat(SendBuf,"Content-type: text/plain; charset=US-ASCII\r\n");
		strcat(SendBuf,"Content-Transfer-Encoding: 7bit\r\n");
		strcat(SendBuf,"\r\n");
	}
	
	// clean up
	delete[] to;
	delete[] cc;
	delete[] bcc;
	
	// done    
	return true;    
}

bool CSmtp::ReceiveData()
{
	assert(RecvBuf);

	int res;

	if(RecvBuf == NULL)
		return false;
	
	if( (res = recv(hSocket,RecvBuf,BUFFER_SIZE,0)) == SOCKET_ERROR )
	{
		m_oError = CSMTP_WSA_RECV;
		return false;
	}
	if(res == 0)
	{
		m_oError = CSMTP_CONNECTION_CLOSED;
		return false;
	}
	RecvBuf[res] = '\0';

	return true;
}

bool CSmtp::SendData()
{
	assert(SendBuf);

	int idx = 0,res,nLeft = strlen(SendBuf);
	while(nLeft > 0)
	{
		if( res = send(hSocket,&SendBuf[idx],nLeft,0) == SOCKET_ERROR)
		{
			m_oError = CSMTP_WSA_SEND;
			return false;
		}
		if(!res)
			break;
		nLeft -= res;
		idx += res;
	}
	return true;
}

CSmtpError CSmtp::GetLastError()
{
	return m_oError;
}

/*
const char* const CSmtp::GetLocalHostIP()
{
	in_addr *iaHost = NULL;
	HOSTENT *pHe = NULL;
	
	if (m_pcIPAddr)
		delete[] m_pcIPAddr;
	
	if(gethostname(m_pcHostName,255) != SOCKET_ERROR)
	{
		pHe = gethostbyname(m_pcHostName);
		if (pHe != NULL) 
		{
			for (int i=0;pHe->h_addr_list[i] != 0;i++)
			{
				iaHost = (LPIN_ADDR)pHe->h_addr_list[i];
				m_pcIPAddr = inet_ntoa(*iaHost);
			}
		}            
	} 
	else 
	{
		m_oError = CSMTP_WSA_GETHOSTBY_NAME_ADDR;
		m_pcIPAddr = NULL;
	}
	
	return m_pcIPAddr;
}
*/

const char* const CSmtp::GetLocalHostName() 
{
	if(m_pcLocalHostName)
		delete[] m_pcLocalHostName;
	if((m_pcLocalHostName = new char[255]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return NULL;
	}
	if(gethostname((char FAR*)m_pcLocalHostName,255) == SOCKET_ERROR)
		m_oError = CSMTP_WSA_HOSTNAME;
	return m_pcLocalHostName;
}

unsigned const int CSmtp::GetBCCRecipientCount()
{
	return BCCRecipients.size();
}

unsigned const int CSmtp::GetCCRecipientCount() 
{
	return CCRecipients.size();
}

const char* const CSmtp::GetMessageBody() 
{
	return m_pcMsgBody;
}

unsigned const int CSmtp::GetRecipientCount()
{
	return Recipients.size();
}

const char* const CSmtp::GetReplyTo()  
{
	return m_pcReplyTo;
}

const char* const CSmtp::GetMailFrom() 
{
	return m_pcMailFrom;
}

const char* const CSmtp::GetSenderName() 
{
	return m_pcNameFrom;
}

const char* const CSmtp::GetSubject() 
{
	return m_pcSubject;
}

const char* const CSmtp::GetXMailer() 
{
	return m_pcXMailer;
}

CSmptXPriority CSmtp::GetXPriority()
{
	return m_iXPriority;
}

void CSmtp::SetXPriority(CSmptXPriority priority)
{
	m_iXPriority = priority;
}

void CSmtp::SetMessageBody(const char *body)
{
	assert(body);
	int s = strlen(body);
	if (m_pcMsgBody)
		delete[] m_pcMsgBody;
	if((m_pcMsgBody = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcMsgBody, body);    
}

void CSmtp::SetReplyTo(const char *replyto)
{
	assert(replyto);
	int s = strlen(replyto);
	if (m_pcReplyTo)
		delete[] m_pcReplyTo;
	if((m_pcReplyTo = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcReplyTo, replyto);
}

void CSmtp::SetSenderMail(const char *email)
{
	assert(email);
	int s = strlen(email);
	if (m_pcMailFrom)
		delete[] m_pcMailFrom;
	if((m_pcMailFrom = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcMailFrom, email);        
}

void CSmtp::SetSenderName(const char *name)
{
	assert(name);
	int s = strlen(name);
	if (m_pcNameFrom)
		delete[] m_pcNameFrom;
	if((m_pcNameFrom = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcNameFrom, name);
}

void CSmtp::SetSubject(const char *subject)
{
	assert(subject);
	int s = strlen(subject);
	if (m_pcSubject)
		delete[] m_pcSubject;
	m_pcSubject = new char[s+1];
	strcpy(m_pcSubject, subject);
}

void CSmtp::SetXMailer(const char *xmailer)
{
	assert(xmailer);
	int s = strlen(xmailer);
	if (m_pcXMailer)
		delete[] m_pcXMailer;
	if((m_pcXMailer = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcXMailer, xmailer);
}

void CSmtp::SetLogin(const char *login)
{
	assert(login);
	int s = strlen(login);
	if (m_pcLogin)
		delete[] m_pcLogin;
	if((m_pcLogin = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcLogin, login);
}

void CSmtp::SetPassword(const char *password)
{
	assert(password);
	int s = strlen(password);
	if (m_pcPassword)
		delete[] m_pcPassword;
	if((m_pcPassword = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcPassword, password);
}

void CSmtp::SetSMTPServer(const char* SrvName,const unsigned short SrvPort)
{
	assert(SrvName);
	m_iSMTPSrvPort = SrvPort;
	int s = strlen(SrvName);
	if(m_pcSMTPSrvName)
		delete[] m_pcSMTPSrvName;
	if((m_pcSMTPSrvName = new char[s+1]) == NULL)
	{
		m_oError = CSMTP_LACK_OF_MEMORY;
		return;
	}
	strcpy(m_pcSMTPSrvName, SrvName);
}

//////////////////////////////////////////////////////////////////////
// Friends
//////////////////////////////////////////////////////////////////////

char* GetErrorText(CSmtpError ErrorId)
{
	switch(ErrorId)
	{
		case CSMTP_NO_ERROR:
			return "";
		case CSMTP_WSA_STARTUP:
			return "Unable to initialise winsock2.";
		case CSMTP_WSA_VER:
			return "Wrong version of the winsock2.";
		case CSMTP_WSA_SEND:
			return "Function send() failed.";
		case CSMTP_WSA_RECV:
			return "Function recv() failed.";
		case CSMTP_WSA_CONNECT:
			return "Function connect failed.";
		case CSMTP_WSA_GETHOSTBY_NAME_ADDR:
			return "Functions gethostbyname() or gethostbyaddr() failed.";
		case CSMTP_WSA_INVALID_SOCKET:
			return "Invalid winsock2 socket.";
		case CSMTP_WSA_HOSTNAME:
			return "Function hostname() failed.";
		case CSMTP_BAD_IPV4_ADDR:
			return "Improper IPv4 address.";
		case CSMTP_UNDEF_MSG_HEADER:
			return "Undefined message header.";
		case CSMTP_UNDEF_MAILFROM:
			return "Undefined from is the mail.";
		case CSMTP_UNDEF_SUBJECT:
			return "Undefined message subject.";
		case CSMTP_UNDEF_RECIPENTS:
			return "Undefined at least one reciepent.";
		case CSMTP_UNDEF_RECIPENT_MAIL:
			return "Undefined recipent mail.";
		case CSMTP_UNDEF_LOGIN:
			return "Undefined user login.";
		case CSMTP_UNDEF_PASSWORD:
			return "Undefined user password.";
		case CSMTP_COMMAND_MAIL_FROM:
			return "Server returned error after sending MAIL FROM.";
		case CSMTP_COMMAND_EHLO:
			return "Server returned error after sending EHLO.";
		case CSMTP_COMMAND_AUTH_LOGIN:
			return "Server returned error after sending AUTH LOGIN.";
		case CSMTP_COMMAND_DATA:
			return "Server returned error after sending DATA.";
		case CSMTP_COMMAND_QUIT:
			return "Server returned error after sending QUIT.";
		case CSMTP_COMMAND_RCPT_TO:
			return "Server returned error after sending RCPT TO.";
		case CSMTP_MSG_BODY_ERROR:
			return "Error in message body";
		case CSMTP_CONNECTION_CLOSED:
			return "Server has closed the connection.";
		case CSMTP_SERVER_NOT_READY:
			return "Server is not ready.";
		case CSMTP_FILE_NOT_EXIST:
			return "File not exist.";
		case CSMTP_MSG_TOO_BIG:
			return "Message is too big.";
		case CSMTP_BAD_LOGIN_PASS:
			return "Bad login or password.";
		case CSMTP_UNDEF_XYZ_RESPOMSE:
			return "Undefined xyz SMTP response.";
		case CSMTP_LACK_OF_MEMORY:
			return "Lack of memory.";
		default:
			return "Undefined error id.";
	}
}

#pragma warning(pop)
