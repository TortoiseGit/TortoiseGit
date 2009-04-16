// OneNetAdapterInfo.h: interface for the COneNetAdapterInfo class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_OneNetAdapterInfo_H__A899410F_5CFF_4958_80C4_D1AC693F62E3__INCLUDED_)
#define AFX_OneNetAdapterInfo_H__A899410F_5CFF_4958_80C4_D1AC693F62E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/*
		说明
	获取网卡信息，包括IP地址、DNS、网关、是否为动态IP、DHCP等，该类不用读取注册表即可获取最新的网卡信息
另外还可以释放网卡地址并重新刷新地址
*/

#include <winsock2.h>
#include <iphlpapi.h>
#include <Afxtempl.h>

typedef struct _IPINFO
{
	CString csIP;
	CString csSubnet;
} t_IPINFO;
typedef CArray<t_IPINFO,t_IPINFO&> t_Ary_IPINFO;

typedef DWORD ( __stdcall *Func_OperateIP)( PIP_ADAPTER_INDEX_MAP AdapterInfo );
CString MacAddress2String ( int nPhysicalAddressLength, BYTE *pPhysicalAddress );

class COneNetAdapterInfo
{
public:
	COneNetAdapterInfo ( IP_ADAPTER_INFO *pAdptInfo );
	virtual ~COneNetAdapterInfo();
	CString Get_Name() { return m_csName; }
	CString Get_Desc() { return m_csDesc; }
	BOOL ReleaseIP();
	BOOL RenewIP();
	CString GetAdapterTypeString ();

	time_t Get_LeaseObtained() const;
	time_t Get_LeaseExpired() const;
	int	Get_IPCount() const;
	int	Get_DNSCount() const;
	CString Get_CurrentIP() const;
	BOOL Is_DHCP_Used() const;
	CString	Get_DHCPAddr() const;
	BOOL Is_Wins_Used() const;
	CString Get_PrimaryWinsServer() const;
	CString Get_SecondaryWinsServer() const;
	int	Get_GatewayCount() const;
	DWORD Get_AdapterIndex() const;
	UINT Get_AdapterType() const;
	CString	Get_IPAddr ( int nIndex ) const;
	CString Get_Subnet ( int nIndex ) const;
	CString	Get_DNSAddr ( int nIndex ) const;
	CString	Get_GatewayAddr ( int nIndex ) const;
	void Set_PhysicalAddress ( int nPhysicalAddressLength, BYTE *pPhysicalAddress );
	CString Get_PhysicalAddressStr () { return MacAddress2String ( m_nPhysicalAddressLength, m_PhysicalAddress ); }

private:
	BOOL Init ();
	BOOL RenewReleaseIP( Func_OperateIP func );

public:
	BOOL m_bInitOk;
private:
	IP_ADAPTER_INFO		m_AdptInfo;
	CString				m_csName;
	CString				m_csDesc;
	t_IPINFO			m_CurIPInfo;	// this is also in the ip address list but this is the address currently active.	
	t_Ary_IPINFO		m_Ary_IP;
	CStringArray		m_Ary_DNS;
	CStringArray		m_Ary_Gateway;
	BYTE				m_PhysicalAddress[MAX_ADAPTER_ADDRESS_LENGTH];
	int					m_nPhysicalAddressLength;
};

class CNetAdapterInfo
{
public:
	CNetAdapterInfo ();
	~CNetAdapterInfo();
	int GetNetCardCount () { return m_Ary_NetAdapterInfo.GetSize(); }
	COneNetAdapterInfo* Get_OneNetAdapterInfo ( int nIndex );
	COneNetAdapterInfo* CNetAdapterInfo::Get_OneNetAdapterInfo ( DWORD dwIndex );
	void Refresh ();

private:
	int EnumNetworkAdapters ();
	void DeleteAllNetAdapterInfo();
	BOOL GetAdapterAddress ();

private:
	CPtrArray m_Ary_NetAdapterInfo;
};

#endif // !defined(AFX_OneNetAdapterInfo_H__A899410F_5CFF_4958_80C4_D1AC693F62E3__INCLUDED_)
