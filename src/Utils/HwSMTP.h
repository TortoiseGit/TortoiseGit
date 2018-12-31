// HwSMTP.h: interface for the CHwSMTP class.
//
//////////////////////////////////////////////////////////////////////

/*******************************************************************
		说明
	1、这是我自己写的一个利用SMTP服务器发送邮件的类，可以实现附件的
发送。
	2、在 InitInstance() 函数调用
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}
	在 ExitInstance () 中调用 ::WSACleanup ();
*******************************************************************/

/****************************************************************************************************************
投稿说明：
****************************************************************************************************************/
#define NOTE_SpeedPostMail \
L"\
			【软件名称】\r\n\
	无须SMTP服务器中转直接将E-Mail电子邮件发送到对方邮箱\r\n\
			【版    本】\r\n\
	1.0.0\r\n\
			【操作系统】\r\n\
	Windows 桌面系列\r\n\
			【作    者】\r\n\
	谢红伟 · chrys · chrys@163.com · http://www.viction.net\r\n\
			【软件说明】\r\n\
	大家一定熟悉Foxmail中的“特快专递”，它能直接将邮件发送到对方的邮件服务器中，而不需要经过SMTP服务器中转，\
本代码将向你剖析“特快专递”发送电子邮件的方法。代码中提供了网卡信息获取类，可以获取本机IP地址、子网掩码、DNS、\
Wins、网卡MAC地址等相关信息；还提供了SMTP协议解析类，该类实现了SMTP客户端功能的实现类，实现电子邮件收发；Base64编码实现\
了字符编码的方法。\r\n\
	你可以任意修改复制本代码，但请保留这段文字不要修改。\r\n\
	希望我能为中国的软件行业尽一份薄力！\r\n\
			【开发日期】\r\n\
	2008-11-23 3:54\r\n"

#include <afxsock.h>

#define SECURITY_WIN32
#include <wincrypt.h>
#include <wintrust.h>
#include <schannel.h>
#include <security.h>
#include <sspi.h>
#include "WindowsCredentialsStore.h"

#if !defined(AFX_HwSMTP_H__633A52B7_1CBE_41D7_BDA3_188D98D692AF__INCLUDED_)
#define AFX_HwSMTP_H__633A52B7_1CBE_41D7_BDA3_188D98D692AF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

enum SECURITY_LEVEL {
	none,
	want_tls,
	ssl,
	tls_established,
};

class CHwSMTP
{
public:
	CString GetLastErrorText();
	BOOL SendEmail (
		LPCTSTR lpszSmtpSrvHost,
		CCredentials* credentials,
		BOOL bMustAuth,
		LPCTSTR lpszAddrFrom,
		LPCTSTR lpszAddrTo,
		LPCTSTR lpszSubject,
		LPCTSTR lpszBody,
		CStringArray* pStrAryAttach = nullptr,
		LPCTSTR pStrAryCC = nullptr,
		UINT nSmtpSrvPort=25,
		LPCTSTR pSend = nullptr,
		LPCTSTR pToList = nullptr,
		DWORD secLevel = SECURITY_LEVEL::none
		);
	BOOL SendSpeedEmail
		(
			LPCTSTR lpszAddrFrom,
			LPCTSTR lpszAddrTo,
			LPCTSTR lpszSubject,
			LPCTSTR lpszBody,
			CStringArray* pStrAryAttach = nullptr,
			LPCTSTR pStrAryCC = nullptr,
			LPCTSTR pSend = nullptr
		);
	CHwSMTP();
	virtual ~CHwSMTP();
	static CString CHwSMTP::GetEncodedHeader(const CString& text);

protected:
	CString GetServerAddress(const CString& in);

private:
	BOOL SendSubject(const CString &hostname);
	CString m_StrCC;
	BOOL SendHead();
	BOOL auth();
	BOOL SendEmail();
	BOOL Send(const CStringA& data);
	BOOL Send ( const CString &data );
	BOOL SendBuffer(const char* buffer, int size = -1);
	BOOL GetResponse(LPCSTR lpszVerifyCode);
	BOOL m_bConnected;
	CSocket m_SendSock;
	CStringArray m_StrAryAttach;
	CString m_csSmtpSrvHost;
	CCredentials* m_credentials;
	CString m_csAddrFrom;
	CString m_csAddrTo;
	CString m_csFromName;
	CString m_csReceiverName;
	CString m_csSubject;
	CString m_csBody;
	CString m_csSender;
	CString m_csToList;

	CtxtHandle * hContext;
	CredHandle * hCreds;
	SecPkgContext_StreamSizes Sizes;
	PBYTE pbIoBuffer;
	DWORD cbIoBufferLength;
	SECURITY_LEVEL m_iSecurityLevel;

	BOOL m_bMustAuth;
	UINT m_nSmtpSrvPort;

	CString m_csLastError;
	BOOL SendOnAttach(LPCTSTR lpszFileName);
	BOOL SendAttach();
	BOOL SendBody();
	CString m_csMIMEContentType;
	CString m_csPartBoundary;
	CString m_csNoMIMEText;
};
#endif // !defined(AFX_HwSMTP_H__633A52B7_1CBE_41D7_BDA3_188D98D692AF__INCLUDED_)
