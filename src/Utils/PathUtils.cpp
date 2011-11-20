// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2008 - TortoiseSVN

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
#include "StdAfx.h"
#include "PathUtils.h"

BOOL CPathUtils::MakeSureDirectoryPathExists(LPCTSTR path)
{
	size_t len = _tcslen(path);
	TCHAR * buf = new TCHAR[len+10];
	TCHAR * internalpathbuf = new TCHAR[len+10];
	TCHAR * pPath = internalpathbuf;
	SECURITY_ATTRIBUTES attribs;

	SecureZeroMemory(&attribs, sizeof(SECURITY_ATTRIBUTES));

	attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
	attribs.bInheritHandle = FALSE;

	ConvertToBackslash(internalpathbuf, path, len+10);
	do
	{
		SecureZeroMemory(buf, (len+10)*sizeof(TCHAR));
		TCHAR * slashpos = _tcschr(pPath, '\\');
		if (slashpos)
			_tcsncpy_s(buf, len+10, internalpathbuf, slashpos - internalpathbuf);
		else
			_tcsncpy_s(buf, len+10, internalpathbuf, len+10);
		CreateDirectory(buf, &attribs);
		pPath = _tcschr(pPath, '\\');
	} while ((pPath++)&&(_tcschr(pPath, '\\')));
	
	BOOL bRet = CreateDirectory(internalpathbuf, &attribs);
	delete[] buf;
	delete[] internalpathbuf;
	return bRet;
}

void CPathUtils::Unescape(char * psz)
{
	char * pszSource = psz;
	char * pszDest = psz;

	static const char szHex[] = "0123456789ABCDEF";

	// Unescape special characters. The number of characters
	// in the *pszDest is assumed to be <= the number of characters
	// in pszSource (they are both the same string anyway)

	while (*pszSource != '\0' && *pszDest != '\0')
	{
		if (*pszSource == '%')
		{
			// The next two chars following '%' should be digits
			if ( *(pszSource + 1) == '\0' ||
				*(pszSource + 2) == '\0' )
			{
				// nothing left to do
				break;
			}

			char nValue = '?';
			const char * pszLow = NULL;
			const char * pszHigh = NULL;
			pszSource++;

			*pszSource = (char) toupper(*pszSource);
			pszHigh = strchr(szHex, *pszSource);

			if (pszHigh != NULL)
			{
				pszSource++;
				*pszSource = (char) toupper(*pszSource);
				pszLow = strchr(szHex, *pszSource);

				if (pszLow != NULL)
				{
					nValue = (char) (((pszHigh - szHex) << 4) +
						(pszLow - szHex));
				}
			}
			else
			{
				pszSource--;
				nValue = *pszSource;
			}
			*pszDest++ = nValue;
		} 
		else
			*pszDest++ = *pszSource;

		pszSource++;
	}

	*pszDest = '\0';
}

static const char iri_escape_chars[256] = {
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,  1, 1, 1, 1, 1, 1, 1, 1,

	/* 128 */
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
};

const char uri_autoescape_chars[256] = {
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

	/* 64 */
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
	0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

	/* 128 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

	/* 192 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};

static const char uri_char_validity[256] = {
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 1, 0, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 1, 0, 0,

	/* 64 */
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 0, 1,
	0, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1,   1, 1, 1, 0, 0, 0, 1, 0,

	/* 128 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,

	/* 192 */
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,   0, 0, 0, 0, 0, 0, 0, 0,
};


void CPathUtils::ConvertToBackslash(LPTSTR dest, LPCTSTR src, size_t len)
{
	_tcscpy_s(dest, len, src);
	TCHAR * p = dest;
	for (; *p != '\0'; ++p)
		if (*p == '/')
			*p = '\\';
}

CStringA CPathUtils::PathEscape(const CStringA& path)
{
	CStringA ret2;
	int c;
	int i;
	for (i=0; path[i]; ++i)
	{
		c = (unsigned char)path[i];
		if (iri_escape_chars[c])
		{
			// no escaping needed for that char
			ret2 += (unsigned char)path[i];
		}
		else
		{
			// char needs escaping
			CStringA temp;
			temp.Format("%%%02X", (unsigned char)c);
			ret2 += temp;
		}
	}
	CStringA ret;
	for (i=0; ret2[i]; ++i)
	{
		c = (unsigned char)ret2[i];
		if (uri_autoescape_chars[c])
		{
			// no escaping needed for that char
			ret += (unsigned char)ret2[i];
		}
		else
		{
			// char needs escaping
			CStringA temp;
			temp.Format("%%%02X", (unsigned char)c);
			ret += temp;
		}
	}

	if ((ret.Left(11).Compare("file:///%5C") == 0) && (ret.Find('%', 12) < 0))
		ret.Replace(("file:///%5C"), ("file://"));
	ret.Replace(("file:////%5C"), ("file://"));

	return ret;
}

#ifdef CSTRING_AVAILABLE
CString CPathUtils::GetFileNameFromPath(CString sPath)
{
	CString ret;
	sPath.Replace(_T("/"), _T("\\"));
	ret = sPath.Mid(sPath.ReverseFind('\\') + 1);
	return ret;
}

CString CPathUtils::GetFileExtFromPath(const CString& sPath)
{
	int dotPos = sPath.ReverseFind('.');
	int slashPos = sPath.ReverseFind('\\');
	if (slashPos < 0)
		slashPos = sPath.ReverseFind('/');
	if (dotPos > slashPos)
		return sPath.Mid(dotPos);
	return CString();
}

CString CPathUtils::GetLongPathname(const CString& path)
{
	if (path.IsEmpty())
		return path;
	TCHAR pathbufcanonicalized[MAX_PATH]; // MAX_PATH ok.
	DWORD ret = 0;
	CString sRet;
	if (!PathIsURL(path) && PathIsRelative(path))
	{
		ret = GetFullPathName(path, 0, NULL, NULL);
		if (ret)
		{
			TCHAR * pathbuf = new TCHAR[ret+1];
			if ((ret = GetFullPathName(path, ret, pathbuf, NULL))!=0)
			{
				sRet = CString(pathbuf, ret);
			}
			delete [] pathbuf;
		}
	}
	else if (PathCanonicalize(pathbufcanonicalized, path))
	{
		ret = ::GetLongPathName(pathbufcanonicalized, NULL, 0);
		TCHAR * pathbuf = new TCHAR[ret+2];	
		ret = ::GetLongPathName(pathbufcanonicalized, pathbuf, ret+1);
		sRet = CString(pathbuf, ret);
		delete[] pathbuf;
	}
	else
	{
		ret = ::GetLongPathName(path, NULL, 0);
		TCHAR * pathbuf = new TCHAR[ret+2];
		ret = ::GetLongPathName(path, pathbuf, ret+1);
		sRet = CString(pathbuf, ret);
		delete[] pathbuf;
	}
	if (ret == 0)
		return path;
	return sRet;
}

BOOL CPathUtils::FileCopy(CString srcPath, CString destPath, BOOL force)
{
	srcPath.Replace('/', '\\');
	destPath.Replace('/', '\\');
	CString destFolder = destPath.Left(destPath.ReverseFind('\\'));
	MakeSureDirectoryPathExists(destFolder);
	return (CopyFile(srcPath, destPath, !force));
}

CString CPathUtils::ParsePathInString(const CString& Str)
{
	CString sToken;
	int curPos = 0;
	sToken = Str.Tokenize(_T("'\t\r\n"), curPos);
	while (!sToken.IsEmpty())
	{
		if ((sToken.Find('/')>=0)||(sToken.Find('\\')>=0))
		{
			sToken.Trim(_T("'\""));
			return sToken;
		}
		sToken = Str.Tokenize(_T("'\t\r\n"), curPos);
	}
	sToken.Empty();
	return sToken;
}

CString CPathUtils::GetAppDirectory(HMODULE hMod /* = NULL */)
{
    CString path;
    DWORD len = 0;
    DWORD bufferlen = MAX_PATH;     // MAX_PATH is not the limit here!
    path.GetBuffer(bufferlen);
    do
    {
        bufferlen += MAX_PATH;      // MAX_PATH is not the limit here!
        path.ReleaseBuffer(0);
        len = GetModuleFileName(hMod, path.GetBuffer(bufferlen+1), bufferlen);
    } while(len == bufferlen);
    path.ReleaseBuffer();
    path = path.Left(path.ReverseFind('\\')+1);
    return path;
}

CString CPathUtils::GetAppParentDirectory(HMODULE hMod /* = NULL */)
{
    CString path = GetAppDirectory(hMod);
    path = path.Left(path.ReverseFind('\\'));
    path = path.Left(path.ReverseFind('\\')+1);
    return path;
}

CString CPathUtils::GetAppDataDirectory()
{
	TCHAR path[MAX_PATH];		//MAX_PATH ok here.
	if (SHGetFolderPath(NULL, CSIDL_APPDATA, NULL, SHGFP_TYPE_CURRENT, path)!=S_OK)
		return CString();

	_tcscat_s(path, MAX_PATH, _T("\\TortoiseGit"));
	if (!PathIsDirectory(path))
		CreateDirectory(path, NULL);

	return CString (path) + _T('\\');
}


CStringA CPathUtils::PathUnescape(const CStringA& path)
{
	std::auto_ptr<char> urlabuf (new char[path.GetLength()+1]);

	strcpy_s(urlabuf.get(), path.GetLength()+1, path);
	Unescape(urlabuf.get());

	return urlabuf.get();
}

CStringW CPathUtils::PathUnescape(const CStringW& path)
{
	char * buf;
	CStringA patha;
	int len = path.GetLength();
	if (len==0)
		return CStringW();
	buf = patha.GetBuffer(len*4 + 1);
	int lengthIncTerminator = WideCharToMultiByte(CP_UTF8, 0, path, -1, buf, len*4, NULL, NULL);
	patha.ReleaseBuffer(lengthIncTerminator-1);

	patha = PathUnescape(patha);

	WCHAR * bufw;
	len = patha.GetLength();
	bufw = new WCHAR[len*4 + 1];
	SecureZeroMemory(bufw, (len*4 + 1)*sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, patha, -1, bufw, len*4);
	CStringW ret = CStringW(bufw);
	delete [] bufw;
	return ret;
}
#ifdef _MFC_VER
CString CPathUtils::GetVersionFromFile(const CString & p_strDateiname)
{
	struct TRANSARRAY
	{
		WORD wLanguageID;
		WORD wCharacterSet;
	};

	CString strReturn;
	DWORD dwReserved,dwBufferSize;
	dwBufferSize = GetFileVersionInfoSize((LPTSTR)(LPCTSTR)p_strDateiname,&dwReserved);

	if (dwBufferSize > 0)
	{
		LPVOID pBuffer = (void*) malloc(dwBufferSize);

		if (pBuffer != (void*) NULL)
		{
			UINT        nInfoSize = 0,
				nFixedLength = 0;
			LPSTR       lpVersion = NULL;
			VOID*       lpFixedPointer;
			TRANSARRAY* lpTransArray;
			CString     strLangProduktVersion;

			GetFileVersionInfo((LPTSTR)(LPCTSTR)p_strDateiname,
				dwReserved,
				dwBufferSize,
				pBuffer);

			// Check the current language
			VerQueryValue(	pBuffer,
				_T("\\VarFileInfo\\Translation"),
				&lpFixedPointer,
				&nFixedLength);
			lpTransArray = (TRANSARRAY*) lpFixedPointer;

			strLangProduktVersion.Format(_T("\\StringFileInfo\\%04x%04x\\ProductVersion"),
				lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

			VerQueryValue(pBuffer,
				(LPTSTR)(LPCTSTR)strLangProduktVersion,
				(LPVOID *)&lpVersion,
				&nInfoSize);
			strReturn = (LPCTSTR)lpVersion;
			free(pBuffer);
		}
	} 

	return strReturn;
}
#endif
CString CPathUtils::PathPatternEscape(const CString& path)
{
	CString result = path;
	// first remove already escaped patterns to avoid having those
	// escaped twice
	result.Replace(_T("\\["), _T("["));
	result.Replace(_T("\\]"), _T("]"));
	// now escape the patterns (again)
	result.Replace(_T("["), _T("\\["));
	result.Replace(_T("]"), _T("\\]"));
	return result;
}

CString CPathUtils::PathPatternUnEscape(const CString& path)
{
	CString result = path;
	result.Replace(_T("\\["), _T("["));
	result.Replace(_T("\\]"), _T("]"));
	return result;
}

#endif

#if defined(_DEBUG) && defined(_MFC_VER)
// Some test cases for these classes
static class CPathTests
{
public:
	CPathTests()
	{
		UnescapeTest();
		ExtTest();
		ParseTests();
	}

private:
	void UnescapeTest()
	{
		CString test(_T("file:///d:/REpos1/uCOS-100/Trunk/name%20with%20spaces/NewTest%20%%20NewTest"));
		CString test2 = CPathUtils::PathUnescape(test);
		ATLASSERT(test2.Compare(_T("file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest")) == 0);
		CStringA test3 = CPathUtils::PathEscape("file:///d:/REpos1/uCOS-100/Trunk/name with spaces/NewTest % NewTest");
		ATLASSERT(test3.Compare("file:///d:/REpos1/uCOS-100/Trunk/name%20with%20spaces/NewTest%20%%20NewTest") == 0);
	}
	void ExtTest()
	{
		CString test(_T("d:\\test\filename.ext"));
		ATLASSERT(CPathUtils::GetFileExtFromPath(test).Compare(_T(".ext")) == 0);
		test = _T("filename.ext");
		ATLASSERT(CPathUtils::GetFileExtFromPath(test).Compare(_T(".ext")) == 0);
		test = _T("d:\\test\filename");
		ATLASSERT(CPathUtils::GetFileExtFromPath(test).IsEmpty());
		test = _T("filename");
		ATLASSERT(CPathUtils::GetFileExtFromPath(test).IsEmpty());
	}
	void ParseTests()
	{
		CString test(_T("test 'd:\\testpath with spaces' test"));
		ATLASSERT(CPathUtils::ParsePathInString(test).Compare(_T("d:\\testpath with spaces")) == 0);
		test = _T("d:\\testpath with spaces");
		ATLASSERT(CPathUtils::ParsePathInString(test).Compare(_T("d:\\testpath with spaces")) == 0);
	}

} CPathTests;
#endif

