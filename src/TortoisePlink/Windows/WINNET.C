/*
 * Windows networking abstraction.
 *
 * For the IPv6 code in here I am indebted to Jeroen Massar and
 * unfix.org.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define DEFINE_PLUG_METHOD_MACROS
#include "putty.h"
#include "network.h"
#include "tree234.h"

#include <ws2tcpip.h>

#ifndef NO_IPV6
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
#endif

#define ipv4_is_loopback(addr) \
	((p_ntohl(addr.s_addr) & 0xFF000000L) == 0x7F000000L)

/*
 * We used to typedef struct Socket_tag *Socket.
 *
 * Since we have made the networking abstraction slightly more
 * abstract, Socket no longer means a tcp socket (it could mean
 * an ssl socket).  So now we must use Actual_Socket when we know
 * we are talking about a tcp socket.
 */
typedef struct Socket_tag *Actual_Socket;

/*
 * Mutable state that goes with a SockAddr: stores information
 * about where in the list of candidate IP(v*) addresses we've
 * currently got to.
 */
typedef struct SockAddrStep_tag SockAddrStep;
struct SockAddrStep_tag {
#ifndef NO_IPV6
    struct addrinfo *ai;	       /* steps along addr->ais */
#endif
    int curraddr;
};

struct Socket_tag {
    const struct socket_function_table *fn;
    /* the above variable absolutely *must* be the first in this structure */
    char *error;
    SOCKET s;
    Plug plug;
    bufchain output_data;
    int connected;
    int writable;
    int frozen; /* this causes readability notifications to be ignored */
    int frozen_readable; /* this means we missed at least one readability
			  * notification while we were frozen */
    int localhost_only;		       /* for listening sockets */
    char oobdata[1];
    int sending_oob;
    int oobinline, nodelay, keepalive, privport;
    enum { EOF_NO, EOF_PENDING, EOF_SENT } outgoingeof;
    SockAddr addr;
    SockAddrStep step;
    int port;
    int pending_error;		       /* in case send() returns error */
    /*
     * We sometimes need pairs of Socket structures to be linked:
     * if we are listening on the same IPv6 and v4 port, for
     * example. So here we define `parent' and `child' pointers to
     * track this link.
     */
    Actual_Socket parent, child;
};

struct SockAddr_tag {
    int refcount;
    char *error;
    int resolved;
    int namedpipe; /* indicates that this SockAddr is phony, holding a Windows
                    * named pipe pathname instead of a network address */
#ifndef NO_IPV6
    struct addrinfo *ais;	       /* Addresses IPv6 style. */
#endif
    unsigned long *addresses;	       /* Addresses IPv4 style. */
    int naddresses;
    char hostname[512];		       /* Store an unresolved host name. */
};

/*
 * Which address family this address belongs to. AF_INET for IPv4;
 * AF_INET6 for IPv6; AF_UNSPEC indicates that name resolution has
 * not been done and a simple host name is held in this SockAddr
 * structure.
 */
#ifndef NO_IPV6
#define SOCKADDR_FAMILY(addr, step) \
    (!(addr)->resolved ? AF_UNSPEC : \
     (step).ai ? (step).ai->ai_family : AF_INET)
#else
#define SOCKADDR_FAMILY(addr, step) \
    (!(addr)->resolved ? AF_UNSPEC : AF_INET)
#endif

/*
 * Start a SockAddrStep structure to step through multiple
 * addresses.
 */
#ifndef NO_IPV6
#define START_STEP(addr, step) \
    ((step).ai = (addr)->ais, (step).curraddr = 0)
#else
#define START_STEP(addr, step) \
    ((step).curraddr = 0)
#endif

static tree234 *sktree;

static int cmpfortree(void *av, void *bv)
{
    Actual_Socket a = (Actual_Socket) av, b = (Actual_Socket) bv;
    unsigned long as = (unsigned long) a->s, bs = (unsigned long) b->s;
    if (as < bs)
	return -1;
    if (as > bs)
	return +1;
    if (a < b)
	return -1;
    if (a > b)
	return +1;
    return 0;
}

static int cmpforsearch(void *av, void *bv)
{
    Actual_Socket b = (Actual_Socket) bv;
    unsigned long as = (unsigned long) av, bs = (unsigned long) b->s;
    if (as < bs)
	return -1;
    if (as > bs)
	return +1;
    return 0;
}

DECL_WINDOWS_FUNCTION(static, int, WSAStartup, (WORD, LPWSADATA));
DECL_WINDOWS_FUNCTION(static, int, WSACleanup, (void));
DECL_WINDOWS_FUNCTION(static, int, closesocket, (SOCKET));
DECL_WINDOWS_FUNCTION(static, u_long, ntohl, (u_long));
DECL_WINDOWS_FUNCTION(static, u_long, htonl, (u_long));
DECL_WINDOWS_FUNCTION(static, u_short, htons, (u_short));
DECL_WINDOWS_FUNCTION(static, u_short, ntohs, (u_short));
DECL_WINDOWS_FUNCTION(static, int, gethostname, (char *, int));
DECL_WINDOWS_FUNCTION(static, struct hostent FAR *, gethostbyname,
		      (const char FAR *));
DECL_WINDOWS_FUNCTION(static, struct servent FAR *, getservbyname,
		      (const char FAR *, const char FAR *));
DECL_WINDOWS_FUNCTION(static, unsigned long, inet_addr, (const char FAR *));
DECL_WINDOWS_FUNCTION(static, char FAR *, inet_ntoa, (struct in_addr));
DECL_WINDOWS_FUNCTION(static, int, connect,
		      (SOCKET, const struct sockaddr FAR *, int));
DECL_WINDOWS_FUNCTION(static, int, bind,
		      (SOCKET, const struct sockaddr FAR *, int));
DECL_WINDOWS_FUNCTION(static, int, setsockopt,
		      (SOCKET, int, int, const char FAR *, int));
DECL_WINDOWS_FUNCTION(static, SOCKET, socket, (int, int, int));
DECL_WINDOWS_FUNCTION(static, int, listen, (SOCKET, int));
DECL_WINDOWS_FUNCTION(static, int, send, (SOCKET, const char FAR *, int, int));
DECL_WINDOWS_FUNCTION(static, int, shutdown, (SOCKET, int));
DECL_WINDOWS_FUNCTION(static, int, ioctlsocket,
		      (SOCKET, long, u_long FAR *));
DECL_WINDOWS_FUNCTION(static, SOCKET, accept,
		      (SOCKET, struct sockaddr FAR *, int FAR *));
DECL_WINDOWS_FUNCTION(static, int, recv, (SOCKET, char FAR *, int, int));
DECL_WINDOWS_FUNCTION(static, int, WSAIoctl,
		      (SOCKET, DWORD, LPVOID, DWORD, LPVOID, DWORD,
		       LPDWORD, LPWSAOVERLAPPED,
		       LPWSAOVERLAPPED_COMPLETION_ROUTINE));
#ifndef NO_IPV6
DECL_WINDOWS_FUNCTION(static, int, getaddrinfo,
		      (const char *nodename, const char *servname,
		       const struct addrinfo *hints, struct addrinfo **res));
DECL_WINDOWS_FUNCTION(static, void, freeaddrinfo, (struct addrinfo *res));
DECL_WINDOWS_FUNCTION(static, int, getnameinfo,
		      (const struct sockaddr FAR * sa, socklen_t salen,
		       char FAR * host, size_t hostlen, char FAR * serv,
		       size_t servlen, int flags));
DECL_WINDOWS_FUNCTION(static, char *, gai_strerror, (int ecode));
DECL_WINDOWS_FUNCTION(static, int, WSAAddressToStringA,
		      (LPSOCKADDR, DWORD, LPWSAPROTOCOL_INFO,
		       LPSTR, LPDWORD));
#endif

static HMODULE winsock_module = NULL;
static WSADATA wsadata;
#ifndef NO_IPV6
static HMODULE winsock2_module = NULL;
static HMODULE wship6_module = NULL;
#endif

int sk_startup(int hi, int lo)
{
    WORD winsock_ver;

    winsock_ver = MAKEWORD(hi, lo);

    if (p_WSAStartup(winsock_ver, &wsadata)) {
	return FALSE;
    }

    if (LOBYTE(wsadata.wVersion) != LOBYTE(winsock_ver)) {
	return FALSE;
    }

#ifdef NET_SETUP_DIAGNOSTICS
    {
	char buf[80];
	sprintf(buf, "Using WinSock %d.%d", hi, lo);
	logevent(NULL, buf);
    }
#endif
    return TRUE;
}

void sk_init(void)
{
#ifndef NO_IPV6
    winsock2_module =
#endif
        winsock_module = load_system32_dll("ws2_32.dll");
    if (!winsock_module) {
	winsock_module = load_system32_dll("wsock32.dll");
    }
    if (!winsock_module)
	fatalbox("Unable to load any WinSock library");

#ifndef NO_IPV6
    /* Check if we have getaddrinfo in Winsock */
    if (GetProcAddress(winsock_module, "getaddrinfo") != NULL) {
#ifdef NET_SETUP_DIAGNOSTICS
	logevent(NULL, "Native WinSock IPv6 support detected");
#endif
	GET_WINDOWS_FUNCTION(winsock_module, getaddrinfo);
	GET_WINDOWS_FUNCTION(winsock_module, freeaddrinfo);
	GET_WINDOWS_FUNCTION(winsock_module, getnameinfo);
	GET_WINDOWS_FUNCTION(winsock_module, gai_strerror);
    } else {
	/* Fall back to wship6.dll for Windows 2000 */
	wship6_module = load_system32_dll("wship6.dll");
	if (wship6_module) {
#ifdef NET_SETUP_DIAGNOSTICS
	    logevent(NULL, "WSH IPv6 support detected");
#endif
	    GET_WINDOWS_FUNCTION(wship6_module, getaddrinfo);
	    GET_WINDOWS_FUNCTION(wship6_module, freeaddrinfo);
	    GET_WINDOWS_FUNCTION(wship6_module, getnameinfo);
	    GET_WINDOWS_FUNCTION(wship6_module, gai_strerror);
	} else {
#ifdef NET_SETUP_DIAGNOSTICS
	    logevent(NULL, "No IPv6 support detected");
#endif
	}
    }
    GET_WINDOWS_FUNCTION(winsock2_module, WSAAddressToStringA);
#else
#ifdef NET_SETUP_DIAGNOSTICS
    logevent(NULL, "PuTTY was built without IPv6 support");
#endif
#endif

    GET_WINDOWS_FUNCTION(winsock_module, WSAAsyncSelect);
    GET_WINDOWS_FUNCTION(winsock_module, WSAEventSelect);
    GET_WINDOWS_FUNCTION(winsock_module, select);
    GET_WINDOWS_FUNCTION(winsock_module, WSAGetLastError);
    GET_WINDOWS_FUNCTION(winsock_module, WSAEnumNetworkEvents);
    GET_WINDOWS_FUNCTION(winsock_module, WSAStartup);
    GET_WINDOWS_FUNCTION(winsock_module, WSACleanup);
    GET_WINDOWS_FUNCTION(winsock_module, closesocket);
    GET_WINDOWS_FUNCTION(winsock_module, ntohl);
    GET_WINDOWS_FUNCTION(winsock_module, htonl);
    GET_WINDOWS_FUNCTION(winsock_module, htons);
    GET_WINDOWS_FUNCTION(winsock_module, ntohs);
    GET_WINDOWS_FUNCTION(winsock_module, gethostname);
    GET_WINDOWS_FUNCTION(winsock_module, gethostbyname);
    GET_WINDOWS_FUNCTION(winsock_module, getservbyname);
    GET_WINDOWS_FUNCTION(winsock_module, inet_addr);
    GET_WINDOWS_FUNCTION(winsock_module, inet_ntoa);
    GET_WINDOWS_FUNCTION(winsock_module, connect);
    GET_WINDOWS_FUNCTION(winsock_module, bind);
    GET_WINDOWS_FUNCTION(winsock_module, setsockopt);
    GET_WINDOWS_FUNCTION(winsock_module, socket);
    GET_WINDOWS_FUNCTION(winsock_module, listen);
    GET_WINDOWS_FUNCTION(winsock_module, send);
    GET_WINDOWS_FUNCTION(winsock_module, shutdown);
    GET_WINDOWS_FUNCTION(winsock_module, ioctlsocket);
    GET_WINDOWS_FUNCTION(winsock_module, accept);
    GET_WINDOWS_FUNCTION(winsock_module, recv);
    GET_WINDOWS_FUNCTION(winsock_module, WSAIoctl);

    /* Try to get the best WinSock version we can get */
    if (!sk_startup(2,2) &&
	!sk_startup(2,0) &&
	!sk_startup(1,1)) {
	fatalbox("Unable to initialise WinSock");
    }

    sktree = newtree234(cmpfortree);
}

void sk_cleanup(void)
{
    Actual_Socket s;
    int i;

    if (sktree) {
	for (i = 0; (s = index234(sktree, i)) != NULL; i++) {
	    p_closesocket(s->s);
	}
	freetree234(sktree);
	sktree = NULL;
    }

    if (p_WSACleanup)
	p_WSACleanup();
    if (winsock_module)
	FreeLibrary(winsock_module);
#ifndef NO_IPV6
    if (wship6_module)
	FreeLibrary(wship6_module);
#endif
}

struct errstring {
    int error;
    char *text;
};

static int errstring_find(void *av, void *bv)
{
    int *a = (int *)av;
    struct errstring *b = (struct errstring *)bv;
    if (*a < b->error)
        return -1;
    if (*a > b->error)
        return +1;
    return 0;
}
static int errstring_compare(void *av, void *bv)
{
    struct errstring *a = (struct errstring *)av;
    return errstring_find(&a->error, bv);
}

static tree234 *errstrings = NULL;

char *winsock_error_string(int error)
{
    const char prefix[] = "Network error: ";
    struct errstring *es;

    /*
     * Error codes we know about and have historically had reasonably
     * sensible error messages for.
     */
    switch (error) {
      case WSAEACCES:
	return "Network error: Permission denied";
      case WSAEADDRINUSE:
	return "Network error: Address already in use";
      case WSAEADDRNOTAVAIL:
	return "Network error: Cannot assign requested address";
      case WSAEAFNOSUPPORT:
	return
	    "Network error: Address family not supported by protocol family";
      case WSAEALREADY:
	return "Network error: Operation already in progress";
      case WSAECONNABORTED:
	return "Network error: Software caused connection abort";
      case WSAECONNREFUSED:
	return "Network error: Connection refused";
      case WSAECONNRESET:
	return "Network error: Connection reset by peer";
      case WSAEDESTADDRREQ:
	return "Network error: Destination address required";
      case WSAEFAULT:
	return "Network error: Bad address";
      case WSAEHOSTDOWN:
	return "Network error: Host is down";
      case WSAEHOSTUNREACH:
	return "Network error: No route to host";
      case WSAEINPROGRESS:
	return "Network error: Operation now in progress";
      case WSAEINTR:
	return "Network error: Interrupted function call";
      case WSAEINVAL:
	return "Network error: Invalid argument";
      case WSAEISCONN:
	return "Network error: Socket is already connected";
      case WSAEMFILE:
	return "Network error: Too many open files";
      case WSAEMSGSIZE:
	return "Network error: Message too long";
      case WSAENETDOWN:
	return "Network error: Network is down";
      case WSAENETRESET:
	return "Network error: Network dropped connection on reset";
      case WSAENETUNREACH:
	return "Network error: Network is unreachable";
      case WSAENOBUFS:
	return "Network error: No buffer space available";
      case WSAENOPROTOOPT:
	return "Network error: Bad protocol option";
      case WSAENOTCONN:
	return "Network error: Socket is not connected";
      case WSAENOTSOCK:
	return "Network error: Socket operation on non-socket";
      case WSAEOPNOTSUPP:
	return "Network error: Operation not supported";
      case WSAEPFNOSUPPORT:
	return "Network error: Protocol family not supported";
      case WSAEPROCLIM:
	return "Network error: Too many processes";
      case WSAEPROTONOSUPPORT:
	return "Network error: Protocol not supported";
      case WSAEPROTOTYPE:
	return "Network error: Protocol wrong type for socket";
      case WSAESHUTDOWN:
	return "Network error: Cannot send after socket shutdown";
      case WSAESOCKTNOSUPPORT:
	return "Network error: Socket type not supported";
      case WSAETIMEDOUT:
	return "Network error: Connection timed out";
      case WSAEWOULDBLOCK:
	return "Network error: Resource temporarily unavailable";
      case WSAEDISCON:
	return "Network error: Graceful shutdown in progress";
    }

    /*
     * Generic code to handle any other error.
     *
     * Slightly nasty hack here: we want to return a static string
     * which the caller will never have to worry about freeing, but on
     * the other hand if we call FormatMessage to get it then it will
     * want to either allocate a buffer or write into one we own.
     *
     * So what we do is to maintain a tree234 of error strings we've
     * already used. New ones are allocated from the heap, but then
     * put in this tree and kept forever.
     */

    if (!errstrings)
        errstrings = newtree234(errstring_compare);

    es = find234(errstrings, &error, errstring_find);

    if (!es) {
        int bufsize, bufused;

        es = snew(struct errstring);
        es->error = error;
        /* maximum size for FormatMessage is 64K */
        bufsize = 65535 + sizeof(prefix);
        es->text = snewn(bufsize, char);
        strcpy(es->text, prefix);
        bufused = strlen(es->text);
        if (!FormatMessage((FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_IGNORE_INSERTS), NULL, error,
                           MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                           es->text + bufused, bufsize - bufused, NULL)) {
            sprintf(es->text + bufused,
                    "Windows error code %d (and FormatMessage returned %d)", 
                    error, GetLastError());
        } else {
            int len = strlen(es->text);
            if (len > 0 && es->text[len-1] == '\n')
                es->text[len-1] = '\0';
        }
        es->text = sresize(es->text, strlen(es->text) + 1, char);
        add234(errstrings, es);
    }

    return es->text;
}

SockAddr sk_namelookup(const char *host, char **canonicalname,
		       int address_family)
{
    SockAddr ret = snew(struct SockAddr_tag);
    unsigned long a;
    char realhost[8192];
    int hint_family;

    /* Default to IPv4. */
    hint_family = (address_family == ADDRTYPE_IPV4 ? AF_INET :
#ifndef NO_IPV6
		   address_family == ADDRTYPE_IPV6 ? AF_INET6 :
#endif
		   AF_UNSPEC);

    /* Clear the structure and default to IPv4. */
    memset(ret, 0, sizeof(struct SockAddr_tag));
#ifndef NO_IPV6
    ret->ais = NULL;
#endif
    ret->namedpipe = FALSE;
    ret->addresses = NULL;
    ret->resolved = FALSE;
    ret->refcount = 1;
    *realhost = '\0';

    if ((a = p_inet_addr(host)) == (unsigned long) INADDR_NONE) {
	struct hostent *h = NULL;
	int err;
#ifndef NO_IPV6
	/*
	 * Use getaddrinfo when it's available
	 */
	if (p_getaddrinfo) {
	    struct addrinfo hints;
#ifdef NET_SETUP_DIAGNOSTICS
	    logevent(NULL, "Using getaddrinfo() for resolving");
#endif
	    memset(&hints, 0, sizeof(hints));
	    hints.ai_family = hint_family;
	    hints.ai_flags = AI_CANONNAME;
            {
                /* strip [] on IPv6 address literals */
                char *trimmed_host = host_strduptrim(host);
                err = p_getaddrinfo(trimmed_host, NULL, &hints, &ret->ais);
                sfree(trimmed_host);
            }
	    if (err == 0)
		ret->resolved = TRUE;
	} else
#endif
	{
#ifdef NET_SETUP_DIAGNOSTICS
	    logevent(NULL, "Using gethostbyname() for resolving");
#endif
	    /*
	     * Otherwise use the IPv4-only gethostbyname...
	     * (NOTE: we don't use gethostbyname as a fallback!)
	     */
	    if ( (h = p_gethostbyname(host)) )
		ret->resolved = TRUE;
	    else
		err = p_WSAGetLastError();
	}

	if (!ret->resolved) {
	    ret->error = (err == WSAENETDOWN ? "Network is down" :
			  err == WSAHOST_NOT_FOUND ? "Host does not exist" :
			  err == WSATRY_AGAIN ? "Host not found" :
#ifndef NO_IPV6
			  p_getaddrinfo&&p_gai_strerror ? p_gai_strerror(err) :
#endif
			  "gethostbyname: unknown error");
	} else {
	    ret->error = NULL;

#ifndef NO_IPV6
	    /* If we got an address info use that... */
	    if (ret->ais) {
		/* Are we in IPv4 fallback mode? */
		/* We put the IPv4 address into the a variable so we can further-on use the IPv4 code... */
		if (ret->ais->ai_family == AF_INET)
		    memcpy(&a,
			   (char *) &((SOCKADDR_IN *) ret->ais->
				      ai_addr)->sin_addr, sizeof(a));

		if (ret->ais->ai_canonname)
		    strncpy(realhost, ret->ais->ai_canonname, lenof(realhost));
		else
		    strncpy(realhost, host, lenof(realhost));
	    }
	    /* We used the IPv4-only gethostbyname()... */
	    else
#endif
	    {
		int n;
		for (n = 0; h->h_addr_list[n]; n++);
		ret->addresses = snewn(n, unsigned long);
		ret->naddresses = n;
		for (n = 0; n < ret->naddresses; n++) {
		    memcpy(&a, h->h_addr_list[n], sizeof(a));
		    ret->addresses[n] = p_ntohl(a);
		}
		memcpy(&a, h->h_addr, sizeof(a));
		/* This way we are always sure the h->h_name is valid :) */
		strncpy(realhost, h->h_name, sizeof(realhost));
	    }
	}
    } else {
	/*
	 * This must be a numeric IPv4 address because it caused a
	 * success return from inet_addr.
	 */
	ret->addresses = snewn(1, unsigned long);
	ret->naddresses = 1;
	ret->addresses[0] = p_ntohl(a);
	ret->resolved = TRUE;
	strncpy(realhost, host, sizeof(realhost));
    }
    realhost[lenof(realhost)-1] = '\0';
    *canonicalname = snewn(1+strlen(realhost), char);
    strcpy(*canonicalname, realhost);
    return ret;
}

SockAddr sk_nonamelookup(const char *host)
{
    SockAddr ret = snew(struct SockAddr_tag);
    ret->error = NULL;
    ret->resolved = FALSE;
#ifndef NO_IPV6
    ret->ais = NULL;
#endif
    ret->namedpipe = FALSE;
    ret->addresses = NULL;
    ret->naddresses = 0;
    ret->refcount = 1;
    strncpy(ret->hostname, host, lenof(ret->hostname));
    ret->hostname[lenof(ret->hostname)-1] = '\0';
    return ret;
}

SockAddr sk_namedpipe_addr(const char *pipename)
{
    SockAddr ret = snew(struct SockAddr_tag);
    ret->error = NULL;
    ret->resolved = FALSE;
#ifndef NO_IPV6
    ret->ais = NULL;
#endif
    ret->namedpipe = TRUE;
    ret->addresses = NULL;
    ret->naddresses = 0;
    ret->refcount = 1;
    strncpy(ret->hostname, pipename, lenof(ret->hostname));
    ret->hostname[lenof(ret->hostname)-1] = '\0';
    return ret;
}

int sk_nextaddr(SockAddr addr, SockAddrStep *step)
{
#ifndef NO_IPV6
    if (step->ai) {
	if (step->ai->ai_next) {
	    step->ai = step->ai->ai_next;
	    return TRUE;
	} else
	    return FALSE;
    }
#endif
    if (step->curraddr+1 < addr->naddresses) {
	step->curraddr++;
	return TRUE;
    } else {
	return FALSE;
    }
}

void sk_getaddr(SockAddr addr, char *buf, int buflen)
{
    SockAddrStep step;
    START_STEP(addr, step);

#ifndef NO_IPV6
    if (step.ai) {
	int err = 0;
	if (p_WSAAddressToStringA) {
	    DWORD dwbuflen = buflen;
	    err = p_WSAAddressToStringA(step.ai->ai_addr, step.ai->ai_addrlen,
					NULL, buf, &dwbuflen);
	} else
	    err = -1;
	if (err) {
	    strncpy(buf, addr->hostname, buflen);
	    if (!buf[0])
		strncpy(buf, "<unknown>", buflen);
	    buf[buflen-1] = '\0';
	}
    } else
#endif
    if (SOCKADDR_FAMILY(addr, step) == AF_INET) {
	struct in_addr a;
	assert(addr->addresses && step.curraddr < addr->naddresses);
	a.s_addr = p_htonl(addr->addresses[step.curraddr]);
	strncpy(buf, p_inet_ntoa(a), buflen);
	buf[buflen-1] = '\0';
    } else {
	strncpy(buf, addr->hostname, buflen);
	buf[buflen-1] = '\0';
    }
}

int sk_addr_needs_port(SockAddr addr)
{
    return addr->namedpipe ? FALSE : TRUE;
}

int sk_hostname_is_local(const char *name)
{
    return !strcmp(name, "localhost") ||
	   !strcmp(name, "::1") ||
	   !strncmp(name, "127.", 4);
}

static INTERFACE_INFO local_interfaces[16];
static int n_local_interfaces;       /* 0=not yet, -1=failed, >0=number */

static int ipv4_is_local_addr(struct in_addr addr)
{
    if (ipv4_is_loopback(addr))
	return 1;		       /* loopback addresses are local */
    if (!n_local_interfaces) {
	SOCKET s = p_socket(AF_INET, SOCK_DGRAM, 0);
	DWORD retbytes;

	if (p_WSAIoctl &&
	    p_WSAIoctl(s, SIO_GET_INTERFACE_LIST, NULL, 0,
		       local_interfaces, sizeof(local_interfaces),
		       &retbytes, NULL, NULL) == 0)
	    n_local_interfaces = retbytes / sizeof(INTERFACE_INFO);
	else
	    logevent(NULL, "Unable to get list of local IP addresses");
    }
    if (n_local_interfaces > 0) {
	int i;
	for (i = 0; i < n_local_interfaces; i++) {
	    SOCKADDR_IN *address =
		(SOCKADDR_IN *)&local_interfaces[i].iiAddress;
	    if (address->sin_addr.s_addr == addr.s_addr)
		return 1;	       /* this address is local */
	}
    }
    return 0;		       /* this address is not local */
}

int sk_address_is_local(SockAddr addr)
{
    SockAddrStep step;
    int family;
    START_STEP(addr, step);
    family = SOCKADDR_FAMILY(addr, step);

#ifndef NO_IPV6
    if (family == AF_INET6) {
    	return IN6_IS_ADDR_LOOPBACK(&((const struct sockaddr_in6 *)step.ai->ai_addr)->sin6_addr);
    } else
#endif
    if (family == AF_INET) {
#ifndef NO_IPV6
	if (step.ai) {
	    return ipv4_is_local_addr(((struct sockaddr_in *)step.ai->ai_addr)
				      ->sin_addr);
	} else
#endif
	{
	    struct in_addr a;
	    assert(addr->addresses && step.curraddr < addr->naddresses);
	    a.s_addr = p_htonl(addr->addresses[step.curraddr]);
	    return ipv4_is_local_addr(a);
	}
    } else {
	assert(family == AF_UNSPEC);
	return 0;		       /* we don't know; assume not */
    }
}

int sk_address_is_special_local(SockAddr addr)
{
    return 0;                /* no Unix-domain socket analogue here */
}

int sk_addrtype(SockAddr addr)
{
    SockAddrStep step;
    int family;
    START_STEP(addr, step);
    family = SOCKADDR_FAMILY(addr, step);

    return (family == AF_INET ? ADDRTYPE_IPV4 :
#ifndef NO_IPV6
	    family == AF_INET6 ? ADDRTYPE_IPV6 :
#endif
	    ADDRTYPE_NAME);
}

void sk_addrcopy(SockAddr addr, char *buf)
{
    SockAddrStep step;
    int family;
    START_STEP(addr, step);
    family = SOCKADDR_FAMILY(addr, step);

    assert(family != AF_UNSPEC);
#ifndef NO_IPV6
    if (step.ai) {
	if (family == AF_INET)
	    memcpy(buf, &((struct sockaddr_in *)step.ai->ai_addr)->sin_addr,
		   sizeof(struct in_addr));
	else if (family == AF_INET6)
	    memcpy(buf, &((struct sockaddr_in6 *)step.ai->ai_addr)->sin6_addr,
		   sizeof(struct in6_addr));
	else
	    assert(FALSE);
    } else
#endif
    if (family == AF_INET) {
	struct in_addr a;
	assert(addr->addresses && step.curraddr < addr->naddresses);
	a.s_addr = p_htonl(addr->addresses[step.curraddr]);
	memcpy(buf, (char*) &a.s_addr, 4);
    }
}

void sk_addr_free(SockAddr addr)
{
    if (--addr->refcount > 0)
	return;
#ifndef NO_IPV6
    if (addr->ais && p_freeaddrinfo)
	p_freeaddrinfo(addr->ais);
#endif
    if (addr->addresses)
	sfree(addr->addresses);
    sfree(addr);
}

SockAddr sk_addr_dup(SockAddr addr)
{
    addr->refcount++;
    return addr;
}

static Plug sk_tcp_plug(Socket sock, Plug p)
{
    Actual_Socket s = (Actual_Socket) sock;
    Plug ret = s->plug;
    if (p)
	s->plug = p;
    return ret;
}

static void sk_tcp_flush(Socket s)
{
    /*
     * We send data to the socket as soon as we can anyway,
     * so we don't need to do anything here.  :-)
     */
}

static void sk_tcp_close(Socket s);
static int sk_tcp_write(Socket s, const char *data, int len);
static int sk_tcp_write_oob(Socket s, const char *data, int len);
static void sk_tcp_write_eof(Socket s);
static void sk_tcp_set_frozen(Socket s, int is_frozen);
static const char *sk_tcp_socket_error(Socket s);

extern char *do_select(SOCKET skt, int startup);

static Socket sk_tcp_accept(accept_ctx_t ctx, Plug plug)
{
    static const struct socket_function_table fn_table = {
	sk_tcp_plug,
	sk_tcp_close,
	sk_tcp_write,
	sk_tcp_write_oob,
	sk_tcp_write_eof,
	sk_tcp_flush,
	sk_tcp_set_frozen,
	sk_tcp_socket_error
    };

    DWORD err;
    char *errstr;
    Actual_Socket ret;

    /*
     * Create Socket structure.
     */
    ret = snew(struct Socket_tag);
    ret->fn = &fn_table;
    ret->error = NULL;
    ret->plug = plug;
    bufchain_init(&ret->output_data);
    ret->writable = 1;		       /* to start with */
    ret->sending_oob = 0;
    ret->outgoingeof = EOF_NO;
    ret->frozen = 1;
    ret->frozen_readable = 0;
    ret->localhost_only = 0;	       /* unused, but best init anyway */
    ret->pending_error = 0;
    ret->parent = ret->child = NULL;
    ret->addr = NULL;

    ret->s = (SOCKET)ctx.p;

    if (ret->s == INVALID_SOCKET) {
	err = p_WSAGetLastError();
	ret->error = winsock_error_string(err);
	return (Socket) ret;
    }

    ret->oobinline = 0;

    /* Set up a select mechanism. This could be an AsyncSelect on a
     * window, or an EventSelect on an event object. */
    errstr = do_select(ret->s, 1);
    if (errstr) {
	ret->error = errstr;
	return (Socket) ret;
    }

    add234(sktree, ret);

    return (Socket) ret;
}

static DWORD try_connect(Actual_Socket sock)
{
    SOCKET s;
#ifndef NO_IPV6
    SOCKADDR_IN6 a6;
#endif
    SOCKADDR_IN a;
    DWORD err;
    char *errstr;
    short localport;
    int family;

    if (sock->s != INVALID_SOCKET) {
	do_select(sock->s, 0);
        p_closesocket(sock->s);
    }

    plug_log(sock->plug, 0, sock->addr, sock->port, NULL, 0);

    /*
     * Open socket.
     */
    family = SOCKADDR_FAMILY(sock->addr, sock->step);

    /*
     * Remove the socket from the tree before we overwrite its
     * internal socket id, because that forms part of the tree's
     * sorting criterion. We'll add it back before exiting this
     * function, whether we changed anything or not.
     */
    del234(sktree, sock);

    s = p_socket(family, SOCK_STREAM, 0);
    sock->s = s;

    if (s == INVALID_SOCKET) {
	err = p_WSAGetLastError();
	sock->error = winsock_error_string(err);
	goto ret;
    }

    if (sock->oobinline) {
	BOOL b = TRUE;
	p_setsockopt(s, SOL_SOCKET, SO_OOBINLINE, (void *) &b, sizeof(b));
    }

    if (sock->nodelay) {
	BOOL b = TRUE;
	p_setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (void *) &b, sizeof(b));
    }

    if (sock->keepalive) {
	BOOL b = TRUE;
	p_setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (void *) &b, sizeof(b));
    }

    /*
     * Bind to local address.
     */
    if (sock->privport)
	localport = 1023;	       /* count from 1023 downwards */
    else
	localport = 0;		       /* just use port 0 (ie winsock picks) */

    /* Loop round trying to bind */
    while (1) {
	int sockcode;

#ifndef NO_IPV6
	if (family == AF_INET6) {
	    memset(&a6, 0, sizeof(a6));
	    a6.sin6_family = AF_INET6;
          /*a6.sin6_addr = in6addr_any; */ /* == 0 done by memset() */
	    a6.sin6_port = p_htons(localport);
	} else
#endif
	{
	    a.sin_family = AF_INET;
	    a.sin_addr.s_addr = p_htonl(INADDR_ANY);
	    a.sin_port = p_htons(localport);
	}
#ifndef NO_IPV6
	sockcode = p_bind(s, (family == AF_INET6 ?
			      (struct sockaddr *) &a6 :
			      (struct sockaddr *) &a),
			  (family == AF_INET6 ? sizeof(a6) : sizeof(a)));
#else
	sockcode = p_bind(s, (struct sockaddr *) &a, sizeof(a));
#endif
	if (sockcode != SOCKET_ERROR) {
	    err = 0;
	    break;		       /* done */
	} else {
	    err = p_WSAGetLastError();
	    if (err != WSAEADDRINUSE)  /* failed, for a bad reason */
		break;
	}

	if (localport == 0)
	    break;		       /* we're only looping once */
	localport--;
	if (localport == 0)
	    break;		       /* we might have got to the end */
    }

    if (err) {
	sock->error = winsock_error_string(err);
	goto ret;
    }

    /*
     * Connect to remote address.
     */
#ifndef NO_IPV6
    if (sock->step.ai) {
	if (family == AF_INET6) {
	    a6.sin6_family = AF_INET6;
	    a6.sin6_port = p_htons((short) sock->port);
	    a6.sin6_addr =
		((struct sockaddr_in6 *) sock->step.ai->ai_addr)->sin6_addr;
	    a6.sin6_flowinfo = ((struct sockaddr_in6 *) sock->step.ai->ai_addr)->sin6_flowinfo;
	    a6.sin6_scope_id = ((struct sockaddr_in6 *) sock->step.ai->ai_addr)->sin6_scope_id;
	} else {
	    a.sin_family = AF_INET;
	    a.sin_addr =
		((struct sockaddr_in *) sock->step.ai->ai_addr)->sin_addr;
	    a.sin_port = p_htons((short) sock->port);
	}
    } else
#endif
    {
	assert(sock->addr->addresses && sock->step.curraddr < sock->addr->naddresses);
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = p_htonl(sock->addr->addresses[sock->step.curraddr]);
	a.sin_port = p_htons((short) sock->port);
    }

    /* Set up a select mechanism. This could be an AsyncSelect on a
     * window, or an EventSelect on an event object. */
    errstr = do_select(s, 1);
    if (errstr) {
	sock->error = errstr;
	err = 1;
	goto ret;
    }

    if ((
#ifndef NO_IPV6
	    p_connect(s,
		      ((family == AF_INET6) ? (struct sockaddr *) &a6 :
		       (struct sockaddr *) &a),
		      (family == AF_INET6) ? sizeof(a6) : sizeof(a))
#else
	    p_connect(s, (struct sockaddr *) &a, sizeof(a))
#endif
	) == SOCKET_ERROR) {
	err = p_WSAGetLastError();
	/*
	 * We expect a potential EWOULDBLOCK here, because the
	 * chances are the front end has done a select for
	 * FD_CONNECT, so that connect() will complete
	 * asynchronously.
	 */
	if ( err != WSAEWOULDBLOCK ) {
	    sock->error = winsock_error_string(err);
	    goto ret;
	}
    } else {
	/*
	 * If we _don't_ get EWOULDBLOCK, the connect has completed
	 * and we should set the socket as writable.
	 */
	sock->writable = 1;
    }

    err = 0;

    ret:

    /*
     * No matter what happened, put the socket back in the tree.
     */
    add234(sktree, sock);

    if (err)
	plug_log(sock->plug, 1, sock->addr, sock->port, sock->error, err);
    return err;
}

Socket sk_new(SockAddr addr, int port, int privport, int oobinline,
	      int nodelay, int keepalive, Plug plug)
{
    static const struct socket_function_table fn_table = {
	sk_tcp_plug,
	sk_tcp_close,
	sk_tcp_write,
	sk_tcp_write_oob,
	sk_tcp_write_eof,
	sk_tcp_flush,
	sk_tcp_set_frozen,
	sk_tcp_socket_error
    };

    Actual_Socket ret;
    DWORD err;

    /*
     * Create Socket structure.
     */
    ret = snew(struct Socket_tag);
    ret->fn = &fn_table;
    ret->error = NULL;
    ret->plug = plug;
    bufchain_init(&ret->output_data);
    ret->connected = 0;		       /* to start with */
    ret->writable = 0;		       /* to start with */
    ret->sending_oob = 0;
    ret->outgoingeof = EOF_NO;
    ret->frozen = 0;
    ret->frozen_readable = 0;
    ret->localhost_only = 0;	       /* unused, but best init anyway */
    ret->pending_error = 0;
    ret->parent = ret->child = NULL;
    ret->oobinline = oobinline;
    ret->nodelay = nodelay;
    ret->keepalive = keepalive;
    ret->privport = privport;
    ret->port = port;
    ret->addr = addr;
    START_STEP(ret->addr, ret->step);
    ret->s = INVALID_SOCKET;

    err = 0;
    do {
        err = try_connect(ret);
    } while (err && sk_nextaddr(ret->addr, &ret->step));

    return (Socket) ret;
}

Socket sk_newlistener(char *srcaddr, int port, Plug plug, int local_host_only,
		      int orig_address_family)
{
    static const struct socket_function_table fn_table = {
	sk_tcp_plug,
	sk_tcp_close,
	sk_tcp_write,
	sk_tcp_write_oob,
	sk_tcp_write_eof,
	sk_tcp_flush,
	sk_tcp_set_frozen,
	sk_tcp_socket_error
    };

    SOCKET s;
#ifndef NO_IPV6
    SOCKADDR_IN6 a6;
#endif
    SOCKADDR_IN a;

    DWORD err;
    char *errstr;
    Actual_Socket ret;
    int retcode;
    int on = 1;

    int address_family;

    /*
     * Create Socket structure.
     */
    ret = snew(struct Socket_tag);
    ret->fn = &fn_table;
    ret->error = NULL;
    ret->plug = plug;
    bufchain_init(&ret->output_data);
    ret->writable = 0;		       /* to start with */
    ret->sending_oob = 0;
    ret->outgoingeof = EOF_NO;
    ret->frozen = 0;
    ret->frozen_readable = 0;
    ret->localhost_only = local_host_only;
    ret->pending_error = 0;
    ret->parent = ret->child = NULL;
    ret->addr = NULL;

    /*
     * Translate address_family from platform-independent constants
     * into local reality.
     */
    address_family = (orig_address_family == ADDRTYPE_IPV4 ? AF_INET :
#ifndef NO_IPV6
		      orig_address_family == ADDRTYPE_IPV6 ? AF_INET6 :
#endif
		      AF_UNSPEC);

    /*
     * Our default, if passed the `don't care' value
     * ADDRTYPE_UNSPEC, is to listen on IPv4. If IPv6 is supported,
     * we will also set up a second socket listening on IPv6, but
     * the v4 one is primary since that ought to work even on
     * non-v6-supporting systems.
     */
    if (address_family == AF_UNSPEC) address_family = AF_INET;

    /*
     * Open socket.
     */
    s = p_socket(address_family, SOCK_STREAM, 0);
    ret->s = s;

    if (s == INVALID_SOCKET) {
	err = p_WSAGetLastError();
	ret->error = winsock_error_string(err);
	return (Socket) ret;
    }

    ret->oobinline = 0;

    p_setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));

#ifndef NO_IPV6
	if (address_family == AF_INET6) {
	    memset(&a6, 0, sizeof(a6));
	    a6.sin6_family = AF_INET6;
	    if (local_host_only)
		a6.sin6_addr = in6addr_loopback;
	    else
		a6.sin6_addr = in6addr_any;
            if (srcaddr != NULL && p_getaddrinfo) {
                struct addrinfo hints;
                struct addrinfo *ai;
                int err;

                memset(&hints, 0, sizeof(hints));
                hints.ai_family = AF_INET6;
                hints.ai_flags = 0;
                {
                    /* strip [] on IPv6 address literals */
                    char *trimmed_addr = host_strduptrim(srcaddr);
                    err = p_getaddrinfo(trimmed_addr, NULL, &hints, &ai);
                    sfree(trimmed_addr);
                }
                if (err == 0 && ai->ai_family == AF_INET6) {
                    a6.sin6_addr =
                        ((struct sockaddr_in6 *)ai->ai_addr)->sin6_addr;
                }
            }
	    a6.sin6_port = p_htons(port);
	} else
#endif
	{
	    int got_addr = 0;
	    a.sin_family = AF_INET;

	    /*
	     * Bind to source address. First try an explicitly
	     * specified one...
	     */
	    if (srcaddr) {
		a.sin_addr.s_addr = p_inet_addr(srcaddr);
		if (a.sin_addr.s_addr != INADDR_NONE) {
		    /* Override localhost_only with specified listen addr. */
		    ret->localhost_only = ipv4_is_loopback(a.sin_addr);
		    got_addr = 1;
		}
	    }

	    /*
	     * ... and failing that, go with one of the standard ones.
	     */
	    if (!got_addr) {
		if (local_host_only)
		    a.sin_addr.s_addr = p_htonl(INADDR_LOOPBACK);
		else
		    a.sin_addr.s_addr = p_htonl(INADDR_ANY);
	    }

	    a.sin_port = p_htons((short)port);
	}
#ifndef NO_IPV6
	retcode = p_bind(s, (address_family == AF_INET6 ?
			   (struct sockaddr *) &a6 :
			   (struct sockaddr *) &a),
		       (address_family ==
			AF_INET6 ? sizeof(a6) : sizeof(a)));
#else
	retcode = p_bind(s, (struct sockaddr *) &a, sizeof(a));
#endif
	if (retcode != SOCKET_ERROR) {
	    err = 0;
	} else {
	    err = p_WSAGetLastError();
	}

    if (err) {
	p_closesocket(s);
	ret->error = winsock_error_string(err);
	return (Socket) ret;
    }


    if (p_listen(s, SOMAXCONN) == SOCKET_ERROR) {
        p_closesocket(s);
	ret->error = winsock_error_string(p_WSAGetLastError());
	return (Socket) ret;
    }

    /* Set up a select mechanism. This could be an AsyncSelect on a
     * window, or an EventSelect on an event object. */
    errstr = do_select(s, 1);
    if (errstr) {
	p_closesocket(s);
	ret->error = errstr;
	return (Socket) ret;
    }

    add234(sktree, ret);

#ifndef NO_IPV6
    /*
     * If we were given ADDRTYPE_UNSPEC, we must also create an
     * IPv6 listening socket and link it to this one.
     */
    if (address_family == AF_INET && orig_address_family == ADDRTYPE_UNSPEC) {
	Actual_Socket other;

	other = (Actual_Socket) sk_newlistener(srcaddr, port, plug,
					       local_host_only, ADDRTYPE_IPV6);

	if (other) {
	    if (!other->error) {
		other->parent = ret;
		ret->child = other;
	    } else {
		sfree(other);
	    }
	}
    }
#endif

    return (Socket) ret;
}

static void sk_tcp_close(Socket sock)
{
    extern char *do_select(SOCKET skt, int startup);
    Actual_Socket s = (Actual_Socket) sock;

    if (s->child)
	sk_tcp_close((Socket)s->child);

    del234(sktree, s);
    do_select(s->s, 0);
    p_closesocket(s->s);
    if (s->addr)
	sk_addr_free(s->addr);
    sfree(s);
}

/*
 * Deal with socket errors detected in try_send().
 */
static void socket_error_callback(void *vs)
{
    Actual_Socket s = (Actual_Socket)vs;

    /*
     * Just in case other socket work has caused this socket to vanish
     * or become somehow non-erroneous before this callback arrived...
     */
    if (!find234(sktree, s, NULL) || !s->pending_error)
        return;

    /*
     * An error has occurred on this socket. Pass it to the plug.
     */
    plug_closing(s->plug, winsock_error_string(s->pending_error),
                 s->pending_error, 0);
}

/*
 * The function which tries to send on a socket once it's deemed
 * writable.
 */
void try_send(Actual_Socket s)
{
    while (s->sending_oob || bufchain_size(&s->output_data) > 0) {
	int nsent;
	DWORD err;
	void *data;
	int len, urgentflag;

	if (s->sending_oob) {
	    urgentflag = MSG_OOB;
	    len = s->sending_oob;
	    data = &s->oobdata;
	} else {
	    urgentflag = 0;
	    bufchain_prefix(&s->output_data, &data, &len);
	}
	nsent = p_send(s->s, data, len, urgentflag);
	noise_ultralight(nsent);
	if (nsent <= 0) {
	    err = (nsent < 0 ? p_WSAGetLastError() : 0);
	    if ((err < WSABASEERR && nsent < 0) || err == WSAEWOULDBLOCK) {
		/*
		 * Perfectly normal: we've sent all we can for the moment.
		 * 
		 * (Some WinSock send() implementations can return
		 * <0 but leave no sensible error indication -
		 * WSAGetLastError() is called but returns zero or
		 * a small number - so we check that case and treat
		 * it just like WSAEWOULDBLOCK.)
		 */
		s->writable = FALSE;
		return;
	    } else if (nsent == 0 ||
		       err == WSAECONNABORTED || err == WSAECONNRESET) {
		/*
		 * If send() returns CONNABORTED or CONNRESET, we
		 * unfortunately can't just call plug_closing(),
		 * because it's quite likely that we're currently
		 * _in_ a call from the code we'd be calling back
		 * to, so we'd have to make half the SSH code
		 * reentrant. Instead we flag a pending error on
		 * the socket, to be dealt with (by calling
		 * plug_closing()) at some suitable future moment.
		 */
		s->pending_error = err;
                queue_toplevel_callback(socket_error_callback, s);
		return;
	    } else {
		/* We're inside the Windows frontend here, so we know
		 * that the frontend handle is unnecessary. */
		logevent(NULL, winsock_error_string(err));
		fatalbox("%s", winsock_error_string(err));
	    }
	} else {
	    if (s->sending_oob) {
		if (nsent < len) {
		    memmove(s->oobdata, s->oobdata+nsent, len-nsent);
		    s->sending_oob = len - nsent;
		} else {
		    s->sending_oob = 0;
		}
	    } else {
		bufchain_consume(&s->output_data, nsent);
	    }
	}
    }

    /*
     * If we reach here, we've finished sending everything we might
     * have needed to send. Send EOF, if we need to.
     */
    if (s->outgoingeof == EOF_PENDING) {
        p_shutdown(s->s, SD_SEND);
        s->outgoingeof = EOF_SENT;
    }
}

static int sk_tcp_write(Socket sock, const char *buf, int len)
{
    Actual_Socket s = (Actual_Socket) sock;

    assert(s->outgoingeof == EOF_NO);

    /*
     * Add the data to the buffer list on the socket.
     */
    bufchain_add(&s->output_data, buf, len);

    /*
     * Now try sending from the start of the buffer list.
     */
    if (s->writable)
	try_send(s);

    return bufchain_size(&s->output_data);
}

static int sk_tcp_write_oob(Socket sock, const char *buf, int len)
{
    Actual_Socket s = (Actual_Socket) sock;

    assert(s->outgoingeof == EOF_NO);

    /*
     * Replace the buffer list on the socket with the data.
     */
    bufchain_clear(&s->output_data);
    assert(len <= sizeof(s->oobdata));
    memcpy(s->oobdata, buf, len);
    s->sending_oob = len;

    /*
     * Now try sending from the start of the buffer list.
     */
    if (s->writable)
	try_send(s);

    return s->sending_oob;
}

static void sk_tcp_write_eof(Socket sock)
{
    Actual_Socket s = (Actual_Socket) sock;

    assert(s->outgoingeof == EOF_NO);

    /*
     * Mark the socket as pending outgoing EOF.
     */
    s->outgoingeof = EOF_PENDING;

    /*
     * Now try sending from the start of the buffer list.
     */
    if (s->writable)
	try_send(s);
}

int select_result(WPARAM wParam, LPARAM lParam)
{
    int ret, open;
    DWORD err;
    char buf[20480];		       /* nice big buffer for plenty of speed */
    Actual_Socket s;
    u_long atmark;

    /* wParam is the socket itself */

    if (wParam == 0)
	return 1;		       /* boggle */

    s = find234(sktree, (void *) wParam, cmpforsearch);
    if (!s)
	return 1;		       /* boggle */

    if ((err = WSAGETSELECTERROR(lParam)) != 0) {
	/*
	 * An error has occurred on this socket. Pass it to the
	 * plug.
	 */
	if (s->addr) {
	    plug_log(s->plug, 1, s->addr, s->port,
		     winsock_error_string(err), err);
	    while (s->addr && sk_nextaddr(s->addr, &s->step)) {
		err = try_connect(s);
	    }
	}
	if (err != 0)
	    return plug_closing(s->plug, winsock_error_string(err), err, 0);
	else
	    return 1;
    }

    noise_ultralight(lParam);

    switch (WSAGETSELECTEVENT(lParam)) {
      case FD_CONNECT:
	s->connected = s->writable = 1;
	/*
	 * Once a socket is connected, we can stop falling
	 * back through the candidate addresses to connect
	 * to.
	 */
	if (s->addr) {
	    sk_addr_free(s->addr);
	    s->addr = NULL;
	}
	break;
      case FD_READ:
	/* In the case the socket is still frozen, we don't even bother */
	if (s->frozen) {
	    s->frozen_readable = 1;
	    break;
	}

	/*
	 * We have received data on the socket. For an oobinline
	 * socket, this might be data _before_ an urgent pointer,
	 * in which case we send it to the back end with type==1
	 * (data prior to urgent).
	 */
	if (s->oobinline) {
	    atmark = 1;
	    p_ioctlsocket(s->s, SIOCATMARK, &atmark);
	    /*
	     * Avoid checking the return value from ioctlsocket(),
	     * on the grounds that some WinSock wrappers don't
	     * support it. If it does nothing, we get atmark==1,
	     * which is equivalent to `no OOB pending', so the
	     * effect will be to non-OOB-ify any OOB data.
	     */
	} else
	    atmark = 1;

	ret = p_recv(s->s, buf, sizeof(buf), 0);
	noise_ultralight(ret);
	if (ret < 0) {
	    err = p_WSAGetLastError();
	    if (err == WSAEWOULDBLOCK) {
		break;
	    }
	}
	if (ret < 0) {
	    return plug_closing(s->plug, winsock_error_string(err), err,
				0);
	} else if (0 == ret) {
	    return plug_closing(s->plug, NULL, 0, 0);
	} else {
	    return plug_receive(s->plug, atmark ? 0 : 1, buf, ret);
	}
	break;
      case FD_OOB:
	/*
	 * This will only happen on a non-oobinline socket. It
	 * indicates that we can immediately perform an OOB read
	 * and get back OOB data, which we will send to the back
	 * end with type==2 (urgent data).
	 */
	ret = p_recv(s->s, buf, sizeof(buf), MSG_OOB);
	noise_ultralight(ret);
	if (ret <= 0) {
	    char *str = (ret == 0 ? "Internal networking trouble" :
			 winsock_error_string(p_WSAGetLastError()));
	    /* We're inside the Windows frontend here, so we know
	     * that the frontend handle is unnecessary. */
	    logevent(NULL, str);
	    fatalbox("%s", str);
	} else {
	    return plug_receive(s->plug, 2, buf, ret);
	}
	break;
      case FD_WRITE:
	{
	    int bufsize_before, bufsize_after;
	    s->writable = 1;
	    bufsize_before = s->sending_oob + bufchain_size(&s->output_data);
	    try_send(s);
	    bufsize_after = s->sending_oob + bufchain_size(&s->output_data);
	    if (bufsize_after < bufsize_before)
		plug_sent(s->plug, bufsize_after);
	}
	break;
      case FD_CLOSE:
	/* Signal a close on the socket. First read any outstanding data. */
	open = 1;
	do {
	    ret = p_recv(s->s, buf, sizeof(buf), 0);
	    if (ret < 0) {
		err = p_WSAGetLastError();
		if (err == WSAEWOULDBLOCK)
		    break;
		return plug_closing(s->plug, winsock_error_string(err),
				    err, 0);
	    } else {
		if (ret)
		    open &= plug_receive(s->plug, 0, buf, ret);
		else
		    open &= plug_closing(s->plug, NULL, 0, 0);
	    }
	} while (ret > 0);
	return open;
       case FD_ACCEPT:
	{
#ifdef NO_IPV6
	    struct sockaddr_in isa;
#else
            struct sockaddr_storage isa;
#endif
	    int addrlen = sizeof(isa);
	    SOCKET t;  /* socket of connection */
            accept_ctx_t actx;

	    memset(&isa, 0, sizeof(isa));
	    err = 0;
	    t = p_accept(s->s,(struct sockaddr *)&isa,&addrlen);
	    if (t == INVALID_SOCKET)
	    {
		err = p_WSAGetLastError();
		if (err == WSATRY_AGAIN)
		    break;
	    }

            actx.p = (void *)t;

#ifndef NO_IPV6
            if (isa.ss_family == AF_INET &&
                s->localhost_only &&
                !ipv4_is_local_addr(((struct sockaddr_in *)&isa)->sin_addr))
#else
	    if (s->localhost_only && !ipv4_is_local_addr(isa.sin_addr))
#endif
	    {
		p_closesocket(t);      /* dodgy WinSock let nonlocal through */
	    } else if (plug_accepting(s->plug, sk_tcp_accept, actx)) {
		p_closesocket(t);      /* denied or error */
	    }
	}
    }

    return 1;
}

/*
 * Special error values are returned from sk_namelookup and sk_new
 * if there's a problem. These functions extract an error message,
 * or return NULL if there's no problem.
 */
const char *sk_addr_error(SockAddr addr)
{
    return addr->error;
}
static const char *sk_tcp_socket_error(Socket sock)
{
    Actual_Socket s = (Actual_Socket) sock;
    return s->error;
}

static void sk_tcp_set_frozen(Socket sock, int is_frozen)
{
    Actual_Socket s = (Actual_Socket) sock;
    if (s->frozen == is_frozen)
	return;
    s->frozen = is_frozen;
    if (!is_frozen) {
	do_select(s->s, 1);
	if (s->frozen_readable) {
	    char c;
	    p_recv(s->s, &c, 1, MSG_PEEK);
	}
    }
    s->frozen_readable = 0;
}

void socket_reselect_all(void)
{
    Actual_Socket s;
    int i;

    for (i = 0; (s = index234(sktree, i)) != NULL; i++) {
	if (!s->frozen)
	    do_select(s->s, 1);
    }
}

/*
 * For Plink: enumerate all sockets currently active.
 */
SOCKET first_socket(int *state)
{
    Actual_Socket s;
    *state = 0;
    s = index234(sktree, (*state)++);
    return s ? s->s : INVALID_SOCKET;
}

SOCKET next_socket(int *state)
{
    Actual_Socket s = index234(sktree, (*state)++);
    return s ? s->s : INVALID_SOCKET;
}

extern int socket_writable(SOCKET skt)
{
    Actual_Socket s = find234(sktree, (void *)skt, cmpforsearch);

    if (s)
	return bufchain_size(&s->output_data) > 0;
    else
	return 0;
}

int net_service_lookup(char *service)
{
    struct servent *se;
    se = p_getservbyname(service, NULL);
    if (se != NULL)
	return p_ntohs(se->s_port);
    else
	return 0;
}

char *get_hostname(void)
{
    int len = 128;
    char *hostname = NULL;
    do {
	len *= 2;
	hostname = sresize(hostname, len, char);
	if (p_gethostname(hostname, len) < 0) {
	    sfree(hostname);
	    hostname = NULL;
	    break;
	}
    } while (strlen(hostname) >= (size_t)(len-1));
    return hostname;
}

SockAddr platform_get_x11_unix_address(const char *display, int displaynum,
				       char **canonicalname)
{
    SockAddr ret = snew(struct SockAddr_tag);
    memset(ret, 0, sizeof(struct SockAddr_tag));
    ret->error = "unix sockets not supported on this platform";
    ret->refcount = 1;
    return ret;
}
