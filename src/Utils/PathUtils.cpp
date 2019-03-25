// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2012-2019 - TortoiseGit
// Copyright (C) 2003-2008, 2013-2015 - TortoiseSVN

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
#include "PathUtils.h"
#include <memory>
#include "StringUtils.h"
#include "../../ext/libgit2/src/win32/reparse.h"
#include "SmartHandle.h"

BOOL CPathUtils::MakeSureDirectoryPathExists(LPCTSTR path)
{
	size_t len = wcslen(path) + 10;
	auto buf = std::make_unique<TCHAR[]>(len);
	auto internalpathbuf = std::make_unique<TCHAR[]>(len);
	TCHAR * pPath = internalpathbuf.get();
	SECURITY_ATTRIBUTES attribs = { 0 };
	attribs.nLength = sizeof(SECURITY_ATTRIBUTES);
	attribs.bInheritHandle = FALSE;

	ConvertToBackslash(internalpathbuf.get(), path, len);
	do
	{
		SecureZeroMemory(buf.get(), (len)*sizeof(TCHAR));
		TCHAR* slashpos = wcschr(pPath, L'\\');
		if (slashpos)
			wcsncpy_s(buf.get(), len, internalpathbuf.get(), slashpos - internalpathbuf.get());
		else
			wcsncpy_s(buf.get(), len, internalpathbuf.get(), len);
		CreateDirectory(buf.get(), &attribs);
		pPath = wcschr(pPath, L'\\');
	} while ((pPath++) && (wcschr(pPath, L'\\')));

	return CreateDirectory(internalpathbuf.get(), &attribs);
}

void CPathUtils::ConvertToBackslash(LPTSTR dest, LPCTSTR src, size_t len)
{
	wcscpy_s(dest, len, src);
	TCHAR * p = dest;
	for (; *p != '\0'; ++p)
		if (*p == '/')
			*p = '\\';
}

#ifdef CSTRING_AVAILABLE
void CPathUtils::ConvertToBackslash(CString& path)
{
	path.Replace(L'/', L'\\');
}

bool CPathUtils::Touch(const CString& path)
{
	CAutoFile hFile = CreateFile(path, GENERIC_WRITE, FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hFile)
		return false;

	FILETIME ft;
	SYSTEMTIME st;
	GetSystemTime(&st);					// Gets the current system time
	SystemTimeToFileTime(&st, &ft);		// Converts the current system time to file time format
	return SetFileTime(hFile,			// Sets last-write time of the file 
		nullptr,						// to the converted current system time 
		nullptr,
		&ft) != FALSE;
}

CString CPathUtils::GetFileNameFromPath(CString sPath)
{
	CString ret;
	sPath.Replace(L'/', L'\\');
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
	TCHAR pathbufcanonicalized[MAX_PATH] = {0}; // MAX_PATH ok.
	DWORD ret = 0;
	CString sRet;
	if (!PathIsURL(path) && PathIsRelative(path))
	{
		ret = GetFullPathName(path, 0, nullptr, nullptr);
		if (ret)
		{
			auto pathbuf = std::make_unique<TCHAR[]>(ret + 1);
			if ((ret = GetFullPathName(path, ret, pathbuf.get(), nullptr)) != 0)
				sRet = CString(pathbuf.get(), ret);
		}
	}
	else if (PathCanonicalize(pathbufcanonicalized, path))
	{
		ret = ::GetLongPathName(pathbufcanonicalized, nullptr, 0);
		if (ret == 0)
			return path;
		auto pathbuf = std::make_unique<TCHAR[]>(ret + 2);
		ret = ::GetLongPathName(pathbufcanonicalized, pathbuf.get(), ret + 1);
		sRet = CString(pathbuf.get(), ret);
	}
	else
	{
		ret = ::GetLongPathName(path, nullptr, 0);
		if (ret == 0)
			return path;
		auto pathbuf = std::make_unique<TCHAR[]>(ret + 2);
		ret = ::GetLongPathName(path, pathbuf.get(), ret + 1);
		sRet = CString(pathbuf.get(), ret);
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
	int curPos = 0;
	CString sToken = Str.Tokenize(L"'\t\r\n", curPos);
	while (!sToken.IsEmpty())
	{
		if ((sToken.Find('/')>=0)||(sToken.Find('\\')>=0))
		{
			sToken.Trim(L"'\"");
			return sToken;
		}
		sToken = Str.Tokenize(L"'\t\r\n", curPos);
	}
	sToken.Empty();
	return sToken;
}

CString CPathUtils::GetAppDirectory(HMODULE hMod /* = nullptr */)
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
	return GetLongPathname(path);
}

CString CPathUtils::GetAppParentDirectory(HMODULE hMod /* = nullptr */)
{
	CString path = GetAppDirectory(hMod);
	path = path.Left(path.ReverseFind('\\'));
	path = path.Left(path.ReverseFind('\\')+1);
	return path;
}

CString CPathUtils::GetAppDataDirectory()
{
	CComHeapPtr<WCHAR> pszPath;
	if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
		return CString();

	CString path = pszPath;
	path += L"\\TortoiseGit";
	if (!PathIsDirectory(path))
		CreateDirectory(path, nullptr);

	path += L'\\';
	return path;
}

CString CPathUtils::GetLocalAppDataDirectory()
{
	CComHeapPtr<WCHAR> pszPath;
	if (SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
		return CString();
	CString path = pszPath;
	path += L"\\TortoiseGit";
	if (!PathIsDirectory(path))
		CreateDirectory(path, nullptr);

	path += L'\\';
	return path;
}

CString CPathUtils::GetDocumentsDirectory()
{
	CComHeapPtr<WCHAR> pszPath;
	if (SHGetKnownFolderPath(FOLDERID_Documents, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
		return CString();

	return CString(pszPath);
}

CString CPathUtils::GetProgramsDirectory()
{
	CComHeapPtr<WCHAR> pszPath;
	if (SHGetKnownFolderPath(FOLDERID_ProgramFiles, KF_FLAG_CREATE, nullptr, &pszPath) != S_OK)
		return CString();

	return CString(pszPath);
}

int CPathUtils::ReadLink(LPCTSTR filename, CStringA* pTargetA)
{
	CAutoFile handle  = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT | FILE_FLAG_BACKUP_SEMANTICS, nullptr);
	if (!handle)
		return -1;

	DWORD ioctl_ret;
	BYTE buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE] = { 0 };
	auto reparse_buf = reinterpret_cast<GIT_REPARSE_DATA_BUFFER*>(&buf);
	if (!DeviceIoControl(handle, FSCTL_GET_REPARSE_POINT, nullptr, 0, reparse_buf, sizeof(buf), &ioctl_ret, nullptr))
		return -1;

	if (reparse_buf->ReparseTag != IO_REPARSE_TAG_SYMLINK)
		return -1;

	wchar_t* target = reparse_buf->SymbolicLinkReparseBuffer.PathBuffer + (reparse_buf->SymbolicLinkReparseBuffer.SubstituteNameOffset / sizeof(WCHAR));
	int target_len = reparse_buf->SymbolicLinkReparseBuffer.SubstituteNameLength / sizeof(WCHAR);
	if (!target_len)
		return -1;

	// not a symlink
	if (wcsncmp(target, L"\\??\\Volume{", 11) == 0)
		return -1;

	if (pTargetA)
	{
		CString targetW(target, target_len);
		// The path may need to have a prefix removed
		DropPathPrefixes(targetW);
		targetW.Replace(L'\\', L'/');
		*pTargetA = CUnicodeUtils::GetUTF8(targetW);
	}

	return 0;
}

void CPathUtils::DropPathPrefixes(CString& path)
{
	static const wchar_t dosdevices_prefix[] = L"\\\?\?\\";
	static const wchar_t nt_prefix[] = L"\\\\?\\";
	static const wchar_t unc_prefix[] = L"UNC\\";

	int skip = 0;
	if (CStringUtils::StartsWith(path, dosdevices_prefix))
		skip += static_cast<int>(wcslen(dosdevices_prefix));
	else if (CStringUtils::StartsWith(path, nt_prefix))
		skip += static_cast<int>(wcslen(nt_prefix));

	if (skip)
	{
		if (path.GetLength() - skip > static_cast<int>(wcslen(unc_prefix)) && CStringUtils::StartsWith(path.GetString() + skip, unc_prefix))
			skip += static_cast<int>(wcslen(unc_prefix));

		path = path.Mid(skip);
	}
}

#ifdef _MFC_VER
#pragma comment(lib, "Version.lib")
CString CPathUtils::GetVersionFromFile(const CString & p_strFilename)
{
	struct TRANSARRAY
	{
		WORD wLanguageID;
		WORD wCharacterSet;
	};

	CString strReturn;
	DWORD dwReserved = 0;
	DWORD dwBufferSize = GetFileVersionInfoSize(static_cast<LPCTSTR>(p_strFilename), &dwReserved);

	if (dwBufferSize > 0)
	{
		auto pBuffer = std::make_unique<BYTE[]>(dwBufferSize);

		if (pBuffer)
		{
			UINT        nInfoSize = 0,
						nFixedLength = 0;
			LPSTR       lpVersion = nullptr;
			VOID*       lpFixedPointer;
			TRANSARRAY* lpTransArray;
			CString     strLangProductVersion;

			GetFileVersionInfo(static_cast<LPCTSTR>(p_strFilename),
				dwReserved,
				dwBufferSize,
				pBuffer.get());

			// Check the current language
			VerQueryValue(pBuffer.get(),
				L"\\VarFileInfo\\Translation",
				&lpFixedPointer,
				&nFixedLength);
			lpTransArray = static_cast<TRANSARRAY*>(lpFixedPointer);

			strLangProductVersion.Format(L"\\StringFileInfo\\%04x%04x\\ProductVersion",
				lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

			VerQueryValue(pBuffer.get(),
				static_cast<LPCTSTR>(strLangProductVersion),
				reinterpret_cast<LPVOID*>(&lpVersion),
				&nInfoSize);
			if (nInfoSize && lpVersion)
				strReturn = reinterpret_cast<LPCTSTR>(lpVersion);
		}
	}

	return strReturn;
}

CString CPathUtils::GetCopyrightForSelf()
{
	DWORD len = 0;
	DWORD bufferlen = MAX_PATH; // MAX_PATH is not the limit here!
	CString path;
	path.GetBuffer(bufferlen);
	do
	{
		bufferlen += MAX_PATH; // MAX_PATH is not the limit here!
		path.ReleaseBuffer(0);
		len = GetModuleFileName(nullptr, path.GetBuffer(bufferlen + 1), bufferlen);
	} while (len == bufferlen);
	path.ReleaseBuffer();

	CString strReturn;
	DWORD dwReserved = 0;
	DWORD dwBufferSize = GetFileVersionInfoSize(static_cast<LPCTSTR>(path), &dwReserved);

	if (dwBufferSize > 0)
	{
		auto pBuffer = std::make_unique<BYTE[]>(dwBufferSize);

		if (pBuffer)
		{
			GetFileVersionInfo(static_cast<LPCTSTR>(path),
				dwReserved,
				dwBufferSize,
				pBuffer.get());

			UINT nFixedLength = 0;
			VOID* lpFixedPointer;
			struct TRANSARRAY
			{
				WORD wLanguageID;
				WORD wCharacterSet;
			};
			TRANSARRAY* lpTransArray;
			// Check the current language
			VerQueryValue(pBuffer.get(), L"\\VarFileInfo\\Translation", &lpFixedPointer, &nFixedLength);
			lpTransArray = static_cast<TRANSARRAY*>(lpFixedPointer);

			CString strLangLegalCopyright;
			strLangLegalCopyright.Format(L"\\StringFileInfo\\%04x%04x\\LegalCopyright", lpTransArray[0].wLanguageID, lpTransArray[0].wCharacterSet);

			UINT nInfoSize = 0;
			LPSTR lpVersion = nullptr;
			VerQueryValue(pBuffer.get(), static_cast<LPCTSTR>(strLangLegalCopyright), reinterpret_cast<LPVOID*>(&lpVersion), &nInfoSize);
			if (nInfoSize && lpVersion)
				strReturn = lpVersion;
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
	result.Replace(L"\\[", L"[");
	result.Replace(L"\\]", L"]");
	// now escape the patterns (again)
	result.Replace(L"[", L"\\[");
	result.Replace(L"]", L"\\]");
	return result;
}

CString CPathUtils::PathPatternUnEscape(const CString& path)
{
	CString result = path;
	result.Replace(L"\\[", L"[");
	result.Replace(L"\\]", L"]");
	return result;
}

CString CPathUtils::BuildPathWithPathDelimiter(const CString& path)
{
	CString result(path);
	EnsureTrailingPathDelimiter(result);
	return result;
}

void CPathUtils::EnsureTrailingPathDelimiter(CString& path)
{
	if (!path.IsEmpty() && !CStringUtils::EndsWith(path, L'\\'))
		path.AppendChar(L'\\');
}

void CPathUtils::TrimTrailingPathDelimiter(CString& path)
{
	path.TrimRight(L'\\');
}

CString CPathUtils::ExpandFileName(const CString& path)
{
	if (path.IsEmpty())
		return path;

	DWORD ret = GetFullPathName(path, 0, nullptr, nullptr);
	if (!ret)
		return path;

	CString sRet;
	if (GetFullPathName(path, ret, CStrBuf(sRet, ret), nullptr))
		return sRet;
	return path;
}

CString CPathUtils::NormalizePath(const CString& path)
{
	// Account DOS 8.3 file/folder names
	CString nPath = GetLongPathname(path);

	// Account for ..\ and .\ that may occur in each path
	nPath = ExpandFileName(nPath);

	nPath.MakeLower();

	TrimTrailingPathDelimiter(nPath);

	return nPath;
}

bool CPathUtils::IsSamePath(const CString& path1, const CString& path2)
{
	return ArePathStringsEqualWithCase(NormalizePath(path1), NormalizePath(path2));
}

bool CPathUtils::ArePathStringsEqual(const CString& sP1, const CString& sP2)
{
	int length = sP1.GetLength();
	if (length != sP2.GetLength())
	{
		// Different lengths
		return false;
	}
	// We work from the end of the strings, because path differences
	// are more likely to occur at the far end of a string
	LPCTSTR pP1Start = sP1;
	LPCTSTR pP1 = pP1Start + (length - 1);
	LPCTSTR pP2 = static_cast<LPCTSTR>(sP2) + (length - 1);
	while (length-- > 0)
	{
		if (_totlower(*pP1--) != _totlower(*pP2--))
			return false;
	}
	return true;
}

bool CPathUtils::ArePathStringsEqualWithCase(const CString& sP1, const CString& sP2)
{
	int length = sP1.GetLength();
	if (length != sP2.GetLength())
	{
		// Different lengths
		return false;
	}
	// We work from the end of the strings, because path differences
	// are more likely to occur at the far end of a string
	LPCTSTR pP1Start = sP1;
	LPCTSTR pP1 = pP1Start + (length - 1);
	LPCTSTR pP2 = static_cast<LPCTSTR>(sP2) + (length - 1);
	while (length-- > 0)
	{
		if ((*pP1--) != (*pP2--))
			return false;
	}
	return true;
}
#endif
