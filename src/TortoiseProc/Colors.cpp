// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN
// Copyright (C) 2009-2013, 2016-2019 - TortoiseGit

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
#include "stdafx.h"
#include "Colors.h"

CColors::COLOR_DATA CColors::m_ColorArray[]=
{
	{ Cmd, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Cmd", RGB(100, 100, 100)) },
	{ Conflict, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Conflict", RGB(255, 0, 0)) },
	{ Modified, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Modified", RGB(0, 50, 160)) },
	{ Merged, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Merged", RGB(0, 100, 0)) },
	{ Deleted, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Deleted", RGB(100, 0, 0)) },
	{ Added, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Added", RGB(100, 0, 100)) },
	{ LastCommit, CRegDWORD(L"Software\\TortoiseGit\\Colors\\LastCommit", RGB(100, 100, 100)) },
	{ NoteNode, CRegDWORD(L"Software\\TortoiseGit\\Colors\\NoteNode", RGB(160, 160, 0)) },
	{ Renamed, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Renamed", RGB(0, 0, 255)) },
	{ LastCommitNode, CRegDWORD(L"Software\\TortoiseGit\\Colors\\LastCommitNode", RGB(200, 200, 200)) },
	{ PropertyChanged, CRegDWORD(L"Software\\TortoiseGit\\Colors\\PropertyChanged", RGB(0, 50, 160)) },
	{ CurrentBranch, CRegDWORD(L"Software\\TortoiseGit\\Colors\\CurrentBranch", RGB(200, 0, 0)) },
	{ LocalBranch, CRegDWORD(L"Software\\TortoiseGit\\Colors\\LocalBranch", RGB(0, 195, 0)) },
	{ RemoteBranch, CRegDWORD(L"Software\\TortoiseGit\\Colors\\RemoteBranch", RGB(255, 221, 170)) },
	{ Tag, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Tag", RGB(255, 255, 0)) },
	{ Stash, CRegDWORD(L"Software\\TortoiseGit\\Colors\\Stash", RGB(128, 128, 128)) },
	{ BranchLine1, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine1", RGB(0, 0, 0)) },
	{ BranchLine2, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine2", RGB(0xFF, 0, 0)) },
	{ BranchLine3, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine3", RGB(0, 0xFF, 0)) },
	{ BranchLine4, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine4", RGB(0, 0, 0xFF)) },
	{ BranchLine5, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine5", RGB(128, 128, 128)) },
	{ BranchLine6, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine6", RGB(128, 128, 0)) },
	{ BranchLine7, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine7", RGB(0, 128, 128)) },
	{ BranchLine8, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BranchLine8", RGB(128, 0, 128)) },
	{ BisectGood, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BisectGood", RGB(0, 100, 200)) },
	{ BisectBad, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BisectBad", RGB(255, 0, 0)) },
	{ BisectSkip, CRegDWORD(L"Software\\TortoiseGit\\Colors\\BisectBad", RGB(192, 192, 192)) },
	{ OtherRef, CRegDWORD(L"Software\\TortoiseGit\\Colors\\OtherRef", RGB(224, 224, 224)) },
	{ FilterMatch, CRegDWORD(L"Software\\TortoiseGit\\Colors\\FilterMatch", RGB(200, 0, 0)) },
};

CColors::CColors(void)
{
}

CColors::~CColors(void)
{
}

COLORREF CColors::GetColor(Colors col, bool bDefault /*=true*/)
{
	if (col == COLOR_END)
		return RGB(0, 0, 0);
	if (bDefault)
		return m_ColorArray[col].RegKey.defaultValue();
	else
		return m_ColorArray[col].RegKey;
}

void CColors::SetColor(Colors col, COLORREF cr)
{
	if (col == COLOR_END)
	{
		ASSERT(FALSE);
		return;
	}
	m_ColorArray[col].RegKey = cr;
}


COLORREF CColors::MixColors(COLORREF baseColor, COLORREF newColor, unsigned char mixFactor)
{
	short colRed;
	short colGreen;
	short colBlue;
	colRed   = static_cast<short>(static_cast<float>( baseColor & 0x000000FF)        - static_cast<float>( newColor & 0x000000FF)       ) * mixFactor / 0xFF; // red
	colGreen = static_cast<short>(static_cast<float>((baseColor & 0x0000FF00) >>  8) - static_cast<float>((newColor & 0x0000FF00) >>  8)) * mixFactor / 0xFF; // green
	colBlue  = static_cast<short>(static_cast<float>((baseColor & 0x00FF0000) >> 16) - static_cast<float>((newColor & 0x00FF0000) >> 16)) * mixFactor / 0xFF; // blue

	colRed   = ( baseColor&0x000000FF)		-colRed;
	colGreen = ((baseColor&0x0000FF00)>>8)	-colGreen;
	colBlue  = ((baseColor&0x00FF0000)>>16) -colBlue;
	baseColor = static_cast<int>(colRed) | (static_cast<int>(colGreen) << 8) | (static_cast<int>(colBlue) << 16);
	return baseColor;
}

COLORREF CColors::Lighten(COLORREF baseColor, unsigned char amount)
{
	return MixColors(baseColor, RGB(255,255,255), amount);
}

COLORREF CColors::Darken(COLORREF baseColor, unsigned char amount)
{
	return MixColors(baseColor, RGB(0,0,0), amount);
}

