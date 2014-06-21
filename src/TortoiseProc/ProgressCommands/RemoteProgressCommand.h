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
#include "GitProgressList.h"

class RemoteProgressCommand : public ProgressCommand
{
protected:
	CTGitPath m_url;
	CString m_RefSpec;
	CString m_remote;

	static int RemoteProgressCallback(const char* str, int len, void* data)
	{
		CString progText;
		progText = CUnicodeUtils::GetUnicode(CStringA(str, len));
		((CGitProgressList*)data)->SetDlgItemText(IDC_PROGRESSLABEL, progText);
		return 0;
	}
	static int RemoteCompletionCallback(git_remote_completion_type /*type*/, void* /*data*/)
	{
		// this method is unused by libgit2 so far
		// TODO: "m_pAnimate->Stop();" and "m_pAnimate->ShowWindow(SW_HIDE);", cleanup possible in GitProgressList::Notify
		return 0;
	}
	static int RemoteUpdatetipsCallback(const char* refname, const git_oid* oldOid, const git_oid* newOid, void* data)
	{
		((CGitProgressList*)data)->Notify(git_wc_notify_update_ref, CUnicodeUtils::GetUnicode(refname), oldOid, newOid);
		return 0;
	}

public:
	void SetUrl(const CString& url) { m_url.SetFromUnknown(url); }
	void SetRefSpec(CString spec){ m_RefSpec = spec; }
	void SetRemote(const CString& remote) { m_remote = remote; }
};

class CSmartAnimation
{
	CAnimateCtrl* m_pAnimate;

public:
	CSmartAnimation(CAnimateCtrl* pAnimate)
	{
		m_pAnimate = pAnimate;
		if (m_pAnimate)
		{
			m_pAnimate->ShowWindow(SW_SHOW);
			m_pAnimate->Play(0, INT_MAX, INT_MAX);
		}
	}
	~CSmartAnimation()
	{
		if (m_pAnimate)
		{
			m_pAnimate->Stop();
			m_pAnimate->ShowWindow(SW_HIDE);
		}
	}
};
