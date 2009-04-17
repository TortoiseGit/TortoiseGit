#pragma once

#ifndef GET_SAFE_STRING
#define GET_SAFE_STRING(str) ( (str)?(str):_T("") )
#endif
// _vsnprintf 函数
#ifndef _vsnprintf_hw
#ifdef UNICODE
#define _vsnprintf_hw _vsnwprintf
#else
#define _vsnprintf_hw _vsnprintf
#endif
#endif
#define HANDLE_IS_VALID(h) ( (HANDLE)(h)!=NULL && (HANDLE)(h)!=INVALID_HANDLE_VALUE )

template<class T>
int FindFromStaticArray ( IN T *pAry, IN int nArySize, IN T Find )
{
	if ( !pAry ) return -1;
	for ( int i=0; i<nArySize; i++ )
	{
		if ( pAry[i] == Find )
			return i;
	}
	return -1;
}

//
// 注意：如果是从 CString 中查找时 Find 千万不要用 LPCTSTR 或者 char* 变量，一定是要用 CString 变量
//
template<class T1, class T2>
int FindFromArray ( IN T1 &Ary, IN T2 Find )
{
	int nCount = (int)Ary.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		T2 tGetValue = Ary.GetAt(i);
		if ( tGetValue == Find )
			return i;
	}
	return -1;
}

//
// 从数组 Ary_Org 中查找，只要 Ary_Find 中任何一个元素在 Ary_Org 中出现过
// 就返回该元素在 Ary_Org 中的位置
//
template<class T1, class T2>
int FindFromArray ( IN T1 &Ary_Org, IN T1 &Ary_Find, OUT T2 &Element )
{
	int nCount = Ary_Find.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		T2 tGetValue = Ary_Find.GetAt(i);
		int nFindPos = FindFromArray ( Ary_Org, tGetValue );
		if ( nFindPos >= 0 )
		{
			Element = Ary_Org.GetAt ( nFindPos );
			return nFindPos;
		}
	}
	return -1;
}

template<class T1, class T2, class T3, class T4>
int FindFromArray ( IN T1 &Ary, IN T2 Find, IN T3 &AppAry, IN T4 AppFind )
{
	int nCount = Ary.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		if ( Ary.GetAt(i) == Find && 
			AppAry.GetAt(i) == AppFind )
		{
			return i;
		}
	}
	return -1;
}

template<class T1>
int FindFromArray ( IN T1 &Ary_Src, IN T1 &Ary_Find )
{
	int nCount = Ary_Src.GetSize();
	for ( int i=0; i<nCount; i++ )
	{
		if ( FindFromArray ( Ary_Find, Ary_Src.GetAt(i) ) >= 0 )
			return i;
	}
	return -1;
}

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
CString FormatString ( LPCTSTR lpszStr, ... );
int hwGetFileAttr ( LPCTSTR lpFileName, OUT CFileStatus *pFileStatus=NULL );
CString FormatBytes ( double fBytesNum, BOOL bShowUnit=TRUE, int nFlag=0 );
BOOL WaitForThreadEnd ( HANDLE *phThread, DWORD dwWaitTime=10*1000 );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.