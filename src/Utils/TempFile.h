// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008, 2012-2013, 2015-2017 - TortoiseGit
// Copyright (C) 2003-2006,2008,2015 - TortoiseSVN

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
#include "GitHash.h"

/**
* \ingroup Utils
* This singleton class handles temporary files.
* All temp files are deleted at the end of a run of SVN operations
*/
class CTempFiles
{
private:
	CTempFiles(void);
	~CTempFiles(void);
	// prevent cloning
	CTempFiles(const CTempFiles&) = delete;
	CTempFiles& operator=(const CTempFiles&) = delete;
public:
	static CTempFiles& Instance();

	/**
	 * Returns a path to a temporary file.
	 * \param bRemoveAtEnd if true, the temp file is removed when this object
	 *                     goes out of scope.
	 * \param path         if set, the temp file will have the same file extension
	 *                     as this path.
	 */
	CTGitPath		GetTempFilePath(bool bRemoveAtEnd, const CTGitPath& path = CTGitPath(), const CGitHash& hash = CGitHash());

private:
	CTGitPathList m_TempFileList;
};
