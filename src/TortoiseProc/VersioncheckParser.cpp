// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2017 - TortoiseGit

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
#include "Git.h"

CVersioncheckParser::CVersioncheckParser()
	: m_versioncheckfile(true)
{
}

CVersioncheckParser::~CVersioncheckParser()
{
}

bool CVersioncheckParser::Load(const CString& filename, CString& err)
{
	git_config_add_file_ondisk(m_versioncheckfile, CUnicodeUtils::GetUTF8(filename), GIT_CONFIG_LEVEL_GLOBAL, nullptr, 0);

	if (m_versioncheckfile.GetString(L"tortoisegit.version", m_version.version))
	{
		err = L"Could not parse version check file: " + g_Git.GetLibGit2LastErr();
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
		return false;

	// another versionstring for the filename can be provided
	// this is needed for preview releases
	m_versioncheckfile.GetString(L"tortoisegit.versionstring", m_version.version_for_filename);
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
	CString infotext;
	m_versioncheckfile.GetString(L"tortoisegit.infotext", infotext);
	return infotext;
}

CString CVersioncheckParser::GetTortoiseGitInfoTextURL()
{
	CString infotexturl;
	m_versioncheckfile.GetString(L"tortoisegit.infotexturl", infotexturl);
	return infotexturl;
}

CString CVersioncheckParser::GetTortoiseGitIssuesURL()
{
	CString issueurl;
	if (m_versioncheckfile.GetString(L"tortoisegit.issuesurl", issueurl))
		issueurl = L"https://tortoisegit.org/issue/%BUGID%";
	return issueurl;
}

CString CVersioncheckParser::GetTortoiseGitChangelogURL()
{
	CString changelogurl;
	m_versioncheckfile.GetString(L"tortoisegit.changelogurl", changelogurl);
	if (changelogurl.IsEmpty())
		changelogurl = L"https://versioncheck.tortoisegit.org/changelog.txt";
	return changelogurl;
}

CString CVersioncheckParser::GetTortoiseGitBaseURL()
{
	CString baseurl;
	m_versioncheckfile.GetString(L"tortoisegit.baseurl", baseurl);
	if (baseurl.IsEmpty())
		baseurl.Format(L"http://updater.download.tortoisegit.org/tgit/%s/", (LPCTSTR)m_version.version_for_filename);
	return baseurl;
}

bool CVersioncheckParser::GetTortoiseGitIsHotfix()
{
	bool ishotfix = false;
	m_versioncheckfile.GetBool(L"tortoisegit.hotfix", ishotfix);
	return ishotfix;
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
	CString mainfilenametemplate;
	m_versioncheckfile.GetString(L"tortoisegit.mainfilename", mainfilenametemplate);
	if (mainfilenametemplate.IsEmpty())
		mainfilenametemplate = L"TortoiseGit-%1!s!-%2!s!bit.msi";
	CString mainfilename;
	mainfilename.FormatMessage(mainfilenametemplate, (LPCTSTR)m_version.version_for_filename, (LPCTSTR)x86x64());
	return mainfilename;
}

CString CVersioncheckParser::GetTortoiseGitLanguagepackFilenameTemplate()
{
	CString languagepackfilenametemplate;
	m_versioncheckfile.GetString(L"tortoisegit.languagepackfilename", languagepackfilenametemplate);
	if (languagepackfilenametemplate.IsEmpty())
		languagepackfilenametemplate = L"TortoiseGit-LanguagePack-%1!s!-%2!s!bit-%3!s!.msi";
	return languagepackfilenametemplate;
}

CVersioncheckParser::LANGPACK_VECTOR CVersioncheckParser::GetTortoiseGitLanguagePacks()
{
	CString languagepackfilenametemplate = GetTortoiseGitLanguagepackFilenameTemplate();
	LANGPACK_VECTOR vec;
	git_config_get_multivar_foreach(m_versioncheckfile, "tortoisegit.langs", nullptr, [](const git_config_entry* configentry, void* payload) -> int
	{
		LANGPACK_VECTOR* languagePacks = (LANGPACK_VECTOR*)payload;
		CString langs = CUnicodeUtils::GetUnicode(configentry->value);

		LanguagePack pack;

		int nextTokenPos = langs.Find(L";", 5); // be extensible for the future
		if (nextTokenPos > 0)
			langs = langs.Left(nextTokenPos);
		pack.m_LangCode = langs.Mid(5);
		pack.m_PackName = L"TortoiseGit Language Pack " + pack.m_LangCode;

		pack.m_LocaleID = _tstoi(langs.Mid(0, 4));
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

		languagePacks->push_back(pack);

		return 0;
	}, &vec);

	for (auto& pack : vec)
		pack.m_filename.FormatMessage(languagepackfilenametemplate, (LPCTSTR)m_version.version_for_filename, (LPCTSTR)x86x64(), (LPCTSTR)pack.m_LangCode, pack.m_LocaleID);

	return vec;
}
