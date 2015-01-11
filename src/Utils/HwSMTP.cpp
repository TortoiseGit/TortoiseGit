// HwSMTP.cpp: implementation of the CHwSMTP class.
//
// Schannel/SSPI implementation based on http://www.coastrd.com/c-schannel-smtp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "afxstr.h"
#include "HwSMTP.h"
#include "SpeedPostEmail.h"
#include "Windns.h"
#include <Afxmt.h>
#include "FormatMessageWrapper.h"
#include <atlenc.h>
#include "AppUtils.h"

#define IO_BUFFER_SIZE 0x10000

#pragma comment(lib, "Secur32.lib")

DWORD dwProtocol = SP_PROT_TLS1; // SP_PROT_TLS1; // SP_PROT_PCT1; SP_PROT_SSL2; SP_PROT_SSL3; 0=default
ALG_ID aiKeyExch = 0; // = default; CALG_DH_EPHEM; CALG_RSA_KEYX;

SCHANNEL_CRED SchannelCred;
PSecurityFunctionTable g_pSSPI;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CHwSMTP::CHwSMTP () :
	m_bConnected ( FALSE ),
	m_nSmtpSrvPort ( 25 ),
	m_bMustAuth ( TRUE )
{
	m_csPartBoundary = _T( "WC_MAIL_PaRt_BoUnDaRy_05151998" );
	m_csMIMEContentType = FormatString ( _T( "multipart/mixed; boundary=%s" ), m_csPartBoundary);
	m_csNoMIMEText = _T( "This is a multi-part message in MIME format." );
	//m_csCharSet = _T("\r\n\tcharset=\"iso-8859-1\"\r\n");

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

void CHwSMTP::GetNameAddress(CString &in, CString &name,CString &address)
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

CString CHwSMTP::GetServerAddress(CString &email)
{
	CString str;
	int start,end;

	start = email.Find(_T("<"));
	end = email.Find(_T(">"));

	if(start>=0 && end >=0)
	{
		str=email.Mid(start+1,end-start-1);
	}
	else
	{
		str=email;
	}

	start = str.Find(_T('@'));
	return str.Mid(start+1);

}

BOOL CHwSMTP::SendSpeedEmail
		(
			LPCTSTR	lpszAddrFrom,
			LPCTSTR	lpszAddrTo,
			LPCTSTR	lpszSubject,
			LPCTSTR	lpszBody,
			LPCTSTR	lpszCharSet,
			CStringArray *pStrAryAttach,
			LPCTSTR	pStrAryCC,
			LPCTSTR	pSend
		)
{

	BOOL ret=true;
	CString To;
	To += GET_SAFE_STRING(lpszAddrTo);
	To += _T(";");
	To += GET_SAFE_STRING(pStrAryCC);

	std::map<CString,std::vector<CString>> Address;

	int start = 0;
	while( start >= 0 )
	{
		CString one= To.Tokenize(_T(";"),start);
		one=one.Trim();
		if(one.IsEmpty())
			continue;

		CString addr;
		addr = GetServerAddress(one);
		if(addr.IsEmpty())
			continue;

		Address[addr].push_back(one);

	}

	std::map<CString,std::vector<CString>>::iterator itr1  =  Address.begin();
	for(  ;  itr1  !=  Address.end();  ++itr1 )
	{
		PDNS_RECORD pDnsRecord;
		PDNS_RECORD pNext;

		DNS_STATUS status = 
		DnsQuery(itr1->first ,
						DNS_TYPE_MX,DNS_QUERY_STANDARD,
						NULL,			//Contains DNS server IP address.
						&pDnsRecord,	//Resource record that contains the response.
						NULL
						);
		if (status)
		{
			m_csLastError.Format(_T("DNS query failed %d"), status);
			ret = false;
			continue;
		}

		CString to;
		to.Empty();
		for (size_t i = 0; i < itr1->second.size(); ++i)
		{
			to+=itr1->second[i];
			to+=_T(";");
		}
		if(to.IsEmpty())
			continue;

		pNext=pDnsRecord;
		while(pNext)
		{
			if(pNext->wType == DNS_TYPE_MX)
				if(SendEmail(pNext->Data.MX.pNameExchange,NULL,NULL,false,
					lpszAddrFrom,to,lpszSubject,lpszBody,lpszCharSet,pStrAryAttach,pStrAryCC,
					25,pSend,lpszAddrTo))
					break;
			pNext=pNext->pNext;
		}
		if(pNext == NULL)
			ret = false;

		if (pDnsRecord)
			DnsRecordListFree(pDnsRecord,DnsFreeRecordList);
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
	PUCHAR IoBuffer;
	BOOL fDoRead;

	dwSSPIFlags = ISC_REQ_SEQUENCE_DETECT	| ISC_REQ_REPLAY_DETECT		| ISC_REQ_CONFIDENTIALITY |
				  ISC_RET_EXTENDED_ERROR	| ISC_REQ_ALLOCATE_MEMORY	| ISC_REQ_STREAM;

	// Allocate data buffer.
	IoBuffer = new UCHAR[IO_BUFFER_SIZE];
	if (IoBuffer == nullptr)
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
				cbData = Socket->Receive(IoBuffer + cbIoBuffer, IO_BUFFER_SIZE - cbIoBuffer, 0);
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
		InBuffers[0].pvBuffer	= IoBuffer;
		InBuffers[0].cbBuffer	= cbIoBuffer;
		InBuffers[0].BufferType	= SECBUFFER_TOKEN;

		InBuffers[1].pvBuffer	= nullptr;
		InBuffers[1].cbBuffer	= 0;
		InBuffers[1].BufferType	= SECBUFFER_EMPTY;

		InBuffer.cBuffers		= 2;
		InBuffer.pBuffers		= InBuffers;
		InBuffer.ulVersion		= SECBUFFER_VERSION;

		// Set up the output buffers. These are initialized to NULL
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

				MoveMemory(pExtraData->pvBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);

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
			MoveMemory(IoBuffer, IoBuffer + (cbIoBuffer - InBuffers[1].cbBuffer), InBuffers[1].cbBuffer);
			cbIoBuffer = InBuffers[1].cbBuffer;
		}
		else
			cbIoBuffer = 0;
	}

	// Delete the security context in the case of a fatal error.
	if (FAILED(scRet))
		g_pSSPI->DeleteSecurityContext(phContext);
	delete[] IoBuffer;

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
	cbMessage = (DWORD)strlen((char *)pbMessage);

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

	pbMessage = (PBYTE)OutBuffers[0].pvBuffer;
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
				buff = (PBYTE)pDataBuffer->pvBuffer;
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
		LPCTSTR lpszUserName,
		LPCTSTR lpszPasswd,
		BOOL bMustAuth,
		LPCTSTR lpszAddrFrom,
		LPCTSTR lpszAddrTo,
		LPCTSTR lpszSubject,
		LPCTSTR lpszBody,
		LPCTSTR lpszCharSet,						// 字符集类型，例如：繁体中文这里应输入"big5"，简体中文时输入"gb2312"
		CStringArray *pStrAryAttach/*=NULL*/,
		LPCTSTR pStrAryCC/*=NULL*/,
		UINT nSmtpSrvPort,/*=25*/
		LPCTSTR pSender,
		LPCTSTR pToList,
		DWORD secLevel
		)
{
	TRACE ( _T("发送邮件：%s,  %s\n"), lpszAddrTo, lpszBody );
	m_StrAryAttach.RemoveAll();

	m_StrCC += GET_SAFE_STRING(pStrAryCC);

	m_csSmtpSrvHost = GET_SAFE_STRING ( lpszSmtpSrvHost );
	if ( m_csSmtpSrvHost.GetLength() <= 0 )
	{
		m_csLastError.Format ( _T("Parameter Error!") );
		return FALSE;
	}
	m_csUserName = GET_SAFE_STRING ( lpszUserName );
	m_csPasswd = GET_SAFE_STRING ( lpszPasswd );
	m_bMustAuth = bMustAuth;
	if ( m_bMustAuth && m_csUserName.GetLength() <= 0 )
	{
		m_csLastError.Format ( _T("Parameter Error!") );
		return FALSE;
	}

	m_csAddrFrom = GET_SAFE_STRING ( lpszAddrFrom );
	m_csAddrTo = GET_SAFE_STRING ( lpszAddrTo );
//	m_csFromName = GET_SAFE_STRING ( lpszFromName );
//	m_csReceiverName = GET_SAFE_STRING ( lpszReceiverName );
	m_csSubject = GET_SAFE_STRING ( lpszSubject );
	m_csBody = GET_SAFE_STRING ( lpszBody );

	this->m_csSender = GET_SAFE_STRING(pSender);
	this->m_csToList = GET_SAFE_STRING(pToList);

	m_nSmtpSrvPort = nSmtpSrvPort;

	if ( lpszCharSet && lstrlen(lpszCharSet) > 0 )
		m_csCharSet.Format ( _T("\r\n\tcharset=\"%s\"\r\n"), lpszCharSet );

	if	(
			m_csAddrFrom.GetLength() <= 0 || m_csAddrTo.GetLength() <= 0
		)
	{
		m_csLastError.Format ( _T("Parameter Error!") );
		return FALSE;
	}

	if ( pStrAryAttach )
	{
		m_StrAryAttach.Append ( *pStrAryAttach );
	}
	if ( m_StrAryAttach.GetSize() < 1 )
		m_csMIMEContentType = FormatString ( _T( "text/plain; %s" ), m_csCharSet);

	// 创建Socket
	m_SendSock.Close();
	if ( !m_SendSock.Create () )
	{
		//int nResult = GetLastError();
		m_csLastError.Format ( _T("Create socket failed!") );
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

	// 连接到服务器
	if ( !m_SendSock.Connect ( m_csSmtpSrvHost, m_nSmtpSrvPort ) )
	{
		m_csLastError.Format ( _T("Connect to [ %s ] failed"), m_csSmtpSrvHost );
		TRACE ( _T("%d\n"), GetLastError() );
		return FALSE;
	}

	if (m_iSecurityLevel == want_tls) {
		if (!GetResponse(_T("220")))
			return FALSE;
		m_bConnected = TRUE;
		Send(L"STARTTLS\n");
		if (!GetResponse(_T("220")))
			return FALSE;
		m_iSecurityLevel = tls_established;
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
		Status = g_pSSPI->QueryContextAttributes(hContext, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (PVOID)&pRemoteCertContext);
		if (Status)
		{
			m_csLastError = CFormatMessageWrapper(Status);
			goto cleanup;
		}

		git_cert_x509 cert;
		cert.cert_type = GIT_CERT_X509;
		cert.data = pRemoteCertContext->pbCertEncoded;
		cert.len = pRemoteCertContext->cbCertEncoded;
		if (CAppUtils::Git2CertificateCheck((git_cert*)&cert, 0, CUnicodeUtils::GetUTF8(m_csSmtpSrvHost), nullptr))
		{
			CertFreeCertificateContext(pRemoteCertContext);
			m_csLastError = _T("Invalid certificate.");
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
		pbIoBuffer = (PBYTE)LocalAlloc(LMEM_FIXED, cbIoBufferLength);
		SecureZeroMemory(pbIoBuffer, cbIoBufferLength);
		if (pbIoBuffer == nullptr)
		{
			m_csLastError = _T("Could not allocate memory");
			goto cleanup;
		}
	}

	if (m_iSecurityLevel <= ssl)
	{
		if (!GetResponse(_T("220")))
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

BOOL CHwSMTP::GetResponse ( LPCTSTR lpszVerifyCode, int *pnCode/*=NULL*/)
{
	if ( !lpszVerifyCode || lstrlen(lpszVerifyCode) < 1 )
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
	TRACE ( _T("Received : %s\r\n"), szRecvBuf );
	if (nRet == 0 && m_iSecurityLevel == none || m_iSecurityLevel >= ssl && scRet != SEC_E_OK)
	{
		m_csLastError.Format ( _T("Receive TCP data failed") );
		return FALSE;
	}
//	TRACE ( _T("收到服务器回应：%s\n"), szRecvBuf );

	memcpy ( szStatusCode, szRecvBuf, 3 );
	if ( pnCode ) (*pnCode) = atoi ( szStatusCode );

	if ( strcmp ( szStatusCode, CMultiByteString(lpszVerifyCode).GetBuffer() ) != 0 )
	{
		m_csLastError.Format ( _T("Received invalid response  : %s"), GetCompatibleString(szRecvBuf,FALSE) );
		return FALSE;
	}

	return TRUE;
}
BOOL CHwSMTP::SendBuffer(char *buff,int size)
{
	if(size<0)
		size=(int)strlen(buff);
	if ( !m_bConnected )
	{
		m_csLastError.Format ( _T("Didn't connect") );
		return FALSE;
	}

	if (m_iSecurityLevel >= ssl)
	{
		int sent = 0;
		while (size - sent > 0)
		{
			int toSend = min(size - sent, (int)Sizes.cbMaximumMessage);
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
		m_csLastError.Format ( _T("Socket send data failed") );
		return FALSE;
	}

	return TRUE;
}
// 利用socket发送数据，数据长度不能超过10M
BOOL CHwSMTP::Send(const CString &str )
{
	CMultiByteString cbsData ( str );

	TRACE ( _T("Send : %s\r\n"), cbsData.GetBuffer() );
	return SendBuffer(cbsData.GetBuffer(), cbsData.GetLength());
}

BOOL CHwSMTP::SendEmail()
{
	BOOL bRet = TRUE;
	char szLocalHostName[64] = {0};
	gethostname ( (char*)szLocalHostName, sizeof(szLocalHostName) );

	// make sure helo hostname can be interpreted as a FQDN
	CString hostname(GetCompatibleString(szLocalHostName, FALSE));
	if (hostname.Find(_T(".")) == -1)
		hostname += _T(".local");

	// hello，握手
	CString str;
	str.Format(_T("HELO %s\r\n"), hostname);
	if ( !Send (  str ))
	{
		return FALSE;
	}
	if ( !GetResponse ( _T("250") ) )
	{
		return FALSE;
	}
	// 身份验证
	if ( m_bMustAuth && !auth() )
	{
		return FALSE;
	}
	// 发送邮件头
	if ( !SendHead() )
	{
		return FALSE;
	}
	// 发送邮件主题
	if (!SendSubject(hostname))
	{
		return FALSE;
	}
	// 发送邮件正文
	if ( !SendBody() )
	{
		return FALSE;
	}
	// 发送附件
	if ( !SendAttach() )
	{
		return FALSE;
	}
	// 结束邮件正文
	if ( !Send ( CString(_T(".\r\n") ) ) ) return FALSE;
	if ( !GetResponse ( _T("250") ) )
		return FALSE;

	// 退出发送
	if ( HANDLE_IS_VALID(m_SendSock.m_hSocket) )
		Send ( CString(_T("QUIT\r\n")) );
	m_bConnected = FALSE;

	return bRet;
}

static CStringA EncodeBase64(const char * source, int len)
{
	int neededLength = Base64EncodeGetRequiredLength(len);
	CStringA output;
	if (!Base64Encode((BYTE *)source, len, output.GetBufferSetLength(neededLength), &neededLength))
	{
		output.ReleaseBuffer(0);
		return output;
	}
	output.ReleaseBuffer(neededLength);
	return output;
}

BOOL CHwSMTP::auth()
{
	int nResponseCode = 0;
	if ( !Send ( CString(_T("auth login\r\n")) ) ) return FALSE;
	if ( !GetResponse ( _T("334"), &nResponseCode ) ) return FALSE;
	if ( nResponseCode != 334 )	// 不需要验证用户名和密码
		return TRUE;

	CMultiByteString cbsUserName ( m_csUserName ), cbsPasswd ( m_csPasswd );
	CString csBase64_UserName = GetCompatibleString(EncodeBase64(cbsUserName.GetBuffer(), cbsUserName.GetLength()).GetBuffer(0), FALSE);
	CString csBase64_Passwd = GetCompatibleString(EncodeBase64(cbsPasswd.GetBuffer(), cbsPasswd.GetLength()).GetBuffer(0), FALSE);

	CString str;
	str.Format( _T("%s\r\n"), csBase64_UserName );
	if ( !Send ( str ) )
		return FALSE;

	if ( !GetResponse ( _T("334") ) )
	{
		m_csLastError.Format ( _T("Authentication UserName failed") );
		return FALSE;
	}

	str.Format(_T("%s\r\n"), csBase64_Passwd );
	if ( !Send ( str ) )
		return FALSE;

	if ( !GetResponse ( _T("235") ) )
	{
		m_csLastError.Format ( _T("Authentication Password failed") );
		return FALSE;
	}

	return TRUE;
}

BOOL CHwSMTP::SendHead()
{
	CString str;
	CString name,addr;
	GetNameAddress(m_csAddrFrom,name,addr);

	str.Format( _T("MAIL From: <%s>\r\n"), addr );
	if ( !Send ( str  ) ) return FALSE;

	if ( !GetResponse ( _T("250") ) ) return FALSE;

	int start=0;
	while(start>=0)
	{
		CString one=m_csAddrTo.Tokenize(_T(";"),start);
		one=one.Trim();
		if(one.IsEmpty())
			continue;


		GetNameAddress(one,name,addr);

		str.Format(_T("RCPT TO: <%s>\r\n"), addr );
		if ( !Send ( str ) ) return FALSE;
		if ( !GetResponse ( _T("250") ) ) return FALSE;
	}

	if ( !Send ( CString(_T("DATA\r\n") ) ) ) return FALSE;
	if ( !GetResponse ( CString(_T("354") )) ) return FALSE;

	return TRUE;
}

BOOL CHwSMTP::SendSubject(const CString &hostname)
{
	CString csSubject;
	csSubject += _T("Date: ");
	COleDateTime tNow = COleDateTime::GetCurrentTime();
	if ( tNow > 1 )
	{
		csSubject += FormatDateTime (tNow, _T("%a, %d %b %y %H:%M:%S %Z"));
	}
	csSubject += _T("\r\n");
	csSubject += FormatString ( _T("From: %s\r\n"), this->m_csAddrFrom);

	if (!m_StrCC.IsEmpty())
		csSubject += FormatString ( _T("CC: %s\r\n"), this->m_StrCC);

	if(m_csSender.IsEmpty())
		m_csSender =  this->m_csAddrFrom;

	csSubject += FormatString ( _T("Sender: %s\r\n"), this->m_csSender);

	if(this->m_csToList.IsEmpty())
		m_csToList = m_csReceiverName;

	csSubject += FormatString ( _T("To: %s\r\n"), this->m_csToList);

	CString m_csToList;

	csSubject += FormatString ( _T("Subject: %s\r\n"), m_csSubject );

	CString m_ListID;
	GUID guid;
	HRESULT hr = CoCreateGuid(&guid);
	if (hr == S_OK)
	{
		RPC_WSTR guidStr;
		if (UuidToString(&guid, &guidStr) == RPC_S_OK)
		{
			m_ListID = (LPTSTR)guidStr;
			RpcStringFree(&guidStr);
		}
	}
	if (m_ListID.IsEmpty())
	{
		m_csLastError = _T("Could not generate Message-ID");
		return FALSE;
	}
	csSubject += FormatString( _T("Message-ID: <%s@%s>\r\n"), m_ListID, hostname);

	csSubject += FormatString ( _T("X-Mailer: TortoiseGit\r\nMIME-Version: 1.0\r\nContent-Type: %s\r\n\r\n"),
		m_csMIMEContentType );

	return Send ( csSubject );
}

BOOL CHwSMTP::SendBody()
{
	CString csBody, csTemp;

	if ( m_StrAryAttach.GetSize() > 0 )
	{
		csTemp.Format ( _T("%s\r\n\r\n"), m_csNoMIMEText );
		csBody += csTemp;

		csTemp.Format ( _T("--%s\r\n"), m_csPartBoundary );
		csBody += csTemp;

		csTemp.Format ( _T("Content-Type: text/plain\r\n%sContent-Transfer-Encoding: UTF-8\r\n\r\n"),
			m_csCharSet );
		csBody += csTemp;
	}

	//csTemp.Format ( _T("%s\r\n"), m_csBody );
	csBody += m_csBody;
	csBody += _T("\r\n");

	return Send ( csBody );
}

BOOL CHwSMTP::SendAttach()
{
	int nCountAttach = (int)m_StrAryAttach.GetSize();
	if ( nCountAttach < 1 ) return TRUE;

	for ( int i=0; i<nCountAttach; i++ )
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
	CString csAttach, csTemp;

	csTemp = lpszFileName;
	CString csShortFileName = csTemp.GetBuffer(0) + csTemp.ReverseFind ( '\\' );
	csShortFileName.TrimLeft ( _T("\\") );

	csTemp.Format ( _T("--%s\r\n"), m_csPartBoundary );
	csAttach += csTemp;

	csTemp.Format ( _T("Content-Type: application/octet-stream; file=%s\r\n"), csShortFileName );
	csAttach += csTemp;

	csTemp.Format ( _T("Content-Transfer-Encoding: base64\r\n") );
	csAttach += csTemp;

	csTemp.Format ( _T("Content-Disposition: attachment; filename=%s\r\n\r\n"), csShortFileName );
	csAttach += csTemp;

	DWORD dwFileSize =  hwGetFileAttr(lpszFileName);
	if ( dwFileSize > 5*1024*1024 )
	{
		m_csLastError.Format ( _T("File [%s] too big. File size is : %s"), lpszFileName, FormatBytes(dwFileSize) );
		return FALSE;
	}
	char *pBuf = new char[dwFileSize+1];
	if ( !pBuf )
	{
		::AfxThrowMemoryException ();
	}

	if(!Send ( csAttach ))
	{
		delete[] pBuf;
		return FALSE;
	}

	CFile file;
	CStringA filedata;
	try
	{
		if ( !file.Open ( lpszFileName, CFile::modeRead ) )
		{
			m_csLastError.Format ( _T("Open file [%s] failed"), lpszFileName );			
			delete[] pBuf;
			return FALSE;
		}
		UINT nFileLen = file.Read ( pBuf, dwFileSize );
		filedata = EncodeBase64(pBuf, nFileLen);
		filedata += _T("\r\n\r\n");
	}
	catch (CFileException *e)
	{
		e->Delete();
		m_csLastError.Format ( _T("Read file [%s] failed"), lpszFileName );
		delete[] pBuf;
		return FALSE;
	}

	if(!SendBuffer( filedata.GetBuffer() ))
	{
		delete[] pBuf;
		return FALSE;
	}

	delete[] pBuf;

	return TRUE;
	//return Send ( csAttach );
}

CString CHwSMTP::GetLastErrorText()
{
	return m_csLastError;
}

//
// 将字符串 lpszOrg 转换为多字节的字符串，如果还要使用多字符串的长度，可以用以下方式来使用这个类：
// CMultiByteString MultiByteString(_T("UNICODE字符串"));
// printf ( "ANSI 字符串为： %s， 字符个数为： %d ， 长度为： %d字节\n", MultiByteString.GetBuffer(), MultiByteString.GetLength(), MultiByteString.GetSize() );
//
CMultiByteString::CMultiByteString( LPCTSTR lpszOrg, int nOrgStringEncodeType/*=STRING_IS_SOFTCODE*/, OUT char *pOutBuf/*=NULL*/, int nOutBufSize/*=0*/ )
{
	m_bNewBuffer = FALSE;
	m_pszData = NULL;
	m_nDataSize = 0;
	m_nCharactersNumber = 0;
	if ( !lpszOrg ) return;

	BOOL bOrgIsUnicode = FALSE;
	if ( nOrgStringEncodeType == STRING_IS_MULTICHARS ) bOrgIsUnicode = FALSE;
	else if ( nOrgStringEncodeType == STRING_IS_UNICODE ) bOrgIsUnicode = TRUE;
	else
	{
#ifdef UNICODE
		bOrgIsUnicode = TRUE;
#else
		bOrgIsUnicode = FALSE;
#endif
	}

	if ( bOrgIsUnicode )
	{
		m_nCharactersNumber = (int)wcslen((WCHAR*)lpszOrg);
		m_nDataSize = (m_nCharactersNumber + 1) * sizeof(WCHAR);
	}
	else
	{
		m_nCharactersNumber = (int)strlen((char*)lpszOrg);
		m_nDataSize = (m_nCharactersNumber + 1) * sizeof(char);
	}

	if ( pOutBuf && nOutBufSize > 0 )
	{
		m_pszData = pOutBuf;
		m_nDataSize = nOutBufSize;
	}
	else
	{
		m_pszData = (char*)new BYTE[m_nDataSize];
		if ( !m_pszData )
		{
			::AfxThrowMemoryException ();
			return;
		}
		m_bNewBuffer = TRUE;
	}
	memset ( m_pszData, 0, m_nDataSize );

	if ( bOrgIsUnicode )
	{
		m_nCharactersNumber = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)lpszOrg, m_nCharactersNumber, (LPSTR)m_pszData, m_nDataSize / sizeof(char) - 1, NULL, NULL);
		if ( m_nCharactersNumber < 1 ) m_nCharactersNumber = (int)strlen ( m_pszData );
	}
	else
	{
		m_nCharactersNumber = __min ( m_nCharactersNumber, (int)(m_nDataSize/sizeof(char)-1) );
		strncpy_s(m_pszData, m_nCharactersNumber, (const char*)lpszOrg, m_nCharactersNumber);
		m_nCharactersNumber = (int)strlen ( m_pszData );
	}
	m_nDataSize = ( m_nCharactersNumber + 1 ) * sizeof(char);
}

CMultiByteString::~CMultiByteString ()
{
	if ( m_bNewBuffer && m_pszData )
	{
		delete[] m_pszData;
	}
}

CString GetCompatibleString ( LPVOID lpszOrg, BOOL bOrgIsUnicode, int nOrgLength/*=-1*/ )
{
	if ( !lpszOrg ) return _T("");

	TRY
	{
#ifdef UNICODE
		if ( bOrgIsUnicode )
		{
			if ( nOrgLength > 0 )
			{
				WCHAR *szRet = new WCHAR[nOrgLength+1];
				if ( !szRet ) return _T("");
				memset ( szRet, 0, (nOrgLength+1)*sizeof(WCHAR) );
				memcpy ( szRet, lpszOrg, nOrgLength*sizeof(WCHAR) );
				CString csRet = szRet;
				delete[] szRet;
				return csRet;
			}
			else if ( nOrgLength == 0 )
				return _T("");
			else
				return (LPCTSTR)lpszOrg;
		}

		if ( nOrgLength < 0 )
			nOrgLength = (int)strlen((const char*)lpszOrg);
		int nWideCount = nOrgLength + 1;
		WCHAR *wchar = new WCHAR[nWideCount];
		if ( !wchar ) return _T("");
		memset ( wchar, 0, nWideCount*sizeof(WCHAR) );
		::MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)lpszOrg, nOrgLength, wchar, nWideCount);
		CString csRet = wchar;
		delete[] wchar;
		return csRet;
#else
		if ( !bOrgIsUnicode )
		{
			if ( nOrgLength > 0 )
			{
				char *szRet = new char[nOrgLength+1];
				if ( !szRet ) return _T("");
				memset ( szRet, 0, (nOrgLength+1)*sizeof(char) );
				memcpy ( szRet, lpszOrg, nOrgLength*sizeof(char) );
				CString csRet = szRet;
				delete[] szRet;
				return csRet;
			}
			else if ( nOrgLength == 0 )
				return _T("");
			else
				return (LPCTSTR)lpszOrg;
		}

		if ( nOrgLength < 0 )
			nOrgLength = (int)wcslen((WCHAR*)lpszOrg);
		int nMultiByteCount = nOrgLength + 1;
		char *szMultiByte = new char[nMultiByteCount];
		if ( !szMultiByte ) return _T("");
		memset ( szMultiByte, 0, nMultiByteCount*sizeof(char) );
		::WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)lpszOrg, nOrgLength, (LPSTR)szMultiByte, nMultiByteCount, NULL, NULL);
		CString csRet = szMultiByte;
		delete[] szMultiByte;
		return csRet;
#endif
	}
	CATCH_ALL(e)
	{
		THROW_LAST ();
	}
	END_CATCH_ALL

	return _T("");
}

CString FormatDateTime (COleDateTime &DateTime, LPCTSTR /*pFormat*/)
{
	// If null, return empty string
	if ( DateTime.GetStatus() == COleDateTime::null || DateTime.GetStatus() == COleDateTime::invalid )
		return _T("");

	UDATE ud;
	if (S_OK != VarUdateFromDate(DateTime.m_dt, 0, &ud))
	{
		return _T("");
	}

	TCHAR *weeks[]={_T("Sun"),_T("Mon"),_T("Tue"),_T("Wen"),_T("Thu"),_T("Fri"),_T("Sat")};
	TCHAR *month[]={_T("Jan"),_T("Feb"),_T("Mar"),_T("Apr"),
					_T("May"),_T("Jun"),_T("Jul"),_T("Aug"),
					_T("Sep"),_T("Oct"),_T("Nov"),_T("Dec")};

	TIME_ZONE_INFORMATION stTimeZone;
	GetTimeZoneInformation(&stTimeZone);

	CString strDate;
	strDate.Format(_T("%s, %d %s %d %02d:%02d:%02d %c%04d")
		,weeks[ud.st.wDayOfWeek],
		ud.st.wDay,month[ud.st.wMonth-1],ud.st.wYear,ud.st.wHour,
		ud.st.wMinute,ud.st.wSecond,
		stTimeZone.Bias>0?_T('-'):_T('+'),
		abs(stTimeZone.Bias*10/6)
		);
	return strDate;
}

CString FormatString ( LPCTSTR lpszStr, ... )
{
	TCHAR *buf = NULL;
	for ( int nBufCount = 1024; nBufCount<5*1024*1024; nBufCount += 1024 )
	{
		buf = new TCHAR[nBufCount];
		if ( !buf )
		{
			::AfxThrowMemoryException ();
			return _T("");
		}
		memset ( buf, 0, nBufCount*sizeof(TCHAR) );

		va_list  va;
		va_start (va, lpszStr);
		int nLen = _vsnprintf_hw((TCHAR*)buf, nBufCount, _TRUNCATE, lpszStr, va);
		va_end(va);
		if ( nLen <= (int)(nBufCount-sizeof(TCHAR)) )
			break;
		delete[] buf; buf = NULL;
	}
	if ( !buf )
	{
		return _T("");
	}

	CString csMsg = buf;
	delete[] buf; buf = NULL;
	return csMsg;
}

int hwGetFileAttr ( LPCTSTR lpFileName, OUT CFileStatus *pFileStatus/*=NULL*/ )
{
	if ( !lpFileName || lstrlen(lpFileName) < 1 ) return -1;

	CFileStatus fileStatus;
	fileStatus.m_attribute = 0;
	fileStatus.m_size = 0;
	memset ( fileStatus.m_szFullName, 0, sizeof(fileStatus.m_szFullName) );
	BOOL bRet = FALSE;
	TRY
	{
		if ( CFile::GetStatus(lpFileName,fileStatus) )
		{
			bRet = TRUE;
		}
	}
	CATCH (CFileException, e)
	{
		ASSERT ( FALSE );
		bRet = FALSE;
	}
	CATCH_ALL(e)
	{
		ASSERT ( FALSE );
		bRet = FALSE;
	}
	END_CATCH_ALL;

	if ( pFileStatus )
	{
		pFileStatus->m_ctime = fileStatus.m_ctime;
		pFileStatus->m_mtime = fileStatus.m_mtime;
		pFileStatus->m_atime = fileStatus.m_atime;
		pFileStatus->m_size = fileStatus.m_size;
		pFileStatus->m_attribute = fileStatus.m_attribute;
		lstrcpy ( pFileStatus->m_szFullName, fileStatus.m_szFullName );

	}

	return (int)fileStatus.m_size;
}

CString FormatBytes ( double fBytesNum, BOOL bShowUnit/*=TRUE*/, int nFlag/*=0*/ )
{
	CString csRes;
	if ( nFlag == 0 )
	{
		if ( fBytesNum >= 1024.0 && fBytesNum < 1024.0*1024.0 )
			csRes.Format ( _T("%.2f%s"), fBytesNum / 1024.0, bShowUnit?_T(" K"):_T("") );
		else if ( fBytesNum >= 1024.0*1024.0 && fBytesNum < 1024.0*1024.0*1024.0 )
			csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0), bShowUnit?_T(" M"):_T("") );
		else if ( fBytesNum >= 1024.0*1024.0*1024.0 )
			csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0*1024.0), bShowUnit?_T(" G"):_T("") );
		else
			csRes.Format ( _T("%.2f%s"), fBytesNum, bShowUnit?_T(" B"):_T("") );
	}
	else if ( nFlag == 1 )
	{
		csRes.Format ( _T("%.2f%s"), fBytesNum / 1024.0, bShowUnit?_T(" K"):_T("") );
	}
	else if ( nFlag == 2 )
	{
		csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0), bShowUnit?_T(" M"):_T("") );
	}
	else if ( nFlag == 3 )
	{
		csRes.Format ( _T("%.2f%s"), fBytesNum / (1024.0*1024.0*1024.0), bShowUnit?_T(" G"):_T("") );
	}

	return csRes;
}
