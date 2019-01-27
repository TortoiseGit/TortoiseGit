// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014, 2016, 2019 TortoiseGit

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

static void safeCloseHandle(HANDLE *handle)
{
	if (*handle != INVALID_HANDLE_VALUE) {
		CloseHandle(*handle);
		*handle = INVALID_HANDLE_VALUE;
	}
}

static int command_read(HANDLE handle, char *buffer, size_t buf_size, size_t *bytes_read)
{
	*bytes_read = 0;

	if (!ReadFile(handle, buffer, (DWORD)buf_size, (DWORD*)bytes_read, NULL)) {
		git_error_set(GIT_ERROR_OS, "could not read data from external process");
		return -1;
	}

	return 0;
}

static int command_readall(HANDLE handle, git_buf *buf)
{
	size_t bytes_read = 0;
	do {
		char buffer[65536];
		command_read(handle, buffer, sizeof(buffer), &bytes_read);
		git_buf_put(buf, buffer, bytes_read);
	} while (bytes_read);

	return 0;
}

struct ASYNCREADINGTHREADARGS {
	HANDLE *handle;
	git_buf *dest;
};

static DWORD WINAPI AsyncReadingThread(LPVOID lpParam)
{
	struct ASYNCREADINGTHREADARGS* pDataArray = (struct ASYNCREADINGTHREADARGS*)lpParam;

	int ret = command_readall(*pDataArray->handle, pDataArray->dest);

	safeCloseHandle(pDataArray->handle);

	git__free(pDataArray);

	return ret;
}

static HANDLE commmand_start_reading_thread(HANDLE *handle, git_buf *dest)
{
	struct ASYNCREADINGTHREADARGS *threadArguments = git__calloc(1, sizeof(struct ASYNCREADINGTHREADARGS));
	HANDLE thread;
	if (!threadArguments) {
		git_error_set_oom();
		return NULL;
	}

	threadArguments->handle = handle;
	threadArguments->dest = dest;

	thread = CreateThread(NULL, 0, AsyncReadingThread, threadArguments, 0, NULL);
	if (!thread) {
		git__free(threadArguments);
		git_error_set(GIT_ERROR_OS, "Could not create thread");
	}
	return thread;
}

int commmand_start_stdout_reading_thread(COMMAND_HANDLE *commandHandle, git_buf *dest)
{
	HANDLE thread = commmand_start_reading_thread(&commandHandle->out, dest);
	if (!thread)
		return -1;
	commandHandle->asyncReadOutThread = thread;
	return 0;
}

static int command_wait_reading_thread(HANDLE *handle)
{
	DWORD exitCode = MAXDWORD;
	if (*handle == INVALID_HANDLE_VALUE)
		return -1;

	WaitForSingleObject(*handle, INFINITE);
	if (!GetExitCodeThread(*handle, &exitCode) || exitCode) {
		safeCloseHandle(handle);
		return -1;
	}
	safeCloseHandle(handle);
	return 0;
}

int command_wait_stdout_reading_thread(COMMAND_HANDLE *commandHandle)
{
	return command_wait_reading_thread(&commandHandle->asyncReadOutThread);
}

void command_init(COMMAND_HANDLE *commandHandle)
{
	memset(commandHandle, 0, sizeof(COMMAND_HANDLE));
	commandHandle->in = INVALID_HANDLE_VALUE;
	commandHandle->out = INVALID_HANDLE_VALUE;
	commandHandle->err = INVALID_HANDLE_VALUE;
	commandHandle->asyncReadErrorThread = INVALID_HANDLE_VALUE;
	commandHandle->asyncReadOutThread = INVALID_HANDLE_VALUE;
}

int command_start(wchar_t *cmd, COMMAND_HANDLE *commandHandle, LPWSTR* pEnv, DWORD flags)
{
	SECURITY_ATTRIBUTES sa;
	HANDLE hReadOut = INVALID_HANDLE_VALUE, hWriteOut = INVALID_HANDLE_VALUE, hReadIn = INVALID_HANDLE_VALUE, hWriteIn = INVALID_HANDLE_VALUE, hReadError = INVALID_HANDLE_VALUE, hWriteError = INVALID_HANDLE_VALUE;
	STARTUPINFOW si = { 0 };
	PROCESS_INFORMATION pi = { 0 };

	si.cb = sizeof(STARTUPINFOW);

	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;
	if (!CreatePipe(&hReadOut, &hWriteOut, &sa, 0)) {
		git_error_set_str(GIT_ERROR_OS, "Could not create pipe");
		return -1;
	}
	if (!CreatePipe(&hReadIn, &hWriteIn, &sa, 0)) {
		git_error_set_str(GIT_ERROR_OS, "Could not create pipe");
		CloseHandle(hReadOut);
		CloseHandle(hWriteOut);
		return -1;
	}
	if (commandHandle->errBuf && !CreatePipe(&hReadError, &hWriteError, &sa, 0)) {
		git_error_set_str(GIT_ERROR_OS, "Could not create pipe");
		CloseHandle(hReadOut);
		CloseHandle(hWriteOut);
		CloseHandle(hReadIn);
		CloseHandle(hWriteIn);
		return -1;
	}

	si.hStdOutput = hWriteOut;
	si.hStdInput = hReadIn;
	si.hStdError = hWriteError;

	// Ensure the read/write handle to the pipe for STDOUT resp. STDIN are not inherited.
	if (!SetHandleInformation(hReadOut, HANDLE_FLAG_INHERIT, 0) || !SetHandleInformation(hWriteIn, HANDLE_FLAG_INHERIT, 0) || (commandHandle->errBuf && !SetHandleInformation(hReadError, HANDLE_FLAG_INHERIT, 0))) {
		git_error_set_str(GIT_ERROR_OS, "SetHandleInformation failed");
		CloseHandle(hReadOut);
		CloseHandle(hWriteOut);
		CloseHandle(hReadIn);
		CloseHandle(hWriteIn);
		safeCloseHandle(&hReadError);
		safeCloseHandle(&hWriteError);
		return -1;
	}

	si.wShowWindow = SW_HIDE;
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;

	if (!CreateProcessW(NULL, cmd, NULL, NULL, TRUE, CREATE_UNICODE_ENVIRONMENT | flags, pEnv ? *pEnv : NULL, NULL, &si, &pi)) {
		git_error_set_str(GIT_ERROR_OS, "Could not start external tool");
		CloseHandle(hReadOut);
		CloseHandle(hWriteOut);
		CloseHandle(hReadIn);
		CloseHandle(hWriteIn);
		safeCloseHandle(&hReadError);
		safeCloseHandle(&hWriteError);
		return -1;
	}

	AllowSetForegroundWindow(pi.dwProcessId);
	WaitForInputIdle(pi.hProcess, 10000);

	CloseHandle(hReadIn);
	CloseHandle(hWriteOut);
	if (commandHandle->errBuf) {
		HANDLE asyncReadErrorThread;
		CloseHandle(hWriteError);
		commandHandle->err = hReadError;
		asyncReadErrorThread = commmand_start_reading_thread(&commandHandle->err, commandHandle->errBuf);
		if (!asyncReadErrorThread) {
			CloseHandle(hReadOut);
			CloseHandle(hWriteIn);
			CloseHandle(hReadError);
			return -1;
		}
		commandHandle->asyncReadErrorThread = asyncReadErrorThread;
	}

	commandHandle->pi = pi;
	commandHandle->out = hReadOut;
	commandHandle->in = hWriteIn;
	commandHandle->running = TRUE;
	return 0;
}

int command_read_stdout(COMMAND_HANDLE *commandHandle, char *buffer, size_t buf_size, size_t *bytes_read)
{
	return command_read(commandHandle->out, buffer, buf_size, bytes_read);
}

int command_write(COMMAND_HANDLE *commandHandle, const char *buffer, size_t len)
{
	size_t off = 0;
	DWORD written = 0;

	do {
		if (!WriteFile(commandHandle->in, buffer + off, (DWORD)(len - off), &written, NULL)) {
			git_error_set_str(GIT_ERROR_OS, "could not write data to external process");
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
	DWORD exitcode = MAXDWORD;
	if (!commandHandle->running)
		return exitcode;

	commandHandle->running = FALSE;

	command_close_stdin(commandHandle);
	command_wait_stdout_reading_thread(commandHandle);
	command_close_stdout(commandHandle);

	CloseHandle(commandHandle->pi.hThread);

	WaitForSingleObject(commandHandle->pi.hProcess, INFINITE);

	if (commandHandle->errBuf)
		command_wait_reading_thread(&commandHandle->asyncReadErrorThread);

	GetExitCodeProcess(commandHandle->pi.hProcess, &exitcode);
	CloseHandle(commandHandle->pi.hProcess);

	return exitcode;
}
