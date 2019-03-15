// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2018 - TortoiseSVN

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
#include "EditWordBreak.h"

bool IsSchemeEnd(LPCWSTR edit_text, int current_pos, int length)
{
	return (current_pos >= 0) &&
		   ((length - current_pos) > 2) &&
		   (edit_text[current_pos] == ':') &&
		   (edit_text[current_pos + 1] == '/') &&
		   (edit_text[current_pos + 2] == '/');
}

int IsDelimiter(const LPCWSTR& edit_text, int current_pos, int length)
{
	if (isspace(edit_text[current_pos]))
	{
		// Whitespace normally breaks words, but the MSDN docs say that we must
		// not break on the CRs in a "CR, LF" or a "CR, CR, LF" sequence.  Just
		// check for an arbitrarily long sequence of CRs followed by LF and
		// report "not a delimiter" for the current CR in that case.
		while ((current_pos < (length - 1)) &&
			   (edit_text[current_pos] == 13))
		{
			if (edit_text[++current_pos] == 10)
				return false;
		}
		return true;
	}

	// Punctuation normally breaks words, but the first two characters in
	// "://" (end of scheme) should not be breaks, so that "http://" will be
	// treated as one word.
	if (ispunct(edit_text[current_pos]) &&
		!IsSchemeEnd(edit_text, current_pos - 0, length) &&
		!IsSchemeEnd(edit_text, current_pos - 1, length))
		return true;

	// Normal character, no flags.
	return false;
}

int CALLBACK UrlWordBreakProc(LPCWSTR edit_text, int current_pos, int length, int action)
{
	// The MSDN docs are not really helpful, and for plain edit controls
	// they're also wrong.
	// When WB_LEFT is asked for, we have to return a different value
	// depending on the user tried a Ctrl-Left or Ctrl-Right. If we don't,
	// then the cursor doesn't move correctly.
	//
	// We detect a Ctrl-Left from a Ctrl-Right by using a static variable,
	// which is set when WB_ISDELIMITER is asked.
	// The order of actions is:
	// WB_LEFT, WB_RIGHT for Ctrl-Left
	// WB_ISDELIMITER, WB_LEFT, WB_RIGHT for Ctrl-Right
	static bool bRight = FALSE; // hack due to bug in USER
	switch (action)
	{
	// Find the beginning of a word to the left of the specified position
	case WB_LEFT:
	{
		if (current_pos < 1)
		{
			// current_pos == 0, so we have a "not found" case and return 0
			return 0;
		}
		if (bRight)
		{
			// when moving the cursor right, if we're at a word start
			// then just return that position
			bRight = false;
			if (IsDelimiter(edit_text, current_pos - 1, length))
				return current_pos - 1;
		}

		bRight = false;
		// skip all delimiters to the left
		--current_pos;
		while ((current_pos >= 0) && IsDelimiter(edit_text, current_pos, length))
			--current_pos;

		// look for a delimiter before the current character.
		// the previous word starts right after.
		for (int i = current_pos - 1; i >= 0; --i)
		{
			if (IsDelimiter(edit_text, i, length))
				return i + 1;
		}

		return 0;
	}
	break;

	// Find the beginning of a word to the right of the specified position
	case WB_RIGHT:
	{
		bRight = false;
		// look for a delimiter after the current position.
		// the next word starts immediately after.
		for (int i = current_pos + 1; i < length; ++i)
		{
			if (IsDelimiter(edit_text, i, length))
			{
				// skip multiple delimiters
				if (i < (length - 2))
				{
					while (IsDelimiter(edit_text, i + 1, length))
						++i;
				}

				return i + 1;
			}
		}
		return length;
	}
	break;

	// Determine if the current character delimits words.
	case WB_ISDELIMITER:
		// set the bRight flag: user tries Ctrl-Right
		bRight = true;
		return IsDelimiter(edit_text, current_pos, length);
		break;

	default:
		ATLASSERT(false);
		break;
	}

	return 0;
}

struct ChildWndProcBaton
{
	int counter = 0;
	bool includeComboboxes = false;
};

BOOL CALLBACK EnumChildProc(HWND hChild, LPARAM lParam)
{
	auto pBaton = reinterpret_cast<ChildWndProcBaton*>(lParam);
	wchar_t cbuf[100] = { 0 };
	if (GetClassName(hChild, cbuf, _countof(cbuf)))
	{
		if (_wcsicmp(cbuf, L"edit") == 0)
		{
			SendMessage(hChild, EM_SETWORDBREAKPROC, 0, reinterpret_cast<LPARAM>(&UrlWordBreakProc));
			++pBaton->counter;
		}
		if (pBaton->includeComboboxes)
		{
			if (_wcsicmp(cbuf, L"combobox") == 0)
			{
				COMBOBOXINFO cbi = { sizeof(COMBOBOXINFO) };
				if (GetComboBoxInfo(hChild, &cbi))
				{
					SendMessage(cbi.hwndItem, EM_SETWORDBREAKPROC, 0, reinterpret_cast<LPARAM>(&UrlWordBreakProc));
					++pBaton->counter;
				}
			}
			if (_wcsicmp(cbuf, L"ComboBoxEx32") == 0)
			{
				auto hEdit = reinterpret_cast<HWND>(SendMessage(hChild, CBEM_GETEDITCONTROL, 0, 0));
				if (hEdit)
				{
					SendMessage(hEdit, EM_SETWORDBREAKPROC, 0, reinterpret_cast<LPARAM>(&UrlWordBreakProc));
					++pBaton->counter;
				}
			}
		}
	}
	return TRUE;
}

int SetUrlWordBreakProcToChildWindows(HWND hParent, bool includeComboboxes)
{
	ChildWndProcBaton baton;
	baton.includeComboboxes = includeComboboxes;
	baton.counter = 0;
	EnumChildWindows(hParent, EnumChildProc, reinterpret_cast<LPARAM>(&baton));
	return baton.counter;
}
