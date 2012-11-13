#include "stdafx.h"
#include "dnsmx.h"

#define ASCII_NULL              '\0'
#define MAXHOSTNAME             256
#define BUFSIZE                 2048

// #pragma comment ( lib, "ws2_32.lib" )

/* DNS Header Format
*
* All DNS Message Formats have basically the same structure
*   (note that an RR (DNS Resource Record) is described in
*   the other structures that follow this header description):
*
*        +--------------------------------+
*        | DNS Header: <defined below>    |
*        +--------------------------------+
*        | Question:   type of query      |
*        |   QNAME:    <see below>        |
*        |   QTYPE:    2-octet RR type    |
*        |   QCLASS:   2-octet RR class   |
*        +--------------------------------+
*        | Answer:     RR answer to query |
*        +--------------------------------+
*        | Authority:  RR for name server |
*        +--------------------------------+
*        | Additional: RR(s) other info   |
*        +--------------------------------+
*
*  QNAME is a variable length field where each portion of the
*   "dotted-notation" domain name is replaced by the number of
*   octets to follow.  So, for example, the domain name
*   "www.sockets.com" is represented by:
*
*         0                   1
*   octet 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6
*        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*        |3|w|w|w|7|s|o|c|k|e|t|s|3|c|o|m|0|
*        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*
* NOTE: The last section, "Additional," often contains records
*  for queries the server anticipates will be sent (to reduce
*  traffic).  For example, a response to an MX query, would
*  usually have the A record in additional information.
*/
typedef struct dns_hdr
{
	USHORT dns_id;          /* client query ID number */
	USHORT dns_flags;       /* qualify contents <see below> */
	USHORT dns_q_count;     /* number of questions */
	USHORT dns_rr_count;    /* number of answer RRs */
	USHORT dns_auth_count;  /* number of authority RRs */
	USHORT dns_add_count;   /* number of additional RRs */
} DNS_HDR, *PDNS_HDR, FAR *LPDNS_HDR;

#define DNS_HDR_LEN 12

/* DNS Flags field values
*
*  bits:  0     1-4     5    6    7    8     9-11    12-15
*       +----+--------+----+----+----+----+--------+-------+
*       | QR | opcode | AA | TC | RD | RA | <zero> | rcode |
*       +----+--------+----+----+----+----+--------+-------+
*
*  QR:     0 for query, and 1 for response
*  opcode: type of query (0: standard, and 1: inverse query)
*  AA:     set if answer from domain authority
*  TC:     set if message had to be truncated
*  RD:     set if recursive query desired
*  RA:     set if recursion is available from server
*  <zero>: reserved field
*  rcode:  resulting error non-zero value from authoritative
*           server (0: no error, 3: name does not exist)
*/
#define DNS_FLAG_QR 0x8SpeedPostEmail
#define DNS_FLAG_AA 0x0400
#define DNS_FLAG_TC 0x0200
#define DNS_FLAG_RD 0x0100
#define DNS_FLAG_RA 0x0080
#define DNS_RCODE_MASK  0xSpeedPostEmailF
#define DNS_OPCODE_MASK 0x7800

/* DNS Opcode (type of query) */
char *DNS_Opcode[] =
{
	"Standard Query",         /* 0: QUERY  */
        "Inverse Query",          /* 1: IQUERY */
        "Server Status Request",  /* 2: STATUS */
};

/* DNS Response Codes (error descriptions) */
char *DNS_RCode[] =
{
	"No Error",        /* 0: ok */
        "Format Error",    /* 1: bad query */
        "Server Failure",  /* 2: server is hosed */
        "Name Error",      /* 3: name doesn't exist (authoritative) */
        "Not Implemented", /* 4: server doesn't support query */
        "Refused"          /* 5: server refused request */
};

/* DNS Generic Resource Record format (from RFC 1034 and 1035)
*
*  NOTE: The first field in the DNS RR Record header is always
*   the domain name in QNAME format (see earlier description)
*/
typedef struct dns_rr_hdr
{
	USHORT rr_type;                /* RR type code (e.g. A, MX, NS, etc.) */
	USHORT rr_class;               /* RR class code (IN for Internet) */
	ULONG  rr_ttl;         /* Time-to-live for resource */
	USHORT rr_rdlength;    /* length of RDATA field (in octets) */
	USHORT rr_rdata;               /* (fieldname used as a ptr) */
} DNS_RR_HDR, *PDNS_RR_HDR, FAR *LPDNS_RR_HDR;

#define DNS_RR_HDR_LEN 12

/* DNS Resource Record RDATA Field Descriptions
*
*  The RDATA field contains resource record data associated
*  with the specified domain name
*
*  Type Value Description
*  -------------------------------------------------------------
*   A     1   IP Address (32-bit IP version 4)
*   NS    2   Name server QNAME (for referrals & recursive queries)
*   CNAME 5   Canonical name of an alias (in QNAME format)
*   SOA   6   Start of Zone Transfer (see definition below)
*   WKS   11  Well-known services (see definition below)
*   PTR   12  QNAME pointing to other nodes (e.g. in inverse lookups)
*   HINFO 13  Host Information (CPU string, then OS string)
*   MX    15  Mail server preference and QNAME (see below)
*/
char *DNS_RR_Type [] =
{
	"<invalid>",
        "A",     // 1:  Host Address
        "NS",    // 2:  Authoritative Name Server
        "MD",    // 3:  <obsolete>
        "MF",    // 4:  <obsolete>
        "CNAME", // 5:  The true, canonical name for an alias
        "SOA",   // 6:  Start-of-Zone of authority record
        "MB",    // 7:  Mailbox   <experimental>
        "MG",    // 8:  Mailgroup <experimental>
        "MR",    // 9:  Mail Rename Domain Name <experimental>
        "NULL",  // 10: NULL Resource Record    <experimental>
        "WKS",   // 11: Well-known service description
        "PTR",   // 12: Domain Name Pointer
        "HINFO", // 13: Host Information
        "MINFO", // 14: Mailbox or Mail List information
        "MX",    // 15: Mail Exchange (from RFC 974)
        "TXT"    // 16: Text String
};

#define DNS_RRTYPE_A     1
#define DNS_RRTYPE_NS    2
#define DNS_RRTYPE_CNAME 5
#define DNS_RRTYPE_SOA   6
#define DNS_RRTYPE_WKS   11
#define DNS_RRTYPE_PTR   12
#define DNS_RRTYPE_HINFO 13
#define DNS_RRTYPE_MX    15

/* DNS Resource Record Classes:
*
* One almost always uses Internet RR Class (also note: the
*  class value 255 denotes a wildcard, all classes)
*/
char *DNS_RR_Class [] =
{
	"<invalid>",
        "IN",    // 1: Internet - used for most queries!
        "CS",    // 2: CSNET <obsolete>
        "CH",    // 3: CHAOS Net
        "HS"     // 4: Hesiod
};

#define DNS_RRCLASS_IN  1
#define DNS_RRCLASS_CS  2
#define DNS_RRCLASS_CH  3
#define DNS_RRCLASS_HS  4

/* DNS SOA Resource Data Field
*
*  NOTE: First two fields not shown here.  They are:
*    MNAME: QNAME of primary server for this zone
*    RNAME: QNAME of mailbox of admin for this zone
*/
typedef struct dns_rdata_soa
{
	ULONG soa_serial;  /* data version for this zone */
	ULONG soa_refresh; /* time-to-live for data (in seconds) */
	ULONG soa_retry;   /* time between retrieds (in seconds) */
	ULONG soa_expire;  /* time until zone not auth (in seconds) */
	ULONG soa_minimum; /* default TTL for RRs (in seconds) */
} DNS_RDATA_SOA, PDNS_RDATA_SOA, FAR *LPDNS_RDATA_SOA;

#define DNS_SOA_LEN 20

/* DNS WKS Resource Data Field (RFC 1035)
*
*  NOTE: The bitmap field is variable length, with as many
*   octets necessary to indicate the bit field for the port
*   number.
*/
typedef struct dns_rdata_wks
{
	ULONG wks_addr;      /* IPv4 address */
	UCHAR wks_protocol;  /* Protocol (e.g. 6=TCP, 17=UDP) */
	UCHAR wks_bitmap;    /* e.g. bit 26 = SMTP (port 25) */
} DNS_RDATA_WKS, *PDNS_RDATA_WKS, FAR *LPDNS_RDATA_WKS;

#define DNS_WKX_LEN 6

/* DNS MX Resource Data Field
*/
typedef struct dns_rdata_mx
{
	USHORT mx_pref;     /* Preference value */
	USHORT mx_xchange;  /* QNAME (field used as ptr) */
} DNS_RDATA_MX, *PDNS_RDATA_MX, FAR *LPDNS_RDATA_MX;

#define DNS_MX_LEN 4

/* Variables used for DNS Header construction & parsing */
PDNS_HDR       pDNShdr;
PDNS_RR_HDR    pDNS_RR;
PDNS_RDATA_SOA pDNS_SOA;
PDNS_RDATA_WKS pDNS_WKS;
PDNS_RDATA_MX  pDNS_MX;

/* For Parsing Names in a Reply */
#define INDIR_MASK	0xc0

/* Number of bytes of fixed size data in query structure */
#define QFIXEDSZ	4
/* number of bytes of fixed size data in resource record */
#define RRFIXEDSZ	10

/* Processor Types */
char *aszProcessor[] =
{
	"",
        "",
        "",
        "Intel 386",
        "Intel 486",
        "Intel Pentium"
};


void GetQName( char FAR *pszHostName, char FAR *pQName );
void PrintQName( char FAR *pQName );
int  PutQName( char FAR *pszHostName, char FAR *pQName );

USHORT _getshort(char *msgp)
{
	register UCHAR *p = (UCHAR *) msgp;
	register USHORT u;

	u = *p++ << 8;
	return ((USHORT)(u | *p));
}

ULONG _getlong(char *msgp)
{
	register UCHAR *p = (UCHAR *) msgp;
	register ULONG u;

	u = *p++; u <<= 8;
	u |= *p++; u <<= 8;
	u |= *p++; u <<= 8;
	return (u | *p);
}


/*
* Expand compressed domain name 'comp_dn' to full domain name.
* 'msg' is a pointer to the begining of the message,
* 'eomorig' points to the first location after the message,
* 'exp_dn' is a pointer to a buffer of size 'length' for the result.
* Return size of compressed name or -1 if there was an error.
*/
int dn_expand(char *msg,char  *eomorig,char *comp_dn,char *exp_dn,int length)
{
	register char *cp, *dn;
	register int n, c;
	char *eom;
	int len = -1, checked = 0;

	dn = exp_dn;
	cp = comp_dn;
	eom = exp_dn + length - 1;
	/*
	* fetch next label in domain name
	*/
	while (n = *cp++) {
	/*
	* Check for indirection
		*/
		switch (n & INDIR_MASK) {
		case 0:
			if (dn != exp_dn) {
				if (dn >= eom)
					return (-1);
				*dn++ = '.';
			}
			if (dn+n >= eom)
				return (-1);
			checked += n + 1;
			while (--n >= 0) {
				if ((c = *cp++) == '.') {
					if (dn+n+1 >= eom)
						return (-1);
					*dn++ = '\\';
				}
				*dn++ = c;
				if (cp >= eomorig)	/* out of range */
					return(-1);
			}
			break;

		case INDIR_MASK:
			if (len < 0)
				len = cp - comp_dn + 1;
			cp = msg + (((n & 0x3f) << 8) | (*cp & 0xff));
			if (cp < msg || cp >= eomorig)	/* out of range */
				return(-1);
			checked += 2;
			/*
			* Check for loops in the compressed name;
			* if we've looked at the whole message,
			* there must be a loop.
			*/
			if (checked >= eomorig - msg)
				return (-1);
			break;

		default:
			return (-1);			/* flag error */
		}
	}
	*dn = '\0';
	if (len < 0)
		len = cp - comp_dn;
	return (len);
}

/*
* Skip over a compressed domain name. Return the size or -1.
*/
int dn_skipname(UCHAR *comp_dn,  UCHAR *eom)
{
	register UCHAR *cp;
	register int n;

	cp = comp_dn;
	while (cp < eom && (n = *cp++)) {
	/*
	* check for indirection
		*/
		switch (n & INDIR_MASK) {
		case 0:		/* normal case, n == len */
			cp += n;
			continue;
		default:	/* illegal type */
			return (-1);
		case INDIR_MASK:	/* indirection */
			cp++;
		}
		break;
	}
	return (cp - comp_dn);
}

//
// 设置阻塞模式
//
BOOL SetBlockingMode ( SOCKET hSocket, BOOL bNonblockingEnable )
{
	if ( hSocket == INVALID_SOCKET || hSocket == 0 )
	{
		return FALSE;
	}

	long cmd = FIONBIO;
	long zero = 0;
	u_long* argp = NULL;
	if ( bNonblockingEnable )
		argp = (u_long*)&cmd;
	else
		argp = (u_long*)&zero;
	int err = ioctlsocket ( hSocket, cmd, argp );
	if ( err != 0 )
	{
		return FALSE;
	}

	return TRUE;
}


BOOL GetMX (
			char *pszQuery,
			char *pszServer,
			OUT t_Ary_MXHostInfos &Ary_MXHostInfos
			)
{
	SOCKET                  hSocket;
	SOCKADDR_IN             stSockAddr;                     // socket address structures
	int                             nAddrLen = sizeof( SOCKADDR_IN );

	HOSTENT                 *pHostEnt;

	char                            achBufOut[ BUFSIZE ] = { 0 };
	char                            achBufIn[ BUFSIZE ] = { 0 };
	int                             nQueryLen = 0;
	int                             nRC;

	char *p, *np, name[128], *eom;
	int count, j, i, n;

	memset( &stSockAddr, ASCII_NULL, sizeof( stSockAddr ) );

	stSockAddr.sin_family      = AF_INET;
	stSockAddr.sin_port             = htons( 53);
	stSockAddr.sin_addr.s_addr = inet_addr( pszServer );
	if ( stSockAddr.sin_addr.s_addr == INADDR_NONE )
	{
		pHostEnt = gethostbyname( pszServer );
		if ( pHostEnt )
		{
			stSockAddr.sin_addr.s_addr = *((ULONG *)pHostEnt->h_addr_list[0]);
		}
		else
		{
			return FALSE;
		} // end if
	} // end if


	  /*------------------------------------------------------------
	  *  Get a DGRAM socket
	*/

	hSocket = socket( AF_INET, SOCK_DGRAM, 0 );

	if ( hSocket == INVALID_SOCKET )
	{
		return FALSE;
	} // end if

	  /*-----------------------------------------------------------
	  * Format DNS Query
	*/

	pDNShdr = (PDNS_HDR)&( achBufOut[ 0 ] );
	pDNShdr->dns_id         = htons( 0xDEAD );
	pDNShdr->dns_flags      = htons( DNS_FLAG_RD ); // do recurse
	pDNShdr->dns_q_count    = htons( 1 );           // one query
	pDNShdr->dns_rr_count   = 0;                  // none in query
	pDNShdr->dns_auth_count = 0;                  // none in query
	pDNShdr->dns_add_count  = 0;                  // none in query

	nQueryLen = PutQName( pszQuery, &(achBufOut[ DNS_HDR_LEN ] ) );
	nQueryLen += DNS_HDR_LEN;

	achBufOut[ nQueryLen++ ]        = 0;
	achBufOut[ nQueryLen++ ]        = 0;
	achBufOut[ nQueryLen ]          = DNS_RRTYPE_MX;
	achBufOut[ nQueryLen + 1 ]      = 0;
	achBufOut[ nQueryLen + 2 ]      = DNS_RRCLASS_IN;
	achBufOut[ nQueryLen + 3 ]      = 0;

	nQueryLen += 4;

	/*-----------------------------------------------------------
	* Send DNS Query to server
	*/

	nRC = sendto( hSocket,
		achBufOut,
		nQueryLen,
		0,
		(LPSOCKADDR)&stSockAddr,
		sizeof( SOCKADDR_IN ) );

	if ( nRC == SOCKET_ERROR )
	{

		closesocket( hSocket );
		return FALSE;
	}
	else
	{

	}

//	VERIFY ( SetBlockingMode ( hSocket, TRUE ) );

	// 用 select 模型实现连接超时
	struct timeval timeout;
	fd_set r;
	FD_ZERO(&r);
	FD_SET(hSocket, &r);
	timeout.tv_sec = 5; //连接超时秒
	timeout.tv_usec =0;
	int ret = select(0, &r, 0, 0, &timeout);
	if ( ret == SOCKET_ERROR )
	{
		::closesocket(hSocket);
		hSocket = SOCKET_ERROR;
		return FALSE;
	}

	// 得到可读的数据长度
	long cmd = FIONREAD;
	u_long argp = 0;
	BOOL err = ioctlsocket ( hSocket, cmd, (u_long*)&argp );
	if ( err || argp < 1 )
	{
		::closesocket(hSocket);
		hSocket = SOCKET_ERROR;
		return FALSE;
	}

	nRC = recvfrom( hSocket,
		achBufIn,
		BUFSIZE,
		0,
		(LPSOCKADDR)&stSockAddr,
		&nAddrLen );

	if ( nRC == SOCKET_ERROR )
	{
		int nWSAErr = WSAGetLastError();

		if ( nWSAErr != WSAETIMEDOUT )
		{

			closesocket( hSocket );
			return FALSE;
		}
		else
		{

			closesocket( hSocket );
			return FALSE;
		}
	}
	else
	{
		pDNShdr = (PDNS_HDR)&( achBufIn[ 0 ] );
		p = (char *)&pDNShdr[0];
		p+=12;
		count = (int)*p;

		// Parse the Question...
		for (i = 0; i< ntohs(pDNShdr->dns_q_count); i++)
		{
			np = name;
			eom = (char *)pDNShdr+nRC;

			if ( (n = dn_expand((char *)pDNShdr, eom, p, name, 127)) < 0 )
			{
				return FALSE;

			}
			p += n + QFIXEDSZ;
		}

		for (i = 0; i< ntohs(pDNShdr->dns_rr_count); i++)
		{

			// The Question Name appears Again...
			if ((n = dn_expand((char *)pDNShdr, eom, p, name, 127)) < 0)
			{
				return FALSE;
			}
			p+=n;


			j =  _getshort(p);;  //TYPE
			p+=2;
			//printf("%s\tType:%d", name, j);

			j = _getshort(p);  //CLASS
			p+=2;
			//	printf("\tClass:%d", j);

			j = _getlong(p);  //TTL
			p+=4;
			//	printf("\tTTL:%d", j);

			j = _getshort(p);  //RDLENGTH
			p+=2;
			//	printf("\tRDLENGTH:%d", j);

			j = _getshort(p);  //N??
			p+=2;

			// This should be an MX Name...
			if ( (n = dn_expand((char *)pDNShdr, eom, p, name, 127)) < 0 )
			{
				return FALSE;
			}

			t_MXHostInfo tMXHostInfo = {0};
			strncpy ( (char*)tMXHostInfo.szMXHost, name, _countof(tMXHostInfo.szMXHost));
			tMXHostInfo.N = j;
			Ary_MXHostInfos.Add ( tMXHostInfo );
			TRACE ( _T("%s\t%d\r\n"), name, j );
			p += n;
		}
		return TRUE;


	}


	closesocket( hSocket );
	return FALSE;
}


void GetQName( char FAR *pszHostName, char FAR *pQName )
{

	int i, j, k;

	for ( i = 0; i < BUFSIZE; i++ )
	{
		j = *pQName;

		if ( j == 0 )
			break;

		for ( k = 1; k <= j; k++ )
		{
			*pszHostName++ = *( pQName + i + k );
		} // end for loop
	} // end for loop

	*pszHostName++ = ASCII_NULL;
} /* end GetQName() */


void PrintQName( char FAR *pQName )
{
	int i, j, k;

	for ( i = 0; i < BUFSIZE; i++ )
	{
		j = *pQName;

		if ( j == 0 )
			break;

		for ( k = 1; k <= j; k++ )
		{
			//printf( "%c", *( pQName + i + k ) );
		} // end for loop
	} // end for loop
} /* end PrintQName() */


int PutQName( char FAR *pszHostName, char FAR *pQName )
{
	int     i;
	int     j = 0;
	int     k = 0;


	for ( i = 0; *( pszHostName + i ); i++ )
	{
		char c = *( pszHostName + i );   /* get next character */


		if ( c == '.' )
		{
			/* dot encountered, fill in previous length */
			*( pQName + j ) = k;

			k = 0;      /* reset segment length */
			j = i + 1;    /* set index to next counter */
		}
		else
		{
			*( pQName + i + 1 ) = c;  /* assign to QName */
			k++;                /* inc count of seg chars */
		} // end if
	} // end for loop

	*(pQName + j )                  = k;   /* count for final segment */
	*(pQName + i + 1 )      = 0;   /* count for trailing NULL segment is 0 */

	return ( i + 1 );        /* return total length of QName */
}

//
// 尝试所有的DNS来查询邮局服务器地址
//
BOOL GetMX (
			char *pszQuery,							// 要查询的域名
			OUT t_Ary_MXHostInfos &Ary_MXHostInfos	// 输出 Mail Exchange 主机名
			)
{
	CNetAdapterInfo m_NetAdapterInfo;
	m_NetAdapterInfo.Refresh ();
	int nNetAdapterCount = m_NetAdapterInfo.GetNetCardCount();
	for ( int i=0; i<nNetAdapterCount; i++ )
	{
		COneNetAdapterInfo *pOneNetAdapterInfo = m_NetAdapterInfo.Get_OneNetAdapterInfo ( i );
		if ( pOneNetAdapterInfo )
		{
			int nDNSCount = pOneNetAdapterInfo->Get_DNSCount ();
			for ( int j=0; j<nDNSCount; j++ )
			{
				CString csDNS = pOneNetAdapterInfo->Get_DNSAddr ( j );
				if ( GetMX ( pszQuery, csDNS.GetBuffer(0), Ary_MXHostInfos ) )
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}