// OneNetAdapterInfo.cpp: implementation of the COneNetAdapterInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "SpeedPostEmail.h"
#include "NetAdapterInfo.h"
#include "Iphlpapi.h"
#include "IpTypes.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define MALLOC( bytes )			::HeapAlloc( ::GetProcessHeap(), HEAP_ZERO_MEMORY, (bytes) )
#define FREE( ptr )				if( ptr ) ::HeapFree( ::GetProcessHeap(), 0, ptr )
#define REMALLOC( ptr, bytes )	::HeapReAlloc( ::GetProcessHeap(), HEAP_ZERO_MEMORY, ptr, bytes )

#pragma comment ( lib, "iphlpapi.lib" )

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
COneNetAdapterInfo::COneNetAdapterInfo ( IP_ADAPTER_INFO *pAdptInfo )
{
	m_bInitOk = FALSE;
	memset ( &m_PhysicalAddress, 0, sizeof(m_PhysicalAddress) );
	m_nPhysicalAddressLength = 0;
	ASSERT ( pAdptInfo );
	memcpy ( &m_AdptInfo, pAdptInfo, sizeof(IP_ADAPTER_INFO) );
	if ( !Init() )
	{
		TRACE ( _T("[%s - %s] initialize failed."), Get_Name(), Get_Desc() );
	}
}

COneNetAdapterInfo::~COneNetAdapterInfo()
{
	
}

//
// 根据传入的 pAdptInfo 信息来获取指定网卡的基本信息
//
BOOL COneNetAdapterInfo::Init ()
{
	IP_ADDR_STRING* pNext			= NULL;
	IP_PER_ADAPTER_INFO* pPerAdapt	= NULL;
	ULONG ulLen						= 0;
	DWORD dwErr = ERROR_SUCCESS;
	ASSERT ( m_AdptInfo.AddressLength > 0 );
	t_IPINFO iphold;

	// 将变量清空
	m_bInitOk = FALSE;
	m_csName.Empty ();
	m_csDesc.Empty ();
	m_CurIPInfo.csIP.Empty ();
	m_CurIPInfo.csSubnet.Empty ();
	m_Ary_IP.RemoveAll ();
	m_Ary_DNS.RemoveAll ();
	m_Ary_Gateway.RemoveAll ();
	
#ifndef _UNICODE
	m_csName			= m_AdptInfo.AdapterName;
	m_csDesc			= m_AdptInfo.Description;
#else
	USES_CONVERSION;
	m_csName			= A2W ( m_AdptInfo.AdapterName );
	m_csDesc			= A2W ( m_AdptInfo.Description );
#endif
	
	// 获取当前正在使用的IP地址
	if ( m_AdptInfo.CurrentIpAddress )
	{
		m_CurIPInfo.csIP		= m_AdptInfo.CurrentIpAddress->IpAddress.String;
		m_CurIPInfo.csSubnet	= m_AdptInfo.CurrentIpAddress->IpMask.String;
	}
	else
	{
		m_CurIPInfo.csIP		= _T("0.0.0.0");
		m_CurIPInfo.csSubnet	= _T("0.0.0.0");
	}
	
	// 获取本网卡中所有的IP地址
	pNext = &( m_AdptInfo.IpAddressList );
	while ( pNext )
	{
		iphold.csIP		= pNext->IpAddress.String;
		iphold.csSubnet	= pNext->IpMask.String;
		m_Ary_IP.Add ( iphold );
		pNext = pNext->Next;
	}
	
	// 获取本网卡中所有的网关信息
	pNext = &( m_AdptInfo.GatewayList );
	while ( pNext )
	{
		m_Ary_Gateway.Add ( A2W( pNext->IpAddress.String ));
		pNext = pNext->Next;
	}
	
	// 获取本网卡中所有的 DNS
	dwErr = ::GetPerAdapterInfo ( m_AdptInfo.Index, pPerAdapt, &ulLen );
	if( dwErr == ERROR_BUFFER_OVERFLOW )
	{
		pPerAdapt = ( IP_PER_ADAPTER_INFO* ) MALLOC( ulLen );
		dwErr = ::GetPerAdapterInfo( m_AdptInfo.Index, pPerAdapt, &ulLen );
		
		// if we succeed than we need to drop into our loop
		// and fill the dns array will all available IP
		// addresses.
		if( dwErr == ERROR_SUCCESS )
		{
			pNext = &( pPerAdapt->DnsServerList );
			while( pNext )
			{
				m_Ary_DNS.Add( A2W( pNext->IpAddress.String ) );
				pNext = pNext->Next;
			}				
			m_bInitOk = TRUE;
		}

		// this is done outside the dwErr == ERROR_SUCCES just in case. the macro
		// uses NULL pointer checking so it is ok if pPerAdapt was never allocated.
		FREE( pPerAdapt );
	}

	return m_bInitOk;
}

//
// 释放或刷新本网卡的IP地址
//
BOOL COneNetAdapterInfo::RenewReleaseIP( Func_OperateIP func )
{	
	IP_INTERFACE_INFO*	pInfo	= NULL;
	BOOL bDidIt					= FALSE;
	ULONG ulLen					= 0;
	int nNumInterfaces			= 0;
	int nCnt					= 0;
	DWORD dwErr					= ERROR_SUCCESS;

	dwErr = ::GetInterfaceInfo ( pInfo, &ulLen );
	if( dwErr == ERROR_INSUFFICIENT_BUFFER )
	{
		pInfo = ( IP_INTERFACE_INFO* ) MALLOC( ulLen );
		dwErr = ::GetInterfaceInfo ( pInfo, &ulLen );
		
		if( dwErr != ERROR_SUCCESS )
		{
			return FALSE;			
		}
	}
	
	// we can assume from here out that we have a valid array
	// of IP_INTERFACE_INFO structures due to the error 
	// checking one above.
	nNumInterfaces = ulLen / sizeof( IP_INTERFACE_INFO );
	for( nCnt = 0; nCnt < nNumInterfaces; nCnt++ )
	{
		if( pInfo[ nCnt ].Adapter[ 0 ].Index == m_AdptInfo.Index )
		{
			dwErr = func( &pInfo[ nCnt ].Adapter[ 0 ] );
			
			// free all used memory since we don't need it any more.
			FREE( pInfo );	
			
			bDidIt = ( dwErr == NO_ERROR );			
			if( ! bDidIt ) {				
				return FALSE;
			}
			
			break;
		}
	}			
	
	return bDidIt;
}

////////////////////////////////////////////////////////////
//	Desc:
//		Releases the addresses held by this adapter.
////////////////////////////////////////////////////////////
BOOL COneNetAdapterInfo::ReleaseIP()
{	
	return RenewReleaseIP ( ::IpReleaseAddress );
}

////////////////////////////////////////////////////////////
//	Desc:
//		Renews the address being held by this adapter.
////////////////////////////////////////////////////////////
BOOL COneNetAdapterInfo::RenewIP()
{
	return RenewReleaseIP ( ::IpRenewAddress );
}

CString COneNetAdapterInfo::GetAdapterTypeString ()
{
	UINT nType = m_AdptInfo.Type;
	CString csType = _T("");
	switch( nType )
	{
	case MIB_IF_TYPE_OTHER:		csType = _T("Other");			break;
	case MIB_IF_TYPE_ETHERNET:	csType = _T("Ethernet");		break; 
	case MIB_IF_TYPE_TOKENRING:	csType = _T("Token Ring");		break; 
	case MIB_IF_TYPE_FDDI:		csType = _T("FDDI");			break; 
	case MIB_IF_TYPE_PPP:		csType = _T("PPP");				break; 
	case MIB_IF_TYPE_LOOPBACK:	csType = _T("Loopback");		break; 
	case MIB_IF_TYPE_SLIP:		csType = _T("SLIP");			break; 	
	default: csType = _T("Invalid Adapter Type");				break;
	};
	
	return csType;
}

time_t COneNetAdapterInfo::Get_LeaseObtained()				const {	return m_AdptInfo.LeaseObtained; }
time_t COneNetAdapterInfo::Get_LeaseExpired()				const {	return m_AdptInfo.LeaseExpires; }
int	COneNetAdapterInfo::Get_IPCount()						const {	return (int)m_Ary_IP.GetSize(); }
int	COneNetAdapterInfo::Get_DNSCount()						const { return (int)m_Ary_DNS.GetSize(); }
CString COneNetAdapterInfo::Get_CurrentIP()					const { return m_CurIPInfo.csIP; }
BOOL COneNetAdapterInfo::Is_DHCP_Used()						const { return m_AdptInfo.DhcpEnabled; }
CString	COneNetAdapterInfo::Get_DHCPAddr()					const {	return m_AdptInfo.DhcpServer.IpAddress.String; }
BOOL COneNetAdapterInfo::Is_Wins_Used()						const { return m_AdptInfo.HaveWins; }
CString COneNetAdapterInfo::Get_PrimaryWinsServer()			const { return m_AdptInfo.PrimaryWinsServer.IpAddress.String; }
CString COneNetAdapterInfo::Get_SecondaryWinsServer()		const { return m_AdptInfo.SecondaryWinsServer.IpAddress.String; }
int	COneNetAdapterInfo::Get_GatewayCount()					const { return m_Ary_Gateway.GetSize(); }
DWORD COneNetAdapterInfo::Get_AdapterIndex()				const {	return m_AdptInfo.Index; }
UINT COneNetAdapterInfo::Get_AdapterType()					const { return m_AdptInfo.Type; }

CString	COneNetAdapterInfo::Get_IPAddr ( int nIndex ) const
{	
	CString csAddr = _T("");
	if ( nIndex >= 0 && nIndex < m_Ary_IP.GetSize() )
	{
        csAddr = m_Ary_IP.GetAt(nIndex).csIP;
	}
	
	return csAddr;
}

CString COneNetAdapterInfo::Get_Subnet ( int nIndex ) const
{ 
	CString csAddr = _T("");
	if ( nIndex >= 0 && nIndex < m_Ary_IP.GetSize() )
	{
        csAddr = m_Ary_IP.GetAt(nIndex).csSubnet;
	}
	
	return csAddr;
}

CString	COneNetAdapterInfo::Get_DNSAddr ( int nIndex ) const
{	
	CString csAddr = _T("");
	if ( nIndex >= 0 && nIndex < m_Ary_DNS.GetSize() )
	{
        csAddr = m_Ary_DNS.GetAt(nIndex);
	}
	
	return csAddr;
}

CString	COneNetAdapterInfo::Get_GatewayAddr ( int nIndex ) const
{	
	CString csAddr = _T("");
	if ( nIndex >= 0 && nIndex < m_Ary_Gateway.GetSize() )
	{
        csAddr = m_Ary_Gateway.GetAt(nIndex);
	}
	
	return csAddr;
}

void COneNetAdapterInfo::Set_PhysicalAddress ( int nPhysicalAddressLength, BYTE *pPhysicalAddress )
{
	if ( !pPhysicalAddress ) return;
	m_nPhysicalAddressLength = __min(nPhysicalAddressLength,sizeof(m_PhysicalAddress));
	memcpy ( m_PhysicalAddress, pPhysicalAddress, m_nPhysicalAddressLength );
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CNetAdapterInfo::CNetAdapterInfo ()
{
}

CNetAdapterInfo::~CNetAdapterInfo()
{
	DeleteAllNetAdapterInfo ();
}

void CNetAdapterInfo::DeleteAllNetAdapterInfo()
{
	for ( int i=0; i<m_Ary_NetAdapterInfo.GetSize(); i++ )
	{
		COneNetAdapterInfo *pOneNetAdapterInfo = (COneNetAdapterInfo*)m_Ary_NetAdapterInfo.GetAt(i);
		if ( pOneNetAdapterInfo ) delete pOneNetAdapterInfo;
	}
	m_Ary_NetAdapterInfo.RemoveAll ();
}

//
// 枚举网络适配器
// return : ------------------------------------------------------------
//	-1	-	失败
//	>=0	-	网络适配器数量
//
int CNetAdapterInfo::EnumNetworkAdapters ()
{
	DeleteAllNetAdapterInfo ();

	IP_ADAPTER_INFO* pAdptInfo	= NULL;
	IP_ADAPTER_INFO* pNextAd	= NULL;
	ULONG ulLen					= 0;
	int nCnt					= 0;
	
	DWORD dwError = ::GetAdaptersInfo ( pAdptInfo, &ulLen );
	if( dwError != ERROR_BUFFER_OVERFLOW ) return -1;
	pAdptInfo = ( IP_ADAPTER_INFO* )MALLOC ( ulLen );
	dwError = ::GetAdaptersInfo( pAdptInfo, &ulLen );
	if ( dwError != ERROR_SUCCESS ) return -1;
	
	pNextAd = pAdptInfo;
	while( pNextAd )
	{
		COneNetAdapterInfo *pOneNetAdapterInfo = new COneNetAdapterInfo ( pNextAd );
		if ( pOneNetAdapterInfo )
		{
			m_Ary_NetAdapterInfo.Add ( pOneNetAdapterInfo );
		}
		nCnt ++;
		pNextAd = pNextAd->Next;
	}
	
	// free any memory we allocated from the heap before
	// exit.  we wouldn't wanna leave memory leaks now would we? ;p
	FREE( pAdptInfo );		
	
	return nCnt;
}

COneNetAdapterInfo* CNetAdapterInfo::Get_OneNetAdapterInfo ( int nIndex )
{
	if ( nIndex < 0 || nIndex >= m_Ary_NetAdapterInfo.GetSize() )
	{
		ASSERT ( FALSE );
		return NULL;
	}

	return (COneNetAdapterInfo*)m_Ary_NetAdapterInfo.GetAt(nIndex);
}

COneNetAdapterInfo* CNetAdapterInfo::Get_OneNetAdapterInfo ( DWORD dwIndex )
{
	for ( int i=0; i<m_Ary_NetAdapterInfo.GetSize(); i++ )
	{
		COneNetAdapterInfo *pOneNetAdapterInfo = (COneNetAdapterInfo*)m_Ary_NetAdapterInfo.GetAt(i);
		if ( pOneNetAdapterInfo && pOneNetAdapterInfo->Get_AdapterIndex() == dwIndex )
			return pOneNetAdapterInfo;
	}

	return NULL;
}

void CNetAdapterInfo::Refresh ()
{
	DeleteAllNetAdapterInfo ();
	EnumNetworkAdapters ();
	GetAdapterAddress ();
}

//
// 获取网卡的地址信息，主要是MAC地址
//
BOOL CNetAdapterInfo::GetAdapterAddress ()
{
	DWORD dwSize = 0;
    DWORD dwRetVal = 0;
	BOOL bRet = FALSE;

    int i = 0;

    // Set the flags to pass to GetAdaptersAddresses
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;

    // default to unspecified address family (both)
	// AF_INET for IPv4, AF_INET6 for IPv6
    ULONG family = AF_UNSPEC;

    LPVOID lpMsgBuf = NULL;

    PIP_ADAPTER_ADDRESSES pAddresses = NULL;
    ULONG outBufLen = 0;

    PIP_ADAPTER_ADDRESSES pCurrAddresses = NULL;
    PIP_ADAPTER_UNICAST_ADDRESS pUnicast = NULL;
    PIP_ADAPTER_ANYCAST_ADDRESS pAnycast = NULL;
    PIP_ADAPTER_MULTICAST_ADDRESS pMulticast = NULL;
    IP_ADAPTER_DNS_SERVER_ADDRESS *pDnServer = NULL;
    IP_ADAPTER_PREFIX *pPrefix = NULL;

    outBufLen = sizeof (IP_ADAPTER_ADDRESSES);
    pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
    if (pAddresses == NULL) return FALSE;

    // Make an initial call to GetAdaptersAddresses to get the 
    // size needed into the outBufLen variable
    if ( ::GetAdaptersAddresses ( family, flags, NULL, pAddresses, &outBufLen ) == ERROR_BUFFER_OVERFLOW )
	{
        FREE(pAddresses);
        pAddresses = (IP_ADAPTER_ADDRESSES *) MALLOC(outBufLen);
    }
    if (pAddresses == NULL) return FALSE;

    // Make a second call to GetAdapters Addresses to get the
    // actual data we want
#ifdef _DEBUG
    TRACE ( _T("Memory allocated for GetAdapterAddresses = %d bytes\n"), outBufLen);
    TRACE ( _T("Calling GetAdaptersAddresses function with family = " ) );
    if (family == AF_INET)
        TRACE(_T("AF_INET\n"));
    if (family == AF_INET6)
        TRACE(_T("AF_INET6\n"));
    if (family == AF_UNSPEC)
        TRACE(_T("AF_UNSPEC\n\n"));
#endif
    dwRetVal = GetAdaptersAddresses ( family, flags, NULL, pAddresses, &outBufLen );

    if ( dwRetVal == NO_ERROR )
	{
        // If successful, output some information from the data we received
        pCurrAddresses = pAddresses;
        while (pCurrAddresses)
		{
            TRACE(_T("\tLength of the IP_ADAPTER_ADDRESS struct: %ld\n"),
                   pCurrAddresses->Length);
            TRACE(_T("\tIfIndex (IPv4 interface): %u\n"), pCurrAddresses->IfIndex);
            TRACE(_T("\tAdapter name: %s\n"), pCurrAddresses->AdapterName);

            pUnicast = pCurrAddresses->FirstUnicastAddress;
            if (pUnicast != NULL)
			{
                for (i = 0; pUnicast != NULL; i++)
                    pUnicast = pUnicast->Next;
                TRACE(_T("\tNumber of Unicast Addresses: %d\n"), i);
            }
			else
			{
                TRACE(_T("\tNo Unicast Addresses\n"));
			}

            pAnycast = pCurrAddresses->FirstAnycastAddress;
            if (pAnycast)
			{
                for (i = 0; pAnycast != NULL; i++)
                    pAnycast = pAnycast->Next;
                TRACE(_T("\tNumber of Anycast Addresses: %d\n"), i);
            }
			else
			{
                TRACE(_T("\tNo Anycast Addresses\n"));
			}

            pMulticast = pCurrAddresses->FirstMulticastAddress;
            if (pMulticast)
			{
                for (i = 0; pMulticast != NULL; i++)
                    pMulticast = pMulticast->Next;
                TRACE(_T("\tNumber of Multicast Addresses: %d\n"), i);
            }
			else
			{
                TRACE(_T("\tNo Multicast Addresses\n"));
			}

            pDnServer = pCurrAddresses->FirstDnsServerAddress;
            if (pDnServer)
			{
                for (i = 0; pDnServer != NULL; i++)
                    pDnServer = pDnServer->Next;
                TRACE(_T("\tNumber of DNS Server Addresses: %d\n"), i);
            }
			else
			{
                TRACE(_T("\tNo DNS Server Addresses\n"));
			}

            TRACE(_T("\tDNS Suffix: %wS\n"), pCurrAddresses->DnsSuffix);
            TRACE(_T("\tDescription: %wS\n"), pCurrAddresses->Description);
            TRACE(_T("\tFriendly name: %wS\n"), pCurrAddresses->FriendlyName);

            if (pCurrAddresses->PhysicalAddressLength != 0)
			{
                TRACE(_T("\tPhysical address: "));
                for (i = 0; i < (int) pCurrAddresses->PhysicalAddressLength; i++)
				{
                    if ( i == (int)(pCurrAddresses->PhysicalAddressLength - 1))
                        TRACE(_T("%.2X\n"), (int) pCurrAddresses->PhysicalAddress[i]);
                    else
                        TRACE(_T("%.2X-"), (int) pCurrAddresses->PhysicalAddress[i]);
                }

				COneNetAdapterInfo* pNetAdapterInfo = Get_OneNetAdapterInfo ( pCurrAddresses->IfIndex );
				if ( pNetAdapterInfo )
				{
					pNetAdapterInfo->Set_PhysicalAddress ( pCurrAddresses->PhysicalAddressLength, pCurrAddresses->PhysicalAddress );
				}
            }
            TRACE(_T("\tFlags: %ld\n"), pCurrAddresses->Flags);
            TRACE(_T("\tMtu: %lu\n"), pCurrAddresses->Mtu);
            TRACE(_T("\tIfType: %ld\n"), pCurrAddresses->IfType);
            TRACE(_T("\tOperStatus: %ld\n"), pCurrAddresses->OperStatus);
            TRACE(_T("\tIpv6IfIndex (IPv6 interface): %u\n"),
                   pCurrAddresses->Ipv6IfIndex);
            TRACE(_T("\tZoneIndices (hex): "));
            for (i = 0; i < 16; i++)
                TRACE(_T("%lx "), pCurrAddresses->ZoneIndices[i]);
            TRACE(_T("\n"));

            pPrefix = pCurrAddresses->FirstPrefix;
            if (pPrefix)
			{
                for (i = 0; pPrefix != NULL; i++)
                    pPrefix = pPrefix->Next;
                TRACE(_T("\tNumber of IP Adapter Prefix entries: %d\n"), i);
            }
			else
			{
                TRACE(_T("\tNumber of IP Adapter Prefix entries: 0\n"));
			}

            TRACE(_T("\n"));

            pCurrAddresses = pCurrAddresses->Next;
			bRet = TRUE;
        }
    }
	else
	{
        TRACE(_T("Call to GetAdaptersAddresses failed with error: %d\n"), dwRetVal );
        if (dwRetVal == ERROR_NO_DATA)
		{
            TRACE(_T("\tNo addresses were found for the requested parameters\n"));
		}
        else
		{
            if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, dwRetVal, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),   // Default language
                              (LPTSTR) & lpMsgBuf, 0, NULL))
			{
                TRACE(_T("\tError: %s"), lpMsgBuf);
                LocalFree(lpMsgBuf);
                FREE(pAddresses);
            }
        }
		bRet = FALSE;
    }
    FREE(pAddresses);

	return bRet;
}

CString MacAddress2String ( int nPhysicalAddressLength, BYTE *pPhysicalAddress )
{
	ASSERT ( nPhysicalAddressLength >= 6 );
	AfxIsValidAddress(pPhysicalAddress,nPhysicalAddressLength,TRUE);
	CString csMacAddress, csOneCell;
	for ( int i=0; i<nPhysicalAddressLength; i++ )
	{
		csOneCell.Format ( _T("%.2X"), (int)pPhysicalAddress[i] );
		if ( !csMacAddress.IsEmpty() ) csMacAddress += _T("-");
		csMacAddress += csOneCell;
	}

	return csMacAddress;
}