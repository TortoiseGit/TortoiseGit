// Base64.h: interface for the CBase64 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Base64_H__E435B968_4992_4795_8AED_B6E55FC89045__INCLUDED_)
#define AFX_Base64_H__E435B968_4992_4795_8AED_B6E55FC89045__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CBase64  
{
public:
	CBase64();
	virtual ~CBase64();

	CStringA Encode( IN const char* szEncoding, IN int nSize );
	int Decode ( IN const char* szDecoding, char* szOutput );

protected:
	void write_bits( UINT nBits, int nNumBts, LPSTR szOutput, int& lp );
	UINT read_bits( int nNumBits, int* pBitsRead, int& lp );

	int m_nInputSize;
	int m_nBitsRemaining;
	ULONG m_lBitStorage;
	LPCSTR m_szInput;
private:
};

#endif // !defined(AFX_Base64_H__E435B968_4992_4795_8AED_B6E55FC89045__INCLUDED_)
