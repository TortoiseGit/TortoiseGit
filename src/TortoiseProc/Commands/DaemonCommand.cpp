// TortoiseGit - a Windows shell extension for easy version control

// Copyright (C) 2013 - TortoiseGit

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
#include "stdafx.h"
#include "DaemonCommand.h"
#include "ProgressDlg.h"
#include "UnicodeUtils.h"
#include "MessageBox.h"


bool DaemonCommand::Execute()
{
	if (CMessageBox::ShowCheck(NULL, IDS_DAEMON_SECURITY_WARN, IDS_APPNAME, MB_ICONINFORMATION | MB_YESNO, _T("DaemonNoSecurityWarning"), IDS_MSGBOX_DONOTSHOWAGAIN) != IDYES)
		return false;

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != NO_ERROR)
	{
		MessageBox(NULL, _T("WSAStartup failed!"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}

	char hostName[128];
	if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR)
	{
		MessageBox(NULL, _T("gethostname failed!"), _T("TortoiseGit"), MB_OK | MB_ICONERROR);
		return false;
	}

	CString ip = _T("localhost");
	struct hostent *ipList = gethostbyname(hostName);
	for (int i = 0; ipList->h_addr_list[i] != 0; ++i)
	{
		struct in_addr addr;
		memcpy(&addr, ipList->h_addr_list[i], sizeof(struct in_addr));
		CStringA str = inet_ntoa(addr);
		ip = CUnicodeUtils::GetUnicode(str);
		break;
	}

	CString cmd;
	cmd.Format(_T("git.exe daemon --verbose --export-all --base-path=\"%s\""), g_Git.m_CurrentDir);
	CProgressDlg progDlg;
	progDlg.m_GitCmd = cmd;
	progDlg.m_PreText = _T("git://") + ip + _T("/");
	progDlg.DoModal();
	return true;
}
