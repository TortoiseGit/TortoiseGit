// HwSMTP.cpp: implementation of the CHwSMTP class.
//
// Schannel/SSPI implementation based on http://www.coastrd.com/c-schannel-smtp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "afxstr.h"
#include "HwSMTP.h"
#include "Windns.h"
#include <Afxmt.h>
#include "FormatMessageWrapper.h"
#include <atlenc.h>
#include "AppUtils.h"
#include "PathUtils.h"
#include "StringUtils.h"

#define IO_BUFFER_SIZE 0x10000

#pragma comment(lib, "Dnsapi.lib")
#pragma comment(lib, "Rpcrt4.lib")
#pragma comment(lib, "Secur32.lib")

#ifndef GET_SAFE_STRING
#define GET_SAFE_STRING(str) ((str) ? (str) : L"")
#endif
#define HANDLE_IS_VALID(h) (reinterpret_cast<HANDLE>(h) != NULL && reinterpret_cast<HANDLE>(h) != INVALID_HANDLE_VALUE)

DWORD dwProtocol = SP_PROT_TLS1; // SP_PROT_TLS1; // SP_PROT_PCT1; SP_PROT_SSL2; SP_PROT_SSL3; 0=default
ALG_ID aiKeyExch = 0; // = default; CALG_DH_EPHEM; CALG_RSA_KEYX;

SCHANNEL_CRED SchannelCred;
PSecurityFunctionTable g_pSSPI;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

static CString FormatDateTime(COleDateTime& DateTime)
{
	// If null, return empty string
	if (DateTime.GetStatus() == COleDateTime::null || DateTime.GetStatus() == COleDateTime::invalid)
		return L"";

	UDATE ud;
	if (S_OK != VarUdateFromDate(DateTime.m_dt, 0, &ud))
		return L"";

	static TCHAR* weeks[] = { L"Sun", L"Mon", L"Tue", L"Wen", L"Thu", L"Fri", L"Sat" };
	static TCHAR* month[] = { L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun", L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec" };

	TIME_ZONE_INFORMATION stTimeZone;
	GetTimeZoneInformation(&stTimeZone);

	CString strDate;
	strDate.Format(L"%s, %d %s %d %02d:%02d:%02d %c%04d", weeks[ud.st.wDayOfWeek],
				   ud.st.wDay, month[ud.st.wMonth - 1], ud.st.wYear, ud.st.wHour,
				   ud.st.wMinute, ud.st.wSecond,
				   stTimeZone.Bias > 0 ? L'-' : L'+',
				   abs(stTimeZone.Bias * 10 / 6));
	return strDate;
}

static int GetFileSize(LPCTSTR lpFileName)
{
	if (!lpFileName || lstrlen(lpFileName) < 1)
		return -1;

	CFileStatus fileStatus = { 0 };
	memset(fileStatus.m_szFullName, 0, sizeof(fileStatus.m_szFullName));
	try
	{
		CFile::GetStatus(lpFileName, fileStatus);
	}
	catch (CException&)
	{
		ASSERT(FALSE);
	}

	return static_cast<int>(fileStatus.m_size);
}

static CString FormatBytes(double fBytesNum, BOOL bShowUnit = TRUE , int nFlag = 0)
{
	CString csRes;
	if (nFlag == 0)
	{
		if (fBytesNum >= 1024.0 && fBytesNum < 1024.0 * 1024.0)
			csRes.Format(L"%.2f%s", fBytesNum / 1024.0, bShowUnit ? L" K" : L"");
		else if (fBytesNum >= 1024.0 * 1024.0 && fBytesNum < 1024.0 * 1024.0 * 1024.0)
			csRes.Format(L"%.2f%s", fBytesNum / (1024.0 * 1024.0), bShowUnit ? L" M" : L"");
		else if (fBytesNum >= 1024.0 * 1024.0 * 1024.0)
			csRes.Format(L"%.2f%s", fBytesNum / (1024.0 * 1024.0 * 1024.0), bShowUnit ? L" G" : L"");
		else
			csRes.Format(L"%.2f%s", fBytesNum, bShowUnit ? L" B" : L"");
	}
	else if (nFlag == 1)
		csRes.Format(L"%.2f%s", fBytesNum / 1024.0, bShowUnit ? L" K" : L"");
	else if (nFlag == 2)
		csRes.Format(L"%.2f%s", fBytesNum / (1024.0 * 1024.0), bShowUnit ? L" M" : L"");
	else if (nFlag == 3)
		csRes.Format(L"%.2f%s", fBytesNum / (1024.0 * 1024.0 * 1024.0), bShowUnit ? L" G" : L"");

	return csRes;
}

static CString GetGUID()
{
	CString sGuid;
	GUID guid;
	if (CoCreateGuid(&guid) == S_OK)
	{
		RPC_WSTR guidStr;
		if (UuidToString(&guid, &guidStr) == RPC_S_OK)
		{
			sGuid = reinterpret_cast<LPTSTR>(guidStr);
			RpcStringFree(&guidStr);
		}
	}
	return sGuid;
}

CHwSMTP::CHwSMTP () :
	m_bConnected ( FALSE ),
	m_nSmtpSrvPort ( 25 ),
	m_bMustAuth ( TRUE )
	, m_credentials(nullptr)
{
	m_csPartBoundary = L"NextPart_" + GetGUID();
	m_csMIMEContentType.Format(L"multipart/mixed; boundary=%s", static_cast<LPCTSTR>(m_csPartBoundary));
	m_csNoMIMEText = L"This is a multi-part message in MIME format.";

	hContext = nullptr;
	hCreds = nullptr;
	pbIoBuffer = nullptr;
	cbIoBufferLength = 0;

	m_iSecurityLevel = none;

	SecureZeroMemory(&Sizes, sizeof(SecPkgContext_StreamSizes));

	AfxSocketInit();
}

CHwSMTP::~CHwSMTP()
{
}

CString CHwSMTP::GetServerAddress(const CString& in)
{
	CString email;
	CStringUtils::ParseEmailAddress(in, email);

	int start = email.Find(L'@');
	return email.Mid(start + 1);
}

BOOL CHwSMTP::SendSpeedEmail
		(
			LPCTSTR	lpszAddrFrom,
			LPCTSTR	lpszAddrTo,
			LPCTSTR	lpszSubject,
			LPCTSTR	lpszBody,
			CStringArray *pStrAryAttach,
			LPCTSTR	pStrAryCC,
			LPCTSTR	pSend
		)
{
	BOOL ret=true;
	CString To;
	To += GET_SAFE_STRING(lpszAddrTo);
	To += L';';
	To += GET_SAFE_STRING(pStrAryCC);

	std::map<CString,std::vector<CString>> Address;

	int start = 0;
	while( start >= 0 )
	{
		CString one = To.Tokenize(L";", start).Trim();
		if(one.IsEmpty())
			continue;

		CString addr = GetServerAddress(one);
		if(addr.IsEmpty())
			continue;

		Address[addr].push_back(one);
	}

	for (const auto& maildomain : Address)
	{
		PDNS_RECORD pDnsRecord;
		DNS_STATUS status =
		DnsQuery(maildomain.first,
						DNS_TYPE_MX,DNS_QUERY_STANDARD,
						nullptr,		//Contains DNS server IP address.
						&pDnsRecord,	//Resource record that contains the response.
						nullptr
						);
		if (status)
		{
			m_csLastError.Format(L"DNS query failed %d", status);
			ret = false;
			continue;
		}
		SCOPE_EXIT { DnsRecordListFree(pDnsRecord, DnsFreeRecordList); };

		CString to;
		std::for_each(maildomain.second.cbegin(), maildomain.second.cend(), [&to](auto recipient) {
			to += recipient;
			to += L';';
		});
		if(to.IsEmpty())
			continue;

		PDNS_RECORD pNext = pDnsRecord;
		while(pNext)
		{
			if(pNext->wType == DNS_TYPE_MX)
				if (SendEmail(pNext->Data.MX.pNameExchange, nullptr, false,
					lpszAddrFrom, to, lpszSubject, lpszBody, pStrAryAttach, pStrAryCC,
					25,pSend,lpszAddrTo))
					break;
			pNext=pNext->pNext;
		}
		if (!pNext)
			ret = false;
	}

	return ret;
}

static SECURITY_STATUS ClientHandshakeLoop(CSocket * Socket, PCredHandle phCreds, CtxtHandle * phContext, BOOL fDoInitialRead, SecBuffer * pExtraData)
{
	SecBufferDesc OutBuffer, InBuffer;
	SecBuffer InBuffers[2], OutBuffers[1];
	DWORD dwSSPIFlags, dwSSPIOutFlags, cbData, cbIoBuffer;
	TimeStamp tsExpiry;
	SECURITY_STATUS scRet;
	BOOL fDoRead;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT	| ISC_REQ_REPLAY_DETECT		| ISC_REQ_CONFIDENTIALITY |
				  ISC_RET_EXTENDED_ERROR	| ISC_REQ_ALLOCATE_MEMORY	| ISC_REQ_STREAM;

	// Allocate data buffer.
	auto IoBuffer = std::make_unique<UCHAR[]>(IO_BUFFER_SIZE);
	if (!IoBuffer)
	{
		// printf("**** Out of memory (1)\n");
		return SEC_E_INTERNAL_ERROR;
	}
	cbIoBuffer = 0;
	fDoRead = fDoInitialRead;

	// Loop until the handshake is finished or an error occurs.
	scRet = SEC_I_CONTINUE_NEEDED;

	while (scRet == SEC_I_CONTINUE_NEEDED || scRet == SEC_E_INCOMPLETE_MESSAGE || scRet == SEC_I_INCOMPLETE_CREDENTIALS)
	{
		if (0 == cbIoBuffer || scRet == SEC_E_INCOMPLETE_MESSAGE) // Read data from server.
		{
			if (fDoRead)
			{
				cbData = Socket->Receive(IoBuffer.get() + cbIoBuffer, IO_BUFFER_SIZE - cbIoBuffer, 0);
				if (cbData == SOCKET_ERROR)
				{
					// printf("**** Error %d reading data from server\n", WSAGetLastError());
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				else if (cbData == 0)
				{
					// printf("**** Server unexpectedly disconnected\n");
					scRet = SEC_E_INTERNAL_ERROR;
					break;
				}
				// printf("%d bytes of handshake data received\n", cbData);
				cbIoBuffer += cbData;
			}
			else
				fDoRead = TRUE;
		}

		// Set up the input buffers. Buffer 0 is used to pass in data
		// received from the server. Schannel will consume some or all
		// of this. Leftover data (if any) will be placed in buffer 1 and
		// given a buffer type of SECBUFFER_EXTRA.
		InBuffers[0].pvBuffer	= IoBuffer.get();
		InBuffers[0].cbBuffer	= cbIoBuffer;
		InBuffers[0].BufferType	= SECBUFFER_TOKEN;

		InBuffers[1].pvBuffer	= nullptr;
		InBuffers[1].cbBuffer	= 0;
		InBuffers[1].BufferType	= SECBUFFER_EMPTY;

		InBuffer.cBuffers		= 2;
		InBuffer.pBuffers		= InBuffers;
		InBuffer.ulVersion		= SECBUFFER_VERSION;

		// Set up the output buffers. These are initialized to nullptr
		// so as to make it less likely we'll attempt to free random
		// garbage later.
		OutBuffers[0].pvBuffer	= nullptr;
		OutBuffers[0].BufferType= SECBUFFER_TOKEN;
		OutBuffers[0].cbBuffer	= 0;

		OutBuffer.cBuffers		= 1;
		OutBuffer.pBuffers		= OutBuffers;
		OutBuffer.ulVersion		= SECBUFFER_VERSION;

		// Call InitializeSecurityContext.
		scRet = g_pSSPI->InitializeSecurityContext(phCreds, phContext, nullptr, dwSSPIFlags, 0, SECURITY_NATIVE_DREP, &InBuffer, 0, nullptr, &OutBuffer, &dwSSPIOutFlags, &tsExpiry);

		// If InitializeSecurityContext was successful (or if the error was
		// one of the special extended ones), send the contends of the output
		// buffer to the server.
		if (scRet == SEC_E_OK || scRet == SEC_I_CONTINUE_NEEDED || FAILED(scRet) && (dwSSPIOutFlags & ISC_RET_EXTENDED_ERROR))
		{
			if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != nullptr)
			{
				cbData = Socket->Send(OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0 );
				if(cbData == SOCKET_ERROR || cbData == 0)
				{
					// printf( "**** Error %d sending data to server (2)\n",  WSAGetLastError() );
					g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
					g_pSSPI->DeleteSecurityContext(phContext);
					return SEC_E_INTERNAL_ERROR;
				}
				// printf("%d bytes of handshake data sent\n", cbData);

				// Free output buffer.
				g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
				OutBuffers[0].pvBuffer = nullptr;
			}
		}

		// If InitializeSecurityContext returned SEC_E_INCOMPLETE_MESSAGE,
		// then we need to read more data from the server and try again.
		if (scRet == SEC_E_INCOMPLETE_MESSAGE) continue;

		// If InitializeSecurityContext returned SEC_E_OK, then the
		// handshake completed successfully.
		if (scRet == SEC_E_OK)
		{
			// If the "extra" buffer contains data, this is encrypted application
			// protocol layer stuff. It needs to be saved. The application layer
			// will later decrypt it with DecryptMessage.
			// printf("Handshake was successful\n");

			if (InBuffers[1].BufferType == SECBUFFER_EXTRA)
			{
				pExtraData->pvBuffer = LocalAlloc( LMEM_FIXED, InBuffers[1].cbBuffer );
				if (pExtraData->pvBuffer == nullptr)
				{
					// printf("**** Out of memory (2)\n");
					return SEC_E_INTERNAL_ERROR;
				}

				MoveMemory(pExtraData->pvBuffer, IoBuffer.get() + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);

				pExtraData->cbBuffer	= InBuffers[1].cbBuffer;
				pExtraData->BufferType	= SECBUFFER_TOKEN;

				// printf( "%d bytes of app data was bundled with handshake data\n", pExtraData->cbBuffer );
			}
			else
			{
				pExtraData->pvBuffer	= nullptr;
				pExtraData->cbBuffer	= 0;
				pExtraData->BufferType	= SECBUFFER_EMPTY;
			}
			break; // Bail out to quit
		}

		// Check for fatal error.
		if (FAILED(scRet))
		{
			// printf("**** Error 0x%x returned by InitializeSecurityContext (2)\n", scRet);
			break;
		}

		// If InitializeSecurityContext returned SEC_I_INCOMPLETE_CREDENTIALS,
		// then the server just requested client authentication.
		if (scRet == SEC_I_INCOMPLETE_CREDENTIALS)
		{
			// Busted. The server has requested client authentication and
			// the credential we supplied didn't contain a client certificate.
			// This function will read the list of trusted certificate
			// authorities ("issuers") that was received from the server
			// and attempt to find a suitable client certificate that
			// was issued by one of these. If this function is successful,
			// then we will connect using the new certificate. Otherwise,
			// we will attempt to connect anonymously (using our current credentials).
			//GetNewClientCredentials(phCreds, phContext);

			// Go around again.
			fDoRead = FALSE;
			scRet = SEC_I_CONTINUE_NEEDED;
			continue;
		}

		// Copy any leftover data from the "extra" buffer, and go around again.
		if ( InBuffers[1].BufferType == SECBUFFER_EXTRA )
		{
			MoveMemory(IoBuffer.get(), IoBuffer.get() + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
			cbIoBuffer = InBuffers[1].cbBuffer;
		}
		else
			cbIoBuffer = 0;
	}

	// Delete the security context in the case of a fatal error.
	if (FAILED(scRet))
		g_pSSPI->DeleteSecurityContext(phContext);

	return scRet;
}

static SECURITY_STATUS PerformClientHandshake( CSocket * Socket, PCredHandle phCreds, LPTSTR pszServerName, CtxtHandle * phContext, SecBuffer * pExtraData)
{
	SecBufferDesc OutBuffer;
	SecBuffer OutBuffers[1];
	DWORD dwSSPIFlags, dwSSPIOutFlags, cbData;
	TimeStamp tsExpiry;
	SECURITY_STATUS scRet;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT	| ISC_REQ_REPLAY_DETECT		| ISC_REQ_CONFIDENTIALITY |
				  ISC_RET_EXTENDED_ERROR	| ISC_REQ_ALLOCATE_MEMORY	| ISC_REQ_STREAM;

	//  Initiate a ClientHello message and generate a token.
	OutBuffers[0].pvBuffer = nullptr;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	scRet = g_pSSPI->InitializeSecurityContext(phCreds, nullptr, pszServerName, dwSSPIFlags, 0, SECURITY_NATIVE_DREP, nullptr, 0, phContext, &OutBuffer, &dwSSPIOutFlags, &tsExpiry);

	if (scRet != SEC_I_CONTINUE_NEEDED)
	{
		// printf("**** Error %d returned by InitializeSecurityContext (1)\n", scRet);
		return scRet;
	}

	// Send response to server if there is one.
	if (OutBuffers[0].cbBuffer != 0 && OutBuffers[0].pvBuffer != nullptr)
	{
		cbData = Socket->Send(OutBuffers[0].pvBuffer, OutBuffers[0].cbBuffer, 0);
		if (cbData == SOCKET_ERROR || cbData == 0)
		{
			// printf("**** Error %d sending data to server (1)\n", WSAGetLastError());
			g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer);
			g_pSSPI->DeleteSecurityContext(phContext);
			return SEC_E_INTERNAL_ERROR;
		}
		// printf("%d bytes of handshake data sent\n", cbData);

		g_pSSPI->FreeContextBuffer(OutBuffers[0].pvBuffer); // Free output buffer.
		OutBuffers[0].pvBuffer = nullptr;
	}

	return ClientHandshakeLoop(Socket, phCreds, phContext, TRUE, pExtraData);
}

static SECURITY_STATUS CreateCredentials(PCredHandle phCreds)
{
	TimeStamp tsExpiry;
	SECURITY_STATUS Status;
	DWORD cSupportedAlgs = 0;
	ALG_ID rgbSupportedAlgs[16];

	// Build Schannel credential structure. Currently, this sample only
	// specifies the protocol to be used (and optionally the certificate,
	// of course). Real applications may wish to specify other parameters as well.
	SecureZeroMemory(&SchannelCred, sizeof(SchannelCred));

	SchannelCred.dwVersion = SCHANNEL_CRED_VERSION;
	SchannelCred.grbitEnabledProtocols = dwProtocol;

	if (aiKeyExch)
		rgbSupportedAlgs[cSupportedAlgs++] = aiKeyExch;

	if (cSupportedAlgs)
	{
		SchannelCred.cSupportedAlgs = cSupportedAlgs;
		SchannelCred.palgSupportedAlgs = rgbSupportedAlgs;
	}

	SchannelCred.dwFlags |= SCH_CRED_NO_DEFAULT_CREDS;

	// The SCH_CRED_MANUAL_CRED_VALIDATION flag is specified because
	// this sample verifies the server certificate manually.
	// Applications that expect to run on WinNT, Win9x, or WinME
	// should specify this flag and also manually verify the server
	// certificate. Applications running on newer versions of Windows can
	// leave off this flag, in which case the InitializeSecurityContext
	// function will validate the server certificate automatically.
	SchannelCred.dwFlags |= SCH_CRED_MANUAL_CRED_VALIDATION;

	// Create an SSPI credential.
	Status = g_pSSPI->AcquireCredentialsHandle(nullptr,                // Name of principal
												 UNISP_NAME,           // Name of package
												 SECPKG_CRED_OUTBOUND, // Flags indicating use
												 nullptr,              // Pointer to logon ID
												 &SchannelCred,        // Package specific data
												 nullptr,              // Pointer to GetKey() func
												 nullptr,              // Value to pass to GetKey()
												 phCreds,              // (out) Cred Handle
												 &tsExpiry );          // (out) Lifetime (optional)

	return Status;
}

static DWORD EncryptSend(CSocket * Socket, CtxtHandle * phContext, PBYTE pbIoBuffer, SecPkgContext_StreamSizes Sizes)
// http://msdn.microsoft.com/en-us/library/aa375378(VS.85).aspx
// The encrypted message is encrypted in place, overwriting the original contents of its buffer.
{
	SECURITY_STATUS scRet;
	SecBufferDesc Message;
	SecBuffer Buffers[4];
	DWORD cbMessage;
	PBYTE pbMessage;

	pbMessage = pbIoBuffer + Sizes.cbHeader; // Offset by "header size"
	cbMessage = static_cast<DWORD>(strlen(reinterpret_cast<char*>(pbMessage)));

	// Encrypt the HTTP request.
	Buffers[0].pvBuffer     = pbIoBuffer;                 // Pointer to buffer 1
	Buffers[0].cbBuffer     = Sizes.cbHeader;             // length of header
	Buffers[0].BufferType   = SECBUFFER_STREAM_HEADER;    // Type of the buffer

	Buffers[1].pvBuffer     = pbMessage;                  // Pointer to buffer 2
	Buffers[1].cbBuffer     = cbMessage;                  // length of the message
	Buffers[1].BufferType   = SECBUFFER_DATA;             // Type of the buffer

	Buffers[2].pvBuffer     = pbMessage + cbMessage;      // Pointer to buffer 3
	Buffers[2].cbBuffer     = Sizes.cbTrailer;            // length of the trailor
	Buffers[2].BufferType   = SECBUFFER_STREAM_TRAILER;   // Type of the buffer

	Buffers[3].pvBuffer     = SECBUFFER_EMPTY;            // Pointer to buffer 4
	Buffers[3].cbBuffer     = SECBUFFER_EMPTY;            // length of buffer 4
	Buffers[3].BufferType   = SECBUFFER_EMPTY;            // Type of the buffer 4

	Message.ulVersion       = SECBUFFER_VERSION;          // Version number
	Message.cBuffers        = 4;                          // Number of buffers - must contain four SecBuffer structures.
	Message.pBuffers        = Buffers;                    // Pointer to array of buffers

	scRet = g_pSSPI->EncryptMessage(phContext, 0, &Message, 0); // must contain four SecBuffer structures.
	if (FAILED(scRet))
	{
		// printf("**** Error 0x%x returned by EncryptMessage\n", scRet);
		return scRet;
	}


	// Send the encrypted data to the server.
	return Socket->Send(pbIoBuffer, Buffers[0].cbBuffer + Buffers[1].cbBuffer + Buffers[2].cbBuffer, 0);
}

static LONG DisconnectFromServer(CSocket * Socket, PCredHandle phCreds, CtxtHandle * phContext)
{
	PBYTE pbMessage;
	DWORD dwType, dwSSPIFlags, dwSSPIOutFlags, cbMessage, cbData, Status;
	SecBufferDesc OutBuffer;
	SecBuffer OutBuffers[1];
	TimeStamp tsExpiry;

	dwType = SCHANNEL_SHUTDOWN; // Notify schannel that we are about to close the connection.

	OutBuffers[0].pvBuffer = &dwType;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = sizeof(dwType);

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	Status = g_pSSPI->ApplyControlToken(phContext, &OutBuffer);
	if (FAILED(Status))
	{
		// printf("**** Error 0x%x returned by ApplyControlToken\n", Status);
		goto cleanup;
	}

	// Build an SSL close notify message.
	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT | ISC_REQ_REPLAY_DETECT | ISC_REQ_CONFIDENTIALITY | ISC_RET_EXTENDED_ERROR | ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_STREAM;

	OutBuffers[0].pvBuffer = nullptr;
	OutBuffers[0].BufferType = SECBUFFER_TOKEN;
	OutBuffers[0].cbBuffer = 0;

	OutBuffer.cBuffers = 1;
	OutBuffer.pBuffers = OutBuffers;
	OutBuffer.ulVersion = SECBUFFER_VERSION;

	Status = g_pSSPI->InitializeSecurityContext(phCreds, phContext, nullptr, dwSSPIFlags, 0, SECURITY_NATIVE_DREP, nullptr, 0, phContext, &OutBuffer, &dwSSPIOutFlags, &tsExpiry);

	if (FAILED(Status))
	{
		// printf("**** Error 0x%x returned by InitializeSecurityContext\n", Status);
		goto cleanup;
	}

	pbMessage = static_cast<PBYTE>(OutBuffers[0].pvBuffer);
	cbMessage = OutBuffers[0].cbBuffer;

	// Send the close notify message to the server.
	if (pbMessage != nullptr && cbMessage != 0)
	{
		cbData = Socket->Send(pbMessage, cbMessage, 0);
		if (cbData == SOCKET_ERROR || cbData == 0)
		{
			Status = WSAGetLastError();
			goto cleanup;
		}
		// printf("Sending Close Notify\n");
		// printf("%d bytes of handshake data sent\n", cbData);
		g_pSSPI->FreeContextBuffer(pbMessage); // Free output buffer.
	}

cleanup:
	g_pSSPI->DeleteSecurityContext(phContext); // Free the security context.
	Socket->Close();

	return Status;
}

static SECURITY_STATUS ReadDecrypt(CSocket * Socket, PCredHandle phCreds, CtxtHandle * phContext, PBYTE pbIoBuffer, DWORD cbIoBufferLength)

// calls recv() - blocking socket read
// http://msdn.microsoft.com/en-us/library/ms740121(VS.85).aspx

// The encrypted message is decrypted in place, overwriting the original contents of its buffer.
// http://msdn.microsoft.com/en-us/library/aa375211(VS.85).aspx

{
	SecBuffer ExtraBuffer;
	SecBuffer * pDataBuffer, * pExtraBuffer;

	SECURITY_STATUS scRet;
	SecBufferDesc Message;
	SecBuffer Buffers[4];

	DWORD cbIoBuffer, cbData, length;
	PBYTE buff;

	// Read data from server until done.
	cbIoBuffer = 0;
	scRet = 0;
	while (TRUE) // Read some data.
	{
		if (cbIoBuffer == 0 || scRet == SEC_E_INCOMPLETE_MESSAGE) // get the data
		{
			cbData = Socket->Receive(pbIoBuffer + cbIoBuffer, cbIoBufferLength - cbIoBuffer, 0);
			if (cbData == SOCKET_ERROR)
			{
				// printf("**** Error %d reading data from server\n", WSAGetLastError());
				scRet = SEC_E_INTERNAL_ERROR;
				break;
			}
			else if (cbData == 0) // Server disconnected.
			{
				if (cbIoBuffer)
				{
					// printf("**** Server unexpectedly disconnected\n");
					scRet = SEC_E_INTERNAL_ERROR;
					return scRet;
				}
				else
					break; // All Done
			}
			else // success
			{
				// printf("%d bytes of (encrypted) application data received\n", cbData);
				cbIoBuffer += cbData;
			}
		}

		// Decrypt the received data.
		Buffers[0].pvBuffer     = pbIoBuffer;
		Buffers[0].cbBuffer     = cbIoBuffer;
		Buffers[0].BufferType   = SECBUFFER_DATA;  // Initial Type of the buffer 1
		Buffers[1].BufferType   = SECBUFFER_EMPTY; // Initial Type of the buffer 2
		Buffers[2].BufferType   = SECBUFFER_EMPTY; // Initial Type of the buffer 3
		Buffers[3].BufferType   = SECBUFFER_EMPTY; // Initial Type of the buffer 4

		Message.ulVersion       = SECBUFFER_VERSION;    // Version number
		Message.cBuffers        = 4;                    // Number of buffers - must contain four SecBuffer structures.
		Message.pBuffers        = Buffers;              // Pointer to array of buffers

		scRet = g_pSSPI->DecryptMessage(phContext, &Message, 0, nullptr);
		if (scRet == SEC_I_CONTEXT_EXPIRED)
			break; // Server signalled end of session
//		if (scRet == SEC_E_INCOMPLETE_MESSAGE - Input buffer has partial encrypted record, read more
		if (scRet != SEC_E_OK && scRet != SEC_I_RENEGOTIATE && scRet != SEC_I_CONTEXT_EXPIRED)
			return scRet;

		// Locate data and (optional) extra buffers.
		pDataBuffer  = nullptr;
		pExtraBuffer = nullptr;
		for (int i = 1; i < 4; ++i)
		{
			if (pDataBuffer  == nullptr && Buffers[i].BufferType == SECBUFFER_DATA)
				pDataBuffer  = &Buffers[i];
			if (pExtraBuffer == nullptr && Buffers[i].BufferType == SECBUFFER_EXTRA)
				pExtraBuffer = &Buffers[i];
		}

		// Display the decrypted data.
		if (pDataBuffer)
		{
			length = pDataBuffer->cbBuffer;
			if (length) // check if last two chars are CR LF
			{
				buff = static_cast<PBYTE>(pDataBuffer->pvBuffer);
				if (buff[length-2] == 13 && buff[length-1] == 10) // Found CRLF
				{
					buff[length] = 0;
					break;
				}
			}
		}

		// Move any "extra" data to the input buffer.
		if (pExtraBuffer)
		{
			MoveMemory(pbIoBuffer, pExtraBuffer->pvBuffer, pExtraBuffer->cbBuffer);
			cbIoBuffer = pExtraBuffer->cbBuffer;
		}
		else
			cbIoBuffer = 0;

		// The server wants to perform another handshake sequence.
		if (scRet == SEC_I_RENEGOTIATE)
		{
			// printf("Server requested renegotiate!\n");
			scRet = ClientHandshakeLoop( Socket, phCreds, phContext, FALSE, &ExtraBuffer);
			if (scRet != SEC_E_OK)
				return scRet;

			if (ExtraBuffer.pvBuffer) // Move any "extra" data to the input buffer.
			{
				MoveMemory(pbIoBuffer, ExtraBuffer.pvBuffer, ExtraBuffer.cbBuffer);
				cbIoBuffer = ExtraBuffer.cbBuffer;
			}
		}
	} // Loop till CRLF is found at the end of the data

	return SEC_E_OK;
}

BOOL CHwSMTP::SendEmail (
		LPCTSTR lpszSmtpSrvHost,
		CCredentials* credentials,
		BOOL bMustAuth,
		LPCTSTR lpszAddrFrom,
		LPCTSTR lpszAddrTo,
		LPCTSTR lpszSubject,
		LPCTSTR lpszBody,
		CStringArray* pStrAryAttach/*=nullptr*/,
		LPCTSTR pStrAryCC/*=nullptr*/,
		UINT nSmtpSrvPort,/*=25*/
		LPCTSTR pSender,
		LPCTSTR pToList,
		DWORD secLevel
		)
{
	m_StrAryAttach.RemoveAll();

	m_StrCC = GET_SAFE_STRING(pStrAryCC);

	m_csSmtpSrvHost = GET_SAFE_STRING ( lpszSmtpSrvHost );
	if ( m_csSmtpSrvHost.GetLength() <= 0 )
	{
		m_csLastError = L"Parameter Error!";
		return FALSE;
	}
	m_credentials = credentials;
	m_bMustAuth = bMustAuth;
	if (m_bMustAuth && (!m_credentials || m_credentials->m_username.IsEmpty()))
	{
		m_csLastError = L"Parameter Error!";
		return FALSE;
	}

	m_csAddrFrom = GET_SAFE_STRING ( lpszAddrFrom );
	m_csAddrTo = GET_SAFE_STRING ( lpszAddrTo );
	m_csSubject = GET_SAFE_STRING ( lpszSubject );
	m_csBody = GET_SAFE_STRING ( lpszBody );

	this->m_csSender = GET_SAFE_STRING(pSender);
	this->m_csToList = GET_SAFE_STRING(pToList);

	m_nSmtpSrvPort = nSmtpSrvPort;

	if	(
			m_csAddrFrom.GetLength() <= 0 || m_csAddrTo.GetLength() <= 0
		)
	{
		m_csLastError = L"Parameter Error!";
		return FALSE;
	}

	if ( pStrAryAttach )
	{
		m_StrAryAttach.Append ( *pStrAryAttach );
	}
	if (m_StrAryAttach.IsEmpty())
		m_csMIMEContentType = L"text/plain\r\nContent-Transfer-Encoding: 8bit";

	// ´´½¨Socket
	m_SendSock.Close();
	if ( !m_SendSock.Create () )
	{
		//int nResult = GetLastError();
		m_csLastError = L"Create socket failed!";
		return FALSE;
	}

	switch (secLevel)
	{
	case 1:
		m_iSecurityLevel = want_tls;
		break;
	case 2:
		m_iSecurityLevel = ssl;
		break;
	default:
		m_iSecurityLevel = none;
	}

	if ( !m_SendSock.Connect ( m_csSmtpSrvHost, m_nSmtpSrvPort ) )
	{
		m_csLastError.Format(L"Connect to [%s] failed", static_cast<LPCTSTR>(m_csSmtpSrvHost));
		return FALSE;
	}

	if (m_iSecurityLevel <= want_tls)
	{
		if (!GetResponse("220"))
			return FALSE;
		m_bConnected = TRUE;
		Send(L"STARTTLS\r\n");
		if (GetResponse("220"))
			m_iSecurityLevel = tls_established;
		else if (m_iSecurityLevel == want_tls)
			return FALSE;
	}

	BOOL ret = FALSE;

	SecBuffer ExtraData;
	SECURITY_STATUS Status;

	CtxtHandle contextStruct;
	CredHandle credentialsStruct;

	if (m_iSecurityLevel >= ssl)
	{
		g_pSSPI = InitSecurityInterface();

		contextStruct.dwLower = 0;
		contextStruct.dwUpper = 0;

		hCreds = &credentialsStruct;
		credentialsStruct.dwLower = 0;
		credentialsStruct.dwUpper = 0;
		Status = CreateCredentials(hCreds);
		if (Status != SEC_E_OK)
		{
			m_csLastError = CFormatMessageWrapper(Status);
			return FALSE;
		}

		hContext = &contextStruct;
		Status = PerformClientHandshake(&m_SendSock, hCreds, m_csSmtpSrvHost.GetBuffer(), hContext, &ExtraData);
		if (Status != SEC_E_OK)
		{
			m_csLastError = CFormatMessageWrapper(Status);
			return FALSE;
		}

		PCCERT_CONTEXT pRemoteCertContext = nullptr;
		// Authenticate server's credentials. Get server's certificate.
		Status = g_pSSPI->QueryContextAttributes(hContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, static_cast<PVOID>(&pRemoteCertContext));
		if (Status)
		{
			m_csLastError = CFormatMessageWrapper(Status);
			goto cleanup;
		}

		git_cert_x509 cert;
		cert.parent.cert_type = GIT_CERT_X509;
		cert.data = pRemoteCertContext->pbCertEncoded;
		cert.len = pRemoteCertContext->cbCertEncoded;
		if (CAppUtils::Git2CertificateCheck(reinterpret_cast<git_cert*>(&cert), 0, CUnicodeUtils::GetUTF8(m_csSmtpSrvHost), nullptr))
		{
			CertFreeCertificateContext(pRemoteCertContext);
			m_csLastError = L"Invalid certificate.";
			goto cleanup;
		}

		CertFreeCertificateContext(pRemoteCertContext);

		Status = g_pSSPI->QueryContextAttributes(hContext, SECPKG_ATTR_STREAM_SIZES, &Sizes);
		if (Status)
		{
			m_csLastError = CFormatMessageWrapper(Status);
			goto cleanup;
		}

		// Create a buffer.
		cbIoBufferLength = Sizes.cbHeader + Sizes.cbMaximumMessage + Sizes.cbTrailer;
		pbIoBuffer = static_cast<PBYTE>(LocalAlloc(LMEM_FIXED, cbIoBufferLength));
		if (pbIoBuffer == nullptr)
		{
			m_csLastError = L"Could not allocate memory";
			goto cleanup;
		}
		SecureZeroMemory(pbIoBuffer, cbIoBufferLength);
	}

	if (m_iSecurityLevel <= ssl)
	{
		if (!GetResponse("220"))
			goto cleanup;
		m_bConnected = TRUE;
	}

	ret = SendEmail();

cleanup:
	if (m_iSecurityLevel >= ssl)
	{
		if (hContext && hCreds)
			DisconnectFromServer(&m_SendSock, hCreds, hContext);
		if (pbIoBuffer)
		{
			SecureZeroMemory(pbIoBuffer, cbIoBufferLength);
			LocalFree(pbIoBuffer);
			pbIoBuffer = nullptr;
			cbIoBufferLength = 0;
		}
		if (hContext)
		{
			g_pSSPI->DeleteSecurityContext(hContext);
			hContext = nullptr;
		}
		if (hCreds)
		{
			g_pSSPI->FreeCredentialsHandle(hCreds);
			hCreds = nullptr;
		}
		g_pSSPI = nullptr;
	}
	else
		m_SendSock.Close();

	return ret;
}

BOOL CHwSMTP::GetResponse(LPCSTR lpszVerifyCode)
{
	if (!lpszVerifyCode || strlen(lpszVerifyCode) < 1)
		return FALSE;

	SECURITY_STATUS scRet = SEC_E_OK;

	char szRecvBuf[1024] = {0};
	int nRet = 0;
	char szStatusCode[4] = {0};

	if (m_iSecurityLevel >= ssl)
	{
		scRet = ReadDecrypt(&m_SendSock, hCreds, hContext, pbIoBuffer, cbIoBufferLength);
		SecureZeroMemory(szRecvBuf, 1024);
		memcpy(szRecvBuf, pbIoBuffer+Sizes.cbHeader, 1024);
	}
	else
		nRet = m_SendSock.Receive(szRecvBuf, sizeof(szRecvBuf));
	//TRACE(L"Received : %s\r\n", szRecvBuf);
	if (nRet == 0 && m_iSecurityLevel == none || m_iSecurityLevel >= ssl && scRet != SEC_E_OK)
	{
		m_csLastError = L"Receive TCP data failed";
		return FALSE;
	}
	memcpy ( szStatusCode, szRecvBuf, 3 );
	if (strcmp(szStatusCode, lpszVerifyCode) != 0)
	{
		m_csLastError.Format(L"Received invalid response: %s", static_cast<LPCTSTR>(CUnicodeUtils::GetUnicode(szRecvBuf)));
		return FALSE;
	}

	return TRUE;
}
BOOL CHwSMTP::SendBuffer(const char* buff, int size)
{
	if(size<0)
		size = static_cast<int>(strlen(buff));
	if ( !m_bConnected )
	{
		m_csLastError = L"Didn't connect";
		return FALSE;
	}

	if (m_iSecurityLevel >= ssl)
	{
		int sent = 0;
		while (size - sent > 0)
		{
			int toSend = min(size - sent, static_cast<int>(Sizes.cbMaximumMessage));
			SecureZeroMemory(pbIoBuffer + Sizes.cbHeader, Sizes.cbMaximumMessage);
			memcpy(pbIoBuffer + Sizes.cbHeader, buff + sent, toSend);
			DWORD cbData = EncryptSend(&m_SendSock, hContext, pbIoBuffer, Sizes);
			if (cbData == SOCKET_ERROR || cbData == 0)
				return FALSE;
			sent += toSend;
		}
	}
	else if (m_SendSock.Send ( buff, size ) != size)
	{
		m_csLastError = L"Socket send data failed";
		return FALSE;
	}

	return TRUE;
}

BOOL CHwSMTP::Send(const CString &str )
{
	return Send(CUnicodeUtils::GetUTF8(str));
}

BOOL CHwSMTP::Send(const CStringA &str)
{
	//TRACE(L"Send: %s\r\n", static_cast<LPCTSTR>(CUnicodeUtils::GetUnicode(str)));
	return SendBuffer(str, str.GetLength());
}

BOOL CHwSMTP::SendEmail()
{
	CStringA hostname;
	gethostname(CStrBufA(hostname, 64), 64);

	// make sure helo hostname can be interpreted as a FQDN
	if (hostname.Find(".") == -1)
		hostname += ".local";

	CStringA str;
	str.Format("HELO %s\r\n", static_cast<LPCSTR>(hostname));
	if (!Send(str))
		return FALSE;
	if (!GetResponse("250"))
		return FALSE;

	if ( m_bMustAuth && !auth() )
		return FALSE;

	if ( !SendHead() )
		return FALSE;

	if (!SendSubject(CUnicodeUtils::GetUnicode(hostname)))
		return FALSE;

	if ( !SendBody() )
		return FALSE;

	if ( !SendAttach() )
	{
		return FALSE;
	}

	if (!Send(".\r\n"))
		return FALSE;
	if (!GetResponse("250"))
		return FALSE;

	if ( HANDLE_IS_VALID(m_SendSock.m_hSocket) )
		Send("QUIT\r\n");
	m_bConnected = FALSE;

	return TRUE;
}

static CStringA EncodeBase64(const char* source, int len, bool noWrap)
{
	int neededLength = Base64EncodeGetRequiredLength(len);
	CStringA output;
	if (Base64Encode(reinterpret_cast<const BYTE*>(source), len, CStrBufA(output, neededLength), &neededLength, noWrap ? ATL_BASE64_FLAG_NOCRLF : 0))
		output.Truncate(neededLength);
	return output;
}

static CStringA EncodeBase64(const CString& source)
{
	CStringA buf = CUnicodeUtils::GetUTF8(source);
	return EncodeBase64(buf, buf.GetLength(), true);
}

CString CHwSMTP::GetEncodedHeader(const CString& text)
{
	if (CStringUtils::IsPlainReadableASCII(text))
		return text;

	return L"=?UTF-8?B?" + CUnicodeUtils::GetUnicode(EncodeBase64(text), true) + L"?=";
}

BOOL CHwSMTP::auth()
{
	if (!Send("auth login\r\n"))
		return FALSE;
	if (!GetResponse("334"))
		return FALSE;

	if (!Send(EncodeBase64(m_credentials->m_username) + "\r\n"))
		return FALSE;

	if (!GetResponse("334"))
	{
		m_csLastError = L"Authentication UserName failed";
		return FALSE;
	}

	if (m_credentials->m_password[0] != L'\0')
	{
		auto len = static_cast<int>(_tcslen(m_credentials->m_password));
		auto size = len * 4 + 1;
		auto buf = new char[size];
		auto ret = WideCharToMultiByte(CP_UTF8, 0, m_credentials->m_password, len, buf, size - 1, nullptr, nullptr);
		buf[ret] = '\0';

		int neededLength = Base64EncodeGetRequiredLength(len);
		auto bufBase64 = new char[neededLength + 1];
		bufBase64[0] = '\0';
		if (Base64Encode(reinterpret_cast<const BYTE*>(buf), ret, bufBase64, &neededLength, ATL_BASE64_FLAG_NOCRLF))
			bufBase64[neededLength] = '\0';

		auto successful = SendBuffer(bufBase64, neededLength);

		SecureZeroMemory(bufBase64, neededLength + 1);
		delete[] bufBase64;
		SecureZeroMemory(buf, size);
		delete[] buf;
		if (!successful)
			return FALSE;
	}
	if (!Send("\r\n"))
		return FALSE;

	if (!GetResponse("235"))
	{
		m_csLastError = L"Authentication Password failed";
		return FALSE;
	}

	return TRUE;
}

BOOL CHwSMTP::SendHead()
{
	CString str;
	CString addr;
	CStringUtils::ParseEmailAddress(m_csAddrFrom, addr);

	str.Format(L"MAIL From: <%s>\r\n", static_cast<LPCTSTR>(addr));
	if (!Send(str))
		return FALSE;

	if (!GetResponse("250"))
		return FALSE;

	int start=0;
	while(start>=0)
	{
		CString one = m_csAddrTo.Tokenize(L";", start).Trim();
		if(one.IsEmpty())
			continue;

		CStringUtils::ParseEmailAddress(one, addr);

		str.Format(L"RCPT TO: <%s>\r\n", static_cast<LPCTSTR>(addr));
		if (!Send(str))
			return FALSE;
		if (!GetResponse("250"))
			return FALSE;
	}

	if (!Send("DATA\r\n"))
		return FALSE;
	if (!GetResponse("354"))
		return FALSE;

	return TRUE;
}

BOOL CHwSMTP::SendSubject(const CString &hostname)
{
	CString csSubject;
	csSubject += L"Date: ";
	COleDateTime tNow = COleDateTime::GetCurrentTime();
	if ( tNow > 1 )
		csSubject += FormatDateTime(tNow);
	csSubject += L"\r\n";
	csSubject.AppendFormat(L"From: %s\r\n", static_cast<LPCTSTR>(m_csAddrFrom));

	if (!m_StrCC.IsEmpty())
		csSubject.AppendFormat(L"CC: %s\r\n", static_cast<LPCTSTR>(m_StrCC));

	if(m_csSender.IsEmpty())
		m_csSender =  this->m_csAddrFrom;

	csSubject.AppendFormat(L"Sender: %s\r\n", static_cast<LPCTSTR>(m_csSender));

	if(this->m_csToList.IsEmpty())
		m_csToList = m_csReceiverName;

	csSubject.AppendFormat(L"To: %s\r\n", static_cast<LPCTSTR>(m_csToList));

	csSubject.AppendFormat(L"Subject: %s\r\n", static_cast<LPCTSTR>(GetEncodedHeader(m_csSubject)));

	CString m_ListID(GetGUID());
	if (m_ListID.IsEmpty())
	{
		m_csLastError = L"Could not generate Message-ID";
		return FALSE;
	}
	csSubject.AppendFormat(L"Message-ID: <%s@%s>\r\n", static_cast<LPCTSTR>(m_ListID), static_cast<LPCTSTR>(hostname));
	csSubject.AppendFormat(L"X-Mailer: TortoiseGit\r\nMIME-Version: 1.0\r\nContent-Type: %s\r\n\r\n", static_cast<LPCTSTR>(m_csMIMEContentType));

	return Send(csSubject);
}

BOOL CHwSMTP::SendBody()
{
	CString csBody;

	if (!m_StrAryAttach.IsEmpty())
	{
		csBody.AppendFormat(L"%s\r\n\r\n", static_cast<LPCTSTR>(m_csNoMIMEText));
		csBody.AppendFormat(L"--%s\r\n", static_cast<LPCTSTR>(m_csPartBoundary));
		csBody += L"Content-Type: text/plain\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
	}

	m_csBody.Replace(L"\n.\n", L"\n..\n");
	m_csBody.Replace(L"\n.\r\n", L"\n..\r\n");

	csBody += m_csBody;
	csBody += L"\r\n";

	return Send(csBody);
}

BOOL CHwSMTP::SendAttach()
{
	if (m_StrAryAttach.IsEmpty())
		return TRUE;

	int nCountAttach = static_cast<int>(m_StrAryAttach.GetSize());
	for (int i = 0; i < nCountAttach; ++i)
	{
		if ( !SendOnAttach ( m_StrAryAttach.GetAt(i) ) )
			return FALSE;
	}

	Send(L"--" + m_csPartBoundary + L"--\r\n");

	return TRUE;
}

BOOL CHwSMTP::SendOnAttach(LPCTSTR lpszFileName)
{
	ASSERT ( lpszFileName );
	CString csAttach;
	CString csShortFileName = GetEncodedHeader(CPathUtils::GetFileNameFromPath(lpszFileName));

	csAttach.AppendFormat(L"--%s\r\n", static_cast<LPCTSTR>(m_csPartBoundary));
	csAttach.AppendFormat(L"Content-Type: application/octet-stream; file=\"%s\"\r\n", static_cast<LPCTSTR>(csShortFileName));
	csAttach.AppendFormat(L"Content-Transfer-Encoding: base64\r\n");
	csAttach.AppendFormat(L"Content-Disposition: attachment; filename=\"%s\"\r\n\r\n", static_cast<LPCTSTR>(csShortFileName));

	auto dwFileSize = GetFileSize(lpszFileName);
	if ( dwFileSize > 5*1024*1024 )
	{
		m_csLastError.Format(L"File [%s] too big. File size is : %s", lpszFileName, static_cast<LPCTSTR>(FormatBytes(dwFileSize)));
		return FALSE;
	}
	auto pBuf = std::make_unique<char[]>(dwFileSize + 1);
	if (!pBuf)
		::AfxThrowMemoryException();

	if (!Send(csAttach))
		return FALSE;

	CFile file;
	CStringA filedata;
	try
	{
		if ( !file.Open ( lpszFileName, CFile::modeRead ) )
		{
			m_csLastError.Format(L"Open file [%s] failed", lpszFileName);
			return FALSE;
		}
		UINT nFileLen = file.Read(pBuf.get(), dwFileSize);
		filedata = EncodeBase64(pBuf.get(), nFileLen, false);
		filedata += L"\r\n\r\n";
	}
	catch (CFileException *e)
	{
		e->Delete();
		m_csLastError.Format(L"Read file [%s] failed", lpszFileName);
		return FALSE;
	}

	if (!SendBuffer(filedata))
		return FALSE;

	return TRUE;
}

CString CHwSMTP::GetLastErrorText()
{
	return m_csLastError;
}
