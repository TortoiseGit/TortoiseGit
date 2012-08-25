// TortoiseMerge - a Diff/Patch program

// Copyright (C) 2007 - TortoiseSVN

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
#include "ViewData.h"

CViewData::CViewData(void)
{
}

CViewData::~CViewData(void)
{
}

void CViewData::AddData(const CString& sLine, DiffStates state, int linenumber, EOL ending)
{
	viewdata data;
	data.sLine = sLine;
	data.state = state;
	data.linenumber = linenumber;
	data.ending = ending;
	return AddData(data);
}

void CViewData::AddData(const viewdata& data)
{
	return m_data.push_back(data);
}

void CViewData::InsertData(int index, const CString& sLine, DiffStates state, int linenumber, EOL ending)
{
	viewdata data;
	data.sLine = sLine;
	data.state = state;
	data.linenumber = linenumber;
	data.ending = ending;
	return InsertData(index, data);
}

void CViewData::InsertData(int index, const viewdata& data)
{
	m_data.insert(m_data.begin()+index, data);
}

int CViewData::FindLineNumber(int number)
{
	for(size_t i = 0; i < m_data.size(); ++i)
		if (m_data[i].linenumber >= number)
			return (int)i;
	return -1;
}
