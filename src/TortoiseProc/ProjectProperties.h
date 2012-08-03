// TortoiseGit - a Windows shell extension for easy version control

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
#include <iostream>
#include <string>
#include <set>
#include "TGitPath.h"
using namespace std;
#include <regex>

#define BUGTRAQPROPNAME_LABEL             _T("bugtraq.label")
#define BUGTRAQPROPNAME_MESSAGE           _T("bugtraq.message")
#define BUGTRAQPROPNAME_NUMBER            _T("bugtraq.number")
#define BUGTRAQPROPNAME_LOGREGEX		  _T("bugtraq.logregex")
#define BUGTRAQPROPNAME_URL               _T("bugtraq.url")
#define BUGTRAQPROPNAME_WARNIFNOISSUE     _T("bugtraq.warnifnoissue")
#define BUGTRAQPROPNAME_APPEND		      _T("bugtraq.append")

#define PROJECTPROPNAME_LOGTEMPLATE		  _T("tsvn.logtemplate")
#define PROJECTPROPNAME_LOGWIDTHLINE	  _T("tsvn.logwidthmarker")
#define PROJECTPROPNAME_LOGMINSIZE		  _T("tsvn.logminsize")
#define PROJECTPROPNAME_LOCKMSGMINSIZE	  _T("tsvn.lockmsgminsize")
#define PROJECTPROPNAME_LOGFILELISTLANG	  _T("tsvn.logfilelistenglish")
#define PROJECTPROPNAME_PROJECTLANGUAGE   _T("tsvn.projectlanguage")
#define PROJECTPROPNAME_USERFILEPROPERTY  _T("tsvn.userfileproperties")
#define PROJECTPROPNAME_USERDIRPROPERTY   _T("tsvn.userdirproperties")

#define PROJECTPROPNAME_WEBVIEWER_REV     _T("webviewer.revision")
#define PROJECTPROPNAME_WEBVIEWER_PATHREV _T("webviewer.pathrevision")

/**
 * \ingroup TortoiseProc
 * Provides methods for retrieving information about bug/issue trackers
 * associated with a Subversion repository/working copy and other project
 * related properties.
 */
class ProjectProperties
{
public:
	ProjectProperties(void);
	~ProjectProperties(void);

	/**
	 * Reads the properties from a path. If the path is a file
	 * then the properties are read from the parent folder of that file.
	 * \param path path to a file or a folder
	 */
	BOOL ReadProps(CTGitPath path);
	static BOOL GetStringProps(CString &prop,TCHAR *key,bool bRemoveCR=true);
	static BOOL GetBOOLProps(BOOL &b, TCHAR*key);
	/**
	 * Reads the properties from all paths found in a path list.
	 * This method calls ReadProps() for each path .
	 * \param list of paths
	 */
	BOOL ReadPropsPathList(const CTGitPathList& pathList);

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

	/** the COM uuid of the bugtraq provider which implements the IBugTraqProvider
	   interface. */
	CString		sProviderUuid;

	/** the parameters passed to the COM bugtraq provider which implements the
	   IBugTraqProvider interface */
	CString		sProviderParams;

	/** The number of chars the width marker should be shown at. If the property
	 * is not set, then this value is 80 by default. */
	int			nLogWidthMarker;

	/** The template to use for log messages. */
	CString		sLogTemplate;

	/** Minimum size a log message must have in chars */
	int			nMinLogSize;

	/** Minimum size a lock message must have in chars */
	int			nMinLockMsgSize;

	/** TRUE if the file list to be inserted in the commit dialog should be in
	 * English and not in the localized language. Default is TRUE */
	BOOL		bFileListInEnglish;

	/** The language identifier this project uses for log messages. */
	LONG		lProjectLanguage;

	/** holds user defined properties for files. */
	CString		sFPPath;

	/** holds user defined properties for directories. */
	CString		sDPPath;

	/** The url pointing to the web viewer. The string %REVISION% is replaced
	 *  with the revision number, "HEAD", or a date */
	CString		sWebViewerRev;

	/** The url pointing to the web viewer. The string %REVISION% is replaced
	 *  with the revision number, "HEAD", or a date. The string %PATH% is replaced
	 *  with the path relative to the repository root, e.g. "/trunk/src/file" */
	CString		sWebViewerPathRev;

	/**
	 * A regex string to extract revisions from a log message.
	 */
	CString		sLogRevRegex;
private:
	CTGitPath	propsPath;

	/**
	 * Constructing regex objects is expensive. Therefore, cache them here.
	 */
	void AutoUpdateRegex();

	bool CheckStringProp(CString& s, const std::string& propname, const CString& propval, LPCSTR prop);

	bool regExNeedUpdate;
	std::tr1::wregex regCheck;
	std::tr1::wregex regBugID;

	int			nBugIdPos;				///< result	of sMessage.Find(L"%BUGID%");

#ifdef DEBUG
	friend class PropTest;
#endif
};
