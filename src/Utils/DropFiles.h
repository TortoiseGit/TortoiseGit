// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2006-2007 - TortoiseSVN

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

#ifndef __DROPFILES_H__
#define __DROPFILES_H__

#include <shlobj.h>
#include <afxcoll.h>

/**
 * \ingroup Utils
 * Use this class to create the DROPFILES structure which is needed to
 * support drag and drop of file names to other applications.
 * Based on an example by Thomas Blenkers.
 */
class CDropFiles
{
public:
	CDropFiles();
	~CDropFiles();

	/**
	 * Add a file with an absolute file name. This file will later be
	 * included the DROPFILES structure.
	 */
	void AddFile(const CString &sFile);

	/**
	 * Returns the number of files which have been added
	 */
	INT_PTR GetCount();

	/**
	 * Call this method when dragging begins. It will fill
	 * the DROPFILES structure with the files previously
	 * added with AddFile(...)
	 */
	void CreateStructure();

protected:
	/**
	 * CreateBuffer must be called once when all files have been added
	 */
	void CreateBuffer();

	/**
	 * Returns a pointer to the buffer containing the DROPFILES
	 * structure
	 */
	void* GetBuffer() const;

	/**
	 * Returns the size of the buffer in bytes
	 */
	int GetBufferSize() const;

protected:
	CStringArray m_arFiles;
	
	char* m_pBuffer;
	int	 m_nBufferSize;
};

#endif
