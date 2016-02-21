#pragma once

#ifndef GET_SAFE_STRING
#define GET_SAFE_STRING(str) ( (str)?(str):_T("") )
#endif
#define HANDLE_IS_VALID(h) ( (HANDLE)(h)!=NULL && (HANDLE)(h)!=INVALID_HANDLE_VALUE )

CString FormatDateTime ( COleDateTime &DateTime, LPCTSTR pFormat );
int hwGetFileAttr ( LPCTSTR lpFileName, OUT CFileStatus *pFileStatus=NULL );
CString FormatBytes ( double fBytesNum, BOOL bShowUnit=TRUE, int nFlag=0 );
BOOL WaitForThreadEnd ( HANDLE *phThread, DWORD dwWaitTime=10*1000 );

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.