// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2011-2019 - TortoiseGit
// Copyright (C) 2006-2008, 2015 - TortoiseSVN

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
#include "registry.h"
#include "TGitPath.h"
#include "ProjectProperties.h"

/**
 * \ingroup TortoiseProc
 * enumeration of all client hook types
 */
typedef enum hooktype
{
	unknown_hook,
	start_commit_hook,
	pre_commit_hook,
	post_commit_hook,
	issue_tracker_hook,
	pre_push_hook,
	post_push_hook,
	pre_rebase_hook,
} hooktype;

/**
 * \ingroup TortoiseProc
 * helper class, used as the key to the std::map we store
 * the data for the client hook scripts in.
 */
class hookkey
{
public:
	hooktype		htype;
	CTGitPath		path;
	bool			local;

	bool operator < (const hookkey& hk) const
	{
		if (htype == hk.htype)
		{
			if (local != hk.local)
				return local < hk.local;

			return (path < hk.path);
		}
		else
			return htype < hk.htype;
	}
};

/**
 * \ingroup TortoiseProc
 * helper struct, used as the value to the std::map we
 * store the data for the client hook scripts in.
 */
typedef struct hookcmd
{
	CString			commandline;
	bool			bWait;
	bool			bShow;
	bool			bEnabled;
	bool			bLocal;
	bool			bApproved; ///< user explicitly approved
	bool			bStored; ///< use decision is stored in reg
	CString			sRegKey;
} hookcmd;

typedef std::map<hookkey, hookcmd>::iterator hookiterator;
typedef std::map<hookkey, hookcmd>::const_iterator const_hookiterator;

/**
 * \ingroup TortoiseProc
 * Singleton class which deals with the client hook scripts.
 */
class CHooks : public std::map<hookkey, hookcmd>
{
private:
	CHooks();
	~CHooks();
	// prevent cloning
	CHooks(const CHooks&) = delete;
	CHooks& operator=(const CHooks&) = delete;

	static void AddPathParam(CString& sCmd, const CTGitPathList& pathList);
	static void AddCWDParam(CString& sCmd, const CString& workingTree);
	static void AddErrorParam(CString& sCmd, const CString& error);
	static void AddParam(CString& sCmd, const CString& param);
	static CTGitPath AddMessageFileParam(CString& sCmd, const CString& message);
public:
	/// Create the singleton. Call this at the start of the program.
	static bool			Create();
	/// Returns the singleton instance
	static CHooks&		Instance();
	/// Destroys the singleton object. Call this at the end of the program.
	static void			Destroy();

public:
	/// Saves the hook script information to the registry.
	bool				Save();
	/**
	 * Removes the hook script identified by \c key. To make the change persistent
	 * call Save().
	 */
	bool				Remove(const hookkey &key);
	/**
	 * Adds a new hook script. To make the change persistent, call Save().
	 */
	void				Add(hooktype ht, const CTGitPath& Path, LPCTSTR szCmd,
							bool bWait, bool bShow, bool bEnabled, bool bLocal);

	/**
	* Toggles the hook script identified by \c key. Returns whether the status has changed.
	* To make the change persistent call Save().
	*/
	bool				SetEnabled(const hookkey& key, bool bEnabled);

	/// returns the string representation of the hook type.
	static CString		GetHookTypeString(hooktype t);
	/// returns the hooktype from a string representation of the same.
	static hooktype		GetHookType(const CString& s);

	/// Add hook script data from project properties
	void				SetProjectProperties(const CTGitPath& Path, const ProjectProperties& pp);

	/**
	 * Executes the Start-Commit-Hook that first matches the path in
	 * \c workingTree.
	 * \param workingTree working tree root directory
	 * \param pathList a list of paths to look for the hook scripts
	 * \param message a commit message
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the commit is about to be done on.
	 * The string %MESSAGEFILE% is replaced with path to temporary file containing
	 * \c message. If the script finishes successfully, contents of this file
	 * is read back into \c message parameter.
	 */
	bool				StartCommit(HWND hWnd, const CString& workingTree, const CTGitPathList& pathList, CString& message,
									DWORD& exitcode, CString& error);
	/**
	 * Executes the Pre-Commit-Hook that first matches the path in
	 * \c workingTree.
	 * \param workingTree working tree root directory
	 * \param pathList a list of paths to look for the hook scripts
	 * \param message the commit message
	 * \param exitcode on return, contains the exit code of the hook script
	 * \param error the data the hook script outputs to stderr
	 * \remark the string "%PATHS% in the command line of the hook script is
	 * replaced with the path to a temporary file which contains a list of files
	 * in \c pathList, separated by newlines. The hook script can parse this
	 * file to get all the paths the update is about to be done on.
	 * If the script finishes successfully, contents of this file is read back
	 * into \c message parameter.
	 */
	bool				PreCommit(HWND hWnd, const CString& workingTree, const CTGitPathList& pathList,
									CString& message, DWORD& exitcode,
									CString& error);
	/**
	 * Executes the Post-Commit-Hook that first matches the path in
	 * \c workingTree.
	 * \param workingTree working tree root directory
	 * \param amend commit was amend
	 */
	bool	PostCommit(HWND hWnd, const CString& workingTree, bool amend, DWORD& exitcode, CString& error);

	bool	PrePush(HWND hWnd, const CString& workingTree, DWORD& exitcode, CString& error);
	bool	PostPush(HWND hWnd, const CString& workingTree, DWORD& exitcode, CString& error);

	bool	PreRebase(HWND hWnd, const CString& workingTree, const CString& upstream, const CString& rebasedBranch, DWORD& exitcode, CString& error);

	bool	IsHookPresent(hooktype t, const CString& workingTree);

private:
	/**
	 * Starts a new process, specified in \c cmd.
	 * \param error the data the process writes to stderr
	 * \param bWait if true, then this method waits until the created process has finished. If false, then the return
	 * value will always be 0 and \c error will be an empty string.
	 * \param bShow set to true if the process should be started visible.
	 * \return the exit code of the process if \c bWait is true, zero otherwise.
	 */
	static DWORD		RunScript(CString cmd, LPCTSTR currentDir, CString& error, bool bWait, bool bShow);
	/**
	 * Find the hook script information for the hook type \c t which matches the
	 * path in \c workingTree.
	 */
	hookiterator	FindItem(hooktype t, const CString& workingTree);

	static void ParseHookString(CString strhooks, bool bLocal);

	/**
	 * Checks whether the hook script has been validated already and
	 * if not, asks the user to validate it.
	 */
	bool ApproveHook(HWND hWnd, hookiterator it, DWORD& exitcode);

	static CHooks *		m_pInstance;
	static CTGitPath	m_RootPath;
};
