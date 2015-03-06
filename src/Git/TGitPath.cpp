// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2015 - TortoiseGit
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
#include "stdafx.h"
#include "TGitPath.h"
#include "UnicodeUtils.h"
#include "GitAdminDir.h"
#include "PathUtils.h"
#include <regex>
#include "Git.h"
#include "../TortoiseShell/Globals.h"
#include "StringUtils.h"
#include "../Resources/LoglistCommonResource.h"
#include <memory>

extern CGit g_Git;

CTGitPath::CTGitPath(void)
	: m_bDirectoryKnown(false)
	, m_bIsDirectory(false)
	, m_bURLKnown(false)
	, m_bHasAdminDirKnown(false)
	, m_bHasAdminDir(false)
	, m_bIsValidOnWindowsKnown(false)
	, m_bIsValidOnWindows(false)
	, m_bIsReadOnly(false)
	, m_bIsAdminDirKnown(false)
	, m_bIsAdminDir(false)
	, m_bExists(false)
	, m_bExistsKnown(false)
	, m_bLastWriteTimeKnown(0)
	, m_lastWriteTime(0)
	, m_customData(NULL)
	, m_bIsSpecialDirectoryKnown(false)
	, m_bIsSpecialDirectory(false)
	, m_bIsWCRootKnown(false)
	, m_bIsWCRoot(false)
	, m_fileSize(0)
	, m_Checked(false)
{
	m_Action=0;
	m_ParentNo=0;
	m_Stage = 0;
}

CTGitPath::~CTGitPath(void)
{
}
// Create a TGitPath object from an unknown path type (same as using SetFromUnknown)
CTGitPath::CTGitPath(const CString& sUnknownPath) :
	  m_bDirectoryKnown(false)
	, m_bIsDirectory(false)
	, m_bURLKnown(false)
	, m_bHasAdminDirKnown(false)
	, m_bHasAdminDir(false)
	, m_bIsValidOnWindowsKnown(false)
	, m_bIsValidOnWindows(false)
	, m_bIsReadOnly(false)
	, m_bIsAdminDirKnown(false)
	, m_bIsAdminDir(false)
	, m_bExists(false)
	, m_bExistsKnown(false)
	, m_bLastWriteTimeKnown(0)
	, m_lastWriteTime(0)
	, m_customData(NULL)
	, m_bIsSpecialDirectoryKnown(false)
	, m_bIsSpecialDirectory(false)
	, m_bIsWCRootKnown(false)
	, m_bIsWCRoot(false)
	, m_fileSize(0)
	, m_Checked(false)
{
	SetFromUnknown(sUnknownPath);
	m_Action=0;
	m_Stage=0;
	m_ParentNo=0;
}

int CTGitPath::ParserAction(BYTE action)
{
	//action=action.TrimLeft();
	//TCHAR c=action.GetAt(0);
	if(action == 'M')
		m_Action|= LOGACTIONS_MODIFIED;
	if(action == 'R')
		m_Action|= LOGACTIONS_REPLACED;
	if(action == 'A')
		m_Action|= LOGACTIONS_ADDED;
	if(action == 'D')
		m_Action|= LOGACTIONS_DELETED;
	if(action == 'U')
		m_Action|= LOGACTIONS_UNMERGED;
	if(action == 'K')
		m_Action|= LOGACTIONS_DELETED;
	if(action == 'C' )
		m_Action|= LOGACTIONS_COPY;
	if(action == 'T')
		m_Action|= LOGACTIONS_MODIFIED;

	return m_Action;
}
void CTGitPath::SetFromGit(const char* pPath)
{
	Reset();
	if (pPath == NULL)
		return;
	int len = MultiByteToWideChar(CP_UTF8, 0, pPath, -1, NULL, 0);
	if (len)
	{
		len = MultiByteToWideChar(CP_UTF8, 0, pPath, -1, m_sFwdslashPath.GetBuffer(len+1), len+1);
		m_sFwdslashPath.ReleaseBuffer(len-1);
	}
	SanitizeRootPath(m_sFwdslashPath, true);
}

void CTGitPath::SetFromGit(const char* pPath, bool bIsDirectory)
{
	SetFromGit(pPath);
	m_bDirectoryKnown = true;
	m_bIsDirectory = bIsDirectory;
}

void CTGitPath::SetFromGit(const TCHAR* pPath, bool bIsDirectory)
{
	Reset();
	if (pPath)
	{
		m_sFwdslashPath = pPath;
		SanitizeRootPath(m_sFwdslashPath, true);
	}
	m_bDirectoryKnown = true;
	m_bIsDirectory = bIsDirectory;
}

void CTGitPath::SetFromGit(const CString& sPath,CString *oldpath)
{
	Reset();
	m_sFwdslashPath = sPath;
	SanitizeRootPath(m_sFwdslashPath, true);
	if(oldpath)
		m_sOldFwdslashPath = *oldpath;
}

void CTGitPath::SetFromWin(LPCTSTR pPath)
{
	Reset();
	m_sBackslashPath = pPath;
	m_sBackslashPath.Replace(L"\\\\?\\", L"");
	SanitizeRootPath(m_sBackslashPath, false);
	ATLASSERT(m_sBackslashPath.Find('/')<0);
}
void CTGitPath::SetFromWin(const CString& sPath)
{
	Reset();
	m_sBackslashPath = sPath;
	m_sBackslashPath.Replace(L"\\\\?\\", L"");
	SanitizeRootPath(m_sBackslashPath, false);
}
void CTGitPath::SetFromWin(LPCTSTR pPath, bool bIsDirectory)
{
	Reset();
	m_sBackslashPath = pPath;
	m_bIsDirectory = bIsDirectory;
	m_bDirectoryKnown = true;
	SanitizeRootPath(m_sBackslashPath, false);
}
void CTGitPath::SetFromWin(const CString& sPath, bool bIsDirectory)
{
	Reset();
	m_sBackslashPath = sPath;
	m_bIsDirectory = bIsDirectory;
	m_bDirectoryKnown = true;
	SanitizeRootPath(m_sBackslashPath, false);
}
void CTGitPath::SetFromUnknown(const CString& sPath)
{
	Reset();
	// Just set whichever path we think is most likely to be used
//	GitAdminDir admin;
//	CString p;
//	if(admin.HasAdminDir(sPath,&p))
//		SetFwdslashPath(sPath.Right(sPath.GetLength()-p.GetLength()));
//	else
		SetFwdslashPath(sPath);
}

LPCTSTR CTGitPath::GetWinPath() const
{
	if(IsEmpty())
	{
		return _T("");
	}
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
	}
	return m_sBackslashPath;
}
// This is a temporary function, to be used during the migration to
// the path class.  Ultimately, functions consuming paths should take a CTGitPath&, not a CString
const CString& CTGitPath::GetWinPathString() const
{
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
	}
	return m_sBackslashPath;
}

const CString& CTGitPath::GetGitPathString() const
{
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
	}
	return m_sFwdslashPath;
}

const CString &CTGitPath::GetGitOldPathString() const
{
	return m_sOldFwdslashPath;
}

const CString& CTGitPath::GetUIPathString() const
{
	if (m_sUIPath.IsEmpty())
	{
		m_sUIPath = GetWinPathString();
	}
	return m_sUIPath;
}

void CTGitPath::SetFwdslashPath(const CString& sPath) const
{
	CString path = sPath;
	path.Replace('\\', '/');

	// We don't leave a trailing /
	path.TrimRight('/');
	path.Replace(L"//?/", L"");

	SanitizeRootPath(path, true);

	path.Replace(_T("file:////"), _T("file://"));
	m_sFwdslashPath = path;

	m_sUTF8FwdslashPath.Empty();
}

void CTGitPath::SetBackslashPath(const CString& sPath) const
{
	CString path = sPath;
	path.Replace('/', '\\');
	path.TrimRight('\\');
	SanitizeRootPath(path, false);
	m_sBackslashPath = path;
}

void CTGitPath::SetUTF8FwdslashPath(const CString& sPath) const
{
	m_sUTF8FwdslashPath = CUnicodeUtils::GetUTF8(sPath);
}

void CTGitPath::SanitizeRootPath(CString& sPath, bool bIsForwardPath) const
{
	// Make sure to add the trailing slash to root paths such as 'C:'
	if (sPath.GetLength() == 2 && sPath[1] == ':')
	{
		sPath += (bIsForwardPath) ? _T("/") : _T("\\");
	}
}

bool CTGitPath::IsDirectory() const
{
	if(!m_bDirectoryKnown)
	{
		UpdateAttributes();
	}
	return m_bIsDirectory;
}

bool CTGitPath::Exists() const
{
	if (!m_bExistsKnown)
	{
		UpdateAttributes();
	}
	return m_bExists;
}

bool CTGitPath::Delete(bool bTrash) const
{
	EnsureBackslashPathSet();
	::SetFileAttributes(m_sBackslashPath, FILE_ATTRIBUTE_NORMAL);
	bool bRet = false;
	if (Exists())
	{
		if ((bTrash)||(IsDirectory()))
		{
			std::unique_ptr<TCHAR[]> buf(new TCHAR[m_sBackslashPath.GetLength() + 2]);
			_tcscpy_s(buf.get(), m_sBackslashPath.GetLength() + 2, m_sBackslashPath);
			buf[m_sBackslashPath.GetLength()] = 0;
			buf[m_sBackslashPath.GetLength()+1] = 0;
			bRet = CTGitPathList::DeleteViaShell(buf.get(), bTrash);
		}
		else
		{
			bRet = !!::DeleteFile(m_sBackslashPath);
		}
	}
	m_bExists = false;
	m_bExistsKnown = true;
	return bRet;
}

__int64  CTGitPath::GetLastWriteTime() const
{
	if(!m_bLastWriteTimeKnown)
	{
		UpdateAttributes();
	}
	return m_lastWriteTime;
}

__int64 CTGitPath::GetFileSize() const
{
	if(!m_bDirectoryKnown)
	{
		UpdateAttributes();
	}
	return m_fileSize;
}

bool CTGitPath::IsReadOnly() const
{
	if(!m_bLastWriteTimeKnown)
	{
		UpdateAttributes();
	}
	return m_bIsReadOnly;
}

void CTGitPath::UpdateAttributes() const
{
	EnsureBackslashPathSet();
	WIN32_FILE_ATTRIBUTE_DATA attribs;
	if (m_sBackslashPath.GetLength() >= 248)
		m_sLongBackslashPath = _T("\\\\?\\") + m_sBackslashPath;
	if(GetFileAttributesEx(m_sBackslashPath.GetLength() >= 248 ? m_sLongBackslashPath : m_sBackslashPath, GetFileExInfoStandard, &attribs))
	{
		m_bIsDirectory = !!(attribs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		// don't cast directly to an __int64:
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724284%28v=vs.85%29.aspx
		// "Do not cast a pointer to a FILETIME structure to either a ULARGE_INTEGER* or __int64* value
		// because it can cause alignment faults on 64-bit Windows."
		m_lastWriteTime = static_cast<__int64>(attribs.ftLastWriteTime.dwHighDateTime) << 32 | attribs.ftLastWriteTime.dwLowDateTime;
		if (m_bIsDirectory)
		{
			m_fileSize = 0;
		}
		else
		{
			m_fileSize = ((INT64)( (DWORD)(attribs.nFileSizeLow) ) | ( (INT64)( (DWORD)(attribs.nFileSizeHigh) )<<32 ));
		}
		m_bIsReadOnly = !!(attribs.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
		m_bExists = true;
	}
	else
	{
		m_bIsDirectory = false;
		m_lastWriteTime = 0;
		m_fileSize = 0;
		DWORD err = GetLastError();
		if ((err == ERROR_FILE_NOT_FOUND)||(err == ERROR_PATH_NOT_FOUND)||(err == ERROR_INVALID_NAME))
		{
			m_bExists = false;
		}
		else
		{
			m_bExists = true;
			return;
		}
	}
	m_bDirectoryKnown = true;
	m_bLastWriteTimeKnown = true;
	m_bExistsKnown = true;
}

CTGitPath CTGitPath::GetSubPath(const CTGitPath &root)
{
	CTGitPath path;

	if(GetWinPathString().Left(root.GetWinPathString().GetLength()) == root.GetWinPathString())
	{
		CString str=GetWinPathString();
		path.SetFromWin(str.Right(str.GetLength()-root.GetWinPathString().GetLength()-1));
	}
	return path;
}

void CTGitPath::EnsureBackslashPathSet() const
{
	if(m_sBackslashPath.IsEmpty())
	{
		SetBackslashPath(m_sFwdslashPath);
		ATLASSERT(IsEmpty() || !m_sBackslashPath.IsEmpty());
	}
}
void CTGitPath::EnsureFwdslashPathSet() const
{
	if(m_sFwdslashPath.IsEmpty())
	{
		SetFwdslashPath(m_sBackslashPath);
		ATLASSERT(IsEmpty() || !m_sFwdslashPath.IsEmpty());
	}
}


// Reset all the caches
void CTGitPath::Reset()
{
	m_bDirectoryKnown = false;
	m_bURLKnown = false;
	m_bLastWriteTimeKnown = false;
	m_bHasAdminDirKnown = false;
	m_bIsValidOnWindowsKnown = false;
	m_bIsAdminDirKnown = false;
	m_bExistsKnown = false;
	m_bIsSpecialDirectoryKnown = false;
	m_bIsSpecialDirectory = false;

	m_sBackslashPath.Empty();
	m_sFwdslashPath.Empty();
	m_sUTF8FwdslashPath.Empty();
	this->m_Action=0;
	this->m_StatAdd=_T("");
	this->m_StatDel=_T("");
	m_ParentNo=0;
	ATLASSERT(IsEmpty());
}

CTGitPath CTGitPath::GetDirectory() const
{
	if ((IsDirectory())||(!Exists()))
	{
		return *this;
	}
	return GetContainingDirectory();
}

CTGitPath CTGitPath::GetContainingDirectory() const
{
	EnsureBackslashPathSet();

	CString sDirName = m_sBackslashPath.Left(m_sBackslashPath.ReverseFind('\\'));
	if(sDirName.GetLength() == 2 && sDirName[1] == ':')
	{
		// This is a root directory, which needs a trailing slash
		sDirName += '\\';
		if(sDirName == m_sBackslashPath)
		{
			// We were clearly provided with a root path to start with - we should return nothing now
			sDirName.Empty();
		}
	}
	if(sDirName.GetLength() == 1 && sDirName[0] == '\\')
	{
		// We have an UNC path and we already are the root
		sDirName.Empty();
	}
	CTGitPath retVal;
	retVal.SetFromWin(sDirName);
	return retVal;
}

CString CTGitPath::GetRootPathString() const
{
	EnsureBackslashPathSet();
	CString workingPath = m_sBackslashPath;
	LPTSTR pPath = workingPath.GetBuffer(MAX_PATH);		// MAX_PATH ok here.
	ATLVERIFY(::PathStripToRoot(pPath));
	workingPath.ReleaseBuffer();
	return workingPath;
}


CString CTGitPath::GetFilename() const
{
	//ATLASSERT(!IsDirectory());
	return GetFileOrDirectoryName();
}

CString CTGitPath::GetFileOrDirectoryName() const
{
	EnsureBackslashPathSet();
	return m_sBackslashPath.Mid(m_sBackslashPath.ReverseFind('\\')+1);
}

CString CTGitPath::GetUIFileOrDirectoryName() const
{
	GetUIPathString();
	return m_sUIPath.Mid(m_sUIPath.ReverseFind('\\')+1);
}

CString CTGitPath::GetFileExtension() const
{
	if(!IsDirectory())
	{
		EnsureBackslashPathSet();
		int dotPos = m_sBackslashPath.ReverseFind('.');
		int slashPos = m_sBackslashPath.ReverseFind('\\');
		if (dotPos > slashPos)
			return m_sBackslashPath.Mid(dotPos);
	}
	return CString();
}
CString CTGitPath::GetBaseFilename() const
{
	int dot;
	CString filename=GetFilename();
	dot = filename.ReverseFind(_T('.'));
	if(dot>0)
		return filename.Left(dot);
	else
		return filename;
}

bool CTGitPath::ArePathStringsEqual(const CString& sP1, const CString& sP2)
{
	int length = sP1.GetLength();
	if(length != sP2.GetLength())
	{
		// Different lengths
		return false;
	}
	// We work from the end of the strings, because path differences
	// are more likely to occur at the far end of a string
	LPCTSTR pP1Start = sP1;
	LPCTSTR pP1 = pP1Start+(length-1);
	LPCTSTR pP2 = ((LPCTSTR)sP2)+(length-1);
	while(length-- > 0)
	{
		if(_totlower(*pP1--) != _totlower(*pP2--))
		{
			return false;
		}
	}
	return true;
}

bool CTGitPath::ArePathStringsEqualWithCase(const CString& sP1, const CString& sP2)
{
	int length = sP1.GetLength();
	if(length != sP2.GetLength())
	{
		// Different lengths
		return false;
	}
	// We work from the end of the strings, because path differences
	// are more likely to occur at the far end of a string
	LPCTSTR pP1Start = sP1;
	LPCTSTR pP1 = pP1Start+(length-1);
	LPCTSTR pP2 = ((LPCTSTR)sP2)+(length-1);
	while(length-- > 0)
	{
		if((*pP1--) != (*pP2--))
		{
			return false;
		}
	}
	return true;
}

bool CTGitPath::IsEmpty() const
{
	// Check the backward slash path first, since the chance that this
	// one is set is higher. In case of a 'false' return value it's a little
	// bit faster.
	return m_sBackslashPath.IsEmpty() && m_sFwdslashPath.IsEmpty();
}

// Test if both paths refer to the same item
// Ignores case and slash direction
bool CTGitPath::IsEquivalentTo(const CTGitPath& rhs) const
{
	// Try and find a slash direction which avoids having to convert
	// both filenames
	if(!m_sBackslashPath.IsEmpty())
	{
		// *We've* got a \ path - make sure that the RHS also has a \ path
		rhs.EnsureBackslashPathSet();
		return ArePathStringsEqualWithCase(m_sBackslashPath, rhs.m_sBackslashPath);
	}
	else
	{
		// Assume we've got a fwdslash path and make sure that the RHS has one
		rhs.EnsureFwdslashPathSet();
		return ArePathStringsEqualWithCase(m_sFwdslashPath, rhs.m_sFwdslashPath);
	}
}

bool CTGitPath::IsEquivalentToWithoutCase(const CTGitPath& rhs) const
{
	// Try and find a slash direction which avoids having to convert
	// both filenames
	if(!m_sBackslashPath.IsEmpty())
	{
		// *We've* got a \ path - make sure that the RHS also has a \ path
		rhs.EnsureBackslashPathSet();
		return ArePathStringsEqual(m_sBackslashPath, rhs.m_sBackslashPath);
	}
	else
	{
		// Assume we've got a fwdslash path and make sure that the RHS has one
		rhs.EnsureFwdslashPathSet();
		return ArePathStringsEqual(m_sFwdslashPath, rhs.m_sFwdslashPath);
	}
}

bool CTGitPath::IsAncestorOf(const CTGitPath& possibleDescendant) const
{
	possibleDescendant.EnsureBackslashPathSet();
	EnsureBackslashPathSet();

	bool bPathStringsEqual = ArePathStringsEqual(m_sBackslashPath, possibleDescendant.m_sBackslashPath.Left(m_sBackslashPath.GetLength()));
	if (m_sBackslashPath.GetLength() >= possibleDescendant.GetWinPathString().GetLength())
	{
		return bPathStringsEqual;
	}

	return (bPathStringsEqual &&
			((possibleDescendant.m_sBackslashPath[m_sBackslashPath.GetLength()] == '\\')||
			(m_sBackslashPath.GetLength()==3 && m_sBackslashPath[1]==':')));
}

// Get a string representing the file path, optionally with a base
// section stripped off the front.
CString CTGitPath::GetDisplayString(const CTGitPath* pOptionalBasePath /* = NULL*/) const
{
	EnsureFwdslashPathSet();
	if(pOptionalBasePath != NULL)
	{
		// Find the length of the base-path without having to do an 'ensure' on it
		int baseLength = max(pOptionalBasePath->m_sBackslashPath.GetLength(), pOptionalBasePath->m_sFwdslashPath.GetLength());

		// Now, chop that baseLength of the front of the path
		return m_sFwdslashPath.Mid(baseLength).TrimLeft('/');
	}
	return m_sFwdslashPath;
}

int CTGitPath::Compare(const CTGitPath& left, const CTGitPath& right)
{
	left.EnsureBackslashPathSet();
	right.EnsureBackslashPathSet();
	return left.m_sBackslashPath.CompareNoCase(right.m_sBackslashPath);
}

bool operator<(const CTGitPath& left, const CTGitPath& right)
{
	return CTGitPath::Compare(left, right) < 0;
}

bool CTGitPath::PredLeftEquivalentToRight(const CTGitPath& left, const CTGitPath& right)
{
	return left.IsEquivalentTo(right);
}

bool CTGitPath::PredLeftSameWCPathAsRight(const CTGitPath& left, const CTGitPath& right)
{
	if (left.IsAdminDir() && right.IsAdminDir())
	{
		CTGitPath l = left;
		CTGitPath r = right;
		do
		{
			l = l.GetContainingDirectory();
		} while(l.HasAdminDir());
		do
		{
			r = r.GetContainingDirectory();
		} while(r.HasAdminDir());
		return l.GetContainingDirectory().IsEquivalentTo(r.GetContainingDirectory());
	}
	return left.GetDirectory().IsEquivalentTo(right.GetDirectory());
}

bool CTGitPath::CheckChild(const CTGitPath &parent, const CTGitPath& child)
{
	return parent.IsAncestorOf(child);
}

void CTGitPath::AppendRawString(const CString& sAppend)
{
	EnsureFwdslashPathSet();
	CString strCopy = m_sFwdslashPath += sAppend;
	SetFromUnknown(strCopy);
}

void CTGitPath::AppendPathString(const CString& sAppend)
{
	EnsureBackslashPathSet();
	CString cleanAppend(sAppend);
	cleanAppend.Replace('/', '\\');
	cleanAppend.TrimLeft('\\');
	m_sBackslashPath.TrimRight('\\');
	CString strCopy = m_sBackslashPath + _T("\\") + cleanAppend;
	SetFromWin(strCopy);
}

bool CTGitPath::IsWCRoot() const
{
	if (m_bIsWCRootKnown)
		return m_bIsWCRoot;

	m_bIsWCRootKnown = true;
	m_bIsWCRoot = false;

	CString topDirectory;
	if (!IsDirectory() || !HasAdminDir(&topDirectory))
	{
		return m_bIsWCRoot;
	}

	if (IsEquivalentToWithoutCase(topDirectory))
	{
		m_bIsWCRoot = true;
	}

	return m_bIsWCRoot;
}

bool CTGitPath::HasAdminDir() const
{
	if (m_bHasAdminDirKnown)
		return m_bHasAdminDir;

	EnsureBackslashPathSet();
	m_bHasAdminDir = GitAdminDir::HasAdminDir(m_sBackslashPath, IsDirectory(), &m_sProjectRoot);
	m_bHasAdminDirKnown = true;
	return m_bHasAdminDir;
}

bool CTGitPath::HasSubmodules() const
{
	if (HasAdminDir())
	{
		CString path = m_sProjectRoot;
		path += _T("\\.gitmodules");
		if( PathFileExists(path) )
			return true;
	}
	return false;
}

int CTGitPath::GetAdminDirMask() const
{
	int status = 0;
	CString topdir;
	if (!GitAdminDir::HasAdminDir(GetWinPathString(), &topdir))
	{
		return status;
	}

	// ITEMIS_INGIT will be revoked if necessary in TortoiseShell/ContextMenu.cpp
	status |= ITEMIS_INGIT|ITEMIS_INVERSIONEDFOLDER;

	if (IsDirectory())
	{
		status |= ITEMIS_FOLDERINGIT;
		if (IsWCRoot())
		{
			status |= ITEMIS_WCROOT;

			CString topProjectDir;
			if (GitAdminDir::HasAdminDir(GetWinPathString(), false, &topProjectDir))
			{
				if (PathFileExists(topProjectDir + _T("\\.gitmodules")))
				{
					CAutoConfig config(true);
					git_config_add_file_ondisk(config, CGit::GetGitPathStringA(topProjectDir + _T("\\.gitmodules")), GIT_CONFIG_LEVEL_APP, FALSE);
					CString relativePath = GetWinPathString().Mid(topProjectDir.GetLength());
					relativePath.Replace(_T("\\"), _T("/"));
					relativePath.Trim(_T("/"));
					CStringA submodulePath = CUnicodeUtils::GetUTF8(relativePath);
					if (git_config_foreach_match(config, "submodule\\..*\\.path", 
						[](const git_config_entry *entry, void *data) { return entry->value == *(CStringA *)data ? GIT_EUSER : 0; }, &submodulePath) == GIT_EUSER)
						status |= ITEMIS_SUBMODULE;
				}
			}
		}
	}

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(topdir, dotGitPath);

	if (PathFileExists(dotGitPath + _T("BISECT_START")))
		status |= ITEMIS_BISECT;

	if (PathFileExists(dotGitPath + _T("MERGE_HEAD")))
		status |= ITEMIS_MERGEACTIVE;

	if (PathFileExists(dotGitPath + _T("refs\\stash")))
		status |= ITEMIS_STASH;

	if (PathFileExists(dotGitPath + _T("svn")))
		status |= ITEMIS_GITSVN;

	if (PathFileExists(topdir + _T("\\.gitmodules")))
		status |= ITEMIS_SUBMODULECONTAINER;

	return status;
}

bool CTGitPath::HasStashDir() const
{
	CString topdir;
	if(!GitAdminDir::HasAdminDir(GetWinPathString(),&topdir))
	{
		return false;
	}

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(topdir, dotGitPath);

	return !!PathFileExists(dotGitPath + _T("\\refs\\stash"));
}
bool CTGitPath::HasGitSVNDir() const
{
	CString topdir;
	if (!GitAdminDir::HasAdminDir(GetWinPathString(), &topdir))
	{
		return false;
	}

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(topdir, dotGitPath);

	return !!PathFileExists(dotGitPath + _T("svn"));
}
bool CTGitPath::IsBisectActive() const
{
	CString topdir;
	if (!GitAdminDir::HasAdminDir(GetWinPathString(), &topdir))
	{
		return false;
	}

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(topdir, dotGitPath);

	return !!PathFileExists(dotGitPath + _T("BISECT_START"));
}
bool CTGitPath::IsMergeActive() const
{
	CString topdir;
	if (!GitAdminDir::HasAdminDir(GetWinPathString(), &topdir))
	{
		return false;
	}

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(topdir, dotGitPath);

	return !!PathFileExists(dotGitPath + _T("MERGE_HEAD"));
}
bool CTGitPath::HasRebaseApply() const
{
	CString topdir;
	if (!GitAdminDir::HasAdminDir(GetWinPathString(), &topdir))
	{
		return false;
	}

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(topdir, dotGitPath);

	return !!PathFileExists(dotGitPath + _T("rebase-apply"));
}

bool CTGitPath::HasAdminDir(CString *ProjectTopDir) const
{
	if (m_bHasAdminDirKnown)
	{
		if (ProjectTopDir)
			*ProjectTopDir = m_sProjectRoot;
		return m_bHasAdminDir;
	}

	EnsureBackslashPathSet();
	m_bHasAdminDir = GitAdminDir::HasAdminDir(m_sBackslashPath, IsDirectory(), &m_sProjectRoot);
	m_bHasAdminDirKnown = true;
	if (ProjectTopDir)
		*ProjectTopDir = m_sProjectRoot;
	return m_bHasAdminDir;
}

bool CTGitPath::IsAdminDir() const
{
	if (m_bIsAdminDirKnown)
		return m_bIsAdminDir;

	EnsureBackslashPathSet();
	m_bIsAdminDir = GitAdminDir::IsAdminDirPath(m_sBackslashPath);
	m_bIsAdminDirKnown = true;
	return m_bIsAdminDir;
}

bool CTGitPath::IsValidOnWindows() const
{
	if (m_bIsValidOnWindowsKnown)
		return m_bIsValidOnWindows;

	m_bIsValidOnWindows = false;
	EnsureBackslashPathSet();
	CString sMatch = m_sBackslashPath + _T("\r\n");
	std::wstring sPattern;
	// the 'file://' URL is just a normal windows path:
	if (sMatch.Left(7).CompareNoCase(_T("file:\\\\"))==0)
	{
		sMatch = sMatch.Mid(7);
		sMatch.TrimLeft(_T("\\"));
		sPattern = _T("^(\\\\\\\\\\?\\\\)?(([a-zA-Z]:|\\\\)\\\\)?(((\\.)|(\\.\\.)|([^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?))\\\\)*[^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?$");
	}
	else
	{
		sPattern = _T("^(\\\\\\\\\\?\\\\)?(([a-zA-Z]:|\\\\)\\\\)?(((\\.)|(\\.\\.)|([^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?))\\\\)*[^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?$");
	}

	try
	{
		std::tr1::wregex rx(sPattern, std::tr1::regex_constants::icase | std::tr1::regex_constants::ECMAScript);
		std::tr1::wsmatch match;

		std::wstring rmatch = std::wstring((LPCTSTR)sMatch);
		if (std::tr1::regex_match(rmatch, match, rx))
		{
			if (std::wstring(match[0]).compare(sMatch)==0)
				m_bIsValidOnWindows = true;
		}
		if (m_bIsValidOnWindows)
		{
			// now check for illegal filenames
			std::tr1::wregex rx2(_T("\\\\(lpt\\d|com\\d|aux|nul|prn|con)(\\\\|$)"), std::tr1::regex_constants::icase | std::tr1::regex_constants::ECMAScript);
			rmatch = m_sBackslashPath;
			if (std::tr1::regex_search(rmatch, rx2, std::tr1::regex_constants::match_default))
				m_bIsValidOnWindows = false;
		}
	}
	catch (std::exception) {}

	m_bIsValidOnWindowsKnown = true;
	return m_bIsValidOnWindows;
}

//////////////////////////////////////////////////////////////////////////

CTGitPathList::CTGitPathList()
{
	m_Action = 0;
}

// A constructor which allows a path list to be easily built which one initial entry in
CTGitPathList::CTGitPathList(const CTGitPath& firstEntry)
{
	m_Action = 0;
	AddPath(firstEntry);
}
int CTGitPathList::ParserFromLsFile(BYTE_VECTOR &out,bool /*staged*/)
{
	int pos=0;
	CString one;
	CTGitPath path;
	CString part;
	this->Clear();

	while (pos >= 0 && pos < (int)out.size())
	{
		one.Empty();
		path.Reset();

		CGit::StringAppend(&one, &out[pos], CP_UTF8);
		int tabstart=0;
		// m_Action is never used and propably never worked (needs to be set after path.SetFromGit)
		// also dropped LOGACTIONS_CACHE for 'H'
		// path.m_Action=path.ParserAction(out[pos]);
		one.Tokenize(_T("\t"),tabstart);

		if(tabstart>=0)
			path.SetFromGit(one.Right(one.GetLength()-tabstart));
		else
			return -1;

		tabstart=0;

		part=one.Tokenize(_T(" "),tabstart); //Tag
		if (tabstart < 0)
			return -1;

		part=one.Tokenize(_T(" "),tabstart); //Mode
		if (tabstart < 0)
			return -1;

		part=one.Tokenize(_T(" "),tabstart); //Hash
		if (tabstart < 0)
			return -1;

		part=one.Tokenize(_T("\t"),tabstart); //Stage
		if (tabstart < 0)
			return -1;

		path.m_Stage=_ttol(part);

		this->AddPath(path);

		pos=out.findNextString(pos);
	}
	return 0;
}
int CTGitPathList::FillUnRev(unsigned int action, CTGitPathList *list, CString *err)
{
	this->Clear();
	CTGitPath path;

	int count;
	if(list==NULL)
		count=1;
	else
		count=list->GetCount();
	for (int i = 0; i < count; ++i)
	{
		CString cmd;
		int pos = 0;

		CString ignored;
		if(action & CTGitPath::LOGACTIONS_IGNORE)
			ignored= _T(" -i");

		if(list==NULL)
		{
			cmd=_T("git.exe ls-files --exclude-standard --full-name --others -z"); // are just null terminated
			cmd+=ignored;

		}
		else
		{	cmd.Format(_T("git.exe ls-files --exclude-standard --full-name --others -z%s -- \"%s\""),
					ignored,
					(*list)[i].GetWinPathString());
		}

		BYTE_VECTOR out, errb;
		out.clear();
		if (g_Git.Run(cmd, &out, &errb))
		{
			if (err != nullptr)
				CGit::StringAppend(err, &errb[0], CP_UTF8, (int)errb.size());
			return -1;
		}

		pos=0;
		CString one;
		while (pos >= 0 && pos < (int)out.size())
		{
			one.Empty();
			CGit::StringAppend(&one, &out[pos], CP_UTF8);
			if(!one.IsEmpty())
			{
				//SetFromGit will clear all status
				path.SetFromGit(one);
				path.m_Action=action;
				AddPath(path);
			}
			pos=out.findNextString(pos);
		}

	}
	return 0;
}
int CTGitPathList::FillBasedOnIndexFlags(unsigned short flag, CTGitPathList* list /*nullptr*/)
{
	Clear();
	CTGitPath path;

	CAutoRepository repository(g_Git.GetGitRepository());
	if (!repository)
		return -1;

	CAutoIndex index;
	if (git_repository_index(index.GetPointer(), repository))
		return -1;

	int count;
	if (list == nullptr)
		count = 1;
	else
		count = list->GetCount();
	for (int j = 0; j < count; ++j)
	{
		for (size_t i = 0, ecount = git_index_entrycount(index); i < ecount; ++i)
		{
			const git_index_entry *e = git_index_get_byindex(index, i);

			if (!e || !((e->flags | e->flags_extended) & flag) || !e->path)
				continue;

			CString one = CUnicodeUtils::GetUnicode(e->path);

			if (!(!list || (*list)[j].GetWinPathString().IsEmpty() || one == (*list)[j].GetGitPathString() || (PathIsDirectory(g_Git.CombinePath((*list)[j].GetWinPathString())) && one.Find((*list)[j].GetGitPathString() + _T("/")) == 0)))
				continue;

			//SetFromGit will clear all status
			path.SetFromGit(one);
			if ((e->flags | e->flags_extended) & GIT_IDXENTRY_SKIP_WORKTREE)
				path.m_Action = CTGitPath::LOGACTIONS_SKIPWORKTREE;
			else if ((e->flags | e->flags_extended) & GIT_IDXENTRY_VALID)
				path.m_Action = CTGitPath::LOGACTIONS_ASSUMEVALID;
			AddPath(path);
		}
	}
	RemoveDuplicates();
	return 0;
}
int CTGitPathList::ParserFromLog(BYTE_VECTOR &log, bool parseDeletes /*false*/)
{
	this->Clear();
	int pos=0;
	CTGitPath path;
	m_Action=0;
	int logend = (int)log.size();
	while (pos >= 0 && pos < logend)
	{
		path.Reset();
		if(log[pos]=='\n')
			++pos;

		if (pos >= logend)
			return -1;

		if(log[pos]==':')
		{
			bool merged=false;
			if (pos + 1 >= logend)
				return -1;
			if(log[pos+1] ==':')
			{
				merged=true;
			}
			int end=log.find(0,pos);
			int actionstart=-1;
			int file1=-1,file2=-1;
			if( end>0 )
			{
				actionstart=log.find(' ',end-6);
				pos=actionstart;
			}
			if( actionstart>0 )
			{
				++actionstart;
				if (actionstart >= logend)
					return -1;

				file1 = log.find(0,actionstart);
				if( file1>=0 )
				{
					++file1;
					pos=file1;
				}
				if( log[actionstart] == 'C' || log[actionstart] == 'R' )
				{
					file2=file1;
					file1 = log.find(0,file1);
					if(file1>=0 )
					{
						++file1;
						pos=file1;
					}

				}
			}

			CString pathname1;
			CString pathname2;

			if( file1>=0 )
				CGit::StringAppend(&pathname1, &log[file1], CP_UTF8);
			if( file2>=0 )
				CGit::StringAppend(&pathname2, &log[file2], CP_UTF8);

			CTGitPath *GitPath=LookForGitPath(pathname1);

			if (actionstart < 0)
				return -1;
			if(GitPath)
			{
				GitPath->ParserAction( log[actionstart] );

				if(merged)
				{
					GitPath->m_Action |= CTGitPath::LOGACTIONS_MERGED;
				}
				m_Action |=GitPath->m_Action;

			}
			else
			{
				int ac=path.ParserAction(log[actionstart] );
				ac |= merged?CTGitPath::LOGACTIONS_MERGED:0;

				path.SetFromGit(pathname1,&pathname2);
				path.m_Action=ac;
					//action must be set after setfromgit. SetFromGit will clear all status.
				this->m_Action|=ac;

				AddPath(path);

			}

		}
		else
		{
			int tabstart=0;
			path.Reset();
			CString StatAdd;
			CString StatDel;
			CString file1;
			CString file2;

			tabstart=log.find('\t',pos);
			if(tabstart >=0)
			{
				log[tabstart]=0;
				CGit::StringAppend(&StatAdd, &log[pos], CP_UTF8);
				pos=tabstart+1;
			}

			tabstart=log.find('\t',pos);
			if(tabstart >=0)
			{
				log[tabstart]=0;

				CGit::StringAppend(&StatDel, &log[pos], CP_UTF8);
				pos=tabstart+1;
			}

			if(log[pos] == 0) //rename
			{
				++pos;
				CGit::StringAppend(&file2, &log[pos], CP_UTF8);
				int sec=log.find(0,pos);
				if(sec>=0)
				{
					++sec;
					CGit::StringAppend(&file1, &log[sec], CP_UTF8);
				}
				pos=sec;

			}
			else
			{
				CGit::StringAppend(&file1, &log[pos], CP_UTF8);
			}
			path.SetFromGit(file1,&file2);

			CTGitPath *GitPath=LookForGitPath(path.GetGitPathString());
			if(GitPath)
			{
				GitPath->m_StatAdd=StatAdd;
				GitPath->m_StatDel=StatDel;
			}
			else
			{
				//path.SetFromGit(pathname);
				if (parseDeletes)
				{
					path.m_StatAdd=_T("0");
					path.m_StatDel=_T("0");
					path.m_Action |= CTGitPath::LOGACTIONS_DELETED;
				}
				else
				{
					path.m_StatAdd=StatAdd;
					path.m_StatDel=StatDel;
				}
				AddPath(path);
			}

		}
		pos=log.findNextString(pos);
	}
	return 0;
}

void CTGitPathList::AddPath(const CTGitPath& newPath)
{
	m_paths.push_back(newPath);
	m_commonBaseDirectory.Reset();
}
int CTGitPathList::GetCount() const
{
	return (int)m_paths.size();
}
bool CTGitPathList::IsEmpty() const
{
	return m_paths.empty();
}
void CTGitPathList::Clear()
{
	m_paths.clear();
	m_commonBaseDirectory.Reset();
}

const CTGitPath& CTGitPathList::operator[](INT_PTR index) const
{
	ATLASSERT(index >= 0 && index < (INT_PTR)m_paths.size());
	return m_paths[index];
}

bool CTGitPathList::AreAllPathsFiles() const
{
	// Look through the vector for any directories - if we find them, return false
	return std::find_if(m_paths.begin(), m_paths.end(), std::mem_fun_ref(&CTGitPath::IsDirectory)) == m_paths.end();
}


#if defined(_MFC_VER)

bool CTGitPathList::LoadFromFile(const CTGitPath& filename)
{
	Clear();
	try
	{
		CString strLine;
		CStdioFile file(filename.GetWinPath(), CFile::typeBinary | CFile::modeRead | CFile::shareDenyWrite);

		// for every selected file/folder
		CTGitPath path;
		while (file.ReadString(strLine))
		{
			path.SetFromUnknown(strLine);
			AddPath(path);
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading target file list\n");
		TCHAR error[10000] = {0};
		pE->GetErrorMessage(error, 10000);
//		CMessageBox::Show(NULL, error, _T("TortoiseGit"), MB_ICONERROR);
		pE->Delete();
		return false;
	}
	return true;
}

bool CTGitPathList::WriteToFile(const CString& sFilename, bool bANSI /* = false */) const
{
	try
	{
		if (bANSI)
		{
			CStdioFile file(sFilename, CFile::typeText | CFile::modeReadWrite | CFile::modeCreate);
			PathVector::const_iterator it;
			for(it = m_paths.begin(); it != m_paths.end(); ++it)
			{
				CStringA line = CStringA(it->GetGitPathString()) + '\n';
				file.Write(line, line.GetLength());
			}
			file.Close();
		}
		else
		{
			CStdioFile file(sFilename, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
			PathVector::const_iterator it;
			for(it = m_paths.begin(); it != m_paths.end(); ++it)
			{
				file.WriteString(it->GetGitPathString()+_T("\n"));
			}
			file.Close();
		}
	}
	catch (CFileException* pE)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException in writing temp file\n");
		pE->Delete();
		return false;
	}
	return true;
}


void CTGitPathList::LoadFromAsteriskSeparatedString(const CString& sPathString)
{
	int pos = 0;
	CString temp;
	for(;;)
	{
		temp = sPathString.Tokenize(_T("*"),pos);
		if(temp.IsEmpty())
		{
			break;
		}
		AddPath(CTGitPath(CPathUtils::GetLongPathname(temp)));
	}
}

CString CTGitPathList::CreateAsteriskSeparatedString() const
{
	CString sRet;
	PathVector::const_iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if (!sRet.IsEmpty())
			sRet += _T("*");
		sRet += it->GetWinPathString();
	}
	return sRet;
}
#endif // _MFC_VER

bool
CTGitPathList::AreAllPathsFilesInOneDirectory() const
{
	// Check if all the paths are files and in the same directory
	PathVector::const_iterator it;
	m_commonBaseDirectory.Reset();
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if(it->IsDirectory())
		{
			return false;
		}
		const CTGitPath& baseDirectory = it->GetDirectory();
		if(m_commonBaseDirectory.IsEmpty())
		{
			m_commonBaseDirectory = baseDirectory;
		}
		else if(!m_commonBaseDirectory.IsEquivalentTo(baseDirectory))
		{
			// Different path
			m_commonBaseDirectory.Reset();
			return false;
		}
	}
	return true;
}

CTGitPath CTGitPathList::GetCommonDirectory() const
{
	if (m_commonBaseDirectory.IsEmpty())
	{
		PathVector::const_iterator it;
		for(it = m_paths.begin(); it != m_paths.end(); ++it)
		{
			const CTGitPath& baseDirectory = it->GetDirectory();
			if(m_commonBaseDirectory.IsEmpty())
			{
				m_commonBaseDirectory = baseDirectory;
			}
			else if(!m_commonBaseDirectory.IsEquivalentTo(baseDirectory))
			{
				// Different path
				m_commonBaseDirectory.Reset();
				break;
			}
		}
	}
	// since we only checked strings, not paths,
	// we have to make sure now that we really return a *path* here
	PathVector::const_iterator iter;
	for(iter = m_paths.begin(); iter != m_paths.end(); ++iter)
	{
		if (!m_commonBaseDirectory.IsAncestorOf(*iter))
		{
			m_commonBaseDirectory = m_commonBaseDirectory.GetContainingDirectory();
			break;
		}
	}
	return m_commonBaseDirectory;
}

CTGitPath CTGitPathList::GetCommonRoot() const
{
	if (IsEmpty())
		return CTGitPath();

	if (GetCount() == 1)
		return m_paths[0];

	// first entry is common root for itself
	// (add trailing '\\' to detect partial matches of the last path element)
	CString root = m_paths[0].GetWinPathString() + _T('\\');
	int rootLength = root.GetLength();

	// determine common path string prefix
	for (PathVector::const_iterator it = m_paths.begin() + 1; it != m_paths.end(); ++it)
	{
		CString path = it->GetWinPathString() + _T('\\');

		int newLength = CStringUtils::GetMatchingLength(root, path);
		if (newLength != rootLength)
		{
			root.Delete(newLength, rootLength);
			rootLength = newLength;
		}
	}

	// remove the last (partial) path element
	if (rootLength > 0)
		root.Delete(root.ReverseFind(_T('\\')), rootLength);

	// done
	return CTGitPath(root);
}

void CTGitPathList::SortByPathname(bool bReverse /*= false*/)
{
	std::sort(m_paths.begin(), m_paths.end());
	if (bReverse)
		std::reverse(m_paths.begin(), m_paths.end());
}

void CTGitPathList::DeleteAllFiles(bool bTrash, bool bFilesOnly)
{
	PathVector::const_iterator it;
	SortByPathname(true); // nested ones first

	CString sPaths;
	for (it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if ((it->Exists()) && ((it->IsDirectory() != bFilesOnly) || !bFilesOnly))
		{
			if (!it->IsDirectory())
				::SetFileAttributes(it->GetWinPath(), FILE_ATTRIBUTE_NORMAL);

			sPaths += it->GetWinPath();
			sPaths += '\0';
		}
	}
	sPaths += '\0';
	sPaths += '\0';
	DeleteViaShell((LPCTSTR)sPaths, bTrash);
	Clear();
}

bool CTGitPathList::DeleteViaShell(LPCTSTR path, bool bTrash)
{
	SHFILEOPSTRUCT shop = {0};
	shop.wFunc = FO_DELETE;
	shop.pFrom = path;
	shop.fFlags = FOF_NOCONFIRMATION|FOF_NOERRORUI|FOF_SILENT|FOF_NO_CONNECTED_ELEMENTS;
	if (bTrash)
		shop.fFlags |= FOF_ALLOWUNDO;
	const bool bRet = (SHFileOperation(&shop) == 0);
	return bRet;
}

void CTGitPathList::RemoveDuplicates()
{
	SortByPathname();
	// Remove the duplicates
	// (Unique moves them to the end of the vector, then erase chops them off)
	m_paths.erase(std::unique(m_paths.begin(), m_paths.end(), &CTGitPath::PredLeftEquivalentToRight), m_paths.end());
}

void CTGitPathList::RemoveAdminPaths()
{
	PathVector::iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); )
	{
		if (it->IsAdminDir())
		{
			m_paths.erase(it);
			it = m_paths.begin();
		}
		else
			++it;
	}
}

void CTGitPathList::RemovePath(const CTGitPath& path)
{
	PathVector::iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if (it->IsEquivalentTo(path))
		{
			m_paths.erase(it);
			return;
		}
	}
}

void CTGitPathList::RemoveItem(CTGitPath & path)
{
	PathVector::iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if (it->GetGitPathString()==path.GetGitPathString())
		{
			m_paths.erase(it);
			return;
		}
	}
}
void CTGitPathList::RemoveChildren()
{
	SortByPathname();
	m_paths.erase(std::unique(m_paths.begin(), m_paths.end(), &CTGitPath::CheckChild), m_paths.end());
}

bool CTGitPathList::IsEqual(const CTGitPathList& list)
{
	if (list.GetCount() != GetCount())
		return false;
	for (int i=0; i<list.GetCount(); ++i)
	{
		if (!list[i].IsEquivalentTo(m_paths[i]))
			return false;
	}
	return true;
}

CTGitPath * CTGitPathList::LookForGitPath(CString path)
{
	int i=0;
	for (i = 0; i < this->GetCount(); ++i)
	{
		if((*this)[i].GetGitPathString() == path )
			return (CTGitPath*)&(*this)[i];
	}
	return NULL;
}
CString CTGitPath::GetActionName(int action)
{
	if(action  & CTGitPath::LOGACTIONS_UNMERGED)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_CONFLICT);
	if(action  & CTGitPath::LOGACTIONS_ADDED)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_ADD);
	if(action  & CTGitPath::LOGACTIONS_DELETED)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_DELETE);
	if(action  & CTGitPath::LOGACTIONS_MERGED )
		return MAKEINTRESOURCE(IDS_PATHACTIONS_MERGED);

	if(action  & CTGitPath::LOGACTIONS_MODIFIED)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_MODIFIED);
	if(action  & CTGitPath::LOGACTIONS_REPLACED)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_RENAME);
	if(action  & CTGitPath::LOGACTIONS_COPY)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_COPY);

	if (action & CTGitPath::LOGACTIONS_ASSUMEVALID)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_ASSUMEUNCHANGED);
	if (action & CTGitPath::LOGACTIONS_SKIPWORKTREE)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_SKIPWORKTREE);

	return MAKEINTRESOURCE(IDS_PATHACTIONS_UNKNOWN);
}
CString CTGitPath::GetActionName()
{
	return GetActionName(m_Action);
}

int CTGitPathList::GetAction()
{
	return m_Action;
}

