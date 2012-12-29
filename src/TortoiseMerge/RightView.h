// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2006-2008, 2011 - TortoiseSVN

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
#include "BaseView.h"

/**
 * \ingroup TortoiseMerge
 * The right view in TortoiseMerge. This is the one which is usually modified and
 * saved.
 */
class CRightView : public CBaseView
{
	DECLARE_DYNCREATE(CRightView)
public:
	CRightView(void);
	~CRightView(void);

	void	UseBothLeftFirst();
	void	UseBothRightFirst();
	void	UseLeftBlock(); ///< Use Block from Left
	void	UseLeftFile(); ///< Use File from Left

protected:
	void	AddContextItems(CIconMenu& popup, DiffStates state);

	void	UseBlock(int nStart, int nEnd);
};
