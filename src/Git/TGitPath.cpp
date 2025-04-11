﻿// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2023, 2025 - TortoiseGit
// Copyright (C) 2003-2008, 2025 - TortoiseSVN

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
#include "SmartHandle.h"
#include "../Resources/LoglistCommonResource.h"
#include <sys/stat.h>

#ifdef TGIT_LFS
#include "nlohmann/json.hpp"
using json = nlohmann::json;
#endif

extern CGit g_Git;

CTGitPath::CTGitPath()
{
}

CTGitPath::~CTGitPath()
{
}
// Create a TGitPath object from an unknown path type (same as using SetFromUnknown)
CTGitPath::CTGitPath(const CString& sUnknownPath) : CTGitPath()
{
	SetFromUnknown(sUnknownPath);
}

CTGitPath::CTGitPath(const CString& sUnknownPath, bool bIsDirectory) : CTGitPath(sUnknownPath)
{
	m_bDirectoryKnown = true;
	m_bIsDirectory = bIsDirectory;
}

unsigned int CTGitPath::ParseStatus(const char status)
{
	switch (status)
	{
	case 'M':
		return LOGACTIONS_MODIFIED;
	case 'R':
		return LOGACTIONS_REPLACED;
	case 'A':
		return LOGACTIONS_ADDED;
	case 'D':
		return LOGACTIONS_DELETED;
	case 'U':
		return LOGACTIONS_UNMERGED;
	case 'K':
		return LOGACTIONS_DELETED;
	case 'C':
		return LOGACTIONS_COPY;
	case 'T':
		return LOGACTIONS_MODIFIED;
	default:
		return 0;
	}
}

unsigned int CTGitPath::ParseAndUpdateStatus(git_delta_t status)
{
	if (status == GIT_DELTA_MODIFIED)
		m_Action |= LOGACTIONS_MODIFIED;
	if (status == GIT_DELTA_RENAMED)
		m_Action |= LOGACTIONS_REPLACED;
	if (status == GIT_DELTA_ADDED)
		m_Action |= LOGACTIONS_ADDED;
	if (status == GIT_DELTA_DELETED)
		m_Action |= LOGACTIONS_DELETED;
	if (status == GIT_DELTA_UNMODIFIED)
		m_Action |= LOGACTIONS_UNMERGED;
	if (status == GIT_DELTA_COPIED)
		m_Action |= LOGACTIONS_COPY;
	if (status == GIT_DELTA_TYPECHANGE)
		m_Action |= LOGACTIONS_MODIFIED;

	return m_Action;
}

void CTGitPath::SetFromGit(const char* pPath)
{
	Reset();
	ATLASSERT(pPath);
	if (!pPath)
		return;
	m_sFwdslashPath = CUnicodeUtils::GetUnicode(pPath);
	SanitizeRootPath(m_sFwdslashPath, true);
}

void CTGitPath::SetFromGit(const char* pPath, bool bIsDirectory)
{
	SetFromGit(pPath);
	m_bDirectoryKnown = true;
	m_bIsDirectory = bIsDirectory;
}

void CTGitPath::SetFromGit(const wchar_t* pPath, bool bIsDirectory)
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

void CTGitPath::SetFromGit(const CString& sPath, CString* oldpath, int* bIsDirectory)
{
	Reset();
	m_sFwdslashPath = sPath;
	SanitizeRootPath(m_sFwdslashPath, true);
	if (bIsDirectory)
	{
		m_bDirectoryKnown = true;
		m_bIsDirectory = *bIsDirectory != FALSE;
	}
	if(oldpath)
		m_sOldFwdslashPath = *oldpath;
}

void CTGitPath::SetFromWin(LPCWSTR pPath)
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
void CTGitPath::SetFromWin(LPCWSTR pPath, bool bIsDirectory)
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

void CTGitPath::UpdateCase()
{
	m_sBackslashPath = CPathUtils::GetLongPathname(GetWinPathString());
	CPathUtils::TrimTrailingPathDelimiter(m_sBackslashPath);
	SanitizeRootPath(m_sBackslashPath, false);
	SetFwdslashPath(m_sBackslashPath);
}

LPCWSTR CTGitPath::GetWinPath() const
{
	if(IsEmpty())
		return L"";
	if(m_sBackslashPath.IsEmpty())
		SetBackslashPath(m_sFwdslashPath);
	return m_sBackslashPath;
}
// This is a temporary function, to be used during the migration to
// the path class.  Ultimately, functions consuming paths should take a CTGitPath&, not a CString
const CString& CTGitPath::GetWinPathString() const
{
	if(m_sBackslashPath.IsEmpty())
		SetBackslashPath(m_sFwdslashPath);
	return m_sBackslashPath;
}

const CString& CTGitPath::GetGitPathString() const
{
	if(m_sFwdslashPath.IsEmpty())
		SetFwdslashPath(m_sBackslashPath);
	return m_sFwdslashPath;
}

const CString &CTGitPath::GetGitOldPathString() const
{
	return m_sOldFwdslashPath;
}

const CString& CTGitPath::GetUIPathString() const
{
	if (m_sUIPath.IsEmpty())
		m_sUIPath = GetWinPathString();
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

	path.Replace(L"file:////", L"file://");
	m_sFwdslashPath = path;
}

void CTGitPath::SetBackslashPath(const CString& sPath) const
{
	CString path = sPath;
	path.Replace('/', '\\');
	path.TrimRight('\\');
	SanitizeRootPath(path, false);
	m_sBackslashPath = path;
}

void CTGitPath::SanitizeRootPath(CString& sPath, bool bIsForwardPath) const
{
	// Make sure to add the trailing slash to root paths such as 'C:'
	if (sPath.GetLength() == 2 && sPath[1] == ':')
		sPath += (bIsForwardPath) ? L'/' : L'\\';
}

bool CTGitPath::IsDirectory() const
{
	if(!m_bDirectoryKnown)
		UpdateAttributes();
	return m_bIsDirectory;
}

bool CTGitPath::Exists() const
{
	if (!m_bExistsKnown)
		UpdateAttributes();
	return m_bExists;
}

bool CTGitPath::Delete(bool bTrash, bool bShowErrorUI) const
{
	EnsureBackslashPathSet();
	::SetFileAttributes(m_sBackslashPath, FILE_ATTRIBUTE_NORMAL);
	bool bRet = false;
	if (Exists())
	{
		if ((bTrash)||(IsDirectory()))
		{
			auto buf = std::make_unique<wchar_t[]>(m_sBackslashPath.GetLength() + 2);
			wcscpy_s(buf.get(), m_sBackslashPath.GetLength() + 2, m_sBackslashPath);
			buf[m_sBackslashPath.GetLength()] = L'\0';
			buf[m_sBackslashPath.GetLength() + 1] = L'\0';
			bRet = CTGitPathList::DeleteViaShell(buf.get(), bTrash, bShowErrorUI);
		}
		else
			bRet = !!::DeleteFile(m_sBackslashPath);
	}
	m_bExists = false;
	m_bExistsKnown = true;
	return bRet;
}

__int64  CTGitPath::GetLastWriteTime(bool force /* = false */) const
{
	if (!m_bLastWriteTimeKnown || force)
		UpdateAttributes();
	return m_lastWriteTime;
}

__int64 CTGitPath::GetFileSize() const
{
	if(!m_bDirectoryKnown)
		UpdateAttributes();
	return m_fileSize;
}

bool CTGitPath::IsReadOnly() const
{
	if(!m_bLastWriteTimeKnown)
		UpdateAttributes();
	return m_bIsReadOnly;
}

void CTGitPath::UpdateAttributes() const
{
	EnsureBackslashPathSet();
	WIN32_FILE_ATTRIBUTE_DATA attribs;
	if (m_sBackslashPath.IsEmpty())
		m_sLongBackslashPath = L".";
	else if (m_sBackslashPath.GetLength() >= 248)
	{
		if (!PathIsRelative(m_sBackslashPath))
			m_sLongBackslashPath = L"\\\\?\\" + m_sBackslashPath;
		else
			m_sLongBackslashPath = L"\\\\?\\" + g_Git.CombinePath(m_sBackslashPath);
	}
	if (GetFileAttributesEx(m_sBackslashPath.IsEmpty() || m_sBackslashPath.GetLength() >= 248 ? m_sLongBackslashPath : m_sBackslashPath, GetFileExInfoStandard, &attribs))
	{
		m_bIsDirectory = !!(attribs.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		// don't cast directly to an __int64:
		// http://msdn.microsoft.com/en-us/library/windows/desktop/ms724284%28v=vs.85%29.aspx
		// "Do not cast a pointer to a FILETIME structure to either a ULARGE_INTEGER* or __int64* value
		// because it can cause alignment faults on 64-bit Windows."
		m_lastWriteTime = static_cast<__int64>(attribs.ftLastWriteTime.dwHighDateTime) << 32 | attribs.ftLastWriteTime.dwLowDateTime;
		if (m_bIsDirectory)
			m_fileSize = 0;
		else
			m_fileSize = static_cast<__int64>(attribs.nFileSizeHigh) << 32 | attribs.nFileSizeLow;
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
			m_bExists = false;
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

CTGitPath CTGitPath::GetSubPath(const CTGitPath& root) const
{
	CTGitPath path;

	if (CStringUtils::StartsWith(GetWinPathString(), root.GetWinPathString()))
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
	m_bLastWriteTimeKnown = false;
	m_bHasAdminDirKnown = false;
	m_bIsValidOnWindowsKnown = false;
	m_bIsAdminDirKnown = false;
	m_bExistsKnown = false;

	m_sBackslashPath.Empty();
	m_sLongBackslashPath.Empty();
	m_sFwdslashPath.Empty();
	m_sUIPath.Empty();
	m_sProjectRoot.Empty();
	m_sOldFwdslashPath.Empty();

	this->m_Action=0;
	this->m_StatAdd.Empty();
	this->m_StatDel.Empty();
	m_ParentNo=0;
	m_stagingStatus = CTGitPath::StagingStatus::DontCare;
	ATLASSERT(IsEmpty());
}

CTGitPath CTGitPath::GetDirectory() const
{
	if ((IsDirectory())||(!Exists()))
		return *this;
	return GetContainingDirectory();
}

CTGitPath CTGitPath::GetContainingDirectory() const
{
	EnsureBackslashPathSet();

	CString sDirName = m_sBackslashPath.Left(m_sBackslashPath.ReverseFind('\\'));
	if(sDirName.GetLength() == 2 && sDirName[1] == ':')
	{
		// This is a root directory, which needs a trailing slash
		sDirName += L'\\';
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
	ATLVERIFY(::PathStripToRoot(CStrBuf(workingPath, MAX_PATH))); // MAX_PATH ok here.
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
		const int dotPos = m_sBackslashPath.ReverseFind('.');
		const int slashPos = m_sBackslashPath.ReverseFind('\\');
		if (dotPos > slashPos)
			return m_sBackslashPath.Mid(dotPos);
	}
	return CString();
}
CString CTGitPath::GetBaseFilename() const
{
	CString filename=GetFilename();
	const int dot = filename.ReverseFind(L'.');
	if(dot>0)
		filename.Truncate(dot);
	return filename;
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
		return CPathUtils::ArePathStringsEqualWithCase(m_sBackslashPath, rhs.m_sBackslashPath);
	}
	else
	{
		// Assume we've got a fwdslash path and make sure that the RHS has one
		rhs.EnsureFwdslashPathSet();
		return CPathUtils::ArePathStringsEqualWithCase(m_sFwdslashPath, rhs.m_sFwdslashPath);
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
		return CPathUtils::ArePathStringsEqual(m_sBackslashPath, rhs.m_sBackslashPath);
	}
	else
	{
		// Assume we've got a fwdslash path and make sure that the RHS has one
		rhs.EnsureFwdslashPathSet();
		return CPathUtils::ArePathStringsEqual(m_sFwdslashPath, rhs.m_sFwdslashPath);
	}
}

bool CTGitPath::IsAncestorOf(const CTGitPath& possibleDescendant) const
{
	possibleDescendant.EnsureBackslashPathSet();
	EnsureBackslashPathSet();

	if (m_sBackslashPath.IsEmpty() && PathIsRelative(possibleDescendant.m_sBackslashPath))
		return true;

	bool bPathStringsEqual = CPathUtils::ArePathStringsEqual(m_sBackslashPath, possibleDescendant.m_sBackslashPath.Left(m_sBackslashPath.GetLength()));
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
CString CTGitPath::GetDisplayString(const CTGitPath* pOptionalBasePath /* = nullptr*/) const
{
	EnsureFwdslashPathSet();
	if (pOptionalBasePath)
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
	return CStringUtils::FastCompareNoCase(left.m_sBackslashPath, right.m_sBackslashPath);
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
	cleanAppend.Replace(L'/', L'\\');
	cleanAppend.TrimLeft(L'\\');
	m_sBackslashPath.TrimRight(L'\\');
	CString strCopy = m_sBackslashPath;
	strCopy += L'\\';
	strCopy += cleanAppend;
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
		return m_bIsWCRoot;

	if (IsEquivalentToWithoutCase(topDirectory))
		m_bIsWCRoot = true;

	return m_bIsWCRoot;
}

bool CTGitPath::HasSubmodules() const
{
	if (HasAdminDir())
	{
		CString path = m_sProjectRoot;
		path += L"\\.gitmodules";
		if( PathFileExists(path) )
			return true;
	}
	return false;
}

int CTGitPath::GetAdminDirMask() const
{
	int status = 0;
	if (!HasAdminDir())
		return status;

	// ITEMIS_INGIT will be revoked if necessary in TortoiseShell/ContextMenu.cpp
	status |= ITEMIS_INGIT|ITEMIS_INVERSIONEDFOLDER;

	if (IsDirectory())
	{
		status |= ITEMIS_FOLDERINGIT;
		if (IsWCRoot())
		{
			status |= ITEMIS_WCROOT;

			if (IsRegisteredSubmoduleOfParentProject())
				status |= ITEMIS_SUBMODULE;
		}
	}

	CString dotGitPath;
	bool isWorktree;
	GitAdminDir::GetAdminDirPath(m_sProjectRoot, dotGitPath, &isWorktree);
	if (HasStashDir(dotGitPath))
		status |= ITEMIS_STASH;

	if (PathFileExists(dotGitPath + L"svn\\.metadata"))
		status |= ITEMIS_GITSVN;

	if (isWorktree)
	{
		dotGitPath.Empty();
		GitAdminDir::GetWorktreeAdminDirPath(m_sProjectRoot, dotGitPath);
	}

	if (PathFileExists(dotGitPath + L"BISECT_START"))
		status |= ITEMIS_BISECT;

	if (PathFileExists(dotGitPath + L"MERGE_HEAD"))
		status |= ITEMIS_MERGEACTIVE;

	if (PathFileExists(m_sProjectRoot + L"\\.gitmodules"))
		status |= ITEMIS_SUBMODULECONTAINER;

	return status;
}

bool CTGitPath::IsRegisteredSubmoduleOfParentProject(CString* parentProjectRoot /* nullptr */) const
{
	CString topProjectDir;
	if (!GitAdminDir::HasAdminDir(GetWinPathString(), false, &topProjectDir))
		return false;

	if (parentProjectRoot)
		*parentProjectRoot = topProjectDir;

	if (!PathFileExists(topProjectDir + L"\\.gitmodules"))
		return false;

	CAutoConfig config(true);
	git_config_add_file_ondisk(config, CGit::GetGitPathStringA(topProjectDir + L"\\.gitmodules"), GIT_CONFIG_LEVEL_APP, nullptr, FALSE);
	CString relativePath = GetWinPathString().Mid(topProjectDir.GetLength());
	relativePath.Replace(L'\\', L'/');
	relativePath.Trim(L'/');
	CStringA submodulePath = CUnicodeUtils::GetUTF8(relativePath);
	if (git_config_foreach_match(config, "submodule\\..*\\.path", [](const git_config_entry* entry, void* data) { return static_cast<CStringA*>(data)->Compare(entry->value) == 0 ? GIT_EUSER : 0; }, &submodulePath) == GIT_EUSER)
		return true;
	return false;
}

bool CTGitPath::HasStashDir(const CString& dotGitPath) const
{
	if (PathFileExists(dotGitPath + L"refs\\stash"))
		return true;

	CAutoFile hfile = CreateFile(dotGitPath + L"packed-refs", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_DELETE | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (!hfile)
		return false;

	LARGE_INTEGER fileSize;
	if (!::GetFileSizeEx(hfile, &fileSize) || fileSize.QuadPart == 0 || fileSize.QuadPart >= INT_MAX)
		return false;

	auto buff = std::unique_ptr<char[]>(new (std::nothrow) char[fileSize.LowPart + 1]); // prevent default initialization and throwing on allocation error
	if (!buff)
		return false;
	DWORD size = 0;
	if (!ReadFile(hfile, buff.get(), fileSize.LowPart, &size, nullptr))
		return false;
	buff[fileSize.LowPart] = '\0';

	if (size != fileSize.LowPart)
		return false;

	for (DWORD i = 0; i < fileSize.LowPart;)
	{
		if (buff[i] == '#' || buff[i] == '^')
		{
			while (buff[i] != '\n')
			{
				++i;
				if (i == fileSize.LowPart)
					break;
			}
			++i;
		}

		if (i >= fileSize.LowPart)
			break;

		while (buff[i] != ' ')
		{
			++i;
			if (i == fileSize.LowPart)
				break;
		}

		++i;
		if (i >= fileSize.LowPart)
			break;

		if (i <= fileSize.LowPart - strlen("refs/stash") && (buff[i + strlen("refs/stash")] == '\n' || buff[i + strlen("refs/stash")] == '\0') && !strncmp("refs/stash", buff.get() + i, strlen("refs/stash")))
			return true;
		while (buff[i] != '\n')
		{
			++i;
			if (i == fileSize.LowPart)
				break;
		}

		while (buff[i] == '\n')
		{
			++i;
			if (i == fileSize.LowPart)
				break;
		}
	}
	return false;
}

bool CTGitPath::HasStashDir() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(m_sProjectRoot, dotGitPath);

	return HasStashDir(dotGitPath);
}

bool CTGitPath::HasGitSVNDir() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(m_sProjectRoot, dotGitPath);

	return PathFileExists(dotGitPath + L"svn\\.metadata") == TRUE;
}
bool CTGitPath::IsBisectActive() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetWorktreeAdminDirPath(m_sProjectRoot, dotGitPath);

	return !!PathFileExists(dotGitPath + L"BISECT_START");
}
bool CTGitPath::IsRebaseActive() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetWorktreeAdminDirPath(m_sProjectRoot, dotGitPath);

	return PathIsDirectory(dotGitPath + L"rebase-apply") || PathIsDirectory(dotGitPath + L"tgitrebase.active");
}
bool CTGitPath::IsCherryPickActive() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetWorktreeAdminDirPath(m_sProjectRoot, dotGitPath);

	return !!PathFileExists(dotGitPath + L"CHERRY_PICK_HEAD");
}
bool CTGitPath::IsMergeActive() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetWorktreeAdminDirPath(m_sProjectRoot, dotGitPath);

	return !!PathFileExists(dotGitPath + L"MERGE_HEAD");
}
bool CTGitPath::HasRebaseApply() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetWorktreeAdminDirPath(m_sProjectRoot, dotGitPath);

	return !!PathFileExists(dotGitPath + L"rebase-apply");
}
bool CTGitPath::HasLFS() const
{
	if (!HasAdminDir())
		return false;

	CString dotGitPath;
	GitAdminDir::GetAdminDirPath(m_sProjectRoot, dotGitPath);

	return PathFileExists(dotGitPath + L"lfs") == TRUE;
}

bool CTGitPath::HasAdminDir(CString* ProjectTopDir /* = nullptr */, bool force /* = false */) const
{
	if (m_bHasAdminDirKnown && !force)
	{
		if (ProjectTopDir)
			*ProjectTopDir = m_sProjectRoot;
		return m_bHasAdminDir;
	}

	EnsureBackslashPathSet();
	bool isAdminDir = false;
	m_bHasAdminDir = GitAdminDir::HasAdminDir(m_sBackslashPath, IsDirectory(), &m_sProjectRoot, &isAdminDir);
	m_bHasAdminDirKnown = true;
	if ((m_bHasAdminDir || isAdminDir) && !m_bIsAdminDirKnown)
	{
		m_bIsAdminDir = isAdminDir;
		m_bIsAdminDirKnown = true;
	}
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
	if (m_bIsAdminDir && !m_bHasAdminDirKnown)
	{
		m_bHasAdminDir = false;
		m_bHasAdminDirKnown = true;
	}
	return m_bIsAdminDir;
}

bool CTGitPath::IsValidOnWindows() const
{
	if (m_bIsValidOnWindowsKnown)
		return m_bIsValidOnWindows;

	m_bIsValidOnWindows = false;
	EnsureBackslashPathSet();
	CString sMatch = m_sBackslashPath + L"\r\n";
	std::wstring sPattern;
	// the 'file://' URL is just a normal windows path:
	if (CStringUtils::StartsWithI(sMatch, L"file:\\\\"))
	{
		sMatch = sMatch.Mid(static_cast<int>(wcslen(L"file:\\\\")));
		sMatch.TrimLeft(L'\\');
		sPattern = L"^(\\\\\\\\\\?\\\\)?(([a-zA-Z]:|\\\\)\\\\)?(((\\.)|(\\.\\.)|([^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?))\\\\)*[^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?$";
	}
	else
		sPattern = L"^(\\\\\\\\\\?\\\\)?(([a-zA-Z]:|\\\\)\\\\)?(((\\.)|(\\.\\.)|([^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?))\\\\)*[^\\\\/:\\*\\?\"\\|<> ](([^\\\\/:\\*\\?\"\\|<>\\. ])|([^\\\\/:\\*\\?\"\\|<>]*[^\\\\/:\\*\\?\"\\|<>\\. ]))?$";

	try
	{
		std::wregex rx(sPattern, std::regex_constants::icase | std::regex_constants::ECMAScript);
		std::wsmatch match;

		std::wstring rmatch = std::wstring(static_cast<LPCWSTR>(sMatch));
		if (std::regex_match(rmatch, match, rx))
		{
			if (std::wstring(match[0]).compare(sMatch)==0)
				m_bIsValidOnWindows = true;
		}
		if (m_bIsValidOnWindows)
		{
			// now check for illegal filenames
			std::wregex rx2(L"\\\\(lpt\\d|com\\d|aux|nul|prn|con)(\\\\|$)", std::regex_constants::icase | std::regex_constants::ECMAScript);
			rmatch = m_sBackslashPath;
			if (std::regex_search(rmatch, rx2, std::regex_constants::match_default))
				m_bIsValidOnWindows = false;
		}
	}
	catch (std::exception&) {}

	m_bIsValidOnWindowsKnown = true;
	return m_bIsValidOnWindows;
}

//////////////////////////////////////////////////////////////////////////

CTGitPathList::CTGitPathList()
{
}

// A constructor which allows a path list to be easily built which one initial entry in
CTGitPathList::CTGitPathList(const CTGitPath& firstEntry)
{
	AddPath(firstEntry);
}

int CTGitPathList::ParserFromLsFileSimple(const BYTE_VECTOR& out, unsigned int action, bool clear /*= true*/)
{
	size_t pos = 0;
	const size_t end = out.size();
	CTGitPath path;
	CString pathstring;
	if (clear)
		Clear();
	while (pos < end)
	{
		const size_t endOfLine = out.find('\0', pos);
		if (endOfLine == CGitByteArray::npos || endOfLine == pos || endOfLine - pos >= INT_MAX)
			return -1;

		pathstring.Empty();
		CGit::StringAppend(pathstring, &out[pos], CP_UTF8, static_cast<int>(endOfLine - pos));
		// SetFromGit resets the path
		if (CStringUtils::EndsWith(pathstring, L'/'))
		{
			pathstring.Truncate(pathstring.GetLength() - 1);
			path.SetFromGit(pathstring, true);
		}
		else
			path.SetFromGit(pathstring);

		path.m_Action = action;
		AddPath(path);

		pos = out.findNextString(endOfLine);
	}
	return 0;
}

// similar code in CGit::ParseConflictHashesFromLsFile
int CTGitPathList::ParserFromLsFile(const BYTE_VECTOR& out, const GIT_HASH_TYPE hashType)
{
	const auto hashLength = 2 * CGitHash::HashLength(hashType);

	size_t pos = 0;
	const size_t end = out.size();
	CTGitPath path;
	CString pathstring;
	this->Clear();
	while (pos < end)
	{
		const size_t lineStart = pos;

		// m_Action is never used and propably never worked (needs to be set after path.SetFromGit)
		// also dropped LOGACTIONS_CACHE for 'H'
		// path.m_Action=path.ParserAction(out[pos]);
		pos = out.find(' ', pos); // advance to mode
		if (pos == CGitByteArray::npos)
			return -1;

		const size_t modestart = pos + 1;

		pos = out.find(' ', pos + 1); // advance to hash
		if (pos == CGitByteArray::npos)
			return -1;

		pos = out.find(' ', pos + 1); // advance to Stage
		if (pos == CGitByteArray::npos)
			return -1;

		const size_t stagestart = pos + 1;

		pos = out.find('\t', pos + 1); // advance to filename
		if (pos == CGitByteArray::npos)
			return -1;

		++pos;
		const size_t fileNameEnd = out.find(0, pos);
		if (fileNameEnd == CGitByteArray::npos || fileNameEnd == pos || pos - lineStart != strlen("H 100644 ") + hashLength + strlen(" 0\t")) // <tag> <mode> <object> <stage>\t<file>
			return -1;
		pathstring.Empty();
		CGit::StringAppend(pathstring, &out[pos], CP_UTF8, static_cast<int>(fileNameEnd - pos));
		// SetFromGit resets the path
		path.SetFromGit(pathstring, (strtol(&out[modestart], nullptr, 8) & S_IFDIR) == S_IFDIR);
		if (strtol(&out[stagestart], nullptr, 10) != 0)
		{
			if (!IsEmpty() && path == m_paths[m_paths.size() - 1])
			{
				pos = out.findNextString(pos);
				continue;
			}
			path.m_Action = CTGitPath::LOGACTIONS_UNMERGED;
		}

		this->AddPath(path);

		pos=out.findNextString(pos);
	}
	return 0;
}

void CTGitPathList::UpdateStagingStatusFromPath(const CString& path, CTGitPath::StagingStatus status)
{
	for (int i = 0; i < this->GetCount(); ++i)
	{
		if (CPathUtils::ArePathStringsEqualWithCase((*this)[i].GetGitPathString(), path))
		{
			m_paths[i].m_stagingStatus = status;
			break;
		}
	}
}

int CTGitPathList::FillUnRev(unsigned int action, const CTGitPathList* list, CString* err)
{
	this->Clear();
	CTGitPath path;

	const int count = [list]() {
		if (!list)
			return 1;
		else
			return list->GetCount();
	}();
	for (int i = 0; i < count; ++i)
	{
		CString cmd;
		CString ignored;
		if(action & CTGitPath::LOGACTIONS_IGNORE)
			ignored = L" -i";

		if (!list)
		{
			cmd = L"git.exe ls-files --exclude-standard --full-name --others -z";
			cmd+=ignored;

		}
		else
		{
			ATLASSERT(!(*list)[i].GetWinPathString().IsEmpty());
			cmd.Format(L"git.exe ls-files --exclude-standard --full-name --others -z%s -- \"%s\"",
					static_cast<LPCWSTR>(ignored),
					(*list)[i].GetWinPath());
		}

		BYTE_VECTOR out, errb;
		if (g_Git.Run(cmd, &out, &errb))
		{
			if (err)
				*err = errb;
			return -1;
		}

		if (ParserFromLsFileSimple(out, action, false) < 0)
			return -1;
	}
	return 0;
}

#ifdef TGIT_LFS
int CTGitPathList::FillLFSLocks(unsigned int action, CString* err)
{
	Clear();

	CString output;
	CString errCmd;
	if (g_Git.Run(L"git.exe lfs locks --json", &output, &errCmd, CP_UTF8) != 0)
	{
		if (err)
			err->Append(errCmd);
		return -1;
	}

	return ParserFromLFSLocks(action, output, err);
}

int CTGitPathList::ParserFromLFSLocks(unsigned int action, const CString& output, CString* err)
{
	Clear();

	if (output.IsEmpty())
		return 0;

	try
	{
		auto result = json::parse(CUnicodeUtils::GetUTF8(output).GetString());
		for (auto& r : result)
		{
			if (r["id"].get<std::string>().empty())
				continue;
			CTGitPath gitPath;
			gitPath.SetFromGit(CUnicodeUtils::GetUnicode(r["path"].get<std::string>()));
			gitPath.m_Action = action;
			gitPath.m_LFSLockOwner = CUnicodeUtils::GetUnicode(r["owner"]["name"].get<std::string>());
			AddPath(gitPath);
		}
	}
	catch (json::parse_error& ex)
	{
		if (err)
			err->Append(CUnicodeUtils::GetUnicode(ex.what()));
		return -1;
	}
	return 0;
}
#endif

int CTGitPathList::FillBasedOnIndexFlags(unsigned short flag, unsigned short flagextended, const CTGitPathList* list /*nullptr*/)
{
	Clear();
	CTGitPath path;

	CAutoRepository repository(g_Git.GetGitRepository());
	if (!repository)
		return -1;

	CAutoIndex index;
	if (git_repository_index(index.GetPointer(), repository))
		return -1;

	const int count = [list]() {
		if (!list)
			return 1;
		else
			return list->GetCount();
	}();
	for (int j = 0; j < count; ++j)
	{
		for (size_t i = 0, ecount = git_index_entrycount(index); i < ecount; ++i)
		{
			const git_index_entry *e = git_index_get_byindex(index, i);

			if (!e || !((e->flags & flag) || (e->flags_extended & flagextended)) || !e->path)
				continue;

			CString one = CUnicodeUtils::GetUnicode(e->path);

			if (!(!list || (*list)[j].GetWinPathString().IsEmpty() || one == (*list)[j].GetGitPathString() || (PathIsDirectory(g_Git.CombinePath((*list)[j].GetWinPathString())) && CStringUtils::StartsWith(one, (*list)[j].GetGitPathString() + L'/'))))
				continue;

			//SetFromGit will clear all status
			path.SetFromGit(one, (e->mode & S_IFDIR) == S_IFDIR);
			if (e->flags_extended & GIT_INDEX_ENTRY_SKIP_WORKTREE)
				path.m_Action = CTGitPath::LOGACTIONS_SKIPWORKTREE;
			else if (e->flags & GIT_INDEX_ENTRY_VALID)
				path.m_Action = CTGitPath::LOGACTIONS_ASSUMEVALID;
			AddPath(path);
		}
	}
	RemoveDuplicates();
	return 0;
}
int CTGitPathList::ParserFromLog(const BYTE_VECTOR& log)
{
	static bool mergeReplacedStatus = CRegDWORD(L"Software\\TortoiseGit\\MergeReplacedStatusKS", TRUE, false, HKEY_LOCAL_MACHINE) == TRUE; // TODO: remove kill-switch
	this->Clear();
	std::map<CString, size_t> duplicateMap;
	size_t pos = 0;
	CTGitPath path;
	m_Action=0;
	CString StatAdd;
	CString StatDel;
	CString pathname1;
	CString pathname2;

	const size_t logend = log.size();
	while (pos < logend)
	{
		if(log[pos]==':')
		{
			bool merged=false;
			if (pos + 1 >= logend)
				return -1;
			if (log[pos + 1] == ':')
			{
				merged = true;
				++pos;
			}

			const size_t statusEnd = log.find('\0', pos);
			/*
			 * There are at least two modes (each 6 characters) and two hashes (variable length [4, 40], cf. https://github.com/git/git/blob/master/environment.c#L18)
			 * and the status (a char + optional score), each separated by space
			 */
			if (statusEnd == BYTE_VECTOR::npos || statusEnd - pos < ((6 + 1) + (6 + 1) + (4 + 1) + (4 + 1) + 1))
				return -1;

			const int modeOld = strtol(&log[pos + 1], nullptr, 8);
			const int modeNew = strtol(&log[pos + 7], nullptr, 8);
			// find start of status character
			size_t statusStart = log.find(' ', statusEnd - 6); // status: "A", "D", "U" or "C100" etc., 6 is chosen to find its start without interferring with the dst hash, see comment above
			if (statusStart == BYTE_VECTOR::npos)
				return -1;

			++statusStart;
			pos = log.find('\0', statusStart); // advance to filename
			if (pos == BYTE_VECTOR::npos || statusStart == pos)
				return -1;
			++pos;

			pathname2.Empty();
			if (log[statusStart] == 'C' || log[statusStart] == 'R')
			{
				const size_t filenameEnd = log.find('\0', pos);
				if (filenameEnd == BYTE_VECTOR::npos || pos == filenameEnd || filenameEnd - pos >= INT_MAX)
					return -1;
				// old filename before rename
				CGit::StringAppend(pathname2, &log[pos], CP_UTF8, static_cast<int>(filenameEnd - pos));
				pos = filenameEnd + 1;
			}
			const size_t filenameEnd = log.find('\0', pos);
			if (filenameEnd == BYTE_VECTOR::npos || pos == filenameEnd || filenameEnd - pos >= INT_MAX)
				return -1;
			pathname1.Empty();
			CGit::StringAppend(pathname1, &log[pos], CP_UTF8, static_cast<int>(filenameEnd - pos));
			pos = filenameEnd + 1;

			if (const auto existing = duplicateMap.find(pathname1); existing != duplicateMap.end())
			{
				CTGitPath& p = m_paths[existing->second];
				if (!(mergeReplacedStatus && p.m_Action == CTGitPath::LOGACTIONS_REPLACED && (log[statusStart] == 'A' || log[statusStart] == 'D')))
					p.ParseAndUpdateStatus(log[statusStart]);

				// reset submodule/folder status if a staged entry is not a folder
				if (p.IsDirectory() && ((modeOld && !(modeOld & S_IFDIR)) || (modeNew && !(modeNew & S_IFDIR))))
					p.UnsetDirectoryStatus();
				else if (!p.IsDirectory() && (modeNew && (modeNew & S_IFDIR)))
					p.SetDirectoryStatus();

				if(merged)
					p.m_Action |= CTGitPath::LOGACTIONS_MERGED;
				m_Action |= p.m_Action;
			}
			else
			{
				unsigned int ac = CTGitPath::ParseStatus(log[statusStart]);
				ac |= merged?CTGitPath::LOGACTIONS_MERGED:0;

				int isSubmodule = FALSE;
				if (ac & (CTGitPath::LOGACTIONS_DELETED | CTGitPath::LOGACTIONS_UNMERGED))
					isSubmodule = (modeOld & S_IFDIR) == S_IFDIR;
				else
					isSubmodule = (modeNew & S_IFDIR) == S_IFDIR;

				// SetFromGit resets the path, hence action must be set afterwards
				path.SetFromGit(pathname1, &pathname2, &isSubmodule);
				path.m_Action=ac;
				this->m_Action|=ac;

				AddPath(path);
				duplicateMap.insert(std::pair<CString, size_t>(path.GetGitPathString(), m_paths.size() - 1));
				if (mergeReplacedStatus && !pathname2.IsEmpty())
					duplicateMap.insert(std::pair<CString, size_t>(path.GetGitOldPathString(), m_paths.size() - 1));
			}
		}
		else // numstat output
		{
			size_t tabstart = log.find('\t', pos); // find end of first number (added lines)
			if (tabstart == BYTE_VECTOR::npos || tabstart - pos >= INT_MAX)
				return -1;

			StatAdd.Empty();
			CGit::StringAppend(StatAdd, &log[pos], CP_UTF8, static_cast<int>(tabstart - pos));
			pos = tabstart + 1;

			tabstart = log.find('\t', pos); // find end of second number (removed lines)
			if (tabstart == BYTE_VECTOR::npos || tabstart - pos >= INT_MAX)
				return -1;

			StatDel.Empty();
			CGit::StringAppend(StatDel, &log[pos], CP_UTF8, static_cast<int>(tabstart - pos));
			pos = tabstart + 1;

			if (pos >= logend)
				return -1;

			pathname2.Empty();
			if (log[pos] == '\0') // rename which holds an "old" pathname
			{
				++pos;
				const size_t endPathname = log.find('\0', pos);
				if (endPathname == BYTE_VECTOR::npos || pos == endPathname || endPathname - pos >= INT_MAX)
					return -1;
				CGit::StringAppend(pathname2, &log[pos], CP_UTF8, static_cast<int>(endPathname - pos));
				pos = endPathname + 1;
			}
			const size_t endPathname = log.find('\0', pos);
			if (endPathname == BYTE_VECTOR::npos || pos == endPathname || endPathname - pos >= INT_MAX)
				return -1;
			pathname1.Empty();
			CGit::StringAppend(pathname1, &log[pos], CP_UTF8, static_cast<int>(endPathname - pos));
			pos = endPathname + 1;

			// SetFromGit resets the path
			int isSubmodule = FALSE;
			path.SetFromGit(pathname1, &pathname2, &isSubmodule);

			auto existing = duplicateMap.find(path.GetGitPathString());
			if (existing != duplicateMap.end())
			{
				CTGitPath& p = m_paths[existing->second];
				p.m_StatAdd = StatAdd;
				p.m_StatDel = StatDel;
			}
			else
			{
				path.m_StatAdd = StatAdd;
				path.m_StatDel = StatDel;
				AddPath(path);
				duplicateMap.insert(std::pair<CString, size_t>(path.GetGitPathString(), m_paths.size() - 1));
			}
		}
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
	return static_cast<int>(m_paths.size());
}
bool CTGitPathList::IsEmpty() const
{
	return m_paths.empty();
}
void CTGitPathList::Clear()
{
	m_Action = 0;
	m_paths.clear();
	m_commonBaseDirectory.Reset();
}

const CTGitPath& CTGitPathList::operator[](INT_PTR index) const
{
	ATLASSERT(index >= 0 && index < static_cast<INT_PTR>(m_paths.size()));
	return m_paths[index];
}

bool CTGitPathList::AreAllPathsFiles() const
{
	// Look through the vector for any directories - if we find them, return false
	return std::none_of(m_paths.cbegin(), m_paths.cend(), std::mem_fn(&CTGitPath::IsDirectory));
}

bool CTGitPathList::AreAllPathsDirectories() const
{
	// Look through the vector for directories - if we find none, return false
	return std::all_of(m_paths.cbegin(), m_paths.cend(), std::mem_fn(&CTGitPath::IsDirectory));
}

bool CTGitPathList::IsAnyAncestorOf(const CTGitPath& possibleDescendant) const
{
	return std::any_of(m_paths.cbegin(), m_paths.cend(), [&possibleDescendant](auto& path) { return path.IsAncestorOf(possibleDescendant); });
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
			if (strLine.IsEmpty())
				continue;
			path.SetFromUnknown(strLine);
			AddPath(path);
		}
		file.Close();
	}
	catch (CFileException* pE)
	{
		CTraceToOutputDebugString::Instance()(__FUNCTION__ ": CFileException loading target file list\n");
		//CString error;
		//pE->GetErrorMessage(CStrBuf(error, 8192), 8192);
		//MessageBox(nullptr, error, L"TortoiseGit", MB_ICONERROR);
		pE->Delete();
		return false;
	}
	return true;
}

bool CTGitPathList::WriteToFile(const CString& sFilename, bool bUTF8 /* = false */) const
{
	try
	{
		if (bUTF8)
		{
			CStdioFile file(sFilename, CFile::typeText | CFile::modeReadWrite | CFile::modeCreate);
			for (const auto& path : m_paths)
			{
				CStringA line = CStringA(path.GetGitPathString()) + '\n';
				file.Write(line, line.GetLength());
			}
			file.Close();
		}
		else
		{
			CStdioFile file(sFilename, CFile::typeBinary | CFile::modeReadWrite | CFile::modeCreate);
			for (const auto& path : m_paths)
				file.WriteString(path.GetGitPathString() + L'\n');
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
		temp = sPathString.Tokenize(L"*", pos);
		if(temp.IsEmpty())
			break;
		AddPath(CTGitPath(CPathUtils::GetLongPathname(temp)));
	}
}

CString CTGitPathList::CreateAsteriskSeparatedString() const
{
	CString sRet;
	for (const auto& path : m_paths)
	{
		if (!sRet.IsEmpty())
			sRet += L'*';
		sRet += path.GetWinPathString();
	}
	return sRet;
}
#endif // _MFC_VER

bool CTGitPathList::WriteToPathSpecFile(const CString& sFilename) const
{
	CAutoFile hFile = ::CreateFile(sFilename, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
	if (!hFile)
		return false;

	constexpr char nullBuf[] = { '\0' };
	DWORD dwWritten = 0;
	for (const auto& path : m_paths)
	{
		CStringA line = CUnicodeUtils::GetUTF8(path.GetGitPathString());
		if (!WriteFile(hFile, line.GetString(), static_cast<DWORD>(line.GetLength()), &dwWritten, nullptr))
			return false;
		if (!WriteFile(hFile, nullBuf, static_cast<DWORD>(sizeof(nullBuf)), &dwWritten, nullptr))
			return false;
	}
	return true;
}

bool
CTGitPathList::AreAllPathsFilesInOneDirectory() const
{
	// Check if all the paths are files and in the same directory
	m_commonBaseDirectory.Reset();
	for (const auto& path : m_paths)
	{
		if (path.IsDirectory())
			return false;
		const CTGitPath& baseDirectory = path.GetDirectory();
		if(m_commonBaseDirectory.IsEmpty())
			m_commonBaseDirectory = baseDirectory;
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
		for (const auto& path : m_paths)
		{
			const CTGitPath& baseDirectory = path.GetDirectory();
			if(m_commonBaseDirectory.IsEmpty())
				m_commonBaseDirectory = baseDirectory;
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
	if (std::any_of(m_paths.cbegin(), m_paths.cend(), [&m_commonBaseDirectory = m_commonBaseDirectory](auto& path) { return !m_commonBaseDirectory.IsAncestorOf(path); }))
		m_commonBaseDirectory = m_commonBaseDirectory.GetContainingDirectory();
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
	CString root = m_paths[0].GetWinPathString() + L'\\';
	int rootLength = root.GetLength();

	// determine common path string prefix
	for (auto it = m_paths.cbegin() + 1; it != m_paths.cend(); ++it)
	{
		CString path = it->GetWinPathString() + L'\\';

		int newLength = CStringUtils::GetMatchingLength(root, path);
		if (newLength != rootLength)
		{
			root.Delete(newLength, rootLength);
			rootLength = newLength;
		}
	}

	// remove the last (partial) path element
	if (rootLength > 0)
		root.Delete(root.ReverseFind(L'\\'), rootLength);

	// done
	return CTGitPath(root);
}

void CTGitPathList::SortByPathname(bool bReverse /*= false*/)
{
	std::sort(m_paths.begin(), m_paths.end());
	if (bReverse)
		std::reverse(m_paths.begin(), m_paths.end());
}

void CTGitPathList::DeleteAllFiles(bool bTrash, bool bFilesOnly, bool bShowErrorUI)
{
	if (m_paths.empty())
		return;
	PathVector::const_iterator it;
	SortByPathname(true); // nested ones first

	CString sPaths;
	for (it = m_paths.cbegin(); it != m_paths.cend(); ++it)
	{
		if ((it->Exists()) && ((it->IsDirectory() != bFilesOnly) || !bFilesOnly))
		{
			if (!it->IsDirectory())
				::SetFileAttributes(it->GetWinPath(), FILE_ATTRIBUTE_NORMAL);

			sPaths += it->GetWinPath();
			sPaths += L'\0';
		}
	}
	if (sPaths.IsEmpty())
		return;
	sPaths += L'\0';
	sPaths += L'\0';
	DeleteViaShell(static_cast<LPCWSTR>(sPaths), bTrash, bShowErrorUI);
	Clear();
}

bool CTGitPathList::DeleteViaShell(LPCWSTR path, bool bTrash, bool bShowErrorUI)
{
	SHFILEOPSTRUCT shop = {0};
	shop.wFunc = FO_DELETE;
	shop.pFrom = path;
	shop.fFlags = FOF_NOCONFIRMATION|FOF_NO_CONNECTED_ELEMENTS;
	if (!bShowErrorUI)
		shop.fFlags |= FOF_NOERRORUI | FOF_SILENT;
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

void CTGitPathList::RemoveItem(const CTGitPath& path)
{
	PathVector::iterator it;
	for(it = m_paths.begin(); it != m_paths.end(); ++it)
	{
		if (CPathUtils::ArePathStringsEqualWithCase(it->GetGitPathString(), path.GetGitPathString()))
		{
			m_paths.erase(it);
			return;
		}
	}
}
void CTGitPathList::RemoveChildren()
{
	// Sort paths using a custom comparator that sorts directories before files and parent directories before their children
	std::sort(m_paths.begin(), m_paths.end(), [](const CTGitPath& left, const CTGitPath& right) {
		CString leftPath = left.GetWinPathString();
		CString rightPath = right.GetWinPathString();
		leftPath.Replace(L"\\", L"\1");
		rightPath.Replace(L"\\", L"\1");
		return leftPath.CompareNoCase(rightPath) < 0;
	});
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

const CTGitPath* CTGitPathList::LookForGitPath(const CString& path) const
{
	for (int i = 0; i < this->GetCount(); ++i)
	{
		if (CPathUtils::ArePathStringsEqualWithCase((*this)[i].GetGitPathString(), path))
			return &(*this)[i];
	}
	return nullptr;
}

CString CTGitPath::GetActionName(unsigned int action)
{
	if(action  & CTGitPath::LOGACTIONS_UNMERGED)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_CONFLICT);
	if(action  & CTGitPath::LOGACTIONS_ADDED)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_ADD);
	if (action & CTGitPath::LOGACTIONS_MISSING)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_MISSING);
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

	if (action & CTGitPath::LOGACTIONS_IGNORE)
		return MAKEINTRESOURCE(IDS_PATHACTIONS_IGNORED);

	return MAKEINTRESOURCE(IDS_PATHACTIONS_UNKNOWN);
}

CString CTGitPath::GetActionName() const
{
	return GetActionName(m_Action);
}

unsigned int CTGitPathList::GetAction()
{
	return m_Action;
}

CString CTGitPath::GetAbbreviatedRename() const
{
	if (GetGitOldPathString().IsEmpty())
		return GetFileOrDirectoryName();

	// Find common prefix which ends with a slash
	int prefix_length = 0;
	for (int i = 0, maxLength = min(m_sOldFwdslashPath.GetLength(), m_sFwdslashPath.GetLength()); i < maxLength; ++i)
	{
		if (m_sOldFwdslashPath[i] != m_sFwdslashPath[i])
			break;
		if (m_sOldFwdslashPath[i] == L'/')
			prefix_length = i + 1;
	}

	LPCWSTR oldName = static_cast<LPCWSTR>(m_sOldFwdslashPath) + m_sOldFwdslashPath.GetLength();
	LPCWSTR newName = static_cast<LPCWSTR>(m_sFwdslashPath) + m_sFwdslashPath.GetLength();

	int suffix_length = 0;
	int prefix_adjust_for_slash = (prefix_length ? 1 : 0);
	while (static_cast<LPCWSTR>(m_sOldFwdslashPath) + prefix_length - prefix_adjust_for_slash <= oldName &&
		   static_cast<LPCWSTR>(m_sFwdslashPath) + prefix_length - prefix_adjust_for_slash <= newName &&
		   *oldName == *newName)
	{
		if (*oldName == L'/')
			suffix_length = m_sOldFwdslashPath.GetLength() - static_cast<int>(oldName - static_cast<LPCWSTR>(m_sOldFwdslashPath));
		--oldName;
		--newName;
	}

	/*
	* pfx{old_midlen => new_midlen}sfx
	* {pfx-old => pfx-new}sfx
	* pfx{sfx-old => sfx-new}
	* name-old => name-new
	*/
	int old_midlen = m_sOldFwdslashPath.GetLength() - prefix_length - suffix_length;
	int new_midlen = m_sFwdslashPath.GetLength() - prefix_length - suffix_length;
	if (old_midlen < 0)
		old_midlen = 0;
	if (new_midlen < 0)
		new_midlen = 0;

	CString ret;
	if (prefix_length + suffix_length)
	{
		ret = m_sOldFwdslashPath.Left(prefix_length);
		ret += L'{';
	}
	ret += m_sOldFwdslashPath.Mid(prefix_length, old_midlen);
	ret += L" => ";
	ret += m_sFwdslashPath.Mid(prefix_length, new_midlen);
	if (prefix_length + suffix_length)
	{
		ret += L'}';
		ret += m_sFwdslashPath.Mid(m_sFwdslashPath.GetLength() - suffix_length, suffix_length);
	}
	return ret;
}
