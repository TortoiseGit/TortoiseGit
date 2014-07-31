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

#include "buffer.h"
#include "netops.h"
#include "system-call.h"

int command_start(wchar_t *cmd, COMMAND_HANDLE *commandHandle)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hReadOut = INVALID_HANDLE_VALUE, hWriteOut = INVALID_HANDLE_VALUE, hReadIn = INVALID_HANDLE_VALUE, hWriteIn = INVALID_HANDLE_VALUE;
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	si.cb = sizeof(STARTUPINFOW);

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hReadOut, &hWriteOut, &sa, 0)) {
		giterr_set(GITERR_OS, "Could not create pipe");
		return -1;
	}
	if (!CreatePipe(&hReadIn, &hWriteIn, &sa, 0)) {
		giterr_set(GITERR_OS, "Could not create pipe");
		CloseHandle(hReadOut);
		CloseHandle(hWriteOut);
		return -1;
	}

	si.hStdOutput = hWriteOut;
	si.hStdInput = hReadIn;
	si.hStdError = INVALID_HANDLE_VALUE;

	// Ensure the read/write handle to the pipe for STDOUT resp. STDIN are not inherited.
	if (!SetHandleInformation(hReadOut, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(hWriteIn, HANDLE_FLAG_INHERIT, 0)) {
		CloseHandle(hReadOut);
		CloseHandle(hWriteOut);
		CloseHandle(hReadIn);
		CloseHandle(hWriteIn);
		return -1;
	}

	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		giterr_set(GITERR_OS, "Could not start external tool");
		CloseHandle(hReadOut);
		CloseHandle(hWriteOut);
		CloseHandle(hReadIn);
		CloseHandle(hWriteIn);
		return -1;
	}

	AllowSetForegroundWindow(pi.dwProcessId);
	WaitForInputIdle(pi.hProcess, 10000);

	CloseHandle(hReadIn);
	CloseHandle(hWriteOut);

	commandHandle->pi = pi;
	commandHandle->out = hReadOut;
	commandHandle->in = hWriteIn;
	commandHandle->running = TRUE;
	return 0;
}

int command_read(COMMAND_HANDLE *commandHandle, char *buffer, size_t buf_size, size_t *bytes_read)
{
	*bytes_read = 0;

	if (!ReadFile(commandHandle->out, buffer, (DWORD)buf_size, (DWORD*)bytes_read, NULL)) {
		giterr_set(GITERR_OS, "could not read data from external process");
		return -1;
	}

	return 0;
}

int command_readall(COMMAND_HANDLE *commandHandle, git_buf *buf)
{
	size_t bytes_read = 0;
	do {
		char buffer[65536];
		command_read(commandHandle->out, buffer, sizeof(buffer), &bytes_read);
		git_buf_put(buf, buffer, bytes_read);
	} while (bytes_read);

	return 0;
}

int command_write(COMMAND_HANDLE *commandHandle, const char *buffer, size_t len)
{
	size_t off = 0;
	DWORD written = 0;

	do {
		if (!WriteFile(commandHandle->in, buffer + off, (DWORD)(len - off), &written, NULL)) {
			giterr_set(GITERR_OS, "could not write data to external process");
			return -1;
		}

		off += written;
	} while (off < len);

	return 0;
}

int command_write_gitbuf(COMMAND_HANDLE *commandHandle, const git_buf *buf)
{
	return command_write(commandHandle, buf->ptr, buf->size);
}

static void safeCloseHandle(HANDLE *handle)
{
	if (*handle != INVALID_HANDLE_VALUE) {
		CloseHandle(*handle);
		*handle = INVALID_HANDLE_VALUE;
	}
}

void command_close_stdin(COMMAND_HANDLE *commandHandle)
{
	safeCloseHandle(&commandHandle->in);
}

void command_close_stdout(COMMAND_HANDLE *commandHandle)
{
	safeCloseHandle(&commandHandle->out);
}

DWORD command_close(COMMAND_HANDLE *commandHandle)
{
	if (!commandHandle->running)
		return 1;

	commandHandle->running = FALSE;

	command_close_stdin(commandHandle);
	command_close_stdout(commandHandle);

	CloseHandle(commandHandle->pi.hThread);

	WaitForSingleObject(commandHandle->pi.hProcess, INFINITE);

	DWORD exitcode = MAXDWORD;
	GetExitCodeProcess(commandHandle->pi.hProcess, &exitcode);
	CloseHandle(commandHandle->pi.hProcess);

	return exitcode;
}
