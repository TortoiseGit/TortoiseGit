// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2007-2012 - TortoiseSVN

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "stdafx.h"
#include "resource.h"
#include "UnicodeUtils.h"
#include "registry.h"
#include "filetextlines.h"
#include "FormatMessageWrapper.h"
#include "SmartHandle.h"

wchar_t inline WideCharSwap(wchar_t nValue)
{
	return (((nValue>> 8)) | (nValue << 8));
	//return _byteswap_ushort(nValue);
}

UINT64 inline WordSwapBytes(UINT64 nValue)
{
	return ((nValue&0xff00ff00ff00ff)<<8) | ((nValue>>8)&0xff00ff00ff00ff); // swap BYTESs in WORDs
}

UINT32 inline DwordSwapBytes(UINT32 nValue)
{
	UINT32 nRet = (nValue<<16) | (nValue>>16); // swap WORDs
	nRet = ((nRet&0xff00ff)<<8) | ((nRet>>8)&0xff00ff); // swap BYTESs in WORDs
	return nRet;
	//return _byteswap_ulong(nValue);
}

UINT64 inline DwordSwapBytes(UINT64 nValue)
{
	UINT64 nRet = ((nValue&0xffff0000ffffL)<<16) | ((nValue>>16)&0xffff0000ffffL); // swap WORDs in DWORDs
	nRet = ((nRet&0xff00ff00ff00ff)<<8) | ((nRet>>8)&0xff00ff00ff00ff); // swap BYTESs in WORDs
	return nRet;
}

CFileTextLines::CFileTextLines(void)
	: m_UnicodeType(CFileTextLines::AUTOTYPE)
	, m_LineEndings(EOL_AUTOLINE)
	, m_bNeedsConversion(false)
{
}

CFileTextLines::~CFileTextLines(void)
{
}

CFileTextLines::UnicodeType CFileTextLines::CheckUnicodeType(LPVOID pBuffer, int cb)
{
	if (cb < 2)
		return CFileTextLines::ASCII;
	const UINT32 * const pVal32 = (UINT32 *)pBuffer;
	const UINT16 * const pVal16 = (UINT16 *)pBuffer;
	const UINT8 * const pVal8 = (UINT8 *)pBuffer;
	// scan the whole buffer for a 0x00000000 sequence
	// if found, we assume a binary file
	int nDwords = cb/4;
	for (int i=0; i<nDwords; ++i)
	{
		if (0x00000000 == pVal32[i])
			return CFileTextLines::BINARY;
	}
	if (cb >=4 )
	{
		if (*pVal32 == 0x0000FEFF)
		{
			return CFileTextLines::UTF32_LE;
		}
		if (*pVal32 == 0xFFFE0000)
		{
			return CFileTextLines::UTF32_BE;
		}
	}
	if (*pVal16 == 0xFEFF)
	{
		return CFileTextLines::UTF16_LE;
	}
	if (*pVal16 == 0xFFFE)
	{
		return CFileTextLines::UTF16_BE;
	}
	if (cb < 3)
		return CFileTextLines::ASCII;
	if (*pVal16 == 0xBBEF)
	{
		if (pVal8[2] == 0xBF)
			return CFileTextLines::UTF8BOM;
	}
	// check for illegal UTF8 sequences
	bool bNonANSI = false;
	int nNeedData = 0;
	int i=0;
	// run fast for ascii
	for (; i<cb; i+=8)
	{
		if ((*(UINT64 *)&pVal8[i] & 0x8080808080808080)!=0) // all Ascii?
		{
			bNonANSI = true;
			break;
		}
	}
	// continue slow
	for (; i<cb; ++i)
	{
		UINT8 zChar = pVal8[i];
		if ((zChar & 0x80)==0) // Ascii
		{
			if (nNeedData)
			{
				return CFileTextLines::ASCII;
			}
			continue;
		}
		if ((zChar & 0x40)==0) // top bit
		{
			if (!nNeedData)
				return CFileTextLines::ASCII;
			--nNeedData;
		}
		else if (nNeedData)
		{
			return CFileTextLines::ASCII;
		}
		else if ((zChar & 0x20)==0) // top two bits
		{
			if (zChar<=0xC1)
				return CFileTextLines::ASCII;
			nNeedData = 1;
		}
		else if ((zChar & 0x10)==0) // top three bits
		{
			nNeedData = 2;
		}
		else if ((zChar & 0x08)==0) // top four bits
		{
			if (zChar>=0xf5)
				return CFileTextLines::ASCII;
			nNeedData = 3;
		}
		else
			return CFileTextLines::ASCII;
	}
	if (bNonANSI && nNeedData==0)
		// if get here thru nonAscii and no missing data left then its valid UTF8
		return CFileTextLines::UTF8;
	if ((!bNonANSI)&&(DWORD(CRegDWORD(_T("Software\\TortoiseGitMerge\\UseUTF8"), FALSE))))
		return CFileTextLines::UTF8;
	return CFileTextLines::ASCII;
}


BOOL CFileTextLines::Load(const CString& sFilePath, int lengthHint /* = 0*/)
{
	WCHAR exceptionError[1000] = {0};
	m_LineEndings = EOL_AUTOLINE;
	m_UnicodeType = CFileTextLines::AUTOTYPE;
	RemoveAll();
	if(lengthHint != 0)
	{
		Reserve(lengthHint);
	}

	if (PathIsDirectory(sFilePath))
	{
		m_sErrorString.Format(IDS_ERR_FILE_NOTAFILE, (LPCTSTR)sFilePath);
		return FALSE;
	}

	if (!PathFileExists(sFilePath))
	{
		//file does not exist, so just return SUCCESS
		return TRUE;
	}

	CAutoFile hFile = CreateFile(sFilePath, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, NULL, NULL);
	if (!hFile)
	{
		SetErrorString();
		return FALSE;
	}

	LARGE_INTEGER fsize;
	if (!GetFileSizeEx(hFile, &fsize))
	{
		SetErrorString();
		return FALSE;
	}
	if (fsize.HighPart)
	{
		// file is way too big for us
		m_sErrorString.LoadString(IDS_ERR_FILE_TOOBIG);
		return FALSE;
	}

	// create buffer
	// If new[] was done for type T delete[] must be called on a pointer of type T*,
	// otherwise the behavior is undefined.
	// +1 is to address possible truncation when integer division is done
	CBuffer oFile;
	try
	{
		oFile.SetLength(fsize.LowPart);
	}
	catch (CMemoryException* e)
	{
		e->GetErrorMessage(exceptionError, _countof(exceptionError));
		m_sErrorString = exceptionError;
		return FALSE;
	}

	// load file
	DWORD dwReadBytes = 0;
	if (!ReadFile(hFile, (void *)oFile, fsize.LowPart, &dwReadBytes, NULL))
	{
		SetErrorString();
		return FALSE;
	}
	hFile.CloseHandle();

	// detect type
	if (m_UnicodeType == CFileTextLines::AUTOTYPE)
	{
		m_UnicodeType = this->CheckUnicodeType((LPVOID)oFile, dwReadBytes);
		// enforce conversion for all but ASCII and UTF8 type
		m_bNeedsConversion = (m_UnicodeType!=CFileTextLines::UTF8)&&(m_UnicodeType!=CFileTextLines::ASCII);
	}

	// we may have to convert the file content - CString is UTF16LE
	try
	{
		CBaseFilter * pFilter = NULL;
		switch (m_UnicodeType)
		{
		case BINARY:
			m_sErrorString.Format(IDS_ERR_FILE_BINARY, (LPCTSTR)sFilePath);
			return FALSE;
		case UTF8:
		case UTF8BOM:
			pFilter = new CUtf8Filter(NULL);
			break;
		default:
		case ASCII:
			pFilter = new CAsciiFilter(NULL);
			break;
		case UTF16_BE:
			pFilter = new CUtf16beFilter(NULL);
			break;
		case UTF16_LE:
			pFilter = new CUtf16leFilter(NULL);
			break;
		case UTF32_BE:
			pFilter = new CUtf32beFilter(NULL);
			break;
		case UTF32_LE:
			pFilter = new CUtf32leFilter(NULL);
			break;
		}
		pFilter->Decode(oFile);
		delete pFilter;
	}
	catch (CMemoryException* e)
	{
		e->GetErrorMessage(exceptionError, _countof(exceptionError));
		m_sErrorString = exceptionError;
		return FALSE;
	}

	int nReadChars=oFile.GetLength()/sizeof(wchar_t);
	wchar_t * pTextBuf = (wchar_t *)oFile;
	wchar_t * pLineStart = pTextBuf;
	if ((m_UnicodeType == UTF8BOM)
		|| (m_UnicodeType == UTF16_LE)
		|| (m_UnicodeType == UTF16_BE)
		|| (m_UnicodeType == UTF32_LE)
		|| (m_UnicodeType == UTF32_BE))
	{
		// ignore the BOM
		++pTextBuf;
		++pLineStart;
		--nReadChars;
	}

	// fill in the lines into the array
	size_t countEOLs[EOL__COUNT];
	memset(countEOLs, 0, sizeof(countEOLs));
	CFileTextLine oTextLine;
	for (int i = nReadChars; i; --i)
	{
		EOL eEol;
		switch (*pTextBuf++)
		{
		case '\r':
			// crlf line ending or cr line ending
			eEol = ((i > 1) && *(pTextBuf) == '\n') ? EOL_CRLF : EOL_CR;
			break;
		case '\n':
			// lfcr line ending or lf line ending
			eEol = ((i > 1) && *(pTextBuf) == '\r') ? EOL_LFCR : EOL_LF;
			break;
		case 0x000b:
			eEol = EOL_VT;
			break;
		case 0x000c:
			eEol = EOL_FF;
			break;
		case 0x0085:
			eEol = EOL_NEL;
			break;
		case 0x2028:
			eEol = EOL_LS;
			break;
		case 0x2029:
			eEol = EOL_PS;
			break;
		default:
			continue;
		}
		oTextLine.sLine = CString(pLineStart, (int)(pTextBuf-pLineStart)-1);
		oTextLine.eEnding = eEol;
		Add(oTextLine);
		++countEOLs[eEol];
		if (eEol==EOL_CRLF || eEol==EOL_LFCR)
		{
			++pTextBuf;
			--i;
		}
		pLineStart = pTextBuf;
	}
	CString line(pLineStart, (int)(pTextBuf-pLineStart));
	Add(line, EOL_NOENDING);

	// some EOLs are not supported by the svn diff lib.
	m_bNeedsConversion |= (countEOLs[EOL_CRLF]!=0);
	m_bNeedsConversion |= (countEOLs[EOL_FF]!=0);
	m_bNeedsConversion |= (countEOLs[EOL_VT]!=0);
	m_bNeedsConversion |= (countEOLs[EOL_NEL]!=0);
	m_bNeedsConversion |= (countEOLs[EOL_LS]!=0);
	m_bNeedsConversion |= (countEOLs[EOL_PS]!=0);

	size_t eolmax = 0;
	for (int nEol = 0; nEol<EOL__COUNT; nEol++)
	{
		if (eolmax < countEOLs[nEol])
		{
			eolmax = countEOLs[nEol];
			m_LineEndings = (EOL)nEol;
		}
	}

	return TRUE;
}

void CFileTextLines::StripWhiteSpace(CString& sLine, DWORD dwIgnoreWhitespaces, bool blame)
{
	if (blame)
	{
		if (sLine.GetLength() > 66)
			sLine = sLine.Mid(66);
	}
	switch (dwIgnoreWhitespaces)
	{
	case 0:
		// Compare whitespaces
		// do nothing
		break;
	case 1:
		// Ignore all whitespaces
		sLine.TrimLeft(_T(" \t"));
		sLine.TrimRight(_T(" \t"));
		break;
	case 2:
		// Ignore leading whitespace
		sLine.TrimLeft(_T(" \t"));
		break;
	case 3:
		// Ignore ending whitespace
		sLine.TrimRight(_T(" \t"));
		break;
	}
}

/**
	Encoding pattern:
		- encode & save BOM
		- Get Line
		- modify line - whitespaces, lowercase
		- encode & save line
		- get cached encoded eol
		- save eol
*/
BOOL CFileTextLines::Save(const CString& sFilePath
						 , bool bSaveAsUTF8 /*= false*/
						 , bool bUseSVNCompatibleEOLs /*= false*/
						 , DWORD dwIgnoreWhitespaces /*=0*/
						 , BOOL bIgnoreCase /*= FALSE*/
						 , bool bBlame /*= false*/) const
{
	try
	{
		CString destPath = sFilePath;
		// now make sure that the destination directory exists
		int ind = 0;
		while (destPath.Find('\\', ind)>=2)
		{
			if (!PathIsDirectory(destPath.Left(destPath.Find('\\', ind))))
			{
				if (!CreateDirectory(destPath.Left(destPath.Find('\\', ind)), NULL))
					return FALSE;
			}
			ind = destPath.Find('\\', ind)+1;
		}

		CStdioFile file;			// Hugely faster than CFile for big file writes - because it uses buffering
		if (!file.Open(sFilePath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
		{
			const_cast<CString *>(&m_sErrorString)->Format(IDS_ERR_FILE_OPEN, (LPCTSTR)sFilePath);
			return FALSE;
		}

		CBaseFilter * pFilter = NULL;
		bool bSaveBom = true;
		CFileTextLines::UnicodeType eUnicodeType = bSaveAsUTF8 ? CFileTextLines::UTF8 : m_UnicodeType;
		switch (eUnicodeType)
		{
		default:
		case CFileTextLines::ASCII:
			bSaveBom = false;
			pFilter = new CAsciiFilter(&file);
			break;
		case CFileTextLines::UTF8:
			bSaveBom = false;
		case CFileTextLines::UTF8BOM:
			pFilter = new CUtf8Filter(&file);
			break;
		case CFileTextLines::UTF16_BE:
			pFilter = new CUtf16beFilter(&file);
			break;
		case CFileTextLines::UTF16_LE:
			pFilter = new CUtf16leFilter(&file);
			break;
		case CFileTextLines::UTF32_BE:
			pFilter = new CUtf32beFilter(&file);
			break;
		case CFileTextLines::UTF32_LE:
			pFilter = new CUtf32leFilter(&file);
			break;
		}

		if (bSaveBom)
		{
			//first write the BOM
			pFilter->Write(L"\xfeff");
		}
		// cache EOLs
		CBuffer oEncodedEol[EOL__COUNT];
		oEncodedEol[EOL_LF] = pFilter->Encode(_T("\n")); // x0a
		oEncodedEol[EOL_CR] = pFilter->Encode(_T("\r")); // x0d
		oEncodedEol[EOL_CRLF] = pFilter->Encode(_T("\r\n")); // x0d x0a
		if (bUseSVNCompatibleEOLs)
		{
			// when using EOLs that are supported by the svn lib,
			// we have to use the same EOLs as the file has in case
			// they're already supported, but a different supported one
			// in case the original one isn't supported.
			// Only this way the option "ignore EOLs (recommended)" unchecked
			// actually shows the lines as different.
			// However, the diff won't find and differences in EOLs
			// for these special EOLs if they differ between those special ones
			// listed below.
			// But it will work properly for the most common EOLs LF/CR/CRLF.
			oEncodedEol[EOL_LFCR] = oEncodedEol[EOL_CR];
			for (int nEol = 0; nEol<EOL_NOENDING; nEol++)
			{
				if (oEncodedEol[nEol].IsEmpty())
					oEncodedEol[nEol] = oEncodedEol[EOL_LF];
			}
		}
		else
		{
			oEncodedEol[EOL_LFCR] = pFilter->Encode(_T("\n\r"));
			oEncodedEol[EOL_VT] = pFilter->Encode(_T("\v")); // x0b
			oEncodedEol[EOL_FF] = pFilter->Encode(_T("\f")); // x0c
			oEncodedEol[EOL_NEL] = pFilter->Encode(_T("\x85"));
			oEncodedEol[EOL_LS] = pFilter->Encode(_T("\x2028"));
			oEncodedEol[EOL_PS] = pFilter->Encode(_T("\x2029"));
		}
		oEncodedEol[EOL_AUTOLINE] = oEncodedEol[m_LineEndings==EOL_AUTOLINE ? EOL_CRLF : m_LineEndings];

		for (int i=0; i<GetCount(); i++)
		{
			CString sLineT = GetAt(i);
			StripWhiteSpace(sLineT, dwIgnoreWhitespaces, bBlame);
			if (bIgnoreCase)
				sLineT = sLineT.MakeLower();
			pFilter->Write(sLineT);
			EOL eEol = GetLineEnding(i);
			pFilter->Write(oEncodedEol[eEol]);
		}
		delete pFilter;
		file.Close();
	}
	catch (CException * e)
	{
		CString * psErrorString = const_cast<CString *>(&m_sErrorString);
		e->GetErrorMessage(psErrorString->GetBuffer(4096), 4096);
		psErrorString->ReleaseBuffer();
		e->Delete();
		return FALSE;
	}
	return TRUE;
}

void CFileTextLines::SetErrorString()
{
	m_sErrorString = CFormatMessageWrapper();
}

void CFileTextLines::CopySettings(CFileTextLines * pFileToCopySettingsTo)
{
	if (pFileToCopySettingsTo)
	{
		pFileToCopySettingsTo->m_UnicodeType = m_UnicodeType;
		pFileToCopySettingsTo->m_LineEndings = m_LineEndings;
	}
}



void CBuffer::ExpandToAtLeast(int nNewSize)
{
	if (nNewSize>m_nAllocated)
	{
		delete [] m_pBuffer; // we don't preserve buffer content intentionally
		nNewSize+=2048-1;
		nNewSize&=~(1024-1);
		m_pBuffer=new BYTE[nNewSize];
		m_nAllocated=nNewSize;
	}
}

void CBuffer::SetLength(int nUsed)
{
	ExpandToAtLeast(nUsed);
	m_nUsed = nUsed;
}

void CBuffer::Swap(CBuffer & Src)
{
	std::swap(Src.m_nAllocated, m_nAllocated);
	std::swap(Src.m_pBuffer, m_pBuffer);
	std::swap(Src.m_nUsed, m_nUsed);
}

void CBuffer::Copy(const CBuffer & Src)
{
	if (&Src != this)
	{
		SetLength(Src.m_nUsed);
		memcpy(m_pBuffer, Src.m_pBuffer, m_nUsed);
	}
}



bool CBaseFilter::Decode(/*in out*/ CBuffer & data)
{
	int nFlags = (m_nCodePage==CP_ACP) ? MB_PRECOMPOSED : 0;
	// dry decode is around 8 times faster then real one, alternatively we can set buffer to max length
	int nReadChars = MultiByteToWideChar(m_nCodePage, nFlags, (LPCSTR)data, data.GetLength(), NULL, 0);
	m_oBuffer.SetLength(nReadChars*sizeof(wchar_t));
	int ret2 = MultiByteToWideChar(m_nCodePage, nFlags, (LPCSTR)data, data.GetLength(), (LPWSTR)(void *)m_oBuffer, nReadChars);
	if (ret2 != nReadChars)
	{
		return FALSE;
	}
	data.Swap(m_oBuffer);
	return TRUE;
}

const CBuffer & CBaseFilter::Encode(const CString s)
{
	m_oBuffer.SetLength(s.GetLength()*3+1); // set buffer to guessed max size
	int nConvertedLen = WideCharToMultiByte(m_nCodePage, 0, (LPCTSTR)s, s.GetLength(), (LPSTR)m_oBuffer, m_oBuffer.GetLength(), NULL, NULL);
	m_oBuffer.SetLength(nConvertedLen); // set buffer to used size
	return m_oBuffer;
}



bool CUtf16leFilter::Decode(/*in out*/ CBuffer & /*data*/)
{
	// we believe data is ok for use
	return TRUE;
}

const CBuffer & CUtf16leFilter::Encode(const CString s)
{
	int nNeedBytes = s.GetLength()*sizeof(TCHAR);
	m_oBuffer.SetLength(nNeedBytes);
	memcpy((void *)m_oBuffer, (LPCTSTR)s, nNeedBytes);
	return m_oBuffer;
}



bool CUtf16beFilter::Decode(/*in out*/ CBuffer & data)
{
	int nNeedBytes = data.GetLength();
	// make in place WORD BYTEs swap
	UINT64 * p_qw = (UINT64 *)(void *)data;
	int nQwords = nNeedBytes/8;
	for (int nQword = 0; nQword<nQwords; nQword++)
	{
		p_qw[nQword] = WordSwapBytes(p_qw[nQword]);
	}
	wchar_t * p_w = (wchar_t *)p_qw;
	int nWords = nNeedBytes/2;
	for (int nWord = nQwords*4; nWord<nWords; nWord++)
	{
		p_w[nWord] = WideCharSwap(p_w[nWord]);
	}
	return CUtf16leFilter::Decode(data);
}

const CBuffer & CUtf16beFilter::Encode(const CString s)
{
	int nNeedBytes = s.GetLength()*sizeof(TCHAR);
	m_oBuffer.SetLength(nNeedBytes);
	// copy swaping BYTE order in WORDs
	const UINT64 * p_qwIn = (const UINT64 *)(LPCTSTR)s;
	UINT64 * p_qwOut = (UINT64 *)(void *)m_oBuffer;
	int nQwords = nNeedBytes/8;
	for (int nQword = 0; nQword<nQwords; nQword++)
	{
		p_qwOut[nQword] = WordSwapBytes(p_qwIn[nQword]);
	}
	wchar_t * p_wIn = (wchar_t *)p_qwIn;
	wchar_t * p_wOut = (wchar_t *)p_qwOut;
	int nWords = nNeedBytes/2;
	for (int nWord = nQwords*4; nWord<nWords; nWord++)
	{
		p_wOut[nWord] = WideCharSwap(p_wIn[nWord]);
	}
	return m_oBuffer;
}



bool CUtf32leFilter::Decode(/*in out*/ CBuffer & data)
{
	// UTF32 have four bytes per char
	int nReadChars = data.GetLength()/4;
	UINT32 * p32 = (UINT32 *)(void *)data;

	// count chars which needs surrogate pair
	int nSurrogatePairCount = 0;
	for (int i = 0; i<nReadChars; ++i)
	{
		if (p32[i]<0x110000 && p32[i]>=0x10000)
		{
			++nSurrogatePairCount;
		}
	}

	// fill buffer
	m_oBuffer.SetLength((nReadChars+nSurrogatePairCount)*sizeof(wchar_t));
	wchar_t * pOut = (wchar_t *)m_oBuffer;
	for (int i = 0; i<nReadChars; ++i, ++pOut)
	{
		UINT32 zChar = p32[i];
		if (zChar>=0x110000)
		{
			*pOut=0xfffd; // ? mark
		}
		else if (zChar>=0x10000)
		{
			zChar-=0x10000;
			pOut[0] = ((zChar>>10)&0x3ff) | 0xd800; // lead surrogate
			pOut[1] = (zChar&0x7ff) | 0xdc00; // trail surrogate
			pOut++;
		}
		else
		{
			*pOut = (wchar_t)zChar;
		}
	}
	data.Swap(m_oBuffer);
	return TRUE;
}

const CBuffer & CUtf32leFilter::Encode(const CString s)
{
	int nInWords = s.GetLength();
	m_oBuffer.SetLength(nInWords*2);

	LPCTSTR p_In = (LPCTSTR)s;
	UINT32 * p_Out = (UINT32 *)(void *)m_oBuffer;
	int nOutDword = 0;
	for (int nInWord = 0; nInWord<nInWords; nInWord++, nOutDword++)
	{
		UINT32 zChar = p_In[nInWord];
		if ((zChar&0xfc00) == 0xd800) // lead surrogate
		{
			if (nInWord+1<nInWords && (p_In[nInWord+1]&0xfc00) == 0xdc00) // trail surrogate follows
			{
				zChar = 0x10000 + ((zChar&0x3ff)<<10) + (p_In[++nInWord]&0x3ff);
			}
			else
			{
				zChar = 0xfffd; // ? mark
			}
		}
		else if ((zChar&0xfc00) == 0xdc00) // trail surrogate without lead
		{
			zChar = 0xfffd; // ? mark
		}
		p_Out[nOutDword] = zChar;
	}
	m_oBuffer.SetLength(nOutDword*4); // store length reduced by surrogates
	return m_oBuffer;
}



bool CUtf32beFilter::Decode(/*in out*/ CBuffer & data)
{

	// swap BYTEs order in DWORDs
	UINT64 * p64 = (UINT64 *)(void *)data;
	int nQwords = data.GetLength()/8;
	for (int nQword = 0; nQword<nQwords; nQword++)
	{
		p64[nQword] = DwordSwapBytes(p64[nQword]);
	}

	UINT32 * p32 = (UINT32 *)p64;
	int nDwords = data.GetLength()/4;
	for (int nDword = nQwords*2; nDword<nDwords; nDword++)
	{
		p32[nDword] = DwordSwapBytes(p32[nDword]);
	}
	return CUtf32leFilter::Decode(data);
}

const CBuffer & CUtf32beFilter::Encode(const CString s)
{
	CUtf32leFilter::Encode(s);

	// swap BYTEs order in DWORDs
	UINT64 * p64 = (UINT64 *)(void *)m_oBuffer;
	int nQwords = m_oBuffer.GetLength()/8;
	for (int nQword = 0; nQword<nQwords; nQword++)
	{
		p64[nQword] = DwordSwapBytes(p64[nQword]);
	}

	UINT32 * p32 = (UINT32 *)p64;
	int nDwords = m_oBuffer.GetLength()/4;
	for (int nDword = nQwords*2; nDword<nDwords; nDword++)
	{
		p32[nDword] = DwordSwapBytes(p32[nDword]);
	}
	return m_oBuffer;
}

