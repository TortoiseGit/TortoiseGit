// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2019 - TortoiseGit
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
#include "GitRevLoglist.h"
#include "CommonAppUtils.h"

class CTGitPath;
struct git_cred;
struct git_indexer_progress;
class CIgnoreFile;
class ProjectProperties;

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
		bool bAlternativeTool; // If true, invert selection of TortoiseGitMerge vs. external diff tool

		DiffFlags(): bWait(false), bBlame(false), bReadOnly(false), bAlternativeTool(false)	{}
		DiffFlags& Wait(bool b = true) { bWait = b; return *this; }
		DiffFlags& Blame(bool b = true) { bBlame = b; return *this; }
		DiffFlags& ReadOnly(bool b = true) { bReadOnly = b; return *this; }
		DiffFlags& AlternativeTool(bool b = true) { bAlternativeTool = b; return *this; }
	};

	CAppUtils() = delete;

public:
	/**
	 * Launches the external merge program if there is one.
	 * \return TRUE if the program could be started
	 */
	static BOOL StartExtMerge(bool bAlternativeTool,
		const CTGitPath& basefile, const CTGitPath& theirfile, const CTGitPath& yourfile, const CTGitPath& mergedfile,
		const CString& basename = CString(), const CString& theirname = CString(), const CString& yourname = CString(),
		const CString& mergedname = CString(), bool bReadOnly = false, HWND resolveMsgHwnd = nullptr, bool bDeleteBaseTheirsMineOnClose = false);

	/**
	 * Starts the external patch program (currently always TortoiseGitMerge)
	 */
	static BOOL StartExtPatch(const CTGitPath& patchfile, const CTGitPath& dir,
			const CString& sOriginalDescription = CString(), const CString& sPatchedDescription = CString(),
			BOOL bReversed = FALSE, BOOL bWait = FALSE);

	/**
	 * Starts the external unified diff viewer (the app associated with *.diff or *.patch files).
	 * If no app is associated with those file types, the default text editor is used.
	 */
	static BOOL StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait = FALSE, bool bAlternativeTool = false);

	/**
	 * Sets up all the default diff and merge scripts.
	 * \param force if true, overwrite all existing entries
	 * \param either "Diff", "Merge" or an empty string
	 */
	static bool SetupDiffScripts(bool force, const CString& type);

	/**
	 * Starts the external diff application
	 */
	static bool StartExtDiff(
		const CString& file1, const CString& file2,
		const CString& sName1, const CString& sName2,
		const CString& originalFile1, const CString& originalFile2,
		const CGitHash& hash1, const CGitHash& hash2, const DiffFlags& flags, int jumpToLine = 0);

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
	 * Returns font name which is used for log messages, etc.
	 */
	static CString GetLogFontName();

	/**
	 * Returns font size which is used for log messages, etc.
	 */
	static DWORD GetLogFontSize();

	/**
	 * Create a font which can is used for log messages, etc
	 */
	static void CreateFontForLogs(CFont& fontToCreate);

	/**
	* Launch the external blame viewer
	*/
	static bool LaunchTortoiseBlame(
		const CString& sBlameFile, const CString& Rev, const CString& sParams = CString());

	/**
	* Launch alternative editor
	*/
	static bool LaunchAlternativeEditor(const CString& filename, bool uac = false);

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
	* implements URL searching with the same logic as CSciEdit::StyleURLs
	*/
	static std::vector<CHARRANGE> FindURLMatches(const CString& msg);
	static BOOL StyleURLs(const CString& msg, CWnd* pWnd);

	/**
	 * Replacement for GitDiff::ShowUnifiedDiff(), but started as a separate process.
	 */
	static bool StartShowUnifiedDiff(HWND hWnd, const CTGitPath& url1, const CString& rev1,
												const CTGitPath& url2, const CString& rev2,

												//const GitRev& peg = GitRev(), const GitRev& headpeg = GitRev(),
												bool bAlternateDiff = false,
												bool bIgnoreAncestry = false,
												bool blame  = false,
												bool bMerge = false,
												bool bCompact = false,
												bool bNoPrefix = false);

	static bool Export(HWND hWnd, const CString* BashHash = nullptr, const CTGitPath* orgPath = nullptr);
	static bool UpdateBranchDescription(const CString& branch, CString description);
	static bool CreateBranchTag(HWND hWnd, bool isTag = true, const CString* ref = nullptr, bool switchNewBranch = false, LPCTSTR name = nullptr);
	static bool Switch(HWND hWnd, const CString& initialRefName = CString());
	static bool PerformSwitch(HWND hWnd, const CString& ref, bool bForce = false, const CString& sNewBranch = CString(), bool bBranchOverride = false, BOOL bTrack = 2, bool bMerge = false);

	static bool IgnoreFile(HWND hWnd, const CTGitPathList& filelist, bool IsMask);
	static bool GitReset(HWND hWnd, const CString& ref, int type = 1);
	static bool ConflictEdit(HWND hWnd, CTGitPath& file, bool bAlternativeTool = false, bool revertTheirMy = false, HWND resolveMsgHwnd = nullptr);
	static void GetConflictTitles(CString* baseText, CString& mineText, CString& theirsText, bool rebaseActive);

	static CString GetMergeTempFile(const CString& str, const CTGitPath& merge);
	static bool	StashSave(HWND hWnd, const CString& msg = CString(), bool showPull = false, bool pullShowPush = false, bool showMerge = false, const CString& mergeRev = CString());
	static bool StashApply(HWND hWnd, CString ref, bool showChanges = true);
	/** Execute "stash pop"
	 * showChanges
	 *              0: only show info on error (and allow to diff on error)
	 *              1: allow to open diff dialog on error or success
	 *              2: only show error or success message
	 */
	static bool	StashPop(HWND hWnd, int showChanges = 1);

	static bool IsSSHPutty();

	static bool LaunchRemoteSetting();

	static bool LaunchPAgent(HWND hWnd, const CString* keyfile = nullptr, const CString* pRemote = nullptr);

	static bool ShellOpen(const CString& file, HWND hwnd = nullptr);
	static bool ShowOpenWithDialog(const CString& file, HWND hwnd = nullptr);

	static CString GetClipboardLink(const CString& skipGitPrefix = L"", int paramsCount = 0);
	static CString ChooseRepository(HWND hWnd, const CString* path);

	static bool SendPatchMail(HWND hWnd, CTGitPathList& pathlist);
	static bool SendPatchMail(HWND hWnd, const CString& cmd, const CString& formatpatchoutput);

	static int  SaveCommitUnicodeFile(const CString& filename, CString& mesage);
	static bool MessageContainsConflictHints(HWND hWnd, const CString& message);

	static int  GetLogOutputEncode(CGit *pGit=&g_Git);

	static bool Pull(HWND hWnd, bool showPush = false, bool showStashPop = false);
	// rebase = 1: ask user what to do, rebase = 2: run autorebase
	static bool RebaseAfterFetch(HWND hWnd, const CString& upstream = L"", int rebase = 0, bool preserveMerges = false);
	static bool Fetch(HWND hWnd, const CString& remoteName = L"", bool allRemotes = false);
	static bool DoPush(HWND hWnd, bool autoloadKey, bool tags, bool allRemotes, bool allBranches, bool force, bool forceWithLease, const CString& localBranch, const CString& remote, const CString& remoteBranch, bool setUpstream, int recurseSubmodules);
	static bool Push(HWND hWnd, const CString& selectLocalBranch = CString());
	static bool RequestPull(HWND hWnd, const CString& endrevision = L"", const CString& repositoryUrl = L"");

	static void RemoveTrailSlash(CString &path);

	static bool CheckUserData(HWND hWnd);

	static BOOL Commit(HWND hWnd, const CString& bugid, BOOL bWholeProject, CString& sLogMsg,
					CTGitPathList &pathList,
					bool bSelectFilesForCommit);

	static BOOL SVNDCommit(HWND hWnd);
	static BOOL Merge(HWND hWnd, const CString* commit = nullptr, bool showStashPop = false);
	static BOOL MergeAbort(HWND hWnd);
	static void RemoveTempMergeFile(const CTGitPath& path);
	static void EditNote(HWND hWnd, GitRevLoglist* rev, ProjectProperties* projectProperties);
	static int GetMsysgitVersion(HWND hWnd);
	static bool IsGitVersionNewerOrEqual(HWND hWnd, unsigned __int8 major, unsigned __int8 minor, unsigned __int8 patchlevel = 0, unsigned __int8 build = 0);
	static void MarkWindowAsUnpinnable(HWND hWnd);

	static bool BisectStart(HWND hWnd, const CString& lastGood, const CString& firstBad);
	static bool BisectOperation(HWND hWnd, const CString& op, const CString& ref = L"");

	static int	Git2GetUserPassword(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, void *payload);

	static int Git2CertificateCheck(git_cert *cert, int valid, const char* host, void *payload);

	static int ExploreTo(HWND hwnd, CString path);

	static bool DeleteRef(CWnd* parent, const CString& ref);

	static void SetupBareRepoIcon(const CString& path);

	enum resolve_with {
		RESOLVE_WITH_CURRENT,
		RESOLVE_WITH_MINE,
		RESOLVE_WITH_THEIRS,
	};

	static int ResolveConflict(HWND hWnd, CTGitPath& path, resolve_with resolveWith);

	static bool IsTGitRebaseActive(HWND hWnd);

private:
	static CString PickDiffTool(const CTGitPath& file1, const CTGitPath& file2);

	static bool OpenIgnoreFile(HWND hWnd, CIgnoreFile& file, const CString& filename);

	static void DescribeConflictFile(bool mode, bool base,CString &descript);
};
