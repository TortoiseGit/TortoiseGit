// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2019 - TortoiseGit

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
#include "ProjectProperties.h"
#include "CommonAppUtils.h"
#include "Git.h"
#include "UnicodeUtils.h"
#include "TempFile.h"
#include <WinInet.h>

struct num_compare
{
	bool operator() (const CString& lhs, const CString& rhs) const
	{
		return StrCmpLogicalW(lhs, rhs) < 0;
	}
};

ProjectProperties::ProjectProperties(void)
	: regExNeedUpdate (true)
	, nBugIdPos(-1)
	, bWarnNoSignedOffBy(FALSE)
	, bNumber(TRUE)
	, bWarnIfNoIssue(FALSE)
	, nLogWidthMarker(0)
	, nMinLogSize(0)
	, bFileListInEnglish(TRUE)
	, bAppend(TRUE)
	, lProjectLanguage(0)
{
}

int ProjectProperties::ReadProps()
{
	CAutoConfig gitconfig(true);
	CAutoRepository repo(g_Git.GetGitRepository());
	CString adminDirPath;
	if (GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, adminDirPath))
		git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(adminDirPath + L"config"), GIT_CONFIG_LEVEL_APP, repo, FALSE); // this needs to have the highest priority in order to override .tgitconfig settings

	if (!GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
		git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.CombinePath(L".tgitconfig")), GIT_CONFIG_LEVEL_LOCAL, nullptr, FALSE); // this needs to have the second highest priority
	else
	{
		CString tmpFile = CTempFiles::Instance().GetTempFilePath(true).GetWinPathString();
		CTGitPath path(L".tgitconfig");
		if (g_Git.GetOneFile(L"HEAD", path, tmpFile) == 0)
			git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(tmpFile), GIT_CONFIG_LEVEL_LOCAL, nullptr, FALSE); // this needs to have the second highest priority
	}

	git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, repo, FALSE);
	git_config_add_file_ondisk(gitconfig,CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, repo, FALSE);
	git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.GetGitSystemConfig()), GIT_CONFIG_LEVEL_SYSTEM, repo, FALSE);
	if (!g_Git.ms_bCygwinGit && !g_Git.ms_bMsys2Git)
		git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.GetGitProgramDataConfig()), GIT_CONFIG_LEVEL_PROGRAMDATA, repo, FALSE);
	git_error_clear();

	CString sPropVal;

	gitconfig.GetString(BUGTRAQPROPNAME_LABEL, sLabel);
	gitconfig.GetString(BUGTRAQPROPNAME_MESSAGE, sMessage);
	nBugIdPos = sMessage.Find(L"%BUGID%");
	gitconfig.GetString(BUGTRAQPROPNAME_URL, sUrl);

	gitconfig.GetBOOL(BUGTRAQPROPNAME_WARNIFNOISSUE, bWarnIfNoIssue);
	gitconfig.GetBOOL(BUGTRAQPROPNAME_NUMBER, bNumber);
	gitconfig.GetBOOL(BUGTRAQPROPNAME_APPEND, bAppend);

	gitconfig.GetString(BUGTRAQPROPNAME_PROVIDERUUID, sProviderUuid);
	gitconfig.GetString(BUGTRAQPROPNAME_PROVIDERUUID64, sProviderUuid64);
	gitconfig.GetString(BUGTRAQPROPNAME_PROVIDERPARAMS, sProviderParams);

	gitconfig.GetBOOL(PROJECTPROPNAME_WARNNOSIGNEDOFFBY, bWarnNoSignedOffBy);
	gitconfig.GetString(PROJECTPROPNAME_ICON, sIcon);

	gitconfig.GetString(BUGTRAQPROPNAME_LOGREGEX, sPropVal);

	sCheckRe = sPropVal;
	if (sCheckRe.Find('\n')>=0)
	{
		sBugIDRe = sCheckRe.Mid(sCheckRe.Find('\n')).Trim();
		sCheckRe = sCheckRe.Left(sCheckRe.Find('\n')).Trim();
	}
	if (!sCheckRe.IsEmpty())
		sCheckRe = sCheckRe.Trim();

	if (gitconfig.GetString(PROJECTPROPNAME_LOGWIDTHLINE, sPropVal) == 0)
	{
		CString val;
		val = sPropVal;
		if (!val.IsEmpty())
			nLogWidthMarker = _wtoi(val);
	}

	if (gitconfig.GetString(PROJECTPROPNAME_PROJECTLANGUAGE, sPropVal) == 0)
	{
		CString val;
		val = sPropVal;
		if (val == L"-1")
			lProjectLanguage = -1;
		if (!val.IsEmpty())
		{
			LPTSTR strEnd;
			lProjectLanguage = wcstol(val, &strEnd, 0);
		}
	}

	if (gitconfig.GetString(PROJECTPROPNAME_LOGMINSIZE, sPropVal) == 0)
	{
		CString val;
		val = sPropVal;
		if (!val.IsEmpty())
			nMinLogSize = _wtoi(val);
	}

	FetchHookString(gitconfig, PROJECTPROPNAME_STARTCOMMITHOOK, sStartCommitHook);
	FetchHookString(gitconfig, PROJECTPROPNAME_PRECOMMITHOOK, sPreCommitHook);
	FetchHookString(gitconfig, PROJECTPROPNAME_POSTCOMMITHOOK, sPostCommitHook);
	FetchHookString(gitconfig, PROJECTPROPNAME_PREPUSHHOOK, sPrePushHook);
	FetchHookString(gitconfig, PROJECTPROPNAME_POSTPUSHHOOK, sPostPushHook);
	FetchHookString(gitconfig, PROJECTPROPNAME_PREREBASEHOOK, sPreRebaseHook);

	return 0;
}

CString ProjectProperties::GetBugIDFromLog(CString& msg)
{
	CString sBugID;

	if (!sMessage.IsEmpty())
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (nBugIdPos < 0)
			return sBugID;
		sFirstPart = sMessage.Left(nBugIdPos);
		sLastPart = sMessage.Mid(nBugIdPos + 7);
		msg.TrimRight('\n');
		if (msg.ReverseFind('\n')>=0)
		{
			if (bAppend)
				sBugLine = msg.Mid(msg.ReverseFind('\n')+1);
			else
			{
				sBugLine = msg.Left(msg.Find('\n'));
				bTop = TRUE;
			}
		}
		else
		{
			if (bNumber)
			{
				// find out if the message consists only of numbers
				bool bOnlyNumbers = true;
				for (int i=0; i<msg.GetLength(); ++i)
				{
					if (!_istdigit(msg[i]))
					{
						bOnlyNumbers = false;
						break;
					}
				}
				if (bOnlyNumbers)
					sBugLine = msg;
			}
			else
				sBugLine = msg;
		}
		if (sBugLine.IsEmpty() && (msg.ReverseFind('\n') < 0))
			sBugLine = msg.Mid(msg.ReverseFind('\n')+1);
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (msg.Find('\n')>=0)
				sBugLine = msg.Left(msg.Find('\n'));
			if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
				sBugLine.Empty();
			if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
				sBugLine.Empty();
			bTop = TRUE;
		}
		if (sBugLine.IsEmpty())
			return sBugID;
		sBugID = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (bTop)
		{
			msg = msg.Mid(sBugLine.GetLength());
			msg.TrimLeft('\n');
		}
		else
		{
			msg = msg.Left(msg.GetLength()-sBugLine.GetLength());
			msg.TrimRight('\n');
		}
	}
	return sBugID;
}

void ProjectProperties::AutoUpdateRegex()
{
	if (regExNeedUpdate)
	{
		try
		{
			regCheck = std::wregex(sCheckRe);
			regBugID = std::wregex(sBugIDRe);
		}
		catch (std::exception&)
		{
		}

		regExNeedUpdate = false;
	}
}

void ProjectProperties::FetchHookString(CAutoConfig& gitconfig, const CString& sBase, CString& sHook)
{
	sHook.Empty();
	CString sVal;
	gitconfig.GetString(sBase + L"cmdline", sVal);
	if (sVal.IsEmpty())
		return;
	sHook += sVal + L'\n';
	bool boolval = false;
	gitconfig.GetBool(sBase + L"wait", boolval);
	if (boolval)
		sHook += L"true\n";
	else
		sHook += L"false\n";
	boolval = false;
	gitconfig.GetBool(sBase + L"show", boolval);
	if (boolval)
		sHook += L"true";
	else
		sHook += L"false";
}

std::vector<CHARRANGE> ProjectProperties::FindBugIDPositions(const CString& msg)
{
	size_t offset1 = 0;
	size_t offset2 = 0;
	std::vector<CHARRANGE> result;

	// first use the checkre string to find bug ID's in the message
	if (!sCheckRe.IsEmpty())
	{
		if (!sBugIDRe.IsEmpty())
		{
			// match with two regex strings (without grouping!)
			try
			{
				AutoUpdateRegex();
				const std::wsregex_iterator end;
				std::wstring s = msg;
				for (std::wsregex_iterator it(s.cbegin(), s.cend(), regCheck); it != end; ++it)
				{
					// (*it)[0] is the matched string
					std::wstring matchedString = (*it)[0];
					ptrdiff_t matchpos = it->position(0);
					for (std::wsregex_iterator it2(matchedString.cbegin(), matchedString.cend(), regBugID); it2 != end; ++it2)
					{
						ATLTRACE(L"matched id : %s\n", (*it2)[0].str().c_str());
						ptrdiff_t matchposID = it2->position(0);
						CHARRANGE range = { static_cast<LONG>(matchpos + matchposID), static_cast<LONG>(matchpos+matchposID + (*it2)[0].str().size()) };
						result.push_back(range);
					}
				}
			}
			catch (std::exception&) {}
		}
		else
		{
			try
			{
				AutoUpdateRegex();
				const std::wsregex_iterator end;
				std::wstring s = msg;
				for (std::wsregex_iterator it(s.cbegin(), s.cend(), regCheck); it != end; ++it)
				{
					const std::wsmatch match = *it;
					// we define group 1 as the whole issue text and
					// group 2 as the bug ID
					if (match.size() >= 2)
					{
						ATLTRACE(L"matched id : %s\n", std::wstring(match[1]).c_str());
						CHARRANGE range = { static_cast<LONG>(match[1].first - s.cbegin()), static_cast<LONG>(match[1].second - s.cbegin()) };
						result.push_back(range);
					}
				}
			}
			catch (std::exception&) {}
		}
	}
	else if (result.empty() && (!sMessage.IsEmpty()))
	{
		CString sBugLine;
		CString sFirstPart;
		CString sLastPart;
		BOOL bTop = FALSE;
		if (nBugIdPos < 0)
			return result;

		sFirstPart = sMessage.Left(nBugIdPos);
		sLastPart = sMessage.Mid(nBugIdPos + 7);
		CString sMsg = msg;
		sMsg.TrimRight('\n');
		if (sMsg.ReverseFind('\n')>=0)
		{
			if (bAppend)
				sBugLine = sMsg.Mid(sMsg.ReverseFind('\n')+1);
			else
			{
				sBugLine = sMsg.Left(sMsg.Find('\n'));
				bTop = TRUE;
			}
		}
		else
			sBugLine = sMsg;
		if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
			sBugLine.Empty();
		if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
			sBugLine.Empty();
		if (sBugLine.IsEmpty())
		{
			if (sMsg.Find('\n')>=0)
				sBugLine = sMsg.Left(sMsg.Find('\n'));
			if (sBugLine.Left(sFirstPart.GetLength()).Compare(sFirstPart)!=0)
				sBugLine.Empty();
			if (sBugLine.Right(sLastPart.GetLength()).Compare(sLastPart)!=0)
				sBugLine.Empty();
			bTop = TRUE;
		}
		if (sBugLine.IsEmpty())
			return result;

		CString sBugIDPart = sBugLine.Mid(sFirstPart.GetLength(), sBugLine.GetLength() - sFirstPart.GetLength() - sLastPart.GetLength());
		if (sBugIDPart.IsEmpty())
			return result;

		//the bug id part can contain several bug id's, separated by commas
		if (!bTop)
			offset1 = sMsg.GetLength() - sBugLine.GetLength() + sFirstPart.GetLength();
		else
			offset1 = sFirstPart.GetLength();
		sBugIDPart.Trim(L',');
		while (sBugIDPart.Find(',')>=0)
		{
			offset2 = offset1 + sBugIDPart.Find(',');
			CHARRANGE range = { static_cast<LONG>(offset1), static_cast<LONG>(offset2) };
			result.push_back(range);
			sBugIDPart = sBugIDPart.Mid(sBugIDPart.Find(',')+1);
			offset1 = offset2 + 1;
		}
		offset2 = offset1 + sBugIDPart.GetLength();
		CHARRANGE range = { static_cast<LONG>(offset1), static_cast<LONG>(offset2) };
		result.push_back(range);
	}

	return result;
}

BOOL ProjectProperties::FindBugID(const CString& msg, CWnd * pWnd)
{
	std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
	CCommonAppUtils::SetCharFormat(pWnd, CFM_LINK, CFE_LINK, positions);

	return positions.empty() ? FALSE : TRUE;
}

std::set<CString> ProjectProperties::FindBugIDs (const CString& msg)
{
	std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
	std::set<CString> bugIDs;

	for (const auto& pos : positions)
		bugIDs.insert(msg.Mid(pos.cpMin, pos.cpMax - pos.cpMin));

	return bugIDs;
}

CString ProjectProperties::FindBugID(const CString& msg)
{
	CString sRet;
	if (!sCheckRe.IsEmpty() || (nBugIdPos >= 0))
	{
		std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
		std::set<CString, num_compare> bugIDs;
		for (const auto& pos : positions)
			bugIDs.insert(msg.Mid(pos.cpMin, pos.cpMax - pos.cpMin));

		for (const auto& id : bugIDs)
		{
			sRet += id;
			sRet += L' ';
		}
		sRet.Trim();
	}

	return sRet;
}

bool ProjectProperties::MightContainABugID()
{
	return !sCheckRe.IsEmpty() || (nBugIdPos >= 0);
}

CString ProjectProperties::GetBugIDUrl(const CString& sBugID)
{
	CString ret;
	if (sUrl.IsEmpty())
		return ret;
	if (!sMessage.IsEmpty() || !sCheckRe.IsEmpty())
	{
		ret = sUrl;
		CString parameter;
		DWORD size = INTERNET_MAX_URL_LENGTH;
		UrlEscape(sBugID, CStrBuf(parameter, size + 1), &size, URL_ESCAPE_SEGMENT_ONLY | URL_ESCAPE_PERCENT | URL_ESCAPE_AS_UTF8);
		// UrlEscape does not escape + and =, starting with Win8 the URL_ESCAPE_ASCII_URI_COMPONENT flag could be used and the following two lines would not be necessary
		parameter.Replace(L"+", L"%2B");
		parameter.Replace(L"=", L"%3D");
		ret.Replace(L"%BUGID%", parameter);
	}
	return ret;
}

BOOL ProjectProperties::CheckBugID(const CString& sID)
{
	if (bNumber)
	{
		// check if the revision actually _is_ a number
		// or a list of numbers separated by colons
		int len = sID.GetLength();
		for (int i=0; i<len; ++i)
		{
			TCHAR c = sID.GetAt(i);
			if ((c < '0')&&(c != ',')&&(c != ' '))
				return FALSE;
			if (c > '9')
				return FALSE;
		}
	}
	return TRUE;
}

BOOL ProjectProperties::HasBugID(const CString& sMsg)
{
	if (!sCheckRe.IsEmpty())
	{
		try
		{
			AutoUpdateRegex();
			return std::regex_search(static_cast<LPCTSTR>(sMsg), regCheck);
		}
		catch (std::exception&) {}
	}
	return FALSE;
}
