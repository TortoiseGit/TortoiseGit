// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2012 - TortoiseGit
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
#include "CommonAppUtils.h"

class CTGitPath;

enum GIT_POST_CMD
{
	GIT_POST_CMD_PUSH,
	GIT_POST_CMD_DCOMMIT
};
/**
 * \ingroup TortoiseProc
 * An utility class with static functions.
 */
class CAppUtils : public CCommonAppUtils
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
	* Launch the external blame viewer
	*/
	static bool LaunchTortoiseBlame(
		const CString& sBlameFile, CString Rev, const CString& sParams = CString());

	/**
	* Launch alternative editor
	*/
	static bool LaunchAlternativeEditor(const CString& filename);

	/**
	* Sets the title of a dialog
	*/
	static void SetWindowTitle(HWND hWnd, const CString& urlorpath, const CString& dialogname);

	/**
	 * Formats text in a rich edit control (version 2).
	 * text in between * chars is formatted bold
	 * text in between ^ chars is formatted italic
	 * text in between _ chars is underlined
	 */
	static bool FormatTextInRichEditControl(CWnd * pWnd);
	static bool FindStyleChars(const CString& sText, TCHAR stylechar, int& start, int& end);

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
												bool blame  = false,
												bool bMerge = false);

	static bool Export(CString *BashHash=NULL);
	static bool CreateBranchTag(bool IsTag=TRUE,CString *CommitHash=NULL, bool switch_new_brach=false);
	static bool Switch(CString *CommitHash, CString initialRefName = CString(), bool autoClose = false);
	static bool PerformSwitch(CString ref, bool bForce = false, CString sNewBranch = CString(), bool bBranchOverride = false, bool bTrack = false, bool autoClose = false);

	static bool IgnoreFile(CTGitPathList &filelist,bool IsMask);
	static bool GitReset(CString *CommitHash,int type=1);
	static bool ConflictEdit(CTGitPath &file,bool bAlternativeTool=false,bool revertTheirMy=false);

	static CString GetMergeTempFile(CString str,CTGitPath &merge);
	static bool	StashSave();
	static int	StashApply(CString ref, bool showChanges = true);
	static int	StashPop(bool showChanges = true);

	static bool IsSSHPutty();

	static bool LaunchRemoteSetting();

	static bool LaunchPAgent(CString *keyfile=NULL,CString * pRemote=NULL);

	static CString GetClipboardLink();
	static CString ChooseRepository(CString *path);

	static bool SendPatchMail(CTGitPathList &pathlist,bool autoclose=false);
	static bool SendPatchMail(CString &cmd,CString &formatpatchoutput,bool autoclose=false);

	static int  SaveCommitUnicodeFile(CString &filename, CString &mesage);

	static int  GetLogOutputEncode(CGit *pGit=&g_Git);

	static bool Fetch(CString remoteName = _T(""), bool allowRebase = false, bool autoClose = false);
	static bool Push(CString selectLocalBranch = CString(), bool autoClose = false);
	static bool RequestPull(CString endrevision = _T(""), CString repositoryUrl = _T(""));

	static bool CreateMultipleDirectory(const CString &dir);

	static void RemoveTrailSlash(CString &path);

	static BOOL Commit(CString bugid,BOOL bWholeProject,CString &sLogMsg,
					CTGitPathList &pathList,
					CTGitPathList &selectedList,
					bool bSelectFilesForCommit,
					bool autoClose = false);

	static int GetCommitTemplate(CString &temp);

	static BOOL SVNDCommit();
	static BOOL Merge(CString *commit =NULL);
	static void RemoveTempMergeFile(CTGitPath &path);
	static void EditNote(GitRev *hash);
	static int GetMsysgitVersion(CString *versionstring=NULL);
	static void MarkWindowAsUnpinnable(HWND hWnd);

private:
	static CString PickDiffTool(const CTGitPath& file1, const CTGitPath& file2);
	static bool GetMimeType(const CTGitPath& file, CString& mimetype);

	static bool OpenIgnoreFile(CStdioFile &file, const CString& filename);

	static void DescribeFile(bool mode, bool base,CString &descript);
};
