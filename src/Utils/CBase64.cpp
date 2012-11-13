// Base64.cpp: implementation of the CBase64 class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CBase64.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// CBase64
//////////////////////////////////////////////////////////////////////
// Static Member Initializers
//

// The 7-bit alphabet used to encode binary information
const char m_sBase64Alphabet[] =  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

const int f_nMask[] = { 0, 1, 3, 7, 15, 31, 63, 127, 255 };

CBase64::CBase64()
{
	m_lBitStorage = 0;
	m_nBitsRemaining = 0;
	m_nInputSize = 0;
	m_szInput = NULL;
}

CBase64::~CBase64()
{
}

CStringA CBase64::Encode( IN const char* szEncoding, IN int nSize )
{
	CStringA sOutput = "";
	int nNumBits = 6;
	UINT nDigit;
	int lp = 0;
	int count = 0;

	ASSERT( szEncoding != NULL );
	if( szEncoding == NULL )
		return sOutput;
	m_szInput = szEncoding;
	m_nInputSize = nSize;

	m_nBitsRemaining = 0;
	nDigit = read_bits( nNumBits, &nNumBits, lp );
	while( nNumBits > 0 )
	{
		sOutput += m_sBase64Alphabet[ (int)nDigit ];
		nDigit = read_bits( nNumBits, &nNumBits, lp );
		count++;

		if(count % 80 == 0)
			sOutput += '\n';
	}
	// Pad with '=' as per RFC 1521
	while( sOutput.GetLength() % 4 != 0 )
	{
		sOutput += '=';
	}
	return sOutput;
}

// The size of the output buffer must not be less than
// 3/4 the size of the input buffer. For simplicity,
// make them the same size.
// return : 解码后数据长度
int CBase64::Decode ( IN const char* szDecoding, char* szOutput )
{
	CString sInput;
    int c, lp =0;
	int nDigit;
    int nDecode[ 256 ];

	ASSERT( szDecoding != NULL );
	ASSERT( szOutput != NULL );
	if( szOutput == NULL )
		return 0;
	if( szDecoding == NULL )
		return 0;
	sInput = szDecoding;
	if( sInput.GetLength() == 0 )
		return 0;

	// Build Decode Table
	//
	int i;
	for( i = 0; i < 256; i++ )
		nDecode[i] = -2; // Illegal digit
	for( i=0; i < 64; i++ )
	{
		nDecode[ m_sBase64Alphabet[ i ] ] = i;
		nDecode[ m_sBase64Alphabet[ i ] | 0x80 ] = i; // Ignore 8th bit
		nDecode[ '\r' ] = -1;
		nDecode[ '\n' ] = -1;
		nDecode[ '=' ] = -1;
		nDecode[ '=' | 0x80 ] = -1; // Ignore MIME padding char
    }

	// Clear the output buffer
	memset( szOutput, 0, sInput.GetLength() + 1 );

	// Decode the Input
	//
	for( lp = 0, i = 0; lp < sInput.GetLength(); lp++ )
	{
		c = sInput[ lp ];
		nDigit = nDecode[ c & 0x7F ];
		if( nDigit < -1 )
		{
			return 0;
		}
		else if( nDigit >= 0 )
			// i (index into output) is incremented by write_bits()
			write_bits( nDigit & 0x3F, 6, szOutput, i );
    }
	return i;
}

UINT CBase64::read_bits(int nNumBits, int * pBitsRead, int& lp)
{
    ULONG lScratch;
    while( ( m_nBitsRemaining < nNumBits ) &&
		   ( lp < m_nInputSize ) )
	{
		int c = m_szInput[ lp++ ];
        m_lBitStorage <<= 8;
        m_lBitStorage |= (c & 0xff);
		m_nBitsRemaining += 8;
    }
    if( m_nBitsRemaining < nNumBits )
	{
		lScratch = m_lBitStorage << ( nNumBits - m_nBitsRemaining );
		*pBitsRead = m_nBitsRemaining;
		m_nBitsRemaining = 0;
    }
	else
	{
		lScratch = m_lBitStorage >> ( m_nBitsRemaining - nNumBits );
		*pBitsRead = nNumBits;
		m_nBitsRemaining -= nNumBits;
    }
    return (UINT)lScratch & f_nMask[nNumBits];
}


void CBase64::write_bits(UINT nBits,
						 int nNumBits,
						 LPSTR szOutput,
						 int& i)
{
	UINT nScratch;

	m_lBitStorage = (m_lBitStorage << nNumBits) | nBits;
	m_nBitsRemaining += nNumBits;
	while( m_nBitsRemaining > 7 )
	{
		nScratch = m_lBitStorage >> (m_nBitsRemaining - 8);
		szOutput[ i++ ] = nScratch & 0xFF;
		m_nBitsRemaining -= 8;
	}
}