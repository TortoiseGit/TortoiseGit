// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2014 - TortoiseSVN

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

class CEditorConfigWrapper
{
public:
	bool Load(CString filename);

	template <typename T> class Nullable
	{
		T m_Value;
		bool m_bNull;

	public:
		Nullable()
			: m_bNull(true)
			, m_Value()
		{
		}

		Nullable(T value)
			: m_bNull(false)
			, m_Value(value)
		{
		}

		operator T()
		{
			return m_Value;
		}

		void operator = (T value)
		{
			m_bNull = false;
			m_Value = value;
		}

		void operator = (void *ptrnull)
		{
			if (ptrnull == nullptr)
				m_bNull = true;
		}

		bool operator == (void* ptrnull)
		{
			return ptrnull == nullptr ? m_bNull : false;
		}

		bool operator != (void* ptrnull)
		{
			return ptrnull == nullptr ? !m_bNull : false;
		}
	};

	Nullable<bool> m_bIndentStyle;
	Nullable<int> m_nIndentSize;
	Nullable<int> m_nTabWidth;
	Nullable<EOL> m_EndOfLine;
	Nullable<CFileTextLines::UnicodeType> m_Charset;
	Nullable<bool> m_bTrimTrailingWhitespace;
	Nullable<bool> m_bInsertFinalNewline;
};
