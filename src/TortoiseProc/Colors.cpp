// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2003-2007 - TortoiseSVN

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
#include "StdAfx.h"
#include ".\colors.h"

CColors::COLOR_DATA CColors::m_ColorArray[]=
{
	{Cmd,_T("Software\\TortoiseGit\\Colors\\Cmd"),RGB(100, 100, 100)},
	{Conflict,_T("Software\\TortoiseGit\\Colors\\Conflict"), RGB(255, 0, 0)},
	{Modified,_T("Software\\TortoiseGit\\Colors\\Modified"), RGB(0, 50, 160)},
	{Merged,_T("Software\\TortoiseGit\\Colors\\Merged"), RGB(0, 100, 0)},
	{Deleted,_T("Software\\TortoiseGit\\Colors\\Deleted"), RGB(100, 0, 0)},
	{Added,_T("Software\\TortoiseGit\\Colors\\Added"), RGB(100, 0, 100)},
	{LastCommit,_T("Software\\TortoiseGit\\Colors\\LastCommit"), RGB(100, 100, 100)},
	{DeletedNode,_T("Software\\TortoiseGit\\Colors\\DeletedNode"), RGB(255, 0, 0)},
	{AddedNode,_T("Software\\TortoiseGit\\Colors\\AddedNode"), RGB(0, 255, 0)},
	{ReplacedNode,_T("Software\\TortoiseGit\\Colors\\ReplacedNode"), RGB(0, 255, 0)},
	{RenamedNode,_T("Software\\TortoiseGit\\Colors\\RenamedNode"), RGB(0, 0, 255)},
	{LastCommitNode,_T("Software\\TortoiseGit\\Colors\\LastCommitNode"), RGB(200, 200, 200)},
	{PropertyChanged,_T("Software\\TortoiseGit\\Colors\\PropertyChanged"), RGB(0, 50, 160)},
	{CurrentBranch,_T("Software\\TortoiseGit\\Colors\\CurrentBranch"), RGB(200, 0, 0)},
	{LocalBranch,_T("Software\\TortoiseGit\\Colors\\LocalBranch"), RGB(0, 195, 0)},
	{RemoteBranch,_T("Software\\TortoiseGit\\Colors\\RemoteBranch"), RGB(255, 221, 170)},
	{Tag,_T("Software\\TortoiseGit\\Colors\\Tag"), RGB(255, 255, 0)},
	{Stash,_T("Software\\TortoiseGit\\Colors\\Stash"), RGB(128, 128, 128)},
	{BranchLine1,_T("Software\\TortoiseGit\\Colors\\BranchLine1"), RGB(0,0,0)},
	{BranchLine2,_T("Software\\TortoiseGit\\Colors\\BranchLine2"), RGB(0xFF,0,0)},
	{BranchLine3,_T("Software\\TortoiseGit\\Colors\\BranchLine3"), RGB(0,0xFF,0)},
	{BranchLine4,_T("Software\\TortoiseGit\\Colors\\BranchLine4"), RGB(0,0,0xFF)},
	{BranchLine5,_T("Software\\TortoiseGit\\Colors\\BranchLine5"), RGB(128,128,128)},
	{BranchLine6,_T("Software\\TortoiseGit\\Colors\\BranchLine6"), RGB(128,128,0)},
	{BranchLine7,_T("Software\\TortoiseGit\\Colors\\BranchLine7"), RGB(0,128,128)},
	{BranchLine8,_T("Software\\TortoiseGit\\Colors\\BranchLine8"), RGB(128,0,128)},
	{BisectGood,_T("Software\\TortoiseGit\\Colors\\BisectGood"), RGB(0,100,200)},
	{BisectBad, _T("Software\\TortoiseGit\\Colors\\BisectBad"),  RGB(255,0,0)},
	{COLOR_END,_T("Software\\TortoiseGit\\Colors\\END"),RGB(0,0,0)},

};

CColors::CColors(void)
{
}

CColors::~CColors(void)
{
}

COLORREF CColors::GetColor(Colors col, bool bDefault /*=true*/)
{
	int i=0;
	while(1)
	{
		if(m_ColorArray[i].Color == COLOR_END)
			return RGB(0,0,0);

		if(m_ColorArray[i].Color == col)
		{
			if(bDefault)
				return m_ColorArray[i].Default;
			else
			{
				CRegDWORD reg(m_ColorArray[i].RegKey,m_ColorArray[i].Default);
				return (COLORREF)(DWORD) reg;
			}
		}

		i++;
	}
}

void CColors::SetColor(Colors col, COLORREF cr)
{
	int i=0;
	while(1)
	{
		if(m_ColorArray[i].Color == COLOR_END)
			break;

		if(m_ColorArray[i].Color == col)
		{
			CRegDWORD reg(m_ColorArray[i].RegKey,m_ColorArray[i].Default);
			reg=cr;

		}
		i++;
	}
}


COLORREF CColors::MixColors(COLORREF baseColor, COLORREF newColor, unsigned char mixFactor)
{
	short colRed;
	short colGreen;
	short colBlue;
	colRed	 = (short)((float)( baseColor&0x000000FF)     -(float)( newColor&0x000000FF)     )*mixFactor/0xFF;//red
	colGreen = (short)((float)((baseColor&0x0000FF00)>>8) -(float)((newColor&0x0000FF00)>>8 ))*mixFactor/0xFF;//green
	colBlue  = (short)((float)((baseColor&0x00FF0000)>>16)-(float)((newColor&0x00FF0000)>>16))*mixFactor/0xFF;//blue

	colRed   = ( baseColor&0x000000FF)		-colRed;
	colGreen = ((baseColor&0x0000FF00)>>8)	-colGreen;
	colBlue  = ((baseColor&0x00FF0000)>>16) -colBlue;
	baseColor=(int)colRed|((int)colGreen<<8)|((int)colBlue<<16);
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

