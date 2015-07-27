/*
See the header file for details. This file is distributed under the MIT licence.
*/

#include "gsoapWinInet.h"

/* ensure that the wininet library is linked */
#pragma comment( lib, "wininet.lib" )
/* disable deprecation warnings */
#pragma warning(disable : 4996)

#define UNUSED_ARG(x)           (x)
#define INVALID_BUFFER_LENGTH  ((DWORD)-1)

/* plugin id */
static const char wininet_id[] = "wininet-2.0";

/* plugin private data */
struct wininet_data
{
    HINTERNET           hInternet;          /* internet session handle */
    HINTERNET           hConnection;        /* current connection handle */
    BOOL                bDisconnect;        /* connection is disconnected */
    BOOL                bKeepAlive;         /* keep connection alive */
    DWORD               dwRequestFlags;     /* extra request flags from user */
    char *              pBuffer;            /* send buffer */
    size_t              uiBufferLenMax;     /* total length of the message */
    size_t              uiBufferLen;        /* length of data in buffer */
    BOOL                bIsChunkSize;       /* expecting a chunk size buffer */
    wininet_rse_callback pRseCallback;      /* wininet_resolve_send_error callback.  Allows clients to resolve ssl errors programatically */
    wininet_progress_callback pProgressCallback; /* callback to report a progress */
    LPVOID              pProgressCallbackContext; /* context for pProgressCallback */
    char *              pRespHeaders;       /* buffer for response headers */
    size_t              uiRespHeadersLen;   /* length of data in response headers buffer */
    size_t              uiRespHeadersReaded;/* length of data readed from response headers buffer */
    char *              pszErrorMessage;    /* wininet/system error message */
};

#if 1
#define Trace(pszFunc, pBuffer, uiBufferLen)    \
    DBGLOG(TEST, SOAP_MESSAGE(fdebug, "wininet %p: %s 0x%p %d\n", soap, pszFunc, pBuffer, uiBufferLen));
#else
void Trace(const char* pszFunc, const void* pBuffer, DWORD uiBufferLen)
{
/*
    static bool newOne = true;
    FILE* f = fopen("C:\\log.log", newOne ? "wt" : "at");
    newOne = false;
    if (f)
    {
        fprintf(f, "\n============================================\n%s:\n", pszFunc);
        if (pBuffer)
            fwrite(pBuffer, uiBufferLen, 1, f);
        fclose(f);
    }
*/
}
#endif

/* forward declarations */
static BOOL
wininet_init(
    struct soap *           soap,
    struct wininet_data *   a_pData,
    DWORD                   a_dwRequestFlags );
static int
wininet_copy(
    struct soap *           soap,
    struct soap_plugin *    a_pDst,
    struct soap_plugin *    a_pSrc );
static void
wininet_delete(
    struct soap *           soap,
    struct soap_plugin *    a_pPluginData );
static SOAP_SOCKET
wininet_connect(
    struct soap *   soap,
    const char *    a_pszEndpoint,
    const char *    a_pszHost,
    int             a_nPort );
static int
wininet_post_header(
    struct soap *   soap,
    const char *    a_pszKey,
    const char *    a_pszValue );
static int
wininet_fsend(
    struct soap *   soap,
    const char *    a_pBuffer,
    size_t          a_uiBufferLen );
static size_t
wininet_frecv(
    struct soap *   soap,
    char *          a_pBuffer,
    size_t          a_uiBufferLen );
static int
wininet_disconnect(
    struct soap *   soap );
void CALLBACK
wininet_callback(
    HINTERNET   hInternet,
    DWORD_PTR   dwContext,
    DWORD       dwInternetStatus,
    LPVOID      lpvStatusInformation,
    DWORD       dwStatusInformationLength );
static BOOL
wininet_have_connection(
    struct soap *           soap,
    struct wininet_data *   a_pData );
static int
wininet_fpoll(
    struct soap *           soap );
static DWORD
wininet_set_timeout(
    struct soap *           soap,
    struct wininet_data *   a_pData,
    const char *            a_pszTimeout,
    DWORD                   a_dwOption,
    int                     a_nTimeout );
static BOOL
wininet_resolve_send_error(
    HINTERNET   a_hHttpRequest,
    DWORD       a_dwErrorCode );
static const char *
wininet_error_message(
    struct soap *   a_pData,
    DWORD           a_dwErrorMsgId );
static void
wininet_free_error_message(
    struct wininet_data *   a_pData );

/* plugin registration */
int
wininet_plugin(
    struct soap *           soap,
    struct soap_plugin *    a_pPluginData,
    void *                  a_dwRequestFlags )
{
    if ( !soap )
        return SOAP_FATAL_ERROR;

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: plugin registration\n", soap ));

    a_pPluginData->id        = wininet_id;
    a_pPluginData->fcopy     = wininet_copy;
    a_pPluginData->fdelete   = wininet_delete;
    a_pPluginData->data      = (void*) malloc( sizeof(struct wininet_data) );
    if ( !a_pPluginData->data )
    {
        return SOAP_EOM;
    }
    if ( !wininet_init( soap,
        (struct wininet_data *) a_pPluginData->data,
        (DWORD) (size_t) a_dwRequestFlags ) )
    {
        free( a_pPluginData->data );
        return SOAP_EOM;
    }

#ifdef SOAP_DEBUG
    if ( (soap->omode & SOAP_IO) == SOAP_IO_STORE )
    {
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: use of SOAP_IO_STORE is not recommended\n", soap ));
    }
#endif

    return SOAP_OK;
}

/* initialize private data */
static BOOL
wininet_init(
    struct soap *           soap,
    struct wininet_data *   a_pData,
    DWORD                   a_dwRequestFlags )
{
    if ( !soap )
        return SOAP_FATAL_ERROR;

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: init private data\n", soap ));

    memset( a_pData, 0, sizeof(struct wininet_data) );
    a_pData->dwRequestFlags = a_dwRequestFlags;

    Trace("InternetOpenA", nullptr, 0);

    /* start our internet session */
    a_pData->hInternet = InternetOpenA(
        "gSOAP", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0 );
    if ( !a_pData->hInternet )
    {
        soap->error = SOAP_EOF;
        soap->errnum = GetLastError();
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: init, error %d (%s) in InternetOpen\n",
            soap, soap->errnum, wininet_error_message(soap,soap->errnum) ));
        wininet_free_error_message( a_pData );
        return FALSE;
    }

    /* set the timeouts, if any of these fail the error isn't fatal */
    wininet_set_timeout( soap, a_pData, "connect",
        INTERNET_OPTION_CONNECT_TIMEOUT, soap->connect_timeout );
    wininet_set_timeout( soap, a_pData, "receive",
        INTERNET_OPTION_RECEIVE_TIMEOUT, soap->recv_timeout );
    wininet_set_timeout( soap, a_pData, "send",
        INTERNET_OPTION_SEND_TIMEOUT, soap->send_timeout );

    /* set up the callback function so we get notifications */
    InternetSetStatusCallback( a_pData->hInternet, wininet_callback );

    /* set all of our callbacks */
    soap->fopen    = wininet_connect;
    soap->fposthdr = wininet_post_header;
    soap->fsend    = wininet_fsend;
    soap->frecv    = wininet_frecv;
    soap->fclose   = wininet_disconnect;
    soap->fpoll    = wininet_fpoll;

    return TRUE;
}

void
wininet_set_rse_callback(
    struct soap *           soap,
    wininet_rse_callback    a_pRsecallback)
{
    struct wininet_data * pData = (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: resolve_send_error callback = '%p'\n", soap, a_pRsecallback ));

    pData->pRseCallback = a_pRsecallback;
}

void
wininet_set_progress_callback(
    struct soap *                soap,
    wininet_progress_callback    a_pProgressCallback,
    LPVOID a_pContext)
{
    struct wininet_data * pData = (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: progress callback = '%p', context = '%p'\n", soap, a_pProgressCallback, a_pContext ));

    pData->pProgressCallback = a_pProgressCallback;
    pData->pProgressCallbackContext = a_pContext;
}

/* copy the private data structure */
static int
wininet_copy(
    struct soap *           soap,
    struct soap_plugin *    a_pDst,
    struct soap_plugin *    a_pSrc )
{
    UNUSED_ARG( soap );
    UNUSED_ARG( a_pDst );
    UNUSED_ARG( a_pSrc );

    _ASSERTE( !"wininet doesn't support copy" );
    return SOAP_FATAL_ERROR;
}

/* deallocate of our private structure */
static void
wininet_delete(
    struct soap *           soap,
    struct soap_plugin *    a_pPluginData )
{
    struct wininet_data * pData =
        (struct wininet_data *) a_pPluginData->data;

    UNUSED_ARG( soap );

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: delete private data\n", soap ));

    /* force a disconnect of any existing connection */
    pData->bDisconnect = TRUE;
    wininet_have_connection( soap, pData );

    /* close down the internet */
    if ( pData->hInternet )
    {
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: closing internet handle\n", soap));
        InternetCloseHandle( pData->hInternet );
        pData->hInternet = NULL;
    }

    /* free our data */
    wininet_free_error_message( pData );
    free( a_pPluginData->data );
}

/* gsoap documentation:
    Called from a client proxy to open a connection to a Web Service located
    at endpoint. Input parameters host and port are micro-parsed from endpoint.
    Should return a valid file descriptor, or SOAP_INVALID_SOCKET and
    soap->error set to an error code. Built-in gSOAP function: tcp_connect
*/
static SOAP_SOCKET
wininet_connect(
    struct soap *   soap,
    const char *    a_pszEndpoint,
    const char *    a_pszHost,
    int             a_nPort )
{
    URL_COMPONENTSA urlComponents;
    char            szUrlPath[MAX_PATH] = {0};
    char            szHost[MAX_PATH] = {0};
    DWORD           dwFlags;
    HINTERNET       hConnection  = NULL;
    HINTERNET       hHttpRequest = NULL;
    const char *    pszVerb;
    INTERNET_PORT   nPort;
    struct wininet_data * pData =
        (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    soap->error = SOAP_OK;

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: connect, endpoint = '%s'\n", soap, a_pszEndpoint ));

    /* we should be initialized but not connected */
    _ASSERTE( pData->hInternet );
    if ( !pData->hInternet )
        return SOAP_FATAL_ERROR;
    _ASSERTE( !pData->bKeepAlive && !pData->hConnection || pData->bKeepAlive && pData->hConnection );
    if ( !(!pData->bKeepAlive && !pData->hConnection || pData->bKeepAlive && pData->hConnection) )
        return SOAP_FATAL_ERROR;
    _ASSERTE( soap->socket == SOAP_INVALID_SOCKET );
    if ( soap->socket != SOAP_INVALID_SOCKET )
        return SOAP_FATAL_ERROR;

    /* parse out the url path */
    memset( &urlComponents, 0, sizeof(urlComponents) );
    urlComponents.dwStructSize = sizeof(urlComponents);
    urlComponents.lpszHostName      = szHost;
    urlComponents.dwHostNameLength  = MAX_PATH;
    urlComponents.lpszUrlPath       = szUrlPath;
    urlComponents.dwUrlPathLength   = MAX_PATH;
    if ( !InternetCrackUrlA( a_pszEndpoint, 0, 0, &urlComponents ) )
    {
        soap->error = SOAP_TCP_ERROR;
        soap->errnum = GetLastError();
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: connect, error %d (%s) in InternetCrackUrl\n",
            soap, soap->errnum, wininet_error_message(soap,soap->errnum) ));
        return SOAP_INVALID_SOCKET;
    }

    nPort = urlComponents.nPort;

    if ( !pData->hConnection )
    {
        Trace("InternetConnectA", szHost, (DWORD)strlen(szHost));

        /* connect to the target url, if we haven't connected yet
           or if it was dropped */
        hConnection = InternetConnectA( pData->hInternet,
            szHost, nPort, "", "", INTERNET_SERVICE_HTTP,
            0, (DWORD_PTR) soap );
        if ( !hConnection )
        {
            soap->error = SOAP_TCP_ERROR;
            soap->errnum = GetLastError();
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: connect, error %d (%s) in InternetConnect\n",
                soap, soap->errnum, wininet_error_message(soap,soap->errnum) ));
            return SOAP_INVALID_SOCKET;
        }
    }
    else
    {
        hConnection = pData->hConnection;
        pData->hConnection = NULL;
    }

    /*
        Note that although we specify HTTP/1.1 for the connection here, the
        actual connection may be HTTP/1.0 depending on the settings in the
        control panel. See the "Internet Options", "HTTP 1.1 settings".
     */
    dwFlags = pData->dwRequestFlags | INTERNET_FLAG_NO_CACHE_WRITE;
    if ( soap->omode & SOAP_IO_KEEPALIVE )
    {
        dwFlags |= INTERNET_FLAG_KEEP_CONNECTION;
        pData->bKeepAlive = TRUE;
    }
    if ( urlComponents.nScheme == INTERNET_SCHEME_HTTPS )
    {
        dwFlags |= INTERNET_FLAG_SECURE;
    }

    /* proxy requires full endpoint URL */
    if ( soap->proxy_host )
    {
        strncpy(szUrlPath, a_pszEndpoint, MAX_PATH);
    }

    /* status determines the HTTP verb */
    switch ( soap->status )
    {
      case SOAP_GET:
          pszVerb = "GET";
          break;
      case SOAP_PUT:
          pszVerb = "PUT";
          break;
      case SOAP_DEL:
          pszVerb = "DELETE";
          break;
      case SOAP_CONNECT:
          pszVerb = "CONNECT";
          _snprintf(szUrlPath, MAX_PATH, "%s:%d", a_pszHost, a_nPort);
          break;
      default:
          pszVerb = "POST";
    }

    Trace("HttpOpenRequestA", szUrlPath, (DWORD)strlen(szUrlPath));

    hHttpRequest = HttpOpenRequestA(
        hConnection, pszVerb, szUrlPath, "HTTP/1.1", NULL, NULL,
        dwFlags, (DWORD_PTR) soap );
    if ( !hHttpRequest )
    {
        InternetCloseHandle( hConnection );
        pData->bKeepAlive = FALSE;
        soap->error = SOAP_HTTP_ERROR;
        soap->errnum = GetLastError();
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: connect, error %d (%s) in HttpOpenRequest\n",
            soap, soap->errnum, wininet_error_message(soap,soap->errnum) ));
        wininet_free_error_message(pData);
        return SOAP_INVALID_SOCKET;
    }

    /* save the connection handle in our data structure */
    pData->hConnection = hConnection;

    /* return the http request handle as our file descriptor. */
    _ASSERTE( sizeof(soap->socket) >= sizeof(HINTERNET) );
    return (SOAP_SOCKET) hHttpRequest;
}

/* gsoap documentation:
    Called by http_post and http_response (through the callbacks). Emits HTTP
    key: val header entries. Should return SOAP_OK, or a gSOAP error code.
    Built-in gSOAP function: http_post_header.
 */
static int
wininet_post_header(
    struct soap *   soap,
    const char *    a_pszKey,
    const char *    a_pszValue )
{
    HINTERNET hHttpRequest = (HINTERNET) soap->socket;
    char      szHeader[4096];
    int       nLen;
    BOOL      bResult = FALSE;
    struct wininet_data * pData =
        (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    soap->error = SOAP_OK;

    /* ensure that our connection hasn't been disconnected */
    if ( !wininet_have_connection( soap, pData ) )
    {
        return SOAP_EOF;
    }

    /* if this is the initial POST header then we initialize our send buffer */
    if ( a_pszKey && !a_pszValue )
    {
        _ASSERTE( !pData->pBuffer );
        pData->uiBufferLenMax = INVALID_BUFFER_LENGTH;
        pData->uiBufferLen    = 0;

        /* if we are using chunk output then we start with a chunk size */
        pData->bIsChunkSize = ( (soap->omode & SOAP_IO) == SOAP_IO_CHUNK );
    }
    else if ( a_pszValue )
    {
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: post_header, adding '%s: %s'\n",
            soap, a_pszKey, a_pszValue ));

        /* determine the maximum length of this message so that we can
           correctly determine when we have completed the send */
        if ( !strcmp( a_pszKey, "Content-Length" ) )
        {
            _ASSERTE( pData->uiBufferLenMax == INVALID_BUFFER_LENGTH );
            pData->uiBufferLenMax = strtoul( a_pszValue, NULL, 10 );
        }

        nLen = _snprintf(
            szHeader, 4096, "%s: %s\r\n", a_pszKey, a_pszValue );
        if ( nLen < 0 )
        {
            return SOAP_EOM;
        }

        Trace("HttpAddRequestHeadersA", szHeader, nLen);

        bResult = HttpAddRequestHeadersA( hHttpRequest, szHeader, nLen,
            HTTP_ADDREQ_FLAG_ADD_IF_NEW );
#ifdef SOAP_DEBUG
        /*
            we don't return an error if this fails because it isn't
            (or shouldn't be) critical.
         */
        if ( !bResult )
        {
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: post_header, error %d (%s) in HttpAddRequestHeaders\n",
                soap, GetLastError(), wininet_error_message(soap,GetLastError()) ));
        }
#endif
    }
    return SOAP_OK;
}

/* gsoap documentation:
    Called for all send operations to emit contents of s of length n.
    Should return SOAP_OK, or a gSOAP error code. Built-in gSOAP
    function: fsend

   Notes:
    I do a heap of buffering here because we need the entire message available
    in a single buffer in order to iterate through the sending loop. I had
    hoped that the SOAP_IO_STORE flag would have worked to do the same, however
    this still breaks the messages up into blocks. Although there were a number
    of ways this could've been implemented, this works and supports all of the
    possible SOAP_IO flags, even though the entire message is still buffered
    the same as if SOAP_IO_STORE was used.
*/
static int
wininet_fsend(
    struct soap *   soap,
    const char *    a_pBuffer,
    size_t          a_uiBufferLen )
{
    HINTERNET   hHttpRequest = (HINTERNET) soap->socket;
    BOOL        bResult;
    BOOL        bRetryPost;
    DWORD       dwStatusCode;
    DWORD       dwStatusCodeLen;
    int         nResult = SOAP_OK;
    struct wininet_data * pData =
        (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: fsend, data len = %lu bytes\n", soap, a_uiBufferLen ));

    /* allow the request to be sent with a NULL buffer */
    if (a_uiBufferLen == 0)
    {
        pData->uiBufferLenMax = 0;
    }

    /* ensure that our connection hasn't been disconnected */
    if ( !wininet_have_connection( soap, pData ) )
    {
        return SOAP_EOF;
    }

    /* initialize on our first time through. pData->pBuffer will always be
       non-null if this is not the first call. */
    if ( !pData->pBuffer )
    {
        /*
            If we are using chunked sending, then we don't know how big the
            buffer will need to be. So we start with a 0 length buffer and
            grow it later to ensure that it is always large enough.

                uiBufferLenMax = length of the allocated memory
                uiBufferLen    = length of the data in the buffer
         */
        if ( (soap->mode & SOAP_IO) == SOAP_IO_CHUNK )
        {
            /* we make the initial allocation large enough for this chunksize
               buffer, plus the next chunk of actual data, and a few extra
               bytes for the final "0" chunksize block. */
            size_t uiChunkSize = strtoul( a_pBuffer, NULL, 16 );
            pData->uiBufferLenMax = uiChunkSize + a_uiBufferLen + 16;
        }
        else if ( a_uiBufferLen == pData->uiBufferLenMax )
        {
            /*
                If the currently supplied buffer from gsoap holds the entire
                message then we just use their buffer and avoid any memory
                allocation. This will only be true when (1) we are not using
                chunked send (so uiBufferLenMax has been previously set to
                the Content-Length header length), and (2) gsoap is sending
                the entire message at one time.
             */
            pData->pBuffer     = (char *) a_pBuffer;
            pData->uiBufferLen = a_uiBufferLen;
        }

        _ASSERTE( pData->uiBufferLenMax != INVALID_BUFFER_LENGTH );
    }

    /*
        If we can't use the gsoap buffer, then we need to allocate our own
        buffer for the entire message. This is because authentication may
        require the entire message to be sent multiple times. Since this send
        is only a part of the message, we need to buffer until we have the
        entire message.
    */
    if ( pData->pBuffer != a_pBuffer )
    {
        /*
            We already have a buffer pointer, this means that it isn't the
            first time we have been called. We have allocated a buffer and
            are current filling it.

            If we don't have enough room in the our buffer to add this new
            data, then we need to reallocate. This case will only occur with
            chunked sends.
         */
        size_t uiNewBufferLen = pData->uiBufferLen + a_uiBufferLen;
        if ( !pData->pBuffer || uiNewBufferLen > pData->uiBufferLenMax )
        {
            while ( uiNewBufferLen > pData->uiBufferLenMax )
            {
                pData->uiBufferLenMax = pData->uiBufferLenMax * 2;
            }
            char *pReallocatedBuffer = (char *) realloc( pData->pBuffer, pData->uiBufferLenMax );
            if ( !pReallocatedBuffer )
            {
                return SOAP_EOM;
            }
            pData->pBuffer = pReallocatedBuffer;
        }
        memcpy( pData->pBuffer + pData->uiBufferLen,
            a_pBuffer, a_uiBufferLen );
        pData->uiBufferLen = uiNewBufferLen;

        /* if we are doing chunked transfers, and this is a chunk size block,
           and it is "0", then this is the last block in the transfer and we
           can set the maximum size now to continue to the actual send. */
        if ( (soap->mode & SOAP_IO) == SOAP_IO_CHUNK
             && pData->bIsChunkSize
             && a_pBuffer[2] == '0' && !isalnum((unsigned char)a_pBuffer[3]) )
        {
            pData->uiBufferLenMax = pData->uiBufferLen;
        }
    }

    /* if we haven't got the entire length of the message yet, then
       we return to gsoap and let it continue */
    if ( pData->uiBufferLen < pData->uiBufferLenMax )
    {
        /* toggle our chunk size marker if we are chunking */
        pData->bIsChunkSize =
            ((soap->mode & SOAP_IO) == SOAP_IO_CHUNK)
            && !pData->bIsChunkSize;
        return SOAP_OK;
    }
    _ASSERTE( pData->uiBufferLen == pData->uiBufferLenMax );

    /* we've now got the entire message, now we can enter our sending loop */
    bRetryPost = TRUE;
    while ( bRetryPost )
    {
        bRetryPost = FALSE;

        soap->error = SOAP_OK;

        /* to report a progress when sending a big buffer we need to send it by small
           chunks since INTERNET_STATUS_REQUEST_SENT callback is sent for entire buffer */
        const DWORD dwChunkSize = 64 * 1024;
        if ( pData->pProgressCallback && (DWORD)pData->uiBufferLen > dwChunkSize )
        {
            INTERNET_BUFFERSA buffer;
            memset(&buffer, 0, sizeof(buffer));
            buffer.dwStructSize = sizeof(buffer);
            buffer.lpvBuffer = pData->pBuffer;
            buffer.dwBufferLength = min((DWORD)pData->uiBufferLen, dwChunkSize);
            buffer.dwBufferTotal = (DWORD)pData->uiBufferLen;
            Trace("HttpSendRequestExA", pData->pBuffer, buffer.dwBufferLength);
            bResult = HttpSendRequestExA( hHttpRequest, &buffer, NULL, 0, 0 );
            while ( bResult && buffer.dwBufferLength < buffer.dwBufferTotal )
            {
                DWORD bytesWritten = min(buffer.dwBufferTotal - buffer.dwBufferLength, dwChunkSize);
                Trace("InternetWriteFile", pData->pBuffer + buffer.dwBufferLength, bytesWritten);
                bResult = InternetWriteFile( hHttpRequest, pData->pBuffer + buffer.dwBufferLength, bytesWritten, &bytesWritten );
                buffer.dwBufferLength += bytesWritten;
            }
            if ( bResult )
            {
                bResult = HttpEndRequestA( hHttpRequest, NULL, 0, 0 );
            }
        }
        else
        {
            Trace("HttpSendRequestA", pData->pBuffer, (DWORD)pData->uiBufferLen);

            bResult = HttpSendRequestA(
                hHttpRequest, NULL, 0, pData->pBuffer, (DWORD)pData->uiBufferLen );
        }
        if ( !bResult )
        {
            soap->error = SOAP_EOF;
            soap->errnum = GetLastError();
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: fsend, error %d (%s) in HttpSendRequest\n",
                soap, soap->errnum, wininet_error_message(soap,soap->errnum) ));

            /* see if we can handle this error, see the MSDN documentation
               for InternetErrorDlg for details */
            switch ( soap->errnum )
            {
            case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
            case ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR:
            case ERROR_INTERNET_INCORRECT_PASSWORD:
            case ERROR_INTERNET_INVALID_CA:
            case ERROR_INTERNET_POST_IS_NON_SECURE:
            case ERROR_INTERNET_SEC_CERT_CN_INVALID:
            case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
            case ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED:
                {
                    wininet_rseReturn errorResolved = rseDisplayDlg;
                    if (pData->pRseCallback)
                    {
                        errorResolved = pData->pRseCallback(hHttpRequest, soap->errnum);
                    }
                    if (errorResolved == rseDisplayDlg)
                    {
                        errorResolved = (wininet_rseReturn)
                            wininet_resolve_send_error( hHttpRequest, soap->errnum );
                        if ( errorResolved == rseTrue )
                        {
                            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                                "wininet %p: fsend, error %d has been resolved\n",
                                soap, soap->errnum ));
                            bRetryPost = TRUE;

                            /*
                                we would have been disconnected by the error. Since we
                                are going to try again, we will automatically be
                                reconnected. Therefore we want to disregard any
                                previous disconnection messages.
                            */
                            pData->bDisconnect = FALSE;
                            continue;
                        }
                    }
                }
                break;
            }

            /* if the error wasn't handled then we exit */
            switch ( soap->errnum )
            {
            case ERROR_INTERNET_NAME_NOT_RESOLVED:
            case ERROR_INTERNET_CANNOT_CONNECT:
                nResult = SOAP_TCP_ERROR;
                break;
            default:
                nResult = SOAP_HTTP_ERROR;
                break;
            }
            soap_set_sender_error(soap, "HttpSendRequest failed", wininet_error_message(soap,soap->errnum), nResult);
            break;
        }

        /* get the status code from the response to determine if we need
           to authorize */
        dwStatusCodeLen = sizeof(dwStatusCode);
        Trace("HttpQueryInfo - wininet_fsend", nullptr, 0);
        bResult = HttpQueryInfo(
            hHttpRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
            &dwStatusCode, &dwStatusCodeLen, NULL);

        char buf[1024];dwStatusCodeLen = sizeof(buf);
        bResult = HttpQueryInfo(
            hHttpRequest, HTTP_QUERY_STATUS_TEXT,
            buf, &dwStatusCodeLen, NULL);

        if ( !bResult )
        {
            soap->error = SOAP_EOF;
            soap->errnum = GetLastError();
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: fsend, error %d (%s) in HttpQueryInfo\n",
                soap, soap->errnum, wininet_error_message(soap,soap->errnum) ));
            nResult = SOAP_HTTP_ERROR;
            break;
        }

        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: fsend, HTTP status code = %lu\n",
            soap, dwStatusCode));

        /*
            if we need authentication, then request the user for the
            appropriate data. Their reply is saved into the request so
            that we can use it later.
         */
        switch ( dwStatusCode )
        {
        case HTTP_STATUS_DENIED:
        case HTTP_STATUS_PROXY_AUTH_REQ:
            {
            wininet_rseReturn errorResolved = rseDisplayDlg;
                DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                    "wininet %p: fsend, user authenication required\n",
                    soap ));
            if (pData->pRseCallback)
            {
                errorResolved = pData->pRseCallback(hHttpRequest, dwStatusCode);
            }
            if (errorResolved == rseDisplayDlg)
            {
                errorResolved = (wininet_rseReturn)
                    wininet_resolve_send_error( hHttpRequest, ERROR_INTERNET_INCORRECT_PASSWORD );
            }
            if ( errorResolved == rseTrue )
            {
                DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                    "wininet %p: fsend, authentication has been provided\n",
                    soap ));

                /*
                    we may have been disconnected by the error. Since we
                    are going to try again, we will automatically be
                    reconnected. Therefore we want to disregard any previous
                    disconnection messages.
                    */
                pData->bDisconnect = FALSE;
                bRetryPost = TRUE;
                continue;
            }
            }
            break;
        }
    }

    /* if we have an allocated buffer then we can deallocate it now */
    if ( pData->pBuffer != a_pBuffer )
    {
        free( pData->pBuffer );
    }

    pData->pBuffer     = 0;
    pData->uiBufferLen = 0;
    pData->uiBufferLenMax = INVALID_BUFFER_LENGTH;

    pData->pRespHeaders = 0;

    return nResult;
}

/* gsoap documentation:
    Called for all receive operations to fill buffer s of maximum length n.
    Should return the number of bytes read or 0 in case of an error, e.g. EOF.
    Built-in gSOAP function: frecv
 */
static size_t
wininet_frecv(
    struct soap *   soap,
    char *          a_pBuffer,
    size_t          a_uiBufferLen )
{
    HINTERNET   hHttpRequest = (HINTERNET) soap->socket;
    DWORD       dwBytesRead = 0;
    size_t      uiTotalBytesRead = 0;
    BOOL        bResult;

    struct wininet_data * pData =
        (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    if (!pData->pRespHeaders)
    {
        DWORD size = 0;

retry:
        // This call will fail on the first pass, because no buffer is allocated.
        if (!HttpQueryInfo(hHttpRequest, HTTP_QUERY_RAW_HEADERS_CRLF,
            (LPVOID)pData->pRespHeaders, &size, NULL))
        {
            // Check for an insufficient buffer.
            if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
            {
                // Allocate the necessary buffer.
                pData->pRespHeaders = (char*) malloc(size);

                // Retry the call.
                goto retry;
            }
            else
            {
                pData->pRespHeaders = (char*) malloc(0);
                pData->uiRespHeadersLen = 0;
                pData->uiRespHeadersReaded = 0;
                Trace("HttpQueryInfo - no headers", nullptr, 0);
            }
        }
        else
        {
            pData->uiRespHeadersLen = size;
            pData->uiRespHeadersReaded = 0;
            Trace("HttpQueryInfo - reading headers", pData->pRespHeaders, static_cast<DWORD>(pData->uiRespHeadersLen));
        }
    }

    if (pData->pRespHeaders && pData->uiRespHeadersReaded < pData->uiRespHeadersLen)
    {
        size_t notReceivedPart = pData->uiRespHeadersLen - pData->uiRespHeadersReaded;
        size_t recvSize = min(a_uiBufferLen, notReceivedPart);

        memcpy(a_pBuffer, pData->pRespHeaders + pData->uiRespHeadersReaded, recvSize);
        pData->uiRespHeadersReaded += recvSize;
        if (a_uiBufferLen == recvSize)
            return a_uiBufferLen;
        uiTotalBytesRead = recvSize;
    }

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: frecv, available buffer len = %lu\n",
        soap, a_uiBufferLen ));

    /*
        NOTE: we do not check here that our connection hasn't been
        disconnected because in HTTP/1.0 connections, it will always have been
        disconnected by now. This is because the response is checked by the
        wininet_fsend function to ensure that we didn't need any special
        authentication. At that time the connection would have been
        disconnected. This is okay however as we can still read the response
        from the request handle.
     */

    do
    {
        /* read from the connection up to our maximum amount of data */
        _ASSERTE( a_uiBufferLen <= ULONG_MAX );
        bResult = InternetReadFile(
            hHttpRequest,
            &a_pBuffer[uiTotalBytesRead],
            static_cast<DWORD>(a_uiBufferLen - uiTotalBytesRead),
            &dwBytesRead );
        Trace("InternetReadFile", &a_pBuffer[uiTotalBytesRead], dwBytesRead);
        if ( bResult )
        {
            uiTotalBytesRead += dwBytesRead;
            if ( dwBytesRead == 0 )
            {
                if (pData->pRespHeaders)
                {
                    free(pData->pRespHeaders);
                    pData->pRespHeaders = 0;
                }
                pData->bDisconnect = TRUE;
            }
        }
        else
        {
            soap->errnum = GetLastError();
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: frecv, error %d (%s) in InternetReadFile\n",
                soap, soap->errnum, wininet_error_message(soap,soap->errnum) ));
            soap_set_sender_error(soap, "InternetReadFile failed", wininet_error_message(soap,soap->errnum), SOAP_HTTP_ERROR);
        }
    }
    while ( bResult && dwBytesRead && uiTotalBytesRead < a_uiBufferLen );

    DBGLOG(TEST, SOAP_MESSAGE(fdebug,
        "wininet %p: recv, received %lu bytes\n", soap, uiTotalBytesRead ));

    return uiTotalBytesRead;
}

/* gsoap documentation:
    Called by client proxy multiple times, to close a socket connection before
    a new socket connection is established and at the end of communications
    when the SOAP_IO_KEEPALIVE flag is not set and soap.keep_alive = 0
    (indicating that the other party supports keep alive). Should return
    SOAP_OK, or a gSOAP error code. Built-in gSOAP function: tcp_disconnect
 */
static int
wininet_disconnect(
    struct soap *   soap )
{
    struct wininet_data * pData =
        (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    DBGLOG(TEST, SOAP_MESSAGE(fdebug, "wininet %p: disconnect\n", soap ));

    /* force a disconnect by setting the disconnect flag to TRUE */
    pData->bDisconnect = TRUE;
    pData->bKeepAlive = FALSE;
    wininet_have_connection( soap, pData );

    return soap->error = SOAP_OK;
}

/* this is mostly for debug tracing */
void CALLBACK
wininet_callback(
    HINTERNET   hInternet,
    DWORD_PTR   dwContext,
    DWORD       dwInternetStatus,
    LPVOID      lpvStatusInformation,
    DWORD       dwStatusInformationLength )
{
    struct soap * soap = (struct soap *) dwContext;

    UNUSED_ARG( hInternet );
    UNUSED_ARG( lpvStatusInformation );
    UNUSED_ARG( dwStatusInformationLength );

    switch ( dwInternetStatus )
    {
    case INTERNET_STATUS_RESOLVING_NAME:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_RESOLVING_NAME\n", soap ));
        break;
    case INTERNET_STATUS_NAME_RESOLVED:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_NAME_RESOLVED\n", soap ));
        break;
    case INTERNET_STATUS_CONNECTING_TO_SERVER:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_CONNECTING_TO_SERVER\n", soap));
        break;
    case INTERNET_STATUS_CONNECTED_TO_SERVER:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_CONNECTED_TO_SERVER\n", soap));
        break;
    case INTERNET_STATUS_SENDING_REQUEST:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_SENDING_REQUEST\n", soap));
        break;
    case INTERNET_STATUS_REQUEST_SENT:
        {
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: INTERNET_STATUS_REQUEST_SENT, bytes sent = %lu\n",
                soap, *(DWORD *)lpvStatusInformation ));
            struct wininet_data * pData =
                (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );
            if ( pData->pProgressCallback )
            {
                pData->pProgressCallback( TRUE, *(DWORD *)lpvStatusInformation, pData->pProgressCallbackContext );
            }
        }
        break;
    case INTERNET_STATUS_RECEIVING_RESPONSE:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_RECEIVING_RESPONSE\n", soap));
        break;
    case INTERNET_STATUS_RESPONSE_RECEIVED:
        {
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: INTERNET_STATUS_RESPONSE_RECEIVED, bytes received = %lu\n",
                soap, *(DWORD *)lpvStatusInformation ));
            struct wininet_data * pData =
                (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );
            if ( pData->pProgressCallback )
            {
                pData->pProgressCallback( FALSE, *(DWORD *)lpvStatusInformation, pData->pProgressCallbackContext );
            }
        }
        break;
    case INTERNET_STATUS_CTL_RESPONSE_RECEIVED:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_CTL_RESPONSE_RECEIVED\n", soap));
        break;
    case INTERNET_STATUS_PREFETCH:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_PREFETCH\n", soap));
        break;
    case INTERNET_STATUS_CLOSING_CONNECTION:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_CLOSING_CONNECTION\n", soap));
        break;
    case INTERNET_STATUS_CONNECTION_CLOSED:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_CONNECTION_CLOSED\n", soap));
        {
            /* the connection has been closed, so we close the handle here */
            struct wininet_data * pData =
                (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );
            if ( pData->hConnection )
            {
                /*
                    we only mark this for disconnection otherwise we get
                    errors when reading the data from the handle. In every
                    function that we use the connection we will check first to
                    see if it has been disconnected.
                 */
                pData->bDisconnect = TRUE;
            }
        }
        break;
    case INTERNET_STATUS_HANDLE_CREATED:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_HANDLE_CREATED\n", soap));
        break;
    case INTERNET_STATUS_HANDLE_CLOSING:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_HANDLE_CLOSING\n", soap));
        break;
#ifdef INTERNET_STATUS_DETECTING_PROXY
    case INTERNET_STATUS_DETECTING_PROXY:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_DETECTING_PROXY\n", soap));
        break;
#endif
    case INTERNET_STATUS_REQUEST_COMPLETE:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_REQUEST_COMPLETE\n", soap));
        break;
    case INTERNET_STATUS_REDIRECT:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_REDIRECT, new url = %s\n",
            soap, (char*) lpvStatusInformation ));
        break;
    case INTERNET_STATUS_INTERMEDIATE_RESPONSE:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_INTERMEDIATE_RESPONSE\n", soap));
        break;
#ifdef INTERNET_STATUS_USER_INPUT_REQUIRED
    case INTERNET_STATUS_USER_INPUT_REQUIRED:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_USER_INPUT_REQUIRED\n", soap));
        break;
#endif
    case INTERNET_STATUS_STATE_CHANGE:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_STATE_CHANGE\n", soap));
        break;
#ifdef INTERNET_STATUS_COOKIE_SENT
    case INTERNET_STATUS_COOKIE_SENT:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_COOKIE_SENT\n", soap));
        break;
#endif
#ifdef INTERNET_STATUS_COOKIE_RECEIVED
    case INTERNET_STATUS_COOKIE_RECEIVED:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_COOKIE_RECEIVED\n", soap));
        break;
#endif
#ifdef INTERNET_STATUS_PRIVACY_IMPACTED
    case INTERNET_STATUS_PRIVACY_IMPACTED:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_PRIVACY_IMPACTED\n", soap));
        break;
#endif
#ifdef INTERNET_STATUS_P3P_HEADER
    case INTERNET_STATUS_P3P_HEADER:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_P3P_HEADER\n", soap));
        break;
#endif
#ifdef INTERNET_STATUS_P3P_POLICYREF
    case INTERNET_STATUS_P3P_POLICYREF:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_P3P_POLICYREF\n", soap));
        break;
#endif
#ifdef INTERNET_STATUS_COOKIE_HISTORY
    case INTERNET_STATUS_COOKIE_HISTORY:
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: INTERNET_STATUS_COOKIE_HISTORY\n", soap));
        break;
#endif
    }
}

/*
    check to ensure that our connection hasn't been disconnected
    and disconnect remaining handles if necessary.
 */
static BOOL
wininet_have_connection(
    struct soap *           soap,
    struct wininet_data *   a_pData )
{
    /* close the http request if we don't have a connection */
    BOOL bCloseRequest = a_pData->bDisconnect || !a_pData->hConnection;
    if ( bCloseRequest && soap->socket != SOAP_INVALID_SOCKET )
    {
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: closing request\n", soap));

        Trace("InternetCloseHandle (closing request)", nullptr, 0);
        InternetCloseHandle( (HINTERNET) soap->socket );
        soap->socket = SOAP_INVALID_SOCKET;

#if 0
        if ( a_pData->bKeepAlive )
        {
            DWORD           dwFlags;
            HINTERNET       hHttpRequest = NULL;
            dwFlags = a_pData->dwRequestFlags | INTERNET_FLAG_NO_CACHE_WRITE;
            if ( soap->omode & SOAP_IO_KEEPALIVE )
            {
                dwFlags |= INTERNET_FLAG_KEEP_CONNECTION;
                a_pData->bKeepAlive = TRUE;
            }
            /*if ( urlComponents.nScheme == INTERNET_SCHEME_HTTPS )
            {
                dwFlags |= INTERNET_FLAG_SECURE;
            }*/

            hHttpRequest = HttpOpenRequestA(
                a_pData->hConnection, "POST", "/MyWebService/Service.asmx"/*szUrlPath*/, "HTTP/1.1", NULL, NULL,
                dwFlags, (DWORD_PTR) soap );
            if ( hHttpRequest )
                soap->socket = (SOAP_SOCKET) hHttpRequest;
            else
                a_pData->bKeepAlive = FALSE;
        }
#endif
    }

    /* close the connection if we don't have a request */
    if ( soap->socket == SOAP_INVALID_SOCKET && a_pData->hConnection && !a_pData->bKeepAlive )
    {
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: closing connection\n", soap));

        Trace("InternetCloseHandle (closing connection)", nullptr, 0);
        InternetCloseHandle( a_pData->hConnection );
        a_pData->hConnection = NULL;
    }
    a_pData->bDisconnect = FALSE;

    /* clean up the send details if we don't have a request */
    if ( soap->socket == SOAP_INVALID_SOCKET )
    {
        if ( a_pData->pBuffer )
        {
            free( a_pData->pBuffer );
            a_pData->pBuffer = 0;
        }
        a_pData->uiBufferLen = 0;
        a_pData->uiBufferLenMax = INVALID_BUFFER_LENGTH;
    }

    return (soap->socket != SOAP_INVALID_SOCKET);
}

static int
wininet_fpoll(
    struct soap *           soap)
{
    struct wininet_data * pData =
        (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );
    return wininet_have_connection(soap, pData) ? SOAP_OK : SOAP_EOF;
}

static DWORD
wininet_set_timeout(
    struct soap *           soap,
    struct wininet_data *   a_pData,
    const char *            a_pszTimeout,
    DWORD                   a_dwOption,
    int                     a_nTimeout )
{
    UNUSED_ARG( soap );
    UNUSED_ARG( a_pszTimeout );

    if ( a_nTimeout > 0 )
    {
        DWORD dwTimeout = a_nTimeout * 1000;
        if ( !InternetSetOption( a_pData->hInternet,
            a_dwOption, &dwTimeout, sizeof(DWORD) ) )
        {
            DWORD dwErrorCode = GetLastError();
            DBGLOG(TEST, SOAP_MESSAGE(fdebug,
                "wininet %p: failed to set %s timeout, error %d (%s)\n",
                soap, a_pszTimeout, dwErrorCode,
                wininet_error_message(soap,dwErrorCode) ));
            return dwErrorCode;
        }
    }
    return 0;
}

#if 0
static BOOL
wininet_flag_set_option(
    HINTERNET   a_hHttpRequest,
    DWORD       a_dwOption,
    DWORD       a_dwFlag )
{
    DWORD dwBuffer;
    DWORD dwBufferLength = sizeof(DWORD);

    BOOL bSuccess = InternetQueryOption(
        a_hHttpRequest,
        a_dwOption,
        &dwBuffer,
        &dwBufferLength );
    if (bSuccess) {
        dwBuffer |= a_dwFlag;
        bSuccess = InternetSetOption(
            a_hHttpRequest,
            a_dwOption,
            &dwBuffer,
            dwBufferLength);
    }
#ifdef SOAP_DEBUG
    if ( !bSuccess ) {
        DWORD dwErrorCode = GetLastError();
        DBGLOG(TEST, SOAP_MESSAGE(fdebug,
            "wininet %p: failed to set option: %X, error %d (%s)\n",
            a_hHttpRequest, a_dwOption, dwErrorCode,
            wininet_error_message(soap,dwErrorCode) ));
    }
#endif
    return bSuccess;
}
#endif

static BOOL
wininet_resolve_send_error(
    HINTERNET   a_hHttpRequest,
    DWORD       a_dwErrorCode )
{
    DWORD dwResult = InternetErrorDlg(
        GetDesktopWindow(),
        a_hHttpRequest,
        a_dwErrorCode,
        FLAGS_ERROR_UI_FILTER_FOR_ERRORS |
        FLAGS_ERROR_UI_FLAGS_GENERATE_DATA |
        FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS,
        NULL );
    BOOL bRetVal = (a_dwErrorCode == ERROR_INTERNET_INCORRECT_PASSWORD) ?
        (dwResult == ERROR_INTERNET_FORCE_RETRY) :
        (dwResult == ERROR_SUCCESS);
    /* If appropriate for your application, it is possible to ignore
       errors in future once they have been handled or ignored once.
       For example, to ignore invalid SSL certificate dates on this
       connection once the client has indicated to ignore them this
       time, uncomment this code.
    */
    /*
    if (bRetVal)
    {
        switch (a_dwErrorCode)
        {
        case ERROR_INTERNET_SEC_CERT_CN_INVALID:
            wininet_flag_set_option(a_hHttpRequest,
                INTERNET_OPTION_SECURITY_FLAGS,
                SECURITY_FLAG_IGNORE_CERT_CN_INVALID );
            break;
        }
    }
    */
    return bRetVal;

}

static const char *
wininet_error_message(
    struct soap *   soap,
    DWORD           a_dwErrorMsgId )
{
    HINSTANCE   hModule;
    DWORD       dwResult;
    DWORD       dwFormatFlags;
    struct wininet_data * pData =
        (struct wininet_data *) soap_lookup_plugin( soap, wininet_id );

    /* free any existing error message */
    wininet_free_error_message( pData );

    dwFormatFlags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_FROM_SYSTEM;

    /* load wininet.dll for the error messages */
    hModule = LoadLibraryExA( "wininet.dll", NULL,
        LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES );
    if ( hModule )
    {
        dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;
    }

    /* format the messages */
    wchar_t* pszErrorMessage = NULL;
    dwResult = FormatMessageW(
        dwFormatFlags,
        hModule,
        a_dwErrorMsgId,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR) &pszErrorMessage,
        0,
        NULL );

    /* remove the CR LF from the error message and convert to proper char set */
    if ( dwResult > 2 )
    {
        dwResult -= 2;
        pszErrorMessage[dwResult] = 0;

        wchar_t* pszExtendedErrorMessage = (wchar_t*)_alloca(dwResult + 100);
        swprintf(pszExtendedErrorMessage, L"WinInet %d, %ls", a_dwErrorMsgId, pszErrorMessage);

        UINT newCodePage = CP_UTF8; // TODO: May be it should use SOAP_C_UTFSTRING or SOAP_C_MBSTRING?
        int len = WideCharToMultiByte(newCodePage, 0, pszExtendedErrorMessage, -1, NULL, 0, NULL, NULL);
        if (len)
        {
            pData->pszErrorMessage = new char[len];
            WideCharToMultiByte(newCodePage, 0, pszExtendedErrorMessage, -1, pData->pszErrorMessage, len, NULL, NULL);
        }
        else
        {
            pData->pszErrorMessage = NULL;
        }
        LocalFree( pszErrorMessage );
    }
    else
    {
        pData->pszErrorMessage = NULL;
    }

    /* free the library if we loaded it */
    if ( hModule )
    {
        FreeLibrary( hModule );
    }

    if ( pData->pszErrorMessage )
    {
        return pData->pszErrorMessage;
    }
    else
    {
        const static char szUnknown[] = "(unknown)";
        return szUnknown;
    }
}

static void
wininet_free_error_message(
    struct wininet_data *   a_pData )
{
    if ( a_pData->pszErrorMessage )
    {
        delete[] a_pData->pszErrorMessage;
        a_pData->pszErrorMessage = 0;
    }
}
