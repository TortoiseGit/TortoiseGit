// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014 TortoiseGit

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

#include "common.h"

typedef struct {
	PROCESS_INFORMATION pi;
	HANDLE in;
	HANDLE out;
	HANDLE err;
	bool running;
	git_buf *outBuf;
	git_buf *errBuf;
	HANDLE asyncReadOutThread;
	HANDLE asyncReadErrorThread;
} COMMAND_HANDLE;

void command_init(COMMAND_HANDLE *commandHandle);
int command_start(wchar_t *cmd, COMMAND_HANDLE *commandHandle, LPWSTR* pEnv, DWORD flags);
void command_close_stdout(COMMAND_HANDLE *commandHandle);
void command_close_stdin(COMMAND_HANDLE *commandHandle);
DWORD command_close(COMMAND_HANDLE *commandHandle);
int command_read_stdout(COMMAND_HANDLE *commandHandle, char *buffer, size_t buf_size, size_t *bytes_read);
int command_write(COMMAND_HANDLE *commandHandle, const char *buffer, size_t len);
int command_write_gitbuf(COMMAND_HANDLE *commandHandle, const git_buf *buf);
int commmand_start_stdout_reading_thread(COMMAND_HANDLE *commandHandle, git_buf *dest);
int command_wait_stdout_reading_thread(COMMAND_HANDLE *commandHandle);
