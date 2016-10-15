// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2018 - TortoiseGit

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
#pragma once
#include "..\..\ext\simpleini\SimpleIni.h"

class CVersioncheckParser
{
public:
	CVersioncheckParser();
	~CVersioncheckParser();

	CVersioncheckParser(const CVersioncheckParser&) = delete;
	CVersioncheckParser& operator=(const CVersioncheckParser&) = delete;

	bool Load(const CString& filename, CString& err);

	typedef struct
	{
		CString version;
		CString version_for_filename;
		unsigned int major = 0;
		unsigned int minor = 0;
		unsigned int micro = 0;
		unsigned int build = 0;
	} Version;
	Version GetTortoiseGitVersion();

	CString GetTortoiseGitInfoText();
	CString GetTortoiseGitInfoTextURL();
	CString GetTortoiseGitIssuesURL();
	CString GetTortoiseGitChangelogURL();
	CString GetTortoiseGitBaseURL();
	bool GetTortoiseGitIsHotfix();
	CString GetTortoiseGitMainfilename();

	typedef struct
	{
		CString m_PackName;
		CString m_LangName;
		DWORD m_LocaleID;
		CString m_LangCode;

		CString m_filename;
	} LanguagePack;
	typedef std::vector<LanguagePack> LANGPACK_VECTOR;
	LANGPACK_VECTOR GetTortoiseGitLanguagePacks();

private:
	CString		GetTortoiseGitLanguagepackFilenameTemplate();
	CString		GetStringValue(const CString& section, const CString& entry);

	CSimpleIni	m_versioncheckfile;

	Version		m_version;
};
