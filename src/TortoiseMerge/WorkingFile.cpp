// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2017 - TortoiseGit
// Copyright (C) 2006-2007, 2011-2014 - TortoiseSVN

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
#include "WorkingFile.h"
#include "AppUtils.h"
#include "PathUtils.h"
#include "resource.h"
#include "SmartHandle.h"

CWorkingFile::CWorkingFile(void)
{
	ClearStoredAttributes();
}

CWorkingFile::~CWorkingFile(void)
{
}

void CWorkingFile::SetFileName(const CString& newFilename)
{
	m_sFilename = newFilename;
	m_sFilename.Replace('/', '\\');
	m_sDescriptiveName.Empty();
	ClearStoredAttributes();
}

void CWorkingFile::SetDescriptiveName(const CString& newDescName)
{
	m_sDescriptiveName = newDescName;
}

CString CWorkingFile::GetDescriptiveName()
{
	if (m_sDescriptiveName.IsEmpty())
	{
		CString sDescriptiveName = CPathUtils::GetFileNameFromPath(m_sFilename);
		WCHAR pathbuf[MAX_PATH] = {0};
		PathCompactPathEx(pathbuf, sDescriptiveName, 50, 0);
		sDescriptiveName = pathbuf;
		return sDescriptiveName;
	}
	return m_sDescriptiveName;
}

CString CWorkingFile::GetReflectedName()
{
	return m_sReflectedName;
}

void CWorkingFile::SetReflectedName(const CString& newReflectedName)
{
	m_sReflectedName = newReflectedName;
}

//
// Make an empty file with this name
void CWorkingFile::CreateEmptyFile()
{
	CAutoFile hFile = CreateFile(m_sFilename, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
}

// Move the details of the specified file to the current one, and then mark the specified file
// as out of use
void CWorkingFile::TransferDetailsFrom(CWorkingFile& rightHandFile)
{
	// We don't do this to files which are already in use
	ASSERT(!InUse());

	m_sFilename = rightHandFile.m_sFilename;
	m_sDescriptiveName = rightHandFile.m_sDescriptiveName;
	rightHandFile.SetOutOfUse();
	m_attribs = rightHandFile.m_attribs;
}

CString CWorkingFile::GetWindowName(UINT type) const
{
	CString sErrMsg;
	// TortoiseMerge allows non-existing files to be used in a merge
	// Inform the user (in a non-intrusive way) if a file is absent
	if (! this->Exists())
	{
		sErrMsg = CString(MAKEINTRESOURCE(IDS_NOTFOUNDVIEWTITLEINDICATOR));
	}

	if(m_sDescriptiveName.IsEmpty())
	{
		// We don't have a proper name - use the filename part of the path
		// return the filename part of the path.
		CString ret;
		if (type)
		{
			ret.LoadString(type);
			ret += L" - ";
		}
		ret += CPathUtils::GetFileNameFromPath(m_sFilename) + L' ' + sErrMsg;
		return ret;
	}
	else if (sErrMsg.IsEmpty())
	{
		return m_sDescriptiveName;
	}
	return m_sDescriptiveName + L' ' + sErrMsg;
}

bool CWorkingFile::Exists() const
{
	return (!!PathFileExists(m_sFilename));
}

void CWorkingFile::SetOutOfUse()
{
	m_sFilename.Empty();
	m_sDescriptiveName.Empty();
	ClearStoredAttributes();
}


bool CWorkingFile::HasSourceFileChanged() const
{
	if (!InUse())
	{
		return false;
	}
	WIN32_FILE_ATTRIBUTE_DATA attribs = {0};
	if (PathFileExists(m_sFilename))
	{
		if (GetFileAttributesEx(m_sFilename, GetFileExInfoStandard, &attribs))
		{
			if ( (m_attribs.nFileSizeHigh != attribs.nFileSizeHigh) ||
				(m_attribs.nFileSizeLow != attribs.nFileSizeLow) )
				return true;
			return ( (CompareFileTime(&m_attribs.ftCreationTime, &attribs.ftCreationTime)!=0) ||
				(CompareFileTime(&m_attribs.ftLastWriteTime, &attribs.ftLastWriteTime)!=0) );
		}
	}

	return false;
}

void CWorkingFile::StoreFileAttributes()
{
	ClearStoredAttributes();

	WIN32_FILE_ATTRIBUTE_DATA attribs = {0};
	if (GetFileAttributesEx(m_sFilename, GetFileExInfoStandard, &attribs))
	{
		m_attribs = attribs;
	}
}

void CWorkingFile::ClearStoredAttributes()
{
	static const WIN32_FILE_ATTRIBUTE_DATA attribsEmpty = {0};
	m_attribs = attribsEmpty;
}

bool CWorkingFile::IsReadonly() const
{
	return (m_attribs.dwFileAttributes != INVALID_FILE_ATTRIBUTES) && (m_attribs.dwFileAttributes & FILE_ATTRIBUTE_READONLY);
}
