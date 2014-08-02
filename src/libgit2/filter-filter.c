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

#include "git2/attr.h"
#include "git2/blob.h"
#include "git2/index.h"
#include "git2/sys/filter.h"

#include "common.h"
#include "fileops.h"
#include "hash.h"
#include "filter.h"
#include "buf_text.h"
#include "repository.h"

#include "system-call.h"
#include "filter-filter.h"

struct filter_filter {
	git_filter	f;
	LPWSTR		pEnv;
	LPWSTR		shexepath;
};

static int filter_check(
	git_filter				*self,
	void					**payload, /* points to NULL ptr on entry, may be set */
	const git_filter_source	*src,
	const char				**attr_values)
{
	GIT_UNUSED(self);
	GIT_UNUSED(src);

	if (GIT_ATTR_UNSPECIFIED(attr_values[0]))
		return GIT_PASSTHROUGH;

	if (GIT_ATTR_FALSE(attr_values[0]))
		return GIT_PASSTHROUGH;

	if (GIT_ATTR_TRUE(attr_values[0]))
		return GIT_PASSTHROUGH;
	
	*payload = git__strdup(attr_values[0]);
	if (!*payload)
		return 1;

	return 0;
}

static int expandPerCentF(git_buf *buf, const char *replaceWith)
{
	ssize_t foundPercentage = git_buf_find(buf, '%');
	if (foundPercentage) {
		git_buf expanded = GIT_BUF_INIT;
		const char *end = buf->ptr + buf->size;
		const char *lastPercentage = buf->ptr;
		const char *idx = buf->ptr + foundPercentage;
		while (idx < end) {
			if (*idx == '%') {
				if (idx + 1 == buf->ptr + buf->size || (idx + 1 < end && *(idx + 1) == '%')) { // one '%' is at the end of the string OR "%%" is in the string
					git_buf_putc(&expanded, '%');
					++idx;
					++idx;
					lastPercentage = idx;
					continue;
				}
				// now we know, that we're not at the end of the string and that the next char is not '%'
				git_buf_put(&expanded, lastPercentage, idx - lastPercentage);
				++idx;
				if (*idx == 'f')
					git_buf_printf(&expanded, "\"%s\"", replaceWith);

				++idx;
				lastPercentage = idx;
				continue;
			}
			++idx;
		}
		if (lastPercentage)
			git_buf_put(&expanded, lastPercentage, idx - lastPercentage);
		if (git_buf_oom(&expanded))
			return -1;
		git_buf_swap(buf, &expanded);
		git_buf_free(&expanded);
	}
	return 0;
}

struct ASYNCREADINGTHREADARGS {
	COMMAND_HANDLE *commandHandle;
	git_buf *dest;
};

static DWORD WINAPI AsyncReadingThread(LPVOID lpParam)
{
	struct ASYNCREADINGTHREADARGS* pDataArray = (struct ASYNCREADINGTHREADARGS*)lpParam;

	int ret = command_readall(pDataArray->commandHandle, pDataArray->dest);

	git__free(pDataArray);

	return ret;
}

static HANDLE start_reading_thread(COMMAND_HANDLE *commandHandle, git_buf *dest)
{
	struct ASYNCREADINGTHREADARGS *threadArguments = git__calloc(1, sizeof(struct ASYNCREADINGTHREADARGS));
	if (!threadArguments)
		return NULL;

	threadArguments->commandHandle = commandHandle;
	threadArguments->dest = dest;

	HANDLE thread = CreateThread(NULL, 0, AsyncReadingThread, threadArguments, 0, NULL);
	if (!thread)
		git__free(threadArguments);
	return thread;
}

static int filter_apply(
	git_filter				*self,
	void					**payload, /* may be read and/or set */
	git_buf					*to,
	const git_buf			*from,
	const git_filter_source	*src)
{
	struct filter_filter *ffs = (struct filter_filter *)self;

	if (!*payload)
		return GIT_PASSTHROUGH;

	git_config *config;
	if (git_repository_config__weakptr(&config, git_filter_source_repo(src)))
		return -1;

	git_buf configKey = GIT_BUF_INIT;
	git_buf_join3(&configKey, '.', "filter", *payload, "required");
	if (git_buf_oom(&configKey))
		return -1;

	int isRequired = FALSE;
	int error = git_config_get_bool(&isRequired, config, configKey.ptr);
	git_buf_free(&configKey);
	if (error && error != GIT_ENOTFOUND)
		return -1;

	git_buf_join(&configKey, '.', "filter", *payload);
	if (git_filter_source_mode(src) == GIT_FILTER_SMUDGE) {
		git_buf_puts(&configKey, ".smudge");
	} else {
		git_buf_puts(&configKey, ".clean");
	}
	if (git_buf_oom(&configKey))
		return -1;

	const char *cmd = NULL;
	error = git_config_get_string(&cmd, config, configKey.ptr);
	git_buf_free(&configKey);
	if (error && error != GIT_ENOTFOUND)
		return -1;

	if (error == GIT_ENOTFOUND) {
		if (isRequired)
			return -1;
		return GIT_PASSTHROUGH;
	}

	git_buf cmdBuf = GIT_BUF_INIT;
	git_buf_puts(&cmdBuf, cmd);
	if (git_buf_oom(&cmdBuf))
		return -1;

	if (expandPerCentF(&cmdBuf, git_filter_source_path(src)))
		return 1;

	if (ffs->shexepath) {
		// build params for sh.exe
		git_buf shParams = GIT_BUF_INIT;
		git_buf_puts(&shParams, " -c \"");
		git_buf_text_puts_escaped(&shParams, cmdBuf.ptr, "\"\\", "\\");
		git_buf_puts(&shParams, "\"");
		git_buf_swap(&shParams, &cmdBuf);
		git_buf_free(&shParams);
	}

	wchar_t *wide_cmd;
	if (git__utf8_to_16_alloc(&wide_cmd, cmdBuf.ptr) < 0)
	{
		git_buf_free(&cmdBuf);
		return 1;
	}
	git_buf_free(&cmdBuf);

	if (ffs->shexepath) {
		// build cmd, i.e. shexepath + params
		size_t len = wcslen(ffs->shexepath) + wcslen(wide_cmd) + 1;
		wchar_t *tmp = git__calloc(len, sizeof(wchar_t));
		if (!tmp) {
			git__free(wide_cmd);
			return -1;
		}
		wcscat_s(tmp, len, ffs->shexepath);
		wcscat_s(tmp, len, wide_cmd);
		git__free(wide_cmd);
		wide_cmd = tmp;
	}

	COMMAND_HANDLE commandHandle = { 0 };
	if (command_start(wide_cmd, &commandHandle, ffs->pEnv)) {
		git__free(wide_cmd);
		if (isRequired)
			return -1;
		return GIT_PASSTHROUGH;
	}
	git__free(wide_cmd);

	HANDLE readingThread = start_reading_thread(&commandHandle, to);
	if (!readingThread) {
		command_close(&commandHandle);
		return -1;
	}

	if (command_write_gitbuf(&commandHandle, from)) {
		command_close(&commandHandle);
		CloseHandle(readingThread);
		if (isRequired)
			return -1;
		return GIT_PASSTHROUGH;
	}
	command_close_stdin(&commandHandle);

	DWORD exitCode = MAXDWORD;
	WaitForSingleObject(readingThread, INFINITE);
	if (!GetExitCodeThread(readingThread, &exitCode) || exitCode) {
		command_close(&commandHandle);
		CloseHandle(readingThread);
		if (isRequired)
			return -1;
		return GIT_PASSTHROUGH;
	}
	CloseHandle(readingThread);

	exitCode = command_close(&commandHandle);
	if (exitCode) {
		if (isRequired) {
			giterr_set(GITERR_FILTER, "External filter application exited non-zero: %ld", exitCode);
			return -1;
		}
		return GIT_PASSTHROUGH;
	}

	return 0;
}

static void filter_cleanup(
	git_filter	*self,
	void		*payload)
{
	GIT_UNUSED(self);
	git__free(payload);
}

static void filter_free(git_filter *self)
{
	struct filter_filter *ffs = (struct filter_filter *)self;

	if (ffs->shexepath)
		git__free(ffs->shexepath);

	git__free(self);
}

git_filter *git_filter_filter_new(LPCWSTR shexepath, LPWSTR pEnv)
{
	struct filter_filter *f = git__calloc(1, sizeof(struct filter_filter));

	f->f.version	= GIT_FILTER_VERSION;
	f->f.attributes	= "filter";
	f->f.initialize	= NULL;
	f->f.shutdown	= filter_free;
	f->f.check		= filter_check;
	f->f.apply		= filter_apply;
	f->f.cleanup	= filter_cleanup;
	f->shexepath	= wcsdup(shexepath);
	f->pEnv			= pEnv;

	return (git_filter *)f;
}
