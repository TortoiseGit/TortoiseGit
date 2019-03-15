// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2019 - TortoiseGit

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
#include "VersioncheckParser.h"
#include "UnicodeUtils.h"

CVersioncheckParser::CVersioncheckParser()
	: m_versioncheckfile(true, true)
{
}

CVersioncheckParser::~CVersioncheckParser()
{
}

static const wchar_t *escapes = L"ntb\"\\";
static const wchar_t *escaped = L"\n\t\b\"\\";
static CString GetConfigValue(const wchar_t* ptr)
{
	if (!ptr)
		return L"";

	CString value;
	{
		CStrBuf working(value, static_cast<int>(min(wcslen(ptr), static_cast<size_t>(UINT16_MAX))));
		wchar_t* fixed = working;
		bool quoted = false;

		while (*ptr)
		{
			if (*ptr == L'"')
				quoted = !quoted;
			else if (*ptr != L'\\')
			{
				if (!quoted && (*ptr == L'#' || *ptr == L';'))
					break;
				*fixed++ = *ptr;
			}
			else
			{
				/* backslash, check the next char */
				++ptr;
				const wchar_t* esc = wcschr(escapes, *ptr);
				if (esc)
					*fixed++ = escaped[esc - escapes];
				else
					return L"";
			}
			++ptr;
		}
		*fixed = L'\0';
	}

	return value;
}

CString CVersioncheckParser::GetStringValue(const CString& section, const CString& entry)
{
	return GetConfigValue(m_versioncheckfile.GetValue(section, entry));
}

bool CVersioncheckParser::Load(const CString& filename, CString& err)
{
	m_versioncheckfile.LoadFile(filename);

	m_version.version = GetStringValue(L"tortoisegit", L"version");
	if (m_version.version.IsEmpty())
	{
		err = L"Invalid version check file.";
		return false;
	}
	unsigned __int64 version = 0;
	CString vertemp = m_version.version;
	m_version.major = _ttoi(vertemp);
	vertemp = vertemp.Mid(vertemp.Find(L'.') + 1);
	m_version.minor = _ttoi(vertemp);
	vertemp = vertemp.Mid(vertemp.Find(L'.') + 1);
	m_version.micro = _ttoi(vertemp);
	vertemp = vertemp.Mid(vertemp.Find(L'.') + 1);
	m_version.build = _ttoi(vertemp);
	version = m_version.major;
	version <<= 16;
	version += m_version.minor;
	version <<= 16;
	version += m_version.micro;
	version <<= 16;
	version += m_version.build;

	if (version == 0)
	{
		err = L"Invalid version check file.";
		return false;
	}

	// another versionstring for the filename can be provided
	// this is needed for preview releases
	m_version.version_for_filename = GetStringValue(L"tortoisegit", L"versionstring");
	if (m_version.version_for_filename.IsEmpty())
		m_version.version_for_filename = m_version.version;

	return true;
}

CVersioncheckParser::Version CVersioncheckParser::GetTortoiseGitVersion()
{
	return m_version;
}

CString CVersioncheckParser::GetTortoiseGitInfoText()
{
	return GetStringValue(L"tortoisegit", L"infotext");;
}

CString CVersioncheckParser::GetTortoiseGitInfoTextURL()
{
	return GetStringValue(L"tortoisegit", L"infotexturl");;
}

CString CVersioncheckParser::GetTortoiseGitIssuesURL()
{
	CString issueurl = GetStringValue(L"tortoisegit", L"issuesurl");
	auto section = m_versioncheckfile.GetSection(L"TortoiseGit");
	if (issueurl.IsEmpty() && section && section->find(L"issuesurl") == section->cend())
		issueurl = L"https://tortoisegit.org/issue/%BUGID%";
	return issueurl;
}

CString CVersioncheckParser::GetTortoiseGitChangelogURL()
{
	CString changelogurl = GetStringValue(L"tortoisegit", L"changelogurl");
	if (changelogurl.IsEmpty())
		changelogurl = L"https://versioncheck.tortoisegit.org/changelog.txt";
	return changelogurl;
}

CString CVersioncheckParser::GetTortoiseGitBaseURL()
{
	CString baseurl = GetStringValue(L"tortoisegit", L"baseurl");
	if (baseurl.IsEmpty())
		baseurl.Format(L"http://updater.download.tortoisegit.org/tgit/%s/", static_cast<LPCTSTR>(m_version.version_for_filename));
	return baseurl;
}

bool CVersioncheckParser::GetTortoiseGitIsHotfix()
{
	return m_versioncheckfile.GetBoolValue(L"tortoisegit", L"hotfix", false);
}

static inline CString x86x64()
{
#if WIN64
	return L"64";
#else
	return L"32";
#endif
}

CString CVersioncheckParser::GetTortoiseGitMainfilename()
{
	CString mainfilenametemplate = GetStringValue(L"tortoisegit", L"mainfilename");
	if (mainfilenametemplate.IsEmpty())
		mainfilenametemplate = L"TortoiseGit-%1!s!-%2!s!bit.msi";
	CString mainfilename;
	mainfilename.FormatMessage(mainfilenametemplate, static_cast<LPCTSTR>(m_version.version_for_filename), static_cast<LPCTSTR>(x86x64()));
	return mainfilename;
}

CString CVersioncheckParser::GetTortoiseGitLanguagepackFilenameTemplate()
{
	CString languagepackfilenametemplate = GetStringValue(L"tortoisegit", L"languagepackfilename");
	if (languagepackfilenametemplate.IsEmpty())
		languagepackfilenametemplate = L"TortoiseGit-LanguagePack-%1!s!-%2!s!bit-%3!s!.msi";
	return languagepackfilenametemplate;
}

CVersioncheckParser::LANGPACK_VECTOR CVersioncheckParser::GetTortoiseGitLanguagePacks()
{
	CString languagepackfilenametemplate = GetTortoiseGitLanguagepackFilenameTemplate();
	LANGPACK_VECTOR vec;

	CSimpleIni::TNamesDepend values;
	m_versioncheckfile.GetAllValues(L"tortoisegit", L"langs", values);
	for (const auto& value : values)
	{
		CString langs = GetConfigValue(value.pItem);
		LanguagePack pack;

		int nextTokenPos = langs.Find(L";", 5); // be extensible for the future
		if (nextTokenPos > 0)
			langs = langs.Left(nextTokenPos);
		pack.m_LangCode = langs.Mid(5);
		pack.m_PackName = L"TortoiseGit Language Pack " + pack.m_LangCode;

		pack.m_LocaleID = _tstoi(langs.Left(4));
		TCHAR buf[MAX_PATH] = { 0 };
		GetLocaleInfo(pack.m_LocaleID, LOCALE_SNATIVELANGNAME, buf, _countof(buf));
		pack.m_LangName = buf;
		GetLocaleInfo(pack.m_LocaleID, LOCALE_SNATIVECTRYNAME, buf, _countof(buf));
		if (buf[0])
		{
			pack.m_LangName += L" (";
			pack.m_LangName += buf;
			pack.m_LangName += L')';
		}

		pack.m_filename.FormatMessage(languagepackfilenametemplate, static_cast<LPCTSTR>(m_version.version_for_filename), static_cast<LPCTSTR>(x86x64()), static_cast<LPCTSTR>(pack.m_LangCode), pack.m_LocaleID);

		vec.push_back(pack);
	}
	return vec;
}
