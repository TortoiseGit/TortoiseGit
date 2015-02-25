// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2011,2013-2015 - TortoiseGit

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
#include "AppUtils.h"
#include "Git.h"
#include "UnicodeUtils.h"


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
{
	bNumber = TRUE;
	bWarnIfNoIssue = FALSE;
	nLogWidthMarker = 0;
	nMinLogSize = 0;
	bFileListInEnglish = TRUE;
	bAppend = TRUE;
	lProjectLanguage = 0;
}

int ProjectProperties::ReadProps()
{
	CAutoConfig gitconfig(true);
	CString adminDirPath;
	if (GitAdminDir::GetAdminDirPath(g_Git.m_CurrentDir, adminDirPath))
		git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(adminDirPath + L"config"), GIT_CONFIG_LEVEL_APP, FALSE); // this needs to have the highest priority in order to override .tgitconfig settings

	if (!GitAdminDir::IsBareRepo(g_Git.m_CurrentDir))
		git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.CombinePath(L".tgitconfig")), GIT_CONFIG_LEVEL_LOCAL, FALSE); // this needs to have the second highest priority
	else
	{
		CString tmpFile = GetTempFile();
		CTGitPath path(_T(".tgitconfig"));
		if (g_Git.GetOneFile(_T("HEAD"), path, tmpFile) == 0)
			git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(tmpFile), GIT_CONFIG_LEVEL_LOCAL, FALSE); // this needs to have the second highest priority
	}

	git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.GetGitGlobalConfig()), GIT_CONFIG_LEVEL_GLOBAL, FALSE);
	git_config_add_file_ondisk(gitconfig,CGit::GetGitPathStringA(g_Git.GetGitGlobalXDGConfig()), GIT_CONFIG_LEVEL_XDG, FALSE);
	git_config_add_file_ondisk(gitconfig, CGit::GetGitPathStringA(g_Git.ms_LastMsysGitDir + _T("\\..\\etc\\gitconfig")), GIT_CONFIG_LEVEL_SYSTEM, FALSE);
	giterr_clear();

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
	{
		sCheckRe = sCheckRe.Trim();
	}

	if (gitconfig.GetString(PROJECTPROPNAME_LOGWIDTHLINE, sPropVal) == 0)
	{
		CString val;
		val = sPropVal;
		if (!val.IsEmpty())
			nLogWidthMarker = _ttoi(val) + 2; // HACK, + 2 needed
	}

	if (gitconfig.GetString(PROJECTPROPNAME_PROJECTLANGUAGE, sPropVal) == 0)
	{
		CString val;
		val = sPropVal;
		if (val == _T("-1"))
			lProjectLanguage = -1;
		if (!val.IsEmpty())
		{
			LPTSTR strEnd;
			lProjectLanguage = _tcstol(val, &strEnd, 0);
		}
	}

	if (gitconfig.GetString(PROJECTPROPNAME_LOGMINSIZE, sPropVal) == 0)
	{
		CString val;
		val = sPropVal;
		if (!val.IsEmpty())
			nMinLogSize = _ttoi(val);
	}

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
		{
			sBugLine = msg.Mid(msg.ReverseFind('\n')+1);
		}
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
			regCheck = std::tr1::wregex(sCheckRe);
			regBugID = std::tr1::wregex(sBugIDRe);
		}
		catch (std::exception)
		{
		}

		regExNeedUpdate = false;
	}
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
				const std::tr1::wsregex_iterator end;
				std::wstring s = msg;
				for (std::tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
				{
					// (*it)[0] is the matched string
					std::wstring matchedString = (*it)[0];
					ptrdiff_t matchpos = it->position(0);
					for (std::tr1::wsregex_iterator it2(matchedString.begin(), matchedString.end(), regBugID); it2 != end; ++it2)
					{
						ATLTRACE(_T("matched id : %s\n"), (*it2)[0].str().c_str());
						ptrdiff_t matchposID = it2->position(0);
						CHARRANGE range = {(LONG)(matchpos+matchposID), (LONG)(matchpos+matchposID+(*it2)[0].str().size())};
						result.push_back(range);
					}
				}
			}
			catch (std::exception) {}
		}
		else
		{
			try
			{
				AutoUpdateRegex();
				const std::tr1::wsregex_iterator end;
				std::wstring s = msg;
				for (std::tr1::wsregex_iterator it(s.begin(), s.end(), regCheck); it != end; ++it)
				{
					const std::tr1::wsmatch match = *it;
					// we define group 1 as the whole issue text and
					// group 2 as the bug ID
					if (match.size() >= 2)
					{
						ATLTRACE(_T("matched id : %s\n"), std::wstring(match[1]).c_str());
						CHARRANGE range = {(LONG)(match[1].first-s.begin()), (LONG)(match[1].second-s.begin())};
						result.push_back(range);
					}
				}
			}
			catch (std::exception) {}
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
		sBugIDPart.Trim(_T(","));
		while (sBugIDPart.Find(',')>=0)
		{
			offset2 = offset1 + sBugIDPart.Find(',');
			CHARRANGE range = {(LONG)offset1, (LONG)offset2};
			result.push_back(range);
			sBugIDPart = sBugIDPart.Mid(sBugIDPart.Find(',')+1);
			offset1 = offset2 + 1;
		}
		offset2 = offset1 + sBugIDPart.GetLength();
		CHARRANGE range = {(LONG)offset1, (LONG)offset2};
		result.push_back(range);
	}

	return result;
}

BOOL ProjectProperties::FindBugID(const CString& msg, CWnd * pWnd)
{
	std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
	CAppUtils::SetCharFormat(pWnd, CFM_LINK, CFE_LINK, positions);

	return positions.empty() ? FALSE : TRUE;
}

std::set<CString> ProjectProperties::FindBugIDs (const CString& msg)
{
	std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
	std::set<CString> bugIDs;

	for (std::vector<CHARRANGE>::iterator iter = positions.begin(), end = positions.end(); iter != end; ++iter)
	{
		bugIDs.insert(msg.Mid(iter->cpMin, iter->cpMax - iter->cpMin));
	}

	return bugIDs;
}

CString ProjectProperties::FindBugID(const CString& msg)
{
	CString sRet;
	if (!sCheckRe.IsEmpty() || (nBugIdPos >= 0))
	{
		std::vector<CHARRANGE> positions = FindBugIDPositions(msg);
		std::set<CString, num_compare> bugIDs;
		for (std::vector<CHARRANGE>::iterator iter = positions.begin(), end = positions.end(); iter != end; ++iter)
		{
			bugIDs.insert(msg.Mid(iter->cpMin, iter->cpMax - iter->cpMin));
		}

		for (std::set<CString, num_compare>::iterator it = bugIDs.begin(); it != bugIDs.end(); ++it)
		{
			sRet += *it;
			sRet += _T(" ");
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
		ret.Replace(_T("%BUGID%"), sBugID);
	}
	return ret;
}

BOOL ProjectProperties::CheckBugID(const CString& sID)
{
	if (bNumber)
	{
		// check if the revision actually _is_ a number
		// or a list of numbers separated by colons
		TCHAR c = 0;
		int len = sID.GetLength();
		for (int i=0; i<len; ++i)
		{
			c = sID.GetAt(i);
			if ((c < '0')&&(c != ',')&&(c != ' '))
			{
				return FALSE;
			}
			if (c > '9')
				return FALSE;
		}
	}
	return TRUE;
}

BOOL ProjectProperties::HasBugID(const CString& sMessage)
{
	if (!sCheckRe.IsEmpty())
	{
		try
		{
			AutoUpdateRegex();
			return std::tr1::regex_search((LPCTSTR)sMessage, regCheck);
		}
		catch (std::exception) {}
	}
	return FALSE;
}

#ifdef DEBUG
static class PropTest
{
public:
	PropTest()
	{
		CString msg = _T("this is a test logmessage: issue 222\nIssue #456, #678, 901  #456");
		CString sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		CString sCheckRe = _T("[Ii]ssue #?(\\d+)(,? ?#?(\\d+))+");
		CString sBugIDRe = _T("(\\d+)");
		ProjectProperties props;
		props.sCheckRe = _T("PAF-[0-9]+");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		CString sRet = props.FindBugID(_T("This is a test for PAF-88"));
		ATLASSERT(sRet.IsEmpty());
		props.sCheckRe = _T("[Ii]ssue #?(\\d+)");
		props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("Testing issue #99"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("99"))==0);
		props.sCheckRe = _T("[Ii]ssues?:?(\\s*(,|and)?\\s*#\\d+)+");
		props.sBugIDRe = _T("(\\d+)");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("This is a test for Issue #7463,#666"));
		ATLASSERT(props.HasBugID(_T("This is a test for Issue #7463,#666")));
		ATLASSERT(!props.HasBugID(_T("This is a test for Issue 7463,666")));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("666 7463"))==0);
		sRet = props.FindBugID(_T("This is a test for Issue #850,#1234,#1345"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("850 1234 1345"))==0);
		props.sCheckRe = _T("^\\[(\\d+)\\].*");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("[000815] some stupid programming error fixed"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("000815"))==0);
		props.sCheckRe = _T("\\[\\[(\\d+)\\]\\]\\]");
		props.sUrl = _T("http://tortoisesvn.tigris.org/issues/show_bug.cgi?id=%BUGID%");
		props.regExNeedUpdate = true;
		sRet = props.FindBugID(_T("test test [[000815]]] some stupid programming error fixed"));
		sRet.Trim();
		ATLASSERT(sRet.Compare(_T("000815"))==0);
		ATLASSERT(props.HasBugID(_T("test test [[000815]]] some stupid programming error fixed")));
		ATLASSERT(!props.HasBugID(_T("test test [000815]] some stupid programming error fixed")));
	}
} PropTest;
#endif



