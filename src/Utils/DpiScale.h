// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2017 - TortoiseSVN

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

class CDpiScale
{
public:
	CDpiScale(HDC hdc)
	{
		m_cxScale = GetDeviceCaps(hdc, LOGPIXELSX);
		m_cyScale = GetDeviceCaps(hdc, LOGPIXELSY);
	}

	int ScaleX(int x) const
	{
		return MulDiv(x, m_cxScale, 96);
	}

	int ScaleY(int y) const
	{
		return MulDiv(y, m_cyScale, 96);
	}

private:
	int m_cxScale;
	int m_cyScale;
};
