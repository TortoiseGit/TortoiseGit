// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2014 - TortoiseGit
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
#include "GitRev.h"
#include "CommonAppUtils.h"

class CTGitPath;
struct git_cred;
struct git_transfer_progress;
class CIgnoreFile;

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

	CAppUtils(void);
	~CAppUtils(void);

	/**
	 * Launches the external merge program if there is one.
	 * \return TRUE if the program could be started
	 */
	static BOOL StartExtMerge(
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
	static BOOL StartUnifiedDiffViewer(const CString& patchfile, const CString& title, BOOL bWait = FALSE);

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
		const git_revnum_t& hash1, const git_revnum_t& hash2, const DiffFlags& flags, int jumpToLine = 0);

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
	 * Replacement for GitDiff::ShowUnifiedDiff(), but started as a separate process.
	 */
	static bool StartShowUnifiedDiff(HWND hWnd, const CTGitPath& url1,  const git_revnum_t& rev1,
												const CTGitPath & url2, const git_revnum_t& rev2,

												//const GitRev& peg = GitRev(), const GitRev& headpeg = GitRev(),
												bool bAlternateDiff = false,
												bool bIgnoreAncestry = false,
												bool blame  = false,
												bool bMerge = false,
												bool bCompact = false);

	static bool Export(const CString* BashHash = nullptr, const CTGitPath* orgPath = nullptr);
	static bool CreateBranchTag(bool IsTag = TRUE, const CString* CommitHash = nullptr, bool switch_new_brach = false);
	static bool Switch(const CString& initialRefName = CString());
	static bool PerformSwitch(const CString& ref, bool bForce = false, const CString& sNewBranch = CString(), bool bBranchOverride = false, BOOL bTrack = 2, bool bMerge = false);

	static bool IgnoreFile(const CTGitPathList& filelist, bool IsMask);
	static bool GitReset(const CString* CommitHash, int type = 1);
	static bool ConflictEdit(const CTGitPath& file, bool bAlternativeTool = false, bool revertTheirMy = false, HWND resolveMsgHwnd = nullptr);

	static CString GetMergeTempFile(const CString& str, const CTGitPath& merge);
	static bool	StashSave(const CString& msg = CString());
	static bool StashApply(CString ref, bool showChanges = true);
	static bool	StashPop(bool showChanges = true);

	static bool IsSSHPutty();

	static bool LaunchRemoteSetting();

	static bool LaunchPAgent(const CString* keyfile = nullptr, const CString* pRemote = nullptr);

	static CString GetClipboardLink(const CString &skipGitPrefix = _T(""), int paramsCount = 0);
	static CString ChooseRepository(const CString* path);

	static bool SendPatchMail(CTGitPathList& pathlist, bool bIsMainWnd = false);
	static bool SendPatchMail(const CString& cmd, const CString& formatpatchoutput, bool bIsMainWnd = false);

	static int  SaveCommitUnicodeFile(const CString& filename, CString& mesage);

	static int  GetLogOutputEncode(CGit *pGit=&g_Git);

	static bool Pull(bool showPush = false);
	static bool RebaseAfterFetch(const CString& upstream = _T(""));
	static bool Fetch(const CString& remoteName = _T(""), bool allRemotes = false);
	static bool Push(const CString& selectLocalBranch = CString());
	static bool RequestPull(const CString& endrevision = _T(""), const CString& repositoryUrl = _T(""), bool bIsMainWnd = false);

	static bool CreateMultipleDirectory(const CString &dir);

	static void RemoveTrailSlash(CString &path);

	static bool CheckUserData();

	static BOOL Commit(const CString& bugid, BOOL bWholeProject, CString &sLogMsg,
					CTGitPathList &pathList,
					CTGitPathList &selectedList,
					bool bSelectFilesForCommit);

	static BOOL SVNDCommit();
	static BOOL Merge(const CString* commit = nullptr);
	static BOOL MergeAbort();
	static void RemoveTempMergeFile(const CTGitPath& path);
	static void EditNote(GitRev *hash);
	static int GetMsysgitVersion();
	static void MarkWindowAsUnpinnable(HWND hWnd);

	static bool BisectStart(const CString& lastGood, const CString& firstBad, bool bIsMainWnd = false);

	static int	Git2GetUserPassword(git_cred **out, const char *url, const char *username_from_url, unsigned int allowed_types, void *payload);

	static int Git2CertificateCheck(git_cert *cert, int valid, const char* host, void *payload);

	static void ExploreTo(HWND hwnd, CString path);

	enum resolve_with {
		RESOLVE_WITH_CURRENT,
		RESOLVE_WITH_MINE,
		RESOLVE_WITH_THEIRS,
	};

	static int ResolveConflict(CTGitPath& path, resolve_with resolveWith);

private:
	static CString PickDiffTool(const CTGitPath& file1, const CTGitPath& file2);

	static bool OpenIgnoreFile(CIgnoreFile &file, const CString& filename);

	static void DescribeConflictFile(bool mode, bool base,CString &descript);
};
