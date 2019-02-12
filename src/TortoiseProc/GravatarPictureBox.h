// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013, 2016-2019 - TortoiseGit

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
#pragma once
#include <WinInet.h>

class CGravatar : public CStatic
{
public:
	CGravatar();
	virtual ~CGravatar();
	void	Init();
	bool	IsGravatarEnabled() const { return m_bEnableGravatar; }
	void	EnableGravatar(bool value) { m_bEnableGravatar = value; }
	void	LoadGravatar(CString email = L"");

private:
	void	GravatarThread();
	void	SafeTerminateGravatarThread();
	afx_msg void OnPaint();
	int		DownloadToFile(bool* gravatarExit, const HINTERNET hConnectHandle, bool isHttps, const CString& urlpath, const CString& dest);

	bool				m_bEnableGravatar;
	CString				m_filename;
	CString				m_email;
	HANDLE				m_gravatarEvent;
	CWinThread*			m_gravatarThread;
	bool*				m_gravatarExit;
	CComAutoCriticalSection m_gravatarLock;

	DECLARE_MESSAGE_MAP();
};
