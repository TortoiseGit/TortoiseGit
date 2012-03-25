// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2006-2008 - TortoiseSVN
// Copyright (C) 2012 - Sven Strickroth <email@cs-ware.de>

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
#include "FileTextLines.h"


#define PATCHSTATE_REMOVED	0
#define PATCHSTATE_ADDED	1
#define PATCHSTATE_CONTEXT	2

/**
 * \ingroup TortoiseMerge
 *
 * Handles unified diff files, parses them and also is able to
 * apply those diff files.
 */
class CPatch
{
public:
	CPatch(void);
	~CPatch(void);

	BOOL		OpenUnifiedDiffFile(const CString& filename);
	int			PatchFile(const int nIndex, const CString& sPath, const CString& sSavePath = _T(""), const CString& sBaseFile = _T(""), const bool force = false);
	int			GetNumberOfFiles() const  {return m_arFileDiffs.GetCount();}
	CString		GetFilename(int nIndex);
	CString		GetRevision(int nIndex);
	CString		GetFilename2(int nIndex);
	CString		GetRevision2(int nIndex);
	CString		GetFullPath(const CString& sPath, int nIndex, int fileno = 0);
	CString		GetErrorMessage() const  {return m_sErrorMessage;}
	CString		CheckPatchPath(const CString& path);

	/**
	 * Returns TRUE if stripping prefixes from the paths in the patch file
	 * allows the patch file to being applied. The variable m_nStrip is then set appropriately.
	 * Returns FALSE if stripping prefixes doesn't help. The variable m_nStrip is set to 0.
	 */
	BOOL		StripPrefixes(const CString& path);
protected:
	void		FreeMemory();
	BOOL		HasExpandedKeyWords(const CString& line);
	int			CountMatches(const CString& path);
	int			CountDirMatches(const CString& path);
	CString		RemoveUnicodeBOM(const CString& str);

	BOOL		ParserGitPatch(CFileTextLines &PatchLines,int nIndex);

	/**
	 * Strips the filename by removing m_nStrip prefixes.
	 */
	CString		Strip(const CString& filename);
	struct Chunk
	{
		LONG					lRemoveStart;
		LONG					lRemoveLength;
		LONG					lAddStart;
		LONG					lAddLength;
		CStdCStringArray		arLines;
		CStdDWORDArray			arLinesStates;
		std::vector<EOL>		arEOLs;
	};

	struct Chunks
	{
		CString					sFilePath;
		CString					sRevision;
		CString					sFilePath2;
		CString					sRevision2;
		CStdArray<Chunk*>		chunks;
	};

	CStdArray<Chunks*>			m_arFileDiffs;
	CString						m_sErrorMessage;
	CFileTextLines::UnicodeType m_UnicodeType;

	/**
	 * Defines how many prefixes are removed from the paths in the
	 * patch file. This allows applying patches which contain absolute
	 * paths or a prefix which differs in the patch and the working copy.
	 * Example: A filename like "/home/ts/my-working-copy/dir/file.txt"
	 * stripped by 4 prefixes is interpreted as "dir/file.txt"
	 */
	int							m_nStrip;
public:
	bool						m_IsGitPatch;
};
