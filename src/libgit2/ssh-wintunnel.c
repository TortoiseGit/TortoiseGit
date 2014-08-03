// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2014 TortoiseGit
// Copyright (C) the libgit2 contributors. All rights reserved.
//               - based on libgit2/src/transports/ssh.c

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
#include "../../ext/libgit2/src/transports/smart.h"
#include "system-call.h"
#include "ssh-wintunnel.h"

#define OWNING_SUBTRANSPORT(s) ((ssh_subtransport *)(s)->parent.subtransport)

static const char prefix_ssh[] = "ssh://";
static const char cmd_uploadpack[] = "git-upload-pack";
static const char cmd_receivepack[] = "git-receive-pack";

typedef struct {
	git_smart_subtransport_stream parent;
	COMMAND_HANDLE commandHandle;
	const char *cmd;
	char *url;
} ssh_stream;

typedef struct {
	git_smart_subtransport parent;
	transport_smart *owner;
	ssh_stream *current_stream;
	char *cmd_uploadpack;
	char *cmd_receivepack;
} ssh_subtransport;

/*
 * Create a git protocol request.
 *
 * For example: git-upload-pack '/libgit2/libgit2'
 */
static int gen_proto(git_buf *request, const char *cmd, const char *url)
{
	const char *repo;

	if (!git__prefixcmp(url, prefix_ssh)) {
		url = url + strlen(prefix_ssh);
		repo = strchr(url, '/');
	} else {
		repo = strchr(url, ':');
		if (repo) repo++;
	}

	if (!repo) {
		giterr_set(GITERR_NET, "Malformed git protocol URL");
		return -1;
	}

	git_buf_printf(request, "\"%s '%s'\"", cmd, repo);

	if (git_buf_oom(request)) {
		giterr_set_oom();
		return -1;
	}

	return 0;
}

static int ssh_stream_read(
	git_smart_subtransport_stream *stream,
	char *buffer,
	size_t buf_size,
	size_t *bytes_read)
{
	ssh_stream *s = (ssh_stream *)stream;

	if (command_read_stdout(&s->commandHandle, buffer, buf_size, bytes_read))
		return -1;

	return 0;
}

static int ssh_stream_write(
	git_smart_subtransport_stream *stream,
	const char *buffer,
	size_t len)
{
	ssh_stream *s = (ssh_stream *)stream;

	if (command_write(&s->commandHandle, buffer, len))
		return -1;

	return 0;
}

static void ssh_stream_free(git_smart_subtransport_stream *stream)
{
	ssh_stream *s = (ssh_stream *)stream;
	ssh_subtransport *t = OWNING_SUBTRANSPORT(s);

	DWORD exitcode = command_close(&s->commandHandle);
	if (s->commandHandle.errBuf) {
		if (exitcode && exitcode != MAXDWORD) {
			if (!git_buf_oom(s->commandHandle.errBuf) && git_buf_len(s->commandHandle.errBuf))
				giterr_set(GITERR_SSH, "Command exited non-zero (%ld) and returned:\n%s", exitcode, s->commandHandle.errBuf->ptr);
			else
				giterr_set(GITERR_SSH, "Command exited non-zero: %ld", exitcode);
		}
		git_buf_free(s->commandHandle.errBuf);
		git__free(s->commandHandle.errBuf);
	}

	t->current_stream = NULL;

	git__free(s->url);
	git__free(s);
}

static int ssh_stream_alloc(
	ssh_subtransport *t,
	const char *url,
	const char *cmd,
	git_smart_subtransport_stream **stream)
{
	ssh_stream *s;
	git_buf *errBuf;

	assert(stream);

	s = git__calloc(sizeof(ssh_stream), 1);
	if (!s) {
		giterr_set_oom();
		return -1;
	}

	errBuf = git__calloc(sizeof(git_buf), 1);
	if (!errBuf) {
		git__free(s);
		giterr_set_oom();
		return -1;
	}

	s->parent.subtransport = &t->parent;
	s->parent.read = ssh_stream_read;
	s->parent.write = ssh_stream_write;
	s->parent.free = ssh_stream_free;

	s->cmd = cmd;

	s->url = git__strdup(url);
	if (!s->url) {
		git__free(s);
		giterr_set_oom();
		return -1;
	}

	command_init(&s->commandHandle);
	s->commandHandle.errBuf = errBuf;

	*stream = &s->parent;
	return 0;
}

static int git_ssh_extract_url_parts(
	char **host,
	char **username,
	const char *url)
{
	const char *colon, *at;
	const char *start;

	colon = strchr(url, ':');

	at = strchr(url, '@');
	if (at) {
		start = at + 1;
		*username = git__substrdup(url, at - url);
		if (!*username) {
			giterr_set_oom();
			return -1;
		}
	} else {
		start = url;
		*username = NULL;
	}

	if (colon == NULL || (colon < start)) {
		giterr_set(GITERR_NET, "Malformed URL");
		return -1;
	}

	*host = git__substrdup(start, colon - start);
	if (!*host) {
		giterr_set_oom();
		return -1;
	}

	return 0;
}

static int wcstristr(const wchar_t *heystack, const wchar_t *needle)
{
	const wchar_t *end;
	size_t lenNeedle = wcslen(needle);
	size_t lenHeystack = wcslen(heystack);
	if (lenNeedle > lenHeystack)
		return 0;

	end = heystack + lenHeystack - lenNeedle;
	while (heystack <= end) {
		if (!wcsnicmp(heystack, needle, lenNeedle))
			return 1;
		++heystack;
	}

	return 0;
}

static int _git_ssh_setup_tunnel(
	ssh_subtransport *t,
	const char *url,
	const char *gitCmd,
	git_smart_subtransport_stream **stream)
{
	char *host = NULL, *port = NULL, *path = NULL, *user = NULL, *pass = NULL;
	const char *default_port = "22";
	ssh_stream *s;
	wchar_t *ssh;
	wchar_t *wideParams = NULL;
	wchar_t *cmd = NULL;
	git_buf params = GIT_BUF_INIT;
	int isPutty;
	size_t length;

	*stream = NULL;
	if (ssh_stream_alloc(t, url, gitCmd, stream) < 0) {
		giterr_set_oom();
		return -1;
	}

	s = (ssh_stream *)*stream;

	if (!git__prefixcmp(url, prefix_ssh)) {
		if (gitno_extract_url_parts(&host, &port, &path, &user, &pass, url, default_port) < 0)
			goto on_error;
	}
	else {
		if (git_ssh_extract_url_parts(&host, &user, url) < 0)
			goto on_error;
		port = git__strdup(default_port);
		if (!port) {
			giterr_set_oom();
			goto on_error;
		}
	}

	ssh = _wgetenv(L"GIT_SSH");
	if (!ssh)
	{
		giterr_set(GITERR_SSH, "No GIT_SSH environment variable set");
		goto on_error;
	}

	isPutty = wcstristr(ssh, L"plink");
	if (port) {
		if (isPutty)
			git_buf_printf(&params, " -P %s", port);
		else
			git_buf_printf(&params, " -p %s", port);
	}
	if (isPutty && !wcstristr(ssh, L"tortoiseplink")) {
		git_buf_puts(&params, " -batch");
	}
	if (user)
		git_buf_printf(&params, " %s@%s ", user, host);
	else
		git_buf_printf(&params, " %s ", host);
	if (gen_proto(&params, s->cmd, s->url))
		goto on_error;
	if (git_buf_oom(&params)) {
		giterr_set_oom();
		goto on_error;
	}

	if (git__utf8_to_16_alloc(&wideParams, params.ptr) < 0) {
		giterr_set_oom();
		goto on_error;
	}
	git_buf_free(&params);

	length = wcslen(ssh) + wcslen(wideParams) + 3;
	cmd = git__calloc(length, sizeof(wchar_t));
	if (!cmd) {
		giterr_set_oom();
		goto on_error;
	}

	wcscat_s(cmd, length, L"\"");
	wcscat_s(cmd, length, ssh);
	wcscat_s(cmd, length, L"\"");
	wcscat_s(cmd, length, wideParams);

	if (command_start(cmd, &s->commandHandle, NULL, CREATE_NEW_CONSOLE))
		goto on_error;

	git__free(wideParams);
	git__free(cmd);
	t->current_stream = s;
	git__free(host);
	git__free(port);
	git__free(path);
	git__free(user);
	git__free(pass);

	return 0;

on_error:
	t->current_stream = NULL;

	if (*stream)
		ssh_stream_free(*stream);

	git_buf_free(&params);

	if (wideParams)
		git__free(wideParams);

	if (cmd)
		git__free(cmd);

	git__free(host);
	git__free(port);
	git__free(user);
	git__free(pass);

	return -1;
}

static int ssh_uploadpack_ls(
	ssh_subtransport *t,
	const char *url,
	git_smart_subtransport_stream **stream)
{
	const char *cmd = t->cmd_uploadpack ? t->cmd_uploadpack : cmd_uploadpack;

	if (_git_ssh_setup_tunnel(t, url, cmd, stream) < 0)
		return -1;

	return 0;
}

static int ssh_uploadpack(
	ssh_subtransport *t,
	const char *url,
	git_smart_subtransport_stream **stream)
{
	GIT_UNUSED(url);

	if (t->current_stream) {
		*stream = &t->current_stream->parent;
		return 0;
	}

	giterr_set(GITERR_NET, "Must call UPLOADPACK_LS before UPLOADPACK");
	return -1;
}

static int ssh_receivepack_ls(
	ssh_subtransport *t,
	const char *url,
	git_smart_subtransport_stream **stream)
{
	const char *cmd = t->cmd_receivepack ? t->cmd_receivepack : cmd_receivepack;

	if (_git_ssh_setup_tunnel(t, url, cmd, stream) < 0)
		return -1;

	return 0;
}

static int ssh_receivepack(
	ssh_subtransport *t,
	const char *url,
	git_smart_subtransport_stream **stream)
{
	GIT_UNUSED(url);

	if (t->current_stream) {
		*stream = &t->current_stream->parent;
		return 0;
	}

	giterr_set(GITERR_NET, "Must call RECEIVEPACK_LS before RECEIVEPACK");
	return -1;
}

static int _ssh_action(
	git_smart_subtransport_stream **stream,
	git_smart_subtransport *subtransport,
	const char *url,
	git_smart_service_t action)
{
	ssh_subtransport *t = (ssh_subtransport *) subtransport;

	switch (action) {
		case GIT_SERVICE_UPLOADPACK_LS:
			return ssh_uploadpack_ls(t, url, stream);

		case GIT_SERVICE_UPLOADPACK:
			return ssh_uploadpack(t, url, stream);

		case GIT_SERVICE_RECEIVEPACK_LS:
			return ssh_receivepack_ls(t, url, stream);

		case GIT_SERVICE_RECEIVEPACK:
			return ssh_receivepack(t, url, stream);
	}

	*stream = NULL;
	return -1;
}

static int _ssh_close(git_smart_subtransport *subtransport)
{
	ssh_subtransport *t = (ssh_subtransport *) subtransport;

	assert(!t->current_stream);

	return 0;
}

static void _ssh_free(git_smart_subtransport *subtransport)
{
	ssh_subtransport *t = (ssh_subtransport *) subtransport;

	assert(!t->current_stream);

	git__free(t->cmd_uploadpack);
	git__free(t->cmd_receivepack);
	git__free(t);
}

int git_smart_subtransport_ssh_wintunnel(
	git_smart_subtransport **out, git_transport *owner)
{
	ssh_subtransport *t;

	assert(out);

	t = git__calloc(sizeof(ssh_subtransport), 1);
	if (!t) {
		giterr_set_oom();
		return -1;
	}

	t->owner = (transport_smart *)owner;
	t->parent.action = _ssh_action;
	t->parent.close = _ssh_close;
	t->parent.free = _ssh_free;

	*out = (git_smart_subtransport *) t;
	return 0;
}

int git_transport_sshwintunnel_with_paths(git_transport **out, git_remote *owner, void *payload)
{
	git_strarray *paths = (git_strarray *) payload;
	git_transport *transport;
	transport_smart *smart;
	ssh_subtransport *t;
	int error;
	git_smart_subtransport_definition ssh_definition = {
		git_smart_subtransport_ssh_wintunnel,
		0, /* no RPC */
	};

	if (paths->count != 2) {
		giterr_set(GITERR_SSH, "invalid ssh paths, must be two strings");
		return GIT_EINVALIDSPEC;
	}

	if ((error = git_transport_smart(&transport, owner, &ssh_definition)) < 0)
		return error;

	smart = (transport_smart *) transport;
	t = (ssh_subtransport *) smart->wrapped;

	t->cmd_uploadpack = git__strdup(paths->strings[0]);
	if (!t->cmd_uploadpack) {
		giterr_set_oom();
		return -1;
	}
	t->cmd_receivepack = git__strdup(paths->strings[1]);
	if (!t->cmd_receivepack)
	{
		git__free(t->cmd_uploadpack);
		giterr_set_oom();
		return -1;
	}

	*out = transport;
	return 0;
}
