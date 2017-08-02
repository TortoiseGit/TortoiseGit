// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2008-2011, 2013, 2016-2017 - TortoiseGit

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
#include "gitdll.h"
#include "stdio.h"

#define MAX_ERROR_STR_SIZE 512
extern "C" char g_last_error[MAX_ERROR_STR_SIZE]={0};
extern "C" int close_all();
static int dyingRecCnt = 0;

static void reset_dll_state()
{
	close_all();
	dyingRecCnt = 0;
}

static void set_last_error(const char *err, va_list params)
{
	memset(g_last_error, 0, MAX_ERROR_STR_SIZE);
	vsnprintf(g_last_error, MAX_ERROR_STR_SIZE - 1, err, params);
}

extern "C" [[noreturn]] void die_dll(const char* err, va_list params)
{
	set_last_error(err, params);
	reset_dll_state();
	throw g_last_error;
}

[[noreturn]] void die(const char* err, ...)
{
	va_list params;
	va_start(params, err);
	set_last_error(err, params);
	va_end(params);

	reset_dll_state();

	throw g_last_error;
}

extern "C" void handle_error(const char*, va_list)
{
// ignore for now
}

extern "C" void handle_warning(const char*, va_list)
{
// ignore for now
}

extern "C" [[noreturn]] void vc_exit(int code)
{
	if (strlen(g_last_error))
	{
		char old_err[MAX_ERROR_STR_SIZE];
		memcpy(old_err, g_last_error, MAX_ERROR_STR_SIZE);
		die("libgit called \"exit(%d)\". Last error was:\n%s", code, old_err);
	}
	else
		die("libgit called \"exit(%d)\".", code);
}

extern "C" int die_is_recursing_dll()
{
	return dyingRecCnt++;
}
