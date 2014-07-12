// TortoiseGitMerge - a Diff/Patch program

// Copyright (C) 2014 - TortoiseGit
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

#include "stdafx.h"
#include "EditorConfigWrapper.h"
#include "UnicodeUtils.h"
#include "editorconfig/editorconfig.h"

bool CEditorConfigWrapper::Load(CString filename)
{
	if (filename.IsEmpty())
		return false;
	CStringA filenameA = CUnicodeUtils::GetUTF8(filename);
	editorconfig_handle eh = editorconfig_handle_init();
	if (editorconfig_parse(filenameA, eh))
	{
		editorconfig_handle_destroy(eh);
		return false;
	}

	int count = editorconfig_handle_get_name_value_count(eh);
	if (count == 0)
	{
		editorconfig_handle_destroy(eh);
		return false;
	}
	for (int i = 0; i < count; ++i)
	{
		const char* name, *value;
		editorconfig_handle_get_name_value(eh, i, &name, &value);
		if (!strcmp(name, "indent_style"))
		{
			if (!strcmp(value, "space"))
				m_bIndentStyle = true;
			else if (!strcmp(value, "tab"))
				m_bIndentStyle = false;
			else
				m_bIndentStyle = nullptr;
		}
		else if (!strcmp(name, "indent_size"))
			m_nIndentSize = atoi(value);
		else if (!strcmp(name, "tab_width"))
			m_nTabWidth = atoi(value);
		else if (!strcmp(name, "end_of_line"))
		{
			if (!strcmp(value, "lf"))
				m_EndOfLine = EOL::EOL_LF;
			else if (!strcmp(value, "cr"))
				m_EndOfLine = EOL::EOL_CR;
			else if (!strcmp(value, "crlf"))
				m_EndOfLine = EOL::EOL_CRLF;
			else
				m_EndOfLine = nullptr;
		}
		else if (!strcmp(name, "charset"))
		{
			if (!strcmp(value, "utf-8"))
				m_Charset = CFileTextLines::UnicodeType::UTF8;
			else if (!strcmp(value, "utf-8-bom"))
				m_Charset = CFileTextLines::UnicodeType::UTF8BOM;
			else if (!strcmp(value, "utf-16be"))
				m_Charset = CFileTextLines::UnicodeType::UTF16_BE;
			else if (!strcmp(value, "utf-16le"))
				m_Charset = CFileTextLines::UnicodeType::UTF16_LE;
			else
				m_Charset = nullptr;
		}
		else if (!strcmp(name, "trim_trailing_whitespace"))
		{
			if (!strcmp(value, "true"))
				m_bTrimTrailingWhitespace = true;
			else if (!strcmp(value, "false"))
				m_bTrimTrailingWhitespace = false;
			else
				m_bTrimTrailingWhitespace = nullptr;
		}
		else if (!strcmp(name, "insert_final_newline"))
		{
			if (!strcmp(value, "true"))
				m_bInsertFinalNewline = true;
			else if (!strcmp(value, "false"))
				m_bInsertFinalNewline = false;
			else
				m_bInsertFinalNewline = nullptr;
		}
	}

	editorconfig_handle_destroy(eh);
	return true;
}
