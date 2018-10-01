// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN
// Copyright (C) 2009-2013, 2016-2018 - TortoiseGit

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

/**
 * \ingroup TortoiseProc
 * Handles the UI colors used in TortoiseSVN
 */
class CColors
{
public:
	CColors(void);
	~CColors(void);

	enum Colors
	{
		Cmd,
		Conflict,
		Modified,
		Merged,
		Deleted,
		Added,
		LastCommit,
		NoteNode,
		Renamed,
		LastCommitNode,
		PropertyChanged,
		CurrentBranch,
		LocalBranch,
		RemoteBranch,
		Tag,
		Stash,
		BranchLine1,
		BranchLine2,
		BranchLine3,
		BranchLine4,
		BranchLine5,
		BranchLine6,
		BranchLine7,
		BranchLine8,
		BisectGood,
		BisectBad,
		BisectSkip,
		OtherRef,
		FilterMatch,
		COLOR_END=-1
	};

	COLORREF GetColor(Colors col, bool bDefault = false);
	void SetColor(Colors col, COLORREF cr);

	//mixFactor: 0 -> baseColor, 255 -> newColor
	static COLORREF MixColors(COLORREF baseColor, COLORREF newColor, unsigned char mixFactor);

	static COLORREF Lighten(COLORREF baseColor, unsigned char amount = 100);
	static COLORREF Darken(COLORREF baseColor, unsigned char amount = 100);

	struct COLOR_DATA
	{
		Colors		Color;
		CRegDWORD	RegKey;
	};

private:

	static COLOR_DATA m_ColorArray[];
};
