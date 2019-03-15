// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013-2014, 2016, 2019 - TortoiseGit

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
#include "ResetProgressCommand.h"
#include "AppUtils.h"
#include "../TGitCache/CacheInterface.h"

bool ResetProgressCommand::Run(CGitProgressList* list, CString& sWindowTitle, int& /*m_itemCountTotal*/, int& /*m_itemCount*/)
{
	if (!g_Git.UsingLibGit2(CGit::GIT_CMD_RESET))
	{
		// should never run to here
		ASSERT(0);
		return false;
	}

	list->SetWindowTitle(IDS_PROGRS_TITLE_RESET, g_Git.m_CurrentDir, sWindowTitle);
	list->SetBackgroundImage(IDI_UPDATE_BKG);
	int resetTypesResource[] = { IDS_RESET_SOFT, IDS_RESET_MIXED, IDS_RESET_HARD };
	list->ReportCmd(CString(MAKEINTRESOURCE(IDS_PROGRS_TITLE_RESET)) + L' ' + CString(MAKEINTRESOURCE(resetTypesResource[m_resetType])) + L' ' + m_revision);

	list->ShowProgressBar();
	CAutoRepository repo(g_Git.GetGitRepository());
	if (!repo)
	{
		list->ReportGitError();
		return false;
	}

	CGitProgressList::Payload cbpayload = { list, repo };
	git_checkout_options checkout_options = GIT_CHECKOUT_OPTIONS_INIT;

	CBlockCacheForPath block(g_Git.m_CurrentDir);
	CAutoObject target;
	if (git_revparse_single(target.GetPointer(), repo, CUnicodeUtils::GetUTF8(m_revision)))
		goto error;
	checkout_options.progress_payload = &cbpayload;
	checkout_options.progress_cb = [](const char*, size_t completed_steps, size_t total_steps, void* payload)
	{
		auto cbpayload = static_cast<CGitProgressList::Payload*>(payload);
		cbpayload->list->m_itemCountTotal = static_cast<int>(total_steps);
		cbpayload->list->m_itemCount = static_cast<int>(completed_steps);
	};
	checkout_options.notify_cb = [](git_checkout_notify_t, const char* pPath, const git_diff_file*, const git_diff_file*, const git_diff_file*, void* payload) -> int
	{
		auto list = static_cast<CGitProgressList*>(payload);
		CString path(CUnicodeUtils::GetUnicode(pPath));
		if (DWORD(CRegDWORD(L"Software\\TortoiseGit\\RevertWithRecycleBin", TRUE)))
		{
			if (!CTGitPath(g_Git.CombinePath(path)).Delete(true, true))
			{
				list->ReportError(L"Could move \"" + path + L"\" to recycle bin");
				return GIT_EUSER;
			}
		}
		list->AddNotify(new CGitProgressList::WC_File_NotificationData(path, CGitProgressList::WC_File_NotificationData::git_wc_notify_checkout));
		return 0;
	};
	checkout_options.notify_flags = GIT_CHECKOUT_NOTIFY_UPDATED;
	checkout_options.notify_payload = list;
	if (git_reset(repo, target, static_cast<git_reset_t>(m_resetType + 1), &checkout_options))
		goto error;

	// Not setting m_PostCmdCallback here, as clone is only called from AppUtils.cpp

	return true;

error:
	list->ReportGitError();
	return false;
}
