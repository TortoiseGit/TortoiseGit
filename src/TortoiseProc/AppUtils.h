// TortoiseSVN - a Windows shell extension for easy version control

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
#pragma once
#include "HistoryCombo.h"
#include "GitRev.h"

class CTGitPath;

/**
 * \ingroup TortoiseProc
 * An utility class with static functions.
 */
class CAppUtils
{
public:
	/**
	* Flags for StartExtDiff function.
	*/
	struct DiffFlags
	{
		bool bWait;
		bool bBlame;
		bool bReadOnly;
		bool bAlternativeTool; // If true, invert selection of TortoiseMerge vs. external diff tool

		DiffFlags(): bWait(false), bBlame(false), bReadOnly(false), bAlternativeTool(false)	{}
		DiffFlags& Wait(bool b = true) { bWait = b; return *this; }
		DiffFlags& Blame(bool b = true) { bBlame = b; return *this; }
		DiffFlags& ReadOnly(bool b = true) { bReadOnly = b; return *this; }
		DiffFlags& AlternativeTool(bool b = true) { bAlternativeTool = b; return *this; }
	};

	CAppUtils(void);
	~CAppUtils(void);

	/**
	 * Launches the external merge program if there is one.
	 * \return TRUE if the program could be started
	 */
	static BOOL StartExtMerge(
		const CTGitPath& basefile, const CTGitPath& theirfile, const CTGitPath& yourfile, const CTGitPath& mergedfile,
		const CString& basename = CString(), const CString& theirname = CString(), const CString& yourname = CString(),
		const CString& mergedname = CString(), bool bReadOnly = false);

	/**
	 * Starts the external patch program (currently always TortoiseMerge)
	 */
	static BOOL StartExtPatch(const CTGitPath& patchfile, const CTGitPath& dir, 
			const CString& sOriginalDescription = CString(), const CString& sPatchedDescription = CString(), 
			BOOL bReversed = FALSE, BOOL bWait = FALSE);

	/**
	 * Starts the external unified diff viewer (the app associated with *.diff or *.patch files).
	 * If no app is associated with those file types, the default text editor is used.
	 */
	static BOOL StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait = FALSE);

	/**
	 * Starts the external diff application
	 */
	static bool StartExtDiff(
		const CString& file1, const CString& file2, 
		const CString& sName1, const CString& sName2, const DiffFlags& flags);

	/**
	 * Starts the external diff application for properties
	 */
	static BOOL StartExtDiffProps(const CTGitPath& file1, const CTGitPath& file2, 
			const CString& sName1 = CString(), const CString& sName2 = CString(),
			BOOL bWait = FALSE, BOOL bReadOnly = FALSE);

	/**
	 * Launches the standard text viewer/editor application which is associated
	 * with txt files.
	 * \return TRUE if the program could be started.
	 */
	static BOOL StartTextViewer(CString file);

	/**
	 * Checks if the given file has a size of less than four, which means
	 * an 'empty' file or just newlines, i.e. an empty diff.
	 */
	static BOOL CheckForEmptyDiff(const CTGitPath& sDiffPath);

	/**
	 * Create a font which can is used for log messages, etc
	 */
	static void CreateFontForLogs(CFont& fontToCreate);

	/**
	* Launch an external application (usually the diff viewer)
	*/
	static bool LaunchApplication(const CString& sCommandLine, UINT idErrMessageFormat, bool bWaitForStartup);

	/**
	* Launch the external blame viewer
	*/
	static bool LaunchTortoiseBlame(
		const CString& sBlameFile, CString Rev, const CString& sParams = CString());
	
	/**
	 * Formats text in a rich edit control (version 2).
	 * text in between * chars is formatted bold
	 * text in between ^ chars is formatted italic
	 * text in between _ chars is underlined
	 */
	static bool FormatTextInRichEditControl(CWnd * pWnd);
	static bool FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end);

	static bool BrowseRepository(CHistoryCombo& combo, CWnd * pParent, GitRev& rev);

	static bool FileOpenSave(CString& path, int * filterindex, UINT title, UINT filter, bool bOpen, HWND hwndOwner = NULL);

	static bool SetListCtrlBackgroundImage(HWND hListCtrl, UINT nID, int width = 128, int height = 128);

	/**
	 * guesses a name of the project from a repository URL
	 */
	static 	CString	GetProjectNameFromURL(CString url);

	/**
	 * Replacement for GitDiff::ShowUnifiedDiff(), but started as a separate process.
	 */
	static bool StartShowUnifiedDiff(HWND hWnd, const CTGitPath& url1,  const git_revnum_t& rev1, 
												const CTGitPath & url2, const git_revnum_t& rev2, 

												//const GitRev& peg = GitRev(), const GitRev& headpeg = GitRev(),
												bool bAlternateDiff = false,
												bool bIgnoreAncestry = false,
												bool /* blame */ = false);

	/**
	 * Replacement for GitDiff::ShowCompare(), but started as a separate process.
	 */
	static bool StartShowCompare(HWND hWnd, const CTGitPath& url1, const GitRev& rev1, 
								const CTGitPath& url2, const GitRev& rev2, 
								const GitRev& peg = GitRev(), const GitRev& headpeg = GitRev(),
								bool bAlternateDiff = false, bool ignoreancestry = false,
								bool blame = false);
	
	static bool Export(CString *BashHash=NULL);
	static bool CreateBranchTag(bool IsTag=TRUE,CString *CommitHash=NULL);
	static bool Switch(CString *CommitHash);

	static bool IgnoreFile(CTGitPath &file, bool IsMask);
	static bool GitReset(CString *CommitHash,int type=1);
	static bool ConflictEdit(CTGitPath &file,bool bAlternativeTool=false);
	/**
	 * FUNCTION    :   FormatDateAndTime
	 * DESCRIPTION :   Generates a displayable string from a CTime object in
	 *                 system short or long format dependant on setting of option
	 *				   as DATE_SHORTDATE or DATE_LONGDATE. bIncludeTime (optional) includes time.
	 * RETURN      :   CString containing date/time
	 */
	static CString FormatDateAndTime( const CTime& cTime, DWORD option, bool bIncludeTime=true,
		bool bRelative=false );
	/**
	 *	Converts a given time to a relative display string (relative to current time)
	 *	Given time must be in local timezone
	 *  If more than a year ago or in the future then normal date/time is shown
	 */
	static CString ToRelativeTimeString(CTime time);

	
private:
	static CString PickDiffTool(const CTGitPath& file1, const CTGitPath& file2);
	static bool GetMimeType(const CTGitPath& file, CString& mimetype);
	/**
	 *	Generates a display string showing the relative time between the two given times as COleDateTimes
	 *	time must be earlier than RelativeTo
	 *  If more than a year ago or time > RelativeTo then an empty string is returned
	 */
	static CString ToRelativeTimeString(COleDateTime time,COleDateTime RelativeTo);
};
