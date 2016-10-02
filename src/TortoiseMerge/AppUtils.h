// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2010, 2013-2014 - TortoiseSVN

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
#include "CommonAppUtils.h"

class CSysProgressDlg;

/**
 * \ingroup TortoiseMerge
 *
 * Helper functions
 */
class CAppUtils : public CCommonAppUtils
{
public:
	CAppUtils() = delete;

	/**
	 * Starts an external program to get a file with a specific revision.
	 * \param sPath path to the file for which a specific revision is fetched
	 * \param sVersion the revision to get
	 * \param sSavePath the path to where the file version shall be saved
	 * \param progDlg
	 * \param hWnd the window handle of the calling app
	 * \return TRUE if successful
	 */
	static BOOL GetVersionedFile(CString sPath, CString sVersion, CString sSavePath, CSysProgressDlg* progDlg, HWND hWnd = nullptr);

	/**
	 * Creates a unified diff from two files
	 */
	static bool CreateUnifiedDiff(const CString& orig, const CString& modified, const CString& output, int contextsize, bool bShowError);

	static bool HasClipboardFormat(UINT format);
	static COLORREF IntenseColor(long scale, COLORREF col);
};
