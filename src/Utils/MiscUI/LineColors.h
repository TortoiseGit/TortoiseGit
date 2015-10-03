// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2010 - TortoiseSVN

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
#include <map>


struct linecolors_t
{
	COLORREF text;
	COLORREF background;
	COLORREF shot;
};

class LineColors : public std::map<int, linecolors_t>
{
public:
	void AddShotColor(int pos, COLORREF b)
	{
		 // make sure position exists
		 SplitBlock(pos);
		 // set value
		 (*this)[pos].shot = b;
	}

	void SetColor(int pos, COLORREF f, COLORREF b)
	{
		linecolors_t c;
		c.text = f;
		c.background = b;
		c.shot = b;
		(*this)[pos] = c;
	}

	void SetColor(int pos, const linecolors_t &c)
	{
		linecolors_t cNew = c;
		cNew.shot = c.background;
		(*this)[pos] = cNew;
	}

	void SplitBlock(int pos) /// insert colormark with same value as previous position defines
	{
		std::map<int, linecolors_t>::const_iterator it = this->upper_bound(pos);
		if (it != this->cbegin())
		{
			if ((it == this->cend()) || (it->first != pos))
			{
				SetColor(pos, (--it)->second);
			}
		}
		else if (it != this->cend())
		{
			 SetColor(pos, it->second);
		}
	}
};
