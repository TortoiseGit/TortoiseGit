// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014 - TortoiseGit

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

#include "stdafx.h"
#include "FetchProgressCommand.h"
#include "AppUtils.h"

bool FetchProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& /*m_itemCountTotal*/, int& /*m_itemCount*/)
{
	if (!g_Git.UsingLibGit2(CGit::GIT_CMD_FETCH))
	{
		// should never run to here
		ASSERT(0);
		return false;
	}

	list->SetWindowTitle(IDS_PROGRS_TITLE_FETCH, g_Git.m_CurrentDir, sWindowTitle);
	list->SetBackgroundImage(IDI_UPDATE_BKG);
	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_TITLE_FETCH)) + _T(" ") + m_url.GetGitPathString() + _T(" ") + m_RefSpec);

	CStringA url = CUnicodeUtils::GetUTF8(m_url.GetGitPathString());

	CSmartAnimation animate(list->m_pAnimate);

	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		list->ReportGitError();
		return false;
	}

	CAutoRemote remote;
	// first try with a named remote (e.g. "origin")
	if (git_remote_load(remote.GetPointer(), repo, url) < 0)
	{
		// retry with repository located at a specific url
		if (git_remote_create_anonymous(remote.GetPointer(), repo, url, nullptr) < 0)
		{
			list->ReportGitError();
			return false;
		}
	}

	git_remote_callbacks callbacks = GIT_REMOTE_CALLBACKS_INIT;
	callbacks.update_tips = RemoteUpdatetipsCallback;
	callbacks.sideband_progress = RemoteProgressCallback;
	callbacks.transfer_progress = FetchCallback;
	callbacks.completion = RemoteCompletionCallback;
	callbacks.credentials = CAppUtils::Git2GetUserPassword;
	callbacks.payload = list;

	git_remote_set_callbacks(remote, &callbacks);
	git_remote_set_autotag(remote, (git_remote_autotag_option_t)m_AutoTag);

	if (!m_RefSpec.IsEmpty() && git_remote_add_fetch(remote, CUnicodeUtils::GetUTF8(m_RefSpec)))
		goto error;

	// Connect to the remote end specifying that we want to fetch
	// information from it.
	if (git_remote_connect(remote, GIT_DIRECTION_FETCH) < 0)
		goto error;

	// Download the packfile and index it. This function updates the
	// amount of received data and the indexer stats which lets you
	// inform the user about progress.
	if (git_remote_download(remote) < 0)
		goto error;

	// Update the references in the remote's namespace to point to the
	// right commits. This may be needed even if there was no packfile
	// to download, which can happen e.g. when the branches have been
	// changed but all the neede objects are available locally.
	if (git_remote_update_tips(remote, nullptr, nullptr) < 0)
		goto error;

	git_remote_disconnect(remote);

	return true;

error:
	list->ReportGitError();
	return false;
}
