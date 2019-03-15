// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2012, 2019 - TortoiseGit
// Copyright (C) 2010-2012, 2015 - TortoiseSVN

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
#include "TempFile.h"
#include "SysProgressDlg.h"

#include "Patch.h"

class GitPatch
{
public:
	GitPatch();
	~GitPatch();

	/**
	 * Does a dry run of the patching, fills in all the arrays.
	 * Call this function first.
	 * The progress dialog is used to show progress info if the initialization takes a long time.
	 * \return the number of files affected by the patchfile, 0 in case of an error
	 */
	int						Init(const CString& patchfile, const CString& targetpath, CSysProgressDlg *pPprogDlg);

	/**
	 * Sets the target path. Use this after getting a new path from CheckPatchPath()
	 */
	void					SetTargetPath(const CString& targetpath) { m_targetpath = targetpath; m_targetpath.Replace('\\', '/'); }
	CString					GetTargetPath() { return m_targetpath; }

	/**
	 * Finds the best path to apply the patch file. Starting from the targetpath
	 * specified in Init() first upwards, then downwards.
	 * \remark this function shows a progress dialog and also shows a dialog asking
	 * the user to accept a possible better target path.
	 * \return the best path to apply the patch to.
	 */
	CString					CheckPatchPath(const CString& path);

	/**
	 * Applies the patch for the given \c path, including property changes if necessary.
	 */
	bool					PatchPath(const CString& path);

	/**
	 * Returns the paths of the patch result for the \c sPath.
	 * The patch is applied in the Init() method.
	 * \return the number of failed hunks, 0 if everything was applied successfully, -1 on error
	 */
	int						GetPatchResult(const CString& sPath, CString& sSavePath, CString& sRejectPath, CString &sBasePath) const;

	/**
	 * returns the number of files that are affected by the patchfile.
	 */
	int						GetNumberOfFiles() const { return static_cast<int>(m_filePaths.size()); }

	/**
	 * Returns the path of the affected file
	 */
	CString					GetFilePath(int index) const { return m_filePaths[index].path; }

	/**
	 * Returns the number of failed hunks for the affected file
	 */
	int						GetFailedHunks(int index) const { return m_filePaths[index].rejects; }

	bool					GetHasConflict(int index) const { return !m_filePaths[index].basePath.IsEmpty(); }

	/**
	 * Returns true if there are content modifications for the path
	 */
	bool					GetContentMods(int index) const { return m_filePaths[index].content; }

	/**
	 * Returns true if there are property modifications for the path
	 */
	bool					GetPropMods(int index) const { return m_filePaths[index].props; }

	/**
	 * Returns the path of the affected file, stripped by m_nStrip.
	 */
	CString					GetStrippedPath(int nIndex) const;

	/**
	 * Returns a string containing the last error message.
	 */
	CString					GetErrorMessage() const { return m_errorStr; }

	/**
	 * Removes the file from version control
	 */
	bool					RemoveFile(const CString& path);

	CString					GetPatchRejects(int nIndex) const;

private:
	int						CountMatches(const CString& path) const;
	int						CountDirMatches(const CString& path) const;
	/**
	 * Strips the filename by removing m_nStrip prefixes.
	 */
	CString					Strip(const CString& filename) const;

	struct PathRejects
	{
		CString		path;
		int			rejects;
		CString		basePath; // empty if patch applies cleany to local file; path to tempfile of base-version if patch only applies cleanly to base-version
		CString		resultPath;
		CString		rejectsPath;
		bool		content;
		bool		props;
	};
	std::vector<PathRejects> m_filePaths;
	int						m_nStrip;
	bool					m_bSuccessfullyPatched;
	int						m_nRejected;
	CString					m_patchfile;
	CString					m_targetpath;
	CString					m_testPath;
	CString					m_filetopatch;
	CString					m_errorStr;
	CSysProgressDlg *		m_pProgDlg;

	CPatch					m_patch;
	bool					ApplyPatches();
	bool					PatchFile(int nIndex, CString &datapath);
};
