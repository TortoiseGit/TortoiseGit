// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2009, 2011-2014, 2018 - TortoiseGit
// Copyright (C) 2003-2008,2011-2012 - TortoiseSVN

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
#include "TGitPath.h"
#include <regex>

#define BUGTRAQPROPNAME_LABEL				L"bugtraq.label"
#define BUGTRAQPROPNAME_MESSAGE				L"bugtraq.message"
#define BUGTRAQPROPNAME_NUMBER				L"bugtraq.number"
#define BUGTRAQPROPNAME_LOGREGEX			L"bugtraq.logregex"
#define BUGTRAQPROPNAME_URL					L"bugtraq.url"
#define BUGTRAQPROPNAME_WARNIFNOISSUE		L"bugtraq.warnifnoissue"
#define BUGTRAQPROPNAME_APPEND				L"bugtraq.append"
#define BUGTRAQPROPNAME_PROVIDERUUID		L"bugtraq.provideruuid"
#define BUGTRAQPROPNAME_PROVIDERUUID64		L"bugtraq.provideruuid64"
#define BUGTRAQPROPNAME_PROVIDERPARAMS		L"bugtraq.providerparams"

#define PROJECTPROPNAME_LOGWIDTHLINE		L"tgit.logwidthmarker"
#define PROJECTPROPNAME_LOGMINSIZE			L"tgit.logminsize"
#define PROJECTPROPNAME_LOGFILELISTLANG		L"tsvn.logfilelistenglish"
#define PROJECTPROPNAME_PROJECTLANGUAGE		L"tgit.projectlanguage"
#define PROJECTPROPNAME_WARNNOSIGNEDOFFBY	L"tgit.warnnosignedoffby"
#define PROJECTPROPNAME_ICON				L"tgit.icon"

#define PROJECTPROPNAME_WEBVIEWER_REV		L"webviewer.revision"
#define PROJECTPROPNAME_WEBVIEWER_PATHREV	L"webviewer.pathrevision"

#define PROJECTPROPNAME_STARTCOMMITHOOK		L"hook.startcommit."
#define PROJECTPROPNAME_PRECOMMITHOOK		L"hook.precommit."
#define PROJECTPROPNAME_POSTCOMMITHOOK		L"hook.postcommit."
#define PROJECTPROPNAME_PREPUSHHOOK			L"hook.prepush."
#define PROJECTPROPNAME_POSTPUSHHOOK		L"hook.postpush."
#define PROJECTPROPNAME_PREREBASEHOOK		L"hook.prerebase."

/**
 * \ingroup TortoiseProc
 * Provides methods for retrieving information about bug/issue trackers
 * associated with a git repository/working copy and other project
 * related properties.
 */
class ProjectProperties
{
public:
	ProjectProperties(void);

	/**
	 * Reads the properties from the current working tree
	 */
	BOOL ReadProps();

public:
	/**
	 * Searches for the BugID inside a log message. If one is found,
	 * the method returns TRUE. The rich edit control is used to set
	 * the CFE_LINK effect on the BugID's.
	 * \param msg the log message
	 * \param pWnd Pointer to a rich edit control
	 */
#ifdef _RICHEDIT_
	std::vector<CHARRANGE> FindBugIDPositions(const CString& msg);
#endif
	BOOL FindBugID(const CString& msg, CWnd * pWnd);

	CString FindBugID(const CString& msg);
	std::set<CString> FindBugIDs(const CString& msg);

	/**
	 * Check whether calling \ref FindBugID or \ref FindBugIDPositions
	 * is worthwhile. If the result is @a false, those functions would
	 * return empty strings or sets, respectively.
	 */
	bool MightContainABugID();

	/**
	 * Searches for the BugID inside a log message. If one is found,
	 * that BugID is returned. If none is found, an empty string is returned.
	 * The \c msg is trimmed off the BugID.
	 */
	CString GetBugIDFromLog(CString& msg);

	/**
	 * Checks if the bug ID is valid. If bugtraq:number is 'true', then the
	 * functions checks if the bug ID doesn't contain any non-number chars in it.
	 */
	BOOL CheckBugID(const CString& sID);

	/**
	 * Checks if the log message \c sMessage contains a bug ID. This is done by
	 * using the bugtraq:checkre property.
	 */
	BOOL HasBugID(const CString& sMessage);

	/**
	 * Returns the URL pointing to the Issue in the issue tracker. The URL is
	 * created from the bugtraq:url property and the BugID found in the log message.
	 * \param msg the BugID extracted from the log message
	 */
	CString GetBugIDUrl(const CString& sBugID);

	/** replaces bNumer: a regular expression string to check the validity of
	  * the entered bug ID. */
	const CString& GetCheckRe() const {return sCheckRe;}
	void SetCheckRe(const CString& s) {sCheckRe = s;regExNeedUpdate=true;AutoUpdateRegex();}

	/** used to extract the bug ID from the string matched by sCheckRe */
	const CString& GetBugIDRe() const {return sBugIDRe;}
	void SetBugIDRe(const CString& s) {sBugIDRe = s;regExNeedUpdate=true;AutoUpdateRegex();}

#ifdef _WIN64
	const CString& GetProviderUUID() const { return (sProviderUuid64.IsEmpty() ? sProviderUuid : sProviderUuid64); }
#else
	const CString& GetProviderUUID() const { return (sProviderUuid.IsEmpty() ? sProviderUuid64 : sProviderUuid); }
#endif

public:
	/** The label to show in the commit dialog where the issue number/bug id
	 * is entered. Example: "Bug-ID: " or "Issue-No.:". Default is "Bug-ID :" */
	CString		sLabel;

	/** The message string to add below the log message the user entered.
	 * It must contain the string "%BUGID%" which gets replaced by the client
	 * with the issue number / bug id the user entered. */
	CString		sMessage;

	/** If this is set, then the bug-id / issue number must be a number, no text */
	BOOL		bNumber;

	/** replaces bNumer: a regular expression string to check the validity of
	  * the entered bug ID. */
	CString		sCheckRe;

	/** used to extract the bug ID from the string matched by sCheckRe */
	CString		sBugIDRe;

	/** The url pointing to the issue tracker. If the url contains the string
	 * "%BUGID% the client has to replace it with the issue number / bug id
	 * the user entered. */
	CString		sUrl;

	/** If set to TRUE, show a warning dialog if the user forgot to enter
	 * an issue number in the commit dialog. */
	BOOL		bWarnIfNoIssue;

	/** If set to FALSE, then the bug tracking entry is inserted at the top of the
	   log message instead of at the bottom. Default is TRUE */
	BOOL		bAppend;

	/** the parameters passed to the COM bugtraq provider which implements the
	   IBugTraqProvider interface */
	CString		sProviderParams;

	/** The number of chars the width marker should be shown at. If the property
	 * is not set, then this value is 80 by default. */
	int			nLogWidthMarker;

	/** Minimum size a log message must have in chars */
	int			nMinLogSize;

	/** TRUE if the file list to be inserted in the commit dialog should be in
	 * English and not in the localized language. Default is TRUE */
	BOOL		bFileListInEnglish;

	/** The language identifier this project uses for log messages. */
	LONG		lProjectLanguage;

	/** The url pointing to the web viewer. The string %REVISION% is replaced
	 *  with the revision number, "HEAD", or a date */
	CString		sWebViewerRev;

	/** The url pointing to the web viewer. The string %REVISION% is replaced
	 *  with the revision number, "HEAD", or a date. The string %PATH% is replaced
	 *  with the path relative to the repository root, e.g. "/trunk/src/file" */
	CString		sWebViewerPathRev;

	BOOL		bWarnNoSignedOffBy;
	CString		sIcon;

	/**
	 * A regex string to extract revisions from a log message.
	 */
	CString		sLogRevRegex;

	/** the COM uuid of the bugtraq provider which implements the IBugTraqProvider
	   interface. */
	CString		sProviderUuid;
	CString		sProviderUuid64;

	/// multi line string containing the data for a start-commit-hook
	CString		sStartCommitHook;
	/// multi line string containing the data for a pre-commit-hook
	CString		sPreCommitHook;
	/// multi line string containing the data for a post-commit-hook
	CString		sPostCommitHook;
	/// multi line string containing the data for a pre-push-hook
	CString		sPrePushHook;
	/// multi line string containing the data for a post-push-hook
	CString		sPostPushHook;
	/// multi line string containing the data for a pre-rebase-hook
	CString		sPreRebaseHook;

private:
	/**
	 * Constructing regex objects is expensive. Therefore, cache them here.
	 */
	void AutoUpdateRegex();

	void FetchHookString(CAutoConfig& gitconfig, const CString& sBase, CString& sHook);

	bool regExNeedUpdate;
	std::wregex regCheck;
	std::wregex regBugID;

	int			nBugIdPos;				///< result	of sMessage.Find(L"%BUGID%");

#ifdef GTEST_INCLUDE_GTEST_GTEST_H_
	FRIEND_TEST(ProjectPropertiesTest, ParseBugIDs);
#endif
};
