#pragma once

#ifndef GET_SAFE_STRING
#define GET_SAFE_STRING(str) ( (str)?(str):_T("") )
#endif
// _vsnprintf 函数
#ifndef _vsnprintf_hw
#ifdef UNICODE
#define _vsnprintf_hw _vsnwprintf_s
#else
#define _vsnprintf_hw _vsnprintf_s
#endif
#endif
#define HANDLE_IS_VALID(h) ( (HANDLE)(h)!=NULL && (HANDLE)(h)!=INVALID_HANDLE_VALUE )

enum
{
	STRING_IS_MULTICHARS = 0,		// 是多字节字符串
		STRING_IS_UNICODE,				// 是UNICODE字符串
		STRING_IS_SOFTCODE,				// 是和程序一样的字符串编码
};

class CMultiByteString
{
public:
	CMultiByteString ( LPCTSTR lpszOrg, int nOrgStringEncodeType=STRING_IS_SOFTCODE, OUT char *pOutBuf=NULL, int nOutBufSize=0 );
	~CMultiByteString ();
	char *GetBuffer()
	{
		if ( m_pszData ) return m_pszData;
		return "";
	}
	// 获取字符串个数
	int GetLength()
	{
		return m_nCharactersNumber;
	}
	// 获取字符串占用内存大小（字节数，包括字符串结束的'\0'所占的位置）
	int GetSize()
	{
		return m_nDataSize;
	}
private:
	char *m_pszData;			// 保存目标字符串的缓冲
	int m_nDataSize;			// 目标字符串占用的内存大小（字节数，包括字符串结束的'\0'）
	int m_nCharactersNumber;	// 目标字符串的个数
	BOOL m_bNewBuffer;			// 是否在这个类中申请了内存
};

CString GetCompatibleString ( LPVOID lpszOrg, BOOL bOrgIsUnicode, int nOrgLength=-1 );
CString FormatDateTime ( COleDateTime &DateTime, LPCTSTR pFormat );
int hwGetFileAttr ( LPCTSTR lpFileName, OUT CFileStatus *pFileStatus=NULL );
CString FormatBytes ( double fBytesNum, BOOL bShowUnit=TRUE, int nFlag=0 );
BOOL WaitForThreadEnd ( HANDLE *phThread, DWORD dwWaitTime=10*1000 );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.