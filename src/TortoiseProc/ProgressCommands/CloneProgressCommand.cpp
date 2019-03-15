// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2015, 2019 - TortoiseGit

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
#include "CloneProgressCommand.h"
#include "AppUtils.h"
#include "../TGitCache/CacheInterface.h"

bool CloneProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& /*m_itemCountTotal*/, int& /*m_itemCount*/)
{
	if (!g_Git.UsingLibGit2(CGit::GIT_CMD_CLONE))
	{
		// should never run to here
		ASSERT(FALSE);
		return false;
	}
	list->SetWindowTitle(IDS_PROGRS_TITLE_CLONE, m_url.GetGitPathString(), sWindowTitle);
	list->SetBackgroundImage(IDI_SWITCH_BKG);
	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROG_CLONE)));

	if (m_url.IsEmpty() || m_targetPathList.IsEmpty())
		return false;

	git_checkout_options checkout_opts = GIT_CHECKOUT_OPTIONS_INIT;
	checkout_opts.checkout_strategy = m_bNoCheckout ? GIT_CHECKOUT_NONE : GIT_CHECKOUT_SAFE;
	checkout_opts.progress_cb = CheckoutCallback;
	checkout_opts.progress_payload = list;

	git_clone_options cloneOpts = GIT_CLONE_OPTIONS_INIT;
	git_remote_callbacks& callbacks = cloneOpts.fetch_opts.callbacks;
	callbacks.update_tips = RemoteUpdatetipsCallback;
	callbacks.sideband_progress = RemoteProgressCallback;
	callbacks.completion = RemoteCompletionCallback;
	callbacks.transfer_progress = FetchCallback;
	callbacks.credentials = CAppUtils::Git2GetUserPassword;
	callbacks.certificate_check = CAppUtils::Git2CertificateCheck;
	CGitProgressList::Payload cbpayload = { list };
	callbacks.payload = &cbpayload;

	CSmartAnimation animate(list->m_pAnimate);
	cloneOpts.bare = m_bBare;
	cloneOpts.repository_cb = [](git_repository** out, const char* path, int bare, void* /*payload*/) -> int
	{
		git_repository_init_options init_options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
		init_options.flags = GIT_REPOSITORY_INIT_MKPATH | GIT_REPOSITORY_INIT_EXTERNAL_TEMPLATE;
		init_options.flags |= bare ? GIT_REPOSITORY_INIT_BARE : 0;

		return git_repository_init_ext(out, path, &init_options);
	};

	struct remote_cb_payload
	{
		const char* remoteName;
	};

	cloneOpts.remote_cb = [](git_remote** out, git_repository* repo, const char* /*name*/, const char* url, void* payload) -> int
	{
		auto data = static_cast<remote_cb_payload*>(payload);

		CAutoRemote origin;
		if (auto error = git_remote_create(origin.GetPointer(), repo, data->remoteName, url); error < 0)
			return error;

		*out = origin.Detach();
		return 0;
	};
	remote_cb_payload remote_cb_payloadData;
	remote_cb_payloadData.remoteName = m_remote.IsEmpty() ? "origin" : CUnicodeUtils::GetUTF8(m_remote);
	cloneOpts.remote_cb_payload = &remote_cb_payloadData;

	CStringA checkout_branch = CUnicodeUtils::GetUTF8(m_RefSpec);
	if (!checkout_branch.IsEmpty())
		cloneOpts.checkout_branch = checkout_branch;
	cloneOpts.checkout_opts = checkout_opts;

	CBlockCacheForPath block(m_targetPathList[0].GetWinPathString());
	CAutoRepository cloned_repo;
	if (git_clone(cloned_repo.GetPointer(), CUnicodeUtils::GetUTF8(m_url.GetGitPathString()), CUnicodeUtils::GetUTF8(m_targetPathList[0].GetWinPathString()), &cloneOpts) < 0)
	{
		list->ReportGitError();
		return false;
	}

	// Not setting m_PostCmdCallback here, as clone is only called from CloneCommand.cpp

	return true;
}
