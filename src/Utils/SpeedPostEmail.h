#pragma once

#ifndef GET_SAFE_STRING
#define GET_SAFE_STRING(str) ( (str)?(str):_T("") )
#endif
// _vsnprintf ����
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
	STRING_IS_MULTICHARS = 0,		// �Ƕ��ֽ��ַ���
		STRING_IS_UNICODE,				// ��UNICODE�ַ���
		STRING_IS_SOFTCODE,				// �Ǻͳ���һ�����ַ�������
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
	// ��ȡ�ַ�������
	int GetLength()
	{
		return m_nCharactersNumber;
	}
	// ��ȡ�ַ���ռ���ڴ��С���ֽ����������ַ���������'\0'��ռ��λ�ã�
	int GetSize()
	{
		return m_nDataSize;
	}
private:
	char *m_pszData;			// ����Ŀ���ַ����Ļ���
	int m_nDataSize;			// Ŀ���ַ���ռ�õ��ڴ��С���ֽ����������ַ���������'\0'��
	int m_nCharactersNumber;	// Ŀ���ַ����ĸ���
	BOOL m_bNewBuffer;			// �Ƿ�����������������ڴ�
};

CString GetCompatibleString ( LPVOID lpszOrg, BOOL bOrgIsUnicode, int nOrgLength=-1 );
CString FormatDateTime ( COleDateTime &DateTime, LPCTSTR pFormat );
CString FormatString ( LPCTSTR lpszStr, ... );
int hwGetFileAttr ( LPCTSTR lpFileName, OUT CFileStatus *pFileStatus=NULL );
CString FormatBytes ( double fBytesNum, BOOL bShowUnit=TRUE, int nFlag=0 );
BOOL WaitForThreadEnd ( HANDLE *phThread, DWORD dwWaitTime=10*1000 );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.