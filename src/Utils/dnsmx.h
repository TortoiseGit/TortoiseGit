//DNS Query

/*		说明
	1、在 InitInstance() 函数中加上以下代码：
WSADATA wsaData;
int err = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
if ( err )
{
	return FALSE;
}

	2、在 ExitInstance() 函数中加上以下代码：
	WSACleanup();

*/

#include <Afxtempl.h>
#include "NetAdapterInfo.h"

typedef struct _MXHostInfo
{
	char szMXHost[1024];
	int N;
} t_MXHostInfo;
typedef CArray<t_MXHostInfo,t_MXHostInfo&> t_Ary_MXHostInfos;

BOOL GetMX (
			char *pszQuery,							// 要查询的域名
			OUT t_Ary_MXHostInfos &Ary_MXHostInfos	// 输出 Mail Exchange 主机名
			);

